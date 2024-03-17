//{{{  includes
#include <math.h>

#include "global.h"
#include "elements.h"

#include "block.h"
#include "buffer.h"
#include "macroblock.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "image.h"
#include "mbAccess.h"
#include "biariDecode.h"
#include "transform.h"
#include "mcPred.h"
#include "quant.h"
#include "mbPred.h"
//}}}

//{{{
static void read_ipred_8x8_modes_mbaff (sMacroblock* mb) {

  int bi, bj, bx, by, dec;
  sSlice* slice = mb->slice;
  const byte* dpMap = assignSE2dp[slice->datadpMode];
  sDecoder* decoder = mb->decoder;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_block, top_block;

  sSyntaxElement se;
  se.type = SE_INTRAPREDMODE;
  sDataPartition* dataPartition = &(slice->dps[dpMap[SE_INTRAPREDMODE]]);

  if (!(decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag))
    se.reading = readIntraPredMode_CABAC;

  for (int b8 = 0; b8 < 4; ++b8)  {
    // loop 8x8 blocks
    by = (b8 & 0x02);
    bj = mb->blockY + by;

    bx = ((b8 & 0x01) << 1);
    bi = mb->blockX + bx;
    // get from stream
    if (decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag)
      readsSyntaxElement_Intra4x4PredictionMode (&se, dataPartition->s);
    else {
      se.context = b8 << 2;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      }

    get4x4Neighbour (mb, (bx << 2) - 1, (by << 2),     decoder->mbSize[IS_LUMA], &left_block);
    get4x4Neighbour (mb, (bx << 2),     (by << 2) - 1, decoder->mbSize[IS_LUMA], &top_block );

    // get from array and decode
    if (decoder->activePPS->constrainedIntraPredFlag) {
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
    slice->predMode[bj][bi] = (byte) dec;
    slice->predMode[bj][bi+1] = (byte) dec;
    slice->predMode[bj+1][bi] = (byte) dec;
    slice->predMode[bj+1][bi+1] = (byte) dec;
    }
  }
//}}}
//{{{
static void read_ipred_8x8_modes (sMacroblock* mb) {

  int b8, bi, bj, bx, by, dec;
  sSlice* slice = mb->slice;
  const byte* dpMap = assignSE2dp[slice->datadpMode];
  sDecoder* decoder = mb->decoder;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  sSyntaxElement se;
  se.type = SE_INTRAPREDMODE;
  sDataPartition* dataPartition = &(slice->dps[dpMap[SE_INTRAPREDMODE]]);
  if (!(decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag))
    se.reading = readIntraPredMode_CABAC;

  get4x4Neighbour (mb, -1,  0, decoder->mbSize[IS_LUMA], &left_mb);
  get4x4Neighbour (mb,  0, -1, decoder->mbSize[IS_LUMA], &top_mb );

  for(b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
    by = (b8 & 0x02);
    bj = mb->blockY + by;

    bx = ((b8 & 0x01) << 1);
    bi = mb->blockX + bx;

    //get from stream
    if (decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag)
      readsSyntaxElement_Intra4x4PredictionMode (&se, dataPartition->s);
    else {
      se.context = (b8 << 2);
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
    }

    get4x4Neighbour (mb, (bx << 2) - 1, by << 2, decoder->mbSize[IS_LUMA], &left_block);
    get4x4Neighbour (mb, bx << 2, (by << 2) - 1, decoder->mbSize[IS_LUMA], &top_block);

    // get from array and decode
    if (decoder->activePPS->constrainedIntraPredFlag) {
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
    slice->predMode[bj][bi] = (byte)dec;
    slice->predMode[bj][bi+1] = (byte)dec;
    slice->predMode[bj+1][bi] = (byte)dec;
    slice->predMode[bj+1][bi+1] = (byte)dec;
  }
}
//}}}
//{{{
static void read_ipred_4x4_modes_mbaff (sMacroblock* mb) {

  int b8,i,j,bi,bj,bx,by;
  sSyntaxElement se;
  sSlice* slice = mb->slice;
  const byte *dpMap = assignSE2dp[slice->datadpMode];
  sDecoder *decoder = mb->decoder;
  sBlockPos *picPos = decoder->picPos;

  int ts, ls;
  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;
  sPixelPos left_block, top_block;

  se.type = SE_INTRAPREDMODE;
  sDataPartition* dataPartition = &(slice->dps[dpMap[SE_INTRAPREDMODE]]);
  if (!(decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag))
    se.reading = readIntraPredMode_CABAC;

  for (b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
    for (j = 0; j < 2; j++) { //loop subblocks
      by = (b8 & 0x02) + j;
      bj = mb->blockY + by;

      for(i = 0; i < 2; i++) {
        bx = ((b8 & 1) << 1) + i;
        bi = mb->blockX + bx;
        //get from stream
        if (decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag)
          readsSyntaxElement_Intra4x4PredictionMode (&se, dataPartition->s);
        else {
          se.context = (b8<<2) + (j<<1) +i;
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
          }

        get4x4Neighbour (mb, (bx<<2) - 1, (by<<2),     decoder->mbSize[IS_LUMA], &left_block);
        get4x4Neighbour (mb, (bx<<2),     (by<<2) - 1, decoder->mbSize[IS_LUMA], &top_block );

        // get from array and decode
        if (decoder->activePPS->constrainedIntraPredFlag) {
          left_block.available = left_block.available ? slice->intraBlock[left_block.mbIndex] : 0;
          top_block.available = top_block.available  ? slice->intraBlock[top_block.mbIndex]  : 0;
          }

        // !! KS: not sure if the following is still correct...
        ts = ls = 0;   // Check to see if the neighboring block is SI
        if (slice->sliceType == SI_SLICE) { // need support for MBINTLC1
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
        slice->predMode[bj][bi] = (byte) ((se.value1 == -1) ?
          mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode));
        }
      }
    }
  }
//}}}
//{{{
static void read_ipred_4x4_modes (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  const byte* dpMap = assignSE2dp[slice->datadpMode];
  sDecoder* decoder = mb->decoder;
  sBlockPos* picPos = decoder->picPos;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  sSyntaxElement se;
  se.type = SE_INTRAPREDMODE;
  sDataPartition* dataPartition = &(slice->dps[dpMap[SE_INTRAPREDMODE]]);
  if (!(decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag))
    se.reading = readIntraPredMode_CABAC;

  get4x4Neighbour (mb, -1,  0, decoder->mbSize[IS_LUMA], &left_mb);
  get4x4Neighbour (mb,  0, -1, decoder->mbSize[IS_LUMA], &top_mb );

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
        if (decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag)
          readsSyntaxElement_Intra4x4PredictionMode (&se, dataPartition->s);
        else {
          se.context=(b8<<2) + (j<<1) +i;
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
          }

        get4x4Neighbour(mb, (bx<<2) - 1, (by<<2), decoder->mbSize[IS_LUMA], &left_block);
        get4x4Neighbour(mb, (bx<<2), (by<<2) - 1, decoder->mbSize[IS_LUMA], &top_block );

        //get from array and decode
        if (decoder->activePPS->constrainedIntraPredFlag) {
          left_block.available = left_block.available ? slice->intraBlock[left_block.mbIndex] : 0;
          top_block.available = top_block.available  ? slice->intraBlock[top_block.mbIndex]  : 0;
          }

        int ts = 0;
        int ls = 0;   // Check to see if the neighboring block is SI
        if (slice->sliceType == SI_SLICE) {
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
        slice->predMode[bj][bi] = (byte)((se.value1 == -1) ?
          mostProbableIntraPredMode : se.value1 + (se.value1 >= mostProbableIntraPredMode));
        }
      }
    }
  }
