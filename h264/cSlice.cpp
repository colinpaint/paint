//{{{  includes
#include "global.h"
#include "memory.h"

#include "buffer.h"
#include "cabac.h"
#include "fmo.h"
#include "macroblock.h"
#include "mcPred.h"

#include "../common/cLog.h"

using namespace std;
//}}}

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
            if (!td || listX[LIST_1][j]->isLongTerm || listX[LIST_0][i]->isLongTerm) {
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
                if (!td || listX[k+LIST_1][j]->isLongTerm || listX[k+LIST_0][i]->isLongTerm) {
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
