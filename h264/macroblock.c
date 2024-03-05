//{{{
/*!
 ***********************************************************************
 * \file macroblock.c
 *
 * \brief
 *     Decode a sMacroblock
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *    - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *    - Jani Lainema                    <jani.lainema@nokia.com>
 *    - Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
 *    - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *    - Detlev Marpe
 *    - Gabi Blaettermann
 *    - Ye-Kui Wang                     <wyk@ieee.org>
 *    - Lowell Winger                   <lwinger@lsil.com>
 *    - Alexis Michael Tourapis         <alexismt@ieee.org>
 ***********************************************************************
*/
//}}}
//{{{  includes
#include <math.h>

#include "global.h"
#include "elements.h"

#include "block.h"
#include "mbuffer.h"
#include "macroblock.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "image.h"
#include "mbAccess.h"
#include "biaridecod.h"
#include "transform.h"
#include "mcPrediction.h"
#include "quant.h"
#include "mbPrediction.h"
//}}}
extern void read_coeff_4x4_CAVLC (sMacroblock* currMB, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);
extern void read_coeff_4x4_CAVLC_444 (sMacroblock* currMB, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);
extern void set_intra_prediction_modes (mSlice* currSlice);
extern void setup_read_macroblock (mSlice* currSlice);
extern void set_read_CBP_and_coeffs_cabac (mSlice* currSlice);
extern void set_read_CBP_and_coeffs_cavlc (mSlice* currSlice);
extern void set_read_comp_coeff_cavlc (sMacroblock* currMB);
extern void set_read_comp_coeff_cabac (sMacroblock* currMB);
extern void update_direct_types (mSlice* currSlice);

//{{{
static void GetMotionVectorPredictorMBAFF (sMacroblock* currMB, PixelPos *block,
                                    MotionVector *pmv, short  ref_frame, sPicMotionParams **mv_info,
                                    int list, int mb_x, int mb_y, int blockshape_x, int blockshape_y) {

  int mv_a, mv_b, mv_c, pred_vec=0;
  int mvPredType, rFrameL, rFrameU, rFrameUR;
  int hv;
  sVidParam* vidParam = currMB->vidParam;
  mvPredType = MVPRED_MEDIAN;

  if (currMB->mb_field) {
    rFrameL  = block[0].available
      ? (vidParam->mb_data[block[0].mb_addr].mb_field
      ? mv_info[block[0].pos_y][block[0].pos_x].ref_idx[list]
    : mv_info[block[0].pos_y][block[0].pos_x].ref_idx[list] * 2) : -1;
    rFrameU  = block[1].available
      ? (vidParam->mb_data[block[1].mb_addr].mb_field
      ? mv_info[block[1].pos_y][block[1].pos_x].ref_idx[list]
    : mv_info[block[1].pos_y][block[1].pos_x].ref_idx[list] * 2) : -1;
    rFrameUR = block[2].available
      ? (vidParam->mb_data[block[2].mb_addr].mb_field
      ? mv_info[block[2].pos_y][block[2].pos_x].ref_idx[list]
    : mv_info[block[2].pos_y][block[2].pos_x].ref_idx[list] * 2) : -1;
    }
  else {
    rFrameL = block[0].available
      ? (vidParam->mb_data[block[0].mb_addr].mb_field
      ? mv_info[block[0].pos_y][block[0].pos_x].ref_idx[list] >>1
      : mv_info[block[0].pos_y][block[0].pos_x].ref_idx[list]) : -1;
    rFrameU  = block[1].available
      ? (vidParam->mb_data[block[1].mb_addr].mb_field
      ? mv_info[block[1].pos_y][block[1].pos_x].ref_idx[list] >>1
      : mv_info[block[1].pos_y][block[1].pos_x].ref_idx[list]) : -1;
    rFrameUR = block[2].available
      ? (vidParam->mb_data[block[2].mb_addr].mb_field
      ? mv_info[block[2].pos_y][block[2].pos_x].ref_idx[list] >>1
      : mv_info[block[2].pos_y][block[2].pos_x].ref_idx[list]) : -1;
    }

  // Prediction if only one of the neighbors uses the reference frame we are checking
  if (rFrameL == ref_frame && rFrameU != ref_frame && rFrameUR != ref_frame)
    mvPredType = MVPRED_L;
  else if (rFrameL != ref_frame && rFrameU == ref_frame && rFrameUR != ref_frame)
    mvPredType = MVPRED_U;
  else if (rFrameL != ref_frame && rFrameU != ref_frame && rFrameUR == ref_frame)
    mvPredType = MVPRED_UR;

  // Directional predictions
  if (blockshape_x == 8 && blockshape_y == 16) {
    if (mb_x == 0) {
      if (rFrameL == ref_frame)
        mvPredType = MVPRED_L;
      }
    else {
      if (rFrameUR == ref_frame)
        mvPredType = MVPRED_UR;
      }
    }
  else if (blockshape_x == 16 && blockshape_y == 8) {
    if(mb_y == 0) {
      if (rFrameU == ref_frame)
        mvPredType = MVPRED_U;
      }
    else {
      if (rFrameL == ref_frame)
        mvPredType = MVPRED_L;
      }
    }

  for (hv = 0; hv < 2; hv++) {
    if (hv == 0) {
      mv_a = block[0].available ? mv_info[block[0].pos_y][block[0].pos_x].mv[list].mv_x : 0;
      mv_b = block[1].available ? mv_info[block[1].pos_y][block[1].pos_x].mv[list].mv_x : 0;
      mv_c = block[2].available ? mv_info[block[2].pos_y][block[2].pos_x].mv[list].mv_x : 0;
      }
    else {
      if (currMB->mb_field) {
        mv_a = block[0].available  ? vidParam->mb_data[block[0].mb_addr].mb_field
          ? mv_info[block[0].pos_y][block[0].pos_x].mv[list].mv_y
        : mv_info[block[0].pos_y][block[0].pos_x].mv[list].mv_y / 2
          : 0;
        mv_b = block[1].available  ? vidParam->mb_data[block[1].mb_addr].mb_field
          ? mv_info[block[1].pos_y][block[1].pos_x].mv[list].mv_y
        : mv_info[block[1].pos_y][block[1].pos_x].mv[list].mv_y / 2
          : 0;
        mv_c = block[2].available  ? vidParam->mb_data[block[2].mb_addr].mb_field
          ? mv_info[block[2].pos_y][block[2].pos_x].mv[list].mv_y
        : mv_info[block[2].pos_y][block[2].pos_x].mv[list].mv_y / 2
          : 0;
        }
      else {
        mv_a = block[0].available  ? vidParam->mb_data[block[0].mb_addr].mb_field
          ? mv_info[block[0].pos_y][block[0].pos_x].mv[list].mv_y * 2
          : mv_info[block[0].pos_y][block[0].pos_x].mv[list].mv_y
        : 0;
        mv_b = block[1].available  ? vidParam->mb_data[block[1].mb_addr].mb_field
          ? mv_info[block[1].pos_y][block[1].pos_x].mv[list].mv_y * 2
          : mv_info[block[1].pos_y][block[1].pos_x].mv[list].mv_y
        : 0;
        mv_c = block[2].available  ? vidParam->mb_data[block[2].mb_addr].mb_field
          ? mv_info[block[2].pos_y][block[2].pos_x].mv[list].mv_y * 2
          : mv_info[block[2].pos_y][block[2].pos_x].mv[list].mv_y
        : 0;
        }
      }

    switch (mvPredType) {
      case MVPRED_MEDIAN:
        if(!(block[1].available || block[2].available))
          pred_vec = mv_a;
        else
          pred_vec = imedian(mv_a, mv_b, mv_c);
        break;
      case MVPRED_L:
        pred_vec = mv_a;
        break;
      case MVPRED_U:
        pred_vec = mv_b;
        break;
      case MVPRED_UR:
        pred_vec = mv_c;
        break;
      default:
        break;
        }

    if (hv == 0)
      pmv->mv_x = (short) pred_vec;
    else
      pmv->mv_y = (short) pred_vec;
    }
  }
