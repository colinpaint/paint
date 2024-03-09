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
static void read_ipred_8x8_modes_mbaff (sMacroblock* curMb) {

  int bi, bj, bx, by, dec;
  sSyntaxElement currSE;
  sDataPartition* dP;
  sSlice* curSlice = curMb->slice;
  const byte* partMap = assignSE2partition[curSlice->dataPartitionMode];
  sVidParam* vidParam = curMb->vidParam;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_block, top_block;
  currSE.type = SE_INTRAPREDMODE;
  dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);

  if (!(vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag))
    currSE.reading = readIntraPredMode_CABAC;

  for (int b8 = 0; b8 < 4; ++b8)  {
    // loop 8x8 blocks
    by = (b8 & 0x02);
    bj = curMb->blockY + by;

    bx = ((b8 & 0x01) << 1);
    bi = curMb->blockX + bx;
    // get from stream
    if (vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag)
      readsSyntaxElement_Intra4x4PredictionMode (&currSE, dP->bitstream);
    else {
      currSE.context = (b8 << 2);
      dP->readsSyntaxElement(curMb, &currSE, dP);
    }

    get4x4Neighbour (curMb, (bx << 2) - 1, (by << 2),     vidParam->mbSize[IS_LUMA], &left_block);
    get4x4Neighbour (curMb, (bx << 2),     (by << 2) - 1, vidParam->mbSize[IS_LUMA], &top_block );

    // get from array and decode
    if (vidParam->activePPS->constrainedIntraPredFlag) {
      left_block.available = left_block.available ? curSlice->intraBlock[left_block.mbAddr] : 0;
      top_block.available = top_block.available  ? curSlice->intraBlock[top_block.mbAddr]  : 0;
      }

    upIntraPredMode = (top_block.available ) ? curSlice->predMode[top_block.posY ][top_block.posX ] : -1;
    leftIntraPredMode = (left_block.available) ? curSlice->predMode[left_block.posY][left_block.posX] : -1;
    mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED :
                                  upIntraPredMode < leftIntraPredMode ? upIntraPredMode :
                                                                        leftIntraPredMode;
    dec = (currSE.value1 == -1) ? mostProbableIntraPredMode :
                                  currSE.value1 + (currSE.value1 >= mostProbableIntraPredMode);

    //set, loop 4x4s in the subblock for 8x8 prediction setting
    curSlice->predMode[bj    ][bi    ] = (byte) dec;
    curSlice->predMode[bj    ][bi + 1] = (byte) dec;
    curSlice->predMode[bj + 1][bi    ] = (byte) dec;
    curSlice->predMode[bj + 1][bi + 1] = (byte) dec;
    }
  }
//}}}
//{{{
static void read_ipred_8x8_modes (sMacroblock* curMb)
{
  int b8, bi, bj, bx, by, dec;
  sSyntaxElement currSE;
  sDataPartition *dP;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];
  sVidParam *vidParam = curMb->vidParam;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  currSE.type = SE_INTRAPREDMODE;

  dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);

  if (!(vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag))
    currSE.reading = readIntraPredMode_CABAC;

  get4x4Neighbour(curMb, -1,  0, vidParam->mbSize[IS_LUMA], &left_mb);
  get4x4Neighbour(curMb,  0, -1, vidParam->mbSize[IS_LUMA], &top_mb );

  for(b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
    by = (b8 & 0x02);
    bj = curMb->blockY + by;

    bx = ((b8 & 0x01) << 1);
    bi = curMb->blockX + bx;

    //get from stream
    if (vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag)
      readsSyntaxElement_Intra4x4PredictionMode(&currSE, dP->bitstream);
    else {
      currSE.context = (b8 << 2);
      dP->readsSyntaxElement(curMb, &currSE, dP);
    }

    get4x4Neighbour(curMb, (bx<<2) - 1, (by<<2),     vidParam->mbSize[IS_LUMA], &left_block);
    get4x4Neighbour(curMb, (bx<<2),     (by<<2) - 1, vidParam->mbSize[IS_LUMA], &top_block );

    //get from array and decode

    if (vidParam->activePPS->constrainedIntraPredFlag) {
      left_block.available = left_block.available ? curSlice->intraBlock[left_block.mbAddr] : 0;
      top_block.available  = top_block.available  ? curSlice->intraBlock[top_block.mbAddr]  : 0;
    }

    upIntraPredMode            = (top_block.available ) ? curSlice->predMode[top_block.posY ][top_block.posX ] : -1;
    leftIntraPredMode          = (left_block.available) ? curSlice->predMode[left_block.posY][left_block.posX] : -1;
    mostProbableIntraPredMode  = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
    dec = (currSE.value1 == -1) ? mostProbableIntraPredMode : currSE.value1 + (currSE.value1 >= mostProbableIntraPredMode);

    //set
    //loop 4x4s in the subblock for 8x8 prediction setting
    curSlice->predMode[bj    ][bi    ] = (byte) dec;
    curSlice->predMode[bj    ][bi + 1] = (byte) dec;
    curSlice->predMode[bj + 1][bi    ] = (byte) dec;
    curSlice->predMode[bj + 1][bi + 1] = (byte) dec;
  }
}
//}}}
//{{{
static void read_ipred_4x4_modes_mbaff (sMacroblock* curMb) {

  int b8,i,j,bi,bj,bx,by;
  sSyntaxElement currSE;
  sDataPartition *dP;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];
  sVidParam *vidParam = curMb->vidParam;
  sBlockPos *picPos = vidParam->picPos;

  int ts, ls;
  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;
  sPixelPos left_block, top_block;

  currSE.type = SE_INTRAPREDMODE;
  dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);
  if (!(vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag))
    currSE.reading = readIntraPredMode_CABAC;

  for (b8 = 0; b8 < 4; ++b8) { //loop 8x8 blocks
    for (j = 0; j < 2; j++) { //loop subblocks
      by = (b8 & 0x02) + j;
      bj = curMb->blockY + by;

      for(i = 0; i < 2; i++) {
        bx = ((b8 & 1) << 1) + i;
        bi = curMb->blockX + bx;
        //get from stream
        if (vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag)
          readsSyntaxElement_Intra4x4PredictionMode(&currSE, dP->bitstream);
        else {
          currSE.context=(b8<<2) + (j<<1) +i;
          dP->readsSyntaxElement(curMb, &currSE, dP);
          }

        get4x4Neighbour(curMb, (bx<<2) - 1, (by<<2),     vidParam->mbSize[IS_LUMA], &left_block);
        get4x4Neighbour(curMb, (bx<<2),     (by<<2) - 1, vidParam->mbSize[IS_LUMA], &top_block );

        // get from array and decode
        if (vidParam->activePPS->constrainedIntraPredFlag) {
          left_block.available = left_block.available ? curSlice->intraBlock[left_block.mbAddr] : 0;
          top_block.available  = top_block.available  ? curSlice->intraBlock[top_block.mbAddr]  : 0;
          }

        // !! KS: not sure if the following is still correct...
        ts = ls = 0;   // Check to see if the neighboring block is SI
        if (curSlice->sliceType == SI_SLICE) { // need support for MBINTLC1
          if (left_block.available)
            if (curSlice->siBlock [picPos[left_block.mbAddr].y][picPos[left_block.mbAddr].x])
              ls=1;

          if (top_block.available)
            if (curSlice->siBlock [picPos[top_block.mbAddr].y][picPos[top_block.mbAddr].x])
              ts=1;
          }

        upIntraPredMode = (top_block.available  &&(ts == 0)) ? curSlice->predMode[top_block.posY ][top_block.posX ] : -1;
        leftIntraPredMode = (left_block.available &&(ls == 0)) ? curSlice->predMode[left_block.posY][left_block.posX] : -1;
        mostProbableIntraPredMode = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
        curSlice->predMode[bj][bi] = (byte) ((currSE.value1 == -1) ? mostProbableIntraPredMode : currSE.value1 + (currSE.value1 >= mostProbableIntraPredMode));
        }
      }
    }
  }
