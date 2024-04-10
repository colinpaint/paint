#pragma once
class cFrameStore;
class cDecoder;

class cDpb {
public:
  void initDpb (cDecoder264* decoder, int type);
  void reInitDpb (cDecoder264* decoder, int type);

  void flushDpb();
  void freeDpb();

  void storePictureDpb (sPicture* picture);

  bool isInitDone() const { return initDone; }
  uint32_t getSize() const { return usedSize; };
  uint32_t getAllocatedSize() const { return allocatedSize; };

  sPicture* getShortTermPic (cSlice* slice, int picNum);
  sPicture* getLongTermPic (cSlice* slice, int picNum);
  sPicture* getLastPicRefFromDpb();

  void setSize (uint32_t size) { usedSize = size; };

  // vars
  cDecoder264*  decoder = nullptr;

  uint32_t      refFramesInBuffer = 0;
  uint32_t      longTermRefFramesInBuffer = 0;
  int           lastOutPoc = 0;
  int           dpbPoc[100] = {0};

  cFrameStore** frameStoreArray = nullptr;
  cFrameStore** frameStoreRefArray = nullptr;
  cFrameStore** frameStoreLongTermRefArray = nullptr;

  sPicture*     noRefPicture = nullptr;

private:
  int removeUnusedDpb();
  void removeFrameDpb (int pos);

  void updateInfo();
  void dump();

  int outputDpbFrame();
  void checkNumDpbFrames();

  void getSmallestPoc (int& poc, int& pos);

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

  // vars
  bool          initDone = false;

  uint32_t      usedSize = 0;
  uint32_t      allocatedSize =0 ;
  int           numRefFrames = 0;
  int           maxLongTermPicIndex = 0;

  cFrameStore* lastPictureFrameStore = nullptr;
  };
