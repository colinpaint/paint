//{{{

/*!
 *************************************************************************************
 * \file annexb.c
 *
 * \brief
 *    Annex B Byte Stream format
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *************************************************************************************
 */
//}}}
//{{{
#define _CRT_SECURE_NO_WARNINGS

#include "global.h"
#include "annexb.h"
#include "memalloc.h"
//}}}
static const int kDebug = 0;

//{{{
ANNEXB_t* allocAnnexB (VideoParameters* p_Vid) {

  ANNEXB_t* annexB = (ANNEXB_t*)calloc (1, sizeof(ANNEXB_t));
  annexB->naluBuf = (byte*)malloc (p_Vid->nalu->max_size);
  return annexB;
  }
//}}}
//{{{
void freeAnnexB (ANNEXB_t** p_annexB) {

  free ((*p_annexB)->naluBuf);
  (*p_annexB)->naluBuf = NULL;

  free (*p_annexB);
  *p_annexB = NULL;
  }
//}}}

//{{{
void openAnnexB (ANNEXB_t* annexB, byte* chunk, size_t chunkSize) {

  annexB->buffer = chunk;
  annexB->bufferSize = chunkSize;

  annexB->bufferPtr = chunk;
  annexB->bytesInBuffer = chunkSize;
  }
//}}}
//{{{
void resetAnnexB (ANNEXB_t* annexB) {

  annexB->bytesInBuffer = annexB->bufferSize;
  annexB->bufferPtr = annexB->buffer;
  }
//}}}

//{{{
static inline byte getByte (ANNEXB_t* annexB) {

  if (annexB->bytesInBuffer) {
    annexB->bytesInBuffer--;
    return *annexB->bufferPtr++;
    }

  return 0;
  }
//}}}
//{{{
static inline int findStartCode (unsigned char* buf, int zerosInStartcode) {

  for (int i = 0; i < zerosInStartcode; i++)
    if (*(buf++) != 0)
      return 0;

  if (*buf != 1)
    return 0;

  return 1;
  }
//}}}
//{{{
int getNALU (ANNEXB_t* annexB, VideoParameters* p_Vid, NALU_t* nalu) {

  int naluBufCount = 0;
  byte* naluBufPtr = annexB->naluBuf;
  if (annexB->nextStartCodeBytes != 0) {
    for (int i = 0; i < annexB->nextStartCodeBytes - 1; i++) {
      *naluBufPtr++ = 0;
      naluBufCount++;
      }
    *naluBufPtr++ = 1;
    naluBufCount++;
    }
  else
    while (annexB->bytesInBuffer) {
      naluBufCount++;
      if ((*(naluBufPtr++) = getByte (annexB)) != 0)
        break;
      }

  if (!annexB->bytesInBuffer) {
    if (naluBufCount == 0)
      return 0;
    else {
      //{{{  error, return
      printf ("get_annexB_NALU can't read start code\n");
      return -1;
      }
      //}}}
    }

  if (*(naluBufPtr - 1) != 1 || naluBufCount < 3) {
    //{{{  error, retuirn
    printf ("get_annexB_NALU: no Start Code at the beginning of the NALU, return -1\n");
    return -1;
    }
    //}}}

  int leadingZero8BitsCount = 0;
  if (naluBufCount == 3)
    nalu->startcodeprefix_len = 3;
  else {
    leadingZero8BitsCount = naluBufCount - 4;
    nalu->startcodeprefix_len = 4;
    }
  //{{{  only 1st byte stream NAL unit can have leading_zero_8bits
  if (!annexB->isFirstByteStreamNALU && leadingZero8BitsCount > 0) {
    printf ("get_annexB_NALU: leading_zero_8bits syntax only present first byte stream NAL unit\n");
    return -1;
    }
  //}}}

  int info2 = 0;
  int info3 = 0;
  leadingZero8BitsCount = naluBufCount;
  annexB->isFirstByteStreamNALU = 0;
  int startCodeFound = 0;
  while (!startCodeFound) {
    if (!annexB->bytesInBuffer) {
      //{{{  eof, last NALU
      naluBufPtr -= 2;
      while (*(naluBufPtr--)==0)
        naluBufCount--;

      nalu->len = (naluBufCount - 1) - leadingZero8BitsCount;
      memcpy (nalu->buf, annexB->naluBuf + leadingZero8BitsCount, nalu->len);
      nalu->forbidden_bit = (*(nalu->buf) >> 7) & 1;
      nalu->nal_reference_idc = (NalRefIdc)((*(nalu->buf) >> 5) & 3);
      nalu->nal_unit_type = (NaluType)((*(nalu->buf)) & 0x1f);
      annexB->nextStartCodeBytes = 0;

      if (kDebug)
        printf ("last %sNALU %d::%d:%d len:%d, \n",
                nalu->startcodeprefix_len == 4 ? "l":"s",
                nalu->forbidden_bit,
                nalu->nal_reference_idc,
                nalu->nal_unit_type,
                nalu->len
                );

      return (naluBufCount - 1);
      }
      //}}}
    naluBufCount++;
    *(naluBufPtr++) = getByte (annexB);
    info3 = findStartCode (naluBufPtr - 4, 3);
    if (info3 != 1) {
      info2 = findStartCode (naluBufPtr - 3, 2);
      startCodeFound = info2 & 0x01;
      }
    else
      startCodeFound = 1;
    }

  // found next startCode, long(4 bytes) or short(3bytes)
  if (info3 == 1) {
    naluBufPtr -= 5;
    while (*(naluBufPtr--) == 0)
      naluBufCount--;
    annexB->nextStartCodeBytes = 4;
    }
  else if (info2 == 1)
    annexB->nextStartCodeBytes = 3;
  else {
    //{{{  error, return
    printf (" Panic: Error in next start code search \n");
    return -1;
    }
    //}}}

  // backup read ptr to trailing startCode
  naluBufCount -= annexB->nextStartCodeBytes;

  // leading zeros, Start code, complete NALU, trailing zeros(if any), next start code in buf
  // - size of Buf is naluBufCount - rewind,
  // - naluBufCount is the number of bytes excluding the next start code,
  // - naluBufCount - LeadingZero8BitsCount is the size of the NALU.
  nalu->len = naluBufCount - leadingZero8BitsCount;
  memcpy (nalu->buf, annexB->naluBuf + leadingZero8BitsCount, nalu->len);
  nalu->forbidden_bit = (*(nalu->buf) >> 7) & 1;
  nalu->nal_reference_idc = (NalRefIdc) ((*(nalu->buf) >> 5) & 3);
  nalu->nal_unit_type = (NaluType) ((*(nalu->buf)) & 0x1f);
  nalu->lost_packets = 0;

  if (kDebug)
    printf ("%sNALU %d::%d:%d len:%d, \n",
            nalu->startcodeprefix_len == 4 ? "l":"s",
            nalu->forbidden_bit,
            nalu->nal_reference_idc,
            nalu->nal_unit_type,
            nalu->len
            );

  return naluBufCount;
  }
//}}}
