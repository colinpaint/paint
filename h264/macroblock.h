#pragma once
#include "global.h"
#include "buffer.h"

//{{{
static const uint8_t SNGL_SCAN[16][2] = {
  {0,0},{1,0},{0,1},{0,2},
  {1,1},{2,0},{3,0},{2,1},
  {1,2},{0,3},{1,3},{2,2},
  {3,1},{3,2},{2,3},{3,3}
};
//}}}
//{{{
static const uint8_t FIELD_SCAN[16][2] = {
  {0,0},{0,1},{1,0},{0,2},
  {0,3},{1,1},{1,2},{1,3},
  {2,0},{2,1},{2,2},{2,3},
  {3,0},{3,1},{3,2},{3,3}
};
//}}}
//{{{
//! used to control block sizes : Not used/16x16/16x8/8x16/8x8/8x4/4x8/4x4
static const int BLOCK_STEP[8][2]= {
  {0,0},{4,4},{4,2},{2,4},{2,2},{2,1},{1,2},{1,1}
};
//}}}
//{{{
//! single scan pattern
static const uint8_t SNGL_SCAN8x8[64][2] = {
  {0,0}, {1,0}, {0,1}, {0,2}, {1,1}, {2,0}, {3,0}, {2,1}, {1,2}, {0,3}, {0,4}, {1,3}, {2,2}, {3,1}, {4,0}, {5,0},
  {4,1}, {3,2}, {2,3}, {1,4}, {0,5}, {0,6}, {1,5}, {2,4}, {3,3}, {4,2}, {5,1}, {6,0}, {7,0}, {6,1}, {5,2}, {4,3},
  {3,4}, {2,5}, {1,6}, {0,7}, {1,7}, {2,6}, {3,5}, {4,4}, {5,3}, {6,2}, {7,1}, {7,2}, {6,3}, {5,4}, {4,5}, {3,6},
  {2,7}, {3,7}, {4,6}, {5,5}, {6,4}, {7,3}, {7,4}, {6,5}, {5,6}, {4,7}, {5,7}, {6,6}, {7,5}, {7,6}, {6,7}, {7,7}
};
//}}}
//{{{
//! field scan pattern
static const uint8_t FIELD_SCAN8x8[64][2] = {
  {0,0}, {0,1}, {0,2}, {1,0}, {1,1}, {0,3}, {0,4}, {1,2}, {2,0}, {1,3}, {0,5}, {0,6}, {0,7}, {1,4}, {2,1}, {3,0},
  {2,2}, {1,5}, {1,6}, {1,7}, {2,3}, {3,1}, {4,0}, {3,2}, {2,4}, {2,5}, {2,6}, {2,7}, {3,3}, {4,1}, {5,0}, {4,2},
  {3,4}, {3,5}, {3,6}, {3,7}, {4,3}, {5,1}, {6,0}, {5,2}, {4,4}, {4,5}, {4,6}, {4,7}, {5,3}, {6,1}, {6,2}, {5,4},
  {5,5}, {5,6}, {5,7}, {6,3}, {7,0}, {7,1}, {6,4}, {6,5}, {6,6}, {6,7}, {7,2}, {7,3}, {7,4}, {7,5}, {7,6}, {7,7}
};
//}}}
//{{{
static const uint8_t SCAN_YUV422[8][2] = {
  {0,0},{0,1},
  {1,0},{0,2},
  {0,3},{1,1},
  {1,2},{1,3}
};
//}}}
//{{{
static const uint8_t cbp_blk_chroma[8][4] = {
  {16, 17, 18, 19},
  {20, 21, 22, 23},
  {24, 25, 26, 27},
  {28, 29, 30, 31},
  {32, 33, 34, 35},
  {36, 37, 38, 39},
  {40, 41, 42, 43},
  {44, 45, 46, 47}
};
//}}}
//{{{
static const uint8_t cofuv_blk_x[3][8][4] = {
  { {0, 1, 0, 1},
    {0, 1, 0, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0} },

  { {0, 1, 0, 1},
    {0, 1, 0, 1},
    {0, 1, 0, 1},
    {0, 1, 0, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0} },

  { {0, 1, 0, 1},
    {2, 3, 2, 3},
    {0, 1, 0, 1},
    {2, 3, 2, 3},
    {0, 1, 0, 1},
    {2, 3, 2, 3},
    {0, 1, 0, 1},
    {2, 3, 2, 3} }
};
//}}}
//{{{
static const uint8_t cofuv_blk_y[3][8][4] = {
  { { 0, 0, 1, 1},
    { 0, 0, 1, 1},
    { 0, 0, 0, 0},
    { 0, 0, 0, 0},
    { 0, 0, 0, 0},
    { 0, 0, 0, 0},
    { 0, 0, 0, 0},
    { 0, 0, 0, 0} },

  { { 0, 0, 1, 1},
    { 2, 2, 3, 3},
    { 0, 0, 1, 1},
    { 2, 2, 3, 3},
    { 0, 0, 0, 0},
    { 0, 0, 0, 0},
    { 0, 0, 0, 0},
    { 0, 0, 0, 0} },

  { { 0, 0, 1, 1},
    { 0, 0, 1, 1},
    { 2, 2, 3, 3},
    { 2, 2, 3, 3},
    { 0, 0, 1, 1},
    { 0, 0, 1, 1},
    { 2, 2, 3, 3},
    { 2, 2, 3, 3}}
};
//}}}

