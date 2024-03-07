//{{{  includes
#include "global.h"
#include "memory.h"

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
static void resetMbs (sMacroblock *curMb) {

  curMb->slice_nr = -1;
  curMb->ei_flag =  1;
  curMb->dpl_flag =  0;
  }
//}}}
//{{{
static void setupBuffers (sVidParam* vidParam, int layerId) {

  if (vidParam->last_dec_layer_id != layerId) {
    sCodingParam* cps = vidParam->p_EncodePar[layerId];
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
    vidParam->qpPerMatrix = cps->qpPerMatrix;
    vidParam->qpRemMatrix = cps->qpRemMatrix;
    vidParam->oldFrameSizeInMbs = cps->oldFrameSizeInMbs;
    vidParam->last_dec_layer_id = layerId;
    }
  }
//}}}
//{{{
static void pad_buf (sPixel* pImgBuf, int iWidth, int iHeight, int iStride, int iPadX, int iPadY) {

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
static void update_mbaff_macroblock_data (sPixel** curPixel, sPixel (*temp)[16], int x0, int width, int height) {

  sPixel (*temp_evn)[16] = temp;
  sPixel (*temp_odd)[16] = temp + height;
  sPixel** temp_img = curPixel;

  for (int y = 0; y < 2 * height; ++y)
    memcpy (*temp++, (*temp_img++ + x0), width * sizeof(sPixel));

  for (int y = 0; y < height; ++y) {
    memcpy ((*curPixel++ + x0), *temp_evn++, width * sizeof(sPixel));
    memcpy ((*curPixel++ + x0), *temp_odd++, width * sizeof(sPixel));
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
static void fill_WPParam (sSlice *curSlice) {

  if (curSlice->slice_type == B_SLICE) {
    int comp;
    int log_weight_denom;
    int tb, td;
    int tx, DistScaleFactor;

    int max_l0_ref = curSlice->num_ref_idx_active[LIST_0];
    int max_l1_ref = curSlice->num_ref_idx_active[LIST_1];

    if (curSlice->activePPS->weighted_bipred_idc == 2) {
      curSlice->luma_log2_weight_denom = 5;
      curSlice->chroma_log2_weight_denom = 5;
      curSlice->wp_round_luma = 16;
      curSlice->wp_round_chroma = 16;
      for (int i = 0; i < MAX_REFERENCE_PICTURES; ++i)
        for (comp = 0; comp < 3; ++comp) {
          log_weight_denom = (comp == 0) ? curSlice->luma_log2_weight_denom : curSlice->chroma_log2_weight_denom;
          curSlice->wp_weight[0][i][comp] = 1 << log_weight_denom;
          curSlice->wp_weight[1][i][comp] = 1 << log_weight_denom;
          curSlice->wp_offset[0][i][comp] = 0;
          curSlice->wp_offset[1][i][comp] = 0;
          }
      }

    for (int i = 0; i < max_l0_ref; ++i)
      for (int j = 0; j < max_l1_ref; ++j)
        for (comp = 0; comp<3; ++comp) {
          log_weight_denom = (comp == 0) ? curSlice->luma_log2_weight_denom : curSlice->chroma_log2_weight_denom;
          if (curSlice->activePPS->weighted_bipred_idc == 1) {
            curSlice->wbp_weight[0][i][j][comp] =  curSlice->wp_weight[0][i][comp];
            curSlice->wbp_weight[1][i][j][comp] =  curSlice->wp_weight[1][j][comp];
            }
          else if (curSlice->activePPS->weighted_bipred_idc == 2) {
            td = iClip3(-128,127,curSlice->listX[LIST_1][j]->poc - curSlice->listX[LIST_0][i]->poc);
            if (td == 0 ||
                curSlice->listX[LIST_1][j]->is_long_term ||
                curSlice->listX[LIST_0][i]->is_long_term) {
              curSlice->wbp_weight[0][i][j][comp] = 32;
              curSlice->wbp_weight[1][i][j][comp] = 32;
              }
            else {
              tb = iClip3(-128,127,curSlice->ThisPOC - curSlice->listX[LIST_0][i]->poc);
              tx = (16384 + iabs(td/2))/td;
              DistScaleFactor = iClip3(-1024, 1023, (tx*tb + 32 )>>6);
              curSlice->wbp_weight[1][i][j][comp] = DistScaleFactor >> 2;
              curSlice->wbp_weight[0][i][j][comp] = 64 - curSlice->wbp_weight[1][i][j][comp];
              if (curSlice->wbp_weight[1][i][j][comp] < -64 || curSlice->wbp_weight[1][i][j][comp] > 128) {
                curSlice->wbp_weight[0][i][j][comp] = 32;
                curSlice->wbp_weight[1][i][j][comp] = 32;
                curSlice->wp_offset[0][i][comp] = 0;
                curSlice->wp_offset[1][j][comp] = 0;
                }
              }
            }
          }

    if (curSlice->mb_aff_frame_flag)
      for (int i = 0; i < 2*max_l0_ref; ++i)
        for (int j = 0; j < 2*max_l1_ref; ++j)
          for (comp = 0; comp<3; ++comp)
            for (int k = 2; k < 6; k += 2) {
              curSlice->wp_offset[k+0][i][comp] = curSlice->wp_offset[0][i>>1][comp];
              curSlice->wp_offset[k+1][j][comp] = curSlice->wp_offset[1][j>>1][comp];
              log_weight_denom = (comp == 0) ? curSlice->luma_log2_weight_denom : curSlice->chroma_log2_weight_denom;
              if (curSlice->activePPS->weighted_bipred_idc == 1) {
                curSlice->wbp_weight[k+0][i][j][comp] = curSlice->wp_weight[0][i>>1][comp];
                curSlice->wbp_weight[k+1][i][j][comp] = curSlice->wp_weight[1][j>>1][comp];
                }
              else if (curSlice->activePPS->weighted_bipred_idc == 2) {
                td = iClip3 (-128, 127, curSlice->listX[k+LIST_1][j]->poc - curSlice->listX[k+LIST_0][i]->poc);
                if (td == 0 ||
                    curSlice->listX[k+LIST_1][j]->is_long_term ||
                    curSlice->listX[k+LIST_0][i]->is_long_term) {
                  curSlice->wbp_weight[k+0][i][j][comp] = 32;
                  curSlice->wbp_weight[k+1][i][j][comp] = 32;
                  }
                else {
                  tb = iClip3(-128,127, ((k == 2) ? curSlice->topPoc :
                                                    curSlice->botPoc) - curSlice->listX[k+LIST_0][i]->poc);
                  tx = (16384 + iabs(td/2)) / td;
                  DistScaleFactor = iClip3(-1024, 1023, (tx*tb + 32 )>>6);
                  curSlice->wbp_weight[k+1][i][j][comp] = DistScaleFactor >> 2;
                  curSlice->wbp_weight[k+0][i][j][comp] = 64 - curSlice->wbp_weight[k+1][i][j][comp];
                  if (curSlice->wbp_weight[k+1][i][j][comp] < -64 ||
                      curSlice->wbp_weight[k+1][i][j][comp] > 128) {
                    curSlice->wbp_weight[k+1][i][j][comp] = 32;
                    curSlice->wbp_weight[k+0][i][j][comp] = 32;
                    curSlice->wp_offset[k+0][i][comp] = 0;
                    curSlice->wp_offset[k+1][j][comp] = 0;
                    }
                  }
                }
              }
    }
  }
//}}}
//{{{
static void errorTracking (sVidParam* vidParam, sSlice *curSlice) {

  if (curSlice->redundant_pic_cnt == 0)
    vidParam->Is_primary_correct = vidParam->Is_redundant_correct = 1;

  if (curSlice->redundant_pic_cnt == 0 && vidParam->type != I_SLICE) {
    for (int i = 0; i < curSlice->num_ref_idx_active[LIST_0];++i)
      if (curSlice->ref_flag[i] == 0)  // any reference of primary slice is incorrect
        vidParam->Is_primary_correct = 0; // primary slice is incorrect
    }
  else if (curSlice->redundant_pic_cnt != 0 && vidParam->type != I_SLICE)
    // reference of redundant slice is incorrect
    if (curSlice->ref_flag[curSlice->redundant_slice_ref_idx] == 0)
      // redundant slice is incorrect
      vidParam->Is_redundant_correct = 0;
  }
//}}}
//{{{
static void copyPOC (sSlice *pSlice0, sSlice *curSlice) {

  curSlice->framePoc  = pSlice0->framePoc;
  curSlice->topPoc    = pSlice0->topPoc;
  curSlice->botPoc = pSlice0->botPoc;
  curSlice->ThisPOC   = pSlice0->ThisPOC;
  }
//}}}
//{{{
static void reorderLists (sSlice* curSlice) {

  sVidParam* vidParam = curSlice->vidParam;

  if ((curSlice->slice_type != I_SLICE)&&(curSlice->slice_type != SI_SLICE)) {
    if (curSlice->ref_pic_list_reordering_flag[LIST_0])
      reorder_ref_pic_list(curSlice, LIST_0);
    if (vidParam->no_reference_picture == curSlice->listX[0][curSlice->num_ref_idx_active[LIST_0] - 1]) {
      if (vidParam->nonConformingStream)
        printf("RefPicList0[ %d ] is equal to 'no reference picture'\n", curSlice->num_ref_idx_active[LIST_0] - 1);
      else
        error("RefPicList0[ num_ref_idx_l0_active_minus1 ] is equal to 'no reference picture', invalid bitstream",500);
      }
    // that's a definition
    curSlice->listXsize[0] = (char) curSlice->num_ref_idx_active[LIST_0];
    }

  if (curSlice->slice_type == B_SLICE) {
    if (curSlice->ref_pic_list_reordering_flag[LIST_1])
      reorder_ref_pic_list(curSlice, LIST_1);
    if (vidParam->no_reference_picture == curSlice->listX[1][curSlice->num_ref_idx_active[LIST_1]-1]) {
      if (vidParam->nonConformingStream)
        printf("RefPicList1[ %d ] is equal to 'no reference picture'\n", curSlice->num_ref_idx_active[LIST_1] - 1);
      else
        error("RefPicList1[ num_ref_idx_l1_active_minus1 ] is equal to 'no reference picture', invalid bitstream",500);
      }
    // that's a definition
    curSlice->listXsize[1] = (char) curSlice->num_ref_idx_active[LIST_1];
    }

  free_ref_pic_list_reordering_buffer(curSlice);
  }
//}}}
//{{{
static void ercWriteMBMODEandMV (sMacroblock* curMb) {

  sVidParam* vidParam = curMb->vidParam;
  int currMBNum = curMb->mbAddrX; //vidParam->currentSlice->current_mb_nr;
  sPicture* picture = vidParam->picture;
  int mbx = xPosMB(currMBNum, picture->size_x), mby = yPosMB(currMBNum, picture->size_x);

  objectBuffer_t* currRegion = vidParam->erc_object_list + (currMBNum<<2);

  if (vidParam->type != B_SLICE) {
    //{{{  non-B frame
    for (int i = 0; i < 4; ++i) {
      objectBuffer_t* pRegion = currRegion + i;
      pRegion->regionMode = (curMb->mb_type  ==I16MB  ? REGMODE_INTRA :
                               curMb->b8mode[i] == IBLOCK ? REGMODE_INTRA_8x8  :
                                 curMb->b8mode[i] == 0 ? REGMODE_INTER_COPY :
                                   curMb->b8mode[i] == 1 ? REGMODE_INTER_PRED :
                                     REGMODE_INTER_PRED_8x8);

      if (curMb->b8mode[i] == 0 || curMb->b8mode[i] == IBLOCK) {
        // INTRA OR COPY
        pRegion->mv[0] = 0;
        pRegion->mv[1] = 0;
        pRegion->mv[2] = 0;
        }
      else {
        int ii = 4*mbx + (i & 0x01)*2;// + BLOCK_SIZE;
        int jj = 4*mby + (i >> 1  )*2;
        if (curMb->b8mode[i]>=5 && curMb->b8mode[i]<=7) {
          // SMALL BLOCKS
          pRegion->mv[0] = (picture->mv_info[jj][ii].mv[LIST_0].mv_x + picture->mv_info[jj][ii + 1].mv[LIST_0].mv_x + picture->mv_info[jj + 1][ii].mv[LIST_0].mv_x + picture->mv_info[jj + 1][ii + 1].mv[LIST_0].mv_x + 2)/4;
          pRegion->mv[1] = (picture->mv_info[jj][ii].mv[LIST_0].mv_y + picture->mv_info[jj][ii + 1].mv[LIST_0].mv_y + picture->mv_info[jj + 1][ii].mv[LIST_0].mv_y + picture->mv_info[jj + 1][ii + 1].mv[LIST_0].mv_y + 2)/4;
          }
        else {
          // 16x16, 16x8, 8x16, 8x8
          pRegion->mv[0] = picture->mv_info[jj][ii].mv[LIST_0].mv_x;
          pRegion->mv[1] = picture->mv_info[jj][ii].mv[LIST_0].mv_y;
          }

        curMb->slice->erc_mvperMB += iabs(pRegion->mv[0]) + iabs(pRegion->mv[1]);
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
      pRegion->regionMode = (curMb->mb_type  ==I16MB  ? REGMODE_INTRA :
                               curMb->b8mode[i]==IBLOCK ? REGMODE_INTRA_8x8 :
                                 REGMODE_INTER_PRED_8x8);

      if (curMb->mb_type==I16MB || curMb->b8mode[i]==IBLOCK) {
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
        curMb->slice->erc_mvperMB += iabs(pRegion->mv[0]) + iabs(pRegion->mv[1]);

        pRegion->mv[2]  = (picture->mv_info[jj][ii].ref_idx[idx]);
        }
      }
    }
    //}}}
  }
//}}}
//{{{
static void init_cur_imgy (sSlice* curSlice, sVidParam* vidParam) {

  if ((vidParam->separate_colour_plane_flag != 0)) {
    sPicture* vidref = vidParam->no_reference_picture;
    int noref = (curSlice->framePoc < vidParam->recovery_poc);
    switch (curSlice->colour_plane_id) {
      case 0:
        for (int j = 0; j < 6; j++) { //for (j = 0; j < (curSlice->slice_type==B_SLICE?2:1); j++) {
          for (int i = 0; i < MAX_LIST_SIZE; i++) {
            sPicture* curRef = curSlice->listX[j][i];
            if (curRef) {
              curRef->no_ref = noref && (curRef == vidref);
              curRef->curPixelY = curRef->imgY;
              }
            }
          }
        break;
      }
    }

  else {
    sPicture *vidref = vidParam->no_reference_picture;
    int noref = (curSlice->framePoc < vidParam->recovery_poc);
    int total_lists = curSlice->mb_aff_frame_flag ? 6 :
                                                    (curSlice->slice_type == B_SLICE ? 2 : 1);

    for (int j = 0; j < total_lists; j++) {
      // note that if we always set this to MAX_LIST_SIZE, we avoid crashes with invalid ref_idx being set
      // since currently this is done at the slice level, it seems safe to do so.
      // Note for some reason I get now a mismatch between version 12 and this one in cabac. I wonder why.
      for (int i = 0; i < MAX_LIST_SIZE; i++) {
        sPicture *curRef = curSlice->listX[j][i];
        if (curRef) {
          curRef->no_ref = noref && (curRef == vidref);
          curRef->curPixelY = curRef->imgY;
          }
        }
      }
    }
  }
//}}}

//{{{
static int isNewPicture (sPicture* picture, sSlice* curSlice, sOldSliceParam* oldSliceParam) {

  int result = (NULL == picture);

  result |= (oldSliceParam->pps_id != curSlice->pic_parameter_set_id);
  result |= (oldSliceParam->frame_num != curSlice->frame_num);
  result |= (oldSliceParam->field_pic_flag != curSlice->field_pic_flag);

  if (curSlice->field_pic_flag && oldSliceParam->field_pic_flag)
    result |= (oldSliceParam->bottom_field_flag != curSlice->bottom_field_flag);

  result |= (oldSliceParam->nal_ref_idc != curSlice->nalRefId) && ((oldSliceParam->nal_ref_idc == 0) || (curSlice->nalRefId == 0));
  result |= (oldSliceParam->idrFlag != curSlice->idrFlag);

  if (curSlice->idrFlag && oldSliceParam->idrFlag)
    result |= (oldSliceParam->idrPicId != curSlice->idrPicId);

  sVidParam* vidParam = curSlice->vidParam;
  if (vidParam->activeSPS->pic_order_cnt_type == 0) {
    result |= (oldSliceParam->pic_oder_cnt_lsb != curSlice->pic_order_cnt_lsb);
    if (vidParam->activePPS->bottom_field_pic_order_in_frame_present_flag  ==  1 &&  !curSlice->field_pic_flag )
      result |= (oldSliceParam->delta_pic_oder_cnt_bottom != curSlice->delta_pic_order_cnt_bottom);
    }

  if (vidParam->activeSPS->pic_order_cnt_type == 1) {
    if (!vidParam->activeSPS->delta_pic_order_always_zero_flag) {
      result |= (oldSliceParam->delta_pic_order_cnt[0] != curSlice->delta_pic_order_cnt[0]);
      if (vidParam->activePPS->bottom_field_pic_order_in_frame_present_flag  ==  1 &&  !curSlice->field_pic_flag )
        result |= (oldSliceParam->delta_pic_order_cnt[1] != curSlice->delta_pic_order_cnt[1]);
      }
    }

  result |= (curSlice->layerId != oldSliceParam->layerId);
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
  dst->idrFlag             = src->idrFlag;
  dst->no_output_of_prior_pics_flag = src->no_output_of_prior_pics_flag;
  dst->long_term_reference_flag = src->long_term_reference_flag;
  dst->adaptive_ref_pic_buffering_flag = src->adaptive_ref_pic_buffering_flag;

  dst->decRefPicMarkingBuffer = src->decRefPicMarkingBuffer;

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
static void initPicture (sVidParam* vidParam, sSlice *curSlice, sInputParam *inputParam) {

  sPicture* picture = NULL;
  sSPS* activeSPS = vidParam->activeSPS;
  sDPB* dpb = curSlice->dpb;

  vidParam->PicHeightInMbs = vidParam->FrameHeightInMbs / ( 1 + curSlice->field_pic_flag );
  vidParam->PicSizeInMbs = vidParam->PicWidthInMbs * vidParam->PicHeightInMbs;
  vidParam->FrameSizeInMbs = vidParam->PicWidthInMbs * vidParam->FrameHeightInMbs;
  vidParam->bFrameInit = 1;
  if (vidParam->picture)
    // this may only happen on slice loss
    exitPicture (vidParam, &vidParam->picture);

  vidParam->dpb_layer_id = curSlice->layerId;
  setupBuffers (vidParam, curSlice->layerId);

  if (vidParam->recovery_point)
    vidParam->recovery_frame_num = (curSlice->frame_num + vidParam->recovery_frame_cnt) % vidParam->max_frame_num;
  if (curSlice->idrFlag)
    vidParam->recovery_frame_num = curSlice->frame_num;
  if (vidParam->recovery_point == 0 &&
    curSlice->frame_num != vidParam->preFrameNum &&
    curSlice->frame_num != (vidParam->preFrameNum + 1) % vidParam->max_frame_num) {
    if (activeSPS->gaps_in_frame_num_value_allowed_flag == 0) {
      // picture error concealment
      if (inputParam->conceal_mode != 0) {
        if ((curSlice->frame_num) < ((vidParam->preFrameNum + 1) % vidParam->max_frame_num)) {
          /* Conceal lost IDR frames and any frames immediately following the IDR.
          // Use frame copy for these since lists cannot be formed correctly for motion copy*/
          vidParam->conceal_mode = 1;
          vidParam->IDR_concealment_flag = 1;
          conceal_lost_frames(dpb, curSlice);
          // reset to original concealment mode for future drops
          vidParam->conceal_mode = inputParam->conceal_mode;
          }
        else {
          // reset to original concealment mode for future drops
          vidParam->conceal_mode = inputParam->conceal_mode;
          vidParam->IDR_concealment_flag = 0;
          conceal_lost_frames(dpb, curSlice);
          }
        }
      else
        // Advanced Error Concealment would be called here to combat unintentional loss of pictures
        error ("An unintentional loss of pictures occurs! Exit\n", 100);
      }
    if (vidParam->conceal_mode == 0)
      fill_frame_num_gap(vidParam, curSlice);
    }

  if (curSlice->nalRefId)
    vidParam->preFrameNum = curSlice->frame_num;

  // calculate POC
  decodePOC (vidParam, curSlice);

  if (vidParam->recovery_frame_num == (int) curSlice->frame_num && vidParam->recovery_poc == 0x7fffffff)
    vidParam->recovery_poc = curSlice->framePoc;

  if(curSlice->nalRefId)
    vidParam->last_ref_pic_poc = curSlice->framePoc;

  if (curSlice->structure == FRAME || curSlice->structure == TOP_FIELD)
    gettime (&(vidParam->startTime));

  picture = vidParam->picture = allocPicture (vidParam, curSlice->structure, vidParam->width, vidParam->height, vidParam->width_cr, vidParam->height_cr, 1);
  picture->top_poc = curSlice->topPoc;
  picture->bottom_poc = curSlice->botPoc;
  picture->frame_poc = curSlice->framePoc;
  picture->qp = curSlice->qp;
  picture->slice_qp_delta = curSlice->slice_qp_delta;
  picture->chroma_qp_offset[0] = vidParam->activePPS->chroma_qp_index_offset;
  picture->chroma_qp_offset[1] = vidParam->activePPS->second_chroma_qp_index_offset;
  picture->iCodingType = curSlice->structure==FRAME? (curSlice->mb_aff_frame_flag? FRAME_MB_PAIR_CODING:FRAME_CODING): FIELD_CODING; //curSlice->slice_type;
  picture->layerId = curSlice->layerId;

  // reset all variables of the error concealment instance before decoding of every frame.
  // here the third parameter should, if perfectly, be equal to the number of slices per frame.
  // using little value is ok, the code will allocate more memory if the slice number is larger
  ercReset (vidParam->erc_errorVar, vidParam->PicSizeInMbs, vidParam->PicSizeInMbs, picture->size_x);

  vidParam->erc_mvperMB = 0;
  switch (curSlice->structure ) {
    //{{{
    case TOP_FIELD:
      picture->poc = curSlice->topPoc;
      vidParam->number *= 2;
      break;
    //}}}
    //{{{
    case BOTTOM_FIELD:
      picture->poc = curSlice->botPoc;
      vidParam->number = vidParam->number * 2 + 1;
      break;
    //}}}
    //{{{
    case FRAME:
      picture->poc = curSlice->framePoc;
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
  if (vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC)
    memset (vidParam->nz_coeff[0][0][0], -1, vidParam->PicSizeInMbs * 48 *sizeof(byte)); // 3 * 4 * 4

  // Set the slice_nr member of each MB to -1, to ensure correct when packet loss occurs
  // TO set sMacroblock Map (mark all MBs as 'have to be concealed')
  if (vidParam->separate_colour_plane_flag != 0) {
    for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
      sMacroblock* curMb = vidParam->mb_data_JV[nplane];
      char* intra_block = vidParam->intra_block_JV[nplane];
      for (int i = 0; i < (int)vidParam->PicSizeInMbs; ++i)
        resetMbs (curMb++);
      memset (vidParam->ipredmode_JV[nplane][0], DC_PRED, 16 * vidParam->FrameHeightInMbs * vidParam->PicWidthInMbs * sizeof(char));
      if (vidParam->activePPS->constrained_intra_pred_flag)
        for (int i = 0; i < (int)vidParam->PicSizeInMbs; ++i)
          intra_block[i] = 1;
      }
    }
  else {
    sMacroblock* curMb = vidParam->mb_data;
    for (int i = 0; i < (int)vidParam->PicSizeInMbs; ++i)
      resetMbs (curMb++);
    if (vidParam->activePPS->constrained_intra_pred_flag)
      for (int i = 0; i < (int)vidParam->PicSizeInMbs; ++i)
        vidParam->intra_block[i] = 1;
    memset (vidParam->ipredmode[0], DC_PRED, 16 * vidParam->FrameHeightInMbs * vidParam->PicWidthInMbs * sizeof(char));
    }

  picture->slice_type = vidParam->type;
  picture->used_for_reference = (curSlice->nalRefId != 0);
  picture->idrFlag = curSlice->idrFlag;
  picture->no_output_of_prior_pics_flag = curSlice->no_output_of_prior_pics_flag;
  picture->long_term_reference_flag = curSlice->long_term_reference_flag;
  picture->adaptive_ref_pic_buffering_flag = curSlice->adaptive_ref_pic_buffering_flag;
  picture->decRefPicMarkingBuffer = curSlice->decRefPicMarkingBuffer;
  curSlice->decRefPicMarkingBuffer = NULL;

  picture->mb_aff_frame_flag = curSlice->mb_aff_frame_flag;
  picture->PicWidthInMbs = vidParam->PicWidthInMbs;

  vidParam->get_mb_block_pos = picture->mb_aff_frame_flag ? get_mb_block_pos_mbaff : get_mb_block_pos_normal;
  vidParam->getNeighbour = picture->mb_aff_frame_flag ? getAffNeighbour : getNonAffNeighbour;

  picture->pic_num = curSlice->frame_num;
  picture->frame_num = curSlice->frame_num;
  picture->recovery_frame = (unsigned int)((int)curSlice->frame_num == vidParam->recovery_frame_num);
  picture->coded_frame = (curSlice->structure==FRAME);
  picture->chroma_format_idc = activeSPS->chroma_format_idc;
  picture->frame_mbs_only_flag = activeSPS->frame_mbs_only_flag;
  picture->frame_cropping_flag = activeSPS->frame_cropping_flag;
  if (picture->frame_cropping_flag) {
    picture->frame_crop_left_offset = activeSPS->frame_crop_left_offset;
    picture->frame_crop_right_offset = activeSPS->frame_crop_right_offset;
    picture->frame_crop_top_offset = activeSPS->frame_crop_top_offset;
    picture->frame_crop_bottom_offset = activeSPS->frame_crop_bottom_offset;
    }

  if (vidParam->separate_colour_plane_flag != 0) {
    vidParam->dec_picture_JV[0] = vidParam->picture;
    vidParam->dec_picture_JV[1] = allocPicture (vidParam, (sPictureStructure) curSlice->structure, vidParam->width, vidParam->height, vidParam->width_cr, vidParam->height_cr, 1);
    copyDecPicture_JV (vidParam, vidParam->dec_picture_JV[1], vidParam->dec_picture_JV[0] );
    vidParam->dec_picture_JV[2] = allocPicture (vidParam, (sPictureStructure) curSlice->structure, vidParam->width, vidParam->height, vidParam->width_cr, vidParam->height_cr, 1);
    copyDecPicture_JV (vidParam, vidParam->dec_picture_JV[2], vidParam->dec_picture_JV[0] );
    }
  }
//}}}
//{{{
static void initPictureDecoding (sVidParam* vidParam) {

  int deblockMode = 1;

  if (vidParam->iSliceNumOfCurrPic >= MAX_NUM_SLICES)
    error ("Maximum number of supported slices exceeded, increase MAX_NUM_SLICES", 200);

  sSlice* slice = vidParam->ppSliceList[0];
  if (vidParam->nextPPS->Valid &&
      (int)vidParam->nextPPS->pic_parameter_set_id == slice->pic_parameter_set_id) {
    sPPS tmpPPS;
    memcpy (&tmpPPS, &(vidParam->PicParSet[slice->pic_parameter_set_id]), sizeof (sPPS));
    (vidParam->PicParSet[slice->pic_parameter_set_id]).slice_group_id = NULL;
    makePPSavailable (vidParam, vidParam->nextPPS->pic_parameter_set_id, vidParam->nextPPS);
    memcpy (vidParam->nextPPS, &tmpPPS, sizeof (sPPS));
    tmpPPS.slice_group_id = NULL;
    }

  useParameterSet (slice);
  if (slice->idrFlag)
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
      deblockMode = 0;
    }

  vidParam->deblockMode = deblockMode;
  }
//}}}
static void framePostProcessing (sVidParam* vidParam) {}
static void fieldPostProcessing (sVidParam* vidParam) { vidParam->number /= 2; }

//{{{
static void copySliceInfo (sSlice* curSlice, sOldSliceParam* oldSliceParam) {

  sVidParam* vidParam = curSlice->vidParam;

  oldSliceParam->pps_id = curSlice->pic_parameter_set_id;
  oldSliceParam->frame_num = curSlice->frame_num; //vidParam->frame_num;
  oldSliceParam->field_pic_flag = curSlice->field_pic_flag; //vidParam->field_pic_flag;

  if (curSlice->field_pic_flag)
    oldSliceParam->bottom_field_flag = curSlice->bottom_field_flag;

  oldSliceParam->nal_ref_idc = curSlice->nalRefId;
  oldSliceParam->idrFlag = (byte) curSlice->idrFlag;

  if (curSlice->idrFlag)
    oldSliceParam->idrPicId = curSlice->idrPicId;

  if (vidParam->activeSPS->pic_order_cnt_type == 0) {
    oldSliceParam->pic_oder_cnt_lsb = curSlice->pic_order_cnt_lsb;
    oldSliceParam->delta_pic_oder_cnt_bottom = curSlice->delta_pic_order_cnt_bottom;
    }

  if (vidParam->activeSPS->pic_order_cnt_type == 1) {
    oldSliceParam->delta_pic_order_cnt[0] = curSlice->delta_pic_order_cnt[0];
    oldSliceParam->delta_pic_order_cnt[1] = curSlice->delta_pic_order_cnt[1];
    }

  oldSliceParam->layerId = curSlice->layerId;
  }
//}}}
//{{{
static void initSlice (sVidParam* vidParam, sSlice *curSlice) {

  vidParam->activeSPS = curSlice->activeSPS;
  vidParam->activePPS = curSlice->activePPS;

  curSlice->init_lists (curSlice);
  reorderLists (curSlice);

  if (curSlice->structure == FRAME)
    init_mbaff_lists (vidParam, curSlice);

  // update reference flags and set current vidParam->ref_flag
  if (!(curSlice->redundant_pic_cnt != 0 && vidParam->previous_frame_num == curSlice->frame_num))
    for (int i = 16; i > 0;i--)
      curSlice->ref_flag[i] = curSlice->ref_flag[i-1];
  curSlice->ref_flag[0] = curSlice->redundant_pic_cnt == 0 ? vidParam->Is_primary_correct :
                                                               vidParam->Is_redundant_correct;

  if ((curSlice->activeSPS->chroma_format_idc == 0) ||
      (curSlice->activeSPS->chroma_format_idc == 3)) {
    curSlice->linfo_cbp_intra = linfo_cbp_intra_other;
    curSlice->linfo_cbp_inter = linfo_cbp_inter_other;
    }
  else {
    curSlice->linfo_cbp_intra = linfo_cbp_intra_normal;
    curSlice->linfo_cbp_inter = linfo_cbp_inter_normal;
    }
  }
//}}}
//{{{
static void decodeOneSlice (sSlice* curSlice) {

  Boolean end_of_slice = FALSE;
  curSlice->cod_counter=-1;

  sVidParam* vidParam = curSlice->vidParam;
  if ((vidParam->separate_colour_plane_flag != 0))
    change_plane_JV (vidParam, curSlice->colour_plane_id, curSlice );
  else {
    curSlice->mb_data = vidParam->mb_data;
    curSlice->picture = vidParam->picture;
    curSlice->siblock = vidParam->siblock;
    curSlice->ipredmode = vidParam->ipredmode;
    curSlice->intra_block = vidParam->intra_block;
    }

  if (curSlice->slice_type == B_SLICE)
    compute_colocated (curSlice, curSlice->listX);

  if (curSlice->slice_type != I_SLICE && curSlice->slice_type != SI_SLICE)
    init_cur_imgy (curSlice,vidParam);

  // loop over macroblocks
  while (end_of_slice == FALSE) {
    sMacroblock* curMb;
    start_macroblock (curSlice, &curMb);
    curSlice->read_one_macroblock (curMb);
    decode_one_macroblock (curMb, curSlice->picture);

    if (curSlice->mb_aff_frame_flag && curMb->mb_field) {
      curSlice->num_ref_idx_active[LIST_0] >>= 1;
      curSlice->num_ref_idx_active[LIST_1] >>= 1;
      }

    ercWriteMBMODEandMV (curMb);
    end_of_slice = exit_macroblock (curSlice, (!curSlice->mb_aff_frame_flag|| curSlice->current_mb_nr%2));
    }
  }
//}}}
//{{{
static void decodeSlice (sSlice *curSlice, int current_header) {

  if (curSlice->activePPS->entropy_coding_mode_flag) {
    init_contexts (curSlice);
    cabac_new_slice (curSlice);
    }

  if ((curSlice->activePPS->weighted_bipred_idc > 0  && (curSlice->slice_type == B_SLICE)) ||
      (curSlice->activePPS->weighted_pred_flag && curSlice->slice_type !=I_SLICE))
    fill_WPParam (curSlice);

  // decode main slice information
  if ((current_header == SOP || current_header == SOS) && curSlice->ei_flag == 0)
    decodeOneSlice (curSlice);
  }
//}}}
//{{{
static int readNewSlice (sSlice* curSlice) {

  static sNalu* pendingNalu = NULL;

  sInputParam* inputParam = curSlice->inputParam;
  sVidParam* vidParam = curSlice->vidParam;

  int current_header = 0;
  sBitstream* curStream = NULL;
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
              vidParam->nonConformingStream = 1;
              }
            else
              vidParam->nonConformingStream = 0;
            }
          vidParam->recovery_point_found = 1;
          }

        if (vidParam->recovery_point_found == 0)
          break;

        curSlice->idrFlag = (nalu->nal_unit_type == NALU_TYPE_IDR);
        curSlice->nalRefId = nalu->nalRefId;
        curSlice->dp_mode = PAR_DP_1;
        curSlice->max_part_nr = 1;

        curStream = curSlice->partArr[0].bitstream;
        curStream->ei_flag = 0;
        curStream->frame_bitoffset = curStream->read_len = 0;
        memcpy (curStream->streamBuffer, &nalu->buf[1], nalu->len-1);
        curStream->code_len = curStream->bitstream_length = RBSPtoSODB (curStream->streamBuffer, nalu->len-1);

        // Some syntax of the sliceHeader depends on parameter set
        // which depends on the parameter set ID of the slice header.
        // - read the pic_parameter_set_id of the slice header first,
        // - then setup the active parameter sets
        // -  read // the rest of the slice header
        readSliceHeader (curSlice);
        useParameterSet (curSlice);
        curSlice->activeSPS = vidParam->activeSPS;
        curSlice->activePPS = vidParam->activePPS;
        curSlice->transform8x8Mode = vidParam->activePPS->transform_8x8_mode_flag;
        curSlice->chroma444notSeparate = (vidParam->activeSPS->chroma_format_idc == YUV444) &&
                                            (vidParam->separate_colour_plane_flag == 0);
        readRestSliceHeader (curSlice);
        assignQuantParams (curSlice);

        // if primary slice is replaced with redundant slice, set the correct image type
        if (curSlice->redundant_pic_cnt &&
            vidParam->Is_primary_correct == 0 &&
            vidParam->Is_redundant_correct)
          vidParam->picture->slice_type = vidParam->type;

        if (isNewPicture (vidParam->picture, curSlice, vidParam->old_slice)) {
          if (vidParam->iSliceNumOfCurrPic == 0)
            initPicture (vidParam, curSlice, inputParam);
          current_header = SOP;
          // check zero_byte if it is also the first NAL unit in the access unit
          checkZeroByteVCL (vidParam, nalu);
          }
        else
          current_header = SOS;

        setup_slice_methods (curSlice);

        // Vid->activeSPS, vidParam->activePPS, sliceHeader valid
        if (curSlice->mb_aff_frame_flag)
          curSlice->current_mb_nr = curSlice->start_mb_nr << 1;
        else
          curSlice->current_mb_nr = curSlice->start_mb_nr;

        if (vidParam->activePPS->entropy_coding_mode_flag) {
          int ByteStartPosition = curStream->frame_bitoffset / 8;
          if ((curStream->frame_bitoffset % 8) != 0)
            ++ByteStartPosition;
          arideco_start_decoding (&curSlice->partArr[0].de_cabac, curStream->streamBuffer,
                                  ByteStartPosition, &curStream->read_len);
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
        curSlice->dpB_NotPresent = 1;
        curSlice->dpC_NotPresent = 1;
        curSlice->idrFlag = FALSE;
        curSlice->nalRefId = nalu->nalRefId;
        curSlice->dp_mode = PAR_DP_3;
        curSlice->max_part_nr = 3;
        curStream = curSlice->partArr[0].bitstream;
        curStream->ei_flag = 0;
        curStream->frame_bitoffset = curStream->read_len = 0;
        memcpy (curStream->streamBuffer, &nalu->buf[1], nalu->len-1);
        curStream->code_len = curStream->bitstream_length = RBSPtoSODB(curStream->streamBuffer, nalu->len-1);

        readSliceHeader (curSlice);
        useParameterSet (curSlice);
        curSlice->activeSPS = vidParam->activeSPS;
        curSlice->activePPS = vidParam->activePPS;
        curSlice->transform8x8Mode = vidParam->activePPS->transform_8x8_mode_flag;
        curSlice->chroma444notSeparate = (vidParam->activeSPS->chroma_format_idc == YUV444) &&
                                            (vidParam->separate_colour_plane_flag == 0);
        readRestSliceHeader (curSlice);
        assignQuantParams (curSlice);

        if (isNewPicture (vidParam->picture, curSlice, vidParam->old_slice)) {
          if (vidParam->iSliceNumOfCurrPic == 0)
            initPicture (vidParam, curSlice, inputParam);
          current_header = SOP;
          checkZeroByteVCL (vidParam, nalu);
          }
        else
          current_header = SOS;

        setup_slice_methods (curSlice);

        // From here on, vidParam->activeSPS, vidParam->activePPS and the slice header are valid
        if (curSlice->mb_aff_frame_flag)
          curSlice->current_mb_nr = curSlice->start_mb_nr << 1;
        else
          curSlice->current_mb_nr = curSlice->start_mb_nr;

        // need to read the slice ID, which depends on the value of redundant_pic_cnt_present_flag

        int slice_id_a = read_ue_v ("NALU: DP_A slice_id", curStream);

        if (vidParam->activePPS->entropy_coding_mode_flag)
          error ("data partition with CABAC not allowed", 500);

        // reading next DP
        if (!readNextNalu (vidParam, nalu))
          return current_header;
        if (NALU_TYPE_DPB == nalu->nal_unit_type) {
          //{{{  got nalu DPB
          curStream = curSlice->partArr[1].bitstream;
          curStream->ei_flag = 0;
          curStream->frame_bitoffset = curStream->read_len = 0;
          memcpy (curStream->streamBuffer, &nalu->buf[1], nalu->len-1);
          curStream->code_len = curStream->bitstream_length = RBSPtoSODB (curStream->streamBuffer, nalu->len-1);
          int slice_id_b = read_ue_v ("NALU: DP_B slice_id", curStream);
          curSlice->dpB_NotPresent = 0;

          if ((slice_id_b != slice_id_a) || (nalu->lost_packets)) {
            printf ("Waning: got a data partition B which does not match DP_A (DP loss!)\n");
            curSlice->dpB_NotPresent = 1;
            curSlice->dpC_NotPresent = 1;
            }
          else {
            if (vidParam->activePPS->redundant_pic_cnt_present_flag)
              read_ue_v ("NALU: DP_B redundant_pic_cnt", curStream);

            // we're finished with DP_B, so let's continue with next DP
            if (!readNextNalu (vidParam, nalu))
              return current_header;
            }
          }
          //}}}
        else
          curSlice->dpB_NotPresent = 1;

        if (NALU_TYPE_DPC == nalu->nal_unit_type) {
          //{{{  got nalu DPC
          curStream = curSlice->partArr[2].bitstream;
          curStream->ei_flag = 0;
          curStream->frame_bitoffset = curStream->read_len = 0;
          memcpy (curStream->streamBuffer, &nalu->buf[1], nalu->len-1);
          curStream->code_len = curStream->bitstream_length = RBSPtoSODB (curStream->streamBuffer, nalu->len-1);

          curSlice->dpC_NotPresent = 0;
          int slice_id_c = read_ue_v ("NALU: DP_C slice_id", curStream);
          if ((slice_id_c != slice_id_a)|| (nalu->lost_packets)) {
            printf ("Warning: got a data partition C which does not match DP_A(DP loss!)\n");
            curSlice->dpC_NotPresent =1;
            }

          if (vidParam->activePPS->redundant_pic_cnt_present_flag)
            read_ue_v ("NALU:SLICE_C redudand_pic_cnt", curStream);
          }
          //}}}
        else {
          curSlice->dpC_NotPresent = 1;
          pendingNalu = nalu;
          }

        // check if we read anything else than the expected partitions
        if ((nalu->nal_unit_type != NALU_TYPE_DPB) &&
            (nalu->nal_unit_type != NALU_TYPE_DPC) && (!curSlice->dpC_NotPresent))
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
        InterpretSEIMessage (nalu->buf,nalu->len,vidParam, curSlice);
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
void initOldSlice (sOldSliceParam* oldSliceParam) {

  oldSliceParam->field_pic_flag = 0;
  oldSliceParam->pps_id = INT_MAX;
  oldSliceParam->frame_num = INT_MAX;
  oldSliceParam->nal_ref_idc = INT_MAX;
  oldSliceParam->idrFlag = FALSE;

  oldSliceParam->pic_oder_cnt_lsb = UINT_MAX;
  oldSliceParam->delta_pic_oder_cnt_bottom = INT_MAX;

  oldSliceParam->delta_pic_order_cnt[0] = INT_MAX;
  oldSliceParam->delta_pic_order_cnt[1] = INT_MAX;
  }
//}}}
//{{{
void calcFrameNum (sVidParam* vidParam, sPicture *p) {

  sInputParam* inputParam = vidParam->inputParam;
  int psnrPOC = vidParam->activeSPS->mb_adaptive_frame_field_flag ? p->poc / (inputParam->poc_scale) :
                                                                     p->poc / (inputParam->poc_scale);
  if (psnrPOC == 0)
    vidParam->idr_psnr_number = vidParam->gapNumFrame * vidParam->ref_poc_gap / (inputParam->poc_scale);
  vidParam->psnr_number = imax (vidParam->psnr_number, vidParam->idr_psnr_number+psnrPOC);
  vidParam->frameNum = vidParam->idr_psnr_number + psnrPOC;
  }
//}}}
//{{{
void padPicture (sVidParam* vidParam, sPicture* picture) {

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
void exitPicture (sVidParam* vidParam, sPicture** picture) {

  sInputParam* inputParam = vidParam->inputParam;

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
    for (i = 1; i < (int)(*picture)->PicSizeInMbs; ++i) {
      if (vidParam->mb_data[i].ei_flag != vidParam->mb_data[i-1].ei_flag) {
        ercStopSegment (i-1, ercSegment, 0, vidParam->erc_errorVar); //! stop current segment

        // mark current segment as lost or OK
        if(vidParam->mb_data[i-1].ei_flag)
          ercMarkCurrSegmentLost ((*picture)->size_x, vidParam->erc_errorVar);
        else
          ercMarkCurrSegmentOK ((*picture)->size_x, vidParam->erc_errorVar);

        ++ercSegment;  //! next segment
        ercStartSegment (i, ercSegment, 0 , vidParam->erc_errorVar); //! start new segment
        }
      }

    // mark end of the last segment
    ercStopSegment ((*picture)->PicSizeInMbs-1, ercSegment, 0, vidParam->erc_errorVar);
    if (vidParam->mb_data[i-1].ei_flag)
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
  if (!vidParam->deblockMode &&
      (vidParam->bDeblockEnable & (1 << (*picture)->used_for_reference))) {
    //{{{  deblocking for frame or field
    if( (vidParam->separate_colour_plane_flag != 0) ) {
      int colour_plane_id = vidParam->ppSliceList[0]->colour_plane_id;
      for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
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
    padPicture (vidParam,* picture);

  int structure = (*picture)->structure;
  int slice_type = (*picture)->slice_type;
  int frame_poc = (*picture)->frame_poc;
  int refpic = (*picture)->used_for_reference;
  int qp = (*picture)->qp;
  int pic_num = (*picture)->pic_num;
  int is_idr = (*picture)->idrFlag;
  int chroma_format_idc = (*picture)->chroma_format_idc;

  store_picture_in_dpb (vidParam->p_Dpb_layer[0],* picture);

  *picture = NULL;
  if (vidParam->last_has_mmco_5)
    vidParam->preFrameNum = 0;

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
    gettime (&(vidParam->endTime));
    int64 tmpTime = timediff(&(vidParam->startTime), &(vidParam->endTime));
    vidParam->totTime += tmpTime;
    printf ("%5d %s poc:%4d pic:%3d qp:%2d %dms\n",
            vidParam->frameNum, vidParam->cslice_type, frame_poc, pic_num, qp, (int)timenorm (tmpTime));

    if (slice_type == I_SLICE || slice_type == SI_SLICE ||
        slice_type == P_SLICE || refpic) // I or P pictures
      ++(vidParam->number);

    ++(vidParam->gapNumFrame);
    }
  }
//}}}

//{{{
int decode_one_frame (sDecoderParam* pDecoder) {

  int ret = 0;

  sVidParam* vidParam = pDecoder->vidParam;
  sInputParam* inputParam = vidParam->inputParam;

  // read one picture first
  vidParam->iSliceNumOfCurrPic = 0;
  vidParam->iNumOfSlicesDecoded = 0;
  vidParam->num_dec_mb = 0;

  int iSliceNo = 0;
  sSlice* curSlice = NULL;
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
    curSlice = ppSliceList[vidParam->iSliceNumOfCurrPic];
    ppSliceList[vidParam->iSliceNumOfCurrPic] = vidParam->pNextSlice;
    vidParam->pNextSlice = curSlice;
    curSlice = ppSliceList[vidParam->iSliceNumOfCurrPic];
    useParameterSet (curSlice);
    initPicture (vidParam, curSlice, inputParam);
    vidParam->iSliceNumOfCurrPic++;
    current_header = SOS;
    }

  while (current_header != SOP && current_header != EOS) {
    //{{{  no pending slices
    assert (vidParam->iSliceNumOfCurrPic < vidParam->iNumOfSlicesAllocated);

    if (!ppSliceList[vidParam->iSliceNumOfCurrPic])
      ppSliceList[vidParam->iSliceNumOfCurrPic] = allocSlice (inputParam, vidParam);

    curSlice = ppSliceList[vidParam->iSliceNumOfCurrPic];
    curSlice->vidParam = vidParam;
    curSlice->inputParam = inputParam;
    curSlice->dpb = vidParam->p_Dpb_layer[0]; //set default value;
    curSlice->next_header = -8888;
    curSlice->num_dec_mb = 0;
    curSlice->coeff_ctr = -1;
    curSlice->pos = 0;
    curSlice->is_reset_coeff = FALSE;
    curSlice->is_reset_coeff_cr = FALSE;
    current_header = readNewSlice (curSlice);
    curSlice->current_header = current_header;

    // error tracking of primary and redundant slices.
    errorTracking (vidParam, curSlice);
    //{{{  manage primary and redundant slices
    // If primary and redundant are received and primary is correct
    //   discard the redundant
    // else,
    //   primary slice will be replaced with redundant slice.
    if (curSlice->frame_num == vidParam->previous_frame_num &&
        curSlice->redundant_pic_cnt != 0 &&
        vidParam->Is_primary_correct != 0 &&
        current_header != EOS)
      continue;

    if ((current_header != SOP && current_header != EOS) ||
        (vidParam->iSliceNumOfCurrPic == 0 && current_header == SOP)) {
       curSlice->current_slice_nr = (short)vidParam->iSliceNumOfCurrPic;
       vidParam->picture->max_slice_id =
         (short)imax (curSlice->current_slice_nr, vidParam->picture->max_slice_id);
       if (vidParam->iSliceNumOfCurrPic > 0) {
         copyPOC (*ppSliceList, curSlice);
         ppSliceList[vidParam->iSliceNumOfCurrPic-1]->end_mb_nr_plus1 = curSlice->start_mb_nr;
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
      curSlice->current_slice_nr = 0;

      // keep it in currentslice;
      ppSliceList[vidParam->iSliceNumOfCurrPic] = vidParam->pNextSlice;
      vidParam->pNextSlice = curSlice;
      }
    //}}}

    copySliceInfo (curSlice, vidParam->old_slice);
    }
    //}}}

  // decode slices
  ret = current_header;
  initPictureDecoding (vidParam);
  for (iSliceNo = 0; iSliceNo < vidParam->iSliceNumOfCurrPic; iSliceNo++) {
    curSlice = ppSliceList[iSliceNo];
    current_header = curSlice->current_header;
    assert (current_header != EOS);
    assert (curSlice->current_slice_nr == iSliceNo);

    initSlice (vidParam, curSlice);
    decodeSlice (curSlice, current_header);
    vidParam->iNumOfSlicesDecoded++;
    vidParam->num_dec_mb += curSlice->num_dec_mb;
    vidParam->erc_mvperMB += curSlice->erc_mvperMB;
    }

  if (vidParam->picture->structure == FRAME)
    vidParam->last_dec_poc = vidParam->picture->frame_poc;
  else if (vidParam->picture->structure == TOP_FIELD)
    vidParam->last_dec_poc = vidParam->picture->top_poc;
  else if (vidParam->picture->structure == BOTTOM_FIELD)
    vidParam->last_dec_poc = vidParam->picture->bottom_poc;

  exitPicture (vidParam, &vidParam->picture);
  vidParam->previous_frame_num = ppSliceList[0]->frame_num;

  return ret;
  }
//}}}
