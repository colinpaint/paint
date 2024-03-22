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
static void sample_reconstruct (sPixel** curImg, sPixel** mpr, int** mbRess, int mb_x,
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
void itrans4x4 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice* slice = mb->slice;
  int** mbRess = slice->mbRess[plane];
  inverse4x4 (slice->cof[plane],mbRess,joff,ioff);

  sample_reconstruct (&slice->mbRec[plane][joff], &slice->mbPred[plane][joff], &mbRess[joff], ioff, ioff, BLOCK_SIZE, BLOCK_SIZE, mb->decoder->coding.maxPelValueComp[plane], DQ_BITS);
  }
//}}}
//{{{
void itrans4x4_ls (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice* slice = mb->slice;
  sPixel** mbPred = slice->mbPred[plane];
  sPixel** mbRec = slice->mbRec[plane];
  int** mbRess = slice->mbRess [plane];

  sDecoder* decoder = mb->decoder;
  int max_imgpel_value = decoder->coding.maxPelValueComp[plane];
  for (int j = joff; j < joff + BLOCK_SIZE; ++j)
    for (int i = ioff; i < ioff + BLOCK_SIZE; ++i)
      mbRec[j][i] = (sPixel) iClip1(max_imgpel_value, mbPred[j][i] + mbRess[j][i]);
  }
//}}}

