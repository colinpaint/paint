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
extern void set_intra_prediction_modes (sSlice* slice);
extern void setReadMacroblock (sSlice* slice);
extern void set_read_CBP_and_coeffs_cavlc (sSlice* slice);
extern void set_read_comp_coeff_cavlc (sMacroblock* mb);

//{{{
static void GetMotionVectorPredictorMBAFF (sMacroblock* mb, sPixelPos* block,
                                           sMotionVec *pmv, short  ref_frame, sPicMotionParam** mvInfo,
                                           int list, int mb_x, int mb_y, int blockshape_x, int blockshape_y) {

  int mv_a, mv_b, mv_c, pred_vec=0;
  int mvPredType, rFrameL, rFrameU, rFrameUR;
  int hv;
  sDecoder* decoder = mb->decoder;
  mvPredType = MVPRED_MEDIAN;

  if (mb->mbField) {
    rFrameL  = block[0].available
      ? (decoder->mbData[block[0].mbIndex].mbField
      ? mvInfo[block[0].posY][block[0].posX].refIndex[list]
    : mvInfo[block[0].posY][block[0].posX].refIndex[list] * 2) : -1;
    rFrameU  = block[1].available
      ? (decoder->mbData[block[1].mbIndex].mbField
      ? mvInfo[block[1].posY][block[1].posX].refIndex[list]
    : mvInfo[block[1].posY][block[1].posX].refIndex[list] * 2) : -1;
    rFrameUR = block[2].available
      ? (decoder->mbData[block[2].mbIndex].mbField
      ? mvInfo[block[2].posY][block[2].posX].refIndex[list]
    : mvInfo[block[2].posY][block[2].posX].refIndex[list] * 2) : -1;
    }
  else {
    rFrameL = block[0].available
      ? (decoder->mbData[block[0].mbIndex].mbField
      ? mvInfo[block[0].posY][block[0].posX].refIndex[list] >>1
      : mvInfo[block[0].posY][block[0].posX].refIndex[list]) : -1;
    rFrameU  = block[1].available
      ? (decoder->mbData[block[1].mbIndex].mbField
      ? mvInfo[block[1].posY][block[1].posX].refIndex[list] >>1
      : mvInfo[block[1].posY][block[1].posX].refIndex[list]) : -1;
    rFrameUR = block[2].available
      ? (decoder->mbData[block[2].mbIndex].mbField
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
        mv_a = block[0].available  ? decoder->mbData[block[0].mbIndex].mbField
          ? mvInfo[block[0].posY][block[0].posX].mv[list].mvY
        : mvInfo[block[0].posY][block[0].posX].mv[list].mvY / 2
          : 0;
        mv_b = block[1].available  ? decoder->mbData[block[1].mbIndex].mbField
          ? mvInfo[block[1].posY][block[1].posX].mv[list].mvY
        : mvInfo[block[1].posY][block[1].posX].mv[list].mvY / 2
          : 0;
        mv_c = block[2].available  ? decoder->mbData[block[2].mbIndex].mbField
          ? mvInfo[block[2].posY][block[2].posX].mv[list].mvY
        : mvInfo[block[2].posY][block[2].posX].mv[list].mvY / 2
          : 0;
        }
      else {
        mv_a = block[0].available  ? decoder->mbData[block[0].mbIndex].mbField
          ? mvInfo[block[0].posY][block[0].posX].mv[list].mvY * 2
          : mvInfo[block[0].posY][block[0].posX].mv[list].mvY
        : 0;
        mv_b = block[1].available  ? decoder->mbData[block[1].mbIndex].mbField
          ? mvInfo[block[1].posY][block[1].posX].mv[list].mvY * 2
          : mvInfo[block[1].posY][block[1].posX].mv[list].mvY
        : 0;
        mv_c = block[2].available  ? decoder->mbData[block[2].mbIndex].mbField
          ? mvInfo[block[2].posY][block[2].posX].mv[list].mvY * 2
          : mvInfo[block[2].posY][block[2].posX].mv[list].mvY
        : 0;
        }
      }

    switch (mvPredType) {
      case MVPRED_MEDIAN:
        if (!(block[1].available || block[2].available))
          pred_vec = mv_a;
        else
          pred_vec = imedian (mv_a, mv_b, mv_c);
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
      pmv->mvX = (short)pred_vec;
    else
      pmv->mvY = (short)pred_vec;
    }
  }
