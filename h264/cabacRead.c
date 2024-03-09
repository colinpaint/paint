//{{{  includes
#include "global.h"
#include "elements.h"

#include "macroblock.h"
#include "cabac.h"
#include "vlc.h"
#include "transform.h"
//}}}

//{{{
static void read_comp_coeff_4x4_smb_CABAC (sMacroblock* curMb, sSyntaxElement* currSE, eColorPlane pl, int blockY, int blockX, int start_scan, int64 *cbp_blk)
{
  int i,j,k;
  int i0, j0;
  int level = 1;
  sDataPartition *dP;
  //sVidParam* vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];

  const byte (*pos_scan4x4)[2] = ((curSlice->structure == FRAME) && (!curMb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];
  int** cof = curSlice->cof[pl];

  for (j = blockY; j < blockY + BLOCK_SIZE_8x8; j += 4)
  {
    curMb->subblockY = j; // position for coeff_count ctx

    for (i = blockX; i < blockX + BLOCK_SIZE_8x8; i += 4)
    {
      curMb->subblockX = i; // position for coeff_count ctx
      pos_scan_4x4 = pos_scan4x4[start_scan];
      level = 1;

      if (start_scan == 0)
      {
        /*
        * make distinction between INTRA and INTER coded
        * luminance coefficients
        */
        currSE->type = (curMb->isIntraBlock ? SE_LUM_DC_INTRA : SE_LUM_DC_INTER);
        dP = &(curSlice->partitions[partMap[currSE->type]]);
        if (dP->bitstream->eiFlag)
          currSE->mapping = linfo_levrun_inter;
        else
          currSE->reading = readRunLevel_CABAC;

        dP->readsSyntaxElement(curMb, currSE, dP);
        level = currSE->value1;

        if (level != 0)    /* leave if level == 0 */
        {
          pos_scan_4x4 += 2 * currSE->value2;

          i0 = *pos_scan_4x4++;
          j0 = *pos_scan_4x4++;

          *cbp_blk |= i64_power2(j + (i >> 2)) ;
          //cof[j + j0][i + i0]= rshift_rnd_sf((level * InvLevelScale4x4[j0][i0]) << qp_per, 4);
          cof[j + j0][i + i0]= level;
          //curSlice->fcf[pl][j + j0][i + i0]= level;
        }
      }

      if (level != 0)
      {
        // make distinction between INTRA and INTER coded luminance coefficients
        currSE->type = (curMb->isIntraBlock ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER);
        dP = &(curSlice->partitions[partMap[currSE->type]]);

        if (dP->bitstream->eiFlag)
          currSE->mapping = linfo_levrun_inter;
        else
          currSE->reading = readRunLevel_CABAC;

        for(k = 1; (k < 17) && (level != 0); ++k)
        {
          dP->readsSyntaxElement(curMb, currSE, dP);
          level = currSE->value1;

          if (level != 0)    /* leave if level == 0 */
          {
            pos_scan_4x4 += 2 * currSE->value2;

            i0 = *pos_scan_4x4++;
            j0 = *pos_scan_4x4++;

            cof[j + j0][i + i0] = level;
            //curSlice->fcf[pl][j + j0][i + i0]= level;
          }
        }
      }
    }
  }
}
//}}}
//{{{
static void read_comp_coeff_4x4_CABAC (sMacroblock* curMb, sSyntaxElement* currSE, eColorPlane pl, int (*InvLevelScale4x4)[4], int qp_per, int cbp)
{
  sSlice* curSlice = curMb->slice;
  sVidParam* vidParam = curMb->vidParam;
  int start_scan = IS_I16MB (curMb)? 1 : 0;
  int blockY, blockX;
  int i, j;
  int64 *cbp_blk = &curMb->cbpStructure[pl].blk;

  if( pl == PLANE_Y || (vidParam->sepColourPlaneFlag != 0) )
    currSE->context = (IS_I16MB(curMb) ? LUMA_16AC: LUMA_4x4);
  else if (pl == PLANE_U)
    currSE->context = (IS_I16MB(curMb) ? CB_16AC: CB_4x4);
  else
    currSE->context = (IS_I16MB(curMb) ? CR_16AC: CR_4x4);

  for (blockY = 0; blockY < MB_BLOCK_SIZE; blockY += BLOCK_SIZE_8x8) /* all modes */
  {
    int** cof = &curSlice->cof[pl][blockY];
    for (blockX = 0; blockX < MB_BLOCK_SIZE; blockX += BLOCK_SIZE_8x8)
    {
      if (cbp & (1 << ((blockY >> 2) + (blockX >> 3))))  // are there any coeff in current block at all
      {
        read_comp_coeff_4x4_smb_CABAC (curMb, currSE, pl, blockY, blockX, start_scan, cbp_blk);

        if (start_scan == 0)
        {
          for (j = 0; j < BLOCK_SIZE_8x8; ++j)
          {
            int *coef = &cof[j][blockX];
            int jj = j & 0x03;
            for (i = 0; i < BLOCK_SIZE_8x8; i+=4)
            {
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
        else
        {
          for (j = 0; j < BLOCK_SIZE_8x8; ++j)
          {
            int *coef = &cof[j][blockX];
            int jj = j & 0x03;
            for (i = 0; i < BLOCK_SIZE_8x8; i += 4)
            {
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
static void read_comp_coeff_4x4_CABAC_ls (sMacroblock* curMb, sSyntaxElement* currSE, eColorPlane pl, int (*InvLevelScale4x4)[4], int qp_per, int cbp)
{
  sVidParam* vidParam = curMb->vidParam;
  int start_scan = IS_I16MB (curMb)? 1 : 0;
  int blockY, blockX;
  int64 *cbp_blk = &curMb->cbpStructure[pl].blk;

  if( pl == PLANE_Y || (vidParam->sepColourPlaneFlag != 0) )
    currSE->context = (IS_I16MB(curMb) ? LUMA_16AC: LUMA_4x4);
  else if (pl == PLANE_U)
    currSE->context = (IS_I16MB(curMb) ? CB_16AC: CB_4x4);
  else
    currSE->context = (IS_I16MB(curMb) ? CR_16AC: CR_4x4);

  for (blockY = 0; blockY < MB_BLOCK_SIZE; blockY += BLOCK_SIZE_8x8) /* all modes */
    for (blockX = 0; blockX < MB_BLOCK_SIZE; blockX += BLOCK_SIZE_8x8)
      if (cbp & (1 << ((blockY >> 2) + (blockX >> 3))))  // are there any coeff in current block at all
        read_comp_coeff_4x4_smb_CABAC (curMb, currSE, pl, blockY, blockX, start_scan, cbp_blk);
  }
//}}}

//{{{
static void readCompCoeff8x8_CABAC (sMacroblock* curMb, sSyntaxElement* currSE, eColorPlane pl, int b8)
{
  if (curMb->cbp & (1<<b8))  // are there any coefficients in the current block
  {
    sVidParam* vidParam = curMb->vidParam;
    int transform_pl = (vidParam->sepColourPlaneFlag != 0) ? curMb->slice->colourPlaneId : pl;

    int** tcoeffs;
    int i,j,k;
    int level = 1;

    sDataPartition *dP;
    sSlice* curSlice = curMb->slice;
    const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];
    int boff_x, boff_y;

    int64 cbp_mask = (int64) 51 << (4 * b8 - 2 * (b8 & 0x01)); // corresponds to 110011, as if all four 4x4 blocks contain coeff, shifted to block position
    int64 *cur_cbp = &curMb->cbpStructure[pl].blk;

    // select scan type
    const byte (*pos_scan8x8) = ((curSlice->structure == FRAME) && (!curMb->mbField)) ? SNGL_SCAN8x8[0] : FIELD_SCAN8x8[0];

    int qp_per = vidParam->qpPerMatrix[ curMb->qpScaled[pl] ];
    int qp_rem = vidParam->qpRemMatrix[ curMb->qpScaled[pl] ];

    int (*InvLevelScale8x8)[8] = (curMb->isIntraBlock == TRUE) ? curSlice->InvLevelScale8x8_Intra[transform_pl][qp_rem] : curSlice->InvLevelScale8x8_Inter[transform_pl][qp_rem];

    // === set offset in current macroblock ===
    boff_x = (b8&0x01) << 3;
    boff_y = (b8 >> 1) << 3;
    tcoeffs = &curSlice->mb_rres[pl][boff_y];

    curMb->subblockX = boff_x; // position for coeff_count ctx
    curMb->subblockY = boff_y; // position for coeff_count ctx

    if (pl==PLANE_Y || (vidParam->sepColourPlaneFlag != 0))
      currSE->context = LUMA_8x8;
    else if (pl==PLANE_U)
      currSE->context = CB_8x8;
    else
      currSE->context = CR_8x8;

    currSE->reading = readRunLevel_CABAC;

    // Read DC
    currSE->type = ((curMb->isIntraBlock == 1) ? SE_LUM_DC_INTRA : SE_LUM_DC_INTER ); // Intra or Inter?
    dP = &(curSlice->partitions[partMap[currSE->type]]);
    dP->readsSyntaxElement(curMb, currSE, dP);
    level = currSE->value1;

    //============ decode =============
    if (level != 0)    /* leave if level == 0 */
    {
      *cur_cbp |= cbp_mask;

      pos_scan8x8 += 2 * (currSE->value2);

      i = *pos_scan8x8++;
      j = *pos_scan8x8++;

      tcoeffs[j][boff_x + i] = rshift_rnd_sf((level * InvLevelScale8x8[j][i]) << qp_per, 6); // dequantization
      //tcoeffs[ j][boff_x + i] = level;

      // AC coefficients
      currSE->type    = ((curMb->isIntraBlock == 1) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER);
      dP = &(curSlice->partitions[partMap[currSE->type]]);

      for(k = 1;(k < 65) && (level != 0);++k)
      {
        dP->readsSyntaxElement(curMb, currSE, dP);
        level = currSE->value1;

        //============ decode =============
        if (level != 0)    /* leave if level == 0 */
        {
          pos_scan8x8 += 2 * (currSE->value2);

          i = *pos_scan8x8++;
          j = *pos_scan8x8++;

          tcoeffs[ j][boff_x + i] = rshift_rnd_sf((level * InvLevelScale8x8[j][i]) << qp_per, 6); // dequantization
          //tcoeffs[ j][boff_x + i] = level;
        }
      }
      /*
      for (j = 0; j < 8; j++)
      {
      for (i = 0; i < 8; i++)
      {
      if (tcoeffs[ j][boff_x + i])
      tcoeffs[ j][boff_x + i] = rshift_rnd_sf((tcoeffs[ j][boff_x + i] * InvLevelScale8x8[j][i]) << qp_per, 6); // dequantization
      }
      }
      */
    }
  }
}
//}}}
//{{{
static void readCompCoeff8x8_CABAC_lossless (sMacroblock* curMb, sSyntaxElement* currSE, eColorPlane pl, int b8)
{
  if (curMb->cbp & (1<<b8))  // are there any coefficients in the current block
  {
    sVidParam* vidParam = curMb->vidParam;

    int** tcoeffs;
    int i,j,k;
    int level = 1;

    sDataPartition *dP;
    sSlice* curSlice = curMb->slice;
    const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];
    int boff_x, boff_y;

    int64 cbp_mask = (int64) 51 << (4 * b8 - 2 * (b8 & 0x01)); // corresponds to 110011, as if all four 4x4 blocks contain coeff, shifted to block position
    int64 *cur_cbp = &curMb->cbpStructure[pl].blk;

    // select scan type
    const byte (*pos_scan8x8) = ((curSlice->structure == FRAME) && (!curMb->mbField)) ? SNGL_SCAN8x8[0] : FIELD_SCAN8x8[0];

    // === set offset in current macroblock ===
    boff_x = (b8&0x01) << 3;
    boff_y = (b8 >> 1) << 3;
    tcoeffs = &curSlice->mb_rres[pl][boff_y];

    curMb->subblockX = boff_x; // position for coeff_count ctx
    curMb->subblockY = boff_y; // position for coeff_count ctx

    if (pl==PLANE_Y || (vidParam->sepColourPlaneFlag != 0))
      currSE->context = LUMA_8x8;
    else if (pl==PLANE_U)
      currSE->context = CB_8x8;
    else
      currSE->context = CR_8x8;

    currSE->reading = readRunLevel_CABAC;

    for(k=0; (k < 65) && (level != 0);++k)
    {
      //============ read =============
      /*
      * make distinction between INTRA and INTER coded
      * luminance coefficients
      */

      currSE->type    = ((curMb->isIntraBlock == 1)
        ? (k==0 ? SE_LUM_DC_INTRA : SE_LUM_AC_INTRA)
        : (k==0 ? SE_LUM_DC_INTER : SE_LUM_AC_INTER));

      dP = &(curSlice->partitions[partMap[currSE->type]]);
      currSE->reading = readRunLevel_CABAC;

      dP->readsSyntaxElement(curMb, currSE, dP);
      level = currSE->value1;

      //============ decode =============
      if (level != 0)    /* leave if level == 0 */
      {
        pos_scan8x8 += 2 * (currSE->value2);

        i = *pos_scan8x8++;
        j = *pos_scan8x8++;

        *cur_cbp |= cbp_mask;

        tcoeffs[j][boff_x + i] = level;
      }
    }
  }
}
//}}}

//{{{
static void read_comp_coeff_8x8_MB_CABAC (sMacroblock* curMb, sSyntaxElement* currSE, eColorPlane pl)
{
  //======= 8x8 transform size & CABAC ========
  readCompCoeff8x8_CABAC (curMb, currSE, pl, 0);
  readCompCoeff8x8_CABAC (curMb, currSE, pl, 1);
  readCompCoeff8x8_CABAC (curMb, currSE, pl, 2);
  readCompCoeff8x8_CABAC (curMb, currSE, pl, 3);
}
//}}}
//{{{
static void read_comp_coeff_8x8_MB_CABAC_ls (sMacroblock* curMb, sSyntaxElement* currSE, eColorPlane pl)
{
  //======= 8x8 transform size & CABAC ========
  readCompCoeff8x8_CABAC_lossless (curMb, currSE, pl, 0);
  readCompCoeff8x8_CABAC_lossless (curMb, currSE, pl, 1);
  readCompCoeff8x8_CABAC_lossless (curMb, currSE, pl, 2);
  readCompCoeff8x8_CABAC_lossless (curMb, currSE, pl, 3);
}
//}}}

//{{{
static void read_CBP_and_coeffs_from_NAL_CABAC_420 (sMacroblock* curMb)
{
  int i,j;
  int level;
  int cbp;
  sSyntaxElement currSE;
  sDataPartition *dP = NULL;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];
  int i0, j0;

  int qp_per, qp_rem;
  sVidParam* vidParam = curMb->vidParam;
  int smb = ((vidParam->type==SP_SLICE) && (curMb->isIntraBlock == FALSE)) || (vidParam->type == SI_SLICE && curMb->mbType == SI4MB);

  int qp_per_uv[2];
  int qp_rem_uv[2];

  int intra = (curMb->isIntraBlock == TRUE);

  sPicture* picture = curSlice->picture;
  int yuv = picture->chromaFormatIdc - 1;

  int (*InvLevelScale4x4)[4] = NULL;

  // select scan type
  const byte (*pos_scan4x4)[2] = ((curSlice->structure == FRAME) && (!curMb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];

  if (!IS_I16MB (curMb))
  {
    int need_transform_size_flag;
    //=====   C B P   =====
    //---------------------
    currSE.type = (curMb->mbType == I4MB || curMb->mbType == SI4MB || curMb->mbType == I8MB)
      ? SE_CBP_INTRA
      : SE_CBP_INTER;

    dP = &(curSlice->partitions[partMap[currSE.type]]);

    if (dP->bitstream->eiFlag)
    {
      currSE.mapping = (curMb->mbType == I4MB || curMb->mbType == SI4MB || curMb->mbType == I8MB)
        ? curSlice->linfoCbpIntra
        : curSlice->linfoCbpInter;
    }
    else
      currSE.reading = read_CBP_CABAC;

    dP->readsSyntaxElement(curMb, &currSE, dP);
    curMb->cbp = cbp = currSE.value1;

    //============= Transform size flag for INTER MBs =============
    //-------------------------------------------------------------
    need_transform_size_flag = (((curMb->mbType >= 1 && curMb->mbType <= 3)||
      (IS_DIRECT(curMb) && vidParam->activeSPS->direct_8x8_inference_flag) ||
      (curMb->noMbPartLessThan8x8Flag))
      && curMb->mbType != I8MB && curMb->mbType != I4MB
      && (curMb->cbp&15)
      && curSlice->transform8x8Mode);

    if (need_transform_size_flag)
    {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partitions[partMap[SE_HEADER]]);
      currSE.reading = readMB_transform_size_flag_CABAC;

      // read CAVLC transform_size_8x8_flag
      if (dP->bitstream->eiFlag)
      {
        currSE.len = 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
      }
      else
      {
        dP->readsSyntaxElement(curMb, &currSE, dP);
      }
      curMb->lumaTransformSize8x8flag = (Boolean) currSE.value1;
    }

    //=====   DQUANT   =====
    //----------------------
    // Delta quant only if nonzero coeffs
    if (cbp !=0)
    {
      readDeltaQuant(&currSE, dP, curMb, partMap, ((curMb->isIntraBlock == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (curSlice->dataPartitionMode)
      {
        if ((curMb->isIntraBlock == FALSE) && curSlice->noDataPartitionC )
          curMb->dplFlag = 1;

        if( intra && curSlice->noDataPartitionB )
        {
          curMb->eiFlag = 1;
          curMb->dplFlag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (curMb);
        if (curMb->dplFlag)
        {
          cbp = 0;
          curMb->cbp = cbp;
        }
      }
    }
  }
  else // read DC coeffs for new intra modes
  {
    cbp = curMb->cbp;

    readDeltaQuant(&currSE, dP, curMb, partMap, SE_DELTA_QUANT_INTRA);

    if (curSlice->dataPartitionMode)
    {
      if (curSlice->noDataPartitionB)
      {
        curMb->eiFlag  = 1;
        curMb->dplFlag = 1;
      }
      checkDpNeighbours (curMb);
      if (curMb->dplFlag)
      {
        curMb->cbp = cbp = 0;
      }
    }

    if (!curMb->dplFlag)
    {
      int** cof = curSlice->cof[0];
      int k;
      pos_scan_4x4 = pos_scan4x4[0];

      currSE.type = SE_LUM_DC_INTRA;
      dP = &(curSlice->partitions[partMap[currSE.type]]);

      currSE.context      = LUMA_16DC;
      currSE.type         = SE_LUM_DC_INTRA;

      if (dP->bitstream->eiFlag)
        currSE.mapping = linfo_levrun_inter;
      else
        currSE.reading = readRunLevel_CABAC;

      level = 1;                            // just to get inside the loop

      for(k = 0; (k < 17) && (level != 0); ++k)
      {
        dP->readsSyntaxElement(curMb, &currSE, dP);
        level = currSE.value1;

        if (level != 0)    /* leave if level == 0 */
        {
          pos_scan_4x4 += (2 * currSE.value2);

          i0 = ((*pos_scan_4x4++) << 2);
          j0 = ((*pos_scan_4x4++) << 2);

          cof[j0][i0] = level;// add new intra DC coeff
          //curSlice->fcf[0][j0][i0] = level;// add new intra DC coeff
        }
      }

      if(curMb->isLossless == FALSE)
        itrans_2(curMb, (eColorPlane) curSlice->colourPlaneId);// transform new intra DC
    }
  }

  updateQp(curMb, curSlice->qp);

  qp_per = vidParam->qpPerMatrix[ curMb->qpScaled[curSlice->colourPlaneId] ];
  qp_rem = vidParam->qpRemMatrix[ curMb->qpScaled[curSlice->colourPlaneId] ];

  // luma coefficients
  //======= Other Modes & CABAC ========
  //------------------------------------
  if (cbp)
  {
    if(curMb->lumaTransformSize8x8flag)
    {
      //======= 8x8 transform size & CABAC ========
      curMb->read_comp_coeff_8x8_CABAC (curMb, &currSE, PLANE_Y);
    }
    else
    {
      InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[curSlice->colourPlaneId][qp_rem] : curSlice->InvLevelScale4x4_Inter[curSlice->colourPlaneId][qp_rem];
      curMb->read_comp_coeff_4x4_CABAC (curMb, &currSE, PLANE_Y, InvLevelScale4x4, qp_per, cbp);
    }
  }

  //init quant parameters for chroma
  for (i=0; i < 2; ++i)
  {
    qp_per_uv[i] = vidParam->qpPerMatrix[ curMb->qpScaled[i + 1] ];
    qp_rem_uv[i] = vidParam->qpRemMatrix[ curMb->qpScaled[i + 1] ];
  }

  //========================== CHROMA DC ============================
  //-----------------------------------------------------------------
  // chroma DC coeff
  if(cbp>15)
  {
    sCBPStructure  *cbpStructure = &curMb->cbpStructure[0];
    int uv, ll, k, coef_ctr;

    for (ll = 0; ll < 3; ll += 2) {
      uv = ll >> 1;

      InvLevelScale4x4 = intra ? curSlice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : curSlice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];
      //===================== CHROMA DC YUV420 ======================
      //memset(curSlice->cofu, 0, 4*sizeof(int));
      curSlice->cofu[0] = curSlice->cofu[1] = curSlice->cofu[2] = curSlice->cofu[3] = 0;
      coef_ctr = -1;

      level = 1;
      curMb->isVblock = ll;
      currSE.context = CHROMA_DC;
      currSE.type = (intra ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER);

      dP = &(curSlice->partitions[partMap[currSE.type]]);

      if (dP->bitstream->eiFlag)
        currSE.mapping = linfo_levrun_c2x2;
      else
        currSE.reading = readRunLevel_CABAC;

      for(k = 0; (k < (vidParam->numCdcCoeff + 1)) && (level != 0); ++k) {
        dP->readsSyntaxElement (curMb, &currSE, dP);
        level = currSE.value1;

        if (level != 0) {
          cbpStructure->blk |= 0xf0000 << (ll<<1) ;
          coef_ctr += currSE.value2 + 1;

          // Bug: curSlice->cofu has only 4 entries, hence coef_ctr MUST be <4 (which is
          // caught by the assert().  If it is bigger than 4, it starts patching the
          // vidParam->predmode pointer, which leads to bugs later on.
          //
          // This assert() should be left in the code, because it captures a very likely
          // bug early when testing in error prone environments (or when testing NAL
          // functionality).

          assert (coef_ctr < vidParam->numCdcCoeff);
          curSlice->cofu[coef_ctr] = level;
        }
      }


      if (smb || (curMb->isLossless == TRUE)) // check to see if MB type is SPred or SIntra4x4
      {
        curSlice->cof[uv + 1][0][0] = curSlice->cofu[0];
        curSlice->cof[uv + 1][0][4] = curSlice->cofu[1];
        curSlice->cof[uv + 1][4][0] = curSlice->cofu[2];
        curSlice->cof[uv + 1][4][4] = curSlice->cofu[3];
        //curSlice->fcf[uv + 1][0][0] = curSlice->cofu[0];
        //curSlice->fcf[uv + 1][4][0] = curSlice->cofu[1];
        //curSlice->fcf[uv + 1][0][4] = curSlice->cofu[2];
        //curSlice->fcf[uv + 1][4][4] = curSlice->cofu[3];
      }
      else {
        int temp[4];
        int scale_dc = InvLevelScale4x4[0][0];
        int** cof = curSlice->cof[uv + 1];

        ihadamard2x2(curSlice->cofu, temp);

        //curSlice->fcf[uv + 1][0][0] = temp[0];
        //curSlice->fcf[uv + 1][0][4] = temp[1];
        //curSlice->fcf[uv + 1][4][0] = temp[2];
        //curSlice->fcf[uv + 1][4][4] = temp[3];

        cof[0][0] = (((temp[0] * scale_dc) << qp_per_uv[uv]) >> 5);
        cof[0][4] = (((temp[1] * scale_dc) << qp_per_uv[uv]) >> 5);
        cof[4][0] = (((temp[2] * scale_dc) << qp_per_uv[uv]) >> 5);
        cof[4][4] = (((temp[3] * scale_dc) << qp_per_uv[uv]) >> 5);
      }
    }
  }

  //========================== CHROMA AC ============================
  //-----------------------------------------------------------------
  // chroma AC coeff, all zero fram start_scan
  if (cbp >31)
  {
    currSE.context      = CHROMA_AC;
    currSE.type         = (curMb->isIntraBlock ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER);

    dP = &(curSlice->partitions[partMap[currSE.type]]);

    if (dP->bitstream->eiFlag)
      currSE.mapping = linfo_levrun_inter;
    else
      currSE.reading = readRunLevel_CABAC;

    if (curMb->isLossless == FALSE) {
      int b4, b8, uv, k;
      int** cof;
      sCBPStructure  *cbpStructure = &curMb->cbpStructure[0];
      for (b8=0; b8 < vidParam->numBlock8x8uv; ++b8) {
        curMb->isVblock = uv = (b8 > ((vidParam->numUvBlocks) - 1 ));
        InvLevelScale4x4 = intra ? curSlice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : curSlice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];
        cof = curSlice->cof[uv + 1];

        for (b4 = 0; b4 < 4; ++b4) {
          i = cofuv_blk_x[yuv][b8][b4];
          j = cofuv_blk_y[yuv][b8][b4];

          curMb->subblockY = subblk_offset_y[yuv][b8][b4];
          curMb->subblockX = subblk_offset_x[yuv][b8][b4];

          pos_scan_4x4 = pos_scan4x4[1];
          level = 1;

          for(k = 0; (k < 16) && (level != 0);++k) {
            dP->readsSyntaxElement(curMb, &currSE, dP);
            level = currSE.value1;

            if (level != 0) {
              cbpStructure->blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              pos_scan_4x4 += (currSE.value2 << 1);

              i0 = *pos_scan_4x4++;
              j0 = *pos_scan_4x4++;

              cof[(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((level * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
              //curSlice->fcf[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
            }
          } //for(k=0;(k<16)&&(level!=0);++k)
        }
      }
    }
    else {
      sCBPStructure  *cbpStructure = &curMb->cbpStructure[0];
      int b4, b8, k;
      int uv;
      for (b8=0; b8 < vidParam->numBlock8x8uv; ++b8) {
        curMb->isVblock = uv = (b8 > ((vidParam->numUvBlocks) - 1 ));

        for (b4=0; b4 < 4; ++b4) {
          i = cofuv_blk_x[yuv][b8][b4];
          j = cofuv_blk_y[yuv][b8][b4];

          pos_scan_4x4 = pos_scan4x4[1];
          level=1;

          curMb->subblockY = subblk_offset_y[yuv][b8][b4];
          curMb->subblockX = subblk_offset_x[yuv][b8][b4];

          for(k=0;(k<16)&&(level!=0);++k) {
            dP->readsSyntaxElement(curMb, &currSE, dP);
            level = currSE.value1;

            if (level != 0) {
              cbpStructure->blk |= i64_power2(cbp_blk_chroma[b8][b4]);
              pos_scan_4x4 += (currSE.value2 << 1);

              i0 = *pos_scan_4x4++;
              j0 = *pos_scan_4x4++;

              curSlice->cof[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
              //curSlice->fcf[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
            }
          }
        }
      }
    } //for (b4=0; b4 < 4; b4++)
  }
}
//}}}
//{{{
static void read_CBP_and_coeffs_from_NAL_CABAC_400 (sMacroblock* curMb)
{
  int k;
  int level;
  int cbp;
  sSyntaxElement currSE;
  sDataPartition *dP = NULL;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];
  int i0, j0;

  int qp_per, qp_rem;
  sVidParam* vidParam = curMb->vidParam;

  int intra = (curMb->isIntraBlock == TRUE);

  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((curSlice->structure == FRAME) && (!curMb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];


  // read CBP if not new intra mode
  if (!IS_I16MB (curMb)) {
    //=====   C B P   =====
    //---------------------
    currSE.type = (curMb->mbType == I4MB || curMb->mbType == SI4MB || curMb->mbType == I8MB)
      ? SE_CBP_INTRA
      : SE_CBP_INTER;

    dP = &(curSlice->partitions[partMap[currSE.type]]);

    if (dP->bitstream->eiFlag) {
      currSE.mapping = (curMb->mbType == I4MB || curMb->mbType == SI4MB || curMb->mbType == I8MB)
        ? curSlice->linfoCbpIntra
        : curSlice->linfoCbpInter;
    }
    else
      currSE.reading = read_CBP_CABAC;

    dP->readsSyntaxElement(curMb, &currSE, dP);
    curMb->cbp = cbp = currSE.value1;


    //============= Transform size flag for INTER MBs =============
    //-------------------------------------------------------------
    need_transform_size_flag = (((curMb->mbType >= 1 && curMb->mbType <= 3)||
      (IS_DIRECT(curMb) && vidParam->activeSPS->direct_8x8_inference_flag) ||
      (curMb->noMbPartLessThan8x8Flag))
      && curMb->mbType != I8MB && curMb->mbType != I4MB
      && (curMb->cbp&15)
      && curSlice->transform8x8Mode);

    if (need_transform_size_flag) {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partitions[partMap[SE_HEADER]]);
      currSE.reading = readMB_transform_size_flag_CABAC;

      // read CAVLC transform_size_8x8_flag
      if (dP->bitstream->eiFlag) {
        currSE.len = 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
      }
      else
        dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->lumaTransformSize8x8flag = (Boolean) currSE.value1;
    }

    //=====   DQUANT   =====
    //----------------------
    // Delta quant only if nonzero coeffs
    if (cbp !=0)
    {
      readDeltaQuant(&currSE, dP, curMb, partMap, ((curMb->isIntraBlock == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (curSlice->dataPartitionMode)  {
        if ((curMb->isIntraBlock == FALSE) && curSlice->noDataPartitionC )
          curMb->dplFlag = 1;

        if( intra && curSlice->noDataPartitionB )  {
          curMb->eiFlag = 1;
          curMb->dplFlag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (curMb);
        if (curMb->dplFlag) {
          cbp = 0;
          curMb->cbp = cbp;
        }
      }
    }
  }
  else // read DC coeffs for new intra modes
  {
    cbp = curMb->cbp;
    readDeltaQuant(&currSE, dP, curMb, partMap, SE_DELTA_QUANT_INTRA);

    if (curSlice->dataPartitionMode) {
      if (curSlice->noDataPartitionB) {
        curMb->eiFlag  = 1;
        curMb->dplFlag = 1;
      }
      checkDpNeighbours (curMb);
      if (curMb->dplFlag)
        curMb->cbp = cbp = 0;
    }

    if (!curMb->dplFlag) {
      pos_scan_4x4 = pos_scan4x4[0];

      {
        currSE.type = SE_LUM_DC_INTRA;
        dP = &(curSlice->partitions[partMap[currSE.type]]);

        currSE.context      = LUMA_16DC;
        currSE.type         = SE_LUM_DC_INTRA;

        if (dP->bitstream->eiFlag)
          currSE.mapping = linfo_levrun_inter;
        else
          currSE.reading = readRunLevel_CABAC;

        level = 1;                            // just to get inside the loop

        for(k = 0; (k < 17) && (level != 0); ++k) {
          dP->readsSyntaxElement(curMb, &currSE, dP);
          level = currSE.value1;

          if (level != 0)    /* leave if level == 0 */
          {
            pos_scan_4x4 += (2 * currSE.value2);

            i0 = ((*pos_scan_4x4++) << 2);
            j0 = ((*pos_scan_4x4++) << 2);

            curSlice->cof[0][j0][i0] = level;// add new intra DC coeff
            //curSlice->fcf[0][j0][i0] = level;// add new intra DC coeff
          }
        }
      }

      if(curMb->isLossless == FALSE)
        itrans_2(curMb, (eColorPlane) curSlice->colourPlaneId);// transform new intra DC
    }
  }

  updateQp(curMb, curSlice->qp);

  qp_per = vidParam->qpPerMatrix[ curMb->qpScaled[PLANE_Y] ];
  qp_rem = vidParam->qpRemMatrix[ curMb->qpScaled[PLANE_Y] ];

  //======= Other Modes & CABAC ========
  //------------------------------------
  if (cbp)  {
    if(curMb->lumaTransformSize8x8flag)  {
      //======= 8x8 transform size & CABAC ========
      curMb->read_comp_coeff_8x8_CABAC (curMb, &currSE, PLANE_Y);
    }
    else {
      InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[curSlice->colourPlaneId][qp_rem] : curSlice->InvLevelScale4x4_Inter[curSlice->colourPlaneId][qp_rem];
      curMb->read_comp_coeff_4x4_CABAC (curMb, &currSE, PLANE_Y, InvLevelScale4x4, qp_per, cbp);
    }
  }
}
//}}}
//{{{
static void read_CBP_and_coeffs_from_NAL_CABAC_444 (sMacroblock* curMb)
{
  int i, k;
  int level;
  int cbp;
  sSyntaxElement currSE;
  sDataPartition *dP = NULL;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];
  int coef_ctr, i0, j0;

  int qp_per, qp_rem;
  sVidParam* vidParam = curMb->vidParam;

  int uv;
  int qp_per_uv[2];
  int qp_rem_uv[2];

  int intra = (curMb->isIntraBlock == TRUE);


  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((curSlice->structure == FRAME) && (!curMb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];

  // QPI
  //init constants for every chroma qp offset
  for (i=0; i<2; ++i) {
    qp_per_uv[i] = vidParam->qpPerMatrix[ curMb->qpScaled[i + 1] ];
    qp_rem_uv[i] = vidParam->qpRemMatrix[ curMb->qpScaled[i + 1] ];
  }


  // read CBP if not new intra mode
  if (!IS_I16MB (curMb)) {
    //=====   C B P   =====
    //---------------------
    currSE.type = (curMb->mbType == I4MB || curMb->mbType == SI4MB || curMb->mbType == I8MB)
      ? SE_CBP_INTRA
      : SE_CBP_INTER;

    dP = &(curSlice->partitions[partMap[currSE.type]]);

    if (dP->bitstream->eiFlag) {
      currSE.mapping = (curMb->mbType == I4MB || curMb->mbType == SI4MB || curMb->mbType == I8MB)
        ? curSlice->linfoCbpIntra
        : curSlice->linfoCbpInter;
    }
    else
      currSE.reading = read_CBP_CABAC;

    dP->readsSyntaxElement(curMb, &currSE, dP);
    curMb->cbp = cbp = currSE.value1;


    //============= Transform size flag for INTER MBs =============
    //-------------------------------------------------------------
    need_transform_size_flag = (((curMb->mbType >= 1 && curMb->mbType <= 3)||
      (IS_DIRECT(curMb) && vidParam->activeSPS->direct_8x8_inference_flag) ||
      (curMb->noMbPartLessThan8x8Flag))
      && curMb->mbType != I8MB && curMb->mbType != I4MB
      && (curMb->cbp&15)
      && curSlice->transform8x8Mode);

    if (need_transform_size_flag)
    {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partitions[partMap[SE_HEADER]]);
      currSE.reading = readMB_transform_size_flag_CABAC;

      // read CAVLC transform_size_8x8_flag
      if (dP->bitstream->eiFlag) {
        currSE.len = 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
      }
      else
        dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->lumaTransformSize8x8flag = (Boolean) currSE.value1;
    }

    //=====   DQUANT   =====
    //----------------------
    // Delta quant only if nonzero coeffs
    if (cbp !=0)
    {
      readDeltaQuant(&currSE, dP, curMb, partMap, ((curMb->isIntraBlock == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (curSlice->dataPartitionMode) {
        if ((curMb->isIntraBlock == FALSE) && curSlice->noDataPartitionC )
          curMb->dplFlag = 1;

        if( intra && curSlice->noDataPartitionB ) {
          curMb->eiFlag = 1;
          curMb->dplFlag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (curMb);
        if (curMb->dplFlag) {
          cbp = 0;
          curMb->cbp = cbp;
        }
      }
    }
  }
  else // read DC coeffs for new intra modes
  {
    cbp = curMb->cbp;

    readDeltaQuant(&currSE, dP, curMb, partMap, SE_DELTA_QUANT_INTRA);

    if (curSlice->dataPartitionMode) {
      if (curSlice->noDataPartitionB) {
        curMb->eiFlag  = 1;
        curMb->dplFlag = 1;
      }
      checkDpNeighbours (curMb);
      if (curMb->dplFlag)
        curMb->cbp = cbp = 0;
    }

    if (!curMb->dplFlag) {
      pos_scan_4x4 = pos_scan4x4[0];

      {
        currSE.type = SE_LUM_DC_INTRA;
        dP = &(curSlice->partitions[partMap[currSE.type]]);

        currSE.context      = LUMA_16DC;
        currSE.type         = SE_LUM_DC_INTRA;

        if (dP->bitstream->eiFlag)
          currSE.mapping = linfo_levrun_inter;
        else
          currSE.reading = readRunLevel_CABAC;

        level = 1;                            // just to get inside the loop

        for(k = 0; (k < 17) && (level != 0); ++k) {
          dP->readsSyntaxElement(curMb, &currSE, dP);
          level = currSE.value1;

          if (level != 0)    /* leave if level == 0 */
          {
            pos_scan_4x4 += (2 * currSE.value2);

            i0 = ((*pos_scan_4x4++) << 2);
            j0 = ((*pos_scan_4x4++) << 2);

            curSlice->cof[0][j0][i0] = level;// add new intra DC coeff
            //curSlice->fcf[0][j0][i0] = level;// add new intra DC coeff
          }
        }
      }

      if(curMb->isLossless == FALSE)
        itrans_2(curMb, (eColorPlane) curSlice->colourPlaneId);// transform new intra DC
    }
  }

  updateQp(curMb, curSlice->qp);

  qp_per = vidParam->qpPerMatrix[ curMb->qpScaled[curSlice->colourPlaneId] ];
  qp_rem = vidParam->qpRemMatrix[ curMb->qpScaled[curSlice->colourPlaneId] ];

  //init quant parameters for chroma
  for(i=0; i < 2; ++i) {
    qp_per_uv[i] = vidParam->qpPerMatrix[ curMb->qpScaled[i + 1] ];
    qp_rem_uv[i] = vidParam->qpRemMatrix[ curMb->qpScaled[i + 1] ];
  }


  InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[curSlice->colourPlaneId][qp_rem] : curSlice->InvLevelScale4x4_Inter[curSlice->colourPlaneId][qp_rem];

  // luma coefficients
  {
    //======= Other Modes & CABAC ========
    //------------------------------------
    if (cbp)
    {
      if(curMb->lumaTransformSize8x8flag)
        //======= 8x8 transform size & CABAC ========
        curMb->read_comp_coeff_8x8_CABAC (curMb, &currSE, PLANE_Y);
      else
        curMb->read_comp_coeff_4x4_CABAC (curMb, &currSE, PLANE_Y, InvLevelScale4x4, qp_per, cbp);
    }
  }

  for (uv = 0; uv < 2; ++uv )
  {
    /*----------------------16x16DC Luma_Add----------------------*/
    if (IS_I16MB (curMb)) // read DC coeffs for new intra modes
    {
      {
        currSE.type = SE_LUM_DC_INTRA;
        dP = &(curSlice->partitions[partMap[currSE.type]]);

        if( (vidParam->sepColourPlaneFlag != 0) )
          currSE.context = LUMA_16DC;
        else
          currSE.context = (uv==0) ? CB_16DC : CR_16DC;

        if (dP->bitstream->eiFlag)
          currSE.mapping = linfo_levrun_inter;
        else
          currSE.reading = readRunLevel_CABAC;

        coef_ctr = -1;
        level = 1;                            // just to get inside the loop

        for(k=0;(k<17) && (level!=0);++k) {
          dP->readsSyntaxElement(curMb, &currSE, dP);
          level = currSE.value1;

          if (level != 0)                     // leave if level == 0
          {
            coef_ctr += currSE.value2 + 1;

            i0 = pos_scan4x4[coef_ctr][0];
            j0 = pos_scan4x4[coef_ctr][1];
            curSlice->cof[uv + 1][j0<<2][i0<<2] = level;
            //curSlice->fcf[uv + 1][j0<<2][i0<<2] = level;
          }
        } //k loop
      } // else CAVLC

      if(curMb->isLossless == FALSE)
        itrans_2(curMb, (eColorPlane) (uv + 1)); // transform new intra DC
    } //IS_I16MB

    updateQp(curMb, curSlice->qp);

    qp_per = vidParam->qpPerMatrix[ (curSlice->qp + vidParam->bitdepth_luma_qp_scale) ];
    qp_rem = vidParam->qpRemMatrix[ (curSlice->qp + vidParam->bitdepth_luma_qp_scale) ];

    //init constants for every chroma qp offset
    qp_per_uv[uv] = vidParam->qpPerMatrix[ (curMb->qpc[uv] + vidParam->bitdepthChromaQpScale) ];
    qp_rem_uv[uv] = vidParam->qpRemMatrix[ (curMb->qpc[uv] + vidParam->bitdepthChromaQpScale) ];

    InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : curSlice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];

    {
      if (cbp) {
        if(curMb->lumaTransformSize8x8flag) {
          //======= 8x8 transform size & CABAC ========
          curMb->read_comp_coeff_8x8_CABAC (curMb, &currSE, (eColorPlane) (PLANE_U + uv));
        }
        else //4x4
          curMb->read_comp_coeff_4x4_CABAC (curMb, &currSE, (eColorPlane) (PLANE_U + uv), InvLevelScale4x4,  qp_per_uv[uv], cbp);
      }
    }
  }
}
//}}}
//{{{
static void read_CBP_and_coeffs_from_NAL_CABAC_422 (sMacroblock* curMb)
{
  int i,j,k;
  int level;
  int cbp;
  sSyntaxElement currSE;
  sDataPartition *dP = NULL;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];
  int coef_ctr, i0, j0, b8;
  int ll;

  int qp_per, qp_rem;
  sVidParam* vidParam = curMb->vidParam;

  int uv;
  int qp_per_uv[2];
  int qp_rem_uv[2];

  int intra = (curMb->isIntraBlock == TRUE);

  int b4;
  sPicture* picture = curSlice->picture;
  int yuv = picture->chromaFormatIdc - 1;
  int m6[4];

  int need_transform_size_flag;

  int (*InvLevelScale4x4)[4] = NULL;
  // select scan type
  const byte (*pos_scan4x4)[2] = ((curSlice->structure == FRAME) && (!curMb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
  const byte *pos_scan_4x4 = pos_scan4x4[0];

  // QPI
  //init constants for every chroma qp offset
  for (i=0; i<2; ++i)
  {
    qp_per_uv[i] = vidParam->qpPerMatrix[ curMb->qpScaled[i + 1] ];
    qp_rem_uv[i] = vidParam->qpRemMatrix[ curMb->qpScaled[i + 1] ];
  }

  // read CBP if not new intra mode
  if (!IS_I16MB (curMb))
  {
    //=====   C B P   =====
    //---------------------
    currSE.type = (curMb->mbType == I4MB || curMb->mbType == SI4MB || curMb->mbType == I8MB)
      ? SE_CBP_INTRA
      : SE_CBP_INTER;

    dP = &(curSlice->partitions[partMap[currSE.type]]);

    if (dP->bitstream->eiFlag) {
      currSE.mapping = (curMb->mbType == I4MB || curMb->mbType == SI4MB || curMb->mbType == I8MB)
        ? curSlice->linfoCbpIntra
        : curSlice->linfoCbpInter;
    }
    else
      currSE.reading = read_CBP_CABAC;

    dP->readsSyntaxElement(curMb, &currSE, dP);
    curMb->cbp = cbp = currSE.value1;


    //============= Transform size flag for INTER MBs =============
    //-------------------------------------------------------------
    need_transform_size_flag = (((curMb->mbType >= 1 && curMb->mbType <= 3)||
      (IS_DIRECT(curMb) && vidParam->activeSPS->direct_8x8_inference_flag) ||
      (curMb->noMbPartLessThan8x8Flag))
      && curMb->mbType != I8MB && curMb->mbType != I4MB
      && (curMb->cbp&15)
      && curSlice->transform8x8Mode);

    if (need_transform_size_flag) {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partitions[partMap[SE_HEADER]]);
      currSE.reading = readMB_transform_size_flag_CABAC;

      // read CAVLC transform_size_8x8_flag
      if (dP->bitstream->eiFlag) {
        currSE.len = 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
      }
      else
        dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->lumaTransformSize8x8flag = (Boolean) currSE.value1;
    }

    //=====   DQUANT   =====
    //----------------------
    // Delta quant only if nonzero coeffs
    if (cbp !=0) {
      readDeltaQuant(&currSE, dP, curMb, partMap, ((curMb->isIntraBlock == FALSE)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

      if (curSlice->dataPartitionMode)
      {
        if ((curMb->isIntraBlock == FALSE) && curSlice->noDataPartitionC )
          curMb->dplFlag = 1;

        if( intra && curSlice->noDataPartitionB ) {
          curMb->eiFlag = 1;
          curMb->dplFlag = 1;
        }

        // check for prediction from neighbours
        checkDpNeighbours (curMb);
        if (curMb->dplFlag) {
          cbp = 0;
          curMb->cbp = cbp;
        }
      }
    }
  }
  else // read DC coeffs for new intra modes
  {
    cbp = curMb->cbp;

    readDeltaQuant(&currSE, dP, curMb, partMap, SE_DELTA_QUANT_INTRA);

    if (curSlice->dataPartitionMode) {
      if (curSlice->noDataPartitionB) {
        curMb->eiFlag  = 1;
        curMb->dplFlag = 1;
      }
      checkDpNeighbours (curMb);
      if (curMb->dplFlag)
        curMb->cbp = cbp = 0;
    }

    if (!curMb->dplFlag) {
      pos_scan_4x4 = pos_scan4x4[0];

      {
        currSE.type = SE_LUM_DC_INTRA;
        dP = &(curSlice->partitions[partMap[currSE.type]]);

        currSE.context      = LUMA_16DC;
        currSE.type         = SE_LUM_DC_INTRA;

        if (dP->bitstream->eiFlag)
          currSE.mapping = linfo_levrun_inter;
        else
          currSE.reading = readRunLevel_CABAC;

        level = 1;                            // just to get inside the loop

        for(k = 0; (k < 17) && (level != 0); ++k) {
          dP->readsSyntaxElement(curMb, &currSE, dP);
          level = currSE.value1;

          if (level != 0)    /* leave if level == 0 */
          {
            pos_scan_4x4 += (2 * currSE.value2);

            i0 = ((*pos_scan_4x4++) << 2);
            j0 = ((*pos_scan_4x4++) << 2);

            curSlice->cof[0][j0][i0] = level;// add new intra DC coeff
            //curSlice->fcf[0][j0][i0] = level;// add new intra DC coeff
          }
        }
      }

      if(curMb->isLossless == FALSE)
        itrans_2(curMb, (eColorPlane) curSlice->colourPlaneId);// transform new intra DC
    }
  }

  updateQp(curMb, curSlice->qp);

  qp_per = vidParam->qpPerMatrix[ curMb->qpScaled[curSlice->colourPlaneId] ];
  qp_rem = vidParam->qpRemMatrix[ curMb->qpScaled[curSlice->colourPlaneId] ];

  //init quant parameters for chroma
  for(i=0; i < 2; ++i) {
    qp_per_uv[i] = vidParam->qpPerMatrix[ curMb->qpScaled[i + 1] ];
    qp_rem_uv[i] = vidParam->qpRemMatrix[ curMb->qpScaled[i + 1] ];
  }

  InvLevelScale4x4 = intra? curSlice->InvLevelScale4x4_Intra[curSlice->colourPlaneId][qp_rem] : curSlice->InvLevelScale4x4_Inter[curSlice->colourPlaneId][qp_rem];

  // luma coefficients
  {
    //======= Other Modes & CABAC ========
    //------------------------------------
    if (cbp) {
      if(curMb->lumaTransformSize8x8flag) {
        //======= 8x8 transform size & CABAC ========
        curMb->read_comp_coeff_8x8_CABAC (curMb, &currSE, PLANE_Y);
      }
      else
        curMb->read_comp_coeff_4x4_CABAC (curMb, &currSE, PLANE_Y, InvLevelScale4x4, qp_per, cbp);
    }
  }

  //========================== CHROMA DC ============================
  //-----------------------------------------------------------------
  // chroma DC coeff
  if(cbp>15) {
    for (ll=0;ll<3;ll+=2) {
      int (*InvLevelScale4x4)[4] = NULL;
      uv = ll>>1;
      {
        int** imgcof = curSlice->cof[uv + 1];
        int m3[2][4] = {{0,0,0,0},{0,0,0,0}};
        int m4[2][4] = {{0,0,0,0},{0,0,0,0}};
        int qp_per_uv_dc = vidParam->qpPerMatrix[ (curMb->qpc[uv] + 3 + vidParam->bitdepthChromaQpScale) ];       //for YUV422 only
        int qp_rem_uv_dc = vidParam->qpRemMatrix[ (curMb->qpc[uv] + 3 + vidParam->bitdepthChromaQpScale) ];       //for YUV422 only
        if (intra)
          InvLevelScale4x4 = curSlice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv_dc];
        else
          InvLevelScale4x4 = curSlice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv_dc];


        //===================== CHROMA DC YUV422 ======================
        {
          sCBPStructure  *cbpStructure = &curMb->cbpStructure[0];
          coef_ctr=-1;
          level=1;
          for(k=0;(k<9)&&(level!=0);++k)
          {
            currSE.context      = CHROMA_DC_2x4;
            currSE.type         = ((curMb->isIntraBlock == TRUE) ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER);
            curMb->isVblock     = ll;

            dP = &(curSlice->partitions[partMap[currSE.type]]);

            if (dP->bitstream->eiFlag)
              currSE.mapping = linfo_levrun_c2x2;
            else
              currSE.reading = readRunLevel_CABAC;

            dP->readsSyntaxElement(curMb, &currSE, dP);

            level = currSE.value1;

            if (level != 0) {
              cbpStructure->blk |= ((int64)0xff0000) << (ll<<2) ;
              coef_ctr += currSE.value2 + 1;
              assert (coef_ctr < vidParam->numCdcCoeff);
              i0=SCAN_YUV422[coef_ctr][0];
              j0=SCAN_YUV422[coef_ctr][1];

              m3[i0][j0]=level;
            }
          }
        }
        // inverse CHROMA DC YUV422 transform
        // horizontal
        if(curMb->isLossless == FALSE)
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

          for(j = 0;j < vidParam->mbCrSizeY; j += BLOCK_SIZE)
          {
            for(i=0;i < vidParam->mbCrSizeX;i+=BLOCK_SIZE)
            {
              imgcof[j][i] = rshift_rnd_sf((imgcof[j][i] * InvLevelScale4x4[0][0]) << qp_per_uv_dc, 6);
            }
          }
        }
        else
        {
          for(j=0;j<4;++j)
          {
            for(i=0;i<2;++i)
            {
              curSlice->cof[uv + 1][j<<2][i<<2] = m3[i][j];
              //curSlice->fcf[uv + 1][j<<2][i<<2] = m3[i][j];
            }
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
  }
  else
  {
    {
      currSE.context      = CHROMA_AC;
      currSE.type         = (curMb->isIntraBlock ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER);

      dP = &(curSlice->partitions[partMap[currSE.type]]);

      if (dP->bitstream->eiFlag)
        currSE.mapping = linfo_levrun_inter;
      else
        currSE.reading = readRunLevel_CABAC;

      if(curMb->isLossless == FALSE)
      {
        sCBPStructure  *cbpStructure = &curMb->cbpStructure[0];
        for (b8=0; b8 < vidParam->numBlock8x8uv; ++b8)
        {
          curMb->isVblock = uv = (b8 > ((vidParam->numUvBlocks) - 1 ));
          InvLevelScale4x4 = intra ? curSlice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : curSlice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];

          for (b4 = 0; b4 < 4; ++b4)
          {
            i = cofuv_blk_x[yuv][b8][b4];
            j = cofuv_blk_y[yuv][b8][b4];

            curMb->subblockY = subblk_offset_y[yuv][b8][b4];
            curMb->subblockX = subblk_offset_x[yuv][b8][b4];

            pos_scan_4x4 = pos_scan4x4[1];
            level=1;

            for(k = 0; (k < 16) && (level != 0);++k)
            {
              dP->readsSyntaxElement(curMb, &currSE, dP);
              level = currSE.value1;

              if (level != 0)
              {
                cbpStructure->blk |= i64_power2(cbp_blk_chroma[b8][b4]);
                pos_scan_4x4 += (currSE.value2 << 1);

                i0 = *pos_scan_4x4++;
                j0 = *pos_scan_4x4++;

                curSlice->cof[uv + 1][(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((level * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
                //curSlice->fcf[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
              }
            } //for(k=0;(k<16)&&(level!=0);++k)
          }
        }
      }
      else
      {
        sCBPStructure  *cbpStructure = &curMb->cbpStructure[0];
        for (b8=0; b8 < vidParam->numBlock8x8uv; ++b8)
        {
          curMb->isVblock = uv = (b8 > ((vidParam->numUvBlocks) - 1 ));

          for (b4=0; b4 < 4; ++b4)
          {
            i = cofuv_blk_x[yuv][b8][b4];
            j = cofuv_blk_y[yuv][b8][b4];

            pos_scan_4x4 = pos_scan4x4[1];
            level=1;

            curMb->subblockY = subblk_offset_y[yuv][b8][b4];
            curMb->subblockX = subblk_offset_x[yuv][b8][b4];

            for(k=0;(k<16)&&(level!=0);++k)
            {
              dP->readsSyntaxElement(curMb, &currSE, dP);
              level = currSE.value1;

              if (level != 0)
              {
                cbpStructure->blk |= i64_power2(cbp_blk_chroma[b8][b4]);
                pos_scan_4x4 += (currSE.value2 << 1);

                i0 = *pos_scan_4x4++;
                j0 = *pos_scan_4x4++;

                curSlice->cof[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
                //curSlice->fcf[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
              }
            }
          }
        }
      } //for (b4=0; b4 < 4; b4++)
    } //for (b8=0; b8 < vidParam->numBlock8x8uv; b8++)
  } //if (picture->chromaFormatIdc != YUV400)
}

void set_read_CBP_and_coeffs_cabac(sSlice* curSlice)
{
  switch (curSlice->vidParam->activeSPS->chromaFormatIdc)
  {
  case YUV444:
    if (curSlice->vidParam->sepColourPlaneFlag == 0)
      curSlice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CABAC_444;
    else
      curSlice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CABAC_400;
    break;
  case YUV422:
    curSlice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CABAC_422;
    break;
  case YUV420:
    curSlice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CABAC_420;
    break;
  case YUV400:
    curSlice->readCBPcoeffs = read_CBP_and_coeffs_from_NAL_CABAC_400;
    break;
  default:
    assert (1);
    curSlice->readCBPcoeffs = NULL;
    break;
  }
}
//}}}

//{{{
void set_read_comp_coeff_cabac (sMacroblock* curMb) {

  if (curMb->isLossless == FALSE) {
    curMb->read_comp_coeff_4x4_CABAC = read_comp_coeff_4x4_CABAC;
    curMb->read_comp_coeff_8x8_CABAC = read_comp_coeff_8x8_MB_CABAC;
    }
  else {
    curMb->read_comp_coeff_4x4_CABAC = read_comp_coeff_4x4_CABAC_ls;
    curMb->read_comp_coeff_8x8_CABAC = read_comp_coeff_8x8_MB_CABAC_ls;
    }
  }
//}}}
