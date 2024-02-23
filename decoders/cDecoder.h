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
  virtual ~cDecoder() = default;

  virtual std::string getInfoString() const = 0;

  virtual void flush() {}

  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, char frameType,
                          std::function<cFrame* (int64_t pts)> allocFrameCallback,
                          std::function<void (cFrame* frame)> addFrameCallback) = 0;
  };
