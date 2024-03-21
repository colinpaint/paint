//{{{  includes
#include "global.h"
#include "syntaxElement.h"

#include "macroblock.h"
#include "vlc.h"
#include "transform.h"
#include "mbAccess.h"
//}}}

//{{{
static int predict_nnz (sMacroBlock* mb, int block_type, int i,int j) {

  sDecoder* decoder = mb->decoder;

  // left block
  sPixelPos pixelPos;
  get4x4Neighbour (mb, i - 1, j, decoder->mbSize[IS_LUMA], &pixelPos);

  int cnt = 0;
  sSlice* slice = mb->slice;
  if ((mb->isIntraBlock == TRUE) && pixelPos.available &&
      decoder->activePPS->constrainedIntraPredFlag && (slice->dataDpMode == PAR_DP_3)) {
    pixelPos.available &= slice->intraBlock[pixelPos.mbIndex];
    if (!pixelPos.available)
      ++cnt;
    }

  int pred_nnz = 0;
  if (pixelPos.available) {
    switch (block_type) {
      //{{{
      case LUMA:
        pred_nnz = decoder->nzCoeff [pixelPos.mbIndex ][0][pixelPos.y][pixelPos.x];
        ++cnt;
        break;
      //}}}
      //{{{
      case CB:
        pred_nnz = decoder->nzCoeff [pixelPos.mbIndex ][1][pixelPos.y][pixelPos.x];
        ++cnt;
        break;
      //}}}
      //{{{
      case CR:
        pred_nnz = decoder->nzCoeff [pixelPos.mbIndex ][2][pixelPos.y][pixelPos.x];
        ++cnt;
        break;
      //}}}
      //{{{
      default:
        error("writeCoeff4x4_CAVLC: Invalid block type");
        break;
      //}}}
      }
    }

  // top block
  get4x4Neighbour (mb, i, j - 1, decoder->mbSize[IS_LUMA], &pixelPos);

  if ((mb->isIntraBlock == TRUE) && pixelPos.available &&
      decoder->activePPS->constrainedIntraPredFlag && (slice->dataDpMode == PAR_DP_3)) {
    pixelPos.available &= slice->intraBlock[pixelPos.mbIndex];
    if (!pixelPos.available)
      ++cnt;
    }

  if (pixelPos.available) {
    switch (block_type) {
      //{{{
      case LUMA:
        pred_nnz += decoder->nzCoeff [pixelPos.mbIndex ][0][pixelPos.y][pixelPos.x];
        ++cnt;
        break;
      //}}}
      //{{{
      case CB:
        pred_nnz += decoder->nzCoeff [pixelPos.mbIndex ][1][pixelPos.y][pixelPos.x];
        ++cnt;
        break;
      //}}}
      //{{{
      case CR:
        pred_nnz += decoder->nzCoeff [pixelPos.mbIndex ][2][pixelPos.y][pixelPos.x];
        ++cnt;
        break;
      //}}}
      //{{{
      default:
        error("writeCoeff4x4_CAVLC: Invalid block type");
        break;
        }
      //}}}
      }

  if (cnt==2) {
    ++pred_nnz;
    pred_nnz >>= 1;
    }

  return pred_nnz;
  }
//}}}
//{{{
static int predict_nnz_chroma (sMacroBlock* mb, int i,int j) {

  sPicture* picture = mb->slice->picture;
  if (picture->chromaFormatIdc != YUV444) {
    sDecoder* decoder = mb->decoder;
    sSlice* slice = mb->slice;
    sPixelPos pixelPos;

    int pred_nnz = 0;
    int cnt      = 0;

    //YUV420 and YUV422
    // left block
    get4x4Neighbour (mb, ((i&0x01)<<2) - 1, j, decoder->mbSize[IS_CHROMA], &pixelPos);

    if ((mb->isIntraBlock == TRUE) && pixelPos.available &&
        decoder->activePPS->constrainedIntraPredFlag && (slice->dataDpMode==PAR_DP_3)) {
      pixelPos.available &= slice->intraBlock[pixelPos.mbIndex];
      if (!pixelPos.available)
        ++cnt;
      }

    if (pixelPos.available) {
      pred_nnz = decoder->nzCoeff [pixelPos.mbIndex ][1][pixelPos.y][2 * (i>>1) + pixelPos.x];
      ++cnt;
      }

    // top block
    get4x4Neighbour (mb, ((i&0x01)<<2), j - 1, decoder->mbSize[IS_CHROMA], &pixelPos);

    if ((mb->isIntraBlock == TRUE) && pixelPos.available &&
        decoder->activePPS->constrainedIntraPredFlag && (slice->dataDpMode==PAR_DP_3)) {
      pixelPos.available &= slice->intraBlock[pixelPos.mbIndex];
      if (!pixelPos.available)
        ++cnt;
      }

    if (pixelPos.available) {
      pred_nnz += decoder->nzCoeff [pixelPos.mbIndex ][1][pixelPos.y][2 * (i>>1) + pixelPos.x];
      ++cnt;
      }

    if (cnt==2) {
      ++pred_nnz;
      pred_nnz >>= 1;
      }
    return pred_nnz;
    }
  else
    return 0;
  }
//}}}

