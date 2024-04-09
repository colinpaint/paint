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
  int getPicNumX (sPicture* picture, int diffPicNumMinus1) {

    int curPicNum;
    if (picture->picStructure == eFrame)
      curPicNum = picture->frameNum;
    else
      curPicNum = 2 * picture->frameNum + 1;

    return curPicNum - (diffPicNumMinus1 + 1);
    }
  //}}}
  }

// dpb
//{{{
void cDpb::initDpb (cDecoder264* decoder, int type) {

  if (initDone)
    freeDpb();

  allocatedSize = getDpbSize (decoder) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  numRefFrames = decoder->activeSps->numRefFrames;
  if (allocatedSize < decoder->activeSps->numRefFrames)
    cDecoder264::error ("DPB size at specified level is smaller than the specified number of refFrames");

  usedSize = 0;
  lastPictureFrameStore = NULL;
  refFramesInBuffer = 0;
  longTermRefFramesInBuffer = 0;
  lastOutPoc = INT_MIN;

  frameStoreArray = (cFrameStore**)calloc (allocatedSize, sizeof (cFrameStore*));
  frameStoreRefArray = (cFrameStore**)calloc (allocatedSize, sizeof (cFrameStore*));
  frameStoreLongTermRefArray = (cFrameStore**)calloc (allocatedSize, sizeof (cFrameStore*));
  for (uint32_t i = 0; i < allocatedSize; i++) {
    frameStoreArray[i] = new cFrameStore();
    frameStoreRefArray[i] = NULL;
    frameStoreLongTermRefArray[i] = NULL;
    }

  // allocate dummyRefPicture
  if (!noRefPicture) {
    noRefPicture = allocPicture (decoder, eFrame, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);
    noRefPicture->topField = noRefPicture;
    noRefPicture->botField = noRefPicture;
    noRefPicture->frame = noRefPicture;
    }

  decoder->lastHasMmco5 = 0;

  // picture error conceal
  if ((decoder->concealMode != 0) && !decoder->lastOutFrameStore)
    decoder->lastOutFrameStore = new cFrameStore();

  initDone = true;
  }
//}}}
//{{{
void cDpb::reInitDpb (cDecoder264* decoder, int type) {

  uint32_t dpbSize = getDpbSize (decoder) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  numRefFrames = decoder->activeSps->numRefFrames;

  if (dpbSize > allocatedSize) {
    if (allocatedSize < decoder->activeSps->numRefFrames)
      cDecoder264::error ("DPB size at specified level is smaller than the specified number of refFrames");

    frameStoreArray = (cFrameStore**)realloc (frameStoreArray, dpbSize * sizeof (cFrameStore*));
    frameStoreRefArray = (cFrameStore**)realloc(frameStoreRefArray, dpbSize * sizeof (cFrameStore*));
    frameStoreLongTermRefArray = (cFrameStore**)realloc(frameStoreLongTermRefArray, dpbSize * sizeof (cFrameStore*));
    for (uint32_t i = allocatedSize; i < dpbSize; i++) {
      frameStoreArray[i] = new cFrameStore();
      frameStoreRefArray[i] = NULL;
      frameStoreLongTermRefArray[i] = NULL;
      }

    allocatedSize = dpbSize;
    lastOutPoc = INT_MIN;

    initDone = true;
    }
  }
//}}}

//{{{
void cDpb::freeDpb () {

  if (frameStoreArray)
    for (uint32_t i = 0; i < allocatedSize; i++)
      delete frameStoreArray[i];
  free (frameStoreArray);
  frameStoreArray = NULL;

  free (frameStoreRefArray);
  frameStoreRefArray = NULL;

  free (frameStoreLongTermRefArray);
  frameStoreLongTermRefArray = NULL;

  lastOutPoc = INT_MIN;

  // picture error conceal
  if (decoder->concealMode != 0 || decoder->lastOutFrameStore)
    delete decoder->lastOutFrameStore;

  freePicture (noRefPicture);
  noRefPicture = NULL;

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
    frameStoreArray[i]->unmarkForRef();

  while (removeUnusedDpb());

  // output frames in POC order
  while (usedSize && outputDpbFrame());

  lastOutPoc = INT_MIN;
  }
