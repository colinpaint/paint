//{{{  includes
#include <limits.h>

#include "global.h"
#include "memory.h"

#include "erc.h"
//}}}
//#define DUMP_DPB
namespace {
  //{{{
  int getPicNumX (sPicture* p, int diffPicNumMinus1) {

    int currPicNum;
    if (p->picStructure == eFrame)
      currPicNum = p->frameNum;
    else
      currPicNum = 2 * p->frameNum + 1;

    return currPicNum - (diffPicNumMinus1 + 1);
    }
  //}}}
  //{{{
  int getDpbSize (cDecoder264* decoder, cSps* activeSps) {

    int pic_size_mb = (activeSps->picWidthMbsMinus1 + 1) * (activeSps->picHeightMapUnitsMinus1 + 1) * (activeSps->frameMbOnly?1:2);
    int size = 0;

    switch (activeSps->levelIdc) {
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
        if (!activeSps->isFrextProfile() && (activeSps->constrainedSet3flag == 1))
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

    if (activeSps->hasVui && activeSps->vuiSeqParams.bitstream_restrictionFlag) {
      int size_vui;
      if ((int)activeSps->vuiSeqParams.max_dec_frame_buffering > size)
        cDecoder264::error ("max_dec_frame_buffering larger than MaxDpbSize");

      size_vui = imax (1, activeSps->vuiSeqParams.max_dec_frame_buffering);
  #ifdef _DEBUG
      if(size_vui < size)
        printf("Warning: max_dec_frame_buffering(%d) is less than DPB size(%d) calculated from Profile/Level.\n", size_vui, size);
  #endif
      size = size_vui;
      }

    return size;
    }
  //}}}
  }

// dpb
//{{{
void cDpb::getSmallestPoc (int* poc, int* pos) {

  if (usedSize<1)
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
void cDpb::updateRefList() {

  uint32_t i, j;
  for (i = 0, j = 0; i < usedSize; i++)
    if (frameStore[i]->isShortTermReference())
      frameStoreRef[j++]=frameStore[i];

  refFramesInBuffer = j;

  while (j < size)
    frameStoreRef[j++] = NULL;
  }
//}}}
//{{{
void cDpb::updateLongTermRefList() {

  uint32_t i, j;
  for (i = 0, j = 0; i < usedSize; i++)
    if (frameStore[i]->isLongTermReference())
      frameStoreLongTermRef[j++] = frameStore[i];

  longTermRefFramesInBuffer = j;

  while (j < size)
    frameStoreLongTermRef[j++] = NULL;
  }
//}}}

//{{{
void cDpb::initDpb (cDecoder264* decoder, int type) {

  cSps* activeSps = decoder->activeSps;

  if (initDone)
    freeDpb();

  size = getDpbSize (decoder, activeSps) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  numRefFrames = activeSps->numRefFrames;
  if (size < activeSps->numRefFrames)
    cDecoder264::error ("DPB size at specified level is smaller than the specified number of reference frames\n");

  usedSize = 0;
  lastPictureFrameStore = NULL;

  refFramesInBuffer = 0;
  longTermRefFramesInBuffer = 0;

  frameStore = (cFrameStore**)calloc (size, sizeof (cFrameStore*));
  frameStoreRef = (cFrameStore**)calloc (size, sizeof (cFrameStore*));
  frameStoreLongTermRef = (cFrameStore**)calloc (size, sizeof (cFrameStore*));

  for (uint32_t i = 0; i < size; i++) {
    frameStore[i] = new cFrameStore();
    frameStoreRef[i] = NULL;
    frameStoreLongTermRef[i] = NULL;
    }

  // allocate a dummy storable picture
  if (!decoder->noReferencePicture) {
    decoder->noReferencePicture = allocPicture (decoder, eFrame, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);
    decoder->noReferencePicture->topField = decoder->noReferencePicture;
    decoder->noReferencePicture->botField = decoder->noReferencePicture;
    decoder->noReferencePicture->frame = decoder->noReferencePicture;
    }
  lastOutputPoc = INT_MIN;
  decoder->lastHasMmco5 = 0;
  initDone = 1;

  // picture error conceal
  if ((decoder->concealMode != 0) && !decoder->lastOutFramestore)
    decoder->lastOutFramestore = new cFrameStore();
  }
//}}}
//{{{
void cDpb::reInitDpb (cDecoder264* decoder, int type) {

  cSps* activeSps = decoder->activeSps;
  int dpbSize = getDpbSize (decoder, activeSps) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  numRefFrames = activeSps->numRefFrames;

  if (dpbSize > (int)size) {
    if (size < activeSps->numRefFrames)
      cDecoder264::error ("DPB size at specified level is smaller than the specified number of reference frames\n");

    frameStore = (cFrameStore**)realloc (frameStore, dpbSize * sizeof (cFrameStore*));
    frameStoreRef = (cFrameStore**)realloc(frameStoreRef, dpbSize * sizeof (cFrameStore*));
    frameStoreLongTermRef = (cFrameStore**)realloc(frameStoreLongTermRef, dpbSize * sizeof (cFrameStore*));

    for (int i = size; i < dpbSize; i++) {
      frameStore[i] = new cFrameStore();
      frameStoreRef[i] = NULL;
      frameStoreLongTermRef[i] = NULL;
      }

    size = dpbSize;
    lastOutputPoc = INT_MIN;
    initDone = 1;
    }
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

  lastOutputPoc = INT_MIN;
  }
//}}}
//{{{
int cDpb::removeUnusedDpb() {

  // check for frames that were already output and no longer used for reference
  for (uint32_t i = 0; i < usedSize; i++) {
    if (frameStore[i]->isOutput && (!frameStore[i]->isReference())) {
      removeFrameDpb (i);
      return 1;
      }
    }

  return 0;
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
    if (picture->usedForReference && (picture->adaptRefPicBufFlag))
      adaptiveMemoryManagement (picture);
    }

  if ((picture->picStructure == eTopField) || (picture->picStructure == eBotField)) {
    // check for frame store with same pic_number
    if (lastPictureFrameStore) {
      if ((int)lastPictureFrameStore->frameNum == picture->picNum) {
        if (((picture->picStructure == eTopField) && (lastPictureFrameStore->isUsed == 2))||
            ((picture->picStructure == eBotField) && (lastPictureFrameStore->isUsed == 1))) {
          if ((picture->usedForReference && (lastPictureFrameStore->usedOrigReference != 0)) ||
              (!picture->usedForReference && (lastPictureFrameStore->usedOrigReference == 0))) {
            lastPictureFrameStore->insertPictureDpb (decoder, picture);
            updateRefList();
            updateLongTermRefList();
            dumpDpb();
            lastPictureFrameStore = NULL;
            return;
            }
          }
        }
      }
    }

  // this is a frame or a field which has no stored complementary field sliding window, if necessary
  if (!picture->isIDR && (picture->usedForReference && !picture->adaptRefPicBufFlag))
    slidingWindowMemoryManagement (picture);

  // picture error conceal
  if (decoder->concealMode != 0)
    for (uint32_t i = 0; i < size; i++)
      if (frameStore[i]->usedReference)
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
    if (!picture->usedForReference) {
      getSmallestPoc (&poc, &pos);
      if ((-1 == pos) || (picture->poc < poc)) {
        decoder->directOutput (picture);
        return;
        }
      }

    // flush a frame
    outputDpbFrame();
    }

