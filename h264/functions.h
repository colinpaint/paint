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

static inline short smin (short a, short b) { return (short) (((a) < (b)) ? (a) : (b)); }
static inline short smax (short a, short b) { return (short) (((a) > (b)) ? (a) : (b)); }
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
static inline int64 i64min (int64 a, int64 b) { return ((a) < (b)) ? (a) : (b); }
static inline int64 i64max (int64 a, int64 b) { return ((a) > (b)) ? (a) : (b); }
static inline distblk distblkmin (distblk a, distblk b) { return ((a) < (b)) ? (a) : (b); }
static inline distblk distblkmax (distblk a, distblk b) { return ((a) > (b)) ? (a) : (b); }

//{{{
static inline short sabs (short x) {

  static const short SHORT_BITS = (sizeof(short) * CHAR_BIT) - 1;
  short y = (short) (x >> SHORT_BITS);
  return (short) ((x ^ y) - y);
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
static inline int64 i64abs (int64 x) {

  static const int64 INT64_BITS = (sizeof(int64) * CHAR_BIT) - 1;
  int64 y = x >> INT64_BITS;
  return (x ^ y) - y;
  }
//}}}
static inline double dabs (double x) { return ((x) < 0) ? -(x) : (x); }
static inline int iabs2 (int x) { return x * x; }
static inline int64 i64abs2 (int64 x) { return x * x; }
static inline double dabs2 (double x) { return x * x; }

static inline int isign (int x) { return ( (x > 0) - (x < 0)); }
static inline int isignab (int a, int b) { return ((b) < 0) ? -iabs(a) : iabs(a); }

//{{{
static inline int rshift_rnd(int x, int a) {
  return (a > 0) ? ((x + (1 << (a-1) )) >> a) : (x << (-a));
  }
//}}}
//{{{
static inline unsigned long rshift_rnd_ul (unsigned long x, int a) {
  return (a > 0) ? ((x + (1 << (a-1) )) >> a) : (x << (-a));
}
//}}}
//{{{
static inline int rshift_rnd_sign (int x, int a) {

  return (x > 0) ? ( ( x + (1 << (a-1)) ) >> a ) : (-( ( iabs(x) + (1 << (a-1)) ) >> a ));
  }
//}}}
//{{{
static inline unsigned int rshift_rnd_us (unsigned int x, unsigned int a) {
  return (a > 0) ? ((x + (1 << (a-1))) >> a) : x;
  }
//}}}
//{{{
static inline int rshift_rnd_sf (int x, int a) {
  return ((x + (1 << (a-1) )) >> a);
  }
//}}}
static inline int shift_off_sf (int x, int o, int a) { return ((x + o) >> a); }
static inline unsigned int rshift_rnd_us_sf (unsigned int x, unsigned int a) { return ((x + (1 << (a-1))) >> a); }

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
static const int64 po2[64] =
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
   0x1000000000000000,0x2000000000000000,0x4000000000000000,(int64)0x8000000000000000};
//}}}
//{{{
static inline int64 i64power2 (int x) {
  return ((x > 63) ? 0 : po2[x]);
  }
//}}}
//{{{
static inline int getBit (int64 x, int n) {
  return (int)(((x >> n) & 1));
  }
//}}}
//{{{
static inline int is_intra_mb (short mbType) {
  return (mbType==SI4MB || mbType==I4MB || mbType==I16MB || mbType==I8MB || mbType==IPCM);
  }
//}}}

//{{{
static inline unsigned ceilLog2 (unsigned value) {

  unsigned result = 0;

  unsigned temp = value - 1;
  while (!temp) {
    temp >>= 1;
    result++;
    }

  return result;
  }
//}}}
//{{{
static inline unsigned ceilLog2sf (unsigned value) {

  unsigned result = 0;

  unsigned temp = value - 1;
  while (temp > 0) {
    temp >>= 1;
    result++;
    }

  return result;
  }
//}}}
