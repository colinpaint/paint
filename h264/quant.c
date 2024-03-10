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
  if (curSlice->activeSPS->chromaFormatIdc == 3) {
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
void allocQuant (sCoding* codingParam) {

  int bitdepth_qp_scale = imax (codingParam->bitdepth_luma_qp_scale, codingParam->bitdepthChromaQpScale);

  // We should allocate memory outside of this process since maybe we will have a change of SPS
  // and we may need to recreate these. Currently should only support same bitdepth
  if (codingParam->qpPerMatrix == NULL)
    if ((codingParam->qpPerMatrix = (int*)malloc((MAX_QP + 1 + bitdepth_qp_scale)*sizeof(int))) == NULL)
      no_mem_exit ("init_qp_process: codingParam->qpPerMatrix");

  if (codingParam->qpRemMatrix == NULL)
    if ((codingParam->qpRemMatrix = (int*)malloc((MAX_QP + 1 + bitdepth_qp_scale)*sizeof(int))) == NULL)
      no_mem_exit ("init_qp_process: codingParam->qpRemMatrix");

  for (int i = 0; i < MAX_QP + bitdepth_qp_scale + 1; i++) {
    codingParam->qpPerMatrix[i] = i / 6;
    codingParam->qpRemMatrix[i] = i % 6;
    }
  }
//}}}
//{{{
void freeQuant (sCoding* codingParam) {

  if (codingParam->qpPerMatrix != NULL) {
    free (codingParam->qpPerMatrix);
    codingParam->qpPerMatrix = NULL;
    }

  if (codingParam->qpRemMatrix != NULL) {
    free (codingParam->qpRemMatrix);
    codingParam->qpRemMatrix = NULL;
    }
  }
//}}}

//{{{
void assignQuantParams (sSlice* curSlice) {

  sSPS* sps = curSlice->activeSPS;
  sPPS* pps = curSlice->activePPS;

  if (!pps->picScalingMatrixPresentFlag && !sps->seq_scaling_matrix_present_flag) {
    for (int i = 0; i < 12; i++)
      curSlice->qmatrix[i] = (i < 6) ? quant_org : quant8_org;
    }
  else {
    int n_ScalingList = (sps->chromaFormatIdc != YUV444) ? 8 : 12;
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
            if (sps->useDefaultScalingMatrix4x4Flag[i])
              curSlice->qmatrix[i] = (i<3) ? quant_intra_default : quant_inter_default;
            else
              curSlice->qmatrix[i] = sps->scalingList4x4[i];
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
            if (sps->useDefaultScalingMatrix8x8Flag[i-6])
              curSlice->qmatrix[i] = (i==6 || i==8 || i==10) ? quant8_intra_default:quant8_inter_default;
            else
              curSlice->qmatrix[i] = sps->scalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    if (pps->picScalingMatrixPresentFlag) {
      //{{{  then check pps
      for (int i = 0; i < n_ScalingList; i++) {
        if (i < 6) {
          if (!pps->picScalingListPresentFlag[i]) {
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
            if (pps->useDefaultScalingMatrix4x4Flag[i])
              curSlice->qmatrix[i] = (i<3) ? quant_intra_default:quant_inter_default;
            else
              curSlice->qmatrix[i] = pps->scalingList4x4[i];
            }
          }
        else {
          if (!pps->picScalingListPresentFlag[i]) {
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
            if (pps->useDefaultScalingMatrix8x8Flag[i-6])
              curSlice->qmatrix[i] = (i==6 || i==8 || i==10) ? quant8_intra_default:quant8_inter_default;
            else
              curSlice->qmatrix[i] = pps->scalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    }

  CalculateQuant4x4Param (curSlice);
  if (pps->transform8x8modeFlag)
    CalculateQuant8x8Param (curSlice);
  }
//}}}
