//{{{  includes
#include "global.h"
#include "memory.h"

#include "block.h"
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

  slice->maxDataPartitions = 3;  // assume dataPartition worst case
  slice->dataPartitions = allocDataPartitions (slice->maxDataPartitions);

  getMem3Dint (&slice->weightedPredWeight, 2, MAX_REFERENCE_PICTURES, 3);
  getMem3Dint (&slice->weightedPredOffset, 6, MAX_REFERENCE_PICTURES, 3);
  getMem4Dint (&slice->weightedBiPredWeight, 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);
  getMem3Dpel (&slice->mbPred, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dpel (&slice->mbRec, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&slice->mbRess, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&slice->cof, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  allocPred (slice);

  // reference flag initialization
  for (int i = 0; i < 17; ++i)
    slice->refFlag[i] = 1;

  for (int i = 0; i < 6; i++) {
    slice->listX[i] = (sPicture**)calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering
    if (!slice->listX[i])
      noMemoryExit ("allocSlice - listX[i]");
    }

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
    freeRefPicListReorderBuffer (this);

  freePred (this);
  freeMem3Dint (cof);
  freeMem3Dint (mbRess);
  freeMem3Dpel (mbRec);
  freeMem3Dpel (mbPred);

  freeMem3Dint (weightedPredWeight);
  freeMem3Dint (weightedPredOffset);
  freeMem4Dint (weightedBiPredWeight);

  freeDataPartitions (dataPartitions, 3);

  // delete all context models
  deleteMotionInfoContexts (motionInfoContexts);
  deleteTextureInfoContexts (textureInfoContexts);

  for (int i = 0; i<6; i++) {
    if (listX[i]) {
      free (listX[i]);
      listX[i] = NULL;
      }
    }

  while (decRefPicMarkBuffer) {
    sDecodedRefPicMark* tempDrpm = decRefPicMarkBuffer;
    decRefPicMarkBuffer = tempDrpm->next;
    free (tempDrpm);
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
              if (weightedBiPredWeight[1][i][j][comp] < -64 || weightedBiPredWeight[1][i][j][comp] > 128) {
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
