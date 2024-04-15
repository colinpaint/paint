//{{{  includes
#include "global.h"
#include "memory.h"
//}}}

//{{{
int** newMem2Dint (int dim0, int dim1) {

  int** array2D = (int**)malloc (dim0 * sizeof(int*));
  *array2D = (int*)malloc (dim0 * dim1 * sizeof(int));
  for (int i = 1 ; i < dim0; i++)
    array2D[i] =  (array2D)[i-1] + dim1;

  return array2D;
  }
//}}}

//{{{
int getMem2D (uint8_t*** array2D, int dim0, int dim1) {

  *array2D = (uint8_t**)malloc (dim0 * sizeof(uint8_t*));
  **array2D = (uint8_t*)malloc (dim0 * dim1 * sizeof(uint8_t));
  for (int i = 1; i < dim0; i++)
    (*array2D)[i] = (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(uint8_t*) + dim1 * sizeof(uint8_t));
  }
//}}}
//{{{
int getMem2Dint (int*** array2D, int dim0, int dim1) {

  *array2D = (int**)malloc (dim0 * sizeof(int*));
  **array2D = (int*)malloc (dim0 * dim1 * sizeof(int));
  for (int i = 1 ; i < dim0; i++)
    (*array2D)[i] = (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(int*) + dim1 * sizeof(int));
  }
