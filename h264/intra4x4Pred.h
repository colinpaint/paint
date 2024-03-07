#pragma once
#include "global.h"
#include "buffer.h"

extern int intra_pred_4x4_mbaff (sMacroblock* curMb, eColorPlane pl, int ioff, int joff, int img_block_x, int img_block_y);
extern int intra_pred_4x4_normal (sMacroblock* curMb, eColorPlane pl, int ioff, int joff, int img_block_x, int img_block_y);
