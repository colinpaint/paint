#pragma once
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/timeb.h>

//{{{  defines
typedef unsigned char  byte;   // byte type definition
typedef unsigned char  uint8;  // type definition for unsigned char (same as byte, 8 bits)
typedef unsigned short uint16; // type definition for unsigned short (16 bits)
typedef unsigned int   uint32; // type definition for unsigned int (32 bits)

typedef char  int8;
typedef short int16;
typedef int   int32;

typedef byte   sPixel;    // pixel type
typedef uint16 distpel;   // distortion type (for pixels)
typedef int32  distblk;   // distortion type (for sMacroBlock)
typedef int32  transpel;  // transformed coefficient type


#define MAX_NUM_SLICES          8
#define MAX_REFERENCE_PICTURES  32       // H.264 allows 32 fields
#define MAX_CODED_FRAME_SIZE    8000000  // bytes for one frame
#define MAX_NUM_DECSLICES       16
#define MCBUF_LUMA_PAD_X        32
#define MCBUF_LUMA_PAD_Y        12
#define MCBUF_CHROMA_PAD_X      16
#define MCBUF_CHROMA_PAD_Y      8

#define NUM_BLOCK_TYPES        22

#define BLOCK_SHIFT            2
#define BLOCK_SIZE             4
#define BLOCK_SIZE_8x8         8
#define SMB_BLOCK_SIZE         8
#define BLOCK_PIXELS          16
#define MB_BLOCK_SIZE         16
#define MB_PIXELS            256  // MB_BLOCK_SIZE * MB_BLOCK_SIZE
#define MB_PIXELS_SHIFT        8  // log2(MB_BLOCK_SIZE * MB_BLOCK_SIZE)
#define MB_BLOCK_SHIFT         4
#define BLOCK_MULTIPLE         4  // (MB_BLOCK_SIZE/BLOCK_SIZE)
#define MB_BLOCK_dpS          16  // (BLOCK_MULTIPLE * BLOCK_MULTIPLE)
#define BLOCK_CONTEXT         64  // (4 * MB_BLOCK_dpS)

// These variables relate to the subpel accuracy supported by the software (1/4)
#define BLOCK_SIZE_SP         16  // BLOCK_SIZE << 2
#define BLOCK_SIZE_8x8_SP     32  // BLOCK_SIZE8x8 << 2

// number of intra prediction modes
#define NO_INTRA_PMODE   9

// Macro defines
#define Q_BITS           15
#define DQ_BITS          6
#define Q_BITS_8         16
#define DQ_BITS_8        6

#define TOTRUN_NUM       15
#define RUNBEFORE_NUM    7
#define RUNBEFORE_NUM_M1 6

// Quantization parameter range
#define MAX_QP           51

#define MAX_PLANE        3
#define INVALIDINDEX     (-135792468)

// Start code and Emulation Prevention need this to be defined in identical manner at encoder and decoder
#define ZEROBYTES_SHORTSTARTCODE  2 //indicates the number of zero bytes in the short start-code prefix
//}}}

#include "win32.h"
#include "frame.h"
#include "nalu.h"
#include "sps.h"
#include "pps.h"

//{{{  enum eMBModeType
typedef enum {
  PSKIP        =  0,
  BSKIP_DIRECT =  0,
  P16x16       =  1,
  P16x8        =  2,
  P8x16        =  3,
  SMB8x8       =  4,
  SMB8x4       =  5,
  SMB4x8       =  6,
  SMB4x4       =  7,
  P8x8         =  8,
  I4MB         =  9,
  I16MB        = 10,
  IBLOCK       = 11,
  SI4MB        = 12,
  I8MB         = 13,
  IPCM         = 14,
  MAXMODE      = 15
  } eMBModeType;
//}}}
#include "functions.h"
//}}}

//{{{  enum eProfileIDC
typedef enum {
  NO_PROFILE     = 0,   // disable profile checking for experimental coding (enables FRExt, but disables MV)
  FREXT_CAVLC444 = 44,  // YUV 4:4:4/14 "eCavlc 4:4:4"
  BASELINE       = 66,  // YUV 4:2:0/8  "Baseline"
  MAIN           = 77,  // YUV 4:2:0/8  "Main"
  EXTENDED       = 88,  // YUV 4:2:0/8  "Extended"
  FREXT_HP       = 100, // YUV 4:2:0/8  "High"
  FREXT_Hi10P    = 110, // YUV 4:2:0/10 "High 10"
  FREXT_Hi422    = 122, // YUV 4:2:2/10 "High 4:2:2"
  FREXT_Hi444    = 244, // YUV 4:4:4/14 "High 4:4:4"
  } eAvcProfileIDC;
