// cPlayer.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <thread>

class cAudioRender;
//}}}

class cPlayer {
public:
  cPlayer::cPlayer (cAudioRender& audioRender, uint32_t sampleRate);
  virtual ~cPlayer();

  // player
  bool isPlaying() const { return mPlaying; }
  int64_t getPts() const { return mPts; }

  void setMute (bool mute) { mMute = mute; }
  void toggleMute() { mMute = !mMute; }

  void startPts (int64_t pts);
  void togglePlay() { mPlaying = !mPlaying; }

private:
  cAudioRender& mAudioRender;
  const uint32_t mSampleRate;

  std::thread mPlayerThread;

  bool mMute = true;
  bool mPlaying = false;
  bool mRunning = true;
  bool mExit = false;

  int64_t mPts = 0;
  };
