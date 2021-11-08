// cDecoder.h - decoder base class
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <functional>

struct AVCodecParserContext;
struct AVCodec;
struct AVCodecContext;

class cFrame;
//}}}

class cDecoder {
public:
  cDecoder();
  virtual ~cDecoder();

  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, uint8_t streamType,
                          std::function<void (cFrame* frame)> addFrameCallback) = 0;

protected:
  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;
  };
