#pragma once

extern void init_contexts  (Slice* currslice);

extern int FirstPartOfSliceHeader (Slice* currSlice);
extern int RestOfSliceHeader (Slice* currSlice);

extern void dec_ref_pic_marking (sVidParam* vidParam, Bitstream* currStream, Slice *pSlice);
extern void decodePOC (sVidParam* vidParam, Slice *pSlice);
extern int dumpPOC (sVidParam* vidParam);
