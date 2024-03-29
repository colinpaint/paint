//{{{  includes
#include "global.h"

#include "intraPred.h"
#include "mbAccess.h"
//}}}
//{{{
// Notation for comments regarding prediction and predictors.
// The pels of the 4x4 block are labelled a..p. The predictor pels above
// are labelled A..H, from the left I..L, and from above left X, as follows:
//
//  X A B C D E F G H
//  I a b c d
//  J e f g h
//  K i j k l
//  L m n o p
//

// Predictor array index definitions
#define P_X (PredPel[0])
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
//}}}

//{{{
static int intra4x4_dc_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int j;
  int s0 = 0;
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
  sPixel *curpel = NULL;

  sPixelPos pix_a, pix_b;

  int block_available_up;
  int block_available_left;

  sPixel** mbPred = slice->mbPred[plane];

  getNonAffNeighbour(mb, ioff - 1, joff   , decoder->mbSize[eLuma], &pix_a);
  getNonAffNeighbour(mb, ioff    , joff -1, decoder->mbSize[eLuma], &pix_b);

  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_left = pix_a.available ? slice->intraBlock [pix_a.mbIndex] : 0;
    block_available_up   = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    }
  else {
    block_available_left = pix_a.available;
    block_available_up   = pix_b.available;
    }

  // form predictor pels
  if (block_available_up) {
    curpel = &imgY[pix_b.posY][pix_b.posX];
    s0 += *curpel++;
    s0 += *curpel++;
    s0 += *curpel++;
    s0 += *curpel;
    }

  if (block_available_left) {
    sPixel** img_pred = &imgY[pix_a.posY];
    int posX = pix_a.posX;
    s0 += *(*(img_pred ++) + posX);
    s0 += *(*(img_pred ++) + posX);
    s0 += *(*(img_pred ++) + posX);
    s0 += *(*(img_pred   ) + posX);
    }

  if (block_available_up && block_available_left)
    // no edge
    s0 = (s0 + 4)>>3;
  else if (!block_available_up && block_available_left)
    // upper edge
    s0 = (s0 + 2)>>2;
  else if (block_available_up && !block_available_left)
    // left edge
    s0 = (s0 + 2)>>2;
  else 
    // top left corner, nothing to predict from
    s0 = decoder->coding.dcPredValueComp[plane];

  for (j=joff; j < joff + BLOCK_SIZE; ++j) {
    // store DC prediction
    mbPred[j][ioff    ] = (sPixel) s0;
    mbPred[j][ioff + 1] = (sPixel) s0;
    mbPred[j][ioff + 2] = (sPixel) s0;
    mbPred[j][ioff + 3] = (sPixel) s0;
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_vert_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int block_available_up;
  sPixelPos pix_b;

  getNonAffNeighbour (mb, ioff, joff - 1 , decoder->mbSize[eLuma], &pix_b);

  if (decoder->activePps->hasConstrainedIntraPred)
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
  else
    block_available_up = pix_b.available;

  if (!block_available_up)
    printf ("warning: Intra_4x4_Vertical prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel** mbPred = slice->mbPred[plane];
    sPixel* imgY = plane ? &slice->picture->imgUV[plane - 1][pix_b.posY][pix_b.posX] : &slice->picture->imgY[pix_b.posY][pix_b.posX];
    memcpy (&mbPred[joff++][ioff], imgY, BLOCK_SIZE * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], imgY, BLOCK_SIZE * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], imgY, BLOCK_SIZE * sizeof(sPixel));
    memcpy (&mbPred[joff  ][ioff], imgY, BLOCK_SIZE * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_hor_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sDecoder* decoder = mb->decoder;
  sSlice *slice = mb->slice;

  sPixelPos pix_a;

  int block_available_left;

  getNonAffNeighbour(mb, ioff - 1 , joff, decoder->mbSize[eLuma], &pix_a);

  if (decoder->activePps->hasConstrainedIntraPred)
    block_available_left = pix_a.available ? slice->intraBlock[pix_a.mbIndex]: 0;
  else
    block_available_left = pix_a.available;

  if (!block_available_left)
    printf ("warning: Intra_4x4_Horizontal prediction mode not allowed at mb %d\n",(int) slice->mbIndex);
  else {
    sPixel** imgY = plane ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
    sPixel** mbPred  =  &slice->mbPred[plane][joff];
    sPixel** img_pred =  &imgY[pix_a.posY];
    int posX = pix_a.posX;

    memset ((*(mbPred++) + ioff), *(*(img_pred++) + posX), BLOCK_SIZE * sizeof (sPixel));
    memset ((*(mbPred++) + ioff), *(*(img_pred++) + posX), BLOCK_SIZE * sizeof (sPixel));
    memset ((*(mbPred++) + ioff), *(*(img_pred++) + posX), BLOCK_SIZE * sizeof (sPixel));
    memset ((*(mbPred  ) + ioff), *(*(img_pred  ) + posX), BLOCK_SIZE * sizeof (sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_diag_down_right_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;

  sPixelPos pix_a;
  sPixelPos pix_b, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;

  sPixel** mbPred = slice->mbPred[plane];

  getNonAffNeighbour(mb, ioff -1 , joff    , decoder->mbSize[eLuma], &pix_a);
  getNonAffNeighbour(mb, ioff    , joff -1 , decoder->mbSize[eLuma], &pix_b);
  getNonAffNeighbour(mb, ioff -1 , joff -1 , decoder->mbSize[eLuma], &pix_d);

  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_left     = pix_a.available ? slice->intraBlock [pix_a.mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
    }
  else {
    block_available_left     = pix_a.available;
    block_available_up       = pix_b.available;
    block_available_up_left  = pix_d.available;
    }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_4x4_Diagonal_Down_Right prediction mode not allowed at mb %d\n",(int) slice->mbIndex);
  else {
    sPixel PredPixel[7];
    sPixel PredPel[13];
    sPixel** img_pred = &imgY[pix_a.posY];
    int pixX = pix_a.posX;
    sPixel* pred_pel = &imgY[pix_b.posY][pix_b.posX];
    // form predictor pels
    // P_A through P_D
    memcpy (&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    P_I = *(*(img_pred++) + pixX);
    P_J = *(*(img_pred++) + pixX);
    P_K = *(*(img_pred++) + pixX);
    P_L = *(*(img_pred  ) + pixX);
    P_X = imgY[pix_d.posY][pix_d.posX];

    PredPixel[0] = (sPixel) ((P_L + (P_K << 1) + P_J + 2) >> 2);
    PredPixel[1] = (sPixel) ((P_K + (P_J << 1) + P_I + 2) >> 2);
    PredPixel[2] = (sPixel) ((P_J + (P_I << 1) + P_X + 2) >> 2);
    PredPixel[3] = (sPixel) ((P_I + (P_X << 1) + P_A + 2) >> 2);
    PredPixel[4] = (sPixel) ((P_X + 2*P_A + P_B + 2) >> 2);
    PredPixel[5] = (sPixel) ((P_A + 2*P_B + P_C + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_B + 2*P_C + P_D + 2) >> 2);

    memcpy (&mbPred[joff++][ioff], &PredPixel[3], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[2], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[1], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff  ][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_diag_down_left_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixelPos pix_b, pix_c;

  int block_available_up;
  int block_available_up_right;

  getNonAffNeighbour(mb, ioff    , joff - 1, decoder->mbSize[eLuma], &pix_b);
  getNonAffNeighbour(mb, ioff + 4, joff - 1, decoder->mbSize[eLuma], &pix_c);

  pix_c.available = pix_c.available && !((ioff==4) && ((joff==4)||(joff==12)));

  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
   }
   else {
    block_available_up = pix_b.available;
    block_available_up_right = pix_c.available;
    }

  if (!block_available_up)
    printf ("warning: Intra_4x4_Diagonal_Down_Left prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
    sPixel** mbPred = slice->mbPred[plane];

    sPixel PredPixel[8];
    sPixel PredPel[25];
    sPixel* pred_pel = &imgY[pix_b.posY][pix_b.posX];

    // form predictor pels
    // P_A through P_D
    memcpy(&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    // P_E through P_H
    if (block_available_up_right)
      memcpy (&PredPel[5], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE * sizeof(sPixel));
    else
      memset (&PredPel[5], PredPel[4], BLOCK_SIZE * sizeof(sPixel));

    PredPixel[0] = (sPixel) ((P_A + P_C + 2*(P_B) + 2) >> 2);
    PredPixel[1] = (sPixel) ((P_B + P_D + 2*(P_C) + 2) >> 2);
    PredPixel[2] = (sPixel) ((P_C + P_E + 2*(P_D) + 2) >> 2);
    PredPixel[3] = (sPixel) ((P_D + P_F + 2*(P_E) + 2) >> 2);
    PredPixel[4] = (sPixel) ((P_E + P_G + 2*(P_F) + 2) >> 2);
    PredPixel[5] = (sPixel) ((P_F + P_H + 2*(P_G) + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_G + 3*(P_H) + 2) >> 2);

    memcpy (&mbPred[joff++][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[1], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[2], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff  ][ioff], &PredPixel[3], 4 * sizeof(sPixel));
   }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_vert_right_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixelPos pix_a, pix_b, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;

  getNonAffNeighbour (mb, ioff -1 , joff    , decoder->mbSize[eLuma], &pix_a);
  getNonAffNeighbour (mb, ioff    , joff -1 , decoder->mbSize[eLuma], &pix_b);
  getNonAffNeighbour (mb, ioff -1 , joff -1 , decoder->mbSize[eLuma], &pix_d);

  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_left = pix_a.available ? slice->intraBlock[pix_a.mbIndex]: 0;
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
    }
  else {
    block_available_left = pix_a.available;
    block_available_up = pix_b.available;
    block_available_up_left = pix_d.available;
    }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_4x4_Vertical_Right prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
    sPixel** mbPred = slice->mbPred[plane];
    sPixel PredPixel[10];
    sPixel PredPel[13];

    sPixel** img_pred = &imgY[pix_a.posY];
    int pixX = pix_a.posX;
    sPixel* pred_pel = &imgY[pix_b.posY][pix_b.posX];
    // form predictor pels
    // P_A through P_D
    memcpy (&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    P_I = *(*(img_pred++) + pixX);
    P_J = *(*(img_pred++) + pixX);
    P_K = *(*(img_pred++) + pixX);

    P_X = imgY[pix_d.posY][pix_d.posX];

    PredPixel[0] = (sPixel) ((P_X + (P_I << 1) + P_J + 2) >> 2);
    PredPixel[1] = (sPixel) ((P_X + P_A + 1) >> 1);
    PredPixel[2] = (sPixel) ((P_A + P_B + 1) >> 1);
    PredPixel[3] = (sPixel) ((P_B + P_C + 1) >> 1);
    PredPixel[4] = (sPixel) ((P_C + P_D + 1) >> 1);
    PredPixel[5] = (sPixel) ((P_I + (P_J << 1) + P_K + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_I + (P_X << 1) + P_A + 2) >> 2);
    PredPixel[7] = (sPixel) ((P_X + 2*P_A + P_B + 2) >> 2);
    PredPixel[8] = (sPixel) ((P_A + 2*P_B + P_C + 2) >> 2);
    PredPixel[9] = (sPixel) ((P_B + 2*P_C + P_D + 2) >> 2);

    memcpy (&mbPred[joff++][ioff], &PredPixel[1], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[6], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff  ][ioff], &PredPixel[5], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_vert_left_pred (sMacroBlock *mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixelPos pix_b, pix_c;

  int block_available_up;
  int block_available_up_right;

  getNonAffNeighbour(mb, ioff    , joff -1 , decoder->mbSize[eLuma], &pix_b);
  getNonAffNeighbour(mb, ioff +4 , joff -1 , decoder->mbSize[eLuma], &pix_c);

  pix_c.available = pix_c.available && !((ioff==4) && ((joff==4)||(joff==12)));

  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    }
  else {
    block_available_up = pix_b.available;
    block_available_up_right = pix_c.available;
    }

  if (!block_available_up)
    printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel PredPixel[10];
    sPixel PredPel[13];
    sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
    sPixel** mbPred = slice->mbPred[plane];
    sPixel *pred_pel = &imgY[pix_b.posY][pix_b.posX];

    // form predictor pels
    // P_A through P_D
    memcpy(&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    // P_E through P_H
    if (block_available_up_right)
      memcpy (&PredPel[5], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE * sizeof(sPixel));
    else
      memset (&PredPel[5], PredPel[4], BLOCK_SIZE * sizeof(sPixel));

    PredPixel[0] = (sPixel) ((P_A + P_B + 1) >> 1);
    PredPixel[1] = (sPixel) ((P_B + P_C + 1) >> 1);
    PredPixel[2] = (sPixel) ((P_C + P_D + 1) >> 1);
    PredPixel[3] = (sPixel) ((P_D + P_E + 1) >> 1);
    PredPixel[4] = (sPixel) ((P_E + P_F + 1) >> 1);
    PredPixel[5] = (sPixel) ((P_A + 2*P_B + P_C + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_B + 2*P_C + P_D + 2) >> 2);
    PredPixel[7] = (sPixel) ((P_C + 2*P_D + P_E + 2) >> 2);
    PredPixel[8] = (sPixel) ((P_D + 2*P_E + P_F + 2) >> 2);
    PredPixel[9] = (sPixel) ((P_E + 2*P_F + P_G + 2) >> 2);

    memcpy (&mbPred[joff++][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[5], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[1], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff  ][ioff], &PredPixel[6], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_hor_up_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixelPos pix_a;

  int block_available_left;

  getNonAffNeighbour (mb, ioff -1 , joff, decoder->mbSize[eLuma], &pix_a);

  if (decoder->activePps->hasConstrainedIntraPred)
    block_available_left = pix_a.available ? slice->intraBlock[pix_a.mbIndex]: 0;
  else
    block_available_left = pix_a.available;

  if (!block_available_left)
    printf ("warning: Intra_4x4_Horizontal_Up prediction mode not allowed at mb %d\n",(int) slice->mbIndex);
  else {
    sPixel PredPixel[10];
    sPixel PredPel[13];
    sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
    sPixel** mbPred = slice->mbPred[plane];

    sPixel** img_pred = &imgY[pix_a.posY];
    int pixX = pix_a.posX;

    // form predictor pels
    P_I = *(*(img_pred++) + pixX);
    P_J = *(*(img_pred++) + pixX);
    P_K = *(*(img_pred++) + pixX);
    P_L = *(*(img_pred  ) + pixX);

    PredPixel[0] = (sPixel) ((P_I + P_J + 1) >> 1);
    PredPixel[1] = (sPixel) ((P_I + (P_J << 1) + P_K + 2) >> 2);
    PredPixel[2] = (sPixel) ((P_J + P_K + 1) >> 1);
    PredPixel[3] = (sPixel) ((P_J + (P_K << 1) + P_L + 2) >> 2);
    PredPixel[4] = (sPixel) ((P_K + P_L + 1) >> 1);
    PredPixel[5] = (sPixel) ((P_K + (P_L << 1) + P_L + 2) >> 2);
    PredPixel[6] = (sPixel) P_L;
    PredPixel[7] = (sPixel) P_L;
    PredPixel[8] = (sPixel) P_L;
    PredPixel[9] = (sPixel) P_L;

    memcpy (&mbPred[joff++][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[2], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[4], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[6], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_hor_down_pred (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixelPos pix_a, pix_b, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;

  getNonAffNeighbour(mb, ioff -1 , joff    , decoder->mbSize[eLuma], &pix_a);
  getNonAffNeighbour(mb, ioff    , joff -1 , decoder->mbSize[eLuma], &pix_b);
  getNonAffNeighbour(mb, ioff -1 , joff -1 , decoder->mbSize[eLuma], &pix_d);

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

  if ((!block_available_up) || (!block_available_left) || (!block_available_up_left))
    printf ("warning: Intra_4x4_Horizontal_Down prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel PredPixel[10];
    sPixel PredPel[13];
    sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
    sPixel** mbPred = slice->mbPred[plane];

    sPixel** img_pred = &imgY[pix_a.posY];
    int pixX = pix_a.posX;
    sPixel* pred_pel = &imgY[pix_b.posY][pix_b.posX];

    // form predictor pels
    // P_A through P_D
    memcpy (&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    P_I = *(*(img_pred++) + pixX);
    P_J = *(*(img_pred++) + pixX);
    P_K = *(*(img_pred++) + pixX);
    P_L = *(*(img_pred  ) + pixX);
    P_X = imgY[pix_d.posY][pix_d.posX];

    PredPixel[0] = (sPixel) ((P_K + P_L + 1) >> 1);
    PredPixel[1] = (sPixel) ((P_J + (P_K << 1) + P_L + 2) >> 2);
    PredPixel[2] = (sPixel) ((P_J + P_K + 1) >> 1);
    PredPixel[3] = (sPixel) ((P_I + (P_J << 1) + P_K + 2) >> 2);
    PredPixel[4] = (sPixel) ((P_I + P_J + 1) >> 1);
    PredPixel[5] = (sPixel) ((P_X + (P_I << 1) + P_J + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_X + P_I + 1) >> 1);
    PredPixel[7] = (sPixel) ((P_I + (P_X << 1) + P_A + 2) >> 2);
    PredPixel[8] = (sPixel) ((P_X + 2*P_A + P_B + 2) >> 2);
    PredPixel[9] = (sPixel) ((P_A + 2*P_B + P_C + 2) >> 2);

    memcpy (&mbPred[joff++][ioff], &PredPixel[6], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[4], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[2], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff  ][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
int intra_pred_4x4_normal (sMacroBlock* mb, eColorPlane plane, int ioff, int joff,
                          int img_block_x, int img_block_y) {

  sDecoder* decoder = mb->decoder;
  byte predmode = decoder->predMode[img_block_y][img_block_x];
  mb->dpcmMode = predmode; //For residual DPCM

  switch (predmode) {
    case DC_PRED:
      return (intra4x4_dc_pred(mb, plane, ioff, joff));
      break;
    case VERT_PRED:
      return (intra4x4_vert_pred(mb, plane, ioff, joff));
      break;
    case HOR_PRED:
      return (intra4x4_hor_pred(mb, plane, ioff, joff));
      break;
    case DIAG_DOWN_RIGHT_PRED:
      return (intra4x4_diag_down_right_pred(mb, plane, ioff, joff));
      break;
    case DIAG_DOWN_LEFT_PRED:
      return (intra4x4_diag_down_left_pred(mb, plane, ioff, joff));
      break;
    case VERT_RIGHT_PRED:
      return (intra4x4_vert_right_pred(mb, plane, ioff, joff));
      break;
    case VERT_LEFT_PRED:
      return (intra4x4_vert_left_pred(mb, plane, ioff, joff));
      break;
    case HOR_UP_PRED:
      return (intra4x4_hor_up_pred(mb, plane, ioff, joff));
      break;
    case HOR_DOWN_PRED:
      return (intra4x4_hor_down_pred(mb, plane, ioff, joff));
    default:
      printf ("Error: illegal intra_4x4 prediction mode: %d\n", (int) predmode);
      return eSearchSync;
      break;
    }
  }
//}}}

//{{{
static int intra4x4_dc_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i,j;
  int s0 = 0;
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;

  sPixelPos pix_a[4], pix_b;

  int block_available_up;
  int block_available_left;

  sPixel** mbPred = slice->mbPred[plane];

  for (i = 0; i < 4; ++i)
    getAffNeighbour (mb, ioff - 1, joff + i, decoder->mbSize[eLuma], &pix_a[i]);
  getAffNeighbour (mb, ioff    , joff -1 , decoder->mbSize[eLuma], &pix_b);

  if (decoder->activePps->hasConstrainedIntraPred) {
    for (i=0, block_available_left=1; i<4;++i)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    }
  else {
    block_available_left = pix_a[0].available;
    block_available_up = pix_b.available;
    }

  // form predictor pels
  if (block_available_up) {
    s0 += imgY[pix_b.posY][pix_b.posX + 0];
    s0 += imgY[pix_b.posY][pix_b.posX + 1];
    s0 += imgY[pix_b.posY][pix_b.posX + 2];
    s0 += imgY[pix_b.posY][pix_b.posX + 3];
    }

  if (block_available_left) {
    s0 += imgY[pix_a[0].posY][pix_a[0].posX];
    s0 += imgY[pix_a[1].posY][pix_a[1].posX];
    s0 += imgY[pix_a[2].posY][pix_a[2].posX];
    s0 += imgY[pix_a[3].posY][pix_a[3].posX];
    }

  if (block_available_up && block_available_left) // no edge
    s0 = (s0 + 4)>>3;
  else if (!block_available_up && block_available_left) // upper edge
    s0 = (s0 + 2)>>2;
  else if (block_available_up && !block_available_left) // left edge
    s0 = (s0 + 2)>>2;
  else // top left corner, nothing to predict from
    s0 = decoder->coding.dcPredValueComp[plane];

  for (j = joff; j < joff + BLOCK_SIZE; ++j) {
    // store DC prediction
    mbPred[j][ioff    ] = (sPixel) s0;
    mbPred[j][ioff + 1] = (sPixel) s0;
    mbPred[j][ioff + 2] = (sPixel) s0;
    mbPred[j][ioff + 3] = (sPixel) s0;
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_vert_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int block_available_up;
  sPixelPos pix_b;

  getAffNeighbour (mb, ioff, joff - 1 , decoder->mbSize[eLuma], &pix_b);

  if (decoder->activePps->hasConstrainedIntraPred)
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
  else
    block_available_up = pix_b.available;

  if (!block_available_up)
    printf ("warning: Intra_4x4_Vertical prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel** mbPred = slice->mbPred[plane];
    sPixel *imgY = (plane) ? &slice->picture->imgUV[plane - 1][pix_b.posY][pix_b.posX] : &slice->picture->imgY[pix_b.posY][pix_b.posX];
    memcpy (&(mbPred[joff++][ioff]), imgY, BLOCK_SIZE * sizeof(sPixel));
    memcpy (&(mbPred[joff++][ioff]), imgY, BLOCK_SIZE * sizeof(sPixel));
    memcpy (&(mbPred[joff++][ioff]), imgY, BLOCK_SIZE * sizeof(sPixel));
    memcpy (&(mbPred[joff  ][ioff]), imgY, BLOCK_SIZE * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_hor_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sDecoder* decoder = mb->decoder;
  sSlice *slice = mb->slice;

  int i,j;
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;

  sPixelPos pix_a[4];

  int block_available_left;

  sPixel* predrow, prediction,** mbPred = slice->mbPred[plane];

  for (i = 0; i < 4; ++i)
    getAffNeighbour (mb, ioff -1 , joff +i , decoder->mbSize[eLuma], &pix_a[i]);

  if (decoder->activePps->hasConstrainedIntraPred)
    for (i = 0, block_available_left = 1; i < 4; ++i)
      block_available_left&= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
  else
    block_available_left = pix_a[0].available;

  if (!block_available_left)
    printf ("warning: Intra_4x4_Horizontal prediction mode not allowed at mb %d\n",(int) slice->mbIndex);

  for(j = 0; j < BLOCK_SIZE; ++j) {
    predrow = mbPred[j+joff];
    prediction = imgY[pix_a[j].posY][pix_a[j].posX];
    for (i = ioff; i < ioff + BLOCK_SIZE;++i)
      predrow[i]= prediction; /* store predicted 4x4 block */
   }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_diag_down_right_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;

  sPixelPos pix_a[4];
  sPixelPos pix_b, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;

  sPixel** mbPred = slice->mbPred[plane];

  for (i = 0; i < 4; ++i)
    getAffNeighbour (mb, ioff -1 , joff +i , decoder->mbSize[eLuma], &pix_a[i]);

  getAffNeighbour (mb, ioff    , joff -1 , decoder->mbSize[eLuma], &pix_b);
  getAffNeighbour (mb, ioff -1 , joff -1 , decoder->mbSize[eLuma], &pix_d);

  if (decoder->activePps->hasConstrainedIntraPred) {
    for (i = 0, block_available_left = 1; i < 4; ++i)
      block_available_left &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_left = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
    }
  else {
    block_available_left = pix_a[0].available;
    block_available_up = pix_b.available;
    block_available_up_left = pix_d.available;
    }

  if ((!block_available_up) || (!block_available_left) || (!block_available_up_left))
    printf ("warning: Intra_4x4_Diagonal_Down_Right prediction mode not allowed at mb %d\n",(int) slice->mbIndex);
  else {
    sPixel PredPixel[7];
    sPixel PredPel[13];
    sPixel *pred_pel = &imgY[pix_b.posY][pix_b.posX];
    // form predictor pels
    // P_A through P_D
    memcpy(&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    P_I = imgY[pix_a[0].posY][pix_a[0].posX];
    P_J = imgY[pix_a[1].posY][pix_a[1].posX];
    P_K = imgY[pix_a[2].posY][pix_a[2].posX];
    P_L = imgY[pix_a[3].posY][pix_a[3].posX];
    P_X = imgY[pix_d.posY][pix_d.posX];

    PredPixel[0] = (sPixel) ((P_L + (P_K << 1) + P_J + 2) >> 2);
    PredPixel[1] = (sPixel) ((P_K + (P_J << 1) + P_I + 2) >> 2);
    PredPixel[2] = (sPixel) ((P_J + (P_I << 1) + P_X + 2) >> 2);
    PredPixel[3] = (sPixel) ((P_I + (P_X << 1) + P_A + 2) >> 2);
    PredPixel[4] = (sPixel) ((P_X + 2*P_A + P_B + 2) >> 2);
    PredPixel[5] = (sPixel) ((P_A + 2*P_B + P_C + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_B + 2*P_C + P_D + 2) >> 2);

    memcpy (&mbPred[joff++][ioff], &PredPixel[3], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[2], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[1], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff  ][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_diag_down_left_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixelPos pix_b, pix_c;

  int block_available_up;
  int block_available_up_right;

  getAffNeighbour(mb, ioff    , joff - 1, decoder->mbSize[eLuma], &pix_b);
  getAffNeighbour(mb, ioff + 4, joff - 1, decoder->mbSize[eLuma], &pix_c);

  pix_c.available = pix_c.available && !((ioff==4) && ((joff==4)||(joff==12)));

  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    }
  else {
    block_available_up       = pix_b.available;
    block_available_up_right = pix_c.available;
    }

  if (!block_available_up)
    printf ("warning: Intra_4x4_Diagonal_Down_Left prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
    sPixel** mbPred = slice->mbPred[plane];

    sPixel PredPixel[8];
    sPixel PredPel[25];
    sPixel *pred_pel = &imgY[pix_b.posY][pix_b.posX];

    // form predictor pels
    // P_A through P_D
    memcpy(&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    // P_E through P_H
    if (block_available_up_right)
      memcpy(&PredPel[5], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE * sizeof(sPixel));
    else {
      P_E = P_F = P_G = P_H = P_D;
      }

    PredPixel[0] = (sPixel) ((P_A + P_C + 2*(P_B) + 2) >> 2);
    PredPixel[1] = (sPixel) ((P_B + P_D + 2*(P_C) + 2) >> 2);
    PredPixel[2] = (sPixel) ((P_C + P_E + 2*(P_D) + 2) >> 2);
    PredPixel[3] = (sPixel) ((P_D + P_F + 2*(P_E) + 2) >> 2);
    PredPixel[4] = (sPixel) ((P_E + P_G + 2*(P_F) + 2) >> 2);
    PredPixel[5] = (sPixel) ((P_F + P_H + 2*(P_G) + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_G + 3*(P_H) + 2) >> 2);

    memcpy(&mbPred[joff++][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    memcpy(&mbPred[joff++][ioff], &PredPixel[1], 4 * sizeof(sPixel));
    memcpy(&mbPred[joff++][ioff], &PredPixel[2], 4 * sizeof(sPixel));
    memcpy(&mbPred[joff  ][ioff], &PredPixel[3], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_vert_right_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;

  sPixelPos pix_a[4];
  sPixelPos pix_b, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;

  sPixel** mbPred = slice->mbPred[plane];

  for (i=0;i<4;++i)
    getAffNeighbour(mb, ioff -1 , joff +i , decoder->mbSize[eLuma], &pix_a[i]);

  getAffNeighbour(mb, ioff    , joff -1 , decoder->mbSize[eLuma], &pix_b);
  getAffNeighbour(mb, ioff -1 , joff -1 , decoder->mbSize[eLuma], &pix_d);

  if (decoder->activePps->hasConstrainedIntraPred) {
    for (i=0, block_available_left=1; i<4;++i)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
    }
  else {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_left  = pix_d.available;
    }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_4x4_Vertical_Right prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel PredPixel[10];
    sPixel PredPel[13];
    sPixel *pred_pel = &imgY[pix_b.posY][pix_b.posX];
    // form predictor pels
    // P_A through P_D
    memcpy(&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    P_I = imgY[pix_a[0].posY][pix_a[0].posX];
    P_J = imgY[pix_a[1].posY][pix_a[1].posX];
    P_K = imgY[pix_a[2].posY][pix_a[2].posX];
    P_X = imgY[pix_d.posY][pix_d.posX];

    PredPixel[0] = (sPixel) ((P_X + (P_I << 1) + P_J + 2) >> 2);
    PredPixel[1] = (sPixel) ((P_X + P_A + 1) >> 1);
    PredPixel[2] = (sPixel) ((P_A + P_B + 1) >> 1);
    PredPixel[3] = (sPixel) ((P_B + P_C + 1) >> 1);
    PredPixel[4] = (sPixel) ((P_C + P_D + 1) >> 1);
    PredPixel[5] = (sPixel) ((P_I + (P_J << 1) + P_K + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_I + (P_X << 1) + P_A + 2) >> 2);
    PredPixel[7] = (sPixel) ((P_X + 2*P_A + P_B + 2) >> 2);
    PredPixel[8] = (sPixel) ((P_A + 2*P_B + P_C + 2) >> 2);
    PredPixel[9] = (sPixel) ((P_B + 2*P_C + P_D + 2) >> 2);

    memcpy(&mbPred[joff++][ioff], &PredPixel[1], 4 * sizeof(sPixel));
    memcpy(&mbPred[joff++][ioff], &PredPixel[6], 4 * sizeof(sPixel));
    memcpy(&mbPred[joff++][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    memcpy(&mbPred[joff  ][ioff], &PredPixel[5], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_vert_left_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPixelPos pix_b, pix_c;

  int block_available_up;
  int block_available_up_right;

  getAffNeighbour(mb, ioff    , joff -1 , decoder->mbSize[eLuma], &pix_b);
  getAffNeighbour(mb, ioff +4 , joff -1 , decoder->mbSize[eLuma], &pix_c);

  pix_c.available = pix_c.available && !((ioff==4) && ((joff==4)||(joff==12)));

  if (decoder->activePps->hasConstrainedIntraPred) {
    block_available_up = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_right = pix_c.available ? slice->intraBlock [pix_c.mbIndex] : 0;
    }
  else {
    block_available_up = pix_b.available;
    block_available_up_right = pix_c.available;
    }

  if (!block_available_up)
    printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel PredPixel[10];
    sPixel PredPel[13];
    sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;
    sPixel** mbPred = slice->mbPred[plane];
    sPixel *pred_pel = &imgY[pix_b.posY][pix_b.posX];

    // form predictor pels
    // P_A through P_D
    memcpy (&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    // P_E through P_H
    if (block_available_up_right)
      memcpy(&PredPel[5], &imgY[pix_c.posY][pix_c.posX], BLOCK_SIZE * sizeof(sPixel));
    else {
      P_E = P_F = P_G = P_H = P_D;
      }

    PredPixel[0] = (sPixel) ((P_A + P_B + 1) >> 1);
    PredPixel[1] = (sPixel) ((P_B + P_C + 1) >> 1);
    PredPixel[2] = (sPixel) ((P_C + P_D + 1) >> 1);
    PredPixel[3] = (sPixel) ((P_D + P_E + 1) >> 1);
    PredPixel[4] = (sPixel) ((P_E + P_F + 1) >> 1);
    PredPixel[5] = (sPixel) ((P_A + 2*P_B + P_C + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_B + 2*P_C + P_D + 2) >> 2);
    PredPixel[7] = (sPixel) ((P_C + 2*P_D + P_E + 2) >> 2);
    PredPixel[8] = (sPixel) ((P_D + 2*P_E + P_F + 2) >> 2);
    PredPixel[9] = (sPixel) ((P_E + 2*P_F + P_G + 2) >> 2);

    memcpy (&mbPred[joff++][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[5], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[1], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff  ][ioff], &PredPixel[6], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_hor_up_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;

  sPixelPos pix_a[4];

  int block_available_left;

  sPixel** mbPred = slice->mbPred[plane];

  for (i = 0; i < 4; ++i)
    getAffNeighbour(mb, ioff -1 , joff +i , decoder->mbSize[eLuma], &pix_a[i]);

  if (decoder->activePps->hasConstrainedIntraPred)
    for (i=0, block_available_left=1; i<4;++i)
      block_available_left &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
  else
    block_available_left = pix_a[0].available;

  if (!block_available_left)
    printf ("warning: Intra_4x4_Horizontal_Up prediction mode not allowed at mb %d\n",(int) slice->mbIndex);
  else {
    sPixel PredPixel[10];
    sPixel PredPel[13];

    // form predictor pels
    P_I = imgY[pix_a[0].posY][pix_a[0].posX];
    P_J = imgY[pix_a[1].posY][pix_a[1].posX];
    P_K = imgY[pix_a[2].posY][pix_a[2].posX];
    P_L = imgY[pix_a[3].posY][pix_a[3].posX];

    PredPixel[0] = (sPixel) ((P_I + P_J + 1) >> 1);
    PredPixel[1] = (sPixel) ((P_I + (P_J << 1) + P_K + 2) >> 2);
    PredPixel[2] = (sPixel) ((P_J + P_K + 1) >> 1);
    PredPixel[3] = (sPixel) ((P_J + (P_K << 1) + P_L + 2) >> 2);
    PredPixel[4] = (sPixel) ((P_K + P_L + 1) >> 1);
    PredPixel[5] = (sPixel) ((P_K + (P_L << 1) + P_L + 2) >> 2);
    PredPixel[6] = (sPixel) P_L;
    PredPixel[7] = (sPixel) P_L;
    PredPixel[8] = (sPixel) P_L;
    PredPixel[9] = (sPixel) P_L;

    memcpy (&mbPred[joff++][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[2], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[4], 4 * sizeof(sPixel));
    memcpy (&mbPred[joff++][ioff], &PredPixel[6], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
static int intra4x4_hor_down_pred_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff) {

  sSlice *slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int i;
  sPixel** imgY = (plane) ? slice->picture->imgUV[plane - 1] : slice->picture->imgY;

  sPixelPos pix_a[4];
  sPixelPos pix_b, pix_d;

  int block_available_up;
  int block_available_left;
  int block_available_up_left;

  sPixel** mbPred = slice->mbPred[plane];

  for (i = 0; i < 4; ++i)
    getAffNeighbour(mb, ioff -1 , joff +i , decoder->mbSize[eLuma], &pix_a[i]);

  getAffNeighbour(mb, ioff    , joff -1 , decoder->mbSize[eLuma], &pix_b);
  getAffNeighbour(mb, ioff -1 , joff -1 , decoder->mbSize[eLuma], &pix_d);

  if (decoder->activePps->hasConstrainedIntraPred) {
    for (i=0, block_available_left=1; i<4;++i)
      block_available_left  &= pix_a[i].available ? slice->intraBlock[pix_a[i].mbIndex]: 0;
    block_available_up       = pix_b.available ? slice->intraBlock [pix_b.mbIndex] : 0;
    block_available_up_left  = pix_d.available ? slice->intraBlock [pix_d.mbIndex] : 0;
    }
  else {
    block_available_left     = pix_a[0].available;
    block_available_up       = pix_b.available;
    block_available_up_left  = pix_d.available;
    }

  if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
    printf ("warning: Intra_4x4_Horizontal_Down prediction mode not allowed at mb %d\n", (int) slice->mbIndex);
  else {
    sPixel PredPixel[10];
    sPixel PredPel[13];
    sPixel *pred_pel = &imgY[pix_b.posY][pix_b.posX];

    // form predictor pels
    // P_A through P_D
    memcpy(&PredPel[1], pred_pel, BLOCK_SIZE * sizeof(sPixel));

    P_I = imgY[pix_a[0].posY][pix_a[0].posX];
    P_J = imgY[pix_a[1].posY][pix_a[1].posX];
    P_K = imgY[pix_a[2].posY][pix_a[2].posX];
    P_L = imgY[pix_a[3].posY][pix_a[3].posX];

    P_X = imgY[pix_d.posY][pix_d.posX];

    PredPixel[0] = (sPixel) ((P_K + P_L + 1) >> 1);
    PredPixel[1] = (sPixel) ((P_J + (P_K << 1) + P_L + 2) >> 2);
    PredPixel[2] = (sPixel) ((P_J + P_K + 1) >> 1);
    PredPixel[3] = (sPixel) ((P_I + (P_J << 1) + P_K + 2) >> 2);
    PredPixel[4] = (sPixel) ((P_I + P_J + 1) >> 1);
    PredPixel[5] = (sPixel) ((P_X + (P_I << 1) + P_J + 2) >> 2);
    PredPixel[6] = (sPixel) ((P_X + P_I + 1) >> 1);
    PredPixel[7] = (sPixel) ((P_I + (P_X << 1) + P_A + 2) >> 2);
    PredPixel[8] = (sPixel) ((P_X + 2*P_A + P_B + 2) >> 2);
    PredPixel[9] = (sPixel) ((P_A + 2*P_B + P_C + 2) >> 2);

    memcpy(&mbPred[joff++][ioff], &PredPixel[6], 4 * sizeof(sPixel));
    memcpy(&mbPred[joff++][ioff], &PredPixel[4], 4 * sizeof(sPixel));
    memcpy(&mbPred[joff++][ioff], &PredPixel[2], 4 * sizeof(sPixel));
    memcpy(&mbPred[joff  ][ioff], &PredPixel[0], 4 * sizeof(sPixel));
    }

  return eDecodingOk;
  }
//}}}
//{{{
int intra_pred_4x4_mbaff (sMacroBlock* mb, eColorPlane plane, int ioff, int joff,
                          int img_block_x, int img_block_y) {

  sDecoder* decoder = mb->decoder;
  byte predmode = decoder->predMode[img_block_y][img_block_x];
  mb->dpcmMode = predmode; //For residual DPCM

  switch (predmode) {
    case DC_PRED:
      return (intra4x4_dc_pred_mbaff(mb, plane, ioff, joff));
      break;
    case VERT_PRED:
      return (intra4x4_vert_pred_mbaff(mb, plane, ioff, joff));
      break;
    case HOR_PRED:
      return (intra4x4_hor_pred_mbaff(mb, plane, ioff, joff));
      break;
    case DIAG_DOWN_RIGHT_PRED:
      return (intra4x4_diag_down_right_pred_mbaff(mb, plane, ioff, joff));
      break;
    case DIAG_DOWN_LEFT_PRED:
      return (intra4x4_diag_down_left_pred_mbaff(mb, plane, ioff, joff));
      break;
    case VERT_RIGHT_PRED:
      return (intra4x4_vert_right_pred_mbaff(mb, plane, ioff, joff));
      break;
    case VERT_LEFT_PRED:
      return (intra4x4_vert_left_pred_mbaff(mb, plane, ioff, joff));
      break;
    case HOR_UP_PRED:
      return (intra4x4_hor_up_pred_mbaff(mb, plane, ioff, joff));
      break;
    case HOR_DOWN_PRED:
      return (intra4x4_hor_down_pred_mbaff(mb, plane, ioff, joff));
    default:
      printf("Error: illegal intra_4x4 prediction mode: %d\n", (int) predmode);
      return eSearchSync;
      break;
    }
  }
//}}}
