//{{{  cTransportStream.h
// PAT inserts <pid,sid> into mProgramMap
// PMT declares pgm and elementary stream pids, adds cService into mServiceMap
// SDT names a service in mServiceMap
//}}}
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <chrono>
#include <mutex>

class cRender;

#include "../common/basicTypes.h"
#include "../common/iOptions.h"
#include "cDvbMultiplex.h"
//}}}
constexpr int kAacLatmStreamType = 17;
constexpr int kH264StreamType = 27;

class cTransportStream {
public:
  enum eStreamType { eVideo, eAudio, eDescription, eSubtitle, eStreamTypeSize };
  //{{{
  class cEpgItem {
  public:
    //{{{
    cEpgItem (bool now, bool record,
              std::chrono::system_clock::time_point time,
              std::chrono::seconds duration,
              const std::string& titleString, const std::string& infoString)
      : mNow(now), mRecord(record),
        mTime(time), mDuration(duration),
        mTitleString(titleString), mInfoString(infoString) {}
    //}}}
    ~cEpgItem() {}

    std::string getTitleString() { return mTitleString; }
    std::string getDesriptionString() { return mInfoString; }

    std::chrono::seconds getDuration() { return mDuration; }
    std::chrono::system_clock::time_point getTime() { return mTime; }

    bool getRecord() { return mRecord; }
    //{{{
    bool toggleRecord() {
      mRecord = !mRecord;
      return mRecord;
      }
    //}}}

    //{{{
    void set (std::chrono::system_clock::time_point time,
              std::chrono::seconds duration,
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

    std::chrono::system_clock::time_point mTime;
    std::chrono::seconds mDuration;

    std::string mTitleString;
    std::string mInfoString;
    };
  //}}}
  //{{{
  class cPidInfo {
  public:
    cPidInfo (uint16_t pid, bool isPsi) : mPid(pid), mPsi(isPsi) {}
    ~cPidInfo() { free (mBuffer); }

    bool isPsi() const { return mPsi; }
    uint16_t getPid() const { return mPid; }
    uint16_t getSid() const { return mSid; }

    int64_t getPts() const { return mPts; }
    int64_t getFirstPts() const { return mFirstPts; }
    int64_t getLastPts() const { return mLastPts; }
    int64_t getDts() const { return mDts; }

    uint8_t getStreamType() const { return mStreamType; }
    std::string getTypeName() const ;
    std::string getInfoString() const { return mInfoString; }

    int getBufUsed() const { return int(mBufPtr - mBuffer); }

    void setSid (uint16_t sid) { mSid = sid; }
    void setStreamType (uint8_t streamType) { mStreamType = streamType; }
    void setInfoString (const std::string infoString) { mInfoString = infoString; }
    //{{{
    void setPts (int64_t pts) {

      mPts = pts;

      if ((pts != -1) && (mFirstPts != -1))
        mFirstPts = pts;
      if ((pts != -1) && (pts > mLastPts))
        mLastPts = pts;
      }
    //}}}
    void setDts (int64_t dts) { mDts = dts; }

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
    int64_t mPackets = 0;
    int mContinuity = -1;
    int mErrors = 0;
    int mRepeatContinuity = 0;

    int mBufSize = 0;
    uint8_t* mBuffer = nullptr;
    uint8_t* mBufPtr = nullptr;

    int64_t mStreamPos = -1;

  private:
    const uint16_t mPid;
    const bool mPsi;

    uint16_t mSid = 0;
    uint8_t mStreamType = 0;

    std::string mInfoString;

    int64_t mPts = -1;
    int64_t mFirstPts = -1;
    int64_t mLastPts = -1;
    int64_t mDts = -1;
    };
  //}}}
  //{{{
  class cStream {
  public:
    ~cStream();

    bool isDefined() const { return mDefined; }
    bool isEnabled() const { return mRender; }

    std::string getLabel() const { return mLabel; }
    uint16_t getPid() const { return mPid; }
    uint8_t getTypeId() const { return mTypeId; }
    std::string getTypeName() const { return mTypeName; }
    int64_t getPts() const { return mPts; }
    cRender& getRender() const { return *mRender; }

    void setLabel (const std::string& label) { mLabel = label; }
    void setPidStreamType (uint16_t pid, uint8_t streamType);
    void setPts (int64_t pts) { mPts = pts; }
    void setRender (cRender* render) { mRender = render; }

    bool toggle();

  private:
    bool mDefined = false;

    std::string mLabel;
    uint16_t mPid = 0;
    uint8_t mTypeId = 0;
    std::string mTypeName;

    int64_t mPts = -1;
    cRender* mRender = nullptr;
    };
  //}}}
  //{{{
  class cService {
  public:
    cService (uint16_t sid, iOptions* options);
    ~cService();

    // gets
    uint16_t getSid() const { return mSid; }
    uint16_t getProgramPid() const { return mProgramPid; }

    // sets
    void setProgramPid (uint16_t pid) { mProgramPid = pid; }
    //{{{
    void setChannelName (const std::string& name, bool record, const std::string& recordName) {

      mChannelName = name;
      mChannelRecord = record;
      mChannelRecordName = recordName;
      }
    //}}}