//{{{
void readCoef4x4cavlc (sMacroBlock* mb, int block_type,
                       int i, int j, int levarr[16], int runarr[16], int *number_coefficients) {

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  int mb_nr = mb->mbIndexX;

  int k, code, vlcnum;
  int numcoeff = 0, numtrailingones;
  int level_two_or_higher;
  int totzeros, abslevel, cdc=0, cac=0;
  int zerosleft, ntr, dptype = 0;
  int max_coeff_num = 0, nnz;
  char type[15];
  static const int incVlc[] = {0, 3, 6, 12, 24, 48, 32768};    // maximum vlc = 6

  switch (block_type) {
    //{{{
    case LUMA:
      max_coeff_num = 16;
      dptype = (mb->isIntraBlock == TRUE) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case LUMA_INTRA16x16DC:
      max_coeff_num = 16;
      dptype = SE_LUM_DC_INTRA;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case LUMA_INTRA16x16AC:
      max_coeff_num = 15;
      dptype = SE_LUM_AC_INTRA;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_DC:
      max_coeff_num = decoder->coding.numCdcCoeff;
      cdc = 1;
      dptype = (mb->isIntraBlock == TRUE) ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_AC:
      max_coeff_num = 15;
      cac = 1;
      dptype = (mb->isIntraBlock == TRUE) ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    default:
      error ("readCoef4x4cavlc: invalid block type");
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    }

  sSyntaxElement se;
  se.type = dptype;
  const byte* dpMap = assignSE2dp[slice->dataDpMode];
  sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[dptype]]);
  sBitStream* s = dataPartition->s;

  if (!cdc) {
    //{{{  luma or chroma AC
    nnz = (!cac) ? predict_nnz(mb, LUMA, i<<2, j<<2) : predict_nnz_chroma(mb, i, ((j-4)<<2));
    se.value1 = (nnz < 2) ? 0 : ((nnz < 4) ? 1 : ((nnz < 8) ? 2 : 3));
    readsSyntaxElement_NumCoeffTrailingOnes(&se, s, type);
    numcoeff        =  se.value1;
    numtrailingones =  se.value2;
    decoder->nzCoeff[mb_nr][0][j][i] = (byte) numcoeff;
    }
    //}}}
  else {
    //{{{  chroma DC
    readsSyntaxElement_NumCoeffTrailingOnesChromaDC(decoder, &se, s);
    numcoeff        =  se.value1;
    numtrailingones =  se.value2;
    }
    //}}}
  memset (levarr, 0, max_coeff_num * sizeof(int));
  memset (runarr, 0, max_coeff_num * sizeof(int));

  int numones = numtrailingones;
  *number_coefficients = numcoeff;
  if (numcoeff) {
    if (numtrailingones) {
      se.len = numtrailingones;
      readsSyntaxElement_FLC (&se, s);
      code = se.inf;
      ntr = numtrailingones;
      for (k = numcoeff - 1; k > numcoeff - 1 - numtrailingones; k--) {
        ntr --;
        levarr[k] = (code>>ntr)&1 ? -1 : 1;
        }
      }

    // decode levels
    level_two_or_higher = (numcoeff > 3 && numtrailingones == 3)? 0 : 1;
    vlcnum = (numcoeff > 10 && numtrailingones < 3) ? 1 : 0;
    for (k = numcoeff - 1 - numtrailingones; k >= 0; k--) {
      //{{{  decode level
      if (vlcnum == 0)
        readsSyntaxElement_Level_VLC0(&se, s);
      else
        readsSyntaxElement_Level_VLCN(&se, vlcnum, s);

      if (level_two_or_higher) {
        se.inf += (se.inf > 0) ? 1 : -1;
        level_two_or_higher = 0;
        }

      levarr[k] = se.inf;
      abslevel = iabs(levarr[k]);
      if (abslevel  == 1)
        ++numones;

      // update VLC table
      if (abslevel  > incVlc[vlcnum])
        ++vlcnum;
      if (k == numcoeff - 1 - numtrailingones && abslevel >3)
        vlcnum = 2;
      }
      //}}}

    if (numcoeff < max_coeff_num) {

      //{{{  decode total run
      vlcnum = numcoeff - 1;
      se.value1 = vlcnum;

      if (cdc)
        readsSyntaxElement_TotalZerosChromaDC (decoder, &se, s);
      else
        readsSyntaxElement_TotalZeros(&se, s);

      totzeros = se.value1;
      }
      //}}}
    else
      totzeros = 0;
    //{{{  decode run before each coefficient
    zerosleft = totzeros;
    i = numcoeff - 1;

    if (zerosleft > 0 && i > 0) {
      do {
        // select VLC for runbefore
        vlcnum = imin(zerosleft - 1, RUNBEFORE_NUM_M1);

        se.value1 = vlcnum;
        readsSyntaxElement_Run(&se, s);
        runarr[i] = se.value1;

        zerosleft -= runarr[i];
        i --;
        } while (zerosleft != 0 && i != 0);
      }
    //}}}
    runarr[i] = zerosleft;
    }
  }
//}}}
//{{{
void readCoef4x4cavlc444 (sMacroBlock* mb, int block_type,
                               int i, int j, int levarr[16], int runarr[16], int *number_coefficients) {

  static const int incVlc[] = {0, 3, 6, 12, 24, 48, 32768};    // maximum vlc = 6

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  int mb_nr = mb->mbIndexX;

  int k, code, vlcnum;
  int numcoeff = 0, numtrailingones;
  int level_two_or_higher;
  int numones, totzeros, abslevel, cdc=0, cac=0;
  int zerosleft, ntr, dptype = 0;
  int max_coeff_num = 0, nnz;
  char type[15];

  switch (block_type) {
    //{{{
    case LUMA:
      max_coeff_num = 16;
      dptype = (mb->isIntraBlock == TRUE) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case LUMA_INTRA16x16DC:
      max_coeff_num = 16;
      dptype = SE_LUM_DC_INTRA;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case LUMA_INTRA16x16AC:
      max_coeff_num = 15;
      dptype = SE_LUM_AC_INTRA;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CB:
      max_coeff_num = 16;
      dptype = ((mb->isIntraBlock == TRUE)) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
      decoder->nzCoeff[mb_nr][1][j][i] = 0;
      break;
    //}}}
    //{{{
    case CB_INTRA16x16DC:
      max_coeff_num = 16;
      dptype = SE_LUM_DC_INTRA;
      decoder->nzCoeff[mb_nr][1][j][i] = 0;
      break;
    //}}}
    //{{{
    case CB_INTRA16x16AC:
      max_coeff_num = 15;
      dptype = SE_LUM_AC_INTRA;
      decoder->nzCoeff[mb_nr][1][j][i] = 0;
      break;
    //}}}
    //{{{
    case CR:
      max_coeff_num = 16;
      dptype = ((mb->isIntraBlock == TRUE)) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
      decoder->nzCoeff[mb_nr][2][j][i] = 0;
      break;
    //}}}
    //{{{
    case CR_INTRA16x16DC:
      max_coeff_num = 16;
      dptype = SE_LUM_DC_INTRA;
      decoder->nzCoeff[mb_nr][2][j][i] = 0;
      break;
    //}}}
    //{{{
    case CR_INTRA16x16AC:
      max_coeff_num = 15;
      dptype = SE_LUM_AC_INTRA;
      decoder->nzCoeff[mb_nr][2][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_DC:
      max_coeff_num = decoder->coding.numCdcCoeff;
      cdc = 1;
      dptype = (mb->isIntraBlock == TRUE) ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_AC:
      max_coeff_num = 15;
      cac = 1;
      dptype = (mb->isIntraBlock == TRUE) ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    default:
      error ("readCoef4x4cavlc: invalid block type");
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    }

  sSyntaxElement se;
  se.type = dptype;
  const byte* dpMap = assignSE2dp[slice->dataDpMode];
  sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[dptype]]);
  sBitStream* s = dataPartition->s;

  if (!cdc) {
    //{{{  luma or chroma AC
    if(block_type==LUMA || block_type==LUMA_INTRA16x16DC || block_type==LUMA_INTRA16x16AC ||block_type==CHROMA_AC)
      nnz = (!cac) ? predict_nnz(mb, LUMA, i<<2, j<<2) : predict_nnz_chroma(mb, i, ((j-4)<<2));
    else if (block_type==CB || block_type==CB_INTRA16x16DC || block_type==CB_INTRA16x16AC)
      nnz = predict_nnz(mb, CB, i<<2, j<<2);
    else
      nnz = predict_nnz(mb, CR, i<<2, j<<2);

    se.value1 = (nnz < 2) ? 0 : ((nnz < 4) ? 1 : ((nnz < 8) ? 2 : 3));
    readsSyntaxElement_NumCoeffTrailingOnes(&se, s, type);
    numcoeff        =  se.value1;
    numtrailingones =  se.value2;
    if  (block_type==LUMA || block_type==LUMA_INTRA16x16DC || block_type==LUMA_INTRA16x16AC ||block_type==CHROMA_AC)
      decoder->nzCoeff[mb_nr][0][j][i] = (byte) numcoeff;
    else if (block_type==CB || block_type==CB_INTRA16x16DC || block_type==CB_INTRA16x16AC)
      decoder->nzCoeff[mb_nr][1][j][i] = (byte) numcoeff;
    else
      decoder->nzCoeff[mb_nr][2][j][i] = (byte) numcoeff;
    }
    //}}}
  else {
    //{{{  chroma DC
    readsSyntaxElement_NumCoeffTrailingOnesChromaDC(decoder, &se, s);
    numcoeff        =  se.value1;
    numtrailingones =  se.value2;
    }
    //}}}
  memset (levarr, 0, max_coeff_num * sizeof(int));
  memset (runarr, 0, max_coeff_num * sizeof(int));

  numones = numtrailingones;
  *number_coefficients = numcoeff;
  if (numcoeff) {
    if (numtrailingones) {
      se.len = numtrailingones;
      readsSyntaxElement_FLC (&se, s);
      code = se.inf;
      ntr = numtrailingones;
      for (k = numcoeff - 1; k > numcoeff - 1 - numtrailingones; k--) {
        ntr --;
        levarr[k] = (code>>ntr)&1 ? -1 : 1;
        }
      }

    // decode levels
    level_two_or_higher = (numcoeff > 3 && numtrailingones == 3)? 0 : 1;
    vlcnum = (numcoeff > 10 && numtrailingones < 3) ? 1 : 0;
    for (k = numcoeff - 1 - numtrailingones; k >= 0; k--) {
      //{{{  decode level
      if (vlcnum == 0)
        readsSyntaxElement_Level_VLC0(&se, s);
      else
        readsSyntaxElement_Level_VLCN(&se, vlcnum, s);

      if (level_two_or_higher) {
        se.inf += (se.inf > 0) ? 1 : -1;
        level_two_or_higher = 0;
        }

      levarr[k] = se.inf;
      abslevel = iabs(levarr[k]);
      if (abslevel  == 1)
        ++numones;

      // update VLC table
      if (abslevel  > incVlc[vlcnum])
        ++vlcnum;

      if (k == numcoeff - 1 - numtrailingones && abslevel >3)
        vlcnum = 2;
      }
      //}}}

    if (numcoeff < max_coeff_num) {
      //{{{  decode total run
      vlcnum = numcoeff - 1;
      se.value1 = vlcnum;

      if (cdc)
        readsSyntaxElement_TotalZerosChromaDC(decoder, &se, s);
      else
        readsSyntaxElement_TotalZeros(&se, s);

      totzeros = se.value1;
      }
      //}}}
    else
      totzeros = 0;

    // decode run before each coefficient
    zerosleft = totzeros;
    i = numcoeff - 1;
    if (zerosleft > 0 && i > 0) {
      do {
        // select VLC for runbefore
        vlcnum = imin(zerosleft - 1, RUNBEFORE_NUM_M1);
        se.value1 = vlcnum;
        readsSyntaxElement_Run(&se, s);
        runarr[i] = se.value1;
        zerosleft -= runarr[i];
        i --;
        } while (zerosleft != 0 && i != 0);
      }
    runarr[i] = zerosleft;
    }
  }
