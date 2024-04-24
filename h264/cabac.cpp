//{{{  includes
#include "global.h"
#include "memory.h"

#include "sCabacDecode.h"
#include "cabac.h"
#include "macroblock.h"

#include "../common/cLog.h"

using namespace std;
//}}}
namespace {
  //{{{  tables
  //{{{
  const int16_t maxpos[] = {
    15, 14, 63, 31, 31, 15,  3, 14,  7, 15, 15, 14, 63, 31, 31, 15, 15, 14, 63, 31, 31, 15};
  //}}}
  //{{{
  const int16_t c1isdc[] = {
    1,  0,  1,  1,  1,  1,  1,  0,  1,  1,  1,  0,  1,  1,  1,  1,  1,  0,  1,  1,  1,  1};
  //}}}
  //{{{
  const int16_t type2ctx_bcbp[] = {
    0,  1,  2,  3,  3,  4,  5,  6,  5,  5, 10, 11, 12, 13, 13, 14, 16, 17, 18, 19, 19, 20};
  //}}}
  //{{{
  const int16_t type2ctx_map[] = {
    0,  1,  2,  3,  4,  5,  6,  7,  6,  6, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21}; // 8
  //}}}
  //{{{
  const int16_t type2ctx_last[] = {
    0,  1,  2,  3,  4,  5,  6,  7,  6,  6, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21}; // 8
  //}}}
  //{{{
  const int16_t type2ctx_one[] = {
   0,  1,  2,  3,  3,  4,  5,  6,  5,  5, 10, 11, 12, 13, 13, 14, 16, 17, 18, 19, 19, 20}; // 7
  //}}}
  //{{{
  const int16_t type2ctx_abs[] = {
    0,  1,  2,  3,  3,  4,  5,  6,  5,  5, 10, 11, 12, 13, 13, 14, 16, 17, 18, 19, 19, 20}; // 7
  //}}}
  //{{{
  const int16_t max_c2 [] = {
    4,  4,  4,  4,  4,  4,  3,  4,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4}; // 9
  //}}}

