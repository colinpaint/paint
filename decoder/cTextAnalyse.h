// cTextAnalyse.h - jpeg analyser - based on tiny jpeg decoder, jhead
#pragma once
//{{{  includes
#include "cFileAnalyse.h"
#include <functional>
//}}}

class cTextAnalyse : public cFileAnalyse {
private:
  using tCallback = std::function <void (int lineType, const std::string& info)>;
public:
  cTextAnalyse (const std::string& filename) : cFileAnalyse(filename) {}
  virtual ~cTextAnalyse() = default;

  tCallback mCallback;
  bool analyse (tCallback callback);
  };
