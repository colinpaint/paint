//{{{  includes
#include <limits.h>

#include "global.h"
#include "memory.h"

#include "erc.h"
  #include "../common/cLog.h"

using namespace std;

//}}}
namespace {
  //{{{
  int getDpbSize (cDecoder264* decoder) {

    int pic_size_mb = (decoder->activeSps->picWidthMbsMinus1 + 1) *
                      (decoder->activeSps->picHeightMapUnitsMinus1 + 1) *
                      (decoder->activeSps->frameMbOnly ? 1 : 2);
    int size = 0;

    switch (decoder->activeSps->levelIdc) {
      //{{{
      case 0:
        // if there is no level defined, we expect experimental usage and return a DPB size of 16
        return 16;
      //}}}
      //{{{
      case 9:
        size = 396;
        break;
      //}}}
      //{{{
      case 10:
        size = 396;
        break;
      //}}}
      //{{{
      case 11:
        if (!decoder->activeSps->isFrextProfile() && (decoder->activeSps->constrainedSet3flag == 1))
          size = 396;
        else
          size = 900;
        break;
      //}}}
      //{{{
      case 12:
        size = 2376;
        break;
      //}}}
      //{{{
      case 13:
        size = 2376;
        break;
      //}}}
      //{{{
      case 20:
        size = 2376;
        break;
      //}}}
      //{{{
      case 21:
        size = 4752;
        break;
      //}}}
      //{{{
      case 22:
        size = 8100;
        break;
      //}}}
      //{{{
      case 30:
        size = 8100;
        break;
      //}}}
      //{{{
      case 31:
        size = 18000;
        break;
      //}}}
      //{{{
      case 32:
        size = 20480;
        break;
      //}}}
      //{{{
      case 40:
        size = 32768;
        break;
      //}}}
      //{{{
      case 41:
        size = 32768;
        break;
      //}}}
      //{{{
      case 42:
        size = 34816;
        break;
      //}}}
      //{{{
      case 50:
        size = 110400;
        break;
      //}}}
      //{{{
      case 51:
        size = 184320;
        break;
      //}}}
      //{{{
      case 52:
        size = 184320;
        break;
      //}}}
      case 60:
      case 61:
      //{{{
      case 62:
        size = 696320;
        break;
      //}}}
      //{{{
      default:
        cDecoder264::error ("undefined level");
        break;
      //}}}
      }

    size /= pic_size_mb;
    size = imin( size, 16);

    if (decoder->activeSps->hasVui && decoder->activeSps->vuiSeqParams.bitstream_restrictionFlag) {
      int size_vui;
      if ((int)decoder->activeSps->vuiSeqParams.max_dec_frame_buffering > size)
        cDecoder264::error ("max_dec_frame_buffering larger than MaxDpbSize");

      size_vui = imax (1, decoder->activeSps->vuiSeqParams.max_dec_frame_buffering);
      if (size_vui < size)
        cLog::log (LOGERROR, fmt::format ("max_dec_frame_buffering:{} less than DPB size:{}", size_vui, size));
      size = size_vui;
      }

    return size;
    }
  //}}}
  //{{{
  int getPicNumX (sPicture* p, int diffPicNumMinus1) {

    int curPicNum;
    if (p->picStructure == eFrame)
      curPicNum = p->frameNum;
    else
      curPicNum = 2 * p->frameNum + 1;

    return curPicNum - (diffPicNumMinus1 + 1);
    }
  //}}}
  }

// dpb
//{{{
void cDpb::initDpb (cDecoder264* decoder, int type) {

  if (initDone)
    freeDpb();

  size = getDpbSize (decoder) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  numRefFrames = decoder->activeSps->numRefFrames;
  if (size < decoder->activeSps->numRefFrames)
    cDecoder264::error ("DPB size at specified level is smaller than the specified number of refFrames");

  usedSize = 0;
  lastPictureFrameStore = NULL;
  refFramesInBuffer = 0;
  longTermRefFramesInBuffer = 0;
  lastOutPoc = INT_MIN;

  frameStore = (cFrameStore**)calloc (size, sizeof (cFrameStore*));
  frameStoreRef = (cFrameStore**)calloc (size, sizeof (cFrameStore*));
  frameStoreLongTermRef = (cFrameStore**)calloc (size, sizeof (cFrameStore*));
  for (uint32_t i = 0; i < size; i++) {
    frameStore[i] = new cFrameStore();
    frameStoreRef[i] = NULL;
    frameStoreLongTermRef[i] = NULL;
    }

  // allocate dummyRefPicture
  if (!decoder->noRefPicture) {
    decoder->noRefPicture = allocPicture (decoder, eFrame, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);
    decoder->noRefPicture->topField = decoder->noRefPicture;
    decoder->noRefPicture->botField = decoder->noRefPicture;
    decoder->noRefPicture->frame = decoder->noRefPicture;
    }

  decoder->lastHasMmco5 = 0;

  // picture error conceal
  if ((decoder->concealMode != 0) && !decoder->lastOutFramestore)
    decoder->lastOutFramestore = new cFrameStore();

  initDone = true;
  }