//}}}
//{{{
void cDpb::storePictureDpb (sPicture* picture) {

  // if frame, check for new store,
  decoder->lastHasMmco5 = 0;
  decoder->lastPicBotField = (picture->picStructure == eBotField);

  if (picture->isIDR) {
    idrMemoryManagement (picture);
    // picture error conceal
    memset (&decoder->dpb.dpbPoc, 0, sizeof(int)*100);
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
            if (decoder->param.dpbDebug)
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
    for (uint32_t i = 0; i < allocatedSize; i++)
      if (frameStoreArray[i]->usedRef)
        frameStoreArray[i]->concealRef = 1;

  // first try to remove unused frames
  if (usedSize == allocatedSize) {
    // picture error conceal
    if (decoder->concealMode != 0)
      concealNonRefPics (this, 2);

    removeUnusedDpb();

    if (decoder->concealMode != 0)
      slidingWindowPocManagement (this, picture);
    }

  // then output frames until one can be removed
  while (usedSize == allocatedSize) {
    // non-reference frames may be output directly
    if (!picture->usedForRef) {
      int poc;
      int pos;
      getSmallestPoc (poc, pos);
      if ((-1 == pos) || (picture->poc < poc)) {
        decoder->directOutput (picture);
        return;
        }
      }
    outputDpbFrame();
    }

  // check for duplicate frame number in int16_t term ref buffer
  if ((picture->usedForRef) && (!picture->usedLongTermRef))
    for (uint32_t i = 0; i < refFramesInBuffer; i++)
      if (frameStoreRefArray[i]->frameNum == picture->frameNum)
        cDecoder264::error ("duplicate frameNum in int16_t-term ref picture buffer");

  // store at end of buffer
  frameStoreArray[usedSize]->insertPictureDpb (decoder, picture);

  // picture error conceal
  if (picture->isIDR)
    decoder->earlierMissingPoc = 0;

  if (picture->picStructure != eFrame)
    lastPictureFrameStore = frameStoreArray[usedSize];
  else
    lastPictureFrameStore = NULL;

  usedSize++;

  if (decoder->concealMode != 0)
    decoder->dpb.dpbPoc[usedSize-1] = picture->poc;

  updateRefList();
  updateLongTermRefList();
  checkNumDpbFrames();
  if (decoder->param.dpbDebug)
    dump();
  }
//}}}

//{{{
sPicture* cDpb::getLastPicRefFromDpb() {

  int usedSize1 = usedSize - 1;
  for (int i = usedSize1; i >= 0; i--)
    if (frameStoreArray[i]->isUsed == 3)
      if (frameStoreArray[i]->frame->usedForRef && !frameStoreArray[i]->frame->usedLongTermRef)
        return frameStoreArray[i]->frame;

  return NULL;
  }
//}}}
//{{{
sPicture* cDpb::getShortTermPic (cSlice* slice, int picNum) {

  for (uint32_t i = 0; i < refFramesInBuffer; i++) {
    if (slice->picStructure == eFrame) {
      if (frameStoreRefArray[i]->usedRef == 3)
        if (!frameStoreRefArray[i]->frame->usedLongTermRef &&
            (frameStoreRefArray[i]->frame->picNum == picNum))
          return frameStoreRefArray[i]->frame;
      }
    else {
      if (frameStoreRefArray[i]->usedRef & 1)
        if (!frameStoreRefArray[i]->topField->usedLongTermRef &&
            (frameStoreRefArray[i]->topField->picNum == picNum))
          return frameStoreRefArray[i]->topField;

      if (frameStoreRefArray[i]->usedRef & 2)
        if (!frameStoreRefArray[i]->botField->usedLongTermRef &&
            (frameStoreRefArray[i]->botField->picNum == picNum))
          return frameStoreRefArray[i]->botField;
      }
    }

  return slice->decoder->dpb.noRefPicture;
  }
//}}}
//{{{
sPicture* cDpb::getLongTermPic (cSlice* slice, int picNum) {

  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++) {
    if (slice->picStructure == eFrame) {
      if (frameStoreLongTermRefArray[i]->usedRef == 3)
        if ((frameStoreLongTermRefArray[i]->frame->usedLongTermRef) &&
            (frameStoreLongTermRefArray[i]->frame->longTermPicNum == picNum))
          return frameStoreLongTermRefArray[i]->frame;
      }

    else {
      if (frameStoreLongTermRefArray[i]->usedRef & 1)
        if ((frameStoreLongTermRefArray[i]->topField->usedLongTermRef) &&
            (frameStoreLongTermRefArray[i]->topField->longTermPicNum == picNum))
          return frameStoreLongTermRefArray[i]->topField;

      if (frameStoreLongTermRefArray[i]->usedRef & 2)
        if ((frameStoreLongTermRefArray[i]->botField->usedLongTermRef) &&
            (frameStoreLongTermRefArray[i]->botField->longTermPicNum == picNum))
          return frameStoreLongTermRefArray[i]->botField;
      }
    }

  return NULL;
  }
