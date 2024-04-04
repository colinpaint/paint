//{{{  includes
#include "global.h"
#include "syntaxElement.h"
#include "fmo.h"
//}}}
//#define PRINT_FMO_MAPS

//{{{
static void FmoGenerateType0MapUnitMap (cDecoder264* decoder, uint32_t PicSizeInMapUnits ) {
// Generate interleaved slice group map type MapUnit map (type 0)

  cPps* pps = decoder->activePps;
  uint32_t iGroup, j;
  uint32_t i = 0;
  do {
    for (iGroup = 0;
         (iGroup <= pps->numSliceGroupsMinus1) && (i < PicSizeInMapUnits);
         i += pps->runLengthMinus1[iGroup++] + 1 )
      for( j = 0; j <= pps->runLengthMinus1[ iGroup ] && i + j < PicSizeInMapUnits; j++ )
        decoder->mapUnitToSliceGroupMap[i+j] = iGroup;
    } while( i < PicSizeInMapUnits );
  }
//}}}
//{{{
// Generate dispersed slice group map type MapUnit map (type 1)
static void FmoGenerateType1MapUnitMap (cDecoder264* decoder, uint32_t PicSizeInMapUnits ) {

  cPps* pps = decoder->activePps;
  uint32_t i;
  for( i = 0; i < PicSizeInMapUnits; i++ )
    decoder->mapUnitToSliceGroupMap[i] = 
      ((i%decoder->coding.picWidthMbs) + (((i/decoder->coding.picWidthMbs)*(pps->numSliceGroupsMinus1+1))/2))
      % (pps->numSliceGroupsMinus1+1);
  }
//}}}
//{{{
// Generate foreground with left-over slice group map type MapUnit map (type 2)
static void FmoGenerateType2MapUnitMap (cDecoder264* decoder, uint32_t PicSizeInMapUnits ) {

  cPps* pps = decoder->activePps;
  int iGroup;
  uint32_t i, x, y;
  uint32_t yTopLeft, xTopLeft, yBottomRight, xBottomRight;

  for (i = 0; i < PicSizeInMapUnits; i++ )
    decoder->mapUnitToSliceGroupMap[ i ] = pps->numSliceGroupsMinus1;

  for (iGroup = pps->numSliceGroupsMinus1 - 1 ; iGroup >= 0; iGroup--) {
    yTopLeft = pps->topLeft[ iGroup ] / decoder->coding.picWidthMbs;
    xTopLeft = pps->topLeft[ iGroup ] % decoder->coding.picWidthMbs;
    yBottomRight = pps->botRight[ iGroup ] / decoder->coding.picWidthMbs;
    xBottomRight = pps->botRight[ iGroup ] % decoder->coding.picWidthMbs;
    for (y = yTopLeft; y <= yBottomRight; y++ )
      for (x = xTopLeft; x <= xBottomRight; x++ )
        decoder->mapUnitToSliceGroupMap[ y * decoder->coding.picWidthMbs + x ] = iGroup;
    }
  }
