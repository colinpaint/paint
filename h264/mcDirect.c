//{{{  includes
#include "global.h"
#include "memalloc.h"

#include "block.h"
#include "mcPrediction.h"
#include "mbuffer.h"
#include "mbAccess.h"
#include "macroblock.h"
//}}}

//{{{
static void update_direct_mv_info_temporal (sMacroblock* currMB) {

  sVidParam* vidParam = currMB->vidParam;
  sSlice* currSlice = currMB->p_Slice;
  int j,k;
  int partmode        = ((currMB->mb_type == P8x8) ? 4 : currMB->mb_type);
  int step_h0         = BLOCK_STEP [partmode][0];
  int step_v0         = BLOCK_STEP [partmode][1];
  int i0, j0, j6;
  int j4, i4;
  sPicture* picture = currSlice->picture;
  int list_offset = currMB->list_offset; // ((currSlice->mb_aff_frame_flag)&&(currMB->mb_field))? (mb_nr&0x01) ? 4 : 2 : 0;
  sPicture** list0 = currSlice->listX[LIST_0 + list_offset];
  sPicture** list1 = currSlice->listX[LIST_1 + list_offset];

  Boolean has_direct = (currMB->b8mode[0] == 0) | (currMB->b8mode[1] == 0) |
                       (currMB->b8mode[2] == 0) | (currMB->b8mode[3] == 0);
  if (has_direct) {
    int mv_scale = 0;
    for (k = 0; k < 4; ++k) {
      // Scan all blocks
      if (currMB->b8mode[k] == 0) {
        currMB->b8pdir[k] = 2;
        for (j0 = 2 * (k >> 1); j0 < 2 * (k >> 1) + 2; j0 += step_v0) {
          for (i0 = currMB->block_x + 2*(k & 0x01); i0 < currMB->block_x + 2 * (k & 0x01)+2; i0 += step_h0) {
            //{{{  block
            int refList;
            int ref_idx;
            int mapped_idx = -1, iref;

            sPicMotionParams* colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                                           &list1[0]->mv_info[RSD(currMB->block_y_aff + j0)][RSD(i0)] :
                                           &list1[0]->mv_info[currMB->block_y_aff + j0][i0];

            if (currSlice->mb_aff_frame_flag) {
              //{{{
              assert (vidParam->active_sps->direct_8x8_inference_flag);
              if (!currMB->mb_field && ((currSlice->listX[LIST_1][0]->iCodingType==FRAME_MB_PAIR_CODING && currSlice->listX[LIST_1][0]->motion.mb_field[currMB->mbAddrX]) ||
                (currSlice->listX[LIST_1][0]->iCodingType==FIELD_CODING))) {
                if (iabs(picture->poc - currSlice->listX[LIST_1+4][0]->poc)> iabs(picture->poc -currSlice->listX[LIST_1+2][0]->poc) )
                  colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                    &currSlice->listX[LIST_1+2][0]->mv_info[RSD(currMB->block_y_aff + j0)>>1][RSD(i0)] :
                    &currSlice->listX[LIST_1+2][0]->mv_info[(currMB->block_y_aff + j0)>>1][i0];
                else
                  colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                    &currSlice->listX[LIST_1+4][0]->mv_info[RSD(currMB->block_y_aff + j0)>>1][RSD(i0)] :
                    &currSlice->listX[LIST_1+4][0]->mv_info[(currMB->block_y_aff + j0)>>1][i0];
                }
              }
              //}}}
            else if (!vidParam->active_sps->frame_mbs_only_flag &&
                     !currSlice->field_pic_flag &&
                     currSlice->listX[LIST_1][0]->iCodingType != FRAME_CODING) {
              //{{{
              if (iabs(picture->poc - list1[0]->bottom_field->poc) > iabs(picture->poc -list1[0]->top_field->poc) )
                colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                  &list1[0]->top_field->mv_info[RSD(currMB->block_y_aff + j0)>>1][RSD(i0)] :
                  &list1[0]->top_field->mv_info[(currMB->block_y_aff + j0)>>1][i0];
              else
                colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                  &list1[0]->bottom_field->mv_info[RSD(currMB->block_y_aff + j0)>>1][RSD(i0)] :
                  &list1[0]->bottom_field->mv_info[(currMB->block_y_aff + j0)>>1][i0];
              }
              //}}}
            else if (!vidParam->active_sps->frame_mbs_only_flag &&
                     currSlice->field_pic_flag &&
                     currSlice->structure != list1[0]->structure &&
                     list1[0]->coded_frame) {
              //{{{
              if (currSlice->structure == TOP_FIELD)
                colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                  &list1[0]->frame->top_field->mv_info[RSD(currMB->block_y_aff + j0)][RSD(i0)] :
                  &list1[0]->frame->top_field->mv_info[currMB->block_y_aff + j0][i0];
              else
                colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                  &list1[0]->frame->bottom_field->mv_info[RSD(currMB->block_y_aff + j0)][RSD(i0)] :
                  &list1[0]->frame->bottom_field->mv_info[currMB->block_y_aff + j0][i0];
              }
              //}}}

            refList = colocated->ref_idx[LIST_0 ]== -1 ? LIST_1 : LIST_0;
            ref_idx = colocated->ref_idx[refList];
            if (ref_idx == -1) {
              //{{{
              for (j4 = currMB->block_y + j0; j4 < currMB->block_y + j0 + step_v0; ++j4) {
                for (i4 = i0; i4 < i0 + step_h0; ++i4) {
                  sPicMotionParams *mv_info = &picture->mv_info[j4][i4];
                  mv_info->ref_pic[LIST_0] = list0[0];
                  mv_info->ref_pic[LIST_1] = list1[0];
                  mv_info->mv [LIST_0] = zero_mv;
                  mv_info->mv [LIST_1] = zero_mv;
                  mv_info->ref_idx [LIST_0] = 0;
                  mv_info->ref_idx [LIST_1] = 0;
                  }
                }
              }
              //}}}
            else {
              if ((currSlice->mb_aff_frame_flag && ( (currMB->mb_field && colocated->ref_pic[refList]->structure==FRAME) ||
                  (!currMB->mb_field && colocated->ref_pic[refList]->structure!=FRAME))) ||
                  (!currSlice->mb_aff_frame_flag && ((currSlice->field_pic_flag==0 && colocated->ref_pic[refList]->structure!=FRAME)||
                  (currSlice->field_pic_flag==1 && colocated->ref_pic[refList]->structure==FRAME))) ) {
                //{{{  Frame with field co-located
                for (iref = 0;
                     iref < imin(currSlice->num_ref_idx_active[LIST_0], currSlice->listXsize[LIST_0 + list_offset]);
                     ++iref) {
                  if (currSlice->listX[LIST_0 + list_offset][iref]->top_field == colocated->ref_pic[refList] ||
                    currSlice->listX[LIST_0 + list_offset][iref]->bottom_field == colocated->ref_pic[refList] ||
                    currSlice->listX[LIST_0 + list_offset][iref]->frame == colocated->ref_pic[refList] )
                  {
                    if ((currSlice->field_pic_flag==1) && (currSlice->listX[LIST_0 + list_offset][iref]->structure != currSlice->structure))
                      mapped_idx=INVALIDINDEX;
                    else {
                      mapped_idx = iref;
                      break;
                      }
                    }
                  else //! invalid index. Default to zero even though this case should not happen
                    mapped_idx=INVALIDINDEX;
                  }
                }
                //}}}
              else {
                //{{{
                for (iref = 0;
                     iref < imin(currSlice->num_ref_idx_active[LIST_0], currSlice->listXsize[LIST_0 + list_offset]);
                     ++iref) {
                  if (currSlice->listX[LIST_0 + list_offset][iref] == colocated->ref_pic[refList]) {
                    mapped_idx = iref;
                    break;
                    }
                  else //! invalid index. Default to zero even though this case should not happen
                    mapped_idx=INVALIDINDEX;
                  }
                }
                //}}}

              if (mapped_idx != INVALIDINDEX) {
                //{{{
                for (j = j0; j < j0 + step_v0; ++j) {
                  j4 = currMB->block_y + j;
                  j6 = currMB->block_y_aff + j;

                  for (i4 = i0; i4 < i0 + step_h0; ++i4) {
                    sPicMotionParams* colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                      &list1[0]->mv_info[RSD(j6)][RSD(i4)] :
                      &list1[0]->mv_info[j6][i4];
                    sPicMotionParams* mv_info = &picture->mv_info[j4][i4];
                    int mv_y;
                    if (currSlice->mb_aff_frame_flag) {
                      if (!currMB->mb_field && ((currSlice->listX[LIST_1][0]->iCodingType==FRAME_MB_PAIR_CODING && currSlice->listX[LIST_1][0]->motion.mb_field[currMB->mbAddrX]) ||
                          (currSlice->listX[LIST_1][0]->iCodingType==FIELD_CODING))) {
                        if (iabs(picture->poc - currSlice->listX[LIST_1+4][0]->poc)> iabs(picture->poc -currSlice->listX[LIST_1+2][0]->poc) )
                          colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                            &currSlice->listX[LIST_1+2][0]->mv_info[RSD(j6)>>1][RSD(i4)] :
                            &currSlice->listX[LIST_1+2][0]->mv_info[j6>>1][i4];
                        else
                          colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                            &currSlice->listX[LIST_1+4][0]->mv_info[RSD(j6)>>1][RSD(i4)] :
                            &currSlice->listX[LIST_1+4][0]->mv_info[j6>>1][i4];
                        }
                      }
                    else if (!vidParam->active_sps->frame_mbs_only_flag &&
                             !currSlice->field_pic_flag &&
                             currSlice->listX[LIST_1][0]->iCodingType!=FRAME_CODING) {
                      if (iabs(picture->poc - list1[0]->bottom_field->poc) > iabs(picture->poc -list1[0]->top_field->poc) )
                        colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                          &list1[0]->top_field->mv_info[RSD(j6)>>1][RSD(i4)] :
                          &list1[0]->top_field->mv_info[(j6)>>1][i4];
                      else
                        colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                          &list1[0]->bottom_field->mv_info[RSD(j6)>>1][RSD(i4)] :
                          &list1[0]->bottom_field->mv_info[(j6)>>1][i4];
                      }
                    else if (!vidParam->active_sps->frame_mbs_only_flag &&
                             currSlice->field_pic_flag &&
                             currSlice->structure!=list1[0]->structure && list1[0]->coded_frame) {
                      if (currSlice->structure == TOP_FIELD)
                        colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                          &list1[0]->frame->top_field->mv_info[RSD(j6)][RSD(i4)] :
                          &list1[0]->frame->top_field->mv_info[j6][i4];
                      else
                        colocated = vidParam->active_sps->direct_8x8_inference_flag ?
                          &list1[0]->frame->bottom_field->mv_info[RSD(j6)][RSD(i4)] :
                          &list1[0]->frame->bottom_field->mv_info[j6][i4];
                      }

                    mv_y = colocated->mv[refList].mv_y;
                    if ((currSlice->mb_aff_frame_flag && !currMB->mb_field && colocated->ref_pic[refList]->structure!=FRAME) ||
                        (!currSlice->mb_aff_frame_flag && currSlice->field_pic_flag==0 && colocated->ref_pic[refList]->structure!=FRAME))
                      mv_y *= 2;
                    else if ((currSlice->mb_aff_frame_flag && currMB->mb_field && colocated->ref_pic[refList]->structure==FRAME) ||
                             (!currSlice->mb_aff_frame_flag && currSlice->field_pic_flag==1 && colocated->ref_pic[refList]->structure==FRAME))
                      mv_y /= 2;

                    mv_scale = currSlice->mvscale[LIST_0 + list_offset][mapped_idx];
                    mv_info->ref_idx [LIST_0] = (char) mapped_idx;
                    mv_info->ref_idx [LIST_1] = 0;
                    mv_info->ref_pic[LIST_0] = list0[mapped_idx];
                    mv_info->ref_pic[LIST_1] = list1[0];
                    if (mv_scale == 9999 || currSlice->listX[LIST_0+list_offset][mapped_idx]->is_long_term) {
                      mv_info->mv[LIST_0].mv_x = colocated->mv[refList].mv_x;
                      mv_info->mv[LIST_0].mv_y = (short) mv_y;
                      mv_info->mv[LIST_1] = zero_mv;
                      }
                    else {
                      mv_info->mv[LIST_0].mv_x = (short)((mv_scale * colocated->mv[refList].mv_x + 128 ) >> 8);
                      mv_info->mv[LIST_0].mv_y = (short)((mv_scale * mv_y/*colocated->mv[refList].mv_y*/ + 128 ) >> 8);
                      mv_info->mv[LIST_1].mv_x = (short)(mv_info->mv[LIST_0].mv_x - colocated->mv[refList].mv_x);
                      mv_info->mv[LIST_1].mv_y = (short)(mv_info->mv[LIST_0].mv_y - mv_y/*colocated->mv[refList].mv_y*/);
                      }
                    }
                 }
                }
                //}}}
              else if (INVALIDINDEX == mapped_idx)
                error("temporal direct error: colocated block has ref that is unavailable",-1111);
              }
            }
            //}}}
          }
        }
      }
    }
  }
