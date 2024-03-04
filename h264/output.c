//{{{
/*!
 ************************************************************************
 * \file output.c
 *
 * \brief
 *    Output an image and Trance support
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Karsten Suehring
 ************************************************************************
 */
//}}}
//{{{  includes
#include "global.h"
#include "image.h"
#include "memalloc.h"
//}}}
#define PAIR_FIELDS_IN_OUTPUT 1 // Pair field pictures output

//{{{
static void img2buf (imgpel** imgX, unsigned char* buf,
                     int size_x, int size_y, int symbol_size_in_bytes,
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
      memcpy (buf + i*iOutStride, imgX[i], size_x * sizeof(imgpel));
  }
//}}}
//{{{
static void allocate_p_dec_pic (VideoParameters* p_Vid, DecodedPicList* pDecPic, StorablePicture* p,
                                int iLumaSize, int iFrameSize, int iLumaSizeX, int iLumaSizeY,
                                int iChromaSizeX, int iChromaSizeY) {

  int symbol_size_in_bytes = ((p_Vid->pic_unit_bitsize_on_disk+7) >> 3);

  if (pDecPic->pY)
    mem_free (pDecPic->pY);

  pDecPic->iBufSize = iFrameSize;
  pDecPic->pY = mem_malloc(pDecPic->iBufSize);
  pDecPic->pU = pDecPic->pY+iLumaSize;
  pDecPic->pV = pDecPic->pU + ((iFrameSize-iLumaSize)>>1);

  //init;
  pDecPic->iYUVFormat = p->chroma_format_idc;
  pDecPic->iYUVStorageFormat = 0;
  pDecPic->iBitDepth = p_Vid->pic_unit_bitsize_on_disk;
  pDecPic->iWidth = iLumaSizeX; //p->size_x;
  pDecPic->iHeight = iLumaSizeY; //p->size_y;
  pDecPic->iYBufStride = iLumaSizeX*symbol_size_in_bytes; //p->size_x *symbol_size_in_bytes;
  pDecPic->iUVBufStride = iChromaSizeX*symbol_size_in_bytes; //p->size_x_cr*symbol_size_in_bytes;
  }
//}}}

//{{{
static void clearPicture (VideoParameters* p_Vid, StorablePicture* p) {

  printf ("-------------------- clearPicture -----------------\n");

  for (int i = 0; i < p->size_y; i++)
    for (int j = 0; j < p->size_x; j++)
      p->imgY[i][j] = (imgpel)p_Vid->dc_pred_value_comp[0];

  for (int i = 0; i < p->size_y_cr;i++)
    for (int j = 0; j < p->size_x_cr; j++)
      p->imgUV[0][i][j] = (imgpel)p_Vid->dc_pred_value_comp[1];

  for (int i = 0; i < p->size_y_cr;i++)
    for (int j = 0; j < p->size_x_cr; j++)
      p->imgUV[1][i][j] = (imgpel)p_Vid->dc_pred_value_comp[2];
  }
