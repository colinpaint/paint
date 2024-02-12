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
#include <functional>
#include <mutex>

class cRender;

#include "../common/basicTypes.h"
#include "../common/iOptions.h"
#include "../common/utils.h"
#include "../common/cDvbUtils.h"

#include "cDvbMultiplex.h"
//}}}
constexpr int kAacLatmStreamType = 17;
constexpr int kH264StreamType = 27;

//{{{
class cRenderStream {
public:
  enum eType { eVideo, eAudio, eDescription, eSubtitle, eTypeSize };

  bool isDefined() const { return mDefined; }
  bool isEnabled() const { return mRender; }

  std::string getLabel() const { return mLabel; }
  uint16_t getPid() const { return mPid; }
  uint8_t getTypeId() const { return mTypeId; }
  std::string getTypeName() const { return mTypeName; }
  cRender& getRender() const { return *mRender; }

  void setLabel (const std::string& label) { mLabel = label; }
  //{{{
  void setPidTypeId (uint16_t pid, uint8_t typeId) {

    mDefined = true;

    mPid = pid;
    mTypeId = typeId;
    mTypeName = cDvbUtils::getStreamTypeName (typeId);
    }
  //}}}
  void setRender (cRender* render) { mRender = render; }

private:
  bool mDefined = false;
  cRender* mRender = nullptr;

  std::string mLabel;
  uint16_t mPid = 0;
  uint8_t mTypeId = 0;
  std::string mTypeName;
  };
//}}}

class cTransportStream {
public:
  //{{{
  class cEpgItem {
  public:
    //{{{
    cEpgItem (bool now, bool record,
              std::chrono::system_clock::time_point startTime,
              std::chrono::seconds duration,
              const std::string& title, const std::string& info) :
        mNow(now), mRecord(record),
        mStartTime(startTime), mDuration(duration),
        mTitle(title), mInfo(info) {}
    //}}}
    ~cEpgItem() = default;

    std::string getTitle() { return mTitle; }
    std::string getInfo() { return mInfo; }

    std::chrono::system_clock::time_point getStartTime() { return mStartTime; }
    std::chrono::seconds getDuration() { return mDuration; }

    bool getRecord() { return mRecord; }
    //{{{
    bool toggleRecord() {
      mRecord = !mRecord;
      return mRecord;
      }
    //}}}

    //{{{
    void set (std::chrono::system_clock::time_point startTime,
              std::chrono::seconds duration,
              const std::string& title,
              const std::string& info) {

      mStartTime = startTime;
      mDuration = duration;
      mTitle = title;
      mInfo = info;
      }
    //}}}

  private:
    const bool mNow = false;
    bool mRecord = false;

    std::chrono::system_clock::time_point mStartTime;
    std::chrono::seconds mDuration;

    std::string mTitle;
    std::string mInfo;
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
    int64_t getDts() const { return mDts; }
    int64_t getStreamPos() const { return mStreamPos; }

    uint8_t getStreamTypeId() const { return mStreamTypeId; }
    std::string getPidName() const ;
    std::string getInfo() const { return mInfo; }

    int getBufSize() const { return int(mBufPtr - mBuffer); }

    void setSid (uint16_t sid) { mSid = sid; }
    void setStreamTypeId (uint8_t streamTypeId) { mStreamTypeId = streamTypeId; }
    void setInfo (const std::string info) { mInfo = info; }
    void setPts (int64_t pts) { mPts = pts; }
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
    uint8_t mStreamTypeId = 0;

    std::string mInfo;

    int64_t mPts = -1;
    int64_t mDts = -1;
    };
  //}}}
  //{{{
  class cService {
  public:
    cService (uint16_t sid, iOptions* options);
    ~cService();

    // gets
    uint16_t getSid() const { return mSid; }
    std::string getName() const { return mName; }
    uint16_t getProgramPid() const { return mProgramPid; }
    uint16_t getVideoPid() const { return mVideoPid; }
    uint16_t getAudioPid() const { return mAudioPid; }
    uint16_t getSubtitlePid() const { return mSubtitlePid; }
    int64_t getPtsFromStart();

    // sets
    void setProgramPid (uint16_t pid) { mProgramPid = pid; }
    void setVideoPid (uint16_t pid) { mVideoPid = pid; }
    void setAudioPid (uint16_t pid) { mAudioPid = pid; }
    void setSubtitlePid (uint16_t pid) { mSubtitlePid = pid; }
    //{{{
    void setName (const std::string& name, bool record, const std::string& recordName) {

      mName = name;
      mRecord = record;
      mRecordName = recordName;
      }
    //}}}

    // streams
    cRenderStream* getStreamByPid (uint16_t pid);
    cRenderStream& getStream (cRenderStream::eType streamType) { return mStreams[streamType]; }
    void enableStream (cRenderStream::eType streamType);

    bool throttle();
    void togglePlay();
    int64_t skip (int64_t skipPts);

    // record
    bool getRecord() const { return mRecord; }
    std::string getRecordName() const { return mRecordName; }

    void start (std::chrono::system_clock::time_point tdt,
                const std::string& programName,
                std::chrono::system_clock::time_point programStartTime,
                bool selected);
    void pesPacket (uint16_t pid, uint8_t* ts);

