//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
//}}}

//{{{
static void allocDecodedPic (sVidParam* vidParam, sDecodedPicture* decodedPicture, sPicture* p,
                             int lumaSize, int frameSize, int lumaSizeX, int lumaSizeY,
                             int chromaSizeX, int chromaSizeY) {

  int symbolSizeInBytes = (vidParam->picUnitBitSizeDisk + 7) >> 3;

  if (decodedPicture->yBuf)
    memFree (decodedPicture->yBuf);

  decodedPicture->bufSize = frameSize;
  decodedPicture->yBuf = memAlloc (decodedPicture->bufSize);
  decodedPicture->uBuf = decodedPicture->yBuf + lumaSize;
  decodedPicture->vBuf = decodedPicture->uBuf + ((frameSize - lumaSize)>>1);

  decodedPicture->yuvFormat = p->chromaFormatIdc;
  decodedPicture->yuvStorageFormat = 0;
  decodedPicture->iBitDepth = vidParam->picUnitBitSizeDisk;
  decodedPicture->width = lumaSizeX;
  decodedPicture->height = lumaSizeY;
  decodedPicture->yStride = lumaSizeX * symbolSizeInBytes;
  decodedPicture->uvStride = chromaSizeX * symbolSizeInBytes;
  }
//}}}

//{{{
static void img2buf (sPixel** imgX, unsigned char* buf,
                     int sizeX, int sizeY, int symbolSizeInBytes,
                     int cropLeft, int cropRight, int cropTop, int cropBot, int outStride) {

  if (cropLeft || cropRight) {
    for (int i = cropTop; i < sizeY - cropBot; i++) {
      int pos = ((i - cropTop) * outStride) + cropLeft;
      memcpy (buf + pos, imgX[i], sizeX - cropRight - cropLeft);
      }
    }
  else {
    buf += cropTop * outStride;
    for (int i = 0; i < sizeY - cropBot - cropTop; i++) {
      memcpy (buf, imgX[i], sizeX);
      buf += outStride;
      }
    }
  }
//}}}
//{{{
static void clearPicture (sVidParam* vidParam, sPicture* p) {

  printf ("-------------------- clearPicture -----------------\n");

  for (int i = 0; i < p->sizeY; i++)
    for (int j = 0; j < p->sizeX; j++)
      p->imgY[i][j] = (sPixel)vidParam->dcPredValueComp[0];

  for (int i = 0; i < p->sizeYcr;i++)
    for (int j = 0; j < p->sizeXcr; j++)
      p->imgUV[0][i][j] = (sPixel)vidParam->dcPredValueComp[1];

  for (int i = 0; i < p->sizeYcr;i++)
    for (int j = 0; j < p->sizeXcr; j++)
      p->imgUV[1][i][j] = (sPixel)vidParam->dcPredValueComp[2];
  }
