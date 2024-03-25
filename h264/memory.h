#pragma once
#include "global.h"
#include "buffer.h"

extern void noMemoryExit (char *where);
//{{{
static inline void* memAlloc (size_t nitems) {

  void* d;
  if ((d = malloc(nitems)) == NULL) {
    noMemoryExit ("malloc failed.\n");
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
  free (a);
  }
//}}}

extern int getMem2Dmp (sPicMotionParam** *array2D, int dim0, int dim1);
extern int getMem3Dmp (sPicMotionParam** **array3D, int dim0, int dim1, int dim2);

extern int getMem2Dmv (sMotionVec** *array2D, int dim0, int dim1);
extern int getMem3Dmv (sMotionVec** **array3D, int dim0, int dim1, int dim2);
extern int getMem4Dmv (sMotionVec** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int getMem5Dmv (sMotionVec** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4);
extern int getMem6Dmv (sMotionVec** *****array6D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int getMem7Dmv (sMotionVec** ******array7D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

extern byte** new_mem2D (int dim0, int dim1);
extern int getMem2D (byte** *array2D, int dim0, int dim1);
extern int getMem3D (byte** **array3D, int dim0, int dim1, int dim2);
extern int getMem4D (byte** ***array4D, int dim0, int dim1, int dim2, int dim3);

extern int** new_mem2Dint (int dim0, int dim1);
extern int getMem2Dint (int** *array2D, int dim0, int dim1);
extern int getMem2Dint_pad (int** *array2D, int dim0, int dim1, int iPadY, int iPadX);
extern int getMem2Dint64 (int64** *array2D, int dim0, int dim1);
extern int getMem3Dint (int** **array3D, int dim0, int dim1, int dim2);
extern int getMem3Dint64 (int64** **array3D, int dim0, int dim1, int dim2);
extern int getMem4Dint (int** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int getMem4Dint64 (int64** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int getMem5Dint (int** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4);

extern uint16** new_mem2Duint16 (int dim0, int dim1);
extern int getMem2Duint16 (uint16** *array2D, int dim0, int dim1);
extern int getMem3Duint16 (uint16** **array3D,int dim0, int dim1, int dim2);

extern int getMem2Ddistblk (distblk** *array2D, int dim0, int dim1);
extern int getMem3Ddistblk (distblk** **array3D, int dim0, int dim1, int dim2);
extern int getMem4Ddistblk (distblk** ***array4D, int dim0, int dim1, int dim2, int dim3);

extern int getMem2Dshort (short** *array2D, int dim0, int dim1);
extern int getMem3Dshort (short** **array3D, int dim0, int dim1, int dim2);
extern int getMem4Dshort (short** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int getMem5Dshort (short** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4);
extern int getMem6Dshort (short** *****array6D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int getMem7Dshort (short** ******array7D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

extern int getMem1Dpel (sPixel** array2D, int dim0);
extern int getMem2Dpel (sPixel** *array2D, int dim0, int dim1);
extern int getMem2DpelPad (sPixel** *array2D, int dim0, int dim1, int iPadY, int iPadX);

extern int getMem3Dpel (sPixel** **array3D, int dim0, int dim1, int dim2);
extern int getMem3DpelPad (sPixel** **array3D, int dim0, int dim1, int dim2, int iPadY, int iPadX);
extern int getMem4Dpel (sPixel** ***array4D, int dim0, int dim1, int dim2, int dim3);
extern int getMem4DpelPad (sPixel** ***array4D, int dim0, int dim1, int dim2, int dim3, int iPadY, int iPadX);
extern int getMem5Dpel (sPixel** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4);
extern int getMem5DpelPad (sPixel** ****array5D, int dim0, int dim1, int dim2, int dim3, int dim4, int iPadY, int iPadX);
extern int getMem2Ddouble (double** *array2D, int dim0, int dim1);

extern int getMem1Dodouble (double** array1D, int dim0, int offset);
extern int getMem2Dodouble (double** *array2D, int dim0, int dim1, int offset);
extern int getMem3Dodouble (double** **array2D, int dim0, int dim1, int dim2, int offset);

extern int getMem2Doint (int** *array2D, int dim0, int dim1, int offset);
extern int getMem3Doint (int** **array3D, int dim0, int dim1, int dim2, int offset);

extern int getMem2Dwp (sWpParam** *array2D, int dim0, int dim1);

extern int get_offset_mem2Dshort(short** *array2D, int rows, int columns, int offset_y, int offset_x);

extern void free_offset_mem2Dshort(short** array2D, int columns, int offset_x, int offset_y);

extern void freeMem2Dmp (sPicMotionParam   ** array2D);
extern void freeMem3Dmp (sPicMotionParam  ** *array2D);

extern void freeMem2Dmv (sMotionVec    ** array2D);
extern void freeMem3Dmv (sMotionVec   ** *array2D);
extern void freeMem4Dmv (sMotionVec  ** **array2D);
extern void freeMem5Dmv (sMotionVec ** ***array2D);
extern void freeMem6Dmv (sMotionVec** ****array2D);
extern void freeMem7Dmv (sMotionVec** *****array7D);

extern int getMem2Dspp (sPicture* ** *array3D, int dim0, int dim1);
extern int getMem3Dspp (sPicture*** **array3D, int dim0, int dim1, int dim2);

extern void freeMem2Dspp (sPicture* ** array2D);
extern void freeMem3Dspp (sPicture*** *array2D);

extern void freeMem2D (byte     ** array2D);
extern void freeMem3D (byte    ** *array3D);
extern void freeMem4D (byte   ** **array4D);

extern void freeMem2Dint (int      ** array2D);
extern void freeMem2Dint_pad (int** array2D, int iPadY, int iPadX);
extern void freeMem3Dint (int     ** *array3D);
extern void freeMem4Dint (int    ** **array4D);
extern void freeMem5Dint (int   ** ***array5D);

extern void freeMem2Duint16 (uint16** array2D);
extern void freeMem3Duint16 (uint16** *array3D);

extern void freeMem2Dint64 (int64    ** array2D);
extern void freeMem3Dint64 (int64   ** *array3D);
extern void freeMem4Dint64 (int64    ** **array4D);

extern void freeMem2Ddistblk (distblk    ** array2D);
extern void freeMem3Ddistblk (distblk   ** *array3D);
extern void freeMem4Ddistblk (distblk    ** **array4D);

extern void freeMem2Dshort (short     ** array2D);
extern void freeMem3Dshort (short    ** *array3D);
extern void freeMem4Dshort (short   ** **array4D);
extern void freeMem5Dshort (short  ** ***array5D);
extern void freeMem6Dshort (short ** ****array6D);
extern void freeMem7Dshort (short** *****array7D);

extern void freeMem1Dpel (sPixel     *array1D);
extern void freeMem2Dpel (sPixel   ** array2D);
extern void freeMem2DpelPad (sPixel** array2D, int iPadY, int iPadX);
extern void freeMem3Dpel (sPixel  ** *array3D);
extern void freeMem3DpelPad (sPixel** *array3D, int iDim12, int iPadY, int iPadX);
extern void freeMem4Dpel (sPixel ** **array4D);
extern void freeMem4DpelPad (sPixel ** **array4D, int iFrames, int iPadY, int iPadX);
extern void freeMem5Dpel (sPixel** ***array5D);
extern void freeMem5DpelPad (sPixel** ***array5D, int iFrames, int iPadY, int iPadX);
extern void freeMem2Ddouble (double** array2D);
extern void freeMem3Ddouble (double** *array3D);

extern void freeMem1Dodouble (double  *array1D, int offset);
extern void freeMem2Dodouble (double** array2D, int offset);
extern void freeMem3Dodouble (double** *array3D, int rows, int columns, int offset);
extern void freeMem2Doint (int** array2D, int offset);
extern void freeMem3Doint (int** *array3D, int rows, int columns, int offset);

extern int  initTopBotPlanes (sPixel** imgFrame, int height, sPixel** *imgTopField, sPixel** *imgBotField);
extern void freeTopBotPlanes (sPixel** imgTopField, sPixel** imgBotField);

extern void freeMem2Dwp (sWpParam** array2D);

extern void copy2DImage (sPixel** dst_img, sPixel** src_img, int sizeX, int sizeY);
extern int  malloc_mem2Dpel_2SLayers (sPixel** *buf0, int imgtype0, sPixel** *buf1, int imgtype1, int height, int width);
extern int  malloc_mem3Dpel_2SLayers (sPixel** **buf0, int imgtype0, sPixel** **buf1, int imgtype1, int frames, int height, int width);

extern void freeMem2Dpel_2SLayers (sPixel** *buf0, sPixel** *buf1);
extern void freeMem3Dpel_2SLayers (sPixel** **buf0, sPixel** **buf1);