//}}}
//{{{
static void GetMotionVectorPredictorNormal (sMacroblock* currMB, PixelPos *block,
                                            MotionVector *pmv, short  ref_frame, sPicMotionParams **mv_info,
                                            int list, int mb_x, int mb_y, int blockshape_x, int blockshape_y) {
  int mvPredType = MVPRED_MEDIAN;

  int rFrameL = block[0].available ? mv_info[block[0].pos_y][block[0].pos_x].ref_idx[list] : -1;
  int rFrameU = block[1].available ? mv_info[block[1].pos_y][block[1].pos_x].ref_idx[list] : -1;
  int rFrameUR = block[2].available ? mv_info[block[2].pos_y][block[2].pos_x].ref_idx[list] : -1;

  // Prediction if only one of the neighbors uses the reference frame we are checking
  if (rFrameL == ref_frame && rFrameU != ref_frame && rFrameUR != ref_frame)
    mvPredType = MVPRED_L;
  else if (rFrameL != ref_frame && rFrameU == ref_frame && rFrameUR != ref_frame)
    mvPredType = MVPRED_U;
  else if (rFrameL != ref_frame && rFrameU != ref_frame && rFrameUR == ref_frame)
    mvPredType = MVPRED_UR;

  // Directional predictions
  if (blockshape_x == 8 && blockshape_y == 16) {
    if (mb_x == 0) {
      if (rFrameL == ref_frame)
        mvPredType = MVPRED_L;
      }
    else {
      if (rFrameUR == ref_frame)
        mvPredType = MVPRED_UR;
      }
    }
  else if (blockshape_x == 16 && blockshape_y == 8) {
    if (mb_y == 0) {
      if (rFrameU == ref_frame)
        mvPredType = MVPRED_U;
      }
    else {
      if (rFrameL == ref_frame)
        mvPredType = MVPRED_L;
      }
    }

  switch (mvPredType) {
    //{{{
    case MVPRED_MEDIAN:
      if(!(block[1].available || block[2].available))
      {
        if (block[0].available)
          *pmv = mv_info[block[0].pos_y][block[0].pos_x].mv[list];
        else
          *pmv = zero_mv;
      }
      else
      {
        MotionVector *mv_a = block[0].available ? &mv_info[block[0].pos_y][block[0].pos_x].mv[list] : (MotionVector *) &zero_mv;
        MotionVector *mv_b = block[1].available ? &mv_info[block[1].pos_y][block[1].pos_x].mv[list] : (MotionVector *) &zero_mv;
        MotionVector *mv_c = block[2].available ? &mv_info[block[2].pos_y][block[2].pos_x].mv[list] : (MotionVector *) &zero_mv;

        pmv->mv_x = (short) imedian(mv_a->mv_x, mv_b->mv_x, mv_c->mv_x);
        pmv->mv_y = (short) imedian(mv_a->mv_y, mv_b->mv_y, mv_c->mv_y);
      }
      break;
    //}}}
    //{{{
    case MVPRED_L:
      if (block[0].available)
      {
        *pmv = mv_info[block[0].pos_y][block[0].pos_x].mv[list];
      }
      else
      {
        *pmv = zero_mv;
      }
      break;
    //}}}
    //{{{
    case MVPRED_U:
      if (block[1].available)
        *pmv = mv_info[block[1].pos_y][block[1].pos_x].mv[list];
      else
        *pmv = zero_mv;
      break;
    //}}}
    //{{{
    case MVPRED_UR:
      if (block[2].available)
        *pmv = mv_info[block[2].pos_y][block[2].pos_x].mv[list];
      else
        *pmv = zero_mv;
      break;
    //}}}
    //{{{
    default:
      break;
    //}}}
    }
  }
//}}}
//{{{
static void init_motion_vector_prediction (sMacroblock* currMB, int mb_aff_frame_flag) {

  if (mb_aff_frame_flag)
    currMB->GetMVPredictor = GetMotionVectorPredictorMBAFF;
  else
    currMB->GetMVPredictor = GetMotionVectorPredictorNormal;
  }
//}}}

//{{{
static int decode_one_component_i_slice (sMacroblock* currMB, sColorPlane curPlane, sPixel **currImg, sPicture* picture)
{
  //For residual DPCM
  currMB->ipmode_DPCM = NO_INTRA_PMODE;
  if(currMB->mb_type == IPCM)
    mb_pred_ipcm(currMB);
  else if (currMB->mb_type==I16MB)
    mb_pred_intra16x16(currMB, curPlane, picture);
  else if (currMB->mb_type == I4MB)
    mb_pred_intra4x4(currMB, curPlane, currImg, picture);
  else if (currMB->mb_type == I8MB)
    mb_pred_intra8x8(currMB, curPlane, currImg, picture);

  return 1;
}
//}}}
//{{{
static int decode_one_component_p_slice (sMacroblock* currMB, sColorPlane curPlane, sPixel **currImg, sPicture* picture)
{
  //For residual DPCM
  currMB->ipmode_DPCM = NO_INTRA_PMODE;
  if(currMB->mb_type == IPCM)
    mb_pred_ipcm(currMB);
  else if (currMB->mb_type==I16MB)
    mb_pred_intra16x16(currMB, curPlane, picture);
  else if (currMB->mb_type == I4MB)
    mb_pred_intra4x4(currMB, curPlane, currImg, picture);
  else if (currMB->mb_type == I8MB)
    mb_pred_intra8x8(currMB, curPlane, currImg, picture);
  else if (currMB->mb_type == PSKIP)
    mb_pred_skip(currMB, curPlane, currImg, picture);
  else if (currMB->mb_type == P16x16)
    mb_pred_p_inter16x16(currMB, curPlane, picture);
  else if (currMB->mb_type == P16x8)
    mb_pred_p_inter16x8(currMB, curPlane, picture);
  else if (currMB->mb_type == P8x16)
    mb_pred_p_inter8x16(currMB, curPlane, picture);
  else
    mb_pred_p_inter8x8(currMB, curPlane, picture);

  return 1;
}
//}}}
//{{{
static int decode_one_component_sp_slice (sMacroblock* currMB, sColorPlane curPlane, sPixel **currImg, sPicture* picture)
{
  //For residual DPCM
  currMB->ipmode_DPCM = NO_INTRA_PMODE;

  if (currMB->mb_type == IPCM)
    mb_pred_ipcm(currMB);
  else if (currMB->mb_type==I16MB)
    mb_pred_intra16x16(currMB, curPlane, picture);
  else if (currMB->mb_type == I4MB)
    mb_pred_intra4x4(currMB, curPlane, currImg, picture);
  else if (currMB->mb_type == I8MB)
    mb_pred_intra8x8(currMB, curPlane, currImg, picture);
  else if (currMB->mb_type == PSKIP)
    mb_pred_sp_skip(currMB, curPlane, picture);
  else if (currMB->mb_type == P16x16)
    mb_pred_p_inter16x16(currMB, curPlane, picture);
  else if (currMB->mb_type == P16x8)
    mb_pred_p_inter16x8(currMB, curPlane, picture);
  else if (currMB->mb_type == P8x16)
    mb_pred_p_inter8x16(currMB, curPlane, picture);
  else
    mb_pred_p_inter8x8(currMB, curPlane, picture);

  return 1;
}
//}}}
//{{{
static int decode_one_component_b_slice (sMacroblock* currMB, sColorPlane curPlane, sPixel **currImg, sPicture* picture)
{
  //For residual DPCM
  currMB->ipmode_DPCM = NO_INTRA_PMODE;

  if(currMB->mb_type == IPCM)
    mb_pred_ipcm(currMB);
  else if (currMB->mb_type==I16MB)
    mb_pred_intra16x16(currMB, curPlane, picture);
  else if (currMB->mb_type == I4MB)
    mb_pred_intra4x4(currMB, curPlane, currImg, picture);
  else if (currMB->mb_type == I8MB)
    mb_pred_intra8x8(currMB, curPlane, currImg, picture);
  else if (currMB->mb_type == P16x16)
    mb_pred_p_inter16x16(currMB, curPlane, picture);
  else if (currMB->mb_type == P16x8)
    mb_pred_p_inter16x8(currMB, curPlane, picture);
  else if (currMB->mb_type == P8x16)
    mb_pred_p_inter8x16(currMB, curPlane, picture);
  else if (currMB->mb_type == BSKIP_DIRECT)
  {
    mSlice* currSlice = currMB->p_Slice;
    if (currSlice->direct_spatial_mv_pred_flag == 0)
    {
      if (currSlice->active_sps->direct_8x8_inference_flag)
        mb_pred_b_d8x8temporal (currMB, curPlane, currImg, picture);
      else
        mb_pred_b_d4x4temporal (currMB, curPlane, currImg, picture);
    }
    else
    {
      if (currSlice->active_sps->direct_8x8_inference_flag)
        mb_pred_b_d8x8spatial (currMB, curPlane, currImg, picture);
      else
        mb_pred_b_d4x4spatial (currMB, curPlane, currImg, picture);
    }
  }
  else
    mb_pred_b_inter8x8 (currMB, curPlane, picture);

 return 1;
}
//}}}

