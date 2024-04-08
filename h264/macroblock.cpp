//{{{  includes
#include "global.h"
#include "memory.h"

#include "cCabacDecode.h"
#include "cabac.h"
#include "cabacRead.h"
#include "cavlcRead.h"
#include "fmo.h"
#include "intraPred.h"
#include "macroblock.h"
#include "mcPred.h"
#include "transform.h"

#include <math.h>
//}}}
namespace {
  static const sMotionVec kZeroMv = {0, 0};
  //{{{
  const uint8_t QP_SCALE_CR[52] = {
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
     12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
     28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
     37,38,38,38,39,39,39,39

  };
  //}}}
  //{{{
  const uint8_t decode_block_scan[16] = {
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15
    };
  //}}}

  //{{{
  void getMotionVectorPredictorMBAFF (sMacroBlock* mb, sPixelPos* block,
                                      sMotionVec *pmv, int16_t  ref_frame, sPicMotion** mvInfo,
                                      int list, int mb_x, int mb_y, int blockshape_x, int blockshape_y) {

    int mv_a, mv_b, mv_c, pred_vec=0;
    int mvPredType, rFrameL, rFrameU, rFrameUR;
    int hv;
    cDecoder264* decoder = mb->decoder;
    mvPredType = MVPRED_MEDIAN;

    if (mb->mbField) {
      rFrameL  = block[0].ok
        ? (decoder->mbData[block[0].mbIndex].mbField
        ? mvInfo[block[0].posY][block[0].posX].refIndex[list]
      : mvInfo[block[0].posY][block[0].posX].refIndex[list] * 2) : -1;
      rFrameU  = block[1].ok
        ? (decoder->mbData[block[1].mbIndex].mbField
        ? mvInfo[block[1].posY][block[1].posX].refIndex[list]
      : mvInfo[block[1].posY][block[1].posX].refIndex[list] * 2) : -1;
      rFrameUR = block[2].ok
        ? (decoder->mbData[block[2].mbIndex].mbField
        ? mvInfo[block[2].posY][block[2].posX].refIndex[list]
      : mvInfo[block[2].posY][block[2].posX].refIndex[list] * 2) : -1;
      }
    else {
      rFrameL = block[0].ok
        ? (decoder->mbData[block[0].mbIndex].mbField
        ? mvInfo[block[0].posY][block[0].posX].refIndex[list] >>1
        : mvInfo[block[0].posY][block[0].posX].refIndex[list]) : -1;
      rFrameU  = block[1].ok
        ? (decoder->mbData[block[1].mbIndex].mbField
        ? mvInfo[block[1].posY][block[1].posX].refIndex[list] >>1
        : mvInfo[block[1].posY][block[1].posX].refIndex[list]) : -1;
      rFrameUR = block[2].ok
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
        mv_a = block[0].ok ? mvInfo[block[0].posY][block[0].posX].mv[list].mvX : 0;
        mv_b = block[1].ok ? mvInfo[block[1].posY][block[1].posX].mv[list].mvX : 0;
        mv_c = block[2].ok ? mvInfo[block[2].posY][block[2].posX].mv[list].mvX : 0;
        }
      else {
        if (mb->mbField) {
          mv_a = block[0].ok  ? decoder->mbData[block[0].mbIndex].mbField
            ? mvInfo[block[0].posY][block[0].posX].mv[list].mvY
          : mvInfo[block[0].posY][block[0].posX].mv[list].mvY / 2
            : 0;
          mv_b = block[1].ok  ? decoder->mbData[block[1].mbIndex].mbField
            ? mvInfo[block[1].posY][block[1].posX].mv[list].mvY
          : mvInfo[block[1].posY][block[1].posX].mv[list].mvY / 2
            : 0;
          mv_c = block[2].ok  ? decoder->mbData[block[2].mbIndex].mbField
            ? mvInfo[block[2].posY][block[2].posX].mv[list].mvY
          : mvInfo[block[2].posY][block[2].posX].mv[list].mvY / 2
            : 0;
          }
        else {
          mv_a = block[0].ok  ? decoder->mbData[block[0].mbIndex].mbField
            ? mvInfo[block[0].posY][block[0].posX].mv[list].mvY * 2
            : mvInfo[block[0].posY][block[0].posX].mv[list].mvY
          : 0;
          mv_b = block[1].ok  ? decoder->mbData[block[1].mbIndex].mbField
            ? mvInfo[block[1].posY][block[1].posX].mv[list].mvY * 2
            : mvInfo[block[1].posY][block[1].posX].mv[list].mvY
          : 0;
          mv_c = block[2].ok  ? decoder->mbData[block[2].mbIndex].mbField
            ? mvInfo[block[2].posY][block[2].posX].mv[list].mvY * 2
            : mvInfo[block[2].posY][block[2].posX].mv[list].mvY
          : 0;
          }
        }

      switch (mvPredType) {
        case MVPRED_MEDIAN:
          if (!(block[1].ok || block[2].ok))
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
        pmv->mvX = (int16_t)pred_vec;
      else
        pmv->mvY = (int16_t)pred_vec;
      }
    }
  //}}}
  //{{{
  void getMotionVectorPredictorNormal (sMacroBlock* mb, sPixelPos* block,
                                       sMotionVec *pmv, int16_t  ref_frame, sPicMotion** mvInfo,
                                       int list, int mb_x, int mb_y, int blockshape_x, int blockshape_y) {
    int mvPredType = MVPRED_MEDIAN;

    int rFrameL = block[0].ok ? mvInfo[block[0].posY][block[0].posX].refIndex[list] : -1;
    int rFrameU = block[1].ok ? mvInfo[block[1].posY][block[1].posX].refIndex[list] : -1;
    int rFrameUR = block[2].ok ? mvInfo[block[2].posY][block[2].posX].refIndex[list] : -1;

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
        if(!(block[1].ok || block[2].ok))
        {
          if (block[0].ok)
            *pmv = mvInfo[block[0].posY][block[0].posX].mv[list];
          else
            *pmv = kZeroMv;
        }
        else
        {
          sMotionVec *mv_a = block[0].ok ? &mvInfo[block[0].posY][block[0].posX].mv[list] : (sMotionVec *) &kZeroMv;
          sMotionVec *mv_b = block[1].ok ? &mvInfo[block[1].posY][block[1].posX].mv[list] : (sMotionVec *) &kZeroMv;
          sMotionVec *mv_c = block[2].ok ? &mvInfo[block[2].posY][block[2].posX].mv[list] : (sMotionVec *) &kZeroMv;

          pmv->mvX = (int16_t)imedian (mv_a->mvX, mv_b->mvX, mv_c->mvX);
          pmv->mvY = (int16_t)imedian (mv_a->mvY, mv_b->mvY, mv_c->mvY);
        }
        break;
      //}}}
      //{{{
      case MVPRED_L:
        if (block[0].ok)
          *pmv = mvInfo[block[0].posY][block[0].posX].mv[list];
        else
          *pmv = kZeroMv;
        break;
      //}}}
      //{{{
      case MVPRED_U:
        if (block[1].ok)
          *pmv = mvInfo[block[1].posY][block[1].posX].mv[list];
        else
          *pmv = kZeroMv;
        break;
      //}}}
      //{{{
      case MVPRED_UR:
        if (block[2].ok)
          *pmv = mvInfo[block[2].posY][block[2].posX].mv[list];
        else
          *pmv = kZeroMv;
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
  void initMotionVectorPrediction (sMacroBlock* mb, int mbAffFrame) {

    if (mbAffFrame)
      mb->GetMVPredictor = getMotionVectorPredictorMBAFF;
    else
      mb->GetMVPredictor = getMotionVectorPredictorNormal;
    }
  //}}}

  //{{{  transform utils
  //{{{
  void copyImage (sPixel** imgBuf1, sPixel** imgBuf2, int off1, int off2, int width, int height) {

    for (int j = 0; j < height; ++j)
      memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), width * sizeof (sPixel));
    }
  //}}}
  //{{{
  void sample_reconstruct (sPixel** curImg, sPixel** mpr, int** mbRess, int mb_x,
                                  int opix_x, int width, int height, int max_imgpel_value, int dq_bits) {

    for (int j = 0; j < height; j++) {
      sPixel* imgOrg = &curImg[j][opix_x];
      sPixel* imgPred = &mpr[j][mb_x];
      int* m7 = &mbRess[j][mb_x];
      for (int i = 0; i < width; i++)
        *imgOrg++ = (sPixel) iClip1 (max_imgpel_value, rshift_rnd_sf(*m7++, dq_bits) + *imgPred++);
      }
    }
  //}}}
  //{{{
  void invResidualTrans4x4 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

    cSlice* slice = mb->slice;
    sPixel** mbPred = slice->mbPred[plane];
    sPixel** mbRec = slice->mbRec[plane];
    int** mbRess = slice->mbRess[plane];
    int** cof = slice->cof[plane];

    int temp[4][4];
    if (mb->dpcmMode == VERT_PRED) {
      for (int i = 0; i<4; ++i) {
        temp[0][i] = cof[joff + 0][ioff + i];
        temp[1][i] = cof[joff + 1][ioff + i] + temp[0][i];
        temp[2][i] = cof[joff + 2][ioff + i] + temp[1][i];
        temp[3][i] = cof[joff + 3][ioff + i] + temp[2][i];
        }

      for (int i = 0; i < 4; ++i) {
        mbRess[joff    ][ioff + i]=temp[0][i];
        mbRess[joff + 1][ioff + i]=temp[1][i];
        mbRess[joff + 2][ioff + i]=temp[2][i];
        mbRess[joff + 3][ioff + i]=temp[3][i];
        }
      }

    else if (mb->dpcmMode == HOR_PRED) {
      for (int j = 0; j < 4; ++j) {
        temp[j][0] = cof[joff + j][ioff    ];
        temp[j][1] = cof[joff + j][ioff + 1] + temp[j][0];
        temp[j][2] = cof[joff + j][ioff + 2] + temp[j][1];
        temp[j][3] = cof[joff + j][ioff + 3] + temp[j][2];
        }

      for (int j = 0; j < 4; ++j) {
        mbRess[joff + j][ioff    ]=temp[j][0];
        mbRess[joff + j][ioff + 1]=temp[j][1];
        mbRess[joff + j][ioff + 2]=temp[j][2];
        mbRess[joff + j][ioff + 3]=temp[j][3];
        }
      }

    else
      for (int j = joff; j < joff + BLOCK_SIZE; ++j)
        for (int i = ioff; i < ioff + BLOCK_SIZE; ++i)
          mbRess[j][i] = cof[j][i];

    for (int j = joff; j < joff + BLOCK_SIZE; ++j)
      for (int i = ioff; i < ioff + BLOCK_SIZE; ++i)
        mbRec[j][i] = (sPixel) (mbRess[j][i] + mbPred[j][i]);
    }
  //}}}
  //{{{
  void invResidualTrans8x8 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

    cSlice* slice = mb->slice;
    sPixel** mbPred = slice->mbPred[plane];
    sPixel** mbRec  = slice->mbRec[plane];
    int** mbRess = slice->mbRess[plane];

    int temp[8][8];
    if (mb->dpcmMode == VERT_PRED) {
      for (int i = 0; i < 8; ++i) {
        temp[0][i] = mbRess[joff + 0][ioff + i];
        temp[1][i] = mbRess[joff + 1][ioff + i] + temp[0][i];
        temp[2][i] = mbRess[joff + 2][ioff + i] + temp[1][i];
        temp[3][i] = mbRess[joff + 3][ioff + i] + temp[2][i];
        temp[4][i] = mbRess[joff + 4][ioff + i] + temp[3][i];
        temp[5][i] = mbRess[joff + 5][ioff + i] + temp[4][i];
        temp[6][i] = mbRess[joff + 6][ioff + i] + temp[5][i];
        temp[7][i] = mbRess[joff + 7][ioff + i] + temp[6][i];
        }
      for (int i = 0; i < 8; ++i) {
        mbRess[joff  ][ioff+i]=temp[0][i];
        mbRess[joff+1][ioff+i]=temp[1][i];
        mbRess[joff+2][ioff+i]=temp[2][i];
        mbRess[joff+3][ioff+i]=temp[3][i];
        mbRess[joff+4][ioff+i]=temp[4][i];
        mbRess[joff+5][ioff+i]=temp[5][i];
        mbRess[joff+6][ioff+i]=temp[6][i];
        mbRess[joff+7][ioff+i]=temp[7][i];
        }
      }
    else if (mb->dpcmMode == HOR_PRED) {
      //HOR_PRED
      for (int i = 0; i < 8; ++i) {
        temp[i][0] = mbRess[joff + i][ioff + 0];
        temp[i][1] = mbRess[joff + i][ioff + 1] + temp[i][0];
        temp[i][2] = mbRess[joff + i][ioff + 2] + temp[i][1];
        temp[i][3] = mbRess[joff + i][ioff + 3] + temp[i][2];
        temp[i][4] = mbRess[joff + i][ioff + 4] + temp[i][3];
        temp[i][5] = mbRess[joff + i][ioff + 5] + temp[i][4];
        temp[i][6] = mbRess[joff + i][ioff + 6] + temp[i][5];
        temp[i][7] = mbRess[joff + i][ioff + 7] + temp[i][6];
        }
      for (int i = 0; i < 8; ++i) {
        mbRess[joff+i][ioff+0]=temp[i][0];
        mbRess[joff+i][ioff+1]=temp[i][1];
        mbRess[joff+i][ioff+2]=temp[i][2];
        mbRess[joff+i][ioff+3]=temp[i][3];
        mbRess[joff+i][ioff+4]=temp[i][4];
        mbRess[joff+i][ioff+5]=temp[i][5];
        mbRess[joff+i][ioff+6]=temp[i][6];
        mbRess[joff+i][ioff+7]=temp[i][7];
        }
      }

    for (int j = joff; j < joff + BLOCK_SIZE*2; ++j)
      for (int i = ioff; i < ioff + BLOCK_SIZE*2; ++i)
        mbRec [j][i] = (sPixel) (mbRess[j][i] + mbPred[j][i]);
    }
  //}}}
  //{{{
  void invResidualTrans16x16 (sMacroBlock* mb, eColorPlane plane) {

    cSlice* slice = mb->slice;
    sPixel** mbPred = slice->mbPred[plane];
    sPixel** mbRec = slice->mbRec[plane];
    int** mbRess = slice->mbRess[plane];
    int** cof = slice->cof[plane];

    int temp[16][16];
    if (mb->dpcmMode == VERT_PRED_16) {
      for (int i = 0; i < MB_BLOCK_SIZE; ++i) {
        temp[0][i] = cof[0][i];
        for (int j = 1; j < MB_BLOCK_SIZE; j++)
          temp[j][i] = cof[j][i] + temp[j-1][i];
        }

      for (int i = 0; i < MB_BLOCK_SIZE; ++i)
        for (int j = 0; j < MB_BLOCK_SIZE; j++)
          mbRess[j][i]=temp[j][i];
      }

    else if (mb->dpcmMode == HOR_PRED_16) {
      for (int j = 0; j < MB_BLOCK_SIZE; ++j) {
        temp[j][ 0] = cof[j][ 0  ];
        for (int i = 1; i < MB_BLOCK_SIZE; i++)
          temp[j][i] = cof[j][i] + temp[j][i-1];
        }

      for (int j = 0; j < MB_BLOCK_SIZE; ++j)
        for (int i = 0; i < MB_BLOCK_SIZE; ++i)
          mbRess[j][i]=temp[j][i];
      }

    else {
      for (int j = 0; j < MB_BLOCK_SIZE; ++j)
        for (int i = 0; i < MB_BLOCK_SIZE; ++i)
          mbRess[j][i] = cof[j][i];
      }

    for (int j = 0; j < MB_BLOCK_SIZE; ++j)
      for (int i = 0; i < MB_BLOCK_SIZE; ++i)
        mbRec[j][i] = (sPixel) (mbRess[j][i] + mbPred[j][i]);
    }
  //}}}
  //{{{
  void iMBtrans4x4 (sMacroBlock* mb, eColorPlane plane, int smb) {

    cSlice* slice = mb->slice;
    sPicture* picture = mb->slice->picture;
    sPixel** curr_img = plane ? picture->imgUV[plane - 1] : picture->imgY;

    if (mb->isLossless && mb->mbType == I16MB)
      invResidualTrans16x16(mb, plane);
    else if (smb || mb->isLossless == true) {
      mb->iTrans4x4 = (smb) ? itransSp : ((mb->isLossless == false) ? itrans4x4 : invResidualTrans4x4);
      for (int block8x8 = 0; block8x8 < MB_BLOCK_SIZE; block8x8 += 4) {
        for (int k = block8x8; k < block8x8 + 4; ++k ) {
          int jj = ((decode_block_scan[k] >> 2) & 3) << BLOCK_SHIFT;
          int ii = (decode_block_scan[k] & 3) << BLOCK_SHIFT;
          // use integer transform and make 4x4 block mbRess from prediction block mbPred
          mb->iTrans4x4 (mb, plane, ii, jj);
          }
        }
      }
    else {
      int** cof = slice->cof[plane];
      int** mbRess = slice->mbRess[plane];

      if (mb->isIntraBlock == false) {
        if (mb->codedBlockPattern & 0x01) {
          inverse4x4 (cof, mbRess, 0, 0);
          inverse4x4 (cof, mbRess, 0, 4);
          inverse4x4 (cof, mbRess, 4, 0);
          inverse4x4 (cof, mbRess, 4, 4);
          }
        if (mb->codedBlockPattern & 0x02) {
          inverse4x4 (cof, mbRess, 0, 8);
          inverse4x4 (cof, mbRess, 0, 12);
          inverse4x4 (cof, mbRess, 4, 8);
          inverse4x4 (cof, mbRess, 4, 12);
          }
        if (mb->codedBlockPattern & 0x04) {
          inverse4x4 (cof, mbRess, 8, 0);
          inverse4x4 (cof, mbRess, 8, 4);
          inverse4x4 (cof, mbRess, 12, 0);
          inverse4x4 (cof, mbRess, 12, 4);
          }
        if (mb->codedBlockPattern & 0x08) {
          inverse4x4 (cof, mbRess, 8, 8);
          inverse4x4 (cof, mbRess, 8, 12);
          inverse4x4 (cof, mbRess, 12, 8);
          inverse4x4 (cof, mbRess, 12, 12);
          }
        }
      else {
        for (int jj = 0; jj < MB_BLOCK_SIZE; jj += BLOCK_SIZE) {
          inverse4x4 (cof, mbRess, jj, 0);
          inverse4x4 (cof, mbRess, jj, 4);
          inverse4x4 (cof, mbRess, jj, 8);
          inverse4x4 (cof, mbRess, jj, 12);
          }
        }
      sample_reconstruct (slice->mbRec[plane], slice->mbPred[plane], mbRess, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE, mb->decoder->coding.maxPelValueComp[plane], DQ_BITS);
      }

    // construct picture from 4x4 blocks
    copyImage16x16 (&curr_img[mb->pixY], slice->mbRec[plane], mb->pixX, 0);
    }
  //}}}
  //{{{
  void iMBtrans8x8 (sMacroBlock* mb, eColorPlane plane) {

    //cDecoder264* decoder = mb->decoder;
    sPicture* picture = mb->slice->picture;
    sPixel** curr_img = plane ? picture->imgUV[plane - 1]: picture->imgY;

    // Perform 8x8 idct
    if (mb->codedBlockPattern & 0x01)
      itrans8x8 (mb, plane, 0, 0);
    else
      icopy8x8 (mb, plane, 0, 0);

    if (mb->codedBlockPattern & 0x02)
      itrans8x8 (mb, plane, 8, 0);
    else
      icopy8x8 (mb, plane, 8, 0);

    if (mb->codedBlockPattern & 0x04)
      itrans8x8 (mb, plane, 0, 8);
    else
      icopy8x8 (mb, plane, 0, 8);

    if (mb->codedBlockPattern & 0x08)
      itrans8x8 (mb, plane, 8, 8);
    else
      icopy8x8 (mb, plane, 8, 8);

    copyImage16x16 (&curr_img[mb->pixY], mb->slice->mbRec[plane], mb->pixX, 0);
    }
  //}}}
  //}}}
  //{{{  mbPred functions
  //{{{
  void setChromaVector (sMacroBlock* mb)
  {
    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;

    if (!slice->mbAffFrame) {
      if(slice->picStructure == eTopField) {
        int k,l;
        for (l = LIST_0; l <= (LIST_1); l++) {
          for(k = 0; k < slice->listXsize[l]; k++) {
            if(slice->picStructure != slice->listX[l][k]->picStructure)
              slice->chromaVectorAdjust[l][k] = -2;
            else
              slice->chromaVectorAdjust[l][k] = 0;
          }
        }
      }
      else if(slice->picStructure == eBotField) {
        int k,l;
        for (l = LIST_0; l <= (LIST_1); l++) {
          for(k = 0; k < slice->listXsize[l]; k++) {
            if (slice->picStructure != slice->listX[l][k]->picStructure)
              slice->chromaVectorAdjust[l][k] = 2;
            else
              slice->chromaVectorAdjust[l][k] = 0;
          }
        }
      }
      else {
        int k,l;
        for (l = LIST_0; l <= (LIST_1); l++) {
          for(k = 0; k < slice->listXsize[l]; k++)
            slice->chromaVectorAdjust[l][k] = 0;
        }
      }
    }
    else {
      int mb_nr = (mb->mbIndexX & 0x01);
      int k,l;

      // find out the correct list offsets
      if (mb->mbField) {
        int listOffset = mb->listOffset;

        for (l = LIST_0 + listOffset; l <= (LIST_1 + listOffset); l++) {
          for(k = 0; k < slice->listXsize[l]; k++) {
            if(mb_nr == 0 && slice->listX[l][k]->picStructure == eBotField)
              slice->chromaVectorAdjust[l][k] = -2;
            else if(mb_nr == 1 && slice->listX[l][k]->picStructure == eTopField)
              slice->chromaVectorAdjust[l][k] = 2;
            else
              slice->chromaVectorAdjust[l][k] = 0;
          }
        }
      }
      else {
        for (l = LIST_0; l <= (LIST_1); l++)
          for(k = 0; k < slice->listXsize[l]; k++)
            slice->chromaVectorAdjust[l][k] = 0;
      }
    }

    slice->maxMbVmvR = (slice->picStructure != eFrame || ( mb->mbField )) ? decoder->coding.maxVmvR >> 1 :
                                                                           decoder->coding.maxVmvR;
  }
  //}}}
  //{{{
  void updateNeighbourMvs (sPicMotion** motion, const sPicMotion* mvInfo, int i4)
  {
    (*motion++)[i4 + 1] = *mvInfo;
    (*motion  )[i4    ] = *mvInfo;
    (*motion  )[i4 + 1] = *mvInfo;
  }
  //}}}
  //{{{
  int mbPredIntra4x4 (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
  {
    cSlice* slice = mb->slice;
    int yuv = picture->chromaFormatIdc - 1;
    int i=0, j=0,k, j4=0,i4=0;
    int j_pos, i_pos;
    int ioff,joff;
    int block8x8;   // needed for ABT
    mb->iTrans4x4 = (mb->isLossless == false) ? itrans4x4 : invResidualTrans4x4;

    for (block8x8 = 0; block8x8 < 4; block8x8++)
    {
      for (k = block8x8 * 4; k < block8x8 * 4 + 4; k ++)
      {
        i =  (decode_block_scan[k] & 3);
        j = ((decode_block_scan[k] >> 2) & 3);

        ioff = (i << 2);
        joff = (j << 2);
        i4   = mb->blockX + i;
        j4   = mb->blockY + j;
        j_pos = j4 * BLOCK_SIZE;
        i_pos = i4 * BLOCK_SIZE;

        // PREDICTION
        //===== INTRA PREDICTION =====
        if (slice->intraPred4x4 (mb, plane, ioff,joff,i4,j4) == eSearchSync)  /* make 4x4 prediction block mpr from given prediction decoder->mb_mode */
          return eSearchSync;                   /* bit error */
        // =============== 4x4 itrans ================
        // -------------------------------------------
        mb->iTrans4x4  (mb, plane, ioff, joff);
        copyImage4x4 (&pixel[j_pos], &slice->mbRec[plane][joff], i_pos, ioff);
      }
    }

    // chroma decoding
    if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444))
      intraChromaDecode(mb, yuv);

    if (mb->codedBlockPattern != 0)
      slice->isResetCoef = false;
    return 1;
  }
  //}}}
  //{{{
  int mbPredIntra16x16 (sMacroBlock* mb, eColorPlane plane, sPicture* picture) {

    int yuv = picture->chromaFormatIdc - 1;

    mb->slice->intraPred16x16(mb, plane, mb->i16mode);
    mb->dpcmMode = (char) mb->i16mode; // For residual DPCM

    // 4x4 itrans
    iMBtrans4x4 (mb, plane, 0);

    // chroma decoding
    if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444))
      intraChromaDecode(mb, yuv);

    mb->slice->isResetCoef = false;
    return 1;
    }
  //}}}
  //{{{
  int mbPredIntra8x8 (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
  {
    cSlice* slice = mb->slice;
    int yuv = picture->chromaFormatIdc - 1;

    int block8x8;   // needed for ABT
    mb->iTrans8x8 = (mb->isLossless == false) ? itrans8x8 : invResidualTrans8x8;

    for (block8x8 = 0; block8x8 < 4; block8x8++) {
      // 8x8 BLOCK TYPE
      int ioff = (block8x8 & 0x01) << 3;
      int joff = (block8x8 >> 1  ) << 3;

      // PREDICTION
      slice->intraPred8x8(mb, plane, ioff, joff);
      if (mb->codedBlockPattern & (1 << block8x8))
        mb->iTrans8x8    (mb, plane, ioff,joff);      // use inverse integer transform and make 8x8 block m7 from prediction block mpr
      else
        icopy8x8(mb, plane, ioff,joff);

      copyImage8x8(&pixel[mb->pixY + joff], &slice->mbRec[plane][joff], mb->pixX + ioff, ioff);
    }
    // chroma decoding** *****************************************************
    if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444))
      intraChromaDecode(mb, yuv);

    if (mb->codedBlockPattern != 0)
      slice->isResetCoef = false;
    return 1;
  }
  //}}}
  //{{{
  int mbPredSkip (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture) {

    cDecoder264* decoder = mb->decoder;

    cSlice* slice = mb->slice;
    setChromaVector (mb);

    performMotionCompensation (mb, plane, picture, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

    copyImage16x16 (&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);

    if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
      copyImage (&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      copyImage (&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      }

    return 1;
    }
  //}}}
  //{{{
  int mbPredSpSkip (sMacroBlock* mb, eColorPlane plane, sPicture* picture) {

    setChromaVector (mb);

    performMotionCompensation (mb, plane, picture, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
    iTransform (mb, plane, 1);
    return 1;
    }
  //}}}
  //{{{
  int mbPredPinter8x8 (sMacroBlock* mb, eColorPlane plane, sPicture* picture) {

    int block8x8;   // needed for ABT
    int i = 0, j = 0, k;

    cSlice* slice = mb->slice;
    int smb = slice->sliceType == eSliceSP && (mb->isIntraBlock == false);

    setChromaVector (mb);

    for (block8x8 = 0; block8x8 < 4; block8x8++) {
      int mv_mode = mb->b8mode[block8x8];
      int predDir = mb->b8pdir[block8x8];

      int k_start = (block8x8 << 2);
      int k_inc = (mv_mode == SMB8x4) ? 2 : 1;
      int k_end = (mv_mode == SMB8x8) ? k_start + 1 : ((mv_mode == SMB4x4) ? k_start + 4 : k_start + k_inc + 1);

      int blockSizeX = (mv_mode == SMB8x4 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;
      int blockSizeY = (mv_mode == SMB4x8 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;

      for (k = k_start; k < k_end; k += k_inc) {
        i =  (decode_block_scan[k] & 3);
        j = ((decode_block_scan[k] >> 2) & 3);
        performMotionCompensation (mb, plane, picture, predDir, i, j, blockSizeX, blockSizeY);
        }
      }

    iTransform (mb, plane, smb);

    if (mb->codedBlockPattern != 0)
      slice->isResetCoef = false;

    return 1;
    }
  //}}}
  //{{{
  int mbPredPinter16x16 (sMacroBlock* mb, eColorPlane plane, sPicture* picture) {

    cSlice* slice = mb->slice;
    int smb = (slice->sliceType == eSliceSP);

    setChromaVector(mb);
    performMotionCompensation (mb, plane, picture, mb->b8pdir[0], 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
    iTransform (mb, plane, smb);

    if (mb->codedBlockPattern != 0)
      slice->isResetCoef = false;

    return 1;
    }
  //}}}
  //{{{
  int mbPredPinter16x8 (sMacroBlock* mb, eColorPlane plane, sPicture* picture) {

    cSlice* slice = mb->slice;
    int smb = (slice->sliceType == eSliceSP);

    setChromaVector (mb);

    performMotionCompensation (mb, plane, picture, mb->b8pdir[0], 0, 0, MB_BLOCK_SIZE, BLOCK_SIZE_8x8);
    performMotionCompensation (mb, plane, picture, mb->b8pdir[2], 0, 2, MB_BLOCK_SIZE, BLOCK_SIZE_8x8);
    iTransform (mb, plane, smb);

    if (mb->codedBlockPattern != 0)
      slice->isResetCoef = false;

    return 1;
    }
  //}}}
  //{{{
  int mbPredPinter8x16 (sMacroBlock* mb, eColorPlane plane, sPicture* picture) {

    cSlice* slice = mb->slice;
    int smb = (slice->sliceType == eSliceSP);

    setChromaVector (mb);

    performMotionCompensation (mb, plane, picture, mb->b8pdir[0], 0, 0, BLOCK_SIZE_8x8, MB_BLOCK_SIZE);
    performMotionCompensation (mb, plane, picture, mb->b8pdir[1], 2, 0, BLOCK_SIZE_8x8, MB_BLOCK_SIZE);
    iTransform (mb, plane, smb);

    if (mb->codedBlockPattern != 0)
      slice->isResetCoef = false;

    return 1;
    }
  //}}}
  //{{{
  int mbPredBd8x8temporal (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture) {

    int16_t refIndex;
    int refList;

    int k, i, j, i4, j4, j6;
    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;
    sPicMotion* mvInfo = NULL, *colocated = NULL;

    int listOffset = mb->listOffset;
    sPicture** list0 = slice->listX[LIST_0 + listOffset];
    sPicture** list1 = slice->listX[LIST_1 + listOffset];

    setChromaVector(mb);

    for (int block8x8 = 0; block8x8 < 4; block8x8++) {
      int predDir = mb->b8pdir[block8x8];
      int k_start = (block8x8 << 2);
      int k_end = k_start + 1;
      for (k = k_start; k < k_start + BLOCK_MULTIPLE; k ++) {
        i =  (decode_block_scan[k] & 3);
        j = ((decode_block_scan[k] >> 2) & 3);
        i4 = mb->blockX + i;
        j4 = mb->blockY + j;
        j6 = mb->blockYaff + j;
        mvInfo = &picture->mvInfo[j4][i4];
        colocated = &list1[0]->mvInfo[RSD(j6)][RSD(i4)];
        if(mb->decoder->coding.isSeperateColourPlane && mb->decoder->coding.yuvFormat==YUV444)
          colocated = &list1[0]->mvInfoJV[mb->slice->colourPlaneId][RSD(j6)][RSD(i4)];
        if (slice->mbAffFrame) {
          if (!mb->mbField && ((slice->listX[LIST_1][0]->codingType==eFrameMbPairCoding && slice->listX[LIST_1][0]->motion.mbField[mb->mbIndexX]) ||
              (slice->listX[LIST_1][0]->codingType==eFieldCoding))) {
            if (iabs(picture->poc - slice->listX[LIST_1+4][0]->poc) > iabs(picture->poc -slice->listX[LIST_1+2][0]->poc))
              colocated = decoder->activeSps->isDirect8x8inference
                            ? &slice->listX[LIST_1+2][0]->mvInfo[RSD(j6) >> 1][RSD(i4)]
                            : &slice->listX[LIST_1+2][0]->mvInfo[j6 >> 1][i4];
            else
              colocated = decoder->activeSps->isDirect8x8inference
                            ? &slice->listX[LIST_1+4][0]->mvInfo[RSD(j6) >> 1][RSD(i4)]
                            : &slice->listX[LIST_1+4][0]->mvInfo[j6 >> 1][i4];
            }
          }
        else if (!decoder->activeSps->frameMbOnly &&
                 (!slice->fieldPic && slice->listX[LIST_1][0]->codingType != eFrameCoding)) {
          if (iabs (picture->poc - list1[0]->botField->poc)> iabs(picture->poc -list1[0]->topField->poc) )
            colocated = decoder->activeSps->isDirect8x8inference ?
              &list1[0]->topField->mvInfo[RSD(j6)>>1][RSD(i4)] : &list1[0]->topField->mvInfo[j6>>1][i4];
          else
            colocated = decoder->activeSps->isDirect8x8inference ?
              &list1[0]->botField->mvInfo[RSD(j6)>>1][RSD(i4)] : &list1[0]->botField->mvInfo[j6>>1][i4];
          }
        else if (!decoder->activeSps->frameMbOnly && slice->fieldPic &&
                 slice->picStructure != list1[0]->picStructure && list1[0]->codedFrame) {
          if (slice->picStructure == eTopField)
            colocated = decoder->activeSps->isDirect8x8inference
                          ? &list1[0]->frame->topField->mvInfo[RSD(j6)][RSD(i4)]
                          : &list1[0]->frame->topField->mvInfo[j6][i4];
          else
            colocated = decoder->activeSps->isDirect8x8inference
                          ? &list1[0]->frame->botField->mvInfo[RSD(j6)][RSD(i4)]
                          : &list1[0]->frame->botField->mvInfo[j6][i4];
          }

        refList = colocated->refIndex[LIST_0] == -1 ? LIST_1 : LIST_0;
        refIndex =  colocated->refIndex[refList];
        if (refIndex == -1) {
          // co-located is intra mode
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_0] = 0;
          mvInfo->refIndex[LIST_1] = 0;
          }
        else {
          // co-located skip or inter mode
          int mapped_idx = 0;
          int iref;
          if ((slice->mbAffFrame &&
              ((mb->mbField && colocated->refPic[refList]->picStructure == eFrame) ||
               (!mb->mbField && colocated->refPic[refList]->picStructure != eFrame))) ||
               (!slice->mbAffFrame && ((slice->fieldPic == 0 &&
                 colocated->refPic[refList]->picStructure != eFrame) ||
               (slice->fieldPic==1 && colocated->refPic[refList]->picStructure == eFrame)))) {
            for (iref = 0; iref < imin(slice->numRefIndexActive[LIST_0], slice->listXsize[LIST_0 + listOffset]);iref++) {
              if(slice->listX[LIST_0 + listOffset][iref]->topField == colocated->refPic[refList] ||
                slice->listX[LIST_0 + listOffset][iref]->botField == colocated->refPic[refList] ||
                slice->listX[LIST_0 + listOffset][iref]->frame == colocated->refPic[refList]) {
                if ((slice->fieldPic == 1) && (slice->listX[LIST_0 + listOffset][iref]->picStructure != slice->picStructure))
                  mapped_idx = INVALIDINDEX;
                else {
                  mapped_idx = iref;
                  break;
                  }
                }
              else
                mapped_idx = INVALIDINDEX;
              }
            }
          else {
            for (iref = 0; iref < imin(slice->numRefIndexActive[LIST_0], slice->listXsize[LIST_0 + listOffset]);iref++) {
              if(slice->listX[LIST_0 + listOffset][iref] == colocated->refPic[refList]) {
                mapped_idx = iref;
                break;
                }
              else
                mapped_idx=INVALIDINDEX;
              }
            }

          if (INVALIDINDEX != mapped_idx) {
            int mv_scale = slice->mvscale[LIST_0 + listOffset][mapped_idx];
            int mvY = colocated->mv[refList].mvY;
            if ((slice->mbAffFrame && !mb->mbField && colocated->refPic[refList]->picStructure != eFrame) ||
                (!slice->mbAffFrame && slice->fieldPic == 0 &&
                 colocated->refPic[refList]->picStructure != eFrame) )
              mvY *= 2;
            else if ((slice->mbAffFrame && mb->mbField && colocated->refPic[refList]->picStructure == eFrame) ||
                     (!slice->mbAffFrame && slice->fieldPic == 1 &&
                      colocated->refPic[refList]->picStructure == eFrame) )
              mvY /= 2;

            // In such case, an array is needed for each different reference.
            if (mv_scale == 9999 || slice->listX[LIST_0 + listOffset][mapped_idx]->usedLongTerm) {
              mvInfo->mv[LIST_0].mvX = colocated->mv[refList].mvX;
              mvInfo->mv[LIST_0].mvY = (int16_t) mvY;
              mvInfo->mv[LIST_1] = kZeroMv;
              }
            else {
              mvInfo->mv[LIST_0].mvX = (int16_t) ((mv_scale * colocated->mv[refList].mvX + 128 ) >> 8);
              mvInfo->mv[LIST_0].mvY = (int16_t) ((mv_scale * mvY/*colocated->mv[refList].mvY*/ + 128 ) >> 8);

              mvInfo->mv[LIST_1].mvX = (int16_t) (mvInfo->mv[LIST_0].mvX - colocated->mv[refList].mvX);
              mvInfo->mv[LIST_1].mvY = (int16_t) (mvInfo->mv[LIST_0].mvY - mvY/*colocated->mv[refList].mvY*/);
              }

            mvInfo->refIndex[LIST_0] = (char) mapped_idx; // colocated->refIndex[refList];
            mvInfo->refIndex[LIST_1] = 0;
            }
          else if (INVALIDINDEX == mapped_idx)
            cDecoder264::error ("temporal direct error: colocated block has ref that is unavailable");
          }

        // store reference picture ID determined by direct mode
        mvInfo->refPic[LIST_0] = list0[(int16_t)mvInfo->refIndex[LIST_0]];
        mvInfo->refPic[LIST_1] = list1[(int16_t)mvInfo->refIndex[LIST_1]];
        }

      for (k = k_start; k < k_end; k ++) {
        int i = decode_block_scan[k] & 3;
        int j = (decode_block_scan[k] >> 2) & 3;
        performMotionCompensation (mb, plane, picture, predDir, i, j, SMB_BLOCK_SIZE, SMB_BLOCK_SIZE);
        }
      }

    if (mb->codedBlockPattern == 0) {
      copyImage16x16 (&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);

      if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
        copyImage (&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        copyImage (&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        }
      }
    else {
      iTransform (mb, plane, 0);
      slice->isResetCoef = false;
      }
    return 1;
    }
  //}}}
  //{{{
  int mbPredBd4x4temporal (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture) {

    int16_t refIndex;
    int refList;

    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;

    int listOffset = mb->listOffset;
    sPicture** list0 = slice->listX[LIST_0 + listOffset];
    sPicture** list1 = slice->listX[LIST_1 + listOffset];

    setChromaVector(mb);

    for (int block8x8 = 0; block8x8 < 4; block8x8++) {
      int predDir = mb->b8pdir[block8x8];
      int k_start = (block8x8 << 2);
      int k_end = k_start + BLOCK_MULTIPLE;
      for (int k = k_start; k < k_end; k ++) {
        int i =  (decode_block_scan[k] & 3);
        int j = ((decode_block_scan[k] >> 2) & 3);
        int i4 = mb->blockX + i;
        int j4 = mb->blockY + j;
        int j6 = mb->blockYaff + j;
        sPicMotion* mvInfo = &picture->mvInfo[j4][i4];
        sPicMotion* colocated = &list1[0]->mvInfo[j6][i4];
        if(mb->decoder->coding.isSeperateColourPlane && mb->decoder->coding.yuvFormat==YUV444)
          colocated = &list1[0]->mvInfoJV[mb->slice->colourPlaneId][RSD(j6)][RSD(i4)];

        refList = (colocated->refIndex[LIST_0]== -1 ? LIST_1 : LIST_0);
        refIndex =  colocated->refIndex[refList];

        if (refIndex == -1) {
          // co-located is intra mode
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_0] = 0;
          mvInfo->refIndex[LIST_1] = 0;
          }
        else {
          // co-located skip or inter mode
          int mapped_idx = 0;
          int iref;

          for (iref = 0; iref < imin(slice->numRefIndexActive[LIST_0], slice->listXsize[LIST_0 + listOffset]); iref++) {
            if(slice->listX[LIST_0 + listOffset][iref] == colocated->refPic[refList]) {
              mapped_idx = iref;
              break;
              }
            else //! invalid index. Default to zero even though this case should not happen
              mapped_idx = INVALIDINDEX;
            }
          if (INVALIDINDEX == mapped_idx)
            cDecoder264::error ("temporal direct error: colocated block has ref that is unavailable");
          else {
            int mv_scale = slice->mvscale[LIST_0 + listOffset][mapped_idx];

            //! In such case, an array is needed for each different reference.
            if (mv_scale == 9999 || slice->listX[LIST_0+listOffset][mapped_idx]->usedLongTerm) {
              mvInfo->mv[LIST_0] = colocated->mv[refList];
              mvInfo->mv[LIST_1] = kZeroMv;
              }
            else {
              mvInfo->mv[LIST_0].mvX = (int16_t) ((mv_scale * colocated->mv[refList].mvX + 128 ) >> 8);
              mvInfo->mv[LIST_0].mvY = (int16_t) ((mv_scale * colocated->mv[refList].mvY + 128 ) >> 8);
              mvInfo->mv[LIST_1].mvX = (int16_t) (mvInfo->mv[LIST_0].mvX - colocated->mv[refList].mvX);
              mvInfo->mv[LIST_1].mvY = (int16_t) (mvInfo->mv[LIST_0].mvY - colocated->mv[refList].mvY);
              }

            mvInfo->refIndex[LIST_0] = (char) mapped_idx; //colocated->refIndex[refList];
            mvInfo->refIndex[LIST_1] = 0;
            }
          }

        // store reference picture ID determined by direct mode
        mvInfo->refPic[LIST_0] = list0[(int16_t)mvInfo->refIndex[LIST_0]];
        mvInfo->refPic[LIST_1] = list1[(int16_t)mvInfo->refIndex[LIST_1]];
        }

      for (int k = k_start; k < k_end; k ++) {
        int i =  (decode_block_scan[k] & 3);
        int j = ((decode_block_scan[k] >> 2) & 3);
        performMotionCompensation (mb, plane, picture, predDir, i, j, BLOCK_SIZE, BLOCK_SIZE);
        }
      }

    if (mb->codedBlockPattern == 0) {
      copyImage16x16 (&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);
      if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
        copyImage (&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        copyImage (&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      }
    }
    else {
      iTransform (mb, plane, 0);
      slice->isResetCoef = false;
      }

    return 1;
    }
  //}}}
  //{{{
  int mbPredBd8x8spatial (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture) {

    char l0_rFrame = -1, l1_rFrame = -1;
    sMotionVec pmvl0 = kZeroMv, pmvl1 = kZeroMv;
    int i4, j4;
    int block8x8;
    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;

    sPicMotion* mvInfo;
    int listOffset = mb->listOffset;
    sPicture** list0 = slice->listX[LIST_0 + listOffset];
    sPicture** list1 = slice->listX[LIST_1 + listOffset];

    int predDir = 0;

    setChromaVector (mb);
    prepareDirectParam (mb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

    if (l0_rFrame == 0 || l1_rFrame == 0) {
      bool is_not_moving;
      for (block8x8 = 0; block8x8 < 4; block8x8++) {
        int k_start = (block8x8 << 2);
        int i  =  (decode_block_scan[k_start] & 3);
        int j  = ((decode_block_scan[k_start] >> 2) & 3);
        i4  = mb->blockX + i;
        j4  = mb->blockY + j;

        is_not_moving = !getColocatedInfo8x8 (mb, list1[0], i4, mb->blockYaff + j);
        mvInfo = &picture->mvInfo[j4][i4];

        if (l1_rFrame == -1) {
          if (is_not_moving) {
            mvInfo->refPic[LIST_0] = list0[0];
            mvInfo->refPic[LIST_1] = NULL;
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = 0;
            mvInfo->refIndex[LIST_1] = -1;
            }
          else {
            mvInfo->refPic[LIST_0] = list0[(int16_t) l0_rFrame];
            mvInfo->refPic[LIST_1] = NULL;
            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
            mvInfo->refIndex[LIST_1] = -1;
            }
          predDir = 0;
          }
        else if (l0_rFrame == -1) {
          if (is_not_moving) {
            mvInfo->refPic[LIST_0] = NULL;
            mvInfo->refPic[LIST_1] = list1[0];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = -1;
            mvInfo->refIndex[LIST_1] = 0;
            }
          else {
            mvInfo->refPic[LIST_0] = NULL;
            mvInfo->refPic[LIST_1] = list1[(int16_t) l1_rFrame];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_0] = -1;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
            }
          predDir = 1;
          }

        else {
          if (l0_rFrame == 0 && ((is_not_moving))) {
            mvInfo->refPic[LIST_0] = list0[0];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->refIndex[LIST_0] = 0;
            }
          else {
            mvInfo->refPic[LIST_0] = list0[(int16_t) l0_rFrame];
            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
            }

          if  (l1_rFrame == 0 && ((is_not_moving))) {
            mvInfo->refPic[LIST_1] = list1[0];
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_1]    = 0;
            }
          else {
            mvInfo->refPic[LIST_1] = list1[(int16_t) l1_rFrame];
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
            }
          predDir = 2;
          }

        updateNeighbourMvs (&picture->mvInfo[j4], mvInfo, i4);
        performMotionCompensation (mb, plane, picture, predDir, i, j, SMB_BLOCK_SIZE, SMB_BLOCK_SIZE);
        }
      }
    else {
      if (l0_rFrame < 0 && l1_rFrame < 0) {
        predDir = 2;
        for (j4 = mb->blockY; j4 < mb->blockY + BLOCK_MULTIPLE; j4 += 2) {
          for (i4 = mb->blockX; i4 < mb->blockX + BLOCK_MULTIPLE; i4 += 2) {
            mvInfo = &picture->mvInfo[j4][i4];
            mvInfo->refPic[LIST_0] = list0[0];
            mvInfo->refPic[LIST_1] = list1[0];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = 0;
            mvInfo->refIndex[LIST_1] = 0;
            updateNeighbourMvs (&picture->mvInfo[j4], mvInfo, i4);
            }
          }
        }

      else if (l1_rFrame == -1) {
        predDir = 0;
        for (j4 = mb->blockY; j4 < mb->blockY + BLOCK_MULTIPLE; j4 += 2) {
          for (i4 = mb->blockX; i4 < mb->blockX + BLOCK_MULTIPLE; i4 += 2) {
            mvInfo = &picture->mvInfo[j4][i4];
            mvInfo->refPic[LIST_0] = list0[(int16_t) l0_rFrame];
            mvInfo->refPic[LIST_1] = NULL;
            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
            mvInfo->refIndex[LIST_1] = -1;
            updateNeighbourMvs (&picture->mvInfo[j4], mvInfo, i4);
            }
          }
        }

      else if (l0_rFrame == -1) {
        predDir = 1;
        for (j4 = mb->blockY; j4 < mb->blockY + BLOCK_MULTIPLE; j4 += 2) {
          for (i4 = mb->blockX; i4 < mb->blockX + BLOCK_MULTIPLE; i4 += 2) {
            mvInfo = &picture->mvInfo[j4][i4];
            mvInfo->refPic[LIST_0] = NULL;
            mvInfo->refPic[LIST_1] = list1[(int16_t) l1_rFrame];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_0] = -1;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
            updateNeighbourMvs (&picture->mvInfo[j4], mvInfo, i4);
            }
          }
        }

      else {
        predDir = 2;
        for (j4 = mb->blockY; j4 < mb->blockY + BLOCK_MULTIPLE; j4 += 2) {
          for (i4 = mb->blockX; i4 < mb->blockX + BLOCK_MULTIPLE; i4 += 2) {
            mvInfo = &picture->mvInfo[j4][i4];
            mvInfo->refPic[LIST_0] = list0[(int16_t) l0_rFrame];
            mvInfo->refPic[LIST_1] = list1[(int16_t) l1_rFrame];
            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
            updateNeighbourMvs (&picture->mvInfo[j4], mvInfo, i4);
            }
          }
        }

      // Now perform Motion Compensation
      performMotionCompensation (mb, plane, picture, predDir, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
      }

    if (mb->codedBlockPattern == 0) {
      copyImage16x16 (&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);
      if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
        copyImage (&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        copyImage (&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        }
      }
    else {
      iTransform (mb, plane, 0);
      slice->isResetCoef = false;
      }

    return 1;
    }
  //}}}
  //{{{
  int mbPredBd4x4spatial (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture) {

    char l0_rFrame = -1, l1_rFrame = -1;
    sMotionVec pmvl0 = kZeroMv, pmvl1 = kZeroMv;
    int k;
    int block8x8;
    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;

    sPicMotion* mvInfo;
    int listOffset = mb->listOffset;
    sPicture** list0 = slice->listX[LIST_0 + listOffset];
    sPicture** list1 = slice->listX[LIST_1 + listOffset];

    int predDir = 0;

    setChromaVector (mb);

    prepareDirectParam (mb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

    for (block8x8 = 0; block8x8 < 4; block8x8++) {
      int k_start = (block8x8 << 2);
      int k_end = k_start + BLOCK_MULTIPLE;
      for (k = k_start; k < k_end; k ++) {
        int i =  (decode_block_scan[k] & 3);
        int j = ((decode_block_scan[k] >> 2) & 3);
        int i4 = mb->blockX + i;
        int j4 = mb->blockY + j;

        mvInfo = &picture->mvInfo[j4][i4];
        // DIRECT PREDICTION
        if (l0_rFrame == 0 || l1_rFrame == 0) {
          bool is_not_moving = !getColocatedInfo4x4 (mb, list1[0], i4, mb->blockYaff + j);
          if (l1_rFrame == -1) {
            if (is_not_moving) {
              mvInfo->refPic[LIST_0] = list0[0];
              mvInfo->refPic[LIST_1] = NULL;
              mvInfo->mv[LIST_0] = kZeroMv;
              mvInfo->mv[LIST_1] = kZeroMv;
              mvInfo->refIndex[LIST_0] = 0;
              mvInfo->refIndex[LIST_1] = -1;
              }
            else {
              mvInfo->refPic[LIST_0] = list0[(int16_t)l0_rFrame];
              mvInfo->refPic[LIST_1] = NULL;
              mvInfo->mv[LIST_0] = pmvl0;
              mvInfo->mv[LIST_1] = kZeroMv;
              mvInfo->refIndex[LIST_0] = l0_rFrame;
              mvInfo->refIndex[LIST_1] = -1;
              }
            predDir = 0;
            }
          else if (l0_rFrame == -1) {
            if  (is_not_moving) {
              mvInfo->refPic[LIST_0] = NULL;
              mvInfo->refPic[LIST_1] = list1[0];
              mvInfo->mv[LIST_0] = kZeroMv;
              mvInfo->mv[LIST_1] = kZeroMv;
              mvInfo->refIndex[LIST_0] = -1;
              mvInfo->refIndex[LIST_1] = 0;
              }
            else {
              mvInfo->refPic[LIST_0] = NULL;
              mvInfo->refPic[LIST_1] = list1[(int16_t)l1_rFrame];
              mvInfo->mv[LIST_0] = kZeroMv;
              mvInfo->mv[LIST_1] = pmvl1;
              mvInfo->refIndex[LIST_0] = -1;
              mvInfo->refIndex[LIST_1] = l1_rFrame;
              }
            predDir = 1;
            }
          else {
            if (l0_rFrame == 0 && ((is_not_moving))) {
              mvInfo->refPic[LIST_0] = list0[0];
              mvInfo->mv[LIST_0] = kZeroMv;
              mvInfo->refIndex[LIST_0] = 0;
              }
            else {
              mvInfo->refPic[LIST_0] = list0[(int16_t)l0_rFrame];
              mvInfo->mv[LIST_0] = pmvl0;
              mvInfo->refIndex[LIST_0] = l0_rFrame;
             }

            if  (l1_rFrame == 0 && ((is_not_moving))) {
              mvInfo->refPic[LIST_1] = list1[0];
              mvInfo->mv[LIST_1] = kZeroMv;
              mvInfo->refIndex[LIST_1] = 0;
              }
            else {
              mvInfo->refPic[LIST_1] = list1[(int16_t)l1_rFrame];
              mvInfo->mv[LIST_1] = pmvl1;
              mvInfo->refIndex[LIST_1] = l1_rFrame;
              }
            predDir = 2;
            }
          }
        else {
          mvInfo = &picture->mvInfo[j4][i4];

          if (l0_rFrame < 0 && l1_rFrame < 0) {
            predDir = 2;
            mvInfo->refPic[LIST_0] = list0[0];
            mvInfo->refPic[LIST_1] = list1[0];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = 0;
            mvInfo->refIndex[LIST_1] = 0;
            }
          else if (l1_rFrame == -1) {
            predDir = 0;
            mvInfo->refPic[LIST_0] = list0[(int16_t)l0_rFrame];
            mvInfo->refPic[LIST_1] = NULL;
            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
            mvInfo->refIndex[LIST_1] = -1;
            }
          else if (l0_rFrame == -1) {
            predDir = 1;
            mvInfo->refPic[LIST_0] = NULL;
            mvInfo->refPic[LIST_1] = list1[(int16_t)l1_rFrame];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_0] = -1;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
            }
          else {
            predDir = 2;
            mvInfo->refPic[LIST_0] = list0[(int16_t)l0_rFrame];
            mvInfo->refPic[LIST_1] = list1[(int16_t)l1_rFrame];
            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
            }
          }
        }

      for (k = k_start; k < k_end; k ++) {
        int i = (decode_block_scan[k] & 3);
        int j = ((decode_block_scan[k] >> 2) & 3);
        performMotionCompensation (mb, plane, picture, predDir, i, j, BLOCK_SIZE, BLOCK_SIZE);
        }
      }

    if (mb->codedBlockPattern == 0) {
      copyImage16x16 (&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);
      if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
        copyImage (&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        copyImage (&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        }
      }
    else {
      iTransform (mb, plane, 0);
      slice->isResetCoef = false;
      }

    return 1;
    }
  //}}}
  //{{{
  int mbPredBinter8x8 (sMacroBlock* mb, eColorPlane plane, sPicture* picture) {

    char l0_rFrame = -1, l1_rFrame = -1;
    sMotionVec pmvl0 = kZeroMv, pmvl1 = kZeroMv;
    int blockSizeX, blockSizeY;
    int k;
    int block8x8;   // needed for ABT
    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;

    int listOffset = mb->listOffset;
    sPicture** list0 = slice->listX[LIST_0 + listOffset];
    sPicture** list1 = slice->listX[LIST_1 + listOffset];

    setChromaVector (mb);

    // prepare direct modes
    if (slice->directSpatialMvPredFlag &&
        (!(mb->b8mode[0] && mb->b8mode[1] && mb->b8mode[2] && mb->b8mode[3])))
      prepareDirectParam (mb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

    for (block8x8 = 0; block8x8 < 4; block8x8++) {
      int mv_mode  = mb->b8mode[block8x8];
      int predDir = mb->b8pdir[block8x8];

      if (mv_mode != 0) {
        int k_start = (block8x8 << 2);
        int k_inc = (mv_mode == SMB8x4) ? 2 : 1;
        int k_end = (mv_mode == SMB8x8) ? k_start + 1 :
                       ((mv_mode == SMB4x4) ? k_start + 4 : k_start + k_inc + 1);

        blockSizeX = (mv_mode == SMB8x4 || mv_mode == SMB8x8) ? SMB_BLOCK_SIZE : BLOCK_SIZE;
        blockSizeY = (mv_mode == SMB4x8 || mv_mode == SMB8x8) ? SMB_BLOCK_SIZE : BLOCK_SIZE;

        for (k = k_start; k < k_end; k += k_inc) {
          int i = decode_block_scan[k] & 3;
          int j = (decode_block_scan[k] >> 2) & 3;
          performMotionCompensation (mb, plane, picture, predDir, i, j, blockSizeX, blockSizeY);
          }
        }
      else {
        int k_start = (block8x8 << 2);
        int k_end = k_start;

        if (decoder->activeSps->isDirect8x8inference) {
          blockSizeX = SMB_BLOCK_SIZE;
          blockSizeY = SMB_BLOCK_SIZE;
          k_end ++;
          }
        else {
          blockSizeX = BLOCK_SIZE;
          blockSizeY = BLOCK_SIZE;
          k_end += BLOCK_MULTIPLE;
          }

        // Prepare mvs (needed for deblocking and mv prediction
        if (slice->directSpatialMvPredFlag) {
          for (k = k_start; k < k_start + BLOCK_MULTIPLE; k ++) {
            int i = decode_block_scan[k] & 3;
            int j = (decode_block_scan[k] >> 2) & 3;
            int i4 = mb->blockX + i;
            int j4 = mb->blockY + j;
            sPicMotion* mvInfo = &picture->mvInfo[j4][i4];

            // DIRECT PREDICTION, motion information should be already set
            if (mvInfo->refIndex[LIST_1] == -1)
              predDir = 0;
            else if (mvInfo->refIndex[LIST_0] == -1)
              predDir = 1;
            else
              predDir = 2;
            }
          }
        else {
          for (k = k_start; k < k_start + BLOCK_MULTIPLE; k ++) {
            int i = decode_block_scan[k] & 3;
            int j = (decode_block_scan[k] >> 2) & 3;
            int i4 = mb->blockX + i;
            int j4 = mb->blockY + j;
            sPicMotion *mvInfo = &picture->mvInfo[j4][i4];

            // store reference picture ID determined by direct mode
            mvInfo->refPic[LIST_0] = list0[(int16_t)mvInfo->refIndex[LIST_0]];
            mvInfo->refPic[LIST_1] = list1[(int16_t)mvInfo->refIndex[LIST_1]];
            }
          }

        for (k = k_start; k < k_end; k ++) {
          int i = decode_block_scan[k] & 3;
          int j = (decode_block_scan[k] >> 2) & 3;
          performMotionCompensation (mb, plane, picture, predDir, i, j, blockSizeX, blockSizeY);
          }
        }
      }

    iTransform (mb, plane, 0);
    if (mb->codedBlockPattern != 0)
      slice->isResetCoef = false;

    return 1;
    }
  //}}}
  //{{{
  int mbPredIpcm (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;

    cSlice* slice = mb->slice;
    sPicture* picture = slice->picture;

    // copy coefficients to decoded picture buffer
    // IPCM coefficients are stored in slice->cof which is set in function readIPCMcoeffs()
    for (int i = 0; i < MB_BLOCK_SIZE; ++i)
      for (int j = 0;j < MB_BLOCK_SIZE ; ++j)
        picture->imgY[mb->pixY + i][mb->pixX + j] = (sPixel) slice->cof[0][i][j];

    if ((picture->chromaFormatIdc != YUV400) && (decoder->coding.isSeperateColourPlane == 0))
      for (int k = 0; k < 2; ++k)
        for(int i = 0; i < decoder->mbCrSizeY; ++i)
          for (int j = 0; j < decoder->mbCrSizeX; ++j)
            picture->imgUV[k][mb->piccY+i][mb->pixcX + j] = (sPixel) slice->cof[k + 1][i][j];

    // for deblocking filter
    updateQp (mb, 0);

    // for cavlc: Set the nzCoeff to 16.
    // These parameters are to be used in cavlc decoding of neighbour blocks
    memset (decoder->nzCoeff[mb->mbIndexX][0][0], 16, 3 * BLOCK_PIXELS * sizeof(uint8_t));

    // for cabac decoding of MB skip flag
    mb->skipFlag = 0;

    // for deblocking filter cabac
    mb->codedBlockPatterns[0].blk = 0xFFFF;

    // for cabac decoding of Dquant
    slice->lastDquant = 0;
    slice->isResetCoef = false;
    slice->isResetCoefCr = false;
    return 1;
    }
  //}}}
  //}}}
  //{{{  decodeComponent functions
  //{{{
  int decodeComponentI (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
  {
    //For residual DPCM
    mb->dpcmMode = NO_INTRA_PMODE;
    if (mb->mbType == IPCM)
      mbPredIpcm (mb);
    else if (mb->mbType == I16MB)
      mbPredIntra16x16 (mb, plane, picture);
    else if (mb->mbType == I4MB)
      mbPredIntra4x4 (mb, plane, pixel, picture);
    else if (mb->mbType == I8MB)
      mbPredIntra8x8 (mb, plane, pixel, picture);

    return 1;
    }
  //}}}
  //{{{
  int decodeComponentP (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
  {
    //For residual DPCM
    mb->dpcmMode = NO_INTRA_PMODE;
    if (mb->mbType == IPCM)
      mbPredIpcm (mb);
    else if (mb->mbType == I16MB)
      mbPredIntra16x16 (mb, plane, picture);
    else if (mb->mbType == I4MB)
      mbPredIntra4x4 (mb, plane, pixel, picture);
    else if (mb->mbType == I8MB)
      mbPredIntra8x8 (mb, plane, pixel, picture);
    else if (mb->mbType == PSKIP)
      mbPredSkip( mb, plane, pixel, picture);
    else if (mb->mbType == P16x16)
      mbPredPinter16x16 (mb, plane, picture);
    else if (mb->mbType == P16x8)
      mbPredPinter16x8 (mb, plane, picture);
    else if (mb->mbType == P8x16)
      mbPredPinter8x16 (mb, plane, picture);
    else
      mbPredPinter8x8 (mb, plane, picture);

    return 1;
    }
  //}}}
  //{{{
  int decodeComponentSP (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
  {
    //For residual DPCM
    mb->dpcmMode = NO_INTRA_PMODE;

    if (mb->mbType == IPCM)
      mbPredIpcm (mb);
    else if (mb->mbType == I16MB)
      mbPredIntra16x16 (mb, plane, picture);
    else if (mb->mbType == I4MB)
      mbPredIntra4x4 (mb, plane, pixel, picture);
    else if (mb->mbType == I8MB)
      mbPredIntra8x8 (mb, plane, pixel, picture);
    else if (mb->mbType == PSKIP)
      mbPredSpSkip (mb, plane, picture);
    else if (mb->mbType == P16x16)
      mbPredPinter16x16 (mb, plane, picture);
    else if (mb->mbType == P16x8)
      mbPredPinter16x8 (mb, plane, picture);
    else if (mb->mbType == P8x16)
      mbPredPinter8x16 (mb, plane, picture);
    else
      mbPredPinter8x8 (mb, plane, picture);

    return 1;
    }
  //}}}
  //{{{
  int decodeComponentB (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
  {
    //For residual DPCM
    mb->dpcmMode = NO_INTRA_PMODE;

    if(mb->mbType == IPCM)
      mbPredIpcm (mb);
    else if (mb->mbType == I16MB)
      mbPredIntra16x16 (mb, plane, picture);
    else if (mb->mbType == I4MB)
      mbPredIntra4x4 (mb, plane, pixel, picture);
    else if (mb->mbType == I8MB)
      mbPredIntra8x8 (mb, plane, pixel, picture);
    else if (mb->mbType == P16x16)
      mbPredPinter16x16 (mb, plane, picture);
    else if (mb->mbType == P16x8)
      mbPredPinter16x8 (mb, plane, picture);
    else if (mb->mbType == P8x16)
      mbPredPinter8x16 (mb, plane, picture);
    else if (mb->mbType == BSKIP_DIRECT) {
      cSlice* slice = mb->slice;
      if (slice->directSpatialMvPredFlag == 0) {
        if (slice->activeSps->isDirect8x8inference)
          mbPredBd8x8temporal (mb, plane, pixel, picture);
        else
          mbPredBd4x4temporal (mb, plane, pixel, picture);
        }
      else {
        if (slice->activeSps->isDirect8x8inference)
          mbPredBd8x8spatial (mb, plane, pixel, picture);
        else
          mbPredBd4x4spatial (mb, plane, pixel, picture);
        }
      }
    else
      mbPredBinter8x8 (mb, plane, picture);

    return 1;
    }
  //}}}
  //}}}

  //{{{
  int BType2CtxRef (int btype) {
    return (btype >= 4);
    }
  //}}}
  //{{{  readRef functions
  //{{{
  char readRefPictureIdxVLC (sMacroBlock* mb, sSyntaxElement* se,
                                     sDataPartition* dataPartition, char b8mode, int list) {

    se->context = BType2CtxRef (b8mode);
    se->value2 = list;
    dataPartition->readSyntaxElement (mb, se, dataPartition);
    return (char) se->value1;
    }
  //}}}
  //{{{
  char readRefPictureIdxFLC (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* dataPartition, char b8mode, int list)
  {
    se->context = BType2CtxRef (b8mode);
    se->len = 1;
    dataPartition->bitStream.readSyntaxElement_FLC (se);
    se->value1 = 1 - se->value1;
    return (char)se->value1;
    }
  //}}}
  //{{{
  char readRefPictureIdxNull (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* dataPartition, char b8mode, int list)
  {
    return 0;
  }
  //}}}
  //}}}
  //{{{
  void prepareListforRefIndex (sMacroBlock* mb, sSyntaxElement* se,
                               sDataPartition* dataPartition, int numRefIndexActive, int refidx_present) {

    if (numRefIndexActive > 1) {
      if (mb->decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag) {
        se->mapping = cBitStream::linfo_ue;
        if (refidx_present)
          mb->readRefPictureIndex = (numRefIndexActive == 2) ? readRefPictureIdxFLC : readRefPictureIdxVLC;
        else
          mb->readRefPictureIndex = readRefPictureIdxNull;
        }
      else {
        se->reading = readRefFrameCabac;
        mb->readRefPictureIndex = refidx_present ? readRefPictureIdxVLC : readRefPictureIdxNull;
        }
      }
    else
      mb->readRefPictureIndex = readRefPictureIdxNull;
    }
  //}}}
  //{{{
  void readMBRefPictureIdx (sSyntaxElement* se, sDataPartition* dataPartition,
                            sMacroBlock* mb, sPicMotion** mvInfo,
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
            refIndex += sizeof(sPicMotion);
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
              refIndex += sizeof(sPicMotion);
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
              refIndex += sizeof(sPicMotion);
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
                refIndex += sizeof(sPicMotion);
                }
              }
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void readMBMotionVectors (sSyntaxElement* se, sDataPartition* dataPartition, sMacroBlock* mb,
                                   int list, int step_h0, int step_v0) {

    if (mb->mbType == 1) {
      if ((mb->b8pdir[0] == list || mb->b8pdir[0]== BI_PRED)) {
        // has forward vector
        int i4, j4, ii, jj;
        int16_t curr_mvd[2];
        sMotionVec pred_mv, curr_mv;
        int16_t (*mvd)[4][2];
        sPicMotion** mvInfo = mb->slice->picture->mvInfo;
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
        curr_mvd[0] = (int16_t) se->value1;

        // Y component
        se->value2 += 2; // identifies the component; only used for context determination
        dataPartition->readSyntaxElement(mb, se, dataPartition);
        curr_mvd[1] = (int16_t) se->value1;

        curr_mv.mvX = (int16_t)(curr_mvd[0] + pred_mv.mvX);  // compute motion vector x
        curr_mv.mvY = (int16_t)(curr_mvd[1] + pred_mv.mvY);  // compute motion vector y

        for (jj = j4; jj < j4 + step_v0; ++jj) {
          sPicMotion *mvinfo = mvInfo[jj] + i4;
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
          memcpy (mvd[jj][0], mvd[0][0],  2 * step_h0 * sizeof(int16_t));
        }
      }
    else {
      int i4, j4, ii, jj;
      int16_t curr_mvd[2];
      sMotionVec pred_mv, curr_mv;
      int16_t (*mvd)[4][2];
      sPicMotion** mvInfo = mb->slice->picture->mvInfo;
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
                  curr_mvd[k] = (int16_t)se->value1;
                  }

                curr_mv.mvX = (int16_t)(curr_mvd[0] + pred_mv.mvX);  // compute motion vector
                curr_mv.mvY = (int16_t)(curr_mvd[1] + pred_mv.mvY);  // compute motion vector

                for(jj = j4; jj < j4 + step_v; ++jj) {
                  sPicMotion *mvinfo = mvInfo[jj] + i4;
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
                  memcpy(&mvd[jj][i][0], &mvd[0][i][0],  2 * step_h * sizeof(int16_t));
                }
              }
            }
          }
        }
      }
    }
  //}}}

  //{{{  macroBlock utils
  //{{{
  bool isMbAvailable (int mbIndex, sMacroBlock* mb) {

    cSlice* slice = mb->slice;
    if ((mbIndex < 0) || (mbIndex > ((int)slice->picture->picSizeInMbs - 1)))
      return false;

    // the following line checks both: slice number and if the mb has been decoded
    if (!mb->DeblockCall)
      if (slice->mbData[mbIndex].sliceNum != mb->sliceNum)
        return false;

    return true;
    }
  //}}}
  //{{{
  void setMbPosInfo (sMacroBlock* mb) {

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
  void read_ipred_8x8_modes_mbaff (sMacroBlock* mb) {

    int bi, bj, bx, by, dec;
    cSlice* slice = mb->slice;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;

    int mostProbableIntraPredMode;
    int upIntraPredMode;
    int leftIntraPredMode;

    sPixelPos left_block, top_block;

    sSyntaxElement se;
    se.type = SE_INTRAPREDMODE;
    sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);

    if (!(decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag))
      se.reading = readIntraPredModeCabac;

    for (int b8 = 0; b8 < 4; ++b8)  {
      // loop 8x8 blocks
      by = (b8 & 0x02);
      bj = mb->blockY + by;

      bx = ((b8 & 0x01) << 1);
      bi = mb->blockX + bx;
      // get from stream
      if (decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag)
        dataPartition->bitStream.readSyntaxElement_Intra4x4PredictionMode (&se);
      else {
        se.context = b8 << 2;
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        }

      get4x4Neighbour (mb, (bx << 2) - 1, (by << 2),     decoder->mbSize[eLuma], &left_block);
      get4x4Neighbour (mb, (bx << 2),     (by << 2) - 1, decoder->mbSize[eLuma], &top_block );

      // get from array and decode
      if (decoder->activePps->hasConstrainedIntraPred) {
        left_block.ok = left_block.ok ? slice->intraBlock[left_block.mbIndex] : 0;
        top_block.ok = top_block.ok  ? slice->intraBlock[top_block.mbIndex]  : 0;
        }

      upIntraPredMode = (top_block.ok ) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
      leftIntraPredMode = (left_block.ok) ? slice->predMode[left_block.posY][left_block.posX] : -1;
      mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED :
                                    upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
      dec = (se.value1 == -1) ?
        mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode);

      //set, loop 4x4s in the subblock for 8x8 prediction setting
      slice->predMode[bj][bi] = (uint8_t) dec;
      slice->predMode[bj][bi+1] = (uint8_t) dec;
      slice->predMode[bj+1][bi] = (uint8_t) dec;
      slice->predMode[bj+1][bi+1] = (uint8_t) dec;
      }
    }
  //}}}
  //{{{
  void read_ipred_8x8_modes (sMacroBlock* mb) {

    int b8, bi, bj, bx, by, dec;
    cSlice* slice = mb->slice;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;

    int mostProbableIntraPredMode;
    int upIntraPredMode;
    int leftIntraPredMode;

    sPixelPos left_mb, top_mb;
    sPixelPos left_block, top_block;

    sSyntaxElement se;
    se.type = SE_INTRAPREDMODE;
    sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);
    if (!(decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag))
      se.reading = readIntraPredModeCabac;

    get4x4Neighbour (mb, -1,  0, decoder->mbSize[eLuma], &left_mb);
    get4x4Neighbour (mb,  0, -1, decoder->mbSize[eLuma], &top_mb );

    for(b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
      by = (b8 & 0x02);
      bj = mb->blockY + by;

      bx = ((b8 & 0x01) << 1);
      bi = mb->blockX + bx;

      //get from stream
      if (decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag)
        dataPartition->bitStream.readSyntaxElement_Intra4x4PredictionMode (&se);
      else {
        se.context = (b8 << 2);
        dataPartition->readSyntaxElement(mb, &se, dataPartition);
      }

      get4x4Neighbour (mb, (bx << 2) - 1, by << 2, decoder->mbSize[eLuma], &left_block);
      get4x4Neighbour (mb, bx << 2, (by << 2) - 1, decoder->mbSize[eLuma], &top_block);

      // get from array and decode
      if (decoder->activePps->hasConstrainedIntraPred) {
        left_block.ok = left_block.ok ? slice->intraBlock[left_block.mbIndex] : 0;
        top_block.ok = top_block.ok  ? slice->intraBlock[top_block.mbIndex] : 0;
      }

      upIntraPredMode = (top_block.ok ) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
      leftIntraPredMode = (left_block.ok) ? slice->predMode[left_block.posY][left_block.posX] : -1;
      mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ?
        DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
      dec = (se.value1 == -1) ?
        mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode);

      //set
      //loop 4x4s in the subblock for 8x8 prediction setting
      slice->predMode[bj][bi] = (uint8_t)dec;
      slice->predMode[bj][bi+1] = (uint8_t)dec;
      slice->predMode[bj+1][bi] = (uint8_t)dec;
      slice->predMode[bj+1][bi+1] = (uint8_t)dec;
    }
  }
  //}}}
  //{{{
  void read_ipred_4x4_modes_mbaff (sMacroBlock* mb) {

    int b8,i,j,bi,bj,bx,by;
    sSyntaxElement se;
    cSlice* slice = mb->slice;
    const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264 *decoder = mb->decoder;
    sBlockPos *picPos = decoder->picPos;

    int ts, ls;
    int mostProbableIntraPredMode;
    int upIntraPredMode;
    int leftIntraPredMode;
    sPixelPos left_block, top_block;

    se.type = SE_INTRAPREDMODE;
    sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);
    if (!(decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag))
      se.reading = readIntraPredModeCabac;

    for (b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
      for (j = 0; j < 2; j++) { //loop subblocks
        by = (b8 & 0x02) + j;
        bj = mb->blockY + by;

        for(i = 0; i < 2; i++) {
          bx = ((b8 & 1) << 1) + i;
          bi = mb->blockX + bx;
          //get from stream
          if (decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag)
            dataPartition->bitStream.readSyntaxElement_Intra4x4PredictionMode (&se);
          else {
            se.context = (b8<<2) + (j<<1) +i;
            dataPartition->readSyntaxElement (mb, &se, dataPartition);
            }

          get4x4Neighbour (mb, (bx<<2) - 1, (by<<2),     decoder->mbSize[eLuma], &left_block);
          get4x4Neighbour (mb, (bx<<2),     (by<<2) - 1, decoder->mbSize[eLuma], &top_block );

          // get from array and decode
          if (decoder->activePps->hasConstrainedIntraPred) {
            left_block.ok = left_block.ok ? slice->intraBlock[left_block.mbIndex] : 0;
            top_block.ok = top_block.ok  ? slice->intraBlock[top_block.mbIndex]  : 0;
            }

          // !! KS: not sure if the following is still correct...
          ts = ls = 0;   // Check to see if the neighboring block is SI
          if (slice->sliceType == eSliceSI) { // need support for MBINTLC1
            if (left_block.ok)
              if (slice->siBlock [picPos[left_block.mbIndex].y][picPos[left_block.mbIndex].x])
                ls = 1;

            if (top_block.ok)
              if (slice->siBlock [picPos[top_block.mbIndex].y][picPos[top_block.mbIndex].x])
                ts = 1;
            }

          upIntraPredMode = (top_block.ok  &&(ts == 0)) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
          leftIntraPredMode = (left_block.ok &&(ls == 0)) ? slice->predMode[left_block.posY][left_block.posX] : -1;
          mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ?
            DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
          slice->predMode[bj][bi] = (uint8_t) ((se.value1 == -1) ?
            mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode));
          }
        }
      }
    }
  //}}}
  //{{{
  void read_ipred_4x4_modes (sMacroBlock* mb) {

    cSlice* slice = mb->slice;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;
    sBlockPos* picPos = decoder->picPos;

    int mostProbableIntraPredMode;
    int upIntraPredMode;
    int leftIntraPredMode;

    sPixelPos left_mb, top_mb;
    sPixelPos left_block, top_block;

    sSyntaxElement se;
    se.type = SE_INTRAPREDMODE;
    sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);
    if (!(decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag))
      se.reading = readIntraPredModeCabac;

    get4x4Neighbour (mb, -1,  0, decoder->mbSize[eLuma], &left_mb);
    get4x4Neighbour (mb,  0, -1, decoder->mbSize[eLuma], &top_mb );

    for (int b8 = 0; b8 < 4; ++b8) {
      // loop 8x8 blocks
      for (int j = 0; j < 2; j++) {
        // loop subblocks
        int by = (b8 & 0x02) + j;
        int bj = mb->blockY + by;
        for (int i = 0; i < 2; i++) {
          int bx = ((b8 & 1) << 1) + i;
          int bi = mb->blockX + bx;

          // get from stream
          if (decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag)
            dataPartition->bitStream.readSyntaxElement_Intra4x4PredictionMode (&se);
          else {
            se.context = (b8 << 2) + (j << 1) + i;
            dataPartition->readSyntaxElement (mb, &se, dataPartition);
            }

          get4x4Neighbour (mb, (bx << 2) - 1, by << 2, decoder->mbSize[eLuma], &left_block);
          get4x4Neighbour (mb, bx << 2, (by << 2) - 1, decoder->mbSize[eLuma], &top_block);

          //get from array and decode
          if (decoder->activePps->hasConstrainedIntraPred) {
            left_block.ok = left_block.ok ? slice->intraBlock[left_block.mbIndex] : 0;
            top_block.ok = top_block.ok  ? slice->intraBlock[top_block.mbIndex]  : 0;
            }

          int ts = 0;
          int ls = 0;   // Check to see if the neighboring block is SI
          if (slice->sliceType == eSliceSI) {
            //{{{  need support for MBINTLC1
            if (left_block.ok)
              if (slice->siBlock [picPos[left_block.mbIndex].y][picPos[left_block.mbIndex].x])
                ls = 1;

            if (top_block.ok)
              if (slice->siBlock [picPos[top_block.mbIndex].y][picPos[top_block.mbIndex].x])
                ts = 1;
            }
            //}}}

          upIntraPredMode = (top_block.ok  &&(ts == 0)) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
          leftIntraPredMode = (left_block.ok &&(ls == 0)) ? slice->predMode[left_block.posY][left_block.posX] : -1;
          mostProbableIntraPredMode  = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
          slice->predMode[bj][bi] = (uint8_t)((se.value1 == -1) ?
            mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode));
          }
        }
      }
    }
  //}}}
  //{{{
  void readIpredModes (sMacroBlock* mb) {

    cSlice* slice = mb->slice;
    sPicture* picture = slice->picture;

    if (slice->mbAffFrame) {
      if (mb->mbType == I8MB)
        read_ipred_8x8_modes_mbaff(mb);
      else if (mb->mbType == I4MB)
        read_ipred_4x4_modes_mbaff(mb);
      }
    else {
      if (mb->mbType == I8MB)
        read_ipred_8x8_modes(mb);
      else if (mb->mbType == I4MB)
        read_ipred_4x4_modes(mb);
      }

    if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
      sSyntaxElement se;
      sDataPartition* dataPartition;
      const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
      cDecoder264* decoder = mb->decoder;

      se.type = SE_INTRAPREDMODE;
      dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);

      if (decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag)
        se.mapping = cBitStream::linfo_ue;
      else
        se.reading = readCiPredModCabac;

      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->chromaPredMode = (char)se.value1;

      if (mb->chromaPredMode < DC_PRED_8 || mb->chromaPredMode > PLANE_8)
        cDecoder264::error ("illegal chroma intra pred mode!\n");
      }
    }
  //}}}

  //{{{
  void resetMvInfo (sPicMotion* mvInfo, int slice_no) {

    mvInfo->refPic[LIST_0] = NULL;
    mvInfo->refPic[LIST_1] = NULL;
    mvInfo->mv[LIST_0] = kZeroMv;
    mvInfo->mv[LIST_1] = kZeroMv;
    mvInfo->refIndex[LIST_0] = -1;
    mvInfo->refIndex[LIST_1] = -1;
    mvInfo->slice_no = slice_no;
    }
  //}}}
  //{{{
  void initMacroBlock (sMacroBlock* mb) {

    cSlice* slice = mb->slice;
    sPicMotion** mvInfo = &slice->picture->mvInfo[mb->blockY];
    int slice_no = slice->curSliceIndex;

    // reset vectors and pred. modes
    for(int j = 0; j < BLOCK_SIZE; ++j) {
      int i = mb->blockX;
      resetMvInfo (*mvInfo + (i++), slice_no);
      resetMvInfo (*mvInfo + (i++), slice_no);
      resetMvInfo (*mvInfo + (i++), slice_no);
      resetMvInfo (*(mvInfo++) + i, slice_no);
      }

    setReadCompCabac (mb);
    setReadCompCoefCavlc (mb);
    }
  //}}}
  //{{{
  void initMacroBlockDirect (sMacroBlock* mb) {

    int slice_no = mb->slice->curSliceIndex;
    sPicMotion** mvInfo = &mb->slice->picture->mvInfo[mb->blockY];

    setReadCompCabac (mb);
    setReadCompCoefCavlc (mb);

    int i = mb->blockX;
    for (int j = 0; j < BLOCK_SIZE; ++j) {
      (*mvInfo+i)->slice_no = slice_no;
      (*mvInfo+i+1)->slice_no = slice_no;
      (*mvInfo+i+2)->slice_no = slice_no;
      (*(mvInfo++)+i+3)->slice_no = slice_no;
      }
    }
  //}}}

  //{{{
  void concealIPCMcoeffs (sMacroBlock* mb) {

    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;
    for (int i = 0; i < MB_BLOCK_SIZE; ++i)
      for (int j = 0; j < MB_BLOCK_SIZE; ++j)
        slice->cof[0][i][j] = decoder->coding.dcPredValueComp[0];

    sPicture* picture = slice->picture;
    if ((picture->chromaFormatIdc != YUV400) && (decoder->coding.isSeperateColourPlane == 0))
      for (int k = 0; k < 2; ++k)
        for (int i = 0; i < decoder->mbCrSizeY; ++i)
          for (int j = 0; j < decoder->mbCrSizeX; ++j)
            slice->cof[k][i][j] = decoder->coding.dcPredValueComp[k];
    }
  //}}}
  //{{{
  void initIPCMdecoding (cSlice* slice) {

    int dpNum;
    if (slice->dataPartitionMode == eDataPartition1)
      dpNum = 1;
    else if (slice->dataPartitionMode == eDataPartition3)
      dpNum = 3;
    else {
      printf ("dataPartition Mode is not supported\n");
      exit (1);
      }

    for (int i = 0; i < dpNum;++i) {
      cBitStream& bitStream = slice->dataPartitions[i].bitStream;
      int byteStartPosition = bitStream.readLen;
      slice->dataPartitions[i].cabacDecode.startDecoding (bitStream.bitStreamBuffer, byteStartPosition, &bitStream.readLen);
      }
    }
  //}}}
  //{{{
  void readIPCMcoeffs (cSlice* slice, sDataPartition* dataPartition) {

    cDecoder264* decoder = slice->decoder;

    //For eCabac, we don't need to read bits to let stream uint8_t aligned
    //  because we have variable for integer bytes position
    if (decoder->activePps->entropyCoding == eCabac) {
      readIpcmCabac (slice, dataPartition);
      initIPCMdecoding (slice);
      }
    else {
      // read bits to let stream uint8_t aligned
      sSyntaxElement se;
      if (((dataPartition->bitStream.bitStreamOffset) & 0x07) != 0) {
        se.len = (8 - ((dataPartition->bitStream.bitStreamOffset) & 0x07));
        dataPartition->bitStream.readSyntaxElement_FLC (&se);
        }

      //read luma and chroma IPCM coefficients
      se.len = decoder->bitDepthLuma;
      for (int i = 0; i < MB_BLOCK_SIZE;++i)
        for (int j = 0; j < MB_BLOCK_SIZE;++j) {
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          slice->cof[0][i][j] = se.value1;
          }

      se.len = decoder->bitDepthChroma;
      sPicture* picture = slice->picture;
      if ((picture->chromaFormatIdc != YUV400) && (decoder->coding.isSeperateColourPlane == 0)) {
        for (int i = 0; i < decoder->mbCrSizeY; ++i)
          for (int j = 0; j < decoder->mbCrSizeX; ++j) {
            dataPartition->bitStream.readSyntaxElement_FLC (&se);
            slice->cof[1][i][j] = se.value1;
            }

        for (int i = 0; i < decoder->mbCrSizeY; ++i)
          for (int j = 0; j < decoder->mbCrSizeX; ++j) {
            dataPartition->bitStream.readSyntaxElement_FLC (&se);
            slice->cof[2][i][j] = se.value1;
            }
        }
      }
    }
  //}}}

  //{{{
  void SetB8Mode (sMacroBlock* mb, int value, int i) {

    cSlice* slice = mb->slice;
    static const char p_v2b8 [ 5] = {4, 5, 6, 7, IBLOCK};
    static const char p_v2pd [ 5] = {0, 0, 0, 0, -1};
    static const char b_v2b8 [14] = {0, 4, 4, 4, 5, 6, 5, 6, 5, 6, 7, 7, 7, IBLOCK};
    static const char b_v2pd [14] = {2, 0, 1, 2, 0, 0, 1, 1, 2, 2, 0, 1, 2, -1};

    if (slice->sliceType == eSliceB) {
      mb->b8mode[i] = b_v2b8[value];
      mb->b8pdir[i] = b_v2pd[value];
      }
    else {
      mb->b8mode[i] = p_v2b8[value];
      mb->b8pdir[i] = p_v2pd[value];
      }
    }
  //}}}

  //{{{
  void resetCoeffs (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;

    if (decoder->activePps->entropyCoding == eCavlc)
      memset (decoder->nzCoeff[mb->mbIndexX][0][0], 0, 3 * BLOCK_PIXELS * sizeof(uint8_t));
    }
  //}}}
  //{{{
  void fieldFlagInference (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    if (mb->mbAvailA)
      mb->mbField = decoder->mbData[mb->mbIndexA].mbField;
    else
      // check top macroBlock pair
      mb->mbField = mb->mbAvailB ? decoder->mbData[mb->mbIndexB].mbField : false;
    }
  //}}}

  //{{{
  void skipMacroBlocks (sMacroBlock* mb) {

    sMotionVec pred_mv;
    int zeroMotionAbove;
    int zeroMotionLeft;
    sPixelPos neighbourMb[4];
    int i, j;
    int a_mv_y = 0;
    int a_ref_idx = 0;
    int b_mv_y = 0;
    int b_ref_idx = 0;
    int img_block_y   = mb->blockY;
    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;
    int   listOffset = LIST_0 + mb->listOffset;
    sPicture* picture = slice->picture;
    sMotionVec* a_mv = NULL;
    sMotionVec* b_mv = NULL;

    getNeighbours (mb, neighbourMb, 0, 0, MB_BLOCK_SIZE);
    if (slice->mbAffFrame == 0) {
      if (neighbourMb[0].ok) {
        a_mv = &picture->mvInfo[neighbourMb[0].posY][neighbourMb[0].posX].mv[LIST_0];
        a_mv_y = a_mv->mvY;
        a_ref_idx = picture->mvInfo[neighbourMb[0].posY][neighbourMb[0].posX].refIndex[LIST_0];
        }

      if (neighbourMb[1].ok) {
        b_mv = &picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].mv[LIST_0];
        b_mv_y = b_mv->mvY;
        b_ref_idx = picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].refIndex[LIST_0];
        }
      }
    else {
      if (neighbourMb[0].ok) {
        a_mv = &picture->mvInfo[neighbourMb[0].posY][neighbourMb[0].posX].mv[LIST_0];
        a_mv_y = a_mv->mvY;
        a_ref_idx = picture->mvInfo[neighbourMb[0].posY][neighbourMb[0].posX].refIndex[LIST_0];

        if (mb->mbField && !decoder->mbData[neighbourMb[0].mbIndex].mbField) {
          a_mv_y /= 2;
          a_ref_idx *= 2;
          }
        if (!mb->mbField && decoder->mbData[neighbourMb[0].mbIndex].mbField) {
          a_mv_y *= 2;
          a_ref_idx >>= 1;
          }
        }

      if (neighbourMb[1].ok) {
        b_mv = &picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].mv[LIST_0];
        b_mv_y = b_mv->mvY;
        b_ref_idx = picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].refIndex[LIST_0];

        if (mb->mbField && !decoder->mbData[neighbourMb[1].mbIndex].mbField) {
          b_mv_y /= 2;
          b_ref_idx *= 2;
          }
        if (!mb->mbField && decoder->mbData[neighbourMb[1].mbIndex].mbField) {
          b_mv_y *= 2;
          b_ref_idx >>= 1;
          }
        }
      }

    zeroMotionLeft = !neighbourMb[0].ok ?
                       1 : (a_ref_idx==0 && a_mv->mvX == 0 && a_mv_y == 0) ? 1 : 0;
    zeroMotionAbove = !neighbourMb[1].ok ?
                       1 : (b_ref_idx==0 && b_mv->mvX == 0 && b_mv_y == 0) ? 1 : 0;

    mb->codedBlockPattern = 0;
    resetCoeffs (mb);

    if (zeroMotionAbove || zeroMotionLeft) {
      sPicMotion** dec_mv_info = &picture->mvInfo[img_block_y];
      sPicture* slicePic = slice->listX[listOffset][0];
      sPicMotion* mvInfo = NULL;

      for (j = 0; j < BLOCK_SIZE; ++j) {
        for (i = mb->blockX; i < mb->blockX + BLOCK_SIZE; ++i) {
          mvInfo = &dec_mv_info[j][i];
          mvInfo->refPic[LIST_0] = slicePic;
          mvInfo->mv [LIST_0] = kZeroMv;
          mvInfo->refIndex[LIST_0] = 0;
          }
        }
      }
    else {
      sPicMotion** dec_mv_info = &picture->mvInfo[img_block_y];
      sPicMotion* mvInfo = NULL;
      sPicture* slicePic = slice->listX[listOffset][0];
      mb->GetMVPredictor (mb, neighbourMb, &pred_mv, 0, picture->mvInfo, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

      // Set first block line (position img_block_y)
      for (j = 0; j < BLOCK_SIZE; ++j) {
        for (i = mb->blockX; i < mb->blockX + BLOCK_SIZE; ++i) {
          mvInfo = &dec_mv_info[j][i];
          mvInfo->refPic[LIST_0] = slicePic;
          mvInfo->mv[LIST_0] = pred_mv;
          mvInfo->refIndex[LIST_0] = 0;
          }
        }
      }
    }
  //}}}
  //{{{
  void resetMvInfoList (sPicMotion* mvInfo, int list, int sliceNum) {

    mvInfo->refPic[list] = NULL;
    mvInfo->mv[list] = kZeroMv;
    mvInfo->refIndex[list] = -1;
    mvInfo->slice_no = sliceNum;
    }
  //}}}
  //{{{
  void initMacroBlockBasic (sMacroBlock* mb) {

    sPicMotion** mvInfo = &mb->slice->picture->mvInfo[mb->blockY];
    int slice_no = mb->slice->curSliceIndex;

    // reset vectors and pred. modes
    for (int j = 0; j < BLOCK_SIZE; ++j) {
      int i = mb->blockX;
      resetMvInfoList (*mvInfo + (i++), LIST_1, slice_no);
      resetMvInfoList (*mvInfo + (i++), LIST_1, slice_no);
      resetMvInfoList (*mvInfo + (i++), LIST_1, slice_no);
      resetMvInfoList (*(mvInfo++) + i, LIST_1, slice_no);
      }
    }
  //}}}
  //{{{
  void readSkipMacroBlock (sMacroBlock* mb) {

    mb->lumaTransformSize8x8flag = false;
    if (mb->decoder->activePps->hasConstrainedIntraPred) {
      int mbNum = mb->mbIndexX;
      mb->slice->intraBlock[mbNum] = 0;
      }

    initMacroBlockBasic (mb);
    skipMacroBlocks (mb);
    }
  //}}}

  //{{{
  void readIntraMacroBlock (sMacroBlock* mb) {

    mb->noMbPartLessThan8x8Flag = true;

    // transform size flag for INTRA_4x4 and INTRA_8x8 modes
    mb->lumaTransformSize8x8flag = false;

    initMacroBlock (mb);
    readIpredModes (mb);
    mb->slice->readCBPcoeffs (mb);
    }
  //}}}
  //{{{
  void readIntra4x4macroblocCavlc (sMacroBlock* mb, const uint8_t* dpMap)
  {
    cSlice* slice = mb->slice;

    //transform size flag for INTRA_4x4 and INTRA_8x8 modes
    if (slice->transform8x8Mode) {
      sSyntaxElement se;
      sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
      se.type = SE_HEADER;

      // read eCavlc transform_size_8x8Flag
      se.len = (int64_t)1;
      dataPartition->bitStream.readSyntaxElement_FLC (&se);
      mb->lumaTransformSize8x8flag = (bool)se.value1;
      if (mb->lumaTransformSize8x8flag) {
        mb->mbType = I8MB;
        memset (&mb->b8mode, I8MB, 4 * sizeof(char));
        memset (&mb->b8pdir, -1, 4 * sizeof(char));
        }
      }
    else
      mb->lumaTransformSize8x8flag = false;

    initMacroBlock (mb);
    readIpredModes (mb);
    slice->readCBPcoeffs (mb);
    }
  //}}}
  //{{{
  void readIntra4x4macroBlockCabac (sMacroBlock* mb, const uint8_t* dpMap) {


    // transform size flag for INTRA_4x4 and INTRA_8x8 modes
    cSlice* slice = mb->slice;
    if (slice->transform8x8Mode) {
     sSyntaxElement se;
      sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
      se.type = SE_HEADER;
      se.reading = readMbTransformSizeFlagCabac;
      // read eCavlc transform_size_8x8Flag
      if (dataPartition->bitStream.errorFlag) {
        se.len = (int64_t) 1;
        dataPartition->bitStream.readSyntaxElement_FLC (&se);
        }
      else
        dataPartition->readSyntaxElement (mb, &se, dataPartition);

      mb->lumaTransformSize8x8flag = (bool)se.value1;
      if (mb->lumaTransformSize8x8flag) {
        mb->mbType = I8MB;
        memset (&mb->b8mode, I8MB, 4 * sizeof(char));
        memset (&mb->b8pdir, -1, 4 * sizeof(char));
        }
      }
    else
      mb->lumaTransformSize8x8flag = false;

    initMacroBlock (mb);
    readIpredModes (mb);
    slice->readCBPcoeffs (mb);
    }
  //}}}

  //{{{
  void readInterMacroBlock (sMacroBlock* mb) {

    cSlice* slice = mb->slice;

    mb->noMbPartLessThan8x8Flag = true;
    mb->lumaTransformSize8x8flag = false;
    if (mb->decoder->activePps->hasConstrainedIntraPred) {
      int mbNum = mb->mbIndexX;
      slice->intraBlock[mbNum] = 0;
      }

    initMacroBlock (mb);
    slice->nalReadMotionInfo (mb);
    slice->readCBPcoeffs (mb);
    }
  //}}}
  //{{{
  void readIPCMmacroBlock (sMacroBlock* mb, const uint8_t* dpMap) {

    cSlice* slice = mb->slice;
    mb->noMbPartLessThan8x8Flag = true;
    mb->lumaTransformSize8x8flag = false;

    initMacroBlock (mb);

    // here dataPartition is assigned with the same dataPartition as SE_MBTYPE, because IPCM syntax is in the
    // same category as MBTYPE
    if (slice->dataPartitionMode && slice->noDataPartitionB )
      concealIPCMcoeffs (mb);
    else {
      sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_LUM_DC_INTRA]]);
      readIPCMcoeffs (slice, dataPartition);
      }
    }
  //}}}
  //{{{
  void readI8x8macroBlock (sMacroBlock* mb, sDataPartition* dataPartition, sSyntaxElement* se) {

    int i;
    cSlice* slice = mb->slice;

    // READ 8x8 SUB-dataPartition MODES (modes of 8x8 blocks) and Intra VBST block modes
    mb->noMbPartLessThan8x8Flag = true;
    mb->lumaTransformSize8x8flag = false;

    for (i = 0; i < 4; ++i) {
      dataPartition->readSyntaxElement (mb, se, dataPartition);
      SetB8Mode (mb, se->value1, i);

      // set noMbPartLessThan8x8Flag for P8x8 mode
      mb->noMbPartLessThan8x8Flag &= (mb->b8mode[i] == 0 && slice->activeSps->isDirect8x8inference) ||
        (mb->b8mode[i] == 4);
      }

    initMacroBlock (mb);
    slice->nalReadMotionInfo (mb);

    if (mb->decoder->activePps->hasConstrainedIntraPred) {
      int mbNum = mb->mbIndexX;
      slice->intraBlock[mbNum] = 0;
      }

    slice->readCBPcoeffs (mb);
    }
  //}}}

  //{{{
  void readCavlcMacroBlockI (sMacroBlock* mb) {

    cSlice* slice = mb->slice;

    sSyntaxElement se;
    int mbNum = mb->mbIndexX;

    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    sPicture* picture = slice->picture;
    sPicMotionOld* motion = &picture->motion;

    mb->mbField = ((mbNum & 0x01) == 0) ? false : slice->mbData[mbNum-1].mbField;

    updateQp (mb, slice->qp);
    se.type = SE_MBTYPE;

    // read MB mode
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    se.mapping = cBitStream::linfo_ue;
    // read MB aff
    if (slice->mbAffFrame && (mbNum & 0x01) == 0) {
      se.len = (int64_t) 1;
      dataPartition->bitStream.readSyntaxElement_FLC (&se);
      mb->mbField = (bool)se.value1;
      }

    // read MB type
    dataPartition->readSyntaxElement (mb, &se, dataPartition);
    mb->mbType = (int16_t)se.value1;
    if (!dataPartition->bitStream.errorFlag)
      mb->errorFlag = 0;

    motion->mbField[mbNum] = (uint8_t) mb->mbField;
    mb->blockYaff = ((slice->mbAffFrame) && (mb->mbField)) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);

    mb->noMbPartLessThan8x8Flag = true;
    if(mb->mbType == IPCM)
      readIPCMmacroBlock (mb, dpMap);
    else if (mb->mbType == I4MB)
      readIntra4x4macroblocCavlc (mb, dpMap);
    else
      readIntraMacroBlock (mb);
    }
  //}}}
  //{{{
  void readCavlcMacroBlockP (sMacroBlock* mb) {

    cSlice* slice = mb->slice;
    sSyntaxElement se;
    int mbNum = mb->mbIndexX;

    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

    if (slice->mbAffFrame == 0) {
      sPicture* picture = slice->picture;
      sPicMotionOld* motion = &picture->motion;

      mb->mbField = false;
      updateQp (mb, slice->qp);

      //  read MB mode
      se.type = SE_MBTYPE;
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      se.mapping = cBitStream::linfo_ue;

      // VLC Non-Intra
      if (slice->codCount == -1) {
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        slice->codCount = se.value1;
        }

      if (slice->codCount==0) {
        // read MB type
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        ++(se.value1);
        mb->mbType = (int16_t)se.value1;
        if(!dataPartition->bitStream.errorFlag)
          mb->errorFlag = 0;
        slice->codCount--;
        mb->skipFlag = 0;
        }
      else {
        slice->codCount--;
        mb->mbType = 0;
        mb->errorFlag = 0;
        mb->skipFlag = 1;
        }

      // update the list offset;
      mb->listOffset = 0;
      motion->mbField[mbNum] = (uint8_t) false;
      mb->blockYaff = mb->blockY;
      slice->siBlock[mb->mb.y][mb->mb.x] = 0;
      slice->interpretMbMode (mb);
      }

    else {
      cDecoder264* decoder = mb->decoder;
      sMacroBlock* topMB = NULL;
      int  prevMbSkipped = 0;
      sPicture* picture = slice->picture;
      sPicMotionOld* motion = &picture->motion;

      if (mbNum & 0x01) {
        topMB= &decoder->mbData[mbNum-1];
        prevMbSkipped = (topMB->mbType == 0);
        }
      else
        prevMbSkipped = 0;

      mb->mbField = ((mbNum & 0x01) == 0)? false : decoder->mbData[mbNum-1].mbField;
      updateQp (mb, slice->qp);

      //  read MB mode
      se.type = SE_MBTYPE;
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      se.mapping = cBitStream::linfo_ue;

      // VLC Non-Intra
      if (slice->codCount == -1) {
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        slice->codCount = se.value1;
        }

      if (slice->codCount == 0) {
        // read MB aff
        if ((((mbNum & 0x01) == 0) || ((mbNum & 0x01) && prevMbSkipped))) {
          se.len = (int64_t) 1;
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          mb->mbField = (bool)se.value1;
          }

        // read MB type
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        ++(se.value1);
        mb->mbType = (int16_t)se.value1;
        if(!dataPartition->bitStream.errorFlag)
          mb->errorFlag = 0;
        slice->codCount--;
        mb->skipFlag = 0;
        }

      else {
        slice->codCount--;
        mb->mbType = 0;
        mb->errorFlag = 0;
        mb->skipFlag = 1;

        // read field flag of bottom block
        if (slice->codCount == 0 && ((mbNum & 0x01) == 0)) {
          se.len = (int64_t) 1;
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          dataPartition->bitStream.bitStreamOffset--;
          mb->mbField = (bool)se.value1;
          }

        else if (slice->codCount > 0 && ((mbNum & 0x01) == 0)) {
          // check left macroBlock pair first
          if (isMbAvailable (mbNum - 2, mb) && ((mbNum % (decoder->coding.picWidthMbs * 2))!=0))
            mb->mbField = decoder->mbData[mbNum-2].mbField;
          else {
            // check top macroBlock pair
            if (isMbAvailable (mbNum - 2*decoder->coding.picWidthMbs, mb))
              mb->mbField = decoder->mbData[mbNum - 2*decoder->coding.picWidthMbs].mbField;
            else
              mb->mbField = false;
            }
          }
        }
      // update the list offset;
      mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;
      motion->mbField[mbNum] = (uint8_t) mb->mbField;
      mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
      slice->siBlock[mb->mb.y][mb->mb.x] = 0;
      slice->interpretMbMode (mb);
      if (mb->mbField) {
        slice->numRefIndexActive[LIST_0] <<=1;
        slice->numRefIndexActive[LIST_1] <<=1;
        }
      }

    mb->noMbPartLessThan8x8Flag = true;
    if (mb->mbType == IPCM)
      readIPCMmacroBlock (mb, dpMap);
    else if (mb->mbType == I4MB)
      readIntra4x4macroblocCavlc (mb, dpMap);
    else if (mb->mbType == P8x8) {
      se.type = SE_MBTYPE;
      se.mapping = cBitStream::linfo_ue;
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      readI8x8macroBlock (mb, dataPartition, &se);
      }
    else if (mb->mbType == PSKIP)
      readSkipMacroBlock (mb);
    else if (mb->isIntraBlock)
      readIntraMacroBlock (mb);
    else
      readInterMacroBlock (mb);
    }
  //}}}
  //{{{
  void readCavlcMacroBlockB (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;
    int mbNum = mb->mbIndexX;
    sSyntaxElement se;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

    if (slice->mbAffFrame == 0) {
      sPicture* picture = slice->picture;
      sPicMotionOld *motion = &picture->motion;

      mb->mbField = false;
      updateQp(mb, slice->qp);

      //  read MB mode
      se.type = SE_MBTYPE;
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      se.mapping = cBitStream::linfo_ue;

      if (slice->codCount == -1) {
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        slice->codCount = se.value1;
        }
      if (slice->codCount==0) {
        // read MB type
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->mbType = (int16_t)se.value1;
        if (!dataPartition->bitStream.errorFlag)
          mb->errorFlag = 0;
        slice->codCount--;
        mb->skipFlag = 0;
        }
      else {
        slice->codCount--;
        mb->mbType = 0;
        mb->errorFlag = 0;
        mb->skipFlag = 1;
        }

      // update the list offset;
      mb->listOffset = 0;
      motion->mbField[mbNum] = false;
      mb->blockYaff = mb->blockY;
      slice->siBlock[mb->mb.y][mb->mb.x] = 0;
      slice->interpretMbMode (mb);
      }
    else {
      sMacroBlock* topMB = NULL;
      int prevMbSkipped = 0;
      sPicture* picture = slice->picture;
      sPicMotionOld* motion = &picture->motion;

      if (mbNum & 0x01) {
        topMB = &decoder->mbData[mbNum-1];
        prevMbSkipped = topMB->skipFlag;
        }
      else
        prevMbSkipped = 0;

      mb->mbField = ((mbNum & 0x01) == 0) ? false : decoder->mbData[mbNum-1].mbField;

      updateQp (mb, slice->qp);

      //  read MB mode
      se.type = SE_MBTYPE;
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      se.mapping = cBitStream::linfo_ue;
      if (slice->codCount == -1) {
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        slice->codCount = se.value1;
        }

      if (slice->codCount == 0) {
        // read MB aff
        if (((mbNum & 0x01) == 0) || ((mbNum & 0x01) && prevMbSkipped)) {
          se.len = (int64_t) 1;
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          mb->mbField = (bool)se.value1;
          }

        // read MB type
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->mbType = (int16_t)se.value1;
        if (!dataPartition->bitStream.errorFlag)
          mb->errorFlag = 0;
        slice->codCount--;
        mb->skipFlag = 0;
        }
      else {
        slice->codCount--;
        mb->mbType = 0;
        mb->errorFlag = 0;
        mb->skipFlag = 1;

        // read field flag of bottom block
        if ((slice->codCount == 0) && ((mbNum & 0x01) == 0)) {
          se.len = (int64_t) 1;
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          dataPartition->bitStream.bitStreamOffset--;
          mb->mbField = (bool)se.value1;
          }
        else if ((slice->codCount > 0) && ((mbNum & 0x01) == 0)) {
          // check left macroBlock pair first
          if (isMbAvailable (mbNum - 2, mb) && ((mbNum % (decoder->coding.picWidthMbs * 2))!=0))
            mb->mbField = decoder->mbData[mbNum-2].mbField;
          else {
            // check top macroBlock pair
            if (isMbAvailable (mbNum - 2*decoder->coding.picWidthMbs, mb))
              mb->mbField = decoder->mbData[mbNum-2*decoder->coding.picWidthMbs].mbField;
            else
              mb->mbField = false;
            }
          }
        }

      // update the list offset;
      mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;
      motion->mbField[mbNum] = (uint8_t)mb->mbField;
      mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
      slice->siBlock[mb->mb.y][mb->mb.x] = 0;
      slice->interpretMbMode (mb);
      if (slice->mbAffFrame) {
        if (mb->mbField) {
          slice->numRefIndexActive[LIST_0] <<=1;
          slice->numRefIndexActive[LIST_1] <<=1;
          }
        }
      }

    if (mb->mbType == IPCM)
      readIPCMmacroBlock (mb, dpMap);
    else if (mb->mbType == I4MB)
      readIntra4x4macroblocCavlc (mb, dpMap);
    else if (mb->mbType == P8x8) {
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      se.type = SE_MBTYPE;
      se.mapping = cBitStream::linfo_ue;
      readI8x8macroBlock (mb, dataPartition, &se);
      }
    else if (mb->mbType == BSKIP_DIRECT) {
      // init noMbPartLessThan8x8Flag
      mb->noMbPartLessThan8x8Flag = (!(slice->activeSps->isDirect8x8inference))? false: true;
      mb->lumaTransformSize8x8flag = false;
      if(decoder->activePps->hasConstrainedIntraPred)
        slice->intraBlock[mbNum] = 0;

      initMacroBlockDirect (mb);
      if (slice->codCount >= 0) {
        mb->codedBlockPattern = 0;
        resetCoeffs (mb);
        }
      else
        slice->readCBPcoeffs (mb);
      }
    else if (mb->isIntraBlock == true)
      readIntraMacroBlock (mb);
    else
      readInterMacroBlock (mb);
    }
  //}}}

  //{{{
  void readIcabacMacroBlock (sMacroBlock* mb) {

    cSlice* slice = mb->slice;

    sSyntaxElement se;
    int mbNum = mb->mbIndexX;

    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    sPicture* picture = slice->picture;
    sPicMotionOld* motion = &picture->motion;

    mb->mbField = ((mbNum & 0x01) == 0) ? false : slice->mbData[mbNum-1].mbField;

    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    if (dataPartition->bitStream.errorFlag)
      se.mapping = cBitStream::linfo_ue;

    // read MB aff
    if (slice->mbAffFrame && (mbNum & 0x01) == 0) {
      if (dataPartition->bitStream.errorFlag) {
        se.len = (int64_t)1;
        dataPartition->bitStream.readSyntaxElement_FLC (&se);
        }
      else {
        se.reading = readFieldModeCabac;
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        }
      mb->mbField = (bool)se.value1;
      }

    checkNeighbourCabac(mb);

    //  read MB type
    se.reading = readMbTypeInfoCabacSliceI;
    dataPartition->readSyntaxElement (mb, &se, dataPartition);

    mb->mbType = (int16_t)se.value1;
    if (!dataPartition->bitStream.errorFlag)
      mb->errorFlag = 0;

    motion->mbField[mbNum] = (uint8_t) mb->mbField;
    mb->blockYaff = ((slice->mbAffFrame) && (mb->mbField)) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);

    // init noMbPartLessThan8x8Flag
    mb->noMbPartLessThan8x8Flag = true;
    if (mb->mbType == IPCM)
      readIPCMmacroBlock (mb, dpMap);
    else if (mb->mbType == I4MB) {
      // transform size flag for INTRA_4x4 and INTRA_8x8 modes
      if (slice->transform8x8Mode) {
        se.type = SE_HEADER;
        dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
        se.reading = readMbTransformSizeFlagCabac;

        // read eCavlc transform_size_8x8Flag
        if (dataPartition->bitStream.errorFlag) {
          se.len = (int64_t) 1;
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          }
        else
          dataPartition->readSyntaxElement (mb, &se, dataPartition);

        mb->lumaTransformSize8x8flag = (bool)se.value1;
        if (mb->lumaTransformSize8x8flag) {
          mb->mbType = I8MB;
          memset (&mb->b8mode, I8MB, 4 * sizeof(char));
          memset (&mb->b8pdir, -1, 4 * sizeof(char));
          }
        }
      else
        mb->lumaTransformSize8x8flag = false;

      initMacroBlock (mb);
      readIpredModes (mb);
      slice->readCBPcoeffs (mb);
      }
    else
      readIntraMacroBlock (mb);
    }
  //}}}
  //{{{
  void readPcabacMacroBlock (sMacroBlock* mb)
  {
    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;
    int mbNum = mb->mbIndexX;
    sSyntaxElement se;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

    if (slice->mbAffFrame == 0) {
      sPicture* picture = slice->picture;
      sPicMotionOld* motion = &picture->motion;

      mb->mbField = false;
      updateQp (mb, slice->qp);

      // read MB mode
      se.type = SE_MBTYPE;
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      if (dataPartition->bitStream.errorFlag)
        se.mapping = cBitStream::linfo_ue;

      checkNeighbourCabac(mb);
      se.reading = readSkipFlagCabacSliceP;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);

      mb->mbType = (int16_t) se.value1;
      mb->skipFlag = (char) (!(se.value1));
      if (!dataPartition->bitStream.errorFlag)
        mb->errorFlag = 0;

      // read MB type
      if (mb->mbType != 0 ) {
        se.reading = readMbTypeInfoCabacSliceP;
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->mbType = (int16_t) se.value1;
        if(!dataPartition->bitStream.errorFlag)
          mb->errorFlag = 0;
        }

      motion->mbField[mbNum] = (uint8_t) false;
      mb->blockYaff = mb->blockY;
      slice->siBlock[mb->mb.y][mb->mb.x] = 0;
      slice->interpretMbMode (mb);
      }

    else {
      sMacroBlock* topMB = NULL;
      int prevMbSkipped = 0;
      int checkBot, readBot, readTop;
      sPicture* picture = slice->picture;
      sPicMotionOld* motion = &picture->motion;
      if (mbNum & 0x01) {
        topMB = &decoder->mbData[mbNum-1];
        prevMbSkipped = (topMB->mbType == 0);
        }
      else
        prevMbSkipped = 0;

      mb->mbField = ((mbNum & 0x01) == 0) ? false : decoder->mbData[mbNum-1].mbField;
      updateQp (mb, slice->qp);

      //  read MB mode
      se.type = SE_MBTYPE;
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      if (dataPartition->bitStream.errorFlag)
        se.mapping = cBitStream::linfo_ue;

      // read MB skipFlag
      if (((mbNum & 0x01) == 0||prevMbSkipped))
        fieldFlagInference (mb);

      checkNeighbourCabac(mb);
      se.reading = readSkipFlagCabacSliceP;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);

      mb->mbType = (int16_t)se.value1;
      mb->skipFlag = (char)(!(se.value1));

      if (!dataPartition->bitStream.errorFlag)
        mb->errorFlag = 0;

      // read MB AFF
      checkBot = readBot = readTop = 0;
      if ((mbNum & 0x01) == 0) {
        checkBot = mb->skipFlag;
        readTop = !checkBot;
        }
      else
        readBot = (topMB->skipFlag && (!mb->skipFlag));

      if (readBot || readTop) {
        se.reading = readFieldModeCabac;
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->mbField = (bool)se.value1;
        }

      if (checkBot)
        checkNextMbGetFieldModeCabacSliceP (slice, &se, dataPartition);

      // update the list offset;
      mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4 : 2) : 0;
      checkNeighbourCabac (mb);

      // read MB type
      if (mb->mbType != 0 ) {
        se.reading = readMbTypeInfoCabacSliceP;
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->mbType = (int16_t) se.value1;
        if (!dataPartition->bitStream.errorFlag)
          mb->errorFlag = 0;
        }

      motion->mbField[mbNum] = (uint8_t) mb->mbField;
      mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
      slice->siBlock[mb->mb.y][mb->mb.x] = 0;
      slice->interpretMbMode(mb);
      if (mb->mbField) {
        slice->numRefIndexActive[LIST_0] <<=1;
        slice->numRefIndexActive[LIST_1] <<=1;
        }
      }

    mb->noMbPartLessThan8x8Flag = true;
    if (mb->mbType == IPCM)
      readIPCMmacroBlock (mb, dpMap);
    else if (mb->mbType == I4MB)
      readIntra4x4macroBlockCabac (mb, dpMap);
    else if (mb->mbType == P8x8) {
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      se.type = SE_MBTYPE;

      if (dataPartition->bitStream.errorFlag)
        se.mapping = cBitStream::linfo_ue;
      else
        se.reading = readB8TypeInfoCabacSliceP;

      readI8x8macroBlock (mb, dataPartition, &se);
      }
    else if (mb->mbType == PSKIP)
      readSkipMacroBlock (mb);
    else if (mb->isIntraBlock == true)
      readIntraMacroBlock (mb);
    else
      readInterMacroBlock (mb);
    }
  //}}}
  //{{{
  void readBcabacMacroBlock (sMacroBlock* mb) {

    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;
    int mbNum = mb->mbIndexX;
    sSyntaxElement se;

    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

    if (slice->mbAffFrame == 0) {
      sPicture* picture = slice->picture;
      sPicMotionOld* motion = &picture->motion;

      mb->mbField = false;
      updateQp(mb, slice->qp);

      //  read MB mode
      se.type = SE_MBTYPE;
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      if (dataPartition->bitStream.errorFlag)
        se.mapping = cBitStream::linfo_ue;

      checkNeighbourCabac(mb);
      se.reading = readSkipFlagCabacSliceB;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);

      mb->mbType  = (int16_t)se.value1;
      mb->skipFlag = (char)(!(se.value1));
      mb->codedBlockPattern = se.value2;
      if (!dataPartition->bitStream.errorFlag)
        mb->errorFlag = 0;

      if (se.value1 == 0 && se.value2 == 0)
        slice->codCount=0;

      // read MB type
      if (mb->mbType != 0 ) {
        se.reading = readMbTypeInfoCabacSliceB;
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->mbType = (int16_t)se.value1;
        if (!dataPartition->bitStream.errorFlag)
          mb->errorFlag = 0;
        }

      motion->mbField[mbNum] = (uint8_t) false;
      mb->blockYaff = mb->blockY;
      slice->siBlock[mb->mb.y][mb->mb.x] = 0;
      slice->interpretMbMode (mb);
      }
    else {
      sMacroBlock* topMB = NULL;
      int  prevMbSkipped = 0;
      int  checkBot, readBot, readTop;
      sPicture* picture = slice->picture;
      sPicMotionOld* motion = &picture->motion;

      if (mbNum & 0x01) {
        topMB = &decoder->mbData[mbNum-1];
        prevMbSkipped = topMB->skipFlag;
        }
      else
        prevMbSkipped = 0;

      mb->mbField = ((mbNum & 0x01) == 0)? false : decoder->mbData[mbNum-1].mbField;

      updateQp (mb, slice->qp);

      //  read MB mode
      se.type = SE_MBTYPE;
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      if (dataPartition->bitStream.errorFlag)
        se.mapping = cBitStream::linfo_ue;

      // read MB skipFlag
      if ((mbNum & 0x01) == 0 || prevMbSkipped)
        fieldFlagInference (mb);

      checkNeighbourCabac (mb);
      se.reading = readSkipFlagCabacSliceB;

      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (int16_t)se.value1;
      mb->skipFlag = (char)(!(se.value1));
      mb->codedBlockPattern = se.value2;
      if (!dataPartition->bitStream.errorFlag)
        mb->errorFlag = 0;
      if (se.value1 == 0 && se.value2 == 0)
        slice->codCount = 0;

      // read MB AFF
      checkBot=readBot = readTop = 0;
      if ((mbNum & 0x01) == 0) {
        checkBot = mb->skipFlag;
        readTop = !checkBot;
        }
      else
        readBot = topMB->skipFlag && (!mb->skipFlag);

      if (readBot || readTop) {
        se.reading = readFieldModeCabac;
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->mbField = (bool)se.value1;
        }
      if (checkBot)
        checkNextMbGetFieldModeCabacSliceB (slice, &se, dataPartition);

      //update the list offset;
      mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;

      checkNeighbourCabac (mb);

      // read MB type
      if (mb->mbType != 0 ) {
        se.reading = readMbTypeInfoCabacSliceB;
        dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->mbType = (int16_t)se.value1;
        if(!dataPartition->bitStream.errorFlag)
          mb->errorFlag = 0;
        }

      motion->mbField[mbNum] = (uint8_t) mb->mbField;
      mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
      slice->siBlock[mb->mb.y][mb->mb.x] = 0;
      slice->interpretMbMode (mb);
      if (mb->mbField) {
        slice->numRefIndexActive[LIST_0] <<=1;
        slice->numRefIndexActive[LIST_1] <<=1;
        }
      }

    if (mb->mbType == IPCM)
      readIPCMmacroBlock (mb, dpMap);
    else if (mb->mbType == I4MB)
      readIntra4x4macroBlockCabac (mb, dpMap);
    else if (mb->mbType == P8x8) {
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
      se.type = SE_MBTYPE;
      if (dataPartition->bitStream.errorFlag)
        se.mapping = cBitStream::linfo_ue;
      else
        se.reading = readB8TypeInfoCabacSliceB;
      readI8x8macroBlock(mb, dataPartition, &se);
      }
    else if (mb->mbType == BSKIP_DIRECT) {
      //init noMbPartLessThan8x8Flag
      mb->noMbPartLessThan8x8Flag = (!(slice->activeSps->isDirect8x8inference)) ? false: true;

      // transform size flag for INTRA_4x4 and INTRA_8x8 modes
      mb->lumaTransformSize8x8flag = false;
      if(decoder->activePps->hasConstrainedIntraPred)
        slice->intraBlock[mbNum] = 0;

      initMacroBlockDirect (mb);
      if (slice->codCount >= 0) {
        slice->isResetCoef = true;
        mb->codedBlockPattern = 0;
        slice->codCount = -1;
        }
      else // read CBP and Coeffs
        slice->readCBPcoeffs (mb);
      }
    else if (mb->isIntraBlock == true)
      readIntraMacroBlock (mb);
    else
      readInterMacroBlock (mb);
    }
  //}}}
  //}}}
  //{{{  interpretMb functions
  //{{{
  void interpretMbModeP (sMacroBlock* mb) {

    static const int16_t ICBPTAB[6] = {0,16,32,15,31,47};

    int16_t mbmode = mb->mbType;
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
      mb->isIntraBlock = true;
      mb->mbType = I4MB;
      memset (mb->b8mode, IBLOCK, 4 * sizeof(char));
      memset (mb->b8pdir,     -1, 4 * sizeof(char));
      }
    else if (mbmode == 31) {
      mb->isIntraBlock = true;
      mb->mbType = IPCM;
      mb->codedBlockPattern = -1;
      mb->i16mode = 0;

      memset (mb->b8mode, 0, 4 * sizeof(char));
      memset (mb->b8pdir,-1, 4 * sizeof(char));
      }
    else {
      mb->isIntraBlock = true;
      mb->mbType = I16MB;
      mb->codedBlockPattern = ICBPTAB[((mbmode-7))>>2];
      mb->i16mode = ((mbmode-7)) & 0x03;
      memset (mb->b8mode, 0, 4 * sizeof(char));
      memset (mb->b8pdir,-1, 4 * sizeof(char));
      }
    }
  //}}}
  //{{{
  void interpretMbModeI (sMacroBlock* mb) {

    static const int16_t ICBPTAB[6] = {0,16,32,15,31,47};

    int16_t mbmode = mb->mbType;
    if (mbmode == 0) {
      mb->isIntraBlock = true;
      mb->mbType = I4MB;
      memset (mb->b8mode,IBLOCK,4 * sizeof(char));
      memset (mb->b8pdir,-1,4 * sizeof(char));
      }
    else if (mbmode == 25) {
      mb->isIntraBlock = true;
      mb->mbType=IPCM;
      mb->codedBlockPattern= -1;
      mb->i16mode = 0;
      memset (mb->b8mode, 0,4 * sizeof(char));
      memset (mb->b8pdir,-1,4 * sizeof(char));
      }
    else {
      mb->isIntraBlock = true;
      mb->mbType = I16MB;
      mb->codedBlockPattern= ICBPTAB[(mbmode-1)>>2];
      mb->i16mode = (mbmode-1) & 0x03;
      memset (mb->b8mode, 0, 4 * sizeof(char));
      memset (mb->b8pdir,-1, 4 * sizeof(char));
      }
    }
  //}}}
  //{{{
  void interpretMbModeB (sMacroBlock* mb) {

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

    int16_t i, mbmode;

    //--- set mbtype, b8type, and b8pdir ---
    int16_t mbtype  = mb->mbType;
    if (mbtype == 0) { // direct
      mbmode = 0;
      memset (mb->b8mode, 0, 4 * sizeof(char));
      memset (mb->b8pdir, 2, 4 * sizeof(char));
      }
    else if (mbtype == 23) { // intra4x4
      mb->isIntraBlock = true;
      mbmode = I4MB;
      memset (mb->b8mode, IBLOCK,4 * sizeof(char));
      memset (mb->b8pdir, -1,4 * sizeof(char));
      }
    else if ((mbtype > 23) && (mbtype < 48) ) { // intra16x16
      mb->isIntraBlock = true;
      mbmode = I16MB;
      memset (mb->b8mode,  0, 4 * sizeof(char));
      memset (mb->b8pdir, -1, 4 * sizeof(char));
      mb->codedBlockPattern = (int) ICBPTAB[(mbtype-24)>>2];
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
      mb->isIntraBlock = true;
      mbmode = IPCM;
      memset (mb->b8mode, 0,4 * sizeof(char));
      memset (mb->b8pdir,-1,4 * sizeof(char));
      mb->codedBlockPattern= -1;
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
  void interpretMbModeSI (sMacroBlock* mb) {

    //cDecoder264* decoder = mb->decoder;
    const int ICBPTAB[6] = {0,16,32,15,31,47};

    int16_t mbmode = mb->mbType;
    if (mbmode == 0) {
      mb->isIntraBlock = true;
      mb->mbType = SI4MB;
      memset (mb->b8mode,IBLOCK,4 * sizeof(char));
      memset (mb->b8pdir,-1,4 * sizeof(char));
      mb->slice->siBlock[mb->mb.y][mb->mb.x]=1;
      }
    else if (mbmode == 1) {
      mb->isIntraBlock = true;
      mb->mbType = I4MB;
      memset (mb->b8mode,IBLOCK,4 * sizeof(char));
      memset (mb->b8pdir,-1,4 * sizeof(char));
      }
    else if (mbmode == 26) {
      mb->isIntraBlock = true;
      mb->mbType = IPCM;
      mb->codedBlockPattern= -1;
      mb->i16mode = 0;
      memset (mb->b8mode,0,4 * sizeof(char));
      memset (mb->b8pdir,-1,4 * sizeof(char));
      }
    else {
      mb->isIntraBlock = true;
      mb->mbType = I16MB;
      mb->codedBlockPattern= ICBPTAB[(mbmode-2)>>2];
      mb->i16mode = (mbmode-2) & 0x03;
      memset (mb->b8mode,0,4 * sizeof(char));
      memset (mb->b8pdir,-1,4 * sizeof(char));
      }
    }
  //}}}
  //}}}
  //{{{  readMotion functions
  //{{{
  void readMotionInfoP (sMacroBlock* mb){

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    sSyntaxElement se;
    sDataPartition* dataPartition = NULL;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int16_t partmode = ((mb->mbType == P8x8) ? 4 : mb->mbType);
    int step_h0 = BLOCK_STEP [partmode][0];
    int step_v0 = BLOCK_STEP [partmode][1];

    int j4;
    sPicture* picture = slice->picture;
    sPicMotion *mvInfo = NULL;

    int listOffset = mb->listOffset;
    sPicture** list0 = slice->listX[LIST_0 + listOffset];
    sPicMotion** p_mv_info = &picture->mvInfo[mb->blockY];

    //=====  READ REFERENCE PICTURE INDICES =====
    se.type = SE_REFFRAME;
    dataPartition = &(slice->dataPartitions[dpMap[SE_REFFRAME]]);

    //  For LIST_0, if multiple ref. pictures, read LIST_0 reference picture indices for the MB
    prepareListforRefIndex (mb, &se, dataPartition, slice->numRefIndexActive[LIST_0], (mb->mbType != P8x8) || (!slice->allrefzero));
    readMBRefPictureIdx  (&se, dataPartition, mb, p_mv_info, LIST_0, step_v0, step_h0);

    //=====  READ MOTION VECTORS =====
    se.type = SE_MVD;
    dataPartition = &(slice->dataPartitions[dpMap[SE_MVD]]);
    if (decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag)
      se.mapping = cBitStream::linfo_se;
    else
      se.reading = slice->mbAffFrame ? readMvdCabacMbAff : readMvdCabac;

    // LIST_0 Motion vectors
    readMBMotionVectors (&se, dataPartition, mb, LIST_0, step_h0, step_v0);

    // record reference picture Ids for deblocking decisions
    for (j4 = 0; j4 < 4;++j4)  {
      mvInfo = &p_mv_info[j4][mb->blockX];
      mvInfo->refPic[LIST_0] = list0[(int16_t) mvInfo->refIndex[LIST_0]];
      mvInfo++;
      mvInfo->refPic[LIST_0] = list0[(int16_t) mvInfo->refIndex[LIST_0]];
      mvInfo++;
      mvInfo->refPic[LIST_0] = list0[(int16_t) mvInfo->refIndex[LIST_0]];
      mvInfo++;
      mvInfo->refPic[LIST_0] = list0[(int16_t) mvInfo->refIndex[LIST_0]];
      }
    }
  //}}}
  //{{{
  void readMotionInfoB (sMacroBlock* mb) {

    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;
    sPicture* picture = slice->picture;
    sSyntaxElement se;
    sDataPartition* dataPartition = NULL;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int partmode = (mb->mbType == P8x8) ? 4 : mb->mbType;
    int step_h0 = BLOCK_STEP [partmode][0];
    int step_v0 = BLOCK_STEP [partmode][1];
    int j4, i4;

    int listOffset = mb->listOffset;
    sPicture** list0 = slice->listX[LIST_0 + listOffset];
    sPicture** list1 = slice->listX[LIST_1 + listOffset];
    sPicMotion** p_mv_info = &picture->mvInfo[mb->blockY];

    if (mb->mbType == P8x8)
      slice->updateDirectMv (mb);

    //=====  READ REFERENCE PICTURE INDICES =====
    se.type = SE_REFFRAME;
    dataPartition = &(slice->dataPartitions[dpMap[SE_REFFRAME]]);

    //  For LIST_0, if multiple ref. pictures, read LIST_0 reference picture indices for the MB
    prepareListforRefIndex (mb, &se, dataPartition, slice->numRefIndexActive[LIST_0], true);
    readMBRefPictureIdx (&se, dataPartition, mb, p_mv_info, LIST_0, step_v0, step_h0);

    //  For LIST_1, if multiple ref. pictures, read LIST_1 reference picture indices for the MB
    prepareListforRefIndex (mb, &se, dataPartition, slice->numRefIndexActive[LIST_1], true);
    readMBRefPictureIdx (&se, dataPartition, mb, p_mv_info, LIST_1, step_v0, step_h0);

    //=====  READ MOTION VECTORS =====
    se.type = SE_MVD;
    dataPartition = &(slice->dataPartitions[dpMap[SE_MVD]]);
    if (decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag)
      se.mapping = cBitStream::linfo_se;
    else
      se.reading = slice->mbAffFrame ? readMvdCabacMbAff : readMvdCabac;

    // LIST_0 Motion vectors
    readMBMotionVectors (&se, dataPartition, mb, LIST_0, step_h0, step_v0);

    // LIST_1 Motion vectors
    readMBMotionVectors (&se, dataPartition, mb, LIST_1, step_h0, step_v0);

    // record reference picture Ids for deblocking decisions
    for (j4 = 0; j4 < 4; ++j4) {
      for (i4 = mb->blockX; i4 < mb->blockX + 4; ++i4) {
        sPicMotion *mvInfo = &p_mv_info[j4][i4];
        int16_t refIndex = mvInfo->refIndex[LIST_0];
        mvInfo->refPic[LIST_0] = (refIndex >= 0) ? list0[refIndex] : NULL;
        refIndex = mvInfo->refIndex[LIST_1];
        mvInfo->refPic[LIST_1] = (refIndex >= 0) ? list1[refIndex] : NULL;
        }
      }
    }
  //}}}
  //}}}
  //{{{
  void initCurImgY (cDecoder264* decoder, cSlice* slice, int plane) {
  // probably a better way (or place) to do this, but I'm not sure what (where) it is [CJV]
  // this is intended to make get_block_luma faster, but I'm still performing
  // this at the MB level, and it really should be done at the slice level

    if (decoder->coding.isSeperateColourPlane == 0) {
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
  void genPicListFromFrameList (ePicStructure currStructure, cFrameStore** frameStoreList,
                                int list_idx, sPicture** list, char *list_size, int long_term) {

    int top_idx = 0;
    int bot_idx = 0;

    int (*is_ref)(sPicture*s) = (long_term) ? isLongRef : isShortRef;

    if (currStructure == eTopField) {
      while ((top_idx<list_idx)||(bot_idx<list_idx)) {
        for ( ; top_idx<list_idx; top_idx++) {
          if (frameStoreList[top_idx]->isUsed & 1) {
            if (is_ref (frameStoreList[top_idx]->topField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = frameStoreList[top_idx]->topField;
              (*list_size)++;
              top_idx++;
              break;
              }
            }
          }

        for ( ; bot_idx<list_idx; bot_idx++) {
          if (frameStoreList[bot_idx]->isUsed & 2) {
            if (is_ref (frameStoreList[bot_idx]->botField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = frameStoreList[bot_idx]->botField;
              (*list_size)++;
              bot_idx++;
              break;
              }
            }
          }
        }
      }

    if (currStructure == eBotField) {
      while ((top_idx<list_idx)||(bot_idx<list_idx)) {
        for ( ; bot_idx<list_idx; bot_idx++) {
          if (frameStoreList[bot_idx]->isUsed & 2) {
            if (is_ref (frameStoreList[bot_idx]->botField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = frameStoreList[bot_idx]->botField;
              (*list_size)++;
              bot_idx++;
              break;
              }
            }
          }

        for ( ; top_idx<list_idx; top_idx++) {
          if (frameStoreList[top_idx]->isUsed & 1) {
            if (is_ref (frameStoreList[top_idx]->topField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = frameStoreList[top_idx]->topField;
              (*list_size)++;
              top_idx++;
              break;
              }
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void initListsSliceI (cSlice* slice) {

    slice->listXsize[0] = 0;
    slice->listXsize[1] = 0;
    }
  //}}}
  //{{{
  void initListsSliceP (cSlice* slice) {

    cDecoder264* decoder = slice->decoder;
    cDpb* dpb = slice->dpb;

    int list0idx = 0;
    int listLtIndex = 0;
    cFrameStore** frameStoreList0;
    cFrameStore** frameStoreListLongTerm;

    if (slice->picStructure == eFrame) {
      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
        if (dpb->frameStoreRef[i]->isUsed == 3)
          if ((dpb->frameStoreRef[i]->frame->usedForReference) &&
              !dpb->frameStoreRef[i]->frame->usedLongTerm)
            slice->listX[0][list0idx++] = dpb->frameStoreRef[i]->frame;

      // order list 0 by picNum
      qsort ((void *)slice->listX[0], list0idx, sizeof(sPicture*), comparePicByPicNumDescending);
      slice->listXsize[0] = (char) list0idx;

      // long term
      for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
        if (dpb->frameStoreLongTermRef[i]->isUsed == 3)
          if (dpb->frameStoreLongTermRef[i]->frame->usedLongTerm)
            slice->listX[0][list0idx++] = dpb->frameStoreLongTermRef[i]->frame;
      qsort ((void*)&slice->listX[0][(int16_t)slice->listXsize[0]], list0idx - slice->listXsize[0],
             sizeof(sPicture*), comparePicByLtPicNumAscending);
      slice->listXsize[0] = (char) list0idx;
      }
    else {
      frameStoreList0 = (cFrameStore**)calloc (dpb->size, sizeof(cFrameStore*));
      frameStoreListLongTerm = (cFrameStore**)calloc (dpb->size, sizeof(cFrameStore*));
      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
        if (dpb->frameStoreRef[i]->usedReference)
          frameStoreList0[list0idx++] = dpb->frameStoreRef[i];
      qsort ((void*)frameStoreList0, list0idx, sizeof(cFrameStore*), compareFsByFrameNumDescending);
      slice->listXsize[0] = 0;
      genPicListFromFrameList (slice->picStructure, frameStoreList0, list0idx, slice->listX[0], &slice->listXsize[0], 0);

      // long term
      for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
        frameStoreListLongTerm[listLtIndex++] = dpb->frameStoreLongTermRef[i];
      qsort ((void*)frameStoreListLongTerm, listLtIndex, sizeof(cFrameStore*), compareFsbyLtPicIndexAscending);
      genPicListFromFrameList (slice->picStructure, frameStoreListLongTerm, listLtIndex, slice->listX[0], &slice->listXsize[0], 1);

      free (frameStoreList0);
      free (frameStoreListLongTerm);
      }
    slice->listXsize[1] = 0;

    // set max size
    slice->listXsize[0] = (char)imin (slice->listXsize[0], slice->numRefIndexActive[LIST_0]);
    slice->listXsize[1] = (char)imin (slice->listXsize[1], slice->numRefIndexActive[LIST_1]);

    // set the unused list entries to NULL
    for (uint32_t i = slice->listXsize[0]; i < (MAX_LIST_SIZE) ; i++)
      slice->listX[0][i] = decoder->noReferencePicture;
    for (uint32_t i = slice->listXsize[1]; i < (MAX_LIST_SIZE) ; i++)
      slice->listX[1][i] = decoder->noReferencePicture;
    }
  //}}}
  //{{{
  void initListsSliceB (cSlice* slice) {

    cDecoder264* decoder = slice->decoder;
    cDpb* dpb = slice->dpb;

    int list0idx = 0;
    int list0index1 = 0;
    int listLtIndex = 0;
    cFrameStore** frameStoreList0;
    cFrameStore** frameStoreList1;
    cFrameStore** frameStoreListLongTerm;

    // B Slice
    if (slice->picStructure == eFrame) {
      //{{{  frame
      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
        if (dpb->frameStoreRef[i]->isUsed==3)
          if ((dpb->frameStoreRef[i]->frame->usedForReference) && (!dpb->frameStoreRef[i]->frame->usedLongTerm))
            if (slice->framePoc >= dpb->frameStoreRef[i]->frame->poc) // !KS use >= for error conceal
              slice->listX[0][list0idx++] = dpb->frameStoreRef[i]->frame;
      qsort ((void*)slice->listX[0], list0idx, sizeof(sPicture*), comparePicByPocdesc);

      // get the backward reference picture (POC>current POC) in list0;
      list0index1 = list0idx;
      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
        if (dpb->frameStoreRef[i]->isUsed==3)
          if ((dpb->frameStoreRef[i]->frame->usedForReference)&&(!dpb->frameStoreRef[i]->frame->usedLongTerm))
            if (slice->framePoc < dpb->frameStoreRef[i]->frame->poc)
              slice->listX[0][list0idx++] = dpb->frameStoreRef[i]->frame;
      qsort ((void*)&slice->listX[0][list0index1], list0idx-list0index1, sizeof(sPicture*), comparePicByPocAscending);

      for (int j = 0; j < list0index1; j++)
        slice->listX[1][list0idx-list0index1+j]=slice->listX[0][j];
      for (int j = list0index1; j < list0idx; j++)
        slice->listX[1][j-list0index1] = slice->listX[0][j];
      slice->listXsize[0] = slice->listXsize[1] = (char) list0idx;

      // long term
      for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
        if (dpb->frameStoreLongTermRef[i]->isUsed == 3) {
          if (dpb->frameStoreLongTermRef[i]->frame->usedLongTerm) {
            slice->listX[0][list0idx] = dpb->frameStoreLongTermRef[i]->frame;
            slice->listX[1][list0idx++] = dpb->frameStoreLongTermRef[i]->frame;
            }
          }
        }
      qsort ((void *)&slice->listX[0][(int16_t) slice->listXsize[0]], list0idx - slice->listXsize[0],
             sizeof(sPicture*), comparePicByLtPicNumAscending);
      qsort ((void *)&slice->listX[1][(int16_t) slice->listXsize[0]], list0idx - slice->listXsize[0],
             sizeof(sPicture*), comparePicByLtPicNumAscending);

      slice->listXsize[0] = slice->listXsize[1] = (char)list0idx;
      }
      //}}}
    else {
      //{{{  field
      frameStoreList0 = (cFrameStore**)calloc(dpb->size, sizeof (cFrameStore*));
      frameStoreList1 = (cFrameStore**)calloc(dpb->size, sizeof (cFrameStore*));
      frameStoreListLongTerm = (cFrameStore**)calloc(dpb->size, sizeof (cFrameStore*));
      slice->listXsize[0] = 0;
      slice->listXsize[1] = 1;

      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
        if (dpb->frameStoreRef[i]->isUsed)
          if (slice->thisPoc >= dpb->frameStoreRef[i]->poc)
            frameStoreList0[list0idx++] = dpb->frameStoreRef[i];
      qsort ((void*)frameStoreList0, list0idx, sizeof(cFrameStore*), comparefsByPocdesc);

      list0index1 = list0idx;
      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
        if (dpb->frameStoreRef[i]->isUsed)
          if (slice->thisPoc < dpb->frameStoreRef[i]->poc)
            frameStoreList0[list0idx++] = dpb->frameStoreRef[i];
      qsort ((void*)&frameStoreList0[list0index1], list0idx-list0index1, sizeof(cFrameStore*), compareFsByPocAscending);

      for (int j = 0; j < list0index1; j++)
        frameStoreList1[list0idx-list0index1+j]=frameStoreList0[j];
      for (int j = list0index1; j < list0idx; j++)
        frameStoreList1[j-list0index1]=frameStoreList0[j];

      slice->listXsize[0] = 0;
      slice->listXsize[1] = 0;
      genPicListFromFrameList (slice->picStructure, frameStoreList0, list0idx, slice->listX[0], &slice->listXsize[0], 0);
      genPicListFromFrameList (slice->picStructure, frameStoreList1, list0idx, slice->listX[1], &slice->listXsize[1], 0);

      // long term
      for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
        frameStoreListLongTerm[listLtIndex++] = dpb->frameStoreLongTermRef[i];

      qsort ((void*)frameStoreListLongTerm, listLtIndex, sizeof(cFrameStore*), compareFsbyLtPicIndexAscending);
      genPicListFromFrameList (slice->picStructure, frameStoreListLongTerm, listLtIndex, slice->listX[0], &slice->listXsize[0], 1);
      genPicListFromFrameList (slice->picStructure, frameStoreListLongTerm, listLtIndex, slice->listX[1], &slice->listXsize[1], 1);

      free (frameStoreList0);
      free (frameStoreList1);
      free (frameStoreListLongTerm);
      }
      //}}}

    if ((slice->listXsize[0] == slice->listXsize[1]) && (slice->listXsize[0] > 1)) {
      // check if lists are identical, if yes swap first two elements of slice->listX[1]
      bool diff = false;
      for (int j = 0; j< slice->listXsize[0]; j++) {
        if (slice->listX[0][j] != slice->listX[1][j]) {
          diff = true;
          break;
          }
        }
      if (!diff) {
        sPicture* tmp_s = slice->listX[1][0];
        slice->listX[1][0] = slice->listX[1][1];
        slice->listX[1][1] = tmp_s;
        }
      }

    // set max size
    slice->listXsize[0] = (char)imin (slice->listXsize[0], slice->numRefIndexActive[LIST_0]);
    slice->listXsize[1] = (char)imin (slice->listXsize[1], slice->numRefIndexActive[LIST_1]);

    // set the unused list entries to NULL
    for (uint32_t i = slice->listXsize[0]; i < (MAX_LIST_SIZE) ; i++)
      slice->listX[0][i] = decoder->noReferencePicture;
    for (uint32_t i = slice->listXsize[1]; i < (MAX_LIST_SIZE) ; i++)
      slice->listX[1][i] = decoder->noReferencePicture;
    }
  //}}}
  }

//{{{
void copyImage4x4 (sPixel** imgBuf1, sPixel** imgBuf2, int off1, int off2) {

  memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE * sizeof (sPixel));
  memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE * sizeof (sPixel));
  memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE * sizeof (sPixel));
  memcpy ((*imgBuf1   + off1), (*imgBuf2   + off2), BLOCK_SIZE * sizeof (sPixel));
  }
//}}}
//{{{
void copyImage8x8 (sPixel** imgBuf1, sPixel** imgBuf2, int off1, int off2) {

  for (int j = 0; j < BLOCK_SIZE_8x8; j+=4) {
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE_8x8 * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE_8x8 * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE_8x8 * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE_8x8 * sizeof (sPixel));
    }
  }
//}}}
//{{{
void copyImage16x16 (sPixel** imgBuf1, sPixel** imgBuf2, int off1, int off2) {

  for (int j = 0; j < MB_BLOCK_SIZE; j += 4) {
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), MB_BLOCK_SIZE * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), MB_BLOCK_SIZE * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), MB_BLOCK_SIZE * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), MB_BLOCK_SIZE * sizeof (sPixel));
    }
  }
//}}}

//{{{
void getMbBlockPosNormal (sBlockPos* picPos, int mbIndex, int16_t* x, int16_t* y) {

  sBlockPos* pPos = &picPos[mbIndex];
  *x = (int16_t) pPos->x;
  *y = (int16_t) pPos->y;
  }
//}}}
//{{{
void getMbBlockPosMbaff (sBlockPos* picPos, int mbIndex, int16_t* x, int16_t* y) {

  sBlockPos* pPos = &picPos[mbIndex >> 1];
  *x = (int16_t)pPos->x;
  *y = (int16_t)((pPos->y << 1) + (mbIndex & 0x01));
  }
//}}}
//{{{
void getMbPos (cDecoder264* decoder, int mbIndex, int mbSize[2], int16_t* x, int16_t* y) {

  decoder->getMbBlockPos (decoder->picPos, mbIndex, x, y);
  (*x) = (int16_t)((*x) * mbSize[0]);
  (*y) = (int16_t)((*y) * mbSize[1]);
  }
//}}}

//{{{
bool checkVertMV (sMacroBlock* mb, int vec1_y, int blockSizeY) {

  cDecoder264* decoder = mb->decoder;
  sPicture* picture = mb->slice->picture;

  int y_pos = vec1_y>>2;
  int maxold_y = (mb->mbField) ? (picture->sizeY >> 1) - 1 : picture->size_y_m1;

  return y_pos < (-decoder->coding.lumaPadY + 2) || y_pos > (maxold_y + decoder->coding.lumaPadY - blockSizeY - 2);
  }
//}}}
//{{{
void checkNeighbours (sMacroBlock* mb) {

  cSlice* slice = mb->slice;
  sPicture* picture = slice->picture; //decoder->picture;
  const int mb_nr = mb->mbIndexX;
  sBlockPos* picPos = mb->decoder->picPos;

  if (picture->mbAffFrame) {
    int cur_mb_pair = mb_nr >> 1;
    mb->mbIndexA = 2 * (cur_mb_pair - 1);
    mb->mbIndexB = 2 * (cur_mb_pair - picture->picWidthMbs);
    mb->mbIndexC = 2 * (cur_mb_pair - picture->picWidthMbs + 1);
    mb->mbIndexD = 2 * (cur_mb_pair - picture->picWidthMbs - 1);

    mb->mbAvailA = (bool) (isMbAvailable(mb->mbIndexA, mb) && ((picPos[cur_mb_pair    ].x)!=0));
    mb->mbAvailB = (bool) (isMbAvailable(mb->mbIndexB, mb));
    mb->mbAvailC = (bool) (isMbAvailable(mb->mbIndexC, mb) && ((picPos[cur_mb_pair + 1].x)!=0));
    mb->mbAvailD = (bool) (isMbAvailable(mb->mbIndexD, mb) && ((picPos[cur_mb_pair    ].x)!=0));
    }

  else {
    sBlockPos* p_pic_pos = &picPos[mb_nr    ];
    mb->mbIndexA = mb_nr - 1;
    mb->mbIndexD = mb->mbIndexA - picture->picWidthMbs;
    mb->mbIndexB = mb->mbIndexD + 1;
    mb->mbIndexC = mb->mbIndexB + 1;

    mb->mbAvailA = (bool) (isMbAvailable(mb->mbIndexA, mb) && ((p_pic_pos->x)!=0));
    mb->mbAvailD = (bool) (isMbAvailable(mb->mbIndexD, mb) && ((p_pic_pos->x)!=0));
    mb->mbAvailC = (bool) (isMbAvailable(mb->mbIndexC, mb) && (((p_pic_pos + 1)->x)!=0));
    mb->mbAvailB = (bool) (isMbAvailable(mb->mbIndexB, mb));
    }

  mb->mbCabacLeft = (mb->mbAvailA) ? &(slice->mbData[mb->mbIndexA]) : NULL;
  mb->mbCabacUp   = (mb->mbAvailB) ? &(slice->mbData[mb->mbIndexB]) : NULL;
  }
//}}}
//{{{
void checkNeighboursNormal (sMacroBlock* mb) {

  cSlice* slice = mb->slice;
  sPicture* picture = slice->picture; //decoder->picture;
  const int mb_nr = mb->mbIndexX;
  sBlockPos* picPos = mb->decoder->picPos;

  sBlockPos* p_pic_pos = &picPos[mb_nr    ];
  mb->mbIndexA = mb_nr - 1;
  mb->mbIndexD = mb->mbIndexA - picture->picWidthMbs;
  mb->mbIndexB = mb->mbIndexD + 1;
  mb->mbIndexC = mb->mbIndexB + 1;

  mb->mbAvailA = (bool) (isMbAvailable(mb->mbIndexA, mb) && ((p_pic_pos->x)!=0));
  mb->mbAvailD = (bool) (isMbAvailable(mb->mbIndexD, mb) && ((p_pic_pos->x)!=0));
  mb->mbAvailC = (bool) (isMbAvailable(mb->mbIndexC, mb) && (((p_pic_pos + 1)->x)!=0));
  mb->mbAvailB = (bool) (isMbAvailable(mb->mbIndexB, mb));

  mb->mbCabacLeft = (mb->mbAvailA) ? &(slice->mbData[mb->mbIndexA]) : NULL;
  mb->mbCabacUp   = (mb->mbAvailB) ? &(slice->mbData[mb->mbIndexB]) : NULL;
  }
//}}}
//{{{
void checkNeighboursMbAff (sMacroBlock* mb) {

  cSlice* slice = mb->slice;
  sPicture* picture = slice->picture;

  const int mb_nr = mb->mbIndexX;
  sBlockPos* picPos = mb->decoder->picPos;

  int cur_mb_pair = mb_nr >> 1;
  mb->mbIndexA = 2 * (cur_mb_pair - 1);
  mb->mbIndexB = 2 * (cur_mb_pair - picture->picWidthMbs);
  mb->mbIndexC = 2 * (cur_mb_pair - picture->picWidthMbs + 1);
  mb->mbIndexD = 2 * (cur_mb_pair - picture->picWidthMbs - 1);

  mb->mbAvailA = (bool) (isMbAvailable(mb->mbIndexA, mb) && ((picPos[cur_mb_pair    ].x)!=0));
  mb->mbAvailB = (bool) (isMbAvailable(mb->mbIndexB, mb));
  mb->mbAvailC = (bool) (isMbAvailable(mb->mbIndexC, mb) && ((picPos[cur_mb_pair + 1].x)!=0));
  mb->mbAvailD = (bool) (isMbAvailable(mb->mbIndexD, mb) && ((picPos[cur_mb_pair    ].x)!=0));

  mb->mbCabacLeft = (mb->mbAvailA) ? &(slice->mbData[mb->mbIndexA]) : NULL;
  mb->mbCabacUp   = (mb->mbAvailB) ? &(slice->mbData[mb->mbIndexB]) : NULL;
  }
//}}}
//{{{
void getAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos* pixelPos) {

  cDecoder264* decoder = mb->decoder;
  int maxW, maxH;
  int yM = -1;

  maxW = mbSize[0];
  maxH = mbSize[1];

  // initialize to "not ok"
  pixelPos->ok = false;

  if (yN > (maxH - 1))
    return;
  if (xN > (maxW - 1) && yN >= 0 && yN < maxH)
    return;

  if (xN < 0) {
    if (yN < 0) {
      if(!mb->mbField) {
        //{{{  frame
        if ((mb->mbIndexX & 0x01) == 0) {
          //  top
          pixelPos->mbIndex   = mb->mbIndexD  + 1;
          pixelPos->ok = mb->mbAvailD;
          yM = yN;
          }

        else {
          //  bottom
          pixelPos->mbIndex   = mb->mbIndexA;
          pixelPos->ok = mb->mbAvailA;
          if (mb->mbAvailA) {
            if(!decoder->mbData[mb->mbIndexA].mbField)
               yM = yN;
            else {
              (pixelPos->mbIndex)++;
               yM = (yN + maxH) >> 1;
              }
            }
          }
        }
        //}}}
      else {
        //{{{  field
        if ((mb->mbIndexX & 0x01) == 0) {
          //  top
          pixelPos->mbIndex   = mb->mbIndexD;
          pixelPos->ok = mb->mbAvailD;
          if (mb->mbAvailD) {
            if(!decoder->mbData[mb->mbIndexD].mbField) {
              (pixelPos->mbIndex)++;
               yM = 2 * yN;
              }
            else
               yM = yN;
            }
          }

        else {
          //  bottom
          pixelPos->mbIndex   = mb->mbIndexD+1;
          pixelPos->ok = mb->mbAvailD;
          yM = yN;
          }
        }
        //}}}
      }
    else {
      // xN < 0 && yN >= 0
      if (yN >= 0 && yN <maxH) {
        if (!mb->mbField) {
          //{{{  frame
          if ((mb->mbIndexX & 0x01) == 0) {
            //{{{  top
            pixelPos->mbIndex   = mb->mbIndexA;
            pixelPos->ok = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField)
                 yM = yN;
              else {
                (pixelPos->mbIndex)+= ((yN & 0x01) != 0);
                yM = yN >> 1;
                }
              }
            }
            //}}}
          else {
            //{{{  bottom
            pixelPos->mbIndex   = mb->mbIndexA;
            pixelPos->ok = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField) {
                (pixelPos->mbIndex)++;
                 yM = yN;
                }
              else {
                (pixelPos->mbIndex)+= ((yN & 0x01) != 0);
                yM = (yN + maxH) >> 1;
                }
              }
            }
            //}}}
          }
          //}}}
        else {
          //{{{  field
          if ((mb->mbIndexX & 0x01) == 0) {
            //{{{  top
            pixelPos->mbIndex  = mb->mbIndexA;
            pixelPos->ok = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField) {
                if (yN < (maxH >> 1))
                   yM = yN << 1;
                else {
                  (pixelPos->mbIndex)++;
                   yM = (yN << 1 ) - maxH;
                  }
                }
              else
                 yM = yN;
              }
            }
            //}}}
          else {
            //{{{  bottom
            pixelPos->mbIndex  = mb->mbIndexA;
            pixelPos->ok = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField) {
                if (yN < (maxH >> 1))
                  yM = (yN << 1) + 1;
                else {
                  (pixelPos->mbIndex)++;
                   yM = (yN << 1 ) + 1 - maxH;
                  }
                }
              else {
                (pixelPos->mbIndex)++;
                 yM = yN;
                }
              }
            }
            //}}}
          }
          //}}}
        }
      }
    }
  else {
     // xN >= 0
    if (xN >= 0 && xN < maxW) {
      if (yN<0) {
        if (!mb->mbField) {
          //{{{  frame
          if ((mb->mbIndexX & 0x01) == 0) {
            //{{{  top
            pixelPos->mbIndex  = mb->mbIndexB;
            // for the deblocker if the current MB is a frame and the one above is a field
            // then the neighbor is the top MB of the pair
            if (mb->mbAvailB) {
              if (!(mb->DeblockCall == 1 && (decoder->mbData[mb->mbIndexB]).mbField))
                pixelPos->mbIndex  += 1;
              }

            pixelPos->ok = mb->mbAvailB;
            yM = yN;
            }
            //}}}
          else {
            //{{{  bottom
            pixelPos->mbIndex   = mb->mbIndexX - 1;
            pixelPos->ok = true;
            yM = yN;
            }
            //}}}
          }
          //}}}
        else {
          //{{{  field
          if ((mb->mbIndexX & 0x01) == 0) {
             //{{{  top
             pixelPos->mbIndex   = mb->mbIndexB;
             pixelPos->ok = mb->mbAvailB;
             if (mb->mbAvailB) {
               if(!decoder->mbData[mb->mbIndexB].mbField) {
                 (pixelPos->mbIndex)++;
                  yM = 2* yN;
                 }
               else
                  yM = yN;
               }
             }
             //}}}
           else {
            //{{{  bottom
            pixelPos->mbIndex   = mb->mbIndexB + 1;
            pixelPos->ok = mb->mbAvailB;
            yM = yN;
            }
            //}}}
          }
          //}}}
        }
      else {
        //{{{  yN >=0
        // for the deblocker if this is the extra edge then do this special stuff
        if (yN == 0 && mb->DeblockCall == 2) {
          pixelPos->mbIndex  = mb->mbIndexB + 1;
          pixelPos->ok = true;
          yM = yN - 1;
          }

        else if ((yN >= 0) && (yN <maxH)) {
          pixelPos->mbIndex   = mb->mbIndexX;
          pixelPos->ok = true;
          yM = yN;
          }
        }
        //}}}
      }
    else {
      //{{{  xN >= maxW
      if(yN < 0) {
        if (!mb->mbField) {
          // frame
          if ((mb->mbIndexX & 0x01) == 0) {
            // top
            pixelPos->mbIndex  = mb->mbIndexC + 1;
            pixelPos->ok = mb->mbAvailC;
            yM = yN;
            }
          else
            // bottom
            pixelPos->ok = false;
          }
        else {
          // field
          if ((mb->mbIndexX & 0x01) == 0) {
            // top
            pixelPos->mbIndex   = mb->mbIndexC;
            pixelPos->ok = mb->mbAvailC;
            if (mb->mbAvailC) {
              if(!decoder->mbData[mb->mbIndexC].mbField) {
                (pixelPos->mbIndex)++;
                 yM = 2* yN;
                }
              else
                yM = yN;
              }
            }
          else {
            // bottom
            pixelPos->mbIndex   = mb->mbIndexC + 1;
            pixelPos->ok = mb->mbAvailC;
            yM = yN;
            }
          }
        }
      }
      //}}}
    }

  if (pixelPos->ok || mb->DeblockCall) {
    pixelPos->x = (int16_t) (xN & (maxW - 1));
    pixelPos->y = (int16_t) (yM & (maxH - 1));
    getMbPos (decoder, pixelPos->mbIndex, mbSize, &(pixelPos->posX), &(pixelPos->posY));
    pixelPos->posX = pixelPos->posX + pixelPos->x;
    pixelPos->posY = pixelPos->posY + pixelPos->y;
    }
  }
