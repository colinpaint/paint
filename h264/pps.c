//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
#include "pps.h"
#include "nalu.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "buffer.h"
#include "erc.h"
//}}}

//{{{
// syntax for scaling list matrix values
static void scalingList (int* scalingList, int scalingListSize, Boolean* useDefaultScalingMatrix, sBitStream* s) {

  //{{{
  static const byte ZZ_SCAN[16] = {
    0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
    };
  //}}}
  //{{{
  static const byte ZZ_SCAN8[64] = {
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
      int delta_scale = readSeV ("   : delta_sl   ", s);
      nextScale = (lastScale + delta_scale + 256) % 256;
      *useDefaultScalingMatrix = (Boolean)(scanj == 0 && nextScale == 0);
      }

    scalingList[scanj] = (nextScale == 0) ? lastScale : nextScale;
    lastScale = scalingList[scanj];
    }
  }
//}}}
//{{{
static int isEqualPps (sPps* pps1, sPps* pps2) {

  if (!pps1->valid || !pps2->valid)
    return 0;

  int equal = 1;
  equal &= (pps1->id == pps2->id);
  equal &= (pps1->spsId == pps2->spsId);
  equal &= (pps1->entropyCoding == pps2->entropyCoding);
  equal &= (pps1->botFieldFrame == pps2->botFieldFrame);
  equal &= (pps1->numSliceGroupsMinus1 == pps2->numSliceGroupsMinus1);
  if (!equal)
    return equal;

  if (pps1->numSliceGroupsMinus1 > 0) {
    equal &= (pps1->sliceGroupMapType == pps2->sliceGroupMapType);
    if (!equal)
      return equal;
    if (pps1->sliceGroupMapType == 0) {
      for (unsigned i = 0; i <= pps1->numSliceGroupsMinus1; i++)
        equal &= (pps1->runLengthMinus1[i] == pps2->runLengthMinus1[i]);
      }
    else if (pps1->sliceGroupMapType == 2) {
      for (unsigned i = 0; i < pps1->numSliceGroupsMinus1; i++) {
        equal &= (pps1->topLeft[i] == pps2->topLeft[i]);
        equal &= (pps1->botRight[i] == pps2->botRight[i]);
        }
      }
    else if (pps1->sliceGroupMapType == 3 ||
             pps1->sliceGroupMapType == 4 ||
             pps1->sliceGroupMapType == 5) {
      equal &= (pps1->sliceGroupChangeDirectionFlag == pps2->sliceGroupChangeDirectionFlag);
      equal &= (pps1->sliceGroupChangeRateMius1 == pps2->sliceGroupChangeRateMius1);
      }
    else if (pps1->sliceGroupMapType == 6) {
      equal &= (pps1->picSizeMapUnitsMinus1 == pps2->picSizeMapUnitsMinus1);
      if (!equal)
        return equal;
      for (unsigned i = 0; i <= pps1->picSizeMapUnitsMinus1; i++)
        equal &= (pps1->sliceGroupId[i] == pps2->sliceGroupId[i]);
      }
    }

  equal &= (pps1->hasWeightedPred == pps2->hasWeightedPred);
  equal &= (pps1->picInitQpMinus26 == pps2->picInitQpMinus26);
  equal &= (pps1->picInitQsMinus26 == pps2->picInitQsMinus26);
  equal &= (pps1->weightedBiPredIdc == pps2->weightedBiPredIdc);
  equal &= (pps1->chromaQpOffset == pps2->chromaQpOffset);
  equal &= (pps1->hasConstrainedIntraPred == pps2->hasConstrainedIntraPred);
  equal &= (pps1->redundantPicCountPresent == pps2->redundantPicCountPresent);
  equal &= (pps1->hasDeblockFilterControl == pps2->hasDeblockFilterControl);
  equal &= (pps1->numRefIndexL0defaultActiveMinus1 == pps2->numRefIndexL0defaultActiveMinus1);
  equal &= (pps1->numRefIndexL1defaultActiveMinus1 == pps2->numRefIndexL1defaultActiveMinus1);
  if (!equal)
    return equal;

  equal &= (pps1->hasTransform8x8mode == pps2->hasTransform8x8mode);
  equal &= (pps1->hasPicScalingMatrix == pps2->hasPicScalingMatrix);
  if (pps1->hasPicScalingMatrix) {
    for (unsigned i = 0; i < (6 + ((unsigned)pps1->hasTransform8x8mode << 1)); i++) {
      equal &= (pps1->picScalingListPresentFlag[i] == pps2->picScalingListPresentFlag[i]);
      if (pps1->picScalingListPresentFlag[i]) {
        if (i < 6)
          for (int j = 0; j < 16; j++)
            equal &= (pps1->scalingList4x4[i][j] == pps2->scalingList4x4[i][j]);
        else
          for (int j = 0; j < 64; j++)
            equal &= (pps1->scalingList8x8[i-6][j] == pps2->scalingList8x8[i-6][j]);
        }
      }
    }
  equal &= (pps1->chromaQpOffset2 == pps2->chromaQpOffset2);

  return equal;
  }
