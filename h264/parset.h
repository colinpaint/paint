#pragma once
#include "parsetcommon.h"
#include "nalu.h"

extern int init_global_buffers (VideoParameters* pVid, int layer_id );
extern void free_global_buffers (VideoParameters* pVid);
extern void free_layer_buffers (VideoParameters* pVid, int layer_id );

extern void ProcessSPS (VideoParameters* pVid, NALU_t *nalu);
extern void activateSPS (VideoParameters* pVid, seq_parameter_set_rbsp_t *sps);

extern void MakePPSavailable (VideoParameters* pVid, int id, pic_parameter_set_rbsp_t *pps);
extern void ProcessPPS (VideoParameters* pVid, NALU_t *nalu);
extern void CleanUpPPS (VideoParameters* pVid);

extern void UseParameterSet (Slice* currSlice);
