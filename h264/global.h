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
#include "types.h"
#include "frame.h"
//}}}
//{{{  defines
#define MAX_PPS  256
#define MAX_NUM_REF_FRAMES_PIC_ORDER  256
#define ET_SIZE 300      //!< size of error text buffer
//}}}
//{{{  enum eDecoderStatus
typedef enum {
  DEC_OPENED = 0,
  DEC_STOPPED,
  } eDecoderStatus;
//}}}
//{{{  enum eColorComponent
typedef enum {
  LumaComp = 0,
  CrComp = 1,
  CbComp = 2
  } eColorComponent;
//}}}

//{{{  sAnnexB
typedef struct AnnexB {
  byte*  buffer;
  size_t bufferSize;
  byte*  bufferPtr;
  size_t bytesInBuffer;

  int    isFirstByteStreamNALU;
  int    nextStartCodeBytes;
  byte*  naluBuffer;
  } sAnnexB;
//}}}
//{{{  enum eNaluType
typedef enum {
  NALU_TYPE_SLICE    = 1,
  NALU_TYPE_DPA      = 2,
  NALU_TYPE_DPB      = 3,
  NALU_TYPE_DPC      = 4,
  NALU_TYPE_IDR      = 5,
  NALU_TYPE_SEI      = 6,
  NALU_TYPE_SPS      = 7,
  NALU_TYPE_PPS      = 8,

  NALU_TYPE_AUD      = 9,
  NALU_TYPE_EOSEQ    = 10,
  NALU_TYPE_EOSTREAM = 11,
  NALU_TYPE_FILL     = 12,
  NALU_TYPE_PREFIX   = 14,
  NALU_TYPE_SUB_SPS  = 15,
  NALU_TYPE_SLC_EXT  = 20,
  NALU_TYPE_VDRD     = 24  // View and Dependency Representation Delimiter NAL Unit
  } eNaluType;
//}}}
//{{{  enum eNalRefIdc
typedef enum {
  NALU_PRIORITY_HIGHEST     = 3,
  NALU_PRIORITY_HIGH        = 2,
  NALU_PRIORITY_LOW         = 1,
  NALU_PRIORITY_DISPOSABLE  = 0
  } eNalRefIdc;
//}}}
//{{{  sNalu
typedef struct Nalu {
  int        startCodeLen; // 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  unsigned   len;          // Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  unsigned   maxSize;      // NAL Unit Buffer size
  int        forbiddenBit; // should be always FALSE
  eNaluType  unitType;     // NALU_TYPE_xxxx
  eNalRefIdc refId;        // NALU_PRIORITY_xxxx
  byte*      buf;          // contains the first byte followed by the EBSP
  uint16     lostPackets;  // true, if packet loss is detected
  } sNalu;
//}}}
//{{{  sHRD
typedef struct {
  unsigned int cpb_cnt_minus1;             // ue(v)
  unsigned int bit_rate_scale;             // u(4)
  unsigned int cpb_size_scale;             // u(4)
  unsigned int bit_rate_value_minus1[32];  // ue(v)
  unsigned int cpb_size_value_minus1[32];  // ue(v)
  unsigned int cbr_flag[32];               // u(1)
  unsigned int initial_cpb_removal_delay_length_minus1;  // u(5)
  unsigned int cpb_removal_delay_length_minus1;          // u(5)
  unsigned int dpb_output_delay_length_minus1;           // u(5)
  unsigned int time_offset_length;                       // u(5)
  } sHRD;
