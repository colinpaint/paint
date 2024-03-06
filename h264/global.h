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
#include "ifunctions.h"
#include "parsetcommon.h"
#include "types.h"
#include "frame.h"
//}}}

typedef struct bit_stream_dec Bitstream;
#define ET_SIZE 300      //!< size of error text buffer
extern char errortext[ET_SIZE]; //!< buffer for error message for exit with error()

//{{{  DecoderStatus_e
typedef enum {
  DEC_OPENED = 0,
  DEC_STOPPED,
  } DecoderStatus_e;
//}}}
//{{{  Color_Component
typedef enum {
  LumaComp = 0,
  CrComp = 1,
  CbComp = 2
  } Color_Component;
//}}}
//{{{  sPixelPos
typedef struct pix_pos {
  int   available;
  int   mb_addr;
  short x;
  short y;
  short pos_x;
  short pos_y;
  } sPixelPos;
//}}}
//{{{  DecodingEnvironment
typedef struct {
  unsigned int Drange;
  unsigned int Dvalue;
  int DbitsLeft;
  byte* Dcodestrm;
  int* Dcodestrm_len;
  } DecodingEnvironment;
typedef DecodingEnvironment *DecodingEnvironmentPtr;
//}}}
//{{{  sMotionVector
typedef struct {
  short mv_x;
  short mv_y;
  } sMotionVector;
//}}}
static const sMotionVector zero_mv = {0, 0};
//{{{  sBlockPos
typedef struct {
  short x;
  short y;
  } sBlockPos;
//}}}
//{{{  BiContextType
//! struct for context management
typedef struct {
  uint16 state;         // index into state-table CP
  unsigned char  MPS;           // Least Probable Symbol 0/1 CP
  unsigned char dummy;          // for alignment
  } BiContextType;
//}}}
typedef BiContextType *BiContextTypePtr;
//{{{  CTX defines
#define NUM_MB_TYPE_CTX  11
#define NUM_B8_TYPE_CTX  9
#define NUM_MV_RES_CTX   10
#define NUM_REF_NO_CTX   6
#define NUM_DELTA_QP_CTX 4
#define NUM_MB_AFF_CTX 4
#define NUM_TRANSFORM_SIZE_CTX 3
//}}}

struct storablePicture;
struct datapartition_dec;
struct syntaxelement_dec;
struct pic_motion_params;
struct pic_motion_params_old;
//{{{  MotionInfoContexts
typedef struct {
  BiContextType mb_type_contexts [3][NUM_MB_TYPE_CTX];
  BiContextType b8_type_contexts [2][NUM_B8_TYPE_CTX];
  BiContextType mv_res_contexts  [2][NUM_MV_RES_CTX];
  BiContextType ref_no_contexts  [2][NUM_REF_NO_CTX];
  BiContextType delta_qp_contexts[NUM_DELTA_QP_CTX];
  BiContextType mb_aff_contexts  [NUM_MB_AFF_CTX];
  } MotionInfoContexts;
//}}}
//{{{  texture context defines
#define NUM_IPR_CTX    2
#define NUM_CIPR_CTX   4
#define NUM_CBP_CTX    4
#define NUM_BCBP_CTX   4
#define NUM_MAP_CTX   15
#define NUM_LAST_CTX  15
#define NUM_ONE_CTX    5
#define NUM_ABS_CTX    5
//}}}
//{{{  TextureInfoContexts
typedef struct {
  BiContextType transform_size_contexts [NUM_TRANSFORM_SIZE_CTX];
  BiContextType ipr_contexts [NUM_IPR_CTX];
  BiContextType cipr_contexts[NUM_CIPR_CTX];
  BiContextType cbp_contexts [3][NUM_CBP_CTX];
  BiContextType bcbp_contexts[NUM_BLOCK_TYPES][NUM_BCBP_CTX];
  BiContextType map_contexts [2][NUM_BLOCK_TYPES][NUM_MAP_CTX];
  BiContextType last_contexts[2][NUM_BLOCK_TYPES][NUM_LAST_CTX];
  BiContextType one_contexts [NUM_BLOCK_TYPES][NUM_ONE_CTX];
  BiContextType abs_contexts [NUM_BLOCK_TYPES][NUM_ABS_CTX];
  } TextureInfoContexts;
