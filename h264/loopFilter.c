//{{{
/*!
 *************************************************************************************
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
 *************************************************************************************
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
#define get_pos_x_luma(mb,x) (mb->pix_x + (x & 15))
#define get_pos_y_luma(mb,y) (mb->pix_y + (y & 15))
#define get_pos_x_chroma(mb,x,max) (mb->pix_c_x + (x & max))
#define get_pos_y_chroma(mb,y,max) (mb->pix_c_y + (y & max))
//}}}
//{{{
// NOTE: In principle, the alpha and beta tables are calculated with the formulas below
//       Alpha( qp ) = 0.8 * (2^(qp/6)  -  1)
//       Beta ( qp ) = 0.5 * qp  -  7
// The tables actually used have been "hand optimized" though (by Anthony Joch). So, the
// table values might be a little different to formula-generated values. Also, the first
// few values of both tables is set to zero to force the filter off at low qp’s
static const byte ALPHA_TABLE[52]  = {0,0,0,0,0,0,0,0,0,0,0,0,
                                      0,0,0,0,4,4,5,6,
                                      7,8,9,10,12,13,15,17,
                                      20,22,25,28,32,36,40,45,
                                      50,56,63,71,80,90,101,113,
                                      127,144,162,182,203,226,255,255} ;
//}}}
//{{{
static const byte  BETA_TABLE[52]  = {0,0,0,0,0,0,0,0,0,0,0,0,
                                      0,0,0,0,2,2,2,3,
                                      3,3,3, 4, 4, 4, 6, 6,
                                      7, 7, 8, 8, 9, 9,10,10,
                                      11,11,12,12,13,13, 14, 14,
                                      15, 15, 16, 16, 17, 17, 18, 18} ;
//}}}
//{{{
static const byte CLIP_TAB[52][5]  =
{
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
//[dir][edge][yuv_format]
static const char chroma_edge[2][4][4] =
{ { {-4, 0, 0, 0},
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
  {0,8,16,16}, {0,8, 8,16}};  //[dir:0=vert, 1=hor.][yuv_format]
//}}}
//{{{
static inline int compare_mvs (const MotionVector *mv0, const MotionVector *mv1, int mvlimit)
{
  return ((iabs( mv0->mv_x - mv1->mv_x) >= 4) | (iabs( mv0->mv_y - mv1->mv_y) >= mvlimit));
}
//}}}

//{{{
static sMacroblock* get_non_aff_neighbor_luma (sMacroblock *mb, int xN, int yN) {

  if (xN < 0)
    return(mb->mbleft);
  else if (yN < 0)
    return(mb->mbup);
  else
    return(mb);
  }
//}}}
//{{{
static sMacroblock* get_non_aff_neighbor_chroma (sMacroblock *mb, int xN, int yN,
                                                int block_width,int block_height) {
  if (xN < 0) {
    if (yN < block_height)
      return(mb->mbleft);
    else
      return(NULL);
    }
  else if (xN < block_width) {
    if (yN < 0)
      return(mb->mbup);
    else if (yN < block_height)
      return(mb);
    else
      return(NULL);
    }
  else
    return(NULL);
  }
//}}}

// mbaff
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Filters 16 pel block edge of Super MB Frame coded MBs
 *****************************************************************************************
 */
