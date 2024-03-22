//{{{  includes
#include "global.h"
#include "memory.h"
#include "syntaxElement.h"
#include "defines.h"

#include "image.h"
#include "fmo.h"
#include "nalu.h"
#include "sps.h"
#include "pps.h"
#include "sei.h"
#include "output.h"
#include "mbAccess.h"
#include "macroblock.h"
#include "loopfilter.h"
#include "biariDecode.h"
#include "cabac.h"
#include "vlc.h"
#include "quant.h"
#include "errorConceal.h"
#include "erc.h"
#include "buffer.h"
#include "mcPred.h"

#include <math.h>
#include <limits.h>
//}}}
//{{{  static tables
#define CTX_UNUSED {0,64}
#define CTX_UNDEF  {0,63}

#define NUM_CTX_MODELS_I 1
#define NUM_CTX_MODELS_P 3

//{{{
static const char INIT_MB_TYPE_I[1][3][11][2] =
{
  //----- model 0 -----
  {
    { {  20, -15} , {   2,  54} , {   3,  74} ,  CTX_UNUSED , { -28, 127} , { -23, 104} , {  -6,  53} , {  -1,  54} , {   7,  51} ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  20, -15} , {   2,  54} , {   3,  74} , {  20, -15} , {   2,  54} , {   3,  74} , { -28, 127} , { -23, 104} , {  -6,  53} , {  -1,  54} , {   7,  51} }, // SI (unused at the moment)
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_MB_TYPE_P[3][3][11][2] =
{
  //----- model 0 -----
  {
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
  { {  23,  33} , {  23,   2} , {  21,   0} ,  CTX_UNUSED , {   1,   9} , {   0,  49} , { -37, 118} , {   5,  57} , { -13,  78} , { -11,  65} , {   1,  62} },
  { {  26,  67} , {  16,  90} , {   9, 104} ,  CTX_UNUSED , { -46, 127} , { -20, 104} , {   1,  67} , {  18,  64} , {   9,  43} , {  29,   0} ,  CTX_UNUSED }
  },
  //----- model 1 -----
  {
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  22,  25} , {  34,   0} , {  16,   0} ,  CTX_UNUSED , {  -2,   9} , {   4,  41} , { -29, 118} , {   2,  65} , {  -6,  71} , { -13,  79} , {   5,  52} },
    { {  57,   2} , {  41,  36} , {  26,  69} ,  CTX_UNUSED , { -45, 127} , { -15, 101} , {  -4,  76} , {  26,  34} , {  19,  22} , {  40,   0} ,  CTX_UNUSED }
  },
  //----- model 2 -----
  {
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  29,  16} , {  25,   0} , {  14,   0} ,  CTX_UNUSED , { -10,  51} , {  -3,  62} , { -27,  99} , {  26,  16} , {  -4,  85} , { -24, 102} , {   5,  57} },
  { {  54,   0} , {  37,  42} , {  12,  97} ,  CTX_UNUSED , { -32, 127} , { -22, 117} , {  -2,  74} , {  20,  40} , {  20,  10} , {  29,   0} ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_B8_TYPE_I[1][2][9][2] =
{
  //----- model 0 -----
  {
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_B8_TYPE_P[3][2][9][2] =
{
  //----- model 0 -----
  {
    {  CTX_UNUSED , {  12,  49} ,  CTX_UNUSED , {  -4,  73} , {  17,  50} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -6,  86} , { -17,  95} , {  -6,  61} , {   9,  45} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  },
  //----- model 1 -----
  {
    {  CTX_UNUSED , {   9,  50} ,  CTX_UNUSED , {  -3,  70} , {  10,  54} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   6,  69} , { -13,  90} , {   0,  52} , {   8,  43} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  },
  //----- model 2 -----
  {
    {  CTX_UNUSED , {   6,  57} ,  CTX_UNUSED , { -17,  73} , {  14,  57} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -6,  93} , { -14,  88} , {  -6,  44} , {   4,  55} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_MV_RES_I[1][2][10][2] =
{
  //----- model 0 -----
  {
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_MV_RES_P[3][2][10][2] =
{
  //----- model 0 -----
  {
    { {  -3,  69} ,  CTX_UNUSED , {  -6,  81} , { -11,  96} ,  CTX_UNUSED , {   0,  58} ,  CTX_UNUSED , {  -3,  76} , { -10,  94} ,  CTX_UNUSED },
    { {   6,  55} , {   7,  67} , {  -5,  86} , {   2,  88} ,  CTX_UNUSED , {   5,  54} , {   4,  69} , {  -3,  81} , {   0,  88} ,  CTX_UNUSED }
  },
  //----- model 1 -----
  {
    { {  -2,  69} ,  CTX_UNUSED , {  -5,  82} , { -10,  96} ,  CTX_UNUSED , {   1,  56} ,  CTX_UNUSED , {  -3,  74} , {  -6,  85} ,  CTX_UNUSED },
    { {   2,  59} , {   2,  75} , {  -3,  87} , {  -3, 100} ,  CTX_UNUSED , {   0,  59} , {  -3,  81} , {  -7,  86} , {  -5,  95} ,  CTX_UNUSED }
  },
  //----- model 2 -----
  {
    { { -11,  89} ,  CTX_UNUSED , { -15, 103} , { -21, 116} ,  CTX_UNUSED , {   1,  63} ,  CTX_UNUSED , {  -5,  85} , { -13, 106} ,  CTX_UNUSED },
    { {  19,  57} , {  20,  58} , {   4,  84} , {   6,  96} ,  CTX_UNUSED , {   5,  63} , {   6,  75} , {  -3,  90} , {  -1, 101} ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_REF_NO_I[1][2][6][2] =
{
  //----- model 0 -----
  {
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_REF_NO_P[3][2][6][2] =
{
  //----- model 0 -----
  {
    { {  -7,  67} , {  -5,  74} , {  -4,  74} , {  -5,  80} , {  -7,  72} , {   1,  58} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  },
  //----- model 1 -----
  {
    { {  -1,  66} , {  -1,  77} , {   1,  70} , {  -2,  86} , {  -5,  72} , {   0,  61} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  },
  //----- model 2 -----
  {
    { {   3,  55} , {  -4,  79} , {  -2,  75} , { -12,  97} , {  -7,  50} , {   1,  60} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_TRANSFORM_SIZE_I[1][1][3][2]=
{
  //----- model 0 -----
  {
    {  {  31,  21} , {  31,  31} , {  25,  50} },
//    { {   0,  41} , {   0,  63} , {   0,  63} },
  }
};
//}}}
//{{{
static const char INIT_TRANSFORM_SIZE_P[3][1][3][2]=
{
  //----- model 0 -----
  {
    {  {  12,  40} , {  11,  51} , {  14,  59} },
//    { {   0,  41} , {   0,  63} , {   0,  63} },
  },
  //----- model 1 -----
  {
    {  {  25,  32} , {  21,  49} , {  21,  54} },
//    { {   0,  41} , {   0,  63} , {   0,  63} },
  },
  //----- model 2 -----
  {
    {  {  21,  33} , {  19,  50} , {  17,  61} },
//    { {   0,  41} , {   0,  63} , {   0,  63} },
  }
};
//}}}
//{{{
static const char INIT_DELTA_QP_I[1][1][4][2]=
{
  //----- model 0 -----
  {
    { {   0,  41} , {   0,  63} , {   0,  63} , {   0,  63} },
  }
};
//}}}
//{{{
static const char INIT_DELTA_QP_P[3][1][4][2]=
{
  //----- model 0 -----
  {
    { {   0,  41} , {   0,  63} , {   0,  63} , {   0,  63} },
  },
  //----- model 1 -----
  {
    { {   0,  41} , {   0,  63} , {   0,  63} , {   0,  63} },
  },
  //----- model 2 -----
  {
    { {   0,  41} , {   0,  63} , {   0,  63} , {   0,  63} },
  }
};
//}}}
//{{{
static const char INIT_MB_AFF_I[1][1][4][2] =
{
  //----- model 0 -----
  {
    { {   0,  11} , {   1,  55} , {   0,  69} ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_MB_AFF_P[3][1][4][2] =
{
  //----- model 0 -----
  {
    { {   0,  45} , {  -4,  78} , {  -3,  96} ,  CTX_UNUSED }
  },
  //----- model 1 -----
  {
    { {  13,  15} , {   7,  51} , {   2,  80} ,  CTX_UNUSED }
  },
  //----- model 2 -----
  {
    { {   7,  34} , {  -9,  88} , { -20, 127} ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_IPR_I[1][1][2][2] =
{
  //----- model 0 -----
  {
    { { 13,  41} , {   3,  62} }
  }
};
//}}}
//{{{
static const char INIT_IPR_P[3][1][2][2] =
{
  //----- model 0 -----
  {
    { { 13,  41} , {   3,  62} }
  },
  //----- model 1 -----
  {
    { { 13,  41} , {   3,  62} }
  },
  //----- model 2 -----
  {
    { { 13,  41} , {   3,  62} }
  }
};
//}}}
//{{{
static const char INIT_CIPR_I[1][1][4][2] =
{
  //----- model 0 -----
  {
    { {  -9,  83} , {   4,  86} , {   0,  97} , {  -7,  72} }
  }
};
//}}}
//{{{
static const char INIT_CIPR_P[3][1][4][2] =
{
  //----- model 0 -----
  {
    { {  -9,  83} , {   4,  86} , {   0,  97} , {  -7,  72} }
  },
  //----- model 1 -----
  {
    { {  -9,  83} , {   4,  86} , {   0,  97} , {  -7,  72} }
  },
  //----- model 2 -----
  {
    { {  -9,  83} , {   4,  86} , {   0,  97} , {  -7,  72} }
  }
};
//}}}
//{{{
static const char INIT_CBP_I[1][3][4][2] =
{
  //----- model 0 -----
  {
    { { -17, 127} , { -13, 102} , {   0,  82} , {  -7,  74} },
    { { -21, 107} , { -27, 127} , { -31, 127} , { -24, 127} },
    { { -18,  95} , { -27, 127} , { -21, 114} , { -30, 127} }
  }
};
//}}}
//{{{
static const char INIT_CBP_P[3][3][4][2] =
{
  //----- model 0 -----
  {
    { { -27, 126} , { -28,  98} , { -25, 101} , { -23,  67} },
    { { -28,  82} , { -20,  94} , { -16,  83} , { -22, 110} },
    { { -21,  91} , { -18, 102} , { -13,  93} , { -29, 127} }
  },
  //----- model 1 -----
  {
    { { -39, 127} , { -18,  91} , { -17,  96} , { -26,  81} },
    { { -35,  98} , { -24, 102} , { -23,  97} , { -27, 119} },
    { { -24,  99} , { -21, 110} , { -18, 102} , { -36, 127} }
  },
  //----- model 2 -----
  {
    { { -36, 127} , { -17,  91} , { -14,  95} , { -25,  84} },
    { { -25,  86} , { -12,  89} , { -17,  91} , { -31, 127} },
    { { -14,  76} , { -18, 103} , { -13,  90} , { -37, 127} }
  }
};
//}}}
//{{{
static const char INIT_BCBP_I[1][22][4][2] =
{
  //----- model 0 -----
  {
    { { -17, 123} , { -12, 115} , { -16, 122} , { -11, 115} },
    { { -12,  63} , {  -2,  68} , { -15,  84} , { -13, 104} },
    { {  -3,  70} , {  -8,  93} , { -10,  90} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -3,  70} , {  -8,  93} , { -10,  90} , { -30, 127} },
    { {  -1,  74} , {  -6,  97} , {  -7,  91} , { -20, 127} },
    { {  -4,  56} , {  -5,  82} , {  -7,  76} , { -22, 125} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    // Cb in the 4:4:4 common mode
    { { -17, 123} , { -12, 115} , { -16, 122} , { -11, 115} },
    { { -12,  63} , {  -2,  68} , { -15,  84} , { -13, 104} },
    { {  -3,  70} , {  -8,  93} , { -10,  90} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -3,  70} , {  -8,  93} , { -10,  90} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    // Cr in the 4:4:4 common mode
    { { -17, 123} , { -12, 115} , { -16, 122} , { -11, 115} },
    { { -12,  63} , {  -2,  68} , { -15,  84} , { -13, 104} },
    { {  -3,  70} , {  -8,  93} , { -10,  90} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -3,  70} , {  -8,  93} , { -10,  90} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_BCBP_P[3][22][4][2] =
{
  //----- model 0 -----
  {
    { {  -7,  92} , {  -5,  89} , {  -7,  96} , { -13, 108} },
    { {  -3,  46} , {  -1,  65} , {  -1,  57} , {  -9,  93} },
    { {  -3,  74} , {  -9,  92} , {  -8,  87} , { -23, 126} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -3,  74} , {  -9,  92} , {  -8,  87} , { -23, 126} },
    { {   5,  54} , {   6,  60} , {   6,  59} , {   6,  69} },
    { {  -1,  48} , {   0,  68} , {  -4,  69} , {  -8,  88} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    // Cb in the 4:4:4 common mode
    { {  -7,  92} , {  -5,  89} , {  -7,  96} , { -13, 108} },
    { {  -3,  46} , {  -1,  65} , {  -1,  57} , {  -9,  93} },
    { {  -3,  74} , {  -9,  92} , {  -8,  87} , { -23, 126} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -3,  74} , {  -9,  92} , {  -8,  87} , { -23, 126} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    // Cr in the 4:4:4 common mode
    { {  -7,  92} , {  -5,  89} , {  -7,  96} , { -13, 108} },
    { {  -3,  46} , {  -1,  65} , {  -1,  57} , {  -9,  93} },
    { {  -3,  74} , {  -9,  92} , {  -8,  87} , { -23, 126} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -3,  74} , {  -9,  92} , {  -8,  87} , { -23, 126} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  },
  //----- model 1 -----
  {
    { {   0,  80} , {  -5,  89} , {  -7,  94} , {  -4,  92} },
    { {   0,  39} , {   0,  65} , { -15,  84} , { -35, 127} },
    { {  -2,  73} , { -12, 104} , {  -9,  91} , { -31, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -2,  73} , { -12, 104} , {  -9,  91} , { -31, 127} },
    { {   3,  55} , {   7,  56} , {   7,  55} , {   8,  61} },
    { {  -3,  53} , {   0,  68} , {  -7,  74} , {  -9,  88} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    // Cb in the 4:4:4 common mode
    { {   0,  80} , {  -5,  89} , {  -7,  94} , {  -4,  92} },
    { {   0,  39} , {   0,  65} , { -15,  84} , { -35, 127} },
    { {  -2,  73} , { -12, 104} , {  -9,  91} , { -31, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -2,  73} , { -12, 104} , {  -9,  91} , { -31, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    // Cr in the 4:4:4 common mode
    { {   0,  80} , {  -5,  89} , {  -7,  94} , {  -4,  92} },
    { {   0,  39} , {   0,  65} , { -15,  84} , { -35, 127} },
    { {  -2,  73} , { -12, 104} , {  -9,  91} , { -31, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -2,  73} , { -12, 104} , {  -9,  91} , { -31, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  },
  //----- model 2 -----
  {
    { {  11,  80} , {   5,  76} , {   2,  84} , {   5,  78} },
    { {  -6,  55} , {   4,  61} , { -14,  83} , { -37, 127} },
    { {  -5,  79} , { -11, 104} , { -11,  91} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -5,  79} , { -11, 104} , { -11,  91} , { -30, 127} },
    { {   0,  65} , {  -2,  79} , {   0,  72} , {  -4,  92} },
    { {  -6,  56} , {   3,  68} , {  -8,  71} , { -13,  98} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    // Cb in the 4:4:4 common mode
    { {  11,  80} , {   5,  76} , {   2,  84} , {   5,  78} },
    { {  -6,  55} , {   4,  61} , { -14,  83} , { -37, 127} },
    { {  -5,  79} , { -11, 104} , { -11,  91} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -5,  79} , { -11, 104} , { -11,  91} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    // Cr in the 4:4:4 common mode
    { {  11,  80} , {   5,  76} , {   2,  84} , {   5,  78} },
    { {  -6,  55} , {   4,  61} , { -14,  83} , { -37, 127} },
    { {  -5,  79} , { -11, 104} , { -11,  91} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -5,  79} , { -11, 104} , { -11,  91} , { -30, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_MAP_I[1][22][15][2] =
{
  //----- model 0 -----
  {
  { {  -7,  93} , { -11,  87} , {  -3,  77} , {  -5,  71} , {  -4,  63} , {  -4,  68} , { -12,  84} , {  -7,  62} , {  -7,  65} , {   8,  61} , {   5,  56} , {  -2,  66} , {   1,  64} , {   0,  61} , {  -2,  78} },
    {  CTX_UNUSED , {   1,  50} , {   7,  52} , {  10,  35} , {   0,  44} , {  11,  38} , {   1,  45} , {   0,  46} , {   5,  44} , {  31,  17} , {   1,  51} , {   7,  50} , {  28,  19} , {  16,  33} , {  14,  62} },
    { { -17, 120} , { -20, 112} , { -18, 114} , { -11,  85} , { -15,  92} , { -14,  89} , { -26,  71} , { -15,  81} , { -14,  80} , {   0,  68} , { -14,  70} , { -24,  56} , { -23,  68} , { -24,  50} , { -11,  74} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -13, 108} , { -15, 100} , { -13, 101} , { -13,  91} , { -12,  94} , { -10,  88} , { -16,  84} , { -10,  86} , {  -7,  83} , { -13,  87} , { -19,  94} , {   1,  70} , {   0,  72} , {  -5,  74} , {  18,  59} },
    { {  -8, 102} , { -15, 100} , {   0,  95} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  -4,  75} , {   2,  72} , { -11,  75} , {  -3,  71} , {  15,  46} , { -13,  69} , {   0,  62} , {   0,  65} , {  21,  37} , { -15,  72} , {   9,  57} , {  16,  54} , {   0,  62} , {  12,  72} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  -7,  93} , { -11,  87} , {  -3,  77} , {  -5,  71} , {  -4,  63} , {  -4,  68} , { -12,  84} , {  -7,  62} , {  -7,  65} , {   8,  61} , {   5,  56} , {  -2,  66} , {   1,  64} , {   0,  61} , {  -2,  78} },
    {  CTX_UNUSED , {   1,  50} , {   7,  52} , {  10,  35} , {   0,  44} , {  11,  38} , {   1,  45} , {   0,  46} , {   5,  44} , {  31,  17} , {   1,  51} , {   7,  50} , {  28,  19} , {  16,  33} , {  14,  62} },
    { { -17, 120} , { -20, 112} , { -18, 114} , { -11,  85} , { -15,  92} , { -14,  89} , { -26,  71} , { -15,  81} , { -14,  80} , {   0,  68} , { -14,  70} , { -24,  56} , { -23,  68} , { -24,  50} , { -11,  74} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -13, 108} , { -15, 100} , { -13, 101} , { -13,  91} , { -12,  94} , { -10,  88} , { -16,  84} , { -10,  86} , {  -7,  83} , { -13,  87} , { -19,  94} , {   1,  70} , {   0,  72} , {  -5,  74} , {  18,  59} },
    //Cr in the 4:4:4 common mode
    { {  -7,  93} , { -11,  87} , {  -3,  77} , {  -5,  71} , {  -4,  63} , {  -4,  68} , { -12,  84} , {  -7,  62} , {  -7,  65} , {   8,  61} , {   5,  56} , {  -2,  66} , {   1,  64} , {   0,  61} , {  -2,  78} },
    {  CTX_UNUSED , {   1,  50} , {   7,  52} , {  10,  35} , {   0,  44} , {  11,  38} , {   1,  45} , {   0,  46} , {   5,  44} , {  31,  17} , {   1,  51} , {   7,  50} , {  28,  19} , {  16,  33} , {  14,  62} },
    { { -17, 120} , { -20, 112} , { -18, 114} , { -11,  85} , { -15,  92} , { -14,  89} , { -26,  71} , { -15,  81} , { -14,  80} , {   0,  68} , { -14,  70} , { -24,  56} , { -23,  68} , { -24,  50} , { -11,  74} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -13, 108} , { -15, 100} , { -13, 101} , { -13,  91} , { -12,  94} , { -10,  88} , { -16,  84} , { -10,  86} , {  -7,  83} , { -13,  87} , { -19,  94} , {   1,  70} , {   0,  72} , {  -5,  74} , {  18,  59} }
  }
};
//}}}
//{{{
static const char INIT_MAP_P[3][22][15][2] =
{
  //----- model 0 -----
  {
    { {  -2,  85} , {  -6,  78} , {  -1,  75} , {  -7,  77} , {   2,  54} , {   5,  50} , {  -3,  68} , {   1,  50} , {   6,  42} , {  -4,  81} , {   1,  63} , {  -4,  70} , {   0,  67} , {   2,  57} , {  -2,  76} },
    {  CTX_UNUSED , {  11,  35} , {   4,  64} , {   1,  61} , {  11,  35} , {  18,  25} , {  12,  24} , {  13,  29} , {  13,  36} , { -10,  93} , {  -7,  73} , {  -2,  73} , {  13,  46} , {   9,  49} , {  -7, 100} },
    { {  -4,  79} , {  -7,  71} , {  -5,  69} , {  -9,  70} , {  -8,  66} , { -10,  68} , { -19,  73} , { -12,  69} , { -16,  70} , { -15,  67} , { -20,  62} , { -19,  70} , { -16,  66} , { -22,  65} , { -20,  63} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   9,  53} , {   2,  53} , {   5,  53} , {  -2,  61} , {   0,  56} , {   0,  56} , { -13,  63} , {  -5,  60} , {  -1,  62} , {   4,  57} , {  -6,  69} , {   4,  57} , {  14,  39} , {   4,  51} , {  13,  68} },
    { {   3,  64} , {   1,  61} , {   9,  63} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {   7,  50} , {  16,  39} , {   5,  44} , {   4,  52} , {  11,  48} , {  -5,  60} , {  -1,  59} , {   0,  59} , {  22,  33} , {   5,  44} , {  14,  43} , {  -1,  78} , {   0,  60} , {   9,  69} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  -2,  85} , {  -6,  78} , {  -1,  75} , {  -7,  77} , {   2,  54} , {   5,  50} , {  -3,  68} , {   1,  50} , {   6,  42} , {  -4,  81} , {   1,  63} , {  -4,  70} , {   0,  67} , {   2,  57} , {  -2,  76} },
    {  CTX_UNUSED , {  11,  35} , {   4,  64} , {   1,  61} , {  11,  35} , {  18,  25} , {  12,  24} , {  13,  29} , {  13,  36} , { -10,  93} , {  -7,  73} , {  -2,  73} , {  13,  46} , {   9,  49} , {  -7, 100} },
    { {  -4,  79} , {  -7,  71} , {  -5,  69} , {  -9,  70} , {  -8,  66} , { -10,  68} , { -19,  73} , { -12,  69} , { -16,  70} , { -15,  67} , { -20,  62} , { -19,  70} , { -16,  66} , { -22,  65} , { -20,  63} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   9,  53} , {   2,  53} , {   5,  53} , {  -2,  61} , {   0,  56} , {   0,  56} , { -13,  63} , {  -5,  60} , {  -1,  62} , {   4,  57} , {  -6,  69} , {   4,  57} , {  14,  39} , {   4,  51} , {  13,  68} },
    //Cr in the 4:4:4 common mode
    { {  -2,  85} , {  -6,  78} , {  -1,  75} , {  -7,  77} , {   2,  54} , {   5,  50} , {  -3,  68} , {   1,  50} , {   6,  42} , {  -4,  81} , {   1,  63} , {  -4,  70} , {   0,  67} , {   2,  57} , {  -2,  76} },
    {  CTX_UNUSED , {  11,  35} , {   4,  64} , {   1,  61} , {  11,  35} , {  18,  25} , {  12,  24} , {  13,  29} , {  13,  36} , { -10,  93} , {  -7,  73} , {  -2,  73} , {  13,  46} , {   9,  49} , {  -7, 100} },
    { {  -4,  79} , {  -7,  71} , {  -5,  69} , {  -9,  70} , {  -8,  66} , { -10,  68} , { -19,  73} , { -12,  69} , { -16,  70} , { -15,  67} , { -20,  62} , { -19,  70} , { -16,  66} , { -22,  65} , { -20,  63} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   9,  53} , {   2,  53} , {   5,  53} , {  -2,  61} , {   0,  56} , {   0,  56} , { -13,  63} , {  -5,  60} , {  -1,  62} , {   4,  57} , {  -6,  69} , {   4,  57} , {  14,  39} , {   4,  51} , {  13,  68} }
  },
  //----- model 1 -----
  {
    { { -13, 103} , { -13,  91} , {  -9,  89} , { -14,  92} , {  -8,  76} , { -12,  87} , { -23, 110} , { -24, 105} , { -10,  78} , { -20, 112} , { -17,  99} , { -78, 127} , { -70, 127} , { -50, 127} , { -46, 127} },
    {  CTX_UNUSED , {  -4,  66} , {  -5,  78} , {  -4,  71} , {  -8,  72} , {   2,  59} , {  -1,  55} , {  -7,  70} , {  -6,  75} , {  -8,  89} , { -34, 119} , {  -3,  75} , {  32,  20} , {  30,  22} , { -44, 127} },
    { {  -5,  85} , {  -6,  81} , { -10,  77} , {  -7,  81} , { -17,  80} , { -18,  73} , {  -4,  74} , { -10,  83} , {  -9,  71} , {  -9,  67} , {  -1,  61} , {  -8,  66} , { -14,  66} , {   0,  59} , {   2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   0,  54} , {  -5,  61} , {   0,  58} , {  -1,  60} , {  -3,  61} , {  -8,  67} , { -25,  84} , { -14,  74} , {  -5,  65} , {   5,  52} , {   2,  57} , {   0,  61} , {  -9,  69} , { -11,  70} , {  18,  55} },
    { {  -4,  71} , {   0,  58} , {   7,  61} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {   9,  41} , {  18,  25} , {   9,  32} , {   5,  43} , {   9,  47} , {   0,  44} , {   0,  51} , {   2,  46} , {  19,  38} , {  -4,  66} , {  15,  38} , {  12,  42} , {   9,  34} , {   0,  89} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { { -13, 103} , { -13,  91} , {  -9,  89} , { -14,  92} , {  -8,  76} , { -12,  87} , { -23, 110} , { -24, 105} , { -10,  78} , { -20, 112} , { -17,  99} , { -78, 127} , { -70, 127} , { -50, 127} , { -46, 127} },
    {  CTX_UNUSED , {  -4,  66} , {  -5,  78} , {  -4,  71} , {  -8,  72} , {   2,  59} , {  -1,  55} , {  -7,  70} , {  -6,  75} , {  -8,  89} , { -34, 119} , {  -3,  75} , {  32,  20} , {  30,  22} , { -44, 127} },
    { {  -5,  85} , {  -6,  81} , { -10,  77} , {  -7,  81} , { -17,  80} , { -18,  73} , {  -4,  74} , { -10,  83} , {  -9,  71} , {  -9,  67} , {  -1,  61} , {  -8,  66} , { -14,  66} , {   0,  59} , {   2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   0,  54} , {  -5,  61} , {   0,  58} , {  -1,  60} , {  -3,  61} , {  -8,  67} , { -25,  84} , { -14,  74} , {  -5,  65} , {   5,  52} , {   2,  57} , {   0,  61} , {  -9,  69} , { -11,  70} , {  18,  55} },
    //Cr in the 4:4:4 common mode
    { { -13, 103} , { -13,  91} , {  -9,  89} , { -14,  92} , {  -8,  76} , { -12,  87} , { -23, 110} , { -24, 105} , { -10,  78} , { -20, 112} , { -17,  99} , { -78, 127} , { -70, 127} , { -50, 127} , { -46, 127} },
    {  CTX_UNUSED , {  -4,  66} , {  -5,  78} , {  -4,  71} , {  -8,  72} , {   2,  59} , {  -1,  55} , {  -7,  70} , {  -6,  75} , {  -8,  89} , { -34, 119} , {  -3,  75} , {  32,  20} , {  30,  22} , { -44, 127} },
    { {  -5,  85} , {  -6,  81} , { -10,  77} , {  -7,  81} , { -17,  80} , { -18,  73} , {  -4,  74} , { -10,  83} , {  -9,  71} , {  -9,  67} , {  -1,  61} , {  -8,  66} , { -14,  66} , {   0,  59} , {   2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   0,  54} , {  -5,  61} , {   0,  58} , {  -1,  60} , {  -3,  61} , {  -8,  67} , { -25,  84} , { -14,  74} , {  -5,  65} , {   5,  52} , {   2,  57} , {   0,  61} , {  -9,  69} , { -11,  70} , {  18,  55} }
  },
  //----- model 2 -----
  {
    { {  -4,  86} , { -12,  88} , {  -5,  82} , {  -3,  72} , {  -4,  67} , {  -8,  72} , { -16,  89} , {  -9,  69} , {  -1,  59} , {   5,  66} , {   4,  57} , {  -4,  71} , {  -2,  71} , {   2,  58} , {  -1,  74} },
    {  CTX_UNUSED , {  -4,  44} , {  -1,  69} , {   0,  62} , {  -7,  51} , {  -4,  47} , {  -6,  42} , {  -3,  41} , {  -6,  53} , {   8,  76} , {  -9,  78} , { -11,  83} , {   9,  52} , {   0,  67} , {  -5,  90} },
    {  {  -3,  78} , {  -8,  74} , {  -9,  72} , { -10,  72} , { -18,  75} , { -12,  71} , { -11,  63} , {  -5,  70} , { -17,  75} , { -14,  72} , { -16,  67} , {  -8,  53} , { -14,  59} , {  -9,  52} , { -11,  68} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   1,  67} , { -15,  72} , {  -5,  75} , {  -8,  80} , { -21,  83} , { -21,  64} , { -13,  31} , { -25,  64} , { -29,  94} , {   9,  75} , {  17,  63} , {  -8,  74} , {  -5,  35} , {  -2,  27} , {  13,  91} },
    { {   3,  65} , {  -7,  69} , {   8,  77} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , { -10,  66} , {   3,  62} , {  -3,  68} , { -20,  81} , {   0,  30} , {   1,   7} , {  -3,  23} , { -21,  74} , {  16,  66} , { -23, 124} , {  17,  37} , {  44, -18} , {  50, -34} , { -22, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  -4,  86} , { -12,  88} , {  -5,  82} , {  -3,  72} , {  -4,  67} , {  -8,  72} , { -16,  89} , {  -9,  69} , {  -1,  59} , {   5,  66} , {   4,  57} , {  -4,  71} , {  -2,  71} , {   2,  58} , {  -1,  74} },
    {  CTX_UNUSED , {  -4,  44} , {  -1,  69} , {   0,  62} , {  -7,  51} , {  -4,  47} , {  -6,  42} , {  -3,  41} , {  -6,  53} , {   8,  76} , {  -9,  78} , { -11,  83} , {   9,  52} , {   0,  67} , {  -5,  90} },
    { {  -3,  78} , {  -8,  74} , {  -9,  72} , { -10,  72} , { -18,  75} , { -12,  71} , { -11,  63} , {  -5,  70} , { -17,  75} , { -14,  72} , { -16,  67} , {  -8,  53} , { -14,  59} , {  -9,  52} , { -11,  68} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   1,  67} , { -15,  72} , {  -5,  75} , {  -8,  80} , { -21,  83} , { -21,  64} , { -13,  31} , { -25,  64} , { -29,  94} , {   9,  75} , {  17,  63} , {  -8,  74} , {  -5,  35} , {  -2,  27} , {  13,  91} },
    //Cr in the 4:4:4 common mode
    { {  -4,  86} , { -12,  88} , {  -5,  82} , {  -3,  72} , {  -4,  67} , {  -8,  72} , { -16,  89} , {  -9,  69} , {  -1,  59} , {   5,  66} , {   4,  57} , {  -4,  71} , {  -2,  71} , {   2,  58} , {  -1,  74} },
    {  CTX_UNUSED , {  -4,  44} , {  -1,  69} , {   0,  62} , {  -7,  51} , {  -4,  47} , {  -6,  42} , {  -3,  41} , {  -6,  53} , {   8,  76} , {  -9,  78} , { -11,  83} , {   9,  52} , {   0,  67} , {  -5,  90} },
    { {  -3,  78} , {  -8,  74} , {  -9,  72} , { -10,  72} , { -18,  75} , { -12,  71} , { -11,  63} , {  -5,  70} , { -17,  75} , { -14,  72} , { -16,  67} , {  -8,  53} , { -14,  59} , {  -9,  52} , { -11,  68} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   1,  67} , { -15,  72} , {  -5,  75} , {  -8,  80} , { -21,  83} , { -21,  64} , { -13,  31} , { -25,  64} , { -29,  94} , {   9,  75} , {  17,  63} , {  -8,  74} , {  -5,  35} , {  -2,  27} , {  13,  91} }
  }
};
//}}}
//{{{
static const char INIT_LAST_I[1][22][15][2] =
{
  //----- model 0 -----
  {
    { {  24,   0} , {  15,   9} , {   8,  25} , {  13,  18} , {  15,   9} , {  13,  19} , {  10,  37} , {  12,  18} , {   6,  29} , {  20,  33} , {  15,  30} , {   4,  45} , {   1,  58} , {   0,  62} , {   7,  61} },
    {  CTX_UNUSED , {  12,  38} , {  11,  45} , {  15,  39} , {  11,  42} , {  13,  44} , {  16,  45} , {  12,  41} , {  10,  49} , {  30,  34} , {  18,  42} , {  10,  55} , {  17,  51} , {  17,  46} , {   0,  89} },
    {  {  23, -13} , {  26, -13} , {  40, -15} , {  49, -14} , {  44,   3} , {  45,   6} , {  44,  34} , {  33,  54} , {  19,  82} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  26, -19} , {  22, -17} , {  26, -17} , {  30, -25} , {  28, -20} , {  33, -23} , {  37, -27} , {  33, -23} , {  40, -28} , {  38, -17} , {  33, -11} , {  40, -15} , {  41,  -6} , {  38,   1} , {  41,  17} },
    { {  30,  -6} , {  27,   3} , {  26,  22} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  37, -16} , {  35,  -4} , {  38,  -8} , {  38,  -3} , {  37,   3} , {  38,   5} , {  42,   0} , {  35,  16} , {  39,  22} , {  14,  48} , {  27,  37} , {  21,  60} , {  12,  68} , {   2,  97} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  24,   0} , {  15,   9} , {   8,  25} , {  13,  18} , {  15,   9} , {  13,  19} , {  10,  37} , {  12,  18} , {   6,  29} , {  20,  33} , {  15,  30} , {   4,  45} , {   1,  58} , {   0,  62} , {   7,  61} },
    {  CTX_UNUSED , {  12,  38} , {  11,  45} , {  15,  39} , {  11,  42} , {  13,  44} , {  16,  45} , {  12,  41} , {  10,  49} , {  30,  34} , {  18,  42} , {  10,  55} , {  17,  51} , {  17,  46} , {   0,  89} },
    {  {  23, -13} , {  26, -13} , {  40, -15} , {  49, -14} , {  44,   3} , {  45,   6} , {  44,  34} , {  33,  54} , {  19,  82} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  26, -19} , {  22, -17} , {  26, -17} , {  30, -25} , {  28, -20} , {  33, -23} , {  37, -27} , {  33, -23} , {  40, -28} , {  38, -17} , {  33, -11} , {  40, -15} , {  41,  -6} , {  38,   1} , {  41,  17} },
    //Cr in the 4:4:4 common mode
    { {  24,   0} , {  15,   9} , {   8,  25} , {  13,  18} , {  15,   9} , {  13,  19} , {  10,  37} , {  12,  18} , {   6,  29} , {  20,  33} , {  15,  30} , {   4,  45} , {   1,  58} , {   0,  62} , {   7,  61} },
    {  CTX_UNUSED , {  12,  38} , {  11,  45} , {  15,  39} , {  11,  42} , {  13,  44} , {  16,  45} , {  12,  41} , {  10,  49} , {  30,  34} , {  18,  42} , {  10,  55} , {  17,  51} , {  17,  46} , {   0,  89} },
    {  {  23, -13} , {  26, -13} , {  40, -15} , {  49, -14} , {  44,   3} , {  45,   6} , {  44,  34} , {  33,  54} , {  19,  82} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  26, -19} , {  22, -17} , {  26, -17} , {  30, -25} , {  28, -20} , {  33, -23} , {  37, -27} , {  33, -23} , {  40, -28} , {  38, -17} , {  33, -11} , {  40, -15} , {  41,  -6} , {  38,   1} , {  41,  17} }
  }
};
//}}}
//{{{
static const char INIT_LAST_P[3][22][15][2] =
{
  //----- model 0 -----
  {
    { {  11,  28} , {   2,  40} , {   3,  44} , {   0,  49} , {   0,  46} , {   2,  44} , {   2,  51} , {   0,  47} , {   4,  39} , {   2,  62} , {   6,  46} , {   0,  54} , {   3,  54} , {   2,  58} , {   4,  63} },
    {  CTX_UNUSED , {   6,  51} , {   6,  57} , {   7,  53} , {   6,  52} , {   6,  55} , {  11,  45} , {  14,  36} , {   8,  53} , {  -1,  82} , {   7,  55} , {  -3,  78} , {  15,  46} , {  22,  31} , {  -1,  84} },
    {  {   9,  -2} , {  26,  -9} , {  33,  -9} , {  39,  -7} , {  41,  -2} , {  45,   3} , {  49,   9} , {  45,  27} , {  36,  59} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  25,   7} , {  30,  -7} , {  28,   3} , {  28,   4} , {  32,   0} , {  34,  -1} , {  30,   6} , {  30,   6} , {  32,   9} , {  31,  19} , {  26,  27} , {  26,  30} , {  37,  20} , {  28,  34} , {  17,  70} },
    { {   1,  67} , {   5,  59} , {   9,  67} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  16,  30} , {  18,  32} , {  18,  35} , {  22,  29} , {  24,  31} , {  23,  38} , {  18,  43} , {  20,  41} , {  11,  63} , {   9,  59} , {   9,  64} , {  -1,  94} , {  -2,  89} , {  -9, 108} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  11,  28} , {   2,  40} , {   3,  44} , {   0,  49} , {   0,  46} , {   2,  44} , {   2,  51} , {   0,  47} , {   4,  39} , {   2,  62} , {   6,  46} , {   0,  54} , {   3,  54} , {   2,  58} , {   4,  63} },
    {  CTX_UNUSED , {   6,  51} , {   6,  57} , {   7,  53} , {   6,  52} , {   6,  55} , {  11,  45} , {  14,  36} , {   8,  53} , {  -1,  82} , {   7,  55} , {  -3,  78} , {  15,  46} , {  22,  31} , {  -1,  84} },
    {  {   9,  -2} , {  26,  -9} , {  33,  -9} , {  39,  -7} , {  41,  -2} , {  45,   3} , {  49,   9} , {  45,  27} , {  36,  59} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  25,   7} , {  30,  -7} , {  28,   3} , {  28,   4} , {  32,   0} , {  34,  -1} , {  30,   6} , {  30,   6} , {  32,   9} , {  31,  19} , {  26,  27} , {  26,  30} , {  37,  20} , {  28,  34} , {  17,  70} },
    //Cr in the 4:4:4 common mode
    { {  11,  28} , {   2,  40} , {   3,  44} , {   0,  49} , {   0,  46} , {   2,  44} , {   2,  51} , {   0,  47} , {   4,  39} , {   2,  62} , {   6,  46} , {   0,  54} , {   3,  54} , {   2,  58} , {   4,  63} },
    {  CTX_UNUSED , {   6,  51} , {   6,  57} , {   7,  53} , {   6,  52} , {   6,  55} , {  11,  45} , {  14,  36} , {   8,  53} , {  -1,  82} , {   7,  55} , {  -3,  78} , {  15,  46} , {  22,  31} , {  -1,  84} },
    {  {   9,  -2} , {  26,  -9} , {  33,  -9} , {  39,  -7} , {  41,  -2} , {  45,   3} , {  49,   9} , {  45,  27} , {  36,  59} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  25,   7} , {  30,  -7} , {  28,   3} , {  28,   4} , {  32,   0} , {  34,  -1} , {  30,   6} , {  30,   6} , {  32,   9} , {  31,  19} , {  26,  27} , {  26,  30} , {  37,  20} , {  28,  34} , {  17,  70} }
  },
  //----- model 1 -----
  {
    { {   4,  45} , {  10,  28} , {  10,  31} , {  33, -11} , {  52, -43} , {  18,  15} , {  28,   0} , {  35, -22} , {  38, -25} , {  34,   0} , {  39, -18} , {  32, -12} , { 102, -94} , {   0,   0} , {  56, -15} },
    {  CTX_UNUSED , {  33,  -4} , {  29,  10} , {  37,  -5} , {  51, -29} , {  39,  -9} , {  52, -34} , {  69, -58} , {  67, -63} , {  44,  -5} , {  32,   7} , {  55, -29} , {  32,   1} , {   0,   0} , {  27,  36} },
    {  {  17, -10} , {  32, -13} , {  42,  -9} , {  49,  -5} , {  53,   0} , {  64,   3} , {  68,  10} , {  66,  27} , {  47,  57} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  33, -25} , {  34, -30} , {  36, -28} , {  38, -28} , {  38, -27} , {  34, -18} , {  35, -16} , {  34, -14} , {  32,  -8} , {  37,  -6} , {  35,   0} , {  30,  10} , {  28,  18} , {  26,  25} , {  29,  41} },
    { {   0,  75} , {   2,  72} , {   8,  77} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  14,  35} , {  18,  31} , {  17,  35} , {  21,  30} , {  17,  45} , {  20,  42} , {  18,  45} , {  27,  26} , {  16,  54} , {   7,  66} , {  16,  56} , {  11,  73} , {  10,  67} , { -10, 116} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {   4,  45} , {  10,  28} , {  10,  31} , {  33, -11} , {  52, -43} , {  18,  15} , {  28,   0} , {  35, -22} , {  38, -25} , {  34,   0} , {  39, -18} , {  32, -12} , { 102, -94} , {   0,   0} , {  56, -15} },
    {  CTX_UNUSED , {  33,  -4} , {  29,  10} , {  37,  -5} , {  51, -29} , {  39,  -9} , {  52, -34} , {  69, -58} , {  67, -63} , {  44,  -5} , {  32,   7} , {  55, -29} , {  32,   1} , {   0,   0} , {  27,  36} },
    {  {  17, -10} , {  32, -13} , {  42,  -9} , {  49,  -5} , {  53,   0} , {  64,   3} , {  68,  10} , {  66,  27} , {  47,  57} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  33, -25} , {  34, -30} , {  36, -28} , {  38, -28} , {  38, -27} , {  34, -18} , {  35, -16} , {  34, -14} , {  32,  -8} , {  37,  -6} , {  35,   0} , {  30,  10} , {  28,  18} , {  26,  25} , {  29,  41} },
    //Cr in the 4:4:4 common mode
    { {   4,  45} , {  10,  28} , {  10,  31} , {  33, -11} , {  52, -43} , {  18,  15} , {  28,   0} , {  35, -22} , {  38, -25} , {  34,   0} , {  39, -18} , {  32, -12} , { 102, -94} , {   0,   0} , {  56, -15} },
    {  CTX_UNUSED , {  33,  -4} , {  29,  10} , {  37,  -5} , {  51, -29} , {  39,  -9} , {  52, -34} , {  69, -58} , {  67, -63} , {  44,  -5} , {  32,   7} , {  55, -29} , {  32,   1} , {   0,   0} , {  27,  36} },
    {  {  17, -10} , {  32, -13} , {  42,  -9} , {  49,  -5} , {  53,   0} , {  64,   3} , {  68,  10} , {  66,  27} , {  47,  57} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  33, -25} , {  34, -30} , {  36, -28} , {  38, -28} , {  38, -27} , {  34, -18} , {  35, -16} , {  34, -14} , {  32,  -8} , {  37,  -6} , {  35,   0} , {  30,  10} , {  28,  18} , {  26,  25} , {  29,  41} }
  },
  //----- model 2 -----
  {
    { {   4,  39} , {   0,  42} , {   7,  34} , {  11,  29} , {   8,  31} , {   6,  37} , {   7,  42} , {   3,  40} , {   8,  33} , {  13,  43} , {  13,  36} , {   4,  47} , {   3,  55} , {   2,  58} , {   6,  60} },
    {  CTX_UNUSED , {   8,  44} , {  11,  44} , {  14,  42} , {   7,  48} , {   4,  56} , {   4,  52} , {  13,  37} , {   9,  49} , {  19,  58} , {  10,  48} , {  12,  45} , {   0,  69} , {  20,  33} , {   8,  63} },
    {  {   9,  -2} , {  30, -10} , {  31,  -4} , {  33,  -1} , {  33,   7} , {  31,  12} , {  37,  23} , {  31,  38} , {  20,  64} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  35, -18} , {  33, -25} , {  28,  -3} , {  24,  10} , {  27,   0} , {  34, -14} , {  52, -44} , {  39, -24} , {  19,  17} , {  31,  25} , {  36,  29} , {  24,  33} , {  34,  15} , {  30,  20} , {  22,  73} },
    { {  20,  34} , {  19,  31} , {  27,  44} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  19,  16} , {  15,  36} , {  15,  36} , {  21,  28} , {  25,  21} , {  30,  20} , {  31,  12} , {  27,  16} , {  24,  42} , {   0,  93} , {  14,  56} , {  15,  57} , {  26,  38} , { -24, 127} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {   4,  39} , {   0,  42} , {   7,  34} , {  11,  29} , {   8,  31} , {   6,  37} , {   7,  42} , {   3,  40} , {   8,  33} , {  13,  43} , {  13,  36} , {   4,  47} , {   3,  55} , {   2,  58} , {   6,  60} },
    {  CTX_UNUSED , {   8,  44} , {  11,  44} , {  14,  42} , {   7,  48} , {   4,  56} , {   4,  52} , {  13,  37} , {   9,  49} , {  19,  58} , {  10,  48} , {  12,  45} , {   0,  69} , {  20,  33} , {   8,  63} },
    {  {   9,  -2} , {  30, -10} , {  31,  -4} , {  33,  -1} , {  33,   7} , {  31,  12} , {  37,  23} , {  31,  38} , {  20,  64} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  35, -18} , {  33, -25} , {  28,  -3} , {  24,  10} , {  27,   0} , {  34, -14} , {  52, -44} , {  39, -24} , {  19,  17} , {  31,  25} , {  36,  29} , {  24,  33} , {  34,  15} , {  30,  20} , {  22,  73} },
    //Cr in the 4:4:4 common mode
    { {   4,  39} , {   0,  42} , {   7,  34} , {  11,  29} , {   8,  31} , {   6,  37} , {   7,  42} , {   3,  40} , {   8,  33} , {  13,  43} , {  13,  36} , {   4,  47} , {   3,  55} , {   2,  58} , {   6,  60} },
    {  CTX_UNUSED , {   8,  44} , {  11,  44} , {  14,  42} , {   7,  48} , {   4,  56} , {   4,  52} , {  13,  37} , {   9,  49} , {  19,  58} , {  10,  48} , {  12,  45} , {   0,  69} , {  20,  33} , {   8,  63} },
    {  {   9,  -2} , {  30, -10} , {  31,  -4} , {  33,  -1} , {  33,   7} , {  31,  12} , {  37,  23} , {  31,  38} , {  20,  64} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  35, -18} , {  33, -25} , {  28,  -3} , {  24,  10} , {  27,   0} , {  34, -14} , {  52, -44} , {  39, -24} , {  19,  17} , {  31,  25} , {  36,  29} , {  24,  33} , {  34,  15} , {  30,  20} , {  22,  73} }
  }
};
//}}}
//{{{
static const char INIT_ONE_I[1][22][5][2] =
{
  //----- model 0 -----
  {
    { {  -3,  71} , {  -6,  42} , {  -5,  50} , {  -3,  54} , {  -2,  62} },
    { {  -5,  67} , {  -5,  27} , {  -3,  39} , {  -2,  44} , {   0,  46} },
    {  {  -3,  75} , {  -1,  23} , {   1,  34} , {   1,  43} , {   0,  54} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -12,  92} , { -15,  55} , { -10,  60} , {  -6,  62} , {  -4,  65} },
    { { -11,  97} , { -20,  84} , { -11,  79} , {  -6,  73} , {  -4,  74} },
    { {  -8,  78} , {  -5,  33} , {  -4,  48} , {  -2,  53} , {  -3,  62} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  -3,  71} , {  -6,  42} , {  -5,  50} , {  -3,  54} , {  -2,  62} },
    { {  -5,  67} , {  -5,  27} , {  -3,  39} , {  -2,  44} , {   0,  46} },
    { {  -3,  75} , {  -1,  23} , {   1,  34} , {   1,  43} , {   0,  54} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -12,  92} , { -15,  55} , { -10,  60} , {  -6,  62} , {  -4,  65} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cr in the 4:4:4 common mode
    { {  -3,  71} , {  -6,  42} , {  -5,  50} , {  -3,  54} , {  -2,  62} },
    { {  -5,  67} , {  -5,  27} , {  -3,  39} , {  -2,  44} , {   0,  46} },
    { {  -3,  75} , {  -1,  23} , {   1,  34} , {   1,  43} , {   0,  54} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -12,  92} , { -15,  55} , { -10,  60} , {  -6,  62} , {  -4,  65} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_ONE_P[3][22][5][2] =
{
  //----- model 0 -----
  {
    { {  -6,  76} , {  -2,  44} , {   0,  45} , {   0,  52} , {  -3,  64} },
    { {  -9,  77} , {   3,  24} , {   0,  42} , {   0,  48} , {   0,  55} },
    {  {  -6,  66} , {  -7,  35} , {  -7,  42} , {  -8,  45} , {  -5,  48} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   1,  58} , {  -3,  29} , {  -1,  36} , {   1,  38} , {   2,  43} },
    { {   0,  70} , {  -4,  29} , {   5,  31} , {   7,  42} , {   1,  59} },
    { {   0,  58} , {   8,   5} , {  10,  14} , {  14,  18} , {  13,  27} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  -6,  76} , {  -2,  44} , {   0,  45} , {   0,  52} , {  -3,  64} },
    { {  -9,  77} , {   3,  24} , {   0,  42} , {   0,  48} , {   0,  55} },
    {  {  -6,  66} , {  -7,  35} , {  -7,  42} , {  -8,  45} , {  -5,  48} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   1,  58} , {  -3,  29} , {  -1,  36} , {   1,  38} , {   2,  43} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cr in the 4:4:4 common mode
    { {  -6,  76} , {  -2,  44} , {   0,  45} , {   0,  52} , {  -3,  64} },
    { {  -9,  77} , {   3,  24} , {   0,  42} , {   0,  48} , {   0,  55} },
    {  {  -6,  66} , {  -7,  35} , {  -7,  42} , {  -8,  45} , {  -5,  48} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   1,  58} , {  -3,  29} , {  -1,  36} , {   1,  38} , {   2,  43} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  },
  //----- model 1 -----
  {
    { { -23, 112} , { -15,  71} , {  -7,  61} , {   0,  53} , {  -5,  66} },
    { { -21, 101} , {  -3,  39} , {  -5,  53} , {  -7,  61} , { -11,  75} },
    {  {  -5,  71} , {   0,  24} , {  -1,  36} , {  -2,  42} , {  -2,  52} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -11,  76} , { -10,  44} , { -10,  52} , { -10,  57} , {  -9,  58} },
    { {   2,  66} , {  -9,  34} , {   1,  32} , {  11,  31} , {   5,  52} },
    { {   3,  52} , {   7,   4} , {  10,   8} , {  17,   8} , {  16,  19} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { { -23, 112} , { -15,  71} , {  -7,  61} , {   0,  53} , {  -5,  66} },
    { { -21, 101} , {  -3,  39} , {  -5,  53} , {  -7,  61} , { -11,  75} },
    {  {  -5,  71} , {   0,  24} , {  -1,  36} , {  -2,  42} , {  -2,  52} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -11,  76} , { -10,  44} , { -10,  52} , { -10,  57} , {  -9,  58} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
     //Cr in the 4:4:4 common mode
    { { -23, 112} , { -15,  71} , {  -7,  61} , {   0,  53} , {  -5,  66} },
    { { -21, 101} , {  -3,  39} , {  -5,  53} , {  -7,  61} , { -11,  75} },
    {  {  -5,  71} , {   0,  24} , {  -1,  36} , {  -2,  42} , {  -2,  52} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -11,  76} , { -10,  44} , { -10,  52} , { -10,  57} , {  -9,  58} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  },
  //----- model 2 -----
  {
    { { -24, 115} , { -22,  82} , {  -9,  62} , {   0,  53} , {   0,  59} },
    { { -21, 100} , { -14,  57} , { -12,  67} , { -11,  71} , { -10,  77} },
    {  {  -9,  71} , {  -7,  37} , {  -8,  44} , { -11,  49} , { -10,  56} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -10,  82} , {  -8,  48} , {  -8,  61} , {  -8,  66} , {  -7,  70} },
    { {  -4,  79} , { -22,  69} , { -16,  75} , {  -2,  58} , {   1,  58} },
    { { -13,  81} , {  -6,  38} , { -13,  62} , {  -6,  58} , {  -2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { { -24, 115} , { -22,  82} , {  -9,  62} , {   0,  53} , {   0,  59} },
    { { -21, 100} , { -14,  57} , { -12,  67} , { -11,  71} , { -10,  77} },
    {  {  -9,  71} , {  -7,  37} , {  -8,  44} , { -11,  49} , { -10,  56} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -10,  82} , {  -8,  48} , {  -8,  61} , {  -8,  66} , {  -7,  70} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cr in the 4:4:4 common mode
    { { -24, 115} , { -22,  82} , {  -9,  62} , {   0,  53} , {   0,  59} },
    { { -21, 100} , { -14,  57} , { -12,  67} , { -11,  71} , { -10,  77} },
    {  {  -9,  71} , {  -7,  37} , {  -8,  44} , { -11,  49} , { -10,  56} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -10,  82} , {  -8,  48} , {  -8,  61} , {  -8,  66} , {  -7,  70} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_ABS_I[1][22][5][2] =
{
  //----- model 0 -----
  {
    { {   0,  58} , {   1,  63} , {  -2,  72} , {  -1,  74} , {  -9,  91} },
    { { -16,  64} , {  -8,  68} , { -10,  78} , {  -6,  77} , { -10,  86} },
    {  {  -2,  55} , {   0,  61} , {   1,  64} , {   0,  68} , {  -9,  92} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -12,  73} , {  -8,  76} , {  -7,  80} , {  -9,  88} , { -17, 110} },
    { { -13,  86} , { -13,  96} , { -11,  97} , { -19, 117} ,  CTX_UNUSED },
    { { -13,  71} , { -10,  79} , { -12,  86} , { -13,  90} , { -14,  97} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {   0,  58} , {   1,  63} , {  -2,  72} , {  -1,  74} , {  -9,  91} },
    { { -16,  64} , {  -8,  68} , { -10,  78} , {  -6,  77} , { -10,  86} },
    {  {  -2,  55} , {   0,  61} , {   1,  64} , {   0,  68} , {  -9,  92} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -12,  73} , {  -8,  76} , {  -7,  80} , {  -9,  88} , { -17, 110} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cr in the 4:4:4 common mode
    { {   0,  58} , {   1,  63} , {  -2,  72} , {  -1,  74} , {  -9,  91} },
    { { -16,  64} , {  -8,  68} , { -10,  78} , {  -6,  77} , { -10,  86} },
    {  {  -2,  55} , {   0,  61} , {   1,  64} , {   0,  68} , {  -9,  92} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -12,  73} , {  -8,  76} , {  -7,  80} , {  -9,  88} , { -17, 110} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}
//{{{
static const char INIT_ABS_P[3][22][5][2] =
{
  //----- model 0 -----
  {
    { {  -2,  59} , {  -4,  70} , {  -4,  75} , {  -8,  82} , { -17, 102} },
    { {  -6,  59} , {  -7,  71} , { -12,  83} , { -11,  87} , { -30, 119} },
    {  { -12,  56} , {  -6,  60} , {  -5,  62} , {  -8,  66} , {  -8,  76} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -6,  55} , {   0,  58} , {   0,  64} , {  -3,  74} , { -10,  90} },
    { {  -2,  58} , {  -3,  72} , {  -3,  81} , { -11,  97} ,  CTX_UNUSED },
    { {   2,  40} , {   0,  58} , {  -3,  70} , {  -6,  79} , {  -8,  85} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  -2,  59} , {  -4,  70} , {  -4,  75} , {  -8,  82} , { -17, 102} },
    { {  -6,  59} , {  -7,  71} , { -12,  83} , { -11,  87} , { -30, 119} },
    {  { -12,  56} , {  -6,  60} , {  -5,  62} , {  -8,  66} , {  -8,  76} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -6,  55} , {   0,  58} , {   0,  64} , {  -3,  74} , { -10,  90} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cr in the 4:4:4 common mode
    { {  -2,  59} , {  -4,  70} , {  -4,  75} , {  -8,  82} , { -17, 102} },
    { {  -6,  59} , {  -7,  71} , { -12,  83} , { -11,  87} , { -30, 119} },
    {  { -12,  56} , {  -6,  60} , {  -5,  62} , {  -8,  66} , {  -8,  76} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -6,  55} , {   0,  58} , {   0,  64} , {  -3,  74} , { -10,  90} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
  },
  //----- model 1 -----
  {
    { { -11,  77} , {  -9,  80} , {  -9,  84} , { -10,  87} , { -34, 127} },
    { { -15,  77} , { -17,  91} , { -25, 107} , { -25, 111} , { -28, 122} },
    {  {  -9,  57} , {  -6,  63} , {  -4,  65} , {  -4,  67} , {  -7,  82} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -16,  72} , {  -7,  69} , {  -4,  69} , {  -5,  74} , {  -9,  86} },
    { {  -2,  55} , {  -2,  67} , {   0,  73} , {  -8,  89} ,  CTX_UNUSED },
    { {   3,  37} , {  -1,  61} , {  -5,  73} , {  -1,  70} , {  -4,  78} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { { -11,  77} , {  -9,  80} , {  -9,  84} , { -10,  87} , { -34, 127} },
    { { -15,  77} , { -17,  91} , { -25, 107} , { -25, 111} , { -28, 122} },
    {  {  -9,  57} , {  -6,  63} , {  -4,  65} , {  -4,  67} , {  -7,  82} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -16,  72} , {  -7,  69} , {  -4,  69} , {  -5,  74} , {  -9,  86} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cr in the 4:4:4 common mode
    { { -11,  77} , {  -9,  80} , {  -9,  84} , { -10,  87} , { -34, 127} },
    { { -15,  77} , { -17,  91} , { -25, 107} , { -25, 111} , { -28, 122} },
    {  {  -9,  57} , {  -6,  63} , {  -4,  65} , {  -4,  67} , {  -7,  82} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -16,  72} , {  -7,  69} , {  -4,  69} , {  -5,  74} , {  -9,  86} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
  },
  //----- model 2 -----
  {
    { { -14,  85} , { -13,  89} , { -13,  94} , { -11,  92} , { -29, 127} },
    { { -21,  85} , { -16,  88} , { -23, 104} , { -15,  98} , { -37, 127} },
    {  { -12,  59} , {  -8,  63} , {  -9,  67} , {  -6,  68} , { -10,  79} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -14,  75} , { -10,  79} , {  -9,  83} , { -12,  92} , { -18, 108} },
    { { -13,  78} , {  -9,  83} , {  -4,  81} , { -13,  99} ,  CTX_UNUSED },
    { { -16,  73} , { -10,  76} , { -13,  86} , {  -9,  83} , { -10,  87} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { { -14,  85} , { -13,  89} , { -13,  94} , { -11,  92} , { -29, 127} },
    { { -21,  85} , { -16,  88} , { -23, 104} , { -15,  98} , { -37, 127} },
    {  { -12,  59} , {  -8,  63} , {  -9,  67} , {  -6,  68} , { -10,  79} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -14,  75} , { -10,  79} , {  -9,  83} , { -12,  92} , { -18, 108} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cr in the 4:4:4 common mode
    { { -14,  85} , { -13,  89} , { -13,  94} , { -11,  92} , { -29, 127} },
    { { -21,  85} , { -16,  88} , { -23, 104} , { -15,  98} , { -37, 127} },
    {  { -12,  59} , {  -8,  63} , {  -9,  67} , {  -6,  68} , { -10,  79} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -14,  75} , { -10,  79} , {  -9,  83} , { -12,  92} , { -18, 108} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
  }
};
//}}}

//{{{
static const char INIT_FLD_MAP_I[1][22][15][2] =
{
  //----- model 0 -----
  {
    { {  -6,  93} , {  -6,  84} , {  -8,  79} , {   0,  66} , {  -1,  71} , {   0,  62} , {  -2,  60} , {  -2,  59} , {  -5,  75} , {  -3,  62} , {  -4,  58} , {  -9,  66} , {  -1,  79} , {   0,  71} , {   3,  68} },
    {  CTX_UNUSED , {  10,  44} , {  -7,  62} , {  15,  36} , {  14,  40} , {  16,  27} , {  12,  29} , {   1,  44} , {  20,  36} , {  18,  32} , {   5,  42} , {   1,  48} , {  10,  62} , {  17,  46} , {   9,  64} },
    {  { -14, 106} , { -13,  97} , { -15,  90} , { -12,  90} , { -18,  88} , { -10,  73} , {  -9,  79} , { -14,  86} , { -10,  73} , { -10,  70} , { -10,  69} , {  -5,  66} , {  -9,  64} , {  -5,  58} , {   2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -12, 104} , { -11,  97} , { -16,  96} , {  -7,  88} , {  -8,  85} , {  -7,  85} , {  -9,  85} , { -13,  88} , {   4,  66} , {  -3,  77} , {  -3,  76} , {  -6,  76} , {  10,  58} , {  -1,  76} , {  -1,  83} },
    { {  -7,  99} , { -14,  95} , {   2,  95} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {   0,  76} , {  -5,  74} , {   0,  70} , { -11,  75} , {   1,  68} , {   0,  65} , { -14,  73} , {   3,  62} , {   4,  62} , {  -1,  68} , { -13,  75} , {  11,  55} , {   5,  64} , {  12,  70} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  -6,  93} , {  -6,  84} , {  -8,  79} , {   0,  66} , {  -1,  71} , {   0,  62} , {  -2,  60} , {  -2,  59} , {  -5,  75} , {  -3,  62} , {  -4,  58} , {  -9,  66} , {  -1,  79} , {   0,  71} , {   3,  68} },
    {  CTX_UNUSED , {  10,  44} , {  -7,  62} , {  15,  36} , {  14,  40} , {  16,  27} , {  12,  29} , {   1,  44} , {  20,  36} , {  18,  32} , {   5,  42} , {   1,  48} , {  10,  62} , {  17,  46} , {   9,  64} },
    {  { -14, 106} , { -13,  97} , { -15,  90} , { -12,  90} , { -18,  88} , { -10,  73} , {  -9,  79} , { -14,  86} , { -10,  73} , { -10,  70} , { -10,  69} , {  -5,  66} , {  -9,  64} , {  -5,  58} , {   2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -12, 104} , { -11,  97} , { -16,  96} , {  -7,  88} , {  -8,  85} , {  -7,  85} , {  -9,  85} , { -13,  88} , {   4,  66} , {  -3,  77} , {  -3,  76} , {  -6,  76} , {  10,  58} , {  -1,  76} , {  -1,  83} },
    //Cr in the 4:4:4 common mode
    { {  -6,  93} , {  -6,  84} , {  -8,  79} , {   0,  66} , {  -1,  71} , {   0,  62} , {  -2,  60} , {  -2,  59} , {  -5,  75} , {  -3,  62} , {  -4,  58} , {  -9,  66} , {  -1,  79} , {   0,  71} , {   3,  68} },
    {  CTX_UNUSED , {  10,  44} , {  -7,  62} , {  15,  36} , {  14,  40} , {  16,  27} , {  12,  29} , {   1,  44} , {  20,  36} , {  18,  32} , {   5,  42} , {   1,  48} , {  10,  62} , {  17,  46} , {   9,  64} },
    {  { -14, 106} , { -13,  97} , { -15,  90} , { -12,  90} , { -18,  88} , { -10,  73} , {  -9,  79} , { -14,  86} , { -10,  73} , { -10,  70} , { -10,  69} , {  -5,  66} , {  -9,  64} , {  -5,  58} , {   2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { { -12, 104} , { -11,  97} , { -16,  96} , {  -7,  88} , {  -8,  85} , {  -7,  85} , {  -9,  85} , { -13,  88} , {   4,  66} , {  -3,  77} , {  -3,  76} , {  -6,  76} , {  10,  58} , {  -1,  76} , {  -1,  83} },
  }
};
//}}}
//{{{
static const char INIT_FLD_MAP_P[3][22][15][2] =
{
  //----- model 0 -----
  {
    { { -13, 106} , { -16, 106} , { -10,  87} , { -21, 114} , { -18, 110} , { -14,  98} , { -22, 110} , { -21, 106} , { -18, 103} , { -21, 107} , { -23, 108} , { -26, 112} , { -10,  96} , { -12,  95} , {  -5,  91} },
    {  CTX_UNUSED , {  -9,  93} , { -22,  94} , {  -5,  86} , {   9,  67} , {  -4,  80} , { -10,  85} , {  -1,  70} , {   7,  60} , {   9,  58} , {   5,  61} , {  12,  50} , {  15,  50} , {  18,  49} , {  17,  54} },
    {  {  -5,  85} , {  -6,  81} , { -10,  77} , {  -7,  81} , { -17,  80} , { -18,  73} , {  -4,  74} , { -10,  83} , {  -9,  71} , {  -9,  67} , {  -1,  61} , {  -8,  66} , { -14,  66} , {   0,  59} , {   2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  10,  41} , {   7,  46} , {  -1,  51} , {   7,  49} , {   8,  52} , {   9,  41} , {   6,  47} , {   2,  55} , {  13,  41} , {  10,  44} , {   6,  50} , {   5,  53} , {  13,  49} , {   4,  63} , {   6,  64} },
    { {  -2,  69} , {  -2,  59} , {   6,  70} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  10,  44} , {   9,  31} , {  12,  43} , {   3,  53} , {  14,  34} , {  10,  38} , {  -3,  52} , {  13,  40} , {  17,  32} , {   7,  44} , {   7,  38} , {  13,  50} , {  10,  57} , {  26,  43} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { { -13, 106} , { -16, 106} , { -10,  87} , { -21, 114} , { -18, 110} , { -14,  98} , { -22, 110} , { -21, 106} , { -18, 103} , { -21, 107} , { -23, 108} , { -26, 112} , { -10,  96} , { -12,  95} , {  -5,  91} },
    {  CTX_UNUSED , {  -9,  93} , { -22,  94} , {  -5,  86} , {   9,  67} , {  -4,  80} , { -10,  85} , {  -1,  70} , {   7,  60} , {   9,  58} , {   5,  61} , {  12,  50} , {  15,  50} , {  18,  49} , {  17,  54} },
    {  {  -5,  85} , {  -6,  81} , { -10,  77} , {  -7,  81} , { -17,  80} , { -18,  73} , {  -4,  74} , { -10,  83} , {  -9,  71} , {  -9,  67} , {  -1,  61} , {  -8,  66} , { -14,  66} , {   0,  59} , {   2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  10,  41} , {   7,  46} , {  -1,  51} , {   7,  49} , {   8,  52} , {   9,  41} , {   6,  47} , {   2,  55} , {  13,  41} , {  10,  44} , {   6,  50} , {   5,  53} , {  13,  49} , {   4,  63} , {   6,  64} },
    //Cr in the 4:4:4 common mode
    { { -13, 106} , { -16, 106} , { -10,  87} , { -21, 114} , { -18, 110} , { -14,  98} , { -22, 110} , { -21, 106} , { -18, 103} , { -21, 107} , { -23, 108} , { -26, 112} , { -10,  96} , { -12,  95} , {  -5,  91} },
    {  CTX_UNUSED , {  -9,  93} , { -22,  94} , {  -5,  86} , {   9,  67} , {  -4,  80} , { -10,  85} , {  -1,  70} , {   7,  60} , {   9,  58} , {   5,  61} , {  12,  50} , {  15,  50} , {  18,  49} , {  17,  54} },
    {  {  -5,  85} , {  -6,  81} , { -10,  77} , {  -7,  81} , { -17,  80} , { -18,  73} , {  -4,  74} , { -10,  83} , {  -9,  71} , {  -9,  67} , {  -1,  61} , {  -8,  66} , { -14,  66} , {   0,  59} , {   2,  59} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  10,  41} , {   7,  46} , {  -1,  51} , {   7,  49} , {   8,  52} , {   9,  41} , {   6,  47} , {   2,  55} , {  13,  41} , {  10,  44} , {   6,  50} , {   5,  53} , {  13,  49} , {   4,  63} , {   6,  64} },
  },
  //----- model 1 -----
  {
    { { -21, 126} , { -23, 124} , { -20, 110} , { -26, 126} , { -25, 124} , { -17, 105} , { -27, 121} , { -27, 117} , { -17, 102} , { -26, 117} , { -27, 116} , { -33, 122} , { -10,  95} , { -14, 100} , {  -8,  95} },
    {  CTX_UNUSED , { -17, 111} , { -28, 114} , {  -6,  89} , {  -2,  80} , {  -4,  82} , {  -9,  85} , {  -8,  81} , {  -1,  72} , {   5,  64} , {   1,  67} , {   9,  56} , {   0,  69} , {   1,  69} , {   7,  69} },
    {  {  -3,  81} , {  -3,  76} , {  -7,  72} , {  -6,  78} , { -12,  72} , { -14,  68} , {  -3,  70} , {  -6,  76} , {  -5,  66} , {  -5,  62} , {   0,  57} , {  -4,  61} , {  -9,  60} , {   1,  54} , {   2,  58} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -7,  69} , {  -6,  67} , { -16,  77} , {  -2,  64} , {   2,  61} , {  -6,  67} , {  -3,  64} , {   2,  57} , {  -3,  65} , {  -3,  66} , {   0,  62} , {   9,  51} , {  -1,  66} , {  -2,  71} , {  -2,  75} },
    { {  -1,  70} , {  -9,  72} , {  14,  60} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  16,  37} , {   0,  47} , {  18,  35} , {  11,  37} , {  12,  41} , {  10,  41} , {   2,  48} , {  12,  41} , {  13,  41} , {   0,  59} , {   3,  50} , {  19,  40} , {   3,  66} , {  18,  50} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { { -21, 126} , { -23, 124} , { -20, 110} , { -26, 126} , { -25, 124} , { -17, 105} , { -27, 121} , { -27, 117} , { -17, 102} , { -26, 117} , { -27, 116} , { -33, 122} , { -10,  95} , { -14, 100} , {  -8,  95} },
    {  CTX_UNUSED , { -17, 111} , { -28, 114} , {  -6,  89} , {  -2,  80} , {  -4,  82} , {  -9,  85} , {  -8,  81} , {  -1,  72} , {   5,  64} , {   1,  67} , {   9,  56} , {   0,  69} , {   1,  69} , {   7,  69} },
    {  {  -3,  81} , {  -3,  76} , {  -7,  72} , {  -6,  78} , { -12,  72} , { -14,  68} , {  -3,  70} , {  -6,  76} , {  -5,  66} , {  -5,  62} , {   0,  57} , {  -4,  61} , {  -9,  60} , {   1,  54} , {   2,  58} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -7,  69} , {  -6,  67} , { -16,  77} , {  -2,  64} , {   2,  61} , {  -6,  67} , {  -3,  64} , {   2,  57} , {  -3,  65} , {  -3,  66} , {   0,  62} , {   9,  51} , {  -1,  66} , {  -2,  71} , {  -2,  75} },
    //Cr in the 4:4:4 common mode
    { { -21, 126} , { -23, 124} , { -20, 110} , { -26, 126} , { -25, 124} , { -17, 105} , { -27, 121} , { -27, 117} , { -17, 102} , { -26, 117} , { -27, 116} , { -33, 122} , { -10,  95} , { -14, 100} , {  -8,  95} },
    {  CTX_UNUSED , { -17, 111} , { -28, 114} , {  -6,  89} , {  -2,  80} , {  -4,  82} , {  -9,  85} , {  -8,  81} , {  -1,  72} , {   5,  64} , {   1,  67} , {   9,  56} , {   0,  69} , {   1,  69} , {   7,  69} },
    {  {  -3,  81} , {  -3,  76} , {  -7,  72} , {  -6,  78} , { -12,  72} , { -14,  68} , {  -3,  70} , {  -6,  76} , {  -5,  66} , {  -5,  62} , {   0,  57} , {  -4,  61} , {  -9,  60} , {   1,  54} , {   2,  58} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  -7,  69} , {  -6,  67} , { -16,  77} , {  -2,  64} , {   2,  61} , {  -6,  67} , {  -3,  64} , {   2,  57} , {  -3,  65} , {  -3,  66} , {   0,  62} , {   9,  51} , {  -1,  66} , {  -2,  71} , {  -2,  75} },
  },
  //----- model 2 -----
  {
    { { -22, 127} , { -25, 127} , { -25, 120} , { -27, 127} , { -19, 114} , { -23, 117} , { -25, 118} , { -26, 117} , { -24, 113} , { -28, 118} , { -31, 120} , { -37, 124} , { -10,  94} , { -15, 102} , { -10,  99} },
    {  CTX_UNUSED , { -13, 106} , { -50, 127} , {  -5,  92} , {  17,  57} , {  -5,  86} , { -13,  94} , { -12,  91} , {  -2,  77} , {   0,  71} , {  -1,  73} , {   4,  64} , {  -7,  81} , {   5,  64} , {  15,  57} },
    {  {  -3,  78} , {  -8,  74} , {  -9,  72} , { -10,  72} , { -18,  75} , { -12,  71} , { -11,  63} , {  -5,  70} , { -17,  75} , { -14,  72} , { -16,  67} , {  -8,  53} , { -14,  59} , {  -9,  52} , { -11,  68} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   1,  67} , {   0,  68} , { -10,  67} , {   1,  68} , {   0,  77} , {   2,  64} , {   0,  68} , {  -5,  78} , {   7,  55} , {   5,  59} , {   2,  65} , {  14,  54} , {  15,  44} , {   5,  60} , {   2,  70} },
    { {  -2,  76} , { -18,  86} , {  12,  70} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {   5,  64} , { -12,  70} , {  11,  55} , {   5,  56} , {   0,  69} , {   2,  65} , {  -6,  74} , {   5,  54} , {   7,  54} , {  -6,  76} , { -11,  82} , {  -2,  77} , {  -2,  77} , {  25,  42} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { { -22, 127} , { -25, 127} , { -25, 120} , { -27, 127} , { -19, 114} , { -23, 117} , { -25, 118} , { -26, 117} , { -24, 113} , { -28, 118} , { -31, 120} , { -37, 124} , { -10,  94} , { -15, 102} , { -10,  99} },
    {  CTX_UNUSED , { -13, 106} , { -50, 127} , {  -5,  92} , {  17,  57} , {  -5,  86} , { -13,  94} , { -12,  91} , {  -2,  77} , {   0,  71} , {  -1,  73} , {   4,  64} , {  -7,  81} , {   5,  64} , {  15,  57} },
    {  {  -3,  78} , {  -8,  74} , {  -9,  72} , { -10,  72} , { -18,  75} , { -12,  71} , { -11,  63} , {  -5,  70} , { -17,  75} , { -14,  72} , { -16,  67} , {  -8,  53} , { -14,  59} , {  -9,  52} , { -11,  68} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   1,  67} , {   0,  68} , { -10,  67} , {   1,  68} , {   0,  77} , {   2,  64} , {   0,  68} , {  -5,  78} , {   7,  55} , {   5,  59} , {   2,  65} , {  14,  54} , {  15,  44} , {   5,  60} , {   2,  70} },
    //Cr in the 4:4:4 common mode
    { { -22, 127} , { -25, 127} , { -25, 120} , { -27, 127} , { -19, 114} , { -23, 117} , { -25, 118} , { -26, 117} , { -24, 113} , { -28, 118} , { -31, 120} , { -37, 124} , { -10,  94} , { -15, 102} , { -10,  99} },
    {  CTX_UNUSED , { -13, 106} , { -50, 127} , {  -5,  92} , {  17,  57} , {  -5,  86} , { -13,  94} , { -12,  91} , {  -2,  77} , {   0,  71} , {  -1,  73} , {   4,  64} , {  -7,  81} , {   5,  64} , {  15,  57} },
    {  {  -3,  78} , {  -8,  74} , {  -9,  72} , { -10,  72} , { -18,  75} , { -12,  71} , { -11,  63} , {  -5,  70} , { -17,  75} , { -14,  72} , { -16,  67} , {  -8,  53} , { -14,  59} , {  -9,  52} , { -11,  68} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {   1,  67} , {   0,  68} , { -10,  67} , {   1,  68} , {   0,  77} , {   2,  64} , {   0,  68} , {  -5,  78} , {   7,  55} , {   5,  59} , {   2,  65} , {  14,  54} , {  15,  44} , {   5,  60} , {   2,  70} },
  }
};
//}}}
//{{{
static const char INIT_FLD_LAST_I[1][22][15][2] =
{
  //----- model 0 -----
  {
    { {  15,   6} , {   6,  19} , {   7,  16} , {  12,  14} , {  18,  13} , {  13,  11} , {  13,  15} , {  15,  16} , {  12,  23} , {  13,  23} , {  15,  20} , {  14,  26} , {  14,  44} , {  17,  40} , {  17,  47} },
    {  CTX_UNUSED , {  24,  17} , {  21,  21} , {  25,  22} , {  31,  27} , {  22,  29} , {  19,  35} , {  14,  50} , {  10,  57} , {   7,  63} , {  -2,  77} , {  -4,  82} , {  -3,  94} , {   9,  69} , { -12, 109} },
    {  {  21, -10} , {  24, -11} , {  28,  -8} , {  28,  -1} , {  29,   3} , {  29,   9} , {  35,  20} , {  29,  36} , {  14,  67} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  36, -35} , {  36, -34} , {  32, -26} , {  37, -30} , {  44, -32} , {  34, -18} , {  34, -15} , {  40, -15} , {  33,  -7} , {  35,  -5} , {  33,   0} , {  38,   2} , {  33,  13} , {  23,  35} , {  13,  58} },
    { {  29,  -3} , {  26,   0} , {  22,  30} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  31,  -7} , {  35, -15} , {  34,  -3} , {  34,   3} , {  36,  -1} , {  34,   5} , {  32,  11} , {  35,   5} , {  34,  12} , {  39,  11} , {  30,  29} , {  34,  26} , {  29,  39} , {  19,  66} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  15,   6} , {   6,  19} , {   7,  16} , {  12,  14} , {  18,  13} , {  13,  11} , {  13,  15} , {  15,  16} , {  12,  23} , {  13,  23} , {  15,  20} , {  14,  26} , {  14,  44} , {  17,  40} , {  17,  47} },
    {  CTX_UNUSED , {  24,  17} , {  21,  21} , {  25,  22} , {  31,  27} , {  22,  29} , {  19,  35} , {  14,  50} , {  10,  57} , {   7,  63} , {  -2,  77} , {  -4,  82} , {  -3,  94} , {   9,  69} , { -12, 109} },
    {  {  21, -10} , {  24, -11} , {  28,  -8} , {  28,  -1} , {  29,   3} , {  29,   9} , {  35,  20} , {  29,  36} , {  14,  67} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  36, -35} , {  36, -34} , {  32, -26} , {  37, -30} , {  44, -32} , {  34, -18} , {  34, -15} , {  40, -15} , {  33,  -7} , {  35,  -5} , {  33,   0} , {  38,   2} , {  33,  13} , {  23,  35} , {  13,  58} },
    //Cr in the 4:4:4 common mode
    { {  15,   6} , {   6,  19} , {   7,  16} , {  12,  14} , {  18,  13} , {  13,  11} , {  13,  15} , {  15,  16} , {  12,  23} , {  13,  23} , {  15,  20} , {  14,  26} , {  14,  44} , {  17,  40} , {  17,  47} },
    {  CTX_UNUSED , {  24,  17} , {  21,  21} , {  25,  22} , {  31,  27} , {  22,  29} , {  19,  35} , {  14,  50} , {  10,  57} , {   7,  63} , {  -2,  77} , {  -4,  82} , {  -3,  94} , {   9,  69} , { -12, 109} },
    {  {  21, -10} , {  24, -11} , {  28,  -8} , {  28,  -1} , {  29,   3} , {  29,   9} , {  35,  20} , {  29,  36} , {  14,  67} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  36, -35} , {  36, -34} , {  32, -26} , {  37, -30} , {  44, -32} , {  34, -18} , {  34, -15} , {  40, -15} , {  33,  -7} , {  35,  -5} , {  33,   0} , {  38,   2} , {  33,  13} , {  23,  35} , {  13,  58} },
  }
};
//}}}
//{{{
static const char INIT_FLD_LAST_P[3][22][15][2] =
{
  //----- model 0 -----
  {
    { {  14,  11} , {  11,  14} , {   9,  11} , {  18,  11} , {  21,   9} , {  23,  -2} , {  32, -15} , {  32, -15} , {  34, -21} , {  39, -23} , {  42, -33} , {  41, -31} , {  46, -28} , {  38, -12} , {  21,  29} },
    {  CTX_UNUSED , {  45, -24} , {  53, -45} , {  48, -26} , {  65, -43} , {  43, -19} , {  39, -10} , {  30,   9} , {  18,  26} , {  20,  27} , {   0,  57} , { -14,  82} , {  -5,  75} , { -19,  97} , { -35, 125} },
    {  {  21, -13} , {  33, -14} , {  39,  -7} , {  46,  -2} , {  51,   2} , {  60,   6} , {  61,  17} , {  55,  34} , {  42,  62} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  27,   0} , {  28,   0} , {  31,  -4} , {  27,   6} , {  34,   8} , {  30,  10} , {  24,  22} , {  33,  19} , {  22,  32} , {  26,  31} , {  21,  41} , {  26,  44} , {  23,  47} , {  16,  65} , {  14,  71} },
    { {   8,  60} , {   6,  63} , {  17,  65} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  21,  24} , {  23,  20} , {  26,  23} , {  27,  32} , {  28,  23} , {  28,  24} , {  23,  40} , {  24,  32} , {  28,  29} , {  23,  42} , {  19,  57} , {  22,  53} , {  22,  61} , {  11,  86} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  14,  11} , {  11,  14} , {   9,  11} , {  18,  11} , {  21,   9} , {  23,  -2} , {  32, -15} , {  32, -15} , {  34, -21} , {  39, -23} , {  42, -33} , {  41, -31} , {  46, -28} , {  38, -12} , {  21,  29} },
    {  CTX_UNUSED , {  45, -24} , {  53, -45} , {  48, -26} , {  65, -43} , {  43, -19} , {  39, -10} , {  30,   9} , {  18,  26} , {  20,  27} , {   0,  57} , { -14,  82} , {  -5,  75} , { -19,  97} , { -35, 125} },
    {  {  21, -13} , {  33, -14} , {  39,  -7} , {  46,  -2} , {  51,   2} , {  60,   6} , {  61,  17} , {  55,  34} , {  42,  62} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  27,   0} , {  28,   0} , {  31,  -4} , {  27,   6} , {  34,   8} , {  30,  10} , {  24,  22} , {  33,  19} , {  22,  32} , {  26,  31} , {  21,  41} , {  26,  44} , {  23,  47} , {  16,  65} , {  14,  71} },
    //Cr in the 4:4:4 common mode
    { {  14,  11} , {  11,  14} , {   9,  11} , {  18,  11} , {  21,   9} , {  23,  -2} , {  32, -15} , {  32, -15} , {  34, -21} , {  39, -23} , {  42, -33} , {  41, -31} , {  46, -28} , {  38, -12} , {  21,  29} },
    {  CTX_UNUSED , {  45, -24} , {  53, -45} , {  48, -26} , {  65, -43} , {  43, -19} , {  39, -10} , {  30,   9} , {  18,  26} , {  20,  27} , {   0,  57} , { -14,  82} , {  -5,  75} , { -19,  97} , { -35, 125} },
    {  {  21, -13} , {  33, -14} , {  39,  -7} , {  46,  -2} , {  51,   2} , {  60,   6} , {  61,  17} , {  55,  34} , {  42,  62} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  27,   0} , {  28,   0} , {  31,  -4} , {  27,   6} , {  34,   8} , {  30,  10} , {  24,  22} , {  33,  19} , {  22,  32} , {  26,  31} , {  21,  41} , {  26,  44} , {  23,  47} , {  16,  65} , {  14,  71} },
  },
  //----- model 1 -----
  {
    { {  19,  -6} , {  18,  -6} , {  14,   0} , {  26, -12} , {  31, -16} , {  33, -25} , {  33, -22} , {  37, -28} , {  39, -30} , {  42, -30} , {  47, -42} , {  45, -36} , {  49, -34} , {  41, -17} , {  32,   9} },
    {  CTX_UNUSED , {  69, -71} , {  63, -63} , {  66, -64} , {  77, -74} , {  54, -39} , {  52, -35} , {  41, -10} , {  36,   0} , {  40,  -1} , {  30,  14} , {  28,  26} , {  23,  37} , {  12,  55} , {  11,  65} },
    {  {  17, -10} , {  32, -13} , {  42,  -9} , {  49,  -5} , {  53,   0} , {  64,   3} , {  68,  10} , {  66,  27} , {  47,  57} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  37, -33} , {  39, -36} , {  40, -37} , {  38, -30} , {  46, -33} , {  42, -30} , {  40, -24} , {  49, -29} , {  38, -12} , {  40, -10} , {  38,  -3} , {  46,  -5} , {  31,  20} , {  29,  30} , {  25,  44} },
    { {  12,  48} , {  11,  49} , {  26,  45} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  22,  22} , {  23,  22} , {  27,  21} , {  33,  20} , {  26,  28} , {  30,  24} , {  27,  34} , {  18,  42} , {  25,  39} , {  18,  50} , {  12,  70} , {  21,  54} , {  14,  71} , {  11,  83} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  19,  -6} , {  18,  -6} , {  14,   0} , {  26, -12} , {  31, -16} , {  33, -25} , {  33, -22} , {  37, -28} , {  39, -30} , {  42, -30} , {  47, -42} , {  45, -36} , {  49, -34} , {  41, -17} , {  32,   9} },
    {  CTX_UNUSED , {  69, -71} , {  63, -63} , {  66, -64} , {  77, -74} , {  54, -39} , {  52, -35} , {  41, -10} , {  36,   0} , {  40,  -1} , {  30,  14} , {  28,  26} , {  23,  37} , {  12,  55} , {  11,  65} },
    {  {  17, -10} , {  32, -13} , {  42,  -9} , {  49,  -5} , {  53,   0} , {  64,   3} , {  68,  10} , {  66,  27} , {  47,  57} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  37, -33} , {  39, -36} , {  40, -37} , {  38, -30} , {  46, -33} , {  42, -30} , {  40, -24} , {  49, -29} , {  38, -12} , {  40, -10} , {  38,  -3} , {  46,  -5} , {  31,  20} , {  29,  30} , {  25,  44} },
    //Cr in the 4:4:4 common mode
    { {  19,  -6} , {  18,  -6} , {  14,   0} , {  26, -12} , {  31, -16} , {  33, -25} , {  33, -22} , {  37, -28} , {  39, -30} , {  42, -30} , {  47, -42} , {  45, -36} , {  49, -34} , {  41, -17} , {  32,   9} },
    {  CTX_UNUSED , {  69, -71} , {  63, -63} , {  66, -64} , {  77, -74} , {  54, -39} , {  52, -35} , {  41, -10} , {  36,   0} , {  40,  -1} , {  30,  14} , {  28,  26} , {  23,  37} , {  12,  55} , {  11,  65} },
    {  {  17, -10} , {  32, -13} , {  42,  -9} , {  49,  -5} , {  53,   0} , {  64,   3} , {  68,  10} , {  66,  27} , {  47,  57} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  37, -33} , {  39, -36} , {  40, -37} , {  38, -30} , {  46, -33} , {  42, -30} , {  40, -24} , {  49, -29} , {  38, -12} , {  40, -10} , {  38,  -3} , {  46,  -5} , {  31,  20} , {  29,  30} , {  25,  44} },
  },
  //----- model 2 -----
  {
    { {  17, -13} , {  16,  -9} , {  17, -12} , {  27, -21} , {  37, -30} , {  41, -40} , {  42, -41} , {  48, -47} , {  39, -32} , {  46, -40} , {  52, -51} , {  46, -41} , {  52, -39} , {  43, -19} , {  32,  11} },
    {  CTX_UNUSED , {  61, -55} , {  56, -46} , {  62, -50} , {  81, -67} , {  45, -20} , {  35,  -2} , {  28,  15} , {  34,   1} , {  39,   1} , {  30,  17} , {  20,  38} , {  18,  45} , {  15,  54} , {   0,  79} },
    {  {   9,  -2} , {  30, -10} , {  31,  -4} , {  33,  -1} , {  33,   7} , {  31,  12} , {  37,  23} , {  31,  38} , {  20,  64} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  36, -16} , {  37, -14} , {  37, -17} , {  32,   1} , {  34,  15} , {  29,  15} , {  24,  25} , {  34,  22} , {  31,  16} , {  35,  18} , {  31,  28} , {  33,  41} , {  36,  28} , {  27,  47} , {  21,  62} },
    { {  18,  31} , {  19,  26} , {  36,  24} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED , {  24,  23} , {  27,  16} , {  24,  30} , {  31,  29} , {  22,  41} , {  22,  42} , {  16,  60} , {  15,  52} , {  14,  60} , {   3,  78} , { -16, 123} , {  21,  53} , {  22,  56} , {  25,  61} },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    //Cb in the 4:4:4 common mode
    { {  17, -13} , {  16,  -9} , {  17, -12} , {  27, -21} , {  37, -30} , {  41, -40} , {  42, -41} , {  48, -47} , {  39, -32} , {  46, -40} , {  52, -51} , {  46, -41} , {  52, -39} , {  43, -19} , {  32,  11} },
    {  CTX_UNUSED , {  61, -55} , {  56, -46} , {  62, -50} , {  81, -67} , {  45, -20} , {  35,  -2} , {  28,  15} , {  34,   1} , {  39,   1} , {  30,  17} , {  20,  38} , {  18,  45} , {  15,  54} , {   0,  79} },
    {  {   9,  -2} , {  30, -10} , {  31,  -4} , {  33,  -1} , {  33,   7} , {  31,  12} , {  37,  23} , {  31,  38} , {  20,  64} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  36, -16} , {  37, -14} , {  37, -17} , {  32,   1} , {  34,  15} , {  29,  15} , {  24,  25} , {  34,  22} , {  31,  16} , {  35,  18} , {  31,  28} , {  33,  41} , {  36,  28} , {  27,  47} , {  21,  62} },
    //Cr in the 4:4:4 common mode
    { {  17, -13} , {  16,  -9} , {  17, -12} , {  27, -21} , {  37, -30} , {  41, -40} , {  42, -41} , {  48, -47} , {  39, -32} , {  46, -40} , {  52, -51} , {  46, -41} , {  52, -39} , {  43, -19} , {  32,  11} },
    {  CTX_UNUSED , {  61, -55} , {  56, -46} , {  62, -50} , {  81, -67} , {  45, -20} , {  35,  -2} , {  28,  15} , {  34,   1} , {  39,   1} , {  30,  17} , {  20,  38} , {  18,  45} , {  15,  54} , {   0,  79} },
    {  {   9,  -2} , {  30, -10} , {  31,  -4} , {  33,  -1} , {  33,   7} , {  31,  12} , {  37,  23} , {  31,  38} , {  20,  64} ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
    { {  36, -16} , {  37, -14} , {  37, -17} , {  32,   1} , {  34,  15} , {  29,  15} , {  24,  25} , {  34,  22} , {  31,  16} , {  35,  18} , {  31,  28} , {  33,  41} , {  36,  28} , {  27,  47} , {  21,  62} },
  }
};
//}}}
//}}}