//}}}
//{{{
static void GetMotionVectorPredictorNormal (sMacroblock* mb, sPixelPos* block,
                                            sMotionVec *pmv, short  ref_frame, sPicMotionParam** mvInfo,
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
        sMotionVec *mv_a = block[0].available ? &mvInfo[block[0].posY][block[0].posX].mv[list] : (sMotionVec *) &zero_mv;
        sMotionVec *mv_b = block[1].available ? &mvInfo[block[1].posY][block[1].posX].mv[list] : (sMotionVec *) &zero_mv;
        sMotionVec *mv_c = block[2].available ? &mvInfo[block[2].posY][block[2].posX].mv[list] : (sMotionVec *) &zero_mv;

        pmv->mvX = (short)imedian (mv_a->mvX, mv_b->mvX, mv_c->mvX);
        pmv->mvY = (short)imedian (mv_a->mvY, mv_b->mvY, mv_c->mvY);
      }
      break;
    //}}}
    //{{{
    case MVPRED_L:
      if (block[0].available)
        *pmv = mvInfo[block[0].posY][block[0].posX].mv[list];
      else
        *pmv = zero_mv;
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
static int decodeComponentI (sMacroblock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  //For residual DPCM
  mb->ipmode_DPCM = NO_INTRA_PMODE;
  if (mb->mbType == IPCM)
    mb_pred_ipcm (mb);
  else if (mb->mbType == I16MB)
    mb_pred_intra16x16 (mb, plane, picture);
  else if (mb->mbType == I4MB)
    mb_pred_intra4x4 (mb, plane, pixel, picture);
  else if (mb->mbType == I8MB)
    mb_pred_intra8x8 (mb, plane, pixel, picture);

  return 1;
  }
//}}}
//{{{
static int decodeComponentP (sMacroblock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  //For residual DPCM
  mb->ipmode_DPCM = NO_INTRA_PMODE;
  if (mb->mbType == IPCM)
    mb_pred_ipcm (mb);
  else if (mb->mbType == I16MB)
    mb_pred_intra16x16 (mb, plane, picture);
  else if (mb->mbType == I4MB)
    mb_pred_intra4x4 (mb, plane, pixel, picture);
  else if (mb->mbType == I8MB)
    mb_pred_intra8x8 (mb, plane, pixel, picture);
  else if (mb->mbType == PSKIP)
    mb_pred_skip( mb, plane, pixel, picture);
  else if (mb->mbType == P16x16)
    mb_pred_p_inter16x16 (mb, plane, picture);
  else if (mb->mbType == P16x8)
    mb_pred_p_inter16x8 (mb, plane, picture);
  else if (mb->mbType == P8x16)
    mb_pred_p_inter8x16 (mb, plane, picture);
  else
    mb_pred_p_inter8x8 (mb, plane, picture);

  return 1;
  }
//}}}
//{{{
static int decodeComponentSP (sMacroblock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  //For residual DPCM
  mb->ipmode_DPCM = NO_INTRA_PMODE;

  if (mb->mbType == IPCM)
    mb_pred_ipcm (mb);
  else if (mb->mbType == I16MB)
    mb_pred_intra16x16 (mb, plane, picture);
  else if (mb->mbType == I4MB)
    mb_pred_intra4x4 (mb, plane, pixel, picture);
  else if (mb->mbType == I8MB)
    mb_pred_intra8x8 (mb, plane, pixel, picture);
  else if (mb->mbType == PSKIP)
    mb_pred_sp_skip (mb, plane, picture);
  else if (mb->mbType == P16x16)
    mb_pred_p_inter16x16 (mb, plane, picture);
  else if (mb->mbType == P16x8)
    mb_pred_p_inter16x8 (mb, plane, picture);
  else if (mb->mbType == P8x16)
    mb_pred_p_inter8x16 (mb, plane, picture);
  else
    mb_pred_p_inter8x8 (mb, plane, picture);

  return 1;
  }