  // check for duplicate frame number in int16_t term reference buffer
  if ((picture->usedForReference) && (!picture->usedLongTerm))
    for (uint32_t i = 0; i < refFramesInBuffer; i++)
      if (frameStoreRef[i]->frameNum == picture->frameNum)
        cDecoder264::error ("duplicate frameNum in int16_t-term reference picture buffer");

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
  dumpDpb();
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
  frameStore1->usedLongTerm = 0;
  frameStore1->usedReference = 0;
  frameStore1->usedOrigReference = 0;

  // move empty framestore to end of buffer
  for (uint32_t i = pos; i < usedSize-1; i++)
    frameStore[i] = frameStore[i+1];

  frameStore[usedSize-1] = frameStore1;
  usedSize--;
  }
//}}}
//{{{
void cDpb::freeDpb () {

  if (frameStore) {
    for (uint32_t i = 0; i < size; i++)
      delete frameStore[i];
    free (frameStore);
    frameStore = NULL;
    }

  if (frameStoreRef)
    free (frameStoreRef);
  if (frameStoreLongTermRef)
    free (frameStoreLongTermRef);


  lastOutputPoc = INT_MIN;
  initDone = 0;

  // picture error conceal
  if (decoder->concealMode != 0 || decoder->lastOutFramestore)
    delete decoder->lastOutFramestore;

  if (decoder->noReferencePicture) {
    freePicture (decoder->noReferencePicture);
    decoder->noReferencePicture = NULL;
    }
  }
