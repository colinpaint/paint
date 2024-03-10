//{{{  includes
#include "global.h"
#include "elements.h"
#include "defines.h"

#include "fmo.h"
#include "vlc.h"
#include "buffer.h"
#include "biariDecode.h"
#include "sliceHeader.h"
//}}}
#define ENABLE_FIELD_CTX 1  // Enables Field mode related context types for CABAC

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

//{{{
#define IBIARI_CTX_INIT2(ii,jj,ctx,tab,num, qp) \
{ \
  for (i=0; i<ii; ++i) \
  for (j=0; j<jj; ++j) \
  { \
  biari_init_context (qp, &(ctx[i][j]), tab ## _I[num][i][j]); \
  } \
}
//}}}
//{{{
#define PBIARI_CTX_INIT2(ii,jj,ctx,tab,num, qp) \
{ \
  for (i=0; i<ii; ++i) \
  for (j=0; j<jj; ++j) \
  { \
  biari_init_context (qp, &(ctx[i][j]), tab ## _P[num][i][j]); \
  } \
}
//}}}
//{{{
#define IBIARI_CTX_INIT1(jj,ctx,tab,num, qp) \
{ \
  for (j=0; j<jj; ++j) \
  { \
  biari_init_context (qp, &(ctx[j]), tab ## _I[num][0][j]); \
  } \
}
//}}}
//{{{
#define PBIARI_CTX_INIT1(jj,ctx,tab,num, qp) \
{ \
  { \
  for (j=0; j<jj; ++j) \
  { \
  biari_init_context (qp, &(ctx[j]), tab ## _P[num][0][j]); \
  } \
  } \
}
//}}}
//}}}

//{{{
static void ref_pic_list_reordering (sSlice* curSlice) {

  byte dP_nr = assignSE2partition[curSlice->dataPartitionMode][SE_HEADER];
  sDataPartition* partition = &(curSlice->partitions[dP_nr]);
  sBitstream* curStream = partition->bitstream;

  alloc_ref_pic_list_reordering_buffer (curSlice);
  if (curSlice->sliceType != I_SLICE &&
      curSlice->sliceType != SI_SLICE) {
    int val = curSlice->ref_pic_list_reordering_flag[LIST_0] =
      readU1 ("SLC ref_pic_list_reordering_flag_l0", curStream);

    if (val) {
      int i = 0;
      do {
        val = curSlice->modification_of_pic_nums_idc[LIST_0][i] =
          readUeV("SLC modification_of_pic_nums_idc_l0", curStream);
        if (val==0 || val==1)
          curSlice->abs_diff_pic_num_minus1[LIST_0][i] =
            readUeV ("SLC abs_diff_pic_num_minus1_l0", curStream);
        else {
          if (val == 2)
            curSlice->long_term_pic_idx[LIST_0][i] =
              readUeV ("SLC long_term_pic_idx_l0", curStream);
          }
        i++;
        } while (val != 3);
      }
    }

  if (curSlice->sliceType == B_SLICE) {
    int val = curSlice->ref_pic_list_reordering_flag[LIST_1] =
      readU1 ("SLC ref_pic_list_reordering_flag_l1", curStream);
    if (val) {
      int i = 0;
      do {
        val = curSlice->modification_of_pic_nums_idc[LIST_1][i] =
          readUeV ("SLC modification_of_pic_nums_idc_l1", curStream);
        if (val == 0 || val == 1)
          curSlice->abs_diff_pic_num_minus1[LIST_1][i] =
            readUeV ("SLC abs_diff_pic_num_minus1_l1", curStream);
        else {
          if (val == 2)
            curSlice->long_term_pic_idx[LIST_1][i] =
              readUeV ("SLC long_term_pic_idx_l1", curStream);
          }
        i++;
        } while (val != 3);
      }
    }

  // set reference index of redundant slices.
  if (curSlice->redundantPicCount && (curSlice->sliceType != I_SLICE) )
    curSlice->redundantSliceRefIndex = curSlice->abs_diff_pic_num_minus1[LIST_0][0] + 1;
  }
//}}}
//{{{
static void reset_WPParam (sSlice* curSlice) {

  for (int i = 0; i < MAX_REFERENCE_PICTURES; i++) {
    for (int comp = 0; comp < 3; comp++) {
      int logWeightDenom = (comp == 0) ? curSlice->lumaLog2weightDenom :
                                           curSlice->chromaLog2weightDenom;
      curSlice->wpWeight[0][i][comp] = 1 << logWeightDenom;
      curSlice->wpWeight[1][i][comp] = 1 << logWeightDenom;
      }
    }
  }
//}}}
//{{{
static void pred_weight_table (sSlice* curSlice) {

  sDecoder* decoder = curSlice->decoder;
  sSPS* activeSPS = decoder->activeSPS;

  byte dP_nr = assignSE2partition[curSlice->dataPartitionMode][SE_HEADER];
  sDataPartition* partition = &(curSlice->partitions[dP_nr]);
  sBitstream* curStream = partition->bitstream;

  int luma_weight_flag_l0, luma_weight_flag_l1, chroma_weight_flag_l0, chroma_weight_flag_l1;

  curSlice->lumaLog2weightDenom =
    (unsigned short) readUeV ("SLC lumaLog2weightDenom", curStream);
  curSlice->wp_round_luma =
    curSlice->lumaLog2weightDenom ? 1<<(curSlice->lumaLog2weightDenom - 1) : 0;

  if (0 != activeSPS->chromaFormatIdc) {
    curSlice->chromaLog2weightDenom =
      (unsigned short) readUeV ("SLC chromaLog2weightDenom", curStream);
    curSlice->wp_round_chroma =
      curSlice->chromaLog2weightDenom ? 1<<(curSlice->chromaLog2weightDenom - 1) : 0;
    }

  reset_WPParam (curSlice);

  for (int i = 0; i < curSlice->numRefIndexActive[LIST_0]; i++) {
    luma_weight_flag_l0 = readU1 ("SLC luma_weight_flag_l0", curStream);
    if (luma_weight_flag_l0) {
      curSlice->wpWeight[LIST_0][i][0] = readSeV ("SLC luma_weight_l0", curStream);
      curSlice->wpOffset[LIST_0][i][0] = readSeV ("SLC luma_offset_l0", curStream);
      curSlice->wpOffset[LIST_0][i][0] = curSlice->wpOffset[LIST_0][i][0]<<(decoder->bitdepthLuma - 8);
      }
    else {
      curSlice->wpWeight[LIST_0][i][0] = 1 << curSlice->lumaLog2weightDenom;
      curSlice->wpOffset[LIST_0][i][0] = 0;
      }

    if (activeSPS->chromaFormatIdc != 0) {
      chroma_weight_flag_l0 = readU1 ("SLC chroma_weight_flag_l0", curStream);
      for (int j = 1; j<3; j++) {
        if (chroma_weight_flag_l0) {
          curSlice->wpWeight[LIST_0][i][j] = readSeV("SLC chroma_weight_l0", curStream);
          curSlice->wpOffset[LIST_0][i][j] = readSeV("SLC chroma_offset_l0", curStream);
          curSlice->wpOffset[LIST_0][i][j] = curSlice->wpOffset[LIST_0][i][j]<<(decoder->bitdepthChroma-8);
          }
        else {
          curSlice->wpWeight[LIST_0][i][j] = 1<<curSlice->chromaLog2weightDenom;
          curSlice->wpOffset[LIST_0][i][j] = 0;
          }
        }
      }
    }

  if ((curSlice->sliceType == B_SLICE) &&
      decoder->activePPS->weightedBiPredIdc == 1) {
    for (int i = 0; i < curSlice->numRefIndexActive[LIST_1]; i++) {
      luma_weight_flag_l1 = readU1("SLC luma_weight_flag_l1", curStream);
      if (luma_weight_flag_l1) {
        curSlice->wpWeight[LIST_1][i][0] = readSeV ("SLC luma_weight_l1", curStream);
        curSlice->wpOffset[LIST_1][i][0] = readSeV ("SLC luma_offset_l1", curStream);
        curSlice->wpOffset[LIST_1][i][0] = curSlice->wpOffset[LIST_1][i][0]<<(decoder->bitdepthLuma-8);
        }
      else {
        curSlice->wpWeight[LIST_1][i][0] = 1<<curSlice->lumaLog2weightDenom;
        curSlice->wpOffset[LIST_1][i][0] = 0;
        }

      if (activeSPS->chromaFormatIdc != 0) {
        chroma_weight_flag_l1 = readU1 ("SLC chroma_weight_flag_l1", curStream);
        for (int j = 1; j < 3; j++) {
          if (chroma_weight_flag_l1) {
            curSlice->wpWeight[LIST_1][i][j] = readSeV("SLC chroma_weight_l1", curStream);
            curSlice->wpOffset[LIST_1][i][j] = readSeV("SLC chroma_offset_l1", curStream);
            curSlice->wpOffset[LIST_1][i][j] = curSlice->wpOffset[LIST_1][i][j]<<(decoder->bitdepthChroma-8);
            }
          else {
            curSlice->wpWeight[LIST_1][i][j] = 1<<curSlice->chromaLog2weightDenom;
            curSlice->wpOffset[LIST_1][i][j] = 0;
            }
          }
        }
      }
    }
  }
//}}}

//{{{
unsigned ceilLog2 (unsigned uiVal) {

  unsigned uiRet = 0;

  unsigned uiTmp = uiVal - 1;
  while (uiTmp != 0) {
    uiTmp >>= 1;
    uiRet++;
    }

  return uiRet;
  }
//}}}
//{{{
unsigned ceilLog2sf (unsigned uiVal) {

  unsigned uiRet = 0;

  unsigned uiTmp = uiVal - 1;
  while (uiTmp > 0) {
    uiTmp >>= 1;
    uiRet++;
    }

  return uiRet;
  }
//}}}

//{{{
void initContexts (sSlice* curSlice) {

  sMotionInfoContexts*  mc = curSlice->mot_ctx;
  sTextureInfoContexts* tc = curSlice->tex_ctx;

  int i, j;
  int qp = imax(0, curSlice->qp); //decoder->qp);
  int modelNum = curSlice->modelNum;

  //--- motion coding contexts ---
  if ((curSlice->sliceType == I_SLICE)||(curSlice->sliceType == SI_SLICE)) {
    IBIARI_CTX_INIT2 (3, NUM_MB_TYPE_CTX,   mc->mb_type_contexts,     INIT_MB_TYPE,    modelNum, qp);
    IBIARI_CTX_INIT2 (2, NUM_B8_TYPE_CTX,   mc->b8_type_contexts,     INIT_B8_TYPE,    modelNum, qp);
    IBIARI_CTX_INIT2 (2, NUM_MV_RES_CTX,    mc->mv_res_contexts,      INIT_MV_RES,     modelNum, qp);
    IBIARI_CTX_INIT2 (2, NUM_REF_NO_CTX,    mc->ref_no_contexts,      INIT_REF_NO,     modelNum, qp);
    IBIARI_CTX_INIT1 (   NUM_DELTA_QP_CTX,  mc->delta_qp_contexts,    INIT_DELTA_QP,   modelNum, qp);
    IBIARI_CTX_INIT1 (   NUM_MB_AFF_CTX,    mc->mb_aff_contexts,      INIT_MB_AFF,     modelNum, qp);

    //--- texture coding contexts ---
    IBIARI_CTX_INIT1 (   NUM_TRANSFORM_SIZE_CTX, tc->transform_size_contexts, INIT_TRANSFORM_SIZE, modelNum, qp);
    IBIARI_CTX_INIT1 (                 NUM_IPR_CTX,  tc->ipr_contexts,     INIT_IPR,       modelNum, qp);
    IBIARI_CTX_INIT1 (                 NUM_CIPR_CTX, tc->cipr_contexts,    INIT_CIPR,      modelNum, qp);
    IBIARI_CTX_INIT2 (3,               NUM_CBP_CTX,  tc->cbp_contexts,     INIT_CBP,       modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_BCBP_CTX, tc->bcbp_contexts,    INIT_BCBP,      modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts[0],  INIT_MAP,       modelNum, qp);
  #if ENABLE_FIELD_CTX
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts[1],  INIT_FLD_MAP,   modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts[1], INIT_FLD_LAST,  modelNum, qp);
  #endif
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts[0], INIT_LAST,      modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ONE_CTX,  tc->one_contexts,     INIT_ONE,       modelNum, qp);
    IBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ABS_CTX,  tc->abs_contexts,     INIT_ABS,       modelNum, qp);
    }
  else {
    PBIARI_CTX_INIT2 (3, NUM_MB_TYPE_CTX,   mc->mb_type_contexts,     INIT_MB_TYPE,    modelNum, qp);
    PBIARI_CTX_INIT2 (2, NUM_B8_TYPE_CTX,   mc->b8_type_contexts,     INIT_B8_TYPE,    modelNum, qp);
    PBIARI_CTX_INIT2 (2, NUM_MV_RES_CTX,    mc->mv_res_contexts,      INIT_MV_RES,     modelNum, qp);
    PBIARI_CTX_INIT2 (2, NUM_REF_NO_CTX,    mc->ref_no_contexts,      INIT_REF_NO,     modelNum, qp);
    PBIARI_CTX_INIT1 (   NUM_DELTA_QP_CTX,  mc->delta_qp_contexts,    INIT_DELTA_QP,   modelNum, qp);
    PBIARI_CTX_INIT1 (   NUM_MB_AFF_CTX,    mc->mb_aff_contexts,      INIT_MB_AFF,     modelNum, qp);

    //--- texture coding contexts ---
    PBIARI_CTX_INIT1 (   NUM_TRANSFORM_SIZE_CTX, tc->transform_size_contexts, INIT_TRANSFORM_SIZE, modelNum, qp);
    PBIARI_CTX_INIT1 (                 NUM_IPR_CTX,  tc->ipr_contexts,     INIT_IPR,       modelNum, qp);
    PBIARI_CTX_INIT1 (                 NUM_CIPR_CTX, tc->cipr_contexts,    INIT_CIPR,      modelNum, qp);
    PBIARI_CTX_INIT2 (3,               NUM_CBP_CTX,  tc->cbp_contexts,     INIT_CBP,       modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_BCBP_CTX, tc->bcbp_contexts,    INIT_BCBP,      modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts[0],  INIT_MAP,       modelNum, qp);
  #if ENABLE_FIELD_CTX
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts[1],  INIT_FLD_MAP,   modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts[1], INIT_FLD_LAST,  modelNum, qp);
  #endif
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts[0], INIT_LAST,      modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ONE_CTX,  tc->one_contexts,     INIT_ONE,       modelNum, qp);
    PBIARI_CTX_INIT2 (NUM_BLOCK_TYPES, NUM_ABS_CTX,  tc->abs_contexts,     INIT_ABS,       modelNum, qp);
    }
  }