//}}}
//{{{
static inline void update_neighbor_mvs (sPicMotionParams** motion, const sPicMotionParams *mv_info, int i4)
{
  (*motion++)[i4 + 1] = *mv_info;
  (*motion  )[i4    ] = *mv_info;
  (*motion  )[i4 + 1] = *mv_info;
}
//}}}
//{{{
int get_colocated_info_4x4 (sMacroblock* currMB, sPicture *list1, int i, int j)
{
  if (list1->is_long_term)
    return 1;
  else {
    sPicMotionParams *fs = &list1->mv_info[j][i];

    int moving = !((((fs->ref_idx[LIST_0] == 0) &&
                     (iabs(fs->mv[LIST_0].mv_x)>>1 == 0) &&
                     (iabs(fs->mv[LIST_0].mv_y)>>1 == 0))) ||
                   ((fs->ref_idx[LIST_0] == -1) &&
                    (fs->ref_idx[LIST_1] == 0) &&
                    (iabs(fs->mv[LIST_1].mv_x)>>1 == 0) &&
                    (iabs(fs->mv[LIST_1].mv_y)>>1 == 0)));
    return moving;
    }
  }
//}}}
//{{{
int get_colocated_info_8x8 (sMacroblock* currMB, sPicture *list1, int i, int j)
{
  if (list1->is_long_term)
    return 1;
  else {
    sSlice* currSlice = currMB->p_Slice;
    sVidParam* vidParam = currMB->vidParam;
    if( (currSlice->mb_aff_frame_flag) ||
      (!vidParam->active_sps->frame_mbs_only_flag && ((!currSlice->structure && list1->iCodingType == FIELD_CODING)||(currSlice->structure!=list1->structure && list1->coded_frame))))
    {
      int jj = RSD(j);
      int ii = RSD(i);
      int jdiv = (jj>>1);
      int moving;
      sPicMotionParams *fs = &list1->mv_info[jj][ii];

      if(currSlice->field_pic_flag && currSlice->structure!=list1->structure && list1->coded_frame)
      {
         if(currSlice->structure == TOP_FIELD)
           fs = list1->top_field->mv_info[jj] + ii;
         else
           fs = list1->bottom_field->mv_info[jj] + ii;
      }
      else
      {
        if( (currSlice->mb_aff_frame_flag && ((!currMB->mb_field && list1->motion.mb_field[currMB->mbAddrX]) ||
          (!currMB->mb_field && list1->iCodingType == FIELD_CODING)))
          || (!currSlice->mb_aff_frame_flag))
        {
          if (iabs(currSlice->picture->poc - list1->bottom_field->poc)> iabs(currSlice->picture->poc -list1->top_field->poc) )
          {
            fs = list1->top_field->mv_info[jdiv] + ii;
          }
          else
          {
            fs = list1->bottom_field->mv_info[jdiv] + ii;
          }
        }
      }
      moving = !((((fs->ref_idx[LIST_0] == 0)
        &&  (iabs(fs->mv[LIST_0].mv_x)>>1 == 0)
        &&  (iabs(fs->mv[LIST_0].mv_y)>>1 == 0)))
        || ((fs->ref_idx[LIST_0] == -1)
        &&  (fs->ref_idx[LIST_1] == 0)
        &&  (iabs(fs->mv[LIST_1].mv_x)>>1 == 0)
        &&  (iabs(fs->mv[LIST_1].mv_y)>>1 == 0)));
      return moving;
    }
    else
    {
      sPicMotionParams *fs = &list1->mv_info[RSD(j)][RSD(i)];
      int moving;
      if(currMB->vidParam->separate_colour_plane_flag && currMB->vidParam->yuv_format==YUV444)
        fs = &list1->JVmv_info[currMB->p_Slice->colour_plane_id][RSD(j)][RSD(i)];
      moving= !((((fs->ref_idx[LIST_0] == 0)
        &&  (iabs(fs->mv[LIST_0].mv_x)>>1 == 0)
        &&  (iabs(fs->mv[LIST_0].mv_y)>>1 == 0)))
        || ((fs->ref_idx[LIST_0] == -1)
        &&  (fs->ref_idx[LIST_1] == 0)
        &&  (iabs(fs->mv[LIST_1].mv_x)>>1 == 0)
        &&  (iabs(fs->mv[LIST_1].mv_y)>>1 == 0)));
      return moving;
    }
  }
}
//}}}

