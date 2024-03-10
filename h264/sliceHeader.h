#pragma once

extern void initContexts (sSlice* currslice);
extern void dec_ref_pic_marking (sDecoder* decoder, sBitstream* s, sSlice* splice);

extern int dumpPOC (sDecoder* decoder);
extern void decodePOC (sDecoder* decoder, sSlice* splice);

extern void readSliceHeader (sSlice* slice);
extern void readRestSliceHeader (sSlice* slice);
