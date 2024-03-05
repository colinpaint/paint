//{{{
/*!
 *****************************************************************************
 *
 * \file fmo.c
 *
 * \brief
 *    Support for Flexible Macroblock Ordering (FMO)
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Stephan Wenger      stewe@cs.tu-berlin.de
 *    - Karsten Suehring
 ******************************************************************************
 */
//}}}
//{{{
#include "global.h"
#include "elements.h"
#include "defines.h"

#include "sliceHeader.h"
#include "fmo.h"
//}}}

//#define PRINT_FMO_MAPS

//{{{
static void FmoGenerateType0MapUnitMap (VideoParameters* pVid, unsigned PicSizeInMapUnits ) {
// Generate interleaved slice group map type MapUnit map (type 0)

  pic_parameter_set_rbsp_t* pps = pVid->active_pps;
  unsigned iGroup, j;
  unsigned i = 0;
  do
  {
    for( iGroup = 0;
         (iGroup <= pps->num_slice_groups_minus1) && (i < PicSizeInMapUnits);
         i += pps->run_length_minus1[iGroup++] + 1 )
    {
      for( j = 0; j <= pps->run_length_minus1[ iGroup ] && i + j < PicSizeInMapUnits; j++ )
        pVid->MapUnitToSliceGroupMap[i+j] = iGroup;
    }
  }
  while( i < PicSizeInMapUnits );
}
//}}}
//{{{
// Generate dispersed slice group map type MapUnit map (type 1)
static void FmoGenerateType1MapUnitMap (VideoParameters* pVid, unsigned PicSizeInMapUnits )
{
  pic_parameter_set_rbsp_t* pps = pVid->active_pps;
  unsigned i;
  for( i = 0; i < PicSizeInMapUnits; i++ )
    pVid->MapUnitToSliceGroupMap[i] = ((i%pVid->PicWidthInMbs)+(((i/pVid->PicWidthInMbs)*(pps->num_slice_groups_minus1+1))/2))
                                %(pps->num_slice_groups_minus1+1);
}
//}}}
//{{{
// Generate foreground with left-over slice group map type MapUnit map (type 2)
static void FmoGenerateType2MapUnitMap (VideoParameters* pVid, unsigned PicSizeInMapUnits )
{
  pic_parameter_set_rbsp_t* pps = pVid->active_pps;
  int iGroup;
  unsigned i, x, y;
  unsigned yTopLeft, xTopLeft, yBottomRight, xBottomRight;

  for( i = 0; i < PicSizeInMapUnits; i++ )
    pVid->MapUnitToSliceGroupMap[ i ] = pps->num_slice_groups_minus1;

  for( iGroup = pps->num_slice_groups_minus1 - 1 ; iGroup >= 0; iGroup--) {
    yTopLeft = pps->top_left[ iGroup ] / pVid->PicWidthInMbs;
    xTopLeft = pps->top_left[ iGroup ] % pVid->PicWidthInMbs;
    yBottomRight = pps->bottom_right[ iGroup ] / pVid->PicWidthInMbs;
    xBottomRight = pps->bottom_right[ iGroup ] % pVid->PicWidthInMbs;
    for (y = yTopLeft; y <= yBottomRight; y++ )
      for (x = xTopLeft; x <= xBottomRight; x++ )
        pVid->MapUnitToSliceGroupMap[ y * pVid->PicWidthInMbs + x ] = iGroup;
    }
  }
