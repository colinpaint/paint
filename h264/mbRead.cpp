//{{{  includes
#include <math.h>

#include "global.h"
#include "syntaxElement.h"

#include "block.h"
#include "buffer.h"
#include "macroblock.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "mbAccess.h"
#include "binaryArithmeticDecode.h"
#include "transform.h"
#include "mcPred.h"
#include "quant.h"
#include "mbPred.h"
//}}}
static const sMotionVec kZeroMv = {0, 0};

//{{{
static void read_ipred_8x8_modes_mbaff (sMacroBlock* mb) {

  int bi, bj, bx, by, dec;
  cSlice* slice = mb->slice;
  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
  cDecoder264* decoder = mb->decoder;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_block, top_block;

  sSyntaxElement se;
  se.type = SE_INTRAPREDMODE;
  sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);

  if (!(decoder->activePps->entropyCoding == eCavlc || dataPartition->stream->errorFlag))
    se.reading = readIntraPredMode_CABAC;

  for (int b8 = 0; b8 < 4; ++b8)  {
    // loop 8x8 blocks
    by = (b8 & 0x02);
    bj = mb->blockY + by;

    bx = ((b8 & 0x01) << 1);
    bi = mb->blockX + bx;
    // get from stream
    if (decoder->activePps->entropyCoding == eCavlc || dataPartition->stream->errorFlag)
      readsSyntaxElement_Intra4x4PredictionMode (&se, dataPartition->stream);
    else {
      se.context = b8 << 2;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      }

    get4x4Neighbour (mb, (bx << 2) - 1, (by << 2),     decoder->mbSize[eLuma], &left_block);
    get4x4Neighbour (mb, (bx << 2),     (by << 2) - 1, decoder->mbSize[eLuma], &top_block );

    // get from array and decode
    if (decoder->activePps->hasConstrainedIntraPred) {
      left_block.available = left_block.available ? slice->intraBlock[left_block.mbIndex] : 0;
      top_block.available = top_block.available  ? slice->intraBlock[top_block.mbIndex]  : 0;
      }

    upIntraPredMode = (top_block.available ) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
    leftIntraPredMode = (left_block.available) ? slice->predMode[left_block.posY][left_block.posX] : -1;
    mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED :
                                  upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
    dec = (se.value1 == -1) ?
      mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode);

    //set, loop 4x4s in the subblock for 8x8 prediction setting
    slice->predMode[bj][bi] = (uint8_t) dec;
    slice->predMode[bj][bi+1] = (uint8_t) dec;
    slice->predMode[bj+1][bi] = (uint8_t) dec;
    slice->predMode[bj+1][bi+1] = (uint8_t) dec;
    }
  }
//}}}
//{{{
static void read_ipred_8x8_modes (sMacroBlock* mb) {

  int b8, bi, bj, bx, by, dec;
  cSlice* slice = mb->slice;
  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
  cDecoder264* decoder = mb->decoder;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  sSyntaxElement se;
  se.type = SE_INTRAPREDMODE;
  sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);
  if (!(decoder->activePps->entropyCoding == eCavlc || dataPartition->stream->errorFlag))
    se.reading = readIntraPredMode_CABAC;

  get4x4Neighbour (mb, -1,  0, decoder->mbSize[eLuma], &left_mb);
  get4x4Neighbour (mb,  0, -1, decoder->mbSize[eLuma], &top_mb );

  for(b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
    by = (b8 & 0x02);
    bj = mb->blockY + by;

    bx = ((b8 & 0x01) << 1);
    bi = mb->blockX + bx;

    //get from stream
    if (decoder->activePps->entropyCoding == eCavlc || dataPartition->stream->errorFlag)
      readsSyntaxElement_Intra4x4PredictionMode (&se, dataPartition->stream);
    else {
      se.context = (b8 << 2);
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
    }

    get4x4Neighbour (mb, (bx << 2) - 1, by << 2, decoder->mbSize[eLuma], &left_block);
    get4x4Neighbour (mb, bx << 2, (by << 2) - 1, decoder->mbSize[eLuma], &top_block);

    // get from array and decode
    if (decoder->activePps->hasConstrainedIntraPred) {
      left_block.available = left_block.available ? slice->intraBlock[left_block.mbIndex] : 0;
      top_block.available = top_block.available  ? slice->intraBlock[top_block.mbIndex] : 0;
    }

    upIntraPredMode = (top_block.available ) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
    leftIntraPredMode = (left_block.available) ? slice->predMode[left_block.posY][left_block.posX] : -1;
    mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ?
      DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
    dec = (se.value1 == -1) ?
      mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode);

    //set
    //loop 4x4s in the subblock for 8x8 prediction setting
    slice->predMode[bj][bi] = (uint8_t)dec;
    slice->predMode[bj][bi+1] = (uint8_t)dec;
    slice->predMode[bj+1][bi] = (uint8_t)dec;
    slice->predMode[bj+1][bi+1] = (uint8_t)dec;
  }
}
//}}}
//{{{
static void read_ipred_4x4_modes_mbaff (sMacroBlock* mb) {

  int b8,i,j,bi,bj,bx,by;
  sSyntaxElement se;
  cSlice* slice = mb->slice;
  const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
  cDecoder264 *decoder = mb->decoder;
  sBlockPos *picPos = decoder->picPos;

  int ts, ls;
  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;
  sPixelPos left_block, top_block;

  se.type = SE_INTRAPREDMODE;
  sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);
  if (!(decoder->activePps->entropyCoding == eCavlc || dataPartition->stream->errorFlag))
    se.reading = readIntraPredMode_CABAC;

  for (b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
    for (j = 0; j < 2; j++) { //loop subblocks
      by = (b8 & 0x02) + j;
      bj = mb->blockY + by;

      for(i = 0; i < 2; i++) {
        bx = ((b8 & 1) << 1) + i;
        bi = mb->blockX + bx;
        //get from stream
        if (decoder->activePps->entropyCoding == eCavlc || dataPartition->stream->errorFlag)
          readsSyntaxElement_Intra4x4PredictionMode (&se, dataPartition->stream);
        else {
          se.context = (b8<<2) + (j<<1) +i;
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
          }

        get4x4Neighbour (mb, (bx<<2) - 1, (by<<2),     decoder->mbSize[eLuma], &left_block);
        get4x4Neighbour (mb, (bx<<2),     (by<<2) - 1, decoder->mbSize[eLuma], &top_block );

        // get from array and decode
        if (decoder->activePps->hasConstrainedIntraPred) {
          left_block.available = left_block.available ? slice->intraBlock[left_block.mbIndex] : 0;
          top_block.available = top_block.available  ? slice->intraBlock[top_block.mbIndex]  : 0;
          }

        // !! KS: not sure if the following is still correct...
        ts = ls = 0;   // Check to see if the neighboring block is SI
        if (slice->sliceType == eSliceSI) { // need support for MBINTLC1
          if (left_block.available)
            if (slice->siBlock [picPos[left_block.mbIndex].y][picPos[left_block.mbIndex].x])
              ls = 1;

          if (top_block.available)
            if (slice->siBlock [picPos[top_block.mbIndex].y][picPos[top_block.mbIndex].x])
              ts = 1;
          }

        upIntraPredMode = (top_block.available  &&(ts == 0)) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
        leftIntraPredMode = (left_block.available &&(ls == 0)) ? slice->predMode[left_block.posY][left_block.posX] : -1;
        mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ?
          DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
        slice->predMode[bj][bi] = (uint8_t) ((se.value1 == -1) ?
          mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode));
        }
      }
    }
  }
