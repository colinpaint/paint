#pragma once
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <cstdint>
#include <string>
#include <vector>
#include <string.h> // for memset

#include "win32.h"
#include "functions.h"
//}}}
//{{{  defines
#define MAX_NUM_SLICES       8
#define MAX_NUM_DECSLICES    16
#define MCBUF_LUMA_PAD_X     32
#define MCBUF_LUMA_PAD_Y     12
#define MCBUF_CHROMA_PAD_X   16
#define MCBUF_CHROMA_PAD_Y   8

#define NUM_BLOCK_TYPES      22
#define BLOCK_SHIFT          2
#define BLOCK_SIZE           4
#define BLOCK_SIZE_8x8       8
#define SMB_BLOCK_SIZE       8
#define BLOCK_PIXELS         16
#define MB_BLOCK_SIZE        16
#define MB_PIXELS            256 // MB_BLOCK_SIZE * MB_BLOCK_SIZE
#define MB_PIXELS_SHIFT      8   // log2(MB_BLOCK_SIZE * MB_BLOCK_SIZE)
#define MB_BLOCK_SHIFT       4
#define BLOCK_MULTIPLE       4   // (MB_BLOCK_SIZE/BLOCK_SIZE)
#define MB_BLOCK_dpS         16  // (BLOCK_MULTIPLE * BLOCK_MULTIPLE)
#define BLOCK_CONTEXT        64  // (4 * MB_BLOCK_dpS)

// These variables relate to the subpel accuracy supported by the software (1/4)
#define BLOCK_SIZE_SP        16  // BLOCK_SIZE << 2
#define BLOCK_SIZE_8x8_SP    32  // BLOCK_SIZE8x8 << 2

// number intra prediction modes
#define NO_INTRA_PMODE       9

// Macro defines
#define Q_BITS           15
#define DQ_BITS          6
#define Q_BITS_8         16
#define DQ_BITS_8        6

// Quantization parameter range
#define MAX_QP           51
#define MAX_PLANE        3
#define INVALIDINDEX     (-135792468)

#define TOTRUN_NUM       15
#define RUNBEFORE_NUM_M1 6

#define MAX_LIST_SIZE 33

// Start code and Emulation Prevention need this to be defined in identical manner at encoder and decoder
#define ZEROBYTES_SHORTSTARTCODE  2 // number of zero bytes in the int16_t start-code prefix
//}}}

//{{{
enum ePixelFormat {
  PF_UNKNOWN = -1, // Unknown color ordering
  UYVY       =  0, // UYVY
  YUY2       =  1, // YUY2
  YUYV       =  1, // YUYV
  YVYU       =  2, // YVYU
  BGR        =  3, // BGR
  V210       =  4  // Video Clarity 422 format (10 bits)
  };
//}}}
//{{{
enum eColorModel {
  CM_UNKNOWN = -1,
  CM_YUV     =  0,
  CM_RGB     =  1,
  CM_XYZ     =  2
  };
//}}}
//{{{
enum eComponentType {
  eLuma =   0,
  eChroma = 1
  };
//}}}
//{{{
enum eColorComponent {
  eLumaComp = 0,
  eCrComp   = 1,
  eCbComp   = 2
  };
//}}}
//{{{
enum eCodingType {
  eFrameCoding       = 0,
  eFieldCoding       = 1,
  eAdaptiveCoding    = 2,
  eFrameMbPairCoding = 3
  };
//}}}

//{{{
enum eRcModelType {
  RC_MODE_0 = 0,
  RC_MODE_1 = 1,
  RC_MODE_2 = 2,
  RC_MODE_3 = 3
  };
//}}}
//{{{
enum eEntropyCodingType {
  eCavlc = 0,
  eCabac = 1
  };
//}}}
//{{{
enum eCabacBlockType {
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
  };
