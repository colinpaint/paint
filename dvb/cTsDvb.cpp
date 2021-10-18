// cTsDvb.cpp
#ifdef _WIN32
  //{{{  windows includes
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
  //}}}
#endif

#ifdef __linux__
  #include <unistd.h>
  #include <sys/poll.h>
#endif
//{{{  common includes
#include "cTsDvb.h"

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <thread>

#include "../utils/cLog.h"
#include "../utils/utils.h"

#include "cDvbUtils.h"
#include "cTransportStream.h"
#include "cSubtitle.h"

using namespace std;
//}}}

constexpr bool kDebug = false;

//{{{
class cDvbTransportStream : public cTransportStream {
public:
  //{{{
  cDvbTransportStream (const string& rootName,
                       const vector <string>& channelStrings, const vector <string>& saveStrings,
                       bool decodeSubtitle)
    : mRootName(rootName),
      mChannelStrings(channelStrings), mSaveStrings(saveStrings),
      mRecordAll ((channelStrings.size() == 1) && (channelStrings[0] == "all")),
      mDecodeSubtitle(decodeSubtitle) {}
  //}}}
  //{{{
  virtual ~cDvbTransportStream() {

    for (auto& subtitle : mSubtitleMap)
      delete (subtitle.second);

    mSubtitleMap.clear();
    }
  //}}}

  //{{{
  cSubtitle* getSubtitleBySid (int sid) {

    auto it = mSubtitleMap.find (sid);
    return (it == mSubtitleMap.end()) ? nullptr : (*it).second;
    }
  //}}}

protected:
  //{{{
  virtual void start (cService* service, const string& name,
                      chrono::system_clock::time_point time,
                      chrono::system_clock::time_point starttime,
                      bool selected) {

    (void)starttime;
    lock_guard<mutex> lockGuard (mFileMutex);

    service->closeFile();

    bool record = selected || mRecordAll;
    string saveName;

    if (!mRecordAll) {
      // filter and rename channel prefix
      size_t i = 0;
      for (auto& channelString : mChannelStrings) {
        if (channelString == service->getChannelString()) {
          record = true;
          if (i < mSaveStrings.size())
            saveName = mSaveStrings[i] +  " ";
          break;
          }
        i++;
        }
      }

    saveName += date::format ("%d %b %y %a %H.%M.%S ", date::floor<chrono::seconds>(time));

    if (record) {
      if ((service->getVidPid() > 0) &&
          (service->getAudPid() > 0) &&
          (service->getSubPid() > 0)) {
        auto validName = validFileString (name, "<>:/|?*\"\'\\");
        auto fileNameStr = mRootName + "/" + saveName + validName + ".ts";
        service->openFile (fileNameStr, 0x1234);
        cLog::log (LOGINFO, fileNameStr);
        }
      }
    }
  //}}}
  //{{{
  virtual void pesPacket (int sid, int pid, uint8_t* ts) {
  // look up service and write it

    lock_guard<mutex> lockGuard (mFileMutex);

    auto serviceIt = mServiceMap.find (sid);
    if (serviceIt != mServiceMap.end())
      serviceIt->second.writePacket (ts, pid);
    }
  //}}}
  //{{{
  virtual void stop (cService* service) {

    lock_guard<mutex> lockGuard (mFileMutex);

    service->closeFile();
    }
  //}}}

