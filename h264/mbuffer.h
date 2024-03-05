#pragma once
#include "global.h"

#define MAX_LIST_SIZE 33

//{{{
//! definition of pic motion parameters
typedef struct pic_motion_params_old {
  byte *      mb_field;      //!< field macroblock indicator
  } PicMotionParamsOld;
//}}}
//{{{
//! definition of pic motion parameters
typedef struct pic_motion_params {
  struct storable_picture *ref_pic[2];  //!< referrence picture pointer
  MotionVector             mv[2];       //!< motion vector
  char                     ref_idx[2];  //!< reference picture   [list][subblock_y][subblock_x]
  //byte                   mb_field;    //!< field macroblock indicator
  byte                     slice_no;
  } PicMotionParams;
//}}}
//{{{
//! definition a picture (field or frame)
typedef struct storable_picture {
  PictureStructure structure;

  int         poc;
  int         top_poc;
  int         bottom_poc;
  int         frame_poc;
  unsigned int frame_num;
  unsigned int recovery_frame;

  int         pic_num;
  int         long_term_pic_num;
  int         long_term_frame_idx;

  byte        is_long_term;
  int         used_for_reference;
  int         is_output;
  int         non_existing;
  int         separate_colour_plane_flag;

  short       max_slice_id;

  int         size_x, size_y, size_x_cr, size_y_cr;
  int         size_x_m1, size_y_m1, size_x_cr_m1, size_y_cr_m1;
  int         coded_frame;
  int         mb_aff_frame_flag;
  unsigned    PicWidthInMbs;
  unsigned    PicSizeInMbs;
  int         iLumaPadY, iLumaPadX;
  int         iChromaPadY, iChromaPadX;

  imgpel** imgY;
  imgpel*** imgUV;

  struct pic_motion_params** mv_info;
  struct pic_motion_params** JVmv_info[MAX_PLANE];
  struct pic_motion_params_old  motion;
  struct pic_motion_params_old  JVmotion[MAX_PLANE]; // Motion info for 4:4:4 independent mode decoding

  struct storable_picture* top_field;     // for mb aff, if frame for referencing the top field
  struct storable_picture* bottom_field;  // for mb aff, if frame for referencing the bottom field
  struct storable_picture* frame;         // for mb aff, if field for referencing the combined frame

  int         slice_type;
  int         idr_flag;
  int         no_output_of_prior_pics_flag;
  int         long_term_reference_flag;
  int         adaptive_ref_pic_buffering_flag;

  int         chroma_format_idc;
  int         frame_mbs_only_flag;
  int         frame_cropping_flag;
  int         frame_crop_left_offset;
  int         frame_crop_right_offset;
  int         frame_crop_top_offset;
  int         frame_crop_bottom_offset;
  int         qp;
  int         chroma_qp_offset[2];
  int         slice_qp_delta;
  DecRefPicMarking_t *dec_ref_pic_marking_buffer;  // stores the memory management control operations

  // picture error concealment
  int         concealed_pic;
  int         proc_flag;
  int         iLumaStride;
  int         iChromaStride;
  int         iLumaExpandedHeight;
  int         iChromaExpandedHeight;
  imgpel**    cur_imgY;               // for more efficient get_block_luma
  int no_ref;
  int iCodingType;

  char listXsize[MAX_NUM_SLICES][2];
  struct storable_picture** listX[MAX_NUM_SLICES][2];
  int         layer_id;
  } StorablePicture;
//}}}
typedef StorablePicture* StorablePicturePtr;
//{{{
//! Frame Stores for Decoded Picture Buffer
typedef struct frame_store {
  int       is_used;                //!< 0=empty; 1=top; 2=bottom; 3=both fields (or frame)
  int       is_reference;           //!< 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       is_long_term;           //!< 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       is_orig_reference;      //!< original marking by nal_ref_idc: 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used

  int       is_non_existent;

  unsigned  frame_num;
  unsigned  recovery_frame;

  int       frame_num_wrap;
  int       long_term_frame_idx;
  int       is_output;
  int       poc;

  // picture error concealment
  int concealment_reference;

  StorablePicture *frame;
  StorablePicture *top_field;
  StorablePicture *bottom_field;
  int       layer_id;
  } FrameStore;
//}}}
//{{{
//! Decoded Picture Buffer
typedef struct decoded_picture_buffer {
  VideoParameters* pVid;
  InputParameters* p_Inp;

  FrameStore** fs;
  FrameStore** fs_ref;
  FrameStore** fs_ltref;
  FrameStore** fs_ilref; // inter-layer reference (for multi-layered codecs)

  unsigned size;
  unsigned used_size;
  unsigned ref_frames_in_buffer;
  unsigned ltref_frames_in_buffer;
  int last_output_poc;

  int max_long_term_pic_idx;
  int init_done;
  int num_ref_frames;

  FrameStore* last_picture;
  unsigned used_size_il;
  int layer_id;
  } DecodedPictureBuffer;
