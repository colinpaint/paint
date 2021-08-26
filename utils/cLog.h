// cLog.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>

#include "core.h" //  fmt::format core, used by a lot of logging
//}}}

// no class or namespace qualification, reduces code clutter - cLog::log (LOG* - bad enough
enum eLogLevel { LOGTITLE, LOGNOTICE, LOGERROR, LOGINFO, LOGINFO1, LOGINFO2, LOGINFO3 };

class cLine;

class cLog {
public:
  ~cLog();

  static bool init (eLogLevel logLevel = LOGINFO,
                    bool buffer = false,
                    const std::string& logFilePath = "");

  static enum eLogLevel getLogLevel();

  static void cycleLogLevel();
  static void setLogLevel (eLogLevel logLevel);
  static void setThreadName (const std::string& name);

  static void log (eLogLevel logLevel, const std::string& logStr);
  static void log (eLogLevel logLevel, const char* format, ... );

  static void clearScreen();
  static void status (int row, int colourIndex, const std::string& statusString);

  static bool getLine (cLine& line, unsigned lineNum, unsigned& lastLineIndex);

private:
  static std::string getThreadNameString (uint64_t threadId);
  };