  // position -> ctx for MAP zig-zag scan
  //{{{
  const uint8_t  pos2ctx_map8x8 [] = {
    0,  1,  2,  3,  4,  5,  5,  4,  4,  3,  3,  4,  4,  4,  5,  5,
    4,  4,  4,  4,  3,  3,  6,  7,  7,  7,  8,  9, 10,  9,  8,  7,
    7,  6, 11, 12, 13, 11,  6,  7,  8,  9, 14, 10,  9,  8,  6, 11,
   12, 13, 11,  6,  9, 14, 10,  9, 11, 12, 13, 11 ,14, 10, 12, 14}; // 15 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_map8x4 [] = {
    0,  1,  2,  3,  4,  5,  7,  8,  9, 10, 11,  9,  8,  6,  7,  8,
    9, 10, 11,  9,  8,  6, 12,  8,  9, 10, 11,  9, 13, 13, 14, 14}; // 15 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_map4x4 [] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 14}; // 15 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_map2x4c[] = {
    0,  0,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_map4x4c[] = {
    0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
  //}}}
  //{{{
  const uint8_t* pos2ctx_map[] = {
    pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8, pos2ctx_map8x4,
    pos2ctx_map8x4, pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map4x4,
    pos2ctx_map2x4c, pos2ctx_map4x4c,
    pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8,pos2ctx_map8x4,
    pos2ctx_map8x4, pos2ctx_map4x4,
    pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8,pos2ctx_map8x4,
    pos2ctx_map8x4,pos2ctx_map4x4};
  //}}}

  // interlace scan taken from ABT
  //{{{
  const uint8_t  pos2ctx_map8x8i[] = {
    0,  1,  1,  2,  2,  3,  3,  4,  5,  6,  7,  7,  7,  8,  4,  5,
    6,  9, 10, 10,  8, 11, 12, 11,  9,  9, 10, 10,  8, 11, 12, 11,
    9,  9, 10, 10,  8, 11, 12, 11,  9,  9, 10, 10,  8, 13, 13,  9,
    9, 10, 10,  8, 13, 13,  9,  9, 10, 10, 14, 14, 14, 14, 14, 14}; // 15 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_map8x4i[] = {
    0,  1,  2,  3,  4,  5,  6,  3,  4,  5,  6,  3,  4,  7,  6,  8,
    9,  7,  6,  8,  9, 10, 11, 12, 12, 10, 11, 13, 13, 14, 14, 14}; // 15 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_map4x8i[] = {
    0,  1,  1,  1,  2,  3,  3,  4,  4,  4,  5,  6,  2,  7,  7,  8,
    8,  8,  5,  6,  9, 10, 10, 11, 11, 11, 12, 13, 13, 14, 14, 14}; // 15 CTX
  //}}}
  //{{{
  const uint8_t* pos2ctx_map_int[] = {
    pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
    pos2ctx_map4x8i,pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map4x4,
    pos2ctx_map2x4c, pos2ctx_map4x4c,
    pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
    pos2ctx_map8x4i,pos2ctx_map4x4,
    pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
    pos2ctx_map8x4i,pos2ctx_map4x4};
  //}}}

  // position -> ctx for LAST
  //{{{
  const uint8_t  pos2ctx_last8x8 [] = {
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
    5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8}; //  9 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_last8x4 [] = {
    0,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,
    3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8}; //  9 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_last4x4 [] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15}; // 15 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_last2x4c[] = {
    0,  0,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
  //}}}
  //{{{
  const uint8_t  pos2ctx_last4x4c[] = {
    0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
  //}}}
  //{{{
  const uint8_t* pos2ctx_last[] = {
    pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8, pos2ctx_last8x4,
    pos2ctx_last8x4, pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last4x4,
    pos2ctx_last2x4c, pos2ctx_last4x4c,
    pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8,pos2ctx_last8x4,
    pos2ctx_last8x4, pos2ctx_last4x4,
    pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8,pos2ctx_last8x4,
    pos2ctx_last8x4, pos2ctx_last4x4};
  //}}}
  //}}}
  //{{{
  int setCbp (sMacroBlock* neighbourMb) {

    if (neighbourMb->mbType == IPCM)
      return 1;
    else
      return (int)(neighbourMb->cbp[0].bits & 0x01);
    }
  //}}}
  //{{{
  int setCbpAC (sMacroBlock* neighbourMb, sPixelPos* block) {

    if (neighbourMb->mbType == IPCM)
      return 1;
    else {
      int bit_pos = 1 + (block->y << 2) + block->x;
      return getBit (neighbourMb->cbp[0].bits, bit_pos);
      }
    }
  //}}}

  //{{{
  int readCbpNormal (sMacroBlock* mb, sCabacDecode* cabacDecode, int type) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    sMacroBlock* mbData = slice->mbData;
    sTextureContexts* textureContexts = slice->textureContexts;

    int codedBlockPatternBit = 1;  // always one for 8x8 mode
    if (type == LUMA_16DC) {
      //{{{  luma_16dc
      int left_bit = 1;
      sPixelPos block_a;
      get4x4NeighbourBase (mb, -1,  0, decoder->mbSize[eLuma], &block_a);
      if (block_a.ok)
        left_bit = setCbp (&mbData[block_a.mbIndex]);

      int upper_bit = 1;
      sPixelPos block_b;
      get4x4NeighbourBase (mb,  0, -1, decoder->mbSize[eLuma], &block_b);
      if (block_b.ok)
        upper_bit = setCbp (&mbData[block_b.mbIndex]);

      // encode symbol
      int ctx = 2 * upper_bit + left_bit;
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

      // set bits for current block ---
      if (codedBlockPatternBit)
        mb->cbp[0].bits |= 1;
      }
      //}}}
    else if (type == LUMA_16AC) {
      //{{{  luma_16ac
      int j = mb->subblockY;
      int i = mb->subblockX;

      int bit = 1;
      int default_bit = (mb->isIntraBlock ? 1 : 0);

      int upper_bit = default_bit;
      sPixelPos block_b;
      get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);
      if (block_b.ok)
        upper_bit = setCbpAC (&mbData[block_b.mbIndex], &block_b);

      int left_bit = default_bit;
      sPixelPos block_a;
      get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
      if (block_a.ok)
        left_bit = setCbpAC (&mbData[block_a.mbIndex], &block_a);

      // encode symbol
      int ctx = 2 * upper_bit + left_bit;
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);
      if (codedBlockPatternBit) {
        // set bits for current block ---
        bit = 1 + j + (i >> 2);
        mb->cbp[0].bits |= i64power2(bit);
        }
      }
      //}}}
    else if (type == LUMA_8x4) {
      //{{{  luma8x4
      int j = mb->subblockY;
      int i = mb->subblockX;

      int bit = 1;
      int default_bit = (mb->isIntraBlock ? 1 : 0);

      int upper_bit = default_bit;
      sPixelPos block_b;
      get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);
      if (block_b.ok)
        upper_bit = setCbpAC (&mbData[block_b.mbIndex], &block_b);

      int left_bit = default_bit;
      sPixelPos block_a;
      get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
      if (block_a.ok)
        left_bit = setCbpAC (&mbData[block_a.mbIndex], &block_a);

      int ctx = 2 * upper_bit + left_bit;
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

      if (codedBlockPatternBit) {
        // set bits for current block ---
        bit = 1 + j + (i >> 2);
        mb->cbp[0].bits |= ((int64_t) 0x03 << bit   );
        }
      }
      //}}}
    else if (type == LUMA_4x8) {
      //{{{  luma4x8
      int j           = mb->subblockY;
      int i           = mb->subblockX;
      int bit         = 1;
      int default_bit = (mb->isIntraBlock ? 1 : 0);
      int upper_bit   = default_bit;
      int left_bit    = default_bit;
      int ctx;

      sPixelPos block_a, block_b;

      get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
      get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);

      // get bits from neighboring blocks ---
      if (block_b.ok)
        upper_bit = setCbpAC (&mbData[block_b.mbIndex], &block_b);

      if (block_a.ok)
        left_bit = setCbpAC (&mbData[block_a.mbIndex], &block_a);

      ctx = 2 * upper_bit + left_bit;
      // encode symbol =====
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

      if (codedBlockPatternBit) {
        // set bits for current block ---
        bit = 1 + j + (i >> 2);
        mb->cbp[0].bits   |= ((int64_t) 0x11 << bit   );
        }
      }
      //}}}
    else if (type==LUMA_4x4) {
      //{{{  luma4x4
      int j = mb->subblockY;
      int i = mb->subblockX;
      int bit = 1;
      int default_bit = (mb->isIntraBlock ? 1 : 0);
      int upper_bit = default_bit;
      int left_bit = default_bit;

      sPixelPos block_a;
      get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
      sPixelPos block_b;
      get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);

      // get bits from neighboring blocks
      if (block_b.ok)
        upper_bit = setCbpAC (&mbData[block_b.mbIndex], &block_b);
      if (block_a.ok)
        left_bit = setCbpAC (&mbData[block_a.mbIndex], &block_a);

      int ctx = 2 * upper_bit + left_bit;
      // encode symbol =====
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

      if (codedBlockPatternBit) {
        // set bits for current block ---
        bit = 1 + j + (i >> 2);
        mb->cbp[0].bits   |= i64power2(bit);
        }
      }
      //}}}
    else if (type == LUMA_8x8) {
      //{{{  luma8x8
      int j = mb->subblockY;
      int i = mb->subblockX;

      // set bits for current block ---
      int bit = 1 + j + (i >> 2);
      mb->cbp[0].bits |= ((int64_t) 0x33 << bit   );
      }
      //}}}
    else if (type == CHROMA_DC || type == CHROMA_DC_2x4 || type == CHROMA_DC_4x4) {
      //{{{  chromaDC  2x4 4x4
      int u_dc = (!mb->isVblock);
      int j = 0;
      int i = 0;

      int bit = (u_dc ? 17 : 18);
      int default_bit = (mb->isIntraBlock ? 1 : 0);

      int left_bit = default_bit;
      sPixelPos block_a;
      get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eChroma], &block_a);
      if (block_a.ok) {
        if (mbData[block_a.mbIndex].mbType==IPCM)
          left_bit = 1;
        else
          left_bit = getBit (mbData[block_a.mbIndex].cbp[0].bits, bit);
        }

      int upper_bit = default_bit;
      sPixelPos block_b;
      get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eChroma], &block_b);
      if (block_b.ok) {
        if (mbData[block_b.mbIndex].mbType==IPCM)
          upper_bit = 1;
        else
          upper_bit = getBit (mbData[block_b.mbIndex].cbp[0].bits, bit);
        }

      int ctx = 2 * upper_bit + left_bit;
      // encode symbol =====
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

      if (codedBlockPatternBit) {
        // set bits for current block ---
        bit = u_dc ? 17 : 18;
        mb->cbp[0].bits |= i64power2(bit);
        }
      }
      //}}}
    else {
      //{{{  others
      int u_ac = !mb->isVblock;
      int j = mb->subblockY;
      int i = mb->subblockX;

      int bit = u_ac ? 19 : 35;
      int default_bit = (mb->isIntraBlock ? 1 : 0);

      int left_bit = default_bit;
      sPixelPos block_a;
      get4x4NeighbourBase (mb, i - 1, j, decoder->mbSize[eChroma], &block_a);
      if (block_a.ok) {
        if (mbData[block_a.mbIndex].mbType == IPCM)
          left_bit = 1;
        else {
          int bit_pos_a = 4*block_a.y + block_a.x;
          left_bit = getBit(mbData[block_a.mbIndex].cbp[0].bits,bit + bit_pos_a);
          }
        }

      int upper_bit = default_bit;
      sPixelPos block_b;
      get4x4NeighbourBase (mb, i, j - 1, decoder->mbSize[eChroma], &block_b);
      if (block_b.ok) {
        if(mbData[block_b.mbIndex].mbType == IPCM)
          upper_bit = 1;
        else {
          int bit_pos_b = 4*block_b.y + block_b.x;
          upper_bit = getBit (mbData[block_b.mbIndex].cbp[0].bits, bit + bit_pos_b);
          }
        }

      int ctx = 2 * upper_bit + left_bit;

      // encode symbol
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);
      if (codedBlockPatternBit) {
        // set bits for current block ---
        bit = (u_ac ? 19 + j + (i >> 2) : 35 + j + (i >> 2));
        mb->cbp[0].bits |= i64power2(bit);
        }
      }
      //}}}

    return codedBlockPatternBit;
    }
  //}}}
  //{{{
  int readCbp444 (sMacroBlock* mb, sCabacDecode*  cabacDecode, int type) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;
    sPicture* picture = slice->picture;
    sTextureContexts* textureContexts = slice->textureContexts;
    sMacroBlock* mbData = slice->mbData;

    int y_ac        = (type==LUMA_16AC || type==LUMA_8x8 || type==LUMA_8x4 || type==LUMA_4x8 || type==LUMA_4x4
                      || type==CB_16AC || type==CB_8x8 || type==CB_8x4 || type==CB_4x8 || type==CB_4x4
                      || type==CR_16AC || type==CR_8x8 || type==CR_8x4 || type==CR_4x8 || type==CR_4x4);
    int y_dc        = (type==LUMA_16DC || type==CB_16DC || type==CR_16DC);
    int u_ac        = (type==CHROMA_AC && !mb->isVblock);
    int v_ac        = (type==CHROMA_AC &&  mb->isVblock);
    int chroma_dc   = (type==CHROMA_DC || type==CHROMA_DC_2x4 || type==CHROMA_DC_4x4);
    int u_dc        = (chroma_dc && !mb->isVblock);
    int v_dc        = (chroma_dc &&  mb->isVblock);

    int j           = (y_ac || u_ac || v_ac ? mb->subblockY : 0);
    int i           = (y_ac || u_ac || v_ac ? mb->subblockX : 0);

    int bit         = (y_dc ? 0 : y_ac ? 1 : u_dc ? 17 : v_dc ? 18 : u_ac ? 19 : 35);
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit   = default_bit;
    int left_bit    = default_bit;

    int codedBlockPatternBit     = 1;  // always one for 8x8 mode
    int ctx;
    int bit_pos_a   = 0;
    int bit_pos_b   = 0;

    sPixelPos block_a, block_b;
    if (y_ac) {
      //{{{  yAC
      get4x4Neighbour (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
      get4x4Neighbour (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);
      if (block_a.ok)
          bit_pos_a = 4*block_a.y + block_a.x;
      if (block_b.ok)
        bit_pos_b = 4*block_b.y + block_b.x;
      }
      //}}}
    else if (y_dc) {
      //{{{  yDC
      get4x4Neighbour (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
      get4x4Neighbour (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);
      }
      //}}}
    else if (u_ac || v_ac) {
      //{{{  uAC, vAC
      get4x4Neighbour (mb, i - 1, j    , decoder->mbSize[eChroma], &block_a);
      get4x4Neighbour (mb, i    , j - 1, decoder->mbSize[eChroma], &block_b);
      if (block_a.ok)
        bit_pos_a = 4*block_a.y + block_a.x;
      if (block_b.ok)
        bit_pos_b = 4*block_b.y + block_b.x;
      }
      //}}}
    else {
      //{{{  uDC, vDC
      get4x4Neighbour (mb, i - 1, j    , decoder->mbSize[eChroma], &block_a);
      get4x4Neighbour (mb, i    , j - 1, decoder->mbSize[eChroma], &block_b);
      }
      //}}}

    if (picture->chromaFormatIdc != YUV444) {
      if (type != LUMA_8x8) {
        //{{{  get bits from neighboring blocks ---
        if (block_b.ok) {
          if(mbData[block_b.mbIndex].mbType==IPCM)
            upper_bit = 1;
          else
            upper_bit = getBit (mbData[block_b.mbIndex].cbp[0].bits, bit + bit_pos_b);
          }

        if (block_a.ok) {
          if (mbData[block_a.mbIndex].mbType==IPCM)
            left_bit = 1;
          else
            left_bit = getBit (mbData[block_a.mbIndex].cbp[0].bits, bit + bit_pos_a);
          }


        ctx = 2 * upper_bit + left_bit;
        // encode symbol =====
        codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);
        }
        //}}}
      }
    else if (decoder->coding.isSeperateColourPlane != 0) {
      if (type != LUMA_8x8) {
        //{{{  get bits from neighbouring blocks ---
        if (block_b.ok) {
          if(mbData[block_b.mbIndex].mbType == IPCM)
            upper_bit = 1;
          else
            upper_bit = getBit (mbData[block_b.mbIndex].cbp[0].bits,bit+bit_pos_b);
          }


        if (block_a.ok) {
          if(mbData[block_a.mbIndex].mbType==IPCM)
            left_bit = 1;
          else
            left_bit = getBit (mbData[block_a.mbIndex].cbp[0].bits,bit+bit_pos_a);
          }

        ctx = 2 * upper_bit + left_bit;
        codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);
        }
        //}}}
      }
    else {
      if (block_b.ok) {
        //{{{  block b
        if (mbData[block_b.mbIndex].mbType==IPCM)
          upper_bit = 1;
        else if ((type == LUMA_8x8 || type == CB_8x8 || type == CR_8x8) &&
                 !mbData[block_b.mbIndex].lumaTransformSize8x8flag)
          upper_bit = 0;
        else {
          if (type == LUMA_8x8)
            upper_bit = getBit (mbData[block_b.mbIndex].cbp[0].bits8x8, bit + bit_pos_b);
          else if (type == CB_8x8)
            upper_bit = getBit (mbData[block_b.mbIndex].cbp[1].bits8x8, bit + bit_pos_b);
          else if (type == CR_8x8)
            upper_bit = getBit (mbData[block_b.mbIndex].cbp[2].bits8x8, bit + bit_pos_b);
          else if ((type == CB_4x4)||(type == CB_4x8) || (type == CB_8x4) || (type == CB_16AC) || (type == CB_16DC))
            upper_bit = getBit (mbData[block_b.mbIndex].cbp[1].bits,bit+bit_pos_b);
          else if ((type == CR_4x4)||(type == CR_4x8) || (type == CR_8x4) || (type == CR_16AC)||(type == CR_16DC))
            upper_bit = getBit (mbData[block_b.mbIndex].cbp[2].bits,bit+bit_pos_b);
          else
            upper_bit = getBit (mbData[block_b.mbIndex].cbp[0].bits,bit+bit_pos_b);
          }
        }
        //}}}
      if (block_a.ok) {
        //{{{  block a
        if (mbData[block_a.mbIndex].mbType == IPCM)
          left_bit = 1;
        else if ((type == LUMA_8x8 || type == CB_8x8 || type == CR_8x8) &&
                 !mbData[block_a.mbIndex].lumaTransformSize8x8flag)
          left_bit = 0;
        else {
          if (type == LUMA_8x8)
            left_bit = getBit (mbData[block_a.mbIndex].cbp[0].bits8x8,bit+bit_pos_a);
          else if (type == CB_8x8)
            left_bit = getBit (mbData[block_a.mbIndex].cbp[1].bits8x8,bit+bit_pos_a);
          else if (type == CR_8x8)
            left_bit = getBit (mbData[block_a.mbIndex].cbp[2].bits8x8,bit+bit_pos_a);
          else if ((type == CB_4x4) || (type == CB_4x8) ||
                   (type == CB_8x4) || (type == CB_16AC) || (type == CB_16DC))
            left_bit = getBit (mbData[block_a.mbIndex].cbp[1].bits,bit+bit_pos_a);
          else if ((type == CR_4x4) || (type==CR_4x8) ||
                   (type == CR_8x4) || (type == CR_16AC) || (type == CR_16DC))
            left_bit = getBit (mbData[block_a.mbIndex].cbp[2].bits,bit+bit_pos_a);
          else
            left_bit = getBit (mbData[block_a.mbIndex].cbp[0].bits,bit+bit_pos_a);
          }
        }
        //}}}
      ctx = 2 * upper_bit + left_bit;
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);
      }

    // set bits for current block
    bit = (y_dc ? 0 : y_ac ? 1 + j + (i >> 2) : u_dc ? 17 : v_dc ? 18 : u_ac ? 19 + j + (i >> 2) : 35 + j + (i >> 2));
    if (codedBlockPatternBit) {
      //{{{  get cbp
      sCodedBlockPattern * cbp = mb->cbp;
      if (type == LUMA_8x8) {
        cbp[0].bits |= ((int64_t) 0x33 << bit);
        if (picture->chromaFormatIdc == YUV444)
          cbp[0].bits8x8 |= ((int64_t) 0x33 << bit);
        }
      else if (type == CB_8x8) {
        cbp[1].bits8x8 |= ((int64_t) 0x33 << bit);
        cbp[1].bits |= ((int64_t) 0x33 << bit);
        }
      else if (type == CR_8x8) {
        cbp[2].bits8x8 |= ((int64_t) 0x33 << bit);
        cbp[2].bits |= ((int64_t) 0x33 << bit);
        }
      else if (type == LUMA_8x4)
        cbp[0].bits |= ((int64_t) 0x03 << bit);
      else if (type == CB_8x4)
        cbp[1].bits |= ((int64_t) 0x03 << bit);
      else if (type == CR_8x4)
        cbp[2].bits |= ((int64_t) 0x03 << bit);
      else if (type == LUMA_4x8)
        cbp[0].bits |= ((int64_t) 0x11<< bit);
      else if (type == CB_4x8)
        cbp[1].bits |= ((int64_t)0x11<< bit);
      else if (type == CR_4x8)
        cbp[2].bits |= ((int64_t)0x11<< bit);
      else if ((type == CB_4x4) || (type == CB_16AC) || (type == CB_16DC))
        cbp[1].bits |= i64power2 (bit);
      else if ((type == CR_4x4) || (type == CR_16AC) || (type == CR_16DC))
        cbp[2].bits |= i64power2 (bit);
      else
        cbp[0].bits |= i64power2 (bit);
      }
      //}}}

    return codedBlockPatternBit;
    }
  //}}}

  //{{{
  int readSignificanceMap (sMacroBlock* mb, sCabacDecode*  cabacDecode, int type, int coeff[]) {

    cSlice* slice = mb->slice;

    bool fld = (slice->picStructure != eFrame) || mb->mbField;
    const uint8_t* pos2ctx_Map = fld ? pos2ctx_map_int[type] : pos2ctx_map[type];
    const uint8_t* pos2ctx_Last = pos2ctx_last[type];

    sBiContext* map_ctx = slice->textureContexts->mapContexts [fld][type2ctx_map [type]];
    sBiContext* last_ctx = slice->textureContexts->lastContexts[fld][type2ctx_last[type]];

    int i0 = 0;
    int i1 = maxpos[type];
    if (!c1isdc[type]) {
      ++i0;
      ++i1;
      }

    int coefCount = 0;
    int i;
    for (i = i0; i < i1; ++i){
      // if last coeff is reached, it has to be significant
      if (cabacDecode->getSymbol (map_ctx + pos2ctx_Map[i])) {
        *(coeff++) = 1;
        ++coefCount;
        // read last coefficient symbol
        if (cabacDecode->getSymbol (last_ctx + pos2ctx_Last[i])) {
          memset (coeff, 0, (i1 - i) * sizeof(int));
          return coefCount;
          }
        }
      else
        *(coeff++) = 0;
      }

    // last coefficient must be significant if no last symbol was received
    if (i < i1 + 1) {
      *coeff = 1;
      ++coefCount;
      }

    return coefCount;
    }
  //}}}
  //{{{
  void readSignificantCoefs (sCabacDecode* cabacDecode, sTextureContexts* textureContexts, int type, int* coeff) {

    sBiContext* oneContexts = textureContexts->oneContexts[type2ctx_one[type]];
    sBiContext* absContexts = textureContexts->absContexts[type2ctx_abs[type]];

    const int16_t max_type = max_c2[type];
    int c1 = 1;
    int c2 = 0;
    int i = maxpos[type];
    int* cof = coeff + i;
    for (; i >= 0; i--) {
      if (*cof != 0) {
        *cof += cabacDecode->getSymbol (oneContexts + c1);

        if (*cof == 2) {
          *cof += cabacDecode->unaryExpGolombLevel (absContexts + c2);
          c2 = imin (++c2, max_type);
          c1 = 0;
          }
        else if (c1)
          c1 = imin (++c1, 4);

        if (cabacDecode->getSymbolEqProb())
          *cof = - *cof;
        }
      cof--;
      }
    }
  //}}}

  //{{{
  void readCompCoef4x4smb (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane,
                           int blockY, int blockX, int start_scan, int64_t *cbp_blk) {

    cSlice* slice = mb->slice;
    const uint8_t* dataParttitionMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];
    int** cof = slice->cof[plane];

    int level = 1;
    for (int j = blockY; j < blockY + BLOCK_SIZE_8x8; j += 4) {
      mb->subblockY = j; // position for coeff_count ctx
      for (int i = blockX; i < blockX + BLOCK_SIZE_8x8; i += 4) {
        mb->subblockX = i; // position for coeff_count ctx
        pos_scan_4x4 = pos_scan4x4[start_scan];
        level = 1;

        if (start_scan == 0) {
          // make distinction between INTRA and INTER coded luminance coefficients
          se->type = (mb->isIntraBlock ? SE_LUM_DC_INTRA : SE_LUM_DC_INTER);
          sDataPartition* dataPartition = &slice->dataPartitionArray[dataParttitionMap[se->type]];
          if (dataPartition->mBitStream.mError)
            se->mapping = sBitStream::infoLevelRunInter;
          else
            se->reading = readRunLevelCabac;
          dataPartition->readSyntaxElement (mb, se, dataPartition);
          level = se->value1;
          if (level != 0) {
            // leave if level == 0
            pos_scan_4x4 += 2 * se->value2;
            int i0 = *pos_scan_4x4++;
            int j0 = *pos_scan_4x4++;
            *cbp_blk |= i64power2(j + (i >> 2)) ;
            cof[j + j0][i + i0]= level;
            }
          }

        if (level != 0) {
          // make distinction between INTRA and INTER coded luminance coefficients
          se->type = (mb->isIntraBlock ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER);
          sDataPartition* dataPartition = &slice->dataPartitionArray[dataParttitionMap[se->type]];
          if (dataPartition->mBitStream.mError)
            se->mapping = sBitStream::infoLevelRunInter;
          else
            se->reading = readRunLevelCabac;
          for (int k = 1; (k < 17) && (level != 0); ++k) {
            dataPartition->readSyntaxElement (mb, se, dataPartition);
            level = se->value1;
            if (level != 0) {
              // leave if level == 0
              pos_scan_4x4 += 2 * se->value2;
              int i0 = *pos_scan_4x4++;
              int j0 = *pos_scan_4x4++;
              cof[j + j0][i + i0] = level;
              }
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void readCompCoef4x4 (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane,
                        int (*InvLevelScale4x4)[4], int qp_per, int codedBlockPattern) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int start_scan = IS_I16MB (mb)? 1 : 0;
    int blockY, blockX;
    int64_t *cbp_blk = &mb->cbp[plane].blk;

    if (plane == PLANE_Y || (decoder->coding.isSeperateColourPlane != 0))
      se->context = (IS_I16MB(mb) ? LUMA_16AC: LUMA_4x4);
    else if (plane == PLANE_U)
      se->context = (IS_I16MB(mb) ? CB_16AC: CB_4x4);
    else
      se->context = (IS_I16MB(mb) ? CR_16AC: CR_4x4);

    for (blockY = 0; blockY < MB_BLOCK_SIZE; blockY += BLOCK_SIZE_8x8) {
      /* all modes */
      int** cof = &slice->cof[plane][blockY];
      for (blockX = 0; blockX < MB_BLOCK_SIZE; blockX += BLOCK_SIZE_8x8) {
        if (codedBlockPattern & (1 << ((blockY >> 2) + (blockX >> 3)))) {
          // are there any coeff in current block at all
          readCompCoef4x4smb (mb, se, plane, blockY, blockX, start_scan, cbp_blk);
          if (start_scan == 0) {
            for (int j = 0; j < BLOCK_SIZE_8x8; ++j) {
              int* coef = &cof[j][blockX];
              int jj = j & 0x03;
              for (int i = 0; i < BLOCK_SIZE_8x8; i+=4) {
                if (*coef)
                  *coef = rshift_rnd_sf((*coef * InvLevelScale4x4[jj][0]) << qp_per, 4);
                coef++;
                if (*coef)
                  *coef = rshift_rnd_sf((*coef * InvLevelScale4x4[jj][1]) << qp_per, 4);
                coef++;
                if (*coef)
                  *coef = rshift_rnd_sf((*coef * InvLevelScale4x4[jj][2]) << qp_per, 4);
                coef++;
                if (*coef)
                  *coef = rshift_rnd_sf((*coef * InvLevelScale4x4[jj][3]) << qp_per, 4);
                coef++;
                }
              }
            }
          else {
            for (int j = 0; j < BLOCK_SIZE_8x8; ++j) {
              int* coef = &cof[j][blockX];
              int jj = j & 0x03;
              for (int i = 0; i < BLOCK_SIZE_8x8; i += 4) {
                if ((jj != 0) && *coef)
                  *coef= rshift_rnd_sf((*coef * InvLevelScale4x4[jj][0]) << qp_per, 4);
                coef++;
                if (*coef)
                  *coef= rshift_rnd_sf((*coef * InvLevelScale4x4[jj][1]) << qp_per, 4);
                coef++;
                if (*coef)
                  *coef= rshift_rnd_sf((*coef * InvLevelScale4x4[jj][2]) << qp_per, 4);
                coef++;
                if (*coef)
                  *coef= rshift_rnd_sf((*coef * InvLevelScale4x4[jj][3]) << qp_per, 4);
                coef++;
                }
              }
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void readCompCoef4x4Lossless (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane,
                                            int (*InvLevelScale4x4)[4], int qp_per, int codedBlockPattern) {

    cDecoder264* decoder = mb->decoder;
    int start_scan = IS_I16MB (mb)? 1 : 0;

    if ((plane == PLANE_Y) || (decoder->coding.isSeperateColourPlane != 0) )
      se->context = (IS_I16MB(mb) ? LUMA_16AC: LUMA_4x4);
    else if (plane == PLANE_U)
      se->context = (IS_I16MB(mb) ? CB_16AC: CB_4x4);
    else
      se->context = (IS_I16MB(mb) ? CR_16AC: CR_4x4);

    int64_t* cbp_blk = &mb->cbp[plane].blk;
    for (int blockY = 0; blockY < MB_BLOCK_SIZE; blockY += BLOCK_SIZE_8x8) /* all modes */
      for (int blockX = 0; blockX < MB_BLOCK_SIZE; blockX += BLOCK_SIZE_8x8)
        if (codedBlockPattern & (1 << ((blockY >> 2) + (blockX >> 3))))  // are there any coeff in current block at all
          readCompCoef4x4smb (mb, se, plane, blockY, blockX, start_scan, cbp_blk);
    }
  //}}}

  //{{{
  void readCompCoef8x8 (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane, int b8) {

    if (mb->codedBlockPattern & (1 << b8)) {
      // are there any coefficients in the current block
      cDecoder264* decoder = mb->decoder;
      int transform_pl = (decoder->coding.isSeperateColourPlane != 0) ? mb->slice->colourPlaneId : plane;

      int level = 1;

      cSlice* slice = mb->slice;

      int64_t cbp_mask = (int64_t) 51 << (4 * b8 - 2 * (b8 & 0x01)); // corresponds to 110011, as if all four 4x4 blocks contain coeff, shifted to block position
      int64_t* cur_cbp = &mb->cbp[plane].blk;

      // select scan type
      const uint8_t* pos_scan8x8 = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN8x8[0]
                                                                                       : FIELD_SCAN8x8[0];
      int qp_per = decoder->qpPerMatrix[mb->qpScaled[plane]];
      int qp_rem = decoder->qpRemMatrix[mb->qpScaled[plane]];
      int (*InvLevelScale8x8)[8] = mb->isIntraBlock ? slice->InvLevelScale8x8_Intra[transform_pl][qp_rem]
                                                    : slice->InvLevelScale8x8_Inter[transform_pl][qp_rem];

      // set offset in current macroBlock
      int boff_x = (b8&0x01) << 3;
      int boff_y = (b8 >> 1) << 3;
      int** tcoeffs = &slice->mbRess[plane][boff_y];

      mb->subblockX = boff_x; // position for coeff_count ctx
      mb->subblockY = boff_y; // position for coeff_count ctx

      const uint8_t* dataParttitionMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

      if ((plane == PLANE_Y) || (decoder->coding.isSeperateColourPlane != 0))
        se->context = LUMA_8x8;
      else if (plane == PLANE_U)
        se->context = CB_8x8;
      else
        se->context = CR_8x8;
      se->reading = readRunLevelCabac;

      // read DC
      se->type = ((mb->isIntraBlock == 1) ? SE_LUM_DC_INTRA : SE_LUM_DC_INTER ); // Intra or Inter?
      sDataPartition* dataPartition = &slice->dataPartitionArray[dataParttitionMap[se->type]];
      dataPartition->readSyntaxElement (mb, se, dataPartition);
      level = se->value1;
      if (level != 0) {
        *cur_cbp |= cbp_mask;
        pos_scan8x8 += 2 * (se->value2);
        int i = *pos_scan8x8++;
        int j = *pos_scan8x8++;
        tcoeffs[j][boff_x + i] = rshift_rnd_sf ((level * InvLevelScale8x8[j][i]) << qp_per, 6); // dequantization

        // read AC
        se->type = (mb->isIntraBlock == 1) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
        dataPartition = &slice->dataPartitionArray[dataParttitionMap[se->type]];
        for (int k = 1; (k < 65) && (level != 0); ++k) {
          dataPartition->readSyntaxElement (mb, se, dataPartition);
          level = se->value1;
          if (level != 0) {
            pos_scan8x8 += 2 * (se->value2);
            i = *pos_scan8x8++;
            j = *pos_scan8x8++;
            // dequantization
            tcoeffs[ j][boff_x + i] = rshift_rnd_sf ((level * InvLevelScale8x8[j][i]) << qp_per, 6);
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void readCompCoef8x8mb (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane) {

    // 8x8 transform size & eCabac
    readCompCoef8x8 (mb, se, plane, 0);
    readCompCoef8x8 (mb, se, plane, 1);
    readCompCoef8x8 (mb, se, plane, 2);
    readCompCoef8x8 (mb, se, plane, 3);
    }
  //}}}
  //{{{
  void readCompCoef8x8Lossless (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane, int b8) {

    if (mb->codedBlockPattern & (1 << b8)) {
      // are there any coefficients in the current block
      cDecoder264* decoder = mb->decoder;
      cSlice* slice = mb->slice;

      int64_t cbp_mask = (int64_t) 51 << (4 * b8 - 2 * (b8 & 0x01)); // corresponds to 110011, as if all four 4x4 blocks contain coeff, shifted to block position
      int64_t *cur_cbp = &mb->cbp[plane].blk;

      // select scan type
      const uint8_t* pos_scan8x8 = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN8x8[0] : FIELD_SCAN8x8[0];

      // === set offset in current macroBlock ===
      int boff_x = (b8&0x01) << 3;
      int boff_y = (b8 >> 1) << 3;
      int** tcoeffs = &slice->mbRess[plane][boff_y];

      mb->subblockX = boff_x; // position for coeff_count ctx
      mb->subblockY = boff_y; // position for coeff_count ctx

      const uint8_t* dataParttitionMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
      if ((plane == PLANE_Y) || (decoder->coding.isSeperateColourPlane != 0))
        se->context = LUMA_8x8;
      else if (plane == PLANE_U)
        se->context = CB_8x8;
      else
        se->context = CR_8x8;
      se->reading = readRunLevelCabac;

      int level = 1;
      for (int k = 0; (k < 65) && (level != 0); ++k) {
        // make distinction between INTRA and INTER codedluminance coefficients
        se->type  = ((mb->isIntraBlock == 1) ? (k == 0 ? SE_LUM_DC_INTRA : SE_LUM_AC_INTRA)
                                             : (k == 0 ? SE_LUM_DC_INTER : SE_LUM_AC_INTER));
        sDataPartition* dataPartition = &slice->dataPartitionArray[dataParttitionMap[se->type]];
        se->reading = readRunLevelCabac;
        dataPartition->readSyntaxElement (mb, se, dataPartition);
        level = se->value1;
        if (level != 0) {
          pos_scan8x8 += 2 * (se->value2);
          int i = *pos_scan8x8++;
          int j = *pos_scan8x8++;
          *cur_cbp |= cbp_mask;
          tcoeffs[j][boff_x + i] = level;
          }
        }
     }
    }
  //}}}
  //{{{
  void readCompCoef8x8mbLossless (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane) {

    // 8x8 transform size & eCabac
    readCompCoef8x8Lossless (mb, se, plane, 0);
    readCompCoef8x8Lossless (mb, se, plane, 1);
    readCompCoef8x8Lossless (mb, se, plane, 2);
    readCompCoef8x8Lossless (mb, se, plane, 3);
    }
  //}}}
  }

//{{{
void cabacNewSlice (cSlice* slice) {
  slice->lastDquant = 0;
  }
//}}}
//{{{
int cabacStartCode (cSlice* slice, int eosBit) {

  uint32_t bit;
  if (eosBit) {
    const uint8_t* dataParttitionMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    sDataPartition* dataPartition = &slice->dataPartitionArray[dataParttitionMap[SE_MBTYPE]];
    bit = dataPartition->mCabacDecode.getFinal();
    }
  else
    bit = 0;

  return bit == 1 ? 1 : 0;
  }
//}}}

//{{{
void checkNeighbourCabac (sMacroBlock* mb) {

  cDecoder264* decoder = mb->decoder;

  int* mbSize = decoder->mbSize[eLuma];
  sPixelPos up;
  decoder->getNeighbour (mb,  0, -1, mbSize, &up);
  if (up.ok)
    mb->mbCabacUp = &mb->slice->mbData[up.mbIndex];
  else
    mb->mbCabacUp = NULL;

  sPixelPos left;
  decoder->getNeighbour (mb, -1,  0, mbSize, &left);
  if (left.ok)
    mb->mbCabacLeft = &mb->slice->mbData[left.mbIndex];
  else
    mb->mbCabacLeft = NULL;
  }
//}}}
//{{{
int checkNextMbFieldCabacSliceP (cSlice* slice, sSyntaxElement* se, sDataPartition* dataPartition) {

  sCabacDecode* cabacDecode = &dataPartition->mCabacDecode;
  sMotionContexts* motionContexts  = slice->motionContexts;

  // get next MB
  ++slice->mbIndex;

  sMacroBlock* mb = &slice->mbData[slice->mbIndex];
  mb->decoder = slice->decoder;
  mb->slice = slice;
  mb->sliceNum = slice->curSliceIndex;
  mb->mbField = slice->mbData[slice->mbIndex-1].mbField;
  mb->mbIndexX  = slice->mbIndex;
  mb->listOffset = ((slice->mbAffFrame) && (mb->mbField))? (mb->mbIndexX&0x01) ? 4 : 2 : 0;
  checkNeighboursMbAff (mb);
  checkNeighbourCabac (mb);

  // copy
  sCabacDecode cabacDecodeCopy;
  memcpy (&cabacDecodeCopy, cabacDecode, sizeof(sCabacDecode));
  int codeStreamLen = *(cabacDecodeCopy.codeStreamLen) = *(cabacDecode->codeStreamLen);
  sBiContext mbAffContextCopy[NUM_MB_AFF_CTX];
  memcpy (&mbAffContextCopy, motionContexts->mbAffContexts, NUM_MB_AFF_CTX*sizeof(sBiContext) );
  sBiContext mbTypeContextCopy[3][NUM_MB_TYPE_CTX];
  for (int i = 0; i < 3;++i)
    memcpy (&mbTypeContextCopy[i], motionContexts->mbTypeContexts[i], NUM_MB_TYPE_CTX*sizeof(sBiContext));

  // check_next_mb
  slice->lastDquant = 0;
  readSkipCabacSliceP (mb, se, cabacDecode);
  bool skip = (se->value1 == 0);
  if (!skip) {
    readFieldModeCabac (mb, se,cabacDecode);
    slice->mbData[slice->mbIndex-1].mbField = se->value1;
    }

  // reset
  slice->mbIndex--;

  // restore
  memcpy (cabacDecode, &cabacDecodeCopy, sizeof(sCabacDecode));
  *(cabacDecode->codeStreamLen) = codeStreamLen;
  memcpy (motionContexts->mbAffContexts, &mbAffContextCopy, NUM_MB_AFF_CTX * sizeof(sBiContext));
  for (int i = 0; i < 3; ++i)
    memcpy (motionContexts->mbTypeContexts[i], &mbTypeContextCopy[i], NUM_MB_TYPE_CTX * sizeof(sBiContext));

  checkNeighbourCabac (mb);

  return skip;
  }
//}}}
//{{{
int checkNextMbFieldCabacSliceB (cSlice* slice, sSyntaxElement* se, sDataPartition* dataPartition) {

  sCabacDecode* cabacDecode = &dataPartition->mCabacDecode;
  sMotionContexts* motionContexts = slice->motionContexts;

  // get next MB
  ++slice->mbIndex;

  sMacroBlock* mb = &slice->mbData[slice->mbIndex];
  mb->decoder = slice->decoder;
  mb->slice = slice;
  mb->sliceNum = slice->curSliceIndex;
  mb->mbField = slice->mbData[slice->mbIndex-1].mbField;
  mb->mbIndexX  = slice->mbIndex;
  mb->listOffset = ((slice->mbAffFrame)&&(mb->mbField))? (mb->mbIndexX & 0x01) ? 4 : 2 : 0;
  checkNeighboursMbAff (mb);
  checkNeighbourCabac (mb);

  // copy
  sCabacDecode cabacDecodeCopy;
  memcpy (&cabacDecodeCopy, cabacDecode, sizeof(sCabacDecode));
  int codeStreamLen = *(cabacDecodeCopy.codeStreamLen) = *(cabacDecode->codeStreamLen);
  sBiContext mbAffContextCopy[NUM_MB_AFF_CTX];
  memcpy (&mbAffContextCopy, motionContexts->mbAffContexts, NUM_MB_AFF_CTX * sizeof(sBiContext));
  sBiContext mbTypeContextCopy[3][NUM_MB_TYPE_CTX];
  for (int i = 0; i < 3;++i)
    memcpy (&mbTypeContextCopy[i], motionContexts->mbTypeContexts[i], NUM_MB_TYPE_CTX * sizeof(sBiContext));

  //  check_next_mb
  slice->lastDquant = 0;
  readSkipCabacSliceB (mb, se, cabacDecode);
  bool skip = (se->value1 == 0) && (se->value2 == 0);
  if (!skip) {
    readFieldModeCabac (mb, se, cabacDecode);
    slice->mbData[slice->mbIndex-1].mbField = se->value1;
    }

  // reset
  slice->mbIndex--;

  // restore
  memcpy (cabacDecode, &cabacDecodeCopy, sizeof(sCabacDecode));
  *(cabacDecode->codeStreamLen) = codeStreamLen;
  memcpy (motionContexts->mbAffContexts, &mbAffContextCopy, NUM_MB_AFF_CTX * sizeof(sBiContext));
  for (int i = 0; i < 3; ++i)
    memcpy (motionContexts->mbTypeContexts[i], &mbTypeContextCopy[i], NUM_MB_TYPE_CTX * sizeof(sBiContext));

  checkNeighbourCabac (mb);

  return skip;
  }
//}}}

//{{{
int readSyntaxElementCabac (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* dataPartition) {

  sCabacDecode* cabacDecode = &dataPartition->mCabacDecode;
  int curLen = cabacDecode->getBitsRead();

  // perform the actual decoding by calling the appropriate method
  se->reading (mb, se, cabacDecode);

  // read again and minus curr_len = bitsRead(cabacDecode); from above
  se->len = cabacDecode->getBitsRead() - curLen;

  return se->len;
  }
//}}}

//{{{
void readMbTypeCabacSliceI (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cSlice* slice = mb->slice;
  sMotionContexts* context = slice->motionContexts;

  int actContext;
  int actSym;
  int modeSym;
  int curMbType = 0;

  if (slice->sliceType == cSlice::eSliceI)  {
    //{{{  INTRA-frame
    int a = mb->mbCabacLeft ? (((mb->mbCabacLeft->mbType != I4MB) && (mb->mbCabacLeft->mbType != I8MB)) ? 1 : 0) : 0;
    int b = mb->mbCabacUp ? (((mb->mbCabacUp->mbType != I4MB) && (mb->mbCabacUp->mbType != I8MB)) ? 1 : 0) : 0;
    actContext = a + b;

    actSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext);
    se->context = actContext; // store context

    if (actSym == 0) // 4x4 Intra
      curMbType = actSym;
    else {
      // 16x16 Intra
      modeSym = cabacDecode->getFinal();
      if(modeSym == 1)
        curMbType = 25;
      else {
        actSym = 1;
        actContext = 4;
        modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext ); // decoding of AC/no AC
        actSym += modeSym*12;
        actContext = 5;
        // decoding of codedBlockPattern: 0,1,2
        modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext );
        if (modeSym != 0) {
          actContext = 6;
          modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext );
          actSym += 4;
          if (modeSym != 0)
            actSym += 4;
          }

        // decoding of I pred-mode: 0,1,2,3
        actContext = 7;
        modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext );
        actSym += modeSym * 2;
        actContext = 8;
        modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext );
        actSym += modeSym;
        curMbType = actSym;
        }
      }
    }
    //}}}
  else if(slice->sliceType == cSlice::eSliceSI)  {
    //{{{  SI-frame
    // special context's for SI4MB
    int a = mb->mbCabacLeft ? ((mb->mbCabacLeft->mbType != SI4MB) ? 1 : 0) : 0;
    int b = mb->mbCabacUp ? ((mb->mbCabacUp->mbType != SI4MB) ? 1 : 0) : 0;

    actContext = a + b;
    actSym = cabacDecode->getSymbol (context->mbTypeContexts[1] + actContext);
    se->context = actContext; // store context

    if (actSym == 0) //  SI 4x4 Intra
      curMbType = 0;
    else {
      // analog INTRA_IMG
      if (mb->mbCabacUp)
        b = (((mb->mbCabacUp)->mbType != I4MB) ? 1 : 0 );
      if (mb->mbCabacLeft)
        a = (((mb->mbCabacLeft)->mbType != I4MB) ? 1 : 0 );

      actContext = a + b;
      actSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext);
      se->context = actContext; // store context

      if (actSym == 0) // 4x4 Intra
        curMbType = 1;
      else {
        // 16x16 Intra
        modeSym = cabacDecode->getFinal();
        if (modeSym == 1)
          curMbType = 26;
        else {
          actSym = 2;
          actContext = 4;
          modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext ); // decoding of AC/no AC
          actSym += modeSym*12;
          actContext = 5;

          // decoding of codedBlockPattern: 0,1,2
          modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext );
          if (modeSym != 0) {
            actContext = 6;
            modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext );
            actSym += 4;
            if (modeSym != 0)
              actSym += 4;
            }

          // decoding of I pred-mode: 0,1,2,3
          actContext = 7;
          modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext );
          actSym += modeSym * 2;
          actContext = 8;
          modeSym = cabacDecode->getSymbol (context->mbTypeContexts[0] + actContext );
          actSym += modeSym;
          curMbType = actSym;
          }
        }
      }
    }
    //}}}

  se->value1 = curMbType;
  }
//}}}
//{{{
void readMbTypeCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cSlice* slice = mb->slice;
  sMotionContexts* motionContexts = slice->motionContexts;

  int actSym;
  sBiContext* mbTypeContexts = motionContexts->mbTypeContexts[1];
  if (cabacDecode->getSymbol (&mbTypeContexts[4])) {
    if (cabacDecode->getSymbol (&mbTypeContexts[7]))
      actSym = 7;
    else
      actSym = 6;
    }
  else {
    if (cabacDecode->getSymbol (&mbTypeContexts[5])) {
      if (cabacDecode->getSymbol (&mbTypeContexts[7]))
        actSym = 2;
      else
        actSym = 3;
      }
    else {
      if (cabacDecode->getSymbol (&mbTypeContexts[6]))
        actSym = 4;
      else
        actSym = 1;
      }
    }

  int actContext;
  int modeSym;
  int curMbType;
  if (actSym <= 6)
    curMbType = actSym;
  else  {
    // additional info for 16x16 Intra-mode
    modeSym = cabacDecode->getFinal();
    if (modeSym == 1)
      curMbType = 31;
    else {
      actContext = 8;
      modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext ); // decoding of AC/no AC
      actSym += modeSym*12;

      // decoding of codedBlockPattern: 0,1,2
      actContext = 9;
      modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext );
      if (modeSym != 0) {
        actSym += 4;
        modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext );
        if (modeSym != 0)
          actSym += 4;
        }

      // decoding of I pred-mode: 0,1,2,3
      actContext = 10;
      modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext );
      actSym += modeSym*2;
      modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext );
      actSym += modeSym;
      curMbType = actSym;
      }
    }

  se->value1 = curMbType;
  }
