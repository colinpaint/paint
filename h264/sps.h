#pragma once
#include "defines.h"
#include "nalu.h"

extern void readNaluSPS (sDecoder* decoder, sNalu* nalu);
extern void activateSPS (sDecoder* decoder, sSPS* sps);
