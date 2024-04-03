//{{{  includes
#include "global.h"
#include "memory.h"

#include "cabac.h"
#include "syntaxElement.h"
#include "image.h"
#include "binaryArithmeticDecode.h"
#include "mbAccess.h"
#include "vlc.h"
//}}}

//{{{  static const tables
static const short maxpos       [] = {15, 14, 63, 31, 31, 15,  3, 14,  7, 15, 15, 14, 63, 31, 31, 15, 15, 14, 63, 31, 31, 15};
static const short c1isdc       [] = { 1,  0,  1,  1,  1,  1,  1,  0,  1,  1,  1,  0,  1,  1,  1,  1,  1,  0,  1,  1,  1,  1};
static const short type2ctx_bcbp[] = { 0,  1,  2,  3,  3,  4,  5,  6,  5,  5, 10, 11, 12, 13, 13, 14, 16, 17, 18, 19, 19, 20};
static const short type2ctx_map [] = { 0,  1,  2,  3,  4,  5,  6,  7,  6,  6, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21}; // 8
static const short type2ctx_last[] = { 0,  1,  2,  3,  4,  5,  6,  7,  6,  6, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21}; // 8
static const short type2ctx_one [] = { 0,  1,  2,  3,  3,  4,  5,  6,  5,  5, 10, 11, 12, 13, 13, 14, 16, 17, 18, 19, 19, 20}; // 7
static const short type2ctx_abs [] = { 0,  1,  2,  3,  3,  4,  5,  6,  5,  5, 10, 11, 12, 13, 13, 14, 16, 17, 18, 19, 19, 20}; // 7
static const short max_c2       [] = { 4,  4,  4,  4,  4,  4,  3,  4,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4}; // 9

//===== position -> ctx for MAP =====
//--- zig-zag scan ----
static const uint8_t  pos2ctx_map8x8 [] = { 0,  1,  2,  3,  4,  5,  5,  4,  4,  3,  3,  4,  4,  4,  5,  5,
                                         4,  4,  4,  4,  3,  3,  6,  7,  7,  7,  8,  9, 10,  9,  8,  7,
                                         7,  6, 11, 12, 13, 11,  6,  7,  8,  9, 14, 10,  9,  8,  6, 11,
                                        12, 13, 11,  6,  9, 14, 10,  9, 11, 12, 13, 11 ,14, 10, 12, 14}; // 15 CTX

static const uint8_t  pos2ctx_map8x4 [] = { 0,  1,  2,  3,  4,  5,  7,  8,  9, 10, 11,  9,  8,  6,  7,  8,
                                         9, 10, 11,  9,  8,  6, 12,  8,  9, 10, 11,  9, 13, 13, 14, 14}; // 15 CTX

static const uint8_t  pos2ctx_map4x4 [] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 14}; // 15 CTX
static const uint8_t  pos2ctx_map2x4c[] = { 0,  0,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
static const uint8_t  pos2ctx_map4x4c[] = { 0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX

static const uint8_t* pos2ctx_map    [] = {pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8, pos2ctx_map8x4,
                                        pos2ctx_map8x4, pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map4x4,
                                        pos2ctx_map2x4c, pos2ctx_map4x4c,
                                        pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8,pos2ctx_map8x4,
                                        pos2ctx_map8x4, pos2ctx_map4x4,
                                        pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8,pos2ctx_map8x4,
                                        pos2ctx_map8x4,pos2ctx_map4x4};

//--- interlace scan ----
//taken from ABT
//{{{
static const uint8_t  pos2ctx_map8x8i[] = { 0,  1,  1,  2,  2,  3,  3,  4,  5,  6,  7,  7,  7,  8,  4,  5,
                                         6,  9, 10, 10,  8, 11, 12, 11,  9,  9, 10, 10,  8, 11, 12, 11,
                                         9,  9, 10, 10,  8, 11, 12, 11,  9,  9, 10, 10,  8, 13, 13,  9,
                                         9, 10, 10,  8, 13, 13,  9,  9, 10, 10, 14, 14, 14, 14, 14, 14}; // 15 CTX
//}}}
//{{{
static const uint8_t  pos2ctx_map8x4i[] = { 0,  1,  2,  3,  4,  5,  6,  3,  4,  5,  6,  3,  4,  7,  6,  8,
                                         9,  7,  6,  8,  9, 10, 11, 12, 12, 10, 11, 13, 13, 14, 14, 14}; // 15 CTX
//}}}
//{{{
static const uint8_t  pos2ctx_map4x8i[] = { 0,  1,  1,  1,  2,  3,  3,  4,  4,  4,  5,  6,  2,  7,  7,  8,
                                         8,  8,  5,  6,  9, 10, 10, 11, 11, 11, 12, 13, 13, 14, 14, 14}; // 15 CTX
//}}}
//{{{
static const uint8_t* pos2ctx_map_int[] = {pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
                                        pos2ctx_map4x8i,pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map4x4,
                                        pos2ctx_map2x4c, pos2ctx_map4x4c,
                                        pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
                                        pos2ctx_map8x4i,pos2ctx_map4x4,
                                        pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
                                        pos2ctx_map8x4i,pos2ctx_map4x4};
//}}}

//===== position -> ctx for LAST =====
//{{{
static const uint8_t  pos2ctx_last8x8 [] = { 0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
                                          2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
                                          3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
                                          5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8}; //  9 CTX
//}}}
//{{{
static const uint8_t  pos2ctx_last8x4 [] = { 0,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,
                                          3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8}; //  9 CTX
//}}}

static const uint8_t  pos2ctx_last4x4 [] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15}; // 15 CTX
static const uint8_t  pos2ctx_last2x4c[] = { 0,  0,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
static const uint8_t  pos2ctx_last4x4c[] = { 0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX

static const uint8_t* pos2ctx_last    [] = {pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8, pos2ctx_last8x4,
                                         pos2ctx_last8x4, pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last4x4,
                                         pos2ctx_last2x4c, pos2ctx_last4x4c,
                                         pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8,pos2ctx_last8x4,
                                         pos2ctx_last8x4, pos2ctx_last4x4,
                                         pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8,pos2ctx_last8x4,
                                         pos2ctx_last8x4, pos2ctx_last4x4};
//}}}

//{{{
static uint32_t unary_bin_max_decode (sCabacDecodeEnv* cabacDecodeEnv, sBiContext* context,
                                          int ctx_offset, uint32_t max_symbol) {

  uint32_t symbol =  binaryArithmeticDecodeSymbol (cabacDecodeEnv, context );

  if (symbol == 0 || (max_symbol == 0))
    return symbol;
  else {
    uint32_t l;
    context += ctx_offset;
    symbol = 0;
    do {
      l = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context);
      ++symbol;
      }
    while( (l != 0) && (symbol < max_symbol) );

    if ((l != 0) && (symbol == max_symbol))
      ++symbol;
    return symbol;
    }
  }
//}}}
//{{{
static uint32_t unary_bin_decode (sCabacDecodeEnv* cabacDecodeEnv, sBiContext* context, int ctx_offset) {

  uint32_t symbol = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context);

  if (symbol == 0)
    return 0;
  else {
    uint32_t l;
    context += ctx_offset;;
    symbol = 0;
    do {
      l = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context);
      ++symbol;
      }
    while (l != 0);

    return symbol;
    }
  }
//}}}
//{{{
static uint32_t exp_golomb_decode_eq_prob (sCabacDecodeEnv* cabacDecodeEnv, int k) {

  uint32_t l;
  int symbol = 0;
  int binary_symbol = 0;

  do {
    l = binaryArithmeticDecodeSymbolEqProb(cabacDecodeEnv);
    if (l == 1) {
      symbol += (1<<k);
      ++k;
      }
    } while (l!=0);

  while (k--)
    // next binary part
    if (binaryArithmeticDecodeSymbolEqProb(cabacDecodeEnv)==1)
      binary_symbol |= (1 << k);

  return (uint32_t)(symbol + binary_symbol);
  }
