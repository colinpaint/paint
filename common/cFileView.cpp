// cFileView.cpp - analyse base class
#ifdef _WIN32
//{{{  includes
#include "cFileView.h"

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <string>
#include <stdio.h>
#include <chrono>
#include <functional>

#include "fmt/core.h"
#include "../common/date.h"

using namespace std;
//}}}

namespace {
  //{{{
  chrono::system_clock::time_point getFileTimePoint (FILETIME fileTime) {

    // filetime_duration has the same layout as FILETIME; 100ns intervals
    using filetime_duration = chrono::duration<int64_t, ratio<1, 10'000'000>>;

    // January 1, 1601 (NT epoch) - January 1, 1970 (Unix epoch):
    constexpr chrono::duration<int64_t> nt_to_unix_epoch { INT64_C(-11644473600) };

    const filetime_duration asDuration{static_cast<int64_t> (
        (static_cast<uint64_t>((fileTime).dwHighDateTime) << 32) | (fileTime).dwLowDateTime)};

    const auto withUnixEpoch = asDuration + nt_to_unix_epoch;

    return chrono::system_clock::time_point { chrono::duration_cast<chrono::system_clock::duration>(withUnixEpoch) };
    }
  //}}}
  }

//{{{
cFileView::cFileView (const string& filename) : mFilename(filename) {

  mFileHandle = CreateFile (mFilename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  mMapHandle = CreateFileMapping (mFileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
  mFileBuffer = (uint8_t*)MapViewOfFile (mMapHandle, FILE_MAP_READ, 0, 0, 0);
  mFileSize = GetFileSize (mFileHandle, NULL);

  FILETIME creationTime;
  FILETIME accessTime;
  FILETIME writeTime;
  GetFileTime (mFileHandle, &creationTime, &accessTime, &writeTime);

  mCreationTimePoint = getFileTimePoint (creationTime);
  mAccessTimePoint = getFileTimePoint (accessTime);
  mWriteTimePoint = getFileTimePoint (writeTime);

  mCreationString = date::format ("create %H:%M:%S %a %d %b %y", chrono::floor<chrono::seconds>(mCreationTimePoint));
  mAccessString = date::format ("access %H:%M:%S %a %d %b %y", chrono::floor<chrono::seconds>(mAccessTimePoint));
  mWriteString = date::format ("write  %H:%M:%S %a %d %b %y", chrono::floor<chrono::seconds>(mWriteTimePoint));

  resetRead();
  }
//}}}
//{{{
cFileView::~cFileView() {

  UnmapViewOfFile (mFileBuffer);
  CloseHandle (mMapHandle);
  CloseHandle (mFileHandle);
  }
//}}}

//{{{
bool cFileView::readLine (string& line, uint32_t& lineNumber, uint8_t*& ptr, uint32_t& address, uint32_t& numBytes) {
// return false if no more lines, else true with beginPtr,endPtr of line terminated by carraige return

  address = getReadAddress();
  uint8_t* beginPtr = readBytes (1);
  uint8_t* endPtr = beginPtr;

  while (endPtr) {
    if (*endPtr == 0x0d) { // carraige return, end of line
      line = string (beginPtr, endPtr);
      lineNumber = mReadLineNumber++;
      ptr = beginPtr;
      numBytes = (uint32_t)(endPtr - beginPtr) + 1; // !!! want to include lf !!!
      return true;
      }
    else if (*endPtr == 0x0a) { // skip line feed
      beginPtr = readBytes (1);
      endPtr = beginPtr;
      }
    else // next char
      endPtr = readBytes (1);
    }

  line = "eof";
  lineNumber = mReadLineNumber;
  ptr = nullptr;
  address = 0;
  numBytes = 0;
  return false;
  }
//}}}
#endif
