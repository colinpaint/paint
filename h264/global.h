#pragma once
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/timeb.h>

#include "win32.h"
#include "defines.h"
#include "functions.h"
#include "frame.h"

#include "nalu.h"
#include "sps.h"
#include "pps.h"
//}}}
//{{{  enum eColorPlane
typedef enum {
  // YUV
  PLANE_Y = 0,  // PLANE_Y
  PLANE_U = 1,  // PLANE_Cb
  PLANE_V = 2,  // PLANE_Cr
  } eColorPlane;
//}}}
//{{{  enum eColorComponent
typedef enum {
  LumaComp = 0,
  CrComp = 1,
  CbComp = 2
  } eColorComponent;
//}}}
//{{{  enum ePicStructure
typedef enum {
  eFrame,
  eTopField,
  eBotField
  } ePicStructure;
//}}}
//{{{  enum eDataPartitionType
typedef enum {
  eDataPartition1, // no dataPartiton
  eDataPartition3  // 3 dataPartitions
  } eDataPartitionType;
//}}}
//{{{  enum eSyntaxElementType
typedef enum {
  SE_HEADER,
  SE_PTYPE,
  SE_MBTYPE,
  SE_REFFRAME,
  SE_INTRAPREDMODE,
  SE_MVD,
  SE_CBP,
  SE_LUM_DC_INTRA,
  SE_CHR_DC_INTRA,
  SE_LUM_AC_INTRA,
  SE_CHR_AC_INTRA,
  SE_LUM_DC_INTER,
  SE_CHR_DC_INTER,
  SE_LUM_AC_INTER,
  SE_CHR_AC_INTER,
  SE_DELTA_QUANT,
  SE_BFRAME,
  SE_EOS,
  SE_MAX_ELEMENTS = 20
  } eSyntaxElementType;
//}}}
//{{{  enum eSymbolType
typedef enum {
  eCavlc,
  eCabac
  } eSymbolType;
//}}}
//{{{  enum eCodingType
typedef enum {
  eFrameCoding       = 0,
  eFieldCoding       = 1,
  eAdaptiveCoding    = 2,
  eFrameMbPairCoding = 3
 } eCodingType;
//}}}
//{{{  enum ePredList
typedef enum {
  LIST_0 = 0,
  LIST_1 = 1,
  BI_PRED = 2,
  BI_PRED_L0 = 3,
  BI_PRED_L1 = 4
  } ePredList;
//}}}
//{{{  enum eSliceType
typedef enum {
  ePslice = 0,
  eBslice = 1,
  eIslice = 2,
  eSPslice = 3,
  eSIslice = 4,
  eNumSliceTypes = 5
  } eSliceType;
//}}}
//{{{  enum eMotionEstimation
typedef enum {
  eFullPel,
  eHalfPel,
  eQuarterPel
  } eMotionEstimation;
//}}}
//{{{  enum eComponentType
typedef enum {
  IS_LUMA = 0,
  IS_CHROMA = 1
  } eComponentType;
//}}}
//{{{  enum eRcModelType
typedef enum {
  RC_MODE_0 = 0,
  RC_MODE_1 = 1,
  RC_MODE_2 = 2,
  RC_MODE_3 = 3
  } eRcModelType;
//}}}
//{{{  enum eWeightedPredictionType;
typedef enum {
  WP_MCPREC_PLUS0 =       4,
  WP_MCPREC_PLUS1 =       5,
  WP_MCPREC_MINUS0 =      6,
  WP_MCPREC_MINUS1 =      7,
  WP_MCPREC_MINUS_PLUS0 = 8,
  WP_REGULAR =            9
  } eWeightedPredictionType;
//}}}

struct MacroBlock;
//{{{  sDecodeEnv
typedef struct {
  unsigned int range;
  unsigned int value;
  int          bitsLeft;
  byte*        codeStream;
  int*         codeStreamLen;
  } sDecodeEnv;
//}}}
//{{{  sBitStream
typedef struct {
  // eCavlc Decoding
  byte* bitStreamBuffer;    // actual codebuffer for read bytes
  int   bitStreamOffset; // actual position in the codebuffer, bit-oriented, eCavlc only
  int   bitStreamLen;    // over codebuffer lnegth, byte oriented, eCavlc only
  int   errorFlag;       // error indication, 0: no error, else unspecified error

  // eCabac Decoding
  int   readLen;         // actual position in the codebuffer, eCabac only
  int   codeLen;         // overall codebuffer length, eCabac only
  } sBitStream;
