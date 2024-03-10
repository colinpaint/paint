#pragma once
#ifdef TRACE
  #undef TRACE
#endif

#if defined _DEBUG
  #define TRACE           0     //!< 0:Trace off 1:Trace on 2:detailed CABAC context information
#else
  #define TRACE           0     //!< 0:Trace off 1:Trace on 2:detailed CABAC context information
#endif

#define IMGTYPE                 0  // Define sPixel size type. 0 implies byte (cannot handle >8 bit depths) and 1 implies unsigned short
#define ENABLE_HIGH444_CTX      1  // Enables High 444 profile context types for CABAC.
#define JCOST_CALC_SCALEUP      1  // 1: J = (D<<LAMBDA_ACCURACY_BITS)+Lambda*R; 0: J = D + ((Lambda*R+Rounding)>>LAMBDA_ACCURACY_BITS)
#define SIMULCAST_ENABLE        0  // to test the decoder

#define MVC_INIT_VIEW_ID        -1

#include "typedefs.h"

#define SSE_MEMORY_ALIGNMENT    16

#define MAX_NUM_SLICES          50
#define MAX_REFERENCE_PICTURES  32       // H.264 allows 32 fields
#define MAX_CODED_FRAME_SIZE    8000000  // bytes for one frame
#define MAX_NUM_DECSLICES       16
#define MCBUF_LUMA_PAD_X        32
#define MCBUF_LUMA_PAD_Y        12
#define MCBUF_CHROMA_PAD_X      16
#define MCBUF_CHROMA_PAD_Y      8
#define MAX_NUM_DPB_LAYERS      2

//{{{  AVC Profile IDC definitions
typedef enum {
  NO_PROFILE     =  0,       // disable profile checking for experimental coding (enables FRExt, but disables MV)
  FREXT_CAVLC444 = 44,       // YUV 4:4:4/14 "CAVLC 4:4:4"
  BASELINE       = 66,       // YUV 4:2:0/8  "Baseline"
  MAIN           = 77,       // YUV 4:2:0/8  "Main"
  EXTENDED       = 88,       // YUV 4:2:0/8  "Extended"
  FREXT_HP       = 100,      // YUV 4:2:0/8  "High"
  FREXT_Hi10P    = 110,      // YUV 4:2:0/10 "High 10"
  FREXT_Hi422    = 122,      // YUV 4:2:2/10 "High 4:2:2"
  FREXT_Hi444    = 244,      // YUV 4:4:4/14 "High 4:4:4"
  MVC_HIGH       = 118,      // YUV 4:2:0/8  "Multiview High"
  STEREO_HIGH    = 128       // YUV 4:2:0/8  "Stereo High"
  } ProfileIDC;
//}}}

#define FILE_NAME_SIZE  255
#define INPUT_TEXT_SIZE 1024

#if (ENABLE_HIGH444_CTX == 1)
  #define NUM_BLOCK_TYPES 22
#else
  #define NUM_BLOCK_TYPES 10
#endif

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
#define MB_BLOCK_dpS   16  // (BLOCK_MULTIPLE * BLOCK_MULTIPLE)
#define BLOCK_CONTEXT         64  // (4 * MB_BLOCK_dpS)

// These variables relate to the subpel accuracy supported by the software (1/4)
#define BLOCK_SIZE_SP         16  // BLOCK_SIZE << 2
#define BLOCK_SIZE_8x8_SP     32  // BLOCK_SIZE8x8 << 2

//{{{  Available MB modes
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
  } MBModeTypes;
//}}}

// number of intra prediction modes
#define NO_INTRA_PMODE   9
//{{{  enum DirectModes
typedef enum {
  DIR_TEMPORAL = 0, //!< Temporal Direct Mode
  DIR_SPATIAL  = 1 //!< Spatial Direct Mode
  } DirectModes;
//}}}
//{{{  enum CAVLCBlockTypes
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
  } CAVLCBlockTypes;
//}}}
//{{{  enum CABACBlockTypes
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
  } CABACBlockTypes;
//}}}
//{{{  enum ColorComponent
typedef enum {
  Y_COMP = 0,    // Y Component
  U_COMP = 1,    // U Component
  V_COMP = 2,    // V Component
  R_COMP = 3,    // R Component
  G_COMP = 4,    // G Component
  B_COMP = 5,    // B Component
  T_COMP = 6
  } ColorComponent;
//}}}

// Macro defines
#define Q_BITS           15
#define DQ_BITS          6
#define Q_BITS_8         16
#define DQ_BITS_8        6

#define IS_I16MB(MB)     ((MB)->mbType == I16MB || (MB)->mbType == IPCM)
#define IS_DIRECT(MB)    ((MB)->mbType == 0 && (slice->sliceType == B_SLICE ))

#define TOTRUN_NUM       15
#define RUNBEFORE_NUM    7
#define RUNBEFORE_NUM_M1 6

// Quantization parameter range
#define MIN_QP           0
#define MAX_QP           51
//{{{  enum 4x4 intra prediction modes
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
  } I4x4PredModes;
//}}}
//{{{  enum 16x16 intra prediction modes
typedef enum {
  VERT_PRED_16   = 0,
  HOR_PRED_16    = 1,
  DC_PRED_16     = 2,
  PLANE_16       = 3
  } I16x16PredModes;
//}}}
//{{{  enum 8x8 chroma intra prediction modes
typedef enum {
  DC_PRED_8     =  0,
  HOR_PRED_8    =  1,
  VERT_PRED_8   =  2,
  PLANE_8       =  3
  } I8x8PredModes;
//}}}
//{{{  enum StartEnd
typedef enum {
  EOS = 1,    //!< End Of Sequence
  SOP = 2,    //!< Start Of Picture
  SOS = 3,     //!< Start Of sSlice
  SOS_CONT = 4
  } StartEnd;
//}}}
//{{{  enum MV Prediction types
typedef enum {
  MVPRED_MEDIAN   = 0,
  MVPRED_L        = 1,
  MVPRED_U        = 2,
  MVPRED_UR       = 3
  } MVPredTypes;
//}}}
//{{{  enum DecRet
typedef enum {
  DECODING_OK     = 0,
  SEARCH_SYNC     = 1,
  PICTURE_DECODED = 2
  } DecRet;
//}}}

#define MAX_PLANE                 3
#define LAMBDA_ACCURACY_BITS      16
#define INVALIDINDEX              (-135792468)
#define RC_MAX_TEMPORAL_LEVELS    5

// Start code and Emulation Prevention need this to be defined in identical manner at encoder and decoder
#define ZEROBYTES_SHORTSTARTCODE  2 //indicates the number of zero bytes in the short start-code prefix
