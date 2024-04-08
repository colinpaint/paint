//{{{  includes
#include "global.h"
#include "memory.h"

#include "cabac.h"
#include "fmo.h"
#include "macroblock.h"
#include "mcPred.h"

#include "../common/cLog.h"

using namespace std;
//}}}

namespace {
  //{{{
  int quant_intra_default[16] = {
     6,13,20,28,
    13,20,28,32,
    20,28,32,37,
    28,32,37,42
  };
  //}}}
  //{{{
  int quant_inter_default[16] = {
    10,14,20,24,
    14,20,24,27,
    20,24,27,30,
    24,27,30,34
  };
  //}}}
  //{{{
  int quant8_intra_default[64] = {
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
  int quant8_inter_default[64] = {
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
  int quant_org[16] = {
  16,16,16,16,
  16,16,16,16,
  16,16,16,16,
  16,16,16,16
  };
  //}}}
  //{{{
  int quant8_org[64] = {
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
  void setDequant4x4 (int (*InvLevelScale4x4)[4], const int (*dequant)[4], int* qmatrix) {

    for (int j = 0; j < 4; j++) {
      *(*InvLevelScale4x4      ) = *(*dequant      ) * *qmatrix++;
      *(*InvLevelScale4x4   + 1) = *(*dequant   + 1) * *qmatrix++;
      *(*InvLevelScale4x4   + 2) = *(*dequant   + 2) * *qmatrix++;
      *(*InvLevelScale4x4++ + 3) = *(*dequant++ + 3) * *qmatrix++;
      }
    }
  //}}}
  //{{{
  void setDequant8x8 (int (*InvLevelScale8x8)[8], const int (*dequant)[8], int* qmatrix) {

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
  void calculateQuant4x4Param (cSlice* slice) {

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
  void calculateQuant8x8Param (cSlice* slice) {

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
  void reorderShortTerm (cSlice* slice, int curList, int numRefIndexIXactiveMinus1, int picNumLX, int *refIdxLX) {

    sPicture** refPicListX = slice->listX[curList];
    sPicture* picLX = slice->dpb->getShortTermPic (slice, picNumLX);

    for (int cIdx = numRefIndexIXactiveMinus1+1; cIdx > *refIdxLX; cIdx--)
      refPicListX[cIdx] = refPicListX[cIdx - 1];
    refPicListX[(*refIdxLX)++] = picLX;

    int nIdx = *refIdxLX;
    for (int cIdx = *refIdxLX; cIdx <= numRefIndexIXactiveMinus1+1; cIdx++)
      if (refPicListX[cIdx])
        if ((refPicListX[cIdx]->usedLongTerm) || (refPicListX[cIdx]->picNum != picNumLX))
          refPicListX[nIdx++] = refPicListX[cIdx];
    }
  //}}}
  //{{{
  void reorderLongTerm (cSlice* slice, sPicture** refPicListX,
                        int numRefIndexIXactiveMinus1, int LongTermPicNum, int *refIdxLX) {

    sPicture* picLX = slice->dpb->getLongTermPic (slice, LongTermPicNum);

    for (int cIdx = numRefIndexIXactiveMinus1+1; cIdx > *refIdxLX; cIdx--)
      refPicListX[cIdx] = refPicListX[cIdx - 1];
    refPicListX[(*refIdxLX)++] = picLX;

    int nIdx = *refIdxLX;
    for (int cIdx = *refIdxLX; cIdx <= numRefIndexIXactiveMinus1+1; cIdx++)
      if (refPicListX[cIdx])
        if ((!refPicListX[cIdx]->usedLongTerm) || (refPicListX[cIdx]->longTermPicNum != LongTermPicNum))
          refPicListX[nIdx++] = refPicListX[cIdx];
    }
  //}}}
  }

//{{{
cSlice* cSlice::allocSlice() {

  cSlice* slice = new cSlice();
  memset (slice, 0, sizeof(cSlice));

  // create all context models
  slice->motionInfoContexts = createMotionInfoContexts();
  slice->textureInfoContexts = createTextureInfoContexts();

  slice->maxDataPartitions = 3;
  slice->dataPartitions = allocDataPartitions (slice->maxDataPartitions);

  getMem3Dint (&slice->weightedPredWeight, 2, MAX_REFERENCE_PICTURES, 3);
  getMem3Dint (&slice->weightedPredOffset, 6, MAX_REFERENCE_PICTURES, 3);
  getMem4Dint (&slice->weightedBiPredWeight, 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);
  getMem3Dpel (&slice->mbPred, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dpel (&slice->mbRec, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&slice->mbRess, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&slice->cof, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

  getMem2Dpel (&slice->tempBlockL0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dpel (&slice->tempBlockL1, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dpel (&slice->tempBlockL2, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dpel (&slice->tempBlockL3, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dint (&slice->tempRes, MB_BLOCK_SIZE + 5, MB_BLOCK_SIZE + 5);

  // reference flag initialization
  for (int i = 0; i < 17; ++i)
    slice->refFlag[i] = 1;

  for (int i = 0; i < 6; i++)
    slice->listX[i] = (sPicture**)calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering

  for (int j = 0; j < 6; j++) {
    for (int i = 0; i < MAX_LIST_SIZE; i++)
      slice->listX[j][i] = NULL;
    slice->listXsize[j]=0;
    }

  return slice;
  }
//}}}
//{{{
cSlice::~cSlice() {

  if (sliceType != eSliceI && sliceType != eSliceSI)
    freeRefPicListReorderBuffer();

  freeMem2Dint (tempRes);
  freeMem2Dpel (tempBlockL0);
  freeMem2Dpel (tempBlockL1);
  freeMem2Dpel (tempBlockL2);
  freeMem2Dpel (tempBlockL3);

  freeMem3Dint (cof);
  freeMem3Dint (mbRess);
  freeMem3Dpel (mbRec);
  freeMem3Dpel (mbPred);

  freeMem3Dint (weightedPredWeight);
  freeMem3Dint (weightedPredOffset);
  freeMem4Dint (weightedBiPredWeight);

  freeDataPartitions (dataPartitions, 3);

  deleteMotionInfoContexts (motionInfoContexts);
  deleteTextureInfoContexts (textureInfoContexts);

  for (int i = 0; i < 6; i++) {
    if (listX[i]) {
      free (listX[i]);
      listX[i] = NULL;
      }
    }

  while (decRefPicMarkBuffer) {
    sDecodedRefPicMark* tempDecodedRefPicMark = decRefPicMarkBuffer;
    decRefPicMarkBuffer = tempDecodedRefPicMark->next;
    free (tempDecodedRefPicMark);
    }
  }
//}}}

//{{{
void cSlice::fillWeightedPredParam() {

  if (sliceType == eSliceB) {
    int maxL0Ref = numRefIndexActive[LIST_0];
    int maxL1Ref = numRefIndexActive[LIST_1];
    if (activePps->weightedBiPredIdc == 2) {
      lumaLog2weightDenom = 5;
      chromaLog2weightDenom = 5;
      wpRoundLuma = 16;
      wpRoundChroma = 16;
      for (int i = 0; i < MAX_REFERENCE_PICTURES; ++i)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? lumaLog2weightDenom : chromaLog2weightDenom;
          weightedPredWeight[0][i][comp] = 1 << logWeightDenom;
          weightedPredWeight[1][i][comp] = 1 << logWeightDenom;
          weightedPredOffset[0][i][comp] = 0;
          weightedPredOffset[1][i][comp] = 0;
          }
      }

    for (int i = 0; i < maxL0Ref; ++i)
      for (int j = 0; j < maxL1Ref; ++j)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? lumaLog2weightDenom : chromaLog2weightDenom;
          if (activePps->weightedBiPredIdc == 1) {
            weightedBiPredWeight[0][i][j][comp] = weightedPredWeight[0][i][comp];
            weightedBiPredWeight[1][i][j][comp] = weightedPredWeight[1][j][comp];
            }
          else if (activePps->weightedBiPredIdc == 2) {
            int td = iClip3(-128,127,listX[LIST_1][j]->poc - listX[LIST_0][i]->poc);
            if (!td || listX[LIST_1][j]->usedLongTerm || listX[LIST_0][i]->usedLongTerm) {
              weightedBiPredWeight[0][i][j][comp] = 32;
              weightedBiPredWeight[1][i][j][comp] = 32;
              }
            else {
              int tb = iClip3(-128, 127, thisPoc - listX[LIST_0][i]->poc);
              int tx = (16384 + iabs (td / 2)) / td;
              int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
              weightedBiPredWeight[1][i][j][comp] = distScaleFactor >> 2;
              weightedBiPredWeight[0][i][j][comp] = 64 - weightedBiPredWeight[1][i][j][comp];
              if (weightedBiPredWeight[1][i][j][comp] < -64 ||
                  weightedBiPredWeight[1][i][j][comp] > 128) {
                weightedBiPredWeight[0][i][j][comp] = 32;
                weightedBiPredWeight[1][i][j][comp] = 32;
                weightedPredOffset[0][i][comp] = 0;
                weightedPredOffset[1][j][comp] = 0;
                }
              }
            }
          }

    if (mbAffFrame)
      for (int i = 0; i < 2 * maxL0Ref; ++i)
        for (int j = 0; j < 2 * maxL1Ref; ++j)
          for (int comp = 0; comp<3; ++comp)
            for (int k = 2; k < 6; k += 2) {
              weightedPredOffset[k+0][i][comp] = weightedPredOffset[0][i>>1][comp];
              weightedPredOffset[k+1][j][comp] = weightedPredOffset[1][j>>1][comp];
              int logWeightDenom = !comp ? lumaLog2weightDenom : chromaLog2weightDenom;
              if (activePps->weightedBiPredIdc == 1) {
                weightedBiPredWeight[k+0][i][j][comp] = weightedPredWeight[0][i>>1][comp];
                weightedBiPredWeight[k+1][i][j][comp] = weightedPredWeight[1][j>>1][comp];
                }
              else if (activePps->weightedBiPredIdc == 2) {
                int td = iClip3 (-128, 127, listX[k+LIST_1][j]->poc - listX[k+LIST_0][i]->poc);
                if (!td || listX[k+LIST_1][j]->usedLongTerm || listX[k+LIST_0][i]->usedLongTerm) {
                  weightedBiPredWeight[k+0][i][j][comp] = 32;
                  weightedBiPredWeight[k+1][i][j][comp] = 32;
                  }
                else {
                  int tb = iClip3 (-128,127,
                               ((k == 2) ? topPoc : botPoc) - listX[k+LIST_0][i]->poc);
                  int tx = (16384 + iabs(td/2)) / td;
                  int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
                  weightedBiPredWeight[k+1][i][j][comp] = distScaleFactor >> 2;
                  weightedBiPredWeight[k+0][i][j][comp] = 64 - weightedBiPredWeight[k+1][i][j][comp];
                  if (weightedBiPredWeight[k+1][i][j][comp] < -64 ||
                      weightedBiPredWeight[k+1][i][j][comp] > 128) {
                    weightedBiPredWeight[k+1][i][j][comp] = 32;
                    weightedBiPredWeight[k+0][i][j][comp] = 32;
                    weightedPredOffset[k+0][i][comp] = 0;
                    weightedPredOffset[k+1][j][comp] = 0;
                    }
                  }
                }
              }
    }
  }
//}}}
//{{{
void cSlice::resetWeightedPredParam() {

  for (int i = 0; i < MAX_REFERENCE_PICTURES; i++) {
    for (int comp = 0; comp < 3; comp++) {
      int logWeightDenom = (comp == 0) ? lumaLog2weightDenom : chromaLog2weightDenom;
      weightedPredWeight[0][i][comp] = 1 << logWeightDenom;
      weightedPredWeight[1][i][comp] = 1 << logWeightDenom;
      }
    }
  }
//}}}

//{{{
void cSlice::initMbAffLists (sPicture* noReferencePicture) {
// Initialize listX[2..5] from lists 0 and 1
//  listX[2]: list0 for current_field == top
//  listX[3]: list1 for current_field == top
//  listX[4]: list0 for current_field == bottom
//  listX[5]: list1 for current_field == bottom

  for (int i = 2; i < 6; i++) {
    for (uint32_t j = 0; j < MAX_LIST_SIZE; j++)
      listX[i][j] = noReferencePicture;
    listXsize[i] = 0;
    }

  for (int i = 0; i < listXsize[0]; i++) {
    listX[2][2*i] = listX[0][i]->topField;
    listX[2][2*i+1] = listX[0][i]->botField;
    listX[4][2*i] = listX[0][i]->botField;
    listX[4][2*i+1] = listX[0][i]->topField;
    }
  listXsize[2] = listXsize[4] = listXsize[0] * 2;

  for (int i = 0; i < listXsize[1]; i++) {
    listX[3][2*i] = listX[1][i]->topField;
    listX[3][2*i+1] = listX[1][i]->botField;
    listX[5][2*i] = listX[1][i]->botField;
    listX[5][2*i+1] = listX[1][i]->topField;
    }
  listXsize[3] = listXsize[5] = listXsize[1] * 2;
  }
//}}}
//{{{
void cSlice::copyPoc (cSlice* toSlice) {

  toSlice->topPoc = topPoc;
  toSlice->botPoc = botPoc;
  toSlice->thisPoc = thisPoc;
  toSlice->framePoc = framePoc;
  }
//}}}

//{{{
void cSlice::allocRefPicListReordeBuffer() {

  if ((sliceType != eSliceI) && (sliceType != eSliceSI)) {
    // B,P
    int size = numRefIndexActive[LIST_0] + 1;
    modPicNumsIdc[LIST_0] = (int*)calloc (size ,sizeof(int));
    longTermPicIndex[LIST_0] = (int*)calloc (size,sizeof(int));
    absDiffPicNumMinus1[LIST_0] = (int*)calloc (size,sizeof(int));
    }
  else {
    modPicNumsIdc[LIST_0] = NULL;
    longTermPicIndex[LIST_0] = NULL;
    absDiffPicNumMinus1[LIST_0] = NULL;
    }

  if (sliceType == eSliceB) {
    // B
    int size = numRefIndexActive[LIST_1] + 1;
    modPicNumsIdc[LIST_1] = (int*)calloc (size,sizeof(int));
    longTermPicIndex[LIST_1] = (int*)calloc (size,sizeof(int));
    absDiffPicNumMinus1[LIST_1] = (int*)calloc (size,sizeof(int));
    }
  else {
    modPicNumsIdc[LIST_1] = NULL;
    longTermPicIndex[LIST_1] = NULL;
    absDiffPicNumMinus1[LIST_1] = NULL;
    }
  }
//}}}
//{{{
void cSlice::freeRefPicListReorderBuffer() {

  if (modPicNumsIdc[LIST_0])
    free (modPicNumsIdc[LIST_0]);
  if (absDiffPicNumMinus1[LIST_0])
    free (absDiffPicNumMinus1[LIST_0]);
  if (longTermPicIndex[LIST_0])
    free (longTermPicIndex[LIST_0]);

  modPicNumsIdc[LIST_0] = NULL;
  absDiffPicNumMinus1[LIST_0] = NULL;
  longTermPicIndex[LIST_0] = NULL;

  if (modPicNumsIdc[LIST_1])
    free (modPicNumsIdc[LIST_1]);
  if (absDiffPicNumMinus1[LIST_1])
    free (absDiffPicNumMinus1[LIST_1]);
  if (longTermPicIndex[LIST_1])
    free (longTermPicIndex[LIST_1]);

  modPicNumsIdc[LIST_1] = NULL;
  absDiffPicNumMinus1[LIST_1] = NULL;
  longTermPicIndex[LIST_1] = NULL;
  }
//}}}

//{{{
void cSlice::reorderRefPicList (int curList) {

  int* curModPicNumsIdc = modPicNumsIdc[curList];
  int* curAbsDiffPicNumMinus1 = absDiffPicNumMinus1[curList];
  int* curLongTermPicIndex = longTermPicIndex[curList];
  int numRefIndexIXactiveMinus1 = numRefIndexActive[curList] - 1;

  int maxPicNum;
  int currPicNum;
  if (picStructure == eFrame) {
    maxPicNum = decoder->coding.maxFrameNum;
    currPicNum = frameNum;
    }
  else {
    maxPicNum = 2 * decoder->coding.maxFrameNum;
    currPicNum = 2 * frameNum + 1;
    }

  int picNumLX;
  int picNumLXNoWrap;
  int picNumLXPred = currPicNum;
  int refIdxLX = 0;
  for (int i = 0; curModPicNumsIdc[i] != 3; i++) {
    if (curModPicNumsIdc[i] > 3)
      cDecoder264::error ("Invalid modPicNumsIdc command");
    if (curModPicNumsIdc[i] < 2) {
      if (curModPicNumsIdc[i] == 0) {
        if (picNumLXPred - (curAbsDiffPicNumMinus1[i]+1) < 0)
          picNumLXNoWrap = picNumLXPred - (curAbsDiffPicNumMinus1[i]+1) + maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred - (curAbsDiffPicNumMinus1[i]+1);
        }
      else {
        if (picNumLXPred + curAbsDiffPicNumMinus1[i]+1 >= maxPicNum)
          picNumLXNoWrap = picNumLXPred + (curAbsDiffPicNumMinus1[i]+1) - maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred + (curAbsDiffPicNumMinus1[i]+1);
        }

      picNumLXPred = picNumLXNoWrap;
      if (picNumLXNoWrap > currPicNum)
        picNumLX = picNumLXNoWrap - maxPicNum;
      else
        picNumLX = picNumLXNoWrap;

      reorderShortTerm (this, curList, numRefIndexIXactiveMinus1, picNumLX, &refIdxLX);
      }
    else
      reorderLongTerm (this, listX[curList], numRefIndexIXactiveMinus1, curLongTermPicIndex[i], &refIdxLX);
    }

  listXsize[curList] = (char)(numRefIndexIXactiveMinus1 + 1);
  }
//}}}

//{{{
void useQuantParams (cSlice* slice) {

  cSps* sps = slice->activeSps;
  cPps* pps = slice->activePps;

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

  calculateQuant4x4Param (slice);
  if (pps->hasTransform8x8mode)
    calculateQuant8x8Param (slice);
  }
//}}}
