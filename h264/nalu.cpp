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
  refId = eNaluRefId((*buffer >> 5) & 3);
  unitType = eNaluType(*buffer & 0x1f);

  if (decoder->param.naluDebug)
    debug();

  if (forbiddenBit)
    cDecoder264::error ("NALU with forbiddenBit set");

  // remove emulation preventation bytes
  checkZeroByteNonVCL (decoder);
  naluToRbsp();

  if (naluBytes < 0)
    cDecoder264::error ("NALU invalid startcode");

  return (uint32_t)naluBytes;
  }
//}}}
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
uint32_t cNalu::getSodb (uint8_t* bitStreamBuffer) {

  // does this need to be a copy ???
  memcpy (bitStreamBuffer, buffer+1, naluBytes-1);

  return rbspToSodb (bitStreamBuffer);
  }
//}}}

// private
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
// networkAbstractionLayerUnit to rawByteSequencePayload
// - remove emulation prevention byte sequences
// - what is cabacZeroWord problem ???

uint8_t* bufferPtr = buffer;
  int endBytePos = naluBytes;
  if (endBytePos < 1) {
    naluBytes = endBytePos;
    return naluBytes;
    }

  int zeroCount = 0;
  int toIndex = 1;
  for (int fromIndex = 1; fromIndex < endBytePos; fromIndex++) {
    // in nalu, 0x000000, 0x000001, 0x000002 shall not occur at any uint8_t-aligned position
    if (zeroCount == 2) {
      if (bufferPtr[fromIndex] < 0x03) {
        naluBytes = -1;
        return naluBytes;
        }

      if (bufferPtr[fromIndex] == 0x03) {
        // check the 4th uint8_t after 0x000003
        // - except if cabacZeroWord, last 3 bytes of nalu are 0x000003
        // if cabacZeroWord, final byte of nalu(0x03) is discarded, last 2 bytes RBSP must be 0x0000
        if ((fromIndex < endBytePos-1) && (bufferPtr[fromIndex+1] > 0x03)) {
          naluBytes = -1;
          return naluBytes;
          }
        if (fromIndex == endBytePos-1) {
          naluBytes = toIndex;
          return naluBytes;
          }
        fromIndex++;
        zeroCount = 0;
        }
      }

    bufferPtr[toIndex] = bufferPtr[fromIndex];
    if (bufferPtr[fromIndex] == 0x00)
      zeroCount++;
    else
      zeroCount = 0;
    toIndex++;
    }

  naluBytes = toIndex;
  return naluBytes;
  }
//}}}
//{{{
int cNalu::rbspToSodb (uint8_t* bitStreamBuffer) {
// rawByteSequencePayload to stringOfDataBits
// - find trailing 1 bit

  int bitOffset = 0;
  int lastBytePos = naluBytes - 1;

  bool controlBit = bitStreamBuffer[lastBytePos-1] & (0x01 << bitOffset);
  while (!controlBit) {
    // find trailing 1 bit
    bitOffset++;
    if (bitOffset == 8) {
      if (!lastBytePos)
        cLog::log (LOGERROR, "rbspToSodb allZero data in rbsp");
      --lastBytePos;
      bitOffset = 0;
      }
    controlBit = bitStreamBuffer[lastBytePos - 1] & (0x01 << bitOffset);
    }

  return lastBytePos;
  }
//}}}

//{{{
void cNalu::debug() {
  cLog::log (LOGINFO, getNaluString());
  }
//}}}