//}}}

//{{{
static void readCompCoef4x4cavlc (sMacroBlock* mb, eColorPlane plane,
                                  int (*InvLevelScale4x4)[4], int qp_per, int cbp, byte** nzcoeff) {

  int i0, j0;
  int levarr[16] = {0}, runarr[16] = {0}, numcoeff;

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  const byte (*pos_scan4x4)[2] = ((decoder->coding.structure == FRAME) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];
  int start_scan = IS_I16MB(mb) ? 1 : 0;
  int64 *cur_cbp = &mb->cbpStructure[plane].blk;
  int cur_context;

  if (IS_I16MB(mb)) {
    if (plane == PLANE_Y)
      cur_context = LUMA_INTRA16x16AC;
    else if (plane == PLANE_U)
      cur_context = CB_INTRA16x16AC;
    else
      cur_context = CR_INTRA16x16AC;
    }
  else {
    if (plane == PLANE_Y)
      cur_context = LUMA;
    else if (plane == PLANE_U)
      cur_context = CB;
    else
      cur_context = CR;
    }

  for (int blockY = 0; blockY < 4; blockY += 2) {
    /* all modes */
    int block_y4 = blockY << 2;
    for (int blockX = 0; blockX < 4; blockX += 2) {
      int block_x4 = blockX << 2;
      int b8 = (blockY + (blockX >> 1));
      if (cbp & (1 << b8))  {
        // test if the block contains any coefficients
        for (int j = block_y4; j < block_y4 + 8; j += BLOCK_SIZE)
          for (int i = block_x4; i < block_x4 + 8; i += BLOCK_SIZE) {
            slice->readCoef4x4cavlc(mb, cur_context, i >> 2, j >> 2, levarr, runarr, &numcoeff);
            pos_scan_4x4 = pos_scan4x4[start_scan];
            for (int k = 0; k < numcoeff; ++k) {
              if (levarr[k] != 0) {
                pos_scan_4x4 += (runarr[k] << 1);
                i0 = *pos_scan_4x4++;
                j0 = *pos_scan_4x4++;

                // inverse quant for 4x4 transform only
                *cur_cbp |= i64_power2(j + (i >> 2));
                slice->cof[plane][j + j0][i + i0]= rshift_rnd_sf((levarr[k] * InvLevelScale4x4[j0][i0])<<qp_per, 4);
                }
              }
            }
        }
      else {
        nzcoeff[blockY    ][blockX    ] = 0;
        nzcoeff[blockY    ][blockX + 1] = 0;
        nzcoeff[blockY + 1][blockX    ] = 0;
        nzcoeff[blockY + 1][blockX + 1] = 0;
        }
      }
    }
  }
