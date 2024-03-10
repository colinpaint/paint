//{{{  includes
#include "global.h"
#include "elements.h"
#include "defines.h"

#include "sliceHeader.h"
#include "fmo.h"
//}}}
//#define PRINT_FMO_MAPS

//{{{
static void FmoGenerateType0MapUnitMap (sDecoder* decoder, unsigned PicSizeInMapUnits ) {
// Generate interleaved slice group map type MapUnit map (type 0)

  sPPS* pps = decoder->activePPS;
  unsigned iGroup, j;
  unsigned i = 0;
  do
  {
    for( iGroup = 0;
         (iGroup <= pps->numSliceGroupsMinus1) && (i < PicSizeInMapUnits);
         i += pps->runLengthMinus1[iGroup++] + 1 )
    {
      for( j = 0; j <= pps->runLengthMinus1[ iGroup ] && i + j < PicSizeInMapUnits; j++ )
        decoder->MapUnitToSliceGroupMap[i+j] = iGroup;
    }
  }
  while( i < PicSizeInMapUnits );
}
//}}}
//{{{
// Generate dispersed slice group map type MapUnit map (type 1)
static void FmoGenerateType1MapUnitMap (sDecoder* decoder, unsigned PicSizeInMapUnits )
{
  sPPS* pps = decoder->activePPS;
  unsigned i;
  for( i = 0; i < PicSizeInMapUnits; i++ )
    decoder->MapUnitToSliceGroupMap[i] = ((i%decoder->PicWidthInMbs)+(((i/decoder->PicWidthInMbs)*(pps->numSliceGroupsMinus1+1))/2))
                                %(pps->numSliceGroupsMinus1+1);
}
//}}}
//{{{
// Generate foreground with left-over slice group map type MapUnit map (type 2)
static void FmoGenerateType2MapUnitMap (sDecoder* decoder, unsigned PicSizeInMapUnits )
{
  sPPS* pps = decoder->activePPS;
  int iGroup;
  unsigned i, x, y;
  unsigned yTopLeft, xTopLeft, yBottomRight, xBottomRight;

  for( i = 0; i < PicSizeInMapUnits; i++ )
    decoder->MapUnitToSliceGroupMap[ i ] = pps->numSliceGroupsMinus1;

  for( iGroup = pps->numSliceGroupsMinus1 - 1 ; iGroup >= 0; iGroup--) {
    yTopLeft = pps->topLeft[ iGroup ] / decoder->PicWidthInMbs;
    xTopLeft = pps->topLeft[ iGroup ] % decoder->PicWidthInMbs;
    yBottomRight = pps->botRight[ iGroup ] / decoder->PicWidthInMbs;
    xBottomRight = pps->botRight[ iGroup ] % decoder->PicWidthInMbs;
    for (y = yTopLeft; y <= yBottomRight; y++ )
      for (x = xTopLeft; x <= xBottomRight; x++ )
        decoder->MapUnitToSliceGroupMap[ y * decoder->PicWidthInMbs + x ] = iGroup;
    }
  }
