//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
//}}}

//{{{
static void allocDecodedPic (sVidParam* vidParam, sDecodedPicture* decodedPicture, sPicture* p,
                             int lumaSize, int frameSize, int lumaSizeX, int lumaSizeY,
                             int chromaSizeX, int chromaSizeY) {

  int symbolSizeInBytes = (vidParam->pic_unit_bitsize_on_disk + 7) >> 3;

  if (decodedPicture->yBuf)
    mem_free (decodedPicture->yBuf);

  decodedPicture->bufSize = frameSize;
  decodedPicture->yBuf = mem_malloc (decodedPicture->bufSize);
  decodedPicture->uBuf = decodedPicture->yBuf + lumaSize;
  decodedPicture->vBuf = decodedPicture->uBuf + ((frameSize - lumaSize)>>1);

  decodedPicture->yuvFormat = p->chromaFormatIdc;
  decodedPicture->yuvStorageFormat = 0;
  decodedPicture->iBitDepth = vidParam->pic_unit_bitsize_on_disk;
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

  for (int i = 0; i < p->size_y; i++)
    for (int j = 0; j < p->size_x; j++)
      p->imgY[i][j] = (sPixel)vidParam->dc_pred_value_comp[0];

  for (int i = 0; i < p->size_y_cr;i++)
    for (int j = 0; j < p->size_x_cr; j++)
      p->imgUV[0][i][j] = (sPixel)vidParam->dc_pred_value_comp[1];

  for (int i = 0; i < p->size_y_cr;i++)
    for (int j = 0; j < p->size_x_cr; j++)
      p->imgUV[1][i][j] = (sPixel)vidParam->dc_pred_value_comp[2];
  }
//}}}
//{{{
static void writeOutPicture (sVidParam* vidParam, sPicture* p) {

  static const int SubWidthC  [4]= { 1, 2, 2, 1 };
  static const int SubHeightC [4]= { 1, 2, 1, 1 };

  if (p->non_existing)
    return;

  int cropLeft;
  int cropRight;
  int cropTop;
  int cropBottom;
  if (p->frame_cropping_flag) {
    cropLeft = SubWidthC [p->chromaFormatIdc] * p->frame_crop_left_offset;
    cropRight = SubWidthC [p->chromaFormatIdc] * p->frame_crop_right_offset;
    cropTop = SubHeightC[p->chromaFormatIdc] * ( 2 - p->frame_mbs_only_flag ) * p->frame_crop_top_offset;
    cropBottom = SubHeightC[p->chromaFormatIdc] * ( 2 - p->frame_mbs_only_flag ) * p->frame_crop_bottom_offset;
    }
  else
    cropLeft = cropRight = cropTop = cropBottom = 0;

  int symbolSizeInBytes = ((vidParam->pic_unit_bitsize_on_disk+7) >> 3);
  int chromaSizeX =  p->size_x_cr- p->frame_crop_left_offset -p->frame_crop_right_offset;
  int chromaSizeY = p->size_y_cr - ( 2 - p->frame_mbs_only_flag ) * p->frame_crop_top_offset -( 2 - p->frame_mbs_only_flag ) * p->frame_crop_bottom_offset;
  int lumaSizeX = p->size_x - cropLeft - cropRight;
  int lumaSizeY = p->size_y - cropTop - cropBottom;
  int lumaSize = lumaSizeX * lumaSizeY * symbolSizeInBytes;
  int frameSize = (lumaSizeX * lumaSizeY + 2 * (chromaSizeX * chromaSizeY)) * symbolSizeInBytes;

  sDecodedPicture* decodedPicture = getAvailableDecodedPicture (vidParam->decOutputPic);
  if (!decodedPicture->yBuf || (decodedPicture->bufSize < frameSize))
    allocDecodedPic (vidParam, decodedPicture, p, lumaSize, frameSize, lumaSizeX, lumaSizeY, chromaSizeX, chromaSizeY);
  decodedPicture->valid = 1;
  decodedPicture->poc = p->framePoc;
  if (!decodedPicture->yBuf)
    no_mem_exit ("writeOutPicture: buf");

  img2buf (p->imgY,
           (decodedPicture->valid == 1) ? decodedPicture->yBuf : decodedPicture->yBuf + lumaSizeX * symbolSizeInBytes,
           p->size_x, p->size_y, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPicture->yStride);

  cropLeft = p->frame_crop_left_offset;
  cropRight = p->frame_crop_right_offset;
  cropTop = (2 - p->frame_mbs_only_flag) * p->frame_crop_top_offset;
  cropBottom = (2 - p->frame_mbs_only_flag) * p->frame_crop_bottom_offset;

  img2buf (p->imgUV[0],
           (decodedPicture->valid == 1) ? decodedPicture->uBuf : decodedPicture->uBuf + chromaSizeX * symbolSizeInBytes,
           p->size_x_cr, p->size_y_cr, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPicture->uvStride);

  img2buf (p->imgUV[1],
           (decodedPicture->valid == 1) ? decodedPicture->vBuf : decodedPicture->vBuf + chromaSizeX * symbolSizeInBytes,
           p->size_x_cr, p->size_y_cr, symbolSizeInBytes,
           cropLeft, cropRight, cropTop, cropBottom, decodedPicture->uvStride);
  }
