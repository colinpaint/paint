//{{{  includes
#include "global.h"
#include "memalloc.h"

#include "image.h"
#include "fmo.h"
#include "nalu.h"
#include "parset.h"
#include "sliceHeader.h"
#include "sei.h"
#include "output.h"
#include "mbAccess.h"
#include "macroblock.h"
#include "loopfilter.h"
#include "biariDecode.h"
#include "cabac.h"
#include "vlc.h"
#include "quant.h"
#include "errorConceal.h"
#include "erc.h"
#include "mbuffer.h"
#include "mcPrediction.h"

#include <math.h>
#include <limits.h>
//}}}

//{{{
static void reset_mbs (sMacroblock *currMB) {

  currMB->slice_nr = -1;
  currMB->ei_flag =  1;
  currMB->dpl_flag =  0;
  }
//}}}
//{{{
static void setup_buffers (sVidParam* vidParam, int layer_id) {


  if (vidParam->last_dec_layer_id != layer_id) {
    sCodingParams* cps = vidParam->p_EncodePar[layer_id];
    if (cps->separate_colour_plane_flag) {
      for (int i = 0; i < MAX_PLANE; i++ ) {
        vidParam->mb_data_JV[i] = cps->mb_data_JV[i];
        vidParam->intra_block_JV[i] = cps->intra_block_JV[i];
        vidParam->ipredmode_JV[i] = cps->ipredmode_JV[i];
        vidParam->siblock_JV[i] = cps->siblock_JV[i];
        }
      vidParam->mb_data = NULL;
      vidParam->intra_block = NULL;
      vidParam->ipredmode = NULL;
      vidParam->siblock = NULL;
      }
    else {
      vidParam->mb_data = cps->mb_data;
      vidParam->intra_block = cps->intra_block;
      vidParam->ipredmode = cps->ipredmode;
      vidParam->siblock = cps->siblock;
      }

    vidParam->PicPos = cps->PicPos;
    vidParam->nz_coeff = cps->nz_coeff;
    vidParam->qp_per_matrix = cps->qp_per_matrix;
    vidParam->qp_rem_matrix = cps->qp_rem_matrix;
    vidParam->oldFrameSizeInMbs = cps->oldFrameSizeInMbs;
    vidParam->last_dec_layer_id = layer_id;
    }
  }
//}}}

//{{{
static void update_mbaff_macroblock_data (sPixel** cur_img, sPixel (*temp)[16], int x0, int width, int height) {

  sPixel (*temp_evn)[16] = temp;
  sPixel (*temp_odd)[16] = temp + height;
  sPixel** temp_img = cur_img;

  for (int y = 0; y < 2 * height; ++y)
    memcpy (*temp++, (*temp_img++ + x0), width * sizeof(sPixel));

  for (int y = 0; y < height; ++y) {
    memcpy ((*cur_img++ + x0), *temp_evn++, width * sizeof(sPixel));
    memcpy ((*cur_img++ + x0), *temp_odd++, width * sizeof(sPixel));
    }
  }
