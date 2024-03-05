//{{{  includes
#include "global.h"
#include "elements.h"
#include "defines.h"

#include "sliceHeader.h"
#include "fmo.h"
//}}}

//#define PRINT_FMO_MAPS

//{{{
static void FmoGenerateType0MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits ) {
// Generate interleaved slice group map type MapUnit map (type 0)

  sPPSrbsp* pps = vidParam->active_pps;
  unsigned iGroup, j;
  unsigned i = 0;
  do
  {
    for( iGroup = 0;
         (iGroup <= pps->num_slice_groups_minus1) && (i < PicSizeInMapUnits);
         i += pps->run_length_minus1[iGroup++] + 1 )
    {
      for( j = 0; j <= pps->run_length_minus1[ iGroup ] && i + j < PicSizeInMapUnits; j++ )
        vidParam->MapUnitToSliceGroupMap[i+j] = iGroup;
    }
  }
  while( i < PicSizeInMapUnits );
}
//}}}
//{{{
// Generate dispersed slice group map type MapUnit map (type 1)
static void FmoGenerateType1MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits )
{
  sPPSrbsp* pps = vidParam->active_pps;
  unsigned i;
  for( i = 0; i < PicSizeInMapUnits; i++ )
    vidParam->MapUnitToSliceGroupMap[i] = ((i%vidParam->PicWidthInMbs)+(((i/vidParam->PicWidthInMbs)*(pps->num_slice_groups_minus1+1))/2))
                                %(pps->num_slice_groups_minus1+1);
}
//}}}
//{{{
// Generate foreground with left-over slice group map type MapUnit map (type 2)
static void FmoGenerateType2MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits )
{
  sPPSrbsp* pps = vidParam->active_pps;
  int iGroup;
  unsigned i, x, y;
  unsigned yTopLeft, xTopLeft, yBottomRight, xBottomRight;

  for( i = 0; i < PicSizeInMapUnits; i++ )
    vidParam->MapUnitToSliceGroupMap[ i ] = pps->num_slice_groups_minus1;

  for( iGroup = pps->num_slice_groups_minus1 - 1 ; iGroup >= 0; iGroup--) {
    yTopLeft = pps->top_left[ iGroup ] / vidParam->PicWidthInMbs;
    xTopLeft = pps->top_left[ iGroup ] % vidParam->PicWidthInMbs;
    yBottomRight = pps->bottom_right[ iGroup ] / vidParam->PicWidthInMbs;
    xBottomRight = pps->bottom_right[ iGroup ] % vidParam->PicWidthInMbs;
    for (y = yTopLeft; y <= yBottomRight; y++ )
      for (x = xTopLeft; x <= xBottomRight; x++ )
        vidParam->MapUnitToSliceGroupMap[ y * vidParam->PicWidthInMbs + x ] = iGroup;
    }
  }
