//{{{  includes
#include "global.h"
#include "syntaxElement.h"

#include "block.h"
#include "buffer.h"
#include "errorConceal.h"
#include "macroblock.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "image.h"
#include "mbAccess.h"
#include "binaryArithmeticDecode.h"
#include "transform.h"
#include "mcPred.h"
#include "quant.h"
#include "mbPred.h"
//}}}
static const sMotionVec kZeroMv = {0, 0};

//{{{
int mb_pred_intra4x4 (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  sSlice* slice = mb->slice;
  int yuv = picture->chromaFormatIdc - 1;
  int i=0, j=0,k, j4=0,i4=0;
  int j_pos, i_pos;
  int ioff,joff;
  int block8x8;   // needed for ABT
  mb->iTrans4x4 = (mb->isLossless == FALSE) ? itrans4x4 : Inv_Residual_trans_4x4;

  for (block8x8 = 0; block8x8 < 4; block8x8++)
  {
    for (k = block8x8 * 4; k < block8x8 * 4 + 4; k ++)
    {
      i =  (decode_block_scan[k] & 3);
      j = ((decode_block_scan[k] >> 2) & 3);

      ioff = (i << 2);
      joff = (j << 2);
      i4   = mb->blockX + i;
      j4   = mb->blockY + j;
      j_pos = j4 * BLOCK_SIZE;
      i_pos = i4 * BLOCK_SIZE;

      // PREDICTION
      //===== INTRA PREDICTION =====
      if (slice->intraPred4x4(mb, plane, ioff,joff,i4,j4) == SEARCH_SYNC)  /* make 4x4 prediction block mpr from given prediction decoder->mb_mode */
        return SEARCH_SYNC;                   /* bit error */
      // =============== 4x4 itrans ================
      // -------------------------------------------
      mb->iTrans4x4  (mb, plane, ioff, joff);
      copy_Image_4x4 (&pixel[j_pos], &slice->mbRec[plane][joff], i_pos, ioff);
    }
  }

  // chroma decoding** *****************************************************
  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444))
    intra_cr_decoding(mb, yuv);

  if (mb->codedBlockPattern != 0)
    slice->isResetCoef = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_intra16x16 (sMacroBlock* mb, eColorPlane plane, sPicture* picture)
{
  int yuv = picture->chromaFormatIdc - 1;

  mb->slice->intraPred16x16(mb, plane, mb->i16mode);
  mb->ipmode_DPCM = (char) mb->i16mode; //For residual DPCM
  // =============== 4x4 itrans ================
  // -------------------------------------------
  iMBtrans4x4(mb, plane, 0);

  // chroma decoding** *****************************************************
  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444))
  {
    intra_cr_decoding(mb, yuv);
  }

  mb->slice->isResetCoef = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_intra8x8 (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  sSlice* slice = mb->slice;
  int yuv = picture->chromaFormatIdc - 1;

  int block8x8;   // needed for ABT
  mb->iTrans8x8 = (mb->isLossless == FALSE) ? itrans8x8 : Inv_Residual_trans_8x8;

  for (block8x8 = 0; block8x8 < 4; block8x8++)
  {
    //=========== 8x8 BLOCK TYPE ============
    int ioff = (block8x8 & 0x01) << 3;
    int joff = (block8x8 >> 1  ) << 3;

    //PREDICTION
    slice->intraPred8x8(mb, plane, ioff, joff);
    if (mb->codedBlockPattern & (1 << block8x8))
      mb->iTrans8x8    (mb, plane, ioff,joff);      // use inverse integer transform and make 8x8 block m7 from prediction block mpr
    else
      icopy8x8(mb, plane, ioff,joff);

    copy_Image_8x8(&pixel[mb->pixY + joff], &slice->mbRec[plane][joff], mb->pixX + ioff, ioff);
  }
  // chroma decoding** *****************************************************
  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444))
  {
    intra_cr_decoding(mb, yuv);
  }

  if (mb->codedBlockPattern != 0)
    slice->isResetCoef = FALSE;
  return 1;
}
//}}}

