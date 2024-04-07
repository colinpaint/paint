//{{{  includes
#include "global.h"
#include "memory.h"

#include "cCabacDecode.h"
//}}}

//{{{  defines
#define B_BITS    10      // Number of bits to represent the whole coding interval
#define HALF      0x01FE  //(1 << (B_BITS-1)) - 2
#define QUARTER   0x0100  //(1 << (B_BITS-2))
//}}}
namespace {
  //{{{
  const uint8_t kReNormTable[32] = {
    6,
    5,
    4,4,
    3,3,3,3,
    2,2,2,2,2,2,2,2,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    };
  //}}}
  //{{{
  const uint8_t kLpsTable64x4[64][4] = {
    { 128, 176, 208, 240},
    { 128, 167, 197, 227},
    { 128, 158, 187, 216},
    { 123, 150, 178, 205},
    { 116, 142, 169, 195},
    { 111, 135, 160, 185},
    { 105, 128, 152, 175},
    { 100, 122, 144, 166},
    {  95, 116, 137, 158},
    {  90, 110, 130, 150},
    {  85, 104, 123, 142},
    {  81,  99, 117, 135},
    {  77,  94, 111, 128},
    {  73,  89, 105, 122},
    {  69,  85, 100, 116},
    {  66,  80,  95, 110},
    {  62,  76,  90, 104},
    {  59,  72,  86,  99},
    {  56,  69,  81,  94},
    {  53,  65,  77,  89},
    {  51,  62,  73,  85},
    {  48,  59,  69,  80},
    {  46,  56,  66,  76},
    {  43,  53,  63,  72},
    {  41,  50,  59,  69},
    {  39,  48,  56,  65},
    {  37,  45,  54,  62},
    {  35,  43,  51,  59},
    {  33,  41,  48,  56},
    {  32,  39,  46,  53},
    {  30,  37,  43,  50},
    {  29,  35,  41,  48},
    {  27,  33,  39,  45},
    {  26,  31,  37,  43},
    {  24,  30,  35,  41},
    {  23,  28,  33,  39},
    {  22,  27,  32,  37},
    {  21,  26,  30,  35},
    {  20,  24,  29,  33},
    {  19,  23,  27,  31},
    {  18,  22,  26,  30},
    {  17,  21,  25,  28},
    {  16,  20,  23,  27},
    {  15,  19,  22,  25},
    {  14,  18,  21,  24},
    {  14,  17,  20,  23},
    {  13,  16,  19,  22},
    {  12,  15,  18,  21},
    {  12,  14,  17,  20},
    {  11,  14,  16,  19},
    {  11,  13,  15,  18},
    {  10,  12,  15,  17},
    {  10,  12,  14,  16},
    {   9,  11,  13,  15},
    {   9,  11,  12,  14},
    {   8,  10,  12,  14},
    {   8,   9,  11,  13},
    {   7,   9,  11,  12},
    {   7,   9,  10,  12},
    {   7,   8,  10,  11},
    {   6,   8,   9,  11},
    {   6,   7,   9,  10},
    {   6,   7,   8,   9},
    {   2,   2,   2,   2}
    };
  //}}}
  //{{{
  const uint8_t kAcNextStateMps64[64] = {
    1,2,3,4,5,6,7,8,9,10,
    11,12,13,14,15,16,17,18,19,20,
    21,22,23,24,25,26,27,28,29,30,
    31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,
    51,52,53,54,55,56,57,58,59,60,
    61,62,62,63
    };
  //}}}
  //{{{
  const uint8_t kAcNextStateLps64[64] = {
    0, 0, 1, 2, 2, 4, 4, 5, 6, 7,
    8, 9, 9,11,11,12,13,13,15,15,
    16,16,18,18,19,19,21,21,22,22,
    23,24,24,25,26,26,27,27,28,29,
    29,30,30,30,31,32,32,33,33,33,
    34,34,35,35,35,36,36,36,37,37,
    37,38,38,63
    };
  //}}}
  }

