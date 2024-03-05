#pragma once
#include "global.h"
#include "transform8x8.h"

//{{{
//! look up tables for FRExt_chroma support
static const unsigned char subblk_offset_x[3][8][4] =
{
  {
    {0, 4, 0, 4},
    {0, 4, 0, 4},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
  },
  {
    {0, 4, 0, 4},
    {0, 4, 0, 4},
    {0, 4, 0, 4},
    {0, 4, 0, 4},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
  },
  {
    {0, 4, 0, 4},
    {8,12, 8,12},
    {0, 4, 0, 4},
    {8,12, 8,12},
    {0, 4, 0, 4},
    {8,12, 8,12},
    {0, 4, 0, 4},
    {8,12, 8,12}
  }
};
//}}}
//{{{
static const unsigned char subblk_offset_y[3][8][4] =
{
  {
    {0, 0, 4, 4},
    {0, 0, 4, 4},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  {
    {0, 0, 4, 4},
    {8, 8,12,12},
    {0, 0, 4, 4},
    {8, 8,12,12},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  {
    {0, 0, 4, 4},
    {0, 0, 4, 4},
    {8, 8,12,12},
    {8, 8,12,12},
    {0, 0, 4, 4},
    {0, 0, 4, 4},
    {8, 8,12,12},
    {8, 8,12,12}
  }
};
//}}}
//{{{
static const byte QP_SCALE_CR[52]=
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
   12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
   28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
   37,38,38,38,39,39,39,39

};
//}}}
static const byte decode_block_scan[16] = {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};

extern void iMBtrans4x4 (sMacroblock* currMB, sColorPlane pl, int smb);
extern void iMBtrans8x8 (sMacroblock* currMB, sColorPlane pl);

extern void itrans_sp_cr (sMacroblock* currMB, int uv);

extern void Inv_Residual_trans_4x4 (sMacroblock* currMB, sColorPlane pl, int ioff, int joff);
extern void Inv_Residual_trans_8x8 (sMacroblock* currMB, sColorPlane pl, int ioff,int joff);
extern void Inv_Residual_trans_16x16 (sMacroblock* currMB, sColorPlane pl);
extern void Inv_Residual_trans_Chroma (sMacroblock* currMB, int uv);

extern void itrans4x4 (sMacroblock* currMB, sColorPlane pl, int ioff, int joff);
extern void itrans4x4_ls(sMacroblock* currMB, sColorPlane pl, int ioff, int joff);
extern void itrans_sp (sMacroblock* currMB, sColorPlane pl, int ioff, int joff);
extern void itrans_2 (sMacroblock* currMB, sColorPlane pl);
extern void iTransform (sMacroblock* currMB, sColorPlane pl, int smb);

extern void copy_image_data (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2, int width, int height);
extern void copy_image_data_16x16 (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2);
extern void copy_image_data_8x8 (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2);
extern void copy_image_data_4x4 (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2);
extern int CheckVertMV (sMacroblock* currMB, int vec1_y, int block_size_y);
