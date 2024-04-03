//{{{  includes
#include "global.h"
#include "memory.h"

#include "sps.h"
#include "nalu.h"
#include "vlc.h"
#include "erc.h"
#include "image.h"

#include "../common/cLog.h"

using namespace std;
//}}}

namespace {
  //{{{
  // syntax for scaling list matrix values
  void scalingList (int* scalingList, int scalingListSize, bool* useDefaultScalingMatrix, sBitStream* s) {

    //{{{
    static const uint8_t ZZ_SCAN[16] = {
      0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
      };
    //}}}
    //{{{
    static const uint8_t ZZ_SCAN8[64] = {
      0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
      12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
      35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
      58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
      };
    //}}}

    int lastScale = 8;
    int nextScale = 8;
    for (int j = 0; j < scalingListSize; j++) {
      int scanj = (scalingListSize == 16) ? ZZ_SCAN[j] : ZZ_SCAN8[j];
      if (nextScale != 0) {
        int delta_scale = readSeV ("   : delta_sl   ", s);
        nextScale = (lastScale + delta_scale + 256) % 256;
        *useDefaultScalingMatrix = (bool)(scanj == 0 && nextScale == 0);
        }

      scalingList[scanj] = (nextScale == 0) ? lastScale : nextScale;
      lastScale = scalingList[scanj];
      }
    }
  //}}}
  //{{{
  void readHrdFromStream (sDataPartition* dataPartition, sHrd* hrd) {

    sBitStream *s = dataPartition->stream;
    hrd->cpb_cnt_minus1 = readUeV ("VUI cpb_cnt_minus1", s);
    hrd->bit_rate_scale = readUv (4, "VUI bit_rate_scale", s);
    hrd->cpb_size_scale = readUv (4, "VUI cpb_size_scale", s);

    uint32_t SchedSelIdx;
    for (SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++) {
      hrd->bit_rate_value_minus1[ SchedSelIdx] = readUeV ("VUI bit_rate_value_minus1", s);
      hrd->cpb_size_value_minus1[ SchedSelIdx] = readUeV ("VUI cpb_size_value_minus1", s);
      hrd->cbrFlag[ SchedSelIdx ] = readU1 ("VUI cbrFlag", s);
      }

    hrd->initial_cpb_removal_delay_length_minus1 =
      readUv (5, "VUI initial_cpb_removal_delay_length_minus1", s);
    hrd->cpb_removal_delay_length_minus1 = readUv (5, "VUI cpb_removal_delay_length_minus1", s);
    hrd->dpb_output_delay_length_minus1 = readUv (5, "VUI dpb_output_delay_length_minus1", s);
    hrd->time_offset_length = readUv (5, "VUI time_offset_length", s);
    }
  //}}}
  }

//{{{
string cSps::getString() {

  return fmt::format ("SPS:{}:{} -> mb:{}x{} {}:{}:{}:{} refFrames:{} pocType:{} {}",
                      id, naluLen,
                      picWidthMbsMinus1, picHeightMapUnitsMinus1,
                      cropLeft, cropRight, cropTop, cropBot,
                      numRefFrames,
                      pocType,
                      frameMbOnly ? (mbAffFlag ? "mbAff":"frame"):"field"
                      );
  }
//}}}

