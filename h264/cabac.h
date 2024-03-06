#pragma once
#include "global.h"

extern MotionInfoContexts*  create_contexts_MotionInfo();
extern TextureInfoContexts* create_contexts_TextureInfo();
extern void delete_contexts_MotionInfo (MotionInfoContexts *enco_ctx);
extern void delete_contexts_TextureInfo (TextureInfoContexts *enco_ctx);

extern void cabac_new_slice (sSlice* currSlice);

extern void readMB_typeInfo_CABAC_i_slice (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readMB_typeInfo_CABAC_p_slice (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readMB_typeInfo_CABAC_b_slice (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readB8_typeInfo_CABAC_p_slice (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readB8_typeInfo_CABAC_b_slice (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readIntraPredMode_CABAC (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readRefFrame_CABAC (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void read_MVD_CABAC (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void read_mvd_CABAC_mbaff (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void read_CBP_CABAC (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readRunLevel_CABAC (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void read_dQuant_CABAC (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readCIPredMode_CABAC (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void read_skip_flag_CABAC_p_slice (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void read_skip_flag_CABAC_b_slice (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readFieldModeInfo_CABAC (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);
extern void readMB_transform_size_flag_CABAC (sMacroblock* currMB, sSyntaxElement *se, DecodingEnvironmentPtr dep_dp);

extern void readIPCM_CABAC (sSlice* currSlice, struct DataPartition *dP);
extern int  cabac_startcode_follows (sSlice* currSlice, int eos_bit);
extern int  readsSyntaxElement_CABAC (sMacroblock* currMB, sSyntaxElement *se, sDataPartition *this_dataPart);

extern int check_next_mb_and_get_field_mode_CABAC_p_slice (sSlice* currSlice, sSyntaxElement *se, sDataPartition  *act_dp);
extern int check_next_mb_and_get_field_mode_CABAC_b_slice (sSlice* currSlice, sSyntaxElement *se, sDataPartition  *act_dp);

extern void CheckAvailabilityOfNeighborsCABAC (sMacroblock* currMB);

extern void set_read_and_store_CBP (sMacroblock** currMB, int chroma_format_idc);
