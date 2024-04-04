#pragma once

extern void reset_ecFlags (cDecoder264* decoder);
extern int setEcFlag (cDecoder264* decoder, int se);

extern int  get_concealed_element (cDecoder264* decoder, sSyntaxElement *se);