//}}}
//{{{
static void flushPendingOutput (sVidParam* vidParam) {

  if (vidParam->pendingOutputState != FRAME)
    writeOutPicture (vidParam, vidParam->pendingOutput);

  if (vidParam->pendingOutput->imgY) {
    free_mem2Dpel (vidParam->pendingOutput->imgY);
    vidParam->pendingOutput->imgY = NULL;
    }

  if (vidParam->pendingOutput->imgUV) {
    free_mem3Dpel (vidParam->pendingOutput->imgUV);
    vidParam->pendingOutput->imgUV = NULL;
    }

  vidParam->pendingOutputState = FRAME;
  }
//}}}
//{{{
static void writePicture (sVidParam* vidParam, sPicture* p, int real_structure) {

  if (real_structure == FRAME) {
    flushPendingOutput (vidParam);
    writeOutPicture (vidParam, p);
    return;
    }

  if (real_structure == vidParam->pendingOutputState) {
    flushPendingOutput (vidParam);
    writePicture (vidParam, p, real_structure);
    return;
    }

  if (vidParam->pendingOutputState == FRAME) {
    //{{{  output frame
    vidParam->pendingOutput->size_x = p->size_x;
    vidParam->pendingOutput->size_y = p->size_y;
    vidParam->pendingOutput->size_x_cr = p->size_x_cr;
    vidParam->pendingOutput->size_y_cr = p->size_y_cr;
    vidParam->pendingOutput->chromaFormatIdc = p->chromaFormatIdc;

    vidParam->pendingOutput->frame_mbs_only_flag = p->frame_mbs_only_flag;
    vidParam->pendingOutput->frame_cropping_flag = p->frame_cropping_flag;
    if (vidParam->pendingOutput->frame_cropping_flag) {
      vidParam->pendingOutput->frame_crop_left_offset = p->frame_crop_left_offset;
      vidParam->pendingOutput->frame_crop_right_offset = p->frame_crop_right_offset;
      vidParam->pendingOutput->frame_crop_top_offset = p->frame_crop_top_offset;
      vidParam->pendingOutput->frame_crop_bottom_offset = p->frame_crop_bottom_offset;
      }

    get_mem2Dpel (&(vidParam->pendingOutput->imgY), vidParam->pendingOutput->size_y, vidParam->pendingOutput->size_x);
    get_mem3Dpel (&(vidParam->pendingOutput->imgUV), 2, vidParam->pendingOutput->size_y_cr, vidParam->pendingOutput->size_x_cr);

    clearPicture (vidParam, vidParam->pendingOutput);

    // copy first field
    int add = (real_structure == TOP_FIELD) ? 0 : 1;
    for (int i = 0; i < vidParam->pendingOutput->size_y; i += 2)
      memcpy (vidParam->pendingOutput->imgY[(i+add)], p->imgY[(i+add)], p->size_x * sizeof(sPixel));
    for (int i = 0; i < vidParam->pendingOutput->size_y_cr; i += 2) {
      memcpy (vidParam->pendingOutput->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->size_x_cr * sizeof(sPixel));
      memcpy (vidParam->pendingOutput->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->size_x_cr * sizeof(sPixel));
      }

    vidParam->pendingOutputState = real_structure;
    }
    //}}}
  else {
    if ((vidParam->pendingOutput->size_x!=p->size_x) ||
        (vidParam->pendingOutput->size_y!= p->size_y) ||
        (vidParam->pendingOutput->frame_mbs_only_flag != p->frame_mbs_only_flag) ||
        (vidParam->pendingOutput->frame_cropping_flag != p->frame_cropping_flag) ||
        (vidParam->pendingOutput->frame_cropping_flag &&
         ((vidParam->pendingOutput->frame_crop_left_offset != p->frame_crop_left_offset) ||
          (vidParam->pendingOutput->frame_crop_right_offset != p->frame_crop_right_offset) ||
          (vidParam->pendingOutput->frame_crop_top_offset != p->frame_crop_top_offset) ||
          (vidParam->pendingOutput->frame_crop_bottom_offset != p->frame_crop_bottom_offset)))) {
      flushPendingOutput (vidParam);
      writePicture (vidParam, p, real_structure);
      return;
      }

    // copy second field
    int add = (real_structure == TOP_FIELD) ? 0 : 1;
    for (int i = 0; i < vidParam->pendingOutput->size_y; i+=2)
      memcpy (vidParam->pendingOutput->imgY[(i+add)], p->imgY[(i+add)], p->size_x * sizeof(sPixel));

    for (int i = 0; i < vidParam->pendingOutput->size_y_cr; i+=2) {
      memcpy (vidParam->pendingOutput->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->size_x_cr * sizeof(sPixel));
      memcpy (vidParam->pendingOutput->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->size_x_cr * sizeof(sPixel));
      }

    flushPendingOutput (vidParam);
    }
  }
