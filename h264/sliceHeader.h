#pragma once

extern void init_contexts  (sSlice* currslice);

extern void firstPartOfSliceHeader (sSlice* curSlice);
extern void restOfSliceHeader (sSlice* curSlice);

extern void dec_ref_pic_marking (sVidParam* vidParam, sBitstream* curStream, sSlice *pSlice);
extern void decodePOC (sVidParam* vidParam, sSlice *pSlice);
extern int dumpPOC (sVidParam* vidParam);
