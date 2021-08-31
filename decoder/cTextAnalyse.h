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

  uint32_t readAndParse();
  bool analyse (tCallback callback);

private:
  tCallback mCallback;

  struct sLine {
    std::string mText;
    uint32_t mLineNumber;
    uint32_t mFoldLevel;
    size_t mFoldBeginIndent;
    bool mFoldBegin;
    bool mFoldEnd;
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
