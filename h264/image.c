//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
#include "fmo.h"
#include "nalu.h"
#include "parset.h"
#include "sliceHeader.h"
#include "sei.h"
#include "output.h"
#include "mbAccess.h"
#include "macroblock.h"
#include "loopfilter.h"
#include "biariDecode.h"
#include "cabac.h"
#include "vlc.h"
#include "quant.h"
#include "errorConceal.h"
#include "erc.h"
#include "buffer.h"
#include "mcPred.h"

#include <math.h>
#include <limits.h>
//}}}

static sNalu* pendingNalu = NULL;
//{{{
static void resetMb (sMacroblock* mb) {

  mb->sliceNum = -1;
  mb->eiFlag =  1;
  mb->dplFlag =  0;
  }
//}}}
//{{{
static void setupBuffers (sVidParam* vidParam, int layerId) {

  if (vidParam->last_dec_layer_id != layerId) {
    sCodingParam* codingParam = vidParam->codingParam[layerId];
    if (codingParam->sepColourPlaneFlag) {
      for (int i = 0; i < MAX_PLANE; i++ ) {
        vidParam->mbDataJV[i] = codingParam->mbDataJV[i];
        vidParam->intraBlockJV[i] = codingParam->intraBlockJV[i];
        vidParam->predModeJV[i] = codingParam->predModeJV[i];
        vidParam->siBlockJV[i] = codingParam->siBlockJV[i];
        }
      vidParam->mbData = NULL;
      vidParam->intraBlock = NULL;
      vidParam->predMode = NULL;
      vidParam->siBlock = NULL;
      }
    else {
      vidParam->mbData = codingParam->mbData;
      vidParam->intraBlock = codingParam->intraBlock;
      vidParam->predMode = codingParam->predMode;
      vidParam->siBlock = codingParam->siBlock;
      }

    vidParam->picPos = codingParam->picPos;
    vidParam->nzCoeff = codingParam->nzCoeff;
    vidParam->qpPerMatrix = codingParam->qpPerMatrix;
    vidParam->qpRemMatrix = codingParam->qpRemMatrix;
    vidParam->oldFrameSizeInMbs = codingParam->oldFrameSizeInMbs;
    vidParam->last_dec_layer_id = layerId;
    }
  }
//}}}
//{{{
static void padBuf (sPixel* pixel, int width, int height, int stride, int padx, int pady) {

  int pad_width = padx + width;
  memset (pixel - padx, *pixel, padx * sizeof(sPixel));
  memset (pixel + width, *(pixel + width - 1), padx * sizeof(sPixel));

  sPixel* line0 = pixel - padx;
  sPixel* line = line0 - pady * stride;
  for (int j = -pady; j < 0; j++) {
    memcpy (line, line0, stride * sizeof(sPixel));
    line += stride;
    }

  for (int j = 1; j < height; j++) {
    line += stride;
    memset (line, *(line + padx), padx * sizeof(sPixel));
    memset (line + pad_width, *(line + pad_width - 1), padx * sizeof(sPixel));
    }

  line0 = line + stride;
  for (int j = height; j < height + pady; j++) {
    memcpy (line0,  line, stride * sizeof(sPixel));
    line0 += stride;
    }
  }
//}}}
//{{{
static void copyPOC (sSlice* fromSlice, sSlice* slice) {

  slice->framePoc = fromSlice->framePoc;
  slice->topPoc = fromSlice->topPoc;
  slice->botPoc = fromSlice->botPoc;
  slice->thisPoc = fromSlice->thisPoc;
  }
//}}}

//{{{
static void updateMbAff (sPixel** curPixel, sPixel (*temp)[16], int x0, int width, int height) {

  sPixel (*temp_evn)[16] = temp;
  sPixel (*temp_odd)[16] = temp + height;
  sPixel** temp_img = curPixel;

  for (int y = 0; y < 2 * height; ++y)
    memcpy (*temp++, (*temp_img++ + x0), width * sizeof(sPixel));

  for (int y = 0; y < height; ++y) {
    memcpy ((*curPixel++ + x0), *temp_evn++, width * sizeof(sPixel));
    memcpy ((*curPixel++ + x0), *temp_odd++, width * sizeof(sPixel));
    }
  }
//}}}
//{{{
static void mbAffPostProc (sVidParam* vidParam) {

  sPixel tempBuffer[32][16];
  sPicture* picture = vidParam->picture;
  sPixel** imgY = picture->imgY;
  sPixel*** imgUV = picture->imgUV;

  short x0;
  short y0;
  for (short i = 0; i < (int)picture->picSizeInMbs; i += 2) {
    if (picture->motion.mbField[i]) {
      get_mb_pos (vidParam, i, vidParam->mbSize[IS_LUMA], &x0, &y0);
      updateMbAff (imgY + y0, tempBuffer, x0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

      if (picture->chromaFormatIdc != YUV400) {
        x0 = (short)((x0 * vidParam->mbCrSizeX) >> 4);
        y0 = (short)((y0 * vidParam->mbCrSizeY) >> 4);
        updateMbAff (imgUV[0] + y0, tempBuffer, x0, vidParam->mbCrSizeX, vidParam->mbCrSizeY);
        updateMbAff (imgUV[1] + y0, tempBuffer, x0, vidParam->mbCrSizeX, vidParam->mbCrSizeY);
        }
      }
    }
  }
//}}}
//{{{
static void fillWPParam (sSlice* slice) {

  if (slice->sliceType == B_SLICE) {
    int maxL0Ref = slice->numRefIndexActive[LIST_0];
    int maxL1Ref = slice->numRefIndexActive[LIST_1];
    if (slice->activePPS->weightedBiPredIdc == 2) {
      slice->lumaLog2weightDenom = 5;
      slice->chromaLog2weightDenom = 5;
      slice->wp_round_luma = 16;
      slice->wp_round_chroma = 16;
      for (int i = 0; i < MAX_REFERENCE_PICTURES; ++i)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = (comp == 0) ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
          slice->wpWeight[0][i][comp] = 1 << logWeightDenom;
          slice->wpWeight[1][i][comp] = 1 << logWeightDenom;
          slice->wpOffset[0][i][comp] = 0;
          slice->wpOffset[1][i][comp] = 0;
          }
      }

    for (int i = 0; i < maxL0Ref; ++i)
      for (int j = 0; j < maxL1Ref; ++j)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = (comp == 0) ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
          if (slice->activePPS->weightedBiPredIdc == 1) {
            slice->wbpWeight[0][i][j][comp] = slice->wpWeight[0][i][comp];
            slice->wbpWeight[1][i][j][comp] = slice->wpWeight[1][j][comp];
            }
          else if (slice->activePPS->weightedBiPredIdc == 2) {
            int td = iClip3(-128,127,slice->listX[LIST_1][j]->poc - slice->listX[LIST_0][i]->poc);
            if (td == 0 ||
                slice->listX[LIST_1][j]->isLongTerm || slice->listX[LIST_0][i]->isLongTerm) {
              slice->wbpWeight[0][i][j][comp] = 32;
              slice->wbpWeight[1][i][j][comp] = 32;
              }
            else {
              int tb = iClip3(-128, 127, slice->thisPoc - slice->listX[LIST_0][i]->poc);
              int tx = (16384 + iabs (td / 2)) / td;
              int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
              slice->wbpWeight[1][i][j][comp] = distScaleFactor >> 2;
              slice->wbpWeight[0][i][j][comp] = 64 - slice->wbpWeight[1][i][j][comp];
              if (slice->wbpWeight[1][i][j][comp] < -64 || slice->wbpWeight[1][i][j][comp] > 128) {
                slice->wbpWeight[0][i][j][comp] = 32;
                slice->wbpWeight[1][i][j][comp] = 32;
                slice->wpOffset[0][i][comp] = 0;
                slice->wpOffset[1][j][comp] = 0;
                }
              }
            }
          }

    if (slice->mbAffFrameFlag)
      for (int i = 0; i < 2 * maxL0Ref; ++i)
        for (int j = 0; j < 2 * maxL1Ref; ++j)
          for (int comp = 0; comp<3; ++comp)
            for (int k = 2; k < 6; k += 2) {
              slice->wpOffset[k+0][i][comp] = slice->wpOffset[0][i>>1][comp];
              slice->wpOffset[k+1][j][comp] = slice->wpOffset[1][j>>1][comp];
              int logWeightDenom = (comp == 0) ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
              if (slice->activePPS->weightedBiPredIdc == 1) {
                slice->wbpWeight[k+0][i][j][comp] = slice->wpWeight[0][i>>1][comp];
                slice->wbpWeight[k+1][i][j][comp] = slice->wpWeight[1][j>>1][comp];
                }
              else if (slice->activePPS->weightedBiPredIdc == 2) {
                int td = iClip3 (-128, 127, slice->listX[k+LIST_1][j]->poc - slice->listX[k+LIST_0][i]->poc);
                if (td == 0 ||
                    slice->listX[k+LIST_1][j]->isLongTerm ||
                    slice->listX[k+LIST_0][i]->isLongTerm) {
                  slice->wbpWeight[k+0][i][j][comp] = 32;
                  slice->wbpWeight[k+1][i][j][comp] = 32;
                  }
                else {
                  int tb = iClip3(-128,127, ((k == 2) ? slice->topPoc :
                                                    slice->botPoc) - slice->listX[k+LIST_0][i]->poc);
                  int tx = (16384 + iabs(td/2)) / td;
                  int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
                  slice->wbpWeight[k+1][i][j][comp] = distScaleFactor >> 2;
                  slice->wbpWeight[k+0][i][j][comp] = 64 - slice->wbpWeight[k+1][i][j][comp];
                  if (slice->wbpWeight[k+1][i][j][comp] < -64 ||
                      slice->wbpWeight[k+1][i][j][comp] > 128) {
                    slice->wbpWeight[k+1][i][j][comp] = 32;
                    slice->wbpWeight[k+0][i][j][comp] = 32;
                    slice->wpOffset[k+0][i][comp] = 0;
                    slice->wpOffset[k+1][j][comp] = 0;
                    }
                  }
                }
              }
    }
  }