//}}}

//{{{
string cDpb::getString() const {

  return fmt::format ("DPB:{}:{} numRef:{} refFramesInBuffer:{} numLongTerm:{} max:{} last:{}",
                      usedSize, allocatedSize, numRefFrames, refFramesInBuffer,
                      longTermRefFramesInBuffer, maxLongTermPicIndex, lastOutPoc);
  }
//}}}
//{{{
string cDpb::getIndexString (uint32_t index) const {

  string debugString = fmt::format ("- frame:{:2d} ", frameStoreArray[index]->frameNum);

  debugString += fmt::format (" poc:{:3d}", frameStoreArray[index]->poc);

  if (frameStoreArray[index]->isUsed == 3)
    debugString += fmt::format (":frm:{:3d}", frameStoreArray[index]->frame->poc);

  if (frameStoreArray[index]->isUsed & 1)
    debugString += fmt::format (":top:{:3d}",
      frameStoreArray[index]->topField ? frameStoreArray[index]->topField->poc
                                       : frameStoreArray[index]->frame->topPoc);
  if (frameStoreArray[index]->isUsed & 2)
    debugString += fmt::format (":bot:{:3d}",
      frameStoreArray[index]->botField ? frameStoreArray[index]->botField->poc
                                       : frameStoreArray[index]->frame->botPoc);

  if (frameStoreArray[index]->usedRef)
    debugString += fmt::format (" ref:{}", frameStoreArray[index]->usedRef);

  if (frameStoreArray[index]->usedLongTermRef)
    debugString += fmt::format (" longTermRef:{}", frameStoreArray[index]->usedRef);

  return fmt::format ("{}{}{}",
                      debugString,
                      frameStoreArray[index]->isOutput ? " out":"",
                      (frameStoreArray[index]->isUsed == 3) &&
                        frameStoreArray[index]->frame->nonExisting ? " nonExisiting":"");
  }
//}}}

// private
//{{{
int cDpb::removeUnusedDpb() {
// check for frames that were already output and no longer used for ref

  for (uint32_t i = 0; i < usedSize; i++) {
    if (frameStoreArray[i]->isOutput && (!frameStoreArray[i]->isRef())) {
      removeFrameDpb (i);
      return 1;
      }
    }

  return 0;
  }
//}}}
//{{{
void cDpb::removeFrameDpb (int pos) {

  cFrameStore* frameStore = frameStoreArray[pos];
  switch (frameStore->isUsed) {
    case 3:
      freePicture (frameStore->frame);
      frameStore->frame = NULL;

      freePicture (frameStore->topField);
      frameStore->topField = NULL;

      freePicture (frameStore->botField);
      frameStore->botField = NULL;
      break;

    case 2:
      freePicture (frameStore->botField);
      frameStore->botField = NULL;
      break;

    case 1:
      freePicture (frameStore->topField);
      frameStore->topField = NULL;
      break;

    case 0:
      break;

    default:
      cDecoder264::error ("invalid frame store type");
    }

  frameStore->isUsed = 0;
  frameStore->usedRef = 0;
  frameStore->usedLongTermRef = 0;
  frameStore->usedOrigRef = 0;

  // move empty framestore to end of buffer
  for (uint32_t i = pos; i < usedSize-1; i++)
    frameStoreArray[i] = frameStoreArray[i+1];

  frameStoreArray[usedSize-1] = frameStore;

  usedSize--;
  }
//}}}

//{{{
void cDpb::dump() {

  cLog::log (LOGINFO, getString());
  for (uint32_t i = 0; i < usedSize; i++)
    cLog::log (LOGINFO, getIndexString (i));
  }