//}}}
//{{{
static uint32_t unary_exp_golomb_level_decode (sCabacDecodeEnv* cabacDecodeEnv, sBiContext* context) {

  uint32_t symbol = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context );

  if (symbol == 0)
    return 0;
  else {
    uint32_t l, k = 1;
    uint32_t exp_start = 13;
    symbol = 0;
    do {
      l = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context);
      ++symbol;
      ++k;
      } while ((l != 0) && (k != exp_start));

    if (l != 0)
      symbol += exp_golomb_decode_eq_prob (cabacDecodeEnv,0)+1;
    return symbol;
    }
  }
//}}}
//{{{
static uint32_t unary_exp_golomb_mv_decode (sCabacDecodeEnv* cabacDecodeEnv, sBiContext* context, uint32_t max_bin) {

  uint32_t symbol = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context );

  if (symbol == 0)
    return 0;

  else {
    uint32_t exp_start = 8;
    uint32_t l,k = 1;
    uint32_t bin = 1;
    symbol = 0;
    ++context;
    do {
      l = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context);
      if ((++bin) == 2)
        context++;
      if (bin == max_bin)
        ++context;
      ++symbol;
      ++k;
      } while ((l != 0) && (k != exp_start));

    if (l != 0)
      symbol += exp_golomb_decode_eq_prob (cabacDecodeEnv, 3) + 1;

    return symbol;
    }
  }
//}}}

//{{{
void cabacNewSlice (sSlice* slice) {
  slice->lastDquant = 0;
  }
//}}}
//{{{
int cabacStartCode (sSlice* slice, int eos_bit) {

  uint32_t bit;
  if (eos_bit) {
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    sCabacDecodeEnv* cabacDecodeEnv = &dataPartition->cabacDecodeEnv;
    bit = binaryArithmeticDecodeFinal (cabacDecodeEnv);
    }
  else
    bit = 0;

  return bit == 1 ? 1 : 0;
  }
//}}}
//{{{
void checkNeighbourCabac (sMacroBlock* mb) {

  sDecoder* decoder = mb->decoder;
  int* mbSize = decoder->mbSize[eLuma];

  sPixelPos up;
  decoder->getNeighbour (mb,  0, -1, mbSize, &up);
  if (up.available)
    mb->mbCabacUp = &mb->slice->mbData[up.mbIndex];
  else
    mb->mbCabacUp = NULL;

  sPixelPos left;
  decoder->getNeighbour (mb, -1,  0, mbSize, &left);
  if (left.available)
    mb->mbCabacLeft = &mb->slice->mbData[left.mbIndex];
  else
    mb->mbCabacLeft = NULL;
  }
//}}}

//{{{
sMotionContexts* createMotionInfoContexts() {

  sMotionContexts* contexts = (sMotionContexts*)calloc (1, sizeof(sMotionContexts));
  if (contexts == NULL)
    noMemoryExit ("createMotionInfoContexts");

  return contexts;
  }
//}}}
//{{{
sTextureContexts* createTextureInfoContexts() {

  sTextureContexts* contexts = (sTextureContexts*)calloc (1, sizeof(sTextureContexts));
  if (!contexts)
    noMemoryExit ("createTextureInfoContexts");

  return contexts;
  }
//}}}
//{{{
void deleteMotionInfoContexts (sMotionContexts* contexts) {
  free (contexts);
  }
//}}}
//{{{
void deleteTextureInfoContexts (sTextureContexts* contexts) {
  free (contexts);
  }
//}}}

//{{{
void readFieldModeInfo_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;
  sMotionContexts* context  = slice->motionInfoContexts;

  int a = mb->mbAvailA ? slice->mbData[mb->mbIndexA].mbField : 0;
  int b = mb->mbAvailB ? slice->mbData[mb->mbIndexB].mbField : 0;
  int actContext = a + b;

  se->value1 = binaryArithmeticDecodeSymbol (cabacDecodeEnv, &context->mbAffContexts[actContext]);
  }
//}}}

//{{{
int checkNextMbGetFieldModeCabacSliceP (sSlice* slice, sSyntaxElement* se,
                                                    sDataPartition* act_dp) {

  sDecoder* decoder = slice->decoder;
  sBiContext* mb_type_ctx_copy[3];
  sBiContext* mb_aff_ctx_copy;
  sCabacDecodeEnv* decodingEnv_copy;
  sMotionContexts* motionInfoContexts  = slice->motionInfoContexts;

  int length;
  sCabacDecodeEnv* cabacDecodeEnv = &act_dp->cabacDecodeEnv;

  int skip = 0;
  int field = 0;
  sMacroBlock* mb;

  // get next MB
  ++slice->mbIndex;

  mb = &slice->mbData[slice->mbIndex];
  mb->decoder = decoder;
  mb->slice = slice;
  mb->sliceNum = slice->curSliceIndex;
  mb->mbField = slice->mbData[slice->mbIndex-1].mbField;
  mb->mbIndexX  = slice->mbIndex;
  mb->listOffset = ((slice->mbAffFrame) && (mb->mbField))? (mb->mbIndexX&0x01) ? 4 : 2 : 0;
  checkNeighboursMbAff (mb);
  checkNeighbourCabac (mb);

  // create
  decodingEnv_copy = (sCabacDecodeEnv*)calloc (1, sizeof(sCabacDecodeEnv));
  for (int i = 0; i < 3;++i)
    mb_type_ctx_copy[i] = (sBiContext*) calloc(NUM_MB_TYPE_CTX, sizeof(sBiContext) );
  mb_aff_ctx_copy = (sBiContext*) calloc(NUM_MB_AFF_CTX, sizeof(sBiContext) );

  // copy
  memcpy (decodingEnv_copy,cabacDecodeEnv,sizeof(sCabacDecodeEnv));
  length = *(decodingEnv_copy->codeStreamLen) = *(cabacDecodeEnv->codeStreamLen);
  for (int i=0;i<3;++i)
    memcpy (mb_type_ctx_copy[i], motionInfoContexts->mbTypeContexts[i],NUM_MB_TYPE_CTX*sizeof(sBiContext) );
  memcpy (mb_aff_ctx_copy, motionInfoContexts->mbAffContexts,NUM_MB_AFF_CTX*sizeof(sBiContext) );

  // check_next_mb
  slice->lastDquant = 0;
  read_skip_flag_CABAC_p_slice (mb, se, cabacDecodeEnv);

  skip = (se->value1 == 0);

  if (!skip) {
    readFieldModeInfo_CABAC (mb, se,cabacDecodeEnv);
    field = se->value1;
    slice->mbData[slice->mbIndex-1].mbField = field;
    }

  // reset
  slice->mbIndex--;

  memcpy (cabacDecodeEnv, decodingEnv_copy, sizeof(sCabacDecodeEnv));
  *(cabacDecodeEnv->codeStreamLen) = length;
  for (int i = 0; i < 3; ++i)
    memcpy (motionInfoContexts->mbTypeContexts[i], mb_type_ctx_copy[i], NUM_MB_TYPE_CTX * sizeof(sBiContext));
  memcpy (motionInfoContexts->mbAffContexts, mb_aff_ctx_copy, NUM_MB_AFF_CTX * sizeof(sBiContext));

  checkNeighbourCabac (mb);

  // delete
  free (decodingEnv_copy);
  for (int i = 0; i < 3; ++i)
    free (mb_type_ctx_copy[i]);
  free (mb_aff_ctx_copy);

  return skip;
  }