//}}}
//{{{
static void read_ipred_4x4_modes (sMacroBlock* mb) {

  cSlice* slice = mb->slice;
  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
  cDecoder264* decoder = mb->decoder;
  sBlockPos* picPos = decoder->picPos;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  sSyntaxElement se;
  se.type = SE_INTRAPREDMODE;
  sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);
  if (!(decoder->activePps->entropyCoding == eCavlc || dataPartition->stream->errorFlag))
    se.reading = readIntraPredMode_CABAC;

  get4x4Neighbour (mb, -1,  0, decoder->mbSize[eLuma], &left_mb);
  get4x4Neighbour (mb,  0, -1, decoder->mbSize[eLuma], &top_mb );

  for (int b8 = 0; b8 < 4; ++b8) {
    // loop 8x8 blocks
    for (int j = 0; j < 2; j++) {
      // loop subblocks
      int by = (b8 & 0x02) + j;
      int bj = mb->blockY + by;
      for (int i = 0; i < 2; i++) {
        int bx = ((b8 & 1) << 1) + i;
        int bi = mb->blockX + bx;

        // get from stream
        if (decoder->activePps->entropyCoding == eCavlc || dataPartition->stream->errorFlag)
          readsSyntaxElement_Intra4x4PredictionMode (&se, dataPartition->stream);
        else {
          se.context=(b8<<2) + (j<<1) +i;
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
          }

        get4x4Neighbour(mb, (bx<<2) - 1, (by<<2), decoder->mbSize[eLuma], &left_block);
        get4x4Neighbour(mb, (bx<<2), (by<<2) - 1, decoder->mbSize[eLuma], &top_block );

        //get from array and decode
        if (decoder->activePps->hasConstrainedIntraPred) {
          left_block.available = left_block.available ? slice->intraBlock[left_block.mbIndex] : 0;
          top_block.available = top_block.available  ? slice->intraBlock[top_block.mbIndex]  : 0;
          }

        int ts = 0;
        int ls = 0;   // Check to see if the neighboring block is SI
        if (slice->sliceType == eSliceSI) {
          //{{{  need support for MBINTLC1
          if (left_block.available)
            if (slice->siBlock [picPos[left_block.mbIndex].y][picPos[left_block.mbIndex].x])
              ls = 1;

          if (top_block.available)
            if (slice->siBlock [picPos[top_block.mbIndex].y][picPos[top_block.mbIndex].x])
              ts = 1;
          }
          //}}}

        upIntraPredMode = (top_block.available  &&(ts == 0)) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
        leftIntraPredMode = (left_block.available &&(ls == 0)) ? slice->predMode[left_block.posY][left_block.posX] : -1;
        mostProbableIntraPredMode  = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
        slice->predMode[bj][bi] = (uint8_t)((se.value1 == -1) ?
          mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode));
        }
      }
    }
  }
//}}}
//{{{
static void readIpredModes (sMacroBlock* mb) {

  cSlice* slice = mb->slice;
  sPicture* picture = slice->picture;

  if (slice->mbAffFrame) {
    if (mb->mbType == I8MB)
      read_ipred_8x8_modes_mbaff(mb);
    else if (mb->mbType == I4MB)
      read_ipred_4x4_modes_mbaff(mb);
    }
  else {
    if (mb->mbType == I8MB)
      read_ipred_8x8_modes(mb);
    else if (mb->mbType == I4MB)
      read_ipred_4x4_modes(mb);
    }

  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
    sSyntaxElement se;
    sDataPartition* dataPartition;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;

    se.type = SE_INTRAPREDMODE;
    dataPartition = &(slice->dataPartitions[dpMap[SE_INTRAPREDMODE]]);

    if (decoder->activePps->entropyCoding == eCavlc || dataPartition->stream->errorFlag)
      se.mapping = linfo_ue;
    else
      se.reading = readCIPredMode_CABAC;

    dataPartition->readSyntaxElement (mb, &se, dataPartition);
    mb->chromaPredMode = (char)se.value1;

    if (mb->chromaPredMode < DC_PRED_8 || mb->chromaPredMode > PLANE_8)
      cDecoder264::error ("illegal chroma intra pred mode!\n");
    }
  }
//}}}

//{{{
static void resetMvInfo (sPicMotion* mvInfo, int slice_no) {

  mvInfo->refPic[LIST_0] = NULL;
  mvInfo->refPic[LIST_1] = NULL;
  mvInfo->mv[LIST_0] = kZeroMv;
  mvInfo->mv[LIST_1] = kZeroMv;
  mvInfo->refIndex[LIST_0] = -1;
  mvInfo->refIndex[LIST_1] = -1;
  mvInfo->slice_no = slice_no;
  }
//}}}
//{{{
static void initMacroblock (sMacroBlock* mb) {

  cSlice* slice = mb->slice;
  sPicMotion** mvInfo = &slice->picture->mvInfo[mb->blockY];
  int slice_no = slice->curSliceIndex;

  // reset vectors and pred. modes
  for(int j = 0; j < BLOCK_SIZE; ++j) {
    int i = mb->blockX;
    resetMvInfo (*mvInfo + (i++), slice_no);
    resetMvInfo (*mvInfo + (i++), slice_no);
    resetMvInfo (*mvInfo + (i++), slice_no);
    resetMvInfo (*(mvInfo++) + i, slice_no);
    }

  setReadCompCabac (mb);
  setReadCompCoefCavlc (mb);
  }
