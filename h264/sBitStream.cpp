//{{{  includes
#include "global.h"

// utils
#include "../common/cLog.h"

using namespace std;
//}}}
namespace {
  //{{{
  int showBitsThreshold (int inf, int numbits) {
    return ((inf) >> ((sizeof(uint8_t) * 24) - (numbits)));
    }
  //}}}
  //{{{
  int code2d (sSyntaxElement* se, sBitStream* s,
              const uint8_t* lentab, const uint8_t* codtab,
              int tabwidth, int tabheight, int* code) {

    const uint8_t* len = &lentab[0], *cod = &codtab[0];

    int* mOffset = &s->mOffset;
    uint8_t* buf = &s->mBuffer[*mOffset >> 3];

    // Apply bitOffset to three bytes (maximum that may be traversed by showBitsThreshold)
    // Even at the end of a stream we will still be pulling out of allocated memory as alloc is done by kMaxFrameSize
    uint32_t inf = ((*buf) << 16) + (*(buf + 1) << 8) + *(buf + 2);
    // Offset is constant so apply before extracting different numbers of bits
    inf <<= (*mOffset & 0x07);
    // Arithmetic shift so wipe any sign which may be extended inside showBitsThreshold
    inf  &= 0xFFFFFF;

    // this VLC decoding method is not optimized for speed
    for (int j = 0; j < tabheight; j++) {
      for (int i = 0; i < tabwidth; i++) {
        if ((*len == 0) || (showBitsThreshold(inf, (int) *len) != *cod)) {
          ++len;
          ++cod;
          }
        else {
          se->len = *len;
          *mOffset += *len; // move s pointer
          *code = *cod;
          se->value1 = i;
          se->value2 = j;
          return 0;                 // found code and return
          }
        }
      }

    return -1;  // failed to find code
    }
  //}}}
  }

// static members
//{{{
void sBitStream::infoUe (int len, int info, int* value1, int* dummy) {
  *value1 = (int)(((uint32_t) 1 << (len >> 1)) + (uint32_t) (info) - 1);
  }
//}}}
//{{{
void sBitStream::infoSe (int len, int info, int* value1, int* dummy) {

  uint32_t n = ((uint32_t) 1 << (len >> 1)) + (uint32_t) info - 1;
  *value1 = (n + 1) >> 1;

  // lsb is signed bit
  if ((n & 0x01) == 0)
    *value1 = -*value1;
  }
//}}}
//{{{
void sBitStream::infoCbpIntraNormal (int len, int info, int* codedBlockPattern, int* dummy) {

  int cbp_idx;
  infoUe (len, info, &cbp_idx, dummy);
  *codedBlockPattern = sBitStream::kNCBP[1][cbp_idx][0];
  }
//}}}
//{{{
void sBitStream::infoCbpIntraOther (int len, int info, int* codedBlockPattern, int* dummy) {

  int cbp_idx;
  infoUe (len, info, &cbp_idx, dummy);
  *codedBlockPattern = sBitStream::kNCBP[0][cbp_idx][0];
  }
//}}}
//{{{
void sBitStream::infoCbpInterNormal (int len, int info, int* codedBlockPattern, int* dummy) {

  int cbp_idx;
  infoUe (len, info, &cbp_idx, dummy);
  *codedBlockPattern = sBitStream::kNCBP[1][cbp_idx][1];
  }
//}}}
//{{{
void sBitStream::infoCbpInterOther (int len, int info, int* codedBlockPattern, int *dummy) {

  int cbp_idx;
  infoUe (len, info, &cbp_idx, dummy);
  *codedBlockPattern = sBitStream::kNCBP[0][cbp_idx][1];
  }
//}}}
//{{{
void sBitStream::infoLevelRunInter (int len, int info, int* level, int* irun) {

  if (len <= 9) {
    int l2 = imax(0,(len >> 1)-1);
    int inf = info >> 1;
    *level = sBitStream::kNTAB1[l2][inf][0];
    *irun = sBitStream::kNTAB1[l2][inf][1];
    if ((info & 0x01) == 1)
      *level = -*level; // make sign
    }
  else {
    // if len > 9, skip using the array
    *irun = (info & 0x1e) >> 1;
    *level = sBitStream::kLEVRUN1[*irun] + (info >> 5) + ( 1 << ((len >> 1) - 5));
    if ((info & 0x01) == 1)
      *level = -*level;
    }

  if (len == 1) // EOB
    *level = 0;
  }
