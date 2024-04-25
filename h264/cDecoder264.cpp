//{{{  includes
#include <thread>
#include "global.h"
#include "memory.h"

#include "sCabacDecode.h"
#include "cabac.h"
#include "erc.h"
#include "loopfilter.h"
#include "macroblock.h"
#include "mcPred.h"
#include "nalu.h"
#include "sei.h"

#include "../common/cLog.h"

using namespace std;
//}}}
namespace {
  const int kSubWidthC[]= { 1, 2, 2, 1 };
  const int kSubHeightC[]= { 1, 2, 1, 1 };
  //{{{
  void ercWriteMbModeMv (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;

    int curMbNum = mb->mbIndexX;
    sPicture* picture = decoder->picture;
    int mbx = xPosMB (curMbNum, picture->sizeX), mby = yPosMB (curMbNum, picture->sizeX);
    sObjectBuffer* curRegion = decoder->ercObjectList + (curMbNum << 2);

    if (decoder->coding.sliceType == cSlice::eB) {
      //{{{  B-frame
      for (int i = 0; i < 4; ++i) {
        int ii = 4*mbx + (i%2) * 2;
        int jj = 4*mby + (i/2) * 2;

        sObjectBuffer* region = curRegion + i;
        region->regionMode = (mb->mbType == I16MB  ? REGMODE_INTRA :
                                mb->b8mode[i] == IBLOCK ? REGMODE_INTRA_8x8 :
                                  REGMODE_INTER_PRED_8x8);

        if (mb->mbType == I16MB || mb->b8mode[i] == IBLOCK) {
          // INTRA
          region->mv[0] = 0;
          region->mv[1] = 0;
          region->mv[2] = 0;
          }
        else {
          int idx = (picture->mvInfo[jj][ii].refIndex[0] < 0) ? 1 : 0;
          region->mv[0] = (picture->mvInfo[jj][ii].mv[idx].mvX +
                           picture->mvInfo[jj][ii+1].mv[idx].mvX +
                           picture->mvInfo[jj+1][ii].mv[idx].mvX +
                           picture->mvInfo[jj+1][ii+1].mv[idx].mvX + 2)/4;

          region->mv[1] = (picture->mvInfo[jj][ii].mv[idx].mvY +
                           picture->mvInfo[jj][ii+1].mv[idx].mvY +
                           picture->mvInfo[jj+1][ii].mv[idx].mvY +
                           picture->mvInfo[jj+1][ii+1].mv[idx].mvY + 2)/4;
          mb->slice->ercMvPerMb += iabs (region->mv[0]) + iabs (region->mv[1]);

          region->mv[2] = (picture->mvInfo[jj][ii].refIndex[idx]);
          }
        }
      }
      //}}}
    else {
      //{{{  non-B frame
      for (int i = 0; i < 4; ++i) {
        sObjectBuffer* region = curRegion + i;
        region->regionMode = (mb->mbType ==I16MB  ? REGMODE_INTRA :
                                mb->b8mode[i] == IBLOCK ? REGMODE_INTRA_8x8  :
                                  mb->b8mode[i] == 0 ? REGMODE_INTER_COPY :
                                    mb->b8mode[i] == 1 ? REGMODE_INTER_PRED :
                                      REGMODE_INTER_PRED_8x8);

        if (!mb->b8mode[i] || mb->b8mode[i] == IBLOCK) {
          // INTRA OR COPY
          region->mv[0] = 0;
          region->mv[1] = 0;
          region->mv[2] = 0;
          }
        else {
          int ii = 4*mbx + (i & 0x01)*2;// + BLOCK_SIZE;
          int jj = 4*mby + (i >> 1  )*2;
          if (mb->b8mode[i]>=5 && mb->b8mode[i] <= 7) {
            // SMALL BLOCKS
            region->mv[0] = (picture->mvInfo[jj][ii].mv[LIST_0].mvX + picture->mvInfo[jj][ii + 1].mv[LIST_0].mvX + picture->mvInfo[jj + 1][ii].mv[LIST_0].mvX + picture->mvInfo[jj + 1][ii + 1].mv[LIST_0].mvX + 2)/4;
            region->mv[1] = (picture->mvInfo[jj][ii].mv[LIST_0].mvY + picture->mvInfo[jj][ii + 1].mv[LIST_0].mvY + picture->mvInfo[jj + 1][ii].mv[LIST_0].mvY + picture->mvInfo[jj + 1][ii + 1].mv[LIST_0].mvY + 2)/4;
            }
          else {
            // 16x16, 16x8, 8x16, 8x8
            region->mv[0] = picture->mvInfo[jj][ii].mv[LIST_0].mvX;
            region->mv[1] = picture->mvInfo[jj][ii].mv[LIST_0].mvY;
            }

          mb->slice->ercMvPerMb += iabs (region->mv[0]) + iabs (region->mv[1]);
          region->mv[2] = picture->mvInfo[jj][ii].refIndex[LIST_0];
          }
        }
      }
      //}}}
    }
  //}}}