//}}}
//{{{
static void read_ipred_4x4_modes (sMacroblock* curMb) {

  sSyntaxElement currSE;
  sDataPartition* dP;
  sSlice* curSlice = curMb->slice;
  const byte* partMap = assignSE2partition[curSlice->dataPartitionMode];
  sVidParam* vidParam = curMb->vidParam;
  sBlockPos* picPos = vidParam->picPos;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  currSE.type = SE_INTRAPREDMODE;
  dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);
  if (!(vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag))
    currSE.reading = readIntraPredMode_CABAC;

  get4x4Neighbour (curMb, -1,  0, vidParam->mbSize[IS_LUMA], &left_mb);
  get4x4Neighbour (curMb,  0, -1, vidParam->mbSize[IS_LUMA], &top_mb );

  for (int b8 = 0; b8 < 4; ++b8) {
    // loop 8x8 blocks
    for (int j = 0; j < 2; j++) {
      // loop subblocks
      int by = (b8 & 0x02) + j;
      int bj = curMb->blockY + by;
      for (int i = 0; i < 2; i++) {
        int bx = ((b8 & 1) << 1) + i;
        int bi = curMb->blockX + bx;

        // get from stream
        if (vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC || dP->bitstream->eiFlag)
          readsSyntaxElement_Intra4x4PredictionMode (&currSE, dP->bitstream);
        else {
          currSE.context=(b8<<2) + (j<<1) +i;
          dP->readsSyntaxElement (curMb, &currSE, dP);
          }

        get4x4Neighbour(curMb, (bx<<2) - 1, (by<<2),     vidParam->mbSize[IS_LUMA], &left_block);
        get4x4Neighbour(curMb, (bx<<2),     (by<<2) - 1, vidParam->mbSize[IS_LUMA], &top_block );

        //get from array and decode
        if (vidParam->activePPS->constrainedIntraPredFlag) {
          left_block.available = left_block.available ? curSlice->intraBlock[left_block.mbAddr] : 0;
          top_block.available = top_block.available  ? curSlice->intraBlock[top_block.mbAddr]  : 0;
          }

        int ts = 0;
        int ls = 0;   // Check to see if the neighboring block is SI
        if (curSlice->sliceType == SI_SLICE) {
          //{{{  need support for MBINTLC1
          if (left_block.available)
            if (curSlice->siBlock [picPos[left_block.mbAddr].y][picPos[left_block.mbAddr].x])
              ls = 1;

          if (top_block.available)
            if (curSlice->siBlock [picPos[top_block.mbAddr].y][picPos[top_block.mbAddr].x])
              ts = 1;
          }
          //}}}

        upIntraPredMode = (top_block.available  &&(ts == 0)) ? curSlice->predMode[top_block.posY ][top_block.posX ] : -1;
        leftIntraPredMode = (left_block.available &&(ls == 0)) ? curSlice->predMode[left_block.posY][left_block.posX] : -1;
        mostProbableIntraPredMode  = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
        curSlice->predMode[bj][bi] = (byte)((currSE.value1 == -1) ? mostProbableIntraPredMode : currSE.value1 + (currSE.value1 >= mostProbableIntraPredMode));
        }
      }
    }
  }
//}}}
//{{{
static void read_ipred_modes (sMacroblock* curMb)
{
  sSlice* curSlice = curMb->slice;
  sPicture* picture = curSlice->picture;

  if (curSlice->mbAffFrameFlag) {
    if (curMb->mbType == I8MB)
      read_ipred_8x8_modes_mbaff(curMb);
    else if (curMb->mbType == I4MB)
      read_ipred_4x4_modes_mbaff(curMb);
    }
  else {
    if (curMb->mbType == I8MB)
      read_ipred_8x8_modes(curMb);
    else if (curMb->mbType == I4MB)
      read_ipred_4x4_modes(curMb);
    }

  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
    sSyntaxElement currSE;
    sDataPartition* dP;
    const byte* partMap = assignSE2partition[curSlice->dataPartitionMode];
    sVidParam* vidParam = curMb->vidParam;

    currSE.type = SE_INTRAPREDMODE;
    dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);

    if (vidParam->activePPS->entropyCodingModeFlag == (Boolean)CAVLC || dP->bitstream->eiFlag)
      currSE.mapping = linfo_ue;
    else
      currSE.reading = readCIPredMode_CABAC;

    dP->readsSyntaxElement (curMb, &currSE, dP);
    curMb->cPredMode = (char)currSE.value1;

    if (curMb->cPredMode < DC_PRED_8 || curMb->cPredMode > PLANE_8)
      error ("illegal chroma intra pred mode!\n", 600);
    }
  }