//}}}
//{{{
void sBitStream::infoLevelRunc2x2 (int len, int info, int* level, int* irun) {

  if (len <= 5) {
    int l2 = imax(0, (len >> 1) - 1);
    int inf = info >> 1;
    *level = sBitStream::kNTAB3[l2][inf][0];
    *irun = sBitStream::kNTAB3[l2][inf][1];
    if ((info & 0x01) == 1)
      *level = -*level; // make sign
    }
  else {
    // if len > 5, skip using the array
    *irun  = (info & 0x06) >> 1;
    *level = sBitStream::kLEVRUN3[*irun] + (info >> 3) + (1 << ((len >> 1) - 3));
    if ((info & 0x01) == 1)
      *level = -*level;
    }

  if (len == 1) // EOB
    *level = 0;
  }
//}}}

//{{{
int sBitStream::vlcStartCode (cSlice* slice, int dummy) {

  uint8_t partitionIndex = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode][SE_MBTYPE];
  sDataPartition* dataPartition = &slice->dataPartitionArray[partitionIndex];
  return !moreRbspData (dataPartition->mBitStream.mBuffer,
                        dataPartition->mBitStream.mOffset, dataPartition->mBitStream.mLength);
  }
//}}}
//{{{
int sBitStream::readSyntaxElementVLC (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* dataPartition) {
  return dataPartition->mBitStream.readSyntaxElementVLC (se);
  }
//}}}

//{{{
int sBitStream::getVlcSymbol (uint8_t buffer[], int totalBitOffset, int* info, int bytecount) {

  long byteoffset = totalBitOffset >> 3;        // uint8_t from start of buffer
  int bitOffset  = 7 - (totalBitOffset & 0x07); // bit from start of uint8_t
  int bitCounter = 1;
  int len = 0;
  uint8_t* curByte = &(buffer[byteoffset]);

  int controlBit = ((*curByte) >> (bitOffset)) & 0x01;  // control bit for current bit posision
  while (controlBit == 0) {
    // find leading 1 bit
    len++;
    bitCounter++;
    bitOffset--;
    bitOffset &= 0x07;
    curByte += (bitOffset == 7);
    byteoffset += (bitOffset == 7);
    controlBit = ((*curByte) >> bitOffset) & 0x01;
    }

  if (byteoffset + ((len + 7) >> 3) > bytecount)
    return -1;

  int inf = 0;                          // shortest possible code is 1, then info is always 0
  while (len--) {
    bitOffset--;
    bitOffset &= 0x07;
    curByte += (bitOffset == 7);
    bitCounter++;
    inf <<= 1;
    inf |= ((*curByte) >> (bitOffset)) & 0x01;
    }

  *info = inf;
  return bitCounter;           // return absolute offset in bit from start of frame
  }
//}}}
//{{{
int sBitStream::getVlcSymbolIntraMode (uint8_t buffer[], int totalBitOffset, int* info, int bytecount) {

  int byteoffset = (totalBitOffset >> 3);        // uint8_t from start of buffer
  int bitOffset = (7 - (totalBitOffset & 0x07)); // bit from start of uint8_t
  uint8_t* cur_byte = &(buffer[byteoffset]);

  // first bit
  int controlBit = (*cur_byte & (0x01 << bitOffset));      // control bit for current bit posision
  if (controlBit) {
    *info = 0;
    return 1;
    }

  if (byteoffset >= bytecount)
    return -1;
  else {
    int inf = (*(cur_byte) << 8) + *(cur_byte + 1);
    inf <<= (sizeof(uint8_t) * 8) - bitOffset;
    inf = inf & 0xFFFF;
    inf >>= (sizeof(uint8_t) * 8) * 2 - 3;
    *info = inf;
    return 4;           // return absolute offset in bit from start of frame
    }
  }