//}}}
//{{{
// Generate box-out slice group map type MapUnit map (type 3)
static void FmoGenerateType3MapUnitMap (VideoParameters* pVid, unsigned PicSizeInMapUnits, Slice* currSlice )
{
  pic_parameter_set_rbsp_t* pps = pVid->active_pps;
  unsigned i, k;
  int leftBound, topBound, rightBound, bottomBound;
  int x, y, xDir, yDir;
  int mapUnitVacant;

  unsigned mapUnitsInSliceGroup0 = imin((pps->slice_group_change_rate_minus1 + 1) * currSlice->slice_group_change_cycle, PicSizeInMapUnits);

  for( i = 0; i < PicSizeInMapUnits; i++ )
    pVid->MapUnitToSliceGroupMap[ i ] = 2;

  x = ( pVid->PicWidthInMbs - pps->slice_group_change_direction_flag ) / 2;
  y = ( pVid->PicHeightInMapUnits - pps->slice_group_change_direction_flag ) / 2;

  leftBound   = x;
  topBound    = y;
  rightBound  = x;
  bottomBound = y;

  xDir =  pps->slice_group_change_direction_flag - 1;
  yDir =  pps->slice_group_change_direction_flag;

  for( k = 0; k < PicSizeInMapUnits; k += mapUnitVacant ) {
    mapUnitVacant = ( pVid->MapUnitToSliceGroupMap[ y * pVid->PicWidthInMbs + x ]  ==  2 );
    if( mapUnitVacant )
       pVid->MapUnitToSliceGroupMap[ y * pVid->PicWidthInMbs + x ] = ( k >= mapUnitsInSliceGroup0 );

    if( xDir  ==  -1  &&  x  ==  leftBound ) {
      leftBound = imax( leftBound - 1, 0 );
      x = leftBound;
      xDir = 0;
      yDir = 2 * pps->slice_group_change_direction_flag - 1;
    }
    else
      if( xDir  ==  1  &&  x  ==  rightBound ) {
        rightBound = imin( rightBound + 1, (int)pVid->PicWidthInMbs - 1 );
        x = rightBound;
        xDir = 0;
        yDir = 1 - 2 * pps->slice_group_change_direction_flag;
      }
      else
        if( yDir  ==  -1  &&  y  ==  topBound ) {
          topBound = imax( topBound - 1, 0 );
          y = topBound;
          xDir = 1 - 2 * pps->slice_group_change_direction_flag;
          yDir = 0;
         }
        else
          if( yDir  ==  1  &&  y  ==  bottomBound ) {
            bottomBound = imin( bottomBound + 1, (int)pVid->PicHeightInMapUnits - 1 );
            y = bottomBound;
            xDir = 2 * pps->slice_group_change_direction_flag - 1;
            yDir = 0;
          }
          else {
            x = x + xDir;
            y = y + yDir;
          }
  }

}
//}}}
//{{{
static void FmoGenerateType4MapUnitMap (VideoParameters* pVid, unsigned PicSizeInMapUnits, Slice* currSlice) {
// Generate raster scan slice group map type MapUnit map (type 4)

  pic_parameter_set_rbsp_t* pps = pVid->active_pps;

  unsigned mapUnitsInSliceGroup0 = imin((pps->slice_group_change_rate_minus1 + 1) * currSlice->slice_group_change_cycle, PicSizeInMapUnits);
  unsigned sizeOfUpperLeftGroup = pps->slice_group_change_direction_flag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i;

  for (i = 0; i < PicSizeInMapUnits; i++ )
    if (i < sizeOfUpperLeftGroup )
      pVid->MapUnitToSliceGroupMap[ i ] = pps->slice_group_change_direction_flag;
    else
      pVid->MapUnitToSliceGroupMap[ i ] = 1 - pps->slice_group_change_direction_flag;

  }
//}}}
//{{{
// Generate wipe slice group map type MapUnit map (type 5) *
static void FmoGenerateType5MapUnitMap (VideoParameters* pVid, unsigned PicSizeInMapUnits, Slice* currSlice )
{
  pic_parameter_set_rbsp_t* pps = pVid->active_pps;

  unsigned mapUnitsInSliceGroup0 = imin((pps->slice_group_change_rate_minus1 + 1) * currSlice->slice_group_change_cycle, PicSizeInMapUnits);
  unsigned sizeOfUpperLeftGroup = pps->slice_group_change_direction_flag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i,j, k = 0;

  for( j = 0; j < pVid->PicWidthInMbs; j++ )
    for( i = 0; i < pVid->PicHeightInMapUnits; i++ )
        if( k++ < sizeOfUpperLeftGroup )
            pVid->MapUnitToSliceGroupMap[ i * pVid->PicWidthInMbs + j ] = pps->slice_group_change_direction_flag;
        else
            pVid->MapUnitToSliceGroupMap[ i * pVid->PicWidthInMbs + j ] = 1 - pps->slice_group_change_direction_flag;

}
//}}}
//{{{
static void FmoGenerateType6MapUnitMap (VideoParameters* pVid, unsigned PicSizeInMapUnits ) {
// Generate explicit slice group map type MapUnit map (type 6)

  pic_parameter_set_rbsp_t* pps = pVid->active_pps;
  unsigned i;
  for (i=0; i<PicSizeInMapUnits; i++)
    pVid->MapUnitToSliceGroupMap[i] = pps->slice_group_id[i];
  }
