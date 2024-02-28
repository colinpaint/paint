#pragma once
//{{{  ColorPlane
typedef enum {
  // YUV
  PLANE_Y = 0,  // PLANE_Y
  PLANE_U = 1,  // PLANE_Cb
  PLANE_V = 2,  // PLANE_Cr
  // RGB
  PLANE_G = 0,
  PLANE_B = 1,
  PLANE_R = 2
} ColorPlane;
//}}}
//{{{  PredList
typedef enum {
  LIST_0 = 0,
  LIST_1 = 1,
  BI_PRED = 2,
  BI_PRED_L0 = 3,
  BI_PRED_L1 = 4
} PredList;
//}}}
//{{{  ErrorMetric
typedef enum {
  ERROR_SAD = 0,
  ERROR_SSE = 1,
  ERROR_SATD = 2,
  ERROR_PSATD = 3
} ErrorMetric;
//}}}
//{{{  YuvFormat
typedef enum {
  ME_Y_ONLY = 0,
  ME_YUV_FP = 1,
  ME_YUV_FP_SP = 2
} YuvFormat;
//}}}
//{{{  DistortionMetric
typedef enum {
  DISTORTION_MSE = 0
} Distortionmetric;
//}}}

//{{{  Data Partitioning Modes
typedef enum {
  PAR_DP_1,   //!< no data partitioning is supported
  PAR_DP_3    //!< data partitioning with 3 partitions
} PAR_DP_TYPE;
//}}}
//{{{  Output File Types
typedef enum {
  PAR_OF_ANNEXB,    //!< Annex B byte stream format
  PAR_OF_RTP       //!< RTP packets in outfile
} PAR_OF_TYPE;
//}}}
//{{{  Field Coding Types
typedef enum {
  FRAME_CODING         = 0,
  FIELD_CODING         = 1,
  ADAPTIVE_CODING      = 2,
  FRAME_MB_PAIR_CODING = 3
} CodingType;
//}}}
//{{{  definition of H.264 syntax elements
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
  SE_MAX_ELEMENTS = 20 //!< number of maximum syntax elements
} SE_type;             // substituting the definitions in elements.h
//}}}

//{{{  SliceMode
typedef enum {
  NO_SLICES,
  FIXED_MB,
  FIXED_RATE,
  CALL_BACK
} SliceMode;
//}}}
//{{{  symbolMode
typedef enum {
  CAVLC,
  CABAC
} SymbolMode;
//}}}
//{{{  searchType
typedef enum {
  FULL_SEARCH      = -1,
  FAST_FULL_SEARCH =  0,
  UM_HEX           =  1,
  UM_HEX_SIMPLE    =  2,
  EPZS             =  3
} SearchType;
//}}}
//{{{  pictureStructure
typedef enum {
  FRAME,
  TOP_FIELD,
  BOTTOM_FIELD
} PictureStructure;           //!< New enum for field processing
//}}}

//{{{  SliceType
typedef enum {
  P_SLICE = 0,
  B_SLICE = 1,
  I_SLICE = 2,
  SP_SLICE = 3,
  SI_SLICE = 4,
  NUM_SLICE_TYPES = 5
} SliceType;

//}}}
//{{{  Motion Estimation levels
typedef enum {
  F_PEL,   //!< Full Pel refinement
  H_PEL,   //!< Half Pel refinement
  Q_PEL    //!< Quarter Pel refinement
} MELevel;
//}}}
//{{{  refAccess
typedef enum {
  FAST_ACCESS = 0,    //!< Fast/safe reference access
  UMV_ACCESS = 1      //!< unconstrained reference access
} REF_ACCESS_TYPE;
//}}}
//{{{  componentType
typedef enum {
  IS_LUMA = 0,
  IS_CHROMA = 1
} Component_Type;
//}}}
//{{{  RCModeType
typedef enum {
  RC_MODE_0 = 0,
  RC_MODE_1 = 1,
  RC_MODE_2 = 2,
  RC_MODE_3 = 3
} RCModeType;
//}}}
//{{{  weightedPredictionTypes
typedef enum {
  WP_MCPREC_PLUS0 =       4,
  WP_MCPREC_PLUS1 =       5,
  WP_MCPREC_MINUS0 =      6,
  WP_MCPREC_MINUS1 =      7,
  WP_MCPREC_MINUS_PLUS0 = 8,
  WP_REGULAR =            9
} weighted_prediction_types;
//}}}