//}}}
//{{{
static void readIpredModes (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture;

  if (slice->mbAffFrameFlag) {
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
    const byte* dpMap = assignSE2dp[slice->datadpMode];
    sDecoder* decoder = mb->decoder;

    se.type = SE_INTRAPREDMODE;
    dataPartition = &(slice->dps[dpMap[SE_INTRAPREDMODE]]);

    if (decoder->activePPS->entropyCodingMode == (Boolean)CAVLC || dataPartition->s->errorFlag)
      se.mapping = linfo_ue;
    else
      se.reading = readCIPredMode_CABAC;

    dataPartition->readSyntaxElement (mb, &se, dataPartition);
    mb->cPredMode = (char)se.value1;

    if (mb->cPredMode < DC_PRED_8 || mb->cPredMode > PLANE_8)
      error ("illegal chroma intra pred mode!\n");
    }
  }
//}}}

//{{{
static void resetMvInfo (sPicMotionParam* mvInfo, int slice_no) {

  mvInfo->refPic[LIST_0] = NULL;
  mvInfo->refPic[LIST_1] = NULL;
  mvInfo->mv[LIST_0] = zero_mv;
  mvInfo->mv[LIST_1] = zero_mv;
  mvInfo->refIndex[LIST_0] = -1;
  mvInfo->refIndex[LIST_1] = -1;
  mvInfo->slice_no = slice_no;
  }
//}}}
//{{{
static void initMacroblock (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sPicMotionParam** mvInfo = &slice->picture->mvInfo[mb->blockY];
  int slice_no = slice->curSliceIndex;

  // reset vectors and pred. modes
  for(int j = 0; j < BLOCK_SIZE; ++j) {
    int i = mb->blockX;
    resetMvInfo (*mvInfo + (i++), slice_no);
    resetMvInfo (*mvInfo + (i++), slice_no);
    resetMvInfo (*mvInfo + (i++), slice_no);
    resetMvInfo (*(mvInfo++) + i, slice_no);
    }

  set_read_comp_coeff_cabac (mb);
  set_read_comp_coeff_cavlc (mb);
  }
