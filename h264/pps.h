#pragma once
#include "global.h"
#include "nalu.h"

#define MAX_PPS  256

// sPps
typedef struct {
  Boolean   valid;

  unsigned  ppsId;                            // ue(v)
  unsigned  spsId;                            // ue(v)
  int       entropyCoding;                    // u(1)
  Boolean   botFieldFrame;                    // u(1)

  unsigned int numSliceGroupsMinus1;          // ue(v)
  unsigned int sliceGroupMapType;             // ue(v)
    // sliceGroupMapType 0
    unsigned int runLengthMinus1[8];            // ue(v)
    // sliceGroupMapType 2
    unsigned int topLeft[8];                    // ue(v)
    unsigned int botRight[8];                   // ue(v)
    // sliceGroupMapType 3 || 4 || 5
    Boolean   sliceGroupChangeDirectionFlag;    // u(1)
    unsigned int sliceGroupChangeRateMius1;     // ue(v)
    // sliceGroupMapType 6
    unsigned int picSizeMapUnitsMinus1;         // ue(v)
    byte*     sliceGroupId;                     // complete MBAmap u(v)

  int       numRefIndexL0defaultActiveMinus1; // ue(v)
  int       numRefIndexL1defaultActiveMinus1; // ue(v)

  Boolean   hasWeightedPred;                  // u(1)
  unsigned  weightedBiPredIdc;                // u(2)

  int       picInitQpMinus26;                 // se(v)
  int       picInitQsMinus26;                 // se(v)
  int       chromaQpOffset;                   // se(v)

  Boolean   hasDeblockFilterControl;          // u(1)
  Boolean   hasConstrainedIntraPred;          // u(1)
  Boolean   redundantPicCountPresent;         // u(1)

  Boolean   hasTransform8x8mode;              // u(1)

  Boolean   hasPicScalingMatrix;              // u(1)
    int       picScalingListPresentFlag[12];    // u(1)
    int       scalingList4x4[6][16];            // se(v)
    int       scalingList8x8[6][64];            // se(v)
    Boolean   useDefaultScalingMatrix4x4Flag[6];
    Boolean   useDefaultScalingMatrix8x8Flag[6];

  int       chromaQpOffset2;                  // se(v)
  } sPps;

struct Decoder;

extern sPps* allocPps();
extern void freePps (sPps* pps);

extern void setPpsById (struct Decoder* decoder, int id, sPps* pps);
extern void readPpsFromNalu (struct Decoder* decoder, sNalu* nalu);
extern void usePps (struct Decoder* decoder, sPps* pps);