//}}}
//{{{  enum eStartEnd
typedef enum {
  eEOS = 1, // End Of Sequence
  eSOP = 2, // Start Of Picture
  eSOS = 3, // Start Of sSlice
  } eStartEnd;
//}}}
//{{{  enum eDecodeResult
typedef enum {
  eDecodingOk     = 0,
  eSearchSync     = 1,
  ePictureDecoded = 2
  } eDecodeResult;
//}}}
//{{{  enum eColorPlane
typedef enum {
  // YUV
  PLANE_Y = 0,  // PLANE_Y
  PLANE_U = 1,  // PLANE_Cb
  PLANE_V = 2,  // PLANE_Cr
  } eColorPlane;
//}}}
//{{{  enum eComponentType
typedef enum {
  eLuma =   0,
  eChroma = 1
  } eComponentType;
//}}}
//{{{  enum eColorComponent
typedef enum {
  eLumaComp = 0,
  eCrComp   = 1,
  eCbComp   = 2
  } eColorComponent;
//}}}
//{{{  enum ePicStructure
typedef enum {
  eFrame    = 0,
  eTopField = 1,
  eBotField = 2
  } ePicStructure;
//}}}
//{{{  enum eDataPartitionType
typedef enum {
  eDataPartition1, // no dataPartiton
  eDataPartition3  // 3 dataPartitions
  } eDataPartitionType;
//}}}
//{{{  enum eSyntaxElementType
// almost the same as syntaxElements.h but not quite
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
//{{{  enum eEntropyCodingType
typedef enum {
  eCavlc = 0,
  eCabac = 1
  } eEntropyCodingType;
//}}}
//{{{  enum eCodingType
typedef enum {
  eFrameCoding       = 0,
  eFieldCoding       = 1,
  eAdaptiveCoding    = 2,
  eFrameMbPairCoding = 3
 } eCodingType;
//}}}
//{{{  enum ePredListType
typedef enum {
  LIST_0 = 0,
  LIST_1 = 1,
  BI_PRED = 2,
  BI_PRED_L0 = 3,
  BI_PRED_L1 = 4
  } ePredListType;
//}}}
//{{{  enum eSliceType
typedef enum {
  eSliceP        = 0,
  eSliceB        = 1,
  eSliceI        = 2,
  eSliceSP       = 3,
  eSliceSI       = 4,
  eSliceNumTypes = 5
  } eSliceType;
//}}}
//{{{  enum eMotionEstimationType
typedef enum {
  eFullPel,
  eHalfPel,
  eQuarterPel
  } eMotionEstimationType;
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
//{{{  enum eCavlcBlockType
typedef enum {
  LUMA              =  0,
  LUMA_INTRA16x16DC =  1,
  LUMA_INTRA16x16AC =  2,
  CB                =  3,
  CB_INTRA16x16DC   =  4,
  CB_INTRA16x16AC   =  5,
  CR                =  8,
  CR_INTRA16x16DC   =  9,
  CR_INTRA16x16AC   = 10
  } eCavlcBlockType;
//}}}
//{{{  enum eCabacBlockType
typedef enum {
  LUMA_16DC     =   0,
  LUMA_16AC     =   1,
  LUMA_8x8      =   2,
  LUMA_8x4      =   3,
  LUMA_4x8      =   4,
  LUMA_4x4      =   5,
  CHROMA_DC     =   6,
  CHROMA_AC     =   7,
  CHROMA_DC_2x4 =   8,
  CHROMA_DC_4x4 =   9,
  CB_16DC       =  10,
  CB_16AC       =  11,
  CB_8x8        =  12,
  CB_8x4        =  13,
  CB_4x8        =  14,
  CB_4x4        =  15,
  CR_16DC       =  16,
  CR_16AC       =  17,
  CR_8x8        =  18,
  CR_8x4        =  19,
  CR_4x8        =  20,
  CR_4x4        =  21
  } eCabacBlockType;
//}}}
//{{{  enum eI4x4PredMode
typedef enum {
  VERT_PRED            = 0,
  HOR_PRED             = 1,
  DC_PRED              = 2,
  DIAG_DOWN_LEFT_PRED  = 3,
  DIAG_DOWN_RIGHT_PRED = 4,
  VERT_RIGHT_PRED      = 5,
  HOR_DOWN_PRED        = 6,
  VERT_LEFT_PRED       = 7,
  HOR_UP_PRED          = 8
  } eI4x4PredMode;
