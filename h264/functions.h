#pragma once
#include <math.h>
#include <limits.h>

//{{{  enum eMBModeType
enum eMBModeType {
  PSKIP        =  0,
  BSKIP_DIRECT =  0,
  P16x16       =  1,
  P16x8        =  2,
  P8x16        =  3,
  SMB8x8       =  4,
  SMB8x4       =  5,
  SMB4x8       =  6,
  SMB4x4       =  7,
  P8x8         =  8,
  I4MB         =  9,
  I16MB        = 10,
  IBLOCK       = 11,
  SI4MB        = 12,
  I8MB         = 13,
  IPCM         = 14,
  MAXMODE      = 15
  };
//}}}

static inline int16_t smin (int16_t a, int16_t b) { return (int16_t) (((a) < (b)) ? (a) : (b)); }
static inline int16_t smax (int16_t a, int16_t b) { return (int16_t) (((a) > (b)) ? (a) : (b)); }
static inline int imin (int a, int b) { return ((a) < (b)) ? (a) : (b); }
static inline long lmin( long a, long b) { return ((a) < (b)) ? (a) : (b); }
static inline int imin3 (int a, int b, int c) { return ((a) < (b)) ? imin(a, c) : imin(b, c); }
static inline int imax (int a, int b) { return ((a) > (b)) ? (a) : (b); }
static inline long lmax (long a, long b) { return ((a) > (b)) ? (a) : (b); }

//{{{
static inline int imedian (int a,int b,int c)
{
  if (a > b) {
    // a > b
    if (b > c)
      return(b); // a > b > c
    else if (a > c)
      return(c); // a > c > b
    else
      return(a); // c > a > b
    }
  else {
    // b > a
    if (a > c)
      return(a); // b > a > c
    else if (b > c)
      return(c); // b > c > a
    else
      return(b);  // c > b > a
    }
  }
//}}}
//{{{
static inline int imedian_old (int a, int b, int c) {
  return (a + b + c - imin(a, imin(b, c)) - imax(a, imax(b ,c)));
  }
//}}}

static inline double dmin (double a, double b) { return ((a) < (b)) ? (a) : (b); }
static inline double dmax (double a, double b) { return ((a) > (b)) ? (a) : (b); }
static inline int64_t i64min (int64_t a, int64_t b) { return ((a) < (b)) ? (a) : (b); }
static inline int64_t i64max (int64_t a, int64_t b) { return ((a) > (b)) ? (a) : (b); }
static inline int32_t distblkmin (int32_t a, int32_t b) { return ((a) < (b)) ? (a) : (b); }
static inline int32_t distblkmax (int32_t a, int32_t b) { return ((a) > (b)) ? (a) : (b); }

//{{{
static inline int16_t sabs (int16_t x) {

  static const int16_t SHORT_BITS = (sizeof(int16_t) * CHAR_BIT) - 1;
  int16_t y = (int16_t) (x >> SHORT_BITS);
  return (int16_t) ((x ^ y) - y);
  }
//}}}
//{{{
static inline int iabs (int x) {

  static const int INT_BITS = (sizeof(int) * CHAR_BIT) - 1;
  int y = x >> INT_BITS;
  return (x ^ y) - y;
  }
//}}}
//{{{
static inline int64_t i64abs (int64_t x) {

  static const int64_t INT64_BITS = (sizeof(int64_t) * CHAR_BIT) - 1;
  int64_t y = x >> INT64_BITS;
  return (x ^ y) - y;
  }
//}}}
static inline double dabs (double x) { return ((x) < 0) ? -(x) : (x); }
static inline int iabs2 (int x) { return x * x; }
static inline int64_t i64abs2 (int64_t x) { return x * x; }
static inline double dabs2 (double x) { return x * x; }

static inline int isign (int x) { return ( (x > 0) - (x < 0)); }
static inline int isignab (int a, int b) { return ((b) < 0) ? -iabs(a) : iabs(a); }

//{{{
static inline int rshift_rnd(int x, int a) {
  return (a > 0) ? ((x + (1 << (a-1) )) >> a) : (x << (-a));
  }
