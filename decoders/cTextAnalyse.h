// cTextAnalyse.h - jpeg analyser - based on tiny jpeg decoder, jhead
#pragma once
//{{{  includes
#include "../common/cFileView.h"
#include <vector>
#include <functional>
//}}}

class cTextAnalyse : public cFileView {
private:
  using tCallback = std::function <void (const std::string& info, uint32_t lineNumber)>;
public:
  cTextAnalyse (const std::string& filename) : cFileView(filename) {}
  virtual ~cTextAnalyse() = default;

  void readAndParse();
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

  std::vector<sLine> mLines;

  inline static const std::string kFoldBeginMarker = "//{{{";
  inline static const std::string kFoldEndMarker = "//}}}";
  inline static const std::string kCommentMarker = "//";
  };
