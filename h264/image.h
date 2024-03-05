#pragma once
#include "mbuffer.h"

extern void calculate_frame_no (sVidParam* vidParam, sPicture* p);
extern void init_old_slice (OldSliceParams* p_old_slice);
extern void decode_one_slice (sSlice* currSlice);
extern int read_new_slice (sSlice* currSlice);
extern void exit_picture (sVidParam* vidParam, sPicture** picture);

extern int decode_one_frame (sDecoderParams* pDecoder);
