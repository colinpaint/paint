//{{{  includes
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

#include "global.h"
#include "memory.h"

#include "block.h"
#include "image.h"
#include "mbAccess.h"
#include "transform.h"
#include "quant.h"
//}}}

//{{{
static void sample_reconstruct (sPixel** curImg, sPixel** mpr, int** mb_rres, int mb_x,
                                int opix_x, int width, int height, int max_imgpel_value, int dq_bits) {

  for (int j = 0; j < height; j++) {
    sPixel* imgOrg = &curImg[j][opix_x];
    sPixel* imgPred = &mpr[j][mb_x];
    int* m7 = &mb_rres[j][mb_x];
    for (int i = 0; i < width; i++)
      *imgOrg++ = (sPixel) iClip1 (max_imgpel_value, rshift_rnd_sf(*m7++, dq_bits) + *imgPred++);
    }
  }
//}}}

//{{{
void itrans4x4 (sMacroblock* curMb, eColorPlane pl, int ioff, int joff) {

  sSlice* curSlice = curMb->slice;
  int** mb_rres = curSlice->mb_rres[pl];
  inverse4x4 (curSlice->cof[pl],mb_rres,joff,ioff);

  sample_reconstruct (&curSlice->mb_rec[pl][joff], &curSlice->mb_pred[pl][joff], &mb_rres[joff], ioff, ioff, BLOCK_SIZE, BLOCK_SIZE, curMb->vidParam->maxPelValueComp[pl], DQ_BITS);
  }
//}}}
//{{{
void itrans4x4_ls (sMacroblock* curMb, eColorPlane pl, int ioff, int joff) {

  sSlice* curSlice = curMb->slice;
  sPixel** mb_pred = curSlice->mb_pred[pl];
  sPixel** mb_rec = curSlice->mb_rec[pl];
  int** mb_rres = curSlice->mb_rres [pl];

  sVidParam* vidParam = curMb->vidParam;
  int max_imgpel_value = vidParam->maxPelValueComp[pl];
  for (int j = joff; j < joff + BLOCK_SIZE; ++j)
    for (int i = ioff; i < ioff + BLOCK_SIZE; ++i)
      mb_rec[j][i] = (sPixel) iClip1(max_imgpel_value, mb_pred[j][i] + mb_rres[j][i]);
  }
//}}}

//{{{
void Inv_Residual_trans_4x4 (sMacroblock* curMb, eColorPlane pl, int ioff, int joff) {

  sSlice* curSlice = curMb->slice;
  sPixel** mb_pred = curSlice->mb_pred[pl];
  sPixel** mb_rec = curSlice->mb_rec[pl];
  int** mb_rres = curSlice->mb_rres[pl];
  int** cof = curSlice->cof[pl];

  int temp[4][4];
  if (curMb->ipmode_DPCM == VERT_PRED) {
    for (int i = 0; i<4; ++i) {
      temp[0][i] = cof[joff + 0][ioff + i];
      temp[1][i] = cof[joff + 1][ioff + i] + temp[0][i];
      temp[2][i] = cof[joff + 2][ioff + i] + temp[1][i];
      temp[3][i] = cof[joff + 3][ioff + i] + temp[2][i];
      }

    for (int i = 0; i < 4; ++i) {
      mb_rres[joff    ][ioff + i]=temp[0][i];
      mb_rres[joff + 1][ioff + i]=temp[1][i];
      mb_rres[joff + 2][ioff + i]=temp[2][i];
      mb_rres[joff + 3][ioff + i]=temp[3][i];
      }
    }

  else if (curMb->ipmode_DPCM == HOR_PRED) {
    for (int j = 0; j < 4; ++j) {
      temp[j][0] = cof[joff + j][ioff    ];
      temp[j][1] = cof[joff + j][ioff + 1] + temp[j][0];
      temp[j][2] = cof[joff + j][ioff + 2] + temp[j][1];
      temp[j][3] = cof[joff + j][ioff + 3] + temp[j][2];
      }

    for (int j = 0; j < 4; ++j) {
      mb_rres[joff + j][ioff    ]=temp[j][0];
      mb_rres[joff + j][ioff + 1]=temp[j][1];
      mb_rres[joff + j][ioff + 2]=temp[j][2];
      mb_rres[joff + j][ioff + 3]=temp[j][3];
      }
    }

  else
    for (int j = joff; j < joff + BLOCK_SIZE; ++j)
      for (int i = ioff; i < ioff + BLOCK_SIZE; ++i)
        mb_rres[j][i] = cof[j][i];

  for (int j = joff; j < joff + BLOCK_SIZE; ++j)
    for (int i = ioff; i < ioff + BLOCK_SIZE; ++i)
      mb_rec[j][i] = (sPixel) (mb_rres[j][i] + mb_pred[j][i]);
  }
