#pragma once
class cFrameStore;
class cDecoder;

class cDpb {
public:
  void getSmallestPoc (int* poc, int* pos);

  void updateRefList();
  void updateLongTermRefList();

  void initDpb (cDecoder264* decoder, int type);
  void reInitDpb (cDecoder264* decoder, int type);
  void flushDpb();
  int removeUnusedDpb();
  void storePictureDpb (sPicture* picture);
  void removeFrameDpb (int pos);
  void freeDpb();

  // vars
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

private:
  void dumpDpb();
  void adaptiveMemoryManagement (sPicture* picture);
  void slidingWindowMemoryManagement (sPicture* picture);
  void idrMemoryManagement (sPicture* picture);
  };

void initListsSliceI (cSlice* slice);
void initListsSliceP (cSlice* slice);
void initListsSliceB (cSlice* slice);
void updatePicNum (cSlice* slice);

void reorderRefPicList (cSlice* slice, int curList);
sPicture* getShortTermPic (cSlice* slice, cDpb* dpb, int picNum);

void computeColocated (cSlice* slice, sPicture** listX[6]);
