#pragma once
#include "global.h"
#include "buffer.h"

extern void no_mem_exit (char *where);
//{{{
static inline void* memAlloc (size_t nitems) {

  void* d;
  if ((d = malloc(nitems)) == NULL) {
    no_mem_exit ("malloc failed.\n");
    return NULL;
    }
  return d;
  }
//}}}
//{{{
static inline void* mem_calloc (size_t nitems, size_t size) {

  size_t padded_size = nitems * size;
  void* d = memAlloc (padded_size);
  memset (d, 0, (int)padded_size);
  return d;
  }
//}}}
//{{{
static inline void memFree (void *a) {
  free_pointer (a);
  }
//}}}

extern int get_mem2Dmp (sPicMotionParam** *array2D, int dim0, int dim1);
extern int get_mem3Dmp (sPicMotionParam** **array3D, int dim0, int dim1, int dim2);

extern int get_mem2Dmv (sMotionVector** *array2D, int dim0, int dim1);
extern int get_mem3Dmv (sMotionVector** **array3D, int dim0, int dim1, int dim2);
extern int get_mem4Dmv (sMotionVector** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int get_mem5Dmv (sMotionVector** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4);
extern int get_mem6Dmv (sMotionVector** *****array6D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int get_mem7Dmv (sMotionVector** ******array7D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

extern byte** new_mem2D (int dim0, int dim1);
extern int get_mem2D (byte** *array2D, int dim0, int dim1);
extern int get_mem3D (byte** **array3D, int dim0, int dim1, int dim2);
extern int get_mem4D (byte** ***array4D, int dim0, int dim1, int dim2, int dim3);

extern int** new_mem2Dint (int dim0, int dim1);
extern int get_mem2Dint (int** *array2D, int dim0, int dim1);
extern int get_mem2Dint_pad (int** *array2D, int dim0, int dim1, int iPadY, int iPadX);
extern int get_mem2Dint64 (int64** *array2D, int dim0, int dim1);
extern int get_mem3Dint (int** **array3D, int dim0, int dim1, int dim2);
extern int get_mem3Dint64 (int64** **array3D, int dim0, int dim1, int dim2);
extern int get_mem4Dint (int** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int get_mem4Dint64 (int64** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int get_mem5Dint (int** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4);

extern uint16** new_mem2Duint16 (int dim0, int dim1);
extern int get_mem2Duint16 (uint16** *array2D, int dim0, int dim1);
extern int get_mem3Duint16 (uint16** **array3D,int dim0, int dim1, int dim2);

extern int get_mem2Ddistblk (distblk** *array2D, int dim0, int dim1);
extern int get_mem3Ddistblk (distblk** **array3D, int dim0, int dim1, int dim2);
extern int get_mem4Ddistblk (distblk** ***array4D, int dim0, int dim1, int dim2, int dim3);

extern int get_mem2Dshort (short** *array2D, int dim0, int dim1);
extern int get_mem3Dshort (short** **array3D, int dim0, int dim1, int dim2);
extern int get_mem4Dshort (short** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int get_mem5Dshort (short** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4);
extern int get_mem6Dshort (short** *****array6D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int get_mem7Dshort (short** ******array7D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

extern int get_mem1Dpel (sPixel** array2D, int dim0);
extern int get_mem2Dpel (sPixel** *array2D, int dim0, int dim1);
extern int get_mem2Dpel_pad (sPixel** *array2D, int dim0, int dim1, int iPadY, int iPadX);

extern int get_mem3Dpel (sPixel** **array3D, int dim0, int dim1, int dim2);
extern int get_mem3Dpel_pad (sPixel** **array3D, int dim0, int dim1, int dim2, int iPadY, int iPadX);
extern int get_mem4Dpel (sPixel** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int get_mem4Dpel_pad (sPixel** ***array4D, int dim0, int dim1, int dim2, int dim3, int iPadY, int iPadX);
extern int get_mem5Dpel (sPixel** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4);
extern int get_mem5Dpel_pad (sPixel** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4, int iPadY, int iPadX);
extern int get_mem2Ddouble (double** *array2D, int dim0, int dim1);

extern int get_mem1Dodouble (double** array1D, int dim0, int offset);
extern int get_mem2Dodouble (double** *array2D, int dim0, int dim1, int offset);
extern int get_mem3Dodouble (double** **array2D, int dim0, int dim1, int dim2, int offset);

extern int get_mem2Doint (int** *array2D, int dim0, int dim1, int offset);
extern int get_mem3Doint (int** **array3D, int dim0, int dim1, int dim2, int offset);

extern int get_mem2Dwp (sWPParam** *array2D, int dim0, int dim1);

extern int get_offset_mem2Dshort(short** *array2D, int rows, int columns, int offset_y, int offset_x);

extern void free_offset_mem2Dshort(short** array2D, int columns, int offset_x, int offset_y);

extern void free_mem2Dmp (sPicMotionParam   ** array2D);
extern void free_mem3Dmp (sPicMotionParam  ** *array2D);

extern void free_mem2Dmv (sMotionVector    ** array2D);
extern void free_mem3Dmv (sMotionVector   ** *array2D);
extern void free_mem4Dmv (sMotionVector  ** **array2D);
extern void free_mem5Dmv (sMotionVector ** ***array2D);
extern void free_mem6Dmv (sMotionVector** ****array2D);
extern void free_mem7Dmv (sMotionVector** *****array7D);

extern int get_mem2D_spp (sPicturePtr ** *array3D, int dim0, int dim1);
extern int get_mem3D_spp (sPicturePtr** **array3D, int dim0, int dim1, int dim2);

extern void free_mem2D_spp (sPicturePtr ** array2D);
extern void free_mem3D_spp (sPicturePtr** *array2D);

extern void free_mem2D (byte     ** array2D);
extern void free_mem3D (byte    ** *array3D);
extern void free_mem4D (byte   ** **array4D);

extern void free_mem2Dint (int      ** array2D);
extern void free_mem2Dint_pad (int** array2D, int iPadY, int iPadX);
extern void free_mem3Dint (int     ** *array3D);
extern void free_mem4Dint (int    ** **array4D);
extern void free_mem5Dint (int   ** ***array5D);

extern void free_mem2Duint16 (uint16** array2D);
extern void free_mem3Duint16 (uint16** *array3D);

extern void free_mem2Dint64 (int64    ** array2D);
extern void free_mem3Dint64 (int64   ** *array3D);
extern void free_mem4Dint64 (int64    ** **array4D);

extern void free_mem2Ddistblk (distblk    ** array2D);
extern void free_mem3Ddistblk (distblk   ** *array3D);
extern void free_mem4Ddistblk (distblk    ** **array4D);

extern void free_mem2Dshort (short     ** array2D);
extern void free_mem3Dshort (short    ** *array3D);
extern void free_mem4Dshort (short   ** **array4D);
extern void free_mem5Dshort (short  ** ***array5D);
extern void free_mem6Dshort (short ** ****array6D);
extern void free_mem7Dshort (short** *****array7D);

extern void free_mem1Dpel (sPixel     *array1D);
extern void free_mem2Dpel (sPixel   ** array2D);
extern void free_mem2Dpel_pad (sPixel** array2D, int iPadY, int iPadX);
extern void free_mem3Dpel (sPixel  ** *array3D);
extern void free_mem3Dpel_pad (sPixel** *array3D, int iDim12, int iPadY, int iPadX);
extern void free_mem4Dpel (sPixel ** **array4D);
extern void free_mem4Dpel_pad (sPixel ** **array4D, int iFrames, int iPadY, int iPadX);
extern void free_mem5Dpel (sPixel** ***array5D);
extern void free_mem5Dpel_pad (sPixel** ***array5D, int iFrames, int iPadY, int iPadX);
extern void free_mem2Ddouble (double** array2D);
extern void free_mem3Ddouble (double** *array3D);

extern void free_mem1Dodouble (double  *array1D, int offset);
extern void free_mem2Dodouble (double** array2D, int offset);
extern void free_mem3Dodouble (double** *array3D, int rows, int columns, int offset);
extern void free_mem2Doint (int** array2D, int offset);
extern void free_mem3Doint (int** *array3D, int rows, int columns, int offset);

extern int  init_top_bot_planes (sPixel** imgFrame, int height, sPixel** *imgTopField, sPixel** *imgBotField);
extern void free_top_bot_planes (sPixel** imgTopField, sPixel** imgBotField);

extern void free_mem2Dwp (sWPParam** array2D);

extern void copy2DImage (sPixel** dst_img, sPixel** src_img, int sizeX, int sizeY);
extern int  malloc_mem2Dpel_2SLayers (sPixel** *buf0, int imgtype0, sPixel** *buf1, int imgtype1, int height, int width);
extern int  malloc_mem3Dpel_2SLayers (sPixel** **buf0, int imgtype0, sPixel** **buf1, int imgtype1, int frames, int height, int width);

extern void free_mem2Dpel_2SLayers (sPixel** *buf0, sPixel** *buf1);
extern void free_mem3Dpel_2SLayers (sPixel** **buf0, sPixel** **buf1);
