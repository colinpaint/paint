#pragma once
#include "global.h"

#define MAX_LIST_SIZE 33
//{{{  sPicMotionOld
typedef struct PicMotionOld {
  byte* mbField; // field macroblock indicator
  } sPicMotionOld;
//}}}
//{{{  sPicMotion
typedef struct PicMotion {
  struct Picture* refPic[2];   // referrence picture pointer
  sMotionVec      mv[2];       // motion vector
  char            refIndex[2]; // reference picture   [list][subblockY][subblockX]
  byte            slice_no;
  } sPicMotion;
//}}}
//{{{  sPicture
typedef struct Picture {
  ePicStructure picStructure;

  int           poc;
  int           topPoc;
  int           botPoc;
  int           framePoc;
  unsigned int  frameNum;
  unsigned int  recoveryFrame;

  int           picNum;
  int           longTermPicNum;
  int           longTermFrameIndex;

  byte          isLongTerm;
  int           usedForRef;
  int           isOutput;
  int           non_existing; 
  int           isSeperateColourPlane;

  short         maxSliceId;

  int           sizeX;
  int           sizeY;
  int           sizeXcr;
  int           sizeYcr;
  int           sizeXm1, sizeYm1, sizeXCrm1, sizeYCrm1;
  int           codedFrame;
  int           mbAffFrame;
  unsigned      picWidthMbs;
  unsigned      picSizeInMbs;
  int           iLumaPadX;
  int           iLumaPadY;
  int           iChromaPadX;
  int           iChromaPadY;

  sPixel**      imgY;
  sPixel***     imgUV;

  struct Picture* frame;     // for mb aff, if field for referencing the combined frame
  struct Picture* topField;  // for mb aff, if frame for referencing the top field
  struct Picture* botField;  // for mb aff, if frame for referencing the bottom field

  sPicMotion**  mvInfo;
  sPicMotionOld motion;

  sPicMotion**  JVmv_info[MAX_PLANE];
  sPicMotionOld JVmotion[MAX_PLANE]; // Motion info for 4:4:4 independent mode decoding

  int           sliceType;
  int           isIDR;
  int           noOutputPriorPicFlag;
  int           longTermRefFlag;
  int           adaptRefPicBufFlag;

  eYuvFormat    chromaFormatIdc;
  int           frameMbOnly;
  int           cropFlag;
  int           cropLeft;
  int           cropRight;
  int           cropTop;
  int           cropBot;
  int           qp;
  int           chromaQpOffset[2];
  int           sliceQpDelta;
  sDecodedRefPicMarking* decRefPicMarkingBuffer;  // stores the memory management control operations

  // picture error conceal
  int           concealedPic;
  int           procFlag;
  int           iLumaStride;
  int           iChromaStride;
  int           iLumaExpandedHeight;
  int           iChromaExpandedHeight;
  sPixel**      curPixelY;               // for more efficient get_block_luma
  int           noRef;
  int           iCodingType;

  char             listXsize[MAX_NUM_SLICES][2];
  struct Picture** listX[MAX_NUM_SLICES][2];
  } sPicture;
//}}}
//{{{  sFrameStore
typedef struct FrameStore {
  int       isUsed;        // 0=empty; 1=top; 2=bottom; 3=both fields (or frame)
  int       isRef;   // 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       isLongTerm;    // 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       isOriginalRef; // original marking by nalRefIdc: 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used

  int       isNonExistent;

  unsigned  frameNum;
  unsigned  recoveryFrame;

  int       frameNumWrap;
  int       longTermFrameIndex;
  int       isOutput;
  int       poc;

  // picture error conceal
  int       concealRef;

  sPicture* frame;
  sPicture* topField;
  sPicture* botField;
  } sFrameStore;
//}}}
//{{{  sDPB
// DecodedPictureBuffer
typedef struct DPB {
  sDecoder*     decoder;

  sFrameStore** fs;
  sFrameStore** fsRef;
  sFrameStore** fsLongTermRef;

  unsigned      size;
  unsigned      usedSize;
  unsigned      refFramesInBuffer;
  unsigned      longTermRefFramesInBuffer;

  int           lastOutputPoc;
  int           maxLongTermPicIndex;
  int           initDone;
  int           numRefFrames;

  sFrameStore* lastPicture;
  } sDPB;
//}}}

//{{{
// compares two stored pictures by picture number for qsort in descending order
static inline int comparePicByPicNumDescending (const void* arg1, const void* arg2) {

  int picNum1 = (*(sPicture**)arg1)->picNum;
  int picNum2 = (*(sPicture**)arg2)->picNum;

  if (picNum1 < picNum2)
    return 1;

  if (picNum1 > picNum2)
    return -1;
  else
    return 0;
  }
//}}}
//{{{
// compares two stored pictures by picture number for qsort in descending order
static inline int comparePicByLtPicNumAscending (const void* arg1, const void* arg2) {

  int longTermPicNum1 = (*(sPicture**)arg1)->longTermPicNum;
  int longTermPicNum2 = (*(sPicture**)arg2)->longTermPicNum;

  if (longTermPicNum1 < longTermPicNum2)
    return -1;

  if (longTermPicNum1 > longTermPicNum2)
    return 1;
  else
    return 0;
  }
