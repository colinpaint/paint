#pragma once
#include "global.h"

extern sMotionInfoContexts*  createMotionInfoContexts();
extern sTextureInfoContexts* createTextureInfoContexts();
extern void deleteMotionInfoContexts (sMotionInfoContexts* contexts);
extern void deleteTextureInfoContexts (sTextureInfoContexts* contexts);

extern void cabacNewSlice (sSlice* slice);

extern void readMB_typeInfo_CABAC_i_slice (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void readMB_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void readMB_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void readB8_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void readB8_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);

extern void readIntraPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void readRefFrame_CABAC (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);

extern void read_MVD_CABAC (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void read_mvd_CABAC_mbaff (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void read_CBP_CABAC (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);

extern void readRunLevel_CABAC (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void read_dQuant_CABAC (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void readCIPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);

extern void read_skip_flag_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void read_skip_flag_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void readFieldModeInfo_CABAC (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);
extern void readMB_transform_size_flag_CABAC (sMacroBlock* mb, sSyntaxElement* se, sDecodeEnv* decodeEnv);

extern void readIPCMcabac (sSlice* slice, sDataPartition* dataPartition);
extern int cabacStartCode (sSlice* slice, int eos_bit);
extern int readSyntaxElementCABAC (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* this_dataPart);

extern int check_next_mb_and_get_field_mode_CABAC_p_slice (sSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);
extern int check_next_mb_and_get_field_mode_CABAC_b_slice (sSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);

extern void checkNeighbourCabac (sMacroBlock* mb);

extern void set_read_and_store_CBP (sMacroBlock** mb, int chromaFormatIdc);
extern void set_read_CBP_and_coeffs_cabac (sSlice* slice);
extern void set_read_comp_coeff_cabac (sMacroBlock* mb);
