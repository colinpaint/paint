#pragma once
#include "global.h"
#include "mbuffer.h"

extern int intra_pred_8x8_normal(Macroblock *currMB, ColorPlane pl, int ioff, int joff);
extern int intra_pred_8x8_mbaff(Macroblock *currMB, ColorPlane pl, int ioff, int joff);
