#pragma once

extern void reset_ec_flags (sDecoder* decoder);
extern int setEcFlag (sDecoder* decoder, int se);

extern int  get_concealed_element (sDecoder* decoder, sSyntaxElement *sym);
