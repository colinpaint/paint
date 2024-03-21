#pragma once
#include "global.h"

extern sMotionInfoContexts*  createMotionInfoContexts();
extern sTextureInfoContexts* createTextureInfoContexts();
extern void deleteMotionInfoContexts (sMotionInfoContexts* contexts);
extern void deleteTextureInfoContexts (sTextureInfoContexts* contexts);

extern void cabacNewSlice (sSlice* slice);

extern void readMB_typeInfo_CABAC_i_slice (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void readMB_typeInfo_CABAC_p_slice (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void readMB_typeInfo_CABAC_b_slice (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void readB8_typeInfo_CABAC_p_slice (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void readB8_typeInfo_CABAC_b_slice (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);

extern void readIntraPredMode_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void readRefFrame_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);

extern void read_MVD_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void read_mvd_CABAC_mbaff (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void read_CBP_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);

extern void readRunLevel_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void read_dQuant_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void readCIPredMode_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);

extern void read_skip_flag_CABAC_p_slice (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void read_skip_flag_CABAC_b_slice (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void readFieldModeInfo_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);
extern void readMB_transform_size_flag_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv);

extern void readIPCMcabac (sSlice* slice, sDataPartition* dataPartition);
extern int cabacStartCode (sSlice* slice, int eos_bit);
extern int readSyntaxElementCABAC (sMacroblock* mb, sSyntaxElement* se, sDataPartition* this_dataPart);

extern int check_next_mb_and_get_field_mode_CABAC_p_slice (sSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);
extern int check_next_mb_and_get_field_mode_CABAC_b_slice (sSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);

extern void checkNeighbourCabac (sMacroblock* mb);

extern void set_read_and_store_CBP (sMacroblock** mb, int chromaFormatIdc);
extern void set_read_CBP_and_coeffs_cabac (sSlice* slice);
extern void set_read_comp_coeff_cabac (sMacroblock* mb);