//}}}
//{{{
static void read_comp_coeff_4x4_CAVLC_ls (sMacroBlock* mb, eColorPlane plane,
                                          int (*InvLevelScale4x4)[4], int qp_per, int cbp, byte** nzcoeff) {

  int levarr[16] = {0}, runarr[16] = {0}, numcoeff;

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  const byte (*pos_scan4x4)[2] = ((decoder->coding.structure == FRAME) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  int start_scan = IS_I16MB(mb) ? 1 : 0;
  int64* cur_cbp = &mb->cbpStructure[plane].blk;

  int coef_ctr, cur_context;
  if (IS_I16MB(mb)) {
    if (plane == PLANE_Y)
      cur_context = LUMA_INTRA16x16AC;
    else if (plane == PLANE_U)
      cur_context = CB_INTRA16x16AC;
    else
      cur_context = CR_INTRA16x16AC;
    }
  else {
    if (plane == PLANE_Y)
      cur_context = LUMA;
    else if (plane == PLANE_U)
      cur_context = CB;
    else
      cur_context = CR;
    }

  for (int blockY = 0; blockY < 4; blockY += 2) {
    for (int blockX = 0; blockX < 4; blockX += 2) {
      int b8 = 2*(blockY>>1) + (blockX>>1);
      if (cbp & (1 << b8)) {
        /* are there any coeff in current block at all */
        for (int j = blockY; j < blockY+2; ++j) {
          for (int i = blockX; i < blockX+2; ++i) {
            slice->readCoef4x4cavlc (mb, cur_context, i, j, levarr, runarr, &numcoeff);
            coef_ctr = start_scan - 1;
            for (int k = 0; k < numcoeff; ++k) {
              if (levarr[k] != 0) {
                coef_ctr += runarr[k]+1;
                int i0 = pos_scan4x4[coef_ctr][0];
                int j0 = pos_scan4x4[coef_ctr][1];
                *cur_cbp |= i64_power2((j<<2) + i);
                slice->cof[plane][(j<<2) + j0][(i<<2) + i0]= levarr[k];
                }
              }
            }
          }
        }
      else {
        nzcoeff[blockY    ][blockX    ] = 0;
        nzcoeff[blockY    ][blockX + 1] = 0;
        nzcoeff[blockY + 1][blockX    ] = 0;
        nzcoeff[blockY + 1][blockX + 1] = 0;
        }
      }
    }
  }
//}}}

//{{{
static void readCompCoef8x8cavlc (sMacroBlock* mb, eColorPlane plane,
                                  int (*InvLevelScale8x8)[8], int qp_per, int cbp, byte** nzcoeff) {

  int levarr[16] = {0}, runarr[16] = {0}, numcoeff;
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  const byte (*pos_scan8x8)[2] = ((decoder->coding.structure == FRAME) && (!mb->mbField)) ? SNGL_SCAN8x8 : FIELD_SCAN8x8;
  int start_scan = IS_I16MB(mb) ? 1 : 0;
  int64 *cur_cbp = &mb->cbpStructure[plane].blk;
  int coef_ctr, cur_context;

  if (IS_I16MB(mb)) {
    if (plane == PLANE_Y)
      cur_context = LUMA_INTRA16x16AC;
    else if (plane == PLANE_U)
      cur_context = CB_INTRA16x16AC;
    else
      cur_context = CR_INTRA16x16AC;
    }
  else {
    if (plane == PLANE_Y)
      cur_context = LUMA;
    else if (plane == PLANE_U)
      cur_context = CB;
    else
      cur_context = CR;
    }

  for (int blockY = 0; blockY < 4; blockY += 2) {
    int block_y4 = blockY << 2;
    for (int blockX = 0; blockX < 4; blockX += 2) {
      int block_x4 = blockX << 2;
      int b8 = blockY + (blockX>>1);
      if (cbp & (1<<b8)) {
      /* are there any coeff in current block at all */
        for (int j = blockY; j < blockY + 2; ++j) {
          for (int i = blockX; i < blockX + 2; ++i) {
            slice->readCoef4x4cavlc(mb, cur_context, i, j, levarr, runarr, &numcoeff);
            coef_ctr = start_scan - 1;
            for (int k = 0; k < numcoeff; ++k) {
              if (levarr[k] != 0) {
                coef_ctr += runarr[k] + 1;
                // do same as CABAC for deblocking: any coeff in the 8x8 marks all the 4x4s
                //as containing coefficients
                *cur_cbp |= 51 << (block_y4 + blockX);
                int b4 = (coef_ctr << 2) + 2*(j - blockY) + (i - blockX);
                int i0 = pos_scan8x8[b4][0];
                int j0 = pos_scan8x8[b4][1];
                slice->mbRess[plane][block_y4 +j0][block_x4 +i0] = rshift_rnd_sf((levarr[k] * InvLevelScale8x8[j0][i0])<<qp_per, 6); // dequantization
                }
              }
            }
          }
        }
      else {
        nzcoeff[blockY    ][blockX    ] = 0;
        nzcoeff[blockY    ][blockX + 1] = 0;
        nzcoeff[blockY + 1][blockX    ] = 0;
        nzcoeff[blockY + 1][blockX + 1] = 0;
        }
      }
    }
  }
//}}}
//{{{
static void read_comp_coeff_8x8_CAVLC_ls (sMacroBlock* mb, eColorPlane plane,
                                          int (*InvLevelScale8x8)[8], int qp_per, int cbp, byte** nzcoeff) {

  int levarr[16] = {0}, runarr[16] = {0}, numcoeff;

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  const byte (*pos_scan8x8)[2] = ((decoder->coding.structure == FRAME) && (!mb->mbField)) ? SNGL_SCAN8x8 : FIELD_SCAN8x8;
  int start_scan = IS_I16MB(mb) ? 1 : 0;
  int64*cur_cbp = &mb->cbpStructure[plane].blk;

  int coef_ctr, cur_context;
  if (IS_I16MB(mb)) {
    if (plane == PLANE_Y)
      cur_context = LUMA_INTRA16x16AC;
    else if (plane == PLANE_U)
      cur_context = CB_INTRA16x16AC;
    else
      cur_context = CR_INTRA16x16AC;
    }
  else {
    if (plane == PLANE_Y)
      cur_context = LUMA;
    else if (plane == PLANE_U)
      cur_context = CB;
    else
      cur_context = CR;
    }

  for (int blockY = 0; blockY < 4; blockY += 2) {
    for (int blockX = 0; blockX < 4; blockX += 2) {
      int b8 = 2 * (blockY >> 1) + (blockX >> 1);
      if (cbp & (1<<b8)) {
        /* are there any coeff in current block at all */
        for (int j = blockY; j < blockY+2; ++j) {
          for (int i = blockX; i < blockX+2; ++i) {
            slice->readCoef4x4cavlc (mb, cur_context, i, j, levarr, runarr, &numcoeff);
            coef_ctr = start_scan - 1;
            for (int k = 0; k < numcoeff; ++k) {
              if (levarr[k] != 0) {
                coef_ctr += runarr[k]+1;
                // do same as CABAC for deblocking: any coeff in the 8x8 marks all the 4x4s
                // as containing coefficients
                *cur_cbp |= 51 << ((blockY<<2) + blockX);
                int b4 = 2*(j-blockY)+(i-blockX);
                int iz = pos_scan8x8[coef_ctr*4+b4][0];
                int jz = pos_scan8x8[coef_ctr*4+b4][1];
                slice->mbRess[plane][blockY*4 +jz][blockX*4 +iz] = levarr[k];
                }
              }
            }
          }
        }
      else {
        nzcoeff[blockY    ][blockX    ] = 0;
        nzcoeff[blockY    ][blockX + 1] = 0;
        nzcoeff[blockY + 1][blockX    ] = 0;
        nzcoeff[blockY + 1][blockX + 1] = 0;
        }
      }
    }
  }
//}}}

//{{{
static void read_CBP_and_coeffs_from_NAL_CAVLC_400 (sMacroBlock* mb) {

  int k;
  int mb_nr = mb->mbIndexX;
  int cbp;

  sSlice* slice = mb->slice;
  int i0, j0;

  int levarr[16], runarr[16], numcoeff;
  int qp_per, qp_rem;

  sSyntaxElement se;
  sDataPartition* dataPartition = NULL;
  const byte* dpMap = assignSE2dp[slice->dataDpMode];
  sDecoder* decoder = mb->decoder;
  int intra = (mb->isIntraBlock == TRUE);
  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  int (*InvLevelScale8x8)[8] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((decoder->coding.structure == FRAME) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];

  // read CBP if not new intra mode
  if (!IS_I16MB (mb)) {
    se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB) ? SE_CBP_INTRA : SE_CBP_INTER;
    dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
    se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
      ? slice->linfoCbpIntra : slice->linfoCbpInter;
    dataPartition->readSyntaxElement (mb, &se, dataPartition);
    mb->cbp = cbp = se.value1;

    // Transform size flag for INTER MBs
    need_transform_size_flag = (((mb->mbType >= 1 && mb->mbType <= 3)||
                                 (IS_DIRECT(mb) && decoder->activeSPS->direct_8x8_inference_flag) ||
                                 (mb->noMbPartLessThan8x8Flag))
                               && mb->mbType != I8MB && mb->mbType != I4MB
                               && (mb->cbp&15)
                               && slice->transform8x8Mode);

    if (need_transform_size_flag) {
      //{{{  read CAVLC transform_size_8x8_flag
      se.type   =  SE_HEADER;
      dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
      se.len = 1;
      readsSyntaxElement_FLC(&se, dataPartition->s);
      mb->lumaTransformSize8x8flag = (Boolean) se.value1;
      }
      //}}}
    if (cbp !=0) {
      //{{{  Delta quant only if nonzero coeffs
      readDeltaQuant(&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (slice->dataDpMode) {
        if ((mb->isIntraBlock == FALSE) && slice->noDataPartitionC )
          mb->dplFlag = 1;

        if( intra && slice->noDataPartitionB ) {
          mb->errorFlag = 1;
          mb->dplFlag = 1;
          }

        // check for prediction from neighbours
        checkDpNeighbours (mb);
        if (mb->dplFlag) {
          cbp = 0;
          mb->cbp = cbp;
          }
        }
      }
      //}}}
    }
  else  {
    //{{{  read DC coeffs for new intra modes
    cbp = mb->cbp;
    readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);
    if (slice->dataDpMode) {
      if (slice->noDataPartitionB) {
        mb->errorFlag  = 1;
        mb->dplFlag = 1;
      }
      checkDpNeighbours (mb);
      if (mb->dplFlag)
        mb->cbp = cbp = 0;
      }

    if (!mb->dplFlag) {
      pos_scan_4x4 = pos_scan4x4[0];
      slice->readCoef4x4cavlc (mb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
      for(k = 0; k < numcoeff; ++k) {
        if (levarr[k] != 0) {
          // leave if level == 0
          pos_scan_4x4 += 2 * runarr[k];
          i0 = ((*pos_scan_4x4++) << 2);
          j0 = ((*pos_scan_4x4++) << 2);
          slice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
          }
        }


      if (mb->isLossless == FALSE)
        itrans_2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
      }
    }
    //}}}

  updateQp (mb, slice->qp);

  qp_per = decoder->qpPerMatrix[ mb->qpScaled[PLANE_Y] ];
  qp_rem = decoder->qpRemMatrix[ mb->qpScaled[PLANE_Y] ];

  InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
  InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

  // luma coefficients
  if (cbp) {
    if (!mb->lumaTransformSize8x8flag) // 4x4 transform
      mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, cbp, decoder->nzCoeff[mb_nr][PLANE_Y]);
    else // 8x8 transform
      mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, cbp, decoder->nzCoeff[mb_nr][PLANE_Y]);
    }
  else
    memset(decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(byte));
  }
