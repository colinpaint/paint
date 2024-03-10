//{{{  includes
#include "global.h"
#include "memory.h"

#include "cabac.h"
#include "elements.h"
#include "image.h"
#include "biariDecode.h"
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
static const byte  pos2ctx_map8x8 [] = { 0,  1,  2,  3,  4,  5,  5,  4,  4,  3,  3,  4,  4,  4,  5,  5,
                                         4,  4,  4,  4,  3,  3,  6,  7,  7,  7,  8,  9, 10,  9,  8,  7,
                                         7,  6, 11, 12, 13, 11,  6,  7,  8,  9, 14, 10,  9,  8,  6, 11,
                                        12, 13, 11,  6,  9, 14, 10,  9, 11, 12, 13, 11 ,14, 10, 12, 14}; // 15 CTX

static const byte  pos2ctx_map8x4 [] = { 0,  1,  2,  3,  4,  5,  7,  8,  9, 10, 11,  9,  8,  6,  7,  8,
                                         9, 10, 11,  9,  8,  6, 12,  8,  9, 10, 11,  9, 13, 13, 14, 14}; // 15 CTX

static const byte  pos2ctx_map4x4 [] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 14}; // 15 CTX
static const byte  pos2ctx_map2x4c[] = { 0,  0,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
static const byte  pos2ctx_map4x4c[] = { 0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX

static const byte* pos2ctx_map    [] = {pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8, pos2ctx_map8x4,
                                        pos2ctx_map8x4, pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map4x4,
                                        pos2ctx_map2x4c, pos2ctx_map4x4c,
                                        pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8,pos2ctx_map8x4,
                                        pos2ctx_map8x4, pos2ctx_map4x4,
                                        pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8,pos2ctx_map8x4,
                                        pos2ctx_map8x4,pos2ctx_map4x4};

//--- interlace scan ----
//taken from ABT
//{{{
static const byte  pos2ctx_map8x8i[] = { 0,  1,  1,  2,  2,  3,  3,  4,  5,  6,  7,  7,  7,  8,  4,  5,
                                         6,  9, 10, 10,  8, 11, 12, 11,  9,  9, 10, 10,  8, 11, 12, 11,
                                         9,  9, 10, 10,  8, 11, 12, 11,  9,  9, 10, 10,  8, 13, 13,  9,
                                         9, 10, 10,  8, 13, 13,  9,  9, 10, 10, 14, 14, 14, 14, 14, 14}; // 15 CTX
//}}}
//{{{
static const byte  pos2ctx_map8x4i[] = { 0,  1,  2,  3,  4,  5,  6,  3,  4,  5,  6,  3,  4,  7,  6,  8,
                                         9,  7,  6,  8,  9, 10, 11, 12, 12, 10, 11, 13, 13, 14, 14, 14}; // 15 CTX
//}}}
//{{{
static const byte  pos2ctx_map4x8i[] = { 0,  1,  1,  1,  2,  3,  3,  4,  4,  4,  5,  6,  2,  7,  7,  8,
                                         8,  8,  5,  6,  9, 10, 10, 11, 11, 11, 12, 13, 13, 14, 14, 14}; // 15 CTX
//}}}
//{{{
static const byte* pos2ctx_map_int[] = {pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
                                        pos2ctx_map4x8i,pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map4x4,
                                        pos2ctx_map2x4c, pos2ctx_map4x4c,
                                        pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
                                        pos2ctx_map8x4i,pos2ctx_map4x4,
                                        pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
                                        pos2ctx_map8x4i,pos2ctx_map4x4};
//}}}

//===== position -> ctx for LAST =====
//{{{
static const byte  pos2ctx_last8x8 [] = { 0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
                                          2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
                                          3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
                                          5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8}; //  9 CTX
//}}}
//{{{
static const byte  pos2ctx_last8x4 [] = { 0,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,
                                          3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8}; //  9 CTX
//}}}

static const byte  pos2ctx_last4x4 [] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15}; // 15 CTX
static const byte  pos2ctx_last2x4c[] = { 0,  0,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
static const byte  pos2ctx_last4x4c[] = { 0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX

static const byte* pos2ctx_last    [] = {pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8, pos2ctx_last8x4,
                                         pos2ctx_last8x4, pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last4x4,
                                         pos2ctx_last2x4c, pos2ctx_last4x4c,
                                         pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8,pos2ctx_last8x4,
                                         pos2ctx_last8x4, pos2ctx_last4x4,
                                         pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8,pos2ctx_last8x4,
                                         pos2ctx_last8x4, pos2ctx_last4x4};
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    decoding of unary binarization using one or 2 distinct
 *    models for the first and all remaining bins; no terminating
 *    "0" for max_symbol
** *********************************************************************
 */
static unsigned int unary_bin_max_decode (sDecodingEnv* decodingEnv,
                                  sBiContextType* ctx,
                                  int ctx_offset,
                                  unsigned int max_symbol)
{
  unsigned int symbol =  biari_decode_symbol (decodingEnv, ctx );

  if (symbol == 0 || (max_symbol == 0))
    return symbol;
  else
  {
    unsigned int l;
    ctx += ctx_offset;
    symbol = 0;
    do
    {
      l = biari_decode_symbol(decodingEnv, ctx);
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
/*!
** **********************************************************************
 * \brief
 *    decoding of unary binarization using one or 2 distinct
 *    models for the first and all remaining bins
** *********************************************************************
 */
static unsigned int unary_bin_decode (sDecodingEnv* decodingEnv,
                                     sBiContextType* ctx,
                                     int ctx_offset)
{
  unsigned int symbol = biari_decode_symbol(decodingEnv, ctx );

  if (symbol == 0)
    return 0;
  else
  {
    unsigned int l;
    ctx += ctx_offset;;
    symbol = 0;
    do
    {
      l = biari_decode_symbol(decodingEnv, ctx);
      ++symbol;
    }
    while( l != 0 );
    return symbol;
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Exp Golomb binarization and decoding of a symbol
 *    with prob. of 0.5
** **********************************************************************
 */
static unsigned int exp_golomb_decode_eq_prob (sDecodingEnv* decodingEnv,
                                              int k)
{
  unsigned int l;
  int symbol = 0;
  int binary_symbol = 0;

  do
  {
    l = biari_decode_symbol_eq_prob(decodingEnv);
    if (l == 1)
    {
      symbol += (1<<k);
      ++k;
    }
  }
  while (l!=0);

  while (k--)                             //next binary part
    if (biari_decode_symbol_eq_prob(decodingEnv)==1)
      binary_symbol |= (1<<k);

  return (unsigned int) (symbol + binary_symbol);
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Exp-Golomb decoding for LEVELS
** *********************************************************************
 */
static unsigned int unary_exp_golomb_level_decode (sDecodingEnv* decodingEnv,
                                                  sBiContextType* ctx)
{
  unsigned int symbol = biari_decode_symbol(decodingEnv, ctx );

  if (symbol==0)
    return 0;
  else
  {
    unsigned int l, k = 1;
    unsigned int exp_start = 13;

    symbol = 0;

    do
    {
      l=biari_decode_symbol(decodingEnv, ctx);
      ++symbol;
      ++k;
    }
    while((l != 0) && (k != exp_start));
    if (l!=0)
      symbol += exp_golomb_decode_eq_prob(decodingEnv,0)+1;
    return symbol;
  }
}

//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Exp-Golomb decoding for Motion Vectors
** *********************************************************************
 */
static unsigned int unary_exp_golomb_mv_decode (sDecodingEnv* decodingEnv,
                                               sBiContextType* ctx,
                                               unsigned int max_bin)
{
  unsigned int symbol = biari_decode_symbol(decodingEnv, ctx );

  if (symbol == 0)
    return 0;
  else
  {
    unsigned int exp_start = 8;
    unsigned int l,k = 1;
    unsigned int bin = 1;

    symbol=0;

    ++ctx;
    do
    {
      l=biari_decode_symbol(decodingEnv, ctx);
      if ((++bin)==2) ctx++;
      if (bin==max_bin)
        ++ctx;
      ++symbol;
      ++k;
    }
    while((l!=0) && (k!=exp_start));
    if (l!=0)
      symbol += exp_golomb_decode_eq_prob(decodingEnv,3) + 1;
    return symbol;
  }
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    finding end of a slice in case this is not the end of a frame
 *
 * Unsure whether the "correction" below actually solves an off-by-one
 * problem or whether it introduces one in some cases :-(  Anyway,
 * with this change the bit stream format works with CABAC again.
 * StW, 8.7.02
** **********************************************************************
 */
int cabac_startcode_follows (sSlice* slice, int eos_bit)
{
  unsigned int  bit;

  if( eos_bit )
  {
    const byte   *dpMap    = assignSE2dp[slice->datadpMode];
    sDatadp *dp = &(slice->dps[dpMap[SE_MBTYPE]]);
    sDecodingEnv* decodingEnv = &(dp->deCabac);

    bit = biari_decode_final (decodingEnv); //GB
  }
  else
  {
    bit = 0;
  }

  return (bit == 1 ? 1 : 0);
}
//}}}

//{{{
void checkNeighbourCabac (sMacroblock* mb)
{
  sDecoder* decoder = mb->decoder;
  sPixelPos up, left;
  int *mbSize = decoder->mbSize[IS_LUMA];

  decoder->getNeighbour (mb, -1,  0, mbSize, &left);
  decoder->getNeighbour (mb,  0, -1, mbSize, &up);

  if (up.available)
    mb->mbCabacUp = &mb->slice->mbData[up.mbIndex]; //&decoder->mbData[up.mbIndex];
  else
    mb->mbCabacUp = NULL;

  if (left.available)
    mb->mbCabacLeft = &mb->slice->mbData[left.mbIndex]; //&decoder->mbData[left.mbIndex];
  else
    mb->mbCabacLeft = NULL;
}
//}}}
//{{{
void cabacNewSlice (sSlice* slice)
{
  slice->lastDquant = 0;
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocation of contexts models for the motion info
 *    used for arithmetic decoding
 *
** **********************************************************************
 */
sMotionInfoContexts* create_contexts_MotionInfo()
{
  sMotionInfoContexts *deco_ctx;

  deco_ctx = (sMotionInfoContexts*) calloc(1, sizeof(sMotionInfoContexts) );
  if( deco_ctx == NULL )
    no_mem_exit("create_contexts_MotionInfo: deco_ctx");

  return deco_ctx;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Allocates of contexts models for the texture info
 *    used for arithmetic decoding
** **********************************************************************
 */
sTextureInfoContexts* create_contexts_TextureInfo()
{
  sTextureInfoContexts *deco_ctx;

  deco_ctx = (sTextureInfoContexts*) calloc(1, sizeof(sTextureInfoContexts) );
  if( deco_ctx == NULL )
    no_mem_exit("create_contexts_TextureInfo: deco_ctx");

  return deco_ctx;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Frees the memory of the contexts models
 *    used for arithmetic decoding of the motion info.
** **********************************************************************
 */
void delete_contexts_MotionInfo (sMotionInfoContexts* deco_ctx)
{
  if( deco_ctx == NULL )
    return;

  free( deco_ctx );
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Frees the memory of the contexts models
 *    used for arithmetic decoding of the texture info.
** **********************************************************************
 */
void delete_contexts_TextureInfo (sTextureInfoContexts* deco_ctx)
{
  if( deco_ctx == NULL )
    return;

  free( deco_ctx );
}
//}}}

//{{{
void readFieldModeInfo_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  sMotionInfoContexts *ctx  = slice->mot_ctx;
  int a = mb->mbAvailA ? slice->mbData[mb->mbIndexA].mbField : 0;
  int b = mb->mbAvailB ? slice->mbData[mb->mbIndexB].mbField : 0;
  int act_ctx = a + b;

  se->value1 = biari_decode_symbol (decodingEnv, &ctx->mb_aff_contexts[act_ctx]);
}
//}}}

//{{{
int check_next_mb_and_get_field_mode_CABAC_p_slice (sSlice* slice, sSyntaxElement* se, sDatadp* act_dp)
{
  sDecoder* decoder = slice->decoder;
  sBiContextType*          mb_type_ctx_copy[3];
  sBiContextType*          mb_aff_ctx_copy;
  sDecodingEnv*    decodingEnv_copy;
  sMotionInfoContexts *mot_ctx  = slice->mot_ctx;

  int length;
  sDecodingEnv*    decodingEnv = &(act_dp->deCabac);

  int skip   = 0;
  int field  = 0;
  int i;

  sMacroblock* mb;

  //get next MB
  ++slice->mbIndex;

  mb = &slice->mbData[slice->mbIndex];
  mb->decoder    = decoder;
  mb->slice  = slice;
  mb->sliceNum = slice->curSliceIndex;
  mb->mbField = slice->mbData[slice->mbIndex-1].mbField;
  mb->mbIndexX  = slice->mbIndex;
  mb->listOffset = ((slice->mbAffFrameFlag) && (mb->mbField))? (mb->mbIndexX&0x01) ? 4 : 2 : 0;

  CheckAvailabilityOfNeighborsMBAFF (mb);
  checkNeighbourCabac (mb);

  //create
  decodingEnv_copy = (sDecodingEnv*) calloc(1, sizeof(sDecodingEnv) );
  for (i=0;i<3;++i)
    mb_type_ctx_copy[i] = (sBiContextType*) calloc(NUM_MB_TYPE_CTX, sizeof(sBiContextType) );
  mb_aff_ctx_copy = (sBiContextType*) calloc(NUM_MB_AFF_CTX, sizeof(sBiContextType) );

  //copy
  memcpy(decodingEnv_copy,decodingEnv,sizeof(sDecodingEnv));
  length = *(decodingEnv_copy->Dcodestrm_len) = *(decodingEnv->Dcodestrm_len);
  for (i=0;i<3;++i)
    memcpy(mb_type_ctx_copy[i], mot_ctx->mb_type_contexts[i],NUM_MB_TYPE_CTX*sizeof(sBiContextType) );
  memcpy(mb_aff_ctx_copy, mot_ctx->mb_aff_contexts,NUM_MB_AFF_CTX*sizeof(sBiContextType) );

  //check_next_mb
#if TRACE
  strncpy(se->tracestring, "mb_skip_flag (of following bottom MB)", TRACESTRING_SIZE);
#endif
  slice->lastDquant = 0;
  read_skip_flag_CABAC_p_slice (mb, se, decodingEnv);

  skip = (se->value1==0);

  if (!skip)
  {
#if TRACE
    strncpy(se->tracestring, "mb_field_decoding_flag (of following bottom MB)", TRACESTRING_SIZE);
#endif
    readFieldModeInfo_CABAC (mb, se,decodingEnv);
    field = se->value1;
    slice->mbData[slice->mbIndex-1].mbField = field;
  }

  //reset
  slice->mbIndex--;

  memcpy(decodingEnv,decodingEnv_copy,sizeof(sDecodingEnv));
  *(decodingEnv->Dcodestrm_len) = length;
  for (i=0;i<3;++i)
    memcpy(mot_ctx->mb_type_contexts[i],mb_type_ctx_copy[i], NUM_MB_TYPE_CTX*sizeof(sBiContextType) );
  memcpy( mot_ctx->mb_aff_contexts,mb_aff_ctx_copy,NUM_MB_AFF_CTX*sizeof(sBiContextType) );

  checkNeighbourCabac (mb);

  //delete
  free(decodingEnv_copy);
  for (i=0;i<3;++i)
    free(mb_type_ctx_copy[i]);
  free(mb_aff_ctx_copy);

  return skip;
}
//}}}
//{{{
int check_next_mb_and_get_field_mode_CABAC_b_slice (sSlice* slice,
                                           sSyntaxElement *se,
                                           sDatadp  *act_dp)
{
  sDecoder* decoder = slice->decoder;
  sBiContextType*          mb_type_ctx_copy[3];
  sBiContextType*          mb_aff_ctx_copy;
  sDecodingEnv*    decodingEnv_copy;

  int length;
  sDecodingEnv*    decodingEnv = &(act_dp->deCabac);
  sMotionInfoContexts  *mot_ctx = slice->mot_ctx;

  int skip   = 0;
  int field  = 0;
  int i;

  sMacroblock* mb;

  //get next MB
  ++slice->mbIndex;

  mb = &slice->mbData[slice->mbIndex];
  mb->decoder    = decoder;
  mb->slice  = slice;
  mb->sliceNum = slice->curSliceIndex;
  mb->mbField = slice->mbData[slice->mbIndex-1].mbField;
  mb->mbIndexX  = slice->mbIndex;
  mb->listOffset = ((slice->mbAffFrameFlag)&&(mb->mbField))? (mb->mbIndexX & 0x01) ? 4 : 2 : 0;

  CheckAvailabilityOfNeighborsMBAFF (mb);
  checkNeighbourCabac (mb);

  //create
  decodingEnv_copy = (sDecodingEnv*) calloc(1, sizeof(sDecodingEnv) );
  for (i=0;i<3;++i)
    mb_type_ctx_copy[i] = (sBiContextType*) calloc(NUM_MB_TYPE_CTX, sizeof(sBiContextType) );
  mb_aff_ctx_copy = (sBiContextType*) calloc(NUM_MB_AFF_CTX, sizeof(sBiContextType) );

  //copy
  memcpy(decodingEnv_copy,decodingEnv,sizeof(sDecodingEnv));
  length = *(decodingEnv_copy->Dcodestrm_len) = *(decodingEnv->Dcodestrm_len);

  for (i=0;i<3;++i)
    memcpy(mb_type_ctx_copy[i], mot_ctx->mb_type_contexts[i],NUM_MB_TYPE_CTX*sizeof(sBiContextType) );

  memcpy(mb_aff_ctx_copy, mot_ctx->mb_aff_contexts,NUM_MB_AFF_CTX*sizeof(sBiContextType) );

  //check_next_mb
#if TRACE
  strncpy(se->tracestring, "mb_skip_flag (of following bottom MB)", TRACESTRING_SIZE);
#endif
  slice->lastDquant = 0;
  read_skip_flag_CABAC_b_slice (mb, se, decodingEnv);

  skip = (se->value1==0 && se->value2==0);
  if (!skip)
  {
#if TRACE
    strncpy(se->tracestring, "mb_field_decoding_flag (of following bottom MB)", TRACESTRING_SIZE);
#endif
    readFieldModeInfo_CABAC (mb, se,decodingEnv);
    field = se->value1;
    slice->mbData[slice->mbIndex-1].mbField = field;
  }

  //reset
  slice->mbIndex--;

  memcpy(decodingEnv,decodingEnv_copy,sizeof(sDecodingEnv));
  *(decodingEnv->Dcodestrm_len) = length;

  for (i=0;i<3;++i)
    memcpy(mot_ctx->mb_type_contexts[i],mb_type_ctx_copy[i], NUM_MB_TYPE_CTX * sizeof(sBiContextType) );

  memcpy( mot_ctx->mb_aff_contexts, mb_aff_ctx_copy, NUM_MB_AFF_CTX * sizeof(sBiContextType) );

  checkNeighbourCabac (mb);

  //delete
  free(decodingEnv_copy);
  for (i=0;i<3;++i)
    free(mb_type_ctx_copy[i]);
  free(mb_aff_ctx_copy);

  return skip;
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the motion
 *    vector data of a B-frame MB.
** **********************************************************************
 */
void read_MVD_CABAC (sMacroblock* mb,
                    sSyntaxElement *se,
                    sDecodingEnv* decodingEnv)
{
  int *mbSize = mb->decoder->mbSize[IS_LUMA];
  sSlice* slice = mb->slice;
  sMotionInfoContexts *ctx = slice->mot_ctx;
  int i = mb->subblockX;
  int j = mb->subblockY;
  int a = 0;
  //int act_ctx;
  int act_sym;
  int list_idx = se->value2 & 0x01;
  int k = (se->value2 >> 1); // MVD component

  sPixelPos block_a, block_b;

  get4x4NeighbourBase(mb, i - 1, j    , mbSize, &block_a);
  get4x4NeighbourBase(mb, i    , j - 1, mbSize, &block_b);
  if (block_a.available)
  {
    a = iabs(slice->mbData[block_a.mbIndex].mvd[list_idx][block_a.y][block_a.x][k]);
  }
  if (block_b.available)
  {
    a += iabs(slice->mbData[block_b.mbIndex].mvd[list_idx][block_b.y][block_b.x][k]);
  }

  //a += b;

  if (a < 3)
    a = 5 * k;
  else if (a > 32)
    a = 5 * k + 3;
  else
    a = 5 * k + 2;

  se->context = a;

  act_sym = biari_decode_symbol(decodingEnv, ctx->mv_res_contexts[0] + a );

  if (act_sym != 0)
  {
    a = 5 * k;
    act_sym = unary_exp_golomb_mv_decode(decodingEnv, ctx->mv_res_contexts[1] + a, 3) + 1;

    if(biari_decode_symbol_eq_prob(decodingEnv))
      act_sym = -act_sym;
  }
  se->value1 = act_sym;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the motion
 *    vector data of a B-frame MB.
** **********************************************************************
 */
void read_mvd_CABAC_mbaff (sMacroblock* mb,
                    sSyntaxElement *se,
                    sDecodingEnv* decodingEnv)
{
  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  sMotionInfoContexts *ctx = slice->mot_ctx;
  int i = mb->subblockX;
  int j = mb->subblockY;
  int a = 0, b = 0;
  int act_ctx;
  int act_sym;
  int list_idx = se->value2 & 0x01;
  int k = (se->value2 >> 1); // MVD component

  sPixelPos block_a, block_b;

  get4x4NeighbourBase(mb, i - 1, j    , decoder->mbSize[IS_LUMA], &block_a);
  if (block_a.available)
  {
    a = iabs(slice->mbData[block_a.mbIndex].mvd[list_idx][block_a.y][block_a.x][k]);
    if (slice->mbAffFrameFlag && (k==1))
    {
      if ((mb->mbField==0) && (slice->mbData[block_a.mbIndex].mbField==1))
        a *= 2;
      else if ((mb->mbField==1) && (slice->mbData[block_a.mbIndex].mbField==0))
        a /= 2;
    }
  }

  get4x4NeighbourBase(mb, i    , j - 1, decoder->mbSize[IS_LUMA], &block_b);
  if (block_b.available)
  {
    b = iabs(slice->mbData[block_b.mbIndex].mvd[list_idx][block_b.y][block_b.x][k]);
    if (slice->mbAffFrameFlag && (k==1))
    {
      if ((mb->mbField==0) && (slice->mbData[block_b.mbIndex].mbField==1))
        b *= 2;
      else if ((mb->mbField==1) && (slice->mbData[block_b.mbIndex].mbField==0))
        b /= 2;
    }
  }
  a += b;

  if (a < 3)
    act_ctx = 5 * k;
  else if (a > 32)
    act_ctx = 5 * k + 3;
  else
    act_ctx = 5 * k + 2;

  se->context = act_ctx;

  act_sym = biari_decode_symbol(decodingEnv,&ctx->mv_res_contexts[0][act_ctx] );

  if (act_sym != 0)
  {
    act_ctx = 5 * k;
    act_sym = unary_exp_golomb_mv_decode(decodingEnv, ctx->mv_res_contexts[1] + act_ctx, 3) + 1;

    if(biari_decode_symbol_eq_prob(decodingEnv))
      act_sym = -act_sym;
  }
  se->value1 = act_sym;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the 8x8 block type.
** **********************************************************************
 */
void readB8_typeInfo_CABAC_p_slice (sMacroblock* mb,
                                    sSyntaxElement *se,
                                    sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  int act_sym = 0;

  sMotionInfoContexts *ctx = slice->mot_ctx;
  sBiContextType *b8_type_contexts = &ctx->b8_type_contexts[0][1];

  if (biari_decode_symbol (decodingEnv, b8_type_contexts++))
    act_sym = 0;
  else
  {
    if (biari_decode_symbol (decodingEnv, ++b8_type_contexts))
    {
      act_sym = (biari_decode_symbol (decodingEnv, ++b8_type_contexts))? 2: 3;
    }
    else
    {
      act_sym = 1;
    }
  }

  se->value1 = act_sym;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the 8x8 block type.
** **********************************************************************
 */
void readB8_typeInfo_CABAC_b_slice (sMacroblock* mb,
                                    sSyntaxElement *se,
                                    sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  int act_sym = 0;

  sMotionInfoContexts *ctx = slice->mot_ctx;
  sBiContextType *b8_type_contexts = ctx->b8_type_contexts[1];

  if (biari_decode_symbol (decodingEnv, b8_type_contexts++))
  {
    if (biari_decode_symbol (decodingEnv, b8_type_contexts++))
    {
      if (biari_decode_symbol (decodingEnv, b8_type_contexts++))
      {
        if (biari_decode_symbol (decodingEnv, b8_type_contexts))
        {
          act_sym = 10;
          if (biari_decode_symbol (decodingEnv, b8_type_contexts))
            act_sym++;
        }
        else
        {
          act_sym = 6;
          if (biari_decode_symbol (decodingEnv, b8_type_contexts))
            act_sym += 2;
          if (biari_decode_symbol (decodingEnv, b8_type_contexts))
            act_sym++;
        }
      }
      else
      {
        act_sym = 2;
        if (biari_decode_symbol (decodingEnv, b8_type_contexts))
          act_sym += 2;
        if (biari_decode_symbol (decodingEnv, b8_type_contexts))
          act_sym ++;
      }
    }
    else
    {
      act_sym = (biari_decode_symbol (decodingEnv, ++b8_type_contexts)) ? 1: 0;
    }
    ++act_sym;
  }
  else
  {
    act_sym = 0;
  }

  se->value1 = act_sym;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the macroblock
 *    type info of a given MB.
** **********************************************************************
 */
void read_skip_flag_CABAC_p_slice (sMacroblock* mb,
                                  sSyntaxElement *se,
                                  sDecodingEnv* decodingEnv)
{
  int a = (mb->mbCabacLeft != NULL) ? (mb->mbCabacLeft->skipFlag == 0) : 0;
  int b = (mb->mbCabacUp   != NULL) ? (mb->mbCabacUp  ->skipFlag == 0) : 0;
  sBiContextType *mb_type_contexts = &mb->slice->mot_ctx->mb_type_contexts[1][a + b];

  se->value1 = (biari_decode_symbol(decodingEnv, mb_type_contexts) != 1);

  if (!se->value1)
    mb->slice->lastDquant = 0;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the macroblock
 *    type info of a given MB.
** **********************************************************************
 */
void read_skip_flag_CABAC_b_slice (sMacroblock* mb,
                                  sSyntaxElement *se,
                                  sDecodingEnv* decodingEnv)
{
  int a = (mb->mbCabacLeft != NULL) ? (mb->mbCabacLeft->skipFlag == 0) : 0;
  int b = (mb->mbCabacUp   != NULL) ? (mb->mbCabacUp  ->skipFlag == 0) : 0;
  sBiContextType *mb_type_contexts = &mb->slice->mot_ctx->mb_type_contexts[2][7 + a + b];

  se->value1 = se->value2 = (biari_decode_symbol (decodingEnv, mb_type_contexts) != 1);
  if (!se->value1)
    mb->slice->lastDquant = 0;
}
//}}}

//{{{
/*!
***************************************************************************
* \brief
*    This function is used to arithmetically decode the macroblock
*    intra_pred_size flag info of a given MB.
***************************************************************************
*/

void readMB_transform_size_flag_CABAC (sMacroblock* mb,
                                      sSyntaxElement *se,
                                      sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  sTextureInfoContexts*ctx = slice->tex_ctx;

  int b = (mb->mbCabacUp   == NULL) ? 0 : mb->mbCabacUp->lumaTransformSize8x8flag;
  int a = (mb->mbCabacLeft == NULL) ? 0 : mb->mbCabacLeft->lumaTransformSize8x8flag;

  int act_sym = biari_decode_symbol(decodingEnv, ctx->transform_size_contexts + a + b );

  se->value1 = act_sym;

}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the macroblock
 *    type info of a given MB.
** **********************************************************************
 */
void readMB_typeInfo_CABAC_i_slice (sMacroblock* mb,
                           sSyntaxElement *se,
                           sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  sMotionInfoContexts *ctx = slice->mot_ctx;

  int a = 0, b = 0;
  int act_ctx;
  int act_sym;
  int mode_sym;
  int curr_mb_type = 0;

  if(slice->sliceType == I_SLICE)  // INTRA-frame
  {
    if (mb->mbCabacUp != NULL)
      b = (((mb->mbCabacUp)->mbType != I4MB && mb->mbCabacUp->mbType != I8MB) ? 1 : 0 );

    if (mb->mbCabacLeft != NULL)
      a = (((mb->mbCabacLeft)->mbType != I4MB && mb->mbCabacLeft->mbType != I8MB) ? 1 : 0 );

    act_ctx = a + b;
    act_sym = biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx);
    se->context = act_ctx; // store context

    if (act_sym==0) // 4x4 Intra
    {
      curr_mb_type = act_sym;
    }
    else // 16x16 Intra
    {
      mode_sym = biari_decode_final(decodingEnv);
      if(mode_sym == 1)
      {
        curr_mb_type = 25;
      }
      else
      {
        act_sym = 1;
        act_ctx = 4;
        mode_sym =  biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx ); // decoding of AC/no AC
        act_sym += mode_sym*12;
        act_ctx = 5;
        // decoding of cbp: 0,1,2
        mode_sym =  biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx );
        if (mode_sym!=0)
        {
          act_ctx=6;
          mode_sym = biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx );
          act_sym+=4;
          if (mode_sym!=0)
            act_sym+=4;
        }
        // decoding of I pred-mode: 0,1,2,3
        act_ctx = 7;
        mode_sym =  biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx );
        act_sym += mode_sym*2;
        act_ctx = 8;
        mode_sym =  biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx );
        act_sym += mode_sym;
        curr_mb_type = act_sym;
      }
    }
  }
  else if(slice->sliceType == SI_SLICE)  // SI-frame
  {
    // special ctx's for SI4MB
    if (mb->mbCabacUp != NULL)
      b = (( (mb->mbCabacUp)->mbType != SI4MB) ? 1 : 0 );

    if (mb->mbCabacLeft != NULL)
      a = (( (mb->mbCabacLeft)->mbType != SI4MB) ? 1 : 0 );

    act_ctx = a + b;
    act_sym = biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[1] + act_ctx);
    se->context = act_ctx; // store context

    if (act_sym==0) //  SI 4x4 Intra
    {
      curr_mb_type = 0;
    }
    else // analog INTRA_IMG
    {
      if (mb->mbCabacUp != NULL)
        b = (( (mb->mbCabacUp)->mbType != I4MB) ? 1 : 0 );

      if (mb->mbCabacLeft != NULL)
        a = (( (mb->mbCabacLeft)->mbType != I4MB) ? 1 : 0 );

      act_ctx = a + b;
      act_sym = biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx);
      se->context = act_ctx; // store context

      if (act_sym==0) // 4x4 Intra
      {
        curr_mb_type = 1;
      }
      else // 16x16 Intra
      {
        mode_sym = biari_decode_final(decodingEnv);
        if( mode_sym==1 )
        {
          curr_mb_type = 26;
        }
        else
        {
          act_sym = 2;
          act_ctx = 4;
          mode_sym =  biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx ); // decoding of AC/no AC
          act_sym += mode_sym*12;
          act_ctx = 5;
          // decoding of cbp: 0,1,2
          mode_sym =  biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx );
          if (mode_sym!=0)
          {
            act_ctx=6;
            mode_sym = biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx );
            act_sym+=4;
            if (mode_sym!=0)
              act_sym+=4;
          }
          // decoding of I pred-mode: 0,1,2,3
          act_ctx = 7;
          mode_sym =  biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx );
          act_sym += mode_sym*2;
          act_ctx = 8;
          mode_sym =  biari_decode_symbol(decodingEnv, ctx->mb_type_contexts[0] + act_ctx );
          act_sym += mode_sym;
          curr_mb_type = act_sym;
        }
      }
    }
  }

  se->value1 = curr_mb_type;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the macroblock
 *    type info of a given MB.
** **********************************************************************
 */
void readMB_typeInfo_CABAC_p_slice (sMacroblock* mb,
                           sSyntaxElement *se,
                           sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  sMotionInfoContexts *ctx = slice->mot_ctx;

  int act_ctx;
  int act_sym;
  int mode_sym;
  int curr_mb_type;
  sBiContextType *mb_type_contexts = ctx->mb_type_contexts[1];

  if (biari_decode_symbol(decodingEnv, &mb_type_contexts[4] ))
  {
    if (biari_decode_symbol(decodingEnv, &mb_type_contexts[7] ))
      act_sym = 7;
    else
      act_sym = 6;
  }
  else
  {
    if (biari_decode_symbol(decodingEnv, &mb_type_contexts[5] ))
    {
      if (biari_decode_symbol(decodingEnv, &mb_type_contexts[7] ))
        act_sym = 2;
      else
        act_sym = 3;
    }
    else
    {
      if (biari_decode_symbol(decodingEnv, &mb_type_contexts[6] ))
        act_sym = 4;
      else
        act_sym = 1;
    }
  }

  if (act_sym <= 6)
  {
    curr_mb_type = act_sym;
  }
  else  // additional info for 16x16 Intra-mode
  {
    mode_sym = biari_decode_final(decodingEnv);
    if( mode_sym==1 )
    {
      curr_mb_type = 31;
    }
    else
    {
      act_ctx = 8;
      mode_sym =  biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx ); // decoding of AC/no AC
      act_sym += mode_sym*12;

      // decoding of cbp: 0,1,2
      act_ctx = 9;
      mode_sym = biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx );
      if (mode_sym != 0)
      {
        act_sym+=4;
        mode_sym = biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx );
        if (mode_sym != 0)
          act_sym+=4;
      }

      // decoding of I pred-mode: 0,1,2,3
      act_ctx = 10;
      mode_sym = biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx );
      act_sym += mode_sym*2;
      mode_sym = biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx );
      act_sym += mode_sym;
      curr_mb_type = act_sym;
    }
  }

  se->value1 = curr_mb_type;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the macroblock
 *    type info of a given MB.
** **********************************************************************
 */
void readMB_typeInfo_CABAC_b_slice (sMacroblock* mb,
                           sSyntaxElement *se,
                           sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  sMotionInfoContexts *ctx = slice->mot_ctx;

  int a = 0, b = 0;
  int act_ctx;
  int act_sym;
  int mode_sym;
  int curr_mb_type;
  sBiContextType *mb_type_contexts = ctx->mb_type_contexts[2];

  if (mb->mbCabacUp != NULL)
    b = (( (mb->mbCabacUp)->mbType != 0) ? 1 : 0 );

  if (mb->mbCabacLeft != NULL)
    a = (( (mb->mbCabacLeft)->mbType != 0) ? 1 : 0 );

  act_ctx = a + b;

  if (biari_decode_symbol (decodingEnv, &mb_type_contexts[act_ctx]))
  {
    if (biari_decode_symbol (decodingEnv, &mb_type_contexts[4]))
    {
      if (biari_decode_symbol (decodingEnv, &mb_type_contexts[5]))
      {
        act_sym = 12;
        if (biari_decode_symbol (decodingEnv, &mb_type_contexts[6]))
          act_sym += 8;
        if (biari_decode_symbol (decodingEnv, &mb_type_contexts[6]))
          act_sym += 4;
        if (biari_decode_symbol (decodingEnv, &mb_type_contexts[6]))
          act_sym += 2;

        if      (act_sym == 24)
          act_sym=11;
        else if (act_sym == 26)
          act_sym = 22;
        else
        {
          if (act_sym == 22)
            act_sym = 23;
          if (biari_decode_symbol (decodingEnv, &mb_type_contexts[6]))
            act_sym += 1;
        }
      }
      else
      {
        act_sym = 3;
        if (biari_decode_symbol (decodingEnv, &mb_type_contexts[6]))
          act_sym += 4;
        if (biari_decode_symbol (decodingEnv, &mb_type_contexts[6]))
          act_sym += 2;
        if (biari_decode_symbol (decodingEnv, &mb_type_contexts[6]))
          act_sym += 1;
      }
    }
    else
    {
      if (biari_decode_symbol (decodingEnv, &mb_type_contexts[6]))
        act_sym=2;
      else
        act_sym=1;
    }
  }
  else
  {
    act_sym = 0;
  }


  if (act_sym <= 23)
  {
    curr_mb_type = act_sym;
  }
  else  // additional info for 16x16 Intra-mode
  {
    mode_sym = biari_decode_final(decodingEnv);
    if( mode_sym == 1 )
    {
      curr_mb_type = 48;
    }
    else
    {
      mb_type_contexts = ctx->mb_type_contexts[1];
      act_ctx = 8;
      mode_sym =  biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx ); // decoding of AC/no AC
      act_sym += mode_sym*12;

      // decoding of cbp: 0,1,2
      act_ctx = 9;
      mode_sym = biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx );
      if (mode_sym != 0)
      {
        act_sym+=4;
        mode_sym = biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx );
        if (mode_sym != 0)
          act_sym+=4;
      }

      // decoding of I pred-mode: 0,1,2,3
      act_ctx = 10;
      mode_sym = biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx );
      act_sym += mode_sym*2;
      mode_sym = biari_decode_symbol(decodingEnv, mb_type_contexts + act_ctx );
      act_sym += mode_sym;
      curr_mb_type = act_sym;
    }
  }

  se->value1 = curr_mb_type;
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode a pair of
 *    intra prediction modes of a given MB.
** **********************************************************************
 */
void readIntraPredMode_CABAC (sMacroblock* mb,
                              sSyntaxElement *se,
                              sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  sTextureInfoContexts *ctx     = slice->tex_ctx;
  // use_most_probable_mode
  int act_sym = biari_decode_symbol(decodingEnv, ctx->ipr_contexts);

  // remaining_mode_selector
  if (act_sym == 1)
    se->value1 = -1;
  else
  {
    se->value1  = (biari_decode_symbol(decodingEnv, ctx->ipr_contexts + 1)     );
    se->value1 |= (biari_decode_symbol(decodingEnv, ctx->ipr_contexts + 1) << 1);
    se->value1 |= (biari_decode_symbol(decodingEnv, ctx->ipr_contexts + 1) << 2);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the reference
 *    parameter of a given MB.
** **********************************************************************
 */
void readRefFrame_CABAC (sMacroblock* mb,
                        sSyntaxElement *se,
                        sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  sPicture* picture = slice->picture;
  sMotionInfoContexts *ctx = slice->mot_ctx;
  sMacroblock *neighborMB = NULL;

  int   addctx  = 0;
  int   a = 0, b = 0;
  int   act_ctx;
  int   act_sym;
  int   list = se->value2;

  sPixelPos block_a, block_b;

  get4x4Neighbour(mb, mb->subblockX - 1, mb->subblockY    , decoder->mbSize[IS_LUMA], &block_a);
  get4x4Neighbour(mb, mb->subblockX,     mb->subblockY - 1, decoder->mbSize[IS_LUMA], &block_b);

  if (block_b.available)
  {
    int b8b=((block_b.x >> 1) & 0x01)+(block_b.y & 0x02);
    neighborMB = &slice->mbData[block_b.mbIndex];
    if (!( (neighborMB->mbType==IPCM) || IS_DIRECT(neighborMB) || (neighborMB->b8mode[b8b]==0 && neighborMB->b8pdir[b8b]==2)))
    {
      if (slice->mbAffFrameFlag && (mb->mbField == FALSE) && (neighborMB->mbField == TRUE))
        b = (picture->mvInfo[block_b.posY][block_b.posX].refIndex[list] > 1 ? 2 : 0);
      else
        b = (picture->mvInfo[block_b.posY][block_b.posX].refIndex[list] > 0 ? 2 : 0);
    }
  }

  if (block_a.available)
  {
    int b8a=((block_a.x >> 1) & 0x01)+(block_a.y & 0x02);
    neighborMB = &slice->mbData[block_a.mbIndex];
    if (!((neighborMB->mbType==IPCM) || IS_DIRECT(neighborMB) || (neighborMB->b8mode[b8a]==0 && neighborMB->b8pdir[b8a]==2)))
    {
      if (slice->mbAffFrameFlag && (mb->mbField == FALSE) && (neighborMB->mbField == 1))
        a = (picture->mvInfo[block_a.posY][block_a.posX].refIndex[list] > 1 ? 1 : 0);
      else
        a = (picture->mvInfo[block_a.posY][block_a.posX].refIndex[list] > 0 ? 1 : 0);
    }
  }

  act_ctx = a + b;
  se->context = act_ctx; // store context

  act_sym = biari_decode_symbol(decodingEnv,ctx->ref_no_contexts[addctx] + act_ctx );

  if (act_sym != 0)
  {
    act_ctx = 4;
    act_sym = unary_bin_decode(decodingEnv,ctx->ref_no_contexts[addctx] + act_ctx,1);
    ++act_sym;
  }
  se->value1 = act_sym;
}
//}}}
//{{{
void read_dQuant_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv) {

  sSlice* slice = mb->slice;
  sMotionInfoContexts* ctx = slice->mot_ctx;
  int* dquant = &se->value1;
  int act_ctx = (slice->lastDquant != 0) ? 1 : 0;
  int act_sym = biari_decode_symbol (decodingEnv, ctx->delta_qp_contexts + act_ctx);

  if (act_sym != 0) {
    act_ctx = 2;
    act_sym = unary_bin_decode (decodingEnv, ctx->delta_qp_contexts + act_ctx, 1);
    ++act_sym;
    *dquant = (act_sym + 1) >> 1;
    if ((act_sym & 0x01)==0)                           // lsb is signed bit
      *dquant = -*dquant;
    }
  else
    *dquant = 0;

  slice->lastDquant = *dquant;
  }
//}}}
//{{{
void read_CBP_CABAC (sMacroblock* mb, sSyntaxElement* se, sDecodingEnv* decodingEnv) {

  sDecoder* decoder = mb->decoder;
  sPicture* picture = mb->slice->picture;
  sSlice* slice = mb->slice;
  sTextureInfoContexts *ctx = slice->tex_ctx;
  sMacroblock *neighborMB = NULL;

  int mb_x, mb_y;
  int a = 0, b = 0;
  int curr_cbp_ctx;
  int cbp = 0;
  int cbp_bit;
  int mask;
  sPixelPos block_a;

  //  coding of luma part (bit by bit)
  for (mb_y = 0; mb_y < 4; mb_y += 2)
  {
    if (mb_y == 0)    {
      neighborMB = mb->mbCabacUp;
      b = 0;
    }

    for (mb_x=0; mb_x < 4; mb_x += 2)   {
      if (mb_y == 0)  {
        if (neighborMB != NULL)  {
          if (neighborMB->mbType!=IPCM)
            b = ((neighborMB->cbp & (1<<(2 + (mb_x>>1)))) == 0) ? 2 : 0;
          }
        }
      else
        b = ((cbp & (1<<(mb_x/2))) == 0) ? 2: 0;

      if (mb_x == 0) {
        get4x4Neighbour(mb, (mb_x<<2) - 1, (mb_y << 2), decoder->mbSize[IS_LUMA], &block_a);
        if (block_a.available) {
          if(slice->mbData[block_a.mbIndex].mbType==IPCM)
            a = 0;
          else
            a = (( (slice->mbData[block_a.mbIndex].cbp & (1<<(2*(block_a.y/2)+1))) == 0) ? 1 : 0);
          }
        else
          a = 0;
        }
      else
        a = ( ((cbp & (1<<mb_y)) == 0) ? 1: 0);

      curr_cbp_ctx = a + b;
      mask = (1 << (mb_y + (mb_x >> 1)));
      cbp_bit = biari_decode_symbol(decodingEnv, ctx->cbp_contexts[0] + curr_cbp_ctx );
      if (cbp_bit)
        cbp += mask;
      }
    }

  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
    // coding of chroma part
    // CABAC decoding for BinIdx 0
    b = 0;
    neighborMB = mb->mbCabacUp;
    if (neighborMB != NULL) {
      if (neighborMB->mbType==IPCM || (neighborMB->cbp > 15))
        b = 2;
      }

    a = 0;
    neighborMB = mb->mbCabacLeft;
    if (neighborMB != NULL) {
      if (neighborMB->mbType==IPCM || (neighborMB->cbp > 15))
        a = 1;
      }

    curr_cbp_ctx = a + b;
    cbp_bit = biari_decode_symbol(decodingEnv, ctx->cbp_contexts[1] + curr_cbp_ctx );

    // CABAC decoding for BinIdx 1
    if (cbp_bit) { // set the chroma bits
      b = 0;
      neighborMB = mb->mbCabacUp;
      if (neighborMB != NULL)  {
        //if ((neighborMB->mbType == IPCM) || ((neighborMB->cbp > 15) && ((neighborMB->cbp >> 4) == 2)))
        if ((neighborMB->mbType == IPCM) || ((neighborMB->cbp >> 4) == 2))
          b = 2;
        }

      a = 0;
      neighborMB = mb->mbCabacLeft;
      if (neighborMB != NULL) {
        if ((neighborMB->mbType == IPCM) || ((neighborMB->cbp >> 4) == 2))
          a = 1;
        }

      curr_cbp_ctx = a + b;
      cbp_bit = biari_decode_symbol(decodingEnv, ctx->cbp_contexts[2] + curr_cbp_ctx );
      cbp += (cbp_bit == 1) ? 32 : 16;
      }
    }

  se->value1 = cbp;

  if (!cbp)
    slice->lastDquant = 0;
  }
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    This function is used to arithmetically decode the chroma
 *    intra prediction mode of a given MB.
** **********************************************************************
 */
void readCIPredMode_CABAC (sMacroblock* mb,
                          sSyntaxElement *se,
                          sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  sTextureInfoContexts *ctx = slice->tex_ctx;
  int                 *act_sym  = &se->value1;

  sMacroblock          *MbUp   = mb->mbCabacUp;
  sMacroblock          *MbLeft = mb->mbCabacLeft;

  int b = (MbUp != NULL)   ? (((MbUp->cPredMode   != 0) && (MbUp->mbType != IPCM)) ? 1 : 0) : 0;
  int a = (MbLeft != NULL) ? (((MbLeft->cPredMode != 0) && (MbLeft->mbType != IPCM)) ? 1 : 0) : 0;
  int act_ctx = a + b;

  *act_sym = biari_decode_symbol(decodingEnv, ctx->cipr_contexts + act_ctx );

  if (*act_sym != 0)
    *act_sym = unary_bin_max_decode(decodingEnv, ctx->cipr_contexts + 3, 0, 1) + 1;
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Read CBP4-BIT
** **********************************************************************
*/
static int read_and_store_CBP_block_bit_444 (sMacroblock* mb,
                                             sDecodingEnv*  decodingEnv,
                                             int                     type)
{
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  sPicture* picture = slice->picture;
  sTextureInfoContexts *tex_ctx = slice->tex_ctx;
  sMacroblock *mbData = slice->mbData;
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
  int cbp_bit     = 1;  // always one for 8x8 mode
  int ctx;
  int bit_pos_a   = 0;
  int bit_pos_b   = 0;

  sPixelPos block_a, block_b;
  if (y_ac)
  {
    get4x4Neighbour(mb, i - 1, j    , decoder->mbSize[IS_LUMA], &block_a);
    get4x4Neighbour(mb, i    , j - 1, decoder->mbSize[IS_LUMA], &block_b);
    if (block_a.available)
        bit_pos_a = 4*block_a.y + block_a.x;
      if (block_b.available)
        bit_pos_b = 4*block_b.y + block_b.x;
  }
  else if (y_dc)
  {
    get4x4Neighbour(mb, i - 1, j    , decoder->mbSize[IS_LUMA], &block_a);
    get4x4Neighbour(mb, i    , j - 1, decoder->mbSize[IS_LUMA], &block_b);
  }
  else if (u_ac||v_ac)
  {
    get4x4Neighbour(mb, i - 1, j    , decoder->mbSize[IS_CHROMA], &block_a);
    get4x4Neighbour(mb, i    , j - 1, decoder->mbSize[IS_CHROMA], &block_b);
    if (block_a.available)
      bit_pos_a = 4*block_a.y + block_a.x;
    if (block_b.available)
      bit_pos_b = 4*block_b.y + block_b.x;
  }
  else
  {
    get4x4Neighbour(mb, i - 1, j    , decoder->mbSize[IS_CHROMA], &block_a);
    get4x4Neighbour(mb, i    , j - 1, decoder->mbSize[IS_CHROMA], &block_b);
  }

  if (picture->chromaFormatIdc!=YUV444)
  {
    if (type!=LUMA_8x8)
    {
      //--- get bits from neighboring blocks ---
      if (block_b.available)
      {
        if(mbData[block_b.mbIndex].mbType==IPCM)
          upper_bit=1;
        else
          upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[0].bits, bit + bit_pos_b);
      }

      if (block_a.available)
      {
        if(mbData[block_a.mbIndex].mbType==IPCM)
          left_bit=1;
        else
          left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[0].bits, bit + bit_pos_a);
      }


      ctx = 2 * upper_bit + left_bit;
      //===== encode symbol =====
      cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);
    }
  }
  else if( (decoder->sepColourPlaneFlag != 0) )
  {
    if (type!=LUMA_8x8)
    {
      //--- get bits from neighbouring blocks ---
      if (block_b.available)
      {
        if(mbData[block_b.mbIndex].mbType==IPCM)
          upper_bit = 1;
        else
          upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[0].bits,bit+bit_pos_b);
      }


      if (block_a.available)
      {
        if(mbData[block_a.mbIndex].mbType==IPCM)
          left_bit = 1;
        else
          left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[0].bits,bit+bit_pos_a);
      }


      ctx = 2 * upper_bit + left_bit;
      //===== encode symbol =====
      cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);
    }
  }
  else
  {
    if (block_b.available)
    {
      if(mbData[block_b.mbIndex].mbType==IPCM)
      {
        upper_bit=1;
      }
      else if((type==LUMA_8x8 || type==CB_8x8 || type==CR_8x8) &&
         !mbData[block_b.mbIndex].lumaTransformSize8x8flag)
      {
        upper_bit = 0;
      }
      else
      {
        if(type==LUMA_8x8)
          upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[0].bits_8x8, bit + bit_pos_b);
        else if (type==CB_8x8)
          upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[1].bits_8x8, bit + bit_pos_b);
        else if (type==CR_8x8)
          upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[2].bits_8x8, bit + bit_pos_b);
        else if ((type==CB_4x4)||(type==CB_4x8)||(type==CB_8x4)||(type==CB_16AC)||(type==CB_16DC))
          upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[1].bits,bit+bit_pos_b);
        else if ((type==CR_4x4)||(type==CR_4x8)||(type==CR_8x4)||(type==CR_16AC)||(type==CR_16DC))
          upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[2].bits,bit+bit_pos_b);
        else
          upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[0].bits,bit+bit_pos_b);
      }
    }


    if (block_a.available)
    {
      if(mbData[block_a.mbIndex].mbType==IPCM)
      {
        left_bit=1;
      }
      else if((type==LUMA_8x8 || type==CB_8x8 || type==CR_8x8) &&
         !mbData[block_a.mbIndex].lumaTransformSize8x8flag)
      {
        left_bit=0;
      }
      else
      {
        if(type==LUMA_8x8)
          left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[0].bits_8x8,bit+bit_pos_a);
        else if (type==CB_8x8)
          left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[1].bits_8x8,bit+bit_pos_a);
        else if (type==CR_8x8)
          left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[2].bits_8x8,bit+bit_pos_a);
        else if ((type==CB_4x4)||(type==CB_4x8)||(type==CB_8x4)||(type==CB_16AC)||(type==CB_16DC))
          left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[1].bits,bit+bit_pos_a);
        else if ((type==CR_4x4)||(type==CR_4x8)||(type==CR_8x4)||(type==CR_16AC)||(type==CR_16DC))
          left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[2].bits,bit+bit_pos_a);
        else
          left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[0].bits,bit+bit_pos_a);
      }
    }

    ctx = 2 * upper_bit + left_bit;
    //===== encode symbol =====
    cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);
  }

  //--- set bits for current block ---
  bit = (y_dc ? 0 : y_ac ? 1 + j + (i >> 2) : u_dc ? 17 : v_dc ? 18 : u_ac ? 19 + j + (i >> 2) : 35 + j + (i >> 2));

  if (cbp_bit)
  {
    sCBPStructure  *cbpStructure = mb->cbpStructure;
    if (type==LUMA_8x8)
    {
      cbpStructure[0].bits |= ((int64) 0x33 << bit   );

      if (picture->chromaFormatIdc==YUV444)
      {
        cbpStructure[0].bits_8x8   |= ((int64) 0x33 << bit   );
      }
    }
    else if (type==CB_8x8)
    {
      cbpStructure[1].bits_8x8   |= ((int64) 0x33 << bit   );
      cbpStructure[1].bits   |= ((int64) 0x33 << bit   );
    }
    else if (type==CR_8x8)
    {
      cbpStructure[2].bits_8x8   |= ((int64) 0x33 << bit   );
      cbpStructure[2].bits   |= ((int64) 0x33 << bit   );
    }
    else if (type==LUMA_8x4)
    {
      cbpStructure[0].bits   |= ((int64) 0x03 << bit   );
    }
    else if (type==CB_8x4)
    {
      cbpStructure[1].bits   |= ((int64) 0x03 << bit   );
    }
    else if (type==CR_8x4)
    {
      cbpStructure[2].bits   |= ((int64) 0x03 << bit   );
    }
    else if (type==LUMA_4x8)
    {
      cbpStructure[0].bits   |= ((int64) 0x11<< bit   );
    }
    else if (type==CB_4x8)
    {
      cbpStructure[1].bits   |= ((int64)0x11<< bit   );
    }
    else if (type==CR_4x8)
    {
      cbpStructure[2].bits   |= ((int64)0x11<< bit   );
    }
    else if ((type==CB_4x4)||(type==CB_16AC)||(type==CB_16DC))
    {
      cbpStructure[1].bits   |= i64_power2(bit);
    }
    else if ((type==CR_4x4)||(type==CR_16AC)||(type==CR_16DC))
    {
      cbpStructure[2].bits   |= i64_power2(bit);
    }
    else
    {
      cbpStructure[0].bits   |= i64_power2(bit);
    }
  }
  return cbp_bit;
}


