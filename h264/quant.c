//{{{  includes
#include "global.h"
#include "memAlloc.h"

#include "block.h"
#include "image.h"
#include "mbAccess.h"
#include "transform.h"
#include "quant.h"
//}}}

//{{{
static const int dequant_coef8[6][8][8] = {
  {
    {20,  19, 25, 19, 20, 19, 25, 19},
    {19,  18, 24, 18, 19, 18, 24, 18},
    {25,  24, 32, 24, 25, 24, 32, 24},
    {19,  18, 24, 18, 19, 18, 24, 18},
    {20,  19, 25, 19, 20, 19, 25, 19},
    {19,  18, 24, 18, 19, 18, 24, 18},
    {25,  24, 32, 24, 25, 24, 32, 24},
    {19,  18, 24, 18, 19, 18, 24, 18}
  },
  {
    {22,  21, 28, 21, 22, 21, 28, 21},
    {21,  19, 26, 19, 21, 19, 26, 19},
    {28,  26, 35, 26, 28, 26, 35, 26},
    {21,  19, 26, 19, 21, 19, 26, 19},
    {22,  21, 28, 21, 22, 21, 28, 21},
    {21,  19, 26, 19, 21, 19, 26, 19},
    {28,  26, 35, 26, 28, 26, 35, 26},
    {21,  19, 26, 19, 21, 19, 26, 19}
  },
  {
    {26,  24, 33, 24, 26, 24, 33, 24},
    {24,  23, 31, 23, 24, 23, 31, 23},
    {33,  31, 42, 31, 33, 31, 42, 31},
    {24,  23, 31, 23, 24, 23, 31, 23},
    {26,  24, 33, 24, 26, 24, 33, 24},
    {24,  23, 31, 23, 24, 23, 31, 23},
    {33,  31, 42, 31, 33, 31, 42, 31},
    {24,  23, 31, 23, 24, 23, 31, 23}
  },
  {
    {28,  26, 35, 26, 28, 26, 35, 26},
    {26,  25, 33, 25, 26, 25, 33, 25},
    {35,  33, 45, 33, 35, 33, 45, 33},
    {26,  25, 33, 25, 26, 25, 33, 25},
    {28,  26, 35, 26, 28, 26, 35, 26},
    {26,  25, 33, 25, 26, 25, 33, 25},
    {35,  33, 45, 33, 35, 33, 45, 33},
    {26,  25, 33, 25, 26, 25, 33, 25}
  },
  {
    {32,  30, 40, 30, 32, 30, 40, 30},
    {30,  28, 38, 28, 30, 28, 38, 28},
    {40,  38, 51, 38, 40, 38, 51, 38},
    {30,  28, 38, 28, 30, 28, 38, 28},
    {32,  30, 40, 30, 32, 30, 40, 30},
    {30,  28, 38, 28, 30, 28, 38, 28},
    {40,  38, 51, 38, 40, 38, 51, 38},
    {30,  28, 38, 28, 30, 28, 38, 28}
  },
  {
    {36,  34, 46, 34, 36, 34, 46, 34},
    {34,  32, 43, 32, 34, 32, 43, 32},
    {46,  43, 58, 43, 46, 43, 58, 43},
    {34,  32, 43, 32, 34, 32, 43, 32},
    {36,  34, 46, 34, 36, 34, 46, 34},
    {34,  32, 43, 32, 34, 32, 43, 32},
    {46,  43, 58, 43, 46, 43, 58, 43},
    {34,  32, 43, 32, 34, 32, 43, 32}
  }
};
//}}}
//{{{
//! Dequantization coefficients
static const int dequant_coef[6][4][4] = {
  {
    { 10, 13, 10, 13},
    { 13, 16, 13, 16},
    { 10, 13, 10, 13},
    { 13, 16, 13, 16}},
  {
    { 11, 14, 11, 14},
    { 14, 18, 14, 18},
    { 11, 14, 11, 14},
    { 14, 18, 14, 18}},
  {
    { 13, 16, 13, 16},
    { 16, 20, 16, 20},
    { 13, 16, 13, 16},
    { 16, 20, 16, 20}},
  {
    { 14, 18, 14, 18},
    { 18, 23, 18, 23},
    { 14, 18, 14, 18},
    { 18, 23, 18, 23}},
  {
    { 16, 20, 16, 20},
    { 20, 25, 20, 25},
    { 16, 20, 16, 20},
    { 20, 25, 20, 25}},
  {
    { 18, 23, 18, 23},
    { 23, 29, 23, 29},
    { 18, 23, 18, 23},
    { 23, 29, 23, 29}}
};
//}}}
//{{{
static const int quant_coef[6][4][4] = {
  {
    { 13107,  8066, 13107,  8066},
    {  8066,  5243,  8066,  5243},
    { 13107,  8066, 13107,  8066},
    {  8066,  5243,  8066,  5243}},
  {
    { 11916,  7490, 11916,  7490},
    {  7490,  4660,  7490,  4660},
    { 11916,  7490, 11916,  7490},
    {  7490,  4660,  7490,  4660}},
  {
    { 10082,  6554, 10082,  6554},
    {  6554,  4194,  6554,  4194},
    { 10082,  6554, 10082,  6554},
    {  6554,  4194,  6554,  4194}},
  {
    {  9362,  5825,  9362,  5825},
    {  5825,  3647,  5825,  3647},
    {  9362,  5825,  9362,  5825},
    {  5825,  3647,  5825,  3647}},
  {
    {  8192,  5243,  8192,  5243},
    {  5243,  3355,  5243,  3355},
    {  8192,  5243,  8192,  5243},
    {  5243,  3355,  5243,  3355}},
  {
    {  7282,  4559,  7282,  4559},
    {  4559,  2893,  4559,  2893},
    {  7282,  4559,  7282,  4559},
    {  4559,  2893,  4559,  2893}}
};
//}}}
//{{{
static const int A[4][4] = {
  { 16, 20, 16, 20},
  { 20, 25, 20, 25},
  { 16, 20, 16, 20},
  { 20, 25, 20, 25}
};
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
static void CalculateQuant4x4Param (sSlice* curSlice) {

  const int (*p_dequant_coef)[4][4] = dequant_coef;
  int (*InvLevelScale4x4_Intra_0)[4][4] = curSlice->InvLevelScale4x4_Intra[0];
  int (*InvLevelScale4x4_Intra_1)[4][4] = curSlice->InvLevelScale4x4_Intra[1];
  int (*InvLevelScale4x4_Intra_2)[4][4] = curSlice->InvLevelScale4x4_Intra[2];
  int (*InvLevelScale4x4_Inter_0)[4][4] = curSlice->InvLevelScale4x4_Inter[0];
  int (*InvLevelScale4x4_Inter_1)[4][4] = curSlice->InvLevelScale4x4_Inter[1];
  int (*InvLevelScale4x4_Inter_2)[4][4] = curSlice->InvLevelScale4x4_Inter[2];

  for (int k = 0; k < 6; k++) {
    setDequant4x4(*InvLevelScale4x4_Intra_0++, *p_dequant_coef  , curSlice->qmatrix[0]);
    setDequant4x4(*InvLevelScale4x4_Intra_1++, *p_dequant_coef  , curSlice->qmatrix[1]);
    setDequant4x4(*InvLevelScale4x4_Intra_2++, *p_dequant_coef  , curSlice->qmatrix[2]);
    setDequant4x4(*InvLevelScale4x4_Inter_0++, *p_dequant_coef  , curSlice->qmatrix[3]);
    setDequant4x4(*InvLevelScale4x4_Inter_1++, *p_dequant_coef  , curSlice->qmatrix[4]);
    setDequant4x4(*InvLevelScale4x4_Inter_2++, *p_dequant_coef++, curSlice->qmatrix[5]);
    }
  }
