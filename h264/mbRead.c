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
  const byte* partMap = assignSE2partition[slice->dataPartitionMode];
  sVidParam* vidParam = mb->vidParam;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_block, top_block;

  sSyntaxElement syntaxElement;
  syntaxElement.type = SE_INTRAPREDMODE;
  sDataPartition* dP = &(slice->partitions[partMap[SE_INTRAPREDMODE]]);

  if (!(vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag))
    syntaxElement.reading = readIntraPredMode_CABAC;

  for (int b8 = 0; b8 < 4; ++b8)  {
    // loop 8x8 blocks
    by = (b8 & 0x02);
    bj = mb->blockY + by;

    bx = ((b8 & 0x01) << 1);
    bi = mb->blockX + bx;
    // get from stream
    if (vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag)
      readsSyntaxElement_Intra4x4PredictionMode (&syntaxElement, dP->bitstream);
    else {
      syntaxElement.context = b8 << 2;
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      }

    get4x4Neighbour (mb, (bx << 2) - 1, (by << 2),     vidParam->mbSize[IS_LUMA], &left_block);
    get4x4Neighbour (mb, (bx << 2),     (by << 2) - 1, vidParam->mbSize[IS_LUMA], &top_block );

    // get from array and decode
    if (vidParam->activePPS->constrainedIntraPredFlag) {
      left_block.available = left_block.available ? slice->intraBlock[left_block.mbAddr] : 0;
      top_block.available = top_block.available  ? slice->intraBlock[top_block.mbAddr]  : 0;
      }

    upIntraPredMode = (top_block.available ) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
    leftIntraPredMode = (left_block.available) ? slice->predMode[left_block.posY][left_block.posX] : -1;
    mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED :
                                  upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
    dec = (syntaxElement.value1 == -1) ?
      mostProbableIntraPredMode : syntaxElement.value1 + (syntaxElement.value1 >= mostProbableIntraPredMode);

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
  const byte* partMap = assignSE2partition[slice->dataPartitionMode];
  sVidParam* vidParam = mb->vidParam;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  sSyntaxElement syntaxElement;
  syntaxElement.type = SE_INTRAPREDMODE;
  sDataPartition* dP = &(slice->partitions[partMap[SE_INTRAPREDMODE]]);
  if (!(vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag))
    syntaxElement.reading = readIntraPredMode_CABAC;

  get4x4Neighbour (mb, -1,  0, vidParam->mbSize[IS_LUMA], &left_mb);
  get4x4Neighbour (mb,  0, -1, vidParam->mbSize[IS_LUMA], &top_mb );

  for(b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
    by = (b8 & 0x02);
    bj = mb->blockY + by;

    bx = ((b8 & 0x01) << 1);
    bi = mb->blockX + bx;

    //get from stream
    if (vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag)
      readsSyntaxElement_Intra4x4PredictionMode (&syntaxElement, dP->bitstream);
    else {
      syntaxElement.context = (b8 << 2);
      dP->readsSyntaxElement(mb, &syntaxElement, dP);
    }

    get4x4Neighbour (mb, (bx << 2) - 1, by << 2, vidParam->mbSize[IS_LUMA], &left_block);
    get4x4Neighbour (mb, bx << 2, (by << 2) - 1, vidParam->mbSize[IS_LUMA], &top_block);

    // get from array and decode
    if (vidParam->activePPS->constrainedIntraPredFlag) {
      left_block.available = left_block.available ? slice->intraBlock[left_block.mbAddr] : 0;
      top_block.available = top_block.available  ? slice->intraBlock[top_block.mbAddr] : 0;
    }

    upIntraPredMode = (top_block.available ) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
    leftIntraPredMode = (left_block.available) ? slice->predMode[left_block.posY][left_block.posX] : -1;
    mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? 
      DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
    dec = (syntaxElement.value1 == -1) ?
      mostProbableIntraPredMode : syntaxElement.value1 + (syntaxElement.value1 >= mostProbableIntraPredMode);

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
  sSyntaxElement syntaxElement;
  sSlice* slice = mb->slice;
  const byte *partMap = assignSE2partition[slice->dataPartitionMode];
  sVidParam *vidParam = mb->vidParam;
  sBlockPos *picPos = vidParam->picPos;

  int ts, ls;
  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;
  sPixelPos left_block, top_block;

  syntaxElement.type = SE_INTRAPREDMODE;
  sDataPartition* dP = &(slice->partitions[partMap[SE_INTRAPREDMODE]]);
  if (!(vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag))
    syntaxElement.reading = readIntraPredMode_CABAC;

  for (b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
    for (j = 0; j < 2; j++) { //loop subblocks
      by = (b8 & 0x02) + j;
      bj = mb->blockY + by;

      for(i = 0; i < 2; i++) {
        bx = ((b8 & 1) << 1) + i;
        bi = mb->blockX + bx;
        //get from stream
        if (vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag)
          readsSyntaxElement_Intra4x4PredictionMode (&syntaxElement, dP->bitstream);
        else {
          syntaxElement.context = (b8<<2) + (j<<1) +i;
          dP->readsSyntaxElement (mb, &syntaxElement, dP);
          }

        get4x4Neighbour (mb, (bx<<2) - 1, (by<<2),     vidParam->mbSize[IS_LUMA], &left_block);
        get4x4Neighbour (mb, (bx<<2),     (by<<2) - 1, vidParam->mbSize[IS_LUMA], &top_block );

        // get from array and decode
        if (vidParam->activePPS->constrainedIntraPredFlag) {
          left_block.available = left_block.available ? slice->intraBlock[left_block.mbAddr] : 0;
          top_block.available = top_block.available  ? slice->intraBlock[top_block.mbAddr]  : 0;
          }

        // !! KS: not sure if the following is still correct...
        ts = ls = 0;   // Check to see if the neighboring block is SI
        if (slice->sliceType == SI_SLICE) { // need support for MBINTLC1
          if (left_block.available)
            if (slice->siBlock [picPos[left_block.mbAddr].y][picPos[left_block.mbAddr].x])
              ls = 1;

          if (top_block.available)
            if (slice->siBlock [picPos[top_block.mbAddr].y][picPos[top_block.mbAddr].x])
              ts = 1;
          }

        upIntraPredMode = (top_block.available  &&(ts == 0)) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
        leftIntraPredMode = (left_block.available &&(ls == 0)) ? slice->predMode[left_block.posY][left_block.posX] : -1;
        mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ?
          DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
        slice->predMode[bj][bi] = (byte) ((syntaxElement.value1 == -1) ?
          mostProbableIntraPredMode : syntaxElement.value1 + (syntaxElement.value1 >= mostProbableIntraPredMode));
        }
      }
    }
  }
//}}}
//{{{
static void read_ipred_4x4_modes (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  const byte* partMap = assignSE2partition[slice->dataPartitionMode];
  sVidParam* vidParam = mb->vidParam;
  sBlockPos* picPos = vidParam->picPos;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  sSyntaxElement syntaxElement;
  syntaxElement.type = SE_INTRAPREDMODE;
  sDataPartition* dP = &(slice->partitions[partMap[SE_INTRAPREDMODE]]);
  if (!(vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag))
    syntaxElement.reading = readIntraPredMode_CABAC;

  get4x4Neighbour (mb, -1,  0, vidParam->mbSize[IS_LUMA], &left_mb);
  get4x4Neighbour (mb,  0, -1, vidParam->mbSize[IS_LUMA], &top_mb );

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
        if (vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag)
          readsSyntaxElement_Intra4x4PredictionMode (&syntaxElement, dP->bitstream);
        else {
          syntaxElement.context=(b8<<2) + (j<<1) +i;
          dP->readsSyntaxElement (mb, &syntaxElement, dP);
          }

        get4x4Neighbour(mb, (bx<<2) - 1, (by<<2), vidParam->mbSize[IS_LUMA], &left_block);
        get4x4Neighbour(mb, (bx<<2), (by<<2) - 1, vidParam->mbSize[IS_LUMA], &top_block );

        //get from array and decode
        if (vidParam->activePPS->constrainedIntraPredFlag) {
          left_block.available = left_block.available ? slice->intraBlock[left_block.mbAddr] : 0;
          top_block.available = top_block.available  ? slice->intraBlock[top_block.mbAddr]  : 0;
          }

        int ts = 0;
        int ls = 0;   // Check to see if the neighboring block is SI
        if (slice->sliceType == SI_SLICE) {
          //{{{  need support for MBINTLC1
          if (left_block.available)
            if (slice->siBlock [picPos[left_block.mbAddr].y][picPos[left_block.mbAddr].x])
              ls = 1;

          if (top_block.available)
            if (slice->siBlock [picPos[top_block.mbAddr].y][picPos[top_block.mbAddr].x])
              ts = 1;
          }
          //}}}

        upIntraPredMode = (top_block.available  &&(ts == 0)) ? slice->predMode[top_block.posY ][top_block.posX ] : -1;
        leftIntraPredMode = (left_block.available &&(ls == 0)) ? slice->predMode[left_block.posY][left_block.posX] : -1;
        mostProbableIntraPredMode  = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
        slice->predMode[bj][bi] = (byte)((syntaxElement.value1 == -1) ?
          mostProbableIntraPredMode : syntaxElement.value1 + (syntaxElement.value1 >= mostProbableIntraPredMode));
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
    sSyntaxElement syntaxElement;
    sDataPartition* dP;
    const byte* partMap = assignSE2partition[slice->dataPartitionMode];
    sVidParam* vidParam = mb->vidParam;

    syntaxElement.type = SE_INTRAPREDMODE;
    dP = &(slice->partitions[partMap[SE_INTRAPREDMODE]]);

    if (vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag)
      syntaxElement.mapping = linfo_ue;
    else
      syntaxElement.reading = readCIPredMode_CABAC;

    dP->readsSyntaxElement (mb, &syntaxElement, dP);
    mb->cPredMode = (char)syntaxElement.value1;

    if (mb->cPredMode < DC_PRED_8 || mb->cPredMode > PLANE_8)
      error ("illegal chroma intra pred mode!\n", 600);
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
  int slice_no = slice->curSliceNum;

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

  int slice_no = mb->slice->curSliceNum;
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
  sVidParam* vidParam = mb->vidParam;
  for (int i = 0; i < MB_BLOCK_SIZE; ++i)
    for (int j = 0; j < MB_BLOCK_SIZE; ++j)
      slice->cof[0][i][j] = vidParam->dcPredValueComp[0];

  sPicture* picture = slice->picture;
  if ((picture->chromaFormatIdc != YUV400) && (vidParam->sepColourPlaneFlag == 0))
    for (int k = 0; k < 2; ++k)
      for (int i = 0; i < vidParam->mbCrSizeY; ++i)
        for (int j = 0; j < vidParam->mbCrSizeX; ++j)
          slice->cof[k][i][j] = vidParam->dcPredValueComp[k];
  }
//}}}
//{{{
static void initIPCMdecoding (sSlice* slice) {

  int partitionNum;
  if (slice->dataPartitionMode == PAR_DP_1)
    partitionNum = 1;
  else if (slice->dataPartitionMode == PAR_DP_3)
    partitionNum = 3;
  else {
    printf ("Partition Mode is not supported\n");
    exit (1);
    }

  for (int i = 0; i < partitionNum;++i) {
    sBitstream* stream = slice->partitions[i].bitstream;
    int byteStartPosition = stream->readLen;
    arideco_start_decoding (&slice->partitions[i].deCabac, stream->streamBuffer, byteStartPosition, &stream->readLen);
    }
  }
//}}}
//{{{
static void readIPCMcoeffs (sSlice* slice, struct DataPartition* dP) {

  sVidParam* vidParam = slice->vidParam;

  //For CABAC, we don't need to read bits to let stream byte aligned
  //  because we have variable for integer bytes position
  if (vidParam->activePPS->entropyCodingModeFlag == (Boolean)CABAC) {
    readIPCMcabac (slice, dP);
    initIPCMdecoding (slice);
    }
  else {
    // read bits to let stream byte aligned
    sSyntaxElement syntaxElement;
    if (((dP->bitstream->frameBitOffset) & 0x07) != 0) {
      syntaxElement.len = (8 - ((dP->bitstream->frameBitOffset) & 0x07));
      readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
      }

    //read luma and chroma IPCM coefficients
    syntaxElement.len = vidParam->bitdepthLuma;
    for (int i = 0; i < MB_BLOCK_SIZE;++i)
      for (int j = 0; j < MB_BLOCK_SIZE;++j) {
        readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
        slice->cof[0][i][j] = syntaxElement.value1;
        }

    syntaxElement.len = vidParam->bitdepthChroma;
    sPicture* picture = slice->picture;
    if ((picture->chromaFormatIdc != YUV400) && (vidParam->sepColourPlaneFlag == 0)) {
      for (int i = 0; i < vidParam->mbCrSizeY; ++i)
        for (int j = 0; j < vidParam->mbCrSizeX; ++j) {
          readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
          slice->cof[1][i][j] = syntaxElement.value1;
          }

      for (int i = 0; i < vidParam->mbCrSizeY; ++i)
        for (int j = 0; j < vidParam->mbCrSizeX; ++j) {
          readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
          slice->cof[2][i][j] = syntaxElement.value1;
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

  sVidParam* vidParam = mb->vidParam;

  if (vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC)
    memset (vidParam->nzCoeff[mb->mbAddrX][0][0], 0, 3 * BLOCK_PIXELS * sizeof(byte));
  }
//}}}
//{{{
static void fieldFlagInference (sMacroblock* mb) {

  sVidParam* vidParam = mb->vidParam;
  if (mb->mbAvailA)
    mb->mbField = vidParam->mbData[mb->mbAddrA].mbField;
  else
    // check top macroblock pair
    mb->mbField = mb->mbAvailB ? vidParam->mbData[mb->mbAddrB].mbField : FALSE;
  }
//}}}

//{{{
static void skipMacroblocks (sMacroblock* mb) {

  sMotionVector pred_mv;
  int zeroMotionAbove;
  int zeroMotionLeft;
  sPixelPos neighbourMb[4];
  int i, j;
  int a_mv_y = 0;
  int a_ref_idx = 0;
  int b_mv_y = 0;
  int b_ref_idx = 0;
  int img_block_y   = mb->blockY;
  sVidParam* vidParam = mb->vidParam;
  sSlice* slice = mb->slice;
  int   listOffset = LIST_0 + mb->listOffset;
  sPicture* picture = slice->picture;
  sMotionVector* a_mv = NULL;
  sMotionVector* b_mv = NULL;

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

      if (mb->mbField && !vidParam->mbData[neighbourMb[0].mbAddr].mbField) {
        a_mv_y /= 2;
        a_ref_idx *= 2;
        }
      if (!mb->mbField && vidParam->mbData[neighbourMb[0].mbAddr].mbField) {
        a_mv_y *= 2;
        a_ref_idx >>= 1;
        }
      }

    if (neighbourMb[1].available) {
      b_mv = &picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].mv[LIST_0];
      b_mv_y = b_mv->mvY;
      b_ref_idx = picture->mvInfo[neighbourMb[1].posY][neighbourMb[1].posX].refIndex[LIST_0];

      if (mb->mbField && !vidParam->mbData[neighbourMb[1].mbAddr].mbField) {
        b_mv_y /= 2;
        b_ref_idx *= 2;
        }
      if (!mb->mbField && vidParam->mbData[neighbourMb[1].mbAddr].mbField) {
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
  int slice_no = mb->slice->curSliceNum;

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
  if (mb->vidParam->activePPS->constrainedIntraPredFlag) {
    int mbNum = mb->mbAddrX;
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
static void readIntra4x4macroblocCavlc (sMacroblock* mb, const byte* partMap)
{
  sSlice* slice = mb->slice;

  //transform size flag for INTRA_4x4 and INTRA_8x8 modes
  if (slice->transform8x8Mode) {
    sSyntaxElement syntaxElement;
    sDataPartition* dP = &(slice->partitions[partMap[SE_HEADER]]);
    syntaxElement.type = SE_HEADER;

    // read CAVLC transform_size_8x8_flag
    syntaxElement.len = (int64)1;
    readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);

    mb->lumaTransformSize8x8flag = (Boolean)syntaxElement.value1;
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
static void readIntra4x4macroblockCabac (sMacroblock* mb, const byte* partMap) {


  // transform size flag for INTRA_4x4 and INTRA_8x8 modes
  sSlice* slice = mb->slice;
  if (slice->transform8x8Mode) {
   sSyntaxElement syntaxElement;
    sDataPartition* dP = &(slice->partitions[partMap[SE_HEADER]]);
    syntaxElement.type = SE_HEADER;
    syntaxElement.reading = readMB_transform_size_flag_CABAC;

    // read CAVLC transform_size_8x8_flag
    if (dP->bitstream->eiFlag) {
      syntaxElement.len = (int64) 1;
      readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
      }
    else
      dP->readsSyntaxElement (mb, &syntaxElement, dP);

    mb->lumaTransformSize8x8flag = (Boolean)syntaxElement.value1;
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
  if (mb->vidParam->activePPS->constrainedIntraPredFlag) {
    int mbNum = mb->mbAddrX;
    slice->intraBlock[mbNum] = 0;
    }

  initMacroblock (mb);
  slice->readMotionInfoFromNAL (mb);
  slice->readCBPcoeffs (mb);
  }
//}}}
//{{{
static void readIPCMmacroblock (sMacroblock* mb, const byte* partMap) {

  sSlice* slice = mb->slice;
  mb->noMbPartLessThan8x8Flag = TRUE;
  mb->lumaTransformSize8x8flag = FALSE;

  initMacroblock (mb);

  // here dP is assigned with the same dP as SE_MBTYPE, because IPCM syntax is in the
  // same category as MBTYPE
  if (slice->dataPartitionMode && slice->noDataPartitionB )
    concealIPCMcoeffs (mb);
  else {
    sDataPartition* dP = &(slice->partitions[partMap[SE_LUM_DC_INTRA]]);
    readIPCMcoeffs (slice, dP);
    }
  }
//}}}
//{{{
static void readI8x8macroblock (sMacroblock* mb, sDataPartition* dP, sSyntaxElement* syntaxElement) {

  int i;
  sSlice* slice = mb->slice;

  // READ 8x8 SUB-PARTITION MODES (modes of 8x8 blocks) and Intra VBST block modes
  mb->noMbPartLessThan8x8Flag = TRUE;
  mb->lumaTransformSize8x8flag = FALSE;

  for (i = 0; i < 4; ++i) {
    dP->readsSyntaxElement (mb, syntaxElement, dP);
    SetB8Mode (mb, syntaxElement->value1, i);

    // set noMbPartLessThan8x8Flag for P8x8 mode
    mb->noMbPartLessThan8x8Flag &= (mb->b8mode[i] == 0 && slice->activeSPS->direct_8x8_inference_flag) ||
      (mb->b8mode[i] == 4);
    }

  initMacroblock (mb);
  slice->readMotionInfoFromNAL (mb);

  if (mb->vidParam->activePPS->constrainedIntraPredFlag) {
    int mbNum = mb->mbAddrX;
    slice->intraBlock[mbNum] = 0;
    }

  slice->readCBPcoeffs (mb);
  }
//}}}

//{{{
static void read_one_macroblock_i_slice_cavlc (sMacroblock* mb) {

  sSlice* slice = mb->slice;

  sSyntaxElement syntaxElement;
  int mbNum = mb->mbAddrX;

  const byte* partMap = assignSE2partition[slice->dataPartitionMode];
  sPicture* picture = slice->picture;
  sPicMotionParamsOld* motion = &picture->motion;

  mb->mbField = ((mbNum & 0x01) == 0)? FALSE : slice->mbData[mbNum-1].mbField;

  updateQp(mb, slice->qp);
  syntaxElement.type = SE_MBTYPE;

  // read MB mode
  sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
  syntaxElement.mapping = linfo_ue;

  // read MB aff
  if (slice->mbAffFrameFlag && (mbNum & 0x01) == 0) {
    syntaxElement.len = (int64) 1;
    readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
    mb->mbField = (Boolean)syntaxElement.value1;
    }

  // read MB type
  dP->readsSyntaxElement(mb, &syntaxElement, dP);

  mb->mbType = (short) syntaxElement.value1;
  if (!dP->bitstream->eiFlag)
    mb->eiFlag = 0;

  motion->mbField[mbNum] = (byte) mb->mbField;
  mb->blockYaff = ((slice->mbAffFrameFlag) && (mb->mbField)) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
  slice->siBlock[mb->mb.y][mb->mb.x] = 0;
  slice->interpretMbMode (mb);

  mb->noMbPartLessThan8x8Flag = TRUE;
  if(mb->mbType == IPCM)
    readIPCMmacroblock (mb, partMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblocCavlc (mb, partMap);
  else
    readIntraMacroblock (mb);
  }
//}}}
//{{{
static void read_one_macroblock_p_slice_cavlc (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sSyntaxElement syntaxElement;
  int mbNum = mb->mbAddrX;

  const byte* partMap = assignSE2partition[slice->dataPartitionMode];

  if (slice->mbAffFrameFlag == 0) {
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    mb->mbField = FALSE;
    updateQp (mb, slice->qp);

    //  read MB mode
    syntaxElement.type = SE_MBTYPE;
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    syntaxElement.mapping = linfo_ue;

    // VLC Non-Intra
    if (slice->codCount == -1) {
      dP->readsSyntaxElement(mb, &syntaxElement, dP);
      slice->codCount = syntaxElement.value1;
      }

    if (slice->codCount==0) {
      // read MB type
      dP->readsSyntaxElement(mb, &syntaxElement, dP);
      ++(syntaxElement.value1);
      mb->mbType = (short) syntaxElement.value1;
      if(!dP->bitstream->eiFlag)
        mb->eiFlag = 0;
      slice->codCount--;
      mb->skipFlag = 0;
      }
    else {
      slice->codCount--;
      mb->mbType = 0;
      mb->eiFlag = 0;
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
    sVidParam* vidParam = mb->vidParam;
    sMacroblock* topMB = NULL;
    int  prevMbSkipped = 0;
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    if (mbNum & 0x01) {
      topMB= &vidParam->mbData[mbNum-1];
      prevMbSkipped = (topMB->mbType == 0);
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0)? FALSE : vidParam->mbData[mbNum-1].mbField;
    updateQp (mb, slice->qp);

    //  read MB mode
    syntaxElement.type = SE_MBTYPE;
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    syntaxElement.mapping = linfo_ue;

    // VLC Non-Intra
    if (slice->codCount == -1) {
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      slice->codCount = syntaxElement.value1;
      }

    if (slice->codCount == 0) {
      // read MB aff
      if ((((mbNum & 0x01)==0) || ((mbNum & 0x01) && prevMbSkipped))) {
        syntaxElement.len = (int64) 1;
        readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
        mb->mbField = (Boolean)syntaxElement.value1;
        }

      // read MB type
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      ++(syntaxElement.value1);
      mb->mbType = (short)syntaxElement.value1;
      if(!dP->bitstream->eiFlag)
        mb->eiFlag = 0;
      slice->codCount--;
      mb->skipFlag = 0;
      }
    else {
      slice->codCount--;
      mb->mbType = 0;
      mb->eiFlag = 0;
      mb->skipFlag = 1;

      // read field flag of bottom block
      if (slice->codCount == 0 && ((mbNum & 0x01) == 0)) {
        syntaxElement.len = (int64) 1;
        readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
        dP->bitstream->frameBitOffset--;
        mb->mbField = (Boolean)syntaxElement.value1;
        }
      else if (slice->codCount > 0 && ((mbNum & 0x01) == 0)) {
        // check left macroblock pair first
        if (mb_is_available(mbNum - 2, mb) && ((mbNum % (vidParam->PicWidthInMbs * 2))!=0))
          mb->mbField = vidParam->mbData[mbNum-2].mbField;
        else {
          // check top macroblock pair
          if (mb_is_available (mbNum - 2*vidParam->PicWidthInMbs, mb))
            mb->mbField = vidParam->mbData[mbNum - 2*vidParam->PicWidthInMbs].mbField;
          else
            mb->mbField = FALSE;
          }
        }
      }
    //update the list offset;
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
    readIPCMmacroblock (mb, partMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblocCavlc (mb, partMap);
  else if (mb->mbType == P8x8) {
    syntaxElement.type = SE_MBTYPE;
    syntaxElement.mapping = linfo_ue;
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    readI8x8macroblock (mb, dP, &syntaxElement);
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
static void read_one_macroblock_b_slice_cavlc (sMacroblock* mb) {

  sVidParam* vidParam = mb->vidParam;
  sSlice* slice = mb->slice;
  int mbNum = mb->mbAddrX;
  sSyntaxElement syntaxElement;
  const byte *partMap = assignSE2partition[slice->dataPartitionMode];

  if (slice->mbAffFrameFlag == 0) {
    sPicture* picture = slice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    mb->mbField = FALSE;
    updateQp(mb, slice->qp);

    //  read MB mode
    syntaxElement.type = SE_MBTYPE;
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    syntaxElement.mapping = linfo_ue;

    if(slice->codCount == -1) {
      dP->readsSyntaxElement(mb, &syntaxElement, dP);
      slice->codCount = syntaxElement.value1;
      }

    if (slice->codCount==0) {
      // read MB type
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      mb->mbType = (short)syntaxElement.value1;
      if (!dP->bitstream->eiFlag)
        mb->eiFlag = 0;
      slice->codCount--;
      mb->skipFlag = 0;
      }
    else {
      slice->codCount--;
      mb->mbType = 0;
      mb->eiFlag = 0;
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
      topMB = &vidParam->mbData[mbNum-1];
      prevMbSkipped = topMB->skipFlag;
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0)? FALSE : vidParam->mbData[mbNum-1].mbField;

    updateQp (mb, slice->qp);

    //  read MB mode
    syntaxElement.type = SE_MBTYPE;
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    syntaxElement.mapping = linfo_ue;
    if(slice->codCount == -1) {
      dP->readsSyntaxElement(mb, &syntaxElement, dP);
      slice->codCount = syntaxElement.value1;
      }

    if (slice->codCount==0) {
      // read MB aff
      if (((mbNum & 0x01)==0) || ((mbNum & 0x01) && prevMbSkipped)) {
        syntaxElement.len = (int64) 1;
        readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
        mb->mbField = (Boolean)syntaxElement.value1;
        }

      // read MB type
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      mb->mbType = (short)syntaxElement.value1;
      if(!dP->bitstream->eiFlag)
        mb->eiFlag = 0;
      slice->codCount--;
      mb->skipFlag = 0;
      }
    else {
      slice->codCount--;
      mb->mbType = 0;
      mb->eiFlag = 0;
      mb->skipFlag = 1;

      // read field flag of bottom block
      if ((slice->codCount == 0) && ((mbNum & 0x01) == 0)) {
        syntaxElement.len = (int64) 1;
        readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
        dP->bitstream->frameBitOffset--;
        mb->mbField = (Boolean)syntaxElement.value1;
        }
      else if ((slice->codCount > 0) && ((mbNum & 0x01) == 0)) {
        // check left macroblock pair first
        if (mb_is_available (mbNum - 2, mb) && ((mbNum % (vidParam->PicWidthInMbs * 2))!=0))
          mb->mbField = vidParam->mbData[mbNum-2].mbField;
        else {
          // check top macroblock pair
          if (mb_is_available (mbNum - 2*vidParam->PicWidthInMbs, mb))
            mb->mbField = vidParam->mbData[mbNum-2*vidParam->PicWidthInMbs].mbField;
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
    readIPCMmacroblock (mb, partMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblocCavlc (mb, partMap);
  else if (mb->mbType == P8x8) {
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    syntaxElement.type = SE_MBTYPE;
    syntaxElement.mapping = linfo_ue;
    readI8x8macroblock (mb, dP, &syntaxElement);
    }
  else if (mb->mbType == BSKIP_DIRECT) {
    // init noMbPartLessThan8x8Flag
    mb->noMbPartLessThan8x8Flag = (!(slice->activeSPS->direct_8x8_inference_flag))? FALSE: TRUE;
    mb->lumaTransformSize8x8flag = FALSE;
    if(vidParam->activePPS->constrainedIntraPredFlag)
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
static void read_one_macroblock_i_slice_cabac (sMacroblock* mb) {

  sSlice* slice = mb->slice;

  sSyntaxElement syntaxElement;
  int mbNum = mb->mbAddrX;

  const byte* partMap = assignSE2partition[slice->dataPartitionMode];
  sPicture* picture = slice->picture;
  sPicMotionParamsOld* motion = &picture->motion;

  mb->mbField = ((mbNum & 0x01) == 0) ? FALSE : slice->mbData[mbNum-1].mbField;

  updateQp (mb, slice->qp);

  //  read MB mode
  syntaxElement.type = SE_MBTYPE;
  sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
  if (dP->bitstream->eiFlag)
    syntaxElement.mapping = linfo_ue;

  // read MB aff
  if (slice->mbAffFrameFlag && (mbNum & 0x01)==0) {
    if (dP->bitstream->eiFlag) {
      syntaxElement.len = (int64)1;
      readsSyntaxElement_FLC (&syntaxElement, dP->bitstream);
      }
    else {
      syntaxElement.reading = readFieldModeInfo_CABAC;
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      }
    mb->mbField = (Boolean)syntaxElement.value1;
    }

  checkNeighbourCabac(mb);

  //  read MB type
  syntaxElement.reading = readMB_typeInfo_CABAC_i_slice;
  dP->readsSyntaxElement (mb, &syntaxElement, dP);

  mb->mbType = (short) syntaxElement.value1;
  if (!dP->bitstream->eiFlag)
    mb->eiFlag = 0;

  motion->mbField[mbNum] = (byte) mb->mbField;
  mb->blockYaff = ((slice->mbAffFrameFlag) && (mb->mbField)) ? (mbNum & 0x01) ? (mb->blockY - 4)>>1 : mb->blockY >> 1 : mb->blockY;
  slice->siBlock[mb->mb.y][mb->mb.x] = 0;
  slice->interpretMbMode(mb);

  //init noMbPartLessThan8x8Flag
  mb->noMbPartLessThan8x8Flag = TRUE;
  if (mb->mbType == IPCM)
    readIPCMmacroblock (mb, partMap);
  else if (mb->mbType == I4MB) {
    //transform size flag for INTRA_4x4 and INTRA_8x8 modes
    if (slice->transform8x8Mode) {
      syntaxElement.type = SE_HEADER;
      dP = &(slice->partitions[partMap[SE_HEADER]]);
      syntaxElement.reading = readMB_transform_size_flag_CABAC;

      // read CAVLC transform_size_8x8_flag
      if (dP->bitstream->eiFlag) {
        syntaxElement.len = (int64) 1;
        readsSyntaxElement_FLC(&syntaxElement, dP->bitstream);
        }
      else
        dP->readsSyntaxElement(mb, &syntaxElement, dP);

      mb->lumaTransformSize8x8flag = (Boolean)syntaxElement.value1;
      if (mb->lumaTransformSize8x8flag) {
        mb->mbType = I8MB;
        memset(&mb->b8mode, I8MB, 4 * sizeof(char));
        memset(&mb->b8pdir, -1, 4 * sizeof(char));
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
static void read_one_macroblock_p_slice_cabac (sMacroblock* mb)
{
  sSlice* slice = mb->slice;
  sVidParam* vidParam = mb->vidParam;
  int mbNum = mb->mbAddrX;
  sSyntaxElement syntaxElement;
  const byte* partMap = assignSE2partition[slice->dataPartitionMode];

  if (slice->mbAffFrameFlag == 0) {
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    mb->mbField = FALSE;
    updateQp (mb, slice->qp);

    // read MB mode
    syntaxElement.type = SE_MBTYPE;
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    if (dP->bitstream->eiFlag)
      syntaxElement.mapping = linfo_ue;

    checkNeighbourCabac(mb);
    syntaxElement.reading = read_skip_flag_CABAC_p_slice;
    dP->readsSyntaxElement(mb, &syntaxElement, dP);

    mb->mbType   = (short) syntaxElement.value1;
    mb->skipFlag = (char) (!(syntaxElement.value1));
    if (!dP->bitstream->eiFlag)
      mb->eiFlag = 0;

    // read MB type
    if (mb->mbType != 0 ) {
      syntaxElement.reading = readMB_typeInfo_CABAC_p_slice;
      dP->readsSyntaxElement(mb, &syntaxElement, dP);
      mb->mbType = (short) syntaxElement.value1;
      if(!dP->bitstream->eiFlag)
        mb->eiFlag = 0;
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
      topMB = &vidParam->mbData[mbNum-1];
      prevMbSkipped = (topMB->mbType == 0);
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0) ? FALSE : vidParam->mbData[mbNum-1].mbField;
    updateQp (mb, slice->qp);

    //  read MB mode
    syntaxElement.type = SE_MBTYPE;
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    if (dP->bitstream->eiFlag)
      syntaxElement.mapping = linfo_ue;

    // read MB skipFlag
    if (((mbNum & 0x01) == 0||prevMbSkipped))
      fieldFlagInference (mb);

    checkNeighbourCabac(mb);
    syntaxElement.reading = read_skip_flag_CABAC_p_slice;
    dP->readsSyntaxElement (mb, &syntaxElement, dP);

    mb->mbType = (short)syntaxElement.value1;
    mb->skipFlag = (char)(!(syntaxElement.value1));

    if (!dP->bitstream->eiFlag)
      mb->eiFlag = 0;

    // read MB AFF
    checkBot = readBot=readTop=0;
    if ((mbNum & 0x01)==0) {
      checkBot = mb->skipFlag;
      readTop = !checkBot;
      }
    else
      readBot = (topMB->skipFlag && (!mb->skipFlag));

    if (readBot || readTop) {
      syntaxElement.reading = readFieldModeInfo_CABAC;
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      mb->mbField = (Boolean)syntaxElement.value1;
      }

    if (checkBot)
      check_next_mb_and_get_field_mode_CABAC_p_slice (slice, &syntaxElement, dP);

    // update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;
    checkNeighbourCabac (mb);

    // read MB type
    if (mb->mbType != 0 ) {
      syntaxElement.reading = readMB_typeInfo_CABAC_p_slice;
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      mb->mbType = (short) syntaxElement.value1;
      if (!dP->bitstream->eiFlag)
        mb->eiFlag = 0;
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
    readIPCMmacroblock (mb, partMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblockCabac (mb, partMap);
  else if (mb->mbType == P8x8) {
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    syntaxElement.type = SE_MBTYPE;

    if (dP->bitstream->eiFlag)
      syntaxElement.mapping = linfo_ue;
    else
      syntaxElement.reading = readB8_typeInfo_CABAC_p_slice;

    readI8x8macroblock (mb, dP, &syntaxElement);
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
static void read_one_macroblock_b_slice_cabac (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sVidParam* vidParam = mb->vidParam;
  int mbNum = mb->mbAddrX;
  sSyntaxElement syntaxElement;

  const byte* partMap = assignSE2partition[slice->dataPartitionMode];

  if (slice->mbAffFrameFlag == 0) {
    sPicture* picture = slice->picture;
    sPicMotionParamsOld* motion = &picture->motion;

    mb->mbField = FALSE;
    updateQp(mb, slice->qp);

    //  read MB mode
    syntaxElement.type = SE_MBTYPE;
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    if (dP->bitstream->eiFlag)
      syntaxElement.mapping = linfo_ue;

    checkNeighbourCabac(mb);
    syntaxElement.reading = read_skip_flag_CABAC_b_slice;
    dP->readsSyntaxElement(mb, &syntaxElement, dP);

    mb->mbType   = (short)syntaxElement.value1;
    mb->skipFlag = (char)(!(syntaxElement.value1));
    mb->cbp = syntaxElement.value2;

    if (!dP->bitstream->eiFlag)
      mb->eiFlag = 0;

    if (syntaxElement.value1 == 0 && syntaxElement.value2 == 0)
      slice->codCount=0;

    // read MB type
    if (mb->mbType != 0 ) {
      syntaxElement.reading = readMB_typeInfo_CABAC_b_slice;
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      mb->mbType = (short)syntaxElement.value1;
      if (!dP->bitstream->eiFlag)
        mb->eiFlag = 0;
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
      topMB = &vidParam->mbData[mbNum-1];
      prevMbSkipped = topMB->skipFlag;
      }
    else
      prevMbSkipped = 0;

    mb->mbField = ((mbNum & 0x01) == 0)? FALSE : vidParam->mbData[mbNum-1].mbField;

    updateQp (mb, slice->qp);

    //  read MB mode
    syntaxElement.type = SE_MBTYPE;
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    if (dP->bitstream->eiFlag)
      syntaxElement.mapping = linfo_ue;

    // read MB skipFlag
    if ((mbNum & 0x01) == 0 || prevMbSkipped)
      fieldFlagInference (mb);

    checkNeighbourCabac (mb);
    syntaxElement.reading = read_skip_flag_CABAC_b_slice;

    dP->readsSyntaxElement (mb, &syntaxElement, dP);
    mb->mbType = (short)syntaxElement.value1;
    mb->skipFlag = (char)(!(syntaxElement.value1));
    mb->cbp = syntaxElement.value2;
    if (!dP->bitstream->eiFlag)
      mb->eiFlag = 0;
    if (syntaxElement.value1 == 0 && syntaxElement.value2 == 0)
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
      syntaxElement.reading = readFieldModeInfo_CABAC;
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      mb->mbField = (Boolean)syntaxElement.value1;
      }
    if (checkBot)
      check_next_mb_and_get_field_mode_CABAC_b_slice (slice, &syntaxElement, dP);

    //update the list offset;
    mb->listOffset = (mb->mbField)? ((mbNum & 0x01)? 4: 2): 0;

    checkNeighbourCabac (mb);

    // read MB type
    if (mb->mbType != 0 ) {
      syntaxElement.reading = readMB_typeInfo_CABAC_b_slice;
      dP->readsSyntaxElement (mb, &syntaxElement, dP);
      mb->mbType = (short)syntaxElement.value1;
      if(!dP->bitstream->eiFlag)
        mb->eiFlag = 0;
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
    readIPCMmacroblock (mb, partMap);
  else if (mb->mbType == I4MB)
    readIntra4x4macroblockCabac (mb, partMap);
  else if (mb->mbType == P8x8) {
    sDataPartition* dP = &(slice->partitions[partMap[SE_MBTYPE]]);
    syntaxElement.type = SE_MBTYPE;
    if (dP->bitstream->eiFlag)
      syntaxElement.mapping = linfo_ue;
    else
      syntaxElement.reading = readB8_typeInfo_CABAC_b_slice;
    readI8x8macroblock(mb, dP, &syntaxElement);
    }
  else if (mb->mbType == BSKIP_DIRECT) {
    //init noMbPartLessThan8x8Flag
    mb->noMbPartLessThan8x8Flag = (!(slice->activeSPS->direct_8x8_inference_flag))? FALSE: TRUE;

    // transform size flag for INTRA_4x4 and INTRA_8x8 modes
    mb->lumaTransformSize8x8flag = FALSE;
    if(vidParam->activePPS->constrainedIntraPredFlag)
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

  if (slice->vidParam->activePPS->entropyCodingModeFlag == (Boolean)CABAC) {
    switch (slice->sliceType) {
      case P_SLICE:
      case SP_SLICE:
        slice->readOneMacroblock = read_one_macroblock_p_slice_cabac;
        break;
      case B_SLICE:
        slice->readOneMacroblock = read_one_macroblock_b_slice_cabac;
        break;
      case I_SLICE:
      case SI_SLICE:
        slice->readOneMacroblock = read_one_macroblock_i_slice_cabac;
        break;
      default:
        printf("Unsupported slice type\n");
        break;
      }
    }

  else {
    switch (slice->sliceType) {
      case P_SLICE:
      case SP_SLICE:
        slice->readOneMacroblock = read_one_macroblock_p_slice_cavlc;
        break;
      case B_SLICE:
        slice->readOneMacroblock = read_one_macroblock_b_slice_cavlc;
        break;
      case I_SLICE:
      case SI_SLICE:
        slice->readOneMacroblock = read_one_macroblock_i_slice_cavlc;
        break;
      default:
        printf("Unsupported slice type\n");
        break;
      }
    }
  }
//}}}
