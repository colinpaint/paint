#pragma once
#include "global.h"
#include "buffer.h"

extern void deblockPicture (sDecoder* vidParam, sPicture* p) ;
extern void initDeblock (sDecoder* vidParam, int mbAffFrameFlag);
