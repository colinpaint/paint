#pragma once
#include "mbuffer.h"


extern void calcFrameNum (sVidParam* vidParam, sPicture* p);
extern void init_old_slice (sOldSliceParam* p_old_slice);

extern void pad_dec_picture (sVidParam* vidParam, sPicture* picture);
extern void exitPicture (sVidParam* vidParam, sPicture** picture);

extern int decode_one_frame (sDecoderParam* pDecoder);
