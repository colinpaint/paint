//{{{  cDvbTransportStream.h
// PAT inserts <pid,sid> into mProgramMap
// PMT declares pgm and elementary stream pids, adds cService into mServiceMap
// SDT names a service in mServiceMap
//}}}
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>

#include "cDvbMultiplex.h"

class cDvbSource;
class cVideoRender;
class cAudioRender;
class cSubtitleRender;
class cTexture;
//}}}
using tTimePoint = std::chrono::system_clock::time_point;
using tDurationSeconds = std::chrono::seconds;

class cDvbTransportStream {
public:
  cDvbTransportStream (const cDvbMultiplex& dvbMultiplex, const std::string& recordRootName);
  virtual ~cDvbTransportStream();

  //{{{
  class cPidInfo {
  public:
    cPidInfo (uint16_t pid, bool isPsi) : mPid(pid), mPsi(isPsi) {}
    ~cPidInfo() { free (mBuffer); }

    std::string getTypeName();
    std::string getInfoString() { return mInfoString; }

    int getBufUsed() { return int(mBufPtr - mBuffer); }

    int addToBuffer (uint8_t* buf, int bufSize);

    //{{{
    void clearCounts() {
      mPackets = 0;
      mErrors = 0;
      mRepeatContinuity = 0;
      }
    //}}}
    //{{{
    void clearContinuity() {
      mBufPtr = nullptr;
      mStreamPos = -1;
      mContinuity = -1;
      }
    //}}}

    // vars
    const uint16_t mPid;
    const bool mPsi;

    uint16_t mSid = 0;
    uint16_t mStreamType = 0;

    int mPackets = 0;
    int mContinuity = -1;
    int mErrors = 0;
    int mRepeatContinuity = 0;

    int64_t mPts = -1;
    int64_t mFirstPts = -1;
    int64_t mLastPts = -1;
    int64_t mDts = -1;

    int mBufSize = 0;
    uint8_t* mBuffer = nullptr;
    uint8_t* mBufPtr = nullptr;

    int64_t mStreamPos = -1;

    std::string mInfoString;
    };
  //}}}
  //{{{
  class cEpgItem {
  public:
    //{{{
    cEpgItem (bool now, bool record, tTimePoint time, tDurationSeconds duration,
              const std::string& titleString, const std::string& infoString)
      : mNow(now), mRecord(record),
        mTime(time), mDuration(duration),
        mTitleString(titleString), mInfoString(infoString) {}
    //}}}
    ~cEpgItem() {}

    bool getRecord() { return mRecord; }
    std::string getTitleString() { return mTitleString; }
    std::string getDesriptionString() { return mInfoString; }

    tDurationSeconds getDuration() { return mDuration; }
    tTimePoint getTime() { return mTime; }

    //{{{
    bool toggleRecord() {
      mRecord = !mRecord;
      return mRecord;
      }
    //}}}
    //{{{
    void set (tTimePoint time, tDurationSeconds duration,
              const std::string& titleString, const std::string& infoString) {

      mTime = time;
      mDuration = duration;
      mTitleString = titleString;
      mInfoString = infoString;
      }
    //}}}

  private:
    const bool mNow = false;
    bool mRecord = false;

    tTimePoint mTime;
    tDurationSeconds mDuration;

    std::string mTitleString;
    std::string mInfoString;
    };
  //}}}
  //{{{
  class cService {
  public:
    cService (uint16_t sid);
    ~cService();

    //{{{  gets
    uint16_t getSid() const { return mSid; }
    uint16_t getProgramPid() const { return mProgramPid; }

    uint16_t getVidPid() const { return mVidPid; }
    uint16_t getVidStreamType() const { return mVidStreamType; }
    std::string getVidStreamTypeName() const { return mVidStreamTypeName; }

    uint16_t getAudPid() const { return mAudPid; }
    uint16_t getAudStreamType() const { return mAudStreamType; }
    std::string getAudStreamTypeName() const { return mAudStreamTypeName; }
    uint16_t getAudOtherPid() const { return mAudOtherPid; }
    uint16_t getAudOtherStreamType() const { return mAudOtherStreamType; }
    std::string getAudOtherStreamTypeName() const { return mAudOtherStreamTypeName; }