//}}}
//{{{
static void initMacroblockDirect (sMacroBlock* mb) {

  int slice_no = mb->slice->curSliceIndex;
  sPicMotion** mvInfo = &mb->slice->picture->mvInfo[mb->blockY];

  setReadCompCabac (mb);
  setReadCompCoefCavlc (mb);

  int i = mb->blockX;
  for (int j = 0; j < BLOCK_SIZE; ++j) {
    (*mvInfo+i)->slice_no = slice_no;
    (*mvInfo+i+1)->slice_no = slice_no;
    (*mvInfo+i+2)->slice_no = slice_no;
    (*(mvInfo++)+i+3)->slice_no = slice_no;
    }
  }
//}}}

//{{{
static void concealIPCMcoeffs (sMacroBlock* mb) {

  cSlice* slice = mb->slice;
  cDecoder264* decoder = mb->decoder;
  for (int i = 0; i < MB_BLOCK_SIZE; ++i)
    for (int j = 0; j < MB_BLOCK_SIZE; ++j)
      slice->cof[0][i][j] = decoder->coding.dcPredValueComp[0];

  sPicture* picture = slice->picture;
  if ((picture->chromaFormatIdc != YUV400) && (decoder->coding.isSeperateColourPlane == 0))
    for (int k = 0; k < 2; ++k)
      for (int i = 0; i < decoder->mbCrSizeY; ++i)
        for (int j = 0; j < decoder->mbCrSizeX; ++j)
          slice->cof[k][i][j] = decoder->coding.dcPredValueComp[k];
  }
//}}}
//{{{
static void initIPCMdecoding (cSlice* slice) {

  int dpNum;
  if (slice->dataPartitionMode == eDataPartition1)
    dpNum = 1;
  else if (slice->dataPartitionMode == eDataPartition3)
    dpNum = 3;
  else {
    printf ("dataPartition Mode is not supported\n");
    exit (1);
    }

  for (int i = 0; i < dpNum;++i) {
    sBitStream* stream = slice->dataPartitions[i].stream;
    int byteStartPosition = stream->readLen;
    arithmeticDecodeStartDecoding (&slice->dataPartitions[i].cabacDecodeEnv, stream->bitStreamBuffer, byteStartPosition, &stream->readLen);
    }
  }
//}}}
//{{{
static void readIPCMcoeffs (cSlice* slice, sDataPartition* dataPartition) {

  cDecoder264* decoder = slice->decoder;

  //For eCabac, we don't need to read bits to let stream uint8_t aligned
  //  because we have variable for integer bytes position
  if (decoder->activePps->entropyCoding == eCabac) {
    readIPCMcabac (slice, dataPartition);
    initIPCMdecoding (slice);
    }
  else {
    // read bits to let stream uint8_t aligned
    sSyntaxElement se;
    if (((dataPartition->stream->bitStreamOffset) & 0x07) != 0) {
      se.len = (8 - ((dataPartition->stream->bitStreamOffset) & 0x07));
      readsSyntaxElement_FLC (&se, dataPartition->stream);
      }

    //read luma and chroma IPCM coefficients
    se.len = decoder->bitDepthLuma;
    for (int i = 0; i < MB_BLOCK_SIZE;++i)
      for (int j = 0; j < MB_BLOCK_SIZE;++j) {
        readsSyntaxElement_FLC (&se, dataPartition->stream);
        slice->cof[0][i][j] = se.value1;
        }

    se.len = decoder->bitDepthChroma;
    sPicture* picture = slice->picture;
    if ((picture->chromaFormatIdc != YUV400) && (decoder->coding.isSeperateColourPlane == 0)) {
      for (int i = 0; i < decoder->mbCrSizeY; ++i)
        for (int j = 0; j < decoder->mbCrSizeX; ++j) {
          readsSyntaxElement_FLC (&se, dataPartition->stream);
          slice->cof[1][i][j] = se.value1;
          }

      for (int i = 0; i < decoder->mbCrSizeY; ++i)
        for (int j = 0; j < decoder->mbCrSizeX; ++j) {
          readsSyntaxElement_FLC (&se, dataPartition->stream);
          slice->cof[2][i][j] = se.value1;
          }
      }
    }
  }
//}}}

//{{{
static void SetB8Mode (sMacroBlock* mb, int value, int i) {

  cSlice* slice = mb->slice;
  static const char p_v2b8 [ 5] = {4, 5, 6, 7, IBLOCK};
  static const char p_v2pd [ 5] = {0, 0, 0, 0, -1};
  static const char b_v2b8 [14] = {0, 4, 4, 4, 5, 6, 5, 6, 5, 6, 7, 7, 7, IBLOCK};
  static const char b_v2pd [14] = {2, 0, 1, 2, 0, 0, 1, 1, 2, 2, 0, 1, 2, -1};

  if (slice->sliceType == eSliceB) {
    mb->b8mode[i] = b_v2b8[value];
    mb->b8pdir[i] = b_v2pd[value];
    }
  else {
    mb->b8mode[i] = p_v2b8[value];
    mb->b8pdir[i] = p_v2pd[value];
    }
  }
//}}}

//{{{
static void resetCoeffs (sMacroBlock* mb) {

  cDecoder264* decoder = mb->decoder;

  if (decoder->activePps->entropyCoding == eCavlc)
    memset (decoder->nzCoeff[mb->mbIndexX][0][0], 0, 3 * BLOCK_PIXELS * sizeof(uint8_t));
  }
//}}}
//{{{
static void fieldFlagInference (sMacroBlock* mb) {

  cDecoder264* decoder = mb->decoder;
  if (mb->mbAvailA)
    mb->mbField = decoder->mbData[mb->mbIndexA].mbField;
  else
    // check top macroblock pair
    mb->mbField = mb->mbAvailB ? decoder->mbData[mb->mbIndexB].mbField : false;
  }
//}}}