//{{{
static void update_direct_mv_info_spatial_8x8 (sMacroblock* currMB)
{
  Boolean has_direct = (currMB->b8mode[0] == 0) | (currMB->b8mode[1] == 0) | (currMB->b8mode[2] == 0) | (currMB->b8mode[3] == 0);

  if (has_direct)
  {
    //sVidParam* vidParam = currMB->vidParam;
    sSlice* currSlice = currMB->p_Slice;
    int i,j,k;

    int j4, i4;
    sPicture* picture = currSlice->picture;

    int list_offset = currMB->list_offset; // ((currSlice->mb_aff_frame_flag)&&(currMB->mb_field))? (mb_nr&0x01) ? 4 : 2 : 0;
    sPicture** list0 = currSlice->listX[LIST_0 + list_offset];
    sPicture** list1 = currSlice->listX[LIST_1 + list_offset];

    char  l0_rFrame, l1_rFrame;
    sMotionVector pmvl0, pmvl1;
    int is_not_moving;
    sPicMotionParams *mv_info = NULL;

    prepare_direct_params(currMB, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

    for (k = 0; k < 4; ++k)
    {
      if (currMB->b8mode[k] == 0)
      {
        i = 2 * (k & 0x01);
        j = 2 * (k >> 1);

        //j6 = currMB->block_y_aff + j;
        j4 = currMB->block_y     + j;
        i4 = currMB->block_x     + i;

        mv_info = &picture->mv_info[j4][i4];

        is_not_moving = (get_colocated_info_8x8(currMB, list1[0], i4, currMB->block_y_aff + j) == 0);

        if (is_not_moving && (l0_rFrame == 0 || l1_rFrame == 0))
        {
          if (l1_rFrame == -1)
          {
            if  (l0_rFrame == 0)
            {
              mv_info->ref_pic[LIST_0] = list0[0];
              mv_info->ref_pic[LIST_1] = list1[0];
              mv_info->mv[LIST_0] = zero_mv;
              mv_info->mv[LIST_1] = zero_mv;
              mv_info->ref_idx[LIST_0] = 0;
              mv_info->ref_idx[LIST_1] = -1;
            }
            else
            {
              mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
              mv_info->ref_pic[LIST_1] = NULL;
              mv_info->mv[LIST_0] = pmvl0;
              mv_info->mv[LIST_1] = zero_mv;
              mv_info->ref_idx[LIST_0] = l0_rFrame;
              mv_info->ref_idx[LIST_1] = -1;
            }
          }
          else if (l0_rFrame == -1)
          {
            if  (l1_rFrame == 0)
            {
              mv_info->ref_pic[LIST_0] = NULL;
              mv_info->ref_pic[LIST_1] = list1[0];
              mv_info->mv[LIST_0] = zero_mv;
              mv_info->mv[LIST_1] = zero_mv;
              mv_info->ref_idx[LIST_0] = -1;
              mv_info->ref_idx[LIST_1] = 0;
            }
            else
            {
              mv_info->ref_pic[LIST_0] = NULL;
              mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
              mv_info->mv[LIST_0] = zero_mv;
              mv_info->mv[LIST_1] = pmvl1;
              mv_info->ref_idx[LIST_0] = -1;
              mv_info->ref_idx[LIST_1] = l1_rFrame;
            }
          }
          else
          {
            if  (l0_rFrame == 0)
            {
              mv_info->ref_pic[LIST_0] = list0[0];
              mv_info->mv[LIST_0] = zero_mv;
              mv_info->ref_idx[LIST_0] = 0;
            }
            else
            {
              mv_info->ref_pic[LIST_1] = list1[(short) l0_rFrame];
              mv_info->mv[LIST_0] = pmvl0;
              mv_info->ref_idx[LIST_0] = l0_rFrame;
            }

            if  (l1_rFrame == 0)
            {
              mv_info->ref_pic[LIST_1] = list1[0];
              mv_info->mv[LIST_1] = zero_mv;
              mv_info->ref_idx[LIST_1] = 0;
            }
            else
            {
              mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
              mv_info->mv[LIST_1] = pmvl1;
              mv_info->ref_idx[LIST_1] = l1_rFrame;
            }
          }
        }
        else
        {
          if (l0_rFrame < 0 && l1_rFrame < 0)
          {
            mv_info->ref_pic[LIST_0] = list0[0];
            mv_info->ref_pic[LIST_1] = list1[0];
            mv_info->mv[LIST_0] = zero_mv;
            mv_info->mv[LIST_1] = zero_mv;
            mv_info->ref_idx[LIST_0] = 0;
            mv_info->ref_idx[LIST_1] = 0;
          }
          else if (l0_rFrame < 0)
          {
            mv_info->ref_pic[LIST_0] = NULL;
            mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
            mv_info->mv[LIST_0] = zero_mv;
            mv_info->mv[LIST_1] = pmvl1;
            mv_info->ref_idx[LIST_0] = -1;
            mv_info->ref_idx[LIST_1] = l1_rFrame;
          }
          else  if (l1_rFrame < 0)
          {
            mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
            mv_info->ref_pic[LIST_1] = NULL;

            mv_info->mv[LIST_0] = pmvl0;
            mv_info->mv[LIST_1] = zero_mv;
            mv_info->ref_idx[LIST_0] = l0_rFrame;
            mv_info->ref_idx[LIST_1] = -1;
          }
          else
          {
            mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
            mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
            mv_info->mv[LIST_0] = pmvl0;
            mv_info->mv[LIST_1] = pmvl1;
            mv_info->ref_idx[LIST_0] = l0_rFrame;
            mv_info->ref_idx[LIST_1] = l1_rFrame;
          }
        }
        update_neighbor_mvs(&picture->mv_info[j4], mv_info, i4);
      }
    }
  }
}
//}}}
//{{{
static void update_direct_mv_info_spatial_4x4 (sMacroblock* currMB)
{
  Boolean has_direct = (currMB->b8mode[0] == 0) | (currMB->b8mode[1] == 0) | (currMB->b8mode[2] == 0) | (currMB->b8mode[3] == 0);

  if (has_direct)
  {
    sVidParam* vidParam = currMB->vidParam;
    sSlice* currSlice = currMB->p_Slice;
    int i,j,k;

    int j4, i4;
    sPicture* picture = vidParam->picture;

    int list_offset = currMB->list_offset; // ((currSlice->mb_aff_frame_flag)&&(currMB->mb_field))? (mb_nr&0x01) ? 4 : 2 : 0;
    sPicture** list0 = currSlice->listX[LIST_0 + list_offset];
    sPicture** list1 = currSlice->listX[LIST_1 + list_offset];

    char  l0_rFrame, l1_rFrame;
    sMotionVector pmvl0, pmvl1;

    prepare_direct_params(currMB, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);
    for (k = 0; k < 4; ++k)
    {
      if (currMB->b8mode[k] == 0)
      {

        i = 2 * (k & 0x01);
        for(j = 2 * (k >> 1); j < 2 * (k >> 1)+2;++j)
        {
          j4 = currMB->block_y     + j;

          for(i4 = currMB->block_x + i; i4 < currMB->block_x + i + 2; ++i4)
          {
            sPicMotionParams *mv_info = &picture->mv_info[j4][i4];
            //===== DIRECT PREDICTION =====
            if (l0_rFrame == 0 || l1_rFrame == 0)
            {
              int is_not_moving = (get_colocated_info_4x4(currMB, list1[0], i4, currMB->block_y_aff + j) == 0);

              if (l1_rFrame == -1)
              {
                if (is_not_moving)
                {
                  mv_info->ref_pic[LIST_0] = list0[0];
                  mv_info->ref_pic[LIST_1] = NULL;
                  mv_info->mv[LIST_0] = zero_mv;
                  mv_info->mv[LIST_1] = zero_mv;
                  mv_info->ref_idx[LIST_0] = 0;
                  mv_info->ref_idx[LIST_1] = -1;
                }
                else
                {
                  mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
                  mv_info->ref_pic[LIST_1] = NULL;
                  mv_info->mv[LIST_0] = pmvl0;
                  mv_info->mv[LIST_1] = zero_mv;
                  mv_info->ref_idx[LIST_0] = l0_rFrame;
                  mv_info->ref_idx[LIST_1] = -1;
                }
              }
              else if (l0_rFrame == -1)
              {
                if  (is_not_moving)
                {
                  mv_info->ref_pic[LIST_0] = NULL;
                  mv_info->ref_pic[LIST_1] = list1[0];
                  mv_info->mv[LIST_0] = zero_mv;
                  mv_info->mv[LIST_1] = zero_mv;
                  mv_info->ref_idx[LIST_0] = -1;
                  mv_info->ref_idx[LIST_1] = 0;
                }
                else
                {
                  mv_info->ref_pic[LIST_0] = NULL;
                  mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
                  mv_info->mv[LIST_0] = zero_mv;
                  mv_info->mv[LIST_1] = pmvl1;
                  mv_info->ref_idx[LIST_0] = -1;
                  mv_info->ref_idx[LIST_1] = l1_rFrame;
                }
              }
              else
              {
                if (l0_rFrame == 0 && ((is_not_moving)))
                {
                  mv_info->ref_pic[LIST_0] = list0[0];
                  mv_info->mv[LIST_0] = zero_mv;
                  mv_info->ref_idx[LIST_0] = 0;
                }
                else
                {
                  mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
                  mv_info->mv[LIST_0] = pmvl0;
                  mv_info->ref_idx[LIST_0] = l0_rFrame;
                }

                if  (l1_rFrame == 0 && ((is_not_moving)))
                {
                  mv_info->ref_pic[LIST_1] = list1[0];
                  mv_info->mv[LIST_1] = zero_mv;
                  mv_info->ref_idx[LIST_1]    = 0;
                }
                else
                {
                  mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
                  mv_info->mv[LIST_1] = pmvl1;
                  mv_info->ref_idx[LIST_1] = l1_rFrame;
                }
              }
            }
            else
            {
              mv_info = &picture->mv_info[j4][i4];

              if (l0_rFrame < 0 && l1_rFrame < 0)
              {
                mv_info->ref_pic[LIST_0] = list0[0];
                mv_info->ref_pic[LIST_1] = list1[0];
                mv_info->mv[LIST_0] = zero_mv;
                mv_info->mv[LIST_1] = zero_mv;
                mv_info->ref_idx[LIST_0] = 0;
                mv_info->ref_idx[LIST_1] = 0;
              }
              else if (l1_rFrame == -1)
              {
                mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
                mv_info->ref_pic[LIST_1] = NULL;
                mv_info->mv[LIST_0] = pmvl0;
                mv_info->mv[LIST_1] = zero_mv;
                mv_info->ref_idx[LIST_0] = l0_rFrame;
                mv_info->ref_idx[LIST_1] = -1;
              }
              else if (l0_rFrame == -1)
              {
                mv_info->ref_pic[LIST_0] = NULL;
                mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
                mv_info->mv[LIST_0] = zero_mv;
                mv_info->mv[LIST_1] = pmvl1;
                mv_info->ref_idx[LIST_0] = -1;
                mv_info->ref_idx[LIST_1] = l1_rFrame;
              }
              else
              {
                mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
                mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
                mv_info->mv[LIST_0] = pmvl0;
                mv_info->mv[LIST_1] = pmvl1;
                mv_info->ref_idx[LIST_0] = l0_rFrame;
                mv_info->ref_idx[LIST_1] = l1_rFrame;
              }
            }
          }
        }
      }
    }
  }
}
//}}}
//{{{
void update_direct_types (sSlice* currSlice)
{
  if (currSlice->active_sps->direct_8x8_inference_flag)
    currSlice->update_direct_mv_info =
      currSlice->direct_spatial_mv_pred_flag ? update_direct_mv_info_spatial_8x8 :
                                               update_direct_mv_info_temporal;
  else
    currSlice->update_direct_mv_info =
      currSlice->direct_spatial_mv_pred_flag ? update_direct_mv_info_spatial_4x4 :
                                               update_direct_mv_info_temporal;
}
//}}}
