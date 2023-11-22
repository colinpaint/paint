// cLog.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <chrono>

#include "../fmt/include/fmt/format.h"
//}}}

// no class or namespace qualification, reduces code clutter - cLog::log (LOG* - bad enough
enum eLogLevel { LOGNOTICE, LOGERROR, LOGINFO, LOGINFO1, LOGINFO2, LOGINFO3, eMaxLog };

//{{{
struct cLogLine {
  cLogLine() {}
  cLogLine (eLogLevel logLevel, uint64_t threadId,
         std::chrono::time_point<std::chrono::system_clock> timePoint, const std::string& lineString)
    : mLogLevel(logLevel), mThreadId(threadId), mTimePoint(timePoint), mString(lineString) {}

  eLogLevel mLogLevel = LOGINFO;
  uint64_t mThreadId = 0;
  std::chrono::time_point<std::chrono::system_clock> mTimePoint;
  std::string mString;
  };
//}}}

class cLog {
public:
  cLog() = default;
  ~cLog();

  static bool init (eLogLevel logLevel = LOGINFO, bool buffer = false, const std::string& logFilePath = "");

  // get
  static enum eLogLevel getLogLevel();
  static std::string getThreadName (uint64_t threadId);
  static bool getLine (cLogLine& line, unsigned lineNum, unsigned& lastLineIndex);

  // set
  static void cycleLogLevel();
  static void setLogLevel (eLogLevel logLevel);
  static void setThreadName (const std::string& name);

  // log
  static void log (eLogLevel logLevel, const std::string& logStr);
  static void log (eLogLevel logLevel, const char* format, ... );

  static void clearScreen();
  static void status (int row, int colourIndex, const std::string& statusString);
  };
