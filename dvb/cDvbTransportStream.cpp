// cDvbTransportStream.cpp - file or dvbSource -> transportStream demux
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
//{{{  linux includes
#ifdef __linux__
  #include <unistd.h>
  #include <sys/poll.h>
#endif
//}}}
//{{{  common includes
#include "cDvbTransportStream.h"

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <thread>

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/utils.h"

#include "cDvbUtils.h"
#include "cDvbSubtitle.h"

using namespace std;
//}}}

// public:
//{{{
cDvbTransportStream::cDvbTransportStream (const cDvbMultiplex& dvbMultiplex,
                                          const std::string& recordRootName, bool decodeSubtitle)
    : mDvbMultiplex(dvbMultiplex), mRecordRootName(recordRootName), mDecodeSubtitle(decodeSubtitle) {

  mDvbSource = new cDvbSource (dvbMultiplex.mFrequency, 0);
  }
//}}}
//{{{
cDvbTransportStream::~cDvbTransportStream() {
    mDvbSubtitleMap.clear();
  }
//}}}

//{{{
bool cDvbTransportStream::hasSubtitle (uint16_t sid) {
  return mDvbSubtitleMap.find (sid) != mDvbSubtitleMap.end();
  }
//}}}
//{{{
cDvbSubtitle& cDvbTransportStream::getSubtitle (uint16_t sid) {

  auto it = mDvbSubtitleMap.find (sid);
  return (*it).second;
  }
//}}}

//{{{
void cDvbTransportStream::toggleDecodeSubtitle() {

  mDecodeSubtitle = !mDecodeSubtitle;

  if (!mDecodeSubtitle)
      mDvbSubtitleMap.clear();
  }
//}}}

//{{{
void cDvbTransportStream::dvbSource (bool ownThread) {

  if (ownThread)
    thread([=, this]() { dvbSourceInternal (true); }).detach();
  else
    dvbSourceInternal (false);
  }
//}}}
//{{{
void cDvbTransportStream::fileSource (bool ownThread, const string& fileName) {

  if (ownThread)
    thread ([=, this](){ fileSourceInternal (true, fileName); } ).detach();
  else
    fileSourceInternal (false, fileName);
  }
//}}}

// protected:
//{{{
void cDvbTransportStream::startServiceProgram (cService* service, tTimePoint tdtTime,
                                               const string& programName,
                                               tTimePoint programStartTime, bool selected) {
// start recording service item

  (void)programStartTime;

  // cloase prev program on this service
  lock_guard<mutex> lockGuard (mRecordFileMutex);
  service->closeFile();

  string channelRecordName;
  bool recordProgram = mDvbMultiplex.mRecordAllChannels || selected;
  if (!recordProgram) {
    // filter and rename channel prefix
    size_t i = 0;
    for (auto& channelName : mDvbMultiplex.mChannels) {
      if (channelName == service->getChannelString()) {
        // if channelName recognised, record every item for that channel using channelRootName
        recordProgram = true;
        if (i < mDvbMultiplex.mChannelRecordNames.size())
          channelRecordName = mDvbMultiplex.mChannelRecordNames[i] +  " ";
        break;
        }
      i++;
      }
    }

  if (recordProgram & (service->getVidPid() > 0) && (service->getAudPid() > 0)) {
    string recordFilePath = mRecordRootName +
                            channelRecordName +
                            date::format ("%d %b %y %a %H.%M.%S ", date::floor<chrono::seconds>(tdtTime)) +
                            validFileString (programName, "<>:/|?*\"\'\\") +
                            ".ts";

    // record
    service->openFile (recordFilePath, 0x1234);

    // gui
    mRecordPrograms.push_back (recordFilePath);

    // log program start time,date filename
    string eitStartTime = date::format ("%H.%M.%S %a %d %b %y", date::floor<chrono::seconds>(programStartTime));
    cLog::log (LOGINFO, fmt::format ("{} {}", eitStartTime, recordFilePath));
    }
  }
//}}}
//{{{
void cDvbTransportStream::programPesPacket (uint16_t sid, uint16_t pid, uint8_t* ts) {
// look up service and write it

  lock_guard<mutex> lockGuard (mRecordFileMutex);

  auto serviceIt = getServiceMap().find (sid);
  if (serviceIt != getServiceMap().end())
    serviceIt->second.writePacket (ts, pid);
  }
//}}}
//{{{
void cDvbTransportStream::stopServiceProgram (cService* service) {
// stop recording service, never called

  lock_guard<mutex> lockGuard (mRecordFileMutex);
  service->closeFile();
  }
//}}}

//{{{
bool cDvbTransportStream::audDecodePes (cPidInfo* pidInfo, bool skip) {

  (void)pidInfo;
  (void)skip;

  //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " a - " + dec(pidInfo->getBufUsed());
  return false;
  }