//}}}
//{{{
// Generate box-out slice group map type MapUnit map (type 3)
static void FmoGenerateType3MapUnitMap (cDecoder264* decoder, uint32_t PicSizeInMapUnits, sSlice* slice) {

  cPps* pps = decoder->activePps;
  uint32_t i, k;
  int leftBound, topBound, rightBound, bottomBound;
  int x, y, xDir, yDir;
  int mapUnitVacant;

  uint32_t mapUnitsInSliceGroup0 = imin((pps->sliceGroupChangeRateMius1 + 1) * slice->sliceGroupChangeCycle, PicSizeInMapUnits);

  for (i = 0; i < PicSizeInMapUnits; i++ )
    decoder->mapUnitToSliceGroupMap[i] = 2;

  x = (decoder->coding.picWidthMbs - pps->sliceGroupChangeDirectionFlag) / 2;
  y = (decoder->coding.picHeightMapUnits - pps->sliceGroupChangeDirectionFlag) / 2;

  leftBound = x;
  topBound = y;
  rightBound  = x;
  bottomBound = y;

  xDir = pps->sliceGroupChangeDirectionFlag - 1;
  yDir = pps->sliceGroupChangeDirectionFlag;

  for( k = 0; k < PicSizeInMapUnits; k += mapUnitVacant ) {
    mapUnitVacant = ( decoder->mapUnitToSliceGroupMap[ y * decoder->coding.picWidthMbs + x ]  ==  2 );
    if( mapUnitVacant )
       decoder->mapUnitToSliceGroupMap[ y * decoder->coding.picWidthMbs + x ] = ( k >= mapUnitsInSliceGroup0 );

    if( xDir  ==  -1  &&  x  ==  leftBound ) {
      leftBound = imax( leftBound - 1, 0 );
      x = leftBound;
      xDir = 0;
      yDir = 2 * pps->sliceGroupChangeDirectionFlag - 1;
      }
    else if (xDir  ==  1  &&  x  ==  rightBound ) {
      rightBound = imin( rightBound + 1, (int)decoder->coding.picWidthMbs - 1 );
      x = rightBound;
      xDir = 0;
      yDir = 1 - 2 * pps->sliceGroupChangeDirectionFlag;
      }
    else if (yDir  ==  -1  &&  y  ==  topBound ) {
      topBound = imax( topBound - 1, 0 );
      y = topBound;
      xDir = 1 - 2 * pps->sliceGroupChangeDirectionFlag;
      yDir = 0;
      }
    else if (yDir  ==  1  &&  y  ==  bottomBound ) {
      bottomBound = imin( bottomBound + 1, (int)decoder->coding.picHeightMapUnits - 1 );
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
static void FmoGenerateType4MapUnitMap (cDecoder264* decoder, uint32_t PicSizeInMapUnits, sSlice* slice) {
// Generate raster scan slice group map type MapUnit map (type 4)

  cPps* pps = decoder->activePps;

  uint32_t mapUnitsInSliceGroup0 = imin((pps->sliceGroupChangeRateMius1 + 1) * slice->sliceGroupChangeCycle, PicSizeInMapUnits);
  uint32_t sizeOfUpperLeftGroup = pps->sliceGroupChangeDirectionFlag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  uint32_t i;

  for (i = 0; i < PicSizeInMapUnits; i++ )
    if (i < sizeOfUpperLeftGroup )
      decoder->mapUnitToSliceGroupMap[ i ] = pps->sliceGroupChangeDirectionFlag;
    else
      decoder->mapUnitToSliceGroupMap[ i ] = 1 - pps->sliceGroupChangeDirectionFlag;
  }
//}}}
//{{{
// Generate wipe slice group map type MapUnit map (type 5) *
static void FmoGenerateType5MapUnitMap (cDecoder264* decoder, uint32_t PicSizeInMapUnits, sSlice* slice ) {

  cPps* pps = decoder->activePps;

  uint32_t mapUnitsInSliceGroup0 = 
    imin((pps->sliceGroupChangeRateMius1 + 1) * slice->sliceGroupChangeCycle, PicSizeInMapUnits);
  uint32_t sizeOfUpperLeftGroup = 
    pps->sliceGroupChangeDirectionFlag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  uint32_t i,j, k = 0;

  for( j = 0; j < decoder->coding.picWidthMbs; j++ )
    for( i = 0; i < decoder->coding.picHeightMapUnits; i++ )
        if( k++ < sizeOfUpperLeftGroup )
            decoder->mapUnitToSliceGroupMap[ i * decoder->coding.picWidthMbs + j ] = pps->sliceGroupChangeDirectionFlag;
        else
            decoder->mapUnitToSliceGroupMap[ i * decoder->coding.picWidthMbs + j ] = 1 - pps->sliceGroupChangeDirectionFlag;

  }
//}}}
//{{{
static void FmoGenerateType6MapUnitMap (cDecoder264* decoder, uint32_t PicSizeInMapUnits ) {
// Generate explicit slice group map type MapUnit map (type 6)

  cPps* pps = decoder->activePps;
  uint32_t i;
  for (i = 0; i < PicSizeInMapUnits; i++)
    decoder->mapUnitToSliceGroupMap[i] = pps->sliceGroupId[i];
  }
//}}}
//{{{
static int FmoGenerateMapUnitToSliceGroupMap (cDecoder264* decoder, sSlice* slice) {
// Generates decoder->mapUnitToSliceGroupMap
// Has to be called every time a new Picture Parameter Set is used

  cSps* sps = decoder->activeSps;
  cPps* pps = decoder->activePps;

  uint32_t NumSliceGroupMapUnits;

  NumSliceGroupMapUnits = (sps->picHeightMapUnitsMinus1+1)* (sps->picWidthMbsMinus1+1);

  if (pps->sliceGroupMapType == 6)
    if ((pps->picSizeMapUnitsMinus1 + 1) != NumSliceGroupMapUnits)
      cDecoder264::error ("wrong pps->picSizeMapUnitsMinus1 for used SPS and FMO type 6");

  // allocate memory for decoder->mapUnitToSliceGroupMap
  if (decoder->mapUnitToSliceGroupMap)
    free (decoder->mapUnitToSliceGroupMap);

  if ((decoder->mapUnitToSliceGroupMap = (int*)malloc ((NumSliceGroupMapUnits) * sizeof (int))) == NULL) {
    printf ("cannot allocated %d bytes for decoder->mapUnitToSliceGroupMap, exit\n", (int) ( (pps->picSizeMapUnitsMinus1+1) * sizeof (int)));
    exit (-1);
    }

  if (pps->numSliceGroupsMinus1 == 0) {
    // only one slice group
    memset (decoder->mapUnitToSliceGroupMap, 0, NumSliceGroupMapUnits * sizeof (int));
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
      FmoGenerateType3MapUnitMap (decoder, NumSliceGroupMapUnits, slice);
      break;
    case 4:
      FmoGenerateType4MapUnitMap (decoder, NumSliceGroupMapUnits, slice);
      break;
    case 5:
      FmoGenerateType5MapUnitMap (decoder, NumSliceGroupMapUnits, slice);
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
// Generates decoder->mbToSliceGroupMap from decoder->mapUnitToSliceGroupMap
static int FmoGenerateMbToSliceGroupMap (cDecoder264* decoder, sSlice *slice) {

  // allocate memory for decoder->mbToSliceGroupMap
  if (decoder->mbToSliceGroupMap)
    free (decoder->mbToSliceGroupMap);

  if ((decoder->mbToSliceGroupMap = (int*)malloc ((decoder->picSizeInMbs) * sizeof (int))) == NULL) {
    printf ("cannot allocate %d bytes for decoder->mbToSliceGroupMap, exit\n",
            (int) ((decoder->picSizeInMbs) * sizeof (int)));
    exit (-1);
    }

  cSps* sps = decoder->activeSps;
  if ((sps->frameMbOnly)|| slice->fieldPic) {
    int *mbToSliceGroupMap = decoder->mbToSliceGroupMap;
    int *mapUnitToSliceGroupMap = decoder->mapUnitToSliceGroupMap;
    for (uint32_t i = 0; i < decoder->picSizeInMbs; i++)
      *mbToSliceGroupMap++ = *mapUnitToSliceGroupMap++;
    }
  else
    if (sps->mbAffFlag  &&  (!slice->fieldPic)) {
      for (uint32_t i = 0; i < decoder->picSizeInMbs; i++)
        decoder->mbToSliceGroupMap[i] = decoder->mapUnitToSliceGroupMap[i/2];
      }
    else {
      for (uint32_t i = 0; i < decoder->picSizeInMbs; i++)
        decoder->mbToSliceGroupMap[i] = decoder->mapUnitToSliceGroupMap[(i/(2*decoder->coding.picWidthMbs)) *
                                        decoder->coding.picWidthMbs + (i%decoder->coding.picWidthMbs)];
      }

  return 0;
  }
//}}}

//{{{
int initFmo (cDecoder264* decoder, sSlice* slice) {

  cPps* pps = decoder->activePps;

  FmoGenerateMapUnitToSliceGroupMap (decoder, slice);
  FmoGenerateMbToSliceGroupMap (decoder, slice);
  decoder->sliceGroupsNum = pps->numSliceGroupsMinus1 + 1;

#ifdef PRINT_FMO_MAPS
  printf ("\n");
  printf ("FMO Map (Units):\n");

  for (int j = 0; j<decoder->picHeightMapUnits; j++) {
    for (int i = 0; i<decoder->picWidthMbs; i++) {
      printf("%c",48+decoder->mapUnitToSliceGroupMap[i+j*decoder->picWidthMbs]);
      }
    printf("\n");
    }

  printf("\n");
  printf("FMO Map (Mb):\n");

  for (int j = 0; j < decoder->picHeightInMbs; j++) {
    for (int i = 0; i < decoder->picWidthMbs; i++) {
      printf ("%c", 48 + decoder->mbToSliceGroupMap[i + j * decoder->picWidthMbs]);
      }
    printf ("\n");
    }
  printf ("\n");

#endif

  return 0;
  }
//}}}

//{{{
int closeFmo (cDecoder264* decoder) {

  if (decoder->mbToSliceGroupMap) {
    free (decoder->mbToSliceGroupMap);
    decoder->mbToSliceGroupMap = NULL;
    }

  if (decoder->mapUnitToSliceGroupMap) {
    free (decoder->mapUnitToSliceGroupMap);
    decoder->mapUnitToSliceGroupMap = NULL;
    }

  return 0;
  }
//}}}
//{{{
int FmoGetNumberOfSliceGroup (cDecoder264* decoder) {
  return decoder->sliceGroupsNum;
  }
//}}}
//{{{
int FmoGetLastMBOfPicture (cDecoder264* decoder) {
  return FmoGetLastMBInSliceGroup (decoder, FmoGetNumberOfSliceGroup(decoder)-1);
  }
//}}}
//{{{
int FmoGetLastMBInSliceGroup (cDecoder264* decoder, int SliceGroup) {

  int i;
  for (i = decoder->picSizeInMbs-1; i >= 0; i--)
    if (FmoGetSliceGroupId (decoder, i) == SliceGroup)
      return i;
  return -1;
  }
//}}}
//{{{
int FmoGetSliceGroupId (cDecoder264* decoder, int mb) {

  return decoder->mbToSliceGroupMap[mb];
  }
//}}}
//{{{
int FmoGetNextMBNr (cDecoder264* decoder, int CurrentMbNr) {

  int SliceGroup = FmoGetSliceGroupId (decoder, CurrentMbNr);

  while (++CurrentMbNr<(int)decoder->picSizeInMbs && 
         decoder->mbToSliceGroupMap [CurrentMbNr] != SliceGroup) ;

  if (CurrentMbNr >= (int)decoder->picSizeInMbs)
    return -1;    // No further MB in this slice (could be end of picture)
  else
    return CurrentMbNr;
  }
//}}}
