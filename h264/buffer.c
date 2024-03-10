//{{{  includes
#include <limits.h>

#include "global.h"
#include "memory.h"

#include "erc.h"
#include "sliceHeader.h"
#include "image.h"
#include "buffer.h"
#include "output.h"
//}}}
#define MAX_LIST_SIZE 33
//#define DUMP_DPB

//{{{
static int isReference (sFrameStore* frameStore) {

  if (frameStore->isReference)
    return 1;

  if (frameStore->isUsed == 3) // frame
    if (frameStore->frame->usedForReference)
      return 1;

  if (frameStore->isUsed & 1) // top field
    if (frameStore->topField)
      if (frameStore->topField->usedForReference)
        return 1;

  if (frameStore->isUsed & 2) // bottom field
    if (frameStore->botField)
      if (frameStore->botField->usedForReference)
        return 1;

  return 0;
  }
//}}}
//{{{
static int isShortTermReference (sFrameStore* frameStore) {

  if (frameStore->isUsed == 3) // frame
    if ((frameStore->frame->usedForReference) && (!frameStore->frame->isLongTerm))
      return 1;

  if (frameStore->isUsed & 1) // top field
    if (frameStore->topField)
      if ((frameStore->topField->usedForReference) && (!frameStore->topField->isLongTerm))
        return 1;

  if (frameStore->isUsed & 2) // bottom field
    if (frameStore->botField)
      if ((frameStore->botField->usedForReference) && (!frameStore->botField->isLongTerm))
        return 1;

  return 0;
  }
//}}}
//{{{
static int isLongTermReference (sFrameStore* frameStore) {

  if (frameStore->isUsed == 3) // frame
    if ((frameStore->frame->usedForReference) && (frameStore->frame->isLongTerm))
      return 1;

  if (frameStore->isUsed & 1) // top field
    if (frameStore->topField)
      if ((frameStore->topField->usedForReference) && (frameStore->topField->isLongTerm))
        return 1;

  if (frameStore->isUsed & 2) // bottom field
    if (frameStore->botField)
      if ((frameStore->botField->usedForReference) && (frameStore->botField->isLongTerm))
        return 1;

  return 0;
  }
//}}}
//{{{
static void gen_field_ref_ids (sDecoder* decoder, sPicture* p) {
// Generate Frame parameters from field information.

  // copy the list;
  for (int j = 0; j < decoder->curPicSliceNum; j++) {
    if (p->listX[j][LIST_0]) {
      p->listXsize[j][LIST_0] = decoder->sliceList[j]->listXsize[LIST_0];
      for (int i = 0; i < p->listXsize[j][LIST_0]; i++)
        p->listX[j][LIST_0][i] = decoder->sliceList[j]->listX[LIST_0][i];
      }

    if(p->listX[j][LIST_1]) {
      p->listXsize[j][LIST_1] = decoder->sliceList[j]->listXsize[LIST_1];
      for(int i = 0; i < p->listXsize[j][LIST_1]; i++)
        p->listX[j][LIST_1][i] = decoder->sliceList[j]->listX[LIST_1][i];
      }
    }
  }
//}}}
//{{{
static void unmark_long_term_field_for_reference_by_frame_idx (

  sDPB* dpb, ePicStructure structure,
  int longTermFrameIndex, int mark_current, unsigned curr_frame_num, int curr_pic_num) {

  sDecoder* decoder = dpb->decoder;

  assert (structure!=FRAME);
  if (curr_pic_num < 0)
    curr_pic_num += (2 * decoder->maxFrameNum);

  for (unsigned i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
    if (dpb->fsLongTermRef[i]->longTermFrameIndex == longTermFrameIndex) {
      if (structure == TopField) {
        if (dpb->fsLongTermRef[i]->isLongTerm == 3)
          unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
        else {
          if (dpb->fsLongTermRef[i]->isLongTerm == 1)
            unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
          else {
            if (mark_current) {
              if (dpb->lastPicture) {
                if ((dpb->lastPicture != dpb->fsLongTermRef[i]) ||
                    dpb->lastPicture->frameNum != curr_frame_num)
                  unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
                }
              else
                unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
            }
            else {
              if ((dpb->fsLongTermRef[i]->frameNum) != (unsigned)(curr_pic_num >> 1))
                unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
              }
            }
          }
        }

      if (structure == BotField) {
        if (dpb->fsLongTermRef[i]->isLongTerm == 3)
          unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
        else {
          if (dpb->fsLongTermRef[i]->isLongTerm == 2)
            unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
          else {
            if (mark_current) {
              if (dpb->lastPicture) {
                if ((dpb->lastPicture != dpb->fsLongTermRef[i]) ||
                    dpb->lastPicture->frameNum != curr_frame_num)
                  unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
                }
              else
                unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
              }
            else {
              if ((dpb->fsLongTermRef[i]->frameNum) != (unsigned)(curr_pic_num >> 1))
                unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
              }
            }
          }
        }
      }
    }
  }
//}}}
//{{{
static int get_pic_num_x (sPicture* p, int difference_of_pic_nums_minus1) {

  int currPicNum;
  if (p->structure == FRAME)
    currPicNum = p->frameNum;
  else
    currPicNum = 2 * p->frameNum + 1;

  return currPicNum - (difference_of_pic_nums_minus1 + 1);
  }
//}}}
//{{{
static int outputDpbFrame (sDPB* dpb) {

  // diagnostics
  if (dpb->usedSize < 1)
    error ("Cannot output frame, DPB empty.",150);

  // find smallest POC
  int poc, pos;
  get_smallest_poc (dpb, &poc, &pos);
  if (pos == -1)
    return 0;

  // call the output function
  //printf ("output frame with frameNum #%d, poc %d (dpb. dpb->size=%d, dpb->usedSize=%d)\n", dpb->fs[pos]->frameNum, dpb->fs[pos]->frame->poc, dpb->size, dpb->usedSize);

  // picture error conceal
  sDecoder* decoder = dpb->decoder;
  if (decoder->concealMode != 0) {
    if (dpb->lastOutputPoc == 0)
      write_lost_ref_after_idr (dpb, pos);
    write_lost_non_ref_pic (dpb, poc);
    }

  writeStoredFrame (decoder, dpb->fs[pos]);

  // picture error conceal
  if(decoder->concealMode == 0)
    if (dpb->lastOutputPoc >= poc)
      error ("output POC must be in ascending order", 150);

  dpb->lastOutputPoc = poc;

  // free frame store and move empty store to end of buffer
  if (!isReference (dpb->fs[pos]))
    removeFrameDpb (dpb, pos);

  return 1;
  }
//}}}

// picMotion
//{{{
void allocPicMotion (sPicMotionParamsOld* motion, int sizeY, int sizeX) {

  motion->mbField = calloc (sizeY * sizeX, sizeof(byte));
  if (motion->mbField == NULL)
    no_mem_exit ("allocPicture: motion->mbField");
  }
//}}}
//{{{
void freePicMotion (sPicMotionParamsOld* motion) {

  if (motion->mbField) {
    free (motion->mbField);
    motion->mbField = NULL;
    }
  }
//}}}

// frameStore
//{{{
sFrameStore* allocFrameStore() {

  sFrameStore* fs = calloc (1, sizeof(sFrameStore));
  if (!fs)
    no_mem_exit ("alloc_frame_store: f");

  fs->isUsed = 0;
  fs->isReference = 0;
  fs->isLongTerm = 0;
  fs->isOrigReference = 0;
  fs->is_output = 0;
  fs->frame = NULL;;
  fs->topField = NULL;
  fs->botField = NULL;

  return fs;
  }
//}}}
//{{{
void freeFrameStore (sFrameStore* frameStore) {

  if (frameStore) {
    if (frameStore->frame) {
      freePicture (frameStore->frame);
      frameStore->frame = NULL;
      }

    if (frameStore->topField) {
      freePicture (frameStore->topField);
      frameStore->topField = NULL;
      }

    if (frameStore->botField) {
      freePicture (frameStore->botField);
      frameStore->botField = NULL;
      }

    free (frameStore);
    }
  }
//}}}