//}}}
//{{{
static void reset_mv_info (sPicMotionParam *mvInfo, int slice_no) {

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
static void reset_mv_info_list (sPicMotionParam *mvInfo, int list, int slice_no) {

  mvInfo->refPic[list] = NULL;
  mvInfo->mv[list] = zero_mv;
  mvInfo->refIndex[list] = -1;
  mvInfo->slice_no = slice_no;
  }
//}}}
//{{{
static void init_macroblock_basic (sMacroblock* curMb) {

  sPicMotionParam** mvInfo = &curMb->slice->picture->mvInfo[curMb->blockY];
  int slice_no =  curMb->slice->curSliceNum;

  // reset vectors and pred. modes
  for (int j = 0; j < BLOCK_SIZE; ++j) {
    int i = curMb->blockX;
    reset_mv_info_list(*mvInfo + (i++), LIST_1, slice_no);
    reset_mv_info_list(*mvInfo + (i++), LIST_1, slice_no);
    reset_mv_info_list(*mvInfo + (i++), LIST_1, slice_no);
    reset_mv_info_list(*(mvInfo++) + i, LIST_1, slice_no);
    }
  }
//}}}
//{{{
static void init_macroblock_direct (sMacroblock* curMb) {

  int slice_no = curMb->slice->curSliceNum;
  sPicMotionParam** mvInfo = &curMb->slice->picture->mvInfo[curMb->blockY];

  set_read_comp_coeff_cabac(curMb);
  set_read_comp_coeff_cavlc(curMb);

  int i = curMb->blockX;
  for (int j = 0; j < BLOCK_SIZE; ++j) {
    (*mvInfo+i)->slice_no = slice_no;
    (*mvInfo+i+1)->slice_no = slice_no;
    (*mvInfo+i+2)->slice_no = slice_no;
    (*(mvInfo++)+i+3)->slice_no = slice_no;
    }
  }
//}}}
//{{{
static void init_macroblock (sMacroblock* curMb) {

  int j, i;
  sSlice* curSlice = curMb->slice;
  sPicMotionParam** mvInfo = &curSlice->picture->mvInfo[curMb->blockY];
  int slice_no = curSlice->curSliceNum;
  // reset vectors and pred. modes

  for(j = 0; j < BLOCK_SIZE; ++j) {
    i = curMb->blockX;
    reset_mv_info (*mvInfo + (i++), slice_no);
    reset_mv_info (*mvInfo + (i++), slice_no);
    reset_mv_info (*mvInfo + (i++), slice_no);
    reset_mv_info (*(mvInfo++) + i, slice_no);
    }

  set_read_comp_coeff_cabac (curMb);
  set_read_comp_coeff_cavlc (curMb);
  }
//}}}
//{{{
static void concealIPCMcoeffs (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  sVidParam *vidParam = curMb->vidParam;
  sPicture* picture = curSlice->picture;
  for (int i = 0; i < MB_BLOCK_SIZE;++i)
    for (int j = 0; j < MB_BLOCK_SIZE;++j)
      curSlice->cof[0][i][j] = vidParam->dcPredValueComp[0];

  if ((picture->chromaFormatIdc != YUV400) && (vidParam->sepColourPlaneFlag == 0))
    for (int k = 0; k < 2; ++k)
      for (int i = 0; i < vidParam->mbCrSizeY;++i)
        for (int j = 0; j < vidParam->mbCrSizeX;++j)
          curSlice->cof[k][i][j] = vidParam->dcPredValueComp[k];
  }
//}}}
//{{{
static void init_decoding_engine_IPCM (sSlice* curSlice) {


  int PartitionNumber;
  if (curSlice->dataPartitionMode == PAR_DP_1)
    PartitionNumber = 1;
  else if (curSlice->dataPartitionMode == PAR_DP_3)
    PartitionNumber = 3;
  else {
    printf ("Partition Mode is not supported\n");
    exit (1);
    }

  for (int i = 0; i < PartitionNumber;++i) {
    sBitstream* stream = curSlice->partArr[i].bitstream;
    int byteStartPosition = stream->readLen;
    arideco_start_decoding (&curSlice->partArr[i].deCabac, stream->streamBuffer, byteStartPosition, &stream->readLen);
    }
  }
//}}}
//{{{
static void read_IPCM_coeffs_from_NAL (sSlice* curSlice, struct DataPartition *dP) {

  sVidParam* vidParam = curSlice->vidParam;
  sPicture* picture = curSlice->picture;
  sSyntaxElement currSE;
  int i,j;

  //For CABAC, we don't need to read bits to let stream byte aligned
  //  because we have variable for integer bytes position
  if (vidParam->activePPS->entropyCodingModeFlag == (Boolean) CABAC) {
    readIPCM_CABAC(curSlice, dP);
    init_decoding_engine_IPCM (curSlice);
    }
  else {
    //read bits to let stream byte aligned
    if (((dP->bitstream->frameBitOffset) & 0x07) != 0) {
      currSE.len = (8 - ((dP->bitstream->frameBitOffset) & 0x07));
      readsSyntaxElement_FLC (&currSE, dP->bitstream);
      }

    //read luma and chroma IPCM coefficients
    currSE.len=vidParam->bitdepthLuma;
    for (i=0;i<MB_BLOCK_SIZE;++i) {
      for (j=0;j<MB_BLOCK_SIZE;++j) {
        readsSyntaxElement_FLC (&currSE, dP->bitstream);
        curSlice->cof[0][i][j] = currSE.value1;
        //curSlice->fcf[0][i][j] = currSE.value1;
        }
      }

    currSE.len=vidParam->bitdepthChroma;
    if ((picture->chromaFormatIdc != YUV400) && (vidParam->sepColourPlaneFlag == 0)) {
      for (i=0;i<vidParam->mbCrSizeY;++i) {
        for (j=0;j<vidParam->mbCrSizeX;++j) {
          readsSyntaxElement_FLC (&currSE, dP->bitstream);
          curSlice->cof[1][i][j] = currSE.value1;
          //curSlice->fcf[1][i][j] = currSE.value1;
          }
        }

      for (i=0;i<vidParam->mbCrSizeY;++i) {
        for (j=0;j<vidParam->mbCrSizeX;++j) {
          readsSyntaxElement_FLC (&currSE, dP->bitstream);
          curSlice->cof[2][i][j] = currSE.value1;
          //curSlice->fcf[2][i][j] = currSE.value1;
          }
        }
      }
    }
  }
//}}}
//{{{
static void SetB8Mode (sMacroblock* curMb, int value, int i) {

  sSlice* curSlice = curMb->slice;
  static const char p_v2b8 [ 5] = {4, 5, 6, 7, IBLOCK};
  static const char p_v2pd [ 5] = {0, 0, 0, 0, -1};
  static const char b_v2b8 [14] = {0, 4, 4, 4, 5, 6, 5, 6, 5, 6, 7, 7, 7, IBLOCK};
  static const char b_v2pd [14] = {2, 0, 1, 2, 0, 0, 1, 1, 2, 2, 0, 1, 2, -1};

  if (curSlice->sliceType==B_SLICE) {
    curMb->b8mode[i] = b_v2b8[value];
    curMb->b8pdir[i] = b_v2pd[value];
    }
  else {
    curMb->b8mode[i] = p_v2b8[value];
    curMb->b8pdir[i] = p_v2pd[value];
    }
  }
//}}}
//{{{
static void reset_coeffs (sMacroblock* curMb) {

  sVidParam *vidParam = curMb->vidParam;

  // CAVLC
  if (vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC)
    memset (vidParam->nzCoeff[curMb->mbAddrX][0][0], 0, 3 * BLOCK_PIXELS * sizeof(byte));
  }
//}}}
//{{{
static void field_flag_inference (sMacroblock* curMb) {

  sVidParam *vidParam = curMb->vidParam;
  if (curMb->mbAvailA)
    curMb->mbField = vidParam->mbData[curMb->mbAddrA].mbField;
  else
    // check top macroblock pair
    curMb->mbField = curMb->mbAvailB ? vidParam->mbData[curMb->mbAddrB].mbField : FALSE;
  }
//}}}
//{{{
void skip_macroblock (sMacroblock* curMb) {

  sMotionVector pred_mv;
  int zeroMotionAbove;
  int zeroMotionLeft;
  sPixelPos mb[4];    // neighbor blocks
  int i, j;
  int a_mv_y = 0;
  int a_ref_idx = 0;
  int b_mv_y = 0;
  int b_ref_idx = 0;
  int img_block_y   = curMb->blockY;
  sVidParam *vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;
  int   listOffset = LIST_0 + curMb->listOffset;
  sPicture* picture = curSlice->picture;
  sMotionVector *a_mv = NULL;
  sMotionVector *b_mv = NULL;

  getNeighbours(curMb, mb, 0, 0, MB_BLOCK_SIZE);
  if (curSlice->mbAffFrameFlag == 0) {
    if (mb[0].available) {
      a_mv = &picture->mvInfo[mb[0].posY][mb[0].posX].mv[LIST_0];
      a_mv_y = a_mv->mvY;
      a_ref_idx = picture->mvInfo[mb[0].posY][mb[0].posX].refIndex[LIST_0];
      }

    if (mb[1].available) {
      b_mv = &picture->mvInfo[mb[1].posY][mb[1].posX].mv[LIST_0];
      b_mv_y = b_mv->mvY;
      b_ref_idx = picture->mvInfo[mb[1].posY][mb[1].posX].refIndex[LIST_0];
      }
    }
  else {
    if (mb[0].available) {
      a_mv = &picture->mvInfo[mb[0].posY][mb[0].posX].mv[LIST_0];
      a_mv_y = a_mv->mvY;
      a_ref_idx = picture->mvInfo[mb[0].posY][mb[0].posX].refIndex[LIST_0];

      if (curMb->mbField && !vidParam->mbData[mb[0].mbAddr].mbField) {
        a_mv_y /=2;
        a_ref_idx *=2;
      }
      if (!curMb->mbField && vidParam->mbData[mb[0].mbAddr].mbField) {
        a_mv_y *=2;
        a_ref_idx >>=1;
        }
      }

    if (mb[1].available) {
      b_mv = &picture->mvInfo[mb[1].posY][mb[1].posX].mv[LIST_0];
      b_mv_y = b_mv->mvY;
      b_ref_idx = picture->mvInfo[mb[1].posY][mb[1].posX].refIndex[LIST_0];

      if (curMb->mbField && !vidParam->mbData[mb[1].mbAddr].mbField) {
        b_mv_y /=2;
        b_ref_idx *=2;
        }
      if (!curMb->mbField && vidParam->mbData[mb[1].mbAddr].mbField) {
        b_mv_y *=2;
        b_ref_idx >>=1;
        }
      }
    }

  zeroMotionLeft  = !mb[0].available ? 1 : a_ref_idx==0 && a_mv->mvX == 0 && a_mv_y==0 ? 1 : 0;
  zeroMotionAbove = !mb[1].available ? 1 : b_ref_idx==0 && b_mv->mvX == 0 && b_mv_y==0 ? 1 : 0;

  curMb->cbp = 0;
  reset_coeffs(curMb);

  if (zeroMotionAbove || zeroMotionLeft) {
    sPicMotionParam** dec_mv_info = &picture->mvInfo[img_block_y];
    sPicture *cur_pic = curSlice->listX[listOffset][0];
    sPicMotionParam *mvInfo = NULL;

    for(j = 0; j < BLOCK_SIZE; ++j) {
      for(i = curMb->blockX; i < curMb->blockX + BLOCK_SIZE; ++i) {
        mvInfo = &dec_mv_info[j][i];
        mvInfo->refPic[LIST_0] = cur_pic;
        mvInfo->mv     [LIST_0] = zero_mv;
        mvInfo->refIndex[LIST_0] = 0;
        }
      }
    }
  else {
    sPicMotionParam** dec_mv_info = &picture->mvInfo[img_block_y];
    sPicMotionParam *mvInfo = NULL;
    sPicture *cur_pic = curSlice->listX[listOffset][0];
    curMb->GetMVPredictor (curMb, mb, &pred_mv, 0, picture->mvInfo, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

    // Set first block line (position img_block_y)
    for (j = 0; j < BLOCK_SIZE; ++j) {
      for (i = curMb->blockX; i < curMb->blockX + BLOCK_SIZE; ++i) {
        mvInfo = &dec_mv_info[j][i];
        mvInfo->refPic[LIST_0] = cur_pic;
        mvInfo->mv[LIST_0] = pred_mv;
        mvInfo->refIndex[LIST_0] = 0;
        }
      }
    }
  }
//}}}

//{{{
static void read_skip_macroblock (sMacroblock* curMb) {

  curMb->lumaTransformSize8x8flag = FALSE;
  if (curMb->vidParam->activePPS->constrainedIntraPredFlag) {
    int mb_nr = curMb->mbAddrX;
    curMb->slice->intraBlock[mb_nr] = 0;
    }

  init_macroblock_basic (curMb);
  skip_macroblock (curMb);
  }
//}}}
//{{{
static void read_intra_macroblock (sMacroblock* curMb) {

  curMb->noMbPartLessThan8x8Flag = TRUE;

  // transform size flag for INTRA_4x4 and INTRA_8x8 modes
  curMb->lumaTransformSize8x8flag = FALSE;

  init_macroblock(curMb);
  read_ipred_modes(curMb);
  curMb->slice->read_CBP_and_coeffs_from_NAL (curMb);
  }
//}}}
//{{{
static void read_intra4x4_macroblock_cavlc (sMacroblock* curMb, const byte *partMap)
{
  sSlice* curSlice = curMb->slice;
  //transform size flag for INTRA_4x4 and INTRA_8x8 modes
  if (curSlice->transform8x8Mode) {
    sSyntaxElement currSE;
    sDataPartition* dP = &(curSlice->partArr[partMap[SE_HEADER]]);
    currSE.type =  SE_HEADER;

    // read CAVLC transform_size_8x8_flag
    currSE.len = (int64)1;
    readsSyntaxElement_FLC (&currSE, dP->bitstream);

    curMb->lumaTransformSize8x8flag = (Boolean)currSE.value1;
    if (curMb->lumaTransformSize8x8flag) {
      curMb->mbType = I8MB;
      memset (&curMb->b8mode, I8MB, 4 * sizeof(char));
      memset (&curMb->b8pdir, -1, 4 * sizeof(char));
    }
  }
  else
    curMb->lumaTransformSize8x8flag = FALSE;

  init_macroblock (curMb);
  read_ipred_modes (curMb);
  curSlice->read_CBP_and_coeffs_from_NAL (curMb);
  }
//}}}
//{{{
static void read_intra4x4_macroblock_cabac (sMacroblock* curMb, const byte *partMap) {


  // transform size flag for INTRA_4x4 and INTRA_8x8 modes
  sSlice* curSlice = curMb->slice;
  if (curSlice->transform8x8Mode) {
   sSyntaxElement currSE;
    sDataPartition *dP = &(curSlice->partArr[partMap[SE_HEADER]]);
    currSE.type   =  SE_HEADER;
    currSE.reading = readMB_transform_size_flag_CABAC;

    // read CAVLC transform_size_8x8_flag
    if (dP->bitstream->eiFlag) {
      currSE.len = (int64) 1;
      readsSyntaxElement_FLC(&currSE, dP->bitstream);
      }
    else
      dP->readsSyntaxElement(curMb, &currSE, dP);

    curMb->lumaTransformSize8x8flag = (Boolean) currSE.value1;
    if (curMb->lumaTransformSize8x8flag) {
      curMb->mbType = I8MB;
      memset(&curMb->b8mode, I8MB, 4 * sizeof(char));
      memset(&curMb->b8pdir, -1, 4 * sizeof(char));
      }
    }
  else
    curMb->lumaTransformSize8x8flag = FALSE;

  init_macroblock(curMb);
  read_ipred_modes(curMb);
  curSlice->read_CBP_and_coeffs_from_NAL (curMb);
  }
//}}}
//{{{
static void read_inter_macroblock (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;

  curMb->noMbPartLessThan8x8Flag = TRUE;
  curMb->lumaTransformSize8x8flag = FALSE;
  if (curMb->vidParam->activePPS->constrainedIntraPredFlag) {
    int mb_nr = curMb->mbAddrX;
    curSlice->intraBlock[mb_nr] = 0;
    }

  init_macroblock (curMb);
  curSlice->readMotionInfoFromNAL (curMb);
  curSlice->read_CBP_and_coeffs_from_NAL (curMb);
  }
//}}}
//{{{
static void read_i_pcm_macroblock (sMacroblock* curMb, const byte *partMap) {

  sSlice* curSlice = curMb->slice;
  curMb->noMbPartLessThan8x8Flag = TRUE;
  curMb->lumaTransformSize8x8flag = FALSE;

  init_macroblock (curMb);

  // here dP is assigned with the same dP as SE_MBTYPE, because IPCM syntax is in the
  // same category as MBTYPE
  if (curSlice->dataPartitionMode && curSlice->noDataPartitionB )
    concealIPCMcoeffs (curMb);
  else {
    sDataPartition* dP = &(curSlice->partArr[partMap[SE_LUM_DC_INTRA]]);
    read_IPCM_coeffs_from_NAL (curSlice, dP);
    }
  }
//}}}
//{{{
static void read_P8x8_macroblock (sMacroblock* curMb, sDataPartition *dP, sSyntaxElement* currSE) {

  int i;
  sSlice* curSlice = curMb->slice;

  // READ 8x8 SUB-PARTITION MODES (modes of 8x8 blocks) and Intra VBST block modes
  curMb->noMbPartLessThan8x8Flag = TRUE;
  curMb->lumaTransformSize8x8flag = FALSE;

  for (i = 0; i < 4; ++i) {
    dP->readsSyntaxElement (curMb, currSE, dP);
    SetB8Mode (curMb, currSE->value1, i);

    // set noMbPartLessThan8x8Flag for P8x8 mode
    curMb->noMbPartLessThan8x8Flag &= (curMb->b8mode[i] == 0 && curSlice->activeSPS->direct_8x8_inference_flag) ||
      (curMb->b8mode[i] == 4);
    }

  init_macroblock (curMb);
  curSlice->readMotionInfoFromNAL (curMb);

  if (curMb->vidParam->activePPS->constrainedIntraPredFlag) {
    int mb_nr = curMb->mbAddrX;
    curSlice->intraBlock[mb_nr] = 0;
    }

  curSlice->read_CBP_and_coeffs_from_NAL (curMb);
  }
//}}}
//{{{
static void read_one_macroblock_i_slice_cavlc (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;

  sSyntaxElement currSE;
  int mb_nr = curMb->mbAddrX;

  sDataPartition* dP;
  const byte* partMap = assignSE2partition[curSlice->dataPartitionMode];
  sPicture* picture = curSlice->picture;
  sPicMotionParamsOld *motion = &picture->motion;

  curMb->mbField = ((mb_nr&0x01) == 0)? FALSE : curSlice->mbData[mb_nr-1].mbField;

  updateQp(curMb, curSlice->qp);
  currSE.type = SE_MBTYPE;

  // read MB mode
  dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

  currSE.mapping = linfo_ue;

  // read MB aff
  if (curSlice->mbAffFrameFlag && (mb_nr&0x01) == 0) {
    currSE.len = (int64) 1;
    readsSyntaxElement_FLC(&currSE, dP->bitstream);
    curMb->mbField = (Boolean) currSE.value1;
    }

  // read MB type
  dP->readsSyntaxElement(curMb, &currSE, dP);

  curMb->mbType = (short) currSE.value1;
  if (!dP->bitstream->eiFlag)
    curMb->eiFlag = 0;

  motion->mbField[mb_nr] = (byte) curMb->mbField;
  curMb->blockYaff = ((curSlice->mbAffFrameFlag) && (curMb->mbField)) ? (mb_nr&0x01) ? (curMb->blockY - 4)>>1 : curMb->blockY >> 1 : curMb->blockY;
  curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
  curSlice->interpretMbMode(curMb);

  //init noMbPartLessThan8x8Flag
  curMb->noMbPartLessThan8x8Flag = TRUE;
  if(curMb->mbType == IPCM)
    read_i_pcm_macroblock (curMb, partMap);
  else if (curMb->mbType == I4MB)
    read_intra4x4_macroblock_cavlc (curMb, partMap);
  else 
    read_intra_macroblock (curMb);
  }
//}}}
//{{{
static void read_one_macroblock_i_slice_cabac (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;

  sSyntaxElement currSE;
  int mb_nr = curMb->mbAddrX;

  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];
  sPicture* picture = curSlice->picture;
  sPicMotionParamsOld *motion = &picture->motion;

  curMb->mbField = ((mb_nr&0x01) == 0) ? FALSE : curSlice->mbData[mb_nr-1].mbField;

  updateQp (curMb, curSlice->qp);
  currSE.type = SE_MBTYPE;

  //  read MB mode**
  dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

  if (dP->bitstream->eiFlag)
    currSE.mapping = linfo_ue;

  // read MB aff
  if (curSlice->mbAffFrameFlag && (mb_nr&0x01)==0) {
    if (dP->bitstream->eiFlag) {
      currSE.len = (int64)1;
      readsSyntaxElement_FLC (&currSE, dP->bitstream);
      }
    else {
      currSE.reading = readFieldModeInfo_CABAC;
      dP->readsSyntaxElement (curMb, &currSE, dP);
      }
    curMb->mbField = (Boolean) currSE.value1;
    }

  CheckAvailabilityOfNeighborsCABAC(curMb);

  //  read MB type
  currSE.reading = readMB_typeInfo_CABAC_i_slice;
  dP->readsSyntaxElement (curMb, &currSE, dP);

  curMb->mbType = (short) currSE.value1;
  if (!dP->bitstream->eiFlag)
    curMb->eiFlag = 0;

  motion->mbField[mb_nr] = (byte) curMb->mbField;
  curMb->blockYaff = ((curSlice->mbAffFrameFlag) && (curMb->mbField)) ? (mb_nr&0x01) ? (curMb->blockY - 4)>>1 : curMb->blockY >> 1 : curMb->blockY;
  curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
  curSlice->interpretMbMode(curMb);

  //init noMbPartLessThan8x8Flag
  curMb->noMbPartLessThan8x8Flag = TRUE;
  if(curMb->mbType == IPCM)
    read_i_pcm_macroblock(curMb, partMap);
  else if (curMb->mbType == I4MB) {
    //transform size flag for INTRA_4x4 and INTRA_8x8 modes
    if (curSlice->transform8x8Mode) {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partArr[partMap[SE_HEADER]]);
      currSE.reading = readMB_transform_size_flag_CABAC;

      // read CAVLC transform_size_8x8_flag
      if (dP->bitstream->eiFlag) {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
        }
      else
        dP->readsSyntaxElement(curMb, &currSE, dP);

      curMb->lumaTransformSize8x8flag = (Boolean) currSE.value1;
      if (curMb->lumaTransformSize8x8flag) {
        curMb->mbType = I8MB;
        memset(&curMb->b8mode, I8MB, 4 * sizeof(char));
        memset(&curMb->b8pdir, -1, 4 * sizeof(char));
      }
    }
    else
      curMb->lumaTransformSize8x8flag = FALSE;

    init_macroblock (curMb);
    read_ipred_modes (curMb);
    curSlice->read_CBP_and_coeffs_from_NAL (curMb);
    }
  else
    read_intra_macroblock (curMb);
  }