//{{{
static void skipMacroblocks (sMacroBlock* mb) {

  sMotionVec pred_mv;
  int zeroMotionAbove;
  int zeroMotionLeft;
  sPixelPos neighbourMb[4];
  int i, j;
  int a_mv_y = 0;
  int a_ref_idx = 0;
  int b_mv_y = 0;
  int b_ref_idx = 0;
  int img_block_y   = mb->blockY;
  cDecoder264* decoder = mb->decoder;
  cSlice* slice = mb->slice;
  int   listOffset = LIST_0 + mb->listOffset;
  sPicture* picture = slice->picture;
  sMotionVec* a_mv = NULL;
  sMotionVec* b_mv = NULL;

  getNeighbours (mb, neighbourMb, 0, 0, MB_BLOCK_SIZE);
  if (slice->mbAffFrame == 0) {
    if (neighbourMb[0].available) {
      a_mv = &picture->mvInfo[neighbourMb[0].posY][neighbourMb[0].posX].mv[LIST_0];
      a_mv_y = a_mv->mvY;
      a_ref_idx = picture->mvInfo[neighbourMb[0].posY][neighbourMb[0].posX].refIndex[LIST_0];
      }

    if (neighbourMb[1].available) {
      b_mv = &picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].mv[LIST_0];
      b_mv_y = b_mv->mvY;
      b_ref_idx = picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].refIndex[LIST_0];
      }
    }
  else {
    if (neighbourMb[0].available) {
      a_mv = &picture->mvInfo[neighbourMb[0].posY][neighbourMb[0].posX].mv[LIST_0];
      a_mv_y = a_mv->mvY;
      a_ref_idx = picture->mvInfo[neighbourMb[0].posY][neighbourMb[0].posX].refIndex[LIST_0];

      if (mb->mbField && !decoder->mbData[neighbourMb[0].mbIndex].mbField) {
        a_mv_y /= 2;
        a_ref_idx *= 2;
        }
      if (!mb->mbField && decoder->mbData[neighbourMb[0].mbIndex].mbField) {
        a_mv_y *= 2;
        a_ref_idx >>= 1;
        }
      }

    if (neighbourMb[1].available) {
      b_mv = &picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].mv[LIST_0];
      b_mv_y = b_mv->mvY;
      b_ref_idx = picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].refIndex[LIST_0];

      if (mb->mbField && !decoder->mbData[neighbourMb[1].mbIndex].mbField) {
        b_mv_y /= 2;
        b_ref_idx *= 2;
        }
      if (!mb->mbField && decoder->mbData[neighbourMb[1].mbIndex].mbField) {
        b_mv_y *= 2;
        b_ref_idx >>= 1;
        }
      }
    }

  zeroMotionLeft = !neighbourMb[0].available ?
                     1 : (a_ref_idx==0 && a_mv->mvX == 0 && a_mv_y == 0) ? 1 : 0;
  zeroMotionAbove = !neighbourMb[1].available ?
                     1 : (b_ref_idx==0 && b_mv->mvX == 0 && b_mv_y == 0) ? 1 : 0;

  mb->codedBlockPattern = 0;
  resetCoeffs (mb);

  if (zeroMotionAbove || zeroMotionLeft) {
    sPicMotion** dec_mv_info = &picture->mvInfo[img_block_y];
    sPicture* slicePic = slice->listX[listOffset][0];
    sPicMotion* mvInfo = NULL;

    for (j = 0; j < BLOCK_SIZE; ++j) {
      for (i = mb->blockX; i < mb->blockX + BLOCK_SIZE; ++i) {
        mvInfo = &dec_mv_info[j][i];
        mvInfo->refPic[LIST_0] = slicePic;
        mvInfo->mv [LIST_0] = kZeroMv;
        mvInfo->refIndex[LIST_0] = 0;
        }
      }
    }
  else {
    sPicMotion** dec_mv_info = &picture->mvInfo[img_block_y];
    sPicMotion* mvInfo = NULL;
    sPicture* slicePic = slice->listX[listOffset][0];
    mb->GetMVPredictor (mb, neighbourMb, &pred_mv, 0, picture->mvInfo, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

    // Set first block line (position img_block_y)
    for (j = 0; j < BLOCK_SIZE; ++j) {
      for (i = mb->blockX; i < mb->blockX + BLOCK_SIZE; ++i) {
        mvInfo = &dec_mv_info[j][i];
        mvInfo->refPic[LIST_0] = slicePic;
        mvInfo->mv[LIST_0] = pred_mv;
        mvInfo->refIndex[LIST_0] = 0;
        }
      }
    }
  }
//}}}
//{{{
static void resetMvInfoList (sPicMotion* mvInfo, int list, int sliceNum) {

  mvInfo->refPic[list] = NULL;
  mvInfo->mv[list] = kZeroMv;
  mvInfo->refIndex[list] = -1;
  mvInfo->slice_no = sliceNum;
  }
//}}}
//{{{
static void initMacroblockBasic (sMacroBlock* mb) {

  sPicMotion** mvInfo = &mb->slice->picture->mvInfo[mb->blockY];
  int slice_no = mb->slice->curSliceIndex;

  // reset vectors and pred. modes
  for (int j = 0; j < BLOCK_SIZE; ++j) {
    int i = mb->blockX;
    resetMvInfoList (*mvInfo + (i++), LIST_1, slice_no);
    resetMvInfoList (*mvInfo + (i++), LIST_1, slice_no);
    resetMvInfoList (*mvInfo + (i++), LIST_1, slice_no);
    resetMvInfoList (*(mvInfo++) + i, LIST_1, slice_no);
    }
  }
//}}}
//{{{
static void readSkipMacroblock (sMacroBlock* mb) {

  mb->lumaTransformSize8x8flag = false;
  if (mb->decoder->activePps->hasConstrainedIntraPred) {
    int mbNum = mb->mbIndexX;
    mb->slice->intraBlock[mbNum] = 0;
    }

  initMacroblockBasic (mb);
  skipMacroblocks (mb);
  }
//}}}

//{{{
static void readIntraMacroblock (sMacroBlock* mb) {

  mb->noMbPartLessThan8x8Flag = true;

  // transform size flag for INTRA_4x4 and INTRA_8x8 modes
  mb->lumaTransformSize8x8flag = false;

  initMacroblock (mb);
  readIpredModes (mb);
  mb->slice->readCBPcoeffs (mb);
  }
//}}}
//{{{
static void readIntra4x4macroblocCavlc (sMacroBlock* mb, const uint8_t* dpMap)
{
  cSlice* slice = mb->slice;

  //transform size flag for INTRA_4x4 and INTRA_8x8 modes
  if (slice->transform8x8Mode) {
    sSyntaxElement se;
    sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
    se.type = SE_HEADER;

    // read eCavlc transform_size_8x8Flag
    se.len = (int64_t)1;
    readsSyntaxElement_FLC (&se, dataPartition->stream);

    mb->lumaTransformSize8x8flag = (bool)se.value1;
    if (mb->lumaTransformSize8x8flag) {
      mb->mbType = I8MB;
      memset (&mb->b8mode, I8MB, 4 * sizeof(char));
      memset (&mb->b8pdir, -1, 4 * sizeof(char));
      }
    }
  else
    mb->lumaTransformSize8x8flag = false;

  initMacroblock (mb);
  readIpredModes (mb);
  slice->readCBPcoeffs (mb);
  }