//{{{
static void dpbSplitField (sDecoder* decoder, sFrameStore* frameStore) {

  int twosz16 = 2 * (frameStore->frame->sizeX >> 4);
  sPicture* fsTop = NULL;
  sPicture* fsBot = NULL;
  sPicture* frame = frameStore->frame;

  frameStore->poc = frame->poc;
  if (!frame->frameMbOnlyFlag) {
    fsTop = frameStore->topField = allocPicture (decoder, TopField,
                                                   frame->sizeX, frame->sizeY, frame->sizeXcr,
                                                   frame->sizeYcr, 1);
    fsBot = frameStore->botField = allocPicture (decoder, BotField,
                                                      frame->sizeX, frame->sizeY,
                                                      frame->sizeXcr, frame->sizeYcr, 1);

    for (int i = 0; i < (frame->sizeY >> 1); i++)
      memcpy (fsTop->imgY[i], frame->imgY[i*2], frame->sizeX*sizeof(sPixel));

    for (int i = 0; i< (frame->sizeYcr >> 1); i++) {
      memcpy (fsTop->imgUV[0][i], frame->imgUV[0][i*2], frame->sizeXcr*sizeof(sPixel));
      memcpy (fsTop->imgUV[1][i], frame->imgUV[1][i*2], frame->sizeXcr*sizeof(sPixel));
      }

    for (int i = 0; i < (frame->sizeY>>1); i++)
      memcpy (fsBot->imgY[i], frame->imgY[i*2 + 1], frame->sizeX*sizeof(sPixel));

    for (int i = 0; i < (frame->sizeYcr>>1); i++) {
      memcpy (fsBot->imgUV[0][i], frame->imgUV[0][i*2 + 1], frame->sizeXcr*sizeof(sPixel));
      memcpy (fsBot->imgUV[1][i], frame->imgUV[1][i*2 + 1], frame->sizeXcr*sizeof(sPixel));
      }

    fsTop->poc = frame->topPoc;
    fsBot->poc = frame->botPoc;
    fsTop->framePoc = frame->framePoc;
    fsTop->botPoc = fsBot->botPoc =  frame->botPoc;
    fsTop->topPoc = fsBot->topPoc =  frame->topPoc;
    fsBot->framePoc = frame->framePoc;

    fsTop->usedForReference = fsBot->usedForReference = frame->usedForReference;
    fsTop->isLongTerm = fsBot->isLongTerm = frame->isLongTerm;
    frameStore->longTermFrameIndex = fsTop->longTermFrameIndex
                                    = fsBot->longTermFrameIndex
                                    = frame->longTermFrameIndex;

    fsTop->codedFrame = fsBot->codedFrame = 1;
    fsTop->mbAffFrameFlag = fsBot->mbAffFrameFlag = frame->mbAffFrameFlag;

    frame->topField = fsTop;
    frame->botField = fsBot;
    frame->frame = frame;
    fsTop->botField = fsBot;
    fsTop->frame = frame;
    fsTop->topField = fsTop;
    fsBot->topField = fsTop;
    fsBot->frame = frame;
    fsBot->botField = fsBot;

    fsTop->chromaFormatIdc = fsBot->chromaFormatIdc = frame->chromaFormatIdc;
    fsTop->iCodingType = fsBot->iCodingType = frame->iCodingType;
    if (frame->usedForReference)  {
      padPicture (decoder, fsTop);
      padPicture (decoder, fsBot);
      }
    }
  else {
    frameStore->topField = NULL;
    frameStore->botField = NULL;
    frame->topField = NULL;
    frame->botField = NULL;
    frame->frame = frame;
    }

  if (!frame->frameMbOnlyFlag) {
    if (frame->mbAffFrameFlag) {
      sPicMotionParamsOld* frm_motion = &frame->motion;
      for (int j = 0 ; j < (frame->sizeY >> 3); j++) {
        int jj = (j >> 2)*8 + (j & 0x03);
        int jj4 = jj + 4;
        int jdiv = (j >> 1);
        for (int i = 0 ; i < (frame->sizeX>>2); i++) {
          int idiv = (i >> 2);

          int currentmb = twosz16*(jdiv >> 1)+ (idiv)*2 + (jdiv & 0x01);
          // Assign field mvs attached to MB-Frame buffer to the proper buffer
          if (frm_motion->mbField[currentmb]) {
            fsBot->mvInfo[j][i].mv[LIST_0] = frame->mvInfo[jj4][i].mv[LIST_0];
            fsBot->mvInfo[j][i].mv[LIST_1] = frame->mvInfo[jj4][i].mv[LIST_1];
            fsBot->mvInfo[j][i].refIndex[LIST_0] = frame->mvInfo[jj4][i].refIndex[LIST_0];
            if (fsBot->mvInfo[j][i].refIndex[LIST_0] >=0)
              fsBot->mvInfo[j][i].refPic[LIST_0] = decoder->sliceList[frame->mvInfo[jj4][i].slice_no]->listX[4][(short) fsBot->mvInfo[j][i].refIndex[LIST_0]];
            else
              fsBot->mvInfo[j][i].refPic[LIST_0] = NULL;
            fsBot->mvInfo[j][i].refIndex[LIST_1] = frame->mvInfo[jj4][i].refIndex[LIST_1];
            if (fsBot->mvInfo[j][i].refIndex[LIST_1] >=0)
              fsBot->mvInfo[j][i].refPic[LIST_1] = decoder->sliceList[frame->mvInfo[jj4][i].slice_no]->listX[5][(short) fsBot->mvInfo[j][i].refIndex[LIST_1]];
            else
              fsBot->mvInfo[j][i].refPic[LIST_1] = NULL;

            fsTop->mvInfo[j][i].mv[LIST_0] = frame->mvInfo[jj][i].mv[LIST_0];
            fsTop->mvInfo[j][i].mv[LIST_1] = frame->mvInfo[jj][i].mv[LIST_1];
            fsTop->mvInfo[j][i].refIndex[LIST_0] = frame->mvInfo[jj][i].refIndex[LIST_0];
            if (fsTop->mvInfo[j][i].refIndex[LIST_0] >=0)
              fsTop->mvInfo[j][i].refPic[LIST_0] = decoder->sliceList[frame->mvInfo[jj][i].slice_no]->listX[2][(short) fsTop->mvInfo[j][i].refIndex[LIST_0]];
            else
              fsTop->mvInfo[j][i].refPic[LIST_0] = NULL;
            fsTop->mvInfo[j][i].refIndex[LIST_1] = frame->mvInfo[jj][i].refIndex[LIST_1];
            if (fsTop->mvInfo[j][i].refIndex[LIST_1] >=0)
              fsTop->mvInfo[j][i].refPic[LIST_1] = decoder->sliceList[frame->mvInfo[jj][i].slice_no]->listX[3][(short) fsTop->mvInfo[j][i].refIndex[LIST_1]];
            else
              fsTop->mvInfo[j][i].refPic[LIST_1] = NULL;
            }
          }
        }
      }

      //! Generate field MVs from Frame MVs
    for (int j = 0 ; j < (frame->sizeY >> 3) ; j++) {
      int jj = 2 * RSD(j);
      int jdiv = (j >> 1);
      for (int i = 0 ; i < (frame->sizeX >> 2) ; i++) {
        int ii = RSD(i);
        int idiv = (i >> 2);
        int currentmb = twosz16 * (jdiv >> 1)+ (idiv)*2 + (jdiv & 0x01);
        if (!frame->mbAffFrameFlag  || !frame->motion.mbField[currentmb]) {
          fsTop->mvInfo[j][i].mv[LIST_0] = fsBot->mvInfo[j][i].mv[LIST_0] = frame->mvInfo[jj][ii].mv[LIST_0];
          fsTop->mvInfo[j][i].mv[LIST_1] = fsBot->mvInfo[j][i].mv[LIST_1] = frame->mvInfo[jj][ii].mv[LIST_1];

          // Scaling of references is done here since it will not affect spatial direct (2*0 =0)
          if (frame->mvInfo[jj][ii].refIndex[LIST_0] == -1) {
            fsTop->mvInfo[j][i].refIndex[LIST_0] = fsBot->mvInfo[j][i].refIndex[LIST_0] = - 1;
            fsTop->mvInfo[j][i].refPic[LIST_0] = fsBot->mvInfo[j][i].refPic[LIST_0] = NULL;
            }
          else {
            fsTop->mvInfo[j][i].refIndex[LIST_0] = fsBot->mvInfo[j][i].refIndex[LIST_0] = frame->mvInfo[jj][ii].refIndex[LIST_0];
            fsTop->mvInfo[j][i].refPic[LIST_0] = fsBot->mvInfo[j][i].refPic[LIST_0] = decoder->sliceList[frame->mvInfo[jj][ii].slice_no]->listX[LIST_0][(short) frame->mvInfo[jj][ii].refIndex[LIST_0]];
            }

          if (frame->mvInfo[jj][ii].refIndex[LIST_1] == -1) {
            fsTop->mvInfo[j][i].refIndex[LIST_1] = fsBot->mvInfo[j][i].refIndex[LIST_1] = - 1;
            fsTop->mvInfo[j][i].refPic[LIST_1] = fsBot->mvInfo[j][i].refPic[LIST_1] = NULL;
            }
          else {
            fsTop->mvInfo[j][i].refIndex[LIST_1] = fsBot->mvInfo[j][i].refIndex[LIST_1] = frame->mvInfo[jj][ii].refIndex[LIST_1];
            fsTop->mvInfo[j][i].refPic[LIST_1] = fsBot->mvInfo[j][i].refPic[LIST_1] = decoder->sliceList[frame->mvInfo[jj][ii].slice_no]->listX[LIST_1][(short) frame->mvInfo[jj][ii].refIndex[LIST_1]];
            }
          }
        }
      }
    }
  }
//}}}
//{{{
static void dpb_combine_field (sDecoder* decoder, sFrameStore* frameStore) {

  dpbCombineField (decoder, frameStore);

  frameStore->frame->iCodingType = frameStore->topField->iCodingType; //FIELD_CODING;
   //! Use inference flag to remap mvs/references

  //! Generate Frame parameters from field information.
  for (int j = 0 ; j < (frameStore->topField->sizeY >> 2); j++) {
    int jj = (j << 1);
    int jj4 = jj + 1;
    for (int i = 0 ; i < (frameStore->topField->sizeX >> 2); i++) {
      frameStore->frame->mvInfo[jj][i].mv[LIST_0] = frameStore->topField->mvInfo[j][i].mv[LIST_0];
      frameStore->frame->mvInfo[jj][i].mv[LIST_1] = frameStore->topField->mvInfo[j][i].mv[LIST_1];

      frameStore->frame->mvInfo[jj][i].refIndex[LIST_0] = frameStore->topField->mvInfo[j][i].refIndex[LIST_0];
      frameStore->frame->mvInfo[jj][i].refIndex[LIST_1] = frameStore->topField->mvInfo[j][i].refIndex[LIST_1];

      /* bug: top field list doesnot exist.*/
      int l = frameStore->topField->mvInfo[j][i].slice_no;
      int k = frameStore->topField->mvInfo[j][i].refIndex[LIST_0];
      frameStore->frame->mvInfo[jj][i].refPic[LIST_0] = k>=0? frameStore->topField->listX[l][LIST_0][k]: NULL;
      k = frameStore->topField->mvInfo[j][i].refIndex[LIST_1];
      frameStore->frame->mvInfo[jj][i].refPic[LIST_1] = k>=0? frameStore->topField->listX[l][LIST_1][k]: NULL;

      //! association with id already known for fields.
      frameStore->frame->mvInfo[jj4][i].mv[LIST_0] = frameStore->botField->mvInfo[j][i].mv[LIST_0];
      frameStore->frame->mvInfo[jj4][i].mv[LIST_1] = frameStore->botField->mvInfo[j][i].mv[LIST_1];

      frameStore->frame->mvInfo[jj4][i].refIndex[LIST_0] = frameStore->botField->mvInfo[j][i].refIndex[LIST_0];
      frameStore->frame->mvInfo[jj4][i].refIndex[LIST_1] = frameStore->botField->mvInfo[j][i].refIndex[LIST_1];
      l = frameStore->botField->mvInfo[j][i].slice_no;

      k = frameStore->botField->mvInfo[j][i].refIndex[LIST_0];
      frameStore->frame->mvInfo[jj4][i].refPic[LIST_0] = k >= 0 ? frameStore->botField->listX[l][LIST_0][k]: NULL;
      k = frameStore->botField->mvInfo[j][i].refIndex[LIST_1];
      frameStore->frame->mvInfo[jj4][i].refPic[LIST_1] = k >= 0 ? frameStore->botField->listX[l][LIST_1][k]: NULL;
    }
  }
}
//}}}
//{{{
static void insertPictureDpb (sDecoder* decoder, sFrameStore* fs, sPicture* p) {

  //  printf ("insert (%s) pic with frameNum #%d, poc %d\n",
  //          (p->structure == FRAME)?"FRAME":(p->structure == TopField)?"TopField":"BotField",
  //          p->picNum, p->poc);
  assert (p != NULL);
  assert (fs != NULL);

  switch (p->structure) {
    //{{{
    case FRAME:
      fs->frame = p;
      fs->isUsed = 3;
      if (p->usedForReference) {
        fs->isReference = 3;
        fs->isOrigReference = 3;
        if (p->isLongTerm) {
          fs->isLongTerm = 3;
          fs->longTermFrameIndex = p->longTermFrameIndex;
          }
        }

      // generate field views
      dpbSplitField (decoder, fs);
      break;
    //}}}
    //{{{
    case TopField:
      fs->topField = p;
      fs->isUsed |= 1;

      if (p->usedForReference) {
        fs->isReference |= 1;
        fs->isOrigReference |= 1;
        if (p->isLongTerm) {
          fs->isLongTerm |= 1;
          fs->longTermFrameIndex = p->longTermFrameIndex;
          }
        }
      if (fs->isUsed == 3)
        // generate frame view
        dpb_combine_field(decoder, fs);
      else
        fs->poc = p->poc;

      gen_field_ref_ids(decoder, p);
      break;
    //}}}
    //{{{
    case BotField:
      fs->botField = p;
      fs->isUsed |= 2;

      if (p->usedForReference) {
        fs->isReference |= 2;
        fs->isOrigReference |= 2;
        if (p->isLongTerm) {
          fs->isLongTerm |= 2;
          fs->longTermFrameIndex = p->longTermFrameIndex;
          }
        }

      if (fs->isUsed == 3)
        // generate frame view
        dpb_combine_field(decoder, fs);
      else
        fs->poc = p->poc;

      gen_field_ref_ids(decoder, p);
      break;
    //}}}
    }

  fs->frameNum = p->picNum;
  fs->recoveryFrame = p->recoveryFrame;
  fs->is_output = p->is_output;
  if (fs->isUsed == 3)
    calcFrameNum (decoder, p);
}
//}}}
//{{{
void unmark_for_reference (sFrameStore* frameStore) {

  if (frameStore->isUsed & 1)
    if (frameStore->topField)
      frameStore->topField->usedForReference = 0;

  if (frameStore->isUsed & 2)
    if (frameStore->botField)
      frameStore->botField->usedForReference = 0;

  if (frameStore->isUsed == 3) {
    if (frameStore->topField && frameStore->botField) {
      frameStore->topField->usedForReference = 0;
      frameStore->botField->usedForReference = 0;
      }
    frameStore->frame->usedForReference = 0;
    }

  frameStore->isReference = 0;

  if(frameStore->frame)
    freePicMotion (&frameStore->frame->motion);

  if (frameStore->topField)
    freePicMotion (&frameStore->topField->motion);

  if (frameStore->botField)
    freePicMotion (&frameStore->botField->motion);
  }