//}}}
//{{{
int checkNextMbGetFieldModeCabacSliceB (sSlice* slice, sSyntaxElement* se, sDataPartition  *act_dp) {

  sDecoder* decoder = slice->decoder;
  sBiContext* mb_type_ctx_copy[3];
  sBiContext* mb_aff_ctx_copy;
  sCabacDecodeEnv* decodingEnv_copy;

  int length;
  sCabacDecodeEnv* cabacDecodeEnv = &act_dp->cabacDecodeEnv;
  sMotionContexts* motionInfoContexts = slice->motionInfoContexts;

  int skip = 0;
  int field = 0;
  int i;

  sMacroBlock* mb;

  // get next MB
  ++slice->mbIndex;

  mb = &slice->mbData[slice->mbIndex];
  mb->decoder = decoder;
  mb->slice = slice;
  mb->sliceNum = slice->curSliceIndex;
  mb->mbField = slice->mbData[slice->mbIndex-1].mbField;
  mb->mbIndexX  = slice->mbIndex;
  mb->listOffset = ((slice->mbAffFrame)&&(mb->mbField))? (mb->mbIndexX & 0x01) ? 4 : 2 : 0;
  checkNeighboursMbAff (mb);
  checkNeighbourCabac (mb);

  // create
  decodingEnv_copy = (sCabacDecodeEnv*) calloc(1, sizeof(sCabacDecodeEnv) );
  for (i = 0; i < 3; ++i)
    mb_type_ctx_copy[i] = (sBiContext*)calloc (NUM_MB_TYPE_CTX, sizeof(sBiContext) );
  mb_aff_ctx_copy = (sBiContext*)calloc (NUM_MB_AFF_CTX, sizeof(sBiContext) );

  //copy
  memcpy (decodingEnv_copy, cabacDecodeEnv, sizeof(sCabacDecodeEnv));
  length = *(decodingEnv_copy->codeStreamLen) = *(cabacDecodeEnv->codeStreamLen);

  for (i = 0; i < 3;++i)
    memcpy (mb_type_ctx_copy[i], motionInfoContexts->mbTypeContexts[i], NUM_MB_TYPE_CTX * sizeof(sBiContext) );
  memcpy (mb_aff_ctx_copy, motionInfoContexts->mbAffContexts, NUM_MB_AFF_CTX * sizeof(sBiContext) );

  //  check_next_mb
  slice->lastDquant = 0;
  read_skip_flag_CABAC_b_slice (mb, se, cabacDecodeEnv);

  skip = (se->value1 == 0 && se->value2 == 0);
  if (!skip) {
    readFieldModeInfo_CABAC (mb, se,cabacDecodeEnv);
    field = se->value1;
    slice->mbData[slice->mbIndex-1].mbField = field;
    }

  // reset
  slice->mbIndex--;

  memcpy(cabacDecodeEnv,decodingEnv_copy,sizeof(sCabacDecodeEnv));
  *(cabacDecodeEnv->codeStreamLen) = length;

  for (i = 0; i < 3; ++i)
    memcpy (motionInfoContexts->mbTypeContexts[i], mb_type_ctx_copy[i], NUM_MB_TYPE_CTX * sizeof(sBiContext) );
  memcpy (motionInfoContexts->mbAffContexts, mb_aff_ctx_copy, NUM_MB_AFF_CTX * sizeof(sBiContext) );

  checkNeighbourCabac (mb);

  // delete
  free (decodingEnv_copy);
  for (i = 0; i < 3; ++i)
    free (mb_type_ctx_copy[i]);
  free (mb_aff_ctx_copy);

  return skip;
  }
//}}}

//{{{
void read_MVD_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  int* mbSize = mb->decoder->mbSize[eLuma];
  sSlice* slice = mb->slice;
  sMotionContexts* ctx = slice->motionInfoContexts;
  int i = mb->subblockX;
  int j = mb->subblockY;
  int a = 0;
  int actSym;
  int list_idx = se->value2 & 0x01;
  int k = (se->value2 >> 1); // MVD component

  sPixelPos block_a, block_b;
  get4x4NeighbourBase (mb, i - 1, j    , mbSize, &block_a);
  get4x4NeighbourBase (mb, i    , j - 1, mbSize, &block_b);
  if (block_a.available)
    a = iabs (slice->mbData[block_a.mbIndex].mvd[list_idx][block_a.y][block_a.x][k]);
  if (block_b.available)
    a += iabs (slice->mbData[block_b.mbIndex].mvd[list_idx][block_b.y][block_b.x][k]);

  if (a < 3)
    a = 5 * k;
  else if (a > 32)
    a = 5 * k + 3;
  else
    a = 5 * k + 2;

  se->context = a;

  actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, ctx->mvResContexts[0] + a );

  if (actSym != 0) {
    a = 5 * k;
    actSym = unary_exp_golomb_mv_decode (cabacDecodeEnv, ctx->mvResContexts[1] + a, 3) + 1;

    if(binaryArithmeticDecodeSymbolEqProb (cabacDecodeEnv))
      actSym = -actSym;
    }

  se->value1 = actSym;
  }
//}}}
//{{{
void read_mvd_CABAC_mbaff (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  sMotionContexts *ctx = slice->motionInfoContexts;
  int i = mb->subblockX;
  int j = mb->subblockY;
  int a = 0, b = 0;
  int actContext;
  int actSym;
  int list_idx = se->value2 & 0x01;
  int k = (se->value2 >> 1); // MVD component

  sPixelPos block_a, block_b;
  get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
  if (block_a.available) {
    a = iabs (slice->mbData[block_a.mbIndex].mvd[list_idx][block_a.y][block_a.x][k]);
    if (slice->mbAffFrame && (k == 1)) {
      if ((mb->mbField == 0) && (slice->mbData[block_a.mbIndex].mbField==1))
        a *= 2;
      else if ((mb->mbField == 1) && (slice->mbData[block_a.mbIndex].mbField==0))
        a /= 2;
      }
    }

  get4x4NeighbourBase(mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);
  if (block_b.available) {
    b = iabs(slice->mbData[block_b.mbIndex].mvd[list_idx][block_b.y][block_b.x][k]);
    if (slice->mbAffFrame && (k==1)) {
      if ((mb->mbField == 0) && (slice->mbData[block_b.mbIndex].mbField==1))
        b *= 2;
      else if ((mb->mbField==1) && (slice->mbData[block_b.mbIndex].mbField==0))
        b /= 2;
      }
    }
  a += b;

  if (a < 3)
    actContext = 5 * k;
  else if (a > 32)
    actContext = 5 * k + 3;
  else
    actContext = 5 * k + 2;
  se->context = actContext;

  actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, &ctx->mvResContexts[0][actContext] );
  if (actSym != 0) {
    actContext = 5 * k;
    actSym = unary_exp_golomb_mv_decode (cabacDecodeEnv, ctx->mvResContexts[1] + actContext, 3) + 1;
    if (binaryArithmeticDecodeSymbolEqProb (cabacDecodeEnv))
      actSym = -actSym;
    }

  se->value1 = actSym;
  }
//}}}
//{{{
void readB8_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;

  sMotionContexts *ctx = slice->motionInfoContexts;
  sBiContext* b8TypeContexts = &ctx->b8TypeContexts[0][1];

  int actSym = 0;
  if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts++))
    actSym = 0;
  else {
    if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, ++b8TypeContexts))
      actSym = (binaryArithmeticDecodeSymbol (cabacDecodeEnv, ++b8TypeContexts))? 2: 3;
    else
      actSym = 1;
    }

  se->value1 = actSym;
  }
//}}}
//{{{
void readB8_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;

  int actSym = 0;
  sMotionContexts* ctx = slice->motionInfoContexts;
  sBiContext* b8TypeContexts = ctx->b8TypeContexts[1];
  if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts++)) {
    if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts++)) {
      if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts++)) {
        if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts)) {
          actSym = 10;
          if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts))
            actSym++;
          }
        else {
          actSym = 6;
          if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts))
            actSym += 2;
          if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts))
            actSym++;
          }
        }
      else {
        actSym = 2;
        if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts))
          actSym += 2;
        if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, b8TypeContexts))
          actSym ++;
        }
      }
    else
      actSym = (binaryArithmeticDecodeSymbol (cabacDecodeEnv, ++b8TypeContexts)) ? 1: 0;
    ++actSym;
    }
  else
    actSym = 0;

  se->value1 = actSym;
  }