//}}}
//{{{
bool cDvbTransportStream::vidDecodePes (cPidInfo* pidInfo, bool skip) {

  (void)pidInfo;
  (void)skip;

  //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " v - " + dec(pidInfo->getBufUsed());
  return false;
  }
//}}}
//{{{
bool cDvbTransportStream::subDecodePes (cPidInfo* pidInfo) {

  //{{{
  //cLog::log (LOGINFO1, fmt::format ("subtitle pid:{} sid:{} size:{} {} {} ",
                                      //pidInfo->mPid,
                                      //pidInfo->mSid,
                                      //pidInfo->getBufUsed(),
                                      //getFullPtsString (pidInfo->mPts),
                                      //getChannelStringBySid (pidInfo->mSid)));
  //}}}

  if (mDecodeSubtitle) {
    auto it = mDvbSubtitleMap.find (pidInfo->mSid);
    if (it == mDvbSubtitleMap.end()) // create service dvbSubtitle
      it = mDvbSubtitleMap.insert (
        tDvbSubtitleMap::value_type (pidInfo->mSid, 
                                     cDvbSubtitle (pidInfo->mSid, getChannelStringBySid (pidInfo->mSid)))).first;
    it->second.decode (pidInfo->mBuffer, pidInfo->getBufUsed());
    }

  return false;
  }
//}}}

// private:
//{{{
void cDvbTransportStream::dvbSourceInternal (bool ownThread) {

  if (!mDvbSource->ok()) {
    cLog::log (LOGERROR, "dvbSource - no dvbSource");
    return;
    }

  if (ownThread)
    cLog::setThreadName ("grab");

  FILE* mFile = mDvbMultiplex.mRecordAllChannels ?
    fopen ((mRecordRootName + mDvbMultiplex.mName + ".ts").c_str(), "wb") : nullptr;

  #ifdef _WIN32
    mDvbSource->run();
    int64_t streamPos = 0;
    auto blockSize = 0;
    while (true) {
      auto ptr = mDvbSource->getBlockBDA (blockSize);
      if (blockSize) {
        //{{{  read and demux block
        if (mFile)
          fwrite (ptr, 1, blockSize, mFile);

        streamPos += demux ({}, ptr, blockSize, streamPos, false);
        mDvbSource->releaseBlock (blockSize);

        mErrorString.clear();
        if (getNumErrors())
          mErrorString += fmt::format ("{}err", getNumErrors());
        if (streamPos < 1000000)
          mErrorString = fmt::format ("{}k", streamPos / 1000);
        else
          mErrorString = fmt::format ("{}m", streamPos / 1000000);
        }
        //}}}
      else
        this_thread::sleep_for (1ms);
      mSignalString = mDvbSource->getStatusString();
      }
  #endif

  #ifdef __linux__
    constexpr int kDvrReadBufferSize = 50 * 188;
    auto buffer = (uint8_t*)malloc (kDvrReadBufferSize);

    uint64_t streamPos = 0;
    while (true) {
      int bytesRead = mDvbSource->getBlock (buffer, kDvrReadBufferSize);
      if (bytesRead == 0)
        cLog::log (LOGINFO, "cDvb grabThread no bytes read");
      else {
        // demux
        streamPos += demux ({}, buffer, bytesRead, 0, false);
        if (mFile)
          fwrite (buffer, 1, bytesRead, mFile);

        // get status
        mSignalString = mDvbSource->getStatusString();

        // log if more errors
        bool show = getNumErrors() != mLastErrors;
        mLastErrors = getNumErrors();
        if (show)
          cLog::log (LOGINFO, fmt::format ("err:{} {}", getNumErrors(), mSignalString));
        }
      }
    free (buffer);
  #endif

  if (mFile)
    fclose (mFile);

  if (ownThread)
    cLog::log (LOGINFO, "exit");
  }
//}}}
//{{{
void cDvbTransportStream::fileSourceInternal (bool ownThread, const string& fileName) {

  if (ownThread)
    cLog::setThreadName ("read");

  auto file = fopen (fileName.c_str(), "rb");
  if (!file) {
    //{{{  error, return
    cLog::log (LOGERROR, "no file " + fileName);
    return;
    }
    //}}}

  uint64_t streamPos = 0;
  auto blockSize = 188 * 8;
  auto buffer = (uint8_t*)malloc (blockSize);

  int i = 0;
  bool run = true;
  while (run) {
    i++;
    if (!(i % 200)) // throttle read rate
      this_thread::sleep_for (20ms);

    size_t bytesRead = fread (buffer, 1, blockSize, file);
    if (bytesRead > 0)
      streamPos += demux ({}, buffer, bytesRead, streamPos, false);
    else
      break;
    //mErrorStr = fmt::format ("{}", getNumErrors());
    }

  fclose (file);
  free (buffer);

  if (ownThread)
    cLog::log (LOGERROR, "exit");
  }
//}}}