  //{{{
  void copyDecPictureJV (sPicture* dst, sPicture* src) {

    dst->poc = src->poc;
    dst->topPoc = src->topPoc;
    dst->botPoc = src->botPoc;
    dst->framePoc = src->framePoc;

    dst->qp = src->qp;
    dst->sliceQpDelta = src->sliceQpDelta;
    dst->chromaQpOffset[0] = src->chromaQpOffset[0];
    dst->chromaQpOffset[1] = src->chromaQpOffset[1];

    dst->sliceType = src->sliceType;
    dst->usedForRef = src->usedForRef;
    dst->isIDR = src->isIDR;
    dst->noOutputPriorPicFlag = src->noOutputPriorPicFlag;
    dst->longTermRefFlag = src->longTermRefFlag;
    dst->adaptRefPicBufFlag = src->adaptRefPicBufFlag;
    dst->decRefPicMarkBuffer = src->decRefPicMarkBuffer;
    dst->mbAffFrame = src->mbAffFrame;
    dst->picWidthMbs = src->picWidthMbs;
    dst->picNum  = src->picNum;
    dst->frameNum = src->frameNum;
    dst->recoveryFrame = src->recoveryFrame;
    dst->codedFrame = src->codedFrame;
    dst->chromaFormatIdc = src->chromaFormatIdc;
    dst->frameMbOnly = src->frameMbOnly;
    dst->hasCrop = src->hasCrop;
    dst->cropLeft = src->cropLeft;
    dst->cropRight = src->cropRight;
    dst->cropTop = src->cropTop;
    dst->cropBot = src->cropBot;
    }
  //}}}
  //{{{
  void updateMbAff (sPixel** pixel, sPixel (*temp)[16], int x0, int width, int height) {

    sPixel (*temp_evn)[16] = temp;
    sPixel (*temp_odd)[16] = temp + height;
    sPixel** temp_img = pixel;

    for (int y = 0; y < 2 * height; ++y)
      memcpy (*temp++, (*temp_img++ + x0), width * sizeof(sPixel));

    for (int y = 0; y < height; ++y) {
      memcpy ((*pixel++ + x0), *temp_evn++, width * sizeof(sPixel));
      memcpy ((*pixel++ + x0), *temp_odd++, width * sizeof(sPixel));
      }
    }
  //}}}
  //{{{
  void padBuf (sPixel* pixel, int width, int height, int stride, int padx, int pady) {

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
  void copyCropped (uint8_t* buf, sPixel** imgX, int sizeX, int sizeY, int symbolSizeInBytes,
                    int cropLeft, int cropRight, int cropTop, int cropBot, int outStride) {

    if (cropLeft || cropRight) {
      for (int i = cropTop; i < sizeY - cropBot; i++) {
        int pos = ((i - cropTop) * outStride) + cropLeft;
        memcpy (buf + pos, imgX[i], sizeX - cropRight - cropLeft);
        }
      }
    else {
      buf += cropTop * outStride;
      for (int i = 0; i < sizeY - cropBot - cropTop; i++) {
        memcpy (buf, imgX[i], sizeX);
        buf += outStride;
        }
      }
    }
  //}}}
  //{{{
  sDecodedPic* allocDecodedPicture (sDecodedPic* decodedPic) {

    sDecodedPic* prevDecodedPicture = NULL;
    while (decodedPic && decodedPic->ok) {
      prevDecodedPicture = decodedPic;
      decodedPic = decodedPic->next;
      }

    if (!decodedPic) {
      decodedPic = (sDecodedPic*)calloc (1, sizeof(*decodedPic));
      prevDecodedPicture->next = decodedPic;
      }

    return decodedPic;
    }
  //}}}
  //{{{
  void freeDecodedPictures (sDecodedPic* decodedPic) {

    while (decodedPic) {
      sDecodedPic* nextDecodedPic = decodedPic->next;
      if (decodedPic->yBuf) {
        free (decodedPic->yBuf);
        decodedPic->yBuf = NULL;
        decodedPic->uBuf = NULL;
        decodedPic->vBuf = NULL;
        }

      free (decodedPic);
      decodedPic = nextDecodedPic;
     }
    }
  //}}}
  }

// static
//{{{
cDecoder264* cDecoder264::open (sParam* param, uint8_t* chunk, size_t chunkSize) {

  // alloc decoder
  cDecoder264* decoder = new (cDecoder264);

  initTime();

  // init param
  memcpy (&decoder->param, param, sizeof(sParam));
  decoder->concealMode = param->concealMode;

  // init annexB, nalu
  decoder->annexB = new cAnnexB();
  decoder->annexB->open (chunk, chunkSize);
  decoder->nalu = new cNalu();

  // init slice
  decoder->sliceList = (cSlice**)calloc (cSlice::kMaxNumSlices, sizeof(cSlice*));
  decoder->numAllocatedSlices = cSlice::kMaxNumSlices;
  decoder->oldSlice = (sOldSlice*)malloc (sizeof(sOldSlice));
  decoder->coding.sliceType = cSlice::eI;
  decoder->recoveryPoc = 0x7fffffff;
  decoder->deblockEnable = 0x3;
  decoder->pendingOutPicStructure = eFrame;

  decoder->coding.lumaPadX = MCBUF_LUMA_PAD_X;
  decoder->coding.lumaPadY = MCBUF_LUMA_PAD_Y;
  decoder->coding.chromaPadX = MCBUF_CHROMA_PAD_X;
  decoder->coding.chromaPadY = MCBUF_CHROMA_PAD_Y;

  decoder->dpb.decoder = decoder;

  // alloc output
  decoder->outBuffer = new cFrameStore();
  decoder->pendingOut = (sPicture*)calloc (sizeof(sPicture), 1);
  decoder->pendingOut->imgUV = NULL;
  decoder->pendingOut->imgY = NULL;

  decoder->outDecodedPics = (sDecodedPic*)calloc (1, sizeof(sDecodedPic));

  // global only for vlc
  gDecoder = decoder;
  return decoder;
  }
//}}}
//{{{
void cDecoder264::error (const string& text) {

  cLog::log (LOGERROR, text);
  if (cDecoder264::gDecoder)
    cDecoder264::gDecoder->dpb.flush();

  this_thread::sleep_for (5s);
  exit (0);
  }
//}}}

// public
//{{{
cDecoder264::~cDecoder264() {

  free (annexB);
  free (oldSlice);
  free (nextSlice);

  if (sliceList) {
    for (int i = 0; i < numAllocatedSlices; i++)
      if (sliceList[i])
        free (sliceList[i]);
    free (sliceList);
    }

  free (nalu);

  freeDecodedPictures (outDecodedPics);
  }
//}}}

//{{{
void cDecoder264::reset_ecFlags () {

  for (int i = 0; i < SE_MAX_ELEMENTS; i++)
    ecFlag[i] = NO_EC;
  }
//}}}
//{{{
int cDecoder264::setEcFlag (int se) {

  switch (se) {
    case SE_HEADER :
      ecFlag[SE_HEADER] = EC_REQ;
    case SE_PTYPE :
      ecFlag[SE_PTYPE] = EC_REQ;
    case SE_MBTYPE :
      ecFlag[SE_MBTYPE] = EC_REQ;
    //{{{
    case SE_REFFRAME :
      ecFlag[SE_REFFRAME] = EC_REQ;
      ecFlag[SE_MVD] = EC_REQ; // set all motion vectors to zero length
      se = SE_CBP_INTER;      // conceal also Inter texture elements
      break;
    //}}}

    //{{{
    case SE_INTRAPREDMODE :
      ecFlag[SE_INTRAPREDMODE] = EC_REQ;
      se = SE_CBP_INTRA;      // conceal also Intra texture elements
      break;
    //}}}
    //{{{
    case SE_MVD :
      ecFlag[SE_MVD] = EC_REQ;
      se = SE_CBP_INTER;      // conceal also Inter texture elements
      break;
    //}}}
    //{{{
    default:
      break;
    //}}}
    }

  switch (se) {
    case SE_CBP_INTRA :
      ecFlag[SE_CBP_INTRA] = EC_REQ;
    case SE_LUM_DC_INTRA :
      ecFlag[SE_LUM_DC_INTRA] = EC_REQ;
    case SE_CHR_DC_INTRA :
      ecFlag[SE_CHR_DC_INTRA] = EC_REQ;
    case SE_LUM_AC_INTRA :
      ecFlag[SE_LUM_AC_INTRA] = EC_REQ;
    //{{{
    case SE_CHR_AC_INTRA :
      ecFlag[SE_CHR_AC_INTRA] = EC_REQ;
      break;
    //}}}

    case SE_CBP_INTER :
      ecFlag[SE_CBP_INTER] = EC_REQ;
    case SE_LUM_DC_INTER :
      ecFlag[SE_LUM_DC_INTER] = EC_REQ;
    case SE_CHR_DC_INTER :
      ecFlag[SE_CHR_DC_INTER] = EC_REQ;
    case SE_LUM_AC_INTER :
      ecFlag[SE_LUM_AC_INTER] = EC_REQ;
    //{{{
    case SE_CHR_AC_INTER :
      ecFlag[SE_CHR_AC_INTER] = EC_REQ;
      break;
    //}}}

    //{{{
    case SE_DELTA_QUANT_INTER :
      ecFlag[SE_DELTA_QUANT_INTER] = EC_REQ;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTRA :
      ecFlag[SE_DELTA_QUANT_INTRA] = EC_REQ;
      break;
    //}}}
    //{{{
    default:
      break;
    }
    //}}}

  return EC_REQ;
  }
//}}}
//{{{
int cDecoder264::getConcealElement (sSyntaxElement* se) {

  if (ecFlag[se->type] == NO_EC)
    return NO_EC;

  switch (se->type) {
    //{{{
    case SE_HEADER :
      se->len = 31;
      se->inf = 0; // Picture Header
      break;
    //}}}

    case SE_PTYPE : // inter_img_1
    case SE_MBTYPE : // set COPY_MB
    //{{{
    case SE_REFFRAME :
      se->len = 1;
      se->inf = 0;
      break;
    //}}}

    case SE_INTRAPREDMODE :
    //{{{
    case SE_MVD :
      se->len = 1;
      se->inf = 0;  // set vector to zero length
      break;
    //}}}

    case SE_LUM_DC_INTRA :
    case SE_CHR_DC_INTRA :
    case SE_LUM_AC_INTRA :
    //{{{
    case SE_CHR_AC_INTRA :
      se->len = 1;
      se->inf = 0;  // return EOB
      break;
    //}}}

    case SE_LUM_DC_INTER :
    case SE_CHR_DC_INTER :
    case SE_LUM_AC_INTER :
    //{{{
    case SE_CHR_AC_INTER :
      se->len = 1;
      se->inf = 0;  // return EOB
      break;
    //}}}

    //{{{
    case SE_CBP_INTRA :
      se->len = 5;
      se->inf = 0; // codenumber 3 <=> no CBP information for INTRA images
      break;
    //}}}
    //{{{
    case SE_CBP_INTER :
      se->len = 1;
      se->inf = 0; // codenumber 1 <=> no CBP information for INTER images
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTER:
      se->len = 1;
      se->inf = 0;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTRA:
      se->len = 1;
      se->inf = 0;
      break;
    //}}}
    //{{{
    default:
      break;
    //}}}
    }

  return EC_REQ;
  }
//}}}
//{{{
int cDecoder264::fmoGetNextMbIndex (int curMbIndex) {

  int SliceGroup = fmoGetSliceGroupId (curMbIndex);

  while ((++curMbIndex < (int)picSizeInMbs) &&
         (mbToSliceGroupMap [curMbIndex] != SliceGroup));

  if (curMbIndex >= (int)picSizeInMbs)
    return -1;    // No further MB in this slice (could be end of picture)
  else
    return curMbIndex;
  }
//}}}
//{{{
void cDecoder264::padPicture (sPicture* picture) {

  padBuf (*picture->imgY, picture->sizeX, picture->sizeY,
           picture->lumaStride, coding.lumaPadX, coding.lumaPadY);

  if (picture->chromaFormatIdc != YUV400) {
    padBuf (*picture->imgUV[0], picture->sizeXcr, picture->sizeYcr,
            picture->chromaStride, coding.chromaPadX, coding.chromaPadY);
    padBuf (*picture->imgUV[1], picture->sizeXcr, picture->sizeYcr,
            picture->chromaStride, coding.chromaPadX, coding.chromaPadY);
    }
  }
//}}}
//{{{
void cDecoder264::decodePOC (cSlice* slice) {

  uint32_t maxPicOrderCntLsb = 1 << (activeSps->log2maxPocLsbMinus4+4);

  switch (activeSps->pocType) {
    //{{{
    case 0: // POC MODE 0
      // 1st
      if (slice->isIDR) {
        prevPocMsb = 0;
        prevPocLsb = 0;
        }
      else if (lastHasMmco5) {
        if (lastPicBotField) {
          prevPocMsb = 0;
          prevPocLsb = 0;
          }
        else {
          prevPocMsb = 0;
          prevPocLsb = slice->topPoc;
          }
        }

      // Calculate the MSBs of current picture
      if (slice->picOrderCountLsb < prevPocLsb  &&
          (prevPocLsb - slice->picOrderCountLsb) >= maxPicOrderCntLsb/2)
        slice->PicOrderCntMsb = prevPocMsb + maxPicOrderCntLsb;
      else if (slice->picOrderCountLsb > prevPocLsb &&
               (slice->picOrderCountLsb - prevPocLsb) > maxPicOrderCntLsb/2)
        slice->PicOrderCntMsb = prevPocMsb - maxPicOrderCntLsb;
      else
        slice->PicOrderCntMsb = prevPocMsb;

      // 2nd
      if (slice->fieldPic == 0) {
        // frame pixelPos
        slice->topPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
        slice->botPoc = slice->topPoc + slice->deltaPicOrderCountBot;
        slice->thisPoc = slice->framePoc = (slice->topPoc < slice->botPoc) ? slice->topPoc : slice->botPoc;
        }
      else if (!slice->botField) // top field
        slice->thisPoc= slice->topPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
      else // bottom field
        slice->thisPoc= slice->botPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
      slice->framePoc = slice->thisPoc;

      thisPoc = slice->thisPoc;
      previousFrameNum = slice->frameNum;

      if (slice->refId) {
        prevPocLsb = slice->picOrderCountLsb;
        prevPocMsb = slice->PicOrderCntMsb;
        }

      break;
    //}}}
    //{{{
    case 1: // POC MODE 1
      // 1st
      if (slice->isIDR) {
        // first pixelPos of IDRGOP,
        frameNumOffset = 0;
        if (slice->frameNum)
          error ("frameNum nonZero IDR picture");
        }
      else {
        if (lastHasMmco5) {
          previousFrameNumOffset = 0;
          previousFrameNum = 0;
          }
        if (slice->frameNum<previousFrameNum) // not first pixelPos of IDRGOP
          frameNumOffset = previousFrameNumOffset + coding.maxFrameNum;
        else
          frameNumOffset = previousFrameNumOffset;
        }

      // 2nd
      if (activeSps->numRefFramesPocCycle)
        slice->AbsFrameNum = frameNumOffset + slice->frameNum;
      else
        slice->AbsFrameNum = 0;
      if (!slice->refId && (slice->AbsFrameNum > 0))
        slice->AbsFrameNum--;

      // 3rd
      expectedDeltaPerPocCycle = 0;
      if (activeSps->numRefFramesPocCycle)
        for (uint32_t i = 0; i < activeSps->numRefFramesPocCycle; i++)
          expectedDeltaPerPocCycle += activeSps->offsetForRefFrame[i];

      if (slice->AbsFrameNum) {
        pocCycleCount = (slice->AbsFrameNum-1) / activeSps->numRefFramesPocCycle;
        frameNumPocCycle = (slice->AbsFrameNum-1) % activeSps->numRefFramesPocCycle;
        expectedPOC =
          pocCycleCount*expectedDeltaPerPocCycle;
        for (int i = 0; i <= frameNumPocCycle; i++)
          expectedPOC += activeSps->offsetForRefFrame[i];
        }
      else
        expectedPOC = 0;

      if (!slice->refId)
        expectedPOC += activeSps->offsetNonRefPic;

      if (slice->fieldPic == 0) {
        // frame pixelPos
        slice->topPoc = expectedPOC + slice->deltaPicOrderCount[0];
        slice->botPoc = slice->topPoc + activeSps->offsetTopBotField + slice->deltaPicOrderCount[1];
        slice->thisPoc = slice->framePoc = (slice->topPoc < slice->botPoc) ? slice->topPoc : slice->botPoc;
        }
      else if (!slice->botField) // top field
        slice->thisPoc = slice->topPoc = expectedPOC + slice->deltaPicOrderCount[0];
      else // bottom field
        slice->thisPoc = slice->botPoc = expectedPOC + activeSps->offsetTopBotField + slice->deltaPicOrderCount[0];
      slice->framePoc=slice->thisPoc;

      previousFrameNum = slice->frameNum;
      previousFrameNumOffset = frameNumOffset;
      break;
    //}}}
    //{{{
    case 2: // POC MODE 2
      if (slice->isIDR) {
        // IDR picture, first pixelPos of IDRGOP,
        frameNumOffset = 0;
        slice->thisPoc = slice->framePoc = slice->topPoc = slice->botPoc = 0;
        if (slice->frameNum)
          error ("frameNum not equal to zero in IDR picture");
        }
      else {
        if (lastHasMmco5) {
          previousFrameNum = 0;
          previousFrameNumOffset = 0;
          }

        if (slice->frameNum<previousFrameNum)
          frameNumOffset = previousFrameNumOffset + coding.maxFrameNum;
        else
          frameNumOffset = previousFrameNumOffset;

        slice->AbsFrameNum = frameNumOffset+slice->frameNum;
        if (!slice->refId)
          slice->thisPoc = 2*slice->AbsFrameNum - 1;
        else
          slice->thisPoc = 2*slice->AbsFrameNum;

        if (slice->fieldPic == 0)
          slice->topPoc = slice->botPoc = slice->framePoc = slice->thisPoc;
        else if (!slice->botField)
          slice->topPoc = slice->framePoc = slice->thisPoc;
        else
          slice->botPoc = slice->framePoc = slice->thisPoc;
        }

      previousFrameNum = slice->frameNum;
      previousFrameNumOffset = frameNumOffset;
      break;
    //}}}
    //{{{
    default:
      error ("unknown POC type");
      break;
    //}}}
    }
  }
//}}}
//{{{
void cDecoder264::changePlaneJV (int nplane, cSlice* slice) {

  mbData = mbDataJV[nplane];
  picture  = decPictureJV[nplane];
  siBlock = siBlockJV[nplane];
  predMode = predModeJV[nplane];
  intraBlock = intraBlockJV[nplane];

  if (slice) {
    slice->mbData = mbDataJV[nplane];
    slice->picture  = decPictureJV[nplane];
    slice->siBlock = siBlockJV[nplane];
    slice->predMode = predModeJV[nplane];
    slice->intraBlock = intraBlockJV[nplane];
    }
  }
//}}}

//{{{
void cDecoder264::directOutput (sPicture* picture) {

  switch (picture->picStructure) {
    case eFrame:
      flushDirectOutput();
      writePicture (picture, eFrame);
      sPicture::freePicture (picture);
      return;

    case eTopField:
      if (outBuffer->isUsed & 1)
        flushDirectOutput();
      outBuffer->topField = picture;
      outBuffer->isUsed |= 1;
      break;

    case eBotField:
      if (outBuffer->isUsed & 2)
        flushDirectOutput();
      outBuffer->botField = picture;
      outBuffer->isUsed |= 2;
      break;
    }

  if (outBuffer->isUsed == 3) {
    // output both fields
    outBuffer->dpbCombineField (this);
    writePicture (outBuffer->frame, eFrame);

    sPicture::freePicture (outBuffer->frame);
    sPicture::freePicture (outBuffer->topField);
    sPicture::freePicture (outBuffer->botField);

    outBuffer->isUsed = 0;
    }
  }
//}}}
//{{{
void cDecoder264::writeStoredFrame (cFrameStore* frameStore) {

  // make sure no direct output field is pending
  flushDirectOutput();

  if (frameStore->isUsed < 3)
    writeUnpairedField (frameStore);
  else {
    if (frameStore->recoveryFrame)
      recoveryFlag = 1;
    if ((!nonConformingStream) || recoveryFlag)
      writePicture (frameStore->frame, eFrame);
    }

  frameStore->isOutput = 1;
  }
//}}}

//{{{
sDecodedPic* cDecoder264::decodeOneFrame (int& result) {

  clearDecodedPics();

  int decodeFrameResult = decodeFrame();
  if (decodeFrameResult == eSOP)
    result = DEC_SUCCEED;
  else if (decodeFrameResult == eEOS)
    result = DEC_EOS;
  else
    result |= DEC_ERRMASK;

  return outDecodedPics;
  }
//}}}
//{{{
sDecodedPic* cDecoder264::finish() {

  clearDecodedPics();
  dpb.flush();

  annexB->reset();

  newFrame = false;
  prevFrameNum = 0;

  return outDecodedPics;
  }
//}}}
//{{{
void cDecoder264::close() {

  closeFmo();
  freeLayerBuffers();
  freeGlobalBuffers();

  ercClose (this, ercErrorVar);

  for (uint32_t i = 0; i < kMaxPps; i++) {
    if (pps[i].ok && pps[i].sliceGroupId)
      free (pps[i].sliceGroupId);
    pps[i].ok = false;
    }

  dpb.freeResources();

  // free output
  delete outBuffer;
  flushPendingOut();
  free (pendingOut);

  gDecoder = NULL;
  }
//}}}

// private
//{{{
void cDecoder264::clearDecodedPics() {

  // find the head first;
  sDecodedPic* prevDecodedPicture = NULL;
  sDecodedPic* decodedPic = outDecodedPics;
  while (decodedPic && !decodedPic->ok) {
    prevDecodedPicture = decodedPic;
    decodedPic = decodedPic->next;
    }

  if (decodedPic && (decodedPic != outDecodedPics)) {
    // move all nodes before decodedPic to the end;
    sDecodedPic* decodedPictureTail = decodedPic;
    while (decodedPictureTail->next)
      decodedPictureTail = decodedPictureTail->next;

    decodedPictureTail->next = outDecodedPics;
    outDecodedPics = decodedPic;
    prevDecodedPicture->next = NULL;
    }
  }
//}}}
//{{{
void cDecoder264::initPictureDecode() {

  deblockMode = 1;

  if (picSliceIndex >= MAX_NUM_SLICES)
    error ("initPictureDecode - MAX_NUM_SLICES exceeded");

  cSlice* slice = sliceList[0];
  useParameterSet (slice);

  if (slice->isIDR)
    idrFrameNum = 0;

  picHeightInMbs = coding.frameHeightMbs / (1 + slice->fieldPic);
  picSizeInMbs = coding.picWidthMbs * picHeightInMbs;
  coding.frameSizeMbs = coding.picWidthMbs * coding.frameHeightMbs;
  coding.picStructure = slice->picStructure;
  initFmo (slice);

  slice->updatePicNum();

  initDeblock (this, slice->mbAffFrame);
  for (int j = 0; j < picSliceIndex; j++)
    if (sliceList[j]->deblockFilterDisableIdc != 1)
      deblockMode = 0;
  }
//}}}
//{{{
void cDecoder264::initGlobalBuffers() {

  // alloc coding
  if (coding.isSeperateColourPlane) {
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!mbDataJV[i])
        mbDataJV[i] = (sMacroBlock*)calloc (coding.frameSizeMbs, sizeof(sMacroBlock));
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!intraBlockJV[i])
        intraBlockJV[i] = (char*)calloc (coding.frameSizeMbs, sizeof(char));
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!predModeJV[i])
       getMem2D (&predModeJV[i], 4*coding.frameHeightMbs, 4*coding.picWidthMbs);
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!siBlockJV[i])
        getMem2Dint (&siBlockJV[i], coding.frameHeightMbs, coding.picWidthMbs);
    }
  else {
    if (!mbData)
      mbData = (sMacroBlock*)calloc (coding.frameSizeMbs, sizeof(sMacroBlock));
    if (!intraBlock)
      intraBlock = (char*)calloc (coding.frameSizeMbs, sizeof(char));
    if (!predMode)
      getMem2D (&predMode, 4*coding.frameHeightMbs, 4*coding.picWidthMbs);
    if (!siBlock)
      getMem2Dint (&siBlock, coding.frameHeightMbs, coding.picWidthMbs);
    }

  // alloc picPos
  if (!picPos) {
    picPos = (sBlockPos*)calloc (coding.frameSizeMbs + 1, sizeof(sBlockPos));
    sBlockPos* blockPos = picPos;
    for (uint32_t i = 0; i < coding.frameSizeMbs+1; ++i) {
      blockPos[i].x = (int16_t)(i % coding.picWidthMbs);
      blockPos[i].y = (int16_t)(i / coding.picWidthMbs);
      }
    }

  // alloc cavlc
  if (!nzCoeff)
    getMem4D (&nzCoeff, coding.frameSizeMbs, 3, BLOCK_SIZE, BLOCK_SIZE);

  // alloc quant
  int bitDepth_qp_scale = imax (coding.bitDepthLumaQpScale, coding.bitDepthChromaQpScale);
  if (!qpPerMatrix)
    qpPerMatrix = (int*)malloc ((MAX_QP + 1 + bitDepth_qp_scale) * sizeof(int));
  if (!qpRemMatrix)
    qpRemMatrix = (int*)malloc ((MAX_QP + 1 + bitDepth_qp_scale) * sizeof(int));
  for (int i = 0; i < MAX_QP + bitDepth_qp_scale + 1; i++) {
    qpPerMatrix[i] = i / 6;
    qpRemMatrix[i] = i % 6;
    }
  }
//}}}
//{{{
void cDecoder264::freeGlobalBuffers() {

  sPicture::freePicture (picture);
  }