//}}}
//{{{
static void errorTracking (sVidParam* vidParam, sSlice* slice) {

  if (slice->redundantPicCount == 0)
    vidParam->isPrimaryOk = vidParam->isReduncantOk = 1;

  if ((slice->redundantPicCount == 0) && (vidParam->type != I_SLICE)) {
    for (int i = 0; i < slice->numRefIndexActive[LIST_0];++i)
      if (slice->refFlag[i] == 0)  // any reference of primary slice is incorrect
        vidParam->isPrimaryOk = 0; // primary slice is incorrect
    }
  else if ((slice->redundantPicCount != 0) && (vidParam->type != I_SLICE))
    // reference of redundant slice is incorrect
    if (slice->refFlag[slice->redundantSliceRefIndex] == 0)
      // redundant slice is incorrect
      vidParam->isReduncantOk = 0;
  }
//}}}
//{{{
static void reorderLists (sSlice* slice) {

  sVidParam* vidParam = slice->vidParam;

  if ((slice->sliceType != I_SLICE) && (slice->sliceType != SI_SLICE)) {
    if (slice->ref_pic_list_reordering_flag[LIST_0])
      reorderRefPicList (slice, LIST_0);
    if (vidParam->noReferencePicture == slice->listX[0][slice->numRefIndexActive[LIST_0] - 1]) {
      if (vidParam->nonConformingStream)
        printf ("RefPicList0[ %d ] is equal to 'no reference picture'\n", slice->numRefIndexActive[LIST_0] - 1);
      else
        error ("RefPicList0[ num_ref_idx_l0_active_minus1 ] is equal to 'no reference picture', invalid bitstream",500);
      }
    // that's a definition
    slice->listXsize[0] = (char) slice->numRefIndexActive[LIST_0];
    }

  if (slice->sliceType == B_SLICE) {
    if (slice->ref_pic_list_reordering_flag[LIST_1])
      reorderRefPicList (slice, LIST_1);
    if (vidParam->noReferencePicture == slice->listX[1][slice->numRefIndexActive[LIST_1]-1]) {
      if (vidParam->nonConformingStream)
        printf ("RefPicList1[ %d ] is equal to 'no reference picture'\n", slice->numRefIndexActive[LIST_1] - 1);
      else
        error ("RefPicList1[ num_ref_idx_l1_active_minus1 ] is equal to 'no reference picture', invalid bitstream",500);
      }
    // that's a definition
    slice->listXsize[1] = (char)slice->numRefIndexActive[LIST_1];
    }

  freeRefPicListReorderingBuffer (slice);
  }
//}}}
//{{{
static void ercWriteMBMODEandMV (sMacroblock* mb) {

  sVidParam* vidParam = mb->vidParam;
  int currMBNum = mb->mbAddrX; //vidParam->currentSlice->curMbNum;
  sPicture* picture = vidParam->picture;
  int mbx = xPosMB(currMBNum, picture->sizeX), mby = yPosMB(currMBNum, picture->sizeX);

  sObjectBuffer* currRegion = vidParam->ercObjectList + (currMBNum << 2);

  if (vidParam->type != B_SLICE) {
    //{{{  non-B frame
    for (int i = 0; i < 4; ++i) {
      sObjectBuffer* pRegion = currRegion + i;
      pRegion->regionMode = (mb->mbType  ==I16MB  ? REGMODE_INTRA :
                               mb->b8mode[i] == IBLOCK ? REGMODE_INTRA_8x8  :
                                 mb->b8mode[i] == 0 ? REGMODE_INTER_COPY :
                                   mb->b8mode[i] == 1 ? REGMODE_INTER_PRED :
                                     REGMODE_INTER_PRED_8x8);

      if (mb->b8mode[i] == 0 || mb->b8mode[i] == IBLOCK) {
        // INTRA OR COPY
        pRegion->mv[0] = 0;
        pRegion->mv[1] = 0;
        pRegion->mv[2] = 0;
        }
      else {
        int ii = 4*mbx + (i & 0x01)*2;// + BLOCK_SIZE;
        int jj = 4*mby + (i >> 1  )*2;
        if (mb->b8mode[i]>=5 && mb->b8mode[i] <= 7) {
          // SMALL BLOCKS
          pRegion->mv[0] = (picture->mvInfo[jj][ii].mv[LIST_0].mvX + picture->mvInfo[jj][ii + 1].mv[LIST_0].mvX + picture->mvInfo[jj + 1][ii].mv[LIST_0].mvX + picture->mvInfo[jj + 1][ii + 1].mv[LIST_0].mvX + 2)/4;
          pRegion->mv[1] = (picture->mvInfo[jj][ii].mv[LIST_0].mvY + picture->mvInfo[jj][ii + 1].mv[LIST_0].mvY + picture->mvInfo[jj + 1][ii].mv[LIST_0].mvY + picture->mvInfo[jj + 1][ii + 1].mv[LIST_0].mvY + 2)/4;
          }
        else {
          // 16x16, 16x8, 8x16, 8x8
          pRegion->mv[0] = picture->mvInfo[jj][ii].mv[LIST_0].mvX;
          pRegion->mv[1] = picture->mvInfo[jj][ii].mv[LIST_0].mvY;
          }

        mb->slice->ercMvPerMb += iabs(pRegion->mv[0]) + iabs(pRegion->mv[1]);
        pRegion->mv[2] = picture->mvInfo[jj][ii].refIndex[LIST_0];
        }
      }
    }
    //}}}
  else {
    //{{{  B-frame
    for (int i = 0; i < 4; ++i) {
      int ii = 4*mbx + (i%2) * 2;
      int jj = 4*mby + (i/2) * 2;

      sObjectBuffer* pRegion = currRegion + i;
      pRegion->regionMode = (mb->mbType == I16MB  ? REGMODE_INTRA :
                               mb->b8mode[i] == IBLOCK ? REGMODE_INTRA_8x8 :
                                 REGMODE_INTER_PRED_8x8);

      if (mb->mbType == I16MB || mb->b8mode[i] == IBLOCK) {
        // INTRA
        pRegion->mv[0] = 0;
        pRegion->mv[1] = 0;
        pRegion->mv[2] = 0;
        }
      else {
        int idx = (picture->mvInfo[jj][ii].refIndex[0] < 0) ? 1 : 0;
        pRegion->mv[0] = (picture->mvInfo[jj][ii].mv[idx].mvX +
                          picture->mvInfo[jj][ii+1].mv[idx].mvX +
                          picture->mvInfo[jj+1][ii].mv[idx].mvX +
                          picture->mvInfo[jj+1][ii+1].mv[idx].mvX + 2)/4;

        pRegion->mv[1] = (picture->mvInfo[jj][ii].mv[idx].mvY +
                          picture->mvInfo[jj][ii+1].mv[idx].mvY +
                          picture->mvInfo[jj+1][ii].mv[idx].mvY +
                          picture->mvInfo[jj+1][ii+1].mv[idx].mvY + 2)/4;
        mb->slice->ercMvPerMb += iabs(pRegion->mv[0]) + iabs(pRegion->mv[1]);

        pRegion->mv[2]  = (picture->mvInfo[jj][ii].refIndex[idx]);
        }
      }
    }
    //}}}
  }
