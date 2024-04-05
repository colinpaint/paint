#pragma once
//{{{
//! gives CBP value from codeword number, both for intra and inter
static const uint8_t NCBP[2][48][2]=
{
  {  // 0      1        2       3       4       5       6       7       8       9      10      11
    {15, 0},{ 0, 1},{ 7, 2},{11, 4},{13, 8},{14, 3},{ 3, 5},{ 5,10},{10,12},{12,15},{ 1, 7},{ 2,11},
    { 4,13},{ 8,14},{ 6, 6},{ 9, 9},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},
    { 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},
    { 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0}
  },
  {
    {47, 0},{31,16},{15, 1},{ 0, 2},{23, 4},{27, 8},{29,32},{30, 3},{ 7, 5},{11,10},{13,12},{14,15},
    {39,47},{43, 7},{45,11},{46,13},{16,14},{ 3, 6},{ 5, 9},{10,31},{12,35},{19,37},{21,42},{26,44},
    {28,33},{35,34},{37,36},{42,40},{44,39},{ 1,43},{ 2,45},{ 4,46},{ 8,17},{17,18},{18,20},{20,24},
    {24,19},{ 6,21},{ 9,26},{22,28},{25,23},{32,27},{33,29},{34,30},{36,22},{40,25},{38,38},{41,41}
  }
};
//}}}
//{{{
//! for the linfo_levrun_inter routine
static const uint8_t NTAB1[4][8][2] =
{
  {{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,1},{1,2},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{2,0},{1,3},{1,4},{1,5},{0,0},{0,0},{0,0},{0,0}},
  {{3,0},{2,1},{2,2},{1,6},{1,7},{1,8},{1,9},{4,0}},
};
//}}}
//{{{
static const uint8_t LEVRUN1[16]=
{
  4,2,2,1,1,1,1,1,1,1,0,0,0,0,0,0,
};
//}}}
//{{{
static const uint8_t NTAB2[4][8][2] =
{
  {{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,1},{2,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,2},{3,0},{4,0},{5,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,3},{1,4},{2,1},{3,1},{6,0},{7,0},{8,0},{9,0}},
};
//}}}
//{{{
//! for the linfo_levrun__c2x2 routine
static const uint8_t LEVRUN3[4] =
{
  2,1,0,0
};
//}}}
//{{{
static const uint8_t NTAB3[2][2][2] =
{
  {{1,0},{0,0}},
  {{2,0},{1,1}},
};
//}}}

void linfo_ue (int len, int info, int* value1, int* dummy);
void linfo_se (int len, int info, int* value1, int* dummy);
void linfo_cbp_intra_normal (int len, int info,int* codedBlockPattern, int* dummy);
void linfo_cbp_inter_normal (int len, int info, int* codedBlockPattern, int* dummy);
void linfo_cbp_intra_other (int len, int info, int* codedBlockPattern, int* dummy);
void linfo_cbp_inter_other (int len, int info, int* codedBlockPattern, int* dummy);
void linfo_levrun_inter (int len, int info, int* level, int* irun);
void linfo_levrun_c2x2 (int len, int info, int* level, int* irun);

int readSyntaxElementVLC (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* dataPartition);
int GetVLCSymbol_IntraMode (uint8_t buffer[], int totalBitOffset, int* info, int bytecount);
int moreRbspData (uint8_t buffer[], int totalBitOffset, int bytecount);
int vlcStartCode (cSlice* slice, int dummy);
int GetVLCSymbol (uint8_t buffer[], int totalBitOffset, int* info, int bytecount);

int readSyntaxElement_Level_VLC0 (sSyntaxElement* se, cBitStream* s);
int readSyntaxElement_Level_VLCN (sSyntaxElement* se, int vlc, cBitStream* s);
int readSyntaxElement_TotalZeros (sSyntaxElement* se, cBitStream* s);
int readSyntaxElement_TotalZerosChromaDC (cDecoder264* decoder, sSyntaxElement* se, cBitStream* s);
int readSyntaxElement_Run (sSyntaxElement* se, cBitStream* s);

int getBits (uint8_t buffer[], int totalBitOffset, int* info, int bitCount, int numBits);
int ShowBits (uint8_t buffer[], int totalBitOffset, int bitCount, int numBits);