//}}}
//{{{  sSyntaxElement
typedef struct SyntaxElement {
  int           type;        // type of syntax element for data part.
  int           value1;      // numerical value of syntax element
  int           value2;      // for blocked symbols, e.g. run/level
  int           len;         // length of code
  int           inf;         // info part of eCavlc code
  unsigned int  bitpattern;  // eCavlc bitpattern
  int           context;     // eCabac context
  int           k;           // eCabac context for coeff_count,uv

  // eCavlc mapping to syntaxElement
  void (*mapping) (int, int, int*, int*);

  // eCabac actual coding method of each individual syntax element type
  void (*reading) (struct MacroBlock*, struct SyntaxElement*, sDecodeEnv*);
  } sSyntaxElement;
//}}}
//{{{  sDataPartition
typedef struct DataPartition {
  sBitStream*  s;
  sDecodeEnv deCabac;

  int (*readSyntaxElement) (struct MacroBlock*, struct SyntaxElement*, struct DataPartition*);
  } sDataPartition;
//}}}

//{{{  sBiContextType
typedef struct {
  uint16        state; // index into state-table CP
  unsigned char MPS;   // least probable symbol 0/1 CP
  unsigned char dummy; // for alignment
  } sBiContextType;
//}}}
//{{{  sMotionInfoContexts
#define NUM_MB_TYPE_CTX  11
#define NUM_B8_TYPE_CTX  9
#define NUM_MV_RES_CTX   10
#define NUM_REF_NO_CTX   6
#define NUM_DELTA_QP_CTX 4
#define NUM_MB_AFF_CTX 4
#define NUM_TRANSFORM_SIZE_CTX 3

typedef struct {
  sBiContextType mbTypeContexts[3][NUM_MB_TYPE_CTX];
  sBiContextType b8_type_contexts[2][NUM_B8_TYPE_CTX];
  sBiContextType mvResContexts[2][NUM_MV_RES_CTX];
  sBiContextType ref_no_contexts[2][NUM_REF_NO_CTX];
  sBiContextType delta_qp_contexts[NUM_DELTA_QP_CTX];
  sBiContextType mb_aff_contexts[NUM_MB_AFF_CTX];
  } sMotionInfoContexts;
//}}}
//{{{  sTextureInfoContexts
#define NUM_IPR_CTX    2
#define NUM_CIPR_CTX   4
#define NUM_CBP_CTX    4
#define NUM_BCBP_CTX   4
#define NUM_MAP_CTX   15
#define NUM_LAST_CTX  15
#define NUM_ONE_CTX    5
#define NUM_ABS_CTX    5

typedef struct {
  sBiContextType transform_size_contexts[NUM_TRANSFORM_SIZE_CTX];
  sBiContextType ipr_contexts[NUM_IPR_CTX];
  sBiContextType cipr_contexts[NUM_CIPR_CTX];
  sBiContextType cbp_contexts[3][NUM_CBP_CTX];
  sBiContextType bcbp_contexts[NUM_BLOCK_TYPES][NUM_BCBP_CTX];
  sBiContextType map_contexts[2][NUM_BLOCK_TYPES][NUM_MAP_CTX];
  sBiContextType last_contexts[2][NUM_BLOCK_TYPES][NUM_LAST_CTX];
  sBiContextType one_contexts[NUM_BLOCK_TYPES][NUM_ONE_CTX];
  sBiContextType abs_contexts[NUM_BLOCK_TYPES][NUM_ABS_CTX];
  } sTextureInfoContexts;
//}}}

struct Picture;
struct PicMotion;
//{{{  sBlockPos
typedef struct {
  short x;
  short y;
  } sBlockPos;
//}}}
//{{{  sPixelPos
typedef struct {
  int   available;
  int   mbIndex;
  short x;
  short y;
  short posX;
  short posY;
  } sPixelPos;
//}}}
//{{{  sCbpStructure
typedef struct  {
  int64 blk;
  int64 bits;
  int64 bits_8x8;
  } sCbpStructure;
//}}}
//{{{  sMotionVec
typedef struct {
  short mvX;
  short mvY;
  } sMotionVec;
