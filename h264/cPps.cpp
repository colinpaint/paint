//{{{  includes
#include "global.h"
#include "memory.h"

#include "nalu.h"

#include "../common/cLog.h"

using namespace std;
//}}}
namespace {
  //{{{
  static void scalingList (cBitStream& s, int* scalingList, int scalingListSize, bool* useDefaultScalingMatrix) {
  // syntax for scaling list matrix values

    //{{{
    static const uint8_t ZZ_SCAN[16] = {
      0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
      };
    //}}}
    //{{{
    static const uint8_t ZZ_SCAN8[64] = {
      0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
      12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
      35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
      58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
      };
    //}}}

    int lastScale = 8;
    int nextScale = 8;
    for (int j = 0; j < scalingListSize; j++) {
      int scanj = (scalingListSize == 16) ? ZZ_SCAN[j] : ZZ_SCAN8[j];
      if (nextScale != 0) {
        int delta_scale = s.readSeV ("   : delta_sl   ");
        nextScale = (lastScale + delta_scale + 256) % 256;
        *useDefaultScalingMatrix = (bool)(scanj == 0 && nextScale == 0);
        }

      scalingList[scanj] = (nextScale == 0) ? lastScale : nextScale;
      lastScale = scalingList[scanj];
      }
    }
  //}}}
  }

// static
//{{{
int cPps::readNalu (cDecoder264* decoder, cNalu* nalu) {

  cPps pps;
  pps.naluLen = nalu->getLength();

  sDataPartition* dataPartition = sDataPartition::allocDataPartitionArray (1);
  dataPartition->bitStream.errorFlag = 0;
  dataPartition->bitStream.readLen = dataPartition->bitStream.bitStreamOffset = 0;
  dataPartition->bitStream.bitStreamLen = nalu->getSodb (dataPartition->bitStream.bitStreamBuffer);
  dataPartition->bitStream.codeLen = dataPartition->bitStream.bitStreamLen;

  pps.readFromStream (decoder, dataPartition);
  sDataPartition::freeDataPartitionArray (dataPartition, 1);

  if (decoder->pps[pps.id].ok)
    if (!pps.isEqual (decoder->pps[pps.id]))
      cLog::log (LOGINFO, fmt::format ("-----> cPps::readNalu new pps id:{}", pps.id));

  // free any sliceGroupId calloc
  if (decoder->pps[pps.id].ok && decoder->pps[pps.id].sliceGroupId)
    free (decoder->pps[pps.id].sliceGroupId);

  // - takes ownership, if any, of pps->sliceGroupId calloc
  memcpy (&decoder->pps[pps.id], &pps, sizeof (cPps));

  return pps.id;
  }
//}}}

//{{{
string cPps::getString() {

  return fmt::format ("PPS:{}:{} -> sps:{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
                      id, naluLen,
                      spsId,
                      entropyCoding ? " cabac":" cavlc",
                      numSliceGroupsMinus1 ? fmt::format(" sliceGroups:{}", numSliceGroupsMinus1):"",
                      sliceGroupMapType ? fmt::format(" sliceGroupMap:{}", sliceGroupMapType):"",
                      numRefIndexL0defaultActiveMinus1
                        ? fmt::format (" short:{}", numRefIndexL0defaultActiveMinus1):"",
                      numRefIndexL1defaultActiveMinus1
                        ? fmt::format (" long:{}", numRefIndexL1defaultActiveMinus1):"",
                      hasDeblockFilterControl ? " deblock":"",
                      hasWeightedPred ? " pred":"",
                      hasConstrainedIntraPred ? " intra":"",
                      redundantPicCountPresent ? " redundant":"",
                      hasMoreData && hasTransform8x8mode ? " 8x8":"",
                      hasMoreData && hasPicScalingMatrix ? " scaling":"",
                      weightedBiPredIdc ? fmt::format(" weightedBiPredIdc:{}", weightedBiPredIdc):"",
                      frameBotField ? " botField":""
                      );
  }
