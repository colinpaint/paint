#pragma once
#include "global.h"
#include "mbuffer.h"

extern int  allocate_pred_mem(Slice* currSlice);
extern void free_pred_mem    (Slice* currSlice);

extern void get_block_luma(StorablePicture* curr_ref, int x_pos, int y_pos, int block_size_x, int block_size_y, imgpel **block,
                           int shift_x,int maxold_x,int maxold_y,int **tmp_res,int max_imgpel_value,imgpel no_ref_value,Macroblock* currMB);

extern void intra_cr_decoding    (Macroblock* currMB, int yuv);
extern void prepare_direct_params(Macroblock* currMB, StorablePicture *dec_picture, MotionVector *pmvl0, MotionVector *pmvl1,char *l0_rFrame, char *l1_rFrame);
extern void perform_mc           (Macroblock* currMB, ColorPlane pl, StorablePicture *dec_picture, int pred_dir, int i, int j, int block_size_x, int block_size_y);
