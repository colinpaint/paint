#pragma once
#include "global.h"
#include "nalu.h"

constexpr int kMaxSps = 4;
constexpr int kMaxRefframesPoc = 16;

//{{{  enum eProfileIDC
typedef enum {
  NO_PROFILE     = 0,   // disable profile checking for experimental coding (enables FRExt, but disables MV)
  FREXT_CAVLC444 = 44,  // YUV 4:4:4/14 "eCavlc 4:4:4"
  BASELINE       = 66,  // YUV 4:2:0/8  "Baseline"
  MAIN           = 77,  // YUV 4:2:0/8  "Main"
  EXTENDED       = 88,  // YUV 4:2:0/8  "Extended"
  FREXT_HP       = 100, // YUV 4:2:0/8  "High"
  FREXT_Hi10P    = 110, // YUV 4:2:0/10 "High 10"
  FREXT_Hi422    = 122, // YUV 4:2:2/10 "High 4:2:2"
  FREXT_Hi444    = 244, // YUV 4:4:4/14 "High 4:4:4"
  } eProfileIDC;
//}}}

struct sDataPartition;
//{{{
struct sHrd {
  uint32_t cpb_cnt_minus1;                          // ue(v)
  uint32_t bit_rate_scale;                          // u(4)
  uint32_t cpb_size_scale;                          // u(4)
  uint32_t bit_rate_value_minus1[32];               // ue(v)
  uint32_t cpb_size_value_minus1[32];               // ue(v)
  uint32_t cbrFlag[32];                             // u(1)
  uint32_t initial_cpb_removal_delay_length_minus1; // u(5)
  uint32_t cpb_removal_delay_length_minus1;         // u(5)
  uint32_t dpb_output_delay_length_minus1;          // u(5)
  uint32_t time_offset_length;                      // u(5)
  };
//}}}
//{{{
struct sVui {
  bool      aspect_ratio_info_presentFlag;       // u(1)
  uint32_t aspect_ratio_idc;                     // u(8)

  uint16_t sar_width;                            // u(16)
  uint16_t sar_height;                           // u(16)

  bool      overscan_info_presentFlag;           // u(1)
  bool      overscan_appropriateFlag;            // u(1)

  bool      video_signal_type_presentFlag;       // u(1)
  uint32_t video_format;                         // u(3)
  bool      video_full_rangeFlag;                // u(1)

  bool      colour_description_presentFlag;      // u(1)
  uint32_t colour_primaries;                     // u(8)
  uint32_t transfer_characteristics;             // u(8)
  uint32_t matrix_coefficients;                  // u(8)
  bool      chroma_location_info_presentFlag;    // u(1)
  uint32_t chroma_sample_loc_type_top_field;     // ue(v)
  uint32_t chroma_sample_loc_type_bottom_field;  // ue(v)

  bool     timing_info_presentFlag;              // u(1)
  uint32_t num_units_in_tick;                    // u(32)
  uint32_t time_scale;                           // u(32)
  bool     fixed_frame_rateFlag;                 // u(1)

  bool      nal_hrd_parameters_presentFlag;      // u(1)
  sHrd      nal_hrd_parameters;                  // hrd_paramters_t

  bool      vcl_hrd_parameters_presentFlag;      // u(1)
  sHrd      vcl_hrd_parameters;                  // hrd_paramters_t

  // if ((nal_hrd_parameters_presentFlag || (vcl_hrd_parameters_presentFlag))
  bool      low_delay_hrdFlag;                   // u(1)
  bool      pic_struct_presentFlag;              // u(1)
  bool      bitstream_restrictionFlag;           // u(1)
  bool      motion_vectors_over_pic_boundariesFlag; // u(1)

  uint32_t  max_bytes_per_pic_denom;             // ue(v)
  uint32_t  max_bits_per_mb_denom;               // ue(v)
  uint32_t  log2_max_mv_length_vertical;         // ue(v)
  uint32_t  log2_max_mv_length_horizontal;       // ue(v)
  uint32_t num_reorder_frames;                   // ue(v)
  uint32_t max_dec_frame_buffering;              // ue(v)
  };
//}}}

class cSps {
public:
  static int readNalu (sDecoder* decoder, sNalu* nalu);

