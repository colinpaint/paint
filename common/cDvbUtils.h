// cDvbUtils.h
#pragma once
//{{{  includes
#include <cstdlib>
#include <cstdint>
#include <string>
//}}}

class cDvbUtils {
public:
  static int getSectionLength (uint8_t* buf) { return ((buf[0] & 0x0F) << 8) + buf[1] + 3; }

  static uint32_t getCrc32 (uint8_t* buf, uint32_t len);
  static uint32_t getEpochTime (uint8_t* buf);
  static uint32_t getBcdTime (uint8_t* buf);
  static int64_t getPts (const uint8_t* buf);

  static std::string getStreamTypeName (uint16_t streamType);
  static char getFrameType (uint8_t* pes, int64_t pesSize, bool h264);

  static std::string getDvbString (uint8_t* buf);
  };
