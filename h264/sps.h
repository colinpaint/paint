#pragma once
#include "defines.h"
#include "nalu.h"

#define MAX_NUM_REF_FRAMES_PIC_ORDER  256

//{{{  sHRD
typedef struct {
  unsigned int cpb_cnt_minus1;             // ue(v)
  unsigned int bit_rate_scale;             // u(4)
  unsigned int cpb_size_scale;             // u(4)
  unsigned int bit_rate_value_minus1[32];  // ue(v)
  unsigned int cpb_size_value_minus1[32];  // ue(v)
  unsigned int cbr_flag[32];               // u(1)
  unsigned int initial_cpb_removal_delay_length_minus1;  // u(5)
  unsigned int cpb_removal_delay_length_minus1;          // u(5)
  unsigned int dpb_output_delay_length_minus1;           // u(5)
  unsigned int time_offset_length;                       // u(5)
  } sHRD;
//}}}
//{{{  sVUI
typedef struct {
  Boolean      aspect_ratio_info_present_flag;       // u(1)
  unsigned int aspect_ratio_idc;                     // u(8)

  unsigned short sar_width;                          // u(16)
  unsigned short sar_height;                         // u(16)

  Boolean      overscan_info_present_flag;           // u(1)
  Boolean      overscan_appropriate_flag;            // u(1)

  Boolean      video_signal_type_present_flag;       // u(1)
  unsigned int video_format;                         // u(3)
  Boolean      video_full_range_flag;                // u(1)

  Boolean      colour_description_present_flag;      // u(1)
  unsigned int colour_primaries;                     // u(8)
  unsigned int transfer_characteristics;             // u(8)
  unsigned int matrix_coefficients;                  // u(8)
  Boolean      chroma_location_info_present_flag;    // u(1)
  unsigned int chroma_sample_loc_type_top_field;     // ue(v)
  unsigned int chroma_sample_loc_type_bottom_field;  // ue(v)

  Boolean      timing_info_present_flag;             // u(1)
  unsigned int num_units_in_tick;                    // u(32)
  unsigned int time_scale;                           // u(32)
  Boolean      fixed_frame_rate_flag;                // u(1)

  Boolean      nal_hrd_parameters_present_flag;      // u(1)
  sHRD         nal_hrd_parameters;                   // hrd_paramters_t

  Boolean      vcl_hrd_parameters_present_flag;      // u(1)
  sHRD         vcl_hrd_parameters;                   // hrd_paramters_t

  // if ((nal_hrd_parameters_present_flag || (vcl_hrd_parameters_present_flag))
  Boolean      low_delay_hrd_flag;                      // u(1)
  Boolean      pic_struct_present_flag;                 // u(1)
  Boolean      bitstream_restriction_flag;              // u(1)
  Boolean      motion_vectors_over_pic_boundaries_flag; // u(1)

  unsigned int max_bytes_per_pic_denom;                 // ue(v)
  unsigned int max_bits_per_mb_denom;                   // ue(v)
  unsigned int log2_max_mv_length_vertical;             // ue(v)
  unsigned int log2_max_mv_length_horizontal;           // ue(v)
  unsigned int num_reorder_frames;                      // ue(v)
  unsigned int max_dec_frame_buffering;                 // ue(v)
  } sVUI;
//}}}

// sSPS
typedef struct {
  Boolean  valid;

  unsigned int profileIdc;            // u(8)
  Boolean  constrained_set0_flag;     // u(1)
  Boolean  constrained_set1_flag;     // u(1)
  Boolean  constrained_set2_flag;     // u(1)
  Boolean  constrainedSet3flag;       // u(1)
  unsigned int levelIdc;              // u(8)
  unsigned int spsId;                 // ue(v)
  unsigned int chromaFormatIdc;       // ue(v)

  Boolean  seq_scaling_matrix_present_flag;   // u(1)
  int      seq_scaling_list_present_flag[12]; // u(1)
  int      scalingList4x4[6][16];             // se(v)
  int      scalingList8x8[6][64];             // se(v)
  Boolean  useDefaultScalingMatrix4x4Flag[6];
  Boolean  useDefaultScalingMatrix8x8Flag[6];

  unsigned int bit_depth_luma_minus8;             // ue(v)
  unsigned int bit_depth_chroma_minus8;           // ue(v)
  unsigned int log2maxFrameNumMinus4;         // ue(v)
  unsigned int pocType;
  unsigned int log2_max_pic_order_cnt_lsb_minus4; // ue(v)
  Boolean  delta_pic_order_always_zero_flag;      // u(1)
  int      offsetNonRefPic;                       // se(v)
  int      offsetTopBotField;                     // se(v)

  unsigned int numRefFramesPocCycle;          // ue(v)
  int      offset_for_ref_frame[MAX_NUM_REF_FRAMES_PIC_ORDER]; // se(v)
  unsigned int numRefFrames;                      // ue(v)

  Boolean  gaps_in_frame_num_value_allowed_flag;  // u(1)

  unsigned int pic_width_in_mbs_minus1;           // ue(v)
  unsigned int pic_height_in_map_units_minus1;    // ue(v)

  Boolean  frameMbOnlyFlag;             // u(1)
  Boolean  mbAffFlag;                   // u(1)
  Boolean  direct_8x8_inference_flag;   // u(1)

  Boolean  cropFlag;                    // u(1)
  unsigned int cropLeft;                // ue(v)
  unsigned int cropRight;               // ue(v)
  unsigned int cropTop;                 // ue(v)
  unsigned int cropBot;                 // ue(v)

  Boolean  vui_parameters_present_flag; // u(1)
  sVUI     vui_seq_parameters;          // sVUI

  unsigned sepColourPlaneFlag;          // u(1)
  int      losslessQpPrimeFlag;
  } sSPS;

struct Decoder;
extern void readNaluSPS (struct Decoder* decoder, sNalu* nalu);
extern void activateSPS (struct Decoder* decoder, sSPS* sps);
