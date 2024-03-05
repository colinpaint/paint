//{{{
/*!
** **********************************************************************
 *  \file
 *     parset.c
 *  \brief
 *     Parameter Sets
 *  \author
 *     Main contributors (see contributors.h for copyright, address and affiliation details)
 *     - Stephan Wenger          <stewe@cs.tu-berlin.de>
 *
** *********************************************************************
 */
//}}}
//{{{  includes
#include "global.h"
#include "memalloc.h"

#include "image.h"
#include "parsetcommon.h"
#include "parset.h"
#include "nalu.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "mbuffer.h"
#include "erc.h"

#if TRACE
#define SYMTRACESTRING(s) strncpy(sym->tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // do nothing
#endif
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
static void updateMaxValue (sFrameFormat *format)
{
  format->max_value[0] = (1 << format->bit_depth[0]) - 1;
  format->max_value_sq[0] = format->max_value[0] * format->max_value[0];
  format->max_value[1] = (1 << format->bit_depth[1]) - 1;
  format->max_value_sq[1] = format->max_value[1] * format->max_value[1];
  format->max_value[2] = (1 << format->bit_depth[2]) - 1;
  format->max_value_sq[2] = format->max_value[2] * format->max_value[2];
}
//}}}
//{{{
static void setup_layer_info (sVidParam* vidParam, sSPSrbsp *sps, LayerParameters *p_Lps)
{
  int layer_id = p_Lps->layer_id;
  p_Lps->vidParam = vidParam;
  p_Lps->p_Cps = vidParam->p_EncodePar[layer_id];
  p_Lps->p_SPS = sps;
  p_Lps->dpb = vidParam->p_Dpb_layer[layer_id];
}
//}}}
//{{{
static void set_coding_par (sSPSrbsp *sps, sCodingParams *cps) {

  // maximum vertical motion vector range in luma quarter pixel units
  cps->profile_idc = sps->profile_idc;
  cps->lossless_qpprime_flag = sps->lossless_qpprime_flag;
  if (sps->level_idc <= 10)
    cps->max_vmv_r = 64 * 4;
  else if (sps->level_idc <= 20)
    cps->max_vmv_r = 128 * 4;
  else if (sps->level_idc <= 30)
    cps->max_vmv_r = 256 * 4;
  else
    cps->max_vmv_r = 512 * 4; // 512 pixels in quarter pixels

  // Fidelity Range Extensions stuff (part 1)
  cps->bitdepth_chroma = 0;
  cps->width_cr = 0;
  cps->height_cr = 0;
  cps->bitdepth_luma       = (short) (sps->bit_depth_luma_minus8 + 8);
  cps->bitdepth_scale[0]   = 1 << sps->bit_depth_luma_minus8;
  if (sps->chroma_format_idc != YUV400) {
    cps->bitdepth_chroma   = (short) (sps->bit_depth_chroma_minus8 + 8);
    cps->bitdepth_scale[1] = 1 << sps->bit_depth_chroma_minus8;
    }

  cps->max_frame_num = 1<<(sps->log2_max_frame_num_minus4+4);
  cps->PicWidthInMbs = (sps->pic_width_in_mbs_minus1 +1);
  cps->PicHeightInMapUnits = (sps->pic_height_in_map_units_minus1 +1);
  cps->FrameHeightInMbs = ( 2 - sps->frame_mbs_only_flag ) * cps->PicHeightInMapUnits;
  cps->FrameSizeInMbs = cps->PicWidthInMbs * cps->FrameHeightInMbs;

  cps->yuv_format=sps->chroma_format_idc;
  cps->separate_colour_plane_flag = sps->separate_colour_plane_flag;
  if (cps->separate_colour_plane_flag )
    cps->ChromaArrayType = 0;
  else
    cps->ChromaArrayType = sps->chroma_format_idc;

  cps->width = cps->PicWidthInMbs * MB_BLOCK_SIZE;
  cps->height = cps->FrameHeightInMbs * MB_BLOCK_SIZE;

  cps->iLumaPadX = MCBUF_LUMA_PAD_X;
  cps->iLumaPadY = MCBUF_LUMA_PAD_Y;
  cps->iChromaPadX = MCBUF_CHROMA_PAD_X;
  cps->iChromaPadY = MCBUF_CHROMA_PAD_Y;
  if (sps->chroma_format_idc == YUV420) {
    cps->width_cr  = (cps->width  >> 1);
    cps->height_cr = (cps->height >> 1);
    }
  else if (sps->chroma_format_idc == YUV422) {
    cps->width_cr  = (cps->width >> 1);
    cps->height_cr = cps->height;
    cps->iChromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (sps->chroma_format_idc == YUV444) {
    cps->width_cr = cps->width;
    cps->height_cr = cps->height;
    cps->iChromaPadX = cps->iLumaPadX;
    cps->iChromaPadY = cps->iLumaPadY;
    }
  //pel bitdepth init
  cps->bitdepth_luma_qp_scale   = 6 * (cps->bitdepth_luma - 8);

  if (cps->bitdepth_luma > cps->bitdepth_chroma || sps->chroma_format_idc == YUV400)
    cps->pic_unit_bitsize_on_disk = (cps->bitdepth_luma > 8)? 16:8;
  else
    cps->pic_unit_bitsize_on_disk = (cps->bitdepth_chroma > 8)? 16:8;
  cps->dc_pred_value_comp[0] = 1<<(cps->bitdepth_luma - 1);
  cps->max_pel_value_comp[0] = (1<<cps->bitdepth_luma) - 1;
  cps->mb_size[0][0] = cps->mb_size[0][1] = MB_BLOCK_SIZE;

  if (sps->chroma_format_idc != YUV400) {
    // for chrominance part
    cps->bitdepth_chroma_qp_scale = 6 * (cps->bitdepth_chroma - 8);
    cps->dc_pred_value_comp[1] = (1 << (cps->bitdepth_chroma - 1));
    cps->dc_pred_value_comp[2] = cps->dc_pred_value_comp[1];
    cps->max_pel_value_comp[1] = (1 << cps->bitdepth_chroma) - 1;
    cps->max_pel_value_comp[2] = (1 << cps->bitdepth_chroma) - 1;
    cps->num_blk8x8_uv = (1 << sps->chroma_format_idc) & (~(0x1));
    cps->num_uv_blocks = (cps->num_blk8x8_uv >> 1);
    cps->num_cdc_coeff = (cps->num_blk8x8_uv << 1);
    cps->mb_size[1][0] = cps->mb_size[2][0] = cps->mb_cr_size_x  = (sps->chroma_format_idc==YUV420 || sps->chroma_format_idc==YUV422)?  8 : 16;
    cps->mb_size[1][1] = cps->mb_size[2][1] = cps->mb_cr_size_y  = (sps->chroma_format_idc==YUV444 || sps->chroma_format_idc==YUV422)? 16 :  8;

    cps->subpel_x    = cps->mb_cr_size_x == 8 ? 7 : 3;
    cps->subpel_y    = cps->mb_cr_size_y == 8 ? 7 : 3;
    cps->shiftpel_x  = cps->mb_cr_size_x == 8 ? 3 : 2;
    cps->shiftpel_y  = cps->mb_cr_size_y == 8 ? 3 : 2;
    cps->total_scale = cps->shiftpel_x + cps->shiftpel_y;
    }
  else {
    cps->bitdepth_chroma_qp_scale = 0;
    cps->max_pel_value_comp[1] = 0;
    cps->max_pel_value_comp[2] = 0;
    cps->num_blk8x8_uv = 0;
    cps->num_uv_blocks = 0;
    cps->num_cdc_coeff = 0;
    cps->mb_size[1][0] = cps->mb_size[2][0] = cps->mb_cr_size_x  = 0;
    cps->mb_size[1][1] = cps->mb_size[2][1] = cps->mb_cr_size_y  = 0;
    cps->subpel_x = 0;
    cps->subpel_y = 0;
    cps->shiftpel_x = 0;
    cps->shiftpel_y = 0;
    cps->total_scale = 0;
    }

  cps->mb_cr_size = cps->mb_cr_size_x * cps->mb_cr_size_y;
  cps->mb_size_blk[0][0] = cps->mb_size_blk[0][1] = cps->mb_size[0][0] >> 2;
  cps->mb_size_blk[1][0] = cps->mb_size_blk[2][0] = cps->mb_size[1][0] >> 2;
  cps->mb_size_blk[1][1] = cps->mb_size_blk[2][1] = cps->mb_size[1][1] >> 2;

  cps->mb_size_shift[0][0] = cps->mb_size_shift[0][1] = CeilLog2_sf (cps->mb_size[0][0]);
  cps->mb_size_shift[1][0] = cps->mb_size_shift[2][0] = CeilLog2_sf (cps->mb_size[1][0]);
  cps->mb_size_shift[1][1] = cps->mb_size_shift[2][1] = CeilLog2_sf (cps->mb_size[1][1]);

  cps->rgb_output = (sps->vui_seq_parameters.matrix_coefficients==0);
  }
//}}}
//{{{
static void reset_format_info (sSPSrbsp *sps, sVidParam* vidParam,
                               sFrameFormat *source, sFrameFormat *output) {

  static const int SubWidthC  [4]= { 1, 2, 2, 1};
  static const int SubHeightC [4]= { 1, 2, 1, 1};

  // cropping for luma
  int crop_left, crop_right;
  int crop_top, crop_bottom;
  if (sps->frame_cropping_flag) {
    crop_left = SubWidthC [sps->chroma_format_idc] * sps->frame_crop_left_offset;
    crop_right = SubWidthC [sps->chroma_format_idc] * sps->frame_crop_right_offset;
    crop_top = SubHeightC[sps->chroma_format_idc] * ( 2 - sps->frame_mbs_only_flag ) *  sps->frame_crop_top_offset;
    crop_bottom = SubHeightC[sps->chroma_format_idc] * ( 2 - sps->frame_mbs_only_flag ) *  sps->frame_crop_bottom_offset;
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

  InputParameters* p_Inp = vidParam->p_Inp;
  if ((sps->chroma_format_idc == YUV400) && p_Inp->write_uv) {
    source->width[1] = (source->width[0] >> 1);
    source->width[2] = source->width[1];
    source->height[1] = (source->height[0] >> 1);
    source->height[2] = source->height[1];
    }
  else {
    source->width[1] = vidParam->width_cr - crop_left - crop_right;
    source->width[2] = source->width[1];
    source->height[1] = vidParam->height_cr - crop_top - crop_bottom;
    source->height[2] = source->height[1];
    }

  output->width[0] = vidParam->width;
  source->width[1] = vidParam->width_cr;
  source->width[2] = vidParam->width_cr;
  output->height[0] = vidParam->height;
  output->height[1] = vidParam->height_cr;
  output->height[2] = vidParam->height_cr;

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
  output->yuv_format = source->yuv_format = (ColorFormat) sps->chroma_format_idc;

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

  if (vidParam->first_sps == TRUE) {
    vidParam->first_sps = FALSE;
    printf ("Profile IDC: %d %dx%d %dx%d ",
            sps->profile_idc, source->width[0], source->height[0], vidParam->width, vidParam->height);
    if (vidParam->yuv_format == YUV400)
      printf ("4:0:0");
    else if (vidParam->yuv_format == YUV420)
      printf ("4:2:0");
    else if (vidParam->yuv_format == YUV422)
      printf ("4:2:2");
    else
      printf ("4:4:4");
    printf (" %d:%d:%d\n", source->bit_depth[0], source->bit_depth[1], source->bit_depth[2]);
    }
  }
//}}}

// SPS
//{{{
static sSPSrbsp* AllocSPS() {

   sSPSrbsp* p = calloc (1, sizeof (sSPSrbsp));
   if (p == NULL)
     no_mem_exit ("AllocSPS: SPS");

   return p;
   }
//}}}
//{{{
static void freeSPS (sSPSrbsp *sps) {

  assert (sps != NULL);
  free (sps);
  }
//}}}
//{{{
static int spsIsEqual (sSPSrbsp *sps1, sSPSrbsp *sps2) {

  unsigned i;
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

  if( sps1->pic_order_cnt_type == 0 )
    equal &= (sps1->log2_max_pic_order_cnt_lsb_minus4 == sps2->log2_max_pic_order_cnt_lsb_minus4);
  else if( sps1->pic_order_cnt_type == 1 ) {
    equal &= (sps1->delta_pic_order_always_zero_flag == sps2->delta_pic_order_always_zero_flag);
    equal &= (sps1->offset_for_non_ref_pic == sps2->offset_for_non_ref_pic);
    equal &= (sps1->offset_for_top_to_bottom_field == sps2->offset_for_top_to_bottom_field);
    equal &= (sps1->num_ref_frames_in_pic_order_cnt_cycle == sps2->num_ref_frames_in_pic_order_cnt_cycle);
    if (!equal)
      return equal;
    for ( i = 0 ; i< sps1->num_ref_frames_in_pic_order_cnt_cycle ;i ++)
      equal &= (sps1->offset_for_ref_frame[i] == sps2->offset_for_ref_frame[i]);
    }

  equal &= (sps1->num_ref_frames == sps2->num_ref_frames);
  equal &= (sps1->gaps_in_frame_num_value_allowed_flag == sps2->gaps_in_frame_num_value_allowed_flag);
  equal &= (sps1->pic_width_in_mbs_minus1 == sps2->pic_width_in_mbs_minus1);
  equal &= (sps1->pic_height_in_map_units_minus1 == sps2->pic_height_in_map_units_minus1);
  equal &= (sps1->frame_mbs_only_flag == sps2->frame_mbs_only_flag);

  if (!equal) return
    equal;
  if (!sps1->frame_mbs_only_flag )
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
static void scaling_List (int *scalingList, int sizeOfScalingList, Boolean *UseDefaultScalingMatrix, Bitstream *s)
{
  int j, scanj;
  int delta_scale, lastScale, nextScale;

  lastScale = 8;
  nextScale = 8;

  for (j = 0; j < sizeOfScalingList; j++) {
    scanj = (sizeOfScalingList==16) ? ZZ_SCAN[j]:ZZ_SCAN8[j];
    if (nextScale != 0) {
      delta_scale = read_se_v (   "   : delta_sl   "                           , s, &gDecoder->UsedBits);
      nextScale = (lastScale + delta_scale + 256) % 256;
      *UseDefaultScalingMatrix = (Boolean) (scanj==0 && nextScale==0);
      }

    scalingList[scanj] = (nextScale==0) ? lastScale:nextScale;
    lastScale = scalingList[scanj];
    }
  }
//}}}
//{{{
static void initVUI (sSPSrbsp *sps) {
  sps->vui_seq_parameters.matrix_coefficients = 2;
  }
//}}}
//{{{
static int readHRDParameters (sDataPartition *p, sHRDparams *hrd) {

  Bitstream *s = p->bitstream;
  hrd->cpb_cnt_minus1 = read_ue_v ("VUI: cpb_cnt_minus1", s, &gDecoder->UsedBits);
  hrd->bit_rate_scale = read_u_v (4, "VUI: bit_rate_scale", s, &gDecoder->UsedBits);
  hrd->cpb_size_scale = read_u_v (4, "VUI: cpb_size_scale", s, &gDecoder->UsedBits);

  unsigned int SchedSelIdx;
  for (SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++ ) {
    hrd->bit_rate_value_minus1[ SchedSelIdx] = read_ue_v ("VUI: bit_rate_value_minus1", s, &gDecoder->UsedBits);
    hrd->cpb_size_value_minus1[ SchedSelIdx] = read_ue_v ("VUI: cpb_size_value_minus1", s, &gDecoder->UsedBits);
    hrd->cbr_flag[ SchedSelIdx ] = read_u_1  ("VUI: cbr_flag", s, &gDecoder->UsedBits);
    }

  hrd->initial_cpb_removal_delay_length_minus1 =
    read_u_v (5, "VUI: initial_cpb_removal_delay_length_minus1", s, &gDecoder->UsedBits);
  hrd->cpb_removal_delay_length_minus1 =
    read_u_v (5, "VUI: cpb_removal_delay_length_minus1", s, &gDecoder->UsedBits);
  hrd->dpb_output_delay_length_minus1 =
    read_u_v (5, "VUI: dpb_output_delay_length_minus1", s, &gDecoder->UsedBits);
  hrd->time_offset_length = read_u_v (5, "VUI: time_offset_length", s, &gDecoder->UsedBits);

  return 0;
  }
//}}}
//{{{
static int readVUI (sDataPartition* p, sSPSrbsp* sps) {

  Bitstream* s = p->bitstream;
  if (sps->vui_parameters_present_flag) {
    sps->vui_seq_parameters.aspect_ratio_info_present_flag =
      read_u_1 ("VUI: aspect_ratio_info_present_flag", s, &gDecoder->UsedBits);
    if (sps->vui_seq_parameters.aspect_ratio_info_present_flag) {
      sps->vui_seq_parameters.aspect_ratio_idc =
        read_u_v ( 8, "VUI: aspect_ratio_idc", s, &gDecoder->UsedBits);
      if (255 == sps->vui_seq_parameters.aspect_ratio_idc) {
        sps->vui_seq_parameters.sar_width =
         (unsigned short) read_u_v (16, "VUI: sar_width", s, &gDecoder->UsedBits);
        sps->vui_seq_parameters.sar_height =
         (unsigned short) read_u_v (16, "VUI: sar_height", s, &gDecoder->UsedBits);
        }
      }

    sps->vui_seq_parameters.overscan_info_present_flag =
      read_u_1 ("VUI: overscan_info_present_flag", s, &gDecoder->UsedBits);
    if (sps->vui_seq_parameters.overscan_info_present_flag)
      sps->vui_seq_parameters.overscan_appropriate_flag =
        read_u_1 ("VUI: overscan_appropriate_flag", s, &gDecoder->UsedBits);

    sps->vui_seq_parameters.video_signal_type_present_flag =
      read_u_1 ("VUI: video_signal_type_present_flag", s, &gDecoder->UsedBits);
    if (sps->vui_seq_parameters.video_signal_type_present_flag) {
      sps->vui_seq_parameters.video_format =
        read_u_v (3, "VUI: video_format", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.video_full_range_flag =
        read_u_1 ("VUI: video_full_range_flag", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.colour_description_present_flag =
       read_u_1 ("VUI: color_description_present_flag", s, &gDecoder->UsedBits);
      if (sps->vui_seq_parameters.colour_description_present_flag) {
        sps->vui_seq_parameters.colour_primaries =
         read_u_v (8, "VUI: colour_primaries", s, &gDecoder->UsedBits);
        sps->vui_seq_parameters.transfer_characteristics =
          read_u_v (8, "VUI: transfer_characteristics", s, &gDecoder->UsedBits);
        sps->vui_seq_parameters.matrix_coefficients =
          read_u_v (8, "VUI: matrix_coefficients", s, &gDecoder->UsedBits);
        }
      }

    sps->vui_seq_parameters.chroma_location_info_present_flag =
      read_u_1 ("VUI: chroma_loc_info_present_flag", s, &gDecoder->UsedBits);
    if (sps->vui_seq_parameters.chroma_location_info_present_flag) {
      sps->vui_seq_parameters.chroma_sample_loc_type_top_field =
        read_ue_v ("VUI: chroma_sample_loc_type_top_field", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.chroma_sample_loc_type_bottom_field  =
        read_ue_v ("VUI: chroma_sample_loc_type_bottom_field", s, &gDecoder->UsedBits);
      }

    sps->vui_seq_parameters.timing_info_present_flag =
      read_u_1 ("VUI: timing_info_present_flag", s, &gDecoder->UsedBits);
    if (sps->vui_seq_parameters.timing_info_present_flag) {
      sps->vui_seq_parameters.num_units_in_tick =
        read_u_v (32, "VUI: num_units_in_tick", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.time_scale = read_u_v (32,"VUI: time_scale", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.fixed_frame_rate_flag =
        read_u_1 ("VUI: fixed_frame_rate_flag", s, &gDecoder->UsedBits);
      }

    sps->vui_seq_parameters.nal_hrd_parameters_present_flag   = read_u_1 ("VUI: nal_hrd_parameters_present_flag", s, &gDecoder->UsedBits);
    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
      readHRDParameters (p, &(sps->vui_seq_parameters.nal_hrd_parameters));

    sps->vui_seq_parameters.vcl_hrd_parameters_present_flag =
      read_u_1 ("VUI: vcl_hrd_parameters_present_flag", s, &gDecoder->UsedBits);

    if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
      readHRDParameters(p, &(sps->vui_seq_parameters.vcl_hrd_parameters));

    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag ||
        sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
      sps->vui_seq_parameters.low_delay_hrd_flag = read_u_1 ("VUI: low_delay_hrd_flag", s, &gDecoder->UsedBits);

    sps->vui_seq_parameters.pic_struct_present_flag =
      read_u_1 ("VUI: pic_struct_present_flag   ", s, &gDecoder->UsedBits);
    sps->vui_seq_parameters.bitstream_restriction_flag =
      read_u_1 ("VUI: bitstream_restriction_flag", s, &gDecoder->UsedBits);

    if (sps->vui_seq_parameters.bitstream_restriction_flag) {
      sps->vui_seq_parameters.motion_vectors_over_pic_boundaries_flag =
        read_u_1 ("VUI: motion_vectors_over_pic_boundaries_flag", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.max_bytes_per_pic_denom =
        read_ue_v ("VUI: max_bytes_per_pic_denom", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.max_bits_per_mb_denom =
        read_ue_v ("VUI: max_bits_per_mb_denom", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.log2_max_mv_length_horizontal =
        read_ue_v ("VUI: log2_max_mv_length_horizontal", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.log2_max_mv_length_vertical =
        read_ue_v ("VUI: log2_max_mv_length_vertical", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.num_reorder_frames =
        read_ue_v ("VUI: num_reorder_frames", s, &gDecoder->UsedBits);
      sps->vui_seq_parameters.max_dec_frame_buffering =
        read_ue_v ("VUI: max_dec_frame_buffering", s, &gDecoder->UsedBits);
      }
    }

  return 0;
  }
//}}}
//{{{
static int interpretSPS (sVidParam* vidParam, sDataPartition *p, sSPSrbsp *sps) {

  unsigned i;
  unsigned n_ScalingList;
  int reserved_zero;
  Bitstream *s = p->bitstream;

  assert (p != NULL);
  assert (p->bitstream != NULL);
  assert (p->bitstream->streamBuffer != 0);
  assert (sps != NULL);

  gDecoder->UsedBits = 0;
  sps->profile_idc = read_u_v  (8, "SPS: profile_idc", s, &gDecoder->UsedBits);
  if ((sps->profile_idc!=BASELINE) &&
      (sps->profile_idc!=MAIN) &&
      (sps->profile_idc!=EXTENDED) &&
      (sps->profile_idc!=FREXT_HP) &&
      (sps->profile_idc!=FREXT_Hi10P) &&
      (sps->profile_idc!=FREXT_Hi422) &&
      (sps->profile_idc!=FREXT_Hi444) &&
      (sps->profile_idc!=FREXT_CAVLC444)) {
    printf ("Invalid Profile IDC (%d) encountered. \n", sps->profile_idc);
    return gDecoder->UsedBits;
    }

  sps->constrained_set0_flag = read_u_1 ("SPS: constrained_set0_flag", s, &gDecoder->UsedBits);
  sps->constrained_set1_flag = read_u_1 ("SPS: constrained_set1_flag", s, &gDecoder->UsedBits);
  sps->constrained_set2_flag = read_u_1 ("SPS: constrained_set2_flag", s, &gDecoder->UsedBits);
  sps->constrained_set3_flag = read_u_1 ("SPS: constrained_set3_flag", s, &gDecoder->UsedBits);

  reserved_zero = read_u_v (4, "SPS: reserved_zero_4bits", s, &gDecoder->UsedBits);

  if (reserved_zero != 0)
    printf ("Warning, reserved_zero flag not equal to 0. Possibly new constrained_setX flag introduced.\n");

  sps->level_idc = read_u_v (8, "SPS: level_idc", s, &gDecoder->UsedBits);
  sps->seq_parameter_set_id = read_ue_v ("SPS: seq_parameter_set_id", s, &gDecoder->UsedBits);

  // Fidelity Range Extensions stuff
  sps->chroma_format_idc = 1;
  sps->bit_depth_luma_minus8   = 0;
  sps->bit_depth_chroma_minus8 = 0;
  sps->lossless_qpprime_flag   = 0;
  sps->separate_colour_plane_flag = 0;

  if ((sps->profile_idc==FREXT_HP   ) ||
      (sps->profile_idc==FREXT_Hi10P) ||
      (sps->profile_idc==FREXT_Hi422) ||
      (sps->profile_idc==FREXT_Hi444) ||
      (sps->profile_idc==FREXT_CAVLC444)) {
    sps->chroma_format_idc = read_ue_v ("SPS: chroma_format_idc", s, &gDecoder->UsedBits);
    if (sps->chroma_format_idc == YUV444)
      sps->separate_colour_plane_flag = read_u_1  ("SPS: separate_colour_plane_flag", s, &gDecoder->UsedBits);

    sps->bit_depth_luma_minus8 = read_ue_v ("SPS: bit_depth_luma_minus8", s, &gDecoder->UsedBits);
    sps->bit_depth_chroma_minus8 = read_ue_v ("SPS: bit_depth_chroma_minus8", s, &gDecoder->UsedBits);
    if ((sps->bit_depth_luma_minus8+8 > sizeof(sPixel)*8) || (sps->bit_depth_chroma_minus8+8> sizeof(sPixel)*8))
      error ("Source picture has higher bit depth than sPixel data type. \nPlease recompile with larger data type for sPixel.", 500);

    sps->lossless_qpprime_flag = read_u_1 ("SPS: lossless_qpprime_y_zero_flag", s, &gDecoder->UsedBits);

    sps->seq_scaling_matrix_present_flag = read_u_1 ("SPS: seq_scaling_matrix_present_flag", s, &gDecoder->UsedBits);
    if (sps->seq_scaling_matrix_present_flag) {
      n_ScalingList = (sps->chroma_format_idc != YUV444) ? 8 : 12;
      for (i = 0; i < n_ScalingList; i++) {
        sps->seq_scaling_list_present_flag[i]   = read_u_1 ("SPS: seq_scaling_list_present_flag", s, &gDecoder->UsedBits);
        if (sps->seq_scaling_list_present_flag[i]) {
          if (i < 6)
            scaling_List (sps->ScalingList4x4[i], 16, &sps->UseDefaultScalingMatrix4x4Flag[i], s);
          else
            scaling_List (sps->ScalingList8x8[i-6], 64, &sps->UseDefaultScalingMatrix8x8Flag[i-6], s);
          }
        }
      }
    }

  sps->log2_max_frame_num_minus4 = read_ue_v ("SPS: log2_max_frame_num_minus4", s, &gDecoder->UsedBits);
  sps->pic_order_cnt_type = read_ue_v ("SPS: pic_order_cnt_type", s, &gDecoder->UsedBits);

  if (sps->pic_order_cnt_type == 0)
    sps->log2_max_pic_order_cnt_lsb_minus4 = read_ue_v ("SPS: log2_max_pic_order_cnt_lsb_minus4", s, &gDecoder->UsedBits);
  else if (sps->pic_order_cnt_type == 1) {
    sps->delta_pic_order_always_zero_flag = read_u_1 ("SPS: delta_pic_order_always_zero_flag", s, &gDecoder->UsedBits);
    sps->offset_for_non_ref_pic = read_se_v ("SPS: offset_for_non_ref_pic", s, &gDecoder->UsedBits);
    sps->offset_for_top_to_bottom_field = read_se_v ("SPS: offset_for_top_to_bottom_field", s, &gDecoder->UsedBits);
    sps->num_ref_frames_in_pic_order_cnt_cycle = read_ue_v ("SPS: num_ref_frames_in_pic_order_cnt_cycle", s, &gDecoder->UsedBits);
    for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      sps->offset_for_ref_frame[i] = read_se_v ("SPS: offset_for_ref_frame[i]", s, &gDecoder->UsedBits);
    }

  sps->num_ref_frames = read_ue_v ("SPS: num_ref_frames", s, &gDecoder->UsedBits);
  sps->gaps_in_frame_num_value_allowed_flag = read_u_1  ("SPS: gaps_in_frame_num_value_allowed_flag", s, &gDecoder->UsedBits);
  sps->pic_width_in_mbs_minus1 = read_ue_v ("SPS: pic_width_in_mbs_minus1", s, &gDecoder->UsedBits);
  sps->pic_height_in_map_units_minus1 = read_ue_v ("SPS: pic_height_in_map_units_minus1", s, &gDecoder->UsedBits);
  sps->frame_mbs_only_flag = read_u_1 ("SPS: frame_mbs_only_flag", s, &gDecoder->UsedBits);
  if (!sps->frame_mbs_only_flag)
    sps->mb_adaptive_frame_field_flag = read_u_1  ("SPS: mb_adaptive_frame_field_flag", s, &gDecoder->UsedBits);
  //printf("interlace flags %d %d\n", sps->frame_mbs_only_flag, sps->mb_adaptive_frame_field_flag);

  sps->direct_8x8_inference_flag = read_u_1  ("SPS: direct_8x8_inference_flag", s, &gDecoder->UsedBits);

  sps->frame_cropping_flag = read_u_1  ("SPS: frame_cropping_flag", s, &gDecoder->UsedBits);
  if (sps->frame_cropping_flag) {
    sps->frame_crop_left_offset = read_ue_v ("SPS: frame_crop_left_offset", s, &gDecoder->UsedBits);
    sps->frame_crop_right_offset = read_ue_v ("SPS: frame_crop_right_offset", s, &gDecoder->UsedBits);
    sps->frame_crop_top_offset = read_ue_v ("SPS: frame_crop_top_offset", s, &gDecoder->UsedBits);
    sps->frame_crop_bottom_offset = read_ue_v ("SPS: frame_crop_bottom_offset", s, &gDecoder->UsedBits);
    }
  sps->vui_parameters_present_flag = (Boolean) read_u_1 ("SPS: vui_parameters_present_flag", s, &gDecoder->UsedBits);

  initVUI (sps);
  readVUI (p, sps);

  sps->Valid = TRUE;
  return gDecoder->UsedBits;
  }
//}}}

//{{{
void get_max_dec_frame_buf_size (sSPSrbsp* sps) {

  int pic_size_mb = (sps->pic_width_in_mbs_minus1 + 1) * 
                    (sps->pic_height_in_map_units_minus1 + 1) * 
                    (sps->frame_mbs_only_flag?1:2);
  int size = 0;

  switch (sps->level_idc) {
    //{{{
    case 0:
      // if there is no level defined, we expect experimental usage and return a DPB size of 16
      size = 16 * pic_size_mb;
    //}}}
    //{{{
    case 9:
      size = 396;
      break;
    //}}}
    //{{{
    case 10:
      size = 396;
      break;
    //}}}
    //{{{
    case 11:
      if (!is_FREXT_profile(sps->profile_idc) && (sps->constrained_set3_flag == 1))
        size = 396;
      else
        size = 900;
      break;
    //}}}
    //{{{
    case 12:
      size = 2376;
      break;
    //}}}
    //{{{
    case 13:
      size = 2376;
      break;
    //}}}
    //{{{
    case 20:
      size = 2376;
      break;
    //}}}
    //{{{
    case 21:
      size = 4752;
      break;
    //}}}
    //{{{
    case 22:
      size = 8100;
      break;
    //}}}
    //{{{
    case 30:
      size = 8100;
      break;
    //}}}
    //{{{
    case 31:
      size = 18000;
      break;
    //}}}
    //{{{
    case 32:
      size = 20480;
      break;
    //}}}
    //{{{
    case 40:
      size = 32768;
      break;
    //}}}
    //{{{
    case 41:
      size = 32768;
      break;
    //}}}
    //{{{
    case 42:
      size = 34816;
      break;
    //}}}
    //{{{
    case 50:
      size = 110400;
      break;
    //}}}
    //{{{
    case 51:
      size = 184320;
      break;
    //}}}
    //{{{
    case 52:
      size = 184320;
      break;
    //}}}
    case 60:
    case 61:
    //{{{
    case 62:
      size = 696320;
      break;
    //}}}
    //{{{
    default:
      error ("undefined level", 500);
      break;
    //}}}
    }

  size /= pic_size_mb;
  size = imin(size, 16);
  }
//}}}
//{{{
void MakeSPSavailable (sVidParam* vidParam, int id, sSPSrbsp* sps) {

  assert (sps->Valid == TRUE);
  memcpy (&vidParam->SeqParSet[id], sps, sizeof (sSPSrbsp));
  }

//}}}
//{{{
void ProcessSPS (sVidParam* vidParam, NALU_t* nalu) {

  sDataPartition* dp = AllocPartition (1);
  sSPSrbsp* sps = AllocSPS();
  dp->bitstream->ei_flag = 0;
  dp->bitstream->read_len = dp->bitstream->frame_bitoffset = 0;
  memcpy (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->code_len = dp->bitstream->bitstream_length = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);
  interpretSPS (vidParam, dp, sps);
  get_max_dec_frame_buf_size (sps);

  if (sps->Valid) {
    if (vidParam->active_sps) {
      if (sps->seq_parameter_set_id == vidParam->active_sps->seq_parameter_set_id) {
        if (!spsIsEqual (sps, vidParam->active_sps))   {
          if (vidParam->picture)
            exit_picture (vidParam, &vidParam->picture);
          vidParam->active_sps=NULL;
          }
        }
      }

    MakeSPSavailable (vidParam, sps->seq_parameter_set_id, sps);
    vidParam->profile_idc = sps->profile_idc;
    vidParam->separate_colour_plane_flag = sps->separate_colour_plane_flag;
    if( vidParam->separate_colour_plane_flag )
      vidParam->ChromaArrayType = 0;
    else
      vidParam->ChromaArrayType = sps->chroma_format_idc;
    }

  FreePartition (dp, 1);
  freeSPS (sps);
  }
//}}}
//{{{
void activateSPS (sVidParam* vidParam, sSPSrbsp* sps) {

  InputParameters* p_Inp = vidParam->p_Inp;

  if (vidParam->active_sps != sps) {
    if (vidParam->picture) // this may only happen on slice loss
      exit_picture (vidParam, &vidParam->picture);
    vidParam->active_sps = sps;

    if (vidParam->dpb_layer_id==0 && is_BL_profile(sps->profile_idc) && !vidParam->p_Dpb_layer[0]->init_done) {
      set_coding_par (sps, vidParam->p_EncodePar[0]);
      setup_layer_info ( vidParam, sps, vidParam->p_LayerPar[0]);
      }
    set_global_coding_par(vidParam, vidParam->p_EncodePar[vidParam->dpb_layer_id]);

    init_global_buffers (vidParam, 0);
    if (!vidParam->no_output_of_prior_pics_flag)
      flush_dpb (vidParam->p_Dpb_layer[0]);

    init_dpb (vidParam, vidParam->p_Dpb_layer[0], 0);

    // enable error concealment
    ercInit (vidParam, vidParam->width, vidParam->height, 1);
    if (vidParam->picture) {
      ercReset (vidParam->erc_errorVar, vidParam->PicSizeInMbs, vidParam->PicSizeInMbs, vidParam->picture->size_x);
      vidParam->erc_mvperMB = 0;
      }
    }

  reset_format_info (sps, vidParam, &p_Inp->source, &p_Inp->output);
  }
//}}}

// PPS
//{{{
static int ppsIsEqual (sPPSrbsp* pps1, sPPSrbsp* pps2) {

  unsigned i, j;

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
      for (i = 0; i <= pps1->num_slice_groups_minus1; i++)
        equal &= (pps1->run_length_minus1[i] == pps2->run_length_minus1[i]);
      }
    else if (pps1->slice_group_map_type == 2) {
      for (i = 0; i < pps1->num_slice_groups_minus1; i++) {
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
      for (i = 0; i <= pps1->pic_size_in_map_units_minus1; i++)
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

  // Fidelity Range Extensions Stuff, initialized to zero, so should be ok to check all the time.
  equal &= (pps1->transform_8x8_mode_flag == pps2->transform_8x8_mode_flag);
  equal &= (pps1->pic_scaling_matrix_present_flag == pps2->pic_scaling_matrix_present_flag);
  if (pps1->pic_scaling_matrix_present_flag) {
    for (i = 0; i < (6 + ((unsigned)pps1->transform_8x8_mode_flag << 1)); i++) {
      equal &= (pps1->pic_scaling_list_present_flag[i] == pps2->pic_scaling_list_present_flag[i]);
      if (pps1->pic_scaling_list_present_flag[i]) {
        if (i < 6) {
          for (j = 0; j < 16; j++)
            equal &= (pps1->ScalingList4x4[i][j] == pps2->ScalingList4x4[i][j]);
          }
        else {
          for (j = 0; j < 64; j++)
            equal &= (pps1->ScalingList8x8[i-6][j] == pps2->ScalingList8x8[i-6][j]);
          }
        }
      }
    }

  equal &= (pps1->second_chroma_qp_index_offset == pps2->second_chroma_qp_index_offset);
  return equal;
  }
//}}}
//{{{
static int interpretPPS (sVidParam* vidParam, sDataPartition* p, sPPSrbsp* pps) {

  unsigned i;
  unsigned n_ScalingList;
  int chroma_format_idc;
  int NumberBitsPerSliceGroupId;
  Bitstream *s = p->bitstream;

  assert (p != NULL);
  assert (p->bitstream != NULL);
  assert (p->bitstream->streamBuffer != 0);
  assert (pps != NULL);

  gDecoder->UsedBits = 0;
  pps->pic_parameter_set_id = read_ue_v ("PPS: pic_parameter_set_id", s, &gDecoder->UsedBits);
  pps->seq_parameter_set_id = read_ue_v ("PPS: seq_parameter_set_id", s, &gDecoder->UsedBits);
  pps->entropy_coding_mode_flag = read_u_1  ("PPS: entropy_coding_mode_flag", s, &gDecoder->UsedBits);

  //! Note: as per JVT-F078 the following bit is unconditional.  If F078 is not accepted, then
  //! one has to fetch the correct SPS to check whether the bit is present (hopefully there is
  //! no consistency problem :-(
  //! The current encoder code handles this in the same way.  When you change this, don't forget
  //! the encoder!  StW, 12/8/02
  pps->bottom_field_pic_order_in_frame_present_flag =
    read_u_1  ("PPS: bottom_field_pic_order_in_frame_present_flag", s, &gDecoder->UsedBits);
  pps->num_slice_groups_minus1 = read_ue_v ("PPS: num_slice_groups_minus1", s, &gDecoder->UsedBits);

  if (pps->num_slice_groups_minus1 > 0) {
    //{{{  FMO
    pps->slice_group_map_type = read_ue_v ("PPS: slice_group_map_type", s, &gDecoder->UsedBits);
    if (pps->slice_group_map_type == 0) {
      for (i = 0; i <= pps->num_slice_groups_minus1; i++)
        pps->run_length_minus1 [i] = read_ue_v ("PPS: run_length_minus1 [i]", s, &gDecoder->UsedBits);
      }
    else if (pps->slice_group_map_type == 2) {
      for (i = 0; i < pps->num_slice_groups_minus1; i++) {
        //! JVT-F078: avoid reference of SPS by using ue(v) instead of u(v)
        pps->top_left [i] = read_ue_v ("PPS: top_left [i]", s, &gDecoder->UsedBits);
        pps->bottom_right [i] = read_ue_v ("PPS: bottom_right [i]", s, &gDecoder->UsedBits);
        }
      }
    else if (pps->slice_group_map_type == 3 ||
             pps->slice_group_map_type == 4 ||
             pps->slice_group_map_type == 5) {
      pps->slice_group_change_direction_flag =
        read_u_1  ("PPS: slice_group_change_direction_flag", s, &gDecoder->UsedBits);
      pps->slice_group_change_rate_minus1 =
        read_ue_v ("PPS: slice_group_change_rate_minus1", s, &gDecoder->UsedBits);
      }
    else if (pps->slice_group_map_type == 6) {
      if (pps->num_slice_groups_minus1+1 >4)
        NumberBitsPerSliceGroupId = 3;
      else if (pps->num_slice_groups_minus1+1 > 2)
        NumberBitsPerSliceGroupId = 2;
      else
        NumberBitsPerSliceGroupId = 1;
      pps->pic_size_in_map_units_minus1      = read_ue_v ("PPS: pic_size_in_map_units_minus1"               , s, &gDecoder->UsedBits);
      if ((pps->slice_group_id = calloc (pps->pic_size_in_map_units_minus1+1, 1)) == NULL)
        no_mem_exit ("InterpretPPS: slice_group_id");
      for (i = 0; i <= pps->pic_size_in_map_units_minus1; i++)
        pps->slice_group_id[i] =
          (byte)read_u_v (NumberBitsPerSliceGroupId, "slice_group_id[i]", s, &gDecoder->UsedBits);
      }
    }
    //}}}

  pps->num_ref_idx_l0_default_active_minus1 =
    read_ue_v ("PPS: num_ref_idx_l0_default_active_minus1", s, &gDecoder->UsedBits);
  pps->num_ref_idx_l1_default_active_minus1 =
    read_ue_v ("PPS: num_ref_idx_l1_default_active_minus1", s, &gDecoder->UsedBits);

  pps->weighted_pred_flag = read_u_1  ("PPS: weighted_pred_flag", s, &gDecoder->UsedBits);
  pps->weighted_bipred_idc = read_u_v  ( 2, "PPS: weighted_bipred_idc", s, &gDecoder->UsedBits);

  pps->pic_init_qp_minus26 = read_se_v ("PPS: pic_init_qp_minus26", s, &gDecoder->UsedBits);
  pps->pic_init_qs_minus26 = read_se_v ("PPS: pic_init_qs_minus26", s, &gDecoder->UsedBits);

  pps->chroma_qp_index_offset = read_se_v ("PPS: chroma_qp_index_offset"   , s, &gDecoder->UsedBits);
  pps->deblocking_filter_control_present_flag =
    read_u_1 ("PPS: deblocking_filter_control_present_flag" , s, &gDecoder->UsedBits);

  pps->constrained_intra_pred_flag = read_u_1  ("PPS: constrained_intra_pred_flag", s, &gDecoder->UsedBits);
  pps->redundant_pic_cnt_present_flag =
    read_u_1  ("PPS: redundant_pic_cnt_present_flag", s, &gDecoder->UsedBits);

  if (more_rbsp_data (s->streamBuffer, s->frame_bitoffset,s->bitstream_length)) {
    //{{{  more_data_in_rbsp
    //Fidelity Range Extensions Stuff
    pps->transform_8x8_mode_flag = read_u_1 ("PPS: transform_8x8_mode_flag", s, &gDecoder->UsedBits);
    pps->pic_scaling_matrix_present_flag =
      read_u_1  ("PPS: pic_scaling_matrix_present_flag", s, &gDecoder->UsedBits);

    if (pps->pic_scaling_matrix_present_flag) {
      chroma_format_idc = vidParam->SeqParSet[pps->seq_parameter_set_id].chroma_format_idc;
      n_ScalingList = 6 + ((chroma_format_idc != YUV444) ? 2 : 6) * pps->transform_8x8_mode_flag;
      for(i=0; i<n_ScalingList; i++) {
        pps->pic_scaling_list_present_flag[i]=
          read_u_1 ("PPS: pic_scaling_list_present_flag", s, &gDecoder->UsedBits);

        if(pps->pic_scaling_list_present_flag[i]) {
          if(i<6)
            scaling_List (pps->ScalingList4x4[i], 16, &pps->UseDefaultScalingMatrix4x4Flag[i], s);
          else
            scaling_List (pps->ScalingList8x8[i-6], 64, &pps->UseDefaultScalingMatrix8x8Flag[i-6], s);
          }
        }
      }
    pps->second_chroma_qp_index_offset =
      read_se_v ("PPS: second_chroma_qp_index_offset", s, &gDecoder->UsedBits);
    }
    //}}}
  else
    pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;

  pps->Valid = TRUE;
  return gDecoder->UsedBits;
  }
//}}}
//{{{
static void activatePPS (sVidParam* vidParam, sPPSrbsp *pps) {

  if (vidParam->active_pps != pps) {
    if (vidParam->picture) // && vidParam->num_dec_mb == vidParam->pi)
      // this may only happen on slice loss
      exit_picture (vidParam, &vidParam->picture);
    vidParam->active_pps = pps;
    }
  }
//}}}

//{{{
sPPSrbsp* AllocPPS() {

  sPPSrbsp *p;

  if ((p = calloc (1, sizeof (sPPSrbsp))) == NULL)
    no_mem_exit ("AllocPPS: PPS");
  p->slice_group_id = NULL;
  return p;
  }
//}}}
//{{{
 void FreePPS (sPPSrbsp* pps) {

   assert (pps != NULL);
   if (pps->slice_group_id != NULL)
     free (pps->slice_group_id);
   free (pps);
   }
//}}}
//{{{
void MakePPSavailable (sVidParam* vidParam, int id, sPPSrbsp *pps) {

  assert (pps->Valid == TRUE);

  if (vidParam->PicParSet[id].Valid == TRUE && vidParam->PicParSet[id].slice_group_id != NULL)
    free (vidParam->PicParSet[id].slice_group_id);

  memcpy (&vidParam->PicParSet[id], pps, sizeof (sPPSrbsp));

  // we can simply use the memory provided with the pps. the PPS is destroyed after this function
  // call and will not try to free if pps->slice_group_id == NULL
  vidParam->PicParSet[id].slice_group_id = pps->slice_group_id;
  pps->slice_group_id          = NULL;
  }
//}}}
//{{{
void CleanUpPPS (sVidParam* vidParam) {

  for (int i = 0; i < MAXPPS; i++) {
    if (vidParam->PicParSet[i].Valid == TRUE && vidParam->PicParSet[i].slice_group_id != NULL)
      free (vidParam->PicParSet[i].slice_group_id);
    vidParam->PicParSet[i].Valid = FALSE;
    }
  }
//}}}
//{{{
void ProcessPPS (sVidParam* vidParam, NALU_t *nalu) {

  sDataPartition* dp = AllocPartition (1);
  sPPSrbsp* pps = AllocPPS();
  dp->bitstream->ei_flag = 0;
  dp->bitstream->read_len = dp->bitstream->frame_bitoffset = 0;
  memcpy (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->code_len = dp->bitstream->bitstream_length = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);
  interpretPPS (vidParam, dp, pps);

  if (vidParam->active_pps) {
    if (pps->pic_parameter_set_id == vidParam->active_pps->pic_parameter_set_id) {
      if (!ppsIsEqual (pps, vidParam->active_pps)) {
        // copy to next PPS;
        memcpy (vidParam->pNextPPS, vidParam->active_pps, sizeof (sPPSrbsp));
        if (vidParam->picture)
          exit_picture(vidParam, &vidParam->picture);
        vidParam->active_pps = NULL;
        }
      }
    }

  MakePPSavailable (vidParam, pps->pic_parameter_set_id, pps);
  FreePartition (dp, 1);
  FreePPS (pps);
  }
//}}}
//{{{
void UseParameterSet (sSlice* currSlice) {

  sVidParam* vidParam = currSlice->vidParam;
  int PicParsetId = currSlice->pic_parameter_set_id;
  sPPSrbsp* pps = &vidParam->PicParSet[PicParsetId];
  sSPSrbsp* sps = &vidParam->SeqParSet[pps->seq_parameter_set_id];
  int i;

  if (pps->Valid != TRUE)
    printf ("Trying to use an invalid (uninitialized) Picture Parameter Set with ID %d, expect the unexpected...\n", PicParsetId);

  if (sps->Valid != TRUE)
    printf ("PicParset %d references an invalid (uninitialized) Sequence Parameter Set with ID %d, expect the unexpected...\n",
            PicParsetId, (int) pps->seq_parameter_set_id);

  // In theory, and with a well-designed software, the lines above are everything necessary.
  // In practice, we need to patch many values
  // in vidParam-> (but no more in p_Inp-> -- these have been taken care of)
  // Set Sequence Parameter Stuff first
  if ((int) sps->pic_order_cnt_type < 0 || sps->pic_order_cnt_type > 2) {
    // != 1
    printf ("invalid sps->pic_order_cnt_type = %d\n", (int) sps->pic_order_cnt_type);
    error ("pic_order_cnt_type != 1", -1000);
    }

  if (sps->pic_order_cnt_type == 1)
    if (sps->num_ref_frames_in_pic_order_cnt_cycle >= MAXnum_ref_frames_in_pic_order_cnt_cycle)
      error ("num_ref_frames_in_pic_order_cnt_cycle too large",-1011);

  vidParam->dpb_layer_id = currSlice->layer_id;
  activateSPS (vidParam, sps);
  activatePPS (vidParam, pps);

  // currSlice->dp_mode is set by read_new_slice (NALU first byte available there)
  if (pps->entropy_coding_mode_flag == (Boolean)CAVLC) {
    currSlice->nal_startcode_follows = uvlc_startcode_follows;
    for (i = 0; i < 3; i++)
      currSlice->partArr[i].readSyntaxElement = readSyntaxElement_UVLC;
    }
  else {
    currSlice->nal_startcode_follows = cabac_startcode_follows;
    for (i = 0; i < 3; i++)
      currSlice->partArr[i].readSyntaxElement = readSyntaxElement_CABAC;
    }
  vidParam->type = currSlice->slice_type;
  }
//}}}
