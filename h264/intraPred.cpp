//{{{  includes
#include "global.h"

#include "block.h"
#include "intraPred.h"
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
static void LowPassForIntra8x8Pred (sPixel* PredPel, int block_up_left, int block_up, int block_left) {

  sPixel LoopArray[25];
  memcpy (&LoopArray[0], &PredPel[0], 25 * sizeof(sPixel));

  if (block_up_left) {
    if (block_up && block_left)
      LoopArray[0] = (sPixel) ((P_Q + (P_Z<<1) + P_A + 2)>>2);
    else {
      if(block_up)
        LoopArray[0] = (sPixel) ((P_Z + (P_Z<<1) + P_A + 2)>>2);
      else if (block_left)
        LoopArray[0] = (sPixel) ((P_Z + (P_Z<<1) + P_Q + 2)>>2);
      }
   }

  if (block_up) {
    if (block_up_left)
      LoopArray[1] = (sPixel) ((PredPel[0] + (PredPel[1]<<1) + PredPel[2] + 2)>>2);
    else
      LoopArray[1] = (sPixel) ((PredPel[1] + (PredPel[1]<<1) + PredPel[2] + 2)>>2);


    for (int i = 2; i <16; i++)
      LoopArray[i] = (sPixel) ((PredPel[i-1] + (PredPel[i]<<1) + PredPel[i+1] + 2)>>2);
    LoopArray[16] = (sPixel) ((P_P + (P_P<<1) + P_O + 2)>>2);
    }

  if (block_left) {
    if (block_up_left)
      LoopArray[17] = (sPixel) ((P_Z + (P_Q<<1) + P_R + 2)>>2);
    else
      LoopArray[17] = (sPixel) ((P_Q + (P_Q<<1) + P_R + 2)>>2);

    for (int i = 18; i <24; i++)
      LoopArray[i] = (sPixel) ((PredPel[i-1] + (PredPel[i]<<1) + PredPel[i+1] + 2)>>2);
    LoopArray[24] = (sPixel) ((P_W + (P_X<<1) + P_X + 2) >> 2);
    }

  memcpy (&PredPel[0], &LoopArray[0], 25 * sizeof(sPixel));
  }
//}}}
//{{{
static void LowPassForIntra8x8PredHor (sPixel* PredPel, int block_up_left, int block_up, int block_left) {

  sPixel LoopArray[25];
  memcpy (&LoopArray[0], &PredPel[0], 25 * sizeof(sPixel));

  if (block_up_left) {
    if (block_up && block_left)
      LoopArray[0] = (sPixel) ((P_Q + (P_Z<<1) + P_A + 2)>>2);
    else {
      if (block_up)
        LoopArray[0] = (sPixel) ((P_Z + (P_Z<<1) + P_A + 2)>>2);
      else if (block_left)
        LoopArray[0] = (sPixel) ((P_Z + (P_Z<<1) + P_Q + 2)>>2);
      }
    }

  if (block_up) {
    if (block_up_left)
      LoopArray[1] = (sPixel) ((PredPel[0] + (PredPel[1]<<1) + PredPel[2] + 2)>>2);
    else
      LoopArray[1] = (sPixel) ((PredPel[1] + (PredPel[1]<<1) + PredPel[2] + 2)>>2);

    for (int i = 2; i <16; i++)
      LoopArray[i] = (sPixel) ((PredPel[i-1] + (PredPel[i]<<1) + PredPel[i+1] + 2)>>2);
    LoopArray[16] = (sPixel) ((P_P + (P_P<<1) + P_O + 2)>>2);
    }

  memcpy (&PredPel[0], &LoopArray[0], 17 * sizeof(sPixel));
  }
//}}}
//{{{
static void LowPassForIntra8x8PredVer (sPixel* PredPel, int block_up_left, int block_up, int block_left) {
// These functions need some cleanup and can be further optimized.
// For convenience, let us copy all data for now. It is obvious that the filtering makes things a bit more "complex"

  sPixel LoopArray[25];
  memcpy (&LoopArray[0], &PredPel[0], 25 * sizeof(sPixel));

  if (block_up_left) {
    if (block_up && block_left)
      LoopArray[0] = (sPixel)((P_Q + (P_Z << 1) + P_A + 2) >> 2);
    else {
      if (block_up)
        LoopArray[0] = (sPixel)((P_Z + (P_Z << 1) + P_A + 2) >> 2);
      else if (block_left)
        LoopArray[0] = (sPixel)((P_Z + (P_Z << 1) + P_Q + 2) >> 2);
    }
  }

  if (block_left) {
    if (block_up_left)
      LoopArray[17] = (sPixel)((P_Z + (P_Q << 1) + P_R + 2)>>2);
    else
      LoopArray[17] = (sPixel)((P_Q + (P_Q << 1) + P_R + 2)>>2);

    for (int i = 18; i <24; i++)
      LoopArray[i] = (sPixel)((PredPel[i-1] + (PredPel[i] << 1) + PredPel[i+1] + 2) >> 2);
    LoopArray[24] = (sPixel)((P_W + (P_X << 1) + P_X + 2) >> 2);
    }

  memcpy (&PredPel[0], &LoopArray[0], 25 * sizeof(sPixel));
  }
//}}}

//{{{
static int intra8x8_dc_pred (sMacroBlock * mb, eColorPlane plane, int ioff, int joff) {

  int i,j;
  int s0 = 0;
  sPixel PredPel[25];  // array of predictor pels
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPicture* picture = slice->picture;
  sPixel** imgY = (plane) ? picture->imgUV[plane - 1] : picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a;
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i = 0; i < 25; i++)
    PredPel[i] = 0;

  getNonAffNeighbour(mb, ioff - 1, joff, mbSize, &pix_a);
  getNonAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getNonAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getNonAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available && !(ioff == 8 && joff == 8);
  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_left     = pix_a.available ? slice->intraBlock [pix_a.mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
    }
  else {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
    }

  // form predictor pels
  if (block_available_up)
    memcpy (&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset (&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left) {
    sPixel** img_pred = &imgY[pix_a.posY];
    int posX = pix_a.posX;
    P_Q = *(*(img_pred ++) + posX);
    P_R = *(*(img_pred ++) + posX);
    P_S = *(*(img_pred ++) + posX);
    P_T = *(*(img_pred ++) + posX);
    P_U = *(*(img_pred ++) + posX);
    P_V = *(*(img_pred ++) + posX);
    P_W = *(*(img_pred ++) + posX);
    P_X = *(*(img_pred   ) + posX);
  }
  else {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];
  }

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel)decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred (PredPel, block_available_up_left, block_available_up, block_available_left);

  if (block_available_up && block_available_left)
    // no edge
    s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 8) >> 4;
  else if (!block_available_up && block_available_left)
    // upper edge
    s0 = (P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 4) >> 3;
  else if (block_available_up && !block_available_left)
    // left edge
    s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + 4) >> 3;
  else //if (!block_available_up && !block_available_left)
    // top left corner, nothing to predict from
    s0 = decoder->coding.dcPredValueComp[plane];

  for (i = ioff; i < ioff + BLOCK_SIZE_8x8; i++)
    mpr[joff][i] = (sPixel) s0;

  for (j = joff + 1; j < joff + BLOCK_SIZE_8x8; j++)
    memcpy(&mpr[j][ioff], &mpr[j - 1][ioff], BLOCK_SIZE_8x8 * sizeof(sPixel));

  return eDecodingOk;
  }