//}}}
//{{{
static void mbAffPostProc (sVidParam* vidParam) {

  sPixel temp_buffer[32][16];

  sPicture* picture = vidParam->picture;
  sPixel** imgY = picture->imgY;
  sPixel*** imgUV = picture->imgUV;

  short x0;
  short y0;
  for (short i = 0; i < (int)picture->PicSizeInMbs; i += 2) {
    if (picture->motion.mb_field[i]) {
      get_mb_pos (vidParam, i, vidParam->mb_size[IS_LUMA], &x0, &y0);
      update_mbaff_macroblock_data (imgY + y0, temp_buffer, x0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

      if (picture->chroma_format_idc != YUV400) {
        x0 = (short)((x0 * vidParam->mb_cr_size_x) >> 4);
        y0 = (short)((y0 * vidParam->mb_cr_size_y) >> 4);
        update_mbaff_macroblock_data (imgUV[0] + y0, temp_buffer, x0, vidParam->mb_cr_size_x, vidParam->mb_cr_size_y);
        update_mbaff_macroblock_data (imgUV[1] + y0, temp_buffer, x0, vidParam->mb_cr_size_x, vidParam->mb_cr_size_y);
        }
      }
    }
  }
//}}}
//{{{
static void fill_wp_params (sSlice *currSlice) {

  if (currSlice->slice_type == B_SLICE) {
    int comp;
    int log_weight_denom;
    int tb, td;
    int tx, DistScaleFactor;

    int max_l0_ref = currSlice->num_ref_idx_active[LIST_0];
    int max_l1_ref = currSlice->num_ref_idx_active[LIST_1];

    if (currSlice->active_pps->weighted_bipred_idc == 2) {
      currSlice->luma_log2_weight_denom = 5;
      currSlice->chroma_log2_weight_denom = 5;
      currSlice->wp_round_luma = 16;
      currSlice->wp_round_chroma = 16;
      for (int i = 0; i < MAX_REFERENCE_PICTURES; ++i)
        for (comp = 0; comp < 3; ++comp) {
          log_weight_denom = (comp == 0) ? currSlice->luma_log2_weight_denom : currSlice->chroma_log2_weight_denom;
          currSlice->wp_weight[0][i][comp] = 1 << log_weight_denom;
          currSlice->wp_weight[1][i][comp] = 1 << log_weight_denom;
          currSlice->wp_offset[0][i][comp] = 0;
          currSlice->wp_offset[1][i][comp] = 0;
          }
      }

    for (int i = 0; i < max_l0_ref; ++i)
      for (int j = 0; j < max_l1_ref; ++j)
        for (comp = 0; comp<3; ++comp) {
          log_weight_denom = (comp == 0) ? currSlice->luma_log2_weight_denom : currSlice->chroma_log2_weight_denom;
          if (currSlice->active_pps->weighted_bipred_idc == 1) {
            currSlice->wbp_weight[0][i][j][comp] =  currSlice->wp_weight[0][i][comp];
            currSlice->wbp_weight[1][i][j][comp] =  currSlice->wp_weight[1][j][comp];
            }
          else if (currSlice->active_pps->weighted_bipred_idc == 2) {
            td = iClip3(-128,127,currSlice->listX[LIST_1][j]->poc - currSlice->listX[LIST_0][i]->poc);
            if (td == 0 ||
                currSlice->listX[LIST_1][j]->is_long_term ||
                currSlice->listX[LIST_0][i]->is_long_term) {
              currSlice->wbp_weight[0][i][j][comp] = 32;
              currSlice->wbp_weight[1][i][j][comp] = 32;
              }
            else {
              tb = iClip3(-128,127,currSlice->ThisPOC - currSlice->listX[LIST_0][i]->poc);
              tx = (16384 + iabs(td/2))/td;
              DistScaleFactor = iClip3(-1024, 1023, (tx*tb + 32 )>>6);
              currSlice->wbp_weight[1][i][j][comp] = DistScaleFactor >> 2;
              currSlice->wbp_weight[0][i][j][comp] = 64 - currSlice->wbp_weight[1][i][j][comp];
              if (currSlice->wbp_weight[1][i][j][comp] < -64 || currSlice->wbp_weight[1][i][j][comp] > 128) {
                currSlice->wbp_weight[0][i][j][comp] = 32;
                currSlice->wbp_weight[1][i][j][comp] = 32;
                currSlice->wp_offset[0][i][comp] = 0;
                currSlice->wp_offset[1][j][comp] = 0;
                }
              }
            }
          }

    if (currSlice->mb_aff_frame_flag)
      for (int i = 0; i < 2*max_l0_ref; ++i)
        for (int j = 0; j < 2*max_l1_ref; ++j)
          for (comp = 0; comp<3; ++comp)
            for (int k = 2; k < 6; k += 2) {
              currSlice->wp_offset[k+0][i][comp] = currSlice->wp_offset[0][i>>1][comp];
              currSlice->wp_offset[k+1][j][comp] = currSlice->wp_offset[1][j>>1][comp];
              log_weight_denom = (comp == 0) ? currSlice->luma_log2_weight_denom : currSlice->chroma_log2_weight_denom;
              if (currSlice->active_pps->weighted_bipred_idc == 1) {
                currSlice->wbp_weight[k+0][i][j][comp] = currSlice->wp_weight[0][i>>1][comp];
                currSlice->wbp_weight[k+1][i][j][comp] = currSlice->wp_weight[1][j>>1][comp];
                }
              else if (currSlice->active_pps->weighted_bipred_idc == 2) {
                td = iClip3 (-128, 127, currSlice->listX[k+LIST_1][j]->poc - currSlice->listX[k+LIST_0][i]->poc);
                if (td == 0 ||
                    currSlice->listX[k+LIST_1][j]->is_long_term ||
                    currSlice->listX[k+LIST_0][i]->is_long_term) {
                  currSlice->wbp_weight[k+0][i][j][comp] = 32;
                  currSlice->wbp_weight[k+1][i][j][comp] = 32;
                  }
                else {
                  tb = iClip3(-128,127, ((k == 2) ? currSlice->toppoc :
                                                    currSlice->bottompoc) - currSlice->listX[k+LIST_0][i]->poc);
                  tx = (16384 + iabs(td/2)) / td;
                  DistScaleFactor = iClip3(-1024, 1023, (tx*tb + 32 )>>6);
                  currSlice->wbp_weight[k+1][i][j][comp] = DistScaleFactor >> 2;
                  currSlice->wbp_weight[k+0][i][j][comp] = 64 - currSlice->wbp_weight[k+1][i][j][comp];
                  if (currSlice->wbp_weight[k+1][i][j][comp] < -64 ||
                      currSlice->wbp_weight[k+1][i][j][comp] > 128) {
                    currSlice->wbp_weight[k+1][i][j][comp] = 32;
                    currSlice->wbp_weight[k+0][i][j][comp] = 32;
                    currSlice->wp_offset[k+0][i][comp] = 0;
                    currSlice->wp_offset[k+1][j][comp] = 0;
                    }
                  }
                }
              }
    }
  }
//}}}
//{{{
static void errorTracking (sVidParam* vidParam, sSlice *currSlice) {

  if (currSlice->redundant_pic_cnt == 0)
    vidParam->Is_primary_correct = vidParam->Is_redundant_correct = 1;

  if (currSlice->redundant_pic_cnt == 0 && vidParam->type != I_SLICE) {
    for (int i = 0; i < currSlice->num_ref_idx_active[LIST_0];++i)
      if (currSlice->ref_flag[i] == 0)  // any reference of primary slice is incorrect
        vidParam->Is_primary_correct = 0; // primary slice is incorrect
    }
  else if (currSlice->redundant_pic_cnt != 0 && vidParam->type != I_SLICE)
    // reference of redundant slice is incorrect
    if (currSlice->ref_flag[currSlice->redundant_slice_ref_idx] == 0)
      // redundant slice is incorrect
      vidParam->Is_redundant_correct = 0;
  }
//}}}
//{{{
static void copyPOC (sSlice *pSlice0, sSlice *currSlice) {

  currSlice->framepoc  = pSlice0->framepoc;
  currSlice->toppoc    = pSlice0->toppoc;
  currSlice->bottompoc = pSlice0->bottompoc;
  currSlice->ThisPOC   = pSlice0->ThisPOC;
  }
//}}}
//{{{
static void reorderLists (sSlice* currSlice) {

  sVidParam* vidParam = currSlice->vidParam;

  if ((currSlice->slice_type != I_SLICE)&&(currSlice->slice_type != SI_SLICE)) {
    if (currSlice->ref_pic_list_reordering_flag[LIST_0])
      reorder_ref_pic_list(currSlice, LIST_0);
    if (vidParam->no_reference_picture == currSlice->listX[0][currSlice->num_ref_idx_active[LIST_0] - 1]) {
      if (vidParam->non_conforming_stream)
        printf("RefPicList0[ %d ] is equal to 'no reference picture'\n", currSlice->num_ref_idx_active[LIST_0] - 1);
      else
        error("RefPicList0[ num_ref_idx_l0_active_minus1 ] is equal to 'no reference picture', invalid bitstream",500);
      }
    // that's a definition
    currSlice->listXsize[0] = (char) currSlice->num_ref_idx_active[LIST_0];
    }

  if (currSlice->slice_type == B_SLICE) {
    if (currSlice->ref_pic_list_reordering_flag[LIST_1])
      reorder_ref_pic_list(currSlice, LIST_1);
    if (vidParam->no_reference_picture == currSlice->listX[1][currSlice->num_ref_idx_active[LIST_1]-1]) {
      if (vidParam->non_conforming_stream)
        printf("RefPicList1[ %d ] is equal to 'no reference picture'\n", currSlice->num_ref_idx_active[LIST_1] - 1);
      else
        error("RefPicList1[ num_ref_idx_l1_active_minus1 ] is equal to 'no reference picture', invalid bitstream",500);
      }
    // that's a definition
    currSlice->listXsize[1] = (char) currSlice->num_ref_idx_active[LIST_1];
    }

  free_ref_pic_list_reordering_buffer(currSlice);
  }
//}}}
//{{{
static void ercWriteMBMODEandMV (sMacroblock* currMB) {

  sVidParam* vidParam = currMB->vidParam;
  int currMBNum = currMB->mbAddrX; //vidParam->currentSlice->current_mb_nr;
  sPicture* picture = vidParam->picture;
  int mbx = xPosMB(currMBNum, picture->size_x), mby = yPosMB(currMBNum, picture->size_x);

  objectBuffer_t* currRegion = vidParam->erc_object_list + (currMBNum<<2);

  if (vidParam->type != B_SLICE) {
    //{{{  non-B frame
    for (int i = 0; i < 4; ++i) {
      objectBuffer_t* pRegion = currRegion + i;
      pRegion->regionMode = (currMB->mb_type  ==I16MB  ? REGMODE_INTRA :
                               currMB->b8mode[i] == IBLOCK ? REGMODE_INTRA_8x8  :
                                 currMB->b8mode[i] == 0 ? REGMODE_INTER_COPY :
                                   currMB->b8mode[i] == 1 ? REGMODE_INTER_PRED :
                                     REGMODE_INTER_PRED_8x8);

      if (currMB->b8mode[i] == 0 || currMB->b8mode[i] == IBLOCK) {
        // INTRA OR COPY
        pRegion->mv[0] = 0;
        pRegion->mv[1] = 0;
        pRegion->mv[2] = 0;
        }
      else {
        int ii = 4*mbx + (i & 0x01)*2;// + BLOCK_SIZE;
        int jj = 4*mby + (i >> 1  )*2;
        if (currMB->b8mode[i]>=5 && currMB->b8mode[i]<=7) {
          // SMALL BLOCKS
          pRegion->mv[0] = (picture->mv_info[jj][ii].mv[LIST_0].mv_x + picture->mv_info[jj][ii + 1].mv[LIST_0].mv_x + picture->mv_info[jj + 1][ii].mv[LIST_0].mv_x + picture->mv_info[jj + 1][ii + 1].mv[LIST_0].mv_x + 2)/4;
          pRegion->mv[1] = (picture->mv_info[jj][ii].mv[LIST_0].mv_y + picture->mv_info[jj][ii + 1].mv[LIST_0].mv_y + picture->mv_info[jj + 1][ii].mv[LIST_0].mv_y + picture->mv_info[jj + 1][ii + 1].mv[LIST_0].mv_y + 2)/4;
          }
        else {
          // 16x16, 16x8, 8x16, 8x8
          pRegion->mv[0] = picture->mv_info[jj][ii].mv[LIST_0].mv_x;
          pRegion->mv[1] = picture->mv_info[jj][ii].mv[LIST_0].mv_y;
          }

        currMB->p_Slice->erc_mvperMB += iabs(pRegion->mv[0]) + iabs(pRegion->mv[1]);
        pRegion->mv[2] = picture->mv_info[jj][ii].ref_idx[LIST_0];
        }
      }
    }
    //}}}
  else {
    //{{{  B-frame
    for (int i = 0; i < 4; ++i) {
      int ii = 4*mbx + (i%2)*2;// + BLOCK_SIZE;
      int jj = 4*mby + (i/2)*2;

      objectBuffer_t* pRegion = currRegion + i;
      pRegion->regionMode = (currMB->mb_type  ==I16MB  ? REGMODE_INTRA :
                               currMB->b8mode[i]==IBLOCK ? REGMODE_INTRA_8x8 :
                                 REGMODE_INTER_PRED_8x8);

      if (currMB->mb_type==I16MB || currMB->b8mode[i]==IBLOCK) {
        // INTRA
        pRegion->mv[0] = 0;
        pRegion->mv[1] = 0;
        pRegion->mv[2] = 0;
        }
      else {
        int idx = (picture->mv_info[jj][ii].ref_idx[0] < 0) ? 1 : 0;
        pRegion->mv[0] = (picture->mv_info[jj][ii].mv[idx].mv_x +
                          picture->mv_info[jj][ii+1].mv[idx].mv_x +
                          picture->mv_info[jj+1][ii].mv[idx].mv_x +
                          picture->mv_info[jj+1][ii+1].mv[idx].mv_x + 2)/4;

        pRegion->mv[1] = (picture->mv_info[jj][ii].mv[idx].mv_y +
                          picture->mv_info[jj][ii+1].mv[idx].mv_y +
                          picture->mv_info[jj+1][ii].mv[idx].mv_y +
                          picture->mv_info[jj+1][ii+1].mv[idx].mv_y + 2)/4;
        currMB->p_Slice->erc_mvperMB += iabs(pRegion->mv[0]) + iabs(pRegion->mv[1]);

        pRegion->mv[2]  = (picture->mv_info[jj][ii].ref_idx[idx]);
        }
      }
    }
    //}}}
  }
//}}}
//{{{
static void init_cur_imgy (sSlice* currSlice, sVidParam* vidParam) {

  if ((vidParam->separate_colour_plane_flag != 0)) {
    sPicture* vidref = vidParam->no_reference_picture;
    int noref = (currSlice->framepoc < vidParam->recovery_poc);
    switch (currSlice->colour_plane_id) {
      case 0:
        for (int j = 0; j < 6; j++) { //for (j = 0; j < (currSlice->slice_type==B_SLICE?2:1); j++) {
          for (int i = 0; i < MAX_LIST_SIZE; i++) {
            sPicture* curr_ref = currSlice->listX[j][i];
            if (curr_ref) {
              curr_ref->no_ref = noref && (curr_ref == vidref);
              curr_ref->cur_imgY = curr_ref->imgY;
              }
            }
          }
        break;
      }
    }

  else {
    sPicture *vidref = vidParam->no_reference_picture;
    int noref = (currSlice->framepoc < vidParam->recovery_poc);
    int total_lists = currSlice->mb_aff_frame_flag ? 6 :
                                                    (currSlice->slice_type == B_SLICE ? 2 : 1);

    for (int j = 0; j < total_lists; j++) {
      // note that if we always set this to MAX_LIST_SIZE, we avoid crashes with invalid ref_idx being set
      // since currently this is done at the slice level, it seems safe to do so.
      // Note for some reason I get now a mismatch between version 12 and this one in cabac. I wonder why.
      for (int i = 0; i < MAX_LIST_SIZE; i++) {
        sPicture *curr_ref = currSlice->listX[j][i];
        if (curr_ref) {
          curr_ref->no_ref = noref && (curr_ref == vidref);
          curr_ref->cur_imgY = curr_ref->imgY;
          }
        }
      }
    }
  }
//}}}

//{{{
static int isNewPicture (sPicture* picture, sSlice* currSlice, OldSliceParams* p_old_slice) {

  int result = (NULL == picture);

  result |= (p_old_slice->pps_id != currSlice->pic_parameter_set_id);
  result |= (p_old_slice->frame_num != currSlice->frame_num);
  result |= (p_old_slice->field_pic_flag != currSlice->field_pic_flag);

  if (currSlice->field_pic_flag && p_old_slice->field_pic_flag)
    result |= (p_old_slice->bottom_field_flag != currSlice->bottom_field_flag);

  result |= (p_old_slice->nal_ref_idc != currSlice->nal_reference_idc) && ((p_old_slice->nal_ref_idc == 0) || (currSlice->nal_reference_idc == 0));
  result |= (p_old_slice->idr_flag != currSlice->idr_flag);

  if (currSlice->idr_flag && p_old_slice->idr_flag)
    result |= (p_old_slice->idr_pic_id != currSlice->idr_pic_id);

  sVidParam* vidParam = currSlice->vidParam;
  if (vidParam->active_sps->pic_order_cnt_type == 0) {
    result |= (p_old_slice->pic_oder_cnt_lsb != currSlice->pic_order_cnt_lsb);
    if (vidParam->active_pps->bottom_field_pic_order_in_frame_present_flag  ==  1 &&  !currSlice->field_pic_flag )
      result |= (p_old_slice->delta_pic_oder_cnt_bottom != currSlice->delta_pic_order_cnt_bottom);
    }

  if (vidParam->active_sps->pic_order_cnt_type == 1) {
    if (!vidParam->active_sps->delta_pic_order_always_zero_flag) {
      result |= (p_old_slice->delta_pic_order_cnt[0] != currSlice->delta_pic_order_cnt[0]);
      if (vidParam->active_pps->bottom_field_pic_order_in_frame_present_flag  ==  1 &&  !currSlice->field_pic_flag )
        result |= (p_old_slice->delta_pic_order_cnt[1] != currSlice->delta_pic_order_cnt[1]);
      }
    }

  result |= (currSlice->layer_id != p_old_slice->layer_id);
  return result;
  }
//}}}
//{{{
static void copyDecPicture_JV (sVidParam* vidParam, sPicture* dst, sPicture* src ) {

  dst->top_poc              = src->top_poc;
  dst->bottom_poc           = src->bottom_poc;
  dst->frame_poc            = src->frame_poc;

  dst->qp                   = src->qp;
  dst->slice_qp_delta       = src->slice_qp_delta;
  dst->chroma_qp_offset[0]  = src->chroma_qp_offset[0];
  dst->chroma_qp_offset[1]  = src->chroma_qp_offset[1];

  dst->poc                  = src->poc;

  dst->slice_type           = src->slice_type;
  dst->used_for_reference   = src->used_for_reference;
  dst->idr_flag             = src->idr_flag;
  dst->no_output_of_prior_pics_flag = src->no_output_of_prior_pics_flag;
  dst->long_term_reference_flag = src->long_term_reference_flag;
  dst->adaptive_ref_pic_buffering_flag = src->adaptive_ref_pic_buffering_flag;

  dst->dec_ref_pic_marking_buffer = src->dec_ref_pic_marking_buffer;

  dst->mb_aff_frame_flag    = src->mb_aff_frame_flag;
  dst->PicWidthInMbs        = src->PicWidthInMbs;
  dst->pic_num              = src->pic_num;
  dst->frame_num            = src->frame_num;
  dst->recovery_frame       = src->recovery_frame;
  dst->coded_frame          = src->coded_frame;

  dst->chroma_format_idc    = src->chroma_format_idc;

  dst->frame_mbs_only_flag  = src->frame_mbs_only_flag;

  dst->frame_cropping_flag  = src->frame_cropping_flag;
  dst->frame_crop_left_offset   = src->frame_crop_left_offset;
  dst->frame_crop_right_offset  = src->frame_crop_right_offset;
  dst->frame_crop_top_offset    = src->frame_crop_top_offset;
  dst->frame_crop_bottom_offset = src->frame_crop_bottom_offset;
  }
//}}}
//{{{
static void initPicture (sVidParam* vidParam, sSlice *currSlice, InputParameters *p_Inp) {

  sPicture* picture = NULL;
  sSPSrbsp* active_sps = vidParam->active_sps;
  sDPB* dpb = currSlice->dpb;

  vidParam->PicHeightInMbs = vidParam->FrameHeightInMbs / ( 1 + currSlice->field_pic_flag );
  vidParam->PicSizeInMbs = vidParam->PicWidthInMbs * vidParam->PicHeightInMbs;
  vidParam->FrameSizeInMbs = vidParam->PicWidthInMbs * vidParam->FrameHeightInMbs;
  vidParam->bFrameInit = 1;
  if (vidParam->picture)
    // this may only happen on slice loss
    exit_picture (vidParam, &vidParam->picture);

  vidParam->dpb_layer_id = currSlice->layer_id;
  setup_buffers (vidParam, currSlice->layer_id);

  if (vidParam->recovery_point)
    vidParam->recovery_frame_num = (currSlice->frame_num + vidParam->recovery_frame_cnt) % vidParam->max_frame_num;
  if (currSlice->idr_flag)
    vidParam->recovery_frame_num = currSlice->frame_num;
  if (vidParam->recovery_point == 0 &&
    currSlice->frame_num != vidParam->pre_frame_num &&
    currSlice->frame_num != (vidParam->pre_frame_num + 1) % vidParam->max_frame_num) {
    if (active_sps->gaps_in_frame_num_value_allowed_flag == 0) {
      // picture error concealment
      if (p_Inp->conceal_mode != 0) {
        if ((currSlice->frame_num) < ((vidParam->pre_frame_num + 1) % vidParam->max_frame_num)) {
          /* Conceal lost IDR frames and any frames immediately following the IDR.
          // Use frame copy for these since lists cannot be formed correctly for motion copy*/
          vidParam->conceal_mode = 1;
          vidParam->IDR_concealment_flag = 1;
          conceal_lost_frames(dpb, currSlice);
          // reset to original concealment mode for future drops
          vidParam->conceal_mode = p_Inp->conceal_mode;
          }
        else {
          // reset to original concealment mode for future drops
          vidParam->conceal_mode = p_Inp->conceal_mode;
          vidParam->IDR_concealment_flag = 0;
          conceal_lost_frames(dpb, currSlice);
          }
        }
      else
        // Advanced Error Concealment would be called here to combat unintentional loss of pictures
        error ("An unintentional loss of pictures occurs! Exit\n", 100);
      }
    if (vidParam->conceal_mode == 0)
      fill_frame_num_gap(vidParam, currSlice);
    }

  if (currSlice->nal_reference_idc)
    vidParam->pre_frame_num = currSlice->frame_num;

  // calculate POC
  decodePOC (vidParam, currSlice);

  if (vidParam->recovery_frame_num == (int) currSlice->frame_num && vidParam->recovery_poc == 0x7fffffff)
    vidParam->recovery_poc = currSlice->framepoc;

  if(currSlice->nal_reference_idc)
    vidParam->last_ref_pic_poc = currSlice->framepoc;

  if (currSlice->structure == FRAME || currSlice->structure == TOP_FIELD)
    gettime (&(vidParam->start_time));

  picture = vidParam->picture = alloc_storable_picture (vidParam, currSlice->structure, vidParam->width, vidParam->height, vidParam->width_cr, vidParam->height_cr, 1);
  picture->top_poc = currSlice->toppoc;
  picture->bottom_poc = currSlice->bottompoc;
  picture->frame_poc = currSlice->framepoc;
  picture->qp = currSlice->qp;
  picture->slice_qp_delta = currSlice->slice_qp_delta;
  picture->chroma_qp_offset[0] = vidParam->active_pps->chroma_qp_index_offset;
  picture->chroma_qp_offset[1] = vidParam->active_pps->second_chroma_qp_index_offset;
  picture->iCodingType = currSlice->structure==FRAME? (currSlice->mb_aff_frame_flag? FRAME_MB_PAIR_CODING:FRAME_CODING): FIELD_CODING; //currSlice->slice_type;
  picture->layer_id = currSlice->layer_id;

  // reset all variables of the error concealment instance before decoding of every frame.
  // here the third parameter should, if perfectly, be equal to the number of slices per frame.
  // using little value is ok, the code will allocate more memory if the slice number is larger
  ercReset (vidParam->erc_errorVar, vidParam->PicSizeInMbs, vidParam->PicSizeInMbs, picture->size_x);

  vidParam->erc_mvperMB = 0;
  switch (currSlice->structure ) {
    //{{{
    case TOP_FIELD:
      picture->poc = currSlice->toppoc;
      vidParam->number *= 2;
      break;
    //}}}
    //{{{
    case BOTTOM_FIELD:
      picture->poc = currSlice->bottompoc;
      vidParam->number = vidParam->number * 2 + 1;
      break;
    //}}}
    //{{{
    case FRAME:
      picture->poc = currSlice->framepoc;
      break;
    //}}}
    //{{{
    default:
      error ("vidParam->structure not initialized", 235);
    //}}}
    }

  //vidParam->current_slice_nr=0;
  if (vidParam->type > SI_SLICE) {
    set_ec_flag (vidParam, SE_PTYPE);
    vidParam->type = P_SLICE;  // concealed element
    }

  // CAVLC init
  if (vidParam->active_pps->entropy_coding_mode_flag == (Boolean) CAVLC)
    memset (vidParam->nz_coeff[0][0][0], -1, vidParam->PicSizeInMbs * 48 *sizeof(byte)); // 3 * 4 * 4

  // Set the slice_nr member of each MB to -1, to ensure correct when packet loss occurs
  // TO set sMacroblock Map (mark all MBs as 'have to be concealed')
  if (vidParam->separate_colour_plane_flag != 0) {
    for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
      sMacroblock* currMB = vidParam->mb_data_JV[nplane];
      char* intra_block = vidParam->intra_block_JV[nplane];
      for (int i = 0; i < (int)vidParam->PicSizeInMbs; ++i)
        reset_mbs (currMB++);
      memset (vidParam->ipredmode_JV[nplane][0], DC_PRED, 16 * vidParam->FrameHeightInMbs * vidParam->PicWidthInMbs * sizeof(char));
      if (vidParam->active_pps->constrained_intra_pred_flag)
        for (int i = 0; i < (int)vidParam->PicSizeInMbs; ++i)
          intra_block[i] = 1;
      }
    }
  else {
    sMacroblock* currMB = vidParam->mb_data;
    for (int i = 0; i < (int)vidParam->PicSizeInMbs; ++i)
      reset_mbs (currMB++);
    if (vidParam->active_pps->constrained_intra_pred_flag)
      for (int i = 0; i < (int)vidParam->PicSizeInMbs; ++i)
        vidParam->intra_block[i] = 1;
    memset (vidParam->ipredmode[0], DC_PRED, 16 * vidParam->FrameHeightInMbs * vidParam->PicWidthInMbs * sizeof(char));
    }

  picture->slice_type = vidParam->type;
  picture->used_for_reference = (currSlice->nal_reference_idc != 0);
  picture->idr_flag = currSlice->idr_flag;
  picture->no_output_of_prior_pics_flag = currSlice->no_output_of_prior_pics_flag;
  picture->long_term_reference_flag = currSlice->long_term_reference_flag;
  picture->adaptive_ref_pic_buffering_flag = currSlice->adaptive_ref_pic_buffering_flag;
  picture->dec_ref_pic_marking_buffer = currSlice->dec_ref_pic_marking_buffer;
  currSlice->dec_ref_pic_marking_buffer = NULL;

  picture->mb_aff_frame_flag = currSlice->mb_aff_frame_flag;
  picture->PicWidthInMbs = vidParam->PicWidthInMbs;

  vidParam->get_mb_block_pos = picture->mb_aff_frame_flag ? get_mb_block_pos_mbaff : get_mb_block_pos_normal;
  vidParam->getNeighbour = picture->mb_aff_frame_flag ? getAffNeighbour : getNonAffNeighbour;

  picture->pic_num = currSlice->frame_num;
  picture->frame_num = currSlice->frame_num;
  picture->recovery_frame = (unsigned int)((int)currSlice->frame_num == vidParam->recovery_frame_num);
  picture->coded_frame = (currSlice->structure==FRAME);
  picture->chroma_format_idc = active_sps->chroma_format_idc;
  picture->frame_mbs_only_flag = active_sps->frame_mbs_only_flag;
  picture->frame_cropping_flag = active_sps->frame_cropping_flag;
  if (picture->frame_cropping_flag) {
    picture->frame_crop_left_offset = active_sps->frame_crop_left_offset;
    picture->frame_crop_right_offset = active_sps->frame_crop_right_offset;
    picture->frame_crop_top_offset = active_sps->frame_crop_top_offset;
    picture->frame_crop_bottom_offset = active_sps->frame_crop_bottom_offset;
    }

  if (vidParam->separate_colour_plane_flag != 0) {
    vidParam->dec_picture_JV[0] = vidParam->picture;
    vidParam->dec_picture_JV[1] = alloc_storable_picture (vidParam, (sPictureStructure) currSlice->structure, vidParam->width, vidParam->height, vidParam->width_cr, vidParam->height_cr, 1);
    copyDecPicture_JV (vidParam, vidParam->dec_picture_JV[1], vidParam->dec_picture_JV[0] );
    vidParam->dec_picture_JV[2] = alloc_storable_picture (vidParam, (sPictureStructure) currSlice->structure, vidParam->width, vidParam->height, vidParam->width_cr, vidParam->height_cr, 1);
    copyDecPicture_JV (vidParam, vidParam->dec_picture_JV[2], vidParam->dec_picture_JV[0] );
    }
  }