//{{{
static void set_chroma_vector (sMacroBlock* mb)
{
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  if (!slice->mbAffFrame)
  {
    if(slice->picStructure == eTopField)
    {
      int k,l;
      for (l = LIST_0; l <= (LIST_1); l++)
      {
        for(k = 0; k < slice->listXsize[l]; k++)
        {
          if(slice->picStructure != slice->listX[l][k]->picStructure)
            slice->chromaVectorAdjust[l][k] = -2;
          else
            slice->chromaVectorAdjust[l][k] = 0;
        }
      }
    }
    else if(slice->picStructure == eBotField)
    {
      int k,l;
      for (l = LIST_0; l <= (LIST_1); l++)
      {
        for(k = 0; k < slice->listXsize[l]; k++)
        {
          if (slice->picStructure != slice->listX[l][k]->picStructure)
            slice->chromaVectorAdjust[l][k] = 2;
          else
            slice->chromaVectorAdjust[l][k] = 0;
        }
      }
    }
    else
    {
      int k,l;
      for (l = LIST_0; l <= (LIST_1); l++)
      {
        for(k = 0; k < slice->listXsize[l]; k++)
        {
          slice->chromaVectorAdjust[l][k] = 0;
        }
      }
    }
  }
  else
  {
    int mb_nr = (mb->mbIndexX & 0x01);
    int k,l;

    //////////////////////////
    // find out the correct list offsets
    if (mb->mbField)
    {
      int listOffset = mb->listOffset;

      for (l = LIST_0 + listOffset; l <= (LIST_1 + listOffset); l++)
      {
        for(k = 0; k < slice->listXsize[l]; k++)
        {
          if(mb_nr == 0 && slice->listX[l][k]->picStructure == eBotField)
            slice->chromaVectorAdjust[l][k] = -2;
          else if(mb_nr == 1 && slice->listX[l][k]->picStructure == eTopField)
            slice->chromaVectorAdjust[l][k] = 2;
          else
            slice->chromaVectorAdjust[l][k] = 0;
        }
      }
    }
    else
    {
      for (l = LIST_0; l <= (LIST_1); l++)
      {
        for(k = 0; k < slice->listXsize[l]; k++)
        {
          slice->chromaVectorAdjust[l][k] = 0;
        }
      }
    }
  }

  slice->maxMbVmvR = (slice->picStructure != eFrame || ( mb->mbField )) ? decoder->coding.maxVmvR >> 1 :
                                                                         decoder->coding.maxVmvR;
}
//}}}

