#pragma once
#include "global.h"
#include "nalu.h"

#define MAX_SPS            4
#define MAX_REF_FRAMES_POC 16

//{{{  sHRD
typedef struct {
  uint32_t cpb_cnt_minus1;             // ue(v)
  uint32_t bit_rate_scale;             // u(4)
  uint32_t cpb_size_scale;             // u(4)
  uint32_t bit_rate_value_minus1[32];  // ue(v)
  uint32_t cpb_size_value_minus1[32];  // ue(v)
  uint32_t cbr_flag[32];               // u(1)
  uint32_t initial_cpb_removal_delay_length_minus1;  // u(5)
  uint32_t cpb_removal_delay_length_minus1;          // u(5)
  uint32_t dpb_output_delay_length_minus1;           // u(5)
  uint32_t time_offset_length;                       // u(5)
  } sHRD;
//}}}
//{{{  sVUI
typedef struct {
  bool      aspect_ratio_info_present_flag;       // u(1)
  uint32_t aspect_ratio_idc;                     // u(8)

  uint16_t sar_width;                          // u(16)
  uint16_t sar_height;                         // u(16)

  bool      overscan_info_present_flag;           // u(1)
  bool      overscan_appropriate_flag;            // u(1)

  bool      video_signal_type_present_flag;       // u(1)
  uint32_t video_format;                         // u(3)
  bool      video_full_range_flag;                // u(1)

  bool      colour_description_present_flag;      // u(1)
  uint32_t colour_primaries;                     // u(8)
  uint32_t transfer_characteristics;             // u(8)
  uint32_t matrix_coefficients;                  // u(8)
  bool      chroma_location_info_present_flag;    // u(1)
  uint32_t chroma_sample_loc_type_top_field;     // ue(v)
  uint32_t chroma_sample_loc_type_bottom_field;  // ue(v)

  bool      timing_info_present_flag;             // u(1)
  uint32_t num_units_in_tick;                    // u(32)
  uint32_t time_scale;                           // u(32)
  bool      fixed_frame_rate_flag;                // u(1)

  bool      nal_hrd_parameters_present_flag;      // u(1)
  sHRD         nal_hrd_parameters;                   // hrd_paramters_t

  bool      vcl_hrd_parameters_present_flag;      // u(1)
  sHRD         vcl_hrd_parameters;                   // hrd_paramters_t

  // if ((nal_hrd_parameters_present_flag || (vcl_hrd_parameters_present_flag))
  bool      low_delay_hrd_flag;                      // u(1)
  bool      pic_struct_present_flag;                 // u(1)
  bool      bitstream_restriction_flag;              // u(1)
  bool      motion_vectors_over_pic_boundaries_flag; // u(1)

  uint32_t max_bytes_per_pic_denom;                 // ue(v)
  uint32_t max_bits_per_mb_denom;                   // ue(v)
  uint32_t log2_max_mv_length_vertical;             // ue(v)
  uint32_t log2_max_mv_length_horizontal;           // ue(v)
  uint32_t num_reorder_frames;                      // ue(v)
  uint32_t max_dec_frame_buffering;                 // ue(v)
  } sVUI;
//}}}

// sSps
struct sSps {
  bool     ok;
  int      naluLen;

  uint32_t profileIdc;              // u(8)
  bool     constrained_set0_flag;   // u(1)
  bool     constrained_set1_flag;   // u(1)
  bool     constrained_set2_flag;   // u(1)
  bool     constrainedSet3flag;     // u(1)
  uint32_t levelIdc;                // u(8)
  uint32_t id;                      // ue(v)

  uint32_t chromaFormatIdc;         // ue(v)
  uint32_t isSeperateColourPlane;   // u(1)
  uint32_t bit_depth_luma_minus8;   // ue(v)
  uint32_t bit_depth_chroma_minus8; // ue(v)
  int      useLosslessQpPrime;

  bool     hasSeqScalingMatrix;     // u(1)
  //{{{  optional scalingMatrix fields
  int      hasSeqScalingList[12]; // u(1)
  int      scalingList4x4[6][16];             // se(v)
  int      scalingList8x8[6][64];             // se(v)
  bool  useDefaultScalingMatrix4x4[6];
  bool  useDefaultScalingMatrix8x8[6];
  //}}}

  uint32_t log2maxFrameNumMinus4;   // ue(v)

  uint32_t pocType;
  //{{{  optional poc fields
  uint32_t log2maxPocLsbMinus4;            // ue(v)
  bool  deltaPicOrderAlwaysZero;            // u(1)
  int      offsetNonRefPic;                    // se(v)
  int      offsetTopBotField;                  // se(v)
  uint32_t numRefFramesPocCycle;           // ue(v)
  int      offsetForRefFrame[MAX_REF_FRAMES_POC]; // se(v)
  //}}}

  uint32_t numRefFrames;            // ue(v)
  bool     allowGapsFrameNum;       // u(1)

  uint32_t picWidthMbsMinus1;       // ue(v)
  uint32_t picHeightMapUnitsMinus1; // ue(v)

  bool     frameMbOnly;             // u(1)
  //{{{  optional mbAffFlag
  bool  mbAffFlag;                   // u(1)
  //}}}

  bool     isDirect8x8inference;    // u(1)
  //{{{  optional crop
  bool  cropFlag;                    // u(1)
  uint32_t cropLeft;                // ue(v)
  uint32_t cropRight;               // ue(v)
  uint32_t cropTop;                 // ue(v)
  uint32_t cropBot;                 // ue(v)
  //}}}
  //{{{  optional vui
  bool  hasVui;                      // u(1)
  sVUI     vuiSeqParams;                // sVUI
  //}}}
  };

std::string getSpsString (sSps* sps);
int readNaluSps (sDecoder* decoder, sNalu* nalu);