//}}}
//{{{
void readMbTypeCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cSlice* slice = mb->slice;

  sBiContext* mbTypeContexts = slice->motionContexts->mbTypeContexts[2];

  int a = mb->mbCabacLeft ? (((mb->mbCabacLeft)->mbType != 0) ? 1 : 0 ): 0;
  int b = mb->mbCabacUp ? (((mb->mbCabacUp)->mbType != 0) ? 1 : 0) : 0;
  int actContext = a + b;

  int actSym;
  if (cabacDecode->getSymbol (&mbTypeContexts[actContext])) {
    if (cabacDecode->getSymbol (&mbTypeContexts[4])) {
      if (cabacDecode->getSymbol (&mbTypeContexts[5])) {
        actSym = 12;
        if (cabacDecode->getSymbol (&mbTypeContexts[6]))
          actSym += 8;
        if (cabacDecode->getSymbol (&mbTypeContexts[6]))
          actSym += 4;
        if (cabacDecode->getSymbol (&mbTypeContexts[6]))
          actSym += 2;
        if (actSym == 24)
          actSym = 11;
        else if (actSym == 26)
          actSym = 22;
        else {
          if (actSym == 22)
            actSym = 23;
          if (cabacDecode->getSymbol (&mbTypeContexts[6]))
            actSym += 1;
          }
        }
      else {
        actSym = 3;
        if (cabacDecode->getSymbol (&mbTypeContexts[6]))
          actSym += 4;
        if (cabacDecode->getSymbol (&mbTypeContexts[6]))
          actSym += 2;
        if (cabacDecode->getSymbol (&mbTypeContexts[6]))
          actSym += 1;
        }
      }
    else {
      if (cabacDecode->getSymbol (&mbTypeContexts[6]))
        actSym = 2;
      else
        actSym = 1;
      }
    }
  else
    actSym = 0;

  int curMbType;
  int modeSym;
  if (actSym <= 23)
    curMbType = actSym;
  else  {
    // additional info for 16x16 Intra-mode
    modeSym = cabacDecode->getFinal();
    if (modeSym == 1)
      curMbType = 48;
    else {
      mbTypeContexts = slice->motionContexts->mbTypeContexts[1];
      actContext = 8;
      modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext ); // decoding of AC/no AC
      actSym += modeSym*12;

      // decoding of codedBlockPattern: 0,1,2
      actContext = 9;
      modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext );
      if (modeSym != 0) {
        actSym += 4;
        modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext );
        if (modeSym != 0)
          actSym += 4;
        }

      // decoding of I pred-mode: 0,1,2,3
      actContext = 10;
      modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext );
      actSym += modeSym * 2;
      modeSym = cabacDecode->getSymbol (mbTypeContexts + actContext );
      actSym += modeSym;
      curMbType = actSym;
      }
    }

  se->value1 = curMbType;
  }
