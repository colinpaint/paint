#pragma once
#include "defines.h"
#include "nalu.h"

sPPS* allocPPS();
void freePPS (sPPS* pps);

extern void processSPS (sDecoder* decoder, sNalu* nalu);
extern void activateSPS (sDecoder* decoder, sSPS* sps);

extern void makePPSavailable (sDecoder* decoder, int id, sPPS* pps);
extern void processPPS (sDecoder* decoder, sNalu* nalu);

extern void useParameterSet (sSlice* slice);
