// cLog.cpp - simple logging
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
  #define NOMINMAX

  #include "windows.h"
#endif

#include <algorithm>
#include <string>
#include <mutex>
#include <deque>
#include <map>
#include <chrono>

#include "date.h"
#include "fmt/color.h"

#ifdef __linux__
  #include <stddef.h>
  #include <unistd.h>
  #include <cstdarg>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/syscall.h>
  #include <signal.h>
  #include <pthread.h>

  #define gettid() syscall(SYS_gettid)
#endif

#define remove_utf8   remove
#define rename_utf8   rename
#define fopen64_utf8  fopen
#define stat64_utf8   stat64

#include "cLog.h"

using namespace std;
//}}}

namespace {
  // anonymous namespace
  const int kMaxBuffer = 10000;
  enum eLogLevel gLogLevel = LOGERROR;
  const fmt::color kLevelColours[] = { fmt::color::orange,       // notice
                                       fmt::color::light_salmon, // error
                                       fmt::color::yellow,       // info
                                       fmt::color::green,        // info1
                                       fmt::color::lime_green,   // info2
                                       fmt::color::lavender};    // info3

  map <uint64_t, string> gThreadNameMap;
  deque <cLogLine> gLineDeque;
  mutex gLinesMutex;

  FILE* gFile = NULL;
  bool gBuffer = false;

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

  if (gFile) {
    fclose (gFile);
    gFile = NULL;
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

  gBuffer = buffer;

  gLogLevel = logLevel;
  if (gLogLevel > LOGNOTICE) {
    if (!logFilePath.empty() && !gFile) {
      string logFileString = logFilePath + "/log.txt";
      gFile = fopen (logFileString.c_str(), "wb");
      }
    }

  setThreadName ("main");

  return gFile != NULL;
  }
//}}}

enum eLogLevel cLog::getLogLevel() { return gLogLevel; }

//{{{
string cLog::getThreadName (uint64_t threadId) {

  auto it = gThreadNameMap.find (threadId);
  if (it != gThreadNameMap.end())
    return it->second;
  else
    return "....";
  }
//}}}
//{{{
bool cLog::getLine (cLogLine& line, unsigned lineNum, unsigned& lastLineIndex) {
// still a bit too dumb, holding onto lastLineIndex between searches helps

  lock_guard<mutex> lockGuard (gLinesMutex);

  unsigned matchingLineNum = 0;
  for (auto i = lastLineIndex; i < gLineDeque.size(); i++)
    if (gLineDeque[i].mLogLevel <= gLogLevel)
      if (lineNum == matchingLineNum++) {
        line = gLineDeque[i];
        return true;
        }

  return false;
  }
//}}}

//{{{
void cLog::cycleLogLevel() {
// cycle log level for L key presses in gui

  switch (gLogLevel) {
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
  if (gLogLevel != logLevel) {
    switch (logLevel) {
      case LOGNOTICE: cLog::log (LOGNOTICE, "setLogLevel to LOGNOTICE"); break;
      case LOGERROR:  cLog::log (LOGNOTICE, "setLogLevel to LOGERROR"); break;
      case LOGINFO:   cLog::log (LOGNOTICE, "setLogLevel to LOGINFO");  break;
      case LOGINFO1:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO1"); break;
      case LOGINFO2:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO2"); break;
      case LOGINFO3:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO3"); break;
      case eMaxLog: ;
      }
    gLogLevel = logLevel;
    }
  }
//}}}
//{{{
void cLog::setThreadName (const string& name) {

  auto it = gThreadNameMap.find (getThreadId());
  if (it == gThreadNameMap.end())
    gThreadNameMap.insert (map<uint64_t,string>::value_type (getThreadId(), name));

  log (LOGNOTICE, "start");
  }
//}}}

//{{{
void cLog::log (enum eLogLevel logLevel, const string& logStr) {

  if (!gBuffer && (logLevel > gLogLevel))
    return;

  lock_guard<mutex> lockGuard (gLinesMutex);

  chrono::time_point<chrono::system_clock> now = chrono::system_clock::now() + gDaylightSavingHours;

  if (gBuffer) {
    // buffer for widget display
    gLineDeque.push_front (cLogLine (logLevel, getThreadId(), now, logStr));
    if (gLineDeque.size() > kMaxBuffer)
      gLineDeque.pop_back();
    }

  else if (logLevel <= gLogLevel) {
    fmt::print (fg (fmt::color::floral_white) | fmt::emphasis::bold, "{} {} {}\n",
                date::format ("%T", chrono::floor<chrono::microseconds>(now)),
                fmt::format (fg (fmt::color::dark_gray), "{}", getThreadName (getThreadId())),
                fmt::format (fg (kLevelColours[logLevel]), "{}", logStr));

    if (gFile) {
      fputs (fmt::format ("{} {} {}\n",
                          date::format("%T", chrono::floor<chrono::microseconds>(now)),
                          getThreadName (getThreadId()),
                          logStr).c_str(), gFile);
      fflush (gFile);
      }
    }
  }
//}}}
//{{{
void cLog::log (enum eLogLevel logLevel, const char* format, ... ) {

  if (!gBuffer && (logLevel > gLogLevel)) // bomb out early without lock
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