//}}}
//{{{
static void read_one_macroblock_p_slice_cavlc (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  sSyntaxElement currSE;
  int mb_nr = curMb->mbAddrX;

  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];

  if (curSlice->mbAffFrameFlag == 0) {
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    curMb->mbField = FALSE;

    updateQp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode**
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.mapping = linfo_ue;

    // VLC Non-Intra
    if (curSlice->codCount == -1) {
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curSlice->codCount = currSE.value1;
      }

    if (curSlice->codCount==0) {
      // read MB type
      dP->readsSyntaxElement(curMb, &currSE, dP);
      ++(currSE.value1);
      curMb->mbType = (short) currSE.value1;
      if(!dP->bitstream->eiFlag)
        curMb->eiFlag = 0;
      curSlice->codCount--;
      curMb->skipFlag = 0;
      }
    else {
      curSlice->codCount--;
      curMb->mbType = 0;
      curMb->eiFlag = 0;
      curMb->skipFlag = 1;
      }

    //update the list offset;
    curMb->listOffset = 0;
    motion->mbField[mb_nr] = (byte) FALSE;
    curMb->blockYaff = curMb->blockY;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpretMbMode(curMb);
    }
  else {
    sVidParam *vidParam = curMb->vidParam;
    sMacroblock *topMB = NULL;
    int  prevMbSkipped = 0;
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    if (mb_nr&0x01) {
      topMB= &vidParam->mbData[mb_nr-1];
      prevMbSkipped = (topMB->mbType == 0);
      }
    else
      prevMbSkipped = 0;

    curMb->mbField = ((mb_nr&0x01) == 0)? FALSE : vidParam->mbData[mb_nr-1].mbField;

    updateQp (curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode**
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.mapping = linfo_ue;

    // VLC Non-Intra
    if (curSlice->codCount == -1) {
      dP->readsSyntaxElement (curMb, &currSE, dP);
      curSlice->codCount = currSE.value1;
      }

    if (curSlice->codCount == 0) {
      // read MB aff
      if ((((mb_nr&0x01)==0) || ((mb_nr&0x01) && prevMbSkipped))) {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC (&currSE, dP->bitstream);
        curMb->mbField = (Boolean) currSE.value1;
        }

      // read MB type
      dP->readsSyntaxElement (curMb, &currSE, dP);
      ++(currSE.value1);
      curMb->mbType = (short)currSE.value1;
      if(!dP->bitstream->eiFlag)
        curMb->eiFlag = 0;
      curSlice->codCount--;
      curMb->skipFlag = 0;
      }
    else {
      curSlice->codCount--;
      curMb->mbType = 0;
      curMb->eiFlag = 0;
      curMb->skipFlag = 1;

      // read field flag of bottom block
      if (curSlice->codCount == 0 && ((mb_nr&0x01) == 0)) {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
        dP->bitstream->frameBitOffset--;
        curMb->mbField = (Boolean) currSE.value1;
        }
      else if (curSlice->codCount > 0 && ((mb_nr & 0x01) == 0)) {
        // check left macroblock pair first
        if (mb_is_available(mb_nr - 2, curMb) && ((mb_nr % (vidParam->PicWidthInMbs * 2))!=0))
          curMb->mbField = vidParam->mbData[mb_nr-2].mbField;
        else {
          // check top macroblock pair
          if (mb_is_available(mb_nr - 2*vidParam->PicWidthInMbs, curMb))
            curMb->mbField = vidParam->mbData[mb_nr-2*vidParam->PicWidthInMbs].mbField;
          else
            curMb->mbField = FALSE;
          }
        }
      }
    //update the list offset;
    curMb->listOffset = (curMb->mbField)? ((mb_nr&0x01)? 4: 2): 0;
    motion->mbField[mb_nr] = (byte) curMb->mbField;
    curMb->blockYaff = (curMb->mbField) ? (mb_nr&0x01) ? (curMb->blockY - 4)>>1 : curMb->blockY >> 1 : curMb->blockY;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpretMbMode(curMb);

    if (curMb->mbField) {
      curSlice->numRefIndexActive[LIST_0] <<=1;
      curSlice->numRefIndexActive[LIST_1] <<=1;
      }
    }

  curMb->noMbPartLessThan8x8Flag = TRUE;

  if (curMb->mbType == IPCM)
    read_i_pcm_macroblock (curMb, partMap);
  else if (curMb->mbType == I4MB)
    read_intra4x4_macroblock_cavlc (curMb, partMap);
  else if (curMb->mbType == P8x8) {
    currSE.type = SE_MBTYPE;
    currSE.mapping = linfo_ue;
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    read_P8x8_macroblock (curMb, dP, &currSE);
    }
  else if (curMb->mbType == PSKIP)
    read_skip_macroblock (curMb);
  else if (curMb->isIntraBlock)
    read_intra_macroblock (curMb);
  else
    read_inter_macroblock (curMb);
  }
