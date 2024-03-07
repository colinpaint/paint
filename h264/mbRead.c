//{{{  includes
#include <math.h>

#include "global.h"
#include "elements.h"

#include "block.h"
#include "mbuffer.h"
#include "macroblock.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "image.h"
#include "mbAccess.h"
#include "biariDecode.h"
#include "transform.h"
#include "mcPrediction.h"
#include "quant.h"
#include "mbPrediction.h"
//}}}

//{{{
static void read_ipred_8x8_modes_mbaff (sMacroblock* curMb) {

  int bi, bj, bx, by, dec;
  sSyntaxElement currSE;
  sDataPartition* dP;
  sSlice* curSlice = curMb->slice;
  const byte* partMap = assignSE2partition[curSlice->dp_mode];
  sVidParam* vidParam = curMb->vidParam;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_block, top_block;
  currSE.type = SE_INTRAPREDMODE;
  dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);

  if (!(vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag))
    currSE.reading = readIntraPredMode_CABAC;

  for (int b8 = 0; b8 < 4; ++b8)  {
    // loop 8x8 blocks
    by = (b8 & 0x02);
    bj = curMb->block_y + by;

    bx = ((b8 & 0x01) << 1);
    bi = curMb->block_x + bx;
    // get from stream
    if (vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag)
      readsSyntaxElement_Intra4x4PredictionMode (&currSE, dP->bitstream);
    else {
      currSE.context = (b8 << 2);
      dP->readsSyntaxElement(curMb, &currSE, dP);
    }

    get4x4Neighbour (curMb, (bx << 2) - 1, (by << 2),     vidParam->mb_size[IS_LUMA], &left_block);
    get4x4Neighbour (curMb, (bx << 2),     (by << 2) - 1, vidParam->mb_size[IS_LUMA], &top_block );

    // get from array and decode
    if (vidParam->activePPS->constrained_intra_pred_flag) {
      left_block.available = left_block.available ? curSlice->intraBlock[left_block.mb_addr] : 0;
      top_block.available = top_block.available  ? curSlice->intraBlock[top_block.mb_addr]  : 0;
      }

    upIntraPredMode = (top_block.available ) ? curSlice->predMode[top_block.pos_y ][top_block.pos_x ] : -1;
    leftIntraPredMode = (left_block.available) ? curSlice->predMode[left_block.pos_y][left_block.pos_x] : -1;
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
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  sVidParam *vidParam = curMb->vidParam;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  currSE.type = SE_INTRAPREDMODE;

  dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);

  if (!(vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag))
    currSE.reading = readIntraPredMode_CABAC;

  get4x4Neighbour(curMb, -1,  0, vidParam->mb_size[IS_LUMA], &left_mb);
  get4x4Neighbour(curMb,  0, -1, vidParam->mb_size[IS_LUMA], &top_mb );

  for(b8 = 0; b8 < 4; ++b8)  //loop 8x8 blocks
  {
    by = (b8 & 0x02);
    bj = curMb->block_y + by;

    bx = ((b8 & 0x01) << 1);
    bi = curMb->block_x + bx;

    //get from stream
    if (vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag)
      readsSyntaxElement_Intra4x4PredictionMode(&currSE, dP->bitstream);
    else
    {
      currSE.context = (b8 << 2);
      dP->readsSyntaxElement(curMb, &currSE, dP);
    }

    get4x4Neighbour(curMb, (bx<<2) - 1, (by<<2),     vidParam->mb_size[IS_LUMA], &left_block);
    get4x4Neighbour(curMb, (bx<<2),     (by<<2) - 1, vidParam->mb_size[IS_LUMA], &top_block );

    //get from array and decode

    if (vidParam->activePPS->constrained_intra_pred_flag)
    {
      left_block.available = left_block.available ? curSlice->intraBlock[left_block.mb_addr] : 0;
      top_block.available  = top_block.available  ? curSlice->intraBlock[top_block.mb_addr]  : 0;
    }

    upIntraPredMode            = (top_block.available ) ? curSlice->predMode[top_block.pos_y ][top_block.pos_x ] : -1;
    leftIntraPredMode          = (left_block.available) ? curSlice->predMode[left_block.pos_y][left_block.pos_x] : -1;

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
static void read_ipred_4x4_modes_mbaff (sMacroblock* curMb)
{
  int b8,i,j,bi,bj,bx,by;
  sSyntaxElement currSE;
  sDataPartition *dP;
  sSlice* curSlice = curMb->slice;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  sVidParam *vidParam = curMb->vidParam;
  sBlockPos *picPos = vidParam->picPos;

  int ts, ls;
  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_block, top_block;

  currSE.type = SE_INTRAPREDMODE;

  dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);

  if (!(vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag))
    currSE.reading = readIntraPredMode_CABAC;

  for(b8 = 0; b8 < 4; ++b8)  //loop 8x8 blocks
  {
    for(j = 0; j < 2; j++)  //loop subblocks
    {
      by = (b8 & 0x02) + j;
      bj = curMb->block_y + by;

      for(i = 0; i < 2; i++)
      {
        bx = ((b8 & 1) << 1) + i;
        bi = curMb->block_x + bx;
        //get from stream
        if (vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag)
          readsSyntaxElement_Intra4x4PredictionMode(&currSE, dP->bitstream);
        else
        {
          currSE.context=(b8<<2) + (j<<1) +i;
          dP->readsSyntaxElement(curMb, &currSE, dP);
        }

        get4x4Neighbour(curMb, (bx<<2) - 1, (by<<2),     vidParam->mb_size[IS_LUMA], &left_block);
        get4x4Neighbour(curMb, (bx<<2),     (by<<2) - 1, vidParam->mb_size[IS_LUMA], &top_block );

        //get from array and decode

        if (vidParam->activePPS->constrained_intra_pred_flag)
        {
          left_block.available = left_block.available ? curSlice->intraBlock[left_block.mb_addr] : 0;
          top_block.available  = top_block.available  ? curSlice->intraBlock[top_block.mb_addr]  : 0;
        }

        // !! KS: not sure if the following is still correct...
        ts = ls = 0;   // Check to see if the neighboring block is SI
        if (curSlice->slice_type == SI_SLICE)           // need support for MBINTLC1
        {
          if (left_block.available)
            if (curSlice->siBlock [picPos[left_block.mb_addr].y][picPos[left_block.mb_addr].x])
              ls=1;

          if (top_block.available)
            if (curSlice->siBlock [picPos[top_block.mb_addr].y][picPos[top_block.mb_addr].x])
              ts=1;
        }

        upIntraPredMode            = (top_block.available  &&(ts == 0)) ? curSlice->predMode[top_block.pos_y ][top_block.pos_x ] : -1;
        leftIntraPredMode          = (left_block.available &&(ls == 0)) ? curSlice->predMode[left_block.pos_y][left_block.pos_x] : -1;

        mostProbableIntraPredMode  = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;

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
  const byte* partMap = assignSE2partition[curSlice->dp_mode];
  sVidParam* vidParam = curMb->vidParam;
  sBlockPos* picPos = vidParam->picPos;

  int mostProbableIntraPredMode;
  int upIntraPredMode;
  int leftIntraPredMode;

  sPixelPos left_mb, top_mb;
  sPixelPos left_block, top_block;

  currSE.type = SE_INTRAPREDMODE;
  dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);
  if (!(vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag))
    currSE.reading = readIntraPredMode_CABAC;

  get4x4Neighbour (curMb, -1,  0, vidParam->mb_size[IS_LUMA], &left_mb);
  get4x4Neighbour (curMb,  0, -1, vidParam->mb_size[IS_LUMA], &top_mb );

  for (int b8 = 0; b8 < 4; ++b8) {
    // loop 8x8 blocks
    for (int j = 0; j < 2; j++) {
      // loop subblocks
      int by = (b8 & 0x02) + j;
      int bj = curMb->block_y + by;
      for (int i = 0; i < 2; i++) {
        int bx = ((b8 & 1) << 1) + i;
        int bi = curMb->block_x + bx;

        // get from stream
        if (vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag)
          readsSyntaxElement_Intra4x4PredictionMode (&currSE, dP->bitstream);
        else {
          currSE.context=(b8<<2) + (j<<1) +i;
          dP->readsSyntaxElement (curMb, &currSE, dP);
          }

        get4x4Neighbour(curMb, (bx<<2) - 1, (by<<2),     vidParam->mb_size[IS_LUMA], &left_block);
        get4x4Neighbour(curMb, (bx<<2),     (by<<2) - 1, vidParam->mb_size[IS_LUMA], &top_block );

        //get from array and decode
        if (vidParam->activePPS->constrained_intra_pred_flag) {
          left_block.available = left_block.available ? curSlice->intraBlock[left_block.mb_addr] : 0;
          top_block.available = top_block.available  ? curSlice->intraBlock[top_block.mb_addr]  : 0;
          }

        int ts = 0;
        int ls = 0;   // Check to see if the neighboring block is SI
        if (curSlice->slice_type == SI_SLICE) {
          //{{{  need support for MBINTLC1
          if (left_block.available)
            if (curSlice->siBlock [picPos[left_block.mb_addr].y][picPos[left_block.mb_addr].x])
              ls = 1;

          if (top_block.available)
            if (curSlice->siBlock [picPos[top_block.mb_addr].y][picPos[top_block.mb_addr].x])
              ts = 1;
          }
          //}}}

        upIntraPredMode = (top_block.available  &&(ts == 0)) ? curSlice->predMode[top_block.pos_y ][top_block.pos_x ] : -1;
        leftIntraPredMode = (left_block.available &&(ls == 0)) ? curSlice->predMode[left_block.pos_y][left_block.pos_x] : -1;
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

  if (curSlice->mb_aff_frame_flag)
  {
    if (curMb->mb_type == I8MB)
      read_ipred_8x8_modes_mbaff(curMb);
    else if (curMb->mb_type == I4MB)
      read_ipred_4x4_modes_mbaff(curMb);
  }
  else
  {
  if (curMb->mb_type == I8MB)
    read_ipred_8x8_modes(curMb);
  else if (curMb->mb_type == I4MB)
    read_ipred_4x4_modes(curMb);
  }

  if ((picture->chroma_format_idc != YUV400) && (picture->chroma_format_idc != YUV444))
  {
    sSyntaxElement currSE;
    sDataPartition *dP;
    const byte *partMap = assignSE2partition[curSlice->dp_mode];
    sVidParam *vidParam = curMb->vidParam;

    currSE.type = SE_INTRAPREDMODE;
    dP = &(curSlice->partArr[partMap[SE_INTRAPREDMODE]]);

    if (vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC || dP->bitstream->ei_flag)
      currSE.mapping = linfo_ue;
    else
      currSE.reading = readCIPredMode_CABAC;

    dP->readsSyntaxElement(curMb, &currSE, dP);
    curMb->c_ipred_mode = (char) currSE.value1;

    if (curMb->c_ipred_mode < DC_PRED_8 || curMb->c_ipred_mode > PLANE_8)
    {
      error("illegal chroma intra pred mode!\n", 600);
    }
  }
}
//}}}
//{{{
static void reset_mv_info (sPicMotionParam *mv_info, int slice_no)
{
  mv_info->ref_pic[LIST_0] = NULL;
  mv_info->ref_pic[LIST_1] = NULL;
  mv_info->mv[LIST_0] = zero_mv;
  mv_info->mv[LIST_1] = zero_mv;
  mv_info->ref_idx[LIST_0] = -1;
  mv_info->ref_idx[LIST_1] = -1;
  mv_info->slice_no = slice_no;
}
//}}}
//{{{
static void reset_mv_info_list (sPicMotionParam *mv_info, int list, int slice_no) {

  mv_info->ref_pic[list] = NULL;
  mv_info->mv[list] = zero_mv;
  mv_info->ref_idx[list] = -1;
  mv_info->slice_no = slice_no;
  }
//}}}
//{{{
static void init_macroblock_basic (sMacroblock* curMb) {

  sPicMotionParam** mv_info = &curMb->slice->picture->mv_info[curMb->block_y];
  int slice_no =  curMb->slice->current_slice_nr;

  // reset vectors and pred. modes
  for (int j = 0; j < BLOCK_SIZE; ++j) {
    int i = curMb->block_x;
    reset_mv_info_list(*mv_info + (i++), LIST_1, slice_no);
    reset_mv_info_list(*mv_info + (i++), LIST_1, slice_no);
    reset_mv_info_list(*mv_info + (i++), LIST_1, slice_no);
    reset_mv_info_list(*(mv_info++) + i, LIST_1, slice_no);
    }
  }
//}}}
//{{{
static void init_macroblock_direct (sMacroblock* curMb) {

  int slice_no = curMb->slice->current_slice_nr;
  sPicMotionParam** mv_info = &curMb->slice->picture->mv_info[curMb->block_y];

  set_read_comp_coeff_cabac(curMb);
  set_read_comp_coeff_cavlc(curMb);

  int i = curMb->block_x;
  for (int j = 0; j < BLOCK_SIZE; ++j) {
    (*mv_info+i)->slice_no = slice_no;
    (*mv_info+i+1)->slice_no = slice_no;
    (*mv_info+i+2)->slice_no = slice_no;
    (*(mv_info++)+i+3)->slice_no = slice_no;
    }
  }
//}}}
//{{{
static void init_macroblock (sMacroblock* curMb)
{
  int j, i;
  sSlice* curSlice = curMb->slice;
  sPicMotionParam** mv_info = &curSlice->picture->mv_info[curMb->block_y];
  int slice_no = curSlice->current_slice_nr;
  // reset vectors and pred. modes

  for(j = 0; j < BLOCK_SIZE; ++j)
  {
    i = curMb->block_x;
    reset_mv_info(*mv_info + (i++), slice_no);
    reset_mv_info(*mv_info + (i++), slice_no);
    reset_mv_info(*mv_info + (i++), slice_no);
    reset_mv_info(*(mv_info++) + i, slice_no);
  }

  set_read_comp_coeff_cabac(curMb);
  set_read_comp_coeff_cavlc(curMb);
}
//}}}
//{{{
static void concealIPCMcoeffs (sMacroblock* curMb)
{
  sSlice* curSlice = curMb->slice;
  sVidParam *vidParam = curMb->vidParam;
  sPicture* picture = curSlice->picture;
  int i, j, k;

  for(i=0;i<MB_BLOCK_SIZE;++i)
    for(j=0;j<MB_BLOCK_SIZE;++j)
      curSlice->cof[0][i][j] = vidParam->dc_pred_value_comp[0];
      //curSlice->fcf[0][i][j] = vidParam->dc_pred_value_comp[0];

  if ((picture->chroma_format_idc != YUV400) && (vidParam->separate_colour_plane_flag == 0))
    for (k = 0; k < 2; ++k)
      for(i=0;i<vidParam->mb_cr_size_y;++i)
        for(j=0;j<vidParam->mb_cr_size_x;++j)
          curSlice->cof[k][i][j] = vidParam->dc_pred_value_comp[k];
          //curSlice->fcf[k][i][j] = vidParam->dc_pred_value_comp[k];
}
//}}}
//{{{
static void init_decoding_engine_IPCM (sSlice* curSlice)
{
  sBitstream* curStream;
  int ByteStartPosition;
  int PartitionNumber;
  int i;

  if(curSlice->dp_mode==PAR_DP_1)
    PartitionNumber=1;
  else if(curSlice->dp_mode==PAR_DP_3)
    PartitionNumber=3;
  else
  {
    printf("Partition Mode is not supported\n");
    exit(1);
  }

  for(i=0;i<PartitionNumber;++i)
  {
    curStream = curSlice->partArr[i].bitstream;
    ByteStartPosition = curStream->read_len;

    arideco_start_decoding (&curSlice->partArr[i].de_cabac, curStream->streamBuffer, ByteStartPosition, &curStream->read_len);
  }
}
//}}}
//{{{
static void read_IPCM_coeffs_from_NAL (sSlice* curSlice, struct DataPartition *dP)
{
  sVidParam *vidParam = curSlice->vidParam;

  sPicture* picture = curSlice->picture;
  sSyntaxElement currSE;
  int i,j;

  //For CABAC, we don't need to read bits to let stream byte aligned
  //  because we have variable for integer bytes position
  if(vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CABAC)
  {
    readIPCM_CABAC(curSlice, dP);
    init_decoding_engine_IPCM(curSlice);
  }
  else
  {
    //read bits to let stream byte aligned

    if(((dP->bitstream->frame_bitoffset) & 0x07) != 0)
    {
      currSE.len = (8 - ((dP->bitstream->frame_bitoffset) & 0x07));
      readsSyntaxElement_FLC(&currSE, dP->bitstream);
    }

    //read luma and chroma IPCM coefficients
    currSE.len=vidParam->bitdepth_luma;

    for(i=0;i<MB_BLOCK_SIZE;++i)
    {
      for(j=0;j<MB_BLOCK_SIZE;++j)
      {
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
        curSlice->cof[0][i][j] = currSE.value1;
        //curSlice->fcf[0][i][j] = currSE.value1;
      }
    }
    currSE.len=vidParam->bitdepth_chroma;
    if ((picture->chroma_format_idc != YUV400) && (vidParam->separate_colour_plane_flag == 0))
    {
      for(i=0;i<vidParam->mb_cr_size_y;++i)
      {
        for(j=0;j<vidParam->mb_cr_size_x;++j)
        {
          readsSyntaxElement_FLC(&currSE, dP->bitstream);
          curSlice->cof[1][i][j] = currSE.value1;
          //curSlice->fcf[1][i][j] = currSE.value1;
        }
      }
      for(i=0;i<vidParam->mb_cr_size_y;++i)
      {
        for(j=0;j<vidParam->mb_cr_size_x;++j)
        {
          readsSyntaxElement_FLC(&currSE, dP->bitstream);
          curSlice->cof[2][i][j] = currSE.value1;
          //curSlice->fcf[2][i][j] = currSE.value1;
        }
      }
    }
  }
}
//}}}
//{{{
static void SetB8Mode (sMacroblock* curMb, int value, int i)
{
  sSlice* curSlice = curMb->slice;
  static const char p_v2b8 [ 5] = {4, 5, 6, 7, IBLOCK};
  static const char p_v2pd [ 5] = {0, 0, 0, 0, -1};
  static const char b_v2b8 [14] = {0, 4, 4, 4, 5, 6, 5, 6, 5, 6, 7, 7, 7, IBLOCK};
  static const char b_v2pd [14] = {2, 0, 1, 2, 0, 0, 1, 1, 2, 2, 0, 1, 2, -1};

  if (curSlice->slice_type==B_SLICE)
  {
    curMb->b8mode[i] = b_v2b8[value];
    curMb->b8pdir[i] = b_v2pd[value];
  }
  else
  {
    curMb->b8mode[i] = p_v2b8[value];
    curMb->b8pdir[i] = p_v2pd[value];
  }
}
//}}}
//{{{
static void reset_coeffs (sMacroblock* curMb)
{
  sVidParam *vidParam = curMb->vidParam;

  // CAVLC
  if (vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CAVLC)
    memset (vidParam->nzCoeff[curMb->mbAddrX][0][0], 0, 3 * BLOCK_PIXELS * sizeof(byte));
}
//}}}
//{{{
static void field_flag_inference (sMacroblock* curMb)
{
  sVidParam *vidParam = curMb->vidParam;
  if (curMb->mbAvailA)
    curMb->mb_field = vidParam->mbData[curMb->mbAddrA].mb_field;
  else
    // check top macroblock pair
    curMb->mb_field = curMb->mbAvailB ? vidParam->mbData[curMb->mbAddrB].mb_field : FALSE;
  }
//}}}
//{{{
void skip_macroblock (sMacroblock* curMb) {

  sMotionVector pred_mv;
  int zeroMotionAbove;
  int zeroMotionLeft;
  sPixelPos mb[4];    // neighbor blocks
  int   i, j;
  int   a_mv_y = 0;
  int   a_ref_idx = 0;
  int   b_mv_y = 0;
  int   b_ref_idx = 0;
  int   img_block_y   = curMb->block_y;
  sVidParam *vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;
  int   list_offset = LIST_0 + curMb->list_offset;
  sPicture* picture = curSlice->picture;
  sMotionVector *a_mv = NULL;
  sMotionVector *b_mv = NULL;

  get_neighbors(curMb, mb, 0, 0, MB_BLOCK_SIZE);
  if (curSlice->mb_aff_frame_flag == 0)
  {
    if (mb[0].available)
    {
      a_mv      = &picture->mv_info[mb[0].pos_y][mb[0].pos_x].mv[LIST_0];
      a_mv_y    = a_mv->mv_y;
      a_ref_idx = picture->mv_info[mb[0].pos_y][mb[0].pos_x].ref_idx[LIST_0];
    }

    if (mb[1].available)
    {
      b_mv      = &picture->mv_info[mb[1].pos_y][mb[1].pos_x].mv[LIST_0];
      b_mv_y    = b_mv->mv_y;
      b_ref_idx = picture->mv_info[mb[1].pos_y][mb[1].pos_x].ref_idx[LIST_0];
    }
  }
  else
  {
    if (mb[0].available)
    {
      a_mv      = &picture->mv_info[mb[0].pos_y][mb[0].pos_x].mv[LIST_0];
      a_mv_y    = a_mv->mv_y;
      a_ref_idx = picture->mv_info[mb[0].pos_y][mb[0].pos_x].ref_idx[LIST_0];

      if (curMb->mb_field && !vidParam->mbData[mb[0].mb_addr].mb_field)
      {
        a_mv_y    /=2;
        a_ref_idx *=2;
      }
      if (!curMb->mb_field && vidParam->mbData[mb[0].mb_addr].mb_field)
      {
        a_mv_y    *=2;
        a_ref_idx >>=1;
      }
    }

    if (mb[1].available)
    {
      b_mv      = &picture->mv_info[mb[1].pos_y][mb[1].pos_x].mv[LIST_0];
      b_mv_y    = b_mv->mv_y;
      b_ref_idx = picture->mv_info[mb[1].pos_y][mb[1].pos_x].ref_idx[LIST_0];

      if (curMb->mb_field && !vidParam->mbData[mb[1].mb_addr].mb_field)
      {
        b_mv_y    /=2;
        b_ref_idx *=2;
      }
      if (!curMb->mb_field && vidParam->mbData[mb[1].mb_addr].mb_field)
      {
        b_mv_y    *=2;
        b_ref_idx >>=1;
      }
    }
  }

  zeroMotionLeft  = !mb[0].available ? 1 : a_ref_idx==0 && a_mv->mv_x == 0 && a_mv_y==0 ? 1 : 0;
  zeroMotionAbove = !mb[1].available ? 1 : b_ref_idx==0 && b_mv->mv_x == 0 && b_mv_y==0 ? 1 : 0;

  curMb->cbp = 0;
  reset_coeffs(curMb);

  if (zeroMotionAbove || zeroMotionLeft)
  {
    sPicMotionParam** dec_mv_info = &picture->mv_info[img_block_y];
    sPicture *cur_pic = curSlice->listX[list_offset][0];
    sPicMotionParam *mv_info = NULL;

    for(j = 0; j < BLOCK_SIZE; ++j)
    {
      for(i = curMb->block_x; i < curMb->block_x + BLOCK_SIZE; ++i)
      {
        mv_info = &dec_mv_info[j][i];
        mv_info->ref_pic[LIST_0] = cur_pic;
        mv_info->mv     [LIST_0] = zero_mv;
        mv_info->ref_idx[LIST_0] = 0;
      }
    }
  }
  else
  {
    sPicMotionParam** dec_mv_info = &picture->mv_info[img_block_y];
    sPicMotionParam *mv_info = NULL;
    sPicture *cur_pic = curSlice->listX[list_offset][0];
    curMb->GetMVPredictor (curMb, mb, &pred_mv, 0, picture->mv_info, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

    // Set first block line (position img_block_y)
    for(j = 0; j < BLOCK_SIZE; ++j)
    {
      for(i = curMb->block_x; i < curMb->block_x + BLOCK_SIZE; ++i)
      {
        mv_info = &dec_mv_info[j][i];
        mv_info->ref_pic[LIST_0] = cur_pic;
        mv_info->mv     [LIST_0] = pred_mv;
        mv_info->ref_idx[LIST_0] = 0;
      }
    }
  }
}
//}}}

//{{{
static void read_skip_macroblock (sMacroblock* curMb)
{
  curMb->luma_transform_size_8x8_flag = FALSE;

  if(curMb->vidParam->activePPS->constrained_intra_pred_flag)
  {
    int mb_nr = curMb->mbAddrX;
    curMb->slice->intraBlock[mb_nr] = 0;
  }

  //--- init macroblock data ---
  init_macroblock_basic(curMb);

  skip_macroblock(curMb);
}
//}}}
//{{{
static void read_intra_macroblock (sMacroblock* curMb)
{
  //init NoMbPartLessThan8x8Flag
  curMb->NoMbPartLessThan8x8Flag = TRUE;

  //transform size flag for INTRA_4x4 and INTRA_8x8 modes
  curMb->luma_transform_size_8x8_flag = FALSE;

  //--- init macroblock data ---
  init_macroblock(curMb);

  // intra prediction modes for a macroblock 4x4** ********************************************
  read_ipred_modes(curMb);

  // read CBP and Coeffs ** *************************************************************
  curMb->slice->read_CBP_and_coeffs_from_NAL (curMb);
}
//}}}
//{{{
static void read_intra4x4_macroblock_cavlc (sMacroblock* curMb, const byte *partMap)
{
  sSlice* curSlice = curMb->slice;
  //transform size flag for INTRA_4x4 and INTRA_8x8 modes
  if (curSlice->transform8x8Mode)
  {
    sSyntaxElement currSE;
    sDataPartition *dP = &(curSlice->partArr[partMap[SE_HEADER]]);
    currSE.type   =  SE_HEADER;

    // read CAVLC transform_size_8x8_flag
    currSE.len = (int64) 1;
    readsSyntaxElement_FLC(&currSE, dP->bitstream);

    curMb->luma_transform_size_8x8_flag = (Boolean) currSE.value1;

    if (curMb->luma_transform_size_8x8_flag)
    {
      curMb->mb_type = I8MB;
      memset(&curMb->b8mode, I8MB, 4 * sizeof(char));
      memset(&curMb->b8pdir, -1, 4 * sizeof(char));
    }
  }
  else
  {
    curMb->luma_transform_size_8x8_flag = FALSE;
  }

  //--- init macroblock data ---
  init_macroblock(curMb);

  // intra prediction modes for a macroblock 4x4** ********************************************
  read_ipred_modes(curMb);

  // read CBP and Coeffs ** *************************************************************
  curSlice->read_CBP_and_coeffs_from_NAL (curMb);
}
//}}}
//{{{
static void read_intra4x4_macroblock_cabac (sMacroblock* curMb, const byte *partMap)
{
  sSlice* curSlice = curMb->slice;

  //transform size flag for INTRA_4x4 and INTRA_8x8 modes
  if (curSlice->transform8x8Mode)
  {
   sSyntaxElement currSE;
    sDataPartition *dP = &(curSlice->partArr[partMap[SE_HEADER]]);
    currSE.type   =  SE_HEADER;
    currSE.reading = readMB_transform_size_flag_CABAC;

    // read CAVLC transform_size_8x8_flag
    if (dP->bitstream->ei_flag)
    {
      currSE.len = (int64) 1;
      readsSyntaxElement_FLC(&currSE, dP->bitstream);
    }
    else
    {
      dP->readsSyntaxElement(curMb, &currSE, dP);
    }

    curMb->luma_transform_size_8x8_flag = (Boolean) currSE.value1;

    if (curMb->luma_transform_size_8x8_flag)
    {
      curMb->mb_type = I8MB;
      memset(&curMb->b8mode, I8MB, 4 * sizeof(char));
      memset(&curMb->b8pdir, -1, 4 * sizeof(char));
    }
  }
  else
  {
    curMb->luma_transform_size_8x8_flag = FALSE;
  }

  //--- init macroblock data ---
  init_macroblock(curMb);

  // intra prediction modes for a macroblock 4x4** ********************************************
  read_ipred_modes(curMb);

  // read CBP and Coeffs ** *************************************************************
  curSlice->read_CBP_and_coeffs_from_NAL (curMb);
}
//}}}
//{{{
static void read_inter_macroblock (sMacroblock* curMb)
{
  sSlice* curSlice = curMb->slice;

  //init NoMbPartLessThan8x8Flag
  curMb->NoMbPartLessThan8x8Flag = TRUE;
  curMb->luma_transform_size_8x8_flag = FALSE;

  if(curMb->vidParam->activePPS->constrained_intra_pred_flag)
  {
    int mb_nr = curMb->mbAddrX;
    curSlice->intraBlock[mb_nr] = 0;
  }

  //--- init macroblock data ---
  init_macroblock(curMb);

  // read inter frame vector data** *******************************************************
  curSlice->read_motion_info_from_NAL (curMb);
  // read CBP and Coeffs ** *************************************************************
  curSlice->read_CBP_and_coeffs_from_NAL (curMb);
}
//}}}
//{{{
static void read_i_pcm_macroblock (sMacroblock* curMb, const byte *partMap)
{
  sSlice* curSlice = curMb->slice;
  curMb->NoMbPartLessThan8x8Flag = TRUE;
  curMb->luma_transform_size_8x8_flag = FALSE;

  //--- init macroblock data ---
  init_macroblock(curMb);

  //read pcm_alignment_zero_bit and pcm_byte[i]

  // here dP is assigned with the same dP as SE_MBTYPE, because IPCM syntax is in the
  // same category as MBTYPE
  if ( curSlice->dp_mode && curSlice->dpB_NotPresent )
    concealIPCMcoeffs(curMb);
  else
  {
    sDataPartition *dP = &(curSlice->partArr[partMap[SE_LUM_DC_INTRA]]);
    read_IPCM_coeffs_from_NAL(curSlice, dP);
  }
}
//}}}
//{{{
static void read_P8x8_macroblock (sMacroblock* curMb, sDataPartition *dP, sSyntaxElement* currSE)
{
  int i;
  sSlice* curSlice = curMb->slice;
  //====== READ 8x8 SUB-PARTITION MODES (modes of 8x8 blocks) and Intra VBST block modes ======
  curMb->NoMbPartLessThan8x8Flag = TRUE;
  curMb->luma_transform_size_8x8_flag = FALSE;

  for (i = 0; i < 4; ++i)
  {
    dP->readsSyntaxElement (curMb, currSE, dP);
    SetB8Mode (curMb, currSE->value1, i);

    //set NoMbPartLessThan8x8Flag for P8x8 mode
    curMb->NoMbPartLessThan8x8Flag &= (curMb->b8mode[i] == 0 && curSlice->activeSPS->direct_8x8_inference_flag) ||
      (curMb->b8mode[i] == 4);
  }

  //--- init macroblock data ---
  init_macroblock (curMb);
  curSlice->read_motion_info_from_NAL (curMb);

  if(curMb->vidParam->activePPS->constrained_intra_pred_flag)
  {
    int mb_nr = curMb->mbAddrX;
    curSlice->intraBlock[mb_nr] = 0;
  }

  // read CBP and Coeffs ** *************************************************************
  curSlice->read_CBP_and_coeffs_from_NAL (curMb);
}
//}}}
//{{{
static void read_one_macroblock_i_slice_cavlc (sMacroblock* curMb)
{
  sSlice* curSlice = curMb->slice;

  sSyntaxElement currSE;
  int mb_nr = curMb->mbAddrX;

  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  sPicture* picture = curSlice->picture;
  sPicMotionParamsOld *motion = &picture->motion;

  curMb->mb_field = ((mb_nr&0x01) == 0)? FALSE : curSlice->mbData[mb_nr-1].mb_field;

  update_qp(curMb, curSlice->qp);
  currSE.type = SE_MBTYPE;

  //  read MB mode** ***************************************************************
  dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

  currSE.mapping = linfo_ue;

  // read MB aff
  if (curSlice->mb_aff_frame_flag && (mb_nr&0x01)==0)
  {
    currSE.len = (int64) 1;
    readsSyntaxElement_FLC(&currSE, dP->bitstream);
    curMb->mb_field = (Boolean) currSE.value1;
  }

  //  read MB type
  dP->readsSyntaxElement(curMb, &currSE, dP);

  curMb->mb_type = (short) currSE.value1;
  if(!dP->bitstream->ei_flag)
    curMb->ei_flag = 0;

  motion->mb_field[mb_nr] = (byte) curMb->mb_field;
  curMb->block_y_aff = ((curSlice->mb_aff_frame_flag) && (curMb->mb_field)) ? (mb_nr&0x01) ? (curMb->block_y - 4)>>1 : curMb->block_y >> 1 : curMb->block_y;
  curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
  curSlice->interpret_mb_mode(curMb);

  //init NoMbPartLessThan8x8Flag
  curMb->NoMbPartLessThan8x8Flag = TRUE;
  if(curMb->mb_type == IPCM)
    read_i_pcm_macroblock(curMb, partMap);
  else if (curMb->mb_type == I4MB)
    read_intra4x4_macroblock_cavlc(curMb, partMap);
  else // all other modes
    read_intra_macroblock(curMb);
  return;
}
//}}}
//{{{
static void read_one_macroblock_i_slice_cabac (sMacroblock* curMb)
{
  sSlice* curSlice = curMb->slice;

  sSyntaxElement currSE;
  int mb_nr = curMb->mbAddrX;

  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];
  sPicture* picture = curSlice->picture;
  sPicMotionParamsOld *motion = &picture->motion;

  curMb->mb_field = ((mb_nr&0x01) == 0)? FALSE : curSlice->mbData[mb_nr-1].mb_field;

  update_qp(curMb, curSlice->qp);
  currSE.type = SE_MBTYPE;

  //  read MB mode** ***************************************************************
  dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

  if (dP->bitstream->ei_flag)
    currSE.mapping = linfo_ue;

  // read MB aff
  if (curSlice->mb_aff_frame_flag && (mb_nr&0x01)==0)
  {
    if (dP->bitstream->ei_flag)
    {
      currSE.len = (int64) 1;
      readsSyntaxElement_FLC(&currSE, dP->bitstream);
    }
    else
    {
      currSE.reading = readFieldModeInfo_CABAC;
      dP->readsSyntaxElement(curMb, &currSE, dP);
    }
    curMb->mb_field = (Boolean) currSE.value1;
  }

  CheckAvailabilityOfNeighborsCABAC(curMb);

  //  read MB type
  currSE.reading = readMB_typeInfo_CABAC_i_slice;
  dP->readsSyntaxElement(curMb, &currSE, dP);

  curMb->mb_type = (short) currSE.value1;
  if(!dP->bitstream->ei_flag)
    curMb->ei_flag = 0;

  motion->mb_field[mb_nr] = (byte) curMb->mb_field;
  curMb->block_y_aff = ((curSlice->mb_aff_frame_flag) && (curMb->mb_field)) ? (mb_nr&0x01) ? (curMb->block_y - 4)>>1 : curMb->block_y >> 1 : curMb->block_y;
  curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
  curSlice->interpret_mb_mode(curMb);

  //init NoMbPartLessThan8x8Flag
  curMb->NoMbPartLessThan8x8Flag = TRUE;
  if(curMb->mb_type == IPCM)
    read_i_pcm_macroblock(curMb, partMap);
  else if (curMb->mb_type == I4MB)
  {
    //transform size flag for INTRA_4x4 and INTRA_8x8 modes
    if (curSlice->transform8x8Mode)
    {
      currSE.type   =  SE_HEADER;
      dP = &(curSlice->partArr[partMap[SE_HEADER]]);
      currSE.reading = readMB_transform_size_flag_CABAC;

      // read CAVLC transform_size_8x8_flag
      if (dP->bitstream->ei_flag)
      {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
      }
      else
        dP->readsSyntaxElement(curMb, &currSE, dP);

      curMb->luma_transform_size_8x8_flag = (Boolean) currSE.value1;

      if (curMb->luma_transform_size_8x8_flag)
      {
        curMb->mb_type = I8MB;
        memset(&curMb->b8mode, I8MB, 4 * sizeof(char));
        memset(&curMb->b8pdir, -1, 4 * sizeof(char));
      }
    }
    else
      curMb->luma_transform_size_8x8_flag = FALSE;

    //--- init macroblock data ---
    init_macroblock(curMb);

    // intra prediction modes for a macroblock 4x4** ********************************************
    read_ipred_modes(curMb);

    // read CBP and Coeffs ** *************************************************************
    curSlice->read_CBP_and_coeffs_from_NAL (curMb);
  }
  else // all other modes
    read_intra_macroblock(curMb);
  return;
}
//}}}
//{{{
static void read_one_macroblock_p_slice_cavlc (sMacroblock* curMb)
{
  sSlice* curSlice = curMb->slice;
  sSyntaxElement currSE;
  int mb_nr = curMb->mbAddrX;

  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];

  if (curSlice->mb_aff_frame_flag == 0)
  {
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    curMb->mb_field = FALSE;

    update_qp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode** ***************************************************************
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.mapping = linfo_ue;

    // VLC Non-Intra
    if(curSlice->cod_counter == -1)
    {
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curSlice->cod_counter = currSE.value1;
    }

    if (curSlice->cod_counter==0)
    {
      // read MB type
      dP->readsSyntaxElement(curMb, &currSE, dP);
      ++(currSE.value1);
      curMb->mb_type = (short) currSE.value1;
      if(!dP->bitstream->ei_flag)
        curMb->ei_flag = 0;
      curSlice->cod_counter--;
      curMb->skip_flag = 0;
    }
    else
    {
      curSlice->cod_counter--;
      curMb->mb_type = 0;
      curMb->ei_flag = 0;
      curMb->skip_flag = 1;
    }
    //update the list offset;
    curMb->list_offset = 0;
    motion->mb_field[mb_nr] = (byte) FALSE;
    curMb->block_y_aff = curMb->block_y;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpret_mb_mode(curMb);
  }
  else
  {
    sVidParam *vidParam = curMb->vidParam;
    sMacroblock *topMB = NULL;
    int  prevMbSkipped = 0;
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    if (mb_nr&0x01)
    {
      topMB= &vidParam->mbData[mb_nr-1];
      prevMbSkipped = (topMB->mb_type == 0);
    }
    else
      prevMbSkipped = 0;

    curMb->mb_field = ((mb_nr&0x01) == 0)? FALSE : vidParam->mbData[mb_nr-1].mb_field;

    update_qp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode** ***************************************************************
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

    currSE.mapping = linfo_ue;

    // VLC Non-Intra
    if(curSlice->cod_counter == -1)
    {
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curSlice->cod_counter = currSE.value1;
    }

    if (curSlice->cod_counter==0)
    {
      // read MB aff
      if ((((mb_nr&0x01)==0) || ((mb_nr&0x01) && prevMbSkipped)))
      {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
        curMb->mb_field = (Boolean) currSE.value1;
      }

      // read MB type
      dP->readsSyntaxElement(curMb, &currSE, dP);
      ++(currSE.value1);
      curMb->mb_type = (short) currSE.value1;
      if(!dP->bitstream->ei_flag)
        curMb->ei_flag = 0;
      curSlice->cod_counter--;
      curMb->skip_flag = 0;
    }
    else
    {
      curSlice->cod_counter--;
      curMb->mb_type = 0;
      curMb->ei_flag = 0;
      curMb->skip_flag = 1;

      // read field flag of bottom block
      if(curSlice->cod_counter == 0 && ((mb_nr&0x01) == 0))
      {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
        dP->bitstream->frame_bitoffset--;
        curMb->mb_field = (Boolean) currSE.value1;
      }
      else if (curSlice->cod_counter > 0 && ((mb_nr & 0x01) == 0))
      {
        // check left macroblock pair first
        if (mb_is_available(mb_nr - 2, curMb) && ((mb_nr % (vidParam->PicWidthInMbs * 2))!=0))
        {
          curMb->mb_field = vidParam->mbData[mb_nr-2].mb_field;
        }
        else
        {
          // check top macroblock pair
          if (mb_is_available(mb_nr - 2*vidParam->PicWidthInMbs, curMb))
          {
            curMb->mb_field = vidParam->mbData[mb_nr-2*vidParam->PicWidthInMbs].mb_field;
          }
          else
            curMb->mb_field = FALSE;
        }
      }
    }
    //update the list offset;
    curMb->list_offset = (curMb->mb_field)? ((mb_nr&0x01)? 4: 2): 0;

    motion->mb_field[mb_nr] = (byte) curMb->mb_field;

    curMb->block_y_aff = (curMb->mb_field) ? (mb_nr&0x01) ? (curMb->block_y - 4)>>1 : curMb->block_y >> 1 : curMb->block_y;

    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;

    curSlice->interpret_mb_mode(curMb);

    if(curMb->mb_field)
    {
      curSlice->num_ref_idx_active[LIST_0] <<=1;
      curSlice->num_ref_idx_active[LIST_1] <<=1;
    }
  }
    //init NoMbPartLessThan8x8Flag
    curMb->NoMbPartLessThan8x8Flag = TRUE;

    if (curMb->mb_type == IPCM) // I_PCM mode
    {
      read_i_pcm_macroblock(curMb, partMap);
    }
    else if (curMb->mb_type == I4MB)
    {
      read_intra4x4_macroblock_cavlc(curMb, partMap);
    }
    else if (curMb->mb_type == P8x8)
    {
      currSE.type = SE_MBTYPE;
      currSE.mapping = linfo_ue;
      dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

      read_P8x8_macroblock(curMb, dP, &currSE);
    }
    else if (curMb->mb_type == PSKIP)
    {
      read_skip_macroblock(curMb);
    }
    else if (curMb->is_intra_block) // all other intra modes
    {
      read_intra_macroblock(curMb);
    }
    else // all other remaining modes
    {
      read_inter_macroblock(curMb);
    }


  return;
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
  const byte *partMap = assignSE2partition[curSlice->dp_mode];

  if (curSlice->mb_aff_frame_flag == 0)
  {
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    curMb->mb_field = FALSE;

    update_qp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode** ***************************************************************
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

    if (dP->bitstream->ei_flag)
      currSE.mapping = linfo_ue;

    CheckAvailabilityOfNeighborsCABAC(curMb);
    currSE.reading = read_skip_flag_CABAC_p_slice;
    dP->readsSyntaxElement(curMb, &currSE, dP);

    curMb->mb_type   = (short) currSE.value1;
    curMb->skip_flag = (char) (!(currSE.value1));
    if (!dP->bitstream->ei_flag)
      curMb->ei_flag = 0;

    // read MB type
    if (curMb->mb_type != 0 )
    {
      currSE.reading = readMB_typeInfo_CABAC_p_slice;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mb_type = (short) currSE.value1;
      if(!dP->bitstream->ei_flag)
        curMb->ei_flag = 0;
    }

    motion->mb_field[mb_nr] = (byte) FALSE;
    curMb->block_y_aff = curMb->block_y;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpret_mb_mode(curMb);
  }
  else
  {
    sMacroblock *topMB = NULL;
    int  prevMbSkipped = 0;
    int  check_bottom, read_bottom, read_top;
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;
    if (mb_nr&0x01)
    {
      topMB= &vidParam->mbData[mb_nr-1];
      prevMbSkipped = (topMB->mb_type == 0);
    }
    else
      prevMbSkipped = 0;

    curMb->mb_field = ((mb_nr&0x01) == 0)? FALSE : vidParam->mbData[mb_nr-1].mb_field;

    update_qp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode** ***************************************************************
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

    if (dP->bitstream->ei_flag)
      currSE.mapping = linfo_ue;

    // read MB skip_flag
    if (((mb_nr&0x01) == 0||prevMbSkipped))
      field_flag_inference(curMb);

    CheckAvailabilityOfNeighborsCABAC(curMb);
    currSE.reading = read_skip_flag_CABAC_p_slice;
    dP->readsSyntaxElement(curMb, &currSE, dP);

    curMb->mb_type   = (short) currSE.value1;
    curMb->skip_flag = (char) (!(currSE.value1));

    if (!dP->bitstream->ei_flag)
      curMb->ei_flag = 0;

    // read MB AFF
    check_bottom=read_bottom=read_top=0;
    if ((mb_nr&0x01)==0)
    {
      check_bottom =  curMb->skip_flag;
      read_top = !check_bottom;
    }
    else
      read_bottom = (topMB->skip_flag && (!curMb->skip_flag));

    if (read_bottom || read_top)
    {
      currSE.reading = readFieldModeInfo_CABAC;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mb_field = (Boolean) currSE.value1;
    }

    if (check_bottom)
      check_next_mb_and_get_field_mode_CABAC_p_slice(curSlice, &currSE, dP);

    //update the list offset;
    curMb->list_offset = (curMb->mb_field)? ((mb_nr&0x01)? 4: 2): 0;

    //if (curMb->mb_type != 0 )
    CheckAvailabilityOfNeighborsCABAC(curMb);

    // read MB type
    if (curMb->mb_type != 0 )
    {
      currSE.reading = readMB_typeInfo_CABAC_p_slice;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mb_type = (short) currSE.value1;
      if(!dP->bitstream->ei_flag)
        curMb->ei_flag = 0;
    }

    motion->mb_field[mb_nr] = (byte) curMb->mb_field;
    curMb->block_y_aff = (curMb->mb_field) ? (mb_nr&0x01) ? (curMb->block_y - 4)>>1 : curMb->block_y >> 1 : curMb->block_y;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpret_mb_mode(curMb);
    if(curMb->mb_field)
    {
      curSlice->num_ref_idx_active[LIST_0] <<=1;
      curSlice->num_ref_idx_active[LIST_1] <<=1;
    }
  }
  //init NoMbPartLessThan8x8Flag
  curMb->NoMbPartLessThan8x8Flag = TRUE;

  if (curMb->mb_type == IPCM) // I_PCM mode
    read_i_pcm_macroblock(curMb, partMap);
  else if (curMb->mb_type == I4MB)
    read_intra4x4_macroblock_cabac(curMb, partMap);
  else if (curMb->mb_type == P8x8)
  {
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.type = SE_MBTYPE;

    if (dP->bitstream->ei_flag)
      currSE.mapping = linfo_ue;
    else
      currSE.reading = readB8_typeInfo_CABAC_p_slice;

    read_P8x8_macroblock(curMb, dP, &currSE);
  }
  else if (curMb->mb_type == PSKIP)
    read_skip_macroblock (curMb);
  else if (curMb->is_intra_block == TRUE) // all other intra modes
    read_intra_macroblock(curMb);
  else // all other remaining modes
    read_inter_macroblock(curMb);

  return;
}
//}}}
//{{{
static void read_one_macroblock_b_slice_cavlc (sMacroblock* curMb) {

  sVidParam *vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;
  int mb_nr = curMb->mbAddrX;
  sDataPartition *dP;
  sSyntaxElement currSE;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];

  if (curSlice->mb_aff_frame_flag == 0) {
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    curMb->mb_field = FALSE;
    update_qp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode** ***************************************************************
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.mapping = linfo_ue;

    if(curSlice->cod_counter == -1) {
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curSlice->cod_counter = currSE.value1;
      }
    if (curSlice->cod_counter==0) {
      // read MB type
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mb_type = (short) currSE.value1;
      if(!dP->bitstream->ei_flag)
        curMb->ei_flag = 0;
      curSlice->cod_counter--;
      curMb->skip_flag = 0;
      }
    else {
      curSlice->cod_counter--;
      curMb->mb_type = 0;
      curMb->ei_flag = 0;
      curMb->skip_flag = 1;
      }

    //update the list offset;
    curMb->list_offset = 0;
    motion->mb_field[mb_nr] = FALSE;
    curMb->block_y_aff = curMb->block_y;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpret_mb_mode(curMb);
    }
  else {
    sMacroblock *topMB = NULL;
    int  prevMbSkipped = 0;
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    if (mb_nr&0x01) {
      topMB= &vidParam->mbData[mb_nr-1];
      prevMbSkipped = topMB->skip_flag;
      }
    else
      prevMbSkipped = 0;

    curMb->mb_field = ((mb_nr&0x01) == 0)? FALSE : vidParam->mbData[mb_nr-1].mb_field;

    update_qp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode** ***************************************************************
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.mapping = linfo_ue;
    if(curSlice->cod_counter == -1) {
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curSlice->cod_counter = currSE.value1;
      }
    if (curSlice->cod_counter==0) {
      // read MB aff
      if ((((mb_nr&0x01)==0) || ((mb_nr&0x01) && prevMbSkipped))) {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
        curMb->mb_field = (Boolean) currSE.value1;
        }

      // read MB type
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mb_type = (short) currSE.value1;
      if(!dP->bitstream->ei_flag)
        curMb->ei_flag = 0;
      curSlice->cod_counter--;
      curMb->skip_flag = 0;
      }
    else {
      curSlice->cod_counter--;
      curMb->mb_type = 0;
      curMb->ei_flag = 0;
      curMb->skip_flag = 1;

      // read field flag of bottom block
      if(curSlice->cod_counter == 0 && ((mb_nr&0x01) == 0)) {
        currSE.len = (int64) 1;
        readsSyntaxElement_FLC(&currSE, dP->bitstream);
        dP->bitstream->frame_bitoffset--;
        curMb->mb_field = (Boolean) currSE.value1;
        }
      else if (curSlice->cod_counter > 0 && ((mb_nr & 0x01) == 0)) {
        // check left macroblock pair first
        if (mb_is_available(mb_nr - 2, curMb) && ((mb_nr % (vidParam->PicWidthInMbs * 2))!=0))
          curMb->mb_field = vidParam->mbData[mb_nr-2].mb_field;
        else {
          // check top macroblock pair
          if (mb_is_available(mb_nr - 2*vidParam->PicWidthInMbs, curMb))
            curMb->mb_field = vidParam->mbData[mb_nr-2*vidParam->PicWidthInMbs].mb_field;
          else
            curMb->mb_field = FALSE;
          }
        }
      }

    //update the list offset;
    curMb->list_offset = (curMb->mb_field)? ((mb_nr&0x01)? 4: 2): 0;
    motion->mb_field[mb_nr] = (byte) curMb->mb_field;
    curMb->block_y_aff = (curMb->mb_field) ? (mb_nr&0x01) ? (curMb->block_y - 4)>>1 : curMb->block_y >> 1 : curMb->block_y;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpret_mb_mode(curMb);
    if(curSlice->mb_aff_frame_flag) {
      if(curMb->mb_field) {
        curSlice->num_ref_idx_active[LIST_0] <<=1;
        curSlice->num_ref_idx_active[LIST_1] <<=1;
        }
      }
    }

  if (curMb->mb_type == IPCM)
    read_i_pcm_macroblock(curMb, partMap);
  else if (curMb->mb_type == I4MB)
    read_intra4x4_macroblock_cavlc(curMb, partMap);
  else if (curMb->mb_type == P8x8) {
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.type = SE_MBTYPE;
    currSE.mapping = linfo_ue;
    read_P8x8_macroblock(curMb, dP, &currSE);
    }
  else if (curMb->mb_type == BSKIP_DIRECT) {
    //init NoMbPartLessThan8x8Flag
    curMb->NoMbPartLessThan8x8Flag = (!(curSlice->activeSPS->direct_8x8_inference_flag))? FALSE: TRUE;
    curMb->luma_transform_size_8x8_flag = FALSE;
    if(vidParam->activePPS->constrained_intra_pred_flag)
      curSlice->intraBlock[mb_nr] = 0;

    //--- init macroblock data ---
    init_macroblock_direct(curMb);

    if (curSlice->cod_counter >= 0) {
      curMb->cbp = 0;
      reset_coeffs(curMb);
      }
    else
      // read CBP and Coeffs ** *************************************************************
      curSlice->read_CBP_and_coeffs_from_NAL (curMb);
    }
  else if (curMb->is_intra_block == TRUE) // all other intra modes
    read_intra_macroblock(curMb);
  else // all other remaining modes
    read_inter_macroblock(curMb);

  return;
  }
//}}}
//{{{
static void read_one_macroblock_b_slice_cabac (sMacroblock* curMb)
{
  sSlice* curSlice = curMb->slice;
  sVidParam *vidParam = curMb->vidParam;
  int mb_nr = curMb->mbAddrX;
  sSyntaxElement currSE;

  sDataPartition *dP;
  const byte *partMap = assignSE2partition[curSlice->dp_mode];

  if (curSlice->mb_aff_frame_flag == 0)
  {
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    curMb->mb_field = FALSE;

    update_qp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode** ***************************************************************
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

    if (dP->bitstream->ei_flag)
      currSE.mapping = linfo_ue;

    CheckAvailabilityOfNeighborsCABAC(curMb);
    currSE.reading = read_skip_flag_CABAC_b_slice;
    dP->readsSyntaxElement(curMb, &currSE, dP);

    curMb->mb_type   = (short) currSE.value1;
    curMb->skip_flag = (char) (!(currSE.value1));

    curMb->cbp = currSE.value2;

    if (!dP->bitstream->ei_flag)
      curMb->ei_flag = 0;

    if (currSE.value1 == 0 && currSE.value2 == 0)
      curSlice->cod_counter=0;

    // read MB type
    if (curMb->mb_type != 0 )
    {
      currSE.reading = readMB_typeInfo_CABAC_b_slice;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mb_type = (short) currSE.value1;
      if(!dP->bitstream->ei_flag)
        curMb->ei_flag = 0;
    }

    motion->mb_field[mb_nr] = (byte) FALSE;
    curMb->block_y_aff = curMb->block_y;
    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;
    curSlice->interpret_mb_mode(curMb);
  }
  else
  {
    sMacroblock *topMB = NULL;
    int  prevMbSkipped = 0;
    int  check_bottom, read_bottom, read_top;
    sPicture* picture = curSlice->picture;
    sPicMotionParamsOld *motion = &picture->motion;

    if (mb_nr&0x01)
    {
      topMB= &vidParam->mbData[mb_nr-1];
      prevMbSkipped = topMB->skip_flag;
    }
    else
      prevMbSkipped = 0;

    curMb->mb_field = ((mb_nr&0x01) == 0)? FALSE : vidParam->mbData[mb_nr-1].mb_field;

    update_qp(curMb, curSlice->qp);
    currSE.type = SE_MBTYPE;

    //  read MB mode** ***************************************************************
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);

    if (dP->bitstream->ei_flag)
      currSE.mapping = linfo_ue;

    // read MB skip_flag
    if (((mb_nr&0x01) == 0||prevMbSkipped))
      field_flag_inference(curMb);

    CheckAvailabilityOfNeighborsCABAC(curMb);
    currSE.reading = read_skip_flag_CABAC_b_slice;
    dP->readsSyntaxElement(curMb, &currSE, dP);

    curMb->mb_type   = (short) currSE.value1;
    curMb->skip_flag = (char) (!(currSE.value1));

    curMb->cbp = currSE.value2;

    if (!dP->bitstream->ei_flag)
      curMb->ei_flag = 0;

    if (currSE.value1 == 0 && currSE.value2 == 0)
      curSlice->cod_counter=0;

    // read MB AFF
    check_bottom=read_bottom=read_top=0;
    if ((mb_nr & 0x01) == 0)
    {
      check_bottom =  curMb->skip_flag;
      read_top = !check_bottom;
    }
    else
    {
      read_bottom = (topMB->skip_flag && (!curMb->skip_flag));
    }

    if (read_bottom || read_top)
    {
      currSE.reading = readFieldModeInfo_CABAC;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mb_field = (Boolean) currSE.value1;
    }
    if (check_bottom)
      check_next_mb_and_get_field_mode_CABAC_b_slice(curSlice, &currSE, dP);

    //update the list offset;
    curMb->list_offset = (curMb->mb_field)? ((mb_nr&0x01)? 4: 2): 0;
    //if (curMb->mb_type != 0 )
    CheckAvailabilityOfNeighborsCABAC(curMb);

    // read MB type
    if (curMb->mb_type != 0 )
    {
      currSE.reading = readMB_typeInfo_CABAC_b_slice;
      dP->readsSyntaxElement(curMb, &currSE, dP);
      curMb->mb_type = (short) currSE.value1;
      if(!dP->bitstream->ei_flag)
        curMb->ei_flag = 0;
    }


    motion->mb_field[mb_nr] = (byte) curMb->mb_field;

    curMb->block_y_aff = (curMb->mb_field) ? (mb_nr&0x01) ? (curMb->block_y - 4)>>1 : curMb->block_y >> 1 : curMb->block_y;

    curSlice->siBlock[curMb->mb.y][curMb->mb.x] = 0;

    curSlice->interpret_mb_mode(curMb);

    if(curMb->mb_field)
    {
      curSlice->num_ref_idx_active[LIST_0] <<=1;
      curSlice->num_ref_idx_active[LIST_1] <<=1;
    }
  }

  if(curMb->mb_type == IPCM)
  {
    read_i_pcm_macroblock(curMb, partMap);
  }
  else if (curMb->mb_type == I4MB)
  {
    read_intra4x4_macroblock_cabac(curMb, partMap);
  }
  else if (curMb->mb_type == P8x8)
  {
    dP = &(curSlice->partArr[partMap[SE_MBTYPE]]);
    currSE.type = SE_MBTYPE;

    if (dP->bitstream->ei_flag)
      currSE.mapping = linfo_ue;
    else
      currSE.reading = readB8_typeInfo_CABAC_b_slice;

    read_P8x8_macroblock(curMb, dP, &currSE);
  }
  else if (curMb->mb_type == BSKIP_DIRECT)
  {
    //init NoMbPartLessThan8x8Flag
    curMb->NoMbPartLessThan8x8Flag = (!(curSlice->activeSPS->direct_8x8_inference_flag))? FALSE: TRUE;

    //============= Transform Size Flag for INTRA MBs =============
    //-------------------------------------------------------------
    //transform size flag for INTRA_4x4 and INTRA_8x8 modes
    curMb->luma_transform_size_8x8_flag = FALSE;

    if(vidParam->activePPS->constrained_intra_pred_flag)
    {
      curSlice->intraBlock[mb_nr] = 0;
    }

    //--- init macroblock data ---
    init_macroblock_direct(curMb);

    if (curSlice->cod_counter >= 0)
    {
      curSlice->is_reset_coeff = TRUE;
      curMb->cbp = 0;
      curSlice->cod_counter = -1;
    }
    else
    {
      // read CBP and Coeffs ** *************************************************************
      curSlice->read_CBP_and_coeffs_from_NAL (curMb);
    }
  }
  else if (curMb->is_intra_block == TRUE) // all other intra modes
  {
    read_intra_macroblock(curMb);
  }
  else // all other remaining modes
  {
    read_inter_macroblock(curMb);
  }

  return;
}
//}}}
//{{{
void setup_read_macroblock (sSlice* curSlice) {

  if (curSlice->vidParam->activePPS->entropy_coding_mode_flag == (Boolean) CABAC) {
    switch (curSlice->slice_type) {
      case P_SLICE:
      case SP_SLICE:
        curSlice->read_one_macroblock = read_one_macroblock_p_slice_cabac;
        break;
      case B_SLICE:
        curSlice->read_one_macroblock = read_one_macroblock_b_slice_cabac;
        break;
      case I_SLICE:
      case SI_SLICE:
        curSlice->read_one_macroblock = read_one_macroblock_i_slice_cabac;
        break;
      default:
        printf("Unsupported slice type\n");
        break;
      }
    }

  else {
    switch (curSlice->slice_type) {
      case P_SLICE:
      case SP_SLICE:
        curSlice->read_one_macroblock = read_one_macroblock_p_slice_cavlc;
        break;
      case B_SLICE:
        curSlice->read_one_macroblock = read_one_macroblock_b_slice_cavlc;
        break;
      case I_SLICE:
      case SI_SLICE:
        curSlice->read_one_macroblock = read_one_macroblock_i_slice_cavlc;
        break;
      default:
        printf("Unsupported slice type\n");
        break;
      }
    }
  }
//}}}
