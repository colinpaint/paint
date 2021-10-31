// cAudioDecoder.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cDecoder.h"
#include "../utils/cMiniLog.h"

class cAudioFFmpegDecoder;
class cAudioFrames;
class cAudioPlayer;
//}}}

class cAudioDecoder : public cDecoder {
public:
  cAudioDecoder (const std::string name);
  virtual ~cAudioDecoder();

  bool decode (uint8_t* pes, int pesSize, int64_t pts);

private:
  cAudioFFmpegDecoder* mAudioFFmpegDecoder;
  cAudioFrames* mAudioFrames = nullptr;
  cAudioPlayer* mAudioPlayer = nullptr;
  };