//}}}
//{{{
enum eCavlcBlockType {
  LUMA              =  0,
  LUMA_INTRA16x16DC =  1,
  LUMA_INTRA16x16AC =  2,
  CB                =  3,
  CB_INTRA16x16DC   =  4,
  CB_INTRA16x16AC   =  5,
  CR                =  8,
  CR_INTRA16x16DC   =  9,
  CR_INTRA16x16AC   = 10
  };
//}}}

//{{{
enum ePredListType {
  LIST_0 = 0,
  LIST_1 = 1,
  BI_PRED = 2,
  BI_PRED_L0 = 3,
  BI_PRED_L1 = 4
  };
//}}}
//{{{
enum eI4x4PredMode {
  VERT_PRED            = 0,
  HOR_PRED             = 1,
  DC_PRED              = 2,
  DIAG_DOWN_LEFT_PRED  = 3,
  DIAG_DOWN_RIGHT_PRED = 4,
  VERT_RIGHT_PRED      = 5,
  HOR_DOWN_PRED        = 6,
  VERT_LEFT_PRED       = 7,
  HOR_UP_PRED          = 8
  };
//}}}
//{{{
enum eI8x8PredMode {
  DC_PRED_8     =  0,
  HOR_PRED_8    =  1,
  VERT_PRED_8   =  2,
  PLANE_8       =  3
  };
//}}}
//{{{
enum eI16x16PredMode {
  VERT_PRED_16   = 0,
  HOR_PRED_16    = 1,
  DC_PRED_16     = 2,
  PLANE_16       = 3
  };
//}}}
//{{{
enum eMotionEstimationType {
  eFullPel,
  eHalfPel,
  eQuarterPel
  };
//}}}
//{{{
enum eMvPredType {
  MVPRED_MEDIAN   = 0,
  MVPRED_L        = 1,
  MVPRED_U        = 2,
  MVPRED_UR       = 3
  };
//}}}
//{{{
enum eWeightedPredictionType {
  WP_MCPREC_PLUS0 =       4,
  WP_MCPREC_PLUS1 =       5,
  WP_MCPREC_MINUS0 =      6,
  WP_MCPREC_MINUS1 =      7,
  WP_MCPREC_MINUS_PLUS0 = 8,
  WP_REGULAR =            9
  };
//}}}

//{{{
enum eDataPartitionType {
  eDataPartition1, // no dataPartiton
  eDataPartition3  // 3 dataPartitions
  };
//}}}
//{{{
enum eStartEnd {
  eEOS = 1, // End Of Sequence
  eSOP = 2, // Start Of Picture
  eSOS = 3, // Start Of cSlice
  };
//}}}
//{{{
enum eDecodeResult {
  eDecodingOk     = 0,
  eSearchSync     = 1,
  ePictureDecoded = 2
  };
//}}}

#include "nalu.h"
#include "cSps.h"
#include "cPps.h"
#include "cCabacDecode.h"
#include "cBitStream.h"
#include "cSlice.h"
#include "cFrameStore.h"
#include "sPicture.h"
#include "cDpb.h"

struct sMacroBlock;
//{{{
struct sDataPartition {
  // this are really dataPartitionArray routines
  static const int MAX_CODED_FRAME_SIZE = 2000000;  // bytes for one frame
  //{{{
  static sDataPartition* allocDataPartitions (int numPartitions) {

    sDataPartition* dataPartitions = (sDataPartition*)malloc (numPartitions * sizeof(sDataPartition));
    for (int i = 0; i < numPartitions; ++i)
      dataPartitions[i].bitStream.bitStreamBuffer = (uint8_t*)malloc (MAX_CODED_FRAME_SIZE);

    return dataPartitions;
    }
  //}}}
  //{{{
  static void freeDataPartitions (sDataPartition* dataPartitions, int numPartitions) {

    for (int i = 0; i < numPartitions; ++i)
      free (dataPartitions[i].bitStream.bitStreamBuffer);

    free (dataPartitions);
    }
  //}}}

  cBitStream   bitStream;
  cCabacDecode cabacDecode;

  int (*readSyntaxElement) (sMacroBlock*, sSyntaxElement*, sDataPartition*);
  };
