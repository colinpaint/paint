#pragma once
#include "global.h"
#include "buffer.h"

extern void decodePOC (cDecoder264* decoder, sSlice* slice);
extern void initOldSlice (sOldSlice* oldSlice);
extern void padPicture (cDecoder264* decoder, sPicture* picture);

extern int decodeFrame (cDecoder264* decoder);
