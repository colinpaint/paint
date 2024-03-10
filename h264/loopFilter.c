//{{{
/*!
** ***********************************************************************************
 * \file loopFilter.c
 *
 * \brief
 *    Filter to reduce blocking artifacts on a macroblock level.
 *    The filter strength is QP dependent.
 *
 * \author
 *    Contributors:
 *    - Peter List       Peter.List@t-systems.de:  Original code                                 (13-Aug-2001)
 *    - Jani Lainema     Jani.Lainema@nokia.com:   Some bug fixing, removal of recursiveness     (16-Aug-2001)
 *    - Peter List       Peter.List@t-systems.de:  inplace filtering and various simplifications (10-Jan-2002)
 *    - Anthony Joch     anthony@ubvideo.com:      Simplified switching between filters and
 *                                                 non-recursive default filter.                 (08-Jul-2002)
 *    - Cristina Gomila  cristina.gomila@thomson.net: Simplification of the chroma deblocking
 *                                                    from JVT-E089                              (21-Nov-2002)
 *    - Alexis Michael Tourapis atour@dolby.com:   Speed/Architecture improvements               (08-Feb-2007)
** ***********************************************************************************
 */
//}}}
//{{{
#include "global.h"

#include "image.h"
#include "mbAccess.h"
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
//{{{
// NOTE: In principle, the alpha and beta tables are calculated with the formulas below
//       Alpha( qp ) = 0.8 * (2^(qp/6)  -  1)
//       Beta ( qp ) = 0.5 * qp  -  7
// The tables actually used have been "hand optimized" though (by Anthony Joch). So, the
// table values might be a little different to formula-generated values. Also, the first
// few values of both tables is set to zero to force the filter off at low qp’s
static const byte ALPHA_TABLE[52] = {
  0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,4,4,5,6,
  7,8,9,10,12,13,15,17,
  20,22,25,28,32,36,40,45,
  50,56,63,71,80,90,101,113,
  127,144,162,182,203,226,255,255} ;
//}}}
//{{{
static const byte BETA_TABLE[52] = {
  0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,2,2,2,3,
  3,3,3, 4, 4, 4, 6, 6,
  7, 7, 8, 8, 9, 9,10,10,
  11,11,12,12,13,13, 14, 14,
  15, 15, 16, 16, 17, 17, 18, 18} ;
