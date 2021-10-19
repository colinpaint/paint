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

class cDvbTransportStream : public cTransportStream {
public:
  cDvbTransportStream (int frequency,
          const std::vector <std::string>& channelNames, const std::vector <std::string>& recordFileNames,
          const std::string& recordRoot, const std::string& recordAllRoot,
          bool recordAll, bool decodeSubtitle);
  virtual ~cDvbTransportStream();

  cSubtitle* getSubtitleBySid (uint16_t sid);

  void readFile (bool ownThread, const std::string& fileName);
  void grab (bool ownThread, const std::string& multiplexName);

protected:
  virtual void start (cService* service, const std::string& name,
                      std::chrono::system_clock::time_point time, std::chrono::system_clock::time_point starttime,
                      bool selected) final;
  virtual void pesPacket (uint16_t sid, uint16_t pid, uint8_t* ts) final;
  virtual void stop (cService* service) final;

  virtual bool audDecodePes (cPidInfo* pidInfo, bool skip) final;
  virtual bool vidDecodePes (cPidInfo* pidInfo, bool skip) final;
  virtual bool subDecodePes (cPidInfo* pidInfo) final;

private:
  void readFileInternal (bool ownThread, const std::string& fileName);
  void grabInternal (bool ownThread, const std::string& multiplexName);

  // vars
  std::mutex mFileMutex;

  std::vector <std::string> mChannelNames;
  std::vector <std::string> mRecordFileNames;
  std::string mRecordRoot;
  std::string mRecordAllRoot;
  bool mRecordAll;
  bool mDecodeSubtitle;

  uint64_t mLastErrors = 0;
  std::string mErrorString;
  std::string mSignalString;
  cDvbSource* mDvbSource = nullptr;

  std::map <uint16_t, cSubtitle*> mSubtitleMap; // indexed by sid
  };
