//{{{  cDvbMultiplex.h
// PAT inserts <pid,sid> into mProgramMap
// PMT declares pgm and elementary stream pids, adds cService into mServiceMap
// SDT names a service in mServiceMap
//}}}
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
  bool mRecordAll;
  std::vector <std::string> mChannels;
  std::vector <std::string> mChannelRecordNames;
  std::vector <int> mChannelNums;
  };