    uint16_t getSubPid() const { return mSubPid; }
    uint16_t getSubStreamType() const { return mSubStreamType; }
    std::string getSubStreamTypeName() const { return mSubStreamTypeName; }

    std::string getChannelName() const { return mChannelName; }
    bool getChannelRecord() const { return mChannelRecord; }
    std::string getChannelRecordName() const { return mChannelRecordName; }

    // renders
    cVideoRender* getVideo() const { return mVideo; }
    cAudioRender* getAudio() const { return mAudio; }
    cAudioRender* getAudioOther() const { return mAudioOther; }
    cSubtitleRender* getSubtitle() const { return mSubtitle; }

    // epg
    bool isEpgRecord (const std::string& title, std::chrono::system_clock::time_point startTime);

    bool getShowEpg() { return mShowEpg; }

    cEpgItem* getNowEpgItem() { return mNowEpgItem; }
    std::string getNowTitleString() { return mNowEpgItem ? mNowEpgItem->getTitleString() : ""; }
    std::map <std::chrono::system_clock::time_point, cEpgItem*>& getEpgItemMap() { return mEpgItemMap; }
    //}}}
    //{{{  sets
    void setProgramPid (uint16_t pid) { mProgramPid = pid; }

    void setVidPid (uint16_t pid, uint16_t streamType);
    void setAudPid (uint16_t pid, uint16_t streamType);
    void setSubPid (uint16_t pid, uint16_t streamType);

    //{{{
    void setChannelName (const std::string& name, bool record, const std::string& recordName) {
      mChannelName = name;
      mChannelRecord = record;
      mChannelRecordName = recordName;
      }
    //}}}

    // render
    void setVideo (cVideoRender* video) { mVideo = video; }
    void toggleVideo();
    void setAudio (cAudioRender* audio) { mAudio = audio; }
    void toggleAudio();
    void setAudioOther (cAudioRender* audio) { mAudioOther = audio; }
    void toggleAudioOther();
    void setSubtitle (cSubtitleRender* subtitle) { mSubtitle = subtitle; }
    void toggleSubtitle();

    // epg
    bool setNow (bool record,
                 std::chrono::system_clock::time_point time, std::chrono::seconds duration,
                 const std::string& str1, const std::string& str2);

    bool setEpg (bool record,
                 std::chrono::system_clock::time_point startTime, std::chrono::seconds duration,
                 const std::string& titleString, const std::string& infoString);

    void toggleShowEpg() { mShowEpg = !mShowEpg; }
    //}}}

    // record
    bool openFile (const std::string& fileName, uint16_t tsid);
    void writePacket (uint8_t* ts, uint16_t pid);
    void closeFile();

  private:
    uint8_t* tsHeader (uint8_t* ts, uint16_t pid, uint8_t continuityCount);

    void writePat (uint16_t tsid);
    void writePmt();
    void writeSection (uint8_t* ts, uint8_t* tsSectionStart, uint8_t* tsPtr);

    // var
    const uint16_t mSid;
    uint16_t mProgramPid = 0;

    uint16_t mVidPid = 0;
    uint16_t mVidStreamType = 0;
    std::string mVidStreamTypeName;

    uint16_t mAudPid = 0;
    uint16_t mAudStreamType = 0;
    std::string mAudStreamTypeName;

    uint16_t mAudOtherPid = 0;
    uint16_t mAudOtherStreamType = 0;
    std::string mAudOtherStreamTypeName;

    uint16_t mSubPid = 0;
    uint16_t mSubStreamType = 0;
    std::string mSubStreamTypeName;

    std::string mChannelName;
    bool mChannelRecord = false;
    std::string mChannelRecordName;

    cVideoRender* mVideo = nullptr;
    cAudioRender* mAudio = nullptr;
    cAudioRender* mAudioOther = nullptr;
    cSubtitleRender* mSubtitle = nullptr;

