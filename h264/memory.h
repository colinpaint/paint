#pragma once
#include "global.h"
#include "buffer.h"

 void noMemoryExit (const char* where);
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

int getMem2Dmp (sPicMotion*** array2D, int dim0, int dim1);
int getMem3Dmp (sPicMotion**** array3D, int dim0, int dim1, int dim2);

int getMem2Dmv (sMotionVec*** array2D, int dim0, int dim1);
int getMem3Dmv (sMotionVec**** array3D, int dim0, int dim1, int dim2);
int getMem4Dmv (sMotionVec***** array4D, int dim0, int dim1, int dim2, int dim3);
int getMem5Dmv (sMotionVec****** array5D, int dim0, int dim1, int dim2, int dim3, int dim4);
int getMem6Dmv (sMotionVec******* array6D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5);
int getMem7Dmv (sMotionVec******** array7D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

uint8_t** new_mem2D (int dim0, int dim1);
int getMem2D (uint8_t*** array2D, int dim0, int dim1);
int getMem3D (uint8_t**** array3D, int dim0, int dim1, int dim2);
int getMem4D (uint8_t***** array4D, int dim0, int dim1, int dim2, int dim3);

int** new_mem2Dint (int dim0, int dim1);
int getMem2Dint (int*** array2D, int dim0, int dim1);
int getMem2Dint_pad (int*** array2D, int dim0, int dim1, int iPadY, int iPadX);
int getMem2Dint64 (int64_t*** array2D, int dim0, int dim1);
int getMem3Dint (int**** array3D, int dim0, int dim1, int dim2);
int getMem3Dint64 (int64_t**** array3D, int dim0, int dim1, int dim2);
int getMem4Dint (int***** array4D, int dim0, int dim1, int dim2, int dim3);
int getMem4Dint64 (int64_t***** array4D, int dim0, int dim1, int dim2, int dim3);
int getMem5Dint (int****** array5D, int dim0, int dim1, int dim2, int dim3, int dim4);

uint16_t** new_mem2Duint16 (int dim0, int dim1);
int getMem2Duint16 (uint16_t*** array2D, int dim0, int dim1);
int getMem3Duint16 (uint16_t**** array3D,int dim0, int dim1, int dim2);

int getMem2Ddistblk (int32_t*** array2D, int dim0, int dim1);
int getMem3Ddistblk (int32_t**** array3D, int dim0, int dim1, int dim2);
int getMem4Ddistblk (int32_t***** array4D, int dim0, int dim1, int dim2, int dim3);

int getMem2Dshort (int16_t*** array2D, int dim0, int dim1);
int getMem3Dshort (int16_t**** array3D, int dim0, int dim1, int dim2);
int getMem4Dshort (int16_t***** array4D, int dim0, int dim1, int dim2, int dim3);
int getMem5Dshort (int16_t****** array5D, int dim0, int dim1, int dim2, int dim3, int dim4);
int getMem6Dshort (int16_t******* array6D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5);
int getMem7Dshort (int16_t******** array7D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

int getMem1Dpel (sPixel** array2D, int dim0);
int getMem2Dpel (sPixel** *array2D, int dim0, int dim1);
int getMem2DpelPad (sPixel** *array2D, int dim0, int dim1, int iPadY, int iPadX);

int getMem3Dpel (sPixel**** array3D, int dim0, int dim1, int dim2);
int getMem3DpelPad (sPixel**** array3D, int dim0, int dim1, int dim2, int iPadY, int iPadX);
int getMem4Dpel (sPixel***** array4D, int dim0, int dim1, int dim2, int dim3);
int getMem4DpelPad (sPixel***** array4D, int dim0, int dim1, int dim2, int dim3, int iPadY, int iPadX);
int getMem5Dpel (sPixel** **** array5D, int dim0, int dim1, int dim2, int dim3, int dim4);
int getMem5DpelPad (sPixel****** array5D, int dim0, int dim1, int dim2, int dim3, int dim4, int iPadY, int iPadX);
int getMem2Ddouble (double*** array2D, int dim0, int dim1);

int getMem1Dodouble (double** array1D, int dim0, int offset);
int getMem2Dodouble (double*** array2D, int dim0, int dim1, int offset);
int getMem3Dodouble (double**** array2D, int dim0, int dim1, int dim2, int offset);

int getMem2Doint (int*** array2D, int dim0, int dim1, int offset);
int getMem3Doint (int**** array3D, int dim0, int dim1, int dim2, int offset);

int get_offset_mem2Dshort(int16_t*** array2D, int rows, int columns, int offset_y, int offset_x);

void free_offset_mem2Dshort(int16_t** array2D, int columns, int offset_x, int offset_y);

void freeMem2Dmp (sPicMotion** array2D);
void freeMem3Dmp (sPicMotion*** array2D);

void freeMem2Dmv (sMotionVec** array2D);
void freeMem3Dmv (sMotionVec*** array2D);
void freeMem4Dmv (sMotionVec**** array2D);
void freeMem5Dmv (sMotionVec***** array2D);
void freeMem6Dmv (sMotionVec****** array2D);
void freeMem7Dmv (sMotionVec******* array7D);

int getMem2Dspp (sPicture**** array3D, int dim0, int dim1);
int getMem3Dspp (sPicture***** array3D, int dim0, int dim1, int dim2);

void freeMem2Dspp (sPicture*** array2D);
void freeMem3Dspp (sPicture**** array2D);

void freeMem2D (uint8_t** array2D);
void freeMem3D (uint8_t*** array3D);
void freeMem4D (uint8_t**** array4D);

void freeMem2Dint (int** array2D);
void freeMem2Dint_pad (int** array2D, int iPadY, int iPadX);
void freeMem3Dint (int*** array3D);
void freeMem4Dint (int**** array4D);
void freeMem5Dint (int***** array5D);

void freeMem2Duint16 (uint16_t** array2D);
void freeMem3Duint16 (uint16_t** *array3D);

void freeMem2Dint64 (int64_t** array2D);
void freeMem3Dint64 (int64_t*** array3D);
void freeMem4Dint64 (int64_t**** array4D);

void freeMem2Ddistblk (int32_t** array2D);
void freeMem3Ddistblk (int32_t*** array3D);
void freeMem4Ddistblk (int32_t**** array4D);

void freeMem2Dshort (int16_t** array2D);
void freeMem3Dshort (int16_t*** array3D);
void freeMem4Dshort (int16_t**** array4D);
void freeMem5Dshort (int16_t***** array5D);
void freeMem6Dshort (int16_t****** array6D);
void freeMem7Dshort (int16_t******* array7D);

void freeMem1Dpel (sPixel* array1D);
void freeMem2Dpel (sPixel** array2D);
void freeMem2DpelPad (sPixel** array2D, int iPadY, int iPadX);
void freeMem3Dpel (sPixel*** array3D);
void freeMem3DpelPad (sPixel*** array3D, int iDim12, int iPadY, int iPadX);
void freeMem4Dpel (sPixel**** array4D);
void freeMem4DpelPad (sPixel**** array4D, int iFrames, int iPadY, int iPadX);
void freeMem5Dpel (sPixel***** array5D);
void freeMem5DpelPad (sPixel***** array5D, int iFrames, int iPadY, int iPadX);
void freeMem2Ddouble (double** array2D);
void freeMem3Ddouble (double*** array3D);

void freeMem1Dodouble (double* array1D, int offset);
void freeMem2Dodouble (double** array2D, int offset);
void freeMem3Dodouble (double*** array3D, int rows, int columns, int offset);
void freeMem2Doint (int** array2D, int offset);
void freeMem3Doint (int*** array3D, int rows, int columns, int offset);

int  initTopBotPlanes (sPixel** imgFrame, int height, sPixel*** imgTopField, sPixel*** imgBotField);
void freeTopBotPlanes (sPixel** imgTopField, sPixel** imgBotField);

void copy2DImage (sPixel** dst_img, sPixel** src_img, int sizeX, int sizeY);
int  malloc_mem2Dpel_2SLayers (sPixel*** buf0, int imgtype0, sPixel*** buf1, int imgtype1, int height, int width);
int  malloc_mem3Dpel_2SLayers (sPixel**** buf0, int imgtype0, sPixel**** buf1, int imgtype1, int frames, int height, int width);

void freeMem2Dpel_2SLayers (sPixel*** buf0, sPixel*** buf1);
void freeMem3Dpel_2SLayers (sPixel**** buf0, sPixel**** buf1);