//}}}
//{{{
static void initCurImg (sSlice* slice, sVidParam* vidParam) {

  if ((vidParam->sepColourPlaneFlag != 0)) {
    sPicture* vidref = vidParam->noReferencePicture;
    int noref = (slice->framePoc < vidParam->recoveryPoc);
    switch (slice->colourPlaneId) {
      case 0:
        for (int j = 0; j < 6; j++) { //for (j = 0; j < (slice->sliceType==B_SLICE?2:1); j++) {
          for (int i = 0; i < MAX_LIST_SIZE; i++) {
            sPicture* curRef = slice->listX[j][i];
            if (curRef) {
              curRef->noRef = noref && (curRef == vidref);
              curRef->curPixelY = curRef->imgY;
              }
            }
          }
        break;
      }
    }

  else {
    sPicture* vidref = vidParam->noReferencePicture;
    int noref = (slice->framePoc < vidParam->recoveryPoc);
    int total_lists = slice->mbAffFrameFlag ? 6 : (slice->sliceType == B_SLICE ? 2 : 1);

    for (int j = 0; j < total_lists; j++) {
      // note that if we always set this to MAX_LIST_SIZE, we avoid crashes with invalid refIndex being set
      // since currently this is done at the slice level, it seems safe to do so.
      // Note for some reason I get now a mismatch between version 12 and this one in cabac. I wonder why.
      for (int i = 0; i < MAX_LIST_SIZE; i++) {
        sPicture* curRef = slice->listX[j][i];
        if (curRef) {
          curRef->noRef = noref && (curRef == vidref);
          curRef->curPixelY = curRef->imgY;
          }
        }
      }
    }
  }
//}}}

//{{{
static int isNewPicture (sPicture* picture, sSlice* slice, sOldSliceParam* oldSliceParam) {

  int result = (NULL == picture);

  result |= (oldSliceParam->ppsId != slice->ppsId);
  result |= (oldSliceParam->frameNum != slice->frameNum);
  result |= (oldSliceParam->fieldPicFlag != slice->fieldPicFlag);

  if (slice->fieldPicFlag && oldSliceParam->fieldPicFlag)
    result |= (oldSliceParam->botFieldFlag != slice->botFieldFlag);

  result |= (oldSliceParam->nalRefIdc != slice->refId) && ((oldSliceParam->nalRefIdc == 0) || (slice->refId == 0));
  result |= (oldSliceParam->idrFlag != slice->idrFlag);

  if (slice->idrFlag && oldSliceParam->idrFlag)
    result |= (oldSliceParam->idrPicId != slice->idrPicId);

  sVidParam* vidParam = slice->vidParam;
  if (vidParam->activeSPS->picOrderCountType == 0) {
    result |= (oldSliceParam->picOrderCountLsb != slice->picOrderCountLsb);
    if (vidParam->activePPS->botFieldPicOrderFramePresentFlag  ==  1 &&  !slice->fieldPicFlag )
      result |= (oldSliceParam->deltaPicOrderCountBot != slice->deletaPicOrderCountBot);
    }

  if (vidParam->activeSPS->picOrderCountType == 1) {
    if (!vidParam->activeSPS->delta_pic_order_always_zero_flag) {
      result |= (oldSliceParam->deltaPicOrderCount[0] != slice->deltaPicOrderCount[0]);
      if (vidParam->activePPS->botFieldPicOrderFramePresentFlag  ==  1 &&  !slice->fieldPicFlag )
        result |= (oldSliceParam->deltaPicOrderCount[1] != slice->deltaPicOrderCount[1]);
      }
    }

  result |= (slice->layerId != oldSliceParam->layerId);
  return result;
  }
//}}}
//{{{
static void copyDecPicture_JV (sVidParam* vidParam, sPicture* dst, sPicture* src) {

  dst->topPoc = src->topPoc;
  dst->botPoc = src->botPoc;
  dst->framePoc = src->framePoc;
  dst->poc = src->poc;

  dst->qp = src->qp;
  dst->sliceQpDelta = src->sliceQpDelta;
  dst->chromaQpOffset[0] = src->chromaQpOffset[0];
  dst->chromaQpOffset[1] = src->chromaQpOffset[1];

  dst->sliceType = src->sliceType;
  dst->usedForReference = src->usedForReference;
  dst->idrFlag = src->idrFlag;
  dst->noOutputPriorPicFlag = src->noOutputPriorPicFlag;
  dst->longTermRefFlag = src->longTermRefFlag;
  dst->adaptiveRefPicBufferingFlag = src->adaptiveRefPicBufferingFlag;
  dst->decRefPicMarkingBuffer = src->decRefPicMarkingBuffer;
  dst->mbAffFrameFlag = src->mbAffFrameFlag;
  dst->PicWidthInMbs = src->PicWidthInMbs;
  dst->picNum  = src->picNum;
  dst->frameNum = src->frameNum;
  dst->recoveryFrame = src->recoveryFrame;
  dst->codedFrame = src->codedFrame;
  dst->chromaFormatIdc = src->chromaFormatIdc;
  dst->frameMbOnlyFlag = src->frameMbOnlyFlag;
  dst->frameCropFlag = src->frameCropFlag;
  dst->frameCropLeft = src->frameCropLeft;
  dst->frameCropRight = src->frameCropRight;
  dst->frameCropTop = src->frameCropTop;
  dst->frameCropBot = src->frameCropBot;
  }