//}}}

//{{{
sPicture* cDpb::getShortTermPic (cSlice* slice, int picNum) {

  for (uint32_t i = 0; i < refFramesInBuffer; i++) {
    if (slice->picStructure == eFrame) {
      if (frameStoreRef[i]->usedReference == 3)
        if (!frameStoreRef[i]->frame->usedLongTerm &&
            (frameStoreRef[i]->frame->picNum == picNum))
          return frameStoreRef[i]->frame;
      }
    else {
      if (frameStoreRef[i]->usedReference & 1)
        if (!frameStoreRef[i]->topField->usedLongTerm &&
            (frameStoreRef[i]->topField->picNum == picNum))
          return frameStoreRef[i]->topField;

      if (frameStoreRef[i]->usedReference & 2)
        if (!frameStoreRef[i]->botField->usedLongTerm &&
            (frameStoreRef[i]->botField->picNum == picNum))
          return frameStoreRef[i]->botField;
      }
    }

  return slice->decoder->noReferencePicture;
  }
//}}}
//{{{
sPicture* cDpb::getLongTermPic (cSlice* slice, int longtermPicNum) {

  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++) {
    if (slice->picStructure == eFrame) {
      if (frameStoreLongTermRef[i]->usedReference == 3)
        if ((frameStoreLongTermRef[i]->frame->usedLongTerm) &&
            (frameStoreLongTermRef[i]->frame->longTermPicNum == longtermPicNum))
          return frameStoreLongTermRef[i]->frame;
      }

    else {
      if (frameStoreLongTermRef[i]->usedReference & 1)
        if ((frameStoreLongTermRef[i]->topField->usedLongTerm) &&
            (frameStoreLongTermRef[i]->topField->longTermPicNum == longtermPicNum))
          return frameStoreLongTermRef[i]->topField;

      if (frameStoreLongTermRef[i]->usedReference & 2)
        if ((frameStoreLongTermRef[i]->botField->usedLongTerm) &&
            (frameStoreLongTermRef[i]->botField->longTermPicNum == longtermPicNum))
          return frameStoreLongTermRef[i]->botField;
      }
    }

  return NULL;
  }
//}}}
//{{{
sPicture* cDpb::getLastPicRefFromDpb() {

  int usedSize1 = usedSize - 1;
  for (int i = usedSize1; i >= 0; i--)
    if (frameStore[i]->isUsed == 3)
      if (frameStore[i]->frame->usedForReference && !frameStore[i]->frame->usedLongTerm)
        return frameStore[i]->frame;

  return NULL;
  }
//}}}

// private
//{{{
void cDpb::dumpDpb() {

#ifdef DUMP_DPB
  for (uint32_t i = 0; i < usedSize; i++) {
    printf ("(");
    printf ("fn=%d  ", frameStore[i]->frameNum);
    if (frameStore[i]->isUsed & 1) {
      if (frameStore[i]->topField)
        printf ("T: poc=%d  ", frameStore[i]->topField->poc);
      else
        printf ("T: poc=%d  ", frameStore[i]->frame->topPoc);
      }

    if (frameStore[i]->isUsed & 2) {
      if (frameStore[i]->botField)
        printf ("B: poc=%d  ", frameStore[i]->botField->poc);
      else
        printf ("B: poc=%d  ", frameStore[i]->frame->botPoc);
      }

    if (frameStore[i]->isUsed == 3)
      printf ("F: poc=%d  ", frameStore[i]->frame->poc);
    printf ("G: poc=%d)  ", frameStore[i]->poc);

    if (frameStore[i]->isReference)
      printf ("ref (%d) ", frameStore[i]->isReference);
    if (frameStore[i]->usedLongTerm)
      printf ("lt_ref (%d) ", frameStore[i]->isReference);
    if (frameStore[i]->isOutput)
      printf ("out  ");
    if (frameStore[i]->isUsed == 3)
      if (rameStore[i]->frame->nonExisting)
        printf ("ne  ");

    printf ("\n");
    }
#endif
  }
