//{{{  includes
#include "global.h"
#include "elements.h"

#include "vlc.h"
//}}}

//{{{
int read_ue_v (char* tracestring, sBitstream* bitstream) {

  sSyntaxElement symbol = {.value1 = 0 };
  symbol.type = SE_HEADER;
  symbol.mapping = linfo_ue;
  readsSyntaxElement_VLC (&symbol, bitstream);

  if (gDecoder->p_Inp->vlcDebug)
    printf ("read %s %d\n", tracestring, symbol.value1);

  return symbol.value1;
  }
//}}}
//{{{
int read_se_v (char* tracestring, sBitstream* bitstream) {

  sSyntaxElement symbol = {.value1 = 0 };
  symbol.type = SE_HEADER;
  symbol.mapping = linfo_se;
  readsSyntaxElement_VLC (&symbol, bitstream);

  if (gDecoder->p_Inp->vlcDebug)
    printf ("read %s %d\n", tracestring, symbol.value1);

  return symbol.value1;
  }
//}}}
//{{{
int read_u_v (int LenInBits, char* tracestring, sBitstream* bitstream) {

  sSyntaxElement symbol = {.value1 = 0 };
  symbol.inf = 0;
  symbol.type = SE_HEADER;
  symbol.mapping = linfo_ue;
  symbol.len = LenInBits;
  readsSyntaxElement_FLC (&symbol, bitstream);

  if (gDecoder->p_Inp->vlcDebug)
    printf ("read %s %d\n", tracestring, symbol.inf);

  return symbol.inf;
  }
//}}}
//{{{
int read_i_v (int LenInBits, char* tracestring, sBitstream* bitstream) {

  sSyntaxElement symbol = {.value1 = 0 };
  symbol.inf = 0;
  symbol.type = SE_HEADER;
  symbol.mapping = linfo_ue;
  symbol.len = LenInBits;
  readsSyntaxElement_FLC (&symbol, bitstream);

  // can be negative
  symbol.inf = -( symbol.inf & (1 << (LenInBits - 1)) ) | symbol.inf;

  if (gDecoder->p_Inp->vlcDebug)
    printf ("read %s %d\n", tracestring, symbol.inf);

  return symbol.inf;
  }
//}}}
//{{{
Boolean read_u_1 (char* tracestring, sBitstream* bitstream) {
  return (Boolean)read_u_v (1, tracestring, bitstream);
  }
//}}}

//{{{
void linfo_ue (int len, int info, int* value1, int* dummy) {
  *value1 = (int) (((unsigned int) 1 << (len >> 1)) + (unsigned int) (info) - 1);
  }
//}}}
//{{{
void linfo_se (int len,  int info, int* value1, int* dummy) {

  unsigned int n = ((unsigned int) 1 << (len >> 1)) + (unsigned int) info - 1;
  *value1 = (n + 1) >> 1;

  // lsb is signed bit
  if ((n & 0x01) == 0)
    *value1 = -*value1;
  }
//}}}
//{{{
void linfo_cbp_intra_normal (int len, int info,int* cbp, int* dummy) {

  int cbp_idx;
  linfo_ue (len, info, &cbp_idx, dummy);
  *cbp = NCBP[1][cbp_idx][0];
  }
//}}}
//{{{
void linfo_cbp_intra_other (int len, int info,int* cbp, int* dummy) {

  int cbp_idx;
  linfo_ue(len, info, &cbp_idx, dummy);
  *cbp = NCBP[0][cbp_idx][0];
  }
//}}}
//{{{
void linfo_cbp_inter_normal (int len, int info, int* cbp, int* dummy) {

  int cbp_idx;
  linfo_ue (len, info, &cbp_idx, dummy);
  *cbp = NCBP[1][cbp_idx][1];
  }
//}}}
//{{{
void linfo_cbp_inter_other (int len, int info, int* cbp, int *dummy) {

  int cbp_idx;
  linfo_ue (len, info, &cbp_idx, dummy);
  *cbp = NCBP[0][cbp_idx][1];
  }
//}}}
//{{{
void linfo_levrun_inter (int len, int info, int* level, int* irun) {

  if (len <= 9) {
    int l2 = imax(0,(len >> 1)-1);
    int inf = info >> 1;
    *level = NTAB1[l2][inf][0];
    *irun = NTAB1[l2][inf][1];
    if ((info & 0x01) == 1)
      *level = -*level;                   // make sign
    }
  else {
    // if len > 9, skip using the array
    *irun = (info & 0x1e) >> 1;
    *level = LEVRUN1[*irun] + (info >> 5) + ( 1 << ((len >> 1) - 5));
    if ((info & 0x01) == 1)
      *level = -*level;
    }

  if (len == 1) // EOB
    *level = 0;
  }
