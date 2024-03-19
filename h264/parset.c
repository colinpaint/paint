//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
#include "parset.h"
#include "nalu.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "buffer.h"
#include "erc.h"
//}}}

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
static void setCodingParam (sDecoder* decoder, sSPS* sps) {

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
  decoder->coding.bitdepthChroma = 0;
  decoder->coding.widthCr = 0;
  decoder->coding.heightCr = 0;
  decoder->coding.bitdepthLuma = (short)(sps->bit_depth_luma_minus8 + 8);
  decoder->coding.bitdepthScale[0] = 1 << sps->bit_depth_luma_minus8;
  if (sps->chromaFormatIdc != YUV400) {
    decoder->coding.bitdepthChroma = (short) (sps->bit_depth_chroma_minus8 + 8);
    decoder->coding.bitdepthScale[1] = 1 << sps->bit_depth_chroma_minus8;
    }

  decoder->coding.maxFrameNum = 1 << (sps->log2_max_frame_num_minus4+4);
  decoder->coding.picWidthMbs = (sps->pic_width_in_mbs_minus1 +1);
  decoder->coding.picHeightMapUnits = (sps->pic_height_in_map_units_minus1 +1);
  decoder->coding.frameHeightMbs = (2 - sps->frameMbOnlyFlag) * decoder->coding.picHeightMapUnits;
  decoder->coding.frameSizeMbs = decoder->coding.picWidthMbs * decoder->coding.frameHeightMbs;

  decoder->coding.yuvFormat = sps->chromaFormatIdc;
  decoder->coding.sepColourPlaneFlag = sps->sepColourPlaneFlag;

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

  //pel bitdepth init
  decoder->coding.bitdepthLumaQpScale = 6 * (decoder->coding.bitdepthLuma - 8);

  if (decoder->coding.bitdepthLuma > decoder->coding.bitdepthChroma || sps->chromaFormatIdc == YUV400)
    decoder->coding.picUnitBitSizeDisk = (decoder->coding.bitdepthLuma > 8)? 16:8;
  else
    decoder->coding.picUnitBitSizeDisk = (decoder->coding.bitdepthChroma > 8)? 16:8;
  decoder->coding.dcPredValueComp[0] = 1 << (decoder->coding.bitdepthLuma - 1);
  decoder->coding.maxPelValueComp[0] = (1 << decoder->coding.bitdepthLuma) - 1;
  decoder->coding.mbSize[0][0] = decoder->coding.mbSize[0][1] = MB_BLOCK_SIZE;

  if (sps->chromaFormatIdc != YUV400) {
    // for chrominance part
    decoder->coding.bitdepthChromaQpScale = 6 * (decoder->coding.bitdepthChroma - 8);
    decoder->coding.dcPredValueComp[1] = (1 << (decoder->coding.bitdepthChroma - 1));
    decoder->coding.dcPredValueComp[2] = decoder->coding.dcPredValueComp[1];
    decoder->coding.maxPelValueComp[1] = (1 << decoder->coding.bitdepthChroma) - 1;
    decoder->coding.maxPelValueComp[2] = (1 << decoder->coding.bitdepthChroma) - 1;
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
    decoder->coding.bitdepthChromaQpScale = 0;
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
static void setFormatInfo (sDecoder* decoder, sSPS* sps, sFrameFormat* source, sFrameFormat* output) {

  static const int SubWidthC[4] = { 1, 2, 2, 1};
  static const int SubHeightC[4] = { 1, 2, 1, 1};

  // cropping for luma
  int cropLeft, cropRight, cropTop, cropBot;
  if (sps->frameCropFlag) {
    cropLeft = SubWidthC [sps->chromaFormatIdc] * sps->frameCropLeft;
    cropRight = SubWidthC [sps->chromaFormatIdc] * sps->frameCropRight;
    cropTop = SubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnlyFlag ) *  sps->frameCropTop;
    cropBot = SubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnlyFlag ) *  sps->frameCropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBot = 0;

  source->width[0] = decoder->width - cropLeft - cropRight;
  source->height[0] = decoder->height - cropTop - cropBot;

  // cropping for chroma
  if (sps->frameCropFlag) {
    cropLeft = sps->frameCropLeft;
    cropRight = sps->frameCropRight;
    cropTop = (2 - sps->frameMbOnlyFlag) * sps->frameCropTop;
    cropBot = (2 - sps->frameMbOnlyFlag) * sps->frameCropBot;
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

  output->bitDepth[0] = source->bitDepth[0] = decoder->bitdepthLuma;
  output->bitDepth[1] = source->bitDepth[1] = decoder->bitdepthChroma;
  output->bitDepth[2] = source->bitDepth[2] = decoder->bitdepthChroma;
  output->picDiskUnitSize = (imax(output->bitDepth[0], output->bitDepth[1]) > 8) ? 16 : 8;
  output->pic_unit_size_shift3 = output->picDiskUnitSize >> 3;

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

  if (!decoder->gotSPS) {
    //{{{  print profile info
    decoder->gotSPS = 1;
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
// syntax for scaling list matrix values
static void scalingList (int* scalingList, int scalingListSize, Boolean* useDefaultScalingMatrix, sBitStream* s) {

  int lastScale = 8;
  int nextScale = 8;
  for (int j = 0; j < scalingListSize; j++) {
    int scanj = (scalingListSize == 16) ? ZZ_SCAN[j]:ZZ_SCAN8[j];
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

// SPS
//{{{
static sSPS* allocSPS() {

   sSPS* p = calloc (1, sizeof (sSPS));
   if (!p)
     no_mem_exit ("allocSPS");

   return p;
   }
//}}}
//{{{
static void freeSPS (sSPS* sps) {

  assert (sps);
  free (sps);
  }
//}}}
//{{{
static int isEqualSPS (sSPS* sps1, sSPS* sps2) {

  int equal = 1;
  if ((!sps1->valid) || (!sps2->valid))
    return 0;

  equal &= (sps1->spsId == sps2->spsId);
  equal &= (sps1->levelIdc == sps2->levelIdc);
  equal &= (sps1->profileIdc == sps2->profileIdc);
  equal &= (sps1->constrained_set0_flag == sps2->constrained_set0_flag);
  equal &= (sps1->constrained_set1_flag == sps2->constrained_set1_flag);
  equal &= (sps1->constrained_set2_flag == sps2->constrained_set2_flag);

  equal &= (sps1->log2_max_frame_num_minus4 == sps2->log2_max_frame_num_minus4);
  equal &= (sps1->pocType == sps2->pocType);
  if (!equal)
    return equal;

  if (sps1->pocType == 0)
    equal &= (sps1->log2_max_pic_order_cnt_lsb_minus4 == sps2->log2_max_pic_order_cnt_lsb_minus4);
  else if( sps1->pocType == 1) {
    equal &= (sps1->delta_pic_order_always_zero_flag == sps2->delta_pic_order_always_zero_flag);
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
  equal &= (sps1->frameMbOnlyFlag == sps2->frameMbOnlyFlag);
  if (!equal) return
    equal;

  if (!sps1->frameMbOnlyFlag)
    equal &= (sps1->mb_adaptive_frame_field_flag == sps2->mb_adaptive_frame_field_flag);
  equal &= (sps1->direct_8x8_inference_flag == sps2->direct_8x8_inference_flag);
  equal &= (sps1->frameCropFlag == sps2->frameCropFlag);
  if (!equal)
    return equal;

  if (sps1->frameCropFlag) {
    equal &= (sps1->frameCropLeft == sps2->frameCropLeft);
    equal &= (sps1->frameCropRight == sps2->frameCropRight);
    equal &= (sps1->frameCropTop == sps2->frameCropTop);
    equal &= (sps1->frameCropBot == sps2->frameCropBot);
    }
  equal &= (sps1->vui_parameters_present_flag == sps2->vui_parameters_present_flag);

  return equal;
  }
//}}}
//{{{
static void initVUI (sSPS* sps) {
  sps->vui_seq_parameters.matrix_coefficients = 2;
  }
//}}}
//{{{
static void readHRD (sDataPartition* dataPartition, sHRD* hrd) {

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
static void readVUI (sDataPartition* dataPartition, sSPS* sps) {

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
      readHRD (dataPartition, &(sps->vui_seq_parameters.nal_hrd_parameters));

    sps->vui_seq_parameters.vcl_hrd_parameters_present_flag = readU1 ("VUI vcl_hrd_parameters_present_flag", s);

    if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
      readHRD (dataPartition, &(sps->vui_seq_parameters.vcl_hrd_parameters));

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
static void readSPS (sDecoder* decoder, sDataPartition* dataPartition, sSPS* sps) {

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
  sps->constrained_set3_flag = readU1 ("SPS constrained_set3_flag", s);

  int reserved_zero = readUv (4, "SPS reserved_zero_4bits", s);
  if (reserved_zero)
    printf ("reserved_zero flag not 0\n");

  sps->levelIdc = readUv (8, "SPS levelIdc", s);
  sps->spsId = readUeV ("SPS spsId", s);

  // Fidelity Range Extensions stuff
  sps->chromaFormatIdc = 1;
  sps->bit_depth_luma_minus8 = 0;
  sps->bit_depth_chroma_minus8 = 0;
  sps->losslessQpPrimeFlag = 0;
  sps->sepColourPlaneFlag = 0;

  //{{{  read fidelity range
  if ((sps->profileIdc == FREXT_HP) ||
      (sps->profileIdc == FREXT_Hi10P) ||
      (sps->profileIdc == FREXT_Hi422) ||
      (sps->profileIdc == FREXT_Hi444) ||
      (sps->profileIdc == FREXT_CAVLC444)) {
    // read fidelity range
    sps->chromaFormatIdc = readUeV ("SPS chromaFormatIdc", s);
    if (sps->chromaFormatIdc == YUV444)
      sps->sepColourPlaneFlag = readU1 ("SPS sepColourPlaneFlag", s);
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
  sps->log2_max_frame_num_minus4 = readUeV ("SPS log2_max_frame_num_minus4", s);
  //{{{  read POC
  sps->pocType = readUeV ("SPS pocType", s);
  if (!sps->pocType)
    sps->log2_max_pic_order_cnt_lsb_minus4 = readUeV ("SPS log2_max_pic_order_cnt_lsb_minus4", s);
  else if (sps->pocType == 1) {
    sps->delta_pic_order_always_zero_flag = readU1 ("SPS delta_pic_order_always_zero_flag", s);
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

  sps->frameMbOnlyFlag = readU1 ("SPS frameMbOnlyFlag", s);
  if (!sps->frameMbOnlyFlag)
    sps->mb_adaptive_frame_field_flag = readU1 ("SPS mb_adaptive_frame_field_flag", s);

  sps->direct_8x8_inference_flag = readU1 ("SPS direct_8x8_inference_flag", s);

  //{{{  read crop
  sps->frameCropFlag = readU1 ("SPS frameCropFlag", s);
  if (sps->frameCropFlag) {
    sps->frameCropLeft = readUeV ("SPS frameCropLeft", s);
    sps->frameCropRight = readUeV ("SPS frameCropRight", s);
    sps->frameCropTop = readUeV ("SPS frameCropTop", s);
    sps->frameCropBot = readUeV ("SPS frameCropBot", s);
    }
  //}}}
  sps->vui_parameters_present_flag = (Boolean)readU1 ("SPS vui_parameters_present_flag", s);

  initVUI (sps);
  readVUI (dataPartition, sps);

  if (decoder->param.spsDebug) {
    printf ("-> id:%d refFrames:%d pocType:%d mbs:%dx%d",
            sps->spsId, sps->numRefFrames,  sps->pocType,
            sps->pic_width_in_mbs_minus1, sps->pic_height_in_map_units_minus1);
    if (sps->frameMbOnlyFlag)
      printf (" frame");
    if (sps->frameCropFlag)
      printf (" crop:%d:%d:%d:%d",
              sps->frameCropLeft, sps->frameCropRight, sps->frameCropTop, sps->frameCropBot);
    printf ("\n");
    }

  sps->valid = TRUE;
  }
//}}}
//{{{
static void setSPSbyId (sDecoder* decoder, int id, sSPS* sps) {

  assert (sps->valid == TRUE);
  memcpy (&decoder->sps[id], sps, sizeof(sSPS));
  }
//}}}

//{{{
void readNaluSPS (sDecoder* decoder, sNalu* nalu) {

  sDataPartition* dataPartition = allocDataPartitions (1);
  dataPartition->s->errorFlag = 0;
  dataPartition->s->readLen = dataPartition->s->bitStreamOffset = 0;
  memcpy (dataPartition->s->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
  dataPartition->s->codeLen = dataPartition->s->bitStreamLen = RBSPtoSODB (dataPartition->s->bitStreamBuffer, nalu->len-1);

  sSPS* sps = allocSPS();
  readSPS (decoder, dataPartition, sps);
  if (sps->valid) {
    if (decoder->activeSPS) {
      if (sps->spsId == decoder->activeSPS->spsId) {
        if (!isEqualSPS (sps, decoder->activeSPS))   {
          if (decoder->picture)
            endDecodeFrame (decoder);
          decoder->activeSPS = NULL;
          }
        }
      }

    setSPSbyId (decoder, sps->spsId, sps);

    decoder->coding.sepColourPlaneFlag = sps->sepColourPlaneFlag;
    }

  freeDataPartitions (dataPartition, 1);
  freeSPS (sps);
  }
//}}}
//{{{
void activateSPS (sDecoder* decoder, sSPS* sps) {

  if (decoder->activeSPS != sps) {
    if (decoder->picture) // slice loss
      endDecodeFrame (decoder);
    decoder->activeSPS = sps;

    if (isBLprofile (sps->profileIdc) && !decoder->dpb->initDone)
      setCodingParam (decoder, sps);
    setGlobalCodingProgram (decoder);

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
    }

  setFormatInfo (decoder, sps, &decoder->param.source, &decoder->param.output);
  }
//}}}

// PPS
//{{{
static int isEqualPPS (sPPS* pps1, sPPS* pps2) {

  if (!pps1->valid || !pps2->valid)
    return 0;

  int equal = 1;
  equal &= (pps1->ppsId == pps2->ppsId);
  equal &= (pps1->spsId == pps2->spsId);
  equal &= (pps1->entropyCodingMode == pps2->entropyCodingMode);
  equal &= (pps1->botFieldPicOrderFramePresent == pps2->botFieldPicOrderFramePresent);
  equal &= (pps1->numSliceGroupsMinus1 == pps2->numSliceGroupsMinus1);
  if (!equal)
    return equal;

  if (pps1->numSliceGroupsMinus1 > 0) {
    equal &= (pps1->sliceGroupMapType == pps2->sliceGroupMapType);
    if (!equal)
      return equal;
    if (pps1->sliceGroupMapType == 0) {
      for (unsigned i = 0; i <= pps1->numSliceGroupsMinus1; i++)
        equal &= (pps1->runLengthMinus1[i] == pps2->runLengthMinus1[i]);
      }
    else if (pps1->sliceGroupMapType == 2) {
      for (unsigned i = 0; i < pps1->numSliceGroupsMinus1; i++) {
        equal &= (pps1->topLeft[i] == pps2->topLeft[i]);
        equal &= (pps1->botRight[i] == pps2->botRight[i]);
        }
      }
    else if (pps1->sliceGroupMapType == 3 ||
             pps1->sliceGroupMapType == 4 ||
             pps1->sliceGroupMapType == 5) {
      equal &= (pps1->sliceGroupChangeDirectionFlag == pps2->sliceGroupChangeDirectionFlag);
      equal &= (pps1->sliceGroupChangeRateMius1 == pps2->sliceGroupChangeRateMius1);
      }
    else if (pps1->sliceGroupMapType == 6) {
      equal &= (pps1->picSizeMapUnitsMinus1 == pps2->picSizeMapUnitsMinus1);
      if (!equal)
        return equal;
      for (unsigned i = 0; i <= pps1->picSizeMapUnitsMinus1; i++)
        equal &= (pps1->sliceGroupId[i] == pps2->sliceGroupId[i]);
      }
    }

  equal &= (pps1->weightedPredFlag == pps2->weightedPredFlag);
  equal &= (pps1->picInitQpMinus26 == pps2->picInitQpMinus26);
  equal &= (pps1->picInitQsMinus26 == pps2->picInitQsMinus26);
  equal &= (pps1->weightedBiPredIdc == pps2->weightedBiPredIdc);
  equal &= (pps1->chromaQpIndexOffset == pps2->chromaQpIndexOffset);
  equal &= (pps1->constrainedIntraPredFlag == pps2->constrainedIntraPredFlag);
  equal &= (pps1->redundantPicCountPresent == pps2->redundantPicCountPresent);
  equal &= (pps1->deblockFilterControlPresent == pps2->deblockFilterControlPresent);
  equal &= (pps1->numRefIndexL0defaultActiveMinus1 == pps2->numRefIndexL0defaultActiveMinus1);
  equal &= (pps1->numRefIndexL1defaultActiveMinus1 == pps2->numRefIndexL1defaultActiveMinus1);
  if (!equal)
    return equal;

  equal &= (pps1->transform8x8modeFlag == pps2->transform8x8modeFlag);
  equal &= (pps1->picScalingMatrixPresentFlag == pps2->picScalingMatrixPresentFlag);
  if (pps1->picScalingMatrixPresentFlag) {
    for (unsigned i = 0; i < (6 + ((unsigned)pps1->transform8x8modeFlag << 1)); i++) {
      equal &= (pps1->picScalingListPresentFlag[i] == pps2->picScalingListPresentFlag[i]);
      if (pps1->picScalingListPresentFlag[i]) {
        if (i < 6)
          for (int j = 0; j < 16; j++)
            equal &= (pps1->scalingList4x4[i][j] == pps2->scalingList4x4[i][j]);
        else
          for (int j = 0; j < 64; j++)
            equal &= (pps1->scalingList8x8[i-6][j] == pps2->scalingList8x8[i-6][j]);
        }
      }
    }
  equal &= (pps1->secondChromaQpIndexOffset == pps2->secondChromaQpIndexOffset);

  return equal;
  }
//}}}
//{{{
static void readPPS (sDecoder* decoder, sDataPartition* dataPartition, sPPS* pps) {

  unsigned n_ScalingList;
  int chromaFormatIdc;

  sBitStream* s = dataPartition->s;
  pps->ppsId = readUeV ("PPS ppsId", s);
  pps->spsId = readUeV ("PPS spsId", s);
  pps->entropyCodingMode = readU1 ("PPS entropyCodingMode", s);

  // Note: as per JVT-F078 the following bit is unconditional.  If F078 is not accepted, then
  // one has to fetch the correct SPS to check whether the bit is present (hopefully there is
  // no consistency problem :-(
  // The current encoder code handles this in the same way.  When you change this, don't forget
  // the encoder!  StW, 12/8/02
  pps->botFieldPicOrderFramePresent = readU1 ("PPS botFieldPicOrderFramePresent", s);
  pps->numSliceGroupsMinus1 = readUeV ("PPS numSliceGroupsMinus1", s);

  if (pps->numSliceGroupsMinus1 > 0) {
    //{{{  FMO
    pps->sliceGroupMapType = readUeV ("PPS sliceGroupMapType", s);

    switch (pps->sliceGroupMapType) {
      case 0: {
        for (unsigned i = 0; i <= pps->numSliceGroupsMinus1; i++)
          pps->runLengthMinus1 [i] = readUeV ("PPS runLengthMinus1 [i]", s);
        break;
        }

      case 2: {
        for (unsigned i = 0; i < pps->numSliceGroupsMinus1; i++) {
          //! JVT-F078: avoid reference of SPS by using ue(v) instead of u(v)
          pps->topLeft [i] = readUeV ("PPS topLeft [i]", s);
          pps->botRight [i] = readUeV ("PPS botRight [i]", s);
          }
        break;
        }

      case 3:
      case 4:
      case 5:
        pps->sliceGroupChangeDirectionFlag = readU1 ("PPS sliceGroupChangeDirectionFlag", s);
        pps->sliceGroupChangeRateMius1 = readUeV ("PPS sliceGroupChangeRateMius1", s);
        break;

      case 6: {
        int NumberBitsPerSliceGroupId;
        if (pps->numSliceGroupsMinus1+1 >4)
          NumberBitsPerSliceGroupId = 3;
        else if (pps->numSliceGroupsMinus1+1 > 2)
          NumberBitsPerSliceGroupId = 2;
        else
          NumberBitsPerSliceGroupId = 1;

        pps->picSizeMapUnitsMinus1 = readUeV ("PPS picSizeMapUnitsMinus1", s);
        if ((pps->sliceGroupId = calloc (pps->picSizeMapUnitsMinus1+1, 1)) == NULL)
          no_mem_exit ("readPPS sliceGroupId");
        for (unsigned i = 0; i <= pps->picSizeMapUnitsMinus1; i++)
          pps->sliceGroupId[i] = (byte)readUv (NumberBitsPerSliceGroupId, "sliceGroupId[i]", s);
        break;
        }

      default:;
      }
    }
    //}}}

  pps->numRefIndexL0defaultActiveMinus1 = readUeV ("PPS numRefIndexL0defaultActiveMinus1", s);
  pps->numRefIndexL1defaultActiveMinus1 = readUeV ("PPS numRefIndexL1defaultActiveMinus1", s);
  pps->weightedPredFlag = readU1 ("PPS weightedPredFlag", s);
  pps->weightedBiPredIdc = readUv (2, "PPS weightedBiPredIdc", s);
  pps->picInitQpMinus26 = readSeV ("PPS picInitQpMinus26", s);
  pps->picInitQsMinus26 = readSeV ("PPS picInitQsMinus26", s);
  pps->chromaQpIndexOffset = readSeV ("PPS chromaQpIndexOffset", s);
  pps->deblockFilterControlPresent = readU1 ("PPS deblockFilterControlPresent" , s);
  pps->constrainedIntraPredFlag = readU1 ("PPS constrainedIntraPredFlag", s);
  pps->redundantPicCountPresent = readU1 ("PPS redundantPicCountPresent", s);

  int more = moreRbspData (s->bitStreamBuffer, s->bitStreamOffset,s->bitStreamLen);
  if (more) {
    //{{{  more data in rbsp
    // Fidelity Range Extensions Stuff
    pps->transform8x8modeFlag = readU1 ("PPS transform8x8modeFlag", s);

    pps->picScalingMatrixPresentFlag = readU1 ("PPS picScalingMatrixPresentFlag", s);
    if (pps->picScalingMatrixPresentFlag) {
      chromaFormatIdc = decoder->sps[pps->spsId].chromaFormatIdc;
      n_ScalingList = 6 + ((chromaFormatIdc != YUV444) ? 2 : 6) * pps->transform8x8modeFlag;
      for (unsigned i = 0; i < n_ScalingList; i++) {
        pps->picScalingListPresentFlag[i]= readU1 ("PPS picScalingListPresentFlag", s);
        if (pps->picScalingListPresentFlag[i]) {
          if (i < 6)
            scalingList (pps->scalingList4x4[i], 16, &pps->useDefaultScalingMatrix4x4Flag[i], s);
          else
            scalingList (pps->scalingList8x8[i-6], 64, &pps->useDefaultScalingMatrix8x8Flag[i-6], s);
          }
        }
      }
    pps->secondChromaQpIndexOffset = readSeV ("PPS secondChromaQpIndexOffset", s);
    }
    //}}}
  else
    pps->secondChromaQpIndexOffset = pps->chromaQpIndexOffset;

  if (decoder->param.ppsDebug)
    printf ("-> ppsId:%d spsId:%d %d:%d%s%s%s%s%s%s%s%s%s grp:%d L:%d:%d\n",
            pps->ppsId, pps->spsId,
            pps->picInitQpMinus26, pps->picInitQsMinus26,
            pps->entropyCodingMode ? " cabac":" cavlc",
            pps->deblockFilterControlPresent ? " deblock":"",
            pps->weightedPredFlag ? " pred":"",
            pps->weightedBiPredIdc ? " biPred":"",
            pps->constrainedIntraPredFlag ? " intraPred":"",
            pps->redundantPicCountPresent ? " red":"",
            pps->botFieldPicOrderFramePresent ? " botField":"",
            more && pps->transform8x8modeFlag ? " 8x8":"",
            more && pps->picScalingMatrixPresentFlag ? " scaling":"",
            pps->numSliceGroupsMinus1,
            pps->numRefIndexL0defaultActiveMinus1, pps->numRefIndexL1defaultActiveMinus1);

  pps->valid = TRUE;
  }
//}}}

//{{{
sPPS* allocPPS() {

  sPPS* pps = calloc (1, sizeof (sPPS));
  pps->sliceGroupId = NULL;
  return pps;
  }
//}}}
//{{{
 void freePPS (sPPS* pps) {

   if (!pps->sliceGroupId)
     free (pps->sliceGroupId);
   free (pps);
   }
//}}}
//{{{
void setPPSbyId (sDecoder* decoder, int id, sPPS* pps) {

  if (decoder->pps[id].valid && decoder->pps[id].sliceGroupId)
    free (decoder->pps[id].sliceGroupId);
  memcpy (&decoder->pps[id], pps, sizeof (sPPS));

  // we can simply use the memory provided with the pps.
  // the PPS is destroyed after this function call, will not try to free if pps->sliceGroupId == NULL
  decoder->pps[id].sliceGroupId = pps->sliceGroupId;
  pps->sliceGroupId = NULL;
  }
//}}}

//{{{
void readNaluPPS (sDecoder* decoder, sNalu* nalu) {

  sDataPartition* dataPartition = allocDataPartitions (1);
  dataPartition->s->errorFlag = 0;
  dataPartition->s->readLen = dataPartition->s->bitStreamOffset = 0;
  memcpy (dataPartition->s->bitStreamBuffer, &nalu->buf[1], nalu->len - 1);
  dataPartition->s->codeLen = dataPartition->s->bitStreamLen = RBSPtoSODB (dataPartition->s->bitStreamBuffer, nalu->len-1);

  sPPS* pps = allocPPS();
  readPPS (decoder, dataPartition, pps);
  if (decoder->activePPS) {
    if (pps->ppsId == decoder->activePPS->ppsId) {
      if (!isEqualPPS (pps, decoder->activePPS)) {
        // copy to next PPS;
        memcpy (decoder->nextPPS, decoder->activePPS, sizeof (sPPS));
        if (decoder->picture)
          endDecodeFrame (decoder);
        decoder->activePPS = NULL;
        }
      }
    }

  setPPSbyId (decoder, pps->ppsId, pps);
  freeDataPartitions (dataPartition, 1);
  freePPS (pps);
  }
//}}}
//{{{
void activatePPS (sDecoder* decoder, sPPS* pps) {

  if (decoder->activePPS != pps) {
    if (decoder->picture) // only on slice loss
      endDecodeFrame (decoder);
    decoder->activePPS = pps;
    }
  }
//}}}