//}}}
//{{{
void unmark_for_long_term_reference (sFrameStore* frameStore) {

  if (frameStore->isUsed & 1) {
    if (frameStore->topField) {
      frameStore->topField->usedForReference = 0;
      frameStore->topField->isLongTerm = 0;
      }
    }

  if (frameStore->isUsed & 2) {
    if (frameStore->botField) {
      frameStore->botField->usedForReference = 0;
      frameStore->botField->isLongTerm = 0;
      }
    }

  if (frameStore->isUsed == 3) {
    if (frameStore->topField && frameStore->botField) {
      frameStore->topField->usedForReference = 0;
      frameStore->topField->isLongTerm = 0;
      frameStore->botField->usedForReference = 0;
      frameStore->botField->isLongTerm = 0;
      }
    frameStore->frame->usedForReference = 0;
    frameStore->frame->isLongTerm = 0;
    }

  frameStore->isReference = 0;
  frameStore->isLongTerm = 0;
  }
//}}}
//{{{
void dpbCombineField (sDecoder* decoder, sFrameStore* frameStore) {

  if (!frameStore->frame)
    frameStore->frame = allocPicture (decoder, FRAME,
                                      frameStore->topField->sizeX, frameStore->topField->sizeY*2,
                                      frameStore->topField->sizeXcr, frameStore->topField->sizeYcr*2, 1);

  for (int i = 0; i < frameStore->topField->sizeY; i++) {
    memcpy (frameStore->frame->imgY[i*2], frameStore->topField->imgY[i]   ,
            frameStore->topField->sizeX * sizeof(sPixel));     // top field
    memcpy (frameStore->frame->imgY[i*2 + 1], frameStore->botField->imgY[i],
            frameStore->botField->sizeX * sizeof(sPixel)); // bottom field
    }

  for (int j = 0; j < 2; j++)
    for (int i = 0; i < frameStore->topField->sizeYcr; i++) {
      memcpy (frameStore->frame->imgUV[j][i*2], frameStore->topField->imgUV[j][i],
              frameStore->topField->sizeXcr*sizeof(sPixel));
      memcpy (frameStore->frame->imgUV[j][i*2 + 1], frameStore->botField->imgUV[j][i],
              frameStore->botField->sizeXcr*sizeof(sPixel));
      }

  frameStore->poc = frameStore->frame->poc = frameStore->frame->framePoc
                                           = imin (frameStore->topField->poc, frameStore->botField->poc);

  frameStore->botField->framePoc = frameStore->topField->framePoc = frameStore->frame->poc;
  frameStore->botField->topPoc = frameStore->frame->topPoc = frameStore->topField->poc;
  frameStore->topField->botPoc = frameStore->frame->botPoc = frameStore->botField->poc;

  frameStore->frame->usedForReference = (frameStore->topField->usedForReference && frameStore->botField->usedForReference );
  frameStore->frame->isLongTerm = (frameStore->topField->isLongTerm && frameStore->botField->isLongTerm );

  if (frameStore->frame->isLongTerm)
    frameStore->frame->longTermFrameIndex = frameStore->longTermFrameIndex;

  frameStore->frame->topField = frameStore->topField;
  frameStore->frame->botField = frameStore->botField;
  frameStore->frame->frame = frameStore->frame;
  frameStore->frame->codedFrame = 0;

  frameStore->frame->chromaFormatIdc = frameStore->topField->chromaFormatIdc;
  frameStore->frame->frameCropFlag = frameStore->topField->frameCropFlag;
  if (frameStore->frame->frameCropFlag) {
    frameStore->frame->frameCropTop = frameStore->topField->frameCropTop;
    frameStore->frame->frameCropBot = frameStore->topField->frameCropBot;
    frameStore->frame->frameCropLeft = frameStore->topField->frameCropLeft;
    frameStore->frame->frameCropRight = frameStore->topField->frameCropRight;
    }

  frameStore->topField->frame = frameStore->botField->frame = frameStore->frame;
  frameStore->topField->topField = frameStore->topField;
  frameStore->topField->botField = frameStore->botField;
  frameStore->botField->topField = frameStore->topField;
  frameStore->botField->botField = frameStore->botField;
  if (frameStore->topField->usedForReference || frameStore->botField->usedForReference)
    padPicture (decoder, frameStore->frame);
  }
//}}}

// picture
//{{{
sPicture* allocPicture (sDecoder* decoder, ePicStructure structure,
                        int sizeX, int sizeY, int sizeXcr, int sizeYcr, int is_output) {

  sSPS* activeSPS = decoder->activeSPS;
  //printf ("Allocating (%s) picture (x=%d, y=%d, x_cr=%d, y_cr=%d)\n",
  //        (type == FRAME)?"FRAME":(type == TopField)?"TopField":"BotField",
  //        sizeX, sizeY, sizeXcr, sizeYcr);

  sPicture* s = calloc (1, sizeof(sPicture));
  if (!s)
    no_mem_exit ("allocPicture failed");

  if (structure != FRAME) {
    sizeY /= 2;
    sizeYcr /= 2;
    }

  s->picSizeInMbs = (sizeX*sizeY)/256;
  s->imgUV = NULL;

  get_mem2Dpel_pad (&(s->imgY), sizeY, sizeX, decoder->iLumaPadY, decoder->iLumaPadX);
  s->iLumaStride = sizeX + 2 * decoder->iLumaPadX;
  s->iLumaExpandedHeight = sizeY + 2 * decoder->iLumaPadY;

  if (activeSPS->chromaFormatIdc != YUV400)
    get_mem3Dpel_pad (&(s->imgUV), 2, sizeYcr, sizeXcr, decoder->iChromaPadY, decoder->iChromaPadX);

  s->iChromaStride = sizeXcr + 2*decoder->iChromaPadX;
  s->iChromaExpandedHeight = sizeYcr + 2*decoder->iChromaPadY;
  s->iLumaPadY = decoder->iLumaPadY;
  s->iLumaPadX = decoder->iLumaPadX;
  s->iChromaPadY = decoder->iChromaPadY;
  s->iChromaPadX = decoder->iChromaPadX;
  s->sepColourPlaneFlag = decoder->sepColourPlaneFlag;

  get_mem2Dmp (&s->mvInfo, (sizeY >> BLOCK_SHIFT), (sizeX >> BLOCK_SHIFT));
  allocPicMotion (&s->motion , (sizeY >> BLOCK_SHIFT), (sizeX >> BLOCK_SHIFT));

  if (decoder->sepColourPlaneFlag != 0)
    for (int nplane = 0; nplane < MAX_PLANE; nplane++) {
      get_mem2Dmp (&s->JVmv_info[nplane], (sizeY >> BLOCK_SHIFT), (sizeX >> BLOCK_SHIFT));
      allocPicMotion (&s->JVmotion[nplane] , (sizeY >> BLOCK_SHIFT), (sizeX >> BLOCK_SHIFT));
      }

  s->picNum = 0;
  s->frameNum = 0;
  s->longTermFrameIndex = 0;
  s->longTermPicNum = 0;
  s->usedForReference  = 0;
  s->isLongTerm = 0;
  s->non_existing = 0;
  s->is_output = 0;
  s->maxSliceId = 0;

  s->structure = structure;

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

  s->decRefPicMarkingBuffer = NULL;
  s->codedFrame  = 0;
  s->mbAffFrameFlag  = 0;
  s->topPoc = s->botPoc = s->poc = 0;

  if (!decoder->activeSPS->frameMbOnlyFlag && structure != FRAME)
    for (int j = 0; j < MAX_NUM_SLICES; j++)
      for (int i = 0; i < 2; i++) {
        s->listX[j][i] = calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering
        if (!s->listX[j][i])
          no_mem_exit ("allocPicture: s->listX[i]");
        }

  return s;
  }
//}}}
//{{{
void freePicture (sPicture* p) {

  if (p) {
    if (p->mvInfo) {
      free_mem2Dmp (p->mvInfo);
      p->mvInfo = NULL;
      }
    freePicMotion (&p->motion);

    if ((p->sepColourPlaneFlag != 0) ) {
      for (int nplane = 0; nplane < MAX_PLANE; nplane++ ) {
        if (p->JVmv_info[nplane]) {
          free_mem2Dmp (p->JVmv_info[nplane]);
          p->JVmv_info[nplane] = NULL;
          }
        freePicMotion (&p->JVmotion[nplane]);
        }
      }

    if (p->imgY) {
      free_mem2Dpel_pad (p->imgY, p->iLumaPadY, p->iLumaPadX);
      p->imgY = NULL;
      }

    if (p->imgUV) {
      free_mem3Dpel_pad (p->imgUV, 2, p->iChromaPadY, p->iChromaPadX);
      p->imgUV = NULL;
      }

    for (int j = 0; j < MAX_NUM_SLICES; j++)
      for (int i = 0; i < 2; i++)
        if (p->listX[j][i]) {
          free (p->listX[j][i]);
          p->listX[j][i] = NULL;
          }

    free (p);
    p = NULL;
    }
  }
