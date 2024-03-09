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

  sPPS* pps = vidParam->activePPS;
  unsigned iGroup, j;
  unsigned i = 0;
  do
  {
    for( iGroup = 0;
         (iGroup <= pps->numSliceGroupsMinus1) && (i < PicSizeInMapUnits);
         i += pps->runLengthMinus1[iGroup++] + 1 )
    {
      for( j = 0; j <= pps->runLengthMinus1[ iGroup ] && i + j < PicSizeInMapUnits; j++ )
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
  sPPS* pps = vidParam->activePPS;
  unsigned i;
  for( i = 0; i < PicSizeInMapUnits; i++ )
    vidParam->MapUnitToSliceGroupMap[i] = ((i%vidParam->PicWidthInMbs)+(((i/vidParam->PicWidthInMbs)*(pps->numSliceGroupsMinus1+1))/2))
                                %(pps->numSliceGroupsMinus1+1);
}
//}}}
//{{{
// Generate foreground with left-over slice group map type MapUnit map (type 2)
static void FmoGenerateType2MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits )
{
  sPPS* pps = vidParam->activePPS;
  int iGroup;
  unsigned i, x, y;
  unsigned yTopLeft, xTopLeft, yBottomRight, xBottomRight;

  for( i = 0; i < PicSizeInMapUnits; i++ )
    vidParam->MapUnitToSliceGroupMap[ i ] = pps->numSliceGroupsMinus1;

  for( iGroup = pps->numSliceGroupsMinus1 - 1 ; iGroup >= 0; iGroup--) {
    yTopLeft = pps->topLeft[ iGroup ] / vidParam->PicWidthInMbs;
    xTopLeft = pps->topLeft[ iGroup ] % vidParam->PicWidthInMbs;
    yBottomRight = pps->botRight[ iGroup ] / vidParam->PicWidthInMbs;
    xBottomRight = pps->botRight[ iGroup ] % vidParam->PicWidthInMbs;
    for (y = yTopLeft; y <= yBottomRight; y++ )
      for (x = xTopLeft; x <= xBottomRight; x++ )
        vidParam->MapUnitToSliceGroupMap[ y * vidParam->PicWidthInMbs + x ] = iGroup;
    }
  }