//}}}
//{{{
static void writeUnpairedField (sVidParam* vidParam, sFrameStore* frameStore) {

  assert (frameStore->is_used < 3);

  if (frameStore->is_used & 0x01) {
    // we have a top field, construct an empty bottom field
    sPicture* p = frameStore->top_field;
    frameStore->bottom_field = allocPicture (vidParam, BOTTOM_FIELD, 
                                             p->size_x, 2 * p->size_y, p->size_x_cr, 2 * p->size_y_cr, 1);
    frameStore->bottom_field->chromaFormatIdc = p->chromaFormatIdc;

    clearPicture (vidParam, frameStore->bottom_field);
    dpb_combine_field_yuv (vidParam, frameStore);

    writePicture (vidParam, frameStore->frame, TOP_FIELD);
    }

  if (frameStore->is_used & 0x02) {
    // we have a bottom field, construct an empty top field
    sPicture* p = frameStore->bottom_field;
    frameStore->top_field = allocPicture (vidParam, TOP_FIELD, p->size_x, 2*p->size_y, p->size_x_cr, 2*p->size_y_cr, 1);
    frameStore->top_field->chromaFormatIdc = p->chromaFormatIdc;
    clearPicture (vidParam, frameStore->top_field);

    frameStore ->top_field->frame_cropping_flag = frameStore->bottom_field->frame_cropping_flag;
    if(frameStore->top_field->frame_cropping_flag) {
      frameStore->top_field->frame_crop_top_offset = frameStore->bottom_field->frame_crop_top_offset;
      frameStore->top_field->frame_crop_bottom_offset = frameStore->bottom_field->frame_crop_bottom_offset;
      frameStore->top_field->frame_crop_left_offset = frameStore->bottom_field->frame_crop_left_offset;
      frameStore->top_field->frame_crop_right_offset = frameStore->bottom_field->frame_crop_right_offset;
      }
    dpb_combine_field_yuv (vidParam, frameStore);

    writePicture (vidParam, frameStore->frame, BOTTOM_FIELD);
    }

  frameStore->is_used = 3;
  }
