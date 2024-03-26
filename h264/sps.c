//{{{  includes
#include "global.h"
#include "memory.h"

#include "sps.h"
#include "nalu.h"
#include "vlc.h"
#include "erc.h"
#include "image.h"
//}}}

//{{{
static int isEqualSps (sSps* sps1, sSps* sps2) {

  int equal = 1;
  if ((!sps1->ok) || (!sps2->ok))
    return 0;

  equal &= (sps1->id == sps2->id);
  equal &= (sps1->levelIdc == sps2->levelIdc);
  equal &= (sps1->profileIdc == sps2->profileIdc);

  equal &= (sps1->constrained_set0_flag == sps2->constrained_set0_flag);
  equal &= (sps1->constrained_set1_flag == sps2->constrained_set1_flag);
  equal &= (sps1->constrained_set2_flag == sps2->constrained_set2_flag);

  equal &= (sps1->log2maxFrameNumMinus4 == sps2->log2maxFrameNumMinus4);
  equal &= (sps1->pocType == sps2->pocType);
  if (!equal)
    return equal;

  if (sps1->pocType == 0)
    equal &= (sps1->log2maxPocLsbMinus4 == sps2->log2maxPocLsbMinus4);
  else if (sps1->pocType == 1) {
    equal &= (sps1->deltaPicOrderAlwaysZero == sps2->deltaPicOrderAlwaysZero);
    equal &= (sps1->offsetNonRefPic == sps2->offsetNonRefPic);
    equal &= (sps1->offsetTopBotField == sps2->offsetTopBotField);
    equal &= (sps1->numRefFramesPocCycle == sps2->numRefFramesPocCycle);
    if (!equal)
      return equal;

    for (unsigned i = 0 ; i < sps1->numRefFramesPocCycle ;i ++)
      equal &= (sps1->offsetForRefFrame[i] == sps2->offsetForRefFrame[i]);
    }

  equal &= (sps1->numRefFrames == sps2->numRefFrames);
  equal &= (sps1->allowGapsFrameNum == sps2->allowGapsFrameNum);
  equal &= (sps1->picWidthMbsMinus1 == sps2->picWidthMbsMinus1);
  equal &= (sps1->picHeightMapUnitsMinus1 == sps2->picHeightMapUnitsMinus1);
  equal &= (sps1->frameMbOnly == sps2->frameMbOnly);
  if (!equal) return
    equal;

  if (!sps1->frameMbOnly)
    equal &= (sps1->mbAffFlag == sps2->mbAffFlag);
  equal &= (sps1->isDirect8x8inference == sps2->isDirect8x8inference);
  equal &= (sps1->cropFlag == sps2->cropFlag);
  if (!equal)
    return equal;

  if (sps1->cropFlag) {
    equal &= (sps1->cropLeft == sps2->cropLeft);
    equal &= (sps1->cropRight == sps2->cropRight);
    equal &= (sps1->cropTop == sps2->cropTop);
    equal &= (sps1->cropBot == sps2->cropBot);
    }
  equal &= (sps1->hasVui == sps2->hasVui);

  return equal;
  }
//}}}
//{{{
// syntax for scaling list matrix values
static void scalingList (int* scalingList, int scalingListSize, Boolean* useDefaultScalingMatrix, sBitStream* s) {

  //{{{
  static const byte ZZ_SCAN[16] = {
    0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
    };
  //}}}
  //{{{
  static const byte ZZ_SCAN8[64] = {
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
      *useDefaultScalingMatrix = (Boolean)(scanj == 0 && nextScale == 0);
      }

    scalingList[scanj] = (nextScale == 0) ? lastScale : nextScale;
    lastScale = scalingList[scanj];
    }
  }
//}}}

//{{{
static void readHrdFromStream (sDataPartition* dataPartition, sHRD* hrd) {

  sBitStream *s = dataPartition->s;
  hrd->cpb_cnt_minus1 = readUeV ("VUI cpb_cnt_minus1", s);
  hrd->bit_rate_scale = readUv (4, "VUI bit_rate_scale", s);
  hrd->cpb_size_scale = readUv (4, "VUI cpb_size_scale", s);

  unsigned int SchedSelIdx;
  for (SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++) {
    hrd->bit_rate_value_minus1[ SchedSelIdx] = readUeV ("VUI bit_rate_value_minus1", s);
    hrd->cpb_size_value_minus1[ SchedSelIdx] = readUeV ("VUI cpb_size_value_minus1", s);
    hrd->cbr_flag[ SchedSelIdx ] = readU1 ("VUI cbr_flag", s);
    }

  hrd->initial_cpb_removal_delay_length_minus1 =
    readUv (5, "VUI initial_cpb_removal_delay_length_minus1", s);
  hrd->cpb_removal_delay_length_minus1 = readUv (5, "VUI cpb_removal_delay_length_minus1", s);
  hrd->dpb_output_delay_length_minus1 = readUv (5, "VUI dpb_output_delay_length_minus1", s);
  hrd->time_offset_length = readUv (5, "VUI time_offset_length", s);
  }