    // epg
    bool isEpgRecord (const std::string& title, std::chrono::system_clock::time_point startTime);
    cEpgItem* getNowEpgItem() { return mNowEpgItem; }
    std::string getNowTitleString() const { return mNowEpgItem ? mNowEpgItem->getTitle() : ""; }
    std::map <std::chrono::system_clock::time_point, cEpgItem*>& getTodayEpg() { return mTodayEpg; }
    std::map <std::chrono::system_clock::time_point, cEpgItem*>& getOtherEpg() { return mOtherEpg; }
    //{{{
    bool setNow (bool record,
                 std::chrono::system_clock::time_point startTime,
                 std::chrono::seconds duration,
                 const std::string& title,
                 const std::string& info);
    //}}}
    //{{{
    void setEpg (bool record,
                 std::chrono::system_clock::time_point nowTime,
                 std::chrono::system_clock::time_point startTime,
                 std::chrono::seconds duration,
                 const std::string& title,
                 const std::string& info);
    //}}}

  private:
    static uint8_t* tsHeader (uint8_t* ts, uint16_t pid, uint8_t continuityCount);
    static void writeSection (FILE* file, uint8_t* ts, uint8_t* tsSectionStart, uint8_t* tsPtr);

    bool openFile (const std::string& fileName, uint16_t tsid);
    void writePat (uint16_t tsid);
    void writePmt();
    void closeFile();

    // vars
    const uint16_t mSid;
    iOptions* mOptions = nullptr;
    std::string mName;

    uint16_t mProgramPid = 0;

    uint16_t mVideoPid = 0;
    uint16_t mAudioPid = 0;
    uint16_t mSubtitlePid = 0;

    // streams
    std::array <cRenderStream, cRenderStream::eTypeSize> mStreams;

    // record
    bool mRecord = false;
    std::string mRecordName;

    bool mRecording = false;
    FILE* mFile = nullptr;

    // epg
    cEpgItem* mNowEpgItem = nullptr;
    std::map <std::chrono::system_clock::time_point, cEpgItem*> mTodayEpg;
    std::map <std::chrono::system_clock::time_point, cEpgItem*> mOtherEpg;
    };
  //}}}
  //{{{
  class cOptions {
  public:
    virtual ~cOptions() = default;

    bool mRecordAllServices = false;
    std::string mRecordRoot;
    };
  //}}}

  cTransportStream (const cDvbMultiplex& dvbMultiplex,
                    iOptions* options,
                    const std::function<void (cService& service)> addServiceCallback =
                      [](cService& service) { (void)service; },
                    const std::function<void (cService& service, cPidInfo&pidInfo, bool skip)> pesCallback =
                      [](cService& service, cPidInfo& pidInfo, bool skip) { (void)service;  (void)pidInfo; (void)skip; });
  ~cTransportStream() { clear(); }

  // gets
  uint64_t getNumPackets() const { return mNumPackets; }
  uint64_t getNumErrors() const { return mNumErrors; }

  // tdt
  bool hasFirstTdt() const { return mHasFirstTdt; }
  std::chrono::system_clock::time_point getNowTdt() const { return mNowTdt; }
  std::string getTdtString() const;

  std::map <uint16_t, cPidInfo>& getPidInfoMap() { return mPidInfoMap; };
  std::map <uint16_t, cService>& getServiceMap() { return mServiceMap; };

  bool throttle();
  void togglePlay();
  int64_t getSkipOffset (int64_t skipPts);

  // demux
  int64_t demux (uint8_t* chunk, int64_t chunkSize, int64_t streamPos, bool skip);

private:
  void clear();
  void clearPidCounts();
  void clearPidContinuity();

  cPidInfo& getPidInfo (uint16_t pid);
  cPidInfo* getPsiPidInfo (uint16_t pid);
  cService* getServiceBySid (uint16_t sid);

  //  parse
  uint16_t parsePSI (cPidInfo& pidInfo, uint8_t* buf);
  void parseTDT (cPidInfo& pidInfo, uint8_t* buf);
  void parsePAT (uint8_t* buf);
  void parseSDT (uint8_t* buf);
  void parseEIT (uint8_t* buf);
  void parsePMT (cPidInfo& pidInfo, uint8_t* buf);

  // vars
  const cDvbMultiplex mDvbMultiplex;
  iOptions* mOptions;

  uint64_t mNumPackets = 0;
  uint64_t mNumErrors = 0;

  std::mutex mPidInfoMutex;
  std::map <uint16_t, cPidInfo> mPidInfoMap;
  std::map <uint16_t, uint16_t> mProgramMap;

  std::mutex mServiceMapMutex;
  std::map <uint16_t, cService> mServiceMap;

  // callbacks
  const std::function<void (cService& service)> mAddServiceCallback;
  const std::function<void (cService& service, cPidInfo& pidInfo, bool skip)> mPesCallback;

  // time
  bool mHasFirstTdt = false;
  std::chrono::system_clock::time_point mFirstTdt; // first tdt seen
  std::chrono::system_clock::time_point mNowTdt;   // now tdt
  };
