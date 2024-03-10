#pragma once
#include "global.h"
#include "buffer.h"

extern int allocPred (sSlice* curSlice);
extern void freePred (sSlice* curSlice);

extern void get_block_luma (sPicture* curRef, int x_pos, int y_pos,
                            int block_size_x, int block_size_y, sPixel** block,
                            int shift_x, int maxold_x, int maxold_y,
                            int** tmp_res, int max_imgpel_value,
                            sPixel no_ref_value, sMacroblock* curMb);

extern void intra_cr_decoding (sMacroblock* curMb, int yuv);
extern void prepare_direct_params (sMacroblock* curMb, sPicture* picture,
                                  sMotionVector* pmvl0, sMotionVector* pmvl1,
                                  char* l0_rFrame, char* l1_rFrame);
extern void perform_mc (sMacroblock* curMb, eColorPlane pl, sPicture* picture,
                         int pred_dir, int i, int j, int block_size_x, int block_size_y);

extern void update_direct_types (sSlice* curSlice);