// cAudioRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cRender.h"
#include "../utils/cMiniLog.h"

class cAudioDecoder;
class cAudioPool;
class cAudioPlayer;
//}}}

class cAudioRender : public cRender {
public:
  cAudioRender (const std::string name);
  virtual ~cAudioRender();

  int64_t getPlayPts() const;

  void processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts);

private:
  cAudioDecoder* mAudioDecoder;
  cAudioPool* mAudioPool = nullptr;
  cAudioPlayer* mAudioPlayer = nullptr;
  };