//}}}
//{{{
void readB8TypeCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cSlice* slice = mb->slice;
  sMotionContexts* motionContexts = slice->motionContexts;
  sBiContext* b8TypeContexts = &motionContexts->b8TypeContexts[0][1];

  int actSym = 0;
  if (cabacDecode->getSymbol (b8TypeContexts++))
    actSym = 0;
  else {
    if (cabacDecode->getSymbol (++b8TypeContexts))
      actSym = (cabacDecode->getSymbol (++b8TypeContexts)) ? 2: 3;
    else
      actSym = 1;
    }

  se->value1 = actSym;
  }
//}}}
//{{{
void readB8TypeCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cSlice* slice = mb->slice;

  sMotionContexts* motionContexts = slice->motionContexts;
  sBiContext* b8TypeContexts = motionContexts->b8TypeContexts[1];

  int actSym = 0;
  if (cabacDecode->getSymbol (b8TypeContexts++)) {
    if (cabacDecode->getSymbol (b8TypeContexts++)) {
      if (cabacDecode->getSymbol (b8TypeContexts++)) {
        if (cabacDecode->getSymbol (b8TypeContexts)) {
          actSym = 10;
          if (cabacDecode->getSymbol (b8TypeContexts))
            actSym++;
          }
        else {
          actSym = 6;
          if (cabacDecode->getSymbol (b8TypeContexts))
            actSym += 2;
          if (cabacDecode->getSymbol (b8TypeContexts))
            actSym++;
          }
        }
      else {
        actSym = 2;
        if (cabacDecode->getSymbol (b8TypeContexts))
          actSym += 2;
        if (cabacDecode->getSymbol (b8TypeContexts))
          actSym ++;
        }
      }
    else
      actSym = (cabacDecode->getSymbol (++b8TypeContexts)) ? 1: 0;
    ++actSym;
    }
  else
    actSym = 0;

  se->value1 = actSym;
  }