//}}}
//{{{
// Generate box-out slice group map type MapUnit map (type 3)
static void FmoGenerateType3MapUnitMap (sDecoder* decoder, unsigned PicSizeInMapUnits, sSlice* curSlice )
{
  sPPS* pps = decoder->activePPS;
  unsigned i, k;
  int leftBound, topBound, rightBound, bottomBound;
  int x, y, xDir, yDir;
  int mapUnitVacant;

  unsigned mapUnitsInSliceGroup0 = imin((pps->sliceGroupChangeRateMius1 + 1) * curSlice->sliceGroupChangeCycle, PicSizeInMapUnits);

  for( i = 0; i < PicSizeInMapUnits; i++ )
    decoder->MapUnitToSliceGroupMap[ i ] = 2;

  x = ( decoder->PicWidthInMbs - pps->sliceGroupChangeDirectionFlag ) / 2;
  y = ( decoder->PicHeightInMapUnits - pps->sliceGroupChangeDirectionFlag ) / 2;

  leftBound   = x;
  topBound    = y;
  rightBound  = x;
  bottomBound = y;

  xDir =  pps->sliceGroupChangeDirectionFlag - 1;
  yDir =  pps->sliceGroupChangeDirectionFlag;

  for( k = 0; k < PicSizeInMapUnits; k += mapUnitVacant ) {
    mapUnitVacant = ( decoder->MapUnitToSliceGroupMap[ y * decoder->PicWidthInMbs + x ]  ==  2 );
    if( mapUnitVacant )
       decoder->MapUnitToSliceGroupMap[ y * decoder->PicWidthInMbs + x ] = ( k >= mapUnitsInSliceGroup0 );

    if( xDir  ==  -1  &&  x  ==  leftBound ) {
      leftBound = imax( leftBound - 1, 0 );
      x = leftBound;
      xDir = 0;
      yDir = 2 * pps->sliceGroupChangeDirectionFlag - 1;
    }
    else
      if( xDir  ==  1  &&  x  ==  rightBound ) {
        rightBound = imin( rightBound + 1, (int)decoder->PicWidthInMbs - 1 );
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
            bottomBound = imin( bottomBound + 1, (int)decoder->PicHeightInMapUnits - 1 );
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
static void FmoGenerateType4MapUnitMap (sDecoder* decoder, unsigned PicSizeInMapUnits, sSlice* curSlice) {
// Generate raster scan slice group map type MapUnit map (type 4)

  sPPS* pps = decoder->activePPS;

  unsigned mapUnitsInSliceGroup0 = imin((pps->sliceGroupChangeRateMius1 + 1) * curSlice->sliceGroupChangeCycle, PicSizeInMapUnits);
  unsigned sizeOfUpperLeftGroup = pps->sliceGroupChangeDirectionFlag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i;

  for (i = 0; i < PicSizeInMapUnits; i++ )
    if (i < sizeOfUpperLeftGroup )
      decoder->MapUnitToSliceGroupMap[ i ] = pps->sliceGroupChangeDirectionFlag;
    else
      decoder->MapUnitToSliceGroupMap[ i ] = 1 - pps->sliceGroupChangeDirectionFlag;

  }
//}}}
//{{{
// Generate wipe slice group map type MapUnit map (type 5) *
static void FmoGenerateType5MapUnitMap (sDecoder* decoder, unsigned PicSizeInMapUnits, sSlice* curSlice )
{
  sPPS* pps = decoder->activePPS;

  unsigned mapUnitsInSliceGroup0 = imin((pps->sliceGroupChangeRateMius1 + 1) * curSlice->sliceGroupChangeCycle, PicSizeInMapUnits);
  unsigned sizeOfUpperLeftGroup = pps->sliceGroupChangeDirectionFlag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  unsigned i,j, k = 0;

  for( j = 0; j < decoder->PicWidthInMbs; j++ )
    for( i = 0; i < decoder->PicHeightInMapUnits; i++ )
        if( k++ < sizeOfUpperLeftGroup )
            decoder->MapUnitToSliceGroupMap[ i * decoder->PicWidthInMbs + j ] = pps->sliceGroupChangeDirectionFlag;
        else
            decoder->MapUnitToSliceGroupMap[ i * decoder->PicWidthInMbs + j ] = 1 - pps->sliceGroupChangeDirectionFlag;

}
//}}}
//{{{
static void FmoGenerateType6MapUnitMap (sDecoder* decoder, unsigned PicSizeInMapUnits ) {
// Generate explicit slice group map type MapUnit map (type 6)

  sPPS* pps = decoder->activePPS;
  unsigned i;
  for (i=0; i<PicSizeInMapUnits; i++)
    decoder->MapUnitToSliceGroupMap[i] = pps->sliceGroupId[i];
  }
//}}}
//{{{
static int FmoGenerateMapUnitToSliceGroupMap (sDecoder* decoder, sSlice* curSlice) {
// Generates decoder->MapUnitToSliceGroupMap
// Has to be called every time a new Picture Parameter Set is used

  sSPS* sps = decoder->activeSPS;
  sPPS* pps = decoder->activePPS;

  unsigned int NumSliceGroupMapUnits;

  NumSliceGroupMapUnits = (sps->pic_height_in_map_units_minus1+1)* (sps->pic_width_in_mbs_minus1+1);

  if (pps->sliceGroupMapType == 6)
    if ((pps->picSizeMapUnitsMinus1 + 1) != NumSliceGroupMapUnits)
      error ("wrong pps->picSizeMapUnitsMinus1 for used SPS and FMO type 6", 500);

  // allocate memory for decoder->MapUnitToSliceGroupMap
  if (decoder->MapUnitToSliceGroupMap)
    free (decoder->MapUnitToSliceGroupMap);

  if ((decoder->MapUnitToSliceGroupMap = malloc ((NumSliceGroupMapUnits) * sizeof (int))) == NULL) {
    printf ("cannot allocated %d bytes for decoder->MapUnitToSliceGroupMap, exit\n", (int) ( (pps->picSizeMapUnitsMinus1+1) * sizeof (int)));
    exit (-1);
    }

  if (pps->numSliceGroupsMinus1 == 0) {
    // only one slice group
    memset (decoder->MapUnitToSliceGroupMap, 0, NumSliceGroupMapUnits * sizeof (int));
    return 0;
    }

  switch (pps->sliceGroupMapType) {
    case 0:
      FmoGenerateType0MapUnitMap (decoder, NumSliceGroupMapUnits);
      break;
    case 1:
      FmoGenerateType1MapUnitMap (decoder, NumSliceGroupMapUnits);
      break;
    case 2:
      FmoGenerateType2MapUnitMap (decoder, NumSliceGroupMapUnits);
      break;
    case 3:
      FmoGenerateType3MapUnitMap (decoder, NumSliceGroupMapUnits, curSlice);
      break;
    case 4:
      FmoGenerateType4MapUnitMap (decoder, NumSliceGroupMapUnits, curSlice);
      break;
    case 5:
      FmoGenerateType5MapUnitMap (decoder, NumSliceGroupMapUnits, curSlice);
      break;
    case 6:
      FmoGenerateType6MapUnitMap (decoder, NumSliceGroupMapUnits);
      break;
    default:
      printf ("Illegal sliceGroupMapType %d , exit \n", (int) pps->sliceGroupMapType);
      exit (-1);
      }
  return 0;
  }
//}}}
//{{{
// Generates decoder->MbToSliceGroupMap from decoder->MapUnitToSliceGroupMap
static int FmoGenerateMbToSliceGroupMap (sDecoder* decoder, sSlice *pSlice) {

  // allocate memory for decoder->MbToSliceGroupMap
  if (decoder->MbToSliceGroupMap)
    free (decoder->MbToSliceGroupMap);

  if ((decoder->MbToSliceGroupMap = malloc ((decoder->picSizeInMbs) * sizeof (int))) == NULL) {
    printf ("cannot allocate %d bytes for decoder->MbToSliceGroupMap, exit\n",
            (int) ((decoder->picSizeInMbs) * sizeof (int)));
    exit (-1);
    }

  sSPS* sps = decoder->activeSPS;
  if ((sps->frameMbOnlyFlag)|| pSlice->fieldPicFlag) {
    int *MbToSliceGroupMap = decoder->MbToSliceGroupMap;
    int *MapUnitToSliceGroupMap = decoder->MapUnitToSliceGroupMap;
    for (unsigned i = 0; i < decoder->picSizeInMbs; i++)
      *MbToSliceGroupMap++ = *MapUnitToSliceGroupMap++;
    }
  else
    if (sps->mb_adaptive_frame_field_flag  &&  (!pSlice->fieldPicFlag)) {
      for (unsigned i = 0; i < decoder->picSizeInMbs; i++)
        decoder->MbToSliceGroupMap[i] = decoder->MapUnitToSliceGroupMap[i/2];
      }
    else {
      for (unsigned i = 0; i < decoder->picSizeInMbs; i++)
        decoder->MbToSliceGroupMap[i] = decoder->MapUnitToSliceGroupMap[(i/(2*decoder->PicWidthInMbs))*decoder->PicWidthInMbs+(i%decoder->PicWidthInMbs)];
      }

  return 0;
  }
//}}}

//{{{
int initFmo (sDecoder* decoder, sSlice* pSlice) {

  sPPS* pps = decoder->activePPS;

  FmoGenerateMapUnitToSliceGroupMap (decoder, pSlice);
  FmoGenerateMbToSliceGroupMap (decoder, pSlice);
  decoder->NumberOfSliceGroups = pps->numSliceGroupsMinus1 + 1;

#ifdef PRINT_FMO_MAPS
  printf ("\n");
  printf ("FMO Map (Units):\n");

  for (int j = 0; j<decoder->PicHeightInMapUnits; j++) {
    for (int i = 0; i<decoder->PicWidthInMbs; i++) {
      printf("%c",48+decoder->MapUnitToSliceGroupMap[i+j*decoder->PicWidthInMbs]);
      }
    printf("\n");
    }

  printf("\n");
  printf("FMO Map (Mb):\n");

  for (int j = 0; j < decoder->picHeightInMbs; j++) {
    for (int i = 0; i < decoder->PicWidthInMbs; i++) {
      printf ("%c", 48 + decoder->MbToSliceGroupMap[i + j * decoder->PicWidthInMbs]);
      }
    printf ("\n");
    }
  printf ("\n");

#endif

  return 0;
  }
//}}}

//{{{
int FmoFinit (sDecoder* decoder) {

  if (decoder->MbToSliceGroupMap) {
    free (decoder->MbToSliceGroupMap);
    decoder->MbToSliceGroupMap = NULL;
    }

  if (decoder->MapUnitToSliceGroupMap) { 
    free (decoder->MapUnitToSliceGroupMap);
    decoder->MapUnitToSliceGroupMap = NULL;
    }

  return 0;
  }
//}}}
//{{{
int FmoGetNumberOfSliceGroup (sDecoder* decoder)
{
  return decoder->NumberOfSliceGroups;
}
//}}}
//{{{
int FmoGetLastMBOfPicture (sDecoder* decoder)
{
  return FmoGetLastMBInSliceGroup (decoder, FmoGetNumberOfSliceGroup(decoder)-1);
}
//}}}
//{{{
int FmoGetLastMBInSliceGroup (sDecoder* decoder, int SliceGroup)
{
  int i;

  for (i=decoder->picSizeInMbs-1; i>=0; i--)
    if (FmoGetSliceGroupId (decoder, i) == SliceGroup)
      return i;
  return -1;

}
//}}}
//{{{
int FmoGetSliceGroupId (sDecoder* decoder, int mb)
{
  assert (mb < (int) decoder->picSizeInMbs);
  assert (decoder->MbToSliceGroupMap != NULL);
  return decoder->MbToSliceGroupMap[mb];
}
//}}}
//{{{
int FmoGetNextMBNr (sDecoder* decoder, int CurrentMbNr)
{
  int SliceGroup = FmoGetSliceGroupId (decoder, CurrentMbNr);

  while (++CurrentMbNr<(int)decoder->picSizeInMbs && decoder->MbToSliceGroupMap [CurrentMbNr] != SliceGroup)
    ;

  if (CurrentMbNr >= (int)decoder->picSizeInMbs)
    return -1;    // No further MB in this slice (could be end of picture)
  else
    return CurrentMbNr;
}
//}}}