//}}}
//{{{  sVUI
typedef struct {
  Boolean      aspect_ratio_info_present_flag;       // u(1)
  unsigned int aspect_ratio_idc;                     // u(8)

  unsigned short sar_width;                          // u(16)
  unsigned short sar_height;                         // u(16)

  Boolean      overscan_info_present_flag;           // u(1)
  Boolean      overscan_appropriate_flag;            // u(1)

  Boolean      video_signal_type_present_flag;       // u(1)
  unsigned int video_format;                         // u(3)
  Boolean      video_full_range_flag;                // u(1)

  Boolean      colour_description_present_flag;      // u(1)
  unsigned int colour_primaries;                     // u(8)
  unsigned int transfer_characteristics;             // u(8)
  unsigned int matrix_coefficients;                  // u(8)
  Boolean      chroma_location_info_present_flag;    // u(1)
  unsigned int chroma_sample_loc_type_top_field;     // ue(v)
  unsigned int chroma_sample_loc_type_bottom_field;  // ue(v)

  Boolean      timing_info_present_flag;             // u(1)
  unsigned int num_units_in_tick;                    // u(32)
  unsigned int time_scale;                           // u(32)
  Boolean      fixed_frame_rate_flag;                // u(1)

  Boolean      nal_hrd_parameters_present_flag;      // u(1)
  sHRD         nal_hrd_parameters;                   // hrd_paramters_t

  Boolean      vcl_hrd_parameters_present_flag;      // u(1)
  sHRD         vcl_hrd_parameters;                   // hrd_paramters_t

  // if ((nal_hrd_parameters_present_flag || (vcl_hrd_parameters_present_flag))
  Boolean      low_delay_hrd_flag;                      // u(1)
  Boolean      pic_struct_present_flag;                 // u(1)
  Boolean      bitstream_restriction_flag;              // u(1)
  Boolean      motion_vectors_over_pic_boundaries_flag; // u(1)

  unsigned int max_bytes_per_pic_denom;                 // ue(v)
  unsigned int max_bits_per_mb_denom;                   // ue(v)
  unsigned int log2_max_mv_length_vertical;             // ue(v)
  unsigned int log2_max_mv_length_horizontal;           // ue(v)
  unsigned int num_reorder_frames;                      // ue(v)
  unsigned int max_dec_frame_buffering;                 // ue(v)
  } sVUI;
//}}}
//{{{  sSPS
typedef struct {
  Boolean  valid;

  unsigned int profileIdc;            // u(8)
  Boolean  constrained_set0_flag;     // u(1)
  Boolean  constrained_set1_flag;     // u(1)
  Boolean  constrained_set2_flag;     // u(1)
  Boolean  constrained_set3_flag;     // u(1)
  unsigned int levelIdc;              // u(8)
  unsigned int spsId;                 // ue(v)
  unsigned int chromaFormatIdc;       // ue(v)

  Boolean  seq_scaling_matrix_present_flag;   // u(1)
  int      seq_scaling_list_present_flag[12]; // u(1)
  int      scalingList4x4[6][16];             // se(v)
  int      scalingList8x8[6][64];             // se(v)
  Boolean  useDefaultScalingMatrix4x4Flag[6];
  Boolean  useDefaultScalingMatrix8x8Flag[6];

  unsigned int bit_depth_luma_minus8;             // ue(v)
  unsigned int bit_depth_chroma_minus8;           // ue(v)
  unsigned int log2_max_frame_num_minus4;         // ue(v)
  unsigned int pocType;
  unsigned int log2_max_pic_order_cnt_lsb_minus4; // ue(v)
  Boolean  delta_pic_order_always_zero_flag;      // u(1)
  int      offsetNonRefPic;                // se(v)
  int      offsetTopBotField;        // se(v)

  unsigned int numRefFramesPocCycle;          // ue(v)
  int      offset_for_ref_frame[MAX_NUM_REF_FRAMES_PIC_ORDER]; // se(v)
  unsigned int numRefFrames;                      // ue(v)

  Boolean  gaps_in_frame_num_value_allowed_flag;  // u(1)

  unsigned int pic_width_in_mbs_minus1;           // ue(v)
  unsigned int pic_height_in_map_units_minus1;    // ue(v)

  Boolean  frameMbOnlyFlag;                 // u(1)
  Boolean  mb_adaptive_frame_field_flag;    // u(1)
  Boolean  direct_8x8_inference_flag;       // u(1)

  Boolean  frameCropFlag;                   // u(1)
  unsigned int frameCropLeft;               // ue(v)
  unsigned int frameCropRight;              // ue(v)
  unsigned int frameCropTop;                // ue(v)
  unsigned int frameCropBot;                // ue(v)

  Boolean  vui_parameters_present_flag;     // u(1)
  sVUI     vui_seq_parameters;              // sVUI

  unsigned sepColourPlaneFlag;              // u(1)
  int      losslessQpPrimeFlag;
  } sSPS;
//}}}
//{{{  sPPS
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
//}}}

struct Macroblock;
//{{{  sDecodingEnv
typedef struct {
  unsigned int Drange;
  unsigned int Dvalue;
  int          DbitsLeft;
  byte*        Dcodestrm;
  int*         Dcodestrm_len;
  } sDecodingEnv;
