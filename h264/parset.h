#pragma once
#include "parsetcommon.h"
#include "nalu.h"

extern int initGlobalBuffers (sVidParam* vidParam, int layer_id );
extern void free_global_buffers (sVidParam* vidParam);
extern void free_layer_buffers (sVidParam* vidParam, int layer_id );

extern void processSPS (sVidParam* vidParam, sNalu *nalu);
extern void activateSPS (sVidParam* vidParam, sSPS *sps);

extern void makePPSavailable (sVidParam* vidParam, int id, sPPS *pps);
extern void processPPS (sVidParam* vidParam, sNalu *nalu);
extern void cleanUpPPS (sVidParam* vidParam);

extern void useParameterSet (sSlice* curSlice);
