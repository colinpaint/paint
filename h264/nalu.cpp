//{{{  includes
#include "global.h"
#include "memory.h"

#include "nalu.h"

#include "../common/cLog.h"

using namespace std;
//}}}
namespace {
  //{{{
  bool findStartCode (uint8_t* buf, uint32_t numZeros) {
  // look for start code with numZeros

    for (uint32_t i = 0; i < numZeros; i++)
      if (*buf++ != 0)
        return false;

    return *buf == 1;
    }
  //}}}
  }

// cAnnexB - nalu stream with startCodes
//{{{
void cAnnexB::open (uint8_t* chunk, size_t chunkSize) {

  buffer = chunk;
  bufferSize = chunkSize;

  reset();
  }
//}}}
//{{{
void cAnnexB::reset() {

  bufferPtr = buffer;
  bufferLeft = bufferSize;
  }
//}}}
//{{{
uint32_t cAnnexB::findNalu() {
// extract nalu from stream between startCodes or bufferEnd
// - !!! could be faster !!!

  // find leading startCode, or bufferEnd
  bool startCodeFound = false;
  while (!startCodeFound && (bufferLeft >= 4)) {
    if (findStartCode (bufferPtr, 2)) {
      startCodeFound = true;
      longStartCode = false;
      bufferPtr += 3;
      bufferLeft -= 3;
      }
    else if (findStartCode (bufferPtr, 3)) {
      startCodeFound = true;
      longStartCode = true;
      bufferPtr += 4;
      bufferLeft -= 4;
      }
    else {
      bufferPtr++;
      bufferLeft--;
      }
    }

  if (!startCodeFound) // bufferEnd
    return 0;

  // point start of nalu, return naluBytes to next startVCode or endBuffer
  naluBufferPtr = bufferPtr;
  uint32_t naluBytes = 0;
  while (bufferLeft > 0) {
    if ((bufferLeft >= 3) && findStartCode (bufferPtr, 2)) // 0x000001 startCode, return length
      return naluBytes;
    else if ((bufferLeft >= 4) && findStartCode (bufferPtr, 3)) // 0x00000001 startCode, return length
      return naluBytes;
    else {
      bufferPtr++;
      bufferLeft--;
      naluBytes++;
      }
    }

  // no trailing startCode, bufferEnd, return length
  return naluBytes;
  }
//}}}


// cNalu - nalu extracted from stream
//{{{
cNalu::cNalu (uint32_t size) {

  buffer = (uint8_t*)malloc (size);
  allocBufferSize = size;
  }
//}}}
//{{{
cNalu::~cNalu() {

  free (buffer);
  }
//}}}

//{{{
string cNalu::getNaluString() const {

  string naluTypeString;
  switch (unitType) {
    case NALU_TYPE_IDR:
      naluTypeString = "IDR";
      break;
    case NALU_TYPE_SLICE:
      naluTypeString = "SLC";
      break;
    case NALU_TYPE_SPS:
      naluTypeString = "SPS";
      break;
    case NALU_TYPE_PPS:
      naluTypeString = "PPS";
      break;
    case NALU_TYPE_SEI:
      naluTypeString = "SEI";
      break;
    case NALU_TYPE_DPA:
      naluTypeString = "DPA";
      break;
    case NALU_TYPE_DPB:
      naluTypeString = "DPB";
      break;
    case NALU_TYPE_DPC:
      naluTypeString = "DPC";
      break;
    case NALU_TYPE_AUD:
      naluTypeString = "AUD";
      break;
    case NALU_TYPE_FILL:
      naluTypeString = "FIL";
      break;
    default:
      break;
    }

  return fmt::format ("{} {}:{}:{}:{}:{}",
                      naluTypeString,
                      longStartCode ? 'l' : 's',
                      forbiddenBit,
                      (int)refId,
                      (int)unitType,
                      naluBytes);
  }
//}}}

