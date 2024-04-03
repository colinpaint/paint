//{{{  includes
#include "global.h"
#include "memory.h"

#include "syntaxElement.h"
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
#include "binaryArithmeticDecode.h"
#include "cabac.h"
#include "vlc.h"
#include "quant.h"
#include "errorConceal.h"
#include "erc.h"
#include "buffer.h"
#include "mcPred.h"

#include <math.h>
#include <limits.h>

// utils
#include "../common/basicTypes.h"
#include "../common/cLog.h"
//}}}
//{{{  static const tables
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
void padPicture (sDecoder* decoder, sPicture* picture) {

  padBuf (*picture->imgY, picture->sizeX, picture->sizeY,
           picture->lumaStride, decoder->coding.lumaPadX, decoder->coding.lumaPadY);

  if (picture->chromaFormatIdc != YUV400) {
    padBuf (*picture->imgUV[0], picture->sizeXcr, picture->sizeYcr,
            picture->chromaStride, decoder->coding.chromaPadX, decoder->coding.chromaPadY);
    padBuf (*picture->imgUV[1], picture->sizeXcr, picture->sizeYcr,
            picture->chromaStride, decoder->coding.chromaPadX, decoder->coding.chromaPadY);
    }
  }
//}}}

//{{{
static void copyPoc (sSlice* fromSlice, sSlice* toSlice) {

  toSlice->topPoc = fromSlice->topPoc;
  toSlice->botPoc = fromSlice->botPoc;
  toSlice->thisPoc = fromSlice->thisPoc;
  toSlice->framePoc = fromSlice->framePoc;
  }
