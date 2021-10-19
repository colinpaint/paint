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

class cSubtitle;
//}}}

class cDvbTransportStream : public cTransportStream {
public:
  cDvbTransportStream (const cDvbMultiplex& dvbMultiplex, const std::string& recordRootName, bool subtitle);
  virtual ~cDvbTransportStream();

  std::string getErrorString() { return mErrorString; }
  std::string getSignalString() { return mSignalString; }
  cSubtitle* getSubtitleBySid (uint16_t sid);
  std::vector <std::string>& getRecordItems() { return mRecordItems; }

  void setSubtitle (bool subtitle);

  void dvbSource (bool ownThread);
  void fileSource (bool ownThread, const std::string& fileName);

protected:
  virtual void startServiceItem (cService* service, const std::string& itemName,
                                 std::chrono::system_clock::time_point time,
                                 std::chrono::system_clock::time_point itemStarttime,
                                 bool itemSelected) final;
  virtual void pesPacket (uint16_t sid, uint16_t pid, uint8_t* ts) final;
  virtual void stopServiceItem (cService* service) final;

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
  std::vector <std::string> mRecordItems;

  bool mSubtitle;
  std::map <uint16_t, cSubtitle*> mSubtitleMap; // indexed by sid
  };