//}}}
//{{{
void getNonAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos* pixelPos) {

  int maxW = mbSize[0];
  int maxH = mbSize[1];

  if (xN < 0) {
    if (yN < 0) {
      pixelPos->mbIndex = mb->mbIndexD;
      pixelPos->ok = mb->mbAvailD;
      }
    else if (yN < maxH) {
      pixelPos->mbIndex = mb->mbIndexA;
      pixelPos->ok = mb->mbAvailA;
      }
    else
      pixelPos->ok = false;
    }
  else if (xN < maxW) {
    if (yN < 0) {
      pixelPos->mbIndex = mb->mbIndexB;
      pixelPos->ok = mb->mbAvailB;
      }
    else if (yN < maxH) {
      pixelPos->mbIndex = mb->mbIndexX;
      pixelPos->ok = true;
      }
    else
      pixelPos->ok = false;
    }
  else if ((xN >= maxW) && (yN < 0)) {
    pixelPos->mbIndex = mb->mbIndexC;
    pixelPos->ok = mb->mbAvailC;
    }
  else
    pixelPos->ok = false;

  if (pixelPos->ok || mb->DeblockCall) {
    sBlockPos* blockPos = &(mb->decoder->picPos[pixelPos->mbIndex]);
    pixelPos->x = (int16_t)(xN & (maxW - 1));
    pixelPos->y = (int16_t)(yN & (maxH - 1));
    pixelPos->posX = (int16_t)(pixelPos->x + blockPos->x * maxW);
    pixelPos->posY = (int16_t)(pixelPos->y + blockPos->y * maxH);
    }
  }