//}}}
//{{{
static void readPps (sDecoder* decoder, sDataPartition* dataPartition, sPps* pps, int naluLen) {
// read PPS from NALU

  sBitStream* s = dataPartition->s;

  pps->id = readUeV ("PPS id", s);
  pps->spsId = readUeV ("PPS spsId", s);

  pps->entropyCoding = readU1 ("PPS entropyCoding", s);
  pps->botFieldFrame = readU1 ("PPS botFieldFrame", s);
  pps->numSliceGroupsMinus1 = readUeV ("PPS numSliceGroupsMinus1", s);

  if (pps->numSliceGroupsMinus1 > 0) {
    //{{{  FMO
    pps->sliceGroupMapType = readUeV ("PPS sliceGroupMapType", s);

    switch (pps->sliceGroupMapType) {
      case 0: {
        for (unsigned i = 0; i <= pps->numSliceGroupsMinus1; i++)
          pps->runLengthMinus1 [i] = readUeV ("PPS runLengthMinus1 [i]", s);
        break;
        }

      case 2: {
        for (unsigned i = 0; i < pps->numSliceGroupsMinus1; i++) {
          pps->topLeft [i] = readUeV ("PPS topLeft [i]", s);
          pps->botRight [i] = readUeV ("PPS botRight [i]", s);
          }
        break;
        }

      case 3:
      case 4:
      case 5:
        pps->sliceGroupChangeDirectionFlag = readU1 ("PPS sliceGroupChangeDirectionFlag", s);
        pps->sliceGroupChangeRateMius1 = readUeV ("PPS sliceGroupChangeRateMius1", s);
        break;

      case 6: {
        int NumberBitsPerSliceGroupId;
        if (pps->numSliceGroupsMinus1+1 >4)
          NumberBitsPerSliceGroupId = 3;
        else if (pps->numSliceGroupsMinus1+1 > 2)
          NumberBitsPerSliceGroupId = 2;
        else
          NumberBitsPerSliceGroupId = 1;

        pps->picSizeMapUnitsMinus1 = readUeV ("PPS picSizeMapUnitsMinus1", s);
        pps->sliceGroupId = calloc (pps->picSizeMapUnitsMinus1+1, 1);
        for (unsigned i = 0; i <= pps->picSizeMapUnitsMinus1; i++)
          pps->sliceGroupId[i] = (byte)readUv (NumberBitsPerSliceGroupId, "sliceGroupId[i]", s);
        break;
        }

      default:;
      }
    }
    //}}}

  pps->numRefIndexL0defaultActiveMinus1 = readUeV ("PPS numRefIndexL0defaultActiveMinus1", s);
  pps->numRefIndexL1defaultActiveMinus1 = readUeV ("PPS numRefIndexL1defaultActiveMinus1", s);

  pps->hasWeightedPred = readU1 ("PPS hasWeightedPred", s);
  pps->weightedBiPredIdc = readUv (2, "PPS weightedBiPredIdc", s);

  pps->picInitQpMinus26 = readSeV ("PPS picInitQpMinus26", s);
  pps->picInitQsMinus26 = readSeV ("PPS picInitQsMinus26", s);

  pps->chromaQpOffset = readSeV ("PPS chromaQpOffset", s);

  pps->hasDeblockFilterControl = readU1 ("PPS hasDeblockFilterControl" , s);
  pps->hasConstrainedIntraPred = readU1 ("PPS hasConstrainedIntraPred", s);
  pps->redundantPicCountPresent = readU1 ("PPS redundantPicCountPresent", s);

  int fidelityRange = moreRbspData (s->bitStreamBuffer, s->bitStreamOffset,s->bitStreamLen);
  if (fidelityRange) {
    pps->hasTransform8x8mode = readU1 ("PPS hasTransform8x8mode", s);
    pps->hasPicScalingMatrix = readU1 ("PPS hasPicScalingMatrix", s);
    if (pps->hasPicScalingMatrix) {
      int chromaFormatIdc = decoder->sps[pps->spsId].chromaFormatIdc;
      unsigned n_ScalingList = 6 + ((chromaFormatIdc != YUV444) ? 2 : 6) * pps->hasTransform8x8mode;
      for (unsigned i = 0; i < n_ScalingList; i++) {
        pps->picScalingListPresentFlag[i]= readU1 ("PPS picScalingListPresentFlag", s);
        if (pps->picScalingListPresentFlag[i]) {
          if (i < 6)
            scalingList (pps->scalingList4x4[i], 16, &pps->useDefaultScalingMatrix4x4Flag[i], s);
          else
            scalingList (pps->scalingList8x8[i-6], 64, &pps->useDefaultScalingMatrix8x8Flag[i-6], s);
          }
        }
      }
    pps->chromaQpOffset2 = readSeV ("PPS chromaQpOffset2", s);
    }
  else
    pps->chromaQpOffset2 = pps->chromaQpOffset;

  if (decoder->param.ppsDebug)
    //{{{  print debug
    printf ("PPS:%d:%d -> sps:%d%s%s sliceGroups:%d L:%d:%d%s%s%s%s%s%s biPredIdc:%d\n",
            pps->id, naluLen,
            pps->spsId,
            pps->entropyCoding ? " cabac":" cavlc",
            pps->botFieldFrame ? " botField":"",
            pps->numSliceGroupsMinus1,
            pps->numRefIndexL0defaultActiveMinus1, pps->numRefIndexL1defaultActiveMinus1,
            pps->hasDeblockFilterControl ? " deblock":"",
            pps->hasWeightedPred ? " pred":"",
            pps->hasConstrainedIntraPred ? " intra":"",
            pps->redundantPicCountPresent ? " redundant":"",
            fidelityRange && pps->hasTransform8x8mode ? " 8x8":"",
            fidelityRange && pps->hasPicScalingMatrix ? " scaling":"",
            pps->weightedBiPredIdc
            );
    //}}}

  pps->valid = TRUE;
  }
