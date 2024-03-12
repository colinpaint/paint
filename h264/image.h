#pragma once
#include "buffer.h"


extern void initOldSlice (sOldSlice* oldSlice);

extern void padPicture (sDecoder* decoder, sPicture* picture);
extern void endDecodeFrame (sDecoder* decoder);

extern int decodeFrame (sDecoder* decoder);