//}}}
//{{{
void get4x4Neighbour (sMacroBlock* mb, int blockX, int blockY, int mbSize[2], sPixelPos* pixelPos) {

  mb->decoder->getNeighbour (mb, blockX, blockY, mbSize, pixelPos);
  if (pixelPos->ok) {
    pixelPos->x >>= 2;
    pixelPos->y >>= 2;
    pixelPos->posX >>= 2;
    pixelPos->posY >>= 2;
    }
  }
//}}}
//{{{
void get4x4NeighbourBase (sMacroBlock* mb, int blockX, int blockY, int mbSize[2], sPixelPos* pixelPos) {

  mb->decoder->getNeighbour (mb, blockX, blockY, mbSize, pixelPos);
  if (pixelPos->ok) {
    pixelPos->x >>= 2;
    pixelPos->y >>= 2;
    }
  }
//}}}

//{{{
void itrans4x4 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  cSlice* slice = mb->slice;
  int** mbRess = slice->mbRess[plane];
  inverse4x4 (slice->cof[plane],mbRess,joff,ioff);

  sample_reconstruct (&slice->mbRec[plane][joff], &slice->mbPred[plane][joff], &mbRess[joff], ioff, ioff, BLOCK_SIZE, BLOCK_SIZE, mb->decoder->coding.maxPelValueComp[plane], DQ_BITS);
  }