//}}}
//{{{  sBitStream
typedef struct BitStream {
  // CAVLC Decoding
  byte* bitStreamBuffer;    // actual codebuffer for read bytes
  int   bitStreamOffset; // actual position in the codebuffer, bit-oriented, CAVLC only
  int   bitStreamLen;    // over codebuffer lnegth, byte oriented, CAVLC only
  int   errorFlag;       // error indication, 0: no error, else unspecified error

  // CABAC Decoding
  int   readLen;         // actual position in the codebuffer, CABAC only
  int   codeLen;         // overall codebuffer length, CABAC only
  } sBitStream;
//}}}
//{{{  sSyntaxElement
typedef struct SyntaxElement {
  int           type;        // type of syntax element for data part.
  int           value1;      // numerical value of syntax element
  int           value2;      // for blocked symbols, e.g. run/level
  int           len;         // length of code
  int           inf;         // info part of CAVLC code
  unsigned int  bitpattern;  // CAVLC bitpattern
  int           context;     // CABAC context
  int           k;           // CABAC context for coeff_count,uv

  // CAVLC mapping to syntaxElement
  void (*mapping) (int, int, int*, int*);

  // CABAC actual coding method of each individual syntax element type
  void (*reading) (struct Macroblock*, struct SyntaxElement*, sDecodingEnv*);
  } sSyntaxElement;
//}}}
//{{{  sDataPartition
typedef struct DataPartition {
  sBitStream*  s;
  sDecodingEnv deCabac;
  int (*readSyntaxElement) (struct Macroblock*, struct SyntaxElement*, struct DataPartition*);
  } sDataPartition;
//}}}

//{{{  sBiContextType
//! struct for context management
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
  sBiContextType mb_type_contexts[3][NUM_MB_TYPE_CTX];
  sBiContextType b8_type_contexts[2][NUM_B8_TYPE_CTX];
  sBiContextType mv_res_contexts[2][NUM_MV_RES_CTX];
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
typedef struct PixelPos {
  int   available;
  int   mbIndex;
  short x;
  short y;
  short posX;
  short posY;
  } sPixelPos;
//}}}
//{{{  sCBPStructure
typedef struct CBPStructure {
  int64 blk;
  int64 bits;
  int64 bits_8x8;
  } sCBPStructure;
//}}}
//{{{  sMotionVec
typedef struct {
  short mvX;
  short mvY;
  } sMotionVec;
//}}}
//{{{  sMacroblock
typedef struct Macroblock {
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

  struct Macroblock* mbCabacUp;   // pointer to neighboring MB (CABAC)
  struct Macroblock* mbCabacLeft; // pointer to neighboring MB (CABAC)

  struct Macroblock* mbUp;    // neighbors for loopfilter
  struct Macroblock* mbLeft;  // neighbors for loopfilter

  // some storage of macroblock syntax elements for global access
  short   mbType;
  short   mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];      //!< indices correspond to [forw,backw][blockY][blockX][x,y]
  int     cbp;
  sCBPStructure  cbpStructure[3];

  int   i16mode;
  char  b8mode[4];
  char  b8pdir[4];
  char  ipmode_DPCM;
  char  cPredMode;       //!< chroma intra prediction mode
  char  skipFlag;
  short DFDisableIdc;
  short DFAlphaC0Offset;
  short DFBetaOffset;

  Boolean mbField;

  // Flag for MBAFF deblocking;
  byte  mixedModeEdgeFlag;

  // deblocking strength indices
  byte strengthV[4][4];
  byte strengthH[4][16];

  Boolean lumaTransformSize8x8flag;
  Boolean noMbPartLessThan8x8Flag;

  // virtual methods
  void (*iTrans4x4) (struct Macroblock*, eColorPlane, int, int);
  void (*iTrans8x8) (struct Macroblock*, eColorPlane, int, int);
  void (*GetMVPredictor) (struct Macroblock*, sPixelPos*, sMotionVec*, short, struct PicMotion**, int, int, int, int, int);
  int  (*readStoreCBPblockBit) (struct Macroblock*, sDecodingEnv*, int);
  char (*readRefPictureIndex) (struct Macroblock*, struct SyntaxElement*, struct DataPartition*, char, int);
  void (*readCompCoef4x4cabac) (struct Macroblock*, struct SyntaxElement*, eColorPlane, int(*)[4], int, int);
  void (*readCompCoef8x8cabac) (struct Macroblock*, struct SyntaxElement*, eColorPlane);
  void (*readCompCoef4x4cavlc) (struct Macroblock*, eColorPlane, int(*)[4], int, int, byte**);
  void (*readCompCoef8x8cavlc) (struct Macroblock*, eColorPlane, int(*)[8], int, int, byte**);
  } sMacroblock;