//}}}
//{{{
struct sConcealNode {
  sPicture* picture;
  int       missingpocs;
  sConcealNode* next;
  };
//}}}
//{{{  struct sMotionContexts
#define NUM_MB_TYPE_CTX        11
#define NUM_B8_TYPE_CTX        9
#define NUM_MV_RES_CTX         10
#define NUM_REF_NO_CTX         6
#define NUM_DELTA_QP_CTX       4
#define NUM_MB_AFF_CTX         4
#define NUM_TRANSFORM_SIZE_CTX 3

struct sMotionContexts {
  sBiContext mbTypeContexts[3][NUM_MB_TYPE_CTX];
  sBiContext b8TypeContexts[2][NUM_B8_TYPE_CTX];
  sBiContext mvResContexts[2][NUM_MV_RES_CTX];
  sBiContext refNoContexts[2][NUM_REF_NO_CTX];
  sBiContext deltaQpContexts[NUM_DELTA_QP_CTX];
  sBiContext mbAffContexts[NUM_MB_AFF_CTX];
  };
//}}}
//{{{  struct sTextureContexts
#define NUM_IPR_CTX   2
#define NUM_CIPR_CTX  4
#define NUM_CBP_CTX   4
#define NUM_BCBP_CTX  4
#define NUM_MAP_CTX   15
#define NUM_LAST_CTX  15
#define NUM_ONE_CTX   5
#define NUM_ABS_CTX   5

struct sTextureContexts {
  sBiContext transformSizeContexts[NUM_TRANSFORM_SIZE_CTX];
  sBiContext iprContexts[NUM_IPR_CTX];
  sBiContext ciprContexts[NUM_CIPR_CTX];
  sBiContext cbpContexts[3][NUM_CBP_CTX];
  sBiContext bcbpContexts[NUM_BLOCK_TYPES][NUM_BCBP_CTX];
  sBiContext mapContexts[2][NUM_BLOCK_TYPES][NUM_MAP_CTX];
  sBiContext lastContexts[2][NUM_BLOCK_TYPES][NUM_LAST_CTX];
  sBiContext oneContexts[NUM_BLOCK_TYPES][NUM_ONE_CTX];
  sBiContext absContexts[NUM_BLOCK_TYPES][NUM_ABS_CTX];
  };
//}}}
//{{{
struct sBlockPos {
  int16_t x;
  int16_t y;
  };
//}}}
//{{{
struct sPixelPos {
  int   ok;
  int   mbIndex;
  int16_t x;
  int16_t y;
  int16_t posX;
  int16_t posY;
  };
//}}}
//{{{
struct sCodedBlockPattern {
  int64_t blk;
  int64_t bits;
  int64_t bits8x8;
  };
//}}}
//{{{
struct sMacroBlock {
  void setReadCompCavlc();
  void setReadCompCabac();

  cDecoder264* decoder;
  cSlice*   slice;

  int     mbIndexX;
  int     mbIndexA;
  int     mbIndexB;
  int     mbIndexC;
  int     mbIndexD;

  bool    mbAvailA;
  bool    mbAvailB;
  bool    mbAvailC;
  bool    mbAvailD;

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

  bool    isLossless;
  bool    isIntraBlock;
  bool    isVblock;
  int     DeblockCall;

  int16_t sliceNum;
  char    errorFlag;            // error indicator flag that enables conceal
  char    dplFlag;           // error indicator flag that signals a missing data dataPartition
  int16_t deltaQuant;        // for rate control
  int16_t listOffset;

  sMacroBlock* mbCabacUp;   // pointer to neighboring MB (eCabac)
  sMacroBlock* mbCabacLeft; // pointer to neighboring MB (eCabac)

  sMacroBlock* mbUp;    // neighbors for loopfilter
  sMacroBlock* mbLeft;  // neighbors for loopfilter

