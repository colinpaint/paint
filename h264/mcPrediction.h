#pragma once
#include "global.h"
#include "mbuffer.h"

extern int  allocate_pred_mem(sSlice* curSlice);
extern void free_pred_mem    (sSlice* curSlice);

extern void get_block_luma(sPicture* curr_ref, int x_pos, int y_pos, int block_size_x, int block_size_y, sPixel** block,
                           int shift_x,int maxold_x,int maxold_y,int** tmp_res,int max_imgpel_value,sPixel no_ref_value,sMacroblock* curMb);

extern void intra_cr_decoding    (sMacroblock* curMb, int yuv);
extern void prepare_direct_params(sMacroblock* curMb, sPicture* picture, sMotionVector *pmvl0, sMotionVector *pmvl1,char *l0_rFrame, char *l1_rFrame);
extern void perform_mc           (sMacroblock* curMb, sColorPlane pl, sPicture* picture, int pred_dir, int i, int j, int block_size_x, int block_size_y);