static inline int set_cbp_bit(sMacroblock *neighbor_mb)
{
  if(neighbor_mb->mbType == IPCM)
    return 1;
  else
    return (int) (neighbor_mb->cbpStructure[0].bits & 0x01);
}

static inline int set_cbp_bit_ac(sMacroblock *neighbor_mb, sPixelPos *block)
{
  if (neighbor_mb->mbType == IPCM)
    return 1;
  else
  {
    int bit_pos = 1 + (block->y << 2) + block->x;
    return get_bit(neighbor_mb->cbpStructure[0].bits, bit_pos);
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Read CBP4-BIT
** **********************************************************************
 */
static int read_and_store_CBP_block_bit_normal (sMacroblock* mb,
                                                sDecodingEnv*  decodingEnv,
                                                int                     type)
{
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  sTextureInfoContexts *tex_ctx = slice->tex_ctx;
  int cbp_bit     = 1;  // always one for 8x8 mode
  sMacroblock *mbData = slice->mbData;

  if (type==LUMA_16DC)
  {
    int upper_bit   = 1;
    int left_bit    = 1;
    int ctx;

    sPixelPos block_a, block_b;

    get4x4NeighbourBase(mb, -1,  0, decoder->mbSize[IS_LUMA], &block_a);
    get4x4NeighbourBase(mb,  0, -1, decoder->mbSize[IS_LUMA], &block_b);

    //--- get bits from neighboring blocks ---
    if (block_b.available)
    {
      upper_bit = set_cbp_bit(&mbData[block_b.mbIndex]);
    }

    if (block_a.available)
    {
      left_bit = set_cbp_bit(&mbData[block_a.mbIndex]);
    }

    ctx = 2 * upper_bit + left_bit;
    //===== encode symbol =====
    cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);

    //--- set bits for current block ---

    if (cbp_bit)
    {
      mb->cbpStructure[0].bits |= 1;
    }
  }
  else if (type==LUMA_16AC)
  {
    int j           = mb->subblockY;
    int i           = mb->subblockX;
    int bit         = 1;
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit   = default_bit;
    int left_bit    = default_bit;
    int ctx;

    sPixelPos block_a, block_b;

    get4x4NeighbourBase(mb, i - 1, j    , decoder->mbSize[IS_LUMA], &block_a);
    get4x4NeighbourBase(mb, i    , j - 1, decoder->mbSize[IS_LUMA], &block_b);

    //--- get bits from neighboring blocks ---
    if (block_b.available)
    {
      upper_bit = set_cbp_bit_ac(&mbData[block_b.mbIndex], &block_b);
    }

    if (block_a.available)
    {
      left_bit = set_cbp_bit_ac(&mbData[block_a.mbIndex], &block_a);
    }

    ctx = 2 * upper_bit + left_bit;
    //===== encode symbol =====
    cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);

    if (cbp_bit)
    {
      //--- set bits for current block ---
      bit = 1 + j + (i >> 2);
      mb->cbpStructure[0].bits   |= i64_power2(bit);
    }
  }
  else if (type==LUMA_8x4)
  {
    int j           = mb->subblockY;
    int i           = mb->subblockX;
    int bit         = 1;
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit   = default_bit;
    int left_bit    = default_bit;
    int ctx;

    sPixelPos block_a, block_b;

    get4x4NeighbourBase(mb, i - 1, j    , decoder->mbSize[IS_LUMA], &block_a);
    get4x4NeighbourBase(mb, i    , j - 1, decoder->mbSize[IS_LUMA], &block_b);

    //--- get bits from neighboring blocks ---
    if (block_b.available)
    {
      upper_bit = set_cbp_bit_ac(&mbData[block_b.mbIndex], &block_b);
    }

    if (block_a.available)
    {
      left_bit = set_cbp_bit_ac(&mbData[block_a.mbIndex], &block_a);
    }

    ctx = 2 * upper_bit + left_bit;
    //===== encode symbol =====
    cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);

    if (cbp_bit)
    {
      //--- set bits for current block ---
      bit = 1 + j + (i >> 2);
      mb->cbpStructure[0].bits   |= ((int64) 0x03 << bit   );
    }
  }
  else if (type==LUMA_4x8)
  {
    int j           = mb->subblockY;
    int i           = mb->subblockX;
    int bit         = 1;
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit   = default_bit;
    int left_bit    = default_bit;
    int ctx;

    sPixelPos block_a, block_b;

    get4x4NeighbourBase(mb, i - 1, j    , decoder->mbSize[IS_LUMA], &block_a);
    get4x4NeighbourBase(mb, i    , j - 1, decoder->mbSize[IS_LUMA], &block_b);

    //--- get bits from neighboring blocks ---
    if (block_b.available)
    {
      upper_bit = set_cbp_bit_ac(&mbData[block_b.mbIndex], &block_b);
    }

    if (block_a.available)
    {
      left_bit = set_cbp_bit_ac(&mbData[block_a.mbIndex], &block_a);
    }

    ctx = 2 * upper_bit + left_bit;
    //===== encode symbol =====
    cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);

    if (cbp_bit)
    {
      //--- set bits for current block ---
      bit = 1 + j + (i >> 2);

      mb->cbpStructure[0].bits   |= ((int64) 0x11 << bit   );
    }
  }
  else if (type==LUMA_4x4)
  {
    int j           = mb->subblockY;
    int i           = mb->subblockX;
    int bit         = 1;
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit   = default_bit;
    int left_bit    = default_bit;
    int ctx;

    sPixelPos block_a, block_b;

    get4x4NeighbourBase(mb, i - 1, j    , decoder->mbSize[IS_LUMA], &block_a);
    get4x4NeighbourBase(mb, i    , j - 1, decoder->mbSize[IS_LUMA], &block_b);

    //--- get bits from neighboring blocks ---
    if (block_b.available)
    {
      upper_bit = set_cbp_bit_ac(&mbData[block_b.mbIndex], &block_b);
    }

    if (block_a.available)
    {
      left_bit = set_cbp_bit_ac(&mbData[block_a.mbIndex], &block_a);
    }

    ctx = 2 * upper_bit + left_bit;
    //===== encode symbol =====
    cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);

    if (cbp_bit)
    {
      //--- set bits for current block ---
      bit = 1 + j + (i >> 2);

      mb->cbpStructure[0].bits   |= i64_power2(bit);
    }
  }
  else if (type == LUMA_8x8)
  {
    int j           = mb->subblockY;
    int i           = mb->subblockX;
    //--- set bits for current block ---
    int bit         = 1 + j + (i >> 2);

    mb->cbpStructure[0].bits |= ((int64) 0x33 << bit   );
  }
  else if (type==CHROMA_DC || type==CHROMA_DC_2x4 || type==CHROMA_DC_4x4)
  {
    int u_dc        = (!mb->isVblock);
    int j           = 0;
    int i           = 0;
    int bit         = (u_dc ? 17 : 18);
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit   = default_bit;
    int left_bit    = default_bit;
    int ctx;

    sPixelPos block_a, block_b;

    get4x4NeighbourBase(mb, i - 1, j    , decoder->mbSize[IS_CHROMA], &block_a);
    get4x4NeighbourBase(mb, i    , j - 1, decoder->mbSize[IS_CHROMA], &block_b);

    //--- get bits from neighboring blocks ---
    if (block_b.available)
    {
      if(mbData[block_b.mbIndex].mbType==IPCM)
        upper_bit = 1;
      else
        upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[0].bits, bit);
    }

    if (block_a.available)
    {
      if(mbData[block_a.mbIndex].mbType==IPCM)
        left_bit = 1;
      else
        left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[0].bits, bit);
    }

    ctx = 2 * upper_bit + left_bit;
    //===== encode symbol =====
    cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);

    if (cbp_bit)
    {
      //--- set bits for current block ---
      bit = (u_dc ? 17 : 18);
      mb->cbpStructure[0].bits   |= i64_power2(bit);
    }
  }
  else
  {
    int u_ac        = (!mb->isVblock);
    int j           = mb->subblockY;
    int i           = mb->subblockX;
    int bit         = (u_ac ? 19 : 35);
    int default_bit = (mb->isIntraBlock ? 1 : 0);
    int upper_bit   = default_bit;
    int left_bit    = default_bit;
    int ctx;

    sPixelPos block_a, block_b;

    get4x4NeighbourBase(mb, i - 1, j    , decoder->mbSize[IS_CHROMA], &block_a);
    get4x4NeighbourBase(mb, i    , j - 1, decoder->mbSize[IS_CHROMA], &block_b);

    //--- get bits from neighboring blocks ---
    if (block_b.available)
    {
      if(mbData[block_b.mbIndex].mbType==IPCM)
        upper_bit=1;
      else
      {
        int bit_pos_b = 4*block_b.y + block_b.x;
        upper_bit = get_bit(mbData[block_b.mbIndex].cbpStructure[0].bits, bit + bit_pos_b);
      }
    }

    if (block_a.available)
    {
      if(mbData[block_a.mbIndex].mbType==IPCM)
        left_bit=1;
      else
      {
        int bit_pos_a = 4*block_a.y + block_a.x;
        left_bit = get_bit(mbData[block_a.mbIndex].cbpStructure[0].bits,bit + bit_pos_a);
      }
    }

    ctx = 2 * upper_bit + left_bit;
    //===== encode symbol =====
    cbp_bit = biari_decode_symbol (decodingEnv, tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);

    if (cbp_bit)
    {
      //--- set bits for current block ---
      bit = (u_ac ? 19 + j + (i >> 2) : 35 + j + (i >> 2));
      mb->cbpStructure[0].bits   |= i64_power2(bit);
    }
  }
  return cbp_bit;
}
//}}}
//{{{
void set_read_and_store_CBP (sMacroblock** mb, int chromaFormatIdc)
{
  if (chromaFormatIdc == YUV444)
    (*mb)->readStoreCBPblockBit = read_and_store_CBP_block_bit_444;
  else
    (*mb)->readStoreCBPblockBit = read_and_store_CBP_block_bit_normal;
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Read Significance MAP
** **********************************************************************
 */
static int read_significance_map (sMacroblock* mb,
                                  sDecodingEnv*  decodingEnv,
                                  int                     type,
                                  int                     coeff[])
{
  sSlice* slice = mb->slice;
  int               fld    = ( slice->structure!=FRAME || mb->mbField );
  const byte *pos2ctx_Map = (fld) ? pos2ctx_map_int[type] : pos2ctx_map[type];
  const byte *pos2ctx_Last = pos2ctx_last[type];

  sBiContextType*  map_ctx  = slice->tex_ctx->map_contexts [fld][type2ctx_map [type]];
  sBiContextType*  last_ctx = slice->tex_ctx->last_contexts[fld][type2ctx_last[type]];

  int   i;
  int   coefCount = 0;
  int   i0        = 0;
  int   i1        = maxpos[type];


  if (!c1isdc[type])
  {
    ++i0;
    ++i1;
  }

  for (i=i0; i < i1; ++i) // if last coeff is reached, it has to be significant
  {
    //--- read significance symbol ---
    if (biari_decode_symbol   (decodingEnv, map_ctx + pos2ctx_Map[i]))
    {
      *(coeff++) = 1;
      ++coefCount;
      //--- read last coefficient symbol ---
      if (biari_decode_symbol (decodingEnv, last_ctx + pos2ctx_Last[i]))
      {
        memset(coeff, 0, (i1 - i) * sizeof(int));
        return coefCount;
      }
    }
    else
    {
      *(coeff++) = 0;
    }
  }
  //--- last coefficient must be significant if no last symbol was received ---
  if (i < i1 + 1)
  {
    *coeff = 1;
    ++coefCount;
  }

  return coefCount;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Read Levels
** **********************************************************************
 */
static void read_significant_coefficients (sDecodingEnv* decodingEnv,
                                           sTextureInfoContexts    *tex_ctx,
                                           int                     type,
                                           int                    *coeff)
{
  sBiContextType *one_contexts = tex_ctx->one_contexts[type2ctx_one[type]];
  sBiContextType *abs_contexts = tex_ctx->abs_contexts[type2ctx_abs[type]];
  const short max_type = max_c2[type];
  int i = maxpos[type];
  int *cof = coeff + i;
  int   c1 = 1;
  int   c2 = 0;

  for (; i>=0; i--)
  {
    if (*cof != 0)
    {
      *cof += biari_decode_symbol (decodingEnv, one_contexts + c1);

      if (*cof == 2)
      {
        *cof += unary_exp_golomb_level_decode (decodingEnv, abs_contexts + c2);
        c2 = imin (++c2, max_type);
        c1 = 0;
      }
      else if (c1)
      {
        c1 = imin (++c1, 4);
      }

      if (biari_decode_symbol_eq_prob(decodingEnv))
      {
        *cof = - *cof;
      }
    }
    cof--;
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Read Block-Transform Coefficients
** **********************************************************************
 */
void readRunLevel_CABAC (sMacroblock* mb,
                         sSyntaxElement  *se,
                         sDecodingEnv* decodingEnv)
{
  sSlice* slice = mb->slice;
  int  *coefCount = &slice->coefCount;
  int  *coeff = slice->coeff;

  //--- read coefficients for whole block ---
  if (*coefCount < 0)
  {
    //===== decode CBP-BIT =====
    if ((*coefCount = mb->readStoreCBPblockBit (mb, decodingEnv, se->context) ) != 0)
    {
      //===== decode significance map =====
      *coefCount = read_significance_map (mb, decodingEnv, se->context, coeff);

      //===== decode significant coefficients =====
      read_significant_coefficients    (decodingEnv, slice->tex_ctx, se->context, coeff);
    }
  }

  //--- set run and level ---
  if (*coefCount)
  {
    //--- set run and level (coefficient) ---
    for (se->value2 = 0; coeff[slice->pos] == 0; ++slice->pos, ++se->value2);
    se->value1 = coeff[slice->pos++];
  }
  else
  {
    //--- set run and level (EOB) ---
    se->value1 = se->value2 = 0;
  }
  //--- decrement coefficient counter and re-set position ---
  if ((*coefCount)-- == 0)
    slice->pos = 0;
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    arithmetic decoding
** **********************************************************************
 */
int readsSyntaxElement_CABAC (sMacroblock* mb, sSyntaxElement* se, sDatadp* this_dataPart)
{
  sDecodingEnv* decodingEnv = &(this_dataPart->deCabac);
  int curr_len = arideco_bits_read(decodingEnv);

  // perform the actual decoding by calling the appropriate method
  se->reading(mb, se, decodingEnv);
  //read again and minus curr_len = arideco_bits_read(decodingEnv); from above
  se->len = (arideco_bits_read(decodingEnv) - curr_len);

  return (se->len);
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Read I_PCM macroblock
** **********************************************************************
*/
void readIPCMcabac (sSlice* slice, sDatadp* dp)
{
  sDecoder* decoder = slice->decoder;
  sPicture* picture = slice->picture;
  sBitstream* s = dp->bitstream;
  sDecodingEnv* dep = &(dp->deCabac);
  byte *buf = s->streamBuffer;
  int BitstreamLengthInBits = (dp->bitstream->bitstreamLength << 3) + 7;

  int val = 0;

  int bits_read = 0;
  int bitoffset, bitdepth;
  int uv, i, j;

  while (dep->DbitsLeft >= 8)
  {
    dep->Dvalue   >>= 8;
    dep->DbitsLeft -= 8;
    (*dep->Dcodestrm_len)--;
  }

  bitoffset = (*dep->Dcodestrm_len) << 3;

  // read luma values
  bitdepth = decoder->bitdepthLuma;
  for(i=0;i<MB_BLOCK_SIZE;++i)
  {
    for(j=0;j<MB_BLOCK_SIZE;++j)
    {
      bits_read += GetBits(buf, bitoffset, &val, BitstreamLengthInBits, bitdepth);
#if TRACE
      tracebits2("pcm_byte luma", bitdepth, val);
#endif
      slice->cof[0][i][j] = val;

      bitoffset += bitdepth;
    }
  }

  // read chroma values
  bitdepth = decoder->bitdepthChroma;
  if ((picture->chromaFormatIdc != YUV400) && (decoder->sepColourPlaneFlag == 0))
  {
    for (uv = 1; uv < 3; ++uv)
    {
      for(i = 0; i < decoder->mbCrSizeY; ++i)
      {
        for(j = 0; j < decoder->mbCrSizeX; ++j)
        {
          bits_read += GetBits(buf, bitoffset, &val, BitstreamLengthInBits, bitdepth);
#if TRACE
          tracebits2("pcm_byte chroma", bitdepth, val);
#endif
          slice->cof[uv][i][j] = val;

          bitoffset += bitdepth;
        }
      }
    }
  }

  (*dep->Dcodestrm_len) += ( bits_read >> 3);
  if (bits_read & 7)
  {
    ++(*dep->Dcodestrm_len);
  }
}
//}}}