//}}}
//{{{
void cDecoder264::freeLayerBuffers() {

  // free coding
  if (coding.isSeperateColourPlane) {
    for (int i = 0; i < MAX_PLANE; i++) {
      free (mbDataJV[i]);
      mbDataJV[i] = NULL;

      freeMem2Dint (siBlockJV[i]);
      siBlockJV[i] = NULL;

      freeMem2D (predModeJV[i]);
      predModeJV[i] = NULL;

      free (intraBlockJV[i]);
      intraBlockJV[i] = NULL;
      }
    }
  else {
    free (mbData);
    mbData = NULL;

    freeMem2Dint (siBlock);
    siBlock = NULL;

    freeMem2D (predMode);
    predMode = NULL;

    free (intraBlock);
    intraBlock = NULL;
    }

  // free picPos
  free (picPos);
  picPos = NULL;

  // free cavlc
  freeMem4D (nzCoeff);
  nzCoeff = NULL;

  // free quant
  free (qpPerMatrix);
  free (qpRemMatrix);
  }
//}}}
//{{{
void cDecoder264::mbAffPostProc() {

  sPixel** imgY = picture->imgY;
  sPixel*** imgUV = picture->imgUV;

  for (uint32_t i = 0; i < picture->picSizeInMbs; i += 2) {
    if (picture->motion.mbField[i]) {
      int16_t x0;
      int16_t y0;
      getMbPos (this, i, mbSize[eLuma], &x0, &y0);

      sPixel tempBuffer[32][16];
      updateMbAff (imgY + y0, tempBuffer, x0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

      if (picture->chromaFormatIdc != YUV400) {
        x0 = (int16_t)((x0 * mbCrSizeX) >> 4);
        y0 = (int16_t)((y0 * mbCrSizeY) >> 4);
        updateMbAff (imgUV[0] + y0, tempBuffer, x0, mbCrSizeX, mbCrSizeY);
        updateMbAff (imgUV[1] + y0, tempBuffer, x0, mbCrSizeX, mbCrSizeY);
        }
      }
    }
  }
//}}}

// slice
//{{{
bool cDecoder264::isNewPicture (sPicture* picture, cSlice* slice, sOldSlice* oldSlice) {

  bool result = (NULL == picture);

  result |= (oldSlice->ppsId != slice->ppsId);
  result |= (oldSlice->frameNum != slice->frameNum);
  result |= (oldSlice->fieldPic != slice->fieldPic);

  if (slice->fieldPic && oldSlice->fieldPic)
    result |= (oldSlice->botField != slice->botField);

  result |= (oldSlice->nalRefIdc != slice->refId) && (!oldSlice->nalRefIdc || !slice->refId);
  result |= (oldSlice->isIDR != slice->isIDR);

  if (slice->isIDR && oldSlice->isIDR)
    result |= (oldSlice->idrPicId != slice->idrPicId);

  if (!activeSps->pocType) {
    result |= (oldSlice->picOrderCountLsb != slice->picOrderCountLsb);
    if ((activePps->frameBotField == 1) && !slice->fieldPic)
      result |= (oldSlice->deltaPicOrderCountBot != slice->deltaPicOrderCountBot);
    }

  if (activeSps->pocType == 1) {
    if (!activeSps->deltaPicOrderAlwaysZero) {
      result |= (oldSlice->deltaPicOrderCount[0] != slice->deltaPicOrderCount[0]);
      if ((activePps->frameBotField == 1) && !slice->fieldPic)
        result |= (oldSlice->deltaPicOrderCount[1] != slice->deltaPicOrderCount[1]);
      }
    }

  return result;
  }
//}}}
//{{{
void cDecoder264::initRefPicture (cSlice* slice) {

  sPicture* vidRefPicture = dpb.noRefPicture;
  int noRef = slice->framePoc < recoveryPoc;

  if (coding.isSeperateColourPlane) {
    switch (slice->colourPlaneId) {
      case 0:
        for (int j = 0; j < 6; j++) {
          for (int i = 0; i < MAX_LIST_SIZE; i++) {
            sPicture* refPicture = slice->listX[j][i];
            if (refPicture) {
              refPicture->noRef = noRef && (refPicture == vidRefPicture);
              refPicture->curPixelY = refPicture->imgY;
              }
            }
          }
        break;
      }
    }

  else {
    int totalLists = slice->mbAffFrame ? 6 : (slice->sliceType == cSlice::eB ? 2 : 1);
    for (int j = 0; j < totalLists; j++) {
      // note that if we always set this to MAX_LIST_SIZE, we avoid crashes with invalid refIndex being set
      // since currently this is done at the slice level, it seems safe to do so.
      // Note for some reason I get now a mismatch between version 12 and this one in cabac. I wonder why.
      for (int i = 0; i < MAX_LIST_SIZE; i++) {
        sPicture* refPicture = slice->listX[j][i];
        if (refPicture) {
          refPicture->noRef = noRef && (refPicture == vidRefPicture);
          refPicture->curPixelY = refPicture->imgY;
          }
        }
      }
    }
  }
//}}}
//{{{
void cDecoder264::initSlice (cSlice* slice) {

  activeSps = slice->activeSps;
  activePps = slice->activePps;

  slice->initLists (slice);
  reorderLists (slice);

  if (slice->picStructure == eFrame)
    slice->initMbAffLists (dpb.noRefPicture);

  // update reference flags and set current refFlag
  if (!(slice->redundantPicCount && (prevFrameNum == slice->frameNum)))
    for (int i = 16; i > 0; i--)
      slice->refFlag[i] = slice->refFlag[i-1];
  slice->refFlag[0] = (slice->redundantPicCount == 0) ? isPrimaryOk : isRedundantOk;

  if (!slice->activeSps->chromaFormatIdc ||
      (slice->activeSps->chromaFormatIdc == 3)) {
    slice->infoCbpIntra = sBitStream::infoCbpIntraOther;
    slice->infoCbpInter = sBitStream::infoCbpInterOther;
    }
  else {
    slice->infoCbpIntra = sBitStream::infoCbpIntraNormal;
    slice->infoCbpInter = sBitStream::infoCbpInterNormal;
    }
  }
//}}}
//{{{
void cDecoder264::reorderLists (cSlice* slice) {

  if ((slice->sliceType != cSlice::eI) && (slice->sliceType != cSlice::eSI)) {
    if (slice->refPicReorderFlag[LIST_0])
      slice->reorderRefPicList (LIST_0);
    if (dpb.noRefPicture == slice->listX[0][slice->numRefIndexActive[LIST_0]-1])
      cLog::log (LOGERROR, "------ refPicList0[%d] no refPic %s",
                 slice->numRefIndexActive[LIST_0]-1, nonConformingStream ? "conform":"");
    else
      slice->listXsize[0] = (char) slice->numRefIndexActive[LIST_0];
    }

  if (slice->sliceType == cSlice::eB) {
    if (slice->refPicReorderFlag[LIST_1])
      slice->reorderRefPicList (LIST_1);
    if (dpb.noRefPicture == slice->listX[1][slice->numRefIndexActive[LIST_1]-1])
      cLog::log (LOGERROR, "------ refPicList1[%d] no refPic %s",
                 slice->numRefIndexActive[LIST_0] - 1, nonConformingStream ? "conform" : "");
    else
      slice->listXsize[1] = (char)slice->numRefIndexActive[LIST_1];
    }

  slice->freeRefPicListReorderBuffer();
  }
//}}}
//{{{
void cDecoder264::copySliceInfo (cSlice* slice, sOldSlice* oldSlice) {

  oldSlice->ppsId = slice->ppsId;
  oldSlice->frameNum = slice->frameNum;
  oldSlice->fieldPic = slice->fieldPic;

  if (slice->fieldPic)
    oldSlice->botField = slice->botField;

  oldSlice->nalRefIdc = slice->refId;

  oldSlice->isIDR = (uint8_t)slice->isIDR;
  if (slice->isIDR)
    oldSlice->idrPicId = slice->idrPicId;

  if (activeSps->pocType == 0) {
    oldSlice->picOrderCountLsb = slice->picOrderCountLsb;
    oldSlice->deltaPicOrderCountBot = slice->deltaPicOrderCountBot;
    }
  else if (activeSps->pocType == 1) {
    oldSlice->deltaPicOrderCount[0] = slice->deltaPicOrderCount[0];
    oldSlice->deltaPicOrderCount[1] = slice->deltaPicOrderCount[1];
    }
  }
//}}}

// decode frame
//{{{
int cDecoder264::decodeFrame() {

  int ret = 0;
  numDecodedMbs = 0;
  picSliceIndex = 0;
  numDecodedSlices = 0;

  int curHeader = 0;
  if (newFrame) {
    // get firstSlice from sliceList;
    cSlice* slice = sliceList[picSliceIndex];
    sliceList[picSliceIndex] = nextSlice;
    nextSlice = slice;
    slice = sliceList[picSliceIndex];
    useParameterSet (slice);
    initPicture (slice);
    picSliceIndex++;
    curHeader = eSOS;
    }

  while ((curHeader != eSOP) && (curHeader != eEOS)) {
    // no pending slices, readNalu
    if (!sliceList[picSliceIndex])
      sliceList[picSliceIndex] = cSlice::allocSlice();
    cSlice* slice = sliceList[picSliceIndex];
    slice->decoder = this;
    slice->dpb = &dpb;
    slice->nextHeader = -8888;
    slice->numDecodedMbs = 0;
    slice->coefCount = -1;
    slice->pos = 0;
    slice->isResetCoef = false;
    slice->isResetCoefCr = false;

    curHeader = readNalu (slice);
    slice->curHeader = curHeader;
    //{{{  set primary,redundant flags
    if (!slice->redundantPicCount) {
      isPrimaryOk = 1;
      isRedundantOk = 1;
      }

    if (!slice->redundantPicCount && (coding.sliceType != cSlice::eI)) {
      for (int i = 0; i < slice->numRefIndexActive[LIST_0];++i)
        if (!slice->refFlag[i]) // reference primary slice incorrect, primary slice incorrect
          isPrimaryOk = 0;
      }
    else if (slice->redundantPicCount && (coding.sliceType != cSlice::eI)) // reference redundant slice incorrect
      if (!slice->refFlag[slice->redundantSliceRefIndex]) // redundant slice is incorrect
        isRedundantOk = 0;
    //}}}
    if ((slice->frameNum == prevFrameNum) && slice->redundantPicCount && isPrimaryOk && (curHeader != eEOS))
      continue;
    if (((curHeader != eSOP) && (curHeader != eEOS)) || ((curHeader == eSOP) && !picSliceIndex)) {
      //{{{  not sure ???
      slice->curSliceIndex = (int16_t)picSliceIndex;
      picture->maxSliceId = (int16_t)imax (slice->curSliceIndex, picture->maxSliceId);

      if (picSliceIndex > 0) {
        (*sliceList)->copyPoc (slice);
        sliceList[picSliceIndex-1]->endMbNumPlus1 = slice->startMbNum;
        }
      picSliceIndex++;

      if (picSliceIndex >= numAllocatedSlices)
        error ("decodeFrame - sliceList too few numAllocatedSlices");
      curHeader = eSOS;
      }
      //}}}
    else {
      if (sliceList[picSliceIndex-1]->mbAffFrame)
        sliceList[picSliceIndex-1]->endMbNumPlus1 = coding.frameSizeMbs/2;
      else
        sliceList[picSliceIndex-1]->endMbNumPlus1 = coding.frameSizeMbs / (sliceList[picSliceIndex-1]->fieldPic+1);
      newFrame = true;
      slice->curSliceIndex = 0;
      sliceList[picSliceIndex] = nextSlice;
      nextSlice = slice;
      }
    copySliceInfo (slice, oldSlice);
    }

  // decode slices
  ret = curHeader;
  initPictureDecode();
  for (int sliceIndex = 0; sliceIndex < picSliceIndex; sliceIndex++) {
    cSlice* slice = sliceList[sliceIndex];
    curHeader = slice->curHeader;
    initSlice (slice);
    if (slice->activePps->entropyCoding == eCabac) {
      //{{{  init cabac
      slice->initCabacContexts();
      cabacNewSlice (slice);
      }
      //}}}
    if (((slice->activePps->weightedBiPredIdc > 0) && (slice->sliceType == cSlice::eB)) ||
        (slice->activePps->hasWeightedPred && (slice->sliceType != cSlice::eI)))
      slice->fillWeightedPredParam();
    if (((curHeader == eSOP) || (curHeader == eSOS)) && !slice->mError)
      decodeSlice (slice);
    numDecodedSlices++;
    ercMvPerMb += slice->ercMvPerMb;
    numDecodedMbs += slice->numDecodedMbs;
    }

  endDecodeFrame();
  prevFrameNum = sliceList[0]->frameNum;
  return ret;
  }
//}}}
//{{{
void cDecoder264::useParameterSet (cSlice* slice) {

  if (!pps[slice->ppsId].ok)
    cLog::log (LOGINFO, fmt::format ("useParameterSet - invalid ppsId:{}", slice->ppsId));

  if (!sps[pps->spsId].ok)
    cLog::log (LOGINFO, fmt::format ("useParameterSet - invalid spsId:{} ppsId:{}", slice->ppsId, pps->spsId));

  if (&sps[pps->spsId] != activeSps) {
    //{{{  new sps
    if (picture)
      endDecodeFrame();

    activeSps = &sps[pps->spsId];

    if (activeSps->hasBaseLine() && !dpb.isInitDone())
      setCodingParam (sps);
    setCoding();
    initGlobalBuffers();

    if (!noOutputPriorPicFlag)
      dpb.flush();
    dpb.init (this, 0);

    // enable error conceal
    ercInit (this, coding.width, coding.height, 1);
    if (picture) {
      ercReset (ercErrorVar, picSizeInMbs, picSizeInMbs, picture->sizeX);
      ercMvPerMb = 0;
      }

    setFormat (activeSps, &param.source, &param.output);

    debug.profileString = fmt::format ("Profile:{} {}x{} {}x{} yuv{} {}:{}:{}",
                                       coding.profileIdc,
                                       param.source.width[0], param.source.height[0],
                                       coding.width, coding.height,
                                       coding.yuvFormat == YUV400 ? " 400 ":
                                         coding.yuvFormat == YUV420 ? " 420":
                                           coding.yuvFormat == YUV422 ? " 422":" 4:4:4",
                                       param.source.bitDepth[0],
                                       param.source.bitDepth[1],
                                       param.source.bitDepth[2]);
    cLog::log (LOGINFO, debug.profileString);
    }
    //}}}

  if (&pps[slice->ppsId] != activePps) {
    //{{{  new pps
    if (picture) // only on slice loss
      endDecodeFrame();

    activePps = &pps[slice->ppsId];
    }
    //}}}

  // slice->dataPartitionMode is set by read_new_slice (NALU first uint8_t ok there)
  if (activePps->entropyCoding == eCavlc) {
    slice->nalStartCode = sBitStream::vlcStartCode;
    for (int i = 0; i < 3; i++)
      slice->dataPartitionArray[i].readSyntaxElement = sBitStream::readSyntaxElementVLC;
    }
  else {
    slice->nalStartCode = cabacStartCode;
    for (int i = 0; i < 3; i++)
      slice->dataPartitionArray[i].readSyntaxElement = readSyntaxElementCabac;
    }

  coding.sliceType = slice->sliceType;
  }
//}}}
//{{{
void cDecoder264::initPicture (cSlice* slice) {

  cDpb* dpb = slice->dpb;

  picHeightInMbs = coding.frameHeightMbs / (slice->fieldPic+1);
  picSizeInMbs = coding.picWidthMbs * picHeightInMbs;
  coding.frameSizeMbs = coding.picWidthMbs * coding.frameHeightMbs;

  if (picture) // slice loss
    endDecodeFrame();

  if (recoveryPoint)
    recoveryFrameNum = (slice->frameNum + recoveryFrameCount) % coding.maxFrameNum;
  if (slice->isIDR)
    recoveryFrameNum = slice->frameNum;
  if (!recoveryPoint &&
      (slice->frameNum != preFrameNum) &&
      (slice->frameNum != (preFrameNum + 1) % coding.maxFrameNum)) {
    if (!activeSps->allowGapsFrameNum) {
      //{{{  picture error conceal
      if (param.concealMode) {
        if ((slice->frameNum) < ((preFrameNum + 1) % coding.maxFrameNum)) {
          /* Conceal lost IDR frames and any frames immediately following the IDR.
          // Use frame copy for these since lists cannot be formed correctly for motion copy*/
          concealMode = 1;
          idrConcealFlag = 1;
          concealLostFrames (dpb, slice);
          // reset to original conceal mode for future drops
          concealMode = param.concealMode;
          }
        else {
          // reset to original conceal mode for future drops
          concealMode = param.concealMode;
          idrConcealFlag = 0;
          concealLostFrames (dpb, slice);
          }
        }
      else
        // Advanced Error Concealment would be called here to combat unintentional loss of pictures
        error ("initPicture - unintentional loss of picture");
      }
      //}}}
    if (!concealMode)
      fillFrameNumGap (this, slice);
    }

  if (slice->refId)
    preFrameNum = slice->frameNum;

  // calculate POC
  decodePOC (slice);

  if (recoveryFrameNum == (int)slice->frameNum && recoveryPoc == 0x7fffffff)
    recoveryPoc = slice->framePoc;
  if (slice->refId)
    lastRefPicPoc = slice->framePoc;
  if ((slice->picStructure == eFrame) || (slice->picStructure == eTopField))
    getTime (&debug.startTime);

  picture = sPicture::allocPicture (this, slice->picStructure, coding.width, coding.height, widthCr, heightCr, 1);
  picture->topPoc = slice->topPoc;
  picture->botPoc = slice->botPoc;
  picture->framePoc = slice->framePoc;
  picture->qp = slice->qp;
  picture->sliceQpDelta = slice->sliceQpDelta;
  picture->chromaQpOffset[0] = activePps->chromaQpOffset;
  picture->chromaQpOffset[1] = activePps->chromaQpOffset2;
  picture->codingType = slice->picStructure == eFrame ?
    (slice->mbAffFrame? eFrameMbPairCoding:eFrameCoding) : eFieldCoding;

  // reset all variables of the error conceal instance before decoding of every frame.
  // here the third parameter should, if perfectly, be equal to the number of slices per frame.
  // using little value is ok, the code will allocate more memory if the slice number is larger
  ercReset (ercErrorVar, picSizeInMbs, picSizeInMbs, picture->sizeX);

  ercMvPerMb = 0;
  switch (slice->picStructure ) {
    //{{{
    case eTopField:
      picture->poc = slice->topPoc;
      idrFrameNum *= 2;
      break;
    //}}}
    //{{{
    case eBotField:
      picture->poc = slice->botPoc;
      idrFrameNum = idrFrameNum * 2 + 1;
      break;
    //}}}
    //{{{
    case eFrame:
      picture->poc = slice->framePoc;
      break;
    //}}}
    //{{{
    default:
      error ("picStructure not initialized");
    //}}}
    }

  if (coding.sliceType > cSlice::eSI) {
    setEcFlag (SE_PTYPE);
    coding.sliceType = cSlice::eP;  // concealed element
    }

  // cavlc init
  if (activePps->entropyCoding == eCavlc)
    memset (nzCoeff[0][0][0], -1, picSizeInMbs * 48 *sizeof(uint8_t)); // 3 * 4 * 4

  // Set the sliceNum member of each MB to -1, to ensure correct when packet loss occurs
  // TO set sMacroBlock Map (mark all MBs as 'have to be concealed')
  if (coding.isSeperateColourPlane) {
    for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
      sMacroBlock* mb = mbDataJV[nplane];
      char* intraBlock = intraBlockJV[nplane];
      for (int i = 0; i < (int)picSizeInMbs; ++i) {
        //{{{  resetMb
        mb->mError = 1;
        mb->dplFlag = 0;
        mb->sliceNum = -1;
        mb++;
        }
        //}}}
      memset (predModeJV[nplane][0], DC_PRED, 16 * coding.frameHeightMbs * coding.picWidthMbs * sizeof(char));
      if (activePps->hasConstrainedIntraPred)
        for (int i = 0; i < (int)picSizeInMbs; ++i)
          intraBlock[i] = 1;
      }
    }
  else {
    sMacroBlock* mb = mbData;
    for (int i = 0; i < (int)picSizeInMbs; ++i) {
      //{{{  resetMb
      mb->mError = 1;
      mb->dplFlag = 0;
      mb->sliceNum = -1;
      mb++;
      }
      //}}}
    if (activePps->hasConstrainedIntraPred)
      for (int i = 0; i < (int)picSizeInMbs; ++i)
        intraBlock[i] = 1;
    memset (predMode[0], DC_PRED, 16 * coding.frameHeightMbs * coding.picWidthMbs * sizeof(char));
    }

  picture->sliceType = coding.sliceType;
  picture->usedForRef = (slice->refId != 0);
  picture->isIDR = slice->isIDR;
  picture->noOutputPriorPicFlag = slice->noOutputPriorPicFlag;
  picture->longTermRefFlag = slice->longTermRefFlag;
  picture->adaptRefPicBufFlag = slice->adaptRefPicBufFlag;
  picture->decRefPicMarkBuffer = slice->decRefPicMarkBuffer;
  slice->decRefPicMarkBuffer = NULL;

  picture->mbAffFrame = slice->mbAffFrame;
  picture->picWidthMbs = coding.picWidthMbs;

  getMbBlockPos = picture->mbAffFrame ? getMbBlockPosMbaff : getMbBlockPosNormal;
  getNeighbour = picture->mbAffFrame ? getAffNeighbour : getNonAffNeighbour;

  picture->picNum = slice->frameNum;
  picture->frameNum = slice->frameNum;
  picture->recoveryFrame = (uint32_t)((int)slice->frameNum == recoveryFrameNum);
  picture->codedFrame = (slice->picStructure == eFrame);
  picture->chromaFormatIdc = (eYuvFormat)activeSps->chromaFormatIdc;
  picture->frameMbOnly = activeSps->frameMbOnly;
  picture->hasCrop = activeSps->hasCrop;
  if (picture->hasCrop) {
    picture->cropLeft = activeSps->cropLeft;
    picture->cropRight = activeSps->cropRight;
    picture->cropTop = activeSps->cropTop;
    picture->cropBot = activeSps->cropBot;
    }

  if (coding.isSeperateColourPlane) {
    decPictureJV[0] = picture;
    decPictureJV[1] = sPicture::allocPicture (this, slice->picStructure, coding.width, coding.height, widthCr, heightCr, 1);
    copyDecPictureJV (decPictureJV[1], decPictureJV[0] );
    decPictureJV[2] = sPicture::allocPicture (this, slice->picStructure, coding.width, coding.height, widthCr, heightCr, 1);
    copyDecPictureJV (decPictureJV[2], decPictureJV[0] );
    }
  }