//}}}

//{{{
void setPpsById (sDecoder* decoder, int id, sPps* pps) {

  if (decoder->pps[id].valid && decoder->pps[id].sliceGroupId)
    free (decoder->pps[id].sliceGroupId);
  memcpy (&decoder->pps[id], pps, sizeof (sPps));

  // take ownership of pps->sliceGroupId, set to NULL to stop free by caller
  decoder->pps[id].sliceGroupId = pps->sliceGroupId;
  pps->sliceGroupId = NULL;
  }
//}}}

//{{{
void usePps (sDecoder* decoder, sPps* pps) {

  if (decoder->activePps != pps) {
    if (decoder->picture) // only on slice loss
      endDecodeFrame (decoder);
    decoder->activePps = pps;
    }
  }
//}}}
//{{{
void readPpsFromNalu (sDecoder* decoder, sNalu* nalu) {

  // set up dataPartiton
  sDataPartition* dataPartition = allocDataPartitions (1);
  dataPartition->s->errorFlag = 0;
  dataPartition->s->readLen = dataPartition->s->bitStreamOffset = 0;
  memcpy (dataPartition->s->bitStreamBuffer, &nalu->buf[1], nalu->len - 1);
  dataPartition->s->bitStreamLen = RBSPtoSODB (dataPartition->s->bitStreamBuffer, nalu->len-1);
  dataPartition->s->codeLen = dataPartition->s->bitStreamLen;

  // read pps from Nalu
  sPps pps = { 0 };
  readPps (decoder, dataPartition, &pps, nalu->len);
  freeDataPartitions (dataPartition, 1);

  if (decoder->activePps)
    if (!isEqualPps (&pps, decoder->activePps)) {
      // copy to next PPS;
      memcpy (decoder->nextPps, decoder->activePps, sizeof (sPps));
      if (decoder->picture)
        endDecodeFrame (decoder);
      decoder->activePps = NULL;
      }

  setPpsById (decoder, pps.id, &pps);
  if (!pps.sliceGroupId)
    free (pps.sliceGroupId);
  }
//}}}