//}}}
//{{{
static int intra8x8_vert_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel PredPel[25];  // array of predictor pels
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a;
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;


  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++)
    PredPel[i] = 0;

  getNonAffNeighbour (mb, ioff - 1, joff    , mbSize, &pix_a);
  getNonAffNeighbour (mb, ioff    , joff - 1, mbSize, &pix_b);
  getNonAffNeighbour (mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getNonAffNeighbour (mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_left = pix_a.available ? slice->intraBlock [pix_a.mbIndex] : 0;
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
    }
  else {
    block_available_left = pix_a.available;
    block_available_up = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left = pix_d.available;
   }

  if (!block_available_up)
    printf ("warning: Intra_8x8_Vertical prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy (&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset (&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy (&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset (&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8PredHor (&(P_Z), block_available_up_left, block_available_up, block_available_left);

  for (i = joff; i < joff + BLOCK_SIZE_8x8; i++)
    memcpy(&mpr[i][ioff], &PredPel[1], BLOCK_SIZE_8x8 * sizeof(sPixel));

  return eDecodingOk;
  }
//}}}
//{{{
static int intra8x8_hor_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;

  int j;
  sPixel PredPel[25];  // array of predictor pels
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a;
  sPixelPos pix_b, pix_d;

  for (int i = 0;  i < 25; i++)
    PredPel[i]=0;

  int jpos;
  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  getNonAffNeighbour (mb, ioff - 1, joff    , mbSize, &pix_a);
  getNonAffNeighbour (mb, ioff    , joff - 1, mbSize, &pix_b);
  getNonAffNeighbour (mb, ioff - 1, joff - 1, mbSize, &pix_d);

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_left = pix_a.available ? slice->intraBlock [pix_a.mbIndex]: 0;
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_left = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
    }
  else {
    block_available_left = pix_a.available;
    block_available_up = pix_b.available;
    block_available_up_left = pix_d.available;
    }

  if (!block_available_left)
    printf ("warning: Intra_8x8_Horizontal prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_left) {
    sPixel** img_pred = &imgY[pix_a.posY];
    int posX = pix_a.posX;
    P_Q = *(*(img_pred ++) + posX);
    P_R = *(*(img_pred ++) + posX);
    P_S = *(*(img_pred ++) + posX);
    P_T = *(*(img_pred ++) + posX);
    P_U = *(*(img_pred ++) + posX);
    P_V = *(*(img_pred ++) + posX);
    P_W = *(*(img_pred ++) + posX);
    P_X = *(*(img_pred   ) + posX);
    }
  else {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];
    }

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8PredVer(&(P_Z), block_available_up_left, block_available_up, block_available_left);

  for (j = 0; j < BLOCK_SIZE_8x8; j++) {
    jpos = j + joff;
    memset (&mpr[jpos][ioff], (sPixel) (&P_Q)[j], 8 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra8x8_diag_down_right_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixel PredPel[25];    // array of predictor pels
  sPixel PredArray[16];  // array of final prediction values
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a;
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  sPixel *pred_pels;
  sPixel** mbPred = &slice->mbPred[plane][joff];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++) PredPel[i]=0;

  getNonAffNeighbour(mb, ioff - 1, joff    , mbSize, &pix_a);
  getNonAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getNonAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getNonAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    block_available_left     = pix_a.available ? slice->intraBlock [pix_a.mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Diagonal_Down_Right prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    sPixel** img_pred = &imgY[pix_a.posY];
    int posX = pix_a.posX;
    P_Q = *(*(img_pred ++) + posX);
    P_R = *(*(img_pred ++) + posX);
    P_S = *(*(img_pred ++) + posX);
    P_T = *(*(img_pred ++) + posX);
    P_U = *(*(img_pred ++) + posX);
    P_V = *(*(img_pred ++) + posX);
    P_W = *(*(img_pred ++) + posX);
    P_X = *(*(img_pred   ) + posX);
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  // Mode DIAG_DOWN_RIGHT_PRED
  PredArray[ 0] = (sPixel) ((P_X + P_V + ((P_W) << 1) + 2) >> 2);
  PredArray[ 1] = (sPixel) ((P_W + P_U + ((P_V) << 1) + 2) >> 2);
  PredArray[ 2] = (sPixel) ((P_V + P_T + ((P_U) << 1) + 2) >> 2);
  PredArray[ 3] = (sPixel) ((P_U + P_S + ((P_T) << 1) + 2) >> 2);
  PredArray[ 4] = (sPixel) ((P_T + P_R + ((P_S) << 1) + 2) >> 2);
  PredArray[ 5] = (sPixel) ((P_S + P_Q + ((P_R) << 1) + 2) >> 2);
  PredArray[ 6] = (sPixel) ((P_R + P_Z + ((P_Q) << 1) + 2) >> 2);
  PredArray[ 7] = (sPixel) ((P_Q + P_A + ((P_Z) << 1) + 2) >> 2);
  PredArray[ 8] = (sPixel) ((P_Z + P_B + ((P_A) << 1) + 2) >> 2);
  PredArray[ 9] = (sPixel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  PredArray[10] = (sPixel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  PredArray[11] = (sPixel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  PredArray[12] = (sPixel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  PredArray[13] = (sPixel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);
  PredArray[14] = (sPixel) ((P_F + P_H + ((P_G) << 1) + 2) >> 2);

  pred_pels = &PredArray[7];

  memcpy((*mbPred++) + ioff, pred_pels--, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, pred_pels--, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, pred_pels--, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, pred_pels--, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, pred_pels--, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, pred_pels--, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, pred_pels--, 8 * sizeof(sPixel));
  memcpy((*mbPred  ) + ioff, pred_pels  , 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_diag_down_left_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixel PredPel[25];    // array of predictor pels
  sPixel PredArray[16];  // array of final prediction values
  sPixel *Pred = &PredArray[0];
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a;
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  for (int i=0; i<25;i++) PredPel[i]=0;

  sPixel** mbPred = &slice->mbPred[plane][joff];
  int *mbSize = decoder->mbSize[eLuma];

  getNonAffNeighbour(mb, ioff - 1, joff    , mbSize, &pix_a);
  getNonAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getNonAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getNonAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    block_available_left     = pix_a.available ? slice->intraBlock [pix_a.mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_8x8_Diagonal_Down_Left prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    sPixel** img_pred = &imgY[pix_a.posY];
    int posX = pix_a.posX;
    P_Q = *(*(img_pred ++) + posX);
    P_R = *(*(img_pred ++) + posX);
    P_S = *(*(img_pred ++) + posX);
    P_T = *(*(img_pred ++) + posX);
    P_U = *(*(img_pred ++) + posX);
    P_V = *(*(img_pred ++) + posX);
    P_W = *(*(img_pred ++) + posX);
    P_X = *(*(img_pred   ) + posX);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];
  }

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  // Mode DIAG_DOWN_LEFT_PRED
  *Pred++ = (sPixel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_F + P_H + ((P_G) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_G + P_I + ((P_H) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_H + P_J + ((P_I) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_I + P_K + ((P_J) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_J + P_L + ((P_K) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_K + P_M + ((P_L) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_L + P_N + ((P_M) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_M + P_O + ((P_N) << 1) + 2) >> 2);
  *Pred++ = (sPixel) ((P_N + P_P + ((P_O) << 1) + 2) >> 2);
  *Pred   = (sPixel) ((P_O + P_P + ((P_P) << 1) + 2) >> 2);

  Pred = &PredArray[ 0];

  memcpy((*mbPred++) + ioff, Pred++, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, Pred++, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, Pred++, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, Pred++, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, Pred++, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, Pred++, 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, Pred++, 8 * sizeof(sPixel));
  memcpy((*mbPred  ) + ioff, Pred  , 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_vert_right_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixel PredPel[25];  // array of predictor pels
  sPixel PredArray[22];  // array of final prediction values
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a;
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  for (int i=0; i<25;i++) PredPel[i]=0;

  sPixel *pred_pels;
  sPixel** mbPred = &slice->mbPred[plane][joff];
  int *mbSize = decoder->mbSize[eLuma];

  getNonAffNeighbour(mb, ioff - 1, joff    , mbSize, &pix_a);
  getNonAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getNonAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getNonAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    block_available_left     = pix_a.available ? slice->intraBlock [pix_a.mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Vertical_Right prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    sPixel** img_pred = &imgY[pix_a.posY];
    int posX = pix_a.posX;
    P_Q = *(*(img_pred ++) + posX);
    P_R = *(*(img_pred ++) + posX);
    P_S = *(*(img_pred ++) + posX);
    P_T = *(*(img_pred ++) + posX);
    P_U = *(*(img_pred ++) + posX);
    P_V = *(*(img_pred ++) + posX);
    P_W = *(*(img_pred ++) + posX);
    P_X = *(*(img_pred   ) + posX);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];
  }

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  pred_pels = PredArray;
  *pred_pels++ = (sPixel) ((P_V + P_T + ((P_U) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_T + P_R + ((P_S) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_R + P_Z + ((P_Q) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_Z + P_A + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_A + P_B + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_B + P_C + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_C + P_D + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_D + P_E + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_E + P_F + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_F + P_G + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_G + P_H + 1) >> 1);

  *pred_pels++ = (sPixel) ((P_W + P_U + ((P_V) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_U + P_S + ((P_T) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_S + P_Q + ((P_R) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_Q + P_A + ((P_Z) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_Z + P_B + ((P_A) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);
  *pred_pels   = (sPixel) ((P_F + P_H + ((P_G) << 1) + 2) >> 2);

  memcpy((*mbPred++) + ioff, &PredArray[ 3], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[14], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[ 2], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[13], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[ 1], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[12], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[ 0], 8 * sizeof(sPixel));
  memcpy((*mbPred  ) + ioff, &PredArray[11], 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_vert_left_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixel PredPel[25];  // array of predictor pels
  sPixel PredArray[22];  // array of final prediction values
  sPixel *pred_pel = &PredArray[0];
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a;
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  sPixel** mbPred = &slice->mbPred[plane][joff];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++) PredPel[i]=0;

  getNonAffNeighbour(mb, ioff - 1, joff    , mbSize, &pix_a);
  getNonAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getNonAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getNonAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    block_available_left     = pix_a.available ? slice->intraBlock [pix_a.mbIndex] : 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
     memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    sPixel** img_pred = &imgY[pix_a.posY];
    int posX = pix_a.posX;
    P_Q = *(*(img_pred ++) + posX);
    P_R = *(*(img_pred ++) + posX);
    P_S = *(*(img_pred ++) + posX);
    P_T = *(*(img_pred ++) + posX);
    P_U = *(*(img_pred ++) + posX);
    P_V = *(*(img_pred ++) + posX);
    P_W = *(*(img_pred ++) + posX);
    P_X = *(*(img_pred   ) + posX);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];
  }

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  *pred_pel++ = (sPixel) ((P_A + P_B + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_B + P_C + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_C + P_D + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_D + P_E + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_E + P_F + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_F + P_G + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_G + P_H + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_H + P_I + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_I + P_J + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_J + P_K + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_K + P_L + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_F + P_H + ((P_G) << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_G + P_I + ((P_H) << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_H + P_J + ((P_I) << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_I + P_K + ((P_J) << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_J + P_L + ((P_K) << 1) + 2) >> 2);
  *pred_pel   = (sPixel) ((P_K + P_M + ((P_L) << 1) + 2) >> 2);

  memcpy((*mbPred++) + ioff, &PredArray[ 0], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[11], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[ 1], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[12], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[ 2], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[13], 8 * sizeof(sPixel));
  memcpy((*mbPred++) + ioff, &PredArray[ 3], 8 * sizeof(sPixel));
  memcpy((*mbPred  ) + ioff, &PredArray[14], 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_hor_up_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixel PredPel[25];     // array of predictor pels
  sPixel PredArray[22];   // array of final prediction values
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a;
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;
  int jpos0 = joff    , jpos1 = joff + 1, jpos2 = joff + 2, jpos3 = joff + 3;
  int jpos4 = joff + 4, jpos5 = joff + 5, jpos6 = joff + 6, jpos7 = joff + 7;

  for (int i=0; i<25;i++) PredPel[i]=0;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  getNonAffNeighbour(mb, ioff - 1, joff    , mbSize, &pix_a);
  getNonAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getNonAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getNonAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    block_available_left     = pix_a.available ? slice->intraBlock [pix_a.mbIndex] : 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_left)
    printf ("warning: Intra_8x8_Horizontal_Up prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    sPixel** img_pred = &imgY[pix_a.posY];
    int posX = pix_a.posX;
    P_Q = *(*(img_pred ++) + posX);
    P_R = *(*(img_pred ++) + posX);
    P_S = *(*(img_pred ++) + posX);
    P_T = *(*(img_pred ++) + posX);
    P_U = *(*(img_pred ++) + posX);
    P_V = *(*(img_pred ++) + posX);
    P_W = *(*(img_pred ++) + posX);
    P_X = *(*(img_pred   ) + posX);
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  PredArray[ 0] = (sPixel) ((P_Q + P_R + 1) >> 1);
  PredArray[ 1] = (sPixel) ((P_S + P_Q + ((P_R) << 1) + 2) >> 2);
  PredArray[ 2] = (sPixel) ((P_R + P_S + 1) >> 1);
  PredArray[ 3] = (sPixel) ((P_T + P_R + ((P_S) << 1) + 2) >> 2);
  PredArray[ 4] = (sPixel) ((P_S + P_T + 1) >> 1);
  PredArray[ 5] = (sPixel) ((P_U + P_S + ((P_T) << 1) + 2) >> 2);
  PredArray[ 6] = (sPixel) ((P_T + P_U + 1) >> 1);
  PredArray[ 7] = (sPixel) ((P_V + P_T + ((P_U) << 1) + 2) >> 2);
  PredArray[ 8] = (sPixel) ((P_U + P_V + 1) >> 1);
  PredArray[ 9] = (sPixel) ((P_W + P_U + ((P_V) << 1) + 2) >> 2);
  PredArray[10] = (sPixel) ((P_V + P_W + 1) >> 1);
  PredArray[11] = (sPixel) ((P_X + P_V + ((P_W) << 1) + 2) >> 2);
  PredArray[12] = (sPixel) ((P_W + P_X + 1) >> 1);
  PredArray[13] = (sPixel) ((P_W + P_X + ((P_X) << 1) + 2) >> 2);
  PredArray[14] = (sPixel) P_X;
  PredArray[15] = (sPixel) P_X;
  PredArray[16] = (sPixel) P_X;
  PredArray[17] = (sPixel) P_X;
  PredArray[18] = (sPixel) P_X;
  PredArray[19] = (sPixel) P_X;
  PredArray[20] = (sPixel) P_X;
  PredArray[21] = (sPixel) P_X;

  memcpy(&mpr[jpos0][ioff], &PredArray[0], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos1][ioff], &PredArray[2], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos2][ioff], &PredArray[4], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos3][ioff], &PredArray[6], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos4][ioff], &PredArray[8], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos5][ioff], &PredArray[10], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos6][ioff], &PredArray[12], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos7][ioff], &PredArray[14], 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_hor_down_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixel PredPel[25];  // array of predictor pels
  sPixel PredArray[22];   // array of final prediction values
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a;
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  sPixel *pred_pels;
  sPixel** mbPred = &slice->mbPred[plane][joff];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++) PredPel[i]=0;

  getNonAffNeighbour(mb, ioff - 1, joff    , mbSize, &pix_a);
  getNonAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getNonAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getNonAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    block_available_left     = pix_a.available ? slice->intraBlock [pix_a.mbIndex] : 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Horizontal_Down prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    sPixel** img_pred = &imgY[pix_a.posY];
    int posX = pix_a.posX;
    P_Q = *(*(img_pred ++) + posX);
    P_R = *(*(img_pred ++) + posX);
    P_S = *(*(img_pred ++) + posX);
    P_T = *(*(img_pred ++) + posX);
    P_U = *(*(img_pred ++) + posX);
    P_V = *(*(img_pred ++) + posX);
    P_W = *(*(img_pred ++) + posX);
    P_X = *(*(img_pred   ) + posX);
  }
  else
  {
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];
  }

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  pred_pels = PredArray;

  *pred_pels++ = (sPixel) ((P_X + P_W + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_V + P_X + (P_W << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_W + P_V + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_U + P_W + ((P_V) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_V + P_U + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_T + P_V + ((P_U) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_U + P_T + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_S + P_U + ((P_T) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_T + P_S + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_R + P_T + ((P_S) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_S + P_R + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_Q + P_S + ((P_R) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_R + P_Q + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_Z + P_R + ((P_Q) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_Q + P_Z + 1) >> 1);
  *pred_pels++ = (sPixel) ((P_Q + P_A + ((P_Z) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_Z + P_B + ((P_A) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_A + P_C + ((P_B) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_B + P_D + ((P_C) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_C + P_E + ((P_D) << 1) + 2) >> 2);
  *pred_pels++ = (sPixel) ((P_D + P_F + ((P_E) << 1) + 2) >> 2);
  *pred_pels   = (sPixel) ((P_E + P_G + ((P_F) << 1) + 2) >> 2);

  pred_pels = &PredArray[14];
  memcpy((*mbPred++) + ioff, pred_pels, 8 * sizeof(sPixel));
  pred_pels -= 2;
  memcpy((*mbPred++) + ioff, pred_pels, 8 * sizeof(sPixel));
  pred_pels -= 2;
  memcpy((*mbPred++) + ioff, pred_pels, 8 * sizeof(sPixel));
  pred_pels -= 2;
  memcpy((*mbPred++) + ioff, pred_pels, 8 * sizeof(sPixel));
  pred_pels -= 2;
  memcpy((*mbPred++) + ioff, pred_pels, 8 * sizeof(sPixel));
  pred_pels -= 2;
  memcpy((*mbPred++) + ioff, pred_pels, 8 * sizeof(sPixel));
  pred_pels -= 2;
  memcpy((*mbPred++) + ioff, pred_pels, 8 * sizeof(sPixel));
  pred_pels -= 2;
  memcpy((*mbPred  ) + ioff, pred_pels, 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra_pred_8x8_normal (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  int blockX = (mb->blockX) + (ioff >> 2);
  int blockY = (mb->blockY) + (joff >> 2);
  uint8_t predmode = mb->slice->predMode[blockY][blockX];

  mb->dpcmMode = predmode;  //For residual DPCM

  switch (predmode)
  {
  case DC_PRED:
    return (intra8x8_dc_pred(mb, plane, ioff, joff));
    break;
  case VERT_PRED:
    return (intra8x8_vert_pred(mb, plane, ioff, joff));
    break;
  case HOR_PRED:
    return (intra8x8_hor_pred(mb, plane, ioff, joff));
    break;
  case DIAG_DOWN_RIGHT_PRED:
    return (intra8x8_diag_down_right_pred(mb, plane, ioff, joff));
    break;
  case DIAG_DOWN_LEFT_PRED:
    return (intra8x8_diag_down_left_pred(mb, plane, ioff, joff));
    break;
  case VERT_RIGHT_PRED:
    return (intra8x8_vert_right_pred(mb, plane, ioff, joff));
    break;
  case VERT_LEFT_PRED:
    return (intra8x8_vert_left_pred(mb, plane, ioff, joff));
    break;
  case HOR_UP_PRED:
    return (intra8x8_hor_up_pred(mb, plane, ioff, joff));
    break;
  case HOR_DOWN_PRED:
    return (intra8x8_hor_down_pred(mb, plane, ioff, joff));
  default:
    printf("Error: illegal intra_8x8 prediction mode: %d\n", (int) predmode);
    return eSearchSync;
    break;
  }
}
//}}}

//{{{
static int intra8x8_dc_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  int i,j;
  int s0 = 0;
  sPixel PredPel[25];  // array of predictor pels
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPicture* picture = slice->picture;
  sPixel** imgY = (plane) ? picture->imgUV[plane - 1] : picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a[8];
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  for (int i=0; i<25;i++) PredPel[i]=0;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (i=0;i<8;i++)
  {
    getAffNeighbour(mb, ioff - 1, joff + i, mbSize, &pix_a[i]);
  }

  getAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
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
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].posY][pix_a[0].posX];
    P_R = imgY[pix_a[1].posY][pix_a[1].posX];
    P_S = imgY[pix_a[2].posY][pix_a[2].posX];
    P_T = imgY[pix_a[3].posY][pix_a[3].posX];
    P_U = imgY[pix_a[4].posY][pix_a[4].posX];
    P_V = imgY[pix_a[5].posY][pix_a[5].posX];
    P_W = imgY[pix_a[6].posY][pix_a[6].posX];
    P_X = imgY[pix_a[7].posY][pix_a[7].posX];
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  if (block_available_up && block_available_left)
    // no edge
    s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 8) >> 4;
  else if (!block_available_up && block_available_left)
    // upper edge
    s0 = (P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 4) >> 3;
  else if (block_available_up && !block_available_left)
    // left edge
    s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + 4) >> 3;
  else //if (!block_available_up && !block_available_left)
    // top left corner, nothing to predict from
    s0 = decoder->coding.dcPredValueComp[plane];

  for(j = joff; j < joff + BLOCK_SIZE_8x8; j++)
    for(i = ioff; i < ioff + BLOCK_SIZE_8x8; i++)
      mpr[j][i] = (sPixel) s0;

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_vert_pred_mbaff (sMacroBlock* mb,
                                     eColorPlane plane,         //!< current image plane
                                     int ioff,              //!< pixel offset X within MB
                                     int joff)              //!< pixel offset Y within MB
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel PredPel[25];  // array of predictor pels
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a[8];
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  for (int i=0; i<25;i++) PredPel[i]=0;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (i=0;i<8;i++)
  {
    getAffNeighbour(mb, ioff - 1, joff + i, mbSize, &pix_a[i]);
  }

  getAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_8x8_Vertical prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8PredHor(&(P_Z), block_available_up_left, block_available_up, block_available_left);

  for (i=joff; i < joff + BLOCK_SIZE_8x8; i++)
    memcpy(&mpr[i][ioff], &PredPel[1], BLOCK_SIZE_8x8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_hor_pred_mbaff (sMacroBlock* mb,
                                    eColorPlane plane,         //!< current image plane
                                    int ioff,              //!< pixel offset X within MB
                                    int joff)              //!< pixel offset Y within MB
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;


  int i,j;
  sPixel PredPel[25];  // array of predictor pels
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a[8];
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;

  for (int i=0; i<25;i++) PredPel[i]=0;

  int jpos;
  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (i=0;i<8;i++)
  {
    getAffNeighbour(mb, ioff - 1, joff + i, mbSize, &pix_a[i]);
  }

  getAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_left)
    printf ("warning: Intra_8x8_Horizontal prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].posY][pix_a[0].posX];
    P_R = imgY[pix_a[1].posY][pix_a[1].posX];
    P_S = imgY[pix_a[2].posY][pix_a[2].posX];
    P_T = imgY[pix_a[3].posY][pix_a[3].posX];
    P_U = imgY[pix_a[4].posY][pix_a[4].posX];
    P_V = imgY[pix_a[5].posY][pix_a[5].posX];
    P_W = imgY[pix_a[6].posY][pix_a[6].posX];
    P_X = imgY[pix_a[7].posY][pix_a[7].posX];
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8PredVer(&(P_Z), block_available_up_left, block_available_up, block_available_left);

  for (j=0; j < BLOCK_SIZE_8x8; j++)
  {
    jpos = j + joff;
    memset(&mpr[jpos][ioff], (sPixel) (&P_Q)[j], 8 * sizeof(sPixel));
  }

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_diag_down_right_pred_mbaff (sMacroBlock* mb,
                                                eColorPlane plane,         //!< current image plane
                                                int ioff,              //!< pixel offset X within MB
                                                int joff)              //!< pixel offset Y within MB
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;


  int i;
  sPixel PredPel[25];  // array of predictor pels
  sPixel PredArray[16];  // array of final prediction values
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a[8];
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(mb, ioff - 1, joff + i, mbSize, &pix_a[i]);
  }

  getAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Diagonal_Down_Right prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
     memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].posY][pix_a[0].posX];
    P_R = imgY[pix_a[1].posY][pix_a[1].posX];
    P_S = imgY[pix_a[2].posY][pix_a[2].posX];
    P_T = imgY[pix_a[3].posY][pix_a[3].posX];
    P_U = imgY[pix_a[4].posY][pix_a[4].posX];
    P_V = imgY[pix_a[5].posY][pix_a[5].posX];
    P_W = imgY[pix_a[6].posY][pix_a[6].posX];
    P_X = imgY[pix_a[7].posY][pix_a[7].posX];
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  // Mode DIAG_DOWN_RIGHT_PRED
  PredArray[ 0] = (sPixel) ((P_X + P_V + 2*(P_W) + 2) >> 2);
  PredArray[ 1] = (sPixel) ((P_W + P_U + 2*(P_V) + 2) >> 2);
  PredArray[ 2] = (sPixel) ((P_V + P_T + 2*(P_U) + 2) >> 2);
  PredArray[ 3] = (sPixel) ((P_U + P_S + 2*(P_T) + 2) >> 2);
  PredArray[ 4] = (sPixel) ((P_T + P_R + 2*(P_S) + 2) >> 2);
  PredArray[ 5] = (sPixel) ((P_S + P_Q + 2*(P_R) + 2) >> 2);
  PredArray[ 6] = (sPixel) ((P_R + P_Z + 2*(P_Q) + 2) >> 2);
  PredArray[ 7] = (sPixel) ((P_Q + P_A + 2*(P_Z) + 2) >> 2);
  PredArray[ 8] = (sPixel) ((P_Z + P_B + 2*(P_A) + 2) >> 2);
  PredArray[ 9] = (sPixel) ((P_A + P_C + 2*(P_B) + 2) >> 2);
  PredArray[10] = (sPixel) ((P_B + P_D + 2*(P_C) + 2) >> 2);
  PredArray[11] = (sPixel) ((P_C + P_E + 2*(P_D) + 2) >> 2);
  PredArray[12] = (sPixel) ((P_D + P_F + 2*(P_E) + 2) >> 2);
  PredArray[13] = (sPixel) ((P_E + P_G + 2*(P_F) + 2) >> 2);
  PredArray[14] = (sPixel) ((P_F + P_H + 2*(P_G) + 2) >> 2);

  memcpy(&mpr[joff++][ioff], &PredArray[7], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[6], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[5], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[4], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[3], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[2], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[1], 8 * sizeof(sPixel));
  memcpy(&mpr[joff  ][ioff], &PredArray[0], 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_diag_down_left_pred_mbaff (sMacroBlock* mb,
                                               eColorPlane plane,         //!< current image plane
                                               int ioff,              //!< pixel offset X within MB
                                               int joff)              //!< pixel offset Y within MB
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel PredPel[25];  // array of predictor pels
  sPixel PredArray[16];  // array of final prediction values
  sPixel *Pred = &PredArray[0];
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a[8];
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(mb, ioff - 1, joff + i, mbSize, &pix_a[i]);
  }

  getAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_8x8_Diagonal_Down_Left prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].posY][pix_a[0].posX];
    P_R = imgY[pix_a[1].posY][pix_a[1].posX];
    P_S = imgY[pix_a[2].posY][pix_a[2].posX];
    P_T = imgY[pix_a[3].posY][pix_a[3].posX];
    P_U = imgY[pix_a[4].posY][pix_a[4].posX];
    P_V = imgY[pix_a[5].posY][pix_a[5].posX];
    P_W = imgY[pix_a[6].posY][pix_a[6].posX];
    P_X = imgY[pix_a[7].posY][pix_a[7].posX];
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  // Mode DIAG_DOWN_LEFT_PRED
  *Pred++ = (sPixel) ((P_A + P_C + 2*(P_B) + 2) >> 2);
  *Pred++ = (sPixel) ((P_B + P_D + 2*(P_C) + 2) >> 2);
  *Pred++ = (sPixel) ((P_C + P_E + 2*(P_D) + 2) >> 2);
  *Pred++ = (sPixel) ((P_D + P_F + 2*(P_E) + 2) >> 2);
  *Pred++ = (sPixel) ((P_E + P_G + 2*(P_F) + 2) >> 2);
  *Pred++ = (sPixel) ((P_F + P_H + 2*(P_G) + 2) >> 2);
  *Pred++ = (sPixel) ((P_G + P_I + 2*(P_H) + 2) >> 2);
  *Pred++ = (sPixel) ((P_H + P_J + 2*(P_I) + 2) >> 2);
  *Pred++ = (sPixel) ((P_I + P_K + 2*(P_J) + 2) >> 2);
  *Pred++ = (sPixel) ((P_J + P_L + 2*(P_K) + 2) >> 2);
  *Pred++ = (sPixel) ((P_K + P_M + 2*(P_L) + 2) >> 2);
  *Pred++ = (sPixel) ((P_L + P_N + 2*(P_M) + 2) >> 2);
  *Pred++ = (sPixel) ((P_M + P_O + 2*(P_N) + 2) >> 2);
  *Pred++ = (sPixel) ((P_N + P_P + 2*(P_O) + 2) >> 2);
  *Pred   = (sPixel) ((P_O + 3*(P_P) + 2) >> 2);

  Pred = &PredArray[ 0];

  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], Pred++, 8 * sizeof(sPixel));
  memcpy(&mpr[joff  ][ioff], Pred  , 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_vert_right_pred_mbaff (sMacroBlock* mb,
                                           eColorPlane plane,         //!< current image plane
                                           int ioff,              //!< pixel offset X within MB
                                           int joff)              //!< pixel offset Y within MB
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel PredPel[25];  // array of predictor pels
  sPixel PredArray[22];  // array of final prediction values
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a[8];
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(mb, ioff - 1, joff + i, mbSize, &pix_a[i]);
  }

  getAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Vertical_Right prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].posY][pix_a[0].posX];
    P_R = imgY[pix_a[1].posY][pix_a[1].posX];
    P_S = imgY[pix_a[2].posY][pix_a[2].posX];
    P_T = imgY[pix_a[3].posY][pix_a[3].posX];
    P_U = imgY[pix_a[4].posY][pix_a[4].posX];
    P_V = imgY[pix_a[5].posY][pix_a[5].posX];
    P_W = imgY[pix_a[6].posY][pix_a[6].posX];
    P_X = imgY[pix_a[7].posY][pix_a[7].posX];
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  PredArray[ 0] = (sPixel) ((P_V + P_T + (P_U << 1) + 2) >> 2);
  PredArray[ 1] = (sPixel) ((P_T + P_R + (P_S << 1) + 2) >> 2);
  PredArray[ 2] = (sPixel) ((P_R + P_Z + (P_Q << 1) + 2) >> 2);
  PredArray[ 3] = (sPixel) ((P_Z + P_A + 1) >> 1);
  PredArray[ 4] = (sPixel) ((P_A + P_B + 1) >> 1);
  PredArray[ 5] = (sPixel) ((P_B + P_C + 1) >> 1);
  PredArray[ 6] = (sPixel) ((P_C + P_D + 1) >> 1);
  PredArray[ 7] = (sPixel) ((P_D + P_E + 1) >> 1);
  PredArray[ 8] = (sPixel) ((P_E + P_F + 1) >> 1);
  PredArray[ 9] = (sPixel) ((P_F + P_G + 1) >> 1);
  PredArray[10] = (sPixel) ((P_G + P_H + 1) >> 1);

  PredArray[11] = (sPixel) ((P_W + P_U + (P_V << 1) + 2) >> 2);
  PredArray[12] = (sPixel) ((P_U + P_S + (P_T << 1) + 2) >> 2);
  PredArray[13] = (sPixel) ((P_S + P_Q + (P_R << 1) + 2) >> 2);
  PredArray[14] = (sPixel) ((P_Q + P_A + 2*P_Z + 2) >> 2);
  PredArray[15] = (sPixel) ((P_Z + P_B + 2*P_A + 2) >> 2);
  PredArray[16] = (sPixel) ((P_A + P_C + 2*P_B + 2) >> 2);
  PredArray[17] = (sPixel) ((P_B + P_D + 2*P_C + 2) >> 2);
  PredArray[18] = (sPixel) ((P_C + P_E + 2*P_D + 2) >> 2);
  PredArray[19] = (sPixel) ((P_D + P_F + 2*P_E + 2) >> 2);
  PredArray[20] = (sPixel) ((P_E + P_G + 2*P_F + 2) >> 2);
  PredArray[21] = (sPixel) ((P_F + P_H + 2*P_G + 2) >> 2);

  memcpy(&mpr[joff++][ioff], &PredArray[ 3], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[14], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 2], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[13], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 1], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[12], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 0], 8 * sizeof(sPixel));
  memcpy(&mpr[joff  ][ioff], &PredArray[11], 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_vert_left_pred_mbaff (sMacroBlock* mb,
                                          eColorPlane plane,         //!< current image plane
                                          int ioff,              //!< pixel offset X within MB
                                          int joff)              //!< pixel offset Y within MB
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel PredPel[25];  // array of predictor pels
  sPixel PredArray[22];  // array of final prediction values
  sPixel *pred_pel = &PredArray[0];
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a[8];
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(mb, ioff - 1, joff + i, mbSize, &pix_a[i]);
  }

  getAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_up)
    printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].posY][pix_a[0].posX];
    P_R = imgY[pix_a[1].posY][pix_a[1].posX];
    P_S = imgY[pix_a[2].posY][pix_a[2].posX];
    P_T = imgY[pix_a[3].posY][pix_a[3].posX];
    P_U = imgY[pix_a[4].posY][pix_a[4].posX];
    P_V = imgY[pix_a[5].posY][pix_a[5].posX];
    P_W = imgY[pix_a[6].posY][pix_a[6].posX];
    P_X = imgY[pix_a[7].posY][pix_a[7].posX];
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  *pred_pel++ = (sPixel) ((P_A + P_B + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_B + P_C + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_C + P_D + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_D + P_E + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_E + P_F + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_F + P_G + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_G + P_H + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_H + P_I + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_I + P_J + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_J + P_K + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_K + P_L + 1) >> 1);
  *pred_pel++ = (sPixel) ((P_A + P_C + 2*P_B + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_B + P_D + 2*P_C + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_C + P_E + 2*P_D + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_D + P_F + 2*P_E + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_E + P_G + 2*P_F + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_F + P_H + 2*P_G + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_G + P_I + 2*P_H + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_H + P_J + (P_I << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_I + P_K + (P_J << 1) + 2) >> 2);
  *pred_pel++ = (sPixel) ((P_J + P_L + (P_K << 1) + 2) >> 2);
  *pred_pel   = (sPixel) ((P_K + P_M + (P_L << 1) + 2) >> 2);

  memcpy(&mpr[joff++][ioff], &PredArray[ 0], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[11], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 1], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[12], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 2], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[13], 8 * sizeof(sPixel));
  memcpy(&mpr[joff++][ioff], &PredArray[ 3], 8 * sizeof(sPixel));
  memcpy(&mpr[joff  ][ioff], &PredArray[14], 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_hor_up_pred_mbaff (sMacroBlock* mb,
                                       eColorPlane plane,         //!< current image plane
                                       int ioff,              //!< pixel offset X within MB
                                       int joff)              //!< pixel offset Y within MB
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel PredPel[25];  // array of predictor pels
  sPixel PredArray[22];   // array of final prediction values
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a[8];
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;
  int jpos0 = joff    , jpos1 = joff + 1, jpos2 = joff + 2, jpos3 = joff + 3;
  int jpos4 = joff + 4, jpos5 = joff + 5, jpos6 = joff + 6, jpos7 = joff + 7;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
  {
    getAffNeighbour(mb, ioff - 1, joff + i, mbSize, &pix_a[i]);
  }

  getAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if (!block_available_left)
    printf ("warning: Intra_8x8_Horizontal_Up prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].posY][pix_a[0].posX];
    P_R = imgY[pix_a[1].posY][pix_a[1].posX];
    P_S = imgY[pix_a[2].posY][pix_a[2].posX];
    P_T = imgY[pix_a[3].posY][pix_a[3].posX];
    P_U = imgY[pix_a[4].posY][pix_a[4].posX];
    P_V = imgY[pix_a[5].posY][pix_a[5].posX];
    P_W = imgY[pix_a[6].posY][pix_a[6].posX];
    P_X = imgY[pix_a[7].posY][pix_a[7].posX];
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  PredArray[ 0] = (sPixel) ((P_Q + P_R + 1) >> 1);
  PredArray[ 1] = (sPixel) ((P_S + P_Q + (P_R << 1) + 2) >> 2);
  PredArray[ 2] = (sPixel) ((P_R + P_S + 1) >> 1);
  PredArray[ 3] = (sPixel) ((P_T + P_R + (P_S << 1) + 2) >> 2);
  PredArray[ 4] = (sPixel) ((P_S + P_T + 1) >> 1);
  PredArray[ 5] = (sPixel) ((P_U + P_S + (P_T << 1) + 2) >> 2);
  PredArray[ 6] = (sPixel) ((P_T + P_U + 1) >> 1);
  PredArray[ 7] = (sPixel) ((P_V + P_T + (P_U << 1) + 2) >> 2);
  PredArray[ 8] = (sPixel) ((P_U + P_V + 1) >> 1);
  PredArray[ 9] = (sPixel) ((P_W + P_U + (P_V << 1) + 2) >> 2);
  PredArray[10] = (sPixel) ((P_V + P_W + 1) >> 1);
  PredArray[11] = (sPixel) ((P_X + P_V + (P_W << 1) + 2) >> 2);
  PredArray[12] = (sPixel) ((P_W + P_X + 1) >> 1);
  PredArray[13] = (sPixel) ((P_W + P_X + (P_X << 1) + 2) >> 2);
  PredArray[14] = (sPixel) P_X;
  PredArray[15] = (sPixel) P_X;
  PredArray[16] = (sPixel) P_X;
  PredArray[17] = (sPixel) P_X;
  PredArray[18] = (sPixel) P_X;
  PredArray[19] = (sPixel) P_X;
  PredArray[20] = (sPixel) P_X;
  PredArray[21] = (sPixel) P_X;

  memcpy(&mpr[jpos0][ioff], &PredArray[0], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos1][ioff], &PredArray[2], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos2][ioff], &PredArray[4], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos3][ioff], &PredArray[6], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos4][ioff], &PredArray[8], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos5][ioff], &PredArray[10], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos6][ioff], &PredArray[12], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos7][ioff], &PredArray[14], 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra8x8_hor_down_pred_mbaff (sMacroBlock* mb,
                                         eColorPlane plane,         //!< current image plane
                                         int ioff,              //!< pixel offset X within MB
                                         int joff)              //!< pixel offset Y within MB
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel PredPel[25];  // array of predictor pels
  sPixel PredArray[22];   // array of final prediction values
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY; // For MB level frame/field coding tools -- set default to imgY

  sPixelPos pix_a[8];
  sPixelPos pix_b, pix_c, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;
  int block_available_up_right;
  int jpos0 = joff    , jpos1 = joff + 1, jpos2 = joff + 2, jpos3 = joff + 3;
  int jpos4 = joff + 4, jpos5 = joff + 5, jpos6 = joff + 6, jpos7 = joff + 7;

  sPixel** mpr = slice->mbPred[plane];
  int *mbSize = decoder->mbSize[eLuma];

  for (int i=0; i<25;i++) PredPel[i]=0;

  for (i=0;i<8;i++)
    getAffNeighbour(mb, ioff - 1, joff + i, mbSize, &pix_a[i]);

  getAffNeighbour(mb, ioff    , joff - 1, mbSize, &pix_b);
  getAffNeighbour(mb, ioff + 8, joff - 1, mbSize, &pix_c);
  getAffNeighbour(mb, ioff - 1, joff - 1, mbSize, &pix_d);

  pix_c.available = pix_c.available &&!(ioff == 8 && joff == 8);

  if (decoder->activePps->hasConstrainedIntraPred)
  {
    for (i=0, block_available_left=1; i<8;i++)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
  }
  else
  {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    block_available_up_left  = pix_d.available;
  }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_8x8_Horizontal_Down prediction mode not allowed at mb %d\n", (int) slice->mbIndex);

  // form predictor pels
  if (block_available_up)
    memcpy(&PredPel[1], &imgY[pix_b.posY][pix_b.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[1], decoder->coding.dcPredValueComp[plane], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_up_right)
    memcpy(&PredPel[9], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE_8x8 * sizeof(sPixel));
  else
    memset(&PredPel[9], PredPel[8], BLOCK_SIZE_8x8 * sizeof(sPixel));

  if (block_available_left)
  {
    P_Q = imgY[pix_a[0].posY][pix_a[0].posX];
    P_R = imgY[pix_a[1].posY][pix_a[1].posX];
    P_S = imgY[pix_a[2].posY][pix_a[2].posX];
    P_T = imgY[pix_a[3].posY][pix_a[3].posX];
    P_U = imgY[pix_a[4].posY][pix_a[4].posX];
    P_V = imgY[pix_a[5].posY][pix_a[5].posX];
    P_W = imgY[pix_a[6].posY][pix_a[6].posX];
    P_X = imgY[pix_a[7].posY][pix_a[7].posX];
  }
  else
    P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = (sPixel) decoder->coding.dcPredValueComp[plane];

  if (block_available_up_left)
    P_Z = imgY[pix_d.posY][pix_d.posX];
  else
    P_Z = (sPixel) decoder->coding.dcPredValueComp[plane];

  LowPassForIntra8x8Pred(PredPel, block_available_up_left, block_available_up, block_available_left);

  PredArray[ 0] = (sPixel) ((P_X + P_W + 1) >> 1);
  PredArray[ 1] = (sPixel) ((P_V + P_X + (P_W << 1) + 2) >> 2);
  PredArray[ 2] = (sPixel) ((P_W + P_V + 1) >> 1);
  PredArray[ 3] = (sPixel) ((P_U + P_W + (P_V << 1) + 2) >> 2);
  PredArray[ 4] = (sPixel) ((P_V + P_U + 1) >> 1);
  PredArray[ 5] = (sPixel) ((P_T + P_V + (P_U << 1) + 2) >> 2);
  PredArray[ 6] = (sPixel) ((P_U + P_T + 1) >> 1);
  PredArray[ 7] = (sPixel) ((P_S + P_U + (P_T << 1) + 2) >> 2);
  PredArray[ 8] = (sPixel) ((P_T + P_S + 1) >> 1);
  PredArray[ 9] = (sPixel) ((P_R + P_T + (P_S << 1) + 2) >> 2);
  PredArray[10] = (sPixel) ((P_S + P_R + 1) >> 1);
  PredArray[11] = (sPixel) ((P_Q + P_S + (P_R << 1) + 2) >> 2);
  PredArray[12] = (sPixel) ((P_R + P_Q + 1) >> 1);
  PredArray[13] = (sPixel) ((P_Z + P_R + (P_Q << 1) + 2) >> 2);
  PredArray[14] = (sPixel) ((P_Q + P_Z + 1) >> 1);
  PredArray[15] = (sPixel) ((P_Q + P_A + 2*P_Z + 2) >> 2);
  PredArray[16] = (sPixel) ((P_Z + P_B + 2*P_A + 2) >> 2);
  PredArray[17] = (sPixel) ((P_A + P_C + 2*P_B + 2) >> 2);
  PredArray[18] = (sPixel) ((P_B + P_D + 2*P_C + 2) >> 2);
  PredArray[19] = (sPixel) ((P_C + P_E + 2*P_D + 2) >> 2);
  PredArray[20] = (sPixel) ((P_D + P_F + 2*P_E + 2) >> 2);
  PredArray[21] = (sPixel) ((P_E + P_G + 2*P_F + 2) >> 2);

  memcpy(&mpr[jpos0][ioff], &PredArray[14], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos1][ioff], &PredArray[12], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos2][ioff], &PredArray[10], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos3][ioff], &PredArray[ 8], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos4][ioff], &PredArray[ 6], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos5][ioff], &PredArray[ 4], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos6][ioff], &PredArray[ 2], 8 * sizeof(sPixel));
  memcpy(&mpr[jpos7][ioff], &PredArray[ 0], 8 * sizeof(sPixel));

  return eDecodingOk;
}
//}}}
//{{{
static int intra_pred_8x8_mbaff (sMacroBlock* mb,
                   eColorPlane plane,         //!< Current color plane
                   int ioff,              //!< ioff
                   int joff)              //!< joff