//}}}

//{{{
// compares two stored pictures by picture number for qsort in descending order
static inline int compare_pic_by_pic_num_desc( const void *arg1, const void *arg2 )
{
  int pic_num1 = (*(StorablePicture**)arg1)->pic_num;
  int pic_num2 = (*(StorablePicture**)arg2)->pic_num;

  if (pic_num1 < pic_num2)
    return 1;
  if (pic_num1 > pic_num2)
    return -1;
  else
    return 0;
}
//}}}
//{{{
// compares two stored pictures by picture number for qsort in descending order
static inline int compare_pic_by_lt_pic_num_asc( const void *arg1, const void *arg2 )
{
  int long_term_pic_num1 = (*(StorablePicture**)arg1)->long_term_pic_num;
  int long_term_pic_num2 = (*(StorablePicture**)arg2)->long_term_pic_num;

  if ( long_term_pic_num1 < long_term_pic_num2)
    return -1;
  if ( long_term_pic_num1 > long_term_pic_num2)
    return 1;
  else
    return 0;
}
//}}}
//{{{
// compares two frame stores by pic_num for qsort in descending order
static inline int compare_fs_by_frame_num_desc( const void *arg1, const void *arg2 )
{
  int frame_num_wrap1 = (*(FrameStore**)arg1)->frame_num_wrap;
  int frame_num_wrap2 = (*(FrameStore**)arg2)->frame_num_wrap;
  if ( frame_num_wrap1 < frame_num_wrap2)
    return 1;
  if ( frame_num_wrap1 > frame_num_wrap2)
    return -1;
  else
    return 0;
}
//}}}
//{{{
// compares two frame stores by lt_pic_num for qsort in descending order
static inline int compare_fs_by_lt_pic_idx_asc( const void *arg1, const void *arg2 )
{
  int long_term_frame_idx1 = (*(FrameStore**)arg1)->long_term_frame_idx;
  int long_term_frame_idx2 = (*(FrameStore**)arg2)->long_term_frame_idx;

  if ( long_term_frame_idx1 < long_term_frame_idx2)
    return -1;
  else if ( long_term_frame_idx1 > long_term_frame_idx2)
    return 1;
  else
    return 0;
}
//}}}
//{{{
// compares two stored pictures by poc for qsort in ascending order
static inline int compare_pic_by_poc_asc( const void *arg1, const void *arg2 )
{
  int poc1 = (*(StorablePicture**)arg1)->poc;
  int poc2 = (*(StorablePicture**)arg2)->poc;

  if ( poc1 < poc2)
    return -1;
  else if ( poc1 > poc2)
    return 1;
  else
    return 0;
}
//}}}
//{{{
// compares two stored pictures by poc for qsort in descending order
static inline int compare_pic_by_poc_desc( const void *arg1, const void *arg2 )
{
  int poc1 = (*(StorablePicture**)arg1)->poc;
  int poc2 = (*(StorablePicture**)arg2)->poc;

  if (poc1 < poc2)
    return 1;
  else if (poc1 > poc2)
    return -1;
  else
    return 0;
}
//}}}
//{{{
// compares two frame stores by poc for qsort in ascending order
static inline int compare_fs_by_poc_asc( const void *arg1, const void *arg2 )
{
  int poc1 = (*(FrameStore**)arg1)->poc;
  int poc2 = (*(FrameStore**)arg2)->poc;

  if (poc1 < poc2)
    return -1;
  else if (poc1 > poc2)
    return 1;
  else
    return 0;
}
//}}}
//{{{
// compares two frame stores by poc for qsort in descending order
static inline int compare_fs_by_poc_desc( const void *arg1, const void *arg2 )
{
  int poc1 = (*(FrameStore**)arg1)->poc;
  int poc2 = (*(FrameStore**)arg2)->poc;

  if (poc1 < poc2)
    return 1;
  else if (poc1 > poc2)
    return -1;
  else
    return 0;
}
//}}}
//{{{
// returns true, if picture is short term reference picture
static inline int is_short_ref(StorablePicture *s)
{
  return ((s->used_for_reference) && (!(s->is_long_term)));
}
//}}}
//{{{
// returns true, if picture is long term reference picture
static inline int is_long_ref(StorablePicture *s)
{
  return ((s->used_for_reference) && (s->is_long_term));
}
//}}}

