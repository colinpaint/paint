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
  codingParam->profile_idc = sps->profile_idc;
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
  codingParam->bitdepth_chroma = 0;
  codingParam->widthCr = 0;
  codingParam->heightCr = 0;
  codingParam->bitdepth_luma = (short) (sps->bit_depth_luma_minus8 + 8);
  codingParam->bitdepth_scale[0] = 1 << sps->bit_depth_luma_minus8;
  if (sps->chromaFormatIdc != YUV400) {
    codingParam->bitdepth_chroma = (short) (sps->bit_depth_chroma_minus8 + 8);
    codingParam->bitdepth_scale[1] = 1 << sps->bit_depth_chroma_minus8;
    }

  codingParam->max_frame_num = 1<<(sps->log2_max_frame_num_minus4+4);
  codingParam->PicWidthInMbs = (sps->pic_width_in_mbs_minus1 +1);
  codingParam->PicHeightInMapUnits = (sps->pic_height_in_map_units_minus1 +1);
  codingParam->FrameHeightInMbs = ( 2 - sps->frame_mbs_only_flag ) * codingParam->PicHeightInMapUnits;
  codingParam->FrameSizeInMbs = codingParam->PicWidthInMbs * codingParam->FrameHeightInMbs;

  codingParam->yuvFormat=sps->chromaFormatIdc;
  codingParam->separate_colour_plane_flag = sps->separate_colour_plane_flag;
  if (codingParam->separate_colour_plane_flag )
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
  codingParam->bitdepth_luma_qp_scale   = 6 * (codingParam->bitdepth_luma - 8);

  if (codingParam->bitdepth_luma > codingParam->bitdepth_chroma || sps->chromaFormatIdc == YUV400)
    codingParam->pic_unit_bitsize_on_disk = (codingParam->bitdepth_luma > 8)? 16:8;
  else
    codingParam->pic_unit_bitsize_on_disk = (codingParam->bitdepth_chroma > 8)? 16:8;
  codingParam->dc_pred_value_comp[0] = 1<<(codingParam->bitdepth_luma - 1);
  codingParam->max_pel_value_comp[0] = (1<<codingParam->bitdepth_luma) - 1;
  codingParam->mb_size[0][0] = codingParam->mb_size[0][1] = MB_BLOCK_SIZE;

  if (sps->chromaFormatIdc != YUV400) {
    // for chrominance part
    codingParam->bitdepth_chroma_qp_scale = 6 * (codingParam->bitdepth_chroma - 8);
    codingParam->dc_pred_value_comp[1] = (1 << (codingParam->bitdepth_chroma - 1));
    codingParam->dc_pred_value_comp[2] = codingParam->dc_pred_value_comp[1];
    codingParam->max_pel_value_comp[1] = (1 << codingParam->bitdepth_chroma) - 1;
    codingParam->max_pel_value_comp[2] = (1 << codingParam->bitdepth_chroma) - 1;
    codingParam->num_blk8x8_uv = (1 << sps->chromaFormatIdc) & (~(0x1));
    codingParam->num_uv_blocks = (codingParam->num_blk8x8_uv >> 1);
    codingParam->num_cdc_coeff = (codingParam->num_blk8x8_uv << 1);
    codingParam->mb_size[1][0] = codingParam->mb_size[2][0] = codingParam->mb_cr_size_x  = (sps->chromaFormatIdc==YUV420 || sps->chromaFormatIdc==YUV422)?  8 : 16;
    codingParam->mb_size[1][1] = codingParam->mb_size[2][1] = codingParam->mb_cr_size_y  = (sps->chromaFormatIdc==YUV444 || sps->chromaFormatIdc==YUV422)? 16 :  8;

    codingParam->subpel_x    = codingParam->mb_cr_size_x == 8 ? 7 : 3;
    codingParam->subpel_y    = codingParam->mb_cr_size_y == 8 ? 7 : 3;
    codingParam->shiftpel_x  = codingParam->mb_cr_size_x == 8 ? 3 : 2;
    codingParam->shiftpel_y  = codingParam->mb_cr_size_y == 8 ? 3 : 2;
    codingParam->total_scale = codingParam->shiftpel_x + codingParam->shiftpel_y;
    }
  else {
    codingParam->bitdepth_chroma_qp_scale = 0;
    codingParam->max_pel_value_comp[1] = 0;
    codingParam->max_pel_value_comp[2] = 0;
    codingParam->num_blk8x8_uv = 0;
    codingParam->num_uv_blocks = 0;
    codingParam->num_cdc_coeff = 0;
    codingParam->mb_size[1][0] = codingParam->mb_size[2][0] = codingParam->mb_cr_size_x  = 0;
    codingParam->mb_size[1][1] = codingParam->mb_size[2][1] = codingParam->mb_cr_size_y  = 0;
    codingParam->subpel_x = 0;
    codingParam->subpel_y = 0;
    codingParam->shiftpel_x = 0;
    codingParam->shiftpel_y = 0;
    codingParam->total_scale = 0;
    }

  codingParam->mb_cr_size = codingParam->mb_cr_size_x * codingParam->mb_cr_size_y;
  codingParam->mb_size_blk[0][0] = codingParam->mb_size_blk[0][1] = codingParam->mb_size[0][0] >> 2;
  codingParam->mb_size_blk[1][0] = codingParam->mb_size_blk[2][0] = codingParam->mb_size[1][0] >> 2;
  codingParam->mb_size_blk[1][1] = codingParam->mb_size_blk[2][1] = codingParam->mb_size[1][1] >> 2;

  codingParam->mb_size_shift[0][0] = codingParam->mb_size_shift[0][1] = ceilLog2sf (codingParam->mb_size[0][0]);
  codingParam->mb_size_shift[1][0] = codingParam->mb_size_shift[2][0] = ceilLog2sf (codingParam->mb_size[1][0]);
  codingParam->mb_size_shift[1][1] = codingParam->mb_size_shift[2][1] = ceilLog2sf (codingParam->mb_size[1][1]);
  }
