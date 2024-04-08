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

  sPicture* getLongTermPic (cSlice* slice, int longtermPicNum);
  sPicture* getLastPicRefFromDpb();

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
  int outputDpbFrame();
  void checkNumDpbFrames();
  void dumpDpb();

  void adaptiveMemoryManagement (sPicture* picture);
  void slidingWindowMemoryManagement (sPicture* picture);
  void idrMemoryManagement (sPicture* picture);

  void updateMaxLongTermFrameIndex (int maxLongTermFrameIndexPlus1);
  void unmarkLongTermFrameForRefByFrameIndex (int longTermFrameIndex);
  void unmarkLongTermForRef (sPicture* p, int longTermPicNum);
  void unmarkShortTermForRef (sPicture* p, int diffPicNumMinus1);
  void unmarkAllLongTermForRef();
  void unmarkLongTermFieldRefFrameIndex (ePicStructure picStructure, int longTermFrameIndex,
                                         int mark_current, uint32_t curr_frame_num, int curr_pic_num);
  void unmarkAllShortTermForRef();
  void markPicLongTerm (sPicture* p, int longTermFrameIndex, int picNumX);
  void markCurPicLongTerm (sPicture* p, int longTermFrameIndex);
  void assignLongTermFrameIndex (sPicture* p, int diffPicNumMinus1, int longTermFrameIndex);
  };

void initListsSliceI (cSlice* slice);
void initListsSliceP (cSlice* slice);
void initListsSliceB (cSlice* slice);
void updatePicNum (cSlice* slice);

void reorderRefPicList (cSlice* slice, int curList);
sPicture* getShortTermPic (cSlice* slice, cDpb* dpb, int picNum);

void computeColocated (cSlice* slice, sPicture** listX[6]);
