//{{{  includes
#include "global.h"
#include "elements.h"

#include "macroblock.h"
#include "vlc.h"
#include "transform.h"
#include "mbAccess.h"
//}}}

//{{{
static int predict_nnz (sMacroblock* curMb, int block_type, int i,int j)
{
  sVidParam* vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;

  sPixelPos pix;

  int pred_nnz = 0;
  int cnt      = 0;

  // left block
  get4x4Neighbour(curMb, i - 1, j, vidParam->mb_size[IS_LUMA], &pix);

  if ((curMb->is_intra_block == TRUE) && pix.available && vidParam->activePPS->constrained_intra_pred_flag && (curSlice->dp_mode == PAR_DP_3))
  {
    pix.available &= curSlice->intraBlock[pix.mb_addr];
    if (!pix.available)
      ++cnt;
  }

  if (pix.available)
  {
    switch (block_type)
    {
    case LUMA:
      pred_nnz = vidParam->nzCoeff [pix.mb_addr ][0][pix.y][pix.x];
      ++cnt;
      break;
    case CB:
      pred_nnz = vidParam->nzCoeff [pix.mb_addr ][1][pix.y][pix.x];
      ++cnt;
      break;
    case CR:
      pred_nnz = vidParam->nzCoeff [pix.mb_addr ][2][pix.y][pix.x];
      ++cnt;
      break;
    default:
      error("writeCoeff4x4_CAVLC: Invalid block type", 600);
      break;
    }
  }

  // top block
  get4x4Neighbour(curMb, i, j - 1, vidParam->mb_size[IS_LUMA], &pix);

  if ((curMb->is_intra_block == TRUE) && pix.available && vidParam->activePPS->constrained_intra_pred_flag && (curSlice->dp_mode==PAR_DP_3))
  {
    pix.available &= curSlice->intraBlock[pix.mb_addr];
    if (!pix.available)
      ++cnt;
  }

  if (pix.available)
  {
    switch (block_type)
    {
    case LUMA:
      pred_nnz += vidParam->nzCoeff [pix.mb_addr ][0][pix.y][pix.x];
      ++cnt;
      break;
    case CB:
      pred_nnz += vidParam->nzCoeff [pix.mb_addr ][1][pix.y][pix.x];
      ++cnt;
      break;
    case CR:
      pred_nnz += vidParam->nzCoeff [pix.mb_addr ][2][pix.y][pix.x];
      ++cnt;
      break;
    default:
      error("writeCoeff4x4_CAVLC: Invalid block type", 600);
      break;
    }
  }

  if (cnt==2)
  {
    ++pred_nnz;
    pred_nnz >>= 1;
  }

  return pred_nnz;
}
//}}}
//{{{
static int predict_nnz_chroma (sMacroblock* curMb, int i,int j)
{
  sPicture* picture = curMb->slice->picture;

  if (picture->chromaFormatIdc != YUV444)
  {
    sVidParam* vidParam = curMb->vidParam;
    sSlice* curSlice = curMb->slice;
    sPixelPos pix;
    int pred_nnz = 0;
    int cnt      = 0;

    //YUV420 and YUV422
    // left block
    get4x4Neighbour(curMb, ((i&0x01)<<2) - 1, j, vidParam->mb_size[IS_CHROMA], &pix);

    if ((curMb->is_intra_block == TRUE) && pix.available && vidParam->activePPS->constrained_intra_pred_flag && (curSlice->dp_mode==PAR_DP_3))
    {
      pix.available &= curSlice->intraBlock[pix.mb_addr];
      if (!pix.available)
        ++cnt;
    }

    if (pix.available)
    {
      pred_nnz = vidParam->nzCoeff [pix.mb_addr ][1][pix.y][2 * (i>>1) + pix.x];
      ++cnt;
    }

    // top block
    get4x4Neighbour(curMb, ((i&0x01)<<2), j - 1, vidParam->mb_size[IS_CHROMA], &pix);

    if ((curMb->is_intra_block == TRUE) && pix.available && vidParam->activePPS->constrained_intra_pred_flag && (curSlice->dp_mode==PAR_DP_3))
    {
      pix.available &= curSlice->intraBlock[pix.mb_addr];
      if (!pix.available)
        ++cnt;
    }

    if (pix.available)
    {
      pred_nnz += vidParam->nzCoeff [pix.mb_addr ][1][pix.y][2 * (i>>1) + pix.x];
      ++cnt;
    }

    if (cnt==2)
    {
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
void read_coeff_4x4_CAVLC (sMacroblock* curMb, int block_type,
                           int i, int j, int levarr[16], int runarr[16], int *number_coefficients) {

  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;
  int mb_nr = curMb->mbAddrX;
  sSyntaxElement currSE;
  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  sBitstream* curStream;

  int k, code, vlcnum;
  int numcoeff = 0, numtrailingones;
  int level_two_or_higher;
  int numones, totzeros, abslevel, cdc=0, cac=0;
  int zerosleft, ntr, dptype = 0;
  int max_coeff_num = 0, nnz;
  char type[15];
  static const int incVlc[] = {0, 3, 6, 12, 24, 48, 32768};    // maximum vlc = 6

  switch (block_type) {
    //{{{
    case LUMA:
      max_coeff_num = 16;
      dptype = (curMb->is_intra_block == TRUE) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case LUMA_INTRA16x16DC:
      max_coeff_num = 16;
      dptype = SE_LUM_DC_INTRA;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case LUMA_INTRA16x16AC:
      max_coeff_num = 15;
      dptype = SE_LUM_AC_INTRA;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_DC:
      max_coeff_num = vidParam->num_cdc_coeff;
      cdc = 1;
      dptype = (curMb->is_intra_block == TRUE) ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_AC:
      max_coeff_num = 15;
      cac = 1;
      dptype = (curMb->is_intra_block == TRUE) ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    default:
      error ("read_coeff_4x4_CAVLC: invalid block type", 600);
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    }

  currSE.type = dptype;
  dP = &(curSlice->partArr[partMap[dptype]]);
  curStream = dP->bitstream;

  if (!cdc) {
    //{{{  luma or chroma AC
    nnz = (!cac) ? predict_nnz(curMb, LUMA, i<<2, j<<2) : predict_nnz_chroma(curMb, i, ((j-4)<<2));
    currSE.value1 = (nnz < 2) ? 0 : ((nnz < 4) ? 1 : ((nnz < 8) ? 2 : 3));
    readsSyntaxElement_NumCoeffTrailingOnes(&currSE, curStream, type);
    numcoeff        =  currSE.value1;
    numtrailingones =  currSE.value2;
    vidParam->nzCoeff[mb_nr][0][j][i] = (byte) numcoeff;
    }
    //}}}
  else {
    //{{{  chroma DC
    readsSyntaxElement_NumCoeffTrailingOnesChromaDC(vidParam, &currSE, curStream);
    numcoeff        =  currSE.value1;
    numtrailingones =  currSE.value2;
    }
    //}}}
  memset (levarr, 0, max_coeff_num * sizeof(int));
  memset (runarr, 0, max_coeff_num * sizeof(int));

  numones = numtrailingones;
  *number_coefficients = numcoeff;
  if (numcoeff) {
    if (numtrailingones) {
      currSE.len = numtrailingones;
      readsSyntaxElement_FLC (&currSE, curStream);
      code = currSE.inf;
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
        readsSyntaxElement_Level_VLC0(&currSE, curStream);
      else
        readsSyntaxElement_Level_VLCN(&currSE, vlcnum, curStream);

      if (level_two_or_higher) {
        currSE.inf += (currSE.inf > 0) ? 1 : -1;
        level_two_or_higher = 0;
        }

      levarr[k] = currSE.inf;
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
      currSE.value1 = vlcnum;

      if (cdc)
        readsSyntaxElement_TotalZerosChromaDC(vidParam, &currSE, curStream);
      else
        readsSyntaxElement_TotalZeros(&currSE, curStream);

      totzeros = currSE.value1;
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

        currSE.value1 = vlcnum;
        readsSyntaxElement_Run(&currSE, curStream);
        runarr[i] = currSE.value1;

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
void read_coeff_4x4_CAVLC_444 (sMacroblock* curMb, int block_type,
                               int i, int j, int levarr[16], int runarr[16], int *number_coefficients) {

  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;
  int mb_nr = curMb->mbAddrX;
  sSyntaxElement currSE;
  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  sBitstream* curStream;

  int k, code, vlcnum;
  int numcoeff = 0, numtrailingones;
  int level_two_or_higher;
  int numones, totzeros, abslevel, cdc=0, cac=0;
  int zerosleft, ntr, dptype = 0;
  int max_coeff_num = 0, nnz;
  char type[15];
  static const int incVlc[] = {0, 3, 6, 12, 24, 48, 32768};    // maximum vlc = 6

  switch (block_type) {
    //{{{
    case LUMA:
      max_coeff_num = 16;
      dptype = (curMb->is_intra_block == TRUE) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case LUMA_INTRA16x16DC:
      max_coeff_num = 16;
      dptype = SE_LUM_DC_INTRA;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case LUMA_INTRA16x16AC:
      max_coeff_num = 15;
      dptype = SE_LUM_AC_INTRA;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CB:
      max_coeff_num = 16;
      dptype = ((curMb->is_intra_block == TRUE)) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
      vidParam->nzCoeff[mb_nr][1][j][i] = 0;
      break;
    //}}}
    //{{{
    case CB_INTRA16x16DC:
      max_coeff_num = 16;
      dptype = SE_LUM_DC_INTRA;
      vidParam->nzCoeff[mb_nr][1][j][i] = 0;
      break;
    //}}}
    //{{{
    case CB_INTRA16x16AC:
      max_coeff_num = 15;
      dptype = SE_LUM_AC_INTRA;
      vidParam->nzCoeff[mb_nr][1][j][i] = 0;
      break;
    //}}}
    //{{{
    case CR:
      max_coeff_num = 16;
      dptype = ((curMb->is_intra_block == TRUE)) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
      vidParam->nzCoeff[mb_nr][2][j][i] = 0;
      break;
    //}}}
    //{{{
    case CR_INTRA16x16DC:
      max_coeff_num = 16;
      dptype = SE_LUM_DC_INTRA;
      vidParam->nzCoeff[mb_nr][2][j][i] = 0;
      break;
    //}}}
    //{{{
    case CR_INTRA16x16AC:
      max_coeff_num = 15;
      dptype = SE_LUM_AC_INTRA;
      vidParam->nzCoeff[mb_nr][2][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_DC:
      max_coeff_num = vidParam->num_cdc_coeff;
      cdc = 1;
      dptype = (curMb->is_intra_block == TRUE) ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    case CHROMA_AC:
      max_coeff_num = 15;
      cac = 1;
      dptype = (curMb->is_intra_block == TRUE) ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER;
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    //{{{
    default:
      error ("read_coeff_4x4_CAVLC: invalid block type", 600);
      vidParam->nzCoeff[mb_nr][0][j][i] = 0;
      break;
    //}}}
    }

  currSE.type = dptype;
  dP = &(curSlice->partArr[partMap[dptype]]);
  curStream = dP->bitstream;

  if (!cdc) {
    //{{{  luma or chroma AC
    if(block_type==LUMA || block_type==LUMA_INTRA16x16DC || block_type==LUMA_INTRA16x16AC ||block_type==CHROMA_AC)
      nnz = (!cac) ? predict_nnz(curMb, LUMA, i<<2, j<<2) : predict_nnz_chroma(curMb, i, ((j-4)<<2));
    else if (block_type==CB || block_type==CB_INTRA16x16DC || block_type==CB_INTRA16x16AC)
      nnz = predict_nnz(curMb, CB, i<<2, j<<2);
    else
      nnz = predict_nnz(curMb, CR, i<<2, j<<2);

    currSE.value1 = (nnz < 2) ? 0 : ((nnz < 4) ? 1 : ((nnz < 8) ? 2 : 3));
    readsSyntaxElement_NumCoeffTrailingOnes(&currSE, curStream, type);
    numcoeff        =  currSE.value1;
    numtrailingones =  currSE.value2;
    if  (block_type==LUMA || block_type==LUMA_INTRA16x16DC || block_type==LUMA_INTRA16x16AC ||block_type==CHROMA_AC)
      vidParam->nzCoeff[mb_nr][0][j][i] = (byte) numcoeff;
    else if (block_type==CB || block_type==CB_INTRA16x16DC || block_type==CB_INTRA16x16AC)
      vidParam->nzCoeff[mb_nr][1][j][i] = (byte) numcoeff;
    else
      vidParam->nzCoeff[mb_nr][2][j][i] = (byte) numcoeff;
    }
    //}}}
  else {
    //{{{  chroma DC
    readsSyntaxElement_NumCoeffTrailingOnesChromaDC(vidParam, &currSE, curStream);
    numcoeff        =  currSE.value1;
    numtrailingones =  currSE.value2;
    }
    //}}}
  memset (levarr, 0, max_coeff_num * sizeof(int));
  memset (runarr, 0, max_coeff_num * sizeof(int));

  numones = numtrailingones;
  *number_coefficients = numcoeff;
  if (numcoeff) {
    if (numtrailingones) {
      currSE.len = numtrailingones;
      readsSyntaxElement_FLC (&currSE, curStream);
      code = currSE.inf;
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
        readsSyntaxElement_Level_VLC0(&currSE, curStream);
      else
        readsSyntaxElement_Level_VLCN(&currSE, vlcnum, curStream);

      if (level_two_or_higher) {
        currSE.inf += (currSE.inf > 0) ? 1 : -1;
        level_two_or_higher = 0;
        }

      levarr[k] = currSE.inf;
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
      currSE.value1 = vlcnum;

      if (cdc)
        readsSyntaxElement_TotalZerosChromaDC(vidParam, &currSE, curStream);
      else
        readsSyntaxElement_TotalZeros(&currSE, curStream);

      totzeros = currSE.value1;
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
        currSE.value1 = vlcnum;
        readsSyntaxElement_Run(&currSE, curStream);
        runarr[i] = currSE.value1;
        zerosleft -= runarr[i];
        i --;
        } while (zerosleft != 0 && i != 0);
      }
    runarr[i] = zerosleft;
    } 
  }
//}}}

//{{{
static void read_comp_coeff_4x4_CAVLC (sMacroblock* curMb, eColorPlane pl, int (*InvLevelScale4x4)[4], int qp_per, int cbp, byte** nzcoeff)
{
  int block_y, block_x, b8;
  int i, j, k;
  int i0, j0;
  int levarr[16] = {0}, runarr[16] = {0}, numcoeff;
  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;
  const byte (*pos_scan4x4)[2] = ((vidParam->structure == FRAME) && (!curMb->mb_field)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];
  int start_scan = IS_I16MB(curMb) ? 1 : 0;
  int64 *cur_cbp = &curMb->s_cbp[pl].blk;
  int cur_context;
  int block_y4, block_x4;

  if (IS_I16MB(curMb))
  {
    if (pl == PLANE_Y)
      cur_context = LUMA_INTRA16x16AC;
    else if (pl == PLANE_U)
      cur_context = CB_INTRA16x16AC;
    else
      cur_context = CR_INTRA16x16AC;
  }
  else
  {
    if (pl == PLANE_Y)
      cur_context = LUMA;
    else if (pl == PLANE_U)
      cur_context = CB;
    else
      cur_context = CR;
  }


  for (block_y = 0; block_y < 4; block_y += 2) /* all modes */
  {
    block_y4 = block_y << 2;
    for (block_x = 0; block_x < 4; block_x += 2)
    {
      block_x4 = block_x << 2;
      b8 = (block_y + (block_x >> 1));

      if (cbp & (1 << b8))  // test if the block contains any coefficients
      {
        for (j = block_y4; j < block_y4 + 8; j += BLOCK_SIZE)
        {
          for (i = block_x4; i < block_x4 + 8; i += BLOCK_SIZE)
          {
            curSlice->read_coeff_4x4_CAVLC(curMb, cur_context, i >> 2, j >> 2, levarr, runarr, &numcoeff);
            pos_scan_4x4 = pos_scan4x4[start_scan];

            for (k = 0; k < numcoeff; ++k)
            {
              if (levarr[k] != 0)
              {
                pos_scan_4x4 += (runarr[k] << 1);

                i0 = *pos_scan_4x4++;
                j0 = *pos_scan_4x4++;

                // inverse quant for 4x4 transform only
                *cur_cbp |= i64_power2(j + (i >> 2));

                curSlice->cof[pl][j + j0][i + i0]= rshift_rnd_sf((levarr[k] * InvLevelScale4x4[j0][i0])<<qp_per, 4);
                //curSlice->fcf[pl][j + j0][i + i0]= levarr[k];
              }
            }
          }
        }
      }
      else
      {
        nzcoeff[block_y    ][block_x    ] = 0;
        nzcoeff[block_y    ][block_x + 1] = 0;
        nzcoeff[block_y + 1][block_x    ] = 0;
        nzcoeff[block_y + 1][block_x + 1] = 0;
      }
    }
  }
}
//}}}
//{{{
static void read_comp_coeff_4x4_CAVLC_ls (sMacroblock* curMb, eColorPlane pl, int (*InvLevelScale4x4)[4], int qp_per, int cbp, byte** nzcoeff)
{
  int block_y, block_x, b8;
  int i, j, k;
  int i0, j0;
  int levarr[16] = {0}, runarr[16] = {0}, numcoeff;
  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;
  const byte (*pos_scan4x4)[2] = ((vidParam->structure == FRAME) && (!curMb->mb_field)) ? SNGL_SCAN : FIELD_SCAN;
  int start_scan = IS_I16MB(curMb) ? 1 : 0;
  int64 *cur_cbp = &curMb->s_cbp[pl].blk;
  int coef_ctr, cur_context;

  if (IS_I16MB(curMb))
  {
    if (pl == PLANE_Y)
      cur_context = LUMA_INTRA16x16AC;
    else if (pl == PLANE_U)
      cur_context = CB_INTRA16x16AC;
    else
      cur_context = CR_INTRA16x16AC;
  }
  else
  {
    if (pl == PLANE_Y)
      cur_context = LUMA;
    else if (pl == PLANE_U)
      cur_context = CB;
    else
      cur_context = CR;
  }

  for (block_y=0; block_y < 4; block_y += 2) /* all modes */
  {
    for (block_x=0; block_x < 4; block_x += 2)
    {
      b8 = 2*(block_y>>1) + (block_x>>1);

      if (cbp & (1<<b8))  /* are there any coeff in current block at all */
      {
        for (j=block_y; j < block_y+2; ++j)
        {
          for (i=block_x; i < block_x+2; ++i)
          {
            curSlice->read_coeff_4x4_CAVLC(curMb, cur_context, i, j, levarr, runarr, &numcoeff);

            coef_ctr = start_scan - 1;

            for (k = 0; k < numcoeff; ++k)
            {
              if (levarr[k] != 0)
              {
                coef_ctr += runarr[k]+1;

                i0=pos_scan4x4[coef_ctr][0];
                j0=pos_scan4x4[coef_ctr][1];

                *cur_cbp |= i64_power2((j<<2) + i);
                curSlice->cof[pl][(j<<2) + j0][(i<<2) + i0]= levarr[k];
                //curSlice->fcf[pl][(j<<2) + j0][(i<<2) + i0]= levarr[k];
              }
            }
          }
        }
      }
      else
      {
        nzcoeff[block_y    ][block_x    ] = 0;
        nzcoeff[block_y    ][block_x + 1] = 0;
        nzcoeff[block_y + 1][block_x    ] = 0;
        nzcoeff[block_y + 1][block_x + 1] = 0;
      }
    }
  }
}
//}}}
//{{{
static void read_comp_coeff_8x8_CAVLC (sMacroblock* curMb, eColorPlane pl, int (*InvLevelScale8x8)[8], int qp_per, int cbp, byte** nzcoeff)
{
  int block_y, block_x, b4, b8;
  int block_y4, block_x4;
  int i, j, k;
  int i0, j0;
  int levarr[16] = {0}, runarr[16] = {0}, numcoeff;
  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;
  const byte (*pos_scan8x8)[2] = ((vidParam->structure == FRAME) && (!curMb->mb_field)) ? SNGL_SCAN8x8 : FIELD_SCAN8x8;
  int start_scan = IS_I16MB(curMb) ? 1 : 0;
  int64 *cur_cbp = &curMb->s_cbp[pl].blk;
  int coef_ctr, cur_context;

  if (IS_I16MB(curMb))
  {
    if (pl == PLANE_Y)
      cur_context = LUMA_INTRA16x16AC;
    else if (pl == PLANE_U)
      cur_context = CB_INTRA16x16AC;
    else
      cur_context = CR_INTRA16x16AC;
  }
  else
  {
    if (pl == PLANE_Y)
      cur_context = LUMA;
    else if (pl == PLANE_U)
      cur_context = CB;
    else
      cur_context = CR;
  }

  for (block_y = 0; block_y < 4; block_y += 2) /* all modes */
  {
    block_y4 = block_y << 2;

    for (block_x = 0; block_x < 4; block_x += 2)
    {
      block_x4 = block_x << 2;
      b8 = block_y + (block_x>>1);

      if (cbp & (1<<b8))  /* are there any coeff in current block at all */
      {
        for (j = block_y; j < block_y + 2; ++j)
        {
          for (i = block_x; i < block_x + 2; ++i)
          {
            curSlice->read_coeff_4x4_CAVLC(curMb, cur_context, i, j, levarr, runarr, &numcoeff);

            coef_ctr = start_scan - 1;

            for (k = 0; k < numcoeff; ++k)
            {
              if (levarr[k] != 0)
              {
                coef_ctr += runarr[k] + 1;

                // do same as CABAC for deblocking: any coeff in the 8x8 marks all the 4x4s
                //as containing coefficients
                *cur_cbp |= 51 << (block_y4 + block_x);

                b4 = (coef_ctr << 2) + 2*(j - block_y) + (i - block_x);

                i0 = pos_scan8x8[b4][0];
                j0 = pos_scan8x8[b4][1];

                curSlice->mb_rres[pl][block_y4 +j0][block_x4 +i0] = rshift_rnd_sf((levarr[k] * InvLevelScale8x8[j0][i0])<<qp_per, 6); // dequantization
              }
            }//else (!curMb->luma_transform_size_8x8_flag)
          }
        }
      }
      else
      {
        nzcoeff[block_y    ][block_x    ] = 0;
        nzcoeff[block_y    ][block_x + 1] = 0;
        nzcoeff[block_y + 1][block_x    ] = 0;
        nzcoeff[block_y + 1][block_x + 1] = 0;
      }
    }
  }
}
//}}}
//{{{
static void read_comp_coeff_8x8_CAVLC_ls (sMacroblock* curMb, eColorPlane pl, int (*InvLevelScale8x8)[8], int qp_per, int cbp, byte** nzcoeff)
{
  int block_y, block_x, b4, b8;
  int i, j, k;
  int levarr[16] = {0}, runarr[16] = {0}, numcoeff;
  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;
  const byte (*pos_scan8x8)[2] = ((vidParam->structure == FRAME) && (!curMb->mb_field)) ? SNGL_SCAN8x8 : FIELD_SCAN8x8;
  int start_scan = IS_I16MB(curMb) ? 1 : 0;
  int64 *cur_cbp = &curMb->s_cbp[pl].blk;
  int coef_ctr, cur_context;

  if (IS_I16MB(curMb))
  {
    if (pl == PLANE_Y)
      cur_context = LUMA_INTRA16x16AC;
    else if (pl == PLANE_U)
      cur_context = CB_INTRA16x16AC;
    else
      cur_context = CR_INTRA16x16AC;
  }
  else
  {
    if (pl == PLANE_Y)
      cur_context = LUMA;
    else if (pl == PLANE_U)
      cur_context = CB;
    else
      cur_context = CR;
  }

  for (block_y=0; block_y < 4; block_y += 2) /* all modes */
  {
    for (block_x=0; block_x < 4; block_x += 2)
    {
      b8 = 2*(block_y>>1) + (block_x>>1);

      if (cbp & (1<<b8))  /* are there any coeff in current block at all */
      {
        int iz, jz;

        for (j=block_y; j < block_y+2; ++j)
        {
          for (i=block_x; i < block_x+2; ++i)
          {

            curSlice->read_coeff_4x4_CAVLC(curMb, cur_context, i, j, levarr, runarr, &numcoeff);

            coef_ctr = start_scan - 1;

            for (k = 0; k < numcoeff; ++k)
            {
              if (levarr[k] != 0)
              {
                coef_ctr += runarr[k]+1;

                // do same as CABAC for deblocking: any coeff in the 8x8 marks all the 4x4s
                //as containing coefficients
                *cur_cbp  |= 51 << ((block_y<<2) + block_x);

                b4 = 2*(j-block_y)+(i-block_x);

                iz=pos_scan8x8[coef_ctr*4+b4][0];
                jz=pos_scan8x8[coef_ctr*4+b4][1];

                curSlice->mb_rres[pl][block_y*4 +jz][block_x*4 +iz] = levarr[k];
              }
            }
          }
        }
      }
      else
      {
        nzcoeff[block_y    ][block_x    ] = 0;
        nzcoeff[block_y    ][block_x + 1] = 0;
        nzcoeff[block_y + 1][block_x    ] = 0;
        nzcoeff[block_y + 1][block_x + 1] = 0;
      }
    }
  }
}
//}}}

//{{{
static void read_CBP_and_coeffs_from_NAL_CAVLC_400 (sMacroblock* curMb)
{
  int k;
  int mb_nr = curMb->mbAddrX;
  int cbp;
  sSyntaxElement currSE;
  sDataPartition *dP = NULL;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  int i0, j0;

  int levarr[16], runarr[16], numcoeff;

  int qp_per, qp_rem;
  sVidParam* vidParam = curMb->vidParam;

  int intra = (curMb->is_intra_block == TRUE);

  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  int (*InvLevelScale8x8)[8] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((vidParam->structure == FRAME) && (!curMb->mb_field)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];


  // read CBP if not new intra mode
  if (!IS_I16MB (curMb))
  {
    //=====   C B P   =====
    //---------------------
    currSE.type = (curMb->mb_type == I4MB || curMb->mb_type == SI4MB || curMb->mb_type == I8MB)
      ? SE_CBP_INTRA
      : SE_CBP_INTER;

    dP = &(curSlice->partArr[partMap[currSE.type]]);

    currSE.mapping = (curMb->mb_type == I4MB || curMb->mb_type == SI4MB || curMb->mb_type == I8MB)
      ? curSlice->linfo_cbp_intra
      : curSlice->linfo_cbp_inter;

    dP->readsSyntaxElement(curMb, &currSE, dP);
    curMb->cbp = cbp = currSE.value1;


    //============= Transform size flag for INTER MBs =============
    //-------------------------------------------------------------
    need_transform_size_flag = (((curMb->mb_type >= 1 && curMb->mb_type <= 3)||
      (IS_DIRECT(curMb) && vidParam->activeSPS->direct_8x8_inference_flag) ||
      (curMb->NoMbPartLessThan8x8Flag))
      && curMb->mb_type != I8MB && curMb->mb_type != I4MB
      && (curMb->cbp&15)
      && curSlice->transform8x8Mode);

    if (need_transform_size_flag)
    {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partArr[partMap[SE_HEADER]]);

      // read CAVLC transform_size_8x8_flag
      currSE.len = 1;
      readsSyntaxElement_FLC(&currSE, dP->bitstream);

      curMb->luma_transform_size_8x8_flag = (Boolean) currSE.value1;
    }

    //=====   DQUANT   =====
    //----------------------
    // Delta quant only if nonzero coeffs
    if (cbp !=0)
    {
      readDeltaQuant(&currSE, dP, curMb, partMap, ((curMb->is_intra_block == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (curSlice->dp_mode)
      {
        if ((curMb->is_intra_block == FALSE) && curSlice->dpC_NotPresent )
          curMb->dpl_flag = 1;

        if( intra && curSlice->dpB_NotPresent )
        {
          curMb->eiFlag = 1;
          curMb->dpl_flag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (curMb);
        if (curMb->dpl_flag)
        {
          cbp = 0;
          curMb->cbp = cbp;
        }
      }
    }
  }
  else  // read DC coeffs for new intra modes
  {
    cbp = curMb->cbp;

    readDeltaQuant(&currSE, dP, curMb, partMap, SE_DELTA_QUANT_INTRA);

    if (curSlice->dp_mode)
    {
      if (curSlice->dpB_NotPresent)
      {
        curMb->eiFlag  = 1;
        curMb->dpl_flag = 1;
      }
      checkDpNeighbours (curMb);
      if (curMb->dpl_flag)
      {
        curMb->cbp = cbp = 0;
      }
    }

    if (!curMb->dpl_flag)
    {
      pos_scan_4x4 = pos_scan4x4[0];

      curSlice->read_coeff_4x4_CAVLC(curMb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);

      for(k = 0; k < numcoeff; ++k)
      {
        if (levarr[k] != 0)                     // leave if level == 0
        {
          pos_scan_4x4 += 2 * runarr[k];

          i0 = ((*pos_scan_4x4++) << 2);
          j0 = ((*pos_scan_4x4++) << 2);

          curSlice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
          //curSlice->fcf[0][j0][i0] = levarr[k];// add new intra DC coeff
        }
      }


      if(curMb->is_lossless == FALSE)
        itrans_2(curMb, (eColorPlane) curSlice->colour_plane_id);// transform new intra DC
    }
  }

  updateQp(curMb, curSlice->qp);

  qp_per = vidParam->qpPerMatrix[ curMb->qp_scaled[PLANE_Y] ];
  qp_rem = vidParam->qpRemMatrix[ curMb->qp_scaled[PLANE_Y] ];

  InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[curSlice->colour_plane_id][qp_rem] : curSlice->InvLevelScale4x4_Inter[curSlice->colour_plane_id][qp_rem];
  InvLevelScale8x8 = intra? curSlice->InvLevelScale8x8_Intra[curSlice->colour_plane_id][qp_rem] : curSlice->InvLevelScale8x8_Inter[curSlice->colour_plane_id][qp_rem];

  // luma coefficients
  if (cbp)
  {
    if (!curMb->luma_transform_size_8x8_flag) // 4x4 transform
    {
      curMb->read_comp_coeff_4x4_CAVLC (curMb, PLANE_Y, InvLevelScale4x4, qp_per, cbp, vidParam->nzCoeff[mb_nr][PLANE_Y]);
    }
    else // 8x8 transform
    {
      curMb->read_comp_coeff_8x8_CAVLC (curMb, PLANE_Y, InvLevelScale8x8, qp_per, cbp, vidParam->nzCoeff[mb_nr][PLANE_Y]);
    }
  }
  else
  {
    memset(vidParam->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(byte));
  }
}
//}}}
//{{{
static void read_CBP_and_coeffs_from_NAL_CAVLC_422 (sMacroblock* curMb)
{
  int i,j,k;
  int mb_nr = curMb->mbAddrX;
  int cbp;
  sSyntaxElement currSE;
  sDataPartition *dP = NULL;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  int coef_ctr, i0, j0, b8;
  int ll;
  int levarr[16], runarr[16], numcoeff;

  int qp_per, qp_rem;
  sVidParam* vidParam = curMb->vidParam;

  int uv;
  int qp_per_uv[2];
  int qp_rem_uv[2];

  int intra = (curMb->is_intra_block == TRUE);

  int b4;
  //sPicture* picture = curSlice->picture;
  int m6[4];

  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  int (*InvLevelScale8x8)[8] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((vidParam->structure == FRAME) && (!curMb->mb_field)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];


  // read CBP if not new intra mode
  if (!IS_I16MB (curMb))
  {
    //=====   C B P   =====
    //---------------------
    currSE.type = (curMb->mb_type == I4MB || curMb->mb_type == SI4MB || curMb->mb_type == I8MB)
      ? SE_CBP_INTRA
      : SE_CBP_INTER;

    dP = &(curSlice->partArr[partMap[currSE.type]]);

    currSE.mapping = (curMb->mb_type == I4MB || curMb->mb_type == SI4MB || curMb->mb_type == I8MB)
      ? curSlice->linfo_cbp_intra
      : curSlice->linfo_cbp_inter;

    dP->readsSyntaxElement(curMb, &currSE, dP);
    curMb->cbp = cbp = currSE.value1;


    //============= Transform size flag for INTER MBs =============
    //-------------------------------------------------------------
    need_transform_size_flag = (((curMb->mb_type >= 1 && curMb->mb_type <= 3)||
      (IS_DIRECT(curMb) && vidParam->activeSPS->direct_8x8_inference_flag) ||
      (curMb->NoMbPartLessThan8x8Flag))
      && curMb->mb_type != I8MB && curMb->mb_type != I4MB
      && (curMb->cbp&15)
      && curSlice->transform8x8Mode);

    if (need_transform_size_flag)
    {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partArr[partMap[SE_HEADER]]);

      // read CAVLC transform_size_8x8_flag
      currSE.len = 1;
      readsSyntaxElement_FLC(&currSE, dP->bitstream);

      curMb->luma_transform_size_8x8_flag = (Boolean) currSE.value1;
    }

    //=====   DQUANT   =====
    //----------------------
    // Delta quant only if nonzero coeffs
    if (cbp !=0)
    {
      readDeltaQuant(&currSE, dP, curMb, partMap, ((curMb->is_intra_block == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (curSlice->dp_mode)
      {
        if ((curMb->is_intra_block == FALSE) && curSlice->dpC_NotPresent )
          curMb->dpl_flag = 1;

        if( intra && curSlice->dpB_NotPresent )
        {
          curMb->eiFlag = 1;
          curMb->dpl_flag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (curMb);
        if (curMb->dpl_flag)
        {
          cbp = 0;
          curMb->cbp = cbp;
        }
      }
    }
  }
  else  // read DC coeffs for new intra modes
  {
    cbp = curMb->cbp;

    readDeltaQuant(&currSE, dP, curMb, partMap, SE_DELTA_QUANT_INTRA);

    if (curSlice->dp_mode)
    {
      if (curSlice->dpB_NotPresent)
      {
        curMb->eiFlag  = 1;
        curMb->dpl_flag = 1;
      }
      checkDpNeighbours (curMb);
      if (curMb->dpl_flag)
      {
        curMb->cbp = cbp = 0;
      }
    }

    if (!curMb->dpl_flag)
    {
      pos_scan_4x4 = pos_scan4x4[0];

      curSlice->read_coeff_4x4_CAVLC(curMb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);

      for(k = 0; k < numcoeff; ++k)
      {
        if (levarr[k] != 0)                     // leave if level == 0
        {
          pos_scan_4x4 += 2 * runarr[k];

          i0 = ((*pos_scan_4x4++) << 2);
          j0 = ((*pos_scan_4x4++) << 2);

          curSlice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
          //curSlice->fcf[0][j0][i0] = levarr[k];// add new intra DC coeff
        }
      }


      if(curMb->is_lossless == FALSE)
        itrans_2(curMb, (eColorPlane) curSlice->colour_plane_id);// transform new intra DC
    }
  }

  updateQp(curMb, curSlice->qp);

  qp_per = vidParam->qpPerMatrix[ curMb->qp_scaled[curSlice->colour_plane_id] ];
  qp_rem = vidParam->qpRemMatrix[ curMb->qp_scaled[curSlice->colour_plane_id] ];

  //init quant parameters for chroma
  for(i=0; i < 2; ++i)
  {
    qp_per_uv[i] = vidParam->qpPerMatrix[ curMb->qp_scaled[i + 1] ];
    qp_rem_uv[i] = vidParam->qpRemMatrix[ curMb->qp_scaled[i + 1] ];
  }

  InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[curSlice->colour_plane_id][qp_rem] : curSlice->InvLevelScale4x4_Inter[curSlice->colour_plane_id][qp_rem];
  InvLevelScale8x8 = intra? curSlice->InvLevelScale8x8_Intra[curSlice->colour_plane_id][qp_rem] : curSlice->InvLevelScale8x8_Inter[curSlice->colour_plane_id][qp_rem];

  // luma coefficients
  if (cbp)
  {
    if (!curMb->luma_transform_size_8x8_flag) // 4x4 transform
    {
      curMb->read_comp_coeff_4x4_CAVLC (curMb, PLANE_Y, InvLevelScale4x4, qp_per, cbp, vidParam->nzCoeff[mb_nr][PLANE_Y]);
    }
    else // 8x8 transform
    {
      curMb->read_comp_coeff_8x8_CAVLC (curMb, PLANE_Y, InvLevelScale8x8, qp_per, cbp, vidParam->nzCoeff[mb_nr][PLANE_Y]);
    }
  }
  else
  {
    memset(vidParam->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(byte));
  }

  //========================== CHROMA DC ============================
  //-----------------------------------------------------------------
  // chroma DC coeff
  if(cbp>15)
  {
    for (ll=0;ll<3;ll+=2)
    {
      int (*InvLevelScale4x4)[4] = NULL;
      uv = ll>>1;
      {
        int** imgcof = curSlice->cof[PLANE_U + uv];
        int m3[2][4] = {{0,0,0,0},{0,0,0,0}};
        int m4[2][4] = {{0,0,0,0},{0,0,0,0}};
        int qp_per_uv_dc = vidParam->qpPerMatrix[ (curMb->qpc[uv] + 3 + vidParam->bitdepth_chroma_qp_scale) ];       //for YUV422 only
        int qp_rem_uv_dc = vidParam->qpRemMatrix[ (curMb->qpc[uv] + 3 + vidParam->bitdepth_chroma_qp_scale) ];       //for YUV422 only
        if (intra)
          InvLevelScale4x4 = curSlice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv_dc];
        else
          InvLevelScale4x4 = curSlice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv_dc];


        //===================== CHROMA DC YUV422 ======================
        curSlice->read_coeff_4x4_CAVLC(curMb, CHROMA_DC, 0, 0, levarr, runarr, &numcoeff);
        coef_ctr=-1;
        for(k = 0; k < numcoeff; ++k)
        {
          if (levarr[k] != 0)
          {
            curMb->s_cbp[0].blk |= ((int64)0xff0000) << (ll<<2);
            coef_ctr += runarr[k]+1;
            i0 = SCAN_YUV422[coef_ctr][0];
            j0 = SCAN_YUV422[coef_ctr][1];

            m3[i0][j0]=levarr[k];
          }
        }

        // inverse CHROMA DC YUV422 transform
        // horizontal
        if(curMb->is_lossless == FALSE)
        {
          m4[0][0] = m3[0][0] + m3[1][0];
          m4[0][1] = m3[0][1] + m3[1][1];
          m4[0][2] = m3[0][2] + m3[1][2];
          m4[0][3] = m3[0][3] + m3[1][3];

          m4[1][0] = m3[0][0] - m3[1][0];
          m4[1][1] = m3[0][1] - m3[1][1];
          m4[1][2] = m3[0][2] - m3[1][2];
          m4[1][3] = m3[0][3] - m3[1][3];

          for (i = 0; i < 2; ++i)
          {
            m6[0] = m4[i][0] + m4[i][2];
            m6[1] = m4[i][0] - m4[i][2];
            m6[2] = m4[i][1] - m4[i][3];
            m6[3] = m4[i][1] + m4[i][3];

            imgcof[ 0][i<<2] = m6[0] + m6[3];
            imgcof[ 4][i<<2] = m6[1] + m6[2];
            imgcof[ 8][i<<2] = m6[1] - m6[2];
            imgcof[12][i<<2] = m6[0] - m6[3];
          }//for (i=0;i<2;++i)

          for(j = 0;j < vidParam->mb_cr_size_y; j += BLOCK_SIZE)
          {
            for(i=0;i < vidParam->mb_cr_size_x;i+=BLOCK_SIZE)
            {
              imgcof[j][i] = rshift_rnd_sf((imgcof[j][i] * InvLevelScale4x4[0][0]) << qp_per_uv_dc, 6);
            }
          }
        }
        else
        {
          for(j=0;j<4;++j)
          {
            curSlice->cof[PLANE_U + uv][j<<2][0] = m3[0][j];
            curSlice->cof[PLANE_U + uv][j<<2][4] = m3[1][j];
          }
        }

      }
    }//for (ll=0;ll<3;ll+=2)
  }

  //========================== CHROMA AC ============================
  //-----------------------------------------------------------------
  // chroma AC coeff, all zero fram start_scan
  if (cbp<=31)
  {
    memset (vidParam->nzCoeff [mb_nr ][1][0], 0, 2 * BLOCK_PIXELS * sizeof(byte));
  }
  else
  {
    if(curMb->is_lossless == FALSE)
    {
      for (b8=0; b8 < vidParam->num_blk8x8_uv; ++b8)
      {
        curMb->is_v_block = uv = (b8 > ((vidParam->num_uv_blocks) - 1 ));
        InvLevelScale4x4 = intra ? curSlice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : curSlice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];

        for (b4=0; b4 < 4; ++b4)
        {
          i = cofuv_blk_x[1][b8][b4];
          j = cofuv_blk_y[1][b8][b4];

          curSlice->read_coeff_4x4_CAVLC(curMb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
          coef_ctr = 0;

          for(k = 0; k < numcoeff;++k)
          {
            if (levarr[k] != 0)
            {
              curMb->s_cbp[0].blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              coef_ctr += runarr[k] + 1;

              i0=pos_scan4x4[coef_ctr][0];
              j0=pos_scan4x4[coef_ctr][1];

              curSlice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((levarr[k] * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
              //curSlice->fcf[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = levarr[k];
            }
          }
        }
      }
    }
    else
    {
      for (b8=0; b8 < vidParam->num_blk8x8_uv; ++b8)
      {
        curMb->is_v_block = uv = (b8 > ((vidParam->num_uv_blocks) - 1 ));

        for (b4=0; b4 < 4; ++b4)
        {
          i = cofuv_blk_x[1][b8][b4];
          j = cofuv_blk_y[1][b8][b4];

          curSlice->read_coeff_4x4_CAVLC(curMb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
          coef_ctr = 0;

          for(k = 0; k < numcoeff;++k)
          {
            if (levarr[k] != 0)
            {
              curMb->s_cbp[0].blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              coef_ctr += runarr[k] + 1;

              i0=pos_scan4x4[coef_ctr][0];
              j0=pos_scan4x4[coef_ctr][1];

              curSlice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = levarr[k];
            }
          }
        }
      }
    }
  } //if (picture->chromaFormatIdc != YUV400)
}
//}}}
//{{{
static void read_CBP_and_coeffs_from_NAL_CAVLC_444 (sMacroblock* curMb)
{
  int i,k;
  int mb_nr = curMb->mbAddrX;
  int cbp;
  sSyntaxElement currSE;
  sDataPartition *dP = NULL;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  int coef_ctr, i0, j0;
  int levarr[16], runarr[16], numcoeff;

  int qp_per, qp_rem;
  sVidParam* vidParam = curMb->vidParam;

  int uv;
  int qp_per_uv[3];
  int qp_rem_uv[3];

  int intra = (curMb->is_intra_block == TRUE);

  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  int (*InvLevelScale8x8)[8] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((vidParam->structure == FRAME) && (!curMb->mb_field)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];

  // read CBP if not new intra mode
  if (!IS_I16MB (curMb))
  {
    //=====   C B P   =====
    //---------------------
    currSE.type = (curMb->mb_type == I4MB || curMb->mb_type == SI4MB || curMb->mb_type == I8MB)
      ? SE_CBP_INTRA
      : SE_CBP_INTER;

    dP = &(curSlice->partArr[partMap[currSE.type]]);

    currSE.mapping = (curMb->mb_type == I4MB || curMb->mb_type == SI4MB || curMb->mb_type == I8MB)
      ? curSlice->linfo_cbp_intra
      : curSlice->linfo_cbp_inter;

    dP->readsSyntaxElement(curMb, &currSE, dP);
    curMb->cbp = cbp = currSE.value1;


    //============= Transform size flag for INTER MBs =============
    //-------------------------------------------------------------
    need_transform_size_flag = (((curMb->mb_type >= 1 && curMb->mb_type <= 3)||
      (IS_DIRECT(curMb) && vidParam->activeSPS->direct_8x8_inference_flag) ||
      (curMb->NoMbPartLessThan8x8Flag))
      && curMb->mb_type != I8MB && curMb->mb_type != I4MB
      && (curMb->cbp&15)
      && curSlice->transform8x8Mode);

    if (need_transform_size_flag)
    {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partArr[partMap[SE_HEADER]]);

      // read CAVLC transform_size_8x8_flag
      currSE.len = 1;
      readsSyntaxElement_FLC(&currSE, dP->bitstream);

      curMb->luma_transform_size_8x8_flag = (Boolean) currSE.value1;
    }

    //=====   DQUANT   =====
    //----------------------
    // Delta quant only if nonzero coeffs
    if (cbp !=0)
    {
      readDeltaQuant(&currSE, dP, curMb, partMap, ((curMb->is_intra_block == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (curSlice->dp_mode)
      {
        if ((curMb->is_intra_block == FALSE) && curSlice->dpC_NotPresent )
          curMb->dpl_flag = 1;

        if( intra && curSlice->dpB_NotPresent )
        {
          curMb->eiFlag = 1;
          curMb->dpl_flag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (curMb);
        if (curMb->dpl_flag)
        {
          cbp = 0;
          curMb->cbp = cbp;
        }
      }
    }
  }
  else  // read DC coeffs for new intra modes
  {
    cbp = curMb->cbp;

    readDeltaQuant(&currSE, dP, curMb, partMap, SE_DELTA_QUANT_INTRA);

    if (curSlice->dp_mode)
    {
      if (curSlice->dpB_NotPresent)
      {
        curMb->eiFlag  = 1;
        curMb->dpl_flag = 1;
      }
      checkDpNeighbours (curMb);
      if (curMb->dpl_flag)
      {
        curMb->cbp = cbp = 0;
      }
    }

    if (!curMb->dpl_flag)
    {
      pos_scan_4x4 = pos_scan4x4[0];

      curSlice->read_coeff_4x4_CAVLC(curMb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);

      for(k = 0; k < numcoeff; ++k)
      {
        if (levarr[k] != 0)                     // leave if level == 0
        {
          pos_scan_4x4 += 2 * runarr[k];

          i0 = ((*pos_scan_4x4++) << 2);
          j0 = ((*pos_scan_4x4++) << 2);

          curSlice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
          //curSlice->fcf[0][j0][i0] = levarr[k];// add new intra DC coeff
        }
      }


      if(curMb->is_lossless == FALSE)
        itrans_2(curMb, (eColorPlane) curSlice->colour_plane_id);// transform new intra DC
    }
  }

  updateQp(curMb, curSlice->qp);

  qp_per = vidParam->qpPerMatrix[ curMb->qp_scaled[curSlice->colour_plane_id] ];
  qp_rem = vidParam->qpRemMatrix[ curMb->qp_scaled[curSlice->colour_plane_id] ];

  //init quant parameters for chroma
  for(i=PLANE_U; i <= PLANE_V; ++i)
  {
    qp_per_uv[i] = vidParam->qpPerMatrix[ curMb->qp_scaled[i] ];
    qp_rem_uv[i] = vidParam->qpRemMatrix[ curMb->qp_scaled[i] ];
  }

  InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[curSlice->colour_plane_id][qp_rem] : curSlice->InvLevelScale4x4_Inter[curSlice->colour_plane_id][qp_rem];
  InvLevelScale8x8 = intra? curSlice->InvLevelScale8x8_Intra[curSlice->colour_plane_id][qp_rem] : curSlice->InvLevelScale8x8_Inter[curSlice->colour_plane_id][qp_rem];

  // luma coefficients
  if (cbp)
  {
    if (!curMb->luma_transform_size_8x8_flag) // 4x4 transform
    {
      curMb->read_comp_coeff_4x4_CAVLC (curMb, PLANE_Y, InvLevelScale4x4, qp_per, cbp, vidParam->nzCoeff[mb_nr][PLANE_Y]);
    }
    else // 8x8 transform
    {
      curMb->read_comp_coeff_8x8_CAVLC (curMb, PLANE_Y, InvLevelScale8x8, qp_per, cbp, vidParam->nzCoeff[mb_nr][PLANE_Y]);
    }
  }
  else
  {
    memset(vidParam->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(byte));
  }

  for (uv = PLANE_U; uv <= PLANE_V; ++uv )
  {
    /*----------------------16x16DC Luma_Add----------------------*/
    if (IS_I16MB (curMb)) // read DC coeffs for new intra modes
    {
      if (uv == PLANE_U)
        curSlice->read_coeff_4x4_CAVLC(curMb, CB_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
      else
        curSlice->read_coeff_4x4_CAVLC(curMb, CR_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);

      coef_ctr=-1;

      for(k = 0; k < numcoeff; ++k)
      {
        if (levarr[k] != 0)                     // leave if level == 0
        {
          coef_ctr += runarr[k] + 1;

          i0 = pos_scan4x4[coef_ctr][0];
          j0 = pos_scan4x4[coef_ctr][1];
          curSlice->cof[uv][j0<<2][i0<<2] = levarr[k];// add new intra DC coeff
          //curSlice->fcf[uv][j0<<2][i0<<2] = levarr[k];// add new intra DC coeff
        } //if leavarr[k]
      } //k loop

      if(curMb->is_lossless == FALSE)
      {
        itrans_2(curMb, (eColorPlane) (uv)); // transform new intra DC
      }
    } //IS_I16MB

    updateQp(curMb, curSlice->qp);

    //init constants for every chroma qp offset
    qp_per_uv[uv] = vidParam->qpPerMatrix[ curMb->qp_scaled[uv] ];
    qp_rem_uv[uv] = vidParam->qpRemMatrix[ curMb->qp_scaled[uv] ];

    InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[uv][qp_rem_uv[uv]] : curSlice->InvLevelScale4x4_Inter[uv][qp_rem_uv[uv]];
    InvLevelScale8x8 = intra? curSlice->InvLevelScale8x8_Intra[uv][qp_rem_uv[uv]] : curSlice->InvLevelScale8x8_Inter[uv][qp_rem_uv[uv]];

    if (!curMb->luma_transform_size_8x8_flag) // 4x4 transform
    {
      curMb->read_comp_coeff_4x4_CAVLC (curMb, (eColorPlane) (uv), InvLevelScale4x4, qp_per_uv[uv], cbp, vidParam->nzCoeff[mb_nr][uv]);
    }
    else // 8x8 transform
    {
      curMb->read_comp_coeff_8x8_CAVLC (curMb, (eColorPlane) (uv), InvLevelScale8x8, qp_per_uv[uv], cbp, vidParam->nzCoeff[mb_nr][uv]);
    }
  }
}
//}}}
//{{{
static void read_CBP_and_coeffs_from_NAL_CAVLC_420 (sMacroblock* curMb)
{
  int i,j,k;
  int mb_nr = curMb->mbAddrX;
  int cbp;
  sSyntaxElement currSE;
  sDataPartition *dP = NULL;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  int coef_ctr, i0, j0, b8;
  int ll;
  int levarr[16], runarr[16], numcoeff;

  int qp_per, qp_rem;
  sVidParam* vidParam = curMb->vidParam;
  int smb = ((vidParam->type==SP_SLICE) && (curMb->is_intra_block == FALSE)) || (vidParam->type == SI_SLICE && curMb->mb_type == SI4MB);

  int uv;
  int qp_per_uv[2];
  int qp_rem_uv[2];

  int intra = (curMb->is_intra_block == TRUE);
  int temp[4];

  int b4;
  //sPicture* picture = curSlice->picture;

  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  int (*InvLevelScale8x8)[8] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((vidParam->structure == FRAME) && (!curMb->mb_field)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];

  // read CBP if not new intra mode
  if (!IS_I16MB (curMb))
  {
    //=====   C B P   =====
    //---------------------
    currSE.type = (curMb->mb_type == I4MB || curMb->mb_type == SI4MB || curMb->mb_type == I8MB)
      ? SE_CBP_INTRA
      : SE_CBP_INTER;

    dP = &(curSlice->partArr[partMap[currSE.type]]);

    currSE.mapping = (curMb->mb_type == I4MB || curMb->mb_type == SI4MB || curMb->mb_type == I8MB)
      ? curSlice->linfo_cbp_intra
      : curSlice->linfo_cbp_inter;

    dP->readsSyntaxElement(curMb, &currSE, dP);
    curMb->cbp = cbp = currSE.value1;

    //============= Transform size flag for INTER MBs =============
    //-------------------------------------------------------------
    need_transform_size_flag = (((curMb->mb_type >= 1 && curMb->mb_type <= 3)||
      (IS_DIRECT(curMb) && vidParam->activeSPS->direct_8x8_inference_flag) ||
      (curMb->NoMbPartLessThan8x8Flag))
      && curMb->mb_type != I8MB && curMb->mb_type != I4MB
      && (curMb->cbp&15)
      && curSlice->transform8x8Mode);

    if (need_transform_size_flag)
    {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partArr[partMap[SE_HEADER]]);

      // read CAVLC transform_size_8x8_flag
      currSE.len = 1;
      readsSyntaxElement_FLC(&currSE, dP->bitstream);

      curMb->luma_transform_size_8x8_flag = (Boolean) currSE.value1;
    }

    //=====   DQUANT   =====
    //----------------------
    // Delta quant only if nonzero coeffs
    if (cbp !=0)
    {
      readDeltaQuant(&currSE, dP, curMb, partMap, ((curMb->is_intra_block == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (curSlice->dp_mode)
      {
        if ((curMb->is_intra_block == FALSE) && curSlice->dpC_NotPresent )
          curMb->dpl_flag = 1;

        if( intra && curSlice->dpB_NotPresent )
        {
          curMb->eiFlag = 1;
          curMb->dpl_flag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (curMb);
        if (curMb->dpl_flag)
        {
          cbp = 0;
          curMb->cbp = cbp;
        }
      }
    }
  }
  else
  {
    cbp = curMb->cbp;
    readDeltaQuant(&currSE, dP, curMb, partMap, SE_DELTA_QUANT_INTRA);

    if (curSlice->dp_mode)
    {
      if (curSlice->dpB_NotPresent)
      {
        curMb->eiFlag  = 1;
        curMb->dpl_flag = 1;
      }
      checkDpNeighbours (curMb);
      if (curMb->dpl_flag)
      {
        curMb->cbp = cbp = 0;
      }
    }

    if (!curMb->dpl_flag)
    {
      pos_scan_4x4 = pos_scan4x4[0];

      curSlice->read_coeff_4x4_CAVLC(curMb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);

      for(k = 0; k < numcoeff; ++k)
      {
        if (levarr[k] != 0)                     // leave if level == 0
        {
          pos_scan_4x4 += 2 * runarr[k];

          i0 = ((*pos_scan_4x4++) << 2);
          j0 = ((*pos_scan_4x4++) << 2);

          curSlice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
          //curSlice->fcf[0][j0][i0] = levarr[k];// add new intra DC coeff
        }
      }


      if(curMb->is_lossless == FALSE)
        itrans_2(curMb, (eColorPlane) curSlice->colour_plane_id);// transform new intra DC
    }
  }

  updateQp(curMb, curSlice->qp);

  qp_per = vidParam->qpPerMatrix[ curMb->qp_scaled[curSlice->colour_plane_id] ];
  qp_rem = vidParam->qpRemMatrix[ curMb->qp_scaled[curSlice->colour_plane_id] ];

  //init quant parameters for chroma
  for(i=0; i < 2; ++i)
  {
    qp_per_uv[i] = vidParam->qpPerMatrix[ curMb->qp_scaled[i + 1] ];
    qp_rem_uv[i] = vidParam->qpRemMatrix[ curMb->qp_scaled[i + 1] ];
  }

  InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[curSlice->colour_plane_id][qp_rem] : curSlice->InvLevelScale4x4_Inter[curSlice->colour_plane_id][qp_rem];
  InvLevelScale8x8 = intra? curSlice->InvLevelScale8x8_Intra[curSlice->colour_plane_id][qp_rem] : curSlice->InvLevelScale8x8_Inter[curSlice->colour_plane_id][qp_rem];

  // luma coefficients
  if (cbp)
  {
    if (!curMb->luma_transform_size_8x8_flag) // 4x4 transform
    {
      curMb->read_comp_coeff_4x4_CAVLC (curMb, PLANE_Y, InvLevelScale4x4, qp_per, cbp, vidParam->nzCoeff[mb_nr][PLANE_Y]);
    }
    else // 8x8 transform
    {
      curMb->read_comp_coeff_8x8_CAVLC (curMb, PLANE_Y, InvLevelScale8x8, qp_per, cbp, vidParam->nzCoeff[mb_nr][PLANE_Y]);
    }
  }
  else
  {
    memset(vidParam->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(byte));
  }

  //========================== CHROMA DC ============================
  //-----------------------------------------------------------------
  // chroma DC coeff
  if(cbp>15)
  {
    for (ll=0;ll<3;ll+=2)
    {
      uv = ll>>1;

      InvLevelScale4x4 = intra ? curSlice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : curSlice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];
      //===================== CHROMA DC YUV420 ======================
      //memset(curSlice->cofu, 0, 4 *sizeof(int));
      curSlice->cofu[0] = curSlice->cofu[1] = curSlice->cofu[2] = curSlice->cofu[3] = 0;
      coef_ctr=-1;

      curSlice->read_coeff_4x4_CAVLC(curMb, CHROMA_DC, 0, 0, levarr, runarr, &numcoeff);

      for(k = 0; k < numcoeff; ++k)
      {
        if (levarr[k] != 0)
        {
          curMb->s_cbp[0].blk |= 0xf0000 << (ll<<1) ;
          coef_ctr += runarr[k] + 1;
          curSlice->cofu[coef_ctr]=levarr[k];
        }
      }


      if (smb || (curMb->is_lossless == TRUE)) // check to see if MB type is SPred or SIntra4x4
      {
        curSlice->cof[PLANE_U + uv][0][0] = curSlice->cofu[0];
        curSlice->cof[PLANE_U + uv][0][4] = curSlice->cofu[1];
        curSlice->cof[PLANE_U + uv][4][0] = curSlice->cofu[2];
        curSlice->cof[PLANE_U + uv][4][4] = curSlice->cofu[3];
        //curSlice->fcf[PLANE_U + uv][0][0] = curSlice->cofu[0];
        //curSlice->fcf[PLANE_U + uv][4][0] = curSlice->cofu[1];
        //curSlice->fcf[PLANE_U + uv][0][4] = curSlice->cofu[2];
        //curSlice->fcf[PLANE_U + uv][4][4] = curSlice->cofu[3];
      }
      else
      {
        ihadamard2x2(curSlice->cofu, temp);
        //curSlice->fcf[PLANE_U + uv][0][0] = temp[0];
        //curSlice->fcf[PLANE_U + uv][0][4] = temp[1];
        //curSlice->fcf[PLANE_U + uv][4][0] = temp[2];
        //curSlice->fcf[PLANE_U + uv][4][4] = temp[3];

        curSlice->cof[PLANE_U + uv][0][0] = (((temp[0] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
        curSlice->cof[PLANE_U + uv][0][4] = (((temp[1] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
        curSlice->cof[PLANE_U + uv][4][0] = (((temp[2] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
        curSlice->cof[PLANE_U + uv][4][4] = (((temp[3] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
      }
    }
  }

  //========================== CHROMA AC ============================
  //-----------------------------------------------------------------
  // chroma AC coeff, all zero fram start_scan
  if (cbp<=31)
  {
    memset(vidParam->nzCoeff [mb_nr ][1][0], 0, 2 * BLOCK_PIXELS * sizeof(byte));
  }
  else
  {
    if(curMb->is_lossless == FALSE)
    {
      for (b8=0; b8 < vidParam->num_blk8x8_uv; ++b8)
      {
        curMb->is_v_block = uv = (b8 > ((vidParam->num_uv_blocks) - 1 ));
        InvLevelScale4x4 = intra ? curSlice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : curSlice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];

        for (b4=0; b4 < 4; ++b4)
        {
          i = cofuv_blk_x[0][b8][b4];
          j = cofuv_blk_y[0][b8][b4];

          curSlice->read_coeff_4x4_CAVLC(curMb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
          coef_ctr = 0;

          for(k = 0; k < numcoeff;++k)
          {
            if (levarr[k] != 0)
            {
              curMb->s_cbp[0].blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              coef_ctr += runarr[k] + 1;

              i0=pos_scan4x4[coef_ctr][0];
              j0=pos_scan4x4[coef_ctr][1];

              curSlice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((levarr[k] * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
              //curSlice->fcf[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = levarr[k];
            }
          }
        }
      }
    }
    else
    {
      for (b8=0; b8 < vidParam->num_blk8x8_uv; ++b8)
      {
        curMb->is_v_block = uv = (b8 > ((vidParam->num_uv_blocks) - 1 ));

        for (b4=0; b4 < 4; ++b4)
        {
          i = cofuv_blk_x[0][b8][b4];
          j = cofuv_blk_y[0][b8][b4];

          curSlice->read_coeff_4x4_CAVLC(curMb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
          coef_ctr = 0;

          for(k = 0; k < numcoeff;++k)
          {
            if (levarr[k] != 0)
            {
              curMb->s_cbp[0].blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              coef_ctr += runarr[k] + 1;

              i0=pos_scan4x4[coef_ctr][0];
              j0=pos_scan4x4[coef_ctr][1];

              curSlice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = levarr[k];
            }
          }
        }
      }
    }
  }
}
//}}}

//{{{
void set_read_comp_coeff_cavlc (sMacroblock* curMb) {

  if (curMb->is_lossless == FALSE) {
    curMb->read_comp_coeff_4x4_CAVLC = read_comp_coeff_4x4_CAVLC;
    curMb->read_comp_coeff_8x8_CAVLC = read_comp_coeff_8x8_CAVLC;
    }
  else {
    curMb->read_comp_coeff_4x4_CAVLC = read_comp_coeff_4x4_CAVLC_ls;
    curMb->read_comp_coeff_8x8_CAVLC = read_comp_coeff_8x8_CAVLC_ls;
    }
  }
//}}}
//{{{
void set_read_CBP_and_coeffs_cavlc (sSlice* curSlice) {

  switch (curSlice->vidParam->activeSPS->chromaFormatIdc) {
    case YUV444:
      if (curSlice->vidParam->separate_colour_plane_flag == 0)
        curSlice->read_CBP_and_coeffs_from_NAL = read_CBP_and_coeffs_from_NAL_CAVLC_444;
      else
        curSlice->read_CBP_and_coeffs_from_NAL = read_CBP_and_coeffs_from_NAL_CAVLC_400;
      break;

    case YUV422:
      curSlice->read_CBP_and_coeffs_from_NAL = read_CBP_and_coeffs_from_NAL_CAVLC_422;
      break;

    case YUV420:
      curSlice->read_CBP_and_coeffs_from_NAL = read_CBP_and_coeffs_from_NAL_CAVLC_420;
      break;

    case YUV400:
      curSlice->read_CBP_and_coeffs_from_NAL = read_CBP_and_coeffs_from_NAL_CAVLC_400;
      break;

    default:
      assert (1);
      curSlice->read_CBP_and_coeffs_from_NAL = NULL;
      break;
    }
  }
//}}}
