//{{{  includes
#include "global.h"
#include "memory.h"

#include "nalu.h"
//}}}

//{{{
sAnnexB* allocAnnexB (sDecoder* decoder) {

  sAnnexB* annexB = (sAnnexB*)calloc (1, sizeof(sAnnexB));
  annexB->naluBuffer = (byte*)malloc (decoder->nalu->maxSize);
  return annexB;
  }
//}}}
//{{{
void freeAnnexB (sAnnexB** p_annexB) {

  free ((*p_annexB)->naluBuffer);
  (*p_annexB)->naluBuffer = NULL;

  free (*p_annexB);
  *p_annexB = NULL;
  }
//}}}

//{{{
void openAnnexB (sAnnexB* annexB, byte* chunk, size_t chunkSize) {

  annexB->buffer = chunk;
  annexB->bufferSize = chunkSize;

  annexB->bufferPtr = chunk;
  annexB->bytesInBuffer = chunkSize;
  }
//}}}
//{{{
void resetAnnexB (sAnnexB* annexB) {

  annexB->bytesInBuffer = annexB->bufferSize;
  annexB->bufferPtr = annexB->buffer;
  }
//}}}

//{{{
sNalu* allocNALU (int buffersize) {

  sNalu* nalu = (sNalu*)calloc (1, sizeof(sNalu));
  if (nalu == NULL)
    no_mem_exit ("AllocNALU");

  nalu->buf = (byte*)calloc (buffersize, sizeof (byte));
  if (nalu->buf == NULL) {
    free (nalu);
    no_mem_exit ("AllocNALU buffer");
    }
  nalu->maxSize = buffersize;

  return nalu;
  }
//}}}
//{{{
void freeNALU (sNalu* n) {

  if (n != NULL) {
    if (n->buf != NULL) {
      free (n->buf);
      n->buf = NULL;
      }
    free (n);
    }
  }
//}}}

//{{{
void checkZeroByteVCL (sDecoder* decoder, sNalu* nalu) {

  int CheckZeroByte = 0;

  // This function deals only with VCL NAL units
  if (!(nalu->unitType >= NALU_TYPE_SLICE &&
        nalu->unitType <= NALU_TYPE_IDR))
    return;

  if (decoder->gotLastNalu)
    decoder->naluCount = 0;
  decoder->naluCount++;

  // the first VCL NAL unit that is the first NAL unit after last VCL NAL unit indicates
  // the start of a new access unit and hence the first NAL unit of the new access unit.
  // (sounds like a tongue twister :-)
  if (decoder->naluCount == 1)
    CheckZeroByte = 1;
  decoder->gotLastNalu = 1;

  // because it is not a very serious problem, we do not exit here
  if (CheckZeroByte && nalu->startCodeLen == 3)
    printf ("warning: zero_byte shall exist\n");
   }
//}}}
//{{{
void checkZeroByteNonVCL (sDecoder* decoder, sNalu* nalu) {

  int CheckZeroByte = 0;

  // This function deals only with non-VCL NAL units
  if (nalu->unitType >= 1 &&
      nalu->unitType <= 5)
    return;

  // for SPS and PPS, zero_byte shall exist
  if (nalu->unitType == NALU_TYPE_SPS ||
      nalu->unitType == NALU_TYPE_PPS)
    CheckZeroByte = 1;

  // check the possibility of the current NALU to be the start of a new access unit, according to 7.4.1.2.3
  if (nalu->unitType == NALU_TYPE_AUD ||
      nalu->unitType == NALU_TYPE_SPS ||
      nalu->unitType == NALU_TYPE_PPS ||
      nalu->unitType == NALU_TYPE_SEI ||
      (nalu->unitType >= 13 && nalu->unitType <= 18)) {
    if (decoder->gotLastNalu) {
      // deliver the last access unit to decoder
      decoder->gotLastNalu = 0;
      decoder->naluCount = 0;
      }
    }
  decoder->naluCount++;

  // for the first NAL unit in an access unit, zero_byte shall exists
  if (decoder->naluCount == 1)
    CheckZeroByte = 1;

  if (CheckZeroByte && nalu->startCodeLen == 3)
    printf ("Warning: zero_byte should exist\n");
    //because it is not a very serious problem, we do not exit here
  }
//}}}

