// cSong.h - audioFrame container - power,peak,fft,range select, hls numbering
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <mutex>
#include <shared_mutex>

#include "../decoders/cAudioParser.h"

#include "../audio/kiss_fft.h"
#include "../audio/kiss_fftr.h"
//}}}

class cSong {
public:
  //{{{
  class cFrame {
  public:
    cFrame (int numChannels, int numFreqBytes, float* samples, int64_t pts);
    virtual ~cFrame();

    // gets
    float* getSamples() const { return mSamples; }
    int64_t getPts() const { return mPts; }

    float* getPowerValues() const { return mPowerValues;  }
    float* getPeakValues() const { return mPeakValues;  }
    uint8_t* getFreqValues() const { return mFreqValues; }
    uint8_t* getFreqLuma() const { return mFreqLuma; }

    bool isQuiet() const { return mPeakValues[0] + mPeakValues[1] < kQuietThreshold; }

    bool isMuted() const { return mMuted; }
    bool isSilence() const { return mSilence; }
    void setSilence (bool silence) { mSilence = silence; }

    bool hasTitle() const { return !mTitle.empty(); }
    std::string getTitle() const { return mTitle; }

    // vars
    float* mSamples;
    int64_t mPts;

    float* mPowerValues;
    float* mPeakValues;
    uint8_t* mFreqValues;
    uint8_t* mFreqLuma;

  private:
    static constexpr float kQuietThreshold = 0.01f;

    // vars
    bool mMuted;
    bool mSilence;

    std::string mTitle;
    };
  //}}}
  //{{{
  class cSelect {
  public:
    //{{{
    class cSelectItem {
    public:
      enum eType { eLoop, eMute };

      cSelectItem(eType type, int64_t firstFrameNum, int64_t lastFrameNum, const std::string& title) :
        mType(type), mFirstFrameNum(firstFrameNum), mLastFrameNum(lastFrameNum), mTitle(title) {}

      eType getType() { return mType; }
      int64_t getFirstFrameNum() { return mFirstFrameNum; }
      int64_t getLastFrameNum() { return mLastFrameNum; }
      bool getMark() { return getFirstFrameNum() == getLastFrameNum(); }
      std::string getTitle() { return mTitle; }
      //{{{
      bool inRange (int64_t FrameNum) {
      // ignore 1 FrameNum select range
        return (mFirstFrameNum != mLastFrameNum) && (FrameNum >= mFirstFrameNum) && (FrameNum <= mLastFrameNum);
        }
      //}}}

      void setType (eType type) { mType = type; }
      void setFirstFrameNum (int64_t frameNum) { mFirstFrameNum = frameNum; }
      void setLastFrameNum (int64_t frameNum) { mLastFrameNum = frameNum; }
      void setTitle (const std::string& title) { mTitle = title; }

    private:
      eType mType;
      int64_t mFirstFrameNum = 0;
      int64_t mLastFrameNum = 0;
      std::string mTitle;
      };
    //}}}

    // gets
    bool empty() { return mItems.empty(); }
    int64_t getFirstFrameNum() { return empty() ? 0 : mItems.front().getFirstFrameNum(); }
    int64_t getLastFrameNum() { return empty() ? 0 : mItems.back().getLastFrameNum(); }
    std::vector<cSelectItem>& getItems() { return mItems; }

    bool inRange (int64_t frameNum);
    int64_t constrainToRange (int64_t frameNum, int64_t constrainedFrameNum);

    // actions
    void clearAll();
    void addMark (int64_t frameNum, const std::string& title = "");
    void start (int64_t frameNum);
    void move (int64_t frameNum);
    void end();

  private:
    enum eEdit { eEditNone, eEditFirst, eEditLast, eEditRange };

    eEdit mEdit = eEditNone;
    int64_t mEditFrameNum = 0;
    std::vector<cSelectItem> mItems;
    int mItemNum = 0;
    };
  //}}}
  cSong (eAudioFrameType frameType, int numChannels, int sampleRate, int samplesPerFrame, int maxMapSize);
  virtual ~cSong();

