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
  void readHrdFromStream (sBitStream* bitStream, cHrd* hrd) {

    hrd->cpb_cnt_minus1 = bitStream->readUeV ("VUI cpb_cnt_minus1");
    hrd->bit_rate_scale = bitStream->readUv (4, "VUI bit_rate_scale");
    hrd->cpb_size_scale = bitStream->readUv (4, "VUI cpb_size_scale");

    uint32_t SchedSelIdx;
    for (SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++) {
      hrd->bit_rate_value_minus1[ SchedSelIdx] = bitStream->readUeV ("VUI bit_rate_value_minus1");
      hrd->cpb_size_value_minus1[ SchedSelIdx] = bitStream->readUeV ("VUI cpb_size_value_minus1");
      hrd->cbrFlag[ SchedSelIdx ] = bitStream->readU1 ("VUI cbrFlag");
      }

    hrd->initial_cpb_removal_delay_length_minus1 =
      bitStream->readUv (5, "VUI initial_cpb_removal_delay_length_minus1");
    hrd->cpb_removal_delay_length_minus1 = bitStream->readUv (5, "VUI cpb_removal_delay_length_minus1");
    hrd->dpb_output_delay_length_minus1 = bitStream->readUv (5, "VUI dpb_output_delay_length_minus1");
    hrd->time_offset_length = bitStream->readUv (5, "VUI time_offset_length");
    }
  //}}}
  //{{{
  void scalingList (sBitStream* bitStream, int* list, int listSize, bool* useDefaultMatrix) {
  // syntax for scaling list matrix values

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
    for (int j = 0; j < listSize; j++) {
      int scanj = (listSize == 16) ? ZZ_SCAN[j] : ZZ_SCAN8[j];
      if (nextScale != 0) {
        int deltaScale = bitStream->readSeV ("   : delta_sl   ");
        nextScale = (lastScale + deltaScale + 256) % 256;
        *useDefaultMatrix = (scanj == 0) && (nextScale == 0);
        }

      list[scanj] = (nextScale == 0) ? lastScale : nextScale;
      lastScale = list[scanj];
      }
    }
  //}}}
  }

// static
//{{{
int cSps::readNalu (cDecoder264* decoder, cNalu* nalu) {

  cSps sps;
  sps.naluLen = nalu->getLength();

  // !! no need to be dynamic !!!
  sBitStream* bitStream = (sBitStream*)calloc (1, sizeof(sBitStream));
  bitStream->mLength = nalu->getSodb (bitStream->mBuffer, bitStream->mAllocSize);
  bitStream->mCodeLen = bitStream->mLength;
  sps.readFromStream (decoder, bitStream);
  free (bitStream);

  if (!decoder->sps[sps.id].isEqual (sps))
    cLog::log (LOGINFO, fmt::format ("-----> cSps::readNalu new sps id:%d", sps.id));

  memcpy (&decoder->sps[sps.id], &sps, sizeof(cSps));

  return sps.id;
  }
//}}}

//{{{
string cSps::getString() {

  return fmt::format ("SPS:{}:{} -> mb:{}x{}{}{}{}{}",
                      id, naluLen,
                      picWidthMbsMinus1, picHeightMapUnitsMinus1,
                      hasCrop ? fmt::format (" crop:{}:{}:{}:{}", cropLeft, cropRight, cropTop, cropBot):"",
                      numRefFrames ? fmt::format (" refFrames:{}", numRefFrames):"",
                      pocType ? fmt::format (" pocType:{}", pocType):"",
                      frameMbOnly ? (mbAffFlag ? " mbAff":" frame"):" field"
                      );
  }