//}}}
//{{{
static int FmoGenerateMapUnitToSliceGroupMap (VideoParameters* pVid, Slice* currSlice) {
// Generates pVid->MapUnitToSliceGroupMap
// Has to be called every time a new Picture Parameter Set is used

  seq_parameter_set_rbsp_t* sps = pVid->active_sps;
  pic_parameter_set_rbsp_t* pps = pVid->active_pps;

  unsigned int NumSliceGroupMapUnits;

  NumSliceGroupMapUnits = (sps->pic_height_in_map_units_minus1+1)* (sps->pic_width_in_mbs_minus1+1);

  if (pps->slice_group_map_type == 6)
    if ((pps->pic_size_in_map_units_minus1 + 1) != NumSliceGroupMapUnits)
      error ("wrong pps->pic_size_in_map_units_minus1 for used SPS and FMO type 6", 500);

  // allocate memory for pVid->MapUnitToSliceGroupMap
  if (pVid->MapUnitToSliceGroupMap)
    free (pVid->MapUnitToSliceGroupMap);

  if ((pVid->MapUnitToSliceGroupMap = malloc ((NumSliceGroupMapUnits) * sizeof (int))) == NULL) {
    printf ("cannot allocated %d bytes for pVid->MapUnitToSliceGroupMap, exit\n", (int) ( (pps->pic_size_in_map_units_minus1+1) * sizeof (int)));
    exit (-1);
    }

  if (pps->num_slice_groups_minus1 == 0) {
    // only one slice group
    memset (pVid->MapUnitToSliceGroupMap, 0, NumSliceGroupMapUnits * sizeof (int));
    return 0;
    }

  switch (pps->slice_group_map_type) {
    case 0:
      FmoGenerateType0MapUnitMap (pVid, NumSliceGroupMapUnits);
      break;
    case 1:
      FmoGenerateType1MapUnitMap (pVid, NumSliceGroupMapUnits);
      break;
    case 2:
      FmoGenerateType2MapUnitMap (pVid, NumSliceGroupMapUnits);
      break;
    case 3:
      FmoGenerateType3MapUnitMap (pVid, NumSliceGroupMapUnits, currSlice);
      break;
    case 4:
      FmoGenerateType4MapUnitMap (pVid, NumSliceGroupMapUnits, currSlice);
      break;
    case 5:
      FmoGenerateType5MapUnitMap (pVid, NumSliceGroupMapUnits, currSlice);
      break;
    case 6:
      FmoGenerateType6MapUnitMap (pVid, NumSliceGroupMapUnits);
      break;
    default:
      printf ("Illegal slice_group_map_type %d , exit \n", (int) pps->slice_group_map_type);
      exit (-1);
      }
  return 0;
  }
//}}}
//{{{
// Generates pVid->MbToSliceGroupMap from pVid->MapUnitToSliceGroupMap
static int FmoGenerateMbToSliceGroupMap (VideoParameters* pVid, Slice *pSlice) {

  // allocate memory for pVid->MbToSliceGroupMap
  if (pVid->MbToSliceGroupMap)
    free (pVid->MbToSliceGroupMap);

  if ((pVid->MbToSliceGroupMap = malloc ((pVid->PicSizeInMbs) * sizeof (int))) == NULL) {
    printf ("cannot allocate %d bytes for pVid->MbToSliceGroupMap, exit\n",
            (int) ((pVid->PicSizeInMbs) * sizeof (int)));
    exit (-1);
    }

  seq_parameter_set_rbsp_t* sps = pVid->active_sps;
  if ((sps->frame_mbs_only_flag)|| pSlice->field_pic_flag) {
    int *MbToSliceGroupMap = pVid->MbToSliceGroupMap;
    int *MapUnitToSliceGroupMap = pVid->MapUnitToSliceGroupMap;
    for (unsigned i = 0; i < pVid->PicSizeInMbs; i++)
      *MbToSliceGroupMap++ = *MapUnitToSliceGroupMap++;
    }
  else
    if (sps->mb_adaptive_frame_field_flag  &&  (!pSlice->field_pic_flag)) {
      for (unsigned i = 0; i < pVid->PicSizeInMbs; i++)
        pVid->MbToSliceGroupMap[i] = pVid->MapUnitToSliceGroupMap[i/2];
      }
    else {
      for (unsigned i = 0; i < pVid->PicSizeInMbs; i++)
        pVid->MbToSliceGroupMap[i] = pVid->MapUnitToSliceGroupMap[(i/(2*pVid->PicWidthInMbs))*pVid->PicWidthInMbs+(i%pVid->PicWidthInMbs)];
      }

  return 0;
  }
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    FMO initialization: Generates pVid->MapUnitToSliceGroupMap and pVid->MbToSliceGroupMap.
 *
 * \param pVid
 *      video encoding parameters for current picture
 ************************************************************************
 */
