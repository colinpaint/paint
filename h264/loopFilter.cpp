//{{{  includes
#include "global.h"

#include "macroblock.h"
#include "loopfilter.h"
//}}}
//{{{  defines
#define get_x_luma(x) (x & 15)
#define get_y_luma(y) (y & 15)
#define get_pos_x_luma(mb,x) (mb->pixX + (x & 15))
#define get_pos_y_luma(mb,y) (mb->pixY + (y & 15))
#define get_pos_x_chroma(mb,x,max) (mb->pixcX + (x & max))
#define get_pos_y_chroma(mb,y,max) (mb->piccY + (y & max))
//}}}
namespace {
  //{{{  const tables
  //{{{
  const uint8_t ALPHA_TABLE[52] = {
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,4,4,5,6,
    7,8,9,10,12,13,15,17,
    20,22,25,28,32,36,40,45,
    50,56,63,71,80,90,101,113,
    127,144,162,182,203,226,255,255} ;
  //}}}
  //{{{
  const uint8_t BETA_TABLE[52] = {
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,2,2,2,3,
    3,3,3, 4, 4, 4, 6, 6,
    7, 7, 8, 8, 9, 9,10,10,
    11,11,12,12,13,13, 14, 14,
    15, 15, 16, 16, 17, 17, 18, 18} ;
  //}}}
  //{{{
  const uint8_t CLIP_TAB[52][5] = {
    { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 1, 1, 1, 1},
    { 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 2, 3, 3},
    { 0, 1, 2, 3, 3},{ 0, 2, 2, 3, 3},{ 0, 2, 2, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 3, 3, 5, 5},{ 0, 3, 4, 6, 6},{ 0, 3, 4, 6, 6},
    { 0, 4, 5, 7, 7},{ 0, 4, 5, 8, 8},{ 0, 4, 6, 9, 9},{ 0, 5, 7,10,10},{ 0, 6, 8,11,11},{ 0, 6, 8,13,13},{ 0, 7,10,14,14},{ 0, 8,11,16,16},
    { 0, 9,12,18,18},{ 0,10,13,20,20},{ 0,11,15,23,23},{ 0,13,17,25,25}
  } ;
  //}}}
  //{{{
  const char chroma_edge[2][4][4] = {
    { {-4, 0, 0, 0},
      {-4,-4,-4, 4},
      {-4, 4, 4, 8},
      {-4,-4,-4, 12}},

    { {-4, 0,  0,  0},
      {-4,-4,  4,  4},
      {-4, 4,  8,  8},
      {-4,-4, 12, 12}}};
  //}}}
  //{{{
  const int pelnum_cr[2][4] =  {
    {0,8,16,16},
    {0,8, 8,16}};  //[dir:0=vert, 1=hor.][yuvFormat]
  //}}}
  //}}}

  //{{{
  int compare_mvs (const sMotionVec* mv0, const sMotionVec* mv1, int mvlimit) {
    return ((iabs (mv0->mvX - mv1->mvX) >= 4) | (iabs (mv0->mvY - mv1->mvY) >= mvlimit));
    }
  //}}}
  //{{{
  sMacroBlock* get_non_aff_neighbour_luma (sMacroBlock* mb, int xN, int yN) {

    if (xN < 0)
      return (mb->mbLeft);
    else if (yN < 0)
      return (mb->mbUp);
    else
      return (mb);
    }
  //}}}
  //{{{
  sMacroBlock* get_non_aff_neighbour_chroma (sMacroBlock* mb, int xN, int yN,
                                                   int block_width,int block_height) {
    if (xN < 0) {
      if (yN < block_height)
        return (mb->mbLeft);
      else
        return (NULL);
      }
    else if (xN < block_width) {
      if (yN < 0)
        return (mb->mbUp);
      else if (yN < block_height)
        return (mb);
      else
        return (NULL);
      }
    else
      return (NULL);
    }
  //}}}

