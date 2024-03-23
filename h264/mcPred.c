//{{{  includes
#include "global.h"
#include "memory.h"

#include "block.h"
#include "mcPred.h"
#include "buffer.h"
#include "mbAccess.h"
#include "macroblock.h"
//}}}
static const sMotionVec kZeroMv = {0, 0};

//{{{
int allocPred (sSlice* slice) {

  int alloc_size = 0;

  alloc_size += getMem2Dpel(&slice->tempBlockL0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  alloc_size += getMem2Dpel(&slice->tempBlockL1, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  alloc_size += getMem2Dpel(&slice->tempBlockL2, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  alloc_size += getMem2Dpel(&slice->tempBlockL3, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  alloc_size += getMem2Dint(&slice->tempRes, MB_BLOCK_SIZE + 5, MB_BLOCK_SIZE + 5);

  return (alloc_size);
  }
//}}}
//{{{
void freePred (sSlice* slice) {

  freeMem2Dint (slice->tempRes);
  freeMem2Dpel (slice->tempBlockL0);
  freeMem2Dpel (slice->tempBlockL1);
  freeMem2Dpel (slice->tempBlockL2);
  freeMem2Dpel (slice->tempBlockL3);
  }
//}}}

//{{{
static void update_direct_mv_info_temporal (sMacroBlock* mb) {

  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  int j,k;

  int partmode = ((mb->mbType == P8x8) ? 4 : mb->mbType);
  int step_h0 = BLOCK_STEP [partmode][0];
  int step_v0 = BLOCK_STEP [partmode][1];
  int i0, j0, j6;
  int j4, i4;

  sPicture* picture = slice->picture;
  int listOffset = mb->listOffset; // ((slice->mbAffFrame)&&(mb->mbField))? (mb_nr&0x01) ? 4 : 2 : 0;
  sPicture** list0 = slice->listX[LIST_0 + listOffset];
  sPicture** list1 = slice->listX[LIST_1 + listOffset];

  Boolean has_direct = (mb->b8mode[0] == 0) | (mb->b8mode[1] == 0) |
                       (mb->b8mode[2] == 0) | (mb->b8mode[3] == 0);
  if (has_direct) {
    int mv_scale = 0;
    for (k = 0; k < 4; ++k) {
      // Scan all blocks
      if (mb->b8mode[k] == 0) {
        mb->b8pdir[k] = 2;
        for (j0 = 2 * (k >> 1); j0 < 2 * (k >> 1) + 2; j0 += step_v0) {
          for (i0 = mb->blockX + 2*(k & 0x01); i0 < mb->blockX + 2 * (k & 0x01)+2; i0 += step_h0) {
            //{{{  block
            int refList;
            int refIndex;
            int mapped_idx = -1, iref;

            sPicMotionParam* colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                                           &list1[0]->mvInfo[RSD(mb->blockYaff + j0)][RSD(i0)] :
                                           &list1[0]->mvInfo[mb->blockYaff + j0][i0];

            if (slice->mbAffFrame) {
              //{{{
              if (!mb->mbField && ((slice->listX[LIST_1][0]->iCodingType==eFrameMbPairCoding && slice->listX[LIST_1][0]->motion.mbField[mb->mbIndexX]) ||
                (slice->listX[LIST_1][0]->iCodingType==eFieldCoding))) {
                if (iabs(picture->poc - slice->listX[LIST_1+4][0]->poc)> iabs(picture->poc -slice->listX[LIST_1+2][0]->poc) )
                  colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                    &slice->listX[LIST_1+2][0]->mvInfo[RSD(mb->blockYaff + j0)>>1][RSD(i0)] :
                    &slice->listX[LIST_1+2][0]->mvInfo[(mb->blockYaff + j0)>>1][i0];
                else
                  colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                    &slice->listX[LIST_1+4][0]->mvInfo[RSD(mb->blockYaff + j0)>>1][RSD(i0)] :
                    &slice->listX[LIST_1+4][0]->mvInfo[(mb->blockYaff + j0)>>1][i0];
                }
              }
              //}}}
            else if (!decoder->activeSPS->frameMbOnly &&
                     !slice->fieldPic &&
                     slice->listX[LIST_1][0]->iCodingType != eFrameCoding) {
              //{{{
              if (iabs(picture->poc - list1[0]->botField->poc) > iabs(picture->poc -list1[0]->topField->poc) )
                colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                  &list1[0]->topField->mvInfo[RSD(mb->blockYaff + j0)>>1][RSD(i0)] :
                  &list1[0]->topField->mvInfo[(mb->blockYaff + j0)>>1][i0];
              else
                colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                  &list1[0]->botField->mvInfo[RSD(mb->blockYaff + j0)>>1][RSD(i0)] :
                  &list1[0]->botField->mvInfo[(mb->blockYaff + j0)>>1][i0];
              }
              //}}}
            else if (!decoder->activeSPS->frameMbOnly &&
                     slice->fieldPic &&
                     slice->picStructure != list1[0]->picStructure &&
                     list1[0]->codedFrame) {
              //{{{
              if (slice->picStructure == eTopField)
                colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                  &list1[0]->frame->topField->mvInfo[RSD(mb->blockYaff + j0)][RSD(i0)] :
                  &list1[0]->frame->topField->mvInfo[mb->blockYaff + j0][i0];
              else
                colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                  &list1[0]->frame->botField->mvInfo[RSD(mb->blockYaff + j0)][RSD(i0)] :
                  &list1[0]->frame->botField->mvInfo[mb->blockYaff + j0][i0];
              }
              //}}}

            refList = colocated->refIndex[LIST_0 ]== -1 ? LIST_1 : LIST_0;
            refIndex = colocated->refIndex[refList];
            if (refIndex == -1) {
              //{{{
              for (j4 = mb->blockY + j0; j4 < mb->blockY + j0 + step_v0; ++j4) {
                for (i4 = i0; i4 < i0 + step_h0; ++i4) {
                  sPicMotionParam *mvInfo = &picture->mvInfo[j4][i4];
                  mvInfo->refPic[LIST_0] = list0[0];
                  mvInfo->refPic[LIST_1] = list1[0];
                  mvInfo->mv [LIST_0] = kZeroMv;
                  mvInfo->mv [LIST_1] = kZeroMv;
                  mvInfo->refIndex [LIST_0] = 0;
                  mvInfo->refIndex [LIST_1] = 0;
                  }
                }
              }
              //}}}
            else {
              if ((slice->mbAffFrame && ( (mb->mbField && colocated->refPic[refList]->picStructure==eFrame) ||
                  (!mb->mbField && colocated->refPic[refList]->picStructure!=eFrame))) ||
                  (!slice->mbAffFrame && ((slice->fieldPic==0 && colocated->refPic[refList]->picStructure!=eFrame)||
                  (slice->fieldPic==1 && colocated->refPic[refList]->picStructure==eFrame))) ) {
                //{{{  Frame with field co-located
                for (iref = 0;
                     iref < imin(slice->numRefIndexActive[LIST_0], slice->listXsize[LIST_0 + listOffset]);
                     ++iref) {
                  if (slice->listX[LIST_0 + listOffset][iref]->topField == colocated->refPic[refList] ||
                    slice->listX[LIST_0 + listOffset][iref]->botField == colocated->refPic[refList] ||
                    slice->listX[LIST_0 + listOffset][iref]->frame == colocated->refPic[refList] )
                  {
                    if ((slice->fieldPic==1) && (slice->listX[LIST_0 + listOffset][iref]->picStructure != slice->picStructure))
                      mapped_idx=INVALIDINDEX;
                    else {
                      mapped_idx = iref;
                      break;
                      }
                    }
                  else //! invalid index. Default to zero even though this case should not happen
                    mapped_idx=INVALIDINDEX;
                  }
                }
                //}}}
              else {
                //{{{
                for (iref = 0;
                     iref < imin(slice->numRefIndexActive[LIST_0], slice->listXsize[LIST_0 + listOffset]);
                     ++iref) {
                  if (slice->listX[LIST_0 + listOffset][iref] == colocated->refPic[refList]) {
                    mapped_idx = iref;
                    break;
                    }
                  else //! invalid index. Default to zero even though this case should not happen
                    mapped_idx=INVALIDINDEX;
                  }
                }
                //}}}

              if (mapped_idx != INVALIDINDEX) {
                //{{{
                for (j = j0; j < j0 + step_v0; ++j) {
                  j4 = mb->blockY + j;
                  j6 = mb->blockYaff + j;

                  for (i4 = i0; i4 < i0 + step_h0; ++i4) {
                    sPicMotionParam* colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                      &list1[0]->mvInfo[RSD(j6)][RSD(i4)] :
                      &list1[0]->mvInfo[j6][i4];
                    sPicMotionParam* mvInfo = &picture->mvInfo[j4][i4];
                    int mvY;
                    if (slice->mbAffFrame) {
                      if (!mb->mbField && ((slice->listX[LIST_1][0]->iCodingType==eFrameMbPairCoding && slice->listX[LIST_1][0]->motion.mbField[mb->mbIndexX]) ||
                          (slice->listX[LIST_1][0]->iCodingType==eFieldCoding))) {
                        if (iabs(picture->poc - slice->listX[LIST_1+4][0]->poc)> iabs(picture->poc -slice->listX[LIST_1+2][0]->poc) )
                          colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                            &slice->listX[LIST_1+2][0]->mvInfo[RSD(j6)>>1][RSD(i4)] :
                            &slice->listX[LIST_1+2][0]->mvInfo[j6>>1][i4];
                        else
                          colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                            &slice->listX[LIST_1+4][0]->mvInfo[RSD(j6)>>1][RSD(i4)] :
                            &slice->listX[LIST_1+4][0]->mvInfo[j6>>1][i4];
                        }
                      }
                    else if (!decoder->activeSPS->frameMbOnly &&
                             !slice->fieldPic &&
                             slice->listX[LIST_1][0]->iCodingType!=eFrameCoding) {
                      if (iabs(picture->poc - list1[0]->botField->poc) > iabs(picture->poc -list1[0]->topField->poc) )
                        colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                          &list1[0]->topField->mvInfo[RSD(j6)>>1][RSD(i4)] :
                          &list1[0]->topField->mvInfo[(j6)>>1][i4];
                      else
                        colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                          &list1[0]->botField->mvInfo[RSD(j6)>>1][RSD(i4)] :
                          &list1[0]->botField->mvInfo[(j6)>>1][i4];
                      }
                    else if (!decoder->activeSPS->frameMbOnly &&
                             slice->fieldPic &&
                             slice->picStructure!=list1[0]->picStructure && list1[0]->codedFrame) {
                      if (slice->picStructure == eTopField)
                        colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                          &list1[0]->frame->topField->mvInfo[RSD(j6)][RSD(i4)] :
                          &list1[0]->frame->topField->mvInfo[j6][i4];
                      else
                        colocated = decoder->activeSPS->direct_8x8_inference_flag ?
                          &list1[0]->frame->botField->mvInfo[RSD(j6)][RSD(i4)] :
                          &list1[0]->frame->botField->mvInfo[j6][i4];
                      }

                    mvY = colocated->mv[refList].mvY;
                    if ((slice->mbAffFrame && !mb->mbField && colocated->refPic[refList]->picStructure!=eFrame) ||
                        (!slice->mbAffFrame && slice->fieldPic==0 && colocated->refPic[refList]->picStructure!=eFrame))
                      mvY *= 2;
                    else if ((slice->mbAffFrame && mb->mbField && colocated->refPic[refList]->picStructure==eFrame) ||
                             (!slice->mbAffFrame && slice->fieldPic==1 && colocated->refPic[refList]->picStructure==eFrame))
                      mvY /= 2;

                    mv_scale = slice->mvscale[LIST_0 + listOffset][mapped_idx];
                    mvInfo->refIndex [LIST_0] = (char) mapped_idx;
                    mvInfo->refIndex [LIST_1] = 0;
                    mvInfo->refPic[LIST_0] = list0[mapped_idx];
                    mvInfo->refPic[LIST_1] = list1[0];
                    if (mv_scale == 9999 || slice->listX[LIST_0+listOffset][mapped_idx]->isLongTerm) {
                      mvInfo->mv[LIST_0].mvX = colocated->mv[refList].mvX;
                      mvInfo->mv[LIST_0].mvY = (short) mvY;
                      mvInfo->mv[LIST_1] = kZeroMv;
                      }
                    else {
                      mvInfo->mv[LIST_0].mvX = (short)((mv_scale * colocated->mv[refList].mvX + 128 ) >> 8);
                      mvInfo->mv[LIST_0].mvY = (short)((mv_scale * mvY/*colocated->mv[refList].mvY*/ + 128 ) >> 8);
                      mvInfo->mv[LIST_1].mvX = (short)(mvInfo->mv[LIST_0].mvX - colocated->mv[refList].mvX);
                      mvInfo->mv[LIST_1].mvY = (short)(mvInfo->mv[LIST_0].mvY - mvY/*colocated->mv[refList].mvY*/);
                      }
                    }
                 }
                }
                //}}}
              else if (INVALIDINDEX == mapped_idx)
                error ("temporal direct error: colocated block has ref that is unavailable");
              }
            }
            //}}}
          }
        }
      }
    }
  }
//}}}
//{{{
static inline void update_neighbor_mvs (sPicMotionParam** motion, const sPicMotionParam* mvInfo, int i4) {

  (*motion++)[i4 + 1] = *mvInfo;
  (*motion  )[i4    ] = *mvInfo;
  (*motion  )[i4 + 1] = *mvInfo;
  }
//}}}
//{{{
int get_colocated_info_4x4 (sMacroBlock* mb, sPicture* list1, int i, int j) {

  if (list1->isLongTerm)
    return 1;
  else {
    sPicMotionParam *fs = &list1->mvInfo[j][i];
    int moving = !((((fs->refIndex[LIST_0] == 0) &&
                     (iabs(fs->mv[LIST_0].mvX)>>1 == 0) &&
                     (iabs(fs->mv[LIST_0].mvY)>>1 == 0))) ||
                   ((fs->refIndex[LIST_0] == -1) &&
                    (fs->refIndex[LIST_1] == 0) &&
                    (iabs(fs->mv[LIST_1].mvX)>>1 == 0) &&
                    (iabs(fs->mv[LIST_1].mvY)>>1 == 0)));
    return moving;
    }
  }
//}}}
//{{{
int get_colocated_info_8x8 (sMacroBlock* mb, sPicture* list1, int i, int j) {

  if (list1->isLongTerm)
    return 1;
  else {
    sSlice* slice = mb->slice;
    sDecoder* decoder = mb->decoder;
    if ((slice->mbAffFrame) ||
        (!decoder->activeSPS->frameMbOnly &&
        ((!slice->picStructure && list1->iCodingType == eFieldCoding) ||
        (slice->picStructure!=list1->picStructure && list1->codedFrame)))) {
      int jj = RSD(j);
      int ii = RSD(i);
      int jdiv = (jj>>1);
      int moving;

      sPicMotionParam* fs = &list1->mvInfo[jj][ii];

      if (slice->fieldPic && slice->picStructure!=list1->picStructure && list1->codedFrame) {
         if (slice->picStructure == eTopField)
           fs = list1->topField->mvInfo[jj] + ii;
         else
           fs = list1->botField->mvInfo[jj] + ii;
        }
      else {
        if( (slice->mbAffFrame && ((!mb->mbField && list1->motion.mbField[mb->mbIndexX]) ||
          (!mb->mbField && list1->iCodingType == eFieldCoding)))
          || (!slice->mbAffFrame)) {
          if (iabs(slice->picture->poc - list1->botField->poc)> iabs(slice->picture->poc -list1->topField->poc) )
            fs = list1->topField->mvInfo[jdiv] + ii;
          else
            fs = list1->botField->mvInfo[jdiv] + ii;
          }
        }

      moving = !((((fs->refIndex[LIST_0] == 0)
        &&  (iabs(fs->mv[LIST_0].mvX)>>1 == 0)
        &&  (iabs(fs->mv[LIST_0].mvY)>>1 == 0)))
        || ((fs->refIndex[LIST_0] == -1)
        &&  (fs->refIndex[LIST_1] == 0)
        &&  (iabs(fs->mv[LIST_1].mvX)>>1 == 0)
        &&  (iabs(fs->mv[LIST_1].mvY)>>1 == 0)));
      return moving;
      }
    else {
      sPicMotionParam *fs = &list1->mvInfo[RSD(j)][RSD(i)];

      int moving;
      if (mb->decoder->coding.sepColourPlaneFlag && mb->decoder->coding.yuvFormat==YUV444)
        fs = &list1->JVmv_info[mb->slice->colourPlaneId][RSD(j)][RSD(i)];

      moving = !((((fs->refIndex[LIST_0] == 0)
        &&  (iabs(fs->mv[LIST_0].mvX)>>1 == 0)
        &&  (iabs(fs->mv[LIST_0].mvY)>>1 == 0)))
        || ((fs->refIndex[LIST_0] == -1)
        &&  (fs->refIndex[LIST_1] == 0)
        &&  (iabs(fs->mv[LIST_1].mvX)>>1 == 0)
        &&  (iabs(fs->mv[LIST_1].mvY)>>1 == 0)));

      return moving;
      }
    }
  }
//}}}

//{{{
static void update_direct_mv_info_spatial_8x8 (sMacroBlock* mb)
{
  Boolean has_direct = (mb->b8mode[0] == 0) | (mb->b8mode[1] == 0) | (mb->b8mode[2] == 0) | (mb->b8mode[3] == 0);

  if (has_direct)
  {
    //sDecoder* decoder = mb->decoder;
    sSlice* slice = mb->slice;
    int i,j,k;

    int j4, i4;
    sPicture* picture = slice->picture;

    int listOffset = mb->listOffset; // ((slice->mbAffFrame)&&(mb->mbField))? (mb_nr&0x01) ? 4 : 2 : 0;
    sPicture** list0 = slice->listX[LIST_0 + listOffset];
    sPicture** list1 = slice->listX[LIST_1 + listOffset];

    char  l0_rFrame, l1_rFrame;
    sMotionVec pmvl0, pmvl1;
    int is_not_moving;
    sPicMotionParam *mvInfo = NULL;

    prepare_direct_params(mb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);

    for (k = 0; k < 4; ++k)
    {
      if (mb->b8mode[k] == 0)
      {
        i = 2 * (k & 0x01);
        j = 2 * (k >> 1);

        //j6 = mb->blockYaff + j;
        j4 = mb->blockY     + j;
        i4 = mb->blockX     + i;

        mvInfo = &picture->mvInfo[j4][i4];

        is_not_moving = (get_colocated_info_8x8(mb, list1[0], i4, mb->blockYaff + j) == 0);

        if (is_not_moving && (l0_rFrame == 0 || l1_rFrame == 0))
        {
          if (l1_rFrame == -1)
          {
            if  (l0_rFrame == 0)
            {
              mvInfo->refPic[LIST_0] = list0[0];
              mvInfo->refPic[LIST_1] = list1[0];
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
          }
          else if (l0_rFrame == -1)
          {
            if  (l1_rFrame == 0)
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
          }
          else
          {
            if  (l0_rFrame == 0)
            {
              mvInfo->refPic[LIST_0] = list0[0];
              mvInfo->mv[LIST_0] = kZeroMv;
              mvInfo->refIndex[LIST_0] = 0;
            }
            else
            {
              mvInfo->refPic[LIST_1] = list1[(short) l0_rFrame];
              mvInfo->mv[LIST_0] = pmvl0;
              mvInfo->refIndex[LIST_0] = l0_rFrame;
            }

            if  (l1_rFrame == 0)
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
          }
        }
        else
        {
          if (l0_rFrame < 0 && l1_rFrame < 0)
          {
            mvInfo->refPic[LIST_0] = list0[0];
            mvInfo->refPic[LIST_1] = list1[0];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = 0;
            mvInfo->refIndex[LIST_1] = 0;
          }
          else if (l0_rFrame < 0)
          {
            mvInfo->refPic[LIST_0] = NULL;
            mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
            mvInfo->mv[LIST_0] = kZeroMv;
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_0] = -1;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
          }
          else  if (l1_rFrame < 0)
          {
            mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
            mvInfo->refPic[LIST_1] = NULL;

            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->mv[LIST_1] = kZeroMv;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
            mvInfo->refIndex[LIST_1] = -1;
          }
          else
          {
            mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
            mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
            mvInfo->mv[LIST_0] = pmvl0;
            mvInfo->mv[LIST_1] = pmvl1;
            mvInfo->refIndex[LIST_0] = l0_rFrame;
            mvInfo->refIndex[LIST_1] = l1_rFrame;
          }
        }
        update_neighbor_mvs(&picture->mvInfo[j4], mvInfo, i4);
      }
    }
  }
}
//}}}
//{{{
static void update_direct_mv_info_spatial_4x4 (sMacroBlock* mb)
{
  Boolean has_direct = (mb->b8mode[0] == 0) | (mb->b8mode[1] == 0) | (mb->b8mode[2] == 0) | (mb->b8mode[3] == 0);

  if (has_direct)
  {
    sDecoder* decoder = mb->decoder;
    sSlice* slice = mb->slice;
    int i,j,k;

    int j4, i4;
    sPicture* picture = decoder->picture;

    int listOffset = mb->listOffset; // ((slice->mbAffFrame)&&(mb->mbField))? (mb_nr&0x01) ? 4 : 2 : 0;
    sPicture** list0 = slice->listX[LIST_0 + listOffset];
    sPicture** list1 = slice->listX[LIST_1 + listOffset];

    char  l0_rFrame, l1_rFrame;
    sMotionVec pmvl0, pmvl1;

    prepare_direct_params(mb, picture, &pmvl0, &pmvl1, &l0_rFrame, &l1_rFrame);
    for (k = 0; k < 4; ++k)
    {
      if (mb->b8mode[k] == 0)
      {

        i = 2 * (k & 0x01);
        for(j = 2 * (k >> 1); j < 2 * (k >> 1)+2;++j)
        {
          j4 = mb->blockY     + j;

          for(i4 = mb->blockX + i; i4 < mb->blockX + i + 2; ++i4)
          {
            sPicMotionParam *mvInfo = &picture->mvInfo[j4][i4];
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
                  mvInfo->refIndex[LIST_1]    = 0;
                }
                else
                {
                  mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
                  mvInfo->mv[LIST_1] = pmvl1;
                  mvInfo->refIndex[LIST_1] = l1_rFrame;
                }
              }
            }
            else
            {
              mvInfo = &picture->mvInfo[j4][i4];

              if (l0_rFrame < 0 && l1_rFrame < 0)
              {
                mvInfo->refPic[LIST_0] = list0[0];
                mvInfo->refPic[LIST_1] = list1[0];
                mvInfo->mv[LIST_0] = kZeroMv;
                mvInfo->mv[LIST_1] = kZeroMv;
                mvInfo->refIndex[LIST_0] = 0;
                mvInfo->refIndex[LIST_1] = 0;
              }
              else if (l1_rFrame == -1)
              {
                mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
                mvInfo->refPic[LIST_1] = NULL;
                mvInfo->mv[LIST_0] = pmvl0;
                mvInfo->mv[LIST_1] = kZeroMv;
                mvInfo->refIndex[LIST_0] = l0_rFrame;
                mvInfo->refIndex[LIST_1] = -1;
              }
              else if (l0_rFrame == -1)
              {
                mvInfo->refPic[LIST_0] = NULL;
                mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
                mvInfo->mv[LIST_0] = kZeroMv;
                mvInfo->mv[LIST_1] = pmvl1;
                mvInfo->refIndex[LIST_0] = -1;
                mvInfo->refIndex[LIST_1] = l1_rFrame;
              }
              else
              {
                mvInfo->refPic[LIST_0] = list0[(short) l0_rFrame];
                mvInfo->refPic[LIST_1] = list1[(short) l1_rFrame];
                mvInfo->mv[LIST_0] = pmvl0;
                mvInfo->mv[LIST_1] = pmvl1;
                mvInfo->refIndex[LIST_0] = l0_rFrame;
                mvInfo->refIndex[LIST_1] = l1_rFrame;
              }
            }
          }
        }
      }
    }
  }
}
//}}}
//{{{
void update_direct_types (sSlice* slice)
{
  if (slice->activeSPS->direct_8x8_inference_flag)
    slice->updateDirectMvInfo =
      slice->directSpatialMvPredFlag ? update_direct_mv_info_spatial_8x8 :
                                               update_direct_mv_info_temporal;
  else
    slice->updateDirectMvInfo =
      slice->directSpatialMvPredFlag ? update_direct_mv_info_spatial_4x4 :
                                               update_direct_mv_info_temporal;
}
//}}}

//{{{
static void mc_prediction (sPixel** mbPred, sPixel** block, int blockSizeY, int blockSizeX, int ioff)
{

  int j;

  for (j = 0; j < blockSizeY; j++)
  {
    memcpy(&mbPred[j][ioff], block[j], blockSizeX * sizeof(sPixel));
  }
}
//}}}
//{{{
static void weighted_mc_prediction (sPixel** mbPred, sPixel** block,
                                    int blockSizeY, int blockSizeX,
                                    int ioff, int wp_scale, int wpOffset,
                                    int weight_denom, int color_clip)
{
  int i, j;
  int result;

  for(j = 0; j < blockSizeY; j++)
  {
    for(i = 0; i < blockSizeX; i++)
    {
      result = rshift_rnd((wp_scale * block[j][i]), weight_denom) + wpOffset;
      mbPred[j][i + ioff] = (sPixel)iClip3(0, color_clip, result);
    }
  }
}
//}}}
//{{{
static void bi_prediction (sPixel** mbPred,
                           sPixel** block_l0,
                           sPixel** block_l1,
                           int blockSizeY,
                           int blockSizeX,
                           int ioff)
{
  sPixel *mpr = &mbPred[0][ioff];
  sPixel *b0 = block_l0[0];
  sPixel *b1 = block_l1[0];
  int ii, jj;
  int row_inc = MB_BLOCK_SIZE - blockSizeX;
  for(jj = 0;jj < blockSizeY;jj++)
  {
    // unroll the loop
    for(ii = 0; ii < blockSizeX; ii += 2)
    {
      *(mpr++) = (sPixel)(((*(b0++) + *(b1++)) + 1) >> 1);
      *(mpr++) = (sPixel)(((*(b0++) + *(b1++)) + 1) >> 1);
    }
    mpr += row_inc;
    b0  += row_inc;
    b1  += row_inc;
  }
}
//}}}
//{{{
static void weighted_bi_prediction (sPixel *mbPred,
                                    sPixel *block_l0,
                                    sPixel *block_l1,
                                    int blockSizeY,
                                    int blockSizeX,
                                    int wp_scale_l0,
                                    int wp_scale_l1,
                                    int wpOffset,
                                    int weight_denom,
                                    int color_clip)
{
  int i, j, result;
  int row_inc = MB_BLOCK_SIZE - blockSizeX;

  for(j = 0; j < blockSizeY; j++)
  {
    for(i = 0; i < blockSizeX; i++)
    {
      result = rshift_rnd_sf((wp_scale_l0 * *(block_l0++) + wp_scale_l1 * *(block_l1++)),  weight_denom);

      *(mbPred++) = (sPixel) iClip1(color_clip, result + wpOffset);
    }
    mbPred += row_inc;
    block_l0 += row_inc;
    block_l1 += row_inc;
  }
}
//}}}

//{{{
static void get_block_00 (sPixel *block, sPixel* pixel, int span, int blockSizeY)
{
  // fastest to just move an entire block, since block is a temp block is a 256 byte block (16x16)
  // writes 2 lines of 16 sPixel 1 to 8 times depending in blockSizeY
  int j;

  for (j = 0; j < blockSizeY; j += 2)
  {
    memcpy(block, pixel, MB_BLOCK_SIZE * sizeof(sPixel));
    block += MB_BLOCK_SIZE;
    pixel += span;
    memcpy(block, pixel, MB_BLOCK_SIZE * sizeof(sPixel));
    block += MB_BLOCK_SIZE;
    pixel += span;
  }
}

//}}}
//{{{
static void get_luma_10 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos , int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line, *cur_line;
  int i, j;
  int result;

  for (j = 0; j < blockSizeY; j++)
  {
    cur_line = &(curPixelY[j][x_pos]);
    p0 = &curPixelY[j][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
      *orig_line = (sPixel) ((*orig_line + *(cur_line++) + 1 ) >> 1);
      orig_line++;
    }
  }
}
//}}}
//{{{
static void get_luma_20 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos , int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int i, j;
  int result;
  for (j = 0; j < blockSizeY; j++)
  {
    p0 = &curPixelY[j][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line++ = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }
}
//}}}
//{{{
static void get_luma_30 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos , int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line, *cur_line;
  int i, j;
  int result;

  for (j = 0; j < blockSizeY; j++)
  {
    cur_line = &(curPixelY[j][x_pos + 1]);
    p0 = &curPixelY[j][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
      *orig_line = (sPixel) ((*orig_line + *(cur_line++) + 1 ) >> 1);
      orig_line++;
    }
  }
}
//}}}
//{{{
static void get_luma_01 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos, int shift_x, int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line, *cur_line;
  int i, j;
  int result;
  int jj = 0;
  p0 = &(curPixelY[ - 2][x_pos]);
  for (j = 0; j < blockSizeY; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];
    cur_line = &(curPixelY[jj++][x_pos]);

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
      *orig_line = (sPixel) ((*orig_line + *(cur_line++) + 1 ) >> 1);
      orig_line++;
    }
    p0 = p1 - blockSizeX;
  }
}

