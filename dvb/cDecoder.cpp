// cDecoder.cpp
//{{{  includes
#include "cDecoder.h"

#include <cstdint>
#include <string>

#include "../utils/cLog.h"

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  }

using namespace std;
//}}}

cDecoder::cDecoder (uint8_t streamType) : mStreamType(streamType) {}
//{{{
cDecoder::~cDecoder() {

  if (mAvContext)
    avcodec_close (mAvContext);
  if (mAvParser)
    av_parser_close (mAvParser);
  }
//}}}
