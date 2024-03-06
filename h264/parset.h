#pragma once
#include "parsetcommon.h"
#include "nalu.h"

extern void processSPS (sVidParam* vidParam, sNalu* nalu);
extern void activateSPS (sVidParam* vidParam, sSPS* sps);

extern void makePPSavailable (sVidParam* vidParam, int id, sPPS* pps);
extern void processPPS (sVidParam* vidParam, sNalu* nalu);
extern void cleanUpPPS (sVidParam* vidParam);

extern void useParameterSet (sSlice* curSlice);