//}}}
//{{{
void dec_ref_pic_marking (sDecoder* decoder, sBitstream* curStream, sSlice* pSlice) {

  // free old buffer content
  while (pSlice->decRefPicMarkingBuffer) {
    sDecodedRefPicMarking* tmp_drpm = pSlice->decRefPicMarkingBuffer;
    pSlice->decRefPicMarkingBuffer = tmp_drpm->next;
    free (tmp_drpm);
    }

  if (pSlice->idrFlag) {
    pSlice->noOutputPriorPicFlag =
      readU1 ("SLC noOutputPriorPicFlag", curStream);
    decoder->noOutputPriorPicFlag = pSlice->noOutputPriorPicFlag;
    pSlice->longTermRefFlag =
      readU1 ("SLC longTermRefFlag", curStream);
    }
  else {
    pSlice->adaptiveRefPicBufferingFlag =
      readU1 ("SLC adaptiveRefPicBufferingFlag", curStream);
    if (pSlice->adaptiveRefPicBufferingFlag) {
      // read Memory Management Control Operation
      int val;
      do {
        sDecodedRefPicMarking* tmp_drpm = (sDecodedRefPicMarking*)calloc (1,sizeof (sDecodedRefPicMarking));
        tmp_drpm->next = NULL;
        val = tmp_drpm->memManagement =
          readUeV ("SLC memManagement", curStream);
        if ((val == 1) || (val == 3))
          tmp_drpm->diffPicNumMinus1 =
            readUeV ("SLC diffPicNumMinus1", curStream);
        if (val==2)
          tmp_drpm->longTermPicNum =
            readUeV ("SLC longTermPicNum", curStream);

        if ((val == 3 ) || (val == 6))
          tmp_drpm->longTermFrameIndex =
            readUeV ("SLC longTermFrameIndex", curStream);
        if (val == 4)
          tmp_drpm->maxLongTermFrameIndexPlus1 =
            readUeV ("SLC max_long_term_pic_idx_plus1", curStream);

        // add command
        if (pSlice->decRefPicMarkingBuffer == NULL)
          pSlice->decRefPicMarkingBuffer = tmp_drpm;
        else {
          sDecodedRefPicMarking* tmp_drpm2 = pSlice->decRefPicMarkingBuffer;
          while (tmp_drpm2->next != NULL) tmp_drpm2 = tmp_drpm2->next;
          tmp_drpm2->next = tmp_drpm;
          }
        } while (val != 0);
      }
    }
  }