extern void gen_pic_list_from_frame_list (PictureStructure currStructure, FrameStore **fs_list, int list_idx, StorablePicture **list, char *list_size, int long_term);
extern StorablePicture* get_long_term_pic (Slice* currSlice, DecodedPictureBuffer *p_Dpb, int LongtermPicNum);

extern void update_ref_list (DecodedPictureBuffer *p_Dpb);
extern void update_ltref_list (DecodedPictureBuffer *p_Dpb);

extern void mm_mark_current_picture_long_term (DecodedPictureBuffer *p_Dpb, StorablePicture *p, int long_term_frame_idx);
extern void mm_unmark_short_term_for_reference (DecodedPictureBuffer *p_Dpb, StorablePicture *p, int difference_of_pic_nums_minus1);
extern void mm_unmark_long_term_for_reference (DecodedPictureBuffer *p_Dpb, StorablePicture *p, int long_term_pic_num);
extern void mm_assign_long_term_frame_idx (DecodedPictureBuffer *p_Dpb, StorablePicture* p, int difference_of_pic_nums_minus1, int long_term_frame_idx);
extern void mm_update_max_long_term_frame_idx (DecodedPictureBuffer *p_Dpb, int max_long_term_frame_idx_plus1);
extern void mm_unmark_all_short_term_for_reference (DecodedPictureBuffer *p_Dpb);
extern void mm_unmark_all_long_term_for_reference (DecodedPictureBuffer *p_Dpb);

extern int  is_used_for_reference (FrameStore* fs);
extern void get_smallest_poc (DecodedPictureBuffer *p_Dpb, int *poc,int * pos);
extern int remove_unused_frame_from_dpb (DecodedPictureBuffer *p_Dpb);
extern void init_dpb (VideoParameters* pVid, DecodedPictureBuffer *p_Dpb, int type);
extern void re_init_dpb (VideoParameters* pVid, DecodedPictureBuffer *p_Dpb, int type);
extern void free_dpb (DecodedPictureBuffer *p_Dpb);
extern FrameStore* alloc_frame_store();
extern void free_frame_store (FrameStore* f);
extern StorablePicture*  alloc_storable_picture (VideoParameters* pVid, PictureStructure type, int size_x, int size_y, int size_x_cr, int size_y_cr, int is_output);
extern void free_storable_picture (StorablePicture* p);
extern void store_picture_in_dpb (DecodedPictureBuffer *p_Dpb, StorablePicture* p);
extern StorablePicture*  get_short_term_pic (Slice* currSlice, DecodedPictureBuffer *p_Dpb, int picNum);

extern void unmark_for_reference( FrameStore* fs);
extern void unmark_for_long_term_reference (FrameStore* fs);
extern void remove_frame_from_dpb (DecodedPictureBuffer *p_Dpb, int pos);

extern void flush_dpb(DecodedPictureBuffer *p_Dpb);
extern void init_lists_p_slice (Slice* currSlice);
extern void init_lists_b_slice (Slice* currSlice);
extern void init_lists_i_slice (Slice* currSlice);
extern void update_pic_num (Slice* currSlice);

extern void dpb_split_field (VideoParameters* pVid, FrameStore *fs);
extern void dpb_combine_field (VideoParameters* pVid, FrameStore *fs);
extern void dpb_combine_field_yuv (VideoParameters* pVid, FrameStore *fs);

extern void reorder_ref_pic_list (Slice* currSlice, int cur_list);

extern void init_mbaff_lists (VideoParameters* pVid, Slice* currSlice);
extern void alloc_ref_pic_list_reordering_buffer (Slice* currSlice);
extern void free_ref_pic_list_reordering_buffer (Slice* currSlice);

extern void fill_frame_num_gap (VideoParameters* pVid, Slice *pSlice);

extern void compute_colocated (Slice* currSlice, StorablePicture **listX[6]);

extern int init_img_data (VideoParameters* pVid, ImageData *p_ImgData, seq_parameter_set_rbsp_t *sps);
extern void free_img_data (VideoParameters* pVid, ImageData *p_ImgData);

extern void pad_dec_picture (VideoParameters* pVid, StorablePicture *dec_picture);
extern void pad_buf (imgpel *pImgBuf, int iWidth, int iHeight, int iStride, int iPadX, int iPadY);

extern void process_picture_in_dpb_s (VideoParameters* pVid, StorablePicture *p_pic);
extern StorablePicture * clone_storable_picture (VideoParameters* pVid, StorablePicture *p_pic );
extern void store_proc_picture_in_dpb (DecodedPictureBuffer *p_Dpb, StorablePicture* p);
