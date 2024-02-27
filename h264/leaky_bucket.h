#pragma once
#include "global.h"

#ifdef _LEAKYBUCKET_
  // Leaky Bucket functions
  unsigned long GetBigDoubleWord(FILE *fp);
  void calc_buffer(InputParameters *p_Inp);
#endif