//}}}
//{{{
/*! Buffer structure for decoded referenc picture marking commands */
typedef struct DecRefPicMarking_s {
  int memory_management_control_operation;
  int difference_of_pic_nums_minus1;
  int long_term_pic_num;
  int long_term_frame_idx;
  int max_long_term_frame_idx_plus1;
  struct DecRefPicMarking_s *Next;
  } DecRefPicMarking_t;
//}}}
//{{{
//! cbp structure
typedef struct cbp_s {
  int64 blk;
  int64 bits;
  int64 bits_8x8;
  } CBPStructure;
//}}}
//{{{
//! sMacroblock
typedef struct macroblock_dec {
  struct slice*     p_Slice;  // pointer to the current slice
  struct video_par* vidParam;    // pointer to sVidParam
  struct inp_par*   p_Inp;

  int mbAddrX;                // current MB address
  int mbAddrA;
  int mbAddrB;
  int mbAddrC;
  int mbAddrD;

  Boolean mbAvailA;
  Boolean mbAvailB;
  Boolean mbAvailC;
  Boolean mbAvailD;

  sBlockPos mb;
  int block_x;
  int block_y;
  int block_y_aff;
  int pix_x;
  int pix_y;
  int pix_c_x;
  int pix_c_y;

  int subblock_x;
  int subblock_y;

  int     qp;                    // QP luma
  int     qpc[2];                // QP chroma
  int     qp_scaled[MAX_PLANE];  // QP scaled for all comps.
  Boolean is_lossless;
  Boolean is_intra_block;
  Boolean is_v_block;
  int     DeblockCall;

  short slice_nr;
  char  ei_flag;            // error indicator flag that enables concealment
  char  dpl_flag;           // error indicator flag that signals a missing data partition
  short delta_quant;        // for rate control
  short list_offset;

  struct macroblock_dec   *mb_up;   // pointer to neighboring MB (CABAC)
  struct macroblock_dec   *mb_left; // pointer to neighboring MB (CABAC)

  struct macroblock_dec   *mbup;    // neighbors for loopfilter
  struct macroblock_dec   *mbleft;  // neighbors for loopfilter

  // some storage of macroblock syntax elements for global access
  short         mb_type;
  short         mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];      //!< indices correspond to [forw,backw][block_y][block_x][x,y]
  int           cbp;
  CBPStructure  s_cbp[3];

  int   i16mode;
  char  b8mode[4];
  char  b8pdir[4];
  char  ipmode_DPCM;
  char  c_ipred_mode;       //!< chroma intra prediction mode
  char  skip_flag;
  short DFDisableIdc;
  short DFAlphaC0Offset;
  short DFBetaOffset;

  Boolean mb_field;
  //Flag for MBAFF deblocking;
  byte  mixedModeEdgeFlag;

  // deblocking strength indices
  byte strength_ver[4][4];
  byte strength_hor[4][16];

  Boolean       luma_transform_size_8x8_flag;
  Boolean       NoMbPartLessThan8x8Flag;

  void (*itrans_4x4)(struct macroblock_dec *currMB, sColorPlane pl, int ioff, int joff);
  void (*itrans_8x8)(struct macroblock_dec *currMB, sColorPlane pl, int ioff, int joff);

  void (*GetMVPredictor) (struct macroblock_dec *currMB, sPixelPos *block,
    sMotionVector *pmv, short ref_frame, struct pic_motion_params** mv_info, int list, int mb_x, int mb_y, int blockshape_x, int blockshape_y);

  int  (*read_and_store_CBP_block_bit)  (struct macroblock_dec *currMB, DecodingEnvironmentPtr  dep_dp, int type);
  char (*readRefPictureIdx)             (struct macroblock_dec *currMB, struct syntaxelement_dec *currSE, struct datapartition_dec *dP, char b8mode, int list);

  void (*read_comp_coeff_4x4_CABAC)     (struct macroblock_dec *currMB, struct syntaxelement_dec *currSE, sColorPlane pl, int (*InvLevelScale4x4)[4], int qp_per, int cbp);
  void (*read_comp_coeff_8x8_CABAC)     (struct macroblock_dec *currMB, struct syntaxelement_dec *currSE, sColorPlane pl);

  void (*read_comp_coeff_4x4_CAVLC)     (struct macroblock_dec *currMB, sColorPlane pl, int (*InvLevelScale4x4)[4], int qp_per, int cbp, byte** nzcoeff);
  void (*read_comp_coeff_8x8_CAVLC)     (struct macroblock_dec *currMB, sColorPlane pl, int (*InvLevelScale8x8)[8], int qp_per, int cbp, byte** nzcoeff);
  } sMacroblock;
