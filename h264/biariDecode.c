//{{{  includes
#include "global.h"
#include "memory.h"

#include "biariDecode.h"
//}}}

//{{{  defines
#define B_BITS    10      // Number of bits to represent the whole coding interval
#define HALF      0x01FE  //(1 << (B_BITS-1)) - 2
#define QUARTER   0x0100  //(1 << (B_BITS-2))
//}}}
//{{{
static const byte rLPS_table_64x4[64][4] = {
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
static const byte AC_next_state_MPS_64[64] = {
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
static const byte AC_next_state_LPS_64[64] = {
  0, 0, 1, 2, 2, 4, 4, 5, 6, 7,
  8, 9, 9,11,11,12,13,13,15,15,
  16,16,18,18,19,19,21,21,22,22,
  23,24,24,25,26,26,27,27,28,29,
  29,30,30,30,31,32,32,33,33,33,
  34,34,35,35,35,36,36,36,37,37,
  37,38,38,63
  };
//}}}
//{{{
static const byte renorm_table_32[32] = {
  6,
  5,
  4,4,
  3,3,3,3,
  2,2,2,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
  };
//}}}

//{{{
static inline unsigned int getByte (sDecodeEnv* decodeEnv) {
  return decodeEnv->codeStream[(*decodeEnv->codeStreamLen)++];
  }
//}}}
//{{{
static inline unsigned int getWord (sDecodeEnv* decodeEnv) {

  int* len = decodeEnv->codeStreamLen;
  byte* p_code_strm = &decodeEnv->codeStream[*len];
  *len += 2;

  return (*p_code_strm << 8) | *(p_code_strm + 1);
  }
//}}}

//{{{
void aridecoStartDecoding (sDecodeEnv* decodeEnv, unsigned char* code_buffer, int firstbyte, int* codeLen) {

  decodeEnv->codeStream = code_buffer;
  decodeEnv->codeStreamLen = codeLen;
  *decodeEnv->codeStreamLen = firstbyte;

  decodeEnv->value = getByte (decodeEnv);
  // lookahead of 2 bytes: always make sure that s buffer
  // contains 2 more bytes than actual s
  decodeEnv->value = (decodeEnv->value << 16) | getWord (decodeEnv);
  decodeEnv->bitsLeft = 15;
  decodeEnv->range = HALF;
  }
//}}}
//{{{
int aridecoBitsRead (sDecodeEnv* decodeEnv) {
  return ((*decodeEnv->codeStreamLen) << 3) - decodeEnv->bitsLeft;
  }
//}}}

//{{{
unsigned int biarDecodeSymbol (sDecodeEnv* decodeEnv, sBiContextType* biContext) {

  unsigned int bit = biContext->MPS;
  unsigned int* value = &decodeEnv->value;
  unsigned int* range = &decodeEnv->range;

  uint16* state = &biContext->state;
  unsigned int rLPS = rLPS_table_64x4[*state][(*range >> 6) & 0x03];
  int* bitsLeft = &decodeEnv->bitsLeft;

  *range -= rLPS;
  if (*value < (*range << *bitsLeft)) {
    // MPS
    *state = AC_next_state_MPS_64[*state]; // next state
    if (*range >= QUARTER)
      return (bit);
    else {
      *range <<= 1;
      (*bitsLeft)--;
      }
    }

  else {
    // LPS
    int renorm = renorm_table_32[(rLPS >> 3) & 0x1F];
    *value -= (*range << *bitsLeft);
    *range = (rLPS << renorm);
    (*bitsLeft) -= renorm;
    bit ^= 0x01;
    if (!(*state))  // switch meaning of MPS if necessary
      biContext->MPS ^= 0x01;
    *state = AC_next_state_LPS_64[*state]; // next state
    }

  if (*bitsLeft > 0 )
    return (bit);
  else {
    *value <<= 16;
    // lookahead of 2 bytes: always make sure that s buffer
    // contains 2 more bytes than actual s
    *value |=  getWord (decodeEnv);
    (*bitsLeft) += 16;
    return bit;
    }
  }
//}}}
//{{{
unsigned int biariDecodeSymbolEqProb (sDecodeEnv* decodeEnv) {

  int tmp_value;
  unsigned int* value = &decodeEnv->value;

  int* bitsLeft = &decodeEnv->bitsLeft;
  if (--(*bitsLeft) == 0) {
    // lookahead of 2 bytes: always make sure that s buffer contains 2 more bytes than actual s
    *value = (*value << 16) | getWord (decodeEnv);
    *bitsLeft = 16;
    }

  tmp_value = *value - (decodeEnv->range << *bitsLeft);
  if (tmp_value < 0)
    return 0;
  else {
    *value = tmp_value;
    return 1;
    }
  }
//}}}
//{{{
unsigned int biariDecodeFinal (sDecodeEnv* decodeEnv) {

  unsigned int range  = decodeEnv->range - 2;

  int value = decodeEnv->value;
  value -= (range << decodeEnv->bitsLeft);
  if (value < 0) {
    if (range >= QUARTER) {
      decodeEnv->range = range;
      return 0;
      }
    else {
      decodeEnv->range = (range << 1);
      if (--(decodeEnv->bitsLeft) > 0)
        return 0;
      else {
        // lookahead of 2 bytes: always make sure that s buffer contains 2 more bytes than actual s
        decodeEnv->value = (decodeEnv->value << 16) | getWord (decodeEnv);
        decodeEnv->bitsLeft = 16;
        return 0;
        }
      }
    }
  else
    return 1;
  }
//}}}

//{{{
void biariInitContext (int qp, sBiContextType* context, const char* ini) {

  int pstate = ((ini[0] * qp) >> 4) + ini[1];

  if (pstate >= 64) {
    pstate = imin (126, pstate);
    context->state = (uint16)(pstate - 64);
    context->MPS = 1;
    }
  else {
    pstate = imax (1, pstate);
    context->state = (uint16)(63 - pstate);
    context->MPS = 0;
    }
  }
//}}}