//}}}
//{{{
static void flushDirectOutput (sVidParam* vidParam) {

  writeUnpairedField (vidParam, vidParam->outBuffer);
  freePicture (vidParam->outBuffer->frame);

  vidParam->outBuffer->frame = NULL;
  freePicture (vidParam->outBuffer->top_field);

  vidParam->outBuffer->top_field = NULL;
  freePicture (vidParam->outBuffer->bottom_field);

  vidParam->outBuffer->bottom_field = NULL;
  vidParam->outBuffer->is_used = 0;
  }
//}}}

//{{{
void allocOutput (sVidParam* vidParam) {

  vidParam->outBuffer = allocFrameStore();

  vidParam->pendingOutput = calloc (sizeof(sPicture), 1);
  if (!vidParam->pendingOutput)
    no_mem_exit ("init_out_buffer");

  vidParam->pendingOutput->imgUV = NULL;
  vidParam->pendingOutput->imgY  = NULL;
  }
//}}}
//{{{
void freeOutput (sVidParam* vidParam) {

  freeFrameStore (vidParam->outBuffer);
  vidParam->outBuffer = NULL;

  flushPendingOutput (vidParam);
  free (vidParam->pendingOutput);
  }
//}}}

//{{{
void writeStoredFrame (sVidParam* vidParam, sFrameStore* frameStore) {

  // make sure no direct output field is pending
  flushDirectOutput (vidParam);

  if (frameStore->is_used < 3)
    writeUnpairedField (vidParam, frameStore);
  else {
    if (frameStore->recovery_frame)
      vidParam->recoveryFlag = 1;
    if ((!vidParam->nonConformingStream) || vidParam->recoveryFlag)
      writePicture (vidParam, frameStore->frame, FRAME);
    }

  frameStore->is_output = 1;
  }
//}}}
//{{{
void directOutput (sVidParam* vidParam, sPicture* p) {

  if (p->structure == FRAME) {
    // we have a frame (or complementary field pair), so output it directly
    flushDirectOutput (vidParam);
    writePicture (vidParam, p, FRAME);
    calcFrameNum (vidParam, p);
    freePicture (p);
    return;
    }

  if (p->structure == TOP_FIELD) {
    if (vidParam->outBuffer->is_used & 1)
      flushDirectOutput (vidParam);
    vidParam->outBuffer->top_field = p;
    vidParam->outBuffer->is_used |= 1;
    }

  if (p->structure == BOTTOM_FIELD) {
    if (vidParam->outBuffer->is_used & 2)
      flushDirectOutput (vidParam);
    vidParam->outBuffer->bottom_field = p;
    vidParam->outBuffer->is_used |= 2;
    }

  if (vidParam->outBuffer->is_used == 3) {
    // we have both fields, so output them
    dpb_combine_field_yuv (vidParam, vidParam->outBuffer);
    writePicture (vidParam, vidParam->outBuffer->frame, FRAME);

    calcFrameNum (vidParam, p);
    freePicture (vidParam->outBuffer->frame);

    vidParam->outBuffer->frame = NULL;
    freePicture (vidParam->outBuffer->top_field);

    vidParam->outBuffer->top_field = NULL;
    freePicture (vidParam->outBuffer->bottom_field);

    vidParam->outBuffer->bottom_field = NULL;
    vidParam->outBuffer->is_used = 0;
    }
  }
//}}}