//}}}
//{{{  sMacroBlock
typedef struct MacroBlock {
  struct Decoder* decoder;
  struct Slice*   slice;

  int     mbIndexX;
  int     mbIndexA;
  int     mbIndexB;
  int     mbIndexC;
  int     mbIndexD;

  Boolean mbAvailA;
  Boolean mbAvailB;
  Boolean mbAvailC;
  Boolean mbAvailD;

  sBlockPos mb;
  int     blockX;
  int     blockY;
  int     blockYaff;
  int     pixX;
  int     pixY;
  int     pixcX;
  int     piccY;

  int     subblockX;
  int     subblockY;

  int     qp;                   // QP luma
  int     qpc[2];               // QP chroma
  int     qpScaled[MAX_PLANE];  // QP scaled for all comps.

  Boolean isLossless;
  Boolean isIntraBlock;
  Boolean isVblock;
  int     DeblockCall;

  short   sliceNum;
  char    errorFlag;            // error indicator flag that enables conceal
  char    dplFlag;           // error indicator flag that signals a missing data dataPartition
  short   deltaQuant;        // for rate control
  short   listOffset;

  struct MacroBlock* mbCabacUp;   // pointer to neighboring MB (eCabac)
  struct MacroBlock* mbCabacLeft; // pointer to neighboring MB (eCabac)

  struct MacroBlock* mbUp;    // neighbors for loopfilter
  struct MacroBlock* mbLeft;  // neighbors for loopfilter

  // some storage of macroblock syntax elements for global access
  short   mbType;
  short   mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];      //!< indices correspond to [forw,backw][blockY][blockX][x,y]
  int     cbp;
  sCbpStructure  cbpStructure[3];

  int   i16mode;
  char  b8mode[4];
  char  b8pdir[4];
  char  ipmode_DPCM;
  char  cPredMode;       //!< chroma intra prediction mode
  char  skipFlag;
  short deblockFilterDisableIdc;
  short deblockFilterC0offset;
  short deblockFilterBetaOffset;

  Boolean mbField;

  // Flag for MBAFF deblocking;
  byte  mixedModeEdgeFlag;

  // deblocking strength indices
  byte strengthV[4][4];
  byte strengthH[4][16];

  Boolean lumaTransformSize8x8flag;
  Boolean noMbPartLessThan8x8Flag;

  // virtual methods
  void (*iTrans4x4) (struct MacroBlock*, eColorPlane, int, int);
  void (*iTrans8x8) (struct MacroBlock*, eColorPlane, int, int);
  void (*GetMVPredictor) (struct MacroBlock*, sPixelPos*, sMotionVec*, short, struct PicMotion**, int, int, int, int, int);
  int  (*readStoreCBPblockBit) (struct MacroBlock*, sDecodeEnv*, int);
  char (*readRefPictureIndex) (struct MacroBlock*, struct SyntaxElement*, struct DataPartition*, char, int);
  void (*readCompCoef4x4cabac) (struct MacroBlock*, struct SyntaxElement*, eColorPlane, int(*)[4], int, int);
  void (*readCompCoef8x8cabac) (struct MacroBlock*, struct SyntaxElement*, eColorPlane);
  void (*readCompCoef4x4cavlc) (struct MacroBlock*, eColorPlane, int(*)[4], int, int, byte**);
  void (*readCompCoef8x8cavlc) (struct MacroBlock*, eColorPlane, int(*)[8], int, int, byte**);
  } sMacroBlock;
//}}}
//{{{  sWpParam
typedef struct {
  short weight[3];
  short offset[3];
  } sWpParam;
//}}}
//{{{  sImage
typedef struct Image {
  sFrameFormat format;                 // image format
  sPixel** frm_data[MAX_PLANE];        // Frame Data
  sPixel** top_data[MAX_PLANE];        // pointers to top field data
  sPixel** bot_data[MAX_PLANE];        // pointers to bottom field data
  sPixel** frm_data_buf[2][MAX_PLANE]; // Frame Data
  sPixel** top_data_buf[2][MAX_PLANE]; // pointers to top field data
  sPixel** bot_data_buf[2][MAX_PLANE]; // pointers to bottom field data
  int frm_stride[MAX_PLANE];
  int top_stride[MAX_PLANE];
  int bot_stride[MAX_PLANE];
  } sImage;