//}}}
//{{{
int cDecoder264::readNalu (cSlice* slice) {

  int curHeader = 0;

  for (;;) {
    if (pendingNalu) {
      nalu = pendingNalu;
      pendingNalu = NULL;
      }
    else if (!nalu->readNalu (this))
      return eEOS;

  processNalu:
    switch (nalu->getUnitType()) {
      case cNalu::NALU_TYPE_SLICE:
      //{{{
      case cNalu::NALU_TYPE_IDR: {
        //{{{  recovery
        if (recoveryPoint || nalu->isIdr()) {
          if (!recoveryPointFound) {
            if (!nalu->isIdr()) {
              cLog::log (LOGINFO,  "-> decoding without IDR");
              nonConformingStream = true;
              }
            else
              nonConformingStream = false;
            }
          recoveryPointFound = 1;
          }
        if (!recoveryPointFound)
          break;
        //}}}

        slice->isIDR = nalu->isIdr();
        slice->refId = nalu->getRefId();

        slice->maxDataPartitions = 1;
        slice->dataPartitionMode = eDataPartition1;
        sBitStream& bitStream = slice->dataPartitionArray[0].mBitStream;
        bitStream.mOffset = 0;
        bitStream.mLength = nalu->getSodb (bitStream.mBuffer, bitStream.mAllocSize);
        bitStream.mError = 0;
        bitStream.mReadLen = 0;
        bitStream.mCodeLen = bitStream.mLength;

        readSliceHeader (bitStream, slice);

        // if primary slice replaced by redundant slice, set correct image type
        if (slice->redundantPicCount && !isPrimaryOk && isRedundantOk)
          picture->sliceType = coding.sliceType;
        if (isNewPicture (picture, slice, oldSlice)) {
          if (!picSliceIndex)
            initPicture (slice);
          nalu->checkZeroByteVCL (this);
          curHeader = eSOP;
          }
        else
          curHeader = eSOS;

        slice->setQuant();
        slice->setSliceReadFunctions();

        if (slice->mbAffFrame)
          slice->mbIndex = slice->startMbNum << 1;
        else
          slice->mbIndex = slice->startMbNum;

        if (activePps->entropyCoding == eCabac) {
          //{{{  init cabac decode
          int byteStartPosition = bitStream.mOffset / 8;
          if (bitStream.mOffset % 8)
            ++byteStartPosition;
          slice->dataPartitionArray[0].mCabacDecode.startDecoding (bitStream.mBuffer, byteStartPosition, &bitStream.mReadLen);
          }
          //}}}

        recoveryPoint = 0;

        // debug
        debug.sliceType = slice->sliceType;
        debug.sliceString = fmt::format ("{}:{}:{}:{:6d} -> pps:{} frame:{:2d} {} {}{}",
                                         nalu->isIdr() ? "IDR" : "SLC",
                                         slice->refId, nalu->getRemoveCount(), nalu->getLength(),
                                         slice->ppsId, slice->frameNum,
                                         slice->sliceType ? (slice->sliceType == 1) ?
                                           'B' : ((slice->sliceType == 2) ? 'I':'?') : 'P',
                                         slice->fieldPic ? " field":"",
                                         slice->mbAffFrame ? " mbAff":"");
        if (param.sliceDebug)
          cLog::log (LOGINFO, debug.sliceString);

        return curHeader;
        }
      //}}}

      //{{{
      case cNalu::NALU_TYPE_DPA: {
        cLog::log (LOGINFO, "DPA id:%d:%d len:%d", slice->refId, slice->sliceType, nalu->getLength());
        if (!recoveryPointFound)
          break;

        slice->isIDR = false;
        slice->refId = nalu->getRefId();

        // read dataPartition A
        slice->maxDataPartitions = 3;
        slice->dataPartitionMode = eDataPartition3;
        slice->noDataPartitionB = 1;
        slice->noDataPartitionC = 1;
        sBitStream& bitStream = slice->dataPartitionArray[0].mBitStream;
        bitStream.mOffset = 0;
        bitStream.mLength = nalu->getSodb (bitStream.mBuffer, bitStream.mAllocSize);
        bitStream.mError = 0;
        bitStream.mReadLen = 0;
        bitStream.mCodeLen = bitStream.mLength;

        readSliceHeader (bitStream, slice);

        if (isNewPicture (picture, slice, oldSlice)) {
          if (!picSliceIndex)
            initPicture (slice);
          curHeader = eSOP;
          nalu->checkZeroByteVCL (this);
          }
        else
          curHeader = eSOS;

        slice->setQuant();
        slice->setSliceReadFunctions();
        if (slice->mbAffFrame)
          slice->mbIndex = slice->startMbNum << 1;
        else
          slice->mbIndex = slice->startMbNum;

        // need to read the slice ID, which depends on the value of redundantPicCountPresent
        int sliceIdA = bitStream.readUeV ("NALU: DPA sliceId");
        if (activePps->entropyCoding == eCabac)
          error ("dataPartition with eCabac not allowed");

        if (!nalu->readNalu (this))
          return curHeader;
        if (nalu->getUnitType() == cNalu::NALU_TYPE_DPB) {
          //{{{  got nalu dataPartitionB
          bitStream = slice->dataPartitionArray[1].mBitStream;
          bitStream.mOffset = 0;
          bitStream.mLength = nalu->getSodb (bitStream.mBuffer, bitStream.mAllocSize);
          bitStream.mError = 0;
          bitStream.mReadLen = 0;
          bitStream.mCodeLen = bitStream.mLength;

          int sliceIdB = bitStream.readUeV ("NALU dataPartitionB sliceId");
          slice->noDataPartitionB = 0;
          if (sliceIdB != sliceIdA) {
            cLog::log (LOGINFO, "NALU dataPartitionB does not match dataPartitionA");
            slice->noDataPartitionB = 1;
            slice->noDataPartitionC = 1;
            }
          else {
            if (activePps->redundantPicCountPresent)
              bitStream.readUeV ("NALU dataPartitionB redundantPicCount");

            // we're finished with dataPartitionB, so let's continue with next dataPartition
            if (!nalu->readNalu (this))
              return curHeader;
            }
          }
          //}}}
        else
          slice->noDataPartitionB = 1;

        if (nalu->getUnitType() == cNalu::NALU_TYPE_DPC) {
          //{{{  got nalu dataPartitionC
          bitStream = slice->dataPartitionArray[2].mBitStream;
          bitStream.mOffset = 0;
          bitStream.mLength = nalu->getSodb (bitStream.mBuffer, bitStream.mAllocSize);
          bitStream.mError = 0;
          bitStream.mReadLen = 0;
          bitStream.mCodeLen = bitStream.mLength;

          slice->noDataPartitionC = 0;
          int sliceIdC = bitStream.readUeV ("NALU: dataPartitionC sliceId");
          if (sliceIdC != sliceIdA) {
            cLog::log (LOGINFO, "dataPartitionC does not match dataPartitionA");
            slice->noDataPartitionC = 1;
            }

          if (activePps->redundantPicCountPresent)
            bitStream.readUeV ("NALU: dataPartitionC redundant_pic_cnt");
          }
          //}}}
        else {
          slice->noDataPartitionC = 1;
          pendingNalu = nalu;
          }

        // check if we read anything else than the expected dataPartitions
        if ((nalu->getUnitType() != cNalu::NALU_TYPE_DPB) &&
            (nalu->getUnitType() != cNalu::NALU_TYPE_DPC) && (!slice->noDataPartitionC))
          goto processNalu;

        return curHeader;
        }
      //}}}
      case cNalu::NALU_TYPE_DPB:
        cLog::log (LOGINFO, "dataPartitionB without dataPartitonA"); break;
      case cNalu::NALU_TYPE_DPC:
        cLog::log (LOGINFO, "dataPartitionC without dataPartitonA"); break;

      case cNalu::NALU_TYPE_SPS: {
        int spsId = cSps::readNalu (nalu, this);
        if (param.spsDebug)
          cLog::log (LOGINFO, sps[spsId].getString());
        break;
        }

      case cNalu::NALU_TYPE_PPS: {
        int ppsId = cPps::readNalu (nalu, this);
        if (param.ppsDebug)
          cLog::log (LOGINFO, pps[ppsId].getString());
        break;
        }

      case cNalu::NALU_TYPE_SEI:
        processSei (nalu->getPayload(), nalu->getPayloadLength(), this, slice);
        break;

      case cNalu::NALU_TYPE_AUD: break;
      case cNalu::NALU_TYPE_FILL: break;
      case cNalu::NALU_TYPE_EOSEQ: break;
      case cNalu::NALU_TYPE_EOSTREAM: break;

      default:
        cLog::log (LOGINFO, "NALU:%d unknownType:%d", nalu->getLength(), nalu->getUnitType());
        break;
      }
    }

  return curHeader;
  }
