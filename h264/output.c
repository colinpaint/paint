//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
//}}}

//{{{
static void allocDecodedPic (sDecoder* decoder, sDecodedPic* decodedPic, sPicture* p,
                             int lumaSize, int frameSize, int lumaSizeX, int lumaSizeY,
                             int chromaSizeX, int chromaSizeY) {

  int symbolSizeInBytes = (decoder->picUnitBitSizeDisk + 7) >> 3;

  if (decodedPic->yBuf)
    memFree (decodedPic->yBuf);

  decodedPic->bufSize = frameSize;
  decodedPic->yBuf = memAlloc (decodedPic->bufSize);
  decodedPic->uBuf = decodedPic->yBuf + lumaSize;
  decodedPic->vBuf = decodedPic->uBuf + ((frameSize - lumaSize)>>1);

  decodedPic->yuvFormat = p->chromaFormatIdc;
  decodedPic->bitDepth = decoder->picUnitBitSizeDisk;
  decodedPic->width = lumaSizeX;
  decodedPic->height = lumaSizeY;
  decodedPic->yStride = lumaSizeX * symbolSizeInBytes;
  decodedPic->uvStride = chromaSizeX * symbolSizeInBytes;
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
static void clearPicture (sDecoder* decoder, sPicture* p) {

  printf ("-------------------- clearPicture -----------------\n");

  for (int i = 0; i < p->sizeY; i++)
    for (int j = 0; j < p->sizeX; j++)
      p->imgY[i][j] = (sPixel)decoder->dcPredValueComp[0];

  for (int i = 0; i < p->sizeYcr;i++)
    for (int j = 0; j < p->sizeXcr; j++)
      p->imgUV[0][i][j] = (sPixel)decoder->dcPredValueComp[1];

  for (int i = 0; i < p->sizeYcr;i++)
    for (int j = 0; j < p->sizeXcr; j++)
      p->imgUV[1][i][j] = (sPixel)decoder->dcPredValueComp[2];
  }
//}}}
//{{{
static void writeOutPicture (sDecoder* decoder, sPicture* p) {

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

  int symbolSizeInBytes = ((decoder->picUnitBitSizeDisk+7) >> 3);
  int chromaSizeX =  p->sizeXcr- p->frameCropLeft -p->frameCropRight;
  int chromaSizeY = p->sizeYcr - ( 2 - p->frameMbOnlyFlag ) * p->frameCropTop -( 2 - p->frameMbOnlyFlag ) * p->frameCropBot;
  int lumaSizeX = p->sizeX - cropLeft - cropRight;
  int lumaSizeY = p->sizeY - cropTop - cropBottom;
  int lumaSize = lumaSizeX * lumaSizeY * symbolSizeInBytes;
  int frameSize = (lumaSizeX * lumaSizeY + 2 * (chromaSizeX * chromaSizeY)) * symbolSizeInBytes;

  sDecodedPic* decodedPic = getDecodedPicture (decoder->decOutputPic);
  if (!decodedPic->yBuf || (decodedPic->bufSize < frameSize))
    allocDecodedPic (decoder, decodedPic, p, lumaSize, frameSize, lumaSizeX, lumaSizeY, chromaSizeX, chromaSizeY);
  decodedPic->valid = 1;
  decodedPic->poc = p->framePoc;
  if (!decodedPic->yBuf)
    no_mem_exit ("writeOutPicture: buf");

  img2buf (p->imgY,
           (decodedPic->valid == 1) ? decodedPic->yBuf : decodedPic->yBuf + lumaSizeX * symbolSizeInBytes,
           p->sizeX, p->sizeY, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPic->yStride);

  cropLeft = p->frameCropLeft;
  cropRight = p->frameCropRight;
  cropTop = (2 - p->frameMbOnlyFlag) * p->frameCropTop;
  cropBottom = (2 - p->frameMbOnlyFlag) * p->frameCropBot;

  img2buf (p->imgUV[0],
           (decodedPic->valid == 1) ? decodedPic->uBuf : decodedPic->uBuf + chromaSizeX * symbolSizeInBytes,
           p->sizeXcr, p->sizeYcr, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPic->uvStride);

  img2buf (p->imgUV[1],
           (decodedPic->valid == 1) ? decodedPic->vBuf : decodedPic->vBuf + chromaSizeX * symbolSizeInBytes,
           p->sizeXcr, p->sizeYcr, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPic->uvStride);
  }
//}}}
//{{{
static void flushPendingOut (sDecoder* decoder) {

  if (decoder->pendingOutState != FRAME)
    writeOutPicture (decoder, decoder->pendingOut);

  if (decoder->pendingOut->imgY) {
    free_mem2Dpel (decoder->pendingOut->imgY);
    decoder->pendingOut->imgY = NULL;
    }

  if (decoder->pendingOut->imgUV) {
    free_mem3Dpel (decoder->pendingOut->imgUV);
    decoder->pendingOut->imgUV = NULL;
    }

  decoder->pendingOutState = FRAME;
  }
//}}}

//{{{
static void writePicture (sDecoder* decoder, sPicture* p, int realStructure) {

  if (realStructure == FRAME) {
    flushPendingOut (decoder);
    writeOutPicture (decoder, p);
    return;
    }

  if (realStructure == decoder->pendingOutState) {
    flushPendingOut (decoder);
    writePicture (decoder, p, realStructure);
    return;
    }

  if (decoder->pendingOutState == FRAME) {
    //{{{  output frame
    decoder->pendingOut->sizeX = p->sizeX;
    decoder->pendingOut->sizeY = p->sizeY;
    decoder->pendingOut->sizeXcr = p->sizeXcr;
    decoder->pendingOut->sizeYcr = p->sizeYcr;
    decoder->pendingOut->chromaFormatIdc = p->chromaFormatIdc;

    decoder->pendingOut->frameMbOnlyFlag = p->frameMbOnlyFlag;
    decoder->pendingOut->frameCropFlag = p->frameCropFlag;
    if (decoder->pendingOut->frameCropFlag) {
      decoder->pendingOut->frameCropLeft = p->frameCropLeft;
      decoder->pendingOut->frameCropRight = p->frameCropRight;
      decoder->pendingOut->frameCropTop = p->frameCropTop;
      decoder->pendingOut->frameCropBot = p->frameCropBot;
      }

    get_mem2Dpel (&(decoder->pendingOut->imgY), decoder->pendingOut->sizeY, decoder->pendingOut->sizeX);
    get_mem3Dpel (&(decoder->pendingOut->imgUV), 2, decoder->pendingOut->sizeYcr, decoder->pendingOut->sizeXcr);
    clearPicture (decoder, decoder->pendingOut);

    // copy first field
    int add = (realStructure == TopField) ? 0 : 1;
    for (int i = 0; i < decoder->pendingOut->sizeY; i += 2)
      memcpy (decoder->pendingOut->imgY[(i+add)], p->imgY[(i+add)], p->sizeX * sizeof(sPixel));
    for (int i = 0; i < decoder->pendingOut->sizeYcr; i += 2) {
      memcpy (decoder->pendingOut->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->sizeXcr * sizeof(sPixel));
      memcpy (decoder->pendingOut->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->sizeXcr * sizeof(sPixel));
      }

    decoder->pendingOutState = realStructure;
    }
    //}}}
  else {
    if ((decoder->pendingOut->sizeX!=p->sizeX) ||
        (decoder->pendingOut->sizeY!= p->sizeY) ||
        (decoder->pendingOut->frameMbOnlyFlag != p->frameMbOnlyFlag) ||
        (decoder->pendingOut->frameCropFlag != p->frameCropFlag) ||
        (decoder->pendingOut->frameCropFlag &&
         ((decoder->pendingOut->frameCropLeft != p->frameCropLeft) ||
          (decoder->pendingOut->frameCropRight != p->frameCropRight) ||
          (decoder->pendingOut->frameCropTop != p->frameCropTop) ||
          (decoder->pendingOut->frameCropBot != p->frameCropBot)))) {
      flushPendingOut (decoder);
      writePicture (decoder, p, realStructure);
      return;
      }

    // copy second field
    int add = (realStructure == TopField) ? 0 : 1;
    for (int i = 0; i < decoder->pendingOut->sizeY; i+=2)
      memcpy (decoder->pendingOut->imgY[(i+add)], p->imgY[(i+add)], p->sizeX * sizeof(sPixel));

    for (int i = 0; i < decoder->pendingOut->sizeYcr; i+=2) {
      memcpy (decoder->pendingOut->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->sizeXcr * sizeof(sPixel));
      memcpy (decoder->pendingOut->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->sizeXcr * sizeof(sPixel));
      }

    flushPendingOut (decoder);
    }
  }
//}}}
//{{{
static void writeUnpairedField (sDecoder* decoder, sFrameStore* frameStore) {

  assert (frameStore->isUsed < 3);

  if (frameStore->isUsed & 0x01) {
    // we have a top field, construct an empty bottom field
    sPicture* p = frameStore->topField;
    frameStore->botField = allocPicture (decoder, BotField,
                                         p->sizeX, 2 * p->sizeY, p->sizeXcr, 2 * p->sizeYcr, 1);
    frameStore->botField->chromaFormatIdc = p->chromaFormatIdc;

    clearPicture (decoder, frameStore->botField);
    dpbCombineField (decoder, frameStore);
    writePicture (decoder, frameStore->frame, TopField);
    }

  if (frameStore->isUsed & 0x02) {
    // we have a bottom field, construct an empty top field
    sPicture* p = frameStore->botField;
    frameStore->topField = allocPicture (decoder, TopField, p->sizeX, 2*p->sizeY, p->sizeXcr, 2*p->sizeYcr, 1);
    frameStore->topField->chromaFormatIdc = p->chromaFormatIdc;
    clearPicture (decoder, frameStore->topField);

    frameStore->topField->frameCropFlag = frameStore->botField->frameCropFlag;
    if (frameStore->topField->frameCropFlag) {
      frameStore->topField->frameCropTop = frameStore->botField->frameCropTop;
      frameStore->topField->frameCropBot = frameStore->botField->frameCropBot;
      frameStore->topField->frameCropLeft = frameStore->botField->frameCropLeft;
      frameStore->topField->frameCropRight = frameStore->botField->frameCropRight;
      }

    dpbCombineField (decoder, frameStore);
    writePicture (decoder, frameStore->frame, BotField);
    }

  frameStore->isUsed = 3;
  }
//}}}
//{{{
static void flushDirectOutput (sDecoder* decoder) {

  writeUnpairedField (decoder, decoder->outBuffer);
  freePicture (decoder->outBuffer->frame);

  decoder->outBuffer->frame = NULL;
  freePicture (decoder->outBuffer->topField);

  decoder->outBuffer->topField = NULL;
  freePicture (decoder->outBuffer->botField);

  decoder->outBuffer->botField = NULL;
  decoder->outBuffer->isUsed = 0;
  }
//}}}

//{{{
void allocOutput (sDecoder* decoder) {

  decoder->outBuffer = allocFrameStore();
  decoder->pendingOut = calloc (sizeof(sPicture), 1);
  decoder->pendingOut->imgUV = NULL;
  decoder->pendingOut->imgY = NULL;
  }
//}}}
//{{{
void freeOutput (sDecoder* decoder) {

  freeFrameStore (decoder->outBuffer);
  decoder->outBuffer = NULL;

  flushPendingOut (decoder);
  free (decoder->pendingOut);
  }
//}}}

//{{{
void calcFrameNum (sDecoder* decoder, sPicture* picture) {

  int psnrPOC = decoder->activeSPS->mb_adaptive_frame_field_flag ? picture->poc / (decoder->param.pocScale) :
                                                                    picture->poc / (decoder->param.pocScale);
  if (psnrPOC == 0)
    decoder->idrPsnrNum = decoder->gapNumFrame * decoder->refPocGap / (decoder->param.pocScale);

  decoder->frameNum = decoder->idrPsnrNum + psnrPOC;
  }
//}}}
//{{{
void directOutput (sDecoder* decoder, sPicture* picture) {

  if (picture->structure == FRAME) {
    // we have a frame (or complementary field pair), so output it directly
    flushDirectOutput (decoder);
    writePicture (decoder, picture, FRAME);
    calcFrameNum (decoder, picture);
    freePicture (picture);
    return;
    }

  if (picture->structure == TopField) {
    if (decoder->outBuffer->isUsed & 1)
      flushDirectOutput (decoder);
    decoder->outBuffer->topField = picture;
    decoder->outBuffer->isUsed |= 1;
    }

  if (picture->structure == BotField) {
    if (decoder->outBuffer->isUsed & 2)
      flushDirectOutput (decoder);
    decoder->outBuffer->botField = picture;
    decoder->outBuffer->isUsed |= 2;
    }

  if (decoder->outBuffer->isUsed == 3) {
    // we have both fields, so output them
    dpbCombineField (decoder, decoder->outBuffer);
    writePicture (decoder, decoder->outBuffer->frame, FRAME);

    calcFrameNum (decoder, picture);
    freePicture (decoder->outBuffer->frame);

    decoder->outBuffer->frame = NULL;
    freePicture (decoder->outBuffer->topField);

    decoder->outBuffer->topField = NULL;
    freePicture (decoder->outBuffer->botField);

    decoder->outBuffer->botField = NULL;
    decoder->outBuffer->isUsed = 0;
    }
  }
//}}}
//{{{
void writeStoredFrame (sDecoder* decoder, sFrameStore* frameStore) {

  // make sure no direct output field is pending
  flushDirectOutput (decoder);

  if (frameStore->isUsed < 3)
    writeUnpairedField (decoder, frameStore);
  else {
    if (frameStore->recoveryFrame)
      decoder->recoveryFlag = 1;
    if ((!decoder->nonConformingStream) || decoder->recoveryFlag)
      writePicture (decoder, frameStore->frame, FRAME);
    }

  frameStore->is_output = 1;
  }
//}}}
