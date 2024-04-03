#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#if defined(WIN32) || defined (WIN64)
  //{{{  windows
  #include <io.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <windows.h>
  #include <crtdefs.h>

  #define strcasecmp _strcmpi
  #define strncasecmp _strnicmp

  #define snprintf _snprintf
  #define open     _open
  #define close    _close
  #define read     _read
  #define write    _write
  #define lseek    _lseeki64
  #define fsync    _commit
  #define tell     _telli64
  #define TIMEB    _timeb
  #define TIME_T    LARGE_INTEGER
  #define OPENFLAGS_WRITE _O_WRONLY|_O_CREAT|_O_BINARY|_O_TRUNC
  #define OPEN_PERMISSIONS _S_IREAD | _S_IWRITE
  #define OPENFLAGS_READ  _O_RDONLY|_O_BINARY

  //#define  inline   _inline
  #define forceinline __forceinline

  #define FORMAT_OFF_T "I64d"

  #ifndef INT64_MIN
    #define INT64_MIN (-9223372036854775807i64 - 1i64)
  #endif
  //}}}
#else
  //{{{  linux
  #include <unistd.h>
  #include <sys/time.h>
  #include <sys/stat.h>
  #include <time.h>
  #include <stdint.h>

  #define TIMEB    timeb
  #define TIME_T   struct timeval
  #define tell(fd) lseek(fd, 0, SEEK_CUR)

  #define OPENFLAGS_WRITE O_WRONLY|O_CREAT|O_TRUNC
  #define OPENFLAGS_READ  O_RDONLY
  #define OPEN_PERMISSIONS S_IRUSR | S_IWUSR

  //#if __STDC_VERSION__ >= 199901L
     /* "inline" is a keyword */
  //#else
  //  #define inline /* nothing */
  //#endif

  //#define forceinline inline

  typedef long long int64_t;
  typedef uint32_t long long  uint64;
  #define FORMAT_OFF_T "lld"

  #ifndef INT64_MIN
    #define INT64_MIN (-9223372036854775807LL - 1LL)
  #endif
  //}}}
#endif

extern void getTime (TIME_T* time);
extern void initTime();
extern int64_t timeDiff (TIME_T* start, TIME_T* end);
extern int64_t timeNorm (int64_t cur_time);