//}}}
//{{{
static void initPictureDecoding (sVidParam* vidParam) {

  int iDeblockMode = 1;

  if (vidParam->iSliceNumOfCurrPic >= MAX_NUM_SLICES)
    error ("Maximum number of supported slices exceeded, increase MAX_NUM_SLICES", 200);

  sSlice* slice = vidParam->ppSliceList[0];
  if (vidParam->nextPPS->Valid &&
      (int)vidParam->nextPPS->pic_parameter_set_id == slice->pic_parameter_set_id) {
    sPPSrbsp tmpPPS;
    memcpy (&tmpPPS, &(vidParam->PicParSet[slice->pic_parameter_set_id]), sizeof (sPPSrbsp));
    (vidParam->PicParSet[slice->pic_parameter_set_id]).slice_group_id = NULL;
    makePPSavailable (vidParam, vidParam->nextPPS->pic_parameter_set_id, vidParam->nextPPS);
    memcpy (vidParam->nextPPS, &tmpPPS, sizeof (sPPSrbsp));
    tmpPPS.slice_group_id = NULL;
    }

  useParameterSet (slice);
  if (slice->idr_flag)
    vidParam->number = 0;

  vidParam->PicHeightInMbs = vidParam->FrameHeightInMbs / ( 1 + slice->field_pic_flag );
  vidParam->PicSizeInMbs = vidParam->PicWidthInMbs * vidParam->PicHeightInMbs;
  vidParam->FrameSizeInMbs = vidParam->PicWidthInMbs * vidParam->FrameHeightInMbs;
  vidParam->structure = slice->structure;
  fmo_init (vidParam, slice);

  update_pic_num (slice);

  initDeblock (vidParam, slice->mb_aff_frame_flag);
  for (int j = 0; j < vidParam->iSliceNumOfCurrPic; j++) {
    if (vidParam->ppSliceList[j]->DFDisableIdc != 1)
      iDeblockMode = 0;
    }

  vidParam->iDeblockMode = iDeblockMode;
  }