//}}}
//{{{
void read_skip_flag_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  int a = (mb->mbCabacLeft != NULL) ? (mb->mbCabacLeft->skipFlag == 0) : 0;
  int b = (mb->mbCabacUp   != NULL) ? (mb->mbCabacUp  ->skipFlag == 0) : 0;

  sBiContext *mbTypeContexts = &mb->slice->motionInfoContexts->mbTypeContexts[1][a + b];
  se->value1 = binaryArithmeticDecodeSymbol (cabacDecodeEnv, mbTypeContexts) != 1;

  if (!se->value1)
    mb->slice->lastDquant = 0;
  }
//}}}
//{{{
void read_skip_flag_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  int a = (mb->mbCabacLeft != NULL) ? (mb->mbCabacLeft->skipFlag == 0) : 0;
  int b = (mb->mbCabacUp != NULL) ? (mb->mbCabacUp->skipFlag == 0) : 0;
  sBiContext* mbTypeContexts = &mb->slice->motionInfoContexts->mbTypeContexts[2][7 + a + b];

  se->value1 = se->value2 = (binaryArithmeticDecodeSymbol (cabacDecodeEnv, mbTypeContexts) != 1);
  if (!se->value1)
    mb->slice->lastDquant = 0;
  }
//}}}

//{{{
void readMB_transform_size_flag_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;
  sTextureContexts*ctx = slice->textureInfoContexts;

  int b = (mb->mbCabacUp == NULL) ? 0 : mb->mbCabacUp->lumaTransformSize8x8flag;
  int a = (mb->mbCabacLeft == NULL) ? 0 : mb->mbCabacLeft->lumaTransformSize8x8flag;

  int actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, ctx->transformSizeContexts + a + b );
  se->value1 = actSym;
  }
//}}}
//{{{
void readMB_typeInfo_CABAC_i_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;
  sMotionContexts* context = slice->motionInfoContexts;

  int actContext;
  int actSym;
  int modeSym;
  int curMbType = 0;

  int a = 0;
  int b = 0;
  if (slice->sliceType == eSliceI)  {
    //{{{  INTRA-frame
    if (mb->mbCabacUp)
      b = (((mb->mbCabacUp)->mbType != I4MB && mb->mbCabacUp->mbType != I8MB) ? 1 : 0 );
    if (mb->mbCabacLeft)
      a = (((mb->mbCabacLeft)->mbType != I4MB && mb->mbCabacLeft->mbType != I8MB) ? 1 : 0 );

    actContext = a + b;
    actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext);
    se->context = actContext; // store context

    if (actSym == 0) // 4x4 Intra
      curMbType = actSym;
    else {
      // 16x16 Intra
      modeSym = binaryArithmeticDecodeFinal (cabacDecodeEnv);
      if(modeSym == 1)
        curMbType = 25;
      else {
        actSym = 1;
        actContext = 4;
        modeSym =  binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext ); // decoding of AC/no AC
        actSym += modeSym*12;
        actContext = 5;
        // decoding of codedBlockPattern: 0,1,2
        modeSym =  binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext );
        if (modeSym != 0) {
          actContext = 6;
          modeSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext );
          actSym += 4;
          if (modeSym != 0)
            actSym += 4;
        }
        // decoding of I pred-mode: 0,1,2,3
        actContext = 7;
        modeSym =  binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext );
        actSym += modeSym * 2;
        actContext = 8;
        modeSym =  binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext );
        actSym += modeSym;
        curMbType = actSym;
        }
      }
    }
    //}}}
  else if(slice->sliceType == eSliceSI)  {
    //{{{  SI-frame
    // special context's for SI4MB
    if (mb->mbCabacUp)
      b = ((mb->mbCabacUp)->mbType != SI4MB) ? 1 : 0;
    if (mb->mbCabacLeft)
      a = ((mb->mbCabacLeft)->mbType != SI4MB) ? 1 : 0;

    actContext = a + b;
    actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[1] + actContext);
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
      actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext);
      se->context = actContext; // store context

      if (actSym==0) // 4x4 Intra
        curMbType = 1;
      else {
        // 16x16 Intra
        modeSym = binaryArithmeticDecodeFinal (cabacDecodeEnv);
        if( modeSym==1 )
          curMbType = 26;
        else {
          actSym = 2;
          actContext = 4;
          modeSym =  binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext ); // decoding of AC/no AC
          actSym += modeSym*12;
          actContext = 5;
          // decoding of codedBlockPattern: 0,1,2
          modeSym =  binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext );
          if (modeSym != 0) {
            actContext = 6;
            modeSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext );
            actSym += 4;
            if (modeSym != 0)
              actSym += 4;
            }

          // decoding of I pred-mode: 0,1,2,3
          actContext = 7;
          modeSym =  binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext );
          actSym += modeSym * 2;
          actContext = 8;
          modeSym =  binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->mbTypeContexts[0] + actContext );
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
void readMB_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;
  sMotionContexts* ctx = slice->motionInfoContexts;

  int actContext;
  int actSym;
  int modeSym;
  int curMbType;

  sBiContext* mbTypeContexts = ctx->mbTypeContexts[1];
  if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[4] )) {
    if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[7] ))
      actSym = 7;
    else
      actSym = 6;
    }
  else {
    if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[5] )) {
      if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[7] ))
        actSym = 2;
      else
        actSym = 3;
      }
    else {
      if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[6] ))
        actSym = 4;
      else
        actSym = 1;
      }
    }

  if (actSym <= 6)
    curMbType = actSym;
  else  {
    // additional info for 16x16 Intra-mode
    modeSym = binaryArithmeticDecodeFinal (cabacDecodeEnv);
    if (modeSym == 1)
      curMbType = 31;
    else {
      actContext = 8;
      modeSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, mbTypeContexts + actContext ); // decoding of AC/no AC
      actSym += modeSym*12;

      // decoding of codedBlockPattern: 0,1,2
      actContext = 9;
      modeSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, mbTypeContexts + actContext );
      if (modeSym != 0) {
        actSym += 4;
        modeSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, mbTypeContexts + actContext );
        if (modeSym != 0)
          actSym += 4;
        }

      // decoding of I pred-mode: 0,1,2,3
      actContext = 10;
      modeSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, mbTypeContexts + actContext );
      actSym += modeSym*2;
      modeSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, mbTypeContexts + actContext );
      actSym += modeSym;
      curMbType = actSym;
      }
    }

  se->value1 = curMbType;
  }
//}}}
//{{{
void readMB_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;
  sMotionContexts* ctx = slice->motionInfoContexts;

  int actContext;
  int actSym;
  int modeSym;
  int curMbType;
  sBiContext* mbTypeContexts = ctx->mbTypeContexts[2];

  int b = 0;
  if (mb->mbCabacUp)
    b = ((mb->mbCabacUp)->mbType != 0) ? 1 : 0;
  int a = 0;
  if (mb->mbCabacLeft)
    a = ((mb->mbCabacLeft)->mbType != 0) ? 1 : 0;
  actContext = a + b;

  if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[actContext])) {
    if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[4])) {
      if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[5])) {
        actSym = 12;
        if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[6]))
          actSym += 8;
        if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[6]))
          actSym += 4;
        if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[6]))
          actSym += 2;

        if (actSym == 24)
          actSym = 11;
        else if (actSym == 26)
          actSym = 22;
        else {
          if (actSym == 22)
            actSym = 23;
          if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[6]))
            actSym += 1;
          }
        }
      else {
        actSym = 3;
        if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[6]))
          actSym += 4;
        if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[6]))
          actSym += 2;
        if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[6]))
          actSym += 1;
        }
      }
    else {
      if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, &mbTypeContexts[6]))
        actSym = 2;
      else
        actSym = 1;
      }
    }
  else
    actSym = 0;

  if (actSym <= 23)
    curMbType = actSym;
  else  {
    // additional info for 16x16 Intra-mode
    modeSym = binaryArithmeticDecodeFinal(cabacDecodeEnv);
    if (modeSym == 1)
      curMbType = 48;
    else {
      mbTypeContexts = ctx->mbTypeContexts[1];
      actContext = 8;
      modeSym =  binaryArithmeticDecodeSymbol(cabacDecodeEnv, mbTypeContexts + actContext ); // decoding of AC/no AC
      actSym += modeSym*12;

      // decoding of codedBlockPattern: 0,1,2
      actContext = 9;
      modeSym = binaryArithmeticDecodeSymbol(cabacDecodeEnv, mbTypeContexts + actContext );
      if (modeSym != 0) {
        actSym += 4;
        modeSym = binaryArithmeticDecodeSymbol(cabacDecodeEnv, mbTypeContexts + actContext );
        if (modeSym != 0)
          actSym += 4;
        }

      // decoding of I pred-mode: 0,1,2,3
      actContext = 10;
      modeSym = binaryArithmeticDecodeSymbol(cabacDecodeEnv, mbTypeContexts + actContext );
      actSym += modeSym*2;
      modeSym = binaryArithmeticDecodeSymbol(cabacDecodeEnv, mbTypeContexts + actContext );
      actSym += modeSym;
      curMbType = actSym;
      }
    }

  se->value1 = curMbType;
  }
