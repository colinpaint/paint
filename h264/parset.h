#pragma once
#include "parsetcommon.h"
#include "nalu.h"

extern void ProcessSPS (VideoParameters *p_Vid, NALU_t *nalu);
extern void activateSPS (VideoParameters *p_Vid, seq_parameter_set_rbsp_t *sps);

extern void MakePPSavailable (VideoParameters *p_Vid, int id, pic_parameter_set_rbsp_t *pps);
extern void ProcessPPS (VideoParameters *p_Vid, NALU_t *nalu);
extern void CleanUpPPS (VideoParameters *p_Vid);

extern void UseParameterSet (Slice *currSlice);
