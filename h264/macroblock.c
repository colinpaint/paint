//{{{  includes
#include <math.h>

#include "global.h"
#include "elements.h"

#include "block.h"
#include "buffer.h"
#include "macroblock.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "image.h"
#include "mbAccess.h"
#include "biariDecode.h"
#include "transform.h"
#include "mcPred.h"
#include "quant.h"
#include "mbPred.h"
//}}}
extern void readCoef4x4cavlc (sMacroblock* mb, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);
extern void readCoef4x4cavlc444 (sMacroblock* mb, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);
extern void set_intra_prediction_modes (sSlice* curSlice);
extern void setup_read_macroblock (sSlice* curSlice);
extern void set_read_CBP_and_coeffs_cavlc (sSlice* curSlice);
extern void set_read_comp_coeff_cavlc (sMacroblock* mb);

//{{{
static void GetMotionVectorPredictorMBAFF (sMacroblock* mb, sPixelPos* block,
                                           sMotionVector *pmv, short  ref_frame, sPicMotionParam** mvInfo,
                                           int list, int mb_x, int mb_y, int blockshape_x, int blockshape_y) {

  int mv_a, mv_b, mv_c, pred_vec=0;
  int mvPredType, rFrameL, rFrameU, rFrameUR;
  int hv;
  sVidParam* vidParam = mb->vidParam;
  mvPredType = MVPRED_MEDIAN;

  if (mb->mbField) {
    rFrameL  = block[0].available
      ? (vidParam->mbData[block[0].mbAddr].mbField
      ? mvInfo[block[0].posY][block[0].posX].refIndex[list]
    : mvInfo[block[0].posY][block[0].posX].refIndex[list] * 2) : -1;
    rFrameU  = block[1].available
      ? (vidParam->mbData[block[1].mbAddr].mbField
      ? mvInfo[block[1].posY][block[1].posX].refIndex[list]
    : mvInfo[block[1].posY][block[1].posX].refIndex[list] * 2) : -1;
    rFrameUR = block[2].available
      ? (vidParam->mbData[block[2].mbAddr].mbField
      ? mvInfo[block[2].posY][block[2].posX].refIndex[list]
    : mvInfo[block[2].posY][block[2].posX].refIndex[list] * 2) : -1;
    }
  else {
    rFrameL = block[0].available
      ? (vidParam->mbData[block[0].mbAddr].mbField
      ? mvInfo[block[0].posY][block[0].posX].refIndex[list] >>1
      : mvInfo[block[0].posY][block[0].posX].refIndex[list]) : -1;
    rFrameU  = block[1].available
      ? (vidParam->mbData[block[1].mbAddr].mbField
      ? mvInfo[block[1].posY][block[1].posX].refIndex[list] >>1
      : mvInfo[block[1].posY][block[1].posX].refIndex[list]) : -1;
    rFrameUR = block[2].available
      ? (vidParam->mbData[block[2].mbAddr].mbField
      ? mvInfo[block[2].posY][block[2].posX].refIndex[list] >>1
      : mvInfo[block[2].posY][block[2].posX].refIndex[list]) : -1;
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
      mv_a = block[0].available ? mvInfo[block[0].posY][block[0].posX].mv[list].mvX : 0;
      mv_b = block[1].available ? mvInfo[block[1].posY][block[1].posX].mv[list].mvX : 0;
      mv_c = block[2].available ? mvInfo[block[2].posY][block[2].posX].mv[list].mvX : 0;
      }
    else {
      if (mb->mbField) {
        mv_a = block[0].available  ? vidParam->mbData[block[0].mbAddr].mbField
          ? mvInfo[block[0].posY][block[0].posX].mv[list].mvY
        : mvInfo[block[0].posY][block[0].posX].mv[list].mvY / 2
          : 0;
        mv_b = block[1].available  ? vidParam->mbData[block[1].mbAddr].mbField
          ? mvInfo[block[1].posY][block[1].posX].mv[list].mvY
        : mvInfo[block[1].posY][block[1].posX].mv[list].mvY / 2
          : 0;
        mv_c = block[2].available  ? vidParam->mbData[block[2].mbAddr].mbField
          ? mvInfo[block[2].posY][block[2].posX].mv[list].mvY
        : mvInfo[block[2].posY][block[2].posX].mv[list].mvY / 2
          : 0;
        }
      else {
        mv_a = block[0].available  ? vidParam->mbData[block[0].mbAddr].mbField
          ? mvInfo[block[0].posY][block[0].posX].mv[list].mvY * 2
          : mvInfo[block[0].posY][block[0].posX].mv[list].mvY
        : 0;
        mv_b = block[1].available  ? vidParam->mbData[block[1].mbAddr].mbField
          ? mvInfo[block[1].posY][block[1].posX].mv[list].mvY * 2
          : mvInfo[block[1].posY][block[1].posX].mv[list].mvY
        : 0;
        mv_c = block[2].available  ? vidParam->mbData[block[2].mbAddr].mbField
          ? mvInfo[block[2].posY][block[2].posX].mv[list].mvY * 2
          : mvInfo[block[2].posY][block[2].posX].mv[list].mvY
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
      pmv->mvX = (short) pred_vec;
    else
      pmv->mvY = (short) pred_vec;
    }
  }
//}}}
//{{{
static void GetMotionVectorPredictorNormal (sMacroblock* mb, sPixelPos* block,
                                            sMotionVector *pmv, short  ref_frame, sPicMotionParam** mvInfo,
                                            int list, int mb_x, int mb_y, int blockshape_x, int blockshape_y) {
  int mvPredType = MVPRED_MEDIAN;

  int rFrameL = block[0].available ? mvInfo[block[0].posY][block[0].posX].refIndex[list] : -1;
  int rFrameU = block[1].available ? mvInfo[block[1].posY][block[1].posX].refIndex[list] : -1;
  int rFrameUR = block[2].available ? mvInfo[block[2].posY][block[2].posX].refIndex[list] : -1;

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
          *pmv = mvInfo[block[0].posY][block[0].posX].mv[list];
        else
          *pmv = zero_mv;
      }
      else
      {
        sMotionVector *mv_a = block[0].available ? &mvInfo[block[0].posY][block[0].posX].mv[list] : (sMotionVector *) &zero_mv;
        sMotionVector *mv_b = block[1].available ? &mvInfo[block[1].posY][block[1].posX].mv[list] : (sMotionVector *) &zero_mv;
        sMotionVector *mv_c = block[2].available ? &mvInfo[block[2].posY][block[2].posX].mv[list] : (sMotionVector *) &zero_mv;

        pmv->mvX = (short) imedian(mv_a->mvX, mv_b->mvX, mv_c->mvX);
        pmv->mvY = (short) imedian(mv_a->mvY, mv_b->mvY, mv_c->mvY);
      }
      break;
    //}}}
    //{{{
    case MVPRED_L:
      if (block[0].available)
      {
        *pmv = mvInfo[block[0].posY][block[0].posX].mv[list];
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
        *pmv = mvInfo[block[1].posY][block[1].posX].mv[list];
      else
        *pmv = zero_mv;
      break;
    //}}}
    //{{{
    case MVPRED_UR:
      if (block[2].available)
        *pmv = mvInfo[block[2].posY][block[2].posX].mv[list];
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
static void init_motion_vector_prediction (sMacroblock* mb, int mbAffFrameFlag) {

  if (mbAffFrameFlag)
    mb->GetMVPredictor = GetMotionVectorPredictorMBAFF;
  else
    mb->GetMVPredictor = GetMotionVectorPredictorNormal;
  }
//}}}

//{{{
static int decode_one_component_i_slice (sMacroblock* mb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  //For residual DPCM
  mb->ipmode_DPCM = NO_INTRA_PMODE;
  if (mb->mbType == IPCM)
    mb_pred_ipcm (mb);
  else if (mb->mbType == I16MB)
    mb_pred_intra16x16 (mb, curPlane, picture);
  else if (mb->mbType == I4MB)
    mb_pred_intra4x4 (mb, curPlane, curPixel, picture);
  else if (mb->mbType == I8MB)
    mb_pred_intra8x8 (mb, curPlane, curPixel, picture);

  return 1;
  }
//}}}
//{{{
static int decode_one_component_p_slice (sMacroblock* mb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  //For residual DPCM
  mb->ipmode_DPCM = NO_INTRA_PMODE;
  if (mb->mbType == IPCM)
    mb_pred_ipcm (mb);
  else if (mb->mbType == I16MB)
    mb_pred_intra16x16 (mb, curPlane, picture);
  else if (mb->mbType == I4MB)
    mb_pred_intra4x4 (mb, curPlane, curPixel, picture);
  else if (mb->mbType == I8MB)
    mb_pred_intra8x8 (mb, curPlane, curPixel, picture);
  else if (mb->mbType == PSKIP)
    mb_pred_skip( mb, curPlane, curPixel, picture);
  else if (mb->mbType == P16x16)
    mb_pred_p_inter16x16 (mb, curPlane, picture);
  else if (mb->mbType == P16x8)
    mb_pred_p_inter16x8 (mb, curPlane, picture);
  else if (mb->mbType == P8x16)
    mb_pred_p_inter8x16 (mb, curPlane, picture);
  else
    mb_pred_p_inter8x8 (mb, curPlane, picture);

  return 1;
  }
//}}}
//{{{
static int decode_one_component_sp_slice (sMacroblock* mb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  //For residual DPCM
  mb->ipmode_DPCM = NO_INTRA_PMODE;

  if (mb->mbType == IPCM)
    mb_pred_ipcm (mb);
  else if (mb->mbType == I16MB)
    mb_pred_intra16x16 (mb, curPlane, picture);
  else if (mb->mbType == I4MB)
    mb_pred_intra4x4 (mb, curPlane, curPixel, picture);
  else if (mb->mbType == I8MB)
    mb_pred_intra8x8 (mb, curPlane, curPixel, picture);
  else if (mb->mbType == PSKIP)
    mb_pred_sp_skip (mb, curPlane, picture);
  else if (mb->mbType == P16x16)
    mb_pred_p_inter16x16 (mb, curPlane, picture);
  else if (mb->mbType == P16x8)
    mb_pred_p_inter16x8 (mb, curPlane, picture);
  else if (mb->mbType == P8x16)
    mb_pred_p_inter8x16 (mb, curPlane, picture);
  else
    mb_pred_p_inter8x8 (mb, curPlane, picture);

  return 1;
  }
//}}}
//{{{
static int decode_one_component_b_slice (sMacroblock* mb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture)
{
  //For residual DPCM
  mb->ipmode_DPCM = NO_INTRA_PMODE;

  if(mb->mbType == IPCM)
    mb_pred_ipcm (mb);
  else if (mb->mbType == I16MB)
    mb_pred_intra16x16 (mb, curPlane, picture);
  else if (mb->mbType == I4MB)
    mb_pred_intra4x4 (mb, curPlane, curPixel, picture);
  else if (mb->mbType == I8MB)
    mb_pred_intra8x8 (mb, curPlane, curPixel, picture);
  else if (mb->mbType == P16x16)
    mb_pred_p_inter16x16 (mb, curPlane, picture);
  else if (mb->mbType == P16x8)
    mb_pred_p_inter16x8 (mb, curPlane, picture);
  else if (mb->mbType == P8x16)
    mb_pred_p_inter8x16 (mb, curPlane, picture);
  else if (mb->mbType == BSKIP_DIRECT) {
    sSlice* curSlice = mb->slice;
    if (curSlice->direct_spatial_mv_pred_flag == 0) {
      if (curSlice->activeSPS->direct_8x8_inference_flag)
        mb_pred_b_d8x8temporal (mb, curPlane, curPixel, picture);
      else
        mb_pred_b_d4x4temporal (mb, curPlane, curPixel, picture);
      }
    else {
      if (curSlice->activeSPS->direct_8x8_inference_flag)
        mb_pred_b_d8x8spatial (mb, curPlane, curPixel, picture);
      else
        mb_pred_b_d4x4spatial (mb, curPlane, curPixel, picture);
      }
    }
  else
    mb_pred_b_inter8x8 (mb, curPlane, picture);

  return 1;
  }
//}}}

//{{{
static int BType2CtxRef (int btype) {
  return (btype >= 4);
  }
//}}}
//{{{
static char readRefPictureIdx_VLC (sMacroblock* mb, sSyntaxElement* currSE,
                                   sDataPartition* dP, char b8mode, int list) {

  currSE->context = BType2CtxRef (b8mode);
  currSE->value2 = list;
  dP->readsSyntaxElement (mb, currSE, dP);
  return (char) currSE->value1;
  }
//}}}
//{{{
static char readRefPictureIdx_FLC (sMacroblock* mb, sSyntaxElement* currSE, sDataPartition *dP, char b8mode, int list)
{
  currSE->context = BType2CtxRef (b8mode);
  currSE->len = 1;
  readsSyntaxElement_FLC(currSE, dP->bitstream);
  currSE->value1 = 1 - currSE->value1;

  return (char)currSE->value1;
  }
//}}}
//{{{
static char readRefPictureIdx_Null (sMacroblock* mb, sSyntaxElement* currSE, sDataPartition *dP, char b8mode, int list)
{
  return 0;
}
//}}}
//{{{
static void prepareListforRefIdx (sMacroblock* mb, sSyntaxElement* currSE,
                                  sDataPartition *dP, int numRefIndexActive, int refidx_present) {

  if (numRefIndexActive > 1) {
    if (mb->vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag) {
      currSE->mapping = linfo_ue;
      if (refidx_present)
        mb->readRefPictureIdx = (numRefIndexActive == 2) ? readRefPictureIdx_FLC : readRefPictureIdx_VLC;
      else
        mb->readRefPictureIdx = readRefPictureIdx_Null;
      }
    else {
      currSE->reading = readRefFrame_CABAC;
      mb->readRefPictureIdx = (refidx_present) ? readRefPictureIdx_VLC : readRefPictureIdx_Null;
      }
    }
  else
    mb->readRefPictureIdx = readRefPictureIdx_Null;
  }
//}}}

//{{{
void set_chroma_qp (sMacroblock* mb) {

  sVidParam* vidParam = mb->vidParam;
  sPicture* picture = mb->slice->picture;

  for (int i = 0; i < 2; ++i) {
    mb->qpc[i] = iClip3 (-vidParam->bitdepthChromaQpScale, 51, mb->qp + picture->chromaQpOffset[i] );
    mb->qpc[i] = mb->qpc[i] < 0 ? mb->qpc[i] : QP_SCALE_CR[mb->qpc[i]];
    mb->qpScaled[i+1] = mb->qpc[i] + vidParam->bitdepthChromaQpScale;
  }
}
//}}}
//{{{
void updateQp (sMacroblock* mb, int qp) {

  sVidParam* vidParam = mb->vidParam;

  mb->qp = qp;
  mb->qpScaled[0] = qp + vidParam->bitdepth_luma_qp_scale;
  set_chroma_qp (mb);

  mb->isLossless = (Boolean)((mb->qpScaled[0] == 0) && (vidParam->lossless_qpprime_flag == 1));
  set_read_comp_coeff_cavlc (mb);
  set_read_comp_coeff_cabac (mb);
  }
//}}}
//{{{
void readDeltaQuant (sSyntaxElement* currSE, sDataPartition *dP, sMacroblock* mb, const byte *partMap, int type)
{
  sSlice* curSlice = mb->slice;
  sVidParam* vidParam = mb->vidParam;

  currSE->type = type;

  dP = &(curSlice->partArr[partMap[currSE->type]]);

  if (vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag)
    currSE->mapping = linfo_se;
  else
    currSE->reading= read_dQuant_CABAC;

  dP->readsSyntaxElement(mb, currSE, dP);
  mb->deltaQuant = (short) currSE->value1;
  if ((mb->deltaQuant < -(26 + vidParam->bitdepth_luma_qp_scale/2)) ||
      (mb->deltaQuant > (25 + vidParam->bitdepth_luma_qp_scale/2))) {
    printf("mb_qp_delta is out of range (%d)\n", mb->deltaQuant);
    mb->deltaQuant = iClip3(-(26 + vidParam->bitdepth_luma_qp_scale/2), (25 + vidParam->bitdepth_luma_qp_scale/2), mb->deltaQuant);
    //error ("mb_qp_delta is out of range", 500);
    }

  curSlice->qp = ((curSlice->qp + mb->deltaQuant + 52 + 2*vidParam->bitdepth_luma_qp_scale) % (52+vidParam->bitdepth_luma_qp_scale)) - vidParam->bitdepth_luma_qp_scale;
  updateQp (mb, curSlice->qp);
  }
//}}}

//{{{
static void readMBRefPictureIdx (sSyntaxElement* currSE, sDataPartition *dP,
                                 sMacroblock* mb, sPicMotionParam** mvInfo,
                                 int list, int step_v0, int step_h0) {

  if (mb->mbType == 1) {
    if ((mb->b8pdir[0] == list || mb->b8pdir[0] == BI_PRED)) {
      mb->subblockX = 0;
      mb->subblockY = 0;
      char refframe = mb->readRefPictureIdx(mb, currSE, dP, 1, list);
      for (int j = 0; j <  step_v0; ++j) {
        char* refIndex = &mvInfo[j][mb->blockX].refIndex[list];
        for (int i = 0; i < step_h0; ++i) {
          *refIndex = refframe;
          refIndex += sizeof(sPicMotionParam);
          }
        }
      }
    }
  else if (mb->mbType == 2) {
    for (int j0 = 0; j0 < 4; j0 += step_v0) {
      int k = j0;
      if ((mb->b8pdir[k] == list || mb->b8pdir[k] == BI_PRED)) {
        mb->subblockY = j0 << 2;
        mb->subblockX = 0;
        char refframe = mb->readRefPictureIdx(mb, currSE, dP, mb->b8mode[k], list);
        for (int j = j0; j < j0 + step_v0; ++j) {
          char *refIndex = &mvInfo[j][mb->blockX].refIndex[list];
          for (int i = 0; i < step_h0; ++i) {
            *refIndex = refframe;
            refIndex += sizeof(sPicMotionParam);
            }
          }
        }
      }
    }
  else if (mb->mbType == 3) {
    mb->subblockY = 0;
    for (int i0 = 0; i0 < 4; i0 += step_h0) {
      int k = (i0 >> 1);
      if ((mb->b8pdir[k] == list || mb->b8pdir[k] == BI_PRED) && mb->b8mode[k] != 0) {
        mb->subblockX = i0 << 2;
        char refframe = mb->readRefPictureIdx(mb, currSE, dP, mb->b8mode[k], list);
        for (int j = 0; j < step_v0; ++j) {
          char *refIndex = &mvInfo[j][mb->blockX + i0].refIndex[list];
          for (int i = 0; i < step_h0; ++i) {
            *refIndex = refframe;
            refIndex += sizeof(sPicMotionParam);
            }
          }
        }
      }
    }
  else {
    for (int j0 = 0; j0 < 4; j0 += step_v0) {
      mb->subblockY = j0 << 2;
      for (int i0 = 0; i0 < 4; i0 += step_h0) {
        int k = 2 * (j0 >> 1) + (i0 >> 1);
        if ((mb->b8pdir[k] == list || mb->b8pdir[k] == BI_PRED) && mb->b8mode[k] != 0) {
          mb->subblockX = i0 << 2;
          char refframe = mb->readRefPictureIdx(mb, currSE, dP, mb->b8mode[k], list);
          for (int j = j0; j < j0 + step_v0; ++j) {
            char *refIndex = &mvInfo[j][mb->blockX + i0].refIndex[list];
            for (int i = 0; i < step_h0; ++i) {
              *refIndex = refframe;
              refIndex += sizeof(sPicMotionParam);
              }
            }
          }
        }
      }
    }
  }
//}}}
//{{{
static void readMBMotionVectors (sSyntaxElement* currSE, sDataPartition *dP, sMacroblock* mb,
                                 int list, int step_h0, int step_v0) {

  if (mb->mbType == 1) {
    if ((mb->b8pdir[0] == list || mb->b8pdir[0]== BI_PRED)) {
      // has forward vector
      int i4, j4, ii, jj;
      short curr_mvd[2];
      sMotionVector pred_mv, curr_mv;
      short (*mvd)[4][2];
      sPicMotionParam** mvInfo = mb->slice->picture->mvInfo;
      sPixelPos block[4]; // neighbor blocks
      mb->subblockX = 0; // position used for context determination
      mb->subblockY = 0; // position used for context determination
      i4  = mb->blockX;
      j4  = mb->blockY;
      mvd = &mb->mvd [list][0];

      getNeighbours(mb, block, 0, 0, step_h0 << 2);

      // first get MV predictor
      mb->GetMVPredictor (mb, block, &pred_mv, mvInfo[j4][i4].refIndex[list], mvInfo, list, 0, 0, step_h0 << 2, step_v0 << 2);

      // X component
      currSE->value2 = list; // identifies the component; only used for context determination
      dP->readsSyntaxElement(mb, currSE, dP);
      curr_mvd[0] = (short) currSE->value1;

      // Y component
      currSE->value2 += 2; // identifies the component; only used for context determination
      dP->readsSyntaxElement(mb, currSE, dP);
      curr_mvd[1] = (short) currSE->value1;

      curr_mv.mvX = (short)(curr_mvd[0] + pred_mv.mvX);  // compute motion vector x
      curr_mv.mvY = (short)(curr_mvd[1] + pred_mv.mvY);  // compute motion vector y

      for (jj = j4; jj < j4 + step_v0; ++jj) {
        sPicMotionParam *mvinfo = mvInfo[jj] + i4;
        for (ii = i4; ii < i4 + step_h0; ++ii)
          (mvinfo++)->mv[list] = curr_mv;
        }

      // Init first line (mvd)
      for (ii = 0; ii < step_h0; ++ii) {
        mvd[0][ii][0] = curr_mvd[0];
        mvd[0][ii][1] = curr_mvd[1];
        }

      // now copy all other lines
      for (jj = 1; jj < step_v0; ++jj)
        memcpy (mvd[jj][0], mvd[0][0],  2 * step_h0 * sizeof(short));
      }
    }
  else {
    int i4, j4, ii, jj;
    short curr_mvd[2];
    sMotionVector pred_mv, curr_mv;
    short (*mvd)[4][2];
    sPicMotionParam** mvInfo = mb->slice->picture->mvInfo;
    sPixelPos block[4]; // neighbor blocks

    int i, j, i0, j0, kk, k;
    for (j0 = 0; j0 < 4; j0 += step_v0) {
      for (i0 = 0; i0 < 4; i0 += step_h0) {
        kk = 2 * (j0 >> 1) + (i0 >> 1);
        if ((mb->b8pdir[kk] == list || mb->b8pdir[kk]== BI_PRED) && (mb->b8mode[kk] != 0)) {
          // has forward vector
          char cur_ref_idx = mvInfo[mb->blockY+j0][mb->blockX+i0].refIndex[list];
          int mv_mode  = mb->b8mode[kk];
          int step_h = BLOCK_STEP [mv_mode][0];
          int step_v = BLOCK_STEP [mv_mode][1];
          int step_h4 = step_h << 2;
          int step_v4 = step_v << 2;
          for (j = j0; j < j0 + step_v0; j += step_v) {
            mb->subblockY = j << 2; // position used for context determination
            j4  = mb->blockY + j;
            mvd = &mb->mvd [list][j];
            for (i = i0; i < i0 + step_h0; i += step_h) {
              mb->subblockX = i << 2; // position used for context determination
              i4 = mb->blockX + i;

              getNeighbours (mb, block, BLOCK_SIZE * i, BLOCK_SIZE * j, step_h4);
              // first get MV predictor
              mb->GetMVPredictor (mb, block, &pred_mv, cur_ref_idx, mvInfo, list, BLOCK_SIZE * i, BLOCK_SIZE * j, step_h4, step_v4);

              for (k = 0; k < 2; ++k) {
                currSE->value2   = (k << 1) + list; // identifies the component; only used for context determination
                dP->readsSyntaxElement (mb, currSE, dP);
                curr_mvd[k] = (short)currSE->value1;
                }

              curr_mv.mvX = (short)(curr_mvd[0] + pred_mv.mvX);  // compute motion vector
              curr_mv.mvY = (short)(curr_mvd[1] + pred_mv.mvY);  // compute motion vector

              for(jj = j4; jj < j4 + step_v; ++jj) {
                sPicMotionParam *mvinfo = mvInfo[jj] + i4;
                for(ii = i4; ii < i4 + step_h; ++ii)
                  (mvinfo++)->mv[list] = curr_mv;
                }

              // Init first line (mvd)
              for(ii = i; ii < i + step_h; ++ii) {
                mvd[0][ii][0] = curr_mvd[0];
                mvd[0][ii][1] = curr_mvd[1];
                }

              // now copy all other lines
              for (jj = 1; jj < step_v; ++jj)
                memcpy(&mvd[jj][i][0], &mvd[0][i][0],  2 * step_h * sizeof(short));
              }
            }
          }
        }
      }
    }
  }
//}}}
//{{{
void invScaleCoeff (sMacroblock* mb, int level, int run, int qp_per, int i, int j, int i0, int j0, int coef_ctr, const byte (*pos_scan4x4)[2], int (*InvLevelScale4x4)[4])
{
  if (level != 0) {
    /* leave if level == 0 */
    coef_ctr += run + 1;

    i0 = pos_scan4x4[coef_ctr][0];
    j0 = pos_scan4x4[coef_ctr][1];

    mb->cbpStructure[0].blk |= i64_power2((j << 2) + i) ;
    mb->slice->cof[0][(j<<2) + j0][(i<<2) + i0]= rshift_rnd_sf((level * InvLevelScale4x4[j0][i0]) << qp_per, 4);
    }
  }
//}}}
//{{{
static void setup_mb_pos_info (sMacroblock* mb) {

  int mb_x = mb->mb.x;
  int mb_y = mb->mb.y;

  mb->blockX = mb_x << BLOCK_SHIFT;  /* horizontal block position */
  mb->blockY = mb_y << BLOCK_SHIFT;  /* vertical block position */
  mb->blockYaff = mb->blockY;     /* interlace relative vertical position */

  mb->pixX = mb_x << MB_BLOCK_SHIFT; /* horizontal luma pixel position */
  mb->pixY = mb_y << MB_BLOCK_SHIFT; /* vertical luma pixel position */

  mb->pixcX = mb_x * mb->vidParam->mbCrSizeX;  /* horizontal chroma pixel position */
  mb->piccY = mb_y * mb->vidParam->mbCrSizeY;  /* vertical chroma pixel position */
  }
//}}}

//{{{
void startMacroblock (sSlice* curSlice, sMacroblock** mb) {

  sVidParam* vidParam = curSlice->vidParam;
  int mb_nr = curSlice->current_mb_nr;

  *mb = &curSlice->mbData[mb_nr];
  (*mb)->slice = curSlice;
  (*mb)->vidParam   = vidParam;
  (*mb)->mbAddrX = mb_nr;

  /* Update coordinates of the current macroblock */
  if (curSlice->mbAffFrameFlag) {
    (*mb)->mb.x = (short) (   (mb_nr) % ((2*vidParam->width) / MB_BLOCK_SIZE));
    (*mb)->mb.y = (short) (2*((mb_nr) / ((2*vidParam->width) / MB_BLOCK_SIZE)));
    (*mb)->mb.y += ((*mb)->mb.x & 0x01);
    (*mb)->mb.x >>= 1;
    }
  else
    (*mb)->mb = vidParam->picPos[mb_nr];

  /* Define pixel/block positions */
  setup_mb_pos_info (*mb);

  // reset intra mode
  (*mb)->isIntraBlock = FALSE;
  // reset mode info
  (*mb)->mbType = 0;
  (*mb)->deltaQuant = 0;
  (*mb)->cbp = 0;
  (*mb)->cPredMode = DC_PRED_8;

  // Save the slice number of this macroblock. When the macroblock below
  // is coded it will use this to decide if prediction for above is possible
  (*mb)->sliceNum = (short) curSlice->curSliceNum;

  CheckAvailabilityOfNeighbors (*mb);

  // Select appropriate MV predictor function
  init_motion_vector_prediction (*mb, curSlice->mbAffFrameFlag);
  set_read_and_store_CBP (mb, curSlice->activeSPS->chromaFormatIdc);

  // Reset syntax element entries in MB struct
  if (curSlice->sliceType != I_SLICE) {
    if (curSlice->sliceType != B_SLICE)
      memset((*mb)->mvd[0][0][0], 0, MB_BLOCK_PARTITIONS * 2 * sizeof(short));
    else
      memset((*mb)->mvd[0][0][0], 0, 2 * MB_BLOCK_PARTITIONS * 2 * sizeof(short));
    }

  memset((*mb)->cbpStructure, 0, 3 * sizeof(sCBPStructure));

  // initialize curSlice->mb_rres
  if (curSlice->is_reset_coeff == FALSE) {
    memset (curSlice->mb_rres[0][0], 0, MB_PIXELS * sizeof(int));
    memset (curSlice->mb_rres[1][0], 0, vidParam->mbCrSize * sizeof(int));
    memset (curSlice->mb_rres[2][0], 0, vidParam->mbCrSize * sizeof(int));
    if (curSlice->is_reset_coeff_cr == FALSE) {
      memset (curSlice->cof[0][0], 0, 3 * MB_PIXELS * sizeof(int));
      curSlice->is_reset_coeff_cr = TRUE;
      }
    else
      memset (curSlice->cof[0][0], 0, MB_PIXELS * sizeof(int));
    curSlice->is_reset_coeff = TRUE;
    }

  // store filtering parameters for this MB
  (*mb)->DFDisableIdc = curSlice->DFDisableIdc;
  (*mb)->DFAlphaC0Offset = curSlice->DFAlphaC0Offset;
  (*mb)->DFBetaOffset = curSlice->DFBetaOffset;
  (*mb)->listOffset = 0;
  (*mb)->mixedModeEdgeFlag = 0;
  }
//}}}
//{{{
Boolean exitMacroblock (sSlice* curSlice, int eos_bit) {

  sVidParam* vidParam = curSlice->vidParam;

  //! The if() statement below resembles the original code, which tested
  //! vidParam->current_mb_nr == vidParam->picSizeInMbs.  Both is, of course, nonsense
  //! In an error prone environment, one can only be sure to have a new
  //! picture by checking the tr of the next slice header!

  // printf ("exitMacroblock: FmoGetLastMBOfPicture %d, vidParam->current_mb_nr %d\n", FmoGetLastMBOfPicture(), vidParam->current_mb_nr);
  ++(curSlice->numDecodedMb);

  if (curSlice->current_mb_nr == vidParam->picSizeInMbs - 1) //if (vidParam->numDecodedMb == vidParam->picSizeInMbs)
    return TRUE;
    // ask for last mb in the slice  CAVLC
  else {
    curSlice->current_mb_nr = FmoGetNextMBNr (vidParam, curSlice->current_mb_nr);
    if (curSlice->current_mb_nr == -1) {
      // End of sSlice group, MUST be end of slice
      assert (curSlice->nal_startcode_follows (curSlice, eos_bit) == TRUE);
      return TRUE;
      }

    if (curSlice->nal_startcode_follows(curSlice, eos_bit) == FALSE)
      return FALSE;
    if (curSlice->sliceType == I_SLICE  ||
        curSlice->sliceType == SI_SLICE ||
        vidParam->activePPS->entropyCodingModeFlag == (Boolean)CABAC)
      return TRUE;
    if (curSlice->cod_counter <= 0)
      return TRUE;

    return FALSE;
   }
  }
//}}}

//{{{
static void interpret_mb_mode_P (sMacroblock* mb) {

  static const short ICBPTAB[6] = {0,16,32,15,31,47};

  short mbmode = mb->mbType;
  if (mbmode < 4) {
    mb->mbType = mbmode;
    memset(mb->b8mode, mbmode, 4 * sizeof(char));
    memset(mb->b8pdir, 0, 4 * sizeof(char));
    }
  else if ((mbmode == 4 || mbmode == 5)) {
    mb->mbType = P8x8;
    mb->slice->allrefzero = (mbmode == 5);
    }
  else if (mbmode == 6) {
    mb->isIntraBlock = TRUE;
    mb->mbType = I4MB;
    memset(mb->b8mode, IBLOCK, 4 * sizeof(char));
    memset(mb->b8pdir,     -1, 4 * sizeof(char));
    }
  else if (mbmode == 31) {
    mb->isIntraBlock = TRUE;
    mb->mbType = IPCM;
    mb->cbp = -1;
    mb->i16mode = 0;

    memset(mb->b8mode, 0, 4 * sizeof(char));
    memset(mb->b8pdir,-1, 4 * sizeof(char));
    }
  else {
    mb->isIntraBlock = TRUE;
    mb->mbType = I16MB;
    mb->cbp = ICBPTAB[((mbmode-7))>>2];
    mb->i16mode = ((mbmode-7)) & 0x03;
    memset(mb->b8mode, 0, 4 * sizeof(char));
    memset(mb->b8pdir,-1, 4 * sizeof(char));
    }
  }
//}}}
//{{{
static void interpret_mb_mode_I (sMacroblock* mb) {

  static const short ICBPTAB[6] = {0,16,32,15,31,47};

  short mbmode = mb->mbType;
  if (mbmode == 0) {
    mb->isIntraBlock = TRUE;
    mb->mbType = I4MB;
    memset (mb->b8mode,IBLOCK,4 * sizeof(char));
    memset (mb->b8pdir,-1,4 * sizeof(char));
    }
  else if (mbmode == 25) {
    mb->isIntraBlock = TRUE;
    mb->mbType=IPCM;
    mb->cbp= -1;
    mb->i16mode = 0;
    memset (mb->b8mode, 0,4 * sizeof(char));
    memset (mb->b8pdir,-1,4 * sizeof(char));
    }
  else {
    mb->isIntraBlock = TRUE;
    mb->mbType = I16MB;
    mb->cbp= ICBPTAB[(mbmode-1)>>2];
    mb->i16mode = (mbmode-1) & 0x03;
    memset (mb->b8mode, 0, 4 * sizeof(char));
    memset (mb->b8pdir,-1, 4 * sizeof(char));
    }
  }
//}}}
//{{{
static void interpret_mb_mode_B (sMacroblock* mb) {

  //{{{
  static const char offset2pdir16x16[12] = {
    0, 0, 1, 2, 0,0,0,0,0,0,0,0
    };
  //}}}
  //{{{
  static const char offset2pdir16x8[22][2] = {
    {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},{1,0},
    {0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2},{0,0}};
  //}}}
  //{{{
  static const char offset2pdir8x16[22][2] = {
    {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},
    {1,0},{0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2}
    };
  //}}}
  //{{{
  static const char ICBPTAB[6] = {
    0,16,32,15,31,47
    };
  //}}}

  short i, mbmode;

  //--- set mbtype, b8type, and b8pdir ---
  short mbtype  = mb->mbType;
  if (mbtype == 0) { // direct
    mbmode = 0;
    memset(mb->b8mode, 0, 4 * sizeof(char));
    memset(mb->b8pdir, 2, 4 * sizeof(char));
    }
  else if (mbtype == 23) { // intra4x4
    mb->isIntraBlock = TRUE;
    mbmode = I4MB;
    memset(mb->b8mode, IBLOCK,4 * sizeof(char));
    memset(mb->b8pdir, -1,4 * sizeof(char));
    }
  else if ((mbtype > 23) && (mbtype < 48) ) { // intra16x16
    mb->isIntraBlock = TRUE;
    mbmode = I16MB;
    memset(mb->b8mode,  0, 4 * sizeof(char));
    memset(mb->b8pdir, -1, 4 * sizeof(char));
    mb->cbp = (int) ICBPTAB[(mbtype-24)>>2];
    mb->i16mode = (mbtype-24) & 0x03;
    }
  else if (mbtype == 22) // 8x8(+split)
    mbmode = P8x8;       // b8mode and pdir is transmitted in additional codewords
  else if (mbtype < 4) { // 16x16
    mbmode = 1;
    memset (mb->b8mode, 1,4 * sizeof(char));
    memset (mb->b8pdir, offset2pdir16x16[mbtype], 4 * sizeof(char));
    }
  else if(mbtype == 48) {
    mb->isIntraBlock = TRUE;
    mbmode = IPCM;
    memset (mb->b8mode, 0,4 * sizeof(char));
    memset (mb->b8pdir,-1,4 * sizeof(char));
    mb->cbp= -1;
    mb->i16mode = 0;
    }
  else if ((mbtype & 0x01) == 0) { // 16x8
    mbmode = 2;
    memset (mb->b8mode, 2,4 * sizeof(char));
    for (i = 0; i < 4;++i)
      mb->b8pdir[i] = offset2pdir16x8 [mbtype][i>>1];
    }
  else {
    mbmode = 3;
    memset (mb->b8mode, 3,4 * sizeof(char));
    for (i = 0; i < 4; ++i)
      mb->b8pdir[i] = offset2pdir8x16 [mbtype][i&0x01];
    }

  mb->mbType = mbmode;
  }
//}}}
//{{{
static void interpret_mb_mode_SI (sMacroblock* mb) {

  //sVidParam* vidParam = mb->vidParam;
  const int ICBPTAB[6] = {0,16,32,15,31,47};

  short mbmode = mb->mbType;
  if (mbmode == 0) {
    mb->isIntraBlock = TRUE;
    mb->mbType = SI4MB;
    memset (mb->b8mode,IBLOCK,4 * sizeof(char));
    memset (mb->b8pdir,-1,4 * sizeof(char));
    mb->slice->siBlock[mb->mb.y][mb->mb.x]=1;
    }
  else if (mbmode == 1) {
    mb->isIntraBlock = TRUE;
    mb->mbType = I4MB;
    memset (mb->b8mode,IBLOCK,4 * sizeof(char));
    memset (mb->b8pdir,-1,4 * sizeof(char));
    }
  else if (mbmode == 26) {
    mb->isIntraBlock = TRUE;
    mb->mbType = IPCM;
    mb->cbp= -1;
    mb->i16mode = 0;
    memset (mb->b8mode,0,4 * sizeof(char));
    memset (mb->b8pdir,-1,4 * sizeof(char));
    }
  else {
    mb->isIntraBlock = TRUE;
    mb->mbType = I16MB;
    mb->cbp= ICBPTAB[(mbmode-2)>>2];
    mb->i16mode = (mbmode-2) & 0x03;
    memset (mb->b8mode,0,4 * sizeof(char));
    memset (mb->b8pdir,-1,4 * sizeof(char));
    }
  }
//}}}
//{{{
static void read_motion_info_from_NAL_p_slice (sMacroblock* mb){

  sVidParam* vidParam = mb->vidParam;
  sSlice* curSlice = mb->slice;

  sSyntaxElement currSE;
  sDataPartition *dP = NULL;
  const byte *partMap       = assignSE2partition[curSlice->dataPartitionMode];
  short partmode        = ((mb->mbType == P8x8) ? 4 : mb->mbType);
  int step_h0         = BLOCK_STEP [partmode][0];
  int step_v0         = BLOCK_STEP [partmode][1];

  int j4;
  sPicture* picture = curSlice->picture;
  sPicMotionParam *mvInfo = NULL;

  int listOffset = mb->listOffset;
  sPicture** list0 = curSlice->listX[LIST_0 + listOffset];
  sPicMotionParam** p_mv_info = &picture->mvInfo[mb->blockY];

  //=====  READ REFERENCE PICTURE INDICES =====
  currSE.type = SE_REFFRAME;
  dP = &(curSlice->partArr[partMap[SE_REFFRAME]]);

  //  For LIST_0, if multiple ref. pictures, read LIST_0 reference picture indices for the MB
  prepareListforRefIdx (mb, &currSE, dP, curSlice->numRefIndexActive[LIST_0], (mb->mbType != P8x8) || (!curSlice->allrefzero));
  readMBRefPictureIdx  (&currSE, dP, mb, p_mv_info, LIST_0, step_v0, step_h0);

  //=====  READ MOTION VECTORS =====
  currSE.type = SE_MVD;
  dP = &(curSlice->partArr[partMap[SE_MVD]]);
  if (vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag)
    currSE.mapping = linfo_se;
  else
    currSE.reading = curSlice->mbAffFrameFlag ? read_mvd_CABAC_mbaff : read_MVD_CABAC;

  // LIST_0 Motion vectors
  readMBMotionVectors (&currSE, dP, mb, LIST_0, step_h0, step_v0);

  // record reference picture Ids for deblocking decisions
  for (j4 = 0; j4 < 4;++j4)  {
    mvInfo = &p_mv_info[j4][mb->blockX];
    mvInfo->refPic[LIST_0] = list0[(short) mvInfo->refIndex[LIST_0]];
    mvInfo++;
    mvInfo->refPic[LIST_0] = list0[(short) mvInfo->refIndex[LIST_0]];
    mvInfo++;
    mvInfo->refPic[LIST_0] = list0[(short) mvInfo->refIndex[LIST_0]];
    mvInfo++;
    mvInfo->refPic[LIST_0] = list0[(short) mvInfo->refIndex[LIST_0]];
    }
  }
//}}}
//{{{
static void read_motion_info_from_NAL_b_slice (sMacroblock* mb) {

  sSlice* curSlice = mb->slice;
  sVidParam* vidParam = mb->vidParam;
  sPicture* picture = curSlice->picture;
  sSyntaxElement currSE;
  sDataPartition* dP = NULL;
  const byte* partMap = assignSE2partition[curSlice->dataPartitionMode];
  int partmode = ((mb->mbType == P8x8) ? 4 : mb->mbType);
  int step_h0 = BLOCK_STEP [partmode][0];
  int step_v0 = BLOCK_STEP [partmode][1];
  int j4, i4;

  int listOffset = mb->listOffset;
  sPicture** list0 = curSlice->listX[LIST_0 + listOffset];
  sPicture** list1 = curSlice->listX[LIST_1 + listOffset];
  sPicMotionParam** p_mv_info = &picture->mvInfo[mb->blockY];

  if (mb->mbType == P8x8)
    curSlice->updateDirectMvInfo(mb);

  //=====  READ REFERENCE PICTURE INDICES =====
  currSE.type = SE_REFFRAME;
  dP = &(curSlice->partArr[partMap[SE_REFFRAME]]);

  //  For LIST_0, if multiple ref. pictures, read LIST_0 reference picture indices for the MB
  prepareListforRefIdx (mb, &currSE, dP, curSlice->numRefIndexActive[LIST_0], TRUE);
  readMBRefPictureIdx (&currSE, dP, mb, p_mv_info, LIST_0, step_v0, step_h0);

  //  For LIST_1, if multiple ref. pictures, read LIST_1 reference picture indices for the MB
  prepareListforRefIdx (mb, &currSE, dP, curSlice->numRefIndexActive[LIST_1], TRUE);
  readMBRefPictureIdx (&currSE, dP, mb, p_mv_info, LIST_1, step_v0, step_h0);

  //=====  READ MOTION VECTORS =====
  currSE.type = SE_MVD;
  dP = &(curSlice->partArr[partMap[SE_MVD]]);
  if (vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag)
    currSE.mapping = linfo_se;
  else
    currSE.reading = curSlice->mbAffFrameFlag ? read_mvd_CABAC_mbaff : read_MVD_CABAC;

  // LIST_0 Motion vectors
  readMBMotionVectors (&currSE, dP, mb, LIST_0, step_h0, step_v0);

  // LIST_1 Motion vectors
  readMBMotionVectors (&currSE, dP, mb, LIST_1, step_h0, step_v0);

  // record reference picture Ids for deblocking decisions
  for (j4 = 0; j4 < 4; ++j4) {
    for (i4 = mb->blockX; i4 < (mb->blockX + 4); ++i4) {
      sPicMotionParam *mvInfo = &p_mv_info[j4][i4];
      short refIndex = mvInfo->refIndex[LIST_0];
      mvInfo->refPic[LIST_0] = (refIndex >= 0) ? list0[refIndex] : NULL;
      refIndex = mvInfo->refIndex[LIST_1];
      mvInfo->refPic[LIST_1] = (refIndex >= 0) ? list1[refIndex] : NULL;
      }
    }
  }
//}}}
//{{{
void setSliceMethods (sSlice* slice) {

  setup_read_macroblock (slice);

  switch (slice->sliceType) {
    //{{{
    case P_SLICE:
      slice->interpretMbMode = interpret_mb_mode_P;
      slice->readMotionInfoFromNAL = read_motion_info_from_NAL_p_slice;
      slice->decodeOneComponent = decode_one_component_p_slice;
      slice->updateDirectMvInfo = NULL;
      slice->initLists = initListsPslice;
      break;
    //}}}
    //{{{
    case SP_SLICE:
      slice->interpretMbMode = interpret_mb_mode_P;
      slice->readMotionInfoFromNAL = read_motion_info_from_NAL_p_slice;
      slice->decodeOneComponent = decode_one_component_sp_slice;
      slice->updateDirectMvInfo = NULL;
      slice->initLists = initListsPslice;
      break;
    //}}}
    //{{{
    case B_SLICE:
      slice->interpretMbMode = interpret_mb_mode_B;
      slice->readMotionInfoFromNAL = read_motion_info_from_NAL_b_slice;
      slice->decodeOneComponent = decode_one_component_b_slice;
      update_direct_types (slice);
      slice->initLists  = initListsBslice;
      break;
    //}}}
    //{{{
    case I_SLICE:
      slice->interpretMbMode = interpret_mb_mode_I;
      slice->readMotionInfoFromNAL = NULL;
      slice->decodeOneComponent = decode_one_component_i_slice;
      slice->updateDirectMvInfo = NULL;
      slice->initLists = initListsIslice;
      break;
    //}}}
    //{{{
    case SI_SLICE:
      slice->interpretMbMode = interpret_mb_mode_SI;
      slice->readMotionInfoFromNAL = NULL;
      slice->decodeOneComponent = decode_one_component_i_slice;
      slice->updateDirectMvInfo = NULL;
      slice->initLists = initListsIslice;
      break;
    //}}}
    //{{{
    default:
      printf ("Unsupported slice type\n");
      break;
    //}}}
    }

  set_intra_prediction_modes (slice);

  if (slice->vidParam->activeSPS->chromaFormatIdc==YUV444 && (slice->vidParam->sepColourPlaneFlag == 0) )
    slice->readCoef4x4cavlc = readCoef4x4cavlc444;
  else
    slice->readCoef4x4cavlc = readCoef4x4cavlc;

  switch (slice->vidParam->activePPS->entropyCodingModeFlag) {
    case CABAC:
      set_read_CBP_and_coeffs_cabac (slice);
      break;
    case CAVLC:
      set_read_CBP_and_coeffs_cavlc (slice);
      break;
    default:
      printf ("Unsupported entropy coding mode\n");
      break;
    }
  }
//}}}

//{{{
void getNeighbours (sMacroblock* mb, sPixelPos* block, int mb_x, int mb_y, int blockshape_x) {

  int* mbSize = mb->vidParam->mbSize[IS_LUMA];

  get4x4Neighbour (mb, mb_x - 1, mb_y, mbSize, block);
  get4x4Neighbour (mb, mb_x, mb_y - 1, mbSize, block + 1);
  get4x4Neighbour (mb, mb_x + blockshape_x, mb_y - 1, mbSize, block + 2);

  if (mb_y > 0) {
    if (mb_x < 8) {
      // first column of 8x8 blocks
      if (mb_y == 8) {
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
    get4x4Neighbour (mb, mb_x - 1, mb_y - 1, mbSize, block + 3);
    block[2] = block[3];
    }
  }
//}}}
//{{{
void checkDpNeighbours (sMacroblock* mb) {

  sVidParam* vidParam = mb->vidParam;
  sPixelPos up, left;
  vidParam->getNeighbour (mb, -1,  0, vidParam->mbSize[1], &left);
  vidParam->getNeighbour (mb,  0, -1, vidParam->mbSize[1], &up);

  if ((mb->isIntraBlock == FALSE) || (!(vidParam->activePPS->constrainedIntraPredFlag)) ) {
    if (left.available)
      mb->dplFlag |= vidParam->mbData[left.mbAddr].dplFlag;
    if (up.available)
      mb->dplFlag |= vidParam->mbData[up.mbAddr].dplFlag;
    }
  }
//}}}

//{{{
static void init_cur_imgy (sVidParam* vidParam, sSlice* slice, int pl) {
// probably a better way (or place) to do this, but I'm not sure what (where) it is [CJV]
// this is intended to make get_block_luma faster, but I'm still performing
// this at the MB level, and it really should be done at the slice level

  if (vidParam->sepColourPlaneFlag == 0) {
    sPicture* vidref = vidParam->noReferencePicture;
    int noref = (slice->framePoc < vidParam->recoveryPoc);

    if (pl == PLANE_Y) {
      for (int j = 0; j < 6; j++) {
        for (int i = 0; i < slice->listXsize[j] ; i++) {
          sPicture* curRef = slice->listX[j][i];
          if (curRef) {
            curRef->noRef = noref && (curRef == vidref);
            curRef->curPixelY = curRef->imgY;
            }
          }
        }
      }
    else {
      for (int j = 0; j < 6; j++) {
        for (int i = 0; i < slice->listXsize[j]; i++) {
          sPicture* curRef = slice->listX[j][i];
          if (curRef) {
            curRef->noRef = noref && (curRef == vidref);
            curRef->curPixelY = curRef->imgUV[pl-1];
            }
          }
        }
      }
    }
  }
//}}}
//{{{
void change_plane_JV (sVidParam* vidParam, int nplane, sSlice* slice) {

  vidParam->mbData = vidParam->mbDataJV[nplane];
  vidParam->picture  = vidParam->dec_picture_JV[nplane];
  vidParam->siBlock = vidParam->siBlockJV[nplane];
  vidParam->predMode = vidParam->predModeJV[nplane];
  vidParam->intraBlock = vidParam->intraBlockJV[nplane];

  if (slice) {
    slice->mbData = vidParam->mbDataJV[nplane];
    slice->picture  = vidParam->dec_picture_JV[nplane];
    slice->siBlock = vidParam->siBlockJV[nplane];
    slice->predMode = vidParam->predModeJV[nplane];
    slice->intraBlock = vidParam->intraBlockJV[nplane];
    }
  }
//}}}
//{{{
void make_frame_picture_JV (sVidParam* vidParam) {

  vidParam->picture = vidParam->dec_picture_JV[0];

  // copy;
  if (vidParam->picture->usedForReference) {
    int nsize = (vidParam->picture->sizeY/BLOCK_SIZE)*(vidParam->picture->sizeX/BLOCK_SIZE)*sizeof(sPicMotionParam);
    memcpy (&(vidParam->picture->JVmv_info[PLANE_Y][0][0]), &(vidParam->dec_picture_JV[PLANE_Y]->mvInfo[0][0]), nsize);
    memcpy (&(vidParam->picture->JVmv_info[PLANE_U][0][0]), &(vidParam->dec_picture_JV[PLANE_U]->mvInfo[0][0]), nsize);
    memcpy (&(vidParam->picture->JVmv_info[PLANE_V][0][0]), &(vidParam->dec_picture_JV[PLANE_V]->mvInfo[0][0]), nsize);
    }

  // This could be done with pointers and seems not necessary
  for (int uv = 0; uv < 2; uv++) {
    for (int line = 0; line < vidParam->height; line++) {
      int nsize = sizeof(sPixel) * vidParam->width;
      memcpy (vidParam->picture->imgUV[uv][line], vidParam->dec_picture_JV[uv+1]->imgY[line], nsize );
      }
    freePicture (vidParam->dec_picture_JV[uv+1]);
    }
  }
//}}}

//{{{
int decodeMacroblock (sMacroblock* mb, sPicture* picture) {

  sSlice* curSlice = mb->slice;
  sVidParam* vidParam = mb->vidParam;

  if (curSlice->chroma444notSeparate) {
    if (!mb->isIntraBlock) {
      init_cur_imgy (vidParam, curSlice, PLANE_Y);
      curSlice->decodeOneComponent (mb, PLANE_Y, picture->imgY, picture);
      init_cur_imgy (vidParam, curSlice, PLANE_U);
      curSlice->decodeOneComponent (mb, PLANE_U, picture->imgUV[0], picture);
      init_cur_imgy (vidParam, curSlice, PLANE_V);
      curSlice->decodeOneComponent (mb, PLANE_V, picture->imgUV[1], picture);
      }
    else {
      curSlice->decodeOneComponent (mb, PLANE_Y, picture->imgY, picture);
      curSlice->decodeOneComponent (mb, PLANE_U, picture->imgUV[0], picture);
      curSlice->decodeOneComponent (mb, PLANE_V, picture->imgUV[1], picture);
      }
    curSlice->is_reset_coeff = FALSE;
    curSlice->is_reset_coeff_cr = FALSE;
    }
  else
    curSlice->decodeOneComponent(mb, PLANE_Y, picture->imgY, picture);

  return 0;
  }
//}}}
