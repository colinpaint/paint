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
  } eDecErrCode;

sDecoder* openDecoder (sParam* param, uint8_t* chunk, size_t chunkSize);
int decodeOneFrame (sDecoder* decoder, sDecodedPic** decPicList);
void finishDecoder (sDecoder* decoder, sDecodedPic** decPicList);
void closeDecoder (sDecoder* decoder);