//}}}

//{{{
void readIntraPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;
  sTextureContexts* context = slice->textureInfoContexts;

  // use_most_probable_mode
  int actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->iprContexts);

  // remaining_mode_selector
  if (actSym == 1)
    se->value1 = -1;
  else {
    se->value1  = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->iprContexts + 1);
    se->value1 |= binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->iprContexts + 1) << 1;
    se->value1 |= binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->iprContexts + 1) << 2;
    }
  }
//}}}
//{{{
void readRefFrame_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture;
  sMotionContexts* ctx = slice->motionInfoContexts;
  sMacroBlock* neighborMB = NULL;

  int addctx  = 0;
  int a = 0, b = 0;
  int actContext;
  int actSym;
  int list = se->value2;

  sPixelPos block_a, block_b;
  get4x4Neighbour (mb, mb->subblockX - 1, mb->subblockY    , decoder->mbSize[eLuma], &block_a);
  get4x4Neighbour (mb, mb->subblockX,     mb->subblockY - 1, decoder->mbSize[eLuma], &block_b);

  if (block_b.available) {
    int b8b = ((block_b.x >> 1) & 0x01) + (block_b.y & 0x02);
    neighborMB = &slice->mbData[block_b.mbIndex];
    if (!((neighborMB->mbType == IPCM) || IS_DIRECT(neighborMB) ||
        (neighborMB->b8mode[b8b]==0 && neighborMB->b8pdir[b8b] == 2))) {
      if (slice->mbAffFrame && (mb->mbField == false) && (neighborMB->mbField == true))
        b = (picture->mvInfo[block_b.posY][block_b.posX].refIndex[list] > 1 ? 2 : 0);
      else
        b = (picture->mvInfo[block_b.posY][block_b.posX].refIndex[list] > 0 ? 2 : 0);
      }
    }

  if (block_a.available) {
    int b8a = ((block_a.x >> 1) & 0x01) + (block_a.y & 0x02);
    neighborMB = &slice->mbData[block_a.mbIndex];
    if (!((neighborMB->mbType==IPCM) || IS_DIRECT(neighborMB) ||
        (neighborMB->b8mode[b8a]==0 && neighborMB->b8pdir[b8a]==2))) {
      if (slice->mbAffFrame && (mb->mbField == false) && (neighborMB->mbField == 1))
        a = (picture->mvInfo[block_a.posY][block_a.posX].refIndex[list] > 1 ? 1 : 0);
      else
        a = (picture->mvInfo[block_a.posY][block_a.posX].refIndex[list] > 0 ? 1 : 0);
      }
    }

  actContext = a + b;
  se->context = actContext; // store context

  actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv,ctx->refNoContexts[addctx] + actContext );

  if (actSym != 0) {
    actContext = 4;
    actSym = unary_bin_decode (cabacDecodeEnv,ctx->refNoContexts[addctx] + actContext,1);
    ++actSym;
    }

  se->value1 = actSym;
  }
//}}}
//{{{
void read_dQuant_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;
  sMotionContexts* context = slice->motionInfoContexts;

  int* dquant = &se->value1;
  int actContext = (slice->lastDquant != 0) ? 1 : 0;
  int actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->deltaQpContexts + actContext);

  if (actSym != 0) {
    actContext = 2;
    actSym = unary_bin_decode (cabacDecodeEnv, context->deltaQpContexts + actContext, 1);
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
void read_CBP_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sDecoder* decoder = mb->decoder;
  sPicture* picture = mb->slice->picture;
  sSlice* slice = mb->slice;
  sTextureContexts *ctx = slice->textureInfoContexts;
  sMacroBlock *neighborMB = NULL;

  int mb_x, mb_y;
  int a = 0, b = 0;
  int curr_cbp_ctx;
  int codedBlockPattern = 0;
  int codedBlockPatternBit;
  int mask;
  sPixelPos block_a;

  //  coding of luma part (bit by bit)
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
        if (block_a.available) {
          if (slice->mbData[block_a.mbIndex].mbType==IPCM)
            a = 0;
          else
            a = (( (slice->mbData[block_a.mbIndex].codedBlockPattern & (1<<(2*(block_a.y/2)+1))) == 0) ? 1 : 0);
          }
        else
          a = 0;
        }
      else
        a = ( ((codedBlockPattern & (1<<mb_y)) == 0) ? 1: 0);

      curr_cbp_ctx = a + b;
      mask = (1 << (mb_y + (mb_x >> 1)));
      codedBlockPatternBit = binaryArithmeticDecodeSymbol(cabacDecodeEnv, ctx->cbpContexts[0] + curr_cbp_ctx );
      if (codedBlockPatternBit)
        codedBlockPattern += mask;
      }
    }

  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
    // coding of chroma part eCabac decoding for BinIdx 0
    b = 0;
    neighborMB = mb->mbCabacUp;
    if (neighborMB != NULL)
      if (neighborMB->mbType==IPCM || (neighborMB->codedBlockPattern > 15))
        b = 2;

    a = 0;
    neighborMB = mb->mbCabacLeft;
    if (neighborMB != NULL)
      if (neighborMB->mbType==IPCM || (neighborMB->codedBlockPattern > 15))
        a = 1;

    curr_cbp_ctx = a + b;
    codedBlockPatternBit = binaryArithmeticDecodeSymbol(cabacDecodeEnv, ctx->cbpContexts[1] + curr_cbp_ctx );

    // eCabac decoding for BinIdx 1
    if (codedBlockPatternBit) {
      // set the chroma bits
      b = 0;
      neighborMB = mb->mbCabacUp;
      if (neighborMB != NULL)
        if ((neighborMB->mbType == IPCM) || ((neighborMB->codedBlockPattern >> 4) == 2))
          b = 2;

      a = 0;
      neighborMB = mb->mbCabacLeft;
      if (neighborMB != NULL)
        if ((neighborMB->mbType == IPCM) || ((neighborMB->codedBlockPattern >> 4) == 2))
          a = 1;

      curr_cbp_ctx = a + b;
      codedBlockPatternBit = binaryArithmeticDecodeSymbol(cabacDecodeEnv, ctx->cbpContexts[2] + curr_cbp_ctx );
      codedBlockPattern += (codedBlockPatternBit == 1) ? 32 : 16;
      }
    }

  se->value1 = codedBlockPattern;

  if (!codedBlockPattern)
    slice->lastDquant = 0;
  }