//}}}
//{{{
// Generate box-out slice group map type MapUnit map (type 3)
static void FmoGenerateType3MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits, mSlice* currSlice )
{
  sPPSrbsp* pps = vidParam->active_pps;
  unsigned i, k;
  int leftBound, topBound, rightBound, bottomBound;
  int x, y, xDir, yDir;
  int mapUnitVacant;

  unsigned mapUnitsInSliceGroup0 = imin((pps->slice_group_change_rate_minus1 + 1) * currSlice->slice_group_change_cycle, PicSizeInMapUnits);

  for( i = 0; i < PicSizeInMapUnits; i++ )
    vidParam->MapUnitToSliceGroupMap[ i ] = 2;

  x = ( vidParam->PicWidthInMbs - pps->slice_group_change_direction_flag ) / 2;
  y = ( vidParam->PicHeightInMapUnits - pps->slice_group_change_direction_flag ) / 2;

  leftBound   = x;
  topBound    = y;
  rightBound  = x;
  bottomBound = y;

  xDir =  pps->slice_group_change_direction_flag - 1;
  yDir =  pps->slice_group_change_direction_flag;

  for( k = 0; k < PicSizeInMapUnits; k += mapUnitVacant ) {
    mapUnitVacant = ( vidParam->MapUnitToSliceGroupMap[ y * vidParam->PicWidthInMbs + x ]  ==  2 );
    if( mapUnitVacant )
       vidParam->MapUnitToSliceGroupMap[ y * vidParam->PicWidthInMbs + x ] = ( k >= mapUnitsInSliceGroup0 );

    if( xDir  ==  -1  &&  x  ==  leftBound ) {
      leftBound = imax( leftBound - 1, 0 );
      x = leftBound;
      xDir = 0;
      yDir = 2 * pps->slice_group_change_direction_flag - 1;
    }
    else
      if( xDir  ==  1  &&  x  ==  rightBound ) {
        rightBound = imin( rightBound + 1, (int)vidParam->PicWidthInMbs - 1 );
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
            bottomBound = imin( bottomBound + 1, (int)vidParam->PicHeightInMapUnits - 1 );
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
static void FmoGenerateType4MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits, mSlice* currSlice) {
// Generate raster scan slice group map type MapUnit map (type 4)

  sPPSrbsp* pps = vidParam->active_pps;

  unsigned mapUnitsInSliceGroup0 = imin((pps->slice_group_change_rate_minus1 + 1) * currSlice->slice_group_change_cycle, PicSizeInMapUnits);
  unsigned sizeOfUpperLeftGroup = pps->slice_group_change_direction_flag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i;

  for (i = 0; i < PicSizeInMapUnits; i++ )
    if (i < sizeOfUpperLeftGroup )
      vidParam->MapUnitToSliceGroupMap[ i ] = pps->slice_group_change_direction_flag;
    else
      vidParam->MapUnitToSliceGroupMap[ i ] = 1 - pps->slice_group_change_direction_flag;

  }
//}}}
//{{{
// Generate wipe slice group map type MapUnit map (type 5) *
static void FmoGenerateType5MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits, mSlice* currSlice )
{
  sPPSrbsp* pps = vidParam->active_pps;

  unsigned mapUnitsInSliceGroup0 = imin((pps->slice_group_change_rate_minus1 + 1) * currSlice->slice_group_change_cycle, PicSizeInMapUnits);
  unsigned sizeOfUpperLeftGroup = pps->slice_group_change_direction_flag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i,j, k = 0;

  for( j = 0; j < vidParam->PicWidthInMbs; j++ )
    for( i = 0; i < vidParam->PicHeightInMapUnits; i++ )
        if( k++ < sizeOfUpperLeftGroup )
            vidParam->MapUnitToSliceGroupMap[ i * vidParam->PicWidthInMbs + j ] = pps->slice_group_change_direction_flag;
        else
            vidParam->MapUnitToSliceGroupMap[ i * vidParam->PicWidthInMbs + j ] = 1 - pps->slice_group_change_direction_flag;

}
//}}}
//{{{
static void FmoGenerateType6MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits ) {
// Generate explicit slice group map type MapUnit map (type 6)

  sPPSrbsp* pps = vidParam->active_pps;
  unsigned i;
  for (i=0; i<PicSizeInMapUnits; i++)
    vidParam->MapUnitToSliceGroupMap[i] = pps->slice_group_id[i];
  }
