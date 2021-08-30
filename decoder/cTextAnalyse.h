// cTextAnalyse.h - jpeg analyser - based on tiny jpeg decoder, jhead
#pragma once
//{{{  includes
#include "cFileAnalyse.h"
#include <vector>
#include <functional>
//}}}

class cTextAnalyse : public cFileAnalyse {
private:
  using tCallback = std::function <void (const std::string& info, int lineType, uint32_t foldNum)>;
public:
  cTextAnalyse (const std::string& filename) : cFileAnalyse(filename) {}
  virtual ~cTextAnalyse() = default;

  uint32_t getNumFolds() { return (uint32_t)mFolds.size(); }
  tCallback mCallback;
  bool analyse (tCallback callback);

private:
  struct sFold {
    uint32_t mStartLineNumber;
    bool mOpen;
    sFold (uint32_t startLineNumber, bool open) : mStartLineNumber(startLineNumber), mOpen(open) {}
    };

  std::vector<sFold> mFolds;
  };