//}}}
//{{{
void cDpb::reInitDpb (cDecoder264* decoder, int type) {

  int dpbSize = getDpbSize (decoder) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  numRefFrames = decoder->activeSps->numRefFrames;

  if (dpbSize > (int)size) {
    if (size < decoder->activeSps->numRefFrames)
      cDecoder264::error ("DPB size at specified level is smaller than the specified number of refFrames");

    frameStore = (cFrameStore**)realloc (frameStore, dpbSize * sizeof (cFrameStore*));
    frameStoreRef = (cFrameStore**)realloc(frameStoreRef, dpbSize * sizeof (cFrameStore*));
    frameStoreLongTermRef = (cFrameStore**)realloc(frameStoreLongTermRef, dpbSize * sizeof (cFrameStore*));
    for (int i = size; i < dpbSize; i++) {
      frameStore[i] = new cFrameStore();
      frameStoreRef[i] = NULL;
      frameStoreLongTermRef[i] = NULL;
      }

    size = dpbSize;
    lastOutPoc = INT_MIN;

    initDone = true;
    }
  }
//}}}

//{{{
void cDpb::freeDpb () {

  if (frameStore)
    for (uint32_t i = 0; i < size; i++)
      delete frameStore[i];
  free (frameStore);
  frameStore = NULL;

  free (frameStoreRef);
  frameStoreRef = NULL;

  free (frameStoreLongTermRef);
  frameStoreLongTermRef = NULL;

  lastOutPoc = INT_MIN;

  // picture error conceal
  if (decoder->concealMode != 0 || decoder->lastOutFramestore)
    delete decoder->lastOutFramestore;

  freePicture (decoder->noRefPicture);
  decoder->noRefPicture = NULL;

  initDone = false;
  }
//}}}
//{{{
void cDpb::flushDpb() {

  if (!initDone)
    return;

  if (decoder->concealMode != 0)
    concealNonRefPics (this, 0);

  // mark all frames unused
  for (uint32_t i = 0; i < usedSize; i++)
    frameStore[i]->unmarkForRef();

  while (removeUnusedDpb());

  // output frames in POC order
  while (usedSize && outputDpbFrame());

  lastOutPoc = INT_MIN;
  }
//}}}
//{{{
int cDpb::removeUnusedDpb() {

  // check for frames that were already output and no longer used for ref
  for (uint32_t i = 0; i < usedSize; i++) {
    if (frameStore[i]->isOutput && (!frameStore[i]->isRef())) {
      removeFrameDpb (i);
      return 1;
      }
    }

  return 0;
  }
//}}}
//{{{
void cDpb::removeFrameDpb (int pos) {

  cFrameStore* frameStore1 = frameStore[pos];
  switch (frameStore1->isUsed) {
    case 3:
      freePicture (frameStore1->frame);
      freePicture (frameStore1->topField);
      freePicture (frameStore1->botField);
      frameStore1->frame = NULL;
      frameStore1->topField = NULL;
      frameStore1->botField = NULL;
      break;

    case 2:
      freePicture (frameStore1->botField);
      frameStore1->botField = NULL;
      break;

    case 1:
      freePicture (frameStore1->topField);
      frameStore1->topField = NULL;
      break;

    case 0:
      break;

    default:
      cDecoder264::error ("invalid frame store type");
    }

  frameStore1->isUsed = 0;
  frameStore1->usedLongTermRef = 0;
  frameStore1->usedRef = 0;
  frameStore1->usedOrigRef = 0;

  // move empty framestore to end of buffer
  for (uint32_t i = pos; i < usedSize-1; i++)
    frameStore[i] = frameStore[i+1];

  frameStore[usedSize-1] = frameStore1;
  usedSize--;
  }
