#pragma once
#include "global.h"
#include "mbuffer.h"

extern int intra_pred_16x16_mbaff (Macroblock *currMB, ColorPlane pl, int predmode);
extern int intra_pred_16x16_normal(Macroblock *currMB, ColorPlane pl, int predmode);
