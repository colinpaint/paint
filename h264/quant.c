//{{{  includes
#include "global.h"
#include "memory.h"

#include "block.h"
#include "image.h"
#include "mbAccess.h"
#include "transform.h"
#include "quant.h"
//}}}

//{{{
static int quant_intra_default[16] = {
   6,13,20,28,
  13,20,28,32,
  20,28,32,37,
  28,32,37,42
};
//}}}
//{{{
static int quant_inter_default[16] = {
  10,14,20,24,
  14,20,24,27,
  20,24,27,30,
  24,27,30,34
};
//}}}
//{{{
static int quant8_intra_default[64] = {
 6,10,13,16,18,23,25,27,
10,11,16,18,23,25,27,29,
13,16,18,23,25,27,29,31,
16,18,23,25,27,29,31,33,
18,23,25,27,29,31,33,36,
23,25,27,29,31,33,36,38,
25,27,29,31,33,36,38,40,
27,29,31,33,36,38,40,42
};
//}}}
//{{{
static int quant8_inter_default[64] = {
 9,13,15,17,19,21,22,24,
13,13,17,19,21,22,24,25,
15,17,19,21,22,24,25,27,
17,19,21,22,24,25,27,28,
19,21,22,24,25,27,28,30,
21,22,24,25,27,28,30,32,
22,24,25,27,28,30,32,33,
24,25,27,28,30,32,33,35
};
//}}}
//{{{
static int quant_org[16] = {
16,16,16,16,
16,16,16,16,
16,16,16,16,
16,16,16,16
};
//}}}
//{{{
static int quant8_org[64] = {
16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16
};
//}}}

//{{{
static void setDequant4x4 (int (*InvLevelScale4x4)[4], const int (*dequant)[4], int* qmatrix) {

  for (int j = 0; j < 4; j++) {
    *(*InvLevelScale4x4      ) = *(*dequant      ) * *qmatrix++;
    *(*InvLevelScale4x4   + 1) = *(*dequant   + 1) * *qmatrix++;
    *(*InvLevelScale4x4   + 2) = *(*dequant   + 2) * *qmatrix++;
    *(*InvLevelScale4x4++ + 3) = *(*dequant++ + 3) * *qmatrix++;
    }
  }
//}}}
//{{{
static void setDequant8x8 (int (*InvLevelScale8x8)[8], const int (*dequant)[8], int* qmatrix) {

  for (int j = 0; j < 8; j++) {
    *(*InvLevelScale8x8      ) = *(*dequant      ) * *qmatrix++;
    *(*InvLevelScale8x8   + 1) = *(*dequant   + 1) * *qmatrix++;
    *(*InvLevelScale8x8   + 2) = *(*dequant   + 2) * *qmatrix++;
    *(*InvLevelScale8x8   + 3) = *(*dequant   + 3) * *qmatrix++;
    *(*InvLevelScale8x8   + 4) = *(*dequant   + 4) * *qmatrix++;
    *(*InvLevelScale8x8   + 5) = *(*dequant   + 5) * *qmatrix++;
    *(*InvLevelScale8x8   + 6) = *(*dequant   + 6) * *qmatrix++;
    *(*InvLevelScale8x8++ + 7) = *(*dequant++ + 7) * *qmatrix++;
    }
  }
//}}}

