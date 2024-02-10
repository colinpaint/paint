// cTellyApp.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
#endif

#include "cTellyApp.h"

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
void cTellyApp::liveDvbSource (const cDvbMultiplex& multiplex, cTellyOptions* options) {

  cLog::log (LOGINFO, fmt::format ("using multiplex {} {:4.1f} record {} {}",
                                   multiplex.mName, multiplex.mFrequency/1000.f,
                                   options->mRecordRoot, options->mShowAllServices ? "all " : ""));

  mFileSource = false;
  mMultiplex = multiplex;
  mRecordRoot = options->mRecordRoot;
  if (multiplex.mFrequency)
    mDvbSource = new cDvbSource (multiplex.mFrequency, 0);

  mTransportStream = new cTransportStream (multiplex, options);
  if (mTransportStream) {
    mLiveThread = thread ([=]() {
      cLog::setThreadName ("dvb ");

      FILE* mFile = options->mRecordAll ?
        fopen ((mRecordRoot + mMultiplex.mName + ".ts").c_str(), "wb") : nullptr;

      #ifdef _WIN32
        //{{{  windows
        // raise thread priority
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

        mDvbSource->run();

        int64_t streamPos = 0;
        int blockSize = 0;

        while (true) {
          auto ptr = mDvbSource->getBlockBDA (blockSize);
          if (blockSize) {
            //  read and demux block
            streamPos += mTransportStream->demux (ptr, blockSize, streamPos, false);
            if (options->mRecordAll && mFile)
              fwrite (ptr, 1, blockSize, mFile);
            mDvbSource->releaseBlock (blockSize);
            }
          else
            this_thread::sleep_for (1ms);
          }
        //}}}
      #else
        //{{{  linux
        // raise thread priority
        sched_param sch_params;
        sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
        pthread_setschedparam (mLiveThread.native_handle(), SCHED_RR, &sch_params);

        constexpr int kDvrReadChunkSize = 188 * 64;
        uint8_t* chunk = new uint8_t[kDvrReadChunkSize];

        uint64_t streamPos = 0;
        while (true) {
          int bytesRead = mDvbSource->getBlock (chunk, kDvrReadChunkSize);
          if (bytesRead) {
            streamPos += mTransportStream->demux (chunk, bytesRead, 0, false);
            if (options->mRecordAll && mFile)
              fwrite (chunk, 1, bytesRead, mFile);
            }
          else
            cLog::log (LOGINFO, fmt::format ("liveDvbSource - no bytes"));
          }

        delete[] chunk;
        //}}}
      #endif

      if (mFile)
        fclose (mFile);

      cLog::log (LOGINFO, "exit");
      });

    mLiveThread.detach();
    }

  else
    cLog::log (LOGINFO, "cTellyApp::setLiveDvbSource - failed to create liveDvbSource");
  }
//}}}
//{{{
void cTellyApp::fileSource (const string& filename, cTellyOptions* options) {

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

    int64_t mStreamPos = 0;
    int64_t mFilePos = 0;
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
      }

    fclose (file);
    delete[] chunk;

    cLog::log (LOGERROR, "exit");
    }).detach();
  }
//}}}
