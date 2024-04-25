#pragma once
struct sMacroBlock;
class cDpb;
struct sPicture;
struct sDecodedRefPicMark;
struct sDataPartition;
struct sMotionContexts;
struct sTextureContexts;
class sSps;
class sPps;

#define MAX_REFERENCE_PICTURES  32 // H.264 allows 32 fields
typedef uint8_t sPixel;
//{{{
enum ePicStructure {
  eFrame    = 0,
  eTopField = 1,
  eBotField = 2
  };
//}}}
//{{{
enum eColorPlane {
  // YUV
  PLANE_Y = 0,  // PLANE_Y
  PLANE_U = 1,  // PLANE_Cb
  PLANE_V = 2,  // PLANE_Cr
  };
//}}}

class cSlice {
public:
  //{{{
  enum eSliceType {
    eP        = 0,
    eB        = 1,
    eI        = 2,
    eSP       = 3,
    eSI       = 4,
    eNumTypes = 5
    };
  //}}}

  static const int kMaxNumSlices = 4;
  //{{{
  static inline const int kDequantCoef8[6][8][8] = {
    {
      {20,  19, 25, 19, 20, 19, 25, 19},
      {19,  18, 24, 18, 19, 18, 24, 18},
      {25,  24, 32, 24, 25, 24, 32, 24},
      {19,  18, 24, 18, 19, 18, 24, 18},
      {20,  19, 25, 19, 20, 19, 25, 19},
      {19,  18, 24, 18, 19, 18, 24, 18},
      {25,  24, 32, 24, 25, 24, 32, 24},
      {19,  18, 24, 18, 19, 18, 24, 18}
    },
    {
      {22,  21, 28, 21, 22, 21, 28, 21},
      {21,  19, 26, 19, 21, 19, 26, 19},
      {28,  26, 35, 26, 28, 26, 35, 26},
      {21,  19, 26, 19, 21, 19, 26, 19},
      {22,  21, 28, 21, 22, 21, 28, 21},
      {21,  19, 26, 19, 21, 19, 26, 19},
      {28,  26, 35, 26, 28, 26, 35, 26},
      {21,  19, 26, 19, 21, 19, 26, 19}
    },
    {
      {26,  24, 33, 24, 26, 24, 33, 24},
      {24,  23, 31, 23, 24, 23, 31, 23},
      {33,  31, 42, 31, 33, 31, 42, 31},
      {24,  23, 31, 23, 24, 23, 31, 23},
      {26,  24, 33, 24, 26, 24, 33, 24},
      {24,  23, 31, 23, 24, 23, 31, 23},
      {33,  31, 42, 31, 33, 31, 42, 31},
      {24,  23, 31, 23, 24, 23, 31, 23}
    },
    {
      {28,  26, 35, 26, 28, 26, 35, 26},
      {26,  25, 33, 25, 26, 25, 33, 25},
      {35,  33, 45, 33, 35, 33, 45, 33},
      {26,  25, 33, 25, 26, 25, 33, 25},
      {28,  26, 35, 26, 28, 26, 35, 26},
      {26,  25, 33, 25, 26, 25, 33, 25},
      {35,  33, 45, 33, 35, 33, 45, 33},
      {26,  25, 33, 25, 26, 25, 33, 25}
    },
    {
      {32,  30, 40, 30, 32, 30, 40, 30},
      {30,  28, 38, 28, 30, 28, 38, 28},
      {40,  38, 51, 38, 40, 38, 51, 38},
      {30,  28, 38, 28, 30, 28, 38, 28},
      {32,  30, 40, 30, 32, 30, 40, 30},
      {30,  28, 38, 28, 30, 28, 38, 28},
      {40,  38, 51, 38, 40, 38, 51, 38},
      {30,  28, 38, 28, 30, 28, 38, 28}
    },
    {
      {36,  34, 46, 34, 36, 34, 46, 34},
      {34,  32, 43, 32, 34, 32, 43, 32},
      {46,  43, 58, 43, 46, 43, 58, 43},
      {34,  32, 43, 32, 34, 32, 43, 32},
      {36,  34, 46, 34, 36, 34, 46, 34},
      {34,  32, 43, 32, 34, 32, 43, 32},
      {46,  43, 58, 43, 46, 43, 58, 43},
      {34,  32, 43, 32, 34, 32, 43, 32}
    }
  };
  //}}}
  //{{{
  //! Dequantization coefficients
  static inline const int kDequantCoef[6][4][4] = {
    {
      { 10, 13, 10, 13},
      { 13, 16, 13, 16},
      { 10, 13, 10, 13},
      { 13, 16, 13, 16}},
    {
      { 11, 14, 11, 14},
      { 14, 18, 14, 18},
      { 11, 14, 11, 14},
      { 14, 18, 14, 18}},
    {
      { 13, 16, 13, 16},
      { 16, 20, 16, 20},
      { 13, 16, 13, 16},
      { 16, 20, 16, 20}},
    {
      { 14, 18, 14, 18},
      { 18, 23, 18, 23},
      { 14, 18, 14, 18},
      { 18, 23, 18, 23}},
    {
      { 16, 20, 16, 20},
      { 20, 25, 20, 25},
      { 16, 20, 16, 20},
      { 20, 25, 20, 25}},
    {
      { 18, 23, 18, 23},
      { 23, 29, 23, 29},
      { 18, 23, 18, 23},
      { 23, 29, 23, 29}}
  };
  //}}}
  //{{{
  static inline const int kQuantCoef[6][4][4] = {
    {
      { 13107,  8066, 13107,  8066},
      {  8066,  5243,  8066,  5243},
      { 13107,  8066, 13107,  8066},
      {  8066,  5243,  8066,  5243}},
    {
      { 11916,  7490, 11916,  7490},
      {  7490,  4660,  7490,  4660},
      { 11916,  7490, 11916,  7490},
      {  7490,  4660,  7490,  4660}},
    {
      { 10082,  6554, 10082,  6554},
      {  6554,  4194,  6554,  4194},
      { 10082,  6554, 10082,  6554},
      {  6554,  4194,  6554,  4194}},
    {
      {  9362,  5825,  9362,  5825},
      {  5825,  3647,  5825,  3647},
      {  9362,  5825,  9362,  5825},
      {  5825,  3647,  5825,  3647}},
    {
      {  8192,  5243,  8192,  5243},
      {  5243,  3355,  5243,  3355},
      {  8192,  5243,  8192,  5243},
      {  5243,  3355,  5243,  3355}},
    {
      {  7282,  4559,  7282,  4559},
      {  4559,  2893,  4559,  2893},
      {  7282,  4559,  7282,  4559},
      {  4559,  2893,  4559,  2893}}
  };
  //}}}
  //{{{
  static inline const int kQuantA[4][4] = {
    { 16, 20, 16, 20},
    { 20, 25, 20, 25},
    { 16, 20, 16, 20},
    { 20, 25, 20, 25}
  };
  //}}}

