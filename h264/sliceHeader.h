#pragma once

extern void init_contexts  (sSlice* currslice);

extern void firstPartOfSliceHeader (sSlice* currSlice);
extern void restOfSliceHeader (sSlice* currSlice);

extern void dec_ref_pic_marking (sVidParam* vidParam, Bitstream* currStream, sSlice *pSlice);
extern void decodePOC (sVidParam* vidParam, sSlice *pSlice);
extern int dumpPOC (sVidParam* vidParam);
