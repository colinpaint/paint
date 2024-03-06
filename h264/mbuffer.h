#pragma once
#include "global.h"

#define MAX_LIST_SIZE 33
//{{{
//! definition of pic motion parameters
typedef struct pic_motion_params_old {
  byte *      mb_field;      //!< field macroblock indicator
  } sPicMotionParamsOld;
//}}}
//{{{
//! definition of pic motion parameters
typedef struct pic_motion_params {
  struct storablePicture *ref_pic[2];  //!< referrence picture pointer
  sMotionVector             mv[2];       //!< motion vector
  char                     ref_idx[2];  //!< reference picture   [list][subblock_y][subblock_x]
  //byte                   mb_field;    //!< field macroblock indicator
  byte                     slice_no;
  } sPicMotionParams;
//}}}
//{{{
//! definition a picture (field or frame)
typedef struct storablePicture {
  sPictureStructure structure;

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

  sPixel** imgY;
  sPixel*** imgUV;

  struct pic_motion_params** mv_info;
  struct pic_motion_params** JVmv_info[MAX_PLANE];
  struct pic_motion_params_old  motion;
  struct pic_motion_params_old  JVmotion[MAX_PLANE]; // Motion info for 4:4:4 independent mode decoding

  struct storablePicture* top_field;     // for mb aff, if frame for referencing the top field
  struct storablePicture* bottom_field;  // for mb aff, if frame for referencing the bottom field
  struct storablePicture* frame;         // for mb aff, if field for referencing the combined frame

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
  sDecRefPicMarking *dec_ref_pic_marking_buffer;  // stores the memory management control operations

  // picture error concealment
  int         concealed_pic;
  int         proc_flag;
  int         iLumaStride;
  int         iChromaStride;
  int         iLumaExpandedHeight;
  int         iChromaExpandedHeight;
  sPixel**    cur_imgY;               // for more efficient get_block_luma
  int no_ref;
  int iCodingType;

  char listXsize[MAX_NUM_SLICES][2];
  struct storablePicture** listX[MAX_NUM_SLICES][2];
  int         layer_id;
  } sPicture;
//}}}
typedef sPicture* StorablePicturePtr;
//{{{
//! Frame Stores for Decoded Picture Buffer
typedef struct frameStore {
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

  sPicture* frame;
  sPicture* top_field;
  sPicture* bottom_field;
  } sFrameStore;
//}}}
//{{{
//! Decoded Picture Buffer
typedef struct decoded_picture_buffer {
  sVidParam* vidParam;
  sInputParam* p_Inp;

  sFrameStore** fs;
  sFrameStore** fs_ref;
  sFrameStore** fs_ltref;
  sFrameStore** fs_ilref; // inter-layer reference (for multi-layered codecs)

  unsigned size;
  unsigned used_size;
  unsigned ref_frames_in_buffer;
  unsigned ltref_frames_in_buffer;
  int last_output_poc;

  int max_long_term_pic_idx;
  int init_done;
  int num_ref_frames;

  sFrameStore* last_picture;
  unsigned used_size_il;
  int layer_id;
  } sDPB;
//}}}