//}}}
//{{{
static void initPicture (sVidParam* vidParam, sSlice* slice) {

  sPicture* picture = NULL;
  sSPS* activeSPS = vidParam->activeSPS;
  sDPB* dpb = slice->dpb;

  vidParam->picHeightInMbs = vidParam->FrameHeightInMbs / ( 1 + slice->fieldPicFlag );
  vidParam->picSizeInMbs = vidParam->PicWidthInMbs * vidParam->picHeightInMbs;
  vidParam->FrameSizeInMbs = vidParam->PicWidthInMbs * vidParam->FrameHeightInMbs;
  vidParam->bFrameInit = 1;

  if (vidParam->picture) // this may only happen on slice loss
    exitPicture (vidParam, &vidParam->picture);

  vidParam->dpbLayerId = slice->layerId;
  setupBuffers (vidParam, slice->layerId);

  if (vidParam->recoveryPoint)
    vidParam->recoveryFrameNum = (slice->frameNum + vidParam->recoveryFrameCount) % vidParam->maxFrameNum;
  if (slice->idrFlag)
    vidParam->recoveryFrameNum = slice->frameNum;
  if (vidParam->recoveryPoint == 0 &&
    slice->frameNum != vidParam->preFrameNum &&
    slice->frameNum != (vidParam->preFrameNum + 1) % vidParam->maxFrameNum) {
    if (activeSPS->gaps_in_frame_num_value_allowed_flag == 0) {
      // picture error conceal
      if (vidParam->inputParam->concealMode != 0) {
        if ((slice->frameNum) < ((vidParam->preFrameNum + 1) % vidParam->maxFrameNum)) {
          /* Conceal lost IDR frames and any frames immediately following the IDR.
          // Use frame copy for these since lists cannot be formed correctly for motion copy*/
          vidParam->concealMode = 1;
          vidParam->IdrConcealFlag = 1;
          concealLostFrames (dpb, slice);
          // reset to original conceal mode for future drops
          vidParam->concealMode = vidParam->inputParam->concealMode;
          }
        else {
          // reset to original conceal mode for future drops
          vidParam->concealMode = vidParam->inputParam->concealMode;
          vidParam->IdrConcealFlag = 0;
          concealLostFrames (dpb, slice);
          }
        }
      else
        // Advanced Error Concealment would be called here to combat unintentional loss of pictures
        error ("An unintentional loss of pictures occurs! Exit\n", 100);
      }
    if (vidParam->concealMode == 0)
      fillFrameNumGap (vidParam, slice);
    }

  if (slice->refId)
    vidParam->preFrameNum = slice->frameNum;

  // calculate POC
  decodePOC (vidParam, slice);

  if (vidParam->recoveryFrameNum == (int)slice->frameNum && vidParam->recoveryPoc == 0x7fffffff)
    vidParam->recoveryPoc = slice->framePoc;

  if(slice->refId)
    vidParam->lastRefPicPoc = slice->framePoc;

  if ((slice->structure == FRAME) || (slice->structure == TopField))
    gettime (&(vidParam->startTime));

  picture = vidParam->picture = allocPicture (vidParam, slice->structure, vidParam->width, vidParam->height, vidParam->widthCr, vidParam->heightCr, 1);
  picture->topPoc = slice->topPoc;
  picture->botPoc = slice->botPoc;
  picture->framePoc = slice->framePoc;
  picture->qp = slice->qp;
  picture->sliceQpDelta = slice->sliceQpDelta;
  picture->chromaQpOffset[0] = vidParam->activePPS->chromaQpIndexOffset;
  picture->chromaQpOffset[1] = vidParam->activePPS->secondChromaQpIndexOffset;
  picture->iCodingType = slice->structure == FRAME ?
    (slice->mbAffFrameFlag? FRAME_MB_PAIR_CODING:FRAME_CODING) : FIELD_CODING;
  picture->layerId = slice->layerId;

  // reset all variables of the error conceal instance before decoding of every frame.
  // here the third parameter should, if perfectly, be equal to the number of slices per frame.
  // using little value is ok, the code will allocate more memory if the slice number is larger
  ercReset (vidParam->ercErrorVar, vidParam->picSizeInMbs, vidParam->picSizeInMbs, picture->sizeX);

  vidParam->ercMvPerMb = 0;
  switch (slice->structure ) {
    //{{{
    case TopField:
      picture->poc = slice->topPoc;
      vidParam->number *= 2;
      break;
    //}}}
    //{{{
    case BotField:
      picture->poc = slice->botPoc;
      vidParam->number = vidParam->number * 2 + 1;
      break;
    //}}}
    //{{{
    case FRAME:
      picture->poc = slice->framePoc;
      break;
    //}}}
    //{{{
    default:
      error ("vidParam->structure not initialized", 235);
    //}}}
    }

  //vidParam->sliceNum=0;
  if (vidParam->type > SI_SLICE) {
    setEcFlag (vidParam, SE_PTYPE);
    vidParam->type = P_SLICE;  // concealed element
    }

  // CAVLC init
  if (vidParam->activePPS->entropyCodingModeFlag == (Boolean) CAVLC)
    memset (vidParam->nzCoeff[0][0][0], -1, vidParam->picSizeInMbs * 48 *sizeof(byte)); // 3 * 4 * 4

  // Set the sliceNum member of each MB to -1, to ensure correct when packet loss occurs
  // TO set sMacroblock Map (mark all MBs as 'have to be concealed')
  if (vidParam->sepColourPlaneFlag != 0) {
    for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
      sMacroblock* mb = vidParam->mbDataJV[nplane];
      char* intraBlock = vidParam->intraBlockJV[nplane];
      for (int i = 0; i < (int)vidParam->picSizeInMbs; ++i)
        resetMb (mb++);
      memset (vidParam->predModeJV[nplane][0], DC_PRED, 16 * vidParam->FrameHeightInMbs * vidParam->PicWidthInMbs * sizeof(char));
      if (vidParam->activePPS->constrainedIntraPredFlag)
        for (int i = 0; i < (int)vidParam->picSizeInMbs; ++i)
          intraBlock[i] = 1;
      }
    }
  else {
    sMacroblock* mb = vidParam->mbData;
    for (int i = 0; i < (int)vidParam->picSizeInMbs; ++i)
      resetMb (mb++);
    if (vidParam->activePPS->constrainedIntraPredFlag)
      for (int i = 0; i < (int)vidParam->picSizeInMbs; ++i)
        vidParam->intraBlock[i] = 1;
    memset (vidParam->predMode[0], DC_PRED, 16 * vidParam->FrameHeightInMbs * vidParam->PicWidthInMbs * sizeof(char));
    }

  picture->sliceType = vidParam->type;
  picture->usedForReference = (slice->refId != 0);
  picture->idrFlag = slice->idrFlag;
  picture->noOutputPriorPicFlag = slice->noOutputPriorPicFlag;
  picture->longTermRefFlag = slice->longTermRefFlag;
  picture->adaptiveRefPicBufferingFlag = slice->adaptiveRefPicBufferingFlag;
  picture->decRefPicMarkingBuffer = slice->decRefPicMarkingBuffer;
  slice->decRefPicMarkingBuffer = NULL;

  picture->mbAffFrameFlag = slice->mbAffFrameFlag;
  picture->PicWidthInMbs = vidParam->PicWidthInMbs;

  vidParam->get_mb_block_pos = picture->mbAffFrameFlag ? get_mb_block_pos_mbaff : get_mb_block_pos_normal;
  vidParam->getNeighbour = picture->mbAffFrameFlag ? getAffNeighbour : getNonAffNeighbour;

  picture->picNum = slice->frameNum;
  picture->frameNum = slice->frameNum;
  picture->recoveryFrame = (unsigned int)((int)slice->frameNum == vidParam->recoveryFrameNum);
  picture->codedFrame = (slice->structure == FRAME);
  picture->chromaFormatIdc = activeSPS->chromaFormatIdc;
  picture->frameMbOnlyFlag = activeSPS->frameMbOnlyFlag;
  picture->frameCropFlag = activeSPS->frameCropFlag;
  if (picture->frameCropFlag) {
    picture->frameCropLeft = activeSPS->frameCropLeft;
    picture->frameCropRight = activeSPS->frameCropRight;
    picture->frameCropTop = activeSPS->frameCropTop;
    picture->frameCropBot = activeSPS->frameCropBot;
    }

  if (vidParam->sepColourPlaneFlag != 0) {
    vidParam->decPictureJV[0] = vidParam->picture;
    vidParam->decPictureJV[1] = allocPicture (vidParam, (ePicStructure) slice->structure, vidParam->width, vidParam->height, vidParam->widthCr, vidParam->heightCr, 1);
    copyDecPicture_JV (vidParam, vidParam->decPictureJV[1], vidParam->decPictureJV[0] );
    vidParam->decPictureJV[2] = allocPicture (vidParam, (ePicStructure) slice->structure, vidParam->width, vidParam->height, vidParam->widthCr, vidParam->heightCr, 1);
    copyDecPicture_JV (vidParam, vidParam->decPictureJV[2], vidParam->decPictureJV[0] );
    }
  }
