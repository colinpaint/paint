//{{{  includes
#include <limits.h>

#include "global.h"
#include "memory.h"

#include "erc.h"
//}}}
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
  void checkNumDpbFrames (sDpb* dpb) {

    if ((int)(dpb->longTermRefFramesInBuffer + dpb->refFramesInBuffer) > imax (1, dpb->numRefFrames))
      cDecoder264::error ("Max. number of reference frames exceeded. Invalid stream");
    }
  //}}}

  //{{{
  sPicture* getLongTermPic (cSlice* slice, sDpb* dpb, int longtermPicNum) {

    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (slice->picStructure == eFrame) {
        if (dpb->frameStoreLongTermRef[i]->usedReference == 3)
          if ((dpb->frameStoreLongTermRef[i]->frame->usedLongTerm) &&
              (dpb->frameStoreLongTermRef[i]->frame->longTermPicNum == longtermPicNum))
            return dpb->frameStoreLongTermRef[i]->frame;
        }

      else {
        if (dpb->frameStoreLongTermRef[i]->usedReference & 1)
          if ((dpb->frameStoreLongTermRef[i]->topField->usedLongTerm) &&
              (dpb->frameStoreLongTermRef[i]->topField->longTermPicNum == longtermPicNum))
            return dpb->frameStoreLongTermRef[i]->topField;

        if (dpb->frameStoreLongTermRef[i]->usedReference & 2)
          if ((dpb->frameStoreLongTermRef[i]->botField->usedLongTerm) &&
              (dpb->frameStoreLongTermRef[i]->botField->longTermPicNum == longtermPicNum))
            return dpb->frameStoreLongTermRef[i]->botField;
        }
      }

    return NULL;
    }
  //}}}
  //{{{
  void updateMaxLongTermFrameIndex (sDpb* dpb, int maxLongTermFrameIndexPlus1) {

    dpb->maxLongTermPicIndex = maxLongTermFrameIndexPlus1 - 1;

    // check for invalid frames
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      if (dpb->frameStoreLongTermRef[i]->longTermFrameIndex > dpb->maxLongTermPicIndex)
        dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
    }
  //}}}
  //{{{
  void unmarkLongTermFieldRefFrameIndex (sDpb* dpb, ePicStructure picStructure, int longTermFrameIndex,
                                                int mark_current, uint32_t curr_frame_num, int curr_pic_num) {

    cDecoder264* decoder = dpb->decoder;

    if (curr_pic_num < 0)
      curr_pic_num += (2 * decoder->coding.maxFrameNum);

    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->frameStoreLongTermRef[i]->longTermFrameIndex == longTermFrameIndex) {
        if (picStructure == eTopField) {
          if (dpb->frameStoreLongTermRef[i]->usedLongTerm == 3)
            dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
          else {
            if (dpb->frameStoreLongTermRef[i]->usedLongTerm == 1)
              dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
            else {
              if (mark_current) {
                if (dpb->lastPictureFrameStore) {
                  if ((dpb->lastPictureFrameStore != dpb->frameStoreLongTermRef[i]) ||
                      dpb->lastPictureFrameStore->frameNum != curr_frame_num)
                    dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
                  }
                else
                  dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
              }
              else {
                if ((dpb->frameStoreLongTermRef[i]->frameNum) != (uint32_t)(curr_pic_num >> 1))
                  dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
                }
              }
            }
          }

        if (picStructure == eBotField) {
          if (dpb->frameStoreLongTermRef[i]->usedLongTerm == 3)
            dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
          else {
            if (dpb->frameStoreLongTermRef[i]->usedLongTerm == 2)
              dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
            else {
              if (mark_current) {
                if (dpb->lastPictureFrameStore) {
                  if ((dpb->lastPictureFrameStore != dpb->frameStoreLongTermRef[i]) ||
                      dpb->lastPictureFrameStore->frameNum != curr_frame_num)
                    dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
                  }
                else
                  dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
                }
              else {
                if ((dpb->frameStoreLongTermRef[i]->frameNum) != (uint32_t)(curr_pic_num >> 1))
                  dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
                }
              }
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void unmarkLongTermFrameForRefByFrameIndex (sDpb* dpb, int longTermFrameIndex) {

    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      if (dpb->frameStoreLongTermRef[i]->longTermFrameIndex == longTermFrameIndex)
        dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
    }
  //}}}
  //{{{
  void unmarkLongTermForRef (sDpb* dpb, sPicture* p, int longTermPicNum) {

    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (p->picStructure == eFrame) {
        if ((dpb->frameStoreLongTermRef[i]->usedReference == 3) && (dpb->frameStoreLongTermRef[i]->usedLongTerm == 3))
          if (dpb->frameStoreLongTermRef[i]->frame->longTermPicNum == longTermPicNum)
            dpb->frameStoreLongTermRef[i]->unmarkForLongTermRef();
        }
      else {
        if ((dpb->frameStoreLongTermRef[i]->usedReference & 1) && ((dpb->frameStoreLongTermRef[i]->usedLongTerm & 1)))
          if (dpb->frameStoreLongTermRef[i]->topField->longTermPicNum == longTermPicNum) {
            dpb->frameStoreLongTermRef[i]->topField->usedForReference = 0;
            dpb->frameStoreLongTermRef[i]->topField->usedLongTerm = 0;
            dpb->frameStoreLongTermRef[i]->usedReference &= 2;
            dpb->frameStoreLongTermRef[i]->usedLongTerm &= 2;
            if (dpb->frameStoreLongTermRef[i]->isUsed == 3) {
              dpb->frameStoreLongTermRef[i]->frame->usedForReference = 0;
              dpb->frameStoreLongTermRef[i]->frame->usedLongTerm = 0;
              }
            return;
            }

        if ((dpb->frameStoreLongTermRef[i]->usedReference & 2) && ((dpb->frameStoreLongTermRef[i]->usedLongTerm & 2)))
          if (dpb->frameStoreLongTermRef[i]->botField->longTermPicNum == longTermPicNum) {
            dpb->frameStoreLongTermRef[i]->botField->usedForReference = 0;
            dpb->frameStoreLongTermRef[i]->botField->usedLongTerm = 0;
            dpb->frameStoreLongTermRef[i]->usedReference &= 1;
            dpb->frameStoreLongTermRef[i]->usedLongTerm &= 1;
            if (dpb->frameStoreLongTermRef[i]->isUsed == 3) {
              dpb->frameStoreLongTermRef[i]->frame->usedForReference = 0;
              dpb->frameStoreLongTermRef[i]->frame->usedLongTerm = 0;
              }
            return;
            }
        }
      }
    }
  //}}}
  //{{{
  void unmarkShortTermForRef (sDpb* dpb, sPicture* p, int diffPicNumMinus1)
  {
    int picNumX = getPicNumX(p, diffPicNumMinus1);

    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
      if (p->picStructure == eFrame) {
        if ((dpb->frameStoreRef[i]->usedReference == 3) && (dpb->frameStoreRef[i]->usedLongTerm == 0)) {
          if (dpb->frameStoreRef[i]->frame->picNum == picNumX) {
            dpb->frameStoreRef[i]->unmarkForRef();
            return;
            }
          }
        }
      else {
        if ((dpb->frameStoreRef[i]->usedReference & 1) && (!(dpb->frameStoreRef[i]->usedLongTerm & 1))) {
          if (dpb->frameStoreRef[i]->topField->picNum == picNumX) {
            dpb->frameStoreRef[i]->topField->usedForReference = 0;
            dpb->frameStoreRef[i]->usedReference &= 2;
            if (dpb->frameStoreRef[i]->isUsed == 3)
              dpb->frameStoreRef[i]->frame->usedForReference = 0;
            return;
            }
          }

        if ((dpb->frameStoreRef[i]->usedReference & 2) && (!(dpb->frameStoreRef[i]->usedLongTerm & 2))) {
          if (dpb->frameStoreRef[i]->botField->picNum == picNumX) {
            dpb->frameStoreRef[i]->botField->usedForReference = 0;
            dpb->frameStoreRef[i]->usedReference &= 1;
            if (dpb->frameStoreRef[i]->isUsed == 3)
              dpb->frameStoreRef[i]->frame->usedForReference = 0;
            return;
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void unmarkAllLongTermForRef (sDpb* dpb) {
    updateMaxLongTermFrameIndex (dpb, 0);
    }
  //}}}
  //{{{
  void unmarkAllShortTermForRef (sDpb* dpb) {

    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      dpb->frameStoreRef[i]->unmarkForRef();
    dpb->updateRefList();
    }
  //}}}
  //{{{
  void markPicLongTerm (sDpb* dpb, sPicture* p, int longTermFrameIndex, int picNumX) {

    int addTop, addBot;

    if (p->picStructure == eFrame) {
      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
        if (dpb->frameStoreRef[i]->usedReference == 3) {
          if ((!dpb->frameStoreRef[i]->frame->usedLongTerm)&&(dpb->frameStoreRef[i]->frame->picNum == picNumX)) {
            dpb->frameStoreRef[i]->longTermFrameIndex = dpb->frameStoreRef[i]->frame->longTermFrameIndex
                                               = longTermFrameIndex;
            dpb->frameStoreRef[i]->frame->longTermPicNum = longTermFrameIndex;
            dpb->frameStoreRef[i]->frame->usedLongTerm = 1;

            if (dpb->frameStoreRef[i]->topField && dpb->frameStoreRef[i]->botField) {
              dpb->frameStoreRef[i]->topField->longTermFrameIndex = dpb->frameStoreRef[i]->botField->longTermFrameIndex
                                                            = longTermFrameIndex;
              dpb->frameStoreRef[i]->topField->longTermPicNum = longTermFrameIndex;
              dpb->frameStoreRef[i]->botField->longTermPicNum = longTermFrameIndex;

              dpb->frameStoreRef[i]->topField->usedLongTerm = dpb->frameStoreRef[i]->botField->usedLongTerm = 1;
              }
            dpb->frameStoreRef[i]->usedLongTerm = 3;
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

      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
        if (dpb->frameStoreRef[i]->usedReference & 1) {
          if ((!dpb->frameStoreRef[i]->topField->usedLongTerm) &&
              (dpb->frameStoreRef[i]->topField->picNum == picNumX)) {
            if ((dpb->frameStoreRef[i]->usedLongTerm) &&
                (dpb->frameStoreRef[i]->longTermFrameIndex != longTermFrameIndex)) {
              printf ("Warning: assigning longTermFrameIndex different from other field\n");
              }

            dpb->frameStoreRef[i]->longTermFrameIndex = dpb->frameStoreRef[i]->topField->longTermFrameIndex = longTermFrameIndex;
            dpb->frameStoreRef[i]->topField->longTermPicNum = 2 * longTermFrameIndex + addTop;
            dpb->frameStoreRef[i]->topField->usedLongTerm = 1;
            dpb->frameStoreRef[i]->usedLongTerm |= 1;
            if (dpb->frameStoreRef[i]->usedLongTerm == 3) {
              dpb->frameStoreRef[i]->frame->usedLongTerm = 1;
              dpb->frameStoreRef[i]->frame->longTermFrameIndex = dpb->frameStoreRef[i]->frame->longTermPicNum = longTermFrameIndex;
              }
            return;
            }
          }

        if (dpb->frameStoreRef[i]->usedReference & 2) {
          if ((!dpb->frameStoreRef[i]->botField->usedLongTerm) &&
              (dpb->frameStoreRef[i]->botField->picNum == picNumX)) {
            if ((dpb->frameStoreRef[i]->usedLongTerm) &&
                (dpb->frameStoreRef[i]->longTermFrameIndex != longTermFrameIndex))
              printf ("Warning: assigning longTermFrameIndex different from other field\n");

            dpb->frameStoreRef[i]->longTermFrameIndex = dpb->frameStoreRef[i]->botField->longTermFrameIndex
                                                = longTermFrameIndex;
            dpb->frameStoreRef[i]->botField->longTermPicNum = 2 * longTermFrameIndex + addBot;
            dpb->frameStoreRef[i]->botField->usedLongTerm = 1;
            dpb->frameStoreRef[i]->usedLongTerm |= 2;
            if (dpb->frameStoreRef[i]->usedLongTerm == 3) {
              dpb->frameStoreRef[i]->frame->usedLongTerm = 1;
              dpb->frameStoreRef[i]->frame->longTermFrameIndex = dpb->frameStoreRef[i]->frame->longTermPicNum = longTermFrameIndex;
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
  void markCurPicLongTerm (sDpb* dpb, sPicture* p, int longTermFrameIndex) {

    // remove long term pictures with same longTermFrameIndex
    if (p->picStructure == eFrame)
      unmarkLongTermFrameForRefByFrameIndex (dpb, longTermFrameIndex);
    else
      unmarkLongTermFieldRefFrameIndex (dpb, p->picStructure, longTermFrameIndex, 1, p->picNum, 0);

    p->usedLongTerm = 1;
    p->longTermFrameIndex = longTermFrameIndex;
    }
  //}}}
  //{{{
  void assignLongTermFrameIndex (sDpb* dpb, sPicture* p, int diffPicNumMinus1, int longTermFrameIndex) {

    int picNumX = getPicNumX(p, diffPicNumMinus1);

    // remove frames/fields with same longTermFrameIndex
    if (p->picStructure == eFrame)
      unmarkLongTermFrameForRefByFrameIndex (dpb, longTermFrameIndex);

    else {
      ePicStructure picStructure = eFrame;
      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
        if (dpb->frameStoreRef[i]->usedReference & 1) {
          if (dpb->frameStoreRef[i]->topField->picNum == picNumX) {
            picStructure = eTopField;
            break;
            }
          }

        if (dpb->frameStoreRef[i]->usedReference & 2) {
          if (dpb->frameStoreRef[i]->botField->picNum == picNumX) {
            picStructure = eBotField;
            break;
            }
          }
        }

      if (picStructure == eFrame)
        cDecoder264::error ("field for long term marking not found");

      unmarkLongTermFieldRefFrameIndex (dpb, picStructure, longTermFrameIndex, 0, 0, picNumX);
      }

    markPicLongTerm (dpb, p, longTermFrameIndex, picNumX);
    }
  //}}}

  //{{{
  void adaptiveMemoryManagement (sDpb* dpb, sPicture* p) {

    sDecodedRefPicMark* tmp_drpm;
    cDecoder264* decoder = dpb->decoder;

    decoder->lastHasMmco5 = 0;

    while (p->decRefPicMarkBuffer) {
      tmp_drpm = p->decRefPicMarkBuffer;
      switch (tmp_drpm->memManagement) {
        //{{{
        case 0:
          if (tmp_drpm->next != NULL)
            cDecoder264::error ("memManagement = 0 not last operation in buffer");
          break;
        //}}}
        //{{{
        case 1:
          unmarkShortTermForRef (dpb, p, tmp_drpm->diffPicNumMinus1);
          dpb->updateRefList();
          break;
        //}}}
        //{{{
        case 2:
          unmarkLongTermForRef (dpb, p, tmp_drpm->longTermPicNum);
          dpb->updateLongTermRefList ();
          break;
        //}}}
        //{{{
        case 3:
          assignLongTermFrameIndex (dpb, p, tmp_drpm->diffPicNumMinus1, tmp_drpm->longTermFrameIndex);
          dpb->updateRefList ();
          dpb->updateLongTermRefList();
          break;
        //}}}
        //{{{
        case 4:
          updateMaxLongTermFrameIndex (dpb, tmp_drpm->maxLongTermFrameIndexPlus1);
          dpb->updateLongTermRefList();
          break;
        //}}}
        //{{{
        case 5:
          unmarkAllShortTermForRef (dpb);
          unmarkAllLongTermForRef (dpb);
          decoder->lastHasMmco5 = 1;
          break;
        //}}}
        //{{{
        case 6:
          markCurPicLongTerm (dpb, p, tmp_drpm->longTermFrameIndex);
          checkNumDpbFrames (dpb);
          break;
        //}}}
        //{{{
        default:
          cDecoder264::error ("invalid memManagement in buffer");
        //}}}
        }
      p->decRefPicMarkBuffer = tmp_drpm->next;
      free (tmp_drpm);
      }

    if (decoder->lastHasMmco5 ) {
      p->picNum = p->frameNum = 0;
      switch (p->picStructure) {
        //{{{
        case eTopField:
          p->poc = p->topPoc = 0;
          break;
        //}}}
        //{{{
        case eBotField:
          p->poc = p->botPoc = 0;
          break;
        //}}}
        //{{{
        case eFrame:
          p->topPoc    -= p->poc;
          p->botPoc -= p->poc;
          p->poc = imin (p->topPoc, p->botPoc);
          break;
        //}}}
        }

      decoder->dpb->flushDpb();
      }
    }
  //}}}
  //{{{
  void slidingWindowMemoryManagement (sDpb* dpb, sPicture* p) {

    // if this is a reference pic with sliding window, unmark first ref frame
    if (dpb->refFramesInBuffer == imax (1, dpb->numRefFrames) - dpb->longTermRefFramesInBuffer) {
      for (uint32_t i = 0; i < dpb->usedSize; i++) {
        if (dpb->frameStore[i]->usedReference && (!(dpb->frameStore[i]->usedLongTerm))) {
          dpb->frameStore[i]->unmarkForRef();
          dpb->updateRefList();
          break;
          }
        }
      }

    p->usedLongTerm = 0;
    }
  //}}}
  //{{{
  void idrMemoryManagement (sDpb* dpb, sPicture* p) {

    if (p->noOutputPriorPicFlag) {
      // free all stored pictures
      for (uint32_t i = 0; i < dpb->usedSize; i++) {
        // reset all reference settings
        delete dpb->frameStore[i];
        dpb->frameStore[i] = new cFrameStore();
        }
      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
        dpb->frameStoreRef[i]=NULL;
      for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
        dpb->frameStoreLongTermRef[i]=NULL;
      dpb->usedSize = 0;
      }
    else
      dpb->flushDpb();

    dpb->lastPictureFrameStore = NULL;

    dpb->updateRefList();
    dpb->updateLongTermRefList();
    dpb->lastOutputPoc = INT_MIN;

    if (p->longTermRefFlag) {
      dpb->maxLongTermPicIndex = 0;
      p->usedLongTerm = 1;
      p->longTermFrameIndex = 0;
      }
    else {
      dpb->maxLongTermPicIndex = -1;
      p->usedLongTerm = 0;
      }
    }
  //}}}

  //{{{
  void genPicListFromFrameList (ePicStructure currStructure, cFrameStore** frameStoreList,
                                int list_idx, sPicture** list, char *list_size, int long_term) {

    int top_idx = 0;
    int bot_idx = 0;

    int (*is_ref)(sPicture*s) = (long_term) ? isLongRef : isShortRef;

    if (currStructure == eTopField) {
      while ((top_idx<list_idx)||(bot_idx<list_idx)) {
        for ( ; top_idx<list_idx; top_idx++) {
          if (frameStoreList[top_idx]->isUsed & 1) {
            if (is_ref (frameStoreList[top_idx]->topField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = frameStoreList[top_idx]->topField;
              (*list_size)++;
              top_idx++;
              break;
              }
            }
          }

        for ( ; bot_idx<list_idx; bot_idx++) {
          if (frameStoreList[bot_idx]->isUsed & 2) {
            if (is_ref (frameStoreList[bot_idx]->botField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = frameStoreList[bot_idx]->botField;
              (*list_size)++;
              bot_idx++;
              break;
              }
            }
          }
        }
      }

    if (currStructure == eBotField) {
      while ((top_idx<list_idx)||(bot_idx<list_idx)) {
        for ( ; bot_idx<list_idx; bot_idx++) {
          if (frameStoreList[bot_idx]->isUsed & 2) {
            if (is_ref (frameStoreList[bot_idx]->botField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = frameStoreList[bot_idx]->botField;
              (*list_size)++;
              bot_idx++;
              break;
              }
            }
          }

        for ( ; top_idx<list_idx; top_idx++) {
          if (frameStoreList[top_idx]->isUsed & 1) {
            if (is_ref (frameStoreList[top_idx]->topField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = frameStoreList[top_idx]->topField;
              (*list_size)++;
              top_idx++;
              break;
              }
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void reorderShortTerm (cSlice* slice, int curList, int numRefIndexIXactiveMinus1, int picNumLX, int *refIdxLX) {

    sPicture** refPicListX = slice->listX[curList];
    sPicture* picLX = getShortTermPic (slice, slice->dpb, picNumLX);

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

    sPicture* picLX = getLongTermPic (slice, slice->dpb, LongTermPicNum);

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

  //{{{
  void allocPicMotion (sPicMotionOld* motion, int sizeY, int sizeX) {
    motion->mbField = (uint8_t*)calloc (sizeY * sizeX, sizeof(uint8_t));
    }
  //}}}
  //{{{
  void freePicMotion (sPicMotionOld* motion) {

    free (motion->mbField);
    motion->mbField = NULL;
    }
  //}}}
  }

// picture
//{{{
sPicture* allocPicture (cDecoder264* decoder, ePicStructure picStructure,
                        int sizeX, int sizeY, int sizeXcr, int sizeYcr, int isOutput) {

  cSps* activeSps = decoder->activeSps;
  sPicture* s = (sPicture*)calloc (1, sizeof(sPicture));

  if (picStructure != eFrame) {
    sizeY /= 2;
    sizeYcr /= 2;
    }

  s->picSizeInMbs = (sizeX*sizeY)/256;
  s->imgUV = NULL;

  getMem2DpelPad (&(s->imgY), sizeY, sizeX, decoder->coding.lumaPadY, decoder->coding.lumaPadX);
  s->lumaStride = sizeX + 2 * decoder->coding.lumaPadX;
  s->lumaExpandedHeight = sizeY + 2 * decoder->coding.lumaPadY;

  if (activeSps->chromaFormatIdc != YUV400)
    getMem3DpelPad (&(s->imgUV), 2, sizeYcr, sizeXcr, decoder->coding.chromaPadY, decoder->coding.chromaPadX);

  s->chromaStride = sizeXcr + 2*decoder->coding.chromaPadX;
  s->chromaExpandedHeight = sizeYcr + 2*decoder->coding.chromaPadY;
  s->lumaPadY = decoder->coding.lumaPadY;
  s->lumaPadX = decoder->coding.lumaPadX;
  s->chromaPadY = decoder->coding.chromaPadY;
  s->chromaPadX = decoder->coding.chromaPadX;
  s->isSeperateColourPlane = decoder->coding.isSeperateColourPlane;

  getMem2Dmp (&s->mvInfo, (sizeY >> BLOCK_SHIFT), (sizeX >> BLOCK_SHIFT));
  allocPicMotion (&s->motion , (sizeY >> BLOCK_SHIFT), (sizeX >> BLOCK_SHIFT));

  if (decoder->coding.isSeperateColourPlane != 0)
    for (int nplane = 0; nplane < MAX_PLANE; nplane++) {
      getMem2Dmp (&s->mvInfoJV[nplane], (sizeY >> BLOCK_SHIFT), (sizeX >> BLOCK_SHIFT));
      allocPicMotion (&s->motionJV[nplane] , (sizeY >> BLOCK_SHIFT), (sizeX >> BLOCK_SHIFT));
      }

  s->picNum = 0;
  s->frameNum = 0;
  s->longTermFrameIndex = 0;
  s->longTermPicNum = 0;
  s->usedForReference  = 0;
  s->usedLongTerm = 0;
  s->nonExisting = 0;
  s->isOutput = 0;
  s->maxSliceId = 0;
  s->picStructure = picStructure;

  s->sizeX = sizeX;
  s->sizeY = sizeY;
  s->sizeXcr = sizeXcr;
  s->sizeYcr = sizeYcr;
  s->size_x_m1 = sizeX - 1;
  s->size_y_m1 = sizeY - 1;
  s->size_x_cr_m1 = sizeXcr - 1;
  s->size_y_cr_m1 = sizeYcr - 1;

  s->topField = decoder->noReferencePicture;
  s->botField = decoder->noReferencePicture;
  s->frame = decoder->noReferencePicture;

  s->decRefPicMarkBuffer = NULL;
  s->codedFrame  = 0;
  s->mbAffFrame  = 0;
  s->topPoc = s->botPoc = s->poc = 0;

  if (!decoder->activeSps->frameMbOnly && picStructure != eFrame)
    for (int j = 0; j < MAX_NUM_SLICES; j++)
      for (int i = 0; i < 2; i++)
        s->listX[j][i] = (sPicture**)calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering

  return s;
  }
//}}}
//{{{
void freePicture (sPicture* picture) {

  if (picture) {
    if (picture->mvInfo) {
      freeMem2Dmp (picture->mvInfo);
      picture->mvInfo = NULL;
      }
    freePicMotion (&picture->motion);

    if ((picture->isSeperateColourPlane != 0) ) {
      for (int nplane = 0; nplane < MAX_PLANE; nplane++ ) {
        if (picture->mvInfoJV[nplane]) {
          freeMem2Dmp (picture->mvInfoJV[nplane]);
          picture->mvInfoJV[nplane] = NULL;
          }
        freePicMotion (&picture->motionJV[nplane]);
        }
      }

    if (picture->imgY) {
      freeMem2DpelPad (picture->imgY, picture->lumaPadY, picture->lumaPadX);
      picture->imgY = NULL;
      }

    if (picture->imgUV) {
      freeMem3DpelPad (picture->imgUV, 2, picture->chromaPadY, picture->chromaPadX);
      picture->imgUV = NULL;
      }

    for (int j = 0; j < MAX_NUM_SLICES; j++)
      for (int i = 0; i < 2; i++)
        if (picture->listX[j][i]) {
          free (picture->listX[j][i]);
          picture->listX[j][i] = NULL;
          }

    free (picture);
    picture = NULL;
    }
  }
//}}}
//{{{
void fillFrameNumGap (cDecoder264* decoder, cSlice* slice) {

  cSps* activeSps = decoder->activeSps;

  int tmp1 = slice->deltaPicOrderCount[0];
  int tmp2 = slice->deltaPicOrderCount[1];
  slice->deltaPicOrderCount[0] = slice->deltaPicOrderCount[1] = 0;

  printf ("A gap in frame number is found, try to fill it.\n");

  int unusedShortTermFrameNum = (decoder->preFrameNum + 1) % decoder->coding.maxFrameNum;
  int curFrameNum = slice->frameNum;

  sPicture* picture = NULL;
  while (curFrameNum != unusedShortTermFrameNum) {
    picture = allocPicture (decoder, eFrame, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);
    picture->codedFrame = 1;
    picture->picNum = unusedShortTermFrameNum;
    picture->frameNum = unusedShortTermFrameNum;
    picture->nonExisting = 1;
    picture->isOutput = 1;
    picture->usedForReference = 1;
    picture->adaptRefPicBufFlag = 0;

    slice->frameNum = unusedShortTermFrameNum;
    if (activeSps->pocType != 0)
      decoder->decodePOC (decoder->sliceList[0]);
    picture->topPoc = slice->topPoc;
    picture->botPoc = slice->botPoc;
    picture->framePoc = slice->framePoc;
    picture->poc = slice->framePoc;
    slice->dpb->storePictureDpb (picture);

    decoder->preFrameNum = unusedShortTermFrameNum;
    unusedShortTermFrameNum = (unusedShortTermFrameNum + 1) % decoder->coding.maxFrameNum;
    }

  slice->deltaPicOrderCount[0] = tmp1;
  slice->deltaPicOrderCount[1] = tmp2;
  slice->frameNum = curFrameNum;
  }
//}}}
