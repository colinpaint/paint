#pragma once
#include "global.h"
#include "buffer.h"

extern int intra_pred_4x4_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff, int img_block_x, int img_block_y);
extern int intra_pred_4x4_normal (sMacroBlock* mb, eColorPlane plane, int ioff, int joff, int img_block_x, int img_block_y);

extern void set_intra_prediction_modes (cSlice* slice);