//}}}
//{{{
static void initMacroblockDirect (sMacroblock* mb) {

  int slice_no = mb->slice->curSliceIndex;
  sPicMotionParam** mvInfo = &mb->slice->picture->mvInfo[mb->blockY];

  set_read_comp_coeff_cabac (mb);
  set_read_comp_coeff_cavlc (mb);

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
static void concealIPCMcoeffs (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  for (int i = 0; i < MB_BLOCK_SIZE; ++i)
    for (int j = 0; j < MB_BLOCK_SIZE; ++j)
      slice->cof[0][i][j] = decoder->dcPredValueComp[0];

  sPicture* picture = slice->picture;
  if ((picture->chromaFormatIdc != YUV400) && (decoder->coding.sepColourPlaneFlag == 0))
    for (int k = 0; k < 2; ++k)
      for (int i = 0; i < decoder->mbCrSizeY; ++i)
        for (int j = 0; j < decoder->mbCrSizeX; ++j)
          slice->cof[k][i][j] = decoder->dcPredValueComp[k];
  }
//}}}
//{{{
static void initIPCMdecoding (sSlice* slice) {

  int dpNum;
  if (slice->datadpMode == PAR_DP_1)
    dpNum = 1;
  else if (slice->datadpMode == PAR_DP_3)
    dpNum = 3;
  else {
    printf ("dataPartition Mode is not supported\n");
    exit (1);
    }

  for (int i = 0; i < dpNum;++i) {
    sBitStream* stream = slice->dps[i].s;
    int byteStartPosition = stream->readLen;
    aridecoStartDecoding (&slice->dps[i].deCabac, stream->bitStreamBuffer, byteStartPosition, &stream->readLen);
    }
  }
//}}}
//{{{
static void readIPCMcoeffs (sSlice* slice, sDataPartition* dataPartition) {

  sDecoder* decoder = slice->decoder;

  //For CABAC, we don't need to read bits to let stream byte aligned
  //  because we have variable for integer bytes position
  if (decoder->activePPS->entropyCodingMode == (Boolean)CABAC) {
    readIPCMcabac (slice, dataPartition);
    initIPCMdecoding (slice);
    }
  else {
    // read bits to let stream byte aligned
    sSyntaxElement se;
    if (((dataPartition->s->bitStreamOffset) & 0x07) != 0) {
      se.len = (8 - ((dataPartition->s->bitStreamOffset) & 0x07));
      readsSyntaxElement_FLC (&se, dataPartition->s);
      }

    //read luma and chroma IPCM coefficients
    se.len = decoder->bitdepthLuma;
    for (int i = 0; i < MB_BLOCK_SIZE;++i)
      for (int j = 0; j < MB_BLOCK_SIZE;++j) {
        readsSyntaxElement_FLC (&se, dataPartition->s);
        slice->cof[0][i][j] = se.value1;
        }

    se.len = decoder->bitdepthChroma;
    sPicture* picture = slice->picture;
    if ((picture->chromaFormatIdc != YUV400) && (decoder->coding.sepColourPlaneFlag == 0)) {
      for (int i = 0; i < decoder->mbCrSizeY; ++i)
        for (int j = 0; j < decoder->mbCrSizeX; ++j) {
          readsSyntaxElement_FLC (&se, dataPartition->s);
          slice->cof[1][i][j] = se.value1;
          }

      for (int i = 0; i < decoder->mbCrSizeY; ++i)
        for (int j = 0; j < decoder->mbCrSizeX; ++j) {
          readsSyntaxElement_FLC (&se, dataPartition->s);
          slice->cof[2][i][j] = se.value1;
          }
      }
    }
  }
//}}}

//{{{
static void SetB8Mode (sMacroblock* mb, int value, int i) {

  sSlice* slice = mb->slice;
  static const char p_v2b8 [ 5] = {4, 5, 6, 7, IBLOCK};
  static const char p_v2pd [ 5] = {0, 0, 0, 0, -1};
  static const char b_v2b8 [14] = {0, 4, 4, 4, 5, 6, 5, 6, 5, 6, 7, 7, 7, IBLOCK};
  static const char b_v2pd [14] = {2, 0, 1, 2, 0, 0, 1, 1, 2, 2, 0, 1, 2, -1};

  if (slice->sliceType == B_SLICE) {
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
static void resetCoeffs (sMacroblock* mb) {

  sDecoder* decoder = mb->decoder;

  if (decoder->activePPS->entropyCodingMode == (Boolean)CAVLC)
    memset (decoder->nzCoeff[mb->mbIndexX][0][0], 0, 3 * BLOCK_PIXELS * sizeof(byte));
  }
//}}}
//{{{
static void fieldFlagInference (sMacroblock* mb) {

  sDecoder* decoder = mb->decoder;
  if (mb->mbAvailA)
    mb->mbField = decoder->mbData[mb->mbIndexA].mbField;
  else
    // check top macroblock pair
    mb->mbField = mb->mbAvailB ? decoder->mbData[mb->mbIndexB].mbField : FALSE;
  }
//}}}

//{{{
static void skipMacroblocks (sMacroblock* mb) {

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
  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  int   listOffset = LIST_0 + mb->listOffset;
  sPicture* picture = slice->picture;
  sMotionVec* a_mv = NULL;
  sMotionVec* b_mv = NULL;

  getNeighbours (mb, neighbourMb, 0, 0, MB_BLOCK_SIZE);
  if (slice->mbAffFrameFlag == 0) {
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

  mb->cbp = 0;
  resetCoeffs (mb);

  if (zeroMotionAbove || zeroMotionLeft) {
    sPicMotionParam** dec_mv_info = &picture->mvInfo[img_block_y];
    sPicture* slicePic = slice->listX[listOffset][0];
    sPicMotionParam* mvInfo = NULL;

    for (j = 0; j < BLOCK_SIZE; ++j) {
      for (i = mb->blockX; i < mb->blockX + BLOCK_SIZE; ++i) {
        mvInfo = &dec_mv_info[j][i];
        mvInfo->refPic[LIST_0] = slicePic;
        mvInfo->mv [LIST_0] = zero_mv;
        mvInfo->refIndex[LIST_0] = 0;
        }
      }
    }
  else {
    sPicMotionParam** dec_mv_info = &picture->mvInfo[img_block_y];
    sPicMotionParam* mvInfo = NULL;
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
static void resetMvInfoList (sPicMotionParam* mvInfo, int list, int sliceNum) {

  mvInfo->refPic[list] = NULL;
  mvInfo->mv[list] = zero_mv;
  mvInfo->refIndex[list] = -1;
  mvInfo->slice_no = sliceNum;
  }
//}}}
//{{{
static void initMacroblockBasic (sMacroblock* mb) {

  sPicMotionParam** mvInfo = &mb->slice->picture->mvInfo[mb->blockY];
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
static void readSkipMacroblock (sMacroblock* mb) {

  mb->lumaTransformSize8x8flag = FALSE;
  if (mb->decoder->activePPS->constrainedIntraPredFlag) {
    int mbNum = mb->mbIndexX;
    mb->slice->intraBlock[mbNum] = 0;
    }

  initMacroblockBasic (mb);
  skipMacroblocks (mb);
  }
//}}}

//{{{
static void readIntraMacroblock (sMacroblock* mb) {

  mb->noMbPartLessThan8x8Flag = TRUE;

  // transform size flag for INTRA_4x4 and INTRA_8x8 modes
  mb->lumaTransformSize8x8flag = FALSE;

  initMacroblock (mb);
  readIpredModes (mb);
  mb->slice->readCBPcoeffs (mb);
  }
//}}}
//{{{
static void readIntra4x4macroblocCavlc (sMacroblock* mb, const byte* dpMap)
{
  sSlice* slice = mb->slice;

  //transform size flag for INTRA_4x4 and INTRA_8x8 modes
  if (slice->transform8x8Mode) {
    sSyntaxElement se;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_HEADER]]);
    se.type = SE_HEADER;

    // read CAVLC transform_size_8x8_flag
    se.len = (int64)1;
    readsSyntaxElement_FLC (&se, dataPartition->s);

    mb->lumaTransformSize8x8flag = (Boolean)se.value1;
    if (mb->lumaTransformSize8x8flag) {
      mb->mbType = I8MB;
      memset (&mb->b8mode, I8MB, 4 * sizeof(char));
      memset (&mb->b8pdir, -1, 4 * sizeof(char));
      }
    }
  else
    mb->lumaTransformSize8x8flag = FALSE;

  initMacroblock (mb);
  readIpredModes (mb);
  slice->readCBPcoeffs (mb);
  }
//}}}
//{{{
static void readIntra4x4macroblockCabac (sMacroblock* mb, const byte* dpMap) {


  // transform size flag for INTRA_4x4 and INTRA_8x8 modes
  sSlice* slice = mb->slice;
  if (slice->transform8x8Mode) {
   sSyntaxElement se;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_HEADER]]);
    se.type = SE_HEADER;
    se.reading = readMB_transform_size_flag_CABAC;

    // read CAVLC transform_size_8x8_flag
    if (dataPartition->s->errorFlag) {
      se.len = (int64) 1;
      readsSyntaxElement_FLC (&se, dataPartition->s);
      }
    else
      dataPartition->readSyntaxElement (mb, &se, dataPartition);

    mb->lumaTransformSize8x8flag = (Boolean)se.value1;
    if (mb->lumaTransformSize8x8flag) {
      mb->mbType = I8MB;
      memset (&mb->b8mode, I8MB, 4 * sizeof(char));
      memset (&mb->b8pdir, -1, 4 * sizeof(char));
      }
    }
  else
    mb->lumaTransformSize8x8flag = FALSE;

  initMacroblock (mb);
  readIpredModes (mb);
  slice->readCBPcoeffs (mb);
  }
//}}}

//{{{
static void readInterMacroblock (sMacroblock* mb) {

  sSlice* slice = mb->slice;

  mb->noMbPartLessThan8x8Flag = TRUE;
  mb->lumaTransformSize8x8flag = FALSE;
  if (mb->decoder->activePPS->constrainedIntraPredFlag) {
    int mbNum = mb->mbIndexX;
    slice->intraBlock[mbNum] = 0;
    }

  initMacroblock (mb);
  slice->nalReadMotionInfo (mb);
  slice->readCBPcoeffs (mb);
  }
//}}}
//{{{
static void readIPCMmacroblock (sMacroblock* mb, const byte* dpMap) {

  sSlice* slice = mb->slice;
  mb->noMbPartLessThan8x8Flag = TRUE;
  mb->lumaTransformSize8x8flag = FALSE;

  initMacroblock (mb);

  // here dataPartition is assigned with the same dataPartition as SE_MBTYPE, because IPCM syntax is in the
  // same category as MBTYPE
  if (slice->datadpMode && slice->noDataPartitionB )
    concealIPCMcoeffs (mb);
  else {
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_LUM_DC_INTRA]]);
    readIPCMcoeffs (slice, dataPartition);
    }
  }
