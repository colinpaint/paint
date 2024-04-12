//{{{  includes
#include "global.h"
#include "memory.h"
//}}}

namespace {
  //{{{
  void* mem_calloc (size_t nitems, size_t size) {

    size_t padded_size = nitems * size;
    void* d = malloc (padded_size);
    memset (d, 0, (int)padded_size);
    return d;
    }
  //}}}
  //{{{
  static void noMemoryExit (const char* where) {
    cDecoder264::error ("no more memory");
    }
  //}}}
  }

//{{{
int getMem2Dmp (sPicMotion*** array2D, int dim0, int dim1) {

  if ((*array2D = (sPicMotion**)malloc(dim0 *      sizeof(sPicMotion*))) == NULL)
    noMemoryExit ("getMem2Dmp: array2D");
  if ((*(*array2D) = (sPicMotion* )mem_calloc(dim0 * dim1, sizeof(sPicMotion ))) == NULL)
    noMemoryExit ("getMem2Dmp: array2D");

  for (int i = 1 ; i < dim0; i++)
    (*array2D)[i] = (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(sPicMotion*) + dim1 * sizeof(sPicMotion));
  }
//}}}
//{{{
int getMem3Dmp (sPicMotion**** array3D, int dim0, int dim1, int dim2) {

  int mem_size = dim0 * sizeof(sPicMotion**);

  if (((*array3D) = (sPicMotion***)malloc(dim0 * sizeof(sPicMotion**))) == NULL)
    noMemoryExit ("getMem3Dmp: array3D");

  mem_size += getMem2Dmp (*array3D, dim0 * dim1, dim2);
  for (int i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i - 1] + dim1;

  return mem_size;
  }
//}}}
//{{{
void freeMem2Dmp (sPicMotion** array2D)
{
  if (array2D) {
    if (*array2D)
      free (*array2D);
    free (array2D);
    }
  }
//}}}
//{{{
void freeMem3Dmp (sPicMotion*** array3D)
{
  if (array3D)
  {
    freeMem2Dmp(*array3D);
    free (array3D);
  }
}
//}}}

//{{{
uint8_t** new_mem2D (int dim0, int dim1) {

  int i;
  uint8_t** array2D;

  if ((array2D  = (uint8_t**)malloc(dim0 *      sizeof(uint8_t*))) == NULL)
    noMemoryExit("getMem2D: array2D");
  if ((*(array2D) = (uint8_t* )mem_calloc(dim0 * dim1,sizeof(uint8_t ))) == NULL)
    noMemoryExit("getMem2D: array2D");

  for(i = 1; i < dim0; i++)
    array2D[i] = array2D[i-1] + dim1;

  return (array2D);
  }
//}}}
//{{{
int** new_mem2Dint (int dim0, int dim1) {

  int i;
  int** array2D;

  if ((array2D = (int**)malloc(dim0 *       sizeof(int*))) == NULL)
    noMemoryExit("getMem2Dint: array2D");
  if ((*(array2D) = (int* )mem_calloc(dim0 * dim1, sizeof(int ))) == NULL)
    noMemoryExit("getMem2Dint: array2D");

  for(i = 1 ; i < dim0; i++)
    (array2D)[i] =  (array2D)[i-1] + dim1;

  return (array2D);
  }
//}}}
//{{{
int getMem2D (uint8_t*** array2D, int dim0, int dim1) {

  int i;
  if ((*array2D  = (uint8_t**)malloc(dim0 *      sizeof(uint8_t*))) == NULL)
    noMemoryExit("getMem2D: array2D");
  if ((*(*array2D) = (uint8_t* )mem_calloc(dim0 * dim1,sizeof(uint8_t ))) == NULL)
    noMemoryExit("getMem2D: array2D");

  for(i = 1; i < dim0; i++)
    (*array2D)[i] = (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(uint8_t*) + dim1 * sizeof(uint8_t));
  }