//}}}
//{{{
int sBitStream::getBits (uint8_t buffer[], int totalBitOffset, int* info, int bitCount, int numBits) {

  if ((totalBitOffset + numBits) > bitCount)
    return -1;

  int bitOffset  = 7 - (totalBitOffset & 0x07); // bit from start of uint8_t
  int byteOffset = totalBitOffset >> 3;       // uint8_t from start of buffer
  int bitCounter = numBits;
  uint8_t* curByte = &buffer[byteOffset];

  int inf = 0;
  while (numBits--) {
    inf <<=1;
    inf |= ((*curByte)>> (bitOffset--)) & 0x01;
    if (bitOffset == -1) {
      // Move onto next uint8_t to get all of numBits
      curByte++;
      bitOffset = 7;
      }

    // Above conditional could also be avoided using the following:
    // curbyte   -= (bitOffset >> 3);
    // bitOffset &= 0x07;
    }

  *info = inf;
  return bitCounter;           // return absolute offset in bit from start of frame
  }
//}}}
//{{{
int sBitStream::showBits (uint8_t buffer[], int totalBitOffset, int bitCount, int numBits) {

  if ((totalBitOffset + numBits) > bitCount)
    return -1;

  int bitOffset = 7 - (totalBitOffset & 0x07); // bit from start of uint8_t
  int byteOffset = totalBitOffset >> 3;      // uint8_t from start of buffer
  uint8_t* curByte = &buffer[byteOffset];

  int inf = 0;
  while (numBits--) {
    inf <<= 1;
    inf |= ((*curByte)>> (bitOffset--)) & 0x01;

    if (bitOffset == -1 ) {
      // Move onto next uint8_t to get all of numbits
      curByte++;
      bitOffset = 7;
      }
    }

  return inf; // return absolute offset in bit from start of frame
  }
//}}}
//{{{
int sBitStream::moreRbspData (uint8_t buffer[], int totalBitOffset, int bytecount) {

  // there is more until we're in the last uint8_t
  long byteoffset = totalBitOffset >> 3;      // uint8_t from start of buffer
  if (byteoffset < (bytecount - 1))
    return true;
  else {
    int bitOffset = 7 - (totalBitOffset & 0x07);      // bit from start of uint8_t
    uint8_t* cur_byte  = &buffer[byteoffset];
    // read one bit
    int controlBit = ((*cur_byte) >> (bitOffset--)) & 0x01;   // control bit for current bit posision

    // a stop bit has to be one
    if (controlBit == 0)
      return true;
    else {
      int cnt = 0;
      while (bitOffset >= 0 && !cnt)
        cnt |= ((*cur_byte)>> (bitOffset--)) & 0x01;   // set up control bit
      return cnt;
      }
    }
  }
//}}}

// members
//{{{
int sBitStream::readUeV (const string& label) {

  sSyntaxElement symbol;
  symbol.type = SE_HEADER;
  symbol.value1 = 0;
  symbol.mapping = infoUe;
  readSyntaxElementVLC (&symbol);

  if (cDecoder264::gDecoder->param.vlcDebug)
    cLog::log (LOGINFO, fmt::format ("{} {}", label, symbol.value1));

  return symbol.value1;
  }
//}}}
//{{{
int sBitStream::readSeV (const string& label) {

  sSyntaxElement symbol;
  symbol.type = SE_HEADER;
  symbol.value1 = 0;
  symbol.mapping = infoSe;
  readSyntaxElementVLC (&symbol);

  if (cDecoder264::gDecoder->param.vlcDebug)
   cLog::log (LOGINFO, fmt::format ("{} {}", label, symbol.value1));

  return symbol.value1;
  }
//}}}
//{{{
int sBitStream::readUv (int lenInBits, const string& label) {

  sSyntaxElement symbol;
  symbol.type = SE_HEADER;
  symbol.value1 = 0;
  symbol.len = lenInBits;
  symbol.inf = 0;
  symbol.mapping = infoUe;
  readSyntaxElementFLC (&symbol);

  if (cDecoder264::gDecoder->param.vlcDebug)
    cLog::log (LOGINFO, fmt::format ("{} {}", label, symbol.inf));

  return symbol.inf;
  }
//}}}
//{{{
int sBitStream::readIv (int lenInBits, const string& label) {

  sSyntaxElement symbol;
  symbol.type = SE_HEADER;
  symbol.value1 = 0;
  symbol.len = lenInBits;
  symbol.inf = 0;
  symbol.mapping = infoUe;
  readSyntaxElementFLC (&symbol);

  // can be negative
  symbol.inf = -( symbol.inf & (1 << (lenInBits - 1)) ) | symbol.inf;

  if (cDecoder264::gDecoder->param.vlcDebug)
    cLog::log (LOGINFO, fmt::format ("{} {}", label, symbol.inf));

  return symbol.inf;
  }