//}}}

//{{{
int cDpb::outputDpbFrame() {

  // diagnostics
  if (usedSize < 1)
    cDecoder264::error ("Cannot output frame, DPB empty");

  // find smallest POC
  int poc;
  int pos;
  getSmallestPoc (&poc, &pos);
  if (pos == -1)
    return 0;

  // picture error conceal
  if (decoder->concealMode != 0) {
    if (lastOutputPoc == 0)
      writeLostRefAfterIdr (this, pos);
    writeLostNonRefPic (this, poc);
    }

  decoder->writeStoredFrame (frameStore[pos]);

  // picture error conceal
  if(decoder->concealMode == 0)
    if (lastOutputPoc >= poc)
      cDecoder264::error ("output POC must be in ascending order");

  lastOutputPoc = poc;

  // free frame store and move empty store to end of buffer
  if (!frameStore[pos]->isReference())
    removeFrameDpb (pos);

  return 1;
  }
//}}}
//{{{
void cDpb::checkNumDpbFrames() {

  if ((int)(longTermRefFramesInBuffer + refFramesInBuffer) > imax (1, numRefFrames))
    cDecoder264::error ("Max. number of reference frames exceeded. Invalid stream");
  }
//}}}

//{{{
void cDpb::adaptiveMemoryManagement (sPicture* picture) {

  sDecodedRefPicMark* tmp_drpm;

  decoder->lastHasMmco5 = 0;

  while (picture->decRefPicMarkBuffer) {
    tmp_drpm = picture->decRefPicMarkBuffer;
    switch (tmp_drpm->memManagement) {
      //{{{
      case 0:
        if (tmp_drpm->next != NULL)
          cDecoder264::error ("memManagement = 0 not last operation in buffer");
        break;
      //}}}
      //{{{
      case 1:
        unmarkShortTermForRef (picture, tmp_drpm->diffPicNumMinus1);
        updateRefList();
        break;
      //}}}
      //{{{
      case 2:
        unmarkLongTermForRef (picture, tmp_drpm->longTermPicNum);
        updateLongTermRefList();
        break;
      //}}}
      //{{{
      case 3:
        assignLongTermFrameIndex (picture, tmp_drpm->diffPicNumMinus1, tmp_drpm->longTermFrameIndex);
        updateRefList();
        updateLongTermRefList();
        break;
      //}}}
      //{{{
      case 4:
        updateMaxLongTermFrameIndex (tmp_drpm->maxLongTermFrameIndexPlus1);
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
        markCurPicLongTerm (picture, tmp_drpm->longTermFrameIndex);
        checkNumDpbFrames();
        break;
      //}}}
      //{{{
      default:
        cDecoder264::error ("invalid memManagement in buffer");
      //}}}
      }
    picture->decRefPicMarkBuffer = tmp_drpm->next;
    free (tmp_drpm);
    }

  if (decoder->lastHasMmco5 ) {
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

  // if this is a reference pic with sliding window, unmark first ref frame
  if (refFramesInBuffer == imax (1, numRefFrames) - longTermRefFramesInBuffer) {
    for (uint32_t i = 0; i < usedSize; i++) {
      if (frameStore[i]->usedReference && (!(frameStore[i]->usedLongTerm))) {
        frameStore[i]->unmarkForRef();
        updateRefList();
        break;
        }
      }
    }

  picture->usedLongTerm = 0;
  }
//}}}
//{{{
void cDpb::idrMemoryManagement (sPicture* picture) {

  if (picture->noOutputPriorPicFlag) {
    // free all stored pictures
    for (uint32_t i = 0; i < usedSize; i++) {
      // reset all reference settings
      delete frameStore[i];
      frameStore[i] = new cFrameStore();
      }
    for (uint32_t i = 0; i < refFramesInBuffer; i++)
      frameStoreRef[i]=NULL;
    for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++)
      frameStoreLongTermRef[i]=NULL;
    usedSize = 0;
    }
  else
    flushDpb();

  lastPictureFrameStore = NULL;

  updateRefList();
  updateLongTermRefList();
  lastOutputPoc = INT_MIN;

  if (picture->longTermRefFlag) {
    maxLongTermPicIndex = 0;
    picture->usedLongTerm = 1;
    picture->longTermFrameIndex = 0;
    }
  else {
    maxLongTermPicIndex = -1;
    picture->usedLongTerm = 0;
    }
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
void cDpb::unmarkLongTermFrameForRefByFrameIndex (int longTermFrameIndex) {

  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++)
    if (frameStoreLongTermRef[i]->longTermFrameIndex == longTermFrameIndex)
      frameStoreLongTermRef[i]->unmarkForLongTermRef();
  }
