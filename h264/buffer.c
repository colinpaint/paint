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
    if (frameStore->top_field)
      if (frameStore->top_field->usedForReference)
        return 1;

  if (frameStore->isUsed & 2) // bottom field
    if (frameStore->bottom_field)
      if (frameStore->bottom_field->usedForReference)
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
    if (frameStore->top_field)
      if ((frameStore->top_field->usedForReference) && (!frameStore->top_field->isLongTerm))
        return 1;

  if (frameStore->isUsed & 2) // bottom field
    if (frameStore->bottom_field)
      if ((frameStore->bottom_field->usedForReference) && (!frameStore->bottom_field->isLongTerm))
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
    if (frameStore->top_field)
      if ((frameStore->top_field->usedForReference) && (frameStore->top_field->isLongTerm))
        return 1;

  if (frameStore->isUsed & 2) // bottom field
    if (frameStore->bottom_field)
      if ((frameStore->bottom_field->usedForReference) && (frameStore->bottom_field->isLongTerm))
        return 1;

  return 0;
  }
//}}}
//{{{
static void gen_field_ref_ids (sVidParam* vidParam, sPicture* p) {
// Generate Frame parameters from field information.

  // copy the list;
  for (int j = 0; j < vidParam->curPicSliceNum; j++) {
    if (p->listX[j][LIST_0]) {
      p->listXsize[j][LIST_0] = vidParam->sliceList[j]->listXsize[LIST_0];
      for (int i = 0; i < p->listXsize[j][LIST_0]; i++)
        p->listX[j][LIST_0][i] = vidParam->sliceList[j]->listX[LIST_0][i];
      }

    if(p->listX[j][LIST_1]) {
      p->listXsize[j][LIST_1] = vidParam->sliceList[j]->listXsize[LIST_1];
      for(int i = 0; i < p->listXsize[j][LIST_1]; i++)
        p->listX[j][LIST_1][i] = vidParam->sliceList[j]->listX[LIST_1][i];
      }
    }
  }
