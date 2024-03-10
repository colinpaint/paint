#pragma once

extern void initContexts (sSlice* currslice);
extern void dec_ref_pic_marking (sDecoder* vidParam, sBitstream* curStream, sSlice* pSlice);

extern int dumpPOC (sDecoder* vidParam);
extern void decodePOC (sDecoder* vidParam, sSlice* pSlice);

extern void readSliceHeader (sSlice* curSlice);
extern void readRestSliceHeader (sSlice* curSlice);