//}}}
//{{{
static void readIntra4x4macroblockCabac (sMacroBlock* mb, const uint8_t* dpMap) {


  // transform size flag for INTRA_4x4 and INTRA_8x8 modes
  cSlice* slice = mb->slice;
  if (slice->transform8x8Mode) {
   sSyntaxElement se;
    sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
    se.type = SE_HEADER;
    se.reading = readMB_transform_sizeFlag_CABAC;

    // read eCavlc transform_size_8x8Flag
    if (dataPartition->stream->errorFlag) {
      se.len = (int64_t) 1;
      readsSyntaxElement_FLC (&se, dataPartition->stream);
      }
    else
      dataPartition->readSyntaxElement (mb, &se, dataPartition);

    mb->lumaTransformSize8x8flag = (bool)se.value1;
    if (mb->lumaTransformSize8x8flag) {
      mb->mbType = I8MB;
      memset (&mb->b8mode, I8MB, 4 * sizeof(char));
      memset (&mb->b8pdir, -1, 4 * sizeof(char));
      }
    }
  else
    mb->lumaTransformSize8x8flag = false;

  initMacroblock (mb);
  readIpredModes (mb);
  slice->readCBPcoeffs (mb);
  }
//}}}

//{{{
static void readInterMacroblock (sMacroBlock* mb) {

  cSlice* slice = mb->slice;

  mb->noMbPartLessThan8x8Flag = true;
  mb->lumaTransformSize8x8flag = false;
  if (mb->decoder->activePps->hasConstrainedIntraPred) {
    int mbNum = mb->mbIndexX;
    slice->intraBlock[mbNum] = 0;
    }

  initMacroblock (mb);
  slice->nalReadMotionInfo (mb);
  slice->readCBPcoeffs (mb);
  }
//}}}
//{{{
static void readIPCMmacroblock (sMacroBlock* mb, const uint8_t* dpMap) {

  cSlice* slice = mb->slice;
  mb->noMbPartLessThan8x8Flag = true;
  mb->lumaTransformSize8x8flag = false;

  initMacroblock (mb);

  // here dataPartition is assigned with the same dataPartition as SE_MBTYPE, because IPCM syntax is in the
  // same category as MBTYPE
  if (slice->dataPartitionMode && slice->noDataPartitionB )
    concealIPCMcoeffs (mb);
  else {
    sDataPartition* dataPartition = &(slice->dataPartitions[dpMap[SE_LUM_DC_INTRA]]);
    readIPCMcoeffs (slice, dataPartition);
    }
  }
//}}}
//{{{
static void readI8x8macroblock (sMacroBlock* mb, sDataPartition* dataPartition, sSyntaxElement* se) {

  int i;
  cSlice* slice = mb->slice;

  // READ 8x8 SUB-dataPartition MODES (modes of 8x8 blocks) and Intra VBST block modes
  mb->noMbPartLessThan8x8Flag = true;
  mb->lumaTransformSize8x8flag = false;

  for (i = 0; i < 4; ++i) {
    dataPartition->readSyntaxElement (mb, se, dataPartition);
    SetB8Mode (mb, se->value1, i);

    // set noMbPartLessThan8x8Flag for P8x8 mode
    mb->noMbPartLessThan8x8Flag &= (mb->b8mode[i] == 0 && slice->activeSps->isDirect8x8inference) ||
      (mb->b8mode[i] == 4);
    }

  initMacroblock (mb);
  slice->nalReadMotionInfo (mb);

  if (mb->decoder->activePps->hasConstrainedIntraPred) {
    int mbNum = mb->mbIndexX;
    slice->intraBlock[mbNum] = 0;
    }

  slice->readCBPcoeffs (mb);
  }
//}}}