//}}}

//{{{
int dumpPOC (sDecoder* decoder) {

  sSPS* activeSPS = decoder->activeSPS;

  printf ("POC locals...\n");
  printf ("topPoc                                %d\n", (int) decoder->sliceList[0]->topPoc);
  printf ("botPoc                             %d\n", (int) decoder->sliceList[0]->botPoc);
  printf ("frameNum                             %d\n", (int) decoder->sliceList[0]->frameNum);
  printf ("fieldPicFlag                        %d\n", (int) decoder->sliceList[0]->fieldPicFlag);
  printf ("botFieldFlag                     %d\n", (int) decoder->sliceList[0]->botFieldFlag);

  printf ("POC SPS\n");
  printf ("log2_max_frame_num_minus4             %d\n", (int) activeSPS->log2_max_frame_num_minus4);         // POC200301
  printf ("log2_max_pic_order_cnt_lsb_minus4     %d\n", (int) activeSPS->log2_max_pic_order_cnt_lsb_minus4);
  printf ("picOrderCountType                    %d\n", (int) activeSPS->picOrderCountType);
  printf ("num_ref_frames_in_pic_order_cnt_cycle %d\n", (int) activeSPS->num_ref_frames_in_pic_order_cnt_cycle);
  printf ("delta_pic_order_always_zero_flag      %d\n", (int) activeSPS->delta_pic_order_always_zero_flag);
  printf ("offset_for_non_ref_pic                %d\n", (int) activeSPS->offset_for_non_ref_pic);
  printf ("offset_for_top_to_bottom_field        %d\n", (int) activeSPS->offset_for_top_to_bottom_field);
  printf ("offset_for_ref_frame[0]               %d\n", (int) activeSPS->offset_for_ref_frame[0]);
  printf ("offset_for_ref_frame[1]               %d\n", (int) activeSPS->offset_for_ref_frame[1]);

  printf ("POC in SLice Header\n");
  printf ("botFieldPicOrderFramePresentFlag %d\n", (int) decoder->activePPS->botFieldPicOrderFramePresentFlag);
  printf ("deltaPicOrderCount[0]                %d\n", (int) decoder->sliceList[0]->deltaPicOrderCount[0]);
  printf ("deltaPicOrderCount[1]                %d\n", (int) decoder->sliceList[0]->deltaPicOrderCount[1]);
  printf ("idrFlag                              %d\n", (int) decoder->sliceList[0]->idrFlag);
  printf ("maxFrameNum                         %d\n", (int) decoder->maxFrameNum);

  return 0;
  }
