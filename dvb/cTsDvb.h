// cTsDvb.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "cDvbSource.h"

class cSubtitle;
class cTransportStream;
class cDvbTransportStream;
//}}}

class cTsDvb : public cDvbSource {
public:
  cTsDvb (int frequency,
          const std::vector<std::string>& channelNames,
          const std::vector<std::string>& recordNames,
          const std::string& recordRoot, 
          const std::string& recordAllRoot,
          bool decodeSubtitle);
  virtual ~cTsDvb();

  cTransportStream* getTransportStream();
  cSubtitle* getSubtitleBySid (uint16_t sid);

  std::string getRecordRoot() const;
  bool getRecordAll() const;
  bool getDecodeSubtitle() const;

  void readFile (bool ownThread, const std::string& fileName);
  void grab (bool ownThread, const std::string& multiplexName);

private:
  void readFileInternal (bool ownThread, const std::string& fileName);
  void grabInternal (bool ownThread, const std::string& multiplexName);

  // vars
  cDvbTransportStream* mDvbTransportStream;
  std::string mRecordAllRoot;
  uint64_t mLastErrors = 0;
  };