//}}}
//{{{
void Inv_Residual_trans_8x8 (sMacroblock* curMb, eColorPlane pl, int ioff, int joff) {

  sSlice* curSlice = curMb->slice;
  sPixel** mb_pred = curSlice->mb_pred[pl];
  sPixel** mb_rec  = curSlice->mb_rec[pl];
  int** mb_rres = curSlice->mb_rres[pl];

  int temp[8][8];
  if (curMb->ipmode_DPCM == VERT_PRED) {
    for (int i = 0; i < 8; ++i) {
      temp[0][i] = mb_rres[joff + 0][ioff + i];
      temp[1][i] = mb_rres[joff + 1][ioff + i] + temp[0][i];
      temp[2][i] = mb_rres[joff + 2][ioff + i] + temp[1][i];
      temp[3][i] = mb_rres[joff + 3][ioff + i] + temp[2][i];
      temp[4][i] = mb_rres[joff + 4][ioff + i] + temp[3][i];
      temp[5][i] = mb_rres[joff + 5][ioff + i] + temp[4][i];
      temp[6][i] = mb_rres[joff + 6][ioff + i] + temp[5][i];
      temp[7][i] = mb_rres[joff + 7][ioff + i] + temp[6][i];
      }
    for (int i = 0; i < 8; ++i) {
      mb_rres[joff  ][ioff+i]=temp[0][i];
      mb_rres[joff+1][ioff+i]=temp[1][i];
      mb_rres[joff+2][ioff+i]=temp[2][i];
      mb_rres[joff+3][ioff+i]=temp[3][i];
      mb_rres[joff+4][ioff+i]=temp[4][i];
      mb_rres[joff+5][ioff+i]=temp[5][i];
      mb_rres[joff+6][ioff+i]=temp[6][i];
      mb_rres[joff+7][ioff+i]=temp[7][i];
      }
    }
  else if (curMb->ipmode_DPCM == HOR_PRED) {
    //HOR_PRED
    for (int i = 0; i < 8; ++i) {
      temp[i][0] = mb_rres[joff + i][ioff + 0];
      temp[i][1] = mb_rres[joff + i][ioff + 1] + temp[i][0];
      temp[i][2] = mb_rres[joff + i][ioff + 2] + temp[i][1];
      temp[i][3] = mb_rres[joff + i][ioff + 3] + temp[i][2];
      temp[i][4] = mb_rres[joff + i][ioff + 4] + temp[i][3];
      temp[i][5] = mb_rres[joff + i][ioff + 5] + temp[i][4];
      temp[i][6] = mb_rres[joff + i][ioff + 6] + temp[i][5];
      temp[i][7] = mb_rres[joff + i][ioff + 7] + temp[i][6];
      }
    for (int i = 0; i < 8; ++i) {
      mb_rres[joff+i][ioff+0]=temp[i][0];
      mb_rres[joff+i][ioff+1]=temp[i][1];
      mb_rres[joff+i][ioff+2]=temp[i][2];
      mb_rres[joff+i][ioff+3]=temp[i][3];
      mb_rres[joff+i][ioff+4]=temp[i][4];
      mb_rres[joff+i][ioff+5]=temp[i][5];
      mb_rres[joff+i][ioff+6]=temp[i][6];
      mb_rres[joff+i][ioff+7]=temp[i][7];
      }
    }

  for (int j = joff; j < joff + BLOCK_SIZE*2; ++j)
    for (int i = ioff; i < ioff + BLOCK_SIZE*2; ++i)
      mb_rec [j][i] = (sPixel) (mb_rres[j][i] + mb_pred[j][i]);
  }
