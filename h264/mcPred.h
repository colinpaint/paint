#pragma once
#include "global.h"
#include "buffer.h"

void get_block_luma (sPicture* curRef, int x_pos, int y_pos,
                     int blockSizeX, int blockSizeY, sPixel** block,
                     int shift_x, int maxold_x, int maxold_y,
                     int** tempRes, int max_imgpel_value,
                     sPixel no_ref_value, sMacroBlock* mb);
void update_direct_types (cSlice* slice);

int get_colocated_info_8x8 (sMacroBlock* mb, sPicture* list1, int i, int j);
int get_colocated_info_4x4 (sMacroBlock* mb, sPicture* list1, int i, int j);

void intra_cr_decoding (sMacroBlock* mb, int yuv);
void prepare_direct_params (sMacroBlock* mb, sPicture* picture,
                            sMotionVec* pmvl0, sMotionVec* pmvl1,
                            char* l0_rFrame, char* l1_rFrame);
void perform_mc (sMacroBlock* mb, eColorPlane plane, sPicture* picture,
                 int predDir, int i, int j, int blockSizeX, int blockSizeY);
