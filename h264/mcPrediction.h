#pragma once
#include "global.h"
#include "mbuffer.h"

extern int  allocate_pred_mem(Slice* currSlice);
extern void free_pred_mem    (Slice* currSlice);

extern void get_block_luma(sPicture* curr_ref, int x_pos, int y_pos, int block_size_x, int block_size_y, sPixel **block,
                           int shift_x,int maxold_x,int maxold_y,int **tmp_res,int max_imgpel_value,sPixel no_ref_value,sMacroblock* currMB);

extern void intra_cr_decoding    (sMacroblock* currMB, int yuv);
extern void prepare_direct_params(sMacroblock* currMB, sPicture *dec_picture, MotionVector *pmvl0, MotionVector *pmvl1,char *l0_rFrame, char *l1_rFrame);
extern void perform_mc           (sMacroblock* currMB, ColorPlane pl, sPicture *dec_picture, int pred_dir, int i, int j, int block_size_x, int block_size_y);
