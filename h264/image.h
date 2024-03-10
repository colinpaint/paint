#pragma once
#include "buffer.h"


extern void calcFrameNum (sDecoder* vidParam, sPicture* p);
extern void initOldSlice (sOldSlice* oldSliceParam);

extern void padPicture (sDecoder* vidParam, sPicture* picture);
extern void exitPicture (sDecoder* vidParam, sPicture** picture);

extern int decodeFrame (sDecoder* decoder);
