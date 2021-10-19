// cDvbTransportStream.cpp - file or dvbSource -> transportStream demux
//{{{  windows includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
  #define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

  #include <wrl.h>
  #include <initguid.h>
  #include <DShow.h>
  #include <bdaiface.h>
  #include <ks.h>
  #include <ksmedia.h>
  #include <bdatif.h>

  #include <locale>
  #include <codecvt>

  MIDL_INTERFACE ("0579154A-2B53-4994-B0D0-E773148EFF85")
  ISampleGrabberCB : public IUnknown {
  public:
    virtual HRESULT STDMETHODCALLTYPE SampleCB (double SampleTime, IMediaSample* pSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB (double SampleTime, BYTE* pBuffer, long BufferLen) = 0;
    };

  MIDL_INTERFACE ("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
  ISampleGrabber : public IUnknown {
  public:
    virtual HRESULT STDMETHODCALLTYPE SetOneShot (BOOL OneShot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType (const AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType (AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples (BOOL BufferThem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer (long* pBufferSize, long* pBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample (IMediaSample** ppSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback (ISampleGrabberCB* pCallback, long WhichMethodToCallback) = 0;
    };
  EXTERN_C const CLSID CLSID_SampleGrabber;

  #include <bdamedia.h>
  DEFINE_GUID (CLSID_DVBTLocator, 0x9CD64701, 0xBDF3, 0x4d14, 0x8E,0x03, 0xF1,0x29,0x83,0xD8,0x66,0x64);
  DEFINE_GUID (CLSID_BDAtif, 0xFC772ab0, 0x0c7f, 0x11d3, 0x8F,0xf2, 0x00,0xa0,0xc9,0x22,0x4c,0xf4);
  DEFINE_GUID (CLSID_Dump, 0x36a5f770, 0xfe4c, 0x11ce, 0xa8, 0xed, 0x00, 0xaa, 0x00, 0x2f, 0xea, 0xb5);

  #pragma comment (lib,"strmiids")

  #include <thread>
#endif
//}}}
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
#include "cSubtitle.h"

using namespace std;
//}}}

constexpr bool kDebug = false;

// public:
//{{{
cDvbTransportStream::cDvbTransportStream (const cDvbMultiplex& dvbMultiplex,
                                          const std::string& recordRootName, bool subtitle)
    : mDvbMultiplex(dvbMultiplex), mRecordRootName(recordRootName), mSubtitle(subtitle) {

  mDvbSource = new cDvbSource (dvbMultiplex.mFrequency, 0);
  }
//}}}
//{{{
cDvbTransportStream::~cDvbTransportStream() {

  for (auto& subtitle : mSubtitleMap)
    delete (subtitle.second);
  mSubtitleMap.clear();
  }
//}}}

//{{{
cSubtitle* cDvbTransportStream::getSubtitleBySid (uint16_t sid) {

  auto it = mSubtitleMap.find (sid);
  return (it == mSubtitleMap.end()) ? nullptr : (*it).second;
  }
//}}}

//{{{
void cDvbTransportStream::setSubtitle (bool subtitle) {

  mSubtitle = subtitle;
  if (!mSubtitle)
    mSubtitleMap.clear();
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
  void cDvbTransportStream::startServiceItem (cService* service, const string& itemName,
                                              chrono::system_clock::time_point time,
                                              chrono::system_clock::time_point itemStarttime,
                                              bool itemSelected) {
  // start recording service item

    (void)itemStarttime;

    lock_guard<mutex> lockGuard (mRecordFileMutex);
    service->closeFile();

    bool recordItem = itemSelected || mDvbMultiplex.mRecordAllChannels;
    string channelRecordName;
    if (!mDvbMultiplex.mRecordAllChannels) {
      // filter and rename channel prefix
      size_t i = 0;
      for (auto& channelName : mDvbMultiplex.mChannels) {
        if (channelName == service->getChannelString()) {
          // if channelName recognised, record every item for that channel using channelRootName
          recordItem = true;
          if (i < mDvbMultiplex.mChannelRecordNames.size())
            channelRecordName = mDvbMultiplex.mChannelRecordNames[i] +  " ";
          break;
          }
        i++;
        }
      }

    if (recordItem) {
      if ((service->getVidPid() > 0) &&
          (service->getAudPid() > 0) &&
          (service->getSubPid() > 0)) {
        string recordFilePath = mRecordRootName +
                                channelRecordName +
                                date::format ("%d %b %y %a %H.%M.%S ", date::floor<chrono::seconds>(time)) +
                                validFileString (itemName, "<>:/|?*\"\'\\") +
                                ".ts";
        service->openFile (recordFilePath, 0x1234);

        mRecordItems.push_back (recordFilePath);
        cLog::log (LOGINFO, recordFilePath);
        }
      }
    }
  //}}}
  //{{{
  void cDvbTransportStream::pesPacket (uint16_t sid, uint16_t pid, uint8_t* ts) {
  // look up service and write it

    lock_guard<mutex> lockGuard (mRecordFileMutex);

    auto serviceIt = getServiceMap().find (sid);
    if (serviceIt != getServiceMap().end())
      serviceIt->second.writePacket (ts, pid);
    }
  //}}}
  //{{{
  void cDvbTransportStream::stopServiceItem (cService* service) {
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

    if (kDebug)
      cLog::log (LOGINFO1, fmt::format ("subtitle pid:{} sid:{} size:{} {} {} ",
                                        pidInfo->mPid,
                                        pidInfo->mSid,
                                        pidInfo->getBufUsed(),
                                        getFullPtsString (pidInfo->mPts),
                                        getChannelStringBySid (pidInfo->mSid)));

    if (mSubtitle) {
      // find or create sid service cSubtitleContext
      auto it = mSubtitleMap.find (pidInfo->mSid);
      if (it == mSubtitleMap.end()) {
        auto insertPair = mSubtitleMap.insert (
          map <uint16_t, cSubtitle*>::value_type (pidInfo->mSid, new cSubtitle()));
        it = insertPair.first;
        cLog::log (LOGINFO1, fmt::format ("cDvb::subDecodePes - create serviceStuff sid:{}",pidInfo->mSid));
        }
      auto subtitle = it->second;

      subtitle->decode (pidInfo->mBuffer, pidInfo->getBufUsed());
      if (kDebug)
        subtitle->debug ("- ");
      }

    return false;
    }
  //}}}

// private:
//{{{
void cDvbTransportStream::dvbSourceInternal (bool ownThread) {

  if (ownThread)
    cLog::setThreadName ("grab");

  FILE* mFile = mDvbMultiplex.mRecordAllChannels ?
    fopen ((mRecordRootName + mDvbMultiplex.mName + ".ts").c_str(), "wb") : nullptr;

  #ifdef _WIN32
    if (!mDvbSource->mMediaControl)
      cLog::log (LOGERROR, "no dvbSource");
    else {
      auto hr = mDvbSource->mMediaControl->Run();
      if (hr == S_OK) {
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
          if (mDvbSource->mScanningTuner) {
            //{{{  update mSignalString
            long signal = 0;
            mDvbSource->mScanningTuner->get_SignalStrength (&signal);
            mSignalString = fmt::format ("signal {}", signal / 0x10000);
            }
            //}}}
          }
        }
      else
        cLog::log (LOGERROR, fmt::format ("run graph failed {}", hr));
      }
  #endif

  #ifdef __linux__
    if (!mDvbSource->mDvr)
      cLog::log (LOGERROR, "no dvbSource");
    else {
      constexpr int kDvrReadBufferSize = 50 * 188;
      auto buffer = (uint8_t*)malloc (kDvrReadBufferSize);

      uint64_t streamPos = 0;
      while (true) {
        int bytesRead = read (mDvbSource->mDvr, buffer, kDvrReadBufferSize);
        if (bytesRead > 0) {
          streamPos += demux ({}, buffer, bytesRead, 0, false);
          if (mFile)
            fwrite (buffer, 1, bytesRead, mFile);

          bool show = getNumErrors() != mLastErrors;
          mLastErrors = getNumErrors();

          mSignalString = mDvbSource->getStatusString();
          if (show) {
            mErrorString = fmt::format ("err:{}", getNumErrors());
            cLog::log (LOGINFO, mErrorString + " " + mSignalString);
            }
          }
        else
          cLog::log (LOGINFO, "cDvb grabThread no bytes read");
        }
      free (buffer);
      }
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
