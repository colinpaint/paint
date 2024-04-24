#pragma once
#include "global.h"

#define IS_I16MB(MB) (((MB)->mbType == I16MB) || ((MB)->mbType == IPCM))
#define IS_DIRECT(MB) (((MB)->mbType == 0) && (slice->sliceType == eSliceB))

void cabacNewSlice (cSlice* slice);
int cabacStartCode (cSlice* slice, int eos_bit);

void checkNeighbourCabac (sMacroBlock* mb);
int checkNextMbFieldCabacSliceP (cSlice* slice, sSyntaxElement* se, sDataPartition* dataPartition);
int checkNextMbFieldCabacSliceB (cSlice* slice, sSyntaxElement* se, sDataPartition* dataPartition);

int readSyntaxElementCabac (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* dataPartition);

void readMbTypeCabacSliceI (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readMbTypeCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readMbTypeCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readB8TypeCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readB8TypeCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);

void readRefFrameCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readIntraPredModeCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);

void readMvdCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readMvdCabacMbAff (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);

void readCbpCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readQuantCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readRunLevelCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readCiPredModCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);

void readSkipCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readSkipCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);

void readFieldModeCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);
void readMbTransformSizeCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode);

void readIpcmCabac (cSlice* slice, sDataPartition* dataPartition);

void setReadCbp(sMacroBlock** mb, int chromaFormatIdc);