//}}}
//{{{
void cDpb::storePictureDpb (sPicture* picture) {

  int poc, pos;
  // picture error conceal

  // if frame, check for new store,
  decoder->lastHasMmco5 = 0;
  decoder->lastPicBotField = (picture->picStructure == eBotField);

  if (picture->isIDR) {
    idrMemoryManagement (picture);
    // picture error conceal
    memset (decoder->dpbPoc, 0, sizeof(int)*100);
    }
  else {
    // adaptive memory management
    if (picture->usedForRef && (picture->adaptRefPicBufFlag))
      adaptiveMemoryManagement (picture);
    }

  if ((picture->picStructure == eTopField) || (picture->picStructure == eBotField)) {
    // check for frame store with same pic_number
    if (lastPictureFrameStore) {
      if ((int)lastPictureFrameStore->frameNum == picture->picNum) {
        if (((picture->picStructure == eTopField) && (lastPictureFrameStore->isUsed == 2))||
            ((picture->picStructure == eBotField) && (lastPictureFrameStore->isUsed == 1))) {
          if ((picture->usedForRef && (lastPictureFrameStore->usedOrigRef != 0)) ||
              (!picture->usedForRef && (lastPictureFrameStore->usedOrigRef == 0))) {
            lastPictureFrameStore->insertPictureDpb (decoder, picture);
            updateRefList();
            updateLongTermRefList();
            dump();
            lastPictureFrameStore = NULL;
            return;
            }
          }
        }
      }
    }

  // this is a frame or a field which has no stored complementary field sliding window, if necessary
  if (!picture->isIDR && (picture->usedForRef && !picture->adaptRefPicBufFlag))
    slidingWindowMemoryManagement (picture);

  // picture error conceal
  if (decoder->concealMode != 0)
    for (uint32_t i = 0; i < size; i++)
      if (frameStore[i]->usedRef)
        frameStore[i]->concealRef = 1;

  // first try to remove unused frames
  if (usedSize == size) {
    // picture error conceal
    if (decoder->concealMode != 0)
      concealNonRefPics (this, 2);

    removeUnusedDpb();

    if (decoder->concealMode != 0)
      slidingWindowPocManagement (this, picture);
    }

  // then output frames until one can be removed
  while (usedSize == size) {
    // non-reference frames may be output directly
    if (!picture->usedForRef) {
      getSmallestPoc (&poc, &pos);
      if ((-1 == pos) || (picture->poc < poc)) {
        decoder->directOutput (picture);
        return;
        }
      }

    // flush a frame
    outputDpbFrame();
    }

  // check for duplicate frame number in int16_t term ref buffer
  if ((picture->usedForRef) && (!picture->usedLongTermRef))
    for (uint32_t i = 0; i < refFramesInBuffer; i++)
      if (frameStoreRef[i]->frameNum == picture->frameNum)
        cDecoder264::error ("duplicate frameNum in int16_t-term ref picture buffer");

  // store at end of buffer
  frameStore[usedSize]->insertPictureDpb (decoder,  picture);

  // picture error conceal
  if (picture->isIDR)
    decoder->earlierMissingPoc = 0;

  if (picture->picStructure != eFrame)
    lastPictureFrameStore = frameStore[usedSize];
  else
    lastPictureFrameStore = NULL;

  usedSize++;

  if (decoder->concealMode != 0)
    decoder->dpbPoc[usedSize-1] = picture->poc;

  updateRefList();
  updateLongTermRefList();
  checkNumDpbFrames();
  dump();
  }
//}}}

//{{{
sPicture* cDpb::getLastPicRefFromDpb() {

  int usedSize1 = usedSize - 1;
  for (int i = usedSize1; i >= 0; i--)
    if (frameStore[i]->isUsed == 3)
      if (frameStore[i]->frame->usedForRef && !frameStore[i]->frame->usedLongTermRef)
        return frameStore[i]->frame;

  return NULL;
  }
//}}}
//{{{
sPicture* cDpb::getShortTermPic (cSlice* slice, int picNum) {

  for (uint32_t i = 0; i < refFramesInBuffer; i++) {
    if (slice->picStructure == eFrame) {
      if (frameStoreRef[i]->usedRef == 3)
        if (!frameStoreRef[i]->frame->usedLongTermRef &&
            (frameStoreRef[i]->frame->picNum == picNum))
          return frameStoreRef[i]->frame;
      }
    else {
      if (frameStoreRef[i]->usedRef & 1)
        if (!frameStoreRef[i]->topField->usedLongTermRef &&
            (frameStoreRef[i]->topField->picNum == picNum))
          return frameStoreRef[i]->topField;

      if (frameStoreRef[i]->usedRef & 2)
        if (!frameStoreRef[i]->botField->usedLongTermRef &&
            (frameStoreRef[i]->botField->picNum == picNum))
          return frameStoreRef[i]->botField;
      }
    }

  return slice->decoder->noRefPicture;
  }