//{{{
static int BType2CtxRef (int btype)
{
  return (btype >= 4);
}
//}}}
//{{{
static char readRefPictureIdx_VLC (sMacroblock* currMB, SyntaxElement* currSE, sDataPartition *dP, char b8mode, int list)
{
  currSE->context = BType2CtxRef (b8mode);
  currSE->value2 = list;
  dP->readSyntaxElement (currMB, currSE, dP);
  return (char) currSE->value1;
}
//}}}
//{{{
static char readRefPictureIdx_FLC (sMacroblock* currMB, SyntaxElement* currSE, sDataPartition *dP, char b8mode, int list)
{
  currSE->context = BType2CtxRef (b8mode);
  currSE->len = 1;
  readSyntaxElement_FLC(currSE, dP->bitstream);
  currSE->value1 = 1 - currSE->value1;

  return (char) currSE->value1;
}
//}}}
//{{{
static char readRefPictureIdx_Null (sMacroblock* currMB, SyntaxElement* currSE, sDataPartition *dP, char b8mode, int list)
{
  return 0;
}
//}}}
//{{{
static void prepareListforRefIdx (sMacroblock* currMB, SyntaxElement* currSE, sDataPartition *dP, int num_ref_idx_active, int refidx_present)
{
  if(num_ref_idx_active > 1)
  {
    if (currMB->vidParam->active_pps->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag)
    {
      currSE->mapping = linfo_ue;
      if (refidx_present)
        currMB->readRefPictureIdx = (num_ref_idx_active == 2) ? readRefPictureIdx_FLC : readRefPictureIdx_VLC;
      else
        currMB->readRefPictureIdx = readRefPictureIdx_Null;
    }
    else
    {
      currSE->reading = readRefFrame_CABAC;
      currMB->readRefPictureIdx = (refidx_present) ? readRefPictureIdx_VLC : readRefPictureIdx_Null;
    }
  }
  else
    currMB->readRefPictureIdx = readRefPictureIdx_Null;
}
//}}}

//{{{
void set_chroma_qp (sMacroblock* currMB)
{
  sVidParam* vidParam = currMB->vidParam;
  sPicture* picture = currMB->p_Slice->picture;
  int i;

  for (i=0; i<2; ++i)
  {
    currMB->qpc[i] = iClip3 ( -vidParam->bitdepth_chroma_qp_scale, 51, currMB->qp + picture->chroma_qp_offset[i] );
    currMB->qpc[i] = currMB->qpc[i] < 0 ? currMB->qpc[i] : QP_SCALE_CR[currMB->qpc[i]];
    currMB->qp_scaled[i + 1] = currMB->qpc[i] + vidParam->bitdepth_chroma_qp_scale;
  }
}
//}}}
//{{{
void update_qp (sMacroblock* currMB, int qp)
{
  sVidParam* vidParam = currMB->vidParam;
  currMB->qp = qp;
  currMB->qp_scaled[0] = qp + vidParam->bitdepth_luma_qp_scale;
  set_chroma_qp(currMB);
  currMB->is_lossless = (Boolean) ((currMB->qp_scaled[0] == 0) && (vidParam->lossless_qpprime_flag == 1));
  set_read_comp_coeff_cavlc(currMB);
  set_read_comp_coeff_cabac(currMB);
}
//}}}
//{{{
void read_delta_quant (SyntaxElement* currSE, sDataPartition *dP, sMacroblock* currMB, const byte *partMap, int type)
{
  mSlice* currSlice = currMB->p_Slice;
  sVidParam* vidParam = currMB->vidParam;

  currSE->type = type;

  dP = &(currSlice->partArr[partMap[currSE->type]]);

  if (vidParam->active_pps->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag)
    currSE->mapping = linfo_se;
  else
    currSE->reading= read_dQuant_CABAC;

  dP->readSyntaxElement(currMB, currSE, dP);
  currMB->delta_quant = (short) currSE->value1;
  if ((currMB->delta_quant < -(26 + vidParam->bitdepth_luma_qp_scale/2)) || (currMB->delta_quant > (25 + vidParam->bitdepth_luma_qp_scale/2)))
  {
      printf("mb_qp_delta is out of range (%d)\n", currMB->delta_quant);
    currMB->delta_quant = iClip3(-(26 + vidParam->bitdepth_luma_qp_scale/2), (25 + vidParam->bitdepth_luma_qp_scale/2), currMB->delta_quant);

    //error ("mb_qp_delta is out of range", 500);
  }

  currSlice->qp = ((currSlice->qp + currMB->delta_quant + 52 + 2*vidParam->bitdepth_luma_qp_scale)%(52+vidParam->bitdepth_luma_qp_scale)) - vidParam->bitdepth_luma_qp_scale;
  update_qp(currMB, currSlice->qp);
}
//}}}

