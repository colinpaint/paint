#pragma once

extern void init_contexts  (Slice* currslice);

extern int FirstPartOfSliceHeader (Slice* currSlice);
extern int RestOfSliceHeader (Slice* currSlice);

extern void dec_ref_pic_marking (VideoParameters* pVid, Bitstream* currStream, Slice *pSlice);
extern void decodePOC (VideoParameters* pVid, Slice *pSlice);
extern int dumpPOC (VideoParameters* pVid);
