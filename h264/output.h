#pragma once

extern void flush_pending_output (sVidParam* vidParam);
extern void init_out_buffer (sVidParam* vidParam);
extern void uninit_out_buffer (sVidParam* vidParam);

extern void write_stored_frame (sVidParam* vidParam, sFrameStore* fs);
extern void direct_output (sVidParam* vidParam, sPicture* p);
extern void init_output (sCodingParams* p_CodingParams, int symbol_size_in_bytes);