//{{{
// compares two stored pictures by picture number for qsort in descending order
static inline int compare_pic_by_pic_num_desc( const void *arg1, const void *arg2 )
{
  int pic_num1 = (*(sPicture**)arg1)->pic_num;
  int pic_num2 = (*(sPicture**)arg2)->pic_num;

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
  int long_term_pic_num1 = (*(sPicture**)arg1)->long_term_pic_num;
  int long_term_pic_num2 = (*(sPicture**)arg2)->long_term_pic_num;

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
  int frame_num_wrap1 = (*(sFrameStore**)arg1)->frame_num_wrap;
  int frame_num_wrap2 = (*(sFrameStore**)arg2)->frame_num_wrap;
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
  int long_term_frame_idx1 = (*(sFrameStore**)arg1)->long_term_frame_idx;
  int long_term_frame_idx2 = (*(sFrameStore**)arg2)->long_term_frame_idx;

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
  int poc1 = (*(sPicture**)arg1)->poc;
  int poc2 = (*(sPicture**)arg2)->poc;

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
  int poc1 = (*(sPicture**)arg1)->poc;
  int poc2 = (*(sPicture**)arg2)->poc;

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
  int poc1 = (*(sFrameStore**)arg1)->poc;
  int poc2 = (*(sFrameStore**)arg2)->poc;

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
  int poc1 = (*(sFrameStore**)arg1)->poc;
  int poc2 = (*(sFrameStore**)arg2)->poc;

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
static inline int is_short_ref(sPicture *s)
{
  return ((s->used_for_reference) && (!(s->is_long_term)));
}
//}}}
//{{{
// returns true, if picture is long term reference picture
static inline int is_long_ref(sPicture *s)
{
  return ((s->used_for_reference) && (s->is_long_term));
}
//}}}

extern void gen_pic_list_from_frame_list (sPictureStructure currStructure, sFrameStore** fs_list, int list_idx, sPicture** list, char *list_size, int long_term);
extern sPicture* get_long_term_pic (sSlice* currSlice, sDPB* dpb, int LongtermPicNum);

extern void update_ref_list (sDPB* dpb);
extern void update_ltref_list (sDPB* dpb);

extern void mm_mark_current_picture_long_term (sDPB* dpb, sPicture *p, int long_term_frame_idx);
extern void mm_unmark_short_term_for_reference (sDPB* dpb, sPicture *p, int difference_of_pic_nums_minus1);
extern void mm_unmark_long_term_for_reference (sDPB* dpb, sPicture *p, int long_term_pic_num);
extern void mm_assign_long_term_frame_idx (sDPB* dpb, sPicture* p, int difference_of_pic_nums_minus1, int long_term_frame_idx);
extern void mm_update_max_long_term_frame_idx (sDPB* dpb, int max_long_term_frame_idx_plus1);
extern void mm_unmark_all_short_term_for_reference (sDPB* dpb);
extern void mm_unmark_all_long_term_for_reference (sDPB* dpb);

extern int  is_used_for_reference (sFrameStore* fs);
extern void get_smallest_poc (sDPB* dpb, int *poc,int * pos);
extern int remove_unused_frame_from_dpb (sDPB* dpb);

extern void init_dpb (sVidParam* vidParam, sDPB* dpb, int type);
extern void re_init_dpb (sVidParam* vidParam, sDPB* dpb, int type);
extern void free_dpb (sDPB* dpb);

extern sPicture*  allocPicture (sVidParam* vidParam, sPictureStructure type, int size_x, int size_y, int size_x_cr, int size_y_cr, int is_output);
extern void freePicture (sPicture* p);

extern void store_picture_in_dpb (sDPB* dpb, sPicture* p);
extern sPicture*  get_short_term_pic (sSlice* currSlice, sDPB* dpb, int picNum);

extern sFrameStore* alloc_frame_store();
extern void free_frame_store (sFrameStore* f);
extern void unmark_for_reference( sFrameStore* fs);
extern void unmark_for_long_term_reference (sFrameStore* fs);

extern void remove_frame_from_dpb (sDPB* dpb, int pos);

extern void flush_dpb(sDPB* dpb);
extern void init_lists_p_slice (sSlice* currSlice);
extern void init_lists_b_slice (sSlice* currSlice);
extern void init_lists_i_slice (sSlice* currSlice);
extern void update_pic_num (sSlice* currSlice);

extern void dpb_split_field (sVidParam* vidParam, sFrameStore *fs);
extern void dpb_combine_field (sVidParam* vidParam, sFrameStore *fs);
extern void dpb_combine_field_yuv (sVidParam* vidParam, sFrameStore *fs);

extern void reorder_ref_pic_list (sSlice* currSlice, int cur_list);

extern void init_mbaff_lists (sVidParam* vidParam, sSlice* currSlice);
extern void alloc_ref_pic_list_reordering_buffer (sSlice* currSlice);
extern void free_ref_pic_list_reordering_buffer (sSlice* currSlice);

extern void fill_frame_num_gap (sVidParam* vidParam, sSlice *pSlice);

extern void compute_colocated (sSlice* currSlice, sPicture** listX[6]);

extern sPicture* clone_storable_picture (sVidParam* vidParam, sPicture *p_pic );

extern void store_proc_picture_in_dpb (sDPB* dpb, sPicture* p);
extern int init_img_data (sVidParam* vidParam, sImage *p_ImgData, sSPS *sps);
extern void free_img_data (sVidParam* vidParam, sImage *p_ImgData);

extern void process_picture_in_dpb_s (sVidParam* vidParam, sPicture *p_pic);
