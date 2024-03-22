#pragma once
#include "defines.h"
#include "nalu.h"

#define MAX_PPS  256

// sPPS
typedef struct {
  Boolean   valid;

  unsigned int ppsId;                         // ue(v)
  unsigned int spsId;                         // ue(v)
  Boolean   entropyCodingMode;                // u(1)
  Boolean   transform8x8modeFlag;             // u(1)

  Boolean   picScalingMatrixPresentFlag;      // u(1)
  int       picScalingListPresentFlag[12];    // u(1)
  int       scalingList4x4[6][16];            // se(v)
  int       scalingList8x8[6][64];            // se(v)
  Boolean   useDefaultScalingMatrix4x4Flag[6];
  Boolean   useDefaultScalingMatrix8x8Flag[6];

  // pocType < 2 in the sequence parameter set
  Boolean      botFieldPicOrderFramePresent;  // u(1)
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

  Boolean   weightedPredFlag;                 // u(1)
  unsigned int  weightedBiPredIdc;            // u(2)
  int       picInitQpMinus26;                 // se(v)
  int       picInitQsMinus26;                 // se(v)
  int       chromaQpIndexOffset;              // se(v)
  int       cbQpIndexOffset;                  // se(v)
  int       crQpIndexOffset;                  // se(v)
  int       secondChromaQpIndexOffset;        // se(v)

  Boolean   deblockFilterControlPresent;      // u(1)
  Boolean   constrainedIntraPredFlag;         // u(1)
  Boolean   redundantPicCountPresent;         // u(1)
  Boolean   vuiPicParamFlag;                  // u(1)
  } sPPS;

struct Decoder;
extern sPPS* allocPPS();
extern void freePPS (sPPS* pps);

extern void setPPSbyId (struct Decoder* decoder, int id, sPPS* pps);
extern void readNaluPPS (struct Decoder* decoder, sNalu* nalu);
extern void activatePPS (struct Decoder* decoder, sPPS* pps);
