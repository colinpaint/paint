//{{{  includes
#include "global.h"
#include "memory.h"

#include "nalu.h"

#include "../common/cLog.h"

using namespace std;
//}}}
constexpr uint32_t kBufferInitSize = 32; // bytes for one frame
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
  //{{{
  uint32_t rbspToSodb (uint8_t* buffer, uint32_t len) {
  // rawByteSequencePayload to stringOfDataBits
  // - find trailing 1 bit and return length

    uint32_t bitOffset = 0;
    uint32_t lastBytePos = len;

    bool controlBit = buffer[lastBytePos-1] & (0x01 << bitOffset);
    while (!controlBit) {
      // find trailing 1 bit
      bitOffset++;
      if (bitOffset == 8) {
        if (!lastBytePos)
          cLog::log (LOGERROR, "rbspToSodb allZero data in rbsp");
        --lastBytePos;
        bitOffset = 0;
        }
      controlBit = buffer[lastBytePos - 1] & (0x01 << bitOffset);
      }

    return lastBytePos;
    }
  //}}}

  //{{{
  void allocBuffer (uint8_t*& buffer, uint32_t& allocSize, uint32_t size) {

    if (size > allocSize) {
      // simple buffer grow algorithm
      if (allocSize < kBufferInitSize)
        allocSize = kBufferInitSize;
      while (size > allocSize)
        allocSize *= 2;
      cLog::log (LOGINFO, fmt::format ("cNalu buffer size changed to {} for {}", allocSize, size));
      buffer = (uint8_t*)realloc (buffer, allocSize);
      }
    }
  //}}}
  }

// cAnnexB - stream of startCode,nalu ...
//{{{
void cAnnexB::open (uint8_t* chunk, size_t chunkSize) {

  mBuffer = chunk;
  mBufferSize = chunkSize;

  reset();
  }
//}}}
//{{{
void cAnnexB::reset() {

  mBufferPtr = mBuffer;
  mBufferLeft = mBufferSize;
  }
//}}}
//{{{
uint32_t cAnnexB::findNalu() {
// extract nalu from stream between startCodes or bufferEnd
// - !!! could be faster !!!

  // find leading startCode, or bufferEnd
  bool startCodeFound = false;
  while (!startCodeFound && (mBufferLeft >= 4)) {
    if (findStartCode (mBufferPtr, 2)) {
      startCodeFound = true;
      mLongStartCode = false;
      mBufferPtr += 3;
      mBufferLeft -= 3;
      }
    else if (findStartCode (mBufferPtr, 3)) {
      startCodeFound = true;
      mLongStartCode = true;
      mBufferPtr += 4;
      mBufferLeft -= 4;
      }
    else {
      mBufferPtr++;
      mBufferLeft--;
      }
    }

  if (!startCodeFound) // bufferEnd
    return 0;

  // point start of nalu, return naluBytes to next startVCode or endBuffer
  mNaluBufferPtr = mBufferPtr;
  uint32_t naluBytes = 0;
  while (mBufferLeft > 0) {
    if ((mBufferLeft >= 3) && findStartCode (mBufferPtr, 2)) // 0x000001 startCode, return length
      return naluBytes;
    else if ((mBufferLeft >= 4) && findStartCode (mBufferPtr, 3)) // 0x00000001 startCode, return length
      return naluBytes;
    else {
      mBufferPtr++;
      mBufferLeft--;
      naluBytes++;
      }
    }

  // no trailing startCode, bufferEnd, return length
  return naluBytes;
  }
//}}}

// cNalu - nalu extracted from annexB stream
//{{{
cNalu::cNalu() {
  }
//}}}
//{{{
cNalu::~cNalu() {

  free (mBuffer);
  }
//}}}