//}}}
//{{{
static void readVuiFromStream (sDataPartition* dataPartition, sSps* sps) {

  sBitStream* s = dataPartition->s;
  if (sps->hasVui) {
    sps->vuiSeqParams.aspect_ratio_info_present_flag = readU1 ("VUI aspect_ratio_info_present_flag", s);
    if (sps->vuiSeqParams.aspect_ratio_info_present_flag) {
      sps->vuiSeqParams.aspect_ratio_idc = readUv ( 8, "VUI aspect_ratio_idc", s);
      if (255 == sps->vuiSeqParams.aspect_ratio_idc) {
        sps->vuiSeqParams.sar_width = (unsigned short)readUv (16, "VUI sar_width", s);
        sps->vuiSeqParams.sar_height = (unsigned short)readUv (16, "VUI sar_height", s);
        }
      }

    sps->vuiSeqParams.overscan_info_present_flag = readU1 ("VUI overscan_info_present_flag", s);
    if (sps->vuiSeqParams.overscan_info_present_flag)
      sps->vuiSeqParams.overscan_appropriate_flag = readU1 ("VUI overscan_appropriate_flag", s);

    sps->vuiSeqParams.video_signal_type_present_flag = readU1 ("VUI video_signal_type_present_flag", s);
    if (sps->vuiSeqParams.video_signal_type_present_flag) {
      sps->vuiSeqParams.video_format = readUv (3, "VUI video_format", s);
      sps->vuiSeqParams.video_full_range_flag = readU1 ("VUI video_full_range_flag", s);
      sps->vuiSeqParams.colour_description_present_flag = readU1 ("VUI color_description_present_flag", s);
      if (sps->vuiSeqParams.colour_description_present_flag) {
        sps->vuiSeqParams.colour_primaries = readUv (8, "VUI colour_primaries", s);
        sps->vuiSeqParams.transfer_characteristics = readUv (8, "VUI transfer_characteristics", s);
        sps->vuiSeqParams.matrix_coefficients = readUv (8, "VUI matrix_coefficients", s);
        }
      }

    sps->vuiSeqParams.chroma_location_info_present_flag = readU1 ("VUI chroma_loc_info_present_flag", s);
    if (sps->vuiSeqParams.chroma_location_info_present_flag) {
      sps->vuiSeqParams.chroma_sample_loc_type_top_field = readUeV ("VUI chroma_sample_loc_type_top_field", s);
      sps->vuiSeqParams.chroma_sample_loc_type_bottom_field = readUeV ("VUI chroma_sample_loc_type_bottom_field", s);
      }

    sps->vuiSeqParams.timing_info_present_flag = readU1 ("VUI timing_info_present_flag", s);
    if (sps->vuiSeqParams.timing_info_present_flag) {
      sps->vuiSeqParams.num_units_in_tick = readUv (32, "VUI num_units_in_tick", s);
      sps->vuiSeqParams.time_scale = readUv (32,"VUI time_scale", s);
      sps->vuiSeqParams.fixed_frame_rate_flag = readU1 ("VUI fixed_frame_rate_flag", s);
      }

    sps->vuiSeqParams.nal_hrd_parameters_present_flag = readU1 ("VUI nal_hrd_parameters_present_flag", s);
    if (sps->vuiSeqParams.nal_hrd_parameters_present_flag)
      readHrdFromStream (dataPartition, &(sps->vuiSeqParams.nal_hrd_parameters));

    sps->vuiSeqParams.vcl_hrd_parameters_present_flag = readU1 ("VUI vcl_hrd_parameters_present_flag", s);

    if (sps->vuiSeqParams.vcl_hrd_parameters_present_flag)
      readHrdFromStream (dataPartition, &(sps->vuiSeqParams.vcl_hrd_parameters));

    if (sps->vuiSeqParams.nal_hrd_parameters_present_flag ||
        sps->vuiSeqParams.vcl_hrd_parameters_present_flag)
      sps->vuiSeqParams.low_delay_hrd_flag = readU1 ("VUI low_delay_hrd_flag", s);

    sps->vuiSeqParams.pic_struct_present_flag = readU1 ("VUI pic_struct_present_flag   ", s);
    sps->vuiSeqParams.bitstream_restriction_flag = readU1 ("VUI bitstream_restriction_flag", s);

    if (sps->vuiSeqParams.bitstream_restriction_flag) {
      sps->vuiSeqParams.motion_vectors_over_pic_boundaries_flag = readU1 ("VUI motion_vectors_over_pic_boundaries_flag", s);
      sps->vuiSeqParams.max_bytes_per_pic_denom = readUeV ("VUI max_bytes_per_pic_denom", s);
      sps->vuiSeqParams.max_bits_per_mb_denom = readUeV ("VUI max_bits_per_mb_denom", s);
      sps->vuiSeqParams.log2_max_mv_length_horizontal = readUeV ("VUI log2_max_mv_length_horizontal", s);
      sps->vuiSeqParams.log2_max_mv_length_vertical = readUeV ("VUI log2_max_mv_length_vertical", s);
      sps->vuiSeqParams.num_reorder_frames = readUeV ("VUI num_reorder_frames", s);
      sps->vuiSeqParams.max_dec_frame_buffering = readUeV ("VUI max_dec_frame_buffering", s);
      }
    }
  }
