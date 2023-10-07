// cLog.cpp - simple logging
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
  #define NOMINMAX
#endif

#include "cLog.h"

#ifdef _WIN32
  #include "windows.h"
#endif

#ifdef __linux__
  #include <stdio.h>
  #include <stdlib.h>
  #include <stdint.h>
  #include <string.h>

  #include <stddef.h>
  #include <unistd.h>
  #include <cstdarg>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/syscall.h>
  #define gettid() syscall(SYS_gettid)

  #include <signal.h>
  #include <pthread.h>
#endif

#include <algorithm>
#include <string>
#include <mutex>
#include <deque>
#include <map>
#include <chrono>

#include "date.h"
#include "fmt/color.h"

#define remove_utf8   remove
#define rename_utf8   rename
#define fopen64_utf8  fopen
#define stat64_utf8   stat64

using namespace std;
//}}}

namespace {
  // anonymous namespace
  const int kMaxBuffer = 10000;
  enum eLogLevel mLogLevel = LOGERROR;
  const fmt::color kLevelColours[] = { fmt::color::orange,       // notice
                                       fmt::color::light_salmon, // error
                                       fmt::color::yellow,       // info
                                       fmt::color::green,        // info1
                                       fmt::color::lime_green,   // info2
                                       fmt::color::lavender};    // info3

  map <uint64_t, string> mThreadNameMap;
  deque <cLogLine> mLineDeque;
  mutex mLinesMutex;

  FILE* mFile = NULL;
  bool mBuffer = false;

  chrono::hours gDaylightSavingHours;

  #ifdef _WIN32
    HANDLE hStdOut = 0;
    uint64_t getThreadId() { return GetCurrentThreadId(); }
  #endif

  #ifdef __linux__
    uint64_t getThreadId() { return gettid(); }
  #endif
  }

//{{{
cLog::~cLog() {

  if (mFile) {
    fclose (mFile);
    mFile = NULL;
    }
  }
//}}}

//{{{
bool cLog::init (enum eLogLevel logLevel, bool buffer, const string& logFilePath) {

  // get daylightSaving hours
  time_t current_time;
  time (&current_time);
  struct tm* timeinfo = localtime (&current_time);
  gDaylightSavingHours = chrono::hours ((timeinfo->tm_isdst == 1) ? 1 : 0);

  #ifdef _WIN32
    hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
    DWORD consoleMode = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    SetConsoleMode (hStdOut, consoleMode);
  #endif

  mBuffer = buffer;

  mLogLevel = logLevel;
  if (mLogLevel > LOGNOTICE) {
    if (!logFilePath.empty() && !mFile) {
      string logFileString = logFilePath + "/log.txt";
      mFile = fopen (logFileString.c_str(), "wb");
      }
    }

  setThreadName ("main");

  return mFile != NULL;
  }
//}}}

//{{{
enum eLogLevel cLog::getLogLevel() {
  return mLogLevel;
  }
//}}}
//{{{
string cLog::getThreadName (uint64_t threadId) {

  auto it = mThreadNameMap.find (threadId);
  if (it != mThreadNameMap.end())
    return it->second;
  else
    return "....";
  }
//}}}
//{{{
bool cLog::getLine (cLogLine& line, unsigned lineNum, unsigned& lastLineIndex) {
// still a bit too dumb, holding onto lastLineIndex between searches helps

  lock_guard<mutex> lockGuard (mLinesMutex);

  unsigned matchingLineNum = 0;
  for (auto i = lastLineIndex; i < mLineDeque.size(); i++)
    if (mLineDeque[i].mLogLevel <= mLogLevel)
      if (lineNum == matchingLineNum++) {
        line = mLineDeque[i];
        return true;
        }

  return false;
  }
//}}}