//}}}
//{{{
static void writeOutPicture (VideoParameters* p_Vid, StorablePicture* p) {

  InputParameters* p_Inp = p_Vid->p_Inp;
  DecodedPicList* pDecPic;

  static const int SubWidthC  [4]= { 1, 2, 2, 1};
  static const int SubHeightC [4]= { 1, 2, 1, 1};

  int crop_left, crop_right, crop_top, crop_bottom;
  int symbol_size_in_bytes = ((p_Vid->pic_unit_bitsize_on_disk+7) >> 3);
  int rgb_output =  p_Vid->p_EncodePar[p->layer_id]->rgb_output;
  //(p_Vid->active_sps->vui_seq_parameters.matrix_coefficients==0);

  unsigned char *buf;
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
  iLumaSize  = iLumaSizeX * iLumaSizeY * symbol_size_in_bytes;
  iFrameSize = (iLumaSizeX * iLumaSizeY + 2 * (iChromaSizeX * iChromaSizeY)) * symbol_size_in_bytes; //iLumaSize*iPicSizeTab[p->chroma_format_idc]/2;
  //printf ("write frame size: %dx%d\n", p->size_x-crop_left-crop_right,p->size_y-crop_top-crop_bottom );

  // KS: this buffer should actually be allocated only once, but this is still much faster than the previous version
  pDecPic = get_one_avail_dec_pic_from_list (p_Vid->pDecOuputPic, 0, 0);
  if ((pDecPic->pY == NULL) || (pDecPic->iBufSize < iFrameSize))
    allocate_p_dec_pic (p_Vid, pDecPic, p, iLumaSize, iFrameSize, iLumaSizeX, iLumaSizeY, iChromaSizeX, iChromaSizeY);

  pDecPic->bValid = 1;
  pDecPic->iPOC = p->frame_poc;

  if (NULL == pDecPic->pY)
    no_mem_exit ("writeOutPicture: buf");

  buf = (pDecPic->bValid == 1) ? pDecPic->pY : pDecPic->pY + iLumaSizeX * symbol_size_in_bytes;
  img2buf (p->imgY, buf, p->size_x, p->size_y, symbol_size_in_bytes,
           crop_left, crop_right, crop_top, crop_bottom, pDecPic->iYBufStride);

  crop_left   = p->frame_crop_left_offset;
  crop_right  = p->frame_crop_right_offset;
  crop_top    = (2 - p->frame_mbs_only_flag) * p->frame_crop_top_offset;
  crop_bottom = (2 - p->frame_mbs_only_flag) * p->frame_crop_bottom_offset;

  buf = (pDecPic->bValid == 1) ? pDecPic->pU : pDecPic->pU + iChromaSizeX*symbol_size_in_bytes;
  img2buf (p->imgUV[0], buf, p->size_x_cr, p->size_y_cr, symbol_size_in_bytes,
           crop_left, crop_right, crop_top, crop_bottom, pDecPic->iUVBufStride);

  buf = (pDecPic->bValid == 1) ? pDecPic->pV : pDecPic->pV + iChromaSizeX*symbol_size_in_bytes;
  img2buf (p->imgUV[1], buf, p->size_x_cr, p->size_y_cr, symbol_size_in_bytes,
           crop_left, crop_right, crop_top, crop_bottom, pDecPic->iUVBufStride);
  }
//}}}

