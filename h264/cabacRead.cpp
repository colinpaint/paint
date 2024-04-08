//{{{  includes
#include "global.h"

#include "cabac.h"
#include "macroblock.h"
#include "transform.h"
//}}}

namespace {
  //{{{
  void readCompCoef8x8Lossless (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane, int b8) {

    if (mb->codedBlockPattern & (1 << b8)) {
      // are there any coefficients in the current block
      cDecoder264* decoder = mb->decoder;
      cSlice* slice = mb->slice;

      int64_t cbp_mask = (int64_t) 51 << (4 * b8 - 2 * (b8 & 0x01)); // corresponds to 110011, as if all four 4x4 blocks contain coeff, shifted to block position
      int64_t *cur_cbp = &mb->codedBlockPatterns[plane].blk;

      // select scan type
      const uint8_t* pos_scan8x8 = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN8x8[0] : FIELD_SCAN8x8[0];

      // === set offset in current macroBlock ===
      int boff_x = (b8&0x01) << 3;
      int boff_y = (b8 >> 1) << 3;
      int** tcoeffs = &slice->mbRess[plane][boff_y];

      mb->subblockX = boff_x; // position for coeff_count ctx
      mb->subblockY = boff_y; // position for coeff_count ctx

      sDataPartition* dataPartition;
      const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
      if (plane == PLANE_Y || (decoder->coding.isSeperateColourPlane != 0))
        se->context = LUMA_8x8;
      else if (plane==PLANE_U)
        se->context = CB_8x8;
      else
        se->context = CR_8x8;
      se->reading = readRunLevelCabac;

      int level = 1;
      for (int k = 0; (k < 65) && (level != 0);++k) {
        // make distinction between INTRA and INTER codedluminance coefficients
        se->type  = ((mb->isIntraBlock == 1) ? (k == 0 ? SE_LUM_DC_INTRA : SE_LUM_AC_INTRA)
                                             : (k == 0 ? SE_LUM_DC_INTER : SE_LUM_AC_INTER));
        dataPartition = &(slice->dataPartitions[dpMap[se->type]]);
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
  //{{{
  void readCompCoefx4smb (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane,
                               int blockY, int blockX, int start_scan, int64_t *cbp_blk) {

    int i,j,k;
    int i0, j0;
    int level = 1;

    cSlice* slice = mb->slice;
    sDataPartition* dataPartition;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];
    int** cof = slice->cof[plane];

    for (j = blockY; j < blockY + BLOCK_SIZE_8x8; j += 4) {
      mb->subblockY = j; // position for coeff_count ctx
      for (i = blockX; i < blockX + BLOCK_SIZE_8x8; i += 4) {
        mb->subblockX = i; // position for coeff_count ctx
        pos_scan_4x4 = pos_scan4x4[start_scan];
        level = 1;

        if (start_scan == 0) {
          // make distinction between INTRA and INTER coded luminance coefficients
          se->type = (mb->isIntraBlock ? SE_LUM_DC_INTRA : SE_LUM_DC_INTER);
          dataPartition = &(slice->dataPartitions[dpMap[se->type]]);
          if (dataPartition->bitStream.errorFlag)
            se->mapping = cBitStream::linfo_levrun_inter;
          else
            se->reading = readRunLevelCabac;
          dataPartition->readSyntaxElement (mb, se, dataPartition);
          level = se->value1;
          if (level != 0) {
             /* leave if level == 0 */
            pos_scan_4x4 += 2 * se->value2;
            i0 = *pos_scan_4x4++;
            j0 = *pos_scan_4x4++;
            *cbp_blk |= i64power2(j + (i >> 2)) ;
            cof[j + j0][i + i0]= level;
            }
          }

        if (level != 0) {
          // make distinction between INTRA and INTER coded luminance coefficients
          se->type = (mb->isIntraBlock ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER);
          dataPartition = &(slice->dataPartitions[dpMap[se->type]]);
          if (dataPartition->bitStream.errorFlag)
            se->mapping = cBitStream::linfo_levrun_inter;
          else
            se->reading = readRunLevelCabac;

          for (k = 1; (k < 17) && (level != 0); ++k) {
            dataPartition->readSyntaxElement (mb, se, dataPartition);
            level = se->value1;
            if (level != 0) {   /* leave if level == 0 */
              pos_scan_4x4 += 2 * se->value2;
              i0 = *pos_scan_4x4++;
              j0 = *pos_scan_4x4++;
              cof[j + j0][i + i0] = level;
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

    if( plane == PLANE_Y || (decoder->coding.isSeperateColourPlane != 0) )
      se->context = (IS_I16MB(mb) ? LUMA_16AC: LUMA_4x4);
    else if (plane == PLANE_U)
      se->context = (IS_I16MB(mb) ? CB_16AC: CB_4x4);
    else
      se->context = (IS_I16MB(mb) ? CR_16AC: CR_4x4);

    int64_t* cbp_blk = &mb->codedBlockPatterns[plane].blk;
    for (int blockY = 0; blockY < MB_BLOCK_SIZE; blockY += BLOCK_SIZE_8x8) /* all modes */
      for (int blockX = 0; blockX < MB_BLOCK_SIZE; blockX += BLOCK_SIZE_8x8)
        if (codedBlockPattern & (1 << ((blockY >> 2) + (blockX >> 3))))  // are there any coeff in current block at all
          readCompCoefx4smb (mb, se, plane, blockY, blockX, start_scan, cbp_blk);
    }
  //}}}
  //{{{
  void readCompCoef4x4 (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane,
                                    int (*InvLevelScale4x4)[4], int qp_per, int codedBlockPattern) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int start_scan = IS_I16MB (mb)? 1 : 0;
    int blockY, blockX;
    int i, j;
    int64_t *cbp_blk = &mb->codedBlockPatterns[plane].blk;

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
          readCompCoefx4smb (mb, se, plane, blockY, blockX, start_scan, cbp_blk);
          if (start_scan == 0) {
            for (j = 0; j < BLOCK_SIZE_8x8; ++j) {
              int* coef = &cof[j][blockX];
              int jj = j & 0x03;
              for (i = 0; i < BLOCK_SIZE_8x8; i+=4) {
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
            for (j = 0; j < BLOCK_SIZE_8x8; ++j) {
              int* coef = &cof[j][blockX];
              int jj = j & 0x03;
              for (i = 0; i < BLOCK_SIZE_8x8; i += 4) {
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
  void readCompCoef8x8 (sMacroBlock* mb, sSyntaxElement* se, eColorPlane plane, int b8) {

    if (mb->codedBlockPattern & (1 << b8)) {
      // are there any coefficients in the current block
      cDecoder264* decoder = mb->decoder;
      int transform_pl = (decoder->coding.isSeperateColourPlane != 0) ? mb->slice->colourPlaneId : plane;

      int level = 1;

      cSlice* slice = mb->slice;

      int64_t cbp_mask = (int64_t) 51 << (4 * b8 - 2 * (b8 & 0x01)); // corresponds to 110011, as if all four 4x4 blocks contain coeff, shifted to block position
      int64_t* cur_cbp = &mb->codedBlockPatterns[plane].blk;

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

      const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

      if (plane == PLANE_Y || (decoder->coding.isSeperateColourPlane != 0))
        se->context = LUMA_8x8;
      else if (plane == PLANE_U)
        se->context = CB_8x8;
      else
        se->context = CR_8x8;
      se->reading = readRunLevelCabac;

      // read DC
      se->type = ((mb->isIntraBlock == 1) ? SE_LUM_DC_INTRA : SE_LUM_DC_INTER ); // Intra or Inter?
      sDataPartition* dataPartition = &slice->dataPartitions[dpMap[se->type]];
      dataPartition->readSyntaxElement(mb, se, dataPartition);
      level = se->value1;
      if (level != 0) {
        *cur_cbp |= cbp_mask;
        pos_scan8x8 += 2 * (se->value2);
        int i = *pos_scan8x8++;
        int j = *pos_scan8x8++;
        tcoeffs[j][boff_x + i] = rshift_rnd_sf((level * InvLevelScale8x8[j][i]) << qp_per, 6); // dequantization

        // read AC
        se->type = ((mb->isIntraBlock == 1) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER);
        dataPartition = &slice->dataPartitions[dpMap[se->type]];
        for (int k = 1; (k < 65) && (level != 0);++k) {
          dataPartition->readSyntaxElement(mb, se, dataPartition);
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
  void readCbpCoefs400 (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int k;
    int level;
    int codedBlockPattern;
    const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int i0, j0;
    int qp_per, qp_rem;
    int intra = (mb->isIntraBlock == true);
    int need_transform_sizeFlag;
    int (*InvLevelScale4x4)[4] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    sSyntaxElement se;
    sDataPartition *dataPartition = NULL;
    if (!IS_I16MB (mb)) {
      //=====   C B P   =====
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? SE_CBP_INTRA
        : SE_CBP_INTER;

      dataPartition = &(slice->dataPartitions[dpMap[se.type]]);

      if (dataPartition->bitStream.errorFlag) {
        se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
          ? slice->linfoCbpIntra
          : slice->linfoCbpInter;
        }
      else
        se.reading = readCbpCabac;

      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      //============= Transform size flag for INTER MBs =============
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type =  SE_HEADER;
        dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
        se.reading = readMbTransformSizeFlagCabac;

        // read eCavlc transform_size_8x8Flag
        if (dataPartition->bitStream.errorFlag) {
          se.len = 1;
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          }
        else
          dataPartition->readSyntaxElement(mb, &se, dataPartition);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }

      // Delta quant only if nonzero coeffs
      if (codedBlockPattern != 0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);
        if (slice->dataPartitionMode)  {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if (intra && slice->noDataPartitionB)  {
            mb->errorFlag = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
          }
        }
      }
    else {
      //{{{  read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);

      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->errorFlag  = 1;
          mb->dplFlag = 1;
         }
         checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];

        {
          se.type = SE_LUM_DC_INTRA;
          dataPartition = &(slice->dataPartitions[dpMap[se.type]]);

          se.context      = LUMA_16DC;
          se.type         = SE_LUM_DC_INTRA;

          if (dataPartition->bitStream.errorFlag)
            se.mapping = cBitStream::linfo_levrun_inter;
          else
            se.reading = readRunLevelCabac;

          level = 1;                            // just to get inside the loop

          for(k = 0; (k < 17) && (level != 0); ++k) {
            dataPartition->readSyntaxElement(mb, &se, dataPartition);
            level = se.value1;

            if (level != 0) {
              /* leave if level == 0 */
              pos_scan_4x4 += (2 * se.value2);
              i0 = ((*pos_scan_4x4++) << 2);
              j0 = ((*pos_scan_4x4++) << 2);
              slice->cof[0][j0][i0] = level;// add new intra DC coeff
              }
            }
          }
        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }
      //}}}

    updateQp (mb, slice->qp);

    qp_per = decoder->qpPerMatrix[ mb->qpScaled[PLANE_Y] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[PLANE_Y] ];

    if (codedBlockPattern)  {
      if (mb->lumaTransformSize8x8flag) // 8x8 transform size & eCabac
        mb->readCompCoef8x8cabac (mb, &se, PLANE_Y);
      else {
        InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
        mb->readCompCoef4x4cabac (mb, &se, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern);
        }
      }
    }
  //}}}
  //{{{
  void readCbpCoefs444 (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int i, k;
    int level;
    int codedBlockPattern;
    const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int coef_ctr, i0, j0;
    int qp_per, qp_rem;
    int uv;
    int qp_per_uv[2];
    int qp_rem_uv[2];
    int intra = (mb->isIntraBlock == true);
    int need_transform_sizeFlag;
    int (*InvLevelScale4x4)[4] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // QPI init constants for every chroma qp offset
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    // read CBP if not new intra mode
    sSyntaxElement se;
    sDataPartition* dataPartition = NULL;
    if (!IS_I16MB (mb)) {
      //=====   C B P   =====
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? SE_CBP_INTRA
        : SE_CBP_INTER;

      dataPartition = &(slice->dataPartitions[dpMap[se.type]]);

      if (dataPartition->bitStream.errorFlag) {
        se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
          ? slice->linfoCbpIntra
          : slice->linfoCbpInter;
        }
      else
        se.reading = readCbpCabac;

      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      // Transform size flag for INTER MBs
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type =  SE_HEADER;
        dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
        se.reading = readMbTransformSizeFlagCabac;

        // read eCavlc transform_size_8x8Flag
        if (dataPartition->bitStream.errorFlag) {
          se.len = 1;
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          }
        else
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }

      if (codedBlockPattern !=0 ) {
        readDeltaQuant (&se, dataPartition, mb, dpMap,
                        ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if (intra && slice->noDataPartitionB ) {
            mb->errorFlag = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
         }
        }
      }
    else {
      // read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);
      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->errorFlag  = 1;
          mb->dplFlag = 1;
          }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];

        {
          se.type = SE_LUM_DC_INTRA;
          dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
          se.context = LUMA_16DC;
          se.type = SE_LUM_DC_INTRA;

          if (dataPartition->bitStream.errorFlag)
            se.mapping = cBitStream::linfo_levrun_inter;
          else
            se.reading = readRunLevelCabac;

          level = 1;                            // just to get inside the loop
          for (k = 0; (k < 17) && (level != 0); ++k) {
            dataPartition->readSyntaxElement (mb, &se, dataPartition);
            level = se.value1;

            if (level != 0) {
              pos_scan_4x4 += (2 * se.value2);
              i0 = ((*pos_scan_4x4++) << 2);
              j0 = ((*pos_scan_4x4++) << 2);
              slice->cof[0][j0][i0] = level;// add new intra DC coeff
              }
            }
          }

        if (mb->isLossless == false)
          itrans2 (mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }

    updateQp (mb, slice->qp);

    qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

    //init quant parameters for chroma
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
      {
      if (codedBlockPattern) {
        if (mb->lumaTransformSize8x8flag) // 8x8 transform size & eCabac
          mb->readCompCoef8x8cabac (mb, &se, PLANE_Y);
        else
          mb->readCompCoef4x4cabac (mb, &se, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern);
        }
      }

    for (uv = 0; uv < 2; ++uv ) {
      // 16x16DC Luma_Add
      if (IS_I16MB (mb)) {
        //{{{  read DC coeffs for new intra modes
        {
          se.type = SE_LUM_DC_INTRA;
          dataPartition = &(slice->dataPartitions[dpMap[se.type]]);

          if( (decoder->coding.isSeperateColourPlane != 0) )
            se.context = LUMA_16DC;
          else
            se.context = (uv==0) ? CB_16DC : CR_16DC;

          if (dataPartition->bitStream.errorFlag)
            se.mapping = cBitStream::linfo_levrun_inter;
          else
            se.reading = readRunLevelCabac;

          coef_ctr = -1;
          level = 1;                            // just to get inside the loop

          for(k=0;(k<17) && (level!=0);++k) {
            dataPartition->readSyntaxElement(mb, &se, dataPartition);
            level = se.value1;

            if (level != 0) {
              coef_ctr += se.value2 + 1;
              i0 = pos_scan4x4[coef_ctr][0];
              j0 = pos_scan4x4[coef_ctr][1];
              slice->cof[uv + 1][j0<<2][i0<<2] = level;
              }
            } //k loop
          } // else eCavlc

        if(mb->isLossless == false)
          itrans2(mb, (eColorPlane) (uv + 1)); // transform new intra DC
        } //IS_I16MB
        //}}}

      updateQp (mb, slice->qp);

      qp_per = decoder->qpPerMatrix[ (slice->qp + decoder->coding.bitDepthLumaQpScale) ];
      qp_rem = decoder->qpRemMatrix[ (slice->qp + decoder->coding.bitDepthLumaQpScale) ];

      //init constants for every chroma qp offset
      qp_per_uv[uv] = decoder->qpPerMatrix[ (mb->qpc[uv] + decoder->coding.bitDepthChromaQpScale) ];
      qp_rem_uv[uv] = decoder->qpRemMatrix[ (mb->qpc[uv] + decoder->coding.bitDepthChromaQpScale) ];

      InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];

        {
        if (codedBlockPattern) {
          if (mb->lumaTransformSize8x8flag) // 8x8 transform size & eCabac
            mb->readCompCoef8x8cabac (mb, &se, (eColorPlane) (PLANE_U + uv));
          else //4x4
            mb->readCompCoef4x4cabac (mb, &se, (eColorPlane) (PLANE_U + uv), InvLevelScale4x4,  qp_per_uv[uv], codedBlockPattern);
          }
        }
      }
    }
  //}}}
  //{{{
  void readCbpCoefs422 (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int i,j,k;
    int level;
    int codedBlockPattern;
    const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int coef_ctr, i0, j0, b8;
    int ll;
    int qp_per, qp_rem;
    int uv;
    int qp_per_uv[2];
    int qp_rem_uv[2];
    int intra = (mb->isIntraBlock == true);
    int b4;
    sPicture* picture = slice->picture;
    int yuv = picture->chromaFormatIdc - 1;
    int m6[4];
    int need_transform_sizeFlag;
    int (*InvLevelScale4x4)[4] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // QPI init constants for every chroma qp offset
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    // read CBP if not new intra mode
    sSyntaxElement se;
    sDataPartition *dataPartition = NULL;
    if (!IS_I16MB (mb)) {
      // codedBlockPattern
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
                  ? SE_CBP_INTRA
                  : SE_CBP_INTER;
      dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
      if (dataPartition->bitStream.errorFlag) {
        se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
                       ? slice->linfoCbpIntra
                       : slice->linfoCbpInter;
        }
      else
        se.reading = readCbpCabac;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      // Transform size flag for INTER MBs
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type = SE_HEADER;
        dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
        se.reading = readMbTransformSizeFlagCabac;

        // read eCavlc transform_size_8x8Flag
        if (dataPartition->bitStream.errorFlag) {
          se.len = 1;
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          }
        else
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }

      // Delta quant only if nonzero coeffs
      if (codedBlockPattern != 0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);
        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC)
            mb->dplFlag = 1;
          if (intra && slice->noDataPartitionB) {
            mb->errorFlag = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
          }
        }
      }
    else {
      //{{{  read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);

      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->errorFlag = 1;
          mb->dplFlag = 1;
          }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];

        {
          se.type = SE_LUM_DC_INTRA;
          dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
          se.context = LUMA_16DC;
          se.type = SE_LUM_DC_INTRA;

          if (dataPartition->bitStream.errorFlag)
            se.mapping = cBitStream::linfo_levrun_inter;
          else
            se.reading = readRunLevelCabac;

          level = 1; // just to get inside the loop
          for (k = 0; (k < 17) && (level != 0); ++k) {
            dataPartition->readSyntaxElement(mb, &se, dataPartition);
            level = se.value1;

            if (level != 0) {
              pos_scan_4x4 += (2 * se.value2);
              i0 = ((*pos_scan_4x4++) << 2);
              j0 = ((*pos_scan_4x4++) << 2);
              slice->cof[0][j0][i0] = level; // add new intra DC coeff
              }
            }
          }

        if (mb->isLossless == false)
          itrans2 (mb, (eColorPlane) slice->colourPlaneId); // transform new intra DC
        }
      }
      //}}}

    updateQp (mb, slice->qp);

    qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

    // init quant parameters for chroma
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
    if (codedBlockPattern) {
      if (mb->lumaTransformSize8x8flag)
        mb->readCompCoef8x8cabac (mb, &se, PLANE_Y);
      else
        mb->readCompCoef4x4cabac (mb, &se, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern);
      }

    // chroma DC coeff
    if (codedBlockPattern > 15) {
      for (ll = 0; ll < 3; ll += 2) {
        int (*InvLevelScale4x4)[4] = NULL;
        uv = ll>>1;
        {
          int** imgcof = slice->cof[uv + 1];
          int m3[2][4] = {{0,0,0,0},{0,0,0,0}};
          int m4[2][4] = {{0,0,0,0},{0,0,0,0}};
          int qp_per_uv_dc = decoder->qpPerMatrix[ (mb->qpc[uv] + 3 + decoder->coding.bitDepthChromaQpScale) ];       //for YUV422 only
          int qp_rem_uv_dc = decoder->qpRemMatrix[ (mb->qpc[uv] + 3 + decoder->coding.bitDepthChromaQpScale) ];       //for YUV422 only
          if (intra)
            InvLevelScale4x4 = slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv_dc];
          else
            InvLevelScale4x4 = slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv_dc];


          // CHROMA DC YUV422
          {
            sCodedBlockPattern* codedBlockPatterns = &mb->codedBlockPatterns[0];
            coef_ctr = -1;
            level = 1;
            for (k = 0; (k < 9) && (level != 0); ++k) {
              se.context = CHROMA_DC_2x4;
              se.type = ((mb->isIntraBlock == true) ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER);
              mb->isVblock = ll;

              dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
              if (dataPartition->bitStream.errorFlag)
                se.mapping = cBitStream::linfo_levrun_c2x2;
              else
                se.reading = readRunLevelCabac;
              dataPartition->readSyntaxElement(mb, &se, dataPartition);

              level = se.value1;
              if (level != 0) {
                codedBlockPatterns->blk |= ((int64_t)0xff0000) << (ll<<2) ;
                coef_ctr += se.value2 + 1;
                i0 = SCAN_YUV422[coef_ctr][0];
                j0 = SCAN_YUV422[coef_ctr][1];

                m3[i0][j0]=level;
                }
              }
            }
          //{{{  inverse CHROMA DC YUV422 transform  horizontal
          if (mb->isLossless == false) {
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
              }

            for (int j = 0;j < decoder->mbCrSizeY; j += BLOCK_SIZE)
              for (int i = 0; i < decoder->mbCrSizeX;i+=BLOCK_SIZE)
                imgcof[j][i] = rshift_rnd_sf((imgcof[j][i] * InvLevelScale4x4[0][0]) << qp_per_uv_dc, 6);
            }

          else
            for (int j = 0; j < 4; ++j)
              for (int i = 0; i < 2; ++i)
                slice->cof[uv + 1][j<<2][i<<2] = m3[i][j];
                     //}}}
          }
        }
      }

    //{{{  chroma AC coeff, all zero fram start_scan
    if (codedBlockPattern <= 31) {
      }
    else {
      {
        se.context = CHROMA_AC;
        se.type = (mb->isIntraBlock ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER);
        dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
        if (dataPartition->bitStream.errorFlag)
          se.mapping = cBitStream::linfo_levrun_inter;
        else
          se.reading = readRunLevelCabac;

        if (mb->isLossless == false) {
          sCodedBlockPattern* codedBlockPatterns = &mb->codedBlockPatterns[0];
          for (b8 = 0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
            uv = b8 > (decoder->coding.numUvBlocks - 1);
            mb->isVblock = uv;
            InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];
            for (b4 = 0; b4 < 4; ++b4) {
              i = cofuv_blk_x[yuv][b8][b4];
              j = cofuv_blk_y[yuv][b8][b4];
              mb->subblockY = subblk_offset_y[yuv][b8][b4];
              mb->subblockX = subblk_offset_x[yuv][b8][b4];
              pos_scan_4x4 = pos_scan4x4[1];

              level = 1;
              for(k = 0; (k < 16) && (level != 0);++k) {
                dataPartition->readSyntaxElement(mb, &se, dataPartition);
                level = se.value1;

                if (level != 0) {
                  codedBlockPatterns->blk |= i64power2(cbp_blk_chroma[b8][b4]);
                  pos_scan_4x4 += (se.value2 << 1);
                  i0 = *pos_scan_4x4++;
                  j0 = *pos_scan_4x4++;
                  slice->cof[uv + 1][(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((level * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
                  }
                }
              }
           }
          }
        else {
          sCodedBlockPattern* codedBlockPatterns = &mb->codedBlockPatterns[0];
          for (b8 = 0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
            uv = b8 > (decoder->coding.numUvBlocks - 1);
            mb->isVblock = uv;
            for (b4=0; b4 < 4; ++b4) {
              i = cofuv_blk_x[yuv][b8][b4];
              j = cofuv_blk_y[yuv][b8][b4];

              pos_scan_4x4 = pos_scan4x4[1];
              level=1;

              mb->subblockY = subblk_offset_y[yuv][b8][b4];
              mb->subblockX = subblk_offset_x[yuv][b8][b4];

              for(k = 0; (k < 16) && (level != 0); ++k) {
                dataPartition->readSyntaxElement (mb, &se, dataPartition);
                level = se.value1;
                if (level != 0) {
                  codedBlockPatterns->blk |= i64power2(cbp_blk_chroma[b8][b4]);
                  pos_scan_4x4 += (se.value2 << 1);
                  i0 = *pos_scan_4x4++;
                  j0 = *pos_scan_4x4++;
                  slice->cof[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
                  }
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
  void readCbpCoefs420 (sMacroBlock* mb) {

    int i,j;
    int level;
    int codedBlockPattern;
    cSlice* slice = mb->slice;
    const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int i0, j0;

    int qp_per, qp_rem;
    cDecoder264* decoder = mb->decoder;
    int smb = ((decoder->coding.sliceType == eSliceSP) && (mb->isIntraBlock == false)) ||
               (decoder->coding.sliceType == eSliceSI && mb->mbType == SI4MB);

    int qp_per_uv[2];
    int qp_rem_uv[2];

    int intra = (mb->isIntraBlock == true);

    sPicture* picture = slice->picture;
    int yuv = picture->chromaFormatIdc - 1;

    int (*InvLevelScale4x4)[4] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    sDataPartition* dataPartition = NULL;
    sSyntaxElement se;
    if (!IS_I16MB (mb)) {
      int need_transform_sizeFlag;
      // CBP
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? SE_CBP_INTRA
        : SE_CBP_INTER;

      dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
      if (dataPartition->bitStream.errorFlag) {
        se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
          ? slice->linfoCbpIntra
          : slice->linfoCbpInter;
        }
      else
        se.reading = readCbpCabac;
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      // Transform size flag for INTER MBs
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type   =  SE_HEADER;
        dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
        se.reading = readMbTransformSizeFlagCabac;

        // read eCavlc transform_size_8x8Flag
        if (dataPartition->bitStream.errorFlag) {
          se.len = 1;
          dataPartition->bitStream.readSyntaxElement_FLC (&se);
          }
        else
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }

      //{{{  Delta quant only if nonzero coeffs
      if (codedBlockPattern !=0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);
        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;
          if (intra && slice->noDataPartitionB ) {
            mb->errorFlag = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
          }
        }
      //}}}
      }
    else {
      //{{{  read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant(&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);
      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->errorFlag  = 1;
          mb->dplFlag = 1;
        }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }
      //}}}

      if (!mb->dplFlag) {
        int** cof = slice->cof[0];
        pos_scan_4x4 = pos_scan4x4[0];

        se.type = SE_LUM_DC_INTRA;
        dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
        se.context = LUMA_16DC;
        se.type = SE_LUM_DC_INTRA;
        if (dataPartition->bitStream.errorFlag)
          se.mapping = cBitStream::linfo_levrun_inter;
        else
          se.reading = readRunLevelCabac;

        level = 1;                            // just to get inside the loop
        for (int k = 0; (k < 17) && (level != 0); ++k) {
          dataPartition->readSyntaxElement(mb, &se, dataPartition);
          level = se.value1;
          if (level != 0) {
            pos_scan_4x4 += (2 * se.value2);
            i0 = ((*pos_scan_4x4++) << 2);
            j0 = ((*pos_scan_4x4++) << 2);
            cof[j0][i0] = level;// add new intra DC coeff
            }
          }
        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }

    updateQp (mb, slice->qp);
    qp_per = decoder->qpPerMatrix[mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[mb->qpScaled[slice->colourPlaneId] ];

    // luma coefficients
    if (codedBlockPattern) {
      if(mb->lumaTransformSize8x8flag) // 8x8 transform size & eCabac
        mb->readCompCoef8x8cabac (mb, &se, PLANE_Y);
      else {
        InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
        mb->readCompCoef4x4cabac (mb, &se, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern);
        }
      }

    // init quant parameters for chroma
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }
    //{{{  chroma DC coeff
    if (codedBlockPattern > 15) {
      sCodedBlockPattern* codedBlockPatterns = &mb->codedBlockPatterns[0];
      int uv, ll, k, coef_ctr;

      for (ll = 0; ll < 3; ll += 2) {
        uv = ll >> 1;
        InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];

        // CHROMA DC YUV420
        slice->cofu[0] = slice->cofu[1] = slice->cofu[2] = slice->cofu[3] = 0;
        coef_ctr = -1;

        level = 1;
        mb->isVblock = ll;
        se.context = CHROMA_DC;
        se.type = (intra ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER);
        dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
        if (dataPartition->bitStream.errorFlag)
          se.mapping = cBitStream::linfo_levrun_c2x2;
        else
          se.reading = readRunLevelCabac;

        for (k = 0; (k < (decoder->coding.numCdcCoeff + 1)) && (level != 0); ++k) {
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
          level = se.value1;
          if (level != 0) {
            codedBlockPatterns->blk |= 0xf0000 << (ll<<1) ;
            coef_ctr += se.value2 + 1;
            // Bug: slice->cofu has only 4 entries, hence coef_ctr MUST be <4 (which is
            // caught by the assert().  If it is bigger than 4, it starts patching the
            // decoder->predmode pointer, which leads to bugs later on.
            // This assert() should be left in the code, because it captures a very likely
            // bug early when testing in error prone environments (or when testing NALfunctionality).
            slice->cofu[coef_ctr] = level;
            }
          }


        if (smb || (mb->isLossless == true)) {
          //{{{  check to see if MB type is SPred or SIntra4x4
          slice->cof[uv + 1][0][0] = slice->cofu[0];
          slice->cof[uv + 1][0][4] = slice->cofu[1];
          slice->cof[uv + 1][4][0] = slice->cofu[2];
          slice->cof[uv + 1][4][4] = slice->cofu[3];
          }
          //}}}
        else {
          int temp[4];
          int scale_dc = InvLevelScale4x4[0][0];
          int** cof = slice->cof[uv + 1];
          ihadamard2x2(slice->cofu, temp);
          cof[0][0] = (((temp[0] * scale_dc) << qp_per_uv[uv]) >> 5);
          cof[0][4] = (((temp[1] * scale_dc) << qp_per_uv[uv]) >> 5);
          cof[4][0] = (((temp[2] * scale_dc) << qp_per_uv[uv]) >> 5);
          cof[4][4] = (((temp[3] * scale_dc) << qp_per_uv[uv]) >> 5);
          }
        }
      }
    //}}}
    //{{{  chroma AC coeff, all zero fram start_scan
    if (codedBlockPattern > 31) {
      se.context = CHROMA_AC;
      se.type = (mb->isIntraBlock ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER);
      dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
      if (dataPartition->bitStream.errorFlag)
        se.mapping = cBitStream::linfo_levrun_inter;
      else
        se.reading = readRunLevelCabac;

      if (mb->isLossless == false) {
        int b4, b8, uv, k;
        int** cof;
        sCodedBlockPattern  *codedBlockPatterns = &mb->codedBlockPatterns[0];
        for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];
          cof = slice->cof[uv + 1];

          for (b4 = 0; b4 < 4; ++b4) {
            i = cofuv_blk_x[yuv][b8][b4];
            j = cofuv_blk_y[yuv][b8][b4];

            mb->subblockY = subblk_offset_y[yuv][b8][b4];
            mb->subblockX = subblk_offset_x[yuv][b8][b4];

            pos_scan_4x4 = pos_scan4x4[1];
            level = 1;

            for(k = 0; (k < 16) && (level != 0);++k) {
              dataPartition->readSyntaxElement(mb, &se, dataPartition);
              level = se.value1;

              if (level != 0) {
                codedBlockPatterns->blk |= i64power2(cbp_blk_chroma[b8][b4]);
                pos_scan_4x4 += (se.value2 << 1);

                i0 = *pos_scan_4x4++;
                j0 = *pos_scan_4x4++;

                cof[(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((level * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
                }
              }
            }
          }
        }
      else {
        sCodedBlockPattern* codedBlockPatterns = &mb->codedBlockPatterns[0];
        int b4, b8, k;
        int uv;
        for (b8 = 0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          for (b4 = 0; b4 < 4; ++b4) {
            i = cofuv_blk_x[yuv][b8][b4];
            j = cofuv_blk_y[yuv][b8][b4];
            pos_scan_4x4 = pos_scan4x4[1];
            level = 1;
            mb->subblockY = subblk_offset_y[yuv][b8][b4];
            mb->subblockX = subblk_offset_x[yuv][b8][b4];
            for (k = 0; (k < 16) && (level != 0); ++k) {
              dataPartition->readSyntaxElement(mb, &se, dataPartition);
              level = se.value1;

              if (level != 0) {
                codedBlockPatterns->blk |= i64power2(cbp_blk_chroma[b8][b4]);
                pos_scan_4x4 += (se.value2 << 1);
                i0 = *pos_scan_4x4++;
                j0 = *pos_scan_4x4++;
                slice->cof[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
                }
              }
            }
          }
        }
      }
    //}}}
    }
  //}}}
  }

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
void cSlice::setReadCbpCoefsCabac() {

  switch (decoder->activeSps->chromaFormatIdc) {
    case YUV444:
      if (decoder->coding.isSeperateColourPlane == 0)
        readCBPcoeffs = readCbpCoefs444;
      else
        readCBPcoeffs = readCbpCoefs400;
      break;

    case YUV422:
      readCBPcoeffs = readCbpCoefs422;
      break;

    case YUV420:
      readCBPcoeffs = readCbpCoefs420;
      break;

    case YUV400:
      readCBPcoeffs = readCbpCoefs400;
      break;

    default:
      readCBPcoeffs = NULL;
      break;
    }
  }
//}}}