//}}}

//{{{
void readRefFrameCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cDecoder264* decoder = mb->decoder;
  cSlice* slice = mb->slice;
  sPicture* picture = slice->picture;
  sMotionContexts* motionContexts = slice->motionContexts;

  sMacroBlock* neighborMB = NULL;

  int addctx  = 0;
  int a = 0;
  int b = 0;
  int actContext;
  int actSym;
  int list = se->value2;

  sPixelPos block_a;
  sPixelPos block_b;
  get4x4Neighbour (mb, mb->subblockX - 1, mb->subblockY, decoder->mbSize[eLuma], &block_a);
  get4x4Neighbour (mb, mb->subblockX, mb->subblockY - 1, decoder->mbSize[eLuma], &block_b);

  if (block_b.ok) {
    int b8b = ((block_b.x >> 1) & 0x01) + (block_b.y & 0x02);
    neighborMB = &slice->mbData[block_b.mbIndex];
    if (!((neighborMB->mbType == IPCM) ||
          IS_DIRECT(neighborMB) ||
          (neighborMB->b8mode[b8b]==0 && neighborMB->b8pdir[b8b] == 2))) {
      if (slice->mbAffFrame && (mb->mbField == false) && (neighborMB->mbField == true))
        b = picture->mvInfo[block_b.posY][block_b.posX].refIndex[list] > 1 ? 2 : 0;
      else
        b = picture->mvInfo[block_b.posY][block_b.posX].refIndex[list] > 0 ? 2 : 0;
      }
    }

  if (block_a.ok) {
    int b8a = ((block_a.x >> 1) & 0x01) + (block_a.y & 0x02);
    neighborMB = &slice->mbData[block_a.mbIndex];
    if (!((neighborMB->mbType == IPCM) ||
          IS_DIRECT(neighborMB) ||
          (neighborMB->b8mode[b8a] == 0 && neighborMB->b8pdir[b8a] == 2))) {
      if (slice->mbAffFrame && (mb->mbField == false) && (neighborMB->mbField == 1))
        a = picture->mvInfo[block_a.posY][block_a.posX].refIndex[list] > 1 ? 1 : 0;
      else
        a = picture->mvInfo[block_a.posY][block_a.posX].refIndex[list] > 0 ? 1 : 0;
      }
    }

  actContext = a + b;
  se->context = actContext;

  actSym = cabacDecode->getSymbol (motionContexts->refNoContexts[addctx] + actContext );
  if (actSym != 0) {
    actContext = 4;
    actSym = cabacDecode->unaryBin (motionContexts->refNoContexts[addctx] + actContext,1);
    ++actSym;
    }

  se->value1 = actSym;
  }
