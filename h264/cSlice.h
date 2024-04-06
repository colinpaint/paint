#pragma once
struct sMacroBlock;
struct sDpb;
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

class cSlice {
public:
  static cSlice* allocSlice();
  ~cSlice();

  void fillWeightedPredParam();
  void resetWeightedPredParam();
  void initMbAffLists (sPicture* noReferencePicture);
  void copyPoc (cSlice* toSlice);

  void allocRefPicListReordeBuffer();
  void freeRefPicListReorderBuffer();

  void setReadCbpCoefCavlc();
  void setReadCbpCoefsCabac();
  void setIntraPredFunctions();
  void setUpdateDirectFunction();
  void setSliceFunctions();
  void startMacroBlockDecode (sMacroBlock** mb);
  bool endMacroBlockDecode (int eos_bit);

  //{{{  vars
  cDecoder264* decoder;

  cPps* activePps;
  cSps* activeSps;
  sDpb* dpb;

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
  };