//{{{
static void readMBRefPictureIdx (SyntaxElement* currSE, sDataPartition *dP, sMacroblock* currMB, sPicMotionParams **mv_info, int list, int step_v0, int step_h0)
{
  if (currMB->mb_type == 1)
  {
    if ((currMB->b8pdir[0] == list || currMB->b8pdir[0] == BI_PRED))
    {
      int j, i;
      char refframe;


      currMB->subblock_x = 0;
      currMB->subblock_y = 0;
      refframe = currMB->readRefPictureIdx(currMB, currSE, dP, 1, list);
      for (j = 0; j <  step_v0; ++j)
      {
        char *ref_idx = &mv_info[j][currMB->block_x].ref_idx[list];
        // for (i = currMB->block_x; i < currMB->block_x + step_h0; ++i)
        for (i = 0; i < step_h0; ++i)
        {
          //mv_info[j][i].ref_idx[list] = refframe;
          *ref_idx = refframe;
          ref_idx += sizeof(sPicMotionParams);
        }
      }
    }
  }
  else if (currMB->mb_type == 2)
  {
    int k, j, i, j0;
    char refframe;

    for (j0 = 0; j0 < 4; j0 += step_v0)
    {
      k = j0;

      if ((currMB->b8pdir[k] == list || currMB->b8pdir[k] == BI_PRED))
      {
        currMB->subblock_y = j0 << 2;
        currMB->subblock_x = 0;
        refframe = currMB->readRefPictureIdx(currMB, currSE, dP, currMB->b8mode[k], list);
        for (j = j0; j < j0 + step_v0; ++j)
        {
          char *ref_idx = &mv_info[j][currMB->block_x].ref_idx[list];
          // for (i = currMB->block_x; i < currMB->block_x + step_h0; ++i)
          for (i = 0; i < step_h0; ++i)
          {
            //mv_info[j][i].ref_idx[list] = refframe;
            *ref_idx = refframe;
            ref_idx += sizeof(sPicMotionParams);
          }
        }
      }
    }
  }
  else if (currMB->mb_type == 3)
  {
    int k, j, i, i0;
    char refframe;

    currMB->subblock_y = 0;
    for (i0 = 0; i0 < 4; i0 += step_h0)
    {
      k = (i0 >> 1);

      if ((currMB->b8pdir[k] == list || currMB->b8pdir[k] == BI_PRED) && currMB->b8mode[k] != 0)
      {
        currMB->subblock_x = i0 << 2;
        refframe = currMB->readRefPictureIdx(currMB, currSE, dP, currMB->b8mode[k], list);
        for (j = 0; j < step_v0; ++j)
        {
          char *ref_idx = &mv_info[j][currMB->block_x + i0].ref_idx[list];
          // for (i = currMB->block_x; i < currMB->block_x + step_h0; ++i)
          for (i = 0; i < step_h0; ++i)
          {
            //mv_info[j][i].ref_idx[list] = refframe;
            *ref_idx = refframe;
            ref_idx += sizeof(sPicMotionParams);
          }
        }
      }
    }
  }
  else
  {
    int k, j, i, j0, i0;
    char refframe;

    for (j0 = 0; j0 < 4; j0 += step_v0)
    {
      currMB->subblock_y = j0 << 2;
      for (i0 = 0; i0 < 4; i0 += step_h0)
      {
        k = 2 * (j0 >> 1) + (i0 >> 1);

        if ((currMB->b8pdir[k] == list || currMB->b8pdir[k] == BI_PRED) && currMB->b8mode[k] != 0)
        {
          currMB->subblock_x = i0 << 2;
          refframe = currMB->readRefPictureIdx(currMB, currSE, dP, currMB->b8mode[k], list);
          for (j = j0; j < j0 + step_v0; ++j)
          {
            char *ref_idx = &mv_info[j][currMB->block_x + i0].ref_idx[list];
            //sPicMotionParams *mvinfo = mv_info[j] + currMB->block_x + i0;
            for (i = 0; i < step_h0; ++i)
            {
              //(mvinfo++)->ref_idx[list] = refframe;
              *ref_idx = refframe;
              ref_idx += sizeof(sPicMotionParams);
            }
          }
        }
      }
    }
  }
}
//}}}
//{{{
static void readMBMotionVectors (SyntaxElement* currSE, sDataPartition *dP, sMacroblock* currMB, int list, int step_h0, int step_v0)
{
  if (currMB->mb_type == 1)
  {
    if ((currMB->b8pdir[0] == list || currMB->b8pdir[0]== BI_PRED))//has forward vector
    {
      int i4, j4, ii, jj;
      short curr_mvd[2];
      MotionVector pred_mv, curr_mv;
      short (*mvd)[4][2];
      //sVidParam* vidParam = currMB->vidParam;
      sPicMotionParams **mv_info = currMB->p_Slice->picture->mv_info;
      PixelPos block[4]; // neighbor blocks

      currMB->subblock_x = 0; // position used for context determination
      currMB->subblock_y = 0; // position used for context determination
      i4  = currMB->block_x;
      j4  = currMB->block_y;
      mvd = &currMB->mvd [list][0];

      get_neighbors(currMB, block, 0, 0, step_h0 << 2);

      // first get MV predictor
      currMB->GetMVPredictor (currMB, block, &pred_mv, mv_info[j4][i4].ref_idx[list], mv_info, list, 0, 0, step_h0 << 2, step_v0 << 2);

      // X component
      currSE->value2 = list; // identifies the component; only used for context determination
      dP->readSyntaxElement(currMB, currSE, dP);
      curr_mvd[0] = (short) currSE->value1;

      // Y component
      currSE->value2 += 2; // identifies the component; only used for context determination
      dP->readSyntaxElement(currMB, currSE, dP);
      curr_mvd[1] = (short) currSE->value1;

      curr_mv.mv_x = (short)(curr_mvd[0] + pred_mv.mv_x);  // compute motion vector x
      curr_mv.mv_y = (short)(curr_mvd[1] + pred_mv.mv_y);  // compute motion vector y

      for(jj = j4; jj < j4 + step_v0; ++jj)
      {
        sPicMotionParams *mvinfo = mv_info[jj] + i4;
        for(ii = i4; ii < i4 + step_h0; ++ii)
        {
          (mvinfo++)->mv[list] = curr_mv;
        }
      }

      // Init first line (mvd)
      for(ii = 0; ii < step_h0; ++ii)
      {
        //*((int *) &mvd[0][ii][0]) = *((int *) curr_mvd);
        mvd[0][ii][0] = curr_mvd[0];
        mvd[0][ii][1] = curr_mvd[1];
      }

      // now copy all other lines
      for(jj = 1; jj < step_v0; ++jj)
      {
        memcpy(mvd[jj][0], mvd[0][0],  2 * step_h0 * sizeof(short));
      }
    }
  }
  else
  {
    int i4, j4, ii, jj;
    short curr_mvd[2];
    MotionVector pred_mv, curr_mv;
    short (*mvd)[4][2];
    //sVidParam* vidParam = currMB->vidParam;
    sPicMotionParams **mv_info = currMB->p_Slice->picture->mv_info;
    PixelPos block[4]; // neighbor blocks

    int i, j, i0, j0, kk, k;
    for (j0=0; j0<4; j0+=step_v0)
    {
      for (i0=0; i0<4; i0+=step_h0)
      {
        kk = 2 * (j0 >> 1) + (i0 >> 1);

        if ((currMB->b8pdir[kk] == list || currMB->b8pdir[kk]== BI_PRED) && (currMB->b8mode[kk] != 0))//has forward vector
        {
          char cur_ref_idx = mv_info[currMB->block_y+j0][currMB->block_x+i0].ref_idx[list];
          int mv_mode  = currMB->b8mode[kk];
          int step_h = BLOCK_STEP [mv_mode][0];
          int step_v = BLOCK_STEP [mv_mode][1];
          int step_h4 = step_h << 2;
          int step_v4 = step_v << 2;

          for (j = j0; j < j0 + step_v0; j += step_v)
          {
            currMB->subblock_y = j << 2; // position used for context determination
            j4  = currMB->block_y + j;
            mvd = &currMB->mvd [list][j];

            for (i = i0; i < i0 + step_h0; i += step_h)
            {
              currMB->subblock_x = i << 2; // position used for context determination
              i4 = currMB->block_x + i;

              get_neighbors(currMB, block, BLOCK_SIZE * i, BLOCK_SIZE * j, step_h4);

              // first get MV predictor
              currMB->GetMVPredictor (currMB, block, &pred_mv, cur_ref_idx, mv_info, list, BLOCK_SIZE * i, BLOCK_SIZE * j, step_h4, step_v4);

              for (k=0; k < 2; ++k)
              {
                currSE->value2   = (k << 1) + list; // identifies the component; only used for context determination
                dP->readSyntaxElement(currMB, currSE, dP);
                curr_mvd[k] = (short) currSE->value1;
              }

              curr_mv.mv_x = (short)(curr_mvd[0] + pred_mv.mv_x);  // compute motion vector
              curr_mv.mv_y = (short)(curr_mvd[1] + pred_mv.mv_y);  // compute motion vector

              for(jj = j4; jj < j4 + step_v; ++jj)
              {
                sPicMotionParams *mvinfo = mv_info[jj] + i4;
                for(ii = i4; ii < i4 + step_h; ++ii)
                {
                  (mvinfo++)->mv[list] = curr_mv;
                }
              }

              // Init first line (mvd)
              for(ii = i; ii < i + step_h; ++ii)
              {
                //*((int *) &mvd[0][ii][0]) = *((int *) curr_mvd);
                mvd[0][ii][0] = curr_mvd[0];
                mvd[0][ii][1] = curr_mvd[1];
              }

              // now copy all other lines
              for(jj = 1; jj < step_v; ++jj)
              {
                memcpy(&mvd[jj][i][0], &mvd[0][i][0],  2 * step_h * sizeof(short));
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
void invScaleCoeff (sMacroblock* currMB, int level, int run, int qp_per, int i, int j, int i0, int j0, int coef_ctr, const byte (*pos_scan4x4)[2], int (*InvLevelScale4x4)[4])
{
  if (level != 0)    /* leave if level == 0 */
  {
    coef_ctr += run + 1;

    i0 = pos_scan4x4[coef_ctr][0];
    j0 = pos_scan4x4[coef_ctr][1];

    currMB->s_cbp[0].blk |= i64_power2((j << 2) + i) ;
    currMB->p_Slice->cof[0][(j<<2) + j0][(i<<2) + i0]= rshift_rnd_sf((level * InvLevelScale4x4[j0][i0]) << qp_per, 4);
  }
}
//}}}
//{{{
static void setup_mb_pos_info (sMacroblock* currMB)
{
  int mb_x = currMB->mb.x;
  int mb_y = currMB->mb.y;
  currMB->block_x     = mb_x << BLOCK_SHIFT;           /* horizontal block position */
  currMB->block_y     = mb_y << BLOCK_SHIFT;           /* vertical block position */
  currMB->block_y_aff = currMB->block_y;                       /* interlace relative vertical position */
  currMB->pix_x       = mb_x << MB_BLOCK_SHIFT;        /* horizontal luma pixel position */
  currMB->pix_y       = mb_y << MB_BLOCK_SHIFT;        /* vertical luma pixel position */
  currMB->pix_c_x     = mb_x * currMB->vidParam->mb_cr_size_x;    /* horizontal chroma pixel position */
  currMB->pix_c_y     = mb_y * currMB->vidParam->mb_cr_size_y;    /* vertical chroma pixel position */
}
//}}}

//{{{
void start_macroblock (mSlice* currSlice, sMacroblock **currMB)
{
  sVidParam* vidParam = currSlice->vidParam;
  int mb_nr = currSlice->current_mb_nr;

  *currMB = &currSlice->mb_data[mb_nr];

  (*currMB)->p_Slice = currSlice;
  (*currMB)->vidParam   = vidParam;
  (*currMB)->mbAddrX = mb_nr;

  //assert (mb_nr < (int) vidParam->PicSizeInMbs);

  /* Update coordinates of the current macroblock */
  if (currSlice->mb_aff_frame_flag)
  {
    (*currMB)->mb.x = (short) (   (mb_nr) % ((2*vidParam->width) / MB_BLOCK_SIZE));
    (*currMB)->mb.y = (short) (2*((mb_nr) / ((2*vidParam->width) / MB_BLOCK_SIZE)));

    (*currMB)->mb.y += ((*currMB)->mb.x & 0x01);
    (*currMB)->mb.x >>= 1;
  }
  else
  {
    (*currMB)->mb = vidParam->PicPos[mb_nr];
  }

  /* Define pixel/block positions */
  setup_mb_pos_info(*currMB);

  // reset intra mode
  (*currMB)->is_intra_block = FALSE;
  // reset mode info
  (*currMB)->mb_type         = 0;
  (*currMB)->delta_quant     = 0;
  (*currMB)->cbp             = 0;
  (*currMB)->c_ipred_mode    = DC_PRED_8;

  // Save the slice number of this macroblock. When the macroblock below
  // is coded it will use this to decide if prediction for above is possible
  (*currMB)->slice_nr = (short) currSlice->current_slice_nr;

  CheckAvailabilityOfNeighbors(*currMB);

  // Select appropriate MV predictor function
  init_motion_vector_prediction(*currMB, currSlice->mb_aff_frame_flag);

  set_read_and_store_CBP(currMB, currSlice->active_sps->chroma_format_idc);

  // Reset syntax element entries in MB struct

  if (currSlice->slice_type != I_SLICE)
  {
    if (currSlice->slice_type != B_SLICE)
      memset((*currMB)->mvd[0][0][0], 0, MB_BLOCK_PARTITIONS * 2 * sizeof(short));
    else
      memset((*currMB)->mvd[0][0][0], 0, 2 * MB_BLOCK_PARTITIONS * 2 * sizeof(short));
  }

  memset((*currMB)->s_cbp, 0, 3 * sizeof(CBPStructure));

  // initialize currSlice->mb_rres
  if (currSlice->is_reset_coeff == FALSE)
  {
    memset (currSlice->mb_rres[0][0], 0, MB_PIXELS * sizeof(int));
    memset (currSlice->mb_rres[1][0], 0, vidParam->mb_cr_size * sizeof(int));
    memset (currSlice->mb_rres[2][0], 0, vidParam->mb_cr_size * sizeof(int));
    if (currSlice->is_reset_coeff_cr == FALSE)
    {
      memset (currSlice->cof[0][0], 0, 3 * MB_PIXELS * sizeof(int));
      currSlice->is_reset_coeff_cr = TRUE;
    }
    else
    {
      memset (currSlice->cof[0][0], 0, MB_PIXELS * sizeof(int));
    }
    currSlice->is_reset_coeff = TRUE;
  }

  // store filtering parameters for this MB
  (*currMB)->DFDisableIdc    = currSlice->DFDisableIdc;
  (*currMB)->DFAlphaC0Offset = currSlice->DFAlphaC0Offset;
  (*currMB)->DFBetaOffset    = currSlice->DFBetaOffset;
  (*currMB)->list_offset     = 0;
  (*currMB)->mixedModeEdgeFlag = 0;
}
//}}}
//{{{
Boolean exit_macroblock (mSlice* currSlice, int eos_bit)
{
  sVidParam* vidParam = currSlice->vidParam;

 //! The if() statement below resembles the original code, which tested
  //! vidParam->current_mb_nr == vidParam->PicSizeInMbs.  Both is, of course, nonsense
  //! In an error prone environment, one can only be sure to have a new
  //! picture by checking the tr of the next slice header!

// printf ("exit_macroblock: FmoGetLastMBOfPicture %d, vidParam->current_mb_nr %d\n", FmoGetLastMBOfPicture(), vidParam->current_mb_nr);
  ++(currSlice->num_dec_mb);

  if(currSlice->current_mb_nr == vidParam->PicSizeInMbs - 1) //if (vidParam->num_dec_mb == vidParam->PicSizeInMbs)
    return TRUE;
  // ask for last mb in the slice  CAVLC
  else
  {

    currSlice->current_mb_nr = FmoGetNextMBNr (vidParam, currSlice->current_mb_nr);

    if (currSlice->current_mb_nr == -1)     // End of mSlice group, MUST be end of slice
    {
      assert (currSlice->nal_startcode_follows (currSlice, eos_bit) == TRUE);
      return TRUE;
    }

    if(currSlice->nal_startcode_follows(currSlice, eos_bit) == FALSE)
      return FALSE;

    if(currSlice->slice_type == I_SLICE  || currSlice->slice_type == SI_SLICE || vidParam->active_pps->entropy_coding_mode_flag == (Boolean) CABAC)
      return TRUE;
    if(currSlice->cod_counter <= 0)
      return TRUE;
    return FALSE;
  }
}
//}}}

//{{{
static void interpret_mb_mode_P (sMacroblock* currMB)
{
  static const short ICBPTAB[6] = {0,16,32,15,31,47};
  short  mbmode = currMB->mb_type;

  if(mbmode < 4)
  {
    currMB->mb_type = mbmode;
    memset(currMB->b8mode, mbmode, 4 * sizeof(char));
    memset(currMB->b8pdir, 0, 4 * sizeof(char));
  }
  else if((mbmode == 4 || mbmode == 5))
  {
    currMB->mb_type = P8x8;
    currMB->p_Slice->allrefzero = (mbmode == 5);
  }
  else if(mbmode == 6)
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type = I4MB;
    memset(currMB->b8mode, IBLOCK, 4 * sizeof(char));
    memset(currMB->b8pdir,     -1, 4 * sizeof(char));
  }
  else if(mbmode == 31)
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type = IPCM;
    currMB->cbp = -1;
    currMB->i16mode = 0;

    memset(currMB->b8mode, 0, 4 * sizeof(char));
    memset(currMB->b8pdir,-1, 4 * sizeof(char));
  }
  else
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type = I16MB;
    currMB->cbp = ICBPTAB[((mbmode-7))>>2];
    currMB->i16mode = ((mbmode-7)) & 0x03;
    memset(currMB->b8mode, 0, 4 * sizeof(char));
    memset(currMB->b8pdir,-1, 4 * sizeof(char));
  }
}
//}}}
//{{{
static void interpret_mb_mode_I (sMacroblock* currMB)
{
  static const short ICBPTAB[6] = {0,16,32,15,31,47};
  short mbmode   = currMB->mb_type;

  if (mbmode == 0)
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type = I4MB;
    memset(currMB->b8mode,IBLOCK,4 * sizeof(char));
    memset(currMB->b8pdir,-1,4 * sizeof(char));
  }
  else if(mbmode == 25)
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type=IPCM;
    currMB->cbp= -1;
    currMB->i16mode = 0;

    memset(currMB->b8mode, 0,4 * sizeof(char));
    memset(currMB->b8pdir,-1,4 * sizeof(char));
  }
  else
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type = I16MB;
    currMB->cbp= ICBPTAB[(mbmode-1)>>2];
    currMB->i16mode = (mbmode-1) & 0x03;
    memset(currMB->b8mode, 0, 4 * sizeof(char));
    memset(currMB->b8pdir,-1, 4 * sizeof(char));
  }
}
//}}}
//{{{
static void interpret_mb_mode_B (sMacroblock* currMB)
{
  static const char offset2pdir16x16[12]   = {0, 0, 1, 2, 0,0,0,0,0,0,0,0};
  static const char offset2pdir16x8[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},{1,0},
                                             {0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2},{0,0}};
  static const char offset2pdir8x16[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},
                                             {1,0},{0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2}};

  static const char ICBPTAB[6] = {0,16,32,15,31,47};

  short i, mbmode;
  short mbtype  = currMB->mb_type;

  //--- set mbtype, b8type, and b8pdir ---
  if (mbtype == 0)       // direct
  {
    mbmode=0;
    memset(currMB->b8mode, 0, 4 * sizeof(char));
    memset(currMB->b8pdir, 2, 4 * sizeof(char));
  }
  else if (mbtype == 23) // intra4x4
  {
    currMB->is_intra_block = TRUE;
    mbmode=I4MB;
    memset(currMB->b8mode, IBLOCK,4 * sizeof(char));
    memset(currMB->b8pdir, -1,4 * sizeof(char));
  }
  else if ((mbtype > 23) && (mbtype < 48) ) // intra16x16
  {
    currMB->is_intra_block = TRUE;
    mbmode=I16MB;
    memset(currMB->b8mode,  0, 4 * sizeof(char));
    memset(currMB->b8pdir, -1, 4 * sizeof(char));

    currMB->cbp     = (int) ICBPTAB[(mbtype-24)>>2];
    currMB->i16mode = (mbtype-24) & 0x03;
  }
  else if (mbtype == 22) // 8x8(+split)
  {
    mbmode=P8x8;       // b8mode and pdir is transmitted in additional codewords
  }
  else if (mbtype < 4)   // 16x16
  {
    mbmode = 1;
    memset(currMB->b8mode, 1,4 * sizeof(char));
    memset(currMB->b8pdir, offset2pdir16x16[mbtype], 4 * sizeof(char));
  }
  else if(mbtype == 48)
  {
    currMB->is_intra_block = TRUE;
    mbmode=IPCM;
    memset(currMB->b8mode, 0,4 * sizeof(char));
    memset(currMB->b8pdir,-1,4 * sizeof(char));

    currMB->cbp= -1;
    currMB->i16mode = 0;
  }
  else if ((mbtype & 0x01) == 0) // 16x8
  {
    mbmode = 2;
    memset(currMB->b8mode, 2,4 * sizeof(char));
    for(i=0;i<4;++i)
    {
      currMB->b8pdir[i] = offset2pdir16x8 [mbtype][i>>1];
    }
  }
  else
  {
    mbmode=3;
    memset(currMB->b8mode, 3,4 * sizeof(char));
    for(i=0;i<4; ++i)
    {
      currMB->b8pdir[i] = offset2pdir8x16 [mbtype][i&0x01];
    }
  }
  currMB->mb_type = mbmode;
}
//}}}
//{{{
static void interpret_mb_mode_SI (sMacroblock* currMB)
{
  //sVidParam* vidParam = currMB->vidParam;
  const int ICBPTAB[6] = {0,16,32,15,31,47};
  short mbmode   = currMB->mb_type;

  if (mbmode == 0)
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type = SI4MB;
    memset(currMB->b8mode,IBLOCK,4 * sizeof(char));
    memset(currMB->b8pdir,-1,4 * sizeof(char));
    currMB->p_Slice->siblock[currMB->mb.y][currMB->mb.x]=1;
  }
  else if (mbmode == 1)
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type = I4MB;
    memset(currMB->b8mode,IBLOCK,4 * sizeof(char));
    memset(currMB->b8pdir,-1,4 * sizeof(char));
  }
  else if(mbmode == 26)
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type=IPCM;
    currMB->cbp= -1;
    currMB->i16mode = 0;
    memset(currMB->b8mode,0,4 * sizeof(char));
    memset(currMB->b8pdir,-1,4 * sizeof(char));
  }

  else
  {
    currMB->is_intra_block = TRUE;
    currMB->mb_type = I16MB;
    currMB->cbp= ICBPTAB[(mbmode-2)>>2];
    currMB->i16mode = (mbmode-2) & 0x03;
    memset(currMB->b8mode,0,4 * sizeof(char));
    memset(currMB->b8pdir,-1,4 * sizeof(char));
  }
}
//}}}
//{{{
static void read_motion_info_from_NAL_p_slice (sMacroblock* currMB)
{
  sVidParam* vidParam = currMB->vidParam;
  mSlice* currSlice = currMB->p_Slice;

  SyntaxElement currSE;
  sDataPartition *dP = NULL;
  const byte *partMap       = assignSE2partition[currSlice->dp_mode];
  short partmode        = ((currMB->mb_type == P8x8) ? 4 : currMB->mb_type);
  int step_h0         = BLOCK_STEP [partmode][0];
  int step_v0         = BLOCK_STEP [partmode][1];

  int j4;
  sPicture* picture = currSlice->picture;
  sPicMotionParams *mv_info = NULL;

  int list_offset = currMB->list_offset;
  sPicture **list0 = currSlice->listX[LIST_0 + list_offset];
  sPicMotionParams **p_mv_info = &picture->mv_info[currMB->block_y];

  //=====  READ REFERENCE PICTURE INDICES =====
  currSE.type = SE_REFFRAME;
  dP = &(currSlice->partArr[partMap[SE_REFFRAME]]);

  //  For LIST_0, if multiple ref. pictures, read LIST_0 reference picture indices for the MB ***********
  prepareListforRefIdx (currMB, &currSE, dP, currSlice->num_ref_idx_active[LIST_0], (currMB->mb_type != P8x8) || (!currSlice->allrefzero));
  readMBRefPictureIdx  (&currSE, dP, currMB, p_mv_info, LIST_0, step_v0, step_h0);

  //=====  READ MOTION VECTORS =====
  currSE.type = SE_MVD;
  dP = &(currSlice->partArr[partMap[SE_MVD]]);

  if (vidParam->active_pps->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag)
    currSE.mapping = linfo_se;
  else
    currSE.reading = currSlice->mb_aff_frame_flag ? read_mvd_CABAC_mbaff : read_MVD_CABAC;

  // LIST_0 Motion vectors
  readMBMotionVectors (&currSE, dP, currMB, LIST_0, step_h0, step_v0);

  // record reference picture Ids for deblocking decisions
  for(j4 = 0; j4 < 4;++j4)
  {
    mv_info = &p_mv_info[j4][currMB->block_x];
    mv_info->ref_pic[LIST_0] = list0[(short) mv_info->ref_idx[LIST_0]];
    mv_info++;
    mv_info->ref_pic[LIST_0] = list0[(short) mv_info->ref_idx[LIST_0]];
    mv_info++;
    mv_info->ref_pic[LIST_0] = list0[(short) mv_info->ref_idx[LIST_0]];
    mv_info++;
    mv_info->ref_pic[LIST_0] = list0[(short) mv_info->ref_idx[LIST_0]];
  }
}
//}}}
//{{{
static void read_motion_info_from_NAL_b_slice (sMacroblock* currMB) {

  mSlice* currSlice = currMB->p_Slice;
  sVidParam* vidParam = currMB->vidParam;
  sPicture* picture = currSlice->picture;
  SyntaxElement currSE;
  sDataPartition* dP = NULL;
  const byte* partMap = assignSE2partition[currSlice->dp_mode];
  int partmode = ((currMB->mb_type == P8x8) ? 4 : currMB->mb_type);
  int step_h0 = BLOCK_STEP [partmode][0];
  int step_v0 = BLOCK_STEP [partmode][1];
  int j4, i4;

  int list_offset = currMB->list_offset;
  sPicture** list0 = currSlice->listX[LIST_0 + list_offset];
  sPicture** list1 = currSlice->listX[LIST_1 + list_offset];
  sPicMotionParams** p_mv_info = &picture->mv_info[currMB->block_y];

  if (currMB->mb_type == P8x8)
    currSlice->update_direct_mv_info(currMB);

  //=====  READ REFERENCE PICTURE INDICES =====
  currSE.type = SE_REFFRAME;
  dP = &(currSlice->partArr[partMap[SE_REFFRAME]]);

  //  For LIST_0, if multiple ref. pictures, read LIST_0 reference picture indices for the MB ***********
  prepareListforRefIdx (currMB, &currSE, dP, currSlice->num_ref_idx_active[LIST_0], TRUE);
  readMBRefPictureIdx (&currSE, dP, currMB, p_mv_info, LIST_0, step_v0, step_h0);

  //  For LIST_1, if multiple ref. pictures, read LIST_1 reference picture indices for the MB ***********
  prepareListforRefIdx (currMB, &currSE, dP, currSlice->num_ref_idx_active[LIST_1], TRUE);
  readMBRefPictureIdx (&currSE, dP, currMB, p_mv_info, LIST_1, step_v0, step_h0);

  //=====  READ MOTION VECTORS =====
  currSE.type = SE_MVD;
  dP = &(currSlice->partArr[partMap[SE_MVD]]);

  if (vidParam->active_pps->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag)
    currSE.mapping = linfo_se;
  else
    currSE.reading = currSlice->mb_aff_frame_flag ? read_mvd_CABAC_mbaff : read_MVD_CABAC;

  // LIST_0 Motion vectors
  readMBMotionVectors (&currSE, dP, currMB, LIST_0, step_h0, step_v0);

  // LIST_1 Motion vectors
  readMBMotionVectors (&currSE, dP, currMB, LIST_1, step_h0, step_v0);

  // record reference picture Ids for deblocking decisions
  for (j4 = 0; j4 < 4; ++j4) {
    for (i4 = currMB->block_x; i4 < (currMB->block_x + 4); ++i4) {
      sPicMotionParams *mv_info = &p_mv_info[j4][i4];
      short ref_idx = mv_info->ref_idx[LIST_0];

      mv_info->ref_pic[LIST_0] = (ref_idx >= 0) ? list0[ref_idx] : NULL;
      ref_idx = mv_info->ref_idx[LIST_1];
      mv_info->ref_pic[LIST_1] = (ref_idx >= 0) ? list1[ref_idx] : NULL;
      }
    }
  }