//}}}
//{{{
void cDecoder264::readDecRefPicMarking (sBitStream& bitStream, cSlice* slice) {

  // free old buffer content
  while (slice->decRefPicMarkBuffer) {
    sDecodedRefPicMark* tmp_drpm = slice->decRefPicMarkBuffer;
    slice->decRefPicMarkBuffer = tmp_drpm->next;
    free (tmp_drpm);
    }

  if (slice->isIDR) {
    slice->noOutputPriorPicFlag = bitStream.readU1 ("SLC noOutputPriorPicFlag");
    noOutputPriorPicFlag = slice->noOutputPriorPicFlag;
    slice->longTermRefFlag = bitStream.readU1 ("SLC longTermRefFlag");
    }
  else {
    slice->adaptRefPicBufFlag = bitStream.readU1 ("SLC adaptRefPicBufFlag");
    if (slice->adaptRefPicBufFlag) {
      // read Memory Management Control Operation
      int val;
      do {
        sDecodedRefPicMark* tempDrpm = (sDecodedRefPicMark*)calloc (1,sizeof (sDecodedRefPicMark));
        tempDrpm->next = NULL;
        val = tempDrpm->memManagement = bitStream.readUeV ("SLC memManagement");
        if ((val == 1) || (val == 3))
          tempDrpm->diffPicNumMinus1 = bitStream.readUeV ("SLC diffPicNumMinus1");
        if (val == 2)
          tempDrpm->longTermPicNum = bitStream.readUeV ("SLC longTermPicNum");
        if ((val == 3 ) || (val == 6))
          tempDrpm->longTermFrameIndex = bitStream.readUeV ("SLC longTermFrameIndex");
        if (val == 4)
          tempDrpm->maxLongTermFrameIndexPlus1 = bitStream.readUeV ("SLC max_long_term_pic_idx_plus1");

        // add command
        if (!slice->decRefPicMarkBuffer)
          slice->decRefPicMarkBuffer = tempDrpm;
        else {
          sDecodedRefPicMark* tempDrpm2 = slice->decRefPicMarkBuffer;
          while (tempDrpm2->next)
            tempDrpm2 = tempDrpm2->next;
          tempDrpm2->next = tempDrpm;
          }
        } while (val != 0);
      }
    }
  }
//}}}
//{{{
void cDecoder264::readSliceHeader (sBitStream& bitStream, cSlice* slice) {
// Some slice syntax depends on parameterSet depends on parameterSetID of the slice header
// - read the ppsId of the slice header first
//   - then setup the active parameter sets
// - read the rest of the slice header

  slice->startMbNum = bitStream.readUeV ("SLC first_mb_in_slice");

  int sliceType = bitStream.readUeV ("SLC sliceType");
  if (sliceType > 4)
    sliceType -= 5;
  slice->sliceType = (cSlice::eSliceType)sliceType;
  coding.sliceType = slice->sliceType;

  slice->ppsId = bitStream.readUeV ("SLC ppsId");

  if (coding.isSeperateColourPlane)
    slice->colourPlaneId = bitStream.readUv (2, "SLC colourPlaneId");
  else
    slice->colourPlaneId = PLANE_Y;

  // setup parameterSet
  useParameterSet (slice);
  slice->activeSps = activeSps;
  slice->activePps = activePps;
  slice->transform8x8Mode = activePps->hasTransform8x8mode;
  slice->chroma444notSeparate = (activeSps->chromaFormatIdc == YUV444) && !coding.isSeperateColourPlane;
  slice->frameNum = bitStream.readUv (activeSps->log2maxFrameNumMinus4 + 4, "SLC frameNum");
  if (slice->isIDR) {
    preFrameNum = slice->frameNum;
    lastRefPicPoc = 0;
    }
  //{{{  read field/frame
  if (activeSps->frameMbOnly) {
    slice->fieldPic = 0;
    coding.picStructure = eFrame;
    }
  else {
    slice->fieldPic = bitStream.readU1 ("SLC fieldPic");
    if (slice->fieldPic) {
      slice->botField = (uint8_t)bitStream.readU1 ("SLC botField");
      coding.picStructure = slice->botField ? eBotField : eTopField;
      }
    else {
      slice->botField = false;
      coding.picStructure = eFrame;
      }
    }

  slice->picStructure = coding.picStructure;
  slice->mbAffFrame = activeSps->mbAffFlag && !slice->fieldPic;
  //}}}

  if (slice->isIDR)
    slice->idrPicId = bitStream.readUeV ("SLC idrPicId");
  //{{{  read picOrderCount
  if (activeSps->pocType == 0) {
    slice->picOrderCountLsb = bitStream.readUv (activeSps->log2maxPocLsbMinus4 + 4, "SLC picOrderCountLsb");
    if ((activePps->frameBotField == 1) && !slice->fieldPic)
      slice->deltaPicOrderCountBot = bitStream.readSeV ("SLC deltaPicOrderCountBot");
    else
      slice->deltaPicOrderCountBot = 0;
    }
  else if (activeSps->pocType == 1) {
    if (!activeSps->deltaPicOrderAlwaysZero) {
      slice->deltaPicOrderCount[0] = bitStream.readSeV ("SLC deltaPicOrderCount[0]");
      if ((activePps->frameBotField == 1) && !slice->fieldPic)
        slice->deltaPicOrderCount[1] = bitStream.readSeV ("SLC deltaPicOrderCount[1]");
      else
        slice->deltaPicOrderCount[1] = 0;  // set to zero if not in stream
      }
    else {
      slice->deltaPicOrderCount[0] = 0;
      slice->deltaPicOrderCount[1] = 0;
      }
    }
  //}}}

  if (activePps->redundantPicCountPresent)
    slice->redundantPicCount = bitStream.readUeV ("SLC redundantPicCount");
  if (slice->sliceType == cSlice::eB)
    slice->directSpatialMvPredFlag = bitStream.readU1 ("SLC directSpatialMvPredFlag");

  // read refPicLists
  slice->numRefIndexActive[LIST_0] = activePps->numRefIndexL0defaultActiveMinus1 + 1;
  slice->numRefIndexActive[LIST_1] = activePps->numRefIndexL1defaultActiveMinus1 + 1;
  if ((slice->sliceType == cSlice::eP) || (slice->sliceType == cSlice::eSP) || (slice->sliceType == cSlice::eB)) {
    if (bitStream.readU1 ("SLC isNumRefIndexOverride")) {
      slice->numRefIndexActive[LIST_0] = 1 + bitStream.readUeV ("SLC numRefIndexActiveL0minus1");
      if (slice->sliceType == cSlice::eB)
        slice->numRefIndexActive[LIST_1] = 1 + bitStream.readUeV ("SLC numRefIndexActiveL1minus1");
      }
    }
  if (slice->sliceType != cSlice::eB)
    slice->numRefIndexActive[LIST_1] = 0;
  //{{{  read refPicList reorder
  slice->allocRefPicListReordeBuffer();

  if ((slice->sliceType != cSlice::eI) && (slice->sliceType != cSlice::eSI)) {
    int value = slice->refPicReorderFlag[LIST_0] = bitStream.readU1 ("SLC refPicReorderL0");
    if (value) {
      int i = 0;
      do {
        value = slice->modPicNumsIdc[LIST_0][i] = bitStream.readUeV("SLC modPicNumsIdcl0");
        if ((value == 0) || (value == 1))
          slice->absDiffPicNumMinus1[LIST_0][i] = bitStream.readUeV ("SLC absDiffPicNumMinus1L0");
        else if (value == 2)
          slice->longTermPicIndex[LIST_0][i] = bitStream.readUeV ("SLC longTermPicIndexL0");
        i++;
        } while (value != 3);
      }
    }

  if (slice->sliceType == cSlice::eB) {
    int value = slice->refPicReorderFlag[LIST_1] = bitStream.readU1 ("SLC refPicReorderL1");
    if (value) {
      int i = 0;
      do {
        value = slice->modPicNumsIdc[LIST_1][i] = bitStream.readUeV ("SLC modPicNumsIdcl1");
        if ((value == 0) || (value == 1))
          slice->absDiffPicNumMinus1[LIST_1][i] = bitStream.readUeV ("SLC absDiffPicNumMinus1L1");
        else if (value == 2)
          slice->longTermPicIndex[LIST_1][i] = bitStream.readUeV ("SLC longTermPicIndexL1");
        i++;
        } while (value != 3);
      }
    }

  // set reference index of redundant slicebitStream.
  if (slice->redundantPicCount && (slice->sliceType != cSlice::eI) )
    slice->redundantSliceRefIndex = slice->absDiffPicNumMinus1[LIST_0][0] + 1;
  //}}}
  //{{{  read weightedPredWeight
  slice->hasWeightedPred = (uint16_t)(((slice->sliceType == cSlice::eP) || (slice->sliceType == cSlice::eSP))
                              ? activePps->hasWeightedPred
                              : ((slice->sliceType == cSlice::eB) && (activePps->weightedBiPredIdc == 1)));

  slice->weightedBiPredIdc = (uint16_t)((slice->sliceType == cSlice::eB) &&
                                              (activePps->weightedBiPredIdc > 0));

  if ((activePps->hasWeightedPred &&
       ((slice->sliceType == cSlice::eP) || (slice->sliceType == cSlice::eSP))) ||
      ((activePps->weightedBiPredIdc == 1) && (slice->sliceType == cSlice::eB))) {
    slice->lumaLog2weightDenom = (uint16_t)bitStream.readUeV ("SLC lumaLog2weightDenom");
    slice->wpRoundLuma = slice->lumaLog2weightDenom ? 1 << (slice->lumaLog2weightDenom - 1) : 0;

    if (activeSps->chromaFormatIdc) {
      slice->chromaLog2weightDenom = (uint16_t)bitStream.readUeV ("SLC chromaLog2weightDenom");
      slice->wpRoundChroma = slice->chromaLog2weightDenom ? 1 << (slice->chromaLog2weightDenom - 1) : 0;
      }

    slice->resetWeightedPredParam();
    for (int i = 0; i < slice->numRefIndexActive[LIST_0]; i++) {
      //{{{  read l0 weights
      if (bitStream.readU1 ("SLC hasLumaWeightL0")) {
        slice->weightedPredWeight[LIST_0][i][0] = bitStream.readSeV ("SLC lumaWeightL0");
        slice->weightedPredOffset[LIST_0][i][0] = bitStream.readSeV ("SLC lumaOffsetL0");
        slice->weightedPredOffset[LIST_0][i][0] = slice->weightedPredOffset[LIST_0][i][0] << (bitDepthLuma - 8);
        }
      else {
        slice->weightedPredWeight[LIST_0][i][0] = 1 << slice->lumaLog2weightDenom;
        slice->weightedPredOffset[LIST_0][i][0] = 0;
        }

      if (activeSps->chromaFormatIdc) {
        // l0 chroma weights
        int hasChromaWeightL0 = bitStream.readU1 ("SLC hasChromaWeightL0");
        for (int j = 1; j < 3; j++) {
          if (hasChromaWeightL0) {
            slice->weightedPredWeight[LIST_0][i][j] = bitStream.readSeV ("SLC chromaWeightL0");
            slice->weightedPredOffset[LIST_0][i][j] = bitStream.readSeV ("SLC chromaOffsetL0");
            slice->weightedPredOffset[LIST_0][i][j] = slice->weightedPredOffset[LIST_0][i][j] << (bitDepthChroma-8);
            }
          else {
            slice->weightedPredWeight[LIST_0][i][j] = 1 << slice->chromaLog2weightDenom;
            slice->weightedPredOffset[LIST_0][i][j] = 0;
            }
          }
        }
      }
      //}}}

    if ((slice->sliceType == cSlice::eB) && activePps->weightedBiPredIdc == 1)
      for (int i = 0; i < slice->numRefIndexActive[LIST_1]; i++) {
        //{{{  read l1 weights
        if (bitStream.readU1 ("SLC hasLumaWeightL1")) {
          // read l1 luma weights
          slice->weightedPredWeight[LIST_1][i][0] = bitStream.readSeV ("SLC lumaWeightL1");
          slice->weightedPredOffset[LIST_1][i][0] = bitStream.readSeV ("SLC lumaOffsetL1");
          slice->weightedPredOffset[LIST_1][i][0] = slice->weightedPredOffset[LIST_1][i][0] << (bitDepthLuma-8);
          }
        else {
          slice->weightedPredWeight[LIST_1][i][0] = 1 << slice->lumaLog2weightDenom;
          slice->weightedPredOffset[LIST_1][i][0] = 0;
          }

        if (activeSps->chromaFormatIdc) {
          int hasChromaWeightL1 = bitStream.readU1 ("SLC hasChromaWeightL1");
          for (int j = 1; j < 3; j++) {
            if (hasChromaWeightL1) {
              // read l1 chroma weights
              slice->weightedPredWeight[LIST_1][i][j] = bitStream.readSeV ("SLC chromaWeightL1");
              slice->weightedPredOffset[LIST_1][i][j] = bitStream.readSeV ("SLC chromaOffsetL1");
              slice->weightedPredOffset[LIST_1][i][j] = slice->weightedPredOffset[LIST_1][i][j]<<(bitDepthChroma-8);
              }
            else {
              slice->weightedPredWeight[LIST_1][i][j] = 1 << slice->chromaLog2weightDenom;
              slice->weightedPredOffset[LIST_1][i][j] = 0;
              }
            }
          }
        }
        //}}}
    }
  //}}}

  if (slice->refId)
    readDecRefPicMarking (bitStream, slice);

  if ((activePps->entropyCoding == eCabac) &&
      (slice->sliceType != cSlice::eI) && (slice->sliceType != cSlice::eSI))
    slice->cabacInitIdc = bitStream.readUeV ("SLC cabacInitIdc");
  else
    slice->cabacInitIdc = 0;
  //{{{  read qp
  slice->sliceQpDelta = bitStream.readSeV ("SLC sliceQpDelta");
  slice->qp = 26 + activePps->picInitQpMinus26 + slice->sliceQpDelta;

  if ((slice->sliceType == cSlice::eSP) || (slice->sliceType == cSlice::eSI)) {
    if (slice->sliceType == cSlice::eSP)
      slice->spSwitch = bitStream.readU1 ("SLC sp_for_switchFlag");
    slice->sliceQsDelta = bitStream.readSeV ("SLC sliceQsDelta");
    slice->qs = 26 + activePps->picInitQsMinus26 + slice->sliceQsDelta;
    }
  //}}}

  if (activePps->hasDeblockFilterControl) {
    //{{{  read deblockFilter
    slice->deblockFilterDisableIdc = (int16_t)bitStream.readUeV ("SLC disable_deblocking_filter_idc");
    if (slice->deblockFilterDisableIdc != 1) {
      slice->deblockFilterC0Offset = (int16_t)(2 * bitStream.readSeV ("SLC slice_alpha_c0_offset_div2"));
      slice->deblockFilterBetaOffset = (int16_t)(2 * bitStream.readSeV ("SLC slice_beta_offset_div2"));
      }
    else
      slice->deblockFilterC0Offset = slice->deblockFilterBetaOffset = 0;
    }
    //}}}
  else {
    //{{{  enable deblockFilter
    slice->deblockFilterDisableIdc = 0;
    slice->deblockFilterC0Offset = 0;
    slice->deblockFilterBetaOffset = 0;
    }
    //}}}
  if (activeSps->isHiIntraOnlyProfile() && !param.intraProfileDeblocking) {
    //{{{  hiIntra deblock
    slice->deblockFilterDisableIdc = 1;
    slice->deblockFilterC0Offset = 0;
    slice->deblockFilterBetaOffset = 0;
    }
    //}}}

  //{{{  read sliceGroup
  if ((activePps->numSliceGroupsMinus1 > 0) &&
      (activePps->sliceGroupMapType >= 3) &&
      (activePps->sliceGroupMapType <= 5)) {
    int len = (activeSps->picHeightMapUnitsMinus1+1) * (activeSps->picWidthMbsMinus1+1) /
                (activePps->sliceGroupChangeRateMius1 + 1);
    if (((activeSps->picHeightMapUnitsMinus1+1) * (activeSps->picWidthMbsMinus1+1)) %
        (activePps->sliceGroupChangeRateMius1 + 1))
      len += 1;

    len = ceilLog2 (len+1);
    slice->sliceGroupChangeCycle = bitStream.readUv (len, "SLC sliceGroupChangeCycle");
    }
  //}}}

  picHeightInMbs = coding.frameHeightMbs / ( 1 + slice->fieldPic );
  picSizeInMbs = coding.picWidthMbs * picHeightInMbs;
  coding.frameSizeMbs = coding.picWidthMbs * coding.frameHeightMbs;
  }