{
  int blockX = (mb->blockX) + (ioff >> 2);
  int blockY = (mb->blockY) + (joff >> 2);
  uint8_t predmode = mb->slice->predMode[blockY][blockX];

  mb->dpcmMode = predmode;  //For residual DPCM

  switch (predmode) {
  case DC_PRED:
    return (intra8x8_dc_pred_mbaff(mb, plane, ioff, joff));
    break;
  case VERT_PRED:
    return (intra8x8_vert_pred_mbaff(mb, plane, ioff, joff));
    break;
  case HOR_PRED:
    return (intra8x8_hor_pred_mbaff(mb, plane, ioff, joff));
    break;
  case DIAG_DOWN_RIGHT_PRED:
    return (intra8x8_diag_down_right_pred_mbaff(mb, plane, ioff, joff));
    break;
  case DIAG_DOWN_LEFT_PRED:
    return (intra8x8_diag_down_left_pred_mbaff(mb, plane, ioff, joff));
    break;
  case VERT_RIGHT_PRED:
    return (intra8x8_vert_right_pred_mbaff(mb, plane, ioff, joff));
    break;
  case VERT_LEFT_PRED:
    return (intra8x8_vert_left_pred_mbaff(mb, plane, ioff, joff));
    break;
  case HOR_UP_PRED:
    return (intra8x8_hor_up_pred_mbaff(mb, plane, ioff, joff));
    break;
  case HOR_DOWN_PRED:
    return (intra8x8_hor_down_pred_mbaff(mb, plane, ioff, joff));
  default:
    printf("Error: illegal intra_8x8 prediction mode: %d\n", (int) predmode);
    return eSearchSync;
    break;
  }
}
//}}}