//}}}
//{{{
int cDpb::outputDpbFrame() {

  if (usedSize < 1)
    cDecoder264::error ("Cannot output frame, DPB empty");

  int poc;
  int pos;
  getSmallestPoc (poc, pos);
  if (pos == -1)
    return 0;

  // error conceal
  if (decoder->concealMode != 0) {
    if (lastOutPoc == 0)
      writeLostRefAfterIdr (this, pos);
    writeLostNonRefPic (this, poc);
    }

  decoder->writeStoredFrame (frameStoreArray[pos]);

  // error conceal
  if(decoder->concealMode == 0)
    if (lastOutPoc >= poc)
      cDecoder264::error ("output POC must be in ascending order");

  lastOutPoc = poc;

  // free frameStore and move emptyStore to end of buffer
  if (!frameStoreArray[pos]->isRef())
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
void cDpb::getSmallestPoc (int& poc, int& pos) {

  if (usedSize < 1)
    cDecoder264::error ("Cannot determine smallest POC, DPB empty");

  pos = -1;
  poc = INT_MAX;
  for (uint32_t i = 0; i < usedSize; i++) {
    if ((poc > frameStoreArray[i]->poc) && (!frameStoreArray[i]->isOutput)) {
      poc = frameStoreArray[i]->poc;
      pos = i;
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
      delete frameStoreArray[i];
      frameStoreArray[i] = new cFrameStore();
      }
    for (uint32_t i = 0; i < refFramesInBuffer; i++)
      frameStoreRefArray[i] = NULL;
    for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++)
      frameStoreLongTermRefArray[i] = NULL;
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
    decoder->dpb.flushDpb();
    }
  }