//}}}
//{{{
void cDecoder264::decodeSlice (cSlice* slice) {

  bool endOfSlice = false;

  slice->codCount = -1;

  if (coding.isSeperateColourPlane)
    changePlaneJV (slice->colourPlaneId, slice);
  else {
    slice->mbData = mbData;
    slice->picture = picture;
    slice->siBlock = siBlock;
    slice->predMode = predMode;
    slice->intraBlock = intraBlock;
    }

  if (slice->sliceType == cSlice::eB)
    slice->computeColocated (slice->listX);

  if ((slice->sliceType != cSlice::eI) && (slice->sliceType != cSlice::eSI))
    initRefPicture (slice);

  // loop over macroBlocks
  while (!endOfSlice) {
    sMacroBlock* mb;
    slice->startMacroBlockDecode (&mb);
    slice->readMacroBlock (mb);
    decodeMacroBlock (mb, slice->picture);

    if (slice->mbAffFrame && mb->mbField) {
      slice->numRefIndexActive[LIST_0] >>= 1;
      slice->numRefIndexActive[LIST_1] >>= 1;
      }

    ercWriteMbModeMv (mb);
    endOfSlice = slice->endMacroBlockDecode (!slice->mbAffFrame || (slice->mbIndex % 2));
    }
  }
//}}}
//{{{
void cDecoder264::endDecodeFrame() {

  // return if the last picture has already been finished
  if (!picture ||
      ((numDecodedMbs != picSizeInMbs) && ((coding.yuvFormat != YUV444) || !coding.isSeperateColourPlane)))
    return;

  //{{{  error conceal
  frame recfr;
  recfr.decoder = this;
  recfr.yptr = &picture->imgY[0][0];
  if (picture->chromaFormatIdc != YUV400) {
    recfr.uptr = &picture->imgUV[0][0][0];
    recfr.vptr = &picture->imgUV[1][0][0];
    }

  // this is always true at the beginning of a picture
  int ercSegment = 0;

  // mark the start of the first segment
  if (!picture->mbAffFrame) {
    ercStartSegment (0, ercSegment, 0 , ercErrorVar);
    // generate the segments according to the macroBlock map
    uint32_t i;
    for (i = 1; i < picture->picSizeInMbs; i++) {
      if (mbData[i].mError != mbData[i-1].mError) {
        ercStopSegment (i-1, ercSegment, 0, ercErrorVar); //! stop current segment

        // mark current segment as lost or OK
        if (mbData[i-1].mError)
          ercMarksegmentLost (picture->sizeX, ercErrorVar);
        else
          ercMarksegmentOK (picture->sizeX, ercErrorVar);

        ercSegment++;
        ercStartSegment (i, ercSegment, 0 , ercErrorVar); //! start new segment
        }
      }

    // mark end of the last segment
    ercStopSegment (picture->picSizeInMbs-1, ercSegment, 0, ercErrorVar);
    if (mbData[i-1].mError)
      ercMarksegmentLost (picture->sizeX, ercErrorVar);
    else
      ercMarksegmentOK (picture->sizeX, ercErrorVar);

    // call the right error conceal function depending on the frame type.
    ercMvPerMb /= picture->picSizeInMbs;
    if (picture->sliceType == cSlice::eI || picture->sliceType == cSlice::eSI) // I-frame
      ercConcealIntraFrame (this, &recfr, picture->sizeX, picture->sizeY, ercErrorVar);
    else
      ercConcealInterFrame (&recfr, ercObjectList,
                            picture->sizeX, picture->sizeY, ercErrorVar, picture->chromaFormatIdc);
    }
  //}}}
  if (!deblockMode &&
      param.deblock &&
      (deblockEnable & (1 << picture->usedForRef))) {
    if (coding.isSeperateColourPlane) {
      //{{{  deblockJV
      int colourPlaneId = sliceList[0]->colourPlaneId;
      for (int nplane = 0; nplane < MAX_PLANE; ++nplane) {
        sliceList[0]->colourPlaneId = nplane;
        changePlaneJV (nplane, NULL );
        deblockPicture (this, picture);
        }
      sliceList[0]->colourPlaneId = colourPlaneId;
      makeFramePictureJV();
      }
      //}}}
    else
      deblockPicture (this, picture);
    }
  else if (coding.isSeperateColourPlane)
    makeFramePictureJV();

  if (picture->mbAffFrame)
    mbAffPostProc();
  if (coding.picStructure != eFrame)
    idrFrameNum /= 2;
  if (picture->usedForRef)
    padPicture (picture);

  int picStructure = picture->picStructure;
  int sliceType = picture->sliceType;
  int pocNum = picture->framePoc;
  int refpic = picture->usedForRef;
  int qp = picture->qp;
  int picNum = picture->picNum;
  int isIdr = picture->isIDR;

  dpb.storePicture (picture);
  picture = NULL;

  if (lastHasMmco5)
    preFrameNum = 0;

  if (picStructure == eTopField || picStructure == eFrame) {
    //{{{  sliceTypeString
    if (sliceType == cSlice::eI && isIdr)
      debug.sliceTypeString = "IDR";
    else if (sliceType == cSlice::eI)
      debug.sliceTypeString = " I ";
    else if (sliceType == cSlice::eP)
      debug.sliceTypeString = " P ";
    else if (sliceType == cSlice::eSP)
      debug.sliceTypeString = "SP ";
    else if  (sliceType == cSlice::eSI)
      debug.sliceTypeString = "SI ";
    else if (refpic)
      debug.sliceTypeString = " B ";
    else
      debug.sliceTypeString = " b ";
    }
    //}}}
  else if (picStructure == eBotField) {
    //{{{  sliceTypeString
    if (sliceType == cSlice::eI && isIdr)
      debug.sliceTypeString += "|IDR";
    else if (sliceType == cSlice::eI)
      debug.sliceTypeString += "| I ";
    else if (sliceType == cSlice::eP)
      debug.sliceTypeString += "| P ";
    else if (sliceType == cSlice::eSP)
      debug.sliceTypeString += "|SP ";
    else if  (sliceType == cSlice::eSI)
      debug.sliceTypeString += "|SI ";
    else if (refpic)
      debug.sliceTypeString += "| B ";
    else
      debug.sliceTypeString += "| b ";
    }
    //}}}
  if ((picStructure == eFrame) || picStructure == eBotField) {
    //{{{  out debug
    getTime (&debug.endTime);

    // count numOutputFrames
    int numOutputFrames = 0;
    sDecodedPic* decodedPic = outDecodedPics;
    while (decodedPic) {
      if (decodedPic->ok)
        numOutputFrames++;
      decodedPic = decodedPic->next;
      }

    debug.outSliceType = (cSlice::eSliceType)sliceType;
    debug.outString = fmt::format ("{} {}:{}:{:2d} {:3d}ms ->{}-> poc:{} pic:{} -> {}",
                                   decodeFrameNum,
                                   numDecodedSlices, numDecodedMbs, qp,
                                   (int)timeNorm (timeDiff (&debug.startTime, &debug.endTime)),
                                   debug.sliceTypeString,
                                   pocNum, picNum, numOutputFrames);
    if (param.outDebug)
      cLog::log (LOGINFO, "-> " + debug.outString);
    //}}}

    // I or P pictures ?
    if ((sliceType == cSlice::eI) || (sliceType == cSlice::eSI) || (sliceType == cSlice::eP) || refpic)
      ++idrFrameNum;

    (decodeFrameNum)++;
    }
  }
//}}}