//}}}
//{{{
void linfo_levrun_c2x2 (int len, int info, int* level, int* irun) {

  if (len <= 5) {
    int l2 = imax(0, (len >> 1) - 1);
    int inf = info >> 1;
    *level = NTAB3[l2][inf][0];
    *irun = NTAB3[l2][inf][1];
    if ((info & 0x01) == 1)
      *level = -*level;                 // make sign
    }
  else {
    // if len > 5, skip using the array
    *irun  = (info & 0x06) >> 1;
    *level = LEVRUN3[*irun] + (info >> 3) + (1 << ((len >> 1) - 3));
    if ((info & 0x01) == 1)
      *level = -*level;
    }

  if (len == 1) // EOB
    *level = 0;
  }
//}}}

//{{{
int readsSyntaxElement_VLC (sSyntaxElement* sym, sBitstream* currStream) {

  sym->len =  GetVLCSymbol (currStream->streamBuffer, currStream->frame_bitoffset,
                            &(sym->inf), currStream->bitstream_length);
  if (sym->len == -1)
    return -1;

  currStream->frame_bitoffset += sym->len;
  sym->mapping (sym->len, sym->inf, &(sym->value1), &(sym->value2));

  return 1;
}
//}}}
//{{{
int readsSyntaxElement_UVLC (sMacroblock* currMB, sSyntaxElement* sym, struct DataPartition* dP) {
  return (readsSyntaxElement_VLC(sym, dP->bitstream));
  }
//}}}
//{{{
int readsSyntaxElement_Intra4x4PredictionMode (sSyntaxElement* sym, sBitstream* currStream) {

  sym->len = GetVLCSymbol_IntraMode (currStream->streamBuffer, currStream->frame_bitoffset, &(sym->inf), currStream->bitstream_length);
  if (sym->len == -1)
    return -1;

  currStream->frame_bitoffset += sym->len;
  sym->value1       = (sym->len == 1) ? -1 : sym->inf;

  return 1;
  }
//}}}
//{{{
int GetVLCSymbol_IntraMode (byte buffer[], int totbitoffset,int* info, int bytecount) {

  int byteoffset = (totbitoffset >> 3);        // byte from start of buffer
  int bitoffset   = (7 - (totbitoffset & 0x07)); // bit from start of byte
  byte *cur_byte  = &(buffer[byteoffset]);
  int ctr_bit     = (*cur_byte & (0x01 << bitoffset));      // control bit for current bit posision

  //First bit
  if (ctr_bit) {
    *info = 0;
    return 1;
    }

  if (byteoffset >= bytecount)
    return -1;
  else {
    int inf = (*(cur_byte) << 8) + *(cur_byte + 1);
    inf <<= (sizeof(byte) * 8) - bitoffset;
    inf = inf & 0xFFFF;
    inf >>= (sizeof(byte) * 8) * 2 - 3;
    *info = inf;

    return 4;           // return absolute offset in bit from start of frame
    }
  }
//}}}
//{{{
int more_rbsp_data (byte buffer[], int totbitoffset,int bytecount) {

  // there is more until we're in the last byte
  long byteoffset = (totbitoffset >> 3);      // byte from start of buffer
  if (byteoffset < (bytecount - 1))
    return TRUE;
  else {
    int bitoffset   = (7 - (totbitoffset & 0x07));      // bit from start of byte
    byte *cur_byte  = &(buffer[byteoffset]);
    // read one bit
    int ctr_bit     = ctr_bit = ((*cur_byte)>> (bitoffset--)) & 0x01;      // control bit for current bit posision

    // a stop bit has to be one
    if (ctr_bit==0)
      return TRUE;
    else {
      int cnt = 0;
      while (bitoffset>=0 && !cnt)
        cnt |= ((*cur_byte)>> (bitoffset--)) & 0x01;   // set up control bit
      return (cnt);
      }
    }
  }
//}}}
//{{{
int uvlc_startcode_follows (sSlice* currSlice, int dummy) {

  byte dp_Nr = assignSE2partition[currSlice->dp_mode][SE_MBTYPE];
  sDataPartition* dP = &(currSlice->partArr[dp_Nr]);
  sBitstream* currStream = dP->bitstream;
  byte* buf = currStream->streamBuffer;

  return !(more_rbsp_data (buf, currStream->frame_bitoffset,currStream->bitstream_length));
  }