//{{{
void cCabacDecode::startDecoding (uint8_t* code_buffer, int firstbyte, int* codeLen) {

  codeStream = code_buffer;
  codeStreamLen = codeLen;
  *codeStreamLen = firstbyte;

  // lookahead of 2 bytes: always make sure that s buffer
  // contains 2 more bytes than actual s
  value = (getByte() << 16) | getWord();
  bitsLeft = 15;
  range = HALF;
  }
//}}}

//{{{
void cCabacDecode::ipcmPreamble() {

  while (bitsLeft >= 8) {
    value >>= 8;
    bitsLeft -= 8;
    (*codeStreamLen)--;
    }
  }
//}}}

//{{{
uint32_t cCabacDecode::symbol (sBiContext* biContext) {

  uint32_t bit = biContext->MPS;
  uint32_t* value1 = &value;
  uint32_t* range1 = &range;
  int* bitsLeft1 = &bitsLeft;
  uint16_t* state = &biContext->state;
  uint32_t rLPS = kLpsTable64x4[*state][(*range1 >> 6) & 0x03];

  *range1 -= rLPS;
  if (*value1 < (*range1 << *bitsLeft1)) {
    // MPS
    *state = kAcNextStateMps64[*state]; // next state
    if (*range1 >= QUARTER)
      return (bit);
    else {
      *range1 <<= 1;
      (*bitsLeft1)--;
      }
    }

  else {
    // LPS
    int renorm = kReNormTable[(rLPS >> 3) & 0x1F];
    *value1 -= (*range1 << *bitsLeft1);
    *range1 = rLPS << renorm;
    (*bitsLeft1) -= renorm;
    bit ^= 0x01;
    if (!(*state))  // switch meaning of MPS if necessary
      biContext->MPS ^= 0x01;
    *state = kAcNextStateLps64[*state]; // next state
    }

  if (*bitsLeft1 > 0 )
    return bit;
  else {
    *value1 <<= 16;
    // lookahead of 2 bytes: always make sure that s buffer
    // contains 2 more bytes than actual s
    *value1 |=  getWord();
    (*bitsLeft1) += 16;
    return bit;
    }
  }
//}}}
//{{{
uint32_t cCabacDecode::symbolEqProb() {

  uint32_t* value1 = &value;
  int* bitsLeft1 = &bitsLeft;
  if (--(*bitsLeft1) == 0) {
    // lookahead of 2 bytes: always make sure that s buffer contains 2 more bytes than actual s
    *value1 = (*value1 << 16) | getWord();
    *bitsLeft1 = 16;
    }

  int value2 = *value1 - (range << *bitsLeft1);
  if (value2 < 0)
    return 0;
  else {
    *value1 = value2;
    return 1;
    }
  }
//}}}
//{{{
uint32_t cCabacDecode::final() {

  uint32_t range1 = range - 2;

  int value1 = value - (range1 << bitsLeft);
  if (value1 < 0) {
    if (range1 >= QUARTER) {
      range = range1;
      return 0;
      }
    else {
      range = range1 << 1;
      if (--(bitsLeft) > 0)
        return 0;
      else {
        // lookahead of 2 bytes: always make sure that s buffer contains 2 more bytes than actual s
        value = (value << 16) | getWord();
        bitsLeft = 16;
        return 0;
        }
      }
    }
  else
    return 1;
  }
//}}}

// private
//{{{
uint32_t cCabacDecode::getByte() {
  return codeStream[(*codeStreamLen)++];
  }
//}}}
//{{{
uint32_t cCabacDecode::getWord() {

  int* len = codeStreamLen;
  uint8_t* codeStreamPtr = &codeStream[*len];
  *len += 2;

  return (*codeStreamPtr << 8) | *(codeStreamPtr + 1);
  }
//}}}

//{{{
void binaryArithmeticInitContext (int qp, sBiContext* context, const char* ini) {

  int state = ((ini[0] * qp) >> 4) + ini[1];

  if (state >= 64) {
    state = imin (126, state);
    context->state = (uint16_t)(state - 64);
    context->MPS = 1;
    }
  else {
    state = imax (1, state);
    context->state = (uint16_t)(63 - state);
    context->MPS = 0;
    }
  }
//}}}
