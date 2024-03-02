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

static const int IOBUFFERSIZE = 0x10000;  // 64k
//{{{
static inline size_t getChunk (ANNEXB_t* annex_b) {

  size_t readbytes = read (annex_b->BitStreamFile, annex_b->iobuffer, annex_b->iIOBufferSize);
  if (0 == readbytes) {
    annex_b->is_eof = TRUE;
    return 0;
    }

  annex_b->bytesinbuffer = readbytes;
  annex_b->iobufferread = annex_b->iobuffer;
  return readbytes;
  }
//}}}
//{{{
static inline byte getfbyte (ANNEXB_t* annex_b) {

  if (!annex_b->bytesinbuffer)
    if (!getChunk (annex_b))
      return 0;

  annex_b->bytesinbuffer--;
  return (*annex_b->iobufferread++);
  }
//}}}
//{{{
static inline int findStartCode (unsigned char* Buf, int zeros_in_startcode) {

  for (int i = 0; i < zeros_in_startcode; i++)
    if (*(Buf++) != 0)
      return 0;

  if (*Buf != 1)
    return 0;

  return 1;
  }
//}}}

//{{{
void malloc_annex_b (VideoParameters* p_Vid, ANNEXB_t** p_annex_b) {

  if (((*p_annex_b) = (ANNEXB_t*)calloc(1, sizeof(ANNEXB_t))) == NULL) {
    snprintf (errortext, ET_SIZE, "Memory allocation for Annex_B file failed");
    error (errortext,100);
    }

  if (((*p_annex_b)->Buf = (byte*)malloc(p_Vid->nalu->max_size)) == NULL)
    error ("malloc_annex_b: Buf", 101);
  }
//}}}
//{{{
void open_annex_b (char* fn, ANNEXB_t* annex_b) {

  if (NULL != annex_b->iobuffer)
    error ("open_annex_b: tried to open Annex B file twice",500);

  if ((annex_b->BitStreamFile = open (fn, OPENFLAGS_READ)) == -1) {
    snprintf (errortext, ET_SIZE, "Cannot open Annex B ByteStream file '%s'", fn);
    error (errortext,500);
    }

  annex_b->iIOBufferSize = IOBUFFERSIZE * sizeof (byte);
  annex_b->iobuffer = malloc (annex_b->iIOBufferSize);
  if (NULL == annex_b->iobuffer)
    error ("open_annex_b: cannot allocate IO buffer",500);

  annex_b->is_eof = FALSE;
  getChunk (annex_b);
  }
//}}}
//{{{
void init_annex_b (ANNEXB_t* annex_b) {

  annex_b->BitStreamFile = -1;
  annex_b->iobuffer = NULL;
  annex_b->iobufferread = NULL;
  annex_b->bytesinbuffer = 0;
  annex_b->is_eof = FALSE;
  annex_b->IsFirstByteStreamNALU = 1;
  annex_b->nextStartCodeBytes = 0;
  }
//}}}
//{{{
void reset_annex_b (ANNEXB_t* annex_b) {

  annex_b->is_eof = FALSE;
  annex_b->bytesinbuffer = 0;
  annex_b->iobufferread = annex_b->iobuffer;
  }
//}}}
//{{{
void free_annex_b (ANNEXB_t** p_annex_b) {

  free ((*p_annex_b)->Buf);
  (*p_annex_b)->Buf = NULL;

  free (*p_annex_b);
  *p_annex_b = NULL;
  }
//}}}
//{{{
void close_annex_b (ANNEXB_t* annex_b) {

  if (annex_b->BitStreamFile != -1) {
    close (annex_b->BitStreamFile);
    annex_b->BitStreamFile = - 1;
    }

  free (annex_b->iobuffer);
  annex_b->iobuffer = NULL;
  }
//}}}

