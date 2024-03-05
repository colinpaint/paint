#pragma once
#include "parsetcommon.h"
#include "nalu.h"

extern int init_global_buffers (sVidParam* vidParam, int layer_id );
extern void free_global_buffers (sVidParam* vidParam);
extern void free_layer_buffers (sVidParam* vidParam, int layer_id );

extern void ProcessSPS (sVidParam* vidParam, NALU_t *nalu);
extern void activateSPS (sVidParam* vidParam, seq_parameter_set_rbsp_t *sps);

extern void MakePPSavailable (sVidParam* vidParam, int id, pic_parameter_set_rbsp_t *pps);
extern void ProcessPPS (sVidParam* vidParam, NALU_t *nalu);
extern void CleanUpPPS (sVidParam* vidParam);

extern void UseParameterSet (Slice* currSlice);
