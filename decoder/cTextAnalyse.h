// cTextAnalyse.h - jpeg analyser - based on tiny jpeg decoder, jhead
#pragma once
//{{{  includes
#include "cFileView.h"
#include <vector>
#include <functional>
//}}}

class cTextAnalyse : public cFileView {
private:
  using tCallback = std::function <void (const std::string& info, int lineType)>;
public:
  cTextAnalyse (const std::string& filename) : cFileView(filename) {}
  virtual ~cTextAnalyse() = default;

  bool analyse (tCallback callback);
  uint32_t index();

private:
  tCallback mCallback;

  struct sLine {
    std::string mText;
    std::string mFoldComment;
    uint32_t mLineNumber;
    uint32_t mFoldLevel;
    bool mFoldBegin;
    bool mFoldEnd;
    size_t mFoldBeginPos;
    bool mFoldOpen;

    sLine() {}
    };

  struct sFold {
    uint32_t mBeginLineNumber;
    uint32_t mEndLineNumber;

    sFold (uint32_t beginLineNumber) : mBeginLineNumber(beginLineNumber), mEndLineNumber(beginLineNumber) {}
    };

  std::vector<sLine> mLines;
  std::vector<sFold> mFolds;

  inline static const std::string kFoldBeginMarker = "//{{{";
  inline static const std::string kFoldEndMarker = "//}}}";
  inline static const std::string kCommentMarker = "//";
  };
