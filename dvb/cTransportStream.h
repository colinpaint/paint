//{{{  cTransportStream.h - transport stream parser
// PAT inserts <pid,sid> into mProgramMap
// PMT declares pgm and elementary stream pids, adds cService into mServiceMap
// SDT names a service in mServiceMap
//}}}
//{{{  includes
#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>

#include <time.h>

#include <chrono>
#include "../utils/date.h"
//}}}

//{{{
class cPidInfo {
public:
  cPidInfo (int pid, bool isPsi) : mPid(pid), mPsi(isPsi) {}
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
  const int mPid;
  const bool mPsi;

  int mSid = -1;
  int mStreamType = 0;

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
  cEpgItem (bool now, bool record,
            std::chrono::system_clock::time_point time, std::chrono::seconds duration,
            const std::string& titleString, const std::string& infoString)
    : mNow(now), mRecord(record), mTime(time), mDuration(duration), mTitleString(titleString), mInfoString(infoString) {}
  ~cEpgItem() {}

  bool getRecord() { return mRecord; }
  std::string getTitleString() { return mTitleString; }
  std::string getDesriptionString() { return mInfoString; }

  std::chrono::seconds getDuration() { return mDuration; }
  std::chrono::system_clock::time_point getTime() { return mTime; }

  //{{{
  bool toggleRecord() {
    mRecord = !mRecord;
    return mRecord;
    }
  //}}}
  //{{{
  void set (std::chrono::system_clock::time_point time, std::chrono::seconds duration,
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
class cService {
public:
  cService (int sid) : mSid(sid) {}
  ~cService();

  // gets
  int getSid() const { return mSid; }
  int getProgramPid() const { return mProgramPid; }
  int getVidPid() const { return mVidPid; }
  int getVidStreamType() const { return mVidStreamType; }
  int getAudPid() const { return mAudPid; }
  int getAudStreamType() const { return mAudStreamType; }
  int getAudOtherPid() const { return mAudOtherPid; }
  int getSubPid() const { return mSubPid; }
  int getSubStreamType() const { return mSubStreamType; }
  cEpgItem* getNowEpgItem() { return mNowEpgItem; }
  std::string getChannelString() { return mChannelString; }
  std::string getNowTitleString() { return mNowEpgItem ? mNowEpgItem->getTitleString() : ""; }
  std::map <std::chrono::system_clock::time_point, cEpgItem*>& getEpgItemMap() { return mEpgItemMap; }

  //  sets
  void setProgramPid (int pid) { mProgramPid = pid; }
  void setVidPid (int pid, int streamType) { mVidPid = pid; mVidStreamType = streamType; }
  void setAudPid (int pid, int streamType);
  void setSubPid (int pid, int streamType) { mSubPid = pid; mSubStreamType = streamType; }
  void setChannelString (const std::string& channelString) { mChannelString = channelString;}

  bool setNow (bool record,
               std::chrono::system_clock::time_point time, std::chrono::seconds duration,
               const std::string& str1, const std::string& str2);
  bool setEpg (bool record,
               std::chrono::system_clock::time_point startTime, std::chrono::seconds duration,
               const std::string& titleString, const std::string& infoString);

  // override info
  bool getShowEpg() { return mShowEpg; }
  bool isEpgRecord (const std::string& title, std::chrono::system_clock::time_point startTime);
  void toggleShowEpg() { mShowEpg = !mShowEpg; }

  bool openFile (const std::string& fileName, int tsid);
  void writePacket (uint8_t* ts, int pid);
  void closeFile();

private:
  uint8_t* tsHeader (uint8_t* ts, int pid, uint8_t continuityCount);
  void writePat (int tsid);
  void writePmt();
  void writeSection (uint8_t* ts, uint8_t* tsSectionStart, uint8_t* tsPtr);

  // vars
  const int mSid;
  int mProgramPid = -1;
  int mVidPid = -1;
  int mVidStreamType = 0;
  int mAudPid = -1;
  int mAudOtherPid = -1;
  int mAudStreamType = 0;
  int mSubPid = -1;
  int mSubStreamType = 0;

  std::string mChannelString;
  cEpgItem* mNowEpgItem = nullptr;
  std::map <std::chrono::system_clock::time_point, cEpgItem*> mEpgItemMap;

  // override info, simpler to hold it here for now
  bool mShowEpg = true;
  FILE* mFile = nullptr;
  };
//}}}

class cTransportStream {
friend class cTsEpgBox;
friend class cTsPidBox;
public:
  virtual ~cTransportStream() { clear(); }

  //  gets
  uint64_t getErrors() { return mErrors; }
  std::chrono::system_clock::time_point getTime() { return mTime; }

  std::string getChannelStringBySid (int sid);
  cService* getService (int index, int64_t& firstPts, int64_t& lastPts);
  static char getFrameType (uint8_t* pesBuf, int64_t pesBufSize, int streamType);

  virtual void clear();
  int64_t demux (const std::vector<int>& pids, uint8_t* tsBuf, int64_t tsBufSize, int64_t streamPos, bool skip);

  // vars, public for widget
  std::mutex mMutex;
  std::map <int, cPidInfo> mPidInfoMap;
  std::map <int, cService> mServiceMap;

protected:
  //{{{
  virtual void start (cService* service, const std::string& name,
                      std::chrono::system_clock::time_point time,
                      std::chrono::system_clock::time_point startTime,
                      bool selected) {
   (void)service;
   (void)name;
   (void)time;
   (void)startTime;
   (void)selected;
   }
  //}}}
  virtual void pesPacket (int sid, int pid, uint8_t* ts) { (void)sid; (void)pid; (void)ts; }
  virtual void stop (cService* service) { (void)service; }

  virtual bool audDecodePes (cPidInfo* pidInfo, bool skip) { (void)pidInfo; (void)skip; return false; }
  virtual bool audAltDecodePes (cPidInfo* pidInfo, bool skip) { (void)pidInfo; (void)skip; return false; }
  virtual bool vidDecodePes (cPidInfo* pidInfo, bool skip) { (void)pidInfo; (void)skip; return false; }
  virtual bool subDecodePes (cPidInfo* pidInfo) { (void)pidInfo; return false; }

  void clearPidCounts();
  void clearPidContinuity();

private:
  int64_t getPts (uint8_t* tsPtr);
  cPidInfo* getPidInfo (int pid, bool createPsiOnly);

  void parsePat (cPidInfo* pidInfo, uint8_t* buf);
  void parseNit (cPidInfo* pidInfo, uint8_t* buf);
  void parseSdt (cPidInfo* pidInfo, uint8_t* buf);
  void parseEit (cPidInfo* pidInfo, uint8_t* buf);
  void parseTdt (cPidInfo* pidInfo, uint8_t* buf);
  void parsePmt (cPidInfo* pidInfo, uint8_t* buf);
  int parsePsi (cPidInfo* pidInfo, uint8_t* buf);

  // vars
  uint64_t mErrors = 0;

  std::map <int, int> mProgramMap;

  bool mTimeDefined = false;
  std::chrono::system_clock::time_point mTime;
  std::chrono::system_clock::time_point mFirstTime;
  };