//}}}
//{{{  enum eI8x8PredMode
typedef enum {
  DC_PRED_8     =  0,
  HOR_PRED_8    =  1,
  VERT_PRED_8   =  2,
  PLANE_8       =  3
  } eI8x8PredMode;
//}}}
//{{{  enum eI16x16PredMode
typedef enum {
  VERT_PRED_16   = 0,
  HOR_PRED_16    = 1,
  DC_PRED_16     = 2,
  PLANE_16       = 3
  } eI16x16PredMode;
//}}}
//{{{  enum eMvPredType
typedef enum {
  MVPRED_MEDIAN   = 0,
  MVPRED_L        = 1,
  MVPRED_U        = 2,
  MVPRED_UR       = 3
  } eMvPredType;
//}}}

//{{{  sBiContext
typedef struct {
  uint16        state; // index into state-table CP
  unsigned char MPS;   // least probable symbol 0/1 CP
  unsigned char dummy; // for alignment
  } sBiContext;
//}}}
//{{{  sMotionContexts
#define NUM_MB_TYPE_CTX        11
#define NUM_B8_TYPE_CTX        9
#define NUM_MV_RES_CTX         10
#define NUM_REF_NO_CTX         6
#define NUM_DELTA_QP_CTX       4
#define NUM_MB_AFF_CTX         4
#define NUM_TRANSFORM_SIZE_CTX 3

typedef struct {
  sBiContext mbTypeContexts[3][NUM_MB_TYPE_CTX];
  sBiContext b8TypeContexts[2][NUM_B8_TYPE_CTX];
  sBiContext mvResContexts[2][NUM_MV_RES_CTX];
  sBiContext refNoContexts[2][NUM_REF_NO_CTX];
  sBiContext deltaQpContexts[NUM_DELTA_QP_CTX];
  sBiContext mbAffContexts[NUM_MB_AFF_CTX];
  } sMotionContexts;
//}}}
//{{{  sTextureContexts
#define NUM_IPR_CTX   2
#define NUM_CIPR_CTX  4
#define NUM_CBP_CTX   4
#define NUM_BCBP_CTX  4
#define NUM_MAP_CTX   15
#define NUM_LAST_CTX  15
#define NUM_ONE_CTX   5
#define NUM_ABS_CTX   5

typedef struct {
  sBiContext transformSizeContexts[NUM_TRANSFORM_SIZE_CTX];
  sBiContext iprContexts[NUM_IPR_CTX];
  sBiContext ciprContexts[NUM_CIPR_CTX];
  sBiContext cbpContexts[3][NUM_CBP_CTX];
  sBiContext bcbpContexts[NUM_BLOCK_TYPES][NUM_BCBP_CTX];
  sBiContext mapContexts[2][NUM_BLOCK_TYPES][NUM_MAP_CTX];
  sBiContext lastContexts[2][NUM_BLOCK_TYPES][NUM_LAST_CTX];
  sBiContext oneContexts[NUM_BLOCK_TYPES][NUM_ONE_CTX];
  sBiContext absContexts[NUM_BLOCK_TYPES][NUM_ABS_CTX];
  } sTextureContexts;
//}}}
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
//{{{  sCodedBlockPattern
typedef struct  {
  int64 blk;
  int64 bits;
  int64 bits8x8;
  } sCodedBlockPattern;
//}}}

struct Picture;
struct PicMotion;
struct MacroBlock;
//{{{  sBitStream
typedef struct {
  // cavlc Decoding
  byte* bitStreamBuffer; // codebuffer for read bytes
  int   bitStreamOffset; // position in the codebuffer, bit-oriented
  int   bitStreamLen;    // over codebuffer length, byte oriented
  int   errorFlag;       // error, 0: no error, else unspecified error

  // cabac Decoding
  int   readLen;         // position in the codebuffer
  int   codeLen;         // overall codebuffer length
  } sBitStream;
//}}}
//{{{  sCabacDecodeEnv
typedef struct {
  unsigned int range;
  unsigned int value;
  int          bitsLeft;
  byte*        codeStream;
  int*         codeStreamLen;
  } sCabacDecodeEnv;