//}}}
//{{{
static void read_CBP_and_coeffs_from_NAL_CAVLC_422 (sMacroBlock* mb) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;

  int i,j,k;
  int mb_nr = mb->mbIndexX;
  int cbp;

  int coef_ctr, i0, j0, b8;
  int ll;
  int levarr[16], runarr[16], numcoeff;

  int qp_per, qp_rem;

  int uv;
  int qp_per_uv[2];
  int qp_rem_uv[2];

  int intra = (mb->isIntraBlock == TRUE);

  int b4;
  int m6[4];

  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  int (*InvLevelScale8x8)[8] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((decoder->coding.structure == FRAME) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];

  // read CBP if not new intra mode
  sSyntaxElement se;
  const byte* dpMap = assignSE2dp[slice->dataDpMode];
  sDataPartition* dataPartition = NULL;

  if (!IS_I16MB (mb)) {
    se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
      ? SE_CBP_INTRA : SE_CBP_INTER;
    dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
    se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
      ? slice->linfoCbpIntra : slice->linfoCbpInter;
    dataPartition->readSyntaxElement (mb, &se, dataPartition);
    mb->cbp = cbp = se.value1;

    need_transform_size_flag = (((mb->mbType >= 1 && mb->mbType <= 3)||
      (IS_DIRECT(mb) && decoder->activeSPS->direct_8x8_inference_flag) ||
      (mb->noMbPartLessThan8x8Flag))
      && mb->mbType != I8MB && mb->mbType != I4MB
      && (mb->cbp&15)
      && slice->transform8x8Mode);

    if (need_transform_size_flag) {
      //{{{  Transform size flag for INTER MBs
      se.type   =  SE_HEADER;
      dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);

      // read CAVLC transform_size_8x8_flag
      se.len = 1;
      readsSyntaxElement_FLC(&se, dataPartition->s);
      mb->lumaTransformSize8x8flag = (Boolean) se.value1;
      }
      //}}}
    if (cbp != 0) {
      //{{{  Delta quant only if nonzero coeffs
      readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (slice->dataDpMode) {
        if ((mb->isIntraBlock == FALSE) && slice->noDataPartitionC )
          mb->dplFlag = 1;

        if( intra && slice->noDataPartitionB ) {
          mb->errorFlag = 1;
          mb->dplFlag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (mb);
        if (mb->dplFlag) {
          cbp = 0;
          mb->cbp = cbp;
          }
       }
      }
      //}}}
    }
  else {
    //{{{  read DC coeffs for new intra modes
    cbp = mb->cbp;

    readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);

    if (slice->dataDpMode) {
      if (slice->noDataPartitionB) {
        mb->errorFlag  = 1;
        mb->dplFlag = 1;
      }
      checkDpNeighbours (mb);
      if (mb->dplFlag)
        mb->cbp = cbp = 0;
      }

    if (!mb->dplFlag) {
      pos_scan_4x4 = pos_scan4x4[0];
      slice->readCoef4x4cavlc(mb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
      for(k = 0; k < numcoeff; ++k) {
        if (levarr[k] != 0) {
          // leave if level == 0
          pos_scan_4x4 += 2 * runarr[k];
          i0 = ((*pos_scan_4x4++) << 2);
          j0 = ((*pos_scan_4x4++) << 2);
          slice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
          }
        }
      if (mb->isLossless == FALSE)
        itrans_2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
      }
    }
    //}}}

  updateQp (mb, slice->qp);
  qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
  qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

  // init quant parameters for chroma
  for (i=0; i < 2; ++i) {
    qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
    qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
    }

  InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
  InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

  // luma coefficients
  if (cbp) {
    if (!mb->lumaTransformSize8x8flag) // 4x4 transform
      mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, cbp, decoder->nzCoeff[mb_nr][PLANE_Y]);
    else // 8x8 transform
      mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, cbp, decoder->nzCoeff[mb_nr][PLANE_Y]);
    }
  else
    memset(decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(byte));

  //{{{  chroma DC coeff
  if (cbp>15) {
    for (ll=0;ll<3;ll+=2) {
      int (*InvLevelScale4x4)[4] = NULL;
      uv = ll>>1;
      {
        int** imgcof = slice->cof[PLANE_U + uv];
        int m3[2][4] = {{0,0,0,0},{0,0,0,0}};
        int m4[2][4] = {{0,0,0,0},{0,0,0,0}};
        int qp_per_uv_dc = decoder->qpPerMatrix[ (mb->qpc[uv] + 3 + decoder->coding.bitDepthChromaQpScale) ];       //for YUV422 only
        int qp_rem_uv_dc = decoder->qpRemMatrix[ (mb->qpc[uv] + 3 + decoder->coding.bitDepthChromaQpScale) ];       //for YUV422 only
        if (intra)
          InvLevelScale4x4 = slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv_dc];
        else
          InvLevelScale4x4 = slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv_dc];


        // CHROMA DC YUV422
        slice->readCoef4x4cavlc(mb, CHROMA_DC, 0, 0, levarr, runarr, &numcoeff);
        coef_ctr=-1;
        for(k = 0; k < numcoeff; ++k) {
          if (levarr[k] != 0) {
            mb->cbpStructure[0].blk |= ((int64)0xff0000) << (ll<<2);
            coef_ctr += runarr[k]+1;
            i0 = SCAN_YUV422[coef_ctr][0];
            j0 = SCAN_YUV422[coef_ctr][1];

            m3[i0][j0]=levarr[k];
          }
        }

        // inverse CHROMA DC YUV422 transform horizontal
        if(mb->isLossless == FALSE) {
          m4[0][0] = m3[0][0] + m3[1][0];
          m4[0][1] = m3[0][1] + m3[1][1];
          m4[0][2] = m3[0][2] + m3[1][2];
          m4[0][3] = m3[0][3] + m3[1][3];

          m4[1][0] = m3[0][0] - m3[1][0];
          m4[1][1] = m3[0][1] - m3[1][1];
          m4[1][2] = m3[0][2] - m3[1][2];
          m4[1][3] = m3[0][3] - m3[1][3];

          for (i = 0; i < 2; ++i) {
            m6[0] = m4[i][0] + m4[i][2];
            m6[1] = m4[i][0] - m4[i][2];
            m6[2] = m4[i][1] - m4[i][3];
            m6[3] = m4[i][1] + m4[i][3];

            imgcof[ 0][i<<2] = m6[0] + m6[3];
            imgcof[ 4][i<<2] = m6[1] + m6[2];
            imgcof[ 8][i<<2] = m6[1] - m6[2];
            imgcof[12][i<<2] = m6[0] - m6[3];
            }//for (i=0;i<2;++i)

          for(j = 0;j < decoder->mbCrSizeY; j += BLOCK_SIZE) {
            for(i=0;i < decoder->mbCrSizeX;i+=BLOCK_SIZE)
              imgcof[j][i] = rshift_rnd_sf((imgcof[j][i] * InvLevelScale4x4[0][0]) << qp_per_uv_dc, 6);
            }
         }
         else {
          for(j=0;j<4;++j) {
            slice->cof[PLANE_U + uv][j<<2][0] = m3[0][j];
            slice->cof[PLANE_U + uv][j<<2][4] = m3[1][j];
            }
          }

        }
      }//for (ll=0;ll<3;ll+=2)
    }
  //}}}
  //{{{  chroma AC coeff, all zero fram start_scan
  if (cbp<=31)
    memset (decoder->nzCoeff [mb_nr ][1][0], 0, 2 * BLOCK_PIXELS * sizeof(byte));
  else {
    if(mb->isLossless == FALSE) {
      for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
        mb->isVblock = uv = (b8 > ((decoder->coding.numUvBlocks) - 1 ));
        InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];

        for (b4=0; b4 < 4; ++b4) {
          i = cofuv_blk_x[1][b8][b4];
          j = cofuv_blk_y[1][b8][b4];

          slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
          coef_ctr = 0;

          for(k = 0; k < numcoeff;++k) {
            if (levarr[k] != 0) {
              mb->cbpStructure[0].blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              coef_ctr += runarr[k] + 1;

              i0=pos_scan4x4[coef_ctr][0];
              j0=pos_scan4x4[coef_ctr][1];

              slice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((levarr[k] * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
              }
            }
          }
        }
      }
    else {
      for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
        mb->isVblock = uv = (b8 > ((decoder->coding.numUvBlocks) - 1 ));
        for (b4=0; b4 < 4; ++b4) {
          i = cofuv_blk_x[1][b8][b4];
          j = cofuv_blk_y[1][b8][b4];

          slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
          coef_ctr = 0;

          for(k = 0; k < numcoeff;++k) {
            if (levarr[k] != 0) {
              mb->cbpStructure[0].blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              coef_ctr += runarr[k] + 1;
              i0=pos_scan4x4[coef_ctr][0];
              j0=pos_scan4x4[coef_ctr][1];
              slice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = levarr[k];
              }
            }
          }
        }
      }
    }
  //}}}
  }