//}}}
//{{{
static void initPictureDecoding (sVidParam* vidParam) {

  int deblockMode = 1;

  if (vidParam->curPicSliceNum >= MAX_NUM_SLICES)
    error ("Maximum number of supported slices exceeded, increase MAX_NUM_SLICES", 200);

  sSlice* slice = vidParam->sliceList[0];
  if (vidParam->nextPPS->valid &&
      (int)vidParam->nextPPS->ppsId == slice->ppsId) {
    sPPS pps;
    memcpy (&pps, &(vidParam->PicParSet[slice->ppsId]), sizeof (sPPS));
    (vidParam->PicParSet[slice->ppsId]).sliceGroupId = NULL;
    makePPSavailable (vidParam, vidParam->nextPPS->ppsId, vidParam->nextPPS);
    memcpy (vidParam->nextPPS, &pps, sizeof (sPPS));
    pps.sliceGroupId = NULL;
    }

  useParameterSet (slice);
  if (slice->idrFlag)
    vidParam->number = 0;

  vidParam->picHeightInMbs = vidParam->FrameHeightInMbs / ( 1 + slice->fieldPicFlag );
  vidParam->picSizeInMbs = vidParam->PicWidthInMbs * vidParam->picHeightInMbs;
  vidParam->FrameSizeInMbs = vidParam->PicWidthInMbs * vidParam->FrameHeightInMbs;
  vidParam->structure = slice->structure;
  initFmo (vidParam, slice);

  updatePicNum (slice);

  initDeblock (vidParam, slice->mbAffFrameFlag);
  for (int j = 0; j < vidParam->curPicSliceNum; j++) {
    if (vidParam->sliceList[j]->DFDisableIdc != 1)
      deblockMode = 0;
    }

  vidParam->deblockMode = deblockMode;
  }
//}}}
static void framePostProcessing (sVidParam* vidParam) {}
static void fieldPostProcessing (sVidParam* vidParam) { vidParam->number /= 2; }

//{{{
static void copySliceInfo (sSlice* slice, sOldSliceParam* oldSliceParam) {

  sVidParam* vidParam = slice->vidParam;

  oldSliceParam->ppsId = slice->ppsId;
  oldSliceParam->frameNum = slice->frameNum; //vidParam->frameNum;
  oldSliceParam->fieldPicFlag = slice->fieldPicFlag; //vidParam->fieldPicFlag;

  if (slice->fieldPicFlag)
    oldSliceParam->botFieldFlag = slice->botFieldFlag;

  oldSliceParam->nalRefIdc = slice->refId;
  oldSliceParam->idrFlag = (byte)slice->idrFlag;

  if (slice->idrFlag)
    oldSliceParam->idrPicId = slice->idrPicId;

  if (vidParam->activeSPS->picOrderCountType == 0) {
    oldSliceParam->picOrderCountLsb = slice->picOrderCountLsb;
    oldSliceParam->deltaPicOrderCountBot = slice->deletaPicOrderCountBot;
    }

  if (vidParam->activeSPS->picOrderCountType == 1) {
    oldSliceParam->deltaPicOrderCount[0] = slice->deltaPicOrderCount[0];
    oldSliceParam->deltaPicOrderCount[1] = slice->deltaPicOrderCount[1];
    }

  oldSliceParam->layerId = slice->layerId;
  }
//}}}
//{{{
static void initSlice (sVidParam* vidParam, sSlice* slice) {

  vidParam->activeSPS = slice->activeSPS;
  vidParam->activePPS = slice->activePPS;

  slice->initLists (slice);
  reorderLists (slice);

  if (slice->structure == FRAME)
    init_mbaff_lists (vidParam, slice);

  // update reference flags and set current vidParam->refFlag
  if (!((slice->redundantPicCount != 0) && (vidParam->prevFrameNum == slice->frameNum)))
    for (int i = 16; i > 0; i--)
      slice->refFlag[i] = slice->refFlag[i-1];
  slice->refFlag[0] = slice->redundantPicCount == 0 ? vidParam->isPrimaryOk : vidParam->isReduncantOk;

  if ((slice->activeSPS->chromaFormatIdc == 0) ||
      (slice->activeSPS->chromaFormatIdc == 3)) {
    slice->linfoCbpIntra = linfo_cbp_intra_other;
    slice->linfoCbpInter = linfo_cbp_inter_other;
    }
  else {
    slice->linfoCbpIntra = linfo_cbp_intra_normal;
    slice->linfoCbpInter = linfo_cbp_inter_normal;
    }
  }
//}}}
//{{{
static void decodeOneSlice (sSlice* slice) {

  Boolean endOfSlice = FALSE;

  slice->codCount=-1;

  sVidParam* vidParam = slice->vidParam;
  if ((vidParam->sepColourPlaneFlag != 0))
    changePlaneJV (vidParam, slice->colourPlaneId, slice);
  else {
    slice->mbData = vidParam->mbData;
    slice->picture = vidParam->picture;
    slice->siBlock = vidParam->siBlock;
    slice->predMode = vidParam->predMode;
    slice->intraBlock = vidParam->intraBlock;
    }

  if (slice->sliceType == B_SLICE)
    computeColocated (slice, slice->listX);

  if (slice->sliceType != I_SLICE && slice->sliceType != SI_SLICE)
    initCurImg (slice, vidParam);

  // loop over macroblocks
  while (endOfSlice == FALSE) {
    sMacroblock* mb;
    startMacroblock (slice, &mb);
    slice->readOneMacroblock (mb);
    decodeMacroblock (mb, slice->picture);

    if (slice->mbAffFrameFlag && mb->mbField) {
      slice->numRefIndexActive[LIST_0] >>= 1;
      slice->numRefIndexActive[LIST_1] >>= 1;
      }

    ercWriteMBMODEandMV (mb);
    endOfSlice = exitMacroblock (slice, (!slice->mbAffFrameFlag|| slice->curMbNum%2));
    }
  }