  //{{{  get
  std::shared_mutex& getSharedMutex() { return mSharedMutex; }

  eAudioFrameType getFrameType() { return mFrameType; }
  uint32_t getNumChannels() const { return mNumChannels; }
  uint32_t getNumSampleBytes() const { return mNumChannels * sizeof(float); }
  uint32_t getSampleRate() const { return mSampleRate; }
  uint32_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  int64_t getFramesFromSeconds (int64_t seconds) const { return (seconds * mSampleRate) / mSamplesPerFrame; }
  int64_t getSecondsFromFrames (int64_t frames) const { return (frames * mSamplesPerFrame) / mSampleRate; }

  virtual int64_t getFramePtsDuration() const { return 1; }
  virtual int64_t getFrameNumFromPts (int64_t pts) const { return pts; }
  virtual int64_t getPtsFromFrameNum (int64_t frameNum) const { return frameNum; }
  //}}}
  cSelect& getSelect() { return mSelect; }

  int64_t getPlayPts() const { return mPlayPts; }
  bool getPlaying() const { return mPlaying; }

  // get frameNum
  int64_t getPlayFrameNum() const { return getFrameNumFromPts (mPlayPts); }
  int64_t getFirstFrameNum() const { return mFrameMap.empty() ? 0 : mFrameMap.begin()->first; }
  int64_t getLastFrameNum() const { return mFrameMap.empty() ? 0 : mFrameMap.rbegin()->first;  }
  //{{{
  int64_t getNumFrames() const {
    return mFrameMap.empty() ? 0 : (mFrameMap.rbegin()->first - mFrameMap.begin()->first+1);
    }
  //}}}
  int64_t getTotalFrames() const { return mTotalFrames; }

  virtual bool getPlayFinished() const;
  virtual std::string getFirstTimeString (int daylightSeconds) const;
  virtual std::string getPlayTimeString (int daylightSeconds) const;
  virtual std::string getLastTimeString (int daylightSeconds) const;

  //{{{  get max nums for early allocations
  int getMaxNumSamplesPerFrame() const { return kMaxNumSamplesPerFrame; }
  int getMaxNumSampleBytes() const { return kMaxNumChannels * sizeof(float); }
  int getMaxNumFrameSamplesBytes()const { return getMaxNumSamplesPerFrame() * getMaxNumSampleBytes(); }
  //}}}
  //{{{  get max values for ui
  float getMaxPowerValue()  const{ return mMaxPowerValue; }
  float getMaxPeakValue() const { return mMaxPeakValue; }
  float getMaxFreqValue() const { return mMaxFreqValue; }
  uint32_t getNumFreqBytes() const { return kMaxFreqBytes; }
  //}}}

  //{{{
  cFrame* findFrameByFrameNum (int64_t frameNum) const {
    auto it = mFrameMap.find (frameNum);
    return (it == mFrameMap.end()) ? nullptr : it->second;
    }
  //}}}
  virtual cFrame* findFrameByPts (int64_t pts) const { return findFrameByFrameNum (pts); }
  virtual cFrame* findPlayFrame() const { return findFrameByFrameNum (mPlayPts); }

  //{{{  play
  void togglePlaying() { mPlaying = !mPlaying; }

  virtual void setPlayPts (int64_t pts);
  virtual void setPlayFirstFrame();
  virtual void setPlayLastFrame();

  void nextPlayFrame (bool useSelectRange);
  void incPlaySec (int seconds, bool useSelectRange);
  void prevSilencePlayFrame();
  void nextSilencePlayFrame();
  //}}}

  void addFrame (bool reuseFront, int64_t pts, float* samples, int64_t totalFrames);

protected:
  //{{{  vars
  std::shared_mutex mSharedMutex;

  const int mSampleRate = 0;
  const int mSamplesPerFrame = 0;

  int64_t mPlayPts = 0;
  cSelect mSelect;