//}}}
//{{{
static void unmark_long_term_field_for_reference_by_frame_idx (

  sDPB* dpb, ePicStructure structure,
  int longTermFrameIndex, int mark_current, unsigned curr_frame_num, int curr_pic_num) {

  sVidParam* vidParam = dpb->vidParam;

  assert (structure!=FRAME);
  if (curr_pic_num < 0)
    curr_pic_num += (2 * vidParam->max_frame_num);

  for (unsigned i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
    if (dpb->fs_ltref[i]->longTermFrameIndex == longTermFrameIndex) {
      if (structure == TOP_FIELD) {
        if (dpb->fs_ltref[i]->isLongTerm == 3)
          unmark_for_long_term_reference(dpb->fs_ltref[i]);
        else {
          if (dpb->fs_ltref[i]->isLongTerm == 1)
            unmark_for_long_term_reference(dpb->fs_ltref[i]);
          else {
            if (mark_current) {
              if (dpb->lastPicture) {
                if ((dpb->lastPicture != dpb->fs_ltref[i]) ||
                    dpb->lastPicture->frame_num != curr_frame_num)
                  unmark_for_long_term_reference(dpb->fs_ltref[i]);
                }
              else
                unmark_for_long_term_reference(dpb->fs_ltref[i]);
            }
            else {
              if ((dpb->fs_ltref[i]->frame_num) != (unsigned)(curr_pic_num >> 1))
                unmark_for_long_term_reference(dpb->fs_ltref[i]);
              }
            }
          }
        }

      if (structure == BOTTOM_FIELD) {
        if (dpb->fs_ltref[i]->isLongTerm == 3)
          unmark_for_long_term_reference(dpb->fs_ltref[i]);
        else {
          if (dpb->fs_ltref[i]->isLongTerm == 2)
            unmark_for_long_term_reference(dpb->fs_ltref[i]);
          else {
            if (mark_current) {
              if (dpb->lastPicture) {
                if ((dpb->lastPicture != dpb->fs_ltref[i]) ||
                    dpb->lastPicture->frame_num != curr_frame_num)
                  unmark_for_long_term_reference(dpb->fs_ltref[i]);
                }
              else
                unmark_for_long_term_reference(dpb->fs_ltref[i]);
              }
            else {
              if ((dpb->fs_ltref[i]->frame_num) != (unsigned)(curr_pic_num >> 1))
                unmark_for_long_term_reference(dpb->fs_ltref[i]);
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
    currPicNum = p->frame_num;
  else
    currPicNum = 2 * p->frame_num + 1;

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
  //printf ("output frame with frame_num #%d, poc %d (dpb. dpb->size=%d, dpb->usedSize=%d)\n", dpb->fs[pos]->frame_num, dpb->fs[pos]->frame->poc, dpb->size, dpb->usedSize);

  // picture error conceal
  sVidParam* vidParam = dpb->vidParam;
  if (vidParam->concealMode != 0) {
    if (dpb->lastOutputPoc == 0)
      write_lost_ref_after_idr (dpb, pos);
    write_lost_non_ref_pic (dpb, poc);
    }

  writeStoredFrame (vidParam, dpb->fs[pos]);

  // picture error conceal
  if(vidParam->concealMode == 0)
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
void allocPicMotion (sPicMotionParamsOld* motion, int size_y, int size_x) {

  motion->mb_field = calloc (size_y * size_x, sizeof(byte));
  if (motion->mb_field == NULL)
    no_mem_exit ("allocPicture: motion->mb_field");
  }
//}}}
//{{{
void freePicMotion (sPicMotionParamsOld* motion) {

  if (motion->mb_field) {
    free (motion->mb_field);
    motion->mb_field = NULL;
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
  fs->top_field = NULL;
  fs->bottom_field = NULL;

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

    if (frameStore->top_field) {
      freePicture (frameStore->top_field);
      frameStore->top_field = NULL;
      }

    if (frameStore->bottom_field) {
      freePicture (frameStore->bottom_field);
      frameStore->bottom_field = NULL;
      }

    free (frameStore);
    }
  }
//}}}

//{{{
static void dpbSplitField (sVidParam* vidParam, sFrameStore* frameStore) {

  int twosz16 = 2 * (frameStore->frame->size_x >> 4);
  sPicture* fsTop = NULL;
  sPicture* fsBot = NULL;
  sPicture* frame = frameStore->frame;

  frameStore->poc = frame->poc;
  if (!frame->frame_mbs_only_flag) {
    fsTop = frameStore->top_field = allocPicture (vidParam, TOP_FIELD,
                                                   frame->size_x, frame->size_y, frame->size_x_cr,
                                                   frame->size_y_cr, 1);
    fsBot = frameStore->bottom_field = allocPicture (vidParam, BOTTOM_FIELD,
                                                      frame->size_x, frame->size_y,
                                                      frame->size_x_cr, frame->size_y_cr, 1);

    for (int i = 0; i < (frame->size_y >> 1); i++)
      memcpy (fsTop->imgY[i], frame->imgY[i*2], frame->size_x*sizeof(sPixel));

    for (int i = 0; i< (frame->size_y_cr >> 1); i++) {
      memcpy (fsTop->imgUV[0][i], frame->imgUV[0][i*2], frame->size_x_cr*sizeof(sPixel));
      memcpy (fsTop->imgUV[1][i], frame->imgUV[1][i*2], frame->size_x_cr*sizeof(sPixel));
      }

    for (int i = 0; i < (frame->size_y>>1); i++)
      memcpy (fsBot->imgY[i], frame->imgY[i*2 + 1], frame->size_x*sizeof(sPixel));

    for (int i = 0; i < (frame->size_y_cr>>1); i++) {
      memcpy (fsBot->imgUV[0][i], frame->imgUV[0][i*2 + 1], frame->size_x_cr*sizeof(sPixel));
      memcpy (fsBot->imgUV[1][i], frame->imgUV[1][i*2 + 1], frame->size_x_cr*sizeof(sPixel));
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

    fsTop->coded_frame = fsBot->coded_frame = 1;
    fsTop->mb_aff_frame_flag = fsBot->mb_aff_frame_flag = frame->mb_aff_frame_flag;

    frame->top_field = fsTop;
    frame->bottom_field = fsBot;
    frame->frame = frame;
    fsTop->bottom_field = fsBot;
    fsTop->frame = frame;
    fsTop->top_field = fsTop;
    fsBot->top_field = fsTop;
    fsBot->frame = frame;
    fsBot->bottom_field = fsBot;

    fsTop->chromaFormatIdc = fsBot->chromaFormatIdc = frame->chromaFormatIdc;
    fsTop->iCodingType = fsBot->iCodingType = frame->iCodingType;
    if (frame->usedForReference)  {
      padPicture (vidParam, fsTop);
      padPicture (vidParam, fsBot);
      }
    }
  else {
    frameStore->top_field = NULL;
    frameStore->bottom_field = NULL;
    frame->top_field = NULL;
    frame->bottom_field = NULL;
    frame->frame = frame;
    }

  if (!frame->frame_mbs_only_flag) {
    if (frame->mb_aff_frame_flag) {
      sPicMotionParamsOld* frm_motion = &frame->motion;
      for (int j = 0 ; j < (frame->size_y >> 3); j++) {
        int jj = (j >> 2)*8 + (j & 0x03);
        int jj4 = jj + 4;
        int jdiv = (j >> 1);
        for (int i = 0 ; i < (frame->size_x>>2); i++) {
          int idiv = (i >> 2);

          int currentmb = twosz16*(jdiv >> 1)+ (idiv)*2 + (jdiv & 0x01);
          // Assign field mvs attached to MB-Frame buffer to the proper buffer
          if (frm_motion->mb_field[currentmb]) {
            fsBot->mvInfo[j][i].mv[LIST_0] = frame->mvInfo[jj4][i].mv[LIST_0];
            fsBot->mvInfo[j][i].mv[LIST_1] = frame->mvInfo[jj4][i].mv[LIST_1];
            fsBot->mvInfo[j][i].refIndex[LIST_0] = frame->mvInfo[jj4][i].refIndex[LIST_0];
            if (fsBot->mvInfo[j][i].refIndex[LIST_0] >=0)
              fsBot->mvInfo[j][i].refPic[LIST_0] = vidParam->sliceList[frame->mvInfo[jj4][i].slice_no]->listX[4][(short) fsBot->mvInfo[j][i].refIndex[LIST_0]];
            else
              fsBot->mvInfo[j][i].refPic[LIST_0] = NULL;
            fsBot->mvInfo[j][i].refIndex[LIST_1] = frame->mvInfo[jj4][i].refIndex[LIST_1];
            if (fsBot->mvInfo[j][i].refIndex[LIST_1] >=0)
              fsBot->mvInfo[j][i].refPic[LIST_1] = vidParam->sliceList[frame->mvInfo[jj4][i].slice_no]->listX[5][(short) fsBot->mvInfo[j][i].refIndex[LIST_1]];
            else
              fsBot->mvInfo[j][i].refPic[LIST_1] = NULL;

            fsTop->mvInfo[j][i].mv[LIST_0] = frame->mvInfo[jj][i].mv[LIST_0];
            fsTop->mvInfo[j][i].mv[LIST_1] = frame->mvInfo[jj][i].mv[LIST_1];
            fsTop->mvInfo[j][i].refIndex[LIST_0] = frame->mvInfo[jj][i].refIndex[LIST_0];
            if (fsTop->mvInfo[j][i].refIndex[LIST_0] >=0)
              fsTop->mvInfo[j][i].refPic[LIST_0] = vidParam->sliceList[frame->mvInfo[jj][i].slice_no]->listX[2][(short) fsTop->mvInfo[j][i].refIndex[LIST_0]];
            else
              fsTop->mvInfo[j][i].refPic[LIST_0] = NULL;
            fsTop->mvInfo[j][i].refIndex[LIST_1] = frame->mvInfo[jj][i].refIndex[LIST_1];
            if (fsTop->mvInfo[j][i].refIndex[LIST_1] >=0)
              fsTop->mvInfo[j][i].refPic[LIST_1] = vidParam->sliceList[frame->mvInfo[jj][i].slice_no]->listX[3][(short) fsTop->mvInfo[j][i].refIndex[LIST_1]];
            else
              fsTop->mvInfo[j][i].refPic[LIST_1] = NULL;
            }
          }
        }
      }

      //! Generate field MVs from Frame MVs
    for (int j = 0 ; j < (frame->size_y >> 3) ; j++) {
      int jj = 2 * RSD(j);
      int jdiv = (j >> 1);
      for (int i = 0 ; i < (frame->size_x >> 2) ; i++) {
        int ii = RSD(i);
        int idiv = (i >> 2);
        int currentmb = twosz16 * (jdiv >> 1)+ (idiv)*2 + (jdiv & 0x01);
        if (!frame->mb_aff_frame_flag  || !frame->motion.mb_field[currentmb]) {
          fsTop->mvInfo[j][i].mv[LIST_0] = fsBot->mvInfo[j][i].mv[LIST_0] = frame->mvInfo[jj][ii].mv[LIST_0];
          fsTop->mvInfo[j][i].mv[LIST_1] = fsBot->mvInfo[j][i].mv[LIST_1] = frame->mvInfo[jj][ii].mv[LIST_1];

          // Scaling of references is done here since it will not affect spatial direct (2*0 =0)
          if (frame->mvInfo[jj][ii].refIndex[LIST_0] == -1) {
            fsTop->mvInfo[j][i].refIndex[LIST_0] = fsBot->mvInfo[j][i].refIndex[LIST_0] = - 1;
            fsTop->mvInfo[j][i].refPic[LIST_0] = fsBot->mvInfo[j][i].refPic[LIST_0] = NULL;
            }
          else {
            fsTop->mvInfo[j][i].refIndex[LIST_0] = fsBot->mvInfo[j][i].refIndex[LIST_0] = frame->mvInfo[jj][ii].refIndex[LIST_0];
            fsTop->mvInfo[j][i].refPic[LIST_0] = fsBot->mvInfo[j][i].refPic[LIST_0] = vidParam->sliceList[frame->mvInfo[jj][ii].slice_no]->listX[LIST_0][(short) frame->mvInfo[jj][ii].refIndex[LIST_0]];
            }

          if (frame->mvInfo[jj][ii].refIndex[LIST_1] == -1) {
            fsTop->mvInfo[j][i].refIndex[LIST_1] = fsBot->mvInfo[j][i].refIndex[LIST_1] = - 1;
            fsTop->mvInfo[j][i].refPic[LIST_1] = fsBot->mvInfo[j][i].refPic[LIST_1] = NULL;
            }
          else {
            fsTop->mvInfo[j][i].refIndex[LIST_1] = fsBot->mvInfo[j][i].refIndex[LIST_1] = frame->mvInfo[jj][ii].refIndex[LIST_1];
            fsTop->mvInfo[j][i].refPic[LIST_1] = fsBot->mvInfo[j][i].refPic[LIST_1] = vidParam->sliceList[frame->mvInfo[jj][ii].slice_no]->listX[LIST_1][(short) frame->mvInfo[jj][ii].refIndex[LIST_1]];
            }
          }
        }
      }
    }
  }
//}}}
//{{{
static void dpb_combine_field (sVidParam* vidParam, sFrameStore* frameStore) {

  dpb_combine_field_yuv (vidParam, frameStore);

  frameStore->frame->iCodingType = frameStore->top_field->iCodingType; //FIELD_CODING;
   //! Use inference flag to remap mvs/references

  //! Generate Frame parameters from field information.
  for (int j = 0 ; j < (frameStore->top_field->size_y >> 2); j++) {
    int jj = (j << 1);
    int jj4 = jj + 1;
    for (int i = 0 ; i < (frameStore->top_field->size_x >> 2); i++) {
      frameStore->frame->mvInfo[jj][i].mv[LIST_0] = frameStore->top_field->mvInfo[j][i].mv[LIST_0];
      frameStore->frame->mvInfo[jj][i].mv[LIST_1] = frameStore->top_field->mvInfo[j][i].mv[LIST_1];

      frameStore->frame->mvInfo[jj][i].refIndex[LIST_0] = frameStore->top_field->mvInfo[j][i].refIndex[LIST_0];
      frameStore->frame->mvInfo[jj][i].refIndex[LIST_1] = frameStore->top_field->mvInfo[j][i].refIndex[LIST_1];

      /* bug: top field list doesnot exist.*/
      int l = frameStore->top_field->mvInfo[j][i].slice_no;
      int k = frameStore->top_field->mvInfo[j][i].refIndex[LIST_0];
      frameStore->frame->mvInfo[jj][i].refPic[LIST_0] = k>=0? frameStore->top_field->listX[l][LIST_0][k]: NULL;
      k = frameStore->top_field->mvInfo[j][i].refIndex[LIST_1];
      frameStore->frame->mvInfo[jj][i].refPic[LIST_1] = k>=0? frameStore->top_field->listX[l][LIST_1][k]: NULL;

      //! association with id already known for fields.
      frameStore->frame->mvInfo[jj4][i].mv[LIST_0] = frameStore->bottom_field->mvInfo[j][i].mv[LIST_0];
      frameStore->frame->mvInfo[jj4][i].mv[LIST_1] = frameStore->bottom_field->mvInfo[j][i].mv[LIST_1];

      frameStore->frame->mvInfo[jj4][i].refIndex[LIST_0] = frameStore->bottom_field->mvInfo[j][i].refIndex[LIST_0];
      frameStore->frame->mvInfo[jj4][i].refIndex[LIST_1] = frameStore->bottom_field->mvInfo[j][i].refIndex[LIST_1];
      l = frameStore->bottom_field->mvInfo[j][i].slice_no;

      k = frameStore->bottom_field->mvInfo[j][i].refIndex[LIST_0];
      frameStore->frame->mvInfo[jj4][i].refPic[LIST_0] = k >= 0 ? frameStore->bottom_field->listX[l][LIST_0][k]: NULL;
      k = frameStore->bottom_field->mvInfo[j][i].refIndex[LIST_1];
      frameStore->frame->mvInfo[jj4][i].refPic[LIST_1] = k >= 0 ? frameStore->bottom_field->listX[l][LIST_1][k]: NULL;
    }
  }
}
//}}}
//{{{
static void insertPictureDpb (sVidParam* vidParam, sFrameStore* fs, sPicture* p) {

  sInputParam* inputParam = vidParam->inputParam;
  //  printf ("insert (%s) pic with frame_num #%d, poc %d\n",
  //          (p->structure == FRAME)?"FRAME":(p->structure == TOP_FIELD)?"TOP_FIELD":"BOTTOM_FIELD",
  //          p->pic_num, p->poc);
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
      dpbSplitField (vidParam, fs);
      break;
    //}}}
    //{{{
    case TOP_FIELD:
      fs->top_field = p;
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
        dpb_combine_field(vidParam, fs);
      else
        fs->poc = p->poc;

      gen_field_ref_ids(vidParam, p);
      break;
    //}}}
    //{{{
    case BOTTOM_FIELD:
      fs->bottom_field = p;
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
        dpb_combine_field(vidParam, fs);
      else
        fs->poc = p->poc;

      gen_field_ref_ids(vidParam, p);
      break;
    //}}}
    }

  fs->frame_num = p->pic_num;
  fs->recovery_frame = p->recovery_frame;
  fs->is_output = p->is_output;
  if (fs->isUsed == 3)
    calcFrameNum (vidParam, p);
}
//}}}
//{{{
void unmark_for_reference (sFrameStore* frameStore) {

  if (frameStore->isUsed & 1)
    if (frameStore->top_field)
      frameStore->top_field->usedForReference = 0;

  if (frameStore->isUsed & 2)
    if (frameStore->bottom_field)
      frameStore->bottom_field->usedForReference = 0;

  if (frameStore->isUsed == 3) {
    if (frameStore->top_field && frameStore->bottom_field) {
      frameStore->top_field->usedForReference = 0;
      frameStore->bottom_field->usedForReference = 0;
      }
    frameStore->frame->usedForReference = 0;
    }

  frameStore->isReference = 0;

  if(frameStore->frame)
    freePicMotion (&frameStore->frame->motion);

  if (frameStore->top_field)
    freePicMotion (&frameStore->top_field->motion);

  if (frameStore->bottom_field)
    freePicMotion (&frameStore->bottom_field->motion);
  }
//}}}
//{{{
void unmark_for_long_term_reference (sFrameStore* frameStore) {

  if (frameStore->isUsed & 1) {
    if (frameStore->top_field) {
      frameStore->top_field->usedForReference = 0;
      frameStore->top_field->isLongTerm = 0;
      }
    }

  if (frameStore->isUsed & 2) {
    if (frameStore->bottom_field) {
      frameStore->bottom_field->usedForReference = 0;
      frameStore->bottom_field->isLongTerm = 0;
      }
    }

  if (frameStore->isUsed == 3) {
    if (frameStore->top_field && frameStore->bottom_field) {
      frameStore->top_field->usedForReference = 0;
      frameStore->top_field->isLongTerm = 0;
      frameStore->bottom_field->usedForReference = 0;
      frameStore->bottom_field->isLongTerm = 0;
      }
    frameStore->frame->usedForReference = 0;
    frameStore->frame->isLongTerm = 0;
    }

  frameStore->isReference = 0;
  frameStore->isLongTerm = 0;
  }
//}}}
//{{{
void dpb_combine_field_yuv (sVidParam* vidParam, sFrameStore* frameStore) {

  if (!frameStore->frame)
    frameStore->frame = allocPicture (vidParam, FRAME,
                                      frameStore->top_field->size_x, frameStore->top_field->size_y*2,
                                      frameStore->top_field->size_x_cr, frameStore->top_field->size_y_cr*2, 1);

  for (int i = 0; i < frameStore->top_field->size_y; i++) {
    memcpy (frameStore->frame->imgY[i*2], frameStore->top_field->imgY[i]   ,
            frameStore->top_field->size_x * sizeof(sPixel));     // top field
    memcpy (frameStore->frame->imgY[i*2 + 1], frameStore->bottom_field->imgY[i],
            frameStore->bottom_field->size_x * sizeof(sPixel)); // bottom field
    }

  for (int j = 0; j < 2; j++)
    for (int i = 0; i < frameStore->top_field->size_y_cr; i++) {
      memcpy (frameStore->frame->imgUV[j][i*2], frameStore->top_field->imgUV[j][i],
              frameStore->top_field->size_x_cr*sizeof(sPixel));
      memcpy (frameStore->frame->imgUV[j][i*2 + 1], frameStore->bottom_field->imgUV[j][i],
              frameStore->bottom_field->size_x_cr*sizeof(sPixel));
      }

  frameStore->poc = frameStore->frame->poc = frameStore->frame->framePoc
                                           = imin (frameStore->top_field->poc, frameStore->bottom_field->poc);

  frameStore->bottom_field->framePoc = frameStore->top_field->framePoc = frameStore->frame->poc;
  frameStore->bottom_field->topPoc = frameStore->frame->topPoc = frameStore->top_field->poc;
  frameStore->top_field->botPoc = frameStore->frame->botPoc = frameStore->bottom_field->poc;

  frameStore->frame->usedForReference = (frameStore->top_field->usedForReference && frameStore->bottom_field->usedForReference );
  frameStore->frame->isLongTerm = (frameStore->top_field->isLongTerm && frameStore->bottom_field->isLongTerm );

  if (frameStore->frame->isLongTerm)
    frameStore->frame->longTermFrameIndex = frameStore->longTermFrameIndex;

  frameStore->frame->top_field = frameStore->top_field;
  frameStore->frame->bottom_field = frameStore->bottom_field;
  frameStore->frame->frame = frameStore->frame;
  frameStore->frame->coded_frame = 0;

  frameStore->frame->chromaFormatIdc = frameStore->top_field->chromaFormatIdc;
  frameStore->frame->frame_cropping_flag = frameStore->top_field->frame_cropping_flag;
  if (frameStore->frame->frame_cropping_flag) {
    frameStore->frame->frame_crop_top_offset = frameStore->top_field->frame_crop_top_offset;
    frameStore->frame->frame_crop_bottom_offset = frameStore->top_field->frame_crop_bottom_offset;
    frameStore->frame->frame_crop_left_offset = frameStore->top_field->frame_crop_left_offset;
    frameStore->frame->frame_crop_right_offset = frameStore->top_field->frame_crop_right_offset;
    }

  frameStore->top_field->frame = frameStore->bottom_field->frame = frameStore->frame;
  frameStore->top_field->top_field = frameStore->top_field;
  frameStore->top_field->bottom_field = frameStore->bottom_field;
  frameStore->bottom_field->top_field = frameStore->top_field;
  frameStore->bottom_field->bottom_field = frameStore->bottom_field;
  if (frameStore->top_field->usedForReference || frameStore->bottom_field->usedForReference)
    padPicture (vidParam, frameStore->frame);
  }
//}}}

// picture
//{{{
sPicture* allocPicture (sVidParam* vidParam, ePicStructure structure,
                        int size_x, int size_y, int size_x_cr, int size_y_cr, int is_output) {

  sSPS* activeSPS = vidParam->activeSPS;
  //printf ("Allocating (%s) picture (x=%d, y=%d, x_cr=%d, y_cr=%d)\n",
  //        (type == FRAME)?"FRAME":(type == TOP_FIELD)?"TOP_FIELD":"BOTTOM_FIELD",
  //        size_x, size_y, size_x_cr, size_y_cr);

  sPicture* s = calloc (1, sizeof(sPicture));
  if (!s)
    no_mem_exit ("allocPicture failed");

  if (structure != FRAME) {
    size_y /= 2;
    size_y_cr /= 2;
    }

  s->picSizeInMbs = (size_x*size_y)/256;
  s->imgUV = NULL;

  get_mem2Dpel_pad (&(s->imgY), size_y, size_x, vidParam->iLumaPadY, vidParam->iLumaPadX);
  s->iLumaStride = size_x + 2 * vidParam->iLumaPadX;
  s->iLumaExpandedHeight = size_y + 2 * vidParam->iLumaPadY;

  if (activeSPS->chromaFormatIdc != YUV400)
    get_mem3Dpel_pad (&(s->imgUV), 2, size_y_cr, size_x_cr, vidParam->iChromaPadY, vidParam->iChromaPadX);

  s->iChromaStride = size_x_cr + 2*vidParam->iChromaPadX;
  s->iChromaExpandedHeight = size_y_cr + 2*vidParam->iChromaPadY;
  s->iLumaPadY = vidParam->iLumaPadY;
  s->iLumaPadX = vidParam->iLumaPadX;
  s->iChromaPadY = vidParam->iChromaPadY;
  s->iChromaPadX = vidParam->iChromaPadX;
  s->separate_colour_plane_flag = vidParam->separate_colour_plane_flag;

  get_mem2Dmp (&s->mvInfo, (size_y >> BLOCK_SHIFT), (size_x >> BLOCK_SHIFT));
  allocPicMotion (&s->motion , (size_y >> BLOCK_SHIFT), (size_x >> BLOCK_SHIFT));

  if (vidParam->separate_colour_plane_flag != 0)
    for (int nplane = 0; nplane < MAX_PLANE; nplane++) {
      get_mem2Dmp (&s->JVmv_info[nplane], (size_y >> BLOCK_SHIFT), (size_x >> BLOCK_SHIFT));
      allocPicMotion (&s->JVmotion[nplane] , (size_y >> BLOCK_SHIFT), (size_x >> BLOCK_SHIFT));
      }

  s->pic_num = 0;
  s->frame_num = 0;
  s->longTermFrameIndex = 0;
  s->long_term_pic_num = 0;
  s->usedForReference  = 0;
  s->isLongTerm = 0;
  s->non_existing = 0;
  s->is_output = 0;
  s->max_slice_id = 0;

  s->structure = structure;

  s->size_x = size_x;
  s->size_y = size_y;
  s->size_x_cr = size_x_cr;
  s->size_y_cr = size_y_cr;
  s->size_x_m1 = size_x - 1;
  s->size_y_m1 = size_y - 1;
  s->size_x_cr_m1 = size_x_cr - 1;
  s->size_y_cr_m1 = size_y_cr - 1;

  s->top_field = vidParam->no_reference_picture;
  s->bottom_field = vidParam->no_reference_picture;
  s->frame = vidParam->no_reference_picture;

  s->decRefPicMarkingBuffer = NULL;
  s->coded_frame  = 0;
  s->mb_aff_frame_flag  = 0;
  s->topPoc = s->botPoc = s->poc = 0;

  if (!vidParam->activeSPS->frame_mbs_only_flag && structure != FRAME)
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

    if ((p->separate_colour_plane_flag != 0) ) {
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
void fillFrameNumGap (sVidParam* vidParam, sSlice* slice) {

  sSPS* activeSPS = vidParam->activeSPS;

  int tmp1 = slice->delta_pic_order_cnt[0];
  int tmp2 = slice->delta_pic_order_cnt[1];
  slice->delta_pic_order_cnt[0] = slice->delta_pic_order_cnt[1] = 0;

  printf ("A gap in frame number is found, try to fill it.\n");

  int unusedShortTermFrameNum = (vidParam->preFrameNum + 1) % vidParam->max_frame_num;
  int curFrameNum = slice->frame_num; //vidParam->frame_num;

  sPicture* picture = NULL;
  while (curFrameNum != unusedShortTermFrameNum) {
    picture = allocPicture (vidParam, FRAME, vidParam->width, vidParam->height, vidParam->widthCr, vidParam->heightCr, 1);
    picture->coded_frame = 1;
    picture->pic_num = unusedShortTermFrameNum;
    picture->frame_num = unusedShortTermFrameNum;
    picture->non_existing = 1;
    picture->is_output = 1;
    picture->usedForReference = 1;
    picture->adaptive_ref_pic_buffering_flag = 0;

    slice->frame_num = unusedShortTermFrameNum;
    if (activeSPS->pic_order_cnt_type != 0)
      decodePOC (vidParam, vidParam->sliceList[0]);
    picture->topPoc = slice->topPoc;
    picture->botPoc = slice->botPoc;
    picture->framePoc = slice->framePoc;
    picture->poc = slice->framePoc;
    storePictureDpb (slice->dpb, picture);

    vidParam->preFrameNum = unusedShortTermFrameNum;
    unusedShortTermFrameNum = (unusedShortTermFrameNum + 1) % vidParam->max_frame_num;
    }

  slice->delta_pic_order_cnt[0] = tmp1;
  slice->delta_pic_order_cnt[1] = tmp2;
  slice->frame_num = curFrameNum;
  }
//}}}

// dpb
//{{{
static void dumpDpb (sDPB* dpb) {

#ifdef DUMP_DPB
  for (unsigned i = 0; i < dpb->usedSize;i++) {
    printf ("(");
    printf ("fn=%d  ", dpb->fs[i]->frame_num);
    if (dpb->fs[i]->isUsed & 1) {
      if (dpb->fs[i]->top_field)
        printf ("T: poc=%d  ", dpb->fs[i]->top_field->poc);
      else
        printf ("T: poc=%d  ", dpb->fs[i]->frame->topPoc);
      }

    if (dpb->fs[i]->isUsed & 2) {
      if (dpb->fs[i]->bottom_field)
        printf ("B: poc=%d  ", dpb->fs[i]->bottom_field->poc);
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
static int getDpbSize (sVidParam* vidParam, sSPS *activeSPS) {

  int pic_size_mb = (activeSPS->pic_width_in_mbs_minus1 + 1) * (activeSPS->pic_height_in_map_units_minus1 + 1) * (activeSPS->frame_mbs_only_flag?1:2);
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
      if (!is_FREXT_profile(activeSPS->profile_idc) && (activeSPS->constrained_set3_flag == 1))
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
void initDpb (sVidParam* vidParam, sDPB* dpb, int type) {

  sSPS* activeSPS = vidParam->activeSPS;

  dpb->vidParam = vidParam;
  if (dpb->initDone)
    freeDpb (dpb);

  dpb->size = getDpbSize (vidParam, activeSPS) + vidParam->inputParam->dpb_plus[type == 2 ? 1 : 0];
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

  dpb->fs_ref = calloc (dpb->size, sizeof (sFrameStore*));
  if (!dpb->fs_ref)
    no_mem_exit ("initDpb: dpb->fs_ref");

  dpb->fs_ltref = calloc (dpb->size, sizeof (sFrameStore*));
  if (!dpb->fs_ltref)
    no_mem_exit ("initDpb: dpb->fs_ltref");

  for (unsigned i = 0; i < dpb->size; i++) {
    dpb->fs[i] = allocFrameStore();
    dpb->fs_ref[i] = NULL;
    dpb->fs_ltref[i] = NULL;
    }

 /* allocate a dummy storable picture */
  if (!vidParam->no_reference_picture) {
    vidParam->no_reference_picture = allocPicture (vidParam, FRAME, vidParam->width, vidParam->height, vidParam->widthCr, vidParam->heightCr, 1);
    vidParam->no_reference_picture->top_field = vidParam->no_reference_picture;
    vidParam->no_reference_picture->bottom_field = vidParam->no_reference_picture;
    vidParam->no_reference_picture->frame = vidParam->no_reference_picture;
    }
  dpb->lastOutputPoc = INT_MIN;
  vidParam->last_has_mmco_5 = 0;
  dpb->initDone = 1;

  // picture error conceal
  if (vidParam->concealMode !=0 && !vidParam->lastOutFramestore)
    vidParam->lastOutFramestore = allocFrameStore();
  }
//}}}
//{{{
void reInitDpb (sVidParam* vidParam, sDPB* dpb, int type) {

  sSPS* activeSPS = vidParam->activeSPS;
  int iDpbSize = getDpbSize (vidParam, activeSPS)+vidParam->inputParam->dpb_plus[type == 2 ? 1 : 0];
  dpb->numRefFrames = activeSPS->numRefFrames;

  if (iDpbSize > (int)dpb->size) {
    if (dpb->size < activeSPS->numRefFrames)
      error ("DPB size at specified level is smaller than the specified number of reference frames. This is not allowed.\n", 1000);

    dpb->fs = realloc (dpb->fs, iDpbSize * sizeof (sFrameStore*));
    if (!dpb->fs)
      no_mem_exit ("reInitDpb: dpb->fs");

    dpb->fs_ref = realloc(dpb->fs_ref, iDpbSize * sizeof (sFrameStore*));
    if (!dpb->fs_ref)
      no_mem_exit ("reInitDpb: dpb->fs_ref");

    dpb->fs_ltref = realloc(dpb->fs_ltref, iDpbSize * sizeof (sFrameStore*));
    if (!dpb->fs_ltref)
      no_mem_exit ("reInitDpb: dpb->fs_ltref");

    for (int i = dpb->size; i < iDpbSize; i++) {
      dpb->fs[i] = allocFrameStore();
      dpb->fs_ref[i] = NULL;
      dpb->fs_ltref[i] = NULL;
      }

    dpb->size = iDpbSize;
    dpb->lastOutputPoc = INT_MIN;
    dpb->initDone = 1;
    }
  }
//}}}
//{{{
void flushDpb (sDPB* dpb) {

  if (!dpb->initDone)
    return;

  sVidParam* vidParam = dpb->vidParam;
  if (vidParam->concealMode != 0)
    conceal_non_ref_pics(dpb, 0);

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
  sVidParam* vidParam = dpb->vidParam;

  vidParam->last_has_mmco_5 = 0;

  assert (!p->idrFlag);
  assert (p->adaptive_ref_pic_buffering_flag);

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
        mm_unmark_long_term_for_reference (dpb, p, tmp_drpm->long_term_pic_num);
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
        vidParam->last_has_mmco_5 = 1;
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

  if (vidParam->last_has_mmco_5 ) {
    p->pic_num = p->frame_num = 0;
    switch (p->structure) {
      //{{{
      case TOP_FIELD:
        p->poc = p->topPoc = 0;
        break;
      //}}}
      //{{{
      case BOTTOM_FIELD:
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

    flushDpb(vidParam->dpbLayer[0]);
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

  if (p->no_output_of_prior_pics_flag) {
    // free all stored pictures
    for (uint32 i = 0; i < dpb->usedSize; i++) {
      // reset all reference settings
      freeFrameStore (dpb->fs[i]);
      dpb->fs[i] = allocFrameStore();
      }
    for (uint32 i = 0; i < dpb->refFramesInBuffer; i++)
      dpb->fs_ref[i]=NULL;
    for (uint32 i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      dpb->fs_ltref[i]=NULL;
    dpb->usedSize = 0;
    }
  else
    flushDpb (dpb);

  dpb->lastPicture = NULL;

  updateRefList (dpb);
  updateLongTermRefList (dpb);
  dpb->lastOutputPoc = INT_MIN;

  if (p->long_term_reference_flag) {
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
      if (dpb->fs_ltref[i]->isReference == 3)
        if ((dpb->fs_ltref[i]->frame->isLongTerm)&&(dpb->fs_ltref[i]->frame->long_term_pic_num == LongtermPicNum))
          return dpb->fs_ltref[i]->frame;
      }
    else {
      if (dpb->fs_ltref[i]->isReference & 1)
        if ((dpb->fs_ltref[i]->top_field->isLongTerm)&&(dpb->fs_ltref[i]->top_field->long_term_pic_num == LongtermPicNum))
          return dpb->fs_ltref[i]->top_field;
      if (dpb->fs_ltref[i]->isReference & 2)
        if ((dpb->fs_ltref[i]->bottom_field->isLongTerm)&&(dpb->fs_ltref[i]->bottom_field->long_term_pic_num == LongtermPicNum))
          return dpb->fs_ltref[i]->bottom_field;
      }
    }

  return NULL;
  }
//}}}
//{{{
static void gen_pic_list_from_frame_list (ePicStructure currStructure, sFrameStore** fs_list,
                                          int list_idx, sPicture** list, char *list_size, int long_term) {

  int top_idx = 0;
  int bot_idx = 0;

  int (*is_ref)(sPicture*s) = (long_term) ? is_long_ref : is_short_ref;

  if (currStructure == TOP_FIELD) {
    while ((top_idx<list_idx)||(bot_idx<list_idx)) {
      for ( ; top_idx<list_idx; top_idx++) {
        if (fs_list[top_idx]->isUsed & 1) {
          if (is_ref(fs_list[top_idx]->top_field)) {
            // short term ref pic
            list[(short) *list_size] = fs_list[top_idx]->top_field;
            (*list_size)++;
            top_idx++;
            break;
            }
          }
        }

      for ( ; bot_idx<list_idx; bot_idx++) {
        if (fs_list[bot_idx]->isUsed & 2) {
          if (is_ref(fs_list[bot_idx]->bottom_field)) {
            // short term ref pic
            list[(short) *list_size] = fs_list[bot_idx]->bottom_field;
            (*list_size)++;
            bot_idx++;
            break;
            }
          }
        }
      }
    }

  if (currStructure == BOTTOM_FIELD) {
    while ((top_idx<list_idx)||(bot_idx<list_idx)) {
      for ( ; bot_idx<list_idx; bot_idx++) {
        if (fs_list[bot_idx]->isUsed & 2) {
          if (is_ref(fs_list[bot_idx]->bottom_field)) {
            // short term ref pic
            list[(short) *list_size] = fs_list[bot_idx]->bottom_field;
            (*list_size)++;
            bot_idx++;
            break;
            }
          }
        }

      for ( ; top_idx<list_idx; top_idx++) {
        if (fs_list[top_idx]->isUsed & 1) {
          if (is_ref(fs_list[top_idx]->top_field)) {
            // short term ref pic
            list[(short) *list_size] = fs_list[top_idx]->top_field;
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

  int add_top, add_bottom;

  if (p->structure == FRAME) {
    for (uint32 i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fs_ref[i]->isReference == 3) {
        if ((!dpb->fs_ref[i]->frame->isLongTerm)&&(dpb->fs_ref[i]->frame->pic_num == picNumX)) {
          dpb->fs_ref[i]->longTermFrameIndex = dpb->fs_ref[i]->frame->longTermFrameIndex
                                             = longTermFrameIndex;
          dpb->fs_ref[i]->frame->long_term_pic_num = longTermFrameIndex;
          dpb->fs_ref[i]->frame->isLongTerm = 1;

          if (dpb->fs_ref[i]->top_field && dpb->fs_ref[i]->bottom_field) {
            dpb->fs_ref[i]->top_field->longTermFrameIndex = dpb->fs_ref[i]->bottom_field->longTermFrameIndex
                                                          = longTermFrameIndex;
            dpb->fs_ref[i]->top_field->long_term_pic_num = longTermFrameIndex;
            dpb->fs_ref[i]->bottom_field->long_term_pic_num = longTermFrameIndex;

            dpb->fs_ref[i]->top_field->isLongTerm = dpb->fs_ref[i]->bottom_field->isLongTerm = 1;
            }
          dpb->fs_ref[i]->isLongTerm = 3;
          return;
          }
        }
      }
    printf ("Warning: reference frame for long term marking not found\n");
    }

  else {
    if (p->structure == TOP_FIELD) {
      add_top = 1;
      add_bottom = 0;
      }
    else {
      add_top = 0;
      add_bottom = 1;
      }

    for (uint32 i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fs_ref[i]->isReference & 1) {
        if ((!dpb->fs_ref[i]->top_field->isLongTerm) &&
            (dpb->fs_ref[i]->top_field->pic_num == picNumX)) {
          if ((dpb->fs_ref[i]->isLongTerm) &&
              (dpb->fs_ref[i]->longTermFrameIndex != longTermFrameIndex)) {
            printf ("Warning: assigning longTermFrameIndex different from other field\n");
            }

          dpb->fs_ref[i]->longTermFrameIndex = dpb->fs_ref[i]->top_field->longTermFrameIndex = longTermFrameIndex;
          dpb->fs_ref[i]->top_field->long_term_pic_num = 2 * longTermFrameIndex + add_top;
          dpb->fs_ref[i]->top_field->isLongTerm = 1;
          dpb->fs_ref[i]->isLongTerm |= 1;
          if (dpb->fs_ref[i]->isLongTerm == 3) {
            dpb->fs_ref[i]->frame->isLongTerm = 1;
            dpb->fs_ref[i]->frame->longTermFrameIndex = dpb->fs_ref[i]->frame->long_term_pic_num = longTermFrameIndex;
            }
          return;
          }
        }

      if (dpb->fs_ref[i]->isReference & 2) {
        if ((!dpb->fs_ref[i]->bottom_field->isLongTerm) &&
            (dpb->fs_ref[i]->bottom_field->pic_num == picNumX)) {
          if ((dpb->fs_ref[i]->isLongTerm) &&
              (dpb->fs_ref[i]->longTermFrameIndex != longTermFrameIndex))
            printf ("Warning: assigning longTermFrameIndex different from other field\n");

          dpb->fs_ref[i]->longTermFrameIndex = dpb->fs_ref[i]->bottom_field->longTermFrameIndex
                                              = longTermFrameIndex;
          dpb->fs_ref[i]->bottom_field->long_term_pic_num = 2 * longTermFrameIndex + add_bottom;
          dpb->fs_ref[i]->bottom_field->isLongTerm = 1;
          dpb->fs_ref[i]->isLongTerm |= 2;
          if (dpb->fs_ref[i]->isLongTerm == 3) {
            dpb->fs_ref[i]->frame->isLongTerm = 1;
            dpb->fs_ref[i]->frame->longTermFrameIndex = dpb->fs_ref[i]->frame->long_term_pic_num = longTermFrameIndex;
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
    if (dpb->fs_ltref[i]->longTermFrameIndex == longTermFrameIndex)
      unmark_for_long_term_reference (dpb->fs_ltref[i]);
}
//}}}

//{{{
void updateRefList (sDPB* dpb) {

  unsigned i, j;
  for (i = 0, j = 0; i < dpb->usedSize; i++)
    if (isShortTermReference (dpb->fs[i]))
      dpb->fs_ref[j++]=dpb->fs[i];

  dpb->refFramesInBuffer = j;

  while (j < dpb->size)
    dpb->fs_ref[j++] = NULL;
  }
//}}}
//{{{
void updateLongTermRefList (sDPB* dpb) {

  unsigned i, j;
  for (i = 0, j = 0; i < dpb->usedSize; i++)
    if (isLongTermReference (dpb->fs[i]))
      dpb->fs_ltref[j++] = dpb->fs[i];

  dpb->longTermRefFramesInBuffer = j;

  while (j < dpb->size)
    dpb->fs_ltref[j++] = NULL;
  }
//}}}
//{{{
void mm_unmark_long_term_for_reference (sDPB* dpb, sPicture* p, int long_term_pic_num) {

  for (uint32 i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
    if (p->structure == FRAME) {
      if ((dpb->fs_ltref[i]->isReference==3) && (dpb->fs_ltref[i]->isLongTerm==3))
        if (dpb->fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num)
          unmark_for_long_term_reference (dpb->fs_ltref[i]);
      }
    else {
      if ((dpb->fs_ltref[i]->isReference & 1) && ((dpb->fs_ltref[i]->isLongTerm & 1)))
        if (dpb->fs_ltref[i]->top_field->long_term_pic_num == long_term_pic_num) {
          dpb->fs_ltref[i]->top_field->usedForReference = 0;
          dpb->fs_ltref[i]->top_field->isLongTerm = 0;
          dpb->fs_ltref[i]->isReference &= 2;
          dpb->fs_ltref[i]->isLongTerm &= 2;
          if (dpb->fs_ltref[i]->isUsed == 3) {
            dpb->fs_ltref[i]->frame->usedForReference = 0;
            dpb->fs_ltref[i]->frame->isLongTerm = 0;
            }
          return;
          }

      if ((dpb->fs_ltref[i]->isReference & 2) && ((dpb->fs_ltref[i]->isLongTerm & 2)))
        if (dpb->fs_ltref[i]->bottom_field->long_term_pic_num == long_term_pic_num) {
          dpb->fs_ltref[i]->bottom_field->usedForReference = 0;
          dpb->fs_ltref[i]->bottom_field->isLongTerm = 0;
          dpb->fs_ltref[i]->isReference &= 1;
          dpb->fs_ltref[i]->isLongTerm &= 1;
          if (dpb->fs_ltref[i]->isUsed == 3) {
            dpb->fs_ltref[i]->frame->usedForReference = 0;
            dpb->fs_ltref[i]->frame->isLongTerm = 0;
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
      if ((dpb->fs_ref[i]->isReference==3) && (dpb->fs_ref[i]->isLongTerm==0)) {
        if (dpb->fs_ref[i]->frame->pic_num == picNumX) {
          unmark_for_reference(dpb->fs_ref[i]);
          return;
          }
        }
      }
    else {
      if ((dpb->fs_ref[i]->isReference & 1) && (!(dpb->fs_ref[i]->isLongTerm & 1))) {
        if (dpb->fs_ref[i]->top_field->pic_num == picNumX) {
          dpb->fs_ref[i]->top_field->usedForReference = 0;
          dpb->fs_ref[i]->isReference &= 2;
          if (dpb->fs_ref[i]->isUsed == 3)
            dpb->fs_ref[i]->frame->usedForReference = 0;
          return;
          }
        }

      if ((dpb->fs_ref[i]->isReference & 2) && (!(dpb->fs_ref[i]->isLongTerm & 2))) {
        if (dpb->fs_ref[i]->bottom_field->pic_num == picNumX) {
          dpb->fs_ref[i]->bottom_field->usedForReference = 0;
          dpb->fs_ref[i]->isReference &= 1;
          if (dpb->fs_ref[i]->isUsed == 3)
            dpb->fs_ref[i]->frame->usedForReference = 0;
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
      if (dpb->fs_ref[i]->isReference & 1) {
        if (dpb->fs_ref[i]->top_field->pic_num == picNumX) {
          structure = TOP_FIELD;
          break;
          }
        }

      if (dpb->fs_ref[i]->isReference & 2) {
        if (dpb->fs_ref[i]->bottom_field->pic_num == picNumX) {
          structure = BOTTOM_FIELD;
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
    if (dpb->fs_ltref[i]->longTermFrameIndex > dpb->maxLongTermPicIndex)
      unmark_for_long_term_reference(dpb->fs_ltref[i]);
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
    unmark_for_reference (dpb->fs_ref[i]);
  updateRefList (dpb);
  }
//}}}
//{{{
void mm_mark_current_picture_long_term (sDPB* dpb, sPicture* p, int longTermFrameIndex) {

  // remove long term pictures with same longTermFrameIndex
  if (p->structure == FRAME)
    unmark_long_term_frame_for_reference_by_frame_idx(dpb, longTermFrameIndex);
  else
    unmark_long_term_field_for_reference_by_frame_idx(dpb, p->structure, longTermFrameIndex, 1, p->pic_num, 0);

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

  sVidParam* vidParam = dpb->vidParam;
  int poc, pos;
  // picture error conceal

  // diagnostics
  //printf ("Storing (%s) non-ref pic with frame_num #%d\n",
  //        (p->type == FRAME)?"FRAME":(p->type == TOP_FIELD)?"TOP_FIELD":"BOTTOM_FIELD", p->pic_num);
  // if frame, check for new store,
  assert (p!=NULL);

  vidParam->last_has_mmco_5 = 0;
  vidParam->last_pic_bottom_field = (p->structure == BOTTOM_FIELD);

  if (p->idrFlag) {
    idrMemoryManagement (dpb, p);
    // picture error conceal
    memset (vidParam->pocs_in_dpb, 0, sizeof(int)*100);
    }
  else {
    // adaptive memory management
    if (p->usedForReference && (p->adaptive_ref_pic_buffering_flag))
      adaptiveMemoryManagement (dpb, p);
    }

  if ((p->structure == TOP_FIELD) || (p->structure == BOTTOM_FIELD)) {
    // check for frame store with same pic_number
    if (dpb->lastPicture) {
      if ((int)dpb->lastPicture->frame_num == p->pic_num) {
        if (((p->structure == TOP_FIELD) && (dpb->lastPicture->isUsed == 2))||
            ((p->structure == BOTTOM_FIELD) && (dpb->lastPicture->isUsed == 1))) {
          if ((p->usedForReference && (dpb->lastPicture->isOrigReference != 0)) ||
              (!p->usedForReference && (dpb->lastPicture->isOrigReference == 0))) {
            insertPictureDpb (vidParam, dpb->lastPicture, p);
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
      (!p->adaptive_ref_pic_buffering_flag)))
    slidingWindowMemoryManagement (dpb, p);

  // picture error conceal
  if (vidParam->concealMode != 0)
    for (unsigned i = 0; i < dpb->size; i++)
      if (dpb->fs[i]->isReference)
        dpb->fs[i]->concealment_reference = 1;

  // first try to remove unused frames
  if (dpb->usedSize == dpb->size) {
    // picture error conceal
    if (vidParam->concealMode != 0)
      conceal_non_ref_pics (dpb, 2);

    removeUnusedDpb (dpb);

    if (vidParam->concealMode != 0)
      sliding_window_poc_management (dpb, p);
    }

  // then output frames until one can be removed
  while (dpb->usedSize == dpb->size) {
    // non-reference frames may be output directly
    if (!p->usedForReference) {
      get_smallest_poc (dpb, &poc, &pos);
      if ((-1 == pos) || (p->poc < poc)) {
        directOutput (vidParam, p);
        return;
        }
      }

    // flush a frame
    outputDpbFrame (dpb);
    }

  // check for duplicate frame number in short term reference buffer
  if ((p->usedForReference) && (!p->isLongTerm))
    for (unsigned i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fs_ref[i]->frame_num == p->frame_num)
        error ("duplicate frame_num in short-term reference picture buffer", 500);

  // store at end of buffer
  insertPictureDpb (vidParam, dpb->fs[dpb->usedSize],p);

  // picture error conceal
  if (p->idrFlag)
    vidParam->earlier_missing_poc = 0;

  if (p->structure != FRAME)
    dpb->lastPicture = dpb->fs[dpb->usedSize];
  else
    dpb->lastPicture = NULL;

  dpb->usedSize++;

  if (vidParam->concealMode != 0)
    vidParam->pocs_in_dpb[dpb->usedSize-1] = p->poc;

  updateRefList (dpb);
  updateLongTermRefList (dpb);
  checkNumDpbFrames (dpb);
  dumpDpb (dpb);
  }
//}}}
//{{{
void removeFrameDpb (sDPB* dpb, int pos) {

  //printf ("remove frame with frame_num #%d\n", fs->frame_num);
  sFrameStore* fs = dpb->fs[pos];
  switch (fs->isUsed) {
    case 3:
      freePicture (fs->frame);
      freePicture (fs->top_field);
      freePicture (fs->bottom_field);
      fs->frame = NULL;
      fs->top_field = NULL;
      fs->bottom_field = NULL;
      break;

    case 2:
      freePicture (fs->bottom_field);
      fs->bottom_field = NULL;
      break;

    case 1:
      freePicture (fs->top_field);
      fs->top_field = NULL;
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

  sVidParam* vidParam = dpb->vidParam;
  if (dpb->fs) {
    for (unsigned i = 0; i < dpb->size; i++)
      freeFrameStore (dpb->fs[i]);
    free (dpb->fs);
    dpb->fs = NULL;
    }

  if (dpb->fs_ref)
    free (dpb->fs_ref);
  if (dpb->fs_ltref)
    free (dpb->fs_ltref);


  dpb->lastOutputPoc = INT_MIN;
  dpb->initDone = 0;

  // picture error conceal
  if (vidParam->concealMode != 0 || vidParam->lastOutFramestore)
    freeFrameStore (vidParam->lastOutFramestore);

  if (vidParam->no_reference_picture) {
    freePicture (vidParam->no_reference_picture);
    vidParam->no_reference_picture = NULL;
    }
  }
//}}}

// image
//{{{
void initImage (sVidParam* vidParam, sImage* image, sSPS* sps) {

  sInputParam* inputParam = vidParam->inputParam;

  // allocate memory for reference frame buffers: image->frm_data
  image->format = inputParam->output;
  image->format.width[0]  = vidParam->width;
  image->format.width[1]  = vidParam->widthCr;
  image->format.width[2]  = vidParam->widthCr;
  image->format.height[0] = vidParam->height;
  image->format.height[1] = vidParam->heightCr;
  image->format.height[2] = vidParam->heightCr;
  image->format.yuvFormat  = (eColorFormat)sps->chromaFormatIdc;
  image->format.auto_crop_bottom = inputParam->output.auto_crop_bottom;
  image->format.auto_crop_right = inputParam->output.auto_crop_right;
  image->format.auto_crop_bottom_cr = inputParam->output.auto_crop_bottom_cr;
  image->format.auto_crop_right_cr = inputParam->output.auto_crop_right_cr;
  image->frm_stride[0] = vidParam->width;
  image->frm_stride[1] = image->frm_stride[2] = vidParam->widthCr;
  image->top_stride[0] = image->bot_stride[0] = image->frm_stride[0] << 1;
  image->top_stride[1] = image->top_stride[2] = image->bot_stride[1] = image->bot_stride[2] = image->frm_stride[1] << 1;

  if (sps->separate_colour_plane_flag) {
    for (int nplane = 0; nplane < MAX_PLANE; nplane++ )
      get_mem2Dpel (&(image->frm_data[nplane]), vidParam->height, vidParam->width);
    }
  else {
    get_mem2Dpel (&(image->frm_data[0]), vidParam->height, vidParam->width);
    if (vidParam->yuvFormat != YUV400) {
      get_mem2Dpel (&(image->frm_data[1]), vidParam->heightCr, vidParam->widthCr);
      get_mem2Dpel (&(image->frm_data[2]), vidParam->heightCr, vidParam->widthCr);
      if (sizeof(sPixel) == sizeof(unsigned char)) {
        for (int k = 1; k < 3; k++)
          memset (image->frm_data[k][0], 128, vidParam->heightCr * vidParam->widthCr * sizeof(sPixel));
        }
      else {
        sPixel mean_val;
        for (int k = 1; k < 3; k++) {
          mean_val = (sPixel) ((vidParam->max_pel_value_comp[k] + 1) >> 1);
          for (int j = 0; j < vidParam->heightCr; j++)
            for (int i = 0; i < vidParam->widthCr; i++)
              image->frm_data[k][j][i] = mean_val;
          }
        }
      }
    }

  if (!vidParam->activeSPS->frame_mbs_only_flag) {
    // allocate memory for field reference frame buffers
    init_top_bot_planes (image->frm_data[0], vidParam->height, &(image->top_data[0]), &(image->bot_data[0]));
    if (vidParam->yuvFormat != YUV400) {
      init_top_bot_planes (image->frm_data[1], vidParam->heightCr, &(image->top_data[1]), &(image->bot_data[1]));
      init_top_bot_planes (image->frm_data[2], vidParam->heightCr, &(image->top_data[2]), &(image->bot_data[2]));
      }
    }
  }
//}}}
//{{{
void freeImage (sVidParam* vidParam, sImage* image) {

  if (vidParam->separate_colour_plane_flag ) {
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

  if (!vidParam->activeSPS->frame_mbs_only_flag) {
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
      if ((RefPicListX[cIdx]->isLongTerm) || (RefPicListX[cIdx]->pic_num != picNumLX))
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
      if ((!RefPicListX[cIdx]->isLongTerm ) || (RefPicListX[cIdx]->long_term_pic_num != LongTermPicNum))
        RefPicListX[nIdx++] = RefPicListX[cIdx];
  }
//}}}
//{{{
void reorderRefPicList (sSlice* slice, int curList) {

  int* modification_of_pic_nums_idc = slice->modification_of_pic_nums_idc[curList];
  int* abs_diff_pic_num_minus1 = slice->abs_diff_pic_num_minus1[curList];
  int* long_term_pic_idx = slice->long_term_pic_idx[curList];
  int num_ref_idx_lX_active_minus1 = slice->num_ref_idx_active[curList] - 1;

  sVidParam* vidParam = slice->vidParam;

  int maxPicNum, currPicNum, picNumLXNoWrap, picNumLXPred, picNumLX;
  int refIdxLX = 0;

  if (slice->structure==FRAME) {
    maxPicNum = vidParam->max_frame_num;
    currPicNum = slice->frame_num;
    }
  else {
    maxPicNum = 2 * vidParam->max_frame_num;
    currPicNum = 2 * slice->frame_num + 1;
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
void update_pic_num (sSlice* slice) {

  sVidParam* vidParam = slice->vidParam;
  sDPB* dpb = slice->dpb;
  sSPS *activeSPS = vidParam->activeSPS;

  int add_top = 0, add_bottom = 0;
  int max_frame_num = 1 << (activeSPS->log2_max_frame_num_minus4 + 4);

  if (slice->structure == FRAME) {
    for (unsigned int i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fs_ref[i]->isUsed==3 ) {
        if ((dpb->fs_ref[i]->frame->usedForReference)&&(!dpb->fs_ref[i]->frame->isLongTerm)) {
          if (dpb->fs_ref[i]->frame_num > slice->frame_num )
            dpb->fs_ref[i]->frame_num_wrap = dpb->fs_ref[i]->frame_num - max_frame_num;
          else
            dpb->fs_ref[i]->frame_num_wrap = dpb->fs_ref[i]->frame_num;
          dpb->fs_ref[i]->frame->pic_num = dpb->fs_ref[i]->frame_num_wrap;
          }
        }
      }

    // update long_term_pic_num
    for (unsigned int i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->fs_ltref[i]->isUsed==3) {
        if (dpb->fs_ltref[i]->frame->isLongTerm)
          dpb->fs_ltref[i]->frame->long_term_pic_num = dpb->fs_ltref[i]->frame->longTermFrameIndex;
        }
      }
    }
  else {
    if (slice->structure == TOP_FIELD) {
      add_top = 1;
      add_bottom = 0;
      }
    else {
      add_top = 0;
      add_bottom = 1;
      }

    for (unsigned int i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->fs_ref[i]->isReference) {
        if( dpb->fs_ref[i]->frame_num > slice->frame_num )
          dpb->fs_ref[i]->frame_num_wrap = dpb->fs_ref[i]->frame_num - max_frame_num;
        else
          dpb->fs_ref[i]->frame_num_wrap = dpb->fs_ref[i]->frame_num;
        if (dpb->fs_ref[i]->isReference & 1)
          dpb->fs_ref[i]->top_field->pic_num = (2 * dpb->fs_ref[i]->frame_num_wrap) + add_top;
        if (dpb->fs_ref[i]->isReference & 2)
          dpb->fs_ref[i]->bottom_field->pic_num = (2 * dpb->fs_ref[i]->frame_num_wrap) + add_bottom;
        }
      }

    // update long_term_pic_num
    for (unsigned int i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->fs_ltref[i]->isLongTerm & 1)
        dpb->fs_ltref[i]->top_field->long_term_pic_num = 2 * dpb->fs_ltref[i]->top_field->longTermFrameIndex + add_top;
      if (dpb->fs_ltref[i]->isLongTerm & 2)
        dpb->fs_ltref[i]->bottom_field->long_term_pic_num = 2 * dpb->fs_ltref[i]->bottom_field->longTermFrameIndex + add_bottom;
      }
    }
  }
//}}}
//{{{
void init_lists_i_slice (sSlice* slice) {

  slice->listXsize[0] = 0;
  slice->listXsize[1] = 0;
  }
//}}}
//{{{
void init_lists_p_slice (sSlice* slice) {

  sVidParam* vidParam = slice->vidParam;
  sDPB* dpb = slice->dpb;

  unsigned int i;

  int list0idx = 0;
  int listltidx = 0;

  sFrameStore** fs_list0;
  sFrameStore** fs_listlt;

  if (slice->structure == FRAME) {
    for (i=0; i<dpb->refFramesInBuffer; i++) {
      if (dpb->fs_ref[i]->isUsed==3) {
        if ((dpb->fs_ref[i]->frame->usedForReference) && (!dpb->fs_ref[i]->frame->isLongTerm))
          slice->listX[0][list0idx++] = dpb->fs_ref[i]->frame;
      }
    }
    // order list 0 by PicNum
    qsort((void *)slice->listX[0], list0idx, sizeof(sPicture*), compare_pic_by_pic_num_desc);
    slice->listXsize[0] = (char) list0idx;
    //printf("listX[0] (PicNum): "); for (i=0; i<list0idx; i++){printf ("%d  ", slice->listX[0][i]->pic_num);} printf("\n");

    // long term handling
    for (i=0; i<dpb->longTermRefFramesInBuffer; i++)
      if (dpb->fs_ltref[i]->isUsed==3)
        if (dpb->fs_ltref[i]->frame->isLongTerm)
          slice->listX[0][list0idx++] = dpb->fs_ltref[i]->frame;
    qsort((void *)&slice->listX[0][(short) slice->listXsize[0]], list0idx - slice->listXsize[0], sizeof(sPicture*), compare_pic_by_lt_pic_num_asc);
    slice->listXsize[0] = (char) list0idx;
    }
  else {
    fs_list0 = calloc(dpb->size, sizeof (sFrameStore*));
    if (NULL==fs_list0)
      no_mem_exit("init_lists: fs_list0");
    fs_listlt = calloc(dpb->size, sizeof (sFrameStore*));
    if (NULL==fs_listlt)
      no_mem_exit("init_lists: fs_listlt");

    for (i=0; i<dpb->refFramesInBuffer; i++)
      if (dpb->fs_ref[i]->isReference)
        fs_list0[list0idx++] = dpb->fs_ref[i];

    qsort((void *)fs_list0, list0idx, sizeof(sFrameStore*), compare_fs_by_frame_num_desc);

    //printf("fs_list0 (FrameNum): "); for (i=0; i<list0idx; i++){printf ("%d  ", fs_list0[i]->frame_num_wrap);} printf("\n");

    slice->listXsize[0] = 0;
    gen_pic_list_from_frame_list(slice->structure, fs_list0, list0idx, slice->listX[0], &slice->listXsize[0], 0);

    //printf("listX[0] (PicNum): "); for (i=0; i < slice->listXsize[0]; i++){printf ("%d  ", slice->listX[0][i]->pic_num);} printf("\n");

    // long term handling
    for (i=0; i<dpb->longTermRefFramesInBuffer; i++)
      fs_listlt[listltidx++]=dpb->fs_ltref[i];

    qsort((void *)fs_listlt, listltidx, sizeof(sFrameStore*), compare_fs_by_lt_pic_idx_asc);

    gen_pic_list_from_frame_list(slice->structure, fs_listlt, listltidx, slice->listX[0], &slice->listXsize[0], 1);

    free(fs_list0);
    free(fs_listlt);
    }
  slice->listXsize[1] = 0;

  // set max size
  slice->listXsize[0] = (char) imin (slice->listXsize[0], slice->num_ref_idx_active[LIST_0]);
  slice->listXsize[1] = (char) imin (slice->listXsize[1], slice->num_ref_idx_active[LIST_1]);

  // set the unused list entries to NULL
  for (i = slice->listXsize[0]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[0][i] = vidParam->no_reference_picture;
  for (i = slice->listXsize[1]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[1][i] = vidParam->no_reference_picture;
  }
//}}}
//{{{
void init_lists_b_slice (sSlice* slice) {

  sVidParam* vidParam = slice->vidParam;
  sDPB* dpb = slice->dpb;

  unsigned int i;
  int j;

  int list0idx = 0;
  int list0idx_1 = 0;
  int listltidx = 0;

  sFrameStore** fs_list0;
  sFrameStore** fs_list1;
  sFrameStore** fs_listlt;

  // B Slice
  if (slice->structure == FRAME) {
    //{{{  frame
    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fs_ref[i]->isUsed==3)
        if ((dpb->fs_ref[i]->frame->usedForReference)&&(!dpb->fs_ref[i]->frame->isLongTerm))
          if (slice->framePoc >= dpb->fs_ref[i]->frame->poc) //!KS use >= for error conceal
            slice->listX[0][list0idx++] = dpb->fs_ref[i]->frame;
    qsort ((void *)slice->listX[0], list0idx, sizeof(sPicture*), compare_pic_by_poc_desc);

    // get the backward reference picture (POC>current POC) in list0;
    list0idx_1 = list0idx;
    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fs_ref[i]->isUsed==3)
        if ((dpb->fs_ref[i]->frame->usedForReference)&&(!dpb->fs_ref[i]->frame->isLongTerm))
          if (slice->framePoc < dpb->fs_ref[i]->frame->poc)
            slice->listX[0][list0idx++] = dpb->fs_ref[i]->frame;
    qsort ((void*)&slice->listX[0][list0idx_1], list0idx-list0idx_1, sizeof(sPicture*), compare_pic_by_poc_asc);

    for (j = 0; j < list0idx_1; j++)
      slice->listX[1][list0idx-list0idx_1+j]=slice->listX[0][j];
    for (j = list0idx_1; j < list0idx; j++)
      slice->listX[1][j-list0idx_1] = slice->listX[0][j];
    slice->listXsize[0] = slice->listXsize[1] = (char) list0idx;

    // long term handling
    for (i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->fs_ltref[i]->isUsed == 3) {
        if (dpb->fs_ltref[i]->frame->isLongTerm) {
          slice->listX[0][list0idx] = dpb->fs_ltref[i]->frame;
          slice->listX[1][list0idx++] = dpb->fs_ltref[i]->frame;
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
    fs_list0 = calloc(dpb->size, sizeof (sFrameStore*));
    fs_list1 = calloc(dpb->size, sizeof (sFrameStore*));
    fs_listlt = calloc(dpb->size, sizeof (sFrameStore*));
    slice->listXsize[0] = 0;
    slice->listXsize[1] = 1;

    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fs_ref[i]->isUsed)
        if (slice->thisPoc >= dpb->fs_ref[i]->poc)
          fs_list0[list0idx++] = dpb->fs_ref[i];
    qsort ((void *)fs_list0, list0idx, sizeof(sFrameStore*), compare_fs_by_poc_desc);

    list0idx_1 = list0idx;
    for (i = 0; i < dpb->refFramesInBuffer; i++)
      if (dpb->fs_ref[i]->isUsed)
        if (slice->thisPoc < dpb->fs_ref[i]->poc)
          fs_list0[list0idx++] = dpb->fs_ref[i];
    qsort ((void*)&fs_list0[list0idx_1], list0idx-list0idx_1, sizeof(sFrameStore*), compare_fs_by_poc_asc);

    for (j = 0; j < list0idx_1; j++)
      fs_list1[list0idx-list0idx_1+j]=fs_list0[j];
    for (j = list0idx_1; j < list0idx; j++)
      fs_list1[j-list0idx_1]=fs_list0[j];

    slice->listXsize[0] = 0;
    slice->listXsize[1] = 0;
    gen_pic_list_from_frame_list(slice->structure, fs_list0, list0idx, slice->listX[0], &slice->listXsize[0], 0);
    gen_pic_list_from_frame_list(slice->structure, fs_list1, list0idx, slice->listX[1], &slice->listXsize[1], 0);

    // long term handling
    for (i = 0; i < dpb->longTermRefFramesInBuffer; i++)
      fs_listlt[listltidx++] = dpb->fs_ltref[i];

    qsort ((void*)fs_listlt, listltidx, sizeof(sFrameStore*), compare_fs_by_lt_pic_idx_asc);
    gen_pic_list_from_frame_list (slice->structure, fs_listlt, listltidx, slice->listX[0], &slice->listXsize[0], 1);
    gen_pic_list_from_frame_list (slice->structure, fs_listlt, listltidx, slice->listX[1], &slice->listXsize[1], 1);

    free (fs_list0);
    free (fs_list1);
    free (fs_listlt);
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
  slice->listXsize[0] = (char)imin (slice->listXsize[0], slice->num_ref_idx_active[LIST_0]);
  slice->listXsize[1] = (char)imin (slice->listXsize[1], slice->num_ref_idx_active[LIST_1]);

  // set the unused list entries to NULL
  for (i = slice->listXsize[0]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[0][i] = vidParam->no_reference_picture;
  for (i = slice->listXsize[1]; i < (MAX_LIST_SIZE) ; i++)
    slice->listX[1][i] = vidParam->no_reference_picture;
  }
//}}}
//{{{
void init_mbaff_lists (sVidParam* vidParam, sSlice* slice) {
// Initialize listX[2..5] from lists 0 and 1
//   listX[2]: list0 for current_field==top
//   listX[3]: list1 for current_field==top
//   listX[4]: list0 for current_field==bottom
//   listX[5]: list1 for current_field==bottom

  for (int i = 2; i < 6; i++) {
    for (unsigned j = 0; j < MAX_LIST_SIZE; j++)
      slice->listX[i][j] = vidParam->no_reference_picture;
    slice->listXsize[i]=0;
    }

  for (int i = 0; i < slice->listXsize[0]; i++) {
    slice->listX[2][2*i  ] = slice->listX[0][i]->top_field;
    slice->listX[2][2*i+1] = slice->listX[0][i]->bottom_field;
    slice->listX[4][2*i  ] = slice->listX[0][i]->bottom_field;
    slice->listX[4][2*i+1] = slice->listX[0][i]->top_field;
    }
  slice->listXsize[2] = slice->listXsize[4] = slice->listXsize[0] * 2;

  for (int i = 0; i < slice->listXsize[1]; i++) {
    slice->listX[3][2*i  ] = slice->listX[1][i]->top_field;
    slice->listX[3][2*i+1] = slice->listX[1][i]->bottom_field;
    slice->listX[5][2*i  ] = slice->listX[1][i]->bottom_field;
    slice->listX[5][2*i+1] = slice->listX[1][i]->top_field;
    }
  slice->listXsize[3] = slice->listXsize[5] = slice->listXsize[1] * 2;
  }
//}}}
//{{{
sPicture* get_short_term_pic (sSlice* slice, sDPB* dpb, int picNum) {

  for (unsigned i = 0; i < dpb->refFramesInBuffer; i++) {
    if (slice->structure == FRAME) {
      if (dpb->fs_ref[i]->isReference == 3)
        if ((!dpb->fs_ref[i]->frame->isLongTerm)&&(dpb->fs_ref[i]->frame->pic_num == picNum))
          return dpb->fs_ref[i]->frame;
      }
    else {
      if (dpb->fs_ref[i]->isReference & 1)
        if ((!dpb->fs_ref[i]->top_field->isLongTerm)&&(dpb->fs_ref[i]->top_field->pic_num == picNum))
          return dpb->fs_ref[i]->top_field;
      if (dpb->fs_ref[i]->isReference & 2)
        if ((!dpb->fs_ref[i]->bottom_field->isLongTerm)&&(dpb->fs_ref[i]->bottom_field->pic_num == picNum))
          return dpb->fs_ref[i]->bottom_field;
      }
    }

  return slice->vidParam->no_reference_picture;
  }
//}}}

//{{{
void alloc_ref_pic_list_reordering_buffer (sSlice* slice) {

  if (slice->sliceType != I_SLICE && slice->sliceType != SI_SLICE) {
    int size = slice->num_ref_idx_active[LIST_0] + 1;
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
    int size = slice->num_ref_idx_active[LIST_1] + 1;
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
void free_ref_pic_list_reordering_buffer (sSlice* slice) {

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
void compute_colocated (sSlice* slice, sPicture** listX[6]) {

  sVidParam* vidParam = slice->vidParam;
  if (slice->direct_spatial_mv_pred_flag == 0) {
    for (int j = 0; j < 2 + (slice->mb_aff_frame_flag * 4); j += 2) {
      for (int i = 0; i < slice->listXsize[j];i++) {
        int iTRb;
        if (j == 0)
          iTRb = iClip3 (-128, 127, vidParam->picture->poc - listX[LIST_0 + j][i]->poc);
        else if (j == 2)
          iTRb = iClip3 (-128, 127, vidParam->picture->topPoc - listX[LIST_0 + j][i]->poc);
        else
          iTRb = iClip3 (-128, 127, vidParam->picture->botPoc - listX[LIST_0 + j][i]->poc);

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