//}}}
//{{{
static void read_CBP_and_coeffs_from_NAL_CAVLC_444 (sMacroBlock* mb) {

  sSlice* slice = mb->slice;

  int i,k;
  int mb_nr = mb->mbIndexX;
  int cbp;
  int coef_ctr, i0, j0;
  int levarr[16], runarr[16], numcoeff;

  int qp_per, qp_rem;
  int uv;
  int qp_per_uv[3];
  int qp_rem_uv[3];
  int need_transform_size_flag;

  sSyntaxElement se;
  sDataPartition *dataPartition = NULL;
  const byte* dpMap = assignSE2dp[slice->dataDpMode];
  sDecoder* decoder = mb->decoder;

  int intra = (mb->isIntraBlock == TRUE);

  int (*InvLevelScale4x4)[4] = NULL;
  int (*InvLevelScale8x8)[8] = NULL;

  // select scan type
  const byte (*pos_scan4x4)[2] = ((decoder->coding.structure == FRAME) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];

  // read CBP if not new intra mode
  if (!IS_I16MB (mb)) {
    se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
      ? SE_CBP_INTRA : SE_CBP_INTER;
    dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
    se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
      ? slice->linfoCbpIntra : slice->linfoCbpInter;
    dataPartition->readSyntaxElement (mb, &se, dataPartition);
    mb->cbp = cbp = se.value1;

    //============= Transform size flag for INTER MBs =============
    need_transform_size_flag = (((mb->mbType >= 1 && mb->mbType <= 3)||
      (IS_DIRECT(mb) && decoder->activeSPS->direct_8x8_inference_flag) ||
      (mb->noMbPartLessThan8x8Flag))
      && mb->mbType != I8MB && mb->mbType != I4MB
      && (mb->cbp&15)
      && slice->transform8x8Mode);

    if (need_transform_size_flag) {
      se.type   =  SE_HEADER;
      dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
      // read CAVLC transform_size_8x8_flag
      se.len = 1;
      readsSyntaxElement_FLC(&se, dataPartition->s);
      mb->lumaTransformSize8x8flag = (Boolean) se.value1;
      }

    // Delta quant only if nonzero coeffs
    if (cbp !=0) {
      readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (slice->dataDpMode) {
        if ((mb->isIntraBlock == FALSE) && slice->noDataPartitionC )
          mb->dplFlag = 1;

        if( intra && slice->noDataPartitionB ) {
          mb->errorFlag = 1;
          mb->dplFlag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (mb);
        if (mb->dplFlag) {
          cbp = 0;
          mb->cbp = cbp;
        }
      }
    }
  }
  else {
    //{{{  read DC coeffs for new intra modes
    cbp = mb->cbp;
    readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);
    if (slice->dataDpMode) {
      if (slice->noDataPartitionB) {
        mb->errorFlag  = 1;
        mb->dplFlag = 1;
        }
      checkDpNeighbours (mb);
      if (mb->dplFlag)
        mb->cbp = cbp = 0;
      }

    if (!mb->dplFlag) {
      pos_scan_4x4 = pos_scan4x4[0];
      slice->readCoef4x4cavlc(mb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
      for(k = 0; k < numcoeff; ++k) {
        if (levarr[k] != 0) {
          // leave if level == 0
          pos_scan_4x4 += 2 * runarr[k];
          i0 = ((*pos_scan_4x4++) << 2);
          j0 = ((*pos_scan_4x4++) << 2);
          slice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
          }
        }
      if (mb->isLossless == FALSE)
        itrans_2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
      }
    }
    //}}}

  updateQp(mb, slice->qp);
  qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
  qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

  //init quant parameters for chroma
  for (i=PLANE_U; i <= PLANE_V; ++i) {
    qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i] ];
    qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i] ];
    }

  InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
  InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

  // luma coefficients
  if (cbp) {
    if (!mb->lumaTransformSize8x8flag) // 4x4 transform
      mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, cbp, decoder->nzCoeff[mb_nr][PLANE_Y]);
    else // 8x8 transform
      mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, cbp, decoder->nzCoeff[mb_nr][PLANE_Y]);
  }
  else
    memset(decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(byte));

  for (uv = PLANE_U; uv <= PLANE_V; ++uv ) {
    //{{{  16x16DC Luma_Add
    if (IS_I16MB (mb)) {
      // read DC coeffs for new intra modes
      if (uv == PLANE_U)
        slice->readCoef4x4cavlc(mb, CB_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
      else
        slice->readCoef4x4cavlc(mb, CR_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);

      coef_ctr=-1;

      for(k = 0; k < numcoeff; ++k) {
        if (levarr[k] != 0) {
          // leave if level == 0
          coef_ctr += runarr[k] + 1;
          i0 = pos_scan4x4[coef_ctr][0];
          j0 = pos_scan4x4[coef_ctr][1];
          slice->cof[uv][j0<<2][i0<<2] = levarr[k];// add new intra DC coeff
          } //if leavarr[k]
        } //k loop

      if(mb->isLossless == FALSE)
        itrans_2(mb, (eColorPlane) (uv)); // transform new intra DC
      } //IS_I16MB

    //init constants for every chroma qp offset
    updateQp(mb, slice->qp);
    qp_per_uv[uv] = decoder->qpPerMatrix[ mb->qpScaled[uv] ];
    qp_rem_uv[uv] = decoder->qpRemMatrix[ mb->qpScaled[uv] ];

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv][qp_rem_uv[uv]];
    InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[uv][qp_rem_uv[uv]] : slice->InvLevelScale8x8_Inter[uv][qp_rem_uv[uv]];

    if (!mb->lumaTransformSize8x8flag) // 4x4 transform
      mb->readCompCoef4x4cavlc (mb, (eColorPlane) (uv), InvLevelScale4x4, qp_per_uv[uv], cbp, decoder->nzCoeff[mb_nr][uv]);
    else // 8x8 transform
      mb->readCompCoef8x8cavlc (mb, (eColorPlane) (uv), InvLevelScale8x8, qp_per_uv[uv], cbp, decoder->nzCoeff[mb_nr][uv]);
    }
    //}}}
  }