//}}}
//{{{
void decodePOC (sDecoder* decoder, sSlice* slice) {

  sSps* activeSps = decoder->activeSps;
  uint32_t maxPicOrderCntLsb = (1<<(activeSps->log2maxPocLsbMinus4+4));

  switch (activeSps->pocType) {
    //{{{
    case 0: // POC MODE 0
      // 1st
      if (slice->isIDR) {
        decoder->prevPocMsb = 0;
        decoder->prevPocLsb = 0;
        }
      else if (decoder->lastHasMmco5) {
        if (decoder->lastPicBotField) {
          decoder->prevPocMsb = 0;
          decoder->prevPocLsb = 0;
          }
        else {
          decoder->prevPocMsb = 0;
          decoder->prevPocLsb = slice->topPoc;
          }
        }

      // Calculate the MSBs of current picture
      if (slice->picOrderCountLsb < decoder->prevPocLsb  &&
          (decoder->prevPocLsb - slice->picOrderCountLsb) >= maxPicOrderCntLsb/2)
        slice->PicOrderCntMsb = decoder->prevPocMsb + maxPicOrderCntLsb;
      else if (slice->picOrderCountLsb > decoder->prevPocLsb &&
               (slice->picOrderCountLsb - decoder->prevPocLsb) > maxPicOrderCntLsb/2)
        slice->PicOrderCntMsb = decoder->prevPocMsb - maxPicOrderCntLsb;
      else
        slice->PicOrderCntMsb = decoder->prevPocMsb;

      // 2nd
      if (slice->fieldPic == 0) {
        // frame pixelPos
        slice->topPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
        slice->botPoc = slice->topPoc + slice->deletaPicOrderCountBot;
        slice->thisPoc = slice->framePoc = (slice->topPoc < slice->botPoc) ? slice->topPoc : slice->botPoc;
        }
      else if (!slice->botField) // top field
        slice->thisPoc= slice->topPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
      else // bottom field
        slice->thisPoc= slice->botPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
      slice->framePoc = slice->thisPoc;

      decoder->thisPoc = slice->thisPoc;
      decoder->previousFrameNum = slice->frameNum;

      if (slice->refId) {
        decoder->prevPocLsb = slice->picOrderCountLsb;
        decoder->prevPocMsb = slice->PicOrderCntMsb;
        }

      break;
    //}}}
    //{{{
    case 1: // POC MODE 1
      // 1st
      if (slice->isIDR) {
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
      if (activeSps->numRefFramesPocCycle)
        slice->AbsFrameNum = decoder->frameNumOffset + slice->frameNum;
      else
        slice->AbsFrameNum = 0;
      if (!slice->refId && (slice->AbsFrameNum > 0))
        slice->AbsFrameNum--;

      // 3rd
      decoder->expectedDeltaPerPocCycle = 0;
      if (activeSps->numRefFramesPocCycle)
        for (uint32_t i = 0; i < activeSps->numRefFramesPocCycle; i++)
          decoder->expectedDeltaPerPocCycle += activeSps->offsetForRefFrame[i];

      if (slice->AbsFrameNum) {
        decoder->pocCycleCount = (slice->AbsFrameNum-1) / activeSps->numRefFramesPocCycle;
        decoder->frameNumPocCycle = (slice->AbsFrameNum-1) % activeSps->numRefFramesPocCycle;
        decoder->expectedPOC =
          decoder->pocCycleCount*decoder->expectedDeltaPerPocCycle;
        for (int i = 0; i <= decoder->frameNumPocCycle; i++)
          decoder->expectedPOC += activeSps->offsetForRefFrame[i];
        }
      else
        decoder->expectedPOC = 0;

      if (!slice->refId)
        decoder->expectedPOC += activeSps->offsetNonRefPic;

      if (slice->fieldPic == 0) {
        // frame pixelPos
        slice->topPoc = decoder->expectedPOC + slice->deltaPicOrderCount[0];
        slice->botPoc = slice->topPoc + activeSps->offsetTopBotField + slice->deltaPicOrderCount[1];
        slice->thisPoc = slice->framePoc = (slice->topPoc < slice->botPoc) ? slice->topPoc : slice->botPoc;
        }
      else if (!slice->botField) // top field
        slice->thisPoc = slice->topPoc = decoder->expectedPOC + slice->deltaPicOrderCount[0];
      else // bottom field
        slice->thisPoc = slice->botPoc = decoder->expectedPOC + activeSps->offsetTopBotField + slice->deltaPicOrderCount[0];
      slice->framePoc=slice->thisPoc;

      decoder->previousFrameNum = slice->frameNum;
      decoder->previousFrameNumOffset = decoder->frameNumOffset;
      break;
    //}}}
    //{{{
    case 2: // POC MODE 2
      if (slice->isIDR) {
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

        if (slice->fieldPic == 0)
          slice->topPoc = slice->botPoc = slice->framePoc = slice->thisPoc;
        else if (!slice->botField)
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
static void copySliceInfo (sSlice* slice, sOldSlice* oldSlice) {

  oldSlice->ppsId = slice->ppsId;
  oldSlice->frameNum = slice->frameNum;
  oldSlice->fieldPic = slice->fieldPic;

  if (slice->fieldPic)
    oldSlice->botField = slice->botField;

  oldSlice->nalRefIdc = slice->refId;

  oldSlice->isIDR = (uint8_t)slice->isIDR;
  if (slice->isIDR)
    oldSlice->idrPicId = slice->idrPicId;

  if (slice->decoder->activeSps->pocType == 0) {
    oldSlice->picOrderCountLsb = slice->picOrderCountLsb;
    oldSlice->deltaPicOrderCountBot = slice->deletaPicOrderCountBot;
    }
  else if (slice->decoder->activeSps->pocType == 1) {
    oldSlice->deltaPicOrderCount[0] = slice->deltaPicOrderCount[0];
    oldSlice->deltaPicOrderCount[1] = slice->deltaPicOrderCount[1];
    }
  }
//}}}
//{{{
void initOldSlice (sOldSlice* oldSlice) {

  oldSlice->fieldPic = 0;
  oldSlice->ppsId = INT_MAX;
  oldSlice->frameNum = INT_MAX;

  oldSlice->nalRefIdc = INT_MAX;
  oldSlice->isIDR = false;

  oldSlice->picOrderCountLsb = UINT_MAX;
  oldSlice->deltaPicOrderCountBot = INT_MAX;
  oldSlice->deltaPicOrderCount[0] = INT_MAX;
  oldSlice->deltaPicOrderCount[1] = INT_MAX;
  }
//}}}

//{{{
static void initCabacContexts (sSlice* slice) {

  //{{{
  #define IBIARI_CTX_INIT2(ii,jj,ctx,tab,num, qp) { \
    for (i = 0; i < ii; ++i) \
      for (j = 0; j < jj; ++j) \
        binaryArithmeticInitContext (qp, &(ctx[i][j]), tab ## _I[num][i][j]); \
    }
  //}}}
  //{{{
  #define PBIARI_CTX_INIT2(ii,jj,ctx,tab,num, qp) { \
    for (i = 0; i < ii; ++i) \
      for (j = 0; j < jj; ++j) \
        binaryArithmeticInitContext (qp, &(ctx[i][j]), tab ## _P[num][i][j]); \
    }
  //}}}
  //{{{
  #define IBIARI_CTX_INIT1(jj,ctx,tab,num, qp) { \
    for (j = 0; j < jj; ++j) \
      binaryArithmeticInitContext (qp, &(ctx[j]), tab ## _I[num][0][j]); \
    }
  //}}}
  //{{{
  #define PBIARI_CTX_INIT1(jj,ctx,tab,num, qp) { \
    for (j = 0; j < jj; ++j) \
      binaryArithmeticInitContext (qp, &(ctx[j]), tab ## _P[num][0][j]); \
    }
  //}}}

  sMotionContexts* mc = slice->motionInfoContexts;
  sTextureContexts* tc = slice->textureInfoContexts;

  int i, j;
  int qp = imax (0, slice->qp);
  int cabacInitIdc = slice->cabacInitIdc;

  // motion coding contexts
  if ((slice->sliceType == eSliceI) || (slice->sliceType == eSliceSI)) {
    IBIARI_CTX_INIT2 (3, NUM_MB_TYPE_CTX,   mc->mbTypeContexts,     INIT_MB_TYPE,    cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (2, NUM_B8_TYPE_CTX,   mc->b8TypeContexts,     INIT_B8_TYPE,    cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (2, NUM_MV_RES_CTX,    mc->mvResContexts,      INIT_MV_RES,     cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (2, NUM_REF_NO_CTX,    mc->refNoContexts,      INIT_REF_NO,     cabacInitIdc, qp);
    IBIARI_CTX_INIT1 (   NUM_DELTA_QP_CTX,  mc->deltaQpContexts,    INIT_DELTA_QP,   cabacInitIdc, qp);
    IBIARI_CTX_INIT1 (   NUM_MB_AFF_CTX,    mc->mbAffContexts,      INIT_MB_AFF,     cabacInitIdc, qp);

    // texture coding contexts
    IBIARI_CTX_INIT1 (   NUM_TRANSFORM_SIZE_CTX, tc->transformSizeContexts, INIT_TRANSFORM_SIZE, cabacInitIdc, qp);
    IBIARI_CTX_INIT1 (                 NUM_IPR_CTX,  tc->iprContexts,     INIT_IPR,       cabacInitIdc, qp);
    IBIARI_CTX_INIT1 (                 NUM_CIPR_CTX, tc->ciprContexts,    INIT_CIPR,      cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (3,               NUM_CBP_CTX,  tc->cbpContexts,     INIT_CBP,       cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_BCBP_CTX, tc->bcbpContexts,    INIT_BCBP,      cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->mapContexts[0],  INIT_MAP,       cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->mapContexts[1],  INIT_FLD_MAP,   cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->lastContexts[1], INIT_FLD_LAST,  cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->lastContexts[0], INIT_LAST,      cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ONE_CTX,  tc->oneContexts,     INIT_ONE,       cabacInitIdc, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ABS_CTX,  tc->absContexts,     INIT_ABS,       cabacInitIdc, qp);
    }
  else {
    PBIARI_CTX_INIT2 (3, NUM_MB_TYPE_CTX,   mc->mbTypeContexts,     INIT_MB_TYPE,    cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (2, NUM_B8_TYPE_CTX,   mc->b8TypeContexts,     INIT_B8_TYPE,    cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (2, NUM_MV_RES_CTX,    mc->mvResContexts,      INIT_MV_RES,     cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (2, NUM_REF_NO_CTX,    mc->refNoContexts,      INIT_REF_NO,     cabacInitIdc, qp);
    PBIARI_CTX_INIT1 (   NUM_DELTA_QP_CTX,  mc->deltaQpContexts,    INIT_DELTA_QP,   cabacInitIdc, qp);
    PBIARI_CTX_INIT1 (   NUM_MB_AFF_CTX,    mc->mbAffContexts,      INIT_MB_AFF,     cabacInitIdc, qp);

    // texture coding contexts
    PBIARI_CTX_INIT1 (   NUM_TRANSFORM_SIZE_CTX, tc->transformSizeContexts, INIT_TRANSFORM_SIZE, cabacInitIdc, qp);
    PBIARI_CTX_INIT1 (                 NUM_IPR_CTX,  tc->iprContexts,     INIT_IPR,       cabacInitIdc, qp);
    PBIARI_CTX_INIT1 (                 NUM_CIPR_CTX, tc->ciprContexts,    INIT_CIPR,      cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (3,               NUM_CBP_CTX,  tc->cbpContexts,     INIT_CBP,       cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_BCBP_CTX, tc->bcbpContexts,    INIT_BCBP,      cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->mapContexts[0],  INIT_MAP,       cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->mapContexts[1],  INIT_FLD_MAP,   cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->lastContexts[1], INIT_FLD_LAST,  cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->lastContexts[0], INIT_LAST,      cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ONE_CTX,  tc->oneContexts,     INIT_ONE,       cabacInitIdc, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ABS_CTX,  tc->absContexts,     INIT_ABS,       cabacInitIdc, qp);
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
static void ercWriteMbModeMv (sMacroBlock* mb) {

  sDecoder* decoder = mb->decoder;
  int curMbNum = mb->mbIndexX;
  sPicture* picture = decoder->picture;

  int mbx = xPosMB (curMbNum, picture->sizeX), mby = yPosMB (curMbNum, picture->sizeX);
  sObjectBuffer* curRegion = decoder->ercObjectList + (curMbNum << 2);

  if (decoder->coding.sliceType != eSliceB) {
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
static void fillWeightedPredParam (sSlice* slice) {

  if (slice->sliceType == eSliceB) {
    int maxL0Ref = slice->numRefIndexActive[LIST_0];
    int maxL1Ref = slice->numRefIndexActive[LIST_1];
    if (slice->activePps->weightedBiPredIdc == 2) {
      slice->lumaLog2weightDenom = 5;
      slice->chromaLog2weightDenom = 5;
      slice->wpRoundLuma = 16;
      slice->wpRoundChroma = 16;
      for (int i = 0; i < MAX_REFERENCE_PICTURES; ++i)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
          slice->weightedPredWeight[0][i][comp] = 1 << logWeightDenom;
          slice->weightedPredWeight[1][i][comp] = 1 << logWeightDenom;
          slice->weightedPredOffset[0][i][comp] = 0;
          slice->weightedPredOffset[1][i][comp] = 0;
          }
      }

    for (int i = 0; i < maxL0Ref; ++i)
      for (int j = 0; j < maxL1Ref; ++j)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
          if (slice->activePps->weightedBiPredIdc == 1) {
            slice->weightedBiPredWeight[0][i][j][comp] = slice->weightedPredWeight[0][i][comp];
            slice->weightedBiPredWeight[1][i][j][comp] = slice->weightedPredWeight[1][j][comp];
            }
          else if (slice->activePps->weightedBiPredIdc == 2) {
            int td = iClip3(-128,127,slice->listX[LIST_1][j]->poc - slice->listX[LIST_0][i]->poc);
            if (!td || slice->listX[LIST_1][j]->isLongTerm || slice->listX[LIST_0][i]->isLongTerm) {
              slice->weightedBiPredWeight[0][i][j][comp] = 32;
              slice->weightedBiPredWeight[1][i][j][comp] = 32;
              }
            else {
              int tb = iClip3(-128, 127, slice->thisPoc - slice->listX[LIST_0][i]->poc);
              int tx = (16384 + iabs (td / 2)) / td;
              int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
              slice->weightedBiPredWeight[1][i][j][comp] = distScaleFactor >> 2;
              slice->weightedBiPredWeight[0][i][j][comp] = 64 - slice->weightedBiPredWeight[1][i][j][comp];
              if (slice->weightedBiPredWeight[1][i][j][comp] < -64 || slice->weightedBiPredWeight[1][i][j][comp] > 128) {
                slice->weightedBiPredWeight[0][i][j][comp] = 32;
                slice->weightedBiPredWeight[1][i][j][comp] = 32;
                slice->weightedPredOffset[0][i][comp] = 0;
                slice->weightedPredOffset[1][j][comp] = 0;
                }
              }
            }
          }

    if (slice->mbAffFrame)
      for (int i = 0; i < 2 * maxL0Ref; ++i)
        for (int j = 0; j < 2 * maxL1Ref; ++j)
          for (int comp = 0; comp<3; ++comp)
            for (int k = 2; k < 6; k += 2) {
              slice->weightedPredOffset[k+0][i][comp] = slice->weightedPredOffset[0][i>>1][comp];
              slice->weightedPredOffset[k+1][j][comp] = slice->weightedPredOffset[1][j>>1][comp];
              int logWeightDenom = !comp ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
              if (slice->activePps->weightedBiPredIdc == 1) {
                slice->weightedBiPredWeight[k+0][i][j][comp] = slice->weightedPredWeight[0][i>>1][comp];
                slice->weightedBiPredWeight[k+1][i][j][comp] = slice->weightedPredWeight[1][j>>1][comp];
                }
              else if (slice->activePps->weightedBiPredIdc == 2) {
                int td = iClip3 (-128, 127, slice->listX[k+LIST_1][j]->poc - slice->listX[k+LIST_0][i]->poc);
                if (!td || slice->listX[k+LIST_1][j]->isLongTerm || slice->listX[k+LIST_0][i]->isLongTerm) {
                  slice->weightedBiPredWeight[k+0][i][j][comp] = 32;
                  slice->weightedBiPredWeight[k+1][i][j][comp] = 32;
                  }
                else {
                  int tb = iClip3 (-128,127,
                               ((k == 2) ? slice->topPoc : slice->botPoc) - slice->listX[k+LIST_0][i]->poc);
                  int tx = (16384 + iabs(td/2)) / td;
                  int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
                  slice->weightedBiPredWeight[k+1][i][j][comp] = distScaleFactor >> 2;
                  slice->weightedBiPredWeight[k+0][i][j][comp] = 64 - slice->weightedBiPredWeight[k+1][i][j][comp];
                  if (slice->weightedBiPredWeight[k+1][i][j][comp] < -64 ||
                      slice->weightedBiPredWeight[k+1][i][j][comp] > 128) {
                    slice->weightedBiPredWeight[k+1][i][j][comp] = 32;
                    slice->weightedBiPredWeight[k+0][i][j][comp] = 32;
                    slice->weightedPredOffset[k+0][i][comp] = 0;
                    slice->weightedPredOffset[k+1][j][comp] = 0;
                    }
                  }
                }
              }
    }
  }
//}}}
//{{{
static void resetWeightedPredParam (sSlice* slice) {

  for (int i = 0; i < MAX_REFERENCE_PICTURES; i++) {
    for (int comp = 0; comp < 3; comp++) {
      int logWeightDenom = (comp == 0) ? slice->lumaLog2weightDenom : slice->chromaLog2weightDenom;
      slice->weightedPredWeight[0][i][comp] = 1 << logWeightDenom;
      slice->weightedPredWeight[1][i][comp] = 1 << logWeightDenom;
      }
    }
  }
//}}}

//{{{
static void setCoding (sDecoder* decoder) {

  decoder->widthCr = 0;
  decoder->heightCr = 0;
  decoder->bitDepthLuma = decoder->coding.bitDepthLuma;
  decoder->bitDepthChroma = decoder->coding.bitDepthChroma;

  decoder->coding.lumaPadX = MCBUF_LUMA_PAD_X;
  decoder->coding.lumaPadY = MCBUF_LUMA_PAD_Y;
  decoder->coding.chromaPadX = MCBUF_CHROMA_PAD_X;
  decoder->coding.chromaPadY = MCBUF_CHROMA_PAD_Y;

  if (decoder->coding.yuvFormat == YUV420) {
    //{{{  yuv420
    decoder->widthCr = decoder->coding.width >> 1;
    decoder->heightCr = decoder->coding.height >> 1;
    }
    //}}}
  else if (decoder->coding.yuvFormat == YUV422) {
    //{{{  yuv422
    decoder->widthCr = decoder->coding.width >> 1;
    decoder->heightCr = decoder->coding.height;
    decoder->coding.chromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
    //}}}
  else if (decoder->coding.yuvFormat == YUV444) {
    //{{{  yuv444
    decoder->widthCr = decoder->coding.width;
    decoder->heightCr = decoder->coding.height;
    decoder->coding.chromaPadX = decoder->coding.lumaPadX;
    decoder->coding.chromaPadY = decoder->coding.lumaPadY;
    }
    //}}}

  // pel bitDepth init
  decoder->coding.bitDepthLumaQpScale = 6 * (decoder->bitDepthLuma - 8);

  if ((decoder->bitDepthLuma > decoder->bitDepthChroma) ||
      (decoder->activeSps->chromaFormatIdc == YUV400))
    decoder->coding.picUnitBitSizeDisk = (decoder->bitDepthLuma > 8)? 16:8;
  else
    decoder->coding.picUnitBitSizeDisk = (decoder->bitDepthChroma > 8)? 16:8;

  decoder->coding.dcPredValueComp[0] = 1<<(decoder->bitDepthLuma - 1);
  decoder->coding.maxPelValueComp[0] = (1<<decoder->bitDepthLuma) - 1;
  decoder->mbSize[0][0] = decoder->mbSize[0][1] = MB_BLOCK_SIZE;

  if (decoder->activeSps->chromaFormatIdc == YUV400) {
    //{{{  yuv400
    decoder->coding.bitDepthChromaQpScale = 0;

    decoder->coding.maxPelValueComp[1] = 0;
    decoder->coding.maxPelValueComp[2] = 0;

    decoder->coding.numBlock8x8uv = 0;
    decoder->coding.numUvBlocks = 0;
    decoder->coding.numCdcCoeff = 0;

    decoder->mbSize[1][0] = decoder->mbSize[2][0] = decoder->mbCrSizeX  = 0;
    decoder->mbSize[1][1] = decoder->mbSize[2][1] = decoder->mbCrSizeY  = 0;

    decoder->coding.subpelX = 0;
    decoder->coding.subpelY = 0;
    decoder->coding.shiftpelX = 0;
    decoder->coding.shiftpelY = 0;

    decoder->coding.totalScale = 0;
    }
    //}}}
  else {
    //{{{  not yuv400
    decoder->coding.bitDepthChromaQpScale = 6 * (decoder->bitDepthChroma - 8);

    decoder->coding.dcPredValueComp[1] = 1 << (decoder->bitDepthChroma - 1);
    decoder->coding.dcPredValueComp[2] = decoder->coding.dcPredValueComp[1];
    decoder->coding.maxPelValueComp[1] = (1 << decoder->bitDepthChroma) - 1;
    decoder->coding.maxPelValueComp[2] = (1 << decoder->bitDepthChroma) - 1;

    decoder->coding.numBlock8x8uv = (1 << decoder->activeSps->chromaFormatIdc) & (~(0x1));
    decoder->coding.numUvBlocks = decoder->coding.numBlock8x8uv >> 1;
    decoder->coding.numCdcCoeff = decoder->coding.numBlock8x8uv << 1;

    decoder->mbSize[1][0] = decoder->mbSize[2][0] =
      decoder->mbCrSizeX = (decoder->activeSps->chromaFormatIdc == YUV420 ||
                            decoder->activeSps->chromaFormatIdc == YUV422) ? 8 : 16;
    decoder->mbSize[1][1] = decoder->mbSize[2][1] =
      decoder->mbCrSizeY = (decoder->activeSps->chromaFormatIdc == YUV444 ||
                            decoder->activeSps->chromaFormatIdc == YUV422) ? 16 : 8;

    decoder->coding.subpelX = decoder->mbCrSizeX == 8 ? 7 : 3;
    decoder->coding.subpelY = decoder->mbCrSizeY == 8 ? 7 : 3;
    decoder->coding.shiftpelX = decoder->mbCrSizeX == 8 ? 3 : 2;
    decoder->coding.shiftpelY = decoder->mbCrSizeY == 8 ? 3 : 2;

    decoder->coding.totalScale = decoder->coding.shiftpelX + decoder->coding.shiftpelY;
    }
    //}}}

  decoder->mbCrSize = decoder->mbCrSizeX * decoder->mbCrSizeY;
  decoder->mbSizeBlock[0][0] = decoder->mbSizeBlock[0][1] = decoder->mbSize[0][0] >> 2;
  decoder->mbSizeBlock[1][0] = decoder->mbSizeBlock[2][0] = decoder->mbSize[1][0] >> 2;
  decoder->mbSizeBlock[1][1] = decoder->mbSizeBlock[2][1] = decoder->mbSize[1][1] >> 2;

  decoder->mbSizeShift[0][0] = decoder->mbSizeShift[0][1] = ceilLog2sf (decoder->mbSize[0][0]);
  decoder->mbSizeShift[1][0] = decoder->mbSizeShift[2][0] = ceilLog2sf (decoder->mbSize[1][0]);
  decoder->mbSizeShift[1][1] = decoder->mbSizeShift[2][1] = ceilLog2sf (decoder->mbSize[1][1]);
  }
//}}}
//{{{
static void setCodingParam (sDecoder* decoder, sSps* sps) {

  // maximum vertical motion vector range in luma quarter pixel units
  decoder->coding.profileIdc = sps->profileIdc;
  decoder->coding.useLosslessQpPrime = sps->useLosslessQpPrime;

  if (sps->levelIdc <= 10)
    decoder->coding.maxVmvR = 64 * 4;
  else if (sps->levelIdc <= 20)
    decoder->coding.maxVmvR = 128 * 4;
  else if (sps->levelIdc <= 30)
    decoder->coding.maxVmvR = 256 * 4;
  else
    decoder->coding.maxVmvR = 512 * 4; // 512 pixels in quarter pixels

  // Fidelity Range Extensions stuff (part 1)
  decoder->coding.widthCr = 0;
  decoder->coding.heightCr = 0;
  decoder->coding.bitDepthLuma = (int16_t)(sps->bit_depth_luma_minus8 + 8);
  decoder->coding.bitDepthScale[0] = 1 << sps->bit_depth_luma_minus8;
  decoder->coding.bitDepthChroma = 0;
  if (sps->chromaFormatIdc != YUV400) {
    decoder->coding.bitDepthChroma = (int16_t)(sps->bit_depth_chroma_minus8 + 8);
    decoder->coding.bitDepthScale[1] = 1 << sps->bit_depth_chroma_minus8;
    }

  decoder->coding.maxFrameNum = 1 << (sps->log2maxFrameNumMinus4+4);
  decoder->coding.picWidthMbs = sps->picWidthMbsMinus1 + 1;
  decoder->coding.picHeightMapUnits = sps->picHeightMapUnitsMinus1 + 1;
  decoder->coding.frameHeightMbs = (2 - sps->frameMbOnly) * decoder->coding.picHeightMapUnits;
  decoder->coding.frameSizeMbs = decoder->coding.picWidthMbs * decoder->coding.frameHeightMbs;

  decoder->coding.yuvFormat = sps->chromaFormatIdc;
  decoder->coding.isSeperateColourPlane = sps->isSeperateColourPlane;

  decoder->coding.width = decoder->coding.picWidthMbs * MB_BLOCK_SIZE;
  decoder->coding.height = decoder->coding.frameHeightMbs * MB_BLOCK_SIZE;

  decoder->coding.lumaPadX = MCBUF_LUMA_PAD_X;
  decoder->coding.lumaPadY = MCBUF_LUMA_PAD_Y;
  decoder->coding.chromaPadX = MCBUF_CHROMA_PAD_X;
  decoder->coding.chromaPadY = MCBUF_CHROMA_PAD_Y;

  if (sps->chromaFormatIdc == YUV420) {
    decoder->coding.widthCr  = (decoder->coding.width  >> 1);
    decoder->coding.heightCr = (decoder->coding.height >> 1);
    }
  else if (sps->chromaFormatIdc == YUV422) {
    decoder->coding.widthCr  = (decoder->coding.width >> 1);
    decoder->coding.heightCr = decoder->coding.height;
    decoder->coding.chromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (sps->chromaFormatIdc == YUV444) {
    decoder->coding.widthCr = decoder->coding.width;
    decoder->coding.heightCr = decoder->coding.height;
    decoder->coding.chromaPadX = decoder->coding.lumaPadX;
    decoder->coding.chromaPadY = decoder->coding.lumaPadY;
    }

  // pel bitDepth init
  decoder->coding.bitDepthLumaQpScale = 6 * (decoder->coding.bitDepthLuma - 8);

  if (decoder->coding.bitDepthLuma > decoder->coding.bitDepthChroma || sps->chromaFormatIdc == YUV400)
    decoder->coding.picUnitBitSizeDisk = (decoder->coding.bitDepthLuma > 8)? 16:8;
  else
    decoder->coding.picUnitBitSizeDisk = (decoder->coding.bitDepthChroma > 8)? 16:8;
  decoder->coding.dcPredValueComp[0] = 1 << (decoder->coding.bitDepthLuma - 1);
  decoder->coding.maxPelValueComp[0] = (1 << decoder->coding.bitDepthLuma) - 1;
  decoder->coding.mbSize[0][0] = decoder->coding.mbSize[0][1] = MB_BLOCK_SIZE;

  if (sps->chromaFormatIdc == YUV400) {
    //{{{  YUV400
    decoder->coding.bitDepthChromaQpScale = 0;
    decoder->coding.maxPelValueComp[1] = 0;
    decoder->coding.maxPelValueComp[2] = 0;
    decoder->coding.numBlock8x8uv = 0;
    decoder->coding.numUvBlocks = 0;
    decoder->coding.numCdcCoeff = 0;
    decoder->coding.mbSize[1][0] = decoder->coding.mbSize[2][0] = decoder->coding.mbCrSizeX  = 0;
    decoder->coding.mbSize[1][1] = decoder->coding.mbSize[2][1] = decoder->coding.mbCrSizeY  = 0;
    decoder->coding.subpelX = 0;
    decoder->coding.subpelY = 0;
    decoder->coding.shiftpelX = 0;
    decoder->coding.shiftpelY = 0;
    decoder->coding.totalScale = 0;
    }
    //}}}
  else {
    //{{{  not YUV400
    decoder->coding.bitDepthChromaQpScale = 6 * (decoder->coding.bitDepthChroma - 8);
    decoder->coding.dcPredValueComp[1] = (1 << (decoder->coding.bitDepthChroma - 1));
    decoder->coding.dcPredValueComp[2] = decoder->coding.dcPredValueComp[1];
    decoder->coding.maxPelValueComp[1] = (1 << decoder->coding.bitDepthChroma) - 1;
    decoder->coding.maxPelValueComp[2] = (1 << decoder->coding.bitDepthChroma) - 1;
    decoder->coding.numBlock8x8uv = (1 << sps->chromaFormatIdc) & (~(0x1));
    decoder->coding.numUvBlocks = (decoder->coding.numBlock8x8uv >> 1);
    decoder->coding.numCdcCoeff = (decoder->coding.numBlock8x8uv << 1);
    decoder->coding.mbSize[1][0] = decoder->coding.mbSize[2][0] = decoder->coding.mbCrSizeX  = (sps->chromaFormatIdc==YUV420 || sps->chromaFormatIdc==YUV422)?  8 : 16;
    decoder->coding.mbSize[1][1] = decoder->coding.mbSize[2][1] = decoder->coding.mbCrSizeY  = (sps->chromaFormatIdc==YUV444 || sps->chromaFormatIdc==YUV422)? 16 :  8;

    decoder->coding.subpelX = decoder->coding.mbCrSizeX == 8 ? 7 : 3;
    decoder->coding.subpelY = decoder->coding.mbCrSizeY == 8 ? 7 : 3;
    decoder->coding.shiftpelX = decoder->coding.mbCrSizeX == 8 ? 3 : 2;
    decoder->coding.shiftpelY = decoder->coding.mbCrSizeY == 8 ? 3 : 2;
    decoder->coding.totalScale = decoder->coding.shiftpelX + decoder->coding.shiftpelY;
    }
    //}}}

  decoder->coding.mbCrSize = decoder->coding.mbCrSizeX * decoder->coding.mbCrSizeY;
  decoder->coding.mbSizeBlock[0][0] = decoder->coding.mbSizeBlock[0][1] = decoder->coding.mbSize[0][0] >> 2;
  decoder->coding.mbSizeBlock[1][0] = decoder->coding.mbSizeBlock[2][0] = decoder->coding.mbSize[1][0] >> 2;
  decoder->coding.mbSizeBlock[1][1] = decoder->coding.mbSizeBlock[2][1] = decoder->coding.mbSize[1][1] >> 2;

  decoder->coding.mbSizeShift[0][0] = decoder->coding.mbSizeShift[0][1] = ceilLog2sf (decoder->coding.mbSize[0][0]);
  decoder->coding.mbSizeShift[1][0] = decoder->coding.mbSizeShift[2][0] = ceilLog2sf (decoder->coding.mbSize[1][0]);
  decoder->coding.mbSizeShift[1][1] = decoder->coding.mbSizeShift[2][1] = ceilLog2sf (decoder->coding.mbSize[1][1]);
  }
//}}}
//{{{
static void setFormat (sDecoder* decoder, sSps* sps, sFrameFormat* source, sFrameFormat* output) {

  static const int kSubWidthC[4] = { 1, 2, 2, 1};
  static const int kSubHeightC[4] = { 1, 2, 1, 1};

  // source
  //{{{  crop
  int cropLeft, cropRight, cropTop, cropBot;
  if (sps->hasCrop) {
    cropLeft = kSubWidthC [sps->chromaFormatIdc] * sps->cropLeft;
    cropRight = kSubWidthC [sps->chromaFormatIdc] * sps->cropRight;
    cropTop = kSubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnly ) *  sps->cropTop;
    cropBot = kSubHeightC[sps->chromaFormatIdc] * ( 2 - sps->frameMbOnly ) *  sps->cropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBot = 0;

  source->width[0] = decoder->coding.width - cropLeft - cropRight;
  source->height[0] = decoder->coding.height - cropTop - cropBot;

  // cropping for chroma
  if (sps->hasCrop) {
    cropLeft = sps->cropLeft;
    cropRight = sps->cropRight;
    cropTop = (2 - sps->frameMbOnly) * sps->cropTop;
    cropBot = (2 - sps->frameMbOnly) * sps->cropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBot = 0;
  //}}}
  source->width[1] = decoder->widthCr - cropLeft - cropRight;
  source->width[2] = source->width[1];
  source->height[1] = decoder->heightCr - cropTop - cropBot;
  source->height[2] = source->height[1];

  // ????
  source->width[1] = decoder->widthCr;
  source->width[2] = decoder->widthCr;

  source->sizeCmp[0] = source->width[0] * source->height[0];
  source->sizeCmp[1] = source->width[1] * source->height[1];
  source->sizeCmp[2] = source->sizeCmp[1];
  source->mbWidth = source->width[0]  / MB_BLOCK_SIZE;
  source->mbHeight = source->height[0] / MB_BLOCK_SIZE;

  // output
  output->width[0] = decoder->coding.width;
  output->height[0] = decoder->coding.height;
  output->height[1] = decoder->heightCr;
  output->height[2] = decoder->heightCr;

  // output size (excluding padding)
  output->sizeCmp[0] = output->width[0] * output->height[0];
  output->sizeCmp[1] = output->width[1] * output->height[1];
  output->sizeCmp[2] = output->sizeCmp[1];
  output->mbWidth = output->width[0]  / MB_BLOCK_SIZE;
  output->mbHeight = output->height[0] / MB_BLOCK_SIZE;

  output->bitDepth[0] = source->bitDepth[0] = decoder->bitDepthLuma;
  output->bitDepth[1] = source->bitDepth[1] = decoder->bitDepthChroma;
  output->bitDepth[2] = source->bitDepth[2] = decoder->bitDepthChroma;

  output->colourModel = source->colourModel;
  output->yuvFormat = source->yuvFormat = (eYuvFormat)sps->chromaFormatIdc;
  }
//}}}

//{{{
static void reorderLists (sSlice* slice) {

  sDecoder* decoder = slice->decoder;

  if ((slice->sliceType != eSliceI) && (slice->sliceType != eSliceSI)) {
    if (slice->refPicReorderFlag[LIST_0])
      reorderRefPicList (slice, LIST_0);
    if (decoder->noReferencePicture == slice->listX[0][slice->numRefIndexActive[LIST_0]-1])
      cLog::log (LOGERROR, "------ refPicList0[%d] no refPic %s",
                 slice->numRefIndexActive[LIST_0]-1, decoder->nonConformingStream ? "conform":"");
    else
      slice->listXsize[0] = (char) slice->numRefIndexActive[LIST_0];
    }

  if (slice->sliceType == eSliceB) {
    if (slice->refPicReorderFlag[LIST_1])
      reorderRefPicList (slice, LIST_1);
    if (decoder->noReferencePicture == slice->listX[1][slice->numRefIndexActive[LIST_1]-1])
       cLog::log (LOGERROR, "------ refPicList1[%d] no refPic %s",
              slice->numRefIndexActive[LIST_0] - 1, decoder->nonConformingStream ? "conform" : "");
    else
      slice->listXsize[1] = (char)slice->numRefIndexActive[LIST_1];
    }

  freeRefPicListReorderBuffer (slice);
  }
//}}}
//{{{
static void initRefPicture (sSlice* slice, sDecoder* decoder) {

  sPicture* vidRefPicture = decoder->noReferencePicture;
  int noRef = slice->framePoc < decoder->recoveryPoc;

  if (decoder->coding.isSeperateColourPlane) {
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
    int totalLists = slice->mbAffFrame ? 6 : (slice->sliceType == eSliceB ? 2 : 1);
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
  dst->isIDR = src->isIDR;
  dst->noOutputPriorPicFlag = src->noOutputPriorPicFlag;
  dst->longTermRefFlag = src->longTermRefFlag;
  dst->adaptRefPicBufFlag = src->adaptRefPicBufFlag;
  dst->decRefPicMarkBuffer = src->decRefPicMarkBuffer;
  dst->mbAffFrame = src->mbAffFrame;
  dst->picWidthMbs = src->picWidthMbs;
  dst->picNum  = src->picNum;
  dst->frameNum = src->frameNum;
  dst->recoveryFrame = src->recoveryFrame;
  dst->codedFrame = src->codedFrame;
  dst->chromaFormatIdc = src->chromaFormatIdc;
  dst->frameMbOnly = src->frameMbOnly;
  dst->hasCrop = src->hasCrop;
  dst->cropLeft = src->cropLeft;
  dst->cropRight = src->cropRight;
  dst->cropTop = src->cropTop;
  dst->cropBot = src->cropBot;
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

  sPicture* picture = decoder->picture;

  sPixel** imgY = picture->imgY;
  sPixel*** imgUV = picture->imgUV;

  for (uint32_t i = 0; i < picture->picSizeInMbs; i += 2) {
    if (picture->motion.mbField[i]) {
      int16_t x0;
      int16_t y0;
      getMbPos (decoder, i, decoder->mbSize[eLuma], &x0, &y0);

      sPixel tempBuffer[32][16];
      updateMbAff (imgY + y0, tempBuffer, x0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

      if (picture->chromaFormatIdc != YUV400) {
        x0 = (int16_t)((x0 * decoder->mbCrSizeX) >> 4);
        y0 = (int16_t)((y0 * decoder->mbCrSizeY) >> 4);
        updateMbAff (imgUV[0] + y0, tempBuffer, x0, decoder->mbCrSizeX, decoder->mbCrSizeY);
        updateMbAff (imgUV[1] + y0, tempBuffer, x0, decoder->mbCrSizeX, decoder->mbCrSizeY);
        }
      }
    }
  }
//}}}

//{{{
static void endDecodeFrame (sDecoder* decoder) {

  // return if the last picture has already been finished
  if (!decoder->picture ||
      ((decoder->numDecodedMbs != decoder->picSizeInMbs) &&
       ((decoder->coding.yuvFormat != YUV444) || !decoder->coding.isSeperateColourPlane)))
    return;

  //{{{  error conceal
  frame recfr;
  recfr.decoder = decoder;
  recfr.yptr = &decoder->picture->imgY[0][0];
  if (decoder->picture->chromaFormatIdc != YUV400) {
    recfr.uptr = &decoder->picture->imgUV[0][0][0];
    recfr.vptr = &decoder->picture->imgUV[1][0][0];
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
    if (decoder->picture->sliceType == eSliceI || decoder->picture->sliceType == eSliceSI) // I-frame
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
    if (decoder->coding.isSeperateColourPlane) {
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
  else if (decoder->coding.isSeperateColourPlane)
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
  int isIdr = decoder->picture->isIDR;
  int chromaFormatIdc = decoder->picture->chromaFormatIdc;
  storePictureDpb (decoder->dpb, decoder->picture);
  decoder->picture = NULL;

  if (decoder->lastHasMmco5)
    decoder->preFrameNum = 0;

  if (picStructure == eTopField || picStructure == eFrame) {
    //{{{
    if (sliceType == eSliceI && isIdr)
      decoder->debug.sliceTypeString = "IDR";
    else if (sliceType == eSliceI)
      decoder->debug.sliceTypeString = " I ";
    else if (sliceType == eSliceP)
      decoder->debug.sliceTypeString = " P ";
    else if (sliceType == eSliceSP)
      decoder->debug.sliceTypeString = "SP ";
    else if  (sliceType == eSliceSI)
      decoder->debug.sliceTypeString = "SI ";
    else if (refpic)
      decoder->debug.sliceTypeString = " B ";
    else
      decoder->debug.sliceTypeString = " b ";
    if (picStructure == eFrame)
      decoder->debug.sliceTypeString += "    ";
    }
    //}}}
  else if (picStructure == eBotField) {
    //{{{
    if (sliceType == eSliceI && isIdr)
      decoder->debug.sliceTypeString += "|IDR";
    else if (sliceType == eSliceI)
      decoder->debug.sliceTypeString += "| I ";
    else if (sliceType == eSliceP)
      decoder->debug.sliceTypeString += "| P ";
    else if (sliceType == eSliceSP)
      decoder->debug.sliceTypeString += "|SP ";
    else if  (sliceType == eSliceSI)
      decoder->debug.sliceTypeString += "|SI ";
    else if (refpic)
      decoder->debug.sliceTypeString += "| B ";
    else
      decoder->debug.sliceTypeString += "| b ";
    }
    //}}}
  if ((picStructure == eFrame) || picStructure == eBotField) {
    //{{{  debug
    getTime (&decoder->debug.endTime);

    // count numOutputFrames
    int numOutputFrames = 0;
    sDecodedPic* decodedPic = decoder->outDecodedPics;
    while (decodedPic) {
      if (decodedPic->ok)
        numOutputFrames++;
      decodedPic = decodedPic->next;
      }

    decoder->debug.outSliceType = (eSliceType)sliceType;

    decoder->debug.outString = fmt::format ("{} {}:{}:{:2d} {:3d}ms ->{}-> poc:{} pic:{} -> {}",
             decoder->decodeFrameNum,
             decoder->numDecodedSlices, decoder->numDecodedMbs, qp,
             (int)timeNorm (timeDiff (&decoder->debug.startTime, &decoder->debug.endTime)),
             decoder->debug.sliceTypeString,
             pocNum, picNum, numOutputFrames);

    if (decoder->param.outDebug)
      cLog::log (LOGINFO, "-> " + decoder->debug.outString);
    //}}}

    // I or P pictures ?
    if ((sliceType == eSliceI) || (sliceType == eSliceSI) || (sliceType == eSliceP) || refpic)
      ++(decoder->idrFrameNum);
    (decoder->decodeFrameNum)++;
    }
  }
//}}}
//{{{
static void initPicture (sDecoder* decoder, sSlice* slice) {

  sDpb* dpb = slice->dpb;
  sSps* activeSps = decoder->activeSps;

  decoder->picHeightInMbs = decoder->coding.frameHeightMbs / (slice->fieldPic+1);
  decoder->picSizeInMbs = decoder->coding.picWidthMbs * decoder->picHeightInMbs;
  decoder->coding.frameSizeMbs = decoder->coding.picWidthMbs * decoder->coding.frameHeightMbs;

  if (decoder->picture) // slice loss
    endDecodeFrame (decoder);

  if (decoder->recoveryPoint)
    decoder->recoveryFrameNum = (slice->frameNum + decoder->recoveryFrameCount) % decoder->coding.maxFrameNum;
  if (slice->isIDR)
    decoder->recoveryFrameNum = slice->frameNum;
  if (!decoder->recoveryPoint &&
      (slice->frameNum != decoder->preFrameNum) &&
      (slice->frameNum != (decoder->preFrameNum + 1) % decoder->coding.maxFrameNum)) {
    if (!activeSps->allowGapsFrameNum) {
      //{{{  picture error conceal
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
      //}}}
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
    getTime (&decoder->debug.startTime);

  sPicture* picture = decoder->picture = allocPicture (decoder, slice->picStructure, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);
  picture->topPoc = slice->topPoc;
  picture->botPoc = slice->botPoc;
  picture->framePoc = slice->framePoc;
  picture->qp = slice->qp;
  picture->sliceQpDelta = slice->sliceQpDelta;
  picture->chromaQpOffset[0] = decoder->activePps->chromaQpOffset;
  picture->chromaQpOffset[1] = decoder->activePps->chromaQpOffset2;
  picture->codingType = slice->picStructure == eFrame ?
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

  if (decoder->coding.sliceType > eSliceSI) {
    setEcFlag (decoder, SE_PTYPE);
    decoder->coding.sliceType = eSliceP;  // concealed element
    }

  // cavlc init
  if (decoder->activePps->entropyCoding == eCavlc)
    memset (decoder->nzCoeff[0][0][0], -1, decoder->picSizeInMbs * 48 *sizeof(uint8_t)); // 3 * 4 * 4

  // Set the sliceNum member of each MB to -1, to ensure correct when packet loss occurs
  // TO set sMacroBlock Map (mark all MBs as 'have to be concealed')
  if (decoder->coding.isSeperateColourPlane) {
    for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
      sMacroBlock* mb = decoder->mbDataJV[nplane];
      char* intraBlock = decoder->intraBlockJV[nplane];
      for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
        resetMb (mb++);
      memset (decoder->predModeJV[nplane][0], DC_PRED, 16 * decoder->coding.frameHeightMbs * decoder->coding.picWidthMbs * sizeof(char));
      if (decoder->activePps->hasConstrainedIntraPred)
        for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
          intraBlock[i] = 1;
      }
    }
  else {
    sMacroBlock* mb = decoder->mbData;
    for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
      resetMb (mb++);
    if (decoder->activePps->hasConstrainedIntraPred)
      for (int i = 0; i < (int)decoder->picSizeInMbs; ++i)
        decoder->intraBlock[i] = 1;
    memset (decoder->predMode[0], DC_PRED, 16 * decoder->coding.frameHeightMbs * decoder->coding.picWidthMbs * sizeof(char));
    }

  picture->sliceType = decoder->coding.sliceType;
  picture->usedForReference = (slice->refId != 0);
  picture->isIDR = slice->isIDR;
  picture->noOutputPriorPicFlag = slice->noOutputPriorPicFlag;
  picture->longTermRefFlag = slice->longTermRefFlag;
  picture->adaptRefPicBufFlag = slice->adaptRefPicBufFlag;
  picture->decRefPicMarkBuffer = slice->decRefPicMarkBuffer;
  slice->decRefPicMarkBuffer = NULL;

  picture->mbAffFrame = slice->mbAffFrame;
  picture->picWidthMbs = decoder->coding.picWidthMbs;

  decoder->getMbBlockPos = picture->mbAffFrame ? getMbBlockPosMbaff : getMbBlockPosNormal;
  decoder->getNeighbour = picture->mbAffFrame ? getAffNeighbour : getNonAffNeighbour;

  picture->picNum = slice->frameNum;
  picture->frameNum = slice->frameNum;
  picture->recoveryFrame = (uint32_t)((int)slice->frameNum == decoder->recoveryFrameNum);
  picture->codedFrame = (slice->picStructure == eFrame);
  picture->chromaFormatIdc = (eYuvFormat)activeSps->chromaFormatIdc;
  picture->frameMbOnly = activeSps->frameMbOnly;
  picture->hasCrop = activeSps->hasCrop;
  if (picture->hasCrop) {
    picture->cropLeft = activeSps->cropLeft;
    picture->cropRight = activeSps->cropRight;
    picture->cropTop = activeSps->cropTop;
    picture->cropBot = activeSps->cropBot;
    }

  if (decoder->coding.isSeperateColourPlane) {
    decoder->decPictureJV[0] = decoder->picture;
    decoder->decPictureJV[1] = allocPicture (decoder, (ePicStructure) slice->picStructure, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);
    copyDecPictureJV (decoder, decoder->decPictureJV[1], decoder->decPictureJV[0] );
    decoder->decPictureJV[2] = allocPicture (decoder, (ePicStructure) slice->picStructure, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);
    copyDecPictureJV (decoder, decoder->decPictureJV[2], decoder->decPictureJV[0] );
    }
  }
//}}}
//{{{
static void useParameterSet (sDecoder* decoder, sSlice* slice) {

  sPps* pps = &decoder->pps[slice->ppsId];
  if (!pps->ok)
    cLog::log (LOGINFO, fmt::format ("useParameterSet - invalid ppsId:{}", slice->ppsId));

  sSps* sps = &decoder->sps[pps->spsId];
  if (!sps->ok)
    cLog::log (LOGINFO, fmt::format ("useParameterSet - invalid spsId:{} ppsId:{}", slice->ppsId, pps->spsId));

  if (sps != decoder->activeSps) {
    //{{{  new sps
    if (decoder->picture)
      endDecodeFrame (decoder);

    decoder->activeSps = sps;

    if (isBLprofile (sps->profileIdc) && !decoder->dpb->initDone)
      setCodingParam (decoder, sps);
    setCoding (decoder);
    initGlobalBuffers (decoder);

    if (!decoder->noOutputPriorPicFlag)
      flushDpb (decoder->dpb);
    initDpb (decoder, decoder->dpb, 0);

    // enable error conceal
    ercInit (decoder, decoder->coding.width, decoder->coding.height, 1);
    if (decoder->picture) {
      ercReset (decoder->ercErrorVar, decoder->picSizeInMbs, decoder->picSizeInMbs, decoder->picture->sizeX);
      decoder->ercMvPerMb = 0;
      }

    setFormat (decoder, sps, &decoder->param.source, &decoder->param.output);

    // debug spsStr
    decoder->debug.profileString = fmt::format ("profile:{} {}x{} {}x{} yuv{} {}:{}:{}",
             decoder->coding.profileIdc,
             decoder->param.source.width[0], decoder->param.source.height[0],
             decoder->coding.width, decoder->coding.height,
             decoder->coding.yuvFormat == YUV400 ? " 400 ":
               decoder->coding.yuvFormat == YUV420 ? " 420":
                 decoder->coding.yuvFormat == YUV422 ? " 422":" 4:4:4",
             decoder->param.source.bitDepth[0],
             decoder->param.source.bitDepth[1],
             decoder->param.source.bitDepth[2]);

    // print profile debug
    cLog::log (LOGINFO, decoder->debug.profileString);
    }
    //}}}

  if (pps != decoder->activePps) {
    //{{{  new pps
    if (decoder->picture) // only on slice loss
      endDecodeFrame (decoder);

    decoder->activePps = pps;
    }
    //}}}

  // slice->dataPartitionMode is set by read_new_slice (NALU first uint8_t available there)
  if (pps->entropyCoding == eCavlc) {
    slice->nalStartCode = vlcStartCode;
    for (int i = 0; i < 3; i++)
      slice->dataPartitions[i].readSyntaxElement = readSyntaxElementVLC;
    }
  else {
    slice->nalStartCode = cabacStartCode;
    for (int i = 0; i < 3; i++)
      slice->dataPartitions[i].readSyntaxElement = readSyntaxElementCABAC;
    }

  decoder->coding.sliceType = slice->sliceType;
  }
//}}}
//{{{
static void initPictureDecode (sDecoder* decoder) {

  int deblockMode = 1;

  if (decoder->picSliceIndex >= MAX_NUM_SLICES)
    error ("initPictureDecode - MAX_NUM_SLICES exceeded");

  sSlice* slice = decoder->sliceList[0];
  useParameterSet (decoder, slice);

  if (slice->isIDR)
    decoder->idrFrameNum = 0;

  decoder->picHeightInMbs = decoder->coding.frameHeightMbs / (1 + slice->fieldPic);
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

  decoder->activeSps = slice->activeSps;
  decoder->activePps = slice->activePps;

  slice->initLists (slice);
  reorderLists (slice);

  if (slice->picStructure == eFrame)
    initMbAffLists (decoder, slice);

  // update reference flags and set current decoder->refFlag
  if (!(slice->redundantPicCount && (decoder->prevFrameNum == slice->frameNum)))
    for (int i = 16; i > 0; i--)
      slice->refFlag[i] = slice->refFlag[i-1];
  slice->refFlag[0] = (slice->redundantPicCount == 0) ? decoder->isPrimaryOk : decoder->isRedundantOk;

  if (!slice->activeSps->chromaFormatIdc ||
      (slice->activeSps->chromaFormatIdc == 3)) {
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
static int isNewPicture (sPicture* picture, sSlice* slice, sOldSlice* oldSlice) {

  int result = (NULL == picture);

  result |= (oldSlice->ppsId != slice->ppsId);
  result |= (oldSlice->frameNum != slice->frameNum);
  result |= (oldSlice->fieldPic != slice->fieldPic);

  if (slice->fieldPic && oldSlice->fieldPic)
    result |= (oldSlice->botField != slice->botField);

  result |= (oldSlice->nalRefIdc != slice->refId) && (!oldSlice->nalRefIdc || !slice->refId);
  result |= (oldSlice->isIDR != slice->isIDR);

  if (slice->isIDR && oldSlice->isIDR)
    result |= (oldSlice->idrPicId != slice->idrPicId);

  sDecoder* decoder = slice->decoder;

  if (!decoder->activeSps->pocType) {
    result |= (oldSlice->picOrderCountLsb != slice->picOrderCountLsb);
    if ((decoder->activePps->frameBotField == 1) && !slice->fieldPic)
      result |= (oldSlice->deltaPicOrderCountBot != slice->deletaPicOrderCountBot);
    }

  if (decoder->activeSps->pocType == 1) {
    if (!decoder->activeSps->deltaPicOrderAlwaysZero) {
      result |= (oldSlice->deltaPicOrderCount[0] != slice->deltaPicOrderCount[0]);
      if ((decoder->activePps->frameBotField == 1) && !slice->fieldPic)
        result |= (oldSlice->deltaPicOrderCount[1] != slice->deltaPicOrderCount[1]);
      }
    }

  return result;
  }
//}}}
//{{{
static void readDecRefPicMarking (sDecoder* decoder, sBitStream* s, sSlice* slice) {

  // free old buffer content
  while (slice->decRefPicMarkBuffer) {
    sDecodedRefPicMark* tmp_drpm = slice->decRefPicMarkBuffer;
    slice->decRefPicMarkBuffer = tmp_drpm->next;
    free (tmp_drpm);
    }

  if (slice->isIDR) {
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
        sDecodedRefPicMark* tempDrpm = (sDecodedRefPicMark*)calloc (1,sizeof (sDecodedRefPicMark));
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
        if (!slice->decRefPicMarkBuffer)
          slice->decRefPicMarkBuffer = tempDrpm;
        else {
          sDecodedRefPicMark* tempDrpm2 = slice->decRefPicMarkBuffer;
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
static void readSliceHeader (sDecoder* decoder, sSlice* slice) {
// Some slice syntax depends on parameterSet depends on parameterSetID of the slice header
// - read the ppsId of the slice header first
//   - then setup the active parameter sets
// - read the rest of the slice header

  uint32_t partitionIndex = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode][SE_HEADER];
  sBitStream* s = slice->dataPartitions[partitionIndex].stream;

  slice->startMbNum = readUeV ("SLC first_mb_in_slice", s);

  int tmp = readUeV ("SLC sliceType", s);
  if (tmp > 4)
    tmp -= 5;
  slice->sliceType = (eSliceType)tmp;
  decoder->coding.sliceType = slice->sliceType;

  slice->ppsId = readUeV ("SLC ppsId", s);

  if (decoder->coding.isSeperateColourPlane)
    slice->colourPlaneId = readUv (2, "SLC colourPlaneId", s);
  else
    slice->colourPlaneId = PLANE_Y;

  // setup parameterSet
  useParameterSet (decoder, slice);
  slice->activeSps = decoder->activeSps;
  slice->activePps = decoder->activePps;
  slice->transform8x8Mode = decoder->activePps->hasTransform8x8mode;
  slice->chroma444notSeparate = (decoder->activeSps->chromaFormatIdc == YUV444) && !decoder->coding.isSeperateColourPlane;

  sSps* activeSps = decoder->activeSps;
  slice->frameNum = readUv (activeSps->log2maxFrameNumMinus4 + 4, "SLC frameNum", s);
  if (slice->isIDR) {
    decoder->preFrameNum = slice->frameNum;
    decoder->lastRefPicPoc = 0;
    }
  //{{{  read field/frame
  if (activeSps->frameMbOnly) {
    slice->fieldPic = 0;
    decoder->coding.picStructure = eFrame;
    }

  else {
    slice->fieldPic = readU1 ("SLC fieldPic", s);
    if (slice->fieldPic) {
      slice->botField = (uint8_t)readU1 ("SLC botField", s);
      decoder->coding.picStructure = slice->botField ? eBotField : eTopField;
      }
    else {
      slice->botField = false;
      decoder->coding.picStructure = eFrame;
      }
    }

  slice->picStructure = decoder->coding.picStructure;
  slice->mbAffFrame = activeSps->mbAffFlag && !slice->fieldPic;
  //}}}

  if (slice->isIDR)
    slice->idrPicId = readUeV ("SLC idrPicId", s);
  //{{{  read picOrderCount
  if (activeSps->pocType == 0) {
    slice->picOrderCountLsb = readUv (activeSps->log2maxPocLsbMinus4 + 4, "SLC picOrderCountLsb", s);
    if ((decoder->activePps->frameBotField == 1) && !slice->fieldPic)
      slice->deletaPicOrderCountBot = readSeV ("SLC deletaPicOrderCountBot", s);
    else
      slice->deletaPicOrderCountBot = 0;
    }

  if (activeSps->pocType == 1) {
    if (!activeSps->deltaPicOrderAlwaysZero) {
      slice->deltaPicOrderCount[0] = readSeV ("SLC deltaPicOrderCount[0]", s);
      if ((decoder->activePps->frameBotField == 1) && !slice->fieldPic)
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

  if (decoder->activePps->redundantPicCountPresent)
    slice->redundantPicCount = readUeV ("SLC redundantPicCount", s);

  if (slice->sliceType == eSliceB)
    slice->directSpatialMvPredFlag = readU1 ("SLC directSpatialMvPredFlag", s);

  // read refPics
  slice->numRefIndexActive[LIST_0] = decoder->activePps->numRefIndexL0defaultActiveMinus1 + 1;
  slice->numRefIndexActive[LIST_1] = decoder->activePps->numRefIndexL1defaultActiveMinus1 + 1;
  if ((slice->sliceType == eSliceP) || (slice->sliceType == eSliceSP) || (slice->sliceType == eSliceB)) {
    if (readU1 ("SLC isNumRefIndexOverride", s)) {
      slice->numRefIndexActive[LIST_0] = 1 + readUeV ("SLC numRefIndexActiveL0minus1", s);
      if (slice->sliceType == eSliceB)
        slice->numRefIndexActive[LIST_1] = 1 + readUeV ("SLC numRefIndexActiveL1minus1", s);
      }
    }
  if (slice->sliceType != eSliceB)
    slice->numRefIndexActive[LIST_1] = 0;
  //{{{  read refPicList reorder
  allocRefPicListReordeBuffer (slice);

  if ((slice->sliceType != eSliceI) && (slice->sliceType != eSliceSI)) {
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

  if (slice->sliceType == eSliceB) {
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
  if (slice->redundantPicCount && (slice->sliceType != eSliceI) )
    slice->redundantSliceRefIndex = slice->absDiffPicNumMinus1[LIST_0][0] + 1;
  //}}}
  //{{{  read weightedPredWeight
  slice->hasWeightedPred = (uint16_t)(((slice->sliceType == eSliceP) || (slice->sliceType == eSliceSP))
                              ? decoder->activePps->hasWeightedPred
                              : ((slice->sliceType == eSliceB) && (decoder->activePps->weightedBiPredIdc == 1)));

  slice->weightedBiPredIdc = (uint16_t)((slice->sliceType == eSliceB) &&
                                              (decoder->activePps->weightedBiPredIdc > 0));

  if ((decoder->activePps->hasWeightedPred &&
       ((slice->sliceType == eSliceP) || (slice->sliceType == eSliceSP))) ||
      ((decoder->activePps->weightedBiPredIdc == 1) && (slice->sliceType == eSliceB))) {
    slice->lumaLog2weightDenom = (uint16_t)readUeV ("SLC lumaLog2weightDenom", s);
    slice->wpRoundLuma = slice->lumaLog2weightDenom ? 1 << (slice->lumaLog2weightDenom - 1) : 0;

    if (activeSps->chromaFormatIdc) {
      slice->chromaLog2weightDenom = (uint16_t)readUeV ("SLC chromaLog2weightDenom", s);
      slice->wpRoundChroma = slice->chromaLog2weightDenom ? 1 << (slice->chromaLog2weightDenom - 1) : 0;
      }

    resetWeightedPredParam (slice);
    for (int i = 0; i < slice->numRefIndexActive[LIST_0]; i++) {
      //{{{  read l0 weights
      if (readU1 ("SLC hasLumaWeightL0", s)) {
        slice->weightedPredWeight[LIST_0][i][0] = readSeV ("SLC lumaWeightL0", s);
        slice->weightedPredOffset[LIST_0][i][0] = readSeV ("SLC lumaOffsetL0", s);
        slice->weightedPredOffset[LIST_0][i][0] = slice->weightedPredOffset[LIST_0][i][0] << (decoder->bitDepthLuma - 8);
        }
      else {
        slice->weightedPredWeight[LIST_0][i][0] = 1 << slice->lumaLog2weightDenom;
        slice->weightedPredOffset[LIST_0][i][0] = 0;
        }

      if (activeSps->chromaFormatIdc) {
        // l0 chroma weights
        int hasChromaWeightL0 = readU1 ("SLC hasChromaWeightL0", s);
        for (int j = 1; j < 3; j++) {
          if (hasChromaWeightL0) {
            slice->weightedPredWeight[LIST_0][i][j] = readSeV ("SLC chromaWeightL0", s);
            slice->weightedPredOffset[LIST_0][i][j] = readSeV ("SLC chromaOffsetL0", s);
            slice->weightedPredOffset[LIST_0][i][j] = slice->weightedPredOffset[LIST_0][i][j] << (decoder->bitDepthChroma-8);
            }
          else {
            slice->weightedPredWeight[LIST_0][i][j] = 1 << slice->chromaLog2weightDenom;
            slice->weightedPredOffset[LIST_0][i][j] = 0;
            }
          }
        }
      }
      //}}}

    if ((slice->sliceType == eSliceB) && decoder->activePps->weightedBiPredIdc == 1)
      for (int i = 0; i < slice->numRefIndexActive[LIST_1]; i++) {
        //{{{  read l1 weights
        if (readU1 ("SLC hasLumaWeightL1", s)) {
          // read l1 luma weights
          slice->weightedPredWeight[LIST_1][i][0] = readSeV ("SLC lumaWeightL1", s);
          slice->weightedPredOffset[LIST_1][i][0] = readSeV ("SLC lumaOffsetL1", s);
          slice->weightedPredOffset[LIST_1][i][0] = slice->weightedPredOffset[LIST_1][i][0] << (decoder->bitDepthLuma-8);
          }
        else {
          slice->weightedPredWeight[LIST_1][i][0] = 1 << slice->lumaLog2weightDenom;
          slice->weightedPredOffset[LIST_1][i][0] = 0;
          }

        if (activeSps->chromaFormatIdc) {
          int hasChromaWeightL1 = readU1 ("SLC hasChromaWeightL1", s);
          for (int j = 1; j < 3; j++) {
            if (hasChromaWeightL1) {
              // read l1 chroma weights
              slice->weightedPredWeight[LIST_1][i][j] = readSeV("SLC chromaWeightL1", s);
              slice->weightedPredOffset[LIST_1][i][j] = readSeV("SLC chromaOffsetL1", s);
              slice->weightedPredOffset[LIST_1][i][j] = slice->weightedPredOffset[LIST_1][i][j]<<(decoder->bitDepthChroma-8);
              }
            else {
              slice->weightedPredWeight[LIST_1][i][j] = 1 << slice->chromaLog2weightDenom;
              slice->weightedPredOffset[LIST_1][i][j] = 0;
              }
            }
          }
        }
        //}}}
    }
  //}}}

  if (slice->refId)
    readDecRefPicMarking (decoder, s, slice);

  if (decoder->activePps->entropyCoding &&
      (slice->sliceType != eSliceI) && (slice->sliceType != eSliceSI))
    slice->cabacInitIdc = readUeV ("SLC cabacInitIdc", s);
  else
    slice->cabacInitIdc = 0;
  //{{{  read qp
  slice->sliceQpDelta = readSeV ("SLC sliceQpDelta", s);
  slice->qp = 26 + decoder->activePps->picInitQpMinus26 + slice->sliceQpDelta;

  if ((slice->sliceType == eSliceSP) || (slice->sliceType == eSliceSI)) {
    if (slice->sliceType == eSliceSP)
      slice->spSwitch = readU1 ("SLC sp_for_switchFlag", s);
    slice->sliceQsDelta = readSeV ("SLC sliceQsDelta", s);
    slice->qs = 26 + decoder->activePps->picInitQsMinus26 + slice->sliceQsDelta;
    }
  //}}}

  if (decoder->activePps->hasDeblockFilterControl) {
    //{{{  read deblockFilter
    slice->deblockFilterDisableIdc = (int16_t)readUeV ("SLC disable_deblocking_filter_idc", s);
    if (slice->deblockFilterDisableIdc != 1) {
      slice->deblockFilterC0Offset = (int16_t)(2 * readSeV ("SLC slice_alpha_c0_offset_div2", s));
      slice->deblockFilterBetaOffset = (int16_t)(2 * readSeV ("SLC slice_beta_offset_div2", s));
      }
    else
      slice->deblockFilterC0Offset = slice->deblockFilterBetaOffset = 0;
    }
    //}}}
  else {
    //{{{  enable deblockFilter
    slice->deblockFilterDisableIdc = 0;
    slice->deblockFilterC0Offset = 0;
    slice->deblockFilterBetaOffset = 0;
    }
    //}}}
  if (isHiIntraOnlyProfile (activeSps->profileIdc, activeSps->constrainedSet3flag) &&
      !decoder->param.intraProfileDeblocking) {
    //{{{  hiIntra deblock
    slice->deblockFilterDisableIdc = 1;
    slice->deblockFilterC0Offset = 0;
    slice->deblockFilterBetaOffset = 0;
    }
    //}}}
  //{{{  read sliceGroup
  if ((decoder->activePps->numSliceGroupsMinus1 > 0) &&
      (decoder->activePps->sliceGroupMapType >= 3) &&
      (decoder->activePps->sliceGroupMapType <= 5)) {
    int len = (activeSps->picHeightMapUnitsMinus1+1) * (activeSps->picWidthMbsMinus1+1) /
                (decoder->activePps->sliceGroupChangeRateMius1 + 1);
    if (((activeSps->picHeightMapUnitsMinus1+1) * (activeSps->picWidthMbsMinus1+1)) %
        (decoder->activePps->sliceGroupChangeRateMius1 + 1))
      len += 1;

    len = ceilLog2 (len+1);
    slice->sliceGroupChangeCycle = readUv (len, "SLC sliceGroupChangeCycle", s);
    }
  //}}}

  decoder->picHeightInMbs = decoder->coding.frameHeightMbs / ( 1 + slice->fieldPic );
  decoder->picSizeInMbs = decoder->coding.picWidthMbs * decoder->picHeightInMbs;
  decoder->coding.frameSizeMbs = decoder->coding.picWidthMbs * decoder->coding.frameHeightMbs;
  }
//}}}
//{{{
static int readSlice (sSlice* slice) {

  int curHeader = 0;

  sDecoder* decoder = slice->decoder;

  for (;;) {
    sNalu* nalu = decoder->nalu;
    if (decoder->pendingNalu) {
      nalu = decoder->pendingNalu;
      decoder->pendingNalu = NULL;
      }
    else if (!readNalu (decoder, nalu))
      return eEOS;

  processNalu:
    switch (nalu->unitType) {
      case NALU_TYPE_SLICE:
      //{{{
      case NALU_TYPE_IDR: {
        // recovery
        if (decoder->recoveryPoint || nalu->unitType == NALU_TYPE_IDR) {
          if (!decoder->recoveryPointFound) {
            if (nalu->unitType != NALU_TYPE_IDR) {
              cLog::log (LOGINFO,  "-> decoding without IDR");
              decoder->nonConformingStream = 1;
              }
            else
              decoder->nonConformingStream = 0;
            }
          decoder->recoveryPointFound = 1;
          }
        if (!decoder->recoveryPointFound)
          break;

        // read sliceHeader
        slice->isIDR = (nalu->unitType == NALU_TYPE_IDR);
        slice->refId = nalu->refId;
        slice->dataPartitionMode = eDataPartition1;
        slice->maxDataPartitions = 1;
        sBitStream* s = slice->dataPartitions[0].stream;
        s->readLen = 0;
        s->errorFlag = 0;
        s->bitStreamOffset = 0;
        memcpy (s->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
        s->bitStreamLen = RBSPtoSODB (s->bitStreamBuffer, nalu->len - 1);
        s->codeLen = s->bitStreamLen;

        readSliceHeader (decoder, slice);

        // if primary slice replaced by redundant slice, set correct image type
        if (slice->redundantPicCount && !decoder->isPrimaryOk && decoder->isRedundantOk)
          decoder->picture->sliceType = decoder->coding.sliceType;
        if (isNewPicture (decoder->picture, slice, decoder->oldSlice)) {
          if (!decoder->picSliceIndex)
            initPicture (decoder, slice);
          curHeader = eSOP;
          checkZeroByteVCL (decoder, nalu);
          }
        else
          curHeader = eSOS;

        useQuantParams (slice);
        setSliceFunctions (slice);

        if (slice->mbAffFrame)
          slice->mbIndex = slice->startMbNum << 1;
        else
          slice->mbIndex = slice->startMbNum;

        if (decoder->activePps->entropyCoding) {
          int byteStartPosition = s->bitStreamOffset / 8;
          if (s->bitStreamOffset % 8)
            ++byteStartPosition;
          arithmeticDecodeStartDecoding (&slice->dataPartitions[0].cabacDecodeEnv, s->bitStreamBuffer,
                                         byteStartPosition, &s->readLen);
          }

        decoder->recoveryPoint = 0;

        // debug
        decoder->debug.sliceType = slice->sliceType;
        decoder->debug.sliceString = fmt::format ("{}:{}:{:6d} -> pps:{} frame:{:2d} {} {}{}",
                 (nalu->unitType == NALU_TYPE_IDR) ? "IDR":"SLC", slice->refId, nalu->len,
                 slice->ppsId, slice->frameNum,
                 slice->sliceType ? (slice->sliceType == 1) ? 'B':((slice->sliceType == 2) ? 'I':'?'):'P',
                 slice->fieldPic ? " field":"", slice->mbAffFrame ? " mbAff":"");
        if (decoder->param.sliceDebug)
          cLog::log (LOGINFO, decoder->debug.sliceString);

        return curHeader;
        }
      //}}}

      //{{{
      case NALU_TYPE_SPS: {
        int spsId = sSps::readNaluSps (decoder, nalu);
        if (decoder->param.spsDebug)
          cLog::log (LOGINFO, decoder->sps[spsId].getSpsString());
        break;
        }
      //}}}
      //{{{
      case NALU_TYPE_PPS: {
        int ppsId = readNaluPps (decoder, nalu);
        if (decoder->param.ppsDebug)
          cLog::log (LOGINFO, getPpsString (&decoder->pps[ppsId]));
        break;
        }
      //}}}

      case NALU_TYPE_SEI:
        processSei (nalu->buf, nalu->len, decoder, slice);
        break;

      //{{{
      case NALU_TYPE_DPA: {
        cLog::log (LOGINFO, "DPA id:%d:%d len:%d", slice->refId, slice->sliceType, nalu->len);

        if (!decoder->recoveryPointFound)
          break;

        // read dataPartition A
        slice->isIDR = false;
        slice->refId = nalu->refId;
        slice->noDataPartitionB = 1;
        slice->noDataPartitionC = 1;
        slice->dataPartitionMode = eDataPartition3;
        slice->maxDataPartitions = 3;
        sBitStream* s = slice->dataPartitions[0].stream;
        s->errorFlag = 0;
        s->bitStreamOffset = s->readLen = 0;
        memcpy (s->bitStreamBuffer, &nalu->buf[1], nalu->len - 1);
        s->codeLen = s->bitStreamLen = RBSPtoSODB (s->bitStreamBuffer, nalu->len-1);
        readSliceHeader (decoder, slice);

        if (isNewPicture (decoder->picture, slice, decoder->oldSlice)) {
          if (!decoder->picSliceIndex)
            initPicture (decoder, slice);
          curHeader = eSOP;
          checkZeroByteVCL (decoder, nalu);
          }
        else
          curHeader = eSOS;

        useQuantParams (slice);
        setSliceFunctions (slice);
        if (slice->mbAffFrame)
          slice->mbIndex = slice->startMbNum << 1;
        else
          slice->mbIndex = slice->startMbNum;

        // need to read the slice ID, which depends on the value of redundantPicCountPresent
        int slice_id_a = readUeV ("NALU: DP_A slice_id", s);
        if (decoder->activePps->entropyCoding)
          error ("dataPartition with eCabac not allowed");

        if (!readNalu (decoder, nalu))
          return curHeader;

        if (NALU_TYPE_DPB == nalu->unitType) {
          //{{{  got nalu dataPartitionB
          s = slice->dataPartitions[1].stream;
          s->errorFlag = 0;
          s->bitStreamOffset = s->readLen = 0;
          memcpy (s->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
          s->codeLen = s->bitStreamLen = RBSPtoSODB (s->bitStreamBuffer, nalu->len-1);
          int slice_id_b = readUeV ("NALU dataPartitionB sliceId", s);
          slice->noDataPartitionB = 0;

          if ((slice_id_b != slice_id_a) || (nalu->lostPackets)) {
            cLog::log (LOGINFO, "NALU dataPartitionB does not match dataPartitionA");
            slice->noDataPartitionB = 1;
            slice->noDataPartitionC = 1;
            }
          else {
            if (decoder->activePps->redundantPicCountPresent)
              readUeV ("NALU dataPartitionB redundantPicCount", s);

            // we're finished with dataPartitionB, so let's continue with next dataPartition
            if (!readNalu (decoder, nalu))
              return curHeader;
            }
          }
          //}}}
        else
          slice->noDataPartitionB = 1;

        if (NALU_TYPE_DPC == nalu->unitType) {
          //{{{  got nalu dataPartitionC
          s = slice->dataPartitions[2].stream;
          s->errorFlag = 0;
          s->bitStreamOffset = s->readLen = 0;
          memcpy (s->bitStreamBuffer, &nalu->buf[1], nalu->len-1);
          s->codeLen = s->bitStreamLen = RBSPtoSODB (s->bitStreamBuffer, nalu->len-1);

          slice->noDataPartitionC = 0;
          int slice_id_c = readUeV ("NALU: DP_C slice_id", s);
          if ((slice_id_c != slice_id_a) || (nalu->lostPackets)) {
            cLog::log (LOGINFO, "dataPartitionC does not match dataPartitionA");
            slice->noDataPartitionC = 1;
            }

          if (decoder->activePps->redundantPicCountPresent)
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
          goto processNalu;

        return curHeader;
        }
      //}}}
      //{{{
      case NALU_TYPE_DPB:
        cLog::log (LOGINFO, "dataPartitionB without dataPartitonA");
        break;
      //}}}
      //{{{
      case NALU_TYPE_DPC:
        cLog::log (LOGINFO, "dataPartitionC without dataPartitonA");
        break;
      //}}}

      case NALU_TYPE_AUD: break;
      case NALU_TYPE_FILL: break;
      case NALU_TYPE_EOSEQ: break;
      case NALU_TYPE_EOSTREAM: break;

      default:
        cLog::log (LOGINFO, "NALU:%d unknown:%d\n", nalu->len, nalu->unitType);
        break;
      }
    }

  return curHeader;
  }
//}}}
//{{{
static void decodeSlice (sSlice* slice) {

  bool endOfSlice = false;

  slice->codCount = -1;

  sDecoder* decoder = slice->decoder;
  if (decoder->coding.isSeperateColourPlane)
    changePlaneJV (decoder, slice->colourPlaneId, slice);
  else {
    slice->mbData = decoder->mbData;
    slice->picture = decoder->picture;
    slice->siBlock = decoder->siBlock;
    slice->predMode = decoder->predMode;
    slice->intraBlock = decoder->intraBlock;
    }

  if (slice->sliceType == eSliceB)
    computeColocated (slice, slice->listX);

  if ((slice->sliceType != eSliceI) && (slice->sliceType != eSliceSI))
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

    ercWriteMbModeMv (mb);
    endOfSlice = exitMacroblock (slice, !slice->mbAffFrame || (slice->mbIndex % 2));
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
    // get firstSlice from sliceList;
    sSlice* slice = decoder->sliceList[decoder->picSliceIndex];
    decoder->sliceList[decoder->picSliceIndex] = decoder->nextSlice;
    decoder->nextSlice = slice;

    slice = decoder->sliceList[decoder->picSliceIndex];
    useParameterSet (decoder, slice);
    initPicture (decoder, slice);

    decoder->picSliceIndex++;
    curHeader = eSOS;
    }

  while ((curHeader != eSOP) && (curHeader != eEOS)) {
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
    slice->isResetCoef = false;
    slice->isResetCoefCr = false;

    curHeader = readSlice (slice);
    slice->curHeader = curHeader;
    //{{{  manage primary, redundant slices
    if (!slice->redundantPicCount)
      decoder->isPrimaryOk = decoder->isRedundantOk = 1;

    if (!slice->redundantPicCount && (decoder->coding.sliceType != eSliceI)) {
      for (int i = 0; i < slice->numRefIndexActive[LIST_0];++i)
        if (!slice->refFlag[i]) // reference primary slice incorrect, primary slice incorrect
          decoder->isPrimaryOk = 0;
      }
    else if (slice->redundantPicCount && (decoder->coding.sliceType != eSliceI)) // reference redundant slice incorrect
      if (!slice->refFlag[slice->redundantSliceRefIndex]) // redundant slice is incorrect
        decoder->isRedundantOk = 0;

    // If primary and redundant received, primary is correct
    //   discard redundant
    // else
    //   primary slice replaced with redundant slice.
    if ((slice->frameNum == decoder->prevFrameNum) &&
        slice->redundantPicCount && decoder->isPrimaryOk && (curHeader != eEOS))
      continue;

    if (((curHeader != eSOP) && (curHeader != eEOS)) ||
        ((curHeader == eSOP) && !decoder->picSliceIndex)) {
       slice->curSliceIndex = (int16_t)decoder->picSliceIndex;
       decoder->picture->maxSliceId = (int16_t)imax (slice->curSliceIndex, decoder->picture->maxSliceId);
       if (decoder->picSliceIndex > 0) {
         copyPoc (*(decoder->sliceList), slice);
         decoder->sliceList[decoder->picSliceIndex-1]->endMbNumPlus1 = slice->startMbNum;
         }

       decoder->picSliceIndex++;
       if (decoder->picSliceIndex >= decoder->numAllocatedSlices)
         error ("decodeFrame - sliceList numAllocationSlices too small");
      curHeader = eSOS;
      }

    else {
      if (decoder->sliceList[decoder->picSliceIndex-1]->mbAffFrame)
        decoder->sliceList[decoder->picSliceIndex-1]->endMbNumPlus1 = decoder->coding.frameSizeMbs / 2;
      else
        decoder->sliceList[decoder->picSliceIndex-1]->endMbNumPlus1 =
          decoder->coding.frameSizeMbs / (decoder->sliceList[decoder->picSliceIndex-1]->fieldPic + 1);

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
  initPictureDecode (decoder);
  for (int sliceIndex = 0; sliceIndex < decoder->picSliceIndex; sliceIndex++) {
    sSlice* slice = decoder->sliceList[sliceIndex];
    curHeader = slice->curHeader;
    initSlice (decoder, slice);

    if (slice->activePps->entropyCoding) {
      //{{{  init cabac
      initCabacContexts (slice);
      cabacNewSlice (slice);
      }
      //}}}

    if (((slice->activePps->weightedBiPredIdc > 0) && (slice->sliceType == eSliceB)) ||
        (slice->activePps->hasWeightedPred && (slice->sliceType != eSliceI)))
      fillWeightedPredParam (slice);

    if (((curHeader == eSOP) || (curHeader == eSOS)) && !slice->errorFlag)
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
