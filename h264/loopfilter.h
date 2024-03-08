#pragma once
#include "global.h"
#include "buffer.h"

extern void deblockPicture (sVidParam* vidParam, sPicture* p) ;
extern void initDeblock (sVidParam* vidParam, int mbAffFrameFlag);