//}}}
//{{{
//! Syntaxelement
typedef struct syntaxelement_dec {
  int           type;                  //!< type of syntax element for data part.
  int           value1;                //!< numerical value of syntax element
  int           value2;                //!< for blocked symbols, e.g. run/level
  int           len;                   //!< length of code
  int           inf;                   //!< info part of CAVLC code
  unsigned int  bitpattern;            //!< CAVLC bitpattern
  int           context;               //!< CABAC context
  int           k;                     //!< CABAC context for coeff_count,uv

  //! for mapping of CAVLC to syntaxElement
  void  (*mapping)(int len, int info, int *value1, int *value2);
  //! used for CABAC: refers to actual coding method of each individual syntax element type
  void  (*reading)(struct macroblock_dec *currMB, struct syntaxelement_dec *, DecodingEnvironmentPtr);
  } SyntaxElement;
//}}}
//{{{
//! Bitstream
struct bit_stream_dec {
  // CABAC Decoding
  int read_len;          // actual position in the codebuffer, CABAC only
  int code_len;          // overall codebuffer length, CABAC only

  // CAVLC Decoding
  int frame_bitoffset;   // actual position in the codebuffer, bit-oriented, CAVLC only
  int bitstream_length;  // over codebuffer lnegth, byte oriented, CAVLC only

  // ErrorConcealment
  byte* streamBuffer;     // actual codebuffer for read bytes
  int ei_flag;           // error indication, 0: no error, else unspecified error
  };
//}}}
//{{{  typedef struct sDataPartition
typedef struct datapartition_dec {
  Bitstream           *bitstream;
  DecodingEnvironment de_cabac;

  int     (*readSyntaxElement)(struct macroblock_dec *currMB, struct syntaxelement_dec *, struct datapartition_dec *);
          /*!< virtual function;
               actual method depends on chosen data partition and
               entropy coding method  */
  } sDataPartition;
//}}}
//{{{
typedef struct wp_params {
  short weight[3];
  short offset[3];
  } WPParams;
//}}}
//{{{
typedef struct inp_par {
  int ref_offset;
  int poc_scale;
  int write_uv;
  int intra_profile_deblocking;         // Loop filter usage determined by flags and parameters in bitstream

  // Input/output sequence format related variables
  sFrameFormat source;                   // source related information
  sFrameFormat output;                   // output related information

  // error concealment
  int conceal_mode;
  int ref_poc_gap;
  int poc_gap;

  // Needed to allow compilation for decoder. May be used later for distortion computation operations
  int stdRange;                         // 1 - standard range, 0 - full range
  int videoCode;                        // 1 - 709, 3 - 601:  See VideoCode in io_tiff.
  int export_views;

  int dpb_plus[2];

  int vlcDebug;
  } InputParameters;
//}}}
//{{{
typedef struct image_data {
  sFrameFormat format;                  // image format

  sPixel** frm_data[MAX_PLANE];        // Frame Data
  sPixel** top_data[MAX_PLANE];        // pointers to top field data
  sPixel** bot_data[MAX_PLANE];        // pointers to bottom field data

  sPixel** frm_data_buf[2][MAX_PLANE]; // Frame Data
  sPixel** top_data_buf[2][MAX_PLANE]; // pointers to top field data
  sPixel** bot_data_buf[2][MAX_PLANE]; // pointers to bottom field data

  int frm_stride[MAX_PLANE];
  int top_stride[MAX_PLANE];
  int bot_stride[MAX_PLANE];
  } ImageData;
//}}}
//{{{
typedef struct old_slice_par {
  unsigned field_pic_flag;
  unsigned frame_num;
  int      nal_ref_idc;
  unsigned pic_oder_cnt_lsb;
  int      delta_pic_oder_cnt_bottom;
  int      delta_pic_order_cnt[2];
  byte     bottom_field_flag;
  byte     idr_flag;
  int      idr_pic_id;
  int      pps_id;
  int      layer_id;
  } OldSliceParams;
