//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
//}}}

//{{{
static void allocDecodedPicBuffers (sDecoder* decoder, sDecodedPic* decodedPic, sPicture* p,
                                    int lumaSize, int frameSize, int lumaSizeX, int lumaSizeY,
                                    int chromaSizeX, int chromaSizeY) {

  if (decodedPic->bufSize != frameSize) {
    memFree (decodedPic->yBuf);

    decodedPic->bufSize = frameSize;
    decodedPic->yBuf = memAlloc (decodedPic->bufSize);
    decodedPic->uBuf = decodedPic->yBuf + lumaSize;
    decodedPic->vBuf = decodedPic->uBuf + ((frameSize - lumaSize)>>1);

    decodedPic->yuvFormat = p->chromaFormatIdc;
    decodedPic->bitDepth = decoder->coding.picUnitBitSizeDisk;
    decodedPic->width = lumaSizeX;
    decodedPic->height = lumaSizeY;

    int symbolSizeInBytes = (decoder->coding.picUnitBitSizeDisk + 7) >> 3;
    decodedPic->yStride = lumaSizeX * symbolSizeInBytes;
    decodedPic->uvStride = chromaSizeX * symbolSizeInBytes;
    }
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
      p->imgY[i][j] = (sPixel)decoder->coding.dcPredValueComp[0];

  for (int i = 0; i < p->sizeYcr;i++)
    for (int j = 0; j < p->sizeXcr; j++)
      p->imgUV[0][i][j] = (sPixel)decoder->coding.dcPredValueComp[1];

  for (int i = 0; i < p->sizeYcr;i++)
    for (int j = 0; j < p->sizeXcr; j++)
      p->imgUV[1][i][j] = (sPixel)decoder->coding.dcPredValueComp[2];
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
  if (p->cropFlag) {
    cropLeft = SubWidthC [p->chromaFormatIdc] * p->cropLeft;
    cropRight = SubWidthC [p->chromaFormatIdc] * p->cropRight;
    cropTop = SubHeightC[p->chromaFormatIdc] * ( 2 - p->frameMbOnly ) * p->cropTop;
    cropBottom = SubHeightC[p->chromaFormatIdc] * ( 2 - p->frameMbOnly ) * p->cropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBottom = 0;

  int symbolSizeInBytes = (decoder->coding.picUnitBitSizeDisk+7) >> 3;
  int chromaSizeX =  p->sizeXcr- p->cropLeft -p->cropRight;
  int chromaSizeY = p->sizeYcr - ( 2 - p->frameMbOnly ) * p->cropTop -( 2 - p->frameMbOnly ) * p->cropBot;
  int lumaSizeX = p->sizeX - cropLeft - cropRight;
  int lumaSizeY = p->sizeY - cropTop - cropBottom;
  int lumaSize = lumaSizeX * lumaSizeY * symbolSizeInBytes;
  int frameSize = (lumaSizeX * lumaSizeY + 2 * (chromaSizeX * chromaSizeY)) * symbolSizeInBytes;

  sDecodedPic* decodedPic = allocDecodedPicture (decoder->decOutputPic);
  if (!decodedPic->yBuf || (decodedPic->bufSize < frameSize))
    allocDecodedPicBuffers (decoder, decodedPic, p, lumaSize, frameSize, lumaSizeX, lumaSizeY, chromaSizeX, chromaSizeY);
  decodedPic->valid = 1;
  decodedPic->poc = p->framePoc;
  if (!decodedPic->yBuf)
    noMemoryExit ("writeOutPicture: buf");

  img2buf (p->imgY,
           (decodedPic->valid == 1) ? decodedPic->yBuf : decodedPic->yBuf + lumaSizeX * symbolSizeInBytes,
           p->sizeX, p->sizeY, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPic->yStride);

  cropLeft = p->cropLeft;
  cropRight = p->cropRight;
  cropTop = (2 - p->frameMbOnly) * p->cropTop;
  cropBottom = (2 - p->frameMbOnly) * p->cropBot;

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

  if (decoder->pendingOutState != eFrame)
    writeOutPicture (decoder, decoder->pendingOut);

  if (decoder->pendingOut->imgY) {
    freeMem2Dpel (decoder->pendingOut->imgY);
    decoder->pendingOut->imgY = NULL;
    }

  if (decoder->pendingOut->imgUV) {
    freeMem3Dpel (decoder->pendingOut->imgUV);
    decoder->pendingOut->imgUV = NULL;
    }

  decoder->pendingOutState = eFrame;
  }
//}}}

//{{{
static void writePicture (sDecoder* decoder, sPicture* p, int realStructure) {

  if (realStructure == eFrame) {
    flushPendingOut (decoder);
    writeOutPicture (decoder, p);
    return;
    }

  if (realStructure == decoder->pendingOutState) {
    flushPendingOut (decoder);
    writePicture (decoder, p, realStructure);
    return;
    }

  if (decoder->pendingOutState == eFrame) {
    //{{{  output frame
    decoder->pendingOut->sizeX = p->sizeX;
    decoder->pendingOut->sizeY = p->sizeY;
    decoder->pendingOut->sizeXcr = p->sizeXcr;
    decoder->pendingOut->sizeYcr = p->sizeYcr;
    decoder->pendingOut->chromaFormatIdc = p->chromaFormatIdc;

    decoder->pendingOut->frameMbOnly = p->frameMbOnly;
    decoder->pendingOut->cropFlag = p->cropFlag;
    if (decoder->pendingOut->cropFlag) {
      decoder->pendingOut->cropLeft = p->cropLeft;
      decoder->pendingOut->cropRight = p->cropRight;
      decoder->pendingOut->cropTop = p->cropTop;
      decoder->pendingOut->cropBot = p->cropBot;
      }

    getMem2Dpel (&(decoder->pendingOut->imgY), decoder->pendingOut->sizeY, decoder->pendingOut->sizeX);
    getMem3Dpel (&(decoder->pendingOut->imgUV), 2, decoder->pendingOut->sizeYcr, decoder->pendingOut->sizeXcr);
    clearPicture (decoder, decoder->pendingOut);

    // copy first field
    int add = (realStructure == eTopField) ? 0 : 1;
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
        (decoder->pendingOut->frameMbOnly != p->frameMbOnly) ||
        (decoder->pendingOut->cropFlag != p->cropFlag) ||
        (decoder->pendingOut->cropFlag &&
         ((decoder->pendingOut->cropLeft != p->cropLeft) ||
          (decoder->pendingOut->cropRight != p->cropRight) ||
          (decoder->pendingOut->cropTop != p->cropTop) ||
          (decoder->pendingOut->cropBot != p->cropBot)))) {
      flushPendingOut (decoder);
      writePicture (decoder, p, realStructure);
      return;
      }

    // copy second field
    int add = (realStructure == eTopField) ? 0 : 1;
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
    frameStore->botField = allocPicture (decoder, eBotField,
                                         p->sizeX, 2 * p->sizeY, p->sizeXcr, 2 * p->sizeYcr, 1);
    frameStore->botField->chromaFormatIdc = p->chromaFormatIdc;

    clearPicture (decoder, frameStore->botField);
    dpbCombineField (decoder, frameStore);
    writePicture (decoder, frameStore->frame, eTopField);
    }

  if (frameStore->isUsed & 0x02) {
    // we have a bottom field, construct an empty top field
    sPicture* p = frameStore->botField;
    frameStore->topField = allocPicture (decoder, eTopField, p->sizeX, 2*p->sizeY, p->sizeXcr, 2*p->sizeYcr, 1);
    frameStore->topField->chromaFormatIdc = p->chromaFormatIdc;
    clearPicture (decoder, frameStore->topField);

    frameStore->topField->cropFlag = frameStore->botField->cropFlag;
    if (frameStore->topField->cropFlag) {
      frameStore->topField->cropTop = frameStore->botField->cropTop;
      frameStore->topField->cropBot = frameStore->botField->cropBot;
      frameStore->topField->cropLeft = frameStore->botField->cropLeft;
      frameStore->topField->cropRight = frameStore->botField->cropRight;
      }

    dpbCombineField (decoder, frameStore);
    writePicture (decoder, frameStore->frame, eBotField);
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
void directOutput (sDecoder* decoder, sPicture* picture) {

  if (picture->picStructure == eFrame) {
    // we have a frame (or complementary field pair), so output it directly
    flushDirectOutput (decoder);
    writePicture (decoder, picture, eFrame);
    freePicture (picture);
    return;
    }

  if (picture->picStructure == eTopField) {
    if (decoder->outBuffer->isUsed & 1)
      flushDirectOutput (decoder);
    decoder->outBuffer->topField = picture;
    decoder->outBuffer->isUsed |= 1;
    }

  if (picture->picStructure == eBotField) {
    if (decoder->outBuffer->isUsed & 2)
      flushDirectOutput (decoder);
    decoder->outBuffer->botField = picture;
    decoder->outBuffer->isUsed |= 2;
    }

  if (decoder->outBuffer->isUsed == 3) {
    // we have both fields, so output them
    dpbCombineField (decoder, decoder->outBuffer);
    writePicture (decoder, decoder->outBuffer->frame, eFrame);
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
      writePicture (decoder, frameStore->frame, eFrame);
    }

  frameStore->is_output = 1;
  }
//}}}