//}}}
//{{{
void setup_slice_methods (mSlice* currSlice) {

  setup_read_macroblock (currSlice);

  switch (currSlice->slice_type) {
    //{{{
    case P_SLICE:
      currSlice->interpret_mb_mode         = interpret_mb_mode_P;
      currSlice->read_motion_info_from_NAL = read_motion_info_from_NAL_p_slice;
      currSlice->decode_one_component      = decode_one_component_p_slice;
      currSlice->update_direct_mv_info     = NULL;
      currSlice->init_lists                = init_lists_p_slice;
      break;
    //}}}
    //{{{
    case SP_SLICE:
      currSlice->interpret_mb_mode         = interpret_mb_mode_P;
      currSlice->read_motion_info_from_NAL = read_motion_info_from_NAL_p_slice;
      currSlice->decode_one_component      = decode_one_component_sp_slice;
      currSlice->update_direct_mv_info     = NULL;
      currSlice->init_lists                = init_lists_p_slice;
      break;
    //}}}
    //{{{
    case B_SLICE:
      currSlice->interpret_mb_mode         = interpret_mb_mode_B;
      currSlice->read_motion_info_from_NAL = read_motion_info_from_NAL_b_slice;
      currSlice->decode_one_component      = decode_one_component_b_slice;
      update_direct_types(currSlice);
      currSlice->init_lists                = init_lists_b_slice;
      break;
    //}}}
    //{{{
    case I_SLICE:
      currSlice->interpret_mb_mode         = interpret_mb_mode_I;
      currSlice->read_motion_info_from_NAL = NULL;
      currSlice->decode_one_component      = decode_one_component_i_slice;
      currSlice->update_direct_mv_info     = NULL;
      currSlice->init_lists                = init_lists_i_slice;
      break;
    //}}}
    //{{{
    case SI_SLICE:
      currSlice->interpret_mb_mode         = interpret_mb_mode_SI;
      currSlice->read_motion_info_from_NAL = NULL;
      currSlice->decode_one_component      = decode_one_component_i_slice;
      currSlice->update_direct_mv_info     = NULL;
      currSlice->init_lists                = init_lists_i_slice;
      break;
    //}}}
    //{{{
    default:
      printf("Unsupported slice type\n");
      break;
    //}}}
    }

  set_intra_prediction_modes (currSlice);

  if (currSlice->vidParam->active_sps->chroma_format_idc==YUV444 && (currSlice->vidParam->separate_colour_plane_flag == 0) )
    currSlice->read_coeff_4x4_CAVLC = read_coeff_4x4_CAVLC_444;
  else
    currSlice->read_coeff_4x4_CAVLC = read_coeff_4x4_CAVLC;
  switch (currSlice->vidParam->active_pps->entropy_coding_mode_flag) {
    case CABAC:
      set_read_CBP_and_coeffs_cabac(currSlice);
      break;
    case CAVLC:
      set_read_CBP_and_coeffs_cavlc(currSlice);
      break;
    default:
      printf("Unsupported entropy coding mode\n");
      break;
    }
  }
