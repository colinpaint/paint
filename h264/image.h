#pragma once
#include "buffer.h"


extern void calcFrameNum (sVidParam* vidParam, sPicture* p);
extern void initOldSlice (sOldSliceParam* oldSliceParam);

extern void padPicture (sVidParam* vidParam, sPicture* picture);
extern void exitPicture (sVidParam* vidParam, sPicture** picture);

extern int decodeFrame (sDecoderParam* pDecoder);
