#pragma once

extern void init_contexts  (mSlice* currslice);

extern int FirstPartOfSliceHeader (mSlice* currSlice);
extern int RestOfSliceHeader (mSlice* currSlice);

extern void dec_ref_pic_marking (sVidParam* vidParam, Bitstream* currStream, mSlice *pSlice);
extern void decodePOC (sVidParam* vidParam, mSlice *pSlice);
extern int dumpPOC (sVidParam* vidParam);