//}}}
//{{{
static int decodeComponentB (sMacroblock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  //For residual DPCM
  mb->ipmode_DPCM = NO_INTRA_PMODE;

  if(mb->mbType == IPCM)
    mb_pred_ipcm (mb);
  else if (mb->mbType == I16MB)
    mb_pred_intra16x16 (mb, plane, picture);
  else if (mb->mbType == I4MB)
    mb_pred_intra4x4 (mb, plane, pixel, picture);
  else if (mb->mbType == I8MB)
    mb_pred_intra8x8 (mb, plane, pixel, picture);
  else if (mb->mbType == P16x16)
    mb_pred_p_inter16x16 (mb, plane, picture);
  else if (mb->mbType == P16x8)
    mb_pred_p_inter16x8 (mb, plane, picture);
  else if (mb->mbType == P8x16)
    mb_pred_p_inter8x16 (mb, plane, picture);
  else if (mb->mbType == BSKIP_DIRECT) {
    sSlice* slice = mb->slice;
    if (slice->directSpatialMvPredFlag == 0) {
      if (slice->activeSPS->direct_8x8_inference_flag)
        mb_pred_b_d8x8temporal (mb, plane, pixel, picture);
      else
        mb_pred_b_d4x4temporal (mb, plane, pixel, picture);
      }
    else {
      if (slice->activeSPS->direct_8x8_inference_flag)
        mb_pred_b_d8x8spatial (mb, plane, pixel, picture);
      else
        mb_pred_b_d4x4spatial (mb, plane, pixel, picture);
      }
    }
  else
    mb_pred_b_inter8x8 (mb, plane, picture);

  return 1;
  }
//}}}

//{{{
static int BType2CtxRef (int btype) {
  return (btype >= 4);
  }
//}}}
//{{{
static char readRefPictureIdxVLC (sMacroblock* mb, sSyntaxElement* se,
                                   sDataPartition* dataPartition, char b8mode, int list) {

  se->context = BType2CtxRef (b8mode);
  se->value2 = list;
  dataPartition->readSyntaxElement (mb, se, dataPartition);
  return (char) se->value1;
  }
//}}}
//{{{
static char readRefPictureIdxFLC (sMacroblock* mb, sSyntaxElement* se, sDataPartition* dataPartition, char b8mode, int list)
{
  se->context = BType2CtxRef (b8mode);
  se->len = 1;
  readsSyntaxElement_FLC(se, dataPartition->s);
  se->value1 = 1 - se->value1;

  return (char)se->value1;
  }
//}}}
//{{{
static char readRefPictureIdxNull (sMacroblock* mb, sSyntaxElement* se, sDataPartition* dataPartition, char b8mode, int list)
{
  return 0;
}
//}}}
//{{{
static void prepareListforRefIdx (sMacroblock* mb, sSyntaxElement* se,
                                  sDataPartition *dataPartition, int numRefIndexActive, int refidx_present) {

  if (numRefIndexActive > 1) {
    if (mb->decoder->activePPS->entropyCodingMode == (Boolean) CAVLC || dataPartition->s->errorFlag) {
      se->mapping = linfo_ue;
      if (refidx_present)
        mb->readRefPictureIndex = (numRefIndexActive == 2) ? readRefPictureIdxFLC : readRefPictureIdxVLC;
      else
        mb->readRefPictureIndex = readRefPictureIdxNull;
      }
    else {
      se->reading = readRefFrame_CABAC;
      mb->readRefPictureIndex = (refidx_present) ? readRefPictureIdxVLC : readRefPictureIdxNull;
      }
    }
  else
    mb->readRefPictureIndex = readRefPictureIdxNull;
  }
//}}}