//}}}
//{{{
void readIntraPredModeCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cSlice* slice = mb->slice;
  sTextureContexts* context = slice->textureContexts;

  // use_most_probable_mode
  int actSym = cabacDecode->getSymbol (context->iprContexts);

  // remaining_mode_selector
  if (actSym == 1)
    se->value1 = -1;
  else {
    se->value1= cabacDecode->getSymbol (context->iprContexts + 1);
    se->value1 |= cabacDecode->getSymbol (context->iprContexts + 1) << 1;
    se->value1 |= cabacDecode->getSymbol (context->iprContexts + 1) << 2;
    }
  }
//}}}

//{{{
void readMvdCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  int* mbSize = mb->decoder->mbSize[eLuma];
  cSlice* slice = mb->slice;
  sMotionContexts* motionContexts = slice->motionContexts;

  int i = mb->subblockX;
  int j = mb->subblockY;

  int actSym;
  int list_idx = se->value2 & 0x01;
  int k = (se->value2 >> 1); // MVD component

  sPixelPos block_a;
  sPixelPos block_b;
  get4x4NeighbourBase (mb, i - 1, j, mbSize, &block_a);
  get4x4NeighbourBase (mb, i, j - 1, mbSize, &block_b);

  int a = 0;
  if (block_a.ok)
    a = iabs (slice->mbData[block_a.mbIndex].mvd[list_idx][block_a.y][block_a.x][k]);
  if (block_b.ok)
    a += iabs (slice->mbData[block_b.mbIndex].mvd[list_idx][block_b.y][block_b.x][k]);
  if (a < 3)
    a = 5 * k;
  else if (a > 32)
    a = 5 * k + 3;
  else
    a = 5 * k + 2;

  se->context = a;

  actSym = cabacDecode->getSymbol (motionContexts->mvResContexts[0] + a );
  if (actSym != 0) {
    a = 5 * k;
    actSym = cabacDecode->unaryExpGolombMv (motionContexts->mvResContexts[1] + a, 3) + 1;
    if (cabacDecode->getSymbolEqProb())
      actSym = -actSym;
    }

  se->value1 = actSym;
  }
