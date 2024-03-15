#pragma once
#include "buffer.h"

extern void dec_ref_pic_marking (sDecoder* decoder, sBitStream* s, sSlice* slice);
extern void decodePOC (sDecoder* decoder, sSlice* slice);
extern void initOldSlice (sOldSlice* oldSlice);

extern void padPicture (sDecoder* decoder, sPicture* picture);
extern void endDecodeFrame (sDecoder* decoder);

extern int decodeFrame (sDecoder* decoder);