//}}}
//{{{
void readCIPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;

  sTextureContexts* context = slice->textureInfoContexts;
  int* actSym  = &se->value1;

  sMacroBlock* MbUp = mb->mbCabacUp;
  sMacroBlock* MbLeft = mb->mbCabacLeft;

  int b = (MbUp != NULL) ? (((MbUp->chromaPredMode   != 0) && (MbUp->mbType != IPCM)) ? 1 : 0) : 0;
  int a = (MbLeft != NULL) ? (((MbLeft->chromaPredMode != 0) && (MbLeft->mbType != IPCM)) ? 1 : 0) : 0;
  int actContext = a + b;

  *actSym = binaryArithmeticDecodeSymbol (cabacDecodeEnv, context->ciprContexts + actContext );

  if (*actSym != 0)
    *actSym = unary_bin_max_decode (cabacDecodeEnv, context->ciprContexts + 3, 0, 1) + 1;
  }
//}}}

//{{{
static int readStoreCbpBlockBit444 (sMacroBlock* mb, sCabacDecodeEnv*  cabacDecodeEnv, int type) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture;
  sTextureContexts* textureInfoContexts = slice->textureInfoContexts;
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
    get4x4Neighbour (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
    get4x4Neighbour (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);
    if (block_a.available)
        bit_pos_a = 4*block_a.y + block_a.x;
    if (block_b.available)
      bit_pos_b = 4*block_b.y + block_b.x;
    }
  else if (y_dc) {
    get4x4Neighbour(mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
    get4x4Neighbour(mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);
    }
  else if (u_ac||v_ac) {
    get4x4Neighbour(mb, i - 1, j    , decoder->mbSize[eChroma], &block_a);
    get4x4Neighbour(mb, i    , j - 1, decoder->mbSize[eChroma], &block_b);
    if (block_a.available)
      bit_pos_a = 4*block_a.y + block_a.x;
    if (block_b.available)
      bit_pos_b = 4*block_b.y + block_b.x;
    }
  else {
    get4x4Neighbour(mb, i - 1, j    , decoder->mbSize[eChroma], &block_a);
    get4x4Neighbour(mb, i    , j - 1, decoder->mbSize[eChroma], &block_b);
    }

  if (picture->chromaFormatIdc!=YUV444) {
    if (type != LUMA_8x8) {
      // get bits from neighboring blocks ---
      if (block_b.available) {
        if(mbData[block_b.mbIndex].mbType==IPCM)
          upper_bit=1;
        else
          upper_bit = getBit (mbData[block_b.mbIndex].codedBlockPatterns[0].bits, bit + bit_pos_b);
        }

      if (block_a.available) {
        if(mbData[block_a.mbIndex].mbType==IPCM)
          left_bit=1;
        else
          left_bit = getBit (mbData[block_a.mbIndex].codedBlockPatterns[0].bits, bit + bit_pos_a);
        }


      ctx = 2 * upper_bit + left_bit;
      // encode symbol =====
      codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);
      }
    }
  else if( (decoder->coding.isSeperateColourPlane != 0) ) {
    if (type != LUMA_8x8) {
      // get bits from neighbouring blocks ---
      if (block_b.available) {
        if(mbData[block_b.mbIndex].mbType==IPCM)
          upper_bit = 1;
        else
          upper_bit = getBit(mbData[block_b.mbIndex].codedBlockPatterns[0].bits,bit+bit_pos_b);
        }


      if (block_a.available) {
        if(mbData[block_a.mbIndex].mbType==IPCM)
          left_bit = 1;
        else
          left_bit = getBit(mbData[block_a.mbIndex].codedBlockPatterns[0].bits,bit+bit_pos_a);
        }


      ctx = 2 * upper_bit + left_bit;
      //===== encode symbol =====
      codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);
      }
    }

  else {
    if (block_b.available) {
      if(mbData[block_b.mbIndex].mbType==IPCM)
        upper_bit=1;
      else if ((type==LUMA_8x8 || type==CB_8x8 || type==CR_8x8) &&
               !mbData[block_b.mbIndex].lumaTransformSize8x8flag)
        upper_bit = 0;
      else {
        if (type == LUMA_8x8)
          upper_bit = getBit (mbData[block_b.mbIndex].codedBlockPatterns[0].bits8x8, bit + bit_pos_b);
        else if (type == CB_8x8)
          upper_bit = getBit (mbData[block_b.mbIndex].codedBlockPatterns[1].bits8x8, bit + bit_pos_b);
        else if (type == CR_8x8)
          upper_bit = getBit (mbData[block_b.mbIndex].codedBlockPatterns[2].bits8x8, bit + bit_pos_b);
        else if ((type == CB_4x4)||(type == CB_4x8) || (type == CB_8x4) || (type == CB_16AC) || (type == CB_16DC))
          upper_bit = getBit (mbData[block_b.mbIndex].codedBlockPatterns[1].bits,bit+bit_pos_b);
        else if ((type == CR_4x4)||(type == CR_4x8) || (type == CR_8x4) || (type == CR_16AC)||(type == CR_16DC))
          upper_bit = getBit (mbData[block_b.mbIndex].codedBlockPatterns[2].bits,bit+bit_pos_b);
        else
          upper_bit = getBit (mbData[block_b.mbIndex].codedBlockPatterns[0].bits,bit+bit_pos_b);
        }
      }


    if (block_a.available) {
      if(mbData[block_a.mbIndex].mbType == IPCM)
        left_bit=1;
      else if ((type == LUMA_8x8 || type == CB_8x8 || type == CR_8x8) &&
               !mbData[block_a.mbIndex].lumaTransformSize8x8flag)
        left_bit = 0;
      else {
        if(type == LUMA_8x8)
          left_bit = getBit (mbData[block_a.mbIndex].codedBlockPatterns[0].bits8x8,bit+bit_pos_a);
        else if (type == CB_8x8)
          left_bit = getBit (mbData[block_a.mbIndex].codedBlockPatterns[1].bits8x8,bit+bit_pos_a);
        else if (type == CR_8x8)
          left_bit = getBit (mbData[block_a.mbIndex].codedBlockPatterns[2].bits8x8,bit+bit_pos_a);
        else if ((type == CB_4x4) || (type == CB_4x8) ||
                 (type == CB_8x4) || (type == CB_16AC) || (type == CB_16DC))
          left_bit = getBit (mbData[block_a.mbIndex].codedBlockPatterns[1].bits,bit+bit_pos_a);
        else if ((type == CR_4x4) || (type==CR_4x8) ||
                 (type == CR_8x4) || (type == CR_16AC) || (type == CR_16DC))
          left_bit = getBit (mbData[block_a.mbIndex].codedBlockPatterns[2].bits,bit+bit_pos_a);
        else
          left_bit = getBit (mbData[block_a.mbIndex].codedBlockPatterns[0].bits,bit+bit_pos_a);
        }
      }

    ctx = 2 * upper_bit + left_bit;
    // encode symbol
    codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);
    }

  // set bits for current block ---
  bit = (y_dc ? 0 : y_ac ? 1 + j + (i >> 2) : u_dc ? 17 : v_dc ? 18 : u_ac ? 19 + j + (i >> 2) : 35 + j + (i >> 2));
  if (codedBlockPatternBit) {
    sCodedBlockPattern * codedBlockPatterns = mb->codedBlockPatterns;
    if (type == LUMA_8x8) {
      codedBlockPatterns[0].bits |= ((int64_t) 0x33 << bit);
      if (picture->chromaFormatIdc == YUV444)
        codedBlockPatterns[0].bits8x8 |= ((int64_t) 0x33 << bit);
      }
    else if (type == CB_8x8) {
      codedBlockPatterns[1].bits8x8 |= ((int64_t) 0x33 << bit);
      codedBlockPatterns[1].bits |= ((int64_t) 0x33 << bit);
      }
    else if (type == CR_8x8) {
      codedBlockPatterns[2].bits8x8 |= ((int64_t) 0x33 << bit);
      codedBlockPatterns[2].bits |= ((int64_t) 0x33 << bit);
      }
    else if (type == LUMA_8x4)
      codedBlockPatterns[0].bits |= ((int64_t) 0x03 << bit);
    else if (type == CB_8x4)
      codedBlockPatterns[1].bits |= ((int64_t) 0x03 << bit);
    else if (type == CR_8x4)
      codedBlockPatterns[2].bits |= ((int64_t) 0x03 << bit);
    else if (type == LUMA_4x8)
      codedBlockPatterns[0].bits |= ((int64_t) 0x11<< bit);
    else if (type == CB_4x8)
      codedBlockPatterns[1].bits |= ((int64_t)0x11<< bit);
    else if (type == CR_4x8)
      codedBlockPatterns[2].bits |= ((int64_t)0x11<< bit);
    else if ((type == CB_4x4) || (type == CB_16AC) || (type == CB_16DC))
      codedBlockPatterns[1].bits |= i64power2 (bit);
    else if ((type == CR_4x4) || (type == CR_16AC) || (type == CR_16DC))
      codedBlockPatterns[2].bits |= i64power2 (bit);
    else
      codedBlockPatterns[0].bits |= i64power2 (bit);
    }

  return codedBlockPatternBit;
  }
