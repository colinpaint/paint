// cTextAnalyse.h - jpeg analyser - based on tiny jpeg decoder, jhead
#pragma once
//{{{  includes
#include "cFileView.h"
#include <vector>
#include <functional>
//}}}

class cTextAnalyse : public cFileView {
private:
  using tCallback = std::function <void (const std::string& info, int lineType, uint32_t foldNum)>;
public:
  cTextAnalyse (const std::string& filename) : cFileView(filename) {}
  virtual ~cTextAnalyse() = default;

  tCallback mCallback;
  bool analyse (tCallback callback);
  uint32_t indexFolds();

private:
  struct sFold {
    uint32_t mStartLineNumber;
    uint32_t mLastLineNumber;
    uint32_t mLevel;
    bool mOpen;

    sFold (uint32_t startLineNumber, uint32_t level, bool open)
      : mStartLineNumber(startLineNumber), mLastLineNumber(startLineNumber), mLevel(level), mOpen(open) {}
    };

  struct sLine {
    std::string mText;
    uint32_t mLineNumber;

    sLine (const std::string& text, uint32_t lineNumber)
      : mText(text), mLineNumber(lineNumber) {}
    };

  std::vector<sFold> mFolds;
  std::vector<sLine> mLines;
  };