//{{{
static int intra16x16_dc_pred (sMacroBlock* mb, eColorPlane plane)
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int s0 = 0, s1 = 0, s2 = 0;

  int i,j;

  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
  sPixel** mbPred = &(slice->mbPred[plane][0]);

  sPixelPos a, b;

  int up_avail, left_avail;

  getNonAffNeighbour(mb,   -1,   0, decoder->mbSize[eLuma], &a);
  getNonAffNeighbour(mb,    0,  -1, decoder->mbSize[eLuma], &b);

  if (!decoder->activePps->hasConstrainedIntraPred)
  {
    up_avail      = b.available;
    left_avail    = a.available;
  }
  else
  {
    up_avail      = b.available ? slice->intraBlock[b.mbIndex] : 0;
    left_avail    = a.available ? slice->intraBlock[a.mbIndex]: 0;
  }

  // Sum top predictors
  if (up_avail)
  {
    sPixel *pel = &imgY[b.posY][b.posX];
    for (i = 0; i < MB_BLOCK_SIZE; ++i)
      s1 += *pel++;
  }

  // Sum left predictors
  if (left_avail)
  {
    int posY = a.posY;
    int posX = a.posX;
    for (i = 0; i < MB_BLOCK_SIZE; ++i)
      s2 += imgY[posY++][posX];
  }

  if (up_avail && left_avail)
    s0 = (s1 + s2 + 16)>>5;       // no edge
  else if (!up_avail && left_avail)
    s0 = (s2 + 8)>>4;              // upper edge
  else if (up_avail && !left_avail)
    s0 = (s1 + 8)>>4;              // left edge
  else
    s0 = decoder->coding.dcPredValueComp[plane];                            // top left corner, nothing to predict from

  for(j = 0; j < MB_BLOCK_SIZE; ++j)
    memset(mbPred[j], s0, MB_BLOCK_SIZE * sizeof(sPixel));

  return eDecodingOk;

}
//}}}
//{{{
static int intra16x16_vert_pred (sMacroBlock* mb, eColorPlane plane)
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int j;

  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;

  sPixelPos b;          //!< pixel position p(0,-1)

  int up_avail;

  getNonAffNeighbour(mb,    0,   -1, decoder->mbSize[eLuma], &b);

  if (!decoder->activePps->hasConstrainedIntraPred)
    up_avail = b.available;
  else
    up_avail = b.available ? slice->intraBlock[b.mbIndex] : 0;

  if (!up_avail)
    error ("invalid 16x16 intra pred Mode VERT_PRED_16");
  {
    sPixel** prd = &slice->mbPred[plane][0];
    sPixel *src = &(imgY[b.posY][b.posX]);

    for(j=0;j<MB_BLOCK_SIZE; j+= 4)
    {
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(sPixel));
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(sPixel));
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(sPixel));
      memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(sPixel));
    }
  }

  return eDecodingOk;
}
//}}}
//{{{
static int intra16x16_hor_pred (sMacroBlock* mb, eColorPlane plane)
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  int j;

  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
  sPixel** mbPred = &(slice->mbPred[plane][0]);
  sPixel prediction;
  int posY, posX;

  sPixelPos a;

  int left_avail;

  getNonAffNeighbour(mb, -1,  0, decoder->mbSize[eLuma], &a);

  if (!decoder->activePps->hasConstrainedIntraPred)
    left_avail    = a.available;
  else
    left_avail  = a.available ? slice->intraBlock[a.mbIndex]: 0;

  if (!left_avail)
    error ("invalid 16x16 intra pred Mode HOR_PRED_16");

  posY = a.posY;
  posX = a.posX;

  for(j = 0; j < MB_BLOCK_SIZE; ++j)
  {
    sPixel *prd = mbPred[j];
    prediction = imgY[posY++][posX];
    memset(prd, prediction, MB_BLOCK_SIZE * sizeof(sPixel));
  }

  return eDecodingOk;
}
//}}}
//{{{
static int intra16x16_plane_pred (sMacroBlock* mb, eColorPlane plane)
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i,j;

  int ih = 0, iv = 0;
  int ib,ic,iaa;

  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
  sPixel** mbPred = &(slice->mbPred[plane][0]);
  sPixel *mpr_line;
  int max_imgpel_value = decoder->coding.maxPelValueComp[plane];
  int posY, posX;

  sPixelPos a, b, d;

  int up_avail, left_avail, left_up_avail;

  getNonAffNeighbour(mb, -1,  -1, decoder->mbSize[eLuma], &d);
  getNonAffNeighbour(mb, -1,   0, decoder->mbSize[eLuma], &a);
  getNonAffNeighbour(mb,  0,  -1, decoder->mbSize[eLuma], &b);

  if (!decoder->activePps->hasConstrainedIntraPred)
  {
    up_avail      = b.available;
    left_avail    = a.available;
    left_up_avail = d.available;
  }
  else
  {
    up_avail      = b.available ? slice->intraBlock[b.mbIndex] : 0;
    left_avail    = a.available ? slice->intraBlock[a.mbIndex] : 0;
    left_up_avail = d.available ? slice->intraBlock[d.mbIndex] : 0;
  }

  if (!up_avail || !left_up_avail  || !left_avail)
    error ("invalid 16x16 intra pred Mode PLANE_16");

  mpr_line = &imgY[b.posY][b.posX+7];
  posY = a.posY + 7;
  posX = a.posX;
  for (i = 1; i < 8; ++i)
  {
    ih += i*(mpr_line[i] - mpr_line[-i]);
    iv += i*(imgY[posY + i][posX] - imgY[posY - i][posX]);
  }

  ih += 8*(mpr_line[8] - imgY[d.posY][d.posX]);
  iv += 8*(imgY[posY + 8][posX] - imgY[d.posY][d.posX]);

  ib=(5 * ih + 32)>>6;
  ic=(5 * iv + 32)>>6;

  iaa=16 * (mpr_line[8] + imgY[posY + 8][posX]);
  for (j = 0;j < MB_BLOCK_SIZE; ++j)
  {
    int ibb = iaa + (j - 7) * ic + 16;
    sPixel *prd = mbPred[j];
    for (i = 0;i < MB_BLOCK_SIZE; i += 4)
    {
      *prd++ = (sPixel) iClip1(max_imgpel_value, ((ibb + (i - 7) * ib) >> 5));
      *prd++ = (sPixel) iClip1(max_imgpel_value, ((ibb + (i - 6) * ib) >> 5));
      *prd++ = (sPixel) iClip1(max_imgpel_value, ((ibb + (i - 5) * ib) >> 5));
      *prd++ = (sPixel) iClip1(max_imgpel_value, ((ibb + (i - 4) * ib) >> 5));
    }
  }// store plane prediction

  return eDecodingOk;
}
//}}}
//{{{
static int intra_pred_16x16_normal (sMacroBlock* mb, eColorPlane plane, int predmode) {

  switch (predmode) {
    case VERT_PRED_16:                       // vertical prediction from block above
      return (intra16x16_vert_pred(mb, plane));
      break;
    case HOR_PRED_16:                        // horizontal prediction from left block
      return (intra16x16_hor_pred(mb, plane));
      break;
    case DC_PRED_16:                         // DC prediction
      return (intra16x16_dc_pred(mb, plane));
      break;
    case PLANE_16:// 16 bit integer plan pred
      return (intra16x16_plane_pred(mb, plane));
      break;
    default:
      {                                    // indication of fault in s,exit
        printf("illegal 16x16 intra prediction mode input: %d\n",predmode);
        return eSearchSync;
      }
    }
  }
