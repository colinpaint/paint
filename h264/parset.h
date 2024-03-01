#pragma once
#include "parsetcommon.h"
#include "nalu.h"

extern void ProcessSPS (VideoParameters *p_Vid, NALU_t *nalu);
extern void activateSPS (VideoParameters *p_Vid, seq_parameter_set_rbsp_t *sps);

extern void MakePPSavailable (VideoParameters *p_Vid, int id, pic_parameter_set_rbsp_t *pps);
extern void ProcessPPS (VideoParameters *p_Vid, NALU_t *nalu);
extern void CleanUpPPS (VideoParameters *p_Vid);

extern void UseParameterSet (Slice *currSlice);

#if (MVC_EXTENSION_ENABLE)
  extern void SubsetSPSConsistencyCheck (subset_seq_parameter_set_rbsp_t *subset_sps);
  extern void ProcessSubsetSPS (VideoParameters *p_Vid, NALU_t *nalu);
  extern void mvc_vui_parameters_extension (MVCVUI_t *pMVCVUI, Bitstream *s);
  extern void seq_parameter_set_mvc_extension (subset_seq_parameter_set_rbsp_t *subset_sps, Bitstream *s);
  extern void init_subset_sps_list (subset_seq_parameter_set_rbsp_t *subset_sps_list, int iSize);
  extern void reset_subset_sps (subset_seq_parameter_set_rbsp_t *subset_sps);
  extern int  GetBaseViewId (VideoParameters *p_Vid, subset_seq_parameter_set_rbsp_t **subset_sps);
  extern void get_max_dec_frame_buf_size (seq_parameter_set_rbsp_t *sps);
#endif