//}}}
//{{{
typedef struct slice {
  struct video_par* vidParam;
  struct inp_par* p_Inp;
  sPPSrbsp* active_pps;
  sSPSrbsp* active_sps;

  // dpb pointer
  struct decoded_picture_buffer* dpb;

  // slice property;
  int idr_flag;
  int idr_pic_id;
  int nal_reference_idc;                       // nal_reference_idc from NAL unit
  int Transform8x8Mode;
  Boolean chroma444_not_separate;              // indicates chroma 4:4:4 coding with separate_colour_plane_flag equal to zero

  int toppoc;      // poc for this top field
  int bottompoc;   // poc of bottom field of frame
  int framepoc;    // poc of this frame

  // poc mode 0.
  unsigned int pic_order_cnt_lsb;
  int delta_pic_order_cnt_bottom;

  // poc mode 1.
  int delta_pic_order_cnt[2];

  // POC mode 0:
  signed int PicOrderCntMsb;

  // POC mode 1:
  unsigned int AbsFrameNum;
  int ThisPOC;

  // information need to move to slice;
  unsigned int current_mb_nr; // bitstream order
  unsigned int num_dec_mb;
  short current_slice_nr;
  int cod_counter;                   // Current count of number of skipped macroblocks in a row
  int allrefzero;

  int                 mb_aff_frame_flag;
  int                 direct_spatial_mv_pred_flag;       //!< Indicator for direct mode type (1 for Spatial, 0 for Temporal)
  int                 num_ref_idx_active[2];             //!< number of available list references

  int                 ei_flag;       //!< 0 if the partArr[0] contains valid information
  int                 qp;
  int                 slice_qp_delta;
  int                 qs;
  int                 slice_qs_delta;
  int                 slice_type;    //!< slice type
  int                 model_number;  //!< cabac model number
  unsigned int        frame_num;   //frame_num for this frame
  unsigned int        field_pic_flag;
  byte                bottom_field_flag;
  sPictureStructure    structure;     //!< Identify picture structure type
  int                 start_mb_nr;   //!< MUST be set by NAL even in case of ei_flag == 1
  int                 end_mb_nr_plus1;
  int                 max_part_nr;
  int                 dp_mode;       //!< data partitioning mode
  int                 current_header;
  int                 next_header;
  int                 last_dquant;

  // slice header information;
  int colour_plane_id;               //!< colour_plane_id of the current coded slice
  int redundant_pic_cnt;
  int sp_switch;                              //!< 1 for switching sp, 0 for normal sp
  int slice_group_change_cycle;
  int redundant_slice_ref_idx;     //!< reference index of redundant slice
  int no_output_of_prior_pics_flag;
  int long_term_reference_flag;
  int adaptive_ref_pic_buffering_flag;
  DecRefPicMarking_t *dec_ref_pic_marking_buffer;                    //!< stores the memory management control operations

  char listXsize[6];
  struct storablePicture** listX[6];

  sDataPartition       *partArr;      //!< array of partitions
  MotionInfoContexts  *mot_ctx;      //!< pointer to struct of context models for use in CABAC
  TextureInfoContexts *tex_ctx;      //!< pointer to struct of context models for use in CABAC

  int mvscale[6][MAX_REFERENCE_PICTURES];

  int                 ref_pic_list_reordering_flag[2];
  int                 *modification_of_pic_nums_idc[2];
  int                 *abs_diff_pic_num_minus1[2];
  int                 *long_term_pic_idx[2];

  int                 layer_id;
  short               DFDisableIdc;     //!< Disable deblocking filter on slice
  short               DFAlphaC0Offset;  //!< Alpha and C0 offset for filtering slice
  short               DFBetaOffset;     //!< Beta offset for filtering slice
  int                 pic_parameter_set_id;   //!<the ID of the picture parameter set the slice is reffering to
  int                 dpB_NotPresent;    //!< non-zero, if data partition B is lost
  int                 dpC_NotPresent;    //!< non-zero, if data partition C is lost

  Boolean is_reset_coeff;
  Boolean is_reset_coeff_cr;
  sPixel ** *mb_pred;
  sPixel ** *mb_rec;
  int    ** *mb_rres;
  int    ** *cof;
  int    ** *fcf;
  int cofu[16];

  sPixel** tmp_block_l0;
  sPixel** tmp_block_l1;
  int   ** tmp_res;
  sPixel** tmp_block_l2;
  sPixel** tmp_block_l3;

  // Scaling matrix info
  int  InvLevelScale4x4_Intra[3][6][4][4];
  int  InvLevelScale4x4_Inter[3][6][4][4];
  int  InvLevelScale8x8_Intra[3][6][8][8];
  int  InvLevelScale8x8_Inter[3][6][8][8];

  int* qmatrix[12];

  // Cabac
  int  coeff[64]; // one more for EOB
  int  coeff_ctr;
  int  pos;

  //weighted prediction
  unsigned short weighted_pred_flag;
  unsigned short weighted_bipred_idc;

  unsigned short luma_log2_weight_denom;
  unsigned short chroma_log2_weight_denom;

  WPParams** wp_params; // wp parameters in [list][index]
  int*** wp_weight;  // weight in [list][index][component] order
  int*** wp_offset;  // offset in [list][index][component] order
  int**** wbp_weight; //weight in [list][fw_index][bw_index][component] order
  short wp_round_luma;
  short wp_round_chroma;

  // for signalling to the neighbour logic that this is a deblocker call
  int max_mb_vmv_r;                          //!< maximum vertical motion vector range in luma quarter pixel units for the current level_idc
  int ref_flag[17];                //!< 0: i-th previous frame is incorrect

  int erc_mvperMB;
  sMacroblock *mb_data;
  struct storablePicture* picture;
  int** siblock;
  byte** ipredmode;
  char  *intra_block;
  char  chroma_vector_adjustment[6][32];
  void (*read_CBP_and_coeffs_from_NAL) (sMacroblock *currMB);
  int  (*decode_one_component     )    (sMacroblock *currMB, sColorPlane curPlane, sPixel** curPixel, struct storablePicture* picture);
  int  (*readSlice                )    (struct video_par *, struct inp_par *);
  int  (*nal_startcode_follows    )    (struct slice*, int );
  void (*read_motion_info_from_NAL)    (sMacroblock *currMB);
  void (*read_one_macroblock      )    (sMacroblock *currMB);
  void (*interpret_mb_mode        )    (sMacroblock *currMB);
  void (*init_lists               )    (struct slice *currSlice);

  void (*intra_pred_chroma        )    (sMacroblock *currMB);
  int  (*intra_pred_4x4)               (sMacroblock *currMB, sColorPlane pl, int ioff, int joff,int i4,int j4);
  int  (*intra_pred_8x8)               (sMacroblock *currMB, sColorPlane pl, int ioff, int joff);
  int  (*intra_pred_16x16)             (sMacroblock *currMB, sColorPlane pl, int predmode);

  void (*linfo_cbp_intra          )    (int len, int info, int *cbp, int *dummy);
  void (*linfo_cbp_inter          )    (int len, int info, int *cbp, int *dummy);
  void (*update_direct_mv_info    )    (sMacroblock *currMB);
  void (*read_coeff_4x4_CAVLC     )    (sMacroblock *currMB, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);
  } sSlice;
