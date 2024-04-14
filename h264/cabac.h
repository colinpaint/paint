#pragma once
#include "global.h"

#define IS_I16MB(MB) (((MB)->mbType == I16MB) || ((MB)->mbType == IPCM))
#define IS_DIRECT(MB) (((MB)->mbType == 0) && (slice->sliceType == eSliceB))

void cabacNewSlice (cSlice* slice);
int cabacStartCode (cSlice* slice, int eos_bit);

void checkNeighbourCabac (sMacroBlock* mb);
int checkNextMbFieldCabacSliceP (cSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);
int checkNextMbFieldCabacSliceB (cSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);

int readSyntaxElementCabac (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* this_dataPart);

void readMbTypeCabacSliceI (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMbTypeCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMbTypeCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readB8TypeCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readB8TypeCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readRefFrameCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readIntraPredModeCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readMvdCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMvdCabacMbAff (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readCbpCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readQuantCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readRunLevelCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readCiPredModCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readSkipCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readSkipCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readFieldModeCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMbTransformSizeCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readIpcmCabac (cSlice* slice, sDataPartition* dataPartition);

void setReadCbp(sMacroBlock** mb, int chromaFormatIdc);