//}}}
//{{{
// Generate box-out slice group map type MapUnit map (type 3)
static void FmoGenerateType3MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits, sSlice* curSlice )
{
  sPPS* pps = vidParam->activePPS;
  unsigned i, k;
  int leftBound, topBound, rightBound, bottomBound;
  int x, y, xDir, yDir;
  int mapUnitVacant;

  unsigned mapUnitsInSliceGroup0 = imin((pps->sliceGroupChangeRateMius1 + 1) * curSlice->sliceGroupChangeCycle, PicSizeInMapUnits);

  for( i = 0; i < PicSizeInMapUnits; i++ )
    vidParam->MapUnitToSliceGroupMap[ i ] = 2;

  x = ( vidParam->PicWidthInMbs - pps->sliceGroupChangeDirectionFlag ) / 2;
  y = ( vidParam->PicHeightInMapUnits - pps->sliceGroupChangeDirectionFlag ) / 2;

  leftBound   = x;
  topBound    = y;
  rightBound  = x;
  bottomBound = y;

  xDir =  pps->sliceGroupChangeDirectionFlag - 1;
  yDir =  pps->sliceGroupChangeDirectionFlag;

  for( k = 0; k < PicSizeInMapUnits; k += mapUnitVacant ) {
    mapUnitVacant = ( vidParam->MapUnitToSliceGroupMap[ y * vidParam->PicWidthInMbs + x ]  ==  2 );
    if( mapUnitVacant )
       vidParam->MapUnitToSliceGroupMap[ y * vidParam->PicWidthInMbs + x ] = ( k >= mapUnitsInSliceGroup0 );

    if( xDir  ==  -1  &&  x  ==  leftBound ) {
      leftBound = imax( leftBound - 1, 0 );
      x = leftBound;
      xDir = 0;
      yDir = 2 * pps->sliceGroupChangeDirectionFlag - 1;
    }
    else
      if( xDir  ==  1  &&  x  ==  rightBound ) {
        rightBound = imin( rightBound + 1, (int)vidParam->PicWidthInMbs - 1 );
        x = rightBound;
        xDir = 0;
        yDir = 1 - 2 * pps->sliceGroupChangeDirectionFlag;
      }
      else
        if( yDir  ==  -1  &&  y  ==  topBound ) {
          topBound = imax( topBound - 1, 0 );
          y = topBound;
          xDir = 1 - 2 * pps->sliceGroupChangeDirectionFlag;
          yDir = 0;
         }
        else
          if( yDir  ==  1  &&  y  ==  bottomBound ) {
            bottomBound = imin( bottomBound + 1, (int)vidParam->PicHeightInMapUnits - 1 );
            y = bottomBound;
            xDir = 2 * pps->sliceGroupChangeDirectionFlag - 1;
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
static void FmoGenerateType4MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits, sSlice* curSlice) {
// Generate raster scan slice group map type MapUnit map (type 4)

  sPPS* pps = vidParam->activePPS;

  unsigned mapUnitsInSliceGroup0 = imin((pps->sliceGroupChangeRateMius1 + 1) * curSlice->sliceGroupChangeCycle, PicSizeInMapUnits);
  unsigned sizeOfUpperLeftGroup = pps->sliceGroupChangeDirectionFlag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i;

  for (i = 0; i < PicSizeInMapUnits; i++ )
    if (i < sizeOfUpperLeftGroup )
      vidParam->MapUnitToSliceGroupMap[ i ] = pps->sliceGroupChangeDirectionFlag;
    else
      vidParam->MapUnitToSliceGroupMap[ i ] = 1 - pps->sliceGroupChangeDirectionFlag;

  }
//}}}
//{{{
// Generate wipe slice group map type MapUnit map (type 5) *
static void FmoGenerateType5MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits, sSlice* curSlice )
{
  sPPS* pps = vidParam->activePPS;

  unsigned mapUnitsInSliceGroup0 = imin((pps->sliceGroupChangeRateMius1 + 1) * curSlice->sliceGroupChangeCycle, PicSizeInMapUnits);
  unsigned sizeOfUpperLeftGroup = pps->sliceGroupChangeDirectionFlag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i,j, k = 0;

  for( j = 0; j < vidParam->PicWidthInMbs; j++ )
    for( i = 0; i < vidParam->PicHeightInMapUnits; i++ )
        if( k++ < sizeOfUpperLeftGroup )
            vidParam->MapUnitToSliceGroupMap[ i * vidParam->PicWidthInMbs + j ] = pps->sliceGroupChangeDirectionFlag;
        else
            vidParam->MapUnitToSliceGroupMap[ i * vidParam->PicWidthInMbs + j ] = 1 - pps->sliceGroupChangeDirectionFlag;

}
//}}}
//{{{
static void FmoGenerateType6MapUnitMap (sVidParam* vidParam, unsigned PicSizeInMapUnits ) {
// Generate explicit slice group map type MapUnit map (type 6)

  sPPS* pps = vidParam->activePPS;
  unsigned i;
  for (i=0; i<PicSizeInMapUnits; i++)
    vidParam->MapUnitToSliceGroupMap[i] = pps->sliceGroupId[i];
  }
//}}}
//{{{
static int FmoGenerateMapUnitToSliceGroupMap (sVidParam* vidParam, sSlice* curSlice) {
// Generates vidParam->MapUnitToSliceGroupMap
// Has to be called every time a new Picture Parameter Set is used

  sSPS* sps = vidParam->activeSPS;
  sPPS* pps = vidParam->activePPS;

  unsigned int NumSliceGroupMapUnits;

  NumSliceGroupMapUnits = (sps->pic_height_in_map_units_minus1+1)* (sps->pic_width_in_mbs_minus1+1);

  if (pps->sliceGroupMapType == 6)
    if ((pps->picSizeMapUnitsMinus1 + 1) != NumSliceGroupMapUnits)
      error ("wrong pps->picSizeMapUnitsMinus1 for used SPS and FMO type 6", 500);

  // allocate memory for vidParam->MapUnitToSliceGroupMap
  if (vidParam->MapUnitToSliceGroupMap)
    free (vidParam->MapUnitToSliceGroupMap);

  if ((vidParam->MapUnitToSliceGroupMap = malloc ((NumSliceGroupMapUnits) * sizeof (int))) == NULL) {
    printf ("cannot allocated %d bytes for vidParam->MapUnitToSliceGroupMap, exit\n", (int) ( (pps->picSizeMapUnitsMinus1+1) * sizeof (int)));
    exit (-1);
    }

  if (pps->numSliceGroupsMinus1 == 0) {
    // only one slice group
    memset (vidParam->MapUnitToSliceGroupMap, 0, NumSliceGroupMapUnits * sizeof (int));
    return 0;
    }

  switch (pps->sliceGroupMapType) {
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
      FmoGenerateType3MapUnitMap (vidParam, NumSliceGroupMapUnits, curSlice);
      break;
    case 4:
      FmoGenerateType4MapUnitMap (vidParam, NumSliceGroupMapUnits, curSlice);
      break;
    case 5:
      FmoGenerateType5MapUnitMap (vidParam, NumSliceGroupMapUnits, curSlice);
      break;
    case 6:
      FmoGenerateType6MapUnitMap (vidParam, NumSliceGroupMapUnits);
      break;
    default:
      printf ("Illegal sliceGroupMapType %d , exit \n", (int) pps->sliceGroupMapType);
      exit (-1);
      }
  return 0;
  }
//}}}
//{{{
// Generates vidParam->MbToSliceGroupMap from vidParam->MapUnitToSliceGroupMap
static int FmoGenerateMbToSliceGroupMap (sVidParam* vidParam, sSlice *pSlice) {

  // allocate memory for vidParam->MbToSliceGroupMap
  if (vidParam->MbToSliceGroupMap)
    free (vidParam->MbToSliceGroupMap);

  if ((vidParam->MbToSliceGroupMap = malloc ((vidParam->picSizeInMbs) * sizeof (int))) == NULL) {
    printf ("cannot allocate %d bytes for vidParam->MbToSliceGroupMap, exit\n",
            (int) ((vidParam->picSizeInMbs) * sizeof (int)));
    exit (-1);
    }

  sSPS* sps = vidParam->activeSPS;
  if ((sps->frameMbOnlyFlag)|| pSlice->fieldPicFlag) {
    int *MbToSliceGroupMap = vidParam->MbToSliceGroupMap;
    int *MapUnitToSliceGroupMap = vidParam->MapUnitToSliceGroupMap;
    for (unsigned i = 0; i < vidParam->picSizeInMbs; i++)
      *MbToSliceGroupMap++ = *MapUnitToSliceGroupMap++;
    }
  else
    if (sps->mb_adaptive_frame_field_flag  &&  (!pSlice->fieldPicFlag)) {
      for (unsigned i = 0; i < vidParam->picSizeInMbs; i++)
        vidParam->MbToSliceGroupMap[i] = vidParam->MapUnitToSliceGroupMap[i/2];
      }
    else {
      for (unsigned i = 0; i < vidParam->picSizeInMbs; i++)
        vidParam->MbToSliceGroupMap[i] = vidParam->MapUnitToSliceGroupMap[(i/(2*vidParam->PicWidthInMbs))*vidParam->PicWidthInMbs+(i%vidParam->PicWidthInMbs)];
      }

  return 0;
  }
//}}}

//{{{
int fmo_init (sVidParam* vidParam, sSlice* pSlice) {

  sPPS* pps = vidParam->activePPS;

  FmoGenerateMapUnitToSliceGroupMap (vidParam, pSlice);
  FmoGenerateMbToSliceGroupMap (vidParam, pSlice);
  vidParam->NumberOfSliceGroups = pps->numSliceGroupsMinus1 + 1;

#ifdef PRINT_FMO_MAPS
  printf ("\n");
  printf ("FMO Map (Units):\n");

  for (int j = 0; j<vidParam->PicHeightInMapUnits; j++) {
    for (int i = 0; i<vidParam->PicWidthInMbs; i++) {
      printf("%c",48+vidParam->MapUnitToSliceGroupMap[i+j*vidParam->PicWidthInMbs]);
      }
    printf("\n");
    }

  printf("\n");
  printf("FMO Map (Mb):\n");

  for (int j = 0; j < vidParam->picHeightInMbs; j++) {
    for (int i = 0; i < vidParam->PicWidthInMbs; i++) {
      printf ("%c", 48 + vidParam->MbToSliceGroupMap[i + j * vidParam->PicWidthInMbs]);
      }
    printf ("\n");
    }
  printf ("\n");

#endif

  return 0;
  }
//}}}

//{{{
int FmoFinit (sVidParam* vidParam) {

  if (vidParam->MbToSliceGroupMap) {
    free (vidParam->MbToSliceGroupMap);
    vidParam->MbToSliceGroupMap = NULL;
    }

  if (vidParam->MapUnitToSliceGroupMap) { 
    free (vidParam->MapUnitToSliceGroupMap);
    vidParam->MapUnitToSliceGroupMap = NULL;
    }

  return 0;
  }
//}}}
//{{{
int FmoGetNumberOfSliceGroup (sVidParam* vidParam)
{
  return vidParam->NumberOfSliceGroups;
}
//}}}
//{{{
int FmoGetLastMBOfPicture (sVidParam* vidParam)
{
  return FmoGetLastMBInSliceGroup (vidParam, FmoGetNumberOfSliceGroup(vidParam)-1);
}
//}}}
//{{{
int FmoGetLastMBInSliceGroup (sVidParam* vidParam, int SliceGroup)
{
  int i;

  for (i=vidParam->picSizeInMbs-1; i>=0; i--)
    if (FmoGetSliceGroupId (vidParam, i) == SliceGroup)
      return i;
  return -1;

}
//}}}
//{{{
int FmoGetSliceGroupId (sVidParam* vidParam, int mb)
{
  assert (mb < (int) vidParam->picSizeInMbs);
  assert (vidParam->MbToSliceGroupMap != NULL);
  return vidParam->MbToSliceGroupMap[mb];
}
//}}}
//{{{
int FmoGetNextMBNr (sVidParam* vidParam, int CurrentMbNr)
{
  int SliceGroup = FmoGetSliceGroupId (vidParam, CurrentMbNr);

  while (++CurrentMbNr<(int)vidParam->picSizeInMbs && vidParam->MbToSliceGroupMap [CurrentMbNr] != SliceGroup)
    ;

  if (CurrentMbNr >= (int)vidParam->picSizeInMbs)
    return -1;    // No further MB in this slice (could be end of picture)
  else
    return CurrentMbNr;
}
//}}}
