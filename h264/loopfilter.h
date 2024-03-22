#pragma once
#include "global.h"
#include "buffer.h"

extern void deblockPicture (sDecoder* decoder, sPicture* p) ;
extern void initDeblock (sDecoder* decoder, int mbAffFrame);