//}}}
//{{{
void cDpb::slidingWindowMemoryManagement (sPicture* picture) {
// if this is a refPic with sliding window, unmark first ref frame

  if (refFramesInBuffer == imax (1, numRefFrames) - longTermRefFramesInBuffer) {
    for (uint32_t i = 0; i < usedSize; i++) {
      if (frameStoreArray[i]->usedRef && (!frameStoreArray[i]->usedLongTermRef)) {
        frameStoreArray[i]->unmarkForRef();
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

  uint32_t i = 0;
  uint32_t j = 0;
  for (; i < usedSize; i++)
    if (frameStoreArray[i]->isShortTermRef())
      frameStoreRefArray[j++] = frameStoreArray[i];

  refFramesInBuffer = j;

  while (j < allocatedSize)
    frameStoreRefArray[j++] = NULL;
  }
//}}}
//{{{
void cDpb::updateLongTermRefList() {

  uint32_t i = 0;
  uint32_t j = 0;
  for (; i < usedSize; i++)
    if (frameStoreArray[i]->isLongTermRef())
      frameStoreLongTermRefArray[j++] = frameStoreArray[i];

  longTermRefFramesInBuffer = j;

  while (j < allocatedSize)
    frameStoreLongTermRefArray[j++] = NULL;
  }
//}}}
//{{{
void cDpb::updateMaxLongTermFrameIndex (int maxLongTermFrameIndexPlus1) {

  maxLongTermPicIndex = maxLongTermFrameIndexPlus1 - 1;

  // check for invalid frames
  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++)
    if (frameStoreLongTermRefArray[i]->longTermFrameIndex > maxLongTermPicIndex)
      frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
  }
//}}}

//{{{
void cDpb::unmarkAllShortTermForRef() {

  for (uint32_t i = 0; i < refFramesInBuffer; i++)
    frameStoreRefArray[i]->unmarkForRef();

  updateRefList();
  }
//}}}
//{{{
void cDpb::unmarkShortTermForRef (sPicture* picture, int diffPicNumMinus1) {

  int picNumX = getPicNumX (picture, diffPicNumMinus1);

  for (uint32_t i = 0; i < refFramesInBuffer; i++) {
    if (picture->picStructure == eFrame) {
      if ((frameStoreRefArray[i]->usedRef == 3) && (frameStoreRefArray[i]->usedLongTermRef == 0)) {
        if (frameStoreRefArray[i]->frame->picNum == picNumX) {
          frameStoreRefArray[i]->unmarkForRef();
          return;
          }
        }
      }
    else {
      if ((frameStoreRefArray[i]->usedRef & 1) && !(frameStoreRefArray[i]->usedLongTermRef & 1)) {
        if (frameStoreRefArray[i]->topField->picNum == picNumX) {
          frameStoreRefArray[i]->topField->usedForRef = 0;
          frameStoreRefArray[i]->usedRef &= 2;
          if (frameStoreRefArray[i]->isUsed == 3)
            frameStoreRefArray[i]->frame->usedForRef = 0;
          return;
          }
        }

      if ((frameStoreRefArray[i]->usedRef & 2) && !(frameStoreRefArray[i]->usedLongTermRef & 2)) {
        if (frameStoreRefArray[i]->botField->picNum == picNumX) {
          frameStoreRefArray[i]->botField->usedForRef = 0;
          frameStoreRefArray[i]->usedRef &= 1;
          if (frameStoreRefArray[i]->isUsed == 3)
            frameStoreRefArray[i]->frame->usedForRef = 0;
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
      if ((frameStoreLongTermRefArray[i]->usedRef == 3) &&
          (frameStoreLongTermRefArray[i]->usedLongTermRef == 3))
        if (frameStoreLongTermRefArray[i]->frame->longTermPicNum == picNum)
          frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
      }
    else {
      if ((frameStoreLongTermRefArray[i]->usedRef & 1) &&
          (frameStoreLongTermRefArray[i]->usedLongTermRef & 1))
        if (frameStoreLongTermRefArray[i]->topField->longTermPicNum == picNum) {
          frameStoreLongTermRefArray[i]->topField->usedForRef = 0;
          frameStoreLongTermRefArray[i]->topField->usedLongTermRef = 0;
          frameStoreLongTermRefArray[i]->usedRef &= 2;
          frameStoreLongTermRefArray[i]->usedLongTermRef &= 2;
          if (frameStoreLongTermRefArray[i]->isUsed == 3) {
            frameStoreLongTermRefArray[i]->frame->usedForRef = 0;
            frameStoreLongTermRefArray[i]->frame->usedLongTermRef = 0;
            }
          return;
          }

      if ((frameStoreLongTermRefArray[i]->usedRef & 2) &&
          (frameStoreLongTermRefArray[i]->usedLongTermRef & 2))
        if (frameStoreLongTermRefArray[i]->botField->longTermPicNum == picNum) {
          frameStoreLongTermRefArray[i]->botField->usedForRef = 0;
          frameStoreLongTermRefArray[i]->botField->usedLongTermRef = 0;
          frameStoreLongTermRefArray[i]->usedRef &= 1;
          frameStoreLongTermRefArray[i]->usedLongTermRef &= 1;
          if (frameStoreLongTermRefArray[i]->isUsed == 3) {
            frameStoreLongTermRefArray[i]->frame->usedForRef = 0;
            frameStoreLongTermRefArray[i]->frame->usedLongTermRef = 0;
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
    if (frameStoreLongTermRefArray[i]->longTermFrameIndex == longTermFrameIndex) {
      if (picStructure == eTopField) {
        if (frameStoreLongTermRefArray[i]->usedLongTermRef == 3)
          frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
        else {
          if (frameStoreLongTermRefArray[i]->usedLongTermRef == 1)
            frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
          else {
            if (markCur) {
              if (lastPictureFrameStore) {
                if ((lastPictureFrameStore != frameStoreLongTermRefArray[i]) ||
                    lastPictureFrameStore->frameNum != curFrameNum)
                  frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
                }
              else
                frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
            }
            else {
              if ((frameStoreLongTermRefArray[i]->frameNum) != (uint32_t)(curPicNum >> 1))
                frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
              }
            }
          }
        }

      if (picStructure == eBotField) {
        if (frameStoreLongTermRefArray[i]->usedLongTermRef == 3)
          frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
        else {
          if (frameStoreLongTermRefArray[i]->usedLongTermRef == 2)
            frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
          else {
            if (markCur) {
              if (lastPictureFrameStore) {
                if ((lastPictureFrameStore != frameStoreLongTermRefArray[i]) ||
                    lastPictureFrameStore->frameNum != curFrameNum)
                  frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
                }
              else
                frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
              }
            else {
              if ((frameStoreLongTermRefArray[i]->frameNum) != (uint32_t)(curPicNum >> 1))
                frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
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
    if (frameStoreLongTermRefArray[i]->longTermFrameIndex == longTermFrameIndex)
      frameStoreLongTermRefArray[i]->unmarkForLongTermRef();
  }
//}}}

//{{{
void cDpb::markPicLongTerm (sPicture* picture, int longTermFrameIndex, int picNumX) {

  if (picture->picStructure == eFrame) {
    for (uint32_t i = 0; i < refFramesInBuffer; i++) {
      if (frameStoreRefArray[i]->usedRef == 3) {
        if (!frameStoreRefArray[i]->frame->usedLongTermRef &&
            (frameStoreRefArray[i]->frame->picNum == picNumX)) {
          frameStoreRefArray[i]->longTermFrameIndex = longTermFrameIndex;
          frameStoreRefArray[i]->frame->longTermFrameIndex = longTermFrameIndex;
          frameStoreRefArray[i]->frame->longTermPicNum = longTermFrameIndex;
          frameStoreRefArray[i]->frame->usedLongTermRef = 1;
          if (frameStoreRefArray[i]->topField && frameStoreRefArray[i]->botField) {
            frameStoreRefArray[i]->topField->longTermFrameIndex = longTermFrameIndex;
            frameStoreRefArray[i]->botField->longTermFrameIndex = longTermFrameIndex;
            frameStoreRefArray[i]->topField->longTermPicNum = longTermFrameIndex;
            frameStoreRefArray[i]->botField->longTermPicNum = longTermFrameIndex;
            frameStoreRefArray[i]->topField->usedLongTermRef = 1;
            frameStoreRefArray[i]->botField->usedLongTermRef = 1;
            }
          frameStoreRefArray[i]->usedLongTermRef = 3;
          return;
          }
        }
      }
    cLog::log (LOGERROR, "refFrame for long term marking not found");
    }

  else {
    int addTop, addBot;
    if (picture->picStructure == eTopField) {
      addTop = 1;
      addBot = 0;
      }
    else {
      addTop = 0;
      addBot = 1;
      }

    for (uint32_t i = 0; i < refFramesInBuffer; i++) {
      if (frameStoreRefArray[i]->usedRef & 1) {
        if ((!frameStoreRefArray[i]->topField->usedLongTermRef) &&
            (frameStoreRefArray[i]->topField->picNum == picNumX)) {
          if ((frameStoreRefArray[i]->usedLongTermRef) &&
              (frameStoreRefArray[i]->longTermFrameIndex != longTermFrameIndex)) {
            cLog::log (LOGERROR, "assigning longTermFrameIndex different from other field");
            }

          frameStoreRefArray[i]->longTermFrameIndex = longTermFrameIndex;
          frameStoreRefArray[i]->topField->longTermFrameIndex = longTermFrameIndex;
          frameStoreRefArray[i]->topField->longTermPicNum = 2 * longTermFrameIndex + addTop;
          frameStoreRefArray[i]->topField->usedLongTermRef = 1;
          frameStoreRefArray[i]->usedLongTermRef |= 1;
          if (frameStoreRefArray[i]->usedLongTermRef == 3) {
            frameStoreRefArray[i]->frame->usedLongTermRef = 1;
            frameStoreRefArray[i]->frame->longTermFrameIndex = longTermFrameIndex;
            frameStoreRefArray[i]->frame->longTermPicNum = longTermFrameIndex;
            }
          return;
          }
        }

      if (frameStoreRefArray[i]->usedRef & 2) {
        if (!frameStoreRefArray[i]->botField->usedLongTermRef &&
            (frameStoreRefArray[i]->botField->picNum == picNumX)) {
          if ((frameStoreRefArray[i]->usedLongTermRef) &&
              (frameStoreRefArray[i]->longTermFrameIndex != longTermFrameIndex))
            cLog::log (LOGERROR, "Warning: assigning longTermFrameIndex different from other field");

          frameStoreRefArray[i]->longTermFrameIndex = longTermFrameIndex;
          frameStoreRefArray[i]->botField->longTermFrameIndex = longTermFrameIndex;
          frameStoreRefArray[i]->botField->longTermPicNum = 2 * longTermFrameIndex + addBot;
          frameStoreRefArray[i]->botField->usedLongTermRef = 1;
          frameStoreRefArray[i]->usedLongTermRef |= 2;
          if (frameStoreRefArray[i]->usedLongTermRef == 3) {
            frameStoreRefArray[i]->frame->usedLongTermRef = 1;
            frameStoreRefArray[i]->frame->longTermFrameIndex = longTermFrameIndex;
            frameStoreRefArray[i]->frame->longTermPicNum = longTermFrameIndex;
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
      if (frameStoreRefArray[i]->usedRef & 1) {
        if (frameStoreRefArray[i]->topField->picNum == picNumX) {
          picStructure = eTopField;
          break;
          }
        }

      if (frameStoreRefArray[i]->usedRef & 2) {
        if (frameStoreRefArray[i]->botField->picNum == picNumX) {
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
