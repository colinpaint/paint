#pragma once
#include "global.h"
#include "mbuffer.h"

extern void DeblockPicture (sVidParam* vidParam, sStorablePicture* p) ;
extern void init_Deblock (sVidParam* vidParam, int mb_aff_frame_flag);
