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

  unsigned int profileIdc;           // u(8)
  Boolean  constrained_set0_flag;     // u(1)
  Boolean  constrained_set1_flag;     // u(1)
  Boolean  constrained_set2_flag;     // u(1)
  Boolean  constrained_set3_flag;     // u(1)
  unsigned int level_idc;             // u(8)
  unsigned int spsId;  // ue(v)
  unsigned int chromaFormatIdc;       // ue(v)

  Boolean  seq_scaling_matrix_present_flag;    // u(1)
  int      seq_scaling_list_present_flag[12];  // u(1)
  int      scalingList4x4[6][16];              // se(v)
  int      scalingList8x8[6][64];              // se(v)
  Boolean  useDefaultScalingMatrix4x4Flag[6];
  Boolean  useDefaultScalingMatrix8x8Flag[6];

  unsigned int bit_depth_luma_minus8;             // ue(v)
  unsigned int bit_depth_chroma_minus8;           // ue(v)
  unsigned int log2_max_frame_num_minus4;         // ue(v)
  unsigned int picOrderCountType;
  unsigned int log2_max_pic_order_cnt_lsb_minus4; // ue(v)
  Boolean  delta_pic_order_always_zero_flag;      // u(1)
  int      offset_for_non_ref_pic;                // se(v)
  int      offset_for_top_to_bottom_field;        // se(v)

  unsigned int num_ref_frames_in_pic_order_cnt_cycle;          // ue(v)
  int      offset_for_ref_frame[MAX_NUM_REF_FRAMES_PIC_ORDER]; // se(v)
  unsigned int numRefFrames;                    // ue(v)

  Boolean  gaps_in_frame_num_value_allowed_flag;  // u(1)

  unsigned int pic_width_in_mbs_minus1;           // ue(v)
  unsigned int pic_height_in_map_units_minus1;    // ue(v)
  Boolean  frameMbOnlyFlag;                   // u(1)
  Boolean  mb_adaptive_frame_field_flag;          // u(1)
  Boolean  direct_8x8_inference_flag;             // u(1)

  Boolean  frameCropFlag;                   // u(1)
  unsigned int frameCropLeft;            // ue(v)
  unsigned int frameCropRight;           // ue(v)
  unsigned int frameCropTop;             // ue(v)
  unsigned int frameCropBot;          // ue(v)

  Boolean  vui_parameters_present_flag;           // u(1)
  sVUI     vui_seq_parameters;                    // sVUI

  unsigned sepColourPlaneFlag;            // u(1)
  int      lossless_qpprime_flag;
  } sSPS;
//}}}
//{{{  sPPS
typedef struct {
  Boolean   valid;

  unsigned int ppsId;            // ue(v)
  unsigned int spsId;            // ue(v)
  Boolean   entropyCodingModeFlag;          // u(1)
  Boolean   transform8x8modeFlag;           // u(1)

  Boolean   picScalingMatrixPresentFlag;    // u(1)
  int       picScalingListPresentFlag[12];  // u(1)
  int       scalingList4x4[6][16];          // se(v)
  int       scalingList8x8[6][64];          // se(v)
  Boolean   useDefaultScalingMatrix4x4Flag[6];
  Boolean   useDefaultScalingMatrix8x8Flag[6];

  // if( picOrderCountType < 2 )  in the sequence parameter set
  Boolean      botFieldPicOrderFramePresentFlag; // u(1)
  unsigned int numSliceGroupsMinus1;             // ue(v)

  unsigned int sliceGroupMapType;                // ue(v)
  // if (sliceGroupMapType = = 0)
  unsigned int runLengthMinus1[8];               // ue(v)
  // else if (sliceGroupMapType = = 2 )
  unsigned int topLeft[8];                       // ue(v)
  unsigned int botRight[8];                      // ue(v)
  // else if (sliceGroupMapType = = 3 || 4 || 5
  Boolean   sliceGroupChangeDirectionFlag;      // u(1)
  unsigned int sliceGroupChangeRateMius1;       // ue(v)
  // else if (sliceGroupMapType = = 6)
  unsigned int picSizeMapUnitsMinus1;           // ue(v)
  byte*     sliceGroupId;                       // complete MBAmap u(v)

  int       numRefIndexL0defaultActiveMinus1;   // ue(v)
  int       numRefIndexL1defaultActiveMinus1;   // ue(v)
  Boolean   weightedPredFlag;                     // u(1)
  unsigned int  weightedBiPredIdc;                    // u(2)
  int       picInitQpMinus26;                    // se(v)
  int       picInitQsMinus26;                    // se(v)
  int       chromaQpIndexOffset;                 // se(v)

  int       cbQpIndexOffset;                     // se(v)
  int       crQpIndexOffset;                     // se(v)
  int       secondChromaQpIndexOffset;           // se(v)

  Boolean   deblockingFilterControlPresentFlag; // u(1)
  Boolean   constrainedIntraPredFlag;           // u(1)
  Boolean   redundantPicCountPresentFlag;       // u(1)
  Boolean   vuiPicParamFlag;                    // u(1)
  } sPPS;