//}}}
//{{{
int GetVLCSymbol (byte buffer[], int totbitoffset, int* info, int bytecount) {

  long byteoffset = (totbitoffset >> 3);         // byte from start of buffer
  int bitoffset  = (7 - (totbitoffset & 0x07)); // bit from start of byte
  int bitcounter = 1;
  int len        = 0;
  byte* cur_byte  = &(buffer[byteoffset]);
  int ctr_bit    = ((*cur_byte) >> (bitoffset)) & 0x01;  // control bit for current bit posision

  while (ctr_bit == 0) {
    // find leading 1 bit
    len++;
    bitcounter++;
    bitoffset--;
    bitoffset &= 0x07;
    cur_byte  += (bitoffset == 7);
    byteoffset+= (bitoffset == 7);
    ctr_bit    = ((*cur_byte) >> (bitoffset)) & 0x01;
    }

  if (byteoffset + ((len + 7) >> 3) > bytecount)
    return -1;

  int inf = 0;                          // shortest possible code is 1, then info is always 0
  while (len--) {
    bitoffset --;
    bitoffset &= 0x07;
    cur_byte  += (bitoffset == 7);
    bitcounter++;
    inf <<= 1;
    inf |= ((*cur_byte) >> (bitoffset)) & 0x01;
    }

  *info = inf;
  return bitcounter;           // return absolute offset in bit from start of frame
  }
//}}}

//{{{
static inline int ShowBitsThres (int inf, int numbits) {
  return ((inf) >> ((sizeof(byte) * 24) - (numbits)));
  }
//}}}
//{{{
static int code_from_bitstream_2d (sSyntaxElement* sym, sBitstream* currStream,
                                   const byte *lentab, const byte *codtab,
                                   int tabwidth, int tabheight, int *code) {

  const byte* len = &lentab[0], *cod = &codtab[0];

  int* frame_bitoffset = &currStream->frame_bitoffset;
  byte* buf = &currStream->streamBuffer[*frame_bitoffset >> 3];

  // Apply bitoffset to three bytes (maximum that may be traversed by ShowBitsThres)
  // Even at the end of a stream we will still be pulling out of allocated memory as alloc is done by MAX_CODED_FRAME_SIZE
  unsigned int inf = ((*buf) << 16) + (*(buf + 1) << 8) + *(buf + 2);
  // Offset is constant so apply before extracting different numbers of bits
  inf <<= (*frame_bitoffset & 0x07);
  // Arithmetic shift so wipe any sign which may be extended inside ShowBitsThres
  inf  &= 0xFFFFFF;

  // this VLC decoding method is not optimized for speed
  for (int j = 0; j < tabheight; j++) {
    for (int i = 0; i < tabwidth; i++) {
      if ((*len == 0) || (ShowBitsThres(inf, (int) *len) != *cod)) {
        ++len;
        ++cod;
        }
      else {
        sym->len = *len;
        *frame_bitoffset += *len; // move bitstream pointer
        *code = *cod;
        sym->value1 = i;
        sym->value2 = j;
        return 0;                 // found code and return
        }
      }
    }

  return -1;  // failed to find code
  }
//}}}