//{{{
static void initContexts (sSlice* slice) {

  //{{{
  #define IBIARI_CTX_INIT2(ii,jj,ctx,tab,num, qp) { \
    for (i = 0; i < ii; ++i) \
      for (j = 0; j < jj; ++j) \
        biariInitContext (qp, &(ctx[i][j]), tab ## _I[num][i][j]); \
    }
  //}}}
  //{{{
  #define PBIARI_CTX_INIT2(ii,jj,ctx,tab,num, qp) { \
    for (i = 0; i < ii; ++i) \
      for (j = 0; j < jj; ++j) \
        biariInitContext (qp, &(ctx[i][j]), tab ## _P[num][i][j]); \
    }
  //}}}
  //{{{
  #define IBIARI_CTX_INIT1(jj,ctx,tab,num, qp) { \
    for (j = 0; j < jj; ++j) \
      biariInitContext (qp, &(ctx[j]), tab ## _I[num][0][j]); \
    }
  //}}}
  //{{{
  #define PBIARI_CTX_INIT1(jj,ctx,tab,num, qp) { \
    for (j = 0; j < jj; ++j) \
      biariInitContext (qp, &(ctx[j]), tab ## _P[num][0][j]); \
    }
  //}}}

  sMotionInfoContexts* mc = slice->motionInfoContexts;
  sTextureInfoContexts* tc = slice->textureInfoContexts;

  int i, j;
  int qp = imax (0, slice->qp);
  int modelNum = slice->modelNum;

  // motion coding contexts
  if ((slice->sliceType == eIslice) || (slice->sliceType == eSIslice)) {
    IBIARI_CTX_INIT2 (3, NUM_MB_TYPE_CTX,   mc->mbTypeContexts,     INIT_MB_TYPE,    modelNum, qp);
    IBIARI_CTX_INIT2 (2, NUM_B8_TYPE_CTX,   mc->b8_type_contexts,     INIT_B8_TYPE,    modelNum, qp);
    IBIARI_CTX_INIT2 (2, NUM_MV_RES_CTX,    mc->mvResContexts,      INIT_MV_RES,     modelNum, qp);
    IBIARI_CTX_INIT2 (2, NUM_REF_NO_CTX,    mc->ref_no_contexts,      INIT_REF_NO,     modelNum, qp);
    IBIARI_CTX_INIT1 (   NUM_DELTA_QP_CTX,  mc->delta_qp_contexts,    INIT_DELTA_QP,   modelNum, qp);
    IBIARI_CTX_INIT1 (   NUM_MB_AFF_CTX,    mc->mb_aff_contexts,      INIT_MB_AFF,     modelNum, qp);

    // texture coding contexts
    IBIARI_CTX_INIT1 (   NUM_TRANSFORM_SIZE_CTX, tc->transform_size_contexts, INIT_TRANSFORM_SIZE, modelNum, qp);
    IBIARI_CTX_INIT1 (                 NUM_IPR_CTX,  tc->ipr_contexts,     INIT_IPR,       modelNum, qp);
    IBIARI_CTX_INIT1 (                 NUM_CIPR_CTX, tc->cipr_contexts,    INIT_CIPR,      modelNum, qp);
    IBIARI_CTX_INIT2 (3,               NUM_CBP_CTX,  tc->cbp_contexts,     INIT_CBP,       modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_BCBP_CTX, tc->bcbp_contexts,    INIT_BCBP,      modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts[0],  INIT_MAP,       modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts[1],  INIT_FLD_MAP,   modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts[1], INIT_FLD_LAST,  modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts[0], INIT_LAST,      modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ONE_CTX,  tc->one_contexts,     INIT_ONE,       modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ABS_CTX,  tc->abs_contexts,     INIT_ABS,       modelNum, qp);
    }
  else {
    PBIARI_CTX_INIT2 (3, NUM_MB_TYPE_CTX,   mc->mbTypeContexts,     INIT_MB_TYPE,    modelNum, qp);
    PBIARI_CTX_INIT2 (2, NUM_B8_TYPE_CTX,   mc->b8_type_contexts,     INIT_B8_TYPE,    modelNum, qp);
    PBIARI_CTX_INIT2 (2, NUM_MV_RES_CTX,    mc->mvResContexts,      INIT_MV_RES,     modelNum, qp);
    PBIARI_CTX_INIT2 (2, NUM_REF_NO_CTX,    mc->ref_no_contexts,      INIT_REF_NO,     modelNum, qp);
    PBIARI_CTX_INIT1 (   NUM_DELTA_QP_CTX,  mc->delta_qp_contexts,    INIT_DELTA_QP,   modelNum, qp);
    PBIARI_CTX_INIT1 (   NUM_MB_AFF_CTX,    mc->mb_aff_contexts,      INIT_MB_AFF,     modelNum, qp);

    // texture coding contexts
    PBIARI_CTX_INIT1 (   NUM_TRANSFORM_SIZE_CTX, tc->transform_size_contexts, INIT_TRANSFORM_SIZE, modelNum, qp);
    PBIARI_CTX_INIT1 (                 NUM_IPR_CTX,  tc->ipr_contexts,     INIT_IPR,       modelNum, qp);
    PBIARI_CTX_INIT1 (                 NUM_CIPR_CTX, tc->cipr_contexts,    INIT_CIPR,      modelNum, qp);
    PBIARI_CTX_INIT2 (3,               NUM_CBP_CTX,  tc->cbp_contexts,     INIT_CBP,       modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_BCBP_CTX, tc->bcbp_contexts,    INIT_BCBP,      modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts[0],  INIT_MAP,       modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts[1],  INIT_FLD_MAP,   modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts[1], INIT_FLD_LAST,  modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts[0], INIT_LAST,      modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ONE_CTX,  tc->one_contexts,     INIT_ONE,       modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ABS_CTX,  tc->abs_contexts,     INIT_ABS,       modelNum, qp);
    }
  }
