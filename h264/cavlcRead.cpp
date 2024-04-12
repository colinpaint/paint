//{{{  includes
#include "global.h"

#include "macroblock.h"
#include "cabac.h"
#include "transform.h"
//}}}
namespace {
  //{{{
  int predictNnz (sMacroBlock* mb, int block_type, int i,int j) {

    cDecoder264* decoder = mb->decoder;

    // left block
    sPixelPos pixelPos;
    get4x4Neighbour (mb, i - 1, j, decoder->mbSize[eLuma], &pixelPos);

    int cnt = 0;
    cSlice* slice = mb->slice;
    if ((mb->isIntraBlock == true) && pixelPos.ok &&
        decoder->activePps->hasConstrainedIntraPred && (slice->dataPartitionMode == eDataPartition3)) {
      pixelPos.ok &= slice->intraBlock[pixelPos.mbIndex];
      if (!pixelPos.ok)
        ++cnt;
      }

    int pred_nnz = 0;
    if (pixelPos.ok) {
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
          cDecoder264::cDecoder264::error ("writeCoeff4x4_CAVLC: Invalid block type");
          break;
        //}}}
        }
      }

    // top block
    get4x4Neighbour (mb, i, j - 1, decoder->mbSize[eLuma], &pixelPos);

    if ((mb->isIntraBlock == true) && pixelPos.ok &&
        decoder->activePps->hasConstrainedIntraPred && (slice->dataPartitionMode == eDataPartition3)) {
      pixelPos.ok &= slice->intraBlock[pixelPos.mbIndex];
      if (!pixelPos.ok)
        ++cnt;
      }

