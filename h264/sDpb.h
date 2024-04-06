#pragma once
class cFrameStore;
class cDecoder;

struct sDpb {
  cDecoder264*  decoder;

  cFrameStore** frameStore;
  cFrameStore** frameStoreRef;
  cFrameStore** frameStoreLongTermRef;

  uint32_t      size;
  uint32_t      usedSize;
  uint32_t      refFramesInBuffer;
  uint32_t      longTermRefFramesInBuffer;

  int           lastOutputPoc;
  int           maxLongTermPicIndex;
  int           initDone;
  int           numRefFrames;

  cFrameStore*  lastPictureFrameStore;
  };

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