//}}}

//{{{
void get_neighbors (sMacroblock* currMB,       // <--  current sMacroblock
                   PixelPos   *block,     // <--> neighbor blocks
                   int         mb_x,         // <--  block x position
                   int         mb_y,         // <--  block y position
                   int         blockshape_x  // <--  block width
                   )
{
  int* mb_size = currMB->vidParam->mb_size[IS_LUMA];

  get4x4Neighbour (currMB, mb_x - 1,            mb_y    , mb_size, block    );
  get4x4Neighbour (currMB, mb_x,                mb_y - 1, mb_size, block + 1);
  get4x4Neighbour (currMB, mb_x + blockshape_x, mb_y - 1, mb_size, block + 2);

  if (mb_y > 0) {
    if (mb_x < 8) {
      // first column of 8x8 blocks
      if (mb_y == 8 ) {
        if (blockshape_x == MB_BLOCK_SIZE)
          block[2].available  = 0;
        }
      else if (mb_x + blockshape_x == 8)
        block[2].available = 0;
      }
    else if (mb_x + blockshape_x == MB_BLOCK_SIZE)
      block[2].available = 0;
    }

  if (!block[2].available) {
    get4x4Neighbour(currMB, mb_x - 1, mb_y - 1, mb_size, block + 3);
    block[2] = block[3];
    }
  }