//}}}
static void framePostProcessing (sVidParam* vidParam) {}
static void fieldPostProcessing (sVidParam* vidParam) { vidParam->number /= 2; }

//{{{
static void copySliceInfo (sSlice* currSlice, OldSliceParams* p_old_slice) {

  sVidParam* vidParam = currSlice->vidParam;

  p_old_slice->pps_id = currSlice->pic_parameter_set_id;
  p_old_slice->frame_num = currSlice->frame_num; //vidParam->frame_num;
  p_old_slice->field_pic_flag = currSlice->field_pic_flag; //vidParam->field_pic_flag;

  if (currSlice->field_pic_flag)
    p_old_slice->bottom_field_flag = currSlice->bottom_field_flag;

  p_old_slice->nal_ref_idc = currSlice->nal_reference_idc;
  p_old_slice->idr_flag = (byte) currSlice->idr_flag;

  if (currSlice->idr_flag)
    p_old_slice->idr_pic_id = currSlice->idr_pic_id;

  if (vidParam->active_sps->pic_order_cnt_type == 0) {
    p_old_slice->pic_oder_cnt_lsb = currSlice->pic_order_cnt_lsb;
    p_old_slice->delta_pic_oder_cnt_bottom = currSlice->delta_pic_order_cnt_bottom;
    }

  if (vidParam->active_sps->pic_order_cnt_type == 1) {
    p_old_slice->delta_pic_order_cnt[0] = currSlice->delta_pic_order_cnt[0];
    p_old_slice->delta_pic_order_cnt[1] = currSlice->delta_pic_order_cnt[1];
    }

  p_old_slice->layer_id = currSlice->layer_id;
  }