  std::map <int64_t, cFrame*> mFrameMap;
  //}}}

private:
  //{{{  static const
  inline static const uint32_t kMaxNumChannels = 2;                         // arbitrary chan max
  inline static const uint32_t kMaxNumSamplesPerFrame = 2048;               // arbitrary frame max
  inline static const uint32_t kMaxFreq = (kMaxNumSamplesPerFrame / 2) + 1; // fft max
  inline static const uint32_t kMaxFreqBytes = 512;                         // arbitrary graphics max
  //}}}
  int64_t skipPrev (int64_t fromPts, bool silence);
  int64_t skipNext (int64_t fromPts, bool silence);
  void checkSilenceWindow (int64_t pts);
  //{{{  vars
  const eAudioFrameType mFrameType;
  const int mNumChannels;

  // map of frames keyed by frame pts/ptsDuration
  // - frameNum offset by firstFrame pts/ptsDuration
  int mMaxMapSize = 0;
  int64_t mTotalFrames = 0;
  bool mPlaying = false;

  // fft vars
  kiss_fftr_cfg mFftrConfig;
  kiss_fft_scalar mTimeBuf[kMaxNumSamplesPerFrame];
  kiss_fft_cpx mFreqBuf[kMaxFreq];

  // max stuff for ui
  float mMaxPowerValue = 0.f;
  float mMaxPeakValue = 0.f;
  float mMaxFreqValue = 0.f;
  //}}}
  };

// cPtsSong
//{{{
class cPtsSong : public cSong {
public:
  //{{{
  cPtsSong (eAudioFrameType frameType, int numChannels, int sampleRate,
            int samplesPerFrame, int64_t framePtsDuration, int maxMapSize)
    : cSong (frameType, numChannels, sampleRate, samplesPerFrame, maxMapSize), mFramePtsDuration(framePtsDuration)  {}
  //}}}
  virtual ~cPtsSong() = default;

  virtual int64_t getFramePtsDuration() const final { return mFramePtsDuration; }
  virtual int64_t getFrameNumFromPts (int64_t pts) const final { return pts / mFramePtsDuration; }
  virtual int64_t getPtsFromFrameNum (int64_t frameNum) const final { return frameNum * mFramePtsDuration; }

  virtual int64_t getFirstPts() const final { return mFrameMap.empty() ? 0 : mFrameMap.begin()->second->getPts(); }
  virtual int64_t getLastPts() const final { return mFrameMap.empty() ? 0 : mFrameMap.rbegin()->second->getPts();  }

  virtual bool getPlayFinished()const final;
  virtual std::string getFirstTimeString (int daylightSeconds) const final;
  virtual std::string getPlayTimeString (int daylightSeconds) const final;
  virtual std::string getLastTimeString (int daylightSeconds) const final;

  virtual cFrame* findFrameByPts (int64_t pts) const final { return findFrameByFrameNum (getFrameNumFromPts(pts)); }
  virtual cFrame* findPlayFrame() const final { return findFrameByPts (mPlayPts); }

  void setBasePts (int64_t pts);

  virtual void setPlayPts (int64_t pts) final;
  virtual void setPlayFirstFrame() final;
  virtual void setPlayLastFrame() final;

protected:
  int64_t mFramePtsDuration = 1;
  int64_t mBasePts = 0;
  std::chrono::system_clock::time_point mBaseTimePoint;
  int64_t mBaseSinceMidnightMs = 0;
  };
//}}}

// cHlsSong
//{{{
class cHlsSong : public cPtsSong {
public:
  cHlsSong (eAudioFrameType frameType, int numChannels, int sampleRate,
            int samplesPerFrame, int64_t framePtsDuration, int maxMapSize, int framesPerChunk);
  virtual ~cHlsSong() = default;

  // gets
  int64_t getBasePlayPts() const { return mPlayPts - mBasePts; }
  int getLoadChunkNum (int64_t& loadPts, bool& reuseFromFront) const;
  int64_t getLengthPts() const { return getLastPts(); }

  // sets
  void setBaseHls (int64_t pts,
                   std::chrono::system_clock::time_point timePoint, std::chrono::seconds offset,
                   int chunkNum);

private:
  // vars
  const int mFramesPerChunk = 0;
  int mBaseChunkNum = 0;
  };
//}}}
