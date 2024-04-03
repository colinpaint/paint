//{{{  includes
#include "global.h"
#include "memory.h"
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Exit program if memory allocation failed (using error())
 * \param where
 *    string indicating which memory allocation failed
** **********************************************************************
 */
void noMemoryExit (const char* where) {
   error ("Could not allocate memory");
  }
//}}}

//{{{
int initTopBotPlanes (sPixel** imgFrame, int dim0, sPixel** *imgTopField, sPixel** *imgBotField)
{
  int i;

  if((*imgTopField   = (sPixel**)memAlloc((dim0>>1) * sizeof(sPixel*))) == NULL)
    noMemoryExit("initTopBotPlanes: imgTopField");

  if ((*imgBotField = (sPixel**)memAlloc ((dim0>>1) * sizeof(sPixel*))) == NULL)
    noMemoryExit( "initTopBotPlanes: imgBotField");

  for(i = 0; i < (dim0>>1); i++)
  {
    (*imgTopField)[i] =  imgFrame[2 * i    ];
    (*imgBotField)[i] =  imgFrame[2 * i + 1];
  }

  return dim0 * sizeof(sPixel*);
}
//}}}
//{{{
void freeTopBotPlanes (sPixel** imgTopField, sPixel** imgBotField)
{
  memFree (imgTopField);
  memFree (imgBotField);
}
//}}}

//{{{
int getMem2Dmp (sPicMotion*** array2D, int dim0, int dim1)
{
  int i;

  if((*array2D    = (sPicMotion**)memAlloc(dim0 *      sizeof(sPicMotion*))) == NULL)
    noMemoryExit("getMem2Dmp: array2D");
  if((*(*array2D) = (sPicMotion* )mem_calloc(dim0 * dim1, sizeof(sPicMotion ))) == NULL)
    noMemoryExit("getMem2Dmp: array2D");

  for(i = 1 ; i < dim0; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(sPicMotion*) + dim1 * sizeof(sPicMotion));
}
//}}}
//{{{
int getMem3Dmp (sPicMotion**** array3D, int dim0, int dim1, int dim2)
{
  int i, mem_size = dim0 * sizeof(sPicMotion**);

  if(((*array3D) = (sPicMotion***)memAlloc(dim0 * sizeof(sPicMotion**))) == NULL)
    noMemoryExit("getMem3Dmp: array3D");

  mem_size += getMem2Dmp(*array3D, dim0 * dim1, dim2);

  for(i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i - 1] + dim1;

  return mem_size;
}
//}}}
//{{{
void freeMem2Dmp (sPicMotion** array2D)
{
  if (array2D) {
    if (*array2D)
      memFree (*array2D);
    memFree (array2D);
    }
  }
