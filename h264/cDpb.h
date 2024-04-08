#pragma once
class cFrameStore;
class cDecoder;

class cDpb {
public:
  void updateRefList();
  void updateLongTermRefList();

  void initDpb (cDecoder264* decoder, int type);
  void reInitDpb (cDecoder264* decoder, int type);
  void flushDpb();
  int removeUnusedDpb();
  void storePictureDpb (sPicture* picture);
  void removeFrameDpb (int pos);
  void freeDpb();

  sPicture* getLastPicRefFromDpb();
  sPicture* getShortTermPic (cSlice* slice, int picNum);
  sPicture* getLongTermPic (cSlice* slice, int picNum);

  //  vars
  cDecoder264*  decoder;

  int           numRefFrames;
  cFrameStore** frameStore;
  cFrameStore** frameStoreRef;
  cFrameStore** frameStoreLongTermRef;
  cFrameStore*  lastPictureFrameStore;

  uint32_t      size;
  uint32_t      usedSize;
  uint32_t      refFramesInBuffer;
  uint32_t      longTermRefFramesInBuffer;
  int           maxLongTermPicIndex;

  int           lastOutputPoc;
  int           initDone;

private:
  void dumpDpb();

  int outputDpbFrame();
  void checkNumDpbFrames();

  void adaptiveMemoryManagement (sPicture* picture);
  void slidingWindowMemoryManagement (sPicture* picture);
  void idrMemoryManagement (sPicture* picture);

  void getSmallestPoc (int* poc, int* pos);
  void updateMaxLongTermFrameIndex (int maxLongTermFrameIndexPlus1);

  void unmarkLongTermFrameForRefByFrameIndex (int longTermFrameIndex);
  void unmarkLongTermForRef (sPicture* picture, int picNum);
  void unmarkShortTermForRef (sPicture* picture, int diffPicNumMinus1);
  void unmarkAllLongTermForRef();
  void unmarkLongTermFieldRefFrameIndex (ePicStructure picStructure, int longTermFrameIndex,
                                         int markCur, uint32_t curFrameNum, int curPicNum);
  void unmarkAllShortTermForRef();

  void markPicLongTerm (sPicture* picture, int longTermFrameIndex, int picNumX);
  void markCurPicLongTerm (sPicture* picture, int longTermFrameIndex);

  void assignLongTermFrameIndex (sPicture* picture, int diffPicNumMinus1, int longTermFrameIndex);
  };
