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
#include <array>
#include <map>

#include <mutex>
#include <chrono>

#include <time.h>

#include "cDvbMultiplex.h"
#include "cDvbSource.h"
#include "iDvbDecoder.h"
#include "cDvbSubtitleDecoder.h"

class cTexture;
//}}}
using tTimePoint = std::chrono::system_clock::time_point;
using tDuration = std::chrono::seconds;

//{{{
class cPidInfo {
public:
  cPidInfo (uint16_t pid, bool isPsi) : mPid(pid), mPsi(isPsi) {}
  ~cPidInfo() { free (mBuffer); }

  std::string getTypeString();
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

  uint16_t mSid = 0xFFFF;
  uint16_t mStreamType = 0;

  int mPackets = 0;
  int mContinuity = -1;
  int mErrors = 0;
  int mRepeatContinuity = 0;

  int64_t mPts = -1;
  int64_t mFirstPts = -1;
  int64_t mLastPts = -1;

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
  cEpgItem (bool now, bool record, tTimePoint time, tDuration duration,
            const std::string& titleString, const std::string& infoString)
    : mNow(now), mRecord(record), mTime(time), mDuration(duration), mTitleString(titleString), mInfoString(infoString) {}
  ~cEpgItem() {}

  bool getRecord() { return mRecord; }
  std::string getTitleString() { return mTitleString; }
  std::string getDesriptionString() { return mInfoString; }

  tDuration getDuration() { return mDuration; }
  tTimePoint getTime() { return mTime; }

  //{{{
  bool toggleRecord() {
    mRecord = !mRecord;
    return mRecord;
    }
  //}}}
  //{{{
  void set (tTimePoint time, tDuration duration, const std::string& titleString, const std::string& infoString) {
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
  tDuration mDuration;

  std::string mTitleString;
  std::string mInfoString;
  };
//}}}
//{{{
class cService {
public:
  cService (uint16_t sid) : mSid(sid) {}
  ~cService();

  //{{{  gets
  uint16_t getSid() const { return mSid; }
  uint16_t getProgramPid() const { return mProgramPid; }
  uint16_t getVidPid() const { return mVidPid; }
  uint16_t getVidStreamType() const { return mVidStreamType; }
  uint16_t getAudPid() const { return mAudPid; }
  uint16_t getAudStreamType() const { return mAudStreamType; }
  uint16_t getAudOtherPid() const { return mAudOtherPid; }
  uint16_t getSubPid() const { return mSubPid; }
  uint16_t getSubStreamType() const { return mSubStreamType; }

  cEpgItem* getNowEpgItem() { return mNowEpgItem; }
  std::string getChannelString() { return mChannelString; }
  std::string getNowTitleString() { return mNowEpgItem ? mNowEpgItem->getTitleString() : ""; }
  std::map <std::chrono::system_clock::time_point, cEpgItem*>& getEpgItemMap() { return mEpgItemMap; }

  iDvbDecoder* getDvbDecoder() const { return mDvbDecoder; }
  //}}}
  //{{{  sets
  void setProgramPid (uint16_t pid) { mProgramPid = pid; }
  void setVidPid (uint16_t pid, uint16_t streamType) { mVidPid = pid; mVidStreamType = streamType; }
  void setAudPid (uint16_t pid, uint16_t streamType);
  void setSubPid (uint16_t pid, uint16_t streamType) { mSubPid = pid; mSubStreamType = streamType; }
  void setChannelString (const std::string& channelString) { mChannelString = channelString;}

  void setDvbDecoder (iDvbDecoder* dvbDecoder) { mDvbDecoder = dvbDecoder; }

  bool setNow (bool record,
               std::chrono::system_clock::time_point time, std::chrono::seconds duration,
               const std::string& str1, const std::string& str2);
  bool setEpg (bool record,
               std::chrono::system_clock::time_point startTime, std::chrono::seconds duration,
               const std::string& titleString, const std::string& infoString);
  //}}}

  // epg
  bool isEpgRecord (const std::string& title, std::chrono::system_clock::time_point startTime);
  bool getShowEpg() { return mShowEpg; }
  void toggleShowEpg() { mShowEpg = !mShowEpg; }

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
  uint16_t mProgramPid = 0xFFFF;
  uint16_t mVidPid = 0xFFFF;
  uint16_t mVidStreamType = 0;
  uint16_t mAudPid = 0xFFFF;
  uint16_t mAudOtherPid = 0xFFFF;
  uint16_t mAudStreamType = 0;
  uint16_t mSubPid = 0xFFFF;
  uint16_t mSubStreamType = 0;