  // some storage of macroBlock syntax elements for global access
  int16_t mbType;
  // indices correspond to [forw,backw][blockY][blockX][x,y]
  int16_t mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];
  int     codedBlockPattern;
  sCodedBlockPattern cbp[3];

  int     i16mode;
  char    b8mode[4];
  char    b8pdir[4];
  char    dpcmMode;
  char    chromaPredMode;       // chroma intra prediction mode
  char    skipFlag;

  int16_t deblockFilterDisableIdc;
  int16_t deblockFilterC0Offset;
  int16_t deblockFilterBetaOffset;

  bool    mbField;

  // Flag for MBAFF deblocking;
  uint8_t mixedModeEdgeFlag;

  // deblocking strength indices
  uint8_t strengthV[4][4];
  uint8_t strengthH[4][16];

  bool    lumaTransformSize8x8flag;
  bool    noMbPartLessThan8x8Flag;

  // c style virtual functions
  void (*iTrans4x4) (sMacroBlock*, eColorPlane, int, int);
  void (*iTrans8x8) (sMacroBlock*, eColorPlane, int, int);
  void (*GetMVPredictor) (sMacroBlock*, sPixelPos*, sMotionVec*, int16_t, sPicMotion**, int, int, int, int, int);
  int  (*readCbp) (sMacroBlock*, cCabacDecode*, int);
  char (*readRefPictureIndex) (sMacroBlock*, sSyntaxElement*, sDataPartition*, char, int);
  void (*readCompCoef4x4cabac) (sMacroBlock*, sSyntaxElement*, eColorPlane, int(*)[4], int, int);
  void (*readCompCoef8x8cabac) (sMacroBlock*, sSyntaxElement*, eColorPlane);
  void (*readCompCoef4x4cavlc) (sMacroBlock*, eColorPlane, int(*)[4], int, int, uint8_t**);
  void (*readCompCoef8x8cavlc) (sMacroBlock*, eColorPlane, int(*)[8], int, int, uint8_t**);
  };
//}}}
//{{{
struct sDecodedPic {
  bool ok;

  int yuvFormat; // 4:0:0, 4:2:0, 4:2:2, 4:4:4
  int bitDepth;

  int width;
  int height;
  int yStride;
  int uvStride;

  int bufSize;
  uint8_t* yBuf;
  uint8_t* uBuf;
  uint8_t* vBuf;

  int poc;

  sDecodedPic* next;
  };
//}}}
//{{{
struct sDecodedRefPicMark {
  int memManagement;
  int diffPicNumMinus1;

  int longTermPicNum;
  int longTermFrameIndex;
  int maxLongTermFrameIndexPlus1;

  sDecodedRefPicMark* next;
  };
//}}}
//{{{
class sOldSlice {
public:
  uint32_t fieldPic = 0;
  uint32_t frameNum = INT_MAX;

  int      nalRefIdc = INT_MAX;
  uint32_t picOrderCountLsb = UINT_MAX;

  int      deltaPicOrderCountBot = INT_MAX;
  int      deltaPicOrderCount[2] = {INT_MAX};

  int      botField = 0;
  int      isIDR = false;
  int      idrPicId = 0;
  int      ppsId = INT_MAX;
  };
//}}}

//{{{
struct sCoding {
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
  int16_t bitDepthLuma;
  int16_t bitDepthChroma;
  int bitDepthScale[2];
  int bitDepthLumaQpScale;
  int bitDepthChromaQpScale;
  int maxPelValueComp[MAX_PLANE];          // max value that one picture element (pixel) can take (depends on pic_unit_bitDepth)
  uint32_t dcPredValueComp[MAX_PLANE]; // component value for DC prediction (depends on component pel bit depth)

  int numUvBlocks;
  int numCdcCoeff;
  int numBlock8x8uv;
  int useLosslessQpPrime;

  // macroBlocks
  uint32_t picWidthMbs;
  uint32_t picHeightMapUnits;
  uint32_t frameHeightMbs;
  uint32_t frameSizeMbs;