//}}}
//{{{
int getMem2Dint (int*** array2D, int dim0, int dim1) {

  int i;
  if ((*array2D    = (int**)malloc(dim0 *       sizeof(int*))) == NULL)
    noMemoryExit("getMem2Dint: array2D");
  if ((*(*array2D) = (int* )mem_calloc(dim0 * dim1, sizeof(int ))) == NULL)
    noMemoryExit("getMem2Dint: array2D");

  for (i = 1 ; i < dim0; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(int*) + dim1 * sizeof(int));
  }
//}}}
//{{{
int getMem3D (uint8_t**** array3D, int dim0, int dim1, int dim2) {

  int i, mem_size = dim0 * sizeof(uint8_t**);

  if (((*array3D) = (uint8_t***)malloc(dim0 * sizeof(uint8_t**))) == NULL)
    noMemoryExit("getMem3D: array3D");

  mem_size += getMem2D(*array3D, dim0 * dim1, dim2);

  for (i = 1; i < dim0; i++)
    (*array3D)[i] =  (*array3D)[i-1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem3Dint (int**** array3D, int dim0, int dim1, int dim2) {

  int i, mem_size = dim0 * sizeof(int**);

  if (((*array3D) = (int***)malloc(dim0 * sizeof(int**))) == NULL)
    noMemoryExit("getMem3Dint: array3D");

  mem_size += getMem2Dint(*array3D, dim0 * dim1, dim2);

  for (i = 1; i < dim0; i++)
    (*array3D)[i] =  (*array3D)[i-1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem4D (uint8_t***** array4D, int dim0, int dim1, int dim2, int dim3) {

  int  i, mem_size = dim0 * sizeof(uint8_t***);
  if (((*array4D) = (uint8_t****)malloc(dim0 * sizeof(uint8_t***))) == NULL)
    noMemoryExit("getMem4D: array4D");

  mem_size += getMem3D(*array4D, dim0 * dim1, dim2, dim3);

  for (i = 1; i < dim0; i++)
    (*array4D)[i] = (*array4D)[i-1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem4Dint (int***** array4D, int dim0, int dim1, int dim2, int dim3) {

  int i, mem_size = dim0 * sizeof(int***);

  if (((*array4D) = (int****)malloc(dim0 * sizeof(int***))) == NULL)
    noMemoryExit("getMem4Dint: array4D");

  mem_size += getMem3Dint(*array4D, dim0 * dim1, dim2, dim3);

  for (i = 1; i < dim0; i++)
    (*array4D)[i] =  (*array4D)[i-1] + dim1;

  return mem_size;
  }
//}}}
//{{{
void freeMem2D (uint8_t** array2D)
{
  if (array2D)
  {
    if (*array2D)
      free (*array2D);
    free (array2D);
  }
}
//}}}
//{{{
void freeMem2Dint (int** array2D)
{
  if (array2D)
  {
    if (*array2D)
      free (*array2D);
    free (array2D);
  }
}
//}}}
//{{{
void freeMem3D (uint8_t*** array3D)
{
  if (array3D)
  {
   freeMem2D(*array3D);
   free (array3D);
  }
}
//}}}
//{{{
void freeMem3Dint (int*** array3D)
{
  if (array3D)
  {
   freeMem2Dint(*array3D);
   free (array3D);
  }
}
//}}}
//{{{
void freeMem4D (uint8_t**** array4D)
{
  if (array4D)
  {
   freeMem3D(*array4D);
   free (array4D);
  }
}
//}}}
//{{{
void freeMem4Dint (int**** array4D)
{
  if (array4D)
  {
    freeMem3Dint( *array4D);
    free (array4D);
  }
}
//}}}

//{{{
int getMem2Dshort (int16_t*** array2D, int dim0, int dim1) {

  int i;
  int16_t *curr = NULL;

  if ((*array2D  = (int16_t**)malloc(dim0 *      sizeof(int16_t*))) == NULL)
    noMemoryExit("getMem2Dshort: array2D");
  if ((*(*array2D) = (int16_t* )mem_calloc(dim0 * dim1,sizeof(int16_t ))) == NULL)
    noMemoryExit("getMem2Dshort: array2D");

  curr = (*array2D)[0];
  for(i = 1; i < dim0; i++) {
    curr += dim1;
    (*array2D)[i] = curr;
    }

  return dim0 * (sizeof(int16_t*) + dim1 * sizeof(int16_t));
  }
//}}}
//{{{
int getMem3Dshort (int16_t**** array3D,int dim0, int dim1, int dim2) {

  int i, mem_size = dim0 * sizeof(int16_t**);
  int16_t** curr = NULL;
  if (((*array3D) = (int16_t***)malloc(dim0 * sizeof(int16_t**))) == NULL)
    noMemoryExit ("getMem3Dshort: array3D");

  mem_size += getMem2Dshort (*array3D, dim0 * dim1, dim2);

  curr = (*array3D)[0];
  for (i = 1; i < dim0; i++) {
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
    freeMem2Dshort(*array3D);
    free (array3D);
    }
  }
//}}}

//{{{
int getMem2Dpel (sPixel*** array2D, int dim0, int dim1) {

  int i;
  if ((*array2D = (sPixel**)malloc (dim0 * sizeof(sPixel*))) == NULL)
    noMemoryExit ("getMem2Dpel: array2D");
  if ((*(*array2D) = (sPixel* )malloc(dim0 * dim1 * sizeof(sPixel ))) == NULL)
    noMemoryExit ("getMem2Dpel: array2D");

  for(i = 1 ; i < dim0; i++)
    (*array2D)[i] = (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(sPixel*) + dim1 * sizeof(sPixel));
  }
//}}}
//{{{
int getMem2DpelPad (sPixel*** array2D, int dim0, int dim1, int iPadY, int iPadX) {

  int i;
  sPixel* curr = NULL;
  int height, width;

  height = dim0+2*iPadY;
  width = dim1+2*iPadX;
  if ((*array2D  = (sPixel**)malloc(height*sizeof(sPixel*))) == NULL)
    noMemoryExit("getMem2DpelPad: array2D");
  if ((*(*array2D) = (sPixel* )mem_calloc(height * width, sizeof(sPixel ))) == NULL)
    noMemoryExit("getMem2DpelPad: array2D");

  (*array2D)[0] += iPadX;
  curr = (*array2D)[0];
  for(i = 1 ; i < height; i++) {
    curr += width;
    (*array2D)[i] = curr;
    }
  (*array2D) = &((*array2D)[iPadY]);

  return height * (sizeof(sPixel*) + width * sizeof(sPixel));
  }
//}}}
//{{{
int getMem3Dpel (sPixel**** array3D, int dim0, int dim1, int dim2) {

  int i, mem_size = dim0 * sizeof(sPixel**);

  if (((*array3D) = (sPixel***)malloc(dim0 * sizeof(sPixel**))) == NULL)
    noMemoryExit("getMem3Dpel: array3D");

  mem_size += getMem2Dpel(*array3D, dim0 * dim1, dim2);

  for (i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i - 1] + dim1;

  return mem_size;
  }

//}}}
//{{{
int getMem3DpelPad (sPixel**** array3D, int dim0, int dim1, int dim2, int iPadY, int iPadX) {

  int i, mem_size = dim0 * sizeof(sPixel**);

  if (((*array3D) = (sPixel***)malloc (dim0*sizeof(sPixel**))) == NULL)
    noMemoryExit ("getMem3DpelPad: array3D");

  for (i = 0; i < dim0; i++)
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
    for(int i = 0; i < iDim12; i++)
      if (array3D[i]) {
        freeMem2DpelPad(array3D[i], iPadY, iPadX);
        array3D[i] = NULL;
        }
    free (array3D);
    }
  }
//}}}

//{{{
void freeMem2Ddouble (double** array2D)
{
  if (array2D)
  {
    if (*array2D)
      free (*array2D);

    free (array2D);

  }
}
//}}}
//{{{
void freeMem2Dodouble (double** array2D, int offset)
{
  if (array2D)
  {
    array2D[0] -= offset;
    if (array2D[0])
      free (array2D[0]);

    free (array2D);

  }
}
//}}}
