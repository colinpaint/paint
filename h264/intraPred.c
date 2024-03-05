//{{{
/*!
 *************************************************************************************
 * \file intra_pred_common.c
 *
 * \brief
 *    functions for setting up intra prediction modes
 *
 * \author
 *      Main contributors (see contributors.h for copyright,
 *                         address and affiliation details)
 *      - Alexis Michael Tourapis  <alexismt@ieee.org>
 *
 *************************************************************************************
 */
//}}}
//{{{
#include "global.h"

#include "block.h"
#include "intra4x4Pred.h"
#include "mbAccess.h"
#include "image.h"
//}}}

//{{{
// Notation for comments regarding prediction and predictors.
// The pels of the 8x8 block are labeled a..p. The predictor pels above
// are labeled A..H, from the left I..P, and from above left X, as follows:
//
//  Z  A  B  C  D  E  F  G  H  I  J  K  L  M   N  O  P
//  Q  a1 b1 c1 d1 e1 f1 g1 h1
//  R  a2 b2 c2 d2 e2 f2 g2 h2
//  S  a3 b3 c3 d3 e3 f3 g3 h3
//  T  a4 b4 c4 d4 e4 f4 g4 h4
//  U  a5 b5 c5 d5 e5 f5 g5 h5
//  V  a6 b6 c6 d6 e6 f6 g6 h6
//  W  a7 b7 c7 d7 e7 f7 g7 h7
//  X  a8 b8 c8 d8 e8 f8 g8 h8


// Predictor array index definitions
#define P_Z (PredPel[0])
#define P_A (PredPel[1])
#define P_B (PredPel[2])
#define P_C (PredPel[3])
#define P_D (PredPel[4])
#define P_E (PredPel[5])
#define P_F (PredPel[6])
#define P_G (PredPel[7])
#define P_H (PredPel[8])
#define P_I (PredPel[9])
#define P_J (PredPel[10])
#define P_K (PredPel[11])
#define P_L (PredPel[12])
#define P_M (PredPel[13])
#define P_N (PredPel[14])
#define P_O (PredPel[15])
#define P_P (PredPel[16])
#define P_Q (PredPel[17])
#define P_R (PredPel[18])
#define P_S (PredPel[19])
#define P_T (PredPel[20])
#define P_U (PredPel[21])
#define P_V (PredPel[22])
#define P_W (PredPel[23])
#define P_X (PredPel[24])
//}}}
//{{{
/*!
 *************************************************************************************
 * \brief
 *    Prefiltering for Intra8x8 prediction
 *************************************************************************************
 */
static void LowPassForIntra8x8Pred (imgpel *PredPel, int block_up_left, int block_up, int block_left)
{
  int i;
  imgpel LoopArray[25];

  memcpy(&LoopArray[0], &PredPel[0], 25 * sizeof(imgpel));

  if(block_up_left)
  {
    if(block_up && block_left)
    {
      LoopArray[0] = (imgpel) ((P_Q + (P_Z<<1) + P_A + 2)>>2);
    }
    else
    {
      if(block_up)
        LoopArray[0] = (imgpel) ((P_Z + (P_Z<<1) + P_A + 2)>>2);
      else if (block_left)
        LoopArray[0] = (imgpel) ((P_Z + (P_Z<<1) + P_Q + 2)>>2);
    }
  }

  if(block_up)
  {
    if(block_up_left)
    {
      LoopArray[1] = (imgpel) ((PredPel[0] + (PredPel[1]<<1) + PredPel[2] + 2)>>2);
    }
    else
      LoopArray[1] = (imgpel) ((PredPel[1] + (PredPel[1]<<1) + PredPel[2] + 2)>>2);


    for(i = 2; i <16; i++)
    {
      LoopArray[i] = (imgpel) ((PredPel[i-1] + (PredPel[i]<<1) + PredPel[i+1] + 2)>>2);
    }
    LoopArray[16] = (imgpel) ((P_P + (P_P<<1) + P_O + 2)>>2);
  }

  if(block_left)
  {
    if(block_up_left)
      LoopArray[17] = (imgpel) ((P_Z + (P_Q<<1) + P_R + 2)>>2);
    else
      LoopArray[17] = (imgpel) ((P_Q + (P_Q<<1) + P_R + 2)>>2);

    for(i = 18; i <24; i++)
    {
      LoopArray[i] = (imgpel) ((PredPel[i-1] + (PredPel[i]<<1) + PredPel[i+1] + 2)>>2);
    }
    LoopArray[24] = (imgpel) ((P_W + (P_X<<1) + P_X + 2) >> 2);
  }

  memcpy(&PredPel[0], &LoopArray[0], 25 * sizeof(imgpel));
}
//}}}
//{{{
/*!
 *************************************************************************************
 * \brief
 *    Prefiltering for Intra8x8 prediction (Horizontal)
 *************************************************************************************
 */
static void LowPassForIntra8x8PredHor (imgpel *PredPel, int block_up_left, int block_up, int block_left)
{
  int i;
  imgpel LoopArray[25];

  memcpy(&LoopArray[0], &PredPel[0], 25 * sizeof(imgpel));

  if(block_up_left)
  {
    if(block_up && block_left)
    {
      LoopArray[0] = (imgpel) ((P_Q + (P_Z<<1) + P_A + 2)>>2);
    }
    else
    {
      if(block_up)
        LoopArray[0] = (imgpel) ((P_Z + (P_Z<<1) + P_A + 2)>>2);
      else if (block_left)
        LoopArray[0] = (imgpel) ((P_Z + (P_Z<<1) + P_Q + 2)>>2);
    }
  }

  if(block_up)
  {
    if(block_up_left)
    {
      LoopArray[1] = (imgpel) ((PredPel[0] + (PredPel[1]<<1) + PredPel[2] + 2)>>2);
    }
    else
      LoopArray[1] = (imgpel) ((PredPel[1] + (PredPel[1]<<1) + PredPel[2] + 2)>>2);


    for(i = 2; i <16; i++)
    {
      LoopArray[i] = (imgpel) ((PredPel[i-1] + (PredPel[i]<<1) + PredPel[i+1] + 2)>>2);
    }
    LoopArray[16] = (imgpel) ((P_P + (P_P<<1) + P_O + 2)>>2);
  }


  memcpy(&PredPel[0], &LoopArray[0], 17 * sizeof(imgpel));
}
//}}}
//{{{
/*!
 *************************************************************************************
 * \brief
 *    Prefiltering for Intra8x8 prediction (Vertical)
 *************************************************************************************
 */
static void LowPassForIntra8x8PredVer (imgpel *PredPel, int block_up_left, int block_up, int block_left)
{
  // These functions need some cleanup and can be further optimized.
  // For convenience, let us copy all data for now. It is obvious that the filtering makes things a bit more "complex"
  int i;
  imgpel LoopArray[25];

  memcpy(&LoopArray[0], &PredPel[0], 25 * sizeof(imgpel));

  if(block_up_left)
  {
    if(block_up && block_left)
    {
      LoopArray[0] = (imgpel) ((P_Q + (P_Z<<1) + P_A + 2)>>2);
    }
    else
    {
      if(block_up)
        LoopArray[0] = (imgpel) ((P_Z + (P_Z<<1) + P_A + 2)>>2);
      else if (block_left)
        LoopArray[0] = (imgpel) ((P_Z + (P_Z<<1) + P_Q + 2)>>2);
    }
  }

  if(block_left)
  {
    if(block_up_left)
      LoopArray[17] = (imgpel) ((P_Z + (P_Q<<1) + P_R + 2)>>2);
    else
      LoopArray[17] = (imgpel) ((P_Q + (P_Q<<1) + P_R + 2)>>2);

    for(i = 18; i <24; i++)
    {
      LoopArray[i] = (imgpel) ((PredPel[i-1] + (PredPel[i]<<1) + PredPel[i+1] + 2)>>2);
    }
    LoopArray[24] = (imgpel) ((P_W + (P_X<<1) + P_X + 2) >> 2);
  }

  memcpy(&PredPel[0], &LoopArray[0], 25 * sizeof(imgpel));
}

