//{{{  includes
#include "global.h"
#include "memory.h"

#include "nalu.h"
#include "erc.h"

#include "../common/cLog.h"

using namespace std;
//}}}

namespace {
  //{{{
  void readHrdFromStream (sDataPartition* dataPartition, cHrd* hrd) {

    cBitStream& s = dataPartition->bitStream;
    hrd->cpb_cnt_minus1 = s.readUeV ("VUI cpb_cnt_minus1");
    hrd->bit_rate_scale = s.readUv (4, "VUI bit_rate_scale");
    hrd->cpb_size_scale = s.readUv (4, "VUI cpb_size_scale");

    uint32_t SchedSelIdx;
    for (SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++) {
      hrd->bit_rate_value_minus1[ SchedSelIdx] = s.readUeV ("VUI bit_rate_value_minus1");
      hrd->cpb_size_value_minus1[ SchedSelIdx] = s.readUeV ("VUI cpb_size_value_minus1");
      hrd->cbrFlag[ SchedSelIdx ] = s.readU1 ("VUI cbrFlag");
      }

    hrd->initial_cpb_removal_delay_length_minus1 =
      s.readUv (5, "VUI initial_cpb_removal_delay_length_minus1");
    hrd->cpb_removal_delay_length_minus1 = s.readUv (5, "VUI cpb_removal_delay_length_minus1");
    hrd->dpb_output_delay_length_minus1 = s.readUv (5, "VUI dpb_output_delay_length_minus1");
    hrd->time_offset_length = s.readUv (5, "VUI time_offset_length");
    }
  //}}}
  //{{{
  void scalingList (cBitStream& s, int* scalingList, int scalingListSize, bool* useDefaultScalingMatrix) {

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
        int delta_scale = s.readSeV ("   : delta_sl");
        nextScale = (lastScale + delta_scale + 256) % 256;
        *useDefaultScalingMatrix = (bool)(scanj == 0 && nextScale == 0);
        }

      scalingList[scanj] = (nextScale == 0) ? lastScale : nextScale;
      lastScale = scalingList[scanj];
      }
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
int cSps::readNalu (cDecoder264* decoder, cNalu* nalu) {

  sDataPartition* dataPartition = allocDataPartitions (1);
  dataPartition->bitStream.errorFlag = 0;
  dataPartition->bitStream.readLen = dataPartition->bitStream.bitStreamOffset = 0;
  memcpy (dataPartition->bitStream.bitStreamBuffer, &nalu->buf[1], nalu->len-1);
  dataPartition->bitStream.codeLen = dataPartition->bitStream.bitStreamLen = nalu->RBSPtoSODB (dataPartition->bitStream.bitStreamBuffer);

  cSps sps;
  sps.naluLen = nalu->len;
  sps.readFromStream (decoder, dataPartition);
  freeDataPartitions (dataPartition, 1);

  if (!decoder->sps[sps.id].isEqual (sps))
    cLog::log (LOGINFO, fmt::format ("-----> cSps::readNalu new sps id:%d", sps.id));

  memcpy (&decoder->sps[sps.id], &sps, sizeof(cSps));

  return sps.id;
  }
//}}}