//}}}
//{{{
static void get_luma_02 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos, int shift_x, int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int i, j;
  int result;
  p0 = &(curPixelY[ - 2][x_pos]);
  for (j = 0; j < blockSizeY; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line++ = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
    p0 = p1 - blockSizeX;
  }
}
//}}}
//{{{
static void get_luma_03 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos, int shift_x, int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line, *cur_line;
  int i, j;
  int result;
  int jj = 1;

  p0 = &(curPixelY[ -2][x_pos]);
  for (j = 0; j < blockSizeY; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];
    cur_line = &(curPixelY[jj++][x_pos]);

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
      *orig_line = (sPixel) ((*orig_line + *(cur_line++) + 1 ) >> 1);
      orig_line++;
    }
    p0 = p1 - blockSizeX;
  }
}
//}}}
//{{{
static void get_luma_21 (sPixel** block, sPixel** curPixelY, int** tempRes, int blockSizeY, int blockSizeX, int x_pos, int max_imgpel_value)
{
  int i, j;
  /* Vertical & horizontal interpolation */
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  int jj = -2;

  for (j = 0; j < blockSizeY + 5; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    tmp_line  = tempRes[j];

    for (i = 0; i < blockSizeX; i++)
    {
      *(tmp_line++) = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
  }

  jj = 2;
  for (j = 0; j < blockSizeY; j++)
  {
    tmp_line  = tempRes[jj++];
    x0 = tempRes[j    ];
    x1 = tempRes[j + 1];
    x2 = tempRes[j + 2];
    x3 = tempRes[j + 3];
    x4 = tempRes[j + 4];
    x5 = tempRes[j + 5];
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*x0++ + *x5++) - 5 * (*x1++ + *x4++) + 20 * (*x2++ + *x3++);

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((*(tmp_line++) + 16) >> 5)) + 1 )>> 1);
      orig_line++;
    }
  }
}
//}}}
//{{{
static void get_luma_22 (sPixel** block, sPixel** curPixelY, int** tempRes, int blockSizeY, int blockSizeX, int x_pos, int max_imgpel_value)
{
  int i, j;
  /* Vertical & horizontal interpolation */
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  int jj = - 2;

  for (j = 0; j < blockSizeY + 5; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    tmp_line  = tempRes[j];

    for (i = 0; i < blockSizeX; i++)
    {
      *(tmp_line++) = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
  }

  for (j = 0; j < blockSizeY; j++)
  {
    x0 = tempRes[j    ];
    x1 = tempRes[j + 1];
    x2 = tempRes[j + 2];
    x3 = tempRes[j + 3];
    x4 = tempRes[j + 4];
    x5 = tempRes[j + 5];
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*x0++ + *x5++) - 5 * (*x1++ + *x4++) + 20 * (*x2++ + *x3++);

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
    }
  }
}
//}}}
//{{{
static void get_luma_23 (sPixel** block, sPixel** curPixelY, int** tempRes, int blockSizeY, int blockSizeX, int x_pos, int max_imgpel_value)
{
  int i, j;
  /* Vertical & horizontal interpolation */
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  int jj = -2;

  for (j = 0; j < blockSizeY + 5; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    tmp_line  = tempRes[j];

    for (i = 0; i < blockSizeX; i++)
    {
      *(tmp_line++) = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
  }

  jj = 3;
  for (j = 0; j < blockSizeY; j++)
  {
    tmp_line  = tempRes[jj++];
    x0 = tempRes[j    ];
    x1 = tempRes[j + 1];
    x2 = tempRes[j + 2];
    x3 = tempRes[j + 3];
    x4 = tempRes[j + 4];
    x5 = tempRes[j + 5];
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*x0++ + *x5++) - 5 * (*x1++ + *x4++) + 20 * (*x2++ + *x3++);

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((*(tmp_line++) + 16) >> 5)) + 1 )>> 1);
      orig_line++;
    }
  }
}
//}}}
//{{{
static void get_luma_12 (sPixel** block, sPixel** curPixelY, int** tempRes, int blockSizeY, int blockSizeX, int x_pos, int shift_x, int max_imgpel_value)
{
  int i, j;
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  p0 = &(curPixelY[ -2][x_pos - 2]);
  for (j = 0; j < blockSizeY; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    tmp_line  = tempRes[j];

    for (i = 0; i < blockSizeX + 5; i++)
    {
      *(tmp_line++)  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
    p0 = p1 - (blockSizeX + 5);
  }

  for (j = 0; j < blockSizeY; j++)
  {
    tmp_line  = &tempRes[j][2];
    orig_line = block[j];
    x0 = tempRes[j];
    x1 = x0 + 1;
    x2 = x1 + 1;
    x3 = x2 + 1;
    x4 = x3 + 1;
    x5 = x4 + 1;

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(x0++) + *(x5++)) - 5 * (*(x1++) + *(x4++)) + 20 * (*(x2++) + *(x3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((*(tmp_line++) + 16)>>5))+1)>>1);
      orig_line ++;
    }
  }
}
//}}}
//{{{
static void get_luma_32 (sPixel** block, sPixel** curPixelY, int** tempRes, int blockSizeY, int blockSizeX, int x_pos, int shift_x, int max_imgpel_value)
{
  int i, j;
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  p0 = &(curPixelY[ -2][x_pos - 2]);
  for (j = 0; j < blockSizeY; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    tmp_line  = tempRes[j];

    for (i = 0; i < blockSizeX + 5; i++)
    {
      *(tmp_line++)  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
    p0 = p1 - (blockSizeX + 5);
  }

  for (j = 0; j < blockSizeY; j++)
  {
    tmp_line  = &tempRes[j][3];
    orig_line = block[j];
    x0 = tempRes[j];
    x1 = x0 + 1;
    x2 = x1 + 1;
    x3 = x2 + 1;
    x4 = x3 + 1;
    x5 = x4 + 1;

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(x0++) + *(x5++)) - 5 * (*(x1++) + *(x4++)) + 20 * (*(x2++) + *(x3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((*(tmp_line++) + 16)>>5))+1)>>1);
      orig_line ++;
    }
  }
}
//}}}
//{{{
static void get_luma_33 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos, int shift_x, int max_imgpel_value)
{
  int i, j;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int result;

  int jj = 1;

  for (j = 0; j < blockSizeY; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;

    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }

  p0 = &(curPixelY[-2][x_pos + 1]);
  for (j = 0; j < blockSizeY; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((result + 16) >> 5)) + 1) >> 1);
      orig_line++;
    }
    p0 = p1 - blockSizeX ;
  }
}
//}}}
//{{{
static void get_luma_11 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos, int shift_x, int max_imgpel_value)
{
  int i, j;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int result;

  int jj = 0;

  for (j = 0; j < blockSizeY; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;

    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }

  p0 = &(curPixelY[-2][x_pos]);
  for (j = 0; j < blockSizeY; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((result + 16) >> 5)) + 1) >> 1);
      orig_line++;
    }
    p0 = p1 - blockSizeX ;
  }
}
//}}}
//{{{
static void get_luma_13 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos, int shift_x, int max_imgpel_value)
{
  /* Diagonal interpolation */
  int i, j;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int result;

  int jj = 1;

  for (j = 0; j < blockSizeY; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;

    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }

  p0 = &(curPixelY[-2][x_pos]);
  for (j = 0; j < blockSizeY; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((result + 16) >> 5)) + 1) >> 1);
      orig_line++;
    }
    p0 = p1 - blockSizeX ;
  }
}
//}}}
//{{{
static void get_luma_31 (sPixel** block, sPixel** curPixelY, int blockSizeY, int blockSizeX, int x_pos, int shift_x, int max_imgpel_value)
{
  /* Diagonal interpolation */
  int i, j;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int result;

  int jj = 0;

  for (j = 0; j < blockSizeY; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;

    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }

  p0 = &(curPixelY[-2][x_pos + 1]);
  for (j = 0; j < blockSizeY; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < blockSizeX; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((result + 16) >> 5)) + 1) >> 1);
      orig_line++;
    }
    p0 = p1 - blockSizeX ;
  }
}
//}}}
//{{{
void get_block_luma (sPicture* curRef, int x_pos, int y_pos, int blockSizeX, int blockSizeY, sPixel** block,
                    int shift_x, int maxold_x, int maxold_y, int** tempRes, int max_imgpel_value, sPixel no_ref_value, sMacroBlock* mb)
{
  if (curRef->noRef) {
    //printf("list[ref_frame] is equal to 'no reference picture' before RAP\n");
    memset(block[0],no_ref_value,blockSizeY * blockSizeX * sizeof(sPixel));
  }
  else
  {
    sPixel** curPixelY = (mb->decoder->coding.sepColourPlaneFlag && mb->slice->colourPlaneId>PLANE_Y)? curRef->imgUV[mb->slice->colourPlaneId-1] : curRef->curPixelY;
    int dx = (x_pos & 3);
    int dy = (y_pos & 3);
    x_pos >>= 2;
    y_pos >>= 2;
    x_pos = iClip3(-18, maxold_x+2, x_pos);
    y_pos = iClip3(-10, maxold_y+2, y_pos);

    if (dx == 0 && dy == 0)
      get_block_00(&block[0][0], &curPixelY[y_pos][x_pos], curRef->iLumaStride, blockSizeY);
    else
    { /* other positions */
      if (dy == 0) /* No vertical interpolation */
      {
        if (dx == 1)
          get_luma_10(block, &curPixelY[ y_pos], blockSizeY, blockSizeX, x_pos, max_imgpel_value);
        else if (dx == 2)
          get_luma_20(block, &curPixelY[ y_pos], blockSizeY, blockSizeX, x_pos, max_imgpel_value);
        else
          get_luma_30(block, &curPixelY[ y_pos], blockSizeY, blockSizeX, x_pos, max_imgpel_value);
      }
      else if (dx == 0) /* No horizontal interpolation */
      {
        if (dy == 1)
          get_luma_01(block, &curPixelY[y_pos], blockSizeY, blockSizeX, x_pos, shift_x, max_imgpel_value);
        else if (dy == 2)
          get_luma_02(block, &curPixelY[ y_pos], blockSizeY, blockSizeX, x_pos, shift_x, max_imgpel_value);
        else
          get_luma_03(block, &curPixelY[ y_pos], blockSizeY, blockSizeX, x_pos, shift_x, max_imgpel_value);
      }
      else if (dx == 2)  /* Vertical & horizontal interpolation */
      {
        if (dy == 1)
          get_luma_21(block, &curPixelY[ y_pos], tempRes, blockSizeY, blockSizeX, x_pos, max_imgpel_value);
        else if (dy == 2)
          get_luma_22(block, &curPixelY[ y_pos], tempRes, blockSizeY, blockSizeX, x_pos, max_imgpel_value);
        else
          get_luma_23(block, &curPixelY[ y_pos], tempRes, blockSizeY, blockSizeX, x_pos, max_imgpel_value);
      }
      else if (dy == 2)
      {
        if (dx == 1)
          get_luma_12(block, &curPixelY[ y_pos], tempRes, blockSizeY, blockSizeX, x_pos, shift_x, max_imgpel_value);
        else
          get_luma_32(block, &curPixelY[ y_pos], tempRes, blockSizeY, blockSizeX, x_pos, shift_x, max_imgpel_value);
      }
      else
      {
        if (dx == 1)
        {
          if (dy == 1)
            get_luma_11(block, &curPixelY[ y_pos], blockSizeY, blockSizeX, x_pos, shift_x, max_imgpel_value);
          else
            get_luma_13(block, &curPixelY[ y_pos], blockSizeY, blockSizeX, x_pos, shift_x, max_imgpel_value);
        }
        else
        {
          if (dy == 1)
            get_luma_31(block, &curPixelY[ y_pos], blockSizeY, blockSizeX, x_pos, shift_x, max_imgpel_value);
          else
            get_luma_33(block, &curPixelY[ y_pos], blockSizeY, blockSizeX, x_pos, shift_x, max_imgpel_value);
        }
      }
    }
  }
}
//}}}