//{{{
void cDecoder264::setCoding() {

  widthCr = 0;
  heightCr = 0;
  bitDepthLuma = coding.bitDepthLuma;
  bitDepthChroma = coding.bitDepthChroma;

  coding.lumaPadX = MCBUF_LUMA_PAD_X;
  coding.lumaPadY = MCBUF_LUMA_PAD_Y;
  coding.chromaPadX = MCBUF_CHROMA_PAD_X;
  coding.chromaPadY = MCBUF_CHROMA_PAD_Y;

  if (coding.yuvFormat == YUV420) {
    //{{{  yuv420
    widthCr = coding.width >> 1;
    heightCr = coding.height >> 1;
    }
    //}}}
  else if (coding.yuvFormat == YUV422) {
    //{{{  yuv422
    widthCr = coding.width >> 1;
    heightCr = coding.height;
    coding.chromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
    //}}}
  else if (coding.yuvFormat == YUV444) {
    //{{{  yuv444
    widthCr = coding.width;
    heightCr = coding.height;
    coding.chromaPadX = coding.lumaPadX;
    coding.chromaPadY = coding.lumaPadY;
    }
    //}}}

  // pel bitDepth init
  coding.bitDepthLumaQpScale = 6 * (bitDepthLuma - 8);

  if ((bitDepthLuma > bitDepthChroma) ||
      (activeSps->chromaFormatIdc == YUV400))
    coding.picUnitBitSizeDisk = (bitDepthLuma > 8)? 16:8;
  else
    coding.picUnitBitSizeDisk = (bitDepthChroma > 8)? 16:8;

  coding.dcPredValueComp[0] = 1<<(bitDepthLuma - 1);
  coding.maxPelValueComp[0] = (1<<bitDepthLuma) - 1;
  mbSize[0][0] = mbSize[0][1] = MB_BLOCK_SIZE;

  if (activeSps->chromaFormatIdc == YUV400) {
    //{{{  yuv400
    coding.bitDepthChromaQpScale = 0;

    coding.maxPelValueComp[1] = 0;
    coding.maxPelValueComp[2] = 0;

    coding.numBlock8x8uv = 0;
    coding.numUvBlocks = 0;
    coding.numCdcCoeff = 0;

    mbSize[1][0] = mbSize[2][0] = mbCrSizeX  = 0;
    mbSize[1][1] = mbSize[2][1] = mbCrSizeY  = 0;

    coding.subpelX = 0;
    coding.subpelY = 0;
    coding.shiftpelX = 0;
    coding.shiftpelY = 0;

    coding.totalScale = 0;
    }
    //}}}
  else {
    //{{{  not yuv400
    coding.bitDepthChromaQpScale = 6 * (bitDepthChroma - 8);

    coding.dcPredValueComp[1] = 1 << (bitDepthChroma - 1);
    coding.dcPredValueComp[2] = coding.dcPredValueComp[1];
    coding.maxPelValueComp[1] = (1 << bitDepthChroma) - 1;
    coding.maxPelValueComp[2] = (1 << bitDepthChroma) - 1;

    coding.numBlock8x8uv = (1 << activeSps->chromaFormatIdc) & (~(0x1));
    coding.numUvBlocks = coding.numBlock8x8uv >> 1;
    coding.numCdcCoeff = coding.numBlock8x8uv << 1;

    mbSize[1][0] = mbSize[2][0] =
      mbCrSizeX = (activeSps->chromaFormatIdc == YUV420 ||
                            activeSps->chromaFormatIdc == YUV422) ? 8 : 16;
    mbSize[1][1] = mbSize[2][1] =
      mbCrSizeY = (activeSps->chromaFormatIdc == YUV444 ||
                            activeSps->chromaFormatIdc == YUV422) ? 16 : 8;

    coding.subpelX = mbCrSizeX == 8 ? 7 : 3;
    coding.subpelY = mbCrSizeY == 8 ? 7 : 3;
    coding.shiftpelX = mbCrSizeX == 8 ? 3 : 2;
    coding.shiftpelY = mbCrSizeY == 8 ? 3 : 2;

    coding.totalScale = coding.shiftpelX + coding.shiftpelY;
    }
    //}}}

  mbCrSize = mbCrSizeX * mbCrSizeY;
  mbSizeBlock[0][0] = mbSizeBlock[0][1] = mbSize[0][0] >> 2;
  mbSizeBlock[1][0] = mbSizeBlock[2][0] = mbSize[1][0] >> 2;
  mbSizeBlock[1][1] = mbSizeBlock[2][1] = mbSize[1][1] >> 2;

  mbSizeShift[0][0] = mbSizeShift[0][1] = ceilLog2sf (mbSize[0][0]);
  mbSizeShift[1][0] = mbSizeShift[2][0] = ceilLog2sf (mbSize[1][0]);
  mbSizeShift[1][1] = mbSizeShift[2][1] = ceilLog2sf (mbSize[1][1]);
  }
//}}}
//{{{
void cDecoder264::setCodingParam (cSps* sps) {

  // maximum vertical motion vector range in luma quarter pixel units
  coding.profileIdc = sps->profileIdc;
  coding.useLosslessQpPrime = sps->useLosslessQpPrime;

  if (sps->levelIdc <= 10)
    coding.maxVmvR = 64 * 4;
  else if (sps->levelIdc <= 20)
    coding.maxVmvR = 128 * 4;
  else if (sps->levelIdc <= 30)
    coding.maxVmvR = 256 * 4;
  else
    coding.maxVmvR = 512 * 4; // 512 pixels in quarter pixels

  // Fidelity Range Extensions stuff (part 1)
  coding.widthCr = 0;
  coding.heightCr = 0;
  coding.bitDepthLuma = (int16_t)(sps->bit_depth_luma_minus8 + 8);
  coding.bitDepthScale[0] = 1 << sps->bit_depth_luma_minus8;
  coding.bitDepthChroma = 0;
  if (sps->chromaFormatIdc != YUV400) {
    coding.bitDepthChroma = (int16_t)(sps->bit_depth_chroma_minus8 + 8);
    coding.bitDepthScale[1] = 1 << sps->bit_depth_chroma_minus8;
    }

  coding.maxFrameNum = 1 << (sps->log2maxFrameNumMinus4+4);
  coding.picWidthMbs = sps->picWidthMbsMinus1 + 1;
  coding.picHeightMapUnits = sps->picHeightMapUnitsMinus1 + 1;
  coding.frameHeightMbs = (2 - sps->frameMbOnly) * coding.picHeightMapUnits;
  coding.frameSizeMbs = coding.picWidthMbs * coding.frameHeightMbs;

  coding.yuvFormat = sps->chromaFormatIdc;
  coding.isSeperateColourPlane = sps->isSeperateColourPlane;

  coding.width = coding.picWidthMbs * MB_BLOCK_SIZE;
  coding.height = coding.frameHeightMbs * MB_BLOCK_SIZE;

  coding.lumaPadX = MCBUF_LUMA_PAD_X;
  coding.lumaPadY = MCBUF_LUMA_PAD_Y;
  coding.chromaPadX = MCBUF_CHROMA_PAD_X;
  coding.chromaPadY = MCBUF_CHROMA_PAD_Y;

  if (sps->chromaFormatIdc == YUV420) {
    coding.widthCr  = (coding.width  >> 1);
    coding.heightCr = (coding.height >> 1);
    }
  else if (sps->chromaFormatIdc == YUV422) {
    coding.widthCr  = (coding.width >> 1);
    coding.heightCr = coding.height;
    coding.chromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (sps->chromaFormatIdc == YUV444) {
    coding.widthCr = coding.width;
    coding.heightCr = coding.height;
    coding.chromaPadX = coding.lumaPadX;
    coding.chromaPadY = coding.lumaPadY;
    }

  // pel bitDepth init
  coding.bitDepthLumaQpScale = 6 * (coding.bitDepthLuma - 8);

  if (coding.bitDepthLuma > coding.bitDepthChroma || sps->chromaFormatIdc == YUV400)
    coding.picUnitBitSizeDisk = (coding.bitDepthLuma > 8)? 16:8;
  else
    coding.picUnitBitSizeDisk = (coding.bitDepthChroma > 8)? 16:8;
  coding.dcPredValueComp[0] = 1 << (coding.bitDepthLuma - 1);
  coding.maxPelValueComp[0] = (1 << coding.bitDepthLuma) - 1;
  coding.mbSize[0][0] = coding.mbSize[0][1] = MB_BLOCK_SIZE;

  if (sps->chromaFormatIdc == YUV400) {
    //{{{  YUV400
    coding.bitDepthChromaQpScale = 0;
    coding.maxPelValueComp[1] = 0;
    coding.maxPelValueComp[2] = 0;
    coding.numBlock8x8uv = 0;
    coding.numUvBlocks = 0;
    coding.numCdcCoeff = 0;
    coding.mbSize[1][0] = coding.mbSize[2][0] = coding.mbCrSizeX  = 0;
    coding.mbSize[1][1] = coding.mbSize[2][1] = coding.mbCrSizeY  = 0;
    coding.subpelX = 0;
    coding.subpelY = 0;
    coding.shiftpelX = 0;
    coding.shiftpelY = 0;
    coding.totalScale = 0;
    }
    //}}}
  else {
    //{{{  not YUV400
    coding.bitDepthChromaQpScale = 6 * (coding.bitDepthChroma - 8);
    coding.dcPredValueComp[1] = (1 << (coding.bitDepthChroma - 1));
    coding.dcPredValueComp[2] = coding.dcPredValueComp[1];
    coding.maxPelValueComp[1] = (1 << coding.bitDepthChroma) - 1;
    coding.maxPelValueComp[2] = (1 << coding.bitDepthChroma) - 1;
    coding.numBlock8x8uv = (1 << sps->chromaFormatIdc) & (~(0x1));
    coding.numUvBlocks = (coding.numBlock8x8uv >> 1);
    coding.numCdcCoeff = (coding.numBlock8x8uv << 1);
    coding.mbSize[1][0] = coding.mbSize[2][0] = coding.mbCrSizeX  = (sps->chromaFormatIdc==YUV420 || sps->chromaFormatIdc==YUV422)?  8 : 16;
    coding.mbSize[1][1] = coding.mbSize[2][1] = coding.mbCrSizeY  = (sps->chromaFormatIdc==YUV444 || sps->chromaFormatIdc==YUV422)? 16 :  8;

    coding.subpelX = coding.mbCrSizeX == 8 ? 7 : 3;
    coding.subpelY = coding.mbCrSizeY == 8 ? 7 : 3;
    coding.shiftpelX = coding.mbCrSizeX == 8 ? 3 : 2;
    coding.shiftpelY = coding.mbCrSizeY == 8 ? 3 : 2;
    coding.totalScale = coding.shiftpelX + coding.shiftpelY;
    }
    //}}}

  coding.mbCrSize = coding.mbCrSizeX * coding.mbCrSizeY;
  coding.mbSizeBlock[0][0] = coding.mbSizeBlock[0][1] = coding.mbSize[0][0] >> 2;
  coding.mbSizeBlock[1][0] = coding.mbSizeBlock[2][0] = coding.mbSize[1][0] >> 2;
  coding.mbSizeBlock[1][1] = coding.mbSizeBlock[2][1] = coding.mbSize[1][1] >> 2;

  coding.mbSizeShift[0][0] = coding.mbSizeShift[0][1] = ceilLog2sf (coding.mbSize[0][0]);
  coding.mbSizeShift[1][0] = coding.mbSizeShift[2][0] = ceilLog2sf (coding.mbSize[1][0]);
  coding.mbSizeShift[1][1] = coding.mbSizeShift[2][1] = ceilLog2sf (coding.mbSize[1][1]);
  }
//}}}
//{{{
void cDecoder264::setFormat (cSps* sps, sFrameFormat* source, sFrameFormat* output) {

  static const int kSubWidthC[4] = { 1, 2, 2, 1};
  static const int kSubHeightC[4] = { 1, 2, 1, 1};

  // source
  //{{{  crop
  int cropLeft, cropRight, cropTop, cropBot;
  if (sps->hasCrop) {
    cropLeft = kSubWidthC [sps->chromaFormatIdc] * sps->cropLeft;
    cropRight = kSubWidthC [sps->chromaFormatIdc] * sps->cropRight;
    cropTop = kSubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnly ) *  sps->cropTop;
    cropBot = kSubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnly ) *  sps->cropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBot = 0;

  source->width[0] = coding.width - cropLeft - cropRight;
  source->height[0] = coding.height - cropTop - cropBot;

  // cropping for chroma
  if (sps->hasCrop) {
    cropLeft = sps->cropLeft;
    cropRight = sps->cropRight;
    cropTop = (2 - sps->frameMbOnly) * sps->cropTop;
    cropBot = (2 - sps->frameMbOnly) * sps->cropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBot = 0;
  //}}}
  source->width[1] = widthCr - cropLeft - cropRight;
  source->width[2] = source->width[1];
  source->height[1] = heightCr - cropTop - cropBot;
  source->height[2] = source->height[1];

  // ????
  source->width[1] = widthCr;
  source->width[2] = widthCr;

  source->sizeCmp[0] = source->width[0] * source->height[0];
  source->sizeCmp[1] = source->width[1] * source->height[1];
  source->sizeCmp[2] = source->sizeCmp[1];
  source->mbWidth = source->width[0]  / MB_BLOCK_SIZE;
  source->mbHeight = source->height[0] / MB_BLOCK_SIZE;

  // output
  output->width[0] = coding.width;
  output->height[0] = coding.height;
  output->height[1] = heightCr;
  output->height[2] = heightCr;

  // output size (excluding padding)
  output->sizeCmp[0] = output->width[0] * output->height[0];
  output->sizeCmp[1] = output->width[1] * output->height[1];
  output->sizeCmp[2] = output->sizeCmp[1];
  output->mbWidth = output->width[0]  / MB_BLOCK_SIZE;
  output->mbHeight = output->height[0] / MB_BLOCK_SIZE;

  output->bitDepth[0] = source->bitDepth[0] = bitDepthLuma;
  output->bitDepth[1] = source->bitDepth[1] = bitDepthChroma;
  output->bitDepth[2] = source->bitDepth[2] = bitDepthChroma;

  output->colourModel = source->colourModel;
  output->yuvFormat = source->yuvFormat = (eYuvFormat)sps->chromaFormatIdc;
  }
//}}}

//{{{  flexibleMacroblockOrdering
//{{{
void cDecoder264::initFmo (cSlice* slice) {

  fmoGenerateMapUnitToSliceGroupMap (slice);
  fmoGenerateMbToSliceGroupMap (slice);
  }
//}}}
//{{{
void cDecoder264::closeFmo() {

  free (mbToSliceGroupMap);
  mbToSliceGroupMap = NULL;

  free (mapUnitToSliceGroupMap);
  mapUnitToSliceGroupMap = NULL;
  }
//}}}

int cDecoder264::fmoGetNumberOfSliceGroup() { return sliceGroupsNum; }
int cDecoder264::fmoGetLastMBOfPicture() { return fmoGetLastMBInSliceGroup (fmoGetNumberOfSliceGroup() - 1); }
int cDecoder264::fmoGetSliceGroupId (int mb) { return mbToSliceGroupMap[mb]; }
//{{{
int cDecoder264::fmoGetLastMBInSliceGroup (int SliceGroup) {

  for (int i = picSizeInMbs-1; i >= 0; i--)
    if (fmoGetSliceGroupId (i) == SliceGroup)
      return i;

  return -1;
  }
//}}}

//{{{
void cDecoder264::fmoGenerateType0MapUnitMap (uint32_t PicSizeInMapUnits) {
// Generate interleaved slice group map type MapUnit map (type 0)

  uint32_t iGroup;
  uint32_t j;
  uint32_t i = 0;
  do {
    for (iGroup = 0;
         (iGroup <= activePps->numSliceGroupsMinus1) && (i < PicSizeInMapUnits);
         i += activePps->runLengthMinus1[iGroup++] + 1 )
      for (j = 0; j <= activePps->runLengthMinus1[ iGroup ] && i + j < PicSizeInMapUnits; j++ )
        mapUnitToSliceGroupMap[i+j] = iGroup;
    } while (i < PicSizeInMapUnits);
  }
//}}}
//{{{
void cDecoder264::fmoGenerateType1MapUnitMap (uint32_t PicSizeInMapUnits) {
// Generate dispersed slice group map type MapUnit map (type 1)

  for (uint32_t i = 0; i < PicSizeInMapUnits; i++ )
    mapUnitToSliceGroupMap[i] =
      ((i%coding.picWidthMbs) + (((i/coding.picWidthMbs) * (activePps->numSliceGroupsMinus1+1)) / 2))
        % (activePps->numSliceGroupsMinus1+1);
  }
//}}}
//{{{
void cDecoder264::fmoGenerateType2MapUnitMap (uint32_t PicSizeInMapUnits) {
// Generate foreground with left-over slice group map type MapUnit map (type 2)

  int iGroup;
  uint32_t i, x, y;
  uint32_t yTopLeft, xTopLeft, yBottomRight, xBottomRight;

  for (i = 0; i < PicSizeInMapUnits; i++ )
    mapUnitToSliceGroupMap[i] = activePps->numSliceGroupsMinus1;

  for (iGroup = activePps->numSliceGroupsMinus1 - 1 ; iGroup >= 0; iGroup--) {
    yTopLeft = activePps->topLeft[ iGroup ] / coding.picWidthMbs;
    xTopLeft = activePps->topLeft[ iGroup ] % coding.picWidthMbs;
    yBottomRight = activePps->botRight[iGroup] / coding.picWidthMbs;
    xBottomRight = activePps->botRight[iGroup] % coding.picWidthMbs;
    for (y = yTopLeft; y <= yBottomRight; y++ )
      for (x = xTopLeft; x <= xBottomRight; x++ )
        mapUnitToSliceGroupMap[ y * coding.picWidthMbs + x ] = iGroup;
    }
  }
//}}}
//{{{
void cDecoder264::fmoGenerateType3MapUnitMap (uint32_t PicSizeInMapUnits, cSlice* slice) {
// Generate box-out slice group map type MapUnit map (type 3)

  uint32_t i, k;
  int leftBound, topBound, rightBound, bottomBound;
  int x, y, xDir, yDir;
  int mapUnitVacant;

  uint32_t mapUnitsInSliceGroup0 = imin((activePps->sliceGroupChangeRateMius1 + 1) * slice->sliceGroupChangeCycle, PicSizeInMapUnits);

  for (i = 0; i < PicSizeInMapUnits; i++ )
    mapUnitToSliceGroupMap[i] = 2;

  x = (coding.picWidthMbs - activePps->sliceGroupChangeDirectionFlag) / 2;
  y = (coding.picHeightMapUnits - activePps->sliceGroupChangeDirectionFlag) / 2;

  leftBound = x;
  topBound = y;
  rightBound  = x;
  bottomBound = y;

  xDir = activePps->sliceGroupChangeDirectionFlag - 1;
  yDir = activePps->sliceGroupChangeDirectionFlag;

  for (k = 0; k < PicSizeInMapUnits; k += mapUnitVacant ) {
    mapUnitVacant = ( mapUnitToSliceGroupMap[ y * coding.picWidthMbs + x ]  ==  2 );
    if (mapUnitVacant )
       mapUnitToSliceGroupMap[ y * coding.picWidthMbs + x ] = ( k >= mapUnitsInSliceGroup0 );

    if (xDir  ==  -1  &&  x  ==  leftBound ) {
      leftBound = imax( leftBound - 1, 0 );
      x = leftBound;
      xDir = 0;
      yDir = 2 * activePps->sliceGroupChangeDirectionFlag - 1;
      }
    else if (xDir  ==  1  &&  x  ==  rightBound ) {
      rightBound = imin( rightBound + 1, (int)coding.picWidthMbs - 1 );
      x = rightBound;
      xDir = 0;
      yDir = 1 - 2 * activePps->sliceGroupChangeDirectionFlag;
      }
    else if (yDir  ==  -1  &&  y  ==  topBound ) {
      topBound = imax( topBound - 1, 0 );
      y = topBound;
      xDir = 1 - 2 * activePps->sliceGroupChangeDirectionFlag;
      yDir = 0;
      }
    else if (yDir  ==  1  &&  y  ==  bottomBound ) {
      bottomBound = imin( bottomBound + 1, (int)coding.picHeightMapUnits - 1 );
      y = bottomBound;
      xDir = 2 * activePps->sliceGroupChangeDirectionFlag - 1;
      yDir = 0;
      }
    else {
      x = x + xDir;
      y = y + yDir;
      }
    }
  }
//}}}
//{{{
void cDecoder264::fmoGenerateType4MapUnitMap (uint32_t PicSizeInMapUnits, cSlice* slice) {
// Generate raster scan slice group map type MapUnit map (type 4)

  uint32_t mapUnitsInSliceGroup0 = imin((activePps->sliceGroupChangeRateMius1 + 1) * slice->sliceGroupChangeCycle, PicSizeInMapUnits);
  uint32_t sizeOfUpperLeftGroup = activePps->sliceGroupChangeDirectionFlag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  for (uint32_t i = 0; i < PicSizeInMapUnits; i++ )
    if (i < sizeOfUpperLeftGroup )
      mapUnitToSliceGroupMap[i] = activePps->sliceGroupChangeDirectionFlag;
    else
      mapUnitToSliceGroupMap[i] = 1 - activePps->sliceGroupChangeDirectionFlag;
  }
//}}}
//{{{
void cDecoder264::fmoGenerateType5MapUnitMap (uint32_t PicSizeInMapUnits, cSlice* slice) {
// Generate wipe slice group map type MapUnit map (type 5) *

  uint32_t mapUnitsInSliceGroup0 = imin (
    (activePps->sliceGroupChangeRateMius1 + 1) * slice->sliceGroupChangeCycle, PicSizeInMapUnits);
  uint32_t sizeOfUpperLeftGroup = activePps->sliceGroupChangeDirectionFlag
                                    ? (PicSizeInMapUnits - mapUnitsInSliceGroup0)
                                    : mapUnitsInSliceGroup0;
  uint32_t k = 0;
  for (uint32_t j = 0; j < coding.picWidthMbs; j++)
    for (uint32_t i = 0; i < coding.picHeightMapUnits; i++)
      if (k++ < sizeOfUpperLeftGroup)
        mapUnitToSliceGroupMap[i * coding.picWidthMbs + j] = activePps->sliceGroupChangeDirectionFlag;
      else
        mapUnitToSliceGroupMap[i * coding.picWidthMbs + j] = 1 - activePps->sliceGroupChangeDirectionFlag;
  }
//}}}
//{{{
void cDecoder264::fmoGenerateType6MapUnitMap (uint32_t PicSizeInMapUnits) {
// Generate explicit slice group map type MapUnit map (type 6)

  for (uint32_t i = 0; i < PicSizeInMapUnits; i++)
    mapUnitToSliceGroupMap[i] = activePps->sliceGroupId[i];
  }
//}}}

//{{{
int cDecoder264::fmoGenerateMapUnitToSliceGroupMap (cSlice* slice) {
// Generates mapUnitToSliceGroupMap, called every time a new PPS is used

  uint32_t NumSliceGroupMapUnits = (activeSps->picHeightMapUnitsMinus1+1) * (activeSps->picWidthMbsMinus1+1);

  if (activePps->sliceGroupMapType == 6)
    if ((activePps->picSizeMapUnitsMinus1 + 1) != NumSliceGroupMapUnits)
      error ("wrong activePps->picSizeMapUnitsMinus1 for used activeSps and FMO type 6");

  // allocate memory for mapUnitToSliceGroupMap
  if (mapUnitToSliceGroupMap)
    free (mapUnitToSliceGroupMap);
  mapUnitToSliceGroupMap = (int*)malloc ((NumSliceGroupMapUnits) * sizeof (int));

  if (activePps->numSliceGroupsMinus1 == 0) {
    // only one slice group
    memset (mapUnitToSliceGroupMap, 0, NumSliceGroupMapUnits * sizeof (int));
    return 0;
    }

  switch (activePps->sliceGroupMapType) {
    case 0:
      fmoGenerateType0MapUnitMap (NumSliceGroupMapUnits);
      break;
    case 1:
      fmoGenerateType1MapUnitMap (NumSliceGroupMapUnits);
      break;
    case 2:
      fmoGenerateType2MapUnitMap (NumSliceGroupMapUnits);
      break;
    case 3:
      fmoGenerateType3MapUnitMap (NumSliceGroupMapUnits, slice);
      break;
    case 4:
      fmoGenerateType4MapUnitMap (NumSliceGroupMapUnits, slice);
      break;
    case 5:
      fmoGenerateType5MapUnitMap (NumSliceGroupMapUnits, slice);
      break;
    case 6:
      fmoGenerateType6MapUnitMap (NumSliceGroupMapUnits);
      break;
    default:
      error ("Illegal sliceGroupMapType");
      exit (-1);
    }

  return 0;
  }
//}}}
//{{{
int cDecoder264::fmoGenerateMbToSliceGroupMap (cSlice *slice) {
// Generates mbToSliceGroupMap from mapUnitToSliceGroupMap

  // allocate memory for mbToSliceGroupMap
  free (mbToSliceGroupMap);
  mbToSliceGroupMap = (int*)malloc ((picSizeInMbs) * sizeof (int));

  if ((activeSps->frameMbOnly)|| slice->fieldPic) {
    int* mbToSliceGroupMap1 = mbToSliceGroupMap;
    int* mapUnitToSliceGroupMap1 = mapUnitToSliceGroupMap;
    for (uint32_t i = 0; i < picSizeInMbs; i++)
      *mbToSliceGroupMap1++ = *mapUnitToSliceGroupMap1++;
    }
  else {
    if (activeSps->mbAffFlag  &&  (!slice->fieldPic))
      for (uint32_t i = 0; i < picSizeInMbs; i++)
        mbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i/2];
    else
      for (uint32_t i = 0; i < picSizeInMbs; i++)
        mbToSliceGroupMap[i] = mapUnitToSliceGroupMap[(i/(2*coding.picWidthMbs)) *
                                 coding.picWidthMbs + (i%coding.picWidthMbs)];
    }

  return 0;
  }
//}}}
//}}}

// output
//{{{
void cDecoder264::allocDecodedPicBuffers (sDecodedPic* decodedPic, sPicture* picture,
                                          int frameSize, int lumaSize, int lumaSizeX, int lumaSizeY,
                                          int chromaSizeX, int chromaSizeY) {

  if (decodedPic->bufSize != frameSize) {
    free (decodedPic->yBuf);

    decodedPic->bufSize = frameSize;
    decodedPic->yBuf = (uint8_t*)malloc (decodedPic->bufSize);
    decodedPic->uBuf = decodedPic->yBuf + lumaSize;
    decodedPic->vBuf = decodedPic->uBuf + ((frameSize - lumaSize)>>1);

    decodedPic->yuvFormat = picture->chromaFormatIdc;
    decodedPic->bitDepth = coding.picUnitBitSizeDisk;
    decodedPic->width = lumaSizeX;
    decodedPic->height = lumaSizeY;

    int symbolSizeInBytes = (coding.picUnitBitSizeDisk + 7) >> 3;
    decodedPic->yStride = lumaSizeX * symbolSizeInBytes;
    decodedPic->uvStride = chromaSizeX * symbolSizeInBytes;
    }
  }
//}}}
//{{{
void cDecoder264::clearPicture (sPicture* picture) {

  for (int i = 0; i < picture->sizeY; i++)
    for (int j = 0; j < picture->sizeX; j++)
      picture->imgY[i][j] = (sPixel)coding.dcPredValueComp[0];

  for (int i = 0; i < picture->sizeYcr;i++)
    for (int j = 0; j < picture->sizeXcr; j++)
      picture->imgUV[0][i][j] = (sPixel)coding.dcPredValueComp[1];

  for (int i = 0; i < picture->sizeYcr;i++)
    for (int j = 0; j < picture->sizeXcr; j++)
      picture->imgUV[1][i][j] = (sPixel)coding.dcPredValueComp[2];
  }
//}}}
//{{{
void cDecoder264::writeOutPicture (sPicture* picture) {

  if (picture->nonExisting)
    return;

  int cropLeft = picture->hasCrop ? kSubWidthC [picture->chromaFormatIdc] * picture->cropLeft : 0;
  int cropRight = picture->hasCrop ? kSubWidthC [picture->chromaFormatIdc] * picture->cropRight : 0;
  int cropTop = picture->hasCrop ?
    kSubHeightC[picture->chromaFormatIdc] * (2-picture->frameMbOnly) * picture->cropTop : 0;
  int cropBot = picture->hasCrop ?
    kSubHeightC[picture->chromaFormatIdc] * (2-picture->frameMbOnly) * picture->cropBot : 0;

  int symbolSizeInBytes = (coding.picUnitBitSizeDisk+7) >> 3;
  int chromaSizeX = picture->sizeXcr - picture->cropLeft - picture->cropRight;
  int chromaSizeY = picture->sizeYcr - (2 - picture->frameMbOnly) * picture->cropTop -
                                       (2 - picture->frameMbOnly) * picture->cropBot;
  int lumaSizeX = picture->sizeX - cropLeft - cropRight;
  int lumaSizeY = picture->sizeY - cropTop - cropBot;
  int lumaSize = lumaSizeX * lumaSizeY * symbolSizeInBytes;
  int frameSize = (lumaSizeX * lumaSizeY + 2 * (chromaSizeX * chromaSizeY)) * symbolSizeInBytes;

  sDecodedPic* decodedPic = allocDecodedPicture (outDecodedPics);
  if (!decodedPic->yBuf || (decodedPic->bufSize < frameSize))
    allocDecodedPicBuffers (decodedPic, picture, frameSize,
                            lumaSize, lumaSizeX, lumaSizeY, chromaSizeX, chromaSizeY);
  decodedPic->ok = true;
  decodedPic->poc = picture->framePoc;
  copyCropped (decodedPic->ok ? decodedPic->yBuf : decodedPic->yBuf + lumaSizeX * symbolSizeInBytes,
               picture->imgY, picture->sizeX, picture->sizeY, symbolSizeInBytes,
               cropLeft, cropRight, cropTop, cropBot, decodedPic->yStride);

  cropLeft = picture->cropLeft;
  cropRight = picture->cropRight;
  cropTop = (2 - picture->frameMbOnly) * picture->cropTop;
  cropBot = (2 - picture->frameMbOnly) * picture->cropBot;
  copyCropped (decodedPic->ok ? decodedPic->uBuf : decodedPic->uBuf + chromaSizeX * symbolSizeInBytes,
               picture->imgUV[0], picture->sizeXcr, picture->sizeYcr, symbolSizeInBytes,
               cropLeft, cropRight, cropTop, cropBot, decodedPic->uvStride);
  copyCropped (decodedPic->ok ? decodedPic->vBuf : decodedPic->vBuf + chromaSizeX * symbolSizeInBytes,
               picture->imgUV[1], picture->sizeXcr, picture->sizeYcr, symbolSizeInBytes,
               cropLeft, cropRight, cropTop, cropBot, decodedPic->uvStride);
  }
//}}}
//{{{
void cDecoder264::flushPendingOut() {

  if (pendingOutPicStructure != eFrame)
    writeOutPicture (pendingOut);

  freeMem2Dpel (pendingOut->imgY);
  pendingOut->imgY = NULL;

  freeMem3Dpel (pendingOut->imgUV);
  pendingOut->imgUV = NULL;

  pendingOutPicStructure = eFrame;
  }
//}}}
//{{{
void cDecoder264::writePicture (sPicture* picture, int realStructure) {

  if (realStructure == eFrame) {
    flushPendingOut();
    writeOutPicture (picture);
    return;
    }

  if (realStructure == pendingOutPicStructure) {
    flushPendingOut();
    writePicture (picture, realStructure);
    return;
    }

  if (pendingOutPicStructure == eFrame) {
    //{{{  output frame
    pendingOut->sizeX = picture->sizeX;
    pendingOut->sizeY = picture->sizeY;
    pendingOut->sizeXcr = picture->sizeXcr;
    pendingOut->sizeYcr = picture->sizeYcr;
    pendingOut->chromaFormatIdc = picture->chromaFormatIdc;

    pendingOut->frameMbOnly = picture->frameMbOnly;
    pendingOut->hasCrop = picture->hasCrop;
    if (pendingOut->hasCrop) {
      pendingOut->cropLeft = picture->cropLeft;
      pendingOut->cropRight = picture->cropRight;
      pendingOut->cropTop = picture->cropTop;
      pendingOut->cropBot = picture->cropBot;
      }

    getMem2Dpel (&pendingOut->imgY, pendingOut->sizeY, pendingOut->sizeX);
    getMem3Dpel (&pendingOut->imgUV, 2, pendingOut->sizeYcr, pendingOut->sizeXcr);
    clearPicture (pendingOut);

    // copy first field
    int add = (realStructure == eTopField) ? 0 : 1;
    for (int i = 0; i < pendingOut->sizeY; i += 2)
      memcpy (pendingOut->imgY[(i+add)], picture->imgY[(i+add)], picture->sizeX * sizeof(sPixel));
    for (int i = 0; i < pendingOut->sizeYcr; i += 2) {
      memcpy (pendingOut->imgUV[0][(i+add)], picture->imgUV[0][(i+add)], picture->sizeXcr * sizeof(sPixel));
      memcpy (pendingOut->imgUV[1][(i+add)], picture->imgUV[1][(i+add)], picture->sizeXcr * sizeof(sPixel));
      }

    pendingOutPicStructure = realStructure;
    }
    //}}}
  else {
    if ((pendingOut->sizeX != picture->sizeX) || (pendingOut->sizeY != picture->sizeY) ||
        (pendingOut->frameMbOnly != picture->frameMbOnly) ||
        (pendingOut->hasCrop != picture->hasCrop) ||
        (pendingOut->hasCrop &&
         ((pendingOut->cropLeft != picture->cropLeft) || (pendingOut->cropRight != picture->cropRight) ||
          (pendingOut->cropTop != picture->cropTop) || (pendingOut->cropBot != picture->cropBot)))) {
      flushPendingOut();
      writePicture (picture, realStructure);
      return;
      }

    // copy second field
    int add = (realStructure == eTopField) ? 0 : 1;
    for (int i = 0; i < pendingOut->sizeY; i += 2)
      memcpy (pendingOut->imgY[i+add], picture->imgY[i+add], picture->sizeX * sizeof(sPixel));

    for (int i = 0; i < pendingOut->sizeYcr; i+=2) {
      memcpy (pendingOut->imgUV[0][i+add], picture->imgUV[0][i+add], picture->sizeXcr * sizeof(sPixel));
      memcpy (pendingOut->imgUV[1][i+add], picture->imgUV[1][i+add], picture->sizeXcr * sizeof(sPixel));
      }

    flushPendingOut();
    }
  }
//}}}
//{{{
void cDecoder264::writeUnpairedField (cFrameStore* frameStore) {

  if (frameStore->isUsed & 0x01) {
    // we have a top field, construct an empty bottom field
    sPicture* picture = frameStore->topField;
    frameStore->botField = sPicture::allocPicture (this, eBotField, picture->sizeX, 2 * picture->sizeY,
                                                          picture->sizeXcr, 2 * picture->sizeYcr, 1);
    frameStore->botField->chromaFormatIdc = picture->chromaFormatIdc;

    clearPicture (frameStore->botField);
    frameStore->dpbCombineField (this);
    writePicture (frameStore->frame, eTopField);
    }

  if (frameStore->isUsed & 0x02) {
    // we have a bottom field, construct an empty top field
    sPicture* picture = frameStore->botField;
    frameStore->topField = sPicture::allocPicture (this, eTopField, picture->sizeX, 2*picture->sizeY,
                                         picture->sizeXcr, 2*picture->sizeYcr, 1);
    frameStore->topField->chromaFormatIdc = picture->chromaFormatIdc;
    clearPicture (frameStore->topField);

    frameStore->topField->hasCrop = frameStore->botField->hasCrop;
    if (frameStore->topField->hasCrop) {
      frameStore->topField->cropTop = frameStore->botField->cropTop;
      frameStore->topField->cropBot = frameStore->botField->cropBot;
      frameStore->topField->cropLeft = frameStore->botField->cropLeft;
      frameStore->topField->cropRight = frameStore->botField->cropRight;
      }

    frameStore->dpbCombineField (this);
    writePicture (frameStore->frame, eBotField);
    }

  frameStore->isUsed = 3;
  }
//}}}
//{{{
void cDecoder264::flushDirectOutput() {

  writeUnpairedField (outBuffer);

  sPicture::freePicture (outBuffer->frame);
  sPicture::freePicture (outBuffer->topField);
  sPicture::freePicture (outBuffer->botField);

  outBuffer->isUsed = 0;
  }
//}}}

//{{{
void cDecoder264::makeFramePictureJV() {

  picture = decPictureJV[0];

  // copy;
  if (picture->usedForRef) {
    int nsize = (picture->sizeY/BLOCK_SIZE)*(picture->sizeX/BLOCK_SIZE)*sizeof(sPicMotion);
    memcpy (&(picture->mvInfoJV[PLANE_Y][0][0]), &(decPictureJV[PLANE_Y]->mvInfo[0][0]), nsize);
    memcpy (&(picture->mvInfoJV[PLANE_U][0][0]), &(decPictureJV[PLANE_U]->mvInfo[0][0]), nsize);
    memcpy (&(picture->mvInfoJV[PLANE_V][0][0]), &(decPictureJV[PLANE_V]->mvInfo[0][0]), nsize);
    }

  // This could be done with pointers and seems not necessary
  for (int uv = 0; uv < 2; uv++) {
    for (int line = 0; line < coding.height; line++) {
      int nsize = sizeof(sPixel) * coding.width;
      memcpy (picture->imgUV[uv][line], decPictureJV[uv+1]->imgY[line], nsize );
      }
    sPicture::freePicture (decPictureJV[uv+1]);
    }
  }
//}}}