  std::string getString();
  bool isEqual (cSps& sps);
  //{{{
  bool isBLprofile() {
    return (profileIdc == BASELINE) ||
           (profileIdc == MAIN) ||
           (profileIdc == EXTENDED) ||
           (profileIdc == FREXT_CAVLC444) ||
           (profileIdc == FREXT_HP) || (profileIdc == FREXT_Hi10P) ||
           (profileIdc == FREXT_Hi422) || (profileIdc == FREXT_Hi444);
    }
  //}}}
  //{{{
  bool isFrextProfile() {
  // we allow all FRExt tools, when no profile is active

    return (profileIdc == NO_PROFILE) ||
           (profileIdc == FREXT_HP) ||
           (profileIdc == FREXT_Hi10P) || (profileIdc == FREXT_Hi422) ||
           (profileIdc == FREXT_Hi444) || (profileIdc == FREXT_CAVLC444);
    }
  //}}}
  //{{{
  bool isHiIntraOnlyProfile() {
    return (((profileIdc == FREXT_Hi10P) ||
             (profileIdc == FREXT_Hi422) ||
             (profileIdc == FREXT_Hi444)) && constrainedSet3flag) ||
           (profileIdc == FREXT_CAVLC444);
    }
  //}}}

  void readSpsFromStream (sDecoder* decoder, sDataPartition* dataPartition, int naluLen);

  //{{{  vars
  bool     ok = false;
  int      naluLen = 0;

  eProfileIDC profileIdc = NO_PROFILE;   // u(8)

  bool     constrainedSet0Flag = false;  // u(1)
  bool     constrainedSet1Flag = false;  // u(1)
  bool     constrainedSet2Flag = false;  // u(1)
  bool     constrainedSet3flag = false;  // u(1)

  uint32_t levelIdc = 0;                 // u(8)
  uint32_t id = 0;                       // ue(v)

  uint32_t chromaFormatIdc = 0;          // ue(v)
  uint32_t isSeperateColourPlane = 0;    // u(1)
  uint32_t bit_depth_luma_minus8 = 0;    // ue(v)
  uint32_t bit_depth_chroma_minus8 = 0;  // ue(v)
  int      useLosslessQpPrime = 0;

  bool     hasSeqScalingMatrix = false;  // u(1)
  //{{{  optional scalingMatrix fields
  int hasSeqScalingList[12] = {0};      // u(1)
  int scalingList4x4[6][16] = {0};      // se(v)
  int scalingList8x8[6][64] = {0};      // se(v)

  bool useDefaultScalingMatrix4x4[6] = {false};
  bool useDefaultScalingMatrix8x8[6] = {false};
  //}}}

  uint32_t log2maxFrameNumMinus4;        // ue(v)

  uint32_t pocType;
  //{{{  optional poc fields
  uint32_t log2maxPocLsbMinus4 = 0;                     // ue(v)
  bool     deltaPicOrderAlwaysZero = false;             // u(1)
  int      offsetNonRefPic = 0;                         // se(v)
  int      offsetTopBotField = 0;                       // se(v)
  uint32_t numRefFramesPocCycle = 0;                    // ue(v)
  int      offsetForRefFrame[kMaxRefframesPoc] = {0}; // se(v)
  //}}}

  uint32_t numRefFrames;                 // ue(v)
  bool     allowGapsFrameNum = false;    // u(1)

  uint32_t picWidthMbsMinus1;            // ue(v)
  uint32_t picHeightMapUnitsMinus1;      // ue(v)

  bool     frameMbOnly = false;          // u(1)
  //{{{  optional mbAffFlag
  bool  mbAffFlag = false; // u(1)
  //}}}
  bool     isDirect8x8inference = false; // u(1)

  bool  hasCrop = false;                 // u(1)
  //{{{  optional crop
  uint32_t cropLeft = 0;  // ue(v)
  uint32_t cropRight = 0; // ue(v)
  uint32_t cropTop = 0;   // ue(v)
  uint32_t cropBot = 0;   // ue(v)
  //}}}

  bool hasVui = false;                   // u(1)
  //{{{  optional vui
  sVui vuiSeqParams = {0}; // sVui
  //}}}
  //}}}

private:
  void readVuiFromStream (sDataPartition* dataPartition);
  };
