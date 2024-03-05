#pragma once
#include "global.h"
#include "mbuffer.h"

extern int intra_pred_4x4_mbaff (sMacroblock* currMB, ColorPlane pl, int ioff, int joff, int img_block_x, int img_block_y);
extern int intra_pred_4x4_normal (sMacroblock* currMB, ColorPlane pl, int ioff, int joff, int img_block_x, int img_block_y);