//}}}
//{{{  sWPParam
typedef struct WPParam {
  short weight[3];
  short offset[3];
  } sWPParam;
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
  byte     botFieldFlag;
  byte     idrFlag;
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

  // poc mode 0
  unsigned int picOrderCountLsb;
  int deletaPicOrderCountBot;
  signed int PicOrderCntMsb;

  // poc mode 1
  int deltaPicOrderCount[2];
  unsigned int AbsFrameNum;
  int thisPoc;

  // information need to move to slice
  unsigned int  mbIndex;
  unsigned int  numDecodedMbs;

  short         curSliceIndex;
  int           codCount;    // Current count of number of skipped macroblocks in a row
  int           allrefzero;

  int           mbAffFrameFlag;
  int           directSpatialMvPredFlag; // Indicator for direct mode type (1 for Spatial, 0 for Temporal)
  int           numRefIndexActive[2];    // number of available list references

  int           errorFlag;       // 0 if the dps[0] contains valid information
  int           qp;
  int           sliceQpDelta;
  int           qs;
  int           sliceQsDelta;
  int           sliceType;   // slice type
  int           modelNum;    // cabac model number
  unsigned int  frameNum;    // frameNum for this frame
  unsigned int  fieldPicFlag;
  byte          botFieldFlag;
  ePicStructure structure;   // Identify picture structure type
  int           startMbNum;  // MUST be set by NAL even in case of errorFlag == 1
  int           endMbNumPlus1;
  int           maxDataPartitions;
  int           datadpMode;       // data dping mode
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
  int adaptiveRefPicBufferingFlag;
  sDecodedRefPicMarking* decRefPicMarkingBuffer; // stores the memory management control operations

  char listXsize[6];
  struct Picture** listX[6];

  sDataPartition*       dps;  // array of dps
  sMotionInfoContexts*  motionContext;  // pointer to struct of context models for use in CABAC
  sTextureInfoContexts* textureContext;  // pointer to struct of context models for use in CABAC

  int   mvscale[6][MAX_REFERENCE_PICTURES];
  int   refPicReorderFlag[2];
  int*  modification_of_pic_nums_idc[2];
  int*  abs_diff_pic_num_minus1[2];
  int*  long_term_pic_idx[2];

  short DFDisableIdc;      // Disable deblocking filter on slice
  short DFAlphaC0Offset;   // Alpha and C0 offset for filtering slice
  short DFBetaOffset;      // Beta offset for filtering slice
  int   ppsId;             // the ID of the picture parameter set the slice is reffering to
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

  sPixel**  tmp_block_l0;
  sPixel**  tmp_block_l1;
  int**     tmp_res;
  sPixel**  tmp_block_l2;
  sPixel**  tmp_block_l3;

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
  sWPParam** WPParam;     // wp parameters in [list][index]
  int***     wpWeight;   // weight in [list][index][component] order
  int***     wpOffset;   // offset in [list][index][component] order
  int****    wbpWeight;  // weight in [list][fw_index][bw_index][component] order
  short      wp_round_luma;
  short      wp_round_chroma;

  // for signalling to the neighbour logic that this is a deblocker call
  int max_mb_vmv_r;      // maximum vertical motion vector range in luma quarter pixel units for the current levelIdc
  int refFlag[17];       // 0: i-th previous frame is incorrect

  int ercMvPerMb;
  sMacroblock* mbData;
  struct Picture* picture;

  int**  siBlock;
  byte** predMode;
  char*  intraBlock;
  char   chroma_vector_adjustment[6][32];

  // virtual methods
  int  (*nalStartcode) (struct Slice*, int);
  void (*initLists) (struct Slice*);
  void (*readCBPcoeffs) (sMacroblock*);
  int  (*decodeOneComponent) (sMacroblock*, eColorPlane, sPixel**, struct Picture*);
  void (*nalReadMotionInfo) (sMacroblock*);
  void (*readMacroblock) (sMacroblock*);
  void (*interpretMbMode) (sMacroblock*);
  void (*intraPredChroma) (sMacroblock*);
  int  (*intraPred4x4) (sMacroblock*, eColorPlane, int, int, int, int);
  int  (*intraPred8x8) (sMacroblock*, eColorPlane, int, int);
  int  (*intraPred16x16) (sMacroblock*, eColorPlane plane, int);
  void (*updateDirectMvInfo) (sMacroblock*);
  void (*readCoef4x4cavlc) (sMacroblock*, int, int, int, int[16], int[16], int*);
  void (*linfoCbpIntra) (int, int, int*, int*);
  void (*linfoCbpInter) (int, int, int*, int*);
  } sSlice;
