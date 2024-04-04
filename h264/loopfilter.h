#pragma once
#include "global.h"
#include "buffer.h"

extern void deblockPicture (cDecoder264* decoder, sPicture* p) ;
extern void initDeblock (cDecoder264* decoder, int mbAffFrame);