//}}}
//{{{
void check_dp_neighbors (sMacroblock* currMB) {

  sVidParam* vidParam = currMB->vidParam;
  PixelPos up, left;

  vidParam->getNeighbour (currMB, -1,  0, vidParam->mb_size[1], &left);
  vidParam->getNeighbour (currMB,  0, -1, vidParam->mb_size[1], &up);

  if ((currMB->is_intra_block == FALSE) || (!(vidParam->active_pps->constrained_intra_pred_flag)) ) {
    if (left.available)
      currMB->dpl_flag |= vidParam->mb_data[left.mb_addr].dpl_flag;
    if (up.available)
      currMB->dpl_flag |= vidParam->mb_data[up.mb_addr].dpl_flag;
    }
  }
//}}}

//{{{
// probably a better way (or place) to do this, but I'm not sure what (where) it is [CJV]
// this is intended to make get_block_luma faster, but I'm still performing
// this at the MB level, and it really should be done at the slice level
static void init_cur_imgy (sVidParam* vidParam,mSlice* currSlice,int pl) {

  if (vidParam->separate_colour_plane_flag == 0) {
    sPicture* vidref = vidParam->no_reference_picture;
    int noref = (currSlice->framepoc < vidParam->recovery_poc);

    if (pl == PLANE_Y) {
      for (int j = 0; j < 6; j++) {
        for (int i = 0; i < currSlice->listXsize[j] ; i++) {
          sPicture* curr_ref = currSlice->listX[j][i];
          if (curr_ref) {
            curr_ref->no_ref = noref && (curr_ref == vidref);
            curr_ref->cur_imgY = curr_ref->imgY;
            }
          }
        }
      }
    else {
      for (int j = 0; j < 6; j++) {
        for (int i = 0; i < currSlice->listXsize[j]; i++) {
          sPicture* curr_ref = currSlice->listX[j][i];
          if (curr_ref) {
            curr_ref->no_ref = noref && (curr_ref == vidref);
            curr_ref->cur_imgY = curr_ref->imgUV[pl-1];
            }
          }
        }
      }
    }
  }