//}}}
//{{{
sPicture* cDpb::getLongTermPic (cSlice* slice, int picNum) {

  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++) {
    if (slice->picStructure == eFrame) {
      if (frameStoreLongTermRef[i]->usedRef == 3)
        if ((frameStoreLongTermRef[i]->frame->usedLongTermRef) &&
            (frameStoreLongTermRef[i]->frame->longTermPicNum == picNum))
          return frameStoreLongTermRef[i]->frame;
      }

    else {
      if (frameStoreLongTermRef[i]->usedRef & 1)
        if ((frameStoreLongTermRef[i]->topField->usedLongTermRef) &&
            (frameStoreLongTermRef[i]->topField->longTermPicNum == picNum))
          return frameStoreLongTermRef[i]->topField;

      if (frameStoreLongTermRef[i]->usedRef & 2)
        if ((frameStoreLongTermRef[i]->botField->usedLongTermRef) &&
            (frameStoreLongTermRef[i]->botField->longTermPicNum == picNum))
          return frameStoreLongTermRef[i]->botField;
      }
    }

  return NULL;
  }
//}}}

// private
//{{{
void cDpb::dump() {

  cLog::log (LOGINFO, fmt::format ("dpb {}:{} numRef:{} refFramesInBuffer:{} numLongTerm:{} max:{} last:{}",
                                   usedSize, size,
                                   numRefFrames, refFramesInBuffer,
                                   longTermRefFramesInBuffer, maxLongTermPicIndex, lastOutPoc));

  for (uint32_t i = 0; i < usedSize; i++) {
    string debugString = fmt::format ("{} frameNum:{:2d} ", i, frameStore[i]->frameNum);

    if (frameStore[i]->isUsed & 1)
      debugString += fmt::format (" tPoc:{:3d}", frameStore[i]->topField ? frameStore[i]->topField->poc
                                                                         : frameStore[i]->frame->topPoc);
    if (frameStore[i]->isUsed & 2)
      debugString += fmt::format (" bPoc:{:3d}", frameStore[i]->botField ? frameStore[i]->botField->poc
                                                                         : frameStore[i]->frame->botPoc);
    if (frameStore[i]->isUsed == 3)
      debugString += fmt::format (" fPoc:{:3d}", frameStore[i]->frame->poc);

    debugString += fmt::format (" poc:{:3d}", frameStore[i]->poc);

    if (frameStore[i]->usedRef)
      debugString += fmt::format (" ref:{}", frameStore[i]->usedRef);

    if (frameStore[i]->usedLongTermRef)
      debugString += fmt::format (" longTermRef:{}", frameStore[i]->usedRef);

    cLog::log (LOGINFO, fmt::format ("{}{}{}",
                 debugString,
                 frameStore[i]->isOutput ? " out":"",
                 (frameStore[i]->isUsed == 3) && frameStore[i]->frame->nonExisting ? " nonExisiting":""));
    }
  }
//}}}
//{{{
int cDpb::outputDpbFrame() {

  if (usedSize < 1)
    cDecoder264::error ("Cannot output frame, DPB empty");

  int poc;
  int pos;
  getSmallestPoc (&poc, &pos);
  if (pos == -1)
    return 0;

  // error conceal
  if (decoder->concealMode != 0) {
    if (lastOutPoc == 0)
      writeLostRefAfterIdr (this, pos);
    writeLostNonRefPic (this, poc);
    }

  decoder->writeStoredFrame (frameStore[pos]);

  // error conceal
  if(decoder->concealMode == 0)
    if (lastOutPoc >= poc)
      cDecoder264::error ("output POC must be in ascending order");

  lastOutPoc = poc;

  // free frameStore and move emptyStore to end of buffer
  if (!frameStore[pos]->isRef())
    removeFrameDpb (pos);

  return 1;
  }
//}}}
//{{{
void cDpb::checkNumDpbFrames() {

  if ((int)(longTermRefFramesInBuffer + refFramesInBuffer) > imax (1, numRefFrames))
    cDecoder264::error ("Max. number of refFrames exceeded. Invalid stream");
  }
//}}}

//{{{
void cDpb::getSmallestPoc (int* poc, int* pos) {

  if (usedSize < 1)
    cDecoder264::error ("Cannot determine smallest POC, DPB empty");

  *pos = -1;
  *poc = INT_MAX;
  for (uint32_t i = 0; i < usedSize; i++) {
    if ((*poc > frameStore[i]->poc) && (!frameStore[i]->isOutput)) {
      *poc = frameStore[i]->poc;
      *pos = i;
      }
    }
  }
//}}}