//}}}
//{{{  sBiContextType
//! struct for context management
typedef struct {
  uint16        state;  // index into state-table CP
  unsigned char MPS;    // Least Probable Symbol 0/1 CP
  unsigned char dummy;  // for alignment
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
//{{{  sDecodingEnvironment
typedef struct {
  unsigned int Drange;
  unsigned int Dvalue;
  int          DbitsLeft;
  byte*        Dcodestrm;
  int*         Dcodestrm_len;
  } sDecodingEnvironment;
//}}}
typedef sDecodingEnvironment* sDecodingEnvironmentPtr;

struct Picture;
struct Macroblock;
struct PicMotionParam;
//{{{  sMotionVector
typedef struct {
  short mvX;
  short mvY;
  } sMotionVector;
//}}}
//{{{  sBlockPos
typedef struct {
  short x;
  short y;
  } sBlockPos;
//}}}
//{{{  sPixelPos
typedef struct PixelPos {
  int   available;
  int   mbAddr;
  short x;
  short y;
  short posX;
  short posY;
  } sPixelPos;
//}}}
//{{{  sBitstream
typedef struct Bitstream {
  // CABAC Decoding
  int readLen;          // actual position in the codebuffer, CABAC only
  int codeLen;          // overall codebuffer length, CABAC only

  // CAVLC Decoding
  int frameBitOffset;   // actual position in the codebuffer, bit-oriented, CAVLC only
  int bitstreamLength;  // over codebuffer lnegth, byte oriented, CAVLC only

  // ErrorConcealment
  byte* streamBuffer;   // actual codebuffer for read bytes
  int eiFlag;           // error indication, 0: no error, else unspecified error
  } sBitstream;
//}}}
//{{{  sDecRefPicMarking
typedef struct DecRefPicMarking {
  int memory_management_control_operation;
  int difference_of_pic_nums_minus1;
  int long_term_pic_num;
  int longTermFrameIndex;
  int max_long_term_frame_idx_plus1;
  struct DecRefPicMarking* next;
  } sDecRefPicMarking;
//}}}
//{{{  sCBPStructure
typedef struct CBPStructure {
  int64 blk;
  int64 bits;
  int64 bits_8x8;
  } sCBPStructure;
//}}}
//{{{  sSyntaxElement
typedef struct SyntaxElement {
  int           type;                  //!< type of syntax element for data part.
  int           value1;                //!< numerical value of syntax element
  int           value2;                //!< for blocked symbols, e.g. run/level
  int           len;                   //!< length of code
  int           inf;                   //!< info part of CAVLC code
  unsigned int  bitpattern;            //!< CAVLC bitpattern
  int           context;               //!< CABAC context
  int           k;                     //!< CABAC context for coeff_count,uv

  // for mapping of CAVLC to syntaxElement
  void (*mapping)(int len, int info, int* value1, int* value2);

  // used for CABAC: refers to actual coding method of each individual syntax element type
  void (*reading)(struct Macroblock* curMb, struct SyntaxElement*, sDecodingEnvironmentPtr);
  } sSyntaxElement;
//}}}
//{{{  sDataPartition
typedef struct DataPartition {
  sBitstream*          bitstream;
  sDecodingEnvironment deCabac;
  int (*readsSyntaxElement)(struct Macroblock *curMb, struct SyntaxElement *, struct DataPartition *);
  } sDataPartition;
//}}}
//{{{  sMacroblock
typedef struct Macroblock {
  struct Slice*      slice;
  struct VidParam*   vidParam;
  struct InputParam* inputParam;

  int     mbAddrX;
  int     mbAddrA;
  int     mbAddrB;
  int     mbAddrC;
  int     mbAddrD;

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

  int     qp;                    // QP luma
  int     qpc[2];                // QP chroma
  int     qpScaled[MAX_PLANE];  // QP scaled for all comps.
  Boolean isLossless;
  Boolean isIntraBlock;
  Boolean isVblock;
  int     DeblockCall;

  short   sliceNum;
  char    eiFlag;            // error indicator flag that enables conceal
  char    dplFlag;           // error indicator flag that signals a missing data partition
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

  //Flag for MBAFF deblocking;
  byte  mixedModeEdgeFlag;

  // deblocking strength indices
  byte strength_ver[4][4];
  byte strength_hor[4][16];

  Boolean lumaTransformSize8x8flag;
  Boolean noMbPartLessThan8x8Flag;

  void (*itrans_4x4)(struct Macroblock *curMb, eColorPlane pl, int ioff, int joff);
  void (*itrans_8x8)(struct Macroblock *curMb, eColorPlane pl, int ioff, int joff);
  void (*GetMVPredictor) (struct Macroblock* curMb, sPixelPos* block,
                          sMotionVector* pmv, short ref_frame, struct PicMotionParam** mvInfo,
                          int list, int mb_x, int mb_y, int blockshape_x, int blockshape_y);
  int  (*read_and_store_CBP_block_bit)(struct Macroblock *curMb, sDecodingEnvironmentPtr dep_dp, int type);
  char (*readRefPictureIdx) (struct Macroblock *curMb, struct SyntaxElement *currSE, struct DataPartition *dP, char b8mode, int list);
  void (*read_comp_coeff_4x4_CABAC)(struct Macroblock *curMb, struct SyntaxElement *currSE, eColorPlane pl, int (*InvLevelScale4x4)[4], int qp_per, int cbp);
  void (*read_comp_coeff_8x8_CABAC)(struct Macroblock *curMb, struct SyntaxElement *currSE, eColorPlane pl);
  void (*read_comp_coeff_4x4_CAVLC)(struct Macroblock *curMb, eColorPlane pl, int (*InvLevelScale4x4)[4], int qp_per, int cbp, byte** nzcoeff);
  void (*read_comp_coeff_8x8_CAVLC)(struct Macroblock *curMb, eColorPlane pl, int (*InvLevelScale8x8)[8], int qp_per, int cbp, byte** nzcoeff);
  } sMacroblock;
//}}}
//{{{  sWPParam
typedef struct WPParam {
  short weight[3];
  short offset[3];
  } sWPParam;
//}}}
//{{{  sInputParam
typedef struct InputParam {
  int vlcDebug;
  int refOffset;
  int pocScale;
  int writeUv;
  int intraProfileDeblocking; // Loop filter usage determined by flags and parameters in bitstream

  sFrameFormat source;        
  sFrameFormat output;        

  int concealMode;
  int refPocGap;
  int pocGap;

  int dpb_plus[2];
  } sInputParam;
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
//{{{  sOldSliceParam
typedef struct OldSliceParam {
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
  int      layerId;
  } sOldSliceParam;
//}}}
//{{{  sSlice
typedef struct Slice {
  struct VidParam* vidParam;
  struct InputParam* inputParam;
  sPPS* activePPS;
  sSPS* activeSPS;

  struct DPB* dpb;

  int idrFlag;
  int idrPicId;
  int nalRefId;                   // nalRefId from NAL
  int transform8x8Mode;
  Boolean chroma444notSeparate;   // indicates chroma 4:4:4 coding with sepColourPlaneFlag equal to zero

  int topPoc;   // poc for this top field
  int botPoc;   // poc of bottom field of frame
  int framePoc; // poc of this frame

  // poc mode 0
  unsigned int picOrderCountLsb;
  int deletaPicOrderCountBot;

  // poc mode 1
  int deltaPicOrderCount[2];

  // POC mode 0
  signed int PicOrderCntMsb;

  // POC mode 1
  unsigned int AbsFrameNum;
  int thisPoc;

  // information need to move to slice
  unsigned int current_mb_nr;  // bitstream order
  unsigned int numDecodedMb;
  short curSliceNum;
  int cod_counter;             // Current count of number of skipped macroblocks in a row
  int allrefzero;

  int               mbAffFrameFlag;
  int               direct_spatial_mv_pred_flag; // Indicator for direct mode type (1 for Spatial, 0 for Temporal)
  int               numRefIndexActive[2];       // number of available list references

  int               eiFlag;       // 0 if the partArr[0] contains valid information
  int               qp;
  int               slice_qp_delta;
  int               qs;
  int               slice_qs_delta;
  int               sliceType;    // slice type
  int               model_number;  // cabac model number
  unsigned int      frameNum;     // frameNum for this frame
  unsigned int      fieldPicFlag;
  byte              botFieldFlag;
  ePicStructure     structure;     // Identify picture structure type
  int               startMbNum;   // MUST be set by NAL even in case of eiFlag == 1
  int               end_mb_nr_plus1;
  int               maxPartitionNum;
  int               dataPartitionMode;       // data partitioning mode
  int               current_header;
  int               next_header;
  int               last_dquant;

  // slice header information;
  int colourPlaneId;             // colourPlaneId of the current coded slice
  int redundantPicCount;
  int sp_switch;                   // 1 for switching sp, 0 for normal sp
  int slice_group_change_cycle;
  int redundantSliceRefIndex;     // reference index of redundant slice
  int noOutputPriorPicFlag;
  int longTermRefFlag;
  int adaptiveRefPicBufferingFlag;
  sDecRefPicMarking* decRefPicMarkingBuffer; // stores the memory management control operations

  char listXsize[6];
  struct Picture** listX[6];

  sDataPartition*       partArr;    // array of partitions
  sMotionInfoContexts*  mot_ctx;    // pointer to struct of context models for use in CABAC
  sTextureInfoContexts* tex_ctx;    // pointer to struct of context models for use in CABAC

  int mvscale[6][MAX_REFERENCE_PICTURES];
  int ref_pic_list_reordering_flag[2];
  int* modification_of_pic_nums_idc[2];
  int* abs_diff_pic_num_minus1[2];
  int* long_term_pic_idx[2];

  int   layerId;
  short DFDisableIdc;         // Disable deblocking filter on slice
  short DFAlphaC0Offset;      // Alpha and C0 offset for filtering slice
  short DFBetaOffset;         // Beta offset for filtering slice
  int   ppsId; // the ID of the picture parameter set the slice is reffering to
  int   noDataPartitionB;       // non-zero, if data partition B is lost
  int   noDataPartitionC;       // non-zero, if data partition C is lost

  Boolean   is_reset_coeff;
  Boolean   is_reset_coeff_cr;
  sPixel*** mb_pred;
  sPixel*** mb_rec;
  int***    mb_rres;
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
  int coeff_ctr;
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
  int max_mb_vmv_r;        // maximum vertical motion vector range in luma quarter pixel units for the current level_idc
  int refFlag[17];        // 0: i-th previous frame is incorrect

  int ercMvPerMb;
  sMacroblock* mbData;

  struct Picture* picture;
  int**   siBlock;
  byte** predMode;
  char*  intraBlock;
  char   chroma_vector_adjustment[6][32];

  void (*read_CBP_and_coeffs_from_NAL) (sMacroblock *curMb);
  int  (*decode_one_component     )    (sMacroblock *curMb, eColorPlane curPlane, sPixel** curPixel, struct Picture* picture);
  int  (*readSlice                )    (struct VidParam *, struct InputParam *);
  int  (*nal_startcode_follows    )    (struct Slice*, int );
  void (*read_motion_info_from_NAL)    (sMacroblock *curMb);
  void (*readOneMacroblock      )    (sMacroblock *curMb);
  void (*interpret_mb_mode        )    (sMacroblock *curMb);
  void (*init_lists               )    (struct Slice *curSlice);
  void (*intra_pred_chroma        )    (sMacroblock *curMb);
  int  (*intra_pred_4x4)               (sMacroblock *curMb, eColorPlane pl, int ioff, int joff,int i4,int j4);
  int  (*intra_pred_8x8)               (sMacroblock *curMb, eColorPlane pl, int ioff, int joff);
  int  (*intra_pred_16x16)             (sMacroblock *curMb, eColorPlane pl, int predmode);
  void (*linfo_cbp_intra          )    (int len, int info, int *cbp, int *dummy);
  void (*linfo_cbp_inter          )    (int len, int info, int *cbp, int *dummy);
  void (*update_direct_mv_info    )    (sMacroblock *curMb);
  void (*read_coeff_4x4_CAVLC     )    (sMacroblock *curMb, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);
  } sSlice;
//}}}
//{{{  sDecodedPicture
typedef struct DecodedPicture {
  int valid;             // 0: invalid, 1: valid, 3: valid for 3D output;
  int viewId;            // -1: single view, >=0 multiview[VIEW1|VIEW0];
  int poc;

  int yuvFormat;         // 0: 4:0:0, 1: 4:2:0, 2: 4:2:2, 3: 4:4:4
  int yuvStorageFormat;  // 0: YUV seperate; 1: YUV interleaved; 2: 3D output;
  int iBitDepth;

  byte* yBuf;            // if iPictureFormat is 1, [0]: top; [1] bottom;
  byte* uBuf;
  byte* vBuf;

  int width;             // frame width;
  int height;            // frame height;
  int yStride;           // stride of yBuf[0/1] buffer in bytes;
  int uvStride;          // stride of uBuf[0/1] and vBuf[0/1] buffer in bytes;
  int skipPicNum;
  int bufSize;

  struct DecodedPicture* next;
  } sDecodedPicture;
//}}}
//{{{  sCodingParam
typedef struct CodingParam {
  int layerId;
  int profileIdc;
  int width;
  int height;
  int widthCr;   // width chroma
  int heightCr;  // height chroma

  int picUnitBitSizeDisk;
  short bitdepthLuma;
  short bitdepthChroma;
  int bitdepth_scale[2];
  int bitdepth_luma_qp_scale;
  int bitdepthChromaQpScale;
  unsigned int dcPredValueComp[MAX_PLANE]; // component value for DC prediction (depends on component pel bit depth)
  int maxPelValueComp[MAX_PLANE];          // max value that one picture element (pixel) can take (depends on pic_unit_bitdepth)

  int yuvFormat;
  int lossless_qpprime_flag;
  int numBlock8x8uv;
  int numUvBlocks;
  int numCdcCoeff;
  int mbCrSizeX;
  int mbCrSizeY;
  int mb_cr_size_x_blk;
  int mb_cr_size_y_blk;
  int mbCrSize;
  int mbSize[3][2];      // component macroblock dimensions
  int mbSizeBlock[3][2];  // component macroblock dimensions
  int mbSizeShift[3][2];

  int max_vmv_r;                  // maximum vertical motion vector range in luma quarter frame pixel units for the current level_idc
  int sepColourPlaneFlag;
  int ChromaArrayType;
  int max_frame_num;

  unsigned int PicWidthInMbs;
  unsigned int PicHeightInMapUnits;
  unsigned int FrameHeightInMbs;
  unsigned int FrameSizeInMbs;

  int iLumaPadX;
  int iLumaPadY;
  int iChromaPadX;
  int iChromaPadY;

  int subpelX;
  int subpelY;
  int shiftpelX;
  int shiftpelY;
  int totalScale;
  unsigned int oldFrameSizeInMbs;

  sMacroblock* mbData;               // array containing all MBs of a whole frame
  sMacroblock* mbDataJV[MAX_PLANE]; // mbData to be used for 4:4:4 independent mode

  char*        intraBlock;
  char*        intraBlockJV[MAX_PLANE];
  sBlockPos*   picPos;
  byte**       predMode;             // prediction type [90][74]
  byte**       predModeJV[MAX_PLANE];
  byte****     nzCoeff;

  int**        siBlock;
  int**        siBlockJV[MAX_PLANE];
  int*         qpPerMatrix;
  int*         qpRemMatrix;
  } sCodingParam;
//}}}
//{{{  sLayerParam
typedef struct LayerParam {
  int              layerId;
  struct VidParam* vidParam;
  sCodingParam*    codingParam;
  sSPS*            sps;
  struct DPB*      dpb;
  } sLayerParam;
//}}}
//{{{  sVidParam
typedef struct VidParam {
  struct InputParam* inputParam;

  TIME_T startTime;
  TIME_T endTime;

  sPPS* activePPS;
  sSPS* activeSPS;
  sSPS  SeqParSet[32];
  sPPS  PicParSet[MAX_PPS];

  struct DPB*   dpbLayer[MAX_NUM_DPB_LAYERS];
  sCodingParam* codingParam[MAX_NUM_DPB_LAYERS];
  sLayerParam*  layerParam[MAX_NUM_DPB_LAYERS];

  struct sei_params*    sei;
  struct OldSliceParam* oldSlice;
  int                   number;  //frame number

  // current picture property;
  unsigned int numDecodedMb;
  int          curPicSliceNum;
  int          numSlicesAllocated;
  int          numSlicesDecoded;
  sSlice**     sliceList;
  char*        intraBlock;
  char*        intraBlockJV[MAX_PLANE];

  int          type;                       // image type INTER/INTRA

  byte**       predMode;                  // prediction type [90][74]
  byte**       predModeJV[MAX_PLANE];
  byte****     nzCoeff;
  int**        siBlock;
  int**        siBlockJV[MAX_PLANE];
  sBlockPos*   picPos;

  int          newFrame;
  int          structure;           // Identify picture structure type

  sSlice*      nextSlice;           // pointer to first sSlice of next picture;
  sMacroblock* mbData;              // array containing all MBs of a whole frame
  sMacroblock* mbDataJV[MAX_PLANE]; // mbData to be used for 4:4:4 independent mode
  int          ChromaArrayType;

  // picture error conceal
  // concealment_head points to first node in list, concealment_end points to
  // last node in list. Initialize both to NULL, meaning no nodes in list yet
  struct ConcealNode* concealment_head;
  struct ConcealNode* concealment_end;

  unsigned int preFrameNum;           // store the frameNum in the last decoded slice. For detecting gap in frameNum.
  int          nonConformingStream;

  // for POC mode 0:
  signed int   PrevPicOrderCntMsb;
  unsigned int PrevPicOrderCntLsb;

  // for POC mode 1:
  signed int   ExpectedPicOrderCnt, PicOrderCntCycleCnt, FrameNumInPicOrderCntCycle;
  unsigned int PreviousFrameNum, FrameNumOffset;
  int          ExpectedDeltaPerPicOrderCntCycle;
  int          thisPoc;
  int          PreviousFrameNumOffset;

  unsigned int picHeightInMbs;
  unsigned int picSizeInMbs;

  int          noOutputPriorPicFlag;

  int          last_has_mmco_5;
  int          last_pic_bottom_field;

  int          idrPsnrNum;
  int          psnrNum;

  // picture error conceal
  int          lastRefPicPoc;
  int          refPocGap;
  int          pocGap;
  int          concealMode;
  int          earlier_missing_poc;
  unsigned int frame_to_conceal;
  int          IdrConcealFlag;
  int          conceal_slice_type;

  Boolean      firstSPS;
  int          recovery_point;
  int          recoveryPointFound;
  int          recoveryFrameCount;
  int          recoveryFrameNum;
  int          recoveryPoc;

  // Redundant slices. Should be moved to another structure and allocated only if extended profile
  unsigned int prevFrameNum; // frame number of previous slice

  // non-zero: i-th previous frame is correct
  int          isPrimaryOk;    // if primary frame is correct, 0: incorrect
  int          isReduncantOk;  // if redundant frame is correct, 0:incorrect

  struct       annexBstruct* annexB;
  int          LastAccessUnitExists;
  int          NALUCount;
  struct nalu_t* nalu;

  int          frameNum;
  int          gapNumFrame; // ???
  Boolean      globalInitDone[2];

  int*         qpPerMatrix;
  int*         qpRemMatrix;

  int          pocs_in_dpb[100];

  struct FrameStore* lastOutFramestore;

  struct Picture* picture;
  struct Picture* dec_picture_JV[MAX_PLANE];  // picture to be used during 4:4:4 independent mode decoding
  struct Picture* noReferencePicture;       // dummy storable picture for recovery point

  // Error parameters
  struct ObjectBuffer*  ercObjectList;
  struct ErcVariables* ercErrorVar;
  int                    ercMvPerMb;
  struct VidParam*       ercImg;
  int                    ecFlag[SE_MAX_ELEMENTS];  // array to set errorconcealment

  struct FrameStore* outBuffer;
  struct Picture*  pendingOut;
  int              pendingOutState;
  int              recoveryFlag;

  // report
  char sliceTypeText[9];

  // FMO
  int* MbToSliceGroupMap;
  int* MapUnitToSliceGroupMap;
  int  NumberOfSliceGroups;  // the number of slice groups -1 (0 == scan order, 7 == maximum)

  void (*getNeighbour)     (sMacroblock *curMb, int xN, int yN, int mbSize[2], sPixelPos *pix);
  void (*get_mb_block_pos) (sBlockPos *picPos, int mbAddr, short *x, short *y);
  void (*GetStrengthVer)   (sMacroblock *MbQ, int edge, int mvlimit, struct Picture *p);
  void (*GetStrengthHor)   (sMacroblock *MbQ, int edge, int mvlimit, struct Picture *p);
  void (*EdgeLoopLumaVer)  (eColorPlane pl, sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge);
  void (*EdgeLoopLumaHor)  (eColorPlane pl, sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, struct Picture *p);
  void (*EdgeLoopChromaVer)(sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, int uv, struct Picture *p);
  void (*EdgeLoopChromaHor)(sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, int uv, struct Picture *p);

  sImage       tempData3;
  sDecodedPicture* decOutputPic;
  int          deblockMode;  // 0: deblock in picture, 1: deblock in slice;

  int   iLumaPadX;
  int   iLumaPadY;
  int   iChromaPadX;
  int   iChromaPadY;

  // control;
  int   bDeblockEnable;
  int   iPostProcess;
  int   bFrameInit;
  sPPS* nextPPS;
  int   last_dec_poc;
  int   last_dec_view_id;
  int   last_dec_layer_id;
  int   dpbLayerId;

  int   width;
  int   height;
  int   widthCr;
  int   heightCr;

  // Fidelity Range Extensions Stuff
  int picUnitBitSizeDisk;
  short bitdepthLuma;
  short bitdepthChroma;
  int   bitdepth_scale[2];
  int   bitdepth_luma_qp_scale;
  int   bitdepthChromaQpScale;
  unsigned int dcPredValueComp[MAX_PLANE]; // component value for DC prediction (depends on component pel bit depth)
  int  maxPelValueComp[MAX_PLANE];          // max value that one picture element (pixel) can take (depends on pic_unit_bitdepth)

  int sepColourPlaneFlag;
  int pic_unit_size_on_disk;

  int profileIdc;
  int yuvFormat;
  int lossless_qpprime_flag;
  int numBlock8x8uv;
  int numUvBlocks;
  int numCdcCoeff;
  int mbCrSizeX;
  int mbCrSizeY;
  int mb_cr_size_x_blk;
  int mb_cr_size_y_blk;
  int mbCrSize;
  int mbSize[3][2];       // component macroblock dimensions
  int mbSizeBlock[3][2];   // component macroblock dimensions
  int mbSizeShift[3][2];
  int subpelX;
  int subpelY;
  int shiftpelX;
  int shiftpelY;
  int totalScale;
  int max_frame_num;

  unsigned int PicWidthInMbs;
  unsigned int PicHeightInMapUnits;
  unsigned int FrameHeightInMbs;
  unsigned int FrameSizeInMbs;
  unsigned int oldFrameSizeInMbs;

  // maximum vertical motion vector range in luma quarter frame pixel units for the current level_idc
  int max_vmv_r;
  } sVidParam;
//}}}
//{{{  sDecoderParam
typedef struct DecoderParam {
  sInputParam* inputParam;
  sVidParam*   vidParam;
  } sDecoderParam;
//}}}

typedef sBiContextType* sBiContextTypePtr;
static const sMotionVector zero_mv = {0, 0};
//{{{
static inline int is_BL_profile (unsigned int profileIdc) {
  return profileIdc == FREXT_CAVLC444 || profileIdc == BASELINE ||
         profileIdc == MAIN || profileIdc == EXTENDED ||
         profileIdc == FREXT_HP || profileIdc == FREXT_Hi10P ||
         profileIdc == FREXT_Hi422 || profileIdc == FREXT_Hi444;
}
//}}}
//{{{
static inline int is_FREXT_profile (unsigned int profileIdc) {
  // we allow all FRExt tools, when no profile is active
  return profileIdc == NO_PROFILE || profileIdc == FREXT_HP ||
         profileIdc == FREXT_Hi10P || profileIdc == FREXT_Hi422 ||
         profileIdc == FREXT_Hi444 || profileIdc == FREXT_CAVLC444;
}
//}}}
//{{{
static inline int is_HI_intra_only_profile (unsigned int profileIdc, Boolean constrained_set3_flag) {
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
  extern sDecoderParam* gDecoder;

  extern char errortext[ET_SIZE];
  extern void error (char* text, int code);

  extern void initGlobalBuffers (sVidParam* vidParam, int layerId);
  extern void freeGlobalBuffers (sVidParam* vidParam);
  extern void freeLayerBuffers (sVidParam* vidParam, int layerId);

  extern sDataPartition* allocPartition (int n);
  extern void freePartition (sDataPartition* dp, int n);

  extern sSlice* allocSlice (sInputParam* inputParam, sVidParam* vidParam);

  extern unsigned ceilLog2 (unsigned uiVal);
  extern unsigned ceilLog2sf (unsigned uiVal);

  // For 4:4:4 independent mode
  extern void change_plane_JV (sVidParam* vidParam, int nplane, sSlice *pSlice);
  extern void make_frame_picture_JV (sVidParam* vidParam );

  extern sDecodedPicture* getDecodedPicture (sDecodedPicture* decodedPicture);
  extern void freeDecodedPictures (sDecodedPicture* decodedPicture);
  extern void clearDecodedPictures (sVidParam* vidParam);

  extern void setGlobalCodingProgram (sVidParam* vidParam, sCodingParam* codingParam);
//{{{
#ifdef __cplusplus
}
#endif
//}}}