  int mbCrSizeX;
  int mbCrSizeY;
  int mbCrSizeXblock;
  int mbCrSizeYblock;
  int mbCrSize;
  int mbSize[3][2];       // component macroBlock dimensions
  int mbSizeBlock[3][2];  // component macroBlock dimensions
  int mbSizeShift[3][2];

  int maxVmvR;            // maximum vertical motion vector range in lumaQuarterFrame pixel units
  int maxFrameNum;

  int subpelX;
  int subpelY;
  int shiftpelX;
  int shiftpelY;
  int totalScale;
  };
//}}}
//{{{
struct sFrameFormat {
  eYuvFormat   yuvFormat;       // YUV format (0=4:0:0, 1=4:2:0, 2=4:2:2, 3=4:4:4)
  eColorModel  colourModel;     // 4:4:4 format (0: YUV, 1: RGB, 2: XYZ)
  ePixelFormat pixelFormat;     // pixel format support for certain interleaved yuv sources

  int          width[3];        // component frame width
  int          height[3];       // component frame height

  int          mbWidth;         // luma component frame width
  int          mbHeight;        // luma component frame height

  int          sizeCmp[3];      // component sizes (width * height)
  int          bitDepth[3];     // component bit depth
  };
//}}}
//{{{
struct sParam {
  int naluDebug;
  int vlcDebug;
  int sliceDebug;
  int spsDebug;
  int ppsDebug;
  int seiDebug;
  int dpbDebug;
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
  };
//}}}
//{{{
struct sDebug {
  TIME_T      startTime;
  TIME_T      endTime;

  std::string profileString;

  std::vector <std::string> dpbStrings;

  eSliceType  sliceType;
  std::string sliceTypeString;
  std::string sliceString;

  eSliceType  outSliceType;
  std::string outString;
  };
//}}}

//{{{
class cDecoder264 {
public:
  //{{{
  enum eDecErrCode {
    DEC_GEN_NOERR = 0,
    DEC_OPEN_NOERR = 0,
    DEC_CLOSE_NOERR = 0,
    DEC_SUCCEED = 0,
    DEC_EOS = 1,
    DEC_NEED_DATA = 2,
    DEC_INVALID_PARAM = 3,
    DEC_ERRMASK = 0x8000
    };
  //}}}

  static cDecoder264* open (sParam* param, uint8_t* chunk, size_t chunkSize);
  static void error (const std::string& text);
  ~cDecoder264();

  void reset_ecFlags();
  int setEcFlag (int se);
  int getConcealElement (sSyntaxElement* se);
  int fmoGetNextMBNr (int CurrentMbNr);

  void decodePOC (cSlice* slice);
  void padPicture (sPicture* picture);
  void changePlaneJV (int nplane, cSlice* slice);

  void directOutput (sPicture* picture);
  void writeStoredFrame (cFrameStore* frameStore);

  sDecodedPic* decodeOneFrame (int& result);
  sDecodedPic* finish();
  void close();

  // static var
  static inline cDecoder264* gDecoder = nullptr;

  //{{{  vars
  sParam       param = {};
  sDebug       debug = {};

  // nalu
  int          gotLastNalu = 0;
  int          naluCount = 0;
  cNalu*       nalu = nullptr;
  cNalu*       pendingNalu = nullptr;
  cAnnexB*     annexB = nullptr;

  // sps
  cSps         sps[kMaxSps];
  cSps*        activeSps = nullptr;

  // pps
  cPps         pps[kMaxPps];
  cPps*        activePps = nullptr;

  int          decodeFrameNum = 0;
  int          idrFrameNum = 0;
  uint32_t     preFrameNum = 0;  // last decoded slice. For detecting gap in frameNum.
  uint32_t     prevFrameNum = 0; // number of previous slice
  int          newFrame = 0;

  bool         nonConformingStream = false;
  int          deblockEnable = 0;
  int          deblockMode = 0;  // 0: deblock in picture, 1: deblock in slice;

  uint32_t     picHeightInMbs = 0;
  uint32_t     picSizeInMbs = 0;