//}}}
//{{{
void cSps::readFromStream (cDecoder264* decoder, sBitStream* bitStream) {

  profileIdc = (eProfileIDC)bitStream->readUv (8, "SPS profileIdc");
  if ((profileIdc != BASELINE) && (profileIdc != MAIN) && (profileIdc != EXTENDED) &&
      (profileIdc != FREXT_HP) && (profileIdc != FREXT_Hi10P) &&
      (profileIdc != FREXT_Hi422) && (profileIdc != FREXT_Hi444) &&
      (profileIdc != FREXT_CAVLC444))
    printf ("IDC - invalid %d\n", profileIdc);

  constrainedSet0Flag = bitStream->readU1 ("SPS constrainedSet0Flag");
  constrainedSet1Flag = bitStream->readU1 ("SPS constrainedSet1Flag");
  constrainedSet2Flag = bitStream->readU1 ("SPS constrainedSet2Flag");
  constrainedSet3flag = bitStream->readU1 ("SPS constrainedSet3flag");
  int reservedZero = bitStream->readUv (4, "SPS reservedZero4bits");

  levelIdc = bitStream->readUv (8, "SPS levelIdc");
  id = bitStream->readUeV ("SPS spsId");

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
    chromaFormatIdc = bitStream->readUeV ("SPS chromaFormatIdc");
    if (chromaFormatIdc == YUV444)
      isSeperateColourPlane = bitStream->readU1 ("SPS isSeperateColourPlane");
    bit_depth_luma_minus8 = bitStream->readUeV ("SPS bit_depth_luma_minus8");
    bit_depth_chroma_minus8 = bitStream->readUeV ("SPS bit_depth_chroma_minus8");
    if ((bit_depth_luma_minus8+8 > sizeof(sPixel)*8) ||
        (bit_depth_chroma_minus8+8> sizeof(sPixel)*8))
      cDecoder264::error ("Source picture has higher bit depth than sPixel data type");

    useLosslessQpPrime = bitStream->readU1 ("SPS losslessQpPrimeYzero");

    hasSeqScalingMatrix = bitStream->readU1 ("SPS hasSeqScalingMatrix");
    if (hasSeqScalingMatrix) {
      uint32_t n_ScalingList = (chromaFormatIdc != YUV444) ? 8 : 12;
      for (uint32_t i = 0; i < n_ScalingList; i++) {
        hasSeqScalingList[i] = bitStream->readU1 ("SPS hasSeqScalingList");
        if (hasSeqScalingList[i]) {
          if (i < 6)
            scalingList (bitStream, scalingList4x4[i], 16, &useDefaultScalingMatrix4x4[i]);
          else
            scalingList (bitStream, scalingList8x8[i-6], 64, &useDefaultScalingMatrix8x8[i-6]);
          }
        }
      }
    }
  //}}}
  log2maxFrameNumMinus4 = bitStream->readUeV ("SPS log2maxFrameNumMinus4");
  //{{{  read POC
  pocType = bitStream->readUeV ("SPS pocType");

  if (pocType == 0)
    log2maxPocLsbMinus4 = bitStream->readUeV ("SPS log2maxPocLsbMinus4");

  else if (pocType == 1) {
    deltaPicOrderAlwaysZero = bitStream->readU1 ("SPS deltaPicOrderAlwaysZero");
    offsetNonRefPic = bitStream->readSeV ("SPS offsetNonRefPic");
    offsetTopBotField = bitStream->readSeV ("SPS offsetTopBotField");
    numRefFramesPocCycle = bitStream->readUeV ("SPS numRefFramesPocCycle");
    for (uint32_t i = 0; i < numRefFramesPocCycle; i++)
      offsetForRefFrame[i] = bitStream->readSeV ("SPS offsetRefFrame[i]");
    }
  //}}}

  numRefFrames = bitStream->readUeV ("SPS numRefFrames");
  allowGapsFrameNum = bitStream->readU1 ("SPS allowGapsFrameNum");

  picWidthMbsMinus1 = bitStream->readUeV ("SPS picWidthMbsMinus1");
  picHeightMapUnitsMinus1 = bitStream->readUeV ("SPS picHeightMapUnitsMinus1");

  frameMbOnly = bitStream->readU1 ("SPS frameMbOnly");
  if (!frameMbOnly)
    mbAffFlag = bitStream->readU1 ("SPS mbAffFlag");

  isDirect8x8inference = bitStream->readU1 ("SPS isDirect8x8inference");

  //{{{  read crop
  hasCrop = bitStream->readU1 ("SPS hasCrop");
  if (hasCrop) {
    cropLeft = bitStream->readUeV ("SPS cropLeft");
    cropRight = bitStream->readUeV ("SPS cropRight");
    cropTop = bitStream->readUeV ("SPS cropTop");
    cropBot = bitStream->readUeV ("SPS cropBot");
    }
  //}}}
  hasVui = (bool)bitStream->readU1 ("SPS hasVui");

  vuiSeqParams.matrix_coefficients = 2;
  readVuiFromStream (bitStream);

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
void cSps::readVuiFromStream (sBitStream* bitStream) {

  if (hasVui) {
    vuiSeqParams.aspect_ratio_info_presentFlag = bitStream->readU1 ("VUI aspect_ratio_info_presentFlag");
    if (vuiSeqParams.aspect_ratio_info_presentFlag) {
      vuiSeqParams.aspect_ratio_idc = bitStream->readUv ( 8, "VUI aspect_ratio_idc");
      if (255 == vuiSeqParams.aspect_ratio_idc) {
        vuiSeqParams.sar_width = (uint16_t)bitStream->readUv (16, "VUI sar_width");
        vuiSeqParams.sar_height = (uint16_t)bitStream->readUv (16, "VUI sar_height");
        }
      }

    vuiSeqParams.overscan_info_presentFlag = bitStream->readU1 ("VUI overscan_info_presentFlag");
    if (vuiSeqParams.overscan_info_presentFlag)
      vuiSeqParams.overscan_appropriateFlag = bitStream->readU1 ("VUI overscan_appropriateFlag");

    vuiSeqParams.video_signal_type_presentFlag = bitStream->readU1 ("VUI video_signal_type_presentFlag");
    if (vuiSeqParams.video_signal_type_presentFlag) {
      vuiSeqParams.video_format = bitStream->readUv (3, "VUI video_format");
      vuiSeqParams.video_full_rangeFlag = bitStream->readU1 ("VUI video_full_rangeFlag");
      vuiSeqParams.colour_description_presentFlag = bitStream->readU1 ("VUI color_description_presentFlag");
      if (vuiSeqParams.colour_description_presentFlag) {
        vuiSeqParams.colour_primaries = bitStream->readUv (8, "VUI colour_primaries");
        vuiSeqParams.transfer_characteristics = bitStream->readUv (8, "VUI transfer_characteristics");
        vuiSeqParams.matrix_coefficients = bitStream->readUv (8, "VUI matrix_coefficients");
        }
      }

    vuiSeqParams.chroma_location_info_presentFlag = bitStream->readU1 ("VUI chroma_loc_info_presentFlag");
    if (vuiSeqParams.chroma_location_info_presentFlag) {
      vuiSeqParams.chroma_sample_loc_type_top_field = bitStream->readUeV ("VUI chroma_sample_loc_type_top_field");
      vuiSeqParams.chroma_sample_loc_type_bottom_field = bitStream->readUeV ("VUI chroma_sample_loc_type_bottom_field");
      }

    vuiSeqParams.timing_info_presentFlag = bitStream->readU1 ("VUI timing_info_presentFlag");
    if (vuiSeqParams.timing_info_presentFlag) {
      vuiSeqParams.num_units_in_tick = bitStream->readUv (32, "VUI num_units_in_tick");
      vuiSeqParams.time_scale = bitStream->readUv (32,"VUI time_scale");
      vuiSeqParams.fixed_frame_rateFlag = bitStream->readU1 ("VUI fixed_frame_rateFlag");
      }

    vuiSeqParams.nal_hrd_parameters_presentFlag = bitStream->readU1 ("VUI nal_hrd_parameters_presentFlag");
    if (vuiSeqParams.nal_hrd_parameters_presentFlag)
      readHrdFromStream (bitStream, &vuiSeqParams.nal_hrd_parameters);

    vuiSeqParams.vcl_hrd_parameters_presentFlag = bitStream->readU1 ("VUI vcl_hrd_parameters_presentFlag");

    if (vuiSeqParams.vcl_hrd_parameters_presentFlag)
      readHrdFromStream (bitStream, &vuiSeqParams.vcl_hrd_parameters);

    if (vuiSeqParams.nal_hrd_parameters_presentFlag ||
        vuiSeqParams.vcl_hrd_parameters_presentFlag)
      vuiSeqParams.low_delay_hrdFlag = bitStream->readU1 ("VUI low_delay_hrdFlag");

    vuiSeqParams.pic_struct_presentFlag = bitStream->readU1 ("VUI pic_struct_presentFlag   ");
    vuiSeqParams.bitstream_restrictionFlag = bitStream->readU1 ("VUI bitstream_restrictionFlag");

    if (vuiSeqParams.bitstream_restrictionFlag) {
      vuiSeqParams.motion_vectors_over_pic_boundariesFlag = bitStream->readU1 ("VUI motion_vectors_over_pic_boundariesFlag");
      vuiSeqParams.max_bytes_per_pic_denom = bitStream->readUeV ("VUI max_bytes_per_pic_denom");
      vuiSeqParams.max_bits_per_mb_denom = bitStream->readUeV ("VUI max_bits_per_mb_denom");
      vuiSeqParams.log2_max_mv_length_horizontal = bitStream->readUeV ("VUI log2_max_mv_length_horizontal");
      vuiSeqParams.log2_max_mv_length_vertical = bitStream->readUeV ("VUI log2_max_mv_length_vertical");
      vuiSeqParams.num_reorder_frames = bitStream->readUeV ("VUI num_reorder_frames");
      vuiSeqParams.max_dec_frame_buffering = bitStream->readUeV ("VUI max_dec_frame_buffering");
      }
    }
  }
//}}}
