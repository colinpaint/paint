// cDvbTransportStream.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

#include "cDvbSource.h"
#include "cTransportStream.h"

class cSubtitle;
//}}}

//{{{
class cDvbMultiplex {
public:
  std::string mName;
  int mFrequency;
  std::vector <std::string> mChannels;
  std::vector <std::string> mChannelRecordNames;
  bool mRecordAllChannels;
  };
//}}}

class cDvbTransportStream : public cTransportStream {
public:
  cDvbTransportStream (const cDvbMultiplex& dvbMultiplex, const std::string& recordRootName, bool subtitle);
  virtual ~cDvbTransportStream();

  cSubtitle* getSubtitleBySid (uint16_t sid);

  void dvbSource (bool ownThread);
  void fileSource (bool ownThread, const std::string& fileName);

protected:
  virtual void start (cService* service, const std::string& name,
                      std::chrono::system_clock::time_point time, 
                      std::chrono::system_clock::time_point starttime,
                      bool selected) final;
  virtual void pesPacket (uint16_t sid, uint16_t pid, uint8_t* ts) final;
  virtual void stop (cService* service) final;

  virtual bool audDecodePes (cPidInfo* pidInfo, bool skip) final;
  virtual bool vidDecodePes (cPidInfo* pidInfo, bool skip) final;
  virtual bool subDecodePes (cPidInfo* pidInfo) final;

private:
  void dvbSourceInternal (bool ownThread);
  void fileSourceInternal (bool ownThread, const std::string& fileName);

  // vars
  std::mutex mFileMutex;

  cDvbMultiplex mDvbMultiplex;
  std::string mRecordRootName;

  cDvbSource* mDvbSource = nullptr;
  uint64_t mLastErrors = 0;
  std::string mErrorString;
  std::string mSignalString;

  bool mSubtitle;
  std::map <uint16_t, cSubtitle*> mSubtitleMap; // indexed by sid
  };
