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
enum eSliceType {
  eSliceP        = 0,
  eSliceB        = 1,
  eSliceI        = 2,
  eSliceSP       = 3,
  eSliceSI       = 4,
  eSliceNumTypes = 5
  };
//}}}
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

//{{{
static const int dequant_coef8[6][8][8] = {
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
static const int dequant_coef[6][4][4] = {
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
static const int quant_coef[6][4][4] = {
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
static const int A[4][4] = {
  { 16, 20, 16, 20},
  { 20, 25, 20, 25},
  { 16, 20, 16, 20},
  { 20, 25, 20, 25}
};
//}}}

class cSlice {
public:
  static cSlice* allocSlice();
  ~cSlice();

  void setQuantParams();
  void setReadCbpCoefCavlc();
  void setReadCbpCoefsCabac();
  void setIntraPredFunctions();
  void setUpdateDirectFunction();
  void setSliceReadFunctions();

  void fillWeightedPredParam();
  void resetWeightedPredParam();

  void initMbAffLists (sPicture* noReferencePicture);
  void copyPoc (cSlice* toSlice);
  void updatePicNum();

  void allocRefPicListReordeBuffer();
  void freeRefPicListReorderBuffer();
  void reorderRefPicList (int curList);

  void computeColocated (sPicture** listX[6]);
  void startMacroBlockDecode (sMacroBlock** mb);

  bool endMacroBlockDecode (int eos_bit);

  //{{{  vars
  cDecoder264* decoder;

  cPps* activePps;
  cSps* activeSps;
  cDpb* dpb;

  eSliceType sliceType;

  int isIDR;
  int idrPicId;
  int refId;
  int transform8x8Mode;
  bool chroma444notSeparate; // indicates chroma 4:4:4 coding with isSeperateColourPlane equal to zero

  int topPoc;   // poc for this top field
  int botPoc;   // poc of bottom field of frame
  int framePoc; // poc of this frame

  // pocMode 0
  uint32_t picOrderCountLsb;
  int deltaPicOrderCountBot;
  signed int PicOrderCntMsb;

  // pocMode 1
  int deltaPicOrderCount[2];
  uint32_t AbsFrameNum;
  int thisPoc;

  // information need to move to slice
  uint32_t  mbIndex;
  uint32_t  numDecodedMbs;

  int16_t  curSliceIndex;
  int codCount;    // Current count of number of skipped macroBlocks in a row
  int allrefzero;

  int mbAffFrame;
  int directSpatialMvPredFlag; // Indicator for direct mode type (1 for Spatial, 0 for Temporal)
  int numRefIndexActive[2];    // number of ok list references

  int errorFlag;   // 0 if the dataPartitons[0] contains valid information
  int qp;
  int sliceQpDelta;
  int qs;
  int sliceQsDelta;

  int cabacInitIdc;     // cabac model number
  uint32_t frameNum;

  ePicStructure picStructure;
  uint32_t fieldPic;
  uint8_t botField;
  int startMbNum;   // MUST be set by NAL even in case of errorFlag == 1
  int endMbNumPlus1;
  int maxDataPartitions;
  int dataPartitionMode;
  int curHeader;
  int nextHeader;
  int lastDquant;

  // slice header information;
  int colourPlaneId;             // colourPlaneId of the current coded slice
  int redundantPicCount;
  int spSwitch;                  // 1 for switching sp, 0 for normal sp
  int sliceGroupChangeCycle;
  int redundantSliceRefIndex;    // reference index of redundant slice
  int noOutputPriorPicFlag;
  int longTermRefFlag;
  int adaptRefPicBufFlag;
  sDecodedRefPicMark* decRefPicMarkBuffer; // stores memory management control operations

  char listXsize[6];
  sPicture** listX[6];

  sDataPartition*   dataPartitions;      // array of dataPartition
  sMotionContexts*  motionInfoContexts;  // pointer to struct of context models for use in eCabac
  sTextureContexts* textureInfoContexts; // pointer to struct of context models for use in eCabac

  int mvscale[6][MAX_REFERENCE_PICTURES];
  int refPicReorderFlag[2];
  int* modPicNumsIdc[2];
  int* absDiffPicNumMinus1[2];
  int* longTermPicIndex[2];

  int16_t deblockFilterDisableIdc; // Disable deblocking filter on slice
  int16_t deblockFilterC0Offset;   // Alpha and C0 offset for filtering slice
  int16_t deblockFilterBetaOffset; // Beta offset for filtering slice

  int ppsId;             // ID of picture parameter set the slice is referring to
  int noDataPartitionB;  // non-zero, if data dataPartition B is lost
  int noDataPartitionC;  // non-zero, if data dataPartition C is lost

  bool isResetCoef;
  bool isResetCoefCr;
  sPixel*** mbPred;
  sPixel*** mbRec;
  int*** mbRess;
  int*** cof;
  int*** fcf;
  int cofu[16];

  int** tempRes;
  sPixel** tempBlockL0;
  sPixel** tempBlockL1;
  sPixel** tempBlockL2;
  sPixel** tempBlockL3;

  // Scaling matrix info
  int InvLevelScale4x4_Intra[3][6][4][4];
  int InvLevelScale4x4_Inter[3][6][4][4];
  int InvLevelScale8x8_Intra[3][6][8][8];
  int InvLevelScale8x8_Inter[3][6][8][8];

  int* qmatrix[12];

  // Cabac
  int coeff[64]; // one more for EOB
  int coefCount;
  int pos;

  // weighted pred
  uint16_t hasWeightedPred;
  uint16_t weightedBiPredIdc;
  uint16_t lumaLog2weightDenom;
  uint16_t chromaLog2weightDenom;
  int***  weightedPredWeight;   // weight in [list][index][component] order
  int***  weightedPredOffset;   // offset in [list][index][component] order
  int**** weightedBiPredWeight;  // weight in [list][fw_index][bw_index][component] order
  int16_t wpRoundLuma;
  int16_t wpRoundChroma;

  // for signalling to the neighbour logic that this is a deblocker call
  int maxMbVmvR;   // maximum vertical motion vector range in luma quarter pixel units for the current levelIdc
  int refFlag[17]; // 0: i-th previous frame is incorrect

  int ercMvPerMb;
  sMacroBlock* mbData;
  sPicture* picture;

  int** siBlock;
  uint8_t** predMode;
  char*  intraBlock;
  char chromaVectorAdjust[6][32];
  //}}}
  //{{{  c style virtual functionss
  int  (*nalStartCode) (cSlice*, int);
  void (*initLists) (cSlice*);
  void (*readCBPcoeffs) (sMacroBlock*);
  int  (*decodeComponenet) (sMacroBlock*, eColorPlane, sPixel**, sPicture*);
  void (*nalReadMotionInfo) (sMacroBlock*);
  void (*readMacroBlock) (sMacroBlock*);
  void (*interpretMbMode) (sMacroBlock*);
  void (*intraPredChroma) (sMacroBlock*);
  int  (*intraPred4x4) (sMacroBlock*, eColorPlane, int, int, int, int);
  int  (*intraPred8x8) (sMacroBlock*, eColorPlane, int, int);
  int  (*intraPred16x16) (sMacroBlock*, eColorPlane plane, int);
  void (*updateDirectMv) (sMacroBlock*);
  void (*readCoef4x4cavlc) (sMacroBlock*, int, int, int, int[16], int[16], int*);
  void (*linfoCbpIntra) (int, int, int*, int*);
  void (*linfoCbpInter) (int, int, int*, int*);
  //}}}

private:
  void calculateQuant4x4Param();
  void calculateQuant8x8Param();

  void reorderShortTerm (int curList, int numRefIndexIXactiveMinus1, int picNumLX, int* refIdxLX);
  void reorderLongTerm (sPicture** refPicListX, int numRefIndexIXactiveMinus1, int LongTermPicNum, int* refIdxLX);
  };