//{{{
void cLog::cycleLogLevel() {
// cycle log level for L key presses in gui

  switch (mLogLevel) {
    case LOGNOTICE: setLogLevel(LOGERROR);   break;
    case LOGERROR:  setLogLevel(LOGINFO);    break;
    case LOGINFO:   setLogLevel(LOGINFO1);   break;
    case LOGINFO1:  setLogLevel(LOGINFO2);   break;
    case LOGINFO2:  setLogLevel(LOGINFO3);   break;
    case LOGINFO3:  setLogLevel(LOGERROR);   break;
    case eMaxLog: ;
    }
  }
//}}}
//{{{
void cLog::setLogLevel (enum eLogLevel logLevel) {
// limit to valid, change if different

  logLevel = max (LOGNOTICE, min (LOGINFO3, logLevel));
  if (mLogLevel != logLevel) {
    switch (logLevel) {
      case LOGNOTICE: cLog::log (LOGNOTICE, "setLogLevel to LOGNOTICE"); break;
      case LOGERROR:  cLog::log (LOGNOTICE, "setLogLevel to LOGERROR"); break;
      case LOGINFO:   cLog::log (LOGNOTICE, "setLogLevel to LOGINFO");  break;
      case LOGINFO1:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO1"); break;
      case LOGINFO2:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO2"); break;
      case LOGINFO3:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO3"); break;
      case eMaxLog: ;
      }
    mLogLevel = logLevel;
    }
  }
//}}}
//{{{
void cLog::setThreadName (const string& name) {

  auto it = mThreadNameMap.find (getThreadId());
  if (it == mThreadNameMap.end())
    mThreadNameMap.insert (map<uint64_t,string>::value_type (getThreadId(), name));

  log (LOGNOTICE, "start");
  }
//}}}

//{{{
void cLog::log (enum eLogLevel logLevel, const string& logStr) {

  if (!mBuffer && (logLevel > mLogLevel))
    return;

  lock_guard<mutex> lockGuard (mLinesMutex);

  chrono::time_point<chrono::system_clock> now = chrono::system_clock::now() + gDaylightSavingHours;

  if (mBuffer) {
    // buffer for widget display
    mLineDeque.push_front (cLogLine (logLevel, getThreadId(), now, logStr));
    if (mLineDeque.size() > kMaxBuffer)
      mLineDeque.pop_back();
    }

  else if (logLevel <= mLogLevel) {
    fmt::print (fg (fmt::color::floral_white) | fmt::emphasis::bold, "{} {} {}\n",
                date::format ("%T", chrono::floor<chrono::microseconds>(now)),
                fmt::format (fg (fmt::color::dark_gray), "{}", getThreadName (getThreadId())),
                fmt::format (fg (kLevelColours[logLevel]), "{}", logStr));

    if (mFile) {
      fputs (fmt::format ("{} {} {}\n",
                          date::format("%T", chrono::floor<chrono::microseconds>(now)),
                          getThreadName (getThreadId()),
                          logStr).c_str(), mFile);
      fflush (mFile);
      }
    }
  }
//}}}
//{{{
void cLog::log (enum eLogLevel logLevel, const char* format, ... ) {

  if (!mBuffer && (logLevel > mLogLevel)) // bomb out early without lock
    return;

  // form logStr
  va_list args;
  va_start (args, format);

  // get size of str
  size_t size = vsnprintf (nullptr, 0, format, args) + 1; // Extra space for '\0'

  // allocate buffer
  unique_ptr<char[]> buf (new char[size]);

  // form buffer
  vsnprintf (buf.get(), size, format, args);

  va_end (args);

  log (logLevel, string (buf.get(), buf.get() + size-1));
  }
//}}}

//{{{
void cLog::clearScreen() {

  // cursorPos, clear to end of screen
  string formatString (fmt::format ("\033[{};{}H\033[J\n", 1, 1));
  fputs (formatString.c_str(), stdout);
  fflush (stdout);
  }
//}}}
//{{{
void cLog::status (int row, int colourIndex, const string& statusString) {

  // send colour, pos row column 1, clear from cursor to end of line
  fmt::print (fg (kLevelColours[colourIndex]), "\033[{};{}H{}\033[K{}", row+1, 1, statusString);
  }
//}}}