//{{{
static void CalculateQuant4x4Param (sSlice* slice) {

  const int (*p_dequant_coef)[4][4] = dequant_coef;
  int (*InvLevelScale4x4_Intra_0)[4][4] = slice->InvLevelScale4x4_Intra[0];
  int (*InvLevelScale4x4_Intra_1)[4][4] = slice->InvLevelScale4x4_Intra[1];
  int (*InvLevelScale4x4_Intra_2)[4][4] = slice->InvLevelScale4x4_Intra[2];
  int (*InvLevelScale4x4_Inter_0)[4][4] = slice->InvLevelScale4x4_Inter[0];
  int (*InvLevelScale4x4_Inter_1)[4][4] = slice->InvLevelScale4x4_Inter[1];
  int (*InvLevelScale4x4_Inter_2)[4][4] = slice->InvLevelScale4x4_Inter[2];

  for (int k = 0; k < 6; k++) {
    setDequant4x4(*InvLevelScale4x4_Intra_0++, *p_dequant_coef  , slice->qmatrix[0]);
    setDequant4x4(*InvLevelScale4x4_Intra_1++, *p_dequant_coef  , slice->qmatrix[1]);
    setDequant4x4(*InvLevelScale4x4_Intra_2++, *p_dequant_coef  , slice->qmatrix[2]);
    setDequant4x4(*InvLevelScale4x4_Inter_0++, *p_dequant_coef  , slice->qmatrix[3]);
    setDequant4x4(*InvLevelScale4x4_Inter_1++, *p_dequant_coef  , slice->qmatrix[4]);
    setDequant4x4(*InvLevelScale4x4_Inter_2++, *p_dequant_coef++, slice->qmatrix[5]);
    }
  }
//}}}
//{{{
static void CalculateQuant8x8Param (sSlice* slice) {

  const int (*p_dequant_coef)[8][8] = dequant_coef8;
  int (*InvLevelScale8x8_Intra_0)[8][8] = slice->InvLevelScale8x8_Intra[0];
  int (*InvLevelScale8x8_Intra_1)[8][8] = slice->InvLevelScale8x8_Intra[1];
  int (*InvLevelScale8x8_Intra_2)[8][8] = slice->InvLevelScale8x8_Intra[2];
  int (*InvLevelScale8x8_Inter_0)[8][8] = slice->InvLevelScale8x8_Inter[0];
  int (*InvLevelScale8x8_Inter_1)[8][8] = slice->InvLevelScale8x8_Inter[1];
  int (*InvLevelScale8x8_Inter_2)[8][8] = slice->InvLevelScale8x8_Inter[2];

  for (int k = 0; k < 6; k++) {
    setDequant8x8 (*InvLevelScale8x8_Intra_0++, *p_dequant_coef  , slice->qmatrix[6]);
    setDequant8x8 (*InvLevelScale8x8_Inter_0++, *p_dequant_coef++, slice->qmatrix[7]);
    }

  p_dequant_coef = dequant_coef8;
  if (slice->activeSps->chromaFormatIdc == 3) {
    // 4:4:4
    for (int k = 0; k < 6; k++) {
      setDequant8x8 (*InvLevelScale8x8_Intra_1++, *p_dequant_coef  , slice->qmatrix[8]);
      setDequant8x8 (*InvLevelScale8x8_Inter_1++, *p_dequant_coef  , slice->qmatrix[9]);
      setDequant8x8 (*InvLevelScale8x8_Intra_2++, *p_dequant_coef  , slice->qmatrix[10]);
      setDequant8x8 (*InvLevelScale8x8_Inter_2++, *p_dequant_coef++, slice->qmatrix[11]);
      }
    }
  }
//}}}

//{{{
void allocQuant (sDecoder* decoder) {
// alloc quant matrices

  int bitDepth_qp_scale = imax (decoder->coding.bitDepthLumaQpScale, decoder->coding.bitDepthChromaQpScale);

  if (!decoder->qpPerMatrix)
    decoder->qpPerMatrix = (int*)malloc ((MAX_QP + 1 + bitDepth_qp_scale)*sizeof(int));

  if (!decoder->qpRemMatrix)
    decoder->qpRemMatrix = (int*)malloc ((MAX_QP + 1 + bitDepth_qp_scale)*sizeof(int));

  for (int i = 0; i < MAX_QP + bitDepth_qp_scale + 1; i++) {
    decoder->qpPerMatrix[i] = i / 6;
    decoder->qpRemMatrix[i] = i % 6;
    }
  }
