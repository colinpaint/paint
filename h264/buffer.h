#pragma once
#include "global.h"

#define MAX_LIST_SIZE 33
//{{{  sPicMotionParamsOld
typedef struct PicMotionParamOld {
  byte*  mb_field;      // field macroblock indicator
  } sPicMotionParamsOld;
//}}}
//{{{  sPicMotionParam
typedef struct PicMotionParam {
  struct Picture* refPic[2];  // referrence picture pointer
  sMotionVector           mv[2];       // motion vector
  char                    refIndex[2];  // reference picture   [list][subblock_y][subblock_x]
  byte                    slice_no;
  } sPicMotionParam;
//}}}
//{{{  sPicture
typedef struct Picture {
  ePicStructure structure;

  int         poc;
  int         topPoc;
  int         botPoc;
  int         framePoc;
  unsigned int frameNum;
  unsigned int recoveryFrame;

  int         picNum;
  int         long_term_pic_num;
  int         longTermFrameIndex;

  byte        isLongTerm;
  int         usedForReference;
  int         is_output;
  int         non_existing;
  int         sepColourPlaneFlag;

  short       max_slice_id;

  int         sizeX, sizeY, size_x_cr, size_y_cr;
  int         size_x_m1, size_y_m1, size_x_cr_m1, size_y_cr_m1;
  int         codedFrame;
  int         mbAffFrameFlag;
  unsigned    PicWidthInMbs;
  unsigned    picSizeInMbs;
  int         iLumaPadY, iLumaPadX;
  int         iChromaPadY, iChromaPadX;

  sPixel**    imgY;
  sPixel***   imgUV;

  struct PicMotionParam** mvInfo;
  struct PicMotionParam** JVmv_info[MAX_PLANE];
  struct PicMotionParamOld  motion;
  struct PicMotionParamOld  JVmotion[MAX_PLANE]; // Motion info for 4:4:4 independent mode decoding

  struct Picture* top_field;     // for mb aff, if frame for referencing the top field
  struct Picture* bottom_field;  // for mb aff, if frame for referencing the bottom field
  struct Picture* frame;         // for mb aff, if field for referencing the combined frame

  int         sliceType;
  int         idrFlag;
  int         noOutputPriorPicFlag;
  int         longTermRefFlag;
  int         adaptiveRefPicBufferingFlag;

  int         chromaFormatIdc;
  int         frameMbOnlyFlag;
  int         frameCropFlag;
  int         frameCropLeft;
  int         frameCropRight;
  int         frameCropTop;
  int         frameCropBot;
  int         qp;
  int         chromaQpOffset[2];
  int         slice_qp_delta;
  sDecRefPicMarking* decRefPicMarkingBuffer;  // stores the memory management control operations

  // picture error conceal
  int         concealed_pic;
  int         proc_flag;
  int         iLumaStride;
  int         iChromaStride;
  int         iLumaExpandedHeight;
  int         iChromaExpandedHeight;
  sPixel**    curPixelY;               // for more efficient get_block_luma
  int         noRef;
  int         iCodingType;

  char listXsize[MAX_NUM_SLICES][2];
  struct Picture** listX[MAX_NUM_SLICES][2];
  int         layerId;
  } sPicture;
//}}}
typedef sPicture* sPicturePtr;
//{{{  sFrameStore
typedef struct FrameStore {
  int       isUsed;                // 0=empty; 1=top; 2=bottom; 3=both fields (or frame)
  int       isReference;           // 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       isLongTerm;           // 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       isOrigReference;      // original marking by nalRefIdc: 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used

  int       is_non_existent;

  unsigned  frameNum;
  unsigned  recoveryFrame;

  int       frame_num_wrap;
  int       longTermFrameIndex;
  int       is_output;
  int       poc;

  // picture error conceal
  int concealment_reference;

  sPicture* frame;
  sPicture* top_field;
  sPicture* bottom_field;
  } sFrameStore;
//}}}
//{{{  sDPB
typedef struct DPB {
  sVidParam*    vidParam;
  sInputParam*  inputParam;

  sFrameStore** fs;
  sFrameStore** fs_ref;
  sFrameStore** fs_ltref;

  unsigned size;
  unsigned usedSize;
  unsigned refFramesInBuffer;
  unsigned longTermRefFramesInBuffer;
 
  int lastOutputPoc;
  int maxLongTermPicIndex;
  int initDone;
  int numRefFrames;

  sFrameStore* lastPicture;
  int layerId;
  } sDPB;