//}}}

//{{{
static void resetMb (sMacroBlock* mb) {

  mb->errorFlag = 1;
  mb->dplFlag = 0;
  mb->sliceNum = -1;
  }
//}}}
//{{{
static void ercWriteMBmodeMV (sMacroBlock* mb) {

  sDecoder* decoder = mb->decoder;
  int curMbNum = mb->mbIndexX;
  sPicture* picture = decoder->picture;

  int mbx = xPosMB (curMbNum, picture->sizeX), mby = yPosMB(curMbNum, picture->sizeX);
  sObjectBuffer* curRegion = decoder->ercObjectList + (curMbNum << 2);

  if (decoder->coding.type != eBslice) {
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
  else {
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
  }
//}}}

//{{{
static void resetWpParam (sSlice* slice) {

  for (int i = 0; i < MAX_REFERENCE_PICTURES; i++) {
    for (int comp = 0; comp < 3; comp++) {
      int logWeightDenom = (comp == 0) ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
      slice->wpWeight[0][i][comp] = 1 << logWeightDenom;
      slice->wpWeight[1][i][comp] = 1 << logWeightDenom;
      }
    }
  }
//}}}
//{{{
static void fillWpParam (sSlice* slice) {

  if (slice->sliceType == eBslice) {
    int maxL0Ref = slice->numRefIndexActive[LIST_0];
    int maxL1Ref = slice->numRefIndexActive[LIST_1];
    if (slice->activePPS->weightedBiPredIdc == 2) {
      slice->lumaLog2weightDenom = 5;
      slice->chromaLog2weightDenom = 5;
      slice->wpRoundLuma = 16;
      slice->wpRoundChroma = 16;
      for (int i = 0; i < MAX_REFERENCE_PICTURES; ++i)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
          slice->wpWeight[0][i][comp] = 1 << logWeightDenom;
          slice->wpWeight[1][i][comp] = 1 << logWeightDenom;
          slice->wpOffset[0][i][comp] = 0;
          slice->wpOffset[1][i][comp] = 0;
          }
      }

    for (int i = 0; i < maxL0Ref; ++i)
      for (int j = 0; j < maxL1Ref; ++j)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
          if (slice->activePPS->weightedBiPredIdc == 1) {
            slice->wbpWeight[0][i][j][comp] = slice->wpWeight[0][i][comp];
            slice->wbpWeight[1][i][j][comp] = slice->wpWeight[1][j][comp];
            }
          else if (slice->activePPS->weightedBiPredIdc == 2) {
            int td = iClip3(-128,127,slice->listX[LIST_1][j]->poc - slice->listX[LIST_0][i]->poc);
            if (!td || slice->listX[LIST_1][j]->isLongTerm || slice->listX[LIST_0][i]->isLongTerm) {
              slice->wbpWeight[0][i][j][comp] = 32;
              slice->wbpWeight[1][i][j][comp] = 32;
              }
            else {
              int tb = iClip3(-128, 127, slice->thisPoc - slice->listX[LIST_0][i]->poc);
              int tx = (16384 + iabs (td / 2)) / td;
              int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
              slice->wbpWeight[1][i][j][comp] = distScaleFactor >> 2;
              slice->wbpWeight[0][i][j][comp] = 64 - slice->wbpWeight[1][i][j][comp];
              if (slice->wbpWeight[1][i][j][comp] < -64 || slice->wbpWeight[1][i][j][comp] > 128) {
                slice->wbpWeight[0][i][j][comp] = 32;
                slice->wbpWeight[1][i][j][comp] = 32;
                slice->wpOffset[0][i][comp] = 0;
                slice->wpOffset[1][j][comp] = 0;
                }
              }
            }
          }

    if (slice->mbAffFrame)
      for (int i = 0; i < 2 * maxL0Ref; ++i)
        for (int j = 0; j < 2 * maxL1Ref; ++j)
          for (int comp = 0; comp<3; ++comp)
            for (int k = 2; k < 6; k += 2) {
              slice->wpOffset[k+0][i][comp] = slice->wpOffset[0][i>>1][comp];
              slice->wpOffset[k+1][j][comp] = slice->wpOffset[1][j>>1][comp];
              int logWeightDenom = !comp ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
              if (slice->activePPS->weightedBiPredIdc == 1) {
                slice->wbpWeight[k+0][i][j][comp] = slice->wpWeight[0][i>>1][comp];
                slice->wbpWeight[k+1][i][j][comp] = slice->wpWeight[1][j>>1][comp];
                }
              else if (slice->activePPS->weightedBiPredIdc == 2) {
                int td = iClip3 (-128, 127, slice->listX[k+LIST_1][j]->poc - slice->listX[k+LIST_0][i]->poc);
                if (!td || slice->listX[k+LIST_1][j]->isLongTerm || slice->listX[k+LIST_0][i]->isLongTerm) {
                  slice->wbpWeight[k+0][i][j][comp] = 32;
                  slice->wbpWeight[k+1][i][j][comp] = 32;
                  }
                else {
                  int tb = iClip3 (-128,127, 
                               ((k == 2) ? slice->topPoc : slice->botPoc) - slice->listX[k+LIST_0][i]->poc);
                  int tx = (16384 + iabs(td/2)) / td;
                  int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
                  slice->wbpWeight[k+1][i][j][comp] = distScaleFactor >> 2;
                  slice->wbpWeight[k+0][i][j][comp] = 64 - slice->wbpWeight[k+1][i][j][comp];
                  if (slice->wbpWeight[k+1][i][j][comp] < -64 ||
                      slice->wbpWeight[k+1][i][j][comp] > 128) {
                    slice->wbpWeight[k+1][i][j][comp] = 32;
                    slice->wbpWeight[k+0][i][j][comp] = 32;
                    slice->wpOffset[k+0][i][comp] = 0;
                    slice->wpOffset[k+1][j][comp] = 0;
                    }
                  }
                }
              }
    }
  }
