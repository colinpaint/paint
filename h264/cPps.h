#pragma once
#include "global.h"
#include "nalu.h"

constexpr int kMaxPps = 4;

class cPps {
public:
  static int readNalu (cDecoder264* decoder, cNalu* nalu);

  std::string getString();
  bool isEqual (cPps& pps);

  // vars
  bool     ok = false;
  int      naluLen = 0;

  uint32_t id = 0;                           // ue(v)
  uint32_t spsId = 0;                        // ue(v)
  eEntropyCodingType entropyCoding = eCavlc; // u(1)
  bool     frameBotField = false;            // u(1)

  uint32_t numSliceGroupsMinus1 = 0;         // ue(v)
  uint32_t sliceGroupMapType = 0;            // ue(v)
  //{{{  optional sliceGroupMapType fields
  // sliceGroupMapType 0
  uint32_t runLengthMinus1[8] = {0};         // ue(v)

  // sliceGroupMapType 2
  uint32_t topLeft[8] = {0};                 // ue(v)
  uint32_t botRight[8] = {0};                // ue(v)

  // sliceGroupMapType 3 || 4 || 5
  bool   sliceGroupChangeDirectionFlag = false; // u(1)
  uint32_t sliceGroupChangeRateMius1 = 0;    // ue(v)

  // sliceGroupMapType 6
  uint32_t picSizeMapUnitsMinus1 = 0;        // ue(v)
  uint8_t* sliceGroupId = 0;                 // complete MBAmap u(v)
  //}}}

  int      numRefIndexL0defaultActiveMinus1; // ue(v)
  int      numRefIndexL1defaultActiveMinus1; // ue(v)

  bool     hasWeightedPred = false;          // u(1)
  uint32_t weightedBiPredIdc = 0;            // u(2)

  int      picInitQpMinus26 = 0;             // se(v)
  int      picInitQsMinus26 = 0;             // se(v)
  int      chromaQpOffset = 0;               // se(v)

  bool     hasDeblockFilterControl = false;  // u(1)
  bool     hasConstrainedIntraPred = false;  // u(1)
  bool     redundantPicCountPresent = false; // u(1)

  bool     hasMoreData = false;
  bool     hasTransform8x8mode = false;      // u(1)
  bool     hasPicScalingMatrix = false;      // u(1)
  //{{{  optional scalingMatrix fields
  int  picScalingListPresentFlag[12] = {};  // u(1)
  int  scalingList4x4[6][16] = {};          // se(v)
  int  scalingList8x8[6][64] = {};          // se(v)

  bool useDefaultScalingMatrix4x4Flag[6] = {false};
  bool useDefaultScalingMatrix8x8Flag[6] = {false};
  //}}}
  int      chromaQpOffset2 = 0;              // se(v)

private:
  void readFromStream (cDecoder264* decoder, sDataPartition* dataPartition);
  };