//{{{
void Inv_Residual_trans_4x4 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice* slice = mb->slice;
  sPixel** mbPred = slice->mbPred[plane];
  sPixel** mbRec = slice->mbRec[plane];
  int** mbRess = slice->mbRess[plane];
  int** cof = slice->cof[plane];

  int temp[4][4];
  if (mb->ipmode_DPCM == VERT_PRED) {
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

  else if (mb->ipmode_DPCM == HOR_PRED) {
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
void Inv_Residual_trans_8x8 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice* slice = mb->slice;
  sPixel** mbPred = slice->mbPred[plane];
  sPixel** mbRec  = slice->mbRec[plane];
  int** mbRess = slice->mbRess[plane];

  int temp[8][8];
  if (mb->ipmode_DPCM == VERT_PRED) {
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
  else if (mb->ipmode_DPCM == HOR_PRED) {
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
void Inv_Residual_trans_16x16 (sMacroBlock* mb, eColorPlane plane) {

  sSlice* slice = mb->slice;
  sPixel** mbPred = slice->mbPred[plane];
  sPixel** mbRec = slice->mbRec[plane];
  int** mbRess = slice->mbRess[plane];
  int** cof = slice->cof[plane];

  int temp[16][16];
  if (mb->ipmode_DPCM == VERT_PRED_16) {
    for (int i = 0; i < MB_BLOCK_SIZE; ++i) {
      temp[0][i] = cof[0][i];
      for (int j = 1; j < MB_BLOCK_SIZE; j++)
        temp[j][i] = cof[j][i] + temp[j-1][i];
      }

    for (int i = 0; i < MB_BLOCK_SIZE; ++i)
      for (int j = 0; j < MB_BLOCK_SIZE; j++)
        mbRess[j][i]=temp[j][i];
    }

  else if (mb->ipmode_DPCM == HOR_PRED_16) {
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
void Inv_Residual_trans_Chroma (sMacroBlock* mb, int uv) {

  sSlice* slice = mb->slice;
  int** mbRess = slice->mbRess[uv+1];
  int** cof = slice->cof[uv+1];

  int width = mb->decoder->mbCrSizeX;
  int height = mb->decoder->mbCrSizeY;

  int temp[16][16];
  if (mb->cPredMode == VERT_PRED_8) {
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
void itrans_2 (sMacroBlock* mb, eColorPlane plane) {

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int transform_pl = (decoder->coding.sepColourPlaneFlag != 0) ? PLANE_Y : plane;
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

  free_mem2Dint(M4);
  }
//}}}
//{{{
void itrans_sp (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;

  int qp = (slice->sliceType == eSIslice) ? slice->qs : slice->qp;
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

  if (slice->spSwitch || slice->sliceType == eSIslice) {
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

  free_mem2Dint (PBlock);
  }
//}}}
//{{{
void itrans_sp_cr (sMacroBlock* mb, int uv) {

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  int mp1[BLOCK_SIZE];
  sPixel** mbPred = slice->mbPred[uv + 1];
  int** cof = slice->cof[uv + 1];
  int** PBlock = new_mem2Dint(MB_BLOCK_SIZE, MB_BLOCK_SIZE);

  int qp_per = decoder->qpPerMatrix[((slice->qp < 0 ? slice->qp : QP_SCALE_CR[slice->qp]))];
  int qp_rem = decoder->qpRemMatrix[((slice->qp < 0 ? slice->qp : QP_SCALE_CR[slice->qp]))];
  int qp_per_sp = decoder->qpPerMatrix[((slice->qs < 0 ? slice->qs : QP_SCALE_CR[slice->qs]))];
  int qp_rem_sp = decoder->qpRemMatrix[((slice->qs < 0 ? slice->qs : QP_SCALE_CR[slice->qs]))];
  int q_bits_sp = Q_BITS + qp_per_sp;

  if (slice->sliceType == eSIslice) {
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

  if (slice->spSwitch || slice->sliceType == eSIslice) {
    for (int n2 = 0; n2 < 2; ++n2 )
      for (int n1 = 0; n1 < 2; ++n1 ) {
        // quantization fo predicted block
        int ilev = rshift_rnd_sf (iabs (mp1[n1+n2*2]) * quant_coef[qp_rem_sp][0][0], q_bits_sp + 1);
        // addition
        ilev = isignab (ilev, mp1[n1+n2*2]) + cof[n2<<2][n1<<2];
        // dequantization
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
    for (int n2=0; n2 < 2; ++n2 )
      for (int n1=0; n1 < 2; ++n1 ) {
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

  free_mem2Dint(PBlock);
  }
//}}}
//{{{
void iMBtrans4x4 (sMacroBlock* mb, eColorPlane plane, int smb) {

  sSlice* slice = mb->slice;
  sPicture* picture = mb->slice->picture;
  sPixel** curr_img = plane ? picture->imgUV[plane - 1] : picture->imgY;

  if (mb->isLossless && mb->mbType == I16MB)
    Inv_Residual_trans_16x16(mb, plane);
  else if (smb || mb->isLossless == TRUE) {
    mb->iTrans4x4 = (smb) ? itrans_sp : ((mb->isLossless == FALSE) ? itrans4x4 : Inv_Residual_trans_4x4);
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

    if (mb->isIntraBlock == FALSE) {
      if (mb->cbp & 0x01) {
        inverse4x4 (cof, mbRess, 0, 0);
        inverse4x4 (cof, mbRess, 0, 4);
        inverse4x4 (cof, mbRess, 4, 0);
        inverse4x4 (cof, mbRess, 4, 4);
        }
      if (mb->cbp & 0x02) {
        inverse4x4 (cof, mbRess, 0, 8);
        inverse4x4 (cof, mbRess, 0, 12);
        inverse4x4 (cof, mbRess, 4, 8);
        inverse4x4 (cof, mbRess, 4, 12);
        }
      if (mb->cbp & 0x04) {
        inverse4x4 (cof, mbRess, 8, 0);
        inverse4x4 (cof, mbRess, 8, 4);
        inverse4x4 (cof, mbRess, 12, 0);
        inverse4x4 (cof, mbRess, 12, 4);
        }
      if (mb->cbp & 0x08) {
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
  copy_Image_16x16 (&curr_img[mb->pixY], slice->mbRec[plane], mb->pixX, 0);
  }
//}}}
//{{{
void iMBtrans8x8 (sMacroBlock* mb, eColorPlane plane) {

  //sDecoder* decoder = mb->decoder;
  sPicture* picture = mb->slice->picture;
  sPixel** curr_img = plane ? picture->imgUV[plane - 1]: picture->imgY;

  // Perform 8x8 idct
  if (mb->cbp & 0x01)
    itrans8x8 (mb, plane, 0, 0);
  else
    icopy8x8 (mb, plane, 0, 0);

  if (mb->cbp & 0x02)
    itrans8x8 (mb, plane, 8, 0);
  else
    icopy8x8 (mb, plane, 8, 0);

  if (mb->cbp & 0x04)
    itrans8x8 (mb, plane, 0, 8);
  else
    icopy8x8 (mb, plane, 0, 8);

  if (mb->cbp & 0x08)
    itrans8x8 (mb, plane, 8, 8);
  else
    icopy8x8 (mb, plane, 8, 8);

  copy_Image_16x16 (&curr_img[mb->pixY], mb->slice->mbRec[plane], mb->pixX, 0);
  }
//}}}
//{{{
void iTransform (sMacroBlock* mb, eColorPlane plane, int smb) {

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  sPicture* picture = slice->picture;
  sPixel** curr_img;
  int uv = plane-1;

  if ((mb->cbp & 15) != 0 || smb) {
    if (mb->lumaTransformSize8x8flag == 0)
      iMBtrans4x4 (mb, plane, smb);
    else
      iMBtrans8x8 (mb, plane);
    }
  else {
    curr_img = plane ? picture->imgUV[uv] : picture->imgY;
    copy_Image_16x16(&curr_img[mb->pixY], slice->mbPred[plane], mb->pixX, 0);
    }

  if (smb)
    slice->isResetCoef = FALSE;

  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
    sPixel** curUV;
    int ioff, joff;
    sPixel** mbRec;
    for (uv = PLANE_U; uv <= PLANE_V; ++uv) {
      curUV = &picture->imgUV[uv - 1][mb->piccY];
      mbRec = slice->mbRec[uv];
      if (!smb && (mb->cbp >> 4)) {
        if (mb->isLossless == FALSE) {
          const unsigned char *x_pos, *y_pos;
          for (int b8 = 0; b8 < (decoder->coding.numUvBlocks); ++b8) {
            x_pos = subblk_offset_x[1][b8];
            y_pos = subblk_offset_y[1][b8];
            itrans4x4 (mb, uv, *x_pos++, *y_pos++);
            itrans4x4 (mb, uv, *x_pos++, *y_pos++);
            itrans4x4 (mb, uv, *x_pos++, *y_pos++);
            itrans4x4 (mb, uv, *x_pos  , *y_pos  );
            }
          sample_reconstruct (mbRec, slice->mbPred[uv], slice->mbRess[uv], 0, 0,
            decoder->mbSize[1][0], decoder->mbSize[1][1], mb->decoder->coding.maxPelValueComp[uv], DQ_BITS);
          }
        else {
          for (int b8 = 0; b8 < (decoder->coding.numUvBlocks); ++b8) {
            const unsigned char* x_pos = subblk_offset_x[1][b8];
            const unsigned char* y_pos = subblk_offset_y[1][b8];
            for (int i = 0 ; i < decoder->mbCrSizeY ; i ++)
              for (int j = 0 ; j < decoder->mbCrSizeX ; j ++)
                slice->mbRess[uv][i][j] = slice->cof[uv][i][j] ;

            itrans4x4_ls (mb, uv, *x_pos++, *y_pos++);
            itrans4x4_ls (mb, uv, *x_pos++, *y_pos++);
            itrans4x4_ls (mb, uv, *x_pos++, *y_pos++);
            itrans4x4_ls (mb, uv, *x_pos  , *y_pos  );
            }
          }
        copy_Image (curUV, mbRec, mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        slice->isResetCoefCr = FALSE;
        }
      else if (smb) {
        mb->iTrans4x4 = (mb->isLossless == FALSE) ? itrans4x4 : itrans4x4_ls;
        itrans_sp_cr (mb, uv - 1);

        for (joff = 0; joff < decoder->mbCrSizeY; joff += BLOCK_SIZE)
          for(ioff = 0; ioff < decoder->mbCrSizeX ;ioff += BLOCK_SIZE)
            mb->iTrans4x4 (mb, uv, ioff, joff);

        copy_Image (curUV, mbRec, mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
        slice->isResetCoefCr = FALSE;
        }
      else
        copy_Image (curUV, slice->mbPred[uv], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
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
int CheckVertMV (sMacroBlock* mb, int vec1_y, int blockSizeY) {

  sDecoder* decoder = mb->decoder;
  sPicture* picture = mb->slice->picture;

  int y_pos = vec1_y>>2;
  int maxold_y = (mb->mbField) ? (picture->sizeY >> 1) - 1 : picture->size_y_m1;

  if (y_pos < (-decoder->coding.iLumaPadY + 2) || y_pos > (maxold_y + decoder->coding.iLumaPadY - blockSizeY - 2))
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
