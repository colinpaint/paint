//{{{  includes
#include <limits.h>

#include "global.h"
#include "memory.h"

#include "erc.h"
//}}}
namespace {
  //{{{
  void freePicMotion (sPicMotionOld* motion) {

    free (motion->mbField);
    motion->mbField = NULL;
    }
  //}}}
  //{{{
  void genFieldRefIds (cDecoder264* decoder, sPicture* p) {
  // generate frame parameters from field information

    // copy the list;
    for (int j = 0; j < decoder->picSliceIndex; j++) {
      if (p->listX[j][LIST_0]) {
        p->listXsize[j][LIST_0] = decoder->sliceList[j]->listXsize[LIST_0];
        for (int i = 0; i < p->listXsize[j][LIST_0]; i++)
          p->listX[j][LIST_0][i] = decoder->sliceList[j]->listX[LIST_0][i];
        }

      if (p->listX[j][LIST_1]) {
        p->listXsize[j][LIST_1] = decoder->sliceList[j]->listXsize[LIST_1];
        for (int i = 0; i < p->listXsize[j][LIST_1]; i++)
          p->listX[j][LIST_1][i] = decoder->sliceList[j]->listX[LIST_1][i];
        }
      }
    }
  //}}}
  }

//{{{
cFrameStore::~cFrameStore() {

  sPicture::freePicture (frame);
  sPicture::freePicture (topField);
  sPicture::freePicture (botField);
  }
//}}}

//{{{
int cFrameStore::isRef() {

  if (usedRef)
    return 1;

  if (isUsed == 3) // frame
    if (frame->usedForRef)
      return 1;

  if (isUsed & 1) // topField
    if (topField)
      if (topField->usedForRef)
        return 1;

  if (isUsed & 2) // botField
    if (botField)
      if (botField->usedForRef)
        return 1;

  return 0;
  }
//}}}
//{{{
int cFrameStore::isShortTermRef() {

  if (isUsed == 3) // frame
    if ((frame->usedForRef) && (!frame->usedLongTermRef))
      return 1;

  if (isUsed & 1) // topField
    if (topField)
      if ((topField->usedForRef) && (!topField->usedLongTermRef))
        return 1;

  if (isUsed & 2) // botField
    if (botField)
      if ((botField->usedForRef) && (!botField->usedLongTermRef))
        return 1;

  return 0;
  }
//}}}
//{{{
int cFrameStore::isLongTermRef() {

  if (isUsed == 3) // frame
    if ((frame->usedForRef) && (frame->usedLongTermRef))
      return 1;

  if (isUsed & 1) // topField
    if (topField)
      if ((topField->usedForRef) && (topField->usedLongTermRef))
        return 1;

  if (isUsed & 2) // botField
    if (botField)
      if ((botField->usedForRef) && (botField->usedLongTermRef))
        return 1;

  return 0;
  }
//}}}

//{{{
void cFrameStore::unmarkForRef() {

  if (isUsed & 1)
    if (topField)
      topField->usedForRef = 0;

  if (isUsed & 2)
    if (botField)
      botField->usedForRef = 0;

  if (isUsed == 3) {
    if (topField && botField) {
      topField->usedForRef = 0;
      botField->usedForRef = 0;
      }
    frame->usedForRef = 0;
    }

  usedRef = 0;

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
      topField->usedForRef = 0;
      topField->usedLongTermRef = 0;
      }
    }

  if (isUsed & 2) {
    if (botField) {
      botField->usedForRef = 0;
      botField->usedLongTermRef = 0;
      }
    }

  if (isUsed == 3) {
    if (topField && botField) {
      topField->usedForRef = 0;
      topField->usedLongTermRef = 0;
      botField->usedForRef = 0;
      botField->usedLongTermRef = 0;
      }
    frame->usedForRef = 0;
    frame->usedLongTermRef = 0;
    }

  usedRef = 0;
  usedLongTermRef = 0;
  }
//}}}