//}}}
//{{{
static void CalculateQuant8x8Param (sSlice* curSlice) {

  const int (*p_dequant_coef)[8][8] = dequant_coef8;
  int (*InvLevelScale8x8_Intra_0)[8][8] = curSlice->InvLevelScale8x8_Intra[0];
  int (*InvLevelScale8x8_Intra_1)[8][8] = curSlice->InvLevelScale8x8_Intra[1];
  int (*InvLevelScale8x8_Intra_2)[8][8] = curSlice->InvLevelScale8x8_Intra[2];
  int (*InvLevelScale8x8_Inter_0)[8][8] = curSlice->InvLevelScale8x8_Inter[0];
  int (*InvLevelScale8x8_Inter_1)[8][8] = curSlice->InvLevelScale8x8_Inter[1];
  int (*InvLevelScale8x8_Inter_2)[8][8] = curSlice->InvLevelScale8x8_Inter[2];

  for (int k = 0; k < 6; k++) {
    setDequant8x8 (*InvLevelScale8x8_Intra_0++, *p_dequant_coef  , curSlice->qmatrix[6]);
    setDequant8x8 (*InvLevelScale8x8_Inter_0++, *p_dequant_coef++, curSlice->qmatrix[7]);
    }

  p_dequant_coef = dequant_coef8;
  if (curSlice->active_sps->chroma_format_idc == 3) {
    // 4:4:4
    for (int k = 0; k < 6; k++) {
      setDequant8x8 (*InvLevelScale8x8_Intra_1++, *p_dequant_coef  , curSlice->qmatrix[8]);
      setDequant8x8 (*InvLevelScale8x8_Inter_1++, *p_dequant_coef  , curSlice->qmatrix[9]);
      setDequant8x8 (*InvLevelScale8x8_Intra_2++, *p_dequant_coef  , curSlice->qmatrix[10]);
      setDequant8x8 (*InvLevelScale8x8_Inter_2++, *p_dequant_coef++, curSlice->qmatrix[11]);
      }
    }
  }
//}}}

