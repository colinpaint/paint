#pragma once
#include "global.h"

#if (MVC_EXTENSION_ENABLE)
  extern void reorder_lists_mvc     (Slice * currSlice, int currPOC);
  extern void init_lists_p_slice_mvc(Slice *currSlice);
  extern void init_lists_b_slice_mvc(Slice *currSlice);
  extern void init_lists_i_slice_mvc(Slice *currSlice);

  extern void reorder_ref_pic_list_mvc(Slice *currSlice, int cur_list, int **anchor_ref, int **non_anchor_ref,
                                                   int view_id, int anchor_pic_flag, int currPOC, int listidx);

  extern void reorder_short_term(Slice *currSlice, int cur_list, int num_ref_idx_lX_active_minus1, int picNumLX, int *refIdxLX, int currViewID);
  extern void reorder_long_term(Slice *currSlice, StorablePicture **RefPicListX, int num_ref_idx_lX_active_minus1, int LongTermPicNum, int *refIdxLX, int currViewID);
#endif
