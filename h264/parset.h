#pragma once
#include "parsetcommon.h"
#include "nalu.h"

extern int init_global_buffers (sVidParam* vidParam, int layer_id );
extern void free_global_buffers (sVidParam* vidParam);
extern void free_layer_buffers (sVidParam* vidParam, int layer_id );

extern void processSPS (sVidParam* vidParam, sNalu *nalu);
extern void activateSPS (sVidParam* vidParam, sSPSrbsp *sps);

extern void makePPSavailable (sVidParam* vidParam, int id, sPPSrbsp *pps);
extern void processPPS (sVidParam* vidParam, sNalu *nalu);
extern void cleanUpPPS (sVidParam* vidParam);

extern void useParameterSet (sSlice* currSlice);
