//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
//}}}

//{{{
static void img2buf (sPixel** imgX, unsigned char* buf,
                     int size_x, int size_y, int symbolSizeInBytes,
                     int crop_left, int crop_right, int crop_top, int crop_bottom, int iOutStride) {

  if (crop_top || crop_bottom || crop_left || crop_right) {
    for (int i = crop_top; i < size_y - crop_bottom; i++) {
      // is this right, why is it cropped ???
      int ipos = ((i - crop_top) * iOutStride) + crop_left;
      memcpy (buf + ipos, imgX[i], size_x - crop_right - crop_left);
      }
    }
  else
    for (int i = 0; i < size_y; i++)
      memcpy (buf + i*iOutStride, imgX[i], size_x * sizeof(sPixel));
  }
//}}}
//{{{
static void allocDecPic (sVidParam* vidParam, sDecodedPicList* decPic, sPicture* p,
                         int iLumaSize, int iFrameSize, int iLumaSizeX, int iLumaSizeY,
                         int iChromaSizeX, int iChromaSizeY) {

  int symbolSizeInBytes = (vidParam->pic_unit_bitsize_on_disk + 7) >> 3;

  if (decPic->pY)
    mem_free (decPic->pY);

  decPic->iBufSize = iFrameSize;
  decPic->pY = mem_malloc(decPic->iBufSize);
  decPic->pU = decPic->pY + iLumaSize;
  decPic->pV = decPic->pU + ((iFrameSize-iLumaSize)>>1);

  //init;
  decPic->iYUVFormat = p->chroma_format_idc;
  decPic->iYUVStorageFormat = 0;
  decPic->iBitDepth = vidParam->pic_unit_bitsize_on_disk;
  decPic->iWidth = iLumaSizeX; 
  decPic->iHeight = iLumaSizeY; 
  decPic->iYBufStride = iLumaSizeX * symbolSizeInBytes; 
  decPic->iUVBufStride = iChromaSizeX * symbolSizeInBytes;
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

  sInputParam* inputParam = vidParam->inputParam;
  sDecodedPicList* decPic;

  static const int SubWidthC  [4]= { 1, 2, 2, 1};
  static const int SubHeightC [4]= { 1, 2, 1, 1};

  int crop_left, crop_right, crop_top, crop_bottom;
  int symbolSizeInBytes = ((vidParam->pic_unit_bitsize_on_disk+7) >> 3);

  unsigned char* buf;
  int iLumaSize, iFrameSize;
  int iLumaSizeX, iLumaSizeY;
  int iChromaSizeX, iChromaSizeY;

  if (p->non_existing)
    return;

  // should this be done only once?
  if (p->frame_cropping_flag) {
    crop_left   = SubWidthC [p->chroma_format_idc] * p->frame_crop_left_offset;
    crop_right  = SubWidthC [p->chroma_format_idc] * p->frame_crop_right_offset;
    crop_top    = SubHeightC[p->chroma_format_idc] * ( 2 - p->frame_mbs_only_flag ) * p->frame_crop_top_offset;
    crop_bottom = SubHeightC[p->chroma_format_idc] * ( 2 - p->frame_mbs_only_flag ) * p->frame_crop_bottom_offset;
    }
  else
    crop_left = crop_right = crop_top = crop_bottom = 0;

  iChromaSizeX =  p->size_x_cr- p->frame_crop_left_offset -p->frame_crop_right_offset;
  iChromaSizeY = p->size_y_cr - ( 2 - p->frame_mbs_only_flag ) * p->frame_crop_top_offset -( 2 - p->frame_mbs_only_flag ) * p->frame_crop_bottom_offset;
  iLumaSizeX = p->size_x - crop_left-crop_right;
  iLumaSizeY = p->size_y - crop_top - crop_bottom;
  iLumaSize  = iLumaSizeX * iLumaSizeY * symbolSizeInBytes;
  iFrameSize = (iLumaSizeX * iLumaSizeY + 2 * (iChromaSizeX * iChromaSizeY)) * symbolSizeInBytes; //iLumaSize*iPicSizeTab[p->chroma_format_idc]/2;
  //printf ("write frame size: %dx%d\n", p->size_x-crop_left-crop_right,p->size_y-crop_top-crop_bottom );

  // KS: this buffer should actually be allocated only once, but this is still much faster than the previous version
  decPic = get_one_avail_dec_pic_from_list (vidParam->decOutputPic, 0, 0);
  if ((decPic->pY == NULL) || (decPic->iBufSize < iFrameSize))
    allocDecPic (vidParam, decPic, p, iLumaSize, iFrameSize, iLumaSizeX, iLumaSizeY, iChromaSizeX, iChromaSizeY);

  decPic->bValid = 1;
  decPic->iPOC = p->frame_poc;

  if (NULL == decPic->pY)
    no_mem_exit ("writeOutPicture: buf");

  buf = (decPic->bValid == 1) ? decPic->pY : decPic->pY + iLumaSizeX * symbolSizeInBytes;
  img2buf (p->imgY, buf, p->size_x, p->size_y, symbolSizeInBytes,
           crop_left, crop_right, crop_top, crop_bottom, decPic->iYBufStride);

  crop_left = p->frame_crop_left_offset;
  crop_right = p->frame_crop_right_offset;
  crop_top = (2 - p->frame_mbs_only_flag) * p->frame_crop_top_offset;
  crop_bottom = (2 - p->frame_mbs_only_flag) * p->frame_crop_bottom_offset;

  buf = (decPic->bValid == 1) ? decPic->pU : decPic->pU + iChromaSizeX*symbolSizeInBytes;
  img2buf (p->imgUV[0], buf, p->size_x_cr, p->size_y_cr, symbolSizeInBytes,
           crop_left, crop_right, crop_top, crop_bottom, decPic->iUVBufStride);

  buf = (decPic->bValid == 1) ? decPic->pV : decPic->pV + iChromaSizeX*symbolSizeInBytes;
  img2buf (p->imgUV[1], buf, p->size_x_cr, p->size_y_cr, symbolSizeInBytes,
           crop_left, crop_right, crop_top, crop_bottom, decPic->iUVBufStride);
  }
