#pragma once
#include "global.h"
#include "mbuffer.h"

extern void DeblockPicture (VideoParameters* pVid, StorablePicture* p) ;
extern void init_Deblock (VideoParameters* pVid, int mb_aff_frame_flag);