//}}}
//{{{
void readMvdCabacMbAff (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cDecoder264* decoder = mb->decoder;
  cSlice* slice = mb->slice;
  sMotionContexts* motionContexts = slice->motionContexts;

  int i = mb->subblockX;
  int j = mb->subblockY;

  int list_idx = se->value2 & 0x01;
  int k = (se->value2 >> 1); // MVD component

  sPixelPos block_a;
  get4x4NeighbourBase (mb, i - 1, j, decoder->mbSize[eLuma], &block_a);
  int a = 0;
  if (block_a.ok) {
    a = iabs (slice->mbData[block_a.mbIndex].mvd[list_idx][block_a.y][block_a.x][k]);
    if (slice->mbAffFrame && (k == 1)) {
      if ((mb->mbField == 0) && (slice->mbData[block_a.mbIndex].mbField==1))
        a *= 2;
      else if ((mb->mbField == 1) && (slice->mbData[block_a.mbIndex].mbField==0))
        a /= 2;
      }
    }

  sPixelPos block_b;
  get4x4NeighbourBase (mb, i, j - 1, decoder->mbSize[eLuma], &block_b);
  int b = 0;
  if (block_b.ok) {
    b = iabs(slice->mbData[block_b.mbIndex].mvd[list_idx][block_b.y][block_b.x][k]);
    if (slice->mbAffFrame && (k == 1)) {
      if ((mb->mbField == 0) && (slice->mbData[block_b.mbIndex].mbField==1))
        b *= 2;
      else if ((mb->mbField==1) && (slice->mbData[block_b.mbIndex].mbField==0))
        b /= 2;
      }
    }
  a += b;

  int actContext;
  if (a < 3)
    actContext = 5 * k;
  else if (a > 32)
    actContext = 5 * k + 3;
  else
    actContext = 5 * k + 2;
  se->context = actContext;

  int actSym = cabacDecode->getSymbol (&motionContexts->mvResContexts[0][actContext]);
  if (actSym != 0) {
    actContext = 5 * k;
    actSym = cabacDecode->unaryExpGolombMv (motionContexts->mvResContexts[1] + actContext, 3) + 1;
    if (cabacDecode->getSymbolEqProb())
      actSym = -actSym;
    }

  se->value1 = actSym;
  }
//}}}

