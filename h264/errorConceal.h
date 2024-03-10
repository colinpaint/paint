#pragma once

extern void reset_ec_flags (sDecoder* vidParam);
extern int setEcFlag (sDecoder* vidParam, int se);

extern int  get_concealed_element (sDecoder* vidParam, sSyntaxElement *sym);