//}}}
//{{{
typedef struct decodedpic_t {
  int bValid;                 // 0: invalid, 1: valid, 3: valid for 3D output;
  int iViewId;                // -1: single view, >=0 multiview[VIEW1|VIEW0];
  int iPOC;

  int iYUVFormat;             // 0: 4:0:0, 1: 4:2:0, 2: 4:2:2, 3: 4:4:4
  int iYUVStorageFormat;      // 0: YUV seperate; 1: YUV interleaved; 2: 3D output;
  int iBitDepth;

  byte* pY;                   // if iPictureFormat is 1, [0]: top; [1] bottom;
  byte* pU;
  byte* pV;

  int iWidth;                 // frame width;
  int iHeight;                // frame height;
  int iYBufStride;            // stride of pY[0/1] buffer in bytes;
  int iUVBufStride;           // stride of pU[0/1] and pV[0/1] buffer in bytes;
  int iSkipPicNum;
  int iBufSize;

  struct decodedpic_t *pNext;
  } sDecodedPicList;
//}}}
//{{{
typedef struct coding_par {
  int layer_id;
  int profile_idc;
  int width;
  int height;
  int width_cr;                               //!< width chroma
  int height_cr;                              //!< height chroma

  int pic_unit_bitsize_on_disk;
  short bitdepth_luma;
  short bitdepth_chroma;
  int bitdepth_scale[2];
  int bitdepth_luma_qp_scale;
  int bitdepth_chroma_qp_scale;
  unsigned int dc_pred_value_comp[MAX_PLANE]; //!< component value for DC prediction (depends on component pel bit depth)
  int max_pel_value_comp[MAX_PLANE];       //!< max value that one picture element (pixel) can take (depends on pic_unit_bitdepth)

  int yuv_format;
  int lossless_qpprime_flag;
  int num_blk8x8_uv;
  int num_uv_blocks;
  int num_cdc_coeff;
  int mb_cr_size_x;
  int mb_cr_size_y;
  int mb_cr_size_x_blk;
  int mb_cr_size_y_blk;
  int mb_cr_size;
  int mb_size[3][2];                         //!< component macroblock dimensions
  int mb_size_blk[3][2];                     //!< component macroblock dimensions
  int mb_size_shift[3][2];

  int max_vmv_r;                             //!< maximum vertical motion vector range in luma quarter frame pixel units for the current level_idc
  int separate_colour_plane_flag;
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

  int subpel_x;
  int subpel_y;
  int shiftpel_x;
  int shiftpel_y;
  int total_scale;
  unsigned int oldFrameSizeInMbs;

  // padding info;
  int rgb_output;
  sMacroblock* mb_data;               //!< array containing all MBs of a whole frame
  sMacroblock* mb_data_JV[MAX_PLANE]; //!< mb_data to be used for 4:4:4 independent mode
  char* intra_block;
  char* intra_block_JV[MAX_PLANE];
  sBlockPos *PicPos;
  byte** ipredmode;                  //!< prediction type [90][74]
  byte** ipredmode_JV[MAX_PLANE];
  byte**** nz_coeff;
  int** siblock;
  int** siblock_JV[MAX_PLANE];
  int* qp_per_matrix;
  int* qp_rem_matrix;
  } sCodingParams;