//}}}
//{{{
static void flushPendingOutput (sVidParam* vidParam) {

  if (vidParam->pending_output_state != FRAME)
    writeOutPicture (vidParam, vidParam->pending_output);

  if (vidParam->pending_output->imgY) {
    free_mem2Dpel (vidParam->pending_output->imgY);
    vidParam->pending_output->imgY=NULL;
    }

  if (vidParam->pending_output->imgUV) {
    free_mem3Dpel (vidParam->pending_output->imgUV);
    vidParam->pending_output->imgUV=NULL;
    }

  vidParam->pending_output_state = FRAME;
  }
//}}}
//{{{
static void writePicture (sVidParam* vidParam, sPicture* p, int real_structure) {

  if (real_structure == FRAME) {
    flushPendingOutput (vidParam);
    writeOutPicture (vidParam, p);
    return;
    }

  if (real_structure == vidParam->pending_output_state) {
    flushPendingOutput (vidParam);
    writePicture (vidParam, p, real_structure);
    return;
    }

  if (vidParam->pending_output_state == FRAME) {
    //{{{  output frame
    vidParam->pending_output->size_x = p->size_x;
    vidParam->pending_output->size_y = p->size_y;
    vidParam->pending_output->size_x_cr = p->size_x_cr;
    vidParam->pending_output->size_y_cr = p->size_y_cr;
    vidParam->pending_output->chroma_format_idc = p->chroma_format_idc;

    vidParam->pending_output->frame_mbs_only_flag = p->frame_mbs_only_flag;
    vidParam->pending_output->frame_cropping_flag = p->frame_cropping_flag;
    if (vidParam->pending_output->frame_cropping_flag) {
      vidParam->pending_output->frame_crop_left_offset = p->frame_crop_left_offset;
      vidParam->pending_output->frame_crop_right_offset = p->frame_crop_right_offset;
      vidParam->pending_output->frame_crop_top_offset = p->frame_crop_top_offset;
      vidParam->pending_output->frame_crop_bottom_offset = p->frame_crop_bottom_offset;
      }

    get_mem2Dpel (&(vidParam->pending_output->imgY), vidParam->pending_output->size_y, vidParam->pending_output->size_x);
    get_mem3Dpel (&(vidParam->pending_output->imgUV), 2, vidParam->pending_output->size_y_cr, vidParam->pending_output->size_x_cr);

    clearPicture (vidParam, vidParam->pending_output);

    // copy first field
    int add = (real_structure == TOP_FIELD) ? 0 : 1;
    for (int i = 0; i < vidParam->pending_output->size_y; i += 2)
      memcpy (vidParam->pending_output->imgY[(i+add)], p->imgY[(i+add)], p->size_x * sizeof(sPixel));
    for (int i = 0; i < vidParam->pending_output->size_y_cr; i += 2) {
      memcpy (vidParam->pending_output->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->size_x_cr * sizeof(sPixel));
      memcpy (vidParam->pending_output->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->size_x_cr * sizeof(sPixel));
      }

    vidParam->pending_output_state = real_structure;
    }
    //}}}
  else {
    if (  (vidParam->pending_output->size_x!=p->size_x) || (vidParam->pending_output->size_y!= p->size_y)
       || (vidParam->pending_output->frame_mbs_only_flag != p->frame_mbs_only_flag)
       || (vidParam->pending_output->frame_cropping_flag != p->frame_cropping_flag)
       || ( vidParam->pending_output->frame_cropping_flag &&
            (  (vidParam->pending_output->frame_crop_left_offset   != p->frame_crop_left_offset)
             ||(vidParam->pending_output->frame_crop_right_offset  != p->frame_crop_right_offset)
             ||(vidParam->pending_output->frame_crop_top_offset    != p->frame_crop_top_offset)
             ||(vidParam->pending_output->frame_crop_bottom_offset != p->frame_crop_bottom_offset)
            )
          )
       ) {
      flushPendingOutput (vidParam);
      writePicture (vidParam, p, real_structure);
      return;
      }

    // copy second field
    int add = (real_structure == TOP_FIELD) ? 0 : 1;
    for (int i = 0; i < vidParam->pending_output->size_y; i+=2)
      memcpy (vidParam->pending_output->imgY[(i+add)], p->imgY[(i+add)], p->size_x * sizeof(sPixel));

    for (int i = 0; i < vidParam->pending_output->size_y_cr; i+=2) {
      memcpy (vidParam->pending_output->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->size_x_cr * sizeof(sPixel));
      memcpy (vidParam->pending_output->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->size_x_cr * sizeof(sPixel));
      }

    flushPendingOutput (vidParam);
    }
  }