//{{{
void set_chroma_qp (sMacroblock* mb) {

  sDecoder* decoder = mb->decoder;
  sPicture* picture = mb->slice->picture;
  for (int i = 0; i < 2; ++i) {
    mb->qpc[i] = iClip3 (-decoder->bitdepthChromaQpScale, 51, mb->qp + picture->chromaQpOffset[i] );
    mb->qpc[i] = mb->qpc[i] < 0 ? mb->qpc[i] : QP_SCALE_CR[mb->qpc[i]];
    mb->qpScaled[i+1] = mb->qpc[i] + decoder->bitdepthChromaQpScale;
  }
}
//}}}
//{{{
void updateQp (sMacroblock* mb, int qp) {

  sDecoder* decoder = mb->decoder;

  mb->qp = qp;
  mb->qpScaled[0] = qp + decoder->bitdepthLumaQpScale;
  set_chroma_qp (mb);

  mb->isLossless = (Boolean)((mb->qpScaled[0] == 0) && (decoder->coding.losslessQpPrimeFlag == 1));
  set_read_comp_coeff_cavlc (mb);
  set_read_comp_coeff_cabac (mb);
  }
//}}}
//{{{
void readDeltaQuant (sSyntaxElement* se, sDataPartition *dataPartition, sMacroblock* mb, const byte *dpMap, int type)
{
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  se->type = type;

  dataPartition = &(slice->dps[dpMap[se->type]]);

  if (decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag)
    se->mapping = linfo_se;
  else
    se->reading= read_dQuant_CABAC;

  dataPartition->readSyntaxElement(mb, se, dataPartition);
  mb->deltaQuant = (short) se->value1;
  if ((mb->deltaQuant < -(26 + decoder->bitdepthLumaQpScale/2)) ||
      (mb->deltaQuant > (25 + decoder->bitdepthLumaQpScale/2))) {
    printf("mb_qp_delta is out of range (%d)\n", mb->deltaQuant);
    mb->deltaQuant = iClip3(-(26 + decoder->bitdepthLumaQpScale/2), (25 + decoder->bitdepthLumaQpScale/2), mb->deltaQuant);
    //error ("mb_qp_delta is out of range", 500);
    }

  slice->qp = ((slice->qp + mb->deltaQuant + 52 + 2*decoder->bitdepthLumaQpScale) % (52+decoder->bitdepthLumaQpScale)) - decoder->bitdepthLumaQpScale;
  updateQp (mb, slice->qp);
  }
//}}}

