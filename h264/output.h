#pragma once

extern void flush_pending_output (VideoParameters* pVid);
extern void init_out_buffer (VideoParameters* pVid);
extern void uninit_out_buffer (VideoParameters* pVid);

extern void write_stored_frame (VideoParameters* pVid, FrameStore* fs);
extern void direct_output (VideoParameters* pVid, StorablePicture* p);
extern void init_output (CodingParameters* p_CodingParams, int symbol_size_in_bytes);
