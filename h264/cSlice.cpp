//{{{  includes
#include "global.h"
#include "memory.h"

#include "cabac.h"
#include "macroblock.h"
#include "mcPred.h"
#include "transform.h"

#include "../common/cLog.h"

using namespace std;
//}}}

namespace {
  //{{{
  int quant_org[16] = {
  16,16,16,16,
  16,16,16,16,
  16,16,16,16,
  16,16,16,16
  };
  //}}}
  //{{{
  int quant8_org[64] = {
  16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16
  };
  //}}}
  //{{{
  int quant_intra_default[16] = {
     6,13,20,28,
    13,20,28,32,
    20,28,32,37,
    28,32,37,42
  };
  //}}}
  //{{{
  int quant_inter_default[16] = {
    10,14,20,24,
    14,20,24,27,
    20,24,27,30,
    24,27,30,34
  };
  //}}}
  //{{{
  int quant8_intra_default[64] = {
   6,10,13,16,18,23,25,27,
  10,11,16,18,23,25,27,29,
  13,16,18,23,25,27,29,31,
  16,18,23,25,27,29,31,33,
  18,23,25,27,29,31,33,36,
  23,25,27,29,31,33,36,38,
  25,27,29,31,33,36,38,40,
  27,29,31,33,36,38,40,42
  };
  //}}}
  //{{{
  int quant8_inter_default[64] = {
   9,13,15,17,19,21,22,24,
  13,13,17,19,21,22,24,25,
  15,17,19,21,22,24,25,27,
  17,19,21,22,24,25,27,28,
  19,21,22,24,25,27,28,30,
  21,22,24,25,27,28,30,32,
  22,24,25,27,28,30,32,33,
  24,25,27,28,30,32,33,35
  };
  //}}}

  //{{{
  void setDequant4x4 (int (*InvLevelScale4x4)[4], const int (*dequant)[4], int* qmatrix) {

    for (int j = 0; j < 4; j++) {
      *(*InvLevelScale4x4      ) = *(*dequant      ) * *qmatrix++;
      *(*InvLevelScale4x4   + 1) = *(*dequant   + 1) * *qmatrix++;
      *(*InvLevelScale4x4   + 2) = *(*dequant   + 2) * *qmatrix++;
      *(*InvLevelScale4x4++ + 3) = *(*dequant++ + 3) * *qmatrix++;
      }
    }
  //}}}
  //{{{
  void setDequant8x8 (int (*InvLevelScale8x8)[8], const int (*dequant)[8], int* qmatrix) {

    for (int j = 0; j < 8; j++) {
      *(*InvLevelScale8x8      ) = *(*dequant      ) * *qmatrix++;
      *(*InvLevelScale8x8   + 1) = *(*dequant   + 1) * *qmatrix++;
      *(*InvLevelScale8x8   + 2) = *(*dequant   + 2) * *qmatrix++;
      *(*InvLevelScale8x8   + 3) = *(*dequant   + 3) * *qmatrix++;
      *(*InvLevelScale8x8   + 4) = *(*dequant   + 4) * *qmatrix++;
      *(*InvLevelScale8x8   + 5) = *(*dequant   + 5) * *qmatrix++;
      *(*InvLevelScale8x8   + 6) = *(*dequant   + 6) * *qmatrix++;
      *(*InvLevelScale8x8++ + 7) = *(*dequant++ + 7) * *qmatrix++;
      }
    }
  //}}}