//{{{
void cDpb::idrMemoryManagement (sPicture* picture) {

  if (picture->noOutputPriorPicFlag) {
    // free all stored pictures
    for (uint32_t i = 0; i < usedSize; i++) {
      // reset all ref settings
      delete frameStore[i];
      frameStore[i] = new cFrameStore();
      }
    for (uint32_t i = 0; i < refFramesInBuffer; i++)
      frameStoreRef[i] = NULL;
    for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++)
      frameStoreLongTermRef[i] = NULL;
    usedSize = 0;
    }
  else
    flushDpb();

  lastPictureFrameStore = NULL;

  updateRefList();
  updateLongTermRefList();
  lastOutPoc = INT_MIN;

  if (picture->longTermRefFlag) {
    maxLongTermPicIndex = 0;
    picture->usedLongTermRef = 1;
    picture->longTermFrameIndex = 0;
    }
  else {
    maxLongTermPicIndex = -1;
    picture->usedLongTermRef = 0;
    }
  }
//}}}
//{{{
void cDpb::adaptiveMemoryManagement (sPicture* picture) {

  decoder->lastHasMmco5 = 0;

  sDecodedRefPicMark* tempDecodedRefPicMark;
  while (picture->decRefPicMarkBuffer) {
    tempDecodedRefPicMark = picture->decRefPicMarkBuffer;
    switch (tempDecodedRefPicMark->memManagement) {
      //{{{
      case 0:
        if (tempDecodedRefPicMark->next != NULL)
          cDecoder264::error ("memManagement = 0 not last operation in buffer");
        break;
      //}}}
      //{{{
      case 1:
        unmarkShortTermForRef (picture, tempDecodedRefPicMark->diffPicNumMinus1);
        updateRefList();
        break;
      //}}}
      //{{{
      case 2:
        unmarkLongTermForRef (picture, tempDecodedRefPicMark->longTermPicNum);
        updateLongTermRefList();
        break;
      //}}}
      //{{{
      case 3:
        assignLongTermFrameIndex (picture, tempDecodedRefPicMark->diffPicNumMinus1,
                                           tempDecodedRefPicMark->longTermFrameIndex);
        updateRefList();
        updateLongTermRefList();
        break;
      //}}}
      //{{{
      case 4:
        updateMaxLongTermFrameIndex (tempDecodedRefPicMark->maxLongTermFrameIndexPlus1);
        updateLongTermRefList();
        break;
      //}}}
      //{{{
      case 5:
        unmarkAllShortTermForRef();
        unmarkAllLongTermForRef();
        decoder->lastHasMmco5 = 1;
        break;
      //}}}
      //{{{
      case 6:
        markCurPicLongTerm (picture, tempDecodedRefPicMark->longTermFrameIndex);
        checkNumDpbFrames();
        break;
      //}}}
      //{{{
      default:
        cDecoder264::error ("invalid memManagement in buffer");
      //}}}
      }
    picture->decRefPicMarkBuffer = tempDecodedRefPicMark->next;
    free (tempDecodedRefPicMark);
    }

  if (decoder->lastHasMmco5) {
    picture->picNum = picture->frameNum = 0;
    switch (picture->picStructure) {
      //{{{
      case eTopField:
        picture->poc = picture->topPoc = 0;
        break;
      //}}}
      //{{{
      case eBotField:
        picture->poc = picture->botPoc = 0;
        break;
      //}}}
      //{{{
      case eFrame:
        picture->topPoc -= picture->poc;
        picture->botPoc -= picture->poc;
        picture->poc = imin (picture->topPoc, picture->botPoc);
        break;
      //}}}
      }
    decoder->dpb->flushDpb();
    }
  }
//}}}
//{{{
void cDpb::slidingWindowMemoryManagement (sPicture* picture) {

  // if this is a refPic with sliding window, unmark first ref frame
  if (refFramesInBuffer == imax (1, numRefFrames) - longTermRefFramesInBuffer) {
    for (uint32_t i = 0; i < usedSize; i++) {
      if (frameStore[i]->usedRef && (!frameStore[i]->usedLongTermRef)) {
        frameStore[i]->unmarkForRef();
        updateRefList();
        break;
        }
      }
    }

  picture->usedLongTermRef = 0;
  }
//}}}

//{{{
void cDpb::updateRefList() {

  uint32_t i;
  uint32_t j;
  for (i = 0, j = 0; i < usedSize; i++)
    if (frameStore[i]->isShortTermRef())
      frameStoreRef[j++] = frameStore[i];

  refFramesInBuffer = j;

  while (j < size)
    frameStoreRef[j++] = NULL;
  }
