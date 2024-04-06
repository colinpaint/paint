//{{{  includes
#include <limits.h>

#include "global.h"
#include "memory.h"

#include "erc.h"
#include "buffer.h"
#include "output.h"
//}}}
namespace {
  //{{{
  void freePicMotion (sPicMotionOld* motion) {

    free (motion->mbField);
    motion->mbField = NULL;
    }
  //}}}
  }

//{{{
cFrameStore* cFrameStore::allocFrameStore() {

  cFrameStore* fs = new cFrameStore();
  return fs;
  }
//}}}
//{{{
void cFrameStore::freeFrameStore() {

  freePicture (frame);
  freePicture (topField);
  freePicture (botField);
  }
//}}}

//{{{
int cFrameStore::isReference() {

  if (mIsReference)
    return 1;

  if (isUsed == 3) // frame
    if (frame->usedForReference)
      return 1;

  if (isUsed & 1) // topField
    if (topField)
      if (topField->usedForReference)
        return 1;

  if (isUsed & 2) // botField
    if (botField)
      if (botField->usedForReference)
        return 1;

  return 0;
  }
//}}}
//{{{
int cFrameStore::isShortTermReference() {

  if (isUsed == 3) // frame
    if ((frame->usedForReference) && (!frame->isLongTerm))
      return 1;

  if (isUsed & 1) // topField
    if (topField)
      if ((topField->usedForReference) && (!topField->isLongTerm))
        return 1;

  if (isUsed & 2) // botField
    if (botField)
      if ((botField->usedForReference) && (!botField->isLongTerm))
        return 1;

  return 0;
  }
//}}}
//{{{
int cFrameStore::isLongTermReference() {

  if (isUsed == 3) // frame
    if ((frame->usedForReference) && (frame->isLongTerm))
      return 1;

  if (isUsed & 1) // topField
    if (topField)
      if ((topField->usedForReference) && (topField->isLongTerm))
        return 1;

  if (isUsed & 2) // botField
    if (botField)
      if ((botField->usedForReference) && (botField->isLongTerm))
        return 1;

  return 0;
  }
//}}}

//{{{
void cFrameStore::unmarkForRef() {

  if (isUsed & 1)
    if (topField)
      topField->usedForReference = 0;

  if (isUsed & 2)
    if (botField)
      botField->usedForReference = 0;

  if (isUsed == 3) {
    if (topField && botField) {
      topField->usedForReference = 0;
      botField->usedForReference = 0;
      }
    frame->usedForReference = 0;
    }

  mIsReference = 0;

  if (frame)
    freePicMotion (&frame->motion);

  if (topField)
    freePicMotion (&topField->motion);

  if (botField)
    freePicMotion (&botField->motion);
  }
//}}}
//{{{
void cFrameStore::unmarkForLongTermRef() {

  if (isUsed & 1) {
    if (topField) {
      topField->usedForReference = 0;
      topField->isLongTerm = 0;
      }
    }

  if (isUsed & 2) {
    if (botField) {
      botField->usedForReference = 0;
      botField->isLongTerm = 0;
      }
    }

  if (isUsed == 3) {
    if (topField && botField) {
      topField->usedForReference = 0;
      topField->isLongTerm = 0;
      botField->usedForReference = 0;
      botField->isLongTerm = 0;
      }
    frame->usedForReference = 0;
    frame->isLongTerm = 0;
    }

  mIsReference = 0;
  isLongTerm = 0;
  }
//}}}

//{{{
void cFrameStore::dpbCombineField (cDecoder264* decoder) {

  if (!frame)
    frame = allocPicture (decoder, eFrame, 
                          topField->sizeX, topField->sizeY*2,
                          topField->sizeXcr, topField->sizeYcr*2, 1);

  for (int i = 0; i < topField->sizeY; i++) {
    memcpy (frame->imgY[i*2], topField->imgY[i], topField->sizeX * sizeof(sPixel)); // top field
    memcpy (frame->imgY[i*2 + 1], botField->imgY[i], botField->sizeX * sizeof(sPixel)); // bottom field
    }

  for (int j = 0; j < 2; j++)
    for (int i = 0; i < topField->sizeYcr; i++) {
      memcpy (frame->imgUV[j][i*2], topField->imgUV[j][i], topField->sizeXcr*sizeof(sPixel));
      memcpy (frame->imgUV[j][i*2 + 1], botField->imgUV[j][i], botField->sizeXcr*sizeof(sPixel));
      }

  poc = frame->poc = frame->framePoc = imin (topField->poc, botField->poc);
  botField->framePoc = topField->framePoc = frame->poc;
  botField->topPoc = frame->topPoc = topField->poc;
  topField->botPoc = frame->botPoc = botField->poc;

  frame->usedForReference = (topField->usedForReference && botField->usedForReference );
  frame->isLongTerm = (topField->isLongTerm && botField->isLongTerm );

  if (frame->isLongTerm)
    frame->longTermFrameIndex = longTermFrameIndex;

  frame->topField = topField;
  frame->botField = botField;
  frame->frame = frame;
  frame->codedFrame = 0;

  frame->chromaFormatIdc = topField->chromaFormatIdc;
  frame->hasCrop = topField->hasCrop;
  if (frame->hasCrop) {
    frame->cropTop = topField->cropTop;
    frame->cropBot = topField->cropBot;
    frame->cropLeft = topField->cropLeft;
    frame->cropRight = topField->cropRight;
    }

  topField->frame = botField->frame = frame;
  topField->topField = topField;
  topField->botField = botField;
  botField->topField = topField;
  botField->botField = botField;
  if (topField->usedForReference || botField->usedForReference)
    decoder->padPicture (frame);
  }
//}}}