  // slice
  int          picSliceIndex = 0;
  int          numDecodedMbs = 0;
  int          numDecodedSlices = 0;
  int          numAllocatedSlices = 0;
  cSlice**     sliceList = nullptr;
  cSlice*      nextSlice = nullptr; // pointer to first cSlice of next picture;
  sOldSlice*   oldSlice = nullptr;

  int          lastHasMmco5 = 0;

  // dpb
  cDpb         dpb;

  // out
  sPicture*    picture = nullptr;

  cFrameStore* outBuffer = nullptr;
  sPicture*    pendingOut = nullptr;
  int          pendingOutPicStructure;

  sDecodedPic* outDecodedPics = nullptr;

  // conceal
  cFrameStore* lastOutFrameStore = nullptr;

  // sCoding
  sCoding      coding = {0};

  // sCoding duplicates
  int          width = 0;
  int          height = 0;
  int          widthCr = 0;
  int          heightCr = 0;
  int          mbCrSizeX = 0;
  int          mbCrSizeY = 0;
  int          mbCrSizeXblock = 0;
  int          mbCrSizeYblock = 0;
  int          mbCrSize = 0;
  int          mbSize[3][2] = {};
  int          mbSizeBlock[3][2] = {};
  int          mbSizeShift[3][2] = {};
  int16_t      bitDepthLuma = 0;
  int16_t      bitDepthChroma = 0;

  sBlockPos*   picPos = nullptr;
  uint8_t****  nzCoeff = nullptr;
  sMacroBlock* mbData = nullptr;     // array containing all MBs of a whole frame
  char*        intraBlock = nullptr;
  uint8_t**    predMode = nullptr;   // prediction type [90][74]
  int**        siBlock = nullptr;

  // poc
  int          lastRefPicPoc = 0;

  // - mode 0:
  signed int   prevPocMsb = 0;
  uint32_t     prevPocLsb = 0;
  int          lastPicBotField = 0;

  // - mode 1:
  signed int   expectedPOC = 0;
  signed int   pocCycleCount = 0;
  signed int   frameNumPocCycle = 0;
  uint32_t     previousFrameNum = 0;
  uint32_t     frameNumOffset = 0;
  int          expectedDeltaPerPocCycle = 0;
  int          thisPoc = 0;
  int          previousFrameNumOffset = 0;

  int          noOutputPriorPicFlag = 0;

  // non-zero: i-th previous frame is correct
  int           isPrimaryOk = 0;    // if primary frame is correct, 0: incorrect
  int           isRedundantOk = 0;  // if redundant frame is correct, 0:incorrect

  int*          qpPerMatrix = nullptr;
  int*          qpRemMatrix = nullptr;

  // Error parameters
  struct ObjectBuffer* ercObjectList = nullptr;
  struct ErcVariables* ercErrorVar = nullptr;
  int           ercMvPerMb = 0;
  int           ecFlag[SE_MAX_ELEMENTS] = {0};  // array to set errorconcealment

  // fmo
  int*          mbToSliceGroupMap = nullptr;
  int*          mapUnitToSliceGroupMap = nullptr;
  int           sliceGroupsNum = 0;  // the number of slice groups -1 (0 == scan order, 7 == maximum)

  // recovery
  int           recoveryPoint = 0;
  int           recoveryPointFound = 0;
  int           recoveryFrameCount = 0;
  int           recoveryFrameNum = 0;
  int           recoveryPoc = 0;
  int           recoveryFlag = 0;

  // picture error conceal
  // concealHead points to first node in list, concealTail points to last node in list
  // Initialize both to NULL, meaning no nodes in list yet
  sConcealNode* concealHead = nullptr;
  sConcealNode* concealTail = nullptr;
  int           concealMode = 0;
  int           earlierMissingPoc = 0;
  uint32_t      concealFrame = 0;
  int           idrConcealFlag = 0;
  int           concealSliceType = 0;