//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 DC prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_dc_pred (Macroblock *currMB,
                                   ColorPlane pl,         //!< current image plane
                                   int ioff,              //!< pixel offset X within MB
                                   int joff)              //!< pixel offset Y within MB
{
  int i,j;
  int s0 = 0;
  imgpel PredPel[25];  // array of predictor pels
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  StorablePicture *dec_picture = currSlice->dec_picture;
  imgpel **imgY = (pl) ? dec_picture->imgUV[pl - 1] : dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a;
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  getNonAffNeighbour(currMB, ioff - 1, joff, mb_size, &pix_a);
  getNonAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getNonAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getNonAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    block_available_left     = pix_a.available ? currSlice->intra_block [pix_a.mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    imgpel **img_pred = &imgY[pix_a.pos_y];
    int pos_x = pix_a.pos_x;
    P_Q = *(*(img_pred ++) + pos_x);
    P_R = *(*(img_pred ++) + pos_x);
    P_S = *(*(img_pred ++) + pos_x);
    P_T = *(*(img_pred ++) + pos_x);
    P_U = *(*(img_pred ++) + pos_x);
    P_V = *(*(img_pred ++) + pos_x);
    P_W = *(*(img_pred ++) + pos_x);
    P_X = *(*(img_pred   ) + pos_x);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  if (block_available_up && block_available_left)
  {
    // no edge
    s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 8) >> 4;
  }
  else if (!block_available_up && block_available_left)
  {
    // upper edge
    s0 = (P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 4) >> 3;
  }
  else if (block_available_up && !block_available_left)
  {
    // left edge
    s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + 4) >> 3;
  }
  else //if (!block_available_up && !block_available_left)
  {
    // top left corner, nothing to predict from
    s0 = pVid->dc_pred_value_comp[pl];
  }

  for(i = ioff; i < ioff + BLOCK_SIZE_8x8; i++)
    mpr[joff][i] = (imgpel) s0;

  for(j = joff + 1; j < joff + BLOCK_SIZE_8x8; j++)
    memcpy(&mpr[j][ioff], &mpr[j - 1][ioff], BLOCK_SIZE_8x8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 vertical prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_vert_pred (Macroblock *currMB,
                                     ColorPlane pl,         //!< current image plane
                                     int ioff,              //!< pixel offset X within MB
                                     int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int i;
  imgpel PredPel[25];  // array of predictor pels
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a;
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;


  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  getNonAffNeighbour(currMB, ioff - 1, joff    , mb_size, &pix_a);
  getNonAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getNonAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getNonAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    block_available_left     = pix_a.available ? currSlice->intra_block [pix_a.mb_addr] : 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_8x8_Vertical prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8PredHor(&(P_Z), block_available_up_left, block_available_up, block_available_left);

  for (i=joff; i < joff + BLOCK_SIZE_8x8; i++)
  {
    memcpy(&mpr[i][ioff], &PredPel[1], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 horizontal prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_hor_pred (Macroblock *currMB,
                                    ColorPlane pl,         //!< current image plane
                                    int ioff,              //!< pixel offset X within MB
                                    int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;


  int j;
  imgpel PredPel[25];  // array of predictor pels
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a;
  PixelPos pix_b, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;

  for (int i=0; i<25;i++) PredPel[i]=0;

#if (IMGTYPE != 0)
  int ipos0 = ioff    , ipos1 = ioff + 1, ipos2 = ioff + 2, ipos3 = ioff + 3;
  int ipos4 = ioff + 4, ipos5 = ioff + 5, ipos6 = ioff + 6, ipos7 = ioff + 7;
#endif
  int jpos;
  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  getNonAffNeighbour(currMB, ioff - 1, joff    , mb_size, &pix_a);
  getNonAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getNonAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    block_available_left     = pix_a.available ? currSlice->intra_block [pix_a.mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_left)
    printf ("warning: Intra_8x8_Horizontal prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_left)
  {
    imgpel **img_pred = &imgY[pix_a.pos_y];
    int pos_x = pix_a.pos_x;
    P_Q = *(*(img_pred ++) + pos_x);
    P_R = *(*(img_pred ++) + pos_x);
    P_S = *(*(img_pred ++) + pos_x);
    P_T = *(*(img_pred ++) + pos_x);
    P_U = *(*(img_pred ++) + pos_x);
    P_V = *(*(img_pred ++) + pos_x);
    P_W = *(*(img_pred ++) + pos_x);
    P_X = *(*(img_pred   ) + pos_x);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8PredVer(&(P_Z), block_available_up_left, block_available_up, block_available_left);

  for (j=0; j < BLOCK_SIZE_8x8; j++)
  {
    jpos = j + joff;
#if (IMGTYPE == 0)
    memset(&mpr[jpos][ioff], (imgpel) (&P_Q)[j], 8 * sizeof(imgpel));
#else
    mpr[jpos][ipos0]  =
      mpr[jpos][ipos1]  =
      mpr[jpos][ipos2]  =
      mpr[jpos][ipos3]  =
      mpr[jpos][ipos4]  =
      mpr[jpos][ipos5]  =
      mpr[jpos][ipos6]  =
      mpr[jpos][ipos7]  = (imgpel) (&P_Q)[j];
#endif
  }

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 diagonal down right prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_diag_down_right_pred (Macroblock *currMB,
                                                ColorPlane pl,         //!< current image plane
                                                int ioff,              //!< pixel offset X within MB
                                                int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  imgpel PredPel[25];    // array of predictor pels
  imgpel PredArray[16];  // array of final prediction values
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a;
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  imgpel *pred_pels;
  imgpel **mb_pred = &currSlice->mb_pred[pl][joff];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  getNonAffNeighbour(currMB, ioff - 1, joff    , mb_size, &pix_a);
  getNonAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getNonAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getNonAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    block_available_left     = pix_a.available ? currSlice->intra_block [pix_a.mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Diagonal_Down_Right prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    imgpel **img_pred = &imgY[pix_a.pos_y];
    int pos_x = pix_a.pos_x;
    P_Q = *(*(img_pred ++) + pos_x);
    P_R = *(*(img_pred ++) + pos_x);
    P_S = *(*(img_pred ++) + pos_x);
    P_T = *(*(img_pred ++) + pos_x);
    P_U = *(*(img_pred ++) + pos_x);
    P_V = *(*(img_pred ++) + pos_x);
    P_W = *(*(img_pred ++) + pos_x);
    P_X = *(*(img_pred   ) + pos_x);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  // Mode DIAG_DOWN_RIGHT_PRED
  PredArray[ 0] = (imgpel) ((P_X + P_V + ((P_W) << 1) + 2) >> 2);
  PredArray[ 1] = (imgpel) ((P_W + P_U + ((P_V) << 1) + 2) >> 2);
  PredArray[ 2] = (imgpel) ((P_V + P_T + ((P_U) << 1) + 2) >> 2);
  PredArray[ 3] = (imgpel) ((P_U + P_S + ((P_T) << 1) + 2) >> 2);
  PredArray[ 4] = (imgpel) ((P_T + P_R + ((P_S) << 1) + 2) >> 2);
  PredArray[ 5] = (imgpel) ((P_S + P_Q + ((P_R) << 1) + 2) >> 2);
  PredArray[ 6] = (imgpel) ((P_R + P_Z + ((P_Q) << 1) + 2) >> 2);
  PredArray[ 7] = (imgpel) ((P_Q + P_A + ((P_Z) << 1) + 2) >> 2);
  PredArray[ 8] = (imgpel) ((P_Z + P_B + ((P_A) << 1) + 2) >> 2);
  PredArray[ 9] = (imgpel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  PredArray[10] = (imgpel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  PredArray[11] = (imgpel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  PredArray[12] = (imgpel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  PredArray[13] = (imgpel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);
  PredArray[14] = (imgpel) ((P_F + P_H + ((P_G) << 1) + 2) >> 2);

  pred_pels = &PredArray[7];

  memcpy((*mb_pred++) + ioff, pred_pels--, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, pred_pels--, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, pred_pels--, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, pred_pels--, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, pred_pels--, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, pred_pels--, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, pred_pels--, 8 * sizeof(imgpel));
  memcpy((*mb_pred  ) + ioff, pred_pels  , 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 diagonal down left prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_diag_down_left_pred (Macroblock *currMB,
                                               ColorPlane pl,         //!< current image plane
                                               int ioff,              //!< pixel offset X within MB
                                               int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  imgpel PredPel[25];    // array of predictor pels
  imgpel PredArray[16];  // array of final prediction values
  imgpel *Pred = &PredArray[0];
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a;
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  for (int i=0; i<25;i++) PredPel[i]=0;

  imgpel **mb_pred = &currSlice->mb_pred[pl][joff];
  int *mb_size = pVid->mb_size[IS_LUMA];

  getNonAffNeighbour(currMB, ioff - 1, joff    , mb_size, &pix_a);
  getNonAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getNonAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getNonAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    block_available_left     = pix_a.available ? currSlice->intra_block [pix_a.mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_8x8_Diagonal_Down_Left prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    imgpel **img_pred = &imgY[pix_a.pos_y];
    int pos_x = pix_a.pos_x;
    P_Q = *(*(img_pred ++) + pos_x);
    P_R = *(*(img_pred ++) + pos_x);
    P_S = *(*(img_pred ++) + pos_x);
    P_T = *(*(img_pred ++) + pos_x);
    P_U = *(*(img_pred ++) + pos_x);
    P_V = *(*(img_pred ++) + pos_x);
    P_W = *(*(img_pred ++) + pos_x);
    P_X = *(*(img_pred   ) + pos_x);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  // Mode DIAG_DOWN_LEFT_PRED
  *Pred++ = (imgpel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_F + P_H + ((P_G) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_G + P_I + ((P_H) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_H + P_J + ((P_I) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_I + P_K + ((P_J) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_J + P_L + ((P_K) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_K + P_M + ((P_L) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_L + P_N + ((P_M) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_M + P_O + ((P_N) << 1) + 2) >> 2);
  *Pred++ = (imgpel) ((P_N + P_P + ((P_O) << 1) + 2) >> 2);
  *Pred   = (imgpel) ((P_O + P_P + ((P_P) << 1) + 2) >> 2);

  Pred = &PredArray[ 0];

  memcpy((*mb_pred++) + ioff, Pred++, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, Pred++, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, Pred++, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, Pred++, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, Pred++, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, Pred++, 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, Pred++, 8 * sizeof(imgpel));
  memcpy((*mb_pred  ) + ioff, Pred  , 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 vertical right prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_vert_right_pred (Macroblock *currMB,
                                           ColorPlane pl,         //!< current image plane
                                           int ioff,              //!< pixel offset X within MB
                                           int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  imgpel PredPel[25];  // array of predictor pels
  imgpel PredArray[22];  // array of final prediction values
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a;
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  for (int i=0; i<25;i++) PredPel[i]=0;

  imgpel *pred_pels;
  imgpel **mb_pred = &currSlice->mb_pred[pl][joff];
  int *mb_size = pVid->mb_size[IS_LUMA];

  getNonAffNeighbour(currMB, ioff - 1, joff    , mb_size, &pix_a);
  getNonAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getNonAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getNonAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    block_available_left     = pix_a.available ? currSlice->intra_block [pix_a.mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Vertical_Right prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    imgpel **img_pred = &imgY[pix_a.pos_y];
    int pos_x = pix_a.pos_x;
    P_Q = *(*(img_pred ++) + pos_x);
    P_R = *(*(img_pred ++) + pos_x);
    P_S = *(*(img_pred ++) + pos_x);
    P_T = *(*(img_pred ++) + pos_x);
    P_U = *(*(img_pred ++) + pos_x);
    P_V = *(*(img_pred ++) + pos_x);
    P_W = *(*(img_pred ++) + pos_x);
    P_X = *(*(img_pred   ) + pos_x);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  pred_pels = PredArray;
  *pred_pels++ = (imgpel) ((P_V + P_T + ((P_U) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_T + P_R + ((P_S) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_R + P_Z + ((P_Q) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_Z + P_A + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_A + P_B + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_B + P_C + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_C + P_D + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_D + P_E + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_E + P_F + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_F + P_G + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_G + P_H + 1) >> 1);

  *pred_pels++ = (imgpel) ((P_W + P_U + ((P_V) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_U + P_S + ((P_T) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_S + P_Q + ((P_R) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_Q + P_A + ((P_Z) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_Z + P_B + ((P_A) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);
  *pred_pels   = (imgpel) ((P_F + P_H + ((P_G) << 1) + 2) >> 2);

  memcpy((*mb_pred++) + ioff, &PredArray[ 3], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[14], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[ 2], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[13], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[ 1], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[12], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[ 0], 8 * sizeof(imgpel));
  memcpy((*mb_pred  ) + ioff, &PredArray[11], 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 vertical left prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_vert_left_pred (Macroblock *currMB,
                                          ColorPlane pl,         //!< current image plane
                                          int ioff,              //!< pixel offset X within MB
                                          int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  imgpel PredPel[25];  // array of predictor pels
  imgpel PredArray[22];  // array of final prediction values
  imgpel *pred_pel = &PredArray[0];
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a;
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  imgpel **mb_pred = &currSlice->mb_pred[pl][joff];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  getNonAffNeighbour(currMB, ioff - 1, joff    , mb_size, &pix_a);
  getNonAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getNonAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getNonAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    block_available_left     = pix_a.available ? currSlice->intra_block [pix_a.mb_addr] : 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    imgpel **img_pred = &imgY[pix_a.pos_y];
    int pos_x = pix_a.pos_x;
    P_Q = *(*(img_pred ++) + pos_x);
    P_R = *(*(img_pred ++) + pos_x);
    P_S = *(*(img_pred ++) + pos_x);
    P_T = *(*(img_pred ++) + pos_x);
    P_U = *(*(img_pred ++) + pos_x);
    P_V = *(*(img_pred ++) + pos_x);
    P_W = *(*(img_pred ++) + pos_x);
    P_X = *(*(img_pred   ) + pos_x);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  *pred_pel++ = (imgpel) ((P_A + P_B + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_B + P_C + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_C + P_D + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_D + P_E + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_E + P_F + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_F + P_G + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_G + P_H + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_H + P_I + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_I + P_J + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_J + P_K + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_K + P_L + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_F + P_H + ((P_G) << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_G + P_I + ((P_H) << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_H + P_J + ((P_I) << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_I + P_K + ((P_J) << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_J + P_L + ((P_K) << 1) + 2) >> 2);
  *pred_pel   = (imgpel) ((P_K + P_M + ((P_L) << 1) + 2) >> 2);

  memcpy((*mb_pred++) + ioff, &PredArray[ 0], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[11], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[ 1], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[12], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[ 2], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[13], 8 * sizeof(imgpel));
  memcpy((*mb_pred++) + ioff, &PredArray[ 3], 8 * sizeof(imgpel));
  memcpy((*mb_pred  ) + ioff, &PredArray[14], 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 horizontal up prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_hor_up_pred (Macroblock *currMB,
                                       ColorPlane pl,         //!< current image plane
                                       int ioff,              //!< pixel offset X within MB
                                       int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  imgpel PredPel[25];     // array of predictor pels
  imgpel PredArray[22];   // array of final prediction values
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a;
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;
  int jpos0 = joff    , jpos1 = joff + 1, jpos2 = joff + 2, jpos3 = joff + 3;
  int jpos4 = joff + 4, jpos5 = joff + 5, jpos6 = joff + 6, jpos7 = joff + 7;

  for (int i=0; i<25;i++) PredPel[i]=0;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  getNonAffNeighbour(currMB, ioff - 1, joff    , mb_size, &pix_a);
  getNonAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getNonAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getNonAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    block_available_left     = pix_a.available ? currSlice->intra_block [pix_a.mb_addr] : 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_left)
    printf ("warning: Intra_8x8_Horizontal_Up prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    imgpel **img_pred = &imgY[pix_a.pos_y];
    int pos_x = pix_a.pos_x;
    P_Q = *(*(img_pred ++) + pos_x);
    P_R = *(*(img_pred ++) + pos_x);
    P_S = *(*(img_pred ++) + pos_x);
    P_T = *(*(img_pred ++) + pos_x);
    P_U = *(*(img_pred ++) + pos_x);
    P_V = *(*(img_pred ++) + pos_x);
    P_W = *(*(img_pred ++) + pos_x);
    P_X = *(*(img_pred   ) + pos_x);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  PredArray[ 0] = (imgpel) ((P_Q + P_R + 1) >> 1);
  PredArray[ 1] = (imgpel) ((P_S + P_Q + ((P_R) << 1) + 2) >> 2);
  PredArray[ 2] = (imgpel) ((P_R + P_S + 1) >> 1);
  PredArray[ 3] = (imgpel) ((P_T + P_R + ((P_S) << 1) + 2) >> 2);
  PredArray[ 4] = (imgpel) ((P_S + P_T + 1) >> 1);
  PredArray[ 5] = (imgpel) ((P_U + P_S + ((P_T) << 1) + 2) >> 2);
  PredArray[ 6] = (imgpel) ((P_T + P_U + 1) >> 1);
  PredArray[ 7] = (imgpel) ((P_V + P_T + ((P_U) << 1) + 2) >> 2);
  PredArray[ 8] = (imgpel) ((P_U + P_V + 1) >> 1);
  PredArray[ 9] = (imgpel) ((P_W + P_U + ((P_V) << 1) + 2) >> 2);
  PredArray[10] = (imgpel) ((P_V + P_W + 1) >> 1);
  PredArray[11] = (imgpel) ((P_X + P_V + ((P_W) << 1) + 2) >> 2);
  PredArray[12] = (imgpel) ((P_W + P_X + 1) >> 1);
  PredArray[13] = (imgpel) ((P_W + P_X + ((P_X) << 1) + 2) >> 2);
  PredArray[14] = (imgpel) P_X;
  PredArray[15] = (imgpel) P_X;
  PredArray[16] = (imgpel) P_X;
  PredArray[17] = (imgpel) P_X;
  PredArray[18] = (imgpel) P_X;
  PredArray[19] = (imgpel) P_X;
  PredArray[20] = (imgpel) P_X;
  PredArray[21] = (imgpel) P_X;

  memcpy(&mpr[jpos0][ioff], &PredArray[0], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos1][ioff], &PredArray[2], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos2][ioff], &PredArray[4], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos3][ioff], &PredArray[6], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos4][ioff], &PredArray[8], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos5][ioff], &PredArray[10], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos6][ioff], &PredArray[12], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos7][ioff], &PredArray[14], 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 horizontal down prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_hor_down_pred (Macroblock *currMB,
                                         ColorPlane pl,         //!< current image plane
                                         int ioff,              //!< pixel offset X within MB
                                         int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  imgpel PredPel[25];  // array of predictor pels
  imgpel PredArray[22];   // array of final prediction values
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a;
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  imgpel *pred_pels;
  imgpel **mb_pred = &currSlice->mb_pred[pl][joff];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  getNonAffNeighbour(currMB, ioff - 1, joff    , mb_size, &pix_a);
  getNonAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getNonAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getNonAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    block_available_left     = pix_a.available ? currSlice->intra_block [pix_a.mb_addr] : 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Horizontal_Down prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    imgpel **img_pred = &imgY[pix_a.pos_y];
    int pos_x = pix_a.pos_x;
    P_Q = *(*(img_pred ++) + pos_x);
    P_R = *(*(img_pred ++) + pos_x);
    P_S = *(*(img_pred ++) + pos_x);
    P_T = *(*(img_pred ++) + pos_x);
    P_U = *(*(img_pred ++) + pos_x);
    P_V = *(*(img_pred ++) + pos_x);
    P_W = *(*(img_pred ++) + pos_x);
    P_X = *(*(img_pred   ) + pos_x);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  pred_pels = PredArray;

  *pred_pels++ = (imgpel) ((P_X + P_W + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_V + P_X + (P_W << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_W + P_V + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_U + P_W + ((P_V) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_V + P_U + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_T + P_V + ((P_U) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_U + P_T + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_S + P_U + ((P_T) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_T + P_S + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_R + P_T + ((P_S) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_S + P_R + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_Q + P_S + ((P_R) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_R + P_Q + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_Z + P_R + ((P_Q) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_Q + P_Z + 1) >> 1);
  *pred_pels++ = (imgpel) ((P_Q + P_A + ((P_Z) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_Z + P_B + ((P_A) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  *pred_pels++ = (imgpel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  *pred_pels   = (imgpel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);

  pred_pels = &PredArray[14];
  memcpy((*mb_pred++) + ioff, pred_pels, 8 * sizeof(imgpel));
  pred_pels -= 2;
  memcpy((*mb_pred++) + ioff, pred_pels, 8 * sizeof(imgpel));
  pred_pels -= 2;
  memcpy((*mb_pred++) + ioff, pred_pels, 8 * sizeof(imgpel));
  pred_pels -= 2;
  memcpy((*mb_pred++) + ioff, pred_pels, 8 * sizeof(imgpel));
  pred_pels -= 2;
  memcpy((*mb_pred++) + ioff, pred_pels, 8 * sizeof(imgpel));
  pred_pels -= 2;
  memcpy((*mb_pred++) + ioff, pred_pels, 8 * sizeof(imgpel));
  pred_pels -= 2;
  memcpy((*mb_pred++) + ioff, pred_pels, 8 * sizeof(imgpel));
  pred_pels -= 2;
  memcpy((*mb_pred  ) + ioff, pred_pels, 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Make intra 8x8 prediction according to all 9 prediction modes.
 *    The routine uses left and upper neighbouring points from
 *    previous coded blocks to do this (if available). Notice that
 *    inaccessible neighbouring points are signalled with a negative
 *    value in the predmode array .
 *
 *  \par Input:
 *     Starting point of current 8x8 block image position
 *
 ************************************************************************
 */
static int intra_pred_8x8_normal (Macroblock *currMB,
                        ColorPlane pl,         //!< Current color plane
                        int ioff,              //!< ioff
                        int joff)              //!< joff

{
  int block_x = (currMB->block_x) + (ioff >> 2);
  int block_y = (currMB->block_y) + (joff >> 2);
  byte predmode = currMB->p_Slice->ipredmode[block_y][block_x];

  currMB->ipmode_DPCM = predmode;  //For residual DPCM

  switch (predmode)
  {
  case DC_PRED:
    return (intra8x8_dc_pred(currMB, pl, ioff, joff));
    break;
  case VERT_PRED:
    return (intra8x8_vert_pred(currMB, pl, ioff, joff));
    break;
  case HOR_PRED:
    return (intra8x8_hor_pred(currMB, pl, ioff, joff));
    break;
  case DIAG_DOWN_RIGHT_PRED:
    return (intra8x8_diag_down_right_pred(currMB, pl, ioff, joff));
    break;
  case DIAG_DOWN_LEFT_PRED:
    return (intra8x8_diag_down_left_pred(currMB, pl, ioff, joff));
    break;
  case VERT_RIGHT_PRED:
    return (intra8x8_vert_right_pred(currMB, pl, ioff, joff));
    break;
  case VERT_LEFT_PRED:
    return (intra8x8_vert_left_pred(currMB, pl, ioff, joff));
    break;
  case HOR_UP_PRED:
    return (intra8x8_hor_up_pred(currMB, pl, ioff, joff));
    break;
  case HOR_DOWN_PRED:
    return (intra8x8_hor_down_pred(currMB, pl, ioff, joff));
  default:
    printf("Error: illegal intra_8x8 prediction mode: %d\n", (int) predmode);
    return SEARCH_SYNC;
    break;
  }
}
//}}}

//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 DC prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra_prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_dc_pred_mbaff (Macroblock *currMB,
                                   ColorPlane pl,         //!< current image plane
                                   int ioff,              //!< pixel offset X within MB
                                   int joff)              //!< pixel offset Y within MB
{
  int i,j;
  int s0 = 0;
  imgpel PredPel[25];  // array of predictor pels
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  StorablePicture *dec_picture = currSlice->dec_picture;
  imgpel **imgY = (pl) ? dec_picture->imgUV[pl - 1] : dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a[8];
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  for (int i=0; i<25;i++) PredPel[i]=0;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (i=0;i<8;i++)
  {
    getAffNeighbour(currMB, ioff - 1, joff + i, mb_size, &pix_a[i]);
  }

  getAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? currSlice->intra_block[pix_a[i].mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].pos_y][pix_a[0].pos_x];
    P_R = imgY[pix_a[1].pos_y][pix_a[1].pos_x];
    P_S = imgY[pix_a[2].pos_y][pix_a[2].pos_x];
    P_T = imgY[pix_a[3].pos_y][pix_a[3].pos_x];
    P_U = imgY[pix_a[4].pos_y][pix_a[4].pos_x];
    P_V = imgY[pix_a[5].pos_y][pix_a[5].pos_x];
    P_W = imgY[pix_a[6].pos_y][pix_a[6].pos_x];
    P_X = imgY[pix_a[7].pos_y][pix_a[7].pos_x];
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  if (block_available_up && block_available_left)
  {
    // no edge
    s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 8) >> 4;
  }
  else if (!block_available_up && block_available_left)
  {
    // upper edge
    s0 = (P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 4) >> 3;
  }
  else if (block_available_up && !block_available_left)
  {
    // left edge
    s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + 4) >> 3;
  }
  else //if (!block_available_up && !block_available_left)
  {
    // top left corner, nothing to predict from
    s0 = pVid->dc_pred_value_comp[pl];
  }

  for(j = joff; j < joff + BLOCK_SIZE_8x8; j++)
    for(i = ioff; i < ioff + BLOCK_SIZE_8x8; i++)
      mpr[j][i] = (imgpel) s0;

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 vertical prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra_prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_vert_pred_mbaff (Macroblock *currMB,
                                     ColorPlane pl,         //!< current image plane
                                     int ioff,              //!< pixel offset X within MB
                                     int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int i;
  imgpel PredPel[25];  // array of predictor pels
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a[8];
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  for (int i=0; i<25;i++) PredPel[i]=0;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (i=0;i<8;i++)
  {
    getAffNeighbour(currMB, ioff - 1, joff + i, mb_size, &pix_a[i]);
  }

  getAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? currSlice->intra_block[pix_a[i].mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_8x8_Vertical prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8PredHor(&(P_Z), block_available_up_left, block_available_up, block_available_left);

  for (i=joff; i < joff + BLOCK_SIZE_8x8; i++)
  {
    memcpy(&mpr[i][ioff], &PredPel[1], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 horizontal prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra_prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_hor_pred_mbaff (Macroblock *currMB,
                                    ColorPlane pl,         //!< current image plane
                                    int ioff,              //!< pixel offset X within MB
                                    int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;


  int i,j;
  imgpel PredPel[25];  // array of predictor pels
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a[8];
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;

  for (int i=0; i<25;i++) PredPel[i]=0;

#if (IMGTYPE != 0)
  int ipos0 = ioff    , ipos1 = ioff + 1, ipos2 = ioff + 2, ipos3 = ioff + 3;
  int ipos4 = ioff + 4, ipos5 = ioff + 5, ipos6 = ioff + 6, ipos7 = ioff + 7;
#endif
  int jpos;
  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (i=0;i<8;i++)
  {
    getAffNeighbour(currMB, ioff - 1, joff + i, mb_size, &pix_a[i]);
  }

  getAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? currSlice->intra_block[pix_a[i].mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_left)
    printf ("warning: Intra_8x8_Horizontal prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].pos_y][pix_a[0].pos_x];
    P_R = imgY[pix_a[1].pos_y][pix_a[1].pos_x];
    P_S = imgY[pix_a[2].pos_y][pix_a[2].pos_x];
    P_T = imgY[pix_a[3].pos_y][pix_a[3].pos_x];
    P_U = imgY[pix_a[4].pos_y][pix_a[4].pos_x];
    P_V = imgY[pix_a[5].pos_y][pix_a[5].pos_x];
    P_W = imgY[pix_a[6].pos_y][pix_a[6].pos_x];
    P_X = imgY[pix_a[7].pos_y][pix_a[7].pos_x];
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8PredVer(&(P_Z), block_available_up_left, block_available_up, block_available_left);

  for (j=0; j < BLOCK_SIZE_8x8; j++)
  {
    jpos = j + joff;
#if (IMGTYPE == 0)
    memset(&mpr[jpos][ioff], (imgpel) (&P_Q)[j], 8 * sizeof(imgpel));
#else
    mpr[jpos][ipos0]  =
      mpr[jpos][ipos1]  =
      mpr[jpos][ipos2]  =
      mpr[jpos][ipos3]  =
      mpr[jpos][ipos4]  =
      mpr[jpos][ipos5]  =
      mpr[jpos][ipos6]  =
      mpr[jpos][ipos7]  = (imgpel) (&P_Q)[j];
#endif
  }

  return DECODING_OK;
}
//}}}
//{{{
                                    /*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 diagonal down right prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_diag_down_right_pred_mbaff (Macroblock *currMB,
                                                ColorPlane pl,         //!< current image plane
                                                int ioff,              //!< pixel offset X within MB
                                                int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;


  int i;
  imgpel PredPel[25];  // array of predictor pels
  imgpel PredArray[16];  // array of final prediction values
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a[8];
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(currMB, ioff - 1, joff + i, mb_size, &pix_a[i]);
  }

  getAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? currSlice->intra_block[pix_a[i].mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Diagonal_Down_Right prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].pos_y][pix_a[0].pos_x];
    P_R = imgY[pix_a[1].pos_y][pix_a[1].pos_x];
    P_S = imgY[pix_a[2].pos_y][pix_a[2].pos_x];
    P_T = imgY[pix_a[3].pos_y][pix_a[3].pos_x];
    P_U = imgY[pix_a[4].pos_y][pix_a[4].pos_x];
    P_V = imgY[pix_a[5].pos_y][pix_a[5].pos_x];
    P_W = imgY[pix_a[6].pos_y][pix_a[6].pos_x];
    P_X = imgY[pix_a[7].pos_y][pix_a[7].pos_x];
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  // Mode DIAG_DOWN_RIGHT_PRED
  PredArray[ 0] = (imgpel) ((P_X + P_V + 2*(P_W) + 2) >> 2);
  PredArray[ 1] = (imgpel) ((P_W + P_U + 2*(P_V) + 2) >> 2);
  PredArray[ 2] = (imgpel) ((P_V + P_T + 2*(P_U) + 2) >> 2);
  PredArray[ 3] = (imgpel) ((P_U + P_S + 2*(P_T) + 2) >> 2);
  PredArray[ 4] = (imgpel) ((P_T + P_R + 2*(P_S) + 2) >> 2);
  PredArray[ 5] = (imgpel) ((P_S + P_Q + 2*(P_R) + 2) >> 2);
  PredArray[ 6] = (imgpel) ((P_R + P_Z + 2*(P_Q) + 2) >> 2);
  PredArray[ 7] = (imgpel) ((P_Q + P_A + 2*(P_Z) + 2) >> 2);
  PredArray[ 8] = (imgpel) ((P_Z + P_B + 2*(P_A) + 2) >> 2);
  PredArray[ 9] = (imgpel) ((P_A + P_C + 2*(P_B) + 2) >> 2);
  PredArray[10] = (imgpel) ((P_B + P_D + 2*(P_C) + 2) >> 2);
  PredArray[11] = (imgpel) ((P_C + P_E + 2*(P_D) + 2) >> 2);
  PredArray[12] = (imgpel) ((P_D + P_F + 2*(P_E) + 2) >> 2);
  PredArray[13] = (imgpel) ((P_E + P_G + 2*(P_F) + 2) >> 2);
  PredArray[14] = (imgpel) ((P_F + P_H + 2*(P_G) + 2) >> 2);

  memcpy(&mpr[joff++][ioff], &PredArray[7], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[6], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[5], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[4], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[3], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[2], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[1], 8 * sizeof(imgpel));
  memcpy(&mpr[joff  ][ioff], &PredArray[0], 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 diagonal down left prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_diag_down_left_pred_mbaff (Macroblock *currMB,
                                               ColorPlane pl,         //!< current image plane
                                               int ioff,              //!< pixel offset X within MB
                                               int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int i;
  imgpel PredPel[25];  // array of predictor pels
  imgpel PredArray[16];  // array of final prediction values
  imgpel *Pred = &PredArray[0];
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a[8];
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(currMB, ioff - 1, joff + i, mb_size, &pix_a[i]);
  }

  getAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? currSlice->intra_block[pix_a[i].mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_8x8_Diagonal_Down_Left prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].pos_y][pix_a[0].pos_x];
    P_R = imgY[pix_a[1].pos_y][pix_a[1].pos_x];
    P_S = imgY[pix_a[2].pos_y][pix_a[2].pos_x];
    P_T = imgY[pix_a[3].pos_y][pix_a[3].pos_x];
    P_U = imgY[pix_a[4].pos_y][pix_a[4].pos_x];
    P_V = imgY[pix_a[5].pos_y][pix_a[5].pos_x];
    P_W = imgY[pix_a[6].pos_y][pix_a[6].pos_x];
    P_X = imgY[pix_a[7].pos_y][pix_a[7].pos_x];
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  // Mode DIAG_DOWN_LEFT_PRED
  *Pred++ = (imgpel) ((P_A + P_C + 2*(P_B) + 2) >> 2);
  *Pred++ = (imgpel) ((P_B + P_D + 2*(P_C) + 2) >> 2);
  *Pred++ = (imgpel) ((P_C + P_E + 2*(P_D) + 2) >> 2);
  *Pred++ = (imgpel) ((P_D + P_F + 2*(P_E) + 2) >> 2);
  *Pred++ = (imgpel) ((P_E + P_G + 2*(P_F) + 2) >> 2);
  *Pred++ = (imgpel) ((P_F + P_H + 2*(P_G) + 2) >> 2);
  *Pred++ = (imgpel) ((P_G + P_I + 2*(P_H) + 2) >> 2);
  *Pred++ = (imgpel) ((P_H + P_J + 2*(P_I) + 2) >> 2);
  *Pred++ = (imgpel) ((P_I + P_K + 2*(P_J) + 2) >> 2);
  *Pred++ = (imgpel) ((P_J + P_L + 2*(P_K) + 2) >> 2);
  *Pred++ = (imgpel) ((P_K + P_M + 2*(P_L) + 2) >> 2);
  *Pred++ = (imgpel) ((P_L + P_N + 2*(P_M) + 2) >> 2);
  *Pred++ = (imgpel) ((P_M + P_O + 2*(P_N) + 2) >> 2);
  *Pred++ = (imgpel) ((P_N + P_P + 2*(P_O) + 2) >> 2);
  *Pred   = (imgpel) ((P_O + 3*(P_P) + 2) >> 2);

  Pred = &PredArray[ 0];

  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(imgpel));
  memcpy(&mpr[joff  ][ioff], Pred  , 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 vertical right prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_vert_right_pred_mbaff (Macroblock *currMB,
                                           ColorPlane pl,         //!< current image plane
                                           int ioff,              //!< pixel offset X within MB
                                           int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int i;
  imgpel PredPel[25];  // array of predictor pels
  imgpel PredArray[22];  // array of final prediction values
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a[8];
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(currMB, ioff - 1, joff + i, mb_size, &pix_a[i]);
  }

  getAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? currSlice->intra_block[pix_a[i].mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Vertical_Right prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].pos_y][pix_a[0].pos_x];
    P_R = imgY[pix_a[1].pos_y][pix_a[1].pos_x];
    P_S = imgY[pix_a[2].pos_y][pix_a[2].pos_x];
    P_T = imgY[pix_a[3].pos_y][pix_a[3].pos_x];
    P_U = imgY[pix_a[4].pos_y][pix_a[4].pos_x];
    P_V = imgY[pix_a[5].pos_y][pix_a[5].pos_x];
    P_W = imgY[pix_a[6].pos_y][pix_a[6].pos_x];
    P_X = imgY[pix_a[7].pos_y][pix_a[7].pos_x];
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  PredArray[ 0] = (imgpel) ((P_V + P_T + (P_U << 1) + 2) >> 2);
  PredArray[ 1] = (imgpel) ((P_T + P_R + (P_S << 1) + 2) >> 2);
  PredArray[ 2] = (imgpel) ((P_R + P_Z + (P_Q << 1) + 2) >> 2);
  PredArray[ 3] = (imgpel) ((P_Z + P_A + 1) >> 1);
  PredArray[ 4] = (imgpel) ((P_A + P_B + 1) >> 1);
  PredArray[ 5] = (imgpel) ((P_B + P_C + 1) >> 1);
  PredArray[ 6] = (imgpel) ((P_C + P_D + 1) >> 1);
  PredArray[ 7] = (imgpel) ((P_D + P_E + 1) >> 1);
  PredArray[ 8] = (imgpel) ((P_E + P_F + 1) >> 1);
  PredArray[ 9] = (imgpel) ((P_F + P_G + 1) >> 1);
  PredArray[10] = (imgpel) ((P_G + P_H + 1) >> 1);

  PredArray[11] = (imgpel) ((P_W + P_U + (P_V << 1) + 2) >> 2);
  PredArray[12] = (imgpel) ((P_U + P_S + (P_T << 1) + 2) >> 2);
  PredArray[13] = (imgpel) ((P_S + P_Q + (P_R << 1) + 2) >> 2);
  PredArray[14] = (imgpel) ((P_Q + P_A + 2*P_Z + 2) >> 2);
  PredArray[15] = (imgpel) ((P_Z + P_B + 2*P_A + 2) >> 2);
  PredArray[16] = (imgpel) ((P_A + P_C + 2*P_B + 2) >> 2);
  PredArray[17] = (imgpel) ((P_B + P_D + 2*P_C + 2) >> 2);
  PredArray[18] = (imgpel) ((P_C + P_E + 2*P_D + 2) >> 2);
  PredArray[19] = (imgpel) ((P_D + P_F + 2*P_E + 2) >> 2);
  PredArray[20] = (imgpel) ((P_E + P_G + 2*P_F + 2) >> 2);
  PredArray[21] = (imgpel) ((P_F + P_H + 2*P_G + 2) >> 2);

  memcpy(&mpr[joff++][ioff], &PredArray[ 3], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[14], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 2], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[13], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 1], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[12], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 0], 8 * sizeof(imgpel));
  memcpy(&mpr[joff  ][ioff], &PredArray[11], 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 vertical left prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_vert_left_pred_mbaff (Macroblock *currMB,
                                          ColorPlane pl,         //!< current image plane
                                          int ioff,              //!< pixel offset X within MB
                                          int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int i;
  imgpel PredPel[25];  // array of predictor pels
  imgpel PredArray[22];  // array of final prediction values
  imgpel *pred_pel = &PredArray[0];
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a[8];
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(currMB, ioff - 1, joff + i, mb_size, &pix_a[i]);
  }

  getAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? currSlice->intra_block[pix_a[i].mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].pos_y][pix_a[0].pos_x];
    P_R = imgY[pix_a[1].pos_y][pix_a[1].pos_x];
    P_S = imgY[pix_a[2].pos_y][pix_a[2].pos_x];
    P_T = imgY[pix_a[3].pos_y][pix_a[3].pos_x];
    P_U = imgY[pix_a[4].pos_y][pix_a[4].pos_x];
    P_V = imgY[pix_a[5].pos_y][pix_a[5].pos_x];
    P_W = imgY[pix_a[6].pos_y][pix_a[6].pos_x];
    P_X = imgY[pix_a[7].pos_y][pix_a[7].pos_x];
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  *pred_pel++ = (imgpel) ((P_A + P_B + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_B + P_C + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_C + P_D + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_D + P_E + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_E + P_F + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_F + P_G + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_G + P_H + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_H + P_I + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_I + P_J + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_J + P_K + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_K + P_L + 1) >> 1);
  *pred_pel++ = (imgpel) ((P_A + P_C + 2*P_B + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_B + P_D + 2*P_C + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_C + P_E + 2*P_D + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_D + P_F + 2*P_E + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_E + P_G + 2*P_F + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_F + P_H + 2*P_G + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_G + P_I + 2*P_H + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_H + P_J + (P_I << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_I + P_K + (P_J << 1) + 2) >> 2);
  *pred_pel++ = (imgpel) ((P_J + P_L + (P_K << 1) + 2) >> 2);
  *pred_pel   = (imgpel) ((P_K + P_M + (P_L << 1) + 2) >> 2);

  memcpy(&mpr[joff++][ioff], &PredArray[ 0], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[11], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 1], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[12], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 2], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[13], 8 * sizeof(imgpel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 3], 8 * sizeof(imgpel));
  memcpy(&mpr[joff  ][ioff], &PredArray[14], 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 horizontal up prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_hor_up_pred_mbaff (Macroblock *currMB,
                                       ColorPlane pl,         //!< current image plane
                                       int ioff,              //!< pixel offset X within MB
                                       int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int i;
  imgpel PredPel[25];  // array of predictor pels
  imgpel PredArray[22];   // array of final prediction values
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a[8];
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;
  int jpos0 = joff    , jpos1 = joff + 1, jpos2 = joff + 2, jpos3 = joff + 3;
  int jpos4 = joff + 4, jpos5 = joff + 5, jpos6 = joff + 6, jpos7 = joff + 7;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(currMB, ioff - 1, joff + i, mb_size, &pix_a[i]);
  }

  getAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? currSlice->intra_block[pix_a[i].mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_left)
    printf ("warning: Intra_8x8_Horizontal_Up prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].pos_y][pix_a[0].pos_x];
    P_R = imgY[pix_a[1].pos_y][pix_a[1].pos_x];
    P_S = imgY[pix_a[2].pos_y][pix_a[2].pos_x];
    P_T = imgY[pix_a[3].pos_y][pix_a[3].pos_x];
    P_U = imgY[pix_a[4].pos_y][pix_a[4].pos_x];
    P_V = imgY[pix_a[5].pos_y][pix_a[5].pos_x];
    P_W = imgY[pix_a[6].pos_y][pix_a[6].pos_x];
    P_X = imgY[pix_a[7].pos_y][pix_a[7].pos_x];
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  PredArray[ 0] = (imgpel) ((P_Q + P_R + 1) >> 1);
  PredArray[ 1] = (imgpel) ((P_S + P_Q + (P_R << 1) + 2) >> 2);
  PredArray[ 2] = (imgpel) ((P_R + P_S + 1) >> 1);
  PredArray[ 3] = (imgpel) ((P_T + P_R + (P_S << 1) + 2) >> 2);
  PredArray[ 4] = (imgpel) ((P_S + P_T + 1) >> 1);
  PredArray[ 5] = (imgpel) ((P_U + P_S + (P_T << 1) + 2) >> 2);
  PredArray[ 6] = (imgpel) ((P_T + P_U + 1) >> 1);
  PredArray[ 7] = (imgpel) ((P_V + P_T + (P_U << 1) + 2) >> 2);
  PredArray[ 8] = (imgpel) ((P_U + P_V + 1) >> 1);
  PredArray[ 9] = (imgpel) ((P_W + P_U + (P_V << 1) + 2) >> 2);
  PredArray[10] = (imgpel) ((P_V + P_W + 1) >> 1);
  PredArray[11] = (imgpel) ((P_X + P_V + (P_W << 1) + 2) >> 2);
  PredArray[12] = (imgpel) ((P_W + P_X + 1) >> 1);
  PredArray[13] = (imgpel) ((P_W + P_X + (P_X << 1) + 2) >> 2);
  PredArray[14] = (imgpel) P_X;
  PredArray[15] = (imgpel) P_X;
  PredArray[16] = (imgpel) P_X;
  PredArray[17] = (imgpel) P_X;
  PredArray[18] = (imgpel) P_X;
  PredArray[19] = (imgpel) P_X;
  PredArray[20] = (imgpel) P_X;
  PredArray[21] = (imgpel) P_X;

  memcpy(&mpr[jpos0][ioff], &PredArray[0], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos1][ioff], &PredArray[2], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos2][ioff], &PredArray[4], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos3][ioff], &PredArray[6], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos4][ioff], &PredArray[8], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos5][ioff], &PredArray[10], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos6][ioff], &PredArray[12], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos7][ioff], &PredArray[14], 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 8x8 horizontal down prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra8x8_hor_down_pred_mbaff (Macroblock *currMB,
                                         ColorPlane pl,         //!< current image plane
                                         int ioff,              //!< pixel offset X within MB
                                         int joff)              //!< pixel offset Y within MB
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int i;
  imgpel PredPel[25];  // array of predictor pels
  imgpel PredArray[22];   // array of final prediction values
  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  PixelPos pix_a[8];
  PixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;
  int jpos0 = joff    , jpos1 = joff + 1, jpos2 = joff + 2, jpos3 = joff + 3;
  int jpos4 = joff + 4, jpos5 = joff + 5, jpos6 = joff + 6, jpos7 = joff + 7;

  imgpel **mpr = currSlice->mb_pred[pl];
  int *mb_size = pVid->mb_size[IS_LUMA];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(currMB, ioff - 1, joff + i, mb_size, &pix_a[i]);
  }

  getAffNeighbour(currMB, ioff    , joff - 1, mb_size, &pix_b);
  getAffNeighbour(currMB, ioff + 8, joff - 1, mb_size, &pix_c);
  getAffNeighbour(currMB, ioff - 1, joff - 1, mb_size, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (pVid->active_pps->constrained_intra_pred_flag)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? currSlice->intra_block[pix_a[i].mb_addr]: 0;
    block_available_up       = pix_b.available ? currSlice->intra_block [pix_b.mb_addr] : 0;
    block_available_up_right = pix_c.available ? currSlice->intra_block [pix_c.mb_addr] : 0;
    block_available_up_left  = pix_d.available ? currSlice->intra_block [pix_d.mb_addr] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Horizontal_Down prediction mode not allowed at mb %d\n", (int) currSlice->current_mb_nr);

  // form predictor pels
  if (block_available_up)
  {
    memcpy(&PredPel[1], &imgY[pix_b.pos_y][pix_b.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[1], pVid->dc_pred_value_comp[pl], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = (imgpel) pVid->dc_pred_value_comp[pl];
#endif
  }

  if (block_available_up_right)
  {
    memcpy(&PredPel[9], &imgY[pix_c.pos_y][pix_c.pos_x], BLOCK_SIZE_8x8 * sizeof(imgpel));
  }
  else
  {
#if (IMGTYPE == 0)
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(imgpel));
#else
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;
#endif
  }

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].pos_y][pix_a[0].pos_x];
    P_R = imgY[pix_a[1].pos_y][pix_a[1].pos_x];
    P_S = imgY[pix_a[2].pos_y][pix_a[2].pos_x];
    P_T = imgY[pix_a[3].pos_y][pix_a[3].pos_x];
    P_U = imgY[pix_a[4].pos_y][pix_a[4].pos_x];
    P_V = imgY[pix_a[5].pos_y][pix_a[5].pos_x];
    P_W = imgY[pix_a[6].pos_y][pix_a[6].pos_x];
    P_X = imgY[pix_a[7].pos_y][pix_a[7].pos_x];
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  if (block_available_up_left)
  {
    P_Z = imgY[pix_d.pos_y][pix_d.pos_x];
  }
  else
  {
    P_Z = (imgpel) pVid->dc_pred_value_comp[pl];
  }

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  PredArray[ 0] = (imgpel) ((P_X + P_W + 1) >> 1);
  PredArray[ 1] = (imgpel) ((P_V + P_X + (P_W << 1) + 2) >> 2);
  PredArray[ 2] = (imgpel) ((P_W + P_V + 1) >> 1);
  PredArray[ 3] = (imgpel) ((P_U + P_W + (P_V << 1) + 2) >> 2);
  PredArray[ 4] = (imgpel) ((P_V + P_U + 1) >> 1);
  PredArray[ 5] = (imgpel) ((P_T + P_V + (P_U << 1) + 2) >> 2);
  PredArray[ 6] = (imgpel) ((P_U + P_T + 1) >> 1);
  PredArray[ 7] = (imgpel) ((P_S + P_U + (P_T << 1) + 2) >> 2);
  PredArray[ 8] = (imgpel) ((P_T + P_S + 1) >> 1);
  PredArray[ 9] = (imgpel) ((P_R + P_T + (P_S << 1) + 2) >> 2);
  PredArray[10] = (imgpel) ((P_S + P_R + 1) >> 1);
  PredArray[11] = (imgpel) ((P_Q + P_S + (P_R << 1) + 2) >> 2);
  PredArray[12] = (imgpel) ((P_R + P_Q + 1) >> 1);
  PredArray[13] = (imgpel) ((P_Z + P_R + (P_Q << 1) + 2) >> 2);
  PredArray[14] = (imgpel) ((P_Q + P_Z + 1) >> 1);
  PredArray[15] = (imgpel) ((P_Q + P_A + 2*P_Z + 2) >> 2);
  PredArray[16] = (imgpel) ((P_Z + P_B + 2*P_A + 2) >> 2);
  PredArray[17] = (imgpel) ((P_A + P_C + 2*P_B + 2) >> 2);
  PredArray[18] = (imgpel) ((P_B + P_D + 2*P_C + 2) >> 2);
  PredArray[19] = (imgpel) ((P_C + P_E + 2*P_D + 2) >> 2);
  PredArray[20] = (imgpel) ((P_D + P_F + 2*P_E + 2) >> 2);
  PredArray[21] = (imgpel) ((P_E + P_G + 2*P_F + 2) >> 2);

  memcpy(&mpr[jpos0][ioff], &PredArray[14], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos1][ioff], &PredArray[12], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos2][ioff], &PredArray[10], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos3][ioff], &PredArray[ 8], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos4][ioff], &PredArray[ 6], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos5][ioff], &PredArray[ 4], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos6][ioff], &PredArray[ 2], 8 * sizeof(imgpel));
  memcpy(&mpr[jpos7][ioff], &PredArray[ 0], 8 * sizeof(imgpel));

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Make intra 8x8 prediction according to all 9 prediction modes.
 *    The routine uses left and upper neighbouring points from
 *    previous coded blocks to do this (if available). Notice that
 *    inaccessible neighbouring points are signalled with a negative
 *    value in the predmode array .
 *
 *  \par Input:
 *     Starting point of current 8x8 block image position
 *
 ************************************************************************
 */
static int intra_pred_8x8_mbaff (Macroblock *currMB,
                   ColorPlane pl,         //!< Current color plane
                   int ioff,              //!< ioff
                   int joff)              //!< joff

{
  int block_x = (currMB->block_x) + (ioff >> 2);
  int block_y = (currMB->block_y) + (joff >> 2);
  byte predmode = currMB->p_Slice->ipredmode[block_y][block_x];

  currMB->ipmode_DPCM = predmode;  //For residual DPCM

  switch (predmode)
  {
  case DC_PRED:
    return (intra8x8_dc_pred_mbaff(currMB, pl, ioff, joff));
    break;
  case VERT_PRED:
    return (intra8x8_vert_pred_mbaff(currMB, pl, ioff, joff));
    break;
  case HOR_PRED:
    return (intra8x8_hor_pred_mbaff(currMB, pl, ioff, joff));
    break;
  case DIAG_DOWN_RIGHT_PRED:
    return (intra8x8_diag_down_right_pred_mbaff(currMB, pl, ioff, joff));
    break;
  case DIAG_DOWN_LEFT_PRED:
    return (intra8x8_diag_down_left_pred_mbaff(currMB, pl, ioff, joff));
    break;
  case VERT_RIGHT_PRED:
    return (intra8x8_vert_right_pred_mbaff(currMB, pl, ioff, joff));
    break;
  case VERT_LEFT_PRED:
    return (intra8x8_vert_left_pred_mbaff(currMB, pl, ioff, joff));
    break;
  case HOR_UP_PRED:
    return (intra8x8_hor_up_pred_mbaff(currMB, pl, ioff, joff));
    break;
  case HOR_DOWN_PRED:
    return (intra8x8_hor_down_pred_mbaff(currMB, pl, ioff, joff));
  default:
    printf("Error: illegal intra_8x8 prediction mode: %d\n", (int) predmode);
    return SEARCH_SYNC;
    break;
  }
}
//}}}

//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 DC prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra_prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra16x16_dc_pred (Macroblock *currMB, ColorPlane pl)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int s0 = 0, s1 = 0, s2 = 0;

  int i,j;

  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY;
  imgpel **mb_pred = &(currSlice->mb_pred[pl][0]);

  PixelPos a, b;

  int up_avail, left_avail;

  getNonAffNeighbour(currMB,   -1,   0, pVid->mb_size[IS_LUMA], &a);
  getNonAffNeighbour(currMB,    0,  -1, pVid->mb_size[IS_LUMA], &b);

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    up_avail      = b.available;
    left_avail    = a.available;
  }
  else
  {
    up_avail      = b.available ? currSlice->intra_block[b.mb_addr] : 0;
    left_avail    = a.available ? currSlice->intra_block[a.mb_addr]: 0;
  }

  // Sum top predictors
  if (up_avail)
  {
    imgpel *pel = &imgY[b.pos_y][b.pos_x];
    for (i = 0; i < MB_BLOCK_SIZE; ++i)
    {
      s1 += *pel++;
    }
  }

  // Sum left predictors
  if (left_avail)
  {
    int pos_y = a.pos_y;
    int pos_x = a.pos_x;
    for (i = 0; i < MB_BLOCK_SIZE; ++i)
    {
      s2 += imgY[pos_y++][pos_x];
    }
  }

  if (up_avail && left_avail)
    s0 = (s1 + s2 + 16)>>5;       // no edge
  else if (!up_avail && left_avail)
    s0 = (s2 + 8)>>4;              // upper edge
  else if (up_avail && !left_avail)
    s0 = (s1 + 8)>>4;              // left edge
  else
    s0 = pVid->dc_pred_value_comp[pl];                            // top left corner, nothing to predict from

  for(j = 0; j < MB_BLOCK_SIZE; ++j)
  {
#if (IMGTYPE == 0)
    memset(mb_pred[j], s0, MB_BLOCK_SIZE * sizeof(imgpel));
#else
    for(i = 0; i < MB_BLOCK_SIZE; i += 4)
    {
      mb_pred[j][i    ]=(imgpel) s0;
      mb_pred[j][i + 1]=(imgpel) s0;
      mb_pred[j][i + 2]=(imgpel) s0;
      mb_pred[j][i + 3]=(imgpel) s0;
    }
#endif
  }

  return DECODING_OK;

}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 vertical prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra16x16_vert_pred (Macroblock *currMB, ColorPlane pl)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int j;

  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY;

  PixelPos b;          //!< pixel position p(0,-1)

  int up_avail;

  getNonAffNeighbour(currMB,    0,   -1, pVid->mb_size[IS_LUMA], &b);

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    up_avail = b.available;
  }
  else
  {
    up_avail = b.available ? currSlice->intra_block[b.mb_addr] : 0;
  }

  if (!up_avail)
    error ("invalid 16x16 intra pred Mode VERT_PRED_16",500);
  {
    imgpel **prd = &currSlice->mb_pred[pl][0];
    imgpel *src = &(imgY[b.pos_y][b.pos_x]);

    for(j=0;j<MB_BLOCK_SIZE; j+= 4)
    {
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(imgpel));
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(imgpel));
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(imgpel));
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(imgpel));
    }
  }

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 horizontal prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra16x16_hor_pred (Macroblock *currMB, ColorPlane pl)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;
  int j;

  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY;
  imgpel **mb_pred = &(currSlice->mb_pred[pl][0]);
  imgpel prediction;
  int pos_y, pos_x;

  PixelPos a;

  int left_avail;

  getNonAffNeighbour(currMB, -1,  0, pVid->mb_size[IS_LUMA], &a);

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    left_avail    = a.available;
  }
  else
  {
    left_avail  = a.available ? currSlice->intra_block[a.mb_addr]: 0;
  }

  if (!left_avail)
    error ("invalid 16x16 intra pred Mode HOR_PRED_16",500);

  pos_y = a.pos_y;
  pos_x = a.pos_x;

  for(j = 0; j < MB_BLOCK_SIZE; ++j)
  {
#if (IMGTYPE == 0)
    imgpel *prd = mb_pred[j];
    prediction = imgY[pos_y++][pos_x];

    memset(prd, prediction, MB_BLOCK_SIZE * sizeof(imgpel));
#else
    int i;
    imgpel *prd = mb_pred[j];
    prediction = imgY[pos_y++][pos_x];

    for(i = 0; i < MB_BLOCK_SIZE; i += 4)
    {
      *prd++= prediction; // store predicted 16x16 block
      *prd++= prediction; // store predicted 16x16 block
      *prd++= prediction; // store predicted 16x16 block
      *prd++= prediction; // store predicted 16x16 block
    }
#endif
  }

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 horizontal prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra16x16_plane_pred (Macroblock *currMB, ColorPlane pl)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int i,j;

  int ih = 0, iv = 0;
  int ib,ic,iaa;

  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY;
  imgpel **mb_pred = &(currSlice->mb_pred[pl][0]);
  imgpel *mpr_line;
  int max_imgpel_value = pVid->max_pel_value_comp[pl];
  int pos_y, pos_x;

  PixelPos a, b, d;

  int up_avail, left_avail, left_up_avail;

  getNonAffNeighbour(currMB, -1,  -1, pVid->mb_size[IS_LUMA], &d);
  getNonAffNeighbour(currMB, -1,   0, pVid->mb_size[IS_LUMA], &a);
  getNonAffNeighbour(currMB,  0,  -1, pVid->mb_size[IS_LUMA], &b);

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    up_avail      = b.available;
    left_avail    = a.available;
    left_up_avail = d.available;
  }
  else
  {
    up_avail      = b.available ? currSlice->intra_block[b.mb_addr] : 0;
    left_avail    = a.available ? currSlice->intra_block[a.mb_addr] : 0;
    left_up_avail = d.available ? currSlice->intra_block[d.mb_addr] : 0;
  }

  if (!up_avail || !left_up_avail  || !left_avail)
    error ("invalid 16x16 intra pred Mode PLANE_16",500);

  mpr_line = &imgY[b.pos_y][b.pos_x+7];
  pos_y = a.pos_y + 7;
  pos_x = a.pos_x;
  for (i = 1; i < 8; ++i)
  {
    ih += i*(mpr_line[i] - mpr_line[-i]);
    iv += i*(imgY[pos_y + i][pos_x] - imgY[pos_y - i][pos_x]);
  }

  ih += 8*(mpr_line[8] - imgY[d.pos_y][d.pos_x]);
  iv += 8*(imgY[pos_y + 8][pos_x] - imgY[d.pos_y][d.pos_x]);

  ib=(5 * ih + 32)>>6;
  ic=(5 * iv + 32)>>6;

  iaa=16 * (mpr_line[8] + imgY[pos_y + 8][pos_x]);
  for (j = 0;j < MB_BLOCK_SIZE; ++j)
  {
    int ibb = iaa + (j - 7) * ic + 16;
    imgpel *prd = mb_pred[j];
    for (i = 0;i < MB_BLOCK_SIZE; i += 4)
    {
      *prd++ = (imgpel) iClip1(max_imgpel_value, ((ibb + (i - 7) * ib) >> 5));
      *prd++ = (imgpel) iClip1(max_imgpel_value, ((ibb + (i - 6) * ib) >> 5));
      *prd++ = (imgpel) iClip1(max_imgpel_value, ((ibb + (i - 5) * ib) >> 5));
      *prd++ = (imgpel) iClip1(max_imgpel_value, ((ibb + (i - 4) * ib) >> 5));
    }
  }// store plane prediction

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 intra prediction blocks
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *    SEARCH_SYNC   search next sync element as errors while decoding occured
 ***********************************************************************
 */
static int intra_pred_16x16_normal (Macroblock *currMB,
                           ColorPlane pl,       //!< Current colorplane (for 4:4:4)
                           int predmode)        //!< prediction mode
{
  switch (predmode)
  {
  case VERT_PRED_16:                       // vertical prediction from block above
    return (intra16x16_vert_pred(currMB, pl));
    break;
  case HOR_PRED_16:                        // horizontal prediction from left block
    return (intra16x16_hor_pred(currMB, pl));
    break;
  case DC_PRED_16:                         // DC prediction
    return (intra16x16_dc_pred(currMB, pl));
    break;
  case PLANE_16:// 16 bit integer plan pred
    return (intra16x16_plane_pred(currMB, pl));
    break;
  default:
    {                                    // indication of fault in bitstream,exit
      printf("illegal 16x16 intra prediction mode input: %d\n",predmode);
      return SEARCH_SYNC;
    }
  }
}

//}}}

//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 DC prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra16x16_dc_pred_mbaff (Macroblock *currMB, ColorPlane pl)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int s0 = 0, s1 = 0, s2 = 0;

  int i,j;

  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY;
  imgpel **mb_pred = &(currSlice->mb_pred[pl][0]);

  PixelPos b;          //!< pixel position p(0,-1)
  PixelPos left[17];    //!< pixel positions p(-1, -1..15)

  int up_avail, left_avail;

  s1=s2=0;

  for (i=0;i<17;++i)
  {
    getAffNeighbour(currMB, -1,  i-1, pVid->mb_size[IS_LUMA], &left[i]);
  }
  getAffNeighbour(currMB,    0,   -1, pVid->mb_size[IS_LUMA], &b);

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    up_avail      = b.available;
    left_avail    = left[1].available;
  }
  else
  {
    up_avail      = b.available ? currSlice->intra_block[b.mb_addr] : 0;
    for (i = 1, left_avail = 1; i < 17; ++i)
      left_avail  &= left[i].available ? currSlice->intra_block[left[i].mb_addr]: 0;
  }

  for (i = 0; i < MB_BLOCK_SIZE; ++i)
  {
    if (up_avail)
      s1 += imgY[b.pos_y][b.pos_x+i];    // sum hor pix
    if (left_avail)
      s2 += imgY[left[i + 1].pos_y][left[i + 1].pos_x];    // sum vert pix
  }
  if (up_avail && left_avail)
    s0 = (s1 + s2 + 16)>>5;       // no edge
  else if (!up_avail && left_avail)
    s0 = (s2 + 8)>>4;              // upper edge
  else if (up_avail && !left_avail)
    s0 = (s1 + 8)>>4;              // left edge
  else
    s0 = pVid->dc_pred_value_comp[pl];                            // top left corner, nothing to predict from

  for(j = 0; j < MB_BLOCK_SIZE; ++j)
  {
#if (IMGTYPE == 0)
    memset(mb_pred[j], s0, MB_BLOCK_SIZE * sizeof(imgpel));
#else
    for(i = 0; i < MB_BLOCK_SIZE; ++i)
    {
      mb_pred[j][i]=(imgpel) s0;
    }
#endif
  }

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 vertical prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra16x16_vert_pred_mbaff (Macroblock *currMB, ColorPlane pl)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int j;

  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY;

  PixelPos b;          //!< pixel position p(0,-1)

  int up_avail;

  getAffNeighbour(currMB,    0,   -1, pVid->mb_size[IS_LUMA], &b);

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    up_avail = b.available;
  }
  else
  {
    up_avail = b.available ? currSlice->intra_block[b.mb_addr] : 0;
  }

  if (!up_avail)
    error ("invalid 16x16 intra pred Mode VERT_PRED_16",500);
  {
    imgpel **prd = &currSlice->mb_pred[pl][0];
    imgpel *src = &(imgY[b.pos_y][b.pos_x]);

    for(j=0;j<MB_BLOCK_SIZE; j+= 4)
    {
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(imgpel));
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(imgpel));
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(imgpel));
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(imgpel));
    }
  }

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 horizontal prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra16x16_hor_pred_mbaff (Macroblock *currMB, ColorPlane pl)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;
  int i,j;

  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY;
  imgpel **mb_pred = &(currSlice->mb_pred[pl][0]);
  imgpel prediction;

  PixelPos left[17];    //!< pixel positions p(-1, -1..15)

  int left_avail;

  for (i=0;i<17;++i)
  {
    getAffNeighbour(currMB, -1,  i-1, pVid->mb_size[IS_LUMA], &left[i]);
  }

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    left_avail    = left[1].available;
  }
  else
  {
    for (i = 1, left_avail = 1; i < 17; ++i)
      left_avail  &= left[i].available ? currSlice->intra_block[left[i].mb_addr]: 0;
  }

  if (!left_avail)
    error ("invalid 16x16 intra pred Mode HOR_PRED_16",500);

  for(j = 0; j < MB_BLOCK_SIZE; ++j)
  {
    prediction = imgY[left[j+1].pos_y][left[j+1].pos_x];
    for(i = 0; i < MB_BLOCK_SIZE; ++i)
      mb_pred[j][i]= prediction; // store predicted 16x16 block
  }

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 horizontal prediction mode
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *
 ***********************************************************************
 */
static int intra16x16_plane_pred_mbaff (Macroblock *currMB, ColorPlane pl)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;

  int i,j;

  int ih = 0, iv = 0;
  int ib,ic,iaa;

  imgpel **imgY = (pl) ? currSlice->dec_picture->imgUV[pl - 1] : currSlice->dec_picture->imgY;
  imgpel **mb_pred = &(currSlice->mb_pred[pl][0]);
  imgpel *mpr_line;
  int max_imgpel_value = pVid->max_pel_value_comp[pl];

  PixelPos b;          //!< pixel position p(0,-1)
  PixelPos left[17];    //!< pixel positions p(-1, -1..15)

  int up_avail, left_avail, left_up_avail;

  for (i=0;i<17; ++i)
  {
    getAffNeighbour(currMB, -1,  i-1, pVid->mb_size[IS_LUMA], &left[i]);
  }
  getAffNeighbour(currMB,    0,   -1, pVid->mb_size[IS_LUMA], &b);

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    up_avail      = b.available;
    left_avail    = left[1].available;
    left_up_avail = left[0].available;
  }
  else
  {
    up_avail      = b.available ? currSlice->intra_block[b.mb_addr] : 0;
    for (i = 1, left_avail = 1; i < 17; ++i)
      left_avail  &= left[i].available ? currSlice->intra_block[left[i].mb_addr]: 0;
    left_up_avail = left[0].available ? currSlice->intra_block[left[0].mb_addr]: 0;
  }

  if (!up_avail || !left_up_avail  || !left_avail)
    error ("invalid 16x16 intra pred Mode PLANE_16",500);

  mpr_line = &imgY[b.pos_y][b.pos_x+7];
  for (i = 1; i < 8; ++i)
  {
    ih += i*(mpr_line[i] - mpr_line[-i]);
    iv += i*(imgY[left[8+i].pos_y][left[8+i].pos_x] - imgY[left[8-i].pos_y][left[8-i].pos_x]);
  }

  ih += 8*(mpr_line[8] - imgY[left[0].pos_y][left[0].pos_x]);
  iv += 8*(imgY[left[16].pos_y][left[16].pos_x] - imgY[left[0].pos_y][left[0].pos_x]);

  ib=(5 * ih + 32)>>6;
  ic=(5 * iv + 32)>>6;

  iaa=16 * (mpr_line[8] + imgY[left[16].pos_y][left[16].pos_x]);
  for (j = 0;j < MB_BLOCK_SIZE; ++j)
  {
    for (i = 0;i < MB_BLOCK_SIZE; ++i)
    {
      mb_pred[j][i] = (imgpel) iClip1(max_imgpel_value, ((iaa + (i - 7) * ib + (j - 7) * ic + 16) >> 5));
    }
  }// store plane prediction

  return DECODING_OK;
}
//}}}
//{{{
/*!
 ***********************************************************************
 * \brief
 *    makes and returns 16x16 intra prediction blocks
 *
 * \return
 *    DECODING_OK   decoding of intra prediction mode was successful            \n
 *    SEARCH_SYNC   search next sync element as errors while decoding occured
 ***********************************************************************
 */
static int intra_pred_16x16_mbaff (Macroblock *currMB,
                          ColorPlane pl,       //!< Current colorplane (for 4:4:4)
                          int predmode)        //!< prediction mode
{
  switch (predmode)
  {
  case VERT_PRED_16:                       // vertical prediction from block above
    return (intra16x16_vert_pred_mbaff(currMB, pl));
    break;
  case HOR_PRED_16:                        // horizontal prediction from left block
    return (intra16x16_hor_pred_mbaff(currMB, pl));
    break;
  case DC_PRED_16:                         // DC prediction
    return (intra16x16_dc_pred_mbaff(currMB, pl));
    break;
  case PLANE_16:// 16 bit integer plan pred
    return (intra16x16_plane_pred_mbaff(currMB, pl));
    break;
  default:
    {                                    // indication of fault in bitstream,exit
      printf("illegal 16x16 intra prediction mode input: %d\n",predmode);
      return SEARCH_SYNC;
    }
  }
}
//}}}

//{{{
static void intra_chroma_DC_single (imgpel **curr_img, int up_avail, int left_avail, PixelPos up, PixelPos left, int blk_x, int blk_y, int *pred, int direction )
{
  int i;
  int s0 = 0;

  if ((direction && up_avail) || (!left_avail && up_avail))
  {
    imgpel *cur_pel = &curr_img[up.pos_y][up.pos_x + blk_x];
    for (i = 0; i < 4;++i)
      s0 += *(cur_pel++);
    *pred = (s0 + 2) >> 2;
  }
  else if (left_avail)
  {
    imgpel **cur_pel = &(curr_img[left.pos_y + blk_y - 1]);
    int pos_x = left.pos_x;
    for (i = 0; i < 4;++i)
      s0 += *((*cur_pel++) + pos_x);
    *pred = (s0 + 2) >> 2;
  }
}
//}}}
//{{{
static void intra_chroma_DC_all (imgpel **curr_img, int up_avail, int left_avail, PixelPos up, PixelPos left, int blk_x, int blk_y, int *pred )
{
  int i;
  int s0 = 0, s1 = 0;

  if (up_avail)
  {
    imgpel *cur_pel = &curr_img[up.pos_y][up.pos_x + blk_x];
    for (i = 0; i < 4;++i)
      s0 += *(cur_pel++);
  }

  if (left_avail)
  {
    imgpel **cur_pel = &(curr_img[left.pos_y + blk_y - 1]);
    int pos_x = left.pos_x;
    for (i = 0; i < 4;++i)
      s1 += *((*cur_pel++) + pos_x);
  }

  if (up_avail && left_avail)
    *pred = (s0 + s1 + 4) >> 3;
  else if (up_avail)
    *pred = (s0 + 2) >> 2;
  else if (left_avail)
    *pred = (s1 + 2) >> 2;
}
//}}}
//{{{
static void intrapred_chroma_dc (Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;
  StorablePicture *dec_picture = currSlice->dec_picture;
  int        b8, b4;
  int        yuv = dec_picture->chroma_format_idc - 1;
  int        blk_x, blk_y;
  int        pred, pred1;
  static const int block_pos[3][4][4]= //[yuv][b8][b4]
  {
    { {0, 1, 2, 3},{0, 0, 0, 0},{0, 0, 0, 0},{0, 0, 0, 0}},
    { {0, 1, 2, 3},{2, 3, 2, 3},{0, 0, 0, 0},{0, 0, 0, 0}},
    { {0, 1, 2, 3},{1, 1, 3, 3},{2, 3, 2, 3},{3, 3, 3, 3}}
  };

  PixelPos up;        //!< pixel position  p(0,-1)
  PixelPos left;      //!< pixel positions p(-1, -1..16)
  int up_avail, left_avail;
  imgpel **imgUV0 = dec_picture->imgUV[0];
  imgpel **imgUV1 = dec_picture->imgUV[1];
  imgpel **mb_pred0 = currSlice->mb_pred[0 + 1];
  imgpel **mb_pred1 = currSlice->mb_pred[1 + 1];


  getNonAffNeighbour(currMB, -1,  0, pVid->mb_size[IS_CHROMA], &left);
  getNonAffNeighbour(currMB,  0, -1, pVid->mb_size[IS_CHROMA], &up);

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    up_avail      = up.available;
    left_avail    = left.available;
  }
  else
  {
    up_avail = up.available ? currSlice->intra_block[up.mb_addr] : 0;
    left_avail = left.available ? currSlice->intra_block[left.mb_addr]: 0;
  }

  // DC prediction
  // Note that unlike what is stated in many presentations and papers, this mode does not operate
  // the same way as I_16x16 DC prediction.
  for(b8 = 0; b8 < (pVid->num_uv_blocks) ;++b8)
  {
    for (b4 = 0; b4 < 4; ++b4)
    {
      blk_y = subblk_offset_y[yuv][b8][b4];
      blk_x = subblk_offset_x[yuv][b8][b4];
      pred  = pVid->dc_pred_value_comp[1];
      pred1 = pVid->dc_pred_value_comp[2];
      //===== get prediction value =====
      switch (block_pos[yuv][b8][b4])
      {
      case 0:  //===== TOP LEFT =====
        intra_chroma_DC_all   (imgUV0, up_avail, left_avail, up, left, blk_x, blk_y + 1, &pred);
        intra_chroma_DC_all   (imgUV1, up_avail, left_avail, up, left, blk_x, blk_y + 1, &pred1);
        break;
      case 1: //===== TOP RIGHT =====
        intra_chroma_DC_single(imgUV0, up_avail, left_avail, up, left, blk_x, blk_y + 1, &pred, 1);
        intra_chroma_DC_single(imgUV1, up_avail, left_avail, up, left, blk_x, blk_y + 1, &pred1, 1);
        break;
      case 2: //===== BOTTOM LEFT =====
        intra_chroma_DC_single(imgUV0, up_avail, left_avail, up, left, blk_x, blk_y + 1, &pred, 0);
        intra_chroma_DC_single(imgUV1, up_avail, left_avail, up, left, blk_x, blk_y + 1, &pred1, 0);
        break;
      case 3: //===== BOTTOM RIGHT =====
        intra_chroma_DC_all   (imgUV0, up_avail, left_avail, up, left, blk_x, blk_y + 1, &pred);
        intra_chroma_DC_all   (imgUV1, up_avail, left_avail, up, left, blk_x, blk_y + 1, &pred1);
        break;
      }

#if (IMGTYPE == 0)
      {
        int jj;
        for (jj = blk_y; jj < blk_y + BLOCK_SIZE; ++jj)
        {
          memset(&mb_pred0[jj][blk_x],  pred, BLOCK_SIZE * sizeof(imgpel));
          memset(&mb_pred1[jj][blk_x], pred1, BLOCK_SIZE * sizeof(imgpel));
        }
      }
#else
      {
        int jj, ii;
        for (jj = blk_y; jj < blk_y + BLOCK_SIZE; ++jj)
        {
          for (ii = blk_x; ii < blk_x + BLOCK_SIZE; ++ii)
          {
            mb_pred0[jj][ii]=(imgpel) pred;
            mb_pred1[jj][ii]=(imgpel) pred1;
          }
        }
      }
#endif
    }
  }
}
//}}}
//{{{
static void intrapred_chroma_hor (Macroblock *currMB)
{
  VideoParameters* pVid = currMB->pVid;
  PixelPos a;  //!< pixel positions p(-1, -1..16)
  int left_avail;

  getNonAffNeighbour(currMB, -1, 0, pVid->mb_size[IS_CHROMA], &a);

  if (!pVid->active_pps->constrained_intra_pred_flag)
    left_avail = a.available;
  else
    left_avail = a.available ? currMB->p_Slice->intra_block[a.mb_addr]: 0;
  // Horizontal Prediction
  if (!left_avail )
    error("unexpected HOR_PRED_8 chroma intra prediction mode",-1);
  else
  {
    Slice *currSlice = currMB->p_Slice;
    int cr_MB_x = pVid->mb_cr_size_x;
    int cr_MB_y = pVid->mb_cr_size_y;

    int j;
    StorablePicture *dec_picture = currSlice->dec_picture;
#if (IMGTYPE != 0)
    int i, pred, pred1;
#endif
    int pos_y = a.pos_y;
    int pos_x = a.pos_x;
    imgpel **mb_pred0 = currSlice->mb_pred[0 + 1];
    imgpel **mb_pred1 = currSlice->mb_pred[1 + 1];
    imgpel **i0 = &dec_picture->imgUV[0][pos_y];
    imgpel **i1 = &dec_picture->imgUV[1][pos_y];

    for (j = 0; j < cr_MB_y; ++j)
    {
#if (IMGTYPE == 0)
      memset(mb_pred0[j], (*i0++)[pos_x], cr_MB_x * sizeof(imgpel));
      memset(mb_pred1[j], (*i1++)[pos_x], cr_MB_x * sizeof(imgpel));
#else
      pred  = (*i0++)[pos_x];
      pred1 = (*i1++)[pos_x];
      for (i = 0; i < cr_MB_x; ++i)
      {
        mb_pred0[j][i]=(imgpel) pred;
        mb_pred1[j][i]=(imgpel) pred1;
      }
#endif

    }
  }
}
//}}}
//{{{
static void intrapred_chroma_ver (Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;
  int j;
  StorablePicture *dec_picture = currSlice->dec_picture;

  PixelPos up;        //!< pixel position  p(0,-1)
  int up_avail;
  int cr_MB_x = pVid->mb_cr_size_x;
  int cr_MB_y = pVid->mb_cr_size_y;
  getNonAffNeighbour(currMB, 0, -1, pVid->mb_size[IS_CHROMA], &up);

  if (!pVid->active_pps->constrained_intra_pred_flag)
    up_avail      = up.available;
  else
    up_avail = up.available ? currSlice->intra_block[up.mb_addr] : 0;
  // Vertical Prediction
  if (!up_avail)
    error("unexpected VERT_PRED_8 chroma intra prediction mode",-1);
  else
  {
    imgpel **mb_pred0 = currSlice->mb_pred[1];
    imgpel **mb_pred1 = currSlice->mb_pred[2];
    imgpel *i0 = &(dec_picture->imgUV[0][up.pos_y][up.pos_x]);
    imgpel *i1 = &(dec_picture->imgUV[1][up.pos_y][up.pos_x]);

    for (j = 0; j < cr_MB_y; ++j)
    {
      memcpy(mb_pred0[j], i0, cr_MB_x * sizeof(imgpel));
      memcpy(mb_pred1[j], i1, cr_MB_x * sizeof(imgpel));
    }
  }
}
//}}}
//{{{
static void intrapred_chroma_plane (Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;
  StorablePicture *dec_picture = currSlice->dec_picture;

  PixelPos up;        //!< pixel position  p(0,-1)
  PixelPos up_left;
  PixelPos left;  //!< pixel positions p(-1, -1..16)
  int up_avail, left_avail, left_up_avail;

  getNonAffNeighbour(currMB, -1, -1, pVid->mb_size[IS_CHROMA], &up_left);
  getNonAffNeighbour(currMB, -1,  0, pVid->mb_size[IS_CHROMA], &left);
  getNonAffNeighbour(currMB,  0, -1, pVid->mb_size[IS_CHROMA], &up);

  if (!pVid->active_pps->constrained_intra_pred_flag)
  {
    up_avail      = up.available;
    left_avail    = left.available;
    left_up_avail = up_left.available;
  }
  else
  {
    up_avail      = up.available ? currSlice->intra_block[up.mb_addr] : 0;
    left_avail    = left.available ? currSlice->intra_block[left.mb_addr]: 0;
    left_up_avail = up_left.available ? currSlice->intra_block[up_left.mb_addr]: 0;
  }
  // plane prediction
  if (!left_up_avail || !left_avail || !up_avail)
    error("unexpected PLANE_8 chroma intra prediction mode",-1);
  else
  {
    int cr_MB_x = pVid->mb_cr_size_x;
    int cr_MB_y = pVid->mb_cr_size_y;
    int cr_MB_y2 = (cr_MB_y >> 1);
    int cr_MB_x2 = (cr_MB_x >> 1);

    int i,j;
    int ih, iv, ib, ic, iaa;
    int uv;
    for (uv = 0; uv < 2; uv++)
    {
      imgpel **imgUV = dec_picture->imgUV[uv];
      imgpel **mb_pred = currSlice->mb_pred[uv + 1];
      int max_imgpel_value = pVid->max_pel_value_comp[uv + 1];
      imgpel *upPred = &imgUV[up.pos_y][up.pos_x];
      int pos_x  = up_left.pos_x;
      int pos_y1 = left.pos_y + cr_MB_y2;
      int pos_y2 = pos_y1 - 2;
      //imgpel **predU1 = &imgUV[pos_y1];
      imgpel **predU2 = &imgUV[pos_y2];
      ih = cr_MB_x2 * (upPred[cr_MB_x - 1] - imgUV[up_left.pos_y][pos_x]);

      for (i = 0; i < cr_MB_x2 - 1; ++i)
        ih += (i + 1) * (upPred[cr_MB_x2 + i] - upPred[cr_MB_x2 - 2 - i]);

      iv = cr_MB_y2 * (imgUV[left.pos_y + cr_MB_y - 1][pos_x] - imgUV[up_left.pos_y][pos_x]);

      for (i = 0; i < cr_MB_y2 - 1; ++i)
      {
        iv += (i + 1)*(*(imgUV[pos_y1++] + pos_x) - *((*predU2--) + pos_x));
      }

      ib= ((cr_MB_x == 8 ? 17 : 5) * ih + 2 * cr_MB_x)>>(cr_MB_x == 8 ? 5 : 6);
      ic= ((cr_MB_y == 8 ? 17 : 5) * iv + 2 * cr_MB_y)>>(cr_MB_y == 8 ? 5 : 6);

      iaa = ((imgUV[pos_y1][pos_x] + upPred[cr_MB_x-1]) << 4);

      for (j = 0; j < cr_MB_y; ++j)
      {
        int plane = iaa + (j - cr_MB_y2 + 1) * ic + 16 - (cr_MB_x2 - 1) * ib;

        for (i = 0; i < cr_MB_x; ++i)
          mb_pred[j][i]=(imgpel) iClip1(max_imgpel_value, ((i * ib + plane) >> 5));
      }
    }
  }
}
//}}}
//{{{
static void intra_chroma_DC_single_mbaff (imgpel **curr_img, int up_avail, int left_avail, PixelPos up, PixelPos left[17], int blk_x, int blk_y, int *pred, int direction )
{
  int i;
  int s0 = 0;

  if ((direction && up_avail) || (!left_avail && up_avail))
  {
    for (i = blk_x; i < (blk_x + 4);++i)
      s0 += curr_img[up.pos_y][up.pos_x + i];
    *pred = (s0 + 2) >> 2;
  }
  else if (left_avail)
  {
    for (i = blk_y; i < (blk_y + 4);++i)
      s0 += curr_img[left[i].pos_y][left[i].pos_x];
    *pred = (s0 + 2) >> 2;
  }
}
//}}}
//{{{
static void intra_chroma_DC_all_mbaff (imgpel **curr_img, int up_avail, int left_avail, PixelPos up, PixelPos left[17], int blk_x, int blk_y, int *pred )
{
  int i;
  int s0 = 0, s1 = 0;

  if (up_avail)
    for (i = blk_x; i < (blk_x + 4);++i)
      s0 += curr_img[up.pos_y][up.pos_x + i];

  if (left_avail)
    for (i = blk_y; i < (blk_y + 4);++i)
      s1 += curr_img[left[i].pos_y][left[i].pos_x];

  if (up_avail && left_avail)
    *pred = (s0 + s1 + 4) >> 3;
  else if (up_avail)
    *pred = (s0 + 2) >> 2;
  else if (left_avail)
    *pred = (s1 + 2) >> 2;
}
//}}}
//{{{
static void intrapred_chroma_ver_mbaff (Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;
  int j;
  StorablePicture *dec_picture = currSlice->dec_picture;

  PixelPos up;        //!< pixel position  p(0,-1)
  int up_avail;
  int cr_MB_x = pVid->mb_cr_size_x;
  int cr_MB_y = pVid->mb_cr_size_y;

  getAffNeighbour(currMB, 0, -1, pVid->mb_size[IS_CHROMA], &up);

  if (!pVid->active_pps->constrained_intra_pred_flag)
    up_avail      = up.available;
  else
    up_avail = up.available ? currSlice->intra_block[up.mb_addr] : 0;
  // Vertical Prediction
  if (!up_avail)
    error("unexpected VERT_PRED_8 chroma intra prediction mode",-1);
  else
  {
    imgpel **mb_pred0 = currSlice->mb_pred[1];
    imgpel **mb_pred1 = currSlice->mb_pred[2];
    imgpel *i0 = &(dec_picture->imgUV[0][up.pos_y][up.pos_x]);
    imgpel *i1 = &(dec_picture->imgUV[1][up.pos_y][up.pos_x]);

    for (j = 0; j < cr_MB_y; ++j)
    {
      memcpy(&(mb_pred0[j][0]),i0, cr_MB_x * sizeof(imgpel));
      memcpy(&(mb_pred1[j][0]),i1, cr_MB_x * sizeof(imgpel));
    }
  }
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Chroma Intra prediction. Note that many operations can be moved
 *    outside since they are repeated for both components for no reason.
 ************************************************************************
 */
static void intra_pred_chroma_mbaff (Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters* pVid = currMB->pVid;
  int i,j, ii, jj;
  StorablePicture *dec_picture = currSlice->dec_picture;

  int ih, iv, ib, ic, iaa;

  int        b8, b4;
  int        yuv = dec_picture->chroma_format_idc - 1;
  int        blk_x, blk_y;
  int        pred;
  static const int block_pos[3][4][4]= //[yuv][b8][b4]
  {
    { {0, 1, 4, 5},{0, 0, 0, 0},{0, 0, 0, 0},{0, 0, 0, 0}},
    { {0, 1, 2, 3},{4, 5, 4, 5},{0, 0, 0, 0},{0, 0, 0, 0}},
    { {0, 1, 2, 3},{1, 1, 3, 3},{4, 5, 4, 5},{5, 5, 5, 5}}
  };

  switch (currMB->c_ipred_mode)
  {
  case DC_PRED_8:
    {
      PixelPos up;        //!< pixel position  p(0,-1)
      PixelPos left[17];  //!< pixel positions p(-1, -1..16)

      int up_avail, left_avail[2];

      int cr_MB_y = pVid->mb_cr_size_y;
      int cr_MB_y2 = (cr_MB_y >> 1);

      for (i=0; i < cr_MB_y + 1 ; ++i)
        getAffNeighbour(currMB, -1, i-1, pVid->mb_size[IS_CHROMA], &left[i]);
      getAffNeighbour(currMB, 0, -1, pVid->mb_size[IS_CHROMA], &up);

      if (!pVid->active_pps->constrained_intra_pred_flag)
      {
        up_avail      = up.available;
        left_avail[0] = left_avail[1] = left[1].available;
      }
      else
      {
        up_avail = up.available ? currSlice->intra_block[up.mb_addr] : 0;
        for (i=0, left_avail[0] = 1; i < cr_MB_y2;++i)
          left_avail[0]  &= left[i + 1].available ? currSlice->intra_block[left[i + 1].mb_addr]: 0;

        for (i = cr_MB_y2, left_avail[1] = 1; i<cr_MB_y;++i)
          left_avail[1]  &= left[i + 1].available ? currSlice->intra_block[left[i + 1].mb_addr]: 0;

      }
      // DC prediction
      // Note that unlike what is stated in many presentations and papers, this mode does not operate
      // the same way as I_16x16 DC prediction.
      {
        int pred1;
        imgpel **imgUV0 = dec_picture->imgUV[0];
        imgpel **imgUV1 = dec_picture->imgUV[1];
        imgpel **mb_pred0 = currSlice->mb_pred[0 + 1];
        imgpel **mb_pred1 = currSlice->mb_pred[1 + 1];
        for(b8 = 0; b8 < (pVid->num_uv_blocks) ;++b8)
        {
          for (b4 = 0; b4 < 4; ++b4)
          {
            blk_y = subblk_offset_y[yuv][b8][b4];
            blk_x = subblk_offset_x[yuv][b8][b4];

            pred = pVid->dc_pred_value_comp[1];
            pred1 = pVid->dc_pred_value_comp[2];
            //===== get prediction value =====
            switch (block_pos[yuv][b8][b4])
            {
            case 0:  //===== TOP TOP-LEFT =====
              intra_chroma_DC_all_mbaff    (imgUV0, up_avail, left_avail[0], up, left, blk_x, blk_y + 1, &pred);
              intra_chroma_DC_all_mbaff    (imgUV1, up_avail, left_avail[0], up, left, blk_x, blk_y + 1, &pred1);
              break;
            case 1: //===== TOP TOP-RIGHT =====
              intra_chroma_DC_single_mbaff (imgUV0, up_avail, left_avail[0], up, left, blk_x, blk_y + 1, &pred, 1);
              intra_chroma_DC_single_mbaff (imgUV1, up_avail, left_avail[0], up, left, blk_x, blk_y + 1, &pred1, 1);
              break;
            case 2:  //===== TOP BOTTOM-LEFT =====
              intra_chroma_DC_single_mbaff (imgUV0, up_avail, left_avail[0], up, left, blk_x, blk_y + 1, &pred, 0);
              intra_chroma_DC_single_mbaff (imgUV1, up_avail, left_avail[0], up, left, blk_x, blk_y + 1, &pred1, 0);
              break;
            case 3: //===== TOP BOTTOM-RIGHT =====
              intra_chroma_DC_all_mbaff    (imgUV0, up_avail, left_avail[0], up, left, blk_x, blk_y + 1, &pred);
              intra_chroma_DC_all_mbaff    (imgUV1, up_avail, left_avail[0], up, left, blk_x, blk_y + 1, &pred1);
              break;

            case 4: //===== BOTTOM LEFT =====
              intra_chroma_DC_single_mbaff (imgUV0, up_avail, left_avail[1], up, left, blk_x, blk_y + 1, &pred, 0);
              intra_chroma_DC_single_mbaff (imgUV1, up_avail, left_avail[1], up, left, blk_x, blk_y + 1, &pred1, 0);
              break;
            case 5: //===== BOTTOM RIGHT =====
              intra_chroma_DC_all_mbaff   (imgUV0, up_avail, left_avail[1], up, left, blk_x, blk_y + 1, &pred);
              intra_chroma_DC_all_mbaff   (imgUV1, up_avail, left_avail[1], up, left, blk_x, blk_y + 1, &pred1);
              break;
            }

            for (jj = blk_y; jj < blk_y + BLOCK_SIZE; ++jj)
            {
              for (ii = blk_x; ii < blk_x + BLOCK_SIZE; ++ii)
              {
                mb_pred0[jj][ii]=(imgpel) pred;
                mb_pred1[jj][ii]=(imgpel) pred1;
              }
            }
          }
        }
      }
    }
    break;
  case HOR_PRED_8:
    {
      PixelPos left[17];  //!< pixel positions p(-1, -1..16)

      int left_avail[2];

      int cr_MB_x = pVid->mb_cr_size_x;
      int cr_MB_y = pVid->mb_cr_size_y;
      int cr_MB_y2 = (cr_MB_y >> 1);

      for (i=0; i < cr_MB_y + 1 ; ++i)
        getAffNeighbour(currMB, -1, i-1, pVid->mb_size[IS_CHROMA], &left[i]);

      if (!pVid->active_pps->constrained_intra_pred_flag)
      {
        left_avail[0] = left_avail[1] = left[1].available;
      }
      else
      {
        for (i=0, left_avail[0] = 1; i < cr_MB_y2;++i)
          left_avail[0]  &= left[i + 1].available ? currSlice->intra_block[left[i + 1].mb_addr]: 0;

        for (i = cr_MB_y2, left_avail[1] = 1; i<cr_MB_y;++i)
          left_avail[1]  &= left[i + 1].available ? currSlice->intra_block[left[i + 1].mb_addr]: 0;
      }
      // Horizontal Prediction
      if (!left_avail[0] || !left_avail[1])
        error("unexpected HOR_PRED_8 chroma intra prediction mode",-1);
      else
      {
        int pred1;
        imgpel **mb_pred0 = currSlice->mb_pred[0 + 1];
        imgpel **mb_pred1 = currSlice->mb_pred[1 + 1];
        imgpel **i0 = dec_picture->imgUV[0];
        imgpel **i1 = dec_picture->imgUV[1];
        for (j = 0; j < cr_MB_y; ++j)
        {
          pred = i0[left[1 + j].pos_y][left[1 + j].pos_x];
          pred1 = i1[left[1 + j].pos_y][left[1 + j].pos_x];
          for (i = 0; i < cr_MB_x; ++i)
          {
            mb_pred0[j][i]=(imgpel) pred;
            mb_pred1[j][i]=(imgpel) pred1;
          }
        }
      }
    }
    break;
  case VERT_PRED_8:
    {
      PixelPos up;        //!< pixel position  p(0,-1)

      int up_avail;

      int cr_MB_x = pVid->mb_cr_size_x;
      int cr_MB_y = pVid->mb_cr_size_y;

      getAffNeighbour(currMB, 0, -1, pVid->mb_size[IS_CHROMA], &up);

      if (!pVid->active_pps->constrained_intra_pred_flag)
        up_avail      = up.available;
      else
        up_avail = up.available ? currSlice->intra_block[up.mb_addr] : 0;
      // Vertical Prediction
      if (!up_avail)
        error("unexpected VERT_PRED_8 chroma intra prediction mode",-1);
      else
      {
        imgpel **mb_pred0 = currSlice->mb_pred[0 + 1];
        imgpel **mb_pred1 = currSlice->mb_pred[1 + 1];
        imgpel *i0 = &(dec_picture->imgUV[0][up.pos_y][up.pos_x]);
        imgpel *i1 = &(dec_picture->imgUV[1][up.pos_y][up.pos_x]);
        for (j = 0; j < cr_MB_y; ++j)
        {
          memcpy(&(mb_pred0[j][0]),i0, cr_MB_x * sizeof(imgpel));
          memcpy(&(mb_pred1[j][0]),i1, cr_MB_x * sizeof(imgpel));
        }
      }
    }
    break;
  case PLANE_8:
    {
      PixelPos up;        //!< pixel position  p(0,-1)
      PixelPos left[17];  //!< pixel positions p(-1, -1..16)

      int up_avail, left_avail[2], left_up_avail;

      int cr_MB_x = pVid->mb_cr_size_x;
      int cr_MB_y = pVid->mb_cr_size_y;
      int cr_MB_y2 = (cr_MB_y >> 1);
      int cr_MB_x2 = (cr_MB_x >> 1);

      for (i=0; i < cr_MB_y + 1 ; ++i)
        getAffNeighbour(currMB, -1, i-1, pVid->mb_size[IS_CHROMA], &left[i]);
      getAffNeighbour(currMB, 0, -1, pVid->mb_size[IS_CHROMA], &up);

      if (!pVid->active_pps->constrained_intra_pred_flag)
      {
        up_avail      = up.available;
        left_avail[0] = left_avail[1] = left[1].available;
        left_up_avail = left[0].available;
      }
      else
      {
        up_avail = up.available ? currSlice->intra_block[up.mb_addr] : 0;
        for (i=0, left_avail[0] = 1; i < cr_MB_y2;++i)
          left_avail[0]  &= left[i + 1].available ? currSlice->intra_block[left[i + 1].mb_addr]: 0;

        for (i = cr_MB_y2, left_avail[1] = 1; i<cr_MB_y;++i)
          left_avail[1]  &= left[i + 1].available ? currSlice->intra_block[left[i + 1].mb_addr]: 0;

        left_up_avail = left[0].available ? currSlice->intra_block[left[0].mb_addr]: 0;
      }
      // plane prediction
      if (!left_up_avail || !left_avail[0] || !left_avail[1] || !up_avail)
        error("unexpected PLANE_8 chroma intra prediction mode",-1);
      else
      {
        int uv;
        for (uv = 0; uv < 2; uv++)
        {
          imgpel **imgUV = dec_picture->imgUV[uv];
          imgpel **mb_pred = currSlice->mb_pred[uv + 1];
          int max_imgpel_value = pVid->max_pel_value_comp[uv + 1];
          imgpel *upPred = &imgUV[up.pos_y][up.pos_x];

          ih = cr_MB_x2 * (upPred[cr_MB_x - 1] - imgUV[left[0].pos_y][left[0].pos_x]);
          for (i = 0; i < cr_MB_x2 - 1; ++i)
            ih += (i + 1) * (upPred[cr_MB_x2 + i] - upPred[cr_MB_x2 - 2 - i]);

          iv = cr_MB_y2 * (imgUV[left[cr_MB_y].pos_y][left[cr_MB_y].pos_x] - imgUV[left[0].pos_y][left[0].pos_x]);
          for (i = 0; i < cr_MB_y2 - 1; ++i)
            iv += (i + 1)*(imgUV[left[cr_MB_y2 + 1 + i].pos_y][left[cr_MB_y2 + 1 + i].pos_x] -
            imgUV[left[cr_MB_y2 - 1 - i].pos_y][left[cr_MB_y2 - 1 - i].pos_x]);

          ib= ((cr_MB_x == 8 ? 17 : 5) * ih + 2 * cr_MB_x)>>(cr_MB_x == 8 ? 5 : 6);
          ic= ((cr_MB_y == 8 ? 17 : 5) * iv + 2 * cr_MB_y)>>(cr_MB_y == 8 ? 5 : 6);

          iaa=16*(imgUV[left[cr_MB_y].pos_y][left[cr_MB_y].pos_x] + upPred[cr_MB_x-1]);

          for (j = 0; j < cr_MB_y; ++j)
            for (i = 0; i < cr_MB_x; ++i)
              mb_pred[j][i]=(imgpel) iClip1(max_imgpel_value, ((iaa + (i - cr_MB_x2 + 1) * ib + (j - cr_MB_y2 + 1) * ic + 16) >> 5));
        }
      }
    }
    break;
  default:
    error("illegal chroma intra prediction mode", 600);
    break;
  }
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Chroma Intra prediction. Note that many operations can be moved
 *    outside since they are repeated for both components for no reason.
 ************************************************************************
 */
static void intra_pred_chroma (Macroblock *currMB)
{
  switch (currMB->c_ipred_mode)
  {
  case DC_PRED_8:
    intrapred_chroma_dc(currMB);
    break;
  case HOR_PRED_8:
    intrapred_chroma_hor(currMB);
    break;
  case VERT_PRED_8:
    intrapred_chroma_ver(currMB);
    break;
  case PLANE_8:
    intrapred_chroma_plane(currMB);
    break;
  default:
    error("illegal chroma intra prediction mode", 600);
    break;
  }
}
//}}}

//{{{
void set_intra_prediction_modes (Slice *currSlice) {

  if (currSlice->mb_aff_frame_flag) {
    currSlice->intra_pred_4x4 = intra_pred_4x4_mbaff;
    currSlice->intra_pred_8x8 = intra_pred_8x8_mbaff;
    currSlice->intra_pred_16x16 = intra_pred_16x16_mbaff;
    currSlice->intra_pred_chroma = intra_pred_chroma_mbaff;
    }
  else {
    currSlice->intra_pred_4x4 = intra_pred_4x4_normal;
    currSlice->intra_pred_8x8 = intra_pred_8x8_normal;
    currSlice->intra_pred_16x16 = intra_pred_16x16_normal;
    currSlice->intra_pred_chroma = intra_pred_chroma;
    }
  }
//}}}
