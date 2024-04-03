#pragma once
#include "global.h"

#define IS_I16MB(MB)     (((MB)->mbType == I16MB) || ((MB)->mbType == IPCM))
#define IS_DIRECT(MB)    (((MB)->mbType == 0) && (slice->sliceType == eSliceB))

extern sMotionContexts* createMotionInfoContexts();
extern sTextureContexts* createTextureInfoContexts();
extern void deleteMotionInfoContexts (sMotionContexts* contexts);
extern void deleteTextureInfoContexts (sTextureContexts* contexts);

extern void cabacNewSlice (sSlice* slice);

extern void readMB_typeInfo_CABAC_i_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void readMB_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void readMB_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void readB8_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void readB8_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

extern void readIntraPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void readRefFrame_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

extern void read_MVD_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void read_mvd_CABAC_mbaff (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void read_CBP_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

extern void readRunLevel_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void read_dQuant_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void readCIPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

extern void read_skipFlag_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void read_skipFlag_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void readFieldModeInfo_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);
extern void readMB_transform_sizeFlag_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv);

extern void readIPCMcabac (sSlice* slice, sDataPartition* dataPartition);
extern int cabacStartCode (sSlice* slice, int eos_bit);
extern int readSyntaxElementCABAC (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* this_dataPart);

extern int checkNextMbGetFieldModeCabacSliceP (sSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);
extern int checkNextMbGetFieldModeCabacSliceB (sSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);

extern void checkNeighbourCabac (sMacroBlock* mb);

extern void setReadStoreCodedBlockPattern (sMacroBlock** mb, int chromaFormatIdc);
extern void setReadCbpCoefsCabac (sSlice* slice);
extern void setReadCompCabac (sMacroBlock* mb);
