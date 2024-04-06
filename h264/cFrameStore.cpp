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
