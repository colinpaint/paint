#pragma once
#include "global.h"
#include "nalu.h"

#define MAX_SPS            4
#define MAX_REF_FRAMES_POC 16

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

// sSps
typedef struct {
  Boolean  ok;
  int      naluLen;

  unsigned int profileIdc;              // u(8)
  Boolean  constrained_set0_flag;       // u(1)
  Boolean  constrained_set1_flag;       // u(1)
  Boolean  constrained_set2_flag;       // u(1)
  Boolean  constrainedSet3flag;         // u(1)
  unsigned int levelIdc;                // u(8)
  unsigned int id;                      // ue(v)

  unsigned int chromaFormatIdc;         // ue(v)
  unsigned isSeperateColourPlane;       // u(1)
  unsigned int bit_depth_luma_minus8;   // ue(v)
  unsigned int bit_depth_chroma_minus8; // ue(v)
  int      useLosslessQpPrime;

  Boolean  hasSeqScalingMatrix;         // u(1)
  //{{{  optional scalingMatrix fields
  int      hasSeqScalingList[12]; // u(1)
  int      scalingList4x4[6][16];             // se(v)
  int      scalingList8x8[6][64];             // se(v)
  Boolean  useDefaultScalingMatrix4x4[6];
  Boolean  useDefaultScalingMatrix8x8[6];
  //}}}

  unsigned int log2maxFrameNumMinus4;   // ue(v)

  unsigned int pocType;
  //{{{  optional poc fields
  unsigned int log2maxPocLsbMinus4;            // ue(v)
  Boolean  deltaPicOrderAlwaysZero;            // u(1)
  int      offsetNonRefPic;                    // se(v)
  int      offsetTopBotField;                  // se(v)
  unsigned int numRefFramesPocCycle;           // ue(v)
  int      offsetForRefFrame[MAX_REF_FRAMES_POC]; // se(v)
  //}}}

  unsigned int numRefFrames;            // ue(v)
  Boolean  allowGapsFrameNum;           // u(1)

  unsigned int picWidthMbsMinus1;       // ue(v)
  unsigned int picHeightMapUnitsMinus1; // ue(v)

  Boolean  frameMbOnly;                 // u(1)
  //{{{  optional mbAffFlag
  Boolean  mbAffFlag;                   // u(1)
  //}}}

  Boolean  isDirect8x8inference;        // u(1)

  //{{{  optional crop
  Boolean  cropFlag;                    // u(1)
  unsigned int cropLeft;                // ue(v)
  unsigned int cropRight;               // ue(v)
  unsigned int cropTop;                 // ue(v)
  unsigned int cropBot;                 // ue(v)
  //}}}
  //{{{  optional vui
  Boolean  hasVui;                      // u(1)
  sVUI     vuiSeqParams;                // sVUI
  //}}}
  } sSps;

extern int readNaluSps (struct Decoder* decoder, sNalu* nalu);

//{{{
#ifdef __cplusplus
  extern "C" {
#endif
//}}}
  extern void getSpsStr (sSps* sps, char* str);
//{{{
#ifdef __cplusplus
  }
#endif
//}}}
