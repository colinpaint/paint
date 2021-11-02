// cAudioRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <shared_mutex>

#include "cRender.h"
#include "../utils/cMiniLog.h"

class cAudioFrame;
class cAudioDecoder;
class cAudioPlayer;
//}}}

enum class eAudioFrameType { eUnknown, eId3Tag, eWav, eMp3, eAacAdts, eAacLatm };

class cAudioRender : public cRender {
public:
  cAudioRender (const std::string name);
  virtual ~cAudioRender();

  // gets
  std::shared_mutex& getSharedMutex() { return mSharedMutex; }

  eAudioFrameType getFrameType() const { return mFrameType; }
  size_t getNumChannels() const { return mNumChannels; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  uint32_t getSampleRate() const { return mSampleRate; }

  bool getPlaying() const { return mPlaying; }
  int64_t getPlayPts() const { return mPlayPts; }
  std::string getInfoString() const;

  // find
  cAudioFrame* findPlayFrame() const { return findFrame (mPlayPts); }

  // play
  void togglePlaying() { mPlaying = !mPlaying; }
  void setPlayPts (int64_t pts) { mPlayPts = pts; }

  void processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts);

private:
  cAudioFrame* findFrame (int64_t pts) const;
  void addFrame (cAudioFrame* frame);

  // vars
  std::shared_mutex mSharedMutex;
  cAudioDecoder* mAudioDecoder = nullptr;

  eAudioFrameType mFrameType;
  size_t mNumChannels;
  size_t mSamplesPerFrame = 0;
  uint32_t mSampleRate = 0;

  size_t mMaxMapSize;
  std::map <int64_t, cAudioFrame*> mFrames;

  bool mPlaying = false;
  int64_t mPlayPts = 0;
  cAudioPlayer* mAudioPlayer = nullptr;
  };