//}}}
//{{{
void change_plane_JV (sVidParam* vidParam, int nplane, mSlice *pSlice)
{
  vidParam->mb_data = vidParam->mb_data_JV[nplane];
  vidParam->picture  = vidParam->dec_picture_JV[nplane];
  vidParam->siblock = vidParam->siblock_JV[nplane];
  vidParam->ipredmode = vidParam->ipredmode_JV[nplane];
  vidParam->intra_block = vidParam->intra_block_JV[nplane];

  if (pSlice) {
    pSlice->mb_data = vidParam->mb_data_JV[nplane];
    pSlice->picture  = vidParam->dec_picture_JV[nplane];
    pSlice->siblock = vidParam->siblock_JV[nplane];
    pSlice->ipredmode = vidParam->ipredmode_JV[nplane];
    pSlice->intra_block = vidParam->intra_block_JV[nplane];
    }
  }
//}}}
//{{{
void make_frame_picture_JV (sVidParam* vidParam) {

  vidParam->picture = vidParam->dec_picture_JV[0];

  // copy;
  if (vidParam->picture->used_for_reference) {
    int nsize = (vidParam->picture->size_y/BLOCK_SIZE)*(vidParam->picture->size_x/BLOCK_SIZE)*sizeof(sPicMotionParams);
    memcpy (&(vidParam->picture->JVmv_info[PLANE_Y][0][0]), &(vidParam->dec_picture_JV[PLANE_Y]->mv_info[0][0]), nsize);
    memcpy (&(vidParam->picture->JVmv_info[PLANE_U][0][0]), &(vidParam->dec_picture_JV[PLANE_U]->mv_info[0][0]), nsize);
    memcpy (&(vidParam->picture->JVmv_info[PLANE_V][0][0]), &(vidParam->dec_picture_JV[PLANE_V]->mv_info[0][0]), nsize);
    }

  // This could be done with pointers and seems not necessary
  for (int uv = 0; uv < 2; uv++) {
    for (int line = 0; line < vidParam->height; line++) {
      int nsize = sizeof(sPixel) * vidParam->width;
      memcpy (vidParam->picture->imgUV[uv][line], vidParam->dec_picture_JV[uv+1]->imgY[line], nsize );
      }
    free_storable_picture (vidParam->dec_picture_JV[uv+1]);
    }
  }
//}}}

//{{{
int decode_one_macroblock (sMacroblock* currMB, sPicture* picture)
{
  mSlice* currSlice = currMB->p_Slice;
  sVidParam* vidParam = currMB->vidParam;

  if (currSlice->chroma444_not_separate) {
    if (!currMB->is_intra_block) {
      init_cur_imgy (vidParam, currSlice, PLANE_Y);
      currSlice->decode_one_component (currMB, PLANE_Y, picture->imgY, picture);
      init_cur_imgy (vidParam, currSlice, PLANE_U);
      currSlice->decode_one_component (currMB, PLANE_U, picture->imgUV[0], picture);
      init_cur_imgy (vidParam, currSlice, PLANE_V);
      currSlice->decode_one_component (currMB, PLANE_V, picture->imgUV[1], picture);
      }
    else {
      currSlice->decode_one_component (currMB, PLANE_Y, picture->imgY, picture);
      currSlice->decode_one_component (currMB, PLANE_U, picture->imgUV[0], picture);
      currSlice->decode_one_component (currMB, PLANE_V, picture->imgUV[1], picture);
      }
    currSlice->is_reset_coeff = FALSE;
    currSlice->is_reset_coeff_cr = FALSE;
    }
  else
    currSlice->decode_one_component(currMB, PLANE_Y, picture->imgY, picture);

  return 0;
  }
//}}}
