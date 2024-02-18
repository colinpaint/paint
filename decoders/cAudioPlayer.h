// cPlayer.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <thread>

class cAudioRender;
//}}}

class cAudioPlayer {
public:
  cAudioPlayer (cAudioRender& audioRender, uint32_t sampleRate, uint16_t pid, bool hasAudio);
  virtual ~cAudioPlayer();

  bool isPlaying() const { return mPlaying; }
  int64_t getPts() const { return mPts; }

  void setMute (bool mute) { mMute = mute; }
  void toggleMute() { mMute = !mMute; }

  void startPts (int64_t pts) { mPts = pts; mPlaying = true; }
  void skipPts (int64_t pts) { mPts += pts; mSkip = true; }
  void togglePlay() { mPlaying = !mPlaying; }

private:
  cAudioRender& mAudioRender;
  const uint32_t mSampleRate;

  std::thread mPlayerThread;
  bool mRunning = true;
  bool mExit = false;

  bool mMute = true;
  bool mPlaying = false;
  bool mSkip = false;
  bool mSynced = false;

  int64_t mPts = 0;

  std::array <float, 2048*2> mSamples;
  inline static std::array <float, 2048*2> mSilenceSamples = { 0.f };
  };