  // static member
  static cSlice* allocSlice();

  // pub,iuc members
  ~cSlice();

  void initCabacContexts();

  void setReadCbpCavlc();
  void setReadCbpCabac();

  void setIntraPredFunctions();
  void setUpdateDirectFunction();
  void setSliceReadFunctions();

  void setQuant();
  void fillWeightedPredParam();
  void resetWeightedPredParam();

  void initMbAffLists (sPicture* noRefPicture);
  void copyPoc (cSlice* toSlice);
  void updatePicNum();

  void allocRefPicListReordeBuffer();
  void freeRefPicListReorderBuffer();
  void reorderRefPicList (int curList);

  void computeColocated (sPicture** listX[6]);
  void startMacroBlockDecode (sMacroBlock** mb);

  bool endMacroBlockDecode (int eosBit);

  //{{{  public vars
  cDecoder264* decoder = nullptr;

  cPps* activePps = nullptr;
  cSps* activeSps = nullptr;
  cDpb* dpb = nullptr;

  eSliceType sliceType = eI;

  int isIDR = 0;
  int idrPicId = 0;
  int refId = 0;
  int transform8x8Mode = 0;
  bool chroma444notSeparate = false; // indicates chroma 4:4:4 coding with isSeperateColourPlane equal to zero

  int topPoc = 0;   // poc for this top field
  int botPoc = 0;   // poc of bottom field of frame
  int framePoc = 0; // poc of this frame