  std::string mChannelString;
  iDvbDecoder* mDvbDecoder = nullptr;

  // epg
  cEpgItem* mNowEpgItem = nullptr;
  std::map <std::chrono::system_clock::time_point, cEpgItem*> mEpgItemMap;
  bool mShowEpg = true;

  // record
  FILE* mFile = nullptr;
  };
//}}}

class cDvbTransportStream {
public:
  cDvbTransportStream (const cDvbMultiplex& dvbMultiplex, const std::string& recordRootName, bool decodeSubtitle);
  virtual ~cDvbTransportStream();

  //  gets
  uint64_t getNumPackets() const { return mNumPackets; }
  std::chrono::system_clock::time_point getTime() const { return mTime; }
  std::map <uint16_t, cPidInfo>& getPidInfoMap() { return mPidInfoMap; };
  std::map <uint16_t, cService>& getServiceMap() { return mServiceMap; };

  uint64_t getNumErrors() const { return mNumErrors; }
  std::string getErrorString() { return mErrorString; }
  std::string getSignalString() { return mSignalString; }

  cService* getService (uint16_t sid);
  cService* getService (uint16_t index, int64_t& firstPts, int64_t& lastPts);
  static char getFrameType (uint8_t* pesBuf, int64_t pesBufSize, int streamType);
  std::vector <std::string>& getRecordPrograms() { return mRecordPrograms; }

  // subtitle
  bool getDecodeSubtitle() const { return mDecodeSubtitle; }
  iDvbDecoder* getDecoder (uint16_t sid);

  void toggleDecodeSubtitle();

  void dvbSource (bool ownThread);
  void fileSource (bool ownThread, const std::string& fileName);

  int64_t demux (const std::vector<uint16_t>& pids, uint8_t* tsBuf, int64_t tsBufSize, int64_t streamPos, bool skip);

  // vars
  std::mutex mMutex;

private:
  void clear();
  void clearPidCounts();
  void clearPidContinuity();

  int64_t getPts (uint8_t* tsPtr);
  cPidInfo* getPidInfo (uint16_t pid, bool createPsiOnly);
  std::string getChannelString (uint16_t sid);

  void startServiceProgram (cService* service, tTimePoint tdtTime,
                            const std::string& programName, tTimePoint programStartTime, bool selected);
  void programPesPacket (uint16_t sid, uint16_t pid, uint8_t* ts);
  void stopServiceProgram (cService* service);

  bool audDecodePes (cPidInfo* pidInfo, bool skip);
  bool audAltDecodePes (cPidInfo* pidInfo, bool skip) { (void)pidInfo; (void)skip; return false; }
  bool vidDecodePes (cPidInfo* pidInfo, bool skip);
  bool subDecodePes (cPidInfo* pidInfo);

  void parsePat (cPidInfo* pidInfo, uint8_t* buf);
  void parseNit (cPidInfo* pidInfo, uint8_t* buf);
  void parseSdt (cPidInfo* pidInfo, uint8_t* buf);
  void parseEit (cPidInfo* pidInfo, uint8_t* buf);
  void parseTdt (cPidInfo* pidInfo, uint8_t* buf);
  void parsePmt (cPidInfo* pidInfo, uint8_t* buf);
  int parsePsi (cPidInfo* pidInfo, uint8_t* buf);

  void dvbSourceInternal (bool ownThread);
  void fileSourceInternal (bool ownThread, const std::string& fileName);

  // vars
  cDvbMultiplex mDvbMultiplex;

  // dvbSource
  cDvbSource* mDvbSource = nullptr;
  uint64_t mLastErrors = 0;
  std::string mErrorString;
  std::string mSignalString;

  // record
  std::mutex mRecordFileMutex;
  std::string mRecordRootName;
  std::vector <std::string> mRecordPrograms;

  // subtitle
  bool mDecodeSubtitle = false;

  uint64_t mNumPackets = 0;
  uint64_t mNumErrors = 0;

  std::map <uint16_t, uint16_t> mProgramMap;
  std::map <uint16_t, cPidInfo> mPidInfoMap;
  std::map <uint16_t, cService> mServiceMap;

  bool mTimeDefined = false;
  std::chrono::system_clock::time_point mTime;
  std::chrono::system_clock::time_point mFirstTime;
  };