//}}}
//{{{
static int FmoGenerateMapUnitToSliceGroupMap (sVidParam* vidParam, mSlice* currSlice) {
// Generates vidParam->MapUnitToSliceGroupMap
// Has to be called every time a new Picture Parameter Set is used

  sSPSrbsp* sps = vidParam->active_sps;
  sPPSrbsp* pps = vidParam->active_pps;

  unsigned int NumSliceGroupMapUnits;

  NumSliceGroupMapUnits = (sps->pic_height_in_map_units_minus1+1)* (sps->pic_width_in_mbs_minus1+1);

  if (pps->slice_group_map_type == 6)
    if ((pps->pic_size_in_map_units_minus1 + 1) != NumSliceGroupMapUnits)
      error ("wrong pps->pic_size_in_map_units_minus1 for used SPS and FMO type 6", 500);

  // allocate memory for vidParam->MapUnitToSliceGroupMap
  if (vidParam->MapUnitToSliceGroupMap)
    free (vidParam->MapUnitToSliceGroupMap);

  if ((vidParam->MapUnitToSliceGroupMap = malloc ((NumSliceGroupMapUnits) * sizeof (int))) == NULL) {
    printf ("cannot allocated %d bytes for vidParam->MapUnitToSliceGroupMap, exit\n", (int) ( (pps->pic_size_in_map_units_minus1+1) * sizeof (int)));
    exit (-1);
    }

  if (pps->num_slice_groups_minus1 == 0) {
    // only one slice group
    memset (vidParam->MapUnitToSliceGroupMap, 0, NumSliceGroupMapUnits * sizeof (int));
    return 0;
    }

  switch (pps->slice_group_map_type) {
    case 0:
      FmoGenerateType0MapUnitMap (vidParam, NumSliceGroupMapUnits);
      break;
    case 1:
      FmoGenerateType1MapUnitMap (vidParam, NumSliceGroupMapUnits);
      break;
    case 2:
      FmoGenerateType2MapUnitMap (vidParam, NumSliceGroupMapUnits);
      break;
    case 3:
      FmoGenerateType3MapUnitMap (vidParam, NumSliceGroupMapUnits, currSlice);
      break;
    case 4:
      FmoGenerateType4MapUnitMap (vidParam, NumSliceGroupMapUnits, currSlice);
      break;
    case 5:
      FmoGenerateType5MapUnitMap (vidParam, NumSliceGroupMapUnits, currSlice);
      break;
    case 6:
      FmoGenerateType6MapUnitMap (vidParam, NumSliceGroupMapUnits);
      break;
    default:
      printf ("Illegal slice_group_map_type %d , exit \n", (int) pps->slice_group_map_type);
      exit (-1);
      }
  return 0;
  }
//}}}
//{{{
// Generates vidParam->MbToSliceGroupMap from vidParam->MapUnitToSliceGroupMap
static int FmoGenerateMbToSliceGroupMap (sVidParam* vidParam, mSlice *pSlice) {

  // allocate memory for vidParam->MbToSliceGroupMap
  if (vidParam->MbToSliceGroupMap)
    free (vidParam->MbToSliceGroupMap);

  if ((vidParam->MbToSliceGroupMap = malloc ((vidParam->PicSizeInMbs) * sizeof (int))) == NULL) {
    printf ("cannot allocate %d bytes for vidParam->MbToSliceGroupMap, exit\n",
            (int) ((vidParam->PicSizeInMbs) * sizeof (int)));
    exit (-1);
    }

  sSPSrbsp* sps = vidParam->active_sps;
  if ((sps->frame_mbs_only_flag)|| pSlice->field_pic_flag) {
    int *MbToSliceGroupMap = vidParam->MbToSliceGroupMap;
    int *MapUnitToSliceGroupMap = vidParam->MapUnitToSliceGroupMap;
    for (unsigned i = 0; i < vidParam->PicSizeInMbs; i++)
      *MbToSliceGroupMap++ = *MapUnitToSliceGroupMap++;
    }
  else
    if (sps->mb_adaptive_frame_field_flag  &&  (!pSlice->field_pic_flag)) {
      for (unsigned i = 0; i < vidParam->PicSizeInMbs; i++)
        vidParam->MbToSliceGroupMap[i] = vidParam->MapUnitToSliceGroupMap[i/2];
      }
    else {
      for (unsigned i = 0; i < vidParam->PicSizeInMbs; i++)
        vidParam->MbToSliceGroupMap[i] = vidParam->MapUnitToSliceGroupMap[(i/(2*vidParam->PicWidthInMbs))*vidParam->PicWidthInMbs+(i%vidParam->PicWidthInMbs)];
      }

  return 0;
  }
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    FMO initialization: Generates vidParam->MapUnitToSliceGroupMap and vidParam->MbToSliceGroupMap.
 *
 * \param vidParam
 *      video encoding parameters for current picture
 ************************************************************************
 */
