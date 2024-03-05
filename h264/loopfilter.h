#pragma once
#include "global.h"
#include "mbuffer.h"

extern void DeblockPicture (VideoParameters* p_Vid, StorablePicture* p) ;
extern void init_Deblock (VideoParameters* p_Vid, int mb_aff_frame_flag);