  //{{{
  virtual bool audDecodePes (cPidInfo* pidInfo, bool skip) {
    (void)pidInfo;
    (void)skip;
    //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " a - " + dec(pidInfo->getBufUsed());
    return false;
    }
  //}}}
  //{{{
  virtual bool vidDecodePes (cPidInfo* pidInfo, bool skip) {
    (void)pidInfo;
    (void)skip;
    //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " v - " + dec(pidInfo->getBufUsed());
    return false;
    }
  //}}}
  //{{{
  virtual bool subDecodePes (cPidInfo* pidInfo) {

    if (kDebug)
      cLog::log (LOGINFO1, fmt::format ("subtitle pid:{} sid:{} size:{} {} {} ",
                                        pidInfo->mPid,
                                        pidInfo->mSid,
                                        pidInfo->getBufUsed(),
                                        getFullPtsString (pidInfo->mPts),
                                        getChannelStringBySid (pidInfo->mSid)));

    if (mDecodeSubtitle) {
      // find or create sid service cSubtitleContext
      auto it = mSubtitleMap.find (pidInfo->mSid);
      if (it == mSubtitleMap.end()) {
        auto insertPair = mSubtitleMap.insert (map <int, cSubtitle*>::value_type (pidInfo->mSid, new cSubtitle()));
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

private:
  string mRootName;

  vector<string> mChannelStrings;
  vector<string> mSaveStrings;
  bool mRecordAll;
  bool mDecodeSubtitle;

  mutex mFileMutex;

  map <int, cSubtitle*> mSubtitleMap; // indexed by sid
  };
//}}}

// public:
//{{{
cTsDvb::cTsDvb (int frequency, const string& root,
                const vector <string>& channelNames, const vector <string>& recordNames,
                bool decodeSubtitle)
    : cDvb(frequency, 0), mDecodeSubtitle(decodeSubtitle) {

  mDvbTransportStream = new cDvbTransportStream (root, channelNames, recordNames, decodeSubtitle);
  }
//}}}
//{{{
cTsDvb::~cTsDvb() {
  delete mDvbTransportStream;
  }
//}}}

//{{{
cTransportStream* cTsDvb::getTransportStream() {
  return mDvbTransportStream;
  }
//}}}
//{{{
cSubtitle* cTsDvb::getSubtitleBySid (int sid) {
  return mDvbTransportStream->getSubtitleBySid (sid);
  }
//}}}

//{{{
void cTsDvb::readFile (bool ownThread, const string& fileName) {

  if (ownThread)
    thread ([=, this](){ readFileInternal (true, fileName); } ).detach();
  else
    readFileInternal (false, fileName);
  }
//}}}
//{{{
void cTsDvb::grab (bool ownThread, const string& root, const string& multiplexName) {

  if (ownThread)
    thread([=, this]() { grabInternal (true, root, multiplexName); }).detach();
  else
    grabInternal (false, root, multiplexName);
  }
//}}}

// private
//{{{
void cTsDvb::readFileInternal (bool ownThread, const string& fileName) {

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
      streamPos += mDvbTransportStream->demux ({}, buffer, bytesRead, streamPos, false);
    else
      break;
    mErrorStr = fmt::format ("{}", mDvbTransportStream->getNumErrors());
    }

  fclose (file);
  free (buffer);

  if (ownThread)
    cLog::log (LOGERROR, "exit");
  }
//}}}
//{{{
void cTsDvb::grabInternal (bool ownThread, const string& root, const string& multiplexName) {

  if (ownThread)
    cLog::setThreadName ("grab");

  string allName = root + "/all" + multiplexName + ".ts";
  FILE* mFile = root.empty() ? nullptr : fopen (allName.c_str(), "wb");

  #ifdef _WIN32
    auto hr = mMediaControl->Run();
    if (hr == S_OK) {
      int64_t streamPos = 0;
      auto blockSize = 0;
      while (true) {
        auto ptr = getBlockBDA (blockSize);
        if (blockSize) {
          //{{{  read and demux block
          if (mFile)
            fwrite (ptr, 1, blockSize, mFile);

          streamPos += mDvbTransportStream->demux ({}, ptr, blockSize, streamPos, false);
          releaseBlock (blockSize);

          mErrorStr.clear();
          if (mDvbTransportStream->getNumErrors())
            mErrorStr += fmt::format ("{}err", mDvbTransportStream->getNumErrors());
          if (streamPos < 1000000)
            mErrorStr = fmt::format ("{}k", streamPos / 1000);
          else
            mErrorStr = fmt::format ("{}m", streamPos / 1000000);
          }
          //}}}
        else
          this_thread::sleep_for (1ms);
        if (mScanningTuner) {
          //{{{  update mSignalStr
          long signal = 0;
          mScanningTuner->get_SignalStrength (&signal);
          mSignalStr = fmt::format ("signal {}", signal / 0x10000);
          }
          //}}}
        }
      }
    else
      cLog::log (LOGERROR, fmt::format ("run graph failed {}", hr));
  #endif

  #ifdef __linux__
    constexpr int kDvrReadBufferSize = 50 * 188;
    auto buffer = (uint8_t*)malloc (kDvrReadBufferSize);

    uint64_t streamPos = 0;
    while (true) {
      int bytesRead = read (mDvr, buffer, kDvrReadBufferSize);
      if (bytesRead > 0) {
        streamPos += mDvbTransportStream->demux ({}, buffer, bytesRead, 0, false);
        if (mFile)
          fwrite (buffer, 1, bytesRead, mFile);

        bool show = mDvbTransportStream->getNumErrors() != mLastErrors;
        mLastErrors = mDvbTransportStream->getNumErrors();

        mSignalStr = getStatusString();
        if (show) {
          mErrorStr = fmt::format ("err:{}", mDvbTransportStream->getNumErrors());
          cLog::log (LOGINFO, mErrorStr + " " + mSignalStr);
          }
        }
      else
        cLog::log (LOGINFO, "cDvb grabThread no bytes read");
      }
    free (buffer);
  #endif

  if (mFile)
    fclose (mFile);

  if (ownThread)
    cLog::log (LOGINFO, "exit");
  }
//}}}