//{{{
static void get_chroma_0X (sPixel* block, sPixel* pixel, int span, int blockSizeY, int blockSizeX, int w00, int w01, int totalScale)
{
  sPixel *cur_row = pixel;
  sPixel *nxt_row = pixel + span;


  sPixel *cur_line, *cur_line_p1;
  sPixel *blk_line;
  int result;
  int i, j;
  for (j = 0; j < blockSizeY; j++)
  {
      cur_line    = cur_row;
      cur_line_p1 = nxt_row;
      blk_line = block;
      block += 16;
      cur_row = nxt_row;
      nxt_row += span;
    for (i = 0; i < blockSizeX; i++)
    {
      result = (w00 * *cur_line++ + w01 * *cur_line_p1++);
      *(blk_line++) = (sPixel) rshift_rnd_sf(result, totalScale);
    }
  }
}
//}}}
//{{{
static void get_chroma_X0 (sPixel* block, sPixel* pixel, int span, int blockSizeY, int blockSizeX, int w00, int w10, int totalScale)
{
  sPixel *cur_row = pixel;


    sPixel *cur_line, *cur_line_p1;
    sPixel *blk_line;
    int result;
    int i, j;
    for (j = 0; j < blockSizeY; j++)
    {
      cur_line    = cur_row;
      cur_line_p1 = cur_line + 1;
      blk_line = block;
      block += 16;
      cur_row += span;
      for (i = 0; i < blockSizeX; i++)
      {
        result = (w00 * *cur_line++ + w10 * *cur_line_p1++);
        //*(blk_line++) = (sPixel) iClip1(max_imgpel_value, rshift_rnd_sf(result, totalScale));
        *(blk_line++) = (sPixel) rshift_rnd_sf(result, totalScale);
      }
    }
}
//}}}
//{{{
static void get_chroma_XY (sPixel* block, sPixel* pixel, int span, int blockSizeY, int blockSizeX, int w00, int w01, int w10, int w11, int totalScale)
{
  sPixel *cur_row = pixel;
  sPixel *nxt_row = pixel + span;


  {
    sPixel *cur_line, *cur_line_p1;
    sPixel *blk_line;
    int result;
    int i, j;
    for (j = 0; j < blockSizeY; j++)
    {
      cur_line    = cur_row;
      cur_line_p1 = nxt_row;
      blk_line = block;
      block += 16;
      cur_row = nxt_row;
      nxt_row += span;
      for (i = 0; i < blockSizeX; i++)
      {
        result  = (w00 * *(cur_line++) + w01 * *(cur_line_p1++));
        result += (w10 * *(cur_line  ) + w11 * *(cur_line_p1  ));
        *(blk_line++) = (sPixel) rshift_rnd_sf(result, totalScale);
      }
    }
  }
}
//}}}
//{{{
static void get_block_chroma (sPicture* curRef, int x_pos, int y_pos, int subpelX, int subpelY, int maxold_x, int maxold_y,
                             int blockSizeX, int vert_block_size, int shiftpelX, int shiftpelY,
                             sPixel *block1, sPixel *block2, int totalScale, sPixel no_ref_value, sDecoder* decoder)
{
  sPixel *img1,*img2;
  short dx,dy;
  int span = curRef->iChromaStride;
  if (curRef->noRef) {
    //printf("list[ref_frame] is equal to 'no reference picture' before RAP\n");
    memset(block1,no_ref_value,vert_block_size * blockSizeX * sizeof(sPixel));
    memset(block2,no_ref_value,vert_block_size * blockSizeX * sizeof(sPixel));
  }
  else
  {
    dx = (short) (x_pos & subpelX);
    dy = (short) (y_pos & subpelY);
    x_pos = x_pos >> shiftpelX;
    y_pos = y_pos >> shiftpelY;
    //clip MV;
    x_pos = iClip3(-decoder->coding.iChromaPadX, maxold_x, x_pos); //16
    y_pos = iClip3(-decoder->coding.iChromaPadY, maxold_y, y_pos); //8
    img1 = &curRef->imgUV[0][y_pos][x_pos];
    img2 = &curRef->imgUV[1][y_pos][x_pos];

    if (dx == 0 && dy == 0)
    {
      get_block_00(block1, img1, span, vert_block_size);
      get_block_00(block2, img2, span, vert_block_size);
    }
    else
    {
      short dxcur = (short) (subpelX + 1 - dx);
      short dycur = (short) (subpelY + 1 - dy);
      short w00 = dxcur * dycur;
      if (dx == 0)
      {
        short w01 = dxcur * dy;
        get_chroma_0X(block1, img1, span, vert_block_size, blockSizeX, w00, w01, totalScale);
        get_chroma_0X(block2, img2, span, vert_block_size, blockSizeX, w00, w01, totalScale);
      }
      else if (dy == 0)
      {
        short w10 = dx * dycur;
        get_chroma_X0(block1, img1, span, vert_block_size, blockSizeX, w00, w10, totalScale);
        get_chroma_X0(block2, img2, span, vert_block_size, blockSizeX, w00, w10, totalScale);
      }
      else
      {
        short w01 = dxcur * dy;
        short w10 = dx * dycur;
        short w11 = dx * dy;
        get_chroma_XY(block1, img1, span, vert_block_size, blockSizeX, w00, w01, w10, w11, totalScale);
        get_chroma_XY(block2, img2, span, vert_block_size, blockSizeX, w00, w01, w10, w11, totalScale);
      }
    }
  }
}
//}}}
//{{{
void intra_cr_decoding (sMacroBlock* mb, int yuv)
{
  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture;
  sPixel** curUV;
  int uv;
  int b8,b4;
  int ioff, joff;
  int i,j;

  slice->intraPredChroma(mb);// last argument is ignored, computes needed data for both uv channels

  for(uv = 0; uv < 2; uv++)
  {
    mb->iTrans4x4 = (mb->isLossless == FALSE) ? itrans4x4 : itrans4x4_ls;

    curUV = picture->imgUV[uv];

    if(mb->isLossless)
    {
      if ((mb->cPredMode == VERT_PRED_8)||(mb->cPredMode == HOR_PRED_8))
        Inv_Residual_trans_Chroma(mb, uv) ;
      else
      {
        for(j=0;j<decoder->mbCrSizeY;j++)
          for(i=0;i<decoder->mbCrSizeX;i++)
            slice->mbRess [uv+1][j][i]=slice->cof[uv+1][j][i];
      }
    }

    if ((!(mb->mbType == SI4MB) && (mb->cbp >> 4)) )
    {
      for (b8 = 0; b8 < (decoder->coding.numUvBlocks); b8++)
      {
        for(b4 = 0; b4 < 4; b4++)
        {
          joff = subblk_offset_y[yuv][b8][b4];
          ioff = subblk_offset_x[yuv][b8][b4];
          mb->iTrans4x4(mb, (eColorPlane) (uv + 1), ioff, joff);
          copy_Image_4x4(&curUV[mb->piccY + joff], &(slice->mbRec[uv + 1][joff]), mb->pixcX + ioff, ioff);
        }
      }
      slice->isResetCoefCr = FALSE;
    }
    else if (mb->mbType == SI4MB)
    {
      itrans_sp_cr(mb, uv);

      for (joff  = 0; joff < 8; joff += 4)
      {
        for(ioff = 0; ioff < 8;ioff+=4)
        {
          mb->iTrans4x4 (mb, (eColorPlane) (uv + 1), ioff, joff);
          copy_Image_4x4 (&curUV[mb->piccY + joff], &(slice->mbRec[uv + 1][joff]), mb->pixcX + ioff, ioff);
        }
      }
      slice->isResetCoefCr = FALSE;
    }
    else
    {
      for (b8 = 0; b8 < (decoder->coding.numUvBlocks); b8++)
      {
        for(b4 = 0; b4 < 4; b4++)
        {
          joff = subblk_offset_y[yuv][b8][b4];
          ioff = subblk_offset_x[yuv][b8][b4];
          copy_Image_4x4(&curUV[mb->piccY + joff], &(slice->mbPred[uv + 1][joff]), mb->pixcX + ioff, ioff);
        }
      }
    }
  }
}
//}}}

