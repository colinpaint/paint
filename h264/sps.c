//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
#include "sps.h"
#include "nalu.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "buffer.h"
#include "erc.h"
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
static void updateMaxValue (sFrameFormat* format) {

  format->max_value[0] = (1 << format->bitDepth[0]) - 1;
  format->max_value_sq[0] = format->max_value[0] * format->max_value[0];

  format->max_value[1] = (1 << format->bitDepth[1]) - 1;
  format->max_value_sq[1] = format->max_value[1] * format->max_value[1];

  format->max_value[2] = (1 << format->bitDepth[2]) - 1;
  format->max_value_sq[2] = format->max_value[2] * format->max_value[2];
  }
//}}}
//{{{
static void setCodingParam (sDecoder* decoder, sSps* sps) {

  // maximum vertical motion vector range in luma quarter pixel units
  decoder->coding.profileIdc = sps->profileIdc;
  decoder->coding.losslessQpPrimeFlag = sps->losslessQpPrimeFlag;
  if (sps->levelIdc <= 10)
    decoder->coding.maxVmvR = 64 * 4;
  else if (sps->levelIdc <= 20)
    decoder->coding.maxVmvR = 128 * 4;
  else if (sps->levelIdc <= 30)
    decoder->coding.maxVmvR = 256 * 4;
  else
    decoder->coding.maxVmvR = 512 * 4; // 512 pixels in quarter pixels

  // Fidelity Range Extensions stuff (part 1)
  decoder->coding.bitDepthChroma = 0;
  decoder->coding.widthCr = 0;
  decoder->coding.heightCr = 0;
  decoder->coding.bitDepthLuma = (short)(sps->bit_depth_luma_minus8 + 8);
  decoder->coding.bitDepthScale[0] = 1 << sps->bit_depth_luma_minus8;
  if (sps->chromaFormatIdc != YUV400) {
    decoder->coding.bitDepthChroma = (short) (sps->bit_depth_chroma_minus8 + 8);
    decoder->coding.bitDepthScale[1] = 1 << sps->bit_depth_chroma_minus8;
    }

  decoder->coding.maxFrameNum = 1 << (sps->log2maxFrameNumMinus4+4);
  decoder->coding.picWidthMbs = (sps->pic_width_in_mbs_minus1 +1);
  decoder->coding.picHeightMapUnits = (sps->pic_height_in_map_units_minus1 +1);
  decoder->coding.frameHeightMbs = (2 - sps->frameMbOnly) * decoder->coding.picHeightMapUnits;
  decoder->coding.frameSizeMbs = decoder->coding.picWidthMbs * decoder->coding.frameHeightMbs;

  decoder->coding.yuvFormat = sps->chromaFormatIdc;
  decoder->coding.isSeperateColourPlane = sps->isSeperateColourPlane;

  decoder->coding.width = decoder->coding.picWidthMbs * MB_BLOCK_SIZE;
  decoder->coding.height = decoder->coding.frameHeightMbs * MB_BLOCK_SIZE;

  decoder->coding.iLumaPadX = MCBUF_LUMA_PAD_X;
  decoder->coding.iLumaPadY = MCBUF_LUMA_PAD_Y;
  decoder->coding.iChromaPadX = MCBUF_CHROMA_PAD_X;
  decoder->coding.iChromaPadY = MCBUF_CHROMA_PAD_Y;

  if (sps->chromaFormatIdc == YUV420) {
    decoder->coding.widthCr  = (decoder->coding.width  >> 1);
    decoder->coding.heightCr = (decoder->coding.height >> 1);
    }
  else if (sps->chromaFormatIdc == YUV422) {
    decoder->coding.widthCr  = (decoder->coding.width >> 1);
    decoder->coding.heightCr = decoder->coding.height;
    decoder->coding.iChromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (sps->chromaFormatIdc == YUV444) {
    decoder->coding.widthCr = decoder->coding.width;
    decoder->coding.heightCr = decoder->coding.height;
    decoder->coding.iChromaPadX = decoder->coding.iLumaPadX;
    decoder->coding.iChromaPadY = decoder->coding.iLumaPadY;
    }

  //pel bitDepth init
  decoder->coding.bitDepthLumaQpScale = 6 * (decoder->coding.bitDepthLuma - 8);

  if (decoder->coding.bitDepthLuma > decoder->coding.bitDepthChroma || sps->chromaFormatIdc == YUV400)
    decoder->coding.picUnitBitSizeDisk = (decoder->coding.bitDepthLuma > 8)? 16:8;
  else
    decoder->coding.picUnitBitSizeDisk = (decoder->coding.bitDepthChroma > 8)? 16:8;
  decoder->coding.dcPredValueComp[0] = 1 << (decoder->coding.bitDepthLuma - 1);
  decoder->coding.maxPelValueComp[0] = (1 << decoder->coding.bitDepthLuma) - 1;
  decoder->coding.mbSize[0][0] = decoder->coding.mbSize[0][1] = MB_BLOCK_SIZE;

  if (sps->chromaFormatIdc != YUV400) {
    // for chrominance part
    decoder->coding.bitDepthChromaQpScale = 6 * (decoder->coding.bitDepthChroma - 8);
    decoder->coding.dcPredValueComp[1] = (1 << (decoder->coding.bitDepthChroma - 1));
    decoder->coding.dcPredValueComp[2] = decoder->coding.dcPredValueComp[1];
    decoder->coding.maxPelValueComp[1] = (1 << decoder->coding.bitDepthChroma) - 1;
    decoder->coding.maxPelValueComp[2] = (1 << decoder->coding.bitDepthChroma) - 1;
    decoder->coding.numBlock8x8uv = (1 << sps->chromaFormatIdc) & (~(0x1));
    decoder->coding.numUvBlocks = (decoder->coding.numBlock8x8uv >> 1);
    decoder->coding.numCdcCoeff = (decoder->coding.numBlock8x8uv << 1);
    decoder->coding.mbSize[1][0] = decoder->coding.mbSize[2][0] = decoder->coding.mbCrSizeX  = (sps->chromaFormatIdc==YUV420 || sps->chromaFormatIdc==YUV422)?  8 : 16;
    decoder->coding.mbSize[1][1] = decoder->coding.mbSize[2][1] = decoder->coding.mbCrSizeY  = (sps->chromaFormatIdc==YUV444 || sps->chromaFormatIdc==YUV422)? 16 :  8;

    decoder->coding.subpelX = decoder->coding.mbCrSizeX == 8 ? 7 : 3;
    decoder->coding.subpelY = decoder->coding.mbCrSizeY == 8 ? 7 : 3;
    decoder->coding.shiftpelX = decoder->coding.mbCrSizeX == 8 ? 3 : 2;
    decoder->coding.shiftpelY = decoder->coding.mbCrSizeY == 8 ? 3 : 2;
    decoder->coding.totalScale = decoder->coding.shiftpelX + decoder->coding.shiftpelY;
    }
  else {
    decoder->coding.bitDepthChromaQpScale = 0;
    decoder->coding.maxPelValueComp[1] = 0;
    decoder->coding.maxPelValueComp[2] = 0;
    decoder->coding.numBlock8x8uv = 0;
    decoder->coding.numUvBlocks = 0;
    decoder->coding.numCdcCoeff = 0;
    decoder->coding.mbSize[1][0] = decoder->coding.mbSize[2][0] = decoder->coding.mbCrSizeX  = 0;
    decoder->coding.mbSize[1][1] = decoder->coding.mbSize[2][1] = decoder->coding.mbCrSizeY  = 0;
    decoder->coding.subpelX = 0;
    decoder->coding.subpelY = 0;
    decoder->coding.shiftpelX = 0;
    decoder->coding.shiftpelY = 0;
    decoder->coding.totalScale = 0;
    }

  decoder->coding.mbCrSize = decoder->coding.mbCrSizeX * decoder->coding.mbCrSizeY;
  decoder->coding.mbSizeBlock[0][0] = decoder->coding.mbSizeBlock[0][1] = decoder->coding.mbSize[0][0] >> 2;
  decoder->coding.mbSizeBlock[1][0] = decoder->coding.mbSizeBlock[2][0] = decoder->coding.mbSize[1][0] >> 2;
  decoder->coding.mbSizeBlock[1][1] = decoder->coding.mbSizeBlock[2][1] = decoder->coding.mbSize[1][1] >> 2;

  decoder->coding.mbSizeShift[0][0] = decoder->coding.mbSizeShift[0][1] = ceilLog2sf (decoder->coding.mbSize[0][0]);
  decoder->coding.mbSizeShift[1][0] = decoder->coding.mbSizeShift[2][0] = ceilLog2sf (decoder->coding.mbSize[1][0]);
  decoder->coding.mbSizeShift[1][1] = decoder->coding.mbSizeShift[2][1] = ceilLog2sf (decoder->coding.mbSize[1][1]);
  }
//}}}
//{{{
static void setFormatInfo (sDecoder* decoder, sSps* sps, sFrameFormat* source, sFrameFormat* output) {

  static const int SubWidthC[4] = { 1, 2, 2, 1};
  static const int SubHeightC[4] = { 1, 2, 1, 1};

  // cropping for luma
  int cropLeft, cropRight, cropTop, cropBot;
  if (sps->cropFlag) {
    cropLeft = SubWidthC [sps->chromaFormatIdc] * sps->cropLeft;
    cropRight = SubWidthC [sps->chromaFormatIdc] * sps->cropRight;
    cropTop = SubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnly ) *  sps->cropTop;
    cropBot = SubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnly ) *  sps->cropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBot = 0;

  source->width[0] = decoder->width - cropLeft - cropRight;
  source->height[0] = decoder->height - cropTop - cropBot;

  // cropping for chroma
  if (sps->cropFlag) {
    cropLeft = sps->cropLeft;
    cropRight = sps->cropRight;
    cropTop = (2 - sps->frameMbOnly) * sps->cropTop;
    cropBot = (2 - sps->frameMbOnly) * sps->cropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBot = 0;

  source->width[1] = decoder->widthCr - cropLeft - cropRight;
  source->width[2] = source->width[1];
  source->height[1] = decoder->heightCr - cropTop - cropBot;
  source->height[2] = source->height[1];

  output->width[0] = decoder->width;
  source->width[1] = decoder->widthCr;
  source->width[2] = decoder->widthCr;
  output->height[0] = decoder->height;
  output->height[1] = decoder->heightCr;
  output->height[2] = decoder->heightCr;

  source->sizeCmp[0] = source->width[0] * source->height[0];
  source->sizeCmp[1] = source->width[1] * source->height[1];
  source->sizeCmp[2] = source->sizeCmp[1];
  source->size = source->sizeCmp[0] + source->sizeCmp[1] + source->sizeCmp[2];
  source->mbWidth = source->width[0]  / MB_BLOCK_SIZE;
  source->mbHeight = source->height[0] / MB_BLOCK_SIZE;

  // output size (excluding padding)
  output->sizeCmp[0] = output->width[0] * output->height[0];
  output->sizeCmp[1] = output->width[1] * output->height[1];
  output->sizeCmp[2] = output->sizeCmp[1];
  output->size = output->sizeCmp[0] + output->sizeCmp[1] + output->sizeCmp[2];
  output->mbWidth = output->width[0]  / MB_BLOCK_SIZE;
  output->mbHeight = output->height[0] / MB_BLOCK_SIZE;

  output->bitDepth[0] = source->bitDepth[0] = decoder->bitDepthLuma;
  output->bitDepth[1] = source->bitDepth[1] = decoder->bitDepthChroma;
  output->bitDepth[2] = source->bitDepth[2] = decoder->bitDepthChroma;
  output->picDiskUnitSize = (imax (output->bitDepth[0], output->bitDepth[1]) > 8) ? 16 : 8;

  output->frameRate = source->frameRate;
  output->colourModel = source->colourModel;
  output->yuvFormat = source->yuvFormat = sps->chromaFormatIdc;

  output->autoCropBot = cropBot;
  output->autoCropRight = cropRight;
  output->autoCropBotCr = (cropBot * decoder->mbCrSizeY) / MB_BLOCK_SIZE;
  output->autoCropRightCr = (cropRight * decoder->mbCrSizeX) / MB_BLOCK_SIZE;

  source->autoCropBot = output->autoCropBot;
  source->autoCropRight = output->autoCropRight;
  source->autoCropBotCr = output->autoCropBotCr;
  source->autoCropRightCr = output->autoCropRightCr;

  updateMaxValue (source);
  updateMaxValue (output);

  if (!decoder->gotPps) {
    //{{{  print profile info
    decoder->gotPps = 1;
    printf ("-> profile:%d %dx%d %dx%d ",
            sps->profileIdc, source->width[0], source->height[0], decoder->width, decoder->height);

    if (decoder->coding.yuvFormat == YUV400)
      printf ("4:0:0");
    else if (decoder->coding.yuvFormat == YUV420)
      printf ("4:2:0");
    else if (decoder->coding.yuvFormat == YUV422)
      printf ("4:2:2");
    else
      printf ("4:4:4");

    printf (" %d:%d:%d\n", source->bitDepth[0], source->bitDepth[1], source->bitDepth[2]);
    }
    //}}}
  }
//}}}

//{{{
static int isEqualSps (sSps* sps1, sSps* sps2) {

  int equal = 1;
  if ((!sps1->valid) || (!sps2->valid))
    return 0;

  equal &= (sps1->spsId == sps2->spsId);
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
  else if( sps1->pocType == 1) {
    equal &= (sps1->deltaPicOrderAlwaysZero == sps2->deltaPicOrderAlwaysZero);
    equal &= (sps1->offsetNonRefPic == sps2->offsetNonRefPic);
    equal &= (sps1->offsetTopBotField == sps2->offsetTopBotField);
    equal &= (sps1->numRefFramesPocCycle == sps2->numRefFramesPocCycle);
    if (!equal)
      return equal;

    for (unsigned i = 0 ; i < sps1->numRefFramesPocCycle ;i ++)
      equal &= (sps1->offset_for_ref_frame[i] == sps2->offset_for_ref_frame[i]);
    }

  equal &= (sps1->numRefFrames == sps2->numRefFrames);
  equal &= (sps1->gaps_in_frame_num_value_allowed_flag == sps2->gaps_in_frame_num_value_allowed_flag);
  equal &= (sps1->pic_width_in_mbs_minus1 == sps2->pic_width_in_mbs_minus1);
  equal &= (sps1->pic_height_in_map_units_minus1 == sps2->pic_height_in_map_units_minus1);
  equal &= (sps1->frameMbOnly == sps2->frameMbOnly);
  if (!equal) return
    equal;

  if (!sps1->frameMbOnly)
    equal &= (sps1->mbAffFlag == sps2->mbAffFlag);
  equal &= (sps1->direct_8x8_inference_flag == sps2->direct_8x8_inference_flag);
  equal &= (sps1->cropFlag == sps2->cropFlag);
  if (!equal)
    return equal;

  if (sps1->cropFlag) {
    equal &= (sps1->cropLeft == sps2->cropLeft);
    equal &= (sps1->cropRight == sps2->cropRight);
    equal &= (sps1->cropTop == sps2->cropTop);
    equal &= (sps1->cropBot == sps2->cropBot);
    }
  equal &= (sps1->vui_parameters_present_flag == sps2->vui_parameters_present_flag);

  return equal;
  }
//}}}
//{{{
static void initVui (sSps* sps) {
  sps->vui_seq_parameters.matrix_coefficients = 2;
  }
//}}}
//{{{
static void readHrd (sDataPartition* dataPartition, sHRD* hrd) {

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
static void readVui (sDataPartition* dataPartition, sSps* sps) {

  sBitStream* s = dataPartition->s;
  if (sps->vui_parameters_present_flag) {
    sps->vui_seq_parameters.aspect_ratio_info_present_flag = readU1 ("VUI aspect_ratio_info_present_flag", s);
    if (sps->vui_seq_parameters.aspect_ratio_info_present_flag) {
      sps->vui_seq_parameters.aspect_ratio_idc = readUv ( 8, "VUI aspect_ratio_idc", s);
      if (255 == sps->vui_seq_parameters.aspect_ratio_idc) {
        sps->vui_seq_parameters.sar_width = (unsigned short)readUv (16, "VUI sar_width", s);
        sps->vui_seq_parameters.sar_height = (unsigned short)readUv (16, "VUI sar_height", s);
        }
      }

    sps->vui_seq_parameters.overscan_info_present_flag = readU1 ("VUI overscan_info_present_flag", s);
    if (sps->vui_seq_parameters.overscan_info_present_flag)
      sps->vui_seq_parameters.overscan_appropriate_flag = readU1 ("VUI overscan_appropriate_flag", s);

    sps->vui_seq_parameters.video_signal_type_present_flag = readU1 ("VUI video_signal_type_present_flag", s);
    if (sps->vui_seq_parameters.video_signal_type_present_flag) {
      sps->vui_seq_parameters.video_format = readUv (3, "VUI video_format", s);
      sps->vui_seq_parameters.video_full_range_flag = readU1 ("VUI video_full_range_flag", s);
      sps->vui_seq_parameters.colour_description_present_flag = readU1 ("VUI color_description_present_flag", s);
      if (sps->vui_seq_parameters.colour_description_present_flag) {
        sps->vui_seq_parameters.colour_primaries = readUv (8, "VUI colour_primaries", s);
        sps->vui_seq_parameters.transfer_characteristics = readUv (8, "VUI transfer_characteristics", s);
        sps->vui_seq_parameters.matrix_coefficients = readUv (8, "VUI matrix_coefficients", s);
        }
      }

    sps->vui_seq_parameters.chroma_location_info_present_flag = readU1 ("VUI chroma_loc_info_present_flag", s);
    if (sps->vui_seq_parameters.chroma_location_info_present_flag) {
      sps->vui_seq_parameters.chroma_sample_loc_type_top_field = readUeV ("VUI chroma_sample_loc_type_top_field", s);
      sps->vui_seq_parameters.chroma_sample_loc_type_bottom_field = readUeV ("VUI chroma_sample_loc_type_bottom_field", s);
      }

    sps->vui_seq_parameters.timing_info_present_flag = readU1 ("VUI timing_info_present_flag", s);
    if (sps->vui_seq_parameters.timing_info_present_flag) {
      sps->vui_seq_parameters.num_units_in_tick = readUv (32, "VUI num_units_in_tick", s);
      sps->vui_seq_parameters.time_scale = readUv (32,"VUI time_scale", s);
      sps->vui_seq_parameters.fixed_frame_rate_flag = readU1 ("VUI fixed_frame_rate_flag", s);
      }

    sps->vui_seq_parameters.nal_hrd_parameters_present_flag = readU1 ("VUI nal_hrd_parameters_present_flag", s);
    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
      readHrd (dataPartition, &(sps->vui_seq_parameters.nal_hrd_parameters));

    sps->vui_seq_parameters.vcl_hrd_parameters_present_flag = readU1 ("VUI vcl_hrd_parameters_present_flag", s);

    if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
      readHrd (dataPartition, &(sps->vui_seq_parameters.vcl_hrd_parameters));

    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag ||
        sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
      sps->vui_seq_parameters.low_delay_hrd_flag = readU1 ("VUI low_delay_hrd_flag", s);

    sps->vui_seq_parameters.pic_struct_present_flag = readU1 ("VUI pic_struct_present_flag   ", s);
    sps->vui_seq_parameters.bitstream_restriction_flag = readU1 ("VUI bitstream_restriction_flag", s);

    if (sps->vui_seq_parameters.bitstream_restriction_flag) {
      sps->vui_seq_parameters.motion_vectors_over_pic_boundaries_flag = readU1 ("VUI motion_vectors_over_pic_boundaries_flag", s);
      sps->vui_seq_parameters.max_bytes_per_pic_denom = readUeV ("VUI max_bytes_per_pic_denom", s);
      sps->vui_seq_parameters.max_bits_per_mb_denom = readUeV ("VUI max_bits_per_mb_denom", s);
      sps->vui_seq_parameters.log2_max_mv_length_horizontal = readUeV ("VUI log2_max_mv_length_horizontal", s);
      sps->vui_seq_parameters.log2_max_mv_length_vertical = readUeV ("VUI log2_max_mv_length_vertical", s);
      sps->vui_seq_parameters.num_reorder_frames = readUeV ("VUI num_reorder_frames", s);
      sps->vui_seq_parameters.max_dec_frame_buffering = readUeV ("VUI max_dec_frame_buffering", s);
      }
    }
  }
//}}}
//{{{
static void readSps (sDecoder* decoder, sDataPartition* dataPartition, sSps* sps, int naluLen) {

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
  sps->spsId = readUeV ("SPS spsId", s);

  // Fidelity Range Extensions stuff
  sps->chromaFormatIdc = 1;
  sps->bit_depth_luma_minus8 = 0;
  sps->bit_depth_chroma_minus8 = 0;
  sps->losslessQpPrimeFlag = 0;
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

    sps->losslessQpPrimeFlag = readU1 ("SPS lossless_qpprime_y_zero_flag", s);

    sps->seq_scaling_matrix_present_flag = readU1 ("SPS seq_scaling_matrix_present_flag", s);
    if (sps->seq_scaling_matrix_present_flag) {
      unsigned int n_ScalingList = (sps->chromaFormatIdc != YUV444) ? 8 : 12;
      for (unsigned int i = 0; i < n_ScalingList; i++) {
        sps->seq_scaling_list_present_flag[i] = readU1 ("SPS seq_scaling_list_present_flag", s);
        if (sps->seq_scaling_list_present_flag[i]) {
          if (i < 6)
            scalingList (sps->scalingList4x4[i], 16, &sps->useDefaultScalingMatrix4x4Flag[i], s);
          else
            scalingList (sps->scalingList8x8[i-6], 64, &sps->useDefaultScalingMatrix8x8Flag[i-6], s);
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
      sps->offset_for_ref_frame[i] = readSeV ("SPS offset_for_ref_frame[i]", s);
    }
  //}}}

  sps->numRefFrames = readUeV ("SPS numRefFrames", s);
  sps->gaps_in_frame_num_value_allowed_flag = readU1 ("SPS gaps_in_frame_num_value_allowed_flag", s);

  sps->pic_width_in_mbs_minus1 = readUeV ("SPS pic_width_in_mbs_minus1", s);
  sps->pic_height_in_map_units_minus1 = readUeV ("SPS pic_height_in_map_units_minus1", s);

  sps->frameMbOnly = readU1 ("SPS frameMbOnly", s);
  if (!sps->frameMbOnly)
    sps->mbAffFlag = readU1 ("SPS mbAffFlag", s);

  sps->direct_8x8_inference_flag = readU1 ("SPS direct_8x8_inference_flag", s);

  //{{{  read crop
  sps->cropFlag = readU1 ("SPS cropFlag", s);
  if (sps->cropFlag) {
    sps->cropLeft = readUeV ("SPS cropLeft", s);
    sps->cropRight = readUeV ("SPS cropRight", s);
    sps->cropTop = readUeV ("SPS cropTop", s);
    sps->cropBot = readUeV ("SPS cropBot", s);
    }
  //}}}
  sps->vui_parameters_present_flag = (Boolean)readU1 ("SPS vui_parameters_present_flag", s);

  initVui (sps);
  readVui (dataPartition, sps);

  if (decoder->param.spsDebug) {
    //{{{  print debug
    printf ("SPS:%d:%d -> refFrames:%d pocType:%d mb:%dx%d",
            sps->spsId, naluLen,
            sps->numRefFrames, sps->pocType,
            sps->pic_width_in_mbs_minus1, sps->pic_height_in_map_units_minus1);

    if (sps->cropFlag)
      printf (" crop:%d:%d:%d:%d",
              sps->cropLeft, sps->cropRight, sps->cropTop, sps->cropBot);

    printf ("%s%s\n", sps->frameMbOnly ?" frame":"", sps->mbAffFlag ? " mbAff":"");
    }
    //}}}

  sps->valid = TRUE;
  }
//}}}

//{{{
void readSpsFromNalu (sDecoder* decoder, sNalu* nalu) {

  sDataPartition* dataPartition = allocDataPartitions (1);
  dataPartition->s->errorFlag = 0;
  dataPartition->s->readLen = dataPartition->s->bitStreamOffset = 0;
  memcpy (dataPartition->s->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
  dataPartition->s->codeLen = dataPartition->s->bitStreamLen = RBSPtoSODB (dataPartition->s->bitStreamBuffer, nalu->len-1);

  sSps* sps = calloc (1, sizeof (sSps));
  readSps (decoder, dataPartition, sps, nalu->len);
  if (sps->valid) {
    if (decoder->activeSps)
      if (sps->spsId == decoder->activeSps->spsId)
        if (!isEqualSps (sps, decoder->activeSps))   {
          if (decoder->picture)
            endDecodeFrame (decoder);
          decoder->activeSps = NULL;
          }

    memcpy (&decoder->sps[sps->spsId], sps, sizeof(sSps));
    decoder->coding.isSeperateColourPlane = sps->isSeperateColourPlane;
    }

  freeDataPartitions (dataPartition, 1);
  free (sps);
  }
//}}}
//{{{
void useSps (sDecoder* decoder, sSps* sps) {

  if (decoder->activeSps != sps) {
    if (decoder->picture) // slice loss
      endDecodeFrame (decoder);
    decoder->activeSps = sps;

    if (isBLprofile (sps->profileIdc) && !decoder->dpb->initDone)
      setCodingParam (decoder, sps);
    setCoding (decoder);

    initGlobalBuffers (decoder);
    if (!decoder->noOutputPriorPicFlag)
      flushDpb (decoder->dpb);

    initDpb (decoder, decoder->dpb, 0);

    // enable error conceal
    ercInit (decoder, decoder->width, decoder->height, 1);
    if (decoder->picture) {
      ercReset (decoder->ercErrorVar, decoder->picSizeInMbs, decoder->picSizeInMbs, decoder->picture->sizeX);
      decoder->ercMvPerMb = 0;
      }

    sprintf (decoder->debug.spsStr, "%s", sps->frameMbOnly ? (sps->mbAffFlag ? " mbAff":" frame") : "");
    }

  setFormatInfo (decoder, sps, &decoder->param.source, &decoder->param.output);
  }
//}}}
