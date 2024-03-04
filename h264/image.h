#pragma once
#include "mbuffer.h"

extern void calculate_frame_no (VideoParameters* p_Vid, StorablePicture* p);
extern void init_old_slice (OldSliceParams* p_old_slice);
extern void decode_one_slice (Slice* currSlice);
extern int read_new_slice (Slice* currSlice);
extern void exit_picture (VideoParameters* p_Vid, StorablePicture** dec_picture);

extern int decode_one_frame (DecoderParams* pDecoder);
