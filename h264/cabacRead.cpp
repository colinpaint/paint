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
