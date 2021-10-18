// cTsDvb.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "cDvb.h"

class cSubtitle;
class cTransportStream;
class cDvbTransportStream;
//}}}

class cTsDvb : public cDvb {
public:
  cTsDvb (int frequency, const std::string& root,
          const std::vector<std::string>& channelNames, const std::vector<std::string>& recordNames,
          bool decodeSubtitle);
  virtual ~cTsDvb();

  cTransportStream* getTransportStream();

  cSubtitle* getSubtitleBySid (int sid);

  void readFile (bool ownThread, const std::string& fileName);
  void grab (bool ownThread, const std::string& root, const std::string& multiplexName);

  // public for widget observe
  bool mDecodeSubtitle = false;

private:
  void readFileInternal (bool ownThread, const std::string& fileName);
  void grabInternal (bool ownThread, const std::string& root, const std::string& multiplexName);

  cDvbTransportStream* mDvbTransportStream;
  uint64_t mLastErrors = 0;
  };