  // mbaff
  //{{{
  void edge_loop_luma_ver_MBAff (eColorPlane plane, sPixel** img, uint8_t* Strength,
                                 sMacroBlock* mbQ, int edge) {

    int      pel, Strng ;
    sPixel   L2 = 0, L1, L0, R0, R1, R2 = 0;
    int      Alpha = 0, Beta = 0 ;
    const uint8_t* ClipTab = NULL;
    int      indexA, indexB;
    int      QP;
    sPixelPos pixP, pixQ;

    cDecoder264* decoder = mbQ->decoder;
    int bitDepthScale = plane ? decoder->coding.bitDepthScale[eChroma] : decoder->coding.bitDepthScale[eLuma];
    int max_imgpel_value = decoder->coding.maxPelValueComp[plane];

    int AlphaC0Offset = mbQ->deblockFilterC0Offset;
    int BetaOffset = mbQ->deblockFilterBetaOffset;

    sMacroBlock*MbP;
    sPixel* SrcPtrP, *SrcPtrQ;

    for (pel = 0 ; pel < MB_BLOCK_SIZE ; ++pel ) {
      getAffNeighbour (mbQ, edge - 1, pel, decoder->mbSize[eLuma], &pixP);

      if  (pixP.ok || (mbQ->deblockFilterDisableIdc == 0)) {
        if ((Strng = Strength[pel]) != 0) {
          getAffNeighbour (mbQ, edge, pel, decoder->mbSize[eLuma], &pixQ);

          MbP = &(decoder->mbData[pixP.mbIndex]);
          SrcPtrQ = &(img[pixQ.posY][pixQ.posX]);
          SrcPtrP = &(img[pixP.posY][pixP.posX]);

          // Average QP of the two blocks
          QP = plane? ((MbP->qpc[plane-1] + mbQ->qpc[plane-1] + 1) >> 1) : (MbP->qp + mbQ->qp + 1) >> 1;
          indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
          indexB = iClip3(0, MAX_QP, QP + BetaOffset);
          Alpha = ALPHA_TABLE[indexA] * bitDepthScale;
          Beta = BETA_TABLE [indexB] * bitDepthScale;
          ClipTab = CLIP_TAB[indexA];

          L0 = SrcPtrP[ 0] ;
          R0 = SrcPtrQ[ 0] ;
          if (iabs (R0 - L0 ) < Alpha) {
            L1 = SrcPtrP[-1];
            R1 = SrcPtrQ[ 1];
            if ((iabs (R0 - R1) < Beta )   && (iabs(L0 - L1) < Beta )) {
              L2 = SrcPtrP[-2];
              R2 = SrcPtrQ[ 2];
              if (Strng == 4 )  {  // INTRA strong filtering
                int RL0 = L0 + R0;
                int small_gap = (iabs (R0 - L0 ) < ((Alpha >> 2) + 2));
                int aq =  (iabs (R0 - R2) < Beta ) & small_gap;
                int ap =  (iabs (L0 - L2) < Beta ) & small_gap;
                if (ap) {
                  int L3  = SrcPtrP[-3];
                  SrcPtrP[-2 ] = (sPixel) ((((L3 + L2) << 1) + L2 + L1 + RL0 + 4) >> 3);
                  SrcPtrP[-1 ] = (sPixel) ((L2 + L1 + L0 + R0 + 2) >> 2);
                  SrcPtrP[ 0 ] = (sPixel) ((R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3);
                  }
                else
                  SrcPtrP[ 0 ] = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2) ;

                if (aq) {
                  sPixel R3  = SrcPtrQ[ 3];
                  SrcPtrQ[ 0 ] = (sPixel) ((L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3);
                  SrcPtrQ[ 1 ] = (sPixel) ((R2 + R0 + R1 + L0 + 2) >> 2);
                  SrcPtrQ[ 2 ] = (sPixel) ((((R3 + R2) << 1) + R2 + R1 + RL0 + 4) >> 3);
                  }
                else
                  SrcPtrQ[ 0 ] = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
                }
              else   {
                // normal filtering
                int RL0 = (L0 + R0 + 1) >> 1;
                int aq = (iabs (R0 - R2) < Beta);
                int ap = (iabs (L0 - L2) < Beta);

                int C0 = ClipTab[ Strng ] * bitDepthScale;
                int tc0 = (C0 + ap + aq) ;
                int dif = iClip3 (-tc0, tc0, (((R0 - L0) << 2) + (L1 - R1) + 4) >> 3) ;

                if (ap && (C0 != 0))
                  *(SrcPtrP - 1) += iClip3 (-C0,  C0, ( L2 + RL0 - (L1 << 1)) >> 1 ) ;

                if (dif) {
                  *SrcPtrP  = (sPixel) iClip1 (max_imgpel_value, L0 + dif) ;
                  *SrcPtrQ  = (sPixel) iClip1 (max_imgpel_value, R0 - dif) ;
                  }

                if (aq  && (C0 != 0))
                  *(SrcPtrQ + 1) += iClip3 (-C0,  C0, ( R2 + RL0 - (R1 << 1)) >> 1 ) ;
                }
              }
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void edge_loop_luma_hor_MBAff (eColorPlane plane, sPixel** img, uint8_t* Strength,
                                 sMacroBlock* mbQ, int edge, sPicture *p)
  {
    int      width = p->lumaStride; //p->sizeX;
    int      pel, Strng ;
    int      PelNum = plane? pelnum_cr[1][p->chromaFormatIdc] : MB_BLOCK_SIZE;

    int      yQ = (edge < MB_BLOCK_SIZE ? edge : 1);

    sPixelPos pixP, pixQ;

    cDecoder264* decoder = mbQ->decoder;
    int      bitDepthScale = plane? decoder->coding.bitDepthScale[eChroma] : decoder->coding.bitDepthScale[eLuma];
    int      max_imgpel_value = decoder->coding.maxPelValueComp[plane];

    getAffNeighbour(mbQ, 0, yQ - 1, decoder->mbSize[eLuma], &pixP);

    if (pixP.ok || (mbQ->deblockFilterDisableIdc == 0)) {
      int AlphaC0Offset = mbQ->deblockFilterC0Offset;
      int BetaOffset = mbQ->deblockFilterBetaOffset;

      sMacroBlock *MbP = &(decoder->mbData[pixP.mbIndex]);

      int incQ    = ((MbP->mbField && !mbQ->mbField) ? 2 * width : width);
      int incP    = ((mbQ->mbField && !MbP->mbField) ? 2 * width : width);

      // Average QP of the two blocks
      int QP = plane? ((MbP->qpc[plane - 1] + mbQ->qpc[plane - 1] + 1) >> 1) : (MbP->qp + mbQ->qp + 1) >> 1;
      int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
      int indexB = iClip3(0, MAX_QP, QP + BetaOffset);
      int Alpha   = ALPHA_TABLE[indexA] * bitDepthScale;
      int Beta    = BETA_TABLE [indexB] * bitDepthScale;
      if ((Alpha | Beta )!= 0) {
        const uint8_t* ClipTab = CLIP_TAB[indexA];
        getAffNeighbour(mbQ, 0, yQ, decoder->mbSize[eLuma], &pixQ);

        for (pel = 0 ; pel < PelNum ; ++pel ) {
          if ((Strng = Strength[pel]) != 0) {
            sPixel *SrcPtrQ = &(img[pixQ.posY][pixQ.posX]);
            sPixel *SrcPtrP = &(img[pixP.posY][pixP.posX]);

            sPixel L0  = *SrcPtrP;
            sPixel R0  = *SrcPtrQ;

            if (iabs (R0 - L0 ) < Alpha ) {
              sPixel L1  = SrcPtrP[-incP];
              sPixel R1  = SrcPtrQ[ incQ];

              if ((iabs (R0 - R1) < Beta )   && (iabs(L0 - L1) < Beta )) {
                sPixel L2  = SrcPtrP[-incP*2];
                sPixel R2  = SrcPtrQ[ incQ*2];
                if(Strng == 4 )  {  // INTRA strong filtering
                  int RL0 = L0 + R0;
                  int small_gap = (iabs (R0 - L0 ) < ((Alpha >> 2) + 2));
                  int aq = (iabs (R0 - R2) < Beta ) & small_gap;
                  int ap = (iabs (L0 - L2) < Beta ) & small_gap;

                  if (ap) {
                    sPixel L3  = SrcPtrP[-incP*3];
                    SrcPtrP[-incP * 2] = (sPixel) ((((L3 + L2) << 1) + L2 + L1 + RL0 + 4) >> 3);
                    SrcPtrP[-incP    ] = (sPixel) (( L2 + L1 + L0 + R0 + 2) >> 2);
                    SrcPtrP[    0    ] = (sPixel) (( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3);
                  }
                  else
                    SrcPtrP[     0     ] = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2) ;

                  if (aq) {
                    sPixel R3 = SrcPtrQ[ incQ*3];
                    SrcPtrQ[    0     ] = (sPixel) (( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3);
                    SrcPtrQ[ incQ     ] = (sPixel) (( R2 + R0 + R1 + L0 + 2) >> 2);
                    SrcPtrQ[ incQ * 2 ] = (sPixel) ((((R3 + R2) << 1) + R2 + R1 + RL0 + 4) >> 3);
                  }
                  else
                    SrcPtrQ[    0     ] = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
                }
                else  {
                  // normal filtering
                  int RL0 = (L0 + R0 + 1) >> 1;
                  int aq  = (iabs (R0 - R2) < Beta);
                  int ap  = (iabs (L0 - L2) < Beta);

                  int C0  = ClipTab[ Strng ] * bitDepthScale;
                  int tc0  = (C0 + ap + aq) ;
                  int dif = iClip3 (-tc0, tc0, (((R0 - L0) << 2) + (L1 - R1) + 4) >> 3) ;

                  if (ap && (C0 != 0))
                    *(SrcPtrP - incP) += iClip3 (-C0,  C0, ( L2 + RL0 - (L1 << 1)) >> 1 ) ;

                  if (dif) {
                    *SrcPtrP  = (sPixel) iClip1 (max_imgpel_value, L0 + dif) ;
                    *SrcPtrQ  = (sPixel) iClip1 (max_imgpel_value, R0 - dif) ;
                  }

                  if (aq  && (C0 != 0))
                    *(SrcPtrQ + incQ) += iClip3 (-C0,  C0, ( R2 + RL0 - (R1 << 1)) >> 1 ) ;
                }
              }
            }
          }
          pixP.posX++;
          pixQ.posX++;
        }
      }
    }
  }
  //}}}
  //{{{
  void edge_loop_chroma_ver_MBAff (sPixel** img, uint8_t* Strength,
                                   sMacroBlock* mbQ, int edge, int uv, sPicture* p) {

    int      pel, Strng ;
    sPixel   L1, L0, R0, R1;
    int      Alpha = 0, Beta = 0;
    const uint8_t* ClipTab = NULL;
    int      indexA, indexB;
    cDecoder264* decoder = mbQ->decoder;
    int      PelNum = pelnum_cr[0][p->chromaFormatIdc];
    int      StrengthIdx;
    int      QP;
    int      xQ = edge, yQ;
    sPixelPos pixP, pixQ;
    int      bitDepthScale = decoder->coding.bitDepthScale[eChroma];
    int      max_imgpel_value = decoder->coding.maxPelValueComp[uv + 1];

    int      AlphaC0Offset = mbQ->deblockFilterC0Offset;
    int      BetaOffset    = mbQ->deblockFilterBetaOffset;
    sMacroBlock *MbP;
    sPixel   *SrcPtrP, *SrcPtrQ;

    for (pel = 0 ; pel < PelNum ; ++pel ) {
      yQ = pel;
      getAffNeighbour(mbQ, xQ, yQ, decoder->mbSize[eChroma], &pixQ);
      getAffNeighbour(mbQ, xQ - 1, yQ, decoder->mbSize[eChroma], &pixP);
      MbP = &(decoder->mbData[pixP.mbIndex]);
      StrengthIdx = (PelNum == 8) ? ((mbQ->mbField && !MbP->mbField) ? pel << 1 :((pel >> 1) << 2) + (pel & 0x01)) : pel;

      if (pixP.ok || (mbQ->deblockFilterDisableIdc == 0)) {
        if ((Strng = Strength[StrengthIdx]) != 0) {
          SrcPtrQ = &(img[pixQ.posY][pixQ.posX]);
          SrcPtrP = &(img[pixP.posY][pixP.posX]);

          // Average QP of the two blocks
          QP = (MbP->qpc[uv] + mbQ->qpc[uv] + 1) >> 1;
          indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
          indexB = iClip3(0, MAX_QP, QP + BetaOffset);
          Alpha   = ALPHA_TABLE[indexA] * bitDepthScale;
          Beta    = BETA_TABLE [indexB] * bitDepthScale;
          ClipTab = CLIP_TAB[indexA];
          L0  = *SrcPtrP;
          R0  = *SrcPtrQ;
          if (iabs (R0 - L0) < Alpha ) {
            L1 = SrcPtrP[-1];
            R1 = SrcPtrQ[ 1];
            if (((iabs( R0 - R1) - Beta < 0)  && (iabs(L0 - L1) - Beta < 0 ))  ) {
              if (Strng == 4 ) {
                // INTRA strong filtering
                SrcPtrQ[0] = (sPixel) ( ((R1 << 1) + R0 + L1 + 2) >> 2 );
                SrcPtrP[0] = (sPixel) ( ((L1 << 1) + L0 + R1 + 2) >> 2 );
                }
              else {
                int C0  = ClipTab[ Strng ] * bitDepthScale;
                int tc0  = (C0 + 1);
                int dif = iClip3 (-tc0, tc0, ( ((R0 - L0) << 2) + (L1 - R1) + 4) >> 3 );
                if (dif) {
                  *SrcPtrP = (sPixel) iClip1 (max_imgpel_value, L0 + dif );
                  *SrcPtrQ = (sPixel) iClip1 (max_imgpel_value, R0 - dif );
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
  void edge_loop_chroma_hor_MBAff (sPixel** img, uint8_t* Strength,
                                   sMacroBlock* mbQ, int edge, int uv, sPicture* p)
  {
    cDecoder264* decoder = mbQ->decoder;
    int PelNum = pelnum_cr[1][p->chromaFormatIdc];
    int yQ = (edge < MB_BLOCK_SIZE? edge : 1);
    sPixelPos pixP, pixQ;
    int bitDepthScale = decoder->coding.bitDepthScale[eChroma];
    int max_imgpel_value = decoder->coding.maxPelValueComp[uv + 1];

    int AlphaC0Offset = mbQ->deblockFilterC0Offset;
    int BetaOffset    = mbQ->deblockFilterBetaOffset;
    int width = p->chromaStride; //p->sizeXcr;

    getAffNeighbour (mbQ, 0, yQ - 1, decoder->mbSize[eChroma], &pixP);
    getAffNeighbour (mbQ, 0, yQ, decoder->mbSize[eChroma], &pixQ);

    if (pixP.ok || (mbQ->deblockFilterDisableIdc == 0)) {
      sMacroBlock* MbP = &(decoder->mbData[pixP.mbIndex]);
      int incQ = ((MbP->mbField && !mbQ->mbField) ? 2 * width : width);
      int incP = ((mbQ->mbField  && !MbP->mbField) ? 2 * width : width);

      // Average QP of the two blocks
      int QP = (MbP->qpc[uv] + mbQ->qpc[uv] + 1) >> 1;
      int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
      int indexB = iClip3(0, MAX_QP, QP + BetaOffset);
      int Alpha = ALPHA_TABLE[indexA] * bitDepthScale;
      int Beta = BETA_TABLE [indexB] * bitDepthScale;

      if ((Alpha | Beta )!= 0) {
        const uint8_t* ClipTab = CLIP_TAB[indexA];
        int pel, Strng ;
        int StrengthIdx;
        for (pel = 0 ; pel < PelNum ; ++pel ) {
          StrengthIdx = (PelNum == 8) ? ((mbQ->mbField && !MbP->mbField) ? pel << 1 :((pel >> 1) << 2) + (pel & 0x01)) : pel;

          if ((Strng = Strength[StrengthIdx]) != 0) {
            sPixel* SrcPtrQ = &(img[pixQ.posY][pixQ.posX]);
            sPixel* SrcPtrP = &(img[pixP.posY][pixP.posX]);
            sPixel L0 = *SrcPtrP;
            sPixel R0 = *SrcPtrQ;
            if (iabs (R0 - L0 ) < Alpha ) {
              sPixel L1 = SrcPtrP[-incP];
              sPixel R1 = SrcPtrQ[ incQ];
              if (((iabs (R0 - R1) - Beta < 0)  && (iabs(L0 - L1) - Beta < 0))) {
                if (Strng == 4)  {
                 // INTRA strong filtering
                  SrcPtrQ[0] = (sPixel) ( ((R1 << 1) + R0 + L1 + 2) >> 2 );
                  SrcPtrP[0] = (sPixel) ( ((L1 << 1) + L0 + R1 + 2) >> 2 );
                  }
                else {
                  int C0  = ClipTab[ Strng ] * bitDepthScale;
                  int tc0  = (C0 + 1);
                  int dif = iClip3 (-tc0, tc0, ( ((R0 - L0) << 2) + (L1 - R1) + 4) >> 3 );
                  if (dif) {
                    *SrcPtrP = (sPixel)iClip1 (max_imgpel_value, L0 + dif );
                    *SrcPtrQ = (sPixel)iClip1 (max_imgpel_value, R0 - dif );
                    }
                  }
                }
              }
            }
          pixP.posX++;
          pixQ.posX++;
          }
        }
      }
    }
  //}}}
  //{{{
  void get_strength_ver_MBAff (uint8_t* Strength, sMacroBlock* mb, int edge, int mvlimit, sPicture* p) {

    int16_t  blkP, blkQ, idx;
    int    StrValue, i;
    int16_t  mb_x, mb_y;
    sPixelPos pixP;
    cDecoder264* decoder = mb->decoder;
    sBlockPos *picPos = decoder->picPos;

    sMacroBlock* MbP;
    if ((p->sliceType == cSlice::eSP) || (p->sliceType == cSlice::eSI) ) {
      for (idx = 0; idx < MB_BLOCK_SIZE; ++idx ) {
        getAffNeighbour(mb, edge - 1, idx, decoder->mbSize[eLuma], &pixP);
        blkQ = (int16_t) ((idx & 0xFFFC) + (edge >> 2));
        blkP = (int16_t) ((pixP.y & 0xFFFC) + (pixP.x >> 2));
        MbP = &(decoder->mbData[pixP.mbIndex]);
        mb->mixedModeEdgeFlag = (uint8_t) (mb->mbField != MbP->mbField);
        Strength[idx] = (edge == 0) ? 4 : 3;
        }
      }
    else {
      getAffNeighbour (mb, edge - 1, 0, decoder->mbSize[eLuma], &pixP);

      MbP = &(decoder->mbData[pixP.mbIndex]);
      // Neighboring Frame MBs
      if ((mb->mbField == false && MbP->mbField == false)) {
        mb->mixedModeEdgeFlag = (uint8_t) (mb->mbField != MbP->mbField);
        if (mb->isIntraBlock == true || MbP->isIntraBlock == true) {
          //printf("idx %d %d %d %d %d\n", idx, pixP.x, pixP.y, pixP.posX, pixP.posY);
          // Start with Strength=3. or Strength=4 for Mb-edge
          StrValue = (edge == 0) ? 4 : 3;
          for (i = 0; i < MB_BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
          }
        else {
          getMbBlockPosMbaff (picPos, mb->mbIndexX, &mb_x, &mb_y);
          for (idx = 0; idx < MB_BLOCK_SIZE; idx += BLOCK_SIZE) {
            blkQ = (int16_t) ((idx & 0xFFFC) + (edge >> 2));
            blkP = (int16_t) ((pixP.y & 0xFFFC) + (pixP.x >> 2));

            if (((mb->cbp[0].blk & i64power2(blkQ)) != 0) || ((MbP->cbp[0].blk & i64power2(blkP)) != 0))
              StrValue = 2;
            else if (edge && ((mb->mbType == 1)  || (mb->mbType == 2)))
              StrValue = 0; // if internal edge of certain types, we already know StrValue should be 0
            else {
              // for everything else, if no coefs, but vector difference >= 1 set Strength=1
              int blk_y  = ((mb_y<<2) + (blkQ >> 2));
              int blk_x  = ((mb_x<<2) + (blkQ  & 3));
              int blk_y2 = (pixP.posY >> 2);
              int blk_x2 = (pixP.posX >> 2);

              sPicMotion *mv_info_p = &p->mvInfo[blk_y ][blk_x ];
              sPicMotion *mv_info_q = &p->mvInfo[blk_y2][blk_x2];
              sPicture* ref_p0 = mv_info_p->refPic[LIST_0];
              sPicture* ref_q0 = mv_info_q->refPic[LIST_0];
              sPicture* ref_p1 = mv_info_p->refPic[LIST_1];
              sPicture* ref_q1 = mv_info_q->refPic[LIST_1];

              if  (((ref_p0==ref_q0) && (ref_p1==ref_q1))||((ref_p0==ref_q1) && (ref_p1==ref_q0))) {
                // L0 and L1 reference pictures of p0 are different; q0 as well
                if (ref_p0 != ref_p1) {
                  // compare MV for the same reference picture
                  if (ref_p0==ref_q0) {
                    StrValue =  (uint8_t) (
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit));
                    }
                  else {
                    StrValue =  (uint8_t) (
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit));
                    }
                  }
                else {
                  // L0 and L1 reference pictures of p0 are the same; q0 as well
                  StrValue = (uint8_t) ((
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit))
                    &&(
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit)));
                  }
                }
              else
                StrValue = 1;
                }

            Strength[idx] = StrValue;
            Strength[idx + 1] = StrValue;
            Strength[idx + 2] = StrValue;
            Strength[idx + 3] = StrValue;
            pixP.y += 4;
            pixP.posY += 4;
            }
          }
        }
      else {
        for (idx = 0; idx < MB_BLOCK_SIZE; ++idx ) {
          getAffNeighbour(mb, edge - 1, idx, decoder->mbSize[eLuma], &pixP);
          blkQ = (int16_t) ((idx & 0xFFFC) + (edge >> 2));
          blkP = (int16_t) ((pixP.y & 0xFFFC) + (pixP.x >> 2));
          MbP = &(decoder->mbData[pixP.mbIndex]);
          mb->mixedModeEdgeFlag = (uint8_t) (mb->mbField != MbP->mbField);

          // Start with Strength=3. or Strength=4 for Mb-edge
          Strength[idx] = (edge == 0 && (((!p->mbAffFrame && (p->picStructure==eFrame)) ||
                           (p->mbAffFrame && !MbP->mbField && !mb->mbField)) ||
                           ((p->mbAffFrame || (p->picStructure!=eFrame))))) ? 4 : 3;

          if (mb->isIntraBlock == false && MbP->isIntraBlock == false) {
            if (((mb->cbp[0].blk & i64power2(blkQ)) != 0) || ((MbP->cbp[0].blk & i64power2(blkP)) != 0))
              Strength[idx] = 2 ;
            else {
              // if no coefs, but vector difference >= 1 set Strength=1
              // if this is a mixed mode edge then one set of reference pictures will be frame and the
              // other will be field
              if (mb->mixedModeEdgeFlag) //if (slice->mixedModeEdgeFlag)
                Strength[idx] = 1;
              else {
                getMbBlockPosMbaff (picPos, mb->mbIndexX, &mb_x, &mb_y);
                {
                  int blk_y  = ((mb_y<<2) + (blkQ >> 2));
                  int blk_x  = ((mb_x<<2) + (blkQ  & 3));
                  int blk_y2 = (pixP.posY >> 2);
                  int blk_x2 = (pixP.posX >> 2);

                  sPicMotion *mv_info_p = &p->mvInfo[blk_y ][blk_x ];
                  sPicMotion *mv_info_q = &p->mvInfo[blk_y2][blk_x2];
                  sPicture* ref_p0 = mv_info_p->refPic[LIST_0];
                  sPicture* ref_q0 = mv_info_q->refPic[LIST_0];
                  sPicture* ref_p1 = mv_info_p->refPic[LIST_1];
                  sPicture* ref_q1 = mv_info_q->refPic[LIST_1];

                  if (((ref_p0==ref_q0) && (ref_p1==ref_q1))||((ref_p0==ref_q1) && (ref_p1==ref_q0))) {
                    Strength[idx] = 0;
                    // L0 and L1 reference pictures of p0 are different; q0 as well
                    if (ref_p0 != ref_p1) {
                      // compare MV for the same reference picture
                      if (ref_p0 == ref_q0) {
                        Strength[idx] =  (uint8_t) (
                          compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                          compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit));
                        }
                      else {
                        Strength[idx] =  (uint8_t) (
                          compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                          compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit));
                        }
                      }
                    else { // L0 and L1 reference pictures of p0 are the same; q0 as well
                      Strength[idx] = (uint8_t) ((
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit))
                        &&(
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit)));
                      }
                    }
                  else
                    Strength[idx] = 1;
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
  void get_strength_hor_MBAff (uint8_t* Strength, sMacroBlock* mb, int edge, int mvlimit, sPicture* p) {

    int16_t  blkP, blkQ, idx;
    int16_t  blk_x, blk_x2, blk_y, blk_y2 ;
    int    StrValue, i;
    int    xQ, yQ = (edge < MB_BLOCK_SIZE ? edge : 1);
    int16_t  mb_x, mb_y;
    sMacroBlock *MbP;
    sPixelPos pixP;
    cDecoder264* decoder = mb->decoder;
    sBlockPos *picPos = decoder->picPos;

    if ((p->sliceType == cSlice::eSP) || (p->sliceType == cSlice::eSI) ) {
      for (idx = 0; idx < MB_BLOCK_SIZE; idx += BLOCK_SIZE) {
        xQ = idx;
        getAffNeighbour(mb, xQ, yQ - 1, decoder->mbSize[eLuma], &pixP);

        blkQ = (int16_t) ((yQ & 0xFFFC) + (xQ >> 2));
        blkP = (int16_t) ((pixP.y & 0xFFFC) + (pixP.x >> 2));

        MbP = &(decoder->mbData[pixP.mbIndex]);
        mb->mixedModeEdgeFlag = (uint8_t) (mb->mbField != MbP->mbField);

        StrValue = (edge == 0 && (!MbP->mbField && !mb->mbField)) ? 4 : 3;

        Strength[idx] = StrValue;
        Strength[idx+1] = StrValue;
        Strength[idx+2] = StrValue;
        Strength[idx+3] = StrValue;
        }
      }
    else {
      getAffNeighbour(mb, 0, yQ - 1, decoder->mbSize[eLuma], &pixP);
      MbP = &(decoder->mbData[pixP.mbIndex]);
      mb->mixedModeEdgeFlag = (uint8_t) (mb->mbField != MbP->mbField);

      // Set intra mode deblocking
      if (mb->isIntraBlock == true || MbP->isIntraBlock == true) {
        StrValue = (edge == 0 && (!MbP->mbField && !mb->mbField)) ? 4 : 3;
        for (i = 0; i < MB_BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
        }
      else {
        for (idx = 0; idx < MB_BLOCK_SIZE; idx += BLOCK_SIZE ) {
          xQ = idx;
          getAffNeighbour(mb, xQ, yQ - 1, decoder->mbSize[eLuma], &pixP);
          blkQ = (int16_t) ((yQ & 0xFFFC) + (xQ >> 2));
          blkP = (int16_t) ((pixP.y & 0xFFFC) + (pixP.x >> 2));
          if (((mb->cbp[0].blk & i64power2(blkQ)) != 0) || ((MbP->cbp[0].blk & i64power2(blkP)) != 0))
            StrValue = 2;
          else {
            // if no coefs, but vector difference >= 1 set Strength=1
            // if this is a mixed mode edge then one set of reference pictures will be frame and the
            // other will be field
            if(mb->mixedModeEdgeFlag) //if (slice->mixedModeEdgeFlag)
              StrValue = 1;
            else {
              getMbBlockPosMbaff (picPos, mb->mbIndexX, &mb_x, &mb_y);
              blk_y  = (int16_t) ((mb_y<<2) + (blkQ >> 2));
              blk_x  = (int16_t) ((mb_x<<2) + (blkQ  & 3));
              blk_y2 = (int16_t) (pixP.posY >> 2);
              blk_x2 = (int16_t) (pixP.posX >> 2);

              {
                sPicMotion *mv_info_p = &p->mvInfo[blk_y ][blk_x ];
                sPicMotion *mv_info_q = &p->mvInfo[blk_y2][blk_x2];
                sPicture* ref_p0 = mv_info_p->refPic[LIST_0];
                sPicture* ref_q0 = mv_info_q->refPic[LIST_0];
                sPicture* ref_p1 = mv_info_p->refPic[LIST_1];
                sPicture* ref_q1 = mv_info_q->refPic[LIST_1];

                if (((ref_p0==ref_q0) && (ref_p1==ref_q1)) ||
                  ((ref_p0==ref_q1) && (ref_p1==ref_q0))) {
                  StrValue = 0;
                  // L0 and L1 reference pictures of p0 are different; q0 as well
                  if (ref_p0 != ref_p1) {
                    // compare MV for the same reference picture
                    if (ref_p0==ref_q0) {
                      StrValue =  (uint8_t) (
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit));
                      }
                    else {
                      StrValue =  (uint8_t) (
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit));
                      }
                    }
                  else {
                    // L0 and L1 reference pictures of p0 are the same; q0 as well
                    StrValue = (uint8_t) ((
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit))
                      &&(
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit)));
                    }
                  }
                else
                  StrValue = 1;
                }
              }
            }
          Strength[idx] = StrValue;
          Strength[idx + 1] = StrValue;
          Strength[idx + 2] = StrValue;
          Strength[idx + 3] = StrValue;
          }
        }
      }
    }
  //}}}
  //{{{
  void set_loop_filter_functions_mbaff (cDecoder264* decoder) {

    decoder->edgeLoopLumaV = edge_loop_luma_ver_MBAff;
    decoder->edgeLoopLumaH = edge_loop_luma_hor_MBAff;
    decoder->edgeLoopChromaV = edge_loop_chroma_ver_MBAff;
    decoder->edgeLoopChromaH = edge_loop_chroma_hor_MBAff;
    }
  //}}}

  // normal
  //{{{
  void get_strength_ver (sMacroBlock* mb, int edge, int mvLimit, sPicture* p)
  {
    uint8_t *Strength = mb->strengthV[edge];
    cSlice* slice = mb->slice;
    int     StrValue, i;
    sBlockPos *picPos = mb->decoder->picPos;

    if ((slice->sliceType == cSlice::eSP) || (slice->sliceType == cSlice::eSI) ) {
      // Set strength to either 3 or 4 regardless of pixel position
      StrValue = (edge == 0) ? 4 : 3;
      for (i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
    }
    else {
      if (mb->isIntraBlock == false) {
        sMacroBlock *MbP;
        int xQ = (edge << 2) - 1;
        sMacroBlock *neighbour = get_non_aff_neighbour_luma (mb, xQ, 0);
        MbP = (edge) ? mb : neighbour;

        if (edge || MbP->isIntraBlock == false) {
          if (edge && (slice->sliceType == cSlice::eP && mb->mbType == PSKIP))
            for (i = 0; i < BLOCK_SIZE; i ++ )
              Strength[i] = 0;
          else  if (edge && ((mb->mbType == P16x16)  || (mb->mbType == P16x8))) {
            int      blkP, blkQ, idx;
            for (idx = 0 ; idx < MB_BLOCK_SIZE ; idx += BLOCK_SIZE ) {
              blkQ = idx + (edge);
              blkP = idx + (get_x_luma(xQ) >> 2);
              if ((mb->cbp[0].blk & (i64power2(blkQ) | i64power2(blkP))) != 0)
                StrValue = 2;
              else
                StrValue = 0; // if internal edge of certain types, then we already know StrValue should be 0

              Strength[idx >> 2] = StrValue;
            }
          }
          else {
            int blkP, blkQ, idx;
            sBlockPos blockPos = picPos[ mb->mbIndexX ];
            blockPos.x <<= BLOCK_SHIFT;
            blockPos.y <<= BLOCK_SHIFT;

            for (idx = 0 ; idx < MB_BLOCK_SIZE ; idx += BLOCK_SIZE ) {
              blkQ = idx  + (edge);
              blkP = idx  + (get_x_luma(xQ) >> 2);
              if (((mb->cbp[0].blk & i64power2(blkQ)) != 0) || ((MbP->cbp[0].blk & i64power2(blkP)) != 0))
                StrValue = 2;
              else {
                // for everything else, if no coefs, but vector difference >= 1 set Strength=1
                int blk_y  = blockPos.y + (blkQ >> 2);
                int blk_x  = blockPos.x + (blkQ  & 3);
                int blk_y2 = (int16_t)(get_pos_y_luma(neighbour,  0) + idx) >> 2;
                int blk_x2 = (int16_t)(get_pos_x_luma(neighbour, xQ)      ) >> 2;
                sPicMotion *mv_info_p = &p->mvInfo[blk_y ][blk_x ];
                sPicMotion *mv_info_q = &p->mvInfo[blk_y2][blk_x2];
                sPicture* ref_p0 = mv_info_p->refPic[LIST_0];
                sPicture* ref_q0 = mv_info_q->refPic[LIST_0];
                sPicture* ref_p1 = mv_info_p->refPic[LIST_1];
                sPicture* ref_q1 = mv_info_q->refPic[LIST_1];

                if  (((ref_p0==ref_q0) && (ref_p1==ref_q1)) || ((ref_p0==ref_q1) && (ref_p1==ref_q0))) {
                  // L0 and L1 reference pictures of p0 are different; q0 as well
                  if (ref_p0 != ref_p1) {
                    // compare MV for the same reference picture
                    if (ref_p0 == ref_q0) {
                      StrValue =
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvLimit) |
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvLimit);
                    }
                    else {
                      StrValue =
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvLimit) |
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvLimit);
                    }
                  }
                  else {
                    // L0 and L1 reference pictures of p0 are the same; q0 as well
                    StrValue = ((
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvLimit) |
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvLimit))
                      && (
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvLimit) |
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvLimit)
                      ));
                  }
                }
                else
                  StrValue = 1;
              }
              //*(int*)(Strength+(idx >> 2)) = StrValue; // * 0x01010101;
              Strength[idx >> 2] = StrValue;
            }
          }
        }
        else {
          // Start with Strength=3. or Strength=4 for Mb-edge
          StrValue = (edge == 0) ? 4 : 3;
          for (i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
        }
      }
      else {
        // Start with Strength=3. or Strength=4 for Mb-edge
        StrValue = (edge == 0) ? 4 : 3;
        for (i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
      }
    }
  }
  //}}}
  //{{{
  void get_strength_hor (sMacroBlock* mb, int edge, int mvLimit, sPicture* p) {

    uint8_t  *Strength = mb->strengthH[edge];
    int    StrValue, i;
    cSlice* slice = mb->slice;
    sBlockPos *picPos = mb->decoder->picPos;

    if ((slice->sliceType == cSlice::eSP) || (slice->sliceType == cSlice::eSI) ) {
      // Set strength to either 3 or 4 regardless of pixel position
      StrValue = (edge == 0 && (((p->picStructure==eFrame)))) ? 4 : 3;
      for (i = 0; i < BLOCK_SIZE; i++)
        Strength[i] = StrValue;
      }
    else {
      if (mb->isIntraBlock == false) {
        sMacroBlock* MbP;
        int yQ = (edge < BLOCK_SIZE ? (edge << 2) - 1: 0);
        sMacroBlock* neighbor = get_non_aff_neighbour_luma(mb, 0, yQ);
        MbP = (edge) ? mb : neighbor;
        if (edge || MbP->isIntraBlock == false) {
          if (edge && (slice->sliceType == cSlice::eP && mb->mbType == PSKIP))
            for (i = 0; i < BLOCK_SIZE; i ++)
              Strength[i] = 0;
          else if (edge && ((mb->mbType == P16x16)  || (mb->mbType == P8x16))) {
            int blkP, blkQ, idx;
            for (idx = 0 ; idx < BLOCK_SIZE ; idx ++ ) {
              blkQ = (yQ + 1) + idx;
              blkP = (get_y_luma(yQ) & 0xFFFC) + idx;
              if ((mb->cbp[0].blk & (i64power2(blkQ) | i64power2(blkP))) != 0)
                StrValue = 2;
              else
                StrValue = 0; // if internal edge of certain types, we already know StrValue should be 0
              Strength[idx] = StrValue;
              }
            }
          else {
            int      blkP, blkQ, idx;
            sBlockPos blockPos = picPos[ mb->mbIndexX ];
            blockPos.x <<= 2;
            blockPos.y <<= 2;

            for (idx = 0 ; idx < BLOCK_SIZE ; idx ++) {
              blkQ = (yQ + 1) + idx;
              blkP = (get_y_luma(yQ) & 0xFFFC) + idx;
              if (((mb->cbp[0].blk & i64power2(blkQ)) != 0) || ((MbP->cbp[0].blk & i64power2(blkP)) != 0))
                StrValue = 2;
              else {
                // for everything else, if no coefs, but vector difference >= 1 set Strength=1
                int blk_y = blockPos.y + (blkQ >> 2);
                int blk_x = blockPos.x + (blkQ  & 3);
                int blk_y2 = get_pos_y_luma(neighbor,yQ) >> 2;
                int blk_x2 = ((int16_t)(get_pos_x_luma(neighbor,0)) >> 2) + idx;

                sPicMotion* mv_info_p = &p->mvInfo[blk_y ][blk_x ];
                sPicMotion* mv_info_q = &p->mvInfo[blk_y2][blk_x2];

                sPicture* ref_p0 = mv_info_p->refPic[LIST_0];
                sPicture* ref_q0 = mv_info_q->refPic[LIST_0];
                sPicture* ref_p1 = mv_info_p->refPic[LIST_1];
                sPicture* ref_q1 = mv_info_q->refPic[LIST_1];

                if  (((ref_p0==ref_q0) && (ref_p1==ref_q1)) || ((ref_p0==ref_q1) && (ref_p1==ref_q0))) {
                  // L0 and L1 reference pictures of p0 are different; q0 as well
                  if (ref_p0 != ref_p1) {
                    // compare MV for the same reference picture
                    if (ref_p0 == ref_q0)
                      StrValue = compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvLimit) |
                                 compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvLimit);
                    else
                      StrValue = compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvLimit) |
                                 compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvLimit);
                    }
                  else
                    // L0 and L1 reference pictures of p0 are the same; q0 as well
                    StrValue = ((compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvLimit) |
                                 compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvLimit))
                                 && (
                                 compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvLimit) |
                                 compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvLimit)
                                 ));
                  }
                else
                  StrValue = 1;
                }
              Strength[idx] = StrValue;
              }
            }
          }
        else {
          // Start with Strength=3. or Strength=4 for Mb-edge
          StrValue = (edge == 0 && (p->picStructure == eFrame)) ? 4 : 3;
          for (i = 0; i < BLOCK_SIZE; i ++ )
            Strength[i] = StrValue;
          }
        }
      else {
        // Start with Strength=3. or Strength=4 for Mb-edge
        StrValue = (edge == 0 && (p->picStructure == eFrame)) ? 4 : 3;
        for (i = 0; i < BLOCK_SIZE; i ++ )
          Strength[i] = StrValue;
         }
      }
    }
  //}}}
  //{{{
  void luma_ver_deblock_strong (sPixel** pixel, int pos_x1, int Alpha, int Beta) {

    int i;
    for (i = 0 ; i < BLOCK_SIZE ; ++i )
    {
      sPixel *SrcPtrP = *(pixel++) + pos_x1;
      sPixel *SrcPtrQ = SrcPtrP + 1;
      sPixel  L0 = *SrcPtrP;
      sPixel  R0 = *SrcPtrQ;

      if (iabs (R0 - L0 ) < Alpha ) {
        sPixel  R1 = *(SrcPtrQ + 1);
        sPixel  L1 = *(SrcPtrP - 1);
        if ((iabs (R0 - R1) < Beta)  && (iabs(L0 - L1) < Beta)) {
          if ((iabs (R0 - L0 ) < ((Alpha >> 2) + 2))) {
            sPixel  R2 = *(SrcPtrQ + 2);
            sPixel  L2 = *(SrcPtrP - 2);
            int RL0 = L0 + R0;

            if ((iabs( L0 - L2) < Beta )) {
              sPixel  L3 = *(SrcPtrP - 3);
              *(SrcPtrP--) = (sPixel)  (( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3);
              *(SrcPtrP--) = (sPixel)  (( L2 + L1 + RL0 + 2) >> 2);
              *(SrcPtrP  ) = (sPixel) ((((L3 + L2) <<1) + L2 + L1 + RL0 + 4) >> 3);
            }
            else
              *SrcPtrP = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2);

            if ((iabs( R0 - R2) < Beta )) {
              sPixel  R3 = *(SrcPtrQ + 3);
              *(SrcPtrQ++) = (sPixel) (( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3);
              *(SrcPtrQ++) = (sPixel) (( R2 + R0 + L0 + R1 + 2) >> 2);
              *(SrcPtrQ  ) = (sPixel) ((((R3 + R2) <<1) + R2 + R1 + RL0 + 4) >> 3);
            }
            else
              *SrcPtrQ = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
          }
          else {
            *SrcPtrP = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2);
            *SrcPtrQ = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
          }
        }
      }
    }
  }
  //}}}
  //{{{
  void luma_ver_deblock_normal (sPixel** pixel, int pos_x1,
                                int Alpha, int Beta, int C0, int max_imgpel_value) {

    int i;
    sPixel *SrcPtrP, *SrcPtrQ;
    int edge_diff;

    if (C0 == 0) {
      for (i= 0 ; i < BLOCK_SIZE ; ++i ) {
        SrcPtrP = *(pixel++) + pos_x1;
        SrcPtrQ = SrcPtrP + 1;
        edge_diff = *SrcPtrQ - *SrcPtrP;

        if (iabs (edge_diff ) < Alpha ) {
          sPixel  *SrcPtrQ1 = SrcPtrQ + 1;
          sPixel  *SrcPtrP1 = SrcPtrP - 1;

          if ((iabs (*SrcPtrQ - *SrcPtrQ1) < Beta)  && (iabs(*SrcPtrP - *SrcPtrP1) < Beta)) {
            sPixel  R2 = *(SrcPtrQ1 + 1);
            sPixel  L2 = *(SrcPtrP1 - 1);

            int aq  = (iabs(*SrcPtrQ - R2) < Beta);
            int ap  = (iabs(*SrcPtrP - L2) < Beta);

            int tc0  = (ap + aq) ;
            int dif = iClip3 (-tc0, tc0, (((edge_diff) << 2) + (*SrcPtrP1 - *SrcPtrQ1) + 4) >> 3 );

            if (dif != 0) {
              *SrcPtrP = (sPixel) iClip1(max_imgpel_value, *SrcPtrP + dif);
              *SrcPtrQ = (sPixel) iClip1(max_imgpel_value, *SrcPtrQ - dif);
              }
            }
          }
        }
      }
    else {
      for (i= 0 ; i < BLOCK_SIZE ; ++i ) {
        SrcPtrP = *(pixel++) + pos_x1;
        SrcPtrQ = SrcPtrP + 1;
        edge_diff = *SrcPtrQ - *SrcPtrP;
        if (iabs (edge_diff ) < Alpha ) {
          sPixel  *SrcPtrQ1 = SrcPtrQ + 1;
          sPixel  *SrcPtrP1 = SrcPtrP - 1;
          if ((iabs (*SrcPtrQ - *SrcPtrQ1) < Beta)  && (iabs(*SrcPtrP - *SrcPtrP1) < Beta)) {
            int RL0 = (*SrcPtrP + *SrcPtrQ + 1) >> 1;
            sPixel  R2 = *(SrcPtrQ1 + 1);
            sPixel  L2 = *(SrcPtrP1 - 1);
            int aq  = (iabs(*SrcPtrQ - R2) < Beta);
            int ap  = (iabs(*SrcPtrP - L2) < Beta);
            int tc0  = (C0 + ap + aq) ;
            int dif = iClip3 (-tc0, tc0, (((edge_diff) << 2) + (*SrcPtrP1 - *SrcPtrQ1) + 4) >> 3 );
            if (ap )
              *SrcPtrP1 = (sPixel) (*SrcPtrP1 + iClip3 (-C0,  C0, (L2 + RL0 - (*SrcPtrP1<<1)) >> 1 ));
            if (dif != 0) {
              *SrcPtrP = (sPixel) iClip1(max_imgpel_value, *SrcPtrP + dif);
              *SrcPtrQ = (sPixel) iClip1(max_imgpel_value, *SrcPtrQ - dif);
              }

            if (aq)
              *SrcPtrQ1 = (sPixel) (*SrcPtrQ1 + iClip3 (-C0,  C0, (R2 + RL0 - (*SrcPtrQ1<<1)) >> 1 ));
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void edge_loop_luma_ver (eColorPlane plane, sPixel** img, uint8_t* Strength, sMacroBlock* mb, int edge) {

    cDecoder264* decoder = mb->decoder;

    sMacroBlock* MbP = get_non_aff_neighbour_luma (mb, edge - 1, 0);
    if (MbP || (mb->deblockFilterDisableIdc == 0)) {
      int bitDepthScale   = plane ? decoder->coding.bitDepthScale[eChroma] : decoder->coding.bitDepthScale[eLuma];

      // Average QP of the two blocks
      int QP = plane? ((MbP->qpc[plane-1] + mb->qpc[plane-1] + 1) >> 1) : (MbP->qp + mb->qp + 1) >> 1;
      int indexA = iClip3 (0, MAX_QP, QP + mb->deblockFilterC0Offset);
      int indexB = iClip3 (0, MAX_QP, QP + mb->deblockFilterBetaOffset);

      int Alpha = ALPHA_TABLE[indexA] * bitDepthScale;
      int Beta = BETA_TABLE [indexB] * bitDepthScale;
      if ((Alpha | Beta )!= 0) {
        const uint8_t *ClipTab = CLIP_TAB[indexA];
        int max_imgpel_value = decoder->coding.maxPelValueComp[plane];
        int pos_x1 = get_pos_x_luma(MbP, (edge - 1));
        sPixel** pixel = &img[get_pos_y_luma(MbP, 0)];
        for (int pel = 0 ; pel < MB_BLOCK_SIZE ; pel += 4 ) {
          if (*Strength == 4) // INTRA strong filtering
            luma_ver_deblock_strong (pixel, pos_x1, Alpha, Beta);
          else if (*Strength != 0) // normal filtering
            luma_ver_deblock_normal (pixel, pos_x1, Alpha, Beta, ClipTab[ *Strength ] * bitDepthScale, max_imgpel_value);
          pixel += 4;
          Strength ++;
          }
        }
      }
    }
  //}}}
  //{{{
  void luma_hor_deblock_strong (sPixel* imgP, sPixel* imgQ, int width, int Alpha, int Beta) {

    int inc_dim2 = width * 2;
    int inc_dim3 = width * 3;
    for (int pixel = 0 ; pixel < BLOCK_SIZE; ++pixel) {
      sPixel* SrcPtrP = imgP++;
      sPixel* SrcPtrQ = imgQ++;
      sPixel L0 = *SrcPtrP;
      sPixel R0 = *SrcPtrQ;
      if (iabs (R0 - L0 ) < Alpha ) {
        sPixel L1 = *(SrcPtrP - width);
        sPixel R1 = *(SrcPtrQ + width);

        if ((iabs (R0 - R1) < Beta) && (iabs(L0 - L1) < Beta)) {
          if ((iabs (R0 - L0 ) < ((Alpha >> 2) + 2))) {
            sPixel L2 = *(SrcPtrP - inc_dim2);
            sPixel R2 = *(SrcPtrQ + inc_dim2);
            int RL0 = L0 + R0;

            if ((iabs (L0 - L2) < Beta)) {
              sPixel L3 = *(SrcPtrP - inc_dim3);
              *(SrcPtrP         ) = (sPixel)(( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3);
              *(SrcPtrP -= width) = (sPixel)(( L2 + L1 + RL0 + 2) >> 2);
              *(SrcPtrP -  width) = (sPixel)((((L3 + L2) <<1) + L2 + L1 + RL0 + 4) >> 3);
            }
            else
              *SrcPtrP = (sPixel)(((L1 << 1) + L0 + R1 + 2) >> 2);

            if ( (iabs (R0 - R2) < Beta )) {
              sPixel  R3 = *(SrcPtrQ + inc_dim3);
              *(SrcPtrQ          ) = (sPixel)(( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3);
              *(SrcPtrQ += width ) = (sPixel)(( R2 + R0 + L0 + R1 + 2) >> 2);
              *(SrcPtrQ +  width ) = (sPixel)((((R3 + R2) <<1) + R2 + R1 + RL0 + 4) >> 3);
              }
            else
              *SrcPtrQ = (sPixel)(((R1 << 1) + R0 + L1 + 2) >> 2);
            }
          else {
            *SrcPtrP = (sPixel)(((L1 << 1) + L0 + R1 + 2) >> 2);
            *SrcPtrQ = (sPixel)(((R1 << 1) + R0 + L1 + 2) >> 2);
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void luma_hor_deblock_normal (sPixel* imgP, sPixel* imgQ, int width,
                                int Alpha, int Beta, int C0, int max_imgpel_value) {

    if (C0 == 0)
      for (int i = 0 ; i < BLOCK_SIZE ; ++i) {
        int edge_diff = *imgQ - *imgP;
        if (iabs( edge_diff ) < Alpha ) {
          sPixel* SrcPtrQ1 = imgQ + width;
          sPixel* SrcPtrP1 = imgP - width;
          if ((iabs (*imgQ - *SrcPtrQ1) < Beta)  && (iabs(*imgP - *SrcPtrP1) < Beta)) {
            sPixel R2 = *(SrcPtrQ1 + width);
            sPixel L2 = *(SrcPtrP1 - width);
            int aq = (iabs(*imgQ - R2) < Beta);
            int ap = (iabs(*imgP - L2) < Beta);
            int tc0 = ap + aq;
            int dif = iClip3 (-tc0, tc0, (((edge_diff) << 2) + (*SrcPtrP1 - *SrcPtrQ1) + 4) >> 3 );
            if (dif != 0) {
              *imgP = (sPixel)iClip1(max_imgpel_value, *imgP + dif);
              *imgQ = (sPixel)iClip1(max_imgpel_value, *imgQ - dif);
              }
            }
          }
        imgP++;
        imgQ++;
        }

    else
      for (int i= 0 ; i < BLOCK_SIZE ; ++i) {
        int edge_diff = *imgQ - *imgP;
        if (iabs (edge_diff ) < Alpha ) {
          sPixel* SrcPtrQ1 = imgQ + width;
          sPixel* SrcPtrP1 = imgP - width;
          if ((iabs (*imgQ - *SrcPtrQ1) < Beta)  && (iabs(*imgP - *SrcPtrP1) < Beta)) {
            int RL0 = (*imgP + *imgQ + 1) >> 1;
            sPixel R2 = *(SrcPtrQ1 + width);
            sPixel L2 = *(SrcPtrP1 - width);
            int aq = iabs (*imgQ - R2) < Beta;
            int ap = iabs (*imgP - L2) < Beta;
            int tc0 = C0 + ap + aq;
            int dif = iClip3 (-tc0, tc0, (((edge_diff) << 2) + (*SrcPtrP1 - *SrcPtrQ1) + 4) >> 3 );
            if (ap)
              *SrcPtrP1 = (sPixel) (*SrcPtrP1 + iClip3 (-C0,  C0, (L2 + RL0 - (*SrcPtrP1<<1)) >> 1 ));

            if (dif != 0) {
              *imgP = (sPixel)iClip1 (max_imgpel_value, *imgP + dif);
              *imgQ = (sPixel)iClip1 (max_imgpel_value, *imgQ - dif);
              }

            if (aq )
              *SrcPtrQ1 = (sPixel)(*SrcPtrQ1 + iClip3 (-C0,  C0, (R2 + RL0 - (*SrcPtrQ1<<1)) >> 1 ));
            }
          }
        imgP++;
        imgQ++;
        }
    }
  //}}}
  //{{{
  void edge_loop_luma_hor (eColorPlane plane, sPixel** img, uint8_t* Strength,
                           sMacroBlock* mb, int edge, sPicture *p) {

    cDecoder264* decoder = mb->decoder;

    int ypos = (edge < MB_BLOCK_SIZE ? edge - 1: 0);
    sMacroBlock *MbP = get_non_aff_neighbour_luma (mb, 0, ypos);

    if (MbP || (mb->deblockFilterDisableIdc == 0)) {
      int bitDepthScale = plane ? decoder->coding.bitDepthScale[eChroma] : decoder->coding.bitDepthScale[eLuma];

      // Average QP of the two blocks
      int QP = plane? ((MbP->qpc[plane-1] + mb->qpc[plane-1] + 1) >> 1) : (MbP->qp + mb->qp + 1) >> 1;

      int indexA = iClip3 (0, MAX_QP, QP + mb->deblockFilterC0Offset);
      int indexB = iClip3 (0, MAX_QP, QP + mb->deblockFilterBetaOffset);

      int Alpha = ALPHA_TABLE[indexA] * bitDepthScale;
      int Beta = BETA_TABLE [indexB] * bitDepthScale;

      if ((Alpha | Beta) != 0) {
        const uint8_t* ClipTab = CLIP_TAB[indexA];
        int max_imgpel_value = decoder->coding.maxPelValueComp[plane];
        int width = p->lumaStride;

        sPixel* imgP = &img[get_pos_y_luma(MbP, ypos)][get_pos_x_luma(MbP, 0)];
        sPixel* imgQ = imgP + width;
        for (int pel = 0 ; pel < BLOCK_SIZE ; pel++ ) {
          if (*Strength == 4)  // INTRA strong filtering
            luma_hor_deblock_strong (imgP, imgQ, width, Alpha, Beta);
          else if (*Strength != 0) // normal filtering
            luma_hor_deblock_normal (imgP, imgQ, width, Alpha, Beta, ClipTab[ *Strength ] * bitDepthScale, max_imgpel_value);
          imgP += 4;
          imgQ += 4;
          Strength ++;
        }
      }
    }
  }
  //}}}
  //{{{
  void edge_loop_chroma_ver (sPixel** img, uint8_t* Strength, sMacroBlock* mb, int edge, int uv, sPicture *p) {

    cDecoder264* decoder = mb->decoder;

    int block_width  = decoder->mbCrSizeX;
    int block_height = decoder->mbCrSizeY;
    int xQ = edge - 1;
    int yQ = 0;

    sMacroBlock* MbP = get_non_aff_neighbour_chroma (mb,xQ,yQ,block_width,block_height);

    if (MbP || (mb->deblockFilterDisableIdc == 0)) {
      int bitDepthScale   = decoder->coding.bitDepthScale[eChroma];
      int max_imgpel_value = decoder->coding.maxPelValueComp[uv + 1];
      int AlphaC0Offset = mb->deblockFilterC0Offset;
      int BetaOffset = mb->deblockFilterBetaOffset;

      // Average QP of the two blocks
      int QP = (MbP->qpc[uv] + mb->qpc[uv] + 1) >> 1;
      int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
      int indexB = iClip3(0, MAX_QP, QP + BetaOffset);
      int Alpha   = ALPHA_TABLE[indexA] * bitDepthScale;
      int Beta    = BETA_TABLE [indexB] * bitDepthScale;
      if ((Alpha | Beta) != 0) {
        const int PelNum = pelnum_cr[0][p->chromaFormatIdc];
        const uint8_t* ClipTab = CLIP_TAB[indexA];
        int pos_x1 = get_pos_x_chroma(MbP, xQ, (block_width - 1));
        sPixel** pixel = &img[get_pos_y_chroma(MbP,yQ, (block_height - 1))];
        for (int pel = 0 ; pel < PelNum ; ++pel ) {
          int Strng = Strength[(PelNum == 8) ? (pel >> 1) : (pel >> 2)];
          if (Strng != 0) {
            sPixel* SrcPtrP = *pixel + pos_x1;
            sPixel* SrcPtrQ = SrcPtrP + 1;
            int edge_diff = *SrcPtrQ - *SrcPtrP;
            if (iabs (edge_diff) < Alpha) {
              sPixel R1  = *(SrcPtrQ + 1);
              if (iabs(*SrcPtrQ - R1) < Beta ) {
                sPixel L1  = *(SrcPtrP - 1);
                if (iabs(*SrcPtrP - L1) < Beta ) {
                  if (Strng == 4 )  {
                    // INTRA strong filtering
                    *SrcPtrP = (sPixel) ( ((L1 << 1) + *SrcPtrP + R1 + 2) >> 2 );
                    *SrcPtrQ = (sPixel) ( ((R1 << 1) + *SrcPtrQ + L1 + 2) >> 2 );
                    }
                  else {
                    int tc0  = ClipTab[ Strng ] * bitDepthScale + 1;
                    int dif = iClip3 (-tc0, tc0, (((edge_diff) << 2) + (L1 - R1) + 4) >> 3 );
                    if (dif != 0) {
                      *SrcPtrP = (sPixel) iClip1 (max_imgpel_value, *SrcPtrP + dif );
                      *SrcPtrQ = (sPixel) iClip1 (max_imgpel_value, *SrcPtrQ - dif );
                      }
                    }
                  }
                }
              }
            }
          pixel++;
          }
        }
      }
    }
  //}}}
  //{{{
  void edge_loop_chroma_hor (sPixel** img, uint8_t* Strength, sMacroBlock* mb,
                             int edge, int uv, sPicture *p) {

    cDecoder264* decoder = mb->decoder;
    int block_width = decoder->mbCrSizeX;
    int block_height = decoder->mbCrSizeY;
    int xQ = 0;
    int yQ = (edge < 16 ? edge - 1: 0);

    sMacroBlock* MbP = get_non_aff_neighbour_chroma (mb, xQ, yQ, block_width, block_height);

    if (MbP || (mb->deblockFilterDisableIdc == 0)) {
      int bitDepthScale = decoder->coding.bitDepthScale[eChroma];
      int max_imgpel_value = decoder->coding.maxPelValueComp[uv + 1];

      int AlphaC0Offset = mb->deblockFilterC0Offset;
      int BetaOffset = mb->deblockFilterBetaOffset;
      int width = p->chromaStride; //p->sizeXcr;

      // Average QP of the two blocks
      int QP = (MbP->qpc[uv] + mb->qpc[uv] + 1) >> 1;
      int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
      int indexB = iClip3(0, MAX_QP, QP + BetaOffset);
      int Alpha = ALPHA_TABLE[indexA] * bitDepthScale;
      int Beta = BETA_TABLE [indexB] * bitDepthScale;
      if ((Alpha | Beta) != 0) {
        const int PelNum = pelnum_cr[1][p->chromaFormatIdc];
        const uint8_t* ClipTab = CLIP_TAB[indexA];
        sPixel* imgP = &img[get_pos_y_chroma(MbP,yQ, (block_height-1))][get_pos_x_chroma(MbP,xQ, (block_width - 1))];
        sPixel* imgQ = imgP + width ;
        for (int pel = 0 ; pel < PelNum ; ++pel ) {
          int Strng = Strength[(PelNum == 8) ? (pel >> 1) : (pel >> 2)];
          if (Strng != 0) {
            sPixel* SrcPtrP = imgP;
            sPixel* SrcPtrQ = imgQ;
            int edge_diff = *imgQ - *imgP;
            if (iabs (edge_diff ) < Alpha) {
              sPixel R1  = *(SrcPtrQ + width);
              if (iabs(*SrcPtrQ - R1) < Beta ) {
                sPixel L1  = *(SrcPtrP - width);
                if (iabs(*SrcPtrP - L1) < Beta ) {
                  if (Strng == 4) {
                    // INTRA strong filtering
                    *SrcPtrP = (sPixel) ( ((L1 << 1) + *SrcPtrP + R1 + 2) >> 2 );
                    *SrcPtrQ = (sPixel) ( ((R1 << 1) + *SrcPtrQ + L1 + 2) >> 2 );
                    }
                  else {
                    int tc0  = ClipTab[ Strng ] * bitDepthScale + 1;
                    int dif = iClip3 (-tc0, tc0, ( ((edge_diff) << 2) + (L1 - R1) + 4) >> 3 );

                    if (dif != 0) {
                      *SrcPtrP = (sPixel) iClip1 (max_imgpel_value, *SrcPtrP + dif );
                      *SrcPtrQ = (sPixel) iClip1 (max_imgpel_value, *SrcPtrQ - dif );
                      }
                    }
                 }
               }
              }
            }
          imgP++;
          imgQ++;
          }
        }
      }
    }
  //}}}
  //{{{
  void set_loop_filter_functions_normal (cDecoder264* decoder) {

    decoder->getStrengthV = get_strength_ver;
    decoder->getStrengthH = get_strength_hor;
    decoder->edgeLoopLumaV = edge_loop_luma_ver;
    decoder->edgeLoopLumaH = edge_loop_luma_hor;
    decoder->edgeLoopChromaV = edge_loop_chroma_ver;
    decoder->edgeLoopChromaH = edge_loop_chroma_hor;
    }
  //}}}

  // loopfilter
  //{{{
  void deblockMb (cDecoder264* decoder, sPicture* p, int mbIndex) {

    sMacroBlock* mb = &(decoder->mbData[mbIndex]) ; // current Mb

    // return, if filter is disabled
    if (mb->deblockFilterDisableIdc == 1)
      mb->DeblockCall = 0;

    else {
      int edge;
      uint8_t Strength[16];
      int16_t mb_x, mb_y;
      int filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
      int filterLeftMbEdgeFlag;
      int filterTopMbEdgeFlag;
      int edge_cr;

      sPixel** imgY = p->imgY;
      sPixel*** imgUV = p->imgUV;
      cSlice* slice = mb->slice;
      int mvLimit = ((p->picStructure!=eFrame) || (p->mbAffFrame && mb->mbField)) ? 2 : 4;
      cSps* activeSps = decoder->activeSps;

      mb->DeblockCall = 1;
      getMbPos (decoder, mbIndex, decoder->mbSize[eLuma], &mb_x, &mb_y);

      filterNon8x8LumaEdgesFlag[1] = filterNon8x8LumaEdgesFlag[3] = !(mb->lumaTransformSize8x8flag);

      filterLeftMbEdgeFlag = (mb_x != 0);
      filterTopMbEdgeFlag  = (mb_y != 0);

      if (p->mbAffFrame && mb_y == MB_BLOCK_SIZE && mb->mbField)
        filterTopMbEdgeFlag = 0;

      if (mb->deblockFilterDisableIdc == 2) {
        // don't filter at slice boundaries
        filterLeftMbEdgeFlag = mb->mbAvailA;
        // if this the bottom of a frame macroBlock pair then always filter the top edge
        filterTopMbEdgeFlag  = (p->mbAffFrame && !mb->mbField && (mbIndex & 0x01)) ? 1 : mb->mbAvailB;
        }

      if (p->mbAffFrame == 1)
        checkNeighboursMbAff (mb);

      // Vertical deblocking
      for (edge = 0; edge < 4 ; ++edge ) {
        // If codedBlockPattern == 0 then deblocking for some macroBlock types could be skipped
        if (mb->codedBlockPattern == 0 && (slice->sliceType == cSlice::eP || slice->sliceType == cSlice::eB)) {
          if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSps->chromaFormatIdc != YUV444)
            continue;
          else if (edge > 0) {
            if ((mb->mbType == PSKIP && slice->sliceType == cSlice::eP) ||
                (mb->mbType == P16x16) || (mb->mbType == P16x8))
              continue;
            else if ((edge & 0x01) && ((mb->mbType == P8x16) ||
                     (slice->sliceType == cSlice::eB &&
                      mb->mbType == BSKIP_DIRECT &&
                      activeSps->isDirect8x8inference)))
              continue;
            }
          }

        if (edge || filterLeftMbEdgeFlag) {
          // Strength for 4 blks in 1 stripe
          get_strength_ver_MBAff (Strength, mb, edge << 2, mvLimit, p);
          if  (Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] !=0 ||
               Strength[4] != 0 || Strength[5] != 0 || Strength[6] != 0 || Strength[7] !=0 ||
               Strength[8] != 0 || Strength[9] != 0 || Strength[10] != 0 || Strength[11] !=0 ||
               Strength[12] != 0 || Strength[13] != 0 || Strength[14] != 0 || Strength[15] !=0 ) {
            // only if one of the 16 Strength bytes is != 0
            if (filterNon8x8LumaEdgesFlag[edge]) {
              decoder->edgeLoopLumaV (PLANE_Y, imgY, Strength, mb, edge << 2);
              if (slice->chroma444notSeparate) {
                decoder->edgeLoopLumaV (PLANE_U, imgUV[0], Strength, mb, edge << 2);
                decoder->edgeLoopLumaV (PLANE_V, imgUV[1], Strength, mb, edge << 2);
                }
              }
            if (activeSps->chromaFormatIdc == YUV420 || activeSps->chromaFormatIdc == YUV422) {
              edge_cr = chroma_edge[0][edge][p->chromaFormatIdc];
              if ((imgUV != NULL) && (edge_cr >= 0)) {
                decoder->edgeLoopChromaV (imgUV[0], Strength, mb, edge_cr, 0, p);
                decoder->edgeLoopChromaV (imgUV[1], Strength, mb, edge_cr, 1, p);
                }
              }
            }
          }
        }

      // horizontal deblocking
      for (edge = 0; edge < 4 ; ++edge ) {
        // If codedBlockPattern == 0 then deblocking for some macroBlock types could be skipped
        if (mb->codedBlockPattern == 0 && (slice->sliceType == cSlice::eP || slice->sliceType == cSlice::eB)) {
          if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSps->chromaFormatIdc==YUV420)
            continue;
          else if (edge > 0) {
            if (((mb->mbType == PSKIP && slice->sliceType == cSlice::eP) || (mb->mbType == P16x16) || (mb->mbType == P8x16)))
              continue;
            else if ((edge & 0x01) && ((mb->mbType == P16x8) || (slice->sliceType == cSlice::eB && mb->mbType == BSKIP_DIRECT && activeSps->isDirect8x8inference)))
              continue;
            }
          }

        if (edge || filterTopMbEdgeFlag ) {
          // Strength for 4 blks in 1 stripe
          get_strength_hor_MBAff (Strength, mb, edge << 2, mvLimit, p);
          if  (Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] !=0 ||
               Strength[4] != 0 || Strength[5] != 0 || Strength[6] != 0 || Strength[7] !=0 ||
               Strength[8] != 0 || Strength[9] != 0 || Strength[10] != 0 || Strength[11] !=0 ||
               Strength[12] != 0 || Strength[13] != 0 || Strength[14] != 0 || Strength[15] !=0 ) {
            // only if one of the 16 Strength bytes is != 0
            if (filterNon8x8LumaEdgesFlag[edge]) {
              decoder->edgeLoopLumaH (PLANE_Y, imgY, Strength, mb, edge << 2, p) ;
               if(slice->chroma444notSeparate) {
                decoder->edgeLoopLumaH (PLANE_U, imgUV[0], Strength, mb, edge << 2, p);
                decoder->edgeLoopLumaH (PLANE_V, imgUV[1], Strength, mb, edge << 2, p);
                }
              }

            if ((activeSps->chromaFormatIdc == YUV420) || (activeSps->chromaFormatIdc == YUV422)) {
              edge_cr = chroma_edge[1][edge][p->chromaFormatIdc];
              if ((imgUV != NULL) && (edge_cr >= 0)) {
                decoder->edgeLoopChromaH (imgUV[0], Strength, mb, edge_cr, 0, p);
                decoder->edgeLoopChromaH (imgUV[1], Strength, mb, edge_cr, 1, p);
                }
              }
            }

          if (!edge && !mb->mbField && mb->mixedModeEdgeFlag) {
            // this is the extra horizontal edge between a frame macroBlock pair and a field above it
            mb->DeblockCall = 2;
            get_strength_hor_MBAff (Strength, mb, MB_BLOCK_SIZE, mvLimit, p); // Strength for 4 blks in 1 stripe

            {
              if (filterNon8x8LumaEdgesFlag[edge]) {
                decoder->edgeLoopLumaH (PLANE_Y, imgY, Strength, mb, MB_BLOCK_SIZE, p) ;
                if (slice->chroma444notSeparate) {
                  decoder->edgeLoopLumaH (PLANE_U, imgUV[0], Strength, mb, MB_BLOCK_SIZE, p) ;
                  decoder->edgeLoopLumaH (PLANE_V, imgUV[1], Strength, mb, MB_BLOCK_SIZE, p) ;
                  }
                }

              if ((activeSps->chromaFormatIdc == YUV420) || (activeSps->chromaFormatIdc == YUV422)) {
                edge_cr = chroma_edge[1][edge][p->chromaFormatIdc];
                if (imgUV && (edge_cr >= 0)) {
                  decoder->edgeLoopChromaH (imgUV[0], Strength, mb, MB_BLOCK_SIZE, 0, p) ;
                  decoder->edgeLoopChromaH (imgUV[1], Strength, mb, MB_BLOCK_SIZE, 1, p) ;
                  }
                }
              }
            mb->DeblockCall = 1;
            }
          }
        }

      mb->DeblockCall = 0;
      }
    }
  //}}}
  //{{{
  void getDeblockStrength (cDecoder264* decoder, sPicture* p, int mbIndex) {

    sMacroBlock* mb = &(decoder->mbData[mbIndex]) ; // current Mb

    // return, if filter is disabled
    if (mb->deblockFilterDisableIdc == 1)
      mb->DeblockCall = 0;

    else {
      int edge;
      int16_t mb_x, mb_y;
      int filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
      int filterLeftMbEdgeFlag;
      int filterTopMbEdgeFlag;

      cSlice* slice = mb->slice;
      int mvLimit = ((p->picStructure!=eFrame) || (p->mbAffFrame && mb->mbField)) ? 2 : 4;
      cSps* activeSps = decoder->activeSps;

      mb->DeblockCall = 1;
      getMbPos (decoder, mbIndex, decoder->mbSize[eLuma], &mb_x, &mb_y);

      filterNon8x8LumaEdgesFlag[1] =
        filterNon8x8LumaEdgesFlag[3] = !(mb->lumaTransformSize8x8flag);

      filterLeftMbEdgeFlag = (mb_x != 0);
      filterTopMbEdgeFlag  = (mb_y != 0);

      if (p->mbAffFrame && mb_y == MB_BLOCK_SIZE && mb->mbField)
        filterTopMbEdgeFlag = 0;

      if (mb->deblockFilterDisableIdc==2) {
        // don't filter at slice boundaries
        filterLeftMbEdgeFlag = mb->mbAvailA;
        // if this the bottom of a frame macroBlock pair then always filter the top edge
        filterTopMbEdgeFlag = (p->mbAffFrame && !mb->mbField && (mbIndex & 0x01)) ? 1 : mb->mbAvailB;
        }

      if (p->mbAffFrame == 1)
        checkNeighboursMbAff (mb);

      // Vertical deblocking
      for (edge = 0; edge < 4 ; ++edge ) {
        // If codedBlockPattern == 0 then deblocking for some macroBlock types could be skipped
        if (mb->codedBlockPattern == 0 && (slice->sliceType == cSlice::eP || slice->sliceType == cSlice::eB)) {
          if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSps->chromaFormatIdc != YUV444)
            continue;
          else if (edge > 0) {
            if (((mb->mbType == PSKIP && slice->sliceType == cSlice::eP) ||
                 (mb->mbType == P16x16) ||
                 (mb->mbType == P16x8)))
              continue;
            else if ((edge & 0x01) &&
                     ((mb->mbType == P8x16) ||
                     (slice->sliceType == cSlice::eB &&
                      mb->mbType == BSKIP_DIRECT && activeSps->isDirect8x8inference)))
              continue;
            }
          }

        if (edge || filterLeftMbEdgeFlag )
          // Strength for 4 blks in 1 stripe
          decoder->getStrengthV (mb, edge, mvLimit, p);
        }//end edge

      // horizontal deblocking
      for (edge = 0; edge < 4 ; ++edge ) {
        // If codedBlockPattern == 0 then deblocking for some macroBlock types could be skipped
        if (mb->codedBlockPattern == 0 && (slice->sliceType == cSlice::eP || slice->sliceType == cSlice::eB)) {
          if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSps->chromaFormatIdc==YUV420)
            continue;
          else if (edge > 0) {
            if (((mb->mbType == PSKIP && slice->sliceType == cSlice::eP) ||
                 (mb->mbType == P16x16) || (mb->mbType == P8x16)))
              continue;
            else if ((edge & 0x01) &&
                     ((mb->mbType == P16x8) ||
                       (slice->sliceType == cSlice::eB &&
                        mb->mbType == BSKIP_DIRECT &&
                        activeSps->isDirect8x8inference)))
              continue;
            }
          }

        if (edge || filterTopMbEdgeFlag )
          decoder->getStrengthH (mb, edge, mvLimit, p);
        }

      mb->DeblockCall = 0;
      }
    }
  //}}}
  //{{{
  void performDeblock (cDecoder264* decoder, sPicture* p, int mbIndex) {

    sMacroBlock* mb = &(decoder->mbData[mbIndex]) ; // current Mb

    // return, if filter is disabled
    if (mb->deblockFilterDisableIdc == 1)
      mb->DeblockCall = 0;

    else {
      int edge;
      int16_t mb_x, mb_y;
      int filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
      int filterLeftMbEdgeFlag;
      int filterTopMbEdgeFlag;
      int edge_cr;

      sPixel** imgY = p->imgY;
      sPixel** *imgUV = p->imgUV;
      cSlice* slice = mb->slice;
      int mvLimit = ((p->picStructure!=eFrame) || (p->mbAffFrame && mb->mbField)) ? 2 : 4;
      cSps* activeSps = decoder->activeSps;

      mb->DeblockCall = 1;
      getMbPos (decoder, mbIndex, decoder->mbSize[eLuma], &mb_x, &mb_y);

      filterNon8x8LumaEdgesFlag[1] = filterNon8x8LumaEdgesFlag[3] = !(mb->lumaTransformSize8x8flag);

      filterLeftMbEdgeFlag = (mb_x != 0);
      filterTopMbEdgeFlag  = (mb_y != 0);

      if (p->mbAffFrame && mb_y == MB_BLOCK_SIZE && mb->mbField)
        filterTopMbEdgeFlag = 0;

      if (mb->deblockFilterDisableIdc==2) {
        // don't filter at slice boundaries
        filterLeftMbEdgeFlag = mb->mbAvailA;
        // if this the bottom of a frame macroBlock pair then always filter the top edge
        filterTopMbEdgeFlag  = (p->mbAffFrame && !mb->mbField && (mbIndex & 0x01)) ? 1 : mb->mbAvailB;
        }

      if (p->mbAffFrame == 1)
        checkNeighboursMbAff (mb);

      // Vertical deblocking
      for (edge = 0; edge < 4 ; ++edge ) {
        // If codedBlockPattern == 0 then deblocking for some macroBlock types could be skipped
        if (mb->codedBlockPattern == 0 && (slice->sliceType == cSlice::eP || slice->sliceType == cSlice::eB)) {
          if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSps->chromaFormatIdc != YUV444)
            continue;
          else if (edge > 0) {
            if (((mb->mbType == PSKIP && slice->sliceType == cSlice::eP) || (mb->mbType == P16x16) || (mb->mbType == P16x8)))
              continue;
            else if ((edge & 0x01) && ((mb->mbType == P8x16) || (slice->sliceType == cSlice::eB && mb->mbType == BSKIP_DIRECT && activeSps->isDirect8x8inference)))
              continue;
            }
          }

        if (edge || filterLeftMbEdgeFlag ) {
          uint8_t *Strength = mb->strengthV[edge];
          if  (Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) {
            // only if one of the 4 first Strength bytes is != 0
            if (filterNon8x8LumaEdgesFlag[edge]) {
              decoder->edgeLoopLumaV (PLANE_Y, imgY, Strength, mb, edge << 2);
              if(slice->chroma444notSeparate) {
                decoder->edgeLoopLumaV (PLANE_U, imgUV[0], Strength, mb, edge << 2);
                decoder->edgeLoopLumaV (PLANE_V, imgUV[1], Strength, mb, edge << 2);
                }
              }
            if (activeSps->chromaFormatIdc == YUV420 || activeSps->chromaFormatIdc == YUV422) {
              edge_cr = chroma_edge[0][edge][p->chromaFormatIdc];
              if ((imgUV != NULL) && (edge_cr >= 0)) {
                decoder->edgeLoopChromaV (imgUV[0], Strength, mb, edge_cr, 0, p);
                decoder->edgeLoopChromaV (imgUV[1], Strength, mb, edge_cr, 1, p);
                }
              }
            }
          }
        }//end edge

      // horizontal deblocking
      for (edge = 0; edge < 4 ; ++edge ) {
        // If codedBlockPattern == 0 then deblocking for some macroBlock types could be skipped
        if (mb->codedBlockPattern == 0 && (slice->sliceType == cSlice::eP || slice->sliceType == cSlice::eB)) {
          if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSps->chromaFormatIdc==YUV420)
            continue;
          else if (edge > 0) {
            if (((mb->mbType == PSKIP && slice->sliceType == cSlice::eP) || (mb->mbType == P16x16) || (mb->mbType == P8x16)))
              continue;
            else if ((edge & 0x01) && ((mb->mbType == P16x8) || (slice->sliceType == cSlice::eB && mb->mbType == BSKIP_DIRECT && activeSps->isDirect8x8inference)))
              continue;
            }
          }

        if (edge || filterTopMbEdgeFlag ) {
          uint8_t* Strength = mb->strengthH[edge];
          if (Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] !=0 ||
              Strength[4] != 0 || Strength[5] != 0 || Strength[6] != 0 || Strength[7] !=0 ||
              Strength[8] != 0 || Strength[9] != 0 || Strength[10] != 0 || Strength[11] != 0 ||
              Strength[12] != 0 || Strength[13] != 0 || Strength[14] != 0 || Strength[15] !=0 ) {
            // only if one of the 16 Strength bytes is != 0
            if (filterNon8x8LumaEdgesFlag[edge]) {
              decoder->edgeLoopLumaH (PLANE_Y, imgY, Strength, mb, edge << 2, p) ;
              if(slice->chroma444notSeparate) {
                decoder->edgeLoopLumaH (PLANE_U, imgUV[0], Strength, mb, edge << 2, p);
                decoder->edgeLoopLumaH (PLANE_V, imgUV[1], Strength, mb, edge << 2, p);
                }
              }

            if (activeSps->chromaFormatIdc == YUV420 || activeSps->chromaFormatIdc == YUV422) {
              edge_cr = chroma_edge[1][edge][p->chromaFormatIdc];
              if ((imgUV != NULL) && (edge_cr >= 0)) {
                decoder->edgeLoopChromaH (imgUV[0], Strength, mb, edge_cr, 0, p);
                decoder->edgeLoopChromaH (imgUV[1], Strength, mb, edge_cr, 1, p);
                }
              }
            }

          if (!edge && !mb->mbField && mb->mixedModeEdgeFlag) {
            //slice->mixedModeEdgeFlag)
            // this is the extra horizontal edge between a frame macroBlock pair and a field above it
            mb->DeblockCall = 2;
            decoder->getStrengthH (mb, 4, mvLimit, p); // Strength for 4 blks in 1 stripe

            {
              if (filterNon8x8LumaEdgesFlag[edge]) {
                decoder->edgeLoopLumaH(PLANE_Y, imgY, Strength, mb, MB_BLOCK_SIZE, p) ;
                if(slice->chroma444notSeparate) {
                  decoder->edgeLoopLumaH (PLANE_U, imgUV[0], Strength, mb, MB_BLOCK_SIZE, p) ;
                  decoder->edgeLoopLumaH (PLANE_V, imgUV[1], Strength, mb, MB_BLOCK_SIZE, p) ;
                  }
                }

              if (activeSps->chromaFormatIdc == YUV420 || activeSps->chromaFormatIdc == YUV422) {
                edge_cr = chroma_edge[1][edge][p->chromaFormatIdc];
                if ((imgUV != NULL) && (edge_cr >= 0)) {
                  decoder->edgeLoopChromaH (imgUV[0], Strength, mb, MB_BLOCK_SIZE, 0, p) ;
                  decoder->edgeLoopChromaH (imgUV[1], Strength, mb, MB_BLOCK_SIZE, 1, p) ;
                  }
                }
              }
            mb->DeblockCall = 1;
            }
          }
        }

      mb->DeblockCall = 0;
      }
    }
  //}}}
  //{{{
  void initNeighbours (cDecoder264* decoder) {

    int size = decoder->picSizeInMbs;
    int width = decoder->coding.picWidthMbs;
    int height = decoder->picHeightInMbs;

    // do the top left corner
    sMacroBlock* mb = &decoder->mbData[0];
    mb->mbUp = NULL;
    mb->mbLeft = NULL;
    mb++;

    // do top row
    for (int i = 1; i < width; i++) {
      mb->mbUp = NULL;
      mb->mbLeft = mb - 1;
      mb++;
      }

    // do left edge
    for (int i = width; i < size; i += width) {
      mb->mbUp = mb - width;
      mb->mbLeft = NULL;
      mb += width;
      }

    // do all others
    for (int j = width + 1; j < width * height + 1; j += width) {
      mb = &decoder->mbData[j];
      for (int i = 1; i < width; i++) {
        mb->mbUp   = mb - width;
        mb->mbLeft = mb - 1;
        mb++;
        }
      }
    }
  //}}}
  }

//{{{
void deblockPicture (cDecoder264* decoder, sPicture* p) {

  if (p->mbAffFrame)
    for (uint32_t i = 0; i < p->picSizeInMbs; ++i)
      deblockMb (decoder, p, i) ;
  else {
    for (uint32_t i = 0; i < p->picSizeInMbs; ++i)
      getDeblockStrength (decoder, p, i ) ;
    for (uint32_t i = 0; i < p->picSizeInMbs; ++i)
      performDeblock (decoder, p, i) ;
    }
  }
//}}}
//{{{
void initDeblock (cDecoder264* decoder, int mbAffFrame) {

  if (decoder->coding.yuvFormat == YUV444 && decoder->coding.isSeperateColourPlane) {
    decoder->changePlaneJV (PLANE_Y, NULL);
    initNeighbours (decoder);

    decoder->changePlaneJV (PLANE_U, NULL);
    initNeighbours (decoder);

    decoder->changePlaneJV (PLANE_V, NULL);
    initNeighbours (decoder);

    decoder->changePlaneJV (PLANE_Y, NULL);
    }
  else
    initNeighbours (decoder);

  if (mbAffFrame == 1)
    set_loop_filter_functions_mbaff (decoder);
  else
    set_loop_filter_functions_normal (decoder);
  }
//}}}