//}}}
//{{{
typedef struct layer_par {
  int layer_id;
  struct video_par* vidParam;
  sCodingParams* p_Cps;
  sSPSrbsp* p_SPS;
  struct decoded_picture_buffer* dpb;
  } LayerParameters;
//}}}
//{{{
typedef struct video_par {
  struct inp_par* p_Inp;

  sPPSrbsp* active_pps;
  sSPSrbsp* active_sps;

  sSPSrbsp SeqParSet[MAXSPS];
  sPPSrbsp PicParSet[MAXPPS];

  struct decoded_picture_buffer* p_Dpb_layer[MAX_NUM_DPB_LAYERS];
  sCodingParams* p_EncodePar[MAX_NUM_DPB_LAYERS];
  LayerParameters* p_LayerPar[MAX_NUM_DPB_LAYERS];

  struct sei_params* p_SEI;
  struct old_slice_par* old_slice;
  int number;                       //frame number

  // current picture property;
  unsigned int num_dec_mb;
  int iSliceNumOfCurrPic;
  int iNumOfSlicesAllocated;
  int iNumOfSlicesDecoded;
  sSlice** ppSliceList;
  char* intra_block;
  char* intra_block_JV[MAX_PLANE];

  int type;                          // image type INTER/INTRA

  byte** ipredmode;                  // prediction type [90][74]
  byte** ipredmode_JV[MAX_PLANE];
  byte**** nz_coeff;
  int** siblock;
  int** siblock_JV[MAX_PLANE];
  sBlockPos* PicPos;

  int newframe;
  int structure;                     // Identify picture structure type

  sSlice* pNextSlice;                 // pointer to first sSlice of next picture;
  sMacroblock* mb_data;               // array containing all MBs of a whole frame
  sMacroblock* mb_data_JV[MAX_PLANE]; // mb_data to be used for 4:4:4 independent mode
  int ChromaArrayType;

  // picture error concealment
  // concealment_head points to first node in list, concealment_end points to
  // last node in list. Initialize both to NULL, meaning no nodes in list yet
  struct concealment_node* concealment_head;
  struct concealment_node* concealment_end;

  unsigned int pre_frame_num;           // store the frame_num in the last decoded slice. For detecting gap in frame_num.
  int non_conforming_stream;

  // for POC mode 0:
  signed int PrevPicOrderCntMsb;
  unsigned int PrevPicOrderCntLsb;

  // for POC mode 1:
  signed int ExpectedPicOrderCnt, PicOrderCntCycleCnt, FrameNumInPicOrderCntCycle;
  unsigned int PreviousFrameNum, FrameNumOffset;
  int ExpectedDeltaPerPicOrderCntCycle;
  int ThisPOC;
  int PreviousFrameNumOffset;

  unsigned int PicHeightInMbs;
  unsigned int PicSizeInMbs;

  int no_output_of_prior_pics_flag;

  int last_has_mmco_5;
  int last_pic_bottom_field;

  int idr_psnr_number;
  int psnr_number;

  // Timing related variables
  TIME_T start_time;
  TIME_T end_time;
  int64 tot_time;

  // picture error concealment
  int last_ref_pic_poc;
  int ref_poc_gap;
  int poc_gap;
  int conceal_mode;
  int earlier_missing_poc;
  unsigned int frame_to_conceal;
  int IDR_concealment_flag;
  int conceal_slice_type;

  Boolean first_sps;
  int recovery_point;
  int recovery_point_found;
  int recovery_frame_cnt;
  int recovery_frame_num;
  int recovery_poc;

  byte* buf;
  byte* ibuf;

  // Redundant slices. Should be moved to another structure and allocated only if extended profile
  unsigned int previous_frame_num; // frame number of previous slice

  // non-zero: i-th previous frame is correct
  int Is_primary_correct;          // if primary frame is correct, 0: incorrect
  int Is_redundant_correct;        // if redundant frame is correct, 0:incorrect

  struct annexBstruct* annex_b;
  int LastAccessUnitExists;
  int NALUCount;

  int frame_no;
  int g_nFrame;
  Boolean global_init_done[2];

  int* qp_per_matrix;
  int* qp_rem_matrix;

  int pocs_in_dpb[100];
  struct frameStore* last_out_fs;
  struct storablePicture* picture;
  struct storablePicture* dec_picture_JV[MAX_PLANE];  // picture to be used during 4:4:4 independent mode decoding
  struct storablePicture* no_reference_picture;       // dummy storable picture for recovery point

  // Error parameters
  struct object_buffer* erc_object_list;
  struct ercVariables_s* erc_errorVar;
  int erc_mvperMB;
  struct video_par* erc_img;
  int ec_flag[SE_MAX_ELEMENTS];  // array to set errorconcealment

  struct frameStore* out_buffer;

  struct storablePicture* pending_output;
  int    pending_output_state;
  int    recovery_flag;

  // report
  char cslice_type[9];

  // FMO
  int* MbToSliceGroupMap;
  int* MapUnitToSliceGroupMap;
  int NumberOfSliceGroups;  // the number of slice groups -1 (0 == scan order, 7 == maximum)

  void (*getNeighbour)     (sMacroblock *currMB, int xN, int yN, int mb_size[2], sPixelPos *pix);
  void (*get_mb_block_pos) (sBlockPos *PicPos, int mb_addr, short *x, short *y);
  void (*GetStrengthVer)   (sMacroblock *MbQ, int edge, int mvlimit, struct storablePicture *p);
  void (*GetStrengthHor)   (sMacroblock *MbQ, int edge, int mvlimit, struct storablePicture *p);
  void (*EdgeLoopLumaVer)  (sColorPlane pl, sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge);
  void (*EdgeLoopLumaHor)  (sColorPlane pl, sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, struct storablePicture *p);
  void (*EdgeLoopChromaVer)(sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, int uv, struct storablePicture *p);
  void (*EdgeLoopChromaHor)(sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, int uv, struct storablePicture *p);

  ImageData tempData3;
  sDecodedPicList* pDecOuputPic;
  int iDeblockMode;  //0: deblock in picture, 1: deblock in slice;

  struct nalu_t* nalu;
  int iLumaPadX;
  int iLumaPadY;
  int iChromaPadX;
  int iChromaPadY;

  // control;
  int bDeblockEnable;
  int iPostProcess;
  int bFrameInit;
  sPPSrbsp *nextPPS;
  int last_dec_poc;
  int last_dec_view_id;
  int last_dec_layer_id;
  int dpb_layer_id;

  int width;
  int height;
  int width_cr;
  int height_cr;

  // Fidelity Range Extensions Stuff
  int pic_unit_bitsize_on_disk;
  short bitdepth_luma;
  short bitdepth_chroma;
  int bitdepth_scale[2];
  int bitdepth_luma_qp_scale;
  int bitdepth_chroma_qp_scale;
  unsigned int dc_pred_value_comp[MAX_PLANE]; // component value for DC prediction (depends on component pel bit depth)
  int max_pel_value_comp[MAX_PLANE];          // max value that one picture element (pixel) can take (depends on pic_unit_bitdepth)

  int separate_colour_plane_flag;
  int pic_unit_size_on_disk;

  int profile_idc;
  int yuv_format;
  int lossless_qpprime_flag;
  int num_blk8x8_uv;
  int num_uv_blocks;
  int num_cdc_coeff;
  int mb_cr_size_x;
  int mb_cr_size_y;
  int mb_cr_size_x_blk;
  int mb_cr_size_y_blk;
  int mb_cr_size;
  int mb_size[3][2];       // component macroblock dimensions
  int mb_size_blk[3][2];   // component macroblock dimensions
  int mb_size_shift[3][2];
  int subpel_x;
  int subpel_y;
  int shiftpel_x;
  int shiftpel_y;
  int total_scale;
  int max_frame_num;

  unsigned int PicWidthInMbs;
  unsigned int PicHeightInMapUnits;
  unsigned int FrameHeightInMbs;
  unsigned int FrameSizeInMbs;
  unsigned int oldFrameSizeInMbs;
  int max_vmv_r;  // maximum vertical motion vector range in luma quarter frame pixel units for the current level_idc
  } sVidParam;