//}}}

//{{{
static int intra16x16_dc_pred_mbaff (sMacroBlock* mb, eColorPlane plane) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int s0 = 0, s1 = 0, s2 = 0;

  int i,j;

  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
  sPixel** mbPred = &(slice->mbPred[plane][0]);

  sPixelPos b;        // pixel position p(0,-1)
  sPixelPos left[17]; // pixel positions p(-1, -1..15)

  int up_avail, left_avail;

  s1 = s2 = 0;

  for (i = 0; i < 17;++i)
    getAffNeighbour (mb, -1,  i-1, decoder->mbSize[eLuma], &left[i]);
  getAffNeighbour (mb,    0,   -1, decoder->mbSize[eLuma], &b);

  if (!decoder->activePps->hasConstrainedIntraPred) {
    up_avail      = b.available;
    left_avail    = left[1].available;
    }
  else {
    up_avail      = b.available ? slice->intraBlock[b.mbIndex] : 0;
    for (i = 1, left_avail = 1; i < 17; ++i)
      left_avail  &= left[i].available ? slice->intraBlock[left[i].mbIndex]: 0;
    }

  for (i = 0; i < MB_BLOCK_SIZE; ++i) {
    if (up_avail)
      s1 += imgY[b.posY][b.posX+i];    // sum hor pixelPos
    if (left_avail)
      s2 += imgY[left[i + 1].posY][left[i + 1].posX];    // sum vert pixelPos
    }

  if (up_avail && left_avail)
    s0 = (s1 + s2 + 16)>>5;       // no edge
  else if (!up_avail && left_avail)
    s0 = (s2 + 8)>>4;              // upper edge
  else if (up_avail && !left_avail)
    s0 = (s1 + 8)>>4;              // left edge
  else // top left corner, nothing to predict from
    s0 = decoder->coding.dcPredValueComp[plane];

  for (j = 0; j < MB_BLOCK_SIZE; ++j)
    memset (mbPred[j], s0, MB_BLOCK_SIZE * sizeof(sPixel));

  return eDecodingOk;
  }