//{{{
void cSps::readFromStream (cDecoder264* decoder, sDataPartition* dataPartition) {

  cBitStream& s = dataPartition->bitStream;

  profileIdc = (eProfileIDC)s.readUv (8, "SPS profileIdc");
  if ((profileIdc != BASELINE) && (profileIdc != MAIN) && (profileIdc != EXTENDED) &&
      (profileIdc != FREXT_HP) && (profileIdc != FREXT_Hi10P) &&
      (profileIdc != FREXT_Hi422) && (profileIdc != FREXT_Hi444) &&
      (profileIdc != FREXT_CAVLC444))
    printf ("IDC - invalid %d\n", profileIdc);

  constrainedSet0Flag = s.readU1 ("SPS constrainedSet0Flag");
  constrainedSet1Flag = s.readU1 ("SPS constrainedSet1Flag");
  constrainedSet2Flag = s.readU1 ("SPS constrainedSet2Flag");
  constrainedSet3flag = s.readU1 ("SPS constrainedSet3flag");
  int reservedZero = s.readUv (4, "SPS reservedZero4bits");

  levelIdc = s.readUv (8, "SPS levelIdc");
  id = s.readUeV ("SPS spsId");

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
    chromaFormatIdc = s.readUeV ("SPS chromaFormatIdc");
    if (chromaFormatIdc == YUV444)
      isSeperateColourPlane = s.readU1 ("SPS isSeperateColourPlane");
    bit_depth_luma_minus8 = s.readUeV ("SPS bit_depth_luma_minus8");
    bit_depth_chroma_minus8 = s.readUeV ("SPS bit_depth_chroma_minus8");
    if ((bit_depth_luma_minus8+8 > sizeof(sPixel)*8) ||
        (bit_depth_chroma_minus8+8> sizeof(sPixel)*8))
      cDecoder264::error ("Source picture has higher bit depth than sPixel data type");

    useLosslessQpPrime = s.readU1 ("SPS losslessQpPrimeYzero");

    hasSeqScalingMatrix = s.readU1 ("SPS hasSeqScalingMatrix");
    if (hasSeqScalingMatrix) {
      uint32_t n_ScalingList = (chromaFormatIdc != YUV444) ? 8 : 12;
      for (uint32_t i = 0; i < n_ScalingList; i++) {
        hasSeqScalingList[i] = s.readU1 ("SPS hasSeqScalingList");
        if (hasSeqScalingList[i]) {
          if (i < 6)
            scalingList (s, scalingList4x4[i], 16, &useDefaultScalingMatrix4x4[i]);
          else
            scalingList (s, scalingList8x8[i-6], 64, &useDefaultScalingMatrix8x8[i-6]);
          }
        }
      }
    }
  //}}}
  log2maxFrameNumMinus4 = s.readUeV ("SPS log2maxFrameNumMinus4");
  //{{{  read POC
  pocType = s.readUeV ("SPS pocType");

  if (pocType == 0)
    log2maxPocLsbMinus4 = s.readUeV ("SPS log2maxPocLsbMinus4");

  else if (pocType == 1) {
    deltaPicOrderAlwaysZero = s.readU1 ("SPS deltaPicOrderAlwaysZero");
    offsetNonRefPic = s.readSeV ("SPS offsetNonRefPic");
    offsetTopBotField = s.readSeV ("SPS offsetTopBotField");
    numRefFramesPocCycle = s.readUeV ("SPS numRefFramesPocCycle");
    for (uint32_t i = 0; i < numRefFramesPocCycle; i++)
      offsetForRefFrame[i] = s.readSeV ("SPS offsetRefFrame[i]");
    }
  //}}}

  numRefFrames = s.readUeV ("SPS numRefFrames");
  allowGapsFrameNum = s.readU1 ("SPS allowGapsFrameNum");

  picWidthMbsMinus1 = s.readUeV ("SPS picWidthMbsMinus1");
  picHeightMapUnitsMinus1 = s.readUeV ("SPS picHeightMapUnitsMinus1");

  frameMbOnly = s.readU1 ("SPS frameMbOnly");
  if (!frameMbOnly)
    mbAffFlag = s.readU1 ("SPS mbAffFlag");

  isDirect8x8inference = s.readU1 ("SPS isDirect8x8inference");

  //{{{  read crop
  hasCrop = s.readU1 ("SPS hasCrop");
  if (hasCrop) {
    cropLeft = s.readUeV ("SPS cropLeft");
    cropRight = s.readUeV ("SPS cropRight");
    cropTop = s.readUeV ("SPS cropTop");
    cropBot = s.readUeV ("SPS cropBot");
    }
  //}}}
  hasVui = (bool)s.readU1 ("SPS hasVui");

  vuiSeqParams.matrix_coefficients = 2;
  readVuiFromStream (dataPartition);

  ok = true;
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

  cBitStream& s = dataPartition->bitStream;
  if (hasVui) {
    vuiSeqParams.aspect_ratio_info_presentFlag = s.readU1 ("VUI aspect_ratio_info_presentFlag");
    if (vuiSeqParams.aspect_ratio_info_presentFlag) {
      vuiSeqParams.aspect_ratio_idc = s.readUv ( 8, "VUI aspect_ratio_idc");
      if (255 == vuiSeqParams.aspect_ratio_idc) {
        vuiSeqParams.sar_width = (uint16_t)s.readUv (16, "VUI sar_width");
        vuiSeqParams.sar_height = (uint16_t)s.readUv (16, "VUI sar_height");
        }
      }

    vuiSeqParams.overscan_info_presentFlag = s.readU1 ("VUI overscan_info_presentFlag");
    if (vuiSeqParams.overscan_info_presentFlag)
      vuiSeqParams.overscan_appropriateFlag = s.readU1 ("VUI overscan_appropriateFlag");

    vuiSeqParams.video_signal_type_presentFlag = s.readU1 ("VUI video_signal_type_presentFlag");
    if (vuiSeqParams.video_signal_type_presentFlag) {
      vuiSeqParams.video_format = s.readUv (3, "VUI video_format");
      vuiSeqParams.video_full_rangeFlag = s.readU1 ("VUI video_full_rangeFlag");
      vuiSeqParams.colour_description_presentFlag = s.readU1 ("VUI color_description_presentFlag");
      if (vuiSeqParams.colour_description_presentFlag) {
        vuiSeqParams.colour_primaries = s.readUv (8, "VUI colour_primaries");
        vuiSeqParams.transfer_characteristics = s.readUv (8, "VUI transfer_characteristics");
        vuiSeqParams.matrix_coefficients = s.readUv (8, "VUI matrix_coefficients");
        }
      }

    vuiSeqParams.chroma_location_info_presentFlag = s.readU1 ("VUI chroma_loc_info_presentFlag");
    if (vuiSeqParams.chroma_location_info_presentFlag) {
      vuiSeqParams.chroma_sample_loc_type_top_field = s.readUeV ("VUI chroma_sample_loc_type_top_field");
      vuiSeqParams.chroma_sample_loc_type_bottom_field = s.readUeV ("VUI chroma_sample_loc_type_bottom_field");
      }

    vuiSeqParams.timing_info_presentFlag = s.readU1 ("VUI timing_info_presentFlag");
    if (vuiSeqParams.timing_info_presentFlag) {
      vuiSeqParams.num_units_in_tick = s.readUv (32, "VUI num_units_in_tick");
      vuiSeqParams.time_scale = s.readUv (32,"VUI time_scale");
      vuiSeqParams.fixed_frame_rateFlag = s.readU1 ("VUI fixed_frame_rateFlag");
      }

    vuiSeqParams.nal_hrd_parameters_presentFlag = s.readU1 ("VUI nal_hrd_parameters_presentFlag");
    if (vuiSeqParams.nal_hrd_parameters_presentFlag)
      readHrdFromStream (dataPartition, &(vuiSeqParams.nal_hrd_parameters));

    vuiSeqParams.vcl_hrd_parameters_presentFlag = s.readU1 ("VUI vcl_hrd_parameters_presentFlag");

    if (vuiSeqParams.vcl_hrd_parameters_presentFlag)
      readHrdFromStream (dataPartition, &(vuiSeqParams.vcl_hrd_parameters));

    if (vuiSeqParams.nal_hrd_parameters_presentFlag ||
        vuiSeqParams.vcl_hrd_parameters_presentFlag)
      vuiSeqParams.low_delay_hrdFlag = s.readU1 ("VUI low_delay_hrdFlag");

    vuiSeqParams.pic_struct_presentFlag = s.readU1 ("VUI pic_struct_presentFlag   ");
    vuiSeqParams.bitstream_restrictionFlag = s.readU1 ("VUI bitstream_restrictionFlag");

    if (vuiSeqParams.bitstream_restrictionFlag) {
      vuiSeqParams.motion_vectors_over_pic_boundariesFlag = s.readU1 ("VUI motion_vectors_over_pic_boundariesFlag");
      vuiSeqParams.max_bytes_per_pic_denom = s.readUeV ("VUI max_bytes_per_pic_denom");
      vuiSeqParams.max_bits_per_mb_denom = s.readUeV ("VUI max_bits_per_mb_denom");
      vuiSeqParams.log2_max_mv_length_horizontal = s.readUeV ("VUI log2_max_mv_length_horizontal");
      vuiSeqParams.log2_max_mv_length_vertical = s.readUeV ("VUI log2_max_mv_length_vertical");
      vuiSeqParams.num_reorder_frames = s.readUeV ("VUI num_reorder_frames");
      vuiSeqParams.max_dec_frame_buffering = s.readUeV ("VUI max_dec_frame_buffering");
      }
    }
  }
//}}}