static void edge_loop_luma_ver_MBAff (ColorPlane pl, sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge)
{
  int      pel, Strng ;
  sPixel   L2 = 0, L1, L0, R0, R1, R2 = 0;
  int      Alpha = 0, Beta = 0 ;
  const byte* ClipTab = NULL;
  int      indexA, indexB;

  int      QP;

  PixelPos pixP, pixQ;

  sVidParam* vidParam = MbQ->vidParam;
  int      bitdepth_scale = pl? vidParam->bitdepth_scale[IS_CHROMA] : vidParam->bitdepth_scale[IS_LUMA];
  int      max_imgpel_value = vidParam->max_pel_value_comp[pl];

  int AlphaC0Offset = MbQ->DFAlphaC0Offset;
  int BetaOffset = MbQ->DFBetaOffset;

  sMacroblock *MbP;
  sPixel   *SrcPtrP, *SrcPtrQ;

  for( pel = 0 ; pel < MB_BLOCK_SIZE ; ++pel )
  {
    getAffNeighbour(MbQ, edge - 1, pel, vidParam->mb_size[IS_LUMA], &pixP);

    if ( pixP.available || (MbQ->DFDisableIdc == 0))
    {
      if( (Strng = Strength[pel]) != 0)
      {
        getAffNeighbour(MbQ, edge, pel, vidParam->mb_size[IS_LUMA], &pixQ);

        MbP = &(vidParam->mb_data[pixP.mb_addr]);

        SrcPtrQ = &(Img[pixQ.pos_y][pixQ.pos_x]);
        SrcPtrP = &(Img[pixP.pos_y][pixP.pos_x]);

        // Average QP of the two blocks
        QP = pl? ((MbP->qpc[pl-1] + MbQ->qpc[pl-1] + 1) >> 1) : (MbP->qp + MbQ->qp + 1) >> 1;

        indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
        indexB = iClip3(0, MAX_QP, QP + BetaOffset);

        Alpha   = ALPHA_TABLE[indexA] * bitdepth_scale;
        Beta    = BETA_TABLE [indexB] * bitdepth_scale;
        ClipTab = CLIP_TAB[indexA];

        L0  = SrcPtrP[ 0] ;
        R0  = SrcPtrQ[ 0] ;

        if( iabs( R0 - L0 ) < Alpha )
        {
          L1  = SrcPtrP[-1];
          R1  = SrcPtrQ[ 1];

          if ((iabs( R0 - R1) < Beta )   && (iabs(L0 - L1) < Beta ))
          {
            L2  = SrcPtrP[-2];
            R2  = SrcPtrQ[ 2];
            if(Strng == 4 )    // INTRA strong filtering
            {
              int RL0 = L0 + R0;
              int small_gap = (iabs( R0 - L0 ) < ((Alpha >> 2) + 2));
              int aq  = ( iabs( R0 - R2) < Beta ) & small_gap;
              int ap  = ( iabs( L0 - L2) < Beta ) & small_gap;

              if (ap)
              {
                int L3  = SrcPtrP[-3];
                SrcPtrP[-2 ] = (sPixel) ((((L3 + L2) << 1) + L2 + L1 + RL0 + 4) >> 3);
                SrcPtrP[-1 ] = (sPixel) (( L2 + L1 + L0 + R0 + 2) >> 2);
                SrcPtrP[ 0 ] = (sPixel) (( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3);
              }
              else
              {
                SrcPtrP[ 0 ] = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2) ;
              }

              if (aq)
              {
                sPixel R3  = SrcPtrQ[ 3];
                SrcPtrQ[ 0 ] = (sPixel) (( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3);
                SrcPtrQ[ 1 ] = (sPixel) (( R2 + R0 + R1 + L0 + 2) >> 2);
                SrcPtrQ[ 2 ] = (sPixel) ((((R3 + R2) << 1) + R2 + R1 + RL0 + 4) >> 3);
              }
              else
              {
                SrcPtrQ[ 0 ] = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
              }
            }
            else   // normal filtering
            {
              int RL0 = (L0 + R0 + 1) >> 1;
              int aq  = (iabs( R0 - R2) < Beta);
              int ap  = (iabs( L0 - L2) < Beta);

              int C0  = ClipTab[ Strng ] * bitdepth_scale;
              int tc0  = (C0 + ap + aq) ;
              int dif = iClip3( -tc0, tc0, (((R0 - L0) << 2) + (L1 - R1) + 4) >> 3) ;

              if( ap && (C0 != 0))
                *(SrcPtrP - 1) += iClip3( -C0,  C0, ( L2 + RL0 - (L1 << 1)) >> 1 ) ;

              if (dif)
              {
                *SrcPtrP  = (sPixel) iClip1 (max_imgpel_value, L0 + dif) ;
                *SrcPtrQ  = (sPixel) iClip1 (max_imgpel_value, R0 - dif) ;
              }

              if( aq  && (C0 != 0))
                *(SrcPtrQ + 1) += iClip3( -C0,  C0, ( R2 + RL0 - (R1 << 1)) >> 1 ) ;
            }
          }
        }
      }
    }
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Filters 16 pel block edge of Super MB Frame coded MBs
 *****************************************************************************************
 */
static void edge_loop_luma_hor_MBAff (ColorPlane pl, sPixel** Img, byte *Strength, sMacroblock *MbQ,
              int edge, sPicture *p)
{
  int      width = p->iLumaStride; //p->size_x;
  int      pel, Strng ;
  int      PelNum = pl? pelnum_cr[1][p->chroma_format_idc] : MB_BLOCK_SIZE;

  int      yQ = (edge < MB_BLOCK_SIZE ? edge : 1);

  PixelPos pixP, pixQ;

  sVidParam* vidParam = MbQ->vidParam;
  int      bitdepth_scale = pl? vidParam->bitdepth_scale[IS_CHROMA] : vidParam->bitdepth_scale[IS_LUMA];
  int      max_imgpel_value = vidParam->max_pel_value_comp[pl];

  getAffNeighbour(MbQ, 0, yQ - 1, vidParam->mb_size[IS_LUMA], &pixP);

  if (pixP.available || (MbQ->DFDisableIdc == 0))
  {
    int AlphaC0Offset = MbQ->DFAlphaC0Offset;
    int BetaOffset = MbQ->DFBetaOffset;

    sMacroblock *MbP = &(vidParam->mb_data[pixP.mb_addr]);

    int incQ    = ((MbP->mb_field && !MbQ->mb_field) ? 2 * width : width);
    int incP    = ((MbQ->mb_field && !MbP->mb_field) ? 2 * width : width);

    // Average QP of the two blocks
    int QP = pl? ((MbP->qpc[pl - 1] + MbQ->qpc[pl - 1] + 1) >> 1) : (MbP->qp + MbQ->qp + 1) >> 1;

    int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + BetaOffset);

    int Alpha   = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta    = BETA_TABLE [indexB] * bitdepth_scale;

    if ((Alpha | Beta )!= 0)
    {
      const byte* ClipTab = CLIP_TAB[indexA];
      getAffNeighbour(MbQ, 0, yQ, vidParam->mb_size[IS_LUMA], &pixQ);

      for( pel = 0 ; pel < PelNum ; ++pel )
      {
        if( (Strng = Strength[pel]) != 0)
        {
          sPixel *SrcPtrQ = &(Img[pixQ.pos_y][pixQ.pos_x]);
          sPixel *SrcPtrP = &(Img[pixP.pos_y][pixP.pos_x]);

          sPixel L0  = *SrcPtrP;
          sPixel R0  = *SrcPtrQ;

          if( iabs( R0 - L0 ) < Alpha )
          {
            sPixel L1  = SrcPtrP[-incP];
            sPixel R1  = SrcPtrQ[ incQ];

            if ((iabs( R0 - R1) < Beta )   && (iabs(L0 - L1) < Beta ))
            {
              sPixel L2  = SrcPtrP[-incP*2];
              sPixel R2  = SrcPtrQ[ incQ*2];
              if(Strng == 4 )    // INTRA strong filtering
              {
                int RL0 = L0 + R0;
                int small_gap = (iabs( R0 - L0 ) < ((Alpha >> 2) + 2));
                int aq  = ( iabs( R0 - R2) < Beta ) & small_gap;
                int ap  = ( iabs( L0 - L2) < Beta ) & small_gap;

                if (ap)
                {
                  sPixel L3  = SrcPtrP[-incP*3];
                  SrcPtrP[-incP * 2] = (sPixel) ((((L3 + L2) << 1) + L2 + L1 + RL0 + 4) >> 3);
                  SrcPtrP[-incP    ] = (sPixel) (( L2 + L1 + L0 + R0 + 2) >> 2);
                  SrcPtrP[    0    ] = (sPixel) (( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3);
                }
                else
                {
                  SrcPtrP[     0     ] = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2) ;
                }

                if (aq)
                {
                  sPixel R3 = SrcPtrQ[ incQ*3];
                  SrcPtrQ[    0     ] = (sPixel) (( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3);
                  SrcPtrQ[ incQ     ] = (sPixel) (( R2 + R0 + R1 + L0 + 2) >> 2);
                  SrcPtrQ[ incQ * 2 ] = (sPixel) ((((R3 + R2) << 1) + R2 + R1 + RL0 + 4) >> 3);
                }
                else
                {
                  SrcPtrQ[    0     ] = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
                }
              }
              else   // normal filtering
              {
                int RL0 = (L0 + R0 + 1) >> 1;
                int aq  = (iabs( R0 - R2) < Beta);
                int ap  = (iabs( L0 - L2) < Beta);

                int C0  = ClipTab[ Strng ] * bitdepth_scale;
                int tc0  = (C0 + ap + aq) ;
                int dif = iClip3( -tc0, tc0, (((R0 - L0) << 2) + (L1 - R1) + 4) >> 3) ;

                if( ap && (C0 != 0))
                  *(SrcPtrP - incP) += iClip3( -C0,  C0, ( L2 + RL0 - (L1 << 1)) >> 1 ) ;

                if (dif)
                {
                  *SrcPtrP  = (sPixel) iClip1 (max_imgpel_value, L0 + dif) ;
                  *SrcPtrQ  = (sPixel) iClip1 (max_imgpel_value, R0 - dif) ;
                }

                if( aq  && (C0 != 0))
                  *(SrcPtrQ + incQ) += iClip3( -C0,  C0, ( R2 + RL0 - (R1 << 1)) >> 1 ) ;
              }
            }
          }
        }
        pixP.pos_x++;
        pixQ.pos_x++;
      }
    }
  }
}
//}}}
//{{{
/*!
*****************************************************************************************
* \brief
*    Filters chroma block edge for MBAFF types
*****************************************************************************************
 */
static void edge_loop_chroma_ver_MBAff (sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, int uv, sPicture *p)
{
  int      pel, Strng ;

  sPixel   L1, L0, R0, R1;
  int      Alpha = 0, Beta = 0;
  const byte* ClipTab = NULL;
  int      indexA, indexB;
  sVidParam* vidParam = MbQ->vidParam;
  int      PelNum = pelnum_cr[0][p->chroma_format_idc];
  int      StrengthIdx;
  int      QP;
  int      xQ = edge, yQ;
  PixelPos pixP, pixQ;
  int      bitdepth_scale = vidParam->bitdepth_scale[IS_CHROMA];
  int      max_imgpel_value = vidParam->max_pel_value_comp[uv + 1];

  int      AlphaC0Offset = MbQ->DFAlphaC0Offset;
  int      BetaOffset    = MbQ->DFBetaOffset;
  sMacroblock *MbP;
  sPixel   *SrcPtrP, *SrcPtrQ;


  for( pel = 0 ; pel < PelNum ; ++pel )
  {
    yQ = pel;
    getAffNeighbour(MbQ, xQ, yQ, vidParam->mb_size[IS_CHROMA], &pixQ);
    getAffNeighbour(MbQ, xQ - 1, yQ, vidParam->mb_size[IS_CHROMA], &pixP);
    MbP = &(vidParam->mb_data[pixP.mb_addr]);
    StrengthIdx = (PelNum == 8) ? ((MbQ->mb_field && !MbP->mb_field) ? pel << 1 :((pel >> 1) << 2) + (pel & 0x01)) : pel;

    if (pixP.available || (MbQ->DFDisableIdc == 0))
    {
      if( (Strng = Strength[StrengthIdx]) != 0)
      {
        SrcPtrQ = &(Img[pixQ.pos_y][pixQ.pos_x]);
        SrcPtrP = &(Img[pixP.pos_y][pixP.pos_x]);

        // Average QP of the two blocks
        QP = (MbP->qpc[uv] + MbQ->qpc[uv] + 1) >> 1;

        indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
        indexB = iClip3(0, MAX_QP, QP + BetaOffset);

        Alpha   = ALPHA_TABLE[indexA] * bitdepth_scale;
        Beta    = BETA_TABLE [indexB] * bitdepth_scale;
        ClipTab = CLIP_TAB[indexA];

        L0  = *SrcPtrP;
        R0  = *SrcPtrQ;

        if( iabs( R0 - L0 ) < Alpha )
        {
          L1  = SrcPtrP[-1];
          R1  = SrcPtrQ[ 1];
          if( ((iabs( R0 - R1) - Beta < 0)  && (iabs(L0 - L1) - Beta < 0 ))  )
          {
            if( Strng == 4 )    // INTRA strong filtering
            {
              SrcPtrQ[0] = (sPixel) ( ((R1 << 1) + R0 + L1 + 2) >> 2 );
              SrcPtrP[0] = (sPixel) ( ((L1 << 1) + L0 + R1 + 2) >> 2 );
            }
            else
            {
              int C0  = ClipTab[ Strng ] * bitdepth_scale;
              int tc0  = (C0 + 1);
              int dif = iClip3( -tc0, tc0, ( ((R0 - L0) << 2) + (L1 - R1) + 4) >> 3 );

              if (dif)
              {
                *SrcPtrP = (sPixel) iClip1 ( max_imgpel_value, L0 + dif );
                *SrcPtrQ = (sPixel) iClip1 ( max_imgpel_value, R0 - dif );
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
/*!
*****************************************************************************************
* \brief
*    Filters chroma block edge for MBAFF types
*****************************************************************************************
 */
static void edge_loop_chroma_hor_MBAff (sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, int uv, sPicture *p)
{
  sVidParam* vidParam = MbQ->vidParam;
  int      PelNum = pelnum_cr[1][p->chroma_format_idc];
  int      yQ = (edge < MB_BLOCK_SIZE? edge : 1);
  PixelPos pixP, pixQ;
  int      bitdepth_scale = vidParam->bitdepth_scale[IS_CHROMA];
  int      max_imgpel_value = vidParam->max_pel_value_comp[uv + 1];

  int      AlphaC0Offset = MbQ->DFAlphaC0Offset;
  int      BetaOffset    = MbQ->DFBetaOffset;
  int      width = p->iChromaStride; //p->size_x_cr;

  getAffNeighbour(MbQ, 0, yQ - 1, vidParam->mb_size[IS_CHROMA], &pixP);
  getAffNeighbour(MbQ, 0, yQ, vidParam->mb_size[IS_CHROMA], &pixQ);

  if (pixP.available || (MbQ->DFDisableIdc == 0))
  {
    sMacroblock *MbP = &(vidParam->mb_data[pixP.mb_addr]);

    int incQ = ((MbP->mb_field && !MbQ->mb_field) ? 2 * width : width);
    int incP = ((MbQ->mb_field  && !MbP->mb_field) ? 2 * width : width);

    // Average QP of the two blocks
    int QP = (MbP->qpc[uv] + MbQ->qpc[uv] + 1) >> 1;

    int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + BetaOffset);

    int Alpha   = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta    = BETA_TABLE [indexB] * bitdepth_scale;

    if ((Alpha | Beta )!= 0)
    {
      const byte* ClipTab = CLIP_TAB[indexA];
      int      pel, Strng ;
      int      StrengthIdx;
      for( pel = 0 ; pel < PelNum ; ++pel )
      {
        StrengthIdx = (PelNum == 8) ? ((MbQ->mb_field && !MbP->mb_field) ? pel << 1 :((pel >> 1) << 2) + (pel & 0x01)) : pel;

        if( (Strng = Strength[StrengthIdx]) != 0)
        {
          sPixel *SrcPtrQ = &(Img[pixQ.pos_y][pixQ.pos_x]);
          sPixel *SrcPtrP = &(Img[pixP.pos_y][pixP.pos_x]);

          sPixel L0  = *SrcPtrP;
          sPixel R0  = *SrcPtrQ;

          if( iabs( R0 - L0 ) < Alpha )
          {
            sPixel L1  = SrcPtrP[-incP];
            sPixel R1  = SrcPtrQ[ incQ];
            if( ((iabs( R0 - R1) - Beta < 0)  && (iabs(L0 - L1) - Beta < 0 ))  )
            {
              if( Strng == 4 )    // INTRA strong filtering
              {
                SrcPtrQ[0] = (sPixel) ( ((R1 << 1) + R0 + L1 + 2) >> 2 );
                SrcPtrP[0] = (sPixel) ( ((L1 << 1) + L0 + R1 + 2) >> 2 );
              }
              else
              {
                int C0  = ClipTab[ Strng ] * bitdepth_scale;
                int tc0  = (C0 + 1);
                int dif = iClip3( -tc0, tc0, ( ((R0 - L0) << 2) + (L1 - R1) + 4) >> 3 );

                if (dif)
                {
                  *SrcPtrP = (sPixel) iClip1 ( max_imgpel_value, L0 + dif );
                  *SrcPtrQ = (sPixel) iClip1 ( max_imgpel_value, R0 - dif );
                }
              }
            }
          }
        }
        pixP.pos_x++;
        pixQ.pos_x++;
      }
    }
  }
}
//}}}
//{{{
/*!
 *********************************************************************************************
 * \brief
 *    returns a buffer of 16 Strength values for one stripe in a mb (for MBAFF)
 *********************************************************************************************
 */
static void get_strength_ver_MBAff (byte *Strength, sMacroblock *MbQ, int edge, int mvlimit, sPicture *p)
{
  //byte *Strength = MbQ->strength_ver[edge];
  short  blkP, blkQ, idx;
  //short  blk_x, blk_x2, blk_y, blk_y2 ;

  int    StrValue, i;
  short  mb_x, mb_y;

  sMacroblock *MbP;

  PixelPos pixP;
  sVidParam* vidParam = MbQ->vidParam;
  BlockPos *PicPos = vidParam->PicPos;

  if ((p->slice_type==SP_SLICE)||(p->slice_type==SI_SLICE) )
  {
    for( idx = 0; idx < MB_BLOCK_SIZE; ++idx )
    {
      getAffNeighbour(MbQ, edge - 1, idx, vidParam->mb_size[IS_LUMA], &pixP);
      blkQ = (short) ((idx & 0xFFFC) + (edge >> 2));
      blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));

      MbP = &(vidParam->mb_data[pixP.mb_addr]);
      MbQ->mixedModeEdgeFlag = (byte) (MbQ->mb_field != MbP->mb_field);    //currSlice->mixedModeEdgeFlag = (byte) (MbQ->mb_field != MbP->mb_field);

      Strength[idx] = (edge == 0) ? 4 : 3;
    }
  }
  else
  {
    getAffNeighbour(MbQ, edge - 1, 0, vidParam->mb_size[IS_LUMA], &pixP);

    MbP = &(vidParam->mb_data[pixP.mb_addr]);
    // Neighboring Frame MBs
    if ((MbQ->mb_field == FALSE && MbP->mb_field == FALSE))
    {
      MbQ->mixedModeEdgeFlag = (byte) (MbQ->mb_field != MbP->mb_field);
      if (MbQ->is_intra_block == TRUE || MbP->is_intra_block == TRUE)
      {
        //printf("idx %d %d %d %d %d\n", idx, pixP.x, pixP.y, pixP.pos_x, pixP.pos_y);
        // Start with Strength=3. or Strength=4 for Mb-edge
        StrValue = (edge == 0) ? 4 : 3;
        for( i = 0; i < MB_BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
      }
      else
      {
        get_mb_block_pos_mbaff (PicPos, MbQ->mbAddrX, &mb_x, &mb_y);
        for( idx = 0; idx < MB_BLOCK_SIZE; idx += BLOCK_SIZE)
        {
          blkQ = (short) ((idx & 0xFFFC) + (edge >> 2));
          blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));

          if (((MbQ->s_cbp[0].blk & i64_power2(blkQ)) != 0) || ((MbP->s_cbp[0].blk & i64_power2(blkP)) != 0))
            StrValue = 2;
          else if (edge && ((MbQ->mb_type == 1)  || (MbQ->mb_type == 2)))
            StrValue = 0; // if internal edge of certain types, we already know StrValue should be 0
          else // for everything else, if no coefs, but vector difference >= 1 set Strength=1
          {
            int blk_y  = ((mb_y<<2) + (blkQ >> 2));
            int blk_x  = ((mb_x<<2) + (blkQ  & 3));
            int blk_y2 = (pixP.pos_y >> 2);
            int blk_x2 = (pixP.pos_x >> 2);

            PicMotionParams *mv_info_p = &p->mv_info[blk_y ][blk_x ];
            PicMotionParams *mv_info_q = &p->mv_info[blk_y2][blk_x2];
            StorablePicturePtr ref_p0 = mv_info_p->ref_pic[LIST_0];
            StorablePicturePtr ref_q0 = mv_info_q->ref_pic[LIST_0];
            StorablePicturePtr ref_p1 = mv_info_p->ref_pic[LIST_1];
            StorablePicturePtr ref_q1 = mv_info_q->ref_pic[LIST_1];

            if ( ((ref_p0==ref_q0) && (ref_p1==ref_q1))||((ref_p0==ref_q1) && (ref_p1==ref_q0)))
            {
              // L0 and L1 reference pictures of p0 are different; q0 as well
              if (ref_p0 != ref_p1)
              {
                // compare MV for the same reference picture
                if (ref_p0==ref_q0)
                {
                  StrValue =  (byte) (
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit));
                }
                else
                {
                  StrValue =  (byte) (
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit));
                }
              }
              else
              { // L0 and L1 reference pictures of p0 are the same; q0 as well

                StrValue = (byte) ((
                  compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                  compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit))
                  &&(
                  compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                  compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit)));
              }
            }
            else
            {
              StrValue = 1;
            }
          }
          Strength[idx] = StrValue;
          Strength[idx + 1] = StrValue;
          Strength[idx + 2] = StrValue;
          Strength[idx + 3] = StrValue;
          pixP.y += 4;
          pixP.pos_y += 4;
        }
      }
    }
    else
    {
      for( idx = 0; idx < MB_BLOCK_SIZE; ++idx )
      {
        getAffNeighbour(MbQ, edge - 1, idx, vidParam->mb_size[IS_LUMA], &pixP);
        blkQ = (short) ((idx & 0xFFFC) + (edge >> 2));
        blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));

        MbP = &(vidParam->mb_data[pixP.mb_addr]);
        MbQ->mixedModeEdgeFlag = (byte) (MbQ->mb_field != MbP->mb_field);

        // Start with Strength=3. or Strength=4 for Mb-edge
        Strength[idx] = (edge == 0 && (((!p->mb_aff_frame_flag && (p->structure==FRAME)) ||
          (p->mb_aff_frame_flag && !MbP->mb_field && !MbQ->mb_field)) ||
          ((p->mb_aff_frame_flag || (p->structure!=FRAME))))) ? 4 : 3;

        if (MbQ->is_intra_block == FALSE && MbP->is_intra_block == FALSE)
        {
          if (((MbQ->s_cbp[0].blk & i64_power2(blkQ)) != 0) || ((MbP->s_cbp[0].blk & i64_power2(blkP)) != 0))
            Strength[idx] = 2 ;
          else
          {
            // if no coefs, but vector difference >= 1 set Strength=1
            // if this is a mixed mode edge then one set of reference pictures will be frame and the
            // other will be field
            if(MbQ->mixedModeEdgeFlag) //if (currSlice->mixedModeEdgeFlag)
            {
              Strength[idx] = 1;
            }
            else
            {
              get_mb_block_pos_mbaff (PicPos, MbQ->mbAddrX, &mb_x, &mb_y);
              {
                int blk_y  = ((mb_y<<2) + (blkQ >> 2));
                int blk_x  = ((mb_x<<2) + (blkQ  & 3));
                int blk_y2 = (pixP.pos_y >> 2);
                int blk_x2 = (pixP.pos_x >> 2);

                PicMotionParams *mv_info_p = &p->mv_info[blk_y ][blk_x ];
                PicMotionParams *mv_info_q = &p->mv_info[blk_y2][blk_x2];
                StorablePicturePtr ref_p0 = mv_info_p->ref_pic[LIST_0];
                StorablePicturePtr ref_q0 = mv_info_q->ref_pic[LIST_0];
                StorablePicturePtr ref_p1 = mv_info_p->ref_pic[LIST_1];
                StorablePicturePtr ref_q1 = mv_info_q->ref_pic[LIST_1];

                if ( ((ref_p0==ref_q0) && (ref_p1==ref_q1))||((ref_p0==ref_q1) && (ref_p1==ref_q0)))
                {
                  Strength[idx]=0;
                  // L0 and L1 reference pictures of p0 are different; q0 as well
                  if (ref_p0 != ref_p1)
                  {
                    // compare MV for the same reference picture
                    if (ref_p0==ref_q0)
                    {
                      Strength[idx] =  (byte) (
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit));
                    }
                    else
                    {
                      Strength[idx] =  (byte) (
                        compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                        compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit));
                    }
                  }
                  else
                  { // L0 and L1 reference pictures of p0 are the same; q0 as well

                    Strength[idx] = (byte) ((
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit))
                      &&(
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit)));
                  }
                }
                else
                {
                  Strength[idx] = 1;
                }
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
/*!
 *********************************************************************************************
 * \brief
 *    returns a buffer of 16 Strength values for one stripe in a mb (for MBAFF)
 *********************************************************************************************
 */
static void get_strength_hor_MBAff (byte *Strength, sMacroblock *MbQ, int edge, int mvlimit, sPicture *p)
{
  short  blkP, blkQ, idx;
  short  blk_x, blk_x2, blk_y, blk_y2 ;

  int    StrValue, i;
  int    xQ, yQ = (edge < MB_BLOCK_SIZE ? edge : 1);
  short  mb_x, mb_y;

  sMacroblock *MbP;

  PixelPos pixP;
  sVidParam* vidParam = MbQ->vidParam;
  BlockPos *PicPos = vidParam->PicPos;

  if ((p->slice_type==SP_SLICE)||(p->slice_type==SI_SLICE) )
  {
    for( idx = 0; idx < MB_BLOCK_SIZE; idx += BLOCK_SIZE)
    {
      xQ = idx;
      getAffNeighbour(MbQ, xQ, yQ - 1, vidParam->mb_size[IS_LUMA], &pixP);

      blkQ = (short) ((yQ & 0xFFFC) + (xQ >> 2));
      blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));

      MbP = &(vidParam->mb_data[pixP.mb_addr]);
      MbQ->mixedModeEdgeFlag = (byte) (MbQ->mb_field != MbP->mb_field);

      StrValue = (edge == 0 && (!MbP->mb_field && !MbQ->mb_field)) ? 4 : 3;

      Strength[idx] = StrValue;
      Strength[idx+1] = StrValue;
      Strength[idx+2] = StrValue;
      Strength[idx+3] = StrValue;
    }
  }
  else
  {
    getAffNeighbour(MbQ, 0, yQ - 1, vidParam->mb_size[IS_LUMA], &pixP);
    MbP = &(vidParam->mb_data[pixP.mb_addr]);
    MbQ->mixedModeEdgeFlag = (byte) (MbQ->mb_field != MbP->mb_field);

    // Set intra mode deblocking
    if (MbQ->is_intra_block == TRUE || MbP->is_intra_block == TRUE)
    {
      StrValue = (edge == 0 && (!MbP->mb_field && !MbQ->mb_field)) ? 4 : 3;
      for( i = 0; i < MB_BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
    }
    else
    {
      for( idx = 0; idx < MB_BLOCK_SIZE; idx += BLOCK_SIZE )
      {
        xQ = idx;
        getAffNeighbour(MbQ, xQ, yQ - 1, vidParam->mb_size[IS_LUMA], &pixP);

        blkQ = (short) ((yQ & 0xFFFC) + (xQ >> 2));
        blkP = (short) ((pixP.y & 0xFFFC) + (pixP.x >> 2));

        if (((MbQ->s_cbp[0].blk & i64_power2(blkQ)) != 0) || ((MbP->s_cbp[0].blk & i64_power2(blkP)) != 0))
        {
          StrValue = 2;
        }
        else
        {
          // if no coefs, but vector difference >= 1 set Strength=1
          // if this is a mixed mode edge then one set of reference pictures will be frame and the
          // other will be field
          if(MbQ->mixedModeEdgeFlag) //if (currSlice->mixedModeEdgeFlag)
          {
            StrValue = 1;
          }
          else
          {
            get_mb_block_pos_mbaff (PicPos, MbQ->mbAddrX, &mb_x, &mb_y);
            blk_y  = (short) ((mb_y<<2) + (blkQ >> 2));
            blk_x  = (short) ((mb_x<<2) + (blkQ  & 3));
            blk_y2 = (short) (pixP.pos_y >> 2);
            blk_x2 = (short) (pixP.pos_x >> 2);

            {
              PicMotionParams *mv_info_p = &p->mv_info[blk_y ][blk_x ];
              PicMotionParams *mv_info_q = &p->mv_info[blk_y2][blk_x2];
              StorablePicturePtr ref_p0 = mv_info_p->ref_pic[LIST_0];
              StorablePicturePtr ref_q0 = mv_info_q->ref_pic[LIST_0];
              StorablePicturePtr ref_p1 = mv_info_p->ref_pic[LIST_1];
              StorablePicturePtr ref_q1 = mv_info_q->ref_pic[LIST_1];

              if ( ((ref_p0==ref_q0) && (ref_p1==ref_q1)) ||
                ((ref_p0==ref_q1) && (ref_p1==ref_q0)))
              {
                StrValue = 0;
                // L0 and L1 reference pictures of p0 are different; q0 as well
                if (ref_p0 != ref_p1)
                {
                  // compare MV for the same reference picture
                  if (ref_p0==ref_q0)
                  {
                    StrValue =  (byte) (
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit));
                  }
                  else
                  {
                    StrValue =  (byte) (
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit));
                  }
                }
                else
                { // L0 and L1 reference pictures of p0 are the same; q0 as well
                  StrValue = (byte) ((
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) ||
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit))
                    &&(
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) ||
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit)));
                }
              }
              else
              {
                StrValue = 1;
              }
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
static void get_db_strength_mbaff (sVidParam* vidParam, sPicture *p, int MbQAddr)
{
  sMacroblock   *MbQ = &(vidParam->mb_data[MbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (MbQ->DFDisableIdc == 1)
    MbQ->DeblockCall = 0;

  else {
    int edge;
    short mb_x, mb_y;
    int filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
    int filterLeftMbEdgeFlag;
    int filterTopMbEdgeFlag;

    Slice* currSlice = MbQ->p_Slice;
    int mvlimit = ((p->structure!=FRAME) || (p->mb_aff_frame_flag && MbQ->mb_field)) ? 2 : 4;
    sSPSrbsp *active_sps = vidParam->active_sps;

    MbQ->DeblockCall = 1;
    get_mb_pos (vidParam, MbQAddr, vidParam->mb_size[IS_LUMA], &mb_x, &mb_y);

    if (MbQ->mb_type == I8MB)
      assert(MbQ->luma_transform_size_8x8_flag);

    filterNon8x8LumaEdgesFlag[1] = filterNon8x8LumaEdgesFlag[3] = !(MbQ->luma_transform_size_8x8_flag);
    filterLeftMbEdgeFlag = (mb_x != 0);
    filterTopMbEdgeFlag  = (mb_y != 0);

    if (p->mb_aff_frame_flag && mb_y == MB_BLOCK_SIZE && MbQ->mb_field)
      filterTopMbEdgeFlag = 0;

    if (MbQ->DFDisableIdc==2) {
      // don't filter at slice boundaries
      filterLeftMbEdgeFlag = MbQ->mbAvailA;
      // if this the bottom of a frame macroblock pair then always filter the top edge
      filterTopMbEdgeFlag  = (p->mb_aff_frame_flag && !MbQ->mb_field && (MbQAddr & 0x01)) ? 1 : MbQ->mbAvailB;
      }

    CheckAvailabilityOfNeighborsMBAFF(MbQ);

    // Vertical deblocking
    for (edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc != YUV444)
          continue;
        else if (edge > 0) {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P16x8)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P8x16) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
          }
        }

      if( edge || filterLeftMbEdgeFlag )
        // Strength for 4 blks in 1 stripe
        vidParam->GetStrengthVer(MbQ, edge, mvlimit, p);
      }//end edge

    // horizontal deblocking
    for( edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc==YUV420)
          continue;
        else if (edge > 0) {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P8x16)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P16x8) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
          }
        }

      if( edge || filterTopMbEdgeFlag )
        vidParam->GetStrengthHor(MbQ, edge, mvlimit, p);
      }//end edge

    MbQ->DeblockCall = 0;
    }
  }
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Performing Deblocking for one macroblock.
 *****************************************************************************************
 */
