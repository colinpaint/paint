#pragma once
#include "typedefs.h"

//{{{
static inline void fast_memset(void *dst,int value,int width)
{
  memset(dst,value,width);
}
//}}}
//{{{
static inline void fast_memcpy(void *dst,void *src,int width)
{
  memcpy(dst,src,width);
}
//}}}
//{{{
static inline void fast_memset_zero(void *dst, int width)
{
  memset(dst,0,width);
}
//}}}
