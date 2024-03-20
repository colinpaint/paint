// includes
#include "global.h"

#ifdef _WIN32
  static LARGE_INTEGER freq;

  void initTime() { QueryPerformanceFrequency (&freq); }
  void gettime (TIME_T* time) { QueryPerformanceCounter (time); }
  int64 timenorm (int64  cur_time) { return (int64)(cur_time * 1000 /(freq.QuadPart)); }
  int64 timediff (TIME_T* start, TIME_T* end) { return (int64)((end->QuadPart - start->QuadPart)); }

#else
  static struct timezone tz;

  void initTime() {}
  void gettime (TIME_T* time) { gettimeofday (time, &tz); }
  int64 timenorm (int64 cur_time) { return cur_time / 1000; }
  //{{{
  int64 timediff (TIME_T* start, TIME_T* end)  {

    int64 t1 =  end->tv_sec  - start->tv_sec;
    int64 t2 =  end->tv_usec - start->tv_usec;
    return t2 + t1 * 1000000;
    }
  //}}}
#endif