//}}}
//{{{
static inline uint32_t rshift_rnd_ul (uint32_t x, int a) {
  return (a > 0) ? ((x + (1 << (a-1) )) >> a) : (x << (-a));
}
//}}}
//{{{
static inline int rshift_rnd_sign (int x, int a) {
  return (x > 0) ? ( ( x + (1 << (a-1)) ) >> a ) : (-( ( iabs(x) + (1 << (a-1)) ) >> a ));
  }
//}}}
//{{{
static inline uint32_t rshift_rnd_us (uint32_t x, uint32_t a) {
  return (a > 0) ? ((x + (1 << (a-1))) >> a) : x;
  }
//}}}
//{{{
static inline int rshift_rnd_sf (int x, int a) {
  return ((x + (1 << (a-1) )) >> a);
  }
//}}}
static inline int shift_off_sf (int x, int o, int a) { return ((x + o) >> a); }
static inline uint32_t rshift_rnd_us_sf (uint32_t x, uint32_t a) { return ((x + (1 << (a-1))) >> a); }

//{{{
static inline int iClip1 (int high, int x) {

  x = imax (x, 0);
  x = imin (x, high);
  return x;
  }
//}}}
//{{{
static inline long lClip1 (long high, long x) {

  x = lmax (x, 0);
  x = lmin (x, high);
  return x;
  }
//}}}
//{{{
static inline int iClip3 (int low, int high, int x) {

  x = imax (x, low);
  x = imin (x, high);
  return x;
  }
//}}}
//{{{
static inline long lClip3 (long low, long high, long x) {

  x = lmax (x, low);
  x = lmin (x, high);
  return x;
  }
//}}}

//{{{
static inline int RSD (int x)
{
 return ((x&2)?(x|1):(x&(~1)));
}
//}}}
//{{{
static inline int power2 (int x)
{
  return 1 << (x);
}
//}}}
//{{{
static const int64_t po2[64] =
  {0x1,0x2,0x4,0x8,
   0x10,0x20,0x40,0x80,
   0x100,0x200,0x400,0x800,
   0x1000,0x2000,0x4000,0x8000,
   0x10000,0x20000,0x40000,0x80000,
   0x100000,0x200000,0x400000,0x800000,
   0x1000000,0x2000000,0x4000000,0x8000000,
   0x10000000,0x20000000,0x40000000,0x80000000,
   0x100000000,0x200000000,0x400000000,0x800000000,
   0x1000000000,0x2000000000,0x4000000000,0x8000000000,
   0x10000000000,0x20000000000,0x40000000000,0x80000000000,
   0x100000000000,0x200000000000,0x400000000000,0x800000000000,
   0x1000000000000,0x2000000000000,0x4000000000000,0x8000000000000,
   0x10000000000000,0x20000000000000,0x40000000000000,0x80000000000000,
   0x100000000000000,0x200000000000000,0x400000000000000,0x800000000000000,
   0x1000000000000000,0x2000000000000000,0x4000000000000000,(int64_t)0x8000000000000000};
//}}}
//{{{
static inline int64_t i64power2 (int x) {
  return ((x > 63) ? 0 : po2[x]);
  }
//}}}
//{{{
static inline int getBit (int64_t x, int n) {
  return (int)(((x >> n) & 1));
  }
//}}}
//{{{
static inline int is_intra_mb (int16_t mbType) {
  return (mbType==SI4MB || mbType==I4MB || mbType==I16MB || mbType==I8MB || mbType==IPCM);
  }
//}}}

//{{{
static inline uint32_t ceilLog2 (uint32_t value) {

  uint32_t result = 0;

  uint32_t temp = value - 1;
  while (!temp) {
    temp >>= 1;
    result++;
    }

  return result;
  }
//}}}
//{{{
static inline uint32_t ceilLog2sf (uint32_t value) {

  uint32_t result = 0;

  uint32_t temp = value - 1;
  while (temp > 0) {
    temp >>= 1;
    result++;
    }

  return result;
  }
//}}}