//}}}

//{{{
// compares two stored pictures by picture number for qsort in descending order
static inline int compare_pic_by_pic_num_desc (const void* arg1, const void* arg2 )
{
  int pic_num1 = (*(sPicture**)arg1)->picNum;
  int pic_num2 = (*(sPicture**)arg2)->picNum;

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
static inline int compare_pic_by_lt_pic_num_asc (const void* arg1, const void* arg2 )
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
// compares two frame stores by picNum for qsort in descending order
static inline int compare_fs_by_frame_num_desc (const void* arg1, const void* arg2 )
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
static inline int compare_fs_by_lt_pic_idx_asc (const void* arg1, const void* arg2 )
{
  int long_term_frame_idx1 = (*(sFrameStore**)arg1)->longTermFrameIndex;
  int long_term_frame_idx2 = (*(sFrameStore**)arg2)->longTermFrameIndex;

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
static inline int compare_pic_by_poc_asc (const void* arg1, const void* arg2 )
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
static inline int compare_pic_by_poc_desc (const void* arg1, const void* arg2 )
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
static inline int compare_fs_by_poc_asc (const void* arg1, const void* arg2 )
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
static inline int compare_fs_by_poc_desc (const void* arg1, const void* arg2 )
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
static inline int is_short_ref (sPicture* s)
{
  return ((s->usedForReference) && (!(s->isLongTerm)));
}
//}}}
//{{{
// returns true, if picture is long term reference picture
static inline int is_long_ref (sPicture* s)
{
  return ((s->usedForReference) && (s->isLongTerm));
}
//}}}

extern sFrameStore* allocFrameStore();
extern void freeFrameStore (sFrameStore* frameStore);
extern void unmark_for_reference( sFrameStore* frameStore);
extern void unmark_for_long_term_reference (sFrameStore* frameStore);

extern sPicture* allocPicture (sVidParam* vidParam, ePicStructure type, int sizeX, int sizeY, int size_x_cr, int size_y_cr, int is_output);
extern void freePicture (sPicture* p);
extern void fillFrameNumGap (sVidParam* vidParam, sSlice *pSlice);

extern void updateRefList (sDPB* dpb);
extern void updateLongTermRefList (sDPB* dpb);
extern void mm_mark_current_picture_long_term (sDPB* dpb, sPicture* p, int longTermFrameIndex);
extern void mm_unmark_short_term_for_reference (sDPB* dpb, sPicture* p, int difference_of_pic_nums_minus1);
extern void mm_unmark_long_term_for_reference (sDPB* dpb, sPicture *p, int long_term_pic_num);
extern void mm_assign_long_term_frame_idx (sDPB* dpb, sPicture* p, int difference_of_pic_nums_minus1, int longTermFrameIndex);
extern void mm_update_max_long_term_frame_idx (sDPB* dpb, int max_long_term_frame_idx_plus1);
extern void mm_unmark_all_short_term_for_reference (sDPB* dpb);
extern void mm_unmark_all_long_term_for_reference (sDPB* dpb);
extern void get_smallest_poc (sDPB* dpb, int *poc,int * pos);

extern void initDpb (sVidParam* vidParam, sDPB* dpb, int type);
extern void reInitDpb (sVidParam* vidParam, sDPB* dpb, int type);
extern void flushDpb(sDPB* dpb);

extern int removeUnusedDpb (sDPB* dpb);
extern void storePictureDpb (sDPB* dpb, sPicture* p);
extern void removeFrameDpb (sDPB* dpb, int pos);
extern void freeDpb (sDPB* dpb);

extern void initImage (sVidParam* vidParam, sImage* image, sSPS *sps);
extern void freeImage (sVidParam* vidParam, sImage* image);

extern void init_lists_p_slice (sSlice* slice);
extern void init_lists_b_slice (sSlice* slice);
extern void init_lists_i_slice (sSlice* slice);
extern void update_pic_num (sSlice* slice);

extern void dpb_combine_field_yuv (sVidParam* vidParam, sFrameStore* frameStore);
extern void reorderRefPicList (sSlice* slice, int curList);
extern void init_mbaff_lists (sVidParam* vidParam, sSlice* slice);
extern sPicture* get_short_term_pic (sSlice* slice, sDPB* dpb, int picNum);

extern void alloc_ref_pic_list_reordering_buffer (sSlice* slice);
extern void freeRefPicListReorderingBuffer (sSlice* slice);
extern void compute_colocated (sSlice* slice, sPicture** listX[6]);