    if (pixelPos.ok) {
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
          cDecoder264::cDecoder264::error ("writeCoeff4x4_CAVLC: Invalid block type");
          break;
          }
        //}}}
        }

    if (cnt == 2) {
      ++pred_nnz;
      pred_nnz >>= 1;
      }

    return pred_nnz;
    }
  //}}}
  //{{{
  int predictNnzChroma (sMacroBlock* mb, int i,int j) {

    sPicture* picture = mb->slice->picture;
    if (picture->chromaFormatIdc != YUV444) {
      cDecoder264* decoder = mb->decoder;
      cSlice* slice = mb->slice;
      sPixelPos pixelPos;

      int pred_nnz = 0;
      int cnt      = 0;

      //YUV420 and YUV422
      // left block
      get4x4Neighbour (mb, ((i&0x01)<<2) - 1, j, decoder->mbSize[eChroma], &pixelPos);

      if ((mb->isIntraBlock == true) && pixelPos.ok &&
          decoder->activePps->hasConstrainedIntraPred && (slice->dataPartitionMode==eDataPartition3)) {
        pixelPos.ok &= slice->intraBlock[pixelPos.mbIndex];
        if (!pixelPos.ok)
          ++cnt;
        }

      if (pixelPos.ok) {
        pred_nnz = decoder->nzCoeff [pixelPos.mbIndex ][1][pixelPos.y][2 * (i>>1) + pixelPos.x];
        ++cnt;
        }

      // top block
      get4x4Neighbour (mb, ((i&0x01)<<2), j - 1, decoder->mbSize[eChroma], &pixelPos);

      if ((mb->isIntraBlock == true) && pixelPos.ok &&
          decoder->activePps->hasConstrainedIntraPred && (slice->dataPartitionMode==eDataPartition3)) {
        pixelPos.ok &= slice->intraBlock[pixelPos.mbIndex];
        if (!pixelPos.ok)
          ++cnt;
        }

      if (pixelPos.ok) {
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
  void readCompCoef4x4 (sMacroBlock* mb, eColorPlane plane,
                                    int (*InvLevelScale4x4)[4], int qp_per, int codedBlockPattern, uint8_t** nzcoeff) {

    int i0, j0;
    int levarr[16] = {0}, runarr[16] = {0}, numcoeff;

    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;

    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];
    int start_scan = IS_I16MB(mb) ? 1 : 0;
    int64_t *cur_cbp = &mb->cbp[plane].blk;
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
        if (codedBlockPattern & (1 << b8))  {
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
                  *cur_cbp |= i64power2(j + (i >> 2));
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
  void readCompCoef4x4lossless (sMacroBlock* mb, eColorPlane plane,
                                            int (*InvLevelScale4x4)[4], int qp_per, int codedBlockPattern, uint8_t** nzcoeff) {

    int levarr[16] = {0}, runarr[16] = {0}, numcoeff;

    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;

    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    int start_scan = IS_I16MB(mb) ? 1 : 0;
    int64_t* cur_cbp = &mb->cbp[plane].blk;

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
        if (codedBlockPattern & (1 << b8)) {
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
                  *cur_cbp |= i64power2((j<<2) + i);
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
  void readCompCoef8x8 (sMacroBlock* mb, eColorPlane plane,
                                    int (*InvLevelScale8x8)[8], int qp_per, int codedBlockPattern, uint8_t** nzcoeff) {

    int levarr[16] = {0}, runarr[16] = {0}, numcoeff;
    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;

    const uint8_t (*pos_scan8x8)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN8x8 : FIELD_SCAN8x8;
    int start_scan = IS_I16MB(mb) ? 1 : 0;
    int64_t *cur_cbp = &mb->cbp[plane].blk;
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
        if (codedBlockPattern & (1<<b8)) {
        /* are there any coeff in current block at all */
          for (int j = blockY; j < blockY + 2; ++j) {
            for (int i = blockX; i < blockX + 2; ++i) {
              slice->readCoef4x4cavlc(mb, cur_context, i, j, levarr, runarr, &numcoeff);
              coef_ctr = start_scan - 1;
              for (int k = 0; k < numcoeff; ++k) {
                if (levarr[k] != 0) {
                  coef_ctr += runarr[k] + 1;
                  // do same as eCabac for deblocking: any coeff in the 8x8 marks all the 4x4s
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
  void readCompCoef8x8lossless (sMacroBlock* mb, eColorPlane plane,
                                            int (*InvLevelScale8x8)[8], int qp_per, int codedBlockPattern, uint8_t** nzcoeff) {

    int levarr[16] = {0}, runarr[16] = {0}, numcoeff;

    cSlice* slice = mb->slice;
    cDecoder264* decoder = mb->decoder;

    const uint8_t (*pos_scan8x8)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN8x8 : FIELD_SCAN8x8;
    int start_scan = IS_I16MB(mb) ? 1 : 0;
    int64_t*cur_cbp = &mb->cbp[plane].blk;

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
        if (codedBlockPattern & (1<<b8)) {
          /* are there any coeff in current block at all */
          for (int j = blockY; j < blockY+2; ++j) {
            for (int i = blockX; i < blockX+2; ++i) {
              slice->readCoef4x4cavlc (mb, cur_context, i, j, levarr, runarr, &numcoeff);
              coef_ctr = start_scan - 1;
              for (int k = 0; k < numcoeff; ++k) {
                if (levarr[k] != 0) {
                  coef_ctr += runarr[k]+1;
                  // do same as eCabac for deblocking: any coeff in the 8x8 marks all the 4x4s
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
  }

//{{{
void sMacroBlock::setReadCompCavlc() {

  if (isLossless) {
    readCompCoef4x4cavlc = readCompCoef4x4lossless;
    readCompCoef8x8cavlc = readCompCoef8x8lossless;
    }
  else {
    readCompCoef4x4cavlc = readCompCoef4x4;
    readCompCoef8x8cavlc = readCompCoef8x8;
    }
  }
//}}}

//{{{
void readCoef4x4cavlc444 (sMacroBlock* mb, int block_type, int i, int j,
                          int levarr[16], int runarr[16], int* number_coefficients) {

  static const int incVlc[] = {0, 3, 6, 12, 24, 48, 32768};    // maximum vlc = 6

  cSlice* slice = mb->slice;
  cDecoder264* decoder = mb->decoder;
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
      dptype = (mb->isIntraBlock == true) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
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
      dptype = ((mb->isIntraBlock == true)) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
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
      dptype = ((mb->isIntraBlock == true)) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
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
      dptype = (mb->isIntraBlock == true) ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_AC:
      max_coeff_num = 15;
      cac = 1;
      dptype = (mb->isIntraBlock == true) ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    default:
      cDecoder264::error ("readCoef4x4cavlc: invalid block type");
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    }

  sSyntaxElement se;
  se.type = dptype;
  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
  sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[dptype]]);
  cBitStream& s = dataPartition->bitStream;

  if (!cdc) {
    //{{{  luma or chroma AC
    if (block_type == LUMA || block_type == LUMA_INTRA16x16DC || block_type == LUMA_INTRA16x16AC || block_type == CHROMA_AC)
      nnz = (!cac) ? predictNnz (mb, LUMA, i<<2, j<<2) : predictNnzChroma(mb, i, ((j-4)<<2));
    else if (block_type == CB || block_type == CB_INTRA16x16DC || block_type == CB_INTRA16x16AC)
      nnz = predictNnz (mb, CB, i<<2, j<<2);
    else
      nnz = predictNnz (mb, CR, i<<2, j<<2);

    se.value1 = (nnz < 2) ? 0 : ((nnz < 4) ? 1 : ((nnz < 8) ? 2 : 3));
    s.readSyntaxElementNumCoeffTrailingOnes (&se, type);
    numcoeff = se.value1;
    numtrailingones = se.value2;
    if  (block_type == LUMA || block_type == LUMA_INTRA16x16DC || block_type == LUMA_INTRA16x16AC || block_type == CHROMA_AC)
      decoder->nzCoeff[mb_nr][0][j][i] = (uint8_t)numcoeff;
    else if (block_type == CB || block_type == CB_INTRA16x16DC || block_type == CB_INTRA16x16AC)
      decoder->nzCoeff[mb_nr][1][j][i] = (uint8_t)numcoeff;
    else
      decoder->nzCoeff[mb_nr][2][j][i] = (uint8_t)numcoeff;
    }
    //}}}
  else {
    //{{{  chroma DC
    s.readSyntaxElementNumCoeffTrailingOnesChromaDC (decoder, &se);
    numcoeff = se.value1;
    numtrailingones = se.value2;
    }
    //}}}
  memset (levarr, 0, max_coeff_num * sizeof(int));
  memset (runarr, 0, max_coeff_num * sizeof(int));

  numones = numtrailingones;
  *number_coefficients = numcoeff;
  if (numcoeff) {
    if (numtrailingones) {
      se.len = numtrailingones;
      s.readSyntaxElementFLC (&se);
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
        s.readSyntaxElementLevelVlc0 (&se);
      else
        s.readSyntaxElementLevelVlcN (&se, vlcnum);

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
        s.readSyntaxElementTotalZerosChromaDC (decoder, &se);
      else
        s.readSyntaxElementTotalZeros (&se);

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
        vlcnum = imin (zerosleft - 1, RUNBEFORE_NUM_M1);
        se.value1 = vlcnum;
        s.readSyntaxElementRun (&se);
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
void readCoef4x4cavlcNormal (sMacroBlock* mb, int block_type, int i, int j,
                             int levarr[16], int runarr[16], int* number_coefficients) {

  cSlice* slice = mb->slice;
  cDecoder264* decoder = mb->decoder;
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
      dptype = (mb->isIntraBlock == true) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
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
      dptype = (mb->isIntraBlock == true) ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_AC:
      max_coeff_num = 15;
      cac = 1;
      dptype = (mb->isIntraBlock == true) ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER;
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    default:
      cDecoder264::error ("readCoef4x4cavlc: invalid block type");
      decoder->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    }

  sSyntaxElement se;
  se.type = dptype;
  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
  sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[dptype]]);
  cBitStream& s = dataPartition->bitStream;

  if (!cdc) {
    //{{{  luma or chroma AC
    nnz = (!cac) ? predictNnz (mb, LUMA, i<<2, j<<2) : predictNnzChroma (mb, i, ((j-4)<<2));
    se.value1 = (nnz < 2) ? 0 : ((nnz < 4) ? 1 : ((nnz < 8) ? 2 : 3));
    s.readSyntaxElementNumCoeffTrailingOnes(&se, type);
    numcoeff = se.value1;
    numtrailingones = se.value2;
    decoder->nzCoeff[mb_nr][0][j][i] = (uint8_t) numcoeff;
    }
    //}}}
  else {
    //{{{  chroma DC
    s.readSyntaxElementNumCoeffTrailingOnesChromaDC (decoder, &se);
    numcoeff = se.value1;
    numtrailingones = se.value2;
    }
    //}}}
  memset (levarr, 0, max_coeff_num * sizeof(int));
  memset (runarr, 0, max_coeff_num * sizeof(int));

  int numones = numtrailingones;
  *number_coefficients = numcoeff;
  if (numcoeff) {
    if (numtrailingones) {
      se.len = numtrailingones;
      s.readSyntaxElementFLC (&se);
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
        s.readSyntaxElementLevelVlc0 (&se);
      else
        s.readSyntaxElementLevelVlcN (&se, vlcnum);

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
        s.readSyntaxElementTotalZerosChromaDC (decoder, &se);
      else
        s.readSyntaxElementTotalZeros (&se);

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
        s.readSyntaxElementRun (&se);
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