//{{{
string cNalu::getNaluString() const {

  string naluTypeString;
  switch (mUnitType) {
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

  return fmt::format ("{}{}{} {}:{}:{}",
                      naluTypeString,
                      mLongStartCode ? "" : "short",
                      mForbiddenBit ?  "forbidden":"",
                      (int)mRefId,
                      (int)mUnitType,
                      mNaluBytes);
  }
//}}}

//{{{
uint32_t cNalu::readNalu (cDecoder264* decoder) {
// simple parse of nalu, copy to own buffer before emulation preventation byte removal

  mNaluBytes = decoder->annexB->findNalu();
  if (mNaluBytes == 0)
    return mNaluBytes;

  // copy annexB data to our mBuffer
  allocBuffer (mBuffer, mAllocSize, mNaluBytes);
  memcpy (mBuffer, decoder->annexB->getNaluBuffer(), mNaluBytes);

  mLongStartCode = decoder->annexB->getLongStartCode();
  mForbiddenBit = (*mBuffer >> 7) & 1;
  mRefId = eNaluRefId((*mBuffer >> 5) & 3);
  mUnitType = eNaluType(*mBuffer & 0x1f);

  if (decoder->param.naluDebug)
    debug();

  if (mForbiddenBit)
    cDecoder264::error ("NALU with forbiddenBit set");

  // remove emulation preventation bytes
  checkZeroByteNonVCL (decoder);
  naluToRbsp();

  if (mNaluBytes < 0)
    cDecoder264::error ("NALU invalid startcode");

  return (uint32_t)mNaluBytes;
  }
//}}}
//{{{
void cNalu::checkZeroByteVCL (cDecoder264* decoder) {

  // This function deals only with VCL NAL units
  if (!(mUnitType >= NALU_TYPE_SLICE && mUnitType <= NALU_TYPE_IDR))
    return;

  // the first VCL NAL unit that is the first NAL unit after last VCL NAL unit indicates
  // the start of a new access unit and hence the first NAL unit of the new access unit.
  // (sounds like a tongue twister :-)
  decoder->gotLastNalu = 1;
  }
//}}}
//{{{
uint32_t cNalu::getSodb (uint8_t*& buffer, uint32_t& allocSize) {

  // copy naluBuffer to buffer
  allocBuffer (buffer, allocSize, mNaluBytes-1);
  memcpy (buffer, mBuffer+1, mNaluBytes-1);
  return rbspToSodb (buffer, mNaluBytes-1);
  }
//}}}

// private
//{{{
void cNalu::checkZeroByteNonVCL (cDecoder264* decoder) {

  // This function deals only with non-VCL NAL units
  if ((mUnitType >= 1) && (mUnitType <= 5))
    return;

  // check the possibility of the current NALU to be the start of a new access unit, according to 7.4.1.2.3
  if (mUnitType == NALU_TYPE_AUD || mUnitType == NALU_TYPE_SPS ||
      mUnitType == NALU_TYPE_PPS || mUnitType == NALU_TYPE_SEI ||
      (mUnitType >= 13 && mUnitType <= 18))
    if (decoder->gotLastNalu)
      // deliver the last access unit to decoder
      decoder->gotLastNalu = 0;
  }
//}}}
//{{{
int cNalu::naluToRbsp() {
// networkAbstractionLayerUnit to rawByteSequencePayload
// - remove emulation prevention byte sequences
// - what is the cabacZeroWord problem ???

  uint8_t* bufferPtr = mBuffer;
  int endBytePos = mNaluBytes;
  if (endBytePos < 1) {
    mNaluBytes = endBytePos;
    return mNaluBytes;
    }

  int zeroCount = 0;
  int toIndex = 1;
  for (int fromIndex = 1; fromIndex < endBytePos; fromIndex++) {
    // in nalu, 0x000000, 0x000001, 0x000002 shall not occur at any uint8_t-aligned position
    if (zeroCount == 2) {
      if (bufferPtr[fromIndex] < 0x03) {
        mNaluBytes = -1;
        return mNaluBytes;
        }

      if (bufferPtr[fromIndex] == 0x03) {
        // check the 4th uint8_t after 0x000003
        // - except if cabacZeroWord, last 3 bytes of nalu are 0x000003
        // if cabacZeroWord, final byte of nalu(0x03) is discarded, last 2 bytes RBSP must be 0x0000
        if ((fromIndex < endBytePos-1) && (bufferPtr[fromIndex+1] > 0x03)) {
          mNaluBytes = -1;
          return mNaluBytes;
          }
        if (fromIndex == endBytePos-1) {
          mNaluBytes = toIndex;
          return mNaluBytes;
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

  mNaluBytes = toIndex;
  return mNaluBytes;
  }
//}}}

//{{{
void cNalu::debug() {
  cLog::log (LOGINFO, getNaluString());
  }
//}}}