//}}}
//{{{  sDecodedPic
typedef struct DecodedPic {
  int valid;

  int yuvFormat; // 4:0:0, 4:2:0, 4:2:2, 4:4:4
  int bitDepth;

  int width;
  int height;
  int yStride;
  int uvStride;

  int bufSize;
  byte* yBuf;
  byte* uBuf;
  byte* vBuf;

  int poc;

  struct DecodedPic* next;
  } sDecodedPic;
//}}}
//{{{  sDecodedRefPicMarking
typedef struct DecodedRefPicMarking {
  int memManagement;
  int diffPicNumMinus1;
  int longTermPicNum;
  int longTermFrameIndex;
  int maxLongTermFrameIndexPlus1;

  struct DecodedRefPicMarking* next;
  } sDecodedRefPicMarking;
//}}}
//{{{  sOldSlice
typedef struct OldSlice {
  unsigned fieldPicFlag;
  unsigned frameNum;
  int      nalRefIdc;
  unsigned picOrderCountLsb;
  int      deltaPicOrderCountBot;
  int      deltaPicOrderCount[2];
  int      botFieldFlag;
  int      idrFlag;
  int      idrPicId;
  int      ppsId;
  } sOldSlice;
//}}}
//{{{  sSlice
typedef struct Slice {
  struct Decoder* decoder;

  sPPS* activePPS;
  sSPS* activeSPS;
  struct DPB* dpb;

  int idrFlag;
  int idrPicId;
  int refId;
  int transform8x8Mode;
  Boolean chroma444notSeparate; // indicates chroma 4:4:4 coding with sepColourPlaneFlag equal to zero

  int topPoc;   // poc for this top field
  int botPoc;   // poc of bottom field of frame
  int framePoc; // poc of this frame

  // pocMode 0
  unsigned int picOrderCountLsb;
  int deletaPicOrderCountBot;
  signed int PicOrderCntMsb;

  // pocMode 1
  int deltaPicOrderCount[2];
  unsigned int AbsFrameNum;
  int thisPoc;

  // information need to move to slice
  unsigned int  mbIndex;
  unsigned int  numDecodedMbs;

  short         curSliceIndex;
  int           codCount;    // Current count of number of skipped macroblocks in a row
  int           allrefzero;

  int           mbAffFrame;
  int           directSpatialMvPredFlag; // Indicator for direct mode type (1 for Spatial, 0 for Temporal)
  int           numRefIndexActive[2];    // number of available list references

  int           errorFlag;   // 0 if the dataPartitons[0] contains valid information
  int           qp;
  int           sliceQpDelta;
  int           qs;
  int           sliceQsDelta;
  int           sliceType;    // slice type
  int           modelNum;     // cabac model number
  unsigned int  frameNum;     // frameNum for this frame
  unsigned int  fieldPicFlag;
  byte          botFieldFlag;
  ePicStructure picStructure; // Identify picture picStructure type
  int           startMbNum;   // MUST be set by NAL even in case of errorFlag == 1
  int           endMbNumPlus1;
  int           maxDataPartitions;
  int           dataPartitionMode;   // data dping mode
  int           curHeader;
  int           nextHeader;
  int           lastDquant;

  // slice header information;
  int colourPlaneId;             // colourPlaneId of the current coded slice
  int redundantPicCount;
  int spSwitch;                   // 1 for switching sp, 0 for normal sp
  int sliceGroupChangeCycle;
  int redundantSliceRefIndex;     // reference index of redundant slice
  int noOutputPriorPicFlag;
  int longTermRefFlag;
  int adaptRefPicBufFlag;
  sDecodedRefPicMarking* decRefPicMarkingBuffer; // stores the memory management control operations

  char listXsize[6];
  struct Picture** listX[6];

  sDataPartition*       dataPartitions; // array of dataPartition
  sMotionInfoContexts*  motionInfoContexts;  // pointer to struct of context models for use in eCabac
  sTextureInfoContexts* textureInfoContexts; // pointer to struct of context models for use in eCabac

  int   mvscale[6][MAX_REFERENCE_PICTURES];
  int   refPicReorderFlag[2];
  int*  modPicNumsIdc[2];
  int*  absDiffPicNumMinus1[2];
  int*  longTermPicIndex[2];

  short deblockFilterDisableIdc; // Disable deblocking filter on slice
  short deblockFilterC0offset;   // Alpha and C0 offset for filtering slice
  short deblockFilterBetaOffset; // Beta offset for filtering slice

  int   ppsId;             // ID of picture parameter set the slice is referring to
  int   noDataPartitionB;  // non-zero, if data dataPartition B is lost
  int   noDataPartitionC;  // non-zero, if data dataPartition C is lost

  Boolean   isResetCoef;
  Boolean   isResetCoefCr;
  sPixel*** mbPred;
  sPixel*** mbRec;
  int***    mbRess;
  int***    cof;
  int***    fcf;
  int       cofu[16];

  int**     tempRes;
  sPixel**  tempBlockL0;
  sPixel**  tempBlockL1;
  sPixel**  tempBlockL2;
  sPixel**  tempBlockL3;

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

  // weighted prediction
  unsigned short weightedPredFlag;
  unsigned short weightedBiPredIdc;
  unsigned short lumaLog2weightDenom;
  unsigned short chromaLog2weightDenom;
  sWpParam** wpParam;    // wp parameters in [list][index]
  int***     wpWeight;   // weight in [list][index][component] order
  int***     wpOffset;   // offset in [list][index][component] order
  int****    wbpWeight;  // weight in [list][fw_index][bw_index][component] order
  short      wpRoundLuma;
  short      wpRoundChroma;

  // for signalling to the neighbour logic that this is a deblocker call
  int maxMbVmvR;   // maximum vertical motion vector range in luma quarter pixel units for the current levelIdc
  int refFlag[17]; // 0: i-th previous frame is incorrect

  int ercMvPerMb;
  sMacroBlock* mbData;
  struct Picture* picture;

  int**  siBlock;
  byte** predMode;
  char*  intraBlock;
  char   chromaVectorAdjust[6][32];

  // virtual methods
  int  (*nalStartCode) (struct Slice*, int);
  void (*initLists) (struct Slice*);
  void (*readCBPcoeffs) (sMacroBlock*);
  int  (*decodeComponenet) (sMacroBlock*, eColorPlane, sPixel**, struct Picture*);
  void (*nalReadMotionInfo) (sMacroBlock*);
  void (*readMacroblock) (sMacroBlock*);
  void (*interpretMbMode) (sMacroBlock*);
  void (*intraPredChroma) (sMacroBlock*);
  int  (*intraPred4x4) (sMacroBlock*, eColorPlane, int, int, int, int);
  int  (*intraPred8x8) (sMacroBlock*, eColorPlane, int, int);
  int  (*intraPred16x16) (sMacroBlock*, eColorPlane plane, int);
  void (*updateDirectMvInfo) (sMacroBlock*);
  void (*readCoef4x4cavlc) (sMacroBlock*, int, int, int, int[16], int[16], int*);
  void (*linfoCbpIntra) (int, int, int*, int*);
  void (*linfoCbpInter) (int, int, int*, int*);
  } sSlice;
