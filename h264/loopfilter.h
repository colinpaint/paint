#pragma once
#include "global.h"
#include "mbuffer.h"

extern void deblockPicture (sVidParam* vidParam, sPicture* p) ;
extern void initDeblock (sVidParam* vidParam, int mb_aff_frame_flag);
