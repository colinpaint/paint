#pragma once

extern void initContexts (sSlice* currslice);
extern void dec_ref_pic_marking (sVidParam* vidParam, sBitstream* curStream, sSlice* pSlice);

extern int dumpPOC (sVidParam* vidParam);
extern void decodePOC (sVidParam* vidParam, sSlice* pSlice);

extern void readSliceHeader (sSlice* curSlice);
extern void readRestSliceHeader (sSlice* curSlice);