//}}}
//{{{  sCoding
typedef struct CodingParam {
  int profileIdc;

  int picStructure;           // Identify picture picStructure type
  int yuvFormat;
  int sepColourPlaneFlag;
  int type;                // image type INTER/INTRA

  // size
  int width;
  int height;
  int widthCr;
  int heightCr;

  int iLumaPadX;
  int iLumaPadY;
  int iChromaPadX;
  int iChromaPadY;

  // bits
  int picUnitBitSizeDisk;
  short bitDepthLuma;
  short bitDepthChroma;
  int bitDepthScale[2];
  int bitDepthLumaQpScale;
  int bitDepthChromaQpScale;
  int maxPelValueComp[MAX_PLANE];          // max value that one picture element (pixel) can take (depends on pic_unit_bitDepth)
  unsigned int dcPredValueComp[MAX_PLANE]; // component value for DC prediction (depends on component pel bit depth)

  int numUvBlocks;
  int numCdcCoeff;
  int numBlock8x8uv;
  int losslessQpPrimeFlag;

  // macroblocks
  unsigned int picWidthMbs;
  unsigned int picHeightMapUnits;
  unsigned int frameHeightMbs;
  unsigned int frameSizeMbs;

  int mbCrSizeX;
  int mbCrSizeY;
  int mbCrSizeXblock;
  int mbCrSizeYblock;
  int mbCrSize;
  int mbSize[3][2];       // component macroblock dimensions
  int mbSizeBlock[3][2];  // component macroblock dimensions
  int mbSizeShift[3][2];

  int maxVmvR;            // maximum vertical motion vector range in lumaQuarterFrame pixel units
  int maxFrameNum;

  int subpelX;
  int subpelY;
  int shiftpelX;
  int shiftpelY;
  int totalScale;
  } sCoding;