//{{{
static void readMBRefPictureIdx (sSyntaxElement* se, sDataPartition *dataPartition,
                                 sMacroblock* mb, sPicMotionParam** mvInfo,
                                 int list, int step_v0, int step_h0) {

  if (mb->mbType == 1) {
    if ((mb->b8pdir[0] == list || mb->b8pdir[0] == BI_PRED)) {
      mb->subblockX = 0;
      mb->subblockY = 0;
      char refframe = mb->readRefPictureIndex(mb, se, dataPartition, 1, list);
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
        char refframe = mb->readRefPictureIndex(mb, se, dataPartition, mb->b8mode[k], list);
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
        char refframe = mb->readRefPictureIndex(mb, se, dataPartition, mb->b8mode[k], list);
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
          char refframe = mb->readRefPictureIndex(mb, se, dataPartition, mb->b8mode[k], list);
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
static void readMBMotionVectors (sSyntaxElement* se, sDataPartition *dataPartition, sMacroblock* mb,
                                 int list, int step_h0, int step_v0) {

  if (mb->mbType == 1) {
    if ((mb->b8pdir[0] == list || mb->b8pdir[0]== BI_PRED)) {
      // has forward vector
      int i4, j4, ii, jj;
      short curr_mvd[2];
      sMotionVec pred_mv, curr_mv;
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
      se->value2 = list; // identifies the component; only used for context determination
      dataPartition->readSyntaxElement(mb, se, dataPartition);
      curr_mvd[0] = (short) se->value1;

      // Y component
      se->value2 += 2; // identifies the component; only used for context determination
      dataPartition->readSyntaxElement(mb, se, dataPartition);
      curr_mvd[1] = (short) se->value1;

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
    sMotionVec pred_mv, curr_mv;
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
                se->value2   = (k << 1) + list; // identifies the component; only used for context determination
                dataPartition->readSyntaxElement (mb, se, dataPartition);
                curr_mvd[k] = (short)se->value1;
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

  mb->pixcX = mb_x * mb->decoder->mbCrSizeX;  /* horizontal chroma pixel position */
  mb->piccY = mb_y * mb->decoder->mbCrSizeY;  /* vertical chroma pixel position */
  }
//}}}

//{{{
void startMacroblock (sSlice* slice, sMacroblock** mb) {

  sDecoder* decoder = slice->decoder;
  int mbIndex = slice->mbIndex;

  *mb = &slice->mbData[mbIndex];
  (*mb)->slice = slice;
  (*mb)->decoder   = decoder;
  (*mb)->mbIndexX = mbIndex;

  // Update coordinates of the current macroblock
  if (slice->mbAffFrameFlag) {
    (*mb)->mb.x = (short) (   (mbIndex) % ((2*decoder->width) / MB_BLOCK_SIZE));
    (*mb)->mb.y = (short) (2*((mbIndex) / ((2*decoder->width) / MB_BLOCK_SIZE)));
    (*mb)->mb.y += ((*mb)->mb.x & 0x01);
    (*mb)->mb.x >>= 1;
    }
  else
    (*mb)->mb = decoder->picPos[mbIndex];

  // Define pixel/block positions
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
  (*mb)->sliceNum = (short) slice->curSliceIndex;

  CheckAvailabilityOfNeighbors (*mb);

  // Select appropriate MV predictor function
  init_motion_vector_prediction (*mb, slice->mbAffFrameFlag);
  set_read_and_store_CBP (mb, slice->activeSPS->chromaFormatIdc);

  // Reset syntax element entries in MB struct
  if (slice->sliceType != I_SLICE) {
    if (slice->sliceType != B_SLICE)
      memset((*mb)->mvd[0][0][0], 0, MB_BLOCK_dpS * 2 * sizeof(short));
    else
      memset((*mb)->mvd[0][0][0], 0, 2 * MB_BLOCK_dpS * 2 * sizeof(short));
    }

  memset((*mb)->cbpStructure, 0, 3 * sizeof(sCBPStructure));

  // initialize slice->mbRess
  if (slice->isResetCoef == FALSE) {
    memset (slice->mbRess[0][0], 0, MB_PIXELS * sizeof(int));
    memset (slice->mbRess[1][0], 0, decoder->mbCrSize * sizeof(int));
    memset (slice->mbRess[2][0], 0, decoder->mbCrSize * sizeof(int));
    if (slice->isResetCoefCr == FALSE) {
      memset (slice->cof[0][0], 0, 3 * MB_PIXELS * sizeof(int));
      slice->isResetCoefCr = TRUE;
      }
    else
      memset (slice->cof[0][0], 0, MB_PIXELS * sizeof(int));
    slice->isResetCoef = TRUE;
    }

  // store filtering parameters for this MB
  (*mb)->DFDisableIdc = slice->DFDisableIdc;
  (*mb)->DFAlphaC0Offset = slice->DFAlphaC0Offset;
  (*mb)->DFBetaOffset = slice->DFBetaOffset;
  (*mb)->listOffset = 0;
  (*mb)->mixedModeEdgeFlag = 0;
  }
//}}}
//{{{
Boolean exitMacroblock (sSlice* slice, int eos_bit) {

  // The if() statement below resembles the original code, which tested
  // decoder->mbIndex == decoder->picSizeInMbs.  Both is, of course, nonsense
  // In an error prone environment, one can only be sure to have a new
  // picture by checking the tr of the next slice header!
  ++(slice->numDecodedMbs);

  sDecoder* decoder = slice->decoder;
  if (slice->mbIndex == decoder->picSizeInMbs - 1) 
    return TRUE;
  else {
    slice->mbIndex = FmoGetNextMBNr (decoder, slice->mbIndex);
    if (slice->mbIndex == -1) {
      // End of sSlice group, MUST be end of slice
      assert (slice->nalStartcode (slice, eos_bit) == TRUE);
      return TRUE;
      }

    if (slice->nalStartcode (slice, eos_bit) == FALSE)
      return FALSE;

    if ((slice->sliceType == I_SLICE)  ||
        (slice->sliceType == SI_SLICE) ||
        (decoder->activePPS->entropyCodingMode == (Boolean)CABAC))
      return TRUE;

    if (slice->codCount <= 0)
      return TRUE;

    return FALSE;
   }
  }
//}}}

//{{{
static void interpretMbModeP (sMacroblock* mb) {

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
static void interpretMbModeI (sMacroblock* mb) {

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
static void interpretMbModeB (sMacroblock* mb) {

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
    memset (mb->b8mode, 0, 4 * sizeof(char));
    memset (mb->b8pdir, 2, 4 * sizeof(char));
    }
  else if (mbtype == 23) { // intra4x4
    mb->isIntraBlock = TRUE;
    mbmode = I4MB;
    memset (mb->b8mode, IBLOCK,4 * sizeof(char));
    memset (mb->b8pdir, -1,4 * sizeof(char));
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
static void interpretMbModeSI (sMacroblock* mb) {

  //sDecoder* decoder = mb->decoder;
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
static void readMotionInfoP (sMacroblock* mb){

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;

  sSyntaxElement se;
  sDataPartition *dataPartition = NULL;
  const byte *dpMap       = assignSE2dp[slice->datadpMode];
  short partmode        = ((mb->mbType == P8x8) ? 4 : mb->mbType);
  int step_h0         = BLOCK_STEP [partmode][0];
  int step_v0         = BLOCK_STEP [partmode][1];

  int j4;
  sPicture* picture = slice->picture;
  sPicMotionParam *mvInfo = NULL;

  int listOffset = mb->listOffset;
  sPicture** list0 = slice->listX[LIST_0 + listOffset];
  sPicMotionParam** p_mv_info = &picture->mvInfo[mb->blockY];

  //=====  READ REFERENCE PICTURE INDICES =====
  se.type = SE_REFFRAME;
  dataPartition = &(slice->dps[dpMap[SE_REFFRAME]]);

  //  For LIST_0, if multiple ref. pictures, read LIST_0 reference picture indices for the MB
  prepareListforRefIdx (mb, &se, dataPartition, slice->numRefIndexActive[LIST_0], (mb->mbType != P8x8) || (!slice->allrefzero));
  readMBRefPictureIdx  (&se, dataPartition, mb, p_mv_info, LIST_0, step_v0, step_h0);

  //=====  READ MOTION VECTORS =====
  se.type = SE_MVD;
  dataPartition = &(slice->dps[dpMap[SE_MVD]]);
  if (decoder->activePPS->entropyCodingMode == (Boolean) CAVLC || dataPartition->s->errorFlag)
    se.mapping = linfo_se;
  else
    se.reading = slice->mbAffFrameFlag ? read_mvd_CABAC_mbaff : read_MVD_CABAC;

  // LIST_0 Motion vectors
  readMBMotionVectors (&se, dataPartition, mb, LIST_0, step_h0, step_v0);

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
static void readMotionInfoB (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  sPicture* picture = slice->picture;
  sSyntaxElement se;
  sDataPartition* dataPartition = NULL;
  const byte* dpMap = assignSE2dp[slice->datadpMode];
  int partmode = (mb->mbType == P8x8) ? 4 : mb->mbType;
  int step_h0 = BLOCK_STEP [partmode][0];
  int step_v0 = BLOCK_STEP [partmode][1];
  int j4, i4;

  int listOffset = mb->listOffset;
  sPicture** list0 = slice->listX[LIST_0 + listOffset];
  sPicture** list1 = slice->listX[LIST_1 + listOffset];
  sPicMotionParam** p_mv_info = &picture->mvInfo[mb->blockY];

  if (mb->mbType == P8x8)
    slice->updateDirectMvInfo(mb);

  //=====  READ REFERENCE PICTURE INDICES =====
  se.type = SE_REFFRAME;
  dataPartition = &(slice->dps[dpMap[SE_REFFRAME]]);

  //  For LIST_0, if multiple ref. pictures, read LIST_0 reference picture indices for the MB
  prepareListforRefIdx (mb, &se, dataPartition, slice->numRefIndexActive[LIST_0], TRUE);
  readMBRefPictureIdx (&se, dataPartition, mb, p_mv_info, LIST_0, step_v0, step_h0);

  //  For LIST_1, if multiple ref. pictures, read LIST_1 reference picture indices for the MB
  prepareListforRefIdx (mb, &se, dataPartition, slice->numRefIndexActive[LIST_1], TRUE);
  readMBRefPictureIdx (&se, dataPartition, mb, p_mv_info, LIST_1, step_v0, step_h0);

  //=====  READ MOTION VECTORS =====
  se.type = SE_MVD;
  dataPartition = &(slice->dps[dpMap[SE_MVD]]);
  if (decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag)
    se.mapping = linfo_se;
  else
    se.reading = slice->mbAffFrameFlag ? read_mvd_CABAC_mbaff : read_MVD_CABAC;

  // LIST_0 Motion vectors
  readMBMotionVectors (&se, dataPartition, mb, LIST_0, step_h0, step_v0);

  // LIST_1 Motion vectors
  readMBMotionVectors (&se, dataPartition, mb, LIST_1, step_h0, step_v0);

  // record reference picture Ids for deblocking decisions
  for (j4 = 0; j4 < 4; ++j4) {
    for (i4 = mb->blockX; i4 < mb->blockX + 4; ++i4) {
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

  setReadMacroblock (slice);

  switch (slice->sliceType) {
    //{{{
    case P_SLICE:
      slice->interpretMbMode = interpretMbModeP;
      slice->nalReadMotionInfo = readMotionInfoP;
      slice->decodeOneComponent = decodeComponentP;
      slice->updateDirectMvInfo = NULL;
      slice->initLists = initListsSliceP;
      break;
    //}}}
    //{{{
    case SP_SLICE:
      slice->interpretMbMode = interpretMbModeP;
      slice->nalReadMotionInfo = readMotionInfoP;
      slice->decodeOneComponent = decodeComponentSP;
      slice->updateDirectMvInfo = NULL;
      slice->initLists = initListsSliceP;
      break;
    //}}}
    //{{{
    case B_SLICE:
      slice->interpretMbMode = interpretMbModeB;
      slice->nalReadMotionInfo = readMotionInfoB;
      slice->decodeOneComponent = decodeComponentB;
      update_direct_types (slice);
      slice->initLists  = initListsSliceB;
      break;
    //}}}
    //{{{
    case I_SLICE:
      slice->interpretMbMode = interpretMbModeI;
      slice->nalReadMotionInfo = NULL;
      slice->decodeOneComponent = decodeComponentI;
      slice->updateDirectMvInfo = NULL;
      slice->initLists = initListsSliceI;
      break;
    //}}}
    //{{{
    case SI_SLICE:
      slice->interpretMbMode = interpretMbModeSI;
      slice->nalReadMotionInfo = NULL;
      slice->decodeOneComponent = decodeComponentI;
      slice->updateDirectMvInfo = NULL;
      slice->initLists = initListsSliceI;
      break;
    //}}}
    //{{{
    default:
      printf ("Unsupported slice type\n");
      break;
    //}}}
    }

  set_intra_prediction_modes (slice);

  if (slice->decoder->activeSPS->chromaFormatIdc==YUV444 && (slice->decoder->coding.sepColourPlaneFlag == 0) )
    slice->readCoef4x4cavlc = readCoef4x4cavlc444;
  else
    slice->readCoef4x4cavlc = readCoef4x4cavlc;

  switch (slice->decoder->activePPS->entropyCodingMode) {
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

  int* mbSize = mb->decoder->mbSize[IS_LUMA];

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

  sDecoder* decoder = mb->decoder;
  sPixelPos up, left;
  decoder->getNeighbour (mb, -1,  0, decoder->mbSize[1], &left);
  decoder->getNeighbour (mb,  0, -1, decoder->mbSize[1], &up);

  if ((mb->isIntraBlock == FALSE) || (!(decoder->activePPS->constrainedIntraPredFlag)) ) {
    if (left.available)
      mb->dplFlag |= decoder->mbData[left.mbIndex].dplFlag;
    if (up.available)
      mb->dplFlag |= decoder->mbData[up.mbIndex].dplFlag;
    }
  }
//}}}

//{{{
static void init_cur_imgy (sDecoder* decoder, sSlice* slice, int plane) {
// probably a better way (or place) to do this, but I'm not sure what (where) it is [CJV]
// this is intended to make get_block_luma faster, but I'm still performing
// this at the MB level, and it really should be done at the slice level

  if (decoder->coding.sepColourPlaneFlag == 0) {
    sPicture* vidref = decoder->noReferencePicture;
    int noref = (slice->framePoc < decoder->recoveryPoc);

    if (plane == PLANE_Y) {
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
            curRef->curPixelY = curRef->imgUV[plane-1];
            }
          }
        }
      }
    }
  }
//}}}
//{{{
void changePlaneJV (sDecoder* decoder, int nplane, sSlice* slice) {

  decoder->mbData = decoder->mbDataJV[nplane];
  decoder->picture  = decoder->decPictureJV[nplane];
  decoder->siBlock = decoder->siBlockJV[nplane];
  decoder->predMode = decoder->predModeJV[nplane];
  decoder->intraBlock = decoder->intraBlockJV[nplane];

  if (slice) {
    slice->mbData = decoder->mbDataJV[nplane];
    slice->picture  = decoder->decPictureJV[nplane];
    slice->siBlock = decoder->siBlockJV[nplane];
    slice->predMode = decoder->predModeJV[nplane];
    slice->intraBlock = decoder->intraBlockJV[nplane];
    }
  }
//}}}
//{{{
void makeFramePictureJV (sDecoder* decoder) {

  decoder->picture = decoder->decPictureJV[0];

  // copy;
  if (decoder->picture->usedForReference) {
    int nsize = (decoder->picture->sizeY/BLOCK_SIZE)*(decoder->picture->sizeX/BLOCK_SIZE)*sizeof(sPicMotionParam);
    memcpy (&(decoder->picture->JVmv_info[PLANE_Y][0][0]), &(decoder->decPictureJV[PLANE_Y]->mvInfo[0][0]), nsize);
    memcpy (&(decoder->picture->JVmv_info[PLANE_U][0][0]), &(decoder->decPictureJV[PLANE_U]->mvInfo[0][0]), nsize);
    memcpy (&(decoder->picture->JVmv_info[PLANE_V][0][0]), &(decoder->decPictureJV[PLANE_V]->mvInfo[0][0]), nsize);
    }

  // This could be done with pointers and seems not necessary
  for (int uv = 0; uv < 2; uv++) {
    for (int line = 0; line < decoder->height; line++) {
      int nsize = sizeof(sPixel) * decoder->width;
      memcpy (decoder->picture->imgUV[uv][line], decoder->decPictureJV[uv+1]->imgY[line], nsize );
      }
    freePicture (decoder->decPictureJV[uv+1]);
    }
  }
//}}}

//{{{
int decodeMacroblock (sMacroblock* mb, sPicture* picture) {

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  if (slice->chroma444notSeparate) {
    if (!mb->isIntraBlock) {
      init_cur_imgy (decoder, slice, PLANE_Y);
      slice->decodeOneComponent (mb, PLANE_Y, picture->imgY, picture);
      init_cur_imgy (decoder, slice, PLANE_U);
      slice->decodeOneComponent (mb, PLANE_U, picture->imgUV[0], picture);
      init_cur_imgy (decoder, slice, PLANE_V);
      slice->decodeOneComponent (mb, PLANE_V, picture->imgUV[1], picture);
      }
    else {
      slice->decodeOneComponent (mb, PLANE_Y, picture->imgY, picture);
      slice->decodeOneComponent (mb, PLANE_U, picture->imgUV[0], picture);
      slice->decodeOneComponent (mb, PLANE_V, picture->imgUV[1], picture);
      }
    slice->isResetCoef = FALSE;
    slice->isResetCoefCr = FALSE;
    }
  else
    slice->decodeOneComponent(mb, PLANE_Y, picture->imgY, picture);

  return 0;
  }
//}}}
