#pragma once
#include "defines.h"
#include "global.h"

extern ANNEXB_t* allocAnnexB (sDecoder* decoder);
extern void openAnnexB (ANNEXB_t* annexB, byte* chunk, size_t chunkSize);
extern void resetAnnexB (ANNEXB_t* annexB);
extern void freeAnnexB (ANNEXB_t** p_annexB);

extern sNalu* allocNALU (int);
extern void freeNALU (sNalu* n);

extern void checkZeroByteVCL (sDecoder* decoder, sNalu* nalu);
extern void checkZeroByteNonVCL (sDecoder* decoder, sNalu* nalu);

extern int readNextNalu (sDecoder* decoder, sNalu* nalu);
extern int RBSPtoSODB (byte* streamBuffer, int last_byte_pos);
