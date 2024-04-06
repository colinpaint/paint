#pragma once
struct sPicture;

class cFrameStore {
public:
  static cFrameStore* cFrameStore::allocFrameStore();
  void freeFrameStore();

  int isReference();
  int isShortTermReference();
  int isLongTermReference();

  void unmarkForRef();
  void unmarkForLongTermRef();

  // vars
  int       isUsed = 0;          // 0=empty; 1=top; 2=bottom; 3=both fields (or frame)
  int       mIsReference = 0;    // 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       isLongTerm = 0;      // 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       isOrigReference = 0; // original marking by nalRefIdc: 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       isNonExistent = 0;

  uint32_t  frameNum = 0;
  uint32_t  recoveryFrame = 0;

  int       frameNumWrap = 0;
  int       longTermFrameIndex = 0;
  int       isOutput = 0;
  int       poc = 0;

  int       concealRef = 0;

  sPicture* frame = nullptr;
  sPicture* topField = nullptr;
  sPicture* botField = nullptr;
  };