//{{{
int get_annex_b_NALU (VideoParameters* p_Vid, NALU_t* nalu, ANNEXB_t* annex_b) {

  int pos = 0;
  byte* pBuf = annex_b->Buf;
  if (annex_b->nextStartCodeBytes != 0) {
    for (int i = 0; i < annex_b->nextStartCodeBytes-1; i++) {
      (*pBuf++) = 0;
      pos++;
      }
    (*pBuf++) = 1;
    pos++;
    }
  else {
    while (!annex_b->is_eof) {
      pos++;
      if ((*(pBuf++) = getfbyte(annex_b)) != 0)
        break;
      }
    }

  if (annex_b->is_eof == TRUE) {
    if (pos == 0)
      return 0;
    else {
      //{{{  error, return
      printf ("get_annex_b_NALU can't read start code\n");
      return -1;
      }
      //}}}
    }

  if (*(pBuf - 1) != 1 || pos < 3) {
    //{{{  error, retuirn
    printf ("get_annex_b_NALU: no Start Code at the beginning of the NALU, return -1\n");
    return -1;
    }
    //}}}

  int leadingZero8BitsCount = 0;
  if (pos == 3)
    nalu->startcodeprefix_len = 3;
  else {
    leadingZero8BitsCount = pos - 4;
    nalu->startcodeprefix_len = 4;
    }
  //{{{  only 1st byte stream NAL unit can have leading_zero_8bits
  if (!annex_b->IsFirstByteStreamNALU && leadingZero8BitsCount > 0) {
    printf ("get_annex_b_NALU: leading_zero_8bits syntax only present first byte stream NAL unit\n");
    return -1;
    }
  //}}}

  int info2 = 0;
  int info3 = 0;
  leadingZero8BitsCount = pos;
  annex_b->IsFirstByteStreamNALU = 0;
  int startCodeFound = 0;
  while (!startCodeFound) {
    if (annex_b->is_eof == TRUE) {
      //{{{  eof
      pBuf -= 2;
      while(*(pBuf--)==0)
        pos--;

      nalu->len = (pos - 1) - leadingZero8BitsCount;
      memcpy (nalu->buf, annex_b->Buf + leadingZero8BitsCount, nalu->len);

      nalu->forbidden_bit  = (*(nalu->buf) >> 7) & 1;
      nalu->nal_reference_idc = (NalRefIdc) ((*(nalu->buf) >> 5) & 3);
      nalu->nal_unit_type = (NaluType) ((*(nalu->buf)) & 0x1f);
      annex_b->nextStartCodeBytes = 0;

      if (kDebug)
        printf ("last %sNALU %d::%d:%d len:%d, \n",
                nalu->startcodeprefix_len == 4 ? "l":"s",
                nalu->forbidden_bit,
                nalu->nal_reference_idc,
                nalu->nal_unit_type,
                nalu->len
                );
      return (pos - 1);
      }
      //}}}
    pos++;
    *(pBuf++)  = getfbyte (annex_b);
    info3 = findStartCode (pBuf - 4, 3);
    if (info3 != 1) {
      info2 = findStartCode (pBuf - 3, 2);
      startCodeFound = info2 & 0x01;
      }
    else
      startCodeFound = 1;
    }

  // found another start code (and read length of startcode bytes more than we should have, go back in file
  if (info3 == 1) {
    // if the detected start code is 00 00 01, trailing_zero_8bits is sure not to be present
    pBuf -= 5;
    while (*(pBuf--) == 0)
      pos--;
    annex_b->nextStartCodeBytes = 4;
    }
  else if (info2 == 1)
    annex_b->nextStartCodeBytes = 3;
  else {
    //{{{  error, return
    printf (" Panic: Error in next start code search \n");
    return -1;
    }
    //}}}

  pos -= annex_b->nextStartCodeBytes;

  // leading zeros, Start code, complete NALU, trailing zeros(if any), next start code in buf
  // - size of Buf is pos - rewind,
  // - pos is the number of bytes excluding the next start code,
  // - pos - LeadingZero8BitsCount is the size of the NALU.
  nalu->len = pos - leadingZero8BitsCount;
  fast_memcpy (nalu->buf, annex_b->Buf + leadingZero8BitsCount, nalu->len);

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

  return (pos);
  }
//}}}