//}}}
//{{{
void cDpb::unmarkLongTermForRef (sPicture* p, int longTermPicNum) {

  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++) {
    if (p->picStructure == eFrame) {
      if ((frameStoreLongTermRef[i]->usedReference == 3) && (frameStoreLongTermRef[i]->usedLongTerm == 3))
        if (frameStoreLongTermRef[i]->frame->longTermPicNum == longTermPicNum)
          frameStoreLongTermRef[i]->unmarkForLongTermRef();
      }
    else {
      if ((frameStoreLongTermRef[i]->usedReference & 1) && ((frameStoreLongTermRef[i]->usedLongTerm & 1)))
        if (frameStoreLongTermRef[i]->topField->longTermPicNum == longTermPicNum) {
          frameStoreLongTermRef[i]->topField->usedForReference = 0;
          frameStoreLongTermRef[i]->topField->usedLongTerm = 0;
          frameStoreLongTermRef[i]->usedReference &= 2;
          frameStoreLongTermRef[i]->usedLongTerm &= 2;
          if (frameStoreLongTermRef[i]->isUsed == 3) {
            frameStoreLongTermRef[i]->frame->usedForReference = 0;
            frameStoreLongTermRef[i]->frame->usedLongTerm = 0;
            }
          return;
          }

      if ((frameStoreLongTermRef[i]->usedReference & 2) && ((frameStoreLongTermRef[i]->usedLongTerm & 2)))
        if (frameStoreLongTermRef[i]->botField->longTermPicNum == longTermPicNum) {
          frameStoreLongTermRef[i]->botField->usedForReference = 0;
          frameStoreLongTermRef[i]->botField->usedLongTerm = 0;
          frameStoreLongTermRef[i]->usedReference &= 1;
          frameStoreLongTermRef[i]->usedLongTerm &= 1;
          if (frameStoreLongTermRef[i]->isUsed == 3) {
            frameStoreLongTermRef[i]->frame->usedForReference = 0;
            frameStoreLongTermRef[i]->frame->usedLongTerm = 0;
            }
          return;
          }
      }
    }
  }