//}}}
//{{{
typedef struct decoder_params {
  InputParameters* p_Inp; // Input Parameters
  sVidParam* vidParam;    // Image Parameters
  int64      bufferSize;  // buffersize for tiff reads (not currently supported)
  int        bitcounter;
  } sDecoderParams;
//}}}

//{{{
static inline int is_FREXT_profile (unsigned int profile_idc) {
  // we allow all FRExt tools, when no profile is active
  return profile_idc == NO_PROFILE || profile_idc == FREXT_HP ||
         profile_idc == FREXT_Hi10P || profile_idc == FREXT_Hi422 ||
         profile_idc == FREXT_Hi444 || profile_idc == FREXT_CAVLC444;
}
//}}}
//{{{
static inline int is_HI_intra_only_profile (unsigned int profile_idc, Boolean constrained_set3_flag) {
  return (((profile_idc == FREXT_Hi10P)||(profile_idc == FREXT_Hi422) ||
           (profile_idc == FREXT_Hi444)) && constrained_set3_flag) ||
         (profile_idc == FREXT_CAVLC444);
}
//}}}
//{{{
static inline int is_BL_profile (unsigned int profile_idc) {
  return profile_idc == FREXT_CAVLC444 || profile_idc == BASELINE ||
         profile_idc == MAIN || profile_idc == EXTENDED ||
         profile_idc == FREXT_HP || profile_idc == FREXT_Hi10P ||
         profile_idc == FREXT_Hi422 || profile_idc == FREXT_Hi444;
}
//}}}