int fmo_init (VideoParameters* pVid, Slice *pSlice)
{
  pic_parameter_set_rbsp_t* pps = pVid->active_pps;

#ifdef PRINT_FMO_MAPS
  unsigned i,j;
#endif

  FmoGenerateMapUnitToSliceGroupMap(pVid, pSlice);
  FmoGenerateMbToSliceGroupMap(pVid, pSlice);

  pVid->NumberOfSliceGroups = pps->num_slice_groups_minus1 + 1;

#ifdef PRINT_FMO_MAPS
  printf("\n");
  printf("FMO Map (Units):\n");

  for (j=0; j<pVid->PicHeightInMapUnits; j++)
  {
    for (i=0; i<pVid->PicWidthInMbs; i++)
    {
      printf("%c",48+pVid->MapUnitToSliceGroupMap[i+j*pVid->PicWidthInMbs]);
    }
    printf("\n");
  }
  printf("\n");
  printf("FMO Map (Mb):\n");

  for (j=0; j<pVid->PicHeightInMbs; j++)
  {
    for (i=0; i<pVid->PicWidthInMbs; i++)
    {
      printf("%c",48 + pVid->MbToSliceGroupMap[i + j * pVid->PicWidthInMbs]);
    }
    printf("\n");
  }
  printf("\n");

#endif

  return 0;
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Free memory allocated by FMO functions
 ************************************************************************
 */
int FmoFinit (VideoParameters* pVid)
{
  if (pVid->MbToSliceGroupMap)
  {
    free (pVid->MbToSliceGroupMap);
    pVid->MbToSliceGroupMap = NULL;
  }
  if (pVid->MapUnitToSliceGroupMap)
  {
    free (pVid->MapUnitToSliceGroupMap);
    pVid->MapUnitToSliceGroupMap = NULL;
  }
  return 0;
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    FmoGetNumberOfSliceGroup(pVid)
 *
 * \par pVid:
 *    VideoParameters
 ************************************************************************
 */
int FmoGetNumberOfSliceGroup (VideoParameters* pVid)
{
  return pVid->NumberOfSliceGroups;
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    FmoGetLastMBOfPicture(pVid)
 *    returns the macroblock number of the last MB in a picture.  This
 *    mb happens to be the last macroblock of the picture if there is only
 *    one slice group
 *
 * \par Input:
 *    None
 ************************************************************************
 */
int FmoGetLastMBOfPicture (VideoParameters* pVid)
{
  return FmoGetLastMBInSliceGroup (pVid, FmoGetNumberOfSliceGroup(pVid)-1);
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    FmoGetLastMBInSliceGroup: Returns MB number of last MB in SG
 *
 * \par Input:
 *    SliceGroupID (0 to 7)
 ************************************************************************
 */

int FmoGetLastMBInSliceGroup (VideoParameters* pVid, int SliceGroup)
{
  int i;

  for (i=pVid->PicSizeInMbs-1; i>=0; i--)
    if (FmoGetSliceGroupId (pVid, i) == SliceGroup)
      return i;
  return -1;

}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Returns SliceGroupID for a given MB
 *
 * \param pVid
 *      video encoding parameters for current picture
 * \param mb
 *    Macroblock number (in scan order)
 ************************************************************************
 */
int FmoGetSliceGroupId (VideoParameters* pVid, int mb)
{
  assert (mb < (int) pVid->PicSizeInMbs);
  assert (pVid->MbToSliceGroupMap != NULL);
  return pVid->MbToSliceGroupMap[mb];
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    FmoGetNextMBBr: Returns the MB-Nr (in scan order) of the next
 *    MB in the (scattered) Slice, -1 if the slice is finished
 * \param pVid
 *      video encoding parameters for current picture
 *
 * \param CurrentMbNr
 *    number of the current macroblock
 ************************************************************************
 */
int FmoGetNextMBNr (VideoParameters* pVid, int CurrentMbNr)
{
  int SliceGroup = FmoGetSliceGroupId (pVid, CurrentMbNr);

  while (++CurrentMbNr<(int)pVid->PicSizeInMbs && pVid->MbToSliceGroupMap [CurrentMbNr] != SliceGroup)
    ;

  if (CurrentMbNr >= (int)pVid->PicSizeInMbs)
    return -1;    // No further MB in this slice (could be end of picture)
  else
    return CurrentMbNr;
}
//}}}
