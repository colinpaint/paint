#pragma once

extern void flush_pending_output (VideoParameters* p_Vid);
extern void init_out_buffer (VideoParameters* p_Vid);
extern void uninit_out_buffer (VideoParameters* p_Vid);

extern void write_stored_frame (VideoParameters* p_Vid, FrameStore* fs);
extern void direct_output (VideoParameters* p_Vid, StorablePicture* p);
extern void init_output (CodingParameters* p_CodingParams, int symbol_size_in_bytes);
