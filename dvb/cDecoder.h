// cDecoder.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <functional>

class cFrame;
//}}}

class cDecoder {
public:
  cDecoder() = default;
  virtual ~cDecoder() = default;

  virtual std::string getInfoString() const = 0;

  virtual int64_t decode (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          std::function<cFrame*()> allocFrameCallback,
                          std::function<void (cFrame* frame)> addFrameCallback) = 0;
  };