//{{{
static inline byte getByte (sAnnexB* annexB) {

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
static int getNALU (sAnnexB* annexB, sDecoder* decoder, sNalu* nalu) {

  int naluBufCount = 0;
  byte* naluBufPtr = annexB->naluBuffer;
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
    nalu->startCodeLen = 3;
  else {
    leadingZero8BitsCount = naluBufCount - 4;
    nalu->startCodeLen = 4;
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
      while (*(naluBufPtr--) == 0)
        naluBufCount--;

      nalu->len = (naluBufCount - 1) - leadingZero8BitsCount;
      memcpy (nalu->buf, annexB->naluBuffer + leadingZero8BitsCount, nalu->len);
      nalu->forbiddenBit = (*(nalu->buf) >> 7) & 1;
      nalu->refId = (eNalRefIdc)((*(nalu->buf) >> 5) & 3);
      nalu->unitType = (eNaluType)((*(nalu->buf)) & 0x1f);
      annexB->nextStartCodeBytes = 0;

      if (decoder->param.naluDebug)
        printf ("last %sNALU %d::%d:%d len:%d, \n",
                nalu->startCodeLen == 4 ? "l":"s",
                nalu->forbiddenBit, nalu->refId, nalu->unitType, nalu->len);

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
  memcpy (nalu->buf, annexB->naluBuffer + leadingZero8BitsCount, nalu->len);
  nalu->forbiddenBit = (*(nalu->buf) >> 7) & 1;
  nalu->refId = (eNalRefIdc)((*(nalu->buf) >> 5) & 3);
  nalu->unitType = (eNaluType)((*(nalu->buf)) & 0x1f);
  nalu->lostPackets = 0;

  if (decoder->param.naluDebug)
    printf ("%sNALU %d::%d:%d len:%d, \n",
            nalu->startCodeLen == 4 ? "l":"s",
            nalu->forbiddenBit,
            nalu->refId,
            nalu->unitType,
            nalu->len
            );

  return naluBufCount;
  }
//}}}
//{{{
static int NALUtoRBSP (sNalu* nalu) {
// networkAbstractionLayerUnit to rawByteSequencePayload

  byte* bitStreamBuffer = nalu->buf;
  int end_bytepos = nalu->len;
  if (end_bytepos < 1) {
    nalu->len = end_bytepos;
    return nalu->len;
    }

  int count = 0;
  int j = 1;
  for (int i = 1; i < end_bytepos; ++i) {
    // in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any byte-aligned position
    if (count == ZEROBYTES_SHORTSTARTCODE && bitStreamBuffer[i] < 0x03) {
      nalu->len = -1;
      return nalu->len;
      }

    if (count == ZEROBYTES_SHORTSTARTCODE && bitStreamBuffer[i] == 0x03) {
      // check the 4th byte after 0x000003,
      // except when cabac_zero_word is used
      // , in which case the last three bytes of this NAL unit must be 0x000003
      if ((i < end_bytepos-1) && (bitStreamBuffer[i+1] > 0x03)) {
        nalu->len = -1;
        return nalu->len;
        }

      // if cabac_zero_word, final byte of NALunit(0x03) is discarded
      // and the last two bytes of RBSP must be 0x0000
      if (i == end_bytepos-1) {
        nalu->len = j;
        return nalu->len;
        }

      ++i;
      count = 0;
      }

    bitStreamBuffer[j] = bitStreamBuffer[i];
    if (bitStreamBuffer[i] == 0x00)
      ++count;
    else
      count = 0;
    ++j;
    }

  nalu->len = j;
  return nalu->len;
  }
//}}}
//{{{
int readNextNalu (sDecoder* decoder, sNalu* nalu) {

  int ret = getNALU (decoder->annexB, decoder, nalu);
  if (ret < 0)
    error ("Error while getting the NALU in file format exit\n");
  if (ret == 0)
    return 0;

  // In some cases, zero_byte shall be present.
  // If current NALU is a VCL NALU, we can't tell whether it is the first VCL NALU at this point,
  // so only non-VCL NAL unit is checked here.
  checkZeroByteNonVCL (decoder, nalu);

  ret = NALUtoRBSP (nalu);
  if (ret < 0)
    error ("Invalid startcode emulation prevention");

  // Got a NALU
  if (nalu->forbiddenBit)
    error ("NALU with forbiddenBit set");

  return nalu->len;
  }
//}}}

//{{{
int RBSPtoSODB (byte* bitStreamBuffer, int last_byte_pos) {
// rawByteSequencePayload to stringOfDataBits

  // find trailing 1
  int bitOffset = 0;
  int ctr_bit = (bitStreamBuffer[last_byte_pos-1] & (0x01 << bitOffset));
  while (ctr_bit == 0) {
    // find trailing 1 bit
    ++bitOffset;
    if (bitOffset == 8) {
      if (last_byte_pos == 0)
        printf (" Panic: All zero data sequence in RBSP \n");
      assert (last_byte_pos != 0);
      --last_byte_pos;
      bitOffset = 0;
      }
    ctr_bit = bitStreamBuffer[last_byte_pos - 1] & (0x01 << bitOffset);
    }

  return last_byte_pos;
  }
//}}}