//}}}
//{{{
static void readI8x8macroblock (sMacroblock* mb, sDataPartition* dataPartition, sSyntaxElement* se) {

  int i;
  sSlice* slice = mb->slice;

  // READ 8x8 SUB-dataPartition MODES (modes of 8x8 blocks) and Intra VBST block modes
  mb->noMbPartLessThan8x8Flag = TRUE;
  mb->lumaTransformSize8x8flag = FALSE;

  for (i = 0; i < 4; ++i) {
    dataPartition->readSyntaxElement (mb, se, dataPartition);
    SetB8Mode (mb, se->value1, i);

    // set noMbPartLessThan8x8Flag for P8x8 mode
    mb->noMbPartLessThan8x8Flag &= (mb->b8mode[i] == 0 && slice->activeSPS->direct_8x8_inference_flag) ||
      (mb->b8mode[i] == 4);
    }

  initMacroblock (mb);
  slice->nalReadMotionInfo (mb);

  if (mb->decoder->activePPS->constrainedIntraPredFlag) {
    int mbNum = mb->mbIndexX;
    slice->intraBlock[mbNum] = 0;
    }

  slice->readCBPcoeffs (mb);
  }
//}}}

//{{{
static void readIcavlcMacroblock (sMacroblock* mb) {

  sSlice* slice = mb->slice;

  sSyntaxElement se;
  int mbNum = mb->mbIndexX;

  const byte* dpMap = assignSE2dp[slice->datadpMode];
  sPicture* picture = slice->picture;
  sPicMotionParamsOld* motion = &picture->motion;

  mb->mbField = ((mbNum & 0x01) == 0)? FALSE : slice->mbData[mbNum-1].mbField;

  updateQp (mb, slice->qp);
  se.type = SE_MBTYPE;

  // read MB mode
  sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
  se.mapping = linfo_ue;

  // read MB aff
  if (slice->mbAffFrameFlag && (mbNum & 0x01) == 0) {
    se.len = (int64) 1;
    readsSyntaxElement_FLC (&se, dataPartition->s);
    mb->mbField = (Boolean)se.value1;
    }

  // read MB type
  dataPartition->readSyntaxElement (mb, &se, dataPartition);

  mb->mbType = (short) se.value1;
  if (!dataPartition->s->errorFlag)
    mb->errorFlag = 0;

  motion->mbField[mbNum] = (byte) mb->mbField;
  mb->blockYaff = ((slice->mbAffFrameFlag) && (mb->mbField)) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
  slice->siBlock[mb->mb.y][mb->mb.x] = 0;
  slice->interpretMbMode (mb);

  mb->noMbPartLessThan8x8Flag = TRUE;
  if(mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblocCavlc (mb, dpMap);
  else
    readIntraMacroblock (mb);
  }
//}}}
//{{{
static void readPcavlcMacroblock (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sSyntaxElement se;
  int mbNum = mb->mbIndexX;

  const byte* dpMap = assignSE2dp[slice->datadpMode];

  if (slice->mbAffFrameFlag == 0) {
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    mb->mbField = FALSE;
    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    se.mapping = linfo_ue;

    // VLC Non-Intra
    if (slice->codCount == -1) {
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      slice->codCount = se.value1;
      }

    if (slice->codCount==0) {
      // read MB type
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      ++(se.value1);
      mb->mbType = (short) se.value1;
      if(!dataPartition->s->errorFlag)
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
    motion->mbField[mbNum] = (byte) FALSE;
    mb->blockYaff = mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    }
  else {
    sDecoder* decoder = mb->decoder;
    sMacroblock* topMB = NULL;
    int  prevMbSkipped = 0;
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    if (mbNum & 0x01) {
      topMB= &decoder->mbData[mbNum-1];
      prevMbSkipped = (topMB->mbType == 0);
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0)? FALSE : decoder->mbData[mbNum-1].mbField;
    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    se.mapping = linfo_ue;

    // VLC Non-Intra
    if (slice->codCount == -1) {
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      slice->codCount = se.value1;
      }

    if (slice->codCount == 0) {
      // read MB aff
      if ((((mbNum & 0x01)==0) || ((mbNum & 0x01) && prevMbSkipped))) {
        se.len = (int64) 1;
        readsSyntaxElement_FLC (&se, dataPartition->s);
        mb->mbField = (Boolean)se.value1;
        }

      // read MB type
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      ++(se.value1);
      mb->mbType = (short)se.value1;
      if(!dataPartition->s->errorFlag)
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
        se.len = (int64) 1;
        readsSyntaxElement_FLC (&se, dataPartition->s);
        dataPartition->s->bitStreamOffset--;
        mb->mbField = (Boolean)se.value1;
        }
      else if (slice->codCount > 0 && ((mbNum & 0x01) == 0)) {
        // check left macroblock pair first
        if (isMbAvailable(mbNum - 2, mb) && ((mbNum % (decoder->picWidthMbs * 2))!=0))
          mb->mbField = decoder->mbData[mbNum-2].mbField;
        else {
          // check top macroblock pair
          if (isMbAvailable (mbNum - 2*decoder->picWidthMbs, mb))
            mb->mbField = decoder->mbData[mbNum - 2*decoder->picWidthMbs].mbField;
          else
            mb->mbField = FALSE;
          }
        }
      }
    // update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;
    motion->mbField[mbNum] = (byte) mb->mbField;
    mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    if (mb->mbField) {
      slice->numRefIndexActive[LIST_0] <<=1;
      slice->numRefIndexActive[LIST_1] <<=1;
      }
    }

  mb->noMbPartLessThan8x8Flag = TRUE;
  if (mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblocCavlc (mb, dpMap);
  else if (mb->mbType == P8x8) {
    se.type = SE_MBTYPE;
    se.mapping = linfo_ue;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
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
static void readBcavlcMacroblock (sMacroblock* mb) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  int mbNum = mb->mbIndexX;
  sSyntaxElement se;
  const byte* dpMap = assignSE2dp[slice->datadpMode];

  if (slice->mbAffFrameFlag == 0) {
    sPicture* picture = slice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    mb->mbField = FALSE;
    updateQp(mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    se.mapping = linfo_ue;

    if(slice->codCount == -1) {
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      slice->codCount = se.value1;
      }

    if (slice->codCount==0) {
      // read MB type
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (short)se.value1;
      if (!dataPartition->s->errorFlag)
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
    motion->mbField[mbNum] = FALSE;
    mb->blockYaff = mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    }
  else {
    sMacroblock* topMB = NULL;
    int prevMbSkipped = 0;
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    if (mbNum & 0x01) {
      topMB = &decoder->mbData[mbNum-1];
      prevMbSkipped = topMB->skipFlag;
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0)? FALSE : decoder->mbData[mbNum-1].mbField;

    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    se.mapping = linfo_ue;
    if(slice->codCount == -1) {
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      slice->codCount = se.value1;
      }

    if (slice->codCount==0) {
      // read MB aff
      if (((mbNum & 0x01)==0) || ((mbNum & 0x01) && prevMbSkipped)) {
        se.len = (int64) 1;
        readsSyntaxElement_FLC (&se, dataPartition->s);
        mb->mbField = (Boolean)se.value1;
        }

      // read MB type
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (short)se.value1;
      if(!dataPartition->s->errorFlag)
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
        se.len = (int64) 1;
        readsSyntaxElement_FLC (&se, dataPartition->s);
        dataPartition->s->bitStreamOffset--;
        mb->mbField = (Boolean)se.value1;
        }
      else if ((slice->codCount > 0) && ((mbNum & 0x01) == 0)) {
        // check left macroblock pair first
        if (isMbAvailable (mbNum - 2, mb) && ((mbNum % (decoder->picWidthMbs * 2))!=0))
          mb->mbField = decoder->mbData[mbNum-2].mbField;
        else {
          // check top macroblock pair
          if (isMbAvailable (mbNum - 2*decoder->picWidthMbs, mb))
            mb->mbField = decoder->mbData[mbNum-2*decoder->picWidthMbs].mbField;
          else
            mb->mbField = FALSE;
          }
        }
      }

    // update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;
    motion->mbField[mbNum] = (byte)mb->mbField;
    mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    if (slice->mbAffFrameFlag) {
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
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    se.type = SE_MBTYPE;
    se.mapping = linfo_ue;
    readI8x8macroblock (mb, dataPartition, &se);
    }
  else if (mb->mbType == BSKIP_DIRECT) {
    // init noMbPartLessThan8x8Flag
    mb->noMbPartLessThan8x8Flag = (!(slice->activeSPS->direct_8x8_inference_flag))? FALSE: TRUE;
    mb->lumaTransformSize8x8flag = FALSE;
    if(decoder->activePPS->constrainedIntraPredFlag)
      slice->intraBlock[mbNum] = 0;

    initMacroblockDirect (mb);
    if (slice->codCount >= 0) {
      mb->cbp = 0;
      resetCoeffs (mb);
      }
    else
      slice->readCBPcoeffs (mb);
    }
  else if (mb->isIntraBlock == TRUE)
    readIntraMacroblock (mb);
  else
    readInterMacroblock (mb);
  }
//}}}

//{{{
static void readIcabacMacroblock (sMacroblock* mb) {

  sSlice* slice = mb->slice;

  sSyntaxElement se;
  int mbNum = mb->mbIndexX;

  const byte* dpMap = assignSE2dp[slice->datadpMode];
  sPicture* picture = slice->picture;
  sPicMotionParamsOld* motion = &picture->motion;

  mb->mbField = ((mbNum & 0x01) == 0) ? FALSE : slice->mbData[mbNum-1].mbField;

  updateQp (mb, slice->qp);

  //  read MB mode
  se.type = SE_MBTYPE;
  sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
  if (dataPartition->s->errorFlag)
    se.mapping = linfo_ue;

  // read MB aff
  if (slice->mbAffFrameFlag && (mbNum & 0x01)==0) {
    if (dataPartition->s->errorFlag) {
      se.len = (int64)1;
      readsSyntaxElement_FLC (&se, dataPartition->s);
      }
    else {
      se.reading = readFieldModeInfo_CABAC;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      }
    mb->mbField = (Boolean)se.value1;
    }

  checkNeighbourCabac(mb);

  //  read MB type
  se.reading = readMB_typeInfo_CABAC_i_slice;
  dataPartition->readSyntaxElement (mb, &se, dataPartition);

  mb->mbType = (short)se.value1;
  if (!dataPartition->s->errorFlag)
    mb->errorFlag = 0;

  motion->mbField[mbNum] = (byte) mb->mbField;
  mb->blockYaff = ((slice->mbAffFrameFlag) && (mb->mbField)) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
  slice->siBlock[mb->mb.y][mb->mb.x] = 0;
  slice->interpretMbMode (mb);

  // init noMbPartLessThan8x8Flag
  mb->noMbPartLessThan8x8Flag = TRUE;
  if (mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB) {
    // transform size flag for INTRA_4x4 and INTRA_8x8 modes
    if (slice->transform8x8Mode) {
      se.type = SE_HEADER;
      dataPartition = &(slice->dps[dpMap[SE_HEADER]]);
      se.reading = readMB_transform_size_flag_CABAC;

      // read CAVLC transform_size_8x8_flag
      if (dataPartition->s->errorFlag) {
        se.len = (int64) 1;
        readsSyntaxElement_FLC (&se, dataPartition->s);
        }
      else
        dataPartition->readSyntaxElement (mb, &se, dataPartition);

      mb->lumaTransformSize8x8flag = (Boolean)se.value1;
      if (mb->lumaTransformSize8x8flag) {
        mb->mbType = I8MB;
        memset (&mb->b8mode, I8MB, 4 * sizeof(char));
        memset (&mb->b8pdir, -1, 4 * sizeof(char));
        }
      }
    else
      mb->lumaTransformSize8x8flag = FALSE;

    initMacroblock (mb);
    readIpredModes (mb);
    slice->readCBPcoeffs (mb);
    }
  else
    readIntraMacroblock (mb);
  }
//}}}
//{{{
static void readPcabacMacroblock (sMacroblock* mb)
{
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  int mbNum = mb->mbIndexX;
  sSyntaxElement se;
  const byte* dpMap = assignSE2dp[slice->datadpMode];

  if (slice->mbAffFrameFlag == 0) {
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    mb->mbField = FALSE;
    updateQp (mb, slice->qp);

    // read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    if (dataPartition->s->errorFlag)
      se.mapping = linfo_ue;

    checkNeighbourCabac(mb);
    se.reading = read_skip_flag_CABAC_p_slice;
    dataPartition->readSyntaxElement(mb, &se, dataPartition);

    mb->mbType = (short) se.value1;
    mb->skipFlag = (char) (!(se.value1));
    if (!dataPartition->s->errorFlag)
      mb->errorFlag = 0;

    // read MB type
    if (mb->mbType != 0 ) {
      se.reading = readMB_typeInfo_CABAC_p_slice;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (short) se.value1;
      if(!dataPartition->s->errorFlag)
        mb->errorFlag = 0;
      }

    motion->mbField[mbNum] = (byte) FALSE;
    mb->blockYaff = mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode(mb);
    }

  else {
    sMacroblock* topMB = NULL;
    int prevMbSkipped = 0;
    int checkBot, readBot, readTop;
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;
    if (mbNum & 0x01) {
      topMB = &decoder->mbData[mbNum-1];
      prevMbSkipped = (topMB->mbType == 0);
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0) ? FALSE : decoder->mbData[mbNum-1].mbField;
    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    if (dataPartition->s->errorFlag)
      se.mapping = linfo_ue;

    // read MB skipFlag
    if (((mbNum & 0x01) == 0||prevMbSkipped))
      fieldFlagInference (mb);

    checkNeighbourCabac(mb);
    se.reading = read_skip_flag_CABAC_p_slice;
    dataPartition->readSyntaxElement (mb, &se, dataPartition);

    mb->mbType = (short)se.value1;
    mb->skipFlag = (char)(!(se.value1));

    if (!dataPartition->s->errorFlag)
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
      mb->mbField = (Boolean)se.value1;
      }

    if (checkBot)
      check_next_mb_and_get_field_mode_CABAC_p_slice (slice, &se, dataPartition);

    // update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4 : 2) : 0;
    checkNeighbourCabac (mb);

    // read MB type
    if (mb->mbType != 0 ) {
      se.reading = readMB_typeInfo_CABAC_p_slice;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (short) se.value1;
      if (!dataPartition->s->errorFlag)
        mb->errorFlag = 0;
      }

    motion->mbField[mbNum] = (byte) mb->mbField;
    mb->blockYaff = (mb->mbField) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode(mb);
    if (mb->mbField) {
      slice->numRefIndexActive[LIST_0] <<=1;
      slice->numRefIndexActive[LIST_1] <<=1;
      }
    }

  mb->noMbPartLessThan8x8Flag = TRUE;
  if (mb->mbType == IPCM)
    readIPCMmacroblock (mb, dpMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblockCabac (mb, dpMap);
  else if (mb->mbType == P8x8) {
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    se.type = SE_MBTYPE;

    if (dataPartition->s->errorFlag)
      se.mapping = linfo_ue;
    else
      se.reading = readB8_typeInfo_CABAC_p_slice;

    readI8x8macroblock (mb, dataPartition, &se);
    }
  else if (mb->mbType == PSKIP)
    readSkipMacroblock (mb);
  else if (mb->isIntraBlock == TRUE)
    readIntraMacroblock (mb);
  else
    readInterMacroblock (mb);
  }
//}}}
//{{{
static void readBcabacMacroblock (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  int mbNum = mb->mbIndexX;
  sSyntaxElement se;

  const byte* dpMap = assignSE2dp[slice->datadpMode];

  if (slice->mbAffFrameFlag == 0) {
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    mb->mbField = FALSE;
    updateQp(mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    if (dataPartition->s->errorFlag)
      se.mapping = linfo_ue;

    checkNeighbourCabac(mb);
    se.reading = read_skip_flag_CABAC_b_slice;
    dataPartition->readSyntaxElement(mb, &se, dataPartition);

    mb->mbType  = (short)se.value1;
    mb->skipFlag = (char)(!(se.value1));
    mb->cbp = se.value2;
    if (!dataPartition->s->errorFlag)
      mb->errorFlag = 0;

    if (se.value1 == 0 && se.value2 == 0)
      slice->codCount=0;

    // read MB type
    if (mb->mbType != 0 ) {
      se.reading = readMB_typeInfo_CABAC_b_slice;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (short)se.value1;
      if (!dataPartition->s->errorFlag)
        mb->errorFlag = 0;
      }

    motion->mbField[mbNum] = (byte) FALSE;
    mb->blockYaff = mb->blockY;
    slice->siBlock[mb->mb.y][mb->mb.x] = 0;
    slice->interpretMbMode (mb);
    }
  else {
    sMacroblock* topMB = NULL;
    int  prevMbSkipped = 0;
    int  checkBot, readBot, readTop;
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    if (mbNum & 0x01) {
      topMB = &decoder->mbData[mbNum-1];
      prevMbSkipped = topMB->skipFlag;
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0)? FALSE : decoder->mbData[mbNum-1].mbField;

    updateQp (mb, slice->qp);

    //  read MB mode
    se.type = SE_MBTYPE;
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    if (dataPartition->s->errorFlag)
      se.mapping = linfo_ue;

    // read MB skipFlag
    if ((mbNum & 0x01) == 0 || prevMbSkipped)
      fieldFlagInference (mb);

    checkNeighbourCabac (mb);
    se.reading = read_skip_flag_CABAC_b_slice;

    dataPartition->readSyntaxElement (mb, &se, dataPartition);
    mb->mbType = (short)se.value1;
    mb->skipFlag = (char)(!(se.value1));
    mb->cbp = se.value2;
    if (!dataPartition->s->errorFlag)
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
      mb->mbField = (Boolean)se.value1;
      }
    if (checkBot)
      check_next_mb_and_get_field_mode_CABAC_b_slice (slice, &se, dataPartition);

    //update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;

    checkNeighbourCabac (mb);

    // read MB type
    if (mb->mbType != 0 ) {
      se.reading = readMB_typeInfo_CABAC_b_slice;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->mbType = (short)se.value1;
      if(!dataPartition->s->errorFlag)
        mb->errorFlag = 0;
      }

    motion->mbField[mbNum] = (byte) mb->mbField;
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
    sDataPartition* dataPartition = &(slice->dps[dpMap[SE_MBTYPE]]);
    se.type = SE_MBTYPE;
    if (dataPartition->s->errorFlag)
      se.mapping = linfo_ue;
    else
      se.reading = readB8_typeInfo_CABAC_b_slice;
    readI8x8macroblock(mb, dataPartition, &se);
    }
  else if (mb->mbType == BSKIP_DIRECT) {
    //init noMbPartLessThan8x8Flag
    mb->noMbPartLessThan8x8Flag = (!(slice->activeSPS->direct_8x8_inference_flag))? FALSE: TRUE;

    // transform size flag for INTRA_4x4 and INTRA_8x8 modes
    mb->lumaTransformSize8x8flag = FALSE;
    if(decoder->activePPS->constrainedIntraPredFlag)
      slice->intraBlock[mbNum] = 0;

    initMacroblockDirect (mb);
    if (slice->codCount >= 0) {
      slice->isResetCoef = TRUE;
      mb->cbp = 0;
      slice->codCount = -1;
      }
    else // read CBP and Coeffs
      slice->readCBPcoeffs (mb);
    }
  else if (mb->isIntraBlock == TRUE)
    readIntraMacroblock (mb);
  else
    readInterMacroblock (mb);
  }
//}}}

//{{{
void setReadMacroblock (sSlice* slice) {

  if (slice->decoder->activePPS->entropyCodingMode == (Boolean)CABAC) {
    switch (slice->sliceType) {
      case P_SLICE:
      case SP_SLICE:
        slice->readMacroblock = readPcabacMacroblock;
        break;
      case B_SLICE:
        slice->readMacroblock = readBcabacMacroblock;
        break;
      case I_SLICE:
      case SI_SLICE:
        slice->readMacroblock = readIcabacMacroblock;
        break;
      default:
        printf ("Unsupported slice type\n");
        break;
      }
    }

  else {
    switch (slice->sliceType) {
      case P_SLICE:
      case SP_SLICE:
        slice->readMacroblock = readPcavlcMacroblock;
        break;
      case B_SLICE:
        slice->readMacroblock = readBcavlcMacroblock;
        break;
      case I_SLICE:
      case SI_SLICE:
        slice->readMacroblock = readIcavlcMacroblock;
        break;
      default:
        printf ("Unsupported slice type\n");
        break;
      }
    }
  }
//}}}