//{{{
//! look up tables for FRExt_chroma support
static const uint8_t subblk_offset_x[3][8][4] = {
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
static const uint8_t subblk_offset_y[3][8][4] = {
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
static const uint8_t QP_SCALE_CR[52] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
   12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
   28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
   37,38,38,38,39,39,39,39

};
//}}}
//{{{
static const uint8_t decode_block_scan[16] = {
  0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15
  };
//}}}

void iMBtrans4x4 (sMacroBlock* mb, eColorPlane plane, int smb);
void iMBtrans8x8 (sMacroBlock* mb, eColorPlane plane);

void itransSpChroma (sMacroBlock* mb, int uv);

void invResidualTrans4x4 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff);
void invResidualTrans8x8 (sMacroBlock* mb, eColorPlane plane, int ioff,int joff);
void invResidualTrans16x16 (sMacroBlock* mb, eColorPlane plane);
void invResidualTransChroma (sMacroBlock* mb, int uv);

void itrans4x4 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff);
void itrans4x4Lossless(sMacroBlock* mb, eColorPlane plane, int ioff, int joff);
void itransSp (sMacroBlock* mb, eColorPlane plane, int ioff, int joff);
void itrans2 (sMacroBlock* mb, eColorPlane plane);
void iTransform (sMacroBlock* mb, eColorPlane plane, int smb);

void copyImage (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2, int width, int height);
void copyImage16x16 (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2);
void copyImage8x8 (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2);
void copyImage4x4 (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2);
int CheckVertMV (sMacroBlock* mb, int vec1_y, int blockSizeY);

void readCoef4x4cavlc (sMacroBlock* mb, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);
void readCoef4x4cavlc444 (sMacroBlock* mb, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);

void setReadMacroblock (cSlice* slice);
void setReadCbpCoefCavlc (cSlice* slice);
void setReadCompCoefCavlc (sMacroBlock* mb);

int get_colocated_info_8x8 (sMacroBlock* mb, sPicture* list1, int i, int j);
int get_colocated_info_4x4 (sMacroBlock* mb, sPicture* list1, int i, int j);

void setReadCompCabac (sMacroBlock* mb);
void setReadCompCoefCavlc (sMacroBlock* mb);

void setSliceFunctions (cSlice* slice);
void setup_slice_methods_mbaff (cSlice* slice);

void getNeighbours (sMacroBlock* mb, sPixelPos *block, int mb_x, int mb_y, int blockshape_x);

void startMacroblock (cSlice* slice, sMacroBlock** mb);
int decodeMacroblock (sMacroBlock* mb, sPicture* picture);
bool exitMacroblock (cSlice* slice, int eos_bit);

void updateQp (sMacroBlock* mb, int qp);

void checkDpNeighbours (sMacroBlock* mb);
void readDeltaQuant (sSyntaxElement* se, sDataPartition *dataPartition, sMacroBlock* mb,
                            const uint8_t* dpMap, int type);