#if (PAIR_FIELDS_IN_OUTPUT)
  //{{{
  void flush_pending_output (VideoParameters* p_Vid) {

    if (p_Vid->pending_output_state != FRAME)
      writeOutPicture (p_Vid, p_Vid->pending_output);

    if (p_Vid->pending_output->imgY) {
      free_mem2Dpel (p_Vid->pending_output->imgY);
      p_Vid->pending_output->imgY=NULL;
      }

    if (p_Vid->pending_output->imgUV) {
      free_mem3Dpel (p_Vid->pending_output->imgUV);
      p_Vid->pending_output->imgUV=NULL;
      }

    p_Vid->pending_output_state = FRAME;
    }
  //}}}
  //{{{
  static void writePicture (VideoParameters* p_Vid, StorablePicture* p, int real_structure) {

    if (real_structure == FRAME) {
      flush_pending_output (p_Vid);
      writeOutPicture (p_Vid, p);
      return;
      }

    if (real_structure == p_Vid->pending_output_state) {
      flush_pending_output (p_Vid);
      writePicture (p_Vid, p, real_structure);
      return;
      }

    if (p_Vid->pending_output_state == FRAME) {
      //{{{  output frame
      p_Vid->pending_output->size_x = p->size_x;
      p_Vid->pending_output->size_y = p->size_y;
      p_Vid->pending_output->size_x_cr = p->size_x_cr;
      p_Vid->pending_output->size_y_cr = p->size_y_cr;
      p_Vid->pending_output->chroma_format_idc = p->chroma_format_idc;

      p_Vid->pending_output->frame_mbs_only_flag = p->frame_mbs_only_flag;
      p_Vid->pending_output->frame_cropping_flag = p->frame_cropping_flag;
      if (p_Vid->pending_output->frame_cropping_flag) {
        p_Vid->pending_output->frame_crop_left_offset = p->frame_crop_left_offset;
        p_Vid->pending_output->frame_crop_right_offset = p->frame_crop_right_offset;
        p_Vid->pending_output->frame_crop_top_offset = p->frame_crop_top_offset;
        p_Vid->pending_output->frame_crop_bottom_offset = p->frame_crop_bottom_offset;
        }

      get_mem2Dpel (&(p_Vid->pending_output->imgY), p_Vid->pending_output->size_y, p_Vid->pending_output->size_x);
      get_mem3Dpel (&(p_Vid->pending_output->imgUV), 2, p_Vid->pending_output->size_y_cr, p_Vid->pending_output->size_x_cr);

      clearPicture (p_Vid, p_Vid->pending_output);

      // copy first field
      int add = (real_structure == TOP_FIELD) ? 0 : 1;
      for (int i = 0; i < p_Vid->pending_output->size_y; i += 2)
        memcpy (p_Vid->pending_output->imgY[(i+add)], p->imgY[(i+add)], p->size_x * sizeof(imgpel));
      for (int i = 0; i < p_Vid->pending_output->size_y_cr; i += 2) {
        memcpy (p_Vid->pending_output->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->size_x_cr * sizeof(imgpel));
        memcpy (p_Vid->pending_output->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->size_x_cr * sizeof(imgpel));
        }

      p_Vid->pending_output_state = real_structure;
      }
      //}}}
    else {
      if (  (p_Vid->pending_output->size_x!=p->size_x) || (p_Vid->pending_output->size_y!= p->size_y)
         || (p_Vid->pending_output->frame_mbs_only_flag != p->frame_mbs_only_flag)
         || (p_Vid->pending_output->frame_cropping_flag != p->frame_cropping_flag)
         || ( p_Vid->pending_output->frame_cropping_flag &&
              (  (p_Vid->pending_output->frame_crop_left_offset   != p->frame_crop_left_offset)
               ||(p_Vid->pending_output->frame_crop_right_offset  != p->frame_crop_right_offset)
               ||(p_Vid->pending_output->frame_crop_top_offset    != p->frame_crop_top_offset)
               ||(p_Vid->pending_output->frame_crop_bottom_offset != p->frame_crop_bottom_offset)
              )
            )
         ) {
        flush_pending_output (p_Vid);
        writePicture (p_Vid, p, real_structure);
        return;
        }

      // copy second field
      int add = (real_structure == TOP_FIELD) ? 0 : 1;
      for (int i = 0; i < p_Vid->pending_output->size_y; i+=2)
        memcpy(p_Vid->pending_output->imgY[(i+add)], p->imgY[(i+add)], p->size_x * sizeof(imgpel));
      for (int i = 0; i < p_Vid->pending_output->size_y_cr; i+=2) {
        memcpy(p_Vid->pending_output->imgUV[0][(i+add)], p->imgUV[0][(i+add)], p->size_x_cr * sizeof(imgpel));
        memcpy(p_Vid->pending_output->imgUV[1][(i+add)], p->imgUV[1][(i+add)], p->size_x_cr * sizeof(imgpel));
        }

      flush_pending_output (p_Vid);
      }
    }
  //}}}
#else
  //{{{
  static void writePicture (VideoParameters* p_Vid, StorablePicture* p, int real_structure) {
    writeOutPicture (p_Vid, p);
    }
  //}}}
#endif
//{{{
static void write_unpaired_field (VideoParameters* p_Vid, FrameStore* fs) {

  assert (fs->is_used < 3);

  StorablePicture* p;
  if (fs->is_used & 0x01) {
    // we have a top field, construct an empty bottom field
    p = fs->top_field;
    fs->bottom_field = alloc_storable_picture (p_Vid, BOTTOM_FIELD, p->size_x, 2*p->size_y, p->size_x_cr, 2*p->size_y_cr, 1);
    fs->bottom_field->chroma_format_idc = p->chroma_format_idc;
    clearPicture (p_Vid, fs->bottom_field);
    dpb_combine_field_yuv (p_Vid, fs);
    writePicture (p_Vid, fs->frame, TOP_FIELD);
    }

  if (fs->is_used & 0x02) {
    // we have a bottom field, construct an empty top field
    p = fs->bottom_field;
    fs->top_field = alloc_storable_picture (p_Vid, TOP_FIELD, p->size_x, 2*p->size_y, p->size_x_cr, 2*p->size_y_cr, 1);
    fs->top_field->chroma_format_idc = p->chroma_format_idc;
    clearPicture (p_Vid, fs->top_field);
    fs ->top_field->frame_cropping_flag = fs->bottom_field->frame_cropping_flag;
    if(fs ->top_field->frame_cropping_flag) {
      fs ->top_field->frame_crop_top_offset = fs->bottom_field->frame_crop_top_offset;
      fs ->top_field->frame_crop_bottom_offset = fs->bottom_field->frame_crop_bottom_offset;
      fs ->top_field->frame_crop_left_offset = fs->bottom_field->frame_crop_left_offset;
      fs ->top_field->frame_crop_right_offset = fs->bottom_field->frame_crop_right_offset;
      }
    dpb_combine_field_yuv (p_Vid, fs);
    writePicture (p_Vid, fs->frame, BOTTOM_FIELD);
    }

  fs->is_used = 3;
  }