//}}}
//{{{
// compares two frame stores by picNum for qsort in descending order
static inline int compareFsByFrameNumDescending (const void* arg1, const void* arg2) {

  int frameNumWrap1 = (*(sFrameStore**)arg1)->frameNumWrap;
  int frameNumWrap2 = (*(sFrameStore**)arg2)->frameNumWrap;

  if (frameNumWrap1 < frameNumWrap2)
    return 1;

  if (frameNumWrap1 > frameNumWrap2)
    return -1;
  else
    return 0;
  }
//}}}
//{{{
// compares two frame stores by lt_pic_num for qsort in descending order
static inline int compareFsByLongTermPicIndexAscending (const void* arg1, const void* arg2) {

  int longTermFrameIndex1 = (*(sFrameStore**)arg1)->longTermFrameIndex;
  int longTermFrameIndex2 = (*(sFrameStore**)arg2)->longTermFrameIndex;

  if (longTermFrameIndex1 < longTermFrameIndex2)
    return -1;
  else if (longTermFrameIndex1 > longTermFrameIndex2)
    return 1;
  else
    return 0;
  }
//}}}
//{{{
// compares two stored pictures by poc for qsort in ascending order
static inline int comparePicByPocAscending (const void* arg1, const void* arg2) {

  int poc1 = (*(sPicture**)arg1)->poc;
  int poc2 = (*(sPicture**)arg2)->poc;

  if ( poc1 < poc2)
    return -1;
  else if ( poc1 > poc2)
    return 1;
  else
    return 0;
  }
//}}}
//{{{
// compares two stored pictures by poc for qsort in descending order
static inline int comparePicByPocDescending (const void* arg1, const void* arg2) {

  int poc1 = (*(sPicture**)arg1)->poc;
  int poc2 = (*(sPicture**)arg2)->poc;

  if (poc1 < poc2)
    return 1;
  else if (poc1 > poc2)
    return -1;
  else
    return 0;
  }
//}}}
//{{{
// compares two frame stores by poc for qsort in ascending order
static inline int compareFsByPocAscending (const void* arg1, const void* arg2) {

  int poc1 = (*(sFrameStore**)arg1)->poc;
  int poc2 = (*(sFrameStore**)arg2)->poc;

  if (poc1 < poc2)
    return -1;
  else if (poc1 > poc2)
    return 1;
  else
    return 0;
  }
//}}}
//{{{
// compares two frame stores by poc for qsort in descending order
static inline int compareFsByPocDescending (const void* arg1, const void* arg2) {

  int poc1 = (*(sFrameStore**)arg1)->poc;
  int poc2 = (*(sFrameStore**)arg2)->poc;

  if (poc1 < poc2)
    return 1;
  else if (poc1 > poc2)
    return -1;
  else
    return 0;
  }
//}}}
static inline int isLongRef (sPicture* picture) { return picture->usedForRef && picture->isLongTerm; }
static inline int isShortRef (sPicture* picture) { return picture->usedForRef && !picture->isLongTerm; }

extern void unmarkForRef( sFrameStore* frameStore);
extern void unmarkForLongTermRef (sFrameStore* frameStore);

extern sFrameStore* allocFrameStore();
extern void freeFrameStore (sFrameStore* frameStore);
extern void dpbCombineFields (sDecoder* decoder, sFrameStore* frameStore);

extern sPicture* allocPicture (sDecoder* decoder, ePicStructure type, int sizeX, int sizeY, int sizeXcr, int sizeYcr, int isOutput);
extern void freePicture (sPicture* picture);
extern void fillFrameNumGap (sDecoder* decoder, sSlice *slice);

extern void updateRefList (sDPB* dpb);
extern void updateLongTermRefList (sDPB* dpb);
extern void getSmallestPoc (sDPB* dpb, int *poc,int * pos);

extern void initDpb (sDecoder* decoder, sDPB* dpb, int type);
extern void reInitDpb (sDecoder* decoder, sDPB* dpb, int type);
extern void flushDpb (sDPB* dpb);
extern int removeUnusedDpb (sDPB* dpb);
extern void storePictureDpb (sDPB* dpb, sPicture* p);
extern void removeFrameDpb (sDPB* dpb, int pos);
extern void freeDpb (sDPB* dpb);

extern void initImage (sDecoder* decoder, sImage* image, sSps *sps);
extern void freeImage (sDecoder* decoder, sImage* image);

extern void initListsSliceI (sSlice* slice);
extern void initListsSliceP (sSlice* slice);
extern void initListsSliceB (sSlice* slice);
extern void updatePicNum (sSlice* slice);

extern void reorderRefPicList (sSlice* slice, int curList);
extern void initMbAffLists (sDecoder* decoder, sSlice* slice);
extern sPicture* getShortTermPic (sSlice* slice, sDPB* dpb, int picNum);

extern void computeColocated (sSlice* slice, sPicture** listX[6]);

extern void allocRefPicListReorderBuffer (sSlice* slice);
extern void freeRefPicListReorderBuffer (sSlice* slice);