//{{{
int mb_pred_skip (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  set_chroma_vector(mb);

  perform_mc(mb, plane, picture, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

  copy_Image_16x16(&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);

  if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444))
  {
    copy_Image(&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
    copy_Image(&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
  }
  return 1;
}
//}}}
//{{{
int mb_pred_sp_skip (sMacroBlock* mb, eColorPlane plane, sPicture* picture)
{
  set_chroma_vector(mb);

  perform_mc(mb, plane, picture, LIST_0, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  iTransform(mb, plane, 1);
  return 1;
}
//}}}

//{{{
int mb_pred_p_inter8x8 (sMacroBlock* mb, eColorPlane plane, sPicture* picture)
{
  int block8x8;   // needed for ABT
  int i=0, j=0,k;

  sSlice* slice = mb->slice;
  int smb = slice->sliceType == eSliceSP && (mb->isIntraBlock == FALSE);

  set_chroma_vector(mb);

  for (block8x8=0; block8x8<4; block8x8++)
  {
    int mv_mode  = mb->b8mode[block8x8];
    int predDir = mb->b8pdir[block8x8];

    int k_start = (block8x8 << 2);
    int k_inc = (mv_mode == SMB8x4) ? 2 : 1;
    int k_end = (mv_mode == SMB8x8) ? k_start + 1 : ((mv_mode == SMB4x4) ? k_start + 4 : k_start + k_inc + 1);

    int blockSizeX = ( mv_mode == SMB8x4 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;
    int blockSizeY = ( mv_mode == SMB4x8 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;

    for (k = k_start; k < k_end; k += k_inc)
    {
      i =  (decode_block_scan[k] & 3);
      j = ((decode_block_scan[k] >> 2) & 3);
      perform_mc(mb, plane, picture, predDir, i, j, blockSizeX, blockSizeY);
    }
  }

  iTransform (mb, plane, smb);

  if (mb->codedBlockPattern != 0)
    slice->isResetCoef = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_p_inter16x16 (sMacroBlock* mb, eColorPlane plane, sPicture* picture)
{
  sSlice* slice = mb->slice;
  int smb = (slice->sliceType == eSliceSP);

  set_chroma_vector(mb);
  perform_mc (mb, plane, picture, mb->b8pdir[0], 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  iTransform (mb, plane, smb);

  if (mb->codedBlockPattern != 0)
    slice->isResetCoef = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_p_inter16x8 (sMacroBlock* mb, eColorPlane plane, sPicture* picture)
{
  sSlice* slice = mb->slice;
  int smb = (slice->sliceType == eSliceSP);

  set_chroma_vector(mb);

  perform_mc(mb, plane, picture, mb->b8pdir[0], 0, 0, MB_BLOCK_SIZE, BLOCK_SIZE_8x8);
  perform_mc(mb, plane, picture, mb->b8pdir[2], 0, 2, MB_BLOCK_SIZE, BLOCK_SIZE_8x8);
  iTransform(mb, plane, smb);

  if (mb->codedBlockPattern != 0)
    slice->isResetCoef = FALSE;
  return 1;
}
//}}}
//{{{
int mb_pred_p_inter8x16 (sMacroBlock* mb, eColorPlane plane, sPicture* picture)
{
  sSlice* slice = mb->slice;
  int smb = (slice->sliceType == eSliceSP);

  set_chroma_vector(mb);

  perform_mc(mb, plane, picture, mb->b8pdir[0], 0, 0, BLOCK_SIZE_8x8, MB_BLOCK_SIZE);
  perform_mc(mb, plane, picture, mb->b8pdir[1], 2, 0, BLOCK_SIZE_8x8, MB_BLOCK_SIZE);
  iTransform(mb, plane, smb);

  if (mb->codedBlockPattern != 0)
    slice->isResetCoef = FALSE;
  return 1;
}
//}}}

//{{{
static inline void update_neighbor_mvs (sPicMotionParam** motion, const sPicMotionParam* mvInfo, int i4)
{
  (*motion++)[i4 + 1] = *mvInfo;
  (*motion  )[i4    ] = *mvInfo;
  (*motion  )[i4 + 1] = *mvInfo;
}
//}}}

//{{{
int mb_pred_b_d8x8temporal (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture) {

  short refIndex;
  int refList;

  int k, i, j, i4, j4, j6;
  int block8x8;   // needed for ABT
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;
  sPicMotionParam *mvInfo = NULL, *colocated = NULL;

  int listOffset = mb->listOffset;
  sPicture** list0 = slice->listX[LIST_0 + listOffset];
  sPicture** list1 = slice->listX[LIST_1 + listOffset];

  set_chroma_vector(mb);

  for (block8x8 = 0; block8x8 < 4; block8x8++) {
    int predDir = mb->b8pdir[block8x8];
    int k_start = (block8x8 << 2);
    int k_end = k_start + 1;
    for (k = k_start; k < k_start + BLOCK_MULTIPLE; k ++) {
      i =  (decode_block_scan[k] & 3);
      j = ((decode_block_scan[k] >> 2) & 3);
      i4   = mb->blockX + i;
      j4   = mb->blockY + j;
      j6   = mb->blockYaff + j;
      mvInfo = &picture->mvInfo[j4][i4];
      colocated = &list1[0]->mvInfo[RSD(j6)][RSD(i4)];
      if(mb->decoder->coding.isSeperateColourPlane && mb->decoder->coding.yuvFormat==YUV444)
        colocated = &list1[0]->JVmv_info[mb->slice->colourPlaneId][RSD(j6)][RSD(i4)];
      if (slice->mbAffFrame) {
        assert(decoder->activeSPS->direct_8x8_inference_flag);
        if (!mb->mbField && ((slice->listX[LIST_1][0]->iCodingType==eFrameMbPairCoding && slice->listX[LIST_1][0]->motion.mbField[mb->mbIndexX]) ||
            (slice->listX[LIST_1][0]->iCodingType==eFieldCoding))) {
          if (iabs(picture->poc - slice->listX[LIST_1+4][0]->poc) > iabs(picture->poc -slice->listX[LIST_1+2][0]->poc))
            colocated = decoder->activeSPS->direct_8x8_inference_flag
                          ? &slice->listX[LIST_1+2][0]->mvInfo[RSD(j6) >> 1][RSD(i4)]
                          : &slice->listX[LIST_1+2][0]->mvInfo[j6 >> 1][i4];
          else
            colocated = decoder->activeSPS->direct_8x8_inference_flag
                          ? &slice->listX[LIST_1+4][0]->mvInfo[RSD(j6) >> 1][RSD(i4)]
                          : &slice->listX[LIST_1+4][0]->mvInfo[j6 >> 1][i4];
          }
        }
      else if (!decoder->activeSPS->frameMbOnly &&
               (!slice->fieldPic && slice->listX[LIST_1][0]->iCodingType != eFrameCoding)) {
        if (iabs(picture->poc - list1[0]->botField->poc)> iabs(picture->poc -list1[0]->topField->poc) )
          colocated = decoder->activeSPS->direct_8x8_inference_flag ?
            &list1[0]->topField->mvInfo[RSD(j6)>>1][RSD(i4)] : &list1[0]->topField->mvInfo[j6>>1][i4];
        else
          colocated = decoder->activeSPS->direct_8x8_inference_flag ?
            &list1[0]->botField->mvInfo[RSD(j6)>>1][RSD(i4)] : &list1[0]->botField->mvInfo[j6>>1][i4];
        }
      else if (!decoder->activeSPS->frameMbOnly && slice->fieldPic &&
               slice->picStructure != list1[0]->picStructure && list1[0]->codedFrame) {
        if (slice->picStructure == eTopField)
          colocated = decoder->activeSPS->direct_8x8_inference_flag
                        ? &list1[0]->frame->topField->mvInfo[RSD(j6)][RSD(i4)]
                        : &list1[0]->frame->topField->mvInfo[j6][i4];
        else
          colocated = decoder->activeSPS->direct_8x8_inference_flag
                        ? &list1[0]->frame->botField->mvInfo[RSD(j6)][RSD(i4)]
                        : &list1[0]->frame->botField->mvInfo[j6][i4];
        }

      assert (predDir<=2);
      refList = (colocated->refIndex[LIST_0]== -1 ? LIST_1 : LIST_0);
      refIndex =  colocated->refIndex[refList];
      if (refIndex == -1) {
        // co-located is intra mode
        mvInfo->mv[LIST_0] = kZeroMv;
        mvInfo->mv[LIST_1] = kZeroMv;

        mvInfo->refIndex[LIST_0] = 0;
        mvInfo->refIndex[LIST_1] = 0;
        }
      else {
        // co-located skip or inter mode
        int mapped_idx = 0;
        int iref;
        if ((slice->mbAffFrame &&
            ((mb->mbField && colocated->refPic[refList]->picStructure == eFrame) ||
             (!mb->mbField && colocated->refPic[refList]->picStructure != eFrame))) ||
             (!slice->mbAffFrame && ((slice->fieldPic == 0 &&
               colocated->refPic[refList]->picStructure != eFrame) ||
             (slice->fieldPic==1 && colocated->refPic[refList]->picStructure == eFrame)))) {
          for (iref = 0; iref < imin(slice->numRefIndexActive[LIST_0], slice->listXsize[LIST_0 + listOffset]);iref++) {
            if(slice->listX[LIST_0 + listOffset][iref]->topField == colocated->refPic[refList] ||
              slice->listX[LIST_0 + listOffset][iref]->botField == colocated->refPic[refList] ||
              slice->listX[LIST_0 + listOffset][iref]->frame == colocated->refPic[refList]) {
              if ((slice->fieldPic == 1) && (slice->listX[LIST_0 + listOffset][iref]->picStructure != slice->picStructure))
                mapped_idx = INVALIDINDEX;
              else {
                mapped_idx = iref;
                break;
                }
              }
            else 
              mapped_idx = INVALIDINDEX;
            }
          }
        else {
          for (iref = 0; iref < imin(slice->numRefIndexActive[LIST_0], slice->listXsize[LIST_0 + listOffset]);iref++) {
            if(slice->listX[LIST_0 + listOffset][iref] == colocated->refPic[refList]) {
              mapped_idx = iref;
              break;
              }
            else 
              mapped_idx=INVALIDINDEX;
            }
          }

        if (INVALIDINDEX != mapped_idx) {
          int mv_scale = slice->mvscale[LIST_0 + listOffset][mapped_idx];
          int mvY = colocated->mv[refList].mvY;
          if ((slice->mbAffFrame && !mb->mbField && colocated->refPic[refList]->picStructure!=eFrame) ||
              (!slice->mbAffFrame && slice->fieldPic==0 && colocated->refPic[refList]->picStructure!=eFrame) )
            mvY *= 2;
          else if ((slice->mbAffFrame && mb->mbField && colocated->refPic[refList]->picStructure==eFrame) ||
                   (!slice->mbAffFrame && slice->fieldPic==1 && colocated->refPic[refList]->picStructure==eFrame) )
            mvY /= 2;

          // In such case, an array is needed for each different reference.
          if (mv_scale == 9999 || slice->listX[LIST_0 + listOffset][mapped_idx]->isLongTerm) {
            mvInfo->mv[LIST_0].mvX = colocated->mv[refList].mvX;
            mvInfo->mv[LIST_0].mvY = (short) mvY;
            mvInfo->mv[LIST_1] = kZeroMv;
            }
          else {
            mvInfo->mv[LIST_0].mvX = (short) ((mv_scale * colocated->mv[refList].mvX + 128 ) >> 8);
            mvInfo->mv[LIST_0].mvY = (short) ((mv_scale * mvY/*colocated->mv[refList].mvY*/ + 128 ) >> 8);

            mvInfo->mv[LIST_1].mvX = (short) (mvInfo->mv[LIST_0].mvX - colocated->mv[refList].mvX);
            mvInfo->mv[LIST_1].mvY = (short) (mvInfo->mv[LIST_0].mvY - mvY/*colocated->mv[refList].mvY*/);
            }

          mvInfo->refIndex[LIST_0] = (char) mapped_idx; //colocated->refIndex[refList];
          mvInfo->refIndex[LIST_1] = 0;
          }
        else if (INVALIDINDEX == mapped_idx)
          error ("temporal direct error: colocated block has ref that is unavailable");
        }

      // store reference picture ID determined by direct mode
      mvInfo->refPic[LIST_0] = list0[(short)mvInfo->refIndex[LIST_0]];
      mvInfo->refPic[LIST_1] = list1[(short)mvInfo->refIndex[LIST_1]];
      }

    for (k = k_start; k < k_end; k ++) {
      int i =  (decode_block_scan[k] & 3);
      int j = ((decode_block_scan[k] >> 2) & 3);
      perform_mc (mb, plane, picture, predDir, i, j, SMB_BLOCK_SIZE, SMB_BLOCK_SIZE);
      }
    }

  if (mb->codedBlockPattern == 0) {
    copy_Image_16x16 (&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);

    if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
      copy_Image (&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      copy_Image (&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      }
    }
  else {
    iTransform (mb, plane, 0);
    slice->isResetCoef = FALSE;
    }
  return 1;
  }
//}}}
//{{{
int mb_pred_b_d4x4temporal (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  short refIndex;
  int refList;

  int k;
  int block8x8;   // needed for ABT
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int listOffset = mb->listOffset;
  sPicture** list0 = slice->listX[LIST_0 + listOffset];
  sPicture** list1 = slice->listX[LIST_1 + listOffset];

  set_chroma_vector(mb);

  for (block8x8=0; block8x8<4; block8x8++)
  {
    int predDir = mb->b8pdir[block8x8];

    int k_start = (block8x8 << 2);
    int k_end = k_start + BLOCK_MULTIPLE;

    for (k = k_start; k < k_end; k ++)
    {

      int i =  (decode_block_scan[k] & 3);
      int j = ((decode_block_scan[k] >> 2) & 3);
      int i4   = mb->blockX + i;
      int j4   = mb->blockY + j;
      int j6   = mb->blockYaff + j;
      sPicMotionParam *mvInfo = &picture->mvInfo[j4][i4];
      sPicMotionParam *colocated = &list1[0]->mvInfo[j6][i4];
      if(mb->decoder->coding.isSeperateColourPlane && mb->decoder->coding.yuvFormat==YUV444)
        colocated = &list1[0]->JVmv_info[mb->slice->colourPlaneId][RSD(j6)][RSD(i4)];
      assert (predDir<=2);

      refList = (colocated->refIndex[LIST_0]== -1 ? LIST_1 : LIST_0);
      refIndex =  colocated->refIndex[refList];

      if(refIndex==-1) // co-located is intra mode
      {
        mvInfo->mv[LIST_0] = kZeroMv;
        mvInfo->mv[LIST_1] = kZeroMv;

        mvInfo->refIndex[LIST_0] = 0;
        mvInfo->refIndex[LIST_1] = 0;
      }
      else // co-located skip or inter mode
      {
        int mapped_idx=0;
        int iref;

        for (iref=0;iref<imin(slice->numRefIndexActive[LIST_0], slice->listXsize[LIST_0 + listOffset]);iref++)
        {
          if(slice->listX[LIST_0 + listOffset][iref] == colocated->refPic[refList])
          {
            mapped_idx=iref;
            break;
          }
          else //! invalid index. Default to zero even though this case should not happen
          {
            mapped_idx=INVALIDINDEX;
          }
        }
        if (INVALIDINDEX == mapped_idx)
        {
          error("temporal direct error: colocated block has ref that is unavailable");
        }
        else
        {
          int mv_scale = slice->mvscale[LIST_0 + listOffset][mapped_idx];

          //! In such case, an array is needed for each different reference.
          if (mv_scale == 9999 || slice->listX[LIST_0+listOffset][mapped_idx]->isLongTerm)
          {
            mvInfo->mv[LIST_0] = colocated->mv[refList];
            mvInfo->mv[LIST_1] = kZeroMv;
          }
          else
          {
            mvInfo->mv[LIST_0].mvX = (short) ((mv_scale * colocated->mv[refList].mvX + 128 ) >> 8);
            mvInfo->mv[LIST_0].mvY = (short) ((mv_scale * colocated->mv[refList].mvY + 128 ) >> 8);

            mvInfo->mv[LIST_1].mvX = (short) (mvInfo->mv[LIST_0].mvX - colocated->mv[refList].mvX);
            mvInfo->mv[LIST_1].mvY = (short) (mvInfo->mv[LIST_0].mvY - colocated->mv[refList].mvY);
          }

          mvInfo->refIndex[LIST_0] = (char) mapped_idx; //colocated->refIndex[refList];
          mvInfo->refIndex[LIST_1] = 0;
        }
      }
      // store reference picture ID determined by direct mode
      mvInfo->refPic[LIST_0] = list0[(short)mvInfo->refIndex[LIST_0]];
      mvInfo->refPic[LIST_1] = list1[(short)mvInfo->refIndex[LIST_1]];
    }

    for (k = k_start; k < k_end; k ++)
    {
      int i =  (decode_block_scan[k] & 3);
      int j = ((decode_block_scan[k] >> 2) & 3);
      perform_mc(mb, plane, picture, predDir, i, j, BLOCK_SIZE, BLOCK_SIZE);

    }
  }

  if (mb->codedBlockPattern == 0)
  {
    copy_Image_16x16(&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);

    if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444))
    {
      copy_Image(&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      copy_Image(&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
    }
  }
  else
  {
    iTransform(mb, plane, 0);
    slice->isResetCoef = FALSE;
  }

  return 1;
}
//}}}

//{{{
int mb_pred_b_d8x8spatial (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  char l0_rFrame = -1, l1_rFrame = -1;
  sMotionVec pmvl0 = kZeroMv, pmvl1 = kZeroMv;
  int i4, j4;
  int block8x8;
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPicMotionParam* mvInfo;
  int listOffset = mb->listOffset;
  sPicture** list0 = slice->listX[LIST_0 + listOffset];
  sPicture** list1 = slice->listX[LIST_1 + listOffset];

  int predDir = 0;

  set_chroma_vector (mb);
  prepare_direct_params (mb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

  if (l0_rFrame == 0 || l1_rFrame == 0) {
    int is_not_moving;
    for (block8x8 = 0; block8x8 < 4; block8x8++) {
      int k_start = (block8x8 << 2);
      int i  =  (decode_block_scan[k_start] & 3);
      int j  = ((decode_block_scan[k_start] >> 2) & 3);
      i4  = mb->blockX + i;
      j4  = mb->blockY + j;

      is_not_moving = (get_colocated_info_8x8(mb, list1[0], i4, mb->blockYaff + j) == 0);
      mvInfo = &picture->mvInfo[j4][i4];

      if (l1_rFrame == -1) {
        if (is_not_moving) {
          mvInfo->refPic[LIST_0] = list0[0];
          mvInfo->refPic[LIST_1] = NULL;
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_0] = 0;
          mvInfo->refIndex[LIST_1] = -1;
          }
        else {
          mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
          mvInfo->refPic[LIST_1] = NULL;
          mvInfo->mv[LIST_0] = pmvl0;
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_0] = l0_rFrame;
          mvInfo->refIndex[LIST_1] = -1;
          }
        predDir = 0;
        }
      else if (l0_rFrame == -1) {
        if (is_not_moving) {
          mvInfo->refPic[LIST_0] = NULL;
          mvInfo->refPic[LIST_1] = list1[0];
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_0] = -1;
          mvInfo->refIndex[LIST_1] = 0;
          }
        else {
          mvInfo->refPic[LIST_0] = NULL;
          mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->mv[LIST_1] = pmvl1;
          mvInfo->refIndex[LIST_0] = -1;
          mvInfo->refIndex[LIST_1] = l1_rFrame;
          }
        predDir = 1;
        }

      else {
        if (l0_rFrame == 0 && ((is_not_moving))) {
          mvInfo->refPic[LIST_0] = list0[0];
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->refIndex[LIST_0] = 0;
          }
        else {
          mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
          mvInfo->mv[LIST_0] = pmvl0;
          mvInfo->refIndex[LIST_0] = l0_rFrame;
          }

        if  (l1_rFrame == 0 && ((is_not_moving))) {
          mvInfo->refPic[LIST_1] = list1[0];
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_1]    = 0;
          }
        else {
          mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
          mvInfo->mv[LIST_1] = pmvl1;
          mvInfo->refIndex[LIST_1] = l1_rFrame;
          }
        predDir = 2;
        }

      update_neighbor_mvs (&picture->mvInfo[j4], mvInfo, i4);
      perform_mc (mb, plane, picture, predDir, i, j, SMB_BLOCK_SIZE, SMB_BLOCK_SIZE);
      }
    }
  else {
    if (l0_rFrame < 0 && l1_rFrame < 0) {
      predDir = 2;
      for (j4 = mb->blockY; j4 < mb->blockY + BLOCK_MULTIPLE; j4 += 2) {
        for (i4 = mb->blockX; i4 < mb->blockX + BLOCK_MULTIPLE; i4 += 2) {
          mvInfo = &picture->mvInfo[j4][i4];
          mvInfo->refPic[LIST_0] = list0[0];
          mvInfo->refPic[LIST_1] = list1[0];
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_0] = 0;
          mvInfo->refIndex[LIST_1] = 0;
          update_neighbor_mvs (&picture->mvInfo[j4], mvInfo, i4);
          }
        }
      }

    else if (l1_rFrame == -1) {
      predDir = 0;
      for (j4 = mb->blockY; j4 < mb->blockY + BLOCK_MULTIPLE; j4 += 2) {
        for (i4 = mb->blockX; i4 < mb->blockX + BLOCK_MULTIPLE; i4 += 2) {
          mvInfo = &picture->mvInfo[j4][i4];
          mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
          mvInfo->refPic[LIST_1] = NULL;
          mvInfo->mv[LIST_0] = pmvl0;
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_0] = l0_rFrame;
          mvInfo->refIndex[LIST_1] = -1;
          update_neighbor_mvs (&picture->mvInfo[j4], mvInfo, i4);
          }
        }
      }

    else if (l0_rFrame == -1) {
      predDir = 1;
      for (j4 = mb->blockY; j4 < mb->blockY + BLOCK_MULTIPLE; j4 += 2) {
        for (i4 = mb->blockX; i4 < mb->blockX + BLOCK_MULTIPLE; i4 += 2) {
          mvInfo = &picture->mvInfo[j4][i4];
          mvInfo->refPic[LIST_0] = NULL;
          mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->mv[LIST_1] = pmvl1;
          mvInfo->refIndex[LIST_0] = -1;
          mvInfo->refIndex[LIST_1] = l1_rFrame;
          update_neighbor_mvs (&picture->mvInfo[j4], mvInfo, i4);
          }
        }
      }

    else {
      predDir = 2;
      for (j4 = mb->blockY; j4 < mb->blockY + BLOCK_MULTIPLE; j4 += 2) {
        for (i4 = mb->blockX; i4 < mb->blockX + BLOCK_MULTIPLE; i4 += 2) {
          mvInfo = &picture->mvInfo[j4][i4];
          mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
          mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
          mvInfo->mv[LIST_0] = pmvl0;
          mvInfo->mv[LIST_1] = pmvl1;
          mvInfo->refIndex[LIST_0] = l0_rFrame;
          mvInfo->refIndex[LIST_1] = l1_rFrame;
          update_neighbor_mvs (&picture->mvInfo[j4], mvInfo, i4);
          }
        }
      }

    // Now perform Motion Compensation
    perform_mc (mb, plane, picture, predDir, 0, 0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
    }

  if (mb->codedBlockPattern == 0) {
    copy_Image_16x16 (&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);
    if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444)) {
      copy_Image (&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      copy_Image (&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      }
    }
  else {
    iTransform (mb, plane, 0);
    slice->isResetCoef = FALSE;
    }

  return 1;
  }
//}}}
//{{{
int mb_pred_b_d4x4spatial (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture)
{
  char l0_rFrame = -1, l1_rFrame = -1;
  sMotionVec pmvl0 = kZeroMv, pmvl1 = kZeroMv;
  int k;
  int block8x8;
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  sPicMotionParam *mvInfo;
  int listOffset = mb->listOffset;
  sPicture** list0 = slice->listX[LIST_0 + listOffset];
  sPicture** list1 = slice->listX[LIST_1 + listOffset];

  int predDir = 0;

  set_chroma_vector(mb);

  prepare_direct_params(mb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

  for (block8x8 = 0; block8x8 < 4; block8x8++)
  {
    int k_start = (block8x8 << 2);
    int k_end = k_start + BLOCK_MULTIPLE;

    for (k = k_start; k < k_end; k ++)
    {
      int i  =  (decode_block_scan[k] & 3);
      int j  = ((decode_block_scan[k] >> 2) & 3);
      int i4  = mb->blockX + i;
      int j4  = mb->blockY + j;

      mvInfo = &picture->mvInfo[j4][i4];
      //===== DIRECT PREDICTION =====
      if (l0_rFrame == 0 || l1_rFrame == 0)
      {
        int is_not_moving = (get_colocated_info_4x4(mb, list1[0], i4, mb->blockYaff + j) == 0);

        if (l1_rFrame == -1)
        {
          if (is_not_moving)
          {
            mvInfo->refPic[LIST_0] = list0[0];
            mvInfo->refPic[LIST_1] = NULL;
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = 0;
            mvInfo->refIndex[LIST_1] = -1;
          }
          else
          {
            mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
            mvInfo->refPic[LIST_1] = NULL;
            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
            mvInfo->refIndex[LIST_1] = -1;
          }
          predDir = 0;
        }
        else if (l0_rFrame == -1)
        {
          if  (is_not_moving)
          {
            mvInfo->refPic[LIST_0] = NULL;
            mvInfo->refPic[LIST_1] = list1[0];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = -1;
            mvInfo->refIndex[LIST_1] = 0;
          }
          else
          {
            mvInfo->refPic[LIST_0] = NULL;
            mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_0] = -1;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
          }
          predDir = 1;
        }
        else
        {
          if (l0_rFrame == 0 && ((is_not_moving)))
          {
            mvInfo->refPic[LIST_0] = list0[0];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->refIndex[LIST_0] = 0;
          }
          else
          {
            mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
          }

          if  (l1_rFrame == 0 && ((is_not_moving)))
          {
            mvInfo->refPic[LIST_1] = list1[0];
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_1] = 0;
          }
          else
          {
            mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
          }
          predDir = 2;
        }
      }
      else
      {
        mvInfo = &picture->mvInfo[j4][i4];

        if (l0_rFrame < 0 && l1_rFrame < 0)
        {
          predDir = 2;
          mvInfo->refPic[LIST_0] = list0[0];
          mvInfo->refPic[LIST_1] = list1[0];
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_0] = 0;
          mvInfo->refIndex[LIST_1] = 0;
        }
        else if (l1_rFrame == -1)
        {
          predDir = 0;
          mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
          mvInfo->refPic[LIST_1] = NULL;
          mvInfo->mv[LIST_0] = pmvl0;
          mvInfo->mv[LIST_1] = kZeroMv;
          mvInfo->refIndex[LIST_0] = l0_rFrame;
          mvInfo->refIndex[LIST_1] = -1;
        }
        else if (l0_rFrame == -1)
        {
          predDir = 1;
          mvInfo->refPic[LIST_0] = NULL;
          mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
          mvInfo->mv[LIST_0] = kZeroMv;
          mvInfo->mv[LIST_1] = pmvl1;
          mvInfo->refIndex[LIST_0] = -1;
          mvInfo->refIndex[LIST_1] = l1_rFrame;
        }
        else
        {
          predDir = 2;
          mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
          mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
          mvInfo->mv[LIST_0] = pmvl0;
          mvInfo->mv[LIST_1] = pmvl1;
          mvInfo->refIndex[LIST_0] = l0_rFrame;
          mvInfo->refIndex[LIST_1] = l1_rFrame;
        }
      }
    }

    for (k = k_start; k < k_end; k ++)
    {
      int i =  (decode_block_scan[k] & 3);
      int j = ((decode_block_scan[k] >> 2) & 3);

      perform_mc(mb, plane, picture, predDir, i, j, BLOCK_SIZE, BLOCK_SIZE);
    }
  }

  if (mb->codedBlockPattern == 0)
  {
    copy_Image_16x16(&pixel[mb->pixY], slice->mbPred[plane], mb->pixX, 0);

    if ((picture->chromaFormatIdc != YUV400) && (picture->chromaFormatIdc != YUV444))
    {
      copy_Image(&picture->imgUV[0][mb->piccY], slice->mbPred[1], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
      copy_Image(&picture->imgUV[1][mb->piccY], slice->mbPred[2], mb->pixcX, 0, decoder->mbSize[1][0], decoder->mbSize[1][1]);
    }
  }
  else
  {
    iTransform(mb, plane, 0);
    slice->isResetCoef = FALSE;
  }

  return 1;
}
//}}}
//{{{
int mb_pred_b_inter8x8 (sMacroBlock* mb, eColorPlane plane, sPicture* picture)
{
  char l0_rFrame = -1, l1_rFrame = -1;
  sMotionVec pmvl0 = kZeroMv, pmvl1 = kZeroMv;
  int blockSizeX, blockSizeY;
  int k;
  int block8x8;   // needed for ABT
  sSlice* slice = mb->slice;
  sDecoder* decoder = mb->decoder;

  int listOffset = mb->listOffset;
  sPicture** list0 = slice->listX[LIST_0 + listOffset];
  sPicture** list1 = slice->listX[LIST_1 + listOffset];

  set_chroma_vector(mb);

  // prepare direct modes
  if (slice->directSpatialMvPredFlag && (!(mb->b8mode[0] && mb->b8mode[1] && mb->b8mode[2] && mb->b8mode[3])))
    prepare_direct_params(mb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

  for (block8x8=0; block8x8<4; block8x8++) {
    int mv_mode  = mb->b8mode[block8x8];
    int predDir = mb->b8pdir[block8x8];

    if ( mv_mode != 0 ) {
      int k_start = (block8x8 << 2);
      int k_inc = (mv_mode == SMB8x4) ? 2 : 1;
      int k_end = (mv_mode == SMB8x8) ? k_start + 1 : ((mv_mode == SMB4x4) ? k_start + 4 : k_start + k_inc + 1);

      blockSizeX = ( mv_mode == SMB8x4 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;
      blockSizeY = ( mv_mode == SMB4x8 || mv_mode == SMB8x8 ) ? SMB_BLOCK_SIZE : BLOCK_SIZE;

      for (k = k_start; k < k_end; k += k_inc) {
        int i =  (decode_block_scan[k] & 3);
        int j = ((decode_block_scan[k] >> 2) & 3);
        perform_mc(mb, plane, picture, predDir, i, j, blockSizeX, blockSizeY);
      }
    }
    else {
      int k_start = (block8x8 << 2);
      int k_end = k_start;

      if (decoder->activeSPS->direct_8x8_inference_flag) {
        blockSizeX = SMB_BLOCK_SIZE;
        blockSizeY = SMB_BLOCK_SIZE;
        k_end ++;
      }
      else {
        blockSizeX = BLOCK_SIZE;
        blockSizeY = BLOCK_SIZE;
        k_end += BLOCK_MULTIPLE;
      }

      // Prepare mvs (needed for deblocking and mv prediction
      if (slice->directSpatialMvPredFlag) {
        for (k = k_start; k < k_start + BLOCK_MULTIPLE; k ++) {
          int i  =  (decode_block_scan[k] & 3);
          int j  = ((decode_block_scan[k] >> 2) & 3);
          int i4  = mb->blockX + i;
          int j4  = mb->blockY + j;
          sPicMotionParam *mvInfo = &picture->mvInfo[j4][i4];

          assert (predDir<=2);

          //===== DIRECT PREDICTION =====
          // motion information should be already set
          if (mvInfo->refIndex[LIST_1] == -1) {
            predDir = 0;
          }
          else if (mvInfo->refIndex[LIST_0] == -1) {
            predDir = 1;
          }
          else
          {
            predDir = 2;
          }
        }
      }
      else {
        for (k = k_start; k < k_start + BLOCK_MULTIPLE; k ++) {

          int i =  (decode_block_scan[k] & 3);
          int j = ((decode_block_scan[k] >> 2) & 3);
          int i4   = mb->blockX + i;
          int j4   = mb->blockY + j;
          sPicMotionParam *mvInfo = &picture->mvInfo[j4][i4];

          assert (predDir<=2);

          // store reference picture ID determined by direct mode
          mvInfo->refPic[LIST_0] = list0[(short)mvInfo->refIndex[LIST_0]];
          mvInfo->refPic[LIST_1] = list1[(short)mvInfo->refIndex[LIST_1]];
        }
      }

      for (k = k_start; k < k_end; k ++) {
        int i =  (decode_block_scan[k] & 3);
        int j = ((decode_block_scan[k] >> 2) & 3);
        perform_mc(mb, plane, picture, predDir, i, j, blockSizeX, blockSizeY);
      }
    }
  }

  iTransform(mb, plane, 0);
  if (mb->codedBlockPattern != 0)
    slice->isResetCoef = FALSE;
  return 1;
}
//}}}

//{{{
int mb_pred_ipcm (sMacroBlock* mb) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture;

  // Copy coefficients to decoded picture buffer
  // IPCM coefficients are stored in slice->cof which is set in function readIPCMcoeffs()
  for (int i = 0; i < MB_BLOCK_SIZE; ++i)
    for (int j = 0;j < MB_BLOCK_SIZE ; ++j)
      picture->imgY[mb->pixY + i][mb->pixX + j] = (sPixel) slice->cof[0][i][j];

  if ((picture->chromaFormatIdc != YUV400) && (decoder->coding.isSeperateColourPlane == 0))
    for (int k = 0; k < 2; ++k)
      for(int i = 0; i < decoder->mbCrSizeY; ++i)
        for (int j = 0; j < decoder->mbCrSizeX; ++j)
          picture->imgUV[k][mb->piccY+i][mb->pixcX + j] = (sPixel) slice->cof[k + 1][i][j];

  // for deblocking filter
  updateQp (mb, 0);

  // for eCavlc: Set the nzCoeff to 16.
  // These parameters are to be used in eCavlc decoding of neighbour blocks
  memset (decoder->nzCoeff[mb->mbIndexX][0][0], 16, 3 * BLOCK_PIXELS * sizeof(byte));

  // for eCabac decoding of MB skip flag
  mb->skipFlag = 0;

  //for deblocking filter eCabac
  mb->codedBlockPatterns[0].blk = 0xFFFF;

  //For eCabac decoding of Dquant
  slice->lastDquant = 0;
  slice->isResetCoef = FALSE;
  slice->isResetCoefCr = FALSE;
  return 1;
  }
//}}}