//}}}
//{{{
static int readNewSlice (sSlice* slice) {

  sVidParam* vidParam = slice->vidParam;

  int curHeader = 0;
  sBitstream* curStream = NULL;
  for (;;) {
    sNalu* nalu = vidParam->nalu;
    if (!pendingNalu) {
      if (!readNextNalu (vidParam, nalu))
        return EOS;
      }
    else {
      nalu = pendingNalu;
      pendingNalu = NULL;
      }

  process_nalu:
    switch (nalu->unitType) {
      case NALU_TYPE_SLICE:
      //{{{
      case NALU_TYPE_IDR:
        if (vidParam->recoveryPoint || nalu->unitType == NALU_TYPE_IDR) {
          if (vidParam->recoveryPointFound == 0) {
            if (nalu->unitType != NALU_TYPE_IDR) {
              printf ("Warning: Decoding does not start with an IDR picture.\n");
              vidParam->nonConformingStream = 1;
              }
            else
              vidParam->nonConformingStream = 0;
            }
          vidParam->recoveryPointFound = 1;
          }

        if (vidParam->recoveryPointFound == 0)
          break;

        slice->idrFlag = (nalu->unitType == NALU_TYPE_IDR);
        slice->refId = nalu->refId;
        slice->dataPartitionMode = PAR_DP_1;
        slice->maxPartitionNum = 1;

        curStream = slice->partitions[0].bitstream;
        curStream->eiFlag = 0;
        curStream->frameBitOffset = curStream->readLen = 0;
        memcpy (curStream->streamBuffer, &nalu->buf[1], nalu->len-1);
        curStream->codeLen = curStream->bitstreamLength = RBSPtoSODB (curStream->streamBuffer, nalu->len - 1);

        // Some syntax of the sliceHeader depends on parameter set
        // which depends on the parameter set ID of the slice header.
        // - read the ppsId of the slice header first,
        // - then setup the active parameter sets
        // -  read // the rest of the slice header
        readSliceHeader (slice);
        useParameterSet (slice);
        slice->activeSPS = vidParam->activeSPS;
        slice->activePPS = vidParam->activePPS;
        slice->transform8x8Mode = vidParam->activePPS->transform8x8modeFlag;
        slice->chroma444notSeparate = (vidParam->activeSPS->chromaFormatIdc == YUV444) &&
                                      (vidParam->sepColourPlaneFlag == 0);
        readRestSliceHeader (slice);
        assignQuantParams (slice);

        // if primary slice is replaced with redundant slice, set the correct image type
        if (slice->redundantPicCount && vidParam->isPrimaryOk == 0 && vidParam->isReduncantOk)
          vidParam->picture->sliceType = vidParam->type;

        if (isNewPicture (vidParam->picture, slice, vidParam->oldSlice)) {
          if (vidParam->curPicSliceNum == 0)
            initPicture (vidParam, slice);
          curHeader = SOP;
          // check zero_byte if it is also the first NAL unit in the access unit
          checkZeroByteVCL (vidParam, nalu);
          }
        else
          curHeader = SOS;

        setSliceMethods (slice);

        // Vid->activeSPS, vidParam->activePPS, sliceHeader valid
        if (slice->mbAffFrameFlag)
          slice->curMbNum = slice->startMbNum << 1;
        else
          slice->curMbNum = slice->startMbNum;

        if (vidParam->activePPS->entropyCodingModeFlag) {
          int ByteStartPosition = curStream->frameBitOffset / 8;
          if ((curStream->frameBitOffset % 8) != 0)
            ++ByteStartPosition;
          arideco_start_decoding (&slice->partitions[0].deCabac, curStream->streamBuffer,
                                  ByteStartPosition, &curStream->readLen);
          }

        vidParam->recoveryPoint = 0;
        return curHeader;
        break;
      //}}}
      //{{{
      case NALU_TYPE_DPA:
        if (vidParam->recoveryPointFound == 0)
          break;

        // read DP_A
        slice->noDataPartitionB = 1;
        slice->noDataPartitionC = 1;
        slice->idrFlag = FALSE;
        slice->refId = nalu->refId;
        slice->dataPartitionMode = PAR_DP_3;
        slice->maxPartitionNum = 3;
        curStream = slice->partitions[0].bitstream;
        curStream->eiFlag = 0;
        curStream->frameBitOffset = curStream->readLen = 0;
        memcpy (curStream->streamBuffer, &nalu->buf[1], nalu->len - 1);
        curStream->codeLen = curStream->bitstreamLength = RBSPtoSODB (curStream->streamBuffer, nalu->len-1);

        readSliceHeader (slice);
        useParameterSet (slice);
        slice->activeSPS = vidParam->activeSPS;
        slice->activePPS = vidParam->activePPS;
        slice->transform8x8Mode = vidParam->activePPS->transform8x8modeFlag;
        slice->chroma444notSeparate = (vidParam->activeSPS->chromaFormatIdc == YUV444) &&
                                      (vidParam->sepColourPlaneFlag == 0);
        readRestSliceHeader (slice);
        assignQuantParams (slice);

        if (isNewPicture (vidParam->picture, slice, vidParam->oldSlice)) {
          if (vidParam->curPicSliceNum == 0)
            initPicture (vidParam, slice);
          curHeader = SOP;
          checkZeroByteVCL (vidParam, nalu);
          }
        else
          curHeader = SOS;

        setSliceMethods (slice);

        // From here on, vidParam->activeSPS, vidParam->activePPS and the slice header are valid
        if (slice->mbAffFrameFlag)
          slice->curMbNum = slice->startMbNum << 1;
        else
          slice->curMbNum = slice->startMbNum;

        // need to read the slice ID, which depends on the value of redundantPicCountPresentFlag

        int slice_id_a = readUeV ("NALU: DP_A slice_id", curStream);

        if (vidParam->activePPS->entropyCodingModeFlag)
          error ("data partition with CABAC not allowed", 500);

        // reading next DP
        if (!readNextNalu (vidParam, nalu))
          return curHeader;
        if (NALU_TYPE_DPB == nalu->unitType) {
          //{{{  got nalu DPB
          curStream = slice->partitions[1].bitstream;
          curStream->eiFlag = 0;
          curStream->frameBitOffset = curStream->readLen = 0;
          memcpy (curStream->streamBuffer, &nalu->buf[1], nalu->len-1);
          curStream->codeLen = curStream->bitstreamLength = RBSPtoSODB (curStream->streamBuffer, nalu->len-1);
          int slice_id_b = readUeV ("NALU dataPartitionB sliceId", curStream);
          slice->noDataPartitionB = 0;

          if ((slice_id_b != slice_id_a) || (nalu->lostPackets)) {
            printf ("dataPartitionB does not match dataPartitionA\n");
            slice->noDataPartitionB = 1;
            slice->noDataPartitionC = 1;
            }
          else {
            if (vidParam->activePPS->redundantPicCountPresentFlag)
              readUeV ("NALU dataPartitionB redundantPicCount", curStream);

            // we're finished with DP_B, so let's continue with next DP
            if (!readNextNalu (vidParam, nalu))
              return curHeader;
            }
          }
          //}}}
        else
          slice->noDataPartitionB = 1;

        if (NALU_TYPE_DPC == nalu->unitType) {
          //{{{  got nalu DPC
          curStream = slice->partitions[2].bitstream;
          curStream->eiFlag = 0;
          curStream->frameBitOffset = curStream->readLen = 0;
          memcpy (curStream->streamBuffer, &nalu->buf[1], nalu->len-1);
          curStream->codeLen = curStream->bitstreamLength = RBSPtoSODB (curStream->streamBuffer, nalu->len-1);

          slice->noDataPartitionC = 0;
          int slice_id_c = readUeV ("NALU: DP_C slice_id", curStream);
          if ((slice_id_c != slice_id_a)|| (nalu->lostPackets)) {
            printf ("Warning: got a data partition C which does not match DP_A(DP loss!)\n");
            slice->noDataPartitionC =1;
            }

          if (vidParam->activePPS->redundantPicCountPresentFlag)
            readUeV ("NALU:SLICE_C redudand_pic_cnt", curStream);
          }
          //}}}
        else {
          slice->noDataPartitionC = 1;
          pendingNalu = nalu;
          }

        // check if we read anything else than the expected partitions
        if ((nalu->unitType != NALU_TYPE_DPB) &&
            (nalu->unitType != NALU_TYPE_DPC) && (!slice->noDataPartitionC))
          goto process_nalu;

        return curHeader;
        break;
      //}}}
      //{{{
      case NALU_TYPE_DPB:
        printf ("found data partition B without matching DP A\n");
        break;
      //}}}
      //{{{
      case NALU_TYPE_DPC:
        printf ("found data partition C without matching DP A\n");
        break;
      //}}}
      //{{{
      case NALU_TYPE_SEI:
        InterpretSEIMessage (nalu->buf, nalu->len, vidParam, slice);
        break;
      //}}}
      //{{{
      case NALU_TYPE_PPS:
        processPPS (vidParam, nalu);
        break;
      //}}}
      //{{{
      case NALU_TYPE_SPS:
        processSPS (vidParam, nalu);
        break;
      //}}}
      case NALU_TYPE_AUD: break;
      case NALU_TYPE_EOSEQ: break;
      case NALU_TYPE_EOSTREAM: break;
      case NALU_TYPE_FILL: break;
      //{{{
      default:
        printf ("unknown NALU type %d, len %d\n", (int) nalu->unitType, (int) nalu->len);
        break;
      //}}}
      }
    }
  }