//{{{
void readCbpCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cDecoder264* decoder = mb->decoder;
  sPicture* picture = mb->slice->picture;
  cSlice* slice = mb->slice;

  sTextureContexts* textureContexts = slice->textureContexts;
  sMacroBlock* neighborMB = NULL;

  int mb_x;
  int mb_y;
  int curr_cbp_ctx;
  int codedBlockPattern = 0;
  int codedBlockPatternBit;
  int mask;
  sPixelPos block_a;

  //  coding of luma part (bit by bit)
  int a = 0;
  int b = 0;
  for (mb_y = 0; mb_y < 4; mb_y += 2) {
    if (mb_y == 0) {
      neighborMB = mb->mbCabacUp;
      b = 0;
      }

    for (mb_x=0; mb_x < 4; mb_x += 2) {
      if (mb_y == 0) {
        if (neighborMB != NULL)
          if (neighborMB->mbType != IPCM)
            b = ((neighborMB->codedBlockPattern & (1 << (2 + (mb_x >> 1)))) == 0) ? 2 : 0;
        }
      else
        b = ((codedBlockPattern & (1 << (mb_x /2 ))) == 0) ? 2: 0;

      if (mb_x == 0) {
        get4x4Neighbour (mb, (mb_x<<2) - 1, (mb_y << 2), decoder->mbSize[eLuma], &block_a);
        if (block_a.ok) {
          if (slice->mbData[block_a.mbIndex].mbType==IPCM)
            a = 0;
          else
            a = ((slice->mbData[block_a.mbIndex].codedBlockPattern & (1<<(2*(block_a.y/2)+1))) == 0) ? 1 : 0;
          }
        else
          a = 0;
        }
      else
        a = ((codedBlockPattern & (1 << mb_y)) == 0) ? 1 : 0;

      curr_cbp_ctx = a + b;
      mask = (1 << (mb_y + (mb_x >> 1)));
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->cbpContexts[0] + curr_cbp_ctx);
      if (codedBlockPatternBit)
        codedBlockPattern += mask;
      }
    }

  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
    // coding of chroma part eCabac decoding for BinIdx 0
    b = 0;
    neighborMB = mb->mbCabacUp;
    if (neighborMB)
      if (neighborMB->mbType==IPCM || (neighborMB->codedBlockPattern > 15))
        b = 2;

    a = 0;
    neighborMB = mb->mbCabacLeft;
    if (neighborMB)
      if (neighborMB->mbType==IPCM || (neighborMB->codedBlockPattern > 15))
        a = 1;

    curr_cbp_ctx = a + b;
    codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->cbpContexts[1] + curr_cbp_ctx);

    // cabac decoding for BinIdx 1
    if (codedBlockPatternBit) {
      // set the chroma bits
      b = 0;
      neighborMB = mb->mbCabacUp;
      if (neighborMB)
        if ((neighborMB->mbType == IPCM) || ((neighborMB->codedBlockPattern >> 4) == 2))
          b = 2;

      a = 0;
      neighborMB = mb->mbCabacLeft;
      if (neighborMB)
        if ((neighborMB->mbType == IPCM) || ((neighborMB->codedBlockPattern >> 4) == 2))
          a = 1;

      curr_cbp_ctx = a + b;
      codedBlockPatternBit = cabacDecode->getSymbol (textureContexts->cbpContexts[2] + curr_cbp_ctx);
      codedBlockPattern += (codedBlockPatternBit == 1) ? 32 : 16;
      }
    }

  se->value1 = codedBlockPattern;

  if (!codedBlockPattern)
    slice->lastDquant = 0;
  }
//}}}
//{{{
void readQuantCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cSlice* slice = mb->slice;

  int* dquant = &se->value1;
  int actContext = (slice->lastDquant != 0) ? 1 : 0;
  int actSym = cabacDecode->getSymbol (slice->motionContexts->deltaQpContexts + actContext);
  if (actSym != 0) {
    actContext = 2;
    actSym = cabacDecode->unaryBin (slice->motionContexts->deltaQpContexts + actContext, 1);
    ++actSym;
    *dquant = (actSym + 1) >> 1;
    if ((actSym & 0x01) == 0) // lsb is signed bit
      *dquant = -*dquant;
    }
  else
    *dquant = 0;

  slice->lastDquant = *dquant;
  }
//}}}
//{{{
void readRunLevelCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cSlice* slice = mb->slice;

  int* coefCount = &slice->coefCount;
  int* coeff = slice->coeff;

  // read coefficients for whole block
  if (*coefCount < 0) {
    // decode CBP-BIT
    if ((*coefCount = mb->readCbp (mb, cabacDecode, se->context)) != 0) {
      // decode significance map
      *coefCount = readSignificanceMap (mb, cabacDecode, se->context, coeff);

      // decode significant coefficients
      readSignificantCoefs (cabacDecode, slice->textureContexts, se->context, coeff);
      }
    }

  // set run and level
  if (*coefCount) {
    // set run and level (coefficient)
    for (se->value2 = 0; coeff[slice->pos] == 0; ++slice->pos, ++se->value2);
    se->value1 = coeff[slice->pos++];
    }
  else {
    // set run and level (EOB) ---
    se->value1 = 0;
    se->value2 = 0;
    }

  // decrement coefficient counter and re-set position
  if ((*coefCount)-- == 0)
    slice->pos = 0;
  }
//}}}
//{{{
void readCiPredModCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  sMacroBlock* MbUp = mb->mbCabacUp;
  sMacroBlock* MbLeft = mb->mbCabacLeft;

  int a = MbLeft ? (((MbLeft->chromaPredMode != 0) && (MbLeft->mbType != IPCM)) ? 1 : 0) : 0;
  int b = MbUp ? (((MbUp->chromaPredMode != 0) && (MbUp->mbType != IPCM)) ? 1 : 0) : 0;

  se->value1 = cabacDecode->getSymbol (mb->slice->textureContexts->ciprContexts + a + b);
  if (se->value1 != 0)
    se->value1 = cabacDecode->unaryBinMax (mb->slice->textureContexts->ciprContexts + 3, 0, 1) + 1;
  }
//}}}

//{{{
void readSkipCabacSliceP (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  int a = (mb->mbCabacLeft != NULL) ? (mb->mbCabacLeft->skipFlag == 0) : 0;
  int b = (mb->mbCabacUp != NULL) ? (mb->mbCabacUp  ->skipFlag == 0) : 0;
  sBiContext* mbTypeContexts = &mb->slice->motionContexts->mbTypeContexts[1][a + b];

  se->value1 = cabacDecode->getSymbol (mbTypeContexts) != 1;
  if (!se->value1)
    mb->slice->lastDquant = 0;
  }
//}}}
//{{{
void readSkipCabacSliceB (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  int a = (mb->mbCabacLeft != NULL) ? (mb->mbCabacLeft->skipFlag == 0) : 0;
  int b = (mb->mbCabacUp != NULL) ? (mb->mbCabacUp->skipFlag == 0) : 0;
  sBiContext* mbTypeContexts = &mb->slice->motionContexts->mbTypeContexts[2][7 + a + b];

  se->value1 = se->value2 = (cabacDecode->getSymbol (mbTypeContexts) != 1);
  if (!se->value1)
    mb->slice->lastDquant = 0;
  }
//}}}

//{{{
void readFieldModeCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  cSlice* slice = mb->slice;

  int a = mb->mbAvailA ? slice->mbData[mb->mbIndexA].mbField : 0;
  int b = mb->mbAvailB ? slice->mbData[mb->mbIndexB].mbField : 0;

  se->value1 = cabacDecode->getSymbol (&slice->motionContexts->mbAffContexts[a + b]);
  }
//}}}
//{{{
void readMbTransformSizeCabac (sMacroBlock* mb, sSyntaxElement* se, sCabacDecode* cabacDecode) {

  int a = (mb->mbCabacLeft == NULL) ? 0 : mb->mbCabacLeft->lumaTransformSize8x8flag;
  int b = (mb->mbCabacUp == NULL) ? 0 : mb->mbCabacUp->lumaTransformSize8x8flag;
  se->value1 = cabacDecode->getSymbol (mb->slice->textureContexts->transformSizeContexts + a + b);
  }
//}}}

//{{{
void readIpcmCabac (cSlice* slice, sDataPartition* dataPartition) {

  cDecoder264* decoder = slice->decoder;
  sPicture* picture = slice->picture;

  sBitStream* bitStream = &dataPartition->mBitStream;
  sCabacDecode* cabacDecode = &dataPartition->mCabacDecode;
  uint8_t* buf = bitStream->mBuffer;
  int bitStreamLengthInBits = (dataPartition->mBitStream.mLength << 3) + 7;

  while (cabacDecode->bitsLeft >= 8) {
    cabacDecode->value >>= 8;
    cabacDecode->bitsLeft -= 8;
    (*cabacDecode->codeStreamLen)--;
    }
  int bitOffset = (*cabacDecode->codeStreamLen) << 3;

  // read luma values
  int val = 0;
  int bitsRead = 0;
  int bitDepth = decoder->bitDepthLuma;
  for (int i = 0; i < MB_BLOCK_SIZE; ++i) {
    for (int j = 0; j < MB_BLOCK_SIZE; ++j) {
      bitsRead += sBitStream::getBits (buf, bitOffset, &val, bitStreamLengthInBits, bitDepth);
      slice->cof[0][i][j] = val;
      bitOffset += bitDepth;
      }
    }

  // read chroma values
  bitDepth = decoder->bitDepthChroma;
  if ((picture->chromaFormatIdc != YUV400) && (decoder->coding.isSeperateColourPlane == 0)) {
    for (int uv = 1; uv < 3; ++uv) {
      for (int i = 0; i < decoder->mbCrSizeY; ++i) {
        for (int j = 0; j < decoder->mbCrSizeX; ++j) {
          bitsRead += sBitStream::getBits (buf, bitOffset, &val, bitStreamLengthInBits, bitDepth);
          slice->cof[uv][i][j] = val;
          bitOffset += bitDepth;
          }
         }
      }
    }

  (*cabacDecode->codeStreamLen) += bitsRead >> 3;
  if (bitsRead & 7)
    ++(*cabacDecode->codeStreamLen);
  }
//}}}

//{{{
void sMacroBlock::setReadCompCabac() {

  if (isLossless) {
    readCompCoef4x4cabac = readCompCoef4x4Lossless;
    readCompCoef8x8cabac = readCompCoef8x8mbLossless;
    }
  else {
    readCompCoef4x4cabac = readCompCoef4x4;
    readCompCoef8x8cabac = readCompCoef8x8mb;
    }
  }
//}}}
//{{{
void sMacroBlock::setReadCbpCabac (int chromaFormatIdc) {
  readCbp = (chromaFormatIdc == YUV444) ? readCbp444 : readCbpNormal;
  }
//}}}
