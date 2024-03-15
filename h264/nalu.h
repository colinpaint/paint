#pragma once
#include "defines.h"
#include "global.h"

extern sAnnexB* allocAnnexB (sDecoder* decoder);
extern void openAnnexB (sAnnexB* annexB, byte* chunk, size_t chunkSize);
extern void resetAnnexB (sAnnexB* annexB);
extern void freeAnnexB (sAnnexB** p_annexB);

extern sNalu* allocNALU (int);
extern void freeNALU (sNalu* n);

extern void checkZeroByteVCL (sDecoder* decoder, sNalu* nalu);
extern void checkZeroByteNonVCL (sDecoder* decoder, sNalu* nalu);

extern int readNextNalu (sDecoder* decoder, sNalu* nalu);
extern int RBSPtoSODB (byte* bitStreamBuffer, int last_byte_pos);