//}}}
//{{{
static void read_one_macroblock_p_slice_cabac (sMacroblock* curMb)
{
  sSlice* curSlice = curMb->slice;
  sVidParam *vidParam = curMb->vidParam;
  int mb_nr = curMb->mbAddrX;
  sSyntaxElement currSE;
  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];

  if (curSlice->mbAffFrameFlag == 0) {
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    curMb->mbField = FALSE;

    updateQp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    // read MB mode**
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    if (dP->bitstream->eiFlag)
      currSE.mapping = linfo_ue;

    CheckAvailabilityOfNeighborsCABAC(curMb);
    currSE.reading = read_skip_flag_CABAC_p_slice;
    dP->readsSyntaxElement(curMb, &currSE, dP);

    curMb->mbType   = (short) currSE.value1;
    curMb->skipFlag = (char) (!(currSE.value1));
    if (!dP->bitstream->eiFlag)
      curMb->eiFlag = 0;

    // read MB type
    if (curMb->mbType != 0 ) {
      currSE.reading = readMB_typeInfo_CABAC_p_slice;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mbType = (short) currSE.value1;
      if(!dP->bitstream->eiFlag)
        curMb->eiFlag = 0;
      }

    motion->mbField[mb_nr] = (byte) FALSE;
    curMb->blockYaff = curMb->blockY;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpretMbMode(curMb);
    }
  else {
    sMacroblock *topMB = NULL;
    int  prevMbSkipped = 0;
    int  check_bottom, read_bottom, read_top;
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;
    if (mb_nr&0x01) {
      topMB= &vidParam->mbData[mb_nr-1];
      prevMbSkipped = (topMB->mbType == 0);
      }
    else
      prevMbSkipped = 0;

    curMb->mbField = ((mb_nr&0x01) == 0)? FALSE : vidParam->mbData[mb_nr-1].mbField;

    updateQp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode**
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    if (dP->bitstream->eiFlag)
      currSE.mapping = linfo_ue;

    // read MB skipFlag
    if (((mb_nr&0x01) == 0||prevMbSkipped))
      field_flag_inference(curMb);

    CheckAvailabilityOfNeighborsCABAC(curMb);
    currSE.reading = read_skip_flag_CABAC_p_slice;
    dP->readsSyntaxElement(curMb, &currSE, dP);

    curMb->mbType   = (short) currSE.value1;
    curMb->skipFlag = (char) (!(currSE.value1));

    if (!dP->bitstream->eiFlag)
      curMb->eiFlag = 0;

    // read MB AFF
    check_bottom=read_bottom=read_top=0;
    if ((mb_nr&0x01)==0) {
      check_bottom =  curMb->skipFlag;
      read_top = !check_bottom;
      }
    else
      read_bottom = (topMB->skipFlag && (!curMb->skipFlag));

    if (read_bottom || read_top) {
      currSE.reading = readFieldModeInfo_CABAC;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mbField = (Boolean) currSE.value1;
    }

    if (check_bottom)
      check_next_mb_and_get_field_mode_CABAC_p_slice(curSlice, &currSE, dP);

    // update the list offset;
    curMb->listOffset = (curMb->mbField)? ((mb_nr&0x01)? 4: 2): 0;

    CheckAvailabilityOfNeighborsCABAC(curMb);

    // read MB type
    if (curMb->mbType != 0 ) {
      currSE.reading = readMB_typeInfo_CABAC_p_slice;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mbType = (short) currSE.value1;
      if (!dP->bitstream->eiFlag)
        curMb->eiFlag = 0;
      }

    motion->mbField[mb_nr] = (byte) curMb->mbField;
    curMb->blockYaff = (curMb->mbField) ? (mb_nr&0x01) ? (curMb->blockY - 4)>>1 : curMb->blockY >> 1 : curMb->blockY;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpretMbMode(curMb);
    if (curMb->mbField) {
      curSlice->numRefIndexActive[LIST_0] <<=1;
      curSlice->numRefIndexActive[LIST_1] <<=1;
      }
    }

  curMb->noMbPartLessThan8x8Flag = TRUE;
  if (curMb->mbType == IPCM)
    read_i_pcm_macroblock (curMb, partMap);
  else if (curMb->mbType == I4MB)
    read_intra4x4_macroblock_cabac (curMb, partMap);
  else if (curMb->mbType == P8x8) {
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.type = SE_MBTYPE;

    if (dP->bitstream->eiFlag)
      currSE.mapping = linfo_ue;
    else
      currSE.reading = readB8_typeInfo_CABAC_p_slice;

    read_P8x8_macroblock(curMb, dP, &currSE);
    }
  else if (curMb->mbType == PSKIP)
    read_skip_macroblock (curMb);
  else if (curMb->isIntraBlock == TRUE)
    read_intra_macroblock(curMb);
  else
    read_inter_macroblock(curMb);
  }
