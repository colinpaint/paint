//{{{  includes
#include "global.h"
#include "memory.h"

#include "nalu.h"
//}}}

//{{{
static void dumpNalu (sNalu* nalu) {

  switch (nalu->unitType) {
    case NALU_TYPE_IDR:
      printf ("IDR");
      break;
    case NALU_TYPE_SLICE:
      printf ("SLC");
      break;
    case NALU_TYPE_SPS:
      printf ("SPS");
      break;
    case NALU_TYPE_PPS:
      printf ("PPS");
      break;
    case NALU_TYPE_SEI:
      printf ("SEI");
      break;
    case NALU_TYPE_DPA:
      printf ("DPA");
      break;
    case NALU_TYPE_DPB:
      printf ("DPB");
      break;
    case NALU_TYPE_DPC:
      printf ("DPC");
      break;
    case NALU_TYPE_AUD:
      printf ("AUD");
      break;
    case NALU_TYPE_FILL:
      printf ("FIL");
      break;
    default:
      break;
    }

  printf (" %c:%d:%d:%d:%d\n",
          nalu->startCodeLen == 4 ? 'l':'s', nalu->forbiddenBit, nalu->refId, nalu->unitType, nalu->len);

  }
//}}}

//{{{
sAnnexB* allocAnnexB (sDecoder* decoder) {

  sAnnexB* annexB = (sAnnexB*)calloc (1, sizeof(sAnnexB));
  annexB->naluBuffer = (uint8_t*)malloc (decoder->nalu->maxSize);
  return annexB;
  }
//}}}
//{{{
void freeAnnexB (sAnnexB** annexB) {

  free ((*annexB)->naluBuffer);
  (*annexB)->naluBuffer = NULL;

  free (*annexB);
  *annexB = NULL;
  }
//}}}

//{{{
void openAnnexB (sAnnexB* annexB, uint8_t* chunk, size_t chunkSize) {

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
    noMemoryExit ("AllocNALU");

  nalu->buf = (uint8_t*)calloc (buffersize, sizeof (uint8_t));
  if (nalu->buf == NULL) {
    free (nalu);
    noMemoryExit ("AllocNALU buffer");
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
static inline uint8_t getByte (sAnnexB* annexB) {

  if (annexB->bytesInBuffer) {
    annexB->bytesInBuffer--;
    return *annexB->bufferPtr++;
    }

  return 0;
  }
//}}}
//{{{
static inline int findStartCode (uint8_t* buf, int zerosInStartcode) {

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
  uint8_t* naluBufPtr = annexB->naluBuffer;
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
  //{{{  only 1st uint8_t stream NAL unit can have leading_zero_8bits
  if (!annexB->isFirstByteStreamNALU && leadingZero8BitsCount > 0) {
    printf ("get_annexB_NALU: leading_zero_8bits syntax only present first uint8_t stream NAL unit\n");
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
        dumpNalu (nalu);

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

  // found next startCode, long(4 bytes) or int16_t(3bytes)
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
    dumpNalu (nalu);

  return naluBufCount;
  }
//}}}
//{{{
static int NALUtoRBSP (sNalu* nalu) {
// NetworkAbstractionLayerUnit to RawByteSequencePayload

  uint8_t* bitStreamBuffer = nalu->buf;
  int endBytePos = nalu->len;
  if (endBytePos < 1) {
    nalu->len = endBytePos;
    return nalu->len;
    }

  int count = 0;
  int j = 1;
  for (int i = 1; i < endBytePos; ++i) {
    // in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any uint8_t-aligned position
    if ((count == ZEROBYTES_SHORTSTARTCODE) && (bitStreamBuffer[i] < 0x03)) {
      nalu->len = -1;
      return nalu->len;
      }

    if ((count == ZEROBYTES_SHORTSTARTCODE) && (bitStreamBuffer[i] == 0x03)) {
      // check the 4th uint8_t after 0x000003,
      // except when cabac_zero_word is used
      // , in which case the last three bytes of this NAL unit must be 0x000003
      if ((i < endBytePos-1) && (bitStreamBuffer[i+1] > 0x03)) {
        nalu->len = -1;
        return nalu->len;
        }

      // if cabac_zero_word, final uint8_t of NALunit(0x03) is discarded
      // and the last two bytes of RBSP must be 0x0000
      if (i == endBytePos-1) {
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
int readNalu (sDecoder* decoder, sNalu* nalu) {

  int ret = getNALU (decoder->annexB, decoder, nalu);
  if (ret < 0)
    error ("error getting NALU");
  if (ret == 0)
    return 0;

  // In some cases, zero_byte shall be present.
  // If current NALU is a VCL NALU, we can't tell whether it is the first VCL NALU at this point,
  // so only non-VCL NAL unit is checked here.
  checkZeroByteNonVCL (decoder, nalu);
  ret = NALUtoRBSP (nalu);
  if (ret < 0)
    error ("NALU invalid startcode");

  // Got a NALU
  if (nalu->forbiddenBit)
    error ("NALU with forbiddenBit set");

  return nalu->len;
  }
//}}}

//{{{
int RBSPtoSODB (uint8_t* bitStreamBuffer, int lastBytePos) {
// RawByteSequencePayload to StringOfDataBits

  // find trailing 1
  int bitOffset = 0;
  int controlBit = (bitStreamBuffer[lastBytePos-1] & (0x01 << bitOffset));
  while (!controlBit) {
    // find trailing 1 bit
    ++bitOffset;
    if (bitOffset == 8) {
      if (!lastBytePos)
        printf ("--- RBSPtoSODB All zero data sequence in RBSP\n");
      --lastBytePos;
      bitOffset = 0;
      }
    controlBit = bitStreamBuffer[lastBytePos - 1] & (0x01 << bitOffset);
    }

  return lastBytePos;
  }
//}}}
