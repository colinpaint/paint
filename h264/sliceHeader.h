#pragma once

extern void init_contexts  (Slice *currslice);

extern int FirstPartOfSliceHeader (Slice *currSlice);
extern int RestOfSliceHeader (Slice *currSlice);

extern void dec_ref_pic_marking (VideoParameters *p_Vid, Bitstream *currStream, Slice *pSlice);
extern void decodePOC (VideoParameters *p_Vid, Slice *pSlice);
extern int dumpPOC (VideoParameters *p_Vid);
