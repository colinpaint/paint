#pragma once
#include "global.h"

#define IS_I16MB(MB) (((MB)->mbType == I16MB) || ((MB)->mbType == IPCM))
#define IS_DIRECT(MB) (((MB)->mbType == 0) && (slice->sliceType == eSliceB))

sMotionContexts* createMotionInfoContexts();
sTextureContexts* createTextureInfoContexts();
void deleteMotionInfoContexts (sMotionContexts* contexts);
void deleteTextureInfoContexts (sTextureContexts* contexts);

void cabacNewSlice (cSlice* slice);

void readMB_typeInfo_CABAC_i_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void readMB_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void readMB_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void readB8_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void readB8_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

void readIntraPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void readRefFrame_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

void read_MVD_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void read_mvd_CABAC_mbaff (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void read_CBP_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

void readRunLevel_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void read_dQuant_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void readCIPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

void read_skipFlag_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void read_skipFlag_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void readFieldModeInfo_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
void readMB_transform_sizeFlag_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

void readIPCMcabac (cSlice* slice, sDataPartition* dataPartition);
int cabacStartCode (cSlice* slice, int eos_bit);
int readSyntaxElementCABAC (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* this_dataPart);

int checkNextMbGetFieldModeCabacSliceP (cSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);
int checkNextMbGetFieldModeCabacSliceB (cSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);

void checkNeighbourCabac (sMacroBlock* mb);

void setReadStoreCodedBlockPattern (sMacroBlock** mb, int chromaFormatIdc);
void setReadCbpCoefsCabac (cSlice* slice);
