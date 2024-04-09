#pragma once
struct sPicture;
class cDecoder;

//{{{
enum eYuvFormat {
  CF_UNKNOWN = -1, // Unknown color format
  YUV400     =  0, // Monochrome
  YUV420     =  1, // 4:2:0
  YUV422     =  2, // 4:2:2
  YUV444     =  3  // 4:4:4
  };
//}}}

//{{{
struct sMotionVec {
  int16_t mvX;
  int16_t mvY;
  };
//}}}
//{{{
struct sPicMotionOld {
  uint8_t* mbField; // field macroBlock indicator
  };
//}}}
//{{{
struct sPicMotion {
  sPicture*  refPic[2];   // referrence picture pointer
  sMotionVec mv[2];       // motion vector
  char       refIndex[2]; // reference picture   [list][subblockY][subblockX]
  uint8_t    slice_no;
  };
//}}}
//{{{
struct sPicture {
  ePicStructure picStructure;

  int           poc;
  int           topPoc;
  int           botPoc;
  int           framePoc;
  uint32_t      frameNum;
  uint32_t      recoveryFrame;

  int           picNum;
  int           longTermPicNum;
  int           longTermFrameIndex;

  uint8_t       usedLongTermRef;
  int           usedForRef;
  int           isOutput;
  int           nonExisting;
  int           isSeperateColourPlane;

  int16_t       maxSliceId;

  int           sizeX;
  int           sizeY;
  int           sizeXcr;
  int           sizeYcr;
  int           size_x_m1, size_y_m1, size_x_cr_m1, size_y_cr_m1;
  int           codedFrame;
  int           mbAffFrame;
  uint32_t      picWidthMbs;
  uint32_t      picSizeInMbs;
  int           lumaPadX;
  int           lumaPadY;
  int           chromaPadX;
  int           chromaPadY;

  sPixel**      imgY;
  sPixel***     imgUV;
  sPicMotion**  mvInfo;
  sPicMotionOld motion;
  sPicture*     topField;  // for mb aff, if frame for referencing the top field
  sPicture*     botField;  // for mb aff, if frame for referencing the bottom field
  sPicture*     frame;     // for mb aff, if field for referencing the combined frame

  int           isIDR;
  int           sliceType;
  int           longTermRefFlag;
  int           adaptRefPicBufFlag;
  int           noOutputPriorPicFlag;

  eYuvFormat    chromaFormatIdc;
  int           frameMbOnly;

  int           hasCrop;
  int           cropLeft;
  int           cropRight;
  int           cropTop;
  int           cropBot;

  int           qp;
  int           chromaQpOffset[2];
  int           sliceQpDelta;
  sDecodedRefPicMark* decRefPicMarkBuffer;  // stores the memory management control operations

  // picture error conceal
  int           lumaStride;
  int           chromaStride;
  int           lumaExpandedHeight;
  int           chromaExpandedHeight;
  sPixel**      curPixelY;               // for more efficient get_block_luma
  int           noRef;
  int           codingType;

  char          listXsize[MAX_NUM_SLICES][2];
  sPicture**    listX[MAX_NUM_SLICES][2];

  // Motion info for 4:4:4 independent mode decoding
  sPicMotion**  mvInfoJV[MAX_PLANE];
  sPicMotionOld motionJV[MAX_PLANE];
  };
//}}}

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
//{{{
static inline int isLongRef (sPicture* picture) { 
  return picture->usedForRef && picture->usedLongTermRef; 
  }
//}}}
//{{{
static inline int isShortRef (sPicture* picture) { 
  return picture->usedForRef && !picture->usedLongTermRef; 
  }
//}}}

sPicture* allocPicture (cDecoder264* decoder, ePicStructure type, int sizeX, int sizeY, int sizeXcr, int sizeYcr, int isOutput);
void freePicture (sPicture* picture);

void fillFrameNumGap (cDecoder264* decoder, cSlice *slice);
