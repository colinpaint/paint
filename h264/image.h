#pragma once
#include "buffer.h"


extern void calcFrameNum (sDecoder* decoder, sPicture* picture);
extern void initOldSlice (sOldSlice* oldSlice);

extern void padPicture (sDecoder* decoder, sPicture* picture);
extern void exitPicture (sDecoder* decoder, sPicture** picture);

extern int decodeFrame (sDecoder* decoder);