//}}}
//{{{  sSyntaxElement
typedef struct SyntaxElement {
  int          type;        // type of syntax element for data part.
  int          value1;      // numerical value of syntax element
  int          value2;      // for blocked symbols, e.g. run/level
  int          len;         // length of code
  int          inf;         // info part of eCavlc code
  unsigned int bitpattern;  // cavlc bitpattern
  int          context;     // cabac context
  int          k;           // cabac context for coeff_count,uv

  // eCavlc mapping to syntaxElement
  void (*mapping) (int, int, int*, int*);

  // eCabac actual coding method of each individual syntax element type
  void (*reading) (struct MacroBlock*, struct SyntaxElement*, sCabacDecodeEnv*);
  } sSyntaxElement;
//}}}
//{{{  sDataPartition
typedef struct DataPartition {
  sBitStream* stream;
  sCabacDecodeEnv cabacDecodeEnv;

  int (*readSyntaxElement) (struct MacroBlock*, struct SyntaxElement*, struct DataPartition*);
  } sDataPartition;
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

  bool mbAvailA;
  bool mbAvailB;
  bool mbAvailC;
  bool mbAvailD;

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

  bool isLossless;
  bool isIntraBlock;
  bool isVblock;
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
  int     codedBlockPattern;
  sCodedBlockPattern codedBlockPatterns[3];

  int   i16mode;
  char  b8mode[4];
  char  b8pdir[4];
  char  dpcmMode;
  char  chromaPredMode;       // chroma intra prediction mode
  char  skipFlag;
  short deblockFilterDisableIdc;
  short deblockFilterC0Offset;
  short deblockFilterBetaOffset;

  bool mbField;

  // Flag for MBAFF deblocking;
  byte  mixedModeEdgeFlag;

  // deblocking strength indices
  byte strengthV[4][4];
  byte strengthH[4][16];

  bool lumaTransformSize8x8flag;
  bool noMbPartLessThan8x8Flag;

  // virtual methods
  void (*iTrans4x4) (struct MacroBlock*, eColorPlane, int, int);
  void (*iTrans8x8) (struct MacroBlock*, eColorPlane, int, int);
  void (*GetMVPredictor) (struct MacroBlock*, sPixelPos*, sMotionVec*, short, struct PicMotion**, int, int, int, int, int);
  int  (*readStoreCBPblockBit) (struct MacroBlock*, sCabacDecodeEnv*, int);
  char (*readRefPictureIndex) (struct MacroBlock*, struct SyntaxElement*, struct DataPartition*, char, int);
  void (*readCompCoef4x4cabac) (struct MacroBlock*, struct SyntaxElement*, eColorPlane, int(*)[4], int, int);
  void (*readCompCoef8x8cabac) (struct MacroBlock*, struct SyntaxElement*, eColorPlane);
  void (*readCompCoef4x4cavlc) (struct MacroBlock*, eColorPlane, int(*)[4], int, int, byte**);
  void (*readCompCoef8x8cavlc) (struct MacroBlock*, eColorPlane, int(*)[8], int, int, byte**);
  } sMacroBlock;
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
  int ok;

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
//{{{  sDecodedRefPicMark
typedef struct DecodedRefPicMark {
  int memManagement;
  int diffPicNumMinus1;

  int longTermPicNum;
  int longTermFrameIndex;
  int maxLongTermFrameIndexPlus1;

  struct DecodedRefPicMark* next;
  } sDecodedRefPicMark;
//}}}
//{{{  sOldSlice
typedef struct {
  unsigned fieldPic;
  unsigned frameNum;

  int      nalRefIdc;
  unsigned picOrderCountLsb;

  int      deltaPicOrderCountBot;
  int      deltaPicOrderCount[2];

  int      botField;
  int      isIDR;
  int      idrPicId;
  int      ppsId;
  } sOldSlice;