//}}}
//{{{
static int intra16x16_vert_pred_mbaff (sMacroBlock* mb, eColorPlane plane) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int j;
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
  sPixelPos b;          //!< pixel position p(0,-1)
  int up_avail;

  getAffNeighbour (mb,    0,   -1, decoder->mbSize[eLuma], &b);

  if (!decoder->activePps->hasConstrainedIntraPred)
    up_avail = b.available;
  else
    up_avail = b.available ? slice->intraBlock[b.mbIndex] : 0;

  if  (!up_avail)
    error ("invalid 16x16 intra pred Mode VERT_PRED_16");

  sPixel** prd = &slice->mbPred[plane][0];
  sPixel *src = &(imgY[b.posY][b.posX]);

  for (j = 0; j < MB_BLOCK_SIZE; j+= 4) {
    memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(sPixel));
    memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(sPixel));
    memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(sPixel));
    memcpy(*prd++, src, MB_BLOCK_SIZE * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra16x16_hor_pred_mbaff (sMacroBlock* mb, eColorPlane plane) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  int i,j;

  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
  sPixel** mbPred = &(slice->mbPred[plane][0]);
  sPixel prediction;

  sPixelPos left[17];    //!< pixel positions p(-1, -1..15)

  int left_avail;

  for (i = 0; i < 17; ++i)
    getAffNeighbour (mb, -1,  i-1, decoder->mbSize[eLuma], &left[i]);

  if (!decoder->activePps->hasConstrainedIntraPred)
    left_avail    = left[1].available;
  else
    for (i = 1, left_avail = 1; i < 17; ++i)
      left_avail  &= left[i].available ? slice->intraBlock[left[i].mbIndex]: 0;

  if (!left_avail)
    error ("invalid 16x16 intra pred Mode HOR_PRED_16");

  for (j = 0; j < MB_BLOCK_SIZE; ++j) {
    prediction = imgY[left[j+1].posY][left[j+1].posX];
    for(i = 0; i < MB_BLOCK_SIZE; ++i)
      mbPred[j][i]= prediction; // store predicted 16x16 block
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra16x16_plane_pred_mbaff (sMacroBlock* mb, eColorPlane plane) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i,j;

  int ih = 0, iv = 0;
  int ib,ic,iaa;

  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
  sPixel** mbPred = &(slice->mbPred[plane][0]);
  sPixel *mpr_line;
  int max_imgpel_value = decoder->coding.maxPelValueComp[plane];

  sPixelPos b;          //!< pixel position p(0,-1)
  sPixelPos left[17];    //!< pixel positions p(-1, -1..15)

  int up_avail, left_avail, left_up_avail;

  for (i = 0; i < 17; ++i)
    getAffNeighbour (mb, -1,  i-1, decoder->mbSize[eLuma], &left[i]);
  getAffNeighbour (mb,    0,   -1, decoder->mbSize[eLuma], &b);

  if (!decoder->activePps->hasConstrainedIntraPred) {
    up_avail = b.available;
    left_avail = left[1].available;
    left_up_avail = left[0].available;
    }
  else {
    up_avail = b.available ? slice->intraBlock[b.mbIndex] : 0;
    for (i = 1, left_avail = 1; i < 17; ++i)
      left_avail  &= left[i].available ? slice->intraBlock[left[i].mbIndex]: 0;
    left_up_avail = left[0].available ? slice->intraBlock[left[0].mbIndex]: 0;
    }

  if (!up_avail || !left_up_avail  || !left_avail)
    error ("invalid 16x16 intra pred Mode PLANE_16");

  mpr_line = &imgY[b.posY][b.posX+7];
  for (i = 1; i < 8; ++i) {
    ih += i * (mpr_line[i] - mpr_line[-i]);
    iv += i * (imgY[left[8+i].posY][left[8+i].posX] - imgY[left[8-i].posY][left[8-i].posX]);
    }

  ih += 8 * (mpr_line[8] - imgY[left[0].posY][left[0].posX]);
  iv += 8 * (imgY[left[16].posY][left[16].posX] - imgY[left[0].posY][left[0].posX]);

  ib = (5 * ih + 32) >> 6;
  ic = (5 * iv + 32) >> 6;

  iaa = 16 * (mpr_line[8] + imgY[left[16].posY][left[16].posX]);
  for (j = 0;j < MB_BLOCK_SIZE; ++j)
    for (i = 0; i < MB_BLOCK_SIZE; ++i)
      mbPred[j][i] = (sPixel) iClip1(max_imgpel_value, ((iaa + (i - 7) * ib + (j - 7) * ic + 16) >> 5));

  return eDecodingOk;
  }
//}}}
//{{{
static int intra_pred_16x16_mbaff (sMacroBlock* mb, eColorPlane plane, int predmode) {

  switch (predmode) {
    case VERT_PRED_16:                       // vertical prediction from block above
      return (intra16x16_vert_pred_mbaff(mb, plane));
      break;
    case HOR_PRED_16:                        // horizontal prediction from left block
      return (intra16x16_hor_pred_mbaff(mb, plane));
      break;
    case DC_PRED_16:                         // DC prediction
      return (intra16x16_dc_pred_mbaff(mb, plane));
      break;
    case PLANE_16:// 16 bit integer plan pred
      return (intra16x16_plane_pred_mbaff(mb, plane));
      break;
    default:
      {                                    // indication of fault in s,exit
        printf("illegal 16x16 intra prediction mode input: %d\n",predmode);
        return eSearchSync;
      }
    }
  }
//}}}

//{{{
static void intra_chroma_DC_single (sPixel** curr_img, int up_avail, int left_avail, sPixelPos up, sPixelPos left, int blk_x, int blk_y, int *pred, int direction )
{
  int i;
  int s0 = 0;

  if ((direction && up_avail) || (!left_avail && up_avail))
  {
    sPixel *cur_pel = &curr_img[up.posY][up.posX + blk_x];
    for (i = 0; i < 4;++i)
      s0 += *(cur_pel++);
    *pred = (s0 + 2) >> 2;
  }
  else if (left_avail)
  {
    sPixel** cur_pel = &(curr_img[left.posY + blk_y - 1]);
    int posX = left.posX;
    for (i = 0; i < 4;++i)
      s0 += *((*cur_pel++) + posX);
    *pred = (s0 + 2) >> 2;
  }
}
//}}}
//{{{
static void intra_chroma_DC_all (sPixel** curr_img, int up_avail, int left_avail, sPixelPos up, sPixelPos left, int blk_x, int blk_y, int *pred )
{
  int i;
  int s0 = 0, s1 = 0;

  if (up_avail)
  {
    sPixel *cur_pel = &curr_img[up.posY][up.posX + blk_x];
    for (i = 0; i < 4;++i)
      s0 += *(cur_pel++);
  }

  if (left_avail)
  {
    sPixel** cur_pel = &(curr_img[left.posY + blk_y - 1]);
    int posX = left.posX;
    for (i = 0; i < 4;++i)
      s1 += *((*cur_pel++) + posX);
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
static void intrapred_chroma_dc (sMacroBlock* mb)
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  sPicture* picture = slice->picture;
  int        b8, b4;
  int        yuv = picture->chromaFormatIdc - 1;
  int        blk_x, blk_y;
  int        pred, pred1;
  static const int block_pos[3][4][4]= //[yuv][b8][b4]
  {
    { {0, 1, 2, 3},{0, 0, 0, 0},{0, 0, 0, 0},{0, 0, 0, 0}},
    { {0, 1, 2, 3},{2, 3, 2, 3},{0, 0, 0, 0},{0, 0, 0, 0}},
    { {0, 1, 2, 3},{1, 1, 3, 3},{2, 3, 2, 3},{3, 3, 3, 3}}
  };

  sPixelPos up;        //!< pixel position  p(0,-1)
  sPixelPos left;      //!< pixel positions p(-1, -1..16)
  int up_avail, left_avail;
  sPixel** imgUV0 = picture->imgUV[0];
  sPixel** imgUV1 = picture->imgUV[1];
  sPixel** mb_pred0 = slice->mbPred[0 + 1];
  sPixel** mb_pred1 = slice->mbPred[1 + 1];


  getNonAffNeighbour(mb, -1,  0, decoder->mbSize[eChroma], &left);
  getNonAffNeighbour(mb,  0, -1, decoder->mbSize[eChroma], &up);

  if (!decoder->activePps->hasConstrainedIntraPred)
  {
    up_avail      = up.available;
    left_avail    = left.available;
  }
  else
  {
    up_avail = up.available ? slice->intraBlock[up.mbIndex] : 0;
    left_avail = left.available ? slice->intraBlock[left.mbIndex]: 0;
  }

  // DC prediction
  // Note that unlike what is stated in many presentations and papers, this mode does not operate
  // the same way as I_16x16 DC prediction.
  for(b8 = 0; b8 < (decoder->coding.numUvBlocks) ;++b8)
  {
    for (b4 = 0; b4 < 4; ++b4)
    {
      blk_y = subblk_offset_y[yuv][b8][b4];
      blk_x = subblk_offset_x[yuv][b8][b4];
      pred  = decoder->coding.dcPredValueComp[1];
      pred1 = decoder->coding.dcPredValueComp[2];
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

      {
        int jj;
        for (jj = blk_y; jj < blk_y + BLOCK_SIZE; ++jj)
        {
          memset(&mb_pred0[jj][blk_x],  pred, BLOCK_SIZE * sizeof(sPixel));
          memset(&mb_pred1[jj][blk_x], pred1, BLOCK_SIZE * sizeof(sPixel));
        }
      }
    }
  }
}
//}}}
//{{{
static void intrapred_chroma_hor (sMacroBlock* mb)
{
  sDecoder* decoder = mb->decoder;
  sPixelPos a;  //!< pixel positions p(-1, -1..16)
  int left_avail;

  getNonAffNeighbour(mb, -1, 0, decoder->mbSize[eChroma], &a);

  if (!decoder->activePps->hasConstrainedIntraPred)
    left_avail = a.available;
  else
    left_avail = a.available ? mb->slice->intraBlock[a.mbIndex]: 0;
  // Horizontal Prediction
  if (!left_avail )
    error("unexpected HOR_PRED_8 chroma intra prediction mode");
  else
  {
    sSlice *slice = mb->slice;
    int cr_MB_x = decoder->mbCrSizeX;
    int cr_MB_y = decoder->mbCrSizeY;

    int j;
    sPicture* picture = slice->picture;
    int posY = a.posY;
    int posX = a.posX;
    sPixel** mb_pred0 = slice->mbPred[0 + 1];
    sPixel** mb_pred1 = slice->mbPred[1 + 1];
    sPixel** i0 = &picture->imgUV[0][posY];
    sPixel** i1 = &picture->imgUV[1][posY];

    for (j = 0; j < cr_MB_y; ++j)
    {
      memset(mb_pred0[j], (*i0++)[posX], cr_MB_x * sizeof(sPixel));
      memset(mb_pred1[j], (*i1++)[posX], cr_MB_x * sizeof(sPixel));
    }
  }
}
//}}}
//{{{
static void intrapred_chroma_ver (sMacroBlock* mb)
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  int j;
  sPicture* picture = slice->picture;

  sPixelPos up;        //!< pixel position  p(0,-1)
  int up_avail;
  int cr_MB_x = decoder->mbCrSizeX;
  int cr_MB_y = decoder->mbCrSizeY;
  getNonAffNeighbour(mb, 0, -1, decoder->mbSize[eChroma], &up);

  if (!decoder->activePps->hasConstrainedIntraPred)
    up_avail      = up.available;
  else
    up_avail = up.available ? slice->intraBlock[up.mbIndex] : 0;
  // Vertical Prediction
  if (!up_avail)
    error("unexpected VERT_PRED_8 chroma intra prediction mode");
  else
  {
    sPixel** mb_pred0 = slice->mbPred[1];
    sPixel** mb_pred1 = slice->mbPred[2];
    sPixel *i0 = &(picture->imgUV[0][up.posY][up.posX]);
    sPixel *i1 = &(picture->imgUV[1][up.posY][up.posX]);

    for (j = 0; j < cr_MB_y; ++j)
    {
      memcpy(mb_pred0[j], i0, cr_MB_x * sizeof(sPixel));
      memcpy(mb_pred1[j], i1, cr_MB_x * sizeof(sPixel));
    }
  }
}
//}}}
//{{{
static void intrapred_chroma_plane (sMacroBlock* mb)
{
  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  sPicture* picture = slice->picture;

  sPixelPos up;        //!< pixel position  p(0,-1)
  sPixelPos up_left;
  sPixelPos left;  //!< pixel positions p(-1, -1..16)
  int up_avail, left_avail, left_up_avail;

  getNonAffNeighbour(mb, -1, -1, decoder->mbSize[eChroma], &up_left);
  getNonAffNeighbour(mb, -1,  0, decoder->mbSize[eChroma], &left);
  getNonAffNeighbour(mb,  0, -1, decoder->mbSize[eChroma], &up);

  if (!decoder->activePps->hasConstrainedIntraPred)
  {
    up_avail      = up.available;
    left_avail    = left.available;
    left_up_avail = up_left.available;
  }
  else
  {
    up_avail      = up.available ? slice->intraBlock[up.mbIndex] : 0;
    left_avail    = left.available ? slice->intraBlock[left.mbIndex]: 0;
    left_up_avail = up_left.available ? slice->intraBlock[up_left.mbIndex]: 0;
  }
  // plane prediction
  if (!left_up_avail || !left_avail || !up_avail)
    error("unexpected PLANE_8 chroma intra prediction mode");
  else
  {
    int cr_MB_x = decoder->mbCrSizeX;
    int cr_MB_y = decoder->mbCrSizeY;
    int cr_MB_y2 = (cr_MB_y >> 1);
    int cr_MB_x2 = (cr_MB_x >> 1);

    int i,j;
    int ih, iv, ib, ic, iaa;
    int uv;
    for (uv = 0; uv < 2; uv++)
    {
      sPixel** imgUV = picture->imgUV[uv];
      sPixel** mbPred = slice->mbPred[uv + 1];
      int max_imgpel_value = decoder->coding.maxPelValueComp[uv + 1];
      sPixel *upPred = &imgUV[up.posY][up.posX];
      int posX  = up_left.posX;
      int pos_y1 = left.posY + cr_MB_y2;
      int pos_y2 = pos_y1 - 2;
      //sPixel** predU1 = &imgUV[pos_y1];
      sPixel** predU2 = &imgUV[pos_y2];
      ih = cr_MB_x2 * (upPred[cr_MB_x - 1] - imgUV[up_left.posY][posX]);

      for (i = 0; i < cr_MB_x2 - 1; ++i)
        ih += (i + 1) * (upPred[cr_MB_x2 + i] - upPred[cr_MB_x2 - 2 - i]);

      iv = cr_MB_y2 * (imgUV[left.posY + cr_MB_y - 1][posX] - imgUV[up_left.posY][posX]);

      for (i = 0; i < cr_MB_y2 - 1; ++i)
      {
        iv += (i + 1)*(*(imgUV[pos_y1++] + posX) - *((*predU2--) + posX));
      }

      ib= ((cr_MB_x == 8 ? 17 : 5) * ih + 2 * cr_MB_x)>>(cr_MB_x == 8 ? 5 : 6);
      ic= ((cr_MB_y == 8 ? 17 : 5) * iv + 2 * cr_MB_y)>>(cr_MB_y == 8 ? 5 : 6);

      iaa = ((imgUV[pos_y1][posX] + upPred[cr_MB_x-1]) << 4);

      for (j = 0; j < cr_MB_y; ++j)
      {
        int plane = iaa + (j - cr_MB_y2 + 1) * ic + 16 - (cr_MB_x2 - 1) * ib;

        for (i = 0; i < cr_MB_x; ++i)
          mbPred[j][i]=(sPixel) iClip1(max_imgpel_value, ((i * ib + plane) >> 5));
      }
    }
  }
}
//}}}
//{{{
static void intra_chroma_DC_single_mbaff (sPixel** curr_img, int up_avail, int left_avail, sPixelPos up, sPixelPos left[17], int blk_x, int blk_y, int *pred, int direction )
{
  int i;
  int s0 = 0;

  if ((direction && up_avail) || (!left_avail && up_avail))
  {
    for (i = blk_x; i < (blk_x + 4);++i)
      s0 += curr_img[up.posY][up.posX + i];
    *pred = (s0 + 2) >> 2;
  }
  else if (left_avail)
  {
    for (i = blk_y; i < (blk_y + 4);++i)
      s0 += curr_img[left[i].posY][left[i].posX];
    *pred = (s0 + 2) >> 2;
  }
}
//}}}
//{{{
static void intra_chroma_DC_all_mbaff (sPixel** curr_img, int up_avail, int left_avail, sPixelPos up, sPixelPos left[17], int blk_x, int blk_y, int *pred )
{
  int i;
  int s0 = 0, s1 = 0;

  if (up_avail)
    for (i = blk_x; i < (blk_x + 4);++i)
      s0 += curr_img[up.posY][up.posX + i];

  if (left_avail)
    for (i = blk_y; i < (blk_y + 4);++i)
      s1 += curr_img[left[i].posY][left[i].posX];

  if (up_avail && left_avail)
    *pred = (s0 + s1 + 4) >> 3;
  else if (up_avail)
    *pred = (s0 + 2) >> 2;
  else if (left_avail)
    *pred = (s1 + 2) >> 2;
}
//}}}
//{{{
static void intra_pred_chroma_mbaff (sMacroBlock* mb) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  int i,j, ii, jj;
  sPicture* picture = slice->picture;

  int ih, iv, ib, ic, iaa;
  int        b8, b4;
  int        yuv = picture->chromaFormatIdc - 1;
  int        blk_x, blk_y;
  int        pred;

  //{{{
  static const int block_pos[3][4][4]= { //[yuv][b8][b4]
    { {0, 1, 4, 5},{0, 0, 0, 0},{0, 0, 0, 0},{0, 0, 0, 0}},
    { {0, 1, 2, 3},{4, 5, 4, 5},{0, 0, 0, 0},{0, 0, 0, 0}},
    { {0, 1, 2, 3},{1, 1, 3, 3},{4, 5, 4, 5},{5, 5, 5, 5}}
  };
  //}}}

  switch (mb->chromaPredMode) {
    //{{{
    case DC_PRED_8:
      {
        sPixelPos up;        //!< pixel position  p(0,-1)
        sPixelPos left[17];  //!< pixel positions p(-1, -1..16)

        int up_avail, left_avail[2];

        int cr_MB_y = decoder->mbCrSizeY;
        int cr_MB_y2 = (cr_MB_y >> 1);

        for (i=0; i < cr_MB_y + 1 ; ++i)
          getAffNeighbour(mb, -1, i-1, decoder->mbSize[eChroma], &left[i]);
        getAffNeighbour(mb, 0, -1, decoder->mbSize[eChroma], &up);

        if (!decoder->activePps->hasConstrainedIntraPred)
        {
          up_avail      = up.available;
          left_avail[0] = left_avail[1] = left[1].available;
        }
        else
        {
          up_avail = up.available ? slice->intraBlock[up.mbIndex] : 0;
          for (i=0, left_avail[0] = 1; i < cr_MB_y2;++i)
            left_avail[0]  &= left[i + 1].available ? slice->intraBlock[left[i + 1].mbIndex]: 0;

          for (i = cr_MB_y2, left_avail[1] = 1; i<cr_MB_y;++i)
            left_avail[1]  &= left[i + 1].available ? slice->intraBlock[left[i + 1].mbIndex]: 0;

        }
        // DC prediction
        // Note that unlike what is stated in many presentations and papers, this mode does not operate
        // the same way as I_16x16 DC prediction.
        {
          int pred1;
          sPixel** imgUV0 = picture->imgUV[0];
          sPixel** imgUV1 = picture->imgUV[1];
          sPixel** mb_pred0 = slice->mbPred[0 + 1];
          sPixel** mb_pred1 = slice->mbPred[1 + 1];
          for(b8 = 0; b8 < (decoder->coding.numUvBlocks) ;++b8)
          {
            for (b4 = 0; b4 < 4; ++b4)
            {
              blk_y = subblk_offset_y[yuv][b8][b4];
              blk_x = subblk_offset_x[yuv][b8][b4];

              pred = decoder->coding.dcPredValueComp[1];
              pred1 = decoder->coding.dcPredValueComp[2];
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
                  mb_pred0[jj][ii]=(sPixel) pred;
                  mb_pred1[jj][ii]=(sPixel) pred1;
                }
              }
            }
          }
        }
      }
      break;
    //}}}
    //{{{
    case HOR_PRED_8:
      {
        sPixelPos left[17];  //!< pixel positions p(-1, -1..16)

        int left_avail[2];

        int cr_MB_x = decoder->mbCrSizeX;
        int cr_MB_y = decoder->mbCrSizeY;
        int cr_MB_y2 = (cr_MB_y >> 1);

        for (i=0; i < cr_MB_y + 1 ; ++i)
          getAffNeighbour(mb, -1, i-1, decoder->mbSize[eChroma], &left[i]);

        if (!decoder->activePps->hasConstrainedIntraPred)
        {
          left_avail[0] = left_avail[1] = left[1].available;
        }
        else
        {
          for (i=0, left_avail[0] = 1; i < cr_MB_y2;++i)
            left_avail[0]  &= left[i + 1].available ? slice->intraBlock[left[i + 1].mbIndex]: 0;

          for (i = cr_MB_y2, left_avail[1] = 1; i<cr_MB_y;++i)
            left_avail[1]  &= left[i + 1].available ? slice->intraBlock[left[i + 1].mbIndex]: 0;
        }
        // Horizontal Prediction
        if (!left_avail[0] || !left_avail[1])
          error("unexpected HOR_PRED_8 chroma intra prediction mode");
        else
        {
          int pred1;
          sPixel** mb_pred0 = slice->mbPred[0 + 1];
          sPixel** mb_pred1 = slice->mbPred[1 + 1];
          sPixel** i0 = picture->imgUV[0];
          sPixel** i1 = picture->imgUV[1];
          for (j = 0; j < cr_MB_y; ++j)
          {
            pred = i0[left[1 + j].posY][left[1 + j].posX];
            pred1 = i1[left[1 + j].posY][left[1 + j].posX];
            for (i = 0; i < cr_MB_x; ++i)
            {
              mb_pred0[j][i]=(sPixel) pred;
              mb_pred1[j][i]=(sPixel) pred1;
            }
          }
        }
      }
      break;
    //}}}
    //{{{
    case VERT_PRED_8:
      {
        sPixelPos up;        //!< pixel position  p(0,-1)

        int up_avail;

        int cr_MB_x = decoder->mbCrSizeX;
        int cr_MB_y = decoder->mbCrSizeY;

        getAffNeighbour(mb, 0, -1, decoder->mbSize[eChroma], &up);

        if (!decoder->activePps->hasConstrainedIntraPred)
          up_avail      = up.available;
        else
          up_avail = up.available ? slice->intraBlock[up.mbIndex] : 0;
        // Vertical Prediction
        if (!up_avail)
          error("unexpected VERT_PRED_8 chroma intra prediction mode");
        else
        {
          sPixel** mb_pred0 = slice->mbPred[0 + 1];
          sPixel** mb_pred1 = slice->mbPred[1 + 1];
          sPixel *i0 = &(picture->imgUV[0][up.posY][up.posX]);
          sPixel *i1 = &(picture->imgUV[1][up.posY][up.posX]);
          for (j = 0; j < cr_MB_y; ++j)
          {
            memcpy(&(mb_pred0[j][0]),i0, cr_MB_x * sizeof(sPixel));
            memcpy(&(mb_pred1[j][0]),i1, cr_MB_x * sizeof(sPixel));
          }
        }
      }
      break;
    //}}}
    //{{{
    case PLANE_8:
      {
        sPixelPos up;        //!< pixel position  p(0,-1)
        sPixelPos left[17];  //!< pixel positions p(-1, -1..16)

        int up_avail, left_avail[2], left_up_avail;

        int cr_MB_x = decoder->mbCrSizeX;
        int cr_MB_y = decoder->mbCrSizeY;
        int cr_MB_y2 = (cr_MB_y >> 1);
        int cr_MB_x2 = (cr_MB_x >> 1);

        for (i=0; i < cr_MB_y + 1 ; ++i)
          getAffNeighbour(mb, -1, i-1, decoder->mbSize[eChroma], &left[i]);
        getAffNeighbour(mb, 0, -1, decoder->mbSize[eChroma], &up);

        if (!decoder->activePps->hasConstrainedIntraPred)
        {
          up_avail      = up.available;
          left_avail[0] = left_avail[1] = left[1].available;
          left_up_avail = left[0].available;
        }
        else
        {
          up_avail = up.available ? slice->intraBlock[up.mbIndex] : 0;
          for (i=0, left_avail[0] = 1; i < cr_MB_y2;++i)
            left_avail[0]  &= left[i + 1].available ? slice->intraBlock[left[i + 1].mbIndex]: 0;

          for (i = cr_MB_y2, left_avail[1] = 1; i<cr_MB_y;++i)
            left_avail[1]  &= left[i + 1].available ? slice->intraBlock[left[i + 1].mbIndex]: 0;

          left_up_avail = left[0].available ? slice->intraBlock[left[0].mbIndex]: 0;
        }
        // plane prediction
        if (!left_up_avail || !left_avail[0] || !left_avail[1] || !up_avail)
          error("unexpected PLANE_8 chroma intra prediction mode");
        else
        {
          int uv;
          for (uv = 0; uv < 2; uv++)
          {
            sPixel** imgUV = picture->imgUV[uv];
            sPixel** mbPred = slice->mbPred[uv + 1];
            int max_imgpel_value = decoder->coding.maxPelValueComp[uv + 1];
            sPixel *upPred = &imgUV[up.posY][up.posX];

            ih = cr_MB_x2 * (upPred[cr_MB_x - 1] - imgUV[left[0].posY][left[0].posX]);
            for (i = 0; i < cr_MB_x2 - 1; ++i)
              ih += (i + 1) * (upPred[cr_MB_x2 + i] - upPred[cr_MB_x2 - 2 - i]);

            iv = cr_MB_y2 * (imgUV[left[cr_MB_y].posY][left[cr_MB_y].posX] - imgUV[left[0].posY][left[0].posX]);
            for (i = 0; i < cr_MB_y2 - 1; ++i)
              iv += (i + 1)*(imgUV[left[cr_MB_y2 + 1 + i].posY][left[cr_MB_y2 + 1 + i].posX] -
              imgUV[left[cr_MB_y2 - 1 - i].posY][left[cr_MB_y2 - 1 - i].posX]);

            ib= ((cr_MB_x == 8 ? 17 : 5) * ih + 2 * cr_MB_x)>>(cr_MB_x == 8 ? 5 : 6);
            ic= ((cr_MB_y == 8 ? 17 : 5) * iv + 2 * cr_MB_y)>>(cr_MB_y == 8 ? 5 : 6);

            iaa=16*(imgUV[left[cr_MB_y].posY][left[cr_MB_y].posX] + upPred[cr_MB_x-1]);

            for (j = 0; j < cr_MB_y; ++j)
              for (i = 0; i < cr_MB_x; ++i)
                mbPred[j][i]=(sPixel) iClip1(max_imgpel_value, ((iaa + (i - cr_MB_x2 + 1) * ib + (j - cr_MB_y2 + 1) * ic + 16) >> 5));
          }
        }
      }
      break;
    //}}}
    //{{{
    default:
      error("illegal chroma intra prediction mode");
      break;
    //}}}
    }
  }
//}}}
//{{{
static void intraPredChroma (sMacroBlock* mb) {

  switch (mb->chromaPredMode) {
    case DC_PRED_8:
      intrapred_chroma_dc(mb);
      break;

    case HOR_PRED_8:
      intrapred_chroma_hor(mb);
      break;

    case VERT_PRED_8:
      intrapred_chroma_ver(mb);
      break;

    case PLANE_8:
      intrapred_chroma_plane(mb);
      break;

    default:
      error("illegal chroma intra prediction mode");
      break;
    }
  }
//}}}

//{{{
void set_intra_prediction_modes (sSlice *slice) {

  if (slice->mbAffFrame) {
    slice->intraPred4x4 = intra_pred_4x4_mbaff;
    slice->intraPred8x8 = intra_pred_8x8_mbaff;
    slice->intraPred16x16 = intra_pred_16x16_mbaff;
    slice->intraPredChroma = intra_pred_chroma_mbaff;
    }
  else {
    slice->intraPred4x4 = intra_pred_4x4_normal;
    slice->intraPred8x8 = intra_pred_8x8_normal;
    slice->intraPred16x16 = intra_pred_16x16_normal;
    slice->intraPredChroma = intraPredChroma;
    }
  }
//}}}