//}}}

//{{{
void initOldSlice (sOldSliceParam* oldSliceParam) {

  oldSliceParam->fieldPicFlag = 0;
  oldSliceParam->ppsId = INT_MAX;
  oldSliceParam->frameNum = INT_MAX;

  oldSliceParam->nalRefIdc = INT_MAX;
  oldSliceParam->idrFlag = FALSE;

  oldSliceParam->picOrderCountLsb = UINT_MAX;
  oldSliceParam->deltaPicOrderCountBot = INT_MAX;
  oldSliceParam->deltaPicOrderCount[0] = INT_MAX;
  oldSliceParam->deltaPicOrderCount[1] = INT_MAX;
  }
//}}}
//{{{
void calcFrameNum (sVidParam* vidParam, sPicture* p) {

  int psnrPOC = vidParam->activeSPS->mb_adaptive_frame_field_flag ? p->poc / (vidParam->inputParam->pocScale) :
                                                                    p->poc / (vidParam->inputParam->pocScale);
  if (psnrPOC == 0)
    vidParam->idrPsnrNum = vidParam->gapNumFrame * vidParam->refPocGap / (vidParam->inputParam->pocScale);

  vidParam->psnrNum = imax (vidParam->psnrNum, vidParam->idrPsnrNum+psnrPOC);
  vidParam->frameNum = vidParam->idrPsnrNum + psnrPOC;
  }
//}}}
//{{{
void padPicture (sVidParam* vidParam, sPicture* picture) {

  padBuf (*picture->imgY, picture->sizeX, picture->sizeY,
           picture->iLumaStride, vidParam->iLumaPadX, vidParam->iLumaPadY);

  if (picture->chromaFormatIdc != YUV400) {
    padBuf (*picture->imgUV[0], picture->sizeXcr, picture->sizeYcr,
            picture->iChromaStride, vidParam->iChromaPadX, vidParam->iChromaPadY);
    padBuf (*picture->imgUV[1], picture->sizeXcr, picture->sizeYcr,
            picture->iChromaStride, vidParam->iChromaPadX, vidParam->iChromaPadY);
    }
  }
//}}}
//{{{
void exitPicture (sVidParam* vidParam, sPicture** picture) {

  // return if the last picture has already been finished
  if (*picture == NULL ||
      ((vidParam->numDecodedMb != vidParam->picSizeInMbs) &&
       ((vidParam->yuvFormat != YUV444) || !vidParam->sepColourPlaneFlag)))
    return;

  //{{{  error conceal
  frame recfr;
  recfr.vidParam = vidParam;
  recfr.yptr = &(*picture)->imgY[0][0];
  if ((*picture)->chromaFormatIdc != YUV400) {
    recfr.uptr = &(*picture)->imgUV[0][0][0];
    recfr.vptr = &(*picture)->imgUV[1][0][0];
    }

  // this is always true at the beginning of a picture
  int ercSegment = 0;

  // mark the start of the first segment
  if (!(*picture)->mbAffFrameFlag) {
    int i;
    ercStartSegment (0, ercSegment, 0 , vidParam->ercErrorVar);
    // generate the segments according to the macroblock map
    for (i = 1; i < (int)(*picture)->picSizeInMbs; ++i) {
      if (vidParam->mbData[i].eiFlag != vidParam->mbData[i-1].eiFlag) {
        ercStopSegment (i-1, ercSegment, 0, vidParam->ercErrorVar); //! stop current segment

        // mark current segment as lost or OK
        if(vidParam->mbData[i-1].eiFlag)
          ercMarkCurrSegmentLost ((*picture)->sizeX, vidParam->ercErrorVar);
        else
          ercMarkCurrSegmentOK ((*picture)->sizeX, vidParam->ercErrorVar);

        ++ercSegment;  //! next segment
        ercStartSegment (i, ercSegment, 0 , vidParam->ercErrorVar); //! start new segment
        }
      }

    // mark end of the last segment
    ercStopSegment ((*picture)->picSizeInMbs-1, ercSegment, 0, vidParam->ercErrorVar);
    if (vidParam->mbData[i-1].eiFlag)
      ercMarkCurrSegmentLost ((*picture)->sizeX, vidParam->ercErrorVar);
    else
      ercMarkCurrSegmentOK ((*picture)->sizeX, vidParam->ercErrorVar);

    // call the right error conceal function depending on the frame type.
    vidParam->ercMvPerMb /= (*picture)->picSizeInMbs;
    vidParam->ercImg = vidParam;
    if ((*picture)->sliceType == I_SLICE || (*picture)->sliceType == SI_SLICE) // I-frame
      ercConcealIntraFrame (vidParam, &recfr, (*picture)->sizeX, (*picture)->sizeY, vidParam->ercErrorVar);
    else
      ercConcealInterFrame (&recfr, vidParam->ercObjectList, (*picture)->sizeX, (*picture)->sizeY, vidParam->ercErrorVar, (*picture)->chromaFormatIdc);
    }
  //}}}
  if (!vidParam->deblockMode &&
      (vidParam->bDeblockEnable & (1 << (*picture)->usedForReference))) {
    //{{{  deblocking for frame or field
    if( (vidParam->sepColourPlaneFlag != 0) ) {
      int colourPlaneId = vidParam->sliceList[0]->colourPlaneId;
      for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
        vidParam->sliceList[0]->colourPlaneId = nplane;
        changePlaneJV (vidParam, nplane, NULL );
        deblockPicture (vidParam, *picture );
        }
      vidParam->sliceList[0]->colourPlaneId = colourPlaneId;
      make_frame_picture_JV(vidParam);
      }
    else
      deblockPicture (vidParam,* picture);
    }
    //}}}
  else if (vidParam->sepColourPlaneFlag != 0)
    make_frame_picture_JV (vidParam);

  if ((*picture)->mbAffFrameFlag)
    mbAffPostProc (vidParam);

  if (vidParam->structure == FRAME)
    framePostProcessing (vidParam);
  else
    fieldPostProcessing (vidParam);

  if ((*picture)->usedForReference)
    padPicture (vidParam, *picture);

  int structure = (*picture)->structure;
  int sliceType = (*picture)->sliceType;
  int framePoc = (*picture)->framePoc;
  int refpic = (*picture)->usedForReference;
  int qp = (*picture)->qp;
  int picNum = (*picture)->picNum;
  int isIdr = (*picture)->idrFlag;
  int chromaFormatIdc = (*picture)->chromaFormatIdc;

  storePictureDpb (vidParam->dpbLayer[0], *picture);
  *picture = NULL;

  if (vidParam->last_has_mmco_5)
    vidParam->preFrameNum = 0;

  if (structure == TopField || structure == FRAME) {
    //{{{  frame type string
    if (sliceType == I_SLICE && isIdr) // IDR picture
      strcpy (vidParam->sliceTypeText, "IDR");
    else if (sliceType == I_SLICE) // I picture
      strcpy (vidParam->sliceTypeText, " I ");
    else if (sliceType == P_SLICE) // P pictures
      strcpy (vidParam->sliceTypeText, " P ");
    else if (sliceType == SP_SLICE) // SP pictures
      strcpy (vidParam->sliceTypeText, "SP ");
    else if  (sliceType == SI_SLICE)
      strcpy (vidParam->sliceTypeText, "SI ");
    else if (refpic) // stored B pictures
      strcpy (vidParam->sliceTypeText, " B ");
    else // B pictures
      strcpy (vidParam->sliceTypeText, " b ");
    if (structure == FRAME)
      strncat (vidParam->sliceTypeText, "       ",8-strlen(vidParam->sliceTypeText));
    }
    //}}}
  else if (structure == BotField) {
    //{{{  frame type string
    if (sliceType == I_SLICE && isIdr) // IDR picture
      strncat (vidParam->sliceTypeText, "|IDR", 8-strlen(vidParam->sliceTypeText));
    else if (sliceType == I_SLICE) // I picture
      strncat (vidParam->sliceTypeText, "| I ", 8-strlen(vidParam->sliceTypeText));
    else if (sliceType == P_SLICE) // P pictures
      strncat (vidParam->sliceTypeText, "| P ", 8-strlen(vidParam->sliceTypeText));
    else if (sliceType == SP_SLICE) // SP pictures
      strncat (vidParam->sliceTypeText, "|SP ", 8-strlen(vidParam->sliceTypeText));
    else if  (sliceType == SI_SLICE)
      strncat (vidParam->sliceTypeText, "|SI ", 8-strlen(vidParam->sliceTypeText));
    else if (refpic) // stored B pictures
      strncat (vidParam->sliceTypeText, "| B ", 8-strlen(vidParam->sliceTypeText));
    else // B pictures
      strncat (vidParam->sliceTypeText, "| b ", 8-strlen(vidParam->sliceTypeText));
    }
    //}}}
  vidParam->sliceTypeText[8] = 0;

  if ((structure == FRAME) || structure == BotField) {
    gettime (&(vidParam->endTime));
    printf ("%5d %s poc:%4d pic:%3d qp:%2d %dms\n",
            vidParam->frameNum, vidParam->sliceTypeText, framePoc, picNum, qp,
            (int)timenorm (timediff (&(vidParam->startTime), &(vidParam->endTime))));

    // I or P pictures ?
    if (sliceType == I_SLICE || sliceType == SI_SLICE || sliceType == P_SLICE || refpic)
      ++(vidParam->number);

    ++(vidParam->gapNumFrame);
    }
  }
