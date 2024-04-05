#pragma once
#include "global.h"
#include "buffer.h"

void deblockPicture (cDecoder264* decoder, sPicture* p) ;
void initDeblock (cDecoder264* decoder, int mbAffFrame);
