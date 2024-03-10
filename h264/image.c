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
static void setupBuffers (sDecoder* decoder, int layerId) {

  if (decoder->last_dec_layer_id != layerId) {
    sCoding* coding = decoder->coding[layerId];
    if (coding->sepColourPlaneFlag) {
      for (int i = 0; i < MAX_PLANE; i++ ) {
        decoder->mbDataJV[i] = coding->mbDataJV[i];
        decoder->intraBlockJV[i] = coding->intraBlockJV[i];
        decoder->predModeJV[i] = coding->predModeJV[i];
        decoder->siBlockJV[i] = coding->siBlockJV[i];
        }
      decoder->mbData = NULL;
      decoder->intraBlock = NULL;
      decoder->predMode = NULL;
      decoder->siBlock = NULL;
      }
    else {
      decoder->mbData = coding->mbData;
      decoder->intraBlock = coding->intraBlock;
      decoder->predMode = coding->predMode;
      decoder->siBlock = coding->siBlock;
      }

    decoder->picPos = coding->picPos;
    decoder->nzCoeff = coding->nzCoeff;
    decoder->qpPerMatrix = coding->qpPerMatrix;
    decoder->qpRemMatrix = coding->qpRemMatrix;
    decoder->oldFrameSizeInMbs = coding->oldFrameSizeInMbs;
    decoder->last_dec_layer_id = layerId;
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
static void mbAffPostProc (sDecoder* decoder) {

  sPixel tempBuffer[32][16];
  sPicture* picture = decoder->picture;
  sPixel** imgY = picture->imgY;
  sPixel*** imgUV = picture->imgUV;

  short x0;
  short y0;
  for (short i = 0; i < (int)picture->picSizeInMbs; i += 2) {
    if (picture->motion.mbField[i]) {
      get_mb_pos (decoder, i, decoder->mbSize[IS_LUMA], &x0, &y0);
      updateMbAff (imgY + y0, tempBuffer, x0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

      if (picture->chromaFormatIdc != YUV400) {
        x0 = (short)((x0 * decoder->mbCrSizeX) >> 4);
        y0 = (short)((y0 * decoder->mbCrSizeY) >> 4);
        updateMbAff (imgUV[0] + y0, tempBuffer, x0, decoder->mbCrSizeX, decoder->mbCrSizeY);
        updateMbAff (imgUV[1] + y0, tempBuffer, x0, decoder->mbCrSizeX, decoder->mbCrSizeY);
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
static void errorTracking (sDecoder* decoder, sSlice* slice) {

  if (slice->redundantPicCount == 0)
    decoder->isPrimaryOk = decoder->isReduncantOk = 1;

  if ((slice->redundantPicCount == 0) && (decoder->type != I_SLICE)) {
    for (int i = 0; i < slice->numRefIndexActive[LIST_0];++i)
      if (slice->refFlag[i] == 0)  // any reference of primary slice is incorrect
        decoder->isPrimaryOk = 0; // primary slice is incorrect
    }
  else if ((slice->redundantPicCount != 0) && (decoder->type != I_SLICE))
    // reference of redundant slice is incorrect
    if (slice->refFlag[slice->redundantSliceRefIndex] == 0)
      // redundant slice is incorrect
      decoder->isReduncantOk = 0;
  }
//}}}
//{{{
static void reorderLists (sSlice* slice) {

  sDecoder* decoder = slice->decoder;

  if ((slice->sliceType != I_SLICE) && (slice->sliceType != SI_SLICE)) {
    if (slice->ref_pic_list_reordering_flag[LIST_0])
      reorderRefPicList (slice, LIST_0);
    if (decoder->noReferencePicture == slice->listX[0][slice->numRefIndexActive[LIST_0] - 1]) {
      if (decoder->nonConformingStream)
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
    if (decoder->noReferencePicture == slice->listX[1][slice->numRefIndexActive[LIST_1]-1]) {
      if (decoder->nonConformingStream)
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

  sDecoder* decoder = mb->decoder;
  int currMBNum = mb->mbAddrX; //decoder->currentSlice->curMbNum;
  sPicture* picture = decoder->picture;
  int mbx = xPosMB(currMBNum, picture->sizeX), mby = yPosMB(currMBNum, picture->sizeX);

  sObjectBuffer* currRegion = decoder->ercObjectList + (currMBNum << 2);

  if (decoder->type != B_SLICE) {
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
static void initCurImg (sSlice* slice, sDecoder* decoder) {

  if ((decoder->sepColourPlaneFlag != 0)) {
    sPicture* vidref = decoder->noReferencePicture;
    int noref = (slice->framePoc < decoder->recoveryPoc);
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
    sPicture* vidref = decoder->noReferencePicture;
    int noref = (slice->framePoc < decoder->recoveryPoc);
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
static int isNewPicture (sPicture* picture, sSlice* slice, sOldSlice* oldSliceParam) {

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

  sDecoder* decoder = slice->decoder;
  if (decoder->activeSPS->picOrderCountType == 0) {
    result |= (oldSliceParam->picOrderCountLsb != slice->picOrderCountLsb);
    if (decoder->activePPS->botFieldPicOrderFramePresentFlag  ==  1 &&  !slice->fieldPicFlag )
      result |= (oldSliceParam->deltaPicOrderCountBot != slice->deletaPicOrderCountBot);
    }

  if (decoder->activeSPS->picOrderCountType == 1) {
    if (!decoder->activeSPS->delta_pic_order_always_zero_flag) {
      result |= (oldSliceParam->deltaPicOrderCount[0] != slice->deltaPicOrderCount[0]);
      if (decoder->activePPS->botFieldPicOrderFramePresentFlag  ==  1 &&  !slice->fieldPicFlag )
        result |= (oldSliceParam->deltaPicOrderCount[1] != slice->deltaPicOrderCount[1]);
      }
    }

  result |= (slice->layerId != oldSliceParam->layerId);
  return result;
  }
//}}}
//{{{
static void copyDecPicture_JV (sDecoder* decoder, sPicture* dst, sPicture* src) {

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
static void initPicture (sDecoder* decoder, sSlice* slice) {

  sPicture* picture = NULL;
  sSPS* activeSPS = decoder->activeSPS;
  sDPB* dpb = slice->dpb;

  decoder->picHeightInMbs = decoder->FrameHeightInMbs / ( 1 + slice->fieldPicFlag );
  decoder->picSizeInMbs = decoder->PicWidthInMbs * decoder->picHeightInMbs;
  decoder->FrameSizeInMbs = decoder->PicWidthInMbs * decoder->FrameHeightInMbs;
  decoder->bFrameInit = 1;

  if (decoder->picture) // this may only happen on slice loss
    exitPicture (decoder, &decoder->picture);

  decoder->dpbLayerId = slice->layerId;
  setupBuffers (decoder, slice->layerId);

  if (decoder->recoveryPoint)
    decoder->recoveryFrameNum = (slice->frameNum + decoder->recoveryFrameCount) % decoder->maxFrameNum;
  if (slice->idrFlag)
    decoder->recoveryFrameNum = slice->frameNum;
  if (decoder->recoveryPoint == 0 &&
    slice->frameNum != decoder->preFrameNum &&
    slice->frameNum != (decoder->preFrameNum + 1) % decoder->maxFrameNum) {
    if (activeSPS->gaps_in_frame_num_value_allowed_flag == 0) {
      // picture error conceal
      if (decoder->input.concealMode != 0) {
        if ((slice->frameNum) < ((decoder->preFrameNum + 1) % decoder->maxFrameNum)) {
          /* Conceal lost IDR frames and any frames immediately following the IDR.
          // Use frame copy for these since lists cannot be formed correctly for motion copy*/
          decoder->concealMode = 1;
          decoder->IdrConcealFlag = 1;
          concealLostFrames (dpb, slice);
          // reset to original conceal mode for future drops
          decoder->concealMode = decoder->input.concealMode;
          }
        else {
          // reset to original conceal mode for future drops
          decoder->concealMode = decoder->input.concealMode;
          decoder->IdrConcealFlag = 0;
          concealLostFrames (dpb, slice);
          }
        }
      else
        // Advanced Error Concealment would be called here to combat unintentional loss of pictures
        error ("An unintentional loss of pictures occurs! Exit\n", 100);
      }
    if (decoder->concealMode == 0)
      fillFrameNumGap (decoder, slice);
    }

  if (slice->refId)
    decoder->preFrameNum = slice->frameNum;

  // calculate POC
  decodePOC (decoder, slice);

  if (decoder->recoveryFrameNum == (int)slice->frameNum && decoder->recoveryPoc == 0x7fffffff)
    decoder->recoveryPoc = slice->framePoc;

  if(slice->refId)
    decoder->lastRefPicPoc = slice->framePoc;

  if ((slice->structure == FRAME) || (slice->structure == TopField))
    gettime (&(decoder->startTime));

  picture = decoder->picture = allocPicture (decoder, slice->structure, decoder->width, decoder->height, decoder->widthCr, decoder->heightCr, 1);
  picture->topPoc = slice->topPoc;
  picture->botPoc = slice->botPoc;
  picture->framePoc = slice->framePoc;
  picture->qp = slice->qp;
  picture->sliceQpDelta = slice->sliceQpDelta;
  picture->chromaQpOffset[0] = decoder->activePPS->chromaQpIndexOffset;
  picture->chromaQpOffset[1] = decoder->activePPS->secondChromaQpIndexOffset;
  picture->iCodingType = slice->structure == FRAME ?
    (slice->mbAffFrameFlag? FRAME_MB_PAIR_CODING:FRAME_CODING) : FIELD_CODING;
  picture->layerId = slice->layerId;

  // reset all variables of the error conceal instance before decoding of every frame.
  // here the third parameter should, if perfectly, be equal to the number of slices per frame.
  // using little value is ok, the code will allocate more memory if the slice number is larger
  ercReset (decoder->ercErrorVar, decoder->picSizeInMbs, decoder->picSizeInMbs, picture->sizeX);

  decoder->ercMvPerMb = 0;
  switch (slice->structure ) {
    //{{{
    case TopField:
      picture->poc = slice->topPoc;
      decoder->number *= 2;
      break;
    //}}}
    //{{{
    case BotField:
      picture->poc = slice->botPoc;
      decoder->number = decoder->number * 2 + 1;
      break;
    //}}}
    //{{{
    case FRAME:
      picture->poc = slice->framePoc;
      break;
    //}}}
    //{{{
    default:
      error ("decoder->structure not initialized", 235);
    //}}}
    }

  //decoder->sliceNum=0;
  if (decoder->type > SI_SLICE) {
    setEcFlag (decoder, SE_PTYPE);
    decoder->type = P_SLICE;  // concealed element
    }

  // CAVLC init
  if (decoder->activePPS->entropyCodingModeFlag == (Boolean) CAVLC)
    memset (decoder->nzCoeff[0][0][0], -1, decoder->picSizeInMbs * 48 *sizeof(byte)); // 3 * 4 * 4

  // Set the sliceNum member of each MB to -1, to ensure correct when packet loss occurs
  // TO set sMacroblock Map (mark all MBs as 'have to be concealed')
  if (decoder->sepColourPlaneFlag != 0) {
    for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
      sMacroblock* mb = decoder->mbDataJV[nplane];
      char* intraBlock = decoder->intraBlockJV[nplane];
      for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
        resetMb (mb++);
      memset (decoder->predModeJV[nplane][0], DC_PRED, 16 * decoder->FrameHeightInMbs * decoder->PicWidthInMbs * sizeof(char));
      if (decoder->activePPS->constrainedIntraPredFlag)
        for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
          intraBlock[i] = 1;
      }
    }
  else {
    sMacroblock* mb = decoder->mbData;
    for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
      resetMb (mb++);
    if (decoder->activePPS->constrainedIntraPredFlag)
      for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
        decoder->intraBlock[i] = 1;
    memset (decoder->predMode[0], DC_PRED, 16 * decoder->FrameHeightInMbs * decoder->PicWidthInMbs * sizeof(char));
    }

  picture->sliceType = decoder->type;
  picture->usedForReference = (slice->refId != 0);
  picture->idrFlag = slice->idrFlag;
  picture->noOutputPriorPicFlag = slice->noOutputPriorPicFlag;
  picture->longTermRefFlag = slice->longTermRefFlag;
  picture->adaptiveRefPicBufferingFlag = slice->adaptiveRefPicBufferingFlag;
  picture->decRefPicMarkingBuffer = slice->decRefPicMarkingBuffer;
  slice->decRefPicMarkingBuffer = NULL;

  picture->mbAffFrameFlag = slice->mbAffFrameFlag;
  picture->PicWidthInMbs = decoder->PicWidthInMbs;

  decoder->get_mb_block_pos = picture->mbAffFrameFlag ? get_mb_block_pos_mbaff : get_mb_block_pos_normal;
  decoder->getNeighbour = picture->mbAffFrameFlag ? getAffNeighbour : getNonAffNeighbour;

  picture->picNum = slice->frameNum;
  picture->frameNum = slice->frameNum;
  picture->recoveryFrame = (unsigned int)((int)slice->frameNum == decoder->recoveryFrameNum);
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

  if (decoder->sepColourPlaneFlag != 0) {
    decoder->decPictureJV[0] = decoder->picture;
    decoder->decPictureJV[1] = allocPicture (decoder, (ePicStructure) slice->structure, decoder->width, decoder->height, decoder->widthCr, decoder->heightCr, 1);
    copyDecPicture_JV (decoder, decoder->decPictureJV[1], decoder->decPictureJV[0] );
    decoder->decPictureJV[2] = allocPicture (decoder, (ePicStructure) slice->structure, decoder->width, decoder->height, decoder->widthCr, decoder->heightCr, 1);
    copyDecPicture_JV (decoder, decoder->decPictureJV[2], decoder->decPictureJV[0] );
    }
  }
//}}}
//{{{
static void initPictureDecoding (sDecoder* decoder) {

  int deblockMode = 1;

  if (decoder->curPicSliceNum >= MAX_NUM_SLICES)
    error ("Maximum number of supported slices exceeded, increase MAX_NUM_SLICES", 200);

  sSlice* slice = decoder->sliceList[0];
  if (decoder->nextPPS->valid &&
      (int)decoder->nextPPS->ppsId == slice->ppsId) {
    sPPS pps;
    memcpy (&pps, &(decoder->pps[slice->ppsId]), sizeof (sPPS));
    (decoder->pps[slice->ppsId]).sliceGroupId = NULL;
    makePPSavailable (decoder, decoder->nextPPS->ppsId, decoder->nextPPS);
    memcpy (decoder->nextPPS, &pps, sizeof (sPPS));
    pps.sliceGroupId = NULL;
    }

  useParameterSet (slice);
  if (slice->idrFlag)
    decoder->number = 0;

  decoder->picHeightInMbs = decoder->FrameHeightInMbs / ( 1 + slice->fieldPicFlag );
  decoder->picSizeInMbs = decoder->PicWidthInMbs * decoder->picHeightInMbs;
  decoder->FrameSizeInMbs = decoder->PicWidthInMbs * decoder->FrameHeightInMbs;
  decoder->structure = slice->structure;
  initFmo (decoder, slice);

  updatePicNum (slice);

  initDeblock (decoder, slice->mbAffFrameFlag);
  for (int j = 0; j < decoder->curPicSliceNum; j++) {
    if (decoder->sliceList[j]->DFDisableIdc != 1)
      deblockMode = 0;
    }

  decoder->deblockMode = deblockMode;
  }
//}}}
static void framePostProcessing (sDecoder* decoder) {}
static void fieldPostProcessing (sDecoder* decoder) { decoder->number /= 2; }

//{{{
static void copySliceInfo (sSlice* slice, sOldSlice* oldSliceParam) {

  sDecoder* decoder = slice->decoder;

  oldSliceParam->ppsId = slice->ppsId;
  oldSliceParam->frameNum = slice->frameNum; //decoder->frameNum;
  oldSliceParam->fieldPicFlag = slice->fieldPicFlag; //decoder->fieldPicFlag;

  if (slice->fieldPicFlag)
    oldSliceParam->botFieldFlag = slice->botFieldFlag;

  oldSliceParam->nalRefIdc = slice->refId;
  oldSliceParam->idrFlag = (byte)slice->idrFlag;

  if (slice->idrFlag)
    oldSliceParam->idrPicId = slice->idrPicId;

  if (decoder->activeSPS->picOrderCountType == 0) {
    oldSliceParam->picOrderCountLsb = slice->picOrderCountLsb;
    oldSliceParam->deltaPicOrderCountBot = slice->deletaPicOrderCountBot;
    }

  if (decoder->activeSPS->picOrderCountType == 1) {
    oldSliceParam->deltaPicOrderCount[0] = slice->deltaPicOrderCount[0];
    oldSliceParam->deltaPicOrderCount[1] = slice->deltaPicOrderCount[1];
    }

  oldSliceParam->layerId = slice->layerId;
  }
//}}}
//{{{
static void initSlice (sDecoder* decoder, sSlice* slice) {

  decoder->activeSPS = slice->activeSPS;
  decoder->activePPS = slice->activePPS;

  slice->initLists (slice);
  reorderLists (slice);

  if (slice->structure == FRAME)
    init_mbaff_lists (decoder, slice);

  // update reference flags and set current decoder->refFlag
  if (!((slice->redundantPicCount != 0) && (decoder->prevFrameNum == slice->frameNum)))
    for (int i = 16; i > 0; i--)
      slice->refFlag[i] = slice->refFlag[i-1];
  slice->refFlag[0] = slice->redundantPicCount == 0 ? decoder->isPrimaryOk : decoder->isReduncantOk;

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

  sDecoder* decoder = slice->decoder;
  if ((decoder->sepColourPlaneFlag != 0))
    changePlaneJV (decoder, slice->colourPlaneId, slice);
  else {
    slice->mbData = decoder->mbData;
    slice->picture = decoder->picture;
    slice->siBlock = decoder->siBlock;
    slice->predMode = decoder->predMode;
    slice->intraBlock = decoder->intraBlock;
    }

  if (slice->sliceType == B_SLICE)
    computeColocated (slice, slice->listX);

  if (slice->sliceType != I_SLICE && slice->sliceType != SI_SLICE)
    initCurImg (slice, decoder);

  // loop over macroblocks
  while (endOfSlice == FALSE) {
    sMacroblock* mb;
    startMacroblock (slice, &mb);
    slice->readMacroblock (mb);
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

  sDecoder* decoder = slice->decoder;

  int curHeader = 0;
  sBitstream* curStream = NULL;
  for (;;) {
    sNalu* nalu = decoder->nalu;
    if (!pendingNalu) {
      if (!readNextNalu (decoder, nalu))
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
        if (decoder->recoveryPoint || nalu->unitType == NALU_TYPE_IDR) {
          if (decoder->recoveryPointFound == 0) {
            if (nalu->unitType != NALU_TYPE_IDR) {
              printf ("Warning: Decoding does not start with an IDR picture.\n");
              decoder->nonConformingStream = 1;
              }
            else
              decoder->nonConformingStream = 0;
            }
          decoder->recoveryPointFound = 1;
          }

        if (decoder->recoveryPointFound == 0)
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
        slice->activeSPS = decoder->activeSPS;
        slice->activePPS = decoder->activePPS;
        slice->transform8x8Mode = decoder->activePPS->transform8x8modeFlag;
        slice->chroma444notSeparate = (decoder->activeSPS->chromaFormatIdc == YUV444) &&
                                      (decoder->sepColourPlaneFlag == 0);
        readRestSliceHeader (slice);
        assignQuantParams (slice);

        // if primary slice is replaced with redundant slice, set the correct image type
        if (slice->redundantPicCount && decoder->isPrimaryOk == 0 && decoder->isReduncantOk)
          decoder->picture->sliceType = decoder->type;

        if (isNewPicture (decoder->picture, slice, decoder->oldSlice)) {
          if (decoder->curPicSliceNum == 0)
            initPicture (decoder, slice);
          curHeader = SOP;
          // check zero_byte if it is also the first NAL unit in the access unit
          checkZeroByteVCL (decoder, nalu);
          }
        else
          curHeader = SOS;

        setSliceMethods (slice);

        // Vid->activeSPS, decoder->activePPS, sliceHeader valid
        if (slice->mbAffFrameFlag)
          slice->curMbNum = slice->startMbNum << 1;
        else
          slice->curMbNum = slice->startMbNum;

        if (decoder->activePPS->entropyCodingModeFlag) {
          int ByteStartPosition = curStream->frameBitOffset / 8;
          if ((curStream->frameBitOffset % 8) != 0)
            ++ByteStartPosition;
          arideco_start_decoding (&slice->partitions[0].deCabac, curStream->streamBuffer,
                                  ByteStartPosition, &curStream->readLen);
          }

        decoder->recoveryPoint = 0;
        return curHeader;
        break;
      //}}}
      //{{{
      case NALU_TYPE_DPA:
        if (decoder->recoveryPointFound == 0)
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
        slice->activeSPS = decoder->activeSPS;
        slice->activePPS = decoder->activePPS;
        slice->transform8x8Mode = decoder->activePPS->transform8x8modeFlag;
        slice->chroma444notSeparate = (decoder->activeSPS->chromaFormatIdc == YUV444) &&
                                      (decoder->sepColourPlaneFlag == 0);
        readRestSliceHeader (slice);
        assignQuantParams (slice);

        if (isNewPicture (decoder->picture, slice, decoder->oldSlice)) {
          if (decoder->curPicSliceNum == 0)
            initPicture (decoder, slice);
          curHeader = SOP;
          checkZeroByteVCL (decoder, nalu);
          }
        else
          curHeader = SOS;

        setSliceMethods (slice);

        // From here on, decoder->activeSPS, decoder->activePPS and the slice header are valid
        if (slice->mbAffFrameFlag)
          slice->curMbNum = slice->startMbNum << 1;
        else
          slice->curMbNum = slice->startMbNum;

        // need to read the slice ID, which depends on the value of redundantPicCountPresentFlag

        int slice_id_a = readUeV ("NALU: DP_A slice_id", curStream);

        if (decoder->activePPS->entropyCodingModeFlag)
          error ("data partition with CABAC not allowed", 500);

        // reading next DP
        if (!readNextNalu (decoder, nalu))
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
            if (decoder->activePPS->redundantPicCountPresentFlag)
              readUeV ("NALU dataPartitionB redundantPicCount", curStream);

            // we're finished with DP_B, so let's continue with next DP
            if (!readNextNalu (decoder, nalu))
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

          if (decoder->activePPS->redundantPicCountPresentFlag)
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
        InterpretSEIMessage (nalu->buf, nalu->len, decoder, slice);
        break;
      //}}}
      //{{{
      case NALU_TYPE_PPS:
        processPPS (decoder, nalu);
        break;
      //}}}
      //{{{
      case NALU_TYPE_SPS:
        processSPS (decoder, nalu);
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
void initOldSlice (sOldSlice* oldSliceParam) {

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
void calcFrameNum (sDecoder* decoder, sPicture* p) {

  int psnrPOC = decoder->activeSPS->mb_adaptive_frame_field_flag ? p->poc / (decoder->input.pocScale) :
                                                                    p->poc / (decoder->input.pocScale);
  if (psnrPOC == 0)
    decoder->idrPsnrNum = decoder->gapNumFrame * decoder->refPocGap / (decoder->input.pocScale);

  decoder->psnrNum = imax (decoder->psnrNum, decoder->idrPsnrNum+psnrPOC);
  decoder->frameNum = decoder->idrPsnrNum + psnrPOC;
  }
//}}}
//{{{
void padPicture (sDecoder* decoder, sPicture* picture) {

  padBuf (*picture->imgY, picture->sizeX, picture->sizeY,
           picture->iLumaStride, decoder->iLumaPadX, decoder->iLumaPadY);

  if (picture->chromaFormatIdc != YUV400) {
    padBuf (*picture->imgUV[0], picture->sizeXcr, picture->sizeYcr,
            picture->iChromaStride, decoder->iChromaPadX, decoder->iChromaPadY);
    padBuf (*picture->imgUV[1], picture->sizeXcr, picture->sizeYcr,
            picture->iChromaStride, decoder->iChromaPadX, decoder->iChromaPadY);
    }
  }
//}}}
//{{{
void exitPicture (sDecoder* decoder, sPicture** picture) {

  // return if the last picture has already been finished
  if (*picture == NULL ||
      ((decoder->numDecodedMb != decoder->picSizeInMbs) &&
       ((decoder->yuvFormat != YUV444) || !decoder->sepColourPlaneFlag)))
    return;

  //{{{  error conceal
  frame recfr;
  recfr.decoder = decoder;
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
    ercStartSegment (0, ercSegment, 0 , decoder->ercErrorVar);
    // generate the segments according to the macroblock map
    for (i = 1; i < (int)(*picture)->picSizeInMbs; ++i) {
      if (decoder->mbData[i].eiFlag != decoder->mbData[i-1].eiFlag) {
        ercStopSegment (i-1, ercSegment, 0, decoder->ercErrorVar); //! stop current segment

        // mark current segment as lost or OK
        if(decoder->mbData[i-1].eiFlag)
          ercMarkCurrSegmentLost ((*picture)->sizeX, decoder->ercErrorVar);
        else
          ercMarkCurrSegmentOK ((*picture)->sizeX, decoder->ercErrorVar);

        ++ercSegment;  //! next segment
        ercStartSegment (i, ercSegment, 0 , decoder->ercErrorVar); //! start new segment
        }
      }

    // mark end of the last segment
    ercStopSegment ((*picture)->picSizeInMbs-1, ercSegment, 0, decoder->ercErrorVar);
    if (decoder->mbData[i-1].eiFlag)
      ercMarkCurrSegmentLost ((*picture)->sizeX, decoder->ercErrorVar);
    else
      ercMarkCurrSegmentOK ((*picture)->sizeX, decoder->ercErrorVar);

    // call the right error conceal function depending on the frame type.
    decoder->ercMvPerMb /= (*picture)->picSizeInMbs;
    decoder->ercImg = decoder;
    if ((*picture)->sliceType == I_SLICE || (*picture)->sliceType == SI_SLICE) // I-frame
      ercConcealIntraFrame (decoder, &recfr, (*picture)->sizeX, (*picture)->sizeY, decoder->ercErrorVar);
    else
      ercConcealInterFrame (&recfr, decoder->ercObjectList, (*picture)->sizeX, (*picture)->sizeY, decoder->ercErrorVar, (*picture)->chromaFormatIdc);
    }
  //}}}
  if (!decoder->deblockMode &&
      (decoder->bDeblockEnable & (1 << (*picture)->usedForReference))) {
    //{{{  deblocking for frame or field
    if( (decoder->sepColourPlaneFlag != 0) ) {
      int colourPlaneId = decoder->sliceList[0]->colourPlaneId;
      for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
        decoder->sliceList[0]->colourPlaneId = nplane;
        changePlaneJV (decoder, nplane, NULL );
        deblockPicture (decoder, *picture );
        }
      decoder->sliceList[0]->colourPlaneId = colourPlaneId;
      make_frame_picture_JV(decoder);
      }
    else
      deblockPicture (decoder,* picture);
    }
    //}}}
  else if (decoder->sepColourPlaneFlag != 0)
    make_frame_picture_JV (decoder);

  if ((*picture)->mbAffFrameFlag)
    mbAffPostProc (decoder);

  if (decoder->structure == FRAME)
    framePostProcessing (decoder);
  else
    fieldPostProcessing (decoder);

  if ((*picture)->usedForReference)
    padPicture (decoder, *picture);

  int structure = (*picture)->structure;
  int sliceType = (*picture)->sliceType;
  int framePoc = (*picture)->framePoc;
  int refpic = (*picture)->usedForReference;
  int qp = (*picture)->qp;
  int picNum = (*picture)->picNum;
  int isIdr = (*picture)->idrFlag;
  int chromaFormatIdc = (*picture)->chromaFormatIdc;

  storePictureDpb (decoder->dpbLayer[0], *picture);
  *picture = NULL;

  if (decoder->last_has_mmco_5)
    decoder->preFrameNum = 0;

  if (structure == TopField || structure == FRAME) {
    //{{{  frame type string
    if (sliceType == I_SLICE && isIdr) // IDR picture
      strcpy (decoder->sliceTypeText, "IDR");
    else if (sliceType == I_SLICE) // I picture
      strcpy (decoder->sliceTypeText, " I ");
    else if (sliceType == P_SLICE) // P pictures
      strcpy (decoder->sliceTypeText, " P ");
    else if (sliceType == SP_SLICE) // SP pictures
      strcpy (decoder->sliceTypeText, "SP ");
    else if  (sliceType == SI_SLICE)
      strcpy (decoder->sliceTypeText, "SI ");
    else if (refpic) // stored B pictures
      strcpy (decoder->sliceTypeText, " B ");
    else // B pictures
      strcpy (decoder->sliceTypeText, " b ");
    if (structure == FRAME)
      strncat (decoder->sliceTypeText, "       ",8-strlen(decoder->sliceTypeText));
    }
    //}}}
  else if (structure == BotField) {
    //{{{  frame type string
    if (sliceType == I_SLICE && isIdr) // IDR picture
      strncat (decoder->sliceTypeText, "|IDR", 8-strlen(decoder->sliceTypeText));
    else if (sliceType == I_SLICE) // I picture
      strncat (decoder->sliceTypeText, "| I ", 8-strlen(decoder->sliceTypeText));
    else if (sliceType == P_SLICE) // P pictures
      strncat (decoder->sliceTypeText, "| P ", 8-strlen(decoder->sliceTypeText));
    else if (sliceType == SP_SLICE) // SP pictures
      strncat (decoder->sliceTypeText, "|SP ", 8-strlen(decoder->sliceTypeText));
    else if  (sliceType == SI_SLICE)
      strncat (decoder->sliceTypeText, "|SI ", 8-strlen(decoder->sliceTypeText));
    else if (refpic) // stored B pictures
      strncat (decoder->sliceTypeText, "| B ", 8-strlen(decoder->sliceTypeText));
    else // B pictures
      strncat (decoder->sliceTypeText, "| b ", 8-strlen(decoder->sliceTypeText));
    }
    //}}}
  decoder->sliceTypeText[8] = 0;

  if ((structure == FRAME) || structure == BotField) {
    gettime (&(decoder->endTime));
    printf ("%5d %s poc:%4d pic:%3d qp:%2d %dms\n",
            decoder->frameNum, decoder->sliceTypeText, framePoc, picNum, qp,
            (int)timenorm (timediff (&(decoder->startTime), &(decoder->endTime))));

    // I or P pictures ?
    if (sliceType == I_SLICE || sliceType == SI_SLICE || sliceType == P_SLICE || refpic)
      ++(decoder->number);

    ++(decoder->gapNumFrame);
    }
  }
//}}}

//{{{
int decodeFrame (sDecoder* decoder) {

  int ret = 0;

  decoder->curPicSliceNum = 0;
  decoder->numSlicesDecoded = 0;
  decoder->numDecodedMb = 0;

  sSlice* slice = NULL;
  sSlice** sliceList = decoder->sliceList;
  int curHeader = 0;
  if (decoder->newFrame) {
    if (decoder->nextPPS->valid) {
      //{{{  use next PPS
      makePPSavailable (decoder, decoder->nextPPS->ppsId, decoder->nextPPS);
      decoder->nextPPS->valid = 0;
      }
      //}}}
    // get firstSlice from slice;
    slice = sliceList[decoder->curPicSliceNum];
    sliceList[decoder->curPicSliceNum] = decoder->nextSlice;
    decoder->nextSlice = slice;
    slice = sliceList[decoder->curPicSliceNum];
    useParameterSet (slice);
    initPicture (decoder, slice);
    decoder->curPicSliceNum++;
    curHeader = SOS;
    }

  while ((curHeader != SOP) && (curHeader != EOS)) {
    //{{{  no pending slices
    if (!sliceList[decoder->curPicSliceNum])
      sliceList[decoder->curPicSliceNum] = allocSlice (decoder);

    slice = sliceList[decoder->curPicSliceNum];
    slice->decoder = decoder;
    slice->dpb = decoder->dpbLayer[0]; //set default value;
    slice->nextHeader = -8888;
    slice->numDecodedMb = 0;
    slice->coefCount = -1;
    slice->pos = 0;
    slice->isResetCoef = FALSE;
    slice->isResetCoefCr = FALSE;
    curHeader = readNewSlice (slice);
    slice->curHeader = curHeader;

    // error tracking of primary and redundant slices.
    errorTracking (decoder, slice);
    //{{{  manage primary and redundant slices
    // If primary and redundant are received and primary is correct
    //   discard the redundant
    // else,
    //   primary slice will be replaced with redundant slice.
    if (slice->frameNum == decoder->prevFrameNum &&
        slice->redundantPicCount != 0 &&
        decoder->isPrimaryOk != 0 &&
        curHeader != EOS)
      continue;

    if ((curHeader != SOP && curHeader != EOS) ||
        (decoder->curPicSliceNum == 0 && curHeader == SOP)) {
       slice->curSliceNum = (short)decoder->curPicSliceNum;
       decoder->picture->maxSliceId =
         (short)imax (slice->curSliceNum, decoder->picture->maxSliceId);
       if (decoder->curPicSliceNum > 0) {
         copyPOC (*sliceList, slice);
         sliceList[decoder->curPicSliceNum-1]->endMbNumPlus1 = slice->startMbNum;
         }

       decoder->curPicSliceNum++;
       if (decoder->curPicSliceNum >= decoder->numSlicesAllocated) {
         sSlice** tmpSliceList = (sSlice**)realloc (
           decoder->sliceList, (decoder->numSlicesAllocated + MAX_NUM_DECSLICES) * sizeof(sSlice*));
         if (!tmpSliceList) {
           tmpSliceList = calloc ((decoder->numSlicesAllocated + MAX_NUM_DECSLICES), sizeof(sSlice*));
           memcpy (tmpSliceList, decoder->sliceList, decoder->curPicSliceNum * sizeof(sSlice*));
           free (decoder->sliceList);
           sliceList = decoder->sliceList = tmpSliceList;
           }
         else {
           sliceList = decoder->sliceList = tmpSliceList;
           memset (decoder->sliceList + decoder->curPicSliceNum, 0, sizeof(sSlice*) * MAX_NUM_DECSLICES);
           }
         decoder->numSlicesAllocated += MAX_NUM_DECSLICES;
        }
      curHeader = SOS;
      }
    else {
      if (sliceList[decoder->curPicSliceNum-1]->mbAffFrameFlag)
        sliceList[decoder->curPicSliceNum-1]->endMbNumPlus1 = decoder->FrameSizeInMbs / 2;
      else
        sliceList[decoder->curPicSliceNum-1]->endMbNumPlus1 = decoder->FrameSizeInMbs /
                                                                 (1 + sliceList[decoder->curPicSliceNum-1]->fieldPicFlag);
      decoder->newFrame = 1;
      slice->curSliceNum = 0;

      // keep it in currentslice;
      sliceList[decoder->curPicSliceNum] = decoder->nextSlice;
      decoder->nextSlice = slice;
      }
    //}}}

    copySliceInfo (slice, decoder->oldSlice);
    }
    //}}}

  // decode slices
  ret = curHeader;
  initPictureDecoding (decoder);
  for (int sliceNum = 0; sliceNum < decoder->curPicSliceNum; sliceNum++) {
    //{{{  decode slice
    slice = sliceList[sliceNum];
    curHeader = slice->curHeader;
    initSlice (decoder, slice);

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

    decoder->numSlicesDecoded++;
    decoder->numDecodedMb += slice->numDecodedMb;
    decoder->ercMvPerMb += slice->ercMvPerMb;
    }
    //}}}

  exitPicture (decoder, &decoder->picture);
  decoder->prevFrameNum = sliceList[0]->frameNum;

  return ret;
  }
//}}}
