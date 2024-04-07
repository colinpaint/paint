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
  int outputDpbFrame (sDpb* dpb) {

    // diagnostics
    if (dpb->usedSize < 1)
      cDecoder264::error ("Cannot output frame, DPB empty");

    // find smallest POC
    int poc;
    int pos;
    dpb->getSmallestPoc (&poc, &pos);
    if (pos == -1)
      return 0;

    // picture error conceal
    cDecoder264* decoder = dpb->decoder;
    if (decoder->concealMode != 0) {
      if (dpb->lastOutputPoc == 0)
        write_lost_ref_after_idr (dpb, pos);
      write_lost_non_ref_pic (dpb, poc);
      }

    decoder->writeStoredFrame (dpb->frameStore[pos]);

    // picture error conceal
    if(decoder->concealMode == 0)
      if (dpb->lastOutputPoc >= poc)
        cDecoder264::error ("output POC must be in ascending order");

    dpb->lastOutputPoc = poc;

    // free frame store and move empty store to end of buffer
    if (!dpb->frameStore[pos]->isReference())
      dpb->removeFrameDpb (pos);

    return 1;
    }
  //}}}
  //{{{
  void checkNumDpbFrames (sDpb* dpb) {

    if ((int)(dpb->longTermRefFramesInBuffer + dpb->refFramesInBuffer) > imax (1, dpb->numRefFrames))
      cDecoder264::error ("Max. number of reference frames exceeded. Invalid stream");
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
          dpb->updateLongTermRefList();
          break;
        //}}}
        //{{{
        case 3:
          assignLongTermFrameIndex (dpb, p, tmp_drpm->diffPicNumMinus1, tmp_drpm->longTermFrameIndex);
          dpb->updateRefList();
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

// dpb
//{{{
void sDpb::getSmallestPoc (int* poc, int* pos) {

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
void sDpb::updateRefList() {

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
void sDpb::updateLongTermRefList() {

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
void sDpb::initDpb (cDecoder264* decoder, int type) {

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
void sDpb::reInitDpb (cDecoder264* decoder, int type) {

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
void sDpb::flushDpb() {

  if (!initDone)
    return;

  if (decoder->concealMode != 0)
    conceal_non_ref_pics (this, 0);

  // mark all frames unused
  for (uint32_t i = 0; i < usedSize; i++)
    frameStore[i]->unmarkForRef();

  while (removeUnusedDpb());

  // output frames in POC order
  while (usedSize && outputDpbFrame (this));

  lastOutputPoc = INT_MIN;
  }
//}}}
//{{{
int sDpb::removeUnusedDpb() {

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
void sDpb::storePictureDpb (sPicture* picture) {

  int poc, pos;
  // picture error conceal

  // if frame, check for new store,
  decoder->lastHasMmco5 = 0;
  decoder->lastPicBotField = (picture->picStructure == eBotField);

  if (picture->isIDR) {
    idrMemoryManagement (this, picture);
    // picture error conceal
    memset (decoder->dpbPoc, 0, sizeof(int)*100);
    }
  else {
    // adaptive memory management
    if (picture->usedForReference && (picture->adaptRefPicBufFlag))
      adaptiveMemoryManagement (this, picture);
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
    slidingWindowMemoryManagement (this, picture);

  // picture error conceal
  if (decoder->concealMode != 0)
    for (uint32_t i = 0; i < size; i++)
      if (frameStore[i]->usedReference)
        frameStore[i]->concealRef = 1;

  // first try to remove unused frames
  if (usedSize == size) {
    // picture error conceal
    if (decoder->concealMode != 0)
      conceal_non_ref_pics (this, 2);

    removeUnusedDpb();

    if (decoder->concealMode != 0)
      sliding_window_poc_management (this, picture);
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
    outputDpbFrame (this);
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
  checkNumDpbFrames (this);
  dumpDpb();
  }
//}}}
//{{{
void sDpb::removeFrameDpb (int pos) {

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
void sDpb::freeDpb () {

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

// private
//{{{
void sDpb::dumpDpb() {

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

// slice
//{{{
void reorderRefPicList (cSlice* slice, int curList) {

  cDecoder264* decoder = slice->decoder;

  int* modPicNumsIdc = slice->modPicNumsIdc[curList];
  int* absDiffPicNumMinus1 = slice->absDiffPicNumMinus1[curList];
  int* longTermPicIndex = slice->longTermPicIndex[curList];
  int numRefIndexIXactiveMinus1 = slice->numRefIndexActive[curList] - 1;

  int maxPicNum;
  int currPicNum;
  if (slice->picStructure == eFrame) {
    maxPicNum = decoder->coding.maxFrameNum;
    currPicNum = slice->frameNum;
    }
  else {
    maxPicNum = 2 * decoder->coding.maxFrameNum;
    currPicNum = 2 * slice->frameNum + 1;
    }

  int picNumLX;
  int picNumLXNoWrap;
  int picNumLXPred = currPicNum;
  int refIdxLX = 0;
  for (int i = 0; modPicNumsIdc[i] != 3; i++) {
    if (modPicNumsIdc[i] > 3)
      cDecoder264::error ("Invalid modPicNumsIdc command");
    if (modPicNumsIdc[i] < 2) {
      if (modPicNumsIdc[i] == 0) {
        if (picNumLXPred - (absDiffPicNumMinus1[i]+1) < 0)
          picNumLXNoWrap = picNumLXPred - (absDiffPicNumMinus1[i]+1) + maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred - (absDiffPicNumMinus1[i]+1);
        }
      else {
        if (picNumLXPred + absDiffPicNumMinus1[i]+1 >= maxPicNum)
          picNumLXNoWrap = picNumLXPred + (absDiffPicNumMinus1[i]+1) - maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred + (absDiffPicNumMinus1[i]+1);
        }

      picNumLXPred = picNumLXNoWrap;
      if (picNumLXNoWrap > currPicNum)
        picNumLX = picNumLXNoWrap - maxPicNum;
      else
        picNumLX = picNumLXNoWrap;

      reorderShortTerm (slice, curList, numRefIndexIXactiveMinus1, picNumLX, &refIdxLX);
      }
    else
      reorderLongTerm (slice, slice->listX[curList], numRefIndexIXactiveMinus1, longTermPicIndex[i], &refIdxLX);
    }

  slice->listXsize[curList] = (char)(numRefIndexIXactiveMinus1 + 1);
  }
//}}}

//{{{
void updatePicNum (cSlice* slice) {

  cDecoder264* decoder = slice->decoder;

  int addTop = 0;
  int addBot = 0;
  int maxFrameNum = 1 << (decoder->activeSps->log2maxFrameNumMinus4 + 4);

  sDpb* dpb = slice->dpb;
  if (slice->picStructure == eFrame) {
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->frameStoreRef[i]->isUsed == 3 ) {
        if (dpb->frameStoreRef[i]->frame->usedForReference &&
            !dpb->frameStoreRef[i]->frame->usedLongTerm) {
          if (dpb->frameStoreRef[i]->frameNum > slice->frameNum )
            dpb->frameStoreRef[i]->frameNumWrap = dpb->frameStoreRef[i]->frameNum - maxFrameNum;
          else
            dpb->frameStoreRef[i]->frameNumWrap = dpb->frameStoreRef[i]->frameNum;
          dpb->frameStoreRef[i]->frame->picNum = dpb->frameStoreRef[i]->frameNumWrap;
          }
        }
      }

    // update longTermPicNum
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->frameStoreLongTermRef[i]->isUsed == 3) {
        if (dpb->frameStoreLongTermRef[i]->frame->usedLongTerm)
          dpb->frameStoreLongTermRef[i]->frame->longTermPicNum = dpb->frameStoreLongTermRef[i]->frame->longTermFrameIndex;
        }
      }
    }
  else {
    if (slice->picStructure == eTopField) {
      addTop = 1;
      addBot = 0;
      }
    else {
      addTop = 0;
      addBot = 1;
      }

    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->frameStoreRef[i]->usedReference) {
        if (dpb->frameStoreRef[i]->frameNum > slice->frameNum )
          dpb->frameStoreRef[i]->frameNumWrap = dpb->frameStoreRef[i]->frameNum - maxFrameNum;
        else
          dpb->frameStoreRef[i]->frameNumWrap = dpb->frameStoreRef[i]->frameNum;
        if (dpb->frameStoreRef[i]->usedReference & 1)
          dpb->frameStoreRef[i]->topField->picNum = (2 * dpb->frameStoreRef[i]->frameNumWrap) + addTop;
        if (dpb->frameStoreRef[i]->usedReference & 2)
          dpb->frameStoreRef[i]->botField->picNum = (2 * dpb->frameStoreRef[i]->frameNumWrap) + addBot;
        }
      }

    // update longTermPicNum
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->frameStoreLongTermRef[i]->usedLongTerm & 1)
        dpb->frameStoreLongTermRef[i]->topField->longTermPicNum = 2 * dpb->frameStoreLongTermRef[i]->topField->longTermFrameIndex + addTop;
      if (dpb->frameStoreLongTermRef[i]->usedLongTerm & 2)
        dpb->frameStoreLongTermRef[i]->botField->longTermPicNum = 2 * dpb->frameStoreLongTermRef[i]->botField->longTermFrameIndex + addBot;
      }
    }
  }
//}}}
//{{{
sPicture* getShortTermPic (cSlice* slice, sDpb* dpb, int picNum) {

  for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
    if (slice->picStructure == eFrame) {
      if (dpb->frameStoreRef[i]->usedReference == 3)
        if (!dpb->frameStoreRef[i]->frame->usedLongTerm &&
            (dpb->frameStoreRef[i]->frame->picNum == picNum))
          return dpb->frameStoreRef[i]->frame;
      }
    else {
      if (dpb->frameStoreRef[i]->usedReference & 1)
        if (!dpb->frameStoreRef[i]->topField->usedLongTerm &&
            (dpb->frameStoreRef[i]->topField->picNum == picNum))
          return dpb->frameStoreRef[i]->topField;

      if (dpb->frameStoreRef[i]->usedReference & 2)
        if (!dpb->frameStoreRef[i]->botField->usedLongTerm &&
            (dpb->frameStoreRef[i]->botField->picNum == picNum))
          return dpb->frameStoreRef[i]->botField;
      }
    }

  return slice->decoder->noReferencePicture;
  }
//}}}

//{{{
void initListsSliceI (cSlice* slice) {

  slice->listXsize[0] = 0;
  slice->listXsize[1] = 0;
  }
//}}}
//{{{
void initListsSliceP (cSlice* slice) {

  cDecoder264* decoder = slice->decoder;
  sDpb* dpb = slice->dpb;

  int list0idx = 0;
  int listLtIndex = 0;
  cFrameStore** frameStoreList0;
  cFrameStore** frameStoreListLongTerm;

  if (slice->picStructure == eFrame) {
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->frameStoreRef[i]->isUsed == 3)
        if ((dpb->frameStoreRef[i]->frame->usedForReference) &&
            !dpb->frameStoreRef[i]->frame->usedLongTerm)
          slice->listX[0][list0idx++] = dpb->frameStoreRef[i]->frame;

    // order list 0 by picNum
    qsort ((void *)slice->listX[0], list0idx, sizeof(sPicture*), comparePicByPicNumDescending);
    slice->listXsize[0] = (char) list0idx;

    // long term
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      if (dpb->frameStoreLongTermRef[i]->isUsed == 3)
        if (dpb->frameStoreLongTermRef[i]->frame->usedLongTerm)
          slice->listX[0][list0idx++] = dpb->frameStoreLongTermRef[i]->frame;
    qsort ((void*)&slice->listX[0][(int16_t)slice->listXsize[0]], list0idx - slice->listXsize[0],
           sizeof(sPicture*), comparePicByLtPicNumAscending);
    slice->listXsize[0] = (char) list0idx;
    }
  else {
    frameStoreList0 = (cFrameStore**)calloc (dpb->size, sizeof(cFrameStore*));
    frameStoreListLongTerm = (cFrameStore**)calloc (dpb->size, sizeof(cFrameStore*));
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->frameStoreRef[i]->usedReference)
        frameStoreList0[list0idx++] = dpb->frameStoreRef[i];
    qsort ((void*)frameStoreList0, list0idx, sizeof(cFrameStore*), compareFsByFrameNumDescending);
    slice->listXsize[0] = 0;
    genPicListFromFrameList (slice->picStructure, frameStoreList0, list0idx, slice->listX[0], &slice->listXsize[0], 0);

    // long term
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      frameStoreListLongTerm[listLtIndex++] = dpb->frameStoreLongTermRef[i];
    qsort ((void*)frameStoreListLongTerm, listLtIndex, sizeof(cFrameStore*), compareFsbyLtPicIndexAscending);
    genPicListFromFrameList (slice->picStructure, frameStoreListLongTerm, listLtIndex, slice->listX[0], &slice->listXsize[0], 1);

    free (frameStoreList0);
    free (frameStoreListLongTerm);
    }
  slice->listXsize[1] = 0;

  // set max size
  slice->listXsize[0] = (char)imin (slice->listXsize[0], slice->numRefIndexActive[LIST_0]);
  slice->listXsize[1] = (char)imin (slice->listXsize[1], slice->numRefIndexActive[LIST_1]);

  // set the unused list entries to NULL
  for (uint32_t i = slice->listXsize[0]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[0][i] = decoder->noReferencePicture;
  for (uint32_t i = slice->listXsize[1]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[1][i] = decoder->noReferencePicture;
  }
//}}}
//{{{
void initListsSliceB (cSlice* slice) {

  cDecoder264* decoder = slice->decoder;
  sDpb* dpb = slice->dpb;

  int list0idx = 0;
  int list0index1 = 0;
  int listLtIndex = 0;
  cFrameStore** frameStoreList0;
  cFrameStore** frameStoreList1;
  cFrameStore** frameStoreListLongTerm;

  // B Slice
  if (slice->picStructure == eFrame) {
    //{{{  frame
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->frameStoreRef[i]->isUsed==3)
        if ((dpb->frameStoreRef[i]->frame->usedForReference) && (!dpb->frameStoreRef[i]->frame->usedLongTerm))
          if (slice->framePoc >= dpb->frameStoreRef[i]->frame->poc) // !KS use >= for error conceal
            slice->listX[0][list0idx++] = dpb->frameStoreRef[i]->frame;
    qsort ((void*)slice->listX[0], list0idx, sizeof(sPicture*), comparePicByPocdesc);

    // get the backward reference picture (POC>current POC) in list0;
    list0index1 = list0idx;
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->frameStoreRef[i]->isUsed==3)
        if ((dpb->frameStoreRef[i]->frame->usedForReference)&&(!dpb->frameStoreRef[i]->frame->usedLongTerm))
          if (slice->framePoc < dpb->frameStoreRef[i]->frame->poc)
            slice->listX[0][list0idx++] = dpb->frameStoreRef[i]->frame;
    qsort ((void*)&slice->listX[0][list0index1], list0idx-list0index1, sizeof(sPicture*), comparePicByPocAscending);

    for (int j = 0; j < list0index1; j++)
      slice->listX[1][list0idx-list0index1+j]=slice->listX[0][j];
    for (int j = list0index1; j < list0idx; j++)
      slice->listX[1][j-list0index1] = slice->listX[0][j];
    slice->listXsize[0] = slice->listXsize[1] = (char) list0idx;

    // long term
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->frameStoreLongTermRef[i]->isUsed == 3) {
        if (dpb->frameStoreLongTermRef[i]->frame->usedLongTerm) {
          slice->listX[0][list0idx] = dpb->frameStoreLongTermRef[i]->frame;
          slice->listX[1][list0idx++] = dpb->frameStoreLongTermRef[i]->frame;
          }
        }
      }
    qsort ((void *)&slice->listX[0][(int16_t) slice->listXsize[0]], list0idx - slice->listXsize[0],
           sizeof(sPicture*), comparePicByLtPicNumAscending);
    qsort ((void *)&slice->listX[1][(int16_t) slice->listXsize[0]], list0idx - slice->listXsize[0],
           sizeof(sPicture*), comparePicByLtPicNumAscending);

    slice->listXsize[0] = slice->listXsize[1] = (char)list0idx;
    }
    //}}}
  else {
    //{{{  field
    frameStoreList0 = (cFrameStore**)calloc(dpb->size, sizeof (cFrameStore*));
    frameStoreList1 = (cFrameStore**)calloc(dpb->size, sizeof (cFrameStore*));
    frameStoreListLongTerm = (cFrameStore**)calloc(dpb->size, sizeof (cFrameStore*));
    slice->listXsize[0] = 0;
    slice->listXsize[1] = 1;

    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->frameStoreRef[i]->isUsed)
        if (slice->thisPoc >= dpb->frameStoreRef[i]->poc)
          frameStoreList0[list0idx++] = dpb->frameStoreRef[i];
    qsort ((void*)frameStoreList0, list0idx, sizeof(cFrameStore*), comparefsByPocdesc);

    list0index1 = list0idx;
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->frameStoreRef[i]->isUsed)
        if (slice->thisPoc < dpb->frameStoreRef[i]->poc)
          frameStoreList0[list0idx++] = dpb->frameStoreRef[i];
    qsort ((void*)&frameStoreList0[list0index1], list0idx-list0index1, sizeof(cFrameStore*), compareFsByPocAscending);

    for (int j = 0; j < list0index1; j++)
      frameStoreList1[list0idx-list0index1+j]=frameStoreList0[j];
    for (int j = list0index1; j < list0idx; j++)
      frameStoreList1[j-list0index1]=frameStoreList0[j];

    slice->listXsize[0] = 0;
    slice->listXsize[1] = 0;
    genPicListFromFrameList (slice->picStructure, frameStoreList0, list0idx, slice->listX[0], &slice->listXsize[0], 0);
    genPicListFromFrameList (slice->picStructure, frameStoreList1, list0idx, slice->listX[1], &slice->listXsize[1], 0);

    // long term
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      frameStoreListLongTerm[listLtIndex++] = dpb->frameStoreLongTermRef[i];

    qsort ((void*)frameStoreListLongTerm, listLtIndex, sizeof(cFrameStore*), compareFsbyLtPicIndexAscending);
    genPicListFromFrameList (slice->picStructure, frameStoreListLongTerm, listLtIndex, slice->listX[0], &slice->listXsize[0], 1);
    genPicListFromFrameList (slice->picStructure, frameStoreListLongTerm, listLtIndex, slice->listX[1], &slice->listXsize[1], 1);

    free (frameStoreList0);
    free (frameStoreList1);
    free (frameStoreListLongTerm);
    }
    //}}}

  if ((slice->listXsize[0] == slice->listXsize[1]) && (slice->listXsize[0] > 1)) {
    // check if lists are identical, if yes swap first two elements of slice->listX[1]
    bool diff = false;
    for (int j = 0; j< slice->listXsize[0]; j++) {
      if (slice->listX[0][j] != slice->listX[1][j]) {
        diff = true;
        break;
        }
      }
    if (!diff) {
      sPicture* tmp_s = slice->listX[1][0];
      slice->listX[1][0] = slice->listX[1][1];
      slice->listX[1][1] = tmp_s;
      }
    }

  // set max size
  slice->listXsize[0] = (char)imin (slice->listXsize[0], slice->numRefIndexActive[LIST_0]);
  slice->listXsize[1] = (char)imin (slice->listXsize[1], slice->numRefIndexActive[LIST_1]);

  // set the unused list entries to NULL
  for (uint32_t i = slice->listXsize[0]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[0][i] = decoder->noReferencePicture;
  for (uint32_t i = slice->listXsize[1]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[1][i] = decoder->noReferencePicture;
  }
//}}}

//{{{
void computeColocated (cSlice* slice, sPicture** listX[6]) {

  cDecoder264* decoder = slice->decoder;
  if (slice->directSpatialMvPredFlag == 0) {
    for (int j = 0; j < 2 + (slice->mbAffFrame * 4); j += 2) {
      for (int i = 0; i < slice->listXsize[j];i++) {
        int iTRb;
        if (j == 0)
          iTRb = iClip3 (-128, 127, decoder->picture->poc - listX[LIST_0 + j][i]->poc);
        else if (j == 2)
          iTRb = iClip3 (-128, 127, decoder->picture->topPoc - listX[LIST_0 + j][i]->poc);
        else
          iTRb = iClip3 (-128, 127, decoder->picture->botPoc - listX[LIST_0 + j][i]->poc);

        int iTRp = iClip3 (-128, 127, listX[LIST_1 + j][0]->poc - listX[LIST_0 + j][i]->poc);
        if (iTRp != 0) {
          int prescale = (16384 + iabs( iTRp / 2)) / iTRp;
          slice->mvscale[j][i] = iClip3 (-1024, 1023, (iTRb * prescale + 32) >> 6) ;
          }
        else
          slice->mvscale[j][i] = 9999;
        }
     }
    }
  }
//}}}
