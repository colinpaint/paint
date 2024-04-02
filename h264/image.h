#pragma once
#include "global.h"
#include "buffer.h"

extern void decodePOC (sDecoder* decoder, sSlice* slice);
extern void initOldSlice (sOldSlice* oldSlice);
extern void padPicture (sDecoder* decoder, sPicture* picture);

extern int decodeFrame (sDecoder* decoder);