//}}}
//{{{
void fillFrameNumGap (sDecoder* decoder, sSlice* slice) {

  sSPS* activeSPS = decoder->activeSPS;

  int tmp1 = slice->deltaPicOrderCount[0];
  int tmp2 = slice->deltaPicOrderCount[1];
  slice->deltaPicOrderCount[0] = slice->deltaPicOrderCount[1] = 0;

  printf ("A gap in frame number is found, try to fill it.\n");

  int unusedShortTermFrameNum = (decoder->preFrameNum + 1) % decoder->maxFrameNum;
  int curFrameNum = slice->frameNum; //decoder->frameNum;

  sPicture* picture = NULL;
  while (curFrameNum != unusedShortTermFrameNum) {
    picture = allocPicture (decoder, FRAME, decoder->width, decoder->height, decoder->widthCr, decoder->heightCr, 1);
    picture->codedFrame = 1;
    picture->picNum = unusedShortTermFrameNum;
    picture->frameNum = unusedShortTermFrameNum;
    picture->non_existing = 1;
    picture->is_output = 1;
    picture->usedForReference = 1;
    picture->adaptiveRefPicBufferingFlag = 0;

    slice->frameNum = unusedShortTermFrameNum;
    if (activeSPS->picOrderCountType != 0)
      decodePOC (decoder, decoder->sliceList[0]);
    picture->topPoc = slice->topPoc;
    picture->botPoc = slice->botPoc;
    picture->framePoc = slice->framePoc;
    picture->poc = slice->framePoc;
    storePictureDpb (slice->dpb, picture);

    decoder->preFrameNum = unusedShortTermFrameNum;
    unusedShortTermFrameNum = (unusedShortTermFrameNum + 1) % decoder->maxFrameNum;
    }

  slice->deltaPicOrderCount[0] = tmp1;
  slice->deltaPicOrderCount[1] = tmp2;
  slice->frameNum = curFrameNum;
  }
//}}}

// dpb
//{{{
static void dumpDpb (sDPB* dpb) {

#ifdef DUMP_DPB
  for (unsigned i = 0; i < dpb->usedSize; i++) {
    printf ("(");
    printf ("fn=%d  ", dpb->fs[i]->frameNum);
    if (dpb->fs[i]->isUsed & 1) {
      if (dpb->fs[i]->topField)
        printf ("T: poc=%d  ", dpb->fs[i]->topField->poc);
      else
        printf ("T: poc=%d  ", dpb->fs[i]->frame->topPoc);
      }

    if (dpb->fs[i]->isUsed & 2) {
      if (dpb->fs[i]->botField)
        printf ("B: poc=%d  ", dpb->fs[i]->botField->poc);
      else
        printf ("B: poc=%d  ", dpb->fs[i]->frame->botPoc);
      }

    if (dpb->fs[i]->isUsed == 3)
      printf ("F: poc=%d  ", dpb->fs[i]->frame->poc);
    printf ("G: poc=%d)  ", dpb->fs[i]->poc);

    if (dpb->fs[i]->isReference)
      printf ("ref (%d) ", dpb->fs[i]->isReference);
    if (dpb->fs[i]->isLongTerm)
      printf ("lt_ref (%d) ", dpb->fs[i]->isReference);
    if (dpb->fs[i]->is_output)
      printf ("out  ");
    if (dpb->fs[i]->isUsed == 3)
      if (dpb->fs[i]->frame->non_existing)
        printf ("ne  ");

    printf ("\n");
    }
#endif
  }
