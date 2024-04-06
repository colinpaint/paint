//{{{  includes
#include <limits.h>

#include "global.h"
#include "memory.h"

#include "erc.h"
#include "buffer.h"
#include "output.h"
//}}}
#define MAX_LIST_SIZE 33
//#define DUMP_DPB
namespace {
  //{{{
  void unmarkLongTermFieldRefFrameIndex (sDpb* dpb, ePicStructure picStructure, int longTermFrameIndex,
                                                int mark_current, uint32_t curr_frame_num, int curr_pic_num) {

    cDecoder264* decoder = dpb->decoder;

    if (curr_pic_num < 0)
      curr_pic_num += (2 * decoder->coding.maxFrameNum);

    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->fsLongTermRef[i]->longTermFrameIndex == longTermFrameIndex) {
        if (picStructure == eTopField) {
          if (dpb->fsLongTermRef[i]->isLongTerm == 3)
            dpb->fsLongTermRef[i]->unmarkForLongTermRef();
          else {
            if (dpb->fsLongTermRef[i]->isLongTerm == 1)
              dpb->fsLongTermRef[i]->unmarkForLongTermRef ();
            else {
              if (mark_current) {
                if (dpb->lastPicture) {
                  if ((dpb->lastPicture != dpb->fsLongTermRef[i]) ||
                      dpb->lastPicture->frameNum != curr_frame_num)
                    dpb->fsLongTermRef[i]->unmarkForLongTermRef();
                  }
                else
                  dpb->fsLongTermRef[i]->unmarkForLongTermRef();
              }
              else {
                if ((dpb->fsLongTermRef[i]->frameNum) != (uint32_t)(curr_pic_num >> 1))
                  dpb->fsLongTermRef[i]->unmarkForLongTermRef();
                }
              }
            }
          }

        if (picStructure == eBotField) {
          if (dpb->fsLongTermRef[i]->isLongTerm == 3)
            dpb->fsLongTermRef[i]->unmarkForLongTermRef ();
          else {
            if (dpb->fsLongTermRef[i]->isLongTerm == 2)
              dpb->fsLongTermRef[i]->unmarkForLongTermRef ();
            else {
              if (mark_current) {
                if (dpb->lastPicture) {
                  if ((dpb->lastPicture != dpb->fsLongTermRef[i]) ||
                      dpb->lastPicture->frameNum != curr_frame_num)
                    dpb->fsLongTermRef[i]->unmarkForLongTermRef ();
                  }
                else
                  dpb->fsLongTermRef[i]->unmarkForLongTermRef ();
                }
              else {
                if ((dpb->fsLongTermRef[i]->frameNum) != (uint32_t)(curr_pic_num >> 1))
                  dpb->fsLongTermRef[i]->unmarkForLongTermRef ();
                }
              }
            }
          }
        }
      }
    }
  //}}}
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
    int poc, pos;
    getSmallestPoc (dpb, &poc, &pos);
    if (pos == -1)
      return 0;

    // call the output function
    //printf ("output frame with frameNum #%d, poc %d (dpb. dpb->size=%d, dpb->usedSize=%d)\n", dpb->frameStore[pos]->frameNum, dpb->frameStore[pos]->frame->poc, dpb->size, dpb->usedSize);

    // picture error conceal
    cDecoder264* decoder = dpb->decoder;
    if (decoder->concealMode != 0) {
      if (dpb->lastOutputPoc == 0)
        write_lost_ref_after_idr (dpb, pos);
      write_lost_non_ref_pic (dpb, poc);
      }

    writeStoredFrame (decoder, dpb->frameStore[pos]);

    // picture error conceal
    if(decoder->concealMode == 0)
      if (dpb->lastOutputPoc >= poc)
        cDecoder264::error ("output POC must be in ascending order");

    dpb->lastOutputPoc = poc;

    // free frame store and move empty store to end of buffer
    if (!dpb->frameStore[pos]->isReference ())
      removeFrameDpb (dpb, pos);

    return 1;
    }
  //}}}

  //{{{
  void dumpDpb (sDpb* dpb) {

  #ifdef DUMP_DPB
    for (uint32_t i = 0; i < dpb->usedSize; i++) {
      printf ("(");
      printf ("fn=%d  ", dpb->frameStore[i]->frameNum);
      if (dpb->frameStore[i]->isUsed & 1) {
        if (dpb->frameStore[i]->topField)
          printf ("T: poc=%d  ", dpb->frameStore[i]->topField->poc);
        else
          printf ("T: poc=%d  ", dpb->frameStore[i]->frame->topPoc);
        }

      if (dpb->frameStore[i]->isUsed & 2) {
        if (dpb->frameStore[i]->botField)
          printf ("B: poc=%d  ", dpb->frameStore[i]->botField->poc);
        else
          printf ("B: poc=%d  ", dpb->frameStore[i]->frame->botPoc);
        }

      if (dpb->frameStore[i]->isUsed == 3)
        printf ("F: poc=%d  ", dpb->frameStore[i]->frame->poc);
      printf ("G: poc=%d)  ", dpb->frameStore[i]->poc);

      if (dpb->frameStore[i]->isReference)
        printf ("ref (%d) ", dpb->frameStore[i]->isReference);
      if (dpb->frameStore[i]->isLongTerm)
        printf ("lt_ref (%d) ", dpb->frameStore[i]->isReference);
      if (dpb->frameStore[i]->isOutput)
        printf ("out  ");
      if (dpb->frameStore[i]->isUsed == 3)
        if (dpb->frameStore[i]->frame->nonExisting)
          printf ("ne  ");

      printf ("\n");
      }
  #endif
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
  void markPicLongTerm (sDpb* dpb, sPicture* p, int longTermFrameIndex, int picNumX) {

    int addTop, addBot;

    if (p->picStructure == eFrame) {
      for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
        if (dpb->fsRef[i]->mIsReference == 3) {
          if ((!dpb->fsRef[i]->frame->isLongTerm)&&(dpb->fsRef[i]->frame->picNum == picNumX)) {
            dpb->fsRef[i]->longTermFrameIndex = dpb->fsRef[i]->frame->longTermFrameIndex
                                               = longTermFrameIndex;
            dpb->fsRef[i]->frame->longTermPicNum = longTermFrameIndex;
            dpb->fsRef[i]->frame->isLongTerm = 1;

            if (dpb->fsRef[i]->topField && dpb->fsRef[i]->botField) {
              dpb->fsRef[i]->topField->longTermFrameIndex = dpb->fsRef[i]->botField->longTermFrameIndex
                                                            = longTermFrameIndex;
              dpb->fsRef[i]->topField->longTermPicNum = longTermFrameIndex;
              dpb->fsRef[i]->botField->longTermPicNum = longTermFrameIndex;

              dpb->fsRef[i]->topField->isLongTerm = dpb->fsRef[i]->botField->isLongTerm = 1;
              }
            dpb->fsRef[i]->isLongTerm = 3;
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
        if (dpb->fsRef[i]->mIsReference & 1) {
          if ((!dpb->fsRef[i]->topField->isLongTerm) &&
              (dpb->fsRef[i]->topField->picNum == picNumX)) {
            if ((dpb->fsRef[i]->isLongTerm) &&
                (dpb->fsRef[i]->longTermFrameIndex != longTermFrameIndex)) {
              printf ("Warning: assigning longTermFrameIndex different from other field\n");
              }

            dpb->fsRef[i]->longTermFrameIndex = dpb->fsRef[i]->topField->longTermFrameIndex = longTermFrameIndex;
            dpb->fsRef[i]->topField->longTermPicNum = 2 * longTermFrameIndex + addTop;
            dpb->fsRef[i]->topField->isLongTerm = 1;
            dpb->fsRef[i]->isLongTerm |= 1;
            if (dpb->fsRef[i]->isLongTerm == 3) {
              dpb->fsRef[i]->frame->isLongTerm = 1;
              dpb->fsRef[i]->frame->longTermFrameIndex = dpb->fsRef[i]->frame->longTermPicNum = longTermFrameIndex;
              }
            return;
            }
          }

        if (dpb->fsRef[i]->mIsReference & 2) {
          if ((!dpb->fsRef[i]->botField->isLongTerm) &&
              (dpb->fsRef[i]->botField->picNum == picNumX)) {
            if ((dpb->fsRef[i]->isLongTerm) &&
                (dpb->fsRef[i]->longTermFrameIndex != longTermFrameIndex))
              printf ("Warning: assigning longTermFrameIndex different from other field\n");

            dpb->fsRef[i]->longTermFrameIndex = dpb->fsRef[i]->botField->longTermFrameIndex
                                                = longTermFrameIndex;
            dpb->fsRef[i]->botField->longTermPicNum = 2 * longTermFrameIndex + addBot;
            dpb->fsRef[i]->botField->isLongTerm = 1;
            dpb->fsRef[i]->isLongTerm |= 2;
            if (dpb->fsRef[i]->isLongTerm == 3) {
              dpb->fsRef[i]->frame->isLongTerm = 1;
              dpb->fsRef[i]->frame->longTermFrameIndex = dpb->fsRef[i]->frame->longTermPicNum = longTermFrameIndex;
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
  void unmarkLongTermFrameForRefByFrameIndex (sDpb* dpb, int longTermFrameIndex) {

    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      if (dpb->fsLongTermRef[i]->longTermFrameIndex == longTermFrameIndex)
        dpb->fsLongTermRef[i]->unmarkForLongTermRef ();
    }
  //}}}
  //{{{
  void unmarkLongTermForRef (sDpb* dpb, sPicture* p, int longTermPicNum) {

    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (p->picStructure == eFrame) {
        if ((dpb->fsLongTermRef[i]->mIsReference == 3) && (dpb->fsLongTermRef[i]->isLongTerm == 3))
          if (dpb->fsLongTermRef[i]->frame->longTermPicNum == longTermPicNum)
            dpb->fsLongTermRef[i]->unmarkForLongTermRef ();
        }
      else {
        if ((dpb->fsLongTermRef[i]->mIsReference & 1) && ((dpb->fsLongTermRef[i]->isLongTerm & 1)))
          if (dpb->fsLongTermRef[i]->topField->longTermPicNum == longTermPicNum) {
            dpb->fsLongTermRef[i]->topField->usedForReference = 0;
            dpb->fsLongTermRef[i]->topField->isLongTerm = 0;
            dpb->fsLongTermRef[i]->mIsReference &= 2;
            dpb->fsLongTermRef[i]->isLongTerm &= 2;
            if (dpb->fsLongTermRef[i]->isUsed == 3) {
              dpb->fsLongTermRef[i]->frame->usedForReference = 0;
              dpb->fsLongTermRef[i]->frame->isLongTerm = 0;
              }
            return;
            }

        if ((dpb->fsLongTermRef[i]->mIsReference & 2) && ((dpb->fsLongTermRef[i]->isLongTerm & 2)))
          if (dpb->fsLongTermRef[i]->botField->longTermPicNum == longTermPicNum) {
            dpb->fsLongTermRef[i]->botField->usedForReference = 0;
            dpb->fsLongTermRef[i]->botField->isLongTerm = 0;
            dpb->fsLongTermRef[i]->mIsReference &= 1;
            dpb->fsLongTermRef[i]->isLongTerm &= 1;
            if (dpb->fsLongTermRef[i]->isUsed == 3) {
              dpb->fsLongTermRef[i]->frame->usedForReference = 0;
              dpb->fsLongTermRef[i]->frame->isLongTerm = 0;
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
        if ((dpb->fsRef[i]->mIsReference == 3) && (dpb->fsRef[i]->isLongTerm == 0)) {
          if (dpb->fsRef[i]->frame->picNum == picNumX) {
            dpb->fsRef[i]->unmarkForRef();
            return;
            }
          }
        }
      else {
        if ((dpb->fsRef[i]->mIsReference & 1) && (!(dpb->fsRef[i]->isLongTerm & 1))) {
          if (dpb->fsRef[i]->topField->picNum == picNumX) {
            dpb->fsRef[i]->topField->usedForReference = 0;
            dpb->fsRef[i]->mIsReference &= 2;
            if (dpb->fsRef[i]->isUsed == 3)
              dpb->fsRef[i]->frame->usedForReference = 0;
            return;
            }
          }

        if ((dpb->fsRef[i]->mIsReference & 2) && (!(dpb->fsRef[i]->isLongTerm & 2))) {
          if (dpb->fsRef[i]->botField->picNum == picNumX) {
            dpb->fsRef[i]->botField->usedForReference = 0;
            dpb->fsRef[i]->mIsReference &= 1;
            if (dpb->fsRef[i]->isUsed == 3)
              dpb->fsRef[i]->frame->usedForReference = 0;
            return;
            }
          }
        }
      }
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
        if (dpb->fsRef[i]->mIsReference & 1) {
          if (dpb->fsRef[i]->topField->picNum == picNumX) {
            picStructure = eTopField;
            break;
            }
          }

        if (dpb->fsRef[i]->mIsReference & 2) {
          if (dpb->fsRef[i]->botField->picNum == picNumX) {
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
  void updateMaxLongTermFrameIndex (sDpb* dpb, int maxLongTermFrameIndexPlus1) {

    dpb->maxLongTermPicIndex = maxLongTermFrameIndexPlus1 - 1;

    // check for invalid frames
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      if (dpb->fsLongTermRef[i]->longTermFrameIndex > dpb->maxLongTermPicIndex)
        dpb->fsLongTermRef[i]->unmarkForLongTermRef();
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
      dpb->fsRef[i]->unmarkForRef ();
    updateRefList (dpb);
    }
  //}}}
  //{{{
  void markCurPicLongTerm (sDpb* dpb, sPicture* p, int longTermFrameIndex) {

    // remove long term pictures with same longTermFrameIndex
    if (p->picStructure == eFrame)
      unmarkLongTermFrameForRefByFrameIndex(dpb, longTermFrameIndex);
    else
      unmarkLongTermFieldRefFrameIndex(dpb, p->picStructure, longTermFrameIndex, 1, p->picNum, 0);

    p->isLongTerm = 1;
    p->longTermFrameIndex = longTermFrameIndex;
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
          updateRefList (dpb);
          break;
        //}}}
        //{{{
        case 2:
          unmarkLongTermForRef (dpb, p, tmp_drpm->longTermPicNum);
          updateLongTermRefList (dpb);
          break;
        //}}}
        //{{{
        case 3:
          assignLongTermFrameIndex (dpb, p, tmp_drpm->diffPicNumMinus1, tmp_drpm->longTermFrameIndex);
          updateRefList (dpb);
          updateLongTermRefList(dpb);
          break;
        //}}}
        //{{{
        case 4:
          updateMaxLongTermFrameIndex (dpb, tmp_drpm->maxLongTermFrameIndexPlus1);
          updateLongTermRefList (dpb);
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

      flushDpb(decoder->dpb);
      }
    }
  //}}}
  //{{{
  void slidingWindowMemoryManagement (sDpb* dpb, sPicture* p) {

    // if this is a reference pic with sliding window, unmark first ref frame
    if (dpb->refFramesInBuffer == imax (1, dpb->numRefFrames) - dpb->longTermRefFramesInBuffer) {
      for (uint32_t i = 0; i < dpb->usedSize; i++) {
        if (dpb->frameStore[i]->mIsReference && (!(dpb->frameStore[i]->isLongTerm))) {
          dpb->frameStore[i]->unmarkForRef ();
          updateRefList (dpb);
          break;
          }
        }
      }

    p->isLongTerm = 0;
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
        dpb->fsRef[i]=NULL;
      for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
        dpb->fsLongTermRef[i]=NULL;
      dpb->usedSize = 0;
      }
    else
      flushDpb (dpb);

    dpb->lastPicture = NULL;

    updateRefList (dpb);
    updateLongTermRefList (dpb);
    dpb->lastOutputPoc = INT_MIN;

    if (p->longTermRefFlag) {
      dpb->maxLongTermPicIndex = 0;
      p->isLongTerm = 1;
      p->longTermFrameIndex = 0;
      }
    else {
      dpb->maxLongTermPicIndex = -1;
      p->isLongTerm = 0;
      }
    }
  //}}}
  //{{{
  sPicture* getLongTermPic (cSlice* slice, sDpb* dpb, int LongtermPicNum) {

    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (slice->picStructure == eFrame) {
        if (dpb->fsLongTermRef[i]->mIsReference == 3)
          if ((dpb->fsLongTermRef[i]->frame->isLongTerm)&&(dpb->fsLongTermRef[i]->frame->longTermPicNum == LongtermPicNum))
            return dpb->fsLongTermRef[i]->frame;
        }
      else {
        if (dpb->fsLongTermRef[i]->mIsReference & 1)
          if ((dpb->fsLongTermRef[i]->topField->isLongTerm)&&(dpb->fsLongTermRef[i]->topField->longTermPicNum == LongtermPicNum))
            return dpb->fsLongTermRef[i]->topField;
        if (dpb->fsLongTermRef[i]->mIsReference & 2)
          if ((dpb->fsLongTermRef[i]->botField->isLongTerm)&&(dpb->fsLongTermRef[i]->botField->longTermPicNum == LongtermPicNum))
            return dpb->fsLongTermRef[i]->botField;
        }
      }

    return NULL;
    }
  //}}}
  //{{{
  void genPicListFromFrameList (ePicStructure currStructure, cFrameStore** fs_list,
                                int list_idx, sPicture** list, char *list_size, int long_term) {

    int top_idx = 0;
    int bot_idx = 0;

    int (*is_ref)(sPicture*s) = (long_term) ? isLongRef : isShortRef;

    if (currStructure == eTopField) {
      while ((top_idx<list_idx)||(bot_idx<list_idx)) {
        for ( ; top_idx<list_idx; top_idx++) {
          if (fs_list[top_idx]->isUsed & 1) {
            if (is_ref(fs_list[top_idx]->topField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = fs_list[top_idx]->topField;
              (*list_size)++;
              top_idx++;
              break;
              }
            }
          }

        for ( ; bot_idx<list_idx; bot_idx++) {
          if (fs_list[bot_idx]->isUsed & 2) {
            if (is_ref(fs_list[bot_idx]->botField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = fs_list[bot_idx]->botField;
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
          if (fs_list[bot_idx]->isUsed & 2) {
            if (is_ref(fs_list[bot_idx]->botField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = fs_list[bot_idx]->botField;
              (*list_size)++;
              bot_idx++;
              break;
              }
            }
          }

        for ( ; top_idx<list_idx; top_idx++) {
          if (fs_list[top_idx]->isUsed & 1) {
            if (is_ref(fs_list[top_idx]->topField)) {
              // int16_t term ref pic
              list[(int16_t) *list_size] = fs_list[top_idx]->topField;
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
        if ((refPicListX[cIdx]->isLongTerm) || (refPicListX[cIdx]->picNum != picNumLX))
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
        if ((!refPicListX[cIdx]->isLongTerm) || (refPicListX[cIdx]->longTermPicNum != LongTermPicNum))
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
  s->isLongTerm = 0;
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
      for (int i = 0; i < 2; i++) {
        s->listX[j][i] = (sPicture**)calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering
        if (!s->listX[j][i])
          noMemoryExit ("allocPicture: s->listX[i]");
        }

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
    storePictureDpb (slice->dpb, picture);

    decoder->preFrameNum = unusedShortTermFrameNum;
    unusedShortTermFrameNum = (unusedShortTermFrameNum + 1) % decoder->coding.maxFrameNum;
    }

  slice->deltaPicOrderCount[0] = tmp1;
  slice->deltaPicOrderCount[1] = tmp2;
  slice->frameNum = curFrameNum;
  }
//}}}

// dpb
//{{{
void updateRefList (sDpb* dpb) {

  uint32_t i, j;
  for (i = 0, j = 0; i < dpb->usedSize; i++)
    if (dpb->frameStore[i]->isShortTermReference ())
      dpb->fsRef[j++]=dpb->frameStore[i];

  dpb->refFramesInBuffer = j;

  while (j < dpb->size)
    dpb->fsRef[j++] = NULL;
  }
//}}}
//{{{
void updateLongTermRefList (sDpb* dpb) {

  uint32_t i, j;
  for (i = 0, j = 0; i < dpb->usedSize; i++)
    if (dpb->frameStore[i]->isLongTermReference ())
      dpb->fsLongTermRef[j++] = dpb->frameStore[i];

  dpb->longTermRefFramesInBuffer = j;

  while (j < dpb->size)
    dpb->fsLongTermRef[j++] = NULL;
  }
//}}}
//{{{
void getSmallestPoc (sDpb* dpb, int* poc, int* pos) {

  if (dpb->usedSize<1)
    cDecoder264::error ("Cannot determine smallest POC, DPB empty");

  *pos = -1;
  *poc = INT_MAX;
  for (uint32_t i = 0; i < dpb->usedSize; i++) {
    if ((*poc > dpb->frameStore[i]->poc)&&(!dpb->frameStore[i]->isOutput)) {
      *poc = dpb->frameStore[i]->poc;
      *pos = i;
      }
    }
  }
//}}}

//{{{
void initDpb (cDecoder264* decoder, sDpb* dpb, int type) {

  cSps* activeSps = decoder->activeSps;

  dpb->decoder = decoder;
  if (dpb->initDone)
    freeDpb (dpb);

  dpb->size = getDpbSize (decoder, activeSps) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  dpb->numRefFrames = activeSps->numRefFrames;
  if (dpb->size < activeSps->numRefFrames)
    cDecoder264::error ("DPB size at specified level is smaller than the specified number of reference frames\n");

  dpb->usedSize = 0;
  dpb->lastPicture = NULL;

  dpb->refFramesInBuffer = 0;
  dpb->longTermRefFramesInBuffer = 0;

  dpb->frameStore = (cFrameStore**)calloc (dpb->size, sizeof (cFrameStore*));
  dpb->fsRef = (cFrameStore**)calloc (dpb->size, sizeof (cFrameStore*));
  dpb->fsLongTermRef = (cFrameStore**)calloc (dpb->size, sizeof (cFrameStore*));

  for (uint32_t i = 0; i < dpb->size; i++) {
    dpb->frameStore[i] = new cFrameStore();
    dpb->fsRef[i] = NULL;
    dpb->fsLongTermRef[i] = NULL;
    }

  // allocate a dummy storable picture
  if (!decoder->noReferencePicture) {
    decoder->noReferencePicture = allocPicture (decoder, eFrame, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);
    decoder->noReferencePicture->topField = decoder->noReferencePicture;
    decoder->noReferencePicture->botField = decoder->noReferencePicture;
    decoder->noReferencePicture->frame = decoder->noReferencePicture;
    }
  dpb->lastOutputPoc = INT_MIN;
  decoder->lastHasMmco5 = 0;
  dpb->initDone = 1;

  // picture error conceal
  if ((decoder->concealMode != 0) && !decoder->lastOutFramestore)
    decoder->lastOutFramestore = new cFrameStore();
  }
//}}}
//{{{
void reInitDpb (cDecoder264* decoder, sDpb* dpb, int type) {

  cSps* activeSps = decoder->activeSps;
  int dpbSize = getDpbSize (decoder, activeSps) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  dpb->numRefFrames = activeSps->numRefFrames;

  if (dpbSize > (int)dpb->size) {
    if (dpb->size < activeSps->numRefFrames)
      cDecoder264::error ("DPB size at specified level is smaller than the specified number of reference frames\n");

    dpb->frameStore = (cFrameStore**)realloc (dpb->frameStore, dpbSize * sizeof (cFrameStore*));
    dpb->fsRef = (cFrameStore**)realloc(dpb->fsRef, dpbSize * sizeof (cFrameStore*));
    dpb->fsLongTermRef = (cFrameStore**)realloc(dpb->fsLongTermRef, dpbSize * sizeof (cFrameStore*));

    for (int i = dpb->size; i < dpbSize; i++) {
      dpb->frameStore[i] = new cFrameStore();
      dpb->fsRef[i] = NULL;
      dpb->fsLongTermRef[i] = NULL;
      }

    dpb->size = dpbSize;
    dpb->lastOutputPoc = INT_MIN;
    dpb->initDone = 1;
    }
  }
//}}}
//{{{
void flushDpb (sDpb* dpb) {

  if (!dpb->initDone)
    return;

  cDecoder264* decoder = dpb->decoder;
  if (decoder->concealMode != 0)
    conceal_non_ref_pics (dpb, 0);

  // mark all frames unused
  for (uint32_t i = 0; i < dpb->usedSize; i++)
    dpb->frameStore[i]->unmarkForRef ();

  while (removeUnusedDpb (dpb));

  // output frames in POC order
  while (dpb->usedSize && outputDpbFrame (dpb));

  dpb->lastOutputPoc = INT_MIN;
  }
//}}}
//{{{
int removeUnusedDpb (sDpb* dpb) {

  // check for frames that were already output and no longer used for reference
  for (uint32_t i = 0; i < dpb->usedSize; i++) {
    if (dpb->frameStore[i]->isOutput && (!dpb->frameStore[i]->isReference())) {
      removeFrameDpb(dpb, i);
      return 1;
      }
    }

  return 0;
  }
//}}}
//{{{
void storePictureDpb (sDpb* dpb, sPicture* picture) {

  cDecoder264* decoder = dpb->decoder;
  int poc, pos;
  // picture error conceal

  // if frame, check for new store,
  decoder->lastHasMmco5 = 0;
  decoder->lastPicBotField = (picture->picStructure == eBotField);

  if (picture->isIDR) {
    idrMemoryManagement (dpb, picture);
    // picture error conceal
    memset (decoder->dpbPoc, 0, sizeof(int)*100);
    }
  else {
    // adaptive memory management
    if (picture->usedForReference && (picture->adaptRefPicBufFlag))
      adaptiveMemoryManagement (dpb, picture);
    }

  if ((picture->picStructure == eTopField) || (picture->picStructure == eBotField)) {
    // check for frame store with same pic_number
    if (dpb->lastPicture) {
      if ((int)dpb->lastPicture->frameNum == picture->picNum) {
        if (((picture->picStructure == eTopField) && (dpb->lastPicture->isUsed == 2))||
            ((picture->picStructure == eBotField) && (dpb->lastPicture->isUsed == 1))) {
          if ((picture->usedForReference && (dpb->lastPicture->isOrigReference != 0)) ||
              (!picture->usedForReference && (dpb->lastPicture->isOrigReference == 0))) {
            dpb->lastPicture->insertPictureDpb (decoder, picture);
            updateRefList (dpb);
            updateLongTermRefList (dpb);
            dumpDpb (dpb);
            dpb->lastPicture = NULL;
            return;
            }
          }
        }
      }
    }

  // this is a frame or a field which has no stored complementary field sliding window, if necessary
  if ((!picture->isIDR) && (picture->usedForReference && (!picture->adaptRefPicBufFlag)))
    slidingWindowMemoryManagement (dpb, picture);

  // picture error conceal
  if (decoder->concealMode != 0)
    for (uint32_t i = 0; i < dpb->size; i++)
      if (dpb->frameStore[i]->mIsReference)
        dpb->frameStore[i]->concealRef = 1;

  // first try to remove unused frames
  if (dpb->usedSize == dpb->size) {
    // picture error conceal
    if (decoder->concealMode != 0)
      conceal_non_ref_pics (dpb, 2);

    removeUnusedDpb (dpb);

    if (decoder->concealMode != 0)
      sliding_window_poc_management (dpb, picture);
    }

  // then output frames until one can be removed
  while (dpb->usedSize == dpb->size) {
    // non-reference frames may be output directly
    if (!picture->usedForReference) {
      getSmallestPoc (dpb, &poc, &pos);
      if ((-1 == pos) || (picture->poc < poc)) {
        directOutput (decoder, picture);
        return;
        }
      }

    // flush a frame
    outputDpbFrame (dpb);
    }

  // check for duplicate frame number in int16_t term reference buffer
  if ((picture->usedForReference) && (!picture->isLongTerm))
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->frameNum == picture->frameNum)
        cDecoder264::error ("duplicate frameNum in int16_t-term reference picture buffer");

  // store at end of buffer
  dpb->frameStore[dpb->usedSize]->insertPictureDpb (decoder,  picture);

  // picture error conceal
  if (picture->isIDR)
    decoder->earlierMissingPoc = 0;

  if (picture->picStructure != eFrame)
    dpb->lastPicture = dpb->frameStore[dpb->usedSize];
  else
    dpb->lastPicture = NULL;

  dpb->usedSize++;

  if (decoder->concealMode != 0)
    decoder->dpbPoc[dpb->usedSize-1] = picture->poc;

  updateRefList (dpb);
  updateLongTermRefList (dpb);
  checkNumDpbFrames (dpb);
  dumpDpb (dpb);
  }
//}}}
//{{{
void removeFrameDpb (sDpb* dpb, int pos) {

  cFrameStore* frameStore = dpb->frameStore[pos];
  switch (frameStore->isUsed) {
    case 3:
      freePicture (frameStore->frame);
      freePicture (frameStore->topField);
      freePicture (frameStore->botField);
      frameStore->frame = NULL;
      frameStore->topField = NULL;
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
  frameStore->isLongTerm = 0;
  frameStore->mIsReference = 0;
  frameStore->isOrigReference = 0;

  // move empty framestore to end of buffer
  cFrameStore* tmp = dpb->frameStore[pos];
  for (uint32_t i = pos; i < dpb->usedSize-1; i++)
    dpb->frameStore[i] = dpb->frameStore[i+1];
  dpb->frameStore[dpb->usedSize-1] = tmp;
  dpb->usedSize--;
  }
//}}}
//{{{
void freeDpb (sDpb* dpb) {

  cDecoder264* decoder = dpb->decoder;
  if (dpb->frameStore) {
    for (uint32_t i = 0; i < dpb->size; i++)
      delete dpb->frameStore[i];
    free (dpb->frameStore);
    dpb->frameStore = NULL;
    }

  if (dpb->fsRef)
    free (dpb->fsRef);
  if (dpb->fsLongTermRef)
    free (dpb->fsLongTermRef);


  dpb->lastOutputPoc = INT_MIN;
  dpb->initDone = 0;

  // picture error conceal
  if (decoder->concealMode != 0 || decoder->lastOutFramestore)
    delete decoder->lastOutFramestore;

  if (decoder->noReferencePicture) {
    freePicture (decoder->noReferencePicture);
    decoder->noReferencePicture = NULL;
    }
  }
//}}}

// image
//{{{
void initImage (cDecoder264* decoder, sImage* image, cSps* sps) {

  // allocate memory for reference frame buffers: image->frm_data
  image->format = decoder->param.output;
  image->format.width[0]  = decoder->coding.width;
  image->format.width[1]  = decoder->widthCr;
  image->format.width[2]  = decoder->widthCr;
  image->format.height[0] = decoder->coding.height;
  image->format.height[1] = decoder->heightCr;
  image->format.height[2] = decoder->heightCr;
  image->format.yuvFormat  = (eYuvFormat)sps->chromaFormatIdc;
  image->frm_stride[0] = decoder->coding.width;
  image->frm_stride[1] = image->frm_stride[2] = decoder->widthCr;
  image->top_stride[0] = image->bot_stride[0] = image->frm_stride[0] << 1;
  image->top_stride[1] = image->top_stride[2] = image->bot_stride[1] = image->bot_stride[2] = image->frm_stride[1] << 1;

  if (sps->isSeperateColourPlane) {
    for (int nplane = 0; nplane < MAX_PLANE; nplane++ )
      getMem2Dpel (&(image->frm_data[nplane]), decoder->coding.height, decoder->coding.width);
    }
  else {
    getMem2Dpel (&image->frm_data[0], decoder->coding.height, decoder->coding.width);
    if (decoder->coding.yuvFormat != YUV400) {
      getMem2Dpel (&image->frm_data[1], decoder->heightCr, decoder->widthCr);
      getMem2Dpel (&image->frm_data[2], decoder->heightCr, decoder->widthCr);
      if (sizeof(sPixel) == sizeof(uint8_t)) {
        for (int k = 1; k < 3; k++)
          memset (image->frm_data[k][0], 128, decoder->heightCr * decoder->widthCr * sizeof(sPixel));
        }
      else {
        sPixel mean_val;
        for (int k = 1; k < 3; k++) {
          mean_val = (sPixel)((decoder->coding.maxPelValueComp[k] + 1) >> 1);
          for (int j = 0; j < decoder->heightCr; j++)
            for (int i = 0; i < decoder->widthCr; i++)
              image->frm_data[k][j][i] = mean_val;
          }
        }
      }
    }

  if (!decoder->activeSps->frameMbOnly) {
    // allocate memory for field reference frame buffers
    initTopBotPlanes (image->frm_data[0], decoder->coding.height, &(image->top_data[0]), &(image->bot_data[0]));
    if (decoder->coding.yuvFormat != YUV400) {
      initTopBotPlanes (image->frm_data[1], decoder->heightCr, &(image->top_data[1]), &(image->bot_data[1]));
      initTopBotPlanes (image->frm_data[2], decoder->heightCr, &(image->top_data[2]), &(image->bot_data[2]));
      }
    }
  }
//}}}
//{{{
void freeImage (cDecoder264* decoder, sImage* image) {

  if (decoder->coding.isSeperateColourPlane ) {
    for (int nplane = 0; nplane < MAX_PLANE; nplane++ ) {
      if (image->frm_data[nplane]) {
        freeMem2Dpel (image->frm_data[nplane]);      // free ref frame buffers
        image->frm_data[nplane] = NULL;
        }
      }
    }

  else {
    if (image->frm_data[0]) {
      freeMem2Dpel (image->frm_data[0]);      // free ref frame buffers
      image->frm_data[0] = NULL;
      }

    if (image->format.yuvFormat != YUV400) {
      if (image->frm_data[1]) {
        freeMem2Dpel (image->frm_data[1]);
        image->frm_data[1] = NULL;
        }
      if (image->frm_data[2]) {
        freeMem2Dpel (image->frm_data[2]);
        image->frm_data[2] = NULL;
        }
      }
    }

  if (!decoder->activeSps->frameMbOnly) {
    freeTopBotPlanes (image->top_data[0], image->bot_data[0]);
    if (image->format.yuvFormat != YUV400) {
      freeTopBotPlanes (image->top_data[1], image->bot_data[1]);
      freeTopBotPlanes (image->top_data[2], image->bot_data[2]);
      }
    }
  }
//}}}

// slice
//{{{
void reorderRefPicList (cSlice* slice, int curList) {

  int* modPicNumsIdc = slice->modPicNumsIdc[curList];
  int* absDiffPicNumMinus1 = slice->absDiffPicNumMinus1[curList];
  int* longTermPicIndex = slice->longTermPicIndex[curList];
  int numRefIndexIXactiveMinus1 = slice->numRefIndexActive[curList] - 1;

  cDecoder264* decoder = slice->decoder;

  int maxPicNum, currPicNum, picNumLXNoWrap, picNumLXPred, picNumLX;
  int refIdxLX = 0;

  if (slice->picStructure==eFrame) {
    maxPicNum = decoder->coding.maxFrameNum;
    currPicNum = slice->frameNum;
    }
  else {
    maxPicNum = 2 * decoder->coding.maxFrameNum;
    currPicNum = 2 * slice->frameNum + 1;
    }

  picNumLXPred = currPicNum;

  for (int i = 0; modPicNumsIdc[i] != 3; i++) {
    if (modPicNumsIdc[i]>3)
      cDecoder264::error ("Invalid modPicNumsIdc command");

    if (modPicNumsIdc[i] < 2) {
      if (modPicNumsIdc[i] == 0) {
        if (picNumLXPred - (absDiffPicNumMinus1[i] + 1) < 0)
          picNumLXNoWrap = picNumLXPred - (absDiffPicNumMinus1[i] + 1) + maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred - (absDiffPicNumMinus1[i] + 1);
        }
      else {
        if( picNumLXPred + (absDiffPicNumMinus1[i] + 1)  >=  maxPicNum )
          picNumLXNoWrap = picNumLXPred + (absDiffPicNumMinus1[i] + 1) - maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred + (absDiffPicNumMinus1[i] + 1);
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

  // that's a definition
  slice->listXsize[curList] = (char)(numRefIndexIXactiveMinus1 + 1);
  }
//}}}

//{{{
void updatePicNum (cSlice* slice) {

  cDecoder264* decoder = slice->decoder;
  cSps* activeSps = decoder->activeSps;

  int addTop = 0;
  int addBot = 0;
  int maxFrameNum = 1 << (activeSps->log2maxFrameNumMinus4 + 4);

  sDpb* dpb = slice->dpb;
  if (slice->picStructure == eFrame) {
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fsRef[i]->isUsed == 3 ) {
        if ((dpb->fsRef[i]->frame->usedForReference)&&(!dpb->fsRef[i]->frame->isLongTerm)) {
          if (dpb->fsRef[i]->frameNum > slice->frameNum )
            dpb->fsRef[i]->frameNumWrap = dpb->fsRef[i]->frameNum - maxFrameNum;
          else
            dpb->fsRef[i]->frameNumWrap = dpb->fsRef[i]->frameNum;
          dpb->fsRef[i]->frame->picNum = dpb->fsRef[i]->frameNumWrap;
          }
        }
      }

    // update longTermPicNum
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->fsLongTermRef[i]->isUsed == 3) {
        if (dpb->fsLongTermRef[i]->frame->isLongTerm)
          dpb->fsLongTermRef[i]->frame->longTermPicNum = dpb->fsLongTermRef[i]->frame->longTermFrameIndex;
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
      if (dpb->fsRef[i]->mIsReference) {
        if( dpb->fsRef[i]->frameNum > slice->frameNum )
          dpb->fsRef[i]->frameNumWrap = dpb->fsRef[i]->frameNum - maxFrameNum;
        else
          dpb->fsRef[i]->frameNumWrap = dpb->fsRef[i]->frameNum;
        if (dpb->fsRef[i]->mIsReference & 1)
          dpb->fsRef[i]->topField->picNum = (2 * dpb->fsRef[i]->frameNumWrap) + addTop;
        if (dpb->fsRef[i]->mIsReference & 2)
          dpb->fsRef[i]->botField->picNum = (2 * dpb->fsRef[i]->frameNumWrap) + addBot;
        }
      }

    // update longTermPicNum
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->fsLongTermRef[i]->isLongTerm & 1)
        dpb->fsLongTermRef[i]->topField->longTermPicNum = 2 * dpb->fsLongTermRef[i]->topField->longTermFrameIndex + addTop;
      if (dpb->fsLongTermRef[i]->isLongTerm & 2)
        dpb->fsLongTermRef[i]->botField->longTermPicNum = 2 * dpb->fsLongTermRef[i]->botField->longTermFrameIndex + addBot;
      }
    }
  }
//}}}
//{{{
sPicture* getShortTermPic (cSlice* slice, sDpb* dpb, int picNum) {

  for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
    if (slice->picStructure == eFrame) {
      if (dpb->fsRef[i]->mIsReference == 3)
        if ((!dpb->fsRef[i]->frame->isLongTerm)&&(dpb->fsRef[i]->frame->picNum == picNum))
          return dpb->fsRef[i]->frame;
      }
    else {
      if (dpb->fsRef[i]->mIsReference & 1)
        if ((!dpb->fsRef[i]->topField->isLongTerm)&&(dpb->fsRef[i]->topField->picNum == picNum))
          return dpb->fsRef[i]->topField;
      if (dpb->fsRef[i]->mIsReference & 2)
        if ((!dpb->fsRef[i]->botField->isLongTerm)&&(dpb->fsRef[i]->botField->picNum == picNum))
          return dpb->fsRef[i]->botField;
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
  cFrameStore** fsListLongTerm;

  if (slice->picStructure == eFrame) {
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed == 3)
        if ((dpb->fsRef[i]->frame->usedForReference) && (!dpb->fsRef[i]->frame->isLongTerm))
          slice->listX[0][list0idx++] = dpb->fsRef[i]->frame;

    // order list 0 by picNum
    qsort ((void *)slice->listX[0], list0idx, sizeof(sPicture*), comparePicByPicNumDescending);
    slice->listXsize[0] = (char) list0idx;

    // long term handling
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      if (dpb->fsLongTermRef[i]->isUsed == 3)
        if (dpb->fsLongTermRef[i]->frame->isLongTerm)
          slice->listX[0][list0idx++] = dpb->fsLongTermRef[i]->frame;
    qsort ((void*)&slice->listX[0][(int16_t)slice->listXsize[0]], list0idx - slice->listXsize[0],
           sizeof(sPicture*), comparePicByLtPicNumAscending);
    slice->listXsize[0] = (char) list0idx;
    }
  else {
    frameStoreList0 = (cFrameStore**)calloc (dpb->size, sizeof(cFrameStore*));
    fsListLongTerm = (cFrameStore**)calloc (dpb->size, sizeof(cFrameStore*));
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->mIsReference)
        frameStoreList0[list0idx++] = dpb->fsRef[i];
    qsort ((void*)frameStoreList0, list0idx, sizeof(cFrameStore*), compareFsByFrameNumDescending);
    slice->listXsize[0] = 0;
    genPicListFromFrameList(slice->picStructure, frameStoreList0, list0idx, slice->listX[0], &slice->listXsize[0], 0);

    // long term handling
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      fsListLongTerm[listLtIndex++] = dpb->fsLongTermRef[i];
    qsort ((void*)fsListLongTerm, listLtIndex, sizeof(cFrameStore*), compareFsbyLtPicIndexAscending);
    genPicListFromFrameList (slice->picStructure, fsListLongTerm, listLtIndex, slice->listX[0], &slice->listXsize[0], 1);

    free (frameStoreList0);
    free (fsListLongTerm);
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

  uint32_t i;
  int j;

  int list0idx = 0;
  int list0index1 = 0;
  int listLtIndex = 0;

  cFrameStore** frameStoreList0;
  cFrameStore** frameStoreList1;
  cFrameStore** fsListLongTerm;

  // B Slice
  if (slice->picStructure == eFrame) {
    //{{{  frame
    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed==3)
        if ((dpb->fsRef[i]->frame->usedForReference) && (!dpb->fsRef[i]->frame->isLongTerm))
          if (slice->framePoc >= dpb->fsRef[i]->frame->poc) // !KS use >= for error conceal
            slice->listX[0][list0idx++] = dpb->fsRef[i]->frame;
    qsort ((void*)slice->listX[0], list0idx, sizeof(sPicture*), comparePicByPocdesc);

    // get the backward reference picture (POC>current POC) in list0;
    list0index1 = list0idx;
    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed==3)
        if ((dpb->fsRef[i]->frame->usedForReference)&&(!dpb->fsRef[i]->frame->isLongTerm))
          if (slice->framePoc < dpb->fsRef[i]->frame->poc)
            slice->listX[0][list0idx++] = dpb->fsRef[i]->frame;
    qsort ((void*)&slice->listX[0][list0index1], list0idx-list0index1, sizeof(sPicture*), comparePicByPocAscending);

    for (j = 0; j < list0index1; j++)
      slice->listX[1][list0idx-list0index1+j]=slice->listX[0][j];
    for (j = list0index1; j < list0idx; j++)
      slice->listX[1][j-list0index1] = slice->listX[0][j];
    slice->listXsize[0] = slice->listXsize[1] = (char) list0idx;

    // long term handling
    for (i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->fsLongTermRef[i]->isUsed == 3) {
        if (dpb->fsLongTermRef[i]->frame->isLongTerm) {
          slice->listX[0][list0idx] = dpb->fsLongTermRef[i]->frame;
          slice->listX[1][list0idx++] = dpb->fsLongTermRef[i]->frame;
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
    fsListLongTerm = (cFrameStore**)calloc(dpb->size, sizeof (cFrameStore*));
    slice->listXsize[0] = 0;
    slice->listXsize[1] = 1;

    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed)
        if (slice->thisPoc >= dpb->fsRef[i]->poc)
          frameStoreList0[list0idx++] = dpb->fsRef[i];
    qsort ((void*)frameStoreList0, list0idx, sizeof(cFrameStore*), comparefsByPocdesc);

    list0index1 = list0idx;
    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed)
        if (slice->thisPoc < dpb->fsRef[i]->poc)
          frameStoreList0[list0idx++] = dpb->fsRef[i];
    qsort ((void*)&frameStoreList0[list0index1], list0idx-list0index1, sizeof(cFrameStore*), compareFsByPocAscending);

    for (j = 0; j < list0index1; j++)
      frameStoreList1[list0idx-list0index1+j]=frameStoreList0[j];
    for (j = list0index1; j < list0idx; j++)
      frameStoreList1[j-list0index1]=frameStoreList0[j];

    slice->listXsize[0] = 0;
    slice->listXsize[1] = 0;
    genPicListFromFrameList (slice->picStructure, frameStoreList0, list0idx, slice->listX[0], &slice->listXsize[0], 0);
    genPicListFromFrameList (slice->picStructure, frameStoreList1, list0idx, slice->listX[1], &slice->listXsize[1], 0);

    // long term handling
    for (i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      fsListLongTerm[listLtIndex++] = dpb->fsLongTermRef[i];

    qsort ((void*)fsListLongTerm, listLtIndex, sizeof(cFrameStore*), compareFsbyLtPicIndexAscending);
    genPicListFromFrameList (slice->picStructure, fsListLongTerm, listLtIndex, slice->listX[0], &slice->listXsize[0], 1);
    genPicListFromFrameList (slice->picStructure, fsListLongTerm, listLtIndex, slice->listX[1], &slice->listXsize[1], 1);

    free (frameStoreList0);
    free (frameStoreList1);
    free (fsListLongTerm);
    }
    //}}}

  if ((slice->listXsize[0] == slice->listXsize[1]) && (slice->listXsize[0] > 1)) {
    // check if lists are identical, if yes swap first two elements of slice->listX[1]
    int diff = 0;
    for (j = 0; j< slice->listXsize[0]; j++) {
      if (slice->listX[0][j] != slice->listX[1][j]) {
        diff = 1;
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
  for (i = slice->listXsize[0]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[0][i] = decoder->noReferencePicture;
  for (i = slice->listXsize[1]; i < (MAX_LIST_SIZE) ; i++)
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
