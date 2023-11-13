// cAudioRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <thread>

#include "cRender.h"
#include "../common/cMiniLog.h"

class cAudioFrame;
class cAudioDecoder;
class cAudioPlayer;
//}}}

enum class eAudioFrameType { eUnknown, eId3Tag, eWav, eMp3, eAacAdts, eAacLatm };

class cAudioRender : public cRender {
public:
  cAudioRender (const std::string& name, uint8_t streamType);
  virtual ~cAudioRender();

  // gets
  size_t getNumChannels() const { return mNumChannels; }
  uint32_t getSampleRate() const { return mSampleRate; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  int64_t getPtsDuration() const { return mPtsDuration; }

  // find
  cAudioFrame* findAudioFrameFromPts (int64_t pts);

  // player
  bool isPlaying() const { return mPlaying; }
  int64_t getPlayerPts() const { return mPlayerPts; }

  void setMute (bool mute) { mMute = mute; }
  void toggleMute() { mMute = !mMute; }
  void togglePlaying() { mPlaying = !mPlaying; }

  // callbacks
  cFrame* getFrame();
  void addFrame (cFrame* frame);

  // overrides
  virtual std::string getInfoString() const final;
  virtual bool processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) final;

private:
  void startPlayerPts (int64_t pts);
  void exitWait();

  // vars
  size_t mNumChannels;
  uint32_t mSampleRate;
  size_t mSamplesPerFrame;
  int64_t mPtsDuration;

  std::string mFrameInfo;

  // player
  std::thread mPlayerThread;
  bool mPlaying = false;
  bool mMute = false;
  bool mRunning = true;
  bool mExit = false;
  int64_t mPlayerPts = 0;
  };