//}}}
//{{{  sSlice
typedef struct Slice {
  struct Decoder* decoder;

  sPps* activePps;
  sSps* activeSps;
  struct DPB* dpb;

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

  int           cabacInitIdc;     // cabac model number
  unsigned int  frameNum;

  ePicStructure picStructure;
  unsigned int  fieldPic;
  byte          botField;
  int           startMbNum;   // MUST be set by NAL even in case of errorFlag == 1
  int           endMbNumPlus1;
  int           maxDataPartitions;
  int           dataPartitionMode;
  int           curHeader;
  int           nextHeader;
  int           lastDquant;

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
  struct Picture** listX[6];

  sDataPartition*       dataPartitions; // array of dataPartition
  sMotionContexts*  motionInfoContexts;  // pointer to struct of context models for use in eCabac
  sTextureContexts* textureInfoContexts; // pointer to struct of context models for use in eCabac

  int   mvscale[6][MAX_REFERENCE_PICTURES];
  int   refPicReorderFlag[2];
  int*  modPicNumsIdc[2];
  int*  absDiffPicNumMinus1[2];
  int*  longTermPicIndex[2];

  short deblockFilterDisableIdc; // Disable deblocking filter on slice
  short deblockFilterC0Offset;   // Alpha and C0 offset for filtering slice
  short deblockFilterBetaOffset; // Beta offset for filtering slice

  int   ppsId;             // ID of picture parameter set the slice is referring to
  int   noDataPartitionB;  // non-zero, if data dataPartition B is lost
  int   noDataPartitionC;  // non-zero, if data dataPartition C is lost

  bool   isResetCoef;
  bool   isResetCoefCr;
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

  // weighted pred
  unsigned short hasWeightedPred;
  unsigned short weightedBiPredIdc;
  unsigned short lumaLog2weightDenom;
  unsigned short chromaLog2weightDenom;
  int***         weightedPredWeight;   // weight in [list][index][component] order
  int***         weightedPredOffset;   // offset in [list][index][component] order
  int****        weightedBiPredWeight;  // weight in [list][fw_index][bw_index][component] order
  short          wpRoundLuma;
  short          wpRoundChroma;

  // for signalling to the neighbour logic that this is a deblocker call
  int            maxMbVmvR;   // maximum vertical motion vector range in luma quarter pixel units for the current levelIdc
  int            refFlag[17]; // 0: i-th previous frame is incorrect

  int             ercMvPerMb;
  sMacroBlock*    mbData;
  struct Picture* picture;

  int**           siBlock;
  byte**          predMode;
  char*           intraBlock;
  char            chromaVectorAdjust[6][32];

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
typedef struct {
  int profileIdc;

  ePicStructure picStructure;
  int yuvFormat;
  int isSeperateColourPlane;
  eSliceType sliceType;

  // size
  int width;
  int height;
  int widthCr;
  int heightCr;

  int lumaPadX;
  int lumaPadY;
  int chromaPadX;
  int chromaPadY;

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
  int useLosslessQpPrime;

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
//{{{  sDebug
typedef struct {
  TIME_T  startTime;
  TIME_T  endTime;

  char    profileStr[128];

  eSliceType sliceType;
  char    sliceTypeStr[9];
  char    sliceStr[128];

  eSliceType outSliceType;
  char    outStr[128];
  } sDebug;
//}}}
//{{{  sDecoder
typedef struct Decoder {
  sParam       param;
  sDebug       debug;

  // nalu
  int          gotLastNalu;
  int          naluCount;
  sNalu*       nalu;
  sNalu*       pendingNalu;
  sAnnexB*     annexB;

  // sps
  sSps         sps[32];
  sSps*        activeSps;

  // pps
  sPps         pps[MAX_PPS];
  sPps*        activePps;

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
  sOldSlice*   oldSlice;

  // output
  struct DPB*        dpb;
  int                lastHasMmco5;
  int                dpbPoc[100];
  struct Picture*    picture;
  struct Picture*    decPictureJV[MAX_PLANE];  // picture to be used during 4:4:4 independent mode decoding
  struct Picture*    noReferencePicture;       // dummy storable picture for recovery point
  struct FrameStore* lastOutFramestore;
  sDecodedPic*       outDecodedPics;
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
static inline int isHiIntraOnlyProfile (unsigned profileIdc, bool constrainedSet3flag) {
  return (((profileIdc == FREXT_Hi10P) ||
           (profileIdc == FREXT_Hi422) ||
           (profileIdc == FREXT_Hi444)) && constrainedSet3flag) ||
         (profileIdc == FREXT_CAVLC444);
  }
//}}}

extern sDecoder* gDecoder;

extern void error (const char* text);

extern void initGlobalBuffers (sDecoder* decoder);
extern void freeGlobalBuffers (sDecoder* decoder);
extern void freeLayerBuffers (sDecoder* decoder);

extern sDataPartition* allocDataPartitions (int n);
extern void freeDataPartitions (sDataPartition* dataPartitions, int n);

extern sSlice* allocSlice (sDecoder* decoder);

extern sDecodedPic* allocDecodedPicture (sDecodedPic* decodedPic);
extern void clearDecodedPictures (sDecoder* decoder);
extern void freeDecodedPictures (sDecodedPic* decodedPic);

// For 4:4:4 independent mode
extern void changePlaneJV (sDecoder* decoder, int nplane, sSlice *slice);
extern void makeFramePictureJV (sDecoder* decoder );
