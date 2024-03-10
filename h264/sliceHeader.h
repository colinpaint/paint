#pragma once

extern void initContexts (sSlice* currslice);
extern void dec_ref_pic_marking (sDecoder* decoder, sBitstream* curStream, sSlice* pSlice);

extern int dumpPOC (sDecoder* decoder);
extern void decodePOC (sDecoder* decoder, sSlice* pSlice);

extern void readSliceHeader (sSlice* slice);
extern void readRestSliceHeader (sSlice* slice);
