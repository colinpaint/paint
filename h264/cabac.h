#pragma once
#include "global.h"

extern sMotionInfoContexts*  create_contexts_MotionInfo();
extern sTextureInfoContexts* create_contexts_TextureInfo();
extern void delete_contexts_MotionInfo (sMotionInfoContexts *enco_ctx);
extern void delete_contexts_TextureInfo (sTextureInfoContexts *enco_ctx);

extern void cabac_new_slice (sSlice* curSlice);

extern void readMB_typeInfo_CABAC_i_slice (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readMB_typeInfo_CABAC_p_slice (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readMB_typeInfo_CABAC_b_slice (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readB8_typeInfo_CABAC_p_slice (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readB8_typeInfo_CABAC_b_slice (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readIntraPredMode_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readRefFrame_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void read_MVD_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void read_mvd_CABAC_mbaff (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void read_CBP_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readRunLevel_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void read_dQuant_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readCIPredMode_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void read_skip_flag_CABAC_p_slice (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void read_skip_flag_CABAC_b_slice (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readFieldModeInfo_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);
extern void readMB_transform_size_flag_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDecodingEnvironmentPtr dep_dp);

extern void readIPCM_CABAC (sSlice* curSlice, struct DataPartition *dP);
extern int  cabac_startcode_follows (sSlice* curSlice, int eos_bit);
extern int  readsSyntaxElement_CABAC (sMacroblock* curMb, sSyntaxElement *se, sDataPartition *this_dataPart);

extern int check_next_mb_and_get_field_mode_CABAC_p_slice (sSlice* curSlice, sSyntaxElement *se, sDataPartition  *act_dp);
extern int check_next_mb_and_get_field_mode_CABAC_b_slice (sSlice* curSlice, sSyntaxElement *se, sDataPartition  *act_dp);

extern void CheckAvailabilityOfNeighborsCABAC (sMacroblock* curMb);

extern void set_read_and_store_CBP (sMacroblock** curMb, int chromaFormatIdc);
extern void set_read_CBP_and_coeffs_cabac (sSlice* curSlice);
extern void set_read_comp_coeff_cabac (sMacroblock* curMb);