  // pocMode 0
  uint32_t picOrderCountLsb = 0;
  int deltaPicOrderCountBot = 0;
  signed int PicOrderCntMsb = 0;

  // pocMode 1
  int deltaPicOrderCount[2] = {0};
  uint32_t AbsFrameNum = 0;
  int thisPoc = 0;

  // information need to move to slice
  uint32_t mbIndex = 0;
  uint32_t numDecodedMbs = 0;

  int16_t curSliceIndex = 0;
  int codCount = 0;    // Current count of number of skipped macroBlocks in a row
  int allrefzero = 0;

  int mbAffFrame = 0;
  int directSpatialMvPredFlag = 0; // Indicator for direct mode type (1 for Spatial, 0 for Temporal)
  int numRefIndexActive[2] = {0};    // number of ok list references

  int mError = 0;   // 0 if the dataPartitons[0] contains valid information
  int qp = 0;
  int sliceQpDelta = 0;
  int qs = 0;
  int sliceQsDelta = 0;

  int cabacInitIdc = 0;     // cabac model number
  uint32_t frameNum = 0;

  ePicStructure picStructure = eFrame;
  uint32_t fieldPic = 0;
  uint8_t botField = 0;
  int startMbNum = 0;   // MUST be set by NAL even in case of mError == 1
  int endMbNumPlus1 = 0;

  int curHeader = 0;
  int nextHeader = 0;
  int lastDquant = 0;

  // slice header information;
  int colourPlaneId = 0;             // colourPlaneId of the current coded slice
  int redundantPicCount = 0;
  int spSwitch = 0;                  // 1 for switching sp, 0 for normal sp
  int sliceGroupChangeCycle = 0;
  int redundantSliceRefIndex = 0;    // reference index of redundant slice
  int noOutputPriorPicFlag = 0;
  int longTermRefFlag = 0;
  int adaptRefPicBufFlag = 0;
  sDecodedRefPicMark* decRefPicMarkBuffer = nullptr; // stores memory management control operations

  char listXsize[6] = {0};
  sPicture** listX[6] = {nullptr};

  uint32_t          maxDataPartitions = 0;
  int               dataPartitionMode = 0;
  sDataPartition*   dataPartitionArray = nullptr;
  sMotionContexts*  motionContexts = nullptr;  // pointer to struct of context models for use in eCabac
  sTextureContexts* textureContexts = nullptr; // pointer to struct of context models for use in eCabac

  int mvscale[6][MAX_REFERENCE_PICTURES] = {0};
  int refPicReorderFlag[2] = {0};
  int* modPicNumsIdc[2] = {nullptr};
  int* absDiffPicNumMinus1[2] = {nullptr};
  int* longTermPicIndex[2] = {nullptr};

  int16_t deblockFilterDisableIdc = 0; // Disable deblocking filter on slice
  int16_t deblockFilterC0Offset = 0;   // Alpha and C0 offset for filtering slice
  int16_t deblockFilterBetaOffset = 0; // Beta offset for filtering slice

  int ppsId = 0;             // ID of picture parameter set the slice is referring to
  int noDataPartitionB = 0;  // non-zero, if data dataPartition B is lost
  int noDataPartitionC = 0;  // non-zero, if data dataPartition C is lost