//}}}
//{{{
static void resetFormatInfo (sSPS* sps, sVidParam* vidParam, sFrameFormat* source, sFrameFormat* output) {

  static const int SubWidthC[4] = { 1, 2, 2, 1};
  static const int SubHeightC[4] = { 1, 2, 1, 1};

  // cropping for luma
  int crop_left, crop_right;
  int crop_top, crop_bottom;
  if (sps->frame_cropping_flag) {
    crop_left = SubWidthC [sps->chromaFormatIdc] * sps->frame_crop_left_offset;
    crop_right = SubWidthC [sps->chromaFormatIdc] * sps->frame_crop_right_offset;
    crop_top = SubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frame_mbs_only_flag ) *  sps->frame_crop_top_offset;
    crop_bottom = SubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frame_mbs_only_flag ) *  sps->frame_crop_bottom_offset;
    }
  else
    crop_left = crop_right = crop_top = crop_bottom = 0;

  source->width[0] = vidParam->width - crop_left - crop_right;
  source->height[0] = vidParam->height - crop_top - crop_bottom;

  // cropping for chroma
  if (sps->frame_cropping_flag) {
    crop_left = sps->frame_crop_left_offset;
    crop_right  = sps->frame_crop_right_offset;
    crop_top = ( 2 - sps->frame_mbs_only_flag ) *  sps->frame_crop_top_offset;
    crop_bottom = ( 2 - sps->frame_mbs_only_flag ) *  sps->frame_crop_bottom_offset;
    }
  else
    crop_left = crop_right = crop_top = crop_bottom = 0;

  sInputParam* inputParam = vidParam->inputParam;
  if ((sps->chromaFormatIdc == YUV400) && inputParam->write_uv) {
    source->width[1] = (source->width[0] >> 1);
    source->width[2] = source->width[1];
    source->height[1] = (source->height[0] >> 1);
    source->height[2] = source->height[1];
    }
  else {
    source->width[1] = vidParam->widthCr - crop_left - crop_right;
    source->width[2] = source->width[1];
    source->height[1] = vidParam->heightCr - crop_top - crop_bottom;
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

  output->bit_depth[0] = source->bit_depth[0] = vidParam->bitdepth_luma;
  output->bit_depth[1] = source->bit_depth[1] = vidParam->bitdepth_chroma;
  output->bit_depth[2] = source->bit_depth[2] = vidParam->bitdepth_chroma;
  output->pic_unit_size_on_disk = (imax(output->bit_depth[0], output->bit_depth[1]) > 8) ? 16 : 8;
  output->pic_unit_size_shift3 = output->pic_unit_size_on_disk >> 3;

  output->frame_rate = source->frame_rate;
  output->color_model = source->color_model;
  output->yuvFormat = source->yuvFormat = (eColorFormat)sps->chromaFormatIdc;

  output->auto_crop_bottom = crop_bottom;
  output->auto_crop_right = crop_right;
  output->auto_crop_bottom_cr = (crop_bottom * vidParam->mb_cr_size_y) / MB_BLOCK_SIZE;
  output->auto_crop_right_cr = (crop_right * vidParam->mb_cr_size_x) / MB_BLOCK_SIZE;

  source->auto_crop_bottom = output->auto_crop_bottom;
  source->auto_crop_right = output->auto_crop_right;
  source->auto_crop_bottom_cr = output->auto_crop_bottom_cr;
  source->auto_crop_right_cr = output->auto_crop_right_cr;

  updateMaxValue (source);
  updateMaxValue (output);

  if (vidParam->firstSPS == TRUE) {
    vidParam->firstSPS = FALSE;
    printf ("ProfileIDC: %d %dx%d %dx%d ",
            sps->profile_idc, source->width[0], source->height[0], vidParam->width, vidParam->height);
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

  if ((!sps1->Valid) || (!sps2->Valid))
    return 0;

  equal &= (sps1->profile_idc == sps2->profile_idc);
  equal &= (sps1->constrained_set0_flag == sps2->constrained_set0_flag);
  equal &= (sps1->constrained_set1_flag == sps2->constrained_set1_flag);
  equal &= (sps1->constrained_set2_flag == sps2->constrained_set2_flag);
  equal &= (sps1->level_idc == sps2->level_idc);
  equal &= (sps1->seq_parameter_set_id == sps2->seq_parameter_set_id);
  equal &= (sps1->log2_max_frame_num_minus4 == sps2->log2_max_frame_num_minus4);
  equal &= (sps1->pic_order_cnt_type == sps2->pic_order_cnt_type);
  if (!equal)
    return equal;

  if (sps1->pic_order_cnt_type == 0)
    equal &= (sps1->log2_max_pic_order_cnt_lsb_minus4 == sps2->log2_max_pic_order_cnt_lsb_minus4);
  else if( sps1->pic_order_cnt_type == 1) {
    equal &= (sps1->delta_pic_order_always_zero_flag == sps2->delta_pic_order_always_zero_flag);
    equal &= (sps1->offset_for_non_ref_pic == sps2->offset_for_non_ref_pic);
    equal &= (sps1->offset_for_top_to_bottom_field == sps2->offset_for_top_to_bottom_field);
    equal &= (sps1->num_ref_frames_in_pic_order_cnt_cycle == sps2->num_ref_frames_in_pic_order_cnt_cycle);
    if (!equal)
      return equal;
    for (unsigned i = 0 ; i < sps1->num_ref_frames_in_pic_order_cnt_cycle ;i ++)
      equal &= (sps1->offset_for_ref_frame[i] == sps2->offset_for_ref_frame[i]);
    }

  equal &= (sps1->num_ref_frames == sps2->num_ref_frames);
  equal &= (sps1->gaps_in_frame_num_value_allowed_flag == sps2->gaps_in_frame_num_value_allowed_flag);
  equal &= (sps1->pic_width_in_mbs_minus1 == sps2->pic_width_in_mbs_minus1);
  equal &= (sps1->pic_height_in_map_units_minus1 == sps2->pic_height_in_map_units_minus1);
  equal &= (sps1->frame_mbs_only_flag == sps2->frame_mbs_only_flag);

  if (!equal) return
    equal;
  if (!sps1->frame_mbs_only_flag)
    equal &= (sps1->mb_adaptive_frame_field_flag == sps2->mb_adaptive_frame_field_flag);

  equal &= (sps1->direct_8x8_inference_flag == sps2->direct_8x8_inference_flag);
  equal &= (sps1->frame_cropping_flag == sps2->frame_cropping_flag);
  if (!equal)
    return equal;
  if (sps1->frame_cropping_flag) {
    equal &= (sps1->frame_crop_left_offset == sps2->frame_crop_left_offset);
    equal &= (sps1->frame_crop_right_offset == sps2->frame_crop_right_offset);
    equal &= (sps1->frame_crop_top_offset == sps2->frame_crop_top_offset);
    equal &= (sps1->frame_crop_bottom_offset == sps2->frame_crop_bottom_offset);
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
      int delta_scale = read_se_v ("   : delta_sl   ", s);
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
  hrd->cpb_cnt_minus1 = read_ue_v ("VUI cpb_cnt_minus1", s);
  hrd->bit_rate_scale = read_u_v (4, "VUI bit_rate_scale", s);
  hrd->cpb_size_scale = read_u_v (4, "VUI cpb_size_scale", s);

  unsigned int SchedSelIdx;
  for (SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++) {
    hrd->bit_rate_value_minus1[ SchedSelIdx] = read_ue_v ("VUI bit_rate_value_minus1", s);
    hrd->cpb_size_value_minus1[ SchedSelIdx] = read_ue_v ("VUI cpb_size_value_minus1", s);
    hrd->cbr_flag[ SchedSelIdx ] = read_u_1 ("VUI cbr_flag", s);
    }

  hrd->initial_cpb_removal_delay_length_minus1 =
    read_u_v (5, "VUI initial_cpb_removal_delay_length_minus1", s);
  hrd->cpb_removal_delay_length_minus1 = read_u_v (5, "VUI cpb_removal_delay_length_minus1", s);
  hrd->dpb_output_delay_length_minus1 = read_u_v (5, "VUI dpb_output_delay_length_minus1", s);
  hrd->time_offset_length = read_u_v (5, "VUI time_offset_length", s);

  return 0;
  }
//}}}
//{{{
static int readVUI (sDataPartition* p, sSPS* sps) {

  sBitstream* s = p->bitstream;
  if (sps->vui_parameters_present_flag) {
    sps->vui_seq_parameters.aspect_ratio_info_present_flag = read_u_1 ("VUI aspect_ratio_info_present_flag", s);
    if (sps->vui_seq_parameters.aspect_ratio_info_present_flag) {
      sps->vui_seq_parameters.aspect_ratio_idc = read_u_v ( 8, "VUI aspect_ratio_idc", s);
      if (255 == sps->vui_seq_parameters.aspect_ratio_idc) {
        sps->vui_seq_parameters.sar_width = (unsigned short)read_u_v (16, "VUI sar_width", s);
        sps->vui_seq_parameters.sar_height = (unsigned short)read_u_v (16, "VUI sar_height", s);
        }
      }

    sps->vui_seq_parameters.overscan_info_present_flag = read_u_1 ("VUI overscan_info_present_flag", s);
    if (sps->vui_seq_parameters.overscan_info_present_flag)
      sps->vui_seq_parameters.overscan_appropriate_flag = read_u_1 ("VUI overscan_appropriate_flag", s);

    sps->vui_seq_parameters.video_signal_type_present_flag = read_u_1 ("VUI video_signal_type_present_flag", s);
    if (sps->vui_seq_parameters.video_signal_type_present_flag) {
      sps->vui_seq_parameters.video_format = read_u_v (3, "VUI video_format", s);
      sps->vui_seq_parameters.video_full_range_flag = read_u_1 ("VUI video_full_range_flag", s);
      sps->vui_seq_parameters.colour_description_present_flag = read_u_1 ("VUI color_description_present_flag", s);
      if (sps->vui_seq_parameters.colour_description_present_flag) {
        sps->vui_seq_parameters.colour_primaries = read_u_v (8, "VUI colour_primaries", s);
        sps->vui_seq_parameters.transfer_characteristics = read_u_v (8, "VUI transfer_characteristics", s);
        sps->vui_seq_parameters.matrix_coefficients = read_u_v (8, "VUI matrix_coefficients", s);
        }
      }

    sps->vui_seq_parameters.chroma_location_info_present_flag = read_u_1 ("VUI chroma_loc_info_present_flag", s);
    if (sps->vui_seq_parameters.chroma_location_info_present_flag) {
      sps->vui_seq_parameters.chroma_sample_loc_type_top_field = read_ue_v ("VUI chroma_sample_loc_type_top_field", s);
      sps->vui_seq_parameters.chroma_sample_loc_type_bottom_field = read_ue_v ("VUI chroma_sample_loc_type_bottom_field", s);
      }

    sps->vui_seq_parameters.timing_info_present_flag = read_u_1 ("VUI timing_info_present_flag", s);
    if (sps->vui_seq_parameters.timing_info_present_flag) {
      sps->vui_seq_parameters.num_units_in_tick = read_u_v (32, "VUI num_units_in_tick", s);
      sps->vui_seq_parameters.time_scale = read_u_v (32,"VUI time_scale", s);
      sps->vui_seq_parameters.fixed_frame_rate_flag = read_u_1 ("VUI fixed_frame_rate_flag", s);
      }

    sps->vui_seq_parameters.nal_hrd_parameters_present_flag = read_u_1 ("VUI nal_hrd_parameters_present_flag", s);
    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
      readHRDParameters (p, &(sps->vui_seq_parameters.nal_hrd_parameters));

    sps->vui_seq_parameters.vcl_hrd_parameters_present_flag = read_u_1 ("VUI vcl_hrd_parameters_present_flag", s);

    if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
      readHRDParameters(p, &(sps->vui_seq_parameters.vcl_hrd_parameters));

    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag ||
        sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
      sps->vui_seq_parameters.low_delay_hrd_flag = read_u_1 ("VUI low_delay_hrd_flag", s);

    sps->vui_seq_parameters.pic_struct_present_flag = read_u_1 ("VUI pic_struct_present_flag   ", s);
    sps->vui_seq_parameters.bitstream_restriction_flag = read_u_1 ("VUI bitstream_restriction_flag", s);

    if (sps->vui_seq_parameters.bitstream_restriction_flag) {
      sps->vui_seq_parameters.motion_vectors_over_pic_boundaries_flag = read_u_1 ("VUI motion_vectors_over_pic_boundaries_flag", s);
      sps->vui_seq_parameters.max_bytes_per_pic_denom = read_ue_v ("VUI max_bytes_per_pic_denom", s);
      sps->vui_seq_parameters.max_bits_per_mb_denom = read_ue_v ("VUI max_bits_per_mb_denom", s);
      sps->vui_seq_parameters.log2_max_mv_length_horizontal = read_ue_v ("VUI log2_max_mv_length_horizontal", s);
      sps->vui_seq_parameters.log2_max_mv_length_vertical = read_ue_v ("VUI log2_max_mv_length_vertical", s);
      sps->vui_seq_parameters.num_reorder_frames = read_ue_v ("VUI num_reorder_frames", s);
      sps->vui_seq_parameters.max_dec_frame_buffering = read_ue_v ("VUI max_dec_frame_buffering", s);
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

  sps->profile_idc = read_u_v (8, "SPS profile_idc", s);
  if ((sps->profile_idc != BASELINE) &&
      (sps->profile_idc != MAIN) &&
      (sps->profile_idc != EXTENDED) &&
      (sps->profile_idc != FREXT_HP) &&
      (sps->profile_idc != FREXT_Hi10P) &&
      (sps->profile_idc != FREXT_Hi422) &&
      (sps->profile_idc != FREXT_Hi444) &&
      (sps->profile_idc != FREXT_CAVLC444)) {
    printf ("Invalid Profile IDC (%d) encountered. \n", sps->profile_idc);
    }

  sps->constrained_set0_flag = read_u_1 ("SPS constrained_set0_flag", s);
  sps->constrained_set1_flag = read_u_1 ("SPS constrained_set1_flag", s);
  sps->constrained_set2_flag = read_u_1 ("SPS constrained_set2_flag", s);
  sps->constrained_set3_flag = read_u_1 ("SPS constrained_set3_flag", s);

  reserved_zero = read_u_v (4, "SPS reserved_zero_4bits", s);

  if (reserved_zero != 0)
    printf ("Warning, reserved_zero flag not equal to 0. Possibly new constrained_setX flag introduced.\n");

  sps->level_idc = read_u_v (8, "SPS level_idc", s);
  sps->seq_parameter_set_id = read_ue_v ("SPS seq_parameter_set_id", s);

  // Fidelity Range Extensions stuff
  sps->chromaFormatIdc = 1;
  sps->bit_depth_luma_minus8 = 0;
  sps->bit_depth_chroma_minus8 = 0;
  sps->lossless_qpprime_flag = 0;
  sps->separate_colour_plane_flag = 0;

  if ((sps->profile_idc == FREXT_HP   ) ||
      (sps->profile_idc == FREXT_Hi10P) ||
      (sps->profile_idc == FREXT_Hi422) ||
      (sps->profile_idc == FREXT_Hi444) ||
      (sps->profile_idc == FREXT_CAVLC444)) {
    sps->chromaFormatIdc = read_ue_v ("SPS chromaFormatIdc", s);
    if (sps->chromaFormatIdc == YUV444)
      sps->separate_colour_plane_flag = read_u_1 ("SPS separate_colour_plane_flag", s);

    sps->bit_depth_luma_minus8 = read_ue_v ("SPS bit_depth_luma_minus8", s);
    sps->bit_depth_chroma_minus8 = read_ue_v ("SPS bit_depth_chroma_minus8", s);
    if ((sps->bit_depth_luma_minus8+8 > sizeof(sPixel)*8) ||
        (sps->bit_depth_chroma_minus8+8> sizeof(sPixel)*8))
      error ("Source picture has higher bit depth than sPixel data type", 500);

    sps->lossless_qpprime_flag = read_u_1 ("SPS lossless_qpprime_y_zero_flag", s);

    sps->seq_scaling_matrix_present_flag = read_u_1 ("SPS seq_scaling_matrix_present_flag", s);
    if (sps->seq_scaling_matrix_present_flag) {
      n_ScalingList = (sps->chromaFormatIdc != YUV444) ? 8 : 12;
      for (i = 0; i < n_ScalingList; i++) {
        sps->seq_scaling_list_present_flag[i] = read_u_1 ("SPS seq_scaling_list_present_flag", s);
        if (sps->seq_scaling_list_present_flag[i]) {
          if (i < 6)
            scalingList (sps->ScalingList4x4[i], 16, &sps->UseDefaultScalingMatrix4x4Flag[i], s);
          else
            scalingList (sps->ScalingList8x8[i-6], 64, &sps->UseDefaultScalingMatrix8x8Flag[i-6], s);
          }
        }
      }
    }

  sps->log2_max_frame_num_minus4 = read_ue_v ("SPS log2_max_frame_num_minus4", s);
  sps->pic_order_cnt_type = read_ue_v ("SPS pic_order_cnt_type", s);

  if (sps->pic_order_cnt_type == 0)
    sps->log2_max_pic_order_cnt_lsb_minus4 = read_ue_v ("SPS log2_max_pic_order_cnt_lsb_minus4", s);
  else if (sps->pic_order_cnt_type == 1) {
    sps->delta_pic_order_always_zero_flag = read_u_1 ("SPS delta_pic_order_always_zero_flag", s);
    sps->offset_for_non_ref_pic = read_se_v ("SPS offset_for_non_ref_pic", s);
    sps->offset_for_top_to_bottom_field = read_se_v ("SPS offset_for_top_to_bottom_field", s);
    sps->num_ref_frames_in_pic_order_cnt_cycle = read_ue_v ("SPS num_ref_frames_in_pic_order_cnt_cycle", s);
    for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      sps->offset_for_ref_frame[i] = read_se_v ("SPS offset_for_ref_frame[i]", s);
    }

  sps->num_ref_frames = read_ue_v ("SPS num_ref_frames", s);
  sps->gaps_in_frame_num_value_allowed_flag = read_u_1 ("SPS gaps_in_frame_num_value_allowed_flag", s);
  sps->pic_width_in_mbs_minus1 = read_ue_v ("SPS pic_width_in_mbs_minus1", s);
  sps->pic_height_in_map_units_minus1 = read_ue_v ("SPS pic_height_in_map_units_minus1", s);
  sps->frame_mbs_only_flag = read_u_1 ("SPS frame_mbs_only_flag", s);
  if (!sps->frame_mbs_only_flag)
    sps->mb_adaptive_frame_field_flag = read_u_1 ("SPS mb_adaptive_frame_field_flag", s);

  sps->direct_8x8_inference_flag = read_u_1 ("SPS direct_8x8_inference_flag", s);

  sps->frame_cropping_flag = read_u_1 ("SPS frame_cropping_flag", s);
  if (sps->frame_cropping_flag) {
    sps->frame_crop_left_offset = read_ue_v ("SPS frame_crop_left_offset", s);
    sps->frame_crop_right_offset = read_ue_v ("SPS frame_crop_right_offset", s);
    sps->frame_crop_top_offset = read_ue_v ("SPS frame_crop_top_offset", s);
    sps->frame_crop_bottom_offset = read_ue_v ("SPS frame_crop_bottom_offset", s);
    }
  sps->vui_parameters_present_flag = (Boolean) read_u_1 ("SPS vui_parameters_present_flag", s);

  initVUI (sps);
  readVUI (p, sps);

  sps->Valid = TRUE;
  }
//}}}

//{{{
void makeSPSavailable (sVidParam* vidParam, int id, sSPS* sps) {

  assert (sps->Valid == TRUE);
  memcpy (&vidParam->SeqParSet[id], sps, sizeof (sSPS));
  }
//}}}
//{{{
void processSPS (sVidParam* vidParam, sNalu* nalu) {

  sDataPartition* dp = allocPartition (1);
  dp->bitstream->ei_flag = 0;
  dp->bitstream->read_len = dp->bitstream->frame_bitoffset = 0;
  memcpy (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->code_len = dp->bitstream->bitstream_length = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);

  sSPS* sps = allocSPS();
  interpretSPS (vidParam, dp, sps);

  if (sps->Valid) {
    if (vidParam->activeSPS) {
      if (sps->seq_parameter_set_id == vidParam->activeSPS->seq_parameter_set_id) {
        if (!spsIsEqual (sps, vidParam->activeSPS))   {
          if (vidParam->picture)
            exitPicture (vidParam, &vidParam->picture);
          vidParam->activeSPS=NULL;
          }
        }
      }

    makeSPSavailable (vidParam, sps->seq_parameter_set_id, sps);

    vidParam->profile_idc = sps->profile_idc;
    vidParam->separate_colour_plane_flag = sps->separate_colour_plane_flag;
    if (vidParam->separate_colour_plane_flag )
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

    if (vidParam->dpbLayerId==0 && is_BL_profile(sps->profile_idc) && !vidParam->dpbLayer[0]->init_done) {
      setCodingParam (sps, vidParam->codingParam[0]);
      setupLayerInfo ( vidParam, sps, vidParam->layerParam[0]);
      }
    setGlobalCodingProgram (vidParam, vidParam->codingParam[vidParam->dpbLayerId]);

    initGlobalBuffers (vidParam, 0);
    if (!vidParam->no_output_of_prior_pics_flag)
      flush_dpb (vidParam->dpbLayer[0]);

    initDpb (vidParam, vidParam->dpbLayer[0], 0);

    // enable error conceal
    ercInit (vidParam, vidParam->width, vidParam->height, 1);
    if (vidParam->picture) {
      ercReset (vidParam->ercErrorVar, vidParam->picSizeInMbs, vidParam->picSizeInMbs, vidParam->picture->size_x);
      vidParam->ercMvPerMb = 0;
      }
    }

  resetFormatInfo (sps, vidParam, &inputParam->source, &inputParam->output);
  }
//}}}

// PPS
//{{{
static int ppsIsEqual (sPPS* pps1, sPPS* pps2) {

  if ((!pps1->Valid) || (!pps2->Valid))
    return 0;

  int equal = 1;
  equal &= (pps1->pic_parameter_set_id == pps2->pic_parameter_set_id);
  equal &= (pps1->seq_parameter_set_id == pps2->seq_parameter_set_id);
  equal &= (pps1->entropy_coding_mode_flag == pps2->entropy_coding_mode_flag);
  equal &= (pps1->bottom_field_pic_order_in_frame_present_flag == pps2->bottom_field_pic_order_in_frame_present_flag);
  equal &= (pps1->num_slice_groups_minus1 == pps2->num_slice_groups_minus1);
  if (!equal)
    return equal;

  if (pps1->num_slice_groups_minus1 > 0) {
    equal &= (pps1->slice_group_map_type == pps2->slice_group_map_type);
    if (!equal)
      return equal;
    if (pps1->slice_group_map_type == 0) {
      for (unsigned i = 0; i <= pps1->num_slice_groups_minus1; i++)
        equal &= (pps1->run_length_minus1[i] == pps2->run_length_minus1[i]);
      }
    else if (pps1->slice_group_map_type == 2) {
      for (unsigned i = 0; i < pps1->num_slice_groups_minus1; i++) {
        equal &= (pps1->top_left[i] == pps2->top_left[i]);
        equal &= (pps1->bottom_right[i] == pps2->bottom_right[i]);
        }
      }
    else if (pps1->slice_group_map_type == 3 ||
             pps1->slice_group_map_type == 4 ||
             pps1->slice_group_map_type == 5) {
      equal &= (pps1->slice_group_change_direction_flag == pps2->slice_group_change_direction_flag);
      equal &= (pps1->slice_group_change_rate_minus1 == pps2->slice_group_change_rate_minus1);
      }
    else if (pps1->slice_group_map_type == 6) {
      equal &= (pps1->pic_size_in_map_units_minus1 == pps2->pic_size_in_map_units_minus1);
      if (!equal)
        return equal;
      for (unsigned i = 0; i <= pps1->pic_size_in_map_units_minus1; i++)
        equal &= (pps1->slice_group_id[i] == pps2->slice_group_id[i]);
      }
    }

  equal &= (pps1->num_ref_idx_l0_default_active_minus1 == pps2->num_ref_idx_l0_default_active_minus1);
  equal &= (pps1->num_ref_idx_l1_default_active_minus1 == pps2->num_ref_idx_l1_default_active_minus1);
  equal &= (pps1->weighted_pred_flag == pps2->weighted_pred_flag);
  equal &= (pps1->weighted_bipred_idc == pps2->weighted_bipred_idc);
  equal &= (pps1->pic_init_qp_minus26 == pps2->pic_init_qp_minus26);
  equal &= (pps1->pic_init_qs_minus26 == pps2->pic_init_qs_minus26);
  equal &= (pps1->chroma_qp_index_offset == pps2->chroma_qp_index_offset);
  equal &= (pps1->deblocking_filter_control_present_flag == pps2->deblocking_filter_control_present_flag);
  equal &= (pps1->constrained_intra_pred_flag == pps2->constrained_intra_pred_flag);
  equal &= (pps1->redundant_pic_cnt_present_flag == pps2->redundant_pic_cnt_present_flag);
  if (!equal)
    return equal;

  // Fidelity Range Extensions
  equal &= (pps1->transform_8x8_mode_flag == pps2->transform_8x8_mode_flag);
  equal &= (pps1->pic_scaling_matrix_present_flag == pps2->pic_scaling_matrix_present_flag);
  if (pps1->pic_scaling_matrix_present_flag) {
    for (unsigned i = 0; i < (6 + ((unsigned)pps1->transform_8x8_mode_flag << 1)); i++) {
      equal &= (pps1->pic_scaling_list_present_flag[i] == pps2->pic_scaling_list_present_flag[i]);
      if (pps1->pic_scaling_list_present_flag[i]) {
        if (i < 6)
          for (int j = 0; j < 16; j++)
            equal &= (pps1->ScalingList4x4[i][j] == pps2->ScalingList4x4[i][j]);
        else
          for (int j = 0; j < 64; j++)
            equal &= (pps1->ScalingList8x8[i-6][j] == pps2->ScalingList8x8[i-6][j]);
        }
      }
    }

  equal &= (pps1->second_chroma_qp_index_offset == pps2->second_chroma_qp_index_offset);
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

  pps->pic_parameter_set_id = read_ue_v ("PPS pic_parameter_set_id", s);
  pps->seq_parameter_set_id = read_ue_v ("PPS seq_parameter_set_id", s);
  pps->entropy_coding_mode_flag = read_u_1 ("PPS entropy_coding_mode_flag", s);

  //! Note: as per JVT-F078 the following bit is unconditional.  If F078 is not accepted, then
  //! one has to fetch the correct SPS to check whether the bit is present (hopefully there is
  //! no consistency problem :-(
  //! The current encoder code handles this in the same way.  When you change this, don't forget
  //! the encoder!  StW, 12/8/02
  pps->bottom_field_pic_order_in_frame_present_flag =
    read_u_1 ("PPS bottom_field_pic_order_in_frame_present_flag", s);
  pps->num_slice_groups_minus1 = read_ue_v ("PPS num_slice_groups_minus1", s);

  if (pps->num_slice_groups_minus1 > 0) {
    //{{{  FMO
    pps->slice_group_map_type = read_ue_v ("PPS slice_group_map_type", s);
    if (pps->slice_group_map_type == 0) {
      for (unsigned i = 0; i <= pps->num_slice_groups_minus1; i++)
        pps->run_length_minus1 [i] = read_ue_v ("PPS run_length_minus1 [i]", s);
      }
    else if (pps->slice_group_map_type == 2) {
      for (unsigned i = 0; i < pps->num_slice_groups_minus1; i++) {
        //! JVT-F078: avoid reference of SPS by using ue(v) instead of u(v)
        pps->top_left [i] = read_ue_v ("PPS top_left [i]", s);
        pps->bottom_right [i] = read_ue_v ("PPS bottom_right [i]", s);
        }
      }
    else if (pps->slice_group_map_type == 3 ||
             pps->slice_group_map_type == 4 ||
             pps->slice_group_map_type == 5) {
      pps->slice_group_change_direction_flag =
        read_u_1 ("PPS slice_group_change_direction_flag", s);
      pps->slice_group_change_rate_minus1 =
        read_ue_v ("PPS slice_group_change_rate_minus1", s);
      }
    else if (pps->slice_group_map_type == 6) {
      if (pps->num_slice_groups_minus1+1 >4)
        NumberBitsPerSliceGroupId = 3;
      else if (pps->num_slice_groups_minus1+1 > 2)
        NumberBitsPerSliceGroupId = 2;
      else
        NumberBitsPerSliceGroupId = 1;
      pps->pic_size_in_map_units_minus1      = read_ue_v ("PPS pic_size_in_map_units_minus1"               , s);
      if ((pps->slice_group_id = calloc (pps->pic_size_in_map_units_minus1+1, 1)) == NULL)
        no_mem_exit ("InterpretPPS slice_group_id");
      for (unsigned i = 0; i <= pps->pic_size_in_map_units_minus1; i++)
        pps->slice_group_id[i] =
          (byte)read_u_v (NumberBitsPerSliceGroupId, "slice_group_id[i]", s);
      }
    }
    //}}}

  pps->num_ref_idx_l0_default_active_minus1 = read_ue_v ("PPS num_ref_idx_l0_default_active_minus1", s);
  pps->num_ref_idx_l1_default_active_minus1 = read_ue_v ("PPS num_ref_idx_l1_default_active_minus1", s);

  pps->weighted_pred_flag = read_u_1 ("PPS weighted_pred_flag", s);
  pps->weighted_bipred_idc = read_u_v ( 2, "PPS weighted_bipred_idc", s);

  pps->pic_init_qp_minus26 = read_se_v ("PPS pic_init_qp_minus26", s);
  pps->pic_init_qs_minus26 = read_se_v ("PPS pic_init_qs_minus26", s);

  pps->chroma_qp_index_offset = read_se_v ("PPS chroma_qp_index_offset"   , s);
  pps->deblocking_filter_control_present_flag =
    read_u_1 ("PPS deblocking_filter_control_present_flag" , s);

  pps->constrained_intra_pred_flag = read_u_1 ("PPS constrained_intra_pred_flag", s);
  pps->redundant_pic_cnt_present_flag =
    read_u_1 ("PPS redundant_pic_cnt_present_flag", s);

  if (more_rbsp_data (s->streamBuffer, s->frame_bitoffset,s->bitstream_length)) {
    //{{{  more_data_in_rbsp
    // Fidelity Range Extensions Stuff
    pps->transform_8x8_mode_flag = read_u_1 ("PPS transform_8x8_mode_flag", s);
    pps->pic_scaling_matrix_present_flag = read_u_1 ("PPS pic_scaling_matrix_present_flag", s);

    if (pps->pic_scaling_matrix_present_flag) {
      chromaFormatIdc = vidParam->SeqParSet[pps->seq_parameter_set_id].chromaFormatIdc;
      n_ScalingList = 6 + ((chromaFormatIdc != YUV444) ? 2 : 6) * pps->transform_8x8_mode_flag;
      for (unsigned i = 0; i < n_ScalingList; i++) {
        pps->pic_scaling_list_present_flag[i]=
          read_u_1 ("PPS pic_scaling_list_present_flag", s);
        if (pps->pic_scaling_list_present_flag[i]) {
          if (i < 6)
            scalingList (pps->ScalingList4x4[i], 16, &pps->UseDefaultScalingMatrix4x4Flag[i], s);
          else
            scalingList (pps->ScalingList8x8[i-6], 64, &pps->UseDefaultScalingMatrix8x8Flag[i-6], s);
          }
        }
      }
    pps->second_chroma_qp_index_offset = read_se_v ("PPS second_chroma_qp_index_offset", s);
    }
    //}}}
  else
    pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;

  pps->Valid = TRUE;
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

  p->slice_group_id = NULL;
  return p;
  }
//}}}
//{{{
 void freePPS (sPPS* pps) {

   assert (pps != NULL);
   if (pps->slice_group_id != NULL)
     free (pps->slice_group_id);
   free (pps);
   }
//}}}
//{{{
void makePPSavailable (sVidParam* vidParam, int id, sPPS* pps) {

  if (vidParam->PicParSet[id].Valid && vidParam->PicParSet[id].slice_group_id)
    free (vidParam->PicParSet[id].slice_group_id);

  memcpy (&vidParam->PicParSet[id], pps, sizeof (sPPS));

  // we can simply use the memory provided with the pps. the PPS is destroyed after this function
  // call and will not try to free if pps->slice_group_id == NULL
  vidParam->PicParSet[id].slice_group_id = pps->slice_group_id;
  pps->slice_group_id = NULL;
  }
//}}}
//{{{
void cleanUpPPS (sVidParam* vidParam) {

  for (int i = 0; i < MAX_PPS; i++) {
    if (vidParam->PicParSet[i].Valid == TRUE && vidParam->PicParSet[i].slice_group_id != NULL)
      free (vidParam->PicParSet[i].slice_group_id);
    vidParam->PicParSet[i].Valid = FALSE;
    }
  }
//}}}
//{{{
void processPPS (sVidParam* vidParam, sNalu* nalu) {


  sDataPartition* dp = allocPartition (1);
  dp->bitstream->ei_flag = 0;
  dp->bitstream->read_len = dp->bitstream->frame_bitoffset = 0;
  memcpy (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->code_len = dp->bitstream->bitstream_length = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);

  sPPS* pps = allocPPS();
  interpretPPS (vidParam, dp, pps);

  if (vidParam->activePPS) {
    if (pps->pic_parameter_set_id == vidParam->activePPS->pic_parameter_set_id) {
      if (!ppsIsEqual (pps, vidParam->activePPS)) {
        // copy to next PPS;
        memcpy (vidParam->nextPPS, vidParam->activePPS, sizeof (sPPS));
        if (vidParam->picture)
          exitPicture (vidParam, &vidParam->picture);
        vidParam->activePPS = NULL;
        }
      }
    }

  makePPSavailable (vidParam, pps->pic_parameter_set_id, pps);
  freePartition (dp, 1);
  freePPS (pps);
  }
//}}}

//{{{
void useParameterSet (sSlice* curSlice) {

  sVidParam* vidParam = curSlice->vidParam;
  int PicParsetId = curSlice->pic_parameter_set_id;

  sPPS* pps = &vidParam->PicParSet[PicParsetId];
  sSPS* sps = &vidParam->SeqParSet[pps->seq_parameter_set_id];

  if (pps->Valid != TRUE)
    printf ("Trying to use an invalid (uninitialized) Picture Parameter Set with ID %d, expect the unexpected...\n", PicParsetId);

  if (sps->Valid != TRUE)
    printf ("PicParset %d references uninitialized) SPS ID %d, unexpected\n",
            PicParsetId, (int) pps->seq_parameter_set_id);

  // In theory, and with a well-designed software, the lines above are everything necessary.
  // In practice, we need to patch many values
  // in vidParam-> (but no more in inputParam-> -- these have been taken care of)
  // Set Sequence Parameter Stuff first
  if ((int) sps->pic_order_cnt_type < 0 || sps->pic_order_cnt_type > 2) {
    printf ("invalid sps->pic_order_cnt_type = %d\n", (int) sps->pic_order_cnt_type);
    error ("pic_order_cnt_type != 1", -1000);
    }

  if (sps->pic_order_cnt_type == 1)
    if (sps->num_ref_frames_in_pic_order_cnt_cycle >= MAX_NUM_REF_FRAMES_PIC_ORDER)
      error ("num_ref_frames_in_pic_order_cnt_cycle too large",-1011);

  vidParam->dpbLayerId = curSlice->layerId;
  activateSPS (vidParam, sps);
  activatePPS (vidParam, pps);

  // curSlice->dp_mode is set by read_new_slice (NALU first byte available there)
  if (pps->entropy_coding_mode_flag == (Boolean)CAVLC) {
    curSlice->nal_startcode_follows = uvlc_startcode_follows;
    for (int i = 0; i < 3; i++)
      curSlice->partArr[i].readsSyntaxElement = readsSyntaxElement_UVLC;
    }
  else {
    curSlice->nal_startcode_follows = cabac_startcode_follows;
    for (int i = 0; i < 3; i++)
      curSlice->partArr[i].readsSyntaxElement = readsSyntaxElement_CABAC;
    }
  vidParam->type = curSlice->slice_type;
  }
//}}}