//}}}
//{{{
void Inv_Residual_trans_16x16 (sMacroblock* curMb, eColorPlane pl) {

  sSlice* curSlice = curMb->slice;
  sPixel** mb_pred = curSlice->mb_pred[pl];
  sPixel** mb_rec = curSlice->mb_rec[pl];
  int** mb_rres = curSlice->mb_rres[pl];
  int** cof = curSlice->cof[pl];

  int temp[16][16];
  if (curMb->ipmode_DPCM == VERT_PRED_16) {
    for (int i = 0; i < MB_BLOCK_SIZE; ++i) {
      temp[0][i] = cof[0][i];
      for (int j = 1; j < MB_BLOCK_SIZE; j++)
        temp[j][i] = cof[j][i] + temp[j-1][i];
      }

    for (int i = 0; i < MB_BLOCK_SIZE; ++i)
      for (int j = 0; j < MB_BLOCK_SIZE; j++)
        mb_rres[j][i]=temp[j][i];
    }

  else if (curMb->ipmode_DPCM == HOR_PRED_16) {
    for (int j = 0; j < MB_BLOCK_SIZE; ++j) {
      temp[j][ 0] = cof[j][ 0  ];
      for (int i = 1; i < MB_BLOCK_SIZE; i++)
        temp[j][i] = cof[j][i] + temp[j][i-1];
      }

    for (int j = 0; j < MB_BLOCK_SIZE; ++j)
      for (int i = 0; i < MB_BLOCK_SIZE; ++i)
        mb_rres[j][i]=temp[j][i];
    }

  else {
    for (int j = 0; j < MB_BLOCK_SIZE; ++j)
      for (int i = 0; i < MB_BLOCK_SIZE; ++i)
        mb_rres[j][i] = cof[j][i];
    }

  for (int j = 0; j < MB_BLOCK_SIZE; ++j)
    for (int i = 0; i < MB_BLOCK_SIZE; ++i)
      mb_rec[j][i] = (sPixel) (mb_rres[j][i] + mb_pred[j][i]);
  }
//}}}
//{{{
void Inv_Residual_trans_Chroma (sMacroblock* curMb, int uv) {

  sSlice* curSlice = curMb->slice;
  int** mb_rres = curSlice->mb_rres[uv+1];
  int** cof = curSlice->cof[uv+1];

  int width = curMb->vidParam->mbCrSizeX;
  int height = curMb->vidParam->mbCrSizeY;

  int temp[16][16];
  if (curMb->cPredMode == VERT_PRED_8) {
    for (int i = 0; i < width; i++) {
      temp[0][i] = cof[0][i];
      for (int j = 1; j < height; j++)
        temp[j][i] = temp[j-1][i] + cof[j][i];
      }

    for (int i = 0; i < width; i++)
      for (int j = 0; j < height; j++)
        mb_rres[j][i] = temp[j][i];
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
        mb_rres[i][j] = temp[i][j];
    }
  }
//}}}

//{{{
void itrans_2 (sMacroblock* curMb, eColorPlane pl) {

  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;

  int transform_pl = (vidParam->sepColourPlaneFlag != 0) ? PLANE_Y : pl;
  int** cof = curSlice->cof[transform_pl];

  int qpScaled = curMb->qpScaled[transform_pl];
  int qp_per = vidParam->qpPerMatrix[ qpScaled ];
  int qp_rem = vidParam->qpRemMatrix[ qpScaled ];

  int invLevelScale = curSlice->InvLevelScale4x4_Intra[pl][qp_rem][0][0];

  // horizontal
  int** M4;
  get_mem2Dint (&M4, BLOCK_SIZE, BLOCK_SIZE);
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

  free_mem2Dint(M4);
  }