//}}}
//{{{
void decodePOC (sDecoder* decoder, sSlice* pSlice) {

  // for POC mode 0:
  sSPS* activeSPS = decoder->activeSPS;
  unsigned int MaxPicOrderCntLsb = (1<<(activeSPS->log2_max_pic_order_cnt_lsb_minus4+4));

  switch (activeSPS->picOrderCountType) {
    //{{{
    case 0: // POC MODE 0
      // 1st
      if (pSlice->idrFlag) {
        decoder->PrevPicOrderCntMsb = 0;
        decoder->PrevPicOrderCntLsb = 0;
        }
      else {
        if (decoder->last_has_mmco_5) {
          if (decoder->last_pic_bottom_field) {
            decoder->PrevPicOrderCntMsb = 0;
            decoder->PrevPicOrderCntLsb = 0;
            }
          else {
            decoder->PrevPicOrderCntMsb = 0;
            decoder->PrevPicOrderCntLsb = pSlice->topPoc;
            }
          }
        }

      // Calculate the MSBs of current picture
      if( pSlice->picOrderCountLsb  <  decoder->PrevPicOrderCntLsb  &&
        ( decoder->PrevPicOrderCntLsb - pSlice->picOrderCountLsb )  >=  ( MaxPicOrderCntLsb / 2 ) )
        pSlice->PicOrderCntMsb = decoder->PrevPicOrderCntMsb + MaxPicOrderCntLsb;
      else if ( pSlice->picOrderCountLsb  >  decoder->PrevPicOrderCntLsb  &&
        ( pSlice->picOrderCountLsb - decoder->PrevPicOrderCntLsb )  >  ( MaxPicOrderCntLsb / 2 ) )
        pSlice->PicOrderCntMsb = decoder->PrevPicOrderCntMsb - MaxPicOrderCntLsb;
      else
        pSlice->PicOrderCntMsb = decoder->PrevPicOrderCntMsb;

      // 2nd
      if (pSlice->fieldPicFlag==0) {
        //frame pix
        pSlice->topPoc = pSlice->PicOrderCntMsb + pSlice->picOrderCountLsb;
        pSlice->botPoc = pSlice->topPoc + pSlice->deletaPicOrderCountBot;
        pSlice->thisPoc = pSlice->framePoc = (pSlice->topPoc < pSlice->botPoc)? pSlice->topPoc : pSlice->botPoc; // POC200301
        }
      else if (pSlice->botFieldFlag == FALSE) // top field
        pSlice->thisPoc= pSlice->topPoc = pSlice->PicOrderCntMsb + pSlice->picOrderCountLsb;
      else // bottom field
        pSlice->thisPoc= pSlice->botPoc = pSlice->PicOrderCntMsb + pSlice->picOrderCountLsb;
      pSlice->framePoc = pSlice->thisPoc;

      decoder->thisPoc = pSlice->thisPoc;
      decoder->PreviousFrameNum = pSlice->frameNum;

      if(pSlice->refId) {
        decoder->PrevPicOrderCntLsb = pSlice->picOrderCountLsb;
        decoder->PrevPicOrderCntMsb = pSlice->PicOrderCntMsb;
        }

      break;
    //}}}
    //{{{
    case 1: // POC MODE 1
      // 1st
      if (pSlice->idrFlag) {
        decoder->FrameNumOffset=0;     //  first pix of IDRGOP,
        if (pSlice->frameNum)
          error ("frameNum not equal to zero in IDR picture", -1020);
        }
      else {
        if (decoder->last_has_mmco_5) {
          decoder->PreviousFrameNumOffset = 0;
          decoder->PreviousFrameNum = 0;
          }
        if (pSlice->frameNum<decoder->PreviousFrameNum)
          //not first pix of IDRGOP
          decoder->FrameNumOffset = decoder->PreviousFrameNumOffset + decoder->maxFrameNum;
        else
          decoder->FrameNumOffset = decoder->PreviousFrameNumOffset;
        }

      // 2nd
      if (activeSPS->num_ref_frames_in_pic_order_cnt_cycle)
        pSlice->AbsFrameNum = decoder->FrameNumOffset+pSlice->frameNum;
      else
        pSlice->AbsFrameNum=0;
      if ((!pSlice->refId) && pSlice->AbsFrameNum > 0)
        pSlice->AbsFrameNum--;

      // 3rd
      decoder->ExpectedDeltaPerPicOrderCntCycle = 0;
      if (activeSPS->num_ref_frames_in_pic_order_cnt_cycle)
        for (int i = 0; i < (int) activeSPS->num_ref_frames_in_pic_order_cnt_cycle;i++)
          decoder->ExpectedDeltaPerPicOrderCntCycle += activeSPS->offset_for_ref_frame[i];

      if (pSlice->AbsFrameNum) {
        decoder->PicOrderCntCycleCnt =
          (pSlice->AbsFrameNum-1)/activeSPS->num_ref_frames_in_pic_order_cnt_cycle;
        decoder->FrameNumInPicOrderCntCycle =
          (pSlice->AbsFrameNum-1)%activeSPS->num_ref_frames_in_pic_order_cnt_cycle;
        decoder->ExpectedPicOrderCnt =
          decoder->PicOrderCntCycleCnt*decoder->ExpectedDeltaPerPicOrderCntCycle;
        for (int i = 0; i <= (int)decoder->FrameNumInPicOrderCntCycle; i++)
          decoder->ExpectedPicOrderCnt += activeSPS->offset_for_ref_frame[i];
        }
      else
        decoder->ExpectedPicOrderCnt=0;

      if (!pSlice->refId)
        decoder->ExpectedPicOrderCnt += activeSPS->offset_for_non_ref_pic;

      if (pSlice->fieldPicFlag == 0) {
        // frame pix
        pSlice->topPoc = decoder->ExpectedPicOrderCnt + pSlice->deltaPicOrderCount[0];
        pSlice->botPoc = pSlice->topPoc + activeSPS->offset_for_top_to_bottom_field + pSlice->deltaPicOrderCount[1];
        pSlice->thisPoc = pSlice->framePoc = (pSlice->topPoc < pSlice->botPoc)? pSlice->topPoc : pSlice->botPoc; // POC200301
        }
      else if (pSlice->botFieldFlag == FALSE)
        // top field
        pSlice->thisPoc = pSlice->topPoc = decoder->ExpectedPicOrderCnt + pSlice->deltaPicOrderCount[0];
      else
        // bottom field
        pSlice->thisPoc = pSlice->botPoc = decoder->ExpectedPicOrderCnt + activeSPS->offset_for_top_to_bottom_field + pSlice->deltaPicOrderCount[0];
      pSlice->framePoc=pSlice->thisPoc;

      decoder->PreviousFrameNum=pSlice->frameNum;
      decoder->PreviousFrameNumOffset=decoder->FrameNumOffset;
      break;
    //}}}
    //{{{
    case 2: // POC MODE 2
      if (pSlice->idrFlag) {
        // IDR picture, first pix of IDRGOP,
        decoder->FrameNumOffset = 0;
        pSlice->thisPoc = pSlice->framePoc = pSlice->topPoc = pSlice->botPoc = 0;
        if (pSlice->frameNum)
          error ("frameNum not equal to zero in IDR picture", -1020);
        }
      else {
        if (decoder->last_has_mmco_5) {
          decoder->PreviousFrameNum = 0;
          decoder->PreviousFrameNumOffset = 0;
          }

        if (pSlice->frameNum<decoder->PreviousFrameNum)
          decoder->FrameNumOffset = decoder->PreviousFrameNumOffset + decoder->maxFrameNum;
        else
          decoder->FrameNumOffset = decoder->PreviousFrameNumOffset;

        pSlice->AbsFrameNum = decoder->FrameNumOffset+pSlice->frameNum;
        if (!pSlice->refId)
          pSlice->thisPoc = (2*pSlice->AbsFrameNum - 1);
        else
          pSlice->thisPoc = (2*pSlice->AbsFrameNum);

        if (pSlice->fieldPicFlag==0)
          pSlice->topPoc = pSlice->botPoc = pSlice->framePoc = pSlice->thisPoc;
        else if (pSlice->botFieldFlag == FALSE)
          pSlice->topPoc = pSlice->framePoc = pSlice->thisPoc;
        else
          pSlice->botPoc = pSlice->framePoc = pSlice->thisPoc;
        }

      decoder->PreviousFrameNum=pSlice->frameNum;
      decoder->PreviousFrameNumOffset=decoder->FrameNumOffset;
      break;
    //}}}
    //{{{
    default:
      // error must occurs
      assert( 1==0 );
      break;
    //}}}
    }
  }