//}}}
//{{{
void cDpb::unmarkShortTermForRef (sPicture* p, int diffPicNumMinus1)
{
  int picNumX = getPicNumX(p, diffPicNumMinus1);

  for (uint32_t i = 0; i < refFramesInBuffer; i++) {
    if (p->picStructure == eFrame) {
      if ((frameStoreRef[i]->usedReference == 3) && (frameStoreRef[i]->usedLongTerm == 0)) {
        if (frameStoreRef[i]->frame->picNum == picNumX) {
          frameStoreRef[i]->unmarkForRef();
          return;
          }
        }
      }
    else {
      if ((frameStoreRef[i]->usedReference & 1) && (!(frameStoreRef[i]->usedLongTerm & 1))) {
        if (frameStoreRef[i]->topField->picNum == picNumX) {
          frameStoreRef[i]->topField->usedForReference = 0;
          frameStoreRef[i]->usedReference &= 2;
          if (frameStoreRef[i]->isUsed == 3)
            frameStoreRef[i]->frame->usedForReference = 0;
          return;
          }
        }

      if ((frameStoreRef[i]->usedReference & 2) && (!(frameStoreRef[i]->usedLongTerm & 2))) {
        if (frameStoreRef[i]->botField->picNum == picNumX) {
          frameStoreRef[i]->botField->usedForReference = 0;
          frameStoreRef[i]->usedReference &= 1;
          if (frameStoreRef[i]->isUsed == 3)
            frameStoreRef[i]->frame->usedForReference = 0;
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
void cDpb::unmarkLongTermFieldRefFrameIndex (ePicStructure picStructure, int longTermFrameIndex,
                                              int mark_current, uint32_t curr_frame_num, int curr_pic_num) {

  if (curr_pic_num < 0)
    curr_pic_num += (2 * decoder->coding.maxFrameNum);

  for (uint32_t i = 0; i < longTermRefFramesInBuffer; i++) {
    if (frameStoreLongTermRef[i]->longTermFrameIndex == longTermFrameIndex) {
      if (picStructure == eTopField) {
        if (frameStoreLongTermRef[i]->usedLongTerm == 3)
          frameStoreLongTermRef[i]->unmarkForLongTermRef();
        else {
          if (frameStoreLongTermRef[i]->usedLongTerm == 1)
            frameStoreLongTermRef[i]->unmarkForLongTermRef();
          else {
            if (mark_current) {
              if (lastPictureFrameStore) {
                if ((lastPictureFrameStore != frameStoreLongTermRef[i]) ||
                    lastPictureFrameStore->frameNum != curr_frame_num)
                  frameStoreLongTermRef[i]->unmarkForLongTermRef();
                }
              else
                frameStoreLongTermRef[i]->unmarkForLongTermRef();
            }
            else {
              if ((frameStoreLongTermRef[i]->frameNum) != (uint32_t)(curr_pic_num >> 1))
                frameStoreLongTermRef[i]->unmarkForLongTermRef();
              }
            }
          }
        }

      if (picStructure == eBotField) {
        if (frameStoreLongTermRef[i]->usedLongTerm == 3)
          frameStoreLongTermRef[i]->unmarkForLongTermRef();
        else {
          if (frameStoreLongTermRef[i]->usedLongTerm == 2)
            frameStoreLongTermRef[i]->unmarkForLongTermRef();
          else {
            if (mark_current) {
              if (lastPictureFrameStore) {
                if ((lastPictureFrameStore != frameStoreLongTermRef[i]) ||
                    lastPictureFrameStore->frameNum != curr_frame_num)
                  frameStoreLongTermRef[i]->unmarkForLongTermRef();
                }
              else
                frameStoreLongTermRef[i]->unmarkForLongTermRef();
              }
            else {
              if ((frameStoreLongTermRef[i]->frameNum) != (uint32_t)(curr_pic_num >> 1))
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
void cDpb::unmarkAllShortTermForRef() {

  for (uint32_t i = 0; i < refFramesInBuffer; i++)
    frameStoreRef[i]->unmarkForRef();
  updateRefList();
  }
//}}}

//{{{
void cDpb::markPicLongTerm (sPicture* p, int longTermFrameIndex, int picNumX) {

  int addTop, addBot;

  if (p->picStructure == eFrame) {
    for (uint32_t i = 0; i < refFramesInBuffer; i++) {
      if (frameStoreRef[i]->usedReference == 3) {
        if ((!frameStoreRef[i]->frame->usedLongTerm)&&(frameStoreRef[i]->frame->picNum == picNumX)) {
          frameStoreRef[i]->longTermFrameIndex = frameStoreRef[i]->frame->longTermFrameIndex
                                             = longTermFrameIndex;
          frameStoreRef[i]->frame->longTermPicNum = longTermFrameIndex;
          frameStoreRef[i]->frame->usedLongTerm = 1;

          if (frameStoreRef[i]->topField && frameStoreRef[i]->botField) {
            frameStoreRef[i]->topField->longTermFrameIndex = frameStoreRef[i]->botField->longTermFrameIndex
                                                          = longTermFrameIndex;
            frameStoreRef[i]->topField->longTermPicNum = longTermFrameIndex;
            frameStoreRef[i]->botField->longTermPicNum = longTermFrameIndex;

            frameStoreRef[i]->topField->usedLongTerm = frameStoreRef[i]->botField->usedLongTerm = 1;
            }
          frameStoreRef[i]->usedLongTerm = 3;
          return;
          }
        }
      }
    printf ("Warning: reference frame for long term marking not found\n");
    }

  else {
    if (p->picStructure == eTopField) {
      addTop = 1;
      addBot = 0;
      }
    else {
      addTop = 0;
      addBot = 1;
      }

    for (uint32_t i = 0; i < refFramesInBuffer; i++) {
      if (frameStoreRef[i]->usedReference & 1) {
        if ((!frameStoreRef[i]->topField->usedLongTerm) &&
            (frameStoreRef[i]->topField->picNum == picNumX)) {
          if ((frameStoreRef[i]->usedLongTerm) &&
              (frameStoreRef[i]->longTermFrameIndex != longTermFrameIndex)) {
            printf ("Warning: assigning longTermFrameIndex different from other field\n");
            }

          frameStoreRef[i]->longTermFrameIndex = frameStoreRef[i]->topField->longTermFrameIndex = longTermFrameIndex;
          frameStoreRef[i]->topField->longTermPicNum = 2 * longTermFrameIndex + addTop;
          frameStoreRef[i]->topField->usedLongTerm = 1;
          frameStoreRef[i]->usedLongTerm |= 1;
          if (frameStoreRef[i]->usedLongTerm == 3) {
            frameStoreRef[i]->frame->usedLongTerm = 1;
            frameStoreRef[i]->frame->longTermFrameIndex = frameStoreRef[i]->frame->longTermPicNum = longTermFrameIndex;
            }
          return;
          }
        }

      if (frameStoreRef[i]->usedReference & 2) {
        if ((!frameStoreRef[i]->botField->usedLongTerm) &&
            (frameStoreRef[i]->botField->picNum == picNumX)) {
          if ((frameStoreRef[i]->usedLongTerm) &&
              (frameStoreRef[i]->longTermFrameIndex != longTermFrameIndex))
            printf ("Warning: assigning longTermFrameIndex different from other field\n");

          frameStoreRef[i]->longTermFrameIndex = frameStoreRef[i]->botField->longTermFrameIndex
                                              = longTermFrameIndex;
          frameStoreRef[i]->botField->longTermPicNum = 2 * longTermFrameIndex + addBot;
          frameStoreRef[i]->botField->usedLongTerm = 1;
          frameStoreRef[i]->usedLongTerm |= 2;
          if (frameStoreRef[i]->usedLongTerm == 3) {
            frameStoreRef[i]->frame->usedLongTerm = 1;
            frameStoreRef[i]->frame->longTermFrameIndex = frameStoreRef[i]->frame->longTermPicNum = longTermFrameIndex;
            }
          return;
          }
        }
      }
    printf ("Warning: reference field for long term marking not found\n");
    }
  }
//}}}
//{{{
void cDpb::markCurPicLongTerm (sPicture* p, int longTermFrameIndex) {

  // remove long term pictures with same longTermFrameIndex
  if (p->picStructure == eFrame)
    unmarkLongTermFrameForRefByFrameIndex (longTermFrameIndex);
  else
    unmarkLongTermFieldRefFrameIndex (p->picStructure, longTermFrameIndex, 1, p->picNum, 0);

  p->usedLongTerm = 1;
  p->longTermFrameIndex = longTermFrameIndex;
  }
//}}}

//{{{
void cDpb::assignLongTermFrameIndex (sPicture* p, int diffPicNumMinus1, int longTermFrameIndex) {

  int picNumX = getPicNumX(p, diffPicNumMinus1);

  // remove frames/fields with same longTermFrameIndex
  if (p->picStructure == eFrame)
    unmarkLongTermFrameForRefByFrameIndex (longTermFrameIndex);

  else {
    ePicStructure picStructure = eFrame;
    for (uint32_t i = 0; i < refFramesInBuffer; i++) {
      if (frameStoreRef[i]->usedReference & 1) {
        if (frameStoreRef[i]->topField->picNum == picNumX) {
          picStructure = eTopField;
          break;
          }
        }

      if (frameStoreRef[i]->usedReference & 2) {
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

  markPicLongTerm (p, longTermFrameIndex, picNumX);
  }
//}}}