//}}}
//{{{
void cDpb::updateLongTermRefList() {

  uint32_t i;
  uint32_t j;
  for (i = 0, j = 0; i < usedSize; i++)
    if (frameStore[i]->isLongTermRef())
      frameStoreLongTermRef[j++] = frameStore[i];

  longTermRefFramesInBuffer = j;

  while (j < size)
    frameStoreLongTermRef[j++] = NULL;
  }
//}}}
//{{{
void cDpb::updateMaxLongTermFrameIndex (int maxLongTermFrameIndexPlus1) {

  maxLongTermPicIndex = maxLongTermFrameIndexPlus1 - 1;

  // check for invalid frames
  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++)
    if (frameStoreLongTermRef[i]->longTermFrameIndex > maxLongTermPicIndex)
      frameStoreLongTermRef[i]->unmarkForLongTermRef();
  }
//}}}

//{{{
void cDpb::unmarkAllShortTermForRef() {

  for (uint32_t i = 0; i < refFramesInBuffer; i++)
    frameStoreRef[i]->unmarkForRef();

  updateRefList();
  }
//}}}
//{{{
void cDpb::unmarkShortTermForRef (sPicture* picture, int diffPicNumMinus1) {

  int picNumX = getPicNumX (picture, diffPicNumMinus1);

  for (uint32_t i = 0; i < refFramesInBuffer; i++) {
    if (picture->picStructure == eFrame) {
      if ((frameStoreRef[i]->usedRef == 3) && (frameStoreRef[i]->usedLongTermRef == 0)) {
        if (frameStoreRef[i]->frame->picNum == picNumX) {
          frameStoreRef[i]->unmarkForRef();
          return;
          }
        }
      }
    else {
      if ((frameStoreRef[i]->usedRef & 1) && !(frameStoreRef[i]->usedLongTermRef & 1)) {
        if (frameStoreRef[i]->topField->picNum == picNumX) {
          frameStoreRef[i]->topField->usedForRef = 0;
          frameStoreRef[i]->usedRef &= 2;
          if (frameStoreRef[i]->isUsed == 3)
            frameStoreRef[i]->frame->usedForRef = 0;
          return;
          }
        }

      if ((frameStoreRef[i]->usedRef & 2) && !(frameStoreRef[i]->usedLongTermRef & 2)) {
        if (frameStoreRef[i]->botField->picNum == picNumX) {
          frameStoreRef[i]->botField->usedForRef = 0;
          frameStoreRef[i]->usedRef &= 1;
          if (frameStoreRef[i]->isUsed == 3)
            frameStoreRef[i]->frame->usedForRef = 0;
          return;
          }
        }
      }
    }
  }
//}}}

//{{{
void cDpb::unmarkAllLongTermForRef() {
  updateMaxLongTermFrameIndex (0);
  }
//}}}
//{{{
void cDpb::unmarkLongTermForRef (sPicture* picture, int picNum) {

  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++) {
    if (picture->picStructure == eFrame) {
      if ((frameStoreLongTermRef[i]->usedRef == 3) &&
          (frameStoreLongTermRef[i]->usedLongTermRef == 3))
        if (frameStoreLongTermRef[i]->frame->longTermPicNum == picNum)
          frameStoreLongTermRef[i]->unmarkForLongTermRef();
      }
    else {
      if ((frameStoreLongTermRef[i]->usedRef & 1) &&
          (frameStoreLongTermRef[i]->usedLongTermRef & 1))
        if (frameStoreLongTermRef[i]->topField->longTermPicNum == picNum) {
          frameStoreLongTermRef[i]->topField->usedForRef = 0;
          frameStoreLongTermRef[i]->topField->usedLongTermRef = 0;
          frameStoreLongTermRef[i]->usedRef &= 2;
          frameStoreLongTermRef[i]->usedLongTermRef &= 2;
          if (frameStoreLongTermRef[i]->isUsed == 3) {
            frameStoreLongTermRef[i]->frame->usedForRef = 0;
            frameStoreLongTermRef[i]->frame->usedLongTermRef = 0;
            }
          return;
          }

      if ((frameStoreLongTermRef[i]->usedRef & 2) &&
          (frameStoreLongTermRef[i]->usedLongTermRef & 2))
        if (frameStoreLongTermRef[i]->botField->longTermPicNum == picNum) {
          frameStoreLongTermRef[i]->botField->usedForRef = 0;
          frameStoreLongTermRef[i]->botField->usedLongTermRef = 0;
          frameStoreLongTermRef[i]->usedRef &= 1;
          frameStoreLongTermRef[i]->usedLongTermRef &= 1;
          if (frameStoreLongTermRef[i]->isUsed == 3) {
            frameStoreLongTermRef[i]->frame->usedForRef = 0;
            frameStoreLongTermRef[i]->frame->usedLongTermRef = 0;
            }
          return;
          }
      }
    }
  }