//}}}

//{{{
int decodeFrame (sVidParam* vidParam) {

  int ret = 0;

  vidParam->curPicSliceNum = 0;
  vidParam->numSlicesDecoded = 0;
  vidParam->numDecodedMb = 0;

  sSlice* slice = NULL;
  sSlice** sliceList = vidParam->sliceList;
  int curHeader = 0;
  if (vidParam->newFrame) {
    if (vidParam->nextPPS->valid) {
      //{{{  use next PPS
      makePPSavailable (vidParam, vidParam->nextPPS->ppsId, vidParam->nextPPS);
      vidParam->nextPPS->valid = 0;
      }
      //}}}
    // get firstSlice from slice;
    slice = sliceList[vidParam->curPicSliceNum];
    sliceList[vidParam->curPicSliceNum] = vidParam->nextSlice;
    vidParam->nextSlice = slice;
    slice = sliceList[vidParam->curPicSliceNum];
    useParameterSet (slice);
    initPicture (vidParam, slice);
    vidParam->curPicSliceNum++;
    curHeader = SOS;
    }

  while ((curHeader != SOP) && (curHeader != EOS)) {
    //{{{  no pending slices
    if (!sliceList[vidParam->curPicSliceNum])
      sliceList[vidParam->curPicSliceNum] = allocSlice (vidParam);

    slice = sliceList[vidParam->curPicSliceNum];
    slice->vidParam = vidParam;
    slice->inputParam = vidParam->inputParam;
    slice->dpb = vidParam->dpbLayer[0]; //set default value;
    slice->nextHeader = -8888;
    slice->numDecodedMb = 0;
    slice->coefCount = -1;
    slice->pos = 0;
    slice->isResetCoef = FALSE;
    slice->isResetCoefCr = FALSE;
    curHeader = readNewSlice (slice);
    slice->curHeader = curHeader;

    // error tracking of primary and redundant slices.
    errorTracking (vidParam, slice);
    //{{{  manage primary and redundant slices
    // If primary and redundant are received and primary is correct
    //   discard the redundant
    // else,
    //   primary slice will be replaced with redundant slice.
    if (slice->frameNum == vidParam->prevFrameNum &&
        slice->redundantPicCount != 0 &&
        vidParam->isPrimaryOk != 0 &&
        curHeader != EOS)
      continue;

    if ((curHeader != SOP && curHeader != EOS) ||
        (vidParam->curPicSliceNum == 0 && curHeader == SOP)) {
       slice->curSliceNum = (short)vidParam->curPicSliceNum;
       vidParam->picture->maxSliceId =
         (short)imax (slice->curSliceNum, vidParam->picture->maxSliceId);
       if (vidParam->curPicSliceNum > 0) {
         copyPOC (*sliceList, slice);
         sliceList[vidParam->curPicSliceNum-1]->endMbNumPlus1 = slice->startMbNum;
         }

       vidParam->curPicSliceNum++;
       if (vidParam->curPicSliceNum >= vidParam->numSlicesAllocated) {
         sSlice** tmpSliceList = (sSlice**)realloc (
           vidParam->sliceList, (vidParam->numSlicesAllocated + MAX_NUM_DECSLICES) * sizeof(sSlice*));
         if (!tmpSliceList) {
           tmpSliceList = calloc ((vidParam->numSlicesAllocated + MAX_NUM_DECSLICES), sizeof(sSlice*));
           memcpy (tmpSliceList, vidParam->sliceList, vidParam->curPicSliceNum * sizeof(sSlice*));
           free (vidParam->sliceList);
           sliceList = vidParam->sliceList = tmpSliceList;
           }
         else {
           sliceList = vidParam->sliceList = tmpSliceList;
           memset (vidParam->sliceList + vidParam->curPicSliceNum, 0, sizeof(sSlice*) * MAX_NUM_DECSLICES);
           }
         vidParam->numSlicesAllocated += MAX_NUM_DECSLICES;
        }
      curHeader = SOS;
      }
    else {
      if (sliceList[vidParam->curPicSliceNum-1]->mbAffFrameFlag)
        sliceList[vidParam->curPicSliceNum-1]->endMbNumPlus1 = vidParam->FrameSizeInMbs / 2;
      else
        sliceList[vidParam->curPicSliceNum-1]->endMbNumPlus1 = vidParam->FrameSizeInMbs /
                                                                 (1 + sliceList[vidParam->curPicSliceNum-1]->fieldPicFlag);
      vidParam->newFrame = 1;
      slice->curSliceNum = 0;

      // keep it in currentslice;
      sliceList[vidParam->curPicSliceNum] = vidParam->nextSlice;
      vidParam->nextSlice = slice;
      }
    //}}}

    copySliceInfo (slice, vidParam->oldSlice);
    }
    //}}}

  // decode slices
  ret = curHeader;
  initPictureDecoding (vidParam);
  for (int sliceNum = 0; sliceNum < vidParam->curPicSliceNum; sliceNum++) {
    //{{{  decode slice
    slice = sliceList[sliceNum];
    curHeader = slice->curHeader;
    initSlice (vidParam, slice);

    if (slice->activePPS->entropyCodingModeFlag) {
      initContexts (slice);
      cabacNewSlice (slice);
      }

    if (((slice->activePPS->weightedBiPredIdc > 0)  && (slice->sliceType == B_SLICE)) ||
        (slice->activePPS->weightedPredFlag && (slice->sliceType != I_SLICE)))
      fillWPParam (slice);

    // decode main slice information
    if (((curHeader == SOP) || (curHeader == SOS)) && (slice->eiFlag == 0))
      decodeOneSlice (slice);

    vidParam->numSlicesDecoded++;
    vidParam->numDecodedMb += slice->numDecodedMb;
    vidParam->ercMvPerMb += slice->ercMvPerMb;
    }
    //}}}

  exitPicture (vidParam, &vidParam->picture);
  vidParam->prevFrameNum = sliceList[0]->frameNum;

  return ret;
  }
//}}}