//}}}
//{{{
static const byte CLIP_TAB[52][5] = {
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
//[dir][edge][yuvFormat]
static const char chroma_edge[2][4][4] = {
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
static const int pelnum_cr[2][4] =  {
  {0,8,16,16},
  {0,8, 8,16}};  //[dir:0=vert, 1=hor.][yuvFormat]
//}}}

//{{{
static inline int compare_mvs (const sMotionVec* mv0, const sMotionVec* mv1, int mvlimit) {
  return ((iabs (mv0->mvX - mv1->mvX) >= 4) | (iabs (mv0->mvY - mv1->mvY) >= mvlimit));
  }
//}}}
//{{{
static sMacroblock* get_non_aff_neighbour_luma (sMacroblock* mb, int xN, int yN) {

  if (xN < 0)
    return (mb->mbLeft);
  else if (yN < 0)
    return (mb->mbUp);
  else
    return (mb);
  }
//}}}
//{{{
static sMacroblock* get_non_aff_neighbour_chroma (sMacroblock* mb, int xN, int yN,
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
static void edge_loop_luma_ver_MBAff (eColorPlane pl, sPixel** img,
                                      byte* Strength, sMacroblock* mbQ, int edge) {

  int      pel, Strng ;
  sPixel   L2 = 0, L1, L0, R0, R1, R2 = 0;
  int      Alpha = 0, Beta = 0 ;
  const byte* ClipTab = NULL;
  int      indexA, indexB;
  int      QP;
  sPixelPos pixP, pixQ;

  sDecoder* decoder = mbQ->decoder;
  int bitdepth_scale = pl ? decoder->bitdepth_scale[IS_CHROMA] : decoder->bitdepth_scale[IS_LUMA];
  int max_imgpel_value = decoder->maxPelValueComp[pl];

  int AlphaC0Offset = mbQ->DFAlphaC0Offset;
  int BetaOffset = mbQ->DFBetaOffset;

  sMacroblock*MbP;
  sPixel* SrcPtrP, *SrcPtrQ;

  for (pel = 0 ; pel < MB_BLOCK_SIZE ; ++pel ) {
    getAffNeighbour (mbQ, edge - 1, pel, decoder->mbSize[IS_LUMA], &pixP);

    if  (pixP.available || (mbQ->DFDisableIdc == 0)) {
      if ((Strng = Strength[pel]) != 0) {
        getAffNeighbour (mbQ, edge, pel, decoder->mbSize[IS_LUMA], &pixQ);

        MbP = &(decoder->mbData[pixP.mbAddr]);
        SrcPtrQ = &(img[pixQ.posY][pixQ.posX]);
        SrcPtrP = &(img[pixP.posY][pixP.posX]);

        // Average QP of the two blocks
        QP = pl? ((MbP->qpc[pl-1] + mbQ->qpc[pl-1] + 1) >> 1) : (MbP->qp + mbQ->qp + 1) >> 1;
        indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
        indexB = iClip3(0, MAX_QP, QP + BetaOffset);
        Alpha = ALPHA_TABLE[indexA] * bitdepth_scale;
        Beta = BETA_TABLE [indexB] * bitdepth_scale;
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

              int C0 = ClipTab[ Strng ] * bitdepth_scale;
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
static void edge_loop_luma_hor_MBAff (eColorPlane pl, sPixel** img, byte* Strength, sMacroblock* mbQ,
                                      int edge, sPicture *p)
{
  int      width = p->iLumaStride; //p->sizeX;
  int      pel, Strng ;
  int      PelNum = pl? pelnum_cr[1][p->chromaFormatIdc] : MB_BLOCK_SIZE;

  int      yQ = (edge < MB_BLOCK_SIZE ? edge : 1);

  sPixelPos pixP, pixQ;

  sDecoder* decoder = mbQ->decoder;
  int      bitdepth_scale = pl? decoder->bitdepth_scale[IS_CHROMA] : decoder->bitdepth_scale[IS_LUMA];
  int      max_imgpel_value = decoder->maxPelValueComp[pl];

  getAffNeighbour(mbQ, 0, yQ - 1, decoder->mbSize[IS_LUMA], &pixP);

  if (pixP.available || (mbQ->DFDisableIdc == 0)) {
    int AlphaC0Offset = mbQ->DFAlphaC0Offset;
    int BetaOffset = mbQ->DFBetaOffset;

    sMacroblock *MbP = &(decoder->mbData[pixP.mbAddr]);

    int incQ    = ((MbP->mbField && !mbQ->mbField) ? 2 * width : width);
    int incP    = ((mbQ->mbField && !MbP->mbField) ? 2 * width : width);

    // Average QP of the two blocks
    int QP = pl? ((MbP->qpc[pl - 1] + mbQ->qpc[pl - 1] + 1) >> 1) : (MbP->qp + mbQ->qp + 1) >> 1;
    int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + BetaOffset);
    int Alpha   = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta    = BETA_TABLE [indexB] * bitdepth_scale;
    if ((Alpha | Beta )!= 0) {
      const byte* ClipTab = CLIP_TAB[indexA];
      getAffNeighbour(mbQ, 0, yQ, decoder->mbSize[IS_LUMA], &pixQ);

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

                int C0  = ClipTab[ Strng ] * bitdepth_scale;
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
static void edge_loop_chroma_ver_MBAff (sPixel** img, byte *Strength, sMacroblock* mbQ,
                                        int edge, int uv, sPicture *p) {

  int      pel, Strng ;
  sPixel   L1, L0, R0, R1;
  int      Alpha = 0, Beta = 0;
  const byte* ClipTab = NULL;
  int      indexA, indexB;
  sDecoder* decoder = mbQ->decoder;
  int      PelNum = pelnum_cr[0][p->chromaFormatIdc];
  int      StrengthIdx;
  int      QP;
  int      xQ = edge, yQ;
  sPixelPos pixP, pixQ;
  int      bitdepth_scale = decoder->bitdepth_scale[IS_CHROMA];
  int      max_imgpel_value = decoder->maxPelValueComp[uv + 1];

  int      AlphaC0Offset = mbQ->DFAlphaC0Offset;
  int      BetaOffset    = mbQ->DFBetaOffset;
  sMacroblock *MbP;
  sPixel   *SrcPtrP, *SrcPtrQ;

  for (pel = 0 ; pel < PelNum ; ++pel ) {
    yQ = pel;
    getAffNeighbour(mbQ, xQ, yQ, decoder->mbSize[IS_CHROMA], &pixQ);
    getAffNeighbour(mbQ, xQ - 1, yQ, decoder->mbSize[IS_CHROMA], &pixP);
    MbP = &(decoder->mbData[pixP.mbAddr]);
    StrengthIdx = (PelNum == 8) ? ((mbQ->mbField && !MbP->mbField) ? pel << 1 :((pel >> 1) << 2) + (pel & 0x01)) : pel;

    if (pixP.available || (mbQ->DFDisableIdc == 0)) {
      if ((Strng = Strength[StrengthIdx]) != 0) {
        SrcPtrQ = &(img[pixQ.posY][pixQ.posX]);
        SrcPtrP = &(img[pixP.posY][pixP.posX]);

        // Average QP of the two blocks
        QP = (MbP->qpc[uv] + mbQ->qpc[uv] + 1) >> 1;
        indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
        indexB = iClip3(0, MAX_QP, QP + BetaOffset);
        Alpha   = ALPHA_TABLE[indexA] * bitdepth_scale;
        Beta    = BETA_TABLE [indexB] * bitdepth_scale;
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
              int C0  = ClipTab[ Strng ] * bitdepth_scale;
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
static void edge_loop_chroma_hor_MBAff (sPixel** img, byte *Strength, sMacroblock* mbQ, int edge, int uv, sPicture *p)
{
  sDecoder* decoder = mbQ->decoder;
  int PelNum = pelnum_cr[1][p->chromaFormatIdc];
  int yQ = (edge < MB_BLOCK_SIZE? edge : 1);
  sPixelPos pixP, pixQ;
  int bitdepth_scale = decoder->bitdepth_scale[IS_CHROMA];
  int max_imgpel_value = decoder->maxPelValueComp[uv + 1];

  int AlphaC0Offset = mbQ->DFAlphaC0Offset;
  int BetaOffset    = mbQ->DFBetaOffset;
  int width = p->iChromaStride; //p->sizeXcr;

  getAffNeighbour (mbQ, 0, yQ - 1, decoder->mbSize[IS_CHROMA], &pixP);
  getAffNeighbour (mbQ, 0, yQ, decoder->mbSize[IS_CHROMA], &pixQ);

  if (pixP.available || (mbQ->DFDisableIdc == 0)) {
    sMacroblock* MbP = &(decoder->mbData[pixP.mbAddr]);
    int incQ = ((MbP->mbField && !mbQ->mbField) ? 2 * width : width);
    int incP = ((mbQ->mbField  && !MbP->mbField) ? 2 * width : width);

    // Average QP of the two blocks
    int QP = (MbP->qpc[uv] + mbQ->qpc[uv] + 1) >> 1;
    int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + BetaOffset);
    int Alpha = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta = BETA_TABLE [indexB] * bitdepth_scale;

    if ((Alpha | Beta )!= 0) {
      const byte* ClipTab = CLIP_TAB[indexA];
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
                int C0  = ClipTab[ Strng ] * bitdepth_scale;
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
static void get_strength_ver_MBAff (byte* Strength, sMacroblock* mb, int edge, int mvlimit, sPicture* p) {

  short  blkP, blkQ, idx;
  int    StrValue, i;
  short  mb_x, mb_y;
  sPixelPos pixP;
  sDecoder* decoder = mb->decoder;
  sBlockPos *picPos = decoder->picPos;

  sMacroblock* MbP;
  if ((p->sliceType==SP_SLICE)||(p->sliceType==SI_SLICE) ) {
    for (idx = 0; idx < MB_BLOCK_SIZE; ++idx ) {
      getAffNeighbour(mb, edge - 1, idx, decoder->mbSize[IS_LUMA], &pixP);
      blkQ = (short) ((idx & 0xFFFC) + (edge >> 2));
      blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));
      MbP = &(decoder->mbData[pixP.mbAddr]);
      mb->mixedModeEdgeFlag = (byte) (mb->mbField != MbP->mbField);
      Strength[idx] = (edge == 0) ? 4 : 3;
      }
    }
  else {
    getAffNeighbour (mb, edge - 1, 0, decoder->mbSize[IS_LUMA], &pixP);

    MbP = &(decoder->mbData[pixP.mbAddr]);
    // Neighboring Frame MBs
    if ((mb->mbField == FALSE && MbP->mbField == FALSE)) {
      mb->mixedModeEdgeFlag = (byte) (mb->mbField != MbP->mbField);
      if (mb->isIntraBlock == TRUE || MbP->isIntraBlock == TRUE) {
        //printf("idx %d %d %d %d %d\n", idx, pixP.x, pixP.y, pixP.posX, pixP.posY);
        // Start with Strength=3. or Strength=4 for Mb-edge
        StrValue = (edge == 0) ? 4 : 3;
        for (i = 0; i < MB_BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
        }
      else {
        get_mb_block_pos_mbaff (picPos, mb->mbAddrX, &mb_x, &mb_y);
        for (idx = 0; idx < MB_BLOCK_SIZE; idx += BLOCK_SIZE) {
          blkQ = (short) ((idx & 0xFFFC) + (edge >> 2));
          blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));

          if (((mb->cbpStructure[0].blk & i64_power2(blkQ)) != 0) || ((MbP->cbpStructure[0].blk & i64_power2(blkP)) != 0))
            StrValue = 2;
          else if (edge && ((mb->mbType == 1)  || (mb->mbType == 2)))
            StrValue = 0; // if internal edge of certain types, we already know StrValue should be 0
          else {
            // for everything else, if no coefs, but vector difference >= 1 set Strength=1
            int blk_y  = ((mb_y<<2) + (blkQ >> 2));
            int blk_x  = ((mb_x<<2) + (blkQ  & 3));
            int blk_y2 = (pixP.posY >> 2);
            int blk_x2 = (pixP.posX >> 2);

            sPicMotionParam *mv_info_p = &p->mvInfo[blk_y ][blk_x ];
            sPicMotionParam *mv_info_q = &p->mvInfo[blk_y2][blk_x2];
            sPicturePtr ref_p0 = mv_info_p->refPic[LIST_0];
            sPicturePtr ref_q0 = mv_info_q->refPic[LIST_0];
            sPicturePtr ref_p1 = mv_info_p->refPic[LIST_1];
            sPicturePtr ref_q1 = mv_info_q->refPic[LIST_1];

            if  (((ref_p0==ref_q0) && (ref_p1==ref_q1))||((ref_p0==ref_q1) && (ref_p1==ref_q0))) {
              // L0 and L1 reference pictures of p0 are different; q0 as well
              if (ref_p0 != ref_p1) {
                // compare MV for the same reference picture
                if (ref_p0==ref_q0) {
                  StrValue =  (byte) (
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit));
                  }
                else {
                  StrValue =  (byte) (
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit));
                  }
                }
              else {
                // L0 and L1 reference pictures of p0 are the same; q0 as well
                StrValue = (byte) ((
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
        getAffNeighbour(mb, edge - 1, idx, decoder->mbSize[IS_LUMA], &pixP);
        blkQ = (short) ((idx & 0xFFFC) + (edge >> 2));
        blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));
        MbP = &(decoder->mbData[pixP.mbAddr]);
        mb->mixedModeEdgeFlag = (byte) (mb->mbField != MbP->mbField);

        // Start with Strength=3. or Strength=4 for Mb-edge
        Strength[idx] = (edge == 0 && (((!p->mbAffFrameFlag && (p->structure==FRAME)) ||
                         (p->mbAffFrameFlag && !MbP->mbField && !mb->mbField)) ||
                         ((p->mbAffFrameFlag || (p->structure!=FRAME))))) ? 4 : 3;

        if (mb->isIntraBlock == FALSE && MbP->isIntraBlock == FALSE) {
          if (((mb->cbpStructure[0].blk & i64_power2(blkQ)) != 0) || ((MbP->cbpStructure[0].blk & i64_power2(blkP)) != 0))
            Strength[idx] = 2 ;
          else {
            // if no coefs, but vector difference >= 1 set Strength=1
            // if this is a mixed mode edge then one set of reference pictures will be frame and the
            // other will be field
            if (mb->mixedModeEdgeFlag) //if (curSlice->mixedModeEdgeFlag)
              Strength[idx] = 1;
            else {
              get_mb_block_pos_mbaff (picPos, mb->mbAddrX, &mb_x, &mb_y);
              {
                int blk_y  = ((mb_y<<2) + (blkQ >> 2));
                int blk_x  = ((mb_x<<2) + (blkQ  & 3));
                int blk_y2 = (pixP.posY >> 2);
                int blk_x2 = (pixP.posX >> 2);

                sPicMotionParam *mv_info_p = &p->mvInfo[blk_y ][blk_x ];
                sPicMotionParam *mv_info_q = &p->mvInfo[blk_y2][blk_x2];
                sPicturePtr ref_p0 = mv_info_p->refPic[LIST_0];
                sPicturePtr ref_q0 = mv_info_q->refPic[LIST_0];
                sPicturePtr ref_p1 = mv_info_p->refPic[LIST_1];
                sPicturePtr ref_q1 = mv_info_q->refPic[LIST_1];

                if (((ref_p0==ref_q0) && (ref_p1==ref_q1))||((ref_p0==ref_q1) && (ref_p1==ref_q0))) {
                  Strength[idx] = 0;
                  // L0 and L1 reference pictures of p0 are different; q0 as well
                  if (ref_p0 != ref_p1) {
                    // compare MV for the same reference picture
                    if (ref_p0 == ref_q0) {
                      Strength[idx] =  (byte) (
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit));
                      }
                    else {
                      Strength[idx] =  (byte) (
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit));
                      }
                    }
                  else { // L0 and L1 reference pictures of p0 are the same; q0 as well
                    Strength[idx] = (byte) ((
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
static void get_strength_hor_MBAff (byte* Strength, sMacroblock* mb, int edge, int mvlimit, sPicture* p) {

  short  blkP, blkQ, idx;
  short  blk_x, blk_x2, blk_y, blk_y2 ;
  int    StrValue, i;
  int    xQ, yQ = (edge < MB_BLOCK_SIZE ? edge : 1);
  short  mb_x, mb_y;
  sMacroblock *MbP;
  sPixelPos pixP;
  sDecoder* decoder = mb->decoder;
  sBlockPos *picPos = decoder->picPos;

  if ((p->sliceType==SP_SLICE)||(p->sliceType==SI_SLICE) ) {
    for (idx = 0; idx < MB_BLOCK_SIZE; idx += BLOCK_SIZE) {
      xQ = idx;
      getAffNeighbour(mb, xQ, yQ - 1, decoder->mbSize[IS_LUMA], &pixP);

      blkQ = (short) ((yQ & 0xFFFC) + (xQ >> 2));
      blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));

      MbP = &(decoder->mbData[pixP.mbAddr]);
      mb->mixedModeEdgeFlag = (byte) (mb->mbField != MbP->mbField);

      StrValue = (edge == 0 && (!MbP->mbField && !mb->mbField)) ? 4 : 3;

      Strength[idx] = StrValue;
      Strength[idx+1] = StrValue;
      Strength[idx+2] = StrValue;
      Strength[idx+3] = StrValue;
      }
    }
  else {
    getAffNeighbour(mb, 0, yQ - 1, decoder->mbSize[IS_LUMA], &pixP);
    MbP = &(decoder->mbData[pixP.mbAddr]);
    mb->mixedModeEdgeFlag = (byte) (mb->mbField != MbP->mbField);

    // Set intra mode deblocking
    if (mb->isIntraBlock == TRUE || MbP->isIntraBlock == TRUE) {
      StrValue = (edge == 0 && (!MbP->mbField && !mb->mbField)) ? 4 : 3;
      for (i = 0; i < MB_BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
      }
    else {
      for (idx = 0; idx < MB_BLOCK_SIZE; idx += BLOCK_SIZE ) {
        xQ = idx;
        getAffNeighbour(mb, xQ, yQ - 1, decoder->mbSize[IS_LUMA], &pixP);
        blkQ = (short) ((yQ & 0xFFFC) + (xQ >> 2));
        blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));
        if (((mb->cbpStructure[0].blk & i64_power2(blkQ)) != 0) || ((MbP->cbpStructure[0].blk & i64_power2(blkP)) != 0))
          StrValue = 2;
        else {
          // if no coefs, but vector difference >= 1 set Strength=1
          // if this is a mixed mode edge then one set of reference pictures will be frame and the
          // other will be field
          if(mb->mixedModeEdgeFlag) //if (curSlice->mixedModeEdgeFlag)
            StrValue = 1;
          else {
            get_mb_block_pos_mbaff (picPos, mb->mbAddrX, &mb_x, &mb_y);
            blk_y  = (short) ((mb_y<<2) + (blkQ >> 2));
            blk_x  = (short) ((mb_x<<2) + (blkQ  & 3));
            blk_y2 = (short) (pixP.posY >> 2);
            blk_x2 = (short) (pixP.posX >> 2);

            {
              sPicMotionParam *mv_info_p = &p->mvInfo[blk_y ][blk_x ];
              sPicMotionParam *mv_info_q = &p->mvInfo[blk_y2][blk_x2];
              sPicturePtr ref_p0 = mv_info_p->refPic[LIST_0];
              sPicturePtr ref_q0 = mv_info_q->refPic[LIST_0];
              sPicturePtr ref_p1 = mv_info_p->refPic[LIST_1];
              sPicturePtr ref_q1 = mv_info_q->refPic[LIST_1];

              if (((ref_p0==ref_q0) && (ref_p1==ref_q1)) ||
                ((ref_p0==ref_q1) && (ref_p1==ref_q0))) {
                StrValue = 0;
                // L0 and L1 reference pictures of p0 are different; q0 as well
                if (ref_p0 != ref_p1) {
                  // compare MV for the same reference picture
                  if (ref_p0==ref_q0) {
                    StrValue =  (byte) (
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit));
                    }
                  else {
                    StrValue =  (byte) (
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit));
                    }
                  }
                else {
                  // L0 and L1 reference pictures of p0 are the same; q0 as well
                  StrValue = (byte) ((
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
static void set_loop_filter_functions_mbaff (sDecoder* decoder) {

  decoder->edgeLoopLumaV = edge_loop_luma_ver_MBAff;
  decoder->edgeLoopLumaH = edge_loop_luma_hor_MBAff;
  decoder->edgeLoopChromaV = edge_loop_chroma_ver_MBAff;
  decoder->edgeLoopChromaH = edge_loop_chroma_hor_MBAff;
  }
//}}}

// normal
//{{{
static void get_strength_ver (sMacroblock* mb, int edge, int mvLimit, sPicture* p)
{
  byte *Strength = mb->strengthV[edge];
  sSlice* curSlice = mb->slice;
  int     StrValue, i;
  sBlockPos *picPos = mb->decoder->picPos;

  if ((curSlice->sliceType==SP_SLICE)||(curSlice->sliceType==SI_SLICE) ) {
    // Set strength to either 3 or 4 regardless of pixel position
    StrValue = (edge == 0) ? 4 : 3;
    for (i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
  }
  else {
    if (mb->isIntraBlock == FALSE) {
      sMacroblock *MbP;
      int xQ = (edge << 2) - 1;
      sMacroblock *neighbour = get_non_aff_neighbour_luma (mb, xQ, 0);
      MbP = (edge) ? mb : neighbour;

      if (edge || MbP->isIntraBlock == FALSE) {
        if (edge && (curSlice->sliceType == P_SLICE && mb->mbType == PSKIP))
          for (i = 0; i < BLOCK_SIZE; i ++ )
            Strength[i] = 0;
        else  if (edge && ((mb->mbType == P16x16)  || (mb->mbType == P16x8))) {
          int      blkP, blkQ, idx;
          for (idx = 0 ; idx < MB_BLOCK_SIZE ; idx += BLOCK_SIZE ) {
            blkQ = idx + (edge);
            blkP = idx + (get_x_luma(xQ) >> 2);
            if ((mb->cbpStructure[0].blk & (i64_power2(blkQ) | i64_power2(blkP))) != 0)
              StrValue = 2;
            else
              StrValue = 0; // if internal edge of certain types, then we already know StrValue should be 0

            Strength[idx >> 2] = StrValue;
          }
        }
        else {
          int blkP, blkQ, idx;
          sBlockPos blockPos = picPos[ mb->mbAddrX ];
          blockPos.x <<= BLOCK_SHIFT;
          blockPos.y <<= BLOCK_SHIFT;

          for (idx = 0 ; idx < MB_BLOCK_SIZE ; idx += BLOCK_SIZE ) {
            blkQ = idx  + (edge);
            blkP = idx  + (get_x_luma(xQ) >> 2);
            if (((mb->cbpStructure[0].blk & i64_power2(blkQ)) != 0) || ((MbP->cbpStructure[0].blk & i64_power2(blkP)) != 0))
              StrValue = 2;
            else {
              // for everything else, if no coefs, but vector difference >= 1 set Strength=1
              int blk_y  = blockPos.y + (blkQ >> 2);
              int blk_x  = blockPos.x + (blkQ  & 3);
              int blk_y2 = (short)(get_pos_y_luma(neighbour,  0) + idx) >> 2;
              int blk_x2 = (short)(get_pos_x_luma(neighbour, xQ)      ) >> 2;
              sPicMotionParam *mv_info_p = &p->mvInfo[blk_y ][blk_x ];
              sPicMotionParam *mv_info_q = &p->mvInfo[blk_y2][blk_x2];
              sPicturePtr ref_p0 = mv_info_p->refPic[LIST_0];
              sPicturePtr ref_q0 = mv_info_q->refPic[LIST_0];
              sPicturePtr ref_p1 = mv_info_p->refPic[LIST_1];
              sPicturePtr ref_q1 = mv_info_q->refPic[LIST_1];

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
static void get_strength_hor (sMacroblock* mb, int edge, int mvLimit, sPicture* p) {

  byte  *Strength = mb->strengthH[edge];
  int    StrValue, i;
  sSlice* curSlice = mb->slice;
  sBlockPos *picPos = mb->decoder->picPos;

  if ((curSlice->sliceType==SP_SLICE)||(curSlice->sliceType==SI_SLICE) ) {
    // Set strength to either 3 or 4 regardless of pixel position
    StrValue = (edge == 0 && (((p->structure==FRAME)))) ? 4 : 3;
    for (i = 0; i < BLOCK_SIZE; i++)
      Strength[i] = StrValue;
    }
  else {
    if (mb->isIntraBlock == FALSE) {
      sMacroblock* MbP;
      int yQ = (edge < BLOCK_SIZE ? (edge << 2) - 1: 0);
      sMacroblock* neighbor = get_non_aff_neighbour_luma(mb, 0, yQ);
      MbP = (edge) ? mb : neighbor;
      if (edge || MbP->isIntraBlock == FALSE) {
        if (edge && (curSlice->sliceType == P_SLICE && mb->mbType == PSKIP))
          for (i = 0; i < BLOCK_SIZE; i ++)
            Strength[i] = 0;
        else if (edge && ((mb->mbType == P16x16)  || (mb->mbType == P8x16))) {
          int blkP, blkQ, idx;
          for (idx = 0 ; idx < BLOCK_SIZE ; idx ++ ) {
            blkQ = (yQ + 1) + idx;
            blkP = (get_y_luma(yQ) & 0xFFFC) + idx;
            if ((mb->cbpStructure[0].blk & (i64_power2(blkQ) | i64_power2(blkP))) != 0)
              StrValue = 2;
            else
              StrValue = 0; // if internal edge of certain types, we already know StrValue should be 0
            Strength[idx] = StrValue;
            }
          }
        else {
          int      blkP, blkQ, idx;
          sBlockPos blockPos = picPos[ mb->mbAddrX ];
          blockPos.x <<= 2;
          blockPos.y <<= 2;

          for (idx = 0 ; idx < BLOCK_SIZE ; idx ++) {
            blkQ = (yQ + 1) + idx;
            blkP = (get_y_luma(yQ) & 0xFFFC) + idx;
            if (((mb->cbpStructure[0].blk & i64_power2(blkQ)) != 0) || ((MbP->cbpStructure[0].blk & i64_power2(blkP)) != 0))
              StrValue = 2;
            else {
              // for everything else, if no coefs, but vector difference >= 1 set Strength=1
              int blk_y = blockPos.y + (blkQ >> 2);
              int blk_x = blockPos.x + (blkQ  & 3);
              int blk_y2 = get_pos_y_luma(neighbor,yQ) >> 2;
              int blk_x2 = ((short)(get_pos_x_luma(neighbor,0)) >> 2) + idx;

              sPicMotionParam* mv_info_p = &p->mvInfo[blk_y ][blk_x ];
              sPicMotionParam* mv_info_q = &p->mvInfo[blk_y2][blk_x2];

              sPicturePtr ref_p0 = mv_info_p->refPic[LIST_0];
              sPicturePtr ref_q0 = mv_info_q->refPic[LIST_0];
              sPicturePtr ref_p1 = mv_info_p->refPic[LIST_1];
              sPicturePtr ref_q1 = mv_info_q->refPic[LIST_1];

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
        StrValue = (edge == 0 && (p->structure == FRAME)) ? 4 : 3;
        for (i = 0; i < BLOCK_SIZE; i ++ )
          Strength[i] = StrValue;
        }
      }
    else {
      // Start with Strength=3. or Strength=4 for Mb-edge
      StrValue = (edge == 0 && (p->structure == FRAME)) ? 4 : 3;
      for (i = 0; i < BLOCK_SIZE; i ++ )
        Strength[i] = StrValue;
       }
    }
  }
//}}}
//{{{
static void luma_ver_deblock_strong (sPixel** curPixel, int pos_x1, int Alpha, int Beta) {

  int i;
  for (i = 0 ; i < BLOCK_SIZE ; ++i )
  {
    sPixel *SrcPtrP = *(curPixel++) + pos_x1;
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
static void luma_ver_deblock_normal (sPixel** curPixel, int pos_x1,
                                     int Alpha, int Beta, int C0, int max_imgpel_value) {

  int i;
  sPixel *SrcPtrP, *SrcPtrQ;
  int edge_diff;

  if (C0 == 0) {
    for (i= 0 ; i < BLOCK_SIZE ; ++i ) {
      SrcPtrP = *(curPixel++) + pos_x1;
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
      SrcPtrP = *(curPixel++) + pos_x1;
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
static void edge_loop_luma_ver (eColorPlane pl, sPixel** img, byte* Strength, sMacroblock* mb, int edge) {

  sDecoder* decoder = mb->decoder;

  sMacroblock* MbP = get_non_aff_neighbour_luma (mb, edge - 1, 0);
  if (MbP || (mb->DFDisableIdc == 0)) {
    int bitdepth_scale   = pl ? decoder->bitdepth_scale[IS_CHROMA] : decoder->bitdepth_scale[IS_LUMA];

    // Average QP of the two blocks
    int QP = pl? ((MbP->qpc[pl-1] + mb->qpc[pl-1] + 1) >> 1) : (MbP->qp + mb->qp + 1) >> 1;
    int indexA = iClip3 (0, MAX_QP, QP + mb->DFAlphaC0Offset);
    int indexB = iClip3 (0, MAX_QP, QP + mb->DFBetaOffset);

    int Alpha = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta = BETA_TABLE [indexB] * bitdepth_scale;
    if ((Alpha | Beta )!= 0) {
      const byte *ClipTab = CLIP_TAB[indexA];
      int max_imgpel_value = decoder->maxPelValueComp[pl];
      int pos_x1 = get_pos_x_luma(MbP, (edge - 1));
      sPixel** curPixel = &img[get_pos_y_luma(MbP, 0)];
      for (int pel = 0 ; pel < MB_BLOCK_SIZE ; pel += 4 ) {
        if (*Strength == 4) // INTRA strong filtering
          luma_ver_deblock_strong (curPixel, pos_x1, Alpha, Beta);
        else if (*Strength != 0) // normal filtering
          luma_ver_deblock_normal (curPixel, pos_x1, Alpha, Beta, ClipTab[ *Strength ] * bitdepth_scale, max_imgpel_value);
        curPixel += 4;
        Strength ++;
        }
      }
    }
  }
//}}}
//{{{
static void luma_hor_deblock_strong (sPixel* imgP, sPixel* imgQ, int width, int Alpha, int Beta) {

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
static void luma_hor_deblock_normal (sPixel* imgP, sPixel* imgQ, int width,
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
static void edge_loop_luma_hor (eColorPlane pl, sPixel** img, byte* Strength,
                                sMacroblock* mb, int edge, sPicture *p) {

  sDecoder* decoder = mb->decoder;

  int ypos = (edge < MB_BLOCK_SIZE ? edge - 1: 0);
  sMacroblock *MbP = get_non_aff_neighbour_luma (mb, 0, ypos);

  if (MbP || (mb->DFDisableIdc == 0)) {
    int bitdepth_scale = pl ? decoder->bitdepth_scale[IS_CHROMA] : decoder->bitdepth_scale[IS_LUMA];

    // Average QP of the two blocks
    int QP = pl? ((MbP->qpc[pl-1] + mb->qpc[pl-1] + 1) >> 1) : (MbP->qp + mb->qp + 1) >> 1;

    int indexA = iClip3 (0, MAX_QP, QP + mb->DFAlphaC0Offset);
    int indexB = iClip3 (0, MAX_QP, QP + mb->DFBetaOffset);

    int Alpha = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta = BETA_TABLE [indexB] * bitdepth_scale;

    if ((Alpha | Beta) != 0) {
      const byte* ClipTab = CLIP_TAB[indexA];
      int max_imgpel_value = decoder->maxPelValueComp[pl];
      int width = p->iLumaStride;

      sPixel* imgP = &img[get_pos_y_luma(MbP, ypos)][get_pos_x_luma(MbP, 0)];
      sPixel* imgQ = imgP + width;
      for (int pel = 0 ; pel < BLOCK_SIZE ; pel++ ) {
        if (*Strength == 4)  // INTRA strong filtering
          luma_hor_deblock_strong (imgP, imgQ, width, Alpha, Beta);
        else if (*Strength != 0) // normal filtering
          luma_hor_deblock_normal (imgP, imgQ, width, Alpha, Beta, ClipTab[ *Strength ] * bitdepth_scale, max_imgpel_value);
        imgP += 4;
        imgQ += 4;
        Strength ++;
      }
    }
  }
}
//}}}
//{{{
static void edge_loop_chroma_ver (sPixel** img, byte* Strength, sMacroblock* mb,
                                  int edge, int uv, sPicture *p) {

  sDecoder* decoder = mb->decoder;

  int block_width  = decoder->mbCrSizeX;
  int block_height = decoder->mbCrSizeY;
  int xQ = edge - 1;
  int yQ = 0;

  sMacroblock* MbP = get_non_aff_neighbour_chroma (mb,xQ,yQ,block_width,block_height);

  if (MbP || (mb->DFDisableIdc == 0)) {
    int bitdepth_scale   = decoder->bitdepth_scale[IS_CHROMA];
    int max_imgpel_value = decoder->maxPelValueComp[uv + 1];
    int AlphaC0Offset = mb->DFAlphaC0Offset;
    int BetaOffset = mb->DFBetaOffset;

    // Average QP of the two blocks
    int QP = (MbP->qpc[uv] + mb->qpc[uv] + 1) >> 1;
    int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + BetaOffset);
    int Alpha   = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta    = BETA_TABLE [indexB] * bitdepth_scale;
    if ((Alpha | Beta) != 0) {
      const int PelNum = pelnum_cr[0][p->chromaFormatIdc];
      const byte* ClipTab = CLIP_TAB[indexA];
      int pos_x1 = get_pos_x_chroma(MbP, xQ, (block_width - 1));
      sPixel** curPixel = &img[get_pos_y_chroma(MbP,yQ, (block_height - 1))];
      for (int pel = 0 ; pel < PelNum ; ++pel ) {
        int Strng = Strength[(PelNum == 8) ? (pel >> 1) : (pel >> 2)];
        if (Strng != 0) {
          sPixel* SrcPtrP = *curPixel + pos_x1;
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
                  int tc0  = ClipTab[ Strng ] * bitdepth_scale + 1;
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
        curPixel++;
        }
      }
    }
  }
//}}}
//{{{
static void edge_loop_chroma_hor (sPixel** img, byte* Strength, sMacroblock* mb,
                                  int edge, int uv, sPicture *p) {

  sDecoder* decoder = mb->decoder;
  int block_width = decoder->mbCrSizeX;
  int block_height = decoder->mbCrSizeY;
  int xQ = 0;
  int yQ = (edge < 16 ? edge - 1: 0);

  sMacroblock* MbP = get_non_aff_neighbour_chroma (mb, xQ, yQ, block_width, block_height);

  if (MbP || (mb->DFDisableIdc == 0)) {
    int bitdepth_scale = decoder->bitdepth_scale[IS_CHROMA];
    int max_imgpel_value = decoder->maxPelValueComp[uv + 1];

    int AlphaC0Offset = mb->DFAlphaC0Offset;
    int BetaOffset = mb->DFBetaOffset;
    int width = p->iChromaStride; //p->sizeXcr;

    // Average QP of the two blocks
    int QP = (MbP->qpc[uv] + mb->qpc[uv] + 1) >> 1;
    int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + BetaOffset);
    int Alpha = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta = BETA_TABLE [indexB] * bitdepth_scale;
    if ((Alpha | Beta) != 0) {
      const int PelNum = pelnum_cr[1][p->chromaFormatIdc];
      const byte* ClipTab = CLIP_TAB[indexA];
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
                  int tc0  = ClipTab[ Strng ] * bitdepth_scale + 1;
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
static void set_loop_filter_functions_normal (sDecoder* decoder) {

  decoder->getStrengthV    = get_strength_ver;
  decoder->getStrengthH    = get_strength_hor;
  decoder->edgeLoopLumaV   = edge_loop_luma_ver;
  decoder->edgeLoopLumaH   = edge_loop_luma_hor;
  decoder->edgeLoopChromaV = edge_loop_chroma_ver;
  decoder->edgeLoopChromaH = edge_loop_chroma_hor;
  }
//}}}

// loopfilter
//{{{
static void deblockMb (sDecoder* decoder, sPicture* p, int mbQAddr) {

  sMacroblock* mb = &(decoder->mbData[mbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (mb->DFDisableIdc == 1)
    mb->DeblockCall = 0;

  else {
    int           edge;
    byte Strength[16];
    short         mb_x, mb_y;
    int           filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
    int           filterLeftMbEdgeFlag;
    int           filterTopMbEdgeFlag;
    int           edge_cr;

    sPixel    ** imgY = p->imgY;
    sPixel  ** *imgUV = p->imgUV;
    sSlice * curSlice = mb->slice;
    int       mvLimit = ((p->structure!=FRAME) || (p->mbAffFrameFlag && mb->mbField)) ? 2 : 4;
    sSPS* activeSPS = decoder->activeSPS;

    mb->DeblockCall = 1;
    get_mb_pos (decoder, mbQAddr, decoder->mbSize[IS_LUMA], &mb_x, &mb_y);
    if (mb->mbType == I8MB)
      assert(mb->lumaTransformSize8x8flag);

    filterNon8x8LumaEdgesFlag[1] =
      filterNon8x8LumaEdgesFlag[3] = !(mb->lumaTransformSize8x8flag);

    filterLeftMbEdgeFlag = (mb_x != 0);
    filterTopMbEdgeFlag  = (mb_y != 0);

    if (p->mbAffFrameFlag && mb_y == MB_BLOCK_SIZE && mb->mbField)
      filterTopMbEdgeFlag = 0;

    if (mb->DFDisableIdc==2) {
      // don't filter at slice boundaries
      filterLeftMbEdgeFlag = mb->mbAvailA;
      // if this the bottom of a frame macroblock pair then always filter the top edge
      filterTopMbEdgeFlag  = (p->mbAffFrameFlag && !mb->mbField && (mbQAddr & 0x01)) ? 1 : mb->mbAvailB;
      }

    if (p->mbAffFrameFlag == 1)
      CheckAvailabilityOfNeighborsMBAFF (mb);

    // Vertical deblocking
    for (edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (mb->cbp == 0 && (curSlice->sliceType == P_SLICE || curSlice->sliceType == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSPS->chromaFormatIdc != YUV444)
          continue;
        else if (edge > 0) {
          if (((mb->mbType == PSKIP && curSlice->sliceType == P_SLICE) || (mb->mbType == P16x16) || (mb->mbType == P16x8)))
            continue;
          else if ((edge & 0x01) && ((mb->mbType == P8x16) || (curSlice->sliceType == B_SLICE && mb->mbType == BSKIP_DIRECT && activeSPS->direct_8x8_inference_flag)))
            continue;
          }
        }

      if (edge || filterLeftMbEdgeFlag ) {
        // Strength for 4 blks in 1 stripe
        get_strength_ver_MBAff (Strength, mb, edge << 2, mvLimit, p);

        if  (Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] !=0 ||
        Strength[4] != 0 || Strength[5] != 0 || Strength[6] != 0 || Strength[7] !=0 ||
        Strength[8] != 0 || Strength[9] != 0 || Strength[10] != 0 || Strength[11] !=0 ||
        Strength[12] != 0 || Strength[13] != 0 || Strength[14] != 0 || Strength[15] !=0 ) {
          // only if one of the 16 Strength bytes is != 0
          if (filterNon8x8LumaEdgesFlag[edge]) {
            decoder->edgeLoopLumaV (PLANE_Y, imgY, Strength, mb, edge << 2);
            if (curSlice->chroma444notSeparate) {
              decoder->edgeLoopLumaV(PLANE_U, imgUV[0], Strength, mb, edge << 2);
              decoder->edgeLoopLumaV(PLANE_V, imgUV[1], Strength, mb, edge << 2);
              }
            }
          if (activeSPS->chromaFormatIdc==YUV420 || activeSPS->chromaFormatIdc==YUV422) {
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
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (mb->cbp == 0 && (curSlice->sliceType == P_SLICE || curSlice->sliceType == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSPS->chromaFormatIdc==YUV420)
          continue;
        else if (edge > 0) {
          if (((mb->mbType == PSKIP && curSlice->sliceType == P_SLICE) || (mb->mbType == P16x16) || (mb->mbType == P8x16)))
            continue;
          else if ((edge & 0x01) && ((mb->mbType == P16x8) || (curSlice->sliceType == B_SLICE && mb->mbType == BSKIP_DIRECT && activeSPS->direct_8x8_inference_flag)))
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
             if(curSlice->chroma444notSeparate) {
              decoder->edgeLoopLumaH(PLANE_U, imgUV[0], Strength, mb, edge << 2, p);
              decoder->edgeLoopLumaH(PLANE_V, imgUV[1], Strength, mb, edge << 2, p);
              }
            }

          if (activeSPS->chromaFormatIdc==YUV420 || activeSPS->chromaFormatIdc==YUV422) {
            edge_cr = chroma_edge[1][edge][p->chromaFormatIdc];
            if ((imgUV != NULL) && (edge_cr >= 0)) {
              decoder->edgeLoopChromaH (imgUV[0], Strength, mb, edge_cr, 0, p);
              decoder->edgeLoopChromaH (imgUV[1], Strength, mb, edge_cr, 1, p);
              }
            }
          }

        if (!edge && !mb->mbField && mb->mixedModeEdgeFlag) {
          //curSlice->mixedModeEdgeFlag)
          // this is the extra horizontal edge between a frame macroblock pair and a field above it
          mb->DeblockCall = 2;
          get_strength_hor_MBAff (Strength, mb, MB_BLOCK_SIZE, mvLimit, p); // Strength for 4 blks in 1 stripe

          {
            if (filterNon8x8LumaEdgesFlag[edge]) {
              decoder->edgeLoopLumaH(PLANE_Y, imgY, Strength, mb, MB_BLOCK_SIZE, p) ;
              if(curSlice->chroma444notSeparate) {
                decoder->edgeLoopLumaH(PLANE_U, imgUV[0], Strength, mb, MB_BLOCK_SIZE, p) ;
                decoder->edgeLoopLumaH(PLANE_V, imgUV[1], Strength, mb, MB_BLOCK_SIZE, p) ;
                }
              }

            if (activeSPS->chromaFormatIdc==YUV420 || activeSPS->chromaFormatIdc==YUV422) {
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
      }//end edge

    mb->DeblockCall = 0;
    }
  }
//}}}
//{{{
static void getDeblockStrength (sDecoder* decoder, sPicture* p, int mbQAddr) {

  sMacroblock* mb = &(decoder->mbData[mbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (mb->DFDisableIdc == 1)
    mb->DeblockCall = 0;

  else {
    int           edge;
    short         mb_x, mb_y;
    int           filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
    int           filterLeftMbEdgeFlag;
    int           filterTopMbEdgeFlag;

    sSlice * curSlice = mb->slice;
    int       mvLimit = ((p->structure!=FRAME) || (p->mbAffFrameFlag && mb->mbField)) ? 2 : 4;
    sSPS *activeSPS = decoder->activeSPS;

    mb->DeblockCall = 1;
    get_mb_pos (decoder, mbQAddr, decoder->mbSize[IS_LUMA], &mb_x, &mb_y);

    if (mb->mbType == I8MB)
      assert(mb->lumaTransformSize8x8flag);

    filterNon8x8LumaEdgesFlag[1] =
      filterNon8x8LumaEdgesFlag[3] = !(mb->lumaTransformSize8x8flag);

    filterLeftMbEdgeFlag = (mb_x != 0);
    filterTopMbEdgeFlag  = (mb_y != 0);

    if (p->mbAffFrameFlag && mb_y == MB_BLOCK_SIZE && mb->mbField)
      filterTopMbEdgeFlag = 0;

    if (mb->DFDisableIdc==2) {
      // don't filter at slice boundaries
      filterLeftMbEdgeFlag = mb->mbAvailA;
      // if this the bottom of a frame macroblock pair then always filter the top edge
      filterTopMbEdgeFlag = (p->mbAffFrameFlag && !mb->mbField && (mbQAddr & 0x01)) ? 1 : mb->mbAvailB;
      }

    if (p->mbAffFrameFlag == 1)
      CheckAvailabilityOfNeighborsMBAFF (mb);

    // Vertical deblocking
    for (edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (mb->cbp == 0 && (curSlice->sliceType == P_SLICE || curSlice->sliceType == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSPS->chromaFormatIdc != YUV444)
          continue;
        else if (edge > 0) {
          if (((mb->mbType == PSKIP && curSlice->sliceType == P_SLICE) ||
               (mb->mbType == P16x16) ||
               (mb->mbType == P16x8)))
            continue;
          else if ((edge & 0x01) &&
                   ((mb->mbType == P8x16) ||
                   (curSlice->sliceType == B_SLICE &&
                    mb->mbType == BSKIP_DIRECT && activeSPS->direct_8x8_inference_flag)))
            continue;
          }
        }

      if (edge || filterLeftMbEdgeFlag )
        // Strength for 4 blks in 1 stripe
        decoder->getStrengthV (mb, edge, mvLimit, p);
      }//end edge

    // horizontal deblocking
    for (edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (mb->cbp == 0 && (curSlice->sliceType == P_SLICE || curSlice->sliceType == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSPS->chromaFormatIdc==YUV420)
          continue;
        else if (edge > 0) {
          if (((mb->mbType == PSKIP && curSlice->sliceType == P_SLICE) ||
               (mb->mbType == P16x16) ||
               (mb->mbType == P8x16)))
            continue;
          else if ((edge & 0x01) &&
                   ((mb->mbType == P16x8) ||
                    (curSlice->sliceType == B_SLICE &&
                     mb->mbType == BSKIP_DIRECT &&
                     activeSPS->direct_8x8_inference_flag)))
            continue;
          }
        }

      if (edge || filterTopMbEdgeFlag )
        decoder->getStrengthH (mb, edge, mvLimit, p);
      }//end edge

    mb->DeblockCall = 0;
    }
  }
//}}}
//{{{
static void performDeblock (sDecoder* decoder, sPicture* p, int mbQAddr) {

  sMacroblock* mb = &(decoder->mbData[mbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (mb->DFDisableIdc == 1)
    mb->DeblockCall = 0;

  else {
    int           edge;
    short         mb_x, mb_y;
    int           filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
    int           filterLeftMbEdgeFlag;
    int           filterTopMbEdgeFlag;
    int           edge_cr;
    sPixel    ** imgY = p->imgY;
    sPixel  ** *imgUV = p->imgUV;
    sSlice * curSlice = mb->slice;
    int       mvLimit = ((p->structure!=FRAME) || (p->mbAffFrameFlag && mb->mbField)) ? 2 : 4;
    sSPS *activeSPS = decoder->activeSPS;

    mb->DeblockCall = 1;
    get_mb_pos (decoder, mbQAddr, decoder->mbSize[IS_LUMA], &mb_x, &mb_y);

    if (mb->mbType == I8MB)
      assert(mb->lumaTransformSize8x8flag);

    filterNon8x8LumaEdgesFlag[1] =
      filterNon8x8LumaEdgesFlag[3] = !(mb->lumaTransformSize8x8flag);

    filterLeftMbEdgeFlag = (mb_x != 0);
    filterTopMbEdgeFlag  = (mb_y != 0);

    if (p->mbAffFrameFlag && mb_y == MB_BLOCK_SIZE && mb->mbField)
      filterTopMbEdgeFlag = 0;

    if (mb->DFDisableIdc==2) {
      // don't filter at slice boundaries
      filterLeftMbEdgeFlag = mb->mbAvailA;
      // if this the bottom of a frame macroblock pair then always filter the top edge
      filterTopMbEdgeFlag  = (p->mbAffFrameFlag && !mb->mbField && (mbQAddr & 0x01)) ? 1 : mb->mbAvailB;
      }

    if (p->mbAffFrameFlag == 1)
      CheckAvailabilityOfNeighborsMBAFF(mb);

    // Vertical deblocking
    for (edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (mb->cbp == 0 && (curSlice->sliceType == P_SLICE || curSlice->sliceType == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSPS->chromaFormatIdc != YUV444)
          continue;
        else if (edge > 0) {
          if (((mb->mbType == PSKIP && curSlice->sliceType == P_SLICE) || (mb->mbType == P16x16) || (mb->mbType == P16x8)))
            continue;
          else if ((edge & 0x01) && ((mb->mbType == P8x16) || (curSlice->sliceType == B_SLICE && mb->mbType == BSKIP_DIRECT && activeSPS->direct_8x8_inference_flag)))
            continue;
          }
        }

      if (edge || filterLeftMbEdgeFlag ) {
        byte *Strength = mb->strengthV[edge];
        if  (Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) {
          // only if one of the 4 first Strength bytes is != 0
          if (filterNon8x8LumaEdgesFlag[edge]) {
            decoder->edgeLoopLumaV (PLANE_Y, imgY, Strength, mb, edge << 2);
            if(curSlice->chroma444notSeparate) {
              decoder->edgeLoopLumaV(PLANE_U, imgUV[0], Strength, mb, edge << 2);
              decoder->edgeLoopLumaV(PLANE_V, imgUV[1], Strength, mb, edge << 2);
              }
            }
          if (activeSPS->chromaFormatIdc==YUV420 || activeSPS->chromaFormatIdc==YUV422) {
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
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (mb->cbp == 0 && (curSlice->sliceType == P_SLICE || curSlice->sliceType == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && activeSPS->chromaFormatIdc==YUV420)
          continue;
        else if (edge > 0) {
          if (((mb->mbType == PSKIP && curSlice->sliceType == P_SLICE) || (mb->mbType == P16x16) || (mb->mbType == P8x16)))
            continue;
          else if ((edge & 0x01) && ((mb->mbType == P16x8) || (curSlice->sliceType == B_SLICE && mb->mbType == BSKIP_DIRECT && activeSPS->direct_8x8_inference_flag)))
            continue;
          }
        }

      if (edge || filterTopMbEdgeFlag ) {
        byte* Strength = mb->strengthH[edge];
        if (Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] !=0 ||
            Strength[4] != 0 || Strength[5] != 0 || Strength[6] != 0 || Strength[7] !=0 ||
            Strength[8] != 0 || Strength[9] != 0 || Strength[10] != 0 || Strength[11] != 0 ||
            Strength[12] != 0 || Strength[13] != 0 || Strength[14] != 0 || Strength[15] !=0 ) {
          // only if one of the 16 Strength bytes is != 0
          if (filterNon8x8LumaEdgesFlag[edge]) {
            decoder->edgeLoopLumaH (PLANE_Y, imgY, Strength, mb, edge << 2, p) ;
            if(curSlice->chroma444notSeparate) {
              decoder->edgeLoopLumaH (PLANE_U, imgUV[0], Strength, mb, edge << 2, p);
              decoder->edgeLoopLumaH (PLANE_V, imgUV[1], Strength, mb, edge << 2, p);
              }
            }

          if (activeSPS->chromaFormatIdc==YUV420 || activeSPS->chromaFormatIdc==YUV422) {
            edge_cr = chroma_edge[1][edge][p->chromaFormatIdc];
            if ((imgUV != NULL) && (edge_cr >= 0)) {
              decoder->edgeLoopChromaH (imgUV[0], Strength, mb, edge_cr, 0, p);
              decoder->edgeLoopChromaH (imgUV[1], Strength, mb, edge_cr, 1, p);
              }
            }
          }

        if (!edge && !mb->mbField && mb->mixedModeEdgeFlag) {
          //curSlice->mixedModeEdgeFlag)
          // this is the extra horizontal edge between a frame macroblock pair and a field above it
          mb->DeblockCall = 2;
          decoder->getStrengthH (mb, 4, mvLimit, p); // Strength for 4 blks in 1 stripe

          {
            if (filterNon8x8LumaEdgesFlag[edge]) {
              decoder->edgeLoopLumaH(PLANE_Y, imgY, Strength, mb, MB_BLOCK_SIZE, p) ;
              if(curSlice->chroma444notSeparate) {
                decoder->edgeLoopLumaH (PLANE_U, imgUV[0], Strength, mb, MB_BLOCK_SIZE, p) ;
                decoder->edgeLoopLumaH (PLANE_V, imgUV[1], Strength, mb, MB_BLOCK_SIZE, p) ;
                }
              }

            if (activeSPS->chromaFormatIdc==YUV420 || activeSPS->chromaFormatIdc==YUV422) {
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
      }//end edge

    mb->DeblockCall = 0;
    }
  }
//}}}
//{{{
void deblockPicture (sDecoder* decoder, sPicture* p) {

  if (p->mbAffFrameFlag) {
    for (unsigned i = 0; i < p->picSizeInMbs; ++i)
      deblockMb (decoder, p, i) ;
    }
  else {
    for (unsigned i = 0; i < p->picSizeInMbs; ++i)
      getDeblockStrength (decoder, p, i ) ;
    for (unsigned i = 0; i < p->picSizeInMbs; ++i)
      performDeblock (decoder, p, i) ;
    }
  }
//}}}

//{{{
static void initNeighbours (sDecoder* decoder) {

  int width = decoder->PicWidthInMbs;
  int height = decoder->picHeightInMbs;
  int size = decoder->picSizeInMbs;

  // do the top left corner
  sMacroblock* mb = &decoder->mbData[0];
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
//{{{
void initDeblock (sDecoder* decoder, int mbAffFrameFlag) {

  if (decoder->yuvFormat == YUV444 && decoder->sepColourPlaneFlag) {
    changePlaneJV (decoder, PLANE_Y, NULL);
    initNeighbours (gVidParam);

    changePlaneJV (decoder, PLANE_U, NULL);
    initNeighbours (gVidParam);

    changePlaneJV (decoder, PLANE_V, NULL);
    initNeighbours (gVidParam);

    changePlaneJV (decoder, PLANE_Y, NULL);
    }
  else
    initNeighbours (gVidParam);

  if (mbAffFrameFlag == 1)
    set_loop_filter_functions_mbaff (decoder);
  else
    set_loop_filter_functions_normal (decoder);
  }
//}}}
