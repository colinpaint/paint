#pragma once
struct sPicture;
class cDecoder;

class cFrameStore {
public:
  ~cFrameStore();

  int isReference();
  int isShortTermReference();
  int isLongTermReference();

  void unmarkForRef();
  void unmarkForLongTermRef();

  void dpbCombineField (cDecoder264* decoder);
  void dpbCombineField1 (cDecoder264* decoder);
  void dpbSplitField (cDecoder264* decoder);
  void insertPictureDpb (cDecoder264* decoder, sPicture* picture);

  // vars
  int       isUsed = 0;            // 0=empty; 1=top; 2=bottom; 3=both fields (or frame)
  int       usedReference = 0;     // 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       usedLongTerm = 0;      // 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       usedOrigReference = 0; // original marking by nalRefIdc: 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used

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