//}}}
//{{{
static int setCodedBlockPatternBit (sMacroBlock* neighbor_mb) {

  if (neighbor_mb->mbType == IPCM)
    return 1;
  else
    return (int)(neighbor_mb->codedBlockPatterns[0].bits & 0x01);
  }
//}}}
//{{{
static int setCodedBlockPatternBitAC (sMacroBlock* neighbor_mb, sPixelPos* block) {

  if (neighbor_mb->mbType == IPCM)
    return 1;
  else {
    int bit_pos = 1 + (block->y << 2) + block->x;
    return getBit (neighbor_mb->codedBlockPatterns[0].bits, bit_pos);
    }
  }
//}}}
//{{{
static int readStoreCbpBlockBitNormal (sMacroBlock* mb, sCabacDecodeEnv* cabacDecodeEnv, int type) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  sMacroBlock* mbData = slice->mbData;

  sTextureContexts* textureInfoContexts = slice->textureInfoContexts;

  int codedBlockPatternBit = 1;  // always one for 8x8 mode
  if (type == LUMA_16DC) {
    //{{{  luma_16dc
    int upper_bit = 1;
    int left_bit = 1;
    int ctx;

    sPixelPos block_a, block_b;
    get4x4NeighbourBase (mb, -1,  0, decoder->mbSize[eLuma], &block_a);
    get4x4NeighbourBase (mb,  0, -1, decoder->mbSize[eLuma], &block_b);

    //--- get bits from neighboring blocks ---
    if (block_b.available)
      upper_bit = setCodedBlockPatternBit (&mbData[block_b.mbIndex]);

    if (block_a.available)
      left_bit = setCodedBlockPatternBit (&mbData[block_a.mbIndex]);

    ctx = 2 * upper_bit + left_bit;

    // encode symbol =====
    codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

    // set bits for current block ---
    if (codedBlockPatternBit)
      mb->codedBlockPatterns[0].bits |= 1;
    }
    //}}}
  else if (type == LUMA_16AC) {
    //{{{  luma_16ac
    int j = mb->subblockY;
    int i = mb->subblockX;
    int bit = 1;
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit = default_bit;
    int left_bit = default_bit;
    int ctx;

    sPixelPos block_a, block_b;
    get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
    get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);

    // get bits from neighboring blocks ---
    if (block_b.available)
      upper_bit = setCodedBlockPatternBitAC (&mbData[block_b.mbIndex], &block_b);
    if (block_a.available)
      left_bit = setCodedBlockPatternBitAC (&mbData[block_a.mbIndex], &block_a);

    ctx = 2 * upper_bit + left_bit;

    // encode symbol =====
    codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

    if (codedBlockPatternBit) {
      // set bits for current block ---
      bit = 1 + j + (i >> 2);
      mb->codedBlockPatterns[0].bits |= i64power2(bit);
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
    int left_bit = default_bit;
    int ctx;

    sPixelPos block_a, block_b;

    get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
    get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);

    // get bits from neighboring blocks ---
    if (block_b.available)
      upper_bit = setCodedBlockPatternBitAC (&mbData[block_b.mbIndex], &block_b);

    if (block_a.available)
      left_bit = setCodedBlockPatternBitAC (&mbData[block_a.mbIndex], &block_a);

    ctx = 2 * upper_bit + left_bit;
    // encode symbol =====
    codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

    if (codedBlockPatternBit) {
      // set bits for current block ---
      bit = 1 + j + (i >> 2);
      mb->codedBlockPatterns[0].bits |= ((int64_t) 0x03 << bit   );
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

    get4x4NeighbourBase(mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
    get4x4NeighbourBase(mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);

    // get bits from neighboring blocks ---
    if (block_b.available)
      upper_bit = setCodedBlockPatternBitAC (&mbData[block_b.mbIndex], &block_b);

    if (block_a.available)
      left_bit = setCodedBlockPatternBitAC (&mbData[block_a.mbIndex], &block_a);

    ctx = 2 * upper_bit + left_bit;
    // encode symbol =====
    codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

    if (codedBlockPatternBit) {
      // set bits for current block ---
      bit = 1 + j + (i >> 2);
      mb->codedBlockPatterns[0].bits   |= ((int64_t) 0x11 << bit   );
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

    sPixelPos block_a, block_b;
    get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eLuma], &block_a);
    get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eLuma], &block_b);

    // get bits from neighboring blocks ---
    if (block_b.available)
      upper_bit = setCodedBlockPatternBitAC (&mbData[block_b.mbIndex], &block_b);

    if (block_a.available)
      left_bit = setCodedBlockPatternBitAC (&mbData[block_a.mbIndex], &block_a);

    int ctx = 2 * upper_bit + left_bit;
    // encode symbol =====
    codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

    if (codedBlockPatternBit) {
      // set bits for current block ---
      bit = 1 + j + (i >> 2);
      mb->codedBlockPatterns[0].bits   |= i64power2(bit);
      }
    }
    //}}}
  else if (type == LUMA_8x8) {
    //{{{  luma8x8
    int j = mb->subblockY;
    int i = mb->subblockX;

    // set bits for current block ---
    int bit = 1 + j + (i >> 2);
    mb->codedBlockPatterns[0].bits |= ((int64_t) 0x33 << bit   );
    }
    //}}}
  else if (type == CHROMA_DC || type == CHROMA_DC_2x4 || type == CHROMA_DC_4x4) {
    //{{{  chromaDC  2x4 4x4
    int u_dc = (!mb->isVblock);
    int j = 0;
    int i = 0;
    int bit = (u_dc ? 17 : 18);
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit = default_bit;
    int left_bit = default_bit;

    sPixelPos block_a, block_b;
    get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eChroma], &block_a);
    get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eChroma], &block_b);

    //--- get bits from neighboring blocks ---
    if (block_b.available) {
      if (mbData[block_b.mbIndex].mbType==IPCM)
        upper_bit = 1;
      else
        upper_bit = getBit(mbData[block_b.mbIndex].codedBlockPatterns[0].bits, bit);
      }

    if (block_a.available) {
      if (mbData[block_a.mbIndex].mbType==IPCM)
        left_bit = 1;
      else
        left_bit = getBit(mbData[block_a.mbIndex].codedBlockPatterns[0].bits, bit);
      }

    int ctx = 2 * upper_bit + left_bit;
    // encode symbol =====
    codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);

    if (codedBlockPatternBit) {
      // set bits for current block ---
      bit = (u_dc ? 17 : 18);
      mb->codedBlockPatterns[0].bits   |= i64power2(bit);
      }
    }
    //}}}
  else {
    //{{{  others
    int u_ac = (!mb->isVblock);
    int j = mb->subblockY;
    int i = mb->subblockX;
    int bit = (u_ac ? 19 : 35);
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit = default_bit;
    int left_bit = default_bit;

    sPixelPos block_a, block_b;
    get4x4NeighbourBase (mb, i - 1, j    , decoder->mbSize[eChroma], &block_a);
    get4x4NeighbourBase (mb, i    , j - 1, decoder->mbSize[eChroma], &block_b);

    // get bits from neighboring blocks ---
    if (block_b.available) {
      if(mbData[block_b.mbIndex].mbType==IPCM)
        upper_bit = 1;
      else {
        int bit_pos_b = 4*block_b.y + block_b.x;
        upper_bit = getBit (mbData[block_b.mbIndex].codedBlockPatterns[0].bits, bit + bit_pos_b);
        }
      }

    if (block_a.available) {
      if (mbData[block_a.mbIndex].mbType == IPCM)
        left_bit = 1;
      else {
        int bit_pos_a = 4*block_a.y + block_a.x;
        left_bit = getBit(mbData[block_a.mbIndex].codedBlockPatterns[0].bits,bit + bit_pos_a);
        }
      }

    int ctx = 2 * upper_bit + left_bit;

    // encode symbol
    codedBlockPatternBit = binaryArithmeticDecodeSymbol (cabacDecodeEnv, textureInfoContexts->bcbpContexts[type2ctx_bcbp[type]] + ctx);
    if (codedBlockPatternBit) {
      // set bits for current block ---
      bit = (u_ac ? 19 + j + (i >> 2) : 35 + j + (i >> 2));
      mb->codedBlockPatterns[0].bits   |= i64power2(bit);
      }
    }
    //}}}

  return codedBlockPatternBit;
  }