//}}}
//{{{
bool cPps::isEqual (cPps& pps) {

  if (!ok || !pps.ok)
    return false;

  bool equal = true;
  equal &= (id == pps.id);
  equal &= (spsId == pps.spsId);
  equal &= (entropyCoding == pps.entropyCoding);
  equal &= (frameBotField == pps.frameBotField);
  equal &= (numSliceGroupsMinus1 == pps.numSliceGroupsMinus1);
  if (!equal)
    return equal;

  if (numSliceGroupsMinus1 > 0) {
    equal &= (sliceGroupMapType == pps.sliceGroupMapType);
    if (!equal)
      return equal;
    if (sliceGroupMapType == 0) {
      for (uint32_t i = 0; i <= numSliceGroupsMinus1; i++)
        equal &= (runLengthMinus1[i] == pps.runLengthMinus1[i]);
      }
    else if (sliceGroupMapType == 2) {
      for (uint32_t i = 0; i < numSliceGroupsMinus1; i++) {
        equal &= (topLeft[i] == pps.topLeft[i]);
        equal &= (botRight[i] == pps.botRight[i]);
        }
      }
    else if (sliceGroupMapType == 3 ||
             sliceGroupMapType == 4 ||
             sliceGroupMapType == 5) {
      equal &= (sliceGroupChangeDirectionFlag == pps.sliceGroupChangeDirectionFlag);
      equal &= (sliceGroupChangeRateMius1 == pps.sliceGroupChangeRateMius1);
      }
    else if (sliceGroupMapType == 6) {
      equal &= (picSizeMapUnitsMinus1 == pps.picSizeMapUnitsMinus1);
      if (!equal)
        return equal;
      for (uint32_t i = 0; i <= picSizeMapUnitsMinus1; i++)
        equal &= (sliceGroupId[i] == pps.sliceGroupId[i]);
      }
    }

  equal &= (hasWeightedPred == pps.hasWeightedPred);
  equal &= (picInitQpMinus26 == pps.picInitQpMinus26);
  equal &= (picInitQsMinus26 == pps.picInitQsMinus26);
  equal &= (weightedBiPredIdc == pps.weightedBiPredIdc);
  equal &= (chromaQpOffset == pps.chromaQpOffset);
  equal &= (hasConstrainedIntraPred == pps.hasConstrainedIntraPred);
  equal &= (redundantPicCountPresent == pps.redundantPicCountPresent);
  equal &= (hasDeblockFilterControl == pps.hasDeblockFilterControl);
  equal &= (numRefIndexL0defaultActiveMinus1 == pps.numRefIndexL0defaultActiveMinus1);
  equal &= (numRefIndexL1defaultActiveMinus1 == pps.numRefIndexL1defaultActiveMinus1);
  if (!equal)
    return equal;

  equal &= (hasTransform8x8mode == pps.hasTransform8x8mode);
  equal &= (hasPicScalingMatrix == pps.hasPicScalingMatrix);
  if (hasPicScalingMatrix) {
    for (uint32_t i = 0; i < (6 + ((uint32_t)hasTransform8x8mode << 1)); i++) {
      equal &= (picScalingListPresentFlag[i] == pps.picScalingListPresentFlag[i]);
      if (picScalingListPresentFlag[i]) {
        if (i < 6)
          for (int j = 0; j < 16; j++)
            equal &= (scalingList4x4[i][j] == pps.scalingList4x4[i][j]);
        else
          for (int j = 0; j < 64; j++)
            equal &= (scalingList8x8[i-6][j] == pps.scalingList8x8[i-6][j]);
        }
      }
    }
  equal &= (chromaQpOffset2 == pps.chromaQpOffset2);

  return equal;
  }
//}}}

