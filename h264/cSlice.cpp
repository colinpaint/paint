//{{{  includes
#include "global.h"
#include "memory.h"

#include "block.h"
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
      noMemoryExit ("allocSlice - slice->listX[i]");
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
