#pragma once
#include "global.h"

int** new_mem2Dint (int dim0, int dim1);
int getMem2D (uint8_t*** array2D, int dim0, int dim1);
int getMem2Dint (int*** array2D, int dim0, int dim1);
int getMem3D (uint8_t**** array3D, int dim0, int dim1, int dim2);
int getMem3Dint (int**** array3D, int dim0, int dim1, int dim2);
int getMem4D (uint8_t***** array4D, int dim0, int dim1, int dim2, int dim3);
int getMem4Dint (int***** array4D, int dim0, int dim1, int dim2, int dim3);
void freeMem2D (uint8_t** array2D);
void freeMem2Dint (int** array2D);
void freeMem3D (uint8_t*** array3D);
void freeMem3Dint (int*** array3D);
void freeMem4D (uint8_t**** array4D);
void freeMem4Dint (int**** array4D);

int getMem2Dshort (int16_t*** array2D, int dim0, int dim1);
int getMem3Dshort (int16_t**** array3D, int dim0, int dim1, int dim2);
void freeMem2Dshort (int16_t** array2D);
void freeMem3Dshort (int16_t*** array3D);

int getMem2Dmp (sPicMotion*** array2D, int dim0, int dim1);
int getMem3Dmp (sPicMotion**** array3D, int dim0, int dim1, int dim2);
void freeMem2Dmp (sPicMotion** array2D);
void freeMem3Dmp (sPicMotion*** array2D);

int getMem2Dpel (sPixel** *array2D, int dim0, int dim1);
int getMem2DpelPad (sPixel** *array2D, int dim0, int dim1, int iPadY, int iPadX);
int getMem3Dpel (sPixel**** array3D, int dim0, int dim1, int dim2);
int getMem3DpelPad (sPixel**** array3D, int dim0, int dim1, int dim2, int iPadY, int iPadX);
void freeMem2Dpel (sPixel** array2D);
void freeMem2DpelPad (sPixel** array2D, int iPadY, int iPadX);
void freeMem3Dpel (sPixel*** array3D);
void freeMem3DpelPad (sPixel*** array3D, int iDim12, int iPadY, int iPadX);