//}}}
//{{{
bool sBitStream::readU1 (const string& label) {
  return (bool)readUv (1, label);
  }
//}}}

//{{{
int sBitStream::readSyntaxElementVLC (sSyntaxElement* se) {

  se->len = getVlcSymbol (mBuffer, mOffset, &se->inf, mLength);
  if (se->len == -1)
    return -1;

  mOffset += se->len;
  se->mapping (se->len, se->inf, &se->value1, &se->value2);

  return 1;
  }
//}}}
//{{{
int sBitStream::readSyntaxElementFLC (sSyntaxElement* se) {

  int bitstreamLengthInBits = (mLength << 3) + 7;
  if (getBits (mBuffer, mOffset, &(se->inf), bitstreamLengthInBits, se->len) < 0)
    return -1;

  se->value1 = se->inf;
  mOffset += se->len; // move s pointer

  return 1;
  }
//}}}
//{{{
int sBitStream::readSyntaxElementIntra4x4PredictionMode (sSyntaxElement* se) {

  se->len = getVlcSymbolIntraMode (mBuffer, mOffset, &se->inf, mLength);
  if (se->len == -1)
    return -1;

  mOffset += se->len;
  se->value1 = (se->len == 1) ? -1 : se->inf;

  return 1;
  }
//}}}

//{{{
int sBitStream::readSyntaxElementNumCoeffTrailingOnes (sSyntaxElement* se, char *type) {

  //{{{
  static const uint8_t lentab[3][4][17] = {
    {   // 0702
      { 1, 6, 8, 9,10,11,13,13,13,14,14,15,15,16,16,16,16},
      { 0, 2, 6, 8, 9,10,11,13,13,14,14,15,15,15,16,16,16},
      { 0, 0, 3, 7, 8, 9,10,11,13,13,14,14,15,15,16,16,16},
      { 0, 0, 0, 5, 6, 7, 8, 9,10,11,13,14,14,15,15,16,16},
    },
    {
      { 2, 6, 6, 7, 8, 8, 9,11,11,12,12,12,13,13,13,14,14},
      { 0, 2, 5, 6, 6, 7, 8, 9,11,11,12,12,13,13,14,14,14},
      { 0, 0, 3, 6, 6, 7, 8, 9,11,11,12,12,13,13,13,14,14},
      { 0, 0, 0, 4, 4, 5, 6, 6, 7, 9,11,11,12,13,13,13,14},
    },
    {
      { 4, 6, 6, 6, 7, 7, 7, 7, 8, 8, 9, 9, 9,10,10,10,10},
      { 0, 4, 5, 5, 5, 5, 6, 6, 7, 8, 8, 9, 9, 9,10,10,10},
      { 0, 0, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,10,10,10},
      { 0, 0, 0, 4, 4, 4, 4, 4, 5, 6, 7, 8, 8, 9,10,10,10},
    },
  };
  //}}}
  //{{{
  static const uint8_t codtab[3][4][17] = {
    {
      { 1, 5, 7, 7, 7, 7,15,11, 8,15,11,15,11,15,11, 7,4},
      { 0, 1, 4, 6, 6, 6, 6,14,10,14,10,14,10, 1,14,10,6},
      { 0, 0, 1, 5, 5, 5, 5, 5,13, 9,13, 9,13, 9,13, 9,5},
      { 0, 0, 0, 3, 3, 4, 4, 4, 4, 4,12,12, 8,12, 8,12,8},
    },
    {
      { 3,11, 7, 7, 7, 4, 7,15,11,15,11, 8,15,11, 7, 9,7},
      { 0, 2, 7,10, 6, 6, 6, 6,14,10,14,10,14,10,11, 8,6},
      { 0, 0, 3, 9, 5, 5, 5, 5,13, 9,13, 9,13, 9, 6,10,5},
      { 0, 0, 0, 5, 4, 6, 8, 4, 4, 4,12, 8,12,12, 8, 1,4},
    },
    {
      {15,15,11, 8,15,11, 9, 8,15,11,15,11, 8,13, 9, 5,1},
      { 0,14,15,12,10, 8,14,10,14,14,10,14,10, 7,12, 8,4},
      { 0, 0,13,14,11, 9,13, 9,13,10,13, 9,13, 9,11, 7,3},
      { 0, 0, 0,12,11,10, 9, 8,13,12,12,12, 8,12,10, 6,2},
    },
  };
  //}}}

  int offset = mOffset;
  int BitstreamLengthInBytes = mLength;
  int BitstreamLengthInBits = (BitstreamLengthInBytes << 3) + 7;
  uint8_t* buf = mBuffer;

  int retval = 0, code;
  int vlcnum = se->value1;

  // vlcnum is the index of Table used to code coeff_token
  // vlcnum == 3 means (8<=nC) which uses 6bit FLC
  if (vlcnum == 3) {
    // read 6 bit FLC
    code = showBits (buf, offset, BitstreamLengthInBits, 6);
    offset += 6;
    se->value2 = (code & 3);
    se->value1 = (code >> 2);

    if (!se->value1 && se->value2 == 3)
      // #c = 0, #t1 = 3 =>  #c = 0
      se->value2 = 0;
    else
      se->value1++;

    se->len = 6;
    }
  else {
    retval = code2d (se, this, lentab[vlcnum][0], codtab[vlcnum][0], 17, 4, &code);
    if (retval)
      cDecoder264::error ("ERROR: failed to find NumCoeff/TrailingOnes");
    }

  return retval;
  }