//}}}
//{{{
void cDpb::unmarkLongTermFieldRefFrameIndex (ePicStructure picStructure, int longTermFrameIndex,
                                              int markCur, uint32_t curFrameNum, int curPicNum) {

  if (curPicNum < 0)
    curPicNum += 2 * decoder->coding.maxFrameNum;

  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++) {
    if (frameStoreLongTermRef[i]->longTermFrameIndex == longTermFrameIndex) {
      if (picStructure == eTopField) {
        if (frameStoreLongTermRef[i]->usedLongTermRef == 3)
          frameStoreLongTermRef[i]->unmarkForLongTermRef();
        else {
          if (frameStoreLongTermRef[i]->usedLongTermRef == 1)
            frameStoreLongTermRef[i]->unmarkForLongTermRef();
          else {
            if (markCur) {
              if (lastPictureFrameStore) {
                if ((lastPictureFrameStore != frameStoreLongTermRef[i]) ||
                    lastPictureFrameStore->frameNum != curFrameNum)
                  frameStoreLongTermRef[i]->unmarkForLongTermRef();
                }
              else
                frameStoreLongTermRef[i]->unmarkForLongTermRef();
            }
            else {
              if ((frameStoreLongTermRef[i]->frameNum) != (uint32_t)(curPicNum >> 1))
                frameStoreLongTermRef[i]->unmarkForLongTermRef();
              }
            }
          }
        }

      if (picStructure == eBotField) {
        if (frameStoreLongTermRef[i]->usedLongTermRef == 3)
          frameStoreLongTermRef[i]->unmarkForLongTermRef();
        else {
          if (frameStoreLongTermRef[i]->usedLongTermRef == 2)
            frameStoreLongTermRef[i]->unmarkForLongTermRef();
          else {
            if (markCur) {
              if (lastPictureFrameStore) {
                if ((lastPictureFrameStore != frameStoreLongTermRef[i]) ||
                    lastPictureFrameStore->frameNum != curFrameNum)
                  frameStoreLongTermRef[i]->unmarkForLongTermRef();
                }
              else
                frameStoreLongTermRef[i]->unmarkForLongTermRef();
              }
            else {
              if ((frameStoreLongTermRef[i]->frameNum) != (uint32_t)(curPicNum >> 1))
                frameStoreLongTermRef[i]->unmarkForLongTermRef();
              }
            }
          }
        }
      }
    }
  }
//}}}
//{{{
void cDpb::unmarkLongTermFrameForRefByFrameIndex (int longTermFrameIndex) {

  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++)
    if (frameStoreLongTermRef[i]->longTermFrameIndex == longTermFrameIndex)
      frameStoreLongTermRef[i]->unmarkForLongTermRef();
  }
//}}}

