#pragma once
#include "defines.h"
#include "nalu.h"

extern sPPS* allocPPS();
extern void freePPS (sPPS* pps);

extern void readNaluSPS (sDecoder* decoder, sNalu* nalu);
extern void activateSPS (sDecoder* decoder, sSPS* sps);

extern void setPPSbyId (sDecoder* decoder, int id, sPPS* pps);
extern void readNaluPPS (sDecoder* decoder, sNalu* nalu);
extern void activatePPS (sDecoder* decoder, sPPS* pps);
