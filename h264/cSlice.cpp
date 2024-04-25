//{{{  includes
#include "global.h"
#include "memory.h"

#include "cabac.h"
#include "macroblock.h"
#include "mcPred.h"
#include "transform.h"

#include "../common/cLog.h"

using namespace std;
//}}}
namespace {
  //{{{  tables
  //{{{
  int kQuantOrg[16] = {
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16
    };
  //}}}
  //{{{
  int kQuant8Org[64] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16
    };
  //}}}
  //{{{
  int kQuantIntraDefault[16] = {
     6, 13, 20, 28,
    13, 20, 28, 32,
    20, 28, 32, 37,
    28, 32, 37, 42
    };
  //}}}
  //{{{
  int kQuantInterDefault[16] = {
    10, 14, 20, 24,
    14, 20, 24, 27,
    20, 24, 27, 30,
    24, 27, 30, 34
    };
  //}}}
  //{{{
  int kQuant8IntraDefault[64] = {
     6,10,13,16,18,23,25,27,
    10,11,16,18,23,25,27,29,
    13,16,18,23,25,27,29,31,
    16,18,23,25,27,29,31,33,
    18,23,25,27,29,31,33,36,
    23,25,27,29,31,33,36,38,
    25,27,29,31,33,36,38,40,
    27,29,31,33,36,38,40,42
    };
  //}}}
  //{{{
  int kQuant8InterDefault[64] = {
     9,13,15,17,19,21,22,24,
    13,13,17,19,21,22,24,25,
    15,17,19,21,22,24,25,27,
    17,19,21,22,24,25,27,28,
    19,21,22,24,25,27,28,30,
    21,22,24,25,27,28,30,32,
    22,24,25,27,28,30,32,33,
    24,25,27,28,30,32,33,35
    };
  //}}}
  //}}}
  //{{{  tables
  #define CTX_UNUSED {0,64}
  #define CTX_UNDEF  {0,63}
  #define NUM_CTX_MODELS_I 1
  #define NUM_CTX_MODELS_P 3

  //{{{
  const char INIT_MB_TYPE_I[1][3][11][2] =
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
  const char INIT_MB_TYPE_P[3][3][11][2] =
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
  const char INIT_B8_TYPE_I[1][2][9][2] =
  {
    //----- model 0 -----
    {
      {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
      {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
    }
  };
  //}}}
  //{{{
  const char INIT_B8_TYPE_P[3][2][9][2] =
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
  const char INIT_MV_RES_I[1][2][10][2] =
  {
    //----- model 0 -----
    {
      {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
      {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
    }
  };
  //}}}
  //{{{
  const char INIT_MV_RES_P[3][2][10][2] =
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
  const char INIT_REF_NO_I[1][2][6][2] =
  {
    //----- model 0 -----
    {
      {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED },
      {  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED ,  CTX_UNUSED }
    }
  };
  //}}}
  //{{{
  const char INIT_REF_NO_P[3][2][6][2] =
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
  const char INIT_TRANSFORM_SIZE_I[1][1][3][2]=
  {
    //----- model 0 -----
    {
      {  {  31,  21} , {  31,  31} , {  25,  50} },
  //    { {   0,  41} , {   0,  63} , {   0,  63} },
    }
  };
  //}}}
  //{{{
  const char INIT_TRANSFORM_SIZE_P[3][1][3][2]=
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
  const char INIT_DELTA_QP_I[1][1][4][2]=
  {
    //----- model 0 -----
    {
      { {   0,  41} , {   0,  63} , {   0,  63} , {   0,  63} },
    }
  };
  //}}}
  //{{{
  const char INIT_DELTA_QP_P[3][1][4][2]=
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
  const char INIT_MB_AFF_I[1][1][4][2] =
  {
    //----- model 0 -----
    {
      { {   0,  11} , {   1,  55} , {   0,  69} ,  CTX_UNUSED }
    }
  };
  //}}}
  //{{{
  const char INIT_MB_AFF_P[3][1][4][2] =
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
  const char INIT_IPR_I[1][1][2][2] =
  {
    //----- model 0 -----
    {
      { { 13,  41} , {   3,  62} }
    }
  };
  //}}}
  //{{{
  const char INIT_IPR_P[3][1][2][2] =
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
  const char INIT_CIPR_I[1][1][4][2] =
  {
    //----- model 0 -----
    {
      { {  -9,  83} , {   4,  86} , {   0,  97} , {  -7,  72} }
    }
  };
  //}}}
  //{{{
  const char INIT_CIPR_P[3][1][4][2] =
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
  const char INIT_CBP_I[1][3][4][2] =
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
  const char INIT_CBP_P[3][3][4][2] =
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
  const char INIT_BCBP_I[1][22][4][2] =
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
  const char INIT_BCBP_P[3][22][4][2] =
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
  const char INIT_MAP_I[1][22][15][2] =
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
  const char INIT_MAP_P[3][22][15][2] =
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
  const char INIT_LAST_I[1][22][15][2] =
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
  const char INIT_LAST_P[3][22][15][2] =
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
  const char INIT_ONE_I[1][22][5][2] =
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
  const char INIT_ONE_P[3][22][5][2] =
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
  const char INIT_ABS_I[1][22][5][2] =
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
  const char INIT_ABS_P[3][22][5][2] =
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
  const char INIT_FLD_MAP_I[1][22][15][2] =
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
  const char INIT_FLD_MAP_P[3][22][15][2] =
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
  const char INIT_FLD_LAST_I[1][22][15][2] =
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
  const char INIT_FLD_LAST_P[3][22][15][2] =
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
  void setDequant4x4 (int (*InvLevelScale4x4)[4], const int (*dequant)[4], int* qmatrix) {

    for (int j = 0; j < 4; j++) {
      *(*InvLevelScale4x4) = *(*dequant) * *qmatrix++;
      *(*InvLevelScale4x4 + 1) = *(*dequant + 1) * *qmatrix++;
      *(*InvLevelScale4x4 + 2) = *(*dequant + 2) * *qmatrix++;
      *(*InvLevelScale4x4++ + 3) = *(*dequant++ + 3) * *qmatrix++;
      }
    }
  //}}}
  //{{{
  void setDequant8x8 (int (*InvLevelScale8x8)[8], const int (*dequant)[8], int* qmatrix) {

    for (int j = 0; j < 8; j++) {
      *(*InvLevelScale8x8) = *(*dequant      ) * *qmatrix++;
      *(*InvLevelScale8x8 + 1) = *(*dequant + 1) * *qmatrix++;
      *(*InvLevelScale8x8 + 2) = *(*dequant + 2) * *qmatrix++;
      *(*InvLevelScale8x8 + 3) = *(*dequant + 3) * *qmatrix++;
      *(*InvLevelScale8x8 + 4) = *(*dequant + 4) * *qmatrix++;
      *(*InvLevelScale8x8 + 5) = *(*dequant + 5) * *qmatrix++;
      *(*InvLevelScale8x8 + 6) = *(*dequant  + 6) * *qmatrix++;
      *(*InvLevelScale8x8++ + 7) = *(*dequant++ + 7) * *qmatrix++;
      }
    }
  //}}}

  //{{{
  void readCbp400cabac (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int k;
    int level;
    int codedBlockPattern;
    const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int i0, j0;
    int qp_per, qp_rem;
    int intra = (mb->isIntraBlock == true);
    int need_transform_sizeFlag;
    int (*InvLevelScale4x4)[4] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    sSyntaxElement se;
    sDataPartition *dataPartition = NULL;
    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB) ? SE_CBP_INTRA : SE_CBP_INTER;

      dataPartition = &(slice->dataPartitionArray[dpMap[se.type]]);
      if (dataPartition->mBitStream.mError)
        se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
          ? slice->infoCbpIntra : slice->infoCbpInter;
      else
        se.reading = readCbpCabac;

      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      // Transform size flag for INTER MBs
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type =  SE_HEADER;
        dataPartition = &slice->dataPartitionArray[dpMap[SE_HEADER]];
        se.reading = readMbTransformSizeCabac;

        // read eCavlc transform_size_8x8Flag
        if (dataPartition->mBitStream.mError) {
          se.len = 1;
          dataPartition->mBitStream.readSyntaxElementFLC (&se);
          }
        else
          dataPartition->readSyntaxElement(mb, &se, dataPartition);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }

      // Delta quant only if nonzero coeffs
      if (codedBlockPattern != 0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);
        if (slice->dataPartitionMode)  {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if (intra && slice->noDataPartitionB)  {
            mb->mError = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
          }
        }
      }
    else {
      //{{{  read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);

      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->mError  = 1;
          mb->dplFlag = 1;
         }
         checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];

        {
          se.type = SE_LUM_DC_INTRA;
          dataPartition = &(slice->dataPartitionArray[dpMap[se.type]]);

          se.context      = LUMA_16DC;
          se.type         = SE_LUM_DC_INTRA;

          if (dataPartition->mBitStream.mError)
            se.mapping = sBitStream::infoLevelRunInter;
          else
            se.reading = readRunLevelCabac;

          level = 1;                            // just to get inside the loop

          for(k = 0; (k < 17) && (level != 0); ++k) {
            dataPartition->readSyntaxElement(mb, &se, dataPartition);
            level = se.value1;

            if (level != 0) {
              /* leave if level == 0 */
              pos_scan_4x4 += (2 * se.value2);
              i0 = ((*pos_scan_4x4++) << 2);
              j0 = ((*pos_scan_4x4++) << 2);
              slice->cof[0][j0][i0] = level;// add new intra DC coeff
              }
            }
          }
        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }
      //}}}

    updateQp (mb, slice->qp);

    qp_per = decoder->qpPerMatrix[ mb->qpScaled[PLANE_Y] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[PLANE_Y] ];

    if (codedBlockPattern)  {
      if (mb->lumaTransformSize8x8flag) // 8x8 transform size & eCabac
        mb->readCompCoef8x8cabac (mb, &se, PLANE_Y);
      else {
        InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
        mb->readCompCoef4x4cabac (mb, &se, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern);
        }
      }
    }
  //}}}
  //{{{
  void readCbp444cabac (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int i, k;
    int level;
    int codedBlockPattern;
    const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int coef_ctr, i0, j0;
    int qp_per, qp_rem;
    int uv;
    int qp_per_uv[2];
    int qp_rem_uv[2];
    int intra = (mb->isIntraBlock == true);
    int need_transform_sizeFlag;
    int (*InvLevelScale4x4)[4] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // QPI init constants for every chroma qp offset
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    // read CBP if not new intra mode
    sSyntaxElement se;
    sDataPartition* dataPartition = NULL;
    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? SE_CBP_INTRA : SE_CBP_INTER;

      dataPartition = &(slice->dataPartitionArray[dpMap[se.type]]);
      if (dataPartition->mBitStream.mError)
        se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
          ? slice->infoCbpIntra : slice->infoCbpInter;
      else
        se.reading = readCbpCabac;

      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      // Transform size flag for INTER MBs
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type =  SE_HEADER;
        dataPartition = &slice->dataPartitionArray[dpMap[SE_HEADER]];
        se.reading = readMbTransformSizeCabac;

        // read eCavlc transform_size_8x8Flag
        if (dataPartition->mBitStream.mError) {
          se.len = 1;
          dataPartition->mBitStream.readSyntaxElementFLC (&se);
          }
        else
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }

      if (codedBlockPattern !=0 ) {
        readDeltaQuant (&se, dataPartition, mb, dpMap,
                        ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if (intra && slice->noDataPartitionB ) {
            mb->mError = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
         }
        }
      }
    else {
      // read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);
      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->mError  = 1;
          mb->dplFlag = 1;
          }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];

        {
          se.type = SE_LUM_DC_INTRA;
          dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
          se.context = LUMA_16DC;
          se.type = SE_LUM_DC_INTRA;

          if (dataPartition->mBitStream.mError)
            se.mapping = sBitStream::infoLevelRunInter;
          else
            se.reading = readRunLevelCabac;

          level = 1;                            // just to get inside the loop
          for (k = 0; (k < 17) && (level != 0); ++k) {
            dataPartition->readSyntaxElement (mb, &se, dataPartition);
            level = se.value1;

            if (level != 0) {
              pos_scan_4x4 += (2 * se.value2);
              i0 = ((*pos_scan_4x4++) << 2);
              j0 = ((*pos_scan_4x4++) << 2);
              slice->cof[0][j0][i0] = level;// add new intra DC coeff
              }
            }
          }

        if (mb->isLossless == false)
          itrans2 (mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }

    updateQp (mb, slice->qp);

    qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

    //init quant parameters for chroma
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
      {
      if (codedBlockPattern) {
        if (mb->lumaTransformSize8x8flag) // 8x8 transform size & eCabac
          mb->readCompCoef8x8cabac (mb, &se, PLANE_Y);
        else
          mb->readCompCoef4x4cabac (mb, &se, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern);
        }
      }

    for (uv = 0; uv < 2; ++uv ) {
      // 16x16DC Luma_Add
      if (IS_I16MB (mb)) {
        //{{{  read DC coeffs for new intra modes
        {
          se.type = SE_LUM_DC_INTRA;
          dataPartition = &slice->dataPartitionArray[dpMap[se.type]];

          if (decoder->coding.isSeperateColourPlane != 0)
            se.context = LUMA_16DC;
          else
            se.context = (uv == 0) ? CB_16DC : CR_16DC;

          if (dataPartition->mBitStream.mError)
            se.mapping = sBitStream::infoLevelRunInter;
          else
            se.reading = readRunLevelCabac;

          coef_ctr = -1;
          level = 1;                            // just to get inside the loop

          for (k = 0; (k< 17) && (level != 0); ++k) {
            dataPartition->readSyntaxElement (mb, &se, dataPartition);
            level = se.value1;

            if (level != 0) {
              coef_ctr += se.value2 + 1;
              i0 = pos_scan4x4[coef_ctr][0];
              j0 = pos_scan4x4[coef_ctr][1];
              slice->cof[uv + 1][j0<<2][i0<<2] = level;
              }
            } //k loop
          } // else eCavlc

        if(mb->isLossless == false)
          itrans2(mb, (eColorPlane) (uv + 1)); // transform new intra DC
        } //IS_I16MB
        //}}}

      updateQp (mb, slice->qp);

      qp_per = decoder->qpPerMatrix[ (slice->qp + decoder->coding.bitDepthLumaQpScale) ];
      qp_rem = decoder->qpRemMatrix[ (slice->qp + decoder->coding.bitDepthLumaQpScale) ];

      //init constants for every chroma qp offset
      qp_per_uv[uv] = decoder->qpPerMatrix[ (mb->qpc[uv] + decoder->coding.bitDepthChromaQpScale) ];
      qp_rem_uv[uv] = decoder->qpRemMatrix[ (mb->qpc[uv] + decoder->coding.bitDepthChromaQpScale) ];

      InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];

        {
        if (codedBlockPattern) {
          if (mb->lumaTransformSize8x8flag) // 8x8 transform size & eCabac
            mb->readCompCoef8x8cabac (mb, &se, (eColorPlane) (PLANE_U + uv));
          else //4x4
            mb->readCompCoef4x4cabac (mb, &se, (eColorPlane) (PLANE_U + uv), InvLevelScale4x4,  qp_per_uv[uv], codedBlockPattern);
          }
        }
      }
    }
  //}}}
  //{{{
  void readCbp422cabac (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int i,j,k;
    int level;
    int codedBlockPattern;
    const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int coef_ctr, i0, j0, b8;
    int ll;
    int qp_per, qp_rem;
    int uv;
    int qp_per_uv[2];
    int qp_rem_uv[2];
    int intra = (mb->isIntraBlock == true);
    int b4;
    sPicture* picture = slice->picture;
    int yuv = picture->chromaFormatIdc - 1;
    int m6[4];
    int need_transform_sizeFlag;
    int (*InvLevelScale4x4)[4] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // QPI init constants for every chroma qp offset
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    // read CBP if not new intra mode
    sSyntaxElement se;
    sDataPartition *dataPartition = NULL;
    if (!IS_I16MB (mb)) {
      // codedBlockPattern
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
                  ? SE_CBP_INTRA : SE_CBP_INTER;
      dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
      if (dataPartition->mBitStream.mError)
        se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
                       ? slice->infoCbpIntra : slice->infoCbpInter;
      else
        se.reading = readCbpCabac;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      // Transform size flag for INTER MBs
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type = SE_HEADER;
        dataPartition = &slice->dataPartitionArray[dpMap[SE_HEADER]];
        se.reading = readMbTransformSizeCabac;

        // read eCavlc transform_size_8x8Flag
        if (dataPartition->mBitStream.mError) {
          se.len = 1;
          dataPartition->mBitStream.readSyntaxElementFLC (&se);
          }
        else
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }

      // Delta quant only if nonzero coeffs
      if (codedBlockPattern != 0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);
        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC)
            mb->dplFlag = 1;
          if (intra && slice->noDataPartitionB) {
            mb->mError = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
          }
        }
      }
    else {
      //{{{  read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);

      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->mError = 1;
          mb->dplFlag = 1;
          }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];

        {
          se.type = SE_LUM_DC_INTRA;
          dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
          se.context = LUMA_16DC;
          se.type = SE_LUM_DC_INTRA;

          if (dataPartition->mBitStream.mError)
            se.mapping = sBitStream::infoLevelRunInter;
          else
            se.reading = readRunLevelCabac;

          level = 1; // just to get inside the loop
          for (k = 0; (k < 17) && (level != 0); ++k) {
            dataPartition->readSyntaxElement(mb, &se, dataPartition);
            level = se.value1;

            if (level != 0) {
              pos_scan_4x4 += (2 * se.value2);
              i0 = ((*pos_scan_4x4++) << 2);
              j0 = ((*pos_scan_4x4++) << 2);
              slice->cof[0][j0][i0] = level; // add new intra DC coeff
              }
            }
          }

        if (mb->isLossless == false)
          itrans2 (mb, (eColorPlane) slice->colourPlaneId); // transform new intra DC
        }
      }
      //}}}

    updateQp (mb, slice->qp);

    qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

    // init quant parameters for chroma
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
    if (codedBlockPattern) {
      if (mb->lumaTransformSize8x8flag)
        mb->readCompCoef8x8cabac (mb, &se, PLANE_Y);
      else
        mb->readCompCoef4x4cabac (mb, &se, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern);
      }

    // chroma DC coeff
    if (codedBlockPattern > 15) {
      for (ll = 0; ll < 3; ll += 2) {
        int (*InvLevelScale4x4)[4] = NULL;
        uv = ll>>1;
        {
          int** imgcof = slice->cof[uv + 1];
          int m3[2][4] = {{0,0,0,0},{0,0,0,0}};
          int m4[2][4] = {{0,0,0,0},{0,0,0,0}};
          int qp_per_uv_dc = decoder->qpPerMatrix[ (mb->qpc[uv] + 3 + decoder->coding.bitDepthChromaQpScale) ];       //for YUV422 only
          int qp_rem_uv_dc = decoder->qpRemMatrix[ (mb->qpc[uv] + 3 + decoder->coding.bitDepthChromaQpScale) ];       //for YUV422 only
          if (intra)
            InvLevelScale4x4 = slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv_dc];
          else
            InvLevelScale4x4 = slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv_dc];


          // CHROMA DC YUV422
          {
            sCodedBlockPattern* cbp = &mb->cbp[0];
            coef_ctr = -1;
            level = 1;
            for (k = 0; (k < 9) && (level != 0); ++k) {
              se.context = CHROMA_DC_2x4;
              se.type = ((mb->isIntraBlock == true) ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER);
              mb->isVblock = ll;

              dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
              if (dataPartition->mBitStream.mError)
                se.mapping = sBitStream::infoLevelRunc2x2;
              else
                se.reading = readRunLevelCabac;
              dataPartition->readSyntaxElement(mb, &se, dataPartition);

              level = se.value1;
              if (level != 0) {
                cbp->blk |= ((int64_t)0xff0000) << (ll<<2) ;
                coef_ctr += se.value2 + 1;
                i0 = SCAN_YUV422[coef_ctr][0];
                j0 = SCAN_YUV422[coef_ctr][1];

                m3[i0][j0]=level;
                }
              }
            }
          //{{{  inverse CHROMA DC YUV422 transform  horizontal
          if (mb->isLossless == false) {
            m4[0][0] = m3[0][0] + m3[1][0];
            m4[0][1] = m3[0][1] + m3[1][1];
            m4[0][2] = m3[0][2] + m3[1][2];
            m4[0][3] = m3[0][3] + m3[1][3];

            m4[1][0] = m3[0][0] - m3[1][0];
            m4[1][1] = m3[0][1] - m3[1][1];
            m4[1][2] = m3[0][2] - m3[1][2];
            m4[1][3] = m3[0][3] - m3[1][3];

            for (i = 0; i < 2; ++i) {
              m6[0] = m4[i][0] + m4[i][2];
              m6[1] = m4[i][0] - m4[i][2];
              m6[2] = m4[i][1] - m4[i][3];
              m6[3] = m4[i][1] + m4[i][3];

              imgcof[ 0][i<<2] = m6[0] + m6[3];
              imgcof[ 4][i<<2] = m6[1] + m6[2];
              imgcof[ 8][i<<2] = m6[1] - m6[2];
              imgcof[12][i<<2] = m6[0] - m6[3];
              }

            for (int j = 0;j < decoder->mbCrSizeY; j += BLOCK_SIZE)
              for (int i = 0; i < decoder->mbCrSizeX;i+=BLOCK_SIZE)
                imgcof[j][i] = rshift_rnd_sf((imgcof[j][i] * InvLevelScale4x4[0][0]) << qp_per_uv_dc, 6);
            }

          else
            for (int j = 0; j < 4; ++j)
              for (int i = 0; i < 2; ++i)
                slice->cof[uv + 1][j<<2][i<<2] = m3[i][j];
                     //}}}
          }
        }
      }

    //{{{  chroma AC coeff, all zero fram start_scan
    if (codedBlockPattern <= 31) {
      }
    else {
      {
        se.context = CHROMA_AC;
        se.type = (mb->isIntraBlock ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER);
        dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
        if (dataPartition->mBitStream.mError)
          se.mapping = sBitStream::infoLevelRunInter;
        else
          se.reading = readRunLevelCabac;

        if (mb->isLossless == false) {
          sCodedBlockPattern* cbp = &mb->cbp[0];
          for (b8 = 0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
            uv = b8 > (decoder->coding.numUvBlocks - 1);
            mb->isVblock = uv;
            InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];
            for (b4 = 0; b4 < 4; ++b4) {
              i = cofuv_blk_x[yuv][b8][b4];
              j = cofuv_blk_y[yuv][b8][b4];
              mb->subblockY = subblk_offset_y[yuv][b8][b4];
              mb->subblockX = subblk_offset_x[yuv][b8][b4];
              pos_scan_4x4 = pos_scan4x4[1];

              level = 1;
              for(k = 0; (k < 16) && (level != 0);++k) {
                dataPartition->readSyntaxElement(mb, &se, dataPartition);
                level = se.value1;

                if (level != 0) {
                  cbp->blk |= i64power2(cbp_blk_chroma[b8][b4]);
                  pos_scan_4x4 += (se.value2 << 1);
                  i0 = *pos_scan_4x4++;
                  j0 = *pos_scan_4x4++;
                  slice->cof[uv + 1][(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((level * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
                  }
                }
              }
           }
          }
        else {
          sCodedBlockPattern* cbp = &mb->cbp[0];
          for (b8 = 0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
            uv = b8 > (decoder->coding.numUvBlocks - 1);
            mb->isVblock = uv;
            for (b4=0; b4 < 4; ++b4) {
              i = cofuv_blk_x[yuv][b8][b4];
              j = cofuv_blk_y[yuv][b8][b4];

              pos_scan_4x4 = pos_scan4x4[1];
              level=1;

              mb->subblockY = subblk_offset_y[yuv][b8][b4];
              mb->subblockX = subblk_offset_x[yuv][b8][b4];

              for(k = 0; (k < 16) && (level != 0); ++k) {
                dataPartition->readSyntaxElement (mb, &se, dataPartition);
                level = se.value1;
                if (level != 0) {
                  cbp->blk |= i64power2(cbp_blk_chroma[b8][b4]);
                  pos_scan_4x4 += (se.value2 << 1);
                  i0 = *pos_scan_4x4++;
                  j0 = *pos_scan_4x4++;
                  slice->cof[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
                  }
                }
              }
            }
          }
        }
      }
    //}}}
    }
  //}}}
  //{{{
  void readCbp420cabac (sMacroBlock* mb) {

    int i,j;
    int level;
    int codedBlockPattern;
    cSlice* slice = mb->slice;
    const uint8_t *dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    int i0, j0;

    int qp_per, qp_rem;
    cDecoder264* decoder = mb->decoder;
    int smb = ((decoder->coding.sliceType == cSlice::eSP) && (mb->isIntraBlock == false)) ||
               (decoder->coding.sliceType == cSlice::eSI && mb->mbType == SI4MB);

    int qp_per_uv[2];
    int qp_rem_uv[2];

    int intra = (mb->isIntraBlock == true);

    sPicture* picture = slice->picture;
    int yuv = picture->chromaFormatIdc - 1;

    int (*InvLevelScale4x4)[4] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((slice->picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    sDataPartition* dataPartition = NULL;
    sSyntaxElement se;
    if (!IS_I16MB (mb)) {
      int need_transform_sizeFlag;
      // CBP
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB) ? SE_CBP_INTRA : SE_CBP_INTER;

      dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
      if (dataPartition->mBitStream.mError)
        se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
          ? slice->infoCbpIntra : slice->infoCbpInter;
      else
        se.reading = readCbpCabac;
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      // Transform size flag for INTER MBs
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
                                (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
                                (mb->noMbPartLessThan8x8Flag))
                                && mb->mbType != I8MB && mb->mbType != I4MB
                                && (mb->codedBlockPattern&15)
                                && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type = SE_HEADER;
        dataPartition = &slice->dataPartitionArray[dpMap[SE_HEADER]];
        se.reading = readMbTransformSizeCabac;

        // read eCavlc transform_size_8x8Flag
        if (dataPartition->mBitStream.mError) {
          se.len = 1;
          dataPartition->mBitStream.readSyntaxElementFLC (&se);
          }
        else
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }

      //{{{  Delta quant only if nonzero coeffs
      if (codedBlockPattern !=0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);
        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;
          if (intra && slice->noDataPartitionB ) {
            mb->mError = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
          }
        }
      //}}}
      }
    else {
      //{{{  read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant(&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);
      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->mError  = 1;
          mb->dplFlag = 1;
        }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }
      //}}}

      if (!mb->dplFlag) {
        int** cof = slice->cof[0];
        pos_scan_4x4 = pos_scan4x4[0];

        se.type = SE_LUM_DC_INTRA;
        dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
        se.context = LUMA_16DC;
        se.type = SE_LUM_DC_INTRA;
        if (dataPartition->mBitStream.mError)
          se.mapping = sBitStream::infoLevelRunInter;
        else
          se.reading = readRunLevelCabac;

        level = 1;                            // just to get inside the loop
        for (int k = 0; (k < 17) && (level != 0); ++k) {
          dataPartition->readSyntaxElement(mb, &se, dataPartition);
          level = se.value1;
          if (level != 0) {
            pos_scan_4x4 += (2 * se.value2);
            i0 = ((*pos_scan_4x4++) << 2);
            j0 = ((*pos_scan_4x4++) << 2);
            cof[j0][i0] = level;// add new intra DC coeff
            }
          }
        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }

    updateQp (mb, slice->qp);
    qp_per = decoder->qpPerMatrix[mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[mb->qpScaled[slice->colourPlaneId] ];

    // luma coefficients
    if (codedBlockPattern) {
      if(mb->lumaTransformSize8x8flag) // 8x8 transform size & eCabac
        mb->readCompCoef8x8cabac (mb, &se, PLANE_Y);
      else {
        InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
        mb->readCompCoef4x4cabac (mb, &se, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern);
        }
      }

    // init quant parameters for chroma
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }
    //{{{  chroma DC coeff
    if (codedBlockPattern > 15) {
      sCodedBlockPattern* cbp = &mb->cbp[0];
      int uv, ll, k, coef_ctr;

      for (ll = 0; ll < 3; ll += 2) {
        uv = ll >> 1;
        InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];

        // CHROMA DC YUV420
        slice->cofu[0] = slice->cofu[1] = slice->cofu[2] = slice->cofu[3] = 0;
        coef_ctr = -1;

        level = 1;
        mb->isVblock = ll;
        se.context = CHROMA_DC;
        se.type = (intra ? SE_CHR_DC_INTRA : SE_CHR_DC_INTER);
        dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
        if (dataPartition->mBitStream.mError)
          se.mapping = sBitStream::infoLevelRunc2x2;
        else
          se.reading = readRunLevelCabac;

        for (k = 0; (k < (decoder->coding.numCdcCoeff + 1)) && (level != 0); ++k) {
          dataPartition->readSyntaxElement (mb, &se, dataPartition);
          level = se.value1;
          if (level != 0) {
            cbp->blk |= 0xf0000 << (ll<<1) ;
            coef_ctr += se.value2 + 1;
            // Bug: slice->cofu has only 4 entries, hence coef_ctr MUST be <4 (which is
            // caught by the assert().  If it is bigger than 4, it starts patching the
            // decoder->predmode pointer, which leads to bugs later on.
            // This assert() should be left in the code, because it captures a very likely
            // bug early when testing in error prone environments (or when testing NALfunctionality).
            slice->cofu[coef_ctr] = level;
            }
          }


        if (smb || (mb->isLossless == true)) {
          //{{{  check to see if MB type is SPred or SIntra4x4
          slice->cof[uv + 1][0][0] = slice->cofu[0];
          slice->cof[uv + 1][0][4] = slice->cofu[1];
          slice->cof[uv + 1][4][0] = slice->cofu[2];
          slice->cof[uv + 1][4][4] = slice->cofu[3];
          }
          //}}}
        else {
          int temp[4];
          int scale_dc = InvLevelScale4x4[0][0];
          int** cof = slice->cof[uv + 1];
          ihadamard2x2(slice->cofu, temp);
          cof[0][0] = (((temp[0] * scale_dc) << qp_per_uv[uv]) >> 5);
          cof[0][4] = (((temp[1] * scale_dc) << qp_per_uv[uv]) >> 5);
          cof[4][0] = (((temp[2] * scale_dc) << qp_per_uv[uv]) >> 5);
          cof[4][4] = (((temp[3] * scale_dc) << qp_per_uv[uv]) >> 5);
          }
        }
      }
    //}}}
    //{{{  chroma AC coeff, all zero fram start_scan
    if (codedBlockPattern > 31) {
      se.context = CHROMA_AC;
      se.type = (mb->isIntraBlock ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER);
      dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
      if (dataPartition->mBitStream.mError)
        se.mapping = sBitStream::infoLevelRunInter;
      else
        se.reading = readRunLevelCabac;

      if (mb->isLossless == false) {
        int b4, b8, uv, k;
        int** cof;
        sCodedBlockPattern  *cbp = &mb->cbp[0];
        for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[uv + 1][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv + 1][qp_rem_uv[uv]];
          cof = slice->cof[uv + 1];

          for (b4 = 0; b4 < 4; ++b4) {
            i = cofuv_blk_x[yuv][b8][b4];
            j = cofuv_blk_y[yuv][b8][b4];

            mb->subblockY = subblk_offset_y[yuv][b8][b4];
            mb->subblockX = subblk_offset_x[yuv][b8][b4];

            pos_scan_4x4 = pos_scan4x4[1];
            level = 1;

            for(k = 0; (k < 16) && (level != 0);++k) {
              dataPartition->readSyntaxElement(mb, &se, dataPartition);
              level = se.value1;

              if (level != 0) {
                cbp->blk |= i64power2(cbp_blk_chroma[b8][b4]);
                pos_scan_4x4 += (se.value2 << 1);

                i0 = *pos_scan_4x4++;
                j0 = *pos_scan_4x4++;

                cof[(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((level * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
                }
              }
            }
          }
        }
      else {
        sCodedBlockPattern* cbp = &mb->cbp[0];
        int b4, b8, k;
        int uv;
        for (b8 = 0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          for (b4 = 0; b4 < 4; ++b4) {
            i = cofuv_blk_x[yuv][b8][b4];
            j = cofuv_blk_y[yuv][b8][b4];
            pos_scan_4x4 = pos_scan4x4[1];
            level = 1;
            mb->subblockY = subblk_offset_y[yuv][b8][b4];
            mb->subblockX = subblk_offset_x[yuv][b8][b4];
            for (k = 0; (k < 16) && (level != 0); ++k) {
              dataPartition->readSyntaxElement(mb, &se, dataPartition);
              level = se.value1;

              if (level != 0) {
                cbp->blk |= i64power2(cbp_blk_chroma[b8][b4]);
                pos_scan_4x4 += (se.value2 << 1);
                i0 = *pos_scan_4x4++;
                j0 = *pos_scan_4x4++;
                slice->cof[uv + 1][(j<<2) + j0][(i<<2) + i0] = level;
                }
              }
            }
          }
        }
      }
    //}}}
    }
  //}}}

  //{{{
  void readCbp400cavlc (sMacroBlock* mb) {

    int k;
    int mb_nr = mb->mbIndexX;
    int codedBlockPattern;

    cSlice* slice = mb->slice;
    int i0, j0;

    int levarr[16], runarr[16], numcoeff;
    int qp_per, qp_rem;

    sSyntaxElement se;
    sDataPartition* dataPartition = NULL;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;
    int intra = (mb->isIntraBlock == true);
    int need_transform_sizeFlag;

    int (*InvLevelScale4x4)[4] = NULL;
    int (*InvLevelScale8x8)[8] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB) ? SE_CBP_INTRA : SE_CBP_INTER;
      dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
      se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? slice->infoCbpIntra : slice->infoCbpInter;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      // Transform size flag for INTER MBs
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
                                   (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
                                   (mb->noMbPartLessThan8x8Flag))
                                 && mb->mbType != I8MB && mb->mbType != I4MB
                                 && (mb->codedBlockPattern&15)
                                 && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        //{{{  read eCavlc transform_size_8x8Flag
        se.type =  SE_HEADER;
        dataPartition = &slice->dataPartitionArray[dpMap[SE_HEADER]];
        se.len = 1;
        dataPartition->mBitStream.readSyntaxElementFLC (&se);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }
        //}}}
      if (codedBlockPattern != 0) {
        //{{{  Delta quant only if nonzero coeffs
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if( intra && slice->noDataPartitionB ) {
            mb->mError = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
          }
        }
        //}}}
      }
    else  {
      //{{{  read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);
      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->mError  = 1;
          mb->dplFlag = 1;
        }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];
        slice->readCoef4x4cavlc (mb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
        for(k = 0; k < numcoeff; ++k) {
          if (levarr[k] != 0) {
            // leave if level == 0
            pos_scan_4x4 += 2 * runarr[k];
            i0 = ((*pos_scan_4x4++) << 2);
            j0 = ((*pos_scan_4x4++) << 2);
            slice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
            }
          }


        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }
      //}}}

    updateQp (mb, slice->qp);

    qp_per = decoder->qpPerMatrix[ mb->qpScaled[PLANE_Y] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[PLANE_Y] ];

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
    InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
    if (codedBlockPattern) {
      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      }
    else
      memset (decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(uint8_t));
    }
  //}}}
  //{{{
  void readCbp422cavlc (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
    cSlice* slice = mb->slice;

    int i,j,k;
    int mb_nr = mb->mbIndexX;
    int codedBlockPattern;

    int coef_ctr, i0, j0, b8;
    int ll;
    int levarr[16], runarr[16], numcoeff;

    int qp_per, qp_rem;

    int uv;
    int qp_per_uv[2];
    int qp_rem_uv[2];

    int intra = (mb->isIntraBlock == true);

    int b4;
    int m6[4];

    int need_transform_sizeFlag;

    int (*InvLevelScale4x4)[4] = NULL;
    int (*InvLevelScale8x8)[8] = NULL;
    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    sSyntaxElement se;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    sDataPartition* dataPartition = NULL;

    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB) ? SE_CBP_INTRA : SE_CBP_INTER;
      dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
      se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? slice->infoCbpIntra : slice->infoCbpInter;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        //{{{  Transform size flag for INTER MBs
        se.type   =  SE_HEADER;
        dataPartition = &slice->dataPartitionArray[dpMap[SE_HEADER]];

        // read eCavlc transform_size_8x8Flag
        se.len = 1;
        dataPartition->mBitStream.readSyntaxElementFLC (&se);
        mb->lumaTransformSize8x8flag = (bool) se.value1;
        }
        //}}}
      if (codedBlockPattern != 0) {
        //{{{  Delta quant only if nonzero coeffs
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if( intra && slice->noDataPartitionB ) {
            mb->mError = 1;
            mb->dplFlag = 1;
          }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
         }
        }
        //}}}
      }
    else {
      //{{{  read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;

      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);

      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->mError  = 1;
          mb->dplFlag = 1;
        }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];
        slice->readCoef4x4cavlc(mb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
        for(k = 0; k < numcoeff; ++k) {
          if (levarr[k] != 0) {
            // leave if level == 0
            pos_scan_4x4 += 2 * runarr[k];
            i0 = ((*pos_scan_4x4++) << 2);
            j0 = ((*pos_scan_4x4++) << 2);
            slice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
            }
          }
        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }
      //}}}

    updateQp (mb, slice->qp);
    qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

    // init quant parameters for chroma
    for (i=0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
    InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
    if (codedBlockPattern) {
      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      }
    else
      memset (decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(uint8_t));

    //{{{  chroma DC coeff
    if (codedBlockPattern>15) {
      for (ll=0;ll<3;ll+=2) {
        int (*InvLevelScale4x4)[4] = NULL;
        uv = ll>>1;
        {
          int** imgcof = slice->cof[PLANE_U + uv];
          int m3[2][4] = {{0,0,0,0},{0,0,0,0}};
          int m4[2][4] = {{0,0,0,0},{0,0,0,0}};
          int qp_per_uv_dc = decoder->qpPerMatrix[ (mb->qpc[uv] + 3 + decoder->coding.bitDepthChromaQpScale) ];       //for YUV422 only
          int qp_rem_uv_dc = decoder->qpRemMatrix[ (mb->qpc[uv] + 3 + decoder->coding.bitDepthChromaQpScale) ];       //for YUV422 only
          if (intra)
            InvLevelScale4x4 = slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv_dc];
          else
            InvLevelScale4x4 = slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv_dc];


          // CHROMA DC YUV422
          slice->readCoef4x4cavlc(mb, CHROMA_DC, 0, 0, levarr, runarr, &numcoeff);
          coef_ctr=-1;
          for(k = 0; k < numcoeff; ++k) {
            if (levarr[k] != 0) {
              mb->cbp[0].blk |= ((int64_t)0xff0000) << (ll<<2);
              coef_ctr += runarr[k]+1;
              i0 = SCAN_YUV422[coef_ctr][0];
              j0 = SCAN_YUV422[coef_ctr][1];

              m3[i0][j0]=levarr[k];
            }
          }

          // inverse CHROMA DC YUV422 transform horizontal
          if(mb->isLossless == false) {
            m4[0][0] = m3[0][0] + m3[1][0];
            m4[0][1] = m3[0][1] + m3[1][1];
            m4[0][2] = m3[0][2] + m3[1][2];
            m4[0][3] = m3[0][3] + m3[1][3];

            m4[1][0] = m3[0][0] - m3[1][0];
            m4[1][1] = m3[0][1] - m3[1][1];
            m4[1][2] = m3[0][2] - m3[1][2];
            m4[1][3] = m3[0][3] - m3[1][3];

            for (i = 0; i < 2; ++i) {
              m6[0] = m4[i][0] + m4[i][2];
              m6[1] = m4[i][0] - m4[i][2];
              m6[2] = m4[i][1] - m4[i][3];
              m6[3] = m4[i][1] + m4[i][3];

              imgcof[ 0][i<<2] = m6[0] + m6[3];
              imgcof[ 4][i<<2] = m6[1] + m6[2];
              imgcof[ 8][i<<2] = m6[1] - m6[2];
              imgcof[12][i<<2] = m6[0] - m6[3];
              }//for (i=0;i<2;++i)

            for(j = 0;j < decoder->mbCrSizeY; j += BLOCK_SIZE) {
              for(i=0;i < decoder->mbCrSizeX;i+=BLOCK_SIZE)
                imgcof[j][i] = rshift_rnd_sf((imgcof[j][i] * InvLevelScale4x4[0][0]) << qp_per_uv_dc, 6);
              }
           }
           else {
            for(j=0;j<4;++j) {
              slice->cof[PLANE_U + uv][j<<2][0] = m3[0][j];
              slice->cof[PLANE_U + uv][j<<2][4] = m3[1][j];
              }
            }

          }
        }//for (ll=0;ll<3;ll+=2)
      }
    //}}}
    //{{{  chroma AC coeff, all zero fram start_scan
    if (codedBlockPattern<=31)
      memset (decoder->nzCoeff [mb_nr ][1][0], 0, 2 * BLOCK_PIXELS * sizeof(uint8_t));
    else {
      if(mb->isLossless == false) {
        for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];

          for (b4=0; b4 < 4; ++b4) {
            i = cofuv_blk_x[1][b8][b4];
            j = cofuv_blk_y[1][b8][b4];

            slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
            coef_ctr = 0;

            for(k = 0; k < numcoeff;++k) {
              if (levarr[k] != 0) {
                mb->cbp[0].blk |= i64power2(cbp_blk_chroma[b8][b4]);
                coef_ctr += runarr[k] + 1;

                i0=pos_scan4x4[coef_ctr][0];
                j0=pos_scan4x4[coef_ctr][1];

                slice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((levarr[k] * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
                }
              }
            }
          }
        }
      else {
        for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          for (b4=0; b4 < 4; ++b4) {
            i = cofuv_blk_x[1][b8][b4];
            j = cofuv_blk_y[1][b8][b4];

            slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
            coef_ctr = 0;

            for(k = 0; k < numcoeff;++k) {
              if (levarr[k] != 0) {
                mb->cbp[0].blk |= i64power2(cbp_blk_chroma[b8][b4]);
                coef_ctr += runarr[k] + 1;
                i0=pos_scan4x4[coef_ctr][0];
                j0=pos_scan4x4[coef_ctr][1];
                slice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = levarr[k];
                }
              }
            }
          }
        }
      }
    //}}}
    }
  //}}}
  //{{{
  void readCbp444cavlc (sMacroBlock* mb) {

    cSlice* slice = mb->slice;

    int i,k;
    int mb_nr = mb->mbIndexX;
    int codedBlockPattern;
    int coef_ctr, i0, j0;
    int levarr[16], runarr[16], numcoeff;

    int qp_per, qp_rem;
    int uv;
    int qp_per_uv[3];
    int qp_rem_uv[3];
    int need_transform_sizeFlag;

    sSyntaxElement se;
    sDataPartition *dataPartition = NULL;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;

    int intra = (mb->isIntraBlock == true);

    int (*InvLevelScale4x4)[4] = NULL;
    int (*InvLevelScale8x8)[8] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? SE_CBP_INTRA : SE_CBP_INTER;
      dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
      se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? slice->infoCbpIntra : slice->infoCbpInter;
      dataPartition->readSyntaxElement (mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      //============= Transform size flag for INTER MBs =============
      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);

      if (need_transform_sizeFlag) {
        se.type   =  SE_HEADER;
        dataPartition = &slice->dataPartitionArray[dpMap[SE_HEADER]];
        // read eCavlc transform_size_8x8Flag
        se.len = 1;
        dataPartition->mBitStream.readSyntaxElementFLC (&se);
        mb->lumaTransformSize8x8flag = (bool)se.value1;
        }

      // Delta quant only if nonzero coeffs
      if (codedBlockPattern !=0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);

        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;

          if( intra && slice->noDataPartitionB ) {
            mb->mError = 1;
            mb->dplFlag = 1;
          }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
          }
        }
      }
    }
    else {
      //{{{  read DC coeffs for new intra modes
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);
      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->mError  = 1;
          mb->dplFlag = 1;
          }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];
        slice->readCoef4x4cavlc(mb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
        for(k = 0; k < numcoeff; ++k) {
          if (levarr[k] != 0) {
            // leave if level == 0
            pos_scan_4x4 += 2 * runarr[k];
            i0 = ((*pos_scan_4x4++) << 2);
            j0 = ((*pos_scan_4x4++) << 2);
            slice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
            }
          }
        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }
      //}}}

    updateQp(mb, slice->qp);
    qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

    //init quant parameters for chroma
    for (i=PLANE_U; i <= PLANE_V; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i] ];
      }

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
    InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
    if (codedBlockPattern) {
      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
    }
    else
      memset (decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(uint8_t));

    for (uv = PLANE_U; uv <= PLANE_V; ++uv ) {
      //{{{  16x16DC Luma_Add
      if (IS_I16MB (mb)) {
        // read DC coeffs for new intra modes
        if (uv == PLANE_U)
          slice->readCoef4x4cavlc(mb, CB_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
        else
          slice->readCoef4x4cavlc(mb, CR_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);

        coef_ctr=-1;

        for(k = 0; k < numcoeff; ++k) {
          if (levarr[k] != 0) {
            // leave if level == 0
            coef_ctr += runarr[k] + 1;
            i0 = pos_scan4x4[coef_ctr][0];
            j0 = pos_scan4x4[coef_ctr][1];
            slice->cof[uv][j0<<2][i0<<2] = levarr[k];// add new intra DC coeff
            } //if leavarr[k]
          } //k loop

        if(mb->isLossless == false)
          itrans2(mb, (eColorPlane) (uv)); // transform new intra DC
        } //IS_I16MB

      //init constants for every chroma qp offset
      updateQp(mb, slice->qp);
      qp_per_uv[uv] = decoder->qpPerMatrix[ mb->qpScaled[uv] ];
      qp_rem_uv[uv] = decoder->qpRemMatrix[ mb->qpScaled[uv] ];

      InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[uv][qp_rem_uv[uv]];
      InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[uv][qp_rem_uv[uv]] : slice->InvLevelScale8x8_Inter[uv][qp_rem_uv[uv]];

      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, (eColorPlane) (uv), InvLevelScale4x4, qp_per_uv[uv], codedBlockPattern, decoder->nzCoeff[mb_nr][uv]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, (eColorPlane) (uv), InvLevelScale8x8, qp_per_uv[uv], codedBlockPattern, decoder->nzCoeff[mb_nr][uv]);
      }
      //}}}
    }
  //}}}
  //{{{
  void readCbp420cavlc (sMacroBlock* mb) {

    cSlice* slice = mb->slice;

    int i,j,k;
    int mb_nr = mb->mbIndexX;
    int codedBlockPattern;
    int coef_ctr, i0, j0, b8;
    int ll;
    int levarr[16], runarr[16], numcoeff;

    int qp_per, qp_rem;

    sSyntaxElement se;
    sDataPartition* dataPartition = NULL;
    const uint8_t* dpMap = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode];
    cDecoder264* decoder = mb->decoder;
    int smb = ((decoder->coding.sliceType == cSlice::eSP) && (mb->isIntraBlock == false)) ||
              ((decoder->coding.sliceType == cSlice::eSI) && (mb->mbType == SI4MB));

    int uv;
    int qp_per_uv[2];
    int qp_rem_uv[2];

    int intra = (mb->isIntraBlock == true);
    int temp[4];

    int b4;
    int need_transform_sizeFlag;
    int (*InvLevelScale4x4)[4] = NULL;
    int (*InvLevelScale8x8)[8] = NULL;

    // select scan type
    const uint8_t (*pos_scan4x4)[2] = ((decoder->coding.picStructure == eFrame) && (!mb->mbField)) ? SNGL_SCAN : FIELD_SCAN;
    const uint8_t *pos_scan_4x4 = pos_scan4x4[0];

    // read CBP if not new intra mode
    if (!IS_I16MB (mb)) {
      se.type = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? SE_CBP_INTRA : SE_CBP_INTER;
      dataPartition = &slice->dataPartitionArray[dpMap[se.type]];
      se.mapping = (mb->mbType == I4MB || mb->mbType == SI4MB || mb->mbType == I8MB)
        ? slice->infoCbpIntra : slice->infoCbpInter;
      dataPartition->readSyntaxElement(mb, &se, dataPartition);
      mb->codedBlockPattern = codedBlockPattern = se.value1;

      need_transform_sizeFlag = (((mb->mbType >= 1 && mb->mbType <= 3)||
        (IS_DIRECT(mb) && decoder->activeSps->isDirect8x8inference) ||
        (mb->noMbPartLessThan8x8Flag))
        && mb->mbType != I8MB && mb->mbType != I4MB
        && (mb->codedBlockPattern&15)
        && slice->transform8x8Mode);
      if (need_transform_sizeFlag) {
        //{{{  Transform size flag for INTER MBs
        se.type =  SE_HEADER;
        dataPartition = &slice->dataPartitionArray[dpMap[SE_HEADER]];
        // read eCavlc transform_size_8x8Flag
        se.len = 1;
        dataPartition->mBitStream.readSyntaxElementFLC (&se);
        mb->lumaTransformSize8x8flag = (bool)se.value1;
        }
        //}}}

      // Delta quant only if nonzero coeffs
      if (codedBlockPattern != 0) {
        readDeltaQuant (&se, dataPartition, mb, dpMap, ((mb->isIntraBlock == false)) ? SE_DELTA_QUANT_INTER : SE_DELTA_QUANT_INTRA);
        if (slice->dataPartitionMode) {
          if ((mb->isIntraBlock == false) && slice->noDataPartitionC )
            mb->dplFlag = 1;
          if (intra && slice->noDataPartitionB) {
            mb->mError = 1;
            mb->dplFlag = 1;
            }

          // check for prediction from neighbours
          checkDpNeighbours (mb);
          if (mb->dplFlag) {
            codedBlockPattern = 0;
            mb->codedBlockPattern = codedBlockPattern;
            }
          }
        }
      }
    else {
      codedBlockPattern = mb->codedBlockPattern;
      readDeltaQuant (&se, dataPartition, mb, dpMap, SE_DELTA_QUANT_INTRA);

      if (slice->dataPartitionMode) {
        if (slice->noDataPartitionB) {
          mb->mError  = 1;
          mb->dplFlag = 1;
          }
        checkDpNeighbours (mb);
        if (mb->dplFlag)
          mb->codedBlockPattern = codedBlockPattern = 0;
        }

      if (!mb->dplFlag) {
        pos_scan_4x4 = pos_scan4x4[0];
        slice->readCoef4x4cavlc (mb, LUMA_INTRA16x16DC, 0, 0, levarr, runarr, &numcoeff);
        for (k = 0; k < numcoeff; ++k) {
          if (levarr[k] != 0) {
            // leave if level == 0
            pos_scan_4x4 += 2 * runarr[k];
            i0 = ((*pos_scan_4x4++) << 2);
            j0 = ((*pos_scan_4x4++) << 2);
            slice->cof[0][j0][i0] = levarr[k];// add new intra DC coeff
            }
          }

        if (mb->isLossless == false)
          itrans2(mb, (eColorPlane) slice->colourPlaneId);// transform new intra DC
        }
      }

    updateQp (mb, slice->qp);
    qp_per = decoder->qpPerMatrix[ mb->qpScaled[slice->colourPlaneId] ];
    qp_rem = decoder->qpRemMatrix[ mb->qpScaled[slice->colourPlaneId] ];

    // init quant parameters for chroma
    for (i = 0; i < 2; ++i) {
      qp_per_uv[i] = decoder->qpPerMatrix[ mb->qpScaled[i + 1] ];
      qp_rem_uv[i] = decoder->qpRemMatrix[ mb->qpScaled[i + 1] ];
      }

    InvLevelScale4x4 = intra? slice->InvLevelScale4x4_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale4x4_Inter[slice->colourPlaneId][qp_rem];
    InvLevelScale8x8 = intra? slice->InvLevelScale8x8_Intra[slice->colourPlaneId][qp_rem] : slice->InvLevelScale8x8_Inter[slice->colourPlaneId][qp_rem];

    // luma coefficients
    if (codedBlockPattern) {
      if (!mb->lumaTransformSize8x8flag) // 4x4 transform
        mb->readCompCoef4x4cavlc (mb, PLANE_Y, InvLevelScale4x4, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      else // 8x8 transform
        mb->readCompCoef8x8cavlc (mb, PLANE_Y, InvLevelScale8x8, qp_per, codedBlockPattern, decoder->nzCoeff[mb_nr][PLANE_Y]);
      }
    else
      memset (decoder->nzCoeff[mb_nr][0][0], 0, BLOCK_PIXELS * sizeof(uint8_t));

    //{{{  chroma DC coeff
    if (codedBlockPattern>15) {
      for (ll=0;ll<3;ll+=2) {
        uv = ll>>1;

        InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];
        // CHROMA DC YUV420
        slice->cofu[0] = slice->cofu[1] = slice->cofu[2] = slice->cofu[3] = 0;
        coef_ctr=-1;

        slice->readCoef4x4cavlc(mb, CHROMA_DC, 0, 0, levarr, runarr, &numcoeff);
        for(k = 0; k < numcoeff; ++k) {
          if (levarr[k] != 0) {
            mb->cbp[0].blk |= 0xf0000 << (ll<<1) ;
            coef_ctr += runarr[k] + 1;
            slice->cofu[coef_ctr]=levarr[k];
          }
        }


        if (smb || (mb->isLossless == true)) {
          // check to see if MB type is SPred or SIntra4x4
          slice->cof[PLANE_U + uv][0][0] = slice->cofu[0];
          slice->cof[PLANE_U + uv][0][4] = slice->cofu[1];
          slice->cof[PLANE_U + uv][4][0] = slice->cofu[2];
          slice->cof[PLANE_U + uv][4][4] = slice->cofu[3];
          }
        else {
          ihadamard2x2(slice->cofu, temp);
          slice->cof[PLANE_U + uv][0][0] = (((temp[0] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
          slice->cof[PLANE_U + uv][0][4] = (((temp[1] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
          slice->cof[PLANE_U + uv][4][0] = (((temp[2] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
          slice->cof[PLANE_U + uv][4][4] = (((temp[3] * InvLevelScale4x4[0][0])<<qp_per_uv[uv])>>5);
          }
        }
      }
    //}}}
    //{{{  chroma AC coeff, all zero fram start_scan
    if (codedBlockPattern<=31)
      memset (decoder->nzCoeff [mb_nr ][1][0], 0, 2 * BLOCK_PIXELS * sizeof(uint8_t));
    else {
      if (mb->isLossless == false) {
        for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          InvLevelScale4x4 = intra ? slice->InvLevelScale4x4_Intra[PLANE_U + uv][qp_rem_uv[uv]] : slice->InvLevelScale4x4_Inter[PLANE_U + uv][qp_rem_uv[uv]];

          for (b4=0; b4 < 4; ++b4) {
            i = cofuv_blk_x[0][b8][b4];
            j = cofuv_blk_y[0][b8][b4];
            slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
            coef_ctr = 0;
            for(k = 0; k < numcoeff;++k) {
              if (levarr[k] != 0) {
                mb->cbp[0].blk |= i64power2(cbp_blk_chroma[b8][b4]);
                coef_ctr += runarr[k] + 1;
                i0=pos_scan4x4[coef_ctr][0];
                j0=pos_scan4x4[coef_ctr][1];
                slice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = rshift_rnd_sf((levarr[k] * InvLevelScale4x4[j0][i0])<<qp_per_uv[uv], 4);
              }
            }
          }
        }
      }
      else {
        for (b8=0; b8 < decoder->coding.numBlock8x8uv; ++b8) {
          uv = b8 > (decoder->coding.numUvBlocks - 1);
          mb->isVblock = uv;
          for (b4=0; b4 < 4; ++b4) {
            i = cofuv_blk_x[0][b8][b4];
            j = cofuv_blk_y[0][b8][b4];
            slice->readCoef4x4cavlc(mb, CHROMA_AC, i + 2*uv, j + 4, levarr, runarr, &numcoeff);
            coef_ctr = 0;
            for (k = 0; k < numcoeff;++k) {
              if (levarr[k] != 0) {
                mb->cbp[0].blk |= i64power2(cbp_blk_chroma[b8][b4]);
                coef_ctr += runarr[k] + 1;

                i0=pos_scan4x4[coef_ctr][0];
                j0=pos_scan4x4[coef_ctr][1];

                slice->cof[PLANE_U + uv][(j<<2) + j0][(i<<2) + i0] = levarr[k];
                }
              }
            }
          }
        }
      }
    //}}}
    }
  //}}}
  }

// static
//{{{
cSlice* cSlice::allocSlice() {

  cSlice* slice = new cSlice();

  // create all context models
  slice->motionContexts = (sMotionContexts*)calloc (1, sizeof(sMotionContexts));
  slice->textureContexts = (sTextureContexts*)calloc (1, sizeof(sTextureContexts));

  slice->maxDataPartitions = 3;
  slice->dataPartitionArray = sDataPartition::allocDataPartitionArray (slice->maxDataPartitions);

  getMem3Dint (&slice->weightedPredWeight, 2, MAX_REFERENCE_PICTURES, 3);
  getMem3Dint (&slice->weightedPredOffset, 6, MAX_REFERENCE_PICTURES, 3);
  getMem4Dint (&slice->weightedBiPredWeight, 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);
  getMem3Dpel (&slice->mbPred, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dpel (&slice->mbRec, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&slice->mbRess, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&slice->cof, MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

  getMem2Dpel (&slice->tempBlockL0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dpel (&slice->tempBlockL1, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dpel (&slice->tempBlockL2, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dpel (&slice->tempBlockL3, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem2Dint (&slice->tempRes, MB_BLOCK_SIZE + 5, MB_BLOCK_SIZE + 5);

  // reference flag initialization
  for (int i = 0; i < 17; ++i)
    slice->refFlag[i] = 1;

  for (int i = 0; i < 6; i++)
    slice->listX[i] = (sPicture**)calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering

  for (int j = 0; j < 6; j++) {
    for (int i = 0; i < MAX_LIST_SIZE; i++)
      slice->listX[j][i] = NULL;
    slice->listXsize[j]=0;
    }

  return slice;
  }
//}}}

// public
//{{{
cSlice::~cSlice() {

  if (sliceType != eI && sliceType != eSI)
    freeRefPicListReorderBuffer();

  freeMem2Dint (tempRes);
  freeMem2Dpel (tempBlockL0);
  freeMem2Dpel (tempBlockL1);
  freeMem2Dpel (tempBlockL2);
  freeMem2Dpel (tempBlockL3);

  freeMem3Dint (cof);
  freeMem3Dint (mbRess);
  freeMem3Dpel (mbRec);
  freeMem3Dpel (mbPred);

  freeMem3Dint (weightedPredWeight);
  freeMem3Dint (weightedPredOffset);
  freeMem4Dint (weightedBiPredWeight);

  sDataPartition::freeDataPartitionArray (dataPartitionArray, 3);

  free (motionContexts);
  free (textureContexts);

  for (int i = 0; i < 6; i++) {
    if (listX[i]) {
      free (listX[i]);
      listX[i] = NULL;
      }
    }

  while (decRefPicMarkBuffer) {
    sDecodedRefPicMark* tempDecodedRefPicMark = decRefPicMarkBuffer;
    decRefPicMarkBuffer = tempDecodedRefPicMark->next;
    free (tempDecodedRefPicMark);
    }
  }
//}}}

//{{{
void cSlice::initCabacContexts() {

  //{{{
  #define IBIARI_CTX_INIT2(ii,jj,ctx,tab,num, qp) { \
    for (i = 0; i < ii; ++i) \
      for (j = 0; j < jj; ++j) \
        biContextInit (&ctx[i][j], qp, tab ## _I[num][i][j]); \
    }
  //}}}
  //{{{
  #define PBIARI_CTX_INIT2(ii,jj,ctx,tab,num, qp) { \
    for (i = 0; i < ii; ++i) \
      for (j = 0; j < jj; ++j) \
        biContextInit (&ctx[i][j], qp, tab ## _P[num][i][j]); \
    }
  //}}}
  //{{{
  #define IBIARI_CTX_INIT1(jj,ctx,tab,num, qp) { \
    for (j = 0; j < jj; ++j) \
      biContextInit (&ctx[j], qp, tab ## _I[num][0][j]); \
    }
  //}}}
  //{{{
  #define PBIARI_CTX_INIT1(jj,ctx,tab,num, qp) { \
    for (j = 0; j < jj; ++j) \
      biContextInit (&ctx[j], qp, tab ## _P[num][0][j]); \
    }
  //}}}

  sMotionContexts* mc = motionContexts;
  sTextureContexts* tc = textureContexts;

  int i, j;

  // motion coding contexts
  if ((sliceType == cSlice::eI) || (sliceType == cSlice::eSI)) {
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
void cSlice::setReadCbpCabac() {

  switch (decoder->activeSps->chromaFormatIdc) {
    case YUV444:
      if (decoder->coding.isSeperateColourPlane == 0)
        readCBPcoeffs = readCbp444cabac;
      else
        readCBPcoeffs = readCbp400cabac;
      break;

    case YUV422:
      readCBPcoeffs = readCbp422cabac;
      break;

    case YUV420:
      readCBPcoeffs = readCbp420cabac;
      break;

    case YUV400:
      readCBPcoeffs = readCbp400cabac;
      break;

    default:
      readCBPcoeffs = NULL;
      break;
    }
  }
//}}}
//{{{
void cSlice::setReadCbpCavlc() {

  switch (decoder->activeSps->chromaFormatIdc) {
    case YUV444:
      if (decoder->coding.isSeperateColourPlane == 0)
        readCBPcoeffs = readCbp444cavlc;
      else
        readCBPcoeffs = readCbp400cavlc;
      break;

    case YUV422:
      readCBPcoeffs = readCbp422cavlc;
      break;

    case YUV420:
      readCBPcoeffs = readCbp420cavlc;
      break;

    case YUV400:
      readCBPcoeffs = readCbp400cavlc;
      break;

    default:
      readCBPcoeffs = NULL;
      break;
    }
  }
//}}}

//{{{
void cSlice::setQuant() {

  cSps* sps = activeSps;
  cPps* pps = activePps;

  if (!pps->hasPicScalingMatrix && !sps->hasSeqScalingMatrix) {
    for (int i = 0; i < 12; i++)
      qmatrix[i] = (i < 6) ? kQuantOrg : kQuant8Org;
    }
  else {
    int n_ScalingList = (sps->chromaFormatIdc != YUV444) ? 8 : 12;
    if (sps->hasSeqScalingMatrix) {
      //{{{  check sps first
      for (int i = 0; i < n_ScalingList; i++) {
        if (i < 6) {
          if (!sps->hasSeqScalingList[i]) {
            // fall-back rule A
            if (i == 0)
              qmatrix[i] = kQuantIntraDefault;
            else if (i == 3)
              qmatrix[i] = kQuantInterDefault;
            else
              qmatrix[i] = qmatrix[i-1];
            }
          else {
            if (sps->useDefaultScalingMatrix4x4[i])
              qmatrix[i] = (i<3) ? kQuantIntraDefault : kQuantInterDefault;
            else
              qmatrix[i] = sps->scalingList4x4[i];
          }
        }
        else {
          if (!sps->hasSeqScalingList[i]) {
            // fall-back rule A
            if (i == 6)
              qmatrix[i] = kQuant8IntraDefault;
            else if (i == 7)
              qmatrix[i] = kQuant8InterDefault;
            else
              qmatrix[i] = qmatrix[i-2];
            }
          else {
            if (sps->useDefaultScalingMatrix8x8[i-6])
              qmatrix[i] = (i==6 || i==8 || i==10) ? kQuant8IntraDefault:kQuant8InterDefault;
            else
              qmatrix[i] = sps->scalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    if (pps->hasPicScalingMatrix) {
      //{{{  then check pps
      for (int i = 0; i < n_ScalingList; i++) {
        if (i < 6) {
          if (!pps->picScalingListPresentFlag[i]) {
            // fall-back rule B
            if (i == 0) {
              if (!sps->hasSeqScalingMatrix)
                qmatrix[i] = kQuantIntraDefault;
              }
            else if (i == 3) {
              if (!sps->hasSeqScalingMatrix)
                qmatrix[i] = kQuantInterDefault;
              }
            else
              qmatrix[i] = qmatrix[i-1];
            }
          else {
            if (pps->useDefaultScalingMatrix4x4Flag[i])
              qmatrix[i] = (i<3) ? kQuantIntraDefault:kQuantInterDefault;
            else
              qmatrix[i] = pps->scalingList4x4[i];
            }
          }
        else {
          if (!pps->picScalingListPresentFlag[i]) {
            // fall-back rule B
            if (i == 6) {
              if (!sps->hasSeqScalingMatrix)
                qmatrix[i] = kQuant8IntraDefault;
              }
            else if (i == 7) {
              if (!sps->hasSeqScalingMatrix)
                qmatrix[i] = kQuant8InterDefault;
              }
            else
              qmatrix[i] = qmatrix[i-2];
            }
          else {
            if (pps->useDefaultScalingMatrix8x8Flag[i-6])
              qmatrix[i] = (i==6 || i==8 || i==10) ? kQuant8IntraDefault:kQuant8InterDefault;
            else
              qmatrix[i] = pps->scalingList8x8[i-6];
            }
          }
        }
      }
      //}}}
    }

  calculateQuant4x4Param();
  if (pps->hasTransform8x8mode)
    calculateQuant8x8Param();
  }
//}}}
//{{{
void cSlice::fillWeightedPredParam() {

  if (sliceType == eB) {
    int maxL0Ref = numRefIndexActive[LIST_0];
    int maxL1Ref = numRefIndexActive[LIST_1];
    if (activePps->weightedBiPredIdc == 2) {
      lumaLog2weightDenom = 5;
      chromaLog2weightDenom = 5;
      wpRoundLuma = 16;
      wpRoundChroma = 16;
      for (int i = 0; i < MAX_REFERENCE_PICTURES; ++i)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? lumaLog2weightDenom : chromaLog2weightDenom;
          weightedPredWeight[0][i][comp] = 1 << logWeightDenom;
          weightedPredWeight[1][i][comp] = 1 << logWeightDenom;
          weightedPredOffset[0][i][comp] = 0;
          weightedPredOffset[1][i][comp] = 0;
          }
      }

    for (int i = 0; i < maxL0Ref; ++i)
      for (int j = 0; j < maxL1Ref; ++j)
        for (int comp = 0; comp < 3; ++comp) {
          int logWeightDenom = !comp ? lumaLog2weightDenom : chromaLog2weightDenom;
          if (activePps->weightedBiPredIdc == 1) {
            weightedBiPredWeight[0][i][j][comp] = weightedPredWeight[0][i][comp];
            weightedBiPredWeight[1][i][j][comp] = weightedPredWeight[1][j][comp];
            }
          else if (activePps->weightedBiPredIdc == 2) {
            int td = iClip3(-128,127,listX[LIST_1][j]->poc - listX[LIST_0][i]->poc);
            if (!td || listX[LIST_1][j]->usedLongTermRef || listX[LIST_0][i]->usedLongTermRef) {
              weightedBiPredWeight[0][i][j][comp] = 32;
              weightedBiPredWeight[1][i][j][comp] = 32;
              }
            else {
              int tb = iClip3(-128, 127, thisPoc - listX[LIST_0][i]->poc);
              int tx = (16384 + iabs (td / 2)) / td;
              int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
              weightedBiPredWeight[1][i][j][comp] = distScaleFactor >> 2;
              weightedBiPredWeight[0][i][j][comp] = 64 - weightedBiPredWeight[1][i][j][comp];
              if (weightedBiPredWeight[1][i][j][comp] < -64 ||
                  weightedBiPredWeight[1][i][j][comp] > 128) {
                weightedBiPredWeight[0][i][j][comp] = 32;
                weightedBiPredWeight[1][i][j][comp] = 32;
                weightedPredOffset[0][i][comp] = 0;
                weightedPredOffset[1][j][comp] = 0;
                }
              }
            }
          }

    if (mbAffFrame)
      for (int i = 0; i < 2 * maxL0Ref; ++i)
        for (int j = 0; j < 2 * maxL1Ref; ++j)
          for (int comp = 0; comp<3; ++comp)
            for (int k = 2; k < 6; k += 2) {
              weightedPredOffset[k+0][i][comp] = weightedPredOffset[0][i>>1][comp];
              weightedPredOffset[k+1][j][comp] = weightedPredOffset[1][j>>1][comp];
              int logWeightDenom = !comp ? lumaLog2weightDenom : chromaLog2weightDenom;
              if (activePps->weightedBiPredIdc == 1) {
                weightedBiPredWeight[k+0][i][j][comp] = weightedPredWeight[0][i>>1][comp];
                weightedBiPredWeight[k+1][i][j][comp] = weightedPredWeight[1][j>>1][comp];
                }
              else if (activePps->weightedBiPredIdc == 2) {
                int td = iClip3 (-128, 127, listX[k+LIST_1][j]->poc - listX[k+LIST_0][i]->poc);
                if (!td || listX[k+LIST_1][j]->usedLongTermRef || listX[k+LIST_0][i]->usedLongTermRef) {
                  weightedBiPredWeight[k+0][i][j][comp] = 32;
                  weightedBiPredWeight[k+1][i][j][comp] = 32;
                  }
                else {
                  int tb = iClip3 (-128,127,
                               ((k == 2) ? topPoc : botPoc) - listX[k+LIST_0][i]->poc);
                  int tx = (16384 + iabs(td/2)) / td;
                  int distScaleFactor = iClip3 (-1024, 1023, (tx*tb + 32 )>>6);
                  weightedBiPredWeight[k+1][i][j][comp] = distScaleFactor >> 2;
                  weightedBiPredWeight[k+0][i][j][comp] = 64 - weightedBiPredWeight[k+1][i][j][comp];
                  if (weightedBiPredWeight[k+1][i][j][comp] < -64 ||
                      weightedBiPredWeight[k+1][i][j][comp] > 128) {
                    weightedBiPredWeight[k+1][i][j][comp] = 32;
                    weightedBiPredWeight[k+0][i][j][comp] = 32;
                    weightedPredOffset[k+0][i][comp] = 0;
                    weightedPredOffset[k+1][j][comp] = 0;
                    }
                  }
                }
              }
    }
  }
//}}}
//{{{
void cSlice::resetWeightedPredParam() {

  for (int i = 0; i < MAX_REFERENCE_PICTURES; i++) {
    for (int comp = 0; comp < 3; comp++) {
      int logWeightDenom = (comp == 0) ? lumaLog2weightDenom : chromaLog2weightDenom;
      weightedPredWeight[0][i][comp] = 1 << logWeightDenom;
      weightedPredWeight[1][i][comp] = 1 << logWeightDenom;
      }
    }
  }
//}}}

//{{{
void cSlice::initMbAffLists (sPicture* noRefPicture) {
// Initialize listX[2..5] from lists 0 and 1
//  listX[2]: list0 for current_field == top
//  listX[3]: list1 for current_field == top
//  listX[4]: list0 for current_field == bottom
//  listX[5]: list1 for current_field == bottom

  for (int i = 2; i < 6; i++) {
    for (uint32_t j = 0; j < MAX_LIST_SIZE; j++)
      listX[i][j] = noRefPicture;
    listXsize[i] = 0;
    }

  for (int i = 0; i < listXsize[0]; i++) {
    listX[2][2*i] = listX[0][i]->topField;
    listX[2][2*i+1] = listX[0][i]->botField;
    listX[4][2*i] = listX[0][i]->botField;
    listX[4][2*i+1] = listX[0][i]->topField;
    }
  listXsize[2] = listXsize[4] = listXsize[0] * 2;

  for (int i = 0; i < listXsize[1]; i++) {
    listX[3][2*i] = listX[1][i]->topField;
    listX[3][2*i+1] = listX[1][i]->botField;
    listX[5][2*i] = listX[1][i]->botField;
    listX[5][2*i+1] = listX[1][i]->topField;
    }
  listXsize[3] = listXsize[5] = listXsize[1] * 2;
  }
//}}}
//{{{
void cSlice::copyPoc (cSlice* toSlice) {

  toSlice->topPoc = topPoc;
  toSlice->botPoc = botPoc;
  toSlice->thisPoc = thisPoc;
  toSlice->framePoc = framePoc;
  }
//}}}
//{{{
void cSlice::updatePicNum() {

  int maxFrameNum = 1 << (decoder->activeSps->log2maxFrameNumMinus4 + 4);

  if (picStructure == eFrame) {
    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->frameStoreRefArray[i]->isUsed == 3 ) {
        if (dpb->frameStoreRefArray[i]->frame->usedForRef &&
            !dpb->frameStoreRefArray[i]->frame->usedLongTermRef) {
          if (dpb->frameStoreRefArray[i]->frameNum > frameNum )
            dpb->frameStoreRefArray[i]->frameNumWrap = dpb->frameStoreRefArray[i]->frameNum - maxFrameNum;
          else
            dpb->frameStoreRefArray[i]->frameNumWrap = dpb->frameStoreRefArray[i]->frameNum;
          dpb->frameStoreRefArray[i]->frame->picNum = dpb->frameStoreRefArray[i]->frameNumWrap;
          }
        }
      }

    // update longTermPicNum
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->frameStoreLongTermRefArray[i]->isUsed == 3) {
        if (dpb->frameStoreLongTermRefArray[i]->frame->usedLongTermRef)
          dpb->frameStoreLongTermRefArray[i]->frame->longTermPicNum = dpb->frameStoreLongTermRefArray[i]->frame->longTermFrameIndex;
        }
      }
    }
  else {
    int addTop = 0;
    int addBot = 0;
    if (picStructure == eTopField)
      addTop = 1;
    else
      addBot = 1;

    for (uint32_t i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->frameStoreRefArray[i]->usedRef) {
        if (dpb->frameStoreRefArray[i]->frameNum > frameNum )
          dpb->frameStoreRefArray[i]->frameNumWrap = dpb->frameStoreRefArray[i]->frameNum - maxFrameNum;
        else
          dpb->frameStoreRefArray[i]->frameNumWrap = dpb->frameStoreRefArray[i]->frameNum;
        if (dpb->frameStoreRefArray[i]->usedRef & 1) {
          if (dpb->frameStoreRefArray[i]->topField)
            dpb->frameStoreRefArray[i]->topField->picNum = (2 * dpb->frameStoreRefArray[i]->frameNumWrap) + addTop;
          else
            cLog::log (LOGERROR, fmt::format ("no topField for {}", i));
          }
        if (dpb->frameStoreRefArray[i]->usedRef & 2) {
          if (dpb->frameStoreRefArray[i]->botField)
            dpb->frameStoreRefArray[i]->botField->picNum = (2 * dpb->frameStoreRefArray[i]->frameNumWrap) + addBot;
          else
            cLog::log (LOGERROR, fmt::format ("no botField for {}", i));
          }
        }
      }

    // update longTermPicNum
    for (uint32_t i = 0; i < dpb->longTermRefFramesInBuffer; i++) {
      if (dpb->frameStoreLongTermRefArray[i]->usedLongTermRef & 1)
        dpb->frameStoreLongTermRefArray[i]->topField->longTermPicNum = 2 * dpb->frameStoreLongTermRefArray[i]->topField->longTermFrameIndex + addTop;
      if (dpb->frameStoreLongTermRefArray[i]->usedLongTermRef & 2)
        dpb->frameStoreLongTermRefArray[i]->botField->longTermPicNum = 2 * dpb->frameStoreLongTermRefArray[i]->botField->longTermFrameIndex + addBot;
      }
    }
  }
//}}}

//{{{
void cSlice::allocRefPicListReordeBuffer() {

  if ((sliceType != eI) && (sliceType != eSI)) {
    // B,P
    int size = numRefIndexActive[LIST_0] + 1;
    modPicNumsIdc[LIST_0] = (int*)calloc (size ,sizeof(int));
    longTermPicIndex[LIST_0] = (int*)calloc (size,sizeof(int));
    absDiffPicNumMinus1[LIST_0] = (int*)calloc (size,sizeof(int));
    }
  else {
    modPicNumsIdc[LIST_0] = NULL;
    longTermPicIndex[LIST_0] = NULL;
    absDiffPicNumMinus1[LIST_0] = NULL;
    }

  if (sliceType == eB) {
    // B
    int size = numRefIndexActive[LIST_1] + 1;
    modPicNumsIdc[LIST_1] = (int*)calloc (size,sizeof(int));
    longTermPicIndex[LIST_1] = (int*)calloc (size,sizeof(int));
    absDiffPicNumMinus1[LIST_1] = (int*)calloc (size,sizeof(int));
    }
  else {
    modPicNumsIdc[LIST_1] = NULL;
    longTermPicIndex[LIST_1] = NULL;
    absDiffPicNumMinus1[LIST_1] = NULL;
    }
  }
//}}}
//{{{
void cSlice::freeRefPicListReorderBuffer() {

  free (modPicNumsIdc[LIST_0]);
  modPicNumsIdc[LIST_0] = NULL;

  free (absDiffPicNumMinus1[LIST_0]);
  absDiffPicNumMinus1[LIST_0] = NULL;

  free (longTermPicIndex[LIST_0]);
  longTermPicIndex[LIST_0] = NULL;

  free (modPicNumsIdc[LIST_1]);
  modPicNumsIdc[LIST_1] = NULL;

  free (absDiffPicNumMinus1[LIST_1]);
  absDiffPicNumMinus1[LIST_1] = NULL;

  free (longTermPicIndex[LIST_1]);
  longTermPicIndex[LIST_1] = NULL;
  }
//}}}
//{{{
void cSlice::reorderRefPicList (int curList) {

  int* curModPicNumsIdc = modPicNumsIdc[curList];
  int* curAbsDiffPicNumMinus1 = absDiffPicNumMinus1[curList];
  int* curLongTermPicIndex = longTermPicIndex[curList];
  int numRefIndexIXactiveMinus1 = numRefIndexActive[curList] - 1;

  int maxPicNum;
  int currPicNum;
  if (picStructure == eFrame) {
    maxPicNum = decoder->coding.maxFrameNum;
    currPicNum = frameNum;
    }
  else {
    maxPicNum = 2 * decoder->coding.maxFrameNum;
    currPicNum = 2 * frameNum + 1;
    }

  int picNumLX;
  int picNumLXNoWrap;
  int picNumLXPred = currPicNum;
  int refIdxLX = 0;
  for (int i = 0; curModPicNumsIdc[i] != 3; i++) {
    if (curModPicNumsIdc[i] > 3)
      cDecoder264::error ("Invalid modPicNumsIdc command");
    if (curModPicNumsIdc[i] < 2) {
      if (curModPicNumsIdc[i] == 0) {
        if (picNumLXPred - (curAbsDiffPicNumMinus1[i]+1) < 0)
          picNumLXNoWrap = picNumLXPred - (curAbsDiffPicNumMinus1[i]+1) + maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred - (curAbsDiffPicNumMinus1[i]+1);
        }
      else {
        if (picNumLXPred + curAbsDiffPicNumMinus1[i]+1 >= maxPicNum)
          picNumLXNoWrap = picNumLXPred + (curAbsDiffPicNumMinus1[i]+1) - maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred + (curAbsDiffPicNumMinus1[i]+1);
        }

      picNumLXPred = picNumLXNoWrap;
      if (picNumLXNoWrap > currPicNum)
        picNumLX = picNumLXNoWrap - maxPicNum;
      else
        picNumLX = picNumLXNoWrap;

      reorderShortTerm (curList, numRefIndexIXactiveMinus1, picNumLX, &refIdxLX);
      }
    else
      reorderLongTerm (listX[curList], numRefIndexIXactiveMinus1, curLongTermPicIndex[i], &refIdxLX);
    }

  listXsize[curList] = (char)(numRefIndexIXactiveMinus1 + 1);
  }
//}}}
//{{{
void cSlice::computeColocated (sPicture** listX[6]) {

  if (directSpatialMvPredFlag == 0) {
    for (int j = 0; j < 2 + (mbAffFrame * 4); j += 2) {
      for (int i = 0; i < listXsize[j];i++) {
        int iTRb;
        if (j == 0)
          iTRb = iClip3 (-128, 127, decoder->picture->poc - listX[LIST_0 + j][i]->poc);
        else if (j == 2)
          iTRb = iClip3 (-128, 127, decoder->picture->topPoc - listX[LIST_0 + j][i]->poc);
        else
          iTRb = iClip3 (-128, 127, decoder->picture->botPoc - listX[LIST_0 + j][i]->poc);

        int iTRp = iClip3 (-128, 127, listX[LIST_1 + j][0]->poc - listX[LIST_0 + j][i]->poc);
        if (iTRp != 0) {
          int prescale = (16384 + iabs( iTRp / 2)) / iTRp;
          mvscale[j][i] = iClip3 (-1024, 1023, (iTRb * prescale + 32) >> 6) ;
          }
        else
          mvscale[j][i] = 9999;
        }
     }
    }
  }
//}}}

// private:
//{{{
void cSlice::calculateQuant4x4Param() {

  const int (*p_dequant_coef)[4][4] = kDequantCoef;
  int (*InvLevelScale4x4_Intra_0)[4][4] = InvLevelScale4x4_Intra[0];
  int (*InvLevelScale4x4_Intra_1)[4][4] = InvLevelScale4x4_Intra[1];
  int (*InvLevelScale4x4_Intra_2)[4][4] = InvLevelScale4x4_Intra[2];
  int (*InvLevelScale4x4_Inter_0)[4][4] = InvLevelScale4x4_Inter[0];
  int (*InvLevelScale4x4_Inter_1)[4][4] = InvLevelScale4x4_Inter[1];
  int (*InvLevelScale4x4_Inter_2)[4][4] = InvLevelScale4x4_Inter[2];

  for (int k = 0; k < 6; k++) {
    setDequant4x4 (*InvLevelScale4x4_Intra_0++, *p_dequant_coef, qmatrix[0]);
    setDequant4x4 (*InvLevelScale4x4_Intra_1++, *p_dequant_coef, qmatrix[1]);
    setDequant4x4 (*InvLevelScale4x4_Intra_2++, *p_dequant_coef, qmatrix[2]);
    setDequant4x4 (*InvLevelScale4x4_Inter_0++, *p_dequant_coef, qmatrix[3]);
    setDequant4x4 (*InvLevelScale4x4_Inter_1++, *p_dequant_coef, qmatrix[4]);
    setDequant4x4 (*InvLevelScale4x4_Inter_2++, *p_dequant_coef++, qmatrix[5]);
    }
  }
//}}}
//{{{
void cSlice::calculateQuant8x8Param() {

  const int (*p_dequant_coef)[8][8] = kDequantCoef8;
  int (*InvLevelScale8x8_Intra_0)[8][8] = InvLevelScale8x8_Intra[0];
  int (*InvLevelScale8x8_Intra_1)[8][8] = InvLevelScale8x8_Intra[1];
  int (*InvLevelScale8x8_Intra_2)[8][8] = InvLevelScale8x8_Intra[2];
  int (*InvLevelScale8x8_Inter_0)[8][8] = InvLevelScale8x8_Inter[0];
  int (*InvLevelScale8x8_Inter_1)[8][8] = InvLevelScale8x8_Inter[1];
  int (*InvLevelScale8x8_Inter_2)[8][8] = InvLevelScale8x8_Inter[2];

  for (int k = 0; k < 6; k++) {
    setDequant8x8 (*InvLevelScale8x8_Intra_0++, *p_dequant_coef  , qmatrix[6]);
    setDequant8x8 (*InvLevelScale8x8_Inter_0++, *p_dequant_coef++, qmatrix[7]);
    }

  p_dequant_coef = kDequantCoef8;
  if (activeSps->chromaFormatIdc == 3) {
    // 4:4:4
    for (int k = 0; k < 6; k++) {
      setDequant8x8 (*InvLevelScale8x8_Intra_1++, *p_dequant_coef, qmatrix[8]);
      setDequant8x8 (*InvLevelScale8x8_Inter_1++, *p_dequant_coef, qmatrix[9]);
      setDequant8x8 (*InvLevelScale8x8_Intra_2++, *p_dequant_coef, qmatrix[10]);
      setDequant8x8 (*InvLevelScale8x8_Inter_2++, *p_dequant_coef++, qmatrix[11]);
      }
    }
  }
//}}}

//{{{
void cSlice::reorderShortTerm (int curList, int numRefIndexIXactiveMinus1, int picNumLX, int* refIdxLX) {

  sPicture** refPicListX = listX[curList];
  sPicture* picLX = dpb->getShortTermPic (this, picNumLX);

  for (int cIdx = numRefIndexIXactiveMinus1+1; cIdx > *refIdxLX; cIdx--)
    refPicListX[cIdx] = refPicListX[cIdx - 1];
  refPicListX[(*refIdxLX)++] = picLX;

  int nIdx = *refIdxLX;
  for (int cIdx = *refIdxLX; cIdx <= numRefIndexIXactiveMinus1+1; cIdx++)
    if (refPicListX[cIdx])
      if ((refPicListX[cIdx]->usedLongTermRef) || (refPicListX[cIdx]->picNum != picNumLX))
        refPicListX[nIdx++] = refPicListX[cIdx];
  }
//}}}
//{{{
void cSlice::reorderLongTerm (sPicture** refPicListX, int numRefIndexIXactiveMinus1,
                              int LongTermPicNum, int* refIdxLX) {

  sPicture* picLX = dpb->getLongTermPic (this, LongTermPicNum);

  for (int cIdx = numRefIndexIXactiveMinus1+1; cIdx > *refIdxLX; cIdx--)
    refPicListX[cIdx] = refPicListX[cIdx - 1];
  refPicListX[(*refIdxLX)++] = picLX;

  int nIdx = *refIdxLX;
  for (int cIdx = *refIdxLX; cIdx <= numRefIndexIXactiveMinus1+1; cIdx++)
    if (refPicListX[cIdx])
      if ((!refPicListX[cIdx]->usedLongTermRef) || (refPicListX[cIdx]->longTermPicNum != LongTermPicNum))
        refPicListX[nIdx++] = refPicListX[cIdx];
  }
//}}}