//}}}
//{{{
void itrans4x4Lossless (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  cSlice* slice = mb->slice;
  sPixel** mbPred = slice->mbPred[plane];
  sPixel** mbRec = slice->mbRec[plane];
  int** mbRess = slice->mbRess [plane];

  cDecoder264* decoder = mb->decoder;
  int max_imgpel_value = decoder->coding.maxPelValueComp[plane];
  for (int j = joff; j < joff + BLOCK_SIZE; ++j)
    for (int i = ioff; i < ioff + BLOCK_SIZE; ++i)
      mbRec[j][i] = (sPixel) iClip1(max_imgpel_value, mbPred[j][i] + mbRess[j][i]);
  }
//}}}
//{{{
void invResidualTransChroma (sMacroBlock* mb, int uv) {

  cSlice* slice = mb->slice;
  int** mbRess = slice->mbRess[uv+1];
  int** cof = slice->cof[uv+1];

  int width = mb->decoder->mbCrSizeX;
  int height = mb->decoder->mbCrSizeY;

  int temp[16][16];
  if (mb->chromaPredMode == VERT_PRED_8) {
    for (int i = 0; i < width; i++) {
      temp[0][i] = cof[0][i];
      for (int j = 1; j < height; j++)
        temp[j][i] = temp[j-1][i] + cof[j][i];
      }

    for (int i = 0; i < width; i++)
      for (int j = 0; j < height; j++)
        mbRess[j][i] = temp[j][i];
    }
  else {
    // HOR_PRED_8
    for (int i = 0; i < height; i++) {
      temp[i][0] = cof[i][0];
      for(int j = 1; j < width; j++)
        temp[i][j] = temp[i][j-1] + cof[i][j];
      }

    for (int i = 0; i < height; i++)
      for (int j = 0; j < width; j++)
        mbRess[i][j] = temp[i][j];
    }
  }
