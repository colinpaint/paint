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
#include "buffer.h"
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
    sCodingParam* codingParam = vidParam->codingParam[layerId];
    if (codingParam->separate_colour_plane_flag) {
      for (int i = 0; i < MAX_PLANE; i++ ) {
        vidParam->mbDataJV[i] = codingParam->mbDataJV[i];
        vidParam->intraBlockJV[i] = codingParam->intraBlockJV[i];
        vidParam->predModeJV[i] = codingParam->predModeJV[i];
        vidParam->siBlockJV[i] = codingParam->siBlockJV[i];
        }
      vidParam->mbData = NULL;
      vidParam->intraBlock = NULL;
      vidParam->predMode = NULL;
      vidParam->siBlock = NULL;
      }
    else {
      vidParam->mbData = codingParam->mbData;
      vidParam->intraBlock = codingParam->intraBlock;
      vidParam->predMode = codingParam->predMode;
      vidParam->siBlock = codingParam->siBlock;
      }

    vidParam->picPos = codingParam->picPos;
    vidParam->nzCoeff = codingParam->nzCoeff;
    vidParam->qpPerMatrix = codingParam->qpPerMatrix;
    vidParam->qpRemMatrix = codingParam->qpRemMatrix;
    vidParam->oldFrameSizeInMbs = codingParam->oldFrameSizeInMbs;
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
  for (short i = 0; i < (int)picture->picSizeInMbs; i += 2) {
    if (picture->motion.mb_field[i]) {
      get_mb_pos (vidParam, i, vidParam->mb_size[IS_LUMA], &x0, &y0);
      update_mbaff_macroblock_data (imgY + y0, temp_buffer, x0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

      if (picture->chromaFormatIdc != YUV400) {
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
              tb = iClip3(-128,127,curSlice->thisPoc - curSlice->listX[LIST_0][i]->poc);
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
    vidParam->isPrimaryOk = vidParam->isReduncantOk = 1;

  if (curSlice->redundant_pic_cnt == 0 && vidParam->type != I_SLICE) {
    for (int i = 0; i < curSlice->num_ref_idx_active[LIST_0];++i)
      if (curSlice->ref_flag[i] == 0)  // any reference of primary slice is incorrect
        vidParam->isPrimaryOk = 0; // primary slice is incorrect
    }
  else if (curSlice->redundant_pic_cnt != 0 && vidParam->type != I_SLICE)
    // reference of redundant slice is incorrect
    if (curSlice->ref_flag[curSlice->redundant_slice_ref_idx] == 0)
      // redundant slice is incorrect
      vidParam->isReduncantOk = 0;
  }
//}}}
//{{{
static void copyPOC (sSlice *pSlice0, sSlice *curSlice) {

  curSlice->framePoc  = pSlice0->framePoc;
  curSlice->topPoc    = pSlice0->topPoc;
  curSlice->botPoc = pSlice0->botPoc;
  curSlice->thisPoc   = pSlice0->thisPoc;
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

  sObjectBuffer* currRegion = vidParam->ercObjectList + (currMBNum<<2);

  if (vidParam->type != B_SLICE) {
    //{{{  non-B frame
    for (int i = 0; i < 4; ++i) {
      sObjectBuffer* pRegion = currRegion + i;
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

        curMb->slice->ercMvPerMb += iabs(pRegion->mv[0]) + iabs(pRegion->mv[1]);
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

      sObjectBuffer* pRegion = currRegion + i;
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
        curMb->slice->ercMvPerMb += iabs(pRegion->mv[0]) + iabs(pRegion->mv[1]);

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

  dst->chromaFormatIdc    = src->chromaFormatIdc;

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

  vidParam->picHeightInMbs = vidParam->FrameHeightInMbs / ( 1 + curSlice->field_pic_flag );
  vidParam->picSizeInMbs = vidParam->PicWidthInMbs * vidParam->picHeightInMbs;
  vidParam->FrameSizeInMbs = vidParam->PicWidthInMbs * vidParam->FrameHeightInMbs;
  vidParam->bFrameInit = 1;
  if (vidParam->picture)
    // this may only happen on slice loss
    exitPicture (vidParam, &vidParam->picture);

  vidParam->dpbLayerId = curSlice->layerId;
  setupBuffers (vidParam, curSlice->layerId);

  if (vidParam->recovery_point)
    vidParam->recovery_frame_num = (curSlice->frame_num + vidParam->recovery_frame_cnt) % vidParam->max_frame_num;
  if (curSlice->idrFlag)
    vidParam->recovery_frame_num = curSlice->frame_num;
  if (vidParam->recovery_point == 0 &&
    curSlice->frame_num != vidParam->preFrameNum &&
    curSlice->frame_num != (vidParam->preFrameNum + 1) % vidParam->max_frame_num) {
    if (activeSPS->gaps_in_frame_num_value_allowed_flag == 0) {
      // picture error conceal
      if (inputParam->concealMode != 0) {
        if ((curSlice->frame_num) < ((vidParam->preFrameNum + 1) % vidParam->max_frame_num)) {
          /* Conceal lost IDR frames and any frames immediately following the IDR.
          // Use frame copy for these since lists cannot be formed correctly for motion copy*/
          vidParam->concealMode = 1;
          vidParam->IDR_concealment_flag = 1;
          conceal_lost_frames(dpb, curSlice);
          // reset to original conceal mode for future drops
          vidParam->concealMode = inputParam->concealMode;
          }
        else {
          // reset to original conceal mode for future drops
          vidParam->concealMode = inputParam->concealMode;
          vidParam->IDR_concealment_flag = 0;
          conceal_lost_frames(dpb, curSlice);
          }
        }
      else
        // Advanced Error Concealment would be called here to combat unintentional loss of pictures
        error ("An unintentional loss of pictures occurs! Exit\n", 100);
      }
    if (vidParam->concealMode == 0)
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

  picture = vidParam->picture = allocPicture (vidParam, curSlice->structure, vidParam->width, vidParam->height, vidParam->widthCr, vidParam->heightCr, 1);
  picture->top_poc = curSlice->topPoc;
  picture->bottom_poc = curSlice->botPoc;
  picture->frame_poc = curSlice->framePoc;
  picture->qp = curSlice->qp;
  picture->slice_qp_delta = curSlice->slice_qp_delta;
  picture->chroma_qp_offset[0] = vidParam->activePPS->chroma_qp_index_offset;
  picture->chroma_qp_offset[1] = vidParam->activePPS->second_chroma_qp_index_offset;
  picture->iCodingType = curSlice->structure==FRAME? (curSlice->mb_aff_frame_flag? FRAME_MB_PAIR_CODING:FRAME_CODING): FIELD_CODING; //curSlice->slice_type;
  picture->layerId = curSlice->layerId;

  // reset all variables of the error conceal instance before decoding of every frame.
  // here the third parameter should, if perfectly, be equal to the number of slices per frame.
  // using little value is ok, the code will allocate more memory if the slice number is larger
  ercReset (vidParam->ercErrorVar, vidParam->picSizeInMbs, vidParam->picSizeInMbs, picture->size_x);

  vidParam->ercMvPerMb = 0;
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
    memset (vidParam->nzCoeff[0][0][0], -1, vidParam->picSizeInMbs * 48 *sizeof(byte)); // 3 * 4 * 4

  // Set the slice_nr member of each MB to -1, to ensure correct when packet loss occurs
  // TO set sMacroblock Map (mark all MBs as 'have to be concealed')
  if (vidParam->separate_colour_plane_flag != 0) {
    for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
      sMacroblock* curMb = vidParam->mbDataJV[nplane];
      char* intraBlock = vidParam->intraBlockJV[nplane];
      for (int i = 0; i < (int)vidParam->picSizeInMbs; ++i)
        resetMbs (curMb++);
      memset (vidParam->predModeJV[nplane][0], DC_PRED, 16 * vidParam->FrameHeightInMbs * vidParam->PicWidthInMbs * sizeof(char));
      if (vidParam->activePPS->constrained_intra_pred_flag)
        for (int i = 0; i < (int)vidParam->picSizeInMbs; ++i)
          intraBlock[i] = 1;
      }
    }
  else {
    sMacroblock* curMb = vidParam->mbData;
    for (int i = 0; i < (int)vidParam->picSizeInMbs; ++i)
      resetMbs (curMb++);
    if (vidParam->activePPS->constrained_intra_pred_flag)
      for (int i = 0; i < (int)vidParam->picSizeInMbs; ++i)
        vidParam->intraBlock[i] = 1;
    memset (vidParam->predMode[0], DC_PRED, 16 * vidParam->FrameHeightInMbs * vidParam->PicWidthInMbs * sizeof(char));
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
  picture->chromaFormatIdc = activeSPS->chromaFormatIdc;
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
    vidParam->dec_picture_JV[1] = allocPicture (vidParam, (ePicStructure) curSlice->structure, vidParam->width, vidParam->height, vidParam->widthCr, vidParam->heightCr, 1);
    copyDecPicture_JV (vidParam, vidParam->dec_picture_JV[1], vidParam->dec_picture_JV[0] );
    vidParam->dec_picture_JV[2] = allocPicture (vidParam, (ePicStructure) curSlice->structure, vidParam->width, vidParam->height, vidParam->widthCr, vidParam->heightCr, 1);
    copyDecPicture_JV (vidParam, vidParam->dec_picture_JV[2], vidParam->dec_picture_JV[0] );
    }
  }
//}}}
//{{{
static void initPictureDecoding (sVidParam* vidParam) {

  int deblockMode = 1;

  if (vidParam->iSliceNumOfCurrPic >= MAX_NUM_SLICES)
    error ("Maximum number of supported slices exceeded, increase MAX_NUM_SLICES", 200);

  sSlice* slice = vidParam->sliceList[0];
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

  vidParam->picHeightInMbs = vidParam->FrameHeightInMbs / ( 1 + slice->field_pic_flag );
  vidParam->picSizeInMbs = vidParam->PicWidthInMbs * vidParam->picHeightInMbs;
  vidParam->FrameSizeInMbs = vidParam->PicWidthInMbs * vidParam->FrameHeightInMbs;
  vidParam->structure = slice->structure;
  fmo_init (vidParam, slice);

  update_pic_num (slice);

  initDeblock (vidParam, slice->mb_aff_frame_flag);
  for (int j = 0; j < vidParam->iSliceNumOfCurrPic; j++) {
    if (vidParam->sliceList[j]->DFDisableIdc != 1)
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
  if (!(curSlice->redundant_pic_cnt != 0 && vidParam->prevFrameNum == curSlice->frame_num))
    for (int i = 16; i > 0;i--)
      curSlice->ref_flag[i] = curSlice->ref_flag[i-1];
  curSlice->ref_flag[0] = curSlice->redundant_pic_cnt == 0 ? vidParam->isPrimaryOk :
                                                               vidParam->isReduncantOk;

  if ((curSlice->activeSPS->chromaFormatIdc == 0) ||
      (curSlice->activeSPS->chromaFormatIdc == 3)) {
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
    curSlice->mbData = vidParam->mbData;
    curSlice->picture = vidParam->picture;
    curSlice->siBlock = vidParam->siBlock;
    curSlice->predMode = vidParam->predMode;
    curSlice->intraBlock = vidParam->intraBlock;
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
        curSlice->chroma444notSeparate = (vidParam->activeSPS->chromaFormatIdc == YUV444) &&
                                            (vidParam->separate_colour_plane_flag == 0);
        readRestSliceHeader (curSlice);
        assignQuantParams (curSlice);

        // if primary slice is replaced with redundant slice, set the correct image type
        if (curSlice->redundant_pic_cnt &&
            vidParam->isPrimaryOk == 0 &&
            vidParam->isReduncantOk)
          vidParam->picture->slice_type = vidParam->type;

        if (isNewPicture (vidParam->picture, curSlice, vidParam->oldSlice)) {
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
        curSlice->chroma444notSeparate = (vidParam->activeSPS->chromaFormatIdc == YUV444) &&
                                            (vidParam->separate_colour_plane_flag == 0);
        readRestSliceHeader (curSlice);
        assignQuantParams (curSlice);

        if (isNewPicture (vidParam->picture, curSlice, vidParam->oldSlice)) {
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
    vidParam->idrPsnrNum = vidParam->gapNumFrame * vidParam->ref_poc_gap / (inputParam->poc_scale);
  vidParam->psnrNum = imax (vidParam->psnrNum, vidParam->idrPsnrNum+psnrPOC);
  vidParam->frameNum = vidParam->idrPsnrNum + psnrPOC;
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

  if (picture->chromaFormatIdc != YUV400) {
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
      (vidParam->num_dec_mb != vidParam->picSizeInMbs &&
       (vidParam->yuvFormat != YUV444 || !vidParam->separate_colour_plane_flag)))
    return;

  //{{{  error conceal
  frame recfr;
  recfr.vidParam = vidParam;
  recfr.yptr = &(*picture)->imgY[0][0];
  if ((*picture)->chromaFormatIdc != YUV400) {
    recfr.uptr = &(*picture)->imgUV[0][0][0];
    recfr.vptr = &(*picture)->imgUV[1][0][0];
    }

  // this is always true at the beginning of a picture
  int ercSegment = 0;

  // mark the start of the first segment
  if (!(*picture)->mb_aff_frame_flag) {
    int i;
    ercStartSegment (0, ercSegment, 0 , vidParam->ercErrorVar);
    // generate the segments according to the macroblock map
    for (i = 1; i < (int)(*picture)->picSizeInMbs; ++i) {
      if (vidParam->mbData[i].ei_flag != vidParam->mbData[i-1].ei_flag) {
        ercStopSegment (i-1, ercSegment, 0, vidParam->ercErrorVar); //! stop current segment

        // mark current segment as lost or OK
        if(vidParam->mbData[i-1].ei_flag)
          ercMarkCurrSegmentLost ((*picture)->size_x, vidParam->ercErrorVar);
        else
          ercMarkCurrSegmentOK ((*picture)->size_x, vidParam->ercErrorVar);

        ++ercSegment;  //! next segment
        ercStartSegment (i, ercSegment, 0 , vidParam->ercErrorVar); //! start new segment
        }
      }

    // mark end of the last segment
    ercStopSegment ((*picture)->picSizeInMbs-1, ercSegment, 0, vidParam->ercErrorVar);
    if (vidParam->mbData[i-1].ei_flag)
      ercMarkCurrSegmentLost ((*picture)->size_x, vidParam->ercErrorVar);
    else
      ercMarkCurrSegmentOK ((*picture)->size_x, vidParam->ercErrorVar);

    // call the right error conceal function depending on the frame type.
    vidParam->ercMvPerMb /= (*picture)->picSizeInMbs;
    vidParam->ercImg = vidParam;
    if ((*picture)->slice_type == I_SLICE || (*picture)->slice_type == SI_SLICE) // I-frame
      ercConcealIntraFrame (vidParam, &recfr, (*picture)->size_x, (*picture)->size_y, vidParam->ercErrorVar);
    else
      ercConcealInterFrame (&recfr, vidParam->ercObjectList, (*picture)->size_x, (*picture)->size_y, vidParam->ercErrorVar, (*picture)->chromaFormatIdc);
    }
  //}}}
  if (!vidParam->deblockMode &&
      (vidParam->bDeblockEnable & (1 << (*picture)->used_for_reference))) {
    //{{{  deblocking for frame or field
    if( (vidParam->separate_colour_plane_flag != 0) ) {
      int colour_plane_id = vidParam->sliceList[0]->colour_plane_id;
      for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
        vidParam->sliceList[0]->colour_plane_id = nplane;
        change_plane_JV (vidParam, nplane, NULL );
        deblockPicture (vidParam,* picture );
        }
      vidParam->sliceList[0]->colour_plane_id = colour_plane_id;
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
  int chromaFormatIdc = (*picture)->chromaFormatIdc;

  store_picture_in_dpb (vidParam->dpbLayer[0],* picture);

  *picture = NULL;
  if (vidParam->last_has_mmco_5)
    vidParam->preFrameNum = 0;

  if (structure == TOP_FIELD || structure == FRAME) {
    //{{{  frame type string
    if (slice_type == I_SLICE && is_idr) // IDR picture
      strcpy (vidParam->sliceTypeText,"IDR");
    else if (slice_type == I_SLICE) // I picture
      strcpy (vidParam->sliceTypeText," I ");
    else if (slice_type == P_SLICE) // P pictures
      strcpy (vidParam->sliceTypeText," P ");
    else if (slice_type == SP_SLICE) // SP pictures
      strcpy (vidParam->sliceTypeText,"SP ");
    else if  (slice_type == SI_SLICE)
      strcpy (vidParam->sliceTypeText,"SI ");
    else if (refpic) // stored B pictures
      strcpy (vidParam->sliceTypeText," B ");
    else // B pictures
      strcpy (vidParam->sliceTypeText," b ");

    if (structure == FRAME)
      strncat (vidParam->sliceTypeText, "       ",8-strlen(vidParam->sliceTypeText));
    }
    //}}}
  else if (structure == BOTTOM_FIELD) {
    //{{{  frame type string
    if (slice_type == I_SLICE && is_idr) // IDR picture
      strncat (vidParam->sliceTypeText,"|IDR",8-strlen(vidParam->sliceTypeText));
    else if (slice_type == I_SLICE) // I picture
      strncat (vidParam->sliceTypeText,"| I ",8-strlen(vidParam->sliceTypeText));
    else if (slice_type == P_SLICE) // P pictures
      strncat (vidParam->sliceTypeText,"| P ",8-strlen(vidParam->sliceTypeText));
    else if (slice_type == SP_SLICE) // SP pictures
      strncat (vidParam->sliceTypeText,"|SP ",8-strlen(vidParam->sliceTypeText));
    else if  (slice_type == SI_SLICE)
      strncat (vidParam->sliceTypeText,"|SI ",8-strlen(vidParam->sliceTypeText));
    else if (refpic) // stored B pictures
      strncat (vidParam->sliceTypeText,"| B ",8-strlen(vidParam->sliceTypeText));
    else // B pictures
      strncat (vidParam->sliceTypeText,"| b ",8-strlen(vidParam->sliceTypeText));
    }
    //}}}
  vidParam->sliceTypeText[8] = 0;

  if ((structure == FRAME) || structure == BOTTOM_FIELD) {
    gettime (&(vidParam->endTime));
    int64 tmpTime = timediff(&(vidParam->startTime), &(vidParam->endTime));
    vidParam->totTime += tmpTime;
    printf ("%5d %s poc:%4d pic:%3d qp:%2d %dms\n",
            vidParam->frameNum, vidParam->sliceTypeText, frame_poc, pic_num, qp, (int)timenorm (tmpTime));

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
  vidParam->numSlicesDecoded = 0;
  vidParam->num_dec_mb = 0;

  int iSliceNo = 0;
  sSlice* curSlice = NULL;
  sSlice** sliceList = vidParam->sliceList;
  int current_header = 0;
  if (vidParam->newFrame) {
    if (vidParam->nextPPS->Valid) {
      //{{{  use next PPS
      makePPSavailable (vidParam, vidParam->nextPPS->pic_parameter_set_id, vidParam->nextPPS);
      vidParam->nextPPS->Valid = 0;
      }
      //}}}
    // get firstSlice from currentSlice;
    curSlice = sliceList[vidParam->iSliceNumOfCurrPic];
    sliceList[vidParam->iSliceNumOfCurrPic] = vidParam->nextSlice;
    vidParam->nextSlice = curSlice;
    curSlice = sliceList[vidParam->iSliceNumOfCurrPic];
    useParameterSet (curSlice);
    initPicture (vidParam, curSlice, inputParam);
    vidParam->iSliceNumOfCurrPic++;
    current_header = SOS;
    }

  while (current_header != SOP && current_header != EOS) {
    //{{{  no pending slices
    assert (vidParam->iSliceNumOfCurrPic < vidParam->numSlicesAllocated);

    if (!sliceList[vidParam->iSliceNumOfCurrPic])
      sliceList[vidParam->iSliceNumOfCurrPic] = allocSlice (inputParam, vidParam);

    curSlice = sliceList[vidParam->iSliceNumOfCurrPic];
    curSlice->vidParam = vidParam;
    curSlice->inputParam = inputParam;
    curSlice->dpb = vidParam->dpbLayer[0]; //set default value;
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
    if (curSlice->frame_num == vidParam->prevFrameNum &&
        curSlice->redundant_pic_cnt != 0 &&
        vidParam->isPrimaryOk != 0 &&
        current_header != EOS)
      continue;

    if ((current_header != SOP && current_header != EOS) ||
        (vidParam->iSliceNumOfCurrPic == 0 && current_header == SOP)) {
       curSlice->current_slice_nr = (short)vidParam->iSliceNumOfCurrPic;
       vidParam->picture->max_slice_id =
         (short)imax (curSlice->current_slice_nr, vidParam->picture->max_slice_id);
       if (vidParam->iSliceNumOfCurrPic > 0) {
         copyPOC (*sliceList, curSlice);
         sliceList[vidParam->iSliceNumOfCurrPic-1]->end_mb_nr_plus1 = curSlice->start_mb_nr;
         }

       vidParam->iSliceNumOfCurrPic++;
       if (vidParam->iSliceNumOfCurrPic >= vidParam->numSlicesAllocated) {
         sSlice** tmpSliceList = (sSlice**)realloc (
           vidParam->sliceList, (vidParam->numSlicesAllocated+MAX_NUM_DECSLICES)*sizeof(sSlice*));
         if (!tmpSliceList) {
           tmpSliceList = calloc ((vidParam->numSlicesAllocated + MAX_NUM_DECSLICES), sizeof(sSlice*));
           memcpy (tmpSliceList, vidParam->sliceList, vidParam->iSliceNumOfCurrPic*sizeof(sSlice*));
           free (vidParam->sliceList);
           sliceList = vidParam->sliceList = tmpSliceList;
           }
         else {
           sliceList = vidParam->sliceList = tmpSliceList;
           memset (vidParam->sliceList + vidParam->iSliceNumOfCurrPic, 0, sizeof(sSlice*)*MAX_NUM_DECSLICES);
           }
         vidParam->numSlicesAllocated += MAX_NUM_DECSLICES;
        }
      current_header = SOS;
      }
    else {
      if (sliceList[vidParam->iSliceNumOfCurrPic-1]->mb_aff_frame_flag)
        sliceList[vidParam->iSliceNumOfCurrPic-1]->end_mb_nr_plus1 = vidParam->FrameSizeInMbs/2;
      else
        sliceList[vidParam->iSliceNumOfCurrPic-1]->end_mb_nr_plus1 = vidParam->FrameSizeInMbs/(1+sliceList[vidParam->iSliceNumOfCurrPic-1]->field_pic_flag);
      vidParam->newFrame = 1;
      curSlice->current_slice_nr = 0;

      // keep it in currentslice;
      sliceList[vidParam->iSliceNumOfCurrPic] = vidParam->nextSlice;
      vidParam->nextSlice = curSlice;
      }
    //}}}

    copySliceInfo (curSlice, vidParam->oldSlice);
    }
    //}}}

  // decode slices
  ret = current_header;
  initPictureDecoding (vidParam);
  for (iSliceNo = 0; iSliceNo < vidParam->iSliceNumOfCurrPic; iSliceNo++) {
    curSlice = sliceList[iSliceNo];
    current_header = curSlice->current_header;
    assert (current_header != EOS);
    assert (curSlice->current_slice_nr == iSliceNo);

    initSlice (vidParam, curSlice);
    decodeSlice (curSlice, current_header);
    vidParam->numSlicesDecoded++;
    vidParam->num_dec_mb += curSlice->num_dec_mb;
    vidParam->ercMvPerMb += curSlice->ercMvPerMb;
    }

  if (vidParam->picture->structure == FRAME)
    vidParam->last_dec_poc = vidParam->picture->frame_poc;
  else if (vidParam->picture->structure == TOP_FIELD)
    vidParam->last_dec_poc = vidParam->picture->top_poc;
  else if (vidParam->picture->structure == BOTTOM_FIELD)
    vidParam->last_dec_poc = vidParam->picture->bottom_poc;

  exitPicture (vidParam, &vidParam->picture);
  vidParam->prevFrameNum = sliceList[0]->frame_num;

  return ret;
  }
//}}}