//}}}
//{{{
static void initSlice (sVidParam* vidParam, sSlice *currSlice) {

  vidParam->active_sps = currSlice->active_sps;
  vidParam->active_pps = currSlice->active_pps;

  currSlice->init_lists (currSlice);
  reorderLists (currSlice);

  if (currSlice->structure == FRAME)
    init_mbaff_lists (vidParam, currSlice);

  // update reference flags and set current vidParam->ref_flag
  if (!(currSlice->redundant_pic_cnt != 0 && vidParam->previous_frame_num == currSlice->frame_num))
    for (int i = 16; i > 0;i--)
      currSlice->ref_flag[i] = currSlice->ref_flag[i-1];
  currSlice->ref_flag[0] = currSlice->redundant_pic_cnt == 0 ? vidParam->Is_primary_correct :
                                                               vidParam->Is_redundant_correct;

  if ((currSlice->active_sps->chroma_format_idc == 0) ||
      (currSlice->active_sps->chroma_format_idc == 3)) {
    currSlice->linfo_cbp_intra = linfo_cbp_intra_other;
    currSlice->linfo_cbp_inter = linfo_cbp_inter_other;
    }
  else {
    currSlice->linfo_cbp_intra = linfo_cbp_intra_normal;
    currSlice->linfo_cbp_inter = linfo_cbp_inter_normal;
    }
  }
//}}}
//{{{
static void decodeOneSlice (sSlice* currSlice) {

  Boolean end_of_slice = FALSE;
  currSlice->cod_counter=-1;

  sVidParam* vidParam = currSlice->vidParam;
  if ((vidParam->separate_colour_plane_flag != 0))
    change_plane_JV (vidParam, currSlice->colour_plane_id, currSlice );
  else {
    currSlice->mb_data = vidParam->mb_data;
    currSlice->picture = vidParam->picture;
    currSlice->siblock = vidParam->siblock;
    currSlice->ipredmode = vidParam->ipredmode;
    currSlice->intra_block = vidParam->intra_block;
    }

  if (currSlice->slice_type == B_SLICE)
    compute_colocated (currSlice, currSlice->listX);

  if (currSlice->slice_type != I_SLICE && currSlice->slice_type != SI_SLICE)
    init_cur_imgy (currSlice,vidParam);

  // loop over macroblocks
  while (end_of_slice == FALSE) {
    sMacroblock* currMB;
    start_macroblock (currSlice, &currMB);
    currSlice->read_one_macroblock (currMB);
    decode_one_macroblock (currMB, currSlice->picture);

    if (currSlice->mb_aff_frame_flag && currMB->mb_field) {
      currSlice->num_ref_idx_active[LIST_0] >>= 1;
      currSlice->num_ref_idx_active[LIST_1] >>= 1;
      }

    ercWriteMBMODEandMV (currMB);
    end_of_slice = exit_macroblock (currSlice, (!currSlice->mb_aff_frame_flag|| currSlice->current_mb_nr%2));
    }
  }
//}}}
//{{{
static void decodeSlice (sSlice *currSlice, int current_header) {

  if (currSlice->active_pps->entropy_coding_mode_flag) {
    init_contexts (currSlice);
    cabac_new_slice (currSlice);
    }

  if ((currSlice->active_pps->weighted_bipred_idc > 0  && (currSlice->slice_type == B_SLICE)) ||
      (currSlice->active_pps->weighted_pred_flag && currSlice->slice_type !=I_SLICE))
    fill_wp_params (currSlice);

  // decode main slice information
  if ((current_header == SOP || current_header == SOS) && currSlice->ei_flag == 0)
    decodeOneSlice (currSlice);
  }
