//{{{  includes
#include <limits.h>

#include "global.h"
#include "memory.h"

#include "../common/cLog.h"

using namespace std;
//}}}
namespace {
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

// static
//{{{
sPicture* sPicture::allocPicture (cDecoder264* decoder, ePicStructure picStructure,
                        int sizeX, int sizeY, int sizeXcr, int sizeYcr, int isOutput) {

  sPicture* s = (sPicture*)calloc (1, sizeof(sPicture));

  if (picStructure != eFrame) {
    sizeY /= 2;
    sizeYcr /= 2;
    }

  s->picSizeInMbs = (sizeX*sizeY) / 256;
  s->imgUV = NULL;

  getMem2DpelPad (&s->imgY, sizeY, sizeX, decoder->coding.lumaPadY, decoder->coding.lumaPadX);
  s->lumaStride = sizeX + 2 * decoder->coding.lumaPadX;
  s->lumaExpandedHeight = sizeY + 2 * decoder->coding.lumaPadY;

  if (decoder->activeSps->chromaFormatIdc != YUV400)
    getMem3DpelPad (&s->imgUV, 2, sizeYcr, sizeXcr, decoder->coding.chromaPadY, decoder->coding.chromaPadX);

  s->chromaStride = sizeXcr + 2*decoder->coding.chromaPadX;
  s->chromaExpandedHeight = sizeYcr + 2*decoder->coding.chromaPadY;
  s->lumaPadY = decoder->coding.lumaPadY;
  s->lumaPadX = decoder->coding.lumaPadX;
  s->chromaPadY = decoder->coding.chromaPadY;
  s->chromaPadX = decoder->coding.chromaPadX;
  s->isSeperateColourPlane = decoder->coding.isSeperateColourPlane;

  getMem2Dmp (&s->mvInfo, sizeY >> BLOCK_SHIFT, sizeX >> BLOCK_SHIFT);
  allocPicMotion (&s->motion , sizeY >> BLOCK_SHIFT, sizeX >> BLOCK_SHIFT);

  if (decoder->coding.isSeperateColourPlane != 0)
    for (int nplane = 0; nplane < MAX_PLANE; nplane++) {
      getMem2Dmp (&s->mvInfoJV[nplane], sizeY >> BLOCK_SHIFT, sizeX >> BLOCK_SHIFT);
      allocPicMotion (&s->motionJV[nplane] , sizeY >> BLOCK_SHIFT, sizeX >> BLOCK_SHIFT);
      }

  s->picNum = 0;
  s->frameNum = 0;
  s->longTermFrameIndex = 0;
  s->longTermPicNum = 0;
  s->usedForRef  = 0;
  s->usedLongTermRef = 0;
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

  s->topField = decoder->dpb.noRefPicture;
  s->botField = decoder->dpb.noRefPicture;
  s->frame = decoder->dpb.noRefPicture;

  s->decRefPicMarkBuffer = NULL;
  s->codedFrame = 0;
  s->mbAffFrame = 0;
  s->topPoc = s->botPoc = s->poc = 0;

  if (!decoder->activeSps->frameMbOnly && (picStructure != eFrame))
    for (int j = 0; j < MAX_NUM_SLICES; j++)
      for (int i = 0; i < 2; i++)
        s->listX[j][i] = (sPicture**)calloc (MAX_LIST_SIZE, sizeof(sPicture*)); // +1 for reordering

  return s;
  }
//}}}
//{{{
void sPicture::freePicture (sPicture*& picture) {

  if (picture) {
    if (picture->mvInfo) {
      freeMem2Dmp (picture->mvInfo);
      picture->mvInfo = NULL;
      }
    freePicMotion (&picture->motion);

    if (picture->isSeperateColourPlane != 0) {
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
    }

  free (picture);
  picture = NULL;
  }
//}}}

// no idea
//{{{
void fillFrameNumGap (cDecoder264* decoder, cSlice* slice) {

  cLog::log (LOGERROR, "gap in frame number is found, try to fill it");

  int tmp1 = slice->deltaPicOrderCount[0];
  int tmp2 = slice->deltaPicOrderCount[1];
  slice->deltaPicOrderCount[0] = slice->deltaPicOrderCount[1] = 0;

  int unusedShortTermFrameNum = (decoder->preFrameNum + 1) % decoder->coding.maxFrameNum;
  int curFrameNum = slice->frameNum;

  while (curFrameNum != unusedShortTermFrameNum) {
    sPicture* picture = sPicture::allocPicture (decoder, eFrame, decoder->coding.width, decoder->coding.height, 
                                                decoder->widthCr, decoder->heightCr, 1);
    picture->codedFrame = 1;
    picture->picNum = unusedShortTermFrameNum;
    picture->frameNum = unusedShortTermFrameNum;
    picture->nonExisting = 1;
    picture->isOutput = 1;
    picture->usedForRef = 1;
    picture->adaptRefPicBufFlag = 0;

    slice->frameNum = unusedShortTermFrameNum;
    if (decoder->activeSps->pocType != 0)
      decoder->decodePOC (decoder->sliceList[0]);
    picture->topPoc = slice->topPoc;
    picture->botPoc = slice->botPoc;
    picture->framePoc = slice->framePoc;
    picture->poc = slice->framePoc;
    slice->dpb->storePicture (picture);

    decoder->preFrameNum = unusedShortTermFrameNum;
    unusedShortTermFrameNum = (unusedShortTermFrameNum + 1) % decoder->coding.maxFrameNum;
    }

  slice->deltaPicOrderCount[0] = tmp1;
  slice->deltaPicOrderCount[1] = tmp2;
  slice->frameNum = curFrameNum;
  }
//}}}