//{{{
void cDpb::markPicLongTerm (sPicture* picture, int longTermFrameIndex, int picNumX) {

  int addTop, addBot;

  if (picture->picStructure == eFrame) {
    for (uint32_t i = 0; i < refFramesInBuffer; i++) {
      if (frameStoreRef[i]->usedRef == 3) {
        if ((!frameStoreRef[i]->frame->usedLongTermRef)&&(frameStoreRef[i]->frame->picNum == picNumX)) {
          frameStoreRef[i]->longTermFrameIndex = frameStoreRef[i]->frame->longTermFrameIndex
                                             = longTermFrameIndex;
          frameStoreRef[i]->frame->longTermPicNum = longTermFrameIndex;
          frameStoreRef[i]->frame->usedLongTermRef = 1;

          if (frameStoreRef[i]->topField && frameStoreRef[i]->botField) {
            frameStoreRef[i]->topField->longTermFrameIndex = longTermFrameIndex;
            frameStoreRef[i]->botField->longTermFrameIndex = longTermFrameIndex;
            frameStoreRef[i]->topField->longTermPicNum = longTermFrameIndex;
            frameStoreRef[i]->botField->longTermPicNum = longTermFrameIndex;
            frameStoreRef[i]->topField->usedLongTermRef = 1;
            frameStoreRef[i]->botField->usedLongTermRef = 1;
            }
          frameStoreRef[i]->usedLongTermRef = 3;
          return;
          }
        }
      }
    cLog::log (LOGERROR, "Warning: refFrame for long term marking not found");
    }

  else {
    if (picture->picStructure == eTopField) {
      addTop = 1;
      addBot = 0;
      }
    else {
      addTop = 0;
      addBot = 1;
      }

    for (uint32_t i = 0; i < refFramesInBuffer; i++) {
      if (frameStoreRef[i]->usedRef & 1) {
        if ((!frameStoreRef[i]->topField->usedLongTermRef) &&
            (frameStoreRef[i]->topField->picNum == picNumX)) {
          if ((frameStoreRef[i]->usedLongTermRef) &&
              (frameStoreRef[i]->longTermFrameIndex != longTermFrameIndex)) {
            cLog::log (LOGERROR, "Warning: assigning longTermFrameIndex different from other field");
            }

          frameStoreRef[i]->longTermFrameIndex = longTermFrameIndex;
          frameStoreRef[i]->topField->longTermFrameIndex = longTermFrameIndex;
          frameStoreRef[i]->topField->longTermPicNum = 2 * longTermFrameIndex + addTop;
          frameStoreRef[i]->topField->usedLongTermRef = 1;
          frameStoreRef[i]->usedLongTermRef |= 1;
          if (frameStoreRef[i]->usedLongTermRef == 3) {
            frameStoreRef[i]->frame->usedLongTermRef = 1;
            frameStoreRef[i]->frame->longTermFrameIndex = longTermFrameIndex;
            frameStoreRef[i]->frame->longTermPicNum = longTermFrameIndex;
            }
          return;
          }
        }

      if (frameStoreRef[i]->usedRef & 2) {
        if ((!frameStoreRef[i]->botField->usedLongTermRef) &&
            (frameStoreRef[i]->botField->picNum == picNumX)) {
          if ((frameStoreRef[i]->usedLongTermRef) &&
              (frameStoreRef[i]->longTermFrameIndex != longTermFrameIndex))
            cLog::log (LOGERROR, "Warning: assigning longTermFrameIndex different from other field");

          frameStoreRef[i]->longTermFrameIndex = longTermFrameIndex;
          frameStoreRef[i]->botField->longTermFrameIndex = longTermFrameIndex;
          frameStoreRef[i]->botField->longTermPicNum = 2 * longTermFrameIndex + addBot;
          frameStoreRef[i]->botField->usedLongTermRef = 1;
          frameStoreRef[i]->usedLongTermRef |= 2;
          if (frameStoreRef[i]->usedLongTermRef == 3) {
            frameStoreRef[i]->frame->usedLongTermRef = 1;
            frameStoreRef[i]->frame->longTermFrameIndex = longTermFrameIndex;
            frameStoreRef[i]->frame->longTermPicNum = longTermFrameIndex;
            }
          return;
          }
        }
      }
    cLog::log (LOGERROR, "refField for long term marking not found");
    }
  }
//}}}
//{{{
void cDpb::markCurPicLongTerm (sPicture* picture, int longTermFrameIndex) {

  // remove long term pictures with same longTermFrameIndex
  if (picture->picStructure == eFrame)
    unmarkLongTermFrameForRefByFrameIndex (longTermFrameIndex);
  else
    unmarkLongTermFieldRefFrameIndex (picture->picStructure, longTermFrameIndex, 1, picture->picNum, 0);

  picture->usedLongTermRef = 1;
  picture->longTermFrameIndex = longTermFrameIndex;
  }
//}}}

//{{{
void cDpb::assignLongTermFrameIndex (sPicture* picture, int diffPicNumMinus1, int longTermFrameIndex) {

  int picNumX = getPicNumX (picture, diffPicNumMinus1);

  // remove frames/fields with same longTermFrameIndex
  if (picture->picStructure == eFrame)
    unmarkLongTermFrameForRefByFrameIndex (longTermFrameIndex);

  else {
    ePicStructure picStructure = eFrame;
    for (uint32_t i = 0; i < refFramesInBuffer; i++) {
      if (frameStoreRef[i]->usedRef & 1) {
        if (frameStoreRef[i]->topField->picNum == picNumX) {
          picStructure = eTopField;
          break;
          }
        }

      if (frameStoreRef[i]->usedRef & 2) {
        if (frameStoreRef[i]->botField->picNum == picNumX) {
          picStructure = eBotField;
          break;
          }
        }
      }

    if (picStructure == eFrame)
      cDecoder264::error ("field for long term marking not found");

    unmarkLongTermFieldRefFrameIndex (picStructure, longTermFrameIndex, 0, 0, picNumX);
    }

  markPicLongTerm (picture, longTermFrameIndex, picNumX);
  }
//}}}
