#pragma once
#include "global.h"

#define IS_I16MB(MB) (((MB)->mbType == I16MB) || ((MB)->mbType == IPCM))
#define IS_DIRECT(MB) (((MB)->mbType == 0) && (slice->sliceType == eSliceB))

void cabacNewSlice (cSlice* slice);
int cabacStartCode (cSlice* slice, int eos_bit);

void checkNeighbourCabac (sMacroBlock* mb);
int checkNextMbGetFieldModeCabacSliceP (cSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);
int checkNextMbGetFieldModeCabacSliceB (cSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);

void readMbTypeInfoCabacSliceI (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMbTypeInfoCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMbTypeInfoCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readB8TypeInfoCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readB8TypeInfoCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readIntraPredModeCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readRefFrameCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readMvdCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMvdCabacMbAff (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readCbpCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readRunLevelCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readQuantCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readCiPredModCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readSkipFlagCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readSkipFlagCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readFieldModeCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMbTransformSizeFlagCabac (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

int readSyntaxElementCabac(sMacroBlock* mb, sSyntaxElement* se, sDataPartition* this_dataPart);

void readIpcmCabac (cSlice* slice, sDataPartition* dataPartition);

void setReadStoreCodedBlockPattern(sMacroBlock** mb, int chromaFormatIdc);
