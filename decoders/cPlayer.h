// cPlayer.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <thread>

class cAudioRender;
//}}}

class cPlayer {
public:
  cPlayer (cAudioRender& audioRender, uint32_t sampleRate, uint16_t pid, bool hasAudio);
  virtual ~cPlayer();

  bool isPlaying() const { return mPlaying; }
  int64_t getPts() const { return mPts; }

  void setMute (bool mute) { mMute = mute; }
  void toggleMute() { mMute = !mMute; }

  //{{{
  void startPlayPts (int64_t pts) {
    mPts = pts;
    mPlaying = true;
    }
  //}}}
  //{{{
  void skipPlayPts (int64_t pts) {
    mPts = pts;
    mSkip = true;
    }
  //}}}
  void togglePlay() { mPlaying = !mPlaying; }

private:
  cAudioRender& mAudioRender;
  const uint32_t mSampleRate;

  std::thread mPlayerThread;

  bool mMute = true;
  bool mSkip = false;
  bool mPlaying = false;
  bool mRunning = true;
  bool mExit = false;
  bool mSyncedUp = false;

  int64_t mPts = 0;

  std::array <float, 2048*2> mSamples;
  inline static std::array <float, 2048*2> mSilenceSamples = { 0.f };
  };