//{{{
void allocQuant (sCodingParam* codingParam) {

  int bitdepth_qp_scale = imax (codingParam->bitdepth_luma_qp_scale, codingParam->bitdepth_chroma_qp_scale);

  // We should allocate memory outside of this process since maybe we will have a change of SPS
  // and we may need to recreate these. Currently should only support same bitdepth
  if (codingParam->qp_per_matrix == NULL)
    if ((codingParam->qp_per_matrix = (int*)malloc((MAX_QP + 1 + bitdepth_qp_scale)*sizeof(int))) == NULL)
      no_mem_exit ("init_qp_process: codingParam->qp_per_matrix");

  if (codingParam->qp_rem_matrix == NULL)
    if ((codingParam->qp_rem_matrix = (int*)malloc((MAX_QP + 1 + bitdepth_qp_scale)*sizeof(int))) == NULL)
      no_mem_exit ("init_qp_process: codingParam->qp_rem_matrix");

  for (int i = 0; i < MAX_QP + bitdepth_qp_scale + 1; i++) {
    codingParam->qp_per_matrix[i] = i / 6;
    codingParam->qp_rem_matrix[i] = i % 6;
    }
  }
//}}}
//{{{
void freeQuant (sCodingParam* codingParam) {

  if (codingParam->qp_per_matrix != NULL) {
    free (codingParam->qp_per_matrix);
    codingParam->qp_per_matrix = NULL;
    }

  if (codingParam->qp_rem_matrix != NULL) {
    free (codingParam->qp_rem_matrix);
    codingParam->qp_rem_matrix = NULL;
    }
  }
//}}}

//{{{
void assignQuantParams (sSlice* curSlice) {

  sSPS* sps = curSlice->active_sps;
  sPPS* pps = curSlice->active_pps;

  if (!pps->pic_scaling_matrix_present_flag && !sps->seq_scaling_matrix_present_flag) {
    for (int i = 0; i < 12; i++)
      curSlice->qmatrix[i] = (i < 6) ? quant_org : quant8_org;
    }
  else {
    int n_ScalingList = (sps->chroma_format_idc != YUV444) ? 8 : 12;
    if (sps->seq_scaling_matrix_present_flag) {
      //{{{  check sps first
      for (int i = 0; i < n_ScalingList; i++) {
        if (i < 6) {
          if (!sps->seq_scaling_list_present_flag[i]) {
            // fall-back rule A
            if (i == 0)
              curSlice->qmatrix[i] = quant_intra_default;
            else if (i == 3)
              curSlice->qmatrix[i] = quant_inter_default;
            else
              curSlice->qmatrix[i] = curSlice->qmatrix[i-1];
            }
          else {
            if (sps->UseDefaultScalingMatrix4x4Flag[i])
              curSlice->qmatrix[i] = (i<3) ? quant_intra_default : quant_inter_default;
            else
              curSlice->qmatrix[i] = sps->ScalingList4x4[i];
          }
        }
        else {
          if (!sps->seq_scaling_list_present_flag[i]) {
            // fall-back rule A
            if (i == 6)
              curSlice->qmatrix[i] = quant8_intra_default;
            else if (i == 7)
              curSlice->qmatrix[i] = quant8_inter_default;
            else
              curSlice->qmatrix[i] = curSlice->qmatrix[i-2];
            }
          else {
            if (sps->UseDefaultScalingMatrix8x8Flag[i-6])
              curSlice->qmatrix[i] = (i==6 || i==8 || i==10) ? quant8_intra_default:quant8_inter_default;
            else
              curSlice->qmatrix[i] = sps->ScalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    if (pps->pic_scaling_matrix_present_flag) {
      //{{{  then check pps
      for (int i = 0; i < n_ScalingList; i++) {
        if (i < 6) {
          if (!pps->pic_scaling_list_present_flag[i]) {
            // fall-back rule B
            if (i == 0) {
              if (!sps->seq_scaling_matrix_present_flag)
                curSlice->qmatrix[i] = quant_intra_default;
              }
            else if (i == 3) {
              if (!sps->seq_scaling_matrix_present_flag)
                curSlice->qmatrix[i] = quant_inter_default;
              }
            else
              curSlice->qmatrix[i] = curSlice->qmatrix[i-1];
            }
          else {
            if (pps->UseDefaultScalingMatrix4x4Flag[i])
              curSlice->qmatrix[i] = (i<3) ? quant_intra_default:quant_inter_default;
            else
              curSlice->qmatrix[i] = pps->ScalingList4x4[i];
            }
          }
        else {
          if (!pps->pic_scaling_list_present_flag[i]) {
            // fall-back rule B
            if (i == 6) {
              if (!sps->seq_scaling_matrix_present_flag)
                curSlice->qmatrix[i] = quant8_intra_default;
              }
            else if (i == 7) {
              if (!sps->seq_scaling_matrix_present_flag)
                curSlice->qmatrix[i] = quant8_inter_default;
              }
            else
              curSlice->qmatrix[i] = curSlice->qmatrix[i-2];
            }
          else {
            if (pps->UseDefaultScalingMatrix8x8Flag[i-6])
              curSlice->qmatrix[i] = (i==6 || i==8 || i==10) ? quant8_intra_default:quant8_inter_default;
            else
              curSlice->qmatrix[i] = pps->ScalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    }

  CalculateQuant4x4Param (curSlice);
  if (pps->transform_8x8_mode_flag)
    CalculateQuant8x8Param (curSlice);
  }
//}}}
