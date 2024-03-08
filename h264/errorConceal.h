#pragma once

extern void reset_ec_flags (sVidParam* vidParam);
extern int setEcFlag (sVidParam* vidParam, int se);

extern int  get_concealed_element (sVidParam* vidParam, sSyntaxElement *sym);