//{{{
int readsSyntaxElement_FLC (sSyntaxElement* sym, sBitstream* currStream)
{
  int BitstreamLengthInBits  = (currStream->bitstream_length << 3) + 7;

  if ((GetBits(currStream->streamBuffer, currStream->frame_bitoffset, &(sym->inf), BitstreamLengthInBits, sym->len)) < 0)
    return -1;

  sym->value1 = sym->inf;
  currStream->frame_bitoffset += sym->len; // move bitstream pointer

  return 1;
}
//}}}
//{{{
int readsSyntaxElement_NumCoeffTrailingOnes (sSyntaxElement* sym,
                                           sBitstream *currStream,
                                           char *type)
{
  int frame_bitoffset        = currStream->frame_bitoffset;
  int BitstreamLengthInBytes = currStream->bitstream_length;
  int BitstreamLengthInBits  = (BitstreamLengthInBytes << 3) + 7;
  byte *buf                  = currStream->streamBuffer;

  static const byte lentab[3][4][17] =
  {
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

  static const byte codtab[3][4][17] =
  {
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

  int retval = 0, code;
  int vlcnum = sym->value1;
  // vlcnum is the index of Table used to code coeff_token
  // vlcnum==3 means (8<=nC) which uses 6bit FLC

  if (vlcnum == 3)
  {
    // read 6 bit FLC
    //code = ShowBits(buf, frame_bitoffset, BitstreamLengthInBytes, 6);
    code = ShowBits(buf, frame_bitoffset, BitstreamLengthInBits, 6);
    currStream->frame_bitoffset += 6;
    sym->value2 = (code & 3);
    sym->value1 = (code >> 2);

    if (!sym->value1 && sym->value2 == 3)
    {
      // #c = 0, #t1 = 3 =>  #c = 0
      sym->value2 = 0;
    }
    else
      sym->value1++;

    sym->len = 6;
  }
  else
  {
    //retval = code_from_bitstream_2d(sym, currStream, &lentab[vlcnum][0][0], &codtab[vlcnum][0][0], 17, 4, &code);
    retval = code_from_bitstream_2d(sym, currStream, lentab[vlcnum][0], codtab[vlcnum][0], 17, 4, &code);
    if (retval)
    {
      printf("ERROR: failed to find NumCoeff/TrailingOnes\n");
      exit(-1);
    }
  }

  return retval;
}
//}}}
//{{{
int readsSyntaxElement_NumCoeffTrailingOnesChromaDC (sVidParam* vidParam, sSyntaxElement* sym, sBitstream* currStream)
{
  static const byte lentab[3][4][17] =
  {
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

  static const byte codtab[3][4][17] =
  {
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

  int code;
  int yuv = vidParam->active_sps->chroma_format_idc - 1;
  int retval = code_from_bitstream_2d(sym, currStream, &lentab[yuv][0][0], &codtab[yuv][0][0], 17, 4, &code);

  if (retval)
  {
    printf("ERROR: failed to find NumCoeff/TrailingOnes ChromaDC\n");
    exit(-1);
  }

  return retval;
}
//}}}
//{{{
int readsSyntaxElement_Level_VLC0 (sSyntaxElement* sym, sBitstream* currStream)
{
  int frame_bitoffset        = currStream->frame_bitoffset;
  int BitstreamLengthInBytes = currStream->bitstream_length;
  int BitstreamLengthInBits  = (BitstreamLengthInBytes << 3) + 7;
  byte *buf                  = currStream->streamBuffer;
  int len = 1, sign = 0, level = 0, code = 1;

  while (!ShowBits(buf, frame_bitoffset++, BitstreamLengthInBits, 1))
    len++;

  if (len < 15)
  {
    sign  = (len - 1) & 1;
    level = ((len - 1) >> 1) + 1;
  }
  else if (len == 15)
  {
    // escape code
    code <<= 4;
    code |= ShowBits(buf, frame_bitoffset, BitstreamLengthInBits, 4);
    len  += 4;
    frame_bitoffset += 4;
    sign = (code & 0x01);
    level = ((code >> 1) & 0x07) + 8;
  }
  else if (len >= 16)
  {
    // escape code
    int addbit = (len - 16);
    int offset = (2048 << addbit) - 2032;
    len   -= 4;
    code   = ShowBits(buf, frame_bitoffset, BitstreamLengthInBits, len);
    sign   = (code & 0x01);
    frame_bitoffset += len;
    level = (code >> 1) + offset;

    code |= (1 << (len)); // for display purpose only
    len += addbit + 16;
 }

  sym->inf = (sign) ? -level : level ;
  sym->len = len;

  currStream->frame_bitoffset = frame_bitoffset;
  return 0;
}
//}}}
//{{{
int readsSyntaxElement_Level_VLCN (sSyntaxElement* sym, int vlc, sBitstream* currStream)
{
  int frame_bitoffset        = currStream->frame_bitoffset;
  int BitstreamLengthInBytes = currStream->bitstream_length;
  int BitstreamLengthInBits  = (BitstreamLengthInBytes << 3) + 7;
  byte *buf                  = currStream->streamBuffer;

  int levabs, sign;
  int len = 1;
  int code = 1, sb;

  int shift = vlc - 1;

  // read pre zeros
  while (!ShowBits(buf, frame_bitoffset ++, BitstreamLengthInBits, 1))
    len++;

  frame_bitoffset -= len;

  if (len < 16) {
    levabs = ((len - 1) << shift) + 1;

    // read (vlc-1) bits -> suffix
    if (shift) {
      sb =  ShowBits(buf, frame_bitoffset + len, BitstreamLengthInBits, shift);
      code = (code << (shift) )| sb;
      levabs += sb;
      len += (shift);
    }

    // read 1 bit -> sign
    sign = ShowBits(buf, frame_bitoffset + len, BitstreamLengthInBits, 1);
    code = (code << 1)| sign;
    len ++;
  }
  else {
    // escape
    int addbit = len - 5;
    int offset = (1 << addbit) + (15 << shift) - 2047;

    sb = ShowBits(buf, frame_bitoffset + len, BitstreamLengthInBits, addbit);
    code = (code << addbit ) | sb;
    len   += addbit;

    levabs = sb + offset;

    // read 1 bit -> sign
    sign = ShowBits(buf, frame_bitoffset + len, BitstreamLengthInBits, 1);

    code = (code << 1)| sign;

    len++;
  }

  sym->inf = (sign)? -levabs : levabs;
  sym->len = len;

  currStream->frame_bitoffset = frame_bitoffset + len;

  return 0;
}
//}}}
//{{{
int readsSyntaxElement_TotalZeros (sSyntaxElement* sym,  sBitstream *currStream) {

  //{{{
  static const byte lentab[TOTRUN_NUM][16] =
  {

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
  static const byte codtab[TOTRUN_NUM][16] =
  {
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
  int vlcnum = sym->value1;
  int retval = code_from_bitstream_2d(sym, currStream, &lentab[vlcnum][0], &codtab[vlcnum][0], 16, 1, &code);

  if (retval) {
    printf("ERROR: failed to find Total Zeros !cdc\n");
    exit(-1);
    }

  return retval;
  }
//}}}
//{{{
int readsSyntaxElement_TotalZerosChromaDC (sVidParam* vidParam, sSyntaxElement* sym, sBitstream* currStream) {

  //{{{
  static const byte lentab[3][TOTRUN_NUM][16] =
  {
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
  static const byte codtab[3][TOTRUN_NUM][16] =
  {
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
  int yuv = vidParam->active_sps->chroma_format_idc - 1;
  int vlcnum = sym->value1;
  int retval = code_from_bitstream_2d(sym, currStream, &lentab[yuv][vlcnum][0], &codtab[yuv][vlcnum][0], 16, 1, &code);

  if (retval) {
    printf ("ERROR: failed to find Total Zeros\n");
    exit (-1);
    }

  return retval;
  }
//}}}
//{{{
int readsSyntaxElement_Run (sSyntaxElement* sym, sBitstream* currStream)
{
  //{{{
  static const byte lentab[TOTRUN_NUM][16] =
  {
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
  static const byte codtab[TOTRUN_NUM][16] =
  {
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
  int vlcnum = sym->value1;
  int retval = code_from_bitstream_2d (sym, currStream, &lentab[vlcnum][0], &codtab[vlcnum][0], 16, 1, &code);
  if (retval) {
    printf ("ERROR: failed to find Run\n");
    exit (-1);
    }

  return retval;
  }
//}}}

//{{{
int GetBits (byte buffer[],int totbitoffset, int*info, int bitcount, int numbits) {

  if ((totbitoffset + numbits ) > bitcount)
    return -1;

  int bitoffset  = 7 - (totbitoffset & 0x07); // bit from start of byte
  int byteoffset = (totbitoffset >> 3); // byte from start of buffer
  int bitcounter = numbits;
  byte* curbyte  = &(buffer[byteoffset]);

  int inf = 0;
  while (numbits--) {
    inf <<=1;
    inf |= ((*curbyte)>> (bitoffset--)) & 0x01;
    if (bitoffset == -1) {
      // Move onto next byte to get all of numbits
      curbyte++;
      bitoffset = 7;
      }

    // Above conditional could also be avoided using the following:
    // curbyte   -= (bitoffset >> 3);
    // bitoffset &= 0x07;
    }
  *info = inf;

  return bitcounter;           // return absolute offset in bit from start of frame
  }
//}}}
//{{{
int ShowBits (byte buffer[],int totbitoffset, int bitcount, int numbits) {

  if ((totbitoffset + numbits )  > bitcount)
    return -1;

  int bitoffset = 7 - (totbitoffset & 0x07); // bit from start of byte
  int byteoffset = (totbitoffset >> 3); // byte from start of buffer
  byte* curbyte = &(buffer[byteoffset]);

  int inf = 0;
  while (numbits--) {
    inf <<=1;
    inf |= ((*curbyte)>> (bitoffset--)) & 0x01;

    if (bitoffset == -1 ) {
      // Move onto next byte to get all of numbits
      curbyte++;
      bitoffset = 7;
      }
    }

  return inf; // return absolute offset in bit from start of frame
  }
//}}}
