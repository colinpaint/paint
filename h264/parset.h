#pragma once
#include "defines.h"
#include "nalu.h"

sPPS* allocPPS();
void freePPS (sPPS* pps);

extern void processSPS (sDecoder* vidParam, sNalu* nalu);
extern void activateSPS (sDecoder* vidParam, sSPS* sps);

extern void makePPSavailable (sDecoder* vidParam, int id, sPPS* pps);
extern void processPPS (sDecoder* vidParam, sNalu* nalu);
extern void cleanUpPPS (sDecoder* vidParam);

extern void useParameterSet (sSlice* curSlice);