//}}}
//{{{
void itrans2 (sMacroBlock* mb, eColorPlane plane) {

  cSlice* slice = mb->slice;
  cDecoder264* decoder = mb->decoder;

  int transform_pl = (decoder->coding.isSeperateColourPlane != 0) ? PLANE_Y : plane;
  int** cof = slice->cof[transform_pl];

  int qpScaled = mb->qpScaled[transform_pl];
  int qp_per = decoder->qpPerMatrix[ qpScaled ];
  int qp_rem = decoder->qpRemMatrix[ qpScaled ];

  int invLevelScale = slice->InvLevelScale4x4_Intra[plane][qp_rem][0][0];

  // horizontal
  int** M4;
  getMem2Dint (&M4, BLOCK_SIZE, BLOCK_SIZE);
  for (int j = 0; j < 4;++j) {
    M4[j][0] = cof[j<<2][0];
    M4[j][1] = cof[j<<2][4];
    M4[j][2] = cof[j<<2][8];
    M4[j][3] = cof[j<<2][12];
    }

  // vertical
  ihadamard4x4 (M4, M4);
  for (int j = 0; j < 4;++j) {
    cof[j<<2][0]  = rshift_rnd((( M4[j][0] * invLevelScale) << qp_per), 6);
    cof[j<<2][4]  = rshift_rnd((( M4[j][1] * invLevelScale) << qp_per), 6);
    cof[j<<2][8]  = rshift_rnd((( M4[j][2] * invLevelScale) << qp_per), 6);
    cof[j<<2][12] = rshift_rnd((( M4[j][3] * invLevelScale) << qp_per), 6);
    }

  freeMem2Dint(M4);
  }
