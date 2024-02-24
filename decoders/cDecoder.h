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

  virtual void decode (uint8_t* pes, uint32_t pesSize, int64_t pts,
                       char frameType, const std::string& frameInfo,
                       std::function<cFrame* (int64_t pts)> allocFrameCallback,
                       std::function<void (cFrame* frame)> addFrameCallback) = 0;
  };