//}}}
//{{{  sParam
typedef struct Param {
  int naluDebug;
  int vlcDebug;
  int sliceDebug;
  int spsDebug;
  int ppsDebug;
  int seiDebug;
  int outDebug;
  int deblock;

  int refOffset;
  int pocScale;
  int refPocGap;
  int pocGap;
  int concealMode;
  int intraProfileDeblocking; // Loop filter usage determined by flags and parameters in s

  sFrameFormat source;
  sFrameFormat output;
  int dpbPlus[2];
  } sParam;
//}}}
//{{{  sInfo
typedef struct Info {
  TIME_T  startTime;
  TIME_T  endTime;
  int     took;

  char    spsStr[10];
  char    sliceStr[9];
  char    tookStr[80];
  } sInfo;
//}}}
//{{{  sDecoder
typedef struct Decoder {
  sParam       param;
  sInfo        info;

  // nalu
  int          gotLastNalu;
  int          naluCount;
  sNalu*       nalu;
  sNalu*       pendingNalu;
  sAnnexB*     annexB;

  // sps
  int          gotSPS;
  sSPS         sps[32];
  sSPS*        activeSPS;

  // pps
  sPPS         pps[MAX_PPS];
  sPPS*        activePPS;
  sPPS*        nextPPS;

  int          decodeFrameNum;
  int          idrFrameNum;
  unsigned int preFrameNum;  // last decoded slice. For detecting gap in frameNum.
  unsigned int prevFrameNum; // number of previous slice
  int          newFrame;

  int          nonConformingStream;
  int          deblockEnable;
  int          deblockMode;  // 0: deblock in picture, 1: deblock in slice;

  // recovery
  int          recoveryPoint;
  int          recoveryPointFound;
  int          recoveryFrameCount;
  int          recoveryFrameNum;
  int          recoveryPoc;
  int          recoveryFlag;

  // slice
  int          picSliceIndex;
  int          numDecodedMbs;
  int          numDecodedSlices;
  int          numAllocatedSlices;
  sSlice**     sliceList;
  sSlice*      nextSlice;           // pointer to first sSlice of next picture;
  struct OldSlice* oldSlice;

  int picDiskUnitSize;

  // output
  struct DPB*        dpb;
  int                lastHasMmco5;
  int                dpbPoc[100];
  struct Picture*    picture;
  struct Picture*    decPictureJV[MAX_PLANE];  // picture to be used during 4:4:4 independent mode decoding
  struct Picture*    noReferencePicture;       // dummy storable picture for recovery point
  struct FrameStore* lastOutFramestore;
  sDecodedPic*       decOutputPic;
  struct FrameStore* outBuffer;
  struct Picture*    pendingOut;
  int                pendingOutState;

  // sCoding duplicates
  int width;
  int height;
  int widthCr;
  int heightCr;
  int mbCrSizeX;
  int mbCrSizeY;
  int mbCrSizeXblock;
  int mbCrSizeYblock;
  int mbCrSize;
  int mbSize[3][2];
  int mbSizeBlock[3][2];
  int mbSizeShift[3][2];
  short bitDepthLuma;
  short bitDepthChroma;

  // sCoding
  sCoding      coding;
  sBlockPos*   picPos;
  byte****     nzCoeff;
  sMacroBlock* mbData;              // array containing all MBs of a whole frame
  char*        intraBlock;
  byte**       predMode;            // prediction type [90][74]
  int**        siBlock;
  sMacroBlock* mbDataJV[MAX_PLANE];
  char*        intraBlockJV[MAX_PLANE];
  byte**       predModeJV[MAX_PLANE];
  int**        siBlockJV[MAX_PLANE];

  // POC
  int          lastRefPicPoc;

  // - mode 0:
  signed int   prevPocMsb;
  unsigned int prevPocLsb;
  int          lastPicBotField;

  // - mode 1:
  signed int   expectedPOC, pocCycleCount, frameNumPocCycle;
  unsigned int previousFrameNum;
  unsigned int frameNumOffset;
  int          expectedDeltaPerPocCycle;
  int          thisPoc;
  int          previousFrameNumOffset;

  unsigned int picHeightInMbs;
  unsigned int picSizeInMbs;

  int          noOutputPriorPicFlag;

  // non-zero: i-th previous frame is correct
  int  isPrimaryOk;    // if primary frame is correct, 0: incorrect
  int  isRedundantOk;  // if redundant frame is correct, 0:incorrect

  int* qpPerMatrix;
  int* qpRemMatrix;

  // Error parameters
  struct ObjectBuffer* ercObjectList;
  struct ErcVariables* ercErrorVar;
  int  ercMvPerMb;
  int  ecFlag[SE_MAX_ELEMENTS];  // array to set errorconcealment

  // fmo
  int* mbToSliceGroupMap;
  int* mapUnitToSliceGroupMap;
  int  sliceGroupsNum;  // the number of slice groups -1 (0 == scan order, 7 == maximum)

  // picture error conceal
  // concealHead points to first node in list, concealTail points to last node in list
  // Initialize both to NULL, meaning no nodes in list yet
  struct ConcealNode* concealHead;
  struct ConcealNode* concealTail;
  int                 concealMode;
  int                 earlierMissingPoc;
  unsigned int        concealFrame;
  int                 idrConcealFlag;
  int                 concealSliceType;

  // virtual functions
  void (*getNeighbour) (sMacroBlock*, int, int, int[2], sPixelPos*);
  void (*getMbBlockPos) (sBlockPos*, int, short*, short*);
  void (*getStrengthV) (sMacroBlock*, int, int, struct Picture*);
  void (*getStrengthH) (sMacroBlock*, int, int, struct Picture*);
  void (*edgeLoopLumaV) (eColorPlane, sPixel**, byte*, sMacroBlock*, int);
  void (*edgeLoopLumaH) (eColorPlane, sPixel**, byte*, sMacroBlock*, int, struct Picture*);
  void (*edgeLoopChromaV) (sPixel**, byte*, sMacroBlock*, int, int, struct Picture*);
  void (*edgeLoopChromaH) (sPixel**, byte*, sMacroBlock*, int, int, struct Picture*);
  } sDecoder;
