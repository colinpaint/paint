// cPlayerApp.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
#endif

#include "cPlayerApp.h"

#include <sys/stat.h>

//{{{  include stb
// invoke header only library implementation here
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

  #define STB_IMAGE_IMPLEMENTATION
  #include <stb_image.h>

  #define STB_IMAGE_WRITE_IMPLEMENTATION
  #include <stb_image_write.h>

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}

// utils
#include "../date/include/date/date.h"
#include "../common/utils.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"

// app
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"

using namespace std;
//}}}

//{{{
void cPlayerApp::fileSource (const string& filename, cPlayerOptions* options) {

  // create transportStream
  mTransportStream = new cTransportStream ({"file", 0, {}, {}}, options);
  if (!mTransportStream) {
    //{{{  error, return
    cLog::log (LOGERROR, "fileSource cTransportStream create failed");
    return;
    }
    //}}}

  // open fileSource
  mOptions->mFileName = cFileUtils::resolve (filename);
  FILE* file = fopen (mOptions->mFileName.c_str(), "rb");
  if (!file) {
    //{{{  error, return
    cLog::log (LOGERROR, fmt::format ("fileSource failed to open {}", mOptions->mFileName));
    return;
    }
    //}}}
  mFileSource = true;

  // create fileRead thread
  thread ([=]() {
    cLog::setThreadName ("file");

    mFilePos = 0;
    mStreamPos = 0;
    size_t chunkSize = 188 * 256;
    uint8_t* chunk = new uint8_t[chunkSize];
    while (true) {
      while (mTransportStream->throttle())
        this_thread::sleep_for (1ms);

      // seek and read chunk from file
      bool skip = mStreamPos != mFilePos;
      if (skip) {
        //{{{  seek to mStreamPos
        if (fseek (file, (long)mStreamPos, SEEK_SET))
          cLog::log (LOGERROR, fmt::format ("seek failed {}", mStreamPos));
        else {
          cLog::log (LOGINFO, fmt::format ("seek {}", mStreamPos));
          mFilePos = mStreamPos;
          }
        }
        //}}}
      size_t bytesRead = fread (chunk, 1, chunkSize, file);
      mFilePos = mFilePos + bytesRead;
      if (bytesRead > 0)
        mStreamPos += mTransportStream->demux (chunk, bytesRead, mStreamPos, skip);
      else
        break;

      //{{{  update fileSize
      #ifdef _WIN32
        struct _stati64 st;
        if (_stat64 (mOptions->mFileName.c_str(), &st) != -1)
          mFileSize = st.st_size;
      #else
        struct stat st;
        if (stat (mOptions->mFileName.c_str(), &st) != -1)
          mFileSize = st.st_size;
      #endif
      //}}}
      }

    fclose (file);
    delete[] chunk;

    cLog::log (LOGERROR, "exit");
    }).detach();
  }
//}}}
//{{{
void cPlayerApp::skip (int64_t skipPts) {

  int64_t offset = mTransportStream->getSkipOffset (skipPts);
  cLog::log (LOGINFO, fmt::format ("skip:{} offset:{} pos:{}", skipPts, offset, mStreamPos));
  mStreamPos += offset;
  }
//}}}

//{{{
void cPlayerApp::drop (const vector<string>& dropItems) {
// drop fileSource

  for (auto& item : dropItems) {
    cLog::log (LOGINFO, fmt::format ("cPlayerApp::drop {}", item));
    fileSource (item, mOptions);
    }
  }
//}}}
