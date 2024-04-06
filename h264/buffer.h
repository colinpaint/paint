#pragma once
#include "global.h"

//{{{
// compares two stored pictures by picture number for qsort in descending order
static inline int comparePicByPicNumDescending (const void* arg1, const void* arg2) {

  int pic_num1 = (*(sPicture**)arg1)->picNum;
  int pic_num2 = (*(sPicture**)arg2)->picNum;

  if (pic_num1 < pic_num2)
    return 1;

  if (pic_num1 > pic_num2)
    return -1;
  else
    return 0;
  }
//}}}
//{{{
// compares two stored pictures by picture number for qsort in descending order
static inline int comparePicByLtPicNumAscending (const void* arg1, const void* arg2) {

  int long_term_pic_num1 = (*(sPicture**)arg1)->longTermPicNum;
  int long_term_pic_num2 = (*(sPicture**)arg2)->longTermPicNum;

  if (long_term_pic_num1 < long_term_pic_num2)
    return -1;

  if ( long_term_pic_num1 > long_term_pic_num2)
    return 1;
  else
    return 0;
  }
//}}}
//{{{
// compares two frame stores by picNum for qsort in descending order
static inline int compareFsByFrameNumDescending (const void* arg1, const void* arg2) {

  int frame_num_wrap1 = (*(cFrameStore**)arg1)->frameNumWrap;
  int frame_num_wrap2 = (*(cFrameStore**)arg2)->frameNumWrap;

  if ( frame_num_wrap1 < frame_num_wrap2)
    return 1;

  if ( frame_num_wrap1 > frame_num_wrap2)
    return -1;
  else
    return 0;
  }
//}}}
//{{{
// compares two frame stores by lt_pic_num for qsort in descending order
static inline int compareFsbyLtPicIndexAscending (const void* arg1, const void* arg2) {

  int long_term_frame_idx1 = (*(cFrameStore**)arg1)->longTermFrameIndex;
  int long_term_frame_idx2 = (*(cFrameStore**)arg2)->longTermFrameIndex;

  if ( long_term_frame_idx1 < long_term_frame_idx2)
    return -1;
  else if ( long_term_frame_idx1 > long_term_frame_idx2)
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
static inline int comparePicByPocdesc (const void* arg1, const void* arg2) {

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

  int poc1 = (*(cFrameStore**)arg1)->poc;
  int poc2 = (*(cFrameStore**)arg2)->poc;

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
static inline int comparefsByPocdesc (const void* arg1, const void* arg2) {

  int poc1 = (*(cFrameStore**)arg1)->poc;
  int poc2 = (*(cFrameStore**)arg2)->poc;

  if (poc1 < poc2)
    return 1;
  else if (poc1 > poc2)
    return -1;
  else
    return 0;
  }
//}}}
static inline int isLongRef (sPicture* picture) { return picture->usedForReference && picture->usedLongTerm; }
static inline int isShortRef (sPicture* picture) { return picture->usedForReference && !picture->usedLongTerm; }

sPicture* allocPicture (cDecoder264* decoder, ePicStructure type, int sizeX, int sizeY, int sizeXcr, int sizeYcr, int isOutput);
void freePicture (sPicture* picture);
void fillFrameNumGap (cDecoder264* decoder, cSlice *slice);

void updateRefList (sDpb* dpb);
void updateLongTermRefList (sDpb* dpb);
void getSmallestPoc (sDpb* dpb, int* poc, int* pos);

void initDpb (cDecoder264* decoder, sDpb* dpb, int type);
void reInitDpb (cDecoder264* decoder, sDpb* dpb, int type);
void flushDpb (sDpb* dpb);
int removeUnusedDpb (sDpb* dpb);
void storePictureDpb (sDpb* dpb, sPicture* picture);
void removeFrameDpb (sDpb* dpb, int pos);
void freeDpb (sDpb* dpb);

void initListsSliceI (cSlice* slice);
void initListsSliceP (cSlice* slice);
void initListsSliceB (cSlice* slice);
void updatePicNum (cSlice* slice);

void reorderRefPicList (cSlice* slice, int curList);
sPicture* getShortTermPic (cSlice* slice, sDpb* dpb, int picNum);

void computeColocated (cSlice* slice, sPicture** listX[6]);