//}}}
//{{{
int getMem3D (uint8_t**** array3D, int dim0, int dim1, int dim2) {

  *array3D = (uint8_t***)malloc (dim0 * sizeof(uint8_t**));

  int mem_size = dim0 * sizeof(uint8_t**);
  mem_size += getMem2D (*array3D, dim0 * dim1, dim2);

  for (int i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i-1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem3Dint (int**** array3D, int dim0, int dim1, int dim2) {

  *array3D = (int***)malloc (dim0 * sizeof(int**));

  int mem_size = dim0 * sizeof(int**);
  mem_size += getMem2Dint (*array3D, dim0 * dim1, dim2);

  for (int i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i-1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem4D (uint8_t***** array4D, int dim0, int dim1, int dim2, int dim3) {

  *array4D = (uint8_t****)malloc (dim0 * sizeof(uint8_t***));

  int mem_size = dim0 * sizeof(uint8_t***);
  mem_size += getMem3D (*array4D, dim0 * dim1, dim2, dim3);

  for (int i = 1; i < dim0; i++)
    (*array4D)[i] = (*array4D)[i-1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem4Dint (int***** array4D, int dim0, int dim1, int dim2, int dim3) {

  *array4D = (int****)malloc (dim0 * sizeof(int***));

  int mem_size = dim0 * sizeof(int***);
  mem_size += getMem3Dint (*array4D, dim0 * dim1, dim2, dim3);

  for (int i = 1; i < dim0; i++)
    (*array4D)[i] = (*array4D)[i-1] + dim1;

  return mem_size;
  }
//}}}
//{{{
void freeMem2D (uint8_t** array2D) {

  if (array2D) {
    if (*array2D)
      free (*array2D);
    free (array2D);
    }
  }
//}}}
//{{{
void freeMem2Dint (int** array2D) {

  if (array2D) {
    if (*array2D)
      free (*array2D);
    free (array2D);
    }
  }
//}}}
//{{{
void freeMem3D (uint8_t*** array3D) {

  if (array3D) {
    freeMem2D (*array3D);
    free (array3D);
    }
  }
//}}}
//{{{
void freeMem3Dint (int*** array3D) {

  if (array3D) {
    freeMem2Dint (*array3D);
    free (array3D);
    }
  }
//}}}
//{{{
void freeMem4D (uint8_t**** array4D) {

  if (array4D) {
    freeMem3D (*array4D);
    free (array4D);
    }
  }
//}}}
//{{{
void freeMem4Dint (int**** array4D) {

  if (array4D) {
    freeMem3Dint (*array4D);
    free (array4D);
    }
  }
//}}}

//{{{
int getMem2Dshort (int16_t*** array2D, int dim0, int dim1) {

  *array2D = (int16_t**)malloc (dim0 * sizeof(int16_t*));
  **array2D = (int16_t*)malloc (dim0 * dim1 * sizeof(int16_t));

  int16_t* curr = (*array2D)[0];
  for (int i = 1; i < dim0; i++) {
    curr += dim1;
    (*array2D)[i] = curr;
    }

  return dim0 * (sizeof(int16_t*) + dim1 * sizeof(int16_t));
  }
//}}}
//{{{
int getMem3Dshort (int16_t**** array3D, int dim0, int dim1, int dim2) {

  *array3D = (int16_t***)malloc (dim0 * sizeof(int16_t**));

  int mem_size = dim0 * sizeof(int16_t**);
  mem_size += getMem2Dshort (*array3D, dim0 * dim1, dim2);

  int16_t** curr = (*array3D)[0];
  for (int i = 1; i < dim0; i++) {
    curr += dim1;
    (*array3D)[i] = curr;
    }

  return mem_size;
  }
//}}}
//{{{
void freeMem2Dshort (int16_t** array2D) {

  if (array2D) {
    if (*array2D)
      free (*array2D);
    free (array2D);
    }
  }
//}}}
//{{{
void freeMem3Dshort (int16_t*** array3D) {

  if (array3D) {
    freeMem2Dshort (*array3D);
    free (array3D);
    }
  }
//}}}

//{{{
int getMem2Dmp (sPicMotion*** array2D, int dim0, int dim1) {

  *array2D = (sPicMotion**)malloc (dim0 * sizeof(sPicMotion*));
  **array2D = (sPicMotion*)malloc (dim0 * dim1 * sizeof(sPicMotion));

  for (int i = 1 ; i < dim0; i++)
    (*array2D)[i] = (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(sPicMotion*) + dim1 * sizeof(sPicMotion));
  }
//}}}
//{{{
int getMem3Dmp (sPicMotion**** array3D, int dim0, int dim1, int dim2) {

  *array3D = (sPicMotion***)malloc (dim0 * sizeof(sPicMotion**));

  int mem_size = dim0 * sizeof(sPicMotion**);
  mem_size += getMem2Dmp (*array3D, dim0 * dim1, dim2);
  for (int i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i - 1] + dim1;

  return mem_size;
  }
//}}}
//{{{
void freeMem2Dmp (sPicMotion** array2D) {

  if (array2D) {
    if (*array2D)
      free (*array2D);
    free (array2D);
    }
  }
//}}}
//{{{
void freeMem3Dmp (sPicMotion*** array3D) {

  if (array3D) {
    freeMem2Dmp (*array3D);
    free (array3D);
    }
  }
//}}}

//{{{
int getMem2Dpel (sPixel*** array2D, int dim0, int dim1) {

  *array2D = (sPixel**)malloc (dim0 * sizeof(sPixel*));
  **array2D = (sPixel* )malloc (dim0 * dim1 * sizeof(sPixel));

  for (int i = 1 ; i < dim0; i++)
    (*array2D)[i] = (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(sPixel*) + dim1 * sizeof(sPixel));
  }
//}}}
//{{{
int getMem2DpelPad (sPixel*** array2D, int dim0, int dim1, int iPadY, int iPadX) {

  int height = dim0 + 2*iPadY;
  int width = dim1 + 2*iPadX;

  *array2D = (sPixel**)malloc (height * sizeof(sPixel*));
  **array2D = (sPixel*)malloc (height * width * sizeof(sPixel));

  (*array2D)[0] += iPadX;
  sPixel* curr = (*array2D)[0];
  for (int i = 1 ; i < height; i++) {
    curr += width;
    (*array2D)[i] = curr;
    }
  (*array2D) = &((*array2D)[iPadY]);

  return height * (sizeof(sPixel*) + width * sizeof(sPixel));
  }
//}}}
//{{{
int getMem3Dpel (sPixel**** array3D, int dim0, int dim1, int dim2) {

  *array3D = (sPixel***)malloc (dim0 * sizeof(sPixel**));

  int mem_size = dim0 * sizeof(sPixel**);
  mem_size += getMem2Dpel(*array3D, dim0 * dim1, dim2);

  for (int i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i - 1] + dim1;

  return mem_size;
  }

//}}}
//{{{
int getMem3DpelPad (sPixel**** array3D, int dim0, int dim1, int dim2, int iPadY, int iPadX) {

  *array3D = (sPixel***)malloc (dim0*sizeof(sPixel**));

  int mem_size = dim0 * sizeof(sPixel**);
  for (int i = 0; i < dim0; i++)
    mem_size += getMem2DpelPad ((*array3D)+i, dim1, dim2, iPadY, iPadX);

  return mem_size;
  }
//}}}
//{{{
void freeMem2Dpel (sPixel** array2D) {

  if (array2D)  {
    if (*array2D)
      free (*array2D);
    free (array2D);
    }
  }
//}}}
//{{{
void freeMem2DpelPad (sPixel** array2D, int iPadY, int iPadX) {

  if (array2D)  {
    if (*array2D)
      free (array2D[-iPadY]-iPadX);
    free (&array2D[-iPadY]);
    }
  }
//}}}
//{{{
void freeMem3Dpel (sPixel*** array3D) {

  if (array3D) {
    freeMem2Dpel (*array3D);
    free (array3D);
    }
  }
//}}}
//{{{
void freeMem3DpelPad (sPixel*** array3D, int iDim12, int iPadY, int iPadX) {

  if (array3D) {
    for (int i = 0; i < iDim12; i++)
      if (array3D[i]) {
        freeMem2DpelPad (array3D[i], iPadY, iPadX);
        array3D[i] = NULL;
        }
    free (array3D);
    }
  }
//}}}