//}}}
//{{{
static void writeOutPicture (sVidParam* vidParam, sPicture* p) {

  static const int SubWidthC [4]= { 1, 2, 2, 1 };
  static const int SubHeightC [4]= { 1, 2, 1, 1 };

  if (p->non_existing)
    return;

  int cropLeft;
  int cropRight;
  int cropTop;
  int cropBottom;
  if (p->frameCropFlag) {
    cropLeft = SubWidthC [p->chromaFormatIdc] * p->frameCropLeft;
    cropRight = SubWidthC [p->chromaFormatIdc] * p->frameCropRight;
    cropTop = SubHeightC[p->chromaFormatIdc] * ( 2 - p->frameMbOnlyFlag ) * p->frameCropTop;
    cropBottom = SubHeightC[p->chromaFormatIdc] * ( 2 - p->frameMbOnlyFlag ) * p->frameCropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBottom = 0;

  int symbolSizeInBytes = ((vidParam->picUnitBitSizeDisk+7) >> 3);
  int chromaSizeX =  p->sizeXcr- p->frameCropLeft -p->frameCropRight;
  int chromaSizeY = p->sizeYcr - ( 2 - p->frameMbOnlyFlag ) * p->frameCropTop -( 2 - p->frameMbOnlyFlag ) * p->frameCropBot;
  int lumaSizeX = p->sizeX - cropLeft - cropRight;
  int lumaSizeY = p->sizeY - cropTop - cropBottom;
  int lumaSize = lumaSizeX * lumaSizeY * symbolSizeInBytes;
  int frameSize = (lumaSizeX * lumaSizeY + 2 * (chromaSizeX * chromaSizeY)) * symbolSizeInBytes;

  sDecodedPicture* decodedPicture = getDecodedPicture (vidParam->decOutputPic);
  if (!decodedPicture->yBuf || (decodedPicture->bufSize < frameSize))
    allocDecodedPic (vidParam, decodedPicture, p, lumaSize, frameSize, lumaSizeX, lumaSizeY, chromaSizeX, chromaSizeY);
  decodedPicture->valid = 1;
  decodedPicture->poc = p->framePoc;
  if (!decodedPicture->yBuf)
    no_mem_exit ("writeOutPicture: buf");

  img2buf (p->imgY,
           (decodedPicture->valid == 1) ? decodedPicture->yBuf : decodedPicture->yBuf + lumaSizeX * symbolSizeInBytes,
           p->sizeX, p->sizeY, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPicture->yStride);

  cropLeft = p->frameCropLeft;
  cropRight = p->frameCropRight;
  cropTop = (2 - p->frameMbOnlyFlag) * p->frameCropTop;
  cropBottom = (2 - p->frameMbOnlyFlag) * p->frameCropBot;

  img2buf (p->imgUV[0],
           (decodedPicture->valid == 1) ? decodedPicture->uBuf : decodedPicture->uBuf + chromaSizeX * symbolSizeInBytes,
           p->sizeXcr, p->sizeYcr, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPicture->uvStride);

  img2buf (p->imgUV[1],
           (decodedPicture->valid == 1) ? decodedPicture->vBuf : decodedPicture->vBuf + chromaSizeX * symbolSizeInBytes,
           p->sizeXcr, p->sizeYcr, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPicture->uvStride);
  }
//}}}
//{{{
static void flushPendingOut (sVidParam* vidParam) {

  if (vidParam->pendingOutState != FRAME)
    writeOutPicture (vidParam, vidParam->pendingOut);

  if (vidParam->pendingOut->imgY) {
    free_mem2Dpel (vidParam->pendingOut->imgY);
    vidParam->pendingOut->imgY = NULL;
    }

  if (vidParam->pendingOut->imgUV) {
    free_mem3Dpel (vidParam->pendingOut->imgUV);
    vidParam->pendingOut->imgUV = NULL;
    }

  vidParam->pendingOutState = FRAME;
  }
//}}}

//{{{
static void writePicture (sVidParam* vidParam, sPicture* p, int real_structure) {

  if (real_structure == FRAME) {
    flushPendingOut (vidParam);
    writeOutPicture (vidParam, p);
    return;
    }

  if (real_structure == vidParam->pendingOutState) {
    flushPendingOut (vidParam);
    writePicture (vidParam, p, real_structure);
    return;
    }

  if (vidParam->pendingOutState == FRAME) {
    //{{{  output frame
    vidParam->pendingOut->sizeX = p->sizeX;
    vidParam->pendingOut->sizeY = p->sizeY;
    vidParam->pendingOut->sizeXcr = p->sizeXcr;
    vidParam->pendingOut->sizeYcr = p->sizeYcr;
    vidParam->pendingOut->chromaFormatIdc = p->chromaFormatIdc;

    vidParam->pendingOut->frameMbOnlyFlag = p->frameMbOnlyFlag;
    vidParam->pendingOut->frameCropFlag = p->frameCropFlag;
    if (vidParam->pendingOut->frameCropFlag) {
      vidParam->pendingOut->frameCropLeft = p->frameCropLeft;
      vidParam->pendingOut->frameCropRight = p->frameCropRight;
      vidParam->pendingOut->frameCropTop = p->frameCropTop;
      vidParam->pendingOut->frameCropBot = p->frameCropBot;
      }

    get_mem2Dpel (&(vidParam->pendingOut->imgY), vidParam->pendingOut->sizeY, vidParam->pendingOut->sizeX);
    get_mem3Dpel (&(vidParam->pendingOut->imgUV), 2, vidParam->pendingOut->sizeYcr, vidParam->pendingOut->sizeXcr);
    clearPicture (vidParam, vidParam->pendingOut);

    // copy first field
    int add = (real_structure == TopField) ? 0 : 1;
    for (int i = 0; i < vidParam->pendingOut->sizeY; i += 2)
      memcpy (vidParam->pendingOut->imgY[(i+add)], p->imgY[(i+add)], p->sizeX * sizeof(sPixel));
    for (int i = 0; i < vidParam->pendingOut->sizeYcr; i += 2) {
      memcpy (vidParam->pendingOut->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->sizeXcr * sizeof(sPixel));
      memcpy (vidParam->pendingOut->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->sizeXcr * sizeof(sPixel));
      }

    vidParam->pendingOutState = real_structure;
    }
    //}}}
  else {
    if ((vidParam->pendingOut->sizeX!=p->sizeX) ||
        (vidParam->pendingOut->sizeY!= p->sizeY) ||
        (vidParam->pendingOut->frameMbOnlyFlag != p->frameMbOnlyFlag) ||
        (vidParam->pendingOut->frameCropFlag != p->frameCropFlag) ||
        (vidParam->pendingOut->frameCropFlag &&
         ((vidParam->pendingOut->frameCropLeft != p->frameCropLeft) ||
          (vidParam->pendingOut->frameCropRight != p->frameCropRight) ||
          (vidParam->pendingOut->frameCropTop != p->frameCropTop) ||
          (vidParam->pendingOut->frameCropBot != p->frameCropBot)))) {
      flushPendingOut (vidParam);
      writePicture (vidParam, p, real_structure);
      return;
      }

    // copy second field
    int add = (real_structure == TopField) ? 0 : 1;
    for (int i = 0; i < vidParam->pendingOut->sizeY; i+=2)
      memcpy (vidParam->pendingOut->imgY[(i+add)], p->imgY[(i+add)], p->sizeX * sizeof(sPixel));

    for (int i = 0; i < vidParam->pendingOut->sizeYcr; i+=2) {
      memcpy (vidParam->pendingOut->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->sizeXcr * sizeof(sPixel));
      memcpy (vidParam->pendingOut->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->sizeXcr * sizeof(sPixel));
      }

    flushPendingOut (vidParam);
    }
  }
//}}}
//{{{
static void writeUnpairedField (sVidParam* vidParam, sFrameStore* frameStore) {

  assert (frameStore->isUsed < 3);

  if (frameStore->isUsed & 0x01) {
    // we have a top field, construct an empty bottom field
    sPicture* p = frameStore->topField;
    frameStore->botField = allocPicture (vidParam, BotField,
                                         p->sizeX, 2 * p->sizeY, p->sizeXcr, 2 * p->sizeYcr, 1);
    frameStore->botField->chromaFormatIdc = p->chromaFormatIdc;

    clearPicture (vidParam, frameStore->botField);
    dpbCombineField (vidParam, frameStore);
    writePicture (vidParam, frameStore->frame, TopField);
    }

  if (frameStore->isUsed & 0x02) {
    // we have a bottom field, construct an empty top field
    sPicture* p = frameStore->botField;
    frameStore->topField = allocPicture (vidParam, TopField, p->sizeX, 2*p->sizeY, p->sizeXcr, 2*p->sizeYcr, 1);
    frameStore->topField->chromaFormatIdc = p->chromaFormatIdc;
    clearPicture (vidParam, frameStore->topField);

    frameStore->topField->frameCropFlag = frameStore->botField->frameCropFlag;
    if (frameStore->topField->frameCropFlag) {
      frameStore->topField->frameCropTop = frameStore->botField->frameCropTop;
      frameStore->topField->frameCropBot = frameStore->botField->frameCropBot;
      frameStore->topField->frameCropLeft = frameStore->botField->frameCropLeft;
      frameStore->topField->frameCropRight = frameStore->botField->frameCropRight;
      }

    dpbCombineField (vidParam, frameStore);
    writePicture (vidParam, frameStore->frame, BotField);
    }

  frameStore->isUsed = 3;
  }
//}}}
//{{{
static void flushDirectOutput (sVidParam* vidParam) {

  writeUnpairedField (vidParam, vidParam->outBuffer);
  freePicture (vidParam->outBuffer->frame);

  vidParam->outBuffer->frame = NULL;
  freePicture (vidParam->outBuffer->topField);

  vidParam->outBuffer->topField = NULL;
  freePicture (vidParam->outBuffer->botField);

  vidParam->outBuffer->botField = NULL;
  vidParam->outBuffer->isUsed = 0;
  }
//}}}

//{{{
void allocOutput (sVidParam* vidParam) {

  vidParam->outBuffer = allocFrameStore();
  vidParam->pendingOut = calloc (sizeof(sPicture), 1);
  vidParam->pendingOut->imgUV = NULL;
  vidParam->pendingOut->imgY = NULL;
  }
//}}}
//{{{
void freeOutput (sVidParam* vidParam) {

  freeFrameStore (vidParam->outBuffer);
  vidParam->outBuffer = NULL;

  flushPendingOut (vidParam);
  free (vidParam->pendingOut);
  }
//}}}

//{{{
void directOutput (sVidParam* vidParam, sPicture* picture) {

  if (picture->structure == FRAME) {
    // we have a frame (or complementary field pair), so output it directly
    flushDirectOutput (vidParam);
    writePicture (vidParam, picture, FRAME);
    calcFrameNum (vidParam, picture);
    freePicture (picture);
    return;
    }

  if (picture->structure == TopField) {
    if (vidParam->outBuffer->isUsed & 1)
      flushDirectOutput (vidParam);
    vidParam->outBuffer->topField = picture;
    vidParam->outBuffer->isUsed |= 1;
    }

  if (picture->structure == BotField) {
    if (vidParam->outBuffer->isUsed & 2)
      flushDirectOutput (vidParam);
    vidParam->outBuffer->botField = picture;
    vidParam->outBuffer->isUsed |= 2;
    }

  if (vidParam->outBuffer->isUsed == 3) {
    // we have both fields, so output them
    dpbCombineField (vidParam, vidParam->outBuffer);
    writePicture (vidParam, vidParam->outBuffer->frame, FRAME);

    calcFrameNum (vidParam, picture);
    freePicture (vidParam->outBuffer->frame);

    vidParam->outBuffer->frame = NULL;
    freePicture (vidParam->outBuffer->topField);

    vidParam->outBuffer->topField = NULL;
    freePicture (vidParam->outBuffer->botField);

    vidParam->outBuffer->botField = NULL;
    vidParam->outBuffer->isUsed = 0;
    }
  }
//}}}
//{{{
void writeStoredFrame (sVidParam* vidParam, sFrameStore* frameStore) {

  // make sure no direct output field is pending
  flushDirectOutput (vidParam);

  if (frameStore->isUsed < 3)
    writeUnpairedField (vidParam, frameStore);
  else {
    if (frameStore->recoveryFrame)
      vidParam->recoveryFlag = 1;
    if ((!vidParam->nonConformingStream) || vidParam->recoveryFlag)
      writePicture (vidParam, frameStore->frame, FRAME);
    }

  frameStore->is_output = 1;
  }
//}}}