//{{{
static inline void set_direct_references (const sPixelPos* mb, char* l0_rFrame, char* l1_rFrame, sPicMotionParam** mvInfo)
{
  if (mb->available)
  {
    char *refIndex = mvInfo[mb->posY][mb->posX].refIndex;
    *l0_rFrame  = refIndex[LIST_0];
    *l1_rFrame  = refIndex[LIST_1];
  }
  else
  {
    *l0_rFrame  = -1;
    *l1_rFrame  = -1;
  }
}
//}}}
//{{{
static void set_direct_references_mb_field (const sPixelPos* mb, char* l0_rFrame, char* l1_rFrame, sPicMotionParam** mvInfo, sMacroBlock *mbData)
{
  if (mb->available)
  {
    char *refIndex = mvInfo[mb->posY][mb->posX].refIndex;
    if (mbData[mb->mbIndex].mbField)
    {
      *l0_rFrame  = refIndex[LIST_0];
      *l1_rFrame  = refIndex[LIST_1];
    }
    else
    {
      *l0_rFrame  = (refIndex[LIST_0] < 0) ? refIndex[LIST_0] : refIndex[LIST_0] * 2;
      *l1_rFrame  = (refIndex[LIST_1] < 0) ? refIndex[LIST_1] : refIndex[LIST_1] * 2;
    }
  }
  else
  {
    *l0_rFrame  = -1;
    *l1_rFrame  = -1;
  }
}
//}}}
//{{{
static void set_direct_references_mb_frame (const sPixelPos* mb, char* l0_rFrame, char* l1_rFrame, sPicMotionParam** mvInfo, sMacroBlock *mbData)
{
  if (mb->available)
  {
    char *refIndex = mvInfo[mb->posY][mb->posX].refIndex;
    if (mbData[mb->mbIndex].mbField)
    {
      *l0_rFrame  = (refIndex[LIST_0] >> 1);
      *l1_rFrame  = (refIndex[LIST_1] >> 1);
    }
    else
    {
      *l0_rFrame  = refIndex[LIST_0];
      *l1_rFrame  = refIndex[LIST_1];
    }
  }
  else
  {
    *l0_rFrame  = -1;
    *l1_rFrame  = -1;
  }
}
//}}}
//{{{
void prepare_direct_params (sMacroBlock* mb, sPicture* picture, sMotionVec* pmvl0, sMotionVec *pmvl1, char *l0_rFrame, char *l1_rFrame)
{
  sSlice* slice = mb->slice;
  char l0_refA, l0_refB, l0_refC;
  char l1_refA, l1_refB, l1_refC;
  sPicMotionParam** mvInfo = picture->mvInfo;

  sPixelPos pixelPos[4];

  getNeighbours (mb, pixelPos, 0, 0, 16);

  if (!slice->mbAffFrame) {
    set_direct_references (&pixelPos[0], &l0_refA, &l1_refA, mvInfo);
    set_direct_references (&pixelPos[1], &l0_refB, &l1_refB, mvInfo);
    set_direct_references (&pixelPos[2], &l0_refC, &l1_refC, mvInfo);
    }
  else {
    sDecoder* decoder = mb->decoder;
    if (mb->mbField) {
      set_direct_references_mb_field (&pixelPos[0], &l0_refA, &l1_refA, mvInfo, decoder->mbData);
      set_direct_references_mb_field (&pixelPos[1], &l0_refB, &l1_refB, mvInfo, decoder->mbData);
      set_direct_references_mb_field (&pixelPos[2], &l0_refC, &l1_refC, mvInfo, decoder->mbData);
      }
    else {
      set_direct_references_mb_frame (&pixelPos[0], &l0_refA, &l1_refA, mvInfo, decoder->mbData);
      set_direct_references_mb_frame (&pixelPos[1], &l0_refB, &l1_refB, mvInfo, decoder->mbData);
      set_direct_references_mb_frame (&pixelPos[2], &l0_refC, &l1_refC, mvInfo, decoder->mbData);
      }
    }

  *l0_rFrame = (char)imin (imin((unsigned char)l0_refA, (unsigned char)l0_refB), (unsigned char)l0_refC);
  *l1_rFrame = (char)imin (imin ((unsigned char)l1_refA, (unsigned char)l1_refB), (unsigned char)l1_refC);

  if (*l0_rFrame >= 0)
    mb->GetMVPredictor (mb, pixelPos, pmvl0, *l0_rFrame, mvInfo, LIST_0, 0, 0, 16, 16);

  if (*l1_rFrame >= 0)
    mb->GetMVPredictor (mb, pixelPos, pmvl1, *l1_rFrame, mvInfo, LIST_1, 0, 0, 16, 16);
  }