//}}}
//{{{
static void read_one_macroblock_b_slice_cavlc (sMacroblock* curMb) {

  sVidParam *vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;
  int mb_nr = curMb->mbAddrX;
  sDataPartition *dP;
  sSyntaxElement currSE;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];

  if (curSlice->mbAffFrameFlag == 0) {
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    curMb->mbField = FALSE;
    updateQp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode**
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.mapping = linfo_ue;

    if(curSlice->codCount == -1) {
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curSlice->codCount = currSE.value1;
      }
    if (curSlice->codCount==0) {
      // read MB type
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mbType = (short) currSE.value1;
      if(!dP->bitstream->eiFlag)
        curMb->eiFlag = 0;
      curSlice->codCount--;
      curMb->skipFlag = 0;
      }
    else {
      curSlice->codCount--;
      curMb->mbType = 0;
      curMb->eiFlag = 0;
      curMb->skipFlag = 1;
      }

    // update the list offset;
    curMb->listOffset = 0;
    motion->mbField[mb_nr] = FALSE;
    curMb->blockYaff = curMb->blockY;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpretMbMode(curMb);
    }
  else {
    sMacroblock *topMB = NULL;
    int  prevMbSkipped = 0;
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    if (mb_nr&0x01) {
      topMB= &vidParam->mbData[mb_nr-1];
      prevMbSkipped = topMB->skipFlag;
      }
    else
      prevMbSkipped = 0;

    curMb->mbField = ((mb_nr&0x01) == 0)? FALSE : vidParam->mbData[mb_nr-1].mbField;

    updateQp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode**
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.mapping = linfo_ue;
    if(curSlice->codCount == -1) {
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curSlice->codCount = currSE.value1;
      }
    if (curSlice->codCount==0) {
      // read MB aff
      if ((((mb_nr&0x01)==0) || ((mb_nr&0x01) && prevMbSkipped))) {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
        curMb->mbField = (Boolean) currSE.value1;
        }

      // read MB type
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mbType = (short) currSE.value1;
      if(!dP->bitstream->eiFlag)
        curMb->eiFlag = 0;
      curSlice->codCount--;
      curMb->skipFlag = 0;
      }
    else {
      curSlice->codCount--;
      curMb->mbType = 0;
      curMb->eiFlag = 0;
      curMb->skipFlag = 1;

      // read field flag of bottom block
      if(curSlice->codCount == 0 && ((mb_nr&0x01) == 0)) {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
        dP->bitstream->frameBitOffset--;
        curMb->mbField = (Boolean) currSE.value1;
        }
      else if (curSlice->codCount > 0 && ((mb_nr & 0x01) == 0)) {
        // check left macroblock pair first
        if (mb_is_available(mb_nr - 2, curMb) && ((mb_nr % (vidParam->PicWidthInMbs * 2))!=0))
          curMb->mbField = vidParam->mbData[mb_nr-2].mbField;
        else {
          // check top macroblock pair
          if (mb_is_available(mb_nr - 2*vidParam->PicWidthInMbs, curMb))
            curMb->mbField = vidParam->mbData[mb_nr-2*vidParam->PicWidthInMbs].mbField;
          else
            curMb->mbField = FALSE;
          }
        }
      }

    // update the list offset;
    curMb->listOffset = (curMb->mbField)? ((mb_nr&0x01)? 4: 2): 0;
    motion->mbField[mb_nr] = (byte) curMb->mbField;
    curMb->blockYaff = (curMb->mbField) ? (mb_nr&0x01) ? (curMb->blockY - 4)>>1 : curMb->blockY >> 1 : curMb->blockY;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpretMbMode(curMb);
    if (curSlice->mbAffFrameFlag) {
      if (curMb->mbField) {
        curSlice->numRefIndexActive[LIST_0] <<=1;
        curSlice->numRefIndexActive[LIST_1] <<=1;
        }
      }
    }

  if (curMb->mbType == IPCM)
    read_i_pcm_macroblock (curMb, partMap);
  else if (curMb->mbType == I4MB)
    read_intra4x4_macroblock_cavlc (curMb, partMap);
  else if (curMb->mbType == P8x8) {
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.type = SE_MBTYPE;
    currSE.mapping = linfo_ue;
    read_P8x8_macroblock (curMb, dP, &currSE);
    }
  else if (curMb->mbType == BSKIP_DIRECT) {
    // init noMbPartLessThan8x8Flag
    curMb->noMbPartLessThan8x8Flag = (!(curSlice->activeSPS->direct_8x8_inference_flag))? FALSE: TRUE;
    curMb->lumaTransformSize8x8flag = FALSE;
    if(vidParam->activePPS->constrainedIntraPredFlag)
      curSlice->intraBlock[mb_nr] = 0;

    init_macroblock_direct(curMb);
    if (curSlice->codCount >= 0) {
      curMb->cbp = 0;
      reset_coeffs(curMb);
      }
    else
      curSlice->read_CBP_and_coeffs_from_NAL (curMb);
    }
  else if (curMb->isIntraBlock == TRUE)
    read_intra_macroblock(curMb);
  else
    read_inter_macroblock(curMb);
  }
