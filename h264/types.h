#pragma once
//{{{  enum eColorPlane
typedef enum {
  // YUV
  PLANE_Y = 0,  // PLANE_Y
  PLANE_U = 1,  // PLANE_Cb
  PLANE_V = 2,  // PLANE_Cr
  } eColorPlane;
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

//{{{  enum eDataPartitionType
typedef enum {
  PAR_DP_1,   //!< no data dping is supported
  PAR_DP_3    //!< data dping with 3 dps
  } eDataPartitionType;
//}}}
//{{{  enum eCodingType
typedef enum {
  FRAME_CODING         = 0,
  FIELD_CODING         = 1,
  ADAPTIVE_CODING      = 2,
  FRAME_MB_PAIR_CODING = 3
 } eCodingType;
//}}}
//{{{  enum eSeType - definition of H.264 syntax elements
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
  } eSeType;             
//}}}

//{{{  enum eSliceMode
typedef enum {
  NO_SLICES,
  FIXED_MB,
  FIXED_RATE,
  CALL_BACK
  } eSliceMode;
//}}}
//{{{  enum eSymbolMode
typedef enum {
  CAVLC,
  CABAC
  } eSymbolMode;
//}}}
//{{{  enum eSearchType
typedef enum {
  FULL_SEARCH      = -1,
  FAST_FULL_SEARCH =  0,
  UM_HEX           =  1,
  UM_HEX_SIMPLE    =  2,
  EPZS             =  3
  } eSearchType;
//}}}
//{{{  enum ePicStructure
typedef enum {
  FRAME,
  TopField,
  BotField
  } ePicStructure;
//}}}

//{{{  enum eSliceType
typedef enum {
  P_SLICE = 0,
  B_SLICE = 1,
  I_SLICE = 2,
  SP_SLICE = 3,
  SI_SLICE = 4,
  NUM_SLICE_TYPES = 5
  } eSliceType;
//}}}
//{{{  enum eMotionEstimationLevel
typedef enum {
  F_PEL,   // Full Pel refinement
  H_PEL,   // Half Pel refinement
  Q_PEL    // Quarter Pel refinement
  } eMotionEstimationLevel;
//}}}
//{{{  enum refAccess
typedef enum {
  FAST_ACCESS = 0, // Fast/safe reference access
  UMV_ACCESS = 1   // unconstrained reference access
  } eRefAccessType;
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
//{{{  enum  eWeightedPredictionType;
typedef enum {
  WP_MCPREC_PLUS0 =       4,
  WP_MCPREC_PLUS1 =       5,
  WP_MCPREC_MINUS0 =      6,
  WP_MCPREC_MINUS1 =      7,
  WP_MCPREC_MINUS_PLUS0 = 8,
  WP_REGULAR =            9
  } eWeightedPredictionType;
//}}}
