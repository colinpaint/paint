// cTextAnalyse.h - jpeg analyser - based on tiny jpeg decoder, jhead
#pragma once
//{{{  includes
#include "cFileAnalyse.h"
#include <functional>
//}}}

class cTextAnalyse : public cFileAnalyse {
private:
  using tCallback = std::function <void (uint8_t level, const std::string& info, uint8_t* ptr,
                                         uint32_t offset, uint32_t numBytes)>;
public:
  cTextAnalyse (const std::string& filename) : cFileAnalyse(filename) {}
  virtual ~cTextAnalyse() = default;

  tCallback mCallback;
  bool analyse (tCallback callback);
  };
