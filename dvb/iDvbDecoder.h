// iDvbDecoder
//{{{  includes
#pragma once

#include <cstdint>
//}}}

class iDvbDecoder {
public:
  iDvbDecoder() = default;
  virtual ~iDvbDecoder() = default;

  virtual bool decode (const uint8_t* buf, int bufSize) = 0;
  };