//}}}
//{{{
void freeMem3Dmp (sPicMotion** *array3D)
{
  if (array3D)
  {
    freeMem2Dmp(*array3D);
    memFree (array3D);
  }
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D memory array -> sPicture* array2D[dim0][dim1]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************/
int getMem2Dspp (sPicture**** array2D, int dim0, int dim1)
{
  int i;

  if((*array2D    = (sPicture***)memAlloc(dim0 *      sizeof(sPicture**))) == NULL)
    noMemoryExit("getMem2Dspp: array2D");
  if((*(*array2D) = (sPicture** )mem_calloc(dim0 * dim1,sizeof(sPicture* ))) == NULL)
    noMemoryExit("getMem2Dspp: array2D");

  for(i = 1 ; i < dim0; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(sPicture**) + dim1 * sizeof(sPicture*));
}
//}}}
//{{{
int getMem3Dspp (sPicture***** array3D, int dim0, int dim1, int dim2) {

  int i, mem_size = dim0 * sizeof(sPicture***);
  if (((*array3D) = (sPicture****)memAlloc(dim0 * sizeof(sPicture***))) == NULL)
    noMemoryExit("getMem3Dspp: array3D");

  mem_size += getMem2Dspp(*array3D, dim0 * dim1, dim2);
  for (i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i - 1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem2Dmv (sMotionVec*** array2D, int dim0, int dim1) {

  int i;
  if ((*array2D    = (sMotionVec**)memAlloc(dim0 *      sizeof(sMotionVec*))) == NULL)
    noMemoryExit("getMem2Dmv: array2D");
  if ((*(*array2D) = (sMotionVec* )mem_calloc(dim0 * dim1,sizeof(sMotionVec ))) == NULL)
    noMemoryExit("getMem2Dmv: array2D");

  for (i = 1 ; i < dim0; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(sMotionVec*) + dim1 * sizeof(sMotionVec));
  }
//}}}
//{{{
int getMem3Dmv (sMotionVec**** array3D, int dim0, int dim1, int dim2) {

  int i, mem_size = dim0 * sizeof(sMotionVec**);
  if (((*array3D) = (sMotionVec***)memAlloc(dim0 * sizeof(sMotionVec**))) == NULL)
    noMemoryExit("getMem3Dmv: array3D");

  mem_size += getMem2Dmv(*array3D, dim0 * dim1, dim2);

  for (i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i - 1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem4Dmv (sMotionVec***** array4D, int dim0, int dim1, int dim2, int dim3) {

  int i, mem_size = dim0 * sizeof(sMotionVec***);
  if (((*array4D) = (sMotionVec****)memAlloc(dim0 * sizeof(sMotionVec***))) == NULL)
    noMemoryExit("getMem4Dpel: array4D");

  mem_size += getMem3Dmv(*array4D, dim0 * dim1, dim2, dim3);

  for (i = 1; i < dim0; i++)
    (*array4D)[i] = (*array4D)[i - 1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem5Dmv (sMotionVec****** array5D, int dim0, int dim1, int dim2, int dim3, int dim4) {

  int i, mem_size = dim0 * sizeof(sMotionVec***);
  if (((*array5D) = (sMotionVec*****)memAlloc(dim0 * sizeof(sMotionVec****))) == NULL)
    noMemoryExit("getMem5Dmv: array5D");

  mem_size += getMem4Dmv(*array5D, dim0 * dim1, dim2, dim3, dim4);

  for (i = 1; i < dim0; i++)
    (*array5D)[i] = (*array5D)[i - 1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem6Dmv (sMotionVec******* array6D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5) {

  int i, mem_size = dim0 * sizeof(sMotionVec*****);
  if (((*array6D) = (sMotionVec******)memAlloc(dim0 * sizeof(sMotionVec*****))) == NULL)
    noMemoryExit("getMem6Dmv: array6D");

  mem_size += getMem5Dmv(*array6D, dim0 * dim1, dim2, dim3, dim4, dim5);

  for (i = 1; i < dim0; i++)
    (*array6D)[i] = (*array6D)[i - 1] + dim1;

  return mem_size;
  }
//}}}
//{{{
int getMem7Dmv (sMotionVec******** array7D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6) {

  int i, mem_size = dim0 * sizeof(sMotionVec*****);
  if (((*array7D) = (sMotionVec*******)memAlloc(dim0 * sizeof(sMotionVec******))) == NULL)
    noMemoryExit("getMem7Dmv: array7D");

  mem_size += getMem6Dmv(*array7D, dim0 * dim1, dim2, dim3, dim4, dim5, dim6);

  for (i = 1; i < dim0; i++)
    (*array7D)[i] = (*array7D)[i - 1] + dim1;

  return mem_size;
  }
//}}}

//{{{
void freeMem2Dspp (sPicture*** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);
    memFree (array2D);
  }
}

//}}}
//{{{
void freeMem3Dspp (sPicture**** array3D)
{
  if (array3D)
  {
    freeMem2Dspp(*array3D);
    memFree (array3D);
  }
}
//}}}
//{{{
void freeMem2Dmv (sMotionVec** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);
    memFree (array2D);
  }
}
//}}}
//{{{
void freeMem3Dmv (sMotionVec*** array3D)
{
  if (array3D)
  {
    freeMem2Dmv(*array3D);
    memFree (array3D);
  }
}
//}}}
//{{{
void freeMem4Dmv (sMotionVec**** array4D)
{
  if (array4D)
  {
    freeMem3Dmv(*array4D);
    memFree (array4D);
  }
}
//}}}
//{{{
void freeMem5Dmv (sMotionVec***** array5D)
{
  if (array5D)
  {
    freeMem4Dmv(*array5D);
    memFree (array5D);
  }
}
//}}}
//{{{
void freeMem6Dmv (sMotionVec****** array6D)
{
  if (array6D)
  {
    freeMem5Dmv(*array6D);
    memFree (array6D);
  }
}
//}}}
//{{{
void freeMem7Dmv (sMotionVec******* array7D)
{
  if (array7D)
  {
    freeMem6Dmv(*array7D);
    memFree (array7D);
  }
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 1D memory array -> sPixel array1D[dim0
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************/
int getMem1Dpel (sPixel** array1D, int dim0)
{
  if((*array1D    = (sPixel*)mem_calloc(dim0,       sizeof(sPixel))) == NULL)
    noMemoryExit("getMem1Dpel: array1D");

  return (sizeof(sPixel*) + dim0 * sizeof(sPixel));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D memory array -> sPixel array2D[dim0][dim1]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************/
int getMem2Dpel (sPixel*** array2D, int dim0, int dim1)
{
  int i;

  if((*array2D    = (sPixel**)memAlloc(dim0 *        sizeof(sPixel*))) == NULL)
    noMemoryExit("getMem2Dpel: array2D");
  if((*(*array2D) = (sPixel* )memAlloc(dim0 * dim1 * sizeof(sPixel ))) == NULL)
    noMemoryExit("getMem2Dpel: array2D");

  for(i = 1 ; i < dim0; i++)
  {
    (*array2D)[i] = (*array2D)[i-1] + dim1;
  }

  return dim0 * (sizeof(sPixel*) + dim1 * sizeof(sPixel));
}
//}}}
//{{{
int getMem2DpelPad (sPixel*** array2D, int dim0, int dim1, int iPadY, int iPadX)
{
  int i;
  sPixel *curr = NULL;
  int height, width;

  height = dim0+2*iPadY;
  width = dim1+2*iPadX;
  if((*array2D    = (sPixel**)memAlloc(height*sizeof(sPixel*))) == NULL)
    noMemoryExit("getMem2DpelPad: array2D");
  if((*(*array2D) = (sPixel* )mem_calloc(height * width, sizeof(sPixel ))) == NULL)
    noMemoryExit("getMem2DpelPad: array2D");

  (*array2D)[0] += iPadX;
  curr = (*array2D)[0];
  for(i = 1 ; i < height; i++)
  {
    curr += width;
    (*array2D)[i] = curr;
  }
  (*array2D) = &((*array2D)[iPadY]);

  return height * (sizeof(sPixel*) + width * sizeof(sPixel));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 3D memory array -> sPixel array3D[dim0][dim1][dim2]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem3Dpel (sPixel**** array3D, int dim0, int dim1, int dim2)
{
  int i, mem_size = dim0 * sizeof(sPixel**);

  if(((*array3D) = (sPixel***)malloc(dim0 * sizeof(sPixel**))) == NULL)
    noMemoryExit("getMem3Dpel: array3D");

  mem_size += getMem2Dpel(*array3D, dim0 * dim1, dim2);

  for(i = 1; i < dim0; i++)
    (*array3D)[i] = (*array3D)[i - 1] + dim1;

  return mem_size;
}

int getMem3DpelPad(sPixel** **array3D, int dim0, int dim1, int dim2, int iPadY, int iPadX)
{
  int i, mem_size = dim0 * sizeof(sPixel**);

  if(((*array3D) = (sPixel***)memAlloc(dim0*sizeof(sPixel**))) == NULL)
    noMemoryExit("getMem3DpelPad: array3D");

  for(i = 0; i < dim0; i++)
    mem_size += getMem2DpelPad((*array3D)+i, dim1, dim2, iPadY, iPadX);

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 4D memory array -> sPixel array4D[dim0][dim1][dim2][dim3]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem4Dpel (sPixel***** array4D, int dim0, int dim1, int dim2, int dim3)
{
  int  i, mem_size = dim0 * sizeof(sPixel***);

  if(((*array4D) = (sPixel****)memAlloc(dim0 * sizeof(sPixel***))) == NULL)
    noMemoryExit("getMem4Dpel: array4D");

  mem_size += getMem3Dpel(*array4D, dim0 * dim1, dim2, dim3);

  for(i = 1; i < dim0; i++)
    (*array4D)[i] = (*array4D)[i - 1] + dim1;

  return mem_size;
}
//}}}
//{{{
int getMem4DpelPad (sPixel***** array4D, int dim0, int dim1, int dim2, int dim3, int iPadY, int iPadX)
{
  int  i, mem_size = dim0 * sizeof(sPixel***);

  if(((*array4D) = (sPixel****)memAlloc(dim0 * sizeof(sPixel***))) == NULL)
    noMemoryExit("getMem4DpelPad: array4D");

  mem_size += getMem3DpelPad(*array4D, dim0 * dim1, dim2, dim3, iPadY, iPadX);

  for(i = 1; i < dim0; i++)
    (*array4D)[i] = (*array4D)[i - 1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 5D memory array -> sPixel array5D[dim0][dim1][dim2][dim3][dim4]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem5Dpel (sPixel****** array5D, int dim0, int dim1, int dim2, int dim3, int dim4)
{
  int  i, mem_size = dim0 * sizeof(sPixel****);

  if(((*array5D) = (sPixel*****)memAlloc(dim0 * sizeof(sPixel****))) == NULL)
    noMemoryExit("getMem5Dpel: array5D");

  mem_size += getMem4Dpel(*array5D, dim0 * dim1, dim2, dim3, dim4);

  for(i = 1; i < dim0; i++)
    (*array5D)[i] = (*array5D)[i - 1] + dim1;

  return mem_size;
}
//}}}
//{{{
int getMem5DpelPad (sPixel****** array5D, int dim0, int dim1, int dim2, int dim3, int dim4, int iPadY, int iPadX)
{
  int  i, mem_size = dim0 * sizeof(sPixel****);

  if(((*array5D) = (sPixel*****)memAlloc(dim0 * sizeof(sPixel****))) == NULL)
    noMemoryExit("getMem5DpelPad: array5D");

  mem_size += getMem4DpelPad(*array5D, dim0 * dim1, dim2, dim3, dim4, iPadY, iPadX);

  for(i = 1; i < dim0; i++)
    (*array5D)[i] = (*array5D)[i - 1] + dim1;

  return mem_size;
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    free 1D memory array
 *    which was allocated with getMem1Dpel()
** **********************************************************************
 */
void freeMem1Dpel (sPixel* array1D)
{
  if (array1D)
    memFree (array1D);
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 2D memory array
 *    which was allocated with getMem2Dpel()
** **********************************************************************
 */
void freeMem2Dpel (sPixel** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);

    memFree (array2D);
  }
}
//}}}
//{{{
void freeMem2DpelPad (sPixel** array2D, int iPadY, int iPadX)
{
  if (array2D)
  {
    if (*array2D)
      memFree (array2D[-iPadY]-iPadX);
    memFree (&array2D[-iPadY]);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 3D memory array
 *    which was allocated with getMem3Dpel()
** **********************************************************************
 */
void freeMem3Dpel (sPixel*** array3D)
{
  if (array3D)
  {
    freeMem2Dpel(*array3D);
    memFree (array3D);
  }
}
//}}}
//{{{
void freeMem3DpelPad (sPixel*** array3D, int iDim12, int iPadY, int iPadX)
{
  if (array3D)
  {
    int i;
    for(i=0; i<iDim12; i++)
      if(array3D[i])
      {
        freeMem2DpelPad(array3D[i], iPadY, iPadX);
        array3D[i] = NULL;
      }
    memFree (array3D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 4D memory array
 *    which was allocated with getMem4Dpel()
** **********************************************************************
 */
void freeMem4Dpel (sPixel**** array4D)
{
  if (array4D)
  {
    freeMem3Dpel(*array4D);
    memFree (array4D);
  }
}
//}}}
//{{{
void freeMem4DpelPad (sPixel**** array4D, int iFrames, int iPadY, int iPadX)
{
  if (array4D)
  {
    freeMem3DpelPad(*array4D, iFrames, iPadY, iPadX);
    memFree (array4D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 5D memory array
 *    which was allocated with getMem5Dpel()
** **********************************************************************
 */
void freeMem5Dpel (sPixel***** array5D)
{
  if (array5D)
  {
    freeMem4Dpel(*array5D);
    memFree (array5D);
  }
}
//}}}
//{{{
void freeMem5DpelPad (sPixel***** array5D, int iFrames, int iPadY, int iPadX)
{
  if (array5D)
  {
    freeMem4DpelPad(*array5D, iFrames, iPadY, iPadX);
    memFree(array5D);
  }
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Create 2D memory array -> byte array2D[dim0][dim1]
 *
 * \par Output:
 *    byte type array of size dim0 * dim1
** **********************************************************************
 */
byte** new_mem2D (int dim0, int dim1)
{
  int i;
  byte** array2D;

  if((  array2D  = (byte**)memAlloc(dim0 *      sizeof(byte*))) == NULL)
    noMemoryExit("getMem2D: array2D");
  if((*(array2D) = (byte* )mem_calloc(dim0 * dim1,sizeof(byte ))) == NULL)
    noMemoryExit("getMem2D: array2D");

  for(i = 1; i < dim0; i++)
    array2D[i] = array2D[i-1] + dim1;

  return (array2D);
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Create 2D memory array -> int array2D[dim0][dim1]
 *
 * \par Output:
 *    int type array of size dim0 * dim1
** **********************************************************************
 */
int** new_mem2Dint (int dim0, int dim1)
{
  int i;
  int** array2D;

  if((array2D    = (int**)memAlloc(dim0 *       sizeof(int*))) == NULL)
    noMemoryExit("getMem2Dint: array2D");
  if((*(array2D) = (int* )mem_calloc(dim0 * dim1, sizeof(int ))) == NULL)
    noMemoryExit("getMem2Dint: array2D");

  for(i = 1 ; i < dim0; i++)
    (array2D)[i] =  (array2D)[i-1] + dim1;

  return (array2D);
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D memory array -> unsigned char array2D[dim0][dim1]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************/
int getMem2D (byte*** array2D, int dim0, int dim1)
{
  int i;

  if((  *array2D  = (byte**)memAlloc(dim0 *      sizeof(byte*))) == NULL)
    noMemoryExit("getMem2D: array2D");
  if((*(*array2D) = (byte* )mem_calloc(dim0 * dim1,sizeof(byte ))) == NULL)
    noMemoryExit("getMem2D: array2D");

  for(i = 1; i < dim0; i++)
    (*array2D)[i] = (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(byte*) + dim1 * sizeof(byte));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D memory array -> int array2D[dim0][dim1]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem2Dint (int*** array2D, int dim0, int dim1)
{
  int i;

  if((*array2D    = (int**)memAlloc(dim0 *       sizeof(int*))) == NULL)
    noMemoryExit("getMem2Dint: array2D");
  if((*(*array2D) = (int* )mem_calloc(dim0 * dim1, sizeof(int ))) == NULL)
    noMemoryExit("getMem2Dint: array2D");

  for(i = 1 ; i < dim0; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(int*) + dim1 * sizeof(int));
}
//}}}
//{{{
int getMem2Dint_pad (int*** array2D, int dim0, int dim1, int iPadY, int iPadX)
{
  int i;
  int *curr = NULL;
  int height, width;

  height = dim0+2*iPadY;
  width = dim1+2*iPadX;
  if((*array2D    = (int**)memAlloc(height*sizeof(int*))) == NULL)
    noMemoryExit("getMem2Dint_pad: array2D");
  if((*(*array2D) = (int* )mem_calloc(height * width, sizeof(int ))) == NULL)
    noMemoryExit("getMem2Dint_pad: array2D");

  (*array2D)[0] += iPadX;
  curr = (*array2D)[0];
  for(i = 1 ; i < height; i++)
  {
    curr += width;
    (*array2D)[i] = curr;
  }
  (*array2D) = &((*array2D)[iPadY]);

  return height * (sizeof(int*) + width * sizeof(int));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D memory array -> int64 array2D[dim0][dim1]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem2Dint64 (int64*** array2D, int dim0, int dim1)
{
  int i;

  if((*array2D    = (int64**)memAlloc(dim0 *      sizeof(int64*))) == NULL)
    noMemoryExit("getMem2Dint64: array2D");
  if((*(*array2D) = (int64* )mem_calloc(dim0 * dim1,sizeof(int64 ))) == NULL)
    noMemoryExit("getMem2Dint64: array2D");

  for(i = 1; i < dim0; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(int64*) + dim1 * sizeof(int64));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 3D memory array -> unsigned char array3D[dim0][dim1][dim2]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem3D (byte**** array3D, int dim0, int dim1, int dim2)
{
  int  i, mem_size = dim0 * sizeof(byte**);

  if(((*array3D) = (byte***)memAlloc(dim0 * sizeof(byte**))) == NULL)
    noMemoryExit("getMem3D: array3D");

  mem_size += getMem2D(*array3D, dim0 * dim1, dim2);

  for(i = 1; i < dim0; i++)
    (*array3D)[i] =  (*array3D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 4D memory array -> unsigned char array4D[dim0][dim1][dim2][dim3]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem4D (byte***** array4D, int dim0, int dim1, int dim2, int dim3)
{
  int  i, mem_size = dim0 * sizeof(byte***);

  if(((*array4D) = (byte****)memAlloc(dim0 * sizeof(byte***))) == NULL)
    noMemoryExit("getMem4D: array4D");

  mem_size += getMem3D(*array4D, dim0 * dim1, dim2, dim3);

  for(i = 1; i < dim0; i++)
    (*array4D)[i] =  (*array4D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 3D memory array -> int array3D[dim0][dim1][dim2]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem3Dint (int**** array3D, int dim0, int dim1, int dim2)
{
  int  i, mem_size = dim0 * sizeof(int**);

  if(((*array3D) = (int***)memAlloc(dim0 * sizeof(int**))) == NULL)
    noMemoryExit("getMem3Dint: array3D");

  mem_size += getMem2Dint(*array3D, dim0 * dim1, dim2);

  for(i = 1; i < dim0; i++)
    (*array3D)[i] =  (*array3D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 3D memory array -> int64 array3D[dim0][dim1][dim2]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem3Dint64 (int64**** array3D, int dim0, int dim1, int dim2)
{
  int  i, mem_size = dim0 * sizeof(int64**);

  if(((*array3D) = (int64***)memAlloc(dim0 * sizeof(int64**))) == NULL)
    noMemoryExit("getMem3Dint64: array3D");

  mem_size += getMem2Dint64(*array3D, dim0 * dim1, dim2);

  for(i = 1; i < dim0; i++)
    (*array3D)[i] =  (*array3D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
int getMem2Ddistblk (distblk*** array2D, int dim0, int dim1)
{
  int i;

  if((*array2D    = (distblk**)memAlloc(dim0 *      sizeof(distblk*))) == NULL)
    noMemoryExit("getMem2Ddistblk: array2D");
  if((*(*array2D) = (distblk* )mem_calloc(dim0 * dim1,sizeof(distblk ))) == NULL)
    noMemoryExit("getMem2Ddistblk: array2D");

  for(i = 1; i < dim0; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(distblk*) + dim1 * sizeof(distblk));
}
//}}}
//{{{
int getMem3Ddistblk (distblk**** array3D, int dim0, int dim1, int dim2)
{
  int  i, mem_size = dim0 * sizeof(distblk**);

  if(((*array3D) = (distblk***)memAlloc(dim0 * sizeof(distblk**))) == NULL)
    noMemoryExit("getMem3Ddistblk: array3D");

  mem_size += getMem2Ddistblk(*array3D, dim0 * dim1, dim2);

  for(i = 1; i < dim0; i++)
    (*array3D)[i] =  (*array3D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
int getMem4Ddistblk (distblk***** array4D, int dim0, int dim1, int dim2, int dim3)
{
  int  i, mem_size = dim0 * sizeof(distblk***);

  if(((*array4D) = (distblk****)memAlloc(dim0 * sizeof(distblk***))) == NULL)
    noMemoryExit("getMem4Ddistblk: array4D");

  mem_size += getMem3Ddistblk(*array4D, dim0 * dim1, dim2, dim3);

  for(i = 1; i < dim0; i++)
    (*array4D)[i] =  (*array4D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 4D memory array -> int array4D[dim0][dim1][dim2][dim3]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem4Dint (int***** array4D, int dim0, int dim1, int dim2, int dim3)
{
  int  i, mem_size = dim0 * sizeof(int***);

  if(((*array4D) = (int****)memAlloc(dim0 * sizeof(int***))) == NULL)
    noMemoryExit("getMem4Dint: array4D");

  mem_size += getMem3Dint(*array4D, dim0 * dim1, dim2, dim3);

  for(i = 1; i < dim0; i++)
    (*array4D)[i] =  (*array4D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
int getMem4Dint64 (int64***** array4D, int dim0, int dim1, int dim2, int dim3)
{
  int  i, mem_size = dim0 * sizeof(int64***);

  if(((*array4D) = (int64****)memAlloc(dim0 * sizeof(int64***))) == NULL)
    noMemoryExit("getMem4Dint64: array4D");

  mem_size += getMem3Dint64(*array4D, dim0 * dim1, dim2, dim3);

  for(i = 1; i < dim0; i++)
    (*array4D)[i] =  (*array4D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 5D memory array -> int array5D[dim0][dim1][dim2][dim3][dim4]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem5Dint (int****** array5D, int dim0, int dim1, int dim2, int dim3, int dim4)
{
  int  i, mem_size = dim0 * sizeof(int****);

  if(((*array5D) = (int*****)memAlloc(dim0 * sizeof(int****))) == NULL)
    noMemoryExit("getMem5Dint: array5D");

  mem_size += getMem4Dint(*array5D, dim0 * dim1, dim2, dim3, dim4);

  for(i = 1; i < dim0; i++)
    (*array5D)[i] =  (*array5D)[i-1] + dim1;

  return mem_size;
}

//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    free 2D memory array
 *    which was allocated with getMem2D()
** **********************************************************************
 */
void freeMem2D (byte** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);
    memFree (array2D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 2D memory array
 *    which was allocated with getMem2Dint()
** **********************************************************************
 */
void freeMem2Dint (int** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);
    memFree (array2D);
  }
}
//}}}
//{{{
void freeMem2Dint_pad (int** array2D, int iPadY, int iPadX)
{
  if (array2D)
  {
    if (*array2D)
      memFree (array2D[-iPadY]-iPadX);
    memFree (&array2D[-iPadY]);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 2D memory array
 *    which was allocated with getMem2Dint64()
** **********************************************************************
 */
void freeMem2Dint64 (int64** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);
    memFree (array2D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 3D memory array
 *    which was allocated with getMem3D()
** **********************************************************************
 */
void freeMem3D (byte*** array3D)
{
  if (array3D)
  {
   freeMem2D(*array3D);
   memFree (array3D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 4D memory array
 *    which was allocated with getMem3D()
** **********************************************************************
 */
void freeMem4D (byte**** array4D)
{
  if (array4D)
  {
   freeMem3D(*array4D);
   memFree (array4D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 3D memory array
 *    which was allocated with getMem3Dint()
** **********************************************************************
 */
void freeMem3Dint (int*** array3D)
{
  if (array3D)
  {
   freeMem2Dint(*array3D);
   memFree (array3D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 3D memory array
 *    which was allocated with getMem3Dint64()
** **********************************************************************
 */
void freeMem3Dint64 (int64*** array3D)
{
  if (array3D)
  {
   freeMem2Dint64(*array3D);
   memFree (array3D);
  }
}
//}}}
//{{{
void freeMem3Ddistblk (distblk*** array3D)
{
  if (array3D)
  {
   freeMem2Ddistblk(*array3D);
   memFree (array3D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 4D memory array
 *    which was allocated with getMem4Dint()
** **********************************************************************
 */
void freeMem4Dint (int**** array4D)
{
  if (array4D)
  {
    freeMem3Dint( *array4D);
    memFree (array4D);
  }
}
//}}}
//{{{
void freeMem4Dint64 (int64**** array4D)
{
  if (array4D)
  {
    freeMem3Dint64( *array4D);
    memFree (array4D);
  }
}
//}}}
//{{{
void freeMem4Ddistblk (distblk**** array4D)
{
  if (array4D)
  {
    freeMem3Ddistblk( *array4D);
    memFree (array4D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 5D int memory array
 *    which was allocated with getMem5Dint()
** **********************************************************************
 */
void freeMem5Dint (int***** array5D)
{
  if (array5D)
  {
    freeMem4Dint( *array5D);
    memFree (array5D);
  }
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Create 2D memory array -> uint16_t array2D[dim0][dim1]
 *
 * \par Output:
 *    uint16_t type array of size dim0 * dim1
** **********************************************************************
 */
uint16_t** new_mem2Duint16 (int dim0, int dim1)
{
  int i;
  uint16_t** array2D;

  if(( array2D = (uint16_t**)memAlloc(dim0 *      sizeof(uint16_t*))) == NULL)
    noMemoryExit("getMem2Duint16: array2D");
  if((*array2D = (uint16_t* )mem_calloc(dim0 * dim1,sizeof(uint16_t ))) == NULL)
    noMemoryExit("getMem2Duint16: array2D");

  for(i = 1; i < dim0; i++)
    array2D[i] = array2D[i-1] + dim1;

  return (array2D);
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D uint16_t memory array -> uint16_t array2D[dim0][dim1]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem2Duint16 (uint16_t*** array2D, int dim0, int dim1)
{
  int i;

  if((  *array2D  = (uint16_t**)memAlloc(dim0 *      sizeof(uint16_t*))) == NULL)
    noMemoryExit("getMem2Duint16: array2D");

  if((*(*array2D) = (uint16_t* )mem_calloc(dim0 * dim1,sizeof(uint16_t ))) == NULL)
    noMemoryExit("getMem2Duint16: array2D");

  for(i = 1; i < dim0; i++)
    (*array2D)[i] = (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(uint16_t*) + dim1 * sizeof(uint16_t));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 3D memory uint16_t array -> uint16_t array3D[dim0][dim1][dim2]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem3Duint16 (uint16_t**** array3D,int dim0, int dim1, int dim2)
{
  int  i, mem_size = dim0 * sizeof(uint16_t**);

  if(((*array3D) = (uint16_t***)memAlloc(dim0 * sizeof(uint16_t**))) == NULL)
    noMemoryExit("getMem3Duint16: array3D");

  mem_size += getMem2Duint16(*array3D, dim0 * dim1, dim2);

  for(i = 1; i < dim0; i++)
    (*array3D)[i] =  (*array3D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 4D memory uint16_t array -> uint16_t array3D[dim0][dim1][dim2][dim3]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem4Duint16 (uint16_t***** array4D, int dim0, int dim1, int dim2, int dim3)
{
  int  i, mem_size = dim0 * sizeof(uint16_t***);

  if(((*array4D) = (uint16_t****)memAlloc(dim0 * sizeof(uint16_t***))) == NULL)
    noMemoryExit("getMem4Duint16: array4D");

  mem_size += getMem3Duint16(*array4D, dim0 * dim1, dim2, dim3);

  for(i = 1; i < dim0; i++)
    (*array4D)[i] =  (*array4D)[i-1] + dim1;

  return mem_size;
}

//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D short memory array -> short array2D[dim0][dim1]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem2Dshort (short*** array2D, int dim0, int dim1)
{
  int i;
  short *curr = NULL;

  if((  *array2D  = (short**)memAlloc(dim0 *      sizeof(short*))) == NULL)
    noMemoryExit("getMem2Dshort: array2D");
  if((*(*array2D) = (short* )mem_calloc(dim0 * dim1,sizeof(short ))) == NULL)
    noMemoryExit("getMem2Dshort: array2D");

  curr = (*array2D)[0];
  for(i = 1; i < dim0; i++)
  {
    curr += dim1;
    (*array2D)[i] = curr;
  }

  return dim0 * (sizeof(short*) + dim1 * sizeof(short));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 3D memory short array -> short array3D[dim0][dim1][dim2]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem3Dshort (short**** array3D,int dim0, int dim1, int dim2)
{
  int  i, mem_size = dim0 * sizeof(short**);
  short** curr = NULL;

  if(((*array3D) = (short***)memAlloc(dim0 * sizeof(short**))) == NULL)
    noMemoryExit("getMem3Dshort: array3D");

  mem_size += getMem2Dshort(*array3D, dim0 * dim1, dim2);

  curr = (*array3D)[0];
  for(i = 1; i < dim0; i++)
  {
    curr += dim1;
    (*array3D)[i] = curr;
  }

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 4D memory short array -> short array3D[dim0][dim1][dim2][dim3]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem4Dshort (short***** array4D, int dim0, int dim1, int dim2, int dim3)
{
  int  i, mem_size = dim0 * sizeof(short***);

  if(((*array4D) = (short****)memAlloc(dim0 * sizeof(short***))) == NULL)
    noMemoryExit("getMem4Dshort: array4D");

  mem_size += getMem3Dshort(*array4D, dim0 * dim1, dim2, dim3);

  for(i = 1; i < dim0; i++)
    (*array4D)[i] =  (*array4D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 5D memory array -> short array5D[dim0][dim1][dim2][dim3][dim4]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem5Dshort (short****** array5D, int dim0, int dim1, int dim2, int dim3, int dim4)
{
  int  i, mem_size = dim0 * sizeof(short****);

  if(((*array5D) = (short*****)memAlloc(dim0 * sizeof(short****))) == NULL)
    noMemoryExit("getMem5Dshort: array5D");

  mem_size += getMem4Dshort(*array5D, dim0 * dim1, dim2, dim3, dim4);

  for(i = 1; i < dim0; i++)
    (*array5D)[i] =  (*array5D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 6D memory array -> short array6D[dim0][dim1][dim2][dim3][dim4][dim5]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem6Dshort (short******* array6D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5)
{
  int  i, mem_size = dim0 * sizeof(short*****);

  if(((*array6D) = (short******)memAlloc(dim0 * sizeof(short*****))) == NULL)
    noMemoryExit("getMem6Dshort: array6D");

  mem_size += getMem5Dshort(*array6D, dim0 * dim1, dim2, dim3, dim4, dim5);

  for(i = 1; i < dim0; i++)
    (*array6D)[i] =  (*array6D)[i-1] + dim1;

  return mem_size;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 7D memory array -> short array7D[dim0][dim1][dim2][dim3][dim4][dim5][dim6]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem7Dshort (short******** array7D, int dim0, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
  int  i, mem_size = dim0 * sizeof(short******);

  if(((*array7D) = (short*******)memAlloc(dim0 * sizeof(short******))) == NULL)
    noMemoryExit("getMem7Dshort: array7D");

  mem_size += getMem6Dshort(*array7D, dim0 * dim1, dim2, dim3, dim4, dim5, dim6);

  for(i = 1; i < dim0; i++)
    (*array7D)[i] =  (*array7D)[i-1] + dim1;

  return mem_size;
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    free 2D uint16_t memory array
 *    which was allocated with getMem2Duint16()
** **********************************************************************
 */
void freeMem2Duint16 (uint16_t** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);

    memFree (array2D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 3D uint16_t memory array
 *    which was allocated with getMem3Duint16()
** **********************************************************************
 */
void freeMem3Duint16 (uint16_t*** array3D)
{
  if (array3D)
  {
   freeMem2Duint16(*array3D);
   memFree (array3D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 4D uint16_t memory array
 *    which was allocated with getMem4Duint16()
** **********************************************************************
 */
void freeMem4Duint16 (uint16_t**** array4D)
{
  if (array4D)
  {
    freeMem3Duint16( *array4D);
    memFree (array4D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 2D short memory array
 *    which was allocated with getMem2Dshort()
** **********************************************************************
 */
void freeMem2Dshort (short** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);

    memFree (array2D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 3D short memory array
 *    which was allocated with getMem3Dshort()
** **********************************************************************
 */
void freeMem3Dshort (short*** array3D)
{
  if (array3D)
  {
   freeMem2Dshort(*array3D);
   memFree (array3D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 4D short memory array
 *    which was allocated with getMem4Dshort()
** **********************************************************************
 */
void freeMem4Dshort (short**** array4D)
{
  if (array4D)
  {
    freeMem3Dshort( *array4D);
    memFree (array4D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 5D short memory array
 *    which was allocated with getMem5Dshort()
** **********************************************************************
 */
void freeMem5Dshort (short***** array5D)
{
  if (array5D)
  {
    freeMem4Dshort( *array5D) ;
    memFree (array5D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 6D short memory array
 *    which was allocated with getMem6Dshort()
** **********************************************************************
 */
void freeMem6Dshort (short****** array6D)
{
  if (array6D)
  {
    freeMem5Dshort( *array6D);
    memFree (array6D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 7D short memory array
 *    which was allocated with getMem7Dshort()
** **********************************************************************
 */
void freeMem7Dshort (short******* array7D)
{
  if (array7D)
  {
    freeMem6Dshort( *array7D);
    memFree (array7D);
  }
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D memory array -> double array2D[dim0][dim1]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem2Ddouble (double*** array2D, int dim0, int dim1)
{
  int i;

  if((*array2D      = (double**)memAlloc(dim0 *       sizeof(double*))) == NULL)
    noMemoryExit("getMem2Ddouble: array2D");

  if(((*array2D)[0] = (double* )mem_calloc(dim0 * dim1, sizeof(double ))) == NULL)
    noMemoryExit("getMem2Ddouble: array2D");

  for(i=1 ; i<dim0 ; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1  ;

  return dim0 * (sizeof(double*) + dim1 * sizeof(double));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 1D memory array -> double array1D[dim0]
 *    Note that array is shifted towards offset allowing negative values
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem1Dodouble (double** array1D, int dim0, int offset)
{
  if((*array1D      = (double*)mem_calloc(dim0, sizeof(double))) == NULL)
    noMemoryExit("getMem1Dodouble: array2D");

  *array1D += offset;

  return dim0 * sizeof(double);
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D memory array -> double array2D[dim0][dim1]
 *    Note that array is shifted towards offset allowing negative values
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem2Dodouble (double*** array2D, int dim0, int dim1, int offset)
{
  int i;

  if((*array2D      = (double**)memAlloc(dim0 *      sizeof(double*))) == NULL)
    noMemoryExit("getMem2Dodouble: array2D");
  if(((*array2D)[0] = (double* )mem_calloc(dim0 * dim1,sizeof(double ))) == NULL)
    noMemoryExit("getMem2Dodouble: array2D");

  (*array2D)[0] += offset;

  for(i=1 ; i<dim0 ; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1  ;

  return dim0 * (sizeof(double*) + dim1 * sizeof(double));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 3D memory double array -> double array3D[dim0][dim1][dim2]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem3Dodouble (double**** array3D, int dim0, int dim1, int dim2, int offset)
{
  int  i,j;

  if(((*array3D) = (double***)memAlloc(dim0 * sizeof(double**))) == NULL)
    noMemoryExit("getMem3Dodouble: array3D");

  if(((*array3D)[0] = (double** )mem_calloc(dim0 * dim1,sizeof(double*))) == NULL)
    noMemoryExit("getMem3Dodouble: array3D");

  (*array3D) [0] += offset;

  for(i=1 ; i<dim0 ; i++)
    (*array3D)[i] =  (*array3D)[i-1] + dim1  ;

  for (i = 0; i < dim0; i++)
    for (j = -offset; j < dim1 - offset; j++)
      if(((*array3D)[i][j] = (double* )mem_calloc(dim2, sizeof(double))) == NULL)
        noMemoryExit("getMem3Dodouble: array3D");

  return dim0*( sizeof(double**) + dim1 * ( sizeof(double*) + dim2 * sizeof(double)));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D memory array -> int array2D[dim0][dim1]
 *    Note that array is shifted towards offset allowing negative values
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int get_offset_mem2Dshort (short*** array2D, int dim0, int dim1, int offset_y, int offset_x)
{
  int i;

  if((*array2D      = (short**)memAlloc(dim0 * sizeof(short*))) == NULL)
    noMemoryExit("get_offset_mem2Dshort: array2D");

  if(((*array2D)[0] = (short* )mem_calloc(dim0 * dim1,sizeof(short))) == NULL)
    noMemoryExit("get_offset_mem2Dshort: array2D");
  (*array2D)[0] += offset_x + offset_y * dim1;

  for(i=-1 ; i > -offset_y - 1; i--)
  {
    (*array2D)[i] =  (*array2D)[i+1] - dim1;
  }

  for(i=1 ; i < dim1 - offset_y; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1;

  return dim0 * (sizeof(short*) + dim1 * sizeof(short));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 3D memory int array -> int array3D[dim0][dim1][dim2]
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem3Doint (int**** array3D, int dim0, int dim1, int dim2, int offset)
{
  int  i,j;

  if(((*array3D) = (int***)memAlloc(dim0 * sizeof(int**))) == NULL)
    noMemoryExit("getMem3Doint: array3D");

  if(((*array3D)[0] = (int** )mem_calloc(dim0 * dim1,sizeof(int*))) == NULL)
    noMemoryExit("getMem3Doint: array3D");

  (*array3D) [0] += offset;

  for(i=1 ; i<dim0 ; i++)
    (*array3D)[i] =  (*array3D)[i-1] + dim1  ;

  for (i = 0; i < dim0; i++)
    for (j = -offset; j < dim1 - offset; j++)
      if(((*array3D)[i][j] = (int* )mem_calloc(dim2,sizeof(int))) == NULL)
        noMemoryExit("getMem3Doint: array3D");

  return dim0 * (sizeof(int**) + dim1 * (sizeof(int*) + dim2 * sizeof(int)));
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocate 2D memory array -> int array2D[dim0][dim1]
 *    Note that array is shifted towards offset allowing negative values
 *
 * \par Output:
 *    memory size in bytes
** **********************************************************************
 */
int getMem2Doint (int*** array2D, int dim0, int dim1, int offset)
{
  int i;

  if((*array2D      = (int**)memAlloc(dim0 * sizeof(int*))) == NULL)
    noMemoryExit("getMem2Dint: array2D");
  if(((*array2D)[0] = (int* )mem_calloc(dim0 * dim1,sizeof(int))) == NULL)
    noMemoryExit("getMem2Dint: array2D");

  (*array2D)[0] += offset;

  for(i=1 ; i<dim0 ; i++)
    (*array2D)[i] =  (*array2D)[i-1] + dim1  ;

  return dim0 * (sizeof(int*) + dim1 * sizeof(int));
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    free 2D double memory array
 *    which was allocated with getMem2Ddouble()
** **********************************************************************
 */
void freeMem2Ddouble (double** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);

    memFree (array2D);

  }
}
//}}}
//{{{
/*!
************************************************************************
* \brief
*    free 1D double memory array (with offset)
*    which was allocated with getMem1Ddouble()
************************************************************************
*/
void freeMem1Dodouble (double* array1D, int offset)
{
  if (array1D)
  {
    array1D -= offset;
    memFree (array1D);
  }
}
//}}}
//{{{
/*!
************************************************************************
* \brief
*    free 2D double memory array (with offset)
*    which was allocated with getMem2Ddouble()
************************************************************************
*/
void freeMem2Dodouble (double** array2D, int offset)
{
  if (array2D)
  {
    array2D[0] -= offset;
    if (array2D[0])
      memFree (array2D[0]);

    memFree (array2D);

  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 3D memory array with offset
** **********************************************************************
 */
void freeMem3Dodouble (double*** array3D, int dim0, int dim1, int offset)
{
  int i, j;

  if (array3D)
  {
    for (i = 0; i < dim0; i++)
    {
      for (j = -offset; j < dim1 - offset; j++)
      {
        if (array3D[i][j])
          memFree (array3D[i][j]);
      }
    }
    array3D[0] -= offset;
    if (array3D[0])
      memFree (array3D[0]);
    memFree (array3D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 3D memory array with offset
** **********************************************************************
 */
void freeMem3Doint (int*** array3D, int dim0, int dim1, int offset)
{
  int i, j;

  if (array3D)
  {
    for (i = 0; i < dim0; i++)
    {
      for (j = -offset; j < dim1 - offset; j++)
      {
        if (array3D[i][j])
          memFree (array3D[i][j]);
      }
    }
    array3D[0] -= offset;
    if (array3D[0])
      memFree (array3D[0]);
    memFree (array3D);
  }
}
//}}}
//{{{
/*!
************************************************************************
* \brief
*    free 2D double memory array (with offset)
*    which was allocated with getMem2Ddouble()
************************************************************************
*/
void freeMem2Doint (int** array2D, int offset)
{
  if (array2D)
  {
    array2D[0] -= offset;
    if (array2D[0])
      memFree (array2D[0]);
    memFree (array2D);

  }
}
//}}}

//{{{
/*!
************************************************************************
* \brief
*    free 2D double memory array (with offset)
*    which was allocated with getMem2Ddouble()
************************************************************************
*/
void free_offset_mem2Dshort (short** array2D, int dim1, int offset_y, int offset_x)
{
  if (array2D)
  {
    array2D[0] -= offset_x + offset_y * dim1;
    if (array2D[0])
      memFree (array2D[0]);

    memFree (array2D);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    free 3D memory array
 *    which was alocated with getMem3Dint()
** **********************************************************************
 */
void freeMem3Ddouble (double*** array3D)
{
  if (array3D)
  {
    freeMem2Ddouble(*array3D);
    memFree (array3D);
  }
}
//}}}
//{{{
void freeMem2Ddistblk (distblk** array2D)
{
  if (array2D)
  {
    if (*array2D)
      memFree (*array2D);
    free (array2D);
  }
}
//}}}