//}}}
//{{{
static int readNewSlice (sSlice* currSlice) {

  static sNalu* pendingNalu = NULL;

  InputParameters* p_Inp = currSlice->p_Inp;
  sVidParam* vidParam = currSlice->vidParam;

  int current_header = 0;
  Bitstream* currStream = NULL;
  for (;;) {
    sNalu* nalu = vidParam->nalu;
    if (!pendingNalu) {
      if (!readNextNalu (vidParam, nalu))
        return EOS;
      }
    else {
      nalu = pendingNalu;
      pendingNalu = NULL;
      }

  process_nalu:
    switch (nalu->nal_unit_type) {
      case NALU_TYPE_SLICE:
      //{{{
      case NALU_TYPE_IDR:
        if (vidParam->recovery_point || nalu->nal_unit_type == NALU_TYPE_IDR) {
          if (vidParam->recovery_point_found == 0) {
            if (nalu->nal_unit_type != NALU_TYPE_IDR) {
              printf ("Warning: Decoding does not start with an IDR picture.\n");
              vidParam->non_conforming_stream = 1;
              }
            else
              vidParam->non_conforming_stream = 0;
            }
          vidParam->recovery_point_found = 1;
          }

        if (vidParam->recovery_point_found == 0)
          break;

        currSlice->idr_flag = (nalu->nal_unit_type == NALU_TYPE_IDR);
        currSlice->nal_reference_idc = nalu->nal_reference_idc;
        currSlice->dp_mode = PAR_DP_1;
        currSlice->max_part_nr = 1;

        currStream = currSlice->partArr[0].bitstream;
        currStream->ei_flag = 0;
        currStream->frame_bitoffset = currStream->read_len = 0;
        memcpy (currStream->streamBuffer, &nalu->buf[1], nalu->len-1);
        currStream->code_len = currStream->bitstream_length = RBSPtoSODB (currStream->streamBuffer, nalu->len-1);

        // Some syntax of the sliceHeader depends on parameter set
        // which depends on the parameter set ID of the slice header.
        // - read the pic_parameter_set_id of the slice header first,
        // - then setup the active parameter sets
        // -  read // the rest of the slice header
        firstPartOfSliceHeader (currSlice);
        useParameterSet (currSlice);
        currSlice->active_sps = vidParam->active_sps;
        currSlice->active_pps = vidParam->active_pps;
        currSlice->Transform8x8Mode = vidParam->active_pps->transform_8x8_mode_flag;
        currSlice->chroma444_not_separate = (vidParam->active_sps->chroma_format_idc == YUV444) &&
                                            (vidParam->separate_colour_plane_flag == 0);
        restOfSliceHeader (currSlice);
        assign_quant_params (currSlice);

        // if primary slice is replaced with redundant slice, set the correct image type
        if (currSlice->redundant_pic_cnt &&
            vidParam->Is_primary_correct == 0 &&
            vidParam->Is_redundant_correct)
          vidParam->picture->slice_type = vidParam->type;

        if (isNewPicture (vidParam->picture, currSlice, vidParam->old_slice)) {
          if (vidParam->iSliceNumOfCurrPic == 0)
            initPicture (vidParam, currSlice, p_Inp);
          current_header = SOP;
          // check zero_byte if it is also the first NAL unit in the access unit
          checkZeroByteVCL (vidParam, nalu);
          }
        else
          current_header = SOS;

        setup_slice_methods (currSlice);

        // Vid->active_sps, vidParam->active_pps, sliceHeader valid
        if (currSlice->mb_aff_frame_flag)
          currSlice->current_mb_nr = currSlice->start_mb_nr << 1;
        else
          currSlice->current_mb_nr = currSlice->start_mb_nr;

        if (vidParam->active_pps->entropy_coding_mode_flag) {
          int ByteStartPosition = currStream->frame_bitoffset / 8;
          if (currStream->frame_bitoffset%8 != 0)
            ++ByteStartPosition;
          arideco_start_decoding (&currSlice->partArr[0].de_cabac, currStream->streamBuffer,
                                  ByteStartPosition, &currStream->read_len);
          }

        vidParam->recovery_point = 0;
        return current_header;
        break;
      //}}}
      //{{{
      case NALU_TYPE_DPA:
        if (vidParam->recovery_point_found == 0)
          break;

        // read DP_A
        currSlice->dpB_NotPresent = 1;
        currSlice->dpC_NotPresent = 1;
        currSlice->idr_flag = FALSE;
        currSlice->nal_reference_idc = nalu->nal_reference_idc;
        currSlice->dp_mode = PAR_DP_3;
        currSlice->max_part_nr = 3;
        currStream = currSlice->partArr[0].bitstream;
        currStream->ei_flag = 0;
        currStream->frame_bitoffset = currStream->read_len = 0;
        memcpy (currStream->streamBuffer, &nalu->buf[1], nalu->len-1);
        currStream->code_len = currStream->bitstream_length = RBSPtoSODB(currStream->streamBuffer, nalu->len-1);

        firstPartOfSliceHeader (currSlice);
        useParameterSet (currSlice);
        currSlice->active_sps = vidParam->active_sps;
        currSlice->active_pps = vidParam->active_pps;
        currSlice->Transform8x8Mode = vidParam->active_pps->transform_8x8_mode_flag;
        currSlice->chroma444_not_separate = (vidParam->active_sps->chroma_format_idc == YUV444) &&
                                            (vidParam->separate_colour_plane_flag == 0);
        restOfSliceHeader (currSlice);
        assign_quant_params (currSlice);

        if (isNewPicture (vidParam->picture, currSlice, vidParam->old_slice)) {
          if (vidParam->iSliceNumOfCurrPic == 0)
            initPicture (vidParam, currSlice, p_Inp);
          current_header = SOP;
          checkZeroByteVCL (vidParam, nalu);
          }
        else
          current_header = SOS;

        setup_slice_methods (currSlice);

        // From here on, vidParam->active_sps, vidParam->active_pps and the slice header are valid
        if (currSlice->mb_aff_frame_flag)
          currSlice->current_mb_nr = currSlice->start_mb_nr << 1;
        else
          currSlice->current_mb_nr = currSlice->start_mb_nr;

        // need to read the slice ID, which depends on the value of redundant_pic_cnt_present_flag

        int slice_id_a = read_ue_v ("NALU: DP_A slice_id", currStream);

        if (vidParam->active_pps->entropy_coding_mode_flag)
          error ("data partition with CABAC not allowed", 500);

        // reading next DP
        if (!readNextNalu (vidParam, nalu))
          return current_header;
        if (NALU_TYPE_DPB == nalu->nal_unit_type) {
          //{{{  got nalu DPB
          currStream = currSlice->partArr[1].bitstream;
          currStream->ei_flag = 0;
          currStream->frame_bitoffset = currStream->read_len = 0;
          memcpy (currStream->streamBuffer, &nalu->buf[1], nalu->len-1);
          currStream->code_len = currStream->bitstream_length = RBSPtoSODB (currStream->streamBuffer, nalu->len-1);
          int slice_id_b = read_ue_v ("NALU: DP_B slice_id", currStream);
          currSlice->dpB_NotPresent = 0;

          if ((slice_id_b != slice_id_a) || (nalu->lost_packets)) {
            printf ("Waning: got a data partition B which does not match DP_A (DP loss!)\n");
            currSlice->dpB_NotPresent = 1;
            currSlice->dpC_NotPresent = 1;
            }
          else {
            if (vidParam->active_pps->redundant_pic_cnt_present_flag)
              read_ue_v ("NALU: DP_B redundant_pic_cnt", currStream);

            // we're finished with DP_B, so let's continue with next DP
            if (!readNextNalu (vidParam, nalu))
              return current_header;
            }
          }
          //}}}
        else
          currSlice->dpB_NotPresent = 1;

        if (NALU_TYPE_DPC == nalu->nal_unit_type) {
          //{{{  got nalu DPC
          currStream = currSlice->partArr[2].bitstream;
          currStream->ei_flag = 0;
          currStream->frame_bitoffset = currStream->read_len = 0;
          memcpy (currStream->streamBuffer, &nalu->buf[1], nalu->len-1);
          currStream->code_len = currStream->bitstream_length = RBSPtoSODB (currStream->streamBuffer, nalu->len-1);

          currSlice->dpC_NotPresent = 0;
          int slice_id_c = read_ue_v ("NALU: DP_C slice_id", currStream);
          if ((slice_id_c != slice_id_a)|| (nalu->lost_packets)) {
            printf ("Warning: got a data partition C which does not match DP_A(DP loss!)\n");
            currSlice->dpC_NotPresent =1;
            }

          if (vidParam->active_pps->redundant_pic_cnt_present_flag)
            read_ue_v ("NALU:SLICE_C redudand_pic_cnt", currStream);
          }
          //}}}
        else {
          currSlice->dpC_NotPresent = 1;
          pendingNalu = nalu;
          }

        // check if we read anything else than the expected partitions
        if ((nalu->nal_unit_type != NALU_TYPE_DPB) &&
            (nalu->nal_unit_type != NALU_TYPE_DPC) && (!currSlice->dpC_NotPresent))
          goto process_nalu;

        return current_header;
        break;
      //}}}
      //{{{
      case NALU_TYPE_DPB:
        printf ("found data partition B without matching DP A\n");
        break;
      //}}}
      //{{{
      case NALU_TYPE_DPC:
        printf ("found data partition C without matching DP A\n");
        break;
      //}}}
      //{{{
      case NALU_TYPE_SEI:
        InterpretSEIMessage (nalu->buf,nalu->len,vidParam, currSlice);
        break;
      //}}}
      //{{{
      case NALU_TYPE_PPS:
        processPPS (vidParam, nalu);
        break;
      //}}}
      //{{{
      case NALU_TYPE_SPS:
        processSPS (vidParam, nalu);
        break;
      //}}}
      case NALU_TYPE_AUD: break;
      case NALU_TYPE_EOSEQ: break;
      case NALU_TYPE_EOSTREAM: break;
      case NALU_TYPE_FILL: break;
      //{{{
      default:
        printf ("unknown NALU type %d, len %d\n", (int) nalu->nal_unit_type, (int) nalu->len);
        break;
      //}}}
      }
    }
  }
//}}}

//{{{
void pad_buf (sPixel* pImgBuf, int iWidth, int iHeight, int iStride, int iPadX, int iPadY) {

  int pad_width = iPadX + iWidth;
  memset (pImgBuf - iPadX, *pImgBuf, iPadX * sizeof(sPixel));
  memset (pImgBuf + iWidth, *(pImgBuf + iWidth - 1), iPadX * sizeof(sPixel));

  sPixel* pLine0 = pImgBuf - iPadX;
  sPixel* pLine = pLine0 - iPadY * iStride;
  for (int j = -iPadY; j < 0; j++) {
    memcpy (pLine, pLine0, iStride * sizeof(sPixel));
    pLine += iStride;
    }

  for (int j = 1; j < iHeight; j++) {
    pLine += iStride;
    memset (pLine, *(pLine + iPadX), iPadX * sizeof(sPixel));
    memset (pLine + pad_width, *(pLine + pad_width - 1), iPadX * sizeof(sPixel));
    }

  pLine0 = pLine + iStride;
  for (int j = iHeight; j < iHeight + iPadY; j++) {
    memcpy (pLine0,  pLine, iStride * sizeof(sPixel));
    pLine0 += iStride;
    }
  }