//}}}
//{{{
static void flush_direct_output (VideoParameters* p_Vid) {

  write_unpaired_field (p_Vid, p_Vid->out_buffer);
  free_storable_picture (p_Vid->out_buffer->frame);

  p_Vid->out_buffer->frame = NULL;
  free_storable_picture (p_Vid->out_buffer->top_field);

  p_Vid->out_buffer->top_field = NULL;
  free_storable_picture (p_Vid->out_buffer->bottom_field);

  p_Vid->out_buffer->bottom_field = NULL;
  p_Vid->out_buffer->is_used = 0;
  }
//}}}

//{{{
void init_out_buffer (VideoParameters* p_Vid) {

  p_Vid->out_buffer = alloc_frame_store();

#if (PAIR_FIELDS_IN_OUTPUT)
  p_Vid->pending_output = calloc (sizeof(StorablePicture), 1);
  if (NULL==p_Vid->pending_output)
    no_mem_exit("init_out_buffer");
  p_Vid->pending_output->imgUV = NULL;
  p_Vid->pending_output->imgY  = NULL;
#endif
  }
//}}}
//{{{
void uninit_out_buffer (VideoParameters* p_Vid) {

  free_frame_store (p_Vid->out_buffer);
  p_Vid->out_buffer=NULL;

#if (PAIR_FIELDS_IN_OUTPUT)
  flush_pending_output(p_Vid);
  free (p_Vid->pending_output);
#endif
  }
//}}}

//{{{
void write_stored_frame (VideoParameters* p_Vid, FrameStore* fs) {

  // make sure no direct output field is pending
  flush_direct_output (p_Vid);

  if (fs->is_used < 3)
    write_unpaired_field (p_Vid, fs);
  else {
    if (fs->recovery_frame)
      p_Vid->recovery_flag = 1;
    if ((!p_Vid->non_conforming_stream) || p_Vid->recovery_flag)
      writePicture (p_Vid, fs->frame, FRAME);
    }

  fs->is_output = 1;
  }
//}}}
//{{{
void direct_output (VideoParameters* p_Vid, StorablePicture* p) {

  InputParameters* p_Inp = p_Vid->p_Inp;
  if (p->structure == FRAME) {
    // we have a frame (or complementary field pair), so output it directly
    flush_direct_output (p_Vid);
    writePicture (p_Vid, p, FRAME);
    calculate_frame_no (p_Vid, p);
    free_storable_picture (p);
    return;
    }

  if (p->structure == TOP_FIELD) {
    if (p_Vid->out_buffer->is_used & 1)
      flush_direct_output (p_Vid);
    p_Vid->out_buffer->top_field = p;
    p_Vid->out_buffer->is_used |= 1;
    }

  if (p->structure == BOTTOM_FIELD) {
    if (p_Vid->out_buffer->is_used & 2)
      flush_direct_output (p_Vid);
    p_Vid->out_buffer->bottom_field = p;
    p_Vid->out_buffer->is_used |= 2;
    }

  if (p_Vid->out_buffer->is_used == 3) {
    // we have both fields, so output them
    dpb_combine_field_yuv (p_Vid, p_Vid->out_buffer);
    writePicture (p_Vid, p_Vid->out_buffer->frame, FRAME);

    calculate_frame_no (p_Vid, p);
    free_storable_picture (p_Vid->out_buffer->frame);

    p_Vid->out_buffer->frame = NULL;
    free_storable_picture (p_Vid->out_buffer->top_field);

    p_Vid->out_buffer->top_field = NULL;
    free_storable_picture (p_Vid->out_buffer->bottom_field);

    p_Vid->out_buffer->bottom_field = NULL;
    p_Vid->out_buffer->is_used = 0;
    }
  }
//}}}
