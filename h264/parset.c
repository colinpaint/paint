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
extern void init_frext (sVidParam* vidParam);

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

  format->max_value[0] = (1 << format->bit_depth[0]) - 1;
  format->max_value_sq[0] = format->max_value[0] * format->max_value[0];

  format->max_value[1] = (1 << format->bit_depth[1]) - 1;
  format->max_value_sq[1] = format->max_value[1] * format->max_value[1];

  format->max_value[2] = (1 << format->bit_depth[2]) - 1;
  format->max_value_sq[2] = format->max_value[2] * format->max_value[2];
  }
//}}}
//{{{
static void setupLayerInfo (sVidParam* vidParam, sSPS* sps, sLayerParam* layerParam) {

  layerParam->vidParam = vidParam;
  layerParam->codingParam = vidParam->codingParam[layerParam->layerId];
  layerParam->sps = sps;
  layerParam->dpb = vidParam->dpbLayer[layerParam->layerId];
  }
//}}}
//{{{
static void setCodingParam (sSPS* sps, sCodingParam* codingParam) {

  // maximum vertical motion vector range in luma quarter pixel units
  codingParam->profileIdc = sps->profileIdc;
  codingParam->lossless_qpprime_flag = sps->lossless_qpprime_flag;
  if (sps->level_idc <= 10)
    codingParam->max_vmv_r = 64 * 4;
  else if (sps->level_idc <= 20)
    codingParam->max_vmv_r = 128 * 4;
  else if (sps->level_idc <= 30)
    codingParam->max_vmv_r = 256 * 4;
  else
    codingParam->max_vmv_r = 512 * 4; // 512 pixels in quarter pixels

  // Fidelity Range Extensions stuff (part 1)
  codingParam->bitdepthChroma = 0;
  codingParam->widthCr = 0;
  codingParam->heightCr = 0;
  codingParam->bitdepthLuma = (short) (sps->bit_depth_luma_minus8 + 8);
  codingParam->bitdepth_scale[0] = 1 << sps->bit_depth_luma_minus8;
  if (sps->chromaFormatIdc != YUV400) {
    codingParam->bitdepthChroma = (short) (sps->bit_depth_chroma_minus8 + 8);
    codingParam->bitdepth_scale[1] = 1 << sps->bit_depth_chroma_minus8;
    }

  codingParam->max_frame_num = 1<<(sps->log2_max_frame_num_minus4+4);
  codingParam->PicWidthInMbs = (sps->pic_width_in_mbs_minus1 +1);
  codingParam->PicHeightInMapUnits = (sps->pic_height_in_map_units_minus1 +1);
  codingParam->FrameHeightInMbs = ( 2 - sps->frameMbOnlyFlag ) * codingParam->PicHeightInMapUnits;
  codingParam->FrameSizeInMbs = codingParam->PicWidthInMbs * codingParam->FrameHeightInMbs;

  codingParam->yuvFormat=sps->chromaFormatIdc;
  codingParam->sepColourPlaneFlag = sps->sepColourPlaneFlag;
  if (codingParam->sepColourPlaneFlag )
    codingParam->ChromaArrayType = 0;
  else
    codingParam->ChromaArrayType = sps->chromaFormatIdc;

  codingParam->width = codingParam->PicWidthInMbs * MB_BLOCK_SIZE;
  codingParam->height = codingParam->FrameHeightInMbs * MB_BLOCK_SIZE;

  codingParam->iLumaPadX = MCBUF_LUMA_PAD_X;
  codingParam->iLumaPadY = MCBUF_LUMA_PAD_Y;
  codingParam->iChromaPadX = MCBUF_CHROMA_PAD_X;
  codingParam->iChromaPadY = MCBUF_CHROMA_PAD_Y;
  if (sps->chromaFormatIdc == YUV420) {
    codingParam->widthCr  = (codingParam->width  >> 1);
    codingParam->heightCr = (codingParam->height >> 1);
    }
  else if (sps->chromaFormatIdc == YUV422) {
    codingParam->widthCr  = (codingParam->width >> 1);
    codingParam->heightCr = codingParam->height;
    codingParam->iChromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (sps->chromaFormatIdc == YUV444) {
    codingParam->widthCr = codingParam->width;
    codingParam->heightCr = codingParam->height;
    codingParam->iChromaPadX = codingParam->iLumaPadX;
    codingParam->iChromaPadY = codingParam->iLumaPadY;
    }
  //pel bitdepth init
  codingParam->bitdepth_luma_qp_scale   = 6 * (codingParam->bitdepthLuma - 8);

  if (codingParam->bitdepthLuma > codingParam->bitdepthChroma || sps->chromaFormatIdc == YUV400)
    codingParam->picUnitBitSizeDisk = (codingParam->bitdepthLuma > 8)? 16:8;
  else
    codingParam->picUnitBitSizeDisk = (codingParam->bitdepthChroma > 8)? 16:8;
  codingParam->dcPredValueComp[0] = 1<<(codingParam->bitdepthLuma - 1);
  codingParam->maxPelValueComp[0] = (1<<codingParam->bitdepthLuma) - 1;
  codingParam->mbSize[0][0] = codingParam->mbSize[0][1] = MB_BLOCK_SIZE;

  if (sps->chromaFormatIdc != YUV400) {
    // for chrominance part
    codingParam->bitdepthChromaQpScale = 6 * (codingParam->bitdepthChroma - 8);
    codingParam->dcPredValueComp[1] = (1 << (codingParam->bitdepthChroma - 1));
    codingParam->dcPredValueComp[2] = codingParam->dcPredValueComp[1];
    codingParam->maxPelValueComp[1] = (1 << codingParam->bitdepthChroma) - 1;
    codingParam->maxPelValueComp[2] = (1 << codingParam->bitdepthChroma) - 1;
    codingParam->numBlock8x8uv = (1 << sps->chromaFormatIdc) & (~(0x1));
    codingParam->numUvBlocks = (codingParam->numBlock8x8uv >> 1);
    codingParam->numCdcCoeff = (codingParam->numBlock8x8uv << 1);
    codingParam->mbSize[1][0] = codingParam->mbSize[2][0] = codingParam->mbCrSizeX  = (sps->chromaFormatIdc==YUV420 || sps->chromaFormatIdc==YUV422)?  8 : 16;
    codingParam->mbSize[1][1] = codingParam->mbSize[2][1] = codingParam->mbCrSizeY  = (sps->chromaFormatIdc==YUV444 || sps->chromaFormatIdc==YUV422)? 16 :  8;

    codingParam->subpelX = codingParam->mbCrSizeX == 8 ? 7 : 3;
    codingParam->subpelY = codingParam->mbCrSizeY == 8 ? 7 : 3;
    codingParam->shiftpelX = codingParam->mbCrSizeX == 8 ? 3 : 2;
    codingParam->shiftpelY = codingParam->mbCrSizeY == 8 ? 3 : 2;
    codingParam->totalScale = codingParam->shiftpelX + codingParam->shiftpelY;
    }
  else {
    codingParam->bitdepthChromaQpScale = 0;
    codingParam->maxPelValueComp[1] = 0;
    codingParam->maxPelValueComp[2] = 0;
    codingParam->numBlock8x8uv = 0;
    codingParam->numUvBlocks = 0;
    codingParam->numCdcCoeff = 0;
    codingParam->mbSize[1][0] = codingParam->mbSize[2][0] = codingParam->mbCrSizeX  = 0;
    codingParam->mbSize[1][1] = codingParam->mbSize[2][1] = codingParam->mbCrSizeY  = 0;
    codingParam->subpelX = 0;
    codingParam->subpelY = 0;
    codingParam->shiftpelX = 0;
    codingParam->shiftpelY = 0;
    codingParam->totalScale = 0;
    }

  codingParam->mbCrSize = codingParam->mbCrSizeX * codingParam->mbCrSizeY;
  codingParam->mbSizeBlock[0][0] = codingParam->mbSizeBlock[0][1] = codingParam->mbSize[0][0] >> 2;
  codingParam->mbSizeBlock[1][0] = codingParam->mbSizeBlock[2][0] = codingParam->mbSize[1][0] >> 2;
  codingParam->mbSizeBlock[1][1] = codingParam->mbSizeBlock[2][1] = codingParam->mbSize[1][1] >> 2;

  codingParam->mbSizeShift[0][0] = codingParam->mbSizeShift[0][1] = ceilLog2sf (codingParam->mbSize[0][0]);
  codingParam->mbSizeShift[1][0] = codingParam->mbSizeShift[2][0] = ceilLog2sf (codingParam->mbSize[1][0]);
  codingParam->mbSizeShift[1][1] = codingParam->mbSizeShift[2][1] = ceilLog2sf (codingParam->mbSize[1][1]);
  }
//}}}
//{{{
static void resetFormatInfo (sSPS* sps, sVidParam* vidParam, sFrameFormat* source, sFrameFormat* output) {

  static const int SubWidthC[4] = { 1, 2, 2, 1};
  static const int SubHeightC[4] = { 1, 2, 1, 1};

  // cropping for luma
  int crop_left, cropRight;
  int crop_top, cropBot;
  if (sps->frameCropFlag) {
    crop_left = SubWidthC [sps->chromaFormatIdc] * sps->frameCropLeft;
    cropRight = SubWidthC [sps->chromaFormatIdc] * sps->frameCropRight;
    crop_top = SubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnlyFlag ) *  sps->frameCropTop;
    cropBot = SubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnlyFlag ) *  sps->frameCropBot;
    }
  else
    crop_left = cropRight = crop_top = cropBot = 0;

  source->width[0] = vidParam->width - crop_left - cropRight;
  source->height[0] = vidParam->height - crop_top - cropBot;

  // cropping for chroma
  if (sps->frameCropFlag) {
    crop_left = sps->frameCropLeft;
    cropRight  = sps->frameCropRight;
    crop_top = ( 2 - sps->frameMbOnlyFlag ) *  sps->frameCropTop;
    cropBot = ( 2 - sps->frameMbOnlyFlag ) *  sps->frameCropBot;
    }
  else
    crop_left = cropRight = crop_top = cropBot = 0;

  sInputParam* inputParam = vidParam->inputParam;
  if ((sps->chromaFormatIdc == YUV400) && inputParam->writeUv) {
    source->width[1] = (source->width[0] >> 1);
    source->width[2] = source->width[1];
    source->height[1] = (source->height[0] >> 1);
    source->height[2] = source->height[1];
    }
  else {
    source->width[1] = vidParam->widthCr - crop_left - cropRight;
    source->width[2] = source->width[1];
    source->height[1] = vidParam->heightCr - crop_top - cropBot;
    source->height[2] = source->height[1];
    }

  output->width[0] = vidParam->width;
  source->width[1] = vidParam->widthCr;
  source->width[2] = vidParam->widthCr;
  output->height[0] = vidParam->height;
  output->height[1] = vidParam->heightCr;
  output->height[2] = vidParam->heightCr;

  source->size_cmp[0] = source->width[0] * source->height[0];
  source->size_cmp[1] = source->width[1] * source->height[1];
  source->size_cmp[2] = source->size_cmp[1];
  source->size = source->size_cmp[0] + source->size_cmp[1] + source->size_cmp[2];
  source->mb_width = source->width[0]  / MB_BLOCK_SIZE;
  source->mb_height = source->height[0] / MB_BLOCK_SIZE;

  // output size (excluding padding)
  output->size_cmp[0] = output->width[0] * output->height[0];
  output->size_cmp[1] = output->width[1] * output->height[1];
  output->size_cmp[2] = output->size_cmp[1];
  output->size = output->size_cmp[0] + output->size_cmp[1] + output->size_cmp[2];
  output->mb_width = output->width[0]  / MB_BLOCK_SIZE;
  output->mb_height = output->height[0] / MB_BLOCK_SIZE;

  output->bit_depth[0] = source->bit_depth[0] = vidParam->bitdepthLuma;
  output->bit_depth[1] = source->bit_depth[1] = vidParam->bitdepthChroma;
  output->bit_depth[2] = source->bit_depth[2] = vidParam->bitdepthChroma;
  output->pic_unit_size_on_disk = (imax(output->bit_depth[0], output->bit_depth[1]) > 8) ? 16 : 8;
  output->pic_unit_size_shift3 = output->pic_unit_size_on_disk >> 3;

  output->frameRate = source->frameRate;
  output->colourModel = source->colourModel;
  output->yuvFormat = source->yuvFormat = (eColorFormat)sps->chromaFormatIdc;

  output->autoCropBot = cropBot;
  output->autoCropRight = cropRight;
  output->autoCropBotCr = (cropBot * vidParam->mbCrSizeY) / MB_BLOCK_SIZE;
  output->autoCropRightCr = (cropRight * vidParam->mbCrSizeX) / MB_BLOCK_SIZE;

  source->autoCropBot = output->autoCropBot;
  source->autoCropRight = output->autoCropRight;
  source->autoCropBotCr = output->autoCropBotCr;
  source->autoCropRightCr = output->autoCropRightCr;

  updateMaxValue (source);
  updateMaxValue (output);

  if (vidParam->firstSPS == TRUE) {
    vidParam->firstSPS = FALSE;
    printf ("ProfileIDC: %d %dx%d %dx%d ",
            sps->profileIdc, source->width[0], source->height[0], vidParam->width, vidParam->height);
    if (vidParam->yuvFormat == YUV400)
      printf ("4:0:0");
    else if (vidParam->yuvFormat == YUV420)
      printf ("4:2:0");
    else if (vidParam->yuvFormat == YUV422)
      printf ("4:2:2");
    else
      printf ("4:4:4");
    printf (" %d:%d:%d\n", source->bit_depth[0], source->bit_depth[1], source->bit_depth[2]);
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
static int spsIsEqual (sSPS* sps1, sSPS* sps2) {

  int equal = 1;

  if ((!sps1->valid) || (!sps2->valid))
    return 0;

  equal &= (sps1->profileIdc == sps2->profileIdc);
  equal &= (sps1->constrained_set0_flag == sps2->constrained_set0_flag);
  equal &= (sps1->constrained_set1_flag == sps2->constrained_set1_flag);
  equal &= (sps1->constrained_set2_flag == sps2->constrained_set2_flag);
  equal &= (sps1->level_idc == sps2->level_idc);
  equal &= (sps1->spsId == sps2->spsId);
  equal &= (sps1->log2_max_frame_num_minus4 == sps2->log2_max_frame_num_minus4);
  equal &= (sps1->picOrderCountType == sps2->picOrderCountType);
  if (!equal)
    return equal;

  if (sps1->picOrderCountType == 0)
    equal &= (sps1->log2_max_pic_order_cnt_lsb_minus4 == sps2->log2_max_pic_order_cnt_lsb_minus4);
  else if( sps1->picOrderCountType == 1) {
    equal &= (sps1->delta_pic_order_always_zero_flag == sps2->delta_pic_order_always_zero_flag);
    equal &= (sps1->offset_for_non_ref_pic == sps2->offset_for_non_ref_pic);
    equal &= (sps1->offset_for_top_to_bottom_field == sps2->offset_for_top_to_bottom_field);
    equal &= (sps1->num_ref_frames_in_pic_order_cnt_cycle == sps2->num_ref_frames_in_pic_order_cnt_cycle);
    if (!equal)
      return equal;
    for (unsigned i = 0 ; i < sps1->num_ref_frames_in_pic_order_cnt_cycle ;i ++)
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
// syntax for scaling list matrix values
static void scalingList (int* scalingList, int sizeOfScalingList,
                         Boolean* useDefaultScalingMatrix, sBitstream* s) {

  int lastScale = 8;
  int nextScale = 8;
  for (int j = 0; j < sizeOfScalingList; j++) {
    int scanj = (sizeOfScalingList == 16) ? ZZ_SCAN[j]:ZZ_SCAN8[j];
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
static void initVUI (sSPS* sps) {
  sps->vui_seq_parameters.matrix_coefficients = 2;
  }
//}}}
//{{{
static int readHRDParameters (sDataPartition* p, sHRD* hrd) {

  sBitstream *s = p->bitstream;
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

  return 0;
  }
//}}}
//{{{
static int readVUI (sDataPartition* p, sSPS* sps) {

  sBitstream* s = p->bitstream;
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
      readHRDParameters (p, &(sps->vui_seq_parameters.nal_hrd_parameters));

    sps->vui_seq_parameters.vcl_hrd_parameters_present_flag = readU1 ("VUI vcl_hrd_parameters_present_flag", s);

    if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
      readHRDParameters(p, &(sps->vui_seq_parameters.vcl_hrd_parameters));

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

  return 0;
  }
//}}}
//{{{
static void interpretSPS (sVidParam* vidParam, sDataPartition* p, sSPS* sps) {

  unsigned i;
  unsigned n_ScalingList;
  int reserved_zero;
  sBitstream *s = p->bitstream;

  assert (p != NULL);
  assert (p->bitstream != NULL);
  assert (p->bitstream->streamBuffer != 0);
  assert (sps != NULL);

  sps->profileIdc = readUv (8, "SPS profileIdc", s);
  if ((sps->profileIdc != BASELINE) &&
      (sps->profileIdc != MAIN) &&
      (sps->profileIdc != EXTENDED) &&
      (sps->profileIdc != FREXT_HP) &&
      (sps->profileIdc != FREXT_Hi10P) &&
      (sps->profileIdc != FREXT_Hi422) &&
      (sps->profileIdc != FREXT_Hi444) &&
      (sps->profileIdc != FREXT_CAVLC444)) {
    printf ("Invalid Profile IDC (%d) encountered. \n", sps->profileIdc);
    }

  sps->constrained_set0_flag = readU1 ("SPS constrained_set0_flag", s);
  sps->constrained_set1_flag = readU1 ("SPS constrained_set1_flag", s);
  sps->constrained_set2_flag = readU1 ("SPS constrained_set2_flag", s);
  sps->constrained_set3_flag = readU1 ("SPS constrained_set3_flag", s);

  reserved_zero = readUv (4, "SPS reserved_zero_4bits", s);

  if (reserved_zero != 0)
    printf ("Warning, reserved_zero flag not equal to 0. Possibly new constrained_setX flag introduced.\n");

  sps->level_idc = readUv (8, "SPS level_idc", s);
  sps->spsId = readUeV ("SPS spsId", s);

  // Fidelity Range Extensions stuff
  sps->chromaFormatIdc = 1;
  sps->bit_depth_luma_minus8 = 0;
  sps->bit_depth_chroma_minus8 = 0;
  sps->lossless_qpprime_flag = 0;
  sps->sepColourPlaneFlag = 0;

  if ((sps->profileIdc == FREXT_HP   ) ||
      (sps->profileIdc == FREXT_Hi10P) ||
      (sps->profileIdc == FREXT_Hi422) ||
      (sps->profileIdc == FREXT_Hi444) ||
      (sps->profileIdc == FREXT_CAVLC444)) {
    sps->chromaFormatIdc = readUeV ("SPS chromaFormatIdc", s);
    if (sps->chromaFormatIdc == YUV444)
      sps->sepColourPlaneFlag = readU1 ("SPS sepColourPlaneFlag", s);

    sps->bit_depth_luma_minus8 = readUeV ("SPS bit_depth_luma_minus8", s);
    sps->bit_depth_chroma_minus8 = readUeV ("SPS bit_depth_chroma_minus8", s);
    if ((sps->bit_depth_luma_minus8+8 > sizeof(sPixel)*8) ||
        (sps->bit_depth_chroma_minus8+8> sizeof(sPixel)*8))
      error ("Source picture has higher bit depth than sPixel data type", 500);

    sps->lossless_qpprime_flag = readU1 ("SPS lossless_qpprime_y_zero_flag", s);

    sps->seq_scaling_matrix_present_flag = readU1 ("SPS seq_scaling_matrix_present_flag", s);
    if (sps->seq_scaling_matrix_present_flag) {
      n_ScalingList = (sps->chromaFormatIdc != YUV444) ? 8 : 12;
      for (i = 0; i < n_ScalingList; i++) {
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

  sps->log2_max_frame_num_minus4 = readUeV ("SPS log2_max_frame_num_minus4", s);
  sps->picOrderCountType = readUeV ("SPS picOrderCountType", s);

  if (sps->picOrderCountType == 0)
    sps->log2_max_pic_order_cnt_lsb_minus4 = readUeV ("SPS log2_max_pic_order_cnt_lsb_minus4", s);
  else if (sps->picOrderCountType == 1) {
    sps->delta_pic_order_always_zero_flag = readU1 ("SPS delta_pic_order_always_zero_flag", s);
    sps->offset_for_non_ref_pic = readSeV ("SPS offset_for_non_ref_pic", s);
    sps->offset_for_top_to_bottom_field = readSeV ("SPS offset_for_top_to_bottom_field", s);
    sps->num_ref_frames_in_pic_order_cnt_cycle = readUeV ("SPS num_ref_frames_in_pic_order_cnt_cycle", s);
    for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      sps->offset_for_ref_frame[i] = readSeV ("SPS offset_for_ref_frame[i]", s);
    }

  sps->numRefFrames = readUeV ("SPS numRefFrames", s);
  sps->gaps_in_frame_num_value_allowed_flag = readU1 ("SPS gaps_in_frame_num_value_allowed_flag", s);
  sps->pic_width_in_mbs_minus1 = readUeV ("SPS pic_width_in_mbs_minus1", s);
  sps->pic_height_in_map_units_minus1 = readUeV ("SPS pic_height_in_map_units_minus1", s);
  sps->frameMbOnlyFlag = readU1 ("SPS frameMbOnlyFlag", s);
  if (!sps->frameMbOnlyFlag)
    sps->mb_adaptive_frame_field_flag = readU1 ("SPS mb_adaptive_frame_field_flag", s);

  sps->direct_8x8_inference_flag = readU1 ("SPS direct_8x8_inference_flag", s);

  sps->frameCropFlag = readU1 ("SPS frameCropFlag", s);
  if (sps->frameCropFlag) {
    sps->frameCropLeft = readUeV ("SPS frameCropLeft", s);
    sps->frameCropRight = readUeV ("SPS frameCropRight", s);
    sps->frameCropTop = readUeV ("SPS frameCropTop", s);
    sps->frameCropBot = readUeV ("SPS frameCropBot", s);
    }
  sps->vui_parameters_present_flag = (Boolean) readU1 ("SPS vui_parameters_present_flag", s);

  initVUI (sps);
  readVUI (p, sps);

  sps->valid = TRUE;
  }
//}}}

//{{{
void makeSPSavailable (sVidParam* vidParam, int id, sSPS* sps) {

  assert (sps->valid == TRUE);
  memcpy (&vidParam->SeqParSet[id], sps, sizeof (sSPS));
  }
//}}}
//{{{
void processSPS (sVidParam* vidParam, sNalu* nalu) {

  sDataPartition* dp = allocPartition (1);
  dp->bitstream->eiFlag = 0;
  dp->bitstream->readLen = dp->bitstream->frameBitOffset = 0;
  memcpy (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->codeLen = dp->bitstream->bitstreamLength = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);

  sSPS* sps = allocSPS();
  interpretSPS (vidParam, dp, sps);

  if (sps->valid) {
    if (vidParam->activeSPS) {
      if (sps->spsId == vidParam->activeSPS->spsId) {
        if (!spsIsEqual (sps, vidParam->activeSPS))   {
          if (vidParam->picture)
            exitPicture (vidParam, &vidParam->picture);
          vidParam->activeSPS=NULL;
          }
        }
      }

    makeSPSavailable (vidParam, sps->spsId, sps);

    vidParam->profileIdc = sps->profileIdc;
    vidParam->sepColourPlaneFlag = sps->sepColourPlaneFlag;
    if (vidParam->sepColourPlaneFlag )
      vidParam->ChromaArrayType = 0;
    else
      vidParam->ChromaArrayType = sps->chromaFormatIdc;
    }

  freePartition (dp, 1);
  freeSPS (sps);
  }
//}}}
//{{{
void activateSPS (sVidParam* vidParam, sSPS* sps) {

  sInputParam* inputParam = vidParam->inputParam;

  if (vidParam->activeSPS != sps) {
    if (vidParam->picture) // this may only happen on slice loss
      exitPicture (vidParam, &vidParam->picture);
    vidParam->activeSPS = sps;

    if (vidParam->dpbLayerId == 0 && is_BL_profile (sps->profileIdc) && !vidParam->dpbLayer[0]->initDone) {
      setCodingParam (sps, vidParam->codingParam[0]);
      setupLayerInfo ( vidParam, sps, vidParam->layerParam[0]);
      }
    setGlobalCodingProgram (vidParam, vidParam->codingParam[vidParam->dpbLayerId]);

    initGlobalBuffers (vidParam, 0);
    if (!vidParam->noOutputPriorPicFlag)
      flushDpb (vidParam->dpbLayer[0]);

    initDpb (vidParam, vidParam->dpbLayer[0], 0);

    // enable error conceal
    ercInit (vidParam, vidParam->width, vidParam->height, 1);
    if (vidParam->picture) {
      ercReset (vidParam->ercErrorVar, vidParam->picSizeInMbs, vidParam->picSizeInMbs, vidParam->picture->sizeX);
      vidParam->ercMvPerMb = 0;
      }
    }

  resetFormatInfo (sps, vidParam, &inputParam->source, &inputParam->output);
  }
//}}}

// PPS
//{{{
static int ppsIsEqual (sPPS* pps1, sPPS* pps2) {

  if ((!pps1->valid) || (!pps2->valid))
    return 0;

  int equal = 1;
  equal &= (pps1->ppsId == pps2->ppsId);
  equal &= (pps1->spsId == pps2->spsId);
  equal &= (pps1->entropyCodingModeFlag == pps2->entropyCodingModeFlag);
  equal &= (pps1->botFieldPicOrderFramePresentFlag == pps2->botFieldPicOrderFramePresentFlag);
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

  equal &= (pps1->numRefIndexL0defaultActiveMinus1 == pps2->numRefIndexL0defaultActiveMinus1);
  equal &= (pps1->numRefIndexL1defaultActiveMinus1 == pps2->numRefIndexL1defaultActiveMinus1);
  equal &= (pps1->weightedPredFlag == pps2->weightedPredFlag);
  equal &= (pps1->weightedBiPredIdc == pps2->weightedBiPredIdc);
  equal &= (pps1->picInitQpMinus26 == pps2->picInitQpMinus26);
  equal &= (pps1->picInitQsMinus26 == pps2->picInitQsMinus26);
  equal &= (pps1->chromaQpIndexOffset == pps2->chromaQpIndexOffset);
  equal &= (pps1->deblockingFilterControlPresentFlag == pps2->deblockingFilterControlPresentFlag);
  equal &= (pps1->constrainedIntraPredFlag == pps2->constrainedIntraPredFlag);
  equal &= (pps1->redundantPicCountPresentFlag == pps2->redundantPicCountPresentFlag);
  if (!equal)
    return equal;

  // Fidelity Range Extensions
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
static void interpretPPS (sVidParam* vidParam, sDataPartition* p, sPPS* pps) {

  unsigned n_ScalingList;
  int chromaFormatIdc;
  int NumberBitsPerSliceGroupId;

  sBitstream* s = p->bitstream;
  assert (p != NULL);
  assert (p->bitstream != NULL);
  assert (p->bitstream->streamBuffer != 0);
  assert (pps != NULL);

  pps->ppsId = readUeV ("PPS ppsId", s);
  pps->spsId = readUeV ("PPS spsId", s);
  pps->entropyCodingModeFlag = readU1 ("PPS entropyCodingModeFlag", s);

  //! Note: as per JVT-F078 the following bit is unconditional.  If F078 is not accepted, then
  //! one has to fetch the correct SPS to check whether the bit is present (hopefully there is
  //! no consistency problem :-(
  //! The current encoder code handles this in the same way.  When you change this, don't forget
  //! the encoder!  StW, 12/8/02
  pps->botFieldPicOrderFramePresentFlag =
    readU1 ("PPS botFieldPicOrderFramePresentFlag", s);
  pps->numSliceGroupsMinus1 = readUeV ("PPS numSliceGroupsMinus1", s);

  if (pps->numSliceGroupsMinus1 > 0) {
    //{{{  FMO
    pps->sliceGroupMapType = readUeV ("PPS sliceGroupMapType", s);
    if (pps->sliceGroupMapType == 0) {
      for (unsigned i = 0; i <= pps->numSliceGroupsMinus1; i++)
        pps->runLengthMinus1 [i] = readUeV ("PPS runLengthMinus1 [i]", s);
      }
    else if (pps->sliceGroupMapType == 2) {
      for (unsigned i = 0; i < pps->numSliceGroupsMinus1; i++) {
        //! JVT-F078: avoid reference of SPS by using ue(v) instead of u(v)
        pps->topLeft [i] = readUeV ("PPS topLeft [i]", s);
        pps->botRight [i] = readUeV ("PPS botRight [i]", s);
        }
      }
    else if (pps->sliceGroupMapType == 3 ||
             pps->sliceGroupMapType == 4 ||
             pps->sliceGroupMapType == 5) {
      pps->sliceGroupChangeDirectionFlag =
        readU1 ("PPS sliceGroupChangeDirectionFlag", s);
      pps->sliceGroupChangeRateMius1 =
        readUeV ("PPS sliceGroupChangeRateMius1", s);
      }
    else if (pps->sliceGroupMapType == 6) {
      if (pps->numSliceGroupsMinus1+1 >4)
        NumberBitsPerSliceGroupId = 3;
      else if (pps->numSliceGroupsMinus1+1 > 2)
        NumberBitsPerSliceGroupId = 2;
      else
        NumberBitsPerSliceGroupId = 1;
      pps->picSizeMapUnitsMinus1      = readUeV ("PPS picSizeMapUnitsMinus1"               , s);
      if ((pps->sliceGroupId = calloc (pps->picSizeMapUnitsMinus1+1, 1)) == NULL)
        no_mem_exit ("InterpretPPS sliceGroupId");
      for (unsigned i = 0; i <= pps->picSizeMapUnitsMinus1; i++)
        pps->sliceGroupId[i] =
          (byte)readUv (NumberBitsPerSliceGroupId, "sliceGroupId[i]", s);
      }
    }
    //}}}

  pps->numRefIndexL0defaultActiveMinus1 = readUeV ("PPS numRefIndexL0defaultActiveMinus1", s);
  pps->numRefIndexL1defaultActiveMinus1 = readUeV ("PPS numRefIndexL1defaultActiveMinus1", s);

  pps->weightedPredFlag = readU1 ("PPS weightedPredFlag", s);
  pps->weightedBiPredIdc = readUv ( 2, "PPS weightedBiPredIdc", s);

  pps->picInitQpMinus26 = readSeV ("PPS picInitQpMinus26", s);
  pps->picInitQsMinus26 = readSeV ("PPS picInitQsMinus26", s);

  pps->chromaQpIndexOffset = readSeV ("PPS chromaQpIndexOffset"   , s);
  pps->deblockingFilterControlPresentFlag =
    readU1 ("PPS deblockingFilterControlPresentFlag" , s);

  pps->constrainedIntraPredFlag = readU1 ("PPS constrainedIntraPredFlag", s);
  pps->redundantPicCountPresentFlag =
    readU1 ("PPS redundantPicCountPresentFlag", s);

  if (more_rbsp_data (s->streamBuffer, s->frameBitOffset,s->bitstreamLength)) {
    //{{{  more_data_in_rbsp
    // Fidelity Range Extensions Stuff
    pps->transform8x8modeFlag = readU1 ("PPS transform8x8modeFlag", s);
    pps->picScalingMatrixPresentFlag = readU1 ("PPS picScalingMatrixPresentFlag", s);

    if (pps->picScalingMatrixPresentFlag) {
      chromaFormatIdc = vidParam->SeqParSet[pps->spsId].chromaFormatIdc;
      n_ScalingList = 6 + ((chromaFormatIdc != YUV444) ? 2 : 6) * pps->transform8x8modeFlag;
      for (unsigned i = 0; i < n_ScalingList; i++) {
        pps->picScalingListPresentFlag[i]=
          readU1 ("PPS picScalingListPresentFlag", s);
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

  pps->valid = TRUE;
  }
//}}}
//{{{
static void activatePPS (sVidParam* vidParam, sPPS* pps) {

  if (vidParam->activePPS != pps) {
    if (vidParam->picture) // this may only happen on slice loss
      exitPicture (vidParam, &vidParam->picture);
    vidParam->activePPS = pps;
    }
  }
//}}}

//{{{
sPPS* allocPPS() {

  sPPS* p = calloc (1, sizeof (sPPS));
  if (!p)
    no_mem_exit ("allocPPS");

  p->sliceGroupId = NULL;
  return p;
  }
//}}}
//{{{
 void freePPS (sPPS* pps) {

   assert (pps != NULL);
   if (pps->sliceGroupId != NULL)
     free (pps->sliceGroupId);
   free (pps);
   }
//}}}
//{{{
void makePPSavailable (sVidParam* vidParam, int id, sPPS* pps) {

  if (vidParam->PicParSet[id].valid && vidParam->PicParSet[id].sliceGroupId)
    free (vidParam->PicParSet[id].sliceGroupId);

  memcpy (&vidParam->PicParSet[id], pps, sizeof (sPPS));

  // we can simply use the memory provided with the pps. the PPS is destroyed after this function
  // call and will not try to free if pps->sliceGroupId == NULL
  vidParam->PicParSet[id].sliceGroupId = pps->sliceGroupId;
  pps->sliceGroupId = NULL;
  }
//}}}
//{{{
void cleanUpPPS (sVidParam* vidParam) {

  for (int i = 0; i < MAX_PPS; i++) {
    if (vidParam->PicParSet[i].valid == TRUE && vidParam->PicParSet[i].sliceGroupId != NULL)
      free (vidParam->PicParSet[i].sliceGroupId);
    vidParam->PicParSet[i].valid = FALSE;
    }
  }
//}}}
//{{{
void processPPS (sVidParam* vidParam, sNalu* nalu) {


  sDataPartition* dp = allocPartition (1);
  dp->bitstream->eiFlag = 0;
  dp->bitstream->readLen = dp->bitstream->frameBitOffset = 0;
  memcpy (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->codeLen = dp->bitstream->bitstreamLength = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);

  sPPS* pps = allocPPS();
  interpretPPS (vidParam, dp, pps);

  if (vidParam->activePPS) {
    if (pps->ppsId == vidParam->activePPS->ppsId) {
      if (!ppsIsEqual (pps, vidParam->activePPS)) {
        // copy to next PPS;
        memcpy (vidParam->nextPPS, vidParam->activePPS, sizeof (sPPS));
        if (vidParam->picture)
          exitPicture (vidParam, &vidParam->picture);
        vidParam->activePPS = NULL;
        }
      }
    }

  makePPSavailable (vidParam, pps->ppsId, pps);
  freePartition (dp, 1);
  freePPS (pps);
  }
//}}}

//{{{
void useParameterSet (sSlice* curSlice) {

  sVidParam* vidParam = curSlice->vidParam;
  int PicParsetId = curSlice->ppsId;

  sPPS* pps = &vidParam->PicParSet[PicParsetId];
  sSPS* sps = &vidParam->SeqParSet[pps->spsId];

  if (pps->valid != TRUE)
    printf ("Trying to use an invalid (uninitialized) Picture Parameter Set with ID %d, expect the unexpected...\n", PicParsetId);

  if (sps->valid != TRUE)
    printf ("PicParset %d references uninitialized) SPS ID %d, unexpected\n",
            PicParsetId, (int) pps->spsId);

  // In theory, and with a well-designed software, the lines above are everything necessary.
  // In practice, we need to patch many values
  // in vidParam-> (but no more in inputParam-> -- these have been taken care of)
  // Set Sequence Parameter Stuff first
  if ((int) sps->picOrderCountType < 0 || sps->picOrderCountType > 2) {
    printf ("invalid sps->picOrderCountType = %d\n", (int) sps->picOrderCountType);
    error ("picOrderCountType != 1", -1000);
    }

  if (sps->picOrderCountType == 1)
    if (sps->num_ref_frames_in_pic_order_cnt_cycle >= MAX_NUM_REF_FRAMES_PIC_ORDER)
      error ("num_ref_frames_in_pic_order_cnt_cycle too large",-1011);

  vidParam->dpbLayerId = curSlice->layerId;
  activateSPS (vidParam, sps);
  activatePPS (vidParam, pps);

  // curSlice->dataPartitionMode is set by read_new_slice (NALU first byte available there)
  if (pps->entropyCodingModeFlag == (Boolean)CAVLC) {
    curSlice->nal_startcode_follows = uvlc_startcode_follows;
    for (int i = 0; i < 3; i++)
      curSlice->partArr[i].readsSyntaxElement = readsSyntaxElement_UVLC;
    }
  else {
    curSlice->nal_startcode_follows = cabac_startcode_follows;
    for (int i = 0; i < 3; i++)
      curSlice->partArr[i].readsSyntaxElement = readsSyntaxElement_CABAC;
    }
  vidParam->type = curSlice->sliceType;
  }
//}}}