//}}}
//{{{
void pad_dec_picture (sVidParam* vidParam, sPicture* picture)
{
  int iPadX = vidParam->iLumaPadX;
  int iPadY = vidParam->iLumaPadY;
  int iWidth = picture->size_x;
  int iHeight = picture->size_y;
  int iStride = picture->iLumaStride;

  pad_buf (*picture->imgY, iWidth, iHeight, iStride, iPadX, iPadY);

  if (picture->chroma_format_idc != YUV400) {
    iPadX = vidParam->iChromaPadX;
    iPadY = vidParam->iChromaPadY;
    iWidth = picture->size_x_cr;
    iHeight = picture->size_y_cr;
    iStride = picture->iChromaStride;
    pad_buf (*picture->imgUV[0], iWidth, iHeight, iStride, iPadX, iPadY);
    pad_buf (*picture->imgUV[1], iWidth, iHeight, iStride, iPadX, iPadY);
    }
  }
//}}}
//{{{
void init_old_slice (OldSliceParams* p_old_slice) {

  p_old_slice->field_pic_flag = 0;
  p_old_slice->pps_id = INT_MAX;
  p_old_slice->frame_num = INT_MAX;
  p_old_slice->nal_ref_idc = INT_MAX;
  p_old_slice->idr_flag = FALSE;

  p_old_slice->pic_oder_cnt_lsb = UINT_MAX;
  p_old_slice->delta_pic_oder_cnt_bottom = INT_MAX;

  p_old_slice->delta_pic_order_cnt[0] = INT_MAX;
  p_old_slice->delta_pic_order_cnt[1] = INT_MAX;
  }
//}}}
//{{{
void calculate_frame_no (sVidParam* vidParam, sPicture *p) {

  InputParameters* p_Inp = vidParam->p_Inp;

  // calculate frame number
  int psnrPOC = vidParam->active_sps->mb_adaptive_frame_field_flag ? p->poc / (p_Inp->poc_scale) :
                                                                     p->poc / (p_Inp->poc_scale);
  if (psnrPOC == 0)
    vidParam->idr_psnr_number = vidParam->g_nFrame * vidParam->ref_poc_gap/(p_Inp->poc_scale);

  vidParam->psnr_number = imax (vidParam->psnr_number, vidParam->idr_psnr_number+psnrPOC);
  vidParam->frame_no = vidParam->idr_psnr_number + psnrPOC;
  }
//}}}
//{{{
void exit_picture (sVidParam* vidParam, sPicture** picture) {

  InputParameters* p_Inp = vidParam->p_Inp;

  // return if the last picture has already been finished
  if (*picture == NULL ||
      (vidParam->num_dec_mb != vidParam->PicSizeInMbs &&
       (vidParam->yuv_format != YUV444 || !vidParam->separate_colour_plane_flag)))
    return;

  //{{{  error concealment
  frame recfr;
  recfr.vidParam = vidParam;
  recfr.yptr = &(*picture)->imgY[0][0];
  if ((*picture)->chroma_format_idc != YUV400) {
    recfr.uptr = &(*picture)->imgUV[0][0][0];
    recfr.vptr = &(*picture)->imgUV[1][0][0];
    }

  // this is always true at the beginning of a picture
  int ercSegment = 0;

  // mark the start of the first segment
  if (!(*picture)->mb_aff_frame_flag) {
    int i;
    ercStartSegment (0, ercSegment, 0 , vidParam->erc_errorVar);
    // generate the segments according to the macroblock map
    for (i = 1; i < (int) (*picture)->PicSizeInMbs; ++i) {
      if (vidParam->mb_data[i].ei_flag != vidParam->mb_data[i-1].ei_flag) {
        ercStopSegment (i-1, ercSegment, 0, vidParam->erc_errorVar); //! stop current segment

        // mark current segment as lost or OK
        if(vidParam->mb_data[i-1].ei_flag)
          ercMarkCurrSegmentLost ((*picture)->size_x, vidParam->erc_errorVar);
        else
          ercMarkCurrSegmentOK ((*picture)->size_x, vidParam->erc_errorVar);

        ++ercSegment;  //! next segment
        ercStartSegment (i, ercSegment, 0 , vidParam->erc_errorVar); //! start new segment
        //ercStartMB = i;//! save start MB for this segment
        }
      }

    // mark end of the last segment
    ercStopSegment ((*picture)->PicSizeInMbs-1, ercSegment, 0, vidParam->erc_errorVar);
    if(vidParam->mb_data[i-1].ei_flag)
      ercMarkCurrSegmentLost ((*picture)->size_x, vidParam->erc_errorVar);
    else
      ercMarkCurrSegmentOK ((*picture)->size_x, vidParam->erc_errorVar);

    // call the right error concealment function depending on the frame type.
    vidParam->erc_mvperMB /= (*picture)->PicSizeInMbs;
    vidParam->erc_img = vidParam;
    if ((*picture)->slice_type == I_SLICE || (*picture)->slice_type == SI_SLICE) // I-frame
      ercConcealIntraFrame (vidParam, &recfr, (*picture)->size_x, (*picture)->size_y, vidParam->erc_errorVar);
    else
      ercConcealInterFrame (&recfr, vidParam->erc_object_list, (*picture)->size_x, (*picture)->size_y, vidParam->erc_errorVar, (*picture)->chroma_format_idc);
    }
  //}}}
  if (!vidParam->iDeblockMode &&
      (vidParam->bDeblockEnable & (1<<(*picture)->used_for_reference))) {
    //{{{  deblocking for frame or field
    if( (vidParam->separate_colour_plane_flag != 0) ) {
      int nplane;
      int colour_plane_id = vidParam->ppSliceList[0]->colour_plane_id;
      for( nplane=0; nplane<MAX_PLANE; ++nplane ) {
        vidParam->ppSliceList[0]->colour_plane_id = nplane;
        change_plane_JV (vidParam, nplane, NULL );
        deblockPicture (vidParam,* picture );
        }
      vidParam->ppSliceList[0]->colour_plane_id = colour_plane_id;
      make_frame_picture_JV(vidParam);
      }
    else
      deblockPicture (vidParam,* picture);
    }
    //}}}
  else if (vidParam->separate_colour_plane_flag != 0)
    make_frame_picture_JV (vidParam);

  if ((*picture)->mb_aff_frame_flag)
    mbAffPostProc (vidParam);

  if (vidParam->structure == FRAME)
    framePostProcessing (vidParam);
  else
    fieldPostProcessing (vidParam);

  if ((*picture)->used_for_reference)
    pad_dec_picture (vidParam,* picture);

  int structure = (*picture)->structure;
  int slice_type = (*picture)->slice_type;
  int frame_poc = (*picture)->frame_poc;
  int refpic = (*picture)->used_for_reference;
  int qp = (*picture)->qp;
  int pic_num = (*picture)->pic_num;
  int is_idr = (*picture)->idr_flag;
  int chroma_format_idc = (*picture)->chroma_format_idc;

  store_picture_in_dpb (vidParam->p_Dpb_layer[0],* picture);

  *picture = NULL;
  if (vidParam->last_has_mmco_5)
    vidParam->pre_frame_num = 0;

  if (structure == TOP_FIELD || structure == FRAME) {
    //{{{  frame type string
    if (slice_type == I_SLICE && is_idr) // IDR picture
      strcpy (vidParam->cslice_type,"IDR");
    else if (slice_type == I_SLICE) // I picture
      strcpy (vidParam->cslice_type," I ");
    else if (slice_type == P_SLICE) // P pictures
      strcpy (vidParam->cslice_type," P ");
    else if (slice_type == SP_SLICE) // SP pictures
      strcpy (vidParam->cslice_type,"SP ");
    else if  (slice_type == SI_SLICE)
      strcpy (vidParam->cslice_type,"SI ");
    else if (refpic) // stored B pictures
      strcpy (vidParam->cslice_type," B ");
    else // B pictures
      strcpy (vidParam->cslice_type," b ");

    if (structure == FRAME)
      strncat (vidParam->cslice_type, "       ",8-strlen(vidParam->cslice_type));
    }
    //}}}
  else if (structure == BOTTOM_FIELD) {
    //{{{  frame type string
    if (slice_type == I_SLICE && is_idr) // IDR picture
      strncat (vidParam->cslice_type,"|IDR",8-strlen(vidParam->cslice_type));
    else if (slice_type == I_SLICE) // I picture
      strncat (vidParam->cslice_type,"| I ",8-strlen(vidParam->cslice_type));
    else if (slice_type == P_SLICE) // P pictures
      strncat (vidParam->cslice_type,"| P ",8-strlen(vidParam->cslice_type));
    else if (slice_type == SP_SLICE) // SP pictures
      strncat (vidParam->cslice_type,"|SP ",8-strlen(vidParam->cslice_type));
    else if  (slice_type == SI_SLICE)
      strncat (vidParam->cslice_type,"|SI ",8-strlen(vidParam->cslice_type));
    else if (refpic) // stored B pictures
      strncat (vidParam->cslice_type,"| B ",8-strlen(vidParam->cslice_type));
    else // B pictures
      strncat (vidParam->cslice_type,"| b ",8-strlen(vidParam->cslice_type));
    }
    //}}}
  vidParam->cslice_type[8] = 0;

  if ((structure == FRAME) || structure == BOTTOM_FIELD) {
    gettime (&(vidParam->end_time));
    int64 tmp_time = timediff(&(vidParam->start_time), &(vidParam->end_time));
    vidParam->tot_time += tmp_time;
    printf ("%5d %s poc:%4d pic:%3d qp:%2d %dms\n",
            vidParam->frame_no, vidParam->cslice_type, frame_poc, pic_num, qp, (int)timenorm(tmp_time));

    if (slice_type == I_SLICE || slice_type == SI_SLICE ||
        slice_type == P_SLICE || refpic) // I or P pictures
      ++(vidParam->number);

    ++(vidParam->g_nFrame);
    }
  }
