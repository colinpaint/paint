#pragma once
#include "global.h"
#include "nalu.h"

#define MAX_PPS  256

// sPps
struct sPps{
  bool   ok;
  int       naluLen;

  uint32_t  id;                               // ue(v)
  uint32_t  spsId;                            // ue(v)
  int       entropyCoding;                    // u(1)
  bool   frameBotField;                    // u(1)

  uint32_t numSliceGroupsMinus1;          // ue(v)
  uint32_t sliceGroupMapType;             // ue(v)
  //{{{  optional sliceGroupMapType fields
  // sliceGroupMapType 0
  uint32_t runLengthMinus1[8];            // ue(v)
  // sliceGroupMapType 2
  uint32_t topLeft[8];                    // ue(v)
  uint32_t botRight[8];                   // ue(v)
  // sliceGroupMapType 3 || 4 || 5
  bool   sliceGroupChangeDirectionFlag;    // u(1)
  uint32_t sliceGroupChangeRateMius1;     // ue(v)
  // sliceGroupMapType 6
  uint32_t picSizeMapUnitsMinus1;         // ue(v)
  byte*     sliceGroupId;                     // complete MBAmap u(v)
  //}}}

  int       numRefIndexL0defaultActiveMinus1; // ue(v)
  int       numRefIndexL1defaultActiveMinus1; // ue(v)

  bool   hasWeightedPred;                  // u(1)
  uint32_t  weightedBiPredIdc;                // u(2)

  int       picInitQpMinus26;                 // se(v)
  int       picInitQsMinus26;                 // se(v)
  int       chromaQpOffset;                   // se(v)

  bool   hasDeblockFilterControl;          // u(1)
  bool   hasConstrainedIntraPred;          // u(1)
  bool   redundantPicCountPresent;         // u(1)

  bool   hasMoreData;
  bool   hasTransform8x8mode;              // u(1)

  bool   hasPicScalingMatrix;              // u(1)
  //{{{  optional scalingMatrix fields
  int       picScalingListPresentFlag[12];  // u(1)
  int       scalingList4x4[6][16];          // se(v)
  int       scalingList8x8[6][64];          // se(v)

  bool   useDefaultScalingMatrix4x4Flag[6];
  bool   useDefaultScalingMatrix8x8Flag[6];
  //}}}

  int       chromaQpOffset2;                  // se(v)
  };

extern int readNaluPps (sDecoder* decoder, sNalu* nalu);
extern void getPpsStr (sPps* pps, char* str);