//}}}
//{{{
int sBitStream::readSyntaxElementNumCoeffTrailingOnesChromaDC (cDecoder264* decoder, sSyntaxElement* se) {

  //{{{
  static const uint8_t lentab[3][4][17] = {
    //YUV420
    {{ 2, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 1, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 3, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    //YUV422
    {{ 1, 7, 7, 9, 9,10,11,12,13, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 2, 7, 7, 9,10,11,12,12, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 3, 7, 7, 9,10,11,12, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 5, 6, 7, 7,10,11, 0, 0, 0, 0, 0, 0, 0, 0}},

    //YUV444
    {{ 1, 6, 8, 9,10,11,13,13,13,14,14,15,15,16,16,16,16},
    { 0, 2, 6, 8, 9,10,11,13,13,14,14,15,15,15,16,16,16},
    { 0, 0, 3, 7, 8, 9,10,11,13,13,14,14,15,15,16,16,16},
    { 0, 0, 0, 5, 6, 7, 8, 9,10,11,13,14,14,15,15,16,16}}
  };
  //}}}
  //{{{
  static const uint8_t codtab[3][4][17] = {
    //YUV420
    {{ 1, 7, 4, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 1, 6, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    //YUV422
    {{ 1,15,14, 7, 6, 7, 7, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 1,13,12, 5, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 1,11,10, 4, 5, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 1, 1, 9, 8, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0}},

    //YUV444
    {{ 1, 5, 7, 7, 7, 7,15,11, 8,15,11,15,11,15,11, 7, 4},
    { 0, 1, 4, 6, 6, 6, 6,14,10,14,10,14,10, 1,14,10, 6},
    { 0, 0, 1, 5, 5, 5, 5, 5,13, 9,13, 9,13, 9,13, 9, 5},
    { 0, 0, 0, 3, 3, 4, 4, 4, 4, 4,12,12, 8,12, 8,12, 8}}

  };
  //}}}

  int code;
  int yuv = decoder->activeSps->chromaFormatIdc - 1;
  int retval = code2d (se, this, &lentab[yuv][0][0], &codtab[yuv][0][0], 17, 4, &code);

  if (retval)
    cDecoder264::error ("failed to find NumCoeff/TrailingOnes ChromaDC");

  return retval;
  }
//}}}

//{{{
int sBitStream::readSyntaxElementLevelVlc0 (sSyntaxElement* se) {

  int offset = mOffset;
  int bitstreamLengthInBytes = mLength;
  int bitstreamLengthInBits = (bitstreamLengthInBytes << 3) + 7;
  uint8_t* buf = mBuffer;

  int len = 1;
  int sign = 0;
  int level = 0;
  int  code = 1;

  while (!showBits (buf, offset++, bitstreamLengthInBits, 1))
    len++;

  if (len < 15) {
    sign  = (len - 1) & 1;
    level = ((len - 1) >> 1) + 1;
    }
  else if (len == 15) {
    // escape code
    code <<= 4;
    code |= showBits(buf, offset, bitstreamLengthInBits, 4);
    len  += 4;
    offset += 4;
    sign = code & 0x01;
    level = ((code >> 1) & 0x07) + 8;
    }
  else if (len >= 16) {
    // escape code
    int addbit = (len - 16);
    int offset1 = (2048 << addbit) - 2032;
    len -= 4;
    code = showBits (buf, offset, bitstreamLengthInBits, len);
    sign = code & 0x01;
    offset += len;
    level = (code >> 1) + offset1;

    code |= (1 << len); // for display purpose only
    len += addbit + 16;
    }

  se->inf = sign ? -level : level;
  se->len = len;

  mOffset = offset;
  return 0;
  }
//}}}
//{{{
int sBitStream::readSyntaxElementLevelVlcN (sSyntaxElement* se, int vlc) {

  int offset = mOffset;
  int bitstreamLengthInBytes = mLength;
  int bitstreamLengthInBits  = (bitstreamLengthInBytes << 3) + 7;

  int levabs;
  int sign;
  int len = 1;
  int code = 1;
  int sb;
  int shift = vlc - 1;

  // read pre zeros
  while (!showBits (mBuffer, offset ++, bitstreamLengthInBits, 1))
    len++;

  offset -= len;

  if (len < 16) {
    levabs = ((len - 1) << shift) + 1;

    // read (vlc-1) bits -> suffix
    if (shift) {
      sb =  showBits (mBuffer, offset + len, bitstreamLengthInBits, shift);
      code = (code << (shift) )| sb;
      levabs += sb;
      len += (shift);
    }

    // read 1 bit -> sign
    sign = showBits (mBuffer, offset + len, bitstreamLengthInBits, 1);
    code = (code << 1)| sign;
    len ++;
    }
  else {
    // escape
    int addbit = len - 5;
    int offset1 = (1 << addbit) + (15 << shift) - 2047;

    sb = showBits (mBuffer, offset1 + len, bitstreamLengthInBits, addbit);
    code = (code << addbit ) | sb;
    len   += addbit;

    levabs = sb + offset1;

    // read 1 bit -> sign
    sign = showBits (mBuffer, offset1 + len, bitstreamLengthInBits, 1);
    code = (code << 1)| sign;
    len++;
    }

  se->inf = (sign)? -levabs : levabs;
  se->len = len;

  mOffset = offset + len;

  return 0;
}
//}}}

//{{{
int sBitStream::readSyntaxElementTotalZeros (sSyntaxElement* se) {

  //{{{
  static const uint8_t lentab[TOTRUN_NUM][16] = {
    { 1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9},
    { 3,3,3,3,3,4,4,4,4,5,5,6,6,6,6},
    { 4,3,3,3,4,4,3,3,4,5,5,6,5,6},
    { 5,3,4,4,3,3,3,4,3,4,5,5,5},
    { 4,4,4,3,3,3,3,3,4,5,4,5},
    { 6,5,3,3,3,3,3,3,4,3,6},
    { 6,5,3,3,3,2,3,4,3,6},
    { 6,4,5,3,2,2,3,3,6},
    { 6,6,4,2,2,3,2,5},
    { 5,5,3,2,2,2,4},
    { 4,4,3,3,1,3},
    { 4,4,2,1,3},
    { 3,3,1,2},
    { 2,2,1},
    { 1,1},
  };
  //}}}
  //{{{
  static const uint8_t codtab[TOTRUN_NUM][16] = {
    {1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1},
    {7,6,5,4,3,5,4,3,2,3,2,3,2,1,0},
    {5,7,6,5,4,3,4,3,2,3,2,1,1,0},
    {3,7,5,4,6,5,4,3,3,2,2,1,0},
    {5,4,3,7,6,5,4,3,2,1,1,0},
    {1,1,7,6,5,4,3,2,1,1,0},
    {1,1,5,4,3,3,2,1,1,0},
    {1,1,1,3,3,2,2,1,0},
    {1,0,1,3,2,1,1,1,},
    {1,0,1,3,2,1,1,},
    {0,1,1,2,1,3},
    {0,1,1,1,1},
    {0,1,1,1},
    {0,1,1},
    {0,1},
  };
  //}}}

  int code;
  int vlcnum = se->value1;
  int retval = code2d (se, this, &lentab[vlcnum][0], &codtab[vlcnum][0], 16, 1, &code);
  if (retval)
    cDecoder264::error ("failed to find Total Zeros !cdc");

  return retval;
  }
//}}}
//{{{
int sBitStream::readSyntaxElementTotalZerosChromaDC (cDecoder264* decoder, sSyntaxElement* se) {

  //{{{
  static const uint8_t lentab[3][TOTRUN_NUM][16] = {
    //YUV420
   {{ 1,2,3,3},
    { 1,2,2},
    { 1,1}},

    //YUV422
   {{ 1,3,3,4,4,4,5,5},
    { 3,2,3,3,3,3,3},
    { 3,3,2,2,3,3},
    { 3,2,2,2,3},
    { 2,2,2,2},
    { 2,2,1},
    { 1,1}},

    //YUV444
   {{ 1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9},
    { 3,3,3,3,3,4,4,4,4,5,5,6,6,6,6},
    { 4,3,3,3,4,4,3,3,4,5,5,6,5,6},
    { 5,3,4,4,3,3,3,4,3,4,5,5,5},
    { 4,4,4,3,3,3,3,3,4,5,4,5},
    { 6,5,3,3,3,3,3,3,4,3,6},
    { 6,5,3,3,3,2,3,4,3,6},
    { 6,4,5,3,2,2,3,3,6},
    { 6,6,4,2,2,3,2,5},
    { 5,5,3,2,2,2,4},
    { 4,4,3,3,1,3},
    { 4,4,2,1,3},
    { 3,3,1,2},
    { 2,2,1},
    { 1,1}}
  };
  //}}}
  //{{{
  static const uint8_t codtab[3][TOTRUN_NUM][16] = {
    //YUV420
   {{ 1,1,1,0},
    { 1,1,0},
    { 1,0}},

    //YUV422
   {{ 1,2,3,2,3,1,1,0},
    { 0,1,1,4,5,6,7},
    { 0,1,1,2,6,7},
    { 6,0,1,2,7},
    { 0,1,2,3},
    { 0,1,1},
    { 0,1}},

    //YUV444
   {{1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1},
    {7,6,5,4,3,5,4,3,2,3,2,3,2,1,0},
    {5,7,6,5,4,3,4,3,2,3,2,1,1,0},
    {3,7,5,4,6,5,4,3,3,2,2,1,0},
    {5,4,3,7,6,5,4,3,2,1,1,0},
    {1,1,7,6,5,4,3,2,1,1,0},
    {1,1,5,4,3,3,2,1,1,0},
    {1,1,1,3,3,2,2,1,0},
    {1,0,1,3,2,1,1,1,},
    {1,0,1,3,2,1,1,},
    {0,1,1,2,1,3},
    {0,1,1,1,1},
    {0,1,1,1},
    {0,1,1},
    {0,1}}
  };
  //}}}

  int code;
  int yuv = decoder->activeSps->chromaFormatIdc - 1;
  int vlcnum = se->value1;
  int retval = code2d(se, this, &lentab[yuv][vlcnum][0], &codtab[yuv][vlcnum][0], 16, 1, &code);
  if (retval)
    cDecoder264::error ("failed to find Total Zeros");

  return retval;
  }
//}}}
//{{{
int sBitStream::readSyntaxElementRun (sSyntaxElement* se) {

  //{{{
  static const uint8_t lentab[TOTRUN_NUM][16] = {
    {1,1},
    {1,2,2},
    {2,2,2,2},
    {2,2,2,3,3},
    {2,2,3,3,3,3},
    {2,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,4,5,6,7,8,9,10,11},
  };
  //}}}
  //{{{
  static const uint8_t codtab[TOTRUN_NUM][16] = {
    {1,0},
    {1,1,0},
    {3,2,1,0},
    {3,2,1,1,0},
    {3,2,3,2,1,0},
    {3,0,1,3,2,5,4},
    {7,6,5,4,3,2,1,1,1,1,1,1,1,1,1},
  };
  //}}}

  int code;
  int vlcnum = se->value1;
  int retval = code2d (se, this, &lentab[vlcnum][0], &codtab[vlcnum][0], 16, 1, &code);
  if (retval)
    cDecoder264::error ("failed to find Run");

  return retval;
  }
//}}}