//}}}
//{{{
void itrans_sp (sMacroblock* curMb, eColorPlane pl, int ioff, int joff) {

  sVidParam* vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;

  int qp = (curSlice->sliceType == SI_SLICE) ? curSlice->qs : curSlice->qp;
  int qp_per = vidParam->qpPerMatrix[qp];
  int qp_rem = vidParam->qpRemMatrix[qp];
  int qp_per_sp = vidParam->qpPerMatrix[curSlice->qs];
  int qp_rem_sp = vidParam->qpRemMatrix[curSlice->qs];
  int q_bits_sp = Q_BITS + qp_per_sp;

  sPixel** mb_pred = curSlice->mb_pred[pl];
  sPixel** mb_rec = curSlice->mb_rec[pl];
  int** mb_rres = curSlice->mb_rres[pl];
  int** cof = curSlice->cof[pl];
  int max_imgpel_value = vidParam->maxPelValueComp[pl];

  const int (*InvLevelScale4x4)[4] = dequant_coef[qp_rem];
  const int (*InvLevelScale4x4SP)[4] = dequant_coef[qp_rem_sp];

  int** PBlock;
  get_mem2Dint (&PBlock, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  for (int j = 0; j < BLOCK_SIZE; ++j) {
    PBlock[j][0] = mb_pred[j+joff][ioff    ];
    PBlock[j][1] = mb_pred[j+joff][ioff + 1];
    PBlock[j][2] = mb_pred[j+joff][ioff + 2];
    PBlock[j][3] = mb_pred[j+joff][ioff + 3];
    }

  forward4x4 (PBlock, PBlock, 0, 0);

  if (curSlice->sp_switch || curSlice->sliceType == SI_SLICE) {
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

  inverse4x4 (cof, mb_rres, joff, ioff);

  for (int j = joff; j < joff +BLOCK_SIZE; ++j) {
    mb_rec[j][ioff   ] = (sPixel) iClip1(max_imgpel_value,rshift_rnd_sf(mb_rres[j][ioff   ], DQ_BITS));
    mb_rec[j][ioff+ 1] = (sPixel) iClip1(max_imgpel_value,rshift_rnd_sf(mb_rres[j][ioff+ 1], DQ_BITS));
    mb_rec[j][ioff+ 2] = (sPixel) iClip1(max_imgpel_value,rshift_rnd_sf(mb_rres[j][ioff+ 2], DQ_BITS));
    mb_rec[j][ioff+ 3] = (sPixel) iClip1(max_imgpel_value,rshift_rnd_sf(mb_rres[j][ioff+ 3], DQ_BITS));
    }

  free_mem2Dint (PBlock);
  }
//}}}
//{{{
void itrans_sp_cr (sMacroblock* curMb, int uv) {

  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;
  int mp1[BLOCK_SIZE];
  sPixel** mb_pred = curSlice->mb_pred[uv + 1];
  int** cof = curSlice->cof[uv + 1];
  int** PBlock = new_mem2Dint(MB_BLOCK_SIZE, MB_BLOCK_SIZE);

  int qp_per = vidParam->qpPerMatrix[ ((curSlice->qp < 0 ? curSlice->qp : QP_SCALE_CR[curSlice->qp]))];
  int qp_rem = vidParam->qpRemMatrix[ ((curSlice->qp < 0 ? curSlice->qp : QP_SCALE_CR[curSlice->qp]))];
  int qp_per_sp = vidParam->qpPerMatrix[ ((curSlice->qs < 0 ? curSlice->qs : QP_SCALE_CR[curSlice->qs]))];
  int qp_rem_sp = vidParam->qpRemMatrix[ ((curSlice->qs < 0 ? curSlice->qs : QP_SCALE_CR[curSlice->qs]))];
  int q_bits_sp = Q_BITS + qp_per_sp;

  if (curSlice->sliceType == SI_SLICE) {
    qp_per = qp_per_sp;
    qp_rem = qp_rem_sp;
    }

  for (int j = 0; j < vidParam->mbCrSizeY; ++j)
    for (int i = 0; i < vidParam->mbCrSizeX; ++i) {
      PBlock[j][i] = mb_pred[j][i];
      mb_pred[j][i] = 0;
      }

  for (int n2 = 0; n2 < vidParam->mbCrSizeY; n2 += BLOCK_SIZE)
    for (int n1 = 0; n1 < vidParam->mbCrSizeX; n1 += BLOCK_SIZE)
      forward4x4 (PBlock, PBlock, n2, n1);

  // 2X2 transform of DC coeffs.
  mp1[0] = (PBlock[0][0] + PBlock[4][0] + PBlock[0][4] + PBlock[4][4]);
  mp1[1] = (PBlock[0][0] - PBlock[4][0] + PBlock[0][4] - PBlock[4][4]);
  mp1[2] = (PBlock[0][0] + PBlock[4][0] - PBlock[0][4] - PBlock[4][4]);
  mp1[3] = (PBlock[0][0] - PBlock[4][0] - PBlock[0][4] + PBlock[4][4]);

  if (curSlice->sp_switch || curSlice->sliceType == SI_SLICE) {
    for (int n2 = 0; n2 < 2; ++n2 )
      for (int n1 = 0; n1 < 2; ++n1 ) {
        // quantization fo predicted block
        int ilev = rshift_rnd_sf (iabs (mp1[n1+n2*2]) * quant_coef[qp_rem_sp][0][0], q_bits_sp + 1);
        // addition
        ilev = isignab (ilev, mp1[n1+n2*2]) + cof[n2<<2][n1<<2];
        // dequantization
        mp1[n1+n2*2] = ilev * dequant_coef[qp_rem_sp][0][0] << qp_per_sp;
        }

    for (int n2 = 0; n2 < vidParam->mbCrSizeY; n2 += BLOCK_SIZE)
      for (int n1 = 0; n1 < vidParam->mbCrSizeX; n1 += BLOCK_SIZE)
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
    for (int n2=0; n2 < 2; ++n2 )
      for (int n1=0; n1 < 2; ++n1 ) {
        int ilev = mp1[n1+n2*2] + (((cof[n2<<2][n1<<2] * dequant_coef[qp_rem][0][0] * A[0][0]) << qp_per) >> 5);
        ilev = isign (ilev) * rshift_rnd_sf (iabs(ilev) * quant_coef[qp_rem_sp][0][0], q_bits_sp + 1);
        mp1[n1+n2*2] = ilev * dequant_coef[qp_rem_sp][0][0] << qp_per_sp;
        }

    for (int n2 = 0; n2 < vidParam->mbCrSizeY; n2 += BLOCK_SIZE)
      for (int n1 = 0; n1 < vidParam->mbCrSizeX; n1 += BLOCK_SIZE)
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

  free_mem2Dint(PBlock);
  }
//}}}
//{{{
void iMBtrans4x4 (sMacroblock* curMb, eColorPlane pl, int smb) {

  sSlice* curSlice = curMb->slice;
  sPicture* picture = curMb->slice->picture;
  sPixel** curr_img = pl ? picture->imgUV[pl - 1] : picture->imgY;

  if (curMb->isLossless && curMb->mbType == I16MB)
    Inv_Residual_trans_16x16(curMb, pl);
  else if (smb || curMb->isLossless == TRUE) {
    curMb->itrans_4x4 = (smb) ? itrans_sp : ((curMb->isLossless == FALSE) ? itrans4x4 : Inv_Residual_trans_4x4);
    for (int block8x8 = 0; block8x8 < MB_BLOCK_SIZE; block8x8 += 4) {
      for (int k = block8x8; k < block8x8 + 4; ++k ) {
        int jj = ((decode_block_scan[k] >> 2) & 3) << BLOCK_SHIFT;
        int ii = (decode_block_scan[k] & 3) << BLOCK_SHIFT;
        // use integer transform and make 4x4 block mb_rres from prediction block mb_pred
        curMb->itrans_4x4 (curMb, pl, ii, jj);
        }
      }
    }
  else {
    int** cof = curSlice->cof[pl];
    int** mb_rres = curSlice->mb_rres[pl];

    if (curMb->isIntraBlock == FALSE) {
      if (curMb->cbp & 0x01) {
        inverse4x4 (cof, mb_rres, 0, 0);
        inverse4x4 (cof, mb_rres, 0, 4);
        inverse4x4 (cof, mb_rres, 4, 0);
        inverse4x4 (cof, mb_rres, 4, 4);
        }
      if (curMb->cbp & 0x02) {
        inverse4x4 (cof, mb_rres, 0, 8);
        inverse4x4 (cof, mb_rres, 0, 12);
        inverse4x4 (cof, mb_rres, 4, 8);
        inverse4x4 (cof, mb_rres, 4, 12);
        }
      if (curMb->cbp & 0x04) {
        inverse4x4 (cof, mb_rres, 8, 0);
        inverse4x4 (cof, mb_rres, 8, 4);
        inverse4x4 (cof, mb_rres, 12, 0);
        inverse4x4 (cof, mb_rres, 12, 4);
        }
      if (curMb->cbp & 0x08) {
        inverse4x4 (cof, mb_rres, 8, 8);
        inverse4x4 (cof, mb_rres, 8, 12);
        inverse4x4 (cof, mb_rres, 12, 8);
        inverse4x4 (cof, mb_rres, 12, 12);
        }
      }
    else {
      for (int jj = 0; jj < MB_BLOCK_SIZE; jj += BLOCK_SIZE) {
        inverse4x4 (cof, mb_rres, jj, 0);
        inverse4x4 (cof, mb_rres, jj, 4);
        inverse4x4 (cof, mb_rres, jj, 8);
        inverse4x4 (cof, mb_rres, jj, 12);
        }
      }
    sample_reconstruct (curSlice->mb_rec[pl], curSlice->mb_pred[pl], mb_rres, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE, curMb->vidParam->maxPelValueComp[pl], DQ_BITS);
    }

  // construct picture from 4x4 blocks
  copy_Image_16x16 (&curr_img[curMb->pixY], curSlice->mb_rec[pl], curMb->pixX, 0);
  }
//}}}
//{{{
void iMBtrans8x8 (sMacroblock* curMb, eColorPlane pl) {

  //sVidParam* vidParam = curMb->vidParam;
  sPicture* picture = curMb->slice->picture;
  sPixel** curr_img = pl ? picture->imgUV[pl - 1]: picture->imgY;

  // Perform 8x8 idct
  if (curMb->cbp & 0x01)
    itrans8x8 (curMb, pl, 0, 0);
  else
    icopy8x8 (curMb, pl, 0, 0);

  if (curMb->cbp & 0x02)
    itrans8x8 (curMb, pl, 8, 0);
  else
    icopy8x8 (curMb, pl, 8, 0);

  if (curMb->cbp & 0x04)
    itrans8x8 (curMb, pl, 0, 8);
  else
    icopy8x8 (curMb, pl, 0, 8);

  if (curMb->cbp & 0x08)
    itrans8x8 (curMb, pl, 8, 8);
  else
    icopy8x8 (curMb, pl, 8, 8);

  copy_Image_16x16 (&curr_img[curMb->pixY], curMb->slice->mb_rec[pl], curMb->pixX, 0);
  }
//}}}
//{{{
void iTransform (sMacroblock* curMb, eColorPlane pl, int smb) {

  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;
  sPicture* picture = curSlice->picture;
  sPixel** curr_img;
  int uv = pl-1;

  if ((curMb->cbp & 15) != 0 || smb) {
    if (curMb->lumaTransformSize8x8flag == 0)
      iMBtrans4x4 (curMb, pl, smb);
    else
      iMBtrans8x8 (curMb, pl);
    }
  else {
    curr_img = pl ? picture->imgUV[uv] : picture->imgY;
    copy_Image_16x16(&curr_img[curMb->pixY], curSlice->mb_pred[pl], curMb->pixX, 0);
    }

  if (smb)
    curSlice->is_reset_coeff = FALSE;

  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
    sPixel** curUV;
    int ioff, joff;
    sPixel** mb_rec;
    for (uv = PLANE_U; uv <= PLANE_V; ++uv) {
      curUV = &picture->imgUV[uv - 1][curMb->piccY];
      mb_rec = curSlice->mb_rec[uv];
      if (!smb && (curMb->cbp >> 4)) {
        if (curMb->isLossless == FALSE) {
          const unsigned char *x_pos, *y_pos;
          for (int b8 = 0; b8 < (vidParam->numUvBlocks); ++b8) {
            x_pos = subblk_offset_x[1][b8];
            y_pos = subblk_offset_y[1][b8];
            itrans4x4 (curMb, uv, *x_pos++, *y_pos++);
            itrans4x4 (curMb, uv, *x_pos++, *y_pos++);
            itrans4x4 (curMb, uv, *x_pos++, *y_pos++);
            itrans4x4 (curMb, uv, *x_pos  , *y_pos  );
            }
          sample_reconstruct (mb_rec, curSlice->mb_pred[uv], curSlice->mb_rres[uv], 0, 0,
            vidParam->mbSize[1][0], vidParam->mbSize[1][1], curMb->vidParam->maxPelValueComp[uv], DQ_BITS);
          }
        else {
          for (int b8 = 0; b8 < (vidParam->numUvBlocks); ++b8) {
            const unsigned char* x_pos = subblk_offset_x[1][b8];
            const unsigned char* y_pos = subblk_offset_y[1][b8];
            for (int i = 0 ; i < vidParam->mbCrSizeY ; i ++)
              for (int j = 0 ; j < vidParam->mbCrSizeX ; j ++)
                curSlice->mb_rres[uv][i][j] = curSlice->cof[uv][i][j] ;

            itrans4x4_ls (curMb, uv, *x_pos++, *y_pos++);
            itrans4x4_ls (curMb, uv, *x_pos++, *y_pos++);
            itrans4x4_ls (curMb, uv, *x_pos++, *y_pos++);
            itrans4x4_ls (curMb, uv, *x_pos  , *y_pos  );
            }
          }
        copy_Image (curUV, mb_rec, curMb->pixcX, 0, vidParam->mbSize[1][0], vidParam->mbSize[1][1]);
        curSlice->is_reset_coeff_cr = FALSE;
        }
      else if (smb) {
        curMb->itrans_4x4 = (curMb->isLossless == FALSE) ? itrans4x4 : itrans4x4_ls;
        itrans_sp_cr (curMb, uv - 1);

        for (joff = 0; joff < vidParam->mbCrSizeY; joff += BLOCK_SIZE)
          for(ioff = 0; ioff < vidParam->mbCrSizeX ;ioff += BLOCK_SIZE)
            curMb->itrans_4x4 (curMb, uv, ioff, joff);

        copy_Image (curUV, mb_rec, curMb->pixcX, 0, vidParam->mbSize[1][0], vidParam->mbSize[1][1]);
        curSlice->is_reset_coeff_cr = FALSE;
        }
      else
        copy_Image (curUV, curSlice->mb_pred[uv], curMb->pixcX, 0, vidParam->mbSize[1][0], vidParam->mbSize[1][1]);
      }
    }
  }
//}}}

//{{{
void copy_Image_4x4 (sPixel** imgBuf1, sPixel** imgBuf2, int off1, int off2) {

  memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE * sizeof (sPixel));
  memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE * sizeof (sPixel));
  memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE * sizeof (sPixel));
  memcpy ((*imgBuf1   + off1), (*imgBuf2   + off2), BLOCK_SIZE * sizeof (sPixel));
  }
