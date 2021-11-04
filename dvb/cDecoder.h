// cDecoder.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

struct AVCodecParserContext;
struct AVCodec;
struct AVCodecContext;
//}}}

class cDecoder {
public:
  cDecoder();
  virtual ~cDecoder();

protected:
  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;
  };