//}}}
//{{{
static void read_one_macroblock_b_slice_cabac (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  sVidParam *vidParam = curMb->vidParam;
  int mb_nr = curMb->mbAddrX;
  sSyntaxElement currSE;

  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dataPartitionMode];

  if (curSlice->mbAffFrameFlag == 0) {
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    curMb->mbField = FALSE;

    updateQp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode**
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    if (dP->bitstream->eiFlag)
      currSE.mapping = linfo_ue;

    CheckAvailabilityOfNeighborsCABAC(curMb);
    currSE.reading = read_skip_flag_CABAC_b_slice;
    dP->readsSyntaxElement(curMb, &currSE, dP);

    curMb->mbType   = (short) currSE.value1;
    curMb->skipFlag = (char) (!(currSE.value1));

    curMb->cbp = currSE.value2;

    if (!dP->bitstream->eiFlag)
      curMb->eiFlag = 0;

    if (currSE.value1 == 0 && currSE.value2 == 0)
      curSlice->codCount=0;

    // read MB type
    if (curMb->mbType != 0 ) {
      currSE.reading = readMB_typeInfo_CABAC_b_slice;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mbType = (short) currSE.value1;
      if(!dP->bitstream->eiFlag)
        curMb->eiFlag = 0;
      }

    motion->mbField[mb_nr] = (byte) FALSE;
    curMb->blockYaff = curMb->blockY;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpretMbMode(curMb);
    }
  else {
    sMacroblock *topMB = NULL;
    int  prevMbSkipped = 0;
    int  check_bottom, read_bottom, read_top;
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    if (mb_nr&0x01) {
      topMB= &vidParam->mbData[mb_nr-1];
      prevMbSkipped = topMB->skipFlag;
      }
    else
      prevMbSkipped = 0;

    curMb->mbField = ((mb_nr&0x01) == 0)? FALSE : vidParam->mbData[mb_nr-1].mbField;

    updateQp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode**
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

    if (dP->bitstream->eiFlag)
      currSE.mapping = linfo_ue;

    // read MB skipFlag
    if (((mb_nr&0x01) == 0||prevMbSkipped))
      field_flag_inference(curMb);

    CheckAvailabilityOfNeighborsCABAC(curMb);
    currSE.reading = read_skip_flag_CABAC_b_slice;
    dP->readsSyntaxElement(curMb, &currSE, dP);

    curMb->mbType   = (short) currSE.value1;
    curMb->skipFlag = (char) (!(currSE.value1));
    curMb->cbp = currSE.value2;

    if (!dP->bitstream->eiFlag)
      curMb->eiFlag = 0;

    if (currSE.value1 == 0 && currSE.value2 == 0)
      curSlice->codCount=0;

    // read MB AFF
    check_bottom=read_bottom=read_top=0;
    if ((mb_nr & 0x01) == 0) {
      check_bottom =  curMb->skipFlag;
      read_top = !check_bottom;
      }
    else
      read_bottom = (topMB->skipFlag && (!curMb->skipFlag));

    if (read_bottom || read_top) {
      currSE.reading = readFieldModeInfo_CABAC;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mbField = (Boolean) currSE.value1;
      }
    if (check_bottom)
      check_next_mb_and_get_field_mode_CABAC_b_slice(curSlice, &currSE, dP);

    //update the list offset;
    curMb->listOffset = (curMb->mbField)? ((mb_nr&0x01)? 4: 2): 0;

    CheckAvailabilityOfNeighborsCABAC(curMb);

    // read MB type
    if (curMb->mbType != 0 ) {
      currSE.reading = readMB_typeInfo_CABAC_b_slice;
      dP->readsSyntaxElement (curMb, &currSE, dP);
      curMb->mbType = (short)currSE.value1;
      if(!dP->bitstream->eiFlag)
        curMb->eiFlag = 0;
      }

    motion->mbField[mb_nr] = (byte) curMb->mbField;
    curMb->blockYaff = (curMb->mbField) ? (mb_nr&0x01) ? (curMb->blockY - 4)>>1 : curMb->blockY >> 1 : curMb->blockY;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpretMbMode(curMb);
    if (curMb->mbField) {
      curSlice->numRefIndexActive[LIST_0] <<=1;
      curSlice->numRefIndexActive[LIST_1] <<=1;
      }
    }

  if(curMb->mbType == IPCM)
    read_i_pcm_macroblock (curMb, partMap);
  else if (curMb->mbType == I4MB)
    read_intra4x4_macroblock_cabac (curMb, partMap);
  else if (curMb->mbType == P8x8) {
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.type = SE_MBTYPE;
    if (dP->bitstream->eiFlag)
      currSE.mapping = linfo_ue;
    else
      currSE.reading = readB8_typeInfo_CABAC_b_slice;
    read_P8x8_macroblock(curMb, dP, &currSE);
    }
  else if (curMb->mbType == BSKIP_DIRECT) {
    //init noMbPartLessThan8x8Flag
    curMb->noMbPartLessThan8x8Flag = (!(curSlice->activeSPS->direct_8x8_inference_flag))? FALSE: TRUE;

    //============= Transform Size Flag for INTRA MBs =============
    //transform size flag for INTRA_4x4 and INTRA_8x8 modes
    curMb->lumaTransformSize8x8flag = FALSE;
    if(vidParam->activePPS->constrainedIntraPredFlag)
      curSlice->intraBlock[mb_nr] = 0;

    //--- init macroblock data
    init_macroblock_direct (curMb);

    if (curSlice->codCount >= 0) {
      curSlice->is_reset_coeff = TRUE;
      curMb->cbp = 0;
      curSlice->codCount = -1;
      }
    else // read CBP and Coeffs **
      curSlice->read_CBP_and_coeffs_from_NAL (curMb);
    }
  else if (curMb->isIntraBlock == TRUE)
    read_intra_macroblock (curMb);
  else
    read_inter_macroblock (curMb);
  }