//}}}
//{{{
static void read_CBP_and_coeffs_from_NAL_CAVLC_420 (sMacroBlock* mb) {

  sSlice* slice = mb->slice;

  int i,j,k;
  int mb_nr = mb->mbIndexX;
  int cbp;
  int coef_ctr, i0, j0, b8;
  int ll;
  int levarr[16], runarr[16], numcoeff;

  int qp_per, qp_rem;

  sSyntaxElement se;
  sDataPartition* dataPartition = NULL;
  const byte* dpMap = assignSE2dp[slice->dataDpMode];
  sDecoder* decoder = mb->decoder;
  int smb = ((decoder->coding.type == SP_SLICE) && (mb->isIntraBlock == FALSE)) || 
            ((decoder->coding.type == SI_SLICE) && (mb->mbType == SI4MB));

  int uv;
  int qp_per_uv[2];
  int qp_rem_uv[2];

  int intra = (mb->isIntraBlock == TRUE);
  int temp[4];

  int b4;
  //sPicture* picture = slice->picture;

  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  int (*InvLevelScale8x8)[8] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((decoder->coding.structure == FRAME) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];

  // read CBP if not new intra mode
  if (!IS_I16MB (mb)) {
    se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
      ? SE_CBP_INTRA : SE_CBP_INTER;
    dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
    se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
      ? slice->linfoCbpIntra : slice->linfoCbpInter;
    dataPartition->readSyntaxElement(mb, &se, dataPartition);
    mb->cbp = cbp = se.value1;

    need_transform_size_flag = (((mb->mbType >= 1 && mb->mbType <= 3)||
      (IS_DIRECT(mb) && decoder->activeSPS->direct_8x8_inference_flag) ||
      (mb->noMbPartLessThan8x8Flag))
      && mb->mbType != I8MB && mb->mbType != I4MB
      && (mb->cbp&15)
      && slice->transform8x8Mode);
    if (need_transform_size_flag) {
      //{{{  Transform size flag for INTER MBs
      se.type =  SE_HEADER;
      dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
      // read CAVLC transform_size_8x8_flag
      se.len = 1;
      readsSyntaxElement_FLC(&se, dataPartition->s);
      mb->lumaTransformSize8x8flag = (Boolean) se.value1;
      }
      //}}}

    // Delta quant only if nonzero coeffs
    if (cbp != 0) {
      readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (slice->dataDpMode) {
        if ((mb->isIntraBlock == FALSE) && slice->noDataPartitionC )
          mb->dplFlag = 1;

        if (intra && slice->noDataPartitionB) {
          mb->errorFlag = 1;
          mb->dplFlag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (mb);
        if (mb->dplFlag) {
          cbp = 0;
          mb->cbp = cbp;
          }
        }
      }
    }
  else {
    cbp = mb->cbp;
    readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);

    if (slice->dataDpMode) {
      if (slice->noDataPartitionB) {
        mb->errorFlag  = 1;
        mb->dplFlag = 1;
        }
      checkDpNeighbours (mb);
      if (mb->dplFlag)
        mb->cbp = cbp = 0;
      }

    if (!mb->dplFlag) {
      pos_scan_4x4 = pos_scan4x4[0];
      slice->readCoef4x4cavlc (mb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
      for (k = 0; k < numcoeff; ++k) {
        if (levarr[k] != 0) {
          // leave if level == 0
          pos_scan_4x4 += 2 * runarr[k];
          i0 = ((*pos_scan_4x4++) << 2);
          j0 = ((*pos_scan_4x4++) << 2);
          slice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
          }
        }

      if (mb->isLossless == FALSE)
        itrans_2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
      }
    }

  updateQp (mb, slice->qp);
  qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
  qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

  // init quant parameters for chroma
  for(i=0; i < 2; ++i) {
    qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
    qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
  }

  InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
  InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

  // luma coefficients
  if (cbp) {
    if (!mb->lumaTransformSize8x8flag) // 4x4 transform
      mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, cbp, decoder->nzCoeff[mb_nr][PLANE_Y]);
    else // 8x8 transform
      mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, cbp, decoder->nzCoeff[mb_nr][PLANE_Y]);
    }
  else
    memset(decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(byte));

  //{{{  chroma DC coeff
  if (cbp>15) {
    for (ll=0;ll<3;ll+=2) {
      uv = ll>>1;

      InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];
      // CHROMA DC YUV420
      slice->cofu[0] = slice->cofu[1] = slice->cofu[2] = slice->cofu[3] = 0;
      coef_ctr=-1;

      slice->readCoef4x4cavlc(mb, CHROMA_DC, 0, 0, levarr, runarr, &numcoeff);
      for(k = 0; k < numcoeff; ++k) {
        if (levarr[k] != 0) {
          mb->cbpStructure[0].blk |= 0xf0000 << (ll<<1) ;
          coef_ctr += runarr[k] + 1;
          slice->cofu[coef_ctr]=levarr[k];
        }
      }


      if (smb || (mb->isLossless == TRUE)) {
        // check to see if MB type is SPred or SIntra4x4
        slice->cof[PLANE_U + uv][0][0] = slice->cofu[0];
        slice->cof[PLANE_U + uv][0][4] = slice->cofu[1];
        slice->cof[PLANE_U + uv][4][0] = slice->cofu[2];
        slice->cof[PLANE_U + uv][4][4] = slice->cofu[3];
        }
      else {
        ihadamard2x2(slice->cofu, temp);
        slice->cof[PLANE_U + uv][0][0] = (((temp[0] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
        slice->cof[PLANE_U + uv][0][4] = (((temp[1] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
        slice->cof[PLANE_U + uv][4][0] = (((temp[2] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
        slice->cof[PLANE_U + uv][4][4] = (((temp[3] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
        }
      }
    }
  //}}}
  //{{{  chroma AC coeff, all zero fram start_scan
  if (cbp<=31)
    memset(decoder->nzCoeff [mb_nr ][1][0], 0, 2 * BLOCK_PIXELS * sizeof(byte));
  else {
    if(mb->isLossless == FALSE) {
      for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
        mb->isVblock = uv = (b8 > ((decoder->coding.numUvBlocks) - 1 ));
        InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];

        for (b4=0; b4 < 4; ++b4) {
          i = cofuv_blk_x[0][b8][b4];
          j = cofuv_blk_y[0][b8][b4];
          slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
          coef_ctr = 0;
          for(k = 0; k < numcoeff;++k) {
            if (levarr[k] != 0) {
              mb->cbpStructure[0].blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              coef_ctr += runarr[k] + 1;
              i0=pos_scan4x4[coef_ctr][0];
              j0=pos_scan4x4[coef_ctr][1];
              slice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((levarr[k] * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
            }
          }
        }
      }
    }
    else {
      for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
        mb->isVblock = uv = (b8 > ((decoder->coding.numUvBlocks) - 1 ));
        for (b4=0; b4 < 4; ++b4) {
          i = cofuv_blk_x[0][b8][b4];
          j = cofuv_blk_y[0][b8][b4];
          slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
          coef_ctr = 0;
          for (k = 0; k < numcoeff;++k) {
            if (levarr[k] != 0) {
              mb->cbpStructure[0].blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              coef_ctr += runarr[k] + 1;

              i0=pos_scan4x4[coef_ctr][0];
              j0=pos_scan4x4[coef_ctr][1];

              slice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = levarr[k];
              }
            }
          }
        }
      }
    }
  //}}}
  }
//}}}

//{{{
void setReadCompCoefCavlc (sMacroBlock* mb) {

  if (mb->isLossless == FALSE) {
    mb->readCompCoef4x4cavlc = readCompCoef4x4cavlc;
    mb->readCompCoef8x8cavlc = readCompCoef8x8cavlc;
    }
  else {
    mb->readCompCoef4x4cavlc = read_comp_coeff_4x4_CAVLC_ls;
    mb->readCompCoef8x8cavlc = read_comp_coeff_8x8_CAVLC_ls;
    }
  }
//}}}
//{{{
void setReadCbpCoefCavlc (sSlice* slice) {

  switch (slice->decoder->activeSPS->chromaFormatIdc) {
    case YUV444:
      if (slice->decoder->coding.sepColourPlaneFlag == 0)
        slice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CAVLC_444;
      else
        slice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CAVLC_400;
      break;

    case YUV422:
      slice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CAVLC_422;
      break;

    case YUV420:
      slice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CAVLC_420;
      break;

    case YUV400:
      slice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CAVLC_400;
      break;

    default:
      assert (1);
      slice->readCBPcoeffs = NULL;
      break;
    }
  }
//}}}