static void perform_db_mbaff (sVidParam* vidParam, sPicture *p, int MbQAddr)
{
  sMacroblock   *MbQ = &(vidParam->mb_data[MbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (MbQ->DFDisableIdc == 1)
  {
    MbQ->DeblockCall = 0;
  }
  else
  {
    int           edge;

    short         mb_x, mb_y;

    int           filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
    int           filterLeftMbEdgeFlag;
    int           filterTopMbEdgeFlag;
    int           edge_cr;

    sPixel     **imgY = p->imgY;
    sPixel   ***imgUV = p->imgUV;
    Slice * currSlice = MbQ->p_Slice;
    int       mvlimit = ((p->structure!=FRAME) || (p->mb_aff_frame_flag && MbQ->mb_field)) ? 2 : 4;

    sSPSrbsp *active_sps = vidParam->active_sps;

    MbQ->DeblockCall = 1;
    get_mb_pos (vidParam, MbQAddr, vidParam->mb_size[IS_LUMA], &mb_x, &mb_y);

    if (MbQ->mb_type == I8MB)
      assert(MbQ->luma_transform_size_8x8_flag);

    filterNon8x8LumaEdgesFlag[1] =
      filterNon8x8LumaEdgesFlag[3] = !(MbQ->luma_transform_size_8x8_flag);

    filterLeftMbEdgeFlag = (mb_x != 0);
    filterTopMbEdgeFlag  = (mb_y != 0);

    if (p->mb_aff_frame_flag && mb_y == MB_BLOCK_SIZE && MbQ->mb_field)
      filterTopMbEdgeFlag = 0;

    if (MbQ->DFDisableIdc==2)
    {
      // don't filter at slice boundaries
      filterLeftMbEdgeFlag = MbQ->mbAvailA;
      // if this the bottom of a frame macroblock pair then always filter the top edge
      filterTopMbEdgeFlag  = (p->mb_aff_frame_flag && !MbQ->mb_field && (MbQAddr & 0x01)) ? 1 : MbQ->mbAvailB;
    }

    CheckAvailabilityOfNeighborsMBAFF(MbQ);

    // Vertical deblocking
    for (edge = 0; edge < 4 ; ++edge )
    {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0)
      {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc != YUV444)
          continue;
        else if (edge > 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE))
        {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P16x8)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P8x16) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
        }
      }

      if( edge || filterLeftMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_ver[edge];

        if ((*((int64 *) Strength)) || ((*(((int64 *) Strength) + 1)))) // only if one of the 16 Strength bytes is != 0
        {
          if (filterNon8x8LumaEdgesFlag[edge])
          {
            vidParam->EdgeLoopLumaVer( PLANE_Y, imgY, Strength, MbQ, edge << 2);
            if(currSlice->chroma444_not_separate)
            {
              vidParam->EdgeLoopLumaVer(PLANE_U, imgUV[0], Strength, MbQ, edge << 2);
              vidParam->EdgeLoopLumaVer(PLANE_V, imgUV[1], Strength, MbQ, edge << 2);
            }
          }
          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
          {
            edge_cr = chroma_edge[0][edge][p->chroma_format_idc];
            if( (imgUV != NULL) && (edge_cr >= 0))
            {
              vidParam->EdgeLoopChromaVer( imgUV[0], Strength, MbQ, edge_cr, 0, p);
              vidParam->EdgeLoopChromaVer( imgUV[1], Strength, MbQ, edge_cr, 1, p);
            }
          }
        }
      }
    }//end edge

    // horizontal deblocking
    for( edge = 0; edge < 4 ; ++edge )
    {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0)
      {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc==YUV420)
          continue;
        else if (edge > 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE))
        {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P8x16)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P16x8) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
        }
      }

      if( edge || filterTopMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_hor[edge];

        if ((*((int64 *) Strength)) || ((*(((int64 *) Strength) + 1)))) // only if one of the 16 Strength bytes is != 0
        {
          if (filterNon8x8LumaEdgesFlag[edge])
          {
            vidParam->EdgeLoopLumaHor( PLANE_Y, imgY, Strength, MbQ, edge << 2, p) ;
            if(currSlice->chroma444_not_separate)
            {
              vidParam->EdgeLoopLumaHor(PLANE_U, imgUV[0], Strength, MbQ, edge << 2, p);
              vidParam->EdgeLoopLumaHor(PLANE_V, imgUV[1], Strength, MbQ, edge << 2, p);
            }
          }

          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
          {
            edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
            if( (imgUV != NULL) && (edge_cr >= 0))
            {
              vidParam->EdgeLoopChromaHor( imgUV[0], Strength, MbQ, edge_cr, 0, p);
              vidParam->EdgeLoopChromaHor( imgUV[1], Strength, MbQ, edge_cr, 1, p);
            }
          }
        }

        if (!edge && !MbQ->mb_field && MbQ->mixedModeEdgeFlag) //currSlice->mixedModeEdgeFlag)
        {
          // this is the extra horizontal edge between a frame macroblock pair and a field above it
          MbQ->DeblockCall = 2;
          vidParam->GetStrengthHor(MbQ, 4, mvlimit, p); // Strength for 4 blks in 1 stripe

          //if( *((int*)Strength) )                      // only if one of the 4 Strength bytes is != 0
          {
            if (filterNon8x8LumaEdgesFlag[edge])
            {

              vidParam->EdgeLoopLumaHor(PLANE_Y, imgY, Strength, MbQ, MB_BLOCK_SIZE, p) ;
              if(currSlice->chroma444_not_separate)
              {
                vidParam->EdgeLoopLumaHor(PLANE_U, imgUV[0], Strength, MbQ, MB_BLOCK_SIZE, p) ;
                vidParam->EdgeLoopLumaHor(PLANE_V, imgUV[1], Strength, MbQ, MB_BLOCK_SIZE, p) ;
              }
            }
            if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
            {
              edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
              if( (imgUV != NULL) && (edge_cr >= 0))
              {
                vidParam->EdgeLoopChromaHor( imgUV[0], Strength, MbQ, MB_BLOCK_SIZE, 0, p) ;
                vidParam->EdgeLoopChromaHor( imgUV[1], Strength, MbQ, MB_BLOCK_SIZE, 1, p) ;
              }
            }
          }
          MbQ->DeblockCall = 1;
        }
      }
    }//end edge

    MbQ->DeblockCall = 0;
  }
}