//}}}

//{{{
int decode_one_frame (sDecoderParams* pDecoder) {

  int ret = 0;

  sVidParam* vidParam = pDecoder->vidParam;
  InputParameters* p_Inp = vidParam->p_Inp;

  // read one picture first
  vidParam->iSliceNumOfCurrPic = 0;
  vidParam->iNumOfSlicesDecoded = 0;
  vidParam->num_dec_mb = 0;

  int iSliceNo = 0;
  sSlice* currSlice = NULL;
  sSlice** ppSliceList = vidParam->ppSliceList;
  int current_header = 0;
  if (vidParam->newframe) {
    if (vidParam->nextPPS->Valid) {
      //{{{  use next PPS
      makePPSavailable (vidParam, vidParam->nextPPS->pic_parameter_set_id, vidParam->nextPPS);
      vidParam->nextPPS->Valid = 0;
      }
      //}}}
    // get firstSlice from currentSlice;
    currSlice = ppSliceList[vidParam->iSliceNumOfCurrPic];
    ppSliceList[vidParam->iSliceNumOfCurrPic] = vidParam->pNextSlice;
    vidParam->pNextSlice = currSlice;
    currSlice = ppSliceList[vidParam->iSliceNumOfCurrPic];
    useParameterSet (currSlice);
    initPicture (vidParam, currSlice, p_Inp);
    vidParam->iSliceNumOfCurrPic++;
    current_header = SOS;
    }

  while (current_header != SOP && current_header != EOS) {
    //{{{  no pending slices
    assert (vidParam->iSliceNumOfCurrPic < vidParam->iNumOfSlicesAllocated);

    if (!ppSliceList[vidParam->iSliceNumOfCurrPic])
      ppSliceList[vidParam->iSliceNumOfCurrPic] = malloc_slice (p_Inp, vidParam);

    currSlice = ppSliceList[vidParam->iSliceNumOfCurrPic];
    currSlice->vidParam = vidParam;
    currSlice->p_Inp = p_Inp;
    currSlice->dpb = vidParam->p_Dpb_layer[0]; //set default value;
    currSlice->next_header = -8888;
    currSlice->num_dec_mb = 0;
    currSlice->coeff_ctr = -1;
    currSlice->pos = 0;
    currSlice->is_reset_coeff = FALSE;
    currSlice->is_reset_coeff_cr = FALSE;
    current_header = readNewSlice (currSlice);
    currSlice->current_header = current_header;

    // error tracking of primary and redundant slices.
    errorTracking (vidParam, currSlice);
    //{{{  manage primary and redundant slices
    // If primary and redundant are received and primary is correct
    //   discard the redundant
    // else,
    //   primary slice will be replaced with redundant slice.
    if (currSlice->frame_num == vidParam->previous_frame_num &&
        currSlice->redundant_pic_cnt != 0 &&
        vidParam->Is_primary_correct != 0 &&
        current_header != EOS)
      continue;

    if ((current_header != SOP && current_header != EOS) ||
        (vidParam->iSliceNumOfCurrPic == 0 && current_header == SOP)) {
       currSlice->current_slice_nr = (short)vidParam->iSliceNumOfCurrPic;
       vidParam->picture->max_slice_id =
         (short)imax (currSlice->current_slice_nr, vidParam->picture->max_slice_id);
       if (vidParam->iSliceNumOfCurrPic > 0) {
         copyPOC (*ppSliceList, currSlice);
         ppSliceList[vidParam->iSliceNumOfCurrPic-1]->end_mb_nr_plus1 = currSlice->start_mb_nr;
         }

       vidParam->iSliceNumOfCurrPic++;
       if (vidParam->iSliceNumOfCurrPic >= vidParam->iNumOfSlicesAllocated) {
         sSlice** tmpSliceList = (sSlice**)realloc (
           vidParam->ppSliceList, (vidParam->iNumOfSlicesAllocated+MAX_NUM_DECSLICES)*sizeof(sSlice*));
         if (!tmpSliceList) {
           tmpSliceList = calloc ((vidParam->iNumOfSlicesAllocated + MAX_NUM_DECSLICES), sizeof(sSlice*));
           memcpy (tmpSliceList, vidParam->ppSliceList, vidParam->iSliceNumOfCurrPic*sizeof(sSlice*));
           free (vidParam->ppSliceList);
           ppSliceList = vidParam->ppSliceList = tmpSliceList;
           }
         else {
           ppSliceList = vidParam->ppSliceList = tmpSliceList;
           memset (vidParam->ppSliceList + vidParam->iSliceNumOfCurrPic, 0, sizeof(sSlice*)*MAX_NUM_DECSLICES);
           }
         vidParam->iNumOfSlicesAllocated += MAX_NUM_DECSLICES;
        }
      current_header = SOS;
      }
    else {
      if (ppSliceList[vidParam->iSliceNumOfCurrPic-1]->mb_aff_frame_flag)
        ppSliceList[vidParam->iSliceNumOfCurrPic-1]->end_mb_nr_plus1 = vidParam->FrameSizeInMbs/2;
      else
        ppSliceList[vidParam->iSliceNumOfCurrPic-1]->end_mb_nr_plus1 = vidParam->FrameSizeInMbs/(1+ppSliceList[vidParam->iSliceNumOfCurrPic-1]->field_pic_flag);
      vidParam->newframe = 1;
      currSlice->current_slice_nr = 0;

      // keep it in currentslice;
      ppSliceList[vidParam->iSliceNumOfCurrPic] = vidParam->pNextSlice;
      vidParam->pNextSlice = currSlice;
      }
    //}}}

    copySliceInfo (currSlice, vidParam->old_slice);
    }
    //}}}

  // decode slices
  ret = current_header;
  initPictureDecoding (vidParam);
  for (iSliceNo = 0; iSliceNo < vidParam->iSliceNumOfCurrPic; iSliceNo++) {
    currSlice = ppSliceList[iSliceNo];
    current_header = currSlice->current_header;
    assert (current_header != EOS);
    assert (currSlice->current_slice_nr == iSliceNo);

    initSlice (vidParam, currSlice);
    decodeSlice (currSlice, current_header);
    vidParam->iNumOfSlicesDecoded++;
    vidParam->num_dec_mb += currSlice->num_dec_mb;
    vidParam->erc_mvperMB += currSlice->erc_mvperMB;
    }

  if (vidParam->picture->structure == FRAME)
    vidParam->last_dec_poc = vidParam->picture->frame_poc;
  else if (vidParam->picture->structure == TOP_FIELD)
    vidParam->last_dec_poc = vidParam->picture->top_poc;
  else if (vidParam->picture->structure == BOTTOM_FIELD)
    vidParam->last_dec_poc = vidParam->picture->bottom_poc;

  exit_picture (vidParam, &vidParam->picture);
  vidParam->previous_frame_num = ppSliceList[0]->frame_num;

  return ret;
  }
//}}}
