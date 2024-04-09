#pragma once
class cFrameStore;
class cDecoder;

class cDpb {
public:
  void initDpb (cDecoder264* decoder, int type);
  void reInitDpb (cDecoder264* decoder, int type);

  void flushDpb();
  void freeDpb();
  int removeUnusedDpb();
  void removeFrameDpb (int pos);
  void storePictureDpb (sPicture* picture);

  sPicture* getLastPicRefFromDpb();
  sPicture* getShortTermPic (cSlice* slice, int picNum);
  sPicture* getLongTermPic (cSlice* slice, int picNum);

  //  vars
  cDecoder264*  decoder;
  bool          initDone;

  int           numRefFrames;
  uint32_t      size;
  uint32_t      usedSize;
  uint32_t      refFramesInBuffer;
  uint32_t      longTermRefFramesInBuffer;
  int           maxLongTermPicIndex;
  int           lastOutPoc;

  cFrameStore** frameStore;
  cFrameStore** frameStoreRef;
  cFrameStore** frameStoreLongTermRef;
  cFrameStore* lastPictureFrameStore;

private:
  void dump();
  int outputDpbFrame();
  void checkNumDpbFrames();

  void getSmallestPoc (int* poc, int* pos);

  void idrMemoryManagement (sPicture* picture);
  void adaptiveMemoryManagement (sPicture* picture);
  void slidingWindowMemoryManagement (sPicture* picture);

  void updateRefList();
  void updateLongTermRefList();
  void updateMaxLongTermFrameIndex (int maxLongTermFrameIndexPlus1);

  void unmarkAllShortTermForRef();
  void unmarkShortTermForRef (sPicture* picture, int diffPicNumMinus1);

  void unmarkAllLongTermForRef();
  void unmarkLongTermForRef (sPicture* picture, int picNum);
  void unmarkLongTermFrameForRefByFrameIndex (int longTermFrameIndex);
  void unmarkLongTermFieldRefFrameIndex (ePicStructure picStructure, int longTermFrameIndex,
                                        int markCur, uint32_t curFrameNum, int curPicNum);

  void markPicLongTerm (sPicture* picture, int longTermFrameIndex, int picNumX);
  void markCurPicLongTerm (sPicture* picture, int longTermFrameIndex);

  void assignLongTermFrameIndex (sPicture* picture, int diffPicNumMinus1, int longTermFrameIndex);
  };