//}}}
//{{{
static int getDpbSize (sDecoder* decoder, sSPS *activeSPS) {

  int pic_size_mb = (activeSPS->pic_width_in_mbs_minus1 + 1) * (activeSPS->pic_height_in_map_units_minus1 + 1) * (activeSPS->frameMbOnlyFlag?1:2);
  int size = 0;

  switch (activeSPS->level_idc) {
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
      if (!isFrextProfile(activeSPS->profileIdc) && (activeSPS->constrained_set3_flag == 1))
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
      error ("undefined level", 500);
      break;
    //}}}
    }

  size /= pic_size_mb;
    size = imin( size, 16);

  if (activeSPS->vui_parameters_present_flag && activeSPS->vui_seq_parameters.bitstream_restriction_flag) {
    int size_vui;
    if ((int)activeSPS->vui_seq_parameters.max_dec_frame_buffering > size)
      error ("max_dec_frame_buffering larger than MaxDpbSize", 500);

    size_vui = imax (1, activeSPS->vui_seq_parameters.max_dec_frame_buffering);
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
void initDpb (sDecoder* decoder, sDPB* dpb, int type) {

  sSPS* activeSPS = decoder->activeSPS;

  dpb->decoder = decoder;
  if (dpb->initDone)
    freeDpb (dpb);

  dpb->size = getDpbSize (decoder, activeSPS) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  dpb->numRefFrames = activeSPS->numRefFrames;

  if (dpb->size < activeSPS->numRefFrames)
    error ("DPB size at specified level is smaller than the specified number of reference frames. This is not allowed.\n", 1000);

  dpb->usedSize = 0;
  dpb->lastPicture = NULL;

  dpb->refFramesInBuffer = 0;
  dpb->longTermRefFramesInBuffer = 0;

  dpb->fs = calloc (dpb->size, sizeof (sFrameStore*));
  if (!dpb->fs)
    no_mem_exit ("initDpb: dpb->fs");

  dpb->fsRef = calloc (dpb->size, sizeof (sFrameStore*));
  if (!dpb->fsRef)
    no_mem_exit ("initDpb: dpb->fsRef");

  dpb->fsLongTermRef = calloc (dpb->size, sizeof (sFrameStore*));
  if (!dpb->fsLongTermRef)
    no_mem_exit ("initDpb: dpb->fsLongTermRef");

  for (unsigned i = 0; i < dpb->size; i++) {
    dpb->fs[i] = allocFrameStore();
    dpb->fsRef[i] = NULL;
    dpb->fsLongTermRef[i] = NULL;
    }

 /* allocate a dummy storable picture */
  if (!decoder->noReferencePicture) {
    decoder->noReferencePicture = allocPicture (decoder, FRAME, decoder->width, decoder->height, decoder->widthCr, decoder->heightCr, 1);
    decoder->noReferencePicture->topField = decoder->noReferencePicture;
    decoder->noReferencePicture->botField = decoder->noReferencePicture;
    decoder->noReferencePicture->frame = decoder->noReferencePicture;
    }
  dpb->lastOutputPoc = INT_MIN;
  decoder->last_has_mmco_5 = 0;
  dpb->initDone = 1;

  // picture error conceal
  if (decoder->concealMode !=0 && !decoder->lastOutFramestore)
    decoder->lastOutFramestore = allocFrameStore();
  }
//}}}
//{{{
void reInitDpb (sDecoder* decoder, sDPB* dpb, int type) {

  sSPS* activeSPS = decoder->activeSPS;
  int dpbSize = getDpbSize (decoder, activeSPS) + decoder->param.dpbPlus[type == 2 ? 1 : 0];
  dpb->numRefFrames = activeSPS->numRefFrames;

  if (dpbSize > (int)dpb->size) {
    if (dpb->size < activeSPS->numRefFrames)
      error ("DPB size at specified level is smaller than the specified number of reference frames\n", 1000);

    dpb->fs = realloc (dpb->fs, dpbSize * sizeof (sFrameStore*));
    if (!dpb->fs)
      no_mem_exit ("reInitDpb: dpb->fs");

    dpb->fsRef = realloc(dpb->fsRef, dpbSize * sizeof (sFrameStore*));
    if (!dpb->fsRef)
      no_mem_exit ("reInitDpb: dpb->fsRef");

    dpb->fsLongTermRef = realloc(dpb->fsLongTermRef, dpbSize * sizeof (sFrameStore*));
    if (!dpb->fsLongTermRef)
      no_mem_exit ("reInitDpb: dpb->fsLongTermRef");

    for (int i = dpb->size; i < dpbSize; i++) {
      dpb->fs[i] = allocFrameStore();
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
void flushDpb (sDPB* dpb) {

  if (!dpb->initDone)
    return;

  sDecoder* decoder = dpb->decoder;
  if (decoder->concealMode != 0)
    conceal_non_ref_pics (dpb, 0);

  // mark all frames unused
  for (uint32 i = 0; i < dpb->usedSize; i++)
    unmark_for_reference (dpb->fs[i]);

  while (removeUnusedDpb (dpb));

  // output frames in POC order
  while (dpb->usedSize && outputDpbFrame (dpb));

  dpb->lastOutputPoc = INT_MIN;
  }
//}}}

//{{{
static void checkNumDpbFrames (sDPB* dpb) {

  if ((int)(dpb->longTermRefFramesInBuffer + dpb->refFramesInBuffer) > imax (1, dpb->numRefFrames))
    error ("Max. number of reference frames exceeded. Invalid stream.", 500);
  }
//}}}
//{{{
static void adaptiveMemoryManagement (sDPB* dpb, sPicture* p) {

  sDecRefPicMarking* tmp_drpm;
  sDecoder* decoder = dpb->decoder;

  decoder->last_has_mmco_5 = 0;

  assert (!p->idrFlag);
  assert (p->adaptiveRefPicBufferingFlag);

  while (p->decRefPicMarkingBuffer) {
    tmp_drpm = p->decRefPicMarkingBuffer;
    switch (tmp_drpm->memory_management_control_operation) {
      //{{{
      case 0:
        if (tmp_drpm->next != NULL)
          error ("memory_management_control_operation = 0 not last operation in buffer", 500);
        break;
      //}}}
      //{{{
      case 1:
        mm_unmark_short_term_for_reference (dpb, p, tmp_drpm->difference_of_pic_nums_minus1);
        updateRefList (dpb);
        break;
      //}}}
      //{{{
      case 2:
        mm_unmark_long_term_for_reference (dpb, p, tmp_drpm->longTermPicNum);
        updateLongTermRefList (dpb);
        break;
      //}}}
      //{{{
      case 3:
        mm_assign_long_term_frame_idx (dpb, p, tmp_drpm->difference_of_pic_nums_minus1, tmp_drpm->longTermFrameIndex);
        updateRefList (dpb);
        updateLongTermRefList(dpb);
        break;
      //}}}
      //{{{
      case 4:
        mm_update_max_long_term_frame_idx (dpb, tmp_drpm->max_long_term_frame_idx_plus1);
        updateLongTermRefList (dpb);
        break;
      //}}}
      //{{{
      case 5:
        mm_unmark_all_short_term_for_reference (dpb);
        mm_unmark_all_long_term_for_reference (dpb);
        decoder->last_has_mmco_5 = 1;
        break;
      //}}}
      //{{{
      case 6:
        mm_mark_current_picture_long_term (dpb, p, tmp_drpm->longTermFrameIndex);
        checkNumDpbFrames (dpb);
        break;
      //}}}
      //{{{
      default:
        error ("invalid memory_management_control_operation in buffer", 500);
      //}}}
      }
    p->decRefPicMarkingBuffer = tmp_drpm->next;
    free (tmp_drpm);
    }

  if (decoder->last_has_mmco_5 ) {
    p->picNum = p->frameNum = 0;
    switch (p->structure) {
      //{{{
      case TopField:
        p->poc = p->topPoc = 0;
        break;
      //}}}
      //{{{
      case BotField:
        p->poc = p->botPoc = 0;
        break;
      //}}}
      //{{{
      case FRAME:
        p->topPoc    -= p->poc;
        p->botPoc -= p->poc;
        p->poc = imin (p->topPoc, p->botPoc);
        break;
      //}}}
      }

    flushDpb(decoder->dpbLayer[0]);
    }
  }
//}}}
//{{{
static void slidingWindowMemoryManagement (sDPB* dpb, sPicture* p) {

  assert (!p->idrFlag);

  // if this is a reference pic with sliding window, unmark first ref frame
  if (dpb->refFramesInBuffer == imax (1, dpb->numRefFrames) - dpb->longTermRefFramesInBuffer) {
    for (uint32 i = 0; i < dpb->usedSize; i++) {
      if (dpb->fs[i]->isReference && (!(dpb->fs[i]->isLongTerm))) {
        unmark_for_reference (dpb->fs[i]);
        updateRefList (dpb);
        break;
        }
      }
    }

  p->isLongTerm = 0;
  }
//}}}
//{{{
static void idrMemoryManagement (sDPB* dpb, sPicture* p) {

  if (p->noOutputPriorPicFlag) {
    // free all stored pictures
    for (uint32 i = 0; i < dpb->usedSize; i++) {
      // reset all reference settings
      freeFrameStore (dpb->fs[i]);
      dpb->fs[i] = allocFrameStore();
      }
    for (uint32 i = 0; i < dpb->refFramesInBuffer; i++)
      dpb->fsRef[i]=NULL;
    for (uint32 i = 0; i < dpb->longTermRefFramesInBuffer; i++)
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
static sPicture* get_long_term_pic (sSlice* curSlice, sDPB* dpb, int LongtermPicNum) {

  for (uint32 i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
    if (curSlice->structure==FRAME) {
      if (dpb->fsLongTermRef[i]->isReference == 3)
        if ((dpb->fsLongTermRef[i]->frame->isLongTerm)&&(dpb->fsLongTermRef[i]->frame->longTermPicNum == LongtermPicNum))
          return dpb->fsLongTermRef[i]->frame;
      }
    else {
      if (dpb->fsLongTermRef[i]->isReference & 1)
        if ((dpb->fsLongTermRef[i]->topField->isLongTerm)&&(dpb->fsLongTermRef[i]->topField->longTermPicNum == LongtermPicNum))
          return dpb->fsLongTermRef[i]->topField;
      if (dpb->fsLongTermRef[i]->isReference & 2)
        if ((dpb->fsLongTermRef[i]->botField->isLongTerm)&&(dpb->fsLongTermRef[i]->botField->longTermPicNum == LongtermPicNum))
          return dpb->fsLongTermRef[i]->botField;
      }
    }

  return NULL;
  }
//}}}
//{{{
static void genPicListFromFrameList (ePicStructure currStructure, sFrameStore** fs_list,
                                          int list_idx, sPicture** list, char *list_size, int long_term) {

  int top_idx = 0;
  int bot_idx = 0;

  int (*is_ref)(sPicture*s) = (long_term) ? is_long_ref : is_short_ref;

  if (currStructure == TopField) {
    while ((top_idx<list_idx)||(bot_idx<list_idx)) {
      for ( ; top_idx<list_idx; top_idx++) {
        if (fs_list[top_idx]->isUsed & 1) {
          if (is_ref(fs_list[top_idx]->topField)) {
            // short term ref pic
            list[(short) *list_size] = fs_list[top_idx]->topField;
            (*list_size)++;
            top_idx++;
            break;
            }
          }
        }

      for ( ; bot_idx<list_idx; bot_idx++) {
        if (fs_list[bot_idx]->isUsed & 2) {
          if (is_ref(fs_list[bot_idx]->botField)) {
            // short term ref pic
            list[(short) *list_size] = fs_list[bot_idx]->botField;
            (*list_size)++;
            bot_idx++;
            break;
            }
          }
        }
      }
    }

  if (currStructure == BotField) {
    while ((top_idx<list_idx)||(bot_idx<list_idx)) {
      for ( ; bot_idx<list_idx; bot_idx++) {
        if (fs_list[bot_idx]->isUsed & 2) {
          if (is_ref(fs_list[bot_idx]->botField)) {
            // short term ref pic
            list[(short) *list_size] = fs_list[bot_idx]->botField;
            (*list_size)++;
            bot_idx++;
            break;
            }
          }
        }

      for ( ; top_idx<list_idx; top_idx++) {
        if (fs_list[top_idx]->isUsed & 1) {
          if (is_ref(fs_list[top_idx]->topField)) {
            // short term ref pic
            list[(short) *list_size] = fs_list[top_idx]->topField;
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
static void mark_pic_long_term (sDPB* dpb, sPicture* p, int longTermFrameIndex, int picNumX) {

  int addTop, addBot;

  if (p->structure == FRAME) {
    for (uint32 i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fsRef[i]->isReference == 3) {
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
    if (p->structure == TopField) {
      addTop = 1;
      addBot = 0;
      }
    else {
      addTop = 0;
      addBot = 1;
      }

    for (uint32 i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fsRef[i]->isReference & 1) {
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

      if (dpb->fsRef[i]->isReference & 2) {
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
static void unmark_long_term_frame_for_reference_by_frame_idx (sDPB* dpb, int longTermFrameIndex) {

  for (uint32 i = 0; i < dpb->longTermRefFramesInBuffer; i++)
    if (dpb->fsLongTermRef[i]->longTermFrameIndex == longTermFrameIndex)
      unmark_for_long_term_reference (dpb->fsLongTermRef[i]);
}
//}}}

//{{{
void updateRefList (sDPB* dpb) {

  unsigned i, j;
  for (i = 0, j = 0; i < dpb->usedSize; i++)
    if (isShortTermReference (dpb->fs[i]))
      dpb->fsRef[j++]=dpb->fs[i];

  dpb->refFramesInBuffer = j;

  while (j < dpb->size)
    dpb->fsRef[j++] = NULL;
  }
//}}}
//{{{
void updateLongTermRefList (sDPB* dpb) {

  unsigned i, j;
  for (i = 0, j = 0; i < dpb->usedSize; i++)
    if (isLongTermReference (dpb->fs[i]))
      dpb->fsLongTermRef[j++] = dpb->fs[i];

  dpb->longTermRefFramesInBuffer = j;

  while (j < dpb->size)
    dpb->fsLongTermRef[j++] = NULL;
  }
//}}}
//{{{
void mm_unmark_long_term_for_reference (sDPB* dpb, sPicture* p, int longTermPicNum) {

  for (uint32 i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
    if (p->structure == FRAME) {
      if ((dpb->fsLongTermRef[i]->isReference==3) && (dpb->fsLongTermRef[i]->isLongTerm==3))
        if (dpb->fsLongTermRef[i]->frame->longTermPicNum == longTermPicNum)
          unmark_for_long_term_reference (dpb->fsLongTermRef[i]);
      }
    else {
      if ((dpb->fsLongTermRef[i]->isReference & 1) && ((dpb->fsLongTermRef[i]->isLongTerm & 1)))
        if (dpb->fsLongTermRef[i]->topField->longTermPicNum == longTermPicNum) {
          dpb->fsLongTermRef[i]->topField->usedForReference = 0;
          dpb->fsLongTermRef[i]->topField->isLongTerm = 0;
          dpb->fsLongTermRef[i]->isReference &= 2;
          dpb->fsLongTermRef[i]->isLongTerm &= 2;
          if (dpb->fsLongTermRef[i]->isUsed == 3) {
            dpb->fsLongTermRef[i]->frame->usedForReference = 0;
            dpb->fsLongTermRef[i]->frame->isLongTerm = 0;
            }
          return;
          }

      if ((dpb->fsLongTermRef[i]->isReference & 2) && ((dpb->fsLongTermRef[i]->isLongTerm & 2)))
        if (dpb->fsLongTermRef[i]->botField->longTermPicNum == longTermPicNum) {
          dpb->fsLongTermRef[i]->botField->usedForReference = 0;
          dpb->fsLongTermRef[i]->botField->isLongTerm = 0;
          dpb->fsLongTermRef[i]->isReference &= 1;
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
void mm_unmark_short_term_for_reference (sDPB* dpb, sPicture* p, int difference_of_pic_nums_minus1)
{
  int picNumX = get_pic_num_x(p, difference_of_pic_nums_minus1);

  for (uint32 i = 0; i < dpb->refFramesInBuffer; i++) {
    if (p->structure == FRAME) {
      if ((dpb->fsRef[i]->isReference==3) && (dpb->fsRef[i]->isLongTerm==0)) {
        if (dpb->fsRef[i]->frame->picNum == picNumX) {
          unmark_for_reference(dpb->fsRef[i]);
          return;
          }
        }
      }
    else {
      if ((dpb->fsRef[i]->isReference & 1) && (!(dpb->fsRef[i]->isLongTerm & 1))) {
        if (dpb->fsRef[i]->topField->picNum == picNumX) {
          dpb->fsRef[i]->topField->usedForReference = 0;
          dpb->fsRef[i]->isReference &= 2;
          if (dpb->fsRef[i]->isUsed == 3)
            dpb->fsRef[i]->frame->usedForReference = 0;
          return;
          }
        }

      if ((dpb->fsRef[i]->isReference & 2) && (!(dpb->fsRef[i]->isLongTerm & 2))) {
        if (dpb->fsRef[i]->botField->picNum == picNumX) {
          dpb->fsRef[i]->botField->usedForReference = 0;
          dpb->fsRef[i]->isReference &= 1;
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
void mm_assign_long_term_frame_idx (sDPB* dpb, sPicture* p,
                                    int difference_of_pic_nums_minus1, int longTermFrameIndex) {

  int picNumX = get_pic_num_x(p, difference_of_pic_nums_minus1);

  // remove frames/fields with same longTermFrameIndex
  if (p->structure == FRAME)
    unmark_long_term_frame_for_reference_by_frame_idx (dpb, longTermFrameIndex);

  else {
    ePicStructure structure = FRAME;
    for (unsigned i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fsRef[i]->isReference & 1) {
        if (dpb->fsRef[i]->topField->picNum == picNumX) {
          structure = TopField;
          break;
          }
        }

      if (dpb->fsRef[i]->isReference & 2) {
        if (dpb->fsRef[i]->botField->picNum == picNumX) {
          structure = BotField;
          break;
          }
        }
      }

    if (structure == FRAME)
      error ("field for long term marking not found", 200);

    unmark_long_term_field_for_reference_by_frame_idx (dpb, structure, longTermFrameIndex, 0, 0, picNumX);
    }

  mark_pic_long_term (dpb, p, longTermFrameIndex, picNumX);
  }
//}}}
//{{{
void mm_update_max_long_term_frame_idx (sDPB* dpb, int max_long_term_frame_idx_plus1) {

  dpb->maxLongTermPicIndex = max_long_term_frame_idx_plus1 - 1;

  // check for invalid frames
  for (uint32 i = 0; i < dpb->longTermRefFramesInBuffer; i++)
    if (dpb->fsLongTermRef[i]->longTermFrameIndex > dpb->maxLongTermPicIndex)
      unmark_for_long_term_reference(dpb->fsLongTermRef[i]);
  }
//}}}
//{{{
void mm_unmark_all_long_term_for_reference (sDPB* dpb) {
  mm_update_max_long_term_frame_idx (dpb, 0);
  }
//}}}
//{{{
void mm_unmark_all_short_term_for_reference (sDPB* dpb) {

  for (unsigned int i = 0; i < dpb->refFramesInBuffer; i++)
    unmark_for_reference (dpb->fsRef[i]);
  updateRefList (dpb);
  }
//}}}
//{{{
void mm_mark_current_picture_long_term (sDPB* dpb, sPicture* p, int longTermFrameIndex) {

  // remove long term pictures with same longTermFrameIndex
  if (p->structure == FRAME)
    unmark_long_term_frame_for_reference_by_frame_idx(dpb, longTermFrameIndex);
  else
    unmark_long_term_field_for_reference_by_frame_idx(dpb, p->structure, longTermFrameIndex, 1, p->picNum, 0);

  p->isLongTerm = 1;
  p->longTermFrameIndex = longTermFrameIndex;
  }
//}}}
//{{{
void get_smallest_poc (sDPB* dpb, int* poc, int* pos) {

  if (dpb->usedSize<1)
    error ("Cannot determine smallest POC, DPB empty.",150);

  *pos = -1;
  *poc = INT_MAX;
  for (uint32 i = 0; i < dpb->usedSize; i++) {
    if ((*poc > dpb->fs[i]->poc)&&(!dpb->fs[i]->is_output)) {
      *poc = dpb->fs[i]->poc;
      *pos = i;
      }
    }
  }
//}}}

//{{{
int removeUnusedDpb (sDPB* dpb) {

  // check for frames that were already output and no longer used for reference
  for (uint32 i = 0; i < dpb->usedSize; i++) {
    if (dpb->fs[i]->is_output && (!isReference(dpb->fs[i]))) {
      removeFrameDpb(dpb, i);
      return 1;
      }
    }

  return 0;
  }
//}}}
//{{{
void storePictureDpb (sDPB* dpb, sPicture* p) {

  sDecoder* decoder = dpb->decoder;
  int poc, pos;
  // picture error conceal

  // diagnostics
  //printf ("Storing (%s) non-ref pic with frameNum #%d\n",
  //        (p->type == FRAME)?"FRAME":(p->type == TopField)?"TopField":"BotField", p->picNum);
  // if frame, check for new store,
  assert (p!=NULL);

  decoder->last_has_mmco_5 = 0;
  decoder->last_pic_bottom_field = (p->structure == BotField);

  if (p->idrFlag) {
    idrMemoryManagement (dpb, p);
    // picture error conceal
    memset (decoder->pocs_in_dpb, 0, sizeof(int)*100);
    }
  else {
    // adaptive memory management
    if (p->usedForReference && (p->adaptiveRefPicBufferingFlag))
      adaptiveMemoryManagement (dpb, p);
    }

  if ((p->structure == TopField) || (p->structure == BotField)) {
    // check for frame store with same pic_number
    if (dpb->lastPicture) {
      if ((int)dpb->lastPicture->frameNum == p->picNum) {
        if (((p->structure == TopField) && (dpb->lastPicture->isUsed == 2))||
            ((p->structure == BotField) && (dpb->lastPicture->isUsed == 1))) {
          if ((p->usedForReference && (dpb->lastPicture->isOrigReference != 0)) ||
              (!p->usedForReference && (dpb->lastPicture->isOrigReference == 0))) {
            insertPictureDpb (decoder, dpb->lastPicture, p);
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
  if ((!p->idrFlag) &&
      (p->usedForReference &&
      (!p->adaptiveRefPicBufferingFlag)))
    slidingWindowMemoryManagement (dpb, p);

  // picture error conceal
  if (decoder->concealMode != 0)
    for (unsigned i = 0; i < dpb->size; i++)
      if (dpb->fs[i]->isReference)
        dpb->fs[i]->concealment_reference = 1;

  // first try to remove unused frames
  if (dpb->usedSize == dpb->size) {
    // picture error conceal
    if (decoder->concealMode != 0)
      conceal_non_ref_pics (dpb, 2);

    removeUnusedDpb (dpb);

    if (decoder->concealMode != 0)
      sliding_window_poc_management (dpb, p);
    }

  // then output frames until one can be removed
  while (dpb->usedSize == dpb->size) {
    // non-reference frames may be output directly
    if (!p->usedForReference) {
      get_smallest_poc (dpb, &poc, &pos);
      if ((-1 == pos) || (p->poc < poc)) {
        directOutput (decoder, p);
        return;
        }
      }

    // flush a frame
    outputDpbFrame (dpb);
    }

  // check for duplicate frame number in short term reference buffer
  if ((p->usedForReference) && (!p->isLongTerm))
    for (unsigned i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->frameNum == p->frameNum)
        error ("duplicate frameNum in short-term reference picture buffer", 500);

  // store at end of buffer
  insertPictureDpb (decoder, dpb->fs[dpb->usedSize],p);

  // picture error conceal
  if (p->idrFlag)
    decoder->earlier_missing_poc = 0;

  if (p->structure != FRAME)
    dpb->lastPicture = dpb->fs[dpb->usedSize];
  else
    dpb->lastPicture = NULL;

  dpb->usedSize++;

  if (decoder->concealMode != 0)
    decoder->pocs_in_dpb[dpb->usedSize-1] = p->poc;

  updateRefList (dpb);
  updateLongTermRefList (dpb);
  checkNumDpbFrames (dpb);
  dumpDpb (dpb);
  }
//}}}
//{{{
void removeFrameDpb (sDPB* dpb, int pos) {

  //printf ("remove frame with frameNum #%d\n", fs->frameNum);
  sFrameStore* fs = dpb->fs[pos];
  switch (fs->isUsed) {
    case 3:
      freePicture (fs->frame);
      freePicture (fs->topField);
      freePicture (fs->botField);
      fs->frame = NULL;
      fs->topField = NULL;
      fs->botField = NULL;
      break;

    case 2:
      freePicture (fs->botField);
      fs->botField = NULL;
      break;

    case 1:
      freePicture (fs->topField);
      fs->topField = NULL;
      break;

    case 0:
      break;

    default:
      error ("invalid frame store type",500);
    }
  fs->isUsed = 0;
  fs->isLongTerm = 0;
  fs->isReference = 0;
  fs->isOrigReference = 0;

  // move empty framestore to end of buffer
  sFrameStore* tmp = dpb->fs[pos];

  for (unsigned i = pos; i < dpb->usedSize-1; i++)
    dpb->fs[i] = dpb->fs[i+1];

  dpb->fs[dpb->usedSize-1] = tmp;
  dpb->usedSize--;
  }
//}}}
//{{{
void freeDpb (sDPB* dpb) {

  sDecoder* decoder = dpb->decoder;
  if (dpb->fs) {
    for (unsigned i = 0; i < dpb->size; i++)
      freeFrameStore (dpb->fs[i]);
    free (dpb->fs);
    dpb->fs = NULL;
    }

  if (dpb->fsRef)
    free (dpb->fsRef);
  if (dpb->fsLongTermRef)
    free (dpb->fsLongTermRef);


  dpb->lastOutputPoc = INT_MIN;
  dpb->initDone = 0;

  // picture error conceal
  if (decoder->concealMode != 0 || decoder->lastOutFramestore)
    freeFrameStore (decoder->lastOutFramestore);

  if (decoder->noReferencePicture) {
    freePicture (decoder->noReferencePicture);
    decoder->noReferencePicture = NULL;
    }
  }
//}}}

// image
//{{{
void initImage (sDecoder* decoder, sImage* image, sSPS* sps) {

  // allocate memory for reference frame buffers: image->frm_data
  image->format = decoder->param.output;
  image->format.width[0]  = decoder->width;
  image->format.width[1]  = decoder->widthCr;
  image->format.width[2]  = decoder->widthCr;
  image->format.height[0] = decoder->height;
  image->format.height[1] = decoder->heightCr;
  image->format.height[2] = decoder->heightCr;
  image->format.yuvFormat  = (eColorFormat)sps->chromaFormatIdc;
  image->format.autoCropBot = decoder->param.output.autoCropBot;
  image->format.autoCropRight = decoder->param.output.autoCropRight;
  image->format.autoCropBotCr = decoder->param.output.autoCropBotCr;
  image->format.autoCropRightCr = decoder->param.output.autoCropRightCr;
  image->frm_stride[0] = decoder->width;
  image->frm_stride[1] = image->frm_stride[2] = decoder->widthCr;
  image->top_stride[0] = image->bot_stride[0] = image->frm_stride[0] << 1;
  image->top_stride[1] = image->top_stride[2] = image->bot_stride[1] = image->bot_stride[2] = image->frm_stride[1] << 1;

  if (sps->sepColourPlaneFlag) {
    for (int nplane = 0; nplane < MAX_PLANE; nplane++ )
      get_mem2Dpel (&(image->frm_data[nplane]), decoder->height, decoder->width);
    }
  else {
    get_mem2Dpel (&(image->frm_data[0]), decoder->height, decoder->width);
    if (decoder->yuvFormat != YUV400) {
      get_mem2Dpel (&(image->frm_data[1]), decoder->heightCr, decoder->widthCr);
      get_mem2Dpel (&(image->frm_data[2]), decoder->heightCr, decoder->widthCr);
      if (sizeof(sPixel) == sizeof(unsigned char)) {
        for (int k = 1; k < 3; k++)
          memset (image->frm_data[k][0], 128, decoder->heightCr * decoder->widthCr * sizeof(sPixel));
        }
      else {
        sPixel mean_val;
        for (int k = 1; k < 3; k++) {
          mean_val = (sPixel) ((decoder->maxPelValueComp[k] + 1) >> 1);
          for (int j = 0; j < decoder->heightCr; j++)
            for (int i = 0; i < decoder->widthCr; i++)
              image->frm_data[k][j][i] = mean_val;
          }
        }
      }
    }

  if (!decoder->activeSPS->frameMbOnlyFlag) {
    // allocate memory for field reference frame buffers
    init_top_bot_planes (image->frm_data[0], decoder->height, &(image->top_data[0]), &(image->bot_data[0]));
    if (decoder->yuvFormat != YUV400) {
      init_top_bot_planes (image->frm_data[1], decoder->heightCr, &(image->top_data[1]), &(image->bot_data[1]));
      init_top_bot_planes (image->frm_data[2], decoder->heightCr, &(image->top_data[2]), &(image->bot_data[2]));
      }
    }
  }
//}}}
//{{{
void freeImage (sDecoder* decoder, sImage* image) {

  if (decoder->sepColourPlaneFlag ) {
    for (int nplane = 0; nplane < MAX_PLANE; nplane++ ) {
      if (image->frm_data[nplane]) {
        free_mem2Dpel (image->frm_data[nplane]);      // free ref frame buffers
        image->frm_data[nplane] = NULL;
        }
      }
    }

  else {
    if (image->frm_data[0]) {
      free_mem2Dpel (image->frm_data[0]);      // free ref frame buffers
      image->frm_data[0] = NULL;
      }

    if (image->format.yuvFormat != YUV400) {
      if (image->frm_data[1]) {
        free_mem2Dpel (image->frm_data[1]);
        image->frm_data[1] = NULL;
        }
      if (image->frm_data[2]) {
        free_mem2Dpel (image->frm_data[2]);
        image->frm_data[2] = NULL;
        }
      }
    }

  if (!decoder->activeSPS->frameMbOnlyFlag) {
    free_top_bot_planes(image->top_data[0], image->bot_data[0]);
    if (image->format.yuvFormat != YUV400) {
      free_top_bot_planes (image->top_data[1], image->bot_data[1]);
      free_top_bot_planes (image->top_data[2], image->bot_data[2]);
      }
    }
  }
//}}}

// slice
//{{{
static void reorderShortTerm (sSlice* slice, int curList, int num_ref_idx_lX_active_minus1, int picNumLX, int *refIdxLX) {

  sPicture** RefPicListX = slice->listX[curList];
  sPicture* picLX = get_short_term_pic (slice, slice->dpb, picNumLX);

  for (int cIdx = num_ref_idx_lX_active_minus1+1; cIdx > *refIdxLX; cIdx--)
    RefPicListX[cIdx] = RefPicListX[cIdx - 1];
  RefPicListX[(*refIdxLX)++] = picLX;

  int nIdx = *refIdxLX;
  for (int cIdx = *refIdxLX; cIdx <= num_ref_idx_lX_active_minus1+1; cIdx++)
    if (RefPicListX[cIdx])
      if ((RefPicListX[cIdx]->isLongTerm) || (RefPicListX[cIdx]->picNum != picNumLX))
        RefPicListX[nIdx++] = RefPicListX[ cIdx ];
  }
//}}}
//{{{
static void reorderLongTerm (sSlice* slice, sPicture** RefPicListX,
                             int num_ref_idx_lX_active_minus1, int LongTermPicNum, int *refIdxLX) {

  sPicture* picLX = get_long_term_pic (slice, slice->dpb, LongTermPicNum);

  for (int cIdx = num_ref_idx_lX_active_minus1+1; cIdx > *refIdxLX; cIdx--)
    RefPicListX[cIdx] = RefPicListX[cIdx - 1];
  RefPicListX[(*refIdxLX)++] = picLX;

  int nIdx = *refIdxLX;
  for (int cIdx = *refIdxLX; cIdx <= num_ref_idx_lX_active_minus1+1; cIdx++)
    if (RefPicListX[cIdx])
      if ((!RefPicListX[cIdx]->isLongTerm ) || (RefPicListX[cIdx]->longTermPicNum != LongTermPicNum))
        RefPicListX[nIdx++] = RefPicListX[cIdx];
  }
//}}}
//{{{
void reorderRefPicList (sSlice* slice, int curList) {

  int* modification_of_pic_nums_idc = slice->modification_of_pic_nums_idc[curList];
  int* abs_diff_pic_num_minus1 = slice->abs_diff_pic_num_minus1[curList];
  int* long_term_pic_idx = slice->long_term_pic_idx[curList];
  int num_ref_idx_lX_active_minus1 = slice->numRefIndexActive[curList] - 1;

  sDecoder* decoder = slice->decoder;

  int maxPicNum, currPicNum, picNumLXNoWrap, picNumLXPred, picNumLX;
  int refIdxLX = 0;

  if (slice->structure==FRAME) {
    maxPicNum = decoder->maxFrameNum;
    currPicNum = slice->frameNum;
    }
  else {
    maxPicNum = 2 * decoder->maxFrameNum;
    currPicNum = 2 * slice->frameNum + 1;
    }

  picNumLXPred = currPicNum;

  for (int i = 0; modification_of_pic_nums_idc[i] != 3; i++) {
    if (modification_of_pic_nums_idc[i]>3)
      error ("Invalid modification_of_pic_nums_idc command", 500);

    if (modification_of_pic_nums_idc[i] < 2) {
      if (modification_of_pic_nums_idc[i] == 0) {
        if (picNumLXPred - (abs_diff_pic_num_minus1[i] + 1) < 0)
          picNumLXNoWrap = picNumLXPred - (abs_diff_pic_num_minus1[i] + 1) + maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred - (abs_diff_pic_num_minus1[i] + 1);
        }
      else {
        if( picNumLXPred + (abs_diff_pic_num_minus1[i] + 1)  >=  maxPicNum )
          picNumLXNoWrap = picNumLXPred + (abs_diff_pic_num_minus1[i] + 1) - maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred + (abs_diff_pic_num_minus1[i] + 1);
        }
      picNumLXPred = picNumLXNoWrap;

      if (picNumLXNoWrap > currPicNum)
        picNumLX = picNumLXNoWrap - maxPicNum;
      else
        picNumLX = picNumLXNoWrap;

      reorderShortTerm (slice, curList, num_ref_idx_lX_active_minus1, picNumLX, &refIdxLX);
      }
    else
      reorderLongTerm (slice, slice->listX[curList], num_ref_idx_lX_active_minus1, long_term_pic_idx[i], &refIdxLX);
    }

  // that's a definition
  slice->listXsize[curList] = (char)(num_ref_idx_lX_active_minus1 + 1);
  }
//}}}

//{{{
void updatePicNum (sSlice* slice) {

  sDecoder* decoder = slice->decoder;
  sSPS* activeSPS = decoder->activeSPS;

  int addTop = 0;
  int addBot = 0;
  int maxFrameNum = 1 << (activeSPS->log2_max_frame_num_minus4 + 4);

  sDPB* dpb = slice->dpb;
  if (slice->structure == FRAME) {
    for (unsigned int i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fsRef[i]->isUsed==3 ) {
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
    for (unsigned int i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->fsLongTermRef[i]->isUsed==3) {
        if (dpb->fsLongTermRef[i]->frame->isLongTerm)
          dpb->fsLongTermRef[i]->frame->longTermPicNum = dpb->fsLongTermRef[i]->frame->longTermFrameIndex;
        }
      }
    }
  else {
    if (slice->structure == TopField) {
      addTop = 1;
      addBot = 0;
      }
    else {
      addTop = 0;
      addBot = 1;
      }

    for (unsigned int i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fsRef[i]->isReference) {
        if( dpb->fsRef[i]->frameNum > slice->frameNum )
          dpb->fsRef[i]->frameNumWrap = dpb->fsRef[i]->frameNum - maxFrameNum;
        else
          dpb->fsRef[i]->frameNumWrap = dpb->fsRef[i]->frameNum;
        if (dpb->fsRef[i]->isReference & 1)
          dpb->fsRef[i]->topField->picNum = (2 * dpb->fsRef[i]->frameNumWrap) + addTop;
        if (dpb->fsRef[i]->isReference & 2)
          dpb->fsRef[i]->botField->picNum = (2 * dpb->fsRef[i]->frameNumWrap) + addBot;
        }
      }

    // update longTermPicNum
    for (unsigned int i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->fsLongTermRef[i]->isLongTerm & 1)
        dpb->fsLongTermRef[i]->topField->longTermPicNum = 2 * dpb->fsLongTermRef[i]->topField->longTermFrameIndex + addTop;
      if (dpb->fsLongTermRef[i]->isLongTerm & 2)
        dpb->fsLongTermRef[i]->botField->longTermPicNum = 2 * dpb->fsLongTermRef[i]->botField->longTermFrameIndex + addBot;
      }
    }
  }
//}}}
//{{{
void initListsIslice (sSlice* slice) {

  slice->listXsize[0] = 0;
  slice->listXsize[1] = 0;
  }
//}}}
//{{{
void initListsPslice (sSlice* slice) {

  sDecoder* decoder = slice->decoder;
  sDPB* dpb = slice->dpb;

  int list0idx = 0;
  int listltidx = 0;
  sFrameStore** fsList0;
  sFrameStore** fsListLongTerm;

  if (slice->structure == FRAME) {
    for (unsigned int i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed == 3)
        if ((dpb->fsRef[i]->frame->usedForReference) && (!dpb->fsRef[i]->frame->isLongTerm))
          slice->listX[0][list0idx++] = dpb->fsRef[i]->frame;

    // order list 0 by PicNum
    qsort ((void *)slice->listX[0], list0idx, sizeof(sPicture*), compare_pic_by_pic_num_desc);
    slice->listXsize[0] = (char) list0idx;

    // long term handling
    for (unsigned int i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      if (dpb->fsLongTermRef[i]->isUsed == 3)
        if (dpb->fsLongTermRef[i]->frame->isLongTerm)
          slice->listX[0][list0idx++] = dpb->fsLongTermRef[i]->frame;
    qsort ((void*)&slice->listX[0][(short) slice->listXsize[0]], list0idx - slice->listXsize[0], sizeof(sPicture*), compare_pic_by_lt_pic_num_asc);
    slice->listXsize[0] = (char) list0idx;
    }
  else {
    fsList0 = calloc (dpb->size, sizeof(sFrameStore*));
    if (!fsList0)
      no_mem_exit ("initLists: fsList0");
    fsListLongTerm = calloc (dpb->size, sizeof(sFrameStore*));
    if (!fsListLongTerm)
      no_mem_exit("initLists: fsListLongTerm");

    for (unsigned int i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isReference)
        fsList0[list0idx++] = dpb->fsRef[i];
    qsort ((void*)fsList0, list0idx, sizeof(sFrameStore*), compare_fs_by_frame_num_desc);
    slice->listXsize[0] = 0;
    genPicListFromFrameList(slice->structure, fsList0, list0idx, slice->listX[0], &slice->listXsize[0], 0);

    // long term handling
    for (unsigned int i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      fsListLongTerm[listltidx++] = dpb->fsLongTermRef[i];
    qsort ((void*)fsListLongTerm, listltidx, sizeof(sFrameStore*), compare_fs_by_lt_pic_idx_asc);
    genPicListFromFrameList (slice->structure, fsListLongTerm, listltidx, slice->listX[0], &slice->listXsize[0], 1);

    free (fsList0);
    free (fsListLongTerm);
    }
  slice->listXsize[1] = 0;

  // set max size
  slice->listXsize[0] = (char)imin (slice->listXsize[0], slice->numRefIndexActive[LIST_0]);
  slice->listXsize[1] = (char)imin (slice->listXsize[1], slice->numRefIndexActive[LIST_1]);

  // set the unused list entries to NULL
  for (unsigned int i = slice->listXsize[0]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[0][i] = decoder->noReferencePicture;
  for (unsigned int i = slice->listXsize[1]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[1][i] = decoder->noReferencePicture;
  }
//}}}
//{{{
void initListsBslice (sSlice* slice) {

  sDecoder* decoder = slice->decoder;
  sDPB* dpb = slice->dpb;

  unsigned int i;
  int j;

  int list0idx = 0;
  int list0idx_1 = 0;
  int listltidx = 0;

  sFrameStore** fsList0;
  sFrameStore** fs_list1;
  sFrameStore** fsListLongTerm;

  // B Slice
  if (slice->structure == FRAME) {
    //{{{  frame
    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed==3)
        if ((dpb->fsRef[i]->frame->usedForReference)&&(!dpb->fsRef[i]->frame->isLongTerm))
          if (slice->framePoc >= dpb->fsRef[i]->frame->poc) //!KS use >= for error conceal
            slice->listX[0][list0idx++] = dpb->fsRef[i]->frame;
    qsort ((void *)slice->listX[0], list0idx, sizeof(sPicture*), compare_pic_by_poc_desc);

    // get the backward reference picture (POC>current POC) in list0;
    list0idx_1 = list0idx;
    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed==3)
        if ((dpb->fsRef[i]->frame->usedForReference)&&(!dpb->fsRef[i]->frame->isLongTerm))
          if (slice->framePoc < dpb->fsRef[i]->frame->poc)
            slice->listX[0][list0idx++] = dpb->fsRef[i]->frame;
    qsort ((void*)&slice->listX[0][list0idx_1], list0idx-list0idx_1, sizeof(sPicture*), compare_pic_by_poc_asc);

    for (j = 0; j < list0idx_1; j++)
      slice->listX[1][list0idx-list0idx_1+j]=slice->listX[0][j];
    for (j = list0idx_1; j < list0idx; j++)
      slice->listX[1][j-list0idx_1] = slice->listX[0][j];
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
    qsort ((void *)&slice->listX[0][(short) slice->listXsize[0]], list0idx - slice->listXsize[0], sizeof(sPicture*), compare_pic_by_lt_pic_num_asc);
    qsort ((void *)&slice->listX[1][(short) slice->listXsize[0]], list0idx - slice->listXsize[0], sizeof(sPicture*), compare_pic_by_lt_pic_num_asc);
    slice->listXsize[0] = slice->listXsize[1] = (char)list0idx;
    }
    //}}}
  else {
    //{{{  field
    fsList0 = calloc(dpb->size, sizeof (sFrameStore*));
    fs_list1 = calloc(dpb->size, sizeof (sFrameStore*));
    fsListLongTerm = calloc(dpb->size, sizeof (sFrameStore*));
    slice->listXsize[0] = 0;
    slice->listXsize[1] = 1;

    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed)
        if (slice->thisPoc >= dpb->fsRef[i]->poc)
          fsList0[list0idx++] = dpb->fsRef[i];
    qsort ((void *)fsList0, list0idx, sizeof(sFrameStore*), compare_fs_by_poc_desc);

    list0idx_1 = list0idx;
    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fsRef[i]->isUsed)
        if (slice->thisPoc < dpb->fsRef[i]->poc)
          fsList0[list0idx++] = dpb->fsRef[i];
    qsort ((void*)&fsList0[list0idx_1], list0idx-list0idx_1, sizeof(sFrameStore*), compare_fs_by_poc_asc);

    for (j = 0; j < list0idx_1; j++)
      fs_list1[list0idx-list0idx_1+j]=fsList0[j];
    for (j = list0idx_1; j < list0idx; j++)
      fs_list1[j-list0idx_1]=fsList0[j];

    slice->listXsize[0] = 0;
    slice->listXsize[1] = 0;
    genPicListFromFrameList (slice->structure, fsList0, list0idx, slice->listX[0], &slice->listXsize[0], 0);
    genPicListFromFrameList (slice->structure, fs_list1, list0idx, slice->listX[1], &slice->listXsize[1], 0);

    // long term handling
    for (i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      fsListLongTerm[listltidx++] = dpb->fsLongTermRef[i];

    qsort ((void*)fsListLongTerm, listltidx, sizeof(sFrameStore*), compare_fs_by_lt_pic_idx_asc);
    genPicListFromFrameList (slice->structure, fsListLongTerm, listltidx, slice->listX[0], &slice->listXsize[0], 1);
    genPicListFromFrameList (slice->structure, fsListLongTerm, listltidx, slice->listX[1], &slice->listXsize[1], 1);

    free (fsList0);
    free (fs_list1);
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
void init_mbaff_lists (sDecoder* decoder, sSlice* slice) {
// Initialize listX[2..5] from lists 0 and 1
//   listX[2]: list0 for current_field==top
//   listX[3]: list1 for current_field==top
//   listX[4]: list0 for current_field==bottom
//   listX[5]: list1 for current_field==bottom

  for (int i = 2; i < 6; i++) {
    for (unsigned j = 0; j < MAX_LIST_SIZE; j++)
      slice->listX[i][j] = decoder->noReferencePicture;
    slice->listXsize[i]=0;
    }

  for (int i = 0; i < slice->listXsize[0]; i++) {
    slice->listX[2][2*i  ] = slice->listX[0][i]->topField;
    slice->listX[2][2*i+1] = slice->listX[0][i]->botField;
    slice->listX[4][2*i  ] = slice->listX[0][i]->botField;
    slice->listX[4][2*i+1] = slice->listX[0][i]->topField;
    }
  slice->listXsize[2] = slice->listXsize[4] = slice->listXsize[0] * 2;

  for (int i = 0; i < slice->listXsize[1]; i++) {
    slice->listX[3][2*i  ] = slice->listX[1][i]->topField;
    slice->listX[3][2*i+1] = slice->listX[1][i]->botField;
    slice->listX[5][2*i  ] = slice->listX[1][i]->botField;
    slice->listX[5][2*i+1] = slice->listX[1][i]->topField;
    }
  slice->listXsize[3] = slice->listXsize[5] = slice->listXsize[1] * 2;
  }
//}}}
//{{{
sPicture* get_short_term_pic (sSlice* slice, sDPB* dpb, int picNum) {

  for (unsigned i = 0; i < dpb->refFramesInBuffer; i++) {
    if (slice->structure == FRAME) {
      if (dpb->fsRef[i]->isReference == 3)
        if ((!dpb->fsRef[i]->frame->isLongTerm)&&(dpb->fsRef[i]->frame->picNum == picNum))
          return dpb->fsRef[i]->frame;
      }
    else {
      if (dpb->fsRef[i]->isReference & 1)
        if ((!dpb->fsRef[i]->topField->isLongTerm)&&(dpb->fsRef[i]->topField->picNum == picNum))
          return dpb->fsRef[i]->topField;
      if (dpb->fsRef[i]->isReference & 2)
        if ((!dpb->fsRef[i]->botField->isLongTerm)&&(dpb->fsRef[i]->botField->picNum == picNum))
          return dpb->fsRef[i]->botField;
      }
    }

  return slice->decoder->noReferencePicture;
  }
//}}}

//{{{
void alloc_ref_pic_list_reordering_buffer (sSlice* slice) {

  if (slice->sliceType != I_SLICE && slice->sliceType != SI_SLICE) {
    int size = slice->numRefIndexActive[LIST_0] + 1;
    if ((slice->modification_of_pic_nums_idc[LIST_0] = calloc (size ,sizeof(int))) == NULL)
       no_mem_exit ("alloc_ref_pic_list_reordering_buffer: modification_of_pic_nums_idc_l0");
    if ((slice->abs_diff_pic_num_minus1[LIST_0] = calloc (size,sizeof(int))) == NULL)
       no_mem_exit ("alloc_ref_pic_list_reordering_buffer: abs_diff_pic_num_minus1_l0");
    if ((slice->long_term_pic_idx[LIST_0] = calloc (size,sizeof(int))) == NULL)
       no_mem_exit ("alloc_ref_pic_list_reordering_buffer: long_term_pic_idx_l0");
    }
  else {
    slice->modification_of_pic_nums_idc[LIST_0] = NULL;
    slice->abs_diff_pic_num_minus1[LIST_0] = NULL;
    slice->long_term_pic_idx[LIST_0] = NULL;
    }

  if (slice->sliceType == B_SLICE) {
    int size = slice->numRefIndexActive[LIST_1] + 1;
    if ((slice->modification_of_pic_nums_idc[LIST_1] = calloc (size,sizeof(int))) == NULL)
      no_mem_exit ("alloc_ref_pic_list_reordering_buffer: modification_of_pic_nums_idc_l1");
    if ((slice->abs_diff_pic_num_minus1[LIST_1] = calloc (size,sizeof(int))) == NULL)
      no_mem_exit ("alloc_ref_pic_list_reordering_buffer: abs_diff_pic_num_minus1_l1");
    if ((slice->long_term_pic_idx[LIST_1] = calloc (size,sizeof(int))) == NULL)
      no_mem_exit ("alloc_ref_pic_list_reordering_buffer: long_term_pic_idx_l1");
    }
  else {
    slice->modification_of_pic_nums_idc[LIST_1] = NULL;
    slice->abs_diff_pic_num_minus1[LIST_1] = NULL;
    slice->long_term_pic_idx[LIST_1] = NULL;
    }
  }
//}}}
//{{{
void freeRefPicListReorderingBuffer (sSlice* slice) {

  if (slice->modification_of_pic_nums_idc[LIST_0])
    free(slice->modification_of_pic_nums_idc[LIST_0]);
  if (slice->abs_diff_pic_num_minus1[LIST_0])
    free(slice->abs_diff_pic_num_minus1[LIST_0]);
  if (slice->long_term_pic_idx[LIST_0])
    free(slice->long_term_pic_idx[LIST_0]);

  slice->modification_of_pic_nums_idc[LIST_0] = NULL;
  slice->abs_diff_pic_num_minus1[LIST_0] = NULL;
  slice->long_term_pic_idx[LIST_0] = NULL;

  if (slice->modification_of_pic_nums_idc[LIST_1])
    free(slice->modification_of_pic_nums_idc[LIST_1]);
  if (slice->abs_diff_pic_num_minus1[LIST_1])
    free(slice->abs_diff_pic_num_minus1[LIST_1]);
  if (slice->long_term_pic_idx[LIST_1])
    free(slice->long_term_pic_idx[LIST_1]);

  slice->modification_of_pic_nums_idc[LIST_1] = NULL;
  slice->abs_diff_pic_num_minus1[LIST_1] = NULL;
  slice->long_term_pic_idx[LIST_1] = NULL;
  }
//}}}
//{{{
void computeColocated (sSlice* slice, sPicture** listX[6]) {

  sDecoder* decoder = slice->decoder;
  if (slice->directSpatialMvPredFlag == 0) {
    for (int j = 0; j < 2 + (slice->mbAffFrameFlag * 4); j += 2) {
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