//}}}

//{{{
static void check_motion_vector_range (const sMotionVec *mv, sSlice *slice) {

  if (mv->mvX > 8191 || mv->mvX < -8192)
    fprintf(stderr,"WARNING! Horizontal motion vector %d is out of allowed range {-8192, 8191} in picture %d, macroblock %d\n", mv->mvX, slice->decoder->idrFrameNum, slice->mbIndex);

  if (mv->mvY > (slice->maxMbVmvR - 1) || mv->mvY < (-slice->maxMbVmvR))
    fprintf(stderr,"WARNING! Vertical motion vector %d is out of allowed range {%d, %d} in picture %d, macroblock %d\n", mv->mvY, (-slice->maxMbVmvR), (slice->maxMbVmvR - 1), slice->decoder->idrFrameNum, slice->mbIndex);
  }
//}}}
//{{{
static inline int check_vert_mv (int llimit, int vec1_y,int rlimit) {

  int y_pos = vec1_y >> 2;
  if(y_pos < llimit || y_pos > rlimit)
    return 1;
  else
    return 0;
  }
//}}}
//{{{
static void perform_mc_single_wp (sMacroBlock* mb, eColorPlane plane, sPicture* picture, int predDir, int i, int j, int blockSizeX, int blockSizeY)
{
  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  sSPS *activeSPS = slice->activeSPS;
  sPixel** tempBlockL0 = slice->tempBlockL0;
  sPixel** tempBlockL1 = slice->tempBlockL1;
  static const int mv_mul = 16; // 4 * 4
  int i4   = mb->blockX + i;
  int j4   = mb->blockY + j;
  int type = slice->sliceType;
  int chromaFormatIdc = picture->chromaFormatIdc;

  //===== Single List Prediction =====
  int ioff = (i << 2);
  int joff = (j << 2);
  sPicMotionParam *mvInfo = &picture->mvInfo[j4][i4];
  short       refIndex = mvInfo->refIndex[predDir];
  short       ref_idx_wp = refIndex;
  sMotionVec *mv_array = &mvInfo->mv[predDir];
  int listOffset = mb->listOffset;
  sPicture *list = slice->listX[listOffset + predDir][refIndex];
  int vec1_x, vec1_y;
  // vars for get_block_luma
  int maxold_x = picture->size_x_m1;
  int maxold_y = (mb->mbField) ? (picture->sizeY >> 1) - 1 : picture->size_y_m1;
  int shift_x  = picture->iLumaStride;
  int** tempRes = slice->tempRes;
  int max_imgpel_value = decoder->coding.maxPelValueComp[plane];
  sPixel no_ref_value = (sPixel) decoder->coding.dcPredValueComp[plane];
  //

  check_motion_vector_range(mv_array, slice);
  vec1_x = i4 * mv_mul + mv_array->mvX;
  vec1_y = (mb->blockYaff + j) * mv_mul + mv_array->mvY;
  if(blockSizeY > (decoder->coding.iLumaPadY-4) && CheckVertMV(mb, vec1_y, blockSizeY))
  {
    get_block_luma(list, vec1_x, vec1_y, blockSizeX, BLOCK_SIZE_8x8, tempBlockL0, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
    get_block_luma(list, vec1_x, vec1_y+BLOCK_SIZE_8x8_SP, blockSizeX, blockSizeY-BLOCK_SIZE_8x8, tempBlockL0+BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
  }
  else
    get_block_luma(list, vec1_x, vec1_y, blockSizeX, blockSizeY, tempBlockL0,shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);

  {
    int alpha_l0, wpOffset, wp_denom;
    if (mb->mbField && ((decoder->activePPS->weightedPredFlag&&(type==ePslice|| type == eSPslice))||(decoder->activePPS->weightedBiPredIdc==1 && (type==eBslice))))
      ref_idx_wp >>=1;
    alpha_l0  = slice->wpWeight[predDir][ref_idx_wp][plane];
    wpOffset = slice->wpOffset[predDir][ref_idx_wp][plane];
    wp_denom  = plane > 0 ? slice->chromaLog2weightDenom : slice->lumaLog2weightDenom;
    weighted_mc_prediction(&slice->mbPred[plane][joff], tempBlockL0, blockSizeY, blockSizeX, ioff, alpha_l0, wpOffset, wp_denom, max_imgpel_value);
  }

  if ((chromaFormatIdc != YUV400) && (chromaFormatIdc != YUV444) )
  {
    int ioff_cr,joff_cr,block_size_x_cr,block_size_y_cr;
    int vec1_y_cr = vec1_y + ((activeSPS->chromaFormatIdc == 1)? slice->chromaVectorAdjust[listOffset + predDir][refIndex] : 0);
    int totalScale = decoder->coding.totalScale;
    int maxold_x = picture->size_x_cr_m1;
    int maxold_y = (mb->mbField) ? (picture->sizeYcr >> 1) - 1 : picture->size_y_cr_m1;
    int chroma_log2_weight = slice->chromaLog2weightDenom;
    if (decoder->mbCrSizeX == MB_BLOCK_SIZE)
    {
      ioff_cr = ioff;
      block_size_x_cr = blockSizeX;
    }
    else
    {
      ioff_cr = ioff >> 1;
      block_size_x_cr = blockSizeX >> 1;
    }
    if (decoder->mbCrSizeY == MB_BLOCK_SIZE)
    {
      joff_cr = joff;
      block_size_y_cr = blockSizeY;
    }
    else
    {
      joff_cr = joff >> 1;
      block_size_y_cr = blockSizeY >> 1;
    }
    no_ref_value = (sPixel)decoder->coding.dcPredValueComp[1];
    {
      int *weight = slice->wpWeight[predDir][ref_idx_wp];
      int *offset = slice->wpOffset[predDir][ref_idx_wp];
      get_block_chroma(list,vec1_x,vec1_y_cr,decoder->coding.subpelX,decoder->coding.subpelY,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,decoder->coding.shiftpelX,decoder->coding.shiftpelY,&tempBlockL0[0][0],&tempBlockL1[0][0] ,totalScale,no_ref_value,decoder);
      weighted_mc_prediction(&slice->mbPred[1][joff_cr], tempBlockL0, block_size_y_cr, block_size_x_cr, ioff_cr, weight[1], offset[1], chroma_log2_weight, decoder->coding.maxPelValueComp[1]);
      weighted_mc_prediction(&slice->mbPred[2][joff_cr], tempBlockL1, block_size_y_cr, block_size_x_cr, ioff_cr, weight[2], offset[2], chroma_log2_weight, decoder->coding.maxPelValueComp[2]);
    }
  }
}
//}}}
//{{{
static void perform_mc_single (sMacroBlock* mb, eColorPlane plane, sPicture* picture, int predDir, int i, int j, int blockSizeX, int blockSizeY)
{
  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;
  sSPS *activeSPS = slice->activeSPS;
  sPixel** tempBlockL0 = slice->tempBlockL0;
  sPixel** tempBlockL1 = slice->tempBlockL1;
  static const int mv_mul = 16; // 4 * 4
  int i4   = mb->blockX + i;
  int j4   = mb->blockY + j;
  int chromaFormatIdc = picture->chromaFormatIdc;
  //===== Single List Prediction =====
  int ioff = (i << 2);
  int joff = (j << 2);
  sPicMotionParam *mvInfo = &picture->mvInfo[j4][i4];
  sMotionVec *mv_array = &mvInfo->mv[predDir];
  short          refIndex =  mvInfo->refIndex[predDir];
  int listOffset = mb->listOffset;
  sPicture *list = slice->listX[listOffset + predDir][refIndex];
  int vec1_x, vec1_y;
  // vars for get_block_luma
  int maxold_x = picture->size_x_m1;
  int maxold_y = (mb->mbField) ? (picture->sizeY >> 1) - 1 : picture->size_y_m1;
  int shift_x  = picture->iLumaStride;
  int** tempRes = slice->tempRes;
  int max_imgpel_value = decoder->coding.maxPelValueComp[plane];
  sPixel no_ref_value = (sPixel) decoder->coding.dcPredValueComp[plane];


  check_motion_vector_range(mv_array, slice);
  vec1_x = i4 * mv_mul + mv_array->mvX;
  vec1_y = (mb->blockYaff + j) * mv_mul + mv_array->mvY;

  if (blockSizeY > (decoder->coding.iLumaPadY-4) && CheckVertMV(mb, vec1_y, blockSizeY))
  {
    get_block_luma(list, vec1_x, vec1_y, blockSizeX, BLOCK_SIZE_8x8, tempBlockL0, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
    get_block_luma(list, vec1_x, vec1_y+BLOCK_SIZE_8x8_SP, blockSizeX, blockSizeY-BLOCK_SIZE_8x8, tempBlockL0+BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
  }
  else
    get_block_luma(list, vec1_x, vec1_y, blockSizeX, blockSizeY, tempBlockL0,shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);

  mc_prediction(&slice->mbPred[plane][joff], tempBlockL0, blockSizeY, blockSizeX, ioff);

  if ((chromaFormatIdc != YUV400) && (chromaFormatIdc != YUV444) )
  {
    int ioff_cr,joff_cr,block_size_x_cr,block_size_y_cr;
    int vec1_y_cr = vec1_y + ((activeSPS->chromaFormatIdc == 1)? slice->chromaVectorAdjust[listOffset + predDir][refIndex] : 0);
    int totalScale = decoder->coding.totalScale;
    int maxold_x = picture->size_x_cr_m1;
    int maxold_y = (mb->mbField) ? (picture->sizeYcr >> 1) - 1 : picture->size_y_cr_m1;
    if (decoder->mbCrSizeX == MB_BLOCK_SIZE)
    {
      ioff_cr = ioff;
      block_size_x_cr = blockSizeX;
    }
    else
    {
      ioff_cr = ioff >> 1;
      block_size_x_cr = blockSizeX >> 1;
    }
    if (decoder->mbCrSizeY == MB_BLOCK_SIZE)
    {
      joff_cr = joff;
      block_size_y_cr = blockSizeY;
    }
    else
    {
      joff_cr = joff >> 1;
      block_size_y_cr = blockSizeY >> 1;
    }
    no_ref_value = (sPixel)decoder->coding.dcPredValueComp[1];
    get_block_chroma(list,vec1_x,vec1_y_cr,decoder->coding.subpelX,decoder->coding.subpelY,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,decoder->coding.shiftpelX,decoder->coding.shiftpelY,&tempBlockL0[0][0],&tempBlockL1[0][0] ,totalScale,no_ref_value,decoder);
    mc_prediction(&slice->mbPred[1][joff_cr], tempBlockL0, block_size_y_cr, block_size_x_cr, ioff_cr);
    mc_prediction(&slice->mbPred[2][joff_cr], tempBlockL1, block_size_y_cr, block_size_x_cr, ioff_cr);
  }
}
//}}}
//{{{
static void perform_mc_bi_wp (sMacroBlock* mb, eColorPlane plane, sPicture* picture, int i, int j, int blockSizeX, int blockSizeY)
{
  static const int mv_mul = 16;
  int  vec1_x, vec1_y, vec2_x, vec2_y;
  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;

  int weightedBiPredIdc = decoder->activePPS->weightedBiPredIdc;
  int blockYaff = mb->blockYaff;
  int i4 = mb->blockX + i;
  int j4 = mb->blockY + j;
  int ioff = (i << 2);
  int joff = (j << 2);
  int chromaFormatIdc = picture->chromaFormatIdc;
  int listOffset = mb->listOffset;
  sPicMotionParam *mvInfo = &picture->mvInfo[j4][i4];
  sMotionVec *l0_mv_array = &mvInfo->mv[LIST_0];
  sMotionVec *l1_mv_array = &mvInfo->mv[LIST_1];
  short l0_refframe = mvInfo->refIndex[LIST_0];
  short l1_refframe = mvInfo->refIndex[LIST_1];
  int l0_ref_idx  = (mb->mbField && weightedBiPredIdc == 1) ? l0_refframe >> 1: l0_refframe;
  int l1_ref_idx  = (mb->mbField && weightedBiPredIdc == 1) ? l1_refframe >> 1: l1_refframe;


  /// WP Parameters
  int wt_list_offset = (weightedBiPredIdc==2)? listOffset : 0;
  int *weight0 = slice->wbpWeight[LIST_0 + wt_list_offset][l0_ref_idx][l1_ref_idx];
  int *weight1 = slice->wbpWeight[LIST_1 + wt_list_offset][l0_ref_idx][l1_ref_idx];
  int *offset0 = slice->wpOffset[LIST_0 + wt_list_offset][l0_ref_idx];
  int *offset1 = slice->wpOffset[LIST_1 + wt_list_offset][l1_ref_idx];
  int maxold_y = (mb->mbField) ? (picture->sizeY >> 1) - 1 : picture->size_y_m1;
  int pady = decoder->coding.iLumaPadY;
  int rlimit = maxold_y + pady - blockSizeY - 2;
  int llimit = 2 - pady;
  int big_blocky = blockSizeY > (pady - 4);
  sPicture *list0 = slice->listX[LIST_0 + listOffset][l0_refframe];
  sPicture *list1 = slice->listX[LIST_1 + listOffset][l1_refframe];
  sPixel** tempBlockL0 = slice->tempBlockL0;
  sPixel *block0 = tempBlockL0[0];
  sPixel** tempBlockL1 = slice->tempBlockL1;
  sPixel *block1 = tempBlockL1[0];
  sPixel** tempBlockL2 = slice->tempBlockL2;
  sPixel *block2 = tempBlockL2[0];
  sPixel** tempBlockL3 = slice->tempBlockL3;
  sPixel *block3 = tempBlockL3[0];
  int wpOffset;
  int wp_denom;

  // vars for get_block_luma
  int maxold_x = picture->size_x_m1;
  int shift_x  = picture->iLumaStride;
  int** tempRes = slice->tempRes;
  int max_imgpel_value = decoder->coding.maxPelValueComp[plane];
  sPixel no_ref_value = (sPixel) decoder->coding.dcPredValueComp[plane];

  check_motion_vector_range(l0_mv_array, slice);
  check_motion_vector_range(l1_mv_array, slice);
  vec1_x = i4 * mv_mul + l0_mv_array->mvX;
  vec2_x = i4 * mv_mul + l1_mv_array->mvX;
  vec1_y = (blockYaff + j) * mv_mul + l0_mv_array->mvY;
  vec2_y = (blockYaff + j) * mv_mul + l1_mv_array->mvY;

  if (big_blocky && check_vert_mv(llimit, vec1_y, rlimit))
  {
    get_block_luma(list0, vec1_x, vec1_y, blockSizeX, BLOCK_SIZE_8x8, tempBlockL0, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
    get_block_luma(list0, vec1_x, vec1_y+BLOCK_SIZE_8x8_SP, blockSizeX, blockSizeY-BLOCK_SIZE_8x8, tempBlockL0+BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
  }
  else
    get_block_luma(list0, vec1_x, vec1_y, blockSizeX, blockSizeY, tempBlockL0,shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
  if (big_blocky && check_vert_mv(llimit, vec2_y,rlimit))
  {
    get_block_luma(list1, vec2_x, vec2_y, blockSizeX, BLOCK_SIZE_8x8, tempBlockL1, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
    get_block_luma(list1, vec2_x, vec2_y+BLOCK_SIZE_8x8_SP, blockSizeX, blockSizeY-BLOCK_SIZE_8x8, tempBlockL1 + BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
  }
  else
    get_block_luma(list1, vec2_x, vec2_y, blockSizeX, blockSizeY, tempBlockL1,shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);


  wpOffset = ((offset0[plane] + offset1[plane] + 1) >>1);
  wp_denom  = plane > 0 ? slice->chromaLog2weightDenom : slice->lumaLog2weightDenom;
  weighted_bi_prediction(&slice->mbPred[plane][joff][ioff], block0, block1, blockSizeY, blockSizeX, weight0[plane], weight1[plane], wpOffset, wp_denom + 1, max_imgpel_value);

  if ((chromaFormatIdc != YUV400) && (chromaFormatIdc != YUV444) )
  {
    int ioff_cr, joff_cr,block_size_y_cr,block_size_x_cr,vec2_y_cr,vec1_y_cr;
    int maxold_x = picture->size_x_cr_m1;
    int maxold_y = (mb->mbField) ? (picture->sizeYcr >> 1) - 1 : picture->size_y_cr_m1;
    int shiftpelX = decoder->coding.shiftpelX;
    int shiftpelY = decoder->coding.shiftpelY;
    int subpelX = decoder->coding.subpelX;
    int subpelY =  decoder->coding.subpelY;
    int totalScale = decoder->coding.totalScale;
    int chroma_log2 = slice->chromaLog2weightDenom + 1;

    if (decoder->mbCrSizeX == MB_BLOCK_SIZE)
    {
      ioff_cr =  ioff;
      block_size_x_cr =  blockSizeX;
    }
    else
    {
      ioff_cr = ioff >> 1;
      block_size_x_cr =  blockSizeX >> 1;
    }

    if (decoder->mbCrSizeY == MB_BLOCK_SIZE)
    {
      joff_cr = joff;
      block_size_y_cr = blockSizeY;
    }
    else
    {
      joff_cr = joff >> 1;
      block_size_y_cr = blockSizeY >> 1;
    }
    if (chromaFormatIdc == 1)
    {
      vec1_y_cr = vec1_y + slice->chromaVectorAdjust[LIST_0 + listOffset][l0_refframe];
      vec2_y_cr = vec2_y + slice->chromaVectorAdjust[LIST_1 + listOffset][l1_refframe];
    }
    else
    {
      vec1_y_cr = vec1_y;
      vec2_y_cr = vec2_y;
    }
    no_ref_value = (sPixel)decoder->coding.dcPredValueComp[1];

    wpOffset = ((offset0[1] + offset1[1] + 1) >>1);
    get_block_chroma(list0,vec1_x,vec1_y_cr,subpelX,subpelY,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,shiftpelX,shiftpelY,block0,block2 ,totalScale,no_ref_value,decoder);
    get_block_chroma(list1,vec2_x,vec2_y_cr,subpelX,subpelY,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,shiftpelX,shiftpelY,block1,block3 ,totalScale,no_ref_value,decoder);
    weighted_bi_prediction(&slice->mbPred[1][joff_cr][ioff_cr],block0,block1,block_size_y_cr,block_size_x_cr,weight0[1],weight1[1],wpOffset,chroma_log2,decoder->coding.maxPelValueComp[1]);
    wpOffset = ((offset0[2] + offset1[2] + 1) >>1);
    weighted_bi_prediction(&slice->mbPred[2][joff_cr][ioff_cr],block2,block3,block_size_y_cr,block_size_x_cr,weight0[2],weight1[2],wpOffset,chroma_log2,decoder->coding.maxPelValueComp[2]);
  }
}
//}}}
//{{{
static void perform_mc_bi (sMacroBlock* mb, eColorPlane plane, sPicture* picture, int i, int j, int blockSizeX, int blockSizeY)
{
  static const int mv_mul = 16;
  int vec1_x=0, vec1_y=0, vec2_x=0, vec2_y=0;
  sDecoder* decoder = mb->decoder;
  sSlice* slice = mb->slice;

  int blockYaff = mb->blockYaff;
  int i4 = mb->blockX + i;
  int j4 = mb->blockY + j;
  int ioff = (i << 2);
  int joff = (j << 2);
  int chromaFormatIdc = picture->chromaFormatIdc;
  sPicMotionParam *mvInfo = &picture->mvInfo[j4][i4];
  sMotionVec *l0_mv_array = &mvInfo->mv[LIST_0];
  sMotionVec *l1_mv_array = &mvInfo->mv[LIST_1];
  short l0_refframe = mvInfo->refIndex[LIST_0];
  short l1_refframe = mvInfo->refIndex[LIST_1];
  int listOffset = mb->listOffset;

  int maxold_y = (mb->mbField) ? (picture->sizeY >> 1) - 1 : picture->size_y_m1;
  int pady = decoder->coding.iLumaPadY;
  int rlimit = maxold_y + pady - blockSizeY - 2;
  int llimit = 2 - pady;
  int big_blocky = blockSizeY > (pady - 4);
  sPicture *list0 = slice->listX[LIST_0 + listOffset][l0_refframe];
  sPicture *list1 = slice->listX[LIST_1 + listOffset][l1_refframe];
  sPixel** tempBlockL0 = slice->tempBlockL0;
  sPixel *block0 = tempBlockL0[0];
  sPixel** tempBlockL1 = slice->tempBlockL1;
  sPixel *block1 = tempBlockL1[0];
  sPixel** tempBlockL2 = slice->tempBlockL2;
  sPixel *block2 = tempBlockL2[0];
  sPixel** tempBlockL3 = slice->tempBlockL3;
  sPixel *block3 = tempBlockL3[0];
  // vars for get_block_luma
  int maxold_x = picture->size_x_m1;
  int shift_x  = picture->iLumaStride;
  int** tempRes = slice->tempRes;
  int max_imgpel_value = decoder->coding.maxPelValueComp[plane];
  sPixel no_ref_value = (sPixel) decoder->coding.dcPredValueComp[plane];
  check_motion_vector_range(l0_mv_array, slice);
  check_motion_vector_range(l1_mv_array, slice);
  vec1_x = i4 * mv_mul + l0_mv_array->mvX;
  vec2_x = i4 * mv_mul + l1_mv_array->mvX;
  vec1_y = (blockYaff + j) * mv_mul + l0_mv_array->mvY;
  vec2_y = (blockYaff + j) * mv_mul + l1_mv_array->mvY;
  if (big_blocky && check_vert_mv(llimit, vec1_y, rlimit))
  {
    get_block_luma(list0, vec1_x, vec1_y, blockSizeX, BLOCK_SIZE_8x8, tempBlockL0, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
    get_block_luma(list0, vec1_x, vec1_y+BLOCK_SIZE_8x8_SP, blockSizeX, blockSizeY-BLOCK_SIZE_8x8, tempBlockL0+BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
  }
  else
    get_block_luma(list0, vec1_x, vec1_y, blockSizeX, blockSizeY, tempBlockL0,shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
  if (big_blocky && check_vert_mv(llimit, vec2_y,rlimit))
  {
    get_block_luma(list1, vec2_x, vec2_y, blockSizeX, BLOCK_SIZE_8x8, tempBlockL1, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
    get_block_luma(list1, vec2_x, vec2_y+BLOCK_SIZE_8x8_SP, blockSizeX, blockSizeY-BLOCK_SIZE_8x8, tempBlockL1 + BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
  }
  else
    get_block_luma(list1, vec2_x, vec2_y, blockSizeX, blockSizeY, tempBlockL1,shift_x,maxold_x,maxold_y,tempRes,max_imgpel_value,no_ref_value, mb);
  bi_prediction(&slice->mbPred[plane][joff],tempBlockL0,tempBlockL1, blockSizeY, blockSizeX, ioff);

  if ((chromaFormatIdc != YUV400) && (chromaFormatIdc != YUV444) )
  {
    int ioff_cr, joff_cr,block_size_y_cr,block_size_x_cr,vec2_y_cr,vec1_y_cr;
    int chromaFormatIdc = decoder->activeSPS->chromaFormatIdc;
    int maxold_x = picture->size_x_cr_m1;
    int maxold_y = (mb->mbField) ? (picture->sizeYcr >> 1) - 1 : picture->size_y_cr_m1;
    int shiftpelX = decoder->coding.shiftpelX;
    int shiftpelY = decoder->coding.shiftpelY;
    int subpelX = decoder->coding.subpelX;
    int subpelY =  decoder->coding.subpelY;
    int totalScale = decoder->coding.totalScale;
    if (decoder->mbCrSizeX == MB_BLOCK_SIZE)
    {
      ioff_cr =  ioff;
      block_size_x_cr =  blockSizeX;
    }
    else
    {
      ioff_cr = ioff >> 1;
      block_size_x_cr =  blockSizeX >> 1;
    }
    if (decoder->mbCrSizeY == MB_BLOCK_SIZE)
    {
      joff_cr = joff;
      block_size_y_cr = blockSizeY;
    }
    else
    {
      joff_cr = joff >> 1;
      block_size_y_cr = blockSizeY >> 1;
    }
    if (chromaFormatIdc == 1)
    {
      vec1_y_cr = vec1_y + slice->chromaVectorAdjust[LIST_0 + listOffset][l0_refframe];
      vec2_y_cr = vec2_y + slice->chromaVectorAdjust[LIST_1 + listOffset][l1_refframe];
    }
    else
    {
      vec1_y_cr = vec1_y;
      vec2_y_cr = vec2_y;
    }
    no_ref_value = (sPixel)decoder->coding.dcPredValueComp[1];
    get_block_chroma(list0,vec1_x,vec1_y_cr,subpelX,subpelY,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,shiftpelX,shiftpelY,block0,block2 ,totalScale,no_ref_value,decoder);
    get_block_chroma(list1,vec2_x,vec2_y_cr,subpelX,subpelY,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,shiftpelX,shiftpelY,block1,block3 ,totalScale,no_ref_value,decoder);
    bi_prediction(&slice->mbPred[1][joff_cr],tempBlockL0,tempBlockL1, block_size_y_cr, block_size_x_cr, ioff_cr);
    bi_prediction(&slice->mbPred[2][joff_cr],tempBlockL2,tempBlockL3, block_size_y_cr, block_size_x_cr, ioff_cr);
  }
}
//}}}
//{{{
void perform_mc (sMacroBlock* mb, eColorPlane plane, sPicture* picture,
                 int predDir, int i, int j, int blockSizeX, int blockSizeY) {

  sSlice* slice = mb->slice;
  if (predDir != 2) {
    if (slice->weightedPredFlag)
      perform_mc_single_wp(mb, plane, picture, predDir, i, j, blockSizeX, blockSizeY);
    else
      perform_mc_single(mb, plane, picture, predDir, i, j, blockSizeX, blockSizeY);
    }
  else {
    if (slice->weightedBiPredIdc)
      perform_mc_bi_wp(mb, plane, picture, i, j, blockSizeX, blockSizeY);
    else
      perform_mc_bi(mb, plane, picture, i, j, blockSizeX, blockSizeY);
    }
  }
//}}}