//{{{
#ifdef __cplusplus
  extern "C" {
#endif
//}}}
  extern sDecoderParams* gDecoder;

  extern void error (char* text, int code);

  extern int init_global_buffers (sVidParam *vidParam, int layer_id );
  extern void free_global_buffers (sVidParam *vidParam);
  extern void free_layer_buffers (sVidParam *vidParam, int layer_id );

  extern sDataPartition* allocPartition (int n);
  extern void freePartition (sDataPartition* dp, int n);

  extern unsigned ceilLog2 (unsigned uiVal);
  extern unsigned ceilLog2_sf (unsigned uiVal);

  // For 4:4:4 independent mode
  extern void change_plane_JV (sVidParam *vidParam, int nplane, sSlice *pSlice);
  extern void make_frame_picture_JV (sVidParam *vidParam );

  extern sDecodedPicList* get_one_avail_dec_pic_from_list (sDecodedPicList *pDecPicList, int b3D, int view_id);
  extern void freeDecPicList (sDecodedPicList *pDecPicList );
  extern void clearDecPicList (sVidParam *vidParam );

  extern sSlice* malloc_slice (InputParameters *p_Inp, sVidParam *vidParam );
  extern void copy_slice_info (sSlice* currSlice, OldSliceParams *p_old_slice );

  extern void set_global_coding_par (sVidParam *vidParam, sCodingParams *cps);
  extern void OpenOutputFiles (sVidParam *vidParam, int view0_id, int view1_id);
//{{{
#ifdef __cplusplus
}
#endif
//}}}
