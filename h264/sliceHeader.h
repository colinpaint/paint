#pragma once

extern void init_contexts  (sSlice* currslice);

extern int FirstPartOfSliceHeader (sSlice* currSlice);
extern int RestOfSliceHeader (sSlice* currSlice);

extern void dec_ref_pic_marking (sVidParam* vidParam, Bitstream* currStream, sSlice *pSlice);
extern void decodePOC (sVidParam* vidParam, sSlice *pSlice);
extern int dumpPOC (sVidParam* vidParam);