//}}}
//{{{
static void readSpsFromStream (sDecoder* decoder, sDataPartition* dataPartition, sSps* sps, int naluLen) {

  sBitStream* s = dataPartition->s;
  sps->profileIdc = readUv (8, "SPS profileIdc", s);
  if ((sps->profileIdc != BASELINE) && (sps->profileIdc != MAIN) && (sps->profileIdc != EXTENDED) &&
      (sps->profileIdc != FREXT_HP) && (sps->profileIdc != FREXT_Hi10P) &&
      (sps->profileIdc != FREXT_Hi422) && (sps->profileIdc != FREXT_Hi444) &&
      (sps->profileIdc != FREXT_CAVLC444))
    printf ("IDC - invalid %d\n", sps->profileIdc);

  sps->constrained_set0_flag = readU1 ("SPS constrained_set0_flag", s);
  sps->constrained_set1_flag = readU1 ("SPS constrained_set1_flag", s);
  sps->constrained_set2_flag = readU1 ("SPS constrained_set2_flag", s);
  sps->constrainedSet3flag = readU1 ("SPS constrainedSet3flag", s);
  int reserved_zero = readUv (4, "SPS reserved_zero_4bits", s);

  sps->levelIdc = readUv (8, "SPS levelIdc", s);
  sps->id = readUeV ("SPS id", s);

  // Fidelity Range Extensions stuff
  sps->chromaFormatIdc = 1;
  sps->bit_depth_luma_minus8 = 0;
  sps->bit_depth_chroma_minus8 = 0;
  sps->useLosslessQpPrime = 0;
  sps->isSeperateColourPlane = 0;

  //{{{  read fidelity range
  if ((sps->profileIdc == FREXT_HP) ||
      (sps->profileIdc == FREXT_Hi10P) ||
      (sps->profileIdc == FREXT_Hi422) ||
      (sps->profileIdc == FREXT_Hi444) ||
      (sps->profileIdc == FREXT_CAVLC444)) {
    // read fidelity range
    sps->chromaFormatIdc = readUeV ("SPS chromaFormatIdc", s);
    if (sps->chromaFormatIdc == YUV444)
      sps->isSeperateColourPlane = readU1 ("SPS isSeperateColourPlane", s);
    sps->bit_depth_luma_minus8 = readUeV ("SPS bit_depth_luma_minus8", s);
    sps->bit_depth_chroma_minus8 = readUeV ("SPS bit_depth_chroma_minus8", s);
    if ((sps->bit_depth_luma_minus8+8 > sizeof(sPixel)*8) ||
        (sps->bit_depth_chroma_minus8+8> sizeof(sPixel)*8))
      error ("Source picture has higher bit depth than sPixel data type");

    sps->useLosslessQpPrime = readU1 ("SPS losslessQpPrimeYzero", s);

    sps->hasSeqScalingMatrix = readU1 ("SPS hasSeqScalingMatrix", s);
    if (sps->hasSeqScalingMatrix) {
      unsigned int n_ScalingList = (sps->chromaFormatIdc != YUV444) ? 8 : 12;
      for (unsigned int i = 0; i < n_ScalingList; i++) {
        sps->hasSeqScalingList[i] = readU1 ("SPS hasSeqScalingList", s);
        if (sps->hasSeqScalingList[i]) {
          if (i < 6)
            scalingList (sps->scalingList4x4[i], 16, &sps->useDefaultScalingMatrix4x4[i], s);
          else
            scalingList (sps->scalingList8x8[i-6], 64, &sps->useDefaultScalingMatrix8x8[i-6], s);
          }
        }
      }
    }
  //}}}
  sps->log2maxFrameNumMinus4 = readUeV ("SPS log2maxFrameNumMinus4", s);
  //{{{  read POC
  sps->pocType = readUeV ("SPS pocType", s);

  if (sps->pocType == 0)
    sps->log2maxPocLsbMinus4 = readUeV ("SPS log2maxPocLsbMinus4", s);

  else if (sps->pocType == 1) {
    sps->deltaPicOrderAlwaysZero = readU1 ("SPS deltaPicOrderAlwaysZero", s);
    sps->offsetNonRefPic = readSeV ("SPS offsetNonRefPic", s);
    sps->offsetTopBotField = readSeV ("SPS offsetTopBotField", s);
    sps->numRefFramesPocCycle = readUeV ("SPS numRefFramesPocCycle", s);
    for (unsigned int i = 0; i < sps->numRefFramesPocCycle; i++)
      sps->offsetForRefFrame[i] = readSeV ("SPS offsetRefFrame[i]", s);
    }
  //}}}

  sps->numRefFrames = readUeV ("SPS numRefFrames", s);
  sps->allowGapsFrameNum = readU1 ("SPS allowGapsFrameNum", s);

  sps->picWidthMbsMinus1 = readUeV ("SPS picWidthMbsMinus1", s);
  sps->picHeightMapUnitsMinus1 = readUeV ("SPS picHeightMapUnitsMinus1", s);

  sps->frameMbOnly = readU1 ("SPS frameMbOnly", s);
  if (!sps->frameMbOnly)
    sps->mbAffFlag = readU1 ("SPS mbAffFlag", s);

  sps->isDirect8x8inference = readU1 ("SPS isDirect8x8inference", s);

  //{{{  read crop
  sps->cropFlag = readU1 ("SPS cropFlag", s);
  if (sps->cropFlag) {
    sps->cropLeft = readUeV ("SPS cropLeft", s);
    sps->cropRight = readUeV ("SPS cropRight", s);
    sps->cropTop = readUeV ("SPS cropTop", s);
    sps->cropBot = readUeV ("SPS cropBot", s);
    }
  //}}}
  sps->hasVui = (Boolean)readU1 ("SPS hasVui", s);

  sps->vuiSeqParams.matrix_coefficients = 2;
  readVuiFromStream (dataPartition, sps);

  if (decoder->param.spsDebug) {
    //{{{  print debug
    printf ("SPS:%d:%d -> refFrames:%d pocType:%d mb:%dx%d",
            sps->id, naluLen,
            sps->numRefFrames, sps->pocType,
            sps->picWidthMbsMinus1, sps->picHeightMapUnitsMinus1);

    if (sps->cropFlag)
      printf (" crop:%d:%d:%d:%d",
              sps->cropLeft, sps->cropRight, sps->cropTop, sps->cropBot);

    printf ("%s%s\n", sps->frameMbOnly ?" frame":"", sps->mbAffFlag ? " mbAff":"");
    }
    //}}}

  sps->ok = TRUE;
  }
//}}}

//{{{
void readNaluSps (sDecoder* decoder, sNalu* nalu) {

  sDataPartition* dataPartition = allocDataPartitions (1);
  dataPartition->s->errorFlag = 0;
  dataPartition->s->readLen = dataPartition->s->bitStreamOffset = 0;
  memcpy (dataPartition->s->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
  dataPartition->s->codeLen = dataPartition->s->bitStreamLen = RBSPtoSODB (dataPartition->s->bitStreamBuffer, nalu->len-1);

  sSps sps = { 0 };
  readSpsFromStream (decoder, dataPartition, &sps, nalu->len);
  freeDataPartitions (dataPartition, 1);

  if (decoder->sps[sps.id].ok)
    if (!isEqualSps (&decoder->sps[sps.id], &sps))
      printf ("-----> readNaluSps new sps id:%d\n", sps.id);

  memcpy (&decoder->sps[sps.id], &sps, sizeof(sSps));
  }
//}}}
