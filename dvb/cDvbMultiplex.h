// cDvbMultiplex.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
//}}}

class cDvbMultiplex {
public:
  std::string mName;
  int mFrequency;
  std::vector <std::string> mChannels;
  std::vector <std::string> mChannelRecordNames;
  bool mRecordAllChannels;
  };
