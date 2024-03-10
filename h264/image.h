#pragma once
#include "buffer.h"


extern void calcFrameNum (sDecoder* decoder, sPicture* p);
extern void initOldSlice (sOldSlice* oldSliceParam);

extern void padPicture (sDecoder* decoder, sPicture* picture);
extern void exitPicture (sDecoder* decoder, sPicture** picture);

extern int decodeFrame (sDecoder* decoder);