  //{{{
  void readCbp400cabac (sMacroBlock* mb) {

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
        dataPartition = &slice->dataPartitions[dpMap[SE_HEADER]];
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
  void readCbp444cabac (sMacroBlock* mb) {

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
  void readCbp422cabac (sMacroBlock* mb) {

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
            sCodedBlockPattern* cbp = &mb->cbp[0];
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
                cbp->blk |= ((int64_t)0xff0000) << (ll<<2) ;
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
          sCodedBlockPattern* cbp = &mb->cbp[0];
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
                  cbp->blk |= i64power2(cbp_blk_chroma[b8][b4]);
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
          sCodedBlockPattern* cbp = &mb->cbp[0];
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
                  cbp->blk |= i64power2(cbp_blk_chroma[b8][b4]);
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
  void readCbp420cabac (sMacroBlock* mb) {

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
        se.type = SE_HEADER;
        dataPartition = &slice->dataPartitions[dpMap[SE_HEADER]];
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
      sCodedBlockPattern* cbp = &mb->cbp[0];
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
            cbp->blk |= 0xf0000 << (ll<<1) ;
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
        sCodedBlockPattern  *cbp = &mb->cbp[0];
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
                cbp->blk |= i64power2(cbp_blk_chroma[b8][b4]);
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
        sCodedBlockPattern* cbp = &mb->cbp[0];
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
                cbp->blk |= i64power2(cbp_blk_chroma[b8][b4]);
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

  //{{{
  void readCbp400cavlc (sMacroBlock* mb) {

    int k;
    int mb_nr = mb->mbIndexX;
    int codedBlockPattern;

    cSlice* slice = mb->slice;
    int i0, j0;

    int levarr[16], runarr[16], numcoeff;
    int qp_per, qp_rem;

    sSyntaxElement se;
    sDataPartition* dataPartition = NULL;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;
    int intra = (mb->isIntraBlock == true);
    int need_transform_sizeFlag;

    int (*InvLevelScale4x4)[4] = NULL;
    int (*InvLevelScale8x8)[8] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB) ? SE_CBP_INTRA : SE_CBP_INTER;
      dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
      se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? slice->linfoCbpIntra : slice->linfoCbpInter;
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
        //{{{  read eCavlc transform_size_8x8Flag
        se.type =  SE_HEADER;
        dataPartition = &slice->dataPartitions[dpMap[SE_HEADER]];
        se.len = 1;
        dataPartition->bitStream.readSyntaxElement_FLC (&se);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }
        //}}}
      if (codedBlockPattern != 0) {
        //{{{  Delta quant only if nonzero coeffs
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if( intra && slice->noDataPartitionB ) {
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
    else  {
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


        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }
      //}}}

    updateQp (mb, slice->qp);

    qp_per = decoder->qpPerMatrix[ mb->qpScaled[PLANE_Y] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[PLANE_Y] ];

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
    InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
    if (codedBlockPattern) {
      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      }
    else
      memset(decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(uint8_t));
    }
  //}}}
  //{{{
  void readCbp422cavlc (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int i,j,k;
    int mb_nr = mb->mbIndexX;
    int codedBlockPattern;

    int coef_ctr, i0, j0, b8;
    int ll;
    int levarr[16], runarr[16], numcoeff;

    int qp_per, qp_rem;

    int uv;
    int qp_per_uv[2];
    int qp_rem_uv[2];

    int intra = (mb->isIntraBlock == true);

    int b4;
    int m6[4];

    int need_transform_sizeFlag;

    int (*InvLevelScale4x4)[4] = NULL;
    int (*InvLevelScale8x8)[8] = NULL;
    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    sSyntaxElement se;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    sDataPartition* dataPartition = NULL;

    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? SE_CBP_INTRA : SE_CBP_INTER;
      dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
      se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? slice->linfoCbpIntra : slice->linfoCbpInter;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        //{{{  Transform size flag for INTER MBs
        se.type   =  SE_HEADER;
        dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);

        // read eCavlc transform_size_8x8Flag
        se.len = 1;
        dataPartition->bitStream.readSyntaxElement_FLC (&se);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }
        //}}}
      if (codedBlockPattern != 0) {
        //{{{  Delta quant only if nonzero coeffs
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if( intra && slice->noDataPartitionB ) {
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
        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
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
    if (codedBlockPattern) {
      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      }
    else
      memset(decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(uint8_t));

    //{{{  chroma DC coeff
    if (codedBlockPattern>15) {
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
              mb->cbp[0].blk |= ((int64_t)0xff0000) << (ll<<2);
              coef_ctr += runarr[k]+1;
              i0 = SCAN_YUV422[coef_ctr][0];
              j0 = SCAN_YUV422[coef_ctr][1];

              m3[i0][j0]=levarr[k];
            }
          }

          // inverse CHROMA DC YUV422 transform horizontal
          if(mb->isLossless == false) {
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
    if (codedBlockPattern<=31)
      memset (decoder->nzCoeff [mb_nr ][1][0], 0, 2 * BLOCK_PIXELS * sizeof(uint8_t));
    else {
      if(mb->isLossless == false) {
        for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];

          for (b4=0; b4 < 4; ++b4) {
            i = cofuv_blk_x[1][b8][b4];
            j = cofuv_blk_y[1][b8][b4];

            slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
            coef_ctr = 0;

            for(k = 0; k < numcoeff;++k) {
              if (levarr[k] != 0) {
                mb->cbp[0].blk |= i64power2(cbp_blk_chroma[b8][b4]);
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
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          for (b4=0; b4 < 4; ++b4) {
            i = cofuv_blk_x[1][b8][b4];
            j = cofuv_blk_y[1][b8][b4];

            slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
            coef_ctr = 0;

            for(k = 0; k < numcoeff;++k) {
              if (levarr[k] != 0) {
                mb->cbp[0].blk |= i64power2(cbp_blk_chroma[b8][b4]);
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
  void readCbp444cavlc (sMacroBlock* mb) {

    cSlice* slice = mb->slice;

    int i,k;
    int mb_nr = mb->mbIndexX;
    int codedBlockPattern;
    int coef_ctr, i0, j0;
    int levarr[16], runarr[16], numcoeff;

    int qp_per, qp_rem;
    int uv;
    int qp_per_uv[3];
    int qp_rem_uv[3];
    int need_transform_sizeFlag;

    sSyntaxElement se;
    sDataPartition *dataPartition = NULL;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;

    int intra = (mb->isIntraBlock == true);

    int (*InvLevelScale4x4)[4] = NULL;
    int (*InvLevelScale8x8)[8] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? SE_CBP_INTRA : SE_CBP_INTER;
      dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
      se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? slice->linfoCbpIntra : slice->linfoCbpInter;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      //============= Transform size flag for INTER MBs =============
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type   =  SE_HEADER;
        dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
        // read eCavlc transform_size_8x8Flag
        se.len = 1;
        dataPartition->bitStream.readSyntaxElement_FLC (&se);
        mb->lumaTransformSize8x8flag = (bool)se.value1;
        }

      // Delta quant only if nonzero coeffs
      if (codedBlockPattern !=0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if( intra && slice->noDataPartitionB ) {
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
        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
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
    if (codedBlockPattern) {
      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
    }
    else
      memset(decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(uint8_t));

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

        if(mb->isLossless == false)
          itrans2(mb, (eColorPlane) (uv)); // transform new intra DC
        } //IS_I16MB

      //init constants for every chroma qp offset
      updateQp(mb, slice->qp);
      qp_per_uv[uv] = decoder->qpPerMatrix[ mb->qpScaled[uv] ];
      qp_rem_uv[uv] = decoder->qpRemMatrix[ mb->qpScaled[uv] ];

      InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv][qp_rem_uv[uv]];
      InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[uv][qp_rem_uv[uv]] : slice->InvLevelScale8x8_Inter[uv][qp_rem_uv[uv]];

      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, (eColorPlane) (uv), InvLevelScale4x4, qp_per_uv[uv], codedBlockPattern, decoder->nzCoeff[mb_nr][uv]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, (eColorPlane) (uv), InvLevelScale8x8, qp_per_uv[uv], codedBlockPattern, decoder->nzCoeff[mb_nr][uv]);
      }
      //}}}
    }
  //}}}
  //{{{
  void readCbp420cavlc (sMacroBlock* mb) {

    cSlice* slice = mb->slice;

    int i,j,k;
    int mb_nr = mb->mbIndexX;
    int codedBlockPattern;
    int coef_ctr, i0, j0, b8;
    int ll;
    int levarr[16], runarr[16], numcoeff;

    int qp_per, qp_rem;

    sSyntaxElement se;
    sDataPartition* dataPartition = NULL;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;
    int smb = ((decoder->coding.sliceType == eSliceSP) && (mb->isIntraBlock == false)) ||
              ((decoder->coding.sliceType == eSliceSI) && (mb->mbType == SI4MB));

    int uv;
    int qp_per_uv[2];
    int qp_rem_uv[2];

    int intra = (mb->isIntraBlock == true);
    int temp[4];

    int b4;
    int need_transform_sizeFlag;
    int (*InvLevelScale4x4)[4] = NULL;
    int (*InvLevelScale8x8)[8] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? SE_CBP_INTRA : SE_CBP_INTER;
      dataPartition = &(slice->dataPartitions[dpMap[se.type]]);
      se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? slice->linfoCbpIntra : slice->linfoCbpInter;
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);
      if (need_transform_sizeFlag) {
        //{{{  Transform size flag for INTER MBs
        se.type =  SE_HEADER;
        dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
        // read eCavlc transform_size_8x8Flag
        se.len = 1;
        dataPartition->bitStream.readSyntaxElement_FLC (&se);
        mb->lumaTransformSize8x8flag = (bool)se.value1;
        }
        //}}}

      // Delta quant only if nonzero coeffs
      if (codedBlockPattern != 0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);
        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
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

        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }

    updateQp (mb, slice->qp);
    qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

    // init quant parameters for chroma
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
    InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
    if (codedBlockPattern) {
      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      }
    else
      memset(decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(uint8_t));

    //{{{  chroma DC coeff
    if (codedBlockPattern>15) {
      for (ll=0;ll<3;ll+=2) {
        uv = ll>>1;

        InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];
        // CHROMA DC YUV420
        slice->cofu[0] = slice->cofu[1] = slice->cofu[2] = slice->cofu[3] = 0;
        coef_ctr=-1;

        slice->readCoef4x4cavlc(mb, CHROMA_DC, 0, 0, levarr, runarr, &numcoeff);
        for(k = 0; k < numcoeff; ++k) {
          if (levarr[k] != 0) {
            mb->cbp[0].blk |= 0xf0000 << (ll<<1) ;
            coef_ctr += runarr[k] + 1;
            slice->cofu[coef_ctr]=levarr[k];
          }
        }


        if (smb || (mb->isLossless == true)) {
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
    if (codedBlockPattern<=31)
      memset(decoder->nzCoeff [mb_nr ][1][0], 0, 2 * BLOCK_PIXELS * sizeof(uint8_t));
    else {
      if (mb->isLossless == false) {
        for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];

          for (b4=0; b4 < 4; ++b4) {
            i = cofuv_blk_x[0][b8][b4];
            j = cofuv_blk_y[0][b8][b4];
            slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
            coef_ctr = 0;
            for(k = 0; k < numcoeff;++k) {
              if (levarr[k] != 0) {
                mb->cbp[0].blk |= i64power2(cbp_blk_chroma[b8][b4]);
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
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          for (b4=0; b4 < 4; ++b4) {
            i = cofuv_blk_x[0][b8][b4];
            j = cofuv_blk_y[0][b8][b4];
            slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
            coef_ctr = 0;
            for (k = 0; k < numcoeff;++k) {
              if (levarr[k] != 0) {
                mb->cbp[0].blk |= i64power2(cbp_blk_chroma[b8][b4]);
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
  }

//{{{
cSlice* cSlice::allocSlice() {

  cSlice* slice = new cSlice();
  memset (slice, 0, sizeof(cSlice));

  // create all context models
  slice->motionContexts = (sMotionContexts*)calloc (1, sizeof(sMotionContexts));
  slice->textureContexts = (sTextureContexts*)calloc (1, sizeof(sTextureContexts));

  slice->maxDataPartitions = 3;
  slice->dataPartitions = sDataPartition::allocDataPartitions (slice->maxDataPartitions);

  getMem3Dint (&slice->weightedPredWeight, 2, MAX_REFERENCE_PICTURES, 3);
  getMem3Dint (&slice->weightedPredOffset, 6, MAX_REFERENCE_PICTURES, 3);
  getMem4Dint (&slice->weightedBiPredWeight, 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);
  getMem3Dpel (&slice->mbPred, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dpel (&slice->mbRec, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&slice->mbRess, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&slice->cof, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

  getMem2Dpel (&slice->tempBlockL0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dpel (&slice->tempBlockL1, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dpel (&slice->tempBlockL2, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dpel (&slice->tempBlockL3, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dint (&slice->tempRes, MB_BLOCK_SIZE + 5, MB_BLOCK_SIZE + 5);

  // reference flag initialization
  for (int i = 0; i < 17; ++i)
    slice->refFlag[i] = 1;

  for (int i = 0; i < 6; i++)
    slice->listX[i] = (sPicture**)calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering

  for (int j = 0; j < 6; j++) {
    for (int i = 0; i < MAX_LIST_SIZE; i++)
      slice->listX[j][i] = NULL;
    slice->listXsize[j]=0;
    }

  return slice;
  }
//}}}
//{{{
cSlice::~cSlice() {

  if (sliceType != eSliceI && sliceType != eSliceSI)
    freeRefPicListReorderBuffer();

  freeMem2Dint (tempRes);
  freeMem2Dpel (tempBlockL0);
  freeMem2Dpel (tempBlockL1);
  freeMem2Dpel (tempBlockL2);
  freeMem2Dpel (tempBlockL3);

  freeMem3Dint (cof);
  freeMem3Dint (mbRess);
  freeMem3Dpel (mbRec);
  freeMem3Dpel (mbPred);

  freeMem3Dint (weightedPredWeight);
  freeMem3Dint (weightedPredOffset);
  freeMem4Dint (weightedBiPredWeight);

  sDataPartition::freeDataPartitions (dataPartitions, 3);

  free (motionContexts);
  free (textureContexts);

  for (int i = 0; i < 6; i++) {
    if (listX[i]) {
      free (listX[i]);
      listX[i] = NULL;
      }
    }

  while (decRefPicMarkBuffer) {
    sDecodedRefPicMark* tempDecodedRefPicMark = decRefPicMarkBuffer;
    decRefPicMarkBuffer = tempDecodedRefPicMark->next;
    free (tempDecodedRefPicMark);
    }
  }
//}}}

//{{{
void cSlice::setReadCbpCabac() {

  switch (decoder->activeSps->chromaFormatIdc) {
    case YUV444:
      if (decoder->coding.isSeperateColourPlane == 0)
        readCBPcoeffs = readCbp444cabac;
      else
        readCBPcoeffs = readCbp400cabac;
      break;

    case YUV422:
      readCBPcoeffs = readCbp422cabac;
      break;

    case YUV420:
      readCBPcoeffs = readCbp420cabac;
      break;

    case YUV400:
      readCBPcoeffs = readCbp400cabac;
      break;

    default:
      readCBPcoeffs = NULL;
      break;
    }
  }
//}}}
//{{{
void cSlice::setReadCbpCavlc() {

  switch (decoder->activeSps->chromaFormatIdc) {
    case YUV444:
      if (decoder->coding.isSeperateColourPlane == 0)
        readCBPcoeffs = readCbp444cavlc;
      else
        readCBPcoeffs = readCbp400cavlc;
      break;

    case YUV422:
      readCBPcoeffs = readCbp422cavlc;
      break;

    case YUV420:
      readCBPcoeffs = readCbp420cavlc;
      break;

    case YUV400:
      readCBPcoeffs = readCbp400cavlc;
      break;

    default:
      readCBPcoeffs = NULL;
      break;
    }
  }
//}}}

//{{{
void cSlice::setQuantParams() {

  cSps* sps = activeSps;
  cPps* pps = activePps;

  if (!pps->hasPicScalingMatrix && !sps->hasSeqScalingMatrix) {
    for (int i = 0; i < 12; i++)
      qmatrix[i] = (i < 6) ? quant_org : quant8_org;
    }
  else {
    int n_ScalingList = (sps->chromaFormatIdc != YUV444) ? 8 : 12;
    if (sps->hasSeqScalingMatrix) {
      //{{{  check sps first
      for (int i = 0; i < n_ScalingList; i++) {
        if (i < 6) {
          if (!sps->hasSeqScalingList[i]) {
            // fall-back rule A
            if (i == 0)
              qmatrix[i] = quant_intra_default;
            else if (i == 3)
              qmatrix[i] = quant_inter_default;
            else
              qmatrix[i] = qmatrix[i-1];
            }
          else {
            if (sps->useDefaultScalingMatrix4x4[i])
              qmatrix[i] = (i<3) ? quant_intra_default : quant_inter_default;
            else
              qmatrix[i] = sps->scalingList4x4[i];
          }
        }
        else {
          if (!sps->hasSeqScalingList[i]) {
            // fall-back rule A
            if (i == 6)
              qmatrix[i] = quant8_intra_default;
            else if (i == 7)
              qmatrix[i] = quant8_inter_default;
            else
              qmatrix[i] = qmatrix[i-2];
            }
          else {
            if (sps->useDefaultScalingMatrix8x8[i-6])
              qmatrix[i] = (i==6 || i==8 || i==10) ? quant8_intra_default:quant8_inter_default;
            else
              qmatrix[i] = sps->scalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    if (pps->hasPicScalingMatrix) {
      //{{{  then check pps
      for (int i = 0; i < n_ScalingList; i++) {
        if (i < 6) {
          if (!pps->picScalingListPresentFlag[i]) {
            // fall-back rule B
            if (i == 0) {
              if (!sps->hasSeqScalingMatrix)
                qmatrix[i] = quant_intra_default;
              }
            else if (i == 3) {
              if (!sps->hasSeqScalingMatrix)
                qmatrix[i] = quant_inter_default;
              }
            else
              qmatrix[i] = qmatrix[i-1];
            }
          else {
            if (pps->useDefaultScalingMatrix4x4Flag[i])
              qmatrix[i] = (i<3) ? quant_intra_default:quant_inter_default;
            else
              qmatrix[i] = pps->scalingList4x4[i];
            }
          }
        else {
          if (!pps->picScalingListPresentFlag[i]) {
            // fall-back rule B
            if (i == 6) {
              if (!sps->hasSeqScalingMatrix)
                qmatrix[i] = quant8_intra_default;
              }
            else if (i == 7) {
              if (!sps->hasSeqScalingMatrix)
                qmatrix[i] = quant8_inter_default;
              }
            else
              qmatrix[i] = qmatrix[i-2];
            }
          else {
            if (pps->useDefaultScalingMatrix8x8Flag[i-6])
              qmatrix[i] = (i==6 || i==8 || i==10) ? quant8_intra_default:quant8_inter_default;
            else
              qmatrix[i] = pps->scalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    }

  calculateQuant4x4Param();
  if (pps->hasTransform8x8mode)
    calculateQuant8x8Param();
  }
//}}}
//{{{
void cSlice::fillWeightedPredParam() {

  if (sliceType == eSliceB) {
    int maxL0Ref = numRefIndexActive[LIST_0];
    int maxL1Ref = numRefIndexActive[LIST_1];
    if (activePps->weightedBiPredIdc == 2) {
      lumaLog2weightDenom = 5;
      chromaLog2weightDenom = 5;
      wpRoundLuma = 16;
      wpRoundChroma = 16;
      for (int i = 0; i < MAX_REFERENCE_PICTURES; ++i)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? lumaLog2weightDenom : chromaLog2weightDenom;
          weightedPredWeight[0][i][comp] = 1 << logWeightDenom;
          weightedPredWeight[1][i][comp] = 1 << logWeightDenom;
          weightedPredOffset[0][i][comp] = 0;
          weightedPredOffset[1][i][comp] = 0;
          }
      }

    for (int i = 0; i < maxL0Ref; ++i)
      for (int j = 0; j < maxL1Ref; ++j)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? lumaLog2weightDenom : chromaLog2weightDenom;
          if (activePps->weightedBiPredIdc == 1) {
            weightedBiPredWeight[0][i][j][comp] = weightedPredWeight[0][i][comp];
            weightedBiPredWeight[1][i][j][comp] = weightedPredWeight[1][j][comp];
            }
          else if (activePps->weightedBiPredIdc == 2) {
            int td = iClip3(-128,127,listX[LIST_1][j]->poc - listX[LIST_0][i]->poc);
            if (!td || listX[LIST_1][j]->usedLongTermRef || listX[LIST_0][i]->usedLongTermRef) {
              weightedBiPredWeight[0][i][j][comp] = 32;
              weightedBiPredWeight[1][i][j][comp] = 32;
              }
            else {
              int tb = iClip3(-128, 127, thisPoc - listX[LIST_0][i]->poc);
              int tx = (16384 + iabs (td / 2)) / td;
              int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
              weightedBiPredWeight[1][i][j][comp] = distScaleFactor >> 2;
              weightedBiPredWeight[0][i][j][comp] = 64 - weightedBiPredWeight[1][i][j][comp];
              if (weightedBiPredWeight[1][i][j][comp] < -64 ||
                  weightedBiPredWeight[1][i][j][comp] > 128) {
                weightedBiPredWeight[0][i][j][comp] = 32;
                weightedBiPredWeight[1][i][j][comp] = 32;
                weightedPredOffset[0][i][comp] = 0;
                weightedPredOffset[1][j][comp] = 0;
                }
              }
            }
          }

    if (mbAffFrame)
      for (int i = 0; i < 2 * maxL0Ref; ++i)
        for (int j = 0; j < 2 * maxL1Ref; ++j)
          for (int comp = 0; comp<3; ++comp)
            for (int k = 2; k < 6; k += 2) {
              weightedPredOffset[k+0][i][comp] = weightedPredOffset[0][i>>1][comp];
              weightedPredOffset[k+1][j][comp] = weightedPredOffset[1][j>>1][comp];
              int logWeightDenom = !comp ? lumaLog2weightDenom : chromaLog2weightDenom;
              if (activePps->weightedBiPredIdc == 1) {
                weightedBiPredWeight[k+0][i][j][comp] = weightedPredWeight[0][i>>1][comp];
                weightedBiPredWeight[k+1][i][j][comp] = weightedPredWeight[1][j>>1][comp];
                }
              else if (activePps->weightedBiPredIdc == 2) {
                int td = iClip3 (-128, 127, listX[k+LIST_1][j]->poc - listX[k+LIST_0][i]->poc);
                if (!td || listX[k+LIST_1][j]->usedLongTermRef || listX[k+LIST_0][i]->usedLongTermRef) {
                  weightedBiPredWeight[k+0][i][j][comp] = 32;
                  weightedBiPredWeight[k+1][i][j][comp] = 32;
                  }
                else {
                  int tb = iClip3 (-128,127,
                               ((k == 2) ? topPoc : botPoc) - listX[k+LIST_0][i]->poc);
                  int tx = (16384 + iabs(td/2)) / td;
                  int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
                  weightedBiPredWeight[k+1][i][j][comp] = distScaleFactor >> 2;
                  weightedBiPredWeight[k+0][i][j][comp] = 64 - weightedBiPredWeight[k+1][i][j][comp];
                  if (weightedBiPredWeight[k+1][i][j][comp] < -64 ||
                      weightedBiPredWeight[k+1][i][j][comp] > 128) {
                    weightedBiPredWeight[k+1][i][j][comp] = 32;
                    weightedBiPredWeight[k+0][i][j][comp] = 32;
                    weightedPredOffset[k+0][i][comp] = 0;
                    weightedPredOffset[k+1][j][comp] = 0;
                    }
                  }
                }
              }
    }
  }
//}}}
//{{{
void cSlice::resetWeightedPredParam() {

  for (int i = 0; i < MAX_REFERENCE_PICTURES; i++) {
    for (int comp = 0; comp < 3; comp++) {
      int logWeightDenom = (comp == 0) ? lumaLog2weightDenom : chromaLog2weightDenom;
      weightedPredWeight[0][i][comp] = 1 << logWeightDenom;
      weightedPredWeight[1][i][comp] = 1 << logWeightDenom;
      }
    }
  }
//}}}

//{{{
void cSlice::initMbAffLists (sPicture* noRefPicture) {
// Initialize listX[2..5] from lists 0 and 1
//  listX[2]: list0 for current_field == top
//  listX[3]: list1 for current_field == top
//  listX[4]: list0 for current_field == bottom
//  listX[5]: list1 for current_field == bottom

  for (int i = 2; i < 6; i++) {
    for (uint32_t j = 0; j < MAX_LIST_SIZE; j++)
      listX[i][j] = noRefPicture;
    listXsize[i] = 0;
    }

  for (int i = 0; i < listXsize[0]; i++) {
    listX[2][2*i] = listX[0][i]->topField;
    listX[2][2*i+1] = listX[0][i]->botField;
    listX[4][2*i] = listX[0][i]->botField;
    listX[4][2*i+1] = listX[0][i]->topField;
    }
  listXsize[2] = listXsize[4] = listXsize[0] * 2;

  for (int i = 0; i < listXsize[1]; i++) {
    listX[3][2*i] = listX[1][i]->topField;
    listX[3][2*i+1] = listX[1][i]->botField;
    listX[5][2*i] = listX[1][i]->botField;
    listX[5][2*i+1] = listX[1][i]->topField;
    }
  listXsize[3] = listXsize[5] = listXsize[1] * 2;
  }
//}}}
//{{{
void cSlice::copyPoc (cSlice* toSlice) {

  toSlice->topPoc = topPoc;
  toSlice->botPoc = botPoc;
  toSlice->thisPoc = thisPoc;
  toSlice->framePoc = framePoc;
  }
//}}}
//{{{
void cSlice::updatePicNum() {

  int maxFrameNum = 1 << (decoder->activeSps->log2maxFrameNumMinus4 + 4);

  if (picStructure == eFrame) {
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->frameStoreRefArray[i]->isUsed == 3 ) {
        if (dpb->frameStoreRefArray[i]->frame->usedForRef &&
            !dpb->frameStoreRefArray[i]->frame->usedLongTermRef) {
          if (dpb->frameStoreRefArray[i]->frameNum > frameNum )
            dpb->frameStoreRefArray[i]->frameNumWrap = dpb->frameStoreRefArray[i]->frameNum - maxFrameNum;
          else
            dpb->frameStoreRefArray[i]->frameNumWrap = dpb->frameStoreRefArray[i]->frameNum;
          dpb->frameStoreRefArray[i]->frame->picNum = dpb->frameStoreRefArray[i]->frameNumWrap;
          }
        }
      }

    // update longTermPicNum
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->frameStoreLongTermRefArray[i]->isUsed == 3) {
        if (dpb->frameStoreLongTermRefArray[i]->frame->usedLongTermRef)
          dpb->frameStoreLongTermRefArray[i]->frame->longTermPicNum = dpb->frameStoreLongTermRefArray[i]->frame->longTermFrameIndex;
        }
      }
    }
  else {
    int addTop = 0;
    int addBot = 0;
    if (picStructure == eTopField) 
      addTop = 1;
    else
      addBot = 1;

    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->frameStoreRefArray[i]->usedRef) {
        if (dpb->frameStoreRefArray[i]->frameNum > frameNum )
          dpb->frameStoreRefArray[i]->frameNumWrap = dpb->frameStoreRefArray[i]->frameNum - maxFrameNum;
        else
          dpb->frameStoreRefArray[i]->frameNumWrap = dpb->frameStoreRefArray[i]->frameNum;
        if (dpb->frameStoreRefArray[i]->usedRef & 1)
          dpb->frameStoreRefArray[i]->topField->picNum = (2 * dpb->frameStoreRefArray[i]->frameNumWrap) + addTop;
        if (dpb->frameStoreRefArray[i]->usedRef & 2)
          dpb->frameStoreRefArray[i]->botField->picNum = (2 * dpb->frameStoreRefArray[i]->frameNumWrap) + addBot;
        }
      }

    // update longTermPicNum
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->frameStoreLongTermRefArray[i]->usedLongTermRef & 1)
        dpb->frameStoreLongTermRefArray[i]->topField->longTermPicNum = 2 * dpb->frameStoreLongTermRefArray[i]->topField->longTermFrameIndex + addTop;
      if (dpb->frameStoreLongTermRefArray[i]->usedLongTermRef & 2)
        dpb->frameStoreLongTermRefArray[i]->botField->longTermPicNum = 2 * dpb->frameStoreLongTermRefArray[i]->botField->longTermFrameIndex + addBot;
      }
    }
  }
//}}}

//{{{
void cSlice::allocRefPicListReordeBuffer() {

  if ((sliceType != eSliceI) && (sliceType != eSliceSI)) {
    // B,P
    int size = numRefIndexActive[LIST_0] + 1;
    modPicNumsIdc[LIST_0] = (int*)calloc (size ,sizeof(int));
    longTermPicIndex[LIST_0] = (int*)calloc (size,sizeof(int));
    absDiffPicNumMinus1[LIST_0] = (int*)calloc (size,sizeof(int));
    }
  else {
    modPicNumsIdc[LIST_0] = NULL;
    longTermPicIndex[LIST_0] = NULL;
    absDiffPicNumMinus1[LIST_0] = NULL;
    }

  if (sliceType == eSliceB) {
    // B
    int size = numRefIndexActive[LIST_1] + 1;
    modPicNumsIdc[LIST_1] = (int*)calloc (size,sizeof(int));
    longTermPicIndex[LIST_1] = (int*)calloc (size,sizeof(int));
    absDiffPicNumMinus1[LIST_1] = (int*)calloc (size,sizeof(int));
    }
  else {
    modPicNumsIdc[LIST_1] = NULL;
    longTermPicIndex[LIST_1] = NULL;
    absDiffPicNumMinus1[LIST_1] = NULL;
    }
  }
//}}}
//{{{
void cSlice::freeRefPicListReorderBuffer() {

  free (modPicNumsIdc[LIST_0]);
  modPicNumsIdc[LIST_0] = NULL;

  free (absDiffPicNumMinus1[LIST_0]);
  absDiffPicNumMinus1[LIST_0] = NULL;

  free (longTermPicIndex[LIST_0]);
  longTermPicIndex[LIST_0] = NULL;

  free (modPicNumsIdc[LIST_1]);
  modPicNumsIdc[LIST_1] = NULL;

  free (absDiffPicNumMinus1[LIST_1]);
  absDiffPicNumMinus1[LIST_1] = NULL;

  free (longTermPicIndex[LIST_1]);
  longTermPicIndex[LIST_1] = NULL;
  }
//}}}
//{{{
void cSlice::reorderRefPicList (int curList) {

  int* curModPicNumsIdc = modPicNumsIdc[curList];
  int* curAbsDiffPicNumMinus1 = absDiffPicNumMinus1[curList];
  int* curLongTermPicIndex = longTermPicIndex[curList];
  int numRefIndexIXactiveMinus1 = numRefIndexActive[curList] - 1;

  int maxPicNum;
  int currPicNum;
  if (picStructure == eFrame) {
    maxPicNum = decoder->coding.maxFrameNum;
    currPicNum = frameNum;
    }
  else {
    maxPicNum = 2 * decoder->coding.maxFrameNum;
    currPicNum = 2 * frameNum + 1;
    }

  int picNumLX;
  int picNumLXNoWrap;
  int picNumLXPred = currPicNum;
  int refIdxLX = 0;
  for (int i = 0; curModPicNumsIdc[i] != 3; i++) {
    if (curModPicNumsIdc[i] > 3)
      cDecoder264::error ("Invalid modPicNumsIdc command");
    if (curModPicNumsIdc[i] < 2) {
      if (curModPicNumsIdc[i] == 0) {
        if (picNumLXPred - (curAbsDiffPicNumMinus1[i]+1) < 0)
          picNumLXNoWrap = picNumLXPred - (curAbsDiffPicNumMinus1[i]+1) + maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred - (curAbsDiffPicNumMinus1[i]+1);
        }
      else {
        if (picNumLXPred + curAbsDiffPicNumMinus1[i]+1 >= maxPicNum)
          picNumLXNoWrap = picNumLXPred + (curAbsDiffPicNumMinus1[i]+1) - maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred + (curAbsDiffPicNumMinus1[i]+1);
        }

      picNumLXPred = picNumLXNoWrap;
      if (picNumLXNoWrap > currPicNum)
        picNumLX = picNumLXNoWrap - maxPicNum;
      else
        picNumLX = picNumLXNoWrap;

      reorderShortTerm (curList, numRefIndexIXactiveMinus1, picNumLX, &refIdxLX);
      }
    else
      reorderLongTerm (listX[curList], numRefIndexIXactiveMinus1, curLongTermPicIndex[i], &refIdxLX);
    }

  listXsize[curList] = (char)(numRefIndexIXactiveMinus1 + 1);
  }
//}}}
//{{{
void cSlice::computeColocated (sPicture** listX[6]) {

  if (directSpatialMvPredFlag == 0) {
    for (int j = 0; j < 2 + (mbAffFrame * 4); j += 2) {
      for (int i = 0; i < listXsize[j];i++) {
        int iTRb;
        if (j == 0)
          iTRb = iClip3 (-128, 127, decoder->picture->poc - listX[LIST_0 + j][i]->poc);
        else if (j == 2)
          iTRb = iClip3 (-128, 127, decoder->picture->topPoc - listX[LIST_0 + j][i]->poc);
        else
          iTRb = iClip3 (-128, 127, decoder->picture->botPoc - listX[LIST_0 + j][i]->poc);

        int iTRp = iClip3 (-128, 127, listX[LIST_1 + j][0]->poc - listX[LIST_0 + j][i]->poc);
        if (iTRp != 0) {
          int prescale = (16384 + iabs( iTRp / 2)) / iTRp;
          mvscale[j][i] = iClip3 (-1024, 1023, (iTRb * prescale + 32) >> 6) ;
          }
        else
          mvscale[j][i] = 9999;
        }
     }
    }
  }
//}}}

// private:
//{{{
void cSlice::calculateQuant4x4Param() {

  const int (*p_dequant_coef)[4][4] = dequant_coef;
  int (*InvLevelScale4x4_Intra_0)[4][4] = InvLevelScale4x4_Intra[0];
  int (*InvLevelScale4x4_Intra_1)[4][4] = InvLevelScale4x4_Intra[1];
  int (*InvLevelScale4x4_Intra_2)[4][4] = InvLevelScale4x4_Intra[2];
  int (*InvLevelScale4x4_Inter_0)[4][4] = InvLevelScale4x4_Inter[0];
  int (*InvLevelScale4x4_Inter_1)[4][4] = InvLevelScale4x4_Inter[1];
  int (*InvLevelScale4x4_Inter_2)[4][4] = InvLevelScale4x4_Inter[2];

  for (int k = 0; k < 6; k++) {
    setDequant4x4 (*InvLevelScale4x4_Intra_0++, *p_dequant_coef, qmatrix[0]);
    setDequant4x4 (*InvLevelScale4x4_Intra_1++, *p_dequant_coef, qmatrix[1]);
    setDequant4x4 (*InvLevelScale4x4_Intra_2++, *p_dequant_coef, qmatrix[2]);
    setDequant4x4 (*InvLevelScale4x4_Inter_0++, *p_dequant_coef, qmatrix[3]);
    setDequant4x4 (*InvLevelScale4x4_Inter_1++, *p_dequant_coef, qmatrix[4]);
    setDequant4x4 (*InvLevelScale4x4_Inter_2++, *p_dequant_coef++, qmatrix[5]);
    }
  }
//}}}
//{{{
void cSlice::calculateQuant8x8Param() {

  const int (*p_dequant_coef)[8][8] = dequant_coef8;
  int (*InvLevelScale8x8_Intra_0)[8][8] = InvLevelScale8x8_Intra[0];
  int (*InvLevelScale8x8_Intra_1)[8][8] = InvLevelScale8x8_Intra[1];
  int (*InvLevelScale8x8_Intra_2)[8][8] = InvLevelScale8x8_Intra[2];
  int (*InvLevelScale8x8_Inter_0)[8][8] = InvLevelScale8x8_Inter[0];
  int (*InvLevelScale8x8_Inter_1)[8][8] = InvLevelScale8x8_Inter[1];
  int (*InvLevelScale8x8_Inter_2)[8][8] = InvLevelScale8x8_Inter[2];

  for (int k = 0; k < 6; k++) {
    setDequant8x8 (*InvLevelScale8x8_Intra_0++, *p_dequant_coef  , qmatrix[6]);
    setDequant8x8 (*InvLevelScale8x8_Inter_0++, *p_dequant_coef++, qmatrix[7]);
    }

  p_dequant_coef = dequant_coef8;
  if (activeSps->chromaFormatIdc == 3) {
    // 4:4:4
    for (int k = 0; k < 6; k++) {
      setDequant8x8 (*InvLevelScale8x8_Intra_1++, *p_dequant_coef, qmatrix[8]);
      setDequant8x8 (*InvLevelScale8x8_Inter_1++, *p_dequant_coef, qmatrix[9]);
      setDequant8x8 (*InvLevelScale8x8_Intra_2++, *p_dequant_coef, qmatrix[10]);
      setDequant8x8 (*InvLevelScale8x8_Inter_2++, *p_dequant_coef++, qmatrix[11]);
      }
    }
  }
//}}}

//{{{
void cSlice::reorderShortTerm (int curList, int numRefIndexIXactiveMinus1, int picNumLX, int* refIdxLX) {

  sPicture** refPicListX = listX[curList];
  sPicture* picLX = dpb->getShortTermPic (this, picNumLX);
  for (int cIdx = numRefIndexIXactiveMinus1+1; cIdx > *refIdxLX; cIdx--)
    refPicListX[cIdx] = refPicListX[cIdx - 1];
  refPicListX[(*refIdxLX)++] = picLX;

  int nIdx = *refIdxLX;
  for (int cIdx = *refIdxLX; cIdx <= numRefIndexIXactiveMinus1+1; cIdx++)
    if (refPicListX[cIdx])
      if ((refPicListX[cIdx]->usedLongTermRef) || (refPicListX[cIdx]->picNum != picNumLX))
        refPicListX[nIdx++] = refPicListX[cIdx];
  }
//}}}
//{{{
void cSlice::reorderLongTerm (sPicture** refPicListX, int numRefIndexIXactiveMinus1,
                              int LongTermPicNum, int* refIdxLX) {

  sPicture* picLX = dpb->getLongTermPic (this, LongTermPicNum);
  for (int cIdx = numRefIndexIXactiveMinus1+1; cIdx > *refIdxLX; cIdx--)
    refPicListX[cIdx] = refPicListX[cIdx - 1];
  refPicListX[(*refIdxLX)++] = picLX;

  int nIdx = *refIdxLX;
  for (int cIdx = *refIdxLX; cIdx <= numRefIndexIXactiveMinus1+1; cIdx++)
    if (refPicListX[cIdx])
      if ((!refPicListX[cIdx]->usedLongTermRef) || (refPicListX[cIdx]->longTermPicNum != LongTermPicNum))
        refPicListX[nIdx++] = refPicListX[cIdx];
  }
//}}}