//{{{
static void readIcavlcMacroblock (sMacroBlock* mb) {

  cSlice* slice = mb->slice;

  sSyntaxElement se;
  int mbNum = mb->mbIndexX;

  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
  sPicture* picture = slice->picture;
  sPicMotionOld* motion = &picture->motion;

  mb->mbField = ((mbNum & 0x01) == 0) ? false : slice->mbData[mbNum-1].mbField;

  updateQp (mb, slice->qp);
  se.type = SE_MBTYPE;

  // read MB mode
  sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
  se.mapping = linfo_ue;

  // read MB aff
  if (slice->mbAffFrame && (mbNum & 0x01) == 0) {
    se.len = (int64_t) 1;
    readsSyntaxElement_FLC (&se, dataPartition->stream);
    mb->mbField = (bool)se.value1;
    }

  // read MB type
  dataPartition->readSyntaxElement (mb, &se, dataPartition);

  mb->mbType = (int16_t) se.value1;
  if (!dataPartition->stream->errorFlag)
    mb->errorFlag = 0;

  motion->mbField[mbNum] = (uint8_t) mb->mbField;
  mb->blockYaff = ((slice->mbAffFrame) && (mb->mbField)) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
  slice->siBlock[mb->mb.y][mb->mb.x] = 0;
  slice->interpretMbMode (mb);

  mb->noMbPartLessThan8x8Flag = true;
  if(mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblocCavlc (mb, dpMap);
  else
    readIntraMacroblock (mb);
  }
//}}}
//{{{
static void readPcavlcMacroblock (sMacroBlock* mb) {

  cSlice* slice = mb->slice;
  sSyntaxElement se;
  int mbNum = mb->mbIndexX;

  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

  if (slice->mbAffFrame == 0) {
    sPicture* picture = slice->picture;
    sPicMotionOld* motion = &picture->motion;

    mb->mbField = false;
    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    se.mapping = linfo_ue;

    // VLC Non-Intra
    if (slice->codCount == -1) {
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      slice->codCount = se.value1;
      }

    if (slice->codCount==0) {
      // read MB type
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      ++(se.value1);
      mb->mbType = (int16_t)se.value1;
      if(!dataPartition->stream->errorFlag)
        mb->errorFlag = 0;
      slice->codCount--;
      mb->skipFlag = 0;
      }
    else {
      slice->codCount--;
      mb->mbType = 0;
      mb->errorFlag = 0;
      mb->skipFlag = 1;
      }

    // update the list offset;
    mb->listOffset = 0;
    motion->mbField[mbNum] = (uint8_t) false;
    mb->blockYaff = mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    }
  else {
    cDecoder264* decoder = mb->decoder;
    sMacroBlock* topMB = NULL;
    int  prevMbSkipped = 0;
    sPicture* picture = slice->picture;
    sPicMotionOld* motion = &picture->motion;

    if (mbNum & 0x01) {
      topMB= &decoder->mbData[mbNum-1];
      prevMbSkipped = (topMB->mbType == 0);
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0)? false : decoder->mbData[mbNum-1].mbField;
    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    se.mapping = linfo_ue;

    // VLC Non-Intra
    if (slice->codCount == -1) {
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      slice->codCount = se.value1;
      }

    if (slice->codCount == 0) {
      // read MB aff
      if ((((mbNum & 0x01) == 0) || ((mbNum & 0x01) && prevMbSkipped))) {
        se.len = (int64_t) 1;
        readsSyntaxElement_FLC (&se, dataPartition->stream);
        mb->mbField = (bool)se.value1;
        }

      // read MB type
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      ++(se.value1);
      mb->mbType = (int16_t)se.value1;
      if(!dataPartition->stream->errorFlag)
        mb->errorFlag = 0;
      slice->codCount--;
      mb->skipFlag = 0;
      }
    else {
      slice->codCount--;
      mb->mbType = 0;
      mb->errorFlag = 0;
      mb->skipFlag = 1;

      // read field flag of bottom block
      if (slice->codCount == 0 && ((mbNum & 0x01) == 0)) {
        se.len = (int64_t) 1;
        readsSyntaxElement_FLC (&se, dataPartition->stream);
        dataPartition->stream->bitStreamOffset--;
        mb->mbField = (bool)se.value1;
        }
      else if (slice->codCount > 0 && ((mbNum & 0x01) == 0)) {
        // check left macroblock pair first
        if (isMbAvailable(mbNum - 2, mb) && ((mbNum % (decoder->coding.picWidthMbs * 2))!=0))
          mb->mbField = decoder->mbData[mbNum-2].mbField;
        else {
          // check top macroblock pair
          if (isMbAvailable (mbNum - 2*decoder->coding.picWidthMbs, mb))
            mb->mbField = decoder->mbData[mbNum - 2*decoder->coding.picWidthMbs].mbField;
          else
            mb->mbField = false;
          }
        }
      }
    // update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;
    motion->mbField[mbNum] = (uint8_t) mb->mbField;
    mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    if (mb->mbField) {
      slice->numRefIndexActive[LIST_0] <<=1;
      slice->numRefIndexActive[LIST_1] <<=1;
      }
    }

  mb->noMbPartLessThan8x8Flag = true;
  if (mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblocCavlc (mb, dpMap);
  else if (mb->mbType == P8x8) {
    se.type = SE_MBTYPE;
    se.mapping = linfo_ue;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    readI8x8macroblock (mb, dataPartition, &se);
    }
  else if (mb->mbType == PSKIP)
    readSkipMacroblock (mb);
  else if (mb->isIntraBlock)
    readIntraMacroblock (mb);
  else
    readInterMacroblock (mb);
  }
//}}}
//{{{
static void readBcavlcMacroblock (sMacroBlock* mb) {

  cDecoder264* decoder = mb->decoder;
  cSlice* slice = mb->slice;
  int mbNum = mb->mbIndexX;
  sSyntaxElement se;
  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

  if (slice->mbAffFrame == 0) {
    sPicture* picture = slice->picture;
    sPicMotionOld *motion = &picture->motion;

    mb->mbField = false;
    updateQp(mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    se.mapping = linfo_ue;

    if(slice->codCount == -1) {
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      slice->codCount = se.value1;
      }

    if (slice->codCount==0) {
      // read MB type
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (int16_t)se.value1;
      if (!dataPartition->stream->errorFlag)
        mb->errorFlag = 0;
      slice->codCount--;
      mb->skipFlag = 0;
      }
    else {
      slice->codCount--;
      mb->mbType = 0;
      mb->errorFlag = 0;
      mb->skipFlag = 1;
      }

    // update the list offset;
    mb->listOffset = 0;
    motion->mbField[mbNum] = false;
    mb->blockYaff = mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    }
  else {
    sMacroBlock* topMB = NULL;
    int prevMbSkipped = 0;
    sPicture* picture = slice->picture;
    sPicMotionOld* motion = &picture->motion;

    if (mbNum & 0x01) {
      topMB = &decoder->mbData[mbNum-1];
      prevMbSkipped = topMB->skipFlag;
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0) ? false : decoder->mbData[mbNum-1].mbField;

    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    se.mapping = linfo_ue;
    if (slice->codCount == -1) {
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      slice->codCount = se.value1;
      }

    if (slice->codCount == 0) {
      // read MB aff
      if (((mbNum & 0x01) == 0) || ((mbNum & 0x01) && prevMbSkipped)) {
        se.len = (int64_t) 1;
        readsSyntaxElement_FLC (&se, dataPartition->stream);
        mb->mbField = (bool)se.value1;
        }

      // read MB type
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (int16_t)se.value1;
      if (!dataPartition->stream->errorFlag)
        mb->errorFlag = 0;
      slice->codCount--;
      mb->skipFlag = 0;
      }
    else {
      slice->codCount--;
      mb->mbType = 0;
      mb->errorFlag = 0;
      mb->skipFlag = 1;

      // read field flag of bottom block
      if ((slice->codCount == 0) && ((mbNum & 0x01) == 0)) {
        se.len = (int64_t) 1;
        readsSyntaxElement_FLC (&se, dataPartition->stream);
        dataPartition->stream->bitStreamOffset--;
        mb->mbField = (bool)se.value1;
        }
      else if ((slice->codCount > 0) && ((mbNum & 0x01) == 0)) {
        // check left macroblock pair first
        if (isMbAvailable (mbNum - 2, mb) && ((mbNum % (decoder->coding.picWidthMbs * 2))!=0))
          mb->mbField = decoder->mbData[mbNum-2].mbField;
        else {
          // check top macroblock pair
          if (isMbAvailable (mbNum - 2*decoder->coding.picWidthMbs, mb))
            mb->mbField = decoder->mbData[mbNum-2*decoder->coding.picWidthMbs].mbField;
          else
            mb->mbField = false;
          }
        }
      }

    // update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;
    motion->mbField[mbNum] = (uint8_t)mb->mbField;
    mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    if (slice->mbAffFrame) {
      if (mb->mbField) {
        slice->numRefIndexActive[LIST_0] <<=1;
        slice->numRefIndexActive[LIST_1] <<=1;
        }
      }
    }

  if (mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblocCavlc (mb, dpMap);
  else if (mb->mbType == P8x8) {
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    se.type = SE_MBTYPE;
    se.mapping = linfo_ue;
    readI8x8macroblock (mb, dataPartition, &se);
    }
  else if (mb->mbType == BSKIP_DIRECT) {
    // init noMbPartLessThan8x8Flag
    mb->noMbPartLessThan8x8Flag = (!(slice->activeSps->isDirect8x8inference))? false: true;
    mb->lumaTransformSize8x8flag = false;
    if(decoder->activePps->hasConstrainedIntraPred)
      slice->intraBlock[mbNum] = 0;

    initMacroblockDirect (mb);
    if (slice->codCount >= 0) {
      mb->codedBlockPattern = 0;
      resetCoeffs (mb);
      }
    else
      slice->readCBPcoeffs (mb);
    }
  else if (mb->isIntraBlock == true)
    readIntraMacroblock (mb);
  else
    readInterMacroblock (mb);
  }
//}}}

//{{{
static void readIcabacMacroblock (sMacroBlock* mb) {

  cSlice* slice = mb->slice;

  sSyntaxElement se;
  int mbNum = mb->mbIndexX;

  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
  sPicture* picture = slice->picture;
  sPicMotionOld* motion = &picture->motion;

  mb->mbField = ((mbNum & 0x01) == 0) ? false : slice->mbData[mbNum-1].mbField;

  updateQp (mb, slice->qp);

  //  read MB mode
  se.type = SE_MBTYPE;
  sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
  if (dataPartition->stream->errorFlag)
    se.mapping = linfo_ue;

  // read MB aff
  if (slice->mbAffFrame && (mbNum & 0x01) == 0) {
    if (dataPartition->stream->errorFlag) {
      se.len = (int64_t)1;
      readsSyntaxElement_FLC (&se, dataPartition->stream);
      }
    else {
      se.reading = readFieldModeInfo_CABAC;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      }
    mb->mbField = (bool)se.value1;
    }

  checkNeighbourCabac(mb);

  //  read MB type
  se.reading = readMB_typeInfo_CABAC_i_slice;
  dataPartition->readSyntaxElement (mb, &se, dataPartition);

  mb->mbType = (int16_t)se.value1;
  if (!dataPartition->stream->errorFlag)
    mb->errorFlag = 0;

  motion->mbField[mbNum] = (uint8_t) mb->mbField;
  mb->blockYaff = ((slice->mbAffFrame) && (mb->mbField)) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
  slice->siBlock[mb->mb.y][mb->mb.x] = 0;
  slice->interpretMbMode (mb);

  // init noMbPartLessThan8x8Flag
  mb->noMbPartLessThan8x8Flag = true;
  if (mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB) {
    // transform size flag for INTRA_4x4 and INTRA_8x8 modes
    if (slice->transform8x8Mode) {
      se.type = SE_HEADER;
      dataPartition = &(slice->dataPartitions[dpMap[SE_HEADER]]);
      se.reading = readMB_transform_sizeFlag_CABAC;

      // read eCavlc transform_size_8x8Flag
      if (dataPartition->stream->errorFlag) {
        se.len = (int64_t) 1;
        readsSyntaxElement_FLC (&se, dataPartition->stream);
        }
      else
        dataPartition->readSyntaxElement (mb, &se, dataPartition);

      mb->lumaTransformSize8x8flag = (bool)se.value1;
      if (mb->lumaTransformSize8x8flag) {
        mb->mbType = I8MB;
        memset (&mb->b8mode, I8MB, 4 * sizeof(char));
        memset (&mb->b8pdir, -1, 4 * sizeof(char));
        }
      }
    else
      mb->lumaTransformSize8x8flag = false;

    initMacroblock (mb);
    readIpredModes (mb);
    slice->readCBPcoeffs (mb);
    }
  else
    readIntraMacroblock (mb);
  }
//}}}
//{{{
static void readPcabacMacroblock (sMacroBlock* mb)
{
  cSlice* slice = mb->slice;
  cDecoder264* decoder = mb->decoder;
  int mbNum = mb->mbIndexX;
  sSyntaxElement se;
  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

  if (slice->mbAffFrame == 0) {
    sPicture* picture = slice->picture;
    sPicMotionOld* motion = &picture->motion;

    mb->mbField = false;
    updateQp (mb, slice->qp);

    // read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    if (dataPartition->stream->errorFlag)
      se.mapping = linfo_ue;

    checkNeighbourCabac(mb);
    se.reading = read_skipFlag_CABAC_p_slice;
    dataPartition->readSyntaxElement (mb, &se, dataPartition);

    mb->mbType = (int16_t) se.value1;
    mb->skipFlag = (char) (!(se.value1));
    if (!dataPartition->stream->errorFlag)
      mb->errorFlag = 0;

    // read MB type
    if (mb->mbType != 0 ) {
      se.reading = readMB_typeInfo_CABAC_p_slice;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (int16_t) se.value1;
      if(!dataPartition->stream->errorFlag)
        mb->errorFlag = 0;
      }

    motion->mbField[mbNum] = (uint8_t) false;
    mb->blockYaff = mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    }

  else {
    sMacroBlock* topMB = NULL;
    int prevMbSkipped = 0;
    int checkBot, readBot, readTop;
    sPicture* picture = slice->picture;
    sPicMotionOld* motion = &picture->motion;
    if (mbNum & 0x01) {
      topMB = &decoder->mbData[mbNum-1];
      prevMbSkipped = (topMB->mbType == 0);
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0) ? false : decoder->mbData[mbNum-1].mbField;
    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    if (dataPartition->stream->errorFlag)
      se.mapping = linfo_ue;

    // read MB skipFlag
    if (((mbNum & 0x01) == 0||prevMbSkipped))
      fieldFlagInference (mb);

    checkNeighbourCabac(mb);
    se.reading = read_skipFlag_CABAC_p_slice;
    dataPartition->readSyntaxElement (mb, &se, dataPartition);

    mb->mbType = (int16_t)se.value1;
    mb->skipFlag = (char)(!(se.value1));

    if (!dataPartition->stream->errorFlag)
      mb->errorFlag = 0;

    // read MB AFF
    checkBot = readBot = readTop = 0;
    if ((mbNum & 0x01) == 0) {
      checkBot = mb->skipFlag;
      readTop = !checkBot;
      }
    else
      readBot = (topMB->skipFlag && (!mb->skipFlag));

    if (readBot || readTop) {
      se.reading = readFieldModeInfo_CABAC;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbField = (bool)se.value1;
      }

    if (checkBot)
      checkNextMbGetFieldModeCabacSliceP (slice, &se, dataPartition);

    // update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4 : 2) : 0;
    checkNeighbourCabac (mb);

    // read MB type
    if (mb->mbType != 0 ) {
      se.reading = readMB_typeInfo_CABAC_p_slice;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (int16_t) se.value1;
      if (!dataPartition->stream->errorFlag)
        mb->errorFlag = 0;
      }

    motion->mbField[mbNum] = (uint8_t) mb->mbField;
    mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode(mb);
    if (mb->mbField) {
      slice->numRefIndexActive[LIST_0] <<=1;
      slice->numRefIndexActive[LIST_1] <<=1;
      }
    }

  mb->noMbPartLessThan8x8Flag = true;
  if (mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblockCabac (mb, dpMap);
  else if (mb->mbType == P8x8) {
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    se.type = SE_MBTYPE;

    if (dataPartition->stream->errorFlag)
      se.mapping = linfo_ue;
    else
      se.reading = readB8_typeInfo_CABAC_p_slice;

    readI8x8macroblock (mb, dataPartition, &se);
    }
  else if (mb->mbType == PSKIP)
    readSkipMacroblock (mb);
  else if (mb->isIntraBlock == true)
    readIntraMacroblock (mb);
  else
    readInterMacroblock (mb);
  }
//}}}
//{{{
static void readBcabacMacroblock (sMacroBlock* mb) {

  cSlice* slice = mb->slice;
  cDecoder264* decoder = mb->decoder;
  int mbNum = mb->mbIndexX;
  sSyntaxElement se;

  const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];

  if (slice->mbAffFrame == 0) {
    sPicture* picture = slice->picture;
    sPicMotionOld* motion = &picture->motion;

    mb->mbField = false;
    updateQp(mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    if (dataPartition->stream->errorFlag)
      se.mapping = linfo_ue;

    checkNeighbourCabac(mb);
    se.reading = read_skipFlag_CABAC_b_slice;
    dataPartition->readSyntaxElement (mb, &se, dataPartition);

    mb->mbType  = (int16_t)se.value1;
    mb->skipFlag = (char)(!(se.value1));
    mb->codedBlockPattern = se.value2;
    if (!dataPartition->stream->errorFlag)
      mb->errorFlag = 0;

    if (se.value1 == 0 && se.value2 == 0)
      slice->codCount=0;

    // read MB type
    if (mb->mbType != 0 ) {
      se.reading = readMB_typeInfo_CABAC_b_slice;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (int16_t)se.value1;
      if (!dataPartition->stream->errorFlag)
        mb->errorFlag = 0;
      }

    motion->mbField[mbNum] = (uint8_t) false;
    mb->blockYaff = mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    }
  else {
    sMacroBlock* topMB = NULL;
    int  prevMbSkipped = 0;
    int  checkBot, readBot, readTop;
    sPicture* picture = slice->picture;
    sPicMotionOld* motion = &picture->motion;

    if (mbNum & 0x01) {
      topMB = &decoder->mbData[mbNum-1];
      prevMbSkipped = topMB->skipFlag;
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0)? false : decoder->mbData[mbNum-1].mbField;

    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    if (dataPartition->stream->errorFlag)
      se.mapping = linfo_ue;

    // read MB skipFlag
    if ((mbNum & 0x01) == 0 || prevMbSkipped)
      fieldFlagInference (mb);

    checkNeighbourCabac (mb);
    se.reading = read_skipFlag_CABAC_b_slice;

    dataPartition->readSyntaxElement (mb, &se, dataPartition);
    mb->mbType = (int16_t)se.value1;
    mb->skipFlag = (char)(!(se.value1));
    mb->codedBlockPattern = se.value2;
    if (!dataPartition->stream->errorFlag)
      mb->errorFlag = 0;
    if (se.value1 == 0 && se.value2 == 0)
      slice->codCount = 0;

    // read MB AFF
    checkBot=readBot = readTop = 0;
    if ((mbNum & 0x01) == 0) {
      checkBot = mb->skipFlag;
      readTop = !checkBot;
      }
    else
      readBot = topMB->skipFlag && (!mb->skipFlag);

    if (readBot || readTop) {
      se.reading = readFieldModeInfo_CABAC;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbField = (bool)se.value1;
      }
    if (checkBot)
      checkNextMbGetFieldModeCabacSliceB (slice, &se, dataPartition);

    //update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;

    checkNeighbourCabac (mb);

    // read MB type
    if (mb->mbType != 0 ) {
      se.reading = readMB_typeInfo_CABAC_b_slice;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (int16_t)se.value1;
      if(!dataPartition->stream->errorFlag)
        mb->errorFlag = 0;
      }

    motion->mbField[mbNum] = (uint8_t) mb->mbField;
    mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    if (mb->mbField) {
      slice->numRefIndexActive[LIST_0] <<=1;
      slice->numRefIndexActive[LIST_1] <<=1;
      }
    }

  if (mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblockCabac (mb, dpMap);
  else if (mb->mbType == P8x8) {
    sDataPartition* dataPartition = &slice->dataPartitions[dpMap[SE_MBTYPE]];
    se.type = SE_MBTYPE;
    if (dataPartition->stream->errorFlag)
      se.mapping = linfo_ue;
    else
      se.reading = readB8_typeInfo_CABAC_b_slice;
    readI8x8macroblock(mb, dataPartition, &se);
    }
  else if (mb->mbType == BSKIP_DIRECT) {
    //init noMbPartLessThan8x8Flag
    mb->noMbPartLessThan8x8Flag = (!(slice->activeSps->isDirect8x8inference)) ? false: true;

    // transform size flag for INTRA_4x4 and INTRA_8x8 modes
    mb->lumaTransformSize8x8flag = false;
    if(decoder->activePps->hasConstrainedIntraPred)
      slice->intraBlock[mbNum] = 0;

    initMacroblockDirect (mb);
    if (slice->codCount >= 0) {
      slice->isResetCoef = true;
      mb->codedBlockPattern = 0;
      slice->codCount = -1;
      }
    else // read CBP and Coeffs
      slice->readCBPcoeffs (mb);
    }
  else if (mb->isIntraBlock == true)
    readIntraMacroblock (mb);
  else
    readInterMacroblock (mb);
  }
//}}}

//{{{
void setReadMacroblock (cSlice* slice) {

  if (slice->decoder->activePps->entropyCoding == eCabac) {
    switch (slice->sliceType) {
      case eSliceP:
      case eSliceSP:
        slice->readMacroblock = readPcabacMacroblock;
        break;
      case eSliceB:
        slice->readMacroblock = readBcabacMacroblock;
        break;
      case eSliceI:
      case eSliceSI:
        slice->readMacroblock = readIcabacMacroblock;
        break;
      default:
        printf ("Unsupported slice type\n");
        break;
      }
    }

  else {
    switch (slice->sliceType) {
      case eSliceP:
      case eSliceSP:
        slice->readMacroblock = readPcavlcMacroblock;
        break;
      case eSliceB:
        slice->readMacroblock = readBcavlcMacroblock;
        break;
      case eSliceI:
      case eSliceSI:
        slice->readMacroblock = readIcavlcMacroblock;
        break;
      default:
        printf ("Unsupported slice type\n");
        break;
      }
    }
  }
//}}}
