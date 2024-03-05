#pragma once
#include "global.h"

typedef enum {
  DEC_GEN_NOERR = 0,
  DEC_OPEN_NOERR = 0,
  DEC_CLOSE_NOERR = 0,
  DEC_SUCCEED = 0,
  DEC_EOS = 1,
  DEC_NEED_DATA = 2,
  DEC_INVALID_PARAM = 3,
  DEC_ERRMASK = 0x8000
  //  DEC_ERRMASK = 0x80000000
  } DecErrCode;

typedef struct dec_set_t {
  int iPostprocLevel; // valid interval are [0..100]
  int bDBEnable;
  int bAllLayers;
  int time_incr;
  int bDecCompAdapt;
  } DecSet_t;

//{{{
#ifdef __cplusplus
  extern "C" {
#endif
//}}}
  int SetOptsDecoder(DecSet_t *pDecOpts);
  int OpenDecoder (InputParameters* p_Inp, byte* chunk, size_t chunkSize);
  int DecodeOneFrame (sDecodedPicList** ppDecPic);
  int FinitDecoder (sDecodedPicList** ppDecPicList);
  int CloseDecoder();
//{{{
#ifdef __cplusplus
  }
#endif
//}}}