//}}}
//{{{
static void writeUnpairedField (sVidParam* vidParam, sFrameStore* fs) {

  assert (fs->is_used < 3);

  if (fs->is_used & 0x01) {
    // we have a top field, construct an empty bottom field
    sPicture* p = fs->top_field;
    fs->bottom_field = allocPicture (vidParam, BOTTOM_FIELD, p->size_x, 2*p->size_y, p->size_x_cr, 2*p->size_y_cr, 1);
    fs->bottom_field->chroma_format_idc = p->chroma_format_idc;

    clearPicture (vidParam, fs->bottom_field);
    dpb_combine_field_yuv (vidParam, fs);

    writePicture (vidParam, fs->frame, TOP_FIELD);
    }

  if (fs->is_used & 0x02) {
    // we have a bottom field, construct an empty top field
    sPicture* p = fs->bottom_field;
    fs->top_field = allocPicture (vidParam, TOP_FIELD, p->size_x, 2*p->size_y, p->size_x_cr, 2*p->size_y_cr, 1);
    fs->top_field->chroma_format_idc = p->chroma_format_idc;
    clearPicture (vidParam, fs->top_field);

    fs ->top_field->frame_cropping_flag = fs->bottom_field->frame_cropping_flag;
    if(fs ->top_field->frame_cropping_flag) {
      fs ->top_field->frame_crop_top_offset = fs->bottom_field->frame_crop_top_offset;
      fs ->top_field->frame_crop_bottom_offset = fs->bottom_field->frame_crop_bottom_offset;
      fs ->top_field->frame_crop_left_offset = fs->bottom_field->frame_crop_left_offset;
      fs ->top_field->frame_crop_right_offset = fs->bottom_field->frame_crop_right_offset;
      }
    dpb_combine_field_yuv (vidParam, fs);

    writePicture (vidParam, fs->frame, BOTTOM_FIELD);
    }

  fs->is_used = 3;
  }
