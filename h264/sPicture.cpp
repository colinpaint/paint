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

  sPicture* picture = (sPicture*)calloc (1, sizeof(sPicture));

  if (picStructure != eFrame) {
    sizeY /= 2;
    sizeYcr /= 2;
    }

  picture->picSizeInMbs = (sizeX*sizeY) / 256;
  picture->imgUV = NULL;

  getMem2DpelPad (&picture->imgY, sizeY, sizeX, decoder->coding.lumaPadY, decoder->coding.lumaPadX);
  picture->lumaStride = sizeX + 2 * decoder->coding.lumaPadX;
  picture->lumaExpandedHeight = sizeY + 2 * decoder->coding.lumaPadY;

  if (decoder->activeSps->chromaFormatIdc != YUV400)
    getMem3DpelPad (&picture->imgUV, 2, sizeYcr, sizeXcr, decoder->coding.chromaPadY, decoder->coding.chromaPadX);

  picture->chromaStride = sizeXcr + 2*decoder->coding.chromaPadX;
  picture->chromaExpandedHeight = sizeYcr + 2*decoder->coding.chromaPadY;
  picture->lumaPadY = decoder->coding.lumaPadY;
  picture->lumaPadX = decoder->coding.lumaPadX;
  picture->chromaPadY = decoder->coding.chromaPadY;
  picture->chromaPadX = decoder->coding.chromaPadX;
  picture->isSeperateColourPlane = decoder->coding.isSeperateColourPlane;

  getMem2Dmp (&picture->mvInfo, sizeY >> BLOCK_SHIFT, sizeX >> BLOCK_SHIFT);
  allocPicMotion (&picture->motion , sizeY >> BLOCK_SHIFT, sizeX >> BLOCK_SHIFT);
  if (decoder->coding.isSeperateColourPlane != 0)
    for (int nplane = 0; nplane < MAX_PLANE; nplane++) {
      getMem2Dmp (&picture->mvInfoJV[nplane], sizeY >> BLOCK_SHIFT, sizeX >> BLOCK_SHIFT);
      allocPicMotion (&picture->motionJV[nplane] , sizeY >> BLOCK_SHIFT, sizeX >> BLOCK_SHIFT);
      }

  picture->picNum = 0;
  picture->frameNum = 0;
  picture->longTermFrameIndex = 0;
  picture->longTermPicNum = 0;
  picture->usedForRef  = 0;
  picture->usedLongTermRef = 0;
  picture->nonExisting = 0;
  picture->isOutput = 0;
  picture->maxSliceId = 0;
  picture->picStructure = picStructure;

  picture->sizeX = sizeX;
  picture->sizeY = sizeY;
  picture->sizeXcr = sizeXcr;
  picture->sizeYcr = sizeYcr;
  picture->size_x_m1 = sizeX - 1;
  picture->size_y_m1 = sizeY - 1;
  picture->size_x_cr_m1 = sizeXcr - 1;
  picture->size_y_cr_m1 = sizeYcr - 1;

  picture->topField = decoder->dpb.noRefPicture;
  picture->botField = decoder->dpb.noRefPicture;
  picture->frame = decoder->dpb.noRefPicture;

  picture->decRefPicMarkBuffer = NULL;
  picture->codedFrame = 0;
  picture->mbAffFrame = 0;

  picture->topPoc = picture->botPoc = picture->poc = 0;

  if (!decoder->activeSps->frameMbOnly && (picStructure != eFrame))
    for (int j = 0; j < MAX_NUM_SLICES; j++)
      for (int i = 0; i < 2; i++)
        picture->listX[j][i] = (sPicture**)calloc (MAX_LIST_SIZE, sizeof(sPicture*)); // +1 for reordering

  return picture;
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