//}}}
//{{{
void setReadMacroblock (sSlice* curSlice) {

  if (curSlice->vidParam->activePPS->entropyCodingModeFlag == (Boolean) CABAC) {
    switch (curSlice->sliceType) {
      case P_SLICE:
      case SP_SLICE:
        curSlice->readOneMacroblock = read_one_macroblock_p_slice_cabac;
        break;
      case B_SLICE:
        curSlice->readOneMacroblock = read_one_macroblock_b_slice_cabac;
        break;
      case I_SLICE:
      case SI_SLICE:
        curSlice->readOneMacroblock = read_one_macroblock_i_slice_cabac;
        break;
      default:
        printf("Unsupported slice type\n");
        break;
      }
    }

  else {
    switch (curSlice->sliceType) {
      case P_SLICE:
      case SP_SLICE:
        curSlice->readOneMacroblock = read_one_macroblock_p_slice_cavlc;
        break;
      case B_SLICE:
        curSlice->readOneMacroblock = read_one_macroblock_b_slice_cavlc;
        break;
      case I_SLICE:
      case SI_SLICE:
        curSlice->readOneMacroblock = read_one_macroblock_i_slice_cavlc;
        break;
      default:
        printf("Unsupported slice type\n");
        break;
      }
    }
  }
//}}}