int fmo_init (sVidParam* vidParam, mSlice *pSlice)
{
  sPPSrbsp* pps = vidParam->active_pps;

#ifdef PRINT_FMO_MAPS
  unsigned i,j;
#endif

  FmoGenerateMapUnitToSliceGroupMap(vidParam, pSlice);
  FmoGenerateMbToSliceGroupMap(vidParam, pSlice);

  vidParam->NumberOfSliceGroups = pps->num_slice_groups_minus1 + 1;

#ifdef PRINT_FMO_MAPS
  printf("\n");
  printf("FMO Map (Units):\n");

  for (j=0; j<vidParam->PicHeightInMapUnits; j++)
  {
    for (i=0; i<vidParam->PicWidthInMbs; i++)
    {
      printf("%c",48+vidParam->MapUnitToSliceGroupMap[i+j*vidParam->PicWidthInMbs]);
    }
    printf("\n");
  }
  printf("\n");
  printf("FMO Map (Mb):\n");

  for (j=0; j<vidParam->PicHeightInMbs; j++)
  {
    for (i=0; i<vidParam->PicWidthInMbs; i++)
    {
      printf("%c",48 + vidParam->MbToSliceGroupMap[i + j * vidParam->PicWidthInMbs]);
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
int FmoFinit (sVidParam* vidParam)
{
  if (vidParam->MbToSliceGroupMap)
  {
    free (vidParam->MbToSliceGroupMap);
    vidParam->MbToSliceGroupMap = NULL;
  }
  if (vidParam->MapUnitToSliceGroupMap)
  {
    free (vidParam->MapUnitToSliceGroupMap);
    vidParam->MapUnitToSliceGroupMap = NULL;
  }
  return 0;
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    FmoGetNumberOfSliceGroup(vidParam)
 *
 * \par vidParam:
 *    sVidParam
 ************************************************************************
 */
int FmoGetNumberOfSliceGroup (sVidParam* vidParam)
{
  return vidParam->NumberOfSliceGroups;
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    FmoGetLastMBOfPicture(vidParam)
 *    returns the macroblock number of the last MB in a picture.  This
 *    mb happens to be the last macroblock of the picture if there is only
 *    one slice group
 *
 * \par Input:
 *    None
 ************************************************************************
 */
int FmoGetLastMBOfPicture (sVidParam* vidParam)
{
  return FmoGetLastMBInSliceGroup (vidParam, FmoGetNumberOfSliceGroup(vidParam)-1);
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

int FmoGetLastMBInSliceGroup (sVidParam* vidParam, int SliceGroup)
{
  int i;

  for (i=vidParam->PicSizeInMbs-1; i>=0; i--)
    if (FmoGetSliceGroupId (vidParam, i) == SliceGroup)
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
 * \param vidParam
 *      video encoding parameters for current picture
 * \param mb
 *    sMacroblock number (in scan order)
 ************************************************************************
 */
int FmoGetSliceGroupId (sVidParam* vidParam, int mb)
{
  assert (mb < (int) vidParam->PicSizeInMbs);
  assert (vidParam->MbToSliceGroupMap != NULL);
  return vidParam->MbToSliceGroupMap[mb];
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    FmoGetNextMBBr: Returns the MB-Nr (in scan order) of the next
 *    MB in the (scattered) mSlice, -1 if the slice is finished
 * \param vidParam
 *      video encoding parameters for current picture
 *
 * \param CurrentMbNr
 *    number of the current macroblock
 ************************************************************************
 */
int FmoGetNextMBNr (sVidParam* vidParam, int CurrentMbNr)
{
  int SliceGroup = FmoGetSliceGroupId (vidParam, CurrentMbNr);

  while (++CurrentMbNr<(int)vidParam->PicSizeInMbs && vidParam->MbToSliceGroupMap [CurrentMbNr] != SliceGroup)
    ;

  if (CurrentMbNr >= (int)vidParam->PicSizeInMbs)
    return -1;    // No further MB in this slice (could be end of picture)
  else
    return CurrentMbNr;
}
//}}}