//}}}
//{{{
void freeQuant (sDecoder* decoder) {

  free (decoder->qpPerMatrix);
  decoder->qpPerMatrix = NULL;

  free (decoder->qpRemMatrix);
  decoder->qpRemMatrix = NULL;
  }
//}}}

//{{{
void useQuantParams (sSlice* slice) {

  sSps* sps = slice->activeSps;
  sPps* pps = slice->activePps;

  if (!pps->hasPicScalingMatrix && !sps->hasSeqScalingMatrix) {
    for (int i = 0; i < 12; i++)
      slice->qmatrix[i] = (i < 6) ? quant_org : quant8_org;
    }
  else {
    int n_ScalingList = (sps->chromaFormatIdc != YUV444) ? 8 : 12;
    if (sps->hasSeqScalingMatrix) {
      //{{{  check sps first
      for (int i = 0; i < n_ScalingList; i++) {
        if (i < 6) {
          if (!sps->hasSeqScalingList[i]) {
            // fall-back rule A
            if (i == 0)
              slice->qmatrix[i] = quant_intra_default;
            else if (i == 3)
              slice->qmatrix[i] = quant_inter_default;
            else
              slice->qmatrix[i] = slice->qmatrix[i-1];
            }
          else {
            if (sps->useDefaultScalingMatrix4x4[i])
              slice->qmatrix[i] = (i<3) ? quant_intra_default : quant_inter_default;
            else
              slice->qmatrix[i] = sps->scalingList4x4[i];
          }
        }
        else {
          if (!sps->hasSeqScalingList[i]) {
            // fall-back rule A
            if (i == 6)
              slice->qmatrix[i] = quant8_intra_default;
            else if (i == 7)
              slice->qmatrix[i] = quant8_inter_default;
            else
              slice->qmatrix[i] = slice->qmatrix[i-2];
            }
          else {
            if (sps->useDefaultScalingMatrix8x8[i-6])
              slice->qmatrix[i] = (i==6 || i==8 || i==10) ? quant8_intra_default:quant8_inter_default;
            else
              slice->qmatrix[i] = sps->scalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    if (pps->hasPicScalingMatrix) {
      //{{{  then check pps
      for (int i = 0; i < n_ScalingList; i++) {
        if (i < 6) {
          if (!pps->picScalingListPresentFlag[i]) {
            // fall-back rule B
            if (i == 0) {
              if (!sps->hasSeqScalingMatrix)
                slice->qmatrix[i] = quant_intra_default;
              }
            else if (i == 3) {
              if (!sps->hasSeqScalingMatrix)
                slice->qmatrix[i] = quant_inter_default;
              }
            else
              slice->qmatrix[i] = slice->qmatrix[i-1];
            }
          else {
            if (pps->useDefaultScalingMatrix4x4Flag[i])
              slice->qmatrix[i] = (i<3) ? quant_intra_default:quant_inter_default;
            else
              slice->qmatrix[i] = pps->scalingList4x4[i];
            }
          }
        else {
          if (!pps->picScalingListPresentFlag[i]) {
            // fall-back rule B
            if (i == 6) {
              if (!sps->hasSeqScalingMatrix)
                slice->qmatrix[i] = quant8_intra_default;
              }
            else if (i == 7) {
              if (!sps->hasSeqScalingMatrix)
                slice->qmatrix[i] = quant8_inter_default;
              }
            else
              slice->qmatrix[i] = slice->qmatrix[i-2];
            }
          else {
            if (pps->useDefaultScalingMatrix8x8Flag[i-6])
              slice->qmatrix[i] = (i==6 || i==8 || i==10) ? quant8_intra_default:quant8_inter_default;
            else
              slice->qmatrix[i] = pps->scalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    }

  CalculateQuant4x4Param (slice);

  if (pps->hasTransform8x8mode)
    CalculateQuant8x8Param (slice);
  }
//}}}