//}}}

//{{{
void readSliceHeader (sSlice* curSlice) {

  sDecoder* decoder = curSlice->decoder;
  byte dP_nr = assignSE2partition[curSlice->dataPartitionMode][SE_HEADER];
  sDataPartition* partition = &(curSlice->partitions[dP_nr]);
  sBitstream* curStream = partition->bitstream;

  // Get first_mb_in_slice
  curSlice->startMbNum = readUeV ("SLC first_mb_in_slice", curStream);

  int tmp = readUeV ("SLC sliceType", curStream);
  if (tmp > 4)
    tmp -= 5;
  decoder->type = curSlice->sliceType = (eSliceType)tmp;

  curSlice->ppsId = readUeV ("SLC ppsId", curStream);

  if (decoder->sepColourPlaneFlag)
    curSlice->colourPlaneId = readUv (2, "SLC colourPlaneId", curStream);
  else
    curSlice->colourPlaneId = PLANE_Y;
  }
//}}}
//{{{
void readRestSliceHeader (sSlice* curSlice) {

  sDecoder* decoder = curSlice->decoder;
  sSPS* activeSPS = decoder->activeSPS;

  byte dP_nr = assignSE2partition[curSlice->dataPartitionMode][SE_HEADER];
  sDataPartition* partition = &(curSlice->partitions[dP_nr]);
  sBitstream* curStream = partition->bitstream;

  int val, len;
  curSlice->frameNum = readUv (activeSPS->log2_max_frame_num_minus4 + 4, "SLC frameNum", curStream);

  // Tian Dong: frameNum gap processing, if found
  if (curSlice->idrFlag) {
    decoder->preFrameNum = curSlice->frameNum;
    // picture error conceal
    decoder->lastRefPicPoc = 0;
    assert(curSlice->frameNum == 0);
    }

  if (activeSPS->frameMbOnlyFlag) {
    decoder->structure = FRAME;
    curSlice->fieldPicFlag=0;
    }
  else {
    // fieldPicFlag   u(1)
    curSlice->fieldPicFlag = readU1 ("SLC fieldPicFlag", curStream);
    if (curSlice->fieldPicFlag) {
      // botFieldFlag  u(1)
      curSlice->botFieldFlag = (byte)readU1 ("SLC botFieldFlag", curStream);
      decoder->structure = curSlice->botFieldFlag ? BotField : TopField;
      }
    else {
      decoder->structure = FRAME;
      curSlice->botFieldFlag = FALSE;
      }
    }

  curSlice->structure = (ePicStructure) decoder->structure;
  curSlice->mbAffFrameFlag =
    (activeSPS->mb_adaptive_frame_field_flag && (curSlice->fieldPicFlag==0));

  if (curSlice->structure == FRAME)
    assert (curSlice->fieldPicFlag == 0);
  if (curSlice->structure == TopField)
    assert (curSlice->fieldPicFlag == 1 && (curSlice->botFieldFlag == FALSE));
  if (curSlice->structure == BotField)
    assert (curSlice->fieldPicFlag == 1 && (curSlice->botFieldFlag == TRUE ));

  if (curSlice->idrFlag)
    curSlice->idrPicId = readUeV ("SLC idrPicId", curStream);

  if (activeSPS->picOrderCountType == 0) {
    curSlice->picOrderCountLsb = readUv (activeSPS->log2_max_pic_order_cnt_lsb_minus4 + 4,
                                             "SLC picOrderCountLsb", curStream);
    if (decoder->activePPS->botFieldPicOrderFramePresentFlag  == 1 &&
        !curSlice->fieldPicFlag )
      curSlice->deletaPicOrderCountBot = readSeV ("SLC deletaPicOrderCountBot", curStream);
    else
      curSlice->deletaPicOrderCountBot = 0;
    }

  if (activeSPS->picOrderCountType == 1) {
    if (!activeSPS->delta_pic_order_always_zero_flag) {
      curSlice->deltaPicOrderCount[0] = readSeV ("SLC deltaPicOrderCount[0]", curStream);
      if (decoder->activePPS->botFieldPicOrderFramePresentFlag == 1 &&
          !curSlice->fieldPicFlag)
        curSlice->deltaPicOrderCount[1] = readSeV ("SLC deltaPicOrderCount[1]", curStream);
      else
        curSlice->deltaPicOrderCount[1] = 0;  // set to zero if not in stream
      }
    else {
      curSlice->deltaPicOrderCount[0] = 0;
      curSlice->deltaPicOrderCount[1] = 0;
      }
    }

  // redundantPicCount is missing here
  if (decoder->activePPS->redundantPicCountPresentFlag)
    curSlice->redundantPicCount = readUeV ("SLC redundantPicCount", curStream);

  if (curSlice->sliceType == B_SLICE)
    curSlice->directSpatialMvPredFlag = readU1 ("SLC directSpatialMvPredFlag", curStream);

  curSlice->numRefIndexActive[LIST_0] = decoder->activePPS->numRefIndexL0defaultActiveMinus1 + 1;
  curSlice->numRefIndexActive[LIST_1] = decoder->activePPS->numRefIndexL1defaultActiveMinus1 + 1;

  if (curSlice->sliceType == P_SLICE ||
      curSlice->sliceType == SP_SLICE ||
      curSlice->sliceType == B_SLICE) {
    val = readU1 ("SLC num_ref_idx_override_flag", curStream);
    if (val) {
      curSlice->numRefIndexActive[LIST_0] = 1 + readUeV ("SLC num_ref_idx_l0_active_minus1", curStream);
      if (curSlice->sliceType == B_SLICE)
        curSlice->numRefIndexActive[LIST_1] = 1 + readUeV ("SLC num_ref_idx_l1_active_minus1", curStream);
      }
    }

  if (curSlice->sliceType != B_SLICE)
    curSlice->numRefIndexActive[LIST_1] = 0;

  ref_pic_list_reordering (curSlice);

  curSlice->weightedPredFlag =
    (unsigned short)((curSlice->sliceType == P_SLICE || curSlice->sliceType == SP_SLICE)
      ? decoder->activePPS->weightedPredFlag
      : (curSlice->sliceType == B_SLICE && decoder->activePPS->weightedBiPredIdc == 1));
  curSlice->weightedBiPredIdc = (unsigned short)(curSlice->sliceType == B_SLICE &&
                                                    decoder->activePPS->weightedBiPredIdc > 0);

  if ((decoder->activePPS->weightedPredFlag &&
      (curSlice->sliceType == P_SLICE || curSlice->sliceType == SP_SLICE)) ||
      (decoder->activePPS->weightedBiPredIdc == 1 && (curSlice->sliceType == B_SLICE)))
    pred_weight_table (curSlice);

  if (curSlice->refId)
    dec_ref_pic_marking (decoder, curStream, curSlice);

  if (decoder->activePPS->entropyCodingModeFlag &&
      curSlice->sliceType != I_SLICE &&
      curSlice->sliceType != SI_SLICE)
    curSlice->modelNum = readUeV ("SLC cabac_init_idc", curStream);
  else
    curSlice->modelNum = 0;

  curSlice->sliceQpDelta = val = readSeV ("SLC sliceQpDelta", curStream);
  curSlice->qp = 26 + decoder->activePPS->picInitQpMinus26 + val;

  if ((curSlice->qp < -decoder->bitdepth_luma_qp_scale) || (curSlice->qp > 51))
    error ("sliceQpDelta makes slice_qp_y out of range", 500);

  if (curSlice->sliceType == SP_SLICE || curSlice->sliceType == SI_SLICE) {
    if (curSlice->sliceType == SP_SLICE)
      curSlice->spSwitch = readU1 ("SLC sp_for_switch_flag", curStream);
    curSlice->sliceQsDelta = val = readSeV ("SLC sliceQsDelta", curStream);
    curSlice->qs = 26 + decoder->activePPS->picInitQsMinus26 + val;
    if ((curSlice->qs < 0) || (curSlice->qs > 51))
      error ("sliceQsDelta makes slice_qs_y out of range", 500);
    }

  //printf ("deblockingFilterControlPresentFlag:%d\n",
  //        decoder->activePPS->deblockingFilterControlPresentFlag);
  if (decoder->activePPS->deblockingFilterControlPresentFlag) {
    curSlice->DFDisableIdc = (short) readUeV ("SLC disable_deblocking_filter_idc", curStream);
    if (curSlice->DFDisableIdc != 1) {
      curSlice->DFAlphaC0Offset = (short)(2 * readSeV ("SLC slice_alpha_c0_offset_div2", curStream));
      curSlice->DFBetaOffset = (short)(2 * readSeV ("SLC slice_beta_offset_div2", curStream));
      }
    else
      curSlice->DFAlphaC0Offset = curSlice->DFBetaOffset = 0;
    }
  else
    curSlice->DFDisableIdc = curSlice->DFAlphaC0Offset = curSlice->DFBetaOffset = 0;

  //printf ("sSlice:%d, DFParameters:(%d,%d,%d)\n\n",
  //        curSlice->curSliceNum, curSlice->DFDisableIdc, curSlice->DFAlphaC0Offset, curSlice->DFBetaOffset);

  // The conformance point for intra profiles is without deblocking, but decoders are still recommended to filter the output.
  // We allow in the decoder config to skip the loop filtering. This is achieved by modifying the parameters here.
  if (isHiIntraOnlyProfile (activeSPS->profileIdc, activeSPS->constrained_set3_flag) &&
      (decoder->param.intraProfileDeblocking == 0)) {
    curSlice->DFDisableIdc = 1;
    curSlice->DFAlphaC0Offset = curSlice->DFBetaOffset = 0;
    }

  if (decoder->activePPS->numSliceGroupsMinus1>0 && decoder->activePPS->sliceGroupMapType>=3 &&
      decoder->activePPS->sliceGroupMapType <= 5) {
    len = (activeSPS->pic_height_in_map_units_minus1+1) * (activeSPS->pic_width_in_mbs_minus1+1)/
          (decoder->activePPS->sliceGroupChangeRateMius1 + 1);
    if (((activeSPS->pic_height_in_map_units_minus1+1) * (activeSPS->pic_width_in_mbs_minus1+1))%
          (decoder->activePPS->sliceGroupChangeRateMius1 + 1))
      len += 1;

    len = ceilLog2 (len+1);
    curSlice->sliceGroupChangeCycle = readUv (len, "SLC sliceGroupChangeCycle", curStream);
    }

  decoder->picHeightInMbs = decoder->FrameHeightInMbs / ( 1 + curSlice->fieldPicFlag );
  decoder->picSizeInMbs   = decoder->PicWidthInMbs * decoder->picHeightInMbs;
  decoder->FrameSizeInMbs = decoder->PicWidthInMbs * decoder->FrameHeightInMbs;
  }
//}}}