//}}}
//{{{  sParam
typedef struct Param {
  int vlcDebug;
  int naluDebug;
  int sliceDebug;
  int spsDebug;
  int ppsDebug;
  int seiDebug;

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
//{{{  sCoding
typedef struct CodingParam {
  int profileIdc;
  int structure;           // Identify picture structure type
  int yuvFormat;
  int sepColourPlaneFlag;
  int type;                // image type INTER/INTRA

  int width;
  int height;
  int widthCr;   // width chroma
  int heightCr;  // height chroma

  int picUnitBitSizeDisk;
  short bitdepthLuma;
  short bitdepthChroma;
  int bitdepthScale[2];
  int bitdepthLumeQpScale;
  int bitdepthChromaQpScale;
  int maxPelValueComp[MAX_PLANE];          // max value that one picture element (pixel) can take (depends on pic_unit_bitdepth)
  unsigned int dcPredValueComp[MAX_PLANE]; // component value for DC prediction (depends on component pel bit depth)

  int losslessQpPrimeFlag;
  int numBlock8x8uv;
  int numUvBlocks;
  int numCdcCoeff;
  int mbCrSizeX;
  int mbCrSizeY;
  int mbCrSizeXblock;
  int mbCrSizeYblock;
  int mbCrSize;
  int mbSize[3][2];       // component macroblock dimensions
  int mbSizeBlock[3][2];  // component macroblock dimensions
  int mbSizeShift[3][2];

  int maxVmvR;            // maximum vertical motion vector range in luma quarter frame pixel units for the current levelIdc
  int maxFrameNum;

  unsigned int picWidthMbs;
  unsigned int picHeightMapUnits;
  unsigned int frameHeightMbs;
  unsigned int frameSizeMbs;

  int iLumaPadX;
  int iLumaPadY;
  int iChromaPadX;
  int iChromaPadY;

  int subpelX;
  int subpelY;
  int shiftpelX;
  int shiftpelY;
  int totalScale;
  } sCoding;
//}}}
//{{{  sDecoder
typedef struct  Decoder {
  sParam       param;

  // debug
  TIME_T       startTime;
  TIME_T       endTime;
  char         sliceTypeStr[9];

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

  // recovery
  int          recoveryPoint;
  int          recoveryPointFound;
  int          recoveryFrameCount;
  int          recoveryFrameNum;
  int          recoveryPoc;

  int          decodeFrameNum;
  int          idrFrameNum;
  unsigned int preFrameNum;  // last decoded slice. For detecting gap in frameNum.
  unsigned int prevFrameNum; // number of previous slice
  int          newFrame;

  struct DPB*  dpb;

  int          picSliceIndex;
  int          numDecodedMbs;
  int          numDecodedSlices;
  int          numAllocatedSlices;
  sSlice**     sliceList;
  sSlice*      nextSlice;           // pointer to first sSlice of next picture;
  struct OldSlice* oldSlice;

  // coding stuff
  int width;
  int height;
  int widthCr;
  int heightCr;

  int picDiskUnitSize;
  int picUnitBitSizeDisk;
  short bitdepthLuma;
  short bitdepthChroma;
  int bitdepthScale[2];
  int bitdepthLumeQpScale;
  int bitdepthChromaQpScale;

  int numBlock8x8uv;
  int numUvBlocks;
  int numCdcCoeff;
  int mbCrSizeX;
  int mbCrSizeY;
  int mbCrSizeXblock;
  int mbCrSizeYblock;
  int mbCrSize;
  int mbSize[3][2];
  int mbSizeBlock[3][2];
  int mbSizeShift[3][2];

  // sCoding
  sCoding      coding;
  sBlockPos*   picPos;
  byte****     nzCoeff;

  sMacroblock* mbData;              // array containing all MBs of a whole frame
  char*        intraBlock;
  byte**       predMode;            // prediction type [90][74]
  int**        siBlock;

  sMacroblock* mbDataJV[MAX_PLANE];
  char*        intraBlockJV[MAX_PLANE];
  byte**       predModeJV[MAX_PLANE];
  int**        siBlockJV[MAX_PLANE];

  // picture error conceal
  // concealHead points to first node in list, concealTail points to
  // last node in list. Initialize both to NULL, meaning no nodes in list yet
  struct ConcealNode* concealHead;
  struct ConcealNode* concealTail;
  int          concealMode;
  int          earlierMissingPoc;
  unsigned int concealFrame;
  int          idrConcealFlag;
  int          concealSliceType;

  int          nonConformingStream;
  int          lastRefPicPoc;
  int          refPocGap;
  int          pocGap;

  // for POC mode 0:
  signed int   PrevPicOrderCntMsb;
  unsigned int PrevPicOrderCntLsb;

  // for POC mode 1:
  signed int   expectedPOC, POCcycleCount, frameNumPOCcycle;
  unsigned int previousFrameNum, frameNumOffset;
  int          expectedDeltaPerPOCcycle;
  int          thisPoc;
  int          previousFrameNumOffset;

  unsigned int picHeightInMbs;
  unsigned int picSizeInMbs;

  int          noOutputPriorPicFlag;

  int          lastHasMmco5;
  int          lastPicBotField;

  // non-zero: i-th previous frame is correct
  int          isPrimaryOk;    // if primary frame is correct, 0: incorrect
  int          isReduncantOk;  // if redundant frame is correct, 0:incorrect

  int*         qpPerMatrix;
  int*         qpRemMatrix;

  int          dpbPoc[100];

  // slice pictures
  struct Picture* picture;
  struct Picture* decPictureJV[MAX_PLANE];  // picture to be used during 4:4:4 independent mode decoding
  struct Picture* noReferencePicture;       // dummy storable picture for recovery point
  struct FrameStore* lastOutFramestore;

  // Error parameters
  struct ObjectBuffer* ercObjectList;
  struct ErcVariables* ercErrorVar;
  int                  ercMvPerMb;
  int                  ecFlag[SE_MAX_ELEMENTS];  // array to set errorconcealment

  struct FrameStore*   outBuffer;
  struct Picture*      pendingOut;
  int                  pendingOutState;
  int                  recoveryFlag;

  // fmo
  int*                 mbToSliceGroupMap;
  int*                 mapUnitToSliceGroupMap;
  int                  sliceGroupsNum;  // the number of slice groups -1 (0 == scan order, 7 == maximum)

  int                  deblockMode;  // 0: deblock in picture, 1: deblock in slice;
  sDecodedPic*         decOutputPic;

  // control;
  int   deblockEnable;

  // virtual methods
  void (*getNeighbour) (sMacroblock*, int, int, int[2], sPixelPos*);
  void (*getMbBlockPos) (sBlockPos*, int, short*, short*);
  void (*getStrengthV) (sMacroblock*, int, int, struct Picture*);
  void (*getStrengthH) (sMacroblock*, int, int, struct Picture*);
  void (*edgeLoopLumaV) (eColorPlane, sPixel**, byte*, sMacroblock*, int);
  void (*edgeLoopLumaH) (eColorPlane, sPixel**, byte*, sMacroblock*, int, struct Picture*);
  void (*edgeLoopChromaV) (sPixel**, byte*, sMacroblock*, int, int, struct Picture*);
  void (*edgeLoopChromaH) (sPixel**, byte*, sMacroblock*, int, int, struct Picture*);
  } sDecoder;
//}}}

static const sMotionVec zero_mv = {0, 0};
//{{{
static inline int isBLprofile (unsigned int profileIdc) {
  return profileIdc == FREXT_CAVLC444 || profileIdc == BASELINE ||
         profileIdc == MAIN || profileIdc == EXTENDED ||
         profileIdc == FREXT_HP || profileIdc == FREXT_Hi10P ||
         profileIdc == FREXT_Hi422 || profileIdc == FREXT_Hi444;
}
//}}}
//{{{
static inline int isFrextProfile (unsigned int profileIdc) {
  // we allow all FRExt tools, when no profile is active
  return profileIdc == NO_PROFILE || profileIdc == FREXT_HP ||
         profileIdc == FREXT_Hi10P || profileIdc == FREXT_Hi422 ||
         profileIdc == FREXT_Hi444 || profileIdc == FREXT_CAVLC444;
}
//}}}
//{{{
static inline int isHiIntraOnlyProfile (unsigned int profileIdc, Boolean constrained_set3_flag) {
  return (((profileIdc == FREXT_Hi10P)||(profileIdc == FREXT_Hi422) ||
           (profileIdc == FREXT_Hi444)) && constrained_set3_flag) ||
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

  extern void setGlobalCodingProgram (sDecoder* decoder);
//{{{
#ifdef __cplusplus
}
#endif
//}}}
