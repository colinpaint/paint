// cAudioRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cRender.h"
#include "../utils/cMiniLog.h"

class cAudioFFmpegDecoder;
class cAudioPool;
class cAudioPlayer;
//}}}

class cAudioRender : public cRender {
public:
  cAudioRender (const std::string name);
  virtual ~cAudioRender();

  int64_t getPlayPts() const;

  void process (uint8_t* pes, uint32_t pesSize, int64_t pts);

private:
  cAudioFFmpegDecoder* mAudioFFmpegDecoder;
  cAudioPool* mAudioPool = nullptr;
  cAudioPlayer* mAudioPlayer = nullptr;
  };