//{{{
uint32_t cNalu::readNalu (cDecoder264* decoder) {
// simple parse of nalu, copy to own buffer before emulation preventation byte removal

  naluBytes = decoder->annexB->findNalu();
  if (naluBytes == 0)
    return naluBytes;

  // copy annexB data our buffer
  memcpy (buffer, decoder->annexB->getNaluBuffer(), naluBytes);

  longStartCode = decoder->annexB->getLongStartCode();
  forbiddenBit = (*buffer >> 7) & 1;
  refId = eNalRefId((*buffer >> 5) & 3);
  unitType = eNaluType(*buffer & 0x1f);

  if (decoder->param.naluDebug)
    debug();

  if (forbiddenBit)
    cDecoder264::error ("NALU with forbiddenBit set");

  checkZeroByteNonVCL (decoder);

  // remove emulation preventation bytes
  naluToRbsp();
  if (naluBytes < 0)
    cDecoder264::error ("NALU invalid startcode");

  return (uint32_t)naluBytes;
  }
//}}}


//{{{
int cNalu::rbspToSodb (uint8_t* bitStreamBuffer) {
// RawByteSequencePayload to StringOfDataBits

  int lastBytePos = naluBytes - 1;

  // find trailing 1
  int bitOffset = 0;
  bool controlBit = bitStreamBuffer[lastBytePos-1] & (0x01 << bitOffset);
  while (!controlBit) {
    // find trailing 1 bit
    ++bitOffset;
    if (bitOffset == 8) {
      if (!lastBytePos)
        cLog::log (LOGERROR, "RBSPtoSODB allZero data in RBSP");
      --lastBytePos;
      bitOffset = 0;
      }
    controlBit = bitStreamBuffer[lastBytePos - 1] & (0x01 << bitOffset);
    }

  return lastBytePos;
  }
//}}}

// private
//{{{
void cNalu::checkZeroByteVCL (cDecoder264* decoder) {

  // This function deals only with VCL NAL units
  if (!(unitType >= NALU_TYPE_SLICE && unitType <= NALU_TYPE_IDR))
    return;

  // the first VCL NAL unit that is the first NAL unit after last VCL NAL unit indicates
  // the start of a new access unit and hence the first NAL unit of the new access unit.
  // (sounds like a tongue twister :-)
  decoder->gotLastNalu = 1;
  }
//}}}
//{{{
void cNalu::checkZeroByteNonVCL (cDecoder264* decoder) {

  // This function deals only with non-VCL NAL units
  if ((unitType >= 1) && (unitType <= 5))
    return;

  // check the possibility of the current NALU to be the start of a new access unit, according to 7.4.1.2.3
  if (unitType == NALU_TYPE_AUD || unitType == NALU_TYPE_SPS ||
      unitType == NALU_TYPE_PPS || unitType == NALU_TYPE_SEI ||
      (unitType >= 13 && unitType <= 18))
    if (decoder->gotLastNalu)
      // deliver the last access unit to decoder
      decoder->gotLastNalu = 0;
  }
//}}}

//{{{
int cNalu::naluToRbsp() {
// NetworkAbstractionLayerUnit to RawByteSequencePayload
// - remove emulation prevention byte sequences
// - what is cabac_zero_word problem ???

  uint8_t* bitStreamBuffer = buffer;
  int endBytePos = naluBytes;
  if (endBytePos < 1) {
    naluBytes = endBytePos;
    return naluBytes;
    }

  int count = 0;
  int j = 1;
  for (int i = 1; i < endBytePos; ++i) {
    // in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any uint8_t-aligned position
    if ((count == 2) && (bitStreamBuffer[i] < 0x03)) {
      naluBytes = -1;
      return naluBytes;
      }

    if ((count == 2) && (bitStreamBuffer[i] == 0x03)) {
      // check the 4th uint8_t after 0x000003, except when cabac_zero_word is used
      // - in which case the last three bytes of this NAL unit must be 0x000003
      if ((i < endBytePos-1) && (bitStreamBuffer[i+1] > 0x03)) {
        naluBytes = -1;
        return naluBytes;
        }

      // if cabac_zero_word, final uint8_t of NALunit(0x03) is discarded, last two bytes RBSP must be 0x0000
      if (i == endBytePos-1) {
        naluBytes = j;
        return naluBytes;
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

  naluBytes = j;
  return naluBytes;
  }
//}}}

//{{{
void cNalu::debug() {
  cLog::log (LOGINFO, getNaluString());
  }
//}}}