// private
//{{{
void cPps::readFromStream (cDecoder264* decoder, sDataPartition* dataPartition) {
// read PPS from NALU

  cBitStream& s = dataPartition->bitStream;

  id = s.readUeV ("PPS ppsId");
  spsId = s.readUeV ("PPS spsId");

  entropyCoding = (eEntropyCodingType)s.readU1 ("PPS entropyCoding");
  frameBotField = s.readU1 ("PPS frameBotField");
  numSliceGroupsMinus1 = s.readUeV ("PPS numSliceGroupsMinus1");

  if (numSliceGroupsMinus1 > 0) {
    //{{{  FMO
    sliceGroupMapType = s.readUeV ("PPS sliceGroupMapType");

    switch (sliceGroupMapType) {
      case 0: {
        for (uint32_t i = 0; i <= numSliceGroupsMinus1; i++)
          runLengthMinus1 [i] = s.readUeV ("PPS runLengthMinus1 [i]");
        break;
        }

      case 2: {
        for (uint32_t i = 0; i < numSliceGroupsMinus1; i++) {
          topLeft [i] = s.readUeV ("PPS topLeft [i]");
          botRight [i] = s.readUeV ("PPS botRight [i]");
          }
        break;
        }

      case 3:
      case 4:
      case 5:
        sliceGroupChangeDirectionFlag = s.readU1 ("PPS sliceGroupChangeDirectionFlag");
        sliceGroupChangeRateMius1 = s.readUeV ("PPS sliceGroupChangeRateMius1");
        break;

      case 6: {
        int NumberBitsPerSliceGroupId;
        if (numSliceGroupsMinus1+1 >4)
          NumberBitsPerSliceGroupId = 3;
        else if (numSliceGroupsMinus1+1 > 2)
          NumberBitsPerSliceGroupId = 2;
        else
          NumberBitsPerSliceGroupId = 1;

        picSizeMapUnitsMinus1 = s.readUeV ("PPS picSizeMapUnitsMinus1");
        sliceGroupId = (uint8_t*)calloc (picSizeMapUnitsMinus1+1, 1);
        for (uint32_t i = 0; i <= picSizeMapUnitsMinus1; i++)
          sliceGroupId[i] = (uint8_t)s.readUv (NumberBitsPerSliceGroupId, "sliceGroupId[i]");
        break;
        }

      default:;
      }
    }
    //}}}

  numRefIndexL0defaultActiveMinus1 = s.readUeV ("PPS numRefIndexL0defaultActiveMinus1");
  numRefIndexL1defaultActiveMinus1 = s.readUeV ("PPS numRefIndexL1defaultActiveMinus1");

  hasWeightedPred = s.readU1 ("PPS hasWeightedPred");
  weightedBiPredIdc = s.readUv (2, "PPS weightedBiPredIdc");

  picInitQpMinus26 = s.readSeV ("PPS picInitQpMinus26");
  picInitQsMinus26 = s.readSeV ("PPS picInitQsMinus26");

  chromaQpOffset = s.readSeV ("PPS chromaQpOffset");

  hasDeblockFilterControl = s.readU1 ("PPS hasDeblockFilterControl" );
  hasConstrainedIntraPred = s.readU1 ("PPS hasConstrainedIntraPred");
  redundantPicCountPresent = s.readU1 ("PPS redundantPicCountPresent");

  hasMoreData = cBitStream::moreRbspData (s.bitStreamBuffer, s.bitStreamOffset,s.bitStreamLen);
  if (hasMoreData) {
    //{{{  read fidelity range
    hasTransform8x8mode = s.readU1 ("PPS hasTransform8x8mode");

    hasPicScalingMatrix = s.readU1 ("PPS hasPicScalingMatrix");
    if (hasPicScalingMatrix) {
      int chromaFormatIdc = decoder->sps[spsId].chromaFormatIdc;
      uint32_t n_ScalingList = 6 + ((chromaFormatIdc != YUV444) ? 2 : 6) * hasTransform8x8mode;
      for (uint32_t i = 0; i < n_ScalingList; i++) {
        picScalingListPresentFlag[i]= s.readU1 ("PPS picScalingListPresentFlag");
        if (picScalingListPresentFlag[i]) {
          if (i < 6)
            scalingList (s, scalingList4x4[i], 16, &useDefaultScalingMatrix4x4Flag[i]);
          else
            scalingList (s, scalingList8x8[i-6], 64, &useDefaultScalingMatrix8x8Flag[i-6]);
          }
        }
      }

    chromaQpOffset2 = s.readSeV ("PPS chromaQpOffset2");
    }
    //}}}
  else
    chromaQpOffset2 = chromaQpOffset;

  ok = true;
  }
//}}}