//}}}

//{{{
static inline int isBLprofile (unsigned profileIdc) {
  return (profileIdc == BASELINE) ||
         (profileIdc == MAIN) ||
         (profileIdc == EXTENDED) ||
         (profileIdc == FREXT_CAVLC444) ||
         (profileIdc == FREXT_HP) || (profileIdc == FREXT_Hi10P) ||
         (profileIdc == FREXT_Hi422) || (profileIdc == FREXT_Hi444);
}
//}}}
//{{{
static inline int isFrextProfile (unsigned profileIdc) {
  // we allow all FRExt tools, when no profile is active
  return (profileIdc == NO_PROFILE) ||
         (profileIdc == FREXT_HP) ||
         (profileIdc == FREXT_Hi10P) || (profileIdc == FREXT_Hi422) ||
         (profileIdc == FREXT_Hi444) || (profileIdc == FREXT_CAVLC444);
}
//}}}
//{{{
static inline int isHiIntraOnlyProfile (unsigned profileIdc, Boolean constrainedSet3flag) {
  return (((profileIdc == FREXT_Hi10P) ||
           (profileIdc == FREXT_Hi422) ||
           (profileIdc == FREXT_Hi444)) && constrainedSet3flag) ||
         (profileIdc == FREXT_CAVLC444);
  }
//}}}

//{{{
#ifdef __cplusplus
  extern "C" {
#endif
//}}}
  extern sDecoder* gDecoder;

  extern void error (char* text);

  extern void initFrext (sDecoder* decoder);
  extern void initGlobalBuffers (sDecoder* decoder);
  extern void freeGlobalBuffers (sDecoder* decoder);
  extern void freeLayerBuffers (sDecoder* decoder);

  extern sDataPartition* allocDataPartitions (int n);
  extern void freeDataPartitions (sDataPartition* dataPartitions, int n);

  extern sSlice* allocSlice (sDecoder* decoder);

  // For 4:4:4 independent mode
  extern void changePlaneJV (sDecoder* decoder, int nplane, sSlice *slice);
  extern void makeFramePictureJV (sDecoder* decoder );

  extern sDecodedPic* allocDecodedPicture (sDecodedPic* decodedPic);
  extern void freeDecodedPictures (sDecodedPic* decodedPic);
  extern void clearDecodedPictures (sDecoder* decoder);

  extern void setCoding (sDecoder* decoder);
//{{{
#ifdef __cplusplus
}
#endif
//}}}