//}}}

//{{{
static void padBuf (sPixel* pixel, int width, int height, int stride, int padx, int pady) {

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
static void copyPOC (sSlice* fromSlice, sSlice* toSlice) {

  toSlice->topPoc = fromSlice->topPoc;
  toSlice->botPoc = fromSlice->botPoc;
  toSlice->thisPoc = fromSlice->thisPoc;
  toSlice->framePoc = fromSlice->framePoc;
  }
//}}}

//{{{
static void reorderLists (sSlice* slice) {

  sDecoder* decoder = slice->decoder;

  if ((slice->sliceType != eIslice) && (slice->sliceType != eSIslice)) {
    if (slice->refPicReorderFlag[LIST_0])
      reorderRefPicList (slice, LIST_0);
    if (decoder->noReferencePicture == slice->listX[0][slice->numRefIndexActive[LIST_0]-1])
      printf ("--- refPicList0[%d] no refPic %s\n",
              slice->numRefIndexActive[LIST_0]-1, decoder->nonConformingStream ? "conform":"");
    else // that's a definition
      slice->listXsize[0] = (char) slice->numRefIndexActive[LIST_0];
    }

  if (slice->sliceType == eBslice) {
    if (slice->refPicReorderFlag[LIST_1])
      reorderRefPicList (slice, LIST_1);
    if (decoder->noReferencePicture == slice->listX[1][slice->numRefIndexActive[LIST_1]-1])
      printf ("--- refPicList1[%d] no refPic %s\n",
              slice->numRefIndexActive[LIST_0] - 1, decoder->nonConformingStream ? "conform" : "");
    else
      // that's a definition
      slice->listXsize[1] = (char)slice->numRefIndexActive[LIST_1];
    }

  freeRefPicListReorderingBuffer (slice);
  }
//}}}
//{{{
static void initRefPicture (sSlice* slice, sDecoder* decoder) {

  sPicture* vidRefPicture = decoder->noReferencePicture;
  int noRef = slice->framePoc < decoder->recoveryPoc;

  if (decoder->coding.sepColourPlaneFlag) {
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
    int totalLists = slice->mbAffFrame ? 6 : (slice->sliceType == eBslice ? 2 : 1);
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
static void updateMbAff (sPixel** pixel, sPixel (*temp)[16], int x0, int width, int height) {

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
static void mbAffPostProc (sDecoder* decoder) {

  sPixel tempBuffer[32][16];

  sPicture* picture = decoder->picture;
  sPixel** imgY = picture->imgY;
  sPixel*** imgUV = picture->imgUV;

  short x0;
  short y0;
  for (short i = 0; i < (int)picture->picSizeInMbs; i += 2) {
    if (picture->motion.mbField[i]) {
      getMbPos (decoder, i, decoder->mbSize[IS_LUMA], &x0, &y0);
      updateMbAff (imgY + y0, tempBuffer, x0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

      if (picture->chromaFormatIdc != YUV400) {
        x0 = (short)((x0 * decoder->mbCrSizeX) >> 4);
        y0 = (short)((y0 * decoder->mbCrSizeY) >> 4);
        updateMbAff (imgUV[0] + y0, tempBuffer, x0, decoder->mbCrSizeX, decoder->mbCrSizeY);
        updateMbAff (imgUV[1] + y0, tempBuffer, x0, decoder->mbCrSizeX, decoder->mbCrSizeY);
        }
      }
    }
  }
//}}}

//{{{
static int isNewPicture (sPicture* picture, sSlice* slice, sOldSlice* oldSlice) {

  int result = (NULL == picture);

  result |= (oldSlice->ppsId != slice->ppsId);
  result |= (oldSlice->frameNum != slice->frameNum);
  result |= (oldSlice->fieldPicFlag != slice->fieldPicFlag);

  if (slice->fieldPicFlag && oldSlice->fieldPicFlag)
    result |= (oldSlice->botFieldFlag != slice->botFieldFlag);

  result |= (oldSlice->nalRefIdc != slice->refId) && (!oldSlice->nalRefIdc || !slice->refId);
  result |= (oldSlice->idrFlag != slice->idrFlag);

  if (slice->idrFlag && oldSlice->idrFlag)
    result |= (oldSlice->idrPicId != slice->idrPicId);

  sDecoder* decoder = slice->decoder;
  if (!decoder->activeSPS->pocType) {
    result |= (oldSlice->picOrderCountLsb != slice->picOrderCountLsb);
    if ((decoder->activePPS->botFieldPicOrderFramePresent == 1) && !slice->fieldPicFlag)
      result |= (oldSlice->deltaPicOrderCountBot != slice->deletaPicOrderCountBot);
    }

  if (decoder->activeSPS->pocType == 1) {
    if (!decoder->activeSPS->delta_pic_order_always_zero_flag) {
      result |= (oldSlice->deltaPicOrderCount[0] != slice->deltaPicOrderCount[0]);
      if ((decoder->activePPS->botFieldPicOrderFramePresent == 1) && !slice->fieldPicFlag)
        result |= (oldSlice->deltaPicOrderCount[1] != slice->deltaPicOrderCount[1]);
      }
    }

  return result;
  }
//}}}
//{{{
static void copyDecPictureJV (sDecoder* decoder, sPicture* dst, sPicture* src) {

  dst->poc = src->poc;
  dst->topPoc = src->topPoc;
  dst->botPoc = src->botPoc;
  dst->framePoc = src->framePoc;

  dst->qp = src->qp;
  dst->sliceQpDelta = src->sliceQpDelta;
  dst->chromaQpOffset[0] = src->chromaQpOffset[0];
  dst->chromaQpOffset[1] = src->chromaQpOffset[1];

  dst->sliceType = src->sliceType;
  dst->usedForReference = src->usedForReference;
  dst->idrFlag = src->idrFlag;
  dst->noOutputPriorPicFlag = src->noOutputPriorPicFlag;
  dst->longTermRefFlag = src->longTermRefFlag;
  dst->adaptRefPicBufFlag = src->adaptRefPicBufFlag;
  dst->decRefPicMarkingBuffer = src->decRefPicMarkingBuffer;
  dst->mbAffFrame = src->mbAffFrame;
  dst->picWidthMbs = src->picWidthMbs;
  dst->picNum  = src->picNum;
  dst->frameNum = src->frameNum;
  dst->recoveryFrame = src->recoveryFrame;
  dst->codedFrame = src->codedFrame;
  dst->chromaFormatIdc = src->chromaFormatIdc;
  dst->frameMbOnlyFlag = src->frameMbOnlyFlag;
  dst->cropFlag = src->cropFlag;
  dst->cropLeft = src->cropLeft;
  dst->cropRight = src->cropRight;
  dst->cropTop = src->cropTop;
  dst->cropBot = src->cropBot;
  }
//}}}
//{{{
static void initPicture (sDecoder* decoder, sSlice* slice) {

  sPicture* picture = NULL;

  sDPB* dpb = slice->dpb;
  sSPS* activeSPS = decoder->activeSPS;

  decoder->picHeightInMbs = decoder->coding.frameHeightMbs / (slice->fieldPicFlag+1);
  decoder->picSizeInMbs = decoder->coding.picWidthMbs * decoder->picHeightInMbs;
  decoder->coding.frameSizeMbs = decoder->coding.picWidthMbs * decoder->coding.frameHeightMbs;

  if (decoder->picture) // slice loss
    endDecodeFrame (decoder);

  if (decoder->recoveryPoint)
    decoder->recoveryFrameNum = (slice->frameNum + decoder->recoveryFrameCount) % decoder->coding.maxFrameNum;
  if (slice->idrFlag)
    decoder->recoveryFrameNum = slice->frameNum;
  if (!decoder->recoveryPoint &&
      (slice->frameNum != decoder->preFrameNum) &&
      (slice->frameNum != (decoder->preFrameNum + 1) % decoder->coding.maxFrameNum)) {
    if (!activeSPS->gaps_in_frame_num_value_allowed_flag) {
      // picture error conceal
      if (decoder->param.concealMode) {
        if ((slice->frameNum) < ((decoder->preFrameNum + 1) % decoder->coding.maxFrameNum)) {
          /* Conceal lost IDR frames and any frames immediately following the IDR.
          // Use frame copy for these since lists cannot be formed correctly for motion copy*/
          decoder->concealMode = 1;
          decoder->idrConcealFlag = 1;
          concealLostFrames (dpb, slice);
          // reset to original conceal mode for future drops
          decoder->concealMode = decoder->param.concealMode;
          }
        else {
          // reset to original conceal mode for future drops
          decoder->concealMode = decoder->param.concealMode;
          decoder->idrConcealFlag = 0;
          concealLostFrames (dpb, slice);
          }
        }
      else
        // Advanced Error Concealment would be called here to combat unintentional loss of pictures
        error ("initPicture - unintentional loss of picture\n");
      }
    if (!decoder->concealMode)
      fillFrameNumGap (decoder, slice);
    }

  if (slice->refId)
    decoder->preFrameNum = slice->frameNum;

  // calculate POC
  decodePOC (decoder, slice);

  if (decoder->recoveryFrameNum == (int)slice->frameNum && decoder->recoveryPoc == 0x7fffffff)
    decoder->recoveryPoc = slice->framePoc;

  if (slice->refId)
    decoder->lastRefPicPoc = slice->framePoc;

  if ((slice->picStructure == eFrame) || (slice->picStructure == eTopField))
    getTime (&decoder->info.startTime);

  picture = decoder->picture = allocPicture (decoder, slice->picStructure, decoder->width, decoder->height, decoder->widthCr, decoder->heightCr, 1);
  picture->topPoc = slice->topPoc;
  picture->botPoc = slice->botPoc;
  picture->framePoc = slice->framePoc;
  picture->qp = slice->qp;
  picture->sliceQpDelta = slice->sliceQpDelta;
  picture->chromaQpOffset[0] = decoder->activePPS->chromaQpIndexOffset;
  picture->chromaQpOffset[1] = decoder->activePPS->secondChromaQpIndexOffset;
  picture->iCodingType = slice->picStructure == eFrame ?
    (slice->mbAffFrame? eFrameMbPairCoding:eFrameCoding) : eFieldCoding;

  // reset all variables of the error conceal instance before decoding of every frame.
  // here the third parameter should, if perfectly, be equal to the number of slices per frame.
  // using little value is ok, the code will allocate more memory if the slice number is larger
  ercReset (decoder->ercErrorVar, decoder->picSizeInMbs, decoder->picSizeInMbs, picture->sizeX);

  decoder->ercMvPerMb = 0;
  switch (slice->picStructure ) {
    //{{{
    case eTopField:
      picture->poc = slice->topPoc;
      decoder->idrFrameNum *= 2;
      break;
    //}}}
    //{{{
    case eBotField:
      picture->poc = slice->botPoc;
      decoder->idrFrameNum = decoder->idrFrameNum * 2 + 1;
      break;
    //}}}
    //{{{
    case eFrame:
      picture->poc = slice->framePoc;
      break;
    //}}}
    //{{{
    default:
      error ("decoder->picStructure not initialized");
    //}}}
    }

  if (decoder->coding.type > eSIslice) {
    setEcFlag (decoder, SE_PTYPE);
    decoder->coding.type = ePslice;  // concealed element
    }

  // eCavlc init
  if (decoder->activePPS->entropyCodingMode == (Boolean)eCavlc)
    memset (decoder->nzCoeff[0][0][0], -1, decoder->picSizeInMbs * 48 *sizeof(byte)); // 3 * 4 * 4

  // Set the sliceNum member of each MB to -1, to ensure correct when packet loss occurs
  // TO set sMacroBlock Map (mark all MBs as 'have to be concealed')
  if (decoder->coding.sepColourPlaneFlag) {
    for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
      sMacroBlock* mb = decoder->mbDataJV[nplane];
      char* intraBlock = decoder->intraBlockJV[nplane];
      for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
        resetMb (mb++);
      memset (decoder->predModeJV[nplane][0], DC_PRED, 16 * decoder->coding.frameHeightMbs * decoder->coding.picWidthMbs * sizeof(char));
      if (decoder->activePPS->constrainedIntraPredFlag)
        for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
          intraBlock[i] = 1;
      }
    }
  else {
    sMacroBlock* mb = decoder->mbData;
    for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
      resetMb (mb++);
    if (decoder->activePPS->constrainedIntraPredFlag)
      for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
        decoder->intraBlock[i] = 1;
    memset (decoder->predMode[0], DC_PRED, 16 * decoder->coding.frameHeightMbs * decoder->coding.picWidthMbs * sizeof(char));
    }

  picture->sliceType = decoder->coding.type;
  picture->usedForReference = (slice->refId != 0);
  picture->idrFlag = slice->idrFlag;
  picture->noOutputPriorPicFlag = slice->noOutputPriorPicFlag;
  picture->longTermRefFlag = slice->longTermRefFlag;
  picture->adaptRefPicBufFlag = slice->adaptRefPicBufFlag;
  picture->decRefPicMarkingBuffer = slice->decRefPicMarkingBuffer;
  slice->decRefPicMarkingBuffer = NULL;

  picture->mbAffFrame = slice->mbAffFrame;
  picture->picWidthMbs = decoder->coding.picWidthMbs;

  decoder->getMbBlockPos = picture->mbAffFrame ? getMbBlockPosMbaff : getMbBlockPosNormal;
  decoder->getNeighbour = picture->mbAffFrame ? getAffNeighbour : getNonAffNeighbour;

  picture->picNum = slice->frameNum;
  picture->frameNum = slice->frameNum;
  picture->recoveryFrame = (unsigned int)((int)slice->frameNum == decoder->recoveryFrameNum);
  picture->codedFrame = (slice->picStructure == eFrame);
  picture->chromaFormatIdc = activeSPS->chromaFormatIdc;
  picture->frameMbOnlyFlag = activeSPS->frameMbOnlyFlag;
  picture->cropFlag = activeSPS->cropFlag;
  if (picture->cropFlag) {
    picture->cropLeft = activeSPS->cropLeft;
    picture->cropRight = activeSPS->cropRight;
    picture->cropTop = activeSPS->cropTop;
    picture->cropBot = activeSPS->cropBot;
    }

  if (decoder->coding.sepColourPlaneFlag) {
    decoder->decPictureJV[0] = decoder->picture;
    decoder->decPictureJV[1] = allocPicture (decoder, (ePicStructure) slice->picStructure, decoder->width, decoder->height, decoder->widthCr, decoder->heightCr, 1);
    copyDecPictureJV (decoder, decoder->decPictureJV[1], decoder->decPictureJV[0] );
    decoder->decPictureJV[2] = allocPicture (decoder, (ePicStructure) slice->picStructure, decoder->width, decoder->height, decoder->widthCr, decoder->heightCr, 1);
    copyDecPictureJV (decoder, decoder->decPictureJV[2], decoder->decPictureJV[0] );
    }
  }
//}}}

//{{{
static void useParameterSet (sDecoder* decoder, sSlice* slice) {

  sPPS* pps = &decoder->pps[slice->ppsId];
  if (!pps->valid)
    printf ("useParameterSet - invalid PPS id:%d\n", slice->ppsId);

  sSPS* sps = &decoder->sps[pps->spsId];
  if (!sps->valid)
    printf ("useParameterSet - no SPS id:%d:%d\n", slice->ppsId, pps->spsId);

  // In theory, and with a well-designed software, the lines above are everything necessary.
  // In practice, we need to patch many values
  // in decoder-> (but no more in input. -- these have been taken care of)
  // Set Sequence Parameter Stuff first
  if (sps->pocType > 2)
    error ("invalid SPS pocType");
  if (sps->pocType == 1)
    if (sps->numRefFramesPocCycle >= MAX_NUM_REF_FRAMES_PIC_ORDER)
      error ("numRefFramesPocCycle too large");

  activateSPS (decoder, sps);
  activatePPS (decoder, pps);

  // slice->dataDpMode is set by read_new_slice (NALU first byte available there)
  if (pps->entropyCodingMode == (Boolean)eCavlc) {
    slice->nalStartCode = vlcStartCode;
    for (int i = 0; i < 3; i++)
      slice->dataPartitions[i].readSyntaxElement = readSyntaxElementVLC;
    }
  else {
    slice->nalStartCode = cabacStartCode;
    for (int i = 0; i < 3; i++)
      slice->dataPartitions[i].readSyntaxElement = readSyntaxElementCABAC;
    }
  decoder->coding.type = slice->sliceType;
  }
//}}}
//{{{
static void readSlice (sDecoder* decoder, sSlice* slice) {
// Some slice syntax depends on parameterSet depends on parameterSetID of the slice header
// - read the ppsId of the slice header first
//   - then setup the active parameter sets
// - read the rest of the slice header

  byte partitionIndex = assignSE2dp[slice->dataDpMode][SE_HEADER];
  sDataPartition* dataPartition = &(slice->dataPartitions[partitionIndex]);
  sBitStream* s = dataPartition->s;

  slice->startMbNum = readUeV ("SLC first_mb_in_slice", s);

  int tmp = readUeV ("SLC sliceType", s);
  if (tmp > 4)
    tmp -= 5;
  slice->sliceType = (eSliceType)tmp;
  decoder->coding.type = slice->sliceType;

  slice->ppsId = readUeV ("SLC ppsId", s);

  if (decoder->coding.sepColourPlaneFlag)
    slice->colourPlaneId = readUv (2, "SLC colourPlaneId", s);
  else
    slice->colourPlaneId = PLANE_Y;

  // setup parameterSet
  useParameterSet (decoder, slice);
  slice->activeSPS = decoder->activeSPS;
  slice->activePPS = decoder->activePPS;
  slice->transform8x8Mode = decoder->activePPS->transform8x8modeFlag;
  slice->chroma444notSeparate = (decoder->activeSPS->chromaFormatIdc == YUV444) && !decoder->coding.sepColourPlaneFlag;

  sSPS* activeSPS = decoder->activeSPS;
  slice->frameNum = readUv (activeSPS->log2_max_frame_num_minus4 + 4, "SLC frameNum", s);
  if (slice->idrFlag) {
    decoder->preFrameNum = slice->frameNum;
    decoder->lastRefPicPoc = 0;
    }
  //{{{  read field/frame stuff
  if (activeSPS->frameMbOnlyFlag) {
    decoder->coding.picStructure = eFrame;
    slice->fieldPicFlag = 0;
    }

  else {
    slice->fieldPicFlag = readU1 ("SLC fieldPicFlag", s);
    if (slice->fieldPicFlag) {
      slice->botFieldFlag = (byte)readU1 ("SLC botFieldFlag", s);
      decoder->coding.picStructure = slice->botFieldFlag ? eBotField : eTopField;
      }
    else {
      decoder->coding.picStructure = eFrame;
      slice->botFieldFlag = FALSE;
      }
    }

  slice->picStructure = (ePicStructure)decoder->coding.picStructure;
  slice->mbAffFrame = activeSPS->mbAffFlag && !slice->fieldPicFlag;
  //}}}

  if (slice->idrFlag)
    slice->idrPicId = readUeV ("SLC idrPicId", s);
  //{{{  read picOrderCount
  if (activeSPS->pocType == 0) {
    slice->picOrderCountLsb = readUv (activeSPS->log2_max_pic_order_cnt_lsb_minus4 + 4, "SLC picOrderCountLsb", s);
    if ((decoder->activePPS->botFieldPicOrderFramePresent == 1) && !slice->fieldPicFlag)
      slice->deletaPicOrderCountBot = readSeV ("SLC deletaPicOrderCountBot", s);
    else
      slice->deletaPicOrderCountBot = 0;
    }

  if (activeSPS->pocType == 1) {
    if (!activeSPS->delta_pic_order_always_zero_flag) {
      slice->deltaPicOrderCount[0] = readSeV ("SLC deltaPicOrderCount[0]", s);
      if ((decoder->activePPS->botFieldPicOrderFramePresent == 1) && !slice->fieldPicFlag)
        slice->deltaPicOrderCount[1] = readSeV ("SLC deltaPicOrderCount[1]", s);
      else
        slice->deltaPicOrderCount[1] = 0;  // set to zero if not in stream
      }
    else {
      slice->deltaPicOrderCount[0] = 0;
      slice->deltaPicOrderCount[1] = 0;
      }
    }
  //}}}

  if (decoder->activePPS->redundantPicCountPresent)
    slice->redundantPicCount = readUeV ("SLC redundantPicCount", s);

  if (slice->sliceType == eBslice)
    slice->directSpatialMvPredFlag = readU1 ("SLC directSpatialMvPredFlag", s);

  // read refPics
  slice->numRefIndexActive[LIST_0] = decoder->activePPS->numRefIndexL0defaultActiveMinus1 + 1;
  slice->numRefIndexActive[LIST_1] = decoder->activePPS->numRefIndexL1defaultActiveMinus1 + 1;
  if ((slice->sliceType == ePslice) || (slice->sliceType == eSPslice) || (slice->sliceType == eBslice)) {
    int num_ref_idx_override_flag = readU1 ("SLC num_ref_idx_override_flag", s);
    if (num_ref_idx_override_flag) {
      slice->numRefIndexActive[LIST_0] = 1 + readUeV ("SLC num_ref_idx_l0_active_minus1", s);
      if (slice->sliceType == eBslice)
        slice->numRefIndexActive[LIST_1] = 1 + readUeV ("SLC num_ref_idx_l1_active_minus1", s);
      }
    }
  if (slice->sliceType != eBslice)
    slice->numRefIndexActive[LIST_1] = 0;
  //{{{  refPicList reorder
  allocRefPicListReordeBuffer (slice);

  if ((slice->sliceType != eIslice) && (slice->sliceType != eSIslice)) {
    int value = slice->refPicReorderFlag[LIST_0] = readU1 ("SLC refPicReorderL0", s);
    if (value) {
      int i = 0;
      do {
        value = slice->modPicNumsIdc[LIST_0][i] = readUeV("SLC modPicNumsIdcl0", s);
        if ((value == 0) || (value == 1))
          slice->absDiffPicNumMinus1[LIST_0][i] = readUeV ("SLC absDiffPicNumMinus1L0", s);
        else if (value == 2)
          slice->longTermPicIndex[LIST_0][i] = readUeV ("SLC longTermPicIndexL0", s);
        i++;
        } while (value != 3);
      }
    }

  if (slice->sliceType == eBslice) {
    int value = slice->refPicReorderFlag[LIST_1] = readU1 ("SLC refPicReorderL1", s);
    if (value) {
      int i = 0;
      do {
        value = slice->modPicNumsIdc[LIST_1][i] = readUeV ("SLC modPicNumsIdcl1", s);
        if ((value == 0) || (value == 1))
          slice->absDiffPicNumMinus1[LIST_1][i] = readUeV ("SLC absDiffPicNumMinus1L1", s);
        else if (value == 2)
          slice->longTermPicIndex[LIST_1][i] = readUeV ("SLC longTermPicIndexL1", s);
        i++;
        } while (value != 3);
      }
    }

  // set reference index of redundant slices.
  if (slice->redundantPicCount && (slice->sliceType != eIslice) )
    slice->redundantSliceRefIndex = slice->absDiffPicNumMinus1[LIST_0][0] + 1;
  //}}}
  //{{{  read weightedPred
  slice->weightedPredFlag = (unsigned short)(((slice->sliceType == ePslice) || (slice->sliceType == eSPslice))
                              ? decoder->activePPS->weightedPredFlag
                              : ((slice->sliceType == eBslice) && (decoder->activePPS->weightedBiPredIdc == 1)));

  slice->weightedBiPredIdc = (unsigned short)((slice->sliceType == eBslice) &&
                                              (decoder->activePPS->weightedBiPredIdc > 0));

  if ((decoder->activePPS->weightedPredFlag &&
       ((slice->sliceType == ePslice) || (slice->sliceType == eSPslice))) ||
      ((decoder->activePPS->weightedBiPredIdc == 1) && (slice->sliceType == eBslice))) {
    slice->lumaLog2weightDenom = (unsigned short) readUeV ("SLC lumaLog2weightDenom", s);
    slice->wpRoundLuma = slice->lumaLog2weightDenom ? 1 << (slice->lumaLog2weightDenom - 1) : 0;

    if (activeSPS->chromaFormatIdc) {
      slice->chromaLog2weightDenom = (unsigned short) readUeV ("SLC chromaLog2weightDenom", s);
      slice->wpRoundChroma = slice->chromaLog2weightDenom ? 1 << (slice->chromaLog2weightDenom - 1) : 0;
      }

    resetWpParam (slice);

    for (int i = 0; i < slice->numRefIndexActive[LIST_0]; i++) {
      //{{{  read l0 weights
      int luma_weight_flag_l0 = readU1 ("SLC luma_weight_flag_l0", s);
      if (luma_weight_flag_l0) {
        slice->wpWeight[LIST_0][i][0] = readSeV ("SLC luma_weight_l0", s);
        slice->wpOffset[LIST_0][i][0] = readSeV ("SLC luma_offset_l0", s);
        slice->wpOffset[LIST_0][i][0] = slice->wpOffset[LIST_0][i][0] << (decoder->bitDepthLuma - 8);
        }
      else {
        slice->wpWeight[LIST_0][i][0] = 1 << slice->lumaLog2weightDenom;
        slice->wpOffset[LIST_0][i][0] = 0;
        }

      if (activeSPS->chromaFormatIdc) {
        // l0 chroma weights
        int chroma_weight_flag_l0 = readU1 ("SLC chroma_weight_flag_l0", s);
        for (int j = 1; j<3; j++) {
          if (chroma_weight_flag_l0) {
            slice->wpWeight[LIST_0][i][j] = readSeV ("SLC chroma_weight_l0", s);
            slice->wpOffset[LIST_0][i][j] = readSeV ("SLC chroma_offset_l0", s);
            slice->wpOffset[LIST_0][i][j] = slice->wpOffset[LIST_0][i][j] << (decoder->bitDepthChroma-8);
            }
          else {
            slice->wpWeight[LIST_0][i][j] = 1 << slice->chromaLog2weightDenom;
            slice->wpOffset[LIST_0][i][j] = 0;
            }
          }
        }
      }
      //}}}

    if ((slice->sliceType == eBslice) && decoder->activePPS->weightedBiPredIdc == 1) {
      for (int i = 0; i < slice->numRefIndexActive[LIST_1]; i++) {
        //{{{  read l1 weights
        int luma_weight_flag_l1 = readU1 ("SLC luma_weight_flag_l1", s);
        if (luma_weight_flag_l1) {
          slice->wpWeight[LIST_1][i][0] = readSeV ("SLC luma_weight_l1", s);
          slice->wpOffset[LIST_1][i][0] = readSeV ("SLC luma_offset_l1", s);
          slice->wpOffset[LIST_1][i][0] = slice->wpOffset[LIST_1][i][0] << (decoder->bitDepthLuma-8);
          }
        else {
          slice->wpWeight[LIST_1][i][0] = 1 << slice->lumaLog2weightDenom;
          slice->wpOffset[LIST_1][i][0] = 0;
          }

        if (activeSPS->chromaFormatIdc) {
          // l1 chroma weights
          int chroma_weight_flag_l1 = readU1 ("SLC chroma_weight_flag_l1", s);
          for (int j = 1; j < 3; j++) {
            if (chroma_weight_flag_l1) {
              slice->wpWeight[LIST_1][i][j] = readSeV("SLC chroma_weight_l1", s);
              slice->wpOffset[LIST_1][i][j] = readSeV("SLC chroma_offset_l1", s);
              slice->wpOffset[LIST_1][i][j] = slice->wpOffset[LIST_1][i][j]<<(decoder->bitDepthChroma-8);
              }
            else {
              slice->wpWeight[LIST_1][i][j] = 1 << slice->chromaLog2weightDenom;
              slice->wpOffset[LIST_1][i][j] = 0;
              }
            }
          }
        }
        //}}}
      }
    }
  //}}}

  if (slice->refId)
    decRefPicMarking (decoder, s, slice);

  if (decoder->activePPS->entropyCodingMode &&
      (slice->sliceType != eIslice) && (slice->sliceType != eSIslice))
    slice->modelNum = readUeV ("SLC cabac_init_idc", s);
  else
    slice->modelNum = 0;
  //{{{  read qp
  slice->sliceQpDelta = readSeV ("SLC sliceQpDelta", s);
  slice->qp = 26 + decoder->activePPS->picInitQpMinus26 + slice->sliceQpDelta;

  if (slice->sliceType == eSPslice || slice->sliceType == eSIslice) {
    if (slice->sliceType == eSPslice)
      slice->spSwitch = readU1 ("SLC sp_for_switch_flag", s);
    slice->sliceQsDelta = readSeV ("SLC sliceQsDelta", s);
    slice->qs = 26 + decoder->activePPS->picInitQsMinus26 + slice->sliceQsDelta;
    }
  //}}}

  if (decoder->activePPS->deblockFilterControlPresent) {
    //{{{  read deblockFilter params
    slice->deblockFilterDisableIdc = (short)readUeV ("SLC disable_deblocking_filter_idc", s);
    if (slice->deblockFilterDisableIdc != 1) {
      slice->deblockFilterC0offset = (short)(2 * readSeV ("SLC slice_alpha_c0_offset_div2", s));
      slice->deblockFilterBetaOffset = (short)(2 * readSeV ("SLC slice_beta_offset_div2", s));
      }
    else
      slice->deblockFilterC0offset = slice->deblockFilterBetaOffset = 0;
    }
    //}}}
  else {
    //{{{  enable deblockFilter
    slice->deblockFilterDisableIdc = 0;
    slice->deblockFilterC0offset = 0;
    slice->deblockFilterBetaOffset = 0;
    }
    //}}}
  if (isHiIntraOnlyProfile (activeSPS->profileIdc, activeSPS->constrainedSet3flag) &&
      !decoder->param.intraProfileDeblocking) {
    //{{{  hiIintra deblock
    slice->deblockFilterDisableIdc = 1;
    slice->deblockFilterC0offset = 0;
    slice->deblockFilterBetaOffset = 0;
    }
    //}}}
  //{{{  read sliceGroup
  if ((decoder->activePPS->numSliceGroupsMinus1 > 0) &&
      (decoder->activePPS->sliceGroupMapType >= 3) &&
      (decoder->activePPS->sliceGroupMapType <= 5)) {
    int len = (activeSPS->pic_height_in_map_units_minus1+1) * (activeSPS->pic_width_in_mbs_minus1+1) /
                (decoder->activePPS->sliceGroupChangeRateMius1 + 1);
    if (((activeSPS->pic_height_in_map_units_minus1+1) * (activeSPS->pic_width_in_mbs_minus1+1)) %
        (decoder->activePPS->sliceGroupChangeRateMius1 + 1))
      len += 1;

    len = ceilLog2 (len+1);
    slice->sliceGroupChangeCycle = readUv (len, "SLC sliceGroupChangeCycle", s);
    }
  //}}}

  decoder->picHeightInMbs = decoder->coding.frameHeightMbs / ( 1 + slice->fieldPicFlag );
  decoder->picSizeInMbs = decoder->coding.picWidthMbs * decoder->picHeightInMbs;
  decoder->coding.frameSizeMbs = decoder->coding.picWidthMbs * decoder->coding.frameHeightMbs;
  }
//}}}
//{{{
static void initPictureDecoding (sDecoder* decoder) {

  int deblockMode = 1;

  if (decoder->picSliceIndex >= MAX_NUM_SLICES)
    error ("initPictureDecoding - MAX_NUM_SLICES exceeded");

  sSlice* slice = decoder->sliceList[0];
  if (decoder->nextPPS->valid && (decoder->nextPPS->ppsId == slice->ppsId)) {
    if (decoder->param.sliceDebug)
      printf ("--- initPictureDecoding - switching PPS - nextPPS:%d == slicePPS:%d\n",
              decoder->nextPPS->ppsId, slice->ppsId);

    sPPS pps;
    memcpy (&pps, &(decoder->pps[slice->ppsId]), sizeof (sPPS));
    decoder->pps[slice->ppsId].sliceGroupId = NULL;
    setPPSbyId (decoder, decoder->nextPPS->ppsId, decoder->nextPPS);
    memcpy (decoder->nextPPS, &pps, sizeof (sPPS));
    pps.sliceGroupId = NULL;
    }

  useParameterSet (decoder, slice);
  if (slice->idrFlag)
    decoder->idrFrameNum = 0;

  decoder->picHeightInMbs = decoder->coding.frameHeightMbs / (1 + slice->fieldPicFlag);
  decoder->picSizeInMbs = decoder->coding.picWidthMbs * decoder->picHeightInMbs;
  decoder->coding.frameSizeMbs = decoder->coding.picWidthMbs * decoder->coding.frameHeightMbs;
  decoder->coding.picStructure = slice->picStructure;
  initFmo (decoder, slice);

  updatePicNum (slice);

  initDeblock (decoder, slice->mbAffFrame);
  for (int j = 0; j < decoder->picSliceIndex; j++)
    if (decoder->sliceList[j]->deblockFilterDisableIdc != 1)
      deblockMode = 0;

  decoder->deblockMode = deblockMode;
  }
//}}}
//{{{
static void initSlice (sDecoder* decoder, sSlice* slice) {

  decoder->activeSPS = slice->activeSPS;
  decoder->activePPS = slice->activePPS;

  slice->initLists (slice);
  reorderLists (slice);

  if (slice->picStructure == eFrame)
    init_mbaff_lists (decoder, slice);

  // update reference flags and set current decoder->refFlag
  if (!(slice->redundantPicCount && (decoder->prevFrameNum == slice->frameNum)))
    for (int i = 16; i > 0; i--)
      slice->refFlag[i] = slice->refFlag[i-1];
  slice->refFlag[0] = (slice->redundantPicCount == 0) ? decoder->isPrimaryOk : decoder->isRedundantOk;

  if (!slice->activeSPS->chromaFormatIdc ||
      (slice->activeSPS->chromaFormatIdc == 3)) {
    slice->linfoCbpIntra = linfo_cbp_intra_other;
    slice->linfoCbpInter = linfo_cbp_inter_other;
    }
  else {
    slice->linfoCbpIntra = linfo_cbp_intra_normal;
    slice->linfoCbpInter = linfo_cbp_inter_normal;
    }
  }
//}}}
//{{{
static void copySliceInfo (sSlice* slice, sOldSlice* oldSlice) {

  oldSlice->ppsId = slice->ppsId;
  oldSlice->frameNum = slice->frameNum;
  oldSlice->fieldPicFlag = slice->fieldPicFlag;

  if (slice->fieldPicFlag)
    oldSlice->botFieldFlag = slice->botFieldFlag;

  oldSlice->nalRefIdc = slice->refId;

  oldSlice->idrFlag = (byte)slice->idrFlag;
  if (slice->idrFlag)
    oldSlice->idrPicId = slice->idrPicId;

  if (slice->decoder->activeSPS->pocType == 0) {
    oldSlice->picOrderCountLsb = slice->picOrderCountLsb;
    oldSlice->deltaPicOrderCountBot = slice->deletaPicOrderCountBot;
    }
  else if (slice->decoder->activeSPS->pocType == 1) {
    oldSlice->deltaPicOrderCount[0] = slice->deltaPicOrderCount[0];
    oldSlice->deltaPicOrderCount[1] = slice->deltaPicOrderCount[1];
    }
  }
//}}}

//{{{
static void decodeSlice (sSlice* slice) {

  Boolean endOfSlice = FALSE;

  slice->codCount = -1;

  sDecoder* decoder = slice->decoder;
  if (decoder->coding.sepColourPlaneFlag)
    changePlaneJV (decoder, slice->colourPlaneId, slice);
  else {
    slice->mbData = decoder->mbData;
    slice->picture = decoder->picture;
    slice->siBlock = decoder->siBlock;
    slice->predMode = decoder->predMode;
    slice->intraBlock = decoder->intraBlock;
    }

  if (slice->sliceType == eBslice)
    computeColocated (slice, slice->listX);

  if ((slice->sliceType != eIslice) && (slice->sliceType != eSIslice))
    initRefPicture (slice, decoder);

  // loop over macroblocks
  while (!endOfSlice) {
    sMacroBlock* mb;
    startMacroblock (slice, &mb);
    slice->readMacroblock (mb);
    decodeMacroblock (mb, slice->picture);

    if (slice->mbAffFrame && mb->mbField) {
      slice->numRefIndexActive[LIST_0] >>= 1;
      slice->numRefIndexActive[LIST_1] >>= 1;
      }

    ercWriteMBmodeMV (mb);
    endOfSlice = exitMacroblock (slice, !slice->mbAffFrame || (slice->mbIndex % 2));
    }
  }
//}}}
//{{{
static int readNextSlice (sSlice* slice) {

  sDecoder* decoder = slice->decoder;

  int curHeader = 0;

  for (;;) {
    sNalu* nalu = decoder->nalu;
    if (!decoder->pendingNalu) {
      if (!readNextNalu (decoder, nalu))
        return EOS;
      }
    else {
      nalu = decoder->pendingNalu;
      decoder->pendingNalu = NULL;
      }

  process_nalu:
    switch (nalu->unitType) {
      //{{{
      case NALU_TYPE_SPS:
        if (decoder->param.spsDebug)
          printf ("SPS %2d:%d:%d ", nalu->len, slice->refId, slice->sliceType);
        readNaluSPS (decoder, nalu);
        break;
      //}}}
      //{{{
      case NALU_TYPE_PPS:
        if (decoder->param.ppsDebug)
          printf ("PPS %2d:%d:%d ", nalu->len, slice->refId, slice->sliceType);
        readNaluPPS (decoder, nalu);
        break;
      //}}}
      //{{{
      case NALU_TYPE_SEI:
        if (decoder->param.seiDebug)
          printf ("SEI %2d:%d:%d ", nalu->len, slice->refId, slice->sliceType);
        processSEI (nalu->buf, nalu->len, decoder, slice);
        break;
      //}}}

      case NALU_TYPE_SLICE:
      //{{{
      case NALU_TYPE_IDR:
        if (decoder->recoveryPoint || nalu->unitType == NALU_TYPE_IDR) {
          if (!decoder->recoveryPointFound) {
            if (nalu->unitType != NALU_TYPE_IDR) {
              printf ("-> decoding without IDR");
              decoder->nonConformingStream = 1;
              }
            else
              decoder->nonConformingStream = 0;
            }
          decoder->recoveryPointFound = 1;
          }
        if (!decoder->recoveryPointFound)
          break;

        slice->idrFlag = nalu->unitType == NALU_TYPE_IDR;
        slice->refId = nalu->refId;
        slice->dataDpMode = eDataPartition1;
        slice->maxDataPartitions = 1;
        sBitStream* s = slice->dataPartitions[0].s;
        s->errorFlag = 0;
        s->bitStreamOffset = 0;
        s->readLen = 0;
        memcpy (s->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
        s->bitStreamLen = RBSPtoSODB (s->bitStreamBuffer, nalu->len - 1);
        s->codeLen = s->bitStreamLen;

        readSlice (decoder, slice);
        assignQuantParams (slice);

        if (decoder->param.sliceDebug) {
          //{{{  print sliceDebug
          if (nalu->unitType == NALU_TYPE_IDR)
            printf ("IDR");
          else
            printf ("SLC");

          printf (" %5d:%d:%d:%s frame:%d%s%s ppsId:%d\n",
                  nalu->len, slice->refId, slice->sliceType,
                  (slice->sliceType == 0) ? "P":
                    (slice->sliceType == 1 ? "B":
                      (slice->sliceType == 2 ? "I" : "?")),
                  slice->frameNum,
                  slice->mbAffFrame ? " mbAff":"",
                  slice->fieldPicFlag ? " field":"",
                  slice->ppsId);
          }
          //}}}

        // if primary slice is replaced with redundant slice, set the correct image type
        if (slice->redundantPicCount && !decoder->isPrimaryOk && decoder->isRedundantOk)
          decoder->picture->sliceType = decoder->coding.type;

        if (isNewPicture (decoder->picture, slice, decoder->oldSlice)) {
          if (!decoder->picSliceIndex)
            initPicture (decoder, slice);
          curHeader = SOP;
          checkZeroByteVCL (decoder, nalu);
          }
        else
          curHeader = SOS;

        setSliceMethods (slice);

        if (slice->mbAffFrame)
          slice->mbIndex = slice->startMbNum << 1;
        else
          slice->mbIndex = slice->startMbNum;

        if (decoder->activePPS->entropyCodingMode) {
          int byteStartPosition = s->bitStreamOffset / 8;
          if (s->bitStreamOffset % 8)
            ++byteStartPosition;
          aridecoStartDecoding (&slice->dataPartitions[0].deCabac, s->bitStreamBuffer, byteStartPosition, &s->readLen);
          }

        decoder->recoveryPoint = 0;
        return curHeader;
      //}}}

      //{{{
      case NALU_TYPE_DPA:
        printf ("DPA id:%d:%d len:%d\n", slice->refId, slice->sliceType, nalu->len);

        if (!decoder->recoveryPointFound)
          break;

        // read dataPartition A
        slice->idrFlag = FALSE;
        slice->refId = nalu->refId;
        slice->noDataPartitionB = 1;
        slice->noDataPartitionC = 1;
        slice->dataDpMode = eDataPartition3;
        slice->maxDataPartitions = 3;
        s = slice->dataPartitions[0].s;
        s->errorFlag = 0;
        s->bitStreamOffset = s->readLen = 0;
        memcpy (s->bitStreamBuffer, &nalu->buf[1], nalu->len - 1);
        s->codeLen = s->bitStreamLen = RBSPtoSODB (s->bitStreamBuffer, nalu->len-1);
        readSlice (decoder, slice);
        assignQuantParams (slice);

        if (isNewPicture (decoder->picture, slice, decoder->oldSlice)) {
          if (!decoder->picSliceIndex)
            initPicture (decoder, slice);
          curHeader = SOP;
          checkZeroByteVCL (decoder, nalu);
          }
        else
          curHeader = SOS;

        setSliceMethods (slice);

        // From here on, decoder->activeSPS, decoder->activePPS and the slice header are valid
        if (slice->mbAffFrame)
          slice->mbIndex = slice->startMbNum << 1;
        else
          slice->mbIndex = slice->startMbNum;

        // need to read the slice ID, which depends on the value of redundantPicCountPresent
        int slice_id_a = readUeV ("NALU: DP_A slice_id", s);
        if (decoder->activePPS->entropyCodingMode)
          error ("dataPartition with eCabac not allowed");

        if (!readNextNalu (decoder, nalu))
          return curHeader;

        if (NALU_TYPE_DPB == nalu->unitType) {
          //{{{  got nalu dataPartitionB
          s = slice->dataPartitions[1].s;
          s->errorFlag = 0;
          s->bitStreamOffset = s->readLen = 0;
          memcpy (s->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
          s->codeLen = s->bitStreamLen = RBSPtoSODB (s->bitStreamBuffer, nalu->len-1);
          int slice_id_b = readUeV ("NALU dataPartitionB sliceId", s);
          slice->noDataPartitionB = 0;

          if ((slice_id_b != slice_id_a) || (nalu->lostPackets)) {
            printf ("NALU dataPartitionB does not match dataPartitionA\n");
            slice->noDataPartitionB = 1;
            slice->noDataPartitionC = 1;
            }
          else {
            if (decoder->activePPS->redundantPicCountPresent)
              readUeV ("NALU dataPartitionB redundantPicCount", s);

            // we're finished with dataPartitionB, so let's continue with next dataPartition
            if (!readNextNalu (decoder, nalu))
              return curHeader;
            }
          }
          //}}}
        else
          slice->noDataPartitionB = 1;

        if (NALU_TYPE_DPC == nalu->unitType) {
          //{{{  got nalu dataPartitionC
          s = slice->dataPartitions[2].s;
          s->errorFlag = 0;
          s->bitStreamOffset = s->readLen = 0;
          memcpy (s->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
          s->codeLen = s->bitStreamLen = RBSPtoSODB (s->bitStreamBuffer, nalu->len-1);

          slice->noDataPartitionC = 0;
          int slice_id_c = readUeV ("NALU: DP_C slice_id", s);
          if ((slice_id_c != slice_id_a) || (nalu->lostPackets)) {
            printf ("dataPartitionC does not match dataPartitionA\n");
            slice->noDataPartitionC = 1;
            }

          if (decoder->activePPS->redundantPicCountPresent)
            readUeV ("NALU:SLICE_C redudand_pic_cnt", s);
          }
          //}}}
        else {
          slice->noDataPartitionC = 1;
          decoder->pendingNalu = nalu;
          }

        // check if we read anything else than the expected dataPartitions
        if ((nalu->unitType != NALU_TYPE_DPB) &&
            (nalu->unitType != NALU_TYPE_DPC) && (!slice->noDataPartitionC))
          goto process_nalu;

        return curHeader;
      //}}}
      //{{{
      case NALU_TYPE_DPB:
        printf ("dataPartitionB without dataPartitonA\n");
        break;
      //}}}
      //{{{
      case NALU_TYPE_DPC:
        printf ("dataPartitionC without dataPartitonA\n");
        break;
      //}}}

      case NALU_TYPE_AUD: break;
      case NALU_TYPE_EOSEQ: break;
      case NALU_TYPE_EOSTREAM: break;
      case NALU_TYPE_FILL: break;
      //{{{
      default:
        printf ("NALU - unknown type %d, len %d\n", (int) nalu->unitType, (int) nalu->len);
        break;
      //}}}
      }
    }

  return curHeader;
  }
//}}}

//{{{
void decRefPicMarking (sDecoder* decoder, sBitStream* s, sSlice* slice) {

  // free old buffer content
  while (slice->decRefPicMarkingBuffer) {
    sDecodedRefPicMarking* tmp_drpm = slice->decRefPicMarkingBuffer;
    slice->decRefPicMarkingBuffer = tmp_drpm->next;
    free (tmp_drpm);
    }

  if (slice->idrFlag) {
    slice->noOutputPriorPicFlag = readU1 ("SLC noOutputPriorPicFlag", s);
    decoder->noOutputPriorPicFlag = slice->noOutputPriorPicFlag;
    slice->longTermRefFlag = readU1 ("SLC longTermRefFlag", s);
    }
  else {
    slice->adaptRefPicBufFlag = readU1 ("SLC adaptRefPicBufFlag", s);
    if (slice->adaptRefPicBufFlag) {
      // read Memory Management Control Operation
      int val;
      do {
        sDecodedRefPicMarking* tempDrpm = (sDecodedRefPicMarking*)calloc (1,sizeof (sDecodedRefPicMarking));
        tempDrpm->next = NULL;
        val = tempDrpm->memManagement = readUeV ("SLC memManagement", s);
        if ((val == 1) || (val == 3))
          tempDrpm->diffPicNumMinus1 = readUeV ("SLC diffPicNumMinus1", s);
        if (val == 2)
          tempDrpm->longTermPicNum = readUeV ("SLC longTermPicNum", s);

        if ((val == 3 ) || (val == 6))
          tempDrpm->longTermFrameIndex = readUeV ("SLC longTermFrameIndex", s);
        if (val == 4)
          tempDrpm->maxLongTermFrameIndexPlus1 = readUeV ("SLC max_long_term_pic_idx_plus1", s);

        // add command
        if (!slice->decRefPicMarkingBuffer)
          slice->decRefPicMarkingBuffer = tempDrpm;
        else {
          sDecodedRefPicMarking* tempDrpm2 = slice->decRefPicMarkingBuffer;
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
void decodePOC (sDecoder* decoder, sSlice* slice) {

  sSPS* activeSPS = decoder->activeSPS;
  unsigned int maxPicOrderCntLsb = (1<<(activeSPS->log2_max_pic_order_cnt_lsb_minus4+4));

  switch (activeSPS->pocType) {
    //{{{
    case 0: // POC MODE 0
      // 1st
      if (slice->idrFlag) {
        decoder->PrevPicOrderCntMsb = 0;
        decoder->PrevPicOrderCntLsb = 0;
        }
      else if (decoder->lastHasMmco5) {
        if (decoder->lastPicBotField) {
          decoder->PrevPicOrderCntMsb = 0;
          decoder->PrevPicOrderCntLsb = 0;
          }
        else {
          decoder->PrevPicOrderCntMsb = 0;
          decoder->PrevPicOrderCntLsb = slice->topPoc;
          }
        }

      // Calculate the MSBs of current picture
      if (slice->picOrderCountLsb < decoder->PrevPicOrderCntLsb  &&
          (decoder->PrevPicOrderCntLsb - slice->picOrderCountLsb) >= maxPicOrderCntLsb/2)
        slice->PicOrderCntMsb = decoder->PrevPicOrderCntMsb + maxPicOrderCntLsb;
      else if (slice->picOrderCountLsb > decoder->PrevPicOrderCntLsb &&
               (slice->picOrderCountLsb - decoder->PrevPicOrderCntLsb) > maxPicOrderCntLsb/2)
        slice->PicOrderCntMsb = decoder->PrevPicOrderCntMsb - maxPicOrderCntLsb;
      else
        slice->PicOrderCntMsb = decoder->PrevPicOrderCntMsb;

      // 2nd
      if (slice->fieldPicFlag == 0) {
        // frame pixelPos
        slice->topPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
        slice->botPoc = slice->topPoc + slice->deletaPicOrderCountBot;
        slice->thisPoc = slice->framePoc = (slice->topPoc < slice->botPoc) ? slice->topPoc : slice->botPoc;
        }
      else if (!slice->botFieldFlag) // top field
        slice->thisPoc= slice->topPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
      else // bottom field
        slice->thisPoc= slice->botPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
      slice->framePoc = slice->thisPoc;

      decoder->thisPoc = slice->thisPoc;
      decoder->previousFrameNum = slice->frameNum;

      if (slice->refId) {
        decoder->PrevPicOrderCntLsb = slice->picOrderCountLsb;
        decoder->PrevPicOrderCntMsb = slice->PicOrderCntMsb;
        }

      break;
    //}}}
    //{{{
    case 1: // POC MODE 1
      // 1st
      if (slice->idrFlag) {
        // first pixelPos of IDRGOP,
        decoder->frameNumOffset = 0;
        if (slice->frameNum)
          error ("frameNum nonZero IDR picture");
        }
      else {
        if (decoder->lastHasMmco5) {
          decoder->previousFrameNumOffset = 0;
          decoder->previousFrameNum = 0;
          }
        if (slice->frameNum<decoder->previousFrameNum) // not first pixelPos of IDRGOP
          decoder->frameNumOffset = decoder->previousFrameNumOffset + decoder->coding.maxFrameNum;
        else
          decoder->frameNumOffset = decoder->previousFrameNumOffset;
        }

      // 2nd
      if (activeSPS->numRefFramesPocCycle)
        slice->AbsFrameNum = decoder->frameNumOffset + slice->frameNum;
      else
        slice->AbsFrameNum = 0;
      if (!slice->refId && (slice->AbsFrameNum > 0))
        slice->AbsFrameNum--;

      // 3rd
      decoder->expectedDeltaPerPOCcycle = 0;
      if (activeSPS->numRefFramesPocCycle)
        for (unsigned i = 0; i < activeSPS->numRefFramesPocCycle; i++)
          decoder->expectedDeltaPerPOCcycle += activeSPS->offset_for_ref_frame[i];

      if (slice->AbsFrameNum) {
        decoder->POCcycleCount = (slice->AbsFrameNum-1) / activeSPS->numRefFramesPocCycle;
        decoder->frameNumPOCcycle = (slice->AbsFrameNum-1) % activeSPS->numRefFramesPocCycle;
        decoder->expectedPOC =
          decoder->POCcycleCount*decoder->expectedDeltaPerPOCcycle;
        for (int i = 0; i <= decoder->frameNumPOCcycle; i++)
          decoder->expectedPOC += activeSPS->offset_for_ref_frame[i];
        }
      else
        decoder->expectedPOC = 0;

      if (!slice->refId)
        decoder->expectedPOC += activeSPS->offsetNonRefPic;

      if (slice->fieldPicFlag == 0) {
        // frame pixelPos
        slice->topPoc = decoder->expectedPOC + slice->deltaPicOrderCount[0];
        slice->botPoc = slice->topPoc + activeSPS->offsetTopBotField + slice->deltaPicOrderCount[1];
        slice->thisPoc = slice->framePoc = (slice->topPoc < slice->botPoc) ? slice->topPoc : slice->botPoc;
        }
      else if (!slice->botFieldFlag) // top field
        slice->thisPoc = slice->topPoc = decoder->expectedPOC + slice->deltaPicOrderCount[0];
      else // bottom field
        slice->thisPoc = slice->botPoc = decoder->expectedPOC + activeSPS->offsetTopBotField + slice->deltaPicOrderCount[0];
      slice->framePoc=slice->thisPoc;

      decoder->previousFrameNum = slice->frameNum;
      decoder->previousFrameNumOffset = decoder->frameNumOffset;
      break;
    //}}}
    //{{{
    case 2: // POC MODE 2
      if (slice->idrFlag) {
        // IDR picture, first pixelPos of IDRGOP,
        decoder->frameNumOffset = 0;
        slice->thisPoc = slice->framePoc = slice->topPoc = slice->botPoc = 0;
        if (slice->frameNum)
          error ("frameNum not equal to zero in IDR picture");
        }
      else {
        if (decoder->lastHasMmco5) {
          decoder->previousFrameNum = 0;
          decoder->previousFrameNumOffset = 0;
          }

        if (slice->frameNum<decoder->previousFrameNum)
          decoder->frameNumOffset = decoder->previousFrameNumOffset + decoder->coding.maxFrameNum;
        else
          decoder->frameNumOffset = decoder->previousFrameNumOffset;

        slice->AbsFrameNum = decoder->frameNumOffset+slice->frameNum;
        if (!slice->refId)
          slice->thisPoc = 2*slice->AbsFrameNum - 1;
        else
          slice->thisPoc = 2*slice->AbsFrameNum;

        if (slice->fieldPicFlag == 0)
          slice->topPoc = slice->botPoc = slice->framePoc = slice->thisPoc;
        else if (!slice->botFieldFlag)
          slice->topPoc = slice->framePoc = slice->thisPoc;
        else
          slice->botPoc = slice->framePoc = slice->thisPoc;
        }

      decoder->previousFrameNum = slice->frameNum;
      decoder->previousFrameNumOffset = decoder->frameNumOffset;
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
void initOldSlice (sOldSlice* oldSlice) {

  oldSlice->fieldPicFlag = 0;
  oldSlice->ppsId = INT_MAX;
  oldSlice->frameNum = INT_MAX;

  oldSlice->nalRefIdc = INT_MAX;
  oldSlice->idrFlag = FALSE;

  oldSlice->picOrderCountLsb = UINT_MAX;
  oldSlice->deltaPicOrderCountBot = INT_MAX;
  oldSlice->deltaPicOrderCount[0] = INT_MAX;
  oldSlice->deltaPicOrderCount[1] = INT_MAX;
  }
//}}}
//{{{
void padPicture (sDecoder* decoder, sPicture* picture) {

  padBuf (*picture->imgY, picture->sizeX, picture->sizeY,
           picture->iLumaStride, decoder->coding.iLumaPadX, decoder->coding.iLumaPadY);

  if (picture->chromaFormatIdc != YUV400) {
    padBuf (*picture->imgUV[0], picture->sizeXcr, picture->sizeYcr,
            picture->iChromaStride, decoder->coding.iChromaPadX, decoder->coding.iChromaPadY);
    padBuf (*picture->imgUV[1], picture->sizeXcr, picture->sizeYcr,
            picture->iChromaStride, decoder->coding.iChromaPadX, decoder->coding.iChromaPadY);
    }
  }
//}}}

//{{{
void endDecodeFrame (sDecoder* decoder) {

  // return if the last picture has already been finished
  if (!decoder->picture ||
      ((decoder->numDecodedMbs != decoder->picSizeInMbs) &&
       ((decoder->coding.yuvFormat != YUV444) || !decoder->coding.sepColourPlaneFlag)))
    return;

  //{{{  error conceal
  frame recfr;
  recfr.decoder = decoder;
  recfr.yptr = &(decoder->picture->imgY[0][0]);
  if (decoder->picture->chromaFormatIdc != YUV400) {
    recfr.uptr = &(decoder->picture->imgUV[0][0][0]);
    recfr.vptr = &(decoder->picture->imgUV[1][0][0]);
    }

  // this is always true at the beginning of a picture
  int ercSegment = 0;

  // mark the start of the first segment
  if (!decoder->picture->mbAffFrame) {
    int i;
    ercStartSegment (0, ercSegment, 0 , decoder->ercErrorVar);
    // generate the segments according to the macroblock map
    for (i = 1; i < (int)(decoder->picture->picSizeInMbs); ++i) {
      if (decoder->mbData[i].errorFlag != decoder->mbData[i-1].errorFlag) {
        ercStopSegment (i-1, ercSegment, 0, decoder->ercErrorVar); //! stop current segment

        // mark current segment as lost or OK
        if(decoder->mbData[i-1].errorFlag)
          ercMarksegmentLost (decoder->picture->sizeX, decoder->ercErrorVar);
        else
          ercMarksegmentOK (decoder->picture->sizeX, decoder->ercErrorVar);

        ++ercSegment;  //! next segment
        ercStartSegment (i, ercSegment, 0 , decoder->ercErrorVar); //! start new segment
        }
      }

    // mark end of the last segment
    ercStopSegment (decoder->picture->picSizeInMbs-1, ercSegment, 0, decoder->ercErrorVar);
    if (decoder->mbData[i-1].errorFlag)
      ercMarksegmentLost (decoder->picture->sizeX, decoder->ercErrorVar);
    else
      ercMarksegmentOK (decoder->picture->sizeX, decoder->ercErrorVar);

    // call the right error conceal function depending on the frame type.
    decoder->ercMvPerMb /= decoder->picture->picSizeInMbs;
    if (decoder->picture->sliceType == eIslice || decoder->picture->sliceType == eSIslice) // I-frame
      ercConcealIntraFrame (decoder, &recfr,
                            decoder->picture->sizeX, decoder->picture->sizeY, decoder->ercErrorVar);
    else
      ercConcealInterFrame (&recfr, decoder->ercObjectList,
                            decoder->picture->sizeX, decoder->picture->sizeY, decoder->ercErrorVar,
                            decoder->picture->chromaFormatIdc);
    }
  //}}}
  if (!decoder->deblockMode &&
      decoder->param.deblock &&
      (decoder->deblockEnable & (1 << decoder->picture->usedForReference))) {
    if (decoder->coding.sepColourPlaneFlag) {
      //{{{  deblockJV
      int colourPlaneId = decoder->sliceList[0]->colourPlaneId;
      for (int nplane = 0; nplane < MAX_PLANE; ++nplane) {
        decoder->sliceList[0]->colourPlaneId = nplane;
        changePlaneJV (decoder, nplane, NULL );
        deblockPicture (decoder, decoder->picture);
        }
      decoder->sliceList[0]->colourPlaneId = colourPlaneId;
      makeFramePictureJV (decoder);
      }
      //}}}
    else
      deblockPicture (decoder, decoder->picture);
    }
  else if (decoder->coding.sepColourPlaneFlag)
    makeFramePictureJV (decoder);

  if (decoder->picture->mbAffFrame)
    mbAffPostProc (decoder);
  if (decoder->coding.picStructure != eFrame)
     decoder->idrFrameNum /= 2;
  if (decoder->picture->usedForReference)
    padPicture (decoder, decoder->picture);

  int picStructure = decoder->picture->picStructure;
  int sliceType = decoder->picture->sliceType;
  int pocNum = decoder->picture->framePoc;
  int refpic = decoder->picture->usedForReference;
  int qp = decoder->picture->qp;
  int picNum = decoder->picture->picNum;
  int isIdr = decoder->picture->idrFlag;
  int chromaFormatIdc = decoder->picture->chromaFormatIdc;
  storePictureDpb (decoder->dpb, decoder->picture);
  decoder->picture = NULL;

  if (decoder->lastHasMmco5)
    decoder->preFrameNum = 0;

  if (picStructure == eTopField || picStructure == eFrame) {
    //{{{
    if (sliceType == eIslice && isIdr)
      strcpy (decoder->info.sliceStr, "IDR");

    else if (sliceType == eIslice)
      strcpy (decoder->info.sliceStr, " I ");

    else if (sliceType == ePslice)
      strcpy (decoder->info.sliceStr, " P ");

    else if (sliceType == eSPslice)
      strcpy (decoder->info.sliceStr, "SP ");

    else if  (sliceType == eSIslice)
      strcpy (decoder->info.sliceStr, "SI ");

    else if (refpic)
      strcpy (decoder->info.sliceStr, " B ");

    else
      strcpy (decoder->info.sliceStr, " b ");

    if (picStructure == eFrame)
      strncat (decoder->info.sliceStr, "    ", 8 - strlen (decoder->info.sliceStr));

    decoder->info.sliceStr[3] = 0;
    }
    //}}}
  else if (picStructure == eBotField) {
    //{{{
    if (sliceType == eIslice && isIdr)
      strncat (decoder->info.sliceStr, "|IDR", 8-strlen (decoder->info.sliceStr));

    else if (sliceType == eIslice)
      strncat (decoder->info.sliceStr, "| I ", 8-strlen (decoder->info.sliceStr));

    else if (sliceType == ePslice)
      strncat (decoder->info.sliceStr, "| P ", 8-strlen (decoder->info.sliceStr));

    else if (sliceType == eSPslice)
      strncat (decoder->info.sliceStr, "|SP ", 8-strlen (decoder->info.sliceStr));

    else if  (sliceType == eSIslice)
      strncat (decoder->info.sliceStr, "|SI ", 8-strlen (decoder->info.sliceStr));

    else if (refpic)
      strncat (decoder->info.sliceStr, "| B ", 8-strlen (decoder->info.sliceStr));

    else
      strncat (decoder->info.sliceStr, "| b ", 8-strlen (decoder->info.sliceStr));

    decoder->info.sliceStr[8] = 0;
    }
    //}}}
  if ((picStructure == eFrame) || picStructure == eBotField) {
    getTime (&decoder->info.endTime);
    decoder->info.took = (int)timeNorm (timeDiff (&decoder->info.startTime, &decoder->info.endTime));
    sprintf (decoder->info.tookStr, "%dms", decoder->info.took);
    if (decoder->param.outDebug) {
      //{{{  print outDebug
      printf ("-> %d %d:%d:%02d %s ->%s-> poc:%d pic:%d",
              decoder->decodeFrameNum,
              decoder->numDecodedSlices, decoder->numDecodedMbs, qp,
              decoder->info.tookStr,
              decoder->info.sliceStr,
              pocNum, picNum);

      // count numOutputFrames
      int numOutputFrames = 0;
      sDecodedPic* pic = decoder->decOutputPic;
      while (pic) {
        if (pic->valid)
          numOutputFrames++;
        pic = pic->next;
        }
      if (numOutputFrames)
        printf (" -> %d", numOutputFrames);

      printf ("\n");
      }
      //}}}

    // I or P pictures ?
    if ((sliceType == eIslice) || (sliceType == eSIslice) || (sliceType == ePslice) || refpic)
      ++(decoder->idrFrameNum);
    (decoder->decodeFrameNum)++;
    }
  }
//}}}
//{{{
int decodeFrame (sDecoder* decoder) {

  int ret = 0;

  decoder->numDecodedMbs = 0;
  decoder->picSliceIndex = 0;
  decoder->numDecodedSlices = 0;

  int curHeader = 0;
  if (decoder->newFrame) {
    if (decoder->nextPPS->valid) {
      setPPSbyId (decoder, decoder->nextPPS->ppsId, decoder->nextPPS);
      decoder->nextPPS->valid = 0;
      }

    // get firstSlice from sliceList;
    sSlice* slice = decoder->sliceList[decoder->picSliceIndex];
    decoder->sliceList[decoder->picSliceIndex] = decoder->nextSlice;
    decoder->nextSlice = slice;
    slice = decoder->sliceList[decoder->picSliceIndex];

    useParameterSet (decoder, slice);
    initPicture (decoder, slice);

    decoder->picSliceIndex++;
    curHeader = SOS;
    }

  while ((curHeader != SOP) && (curHeader != EOS)) {
    //{{{  no pending slices
    if (!decoder->sliceList[decoder->picSliceIndex])
      decoder->sliceList[decoder->picSliceIndex] = allocSlice (decoder);

    sSlice* slice = decoder->sliceList[decoder->picSliceIndex];
    slice->decoder = decoder;
    slice->dpb = decoder->dpb; //set default value;
    slice->nextHeader = -8888;
    slice->numDecodedMbs = 0;
    slice->coefCount = -1;
    slice->pos = 0;
    slice->isResetCoef = FALSE;
    slice->isResetCoefCr = FALSE;

    curHeader = readNextSlice (slice);
    slice->curHeader = curHeader;
    //{{{  manage primary, redundant slices
    if (!slice->redundantPicCount)
      decoder->isPrimaryOk = decoder->isRedundantOk = 1;

    if (!slice->redundantPicCount && (decoder->coding.type != eIslice)) {
      for (int i = 0; i < slice->numRefIndexActive[LIST_0];++i)
        if (!slice->refFlag[i]) // reference primary slice incorrect, primary slice incorrect
          decoder->isPrimaryOk = 0;
      }
    else if (slice->redundantPicCount && (decoder->coding.type != eIslice)) // reference redundant slice incorrect
      if (!slice->refFlag[slice->redundantSliceRefIndex]) // redundant slice is incorrect
        decoder->isRedundantOk = 0;

    // If primary and redundant received, primary is correct
    //   discard redundant
    // else
    //   primary slice replaced with redundant slice.
    if ((slice->frameNum == decoder->prevFrameNum) &&
        slice->redundantPicCount && decoder->isPrimaryOk && (curHeader != EOS))
      continue;

    if (((curHeader != SOP) && (curHeader != EOS)) ||
        ((curHeader == SOP) && !decoder->picSliceIndex)) {
       slice->curSliceIndex = (short)decoder->picSliceIndex;
       decoder->picture->maxSliceId = (short)imax (slice->curSliceIndex, decoder->picture->maxSliceId);
       if (decoder->picSliceIndex > 0) {
         copyPOC (*(decoder->sliceList), slice);
         decoder->sliceList[decoder->picSliceIndex-1]->endMbNumPlus1 = slice->startMbNum;
         }

       decoder->picSliceIndex++;
       if (decoder->picSliceIndex >= decoder->numAllocatedSlices)
         error ("decodeFrame - sliceList numAllocationSlices too small");
      curHeader = SOS;
      }

    else {
      if (decoder->sliceList[decoder->picSliceIndex-1]->mbAffFrame)
        decoder->sliceList[decoder->picSliceIndex-1]->endMbNumPlus1 = decoder->coding.frameSizeMbs / 2;
      else
        decoder->sliceList[decoder->picSliceIndex-1]->endMbNumPlus1 =
          decoder->coding.frameSizeMbs / (decoder->sliceList[decoder->picSliceIndex-1]->fieldPicFlag + 1);

      decoder->newFrame = 1;

      slice->curSliceIndex = 0;
      decoder->sliceList[decoder->picSliceIndex] = decoder->nextSlice;
      decoder->nextSlice = slice;
      }
    //}}}

    copySliceInfo (slice, decoder->oldSlice);
    }
    //}}}

  // decode slices
  ret = curHeader;
  initPictureDecoding (decoder);
  for (int sliceIndex = 0; sliceIndex < decoder->picSliceIndex; sliceIndex++) {
    sSlice* slice = decoder->sliceList[sliceIndex];
    curHeader = slice->curHeader;
    initSlice (decoder, slice);

    if (slice->activePPS->entropyCodingMode) {
      //{{{  init cabac
      initContexts (slice);
      cabacNewSlice (slice);
      }
      //}}}

    if (((slice->activePPS->weightedBiPredIdc > 0) && (slice->sliceType == eBslice)) ||
        (slice->activePPS->weightedPredFlag && (slice->sliceType != eIslice)))
      fillWpParam (slice);

    if (((curHeader == SOP) || (curHeader == SOS)) && !slice->errorFlag)
      decodeSlice (slice);

    decoder->ercMvPerMb += slice->ercMvPerMb;
    decoder->numDecodedMbs += slice->numDecodedMbs;
    decoder->numDecodedSlices++;
    }

  endDecodeFrame (decoder);
  decoder->prevFrameNum = decoder->sliceList[0]->frameNum;

  return ret;
  }
//}}}