    // epg
    cEpgItem* mNowEpgItem = nullptr;
    std::map <std::chrono::system_clock::time_point, cEpgItem*> mEpgItemMap;
    bool mShowEpg = true;

    // record
    FILE* mFile = nullptr;
    };
  //}}}

  //{{{  gets
  uint64_t getNumPackets() const { return mNumPackets; }
  uint64_t getNumErrors() const { return mNumErrors; }

  // tdtTime
  bool hasTdtTime() const { return mFirstTimeDefined; }
  tTimePoint getTdtTime() const { return mTdtTime; }
  std::string getTdtTimeString() const;

  // maps
  std::map <uint16_t, cPidInfo>& getPidInfoMap() { return mPidInfoMap; };
  std::map <uint16_t, cService>& getServiceMap() { return mServiceMap; };

  cService* getService (uint16_t sid);
  std::vector <std::string>& getRecordPrograms() { return mRecordPrograms; }

  cVideoRender* getVideo (uint16_t sid);
  cAudioRender* getAudio (uint16_t sid);
  cAudioRender* getAudioOther (uint16_t sid);
  cSubtitleRender* getSubtitle (uint16_t sid);

  // dvbSource
  bool hasDvbSource() const { return mDvbSource; }
  std::string getErrorString() { return mErrorString; }
  std::string getSignalString() { return mSignalString; }
  //}}}

  // source
  void dvbSource (bool launchThread);
  void fileSource (bool launchThread, const std::string& fileName);

  // vars
  std::mutex mMutex;

private:
  //{{{  clears
  void clear();
  void clearPidCounts();
  void clearPidContinuity();
  //}}}

  int64_t getPts (uint8_t* tsPtr);
  cPidInfo* getPidInfo (uint16_t pid, bool createPsiOnly);

  void startServiceProgram (cService* service, tTimePoint tdtTime,
                            const std::string& programName, tTimePoint programStartTime, bool selected);
  void programPesPacket (uint16_t sid, uint16_t pid, uint8_t* ts);
  void stopServiceProgram (cService* service);

  bool audPes (cPidInfo* pidInfo, bool skip);
  bool audOtherPes (cPidInfo* pidInfo, bool skip);
  bool vidPes (cPidInfo* pidInfo, bool skip);
  bool subPes (cPidInfo* pidInfo);

  //{{{  parse
  void parsePat (cPidInfo* pidInfo, uint8_t* buf);
  void parseNit (cPidInfo* pidInfo, uint8_t* buf);
  void parseSdt (cPidInfo* pidInfo, uint8_t* buf);
  void parseEit (cPidInfo* pidInfo, uint8_t* buf);
  void parseTdt (cPidInfo* pidInfo, uint8_t* buf);
  void parsePmt (cPidInfo* pidInfo, uint8_t* buf);
  int parsePsi (cPidInfo* pidInfo, uint8_t* buf);
  //}}}
  int64_t demux (uint8_t* tsBuf, int64_t tsBufSize, int64_t streamPos, bool skip);
  void dvbSourceInternal (bool launchThread);
  void fileSourceInternal (bool launchThread, const std::string& fileName);

  // vars
  cDvbMultiplex mDvbMultiplex;

  uint64_t mNumPackets = 0;
  uint64_t mNumErrors = 0;

  std::map <uint16_t, uint16_t> mProgramMap;
  std::map <uint16_t, cPidInfo> mPidInfoMap;
  std::map <uint16_t, cService> mServiceMap;

  // dvbSource
  cDvbSource* mDvbSource = nullptr;
  std::string mErrorString;
  std::string mSignalString;
  uint64_t mLastErrors = 0;

  // record
  std::mutex mRecordFileMutex;
  std::string mRecordRootName;
  std::vector <std::string> mRecordPrograms;

  // time
  bool mFirstTimeDefined = false;
  tTimePoint mFirstTime; // first tdtTime seen
  tTimePoint mTdtTime;   // now tdtTime
  };