//}}}
//{{{
void copy_Image_8x8 (sPixel** imgBuf1, sPixel** imgBuf2, int off1, int off2) {

  for (int j = 0; j < BLOCK_SIZE_8x8; j+=4) {
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE_8x8 * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE_8x8 * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE_8x8 * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), BLOCK_SIZE_8x8 * sizeof (sPixel));
    }
  }
//}}}
//{{{
void copy_Image_16x16 (sPixel** imgBuf1, sPixel** imgBuf2, int off1, int off2) {

  for (int j = 0; j < MB_BLOCK_SIZE; j += 4) {
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), MB_BLOCK_SIZE * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), MB_BLOCK_SIZE * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), MB_BLOCK_SIZE * sizeof (sPixel));
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), MB_BLOCK_SIZE * sizeof (sPixel));
    }
  }
//}}}

//{{{
int CheckVertMV (sMacroblock* curMb, int vec1_y, int block_size_y) {

  sVidParam* vidParam = curMb->vidParam;
  sPicture* picture = curMb->slice->picture;

  int y_pos = vec1_y>>2;
  int maxold_y = (curMb->mbField) ? (picture->sizeY >> 1) - 1 : picture->size_y_m1;

  if (y_pos < (-vidParam->iLumaPadY + 2) || y_pos > (maxold_y + vidParam->iLumaPadY - block_size_y - 2))
    return 1;
  else
    return 0;
  }
//}}}
//{{{
void copy_Image (sPixel** imgBuf1, sPixel** imgBuf2, int off1, int off2, int width, int height) {

  for (int j = 0; j < height; ++j)
    memcpy ((*imgBuf1++ + off1), (*imgBuf2++ + off2), width * sizeof (sPixel));
  }
//}}}