//}}}
//{{{
void setReadStoreCodedBlockPattern (sMacroBlock** mb, int chromaFormatIdc) {

  if (chromaFormatIdc == YUV444)
    (*mb)->readStoreCBPblockBit = readStoreCbpBlockBit444;
  else
    (*mb)->readStoreCBPblockBit = readStoreCbpBlockBitNormal;
  }
//}}}

//{{{
static int read_significance_map (sMacroBlock* mb, sCabacDecodeEnv*  cabacDecodeEnv, int type, int coeff[]) {

  sSlice* slice = mb->slice;
  int fld = ( slice->picStructure!=eFrame || mb->mbField );
  const uint8_t* pos2ctx_Map = (fld) ? pos2ctx_map_int[type] : pos2ctx_map[type];
  const uint8_t* pos2ctx_Last = pos2ctx_last[type];

  sBiContext*  map_ctx  = slice->textureInfoContexts->mapContexts [fld][type2ctx_map [type]];
  sBiContext*  last_ctx = slice->textureInfoContexts->lastContexts[fld][type2ctx_last[type]];

  int i;
  int coefCount = 0;
  int i0 = 0;
  int i1 = maxpos[type];


  if (!c1isdc[type]) {
    ++i0;
    ++i1;
    }

  for (i = i0; i < i1; ++i){
    // if last coeff is reached, it has to be significant
    //--- read significance symbol ---
    if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, map_ctx + pos2ctx_Map[i])) {
      *(coeff++) = 1;
      ++coefCount;
      //--- read last coefficient symbol ---
      if (binaryArithmeticDecodeSymbol (cabacDecodeEnv, last_ctx + pos2ctx_Last[i])) {
        memset(coeff, 0, (i1 - i) * sizeof(int));
        return coefCount;
        }
      }
    else
      *(coeff++) = 0;
    }

  //--- last coefficient must be significant if no last symbol was received ---
  if (i < i1 + 1) {
    *coeff = 1;
    ++coefCount;
    }

  return coefCount;
  }
//}}}
//{{{
static void read_significant_coefficients (sCabacDecodeEnv* cabacDecodeEnv, sTextureContexts* textureInfoContexts,
                                           int type, int* coeff) {

  sBiContext* oneContexts = textureInfoContexts->oneContexts[type2ctx_one[type]];
  sBiContext* absContexts = textureInfoContexts->absContexts[type2ctx_abs[type]];

  const short max_type = max_c2[type];
  int i = maxpos[type];
  int *cof = coeff + i;
  int c1 = 1;
  int c2 = 0;

  for (; i >= 0; i--) {
    if (*cof != 0) {
      *cof += binaryArithmeticDecodeSymbol (cabacDecodeEnv, oneContexts + c1);

      if (*cof == 2) {
        *cof += unary_exp_golomb_level_decode (cabacDecodeEnv, absContexts + c2);
        c2 = imin (++c2, max_type);
        c1 = 0;
        }
      else if (c1)
        c1 = imin (++c1, 4);

      if (binaryArithmeticDecodeSymbolEqProb (cabacDecodeEnv))
        *cof = - *cof;
      }
    cof--;
    }
  }
//}}}
//{{{
void readRunLevel_CABAC (sMacroBlock* mb, sSyntaxElement* se, sCabacDecodeEnv* cabacDecodeEnv) {

  sSlice* slice = mb->slice;
  int* coefCount = &slice->coefCount;
  int* coeff = slice->coeff;

  // read coefficients for whole block
  if (*coefCount < 0) {
    // decode CBP-BIT
    if ((*coefCount = mb->readStoreCBPblockBit (mb, cabacDecodeEnv, se->context) ) != 0) {
      // decode significance map
      *coefCount = read_significance_map (mb, cabacDecodeEnv, se->context, coeff);

      // decode significant coefficients
      read_significant_coefficients (cabacDecodeEnv, slice->textureInfoContexts, se->context, coeff);
      }
    }

  // set run and level
  if (*coefCount) {
    // set run and level (coefficient)
    for (se->value2 = 0; coeff[slice->pos] == 0; ++slice->pos, ++se->value2);
    se->value1 = coeff[slice->pos++];
    }
  else
    // set run and level (EOB) ---
    se->value1 = se->value2 = 0;

  // decrement coefficient counter and re-set position
  if ((*coefCount)-- == 0)
    slice->pos = 0;
  }
//}}}
//{{{
int readSyntaxElementCABAC (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* this_dataPart) {

  sCabacDecodeEnv* cabacDecodeEnv = &this_dataPart->cabacDecodeEnv;
  int curr_len = arithmeticDecodeBitsRead (cabacDecodeEnv);

  // perform the actual decoding by calling the appropriate method
  se->reading (mb, se, cabacDecodeEnv);

  //read again and minus curr_len = arithmeticDecodeBitsRead(cabacDecodeEnv); from above
  se->len = arithmeticDecodeBitsRead (cabacDecodeEnv) - curr_len;

  return se->len;
  }
//}}}

//{{{
void readIPCMcabac (sSlice* slice, sDataPartition* dataPartition) {

  sDecoder* decoder = slice->decoder;
  sPicture* picture = slice->picture;

  sBitStream* s = dataPartition->stream;
  sCabacDecodeEnv* cabacDecodeEnv = &dataPartition->cabacDecodeEnv;
  uint8_t* buf = s->bitStreamBuffer;
  int bitStreamLengthInBits = (dataPartition->stream->bitStreamLen << 3) + 7;

  int val = 0;
  int bitsRead = 0;

  while (cabacDecodeEnv->bitsLeft >= 8) {
    cabacDecodeEnv->value >>= 8;
    cabacDecodeEnv->bitsLeft -= 8;
    (*cabacDecodeEnv->codeStreamLen)--;
    }

  int bitOffset = (*cabacDecodeEnv->codeStreamLen) << 3;

  // read luma values
  int bitDepth = decoder->bitDepthLuma;
  for (int i = 0; i < MB_BLOCK_SIZE; ++i) {
    for (int j = 0; j < MB_BLOCK_SIZE; ++j) {
      bitsRead += getBits (buf, bitOffset, &val, bitStreamLengthInBits, bitDepth);
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
          bitsRead += getBits (buf, bitOffset, &val, bitStreamLengthInBits, bitDepth);
          slice->cof[uv][i][j] = val;
          bitOffset += bitDepth;
          }
         }
      }
    }

  (*cabacDecodeEnv->codeStreamLen) += bitsRead >> 3;
  if (bitsRead & 7)
    ++(*cabacDecodeEnv->codeStreamLen);
  }
//}}}