  sPicture*     decPictureJV[MAX_PLANE] = {nullptr};  // picture to be used during 4:4:4 independent mode decoding
  sMacroBlock*  mbDataJV[MAX_PLANE] = {nullptr};
  char*         intraBlockJV[MAX_PLANE] = {nullptr};
  uint8_t**     predModeJV[MAX_PLANE] = {nullptr};
  int**         siBlockJV[MAX_PLANE] = {nullptr};
  //}}}
  //{{{  C style virtual functions
  void (*getNeighbour) (sMacroBlock*, int, int, int[2], sPixelPos*);
  void (*getMbBlockPos) (sBlockPos*, int, int16_t*, int16_t*);
  void (*getStrengthV) (sMacroBlock*, int, int, sPicture*);
  void (*getStrengthH) (sMacroBlock*, int, int, sPicture*);
  void (*edgeLoopLumaV) (eColorPlane, sPixel**, uint8_t*, sMacroBlock*, int);
  void (*edgeLoopLumaH) (eColorPlane, sPixel**, uint8_t*, sMacroBlock*, int, sPicture*);
  void (*edgeLoopChromaV) (sPixel**, uint8_t*, sMacroBlock*, int, int, sPicture*);
  void (*edgeLoopChromaH) (sPixel**, uint8_t*, sMacroBlock*, int, int, sPicture*);
  //}}}

private:
  void clearDecodedPics();
  void initPictureDecode();
  void initGlobalBuffers();
  void freeGlobalBuffers();
  void freeLayerBuffers();
  void mbAffPostProc();

  // slice
  int readNalu (cSlice* slice);
  void readSliceHeader (cBitStream& bitStream, cSlice* slice);
  void decodeSlice (cSlice* slice);
  int decodeFrame();
  void endDecodeFrame();

  //{{{  fmo
  void initFmo (cSlice* slice);
  void closeFmo();

  int fmoGetNumberOfSliceGroup();
  int fmoGetLastMBOfPicture();
  int fmoGetLastMBInSliceGroup (int SliceGroup);
  int fmoGetSliceGroupId (int mb);

  void fmoGenerateType0MapUnitMap (uint32_t PicSizeInMapUnits);
  void fmoGenerateType1MapUnitMap (uint32_t PicSizeInMapUnits);
  void fmoGenerateType2MapUnitMap (uint32_t PicSizeInMapUnits);
  void fmoGenerateType3MapUnitMap (uint32_t PicSizeInMapUnits, cSlice* slice);
  void fmoGenerateType4MapUnitMap (uint32_t PicSizeInMapUnits, cSlice* slice);
  void fmoGenerateType5MapUnitMap (uint32_t PicSizeInMapUnits, cSlice* slice);
  void fmoGenerateType6MapUnitMap (uint32_t PicSizeInMapUnits);
  int fmoGenerateMapUnitToSliceGroupMap (cSlice* slice);
  int fmoGenerateMbToSliceGroupMap (cSlice* slice);
  //}}}

  void setCoding();
  void setCodingParam (cSps* sps);
  void setFormat (cSps* sps, sFrameFormat* source, sFrameFormat* output);

  void allocDecodedPicBuffers (sDecodedPic* decodedPic, sPicture* picture,
                               int lumaSize, int frameSize, int lumaSizeX, int lumaSizeY,
                               int chromaSizeX, int chromaSizeY);
  void clearPicture (sPicture* picture);
  void writeOutPicture (sPicture* picture);
  void flushPendingOut();
  void writePicture (sPicture* picture, int realStructure);
  void writeUnpairedField (cFrameStore* frameStore);
  void flushDirectOutput();

  bool isNewPicture (sPicture* picture, cSlice* slice, sOldSlice* oldSlice);
  void readDecRefPicMarking (cBitStream& s, cSlice* slice);
  void initPicture (cSlice* slice);
  void initRefPicture (cSlice* slice);
  void initSlice (cSlice* slice);
  void reorderLists (cSlice* slice);
  void copySliceInfo (cSlice* slice, sOldSlice* oldSlice);
  void useParameterSet (cSlice* slice);

  void makeFramePictureJV();
  };
//}}}