//{{{
void cFrameStore::dpbCombineField (cDecoder264* decoder) {

  if (!frame)
    frame = sPicture::allocPicture (decoder, eFrame,
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

  poc = imin(topField->poc, botField->poc);
  frame->framePoc = poc;;
  botField->framePoc = frame->poc;
  topField->framePoc = frame->poc;
  botField->topPoc = frame->topPoc = topField->poc;
  topField->botPoc = frame->botPoc = botField->poc;

  frame->usedForRef = topField->usedForRef && botField->usedForRef;
  frame->usedLongTermRef = topField->usedLongTermRef && botField->usedLongTermRef;

  if (frame->usedLongTermRef)
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
  if (topField->usedForRef || botField->usedForRef)
    decoder->padPicture (frame);
  }
//}}}
//{{{
void cFrameStore::dpbCombineField1 (cDecoder264* decoder) {

  dpbCombineField (decoder);

  frame->codingType = topField->codingType; //eFieldCoding;
   //! Use inference flag to remap mvs/references

  //! Generate Frame parameters from field information.
  for (int j = 0 ; j < (topField->sizeY >> 2); j++) {
    int jj = (j << 1);
    int jj4 = jj + 1;
    for (int i = 0 ; i < (topField->sizeX >> 2); i++) {
      frame->mvInfo[jj][i].mv[LIST_0] = topField->mvInfo[j][i].mv[LIST_0];
      frame->mvInfo[jj][i].mv[LIST_1] = topField->mvInfo[j][i].mv[LIST_1];

      frame->mvInfo[jj][i].refIndex[LIST_0] = topField->mvInfo[j][i].refIndex[LIST_0];
      frame->mvInfo[jj][i].refIndex[LIST_1] = topField->mvInfo[j][i].refIndex[LIST_1];

      // bug: top field list doesnot exist.*/
      int l = topField->mvInfo[j][i].slice_no;
      int k = topField->mvInfo[j][i].refIndex[LIST_0];
      frame->mvInfo[jj][i].refPic[LIST_0] = k>=0? topField->listX[l][LIST_0][k]: NULL;
      k = topField->mvInfo[j][i].refIndex[LIST_1];
      frame->mvInfo[jj][i].refPic[LIST_1] = k>=0? topField->listX[l][LIST_1][k]: NULL;

      // association with id already known for fields.
      frame->mvInfo[jj4][i].mv[LIST_0] = botField->mvInfo[j][i].mv[LIST_0];
      frame->mvInfo[jj4][i].mv[LIST_1] = botField->mvInfo[j][i].mv[LIST_1];

      frame->mvInfo[jj4][i].refIndex[LIST_0] = botField->mvInfo[j][i].refIndex[LIST_0];
      frame->mvInfo[jj4][i].refIndex[LIST_1] = botField->mvInfo[j][i].refIndex[LIST_1];
      l = botField->mvInfo[j][i].slice_no;

      k = botField->mvInfo[j][i].refIndex[LIST_0];
      frame->mvInfo[jj4][i].refPic[LIST_0] = k >= 0 ? botField->listX[l][LIST_0][k]: NULL;
      k = botField->mvInfo[j][i].refIndex[LIST_1];
      frame->mvInfo[jj4][i].refPic[LIST_1] = k >= 0 ? botField->listX[l][LIST_1][k]: NULL;
      }
    }  
  }
//}}}
//{{{
void cFrameStore::dpbSplitField (cDecoder264* decoder) {

  int twosz16 = 2 * (frame->sizeX >> 4);
  sPicture* fsTop = NULL;
  sPicture* fsBot = NULL;

  poc = frame->poc;
  if (!frame->frameMbOnly) {
    fsTop = topField = sPicture::allocPicture (decoder, eTopField,
                                                 frame->sizeX, frame->sizeY, frame->sizeXcr,
                                                 frame->sizeYcr, 1);
    fsBot = botField = sPicture::allocPicture (decoder, eBotField,
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

    fsTop->usedForRef = fsBot->usedForRef = frame->usedForRef;
    fsTop->usedLongTermRef = fsBot->usedLongTermRef = frame->usedLongTermRef;
    longTermFrameIndex = fsTop->longTermFrameIndex
                                   = fsBot->longTermFrameIndex
                                   = frame->longTermFrameIndex;

    fsTop->codedFrame = fsBot->codedFrame = 1;
    fsTop->mbAffFrame = fsBot->mbAffFrame = frame->mbAffFrame;

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
    fsTop->codingType = fsBot->codingType = frame->codingType;
    if (frame->usedForRef)  {
      decoder->padPicture (fsTop);
      decoder->padPicture (fsBot);
      }
    }
  else {
    topField = NULL;
    botField = NULL;
    frame->topField = NULL;
    frame->botField = NULL;
    frame->frame = frame;
    }

  if (!frame->frameMbOnly) {
    if (frame->mbAffFrame) {
      sPicMotionOld* frm_motion = &frame->motion;
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
              fsBot->mvInfo[j][i].refPic[LIST_0] = decoder->sliceList[frame->mvInfo[jj4][i].slice_no]->listX[4][(int16_t) fsBot->mvInfo[j][i].refIndex[LIST_0]];
            else
              fsBot->mvInfo[j][i].refPic[LIST_0] = NULL;
            fsBot->mvInfo[j][i].refIndex[LIST_1] = frame->mvInfo[jj4][i].refIndex[LIST_1];
            if (fsBot->mvInfo[j][i].refIndex[LIST_1] >=0)
              fsBot->mvInfo[j][i].refPic[LIST_1] = decoder->sliceList[frame->mvInfo[jj4][i].slice_no]->listX[5][(int16_t) fsBot->mvInfo[j][i].refIndex[LIST_1]];
            else
              fsBot->mvInfo[j][i].refPic[LIST_1] = NULL;

            fsTop->mvInfo[j][i].mv[LIST_0] = frame->mvInfo[jj][i].mv[LIST_0];
            fsTop->mvInfo[j][i].mv[LIST_1] = frame->mvInfo[jj][i].mv[LIST_1];
            fsTop->mvInfo[j][i].refIndex[LIST_0] = frame->mvInfo[jj][i].refIndex[LIST_0];
            if (fsTop->mvInfo[j][i].refIndex[LIST_0] >=0)
              fsTop->mvInfo[j][i].refPic[LIST_0] = decoder->sliceList[frame->mvInfo[jj][i].slice_no]->listX[2][(int16_t) fsTop->mvInfo[j][i].refIndex[LIST_0]];
            else
              fsTop->mvInfo[j][i].refPic[LIST_0] = NULL;
            fsTop->mvInfo[j][i].refIndex[LIST_1] = frame->mvInfo[jj][i].refIndex[LIST_1];
            if (fsTop->mvInfo[j][i].refIndex[LIST_1] >=0)
              fsTop->mvInfo[j][i].refPic[LIST_1] = decoder->sliceList[frame->mvInfo[jj][i].slice_no]->listX[3][(int16_t) fsTop->mvInfo[j][i].refIndex[LIST_1]];
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
        if (!frame->mbAffFrame  || !frame->motion.mbField[currentmb]) {
          fsTop->mvInfo[j][i].mv[LIST_0] = fsBot->mvInfo[j][i].mv[LIST_0] = frame->mvInfo[jj][ii].mv[LIST_0];
          fsTop->mvInfo[j][i].mv[LIST_1] = fsBot->mvInfo[j][i].mv[LIST_1] = frame->mvInfo[jj][ii].mv[LIST_1];

          // Scaling of references is done here since it will not affect spatial direct (2*0 =0)
          if (frame->mvInfo[jj][ii].refIndex[LIST_0] == -1) {
            fsTop->mvInfo[j][i].refIndex[LIST_0] = fsBot->mvInfo[j][i].refIndex[LIST_0] = - 1;
            fsTop->mvInfo[j][i].refPic[LIST_0] = fsBot->mvInfo[j][i].refPic[LIST_0] = NULL;
            }
          else {
            fsTop->mvInfo[j][i].refIndex[LIST_0] = fsBot->mvInfo[j][i].refIndex[LIST_0] = frame->mvInfo[jj][ii].refIndex[LIST_0];
            fsTop->mvInfo[j][i].refPic[LIST_0] = fsBot->mvInfo[j][i].refPic[LIST_0] = decoder->sliceList[frame->mvInfo[jj][ii].slice_no]->listX[LIST_0][(int16_t) frame->mvInfo[jj][ii].refIndex[LIST_0]];
            }

          if (frame->mvInfo[jj][ii].refIndex[LIST_1] == -1) {
            fsTop->mvInfo[j][i].refIndex[LIST_1] = fsBot->mvInfo[j][i].refIndex[LIST_1] = - 1;
            fsTop->mvInfo[j][i].refPic[LIST_1] = fsBot->mvInfo[j][i].refPic[LIST_1] = NULL;
            }
          else {
            fsTop->mvInfo[j][i].refIndex[LIST_1] = fsBot->mvInfo[j][i].refIndex[LIST_1] = frame->mvInfo[jj][ii].refIndex[LIST_1];
            fsTop->mvInfo[j][i].refPic[LIST_1] = fsBot->mvInfo[j][i].refPic[LIST_1] = decoder->sliceList[frame->mvInfo[jj][ii].slice_no]->listX[LIST_1][(int16_t) frame->mvInfo[jj][ii].refIndex[LIST_1]];
            }
          }
        }
      }
    }
  }
//}}}
//{{{
void cFrameStore::insertPictureDpb (cDecoder264* decoder, sPicture* picture) {

  switch (picture->picStructure) {
    case eFrame:
      frame = picture;
      isUsed = 3;
      if (picture->usedForRef) {
        usedRef = 3;
        usedOrigRef = 3;
        if (picture->usedLongTermRef) {
          usedLongTermRef = 3;
          longTermFrameIndex = picture->longTermFrameIndex;
          }
        }
      dpbSplitField (decoder);
      break;

    case eTopField:
      topField = picture;
      isUsed |= 1;

      if (picture->usedForRef) {
        usedRef |= 1;
        usedOrigRef |= 1;
        if (picture->usedLongTermRef) {
          usedLongTermRef |= 1;
          longTermFrameIndex = picture->longTermFrameIndex;
          }
        }
      if (isUsed == 3)
        dpbCombineField1 (decoder);
      else
        poc = picture->poc;
      genFieldRefIds (decoder, picture);
      break;

    case eBotField:
      botField = picture;
      isUsed |= 2;
      if (picture->usedForRef) {
        usedRef |= 2;
        usedOrigRef |= 2;
        if (picture->usedLongTermRef) {
          usedLongTermRef |= 2;
          longTermFrameIndex = picture->longTermFrameIndex;
          }
        }
      if (isUsed == 3)
        dpbCombineField1 (decoder);
      else
        poc = picture->poc;
      genFieldRefIds (decoder, picture);
      break;
    }

  frameNum = picture->picNum;
  recoveryFrame = picture->recoveryFrame;
  isOutput = picture->isOutput;
  }
//}}}
