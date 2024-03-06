//{{{  includes
#include "global.h"
#include "elements.h"

#include "block.h"
#include "mbuffer.h"
#include "errorConceal.h"
#include "macroblock.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "image.h"
#include "mbAccess.h"
#include "biariDecode.h"
#include "transform8x8.h"
#include "transform.h"
#include "mcPrediction.h"
#include "quant.h"
#include "mbPrediction.h"
//}}}
extern int get_colocated_info_8x8 (sMacroblock* curMb, sPicture *list1, int i, int j);
extern int get_colocated_info_4x4 (sMacroblock* curMb, sPicture *list1, int i, int j);

//{{{
int mb_pred_intra4x4 (sMacroblock* curMb, sColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  sSlice* curSlice = curMb->p_Slice;
  int yuv = picture->chroma_format_idc - 1;
  int i=0, j=0,k, j4=0,i4=0;
  int j_pos, i_pos;
  int ioff,joff;
  int block8x8;   // needed for ABT
  curMb->itrans_4x4 = (curMb->is_lossless == FALSE) ? itrans4x4 : Inv_Residual_trans_4x4;

  for (block8x8 = 0; block8x8 < 4; block8x8++)
  {
    for (k = block8x8 * 4; k < block8x8 * 4 + 4; k ++)
    {
      i =  (decode_block_scan[k] & 3);
      j = ((decode_block_scan[k] >> 2) & 3);

      ioff = (i << 2);
      joff = (j << 2);
      i4   = curMb->block_x + i;
      j4   = curMb->block_y + j;
      j_pos = j4 * BLOCK_SIZE;
      i_pos = i4 * BLOCK_SIZE;

      // PREDICTION
      //===== INTRA PREDICTION =====
      if (curSlice->intra_pred_4x4(curMb, curPlane, ioff,joff,i4,j4) == SEARCH_SYNC)  /* make 4x4 prediction block mpr from given prediction vidParam->mb_mode */
        return SEARCH_SYNC;                   /* bit error */
      // =============== 4x4 itrans ================
      // -------------------------------------------
      curMb->itrans_4x4  (curMb, curPlane, ioff, joff);

      copy_Image_4x4(&curPixel[j_pos], &curSlice->mb_rec[curPlane][joff], i_pos, ioff);
    }
  }

  // chroma decoding** *****************************************************
  if ((picture->chroma_format_idc != YUV400) && (picture->chroma_format_idc != YUV444))
  {
    intra_cr_decoding(curMb, yuv);
  }

  if (curMb->cbp != 0)
    curSlice->is_reset_coeff = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_intra16x16 (sMacroblock* curMb, sColorPlane curPlane, sPicture* picture)
{
  int yuv = picture->chroma_format_idc - 1;

  curMb->p_Slice->intra_pred_16x16(curMb, curPlane, curMb->i16mode);
  curMb->ipmode_DPCM = (char) curMb->i16mode; //For residual DPCM
  // =============== 4x4 itrans ================
  // -------------------------------------------
  iMBtrans4x4(curMb, curPlane, 0);

  // chroma decoding** *****************************************************
  if ((picture->chroma_format_idc != YUV400) && (picture->chroma_format_idc != YUV444))
  {
    intra_cr_decoding(curMb, yuv);
  }

  curMb->p_Slice->is_reset_coeff = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_intra8x8 (sMacroblock* curMb, sColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  sSlice* curSlice = curMb->p_Slice;
  int yuv = picture->chroma_format_idc - 1;

  int block8x8;   // needed for ABT
  curMb->itrans_8x8 = (curMb->is_lossless == FALSE) ? itrans8x8 : Inv_Residual_trans_8x8;

  for (block8x8 = 0; block8x8 < 4; block8x8++)
  {
    //=========== 8x8 BLOCK TYPE ============
    int ioff = (block8x8 & 0x01) << 3;
    int joff = (block8x8 >> 1  ) << 3;

    //PREDICTION
    curSlice->intra_pred_8x8(curMb, curPlane, ioff, joff);
    if (curMb->cbp & (1 << block8x8))
      curMb->itrans_8x8    (curMb, curPlane, ioff,joff);      // use inverse integer transform and make 8x8 block m7 from prediction block mpr
    else
      icopy8x8(curMb, curPlane, ioff,joff);

    copy_Image_8x8(&curPixel[curMb->pix_y + joff], &curSlice->mb_rec[curPlane][joff], curMb->pix_x + ioff, ioff);
  }
  // chroma decoding** *****************************************************
  if ((picture->chroma_format_idc != YUV400) && (picture->chroma_format_idc != YUV444))
  {
    intra_cr_decoding(curMb, yuv);
  }

  if (curMb->cbp != 0)
    curSlice->is_reset_coeff = FALSE;
  return 1;
}
//}}}

//{{{
static void set_chroma_vector (sMacroblock* curMb)
{
  sSlice* curSlice = curMb->p_Slice;
  sVidParam* vidParam = curMb->vidParam;

  if (!curSlice->mb_aff_frame_flag)
  {
    if(curSlice->structure == TOP_FIELD)
    {
      int k,l;
      for (l = LIST_0; l <= (LIST_1); l++)
      {
        for(k = 0; k < curSlice->listXsize[l]; k++)
        {
          if(curSlice->structure != curSlice->listX[l][k]->structure)
            curSlice->chroma_vector_adjustment[l][k] = -2;
          else
            curSlice->chroma_vector_adjustment[l][k] = 0;
        }
      }
    }
    else if(curSlice->structure == BOTTOM_FIELD)
    {
      int k,l;
      for (l = LIST_0; l <= (LIST_1); l++)
      {
        for(k = 0; k < curSlice->listXsize[l]; k++)
        {
          if (curSlice->structure != curSlice->listX[l][k]->structure)
            curSlice->chroma_vector_adjustment[l][k] = 2;
          else
            curSlice->chroma_vector_adjustment[l][k] = 0;
        }
      }
    }
    else
    {
      int k,l;
      for (l = LIST_0; l <= (LIST_1); l++)
      {
        for(k = 0; k < curSlice->listXsize[l]; k++)
        {
          curSlice->chroma_vector_adjustment[l][k] = 0;
        }
      }
    }
  }
  else
  {
    int mb_nr = (curMb->mbAddrX & 0x01);
    int k,l;

    //////////////////////////
    // find out the correct list offsets
    if (curMb->mb_field)
    {
      int list_offset = curMb->list_offset;

      for (l = LIST_0 + list_offset; l <= (LIST_1 + list_offset); l++)
      {
        for(k = 0; k < curSlice->listXsize[l]; k++)
        {
          if(mb_nr == 0 && curSlice->listX[l][k]->structure == BOTTOM_FIELD)
            curSlice->chroma_vector_adjustment[l][k] = -2;
          else if(mb_nr == 1 && curSlice->listX[l][k]->structure == TOP_FIELD)
            curSlice->chroma_vector_adjustment[l][k] = 2;
          else
            curSlice->chroma_vector_adjustment[l][k] = 0;
        }
      }
    }
    else
    {
      for (l = LIST_0; l <= (LIST_1); l++)
      {
        for(k = 0; k < curSlice->listXsize[l]; k++)
        {
          curSlice->chroma_vector_adjustment[l][k] = 0;
        }
      }
    }
  }

  curSlice->max_mb_vmv_r = (curSlice->structure != FRAME || ( curMb->mb_field )) ? vidParam->max_vmv_r >> 1 : vidParam->max_vmv_r;
}
//}}}

//{{{
int mb_pred_skip (sMacroblock* curMb, sColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  sSlice* curSlice = curMb->p_Slice;
  sVidParam* vidParam = curMb->vidParam;

  set_chroma_vector(curMb);

  perform_mc(curMb, curPlane, picture, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

  copy_Image_16x16(&curPixel[curMb->pix_y], curSlice->mb_pred[curPlane], curMb->pix_x, 0);

  if ((picture->chroma_format_idc != YUV400) && (picture->chroma_format_idc != YUV444))
  {

    copy_Image(&picture->imgUV[0][curMb->pix_c_y], curSlice->mb_pred[1], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
    copy_Image(&picture->imgUV[1][curMb->pix_c_y], curSlice->mb_pred[2], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
  }
  return 1;
}
//}}}
//{{{
int mb_pred_sp_skip (sMacroblock* curMb, sColorPlane curPlane, sPicture* picture)
{
  set_chroma_vector(curMb);

  perform_mc(curMb, curPlane, picture, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  iTransform(curMb, curPlane, 1);
  return 1;
}
//}}}

//{{{
int mb_pred_p_inter8x8 (sMacroblock* curMb, sColorPlane curPlane, sPicture* picture)
{
  int block8x8;   // needed for ABT
  int i=0, j=0,k;

  sSlice* curSlice = curMb->p_Slice;
  int smb = curSlice->slice_type == SP_SLICE && (curMb->is_intra_block == FALSE);

  set_chroma_vector(curMb);

  for (block8x8=0; block8x8<4; block8x8++)
  {
    int mv_mode  = curMb->b8mode[block8x8];
    int pred_dir = curMb->b8pdir[block8x8];

    int k_start = (block8x8 << 2);
    int k_inc = (mv_mode == SMB8x4) ? 2 : 1;
    int k_end = (mv_mode == SMB8x8) ? k_start + 1 : ((mv_mode == SMB4x4) ? k_start + 4 : k_start + k_inc + 1);

    int block_size_x = ( mv_mode == SMB8x4 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;
    int block_size_y = ( mv_mode == SMB4x8 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;

    for (k = k_start; k < k_end; k += k_inc)
    {
      i =  (decode_block_scan[k] & 3);
      j = ((decode_block_scan[k] >> 2) & 3);
      perform_mc(curMb, curPlane, picture, pred_dir, i, j, block_size_x, block_size_y);
    }
  }

  iTransform (curMb, curPlane, smb);

  if (curMb->cbp != 0)
    curSlice->is_reset_coeff = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_p_inter16x16 (sMacroblock* curMb, sColorPlane curPlane, sPicture* picture)
{
  sSlice* curSlice = curMb->p_Slice;
  int smb = (curSlice->slice_type == SP_SLICE);

  set_chroma_vector(curMb);
  perform_mc (curMb, curPlane, picture, curMb->b8pdir[0], 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  iTransform (curMb, curPlane, smb);

  if (curMb->cbp != 0)
    curSlice->is_reset_coeff = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_p_inter16x8 (sMacroblock* curMb, sColorPlane curPlane, sPicture* picture)
{
  sSlice* curSlice = curMb->p_Slice;
  int smb = (curSlice->slice_type == SP_SLICE);

  set_chroma_vector(curMb);

  perform_mc(curMb, curPlane, picture, curMb->b8pdir[0], 0, 0, MB_BLOCK_SIZE, BLOCK_SIZE_8x8);
  perform_mc(curMb, curPlane, picture, curMb->b8pdir[2], 0, 2, MB_BLOCK_SIZE, BLOCK_SIZE_8x8);
  iTransform(curMb, curPlane, smb);

  if (curMb->cbp != 0)
    curSlice->is_reset_coeff = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_p_inter8x16 (sMacroblock* curMb, sColorPlane curPlane, sPicture* picture)
{
  sSlice* curSlice = curMb->p_Slice;
  int smb = (curSlice->slice_type == SP_SLICE);

  set_chroma_vector(curMb);

  perform_mc(curMb, curPlane, picture, curMb->b8pdir[0], 0, 0, BLOCK_SIZE_8x8, MB_BLOCK_SIZE);
  perform_mc(curMb, curPlane, picture, curMb->b8pdir[1], 2, 0, BLOCK_SIZE_8x8, MB_BLOCK_SIZE);
  iTransform(curMb, curPlane, smb);

  if (curMb->cbp != 0)
    curSlice->is_reset_coeff = FALSE;
  return 1;
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
int mb_pred_b_d8x8temporal (sMacroblock* curMb, sColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  short ref_idx;
  int refList;

  int k, i, j, i4, j4, j6;
  int block8x8;   // needed for ABT
  sSlice* curSlice = curMb->p_Slice;
  sVidParam* vidParam = curMb->vidParam;
  sPicMotionParams *mv_info = NULL, *colocated = NULL;

  int list_offset = curMb->list_offset;
  sPicture** list0 = curSlice->listX[LIST_0 + list_offset];
  sPicture** list1 = curSlice->listX[LIST_1 + list_offset];

  set_chroma_vector(curMb);

  //printf("curMb %d\n", curMb->mbAddrX);
  for (block8x8=0; block8x8<4; block8x8++)
  {
    int pred_dir = curMb->b8pdir[block8x8];

    int k_start = (block8x8 << 2);
    int k_end = k_start + 1;

    for (k = k_start; k < k_start + BLOCK_MULTIPLE; k ++)
    {
      i =  (decode_block_scan[k] & 3);
      j = ((decode_block_scan[k] >> 2) & 3);
      i4   = curMb->block_x + i;
      j4   = curMb->block_y + j;
      j6   = curMb->block_y_aff + j;
      mv_info = &picture->mv_info[j4][i4];
      colocated = &list1[0]->mv_info[RSD(j6)][RSD(i4)];
      if(curMb->vidParam->separate_colour_plane_flag && curMb->vidParam->yuv_format==YUV444)
        colocated = &list1[0]->JVmv_info[curMb->p_Slice->colour_plane_id][RSD(j6)][RSD(i4)];
      if(curSlice->mb_aff_frame_flag /*&& (!vidParam->active_sps->frame_mbs_only_flag || vidParam->active_sps->direct_8x8_inference_flag)*/)
      {
        assert(vidParam->active_sps->direct_8x8_inference_flag);
        if(!curMb->mb_field && ((curSlice->listX[LIST_1][0]->iCodingType==FRAME_MB_PAIR_CODING && curSlice->listX[LIST_1][0]->motion.mb_field[curMb->mbAddrX]) ||
          (curSlice->listX[LIST_1][0]->iCodingType==FIELD_CODING)))
        {
          if (iabs(picture->poc - curSlice->listX[LIST_1+4][0]->poc)> iabs(picture->poc -curSlice->listX[LIST_1+2][0]->poc) )
          {
            colocated = vidParam->active_sps->direct_8x8_inference_flag ?
              &curSlice->listX[LIST_1+2][0]->mv_info[RSD(j6)>>1][RSD(i4)] : &curSlice->listX[LIST_1+2][0]->mv_info[j6>>1][i4];
          }
          else
          {
            colocated = vidParam->active_sps->direct_8x8_inference_flag ?
              &curSlice->listX[LIST_1+4][0]->mv_info[RSD(j6)>>1][RSD(i4)] : &curSlice->listX[LIST_1+4][0]->mv_info[j6>>1][i4];
          }
        }
      }
      else if(/*!curSlice->mb_aff_frame_flag &&*/ !vidParam->active_sps->frame_mbs_only_flag &&
        (!curSlice->field_pic_flag && curSlice->listX[LIST_1][0]->iCodingType!=FRAME_CODING))
      {
        if (iabs(picture->poc - list1[0]->bottom_field->poc)> iabs(picture->poc -list1[0]->top_field->poc) )
        {
          colocated = vidParam->active_sps->direct_8x8_inference_flag ?
            &list1[0]->top_field->mv_info[RSD(j6)>>1][RSD(i4)] : &list1[0]->top_field->mv_info[j6>>1][i4];
        }
        else
        {
          colocated = vidParam->active_sps->direct_8x8_inference_flag ?
            &list1[0]->bottom_field->mv_info[RSD(j6)>>1][RSD(i4)] : &list1[0]->bottom_field->mv_info[j6>>1][i4];
        }
      }
      else if(!vidParam->active_sps->frame_mbs_only_flag && curSlice->field_pic_flag && curSlice->structure!=list1[0]->structure && list1[0]->coded_frame)
      {
        if (curSlice->structure == TOP_FIELD)
        {
          colocated = vidParam->active_sps->direct_8x8_inference_flag ?
            &list1[0]->frame->top_field->mv_info[RSD(j6)][RSD(i4)] : &list1[0]->frame->top_field->mv_info[j6][i4];
        }
        else
        {
          colocated = vidParam->active_sps->direct_8x8_inference_flag ?
            &list1[0]->frame->bottom_field->mv_info[RSD(j6)][RSD(i4)] : &list1[0]->frame->bottom_field->mv_info[j6][i4];
        }
      }


      assert (pred_dir<=2);

      refList = (colocated->ref_idx[LIST_0]== -1 ? LIST_1 : LIST_0);
      ref_idx =  colocated->ref_idx[refList];

      if(ref_idx==-1) // co-located is intra mode
      {
        mv_info->mv[LIST_0] = zero_mv;
        mv_info->mv[LIST_1] = zero_mv;

        mv_info->ref_idx[LIST_0] = 0;
        mv_info->ref_idx[LIST_1] = 0;
      }
      else // co-located skip or inter mode
      {
        int mapped_idx=0;
        int iref;
        if( (curSlice->mb_aff_frame_flag && ( (curMb->mb_field && colocated->ref_pic[refList]->structure==FRAME) ||
          (!curMb->mb_field && colocated->ref_pic[refList]->structure!=FRAME))) ||
          (!curSlice->mb_aff_frame_flag && ((curSlice->field_pic_flag==0 && colocated->ref_pic[refList]->structure!=FRAME)
          ||(curSlice->field_pic_flag==1 && colocated->ref_pic[refList]->structure==FRAME))) )
        {
          for (iref = 0; iref < imin(curSlice->num_ref_idx_active[LIST_0], curSlice->listXsize[LIST_0 + list_offset]);iref++)
          {
            if(curSlice->listX[LIST_0 + list_offset][iref]->top_field == colocated->ref_pic[refList] ||
              curSlice->listX[LIST_0 + list_offset][iref]->bottom_field == colocated->ref_pic[refList] ||
              curSlice->listX[LIST_0 + list_offset][iref]->frame == colocated->ref_pic[refList])
            {
              if ((curSlice->field_pic_flag==1) && (curSlice->listX[LIST_0 + list_offset][iref]->structure != curSlice->structure))
              {
                mapped_idx=INVALIDINDEX;
              }
              else
              {
                mapped_idx = iref;
                break;
              }
            }
            else //! invalid index. Default to zero even though this case should not happen
            {
              mapped_idx=INVALIDINDEX;
            }
          }
        }
        else
        {
          for (iref = 0; iref < imin(curSlice->num_ref_idx_active[LIST_0], curSlice->listXsize[LIST_0 + list_offset]);iref++)
          {
            if(curSlice->listX[LIST_0 + list_offset][iref] == colocated->ref_pic[refList])
            {
              mapped_idx = iref;
              break;
            }
            else //! invalid index. Default to zero even though this case should not happen
            {
              mapped_idx=INVALIDINDEX;
            }
          }
        }

        if(INVALIDINDEX != mapped_idx)
        {
          int mv_scale = curSlice->mvscale[LIST_0 + list_offset][mapped_idx];
          int mv_y = colocated->mv[refList].mv_y;
          if((curSlice->mb_aff_frame_flag && !curMb->mb_field && colocated->ref_pic[refList]->structure!=FRAME) ||
            (!curSlice->mb_aff_frame_flag && curSlice->field_pic_flag==0 && colocated->ref_pic[refList]->structure!=FRAME) )
            mv_y *= 2;
          else if((curSlice->mb_aff_frame_flag && curMb->mb_field && colocated->ref_pic[refList]->structure==FRAME) ||
            (!curSlice->mb_aff_frame_flag && curSlice->field_pic_flag==1 && colocated->ref_pic[refList]->structure==FRAME) )
            mv_y /= 2;

          //! In such case, an array is needed for each different reference.
          if (mv_scale == 9999 || curSlice->listX[LIST_0 + list_offset][mapped_idx]->is_long_term)
          {
            mv_info->mv[LIST_0].mv_x = colocated->mv[refList].mv_x;
            mv_info->mv[LIST_0].mv_y = (short) mv_y;
            mv_info->mv[LIST_1] = zero_mv;
          }
          else
          {
            mv_info->mv[LIST_0].mv_x = (short) ((mv_scale * colocated->mv[refList].mv_x + 128 ) >> 8);
            mv_info->mv[LIST_0].mv_y = (short) ((mv_scale * mv_y/*colocated->mv[refList].mv_y*/ + 128 ) >> 8);

            mv_info->mv[LIST_1].mv_x = (short) (mv_info->mv[LIST_0].mv_x - colocated->mv[refList].mv_x);
            mv_info->mv[LIST_1].mv_y = (short) (mv_info->mv[LIST_0].mv_y - mv_y/*colocated->mv[refList].mv_y*/);
          }

          mv_info->ref_idx[LIST_0] = (char) mapped_idx; //colocated->ref_idx[refList];
          mv_info->ref_idx[LIST_1] = 0;
        }
        else if (INVALIDINDEX == mapped_idx)
        {
          error("temporal direct error: colocated block has ref that is unavailable",-1111);
        }

      }
      // store reference picture ID determined by direct mode
      mv_info->ref_pic[LIST_0] = list0[(short)mv_info->ref_idx[LIST_0]];
      mv_info->ref_pic[LIST_1] = list1[(short)mv_info->ref_idx[LIST_1]];
    }

    for (k = k_start; k < k_end; k ++)
    {
      int i =  (decode_block_scan[k] & 3);
      int j = ((decode_block_scan[k] >> 2) & 3);
      perform_mc(curMb, curPlane, picture, pred_dir, i, j, SMB_BLOCK_SIZE, SMB_BLOCK_SIZE);
    }
  }

  if (curMb->cbp == 0)
  {
    copy_Image_16x16(&curPixel[curMb->pix_y], curSlice->mb_pred[curPlane], curMb->pix_x, 0);

    if ((picture->chroma_format_idc != YUV400) && (picture->chroma_format_idc != YUV444))
    {
      copy_Image(&picture->imgUV[0][curMb->pix_c_y], curSlice->mb_pred[1], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
      copy_Image(&picture->imgUV[1][curMb->pix_c_y], curSlice->mb_pred[2], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
    }
  }
  else
  {
    iTransform(curMb, curPlane, 0);
    curSlice->is_reset_coeff = FALSE;
  }
  return 1;
}
//}}}
//{{{
int mb_pred_b_d4x4temporal (sMacroblock* curMb, sColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  short ref_idx;
  int refList;

  int k;
  int block8x8;   // needed for ABT
  sSlice* curSlice = curMb->p_Slice;
  sVidParam* vidParam = curMb->vidParam;

  int list_offset = curMb->list_offset;
  sPicture** list0 = curSlice->listX[LIST_0 + list_offset];
  sPicture** list1 = curSlice->listX[LIST_1 + list_offset];

  set_chroma_vector(curMb);

  for (block8x8=0; block8x8<4; block8x8++)
  {
    int pred_dir = curMb->b8pdir[block8x8];

    int k_start = (block8x8 << 2);
    int k_end = k_start + BLOCK_MULTIPLE;

    for (k = k_start; k < k_end; k ++)
    {

      int i =  (decode_block_scan[k] & 3);
      int j = ((decode_block_scan[k] >> 2) & 3);
      int i4   = curMb->block_x + i;
      int j4   = curMb->block_y + j;
      int j6   = curMb->block_y_aff + j;
      sPicMotionParams *mv_info = &picture->mv_info[j4][i4];
      sPicMotionParams *colocated = &list1[0]->mv_info[j6][i4];
      if(curMb->vidParam->separate_colour_plane_flag && curMb->vidParam->yuv_format==YUV444)
        colocated = &list1[0]->JVmv_info[curMb->p_Slice->colour_plane_id][RSD(j6)][RSD(i4)];
      assert (pred_dir<=2);

      refList = (colocated->ref_idx[LIST_0]== -1 ? LIST_1 : LIST_0);
      ref_idx =  colocated->ref_idx[refList];

      if(ref_idx==-1) // co-located is intra mode
      {
        mv_info->mv[LIST_0] = zero_mv;
        mv_info->mv[LIST_1] = zero_mv;

        mv_info->ref_idx[LIST_0] = 0;
        mv_info->ref_idx[LIST_1] = 0;
      }
      else // co-located skip or inter mode
      {
        int mapped_idx=0;
        int iref;

        for (iref=0;iref<imin(curSlice->num_ref_idx_active[LIST_0], curSlice->listXsize[LIST_0 + list_offset]);iref++)
        {
          if(curSlice->listX[LIST_0 + list_offset][iref] == colocated->ref_pic[refList])
          {
            mapped_idx=iref;
            break;
          }
          else //! invalid index. Default to zero even though this case should not happen
          {
            mapped_idx=INVALIDINDEX;
          }
        }
        if (INVALIDINDEX == mapped_idx)
        {
          error("temporal direct error: colocated block has ref that is unavailable",-1111);
        }
        else
        {
          int mv_scale = curSlice->mvscale[LIST_0 + list_offset][mapped_idx];

          //! In such case, an array is needed for each different reference.
          if (mv_scale == 9999 || curSlice->listX[LIST_0+list_offset][mapped_idx]->is_long_term)
          {
            mv_info->mv[LIST_0] = colocated->mv[refList];
            mv_info->mv[LIST_1] = zero_mv;
          }
          else
          {
            mv_info->mv[LIST_0].mv_x = (short) ((mv_scale * colocated->mv[refList].mv_x + 128 ) >> 8);
            mv_info->mv[LIST_0].mv_y = (short) ((mv_scale * colocated->mv[refList].mv_y + 128 ) >> 8);

            mv_info->mv[LIST_1].mv_x = (short) (mv_info->mv[LIST_0].mv_x - colocated->mv[refList].mv_x);
            mv_info->mv[LIST_1].mv_y = (short) (mv_info->mv[LIST_0].mv_y - colocated->mv[refList].mv_y);
          }

          mv_info->ref_idx[LIST_0] = (char) mapped_idx; //colocated->ref_idx[refList];
          mv_info->ref_idx[LIST_1] = 0;
        }
      }
      // store reference picture ID determined by direct mode
      mv_info->ref_pic[LIST_0] = list0[(short)mv_info->ref_idx[LIST_0]];
      mv_info->ref_pic[LIST_1] = list1[(short)mv_info->ref_idx[LIST_1]];
    }

    for (k = k_start; k < k_end; k ++)
    {
      int i =  (decode_block_scan[k] & 3);
      int j = ((decode_block_scan[k] >> 2) & 3);
      perform_mc(curMb, curPlane, picture, pred_dir, i, j, BLOCK_SIZE, BLOCK_SIZE);

    }
  }

  if (curMb->cbp == 0)
  {
    copy_Image_16x16(&curPixel[curMb->pix_y], curSlice->mb_pred[curPlane], curMb->pix_x, 0);

    if ((picture->chroma_format_idc != YUV400) && (picture->chroma_format_idc != YUV444))
    {
      copy_Image(&picture->imgUV[0][curMb->pix_c_y], curSlice->mb_pred[1], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
      copy_Image(&picture->imgUV[1][curMb->pix_c_y], curSlice->mb_pred[2], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
    }
  }
  else
  {
    iTransform(curMb, curPlane, 0);
    curSlice->is_reset_coeff = FALSE;
  }

  return 1;
}
//}}}

//{{{
int mb_pred_b_d8x8spatial (sMacroblock* curMb, sColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  char l0_rFrame = -1, l1_rFrame = -1;
  sMotionVector pmvl0 = zero_mv, pmvl1 = zero_mv;
  int i4, j4;
  int block8x8;
  sSlice* curSlice = curMb->p_Slice;
  sVidParam* vidParam = curMb->vidParam;

  sPicMotionParams *mv_info;
  int list_offset = curMb->list_offset;
  sPicture** list0 = curSlice->listX[LIST_0 + list_offset];
  sPicture** list1 = curSlice->listX[LIST_1 + list_offset];

  int pred_dir = 0;

  set_chroma_vector(curMb);

  prepare_direct_params(curMb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

  if (l0_rFrame == 0 || l1_rFrame == 0)
  {
    int is_not_moving;

    for (block8x8 = 0; block8x8 < 4; block8x8++)
    {
      int k_start = (block8x8 << 2);

      int i  =  (decode_block_scan[k_start] & 3);
      int j  = ((decode_block_scan[k_start] >> 2) & 3);
      i4  = curMb->block_x + i;
      j4  = curMb->block_y + j;

      is_not_moving = (get_colocated_info_8x8(curMb, list1[0], i4, curMb->block_y_aff + j) == 0);

      mv_info = &picture->mv_info[j4][i4];

      //===== DIRECT PREDICTION =====
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
        pred_dir = 0;
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
        pred_dir = 1;
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
        pred_dir = 2;
      }

      update_neighbor_mvs(&picture->mv_info[j4], mv_info, i4);
      perform_mc(curMb, curPlane, picture, pred_dir, i, j, SMB_BLOCK_SIZE, SMB_BLOCK_SIZE);
    }
  }
  else
  {
    //===== DIRECT PREDICTION =====
    if (l0_rFrame < 0 && l1_rFrame < 0)
    {
      pred_dir = 2;
      for (j4 = curMb->block_y; j4 < curMb->block_y + BLOCK_MULTIPLE; j4 += 2)
      {
        for (i4 = curMb->block_x; i4 < curMb->block_x + BLOCK_MULTIPLE; i4 += 2)
        {
          mv_info = &picture->mv_info[j4][i4];

          mv_info->ref_pic[LIST_0] = list0[0];
          mv_info->ref_pic[LIST_1] = list1[0];
          mv_info->mv[LIST_0] = zero_mv;
          mv_info->mv[LIST_1] = zero_mv;
          mv_info->ref_idx[LIST_0] = 0;
          mv_info->ref_idx[LIST_1] = 0;

          update_neighbor_mvs(&picture->mv_info[j4], mv_info, i4);
        }
      }
    }
    else if (l1_rFrame == -1)
    {
      pred_dir = 0;

      for (j4 = curMb->block_y; j4 < curMb->block_y + BLOCK_MULTIPLE; j4 += 2)
      {
        for (i4 = curMb->block_x; i4 < curMb->block_x + BLOCK_MULTIPLE; i4 += 2)
        {
          mv_info = &picture->mv_info[j4][i4];

          mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
          mv_info->ref_pic[LIST_1] = NULL;
          mv_info->mv[LIST_0] = pmvl0;
          mv_info->mv[LIST_1] = zero_mv;
          mv_info->ref_idx[LIST_0] = l0_rFrame;
          mv_info->ref_idx[LIST_1] = -1;

          update_neighbor_mvs(&picture->mv_info[j4], mv_info, i4);
        }
      }
    }
    else if (l0_rFrame == -1)
    {
      pred_dir = 1;
      for (j4 = curMb->block_y; j4 < curMb->block_y + BLOCK_MULTIPLE; j4 += 2)
      {
        for (i4 = curMb->block_x; i4 < curMb->block_x + BLOCK_MULTIPLE; i4 += 2)
        {
          mv_info = &picture->mv_info[j4][i4];

          mv_info->ref_pic[LIST_0] = NULL;
          mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
          mv_info->mv[LIST_0] = zero_mv;
          mv_info->mv[LIST_1] = pmvl1;
          mv_info->ref_idx[LIST_0] = -1;
          mv_info->ref_idx[LIST_1] = l1_rFrame;

          update_neighbor_mvs(&picture->mv_info[j4], mv_info, i4);
        }
      }
    }
    else
    {
      pred_dir = 2;

      for (j4 = curMb->block_y; j4 < curMb->block_y + BLOCK_MULTIPLE; j4 += 2)
      {
        for (i4 = curMb->block_x; i4 < curMb->block_x + BLOCK_MULTIPLE; i4 += 2)
        {
          mv_info = &picture->mv_info[j4][i4];

          mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
          mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
          mv_info->mv[LIST_0] = pmvl0;
          mv_info->mv[LIST_1] = pmvl1;
          mv_info->ref_idx[LIST_0] = l0_rFrame;
          mv_info->ref_idx[LIST_1] = l1_rFrame;

          update_neighbor_mvs(&picture->mv_info[j4], mv_info, i4);
        }
      }
    }
    // Now perform Motion Compensation
    perform_mc(curMb, curPlane, picture, pred_dir, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  }

  if (curMb->cbp == 0)
  {
    copy_Image_16x16(&curPixel[curMb->pix_y], curSlice->mb_pred[curPlane], curMb->pix_x, 0);

    if ((picture->chroma_format_idc != YUV400) && (picture->chroma_format_idc != YUV444))
     {
      copy_Image(&picture->imgUV[0][curMb->pix_c_y], curSlice->mb_pred[1], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
      copy_Image(&picture->imgUV[1][curMb->pix_c_y], curSlice->mb_pred[2], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
    }
  }
  else
  {
    iTransform(curMb, curPlane, 0);
    curSlice->is_reset_coeff = FALSE;
  }

  return 1;
}
//}}}
//{{{
int mb_pred_b_d4x4spatial (sMacroblock* curMb, sColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  char l0_rFrame = -1, l1_rFrame = -1;
  sMotionVector pmvl0 = zero_mv, pmvl1 = zero_mv;
  int k;
  int block8x8;
  sSlice* curSlice = curMb->p_Slice;
  sVidParam* vidParam = curMb->vidParam;

  sPicMotionParams *mv_info;
  int list_offset = curMb->list_offset;
  sPicture** list0 = curSlice->listX[LIST_0 + list_offset];
  sPicture** list1 = curSlice->listX[LIST_1 + list_offset];

  int pred_dir = 0;

  set_chroma_vector(curMb);

  prepare_direct_params(curMb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

  for (block8x8 = 0; block8x8 < 4; block8x8++)
  {
    int k_start = (block8x8 << 2);
    int k_end = k_start + BLOCK_MULTIPLE;

    for (k = k_start; k < k_end; k ++)
    {
      int i  =  (decode_block_scan[k] & 3);
      int j  = ((decode_block_scan[k] >> 2) & 3);
      int i4  = curMb->block_x + i;
      int j4  = curMb->block_y + j;

      mv_info = &picture->mv_info[j4][i4];
      //===== DIRECT PREDICTION =====
      if (l0_rFrame == 0 || l1_rFrame == 0)
      {
        int is_not_moving = (get_colocated_info_4x4(curMb, list1[0], i4, curMb->block_y_aff + j) == 0);

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
          pred_dir = 0;
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
          pred_dir = 1;
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
            mv_info->ref_idx[LIST_1] = 0;
          }
          else
          {
            mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
            mv_info->mv[LIST_1] = pmvl1;
            mv_info->ref_idx[LIST_1] = l1_rFrame;
          }
          pred_dir = 2;
        }
      }
      else
      {
        mv_info = &picture->mv_info[j4][i4];

        if (l0_rFrame < 0 && l1_rFrame < 0)
        {
          pred_dir = 2;
          mv_info->ref_pic[LIST_0] = list0[0];
          mv_info->ref_pic[LIST_1] = list1[0];
          mv_info->mv[LIST_0] = zero_mv;
          mv_info->mv[LIST_1] = zero_mv;
          mv_info->ref_idx[LIST_0] = 0;
          mv_info->ref_idx[LIST_1] = 0;
        }
        else if (l1_rFrame == -1)
        {
          pred_dir = 0;
          mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
          mv_info->ref_pic[LIST_1] = NULL;
          mv_info->mv[LIST_0] = pmvl0;
          mv_info->mv[LIST_1] = zero_mv;
          mv_info->ref_idx[LIST_0] = l0_rFrame;
          mv_info->ref_idx[LIST_1] = -1;
        }
        else if (l0_rFrame == -1)
        {
          pred_dir = 1;
          mv_info->ref_pic[LIST_0] = NULL;
          mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
          mv_info->mv[LIST_0] = zero_mv;
          mv_info->mv[LIST_1] = pmvl1;
          mv_info->ref_idx[LIST_0] = -1;
          mv_info->ref_idx[LIST_1] = l1_rFrame;
        }
        else
        {
          pred_dir = 2;
          mv_info->ref_pic[LIST_0] = list0[(short) l0_rFrame];
          mv_info->ref_pic[LIST_1] = list1[(short) l1_rFrame];
          mv_info->mv[LIST_0] = pmvl0;
          mv_info->mv[LIST_1] = pmvl1;
          mv_info->ref_idx[LIST_0] = l0_rFrame;
          mv_info->ref_idx[LIST_1] = l1_rFrame;
        }
      }
    }

    for (k = k_start; k < k_end; k ++)
    {
      int i =  (decode_block_scan[k] & 3);
      int j = ((decode_block_scan[k] >> 2) & 3);

      perform_mc(curMb, curPlane, picture, pred_dir, i, j, BLOCK_SIZE, BLOCK_SIZE);
    }
  }

  if (curMb->cbp == 0)
  {
    copy_Image_16x16(&curPixel[curMb->pix_y], curSlice->mb_pred[curPlane], curMb->pix_x, 0);

    if ((picture->chroma_format_idc != YUV400) && (picture->chroma_format_idc != YUV444))
    {
      copy_Image(&picture->imgUV[0][curMb->pix_c_y], curSlice->mb_pred[1], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
      copy_Image(&picture->imgUV[1][curMb->pix_c_y], curSlice->mb_pred[2], curMb->pix_c_x, 0, vidParam->mb_size[1][0], vidParam->mb_size[1][1]);
    }
  }
  else
  {
    iTransform(curMb, curPlane, 0);
    curSlice->is_reset_coeff = FALSE;
  }

  return 1;
}
//}}}
//{{{
int mb_pred_b_inter8x8 (sMacroblock* curMb, sColorPlane curPlane, sPicture* picture)
{
  char l0_rFrame = -1, l1_rFrame = -1;
  sMotionVector pmvl0 = zero_mv, pmvl1 = zero_mv;
  int block_size_x, block_size_y;
  int k;
  int block8x8;   // needed for ABT
  sSlice* curSlice = curMb->p_Slice;
  sVidParam* vidParam = curMb->vidParam;

  int list_offset = curMb->list_offset;
  sPicture** list0 = curSlice->listX[LIST_0 + list_offset];
  sPicture** list1 = curSlice->listX[LIST_1 + list_offset];

  set_chroma_vector(curMb);

  // prepare direct modes
  if (curSlice->direct_spatial_mv_pred_flag && (!(curMb->b8mode[0] && curMb->b8mode[1] && curMb->b8mode[2] && curMb->b8mode[3])))
    prepare_direct_params(curMb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

  for (block8x8=0; block8x8<4; block8x8++)
  {
    int mv_mode  = curMb->b8mode[block8x8];
    int pred_dir = curMb->b8pdir[block8x8];

    if ( mv_mode != 0 )
    {
      int k_start = (block8x8 << 2);
      int k_inc = (mv_mode == SMB8x4) ? 2 : 1;
      int k_end = (mv_mode == SMB8x8) ? k_start + 1 : ((mv_mode == SMB4x4) ? k_start + 4 : k_start + k_inc + 1);

      block_size_x = ( mv_mode == SMB8x4 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;
      block_size_y = ( mv_mode == SMB4x8 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;

      for (k = k_start; k < k_end; k += k_inc)
      {
        int i =  (decode_block_scan[k] & 3);
        int j = ((decode_block_scan[k] >> 2) & 3);
        perform_mc(curMb, curPlane, picture, pred_dir, i, j, block_size_x, block_size_y);
      }
    }
    else
    {
      int k_start = (block8x8 << 2);
      int k_end = k_start;

      if (vidParam->active_sps->direct_8x8_inference_flag)
      {
        block_size_x = SMB_BLOCK_SIZE;
        block_size_y = SMB_BLOCK_SIZE;
        k_end ++;
      }
      else
      {
        block_size_x = BLOCK_SIZE;
        block_size_y = BLOCK_SIZE;
        k_end += BLOCK_MULTIPLE;
      }

      // Prepare mvs (needed for deblocking and mv prediction
      if (curSlice->direct_spatial_mv_pred_flag)
      {
        for (k = k_start; k < k_start + BLOCK_MULTIPLE; k ++)
        {
          int i  =  (decode_block_scan[k] & 3);
          int j  = ((decode_block_scan[k] >> 2) & 3);
          int i4  = curMb->block_x + i;
          int j4  = curMb->block_y + j;
          sPicMotionParams *mv_info = &picture->mv_info[j4][i4];

          assert (pred_dir<=2);

          //===== DIRECT PREDICTION =====
          // motion information should be already set
          if (mv_info->ref_idx[LIST_1] == -1)
          {
            pred_dir = 0;
          }
          else if (mv_info->ref_idx[LIST_0] == -1)
          {
            pred_dir = 1;
          }
          else
          {
            pred_dir = 2;
          }
        }
      }
      else
      {
        for (k = k_start; k < k_start + BLOCK_MULTIPLE; k ++)
        {

          int i =  (decode_block_scan[k] & 3);
          int j = ((decode_block_scan[k] >> 2) & 3);
          int i4   = curMb->block_x + i;
          int j4   = curMb->block_y + j;
          sPicMotionParams *mv_info = &picture->mv_info[j4][i4];

          assert (pred_dir<=2);

          // store reference picture ID determined by direct mode
          mv_info->ref_pic[LIST_0] = list0[(short)mv_info->ref_idx[LIST_0]];
          mv_info->ref_pic[LIST_1] = list1[(short)mv_info->ref_idx[LIST_1]];
        }
      }

      for (k = k_start; k < k_end; k ++)
      {
        int i =  (decode_block_scan[k] & 3);
        int j = ((decode_block_scan[k] >> 2) & 3);
        perform_mc(curMb, curPlane, picture, pred_dir, i, j, block_size_x, block_size_y);
      }
    }
  }

  iTransform(curMb, curPlane, 0);
  if (curMb->cbp != 0)
    curSlice->is_reset_coeff = FALSE;
  return 1;
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Copy IPCM coefficients to decoded picture buffer and set parameters for this MB
 *    (for IPCM CABAC and IPCM CAVLC  28/11/2003)
 *
 * \author
 *    Dong Wang <Dong.Wang@bristol.ac.uk>
** **********************************************************************
 */

int mb_pred_ipcm (sMacroblock* curMb)
{
  int i, j, k;
  sSlice* curSlice = curMb->p_Slice;
  sVidParam* vidParam = curMb->vidParam;
  sPicture* picture = curSlice->picture;

  //Copy coefficients to decoded picture buffer
  //IPCM coefficients are stored in curSlice->cof which is set in function read_IPCM_coeffs_from_NAL()

  for(i = 0; i < MB_BLOCK_SIZE; ++i)
  {
    for(j = 0;j < MB_BLOCK_SIZE ; ++j)
    {
      picture->imgY[curMb->pix_y + i][curMb->pix_x + j] = (sPixel) curSlice->cof[0][i][j];
    }
  }

  if ((picture->chroma_format_idc != YUV400) && (vidParam->separate_colour_plane_flag == 0))
  {
    for (k = 0; k < 2; ++k)
    {
      for(i = 0; i < vidParam->mb_cr_size_y; ++i)
      {
        for(j = 0;j < vidParam->mb_cr_size_x; ++j)
        {
          picture->imgUV[k][curMb->pix_c_y+i][curMb->pix_c_x + j] = (sPixel) curSlice->cof[k + 1][i][j];
        }
      }
    }
  }

  // for deblocking filter
  update_qp(curMb, 0);

  // for CAVLC: Set the nz_coeff to 16.
  // These parameters are to be used in CAVLC decoding of neighbour blocks
  memset(vidParam->nz_coeff[curMb->mbAddrX][0][0], 16, 3 * BLOCK_PIXELS * sizeof(byte));

  // for CABAC decoding of MB skip flag
  curMb->skip_flag = 0;

  //for deblocking filter CABAC
  curMb->s_cbp[0].blk = 0xFFFF;

  //For CABAC decoding of Dquant
  curSlice->last_dquant = 0;
  curSlice->is_reset_coeff = FALSE;
  curSlice->is_reset_coeff_cr = FALSE;
  return 1;
}
//}}}
