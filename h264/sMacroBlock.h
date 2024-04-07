#pragma once
#include "global.h"

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

// cabac.h
#define IS_I16MB(MB) (((MB)->mbType == I16MB) || ((MB)->mbType == IPCM))
#define IS_DIRECT(MB) (((MB)->mbType == 0) && (slice->sliceType == eSliceB))

sMotionContexts* createMotionInfoContexts();
sTextureContexts* createTextureInfoContexts();
void deleteMotionInfoContexts (sMotionContexts* contexts);
void deleteTextureInfoContexts (sTextureContexts* contexts);

void cabacNewSlice (cSlice* slice);

void readMB_typeInfo_CABAC_i_slice (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMB_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMB_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readB8_typeInfo_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readB8_typeInfo_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readIntraPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readRefFrame_CABAC (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void read_MVD_CABAC (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void read_mvd_CABAC_mbaff (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void read_CBP_CABAC (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readRunLevel_CABAC (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void read_dQuant_CABAC (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readCIPredMode_CABAC (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void read_skipFlag_CABAC_p_slice (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void read_skipFlag_CABAC_b_slice (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readFieldModeInfo_CABAC (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);
void readMB_transform_sizeFlag_CABAC (sMacroBlock* mb, sSyntaxElement* se, cCabacDecode* cabacDecode);

void readIpcmCabac (cSlice* slice, sDataPartition* dataPartition);
int cabacStartCode (cSlice* slice, int eos_bit);
int readSyntaxElementCABAC (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* this_dataPart);

int checkNextMbGetFieldModeCabacSliceP (cSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);
int checkNextMbGetFieldModeCabacSliceB (cSlice* slice, sSyntaxElement* se, sDataPartition* act_dp);
void checkNeighbourCabac (sMacroBlock* mb);

void setReadStoreCodedBlockPattern (sMacroBlock** mb, int chromaFormatIdc);
void setReadCbpCoefsCabac (cSlice* slice);
void setReadCompCabac (sMacroBlock* mb);

// cavlcRead
void readCoef4x4cavlcNormal (sMacroBlock* mb, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);
void readCoef4x4cavlc444 (sMacroBlock* mb, int block_type, int i, int j, int levarr[16], int runarr[16], int* number_coefficients);
void setReadCompCoefCavlc (sMacroBlock* mb);

// macroblock.h
void copyImage4x4 (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2);
void copyImage8x8 (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2);
void copyImage16x16 (sPixel ** imgBuf1, sPixel ** imgBuf2, int off1, int off2);

void getMbPos (cDecoder264* decoder, int mbIndex, int mbSize[2], int16_t* x, int16_t* y);
void getMbBlockPosNormal (sBlockPos* picPos, int mbIndex, int16_t* x, int16_t* y);
void getMbBlockPosMbaff  (sBlockPos* picPos, int mbIndex, int16_t* x, int16_t* y);

bool checkVertMV(sMacroBlock* mb, int vec1_y, int blockSizeY);
void checkNeighbours (sMacroBlock* mb);
void checkNeighboursMbAff (sMacroBlock* mb);
void checkNeighboursNormal (sMacroBlock* mb);
void getAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos* pixelPos);
void getNonAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos* pixelPos);
void get4x4Neighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos* pixelPos);
void get4x4NeighbourBase (sMacroBlock* mb, int blockX, int blockY, int mbSize[2], sPixelPos* pixelPos);

void itransSpChroma (sMacroBlock* mb, int uv);
void invResidualTransChroma (sMacroBlock* mb, int uv);
void itrans4x4 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff);
void itrans4x4Lossless (sMacroBlock* mb, eColorPlane plane, int ioff, int joff);
void itransSp (sMacroBlock* mb, eColorPlane plane, int ioff, int joff);
void itrans2 (sMacroBlock* mb, eColorPlane plane);
void iTransform (sMacroBlock* mb, eColorPlane plane, int smb);

void checkDpNeighbours (sMacroBlock* mb);
void getNeighbours (sMacroBlock* mb, sPixelPos* block, int mb_x, int mb_y, int blockshape_x);
void updateQp (sMacroBlock* mb, int qp);
void readDeltaQuant (sSyntaxElement* se, sDataPartition* dataPartition, sMacroBlock* mb,
                     const uint8_t* dpMap, int type);

int decodeMacroBlock (sMacroBlock* mb, sPicture* picture);