    // record
    bool getChannelRecord() const { return mChannelRecord; }
    std::string getChannelName() const { return mChannelName; }
    std::string getChannelRecordName() const { return mChannelRecordName; }
    bool openFile (const std::string& fileName, uint16_t tsid);
    void writePacket (uint8_t* ts, uint16_t pid);
    void closeFile();

    // epg
    bool isEpgRecord (const std::string& title, std::chrono::system_clock::time_point startTime);
    cEpgItem* getNowEpgItem() { return mNowEpgItem; }
    std::string getNowTitleString() const { return mNowEpgItem ? mNowEpgItem->getTitleString() : ""; }
    std::map <std::chrono::system_clock::time_point, cEpgItem*>& getEpgItemMap() { return mEpgItemMap; }
    //{{{
    bool setNow (bool record,
                 std::chrono::system_clock::time_point time, std::chrono::seconds duration,
                 const std::string& str1, const std::string& str2);
    //}}}
    //{{{
    bool setEpg (bool record,
                 std::chrono::system_clock::time_point startTime, std::chrono::seconds duration,
                 const std::string& titleString, const std::string& infoString);
    //}}}

    // streams
    cStream* getStreamByPid (uint16_t pid);
    cStream& getStream (eStreamType streamType) { return mStreams[streamType]; }
    void enableStreams();

  private:
    uint8_t* tsHeader (uint8_t* ts, uint16_t pid, uint8_t continuityCount);

    void writePat (uint16_t tsid);
    void writePmt();
    void writeSection (uint8_t* ts, uint8_t* tsSectionStart, uint8_t* tsPtr);

    //{{{  vars
    // var
    const uint16_t mSid;
    iOptions* mOptions = nullptr;
    uint16_t mProgramPid = 0;

    // streams
    std::array <cStream, eStreamTypeSize> mStreams;

    // record
    std::string mChannelName;
    bool mChannelRecord = false;
    std::string mChannelRecordName;
    FILE* mFile = nullptr;

    // epg
    cEpgItem* mNowEpgItem = nullptr;
    std::map <std::chrono::system_clock::time_point, cEpgItem*> mEpgItemMap;
    //}}}
    };
  //}}}
  //{{{
  class cOptions {
  public:
    virtual ~cOptions() = default;

    std::string mRecordRoot;
    bool mRecordAll = false;

    bool mShowFirstService = false;
    bool mShowAllServices = true;
    };
  //}}}

  cTransportStream (const cDvbMultiplex& dvbMultiplex, iOptions* options);
  virtual ~cTransportStream() { clear(); }

  // gets
  uint64_t getNumPackets() const { return mNumPackets; }
  uint64_t getNumErrors() const { return mNumErrors; }

  // tdtTime
  std::string getTdtTimeString() const;
  bool hasTdtTime() const { return mFirstTimeDefined; }
  std::chrono::system_clock::time_point getTdtTime() const { return mTdtTime; }

  // maps
  std::map <uint16_t, cPidInfo>& getPidInfoMap() { return mPidInfoMap; };
  std::map <uint16_t, cService>& getServiceMap() { return mServiceMap; };

  cService* getService (uint16_t sid);
  std::vector <std::string>& getRecordPrograms() { return mRecordPrograms; }
  int64_t demux (uint8_t* chunk, int64_t chunkSize, int64_t streamPos, int64_t skipPts);

private:
  void clear();
  void clearPidCounts();
  void clearPidContinuity();

  cPidInfo& getPidInfo (uint16_t pid);
  cPidInfo* getPsiPidInfo (uint16_t pid);

  void foundService (cService& service);
  bool renderPes (cPidInfo& pidInfo, int64_t skipPts);

  void startServiceProgram (cService& service,
                            std::chrono::system_clock::time_point tdtTime,
                            const std::string& programName,
                            std::chrono::system_clock::time_point programStartTime,
                            bool selected);
  void programPesPacket (uint16_t sid, uint16_t pid, uint8_t* ts);
  void stopServiceProgram (cService& service);

  //{{{  parsers
  void parsePat (cPidInfo* pidInfo, uint8_t* buf);
  void parseNit (cPidInfo* pidInfo, uint8_t* buf);
  void parseSdt (cPidInfo* pidInfo, uint8_t* buf);
  void parseEit (cPidInfo* pidInfo, uint8_t* buf);
  void parseTdt (cPidInfo* pidInfo, uint8_t* buf);
  void parsePmt (cPidInfo* pidInfo, uint8_t* buf);

  int parsePsi (cPidInfo* pidInfo, uint8_t* buf);
  //}}}

  // vars
  const cDvbMultiplex mDvbMultiplex;
  iOptions* mOptions;
  bool mShowingFirstService = false;

  std::mutex mMutex;
  uint64_t mNumPackets = 0;
  uint64_t mNumErrors = 0;

  std::map <uint16_t, uint16_t> mProgramMap;
  std::map <uint16_t, cPidInfo> mPidInfoMap;
  std::map <uint16_t, cService> mServiceMap;

  // record
  std::mutex mRecordFileMutex;
  std::vector <std::string> mRecordPrograms;

  // time
  bool mFirstTimeDefined = false;
  std::chrono::system_clock::time_point mFirstTime; // first tdtTime seen
  std::chrono::system_clock::time_point mTdtTime;   // now tdtTime
  };