//}}}
//{{{
static void flushDirectOutput (sVidParam* vidParam) {

  writeUnpairedField (vidParam, vidParam->out_buffer);
  freePicture (vidParam->out_buffer->frame);

  vidParam->out_buffer->frame = NULL;
  freePicture (vidParam->out_buffer->top_field);

  vidParam->out_buffer->top_field = NULL;
  freePicture (vidParam->out_buffer->bottom_field);

  vidParam->out_buffer->bottom_field = NULL;
  vidParam->out_buffer->is_used = 0;
  }
//}}}

//{{{
void allocOutput (sVidParam* vidParam) {

  vidParam->out_buffer = allocFrameStore();

  vidParam->pending_output = calloc (sizeof(sPicture), 1);
  if (NULL == vidParam->pending_output)
    no_mem_exit ("init_out_buffer");

  vidParam->pending_output->imgUV = NULL;
  vidParam->pending_output->imgY  = NULL;
  }
//}}}
//{{{
void freeOutput (sVidParam* vidParam) {

  freeFrameStore (vidParam->out_buffer);
  vidParam->out_buffer = NULL;

  flushPendingOutput (vidParam);
  free (vidParam->pending_output);
  }
//}}}

//{{{
void writeStoredFrame (sVidParam* vidParam, sFrameStore* fs) {

  // make sure no direct output field is pending
  flushDirectOutput (vidParam);

  if (fs->is_used < 3)
    writeUnpairedField (vidParam, fs);
  else {
    if (fs->recovery_frame)
      vidParam->recovery_flag = 1;
    if ((!vidParam->non_conforming_stream) || vidParam->recovery_flag)
      writePicture (vidParam, fs->frame, FRAME);
    }

  fs->is_output = 1;
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
    if (vidParam->out_buffer->is_used & 1)
      flushDirectOutput (vidParam);
    vidParam->out_buffer->top_field = p;
    vidParam->out_buffer->is_used |= 1;
    }

  if (p->structure == BOTTOM_FIELD) {
    if (vidParam->out_buffer->is_used & 2)
      flushDirectOutput (vidParam);
    vidParam->out_buffer->bottom_field = p;
    vidParam->out_buffer->is_used |= 2;
    }

  if (vidParam->out_buffer->is_used == 3) {
    // we have both fields, so output them
    dpb_combine_field_yuv (vidParam, vidParam->out_buffer);
    writePicture (vidParam, vidParam->out_buffer->frame, FRAME);

    calcFrameNum (vidParam, p);
    freePicture (vidParam->out_buffer->frame);

    vidParam->out_buffer->frame = NULL;
    freePicture (vidParam->out_buffer->top_field);

    vidParam->out_buffer->top_field = NULL;
    freePicture (vidParam->out_buffer->bottom_field);

    vidParam->out_buffer->bottom_field = NULL;
    vidParam->out_buffer->is_used = 0;
    }
  }
//}}}
