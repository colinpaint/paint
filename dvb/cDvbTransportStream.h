// cDvbTransportStream.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

#include "cDvbMultiplex.h"
#include "cDvbSource.h"
#include "cTransportStream.h"

class cDvbSubtitle;
//}}}
using tDvbSubtitleMap = std::map <uint16_t, cDvbSubtitle>;

class cDvbTransportStream : public cTransportStream {
public:
  cDvbTransportStream (const cDvbMultiplex& dvbMultiplex, const std::string& recordRootName, bool decodeSubtitle);
  virtual ~cDvbTransportStream();

  std::vector <std::string>& getRecordPrograms() { return mRecordPrograms; }

  // subtitle
  bool getDecodeSubtitle() const { return mDecodeSubtitle; }
  bool hasSubtitle (uint16_t sid);
  cDvbSubtitle& getSubtitle (uint16_t sid);

  void toggleDecodeSubtitle();

  // sources
  std::string getErrorString() { return mErrorString; }
  std::string getSignalString() { return mSignalString; }
  void dvbSource (bool ownThread);
  void fileSource (bool ownThread, const std::string& fileName);

protected:
  virtual void startServiceProgram (cService* service, tTimePoint tdtTime,
                                    const std::string& programName, tTimePoint programTime, bool selected) final;
  virtual void programPesPacket (uint16_t sid, uint16_t pid, uint8_t* ts) final;
  virtual void stopServiceProgram (cService* service) final;

  virtual bool audDecodePes (cPidInfo* pidInfo, bool skip) final;
  virtual bool vidDecodePes (cPidInfo* pidInfo, bool skip) final;
  virtual bool subDecodePes (cPidInfo* pidInfo) final;

private:
  void dvbSourceInternal (bool ownThread);
  void fileSourceInternal (bool ownThread, const std::string& fileName);

  // vars
  cDvbSource* mDvbSource = nullptr;
  uint64_t mLastErrors = 0;
  std::string mErrorString;
  std::string mSignalString;

  cDvbMultiplex mDvbMultiplex;

  std::mutex mRecordFileMutex;
  std::string mRecordRootName;
  std::vector <std::string> mRecordPrograms;

  bool mDecodeSubtitle = false;
  tDvbSubtitleMap mDvbSubtitleMap; // indexed by sid
  };