//}}}
//{{{
static void set_loop_filter_functions_mbaff (sVidParam* vidParam)
{
  //vidParam->GetStrengthVer    = get_strength_ver_MBAff;
  //vidParam->GetStrengthHor    = get_strength_hor_MBAff;
  vidParam->EdgeLoopLumaVer   = edge_loop_luma_ver_MBAff;
  vidParam->EdgeLoopLumaHor   = edge_loop_luma_hor_MBAff;
  vidParam->EdgeLoopChromaVer = edge_loop_chroma_ver_MBAff;
  vidParam->EdgeLoopChromaHor = edge_loop_chroma_hor_MBAff;
}
//}}}

// normal
//{{{
  /*!
 *********************************************************************************************
 * \brief
 *    returns a buffer of 16 Strength values for one stripe in a mb (for different Frame or Field types)
 *********************************************************************************************
 */
static void get_strength_ver (sMacroblock *MbQ, int edge, int mvlimit, sPicture *p)
{
  byte *Strength = MbQ->strength_ver[edge];
  Slice* currSlice = MbQ->p_Slice;
  int     StrValue, i;
  BlockPos *PicPos = MbQ->vidParam->PicPos;

  if ((currSlice->slice_type==SP_SLICE)||(currSlice->slice_type==SI_SLICE) )
  {
    // Set strength to either 3 or 4 regardless of pixel position
    StrValue = (edge == 0) ? 4 : 3;
    for( i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
  }
  else
  {
    if (MbQ->is_intra_block == FALSE)
    {
      sMacroblock *MbP;
      int xQ = (edge << 2) - 1;
      sMacroblock *neighbor = get_non_aff_neighbor_luma(MbQ, xQ, 0);
      MbP = (edge) ? MbQ : neighbor;

      if (edge || MbP->is_intra_block == FALSE)
      {
        if (edge && (currSlice->slice_type == P_SLICE && MbQ->mb_type == PSKIP))
        {
          for( i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = 0;
        }
        else  if (edge && ((MbQ->mb_type == P16x16)  || (MbQ->mb_type == P16x8)))
        {
          int      blkP, blkQ, idx;

          for( idx = 0 ; idx < MB_BLOCK_SIZE ; idx += BLOCK_SIZE )
          {
            blkQ = idx + (edge);
            blkP = idx + (get_x_luma(xQ) >> 2);
            if ((MbQ->s_cbp[0].blk & (i64_power2(blkQ) | i64_power2(blkP))) != 0)
              StrValue = 2;
            else
              StrValue = 0; // if internal edge of certain types, then we already know StrValue should be 0

            Strength[idx >> 2] = StrValue;
          }
        }
        else
        {
          int      blkP, blkQ, idx;
          BlockPos mb = PicPos[ MbQ->mbAddrX ];
          mb.x <<= BLOCK_SHIFT;
          mb.y <<= BLOCK_SHIFT;

          for( idx = 0 ; idx < MB_BLOCK_SIZE ; idx += BLOCK_SIZE )
          {
            blkQ = idx  + (edge);
            blkP = idx  + (get_x_luma(xQ) >> 2);
            if (((MbQ->s_cbp[0].blk & i64_power2(blkQ)) != 0) || ((MbP->s_cbp[0].blk & i64_power2(blkP)) != 0))
              StrValue = 2;
            else // for everything else, if no coefs, but vector difference >= 1 set Strength=1
            {
              int blk_y  = mb.y + (blkQ >> 2);
              int blk_x  = mb.x + (blkQ  & 3);
              int blk_y2 = (short)(get_pos_y_luma(neighbor,  0) + idx) >> 2;
              int blk_x2 = (short)(get_pos_x_luma(neighbor, xQ)      ) >> 2;
              PicMotionParams *mv_info_p = &p->mv_info[blk_y ][blk_x ];
              PicMotionParams *mv_info_q = &p->mv_info[blk_y2][blk_x2];
              StorablePicturePtr ref_p0 = mv_info_p->ref_pic[LIST_0];
              StorablePicturePtr ref_q0 = mv_info_q->ref_pic[LIST_0];
              StorablePicturePtr ref_p1 = mv_info_p->ref_pic[LIST_1];
              StorablePicturePtr ref_q1 = mv_info_q->ref_pic[LIST_1];

              if ( ((ref_p0==ref_q0) && (ref_p1==ref_q1)) || ((ref_p0==ref_q1) && (ref_p1==ref_q0)))
              {
                // L0 and L1 reference pictures of p0 are different; q0 as well
                if (ref_p0 != ref_p1)
                {
                  // compare MV for the same reference picture
                  if (ref_p0 == ref_q0)
                  {
                    StrValue =
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) |
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit);
                  }
                  else
                  {
                    StrValue =
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) |
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit);
                  }
                }
                else
                { // L0 and L1 reference pictures of p0 are the same; q0 as well
                  StrValue = ((
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) |
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit))
                    && (
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) |
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit)
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
      else
      {
        // Start with Strength=3. or Strength=4 for Mb-edge
        StrValue = (edge == 0) ? 4 : 3;
        for( i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
      }
    }
    else
    {
      // Start with Strength=3. or Strength=4 for Mb-edge
      StrValue = (edge == 0) ? 4 : 3;
      for( i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
    }
  }
}
//}}}
//{{{
  /*!
 *********************************************************************************************
 * \brief
 *    returns a buffer of 16 Strength values for one stripe in a mb (for different Frame or Field types)
 *********************************************************************************************
 */
static void get_strength_hor (sMacroblock *MbQ, int edge, int mvlimit, sPicture *p)
{
  byte  *Strength = MbQ->strength_hor[edge];
  int    StrValue, i;
  Slice* currSlice = MbQ->p_Slice;
  BlockPos *PicPos = MbQ->vidParam->PicPos;

  if ((currSlice->slice_type==SP_SLICE)||(currSlice->slice_type==SI_SLICE) )
  {
    // Set strength to either 3 or 4 regardless of pixel position
    StrValue = (edge == 0 && (((p->structure==FRAME)))) ? 4 : 3;
    for( i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
  }
  else
  {
    if (MbQ->is_intra_block == FALSE)
    {
      sMacroblock *MbP;
      int yQ = (edge < BLOCK_SIZE ? (edge << 2) - 1: 0);

      sMacroblock *neighbor = get_non_aff_neighbor_luma(MbQ, 0, yQ);

      MbP = (edge) ? MbQ : neighbor;

      if (edge || MbP->is_intra_block == FALSE)
      {
        if (edge && (currSlice->slice_type == P_SLICE && MbQ->mb_type == PSKIP))
        {
          for( i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = 0;
        }
        else if (edge && ((MbQ->mb_type == P16x16)  || (MbQ->mb_type == P8x16)))
        {
          int      blkP, blkQ, idx;

          for( idx = 0 ; idx < BLOCK_SIZE ; idx ++ )
          {
            blkQ = (yQ + 1) + idx;
            blkP = (get_y_luma(yQ) & 0xFFFC) + idx;

            if ((MbQ->s_cbp[0].blk & (i64_power2(blkQ) | i64_power2(blkP))) != 0)
              StrValue = 2;
            else
              StrValue = 0; // if internal edge of certain types, we already know StrValue should be 0

            Strength[idx] = StrValue;
          }
        }
        else
        {
          int      blkP, blkQ, idx;
          BlockPos mb = PicPos[ MbQ->mbAddrX ];
          mb.x <<= 2;
          mb.y <<= 2;

          for( idx = 0 ; idx < BLOCK_SIZE ; idx ++)
          {
            blkQ = (yQ + 1) + idx;
            blkP = (get_y_luma(yQ) & 0xFFFC) + idx;

            if (((MbQ->s_cbp[0].blk & i64_power2(blkQ)) != 0) || ((MbP->s_cbp[0].blk & i64_power2(blkP)) != 0))
              StrValue = 2;
            else // for everything else, if no coefs, but vector difference >= 1 set Strength=1
            {
              int blk_y  = mb.y + (blkQ >> 2);
              int blk_x  = mb.x + (blkQ  & 3);
              int blk_y2 = get_pos_y_luma(neighbor,yQ) >> 2;
              int blk_x2 = ((short)(get_pos_x_luma(neighbor,0)) >> 2) + idx;

              PicMotionParams *mv_info_p = &p->mv_info[blk_y ][blk_x ];
              PicMotionParams *mv_info_q = &p->mv_info[blk_y2][blk_x2];

              StorablePicturePtr ref_p0 = mv_info_p->ref_pic[LIST_0];
              StorablePicturePtr ref_q0 = mv_info_q->ref_pic[LIST_0];
              StorablePicturePtr ref_p1 = mv_info_p->ref_pic[LIST_1];
              StorablePicturePtr ref_q1 = mv_info_q->ref_pic[LIST_1];

              if ( ((ref_p0==ref_q0) && (ref_p1==ref_q1)) || ((ref_p0==ref_q1) && (ref_p1==ref_q0)))
              {
                // L0 and L1 reference pictures of p0 are different; q0 as well
                if (ref_p0 != ref_p1)
                {
                  // compare MV for the same reference picture
                  if (ref_p0 == ref_q0)
                  {
                    StrValue =
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) |
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit);
                  }
                  else
                  {
                    StrValue =
                      compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) |
                      compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit);
                  }
                }
                else
                { // L0 and L1 reference pictures of p0 are the same; q0 as well
                  StrValue = ((
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_0], mvlimit) |
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_1], mvlimit))
                    && (
                    compare_mvs(&mv_info_p->mv[LIST_0], &mv_info_q->mv[LIST_1], mvlimit) |
                    compare_mvs(&mv_info_p->mv[LIST_1], &mv_info_q->mv[LIST_0], mvlimit)
                    ));
                }
              }
              else
                StrValue = 1;
            }
            Strength[idx] = StrValue;
          }
        }
      }
      else
      {
        // Start with Strength=3. or Strength=4 for Mb-edge
        StrValue = (edge == 0 && (p->structure == FRAME)) ? 4 : 3;
        for( i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
      }
    }
    else
    {
      // Start with Strength=3. or Strength=4 for Mb-edge
      StrValue = (edge == 0 && (p->structure == FRAME)) ? 4 : 3;
      for( i = 0; i < BLOCK_SIZE; i ++ ) Strength[i] = StrValue;
    }
  }
}
//}}}
//{{{
static void get_db_strength_normal (sVidParam* vidParam, sPicture *p, int MbQAddr, int *piCnt)
{
  sMacroblock   *MbQ = &(vidParam->mb_data[MbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (MbQ->DFDisableIdc == 1)
  {
    MbQ->DeblockCall = 0;
  }
  else
  {
    *piCnt = (*piCnt < 0)? MbQAddr: (*piCnt);
    if(MbQ->luma_transform_size_8x8_flag)
    {
      int  filterLeftMbEdgeFlag = (MbQ->pix_x != 0);
      int  filterTopMbEdgeFlag  = (MbQ->pix_y != 0);

      int       mvlimit = (p->structure!=FRAME) ? 2 : 4;

      MbQ->DeblockCall = 1;
      //get_mb_pos (vidParam, MbQAddr, vidParam->mb_size[IS_LUMA], &mb_x, &mb_y);

      if (MbQ->DFDisableIdc==2)
      {
        // don't filter at slice boundaries
        filterLeftMbEdgeFlag = MbQ->mbAvailA;
        // if this the bottom of a frame macroblock pair then always filter the top edge
        filterTopMbEdgeFlag  = MbQ->mbAvailB;
      }

      // Vertical deblocking
      if( filterLeftMbEdgeFlag )
        get_strength_ver(MbQ, 0, mvlimit, p);
      get_strength_ver(MbQ, 2, mvlimit, p);

      // horizontal deblocking
      if( filterTopMbEdgeFlag )
        get_strength_hor(MbQ, 0, mvlimit, p);
      get_strength_hor(MbQ, 2, mvlimit, p);
    }
    else
    {
      int           filterLeftMbEdgeFlag;
      int           filterTopMbEdgeFlag;

      Slice * currSlice = MbQ->p_Slice;
      int       mvlimit = (p->structure!=FRAME) ? 2 : 4;

      MbQ->DeblockCall = 1;
      //get_mb_pos (vidParam, MbQAddr, vidParam->mb_size[IS_LUMA], &mb_x, &mb_y);

      filterLeftMbEdgeFlag = (MbQ->pix_x != 0);
      filterTopMbEdgeFlag  = (MbQ->pix_y != 0);

      if (MbQ->DFDisableIdc==2)
      {
        // don't filter at slice boundaries
        filterLeftMbEdgeFlag = MbQ->mbAvailA;
        // if this the bottom of a frame macroblock pair then always filter the top edge
        filterTopMbEdgeFlag  = MbQ->mbAvailB;
      }

      if ((currSlice->slice_type == P_SLICE && MbQ->mb_type == PSKIP) || ((MbQ->mb_type == P16x16) && (MbQ->cbp == 0)))
      {
        // Vertical deblocking
        if( filterLeftMbEdgeFlag )
          get_strength_ver(MbQ, 0, mvlimit, p);

        // horizontal deblocking
        if( filterTopMbEdgeFlag )
          get_strength_hor(MbQ, 0, mvlimit, p);
      }
      else if ((MbQ->mb_type == P16x8) && (MbQ->cbp == 0))
      {
        // Vertical deblocking
        if( filterLeftMbEdgeFlag )
          get_strength_ver(MbQ, 0, mvlimit, p);

        // horizontal deblocking
        if( filterTopMbEdgeFlag )
          get_strength_hor(MbQ, 0, mvlimit, p);
        get_strength_hor(MbQ, 2, mvlimit, p);
      }
      else if ((MbQ->mb_type == P8x16) && (MbQ->cbp == 0))
      {
        // Vertical deblocking
        if( filterLeftMbEdgeFlag )
          get_strength_ver(MbQ, 0, mvlimit, p);
        get_strength_ver(MbQ, 2, mvlimit, p);

        // horizontal deblocking
        if( filterTopMbEdgeFlag )
          get_strength_hor(MbQ, 0, mvlimit, p);
      }
      else if ((currSlice->slice_type == B_SLICE) && (MbQ->mb_type == BSKIP_DIRECT) && (vidParam->active_sps->direct_8x8_inference_flag) && (MbQ->cbp == 0))
      {
        // Vertical
        if( filterLeftMbEdgeFlag )
          get_strength_ver(MbQ, 0, mvlimit, p);
        get_strength_ver(MbQ, 2, mvlimit, p);

        // Horizontal
        if( filterTopMbEdgeFlag )
          get_strength_hor(MbQ, 0, mvlimit, p);
        get_strength_hor(MbQ, 2, mvlimit, p);
      }
      else
      {
        // Vertical deblocking
        if( filterLeftMbEdgeFlag )
          get_strength_ver(MbQ, 0, mvlimit, p);
        get_strength_ver(MbQ, 1, mvlimit, p);
        get_strength_ver(MbQ, 2, mvlimit, p);
        get_strength_ver(MbQ, 3, mvlimit, p);

        // Horizontal deblocking
        if( filterTopMbEdgeFlag )
          get_strength_hor(MbQ, 0, mvlimit, p);
        get_strength_hor(MbQ, 1, mvlimit, p);
        get_strength_hor(MbQ, 2, mvlimit, p);
        get_strength_hor(MbQ, 3, mvlimit, p);
      }
    }
    MbQ->DeblockCall = 0;
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Vertical Deblocking with Strength = 4
 *****************************************************************************************
 */
static void luma_ver_deblock_strong (sPixel **cur_img, int pos_x1, int Alpha, int Beta)
{
  int i;
  for( i = 0 ; i < BLOCK_SIZE ; ++i )
  {
    sPixel *SrcPtrP = *(cur_img++) + pos_x1;
    sPixel *SrcPtrQ = SrcPtrP + 1;
    sPixel  L0 = *SrcPtrP;
    sPixel  R0 = *SrcPtrQ;

    if( iabs( R0 - L0 ) < Alpha )
    {
      sPixel  R1 = *(SrcPtrQ + 1);
      sPixel  L1 = *(SrcPtrP - 1);
      if ((iabs( R0 - R1) < Beta)  && (iabs(L0 - L1) < Beta))
      {
        if ((iabs( R0 - L0 ) < ((Alpha >> 2) + 2)))
        {
          sPixel  R2 = *(SrcPtrQ + 2);
          sPixel  L2 = *(SrcPtrP - 2);
          int RL0 = L0 + R0;

          if (( iabs( L0 - L2) < Beta ))
          {
            sPixel  L3 = *(SrcPtrP - 3);
            *(SrcPtrP--) = (sPixel)  (( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3);
            *(SrcPtrP--) = (sPixel)  (( L2 + L1 + RL0 + 2) >> 2);
            *(SrcPtrP  ) = (sPixel) ((((L3 + L2) <<1) + L2 + L1 + RL0 + 4) >> 3);
          }
          else
          {
            *SrcPtrP = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2);
          }

          if (( iabs( R0 - R2) < Beta ))
          {
            sPixel  R3 = *(SrcPtrQ + 3);
            *(SrcPtrQ++) = (sPixel) (( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3);
            *(SrcPtrQ++) = (sPixel) (( R2 + R0 + L0 + R1 + 2) >> 2);
            *(SrcPtrQ  ) = (sPixel) ((((R3 + R2) <<1) + R2 + R1 + RL0 + 4) >> 3);
          }
          else
          {
            *SrcPtrQ = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
          }
        }
        else
        {
          *SrcPtrP = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2);
          *SrcPtrQ = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
        }
      }
    }
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Vertical Deblocking with Normal Strength
 *****************************************************************************************
 */
static void luma_ver_deblock_normal (sPixel **cur_img, int pos_x1, int Alpha, int Beta, int C0, int max_imgpel_value)
{
  int i;
  sPixel *SrcPtrP, *SrcPtrQ;
  int edge_diff;

  if (C0 == 0)
  {
    for( i= 0 ; i < BLOCK_SIZE ; ++i )
    {
      SrcPtrP = *(cur_img++) + pos_x1;
      SrcPtrQ = SrcPtrP + 1;
      edge_diff = *SrcPtrQ - *SrcPtrP;

      if( iabs( edge_diff ) < Alpha )
      {
        sPixel  *SrcPtrQ1 = SrcPtrQ + 1;
        sPixel  *SrcPtrP1 = SrcPtrP - 1;

        if ((iabs( *SrcPtrQ - *SrcPtrQ1) < Beta)  && (iabs(*SrcPtrP - *SrcPtrP1) < Beta))
        {
          sPixel  R2 = *(SrcPtrQ1 + 1);
          sPixel  L2 = *(SrcPtrP1 - 1);

          int aq  = (iabs(*SrcPtrQ - R2) < Beta);
          int ap  = (iabs(*SrcPtrP - L2) < Beta);

          int tc0  = (ap + aq) ;
          int dif = iClip3( -tc0, tc0, (((edge_diff) << 2) + (*SrcPtrP1 - *SrcPtrQ1) + 4) >> 3 );

          if (dif != 0)
          {
            *SrcPtrP = (sPixel) iClip1(max_imgpel_value, *SrcPtrP + dif);
            *SrcPtrQ = (sPixel) iClip1(max_imgpel_value, *SrcPtrQ - dif);
          }
        }
      }
    }
  }
  else
  {
    for( i= 0 ; i < BLOCK_SIZE ; ++i )
    {
      SrcPtrP = *(cur_img++) + pos_x1;
      SrcPtrQ = SrcPtrP + 1;
      edge_diff = *SrcPtrQ - *SrcPtrP;

      if( iabs( edge_diff ) < Alpha )
      {
        sPixel  *SrcPtrQ1 = SrcPtrQ + 1;
        sPixel  *SrcPtrP1 = SrcPtrP - 1;

        if ((iabs( *SrcPtrQ - *SrcPtrQ1) < Beta)  && (iabs(*SrcPtrP - *SrcPtrP1) < Beta))
        {
          int RL0 = (*SrcPtrP + *SrcPtrQ + 1) >> 1;
          sPixel  R2 = *(SrcPtrQ1 + 1);
          sPixel  L2 = *(SrcPtrP1 - 1);

          int aq  = (iabs(*SrcPtrQ - R2) < Beta);
          int ap  = (iabs(*SrcPtrP - L2) < Beta);

          int tc0  = (C0 + ap + aq) ;
          int dif = iClip3( -tc0, tc0, (((edge_diff) << 2) + (*SrcPtrP1 - *SrcPtrQ1) + 4) >> 3 );

          if( ap )
            *SrcPtrP1 = (sPixel) (*SrcPtrP1 + iClip3( -C0,  C0, (L2 + RL0 - (*SrcPtrP1<<1)) >> 1 ));

          if (dif != 0)
          {
            *SrcPtrP = (sPixel) iClip1(max_imgpel_value, *SrcPtrP + dif);
            *SrcPtrQ = (sPixel) iClip1(max_imgpel_value, *SrcPtrQ - dif);
          }

          if( aq )
            *SrcPtrQ1 = (sPixel) (*SrcPtrQ1 + iClip3( -C0,  C0, (R2 + RL0 - (*SrcPtrQ1<<1)) >> 1 ));
        }
      }
    }
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Filters 16 pel block edge of Frame or Field coded MBs
 *****************************************************************************************
 */
static void edge_loop_luma_ver (ColorPlane pl, sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge)
{
  sVidParam* vidParam = MbQ->vidParam;

  sMacroblock *MbP = get_non_aff_neighbor_luma(MbQ, edge - 1, 0);

  if (MbP || (MbQ->DFDisableIdc== 0))
  {
    int bitdepth_scale   = pl ? vidParam->bitdepth_scale[IS_CHROMA] : vidParam->bitdepth_scale[IS_LUMA];

    // Average QP of the two blocks
    int QP = pl? ((MbP->qpc[pl-1] + MbQ->qpc[pl-1] + 1) >> 1) : (MbP->qp + MbQ->qp + 1) >> 1;

    int indexA = iClip3(0, MAX_QP, QP + MbQ->DFAlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + MbQ->DFBetaOffset);

    int Alpha  = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta   = BETA_TABLE [indexB] * bitdepth_scale;

    if ((Alpha | Beta )!= 0)
    {
      const byte *ClipTab = CLIP_TAB[indexA];
      int max_imgpel_value = vidParam->max_pel_value_comp[pl];

      int pos_x1 = get_pos_x_luma(MbP, (edge - 1));
      sPixel **cur_img = &Img[get_pos_y_luma(MbP, 0)];
      int pel;

      for( pel = 0 ; pel < MB_BLOCK_SIZE ; pel += 4 )
      {
        if(*Strength == 4 )    // INTRA strong filtering
        {
          luma_ver_deblock_strong(cur_img, pos_x1, Alpha, Beta);
        }
        else if( *Strength != 0) // normal filtering
        {
          luma_ver_deblock_normal(cur_img, pos_x1, Alpha, Beta, ClipTab[ *Strength ] * bitdepth_scale, max_imgpel_value);
        }
        cur_img += 4;
        Strength ++;
      }
    }
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Horizontal Deblocking with Strength = 4
 *****************************************************************************************
 */
static void luma_hor_deblock_strong (sPixel *imgP, sPixel *imgQ, int width, int Alpha, int Beta)
{
  int pixel;
  int inc_dim2 = width * 2;
  int inc_dim3 = width * 3;
  for( pixel = 0 ; pixel < BLOCK_SIZE ; ++pixel )
  {
    sPixel *SrcPtrP = imgP++;
    sPixel *SrcPtrQ = imgQ++;
    sPixel  L0 = *SrcPtrP;
    sPixel  R0 = *SrcPtrQ;

    if( iabs( R0 - L0 ) < Alpha )
    {
      sPixel  L1 = *(SrcPtrP - width);
      sPixel  R1 = *(SrcPtrQ + width);

      if ((iabs( R0 - R1) < Beta)  && (iabs(L0 - L1) < Beta))
      {
        if ((iabs( R0 - L0 ) < ((Alpha >> 2) + 2)))
        {
          sPixel  L2 = *(SrcPtrP - inc_dim2);
          sPixel  R2 = *(SrcPtrQ + inc_dim2);
          int RL0 = L0 + R0;

          if (( iabs( L0 - L2) < Beta ))
          {
            sPixel  L3 = *(SrcPtrP - inc_dim3);
            *(SrcPtrP         ) = (sPixel)  (( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3);
            *(SrcPtrP -= width) = (sPixel)  (( L2 + L1 + RL0 + 2) >> 2);
            *(SrcPtrP -  width) = (sPixel) ((((L3 + L2) <<1) + L2 + L1 + RL0 + 4) >> 3);
          }
          else
          {
            *SrcPtrP = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2);
          }

          if (( iabs( R0 - R2) < Beta ))
          {
            sPixel  R3 = *(SrcPtrQ + inc_dim3);
            *(SrcPtrQ          ) = (sPixel)  (( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3);
            *(SrcPtrQ += width ) = (sPixel)  (( R2 + R0 + L0 + R1 + 2) >> 2);
            *(SrcPtrQ +  width ) = (sPixel) ((((R3 + R2) <<1) + R2 + R1 + RL0 + 4) >> 3);
          }
          else
          {
            *SrcPtrQ = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
          }
        }
        else
        {
          *SrcPtrP = (sPixel) (((L1 << 1) + L0 + R1 + 2) >> 2);
          *SrcPtrQ = (sPixel) (((R1 << 1) + R0 + L1 + 2) >> 2);
        }
      }
    }
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Horizontal Deblocking with Strength = 4
 *****************************************************************************************
 */
static void luma_hor_deblock_normal (sPixel *imgP, sPixel *imgQ, int width, int Alpha, int Beta, int C0, int max_imgpel_value)
{
  int i;
  int edge_diff;
  int tc0, dif, aq, ap;

  if (C0 == 0)
  {
    for( i= 0 ; i < BLOCK_SIZE ; ++i )
    {
      edge_diff = *imgQ - *imgP;

      if( iabs( edge_diff ) < Alpha )
      {
        sPixel  *SrcPtrQ1 = imgQ + width;
        sPixel  *SrcPtrP1 = imgP - width;

        if ((iabs( *imgQ - *SrcPtrQ1) < Beta)  && (iabs(*imgP - *SrcPtrP1) < Beta))
        {
          sPixel  R2 = *(SrcPtrQ1 + width);
          sPixel  L2 = *(SrcPtrP1 - width);

          aq  = (iabs(*imgQ - R2) < Beta);
          ap  = (iabs(*imgP - L2) < Beta);

          tc0  = (ap + aq) ;
          dif = iClip3( -tc0, tc0, (((edge_diff) << 2) + (*SrcPtrP1 - *SrcPtrQ1) + 4) >> 3 );

          if (dif != 0)
          {
            *imgP = (sPixel) iClip1(max_imgpel_value, *imgP + dif);
            *imgQ = (sPixel) iClip1(max_imgpel_value, *imgQ - dif);
          }
        }
      }
      imgP++;
      imgQ++;
    }
  }
  else
  {
    for( i= 0 ; i < BLOCK_SIZE ; ++i )
    {
      edge_diff = *imgQ - *imgP;

      if( iabs( edge_diff ) < Alpha )
      {
        sPixel  *SrcPtrQ1 = imgQ + width;
        sPixel  *SrcPtrP1 = imgP - width;

        if ((iabs( *imgQ - *SrcPtrQ1) < Beta)  && (iabs(*imgP - *SrcPtrP1) < Beta))
        {
          int RL0 = (*imgP + *imgQ + 1) >> 1;
          sPixel  R2 = *(SrcPtrQ1 + width);
          sPixel  L2 = *(SrcPtrP1 - width);

          aq  = (iabs(*imgQ - R2) < Beta);
          ap  = (iabs(*imgP - L2) < Beta);

          tc0  = (C0 + ap + aq) ;
          dif = iClip3( -tc0, tc0, (((edge_diff) << 2) + (*SrcPtrP1 - *SrcPtrQ1) + 4) >> 3 );

          if( ap )
            *SrcPtrP1 = (sPixel) (*SrcPtrP1 + iClip3( -C0,  C0, (L2 + RL0 - (*SrcPtrP1<<1)) >> 1 ));

          if (dif != 0)
          {
            *imgP = (sPixel) iClip1(max_imgpel_value, *imgP + dif);
            *imgQ = (sPixel) iClip1(max_imgpel_value, *imgQ - dif);
          }

          if( aq )
            *SrcPtrQ1 = (sPixel) (*SrcPtrQ1 + iClip3( -C0,  C0, (R2 + RL0 - (*SrcPtrQ1<<1)) >> 1 ));
        }
      }
      imgP++;
      imgQ++;
    }
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Filters 16 pel block edge of Frame or Field coded MBs
 *****************************************************************************************
 */
static void edge_loop_luma_hor (ColorPlane pl, sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, sPicture *p)
{
  sVidParam* vidParam = MbQ->vidParam;

  int ypos = (edge < MB_BLOCK_SIZE ? edge - 1: 0);
  sMacroblock *MbP = get_non_aff_neighbor_luma(MbQ, 0, ypos);

  if (MbP || (MbQ->DFDisableIdc== 0))
  {
    int bitdepth_scale   = pl ? vidParam->bitdepth_scale[IS_CHROMA] : vidParam->bitdepth_scale[IS_LUMA];

    // Average QP of the two blocks
    int QP = pl? ((MbP->qpc[pl-1] + MbQ->qpc[pl-1] + 1) >> 1) : (MbP->qp + MbQ->qp + 1) >> 1;

    int indexA = iClip3(0, MAX_QP, QP + MbQ->DFAlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + MbQ->DFBetaOffset);

    int Alpha  = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta   = BETA_TABLE [indexB] * bitdepth_scale;

    if ((Alpha | Beta )!= 0)
    {
      const byte *ClipTab = CLIP_TAB[indexA];
      int max_imgpel_value = vidParam->max_pel_value_comp[pl];
      int width = p->iLumaStride; //p->size_x;

      sPixel *imgP = &Img[get_pos_y_luma(MbP, ypos)][get_pos_x_luma(MbP, 0)];
      sPixel *imgQ = imgP + width;
      int pel;

      for( pel = 0 ; pel < BLOCK_SIZE ; pel++ )
      {
        if(*Strength == 4 )    // INTRA strong filtering
        {
          luma_hor_deblock_strong(imgP, imgQ, width, Alpha, Beta);
        }
        else if( *Strength != 0) // normal filtering
        {
          luma_hor_deblock_normal(imgP, imgQ, width, Alpha, Beta, ClipTab[ *Strength ] * bitdepth_scale, max_imgpel_value);
        }
        imgP += 4;
        imgQ += 4;
        Strength ++;
      }
    }
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Filters chroma block edge for Frame or Field coded pictures
 *****************************************************************************************
 */
static void edge_loop_chroma_ver (sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, int uv, sPicture *p)
{
  sVidParam* vidParam = MbQ->vidParam;

  int block_width  = vidParam->mb_cr_size_x;
  int block_height = vidParam->mb_cr_size_y;
  int xQ = edge - 1;
  int yQ = 0;

  sMacroblock *MbP = get_non_aff_neighbor_chroma(MbQ,xQ,yQ,block_width,block_height);

  if (MbP || (MbQ->DFDisableIdc == 0))
  {
    int      bitdepth_scale   = vidParam->bitdepth_scale[IS_CHROMA];
    int      max_imgpel_value = vidParam->max_pel_value_comp[uv + 1];

    int AlphaC0Offset = MbQ->DFAlphaC0Offset;
    int BetaOffset = MbQ->DFBetaOffset;

    // Average QP of the two blocks
    int QP = (MbP->qpc[uv] + MbQ->qpc[uv] + 1) >> 1;

    int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + BetaOffset);

    int Alpha   = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta    = BETA_TABLE [indexB] * bitdepth_scale;

    if ((Alpha | Beta) != 0)
    {
      const int PelNum = pelnum_cr[0][p->chroma_format_idc];
      const     byte *ClipTab = CLIP_TAB[indexA];

      int pel;
      int pos_x1 = get_pos_x_chroma(MbP, xQ, (block_width - 1));
      sPixel **cur_img = &Img[get_pos_y_chroma(MbP,yQ, (block_height - 1))];

      for( pel = 0 ; pel < PelNum ; ++pel )
      {
        int Strng = Strength[(PelNum == 8) ? (pel >> 1) : (pel >> 2)];

        if( Strng != 0)
        {
          sPixel *SrcPtrP = *cur_img + pos_x1;
          sPixel *SrcPtrQ = SrcPtrP + 1;
          int edge_diff = *SrcPtrQ - *SrcPtrP;

          if ( iabs( edge_diff ) < Alpha )
          {
            sPixel R1  = *(SrcPtrQ + 1);
            if ( iabs(*SrcPtrQ - R1) < Beta )
            {
              sPixel L1  = *(SrcPtrP - 1);
              if ( iabs(*SrcPtrP - L1) < Beta )
              {
                if( Strng == 4 )    // INTRA strong filtering
                {
                  *SrcPtrP = (sPixel) ( ((L1 << 1) + *SrcPtrP + R1 + 2) >> 2 );
                  *SrcPtrQ = (sPixel) ( ((R1 << 1) + *SrcPtrQ + L1 + 2) >> 2 );
                }
                else
                {
                  int tc0  = ClipTab[ Strng ] * bitdepth_scale + 1;
                  int dif = iClip3( -tc0, tc0, ( ((edge_diff) << 2) + (L1 - R1) + 4) >> 3 );

                  if (dif != 0)
                  {
                    *SrcPtrP = (sPixel) iClip1 ( max_imgpel_value, *SrcPtrP + dif );
                    *SrcPtrQ = (sPixel) iClip1 ( max_imgpel_value, *SrcPtrQ - dif );
                  }
                }
              }
            }
          }
        }
        cur_img++;
      }
    }
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Filters chroma block edge for Frame or Field coded pictures
 *****************************************************************************************
 */
static void edge_loop_chroma_hor (sPixel** Img, byte *Strength, sMacroblock *MbQ, int edge, int uv, sPicture *p)
{
  sVidParam* vidParam = MbQ->vidParam;
  int block_width = vidParam->mb_cr_size_x;
  int block_height = vidParam->mb_cr_size_y;
  int xQ = 0;
  int yQ = (edge < 16 ? edge - 1: 0);

  sMacroblock *MbP = get_non_aff_neighbor_chroma(MbQ,xQ,yQ,block_width,block_height);

  if (MbP || (MbQ->DFDisableIdc == 0))
  {
    int      bitdepth_scale   = vidParam->bitdepth_scale[IS_CHROMA];
    int      max_imgpel_value = vidParam->max_pel_value_comp[uv + 1];

    int AlphaC0Offset = MbQ->DFAlphaC0Offset;
    int BetaOffset = MbQ->DFBetaOffset;
    int width = p->iChromaStride; //p->size_x_cr;

    // Average QP of the two blocks
    int QP = (MbP->qpc[uv] + MbQ->qpc[uv] + 1) >> 1;

    int indexA = iClip3(0, MAX_QP, QP + AlphaC0Offset);
    int indexB = iClip3(0, MAX_QP, QP + BetaOffset);

    int Alpha   = ALPHA_TABLE[indexA] * bitdepth_scale;
    int Beta    = BETA_TABLE [indexB] * bitdepth_scale;

    if ((Alpha | Beta) != 0)
    {
      const int PelNum = pelnum_cr[1][p->chroma_format_idc];
      const     byte *ClipTab = CLIP_TAB[indexA];

      int pel;

      sPixel *imgP = &Img[get_pos_y_chroma(MbP,yQ, (block_height-1))][get_pos_x_chroma(MbP,xQ, (block_width - 1))];
      sPixel *imgQ = imgP + width ;

      for( pel = 0 ; pel < PelNum ; ++pel )
      {
        int Strng = Strength[(PelNum == 8) ? (pel >> 1) : (pel >> 2)];

        if( Strng != 0)
        {
          sPixel *SrcPtrP = imgP;
          sPixel *SrcPtrQ = imgQ;
          int edge_diff = *imgQ - *imgP;

          if ( iabs( edge_diff ) < Alpha )
          {
            sPixel R1  = *(SrcPtrQ + width);
            if ( iabs(*SrcPtrQ - R1) < Beta )
            {
              sPixel L1  = *(SrcPtrP - width);
              if ( iabs(*SrcPtrP - L1) < Beta )
              {
                if( Strng == 4 )    // INTRA strong filtering
                {
                  *SrcPtrP = (sPixel) ( ((L1 << 1) + *SrcPtrP + R1 + 2) >> 2 );
                  *SrcPtrQ = (sPixel) ( ((R1 << 1) + *SrcPtrQ + L1 + 2) >> 2 );
                }
                else
                {
                  int tc0  = ClipTab[ Strng ] * bitdepth_scale + 1;
                  int dif = iClip3( -tc0, tc0, ( ((edge_diff) << 2) + (L1 - R1) + 4) >> 3 );

                  if (dif != 0)
                  {
                    *SrcPtrP = (sPixel) iClip1 ( max_imgpel_value, *SrcPtrP + dif );
                    *SrcPtrQ = (sPixel) iClip1 ( max_imgpel_value, *SrcPtrQ - dif );
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
static void perform_db_dep_normal (sMacroblock *MbQ, sPicture *p)
{
  sVidParam* vidParam = MbQ->vidParam;
  Slice * currSlice = MbQ->p_Slice;
  int           edge;

  short         mb_x, mb_y;

  int           filterLeftMbEdgeFlag;
  int           filterTopMbEdgeFlag;

  sPixel     **imgY = p->imgY;
  sPixel   ***imgUV = p->imgUV;

  sSPSrbsp *active_sps = vidParam->active_sps;

  MbQ->DeblockCall = 1;
  get_mb_pos (vidParam, MbQ->mbAddrX, vidParam->mb_size[IS_LUMA], &mb_x, &mb_y);

  filterLeftMbEdgeFlag = (mb_x != 0);
  filterTopMbEdgeFlag  = (mb_y != 0);

  if (MbQ->DFDisableIdc == 2)
  {
    // don't filter at slice boundaries
    filterLeftMbEdgeFlag = MbQ->mbAvailA;
    // if this the bottom of a frame macroblock pair then always filter the top edge
    filterTopMbEdgeFlag  = MbQ->mbAvailB;
  }

  if (MbQ->luma_transform_size_8x8_flag)
  {
    // Vertical deblocking
    for (edge = 0; edge < 4 ; edge += 2)
    {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE))
      {
        //if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc != YUV444)
        if (edge > 0)
        {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P16x8)))
            continue;
        }
      }

      if( edge || filterLeftMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_ver[edge];

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
        {
          edge_loop_luma_ver( PLANE_Y, imgY, Strength, MbQ, edge << 2);
          edge_loop_luma_ver(PLANE_U, imgUV[0], Strength, MbQ, edge << 2);
          edge_loop_luma_ver(PLANE_V, imgUV[1], Strength, MbQ, edge << 2);
        }
      }
    }//end edge

    // horizontal deblocking
    for( edge = 0; edge < 4 ; edge += 2 )
    {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE))
      {
        if (edge > 0)
        {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P8x16)))
            continue;
        }
      }

      if( edge || filterTopMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_hor[edge];

        if (Strength[0]!=0 || Strength[1]!=0 || Strength[2]!=0 || Strength[3]!=0) // only if one of the 16 Strength bytes is != 0
        {
          edge_loop_luma_hor( PLANE_Y, imgY, Strength, MbQ, edge << 2, p) ;
          edge_loop_luma_hor(PLANE_U, imgUV[0], Strength, MbQ, edge << 2, p);
          edge_loop_luma_hor(PLANE_V, imgUV[1], Strength, MbQ, edge << 2, p);
        }
      }
    }//end edge
  }
  else
  {
    // Vertical deblocking
    for (edge = 0; edge < 4 ; ++edge )
    {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE))
      {
        if (edge > 0)
        {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P16x8)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P8x16) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
        }
      }

      if( edge || filterLeftMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_ver[edge];

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
        {
          edge_loop_luma_ver( PLANE_Y, imgY, Strength, MbQ, edge << 2);
          edge_loop_luma_ver(PLANE_U, imgUV[0], Strength, MbQ, edge << 2);
          edge_loop_luma_ver(PLANE_V, imgUV[1], Strength, MbQ, edge << 2);
        }
      }
    }//end edge

    // horizontal deblocking
    for( edge = 0; edge < 4 ; ++edge )
    {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE))
      {
        if (edge > 0)
        {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P8x16)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P16x8) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
        }
      }

      if( edge || filterTopMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_hor[edge];

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
        {
          edge_loop_luma_hor( PLANE_Y, imgY, Strength, MbQ, edge << 2, p) ;
          edge_loop_luma_hor(PLANE_U, imgUV[0], Strength, MbQ, edge << 2, p);
          edge_loop_luma_hor(PLANE_V, imgUV[1], Strength, MbQ, edge << 2, p);
        }
      }
    }//end edge
  }
}
//}}}
//{{{
static void perform_db_ind_normal (sMacroblock *MbQ, sPicture *p)
{
  sVidParam* vidParam = MbQ->vidParam;
  Slice * currSlice = MbQ->p_Slice;
  //short         mb_x, mb_y;

  int           filterLeftMbEdgeFlag;
  int           filterTopMbEdgeFlag;

  sPixel     **imgY = p->imgY;
  sPixel   ***imgUV = p->imgUV;

  sSPSrbsp *active_sps = vidParam->active_sps;

  MbQ->DeblockCall = 1;
  //get_mb_pos (vidParam, MbQ->mbAddrX, vidParam->mb_size[IS_LUMA], &mb_x, &mb_y);

  filterLeftMbEdgeFlag = (MbQ->pix_x != 0);
  filterTopMbEdgeFlag  = (MbQ->pix_y != 0);

  if (MbQ->DFDisableIdc == 2)
  {
    // don't filter at slice boundaries
    filterLeftMbEdgeFlag = MbQ->mbAvailA;
    // if this the bottom of a frame macroblock pair then always filter the top edge
    filterTopMbEdgeFlag  = MbQ->mbAvailB;
  }

  if (MbQ->luma_transform_size_8x8_flag)
  {
    int edge, edge_cr;

    // Vertical deblocking
    for (edge = 0; edge < 4 ; edge += 2)
    {
      if( edge || filterLeftMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_ver[edge];

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
        {
          edge_loop_luma_ver( PLANE_Y, imgY, Strength, MbQ, edge << 2);

          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
          {
            edge_cr = chroma_edge[0][edge][p->chroma_format_idc];
            if( (imgUV != NULL) && (edge_cr >= 0))
            {
              edge_loop_chroma_ver( imgUV[0], Strength, MbQ, edge_cr, 0, p);
              edge_loop_chroma_ver( imgUV[1], Strength, MbQ, edge_cr, 1, p);
            }
          }
        }
      }
    }//end edge

    // horizontal deblocking
    for( edge = 0; edge < 4 ; edge += 2 )
    {
      if( edge || filterTopMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_hor[edge];

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
        {
          edge_loop_luma_hor( PLANE_Y, imgY, Strength, MbQ, edge << 2, p) ;

          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
          {
            edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
            if( (imgUV != NULL) && (edge_cr >= 0))
            {
              edge_loop_chroma_hor( imgUV[0], Strength, MbQ, edge_cr, 0, p);
              edge_loop_chroma_hor( imgUV[1], Strength, MbQ, edge_cr, 1, p);
            }
          }
        }
      }
    }//end edge
  }
  else
  {
    if (((MbQ->mb_type == PSKIP) && (currSlice->slice_type == P_SLICE)) || ((MbQ->mb_type == P16x16) && (MbQ->cbp == 0)))
    {
      // Vertical deblocking
      if( filterLeftMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_ver[0];

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
        {
          edge_loop_luma_ver( PLANE_Y, imgY, Strength, MbQ, 0);

          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
          {
            if( (imgUV != NULL))
            {
              edge_loop_chroma_ver( imgUV[0], Strength, MbQ, 0, 0, p);
              edge_loop_chroma_ver( imgUV[1], Strength, MbQ, 0, 1, p);
            }
          }
        }
      }

      // horizontal deblocking

      if( filterTopMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_hor[0];

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
        {
          edge_loop_luma_hor( PLANE_Y, imgY, Strength, MbQ, 0, p) ;

          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
          {
            if( (imgUV != NULL))
            {
              edge_loop_chroma_hor( imgUV[0], Strength, MbQ, 0, 0, p);
              edge_loop_chroma_hor( imgUV[1], Strength, MbQ, 0, 1, p);
            }
          }
        }
      }
    }
    else if ((MbQ->mb_type == P16x8) && (MbQ->cbp == 0))
    {
      int edge, edge_cr;
      // Vertical deblocking
      if( filterLeftMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_ver[0];

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
        {
          edge_loop_luma_ver( PLANE_Y, imgY, Strength, MbQ, 0);

          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
          {
            if (imgUV != NULL)
            {
              edge_loop_chroma_ver( imgUV[0], Strength, MbQ, 0, 0, p);
              edge_loop_chroma_ver( imgUV[1], Strength, MbQ, 0, 1, p);
            }
          }
        }
      }

      // horizontal deblocking
      for( edge = 0; edge < 4 ; edge += 2)
      {
        if( edge || filterTopMbEdgeFlag )
        {
          byte *Strength = MbQ->strength_hor[edge];

          if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
          {
            edge_loop_luma_hor( PLANE_Y, imgY, Strength, MbQ, edge << 2, p) ;

            if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
            {
              edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
              if( (imgUV != NULL) && (edge_cr >= 0))
              {
                edge_loop_chroma_hor( imgUV[0], Strength, MbQ, edge_cr, 0, p);
                edge_loop_chroma_hor( imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }
        }
      }//end edge
    }
    else if ((MbQ->mb_type == P8x16) && (MbQ->cbp == 0))
    {
      int edge, edge_cr;
      // Vertical deblocking
      for (edge = 0; edge < 4 ; edge += 2)
      {
        if( edge || filterLeftMbEdgeFlag )
        {
          byte *Strength = MbQ->strength_ver[edge];

          if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
          {
            edge_loop_luma_ver( PLANE_Y, imgY, Strength, MbQ, edge << 2);

            if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
            {
              edge_cr = chroma_edge[0][edge][p->chroma_format_idc];
              if( (imgUV != NULL) && (edge_cr >= 0))
              {
                edge_loop_chroma_ver( imgUV[0], Strength, MbQ, edge_cr, 0, p);
                edge_loop_chroma_ver( imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }
        }
      }//end edge

      // horizontal deblocking
      if( filterTopMbEdgeFlag )
      {
        byte *Strength = MbQ->strength_hor[0];

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
        {
          edge_loop_luma_hor( PLANE_Y, imgY, Strength, MbQ, 0, p) ;

          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
          {
            if (imgUV != NULL)
            {
              edge_loop_chroma_hor( imgUV[0], Strength, MbQ, 0, 0, p);
              edge_loop_chroma_hor( imgUV[1], Strength, MbQ, 0, 1, p);
            }
          }
        }
      }
    }
    else if ((currSlice->slice_type == B_SLICE) && (MbQ->mb_type == BSKIP_DIRECT) && (active_sps->direct_8x8_inference_flag) && (MbQ->cbp == 0))
    {
      int edge, edge_cr;
      // Vertical deblocking
      for (edge = 0; edge < 4 ; edge += 2)
      {
        if( edge || filterLeftMbEdgeFlag )
        {
          byte *Strength = MbQ->strength_ver[edge];

          if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
          {
            edge_loop_luma_ver( PLANE_Y, imgY, Strength, MbQ, edge << 2);

            if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
            {
              edge_cr = chroma_edge[0][edge][p->chroma_format_idc];
              if( (imgUV != NULL) && (edge_cr >= 0))
              {
                edge_loop_chroma_ver( imgUV[0], Strength, MbQ, edge_cr, 0, p);
                edge_loop_chroma_ver( imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }
        }
      }//end edge

      // horizontal deblocking
      for( edge = 0; edge < 4 ; edge += 2)
      {
        if( edge || filterTopMbEdgeFlag )
        {
          byte *Strength = MbQ->strength_hor[edge];

          if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
          {
            edge_loop_luma_hor( PLANE_Y, imgY, Strength, MbQ, edge << 2, p) ;

            if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
            {
              edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
              if( (imgUV != NULL) && (edge_cr >= 0))
              {
                edge_loop_chroma_hor( imgUV[0], Strength, MbQ, edge_cr, 0, p);
                edge_loop_chroma_hor( imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }
        }
      }//end edge
    }
    else
    {
      int edge, edge_cr;
      // Vertical deblocking
      for (edge = 0; edge < 4 ; ++edge )
      {
        if( edge || filterLeftMbEdgeFlag )
        {
          byte *Strength = MbQ->strength_ver[edge];

          if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
          {
            edge_loop_luma_ver( PLANE_Y, imgY, Strength, MbQ, edge << 2);

            if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
            {
              edge_cr = chroma_edge[0][edge][p->chroma_format_idc];
              if( (imgUV != NULL) && (edge_cr >= 0))
              {
                edge_loop_chroma_ver( imgUV[0], Strength, MbQ, edge_cr, 0, p);
                edge_loop_chroma_ver( imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }
        }
      }//end edge

      // horizontal deblocking
      for( edge = 0; edge < 4 ; ++edge )
      {
        if( edge || filterTopMbEdgeFlag )
        {
          byte *Strength = MbQ->strength_hor[edge];

          if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) // only if one of the first 4 Strength bytes is != 0
          {
            edge_loop_luma_hor( PLANE_Y, imgY, Strength, MbQ, edge << 2, p) ;

            if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422)
            {
              edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
              if( (imgUV != NULL) && (edge_cr >= 0))
              {
                edge_loop_chroma_hor( imgUV[0], Strength, MbQ, edge_cr, 0, p);
                edge_loop_chroma_hor( imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }
        }
      }//end edge
    }
  }
}
//}}}
//{{{
/*!
 *****************************************************************************************
 * \brief
 *    Deblocking filter for one macroblock.
 *****************************************************************************************
 */
static void perform_db_normal (sVidParam* vidParam, sPicture *p, int MbQAddr)
{
  sMacroblock   *MbQ = &(vidParam->mb_data[MbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (MbQ->DFDisableIdc == 1)
  {
    MbQ->DeblockCall = 0;
  }
  else
  {
    if(MbQ->p_Slice->chroma444_not_separate)
      perform_db_dep_normal(MbQ, p);
    else
      perform_db_ind_normal(MbQ, p);
    MbQ->DeblockCall = 0;
  }
}
//}}}
//{{{
static void deblock_normal (sVidParam* vidParam, sPicture *p) {

  unsigned int i;
  int j=-1;
  for (i = 0; i < p->PicSizeInMbs; ++i)
    get_db_strength_normal( vidParam, p, i, &j) ;
  for (i = 0; i < p->PicSizeInMbs; ++i)
    perform_db_normal (vidParam, p, i ) ;
  }
//}}}
//{{{
static void set_loop_filter_functions_normal (sVidParam* vidParam)
{
  vidParam->GetStrengthVer    = get_strength_ver;
  vidParam->GetStrengthHor    = get_strength_hor;
  vidParam->EdgeLoopLumaVer   = edge_loop_luma_ver;
  vidParam->EdgeLoopLumaHor   = edge_loop_luma_hor;
  vidParam->EdgeLoopChromaVer = edge_loop_chroma_ver;
  vidParam->EdgeLoopChromaHor = edge_loop_chroma_hor;
}
//}}}

// loopfilter
//{{{
static void DeblockMb (sVidParam* vidParam, sPicture *p, int MbQAddr) {

  sMacroblock* MbQ = &(vidParam->mb_data[MbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (MbQ->DFDisableIdc == 1)
    MbQ->DeblockCall = 0;

  else {
    int           edge;
    byte Strength[16];
    short         mb_x, mb_y;
    int           filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
    int           filterLeftMbEdgeFlag;
    int           filterTopMbEdgeFlag;
    int           edge_cr;

    sPixel     **imgY = p->imgY;
    sPixel   ***imgUV = p->imgUV;
    Slice * currSlice = MbQ->p_Slice;
    int       mvlimit = ((p->structure!=FRAME) || (p->mb_aff_frame_flag && MbQ->mb_field)) ? 2 : 4;
    sSPSrbsp* active_sps = vidParam->active_sps;

    MbQ->DeblockCall = 1;
    get_mb_pos (vidParam, MbQAddr, vidParam->mb_size[IS_LUMA], &mb_x, &mb_y);

    if (MbQ->mb_type == I8MB)
      assert(MbQ->luma_transform_size_8x8_flag);

    filterNon8x8LumaEdgesFlag[1] =
      filterNon8x8LumaEdgesFlag[3] = !(MbQ->luma_transform_size_8x8_flag);

    filterLeftMbEdgeFlag = (mb_x != 0);
    filterTopMbEdgeFlag  = (mb_y != 0);

    if (p->mb_aff_frame_flag && mb_y == MB_BLOCK_SIZE && MbQ->mb_field)
      filterTopMbEdgeFlag = 0;

    if (MbQ->DFDisableIdc==2) {
      // don't filter at slice boundaries
      filterLeftMbEdgeFlag = MbQ->mbAvailA;
      // if this the bottom of a frame macroblock pair then always filter the top edge
      filterTopMbEdgeFlag  = (p->mb_aff_frame_flag && !MbQ->mb_field && (MbQAddr & 0x01)) ? 1 : MbQ->mbAvailB;
      }

    if (p->mb_aff_frame_flag == 1)
      CheckAvailabilityOfNeighborsMBAFF (MbQ);

    // Vertical deblocking
    for (edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc != YUV444)
          continue;
        else if (edge > 0) {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P16x8)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P8x16) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
          }
        }

      if( edge || filterLeftMbEdgeFlag ) {
        // Strength for 4 blks in 1 stripe
        get_strength_ver_MBAff (Strength, MbQ, edge << 2, mvlimit, p);

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] !=0 ||
        Strength[4] != 0 || Strength[5] != 0 || Strength[6] != 0 || Strength[7] !=0 ||
        Strength[8] != 0 || Strength[9] != 0 || Strength[10] != 0 || Strength[11] !=0 ||
        Strength[12] != 0 || Strength[13] != 0 || Strength[14] != 0 || Strength[15] !=0 ) {
          // only if one of the 16 Strength bytes is != 0
          if (filterNon8x8LumaEdgesFlag[edge]) {
            vidParam->EdgeLoopLumaVer( PLANE_Y, imgY, Strength, MbQ, edge << 2);
            if(currSlice->chroma444_not_separate) {
              vidParam->EdgeLoopLumaVer(PLANE_U, imgUV[0], Strength, MbQ, edge << 2);
              vidParam->EdgeLoopLumaVer(PLANE_V, imgUV[1], Strength, MbQ, edge << 2);
              }
            }
          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422) {
            edge_cr = chroma_edge[0][edge][p->chroma_format_idc];
            if( (imgUV != NULL) && (edge_cr >= 0)) {
              vidParam->EdgeLoopChromaVer( imgUV[0], Strength, MbQ, edge_cr, 0, p);
              vidParam->EdgeLoopChromaVer( imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }
        }
      }//end edge

    // horizontal deblocking
    for (edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc==YUV420)
          continue;
        else if (edge > 0) {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P8x16)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P16x8) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
          }
        }

      if( edge || filterTopMbEdgeFlag ) {
        // Strength for 4 blks in 1 stripe
        get_strength_hor_MBAff (Strength, MbQ, edge << 2, mvlimit, p);

        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] !=0 ||
        Strength[4] != 0 || Strength[5] != 0 || Strength[6] != 0 || Strength[7] !=0 ||
        Strength[8] != 0 || Strength[9] != 0 || Strength[10] != 0 || Strength[11] !=0 ||
        Strength[12] != 0 || Strength[13] != 0 || Strength[14] != 0 || Strength[15] !=0 ) {
          // only if one of the 16 Strength bytes is != 0
          if (filterNon8x8LumaEdgesFlag[edge]) {
            vidParam->EdgeLoopLumaHor( PLANE_Y, imgY, Strength, MbQ, edge << 2, p) ;
             if(currSlice->chroma444_not_separate) {
              vidParam->EdgeLoopLumaHor(PLANE_U, imgUV[0], Strength, MbQ, edge << 2, p);
              vidParam->EdgeLoopLumaHor(PLANE_V, imgUV[1], Strength, MbQ, edge << 2, p);
              }
            }

          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422) {
            edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
            if( (imgUV != NULL) && (edge_cr >= 0)) {
              vidParam->EdgeLoopChromaHor( imgUV[0], Strength, MbQ, edge_cr, 0, p);
              vidParam->EdgeLoopChromaHor( imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }

        if (!edge && !MbQ->mb_field && MbQ->mixedModeEdgeFlag) {
          //currSlice->mixedModeEdgeFlag)
          // this is the extra horizontal edge between a frame macroblock pair and a field above it
          MbQ->DeblockCall = 2;
          get_strength_hor_MBAff (Strength, MbQ, MB_BLOCK_SIZE, mvlimit, p); // Strength for 4 blks in 1 stripe

          //if( *((int*)Strength) )                      // only if one of the 4 Strength bytes is != 0
          {
            if (filterNon8x8LumaEdgesFlag[edge]) {
              vidParam->EdgeLoopLumaHor(PLANE_Y, imgY, Strength, MbQ, MB_BLOCK_SIZE, p) ;
              if(currSlice->chroma444_not_separate) {
                vidParam->EdgeLoopLumaHor(PLANE_U, imgUV[0], Strength, MbQ, MB_BLOCK_SIZE, p) ;
                vidParam->EdgeLoopLumaHor(PLANE_V, imgUV[1], Strength, MbQ, MB_BLOCK_SIZE, p) ;
                }
              }

            if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422) {
              edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
              if( (imgUV != NULL) && (edge_cr >= 0)) {
                vidParam->EdgeLoopChromaHor( imgUV[0], Strength, MbQ, MB_BLOCK_SIZE, 0, p) ;
                vidParam->EdgeLoopChromaHor( imgUV[1], Strength, MbQ, MB_BLOCK_SIZE, 1, p) ;
                }
              }
            }
          MbQ->DeblockCall = 1;
          }
        }
      }//end edge

    MbQ->DeblockCall = 0;
    }
  }
//}}}
//{{{
static void get_db_strength (sVidParam* vidParam, sPicture *p, int MbQAddr) {

  sMacroblock   *MbQ = &(vidParam->mb_data[MbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (MbQ->DFDisableIdc == 1)
    MbQ->DeblockCall = 0;

  else {
    int           edge;
    short         mb_x, mb_y;
    int           filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
    int           filterLeftMbEdgeFlag;
    int           filterTopMbEdgeFlag;

    Slice * currSlice = MbQ->p_Slice;
    int       mvlimit = ((p->structure!=FRAME) || (p->mb_aff_frame_flag && MbQ->mb_field)) ? 2 : 4;
    sSPSrbsp *active_sps = vidParam->active_sps;

    MbQ->DeblockCall = 1;
    get_mb_pos (vidParam, MbQAddr, vidParam->mb_size[IS_LUMA], &mb_x, &mb_y);

    if (MbQ->mb_type == I8MB)
      assert(MbQ->luma_transform_size_8x8_flag);

    filterNon8x8LumaEdgesFlag[1] =
      filterNon8x8LumaEdgesFlag[3] = !(MbQ->luma_transform_size_8x8_flag);

    filterLeftMbEdgeFlag = (mb_x != 0);
    filterTopMbEdgeFlag  = (mb_y != 0);

    if (p->mb_aff_frame_flag && mb_y == MB_BLOCK_SIZE && MbQ->mb_field)
      filterTopMbEdgeFlag = 0;

    if (MbQ->DFDisableIdc==2) {
      // don't filter at slice boundaries
      filterLeftMbEdgeFlag = MbQ->mbAvailA;
      // if this the bottom of a frame macroblock pair then always filter the top edge
      filterTopMbEdgeFlag = (p->mb_aff_frame_flag && !MbQ->mb_field && (MbQAddr & 0x01)) ? 1 : MbQ->mbAvailB;
      }

    if (p->mb_aff_frame_flag == 1)
      CheckAvailabilityOfNeighborsMBAFF (MbQ);

    // Vertical deblocking
    for (edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc != YUV444)
          continue;
        else if (edge > 0) {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) ||
               (MbQ->mb_type == P16x16) ||
               (MbQ->mb_type == P16x8)))
            continue;
          else if ((edge & 0x01) &&
                   ((MbQ->mb_type == P8x16) ||
                   (currSlice->slice_type == B_SLICE &&
                    MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
          }
        }

      if (edge || filterLeftMbEdgeFlag )
        // Strength for 4 blks in 1 stripe
        vidParam->GetStrengthVer (MbQ, edge, mvlimit, p);
      }//end edge

    // horizontal deblocking
    for( edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc==YUV420)
          continue;
        else if (edge > 0) {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) ||
               (MbQ->mb_type == P16x16) ||
               (MbQ->mb_type == P8x16)))
            continue;
          else if ((edge & 0x01) &&
                   ((MbQ->mb_type == P16x8) ||
                    (currSlice->slice_type == B_SLICE &&
                     MbQ->mb_type == BSKIP_DIRECT &&
                     active_sps->direct_8x8_inference_flag)))
            continue;
          }
        }

      if( edge || filterTopMbEdgeFlag )
        vidParam->GetStrengthHor (MbQ, edge, mvlimit, p);
      }//end edge

    MbQ->DeblockCall = 0;
   }
  }
//}}}
//{{{
static void perform_db (sVidParam* vidParam, sPicture *p, int MbQAddr) {

  sMacroblock* MbQ = &(vidParam->mb_data[MbQAddr]) ; // current Mb

  // return, if filter is disabled
  if (MbQ->DFDisableIdc == 1)
    MbQ->DeblockCall = 0;

  else {
    int           edge;
    short         mb_x, mb_y;
    int           filterNon8x8LumaEdgesFlag[4] = {1,1,1,1};
    int           filterLeftMbEdgeFlag;
    int           filterTopMbEdgeFlag;
    int           edge_cr;
    sPixel     **imgY = p->imgY;
    sPixel   ***imgUV = p->imgUV;
    Slice * currSlice = MbQ->p_Slice;
    int       mvlimit = ((p->structure!=FRAME) || (p->mb_aff_frame_flag && MbQ->mb_field)) ? 2 : 4;
    sSPSrbsp *active_sps = vidParam->active_sps;

    MbQ->DeblockCall = 1;
    get_mb_pos (vidParam, MbQAddr, vidParam->mb_size[IS_LUMA], &mb_x, &mb_y);

    if (MbQ->mb_type == I8MB)
      assert(MbQ->luma_transform_size_8x8_flag);

    filterNon8x8LumaEdgesFlag[1] =
      filterNon8x8LumaEdgesFlag[3] = !(MbQ->luma_transform_size_8x8_flag);

    filterLeftMbEdgeFlag = (mb_x != 0);
    filterTopMbEdgeFlag  = (mb_y != 0);

    if (p->mb_aff_frame_flag && mb_y == MB_BLOCK_SIZE && MbQ->mb_field)
      filterTopMbEdgeFlag = 0;

    if (MbQ->DFDisableIdc==2) {
      // don't filter at slice boundaries
      filterLeftMbEdgeFlag = MbQ->mbAvailA;
      // if this the bottom of a frame macroblock pair then always filter the top edge
      filterTopMbEdgeFlag  = (p->mb_aff_frame_flag && !MbQ->mb_field && (MbQAddr & 0x01)) ? 1 : MbQ->mbAvailB;
      }

    if (p->mb_aff_frame_flag == 1)
      CheckAvailabilityOfNeighborsMBAFF(MbQ);

    // Vertical deblocking
    for (edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc != YUV444)
          continue;
        else if (edge > 0) {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P16x8)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P8x16) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
          }
        }

      if( edge || filterLeftMbEdgeFlag ) {
        byte *Strength = MbQ->strength_ver[edge];
        if ( Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] != 0 ) {
          // only if one of the 4 first Strength bytes is != 0
          if (filterNon8x8LumaEdgesFlag[edge]) {
            vidParam->EdgeLoopLumaVer( PLANE_Y, imgY, Strength, MbQ, edge << 2);
            if(currSlice->chroma444_not_separate) {
              vidParam->EdgeLoopLumaVer(PLANE_U, imgUV[0], Strength, MbQ, edge << 2);
              vidParam->EdgeLoopLumaVer(PLANE_V, imgUV[1], Strength, MbQ, edge << 2);
              }
            }
          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422) {
            edge_cr = chroma_edge[0][edge][p->chroma_format_idc];
            if ((imgUV != NULL) && (edge_cr >= 0)) {
              vidParam->EdgeLoopChromaVer( imgUV[0], Strength, MbQ, edge_cr, 0, p);
              vidParam->EdgeLoopChromaVer( imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }
        }
      }//end edge

    // horizontal deblocking
    for( edge = 0; edge < 4 ; ++edge ) {
      // If cbp == 0 then deblocking for some macroblock types could be skipped
      if (MbQ->cbp == 0 && (currSlice->slice_type == P_SLICE || currSlice->slice_type == B_SLICE)) {
        if (filterNon8x8LumaEdgesFlag[edge] == 0 && active_sps->chroma_format_idc==YUV420)
          continue;
        else if (edge > 0) {
          if (((MbQ->mb_type == PSKIP && currSlice->slice_type == P_SLICE) || (MbQ->mb_type == P16x16) || (MbQ->mb_type == P8x16)))
            continue;
          else if ((edge & 0x01) && ((MbQ->mb_type == P16x8) || (currSlice->slice_type == B_SLICE && MbQ->mb_type == BSKIP_DIRECT && active_sps->direct_8x8_inference_flag)))
            continue;
          }
        }

      if (edge || filterTopMbEdgeFlag ) {
        byte* Strength = MbQ->strength_hor[edge];
        if (Strength[0] != 0 || Strength[1] != 0 || Strength[2] != 0 || Strength[3] !=0 ||
            Strength[4] != 0 || Strength[5] != 0 || Strength[6] != 0 || Strength[7] !=0 ||
            Strength[8] != 0 || Strength[9] != 0 || Strength[10] != 0 || Strength[11] != 0 ||
            Strength[12] != 0 || Strength[13] != 0 || Strength[14] != 0 || Strength[15] !=0 ) {
          // only if one of the 16 Strength bytes is != 0
          if (filterNon8x8LumaEdgesFlag[edge]) {
            vidParam->EdgeLoopLumaHor (PLANE_Y, imgY, Strength, MbQ, edge << 2, p) ;
            if(currSlice->chroma444_not_separate) {
              vidParam->EdgeLoopLumaHor (PLANE_U, imgUV[0], Strength, MbQ, edge << 2, p);
              vidParam->EdgeLoopLumaHor (PLANE_V, imgUV[1], Strength, MbQ, edge << 2, p);
              }
            }

          if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422) {
            edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
            if( (imgUV != NULL) && (edge_cr >= 0)) {
              vidParam->EdgeLoopChromaHor (imgUV[0], Strength, MbQ, edge_cr, 0, p);
              vidParam->EdgeLoopChromaHor (imgUV[1], Strength, MbQ, edge_cr, 1, p);
              }
            }
          }

        if (!edge && !MbQ->mb_field && MbQ->mixedModeEdgeFlag) {
          //currSlice->mixedModeEdgeFlag)
          // this is the extra horizontal edge between a frame macroblock pair and a field above it
          MbQ->DeblockCall = 2;
          vidParam->GetStrengthHor (MbQ, 4, mvlimit, p); // Strength for 4 blks in 1 stripe

          //if( *((int*)Strength) )                      // only if one of the 4 Strength bytes is != 0
          {
            if (filterNon8x8LumaEdgesFlag[edge]) {
              vidParam->EdgeLoopLumaHor(PLANE_Y, imgY, Strength, MbQ, MB_BLOCK_SIZE, p) ;
              if(currSlice->chroma444_not_separate) {
                vidParam->EdgeLoopLumaHor (PLANE_U, imgUV[0], Strength, MbQ, MB_BLOCK_SIZE, p) ;
                vidParam->EdgeLoopLumaHor (PLANE_V, imgUV[1], Strength, MbQ, MB_BLOCK_SIZE, p) ;
                }
              }

            if (active_sps->chroma_format_idc==YUV420 || active_sps->chroma_format_idc==YUV422) {
              edge_cr = chroma_edge[1][edge][p->chroma_format_idc];
              if( (imgUV != NULL) && (edge_cr >= 0)) {
                vidParam->EdgeLoopChromaHor (imgUV[0], Strength, MbQ, MB_BLOCK_SIZE, 0, p) ;
                vidParam->EdgeLoopChromaHor (imgUV[1], Strength, MbQ, MB_BLOCK_SIZE, 1, p) ;
                }
              }
            }
          MbQ->DeblockCall = 1;
          }
        }
      }//end edge

    MbQ->DeblockCall = 0;
    }
  }
//}}}
//{{{
void DeblockPicture (sVidParam* vidParam, sPicture *p) {

  if (p->mb_aff_frame_flag) {
    for (unsigned i = 0; i < p->PicSizeInMbs; ++i)
      DeblockMb (vidParam, p, i ) ;
    }
  else {
    for (unsigned i = 0; i < p->PicSizeInMbs; ++i)
      get_db_strength (vidParam, p, i ) ;
    for (unsigned i = 0; i < p->PicSizeInMbs; ++i)
      perform_db (vidParam, p, i ) ;
    }
  }
//}}}

//{{{
// likely already set - see testing via asserts
static void init_neighbors (sVidParam* vidParam) {

  int i, j;
  int width = vidParam->PicWidthInMbs;
  int height = vidParam->PicHeightInMbs;
  int size = vidParam->PicSizeInMbs;
  sMacroblock* currMB = &vidParam->mb_data[0];

  // do the top left corner
  currMB->mbup = NULL;
  currMB->mbleft = NULL;
  currMB++;

  // do top row
  for (i = 1; i < width; i++) {
    currMB->mbup = NULL;
    currMB->mbleft = currMB - 1;
    currMB++;
    }

  // do left edge
  for (i = width; i < size; i += width) {
    currMB->mbup = currMB - width;
    currMB->mbleft = NULL;
    currMB += width;
    }

  // do all others
  for (j = width + 1; j < width * height + 1; j += width) {
    currMB = &vidParam->mb_data[j];
    for (i = 1; i < width; i++) {
      currMB->mbup   = currMB - width;
      currMB->mbleft = currMB - 1;
      currMB++;
      }
    }
  }
//}}}
//{{{
void init_Deblock (sVidParam* vidParam, int mb_aff_frame_flag) {

  if (vidParam->yuv_format == YUV444 && vidParam->separate_colour_plane_flag) {
    change_plane_JV (vidParam, PLANE_Y, NULL);
    init_neighbors (gDecoder->vidParam);

    change_plane_JV (vidParam, PLANE_U, NULL);
    init_neighbors (gDecoder->vidParam);

    change_plane_JV (vidParam, PLANE_V, NULL);
    init_neighbors (gDecoder->vidParam);

    change_plane_JV (vidParam, PLANE_Y, NULL);
    }
  else
    init_neighbors (gDecoder->vidParam);

  if (mb_aff_frame_flag == 1)
    set_loop_filter_functions_mbaff (vidParam);
  else
    set_loop_filter_functions_normal (vidParam);
  }
//}}}