  bool isResetCoef = false;
  bool isResetCoefCr =false;
  sPixel*** mbPred = nullptr;
  sPixel*** mbRec = nullptr;
  int*** mbRess = nullptr;
  int*** cof = nullptr;
  int*** fcfv = nullptr;
  int cofu[16] = {0};

  int** tempRes;
  sPixel** tempBlockL0 = nullptr;
  sPixel** tempBlockL1 = nullptr;
  sPixel** tempBlockL2 = nullptr;
  sPixel** tempBlockL3 = nullptr;

  // Scaling matrix info
  int InvLevelScale4x4_Intra[3][6][4][4] = {0};
  int InvLevelScale4x4_Inter[3][6][4][4] = {0};
  int InvLevelScale8x8_Intra[3][6][8][8] = {0};
  int InvLevelScale8x8_Inter[3][6][8][8] = {0};

  int* qmatrix[12];

  // Cabac
  int coeff[64] = {0}; // one more for EOB
  int coefCount = 0;
  int pos = 0;

  // weighted pred
  uint16_t hasWeightedPred = 0;
  uint16_t weightedBiPredIdc = 0;
  uint16_t lumaLog2weightDenom = 0;
  uint16_t chromaLog2weightDenom = 0;
  int***  weightedPredWeight = nullptr;   // weight in [list][index][component] order
  int***  weightedPredOffset = nullptr;   // offset in [list][index][component] order
  int**** weightedBiPredWeight = nullptr;  // weight in [list][fw_index][bw_index][component] order
  int16_t wpRoundLuma = 0;
  int16_t wpRoundChroma = 0;

  // for signalling to the neighbour logic that this is a deblocker call
  int maxMbVmvR;   // maximum vertical motion vector range in luma quarter pixel units for the current levelIdc
  int refFlag[17]; // 0: i-th previous frame is incorrect

  int ercMvPerMb = 0;
  sMacroBlock* mbData = nullptr;
  sPicture* picture = nullptr;

  int** siBlock = nullptr;
  uint8_t** predMode = nullptr;
  char*  intraBlock = nullptr;
  char chromaVectorAdjust[6][32] = {0};
  //}}}
  //{{{  public c style virtual functionss
  int  (*nalStartCode) (cSlice*, int) = nullptr;
  void (*nalReadMotion) (sMacroBlock*) = nullptr;

  void (*initLists) (cSlice*) = nullptr;
  void (*readCBPcoeffs) (sMacroBlock*) = nullptr;
  int  (*decodeComponenet) (sMacroBlock*, eColorPlane, sPixel**, sPicture*) = nullptr;
  void (*readMacroBlock) (sMacroBlock*) = nullptr;
  void (*interpretMbMode) (sMacroBlock*) = nullptr;

  void (*intraPredChroma) (sMacroBlock*) = nullptr;
  int  (*intraPred4x4) (sMacroBlock*, eColorPlane, int, int, int, int) = nullptr;
  int  (*intraPred8x8) (sMacroBlock*, eColorPlane, int, int) = nullptr;
  int  (*intraPred16x16) (sMacroBlock*, eColorPlane plane, int) = nullptr;

  void (*updateDirectMv) (sMacroBlock*) = nullptr;
  void (*readCoef4x4cavlc) (sMacroBlock*, int, int, int, int[16], int[16], int*) = nullptr;

  void (*infoCbpIntra) (int, int, int*, int*) = nullptr;
  void (*infoCbpInter) (int, int, int*, int*) = nullptr;
  //}}}

private:
  void calculateQuant4x4Param();
  void calculateQuant8x8Param();

  void reorderShortTerm (int curList, int numRefIndexIXactiveMinus1, int picNumLX, int* refIdxLX);
  void reorderLongTerm (sPicture** refPicListX, int numRefIndexIXactiveMinus1, int LongTermPicNum, int* refIdxLX);
  };