// static
//{{{
int cSps::readNalu (sDecoder* decoder, sNalu* nalu) {

  sDataPartition* dataPartition = allocDataPartitions (1);
  dataPartition->stream->errorFlag = 0;
  dataPartition->stream->readLen = dataPartition->stream->bitStreamOffset = 0;
  memcpy (dataPartition->stream->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
  dataPartition->stream->codeLen = dataPartition->stream->bitStreamLen = RBSPtoSODB (dataPartition->stream->bitStreamBuffer, nalu->len-1);

  cSps sps;
  sps.readSpsFromStream (decoder, dataPartition, nalu->len);
  freeDataPartitions (dataPartition, 1);

  if (!decoder->sps[sps.id].isEqual (sps))
    cLog::log (LOGINFO, fmt::format ("-----> readNaluSps new sps id:%d", sps.id));

  memcpy (&decoder->sps[sps.id], &sps, sizeof(cSps));

  return sps.id;
  }
//}}}

// private
//{{{
bool cSps::isEqual (cSps& sps) {

  if (!ok || !sps.ok)
    return false;

  bool equal = true;
  equal &= (id == sps.id);
  equal &= (levelIdc == sps.levelIdc);
  equal &= (profileIdc == sps.profileIdc);

  equal &= (constrainedSet0Flag == sps.constrainedSet0Flag);
  equal &= (constrainedSet1Flag == sps.constrainedSet1Flag);
  equal &= (constrainedSet2Flag == sps.constrainedSet2Flag);

  equal &= (log2maxFrameNumMinus4 == sps.log2maxFrameNumMinus4);
  equal &= (pocType == sps.pocType);
  if (!equal)
    return equal;

  if (pocType == 0)
    equal &= (log2maxPocLsbMinus4 == sps.log2maxPocLsbMinus4);
  else if (pocType == 1) {
    equal &= (deltaPicOrderAlwaysZero == sps.deltaPicOrderAlwaysZero);
    equal &= (offsetNonRefPic == sps.offsetNonRefPic);
    equal &= (offsetTopBotField == sps.offsetTopBotField);
    equal &= (numRefFramesPocCycle == sps.numRefFramesPocCycle);
    if (!equal)
      return equal;

    for (uint32_t i = 0 ; i < numRefFramesPocCycle ;i ++)
      equal &= (offsetForRefFrame[i] == sps.offsetForRefFrame[i]);
    }

  equal &= (numRefFrames == sps.numRefFrames);
  equal &= (allowGapsFrameNum == sps.allowGapsFrameNum);
  equal &= (picWidthMbsMinus1 == sps.picWidthMbsMinus1);
  equal &= (picHeightMapUnitsMinus1 == sps.picHeightMapUnitsMinus1);
  equal &= (frameMbOnly == sps.frameMbOnly);
  if (!equal)
    return equal;

  if (!frameMbOnly)
    equal &= (mbAffFlag == sps.mbAffFlag);
  equal &= (isDirect8x8inference == sps.isDirect8x8inference);
  equal &= (hasCrop == sps.hasCrop);
  if (!equal)
    return equal;

  if (hasCrop) {
    equal &= (cropLeft == sps.cropLeft);
    equal &= (cropRight == sps.cropRight);
    equal &= (cropTop == sps.cropTop);
    equal &= (cropBot == sps.cropBot);
    }
  equal &= (hasVui == sps.hasVui);

  return equal;
  }
//}}}
//{{{
void cSps::readVuiFromStream (sDataPartition* dataPartition) {

  sBitStream* s = dataPartition->stream;
  if (hasVui) {
    vuiSeqParams.aspect_ratio_info_presentFlag = readU1 ("VUI aspect_ratio_info_presentFlag", s);
    if (vuiSeqParams.aspect_ratio_info_presentFlag) {
      vuiSeqParams.aspect_ratio_idc = readUv ( 8, "VUI aspect_ratio_idc", s);
      if (255 == vuiSeqParams.aspect_ratio_idc) {
        vuiSeqParams.sar_width = (uint16_t)readUv (16, "VUI sar_width", s);
        vuiSeqParams.sar_height = (uint16_t)readUv (16, "VUI sar_height", s);
        }
      }

    vuiSeqParams.overscan_info_presentFlag = readU1 ("VUI overscan_info_presentFlag", s);
    if (vuiSeqParams.overscan_info_presentFlag)
      vuiSeqParams.overscan_appropriateFlag = readU1 ("VUI overscan_appropriateFlag", s);

    vuiSeqParams.video_signal_type_presentFlag = readU1 ("VUI video_signal_type_presentFlag", s);
    if (vuiSeqParams.video_signal_type_presentFlag) {
      vuiSeqParams.video_format = readUv (3, "VUI video_format", s);
      vuiSeqParams.video_full_rangeFlag = readU1 ("VUI video_full_rangeFlag", s);
      vuiSeqParams.colour_description_presentFlag = readU1 ("VUI color_description_presentFlag", s);
      if (vuiSeqParams.colour_description_presentFlag) {
        vuiSeqParams.colour_primaries = readUv (8, "VUI colour_primaries", s);
        vuiSeqParams.transfer_characteristics = readUv (8, "VUI transfer_characteristics", s);
        vuiSeqParams.matrix_coefficients = readUv (8, "VUI matrix_coefficients", s);
        }
      }

    vuiSeqParams.chroma_location_info_presentFlag = readU1 ("VUI chroma_loc_info_presentFlag", s);
    if (vuiSeqParams.chroma_location_info_presentFlag) {
      vuiSeqParams.chroma_sample_loc_type_top_field = readUeV ("VUI chroma_sample_loc_type_top_field", s);
      vuiSeqParams.chroma_sample_loc_type_bottom_field = readUeV ("VUI chroma_sample_loc_type_bottom_field", s);
      }

    vuiSeqParams.timing_info_presentFlag = readU1 ("VUI timing_info_presentFlag", s);
    if (vuiSeqParams.timing_info_presentFlag) {
      vuiSeqParams.num_units_in_tick = readUv (32, "VUI num_units_in_tick", s);
      vuiSeqParams.time_scale = readUv (32,"VUI time_scale", s);
      vuiSeqParams.fixed_frame_rateFlag = readU1 ("VUI fixed_frame_rateFlag", s);
      }

    vuiSeqParams.nal_hrd_parameters_presentFlag = readU1 ("VUI nal_hrd_parameters_presentFlag", s);
    if (vuiSeqParams.nal_hrd_parameters_presentFlag)
      readHrdFromStream (dataPartition, &(vuiSeqParams.nal_hrd_parameters));

    vuiSeqParams.vcl_hrd_parameters_presentFlag = readU1 ("VUI vcl_hrd_parameters_presentFlag", s);

    if (vuiSeqParams.vcl_hrd_parameters_presentFlag)
      readHrdFromStream (dataPartition, &(vuiSeqParams.vcl_hrd_parameters));

    if (vuiSeqParams.nal_hrd_parameters_presentFlag ||
        vuiSeqParams.vcl_hrd_parameters_presentFlag)
      vuiSeqParams.low_delay_hrdFlag = readU1 ("VUI low_delay_hrdFlag", s);

    vuiSeqParams.pic_struct_presentFlag = readU1 ("VUI pic_struct_presentFlag   ", s);
    vuiSeqParams.bitstream_restrictionFlag = readU1 ("VUI bitstream_restrictionFlag", s);

    if (vuiSeqParams.bitstream_restrictionFlag) {
      vuiSeqParams.motion_vectors_over_pic_boundariesFlag = readU1 ("VUI motion_vectors_over_pic_boundariesFlag", s);
      vuiSeqParams.max_bytes_per_pic_denom = readUeV ("VUI max_bytes_per_pic_denom", s);
      vuiSeqParams.max_bits_per_mb_denom = readUeV ("VUI max_bits_per_mb_denom", s);
      vuiSeqParams.log2_max_mv_length_horizontal = readUeV ("VUI log2_max_mv_length_horizontal", s);
      vuiSeqParams.log2_max_mv_length_vertical = readUeV ("VUI log2_max_mv_length_vertical", s);
      vuiSeqParams.num_reorder_frames = readUeV ("VUI num_reorder_frames", s);
      vuiSeqParams.max_dec_frame_buffering = readUeV ("VUI max_dec_frame_buffering", s);
      }
    }
  }
//}}}
//{{{
void cSps::readSpsFromStream (sDecoder* decoder, sDataPartition* dataPartition, int naluLen) {

  naluLen = naluLen;

  sBitStream* s = dataPartition->stream;

  profileIdc = (eProfileIDC)readUv (8, "SPS profileIdc", s);
  if ((profileIdc != BASELINE) && (profileIdc != MAIN) && (profileIdc != EXTENDED) &&
      (profileIdc != FREXT_HP) && (profileIdc != FREXT_Hi10P) &&
      (profileIdc != FREXT_Hi422) && (profileIdc != FREXT_Hi444) &&
      (profileIdc != FREXT_CAVLC444))
    printf ("IDC - invalid %d\n", profileIdc);

  constrainedSet0Flag = readU1 ("SPS constrainedSet0Flag", s);
  constrainedSet1Flag = readU1 ("SPS constrainedSet1Flag", s);
  constrainedSet2Flag = readU1 ("SPS constrainedSet2Flag", s);
  constrainedSet3flag = readU1 ("SPS constrainedSet3flag", s);
  int reservedZero = readUv (4, "SPS reservedZero4bits", s);

  levelIdc = readUv (8, "SPS levelIdc", s);
  id = readUeV ("SPS spsId", s);

  // Fidelity Range Extensions stuff
  chromaFormatIdc = 1;
  bit_depth_luma_minus8 = 0;
  bit_depth_chroma_minus8 = 0;
  useLosslessQpPrime = 0;
  isSeperateColourPlane = 0;

  //{{{  read fidelity range
  if ((profileIdc == FREXT_HP) ||
      (profileIdc == FREXT_Hi10P) ||
      (profileIdc == FREXT_Hi422) ||
      (profileIdc == FREXT_Hi444) ||
      (profileIdc == FREXT_CAVLC444)) {
    // read fidelity range
    chromaFormatIdc = readUeV ("SPS chromaFormatIdc", s);
    if (chromaFormatIdc == YUV444)
      isSeperateColourPlane = readU1 ("SPS isSeperateColourPlane", s);
    bit_depth_luma_minus8 = readUeV ("SPS bit_depth_luma_minus8", s);
    bit_depth_chroma_minus8 = readUeV ("SPS bit_depth_chroma_minus8", s);
    if ((bit_depth_luma_minus8+8 > sizeof(sPixel)*8) ||
        (bit_depth_chroma_minus8+8> sizeof(sPixel)*8))
      error ("Source picture has higher bit depth than sPixel data type");

    useLosslessQpPrime = readU1 ("SPS losslessQpPrimeYzero", s);

    hasSeqScalingMatrix = readU1 ("SPS hasSeqScalingMatrix", s);
    if (hasSeqScalingMatrix) {
      uint32_t n_ScalingList = (chromaFormatIdc != YUV444) ? 8 : 12;
      for (uint32_t i = 0; i < n_ScalingList; i++) {
        hasSeqScalingList[i] = readU1 ("SPS hasSeqScalingList", s);
        if (hasSeqScalingList[i]) {
          if (i < 6)
            scalingList (scalingList4x4[i], 16, &useDefaultScalingMatrix4x4[i], s);
          else
            scalingList (scalingList8x8[i-6], 64, &useDefaultScalingMatrix8x8[i-6], s);
          }
        }
      }
    }
  //}}}
  log2maxFrameNumMinus4 = readUeV ("SPS log2maxFrameNumMinus4", s);
  //{{{  read POC
  pocType = readUeV ("SPS pocType", s);

  if (pocType == 0)
    log2maxPocLsbMinus4 = readUeV ("SPS log2maxPocLsbMinus4", s);

  else if (pocType == 1) {
    deltaPicOrderAlwaysZero = readU1 ("SPS deltaPicOrderAlwaysZero", s);
    offsetNonRefPic = readSeV ("SPS offsetNonRefPic", s);
    offsetTopBotField = readSeV ("SPS offsetTopBotField", s);
    numRefFramesPocCycle = readUeV ("SPS numRefFramesPocCycle", s);
    for (uint32_t i = 0; i < numRefFramesPocCycle; i++)
      offsetForRefFrame[i] = readSeV ("SPS offsetRefFrame[i]", s);
    }
  //}}}

  numRefFrames = readUeV ("SPS numRefFrames", s);
  allowGapsFrameNum = readU1 ("SPS allowGapsFrameNum", s);

  picWidthMbsMinus1 = readUeV ("SPS picWidthMbsMinus1", s);
  picHeightMapUnitsMinus1 = readUeV ("SPS picHeightMapUnitsMinus1", s);

  frameMbOnly = readU1 ("SPS frameMbOnly", s);
  if (!frameMbOnly)
    mbAffFlag = readU1 ("SPS mbAffFlag", s);

  isDirect8x8inference = readU1 ("SPS isDirect8x8inference", s);

  //{{{  read crop
  hasCrop = readU1 ("SPS hasCrop", s);
  if (hasCrop) {
    cropLeft = readUeV ("SPS cropLeft", s);
    cropRight = readUeV ("SPS cropRight", s);
    cropTop = readUeV ("SPS cropTop", s);
    cropBot = readUeV ("SPS cropBot", s);
    }
  //}}}
  hasVui = (bool)readU1 ("SPS hasVui", s);

  vuiSeqParams.matrix_coefficients = 2;
  readVuiFromStream (dataPartition);

  ok = true;
  }
//}}}