//}}}
//{{{
void itransSp (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  cDecoder264* decoder = mb->decoder;
  cSlice* slice = mb->slice;

  int qp = (slice->sliceType == eSliceSI) ? slice->qs : slice->qp;
  int qp_per = decoder->qpPerMatrix[qp];
  int qp_rem = decoder->qpRemMatrix[qp];
  int qp_per_sp = decoder->qpPerMatrix[slice->qs];
  int qp_rem_sp = decoder->qpRemMatrix[slice->qs];
  int q_bits_sp = Q_BITS + qp_per_sp;

  sPixel** mbPred = slice->mbPred[plane];
  sPixel** mbRec = slice->mbRec[plane];
  int** mbRess = slice->mbRess[plane];
  int** cof = slice->cof[plane];
  int max_imgpel_value = decoder->coding.maxPelValueComp[plane];

  const int (*InvLevelScale4x4)[4] = dequant_coef[qp_rem];
  const int (*InvLevelScale4x4SP)[4] = dequant_coef[qp_rem_sp];

  int** PBlock;
  getMem2Dint (&PBlock, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  for (int j = 0; j < BLOCK_SIZE; ++j) {
    PBlock[j][0] = mbPred[j+joff][ioff    ];
    PBlock[j][1] = mbPred[j+joff][ioff + 1];
    PBlock[j][2] = mbPred[j+joff][ioff + 2];
    PBlock[j][3] = mbPred[j+joff][ioff + 3];
    }

  forward4x4 (PBlock, PBlock, 0, 0);

  if (slice->spSwitch || slice->sliceType == eSliceSI) {
    for (int j = 0; j < BLOCK_SIZE;++j)
      for (int i = 0; i < BLOCK_SIZE;++i) {
        // recovering coefficient since they are already dequantized earlier
        int icof = (cof[joff + j][ioff + i] >> qp_per) / InvLevelScale4x4[j][i];
        int ilev  = rshift_rnd_sf (iabs(PBlock[j][i]) * quant_coef[qp_rem_sp][j][i], q_bits_sp);
        ilev  = isignab (ilev, PBlock[j][i]) + icof;
        cof[joff + j][ioff + i] = ilev * InvLevelScale4x4SP[j][i] << qp_per_sp;
        }
    }
  else
    for (int j = 0; j < BLOCK_SIZE; ++j)
      for (int i = 0; i < BLOCK_SIZE; ++i) {
        // recovering coefficient since they are already dequantized earlier
        int icof = (cof[joff + j][ioff + i] >> qp_per) / InvLevelScale4x4[j][i];
        int ilev = PBlock[j][i] + ((icof * InvLevelScale4x4[j][i] * A[j][i] <<  qp_per) >> 6);
        ilev = isign (ilev) * rshift_rnd_sf (iabs(ilev) * quant_coef[qp_rem_sp][j][i], q_bits_sp);
        cof[joff + j][ioff + i] = ilev * InvLevelScale4x4SP[j][i] << qp_per_sp;
        }

  inverse4x4 (cof, mbRess, joff, ioff);

  for (int j = joff; j < joff +BLOCK_SIZE; ++j) {
    mbRec[j][ioff   ] = (sPixel) iClip1(max_imgpel_value,rshift_rnd_sf(mbRess[j][ioff   ], DQ_BITS));
    mbRec[j][ioff+ 1] = (sPixel) iClip1(max_imgpel_value,rshift_rnd_sf(mbRess[j][ioff+ 1], DQ_BITS));
    mbRec[j][ioff+ 2] = (sPixel) iClip1(max_imgpel_value,rshift_rnd_sf(mbRess[j][ioff+ 2], DQ_BITS));
    mbRec[j][ioff+ 3] = (sPixel) iClip1(max_imgpel_value,rshift_rnd_sf(mbRess[j][ioff+ 3], DQ_BITS));
    }

  freeMem2Dint (PBlock);
  }
//}}}
//{{{
void itransSpChroma (sMacroBlock* mb, int uv) {

  cSlice* slice = mb->slice;
  cDecoder264* decoder = mb->decoder;

  int mp1[BLOCK_SIZE];

  sPixel** mbPred = slice->mbPred[uv + 1];
  int** cof = slice->cof[uv + 1];
  int** PBlock = new_mem2Dint(MB_BLOCK_SIZE, MB_BLOCK_SIZE);

  int qp_per = decoder->qpPerMatrix[((slice->qp < 0 ? slice->qp : QP_SCALE_CR[slice->qp]))];
  int qp_rem = decoder->qpRemMatrix[((slice->qp < 0 ? slice->qp : QP_SCALE_CR[slice->qp]))];
  int qp_per_sp = decoder->qpPerMatrix[((slice->qs < 0 ? slice->qs : QP_SCALE_CR[slice->qs]))];
  int qp_rem_sp = decoder->qpRemMatrix[((slice->qs < 0 ? slice->qs : QP_SCALE_CR[slice->qs]))];
  int q_bits_sp = Q_BITS + qp_per_sp;

  if (slice->sliceType == eSliceSI) {
    qp_per = qp_per_sp;
    qp_rem = qp_rem_sp;
    }

  for (int j = 0; j < decoder->mbCrSizeY; ++j)
    for (int i = 0; i < decoder->mbCrSizeX; ++i) {
      PBlock[j][i] = mbPred[j][i];
      mbPred[j][i] = 0;
      }

  for (int n2 = 0; n2 < decoder->mbCrSizeY; n2 += BLOCK_SIZE)
    for (int n1 = 0; n1 < decoder->mbCrSizeX; n1 += BLOCK_SIZE)
      forward4x4 (PBlock, PBlock, n2, n1);

  // 2X2 transform of DC coeffs.
  mp1[0] = (PBlock[0][0] + PBlock[4][0] + PBlock[0][4] + PBlock[4][4]);
  mp1[1] = (PBlock[0][0] - PBlock[4][0] + PBlock[0][4] - PBlock[4][4]);
  mp1[2] = (PBlock[0][0] + PBlock[4][0] - PBlock[0][4] - PBlock[4][4]);
  mp1[3] = (PBlock[0][0] - PBlock[4][0] - PBlock[0][4] + PBlock[4][4]);

  if (slice->spSwitch || slice->sliceType == eSliceSI) {
    for (int n2 = 0; n2 < 2; ++n2 )
      for (int n1 = 0; n1 < 2; ++n1 ) {
        // quantization fo predicted block
        int ilev = rshift_rnd_sf (iabs (mp1[n1+n2*2]) * quant_coef[qp_rem_sp][0][0], q_bits_sp + 1);
        // addition
        ilev = isignab (ilev, mp1[n1+n2*2]) + cof[n2<<2][n1<<2];
        // dequanti  zation
        mp1[n1+n2*2] = ilev * dequant_coef[qp_rem_sp][0][0] << qp_per_sp;
        }

    for (int n2 = 0; n2 < decoder->mbCrSizeY; n2 += BLOCK_SIZE)
      for (int n1 = 0; n1 < decoder->mbCrSizeX; n1 += BLOCK_SIZE)
        for (int j = 0; j < BLOCK_SIZE; ++j)
          for (int i = 0; i < BLOCK_SIZE; ++i) {
            // recovering coefficient since they are already dequantized earlier
            cof[n2 + j][n1 + i] = (cof[n2 + j][n1 + i] >> qp_per) / dequant_coef[qp_rem][j][i];

            // quantization of the predicted block
            int ilev = rshift_rnd_sf (iabs(PBlock[n2 + j][n1 + i]) * quant_coef[qp_rem_sp][j][i], q_bits_sp);
            // addition of the residual
            ilev = isignab (ilev,PBlock[n2 + j][n1 + i]) + cof[n2 + j][n1 + i];
            // Inverse quantization
            cof[n2 + j][n1 + i] = ilev * dequant_coef[qp_rem_sp][j][i] << qp_per_sp;
            }
    }
  else {
    for (int n2 = 0; n2 < 2; ++n2 )
      for (int n1 = 0; n1 < 2; ++n1 ) {
        int ilev = mp1[n1+n2*2] + (((cof[n2<<2][n1<<2] * dequant_coef[qp_rem][0][0] * A[0][0]) << qp_per) >> 5);
        ilev = isign (ilev) * rshift_rnd_sf (iabs(ilev) * quant_coef[qp_rem_sp][0][0], q_bits_sp + 1);
        mp1[n1+n2*2] = ilev * dequant_coef[qp_rem_sp][0][0] << qp_per_sp;
        }

    for (int n2 = 0; n2 < decoder->mbCrSizeY; n2 += BLOCK_SIZE)
      for (int n1 = 0; n1 < decoder->mbCrSizeX; n1 += BLOCK_SIZE)
        for (int j = 0; j < BLOCK_SIZE; ++j)
          for (int i = 0; i < BLOCK_SIZE; ++i) {
            // recovering coefficient since they are already dequantized earlier
            int icof = (cof[n2 + j][n1 + i] >> qp_per) / dequant_coef[qp_rem][j][i];
            // dequantization and addition of the predicted block
            int ilev = PBlock[n2 + j][n1 + i] + ((icof * dequant_coef[qp_rem][j][i] * A[j][i] << qp_per) >> 6);
            // quantization and dequantization
            ilev = isign (ilev) * rshift_rnd_sf (iabs(ilev) * quant_coef[qp_rem_sp][j][i], q_bits_sp);
            cof[n2 + j][n1 + i] = ilev * dequant_coef[qp_rem_sp][j][i] << qp_per_sp;
            }
    }

  cof[0][0] = (mp1[0] + mp1[1] + mp1[2] + mp1[3]) >> 1;
  cof[0][4] = (mp1[0] + mp1[1] - mp1[2] - mp1[3]) >> 1;
  cof[4][0] = (mp1[0] - mp1[1] + mp1[2] - mp1[3]) >> 1;
  cof[4][4] = (mp1[0] - mp1[1] - mp1[2] + mp1[3]) >> 1;

  freeMem2Dint (PBlock);
  }
//}}}
//{{{
void iTransform (sMacroBlock* mb, eColorPlane plane, int smb) {

  cSlice* slice = mb->slice;
  cDecoder264* decoder = mb->decoder;
  sPicture* picture = slice->picture;
  sPixel** curr_img;
  int uv = plane-1;

  if ((mb->codedBlockPattern & 15) != 0 || smb) {
    if (mb->lumaTransformSize8x8flag == 0)
      iMBtrans4x4 (mb, plane, smb);
    else
      iMBtrans8x8 (mb, plane);
    }
  else {
    curr_img = plane ? picture->imgUV[uv] : picture->imgY;
    copyImage16x16(&curr_img[mb->pixY], slice->mbPred[plane], mb->pixX, 0);
    }

  if (smb)
    slice->isResetCoef = false;

  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
    sPixel** curUV;
    int ioff, joff;
    sPixel** mbRec;
    for (uv = PLANE_U; uv <= PLANE_V; ++uv) {
      curUV = &picture->imgUV[uv - 1][mb->piccY];
      mbRec = slice->mbRec[uv];
      if (!smb && (mb->codedBlockPattern >> 4)) {
        if (mb->isLossless == false) {
          const uint8_t *x_pos, *y_pos;
          for (int b8 = 0; b8 < (decoder->coding.numUvBlocks); ++b8) {
            x_pos = subblk_offset_x[1][b8];
            y_pos = subblk_offset_y[1][b8];
            itrans4x4 (mb, (eColorPlane)uv, *x_pos++, *y_pos++);
            itrans4x4 (mb, (eColorPlane)uv, *x_pos++, *y_pos++);
            itrans4x4 (mb, (eColorPlane)uv, *x_pos++, *y_pos++);
            itrans4x4 (mb, (eColorPlane)uv, *x_pos  , *y_pos  );
            }
          sample_reconstruct (mbRec, slice->mbPred[uv], slice->mbRess[uv], 0, 0,
            decoder->mbSize[1][0], decoder->mbSize[1][1], mb->decoder->coding.maxPelValueComp[uv], DQ_BITS);
          }
        else {
          for (int b8 = 0; b8 < (decoder->coding.numUvBlocks); ++b8) {
            const uint8_t* x_pos = subblk_offset_x[1][b8];
            const uint8_t* y_pos = subblk_offset_y[1][b8];
            for (int i = 0 ; i < decoder->mbCrSizeY ; i ++)
              for (int j = 0 ; j < decoder->mbCrSizeX ; j ++)
                slice->mbRess[uv][i][j] = slice->cof[uv][i][j] ;

            itrans4x4Lossless (mb, (eColorPlane)uv, *x_pos++, *y_pos++);
            itrans4x4Lossless (mb, (eColorPlane)uv, *x_pos++, *y_pos++);
            itrans4x4Lossless (mb, (eColorPlane)uv, *x_pos++, *y_pos++);
            itrans4x4Lossless (mb, (eColorPlane)uv, *x_pos  , *y_pos  );
            }
          }
        copyImage (curUV, mbRec, mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        slice->isResetCoefCr = false;
        }
      else if (smb) {
        mb->iTrans4x4 = (mb->isLossless == false) ? itrans4x4 : itrans4x4Lossless;
        itransSpChroma (mb, uv - 1);

        for (joff = 0; joff < decoder->mbCrSizeY; joff += BLOCK_SIZE)
          for(ioff = 0; ioff < decoder->mbCrSizeX ;ioff += BLOCK_SIZE)
            mb->iTrans4x4 (mb, (eColorPlane)uv, ioff, joff);

        copyImage (curUV, mbRec, mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        slice->isResetCoefCr = false;
        }
      else
        copyImage (curUV, slice->mbPred[uv], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      }
    }
  }
//}}}

//{{{
void setChromaQp (sMacroBlock* mb) {

  cDecoder264* decoder = mb->decoder;
  sPicture* picture = mb->slice->picture;
  for (int i = 0; i < 2; ++i) {
    mb->qpc[i] = iClip3 (-decoder->coding.bitDepthChromaQpScale, 51, mb->qp + picture->chromaQpOffset[i] );
    mb->qpc[i] = mb->qpc[i] < 0 ? mb->qpc[i] : QP_SCALE_CR[mb->qpc[i]];
    mb->qpScaled[i+1] = mb->qpc[i] + decoder->coding.bitDepthChromaQpScale;
  }
}
//}}}
//{{{
void updateQp (sMacroBlock* mb, int qp) {

  cDecoder264* decoder = mb->decoder;

  mb->qp = qp;
  mb->qpScaled[0] = qp + decoder->coding.bitDepthLumaQpScale;
  setChromaQp (mb);

  mb->isLossless = (bool)((mb->qpScaled[0] == 0) && (decoder->coding.useLosslessQpPrime == 1));
  setReadCompCoefCavlc (mb);
  setReadCompCabac (mb);
  }
//}}}
//{{{
void readDeltaQuant (sSyntaxElement* se, sDataPartition* dataPartition, sMacroBlock* mb, const uint8_t* dpMap, int type) {

  cDecoder264* decoder = mb->decoder;
  cSlice* slice = mb->slice;

  se->type = type;
  dataPartition = &slice->dataPartitions[dpMap[se->type]];
  if (decoder->activePps->entropyCoding == eCavlc || dataPartition->bitStream.errorFlag)
    se->mapping = cBitStream::linfo_se;
  else
    se->reading = readQuantCabac;

  dataPartition->readSyntaxElement (mb, se, dataPartition);
  mb->deltaQuant = (int16_t)se->value1;
  if ((mb->deltaQuant < -(26 + decoder->coding.bitDepthLumaQpScale/2)) ||
      (mb->deltaQuant > (25 + decoder->coding.bitDepthLumaQpScale/2))) {
    printf ("mb_qp_delta is out of range (%d)\n", mb->deltaQuant);
    mb->deltaQuant = iClip3 (-(26 + decoder->coding.bitDepthLumaQpScale/2), (25 + decoder->coding.bitDepthLumaQpScale/2), mb->deltaQuant);
    }

  slice->qp = ((slice->qp + mb->deltaQuant + 52 + 2*decoder->coding.bitDepthLumaQpScale) % (52+decoder->coding.bitDepthLumaQpScale)) - decoder->coding.bitDepthLumaQpScale;
  updateQp (mb, slice->qp);
  }
//}}}
//{{{
void invScaleCoeff (sMacroBlock* mb, int level, int run, int qp_per, int i, int j, int i0, int j0, int coef_ctr, const uint8_t (*pos_scan4x4)[2], int (*InvLevelScale4x4)[4])
{
  if (level != 0) {
    /* leave if level == 0 */
    coef_ctr += run + 1;

    i0 = pos_scan4x4[coef_ctr][0];
    j0 = pos_scan4x4[coef_ctr][1];

    mb->codedBlockPatterns[0].blk |= i64power2((j << 2) + i) ;
    mb->slice->cof[0][(j<<2) + j0][(i<<2) + i0]= rshift_rnd_sf((level * InvLevelScale4x4[j0][i0]) << qp_per, 4);
    }
  }
//}}}
//{{{
void checkDpNeighbours (sMacroBlock* mb) {

  cDecoder264* decoder = mb->decoder;
  sPixelPos up, left;
  decoder->getNeighbour (mb, -1,  0, decoder->mbSize[1], &left);
  decoder->getNeighbour (mb,  0, -1, decoder->mbSize[1], &up);

  if ((mb->isIntraBlock == false) || (!(decoder->activePps->hasConstrainedIntraPred)) ) {
    if (left.ok)
      mb->dplFlag |= decoder->mbData[left.mbIndex].dplFlag;
    if (up.ok)
      mb->dplFlag |= decoder->mbData[up.mbIndex].dplFlag;
    }
  }
//}}}
//{{{
void getNeighbours (sMacroBlock* mb, sPixelPos* block, int mb_x, int mb_y, int blockshape_x) {

  int* mbSize = mb->decoder->mbSize[eLuma];

  get4x4Neighbour (mb, mb_x - 1, mb_y, mbSize, block);
  get4x4Neighbour (mb, mb_x, mb_y - 1, mbSize, block + 1);
  get4x4Neighbour (mb, mb_x + blockshape_x, mb_y - 1, mbSize, block + 2);

  if (mb_y > 0) {
    if (mb_x < 8) {
      // first column of 8x8 blocks
      if (mb_y == 8) {
        if (blockshape_x == MB_BLOCK_SIZE)
          block[2].ok  = 0;
        }
      else if (mb_x + blockshape_x == 8)
        block[2].ok = 0;
      }
    else if (mb_x + blockshape_x == MB_BLOCK_SIZE)
      block[2].ok = 0;
    }

  if (!block[2].ok) {
    get4x4Neighbour (mb, mb_x - 1, mb_y - 1, mbSize, block + 3);
    block[2] = block[3];
    }
  }
//}}}

//{{{
int decodeMacroBlock (sMacroBlock* mb, sPicture* picture) {

  cSlice* slice = mb->slice;
  cDecoder264* decoder = mb->decoder;

  if (slice->chroma444notSeparate) {
    if (!mb->isIntraBlock) {
      initCurImgY (decoder, slice, PLANE_Y);
      slice->decodeComponenet (mb, PLANE_Y, picture->imgY, picture);
      initCurImgY (decoder, slice, PLANE_U);
      slice->decodeComponenet (mb, PLANE_U, picture->imgUV[0], picture);
      initCurImgY (decoder, slice, PLANE_V);
      slice->decodeComponenet (mb, PLANE_V, picture->imgUV[1], picture);
      }
    else {
      slice->decodeComponenet (mb, PLANE_Y, picture->imgY, picture);
      slice->decodeComponenet (mb, PLANE_U, picture->imgUV[0], picture);
      slice->decodeComponenet (mb, PLANE_V, picture->imgUV[1], picture);
      }
    slice->isResetCoef = false;
    slice->isResetCoefCr = false;
    }
  else
    slice->decodeComponenet(mb, PLANE_Y, picture->imgY, picture);

  return 0;
  }
//}}}

//{{{
void cSlice::setSliceReadFunctions() {

  if (decoder->activePps->entropyCoding == eCabac) {
    switch (sliceType) {
      case eSliceP:
      //{{{
      case eSliceSP:
        readMacroBlock = readPcabacMacroBlock;
        break;
      //}}}
      //{{{
      case eSliceB:
        readMacroBlock = readBcabacMacroBlock;
        break;
      //}}}
      case eSliceI:
      //{{{
      case eSliceSI:
        readMacroBlock = readIcabacMacroBlock;
        break;
      //}}}
      //{{{
      default:
        printf ("Unsupported slice type\n");
        break;
      //}}}
      }
    }

  else {
    switch (sliceType) {
      case eSliceP:
      //{{{
      case eSliceSP:
        readMacroBlock = readCavlcMacroBlockP;
        break;
      //}}}
      //{{{
      case eSliceB:
        readMacroBlock = readCavlcMacroBlockB;
        break;
      //}}}
      case eSliceI:
      //{{{
      case eSliceSI:
        readMacroBlock = readCavlcMacroBlockI;
        break;
      //}}}
      //{{{
      default:
        printf ("Unsupported slice type\n");
        break;
      //}}}
      }
    }

  switch (sliceType) {
    //{{{
    case eSliceP:
      interpretMbMode = interpretMbModeP;
      nalReadMotionInfo = readMotionInfoP;
      decodeComponenet = decodeComponentP;
      updateDirectMv = NULL;
      initLists = initListsSliceP;
      break;
    //}}}
    //{{{
    case eSliceSP:
      interpretMbMode = interpretMbModeP;
      nalReadMotionInfo = readMotionInfoP;
      decodeComponenet = decodeComponentSP;
      updateDirectMv = NULL;
      initLists = initListsSliceP;
      break;
    //}}}
    //{{{
    case eSliceB:
      interpretMbMode = interpretMbModeB;
      nalReadMotionInfo = readMotionInfoB;
      decodeComponenet = decodeComponentB;
      setUpdateDirectFunction();
      initLists  = initListsSliceB;
      break;
    //}}}
    //{{{
    case eSliceI:
      interpretMbMode = interpretMbModeI;
      nalReadMotionInfo = NULL;
      decodeComponenet = decodeComponentI;
      updateDirectMv = NULL;
      initLists = initListsSliceI;
      break;
    //}}}
    //{{{
    case eSliceSI:
      interpretMbMode = interpretMbModeSI;
      nalReadMotionInfo = NULL;
      decodeComponenet = decodeComponentI;
      updateDirectMv = NULL;
      initLists = initListsSliceI;
      break;
    //}}}
    //{{{
    default:
      printf ("Unsupported slice type\n");
      break;
    //}}}
    }

  setIntraPredFunctions();

  if ((decoder->activeSps->chromaFormatIdc == YUV444) &&
      (decoder->coding.isSeperateColourPlane == 0))
    readCoef4x4cavlc = readCoef4x4cavlc444;
  else
    readCoef4x4cavlc = readCoef4x4cavlcNormal;

  switch (decoder->activePps->entropyCoding) {
    //{{{
    case eCabac:
      setReadCbpCoefsCabac();
      break;
    //}}}
    //{{{
    case eCavlc:
      setReadCbpCoefCavlc();
      break;
    //}}}
    //{{{
    default:
      printf ("Unsupported entropy coding mode\n");
      break;
    //}}}
    }
  }
//}}}
//{{{
void cSlice::startMacroBlockDecode (sMacroBlock** mb) {

  *mb = &mbData[mbIndex];
  (*mb)->slice = this;
  (*mb)->decoder = decoder;
  (*mb)->mbIndexX = mbIndex;

  // Update coordinates of the current macroBlock
  if (mbAffFrame) {
    (*mb)->mb.x = (int16_t) (   (mbIndex) % ((2*decoder->coding.width) / MB_BLOCK_SIZE));
    (*mb)->mb.y = (int16_t) (2*((mbIndex) / ((2*decoder->coding.width) / MB_BLOCK_SIZE)));
    (*mb)->mb.y += ((*mb)->mb.x & 0x01);
    (*mb)->mb.x >>= 1;
    }
  else
    (*mb)->mb = decoder->picPos[mbIndex];

  // Define pixel/block positions
  setMbPosInfo (*mb);

  // reset intra mode
  (*mb)->isIntraBlock = false;

  // reset mode info
  (*mb)->mbType = 0;
  (*mb)->deltaQuant = 0;
  (*mb)->codedBlockPattern = 0;
  (*mb)->chromaPredMode = DC_PRED_8;

  // Save the slice number of this macroBlock. When the macroBlock below
  // is coded it will use this to decide if prediction for above is possible
  (*mb)->sliceNum = (int16_t)curSliceIndex;

  checkNeighbours (*mb);

  // Select appropriate MV predictor function
  initMotionVectorPrediction (*mb, mbAffFrame);
  setReadStoreCodedBlockPattern (mb, activeSps->chromaFormatIdc);

  // Reset syntax element entries in MB struct
  if (sliceType != eSliceI) {
    if (sliceType != eSliceB)
      memset ((*mb)->mvd[0][0][0], 0, MB_BLOCK_dpS * 2 * sizeof(int16_t));
    else
      memset ((*mb)->mvd[0][0][0], 0, 2 * MB_BLOCK_dpS * 2 * sizeof(int16_t));
    }

  memset ((*mb)->codedBlockPatterns, 0, 3 * sizeof(sCodedBlockPattern));

  // initialize mbRess
  if (!isResetCoef) {
    memset (mbRess[0][0], 0, MB_PIXELS * sizeof(int));
    memset (mbRess[1][0], 0, decoder->mbCrSize * sizeof(int));
    memset (mbRess[2][0], 0, decoder->mbCrSize * sizeof(int));
    if (isResetCoefCr)
      memset (cof[0][0], 0, MB_PIXELS * sizeof(int));
    else {
      memset (cof[0][0], 0, 3 * MB_PIXELS * sizeof(int));
      isResetCoefCr = true;
      }
    isResetCoef = true;
    }

  // store filtering parameters for this MB
  (*mb)->deblockFilterDisableIdc = deblockFilterDisableIdc;
  (*mb)->deblockFilterC0Offset = deblockFilterC0Offset;
  (*mb)->deblockFilterBetaOffset = deblockFilterBetaOffset;
  (*mb)->listOffset = 0;
  (*mb)->mixedModeEdgeFlag = 0;
  }
//}}}
//{{{
bool cSlice::endMacroBlockDecode (int eos_bit) {

  // The if() statement below resembles the original code, which tested
  // decoder->mbIndex == decoder->picSizeInMbs.  Both is, of course, nonsense
  // In an error prone environment, one can only be sure to have a new
  // picture by checking the tr of the next slice header!
  ++numDecodedMbs;

  if (mbIndex == decoder->picSizeInMbs - 1)
    return true;
  else {
    mbIndex = FmoGetNextMBNr (decoder, mbIndex);
    if (mbIndex == -1) // End of cSlice group, MUST be end of slice
      return true;

    if (nalStartCode (this, eos_bit) == false)
      return false;

    if ((sliceType == eSliceI) || (sliceType == eSliceSI) ||
        (decoder->activePps->entropyCoding == eCabac))
      return true;

    if (codCount <= 0)
      return true;

    return false;
    }
  }
//}}}
