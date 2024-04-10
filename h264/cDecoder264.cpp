//{{{  includes
#include "global.h"
#include "memory.h"

#include "cCabacDecode.h"
#include "cabac.h"
#include "erc.h"
#include "loopfilter.h"
#include "macroblock.h"
#include "mcPred.h"
#include "nalu.h"
#include "sei.h"

#include "../common/cLog.h"

using namespace std;
//}}}
namespace {
  //{{{  const tables
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

  const int kSubWidthC[]= { 1, 2, 2, 1 };
  const int kSubHeightC[]= { 1, 2, 1, 1 };
  //}}}
  //{{{
  void initCabacContexts (cSlice* slice) {

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

    sMotionContexts* mc = slice->motionContexts;
    sTextureContexts* tc = slice->textureContexts;

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
  void padBuf (sPixel* pixel, int width, int height, int stride, int padx, int pady) {

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
  void copyDecPictureJV (sPicture* dst, sPicture* src) {

    dst->poc = src->poc;
    dst->topPoc = src->topPoc;
    dst->botPoc = src->botPoc;
    dst->framePoc = src->framePoc;

    dst->qp = src->qp;
    dst->sliceQpDelta = src->sliceQpDelta;
    dst->chromaQpOffset[0] = src->chromaQpOffset[0];
    dst->chromaQpOffset[1] = src->chromaQpOffset[1];

    dst->sliceType = src->sliceType;
    dst->usedForRef = src->usedForRef;
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
  void updateMbAff (sPixel** pixel, sPixel (*temp)[16], int x0, int width, int height) {

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
  void ercWriteMbModeMv (sMacroBlock* mb) {

    cDecoder264* decoder = mb->decoder;
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
  void copyCropped (uint8_t* buf, sPixel** imgX, int sizeX, int sizeY, int symbolSizeInBytes,
                    int cropLeft, int cropRight, int cropTop, int cropBot, int outStride) {

    if (cropLeft || cropRight) {
      for (int i = cropTop; i < sizeY - cropBot; i++) {
        int pos = ((i - cropTop) * outStride) + cropLeft;
        memcpy (buf + pos, imgX[i], sizeX - cropRight - cropLeft);
        }
      }
    else {
      buf += cropTop * outStride;
      for (int i = 0; i < sizeY - cropBot - cropTop; i++) {
        memcpy (buf, imgX[i], sizeX);
        buf += outStride;
        }
      }
    }
  //}}}
  //{{{
  sDecodedPic* allocDecodedPicture (sDecodedPic* decodedPic) {

    sDecodedPic* prevDecodedPicture = NULL;
    while (decodedPic && (decodedPic->ok)) {
      prevDecodedPicture = decodedPic;
      decodedPic = decodedPic->next;
      }

    if (!decodedPic) {
      decodedPic = (sDecodedPic*)calloc (1, sizeof(*decodedPic));
      prevDecodedPicture->next = decodedPic;
      }

    return decodedPic;
    }
  //}}}
  //{{{
  void freeDecodedPictures (sDecodedPic* decodedPic) {

    while (decodedPic) {
      sDecodedPic* nextDecodedPic = decodedPic->next;
      if (decodedPic->yBuf) {
        free (decodedPic->yBuf);
        decodedPic->yBuf = NULL;
        decodedPic->uBuf = NULL;
        decodedPic->vBuf = NULL;
        }

      free (decodedPic);
      decodedPic = nextDecodedPic;
     }
    }
  //}}}
  }

// cDecoder264
//{{{
cDecoder264* cDecoder264::open (sParam* param, uint8_t* chunk, size_t chunkSize) {

  // alloc decoder
  cDecoder264* decoder = new (cDecoder264);

  initTime();

  // init param
  memcpy (&decoder->param, param, sizeof(sParam));
  decoder->concealMode = param->concealMode;

  // init nalu, annexB
  decoder->nalu = new cNalu (sDataPartition::MAX_CODED_FRAME_SIZE);
  decoder->annexB = new cAnnexB (decoder);
  decoder->annexB->open (chunk, chunkSize);

  // init slice
  decoder->sliceList = (cSlice**)calloc (MAX_NUM_DECSLICES, sizeof(cSlice*));
  decoder->numAllocatedSlices = MAX_NUM_DECSLICES;
  decoder->oldSlice = (sOldSlice*)malloc(sizeof(sOldSlice));
  decoder->coding.sliceType = eSliceI;
  decoder->recoveryPoc = 0x7fffffff;
  decoder->deblockEnable = 0x3;
  decoder->pendingOutPicStructure = eFrame;

  decoder->coding.lumaPadX = MCBUF_LUMA_PAD_X;
  decoder->coding.lumaPadY = MCBUF_LUMA_PAD_Y;
  decoder->coding.chromaPadX = MCBUF_CHROMA_PAD_X;
  decoder->coding.chromaPadY = MCBUF_CHROMA_PAD_Y;

  decoder->dpb.decoder = decoder;

  decoder->outDecodedPics = (sDecodedPic*)calloc (1, sizeof(sDecodedPic));

  // alloc output
  decoder->outBuffer = new cFrameStore();
  decoder->pendingOut = (sPicture*)calloc (sizeof(sPicture), 1);
  decoder->pendingOut->imgUV = NULL;
  decoder->pendingOut->imgY = NULL;

  // global only for vlc
  gDecoder = decoder;
  return decoder;
  }
//}}}
//{{{
void cDecoder264::error (const string& text) {

  cLog::log (LOGERROR, text);
  if (cDecoder264::gDecoder)
    cDecoder264::gDecoder->dpb.flushDpb();

  exit (0);
  }
//}}}
//{{{
cDecoder264::~cDecoder264() {

  free (annexB);
  free (oldSlice);
  free (nextSlice);

  if (sliceList) {
    for (int i = 0; i < numAllocatedSlices; i++)
      if (sliceList[i])
        free (sliceList[i]);
    free (sliceList);
    }

  free (nalu);

  freeDecodedPictures (outDecodedPics);
  }
//}}}

//{{{  conceal
//{{{
void cDecoder264::reset_ecFlags () {

  for (int i = 0; i < SE_MAX_ELEMENTS; i++)
    ecFlag[i] = NO_EC;
  }
//}}}
//{{{
int cDecoder264::setEcFlag (int se) {

  switch (se) {
    case SE_HEADER :
      ecFlag[SE_HEADER] = EC_REQ;
    case SE_PTYPE :
      ecFlag[SE_PTYPE] = EC_REQ;
    case SE_MBTYPE :
      ecFlag[SE_MBTYPE] = EC_REQ;
    //{{{
    case SE_REFFRAME :
      ecFlag[SE_REFFRAME] = EC_REQ;
      ecFlag[SE_MVD] = EC_REQ; // set all motion vectors to zero length
      se = SE_CBP_INTER;      // conceal also Inter texture elements
      break;
    //}}}

    //{{{
    case SE_INTRAPREDMODE :
      ecFlag[SE_INTRAPREDMODE] = EC_REQ;
      se = SE_CBP_INTRA;      // conceal also Intra texture elements
      break;
    //}}}
    //{{{
    case SE_MVD :
      ecFlag[SE_MVD] = EC_REQ;
      se = SE_CBP_INTER;      // conceal also Inter texture elements
      break;
    //}}}
    //{{{
    default:
      break;
    //}}}
    }

  switch (se) {
    case SE_CBP_INTRA :
      ecFlag[SE_CBP_INTRA] = EC_REQ;
    case SE_LUM_DC_INTRA :
      ecFlag[SE_LUM_DC_INTRA] = EC_REQ;
    case SE_CHR_DC_INTRA :
      ecFlag[SE_CHR_DC_INTRA] = EC_REQ;
    case SE_LUM_AC_INTRA :
      ecFlag[SE_LUM_AC_INTRA] = EC_REQ;
    //{{{
    case SE_CHR_AC_INTRA :
      ecFlag[SE_CHR_AC_INTRA] = EC_REQ;
      break;
    //}}}

    case SE_CBP_INTER :
      ecFlag[SE_CBP_INTER] = EC_REQ;
    case SE_LUM_DC_INTER :
      ecFlag[SE_LUM_DC_INTER] = EC_REQ;
    case SE_CHR_DC_INTER :
      ecFlag[SE_CHR_DC_INTER] = EC_REQ;
    case SE_LUM_AC_INTER :
      ecFlag[SE_LUM_AC_INTER] = EC_REQ;
    //{{{
    case SE_CHR_AC_INTER :
      ecFlag[SE_CHR_AC_INTER] = EC_REQ;
      break;
    //}}}

    //{{{
    case SE_DELTA_QUANT_INTER :
      ecFlag[SE_DELTA_QUANT_INTER] = EC_REQ;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTRA :
      ecFlag[SE_DELTA_QUANT_INTRA] = EC_REQ;
      break;
    //}}}
    //{{{
    default:
      break;
    }
    //}}}

  return EC_REQ;
  }
//}}}
//{{{
int cDecoder264::getConcealElement (sSyntaxElement* se) {

  if (ecFlag[se->type] == NO_EC)
    return NO_EC;

  switch (se->type) {
    //{{{
    case SE_HEADER :
      se->len = 31;
      se->inf = 0; // Picture Header
      break;
    //}}}

    case SE_PTYPE : // inter_img_1
    case SE_MBTYPE : // set COPY_MB
    //{{{
    case SE_REFFRAME :
      se->len = 1;
      se->inf = 0;
      break;
    //}}}

    case SE_INTRAPREDMODE :
    //{{{
    case SE_MVD :
      se->len = 1;
      se->inf = 0;  // set vector to zero length
      break;
    //}}}

    case SE_LUM_DC_INTRA :
    case SE_CHR_DC_INTRA :
    case SE_LUM_AC_INTRA :
    //{{{
    case SE_CHR_AC_INTRA :
      se->len = 1;
      se->inf = 0;  // return EOB
      break;
    //}}}

    case SE_LUM_DC_INTER :
    case SE_CHR_DC_INTER :
    case SE_LUM_AC_INTER :
    //{{{
    case SE_CHR_AC_INTER :
      se->len = 1;
      se->inf = 0;  // return EOB
      break;
    //}}}

    //{{{
    case SE_CBP_INTRA :
      se->len = 5;
      se->inf = 0; // codenumber 3 <=> no CBP information for INTRA images
      break;
    //}}}
    //{{{
    case SE_CBP_INTER :
      se->len = 1;
      se->inf = 0; // codenumber 1 <=> no CBP information for INTER images
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTER:
      se->len = 1;
      se->inf = 0;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTRA:
      se->len = 1;
      se->inf = 0;
      break;
    //}}}
    //{{{
    default:
      break;
    //}}}
    }

  return EC_REQ;
  }
//}}}
//}}}
//{{{  fmo
//{{{
int cDecoder264::fmoGetNextMBNr (int CurrentMbNr) {

  int SliceGroup = fmoGetSliceGroupId (CurrentMbNr);

  while (++CurrentMbNr<(int)picSizeInMbs &&
         mbToSliceGroupMap [CurrentMbNr] != SliceGroup) ;

  if (CurrentMbNr >= (int)picSizeInMbs)
    return -1;    // No further MB in this slice (could be end of picture)
  else
    return CurrentMbNr;
  }
//}}}
//}}}
//{{{
void cDecoder264::padPicture (sPicture* picture) {

  padBuf (*picture->imgY, picture->sizeX, picture->sizeY,
           picture->lumaStride, coding.lumaPadX, coding.lumaPadY);

  if (picture->chromaFormatIdc != YUV400) {
    padBuf (*picture->imgUV[0], picture->sizeXcr, picture->sizeYcr,
            picture->chromaStride, coding.chromaPadX, coding.chromaPadY);
    padBuf (*picture->imgUV[1], picture->sizeXcr, picture->sizeYcr,
            picture->chromaStride, coding.chromaPadX, coding.chromaPadY);
    }
  }
//}}}
//{{{
void cDecoder264::decodePOC (cSlice* slice) {

  uint32_t maxPicOrderCntLsb = (1<<(activeSps->log2maxPocLsbMinus4+4));

  switch (activeSps->pocType) {
    //{{{
    case 0: // POC MODE 0
      // 1st
      if (slice->isIDR) {
        prevPocMsb = 0;
        prevPocLsb = 0;
        }
      else if (lastHasMmco5) {
        if (lastPicBotField) {
          prevPocMsb = 0;
          prevPocLsb = 0;
          }
        else {
          prevPocMsb = 0;
          prevPocLsb = slice->topPoc;
          }
        }

      // Calculate the MSBs of current picture
      if (slice->picOrderCountLsb < prevPocLsb  &&
          (prevPocLsb - slice->picOrderCountLsb) >= maxPicOrderCntLsb/2)
        slice->PicOrderCntMsb = prevPocMsb + maxPicOrderCntLsb;
      else if (slice->picOrderCountLsb > prevPocLsb &&
               (slice->picOrderCountLsb - prevPocLsb) > maxPicOrderCntLsb/2)
        slice->PicOrderCntMsb = prevPocMsb - maxPicOrderCntLsb;
      else
        slice->PicOrderCntMsb = prevPocMsb;

      // 2nd
      if (slice->fieldPic == 0) {
        // frame pixelPos
        slice->topPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
        slice->botPoc = slice->topPoc + slice->deltaPicOrderCountBot;
        slice->thisPoc = slice->framePoc = (slice->topPoc < slice->botPoc) ? slice->topPoc : slice->botPoc;
        }
      else if (!slice->botField) // top field
        slice->thisPoc= slice->topPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
      else // bottom field
        slice->thisPoc= slice->botPoc = slice->PicOrderCntMsb + slice->picOrderCountLsb;
      slice->framePoc = slice->thisPoc;

      thisPoc = slice->thisPoc;
      previousFrameNum = slice->frameNum;

      if (slice->refId) {
        prevPocLsb = slice->picOrderCountLsb;
        prevPocMsb = slice->PicOrderCntMsb;
        }

      break;
    //}}}
    //{{{
    case 1: // POC MODE 1
      // 1st
      if (slice->isIDR) {
        // first pixelPos of IDRGOP,
        frameNumOffset = 0;
        if (slice->frameNum)
          cDecoder264::error ("frameNum nonZero IDR picture");
        }
      else {
        if (lastHasMmco5) {
          previousFrameNumOffset = 0;
          previousFrameNum = 0;
          }
        if (slice->frameNum<previousFrameNum) // not first pixelPos of IDRGOP
          frameNumOffset = previousFrameNumOffset + coding.maxFrameNum;
        else
          frameNumOffset = previousFrameNumOffset;
        }

      // 2nd
      if (activeSps->numRefFramesPocCycle)
        slice->AbsFrameNum = frameNumOffset + slice->frameNum;
      else
        slice->AbsFrameNum = 0;
      if (!slice->refId && (slice->AbsFrameNum > 0))
        slice->AbsFrameNum--;

      // 3rd
      expectedDeltaPerPocCycle = 0;
      if (activeSps->numRefFramesPocCycle)
        for (uint32_t i = 0; i < activeSps->numRefFramesPocCycle; i++)
          expectedDeltaPerPocCycle += activeSps->offsetForRefFrame[i];

      if (slice->AbsFrameNum) {
        pocCycleCount = (slice->AbsFrameNum-1) / activeSps->numRefFramesPocCycle;
        frameNumPocCycle = (slice->AbsFrameNum-1) % activeSps->numRefFramesPocCycle;
        expectedPOC =
          pocCycleCount*expectedDeltaPerPocCycle;
        for (int i = 0; i <= frameNumPocCycle; i++)
          expectedPOC += activeSps->offsetForRefFrame[i];
        }
      else
        expectedPOC = 0;

      if (!slice->refId)
        expectedPOC += activeSps->offsetNonRefPic;

      if (slice->fieldPic == 0) {
        // frame pixelPos
        slice->topPoc = expectedPOC + slice->deltaPicOrderCount[0];
        slice->botPoc = slice->topPoc + activeSps->offsetTopBotField + slice->deltaPicOrderCount[1];
        slice->thisPoc = slice->framePoc = (slice->topPoc < slice->botPoc) ? slice->topPoc : slice->botPoc;
        }
      else if (!slice->botField) // top field
        slice->thisPoc = slice->topPoc = expectedPOC + slice->deltaPicOrderCount[0];
      else // bottom field
        slice->thisPoc = slice->botPoc = expectedPOC + activeSps->offsetTopBotField + slice->deltaPicOrderCount[0];
      slice->framePoc=slice->thisPoc;

      previousFrameNum = slice->frameNum;
      previousFrameNumOffset = frameNumOffset;
      break;
    //}}}
    //{{{
    case 2: // POC MODE 2
      if (slice->isIDR) {
        // IDR picture, first pixelPos of IDRGOP,
        frameNumOffset = 0;
        slice->thisPoc = slice->framePoc = slice->topPoc = slice->botPoc = 0;
        if (slice->frameNum)
          cDecoder264::error ("frameNum not equal to zero in IDR picture");
        }
      else {
        if (lastHasMmco5) {
          previousFrameNum = 0;
          previousFrameNumOffset = 0;
          }

        if (slice->frameNum<previousFrameNum)
          frameNumOffset = previousFrameNumOffset + coding.maxFrameNum;
        else
          frameNumOffset = previousFrameNumOffset;

        slice->AbsFrameNum = frameNumOffset+slice->frameNum;
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

      previousFrameNum = slice->frameNum;
      previousFrameNumOffset = frameNumOffset;
      break;
    //}}}
    //{{{
    default:
      cDecoder264::error ("unknown POC type");
      break;
    //}}}
    }
  }
//}}}
//{{{
void cDecoder264::changePlaneJV (int nplane, cSlice* slice) {

  mbData = mbDataJV[nplane];
  picture  = decPictureJV[nplane];
  siBlock = siBlockJV[nplane];
  predMode = predModeJV[nplane];
  intraBlock = intraBlockJV[nplane];

  if (slice) {
    slice->mbData = mbDataJV[nplane];
    slice->picture  = decPictureJV[nplane];
    slice->siBlock = siBlockJV[nplane];
    slice->predMode = predModeJV[nplane];
    slice->intraBlock = intraBlockJV[nplane];
    }
  }
//}}}

//{{{
void cDecoder264::directOutput (sPicture* picture) {

  if (picture->picStructure == eFrame) {
    // we have a frame (or complementary field pair), so output it directly
    flushDirectOutput();
    writePicture (picture, eFrame);
    freePicture (picture);
    return;
    }

  if (picture->picStructure == eTopField) {
    if (outBuffer->isUsed & 1)
      flushDirectOutput();
    outBuffer->topField = picture;
    outBuffer->isUsed |= 1;
    }

  if (picture->picStructure == eBotField) {
    if (outBuffer->isUsed & 2)
      flushDirectOutput();
    outBuffer->botField = picture;
    outBuffer->isUsed |= 2;
    }

  if (outBuffer->isUsed == 3) {
    // we have both fields, so output them
    outBuffer->dpbCombineField (this);
    writePicture (outBuffer->frame, eFrame);
    freePicture (outBuffer->frame);

    outBuffer->frame = NULL;
    freePicture (outBuffer->topField);

    outBuffer->topField = NULL;
    freePicture (outBuffer->botField);

    outBuffer->botField = NULL;
    outBuffer->isUsed = 0;
    }
  }
//}}}
//{{{
void cDecoder264::writeStoredFrame (cFrameStore* frameStore) {

  // make sure no direct output field is pending
  flushDirectOutput();

  if (frameStore->isUsed < 3)
    writeUnpairedField (frameStore);
  else {
    if (frameStore->recoveryFrame)
      recoveryFlag = 1;
    if ((!nonConformingStream) || recoveryFlag)
      writePicture (frameStore->frame, eFrame);
    }

  frameStore->isOutput = 1;
  }
//}}}

//{{{
int cDecoder264::decodeOneFrame (sDecodedPic** decPicList) {

  int result = 0;

  clearDecodedPics();

  int decodeFrameResult = decodeFrame();
  if (decodeFrameResult == eSOP)
    result = DEC_SUCCEED;
  else if (decodeFrameResult == eEOS)
    result = DEC_EOS;
  else
    result |= DEC_ERRMASK;

  *decPicList = outDecodedPics;
  return result;
  }
//}}}
//{{{
void cDecoder264::finish (sDecodedPic** decPicList) {

  clearDecodedPics();
  dpb.flushDpb();

  annexB->reset();

  newFrame = 0;
  prevFrameNum = 0;
  *decPicList = outDecodedPics;
  }
//}}}
//{{{
void cDecoder264::close() {

  //closeFmo (this);
  freeLayerBuffers();
  freeGlobalBuffers();

  ercClose (this, ercErrorVar);

  for (uint32_t i = 0; i < kMaxPps; i++) {
    if (pps[i].ok && pps[i].sliceGroupId)
      free (pps[i].sliceGroupId);
    pps[i].ok = false;
    }

  dpb.freeDpb();

  // free output
  delete outBuffer;
  flushPendingOut();
  free (pendingOut);

  gDecoder = NULL;
  }
//}}}

//{{{
void cDecoder264::clearDecodedPics() {

  // find the head first;
  sDecodedPic* prevDecodedPicture = NULL;
  sDecodedPic* decodedPic = outDecodedPics;
  while (decodedPic && !decodedPic->ok) {
    prevDecodedPicture = decodedPic;
    decodedPic = decodedPic->next;
    }

  if (decodedPic && (decodedPic != outDecodedPics)) {
    // move all nodes before decodedPic to the end;
    sDecodedPic* decodedPictureTail = decodedPic;
    while (decodedPictureTail->next)
      decodedPictureTail = decodedPictureTail->next;

    decodedPictureTail->next = outDecodedPics;
    outDecodedPics = decodedPic;
    prevDecodedPicture->next = NULL;
    }
  }
//}}}
//{{{
void cDecoder264::initPictureDecode() {

  deblockMode = 1;

  if (picSliceIndex >= MAX_NUM_SLICES)
    cDecoder264::error ("initPictureDecode - MAX_NUM_SLICES exceeded");

  cSlice* slice = sliceList[0];
  useParameterSet (slice);

  if (slice->isIDR)
    idrFrameNum = 0;

  picHeightInMbs = coding.frameHeightMbs / (1 + slice->fieldPic);
  picSizeInMbs = coding.picWidthMbs * picHeightInMbs;
  coding.frameSizeMbs = coding.picWidthMbs * coding.frameHeightMbs;
  coding.picStructure = slice->picStructure;
  initFmo (slice);

  slice->updatePicNum();

  initDeblock (this, slice->mbAffFrame);
  for (int j = 0; j < picSliceIndex; j++)
    if (sliceList[j]->deblockFilterDisableIdc != 1)
      deblockMode = 0;
  }
//}}}
//{{{
void cDecoder264::initGlobalBuffers() {

  // alloc coding
  if (coding.isSeperateColourPlane) {
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!mbDataJV[i])
        mbDataJV[i] = (sMacroBlock*)calloc (coding.frameSizeMbs, sizeof(sMacroBlock));
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!intraBlockJV[i])
        intraBlockJV[i] = (char*)calloc (coding.frameSizeMbs, sizeof(char));
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!predModeJV[i])
       getMem2D (&predModeJV[i], 4*coding.frameHeightMbs, 4*coding.picWidthMbs);
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!siBlockJV[i])
        getMem2Dint (&siBlockJV[i], coding.frameHeightMbs, coding.picWidthMbs);
    }
  else {
    if (!mbData)
      mbData = (sMacroBlock*)calloc (coding.frameSizeMbs, sizeof(sMacroBlock));
    if (!intraBlock)
      intraBlock = (char*)calloc (coding.frameSizeMbs, sizeof(char));

    if (!predMode)
      getMem2D (&predMode, 4*coding.frameHeightMbs, 4*coding.picWidthMbs);
    if (!siBlock)
      getMem2Dint (&siBlock, coding.frameHeightMbs, coding.picWidthMbs);
    }

  // alloc picPos
  if (!picPos) {
    picPos = (sBlockPos*)calloc (coding.frameSizeMbs + 1, sizeof(sBlockPos));
    sBlockPos* blockPos = picPos;
    for (uint32_t i = 0; i < coding.frameSizeMbs+1; ++i) {
      blockPos[i].x = (int16_t)(i % coding.picWidthMbs);
      blockPos[i].y = (int16_t)(i / coding.picWidthMbs);
      }
    }

  // alloc cavlc
  if (!nzCoeff)
    getMem4D (&nzCoeff, coding.frameSizeMbs, 3, BLOCK_SIZE, BLOCK_SIZE);

  // alloc quant
  int bitDepth_qp_scale = imax (coding.bitDepthLumaQpScale, coding.bitDepthChromaQpScale);

  if (!qpPerMatrix)
    qpPerMatrix = (int*)malloc ((MAX_QP + 1 + bitDepth_qp_scale) * sizeof(int));

  if (!qpRemMatrix)
    qpRemMatrix = (int*)malloc ((MAX_QP + 1 + bitDepth_qp_scale) * sizeof(int));

  for (int i = 0; i < MAX_QP + bitDepth_qp_scale + 1; i++) {
    qpPerMatrix[i] = i / 6;
    qpRemMatrix[i] = i % 6;
    }
  }
//}}}
//{{{
void cDecoder264::freeGlobalBuffers() {

  freePicture (picture);
  picture = NULL;
  }
//}}}
//{{{
void cDecoder264::freeLayerBuffers() {

  // free coding
  if (coding.isSeperateColourPlane) {
    for (int i = 0; i < MAX_PLANE; i++) {
      free (mbDataJV[i]);
      mbDataJV[i] = NULL;

      freeMem2Dint (siBlockJV[i]);
      siBlockJV[i] = NULL;

      freeMem2D (predModeJV[i]);
      predModeJV[i] = NULL;

      free (intraBlockJV[i]);
      intraBlockJV[i] = NULL;
      }
    }
  else {
    free (mbData);
    mbData = NULL;

    freeMem2Dint (siBlock);
    siBlock = NULL;

    freeMem2D (predMode);
    predMode = NULL;

    free (intraBlock);
    intraBlock = NULL;
    }

  // free picPos
  free (picPos);
  picPos = NULL;

  // free cavlc
  freeMem4D (nzCoeff);
  nzCoeff = NULL;

  // free quant
  free (qpPerMatrix);
  free (qpRemMatrix);
  }
//}}}
//{{{
void cDecoder264::mbAffPostProc() {

  sPixel** imgY = picture->imgY;
  sPixel*** imgUV = picture->imgUV;

  for (uint32_t i = 0; i < picture->picSizeInMbs; i += 2) {
    if (picture->motion.mbField[i]) {
      int16_t x0;
      int16_t y0;
      getMbPos (this, i, mbSize[eLuma], &x0, &y0);

      sPixel tempBuffer[32][16];
      updateMbAff (imgY + y0, tempBuffer, x0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

      if (picture->chromaFormatIdc != YUV400) {
        x0 = (int16_t)((x0 * mbCrSizeX) >> 4);
        y0 = (int16_t)((y0 * mbCrSizeY) >> 4);
        updateMbAff (imgUV[0] + y0, tempBuffer, x0, mbCrSizeX, mbCrSizeY);
        updateMbAff (imgUV[1] + y0, tempBuffer, x0, mbCrSizeX, mbCrSizeY);
        }
      }
    }
  }
//}}}

//{{{  fmo
//{{{
void cDecoder264::initFmo (cSlice* slice) {

  fmoGenerateMapUnitToSliceGroupMap (slice);
  fmoGenerateMbToSliceGroupMap (slice);
  }
//}}}
//{{{
void cDecoder264::closeFmo() {

  free (mbToSliceGroupMap);
  mbToSliceGroupMap = NULL;

  free (mapUnitToSliceGroupMap);
  mapUnitToSliceGroupMap = NULL;
  }
//}}}

int cDecoder264::fmoGetNumberOfSliceGroup() { return sliceGroupsNum; }
int cDecoder264::fmoGetLastMBOfPicture() { return fmoGetLastMBInSliceGroup (fmoGetNumberOfSliceGroup() - 1); }
int cDecoder264::fmoGetSliceGroupId (int mb) { return mbToSliceGroupMap[mb]; }
//{{{
int cDecoder264::fmoGetLastMBInSliceGroup (int SliceGroup) {

  for (int i = picSizeInMbs-1; i >= 0; i--)
    if (fmoGetSliceGroupId (i) == SliceGroup)
      return i;

  return -1;
  }
//}}}

//{{{
void cDecoder264::fmoGenerateType0MapUnitMap (uint32_t PicSizeInMapUnits ) {
// Generate interleaved slice group map type MapUnit map (type 0)

  uint32_t iGroup, j;
  uint32_t i = 0;
  do {
    for (iGroup = 0;
         (iGroup <= activePps->numSliceGroupsMinus1) && (i < PicSizeInMapUnits);
         i += activePps->runLengthMinus1[iGroup++] + 1 )
      for (j = 0; j <= activePps->runLengthMinus1[ iGroup ] && i + j < PicSizeInMapUnits; j++ )
        mapUnitToSliceGroupMap[i+j] = iGroup;
    } while (i < PicSizeInMapUnits);
  }
//}}}
//{{{
// Generate dispersed slice group map type MapUnit map (type 1)
void cDecoder264::fmoGenerateType1MapUnitMap (uint32_t PicSizeInMapUnits ) {

  for (uint32_t i = 0; i < PicSizeInMapUnits; i++ )
    mapUnitToSliceGroupMap[i] =
      ((i%coding.picWidthMbs) + (((i/coding.picWidthMbs) * (activePps->numSliceGroupsMinus1+1)) / 2))
        % (activePps->numSliceGroupsMinus1+1);
  }
//}}}
//{{{
// Generate foreground with left-over slice group map type MapUnit map (type 2)
void cDecoder264::fmoGenerateType2MapUnitMap (uint32_t PicSizeInMapUnits ) {

  int iGroup;
  uint32_t i, x, y;
  uint32_t yTopLeft, xTopLeft, yBottomRight, xBottomRight;

  for (i = 0; i < PicSizeInMapUnits; i++ )
    mapUnitToSliceGroupMap[i] = activePps->numSliceGroupsMinus1;

  for (iGroup = activePps->numSliceGroupsMinus1 - 1 ; iGroup >= 0; iGroup--) {
    yTopLeft = activePps->topLeft[ iGroup ] / coding.picWidthMbs;
    xTopLeft = activePps->topLeft[ iGroup ] % coding.picWidthMbs;
    yBottomRight = activePps->botRight[iGroup] / coding.picWidthMbs;
    xBottomRight = activePps->botRight[iGroup] % coding.picWidthMbs;
    for (y = yTopLeft; y <= yBottomRight; y++ )
      for (x = xTopLeft; x <= xBottomRight; x++ )
        mapUnitToSliceGroupMap[ y * coding.picWidthMbs + x ] = iGroup;
    }
  }
//}}}
//{{{
// Generate box-out slice group map type MapUnit map (type 3)
void cDecoder264::fmoGenerateType3MapUnitMap (uint32_t PicSizeInMapUnits, cSlice* slice) {

  uint32_t i, k;
  int leftBound, topBound, rightBound, bottomBound;
  int x, y, xDir, yDir;
  int mapUnitVacant;

  uint32_t mapUnitsInSliceGroup0 = imin((activePps->sliceGroupChangeRateMius1 + 1) * slice->sliceGroupChangeCycle, PicSizeInMapUnits);

  for (i = 0; i < PicSizeInMapUnits; i++ )
    mapUnitToSliceGroupMap[i] = 2;

  x = (coding.picWidthMbs - activePps->sliceGroupChangeDirectionFlag) / 2;
  y = (coding.picHeightMapUnits - activePps->sliceGroupChangeDirectionFlag) / 2;

  leftBound = x;
  topBound = y;
  rightBound  = x;
  bottomBound = y;

  xDir = activePps->sliceGroupChangeDirectionFlag - 1;
  yDir = activePps->sliceGroupChangeDirectionFlag;

  for (k = 0; k < PicSizeInMapUnits; k += mapUnitVacant ) {
    mapUnitVacant = ( mapUnitToSliceGroupMap[ y * coding.picWidthMbs + x ]  ==  2 );
    if (mapUnitVacant )
       mapUnitToSliceGroupMap[ y * coding.picWidthMbs + x ] = ( k >= mapUnitsInSliceGroup0 );

    if (xDir  ==  -1  &&  x  ==  leftBound ) {
      leftBound = imax( leftBound - 1, 0 );
      x = leftBound;
      xDir = 0;
      yDir = 2 * activePps->sliceGroupChangeDirectionFlag - 1;
      }
    else if (xDir  ==  1  &&  x  ==  rightBound ) {
      rightBound = imin( rightBound + 1, (int)coding.picWidthMbs - 1 );
      x = rightBound;
      xDir = 0;
      yDir = 1 - 2 * activePps->sliceGroupChangeDirectionFlag;
      }
    else if (yDir  ==  -1  &&  y  ==  topBound ) {
      topBound = imax( topBound - 1, 0 );
      y = topBound;
      xDir = 1 - 2 * activePps->sliceGroupChangeDirectionFlag;
      yDir = 0;
      }
    else if (yDir  ==  1  &&  y  ==  bottomBound ) {
      bottomBound = imin( bottomBound + 1, (int)coding.picHeightMapUnits - 1 );
      y = bottomBound;
      xDir = 2 * activePps->sliceGroupChangeDirectionFlag - 1;
      yDir = 0;
      }
    else {
      x = x + xDir;
      y = y + yDir;
      }
    }
  }
//}}}
//{{{
void cDecoder264::fmoGenerateType4MapUnitMap (uint32_t PicSizeInMapUnits, cSlice* slice) {
// Generate raster scan slice group map type MapUnit map (type 4)

  uint32_t mapUnitsInSliceGroup0 = imin((activePps->sliceGroupChangeRateMius1 + 1) * slice->sliceGroupChangeCycle, PicSizeInMapUnits);
  uint32_t sizeOfUpperLeftGroup = activePps->sliceGroupChangeDirectionFlag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;

  for (uint32_t i = 0; i < PicSizeInMapUnits; i++ )
    if (i < sizeOfUpperLeftGroup )
      mapUnitToSliceGroupMap[i] = activePps->sliceGroupChangeDirectionFlag;
    else
      mapUnitToSliceGroupMap[i] = 1 - activePps->sliceGroupChangeDirectionFlag;
  }
//}}}
//{{{
// Generate wipe slice group map type MapUnit map (type 5) *
void cDecoder264::fmoGenerateType5MapUnitMap (uint32_t PicSizeInMapUnits, cSlice* slice ) {

  uint32_t mapUnitsInSliceGroup0 = imin (
    (activePps->sliceGroupChangeRateMius1 + 1) * slice->sliceGroupChangeCycle, PicSizeInMapUnits);
  uint32_t sizeOfUpperLeftGroup = activePps->sliceGroupChangeDirectionFlag
                                    ? (PicSizeInMapUnits - mapUnitsInSliceGroup0)
                                    : mapUnitsInSliceGroup0;

  uint32_t k = 0;
  for (uint32_t j = 0; j < coding.picWidthMbs; j++)
    for (uint32_t i = 0; i < coding.picHeightMapUnits; i++)
      if (k++ < sizeOfUpperLeftGroup)
        mapUnitToSliceGroupMap[i * coding.picWidthMbs + j] = activePps->sliceGroupChangeDirectionFlag;
      else
        mapUnitToSliceGroupMap[i * coding.picWidthMbs + j] = 1 - activePps->sliceGroupChangeDirectionFlag;
  }
//}}}
//{{{
void cDecoder264::fmoGenerateType6MapUnitMap (uint32_t PicSizeInMapUnits ) {
// Generate explicit slice group map type MapUnit map (type 6)

  for (uint32_t i = 0; i < PicSizeInMapUnits; i++)
    mapUnitToSliceGroupMap[i] = activePps->sliceGroupId[i];
  }
//}}}

//{{{
int cDecoder264::fmoGenerateMapUnitToSliceGroupMap (cSlice* slice) {
// Generates mapUnitToSliceGroupMap
// Has to be called every time a new Picture Parameter Set is used

  uint32_t NumSliceGroupMapUnits = (activeSps->picHeightMapUnitsMinus1+1)* (activeSps->picWidthMbsMinus1+1);

  if (activePps->sliceGroupMapType == 6)
    if ((activePps->picSizeMapUnitsMinus1 + 1) != NumSliceGroupMapUnits)
      cDecoder264::error ("wrong activePps->picSizeMapUnitsMinus1 for used activeSps and FMO type 6");

  // allocate memory for mapUnitToSliceGroupMap
  if (mapUnitToSliceGroupMap)
    free (mapUnitToSliceGroupMap);

  if ((mapUnitToSliceGroupMap = (int*)malloc ((NumSliceGroupMapUnits) * sizeof (int))) == NULL) {
    printf ("cannot allocated %d bytes for mapUnitToSliceGroupMap, exit\n", (int) ( (activePps->picSizeMapUnitsMinus1+1) * sizeof (int)));
    exit (-1);
    }

  if (activePps->numSliceGroupsMinus1 == 0) {
    // only one slice group
    memset (mapUnitToSliceGroupMap, 0, NumSliceGroupMapUnits * sizeof (int));
    return 0;
    }

  switch (activePps->sliceGroupMapType) {
    case 0:
      fmoGenerateType0MapUnitMap (NumSliceGroupMapUnits);
      break;
    case 1:
      fmoGenerateType1MapUnitMap (NumSliceGroupMapUnits);
      break;
    case 2:
      fmoGenerateType2MapUnitMap (NumSliceGroupMapUnits);
      break;
    case 3:
      fmoGenerateType3MapUnitMap (NumSliceGroupMapUnits, slice);
      break;
    case 4:
      fmoGenerateType4MapUnitMap (NumSliceGroupMapUnits, slice);
      break;
    case 5:
      fmoGenerateType5MapUnitMap (NumSliceGroupMapUnits, slice);
      break;
    case 6:
      fmoGenerateType6MapUnitMap (NumSliceGroupMapUnits);
      break;
    default:
      printf ("Illegal sliceGroupMapType %d , exit \n", (int) activePps->sliceGroupMapType);
      exit (-1);
    }

  return 0;
  }
//}}}
//{{{
int cDecoder264::fmoGenerateMbToSliceGroupMap (cSlice *slice) {
// Generates mbToSliceGroupMap from mapUnitToSliceGroupMap

  // allocate memory for mbToSliceGroupMap
  free (mbToSliceGroupMap);
  mbToSliceGroupMap = (int*)malloc ((picSizeInMbs) * sizeof (int));

  if ((activeSps->frameMbOnly)|| slice->fieldPic) {
    int* mbToSliceGroupMap1 = mbToSliceGroupMap;
    int* mapUnitToSliceGroupMap1 = mapUnitToSliceGroupMap;
    for (uint32_t i = 0; i < picSizeInMbs; i++)
      *mbToSliceGroupMap1++ = *mapUnitToSliceGroupMap1++;
    }
  else {
    if (activeSps->mbAffFlag  &&  (!slice->fieldPic))
      for (uint32_t i = 0; i < picSizeInMbs; i++)
        mbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i/2];
    else
      for (uint32_t i = 0; i < picSizeInMbs; i++)
        mbToSliceGroupMap[i] = mapUnitToSliceGroupMap[(i/(2*coding.picWidthMbs)) *
                                 coding.picWidthMbs + (i%coding.picWidthMbs)];
    }

  return 0;
  }
//}}}
//}}}
//{{{
void cDecoder264::setCoding() {

  widthCr = 0;
  heightCr = 0;
  bitDepthLuma = coding.bitDepthLuma;
  bitDepthChroma = coding.bitDepthChroma;

  coding.lumaPadX = MCBUF_LUMA_PAD_X;
  coding.lumaPadY = MCBUF_LUMA_PAD_Y;
  coding.chromaPadX = MCBUF_CHROMA_PAD_X;
  coding.chromaPadY = MCBUF_CHROMA_PAD_Y;

  if (coding.yuvFormat == YUV420) {
    //{{{  yuv420
    widthCr = coding.width >> 1;
    heightCr = coding.height >> 1;
    }
    //}}}
  else if (coding.yuvFormat == YUV422) {
    //{{{  yuv422
    widthCr = coding.width >> 1;
    heightCr = coding.height;
    coding.chromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
    //}}}
  else if (coding.yuvFormat == YUV444) {
    //{{{  yuv444
    widthCr = coding.width;
    heightCr = coding.height;
    coding.chromaPadX = coding.lumaPadX;
    coding.chromaPadY = coding.lumaPadY;
    }
    //}}}

  // pel bitDepth init
  coding.bitDepthLumaQpScale = 6 * (bitDepthLuma - 8);

  if ((bitDepthLuma > bitDepthChroma) ||
      (activeSps->chromaFormatIdc == YUV400))
    coding.picUnitBitSizeDisk = (bitDepthLuma > 8)? 16:8;
  else
    coding.picUnitBitSizeDisk = (bitDepthChroma > 8)? 16:8;

  coding.dcPredValueComp[0] = 1<<(bitDepthLuma - 1);
  coding.maxPelValueComp[0] = (1<<bitDepthLuma) - 1;
  mbSize[0][0] = mbSize[0][1] = MB_BLOCK_SIZE;

  if (activeSps->chromaFormatIdc == YUV400) {
    //{{{  yuv400
    coding.bitDepthChromaQpScale = 0;

    coding.maxPelValueComp[1] = 0;
    coding.maxPelValueComp[2] = 0;

    coding.numBlock8x8uv = 0;
    coding.numUvBlocks = 0;
    coding.numCdcCoeff = 0;

    mbSize[1][0] = mbSize[2][0] = mbCrSizeX  = 0;
    mbSize[1][1] = mbSize[2][1] = mbCrSizeY  = 0;

    coding.subpelX = 0;
    coding.subpelY = 0;
    coding.shiftpelX = 0;
    coding.shiftpelY = 0;

    coding.totalScale = 0;
    }
    //}}}
  else {
    //{{{  not yuv400
    coding.bitDepthChromaQpScale = 6 * (bitDepthChroma - 8);

    coding.dcPredValueComp[1] = 1 << (bitDepthChroma - 1);
    coding.dcPredValueComp[2] = coding.dcPredValueComp[1];
    coding.maxPelValueComp[1] = (1 << bitDepthChroma) - 1;
    coding.maxPelValueComp[2] = (1 << bitDepthChroma) - 1;

    coding.numBlock8x8uv = (1 << activeSps->chromaFormatIdc) & (~(0x1));
    coding.numUvBlocks = coding.numBlock8x8uv >> 1;
    coding.numCdcCoeff = coding.numBlock8x8uv << 1;

    mbSize[1][0] = mbSize[2][0] =
      mbCrSizeX = (activeSps->chromaFormatIdc == YUV420 ||
                            activeSps->chromaFormatIdc == YUV422) ? 8 : 16;
    mbSize[1][1] = mbSize[2][1] =
      mbCrSizeY = (activeSps->chromaFormatIdc == YUV444 ||
                            activeSps->chromaFormatIdc == YUV422) ? 16 : 8;

    coding.subpelX = mbCrSizeX == 8 ? 7 : 3;
    coding.subpelY = mbCrSizeY == 8 ? 7 : 3;
    coding.shiftpelX = mbCrSizeX == 8 ? 3 : 2;
    coding.shiftpelY = mbCrSizeY == 8 ? 3 : 2;

    coding.totalScale = coding.shiftpelX + coding.shiftpelY;
    }
    //}}}

  mbCrSize = mbCrSizeX * mbCrSizeY;
  mbSizeBlock[0][0] = mbSizeBlock[0][1] = mbSize[0][0] >> 2;
  mbSizeBlock[1][0] = mbSizeBlock[2][0] = mbSize[1][0] >> 2;
  mbSizeBlock[1][1] = mbSizeBlock[2][1] = mbSize[1][1] >> 2;

  mbSizeShift[0][0] = mbSizeShift[0][1] = ceilLog2sf (mbSize[0][0]);
  mbSizeShift[1][0] = mbSizeShift[2][0] = ceilLog2sf (mbSize[1][0]);
  mbSizeShift[1][1] = mbSizeShift[2][1] = ceilLog2sf (mbSize[1][1]);
  }
//}}}
//{{{
void cDecoder264::setCodingParam (cSps* sps) {

  // maximum vertical motion vector range in luma quarter pixel units
  coding.profileIdc = sps->profileIdc;
  coding.useLosslessQpPrime = sps->useLosslessQpPrime;

  if (sps->levelIdc <= 10)
    coding.maxVmvR = 64 * 4;
  else if (sps->levelIdc <= 20)
    coding.maxVmvR = 128 * 4;
  else if (sps->levelIdc <= 30)
    coding.maxVmvR = 256 * 4;
  else
    coding.maxVmvR = 512 * 4; // 512 pixels in quarter pixels

  // Fidelity Range Extensions stuff (part 1)
  coding.widthCr = 0;
  coding.heightCr = 0;
  coding.bitDepthLuma = (int16_t)(sps->bit_depth_luma_minus8 + 8);
  coding.bitDepthScale[0] = 1 << sps->bit_depth_luma_minus8;
  coding.bitDepthChroma = 0;
  if (sps->chromaFormatIdc != YUV400) {
    coding.bitDepthChroma = (int16_t)(sps->bit_depth_chroma_minus8 + 8);
    coding.bitDepthScale[1] = 1 << sps->bit_depth_chroma_minus8;
    }

  coding.maxFrameNum = 1 << (sps->log2maxFrameNumMinus4+4);
  coding.picWidthMbs = sps->picWidthMbsMinus1 + 1;
  coding.picHeightMapUnits = sps->picHeightMapUnitsMinus1 + 1;
  coding.frameHeightMbs = (2 - sps->frameMbOnly) * coding.picHeightMapUnits;
  coding.frameSizeMbs = coding.picWidthMbs * coding.frameHeightMbs;

  coding.yuvFormat = sps->chromaFormatIdc;
  coding.isSeperateColourPlane = sps->isSeperateColourPlane;

  coding.width = coding.picWidthMbs * MB_BLOCK_SIZE;
  coding.height = coding.frameHeightMbs * MB_BLOCK_SIZE;

  coding.lumaPadX = MCBUF_LUMA_PAD_X;
  coding.lumaPadY = MCBUF_LUMA_PAD_Y;
  coding.chromaPadX = MCBUF_CHROMA_PAD_X;
  coding.chromaPadY = MCBUF_CHROMA_PAD_Y;

  if (sps->chromaFormatIdc == YUV420) {
    coding.widthCr  = (coding.width  >> 1);
    coding.heightCr = (coding.height >> 1);
    }
  else if (sps->chromaFormatIdc == YUV422) {
    coding.widthCr  = (coding.width >> 1);
    coding.heightCr = coding.height;
    coding.chromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (sps->chromaFormatIdc == YUV444) {
    coding.widthCr = coding.width;
    coding.heightCr = coding.height;
    coding.chromaPadX = coding.lumaPadX;
    coding.chromaPadY = coding.lumaPadY;
    }

  // pel bitDepth init
  coding.bitDepthLumaQpScale = 6 * (coding.bitDepthLuma - 8);

  if (coding.bitDepthLuma > coding.bitDepthChroma || sps->chromaFormatIdc == YUV400)
    coding.picUnitBitSizeDisk = (coding.bitDepthLuma > 8)? 16:8;
  else
    coding.picUnitBitSizeDisk = (coding.bitDepthChroma > 8)? 16:8;
  coding.dcPredValueComp[0] = 1 << (coding.bitDepthLuma - 1);
  coding.maxPelValueComp[0] = (1 << coding.bitDepthLuma) - 1;
  coding.mbSize[0][0] = coding.mbSize[0][1] = MB_BLOCK_SIZE;

  if (sps->chromaFormatIdc == YUV400) {
    //{{{  YUV400
    coding.bitDepthChromaQpScale = 0;
    coding.maxPelValueComp[1] = 0;
    coding.maxPelValueComp[2] = 0;
    coding.numBlock8x8uv = 0;
    coding.numUvBlocks = 0;
    coding.numCdcCoeff = 0;
    coding.mbSize[1][0] = coding.mbSize[2][0] = coding.mbCrSizeX  = 0;
    coding.mbSize[1][1] = coding.mbSize[2][1] = coding.mbCrSizeY  = 0;
    coding.subpelX = 0;
    coding.subpelY = 0;
    coding.shiftpelX = 0;
    coding.shiftpelY = 0;
    coding.totalScale = 0;
    }
    //}}}
  else {
    //{{{  not YUV400
    coding.bitDepthChromaQpScale = 6 * (coding.bitDepthChroma - 8);
    coding.dcPredValueComp[1] = (1 << (coding.bitDepthChroma - 1));
    coding.dcPredValueComp[2] = coding.dcPredValueComp[1];
    coding.maxPelValueComp[1] = (1 << coding.bitDepthChroma) - 1;
    coding.maxPelValueComp[2] = (1 << coding.bitDepthChroma) - 1;
    coding.numBlock8x8uv = (1 << sps->chromaFormatIdc) & (~(0x1));
    coding.numUvBlocks = (coding.numBlock8x8uv >> 1);
    coding.numCdcCoeff = (coding.numBlock8x8uv << 1);
    coding.mbSize[1][0] = coding.mbSize[2][0] = coding.mbCrSizeX  = (sps->chromaFormatIdc==YUV420 || sps->chromaFormatIdc==YUV422)?  8 : 16;
    coding.mbSize[1][1] = coding.mbSize[2][1] = coding.mbCrSizeY  = (sps->chromaFormatIdc==YUV444 || sps->chromaFormatIdc==YUV422)? 16 :  8;

    coding.subpelX = coding.mbCrSizeX == 8 ? 7 : 3;
    coding.subpelY = coding.mbCrSizeY == 8 ? 7 : 3;
    coding.shiftpelX = coding.mbCrSizeX == 8 ? 3 : 2;
    coding.shiftpelY = coding.mbCrSizeY == 8 ? 3 : 2;
    coding.totalScale = coding.shiftpelX + coding.shiftpelY;
    }
    //}}}

  coding.mbCrSize = coding.mbCrSizeX * coding.mbCrSizeY;
  coding.mbSizeBlock[0][0] = coding.mbSizeBlock[0][1] = coding.mbSize[0][0] >> 2;
  coding.mbSizeBlock[1][0] = coding.mbSizeBlock[2][0] = coding.mbSize[1][0] >> 2;
  coding.mbSizeBlock[1][1] = coding.mbSizeBlock[2][1] = coding.mbSize[1][1] >> 2;

  coding.mbSizeShift[0][0] = coding.mbSizeShift[0][1] = ceilLog2sf (coding.mbSize[0][0]);
  coding.mbSizeShift[1][0] = coding.mbSizeShift[2][0] = ceilLog2sf (coding.mbSize[1][0]);
  coding.mbSizeShift[1][1] = coding.mbSizeShift[2][1] = ceilLog2sf (coding.mbSize[1][1]);
  }
//}}}
//{{{
void cDecoder264::setFormat (cSps* sps, sFrameFormat* source, sFrameFormat* output) {

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

  source->width[0] = coding.width - cropLeft - cropRight;
  source->height[0] = coding.height - cropTop - cropBot;

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
  source->width[1] = widthCr - cropLeft - cropRight;
  source->width[2] = source->width[1];
  source->height[1] = heightCr - cropTop - cropBot;
  source->height[2] = source->height[1];

  // ????
  source->width[1] = widthCr;
  source->width[2] = widthCr;

  source->sizeCmp[0] = source->width[0] * source->height[0];
  source->sizeCmp[1] = source->width[1] * source->height[1];
  source->sizeCmp[2] = source->sizeCmp[1];
  source->mbWidth = source->width[0]  / MB_BLOCK_SIZE;
  source->mbHeight = source->height[0] / MB_BLOCK_SIZE;

  // output
  output->width[0] = coding.width;
  output->height[0] = coding.height;
  output->height[1] = heightCr;
  output->height[2] = heightCr;

  // output size (excluding padding)
  output->sizeCmp[0] = output->width[0] * output->height[0];
  output->sizeCmp[1] = output->width[1] * output->height[1];
  output->sizeCmp[2] = output->sizeCmp[1];
  output->mbWidth = output->width[0]  / MB_BLOCK_SIZE;
  output->mbHeight = output->height[0] / MB_BLOCK_SIZE;

  output->bitDepth[0] = source->bitDepth[0] = bitDepthLuma;
  output->bitDepth[1] = source->bitDepth[1] = bitDepthChroma;
  output->bitDepth[2] = source->bitDepth[2] = bitDepthChroma;

  output->colourModel = source->colourModel;
  output->yuvFormat = source->yuvFormat = (eYuvFormat)sps->chromaFormatIdc;
  }
//}}}

//{{{
bool cDecoder264::isNewPicture (sPicture* picture, cSlice* slice, sOldSlice* oldSlice) {

  bool result = (NULL == picture);

  result |= (oldSlice->ppsId != slice->ppsId);
  result |= (oldSlice->frameNum != slice->frameNum);
  result |= (oldSlice->fieldPic != slice->fieldPic);

  if (slice->fieldPic && oldSlice->fieldPic)
    result |= (oldSlice->botField != slice->botField);

  result |= (oldSlice->nalRefIdc != slice->refId) && (!oldSlice->nalRefIdc || !slice->refId);
  result |= (oldSlice->isIDR != slice->isIDR);

  if (slice->isIDR && oldSlice->isIDR)
    result |= (oldSlice->idrPicId != slice->idrPicId);

  if (!activeSps->pocType) {
    result |= (oldSlice->picOrderCountLsb != slice->picOrderCountLsb);
    if ((activePps->frameBotField == 1) && !slice->fieldPic)
      result |= (oldSlice->deltaPicOrderCountBot != slice->deltaPicOrderCountBot);
    }

  if (activeSps->pocType == 1) {
    if (!activeSps->deltaPicOrderAlwaysZero) {
      result |= (oldSlice->deltaPicOrderCount[0] != slice->deltaPicOrderCount[0]);
      if ((activePps->frameBotField == 1) && !slice->fieldPic)
        result |= (oldSlice->deltaPicOrderCount[1] != slice->deltaPicOrderCount[1]);
      }
    }

  return result;
  }
//}}}
//{{{
void cDecoder264::readDecRefPicMarking (cBitStream& s, cSlice* slice) {

  // free old buffer content
  while (slice->decRefPicMarkBuffer) {
    sDecodedRefPicMark* tmp_drpm = slice->decRefPicMarkBuffer;
    slice->decRefPicMarkBuffer = tmp_drpm->next;
    free (tmp_drpm);
    }

  if (slice->isIDR) {
    slice->noOutputPriorPicFlag = s.readU1 ("SLC noOutputPriorPicFlag");
    noOutputPriorPicFlag = slice->noOutputPriorPicFlag;
    slice->longTermRefFlag = s.readU1 ("SLC longTermRefFlag");
    }
  else {
    slice->adaptRefPicBufFlag = s.readU1 ("SLC adaptRefPicBufFlag");
    if (slice->adaptRefPicBufFlag) {
      // read Memory Management Control Operation
      int val;
      do {
        sDecodedRefPicMark* tempDrpm = (sDecodedRefPicMark*)calloc (1,sizeof (sDecodedRefPicMark));
        tempDrpm->next = NULL;
        val = tempDrpm->memManagement = s.readUeV ("SLC memManagement");
        if ((val == 1) || (val == 3))
          tempDrpm->diffPicNumMinus1 = s.readUeV ("SLC diffPicNumMinus1");
        if (val == 2)
          tempDrpm->longTermPicNum = s.readUeV ("SLC longTermPicNum");

        if ((val == 3 ) || (val == 6))
          tempDrpm->longTermFrameIndex = s.readUeV ("SLC longTermFrameIndex");
        if (val == 4)
          tempDrpm->maxLongTermFrameIndexPlus1 = s.readUeV ("SLC max_long_term_pic_idx_plus1");

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
void cDecoder264::initRefPicture (cSlice* slice) {

  sPicture* vidRefPicture = dpb.noRefPicture;
  int noRef = slice->framePoc < recoveryPoc;

  if (coding.isSeperateColourPlane) {
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
void cDecoder264::initPicture (cSlice* slice) {

  cDpb* dpb = slice->dpb;

  picHeightInMbs = coding.frameHeightMbs / (slice->fieldPic+1);
  picSizeInMbs = coding.picWidthMbs * picHeightInMbs;
  coding.frameSizeMbs = coding.picWidthMbs * coding.frameHeightMbs;

  if (picture) // slice loss
    endDecodeFrame();

  if (recoveryPoint)
    recoveryFrameNum = (slice->frameNum + recoveryFrameCount) % coding.maxFrameNum;
  if (slice->isIDR)
    recoveryFrameNum = slice->frameNum;
  if (!recoveryPoint &&
      (slice->frameNum != preFrameNum) &&
      (slice->frameNum != (preFrameNum + 1) % coding.maxFrameNum)) {
    if (!activeSps->allowGapsFrameNum) {
      //{{{  picture error conceal
      if (param.concealMode) {
        if ((slice->frameNum) < ((preFrameNum + 1) % coding.maxFrameNum)) {
          /* Conceal lost IDR frames and any frames immediately following the IDR.
          // Use frame copy for these since lists cannot be formed correctly for motion copy*/
          concealMode = 1;
          idrConcealFlag = 1;
          concealLostFrames (dpb, slice);
          // reset to original conceal mode for future drops
          concealMode = param.concealMode;
          }
        else {
          // reset to original conceal mode for future drops
          concealMode = param.concealMode;
          idrConcealFlag = 0;
          concealLostFrames (dpb, slice);
          }
        }
      else
        // Advanced Error Concealment would be called here to combat unintentional loss of pictures
        cDecoder264::error ("initPicture - unintentional loss of picture\n");
      }
      //}}}
    if (!concealMode)
      fillFrameNumGap (this, slice);
    }

  if (slice->refId)
    preFrameNum = slice->frameNum;

  // calculate POC
  decodePOC (slice);

  if (recoveryFrameNum == (int)slice->frameNum && recoveryPoc == 0x7fffffff)
    recoveryPoc = slice->framePoc;
  if (slice->refId)
    lastRefPicPoc = slice->framePoc;
  if ((slice->picStructure == eFrame) || (slice->picStructure == eTopField))
    getTime (&debug.startTime);

  picture = allocPicture (this, slice->picStructure, coding.width, coding.height, widthCr, heightCr, 1);
  picture->topPoc = slice->topPoc;
  picture->botPoc = slice->botPoc;
  picture->framePoc = slice->framePoc;
  picture->qp = slice->qp;
  picture->sliceQpDelta = slice->sliceQpDelta;
  picture->chromaQpOffset[0] = activePps->chromaQpOffset;
  picture->chromaQpOffset[1] = activePps->chromaQpOffset2;
  picture->codingType = slice->picStructure == eFrame ?
    (slice->mbAffFrame? eFrameMbPairCoding:eFrameCoding) : eFieldCoding;

  // reset all variables of the error conceal instance before decoding of every frame.
  // here the third parameter should, if perfectly, be equal to the number of slices per frame.
  // using little value is ok, the code will allocate more memory if the slice number is larger
  ercReset (ercErrorVar, picSizeInMbs, picSizeInMbs, picture->sizeX);

  ercMvPerMb = 0;
  switch (slice->picStructure ) {
    //{{{
    case eTopField:
      picture->poc = slice->topPoc;
      idrFrameNum *= 2;
      break;
    //}}}
    //{{{
    case eBotField:
      picture->poc = slice->botPoc;
      idrFrameNum = idrFrameNum * 2 + 1;
      break;
    //}}}
    //{{{
    case eFrame:
      picture->poc = slice->framePoc;
      break;
    //}}}
    //{{{
    default:
      cDecoder264::error ("picStructure not initialized");
    //}}}
    }

  if (coding.sliceType > eSliceSI) {
    setEcFlag (SE_PTYPE);
    coding.sliceType = eSliceP;  // concealed element
    }

  // cavlc init
  if (activePps->entropyCoding == eCavlc)
    memset (nzCoeff[0][0][0], -1, picSizeInMbs * 48 *sizeof(uint8_t)); // 3 * 4 * 4

  // Set the sliceNum member of each MB to -1, to ensure correct when packet loss occurs
  // TO set sMacroBlock Map (mark all MBs as 'have to be concealed')
  if (coding.isSeperateColourPlane) {
    for (int nplane = 0; nplane < MAX_PLANE; ++nplane ) {
      sMacroBlock* mb = mbDataJV[nplane];
      char* intraBlock = intraBlockJV[nplane];
      for (int i = 0; i < (int)picSizeInMbs; ++i) {
        //{{{  resetMb
        mb->errorFlag = 1;
        mb->dplFlag = 0;
        mb->sliceNum = -1;
        mb++;
        }
        //}}}
      memset (predModeJV[nplane][0], DC_PRED, 16 * coding.frameHeightMbs * coding.picWidthMbs * sizeof(char));
      if (activePps->hasConstrainedIntraPred)
        for (int i = 0; i < (int)picSizeInMbs; ++i)
          intraBlock[i] = 1;
      }
    }
  else {
    sMacroBlock* mb = mbData;
    for (int i = 0; i < (int)picSizeInMbs; ++i) {
      //{{{  resetMb
      mb->errorFlag = 1;
      mb->dplFlag = 0;
      mb->sliceNum = -1;
      mb++;
      }
      //}}}
    if (activePps->hasConstrainedIntraPred)
      for (int i = 0; i < (int)picSizeInMbs; ++i)
        intraBlock[i] = 1;
    memset (predMode[0], DC_PRED, 16 * coding.frameHeightMbs * coding.picWidthMbs * sizeof(char));
    }

  picture->sliceType = coding.sliceType;
  picture->usedForRef = (slice->refId != 0);
  picture->isIDR = slice->isIDR;
  picture->noOutputPriorPicFlag = slice->noOutputPriorPicFlag;
  picture->longTermRefFlag = slice->longTermRefFlag;
  picture->adaptRefPicBufFlag = slice->adaptRefPicBufFlag;
  picture->decRefPicMarkBuffer = slice->decRefPicMarkBuffer;
  slice->decRefPicMarkBuffer = NULL;

  picture->mbAffFrame = slice->mbAffFrame;
  picture->picWidthMbs = coding.picWidthMbs;

  getMbBlockPos = picture->mbAffFrame ? getMbBlockPosMbaff : getMbBlockPosNormal;
  getNeighbour = picture->mbAffFrame ? getAffNeighbour : getNonAffNeighbour;

  picture->picNum = slice->frameNum;
  picture->frameNum = slice->frameNum;
  picture->recoveryFrame = (uint32_t)((int)slice->frameNum == recoveryFrameNum);
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

  if (coding.isSeperateColourPlane) {
    decPictureJV[0] = picture;
    decPictureJV[1] = allocPicture (this, slice->picStructure, coding.width, coding.height, widthCr, heightCr, 1);
    copyDecPictureJV (decPictureJV[1], decPictureJV[0] );
    decPictureJV[2] = allocPicture (this, slice->picStructure, coding.width, coding.height, widthCr, heightCr, 1);
    copyDecPictureJV (decPictureJV[2], decPictureJV[0] );
    }
  }
//}}}
//{{{
void cDecoder264::initSlice (cSlice* slice) {

  activeSps = slice->activeSps;
  activePps = slice->activePps;

  slice->initLists (slice);
  reorderLists (slice);

  if (slice->picStructure == eFrame)
    slice->initMbAffLists (dpb.noRefPicture);

  // update reference flags and set current refFlag
  if (!(slice->redundantPicCount && (prevFrameNum == slice->frameNum)))
    for (int i = 16; i > 0; i--)
      slice->refFlag[i] = slice->refFlag[i-1];
  slice->refFlag[0] = (slice->redundantPicCount == 0) ? isPrimaryOk : isRedundantOk;

  if (!slice->activeSps->chromaFormatIdc ||
      (slice->activeSps->chromaFormatIdc == 3)) {
    slice->linfoCbpIntra = cBitStream::linfo_cbp_intra_other;
    slice->linfoCbpInter = cBitStream::linfo_cbp_inter_other;
    }
  else {
    slice->linfoCbpIntra = cBitStream::linfo_cbp_intra_normal;
    slice->linfoCbpInter = cBitStream::linfo_cbp_inter_normal;
    }
  }
//}}}
//{{{
void cDecoder264::reorderLists (cSlice* slice) {

  if ((slice->sliceType != eSliceI) && (slice->sliceType != eSliceSI)) {
    if (slice->refPicReorderFlag[LIST_0])
      slice->reorderRefPicList (LIST_0);
    if (dpb.noRefPicture == slice->listX[0][slice->numRefIndexActive[LIST_0]-1])
      cLog::log (LOGERROR, "------ refPicList0[%d] no refPic %s",
                 slice->numRefIndexActive[LIST_0]-1, nonConformingStream ? "conform":"");
    else
      slice->listXsize[0] = (char) slice->numRefIndexActive[LIST_0];
    }

  if (slice->sliceType == eSliceB) {
    if (slice->refPicReorderFlag[LIST_1])
      slice->reorderRefPicList (LIST_1);
    if (dpb.noRefPicture == slice->listX[1][slice->numRefIndexActive[LIST_1]-1])
      cLog::log (LOGERROR, "------ refPicList1[%d] no refPic %s",
                 slice->numRefIndexActive[LIST_0] - 1, nonConformingStream ? "conform" : "");
    else
      slice->listXsize[1] = (char)slice->numRefIndexActive[LIST_1];
    }

  slice->freeRefPicListReorderBuffer();
  }
//}}}
//{{{
void cDecoder264::copySliceInfo (cSlice* slice, sOldSlice* oldSlice) {

  oldSlice->ppsId = slice->ppsId;
  oldSlice->frameNum = slice->frameNum;
  oldSlice->fieldPic = slice->fieldPic;

  if (slice->fieldPic)
    oldSlice->botField = slice->botField;

  oldSlice->nalRefIdc = slice->refId;

  oldSlice->isIDR = (uint8_t)slice->isIDR;
  if (slice->isIDR)
    oldSlice->idrPicId = slice->idrPicId;

  if (activeSps->pocType == 0) {
    oldSlice->picOrderCountLsb = slice->picOrderCountLsb;
    oldSlice->deltaPicOrderCountBot = slice->deltaPicOrderCountBot;
    }
  else if (activeSps->pocType == 1) {
    oldSlice->deltaPicOrderCount[0] = slice->deltaPicOrderCount[0];
    oldSlice->deltaPicOrderCount[1] = slice->deltaPicOrderCount[1];
    }
  }
//}}}
//{{{
void cDecoder264::useParameterSet (cSlice* slice) {

  if (!pps[slice->ppsId].ok)
    cLog::log (LOGINFO, fmt::format ("useParameterSet - invalid ppsId:{}", slice->ppsId));

  if (!sps[pps->spsId].ok)
    cLog::log (LOGINFO, fmt::format ("useParameterSet - invalid spsId:{} ppsId:{}", slice->ppsId, pps->spsId));

  if (&sps[pps->spsId] != activeSps) {
    //{{{  new sps
    if (picture)
      endDecodeFrame();

    activeSps = &sps[pps->spsId];

    if (activeSps->hasBaseLine() && !dpb.isInitDone())
      setCodingParam (sps);
    setCoding();
    initGlobalBuffers();

    if (!noOutputPriorPicFlag)
      dpb.flushDpb();
    dpb.initDpb (this, 0);

    // enable error conceal
    ercInit (this, coding.width, coding.height, 1);
    if (picture) {
      ercReset (ercErrorVar, picSizeInMbs, picSizeInMbs, picture->sizeX);
      ercMvPerMb = 0;
      }

    setFormat (activeSps, &param.source, &param.output);

    debug.profileString = fmt::format ("Profile:{} {}x{} {}x{} yuv{} {}:{}:{}",
                                       coding.profileIdc,
                                       param.source.width[0], param.source.height[0],
                                       coding.width, coding.height,
                                       coding.yuvFormat == YUV400 ? " 400 ":
                                         coding.yuvFormat == YUV420 ? " 420":
                                           coding.yuvFormat == YUV422 ? " 422":" 4:4:4",
                                       param.source.bitDepth[0],
                                       param.source.bitDepth[1],
                                       param.source.bitDepth[2]);
    cLog::log (LOGINFO, debug.profileString);
    }
    //}}}

  if (&pps[slice->ppsId] != activePps) {
    //{{{  new pps
    if (picture) // only on slice loss
      endDecodeFrame();

    activePps = &pps[slice->ppsId];
    }
    //}}}

  // slice->dataPartitionMode is set by read_new_slice (NALU first uint8_t ok there)
  if (activePps->entropyCoding == eCavlc) {
    slice->nalStartCode = cBitStream::vlcStartCode;
    for (int i = 0; i < 3; i++)
      slice->dataPartitions[i].readSyntaxElement = cBitStream::readSyntaxElementVLC;
    }
  else {
    slice->nalStartCode = cabacStartCode;
    for (int i = 0; i < 3; i++)
      slice->dataPartitions[i].readSyntaxElement = readSyntaxElementCabac;
    }

  coding.sliceType = slice->sliceType;
  }
//}}}

// output
//{{{
void cDecoder264::allocDecodedPicBuffers (sDecodedPic* decodedPic, sPicture* p,
                                          int lumaSize, int frameSize, int lumaSizeX, int lumaSizeY,
                                          int chromaSizeX, int chromaSizeY) {

  if (decodedPic->bufSize != frameSize) {
    memFree (decodedPic->yBuf);

    decodedPic->bufSize = frameSize;
    decodedPic->yBuf = (uint8_t*)memAlloc (decodedPic->bufSize);
    decodedPic->uBuf = decodedPic->yBuf + lumaSize;
    decodedPic->vBuf = decodedPic->uBuf + ((frameSize - lumaSize)>>1);

    decodedPic->yuvFormat = p->chromaFormatIdc;
    decodedPic->bitDepth = coding.picUnitBitSizeDisk;
    decodedPic->width = lumaSizeX;
    decodedPic->height = lumaSizeY;

    int symbolSizeInBytes = (coding.picUnitBitSizeDisk + 7) >> 3;
    decodedPic->yStride = lumaSizeX * symbolSizeInBytes;
    decodedPic->uvStride = chromaSizeX * symbolSizeInBytes;
    }
  }
//}}}
//{{{
void cDecoder264::clearPicture (sPicture* picture) {

  for (int i = 0; i < picture->sizeY; i++)
    for (int j = 0; j < picture->sizeX; j++)
      picture->imgY[i][j] = (sPixel)coding.dcPredValueComp[0];

  for (int i = 0; i < picture->sizeYcr;i++)
    for (int j = 0; j < picture->sizeXcr; j++)
      picture->imgUV[0][i][j] = (sPixel)coding.dcPredValueComp[1];

  for (int i = 0; i < picture->sizeYcr;i++)
    for (int j = 0; j < picture->sizeXcr; j++)
      picture->imgUV[1][i][j] = (sPixel)coding.dcPredValueComp[2];
  }
//}}}
//{{{
void cDecoder264::writeOutPicture (sPicture* picture) {

  if (picture->nonExisting)
    return;

  int cropLeft;
  int cropRight;
  int cropTop;
  int cropBottom;
  if (picture->hasCrop) {
    cropLeft = kSubWidthC [picture->chromaFormatIdc] * picture->cropLeft;
    cropRight = kSubWidthC [picture->chromaFormatIdc] * picture->cropRight;
    cropTop = kSubHeightC[picture->chromaFormatIdc] * (2-picture->frameMbOnly) * picture->cropTop;
    cropBottom = kSubHeightC[picture->chromaFormatIdc] * (2-picture->frameMbOnly) * picture->cropBot;
    }
  else
    cropLeft = cropRight = cropTop = cropBottom = 0;

  int symbolSizeInBytes = (coding.picUnitBitSizeDisk+7) >> 3;
  int chromaSizeX = picture->sizeXcr- picture->cropLeft -picture->cropRight;
  int chromaSizeY = picture->sizeYcr - (2 - picture->frameMbOnly) * picture->cropTop -
                                       (2 - picture->frameMbOnly) * picture->cropBot;
  int lumaSizeX = picture->sizeX - cropLeft - cropRight;
  int lumaSizeY = picture->sizeY - cropTop - cropBottom;
  int lumaSize = lumaSizeX * lumaSizeY * symbolSizeInBytes;
  int frameSize = (lumaSizeX * lumaSizeY + 2 * (chromaSizeX * chromaSizeY)) * symbolSizeInBytes;

  sDecodedPic* decodedPic = allocDecodedPicture (outDecodedPics);
  if (!decodedPic->yBuf || (decodedPic->bufSize < frameSize))
    allocDecodedPicBuffers (decodedPic, picture, lumaSize, frameSize,
                            lumaSizeX, lumaSizeY, chromaSizeX, chromaSizeY);
  decodedPic->ok = 1;
  decodedPic->poc = picture->framePoc;
  copyCropped (decodedPic->ok ? decodedPic->yBuf : decodedPic->yBuf + lumaSizeX * symbolSizeInBytes,
               picture->imgY, picture->sizeX, picture->sizeY, symbolSizeInBytes,
               cropLeft, cropRight, cropTop, cropBottom, decodedPic->yStride);

  cropLeft = picture->cropLeft;
  cropRight = picture->cropRight;
  cropTop = (2 - picture->frameMbOnly) * picture->cropTop;
  cropBottom = (2 - picture->frameMbOnly) * picture->cropBot;

  copyCropped (decodedPic->ok ? decodedPic->uBuf : decodedPic->uBuf + chromaSizeX * symbolSizeInBytes,
               picture->imgUV[0], picture->sizeXcr, picture->sizeYcr, symbolSizeInBytes,
               cropLeft, cropRight, cropTop, cropBottom, decodedPic->uvStride);

  copyCropped (decodedPic->ok ? decodedPic->vBuf : decodedPic->vBuf + chromaSizeX * symbolSizeInBytes,
               picture->imgUV[1], picture->sizeXcr, picture->sizeYcr, symbolSizeInBytes,
               cropLeft, cropRight, cropTop, cropBottom, decodedPic->uvStride);
  }
//}}}
//{{{
void cDecoder264::flushPendingOut() {

  if (pendingOutPicStructure != eFrame)
    writeOutPicture (pendingOut);

  freeMem2Dpel (pendingOut->imgY);
  pendingOut->imgY = NULL;

  freeMem3Dpel (pendingOut->imgUV);
  pendingOut->imgUV = NULL;

  pendingOutPicStructure = eFrame;
  }
//}}}
//{{{
void cDecoder264::writePicture (sPicture* picture, int realStructure) {

  if (realStructure == eFrame) {
    flushPendingOut();
    writeOutPicture (picture);
    return;
    }

  if (realStructure == pendingOutPicStructure) {
    flushPendingOut();
    writePicture (picture, realStructure);
    return;
    }

  if (pendingOutPicStructure == eFrame) {
    //{{{  output frame
    pendingOut->sizeX = picture->sizeX;
    pendingOut->sizeY = picture->sizeY;
    pendingOut->sizeXcr = picture->sizeXcr;
    pendingOut->sizeYcr = picture->sizeYcr;
    pendingOut->chromaFormatIdc = picture->chromaFormatIdc;

    pendingOut->frameMbOnly = picture->frameMbOnly;
    pendingOut->hasCrop = picture->hasCrop;
    if (pendingOut->hasCrop) {
      pendingOut->cropLeft = picture->cropLeft;
      pendingOut->cropRight = picture->cropRight;
      pendingOut->cropTop = picture->cropTop;
      pendingOut->cropBot = picture->cropBot;
      }

    getMem2Dpel (&pendingOut->imgY, pendingOut->sizeY, pendingOut->sizeX);
    getMem3Dpel (&pendingOut->imgUV, 2, pendingOut->sizeYcr, pendingOut->sizeXcr);
    clearPicture (pendingOut);

    // copy first field
    int add = (realStructure == eTopField) ? 0 : 1;
    for (int i = 0; i < pendingOut->sizeY; i += 2)
      memcpy (pendingOut->imgY[(i+add)], picture->imgY[(i+add)], picture->sizeX * sizeof(sPixel));
    for (int i = 0; i < pendingOut->sizeYcr; i += 2) {
      memcpy (pendingOut->imgUV[0][(i+add)], picture->imgUV[0][(i+add)], picture->sizeXcr * sizeof(sPixel));
      memcpy (pendingOut->imgUV[1][(i+add)], picture->imgUV[1][(i+add)], picture->sizeXcr * sizeof(sPixel));
      }

    pendingOutPicStructure = realStructure;
    }
    //}}}
  else {
    if ((pendingOut->sizeX != picture->sizeX) || (pendingOut->sizeY != picture->sizeY) ||
        (pendingOut->frameMbOnly != picture->frameMbOnly) ||
        (pendingOut->hasCrop != picture->hasCrop) ||
        (pendingOut->hasCrop &&
         ((pendingOut->cropLeft != picture->cropLeft) || (pendingOut->cropRight != picture->cropRight) ||
          (pendingOut->cropTop != picture->cropTop) || (pendingOut->cropBot != picture->cropBot)))) {
      flushPendingOut();
      writePicture (picture, realStructure);
      return;
      }

    // copy second field
    int add = (realStructure == eTopField) ? 0 : 1;
    for (int i = 0; i < pendingOut->sizeY; i += 2)
      memcpy (pendingOut->imgY[i+add], picture->imgY[i+add], picture->sizeX * sizeof(sPixel));

    for (int i = 0; i < pendingOut->sizeYcr; i+=2) {
      memcpy (pendingOut->imgUV[0][i+add], picture->imgUV[0][i+add], picture->sizeXcr * sizeof(sPixel));
      memcpy (pendingOut->imgUV[1][i+add], picture->imgUV[1][i+add], picture->sizeXcr * sizeof(sPixel));
      }

    flushPendingOut();
    }
  }
//}}}
//{{{
void cDecoder264::writeUnpairedField (cFrameStore* frameStore) {

  if (frameStore->isUsed & 0x01) {
    // we have a top field, construct an empty bottom field
    sPicture* picture = frameStore->topField;
    frameStore->botField = allocPicture (this, eBotField, picture->sizeX, 2 * picture->sizeY,
                                                          picture->sizeXcr, 2 * picture->sizeYcr, 1);
    frameStore->botField->chromaFormatIdc = picture->chromaFormatIdc;

    clearPicture (frameStore->botField);
    frameStore->dpbCombineField (this);
    writePicture (frameStore->frame, eTopField);
    }

  if (frameStore->isUsed & 0x02) {
    // we have a bottom field, construct an empty top field
    sPicture* picture = frameStore->botField;
    frameStore->topField = allocPicture (this, eTopField, picture->sizeX, 2*picture->sizeY,
                                         picture->sizeXcr, 2*picture->sizeYcr, 1);
    frameStore->topField->chromaFormatIdc = picture->chromaFormatIdc;
    clearPicture (frameStore->topField);

    frameStore->topField->hasCrop = frameStore->botField->hasCrop;
    if (frameStore->topField->hasCrop) {
      frameStore->topField->cropTop = frameStore->botField->cropTop;
      frameStore->topField->cropBot = frameStore->botField->cropBot;
      frameStore->topField->cropLeft = frameStore->botField->cropLeft;
      frameStore->topField->cropRight = frameStore->botField->cropRight;
      }

    frameStore->dpbCombineField (this);
    writePicture (frameStore->frame, eBotField);
    }

  frameStore->isUsed = 3;
  }
//}}}
//{{{
void cDecoder264::flushDirectOutput() {

  writeUnpairedField (outBuffer);

  freePicture (outBuffer->frame);
  outBuffer->frame = NULL;

  freePicture (outBuffer->topField);
  outBuffer->topField = NULL;

  freePicture (outBuffer->botField);
  outBuffer->botField = NULL;

  outBuffer->isUsed = 0;
  }
//}}}

//{{{
void cDecoder264::makeFramePictureJV() {

  picture = decPictureJV[0];

  // copy;
  if (picture->usedForRef) {
    int nsize = (picture->sizeY/BLOCK_SIZE)*(picture->sizeX/BLOCK_SIZE)*sizeof(sPicMotion);
    memcpy (&(picture->mvInfoJV[PLANE_Y][0][0]), &(decPictureJV[PLANE_Y]->mvInfo[0][0]), nsize);
    memcpy (&(picture->mvInfoJV[PLANE_U][0][0]), &(decPictureJV[PLANE_U]->mvInfo[0][0]), nsize);
    memcpy (&(picture->mvInfoJV[PLANE_V][0][0]), &(decPictureJV[PLANE_V]->mvInfo[0][0]), nsize);
    }

  // This could be done with pointers and seems not necessary
  for (int uv = 0; uv < 2; uv++) {
    for (int line = 0; line < coding.height; line++) {
      int nsize = sizeof(sPixel) * coding.width;
      memcpy (picture->imgUV[uv][line], decPictureJV[uv+1]->imgY[line], nsize );
      }
    freePicture (decPictureJV[uv+1]);
    }
  }
//}}}

// slice
//{{{
void cDecoder264::readSliceHeader (cSlice* slice) {
// Some slice syntax depends on parameterSet depends on parameterSetID of the slice header
// - read the ppsId of the slice header first
//   - then setup the active parameter sets
// - read the rest of the slice header

  uint32_t partitionIndex = kSyntaxElementToDataPartitionIndex[slice->dataPartitionMode][SE_HEADER];
  cBitStream& s = slice->dataPartitions[partitionIndex].bitStream;

  slice->startMbNum = s.readUeV ("SLC first_mb_in_slice");

  int tmp = s.readUeV ("SLC sliceType");
  if (tmp > 4)
    tmp -= 5;
  slice->sliceType = (eSliceType)tmp;
  coding.sliceType = slice->sliceType;

  slice->ppsId = s.readUeV ("SLC ppsId");

  if (coding.isSeperateColourPlane)
    slice->colourPlaneId = s.readUv (2, "SLC colourPlaneId");
  else
    slice->colourPlaneId = PLANE_Y;

  // setup parameterSet
  useParameterSet (slice);
  slice->activeSps = activeSps;
  slice->activePps = activePps;
  slice->transform8x8Mode = activePps->hasTransform8x8mode;
  slice->chroma444notSeparate = (activeSps->chromaFormatIdc == YUV444) && !coding.isSeperateColourPlane;

  slice->frameNum = s.readUv (activeSps->log2maxFrameNumMinus4 + 4, "SLC frameNum");
  if (slice->isIDR) {
    preFrameNum = slice->frameNum;
    lastRefPicPoc = 0;
    }
  //{{{  read field/frame
  if (activeSps->frameMbOnly) {
    slice->fieldPic = 0;
    coding.picStructure = eFrame;
    }

  else {
    slice->fieldPic = s.readU1 ("SLC fieldPic");
    if (slice->fieldPic) {
      slice->botField = (uint8_t)s.readU1 ("SLC botField");
      coding.picStructure = slice->botField ? eBotField : eTopField;
      }
    else {
      slice->botField = false;
      coding.picStructure = eFrame;
      }
    }

  slice->picStructure = coding.picStructure;
  slice->mbAffFrame = activeSps->mbAffFlag && !slice->fieldPic;
  //}}}

  if (slice->isIDR)
    slice->idrPicId = s.readUeV ("SLC idrPicId");
  //{{{  read picOrderCount
  if (activeSps->pocType == 0) {
    slice->picOrderCountLsb = s.readUv (activeSps->log2maxPocLsbMinus4 + 4, "SLC picOrderCountLsb");
    if ((activePps->frameBotField == 1) && !slice->fieldPic)
      slice->deltaPicOrderCountBot = s.readSeV ("SLC deltaPicOrderCountBot");
    else
      slice->deltaPicOrderCountBot = 0;
    }

  if (activeSps->pocType == 1) {
    if (!activeSps->deltaPicOrderAlwaysZero) {
      slice->deltaPicOrderCount[0] = s.readSeV ("SLC deltaPicOrderCount[0]");
      if ((activePps->frameBotField == 1) && !slice->fieldPic)
        slice->deltaPicOrderCount[1] = s.readSeV ("SLC deltaPicOrderCount[1]");
      else
        slice->deltaPicOrderCount[1] = 0;  // set to zero if not in stream
      }
    else {
      slice->deltaPicOrderCount[0] = 0;
      slice->deltaPicOrderCount[1] = 0;
      }
    }
  //}}}

  if (activePps->redundantPicCountPresent)
    slice->redundantPicCount = s.readUeV ("SLC redundantPicCount");

  if (slice->sliceType == eSliceB)
    slice->directSpatialMvPredFlag = s.readU1 ("SLC directSpatialMvPredFlag");

  // read refPics
  slice->numRefIndexActive[LIST_0] = activePps->numRefIndexL0defaultActiveMinus1 + 1;
  slice->numRefIndexActive[LIST_1] = activePps->numRefIndexL1defaultActiveMinus1 + 1;
  if ((slice->sliceType == eSliceP) || (slice->sliceType == eSliceSP) || (slice->sliceType == eSliceB)) {
    if (s.readU1 ("SLC isNumRefIndexOverride")) {
      slice->numRefIndexActive[LIST_0] = 1 + s.readUeV ("SLC numRefIndexActiveL0minus1");
      if (slice->sliceType == eSliceB)
        slice->numRefIndexActive[LIST_1] = 1 + s.readUeV ("SLC numRefIndexActiveL1minus1");
      }
    }
  if (slice->sliceType != eSliceB)
    slice->numRefIndexActive[LIST_1] = 0;
  //{{{  read refPicList reorder
  slice->allocRefPicListReordeBuffer();

  if ((slice->sliceType != eSliceI) && (slice->sliceType != eSliceSI)) {
    int value = slice->refPicReorderFlag[LIST_0] = s.readU1 ("SLC refPicReorderL0");
    if (value) {
      int i = 0;
      do {
        value = slice->modPicNumsIdc[LIST_0][i] = s.readUeV("SLC modPicNumsIdcl0");
        if ((value == 0) || (value == 1))
          slice->absDiffPicNumMinus1[LIST_0][i] = s.readUeV ("SLC absDiffPicNumMinus1L0");
        else if (value == 2)
          slice->longTermPicIndex[LIST_0][i] = s.readUeV ("SLC longTermPicIndexL0");
        i++;
        } while (value != 3);
      }
    }

  if (slice->sliceType == eSliceB) {
    int value = slice->refPicReorderFlag[LIST_1] = s.readU1 ("SLC refPicReorderL1");
    if (value) {
      int i = 0;
      do {
        value = slice->modPicNumsIdc[LIST_1][i] = s.readUeV ("SLC modPicNumsIdcl1");
        if ((value == 0) || (value == 1))
          slice->absDiffPicNumMinus1[LIST_1][i] = s.readUeV ("SLC absDiffPicNumMinus1L1");
        else if (value == 2)
          slice->longTermPicIndex[LIST_1][i] = s.readUeV ("SLC longTermPicIndexL1");
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
                              ? activePps->hasWeightedPred
                              : ((slice->sliceType == eSliceB) && (activePps->weightedBiPredIdc == 1)));

  slice->weightedBiPredIdc = (uint16_t)((slice->sliceType == eSliceB) &&
                                              (activePps->weightedBiPredIdc > 0));

  if ((activePps->hasWeightedPred &&
       ((slice->sliceType == eSliceP) || (slice->sliceType == eSliceSP))) ||
      ((activePps->weightedBiPredIdc == 1) && (slice->sliceType == eSliceB))) {
    slice->lumaLog2weightDenom = (uint16_t)s.readUeV ("SLC lumaLog2weightDenom");
    slice->wpRoundLuma = slice->lumaLog2weightDenom ? 1 << (slice->lumaLog2weightDenom - 1) : 0;

    if (activeSps->chromaFormatIdc) {
      slice->chromaLog2weightDenom = (uint16_t)s.readUeV ("SLC chromaLog2weightDenom");
      slice->wpRoundChroma = slice->chromaLog2weightDenom ? 1 << (slice->chromaLog2weightDenom - 1) : 0;
      }

    slice->resetWeightedPredParam();
    for (int i = 0; i < slice->numRefIndexActive[LIST_0]; i++) {
      //{{{  read l0 weights
      if (s.readU1 ("SLC hasLumaWeightL0")) {
        slice->weightedPredWeight[LIST_0][i][0] = s.readSeV ("SLC lumaWeightL0");
        slice->weightedPredOffset[LIST_0][i][0] = s.readSeV ("SLC lumaOffsetL0");
        slice->weightedPredOffset[LIST_0][i][0] = slice->weightedPredOffset[LIST_0][i][0] << (bitDepthLuma - 8);
        }
      else {
        slice->weightedPredWeight[LIST_0][i][0] = 1 << slice->lumaLog2weightDenom;
        slice->weightedPredOffset[LIST_0][i][0] = 0;
        }

      if (activeSps->chromaFormatIdc) {
        // l0 chroma weights
        int hasChromaWeightL0 = s.readU1 ("SLC hasChromaWeightL0");
        for (int j = 1; j < 3; j++) {
          if (hasChromaWeightL0) {
            slice->weightedPredWeight[LIST_0][i][j] = s.readSeV ("SLC chromaWeightL0");
            slice->weightedPredOffset[LIST_0][i][j] = s.readSeV ("SLC chromaOffsetL0");
            slice->weightedPredOffset[LIST_0][i][j] = slice->weightedPredOffset[LIST_0][i][j] << (bitDepthChroma-8);
            }
          else {
            slice->weightedPredWeight[LIST_0][i][j] = 1 << slice->chromaLog2weightDenom;
            slice->weightedPredOffset[LIST_0][i][j] = 0;
            }
          }
        }
      }
      //}}}

    if ((slice->sliceType == eSliceB) && activePps->weightedBiPredIdc == 1)
      for (int i = 0; i < slice->numRefIndexActive[LIST_1]; i++) {
        //{{{  read l1 weights
        if (s.readU1 ("SLC hasLumaWeightL1")) {
          // read l1 luma weights
          slice->weightedPredWeight[LIST_1][i][0] = s.readSeV ("SLC lumaWeightL1");
          slice->weightedPredOffset[LIST_1][i][0] = s.readSeV ("SLC lumaOffsetL1");
          slice->weightedPredOffset[LIST_1][i][0] = slice->weightedPredOffset[LIST_1][i][0] << (bitDepthLuma-8);
          }
        else {
          slice->weightedPredWeight[LIST_1][i][0] = 1 << slice->lumaLog2weightDenom;
          slice->weightedPredOffset[LIST_1][i][0] = 0;
          }

        if (activeSps->chromaFormatIdc) {
          int hasChromaWeightL1 = s.readU1 ("SLC hasChromaWeightL1");
          for (int j = 1; j < 3; j++) {
            if (hasChromaWeightL1) {
              // read l1 chroma weights
              slice->weightedPredWeight[LIST_1][i][j] = s.readSeV ("SLC chromaWeightL1");
              slice->weightedPredOffset[LIST_1][i][j] = s.readSeV ("SLC chromaOffsetL1");
              slice->weightedPredOffset[LIST_1][i][j] = slice->weightedPredOffset[LIST_1][i][j]<<(bitDepthChroma-8);
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
    readDecRefPicMarking (s, slice);

  if (activePps->entropyCoding &&
      (slice->sliceType != eSliceI) && (slice->sliceType != eSliceSI))
    slice->cabacInitIdc = s.readUeV ("SLC cabacInitIdc");
  else
    slice->cabacInitIdc = 0;
  //{{{  read qp
  slice->sliceQpDelta = s.readSeV ("SLC sliceQpDelta");
  slice->qp = 26 + activePps->picInitQpMinus26 + slice->sliceQpDelta;

  if ((slice->sliceType == eSliceSP) || (slice->sliceType == eSliceSI)) {
    if (slice->sliceType == eSliceSP)
      slice->spSwitch = s.readU1 ("SLC sp_for_switchFlag");
    slice->sliceQsDelta = s.readSeV ("SLC sliceQsDelta");
    slice->qs = 26 + activePps->picInitQsMinus26 + slice->sliceQsDelta;
    }
  //}}}

  if (activePps->hasDeblockFilterControl) {
    //{{{  read deblockFilter
    slice->deblockFilterDisableIdc = (int16_t)s.readUeV ("SLC disable_deblocking_filter_idc");
    if (slice->deblockFilterDisableIdc != 1) {
      slice->deblockFilterC0Offset = (int16_t)(2 * s.readSeV ("SLC slice_alpha_c0_offset_div2"));
      slice->deblockFilterBetaOffset = (int16_t)(2 * s.readSeV ("SLC slice_beta_offset_div2"));
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
  if (activeSps->isHiIntraOnlyProfile() && !param.intraProfileDeblocking) {
    //{{{  hiIntra deblock
    slice->deblockFilterDisableIdc = 1;
    slice->deblockFilterC0Offset = 0;
    slice->deblockFilterBetaOffset = 0;
    }
    //}}}
  //{{{  read sliceGroup
  if ((activePps->numSliceGroupsMinus1 > 0) &&
      (activePps->sliceGroupMapType >= 3) &&
      (activePps->sliceGroupMapType <= 5)) {
    int len = (activeSps->picHeightMapUnitsMinus1+1) * (activeSps->picWidthMbsMinus1+1) /
                (activePps->sliceGroupChangeRateMius1 + 1);
    if (((activeSps->picHeightMapUnitsMinus1+1) * (activeSps->picWidthMbsMinus1+1)) %
        (activePps->sliceGroupChangeRateMius1 + 1))
      len += 1;

    len = ceilLog2 (len+1);
    slice->sliceGroupChangeCycle = s.readUv (len, "SLC sliceGroupChangeCycle");
    }
  //}}}

  picHeightInMbs = coding.frameHeightMbs / ( 1 + slice->fieldPic );
  picSizeInMbs = coding.picWidthMbs * picHeightInMbs;
  coding.frameSizeMbs = coding.picWidthMbs * coding.frameHeightMbs;
  }
//}}}
//{{{
int cDecoder264::readSlice (cSlice* slice) {

  int curHeader = 0;

  for (;;) {
    if (pendingNalu) {
      nalu = pendingNalu;
      pendingNalu = NULL;
      }
    else if (!nalu->readNalu (this))
      return eEOS;

  processNalu:
    switch (nalu->unitType) {
      case cNalu::NALU_TYPE_SLICE:
      //{{{
      case cNalu::NALU_TYPE_IDR: {
        // recovery
        if (recoveryPoint || nalu->unitType == cNalu::NALU_TYPE_IDR) {
          if (!recoveryPointFound) {
            if (nalu->unitType != cNalu::NALU_TYPE_IDR) {
              cLog::log (LOGINFO,  "-> decoding without IDR");
              nonConformingStream = true;
              }
            else
              nonConformingStream = false;
            }
          recoveryPointFound = 1;
          }
        if (!recoveryPointFound)
          break;

        // read sliceHeader
        slice->isIDR = (nalu->unitType == cNalu::NALU_TYPE_IDR);
        slice->refId = nalu->refId;
        slice->dataPartitionMode = eDataPartition1;
        slice->maxDataPartitions = 1;
        cBitStream& s = slice->dataPartitions[0].bitStream;
        s.readLen = 0;
        s.errorFlag = 0;
        s.bitStreamOffset = 0;
        memcpy (s.bitStreamBuffer, &nalu->buf[1], nalu->len-1);
        s.bitStreamLen = nalu->RBSPtoSODB (s.bitStreamBuffer);
        s.codeLen = s.bitStreamLen;

        readSliceHeader (slice);

        // if primary slice replaced by redundant slice, set correct image type
        if (slice->redundantPicCount && !isPrimaryOk && isRedundantOk)
          picture->sliceType = coding.sliceType;
        if (isNewPicture (picture, slice, oldSlice)) {
          if (!picSliceIndex)
            initPicture (slice);
          curHeader = eSOP;
          nalu->checkZeroByteVCL (this);
          }
        else
          curHeader = eSOS;

        slice->setQuantParams();
        slice->setSliceReadFunctions();

        if (slice->mbAffFrame)
          slice->mbIndex = slice->startMbNum << 1;
        else
          slice->mbIndex = slice->startMbNum;

        if (activePps->entropyCoding) {
          int byteStartPosition = s.bitStreamOffset / 8;
          if (s.bitStreamOffset % 8)
            ++byteStartPosition;
          slice->dataPartitions[0].cabacDecode.startDecoding (s.bitStreamBuffer, byteStartPosition, &s.readLen);
          }

        recoveryPoint = 0;

        // debug
        debug.sliceType = slice->sliceType;
        debug.sliceString = fmt::format ("{}:{}:{:6d} -> pps:{} frame:{:2d} {} {}{}",
                 (nalu->unitType == cNalu::NALU_TYPE_IDR) ? "IDR":"SLC", slice->refId, nalu->len,
                 slice->ppsId, slice->frameNum,
                 slice->sliceType ? (slice->sliceType == 1) ? 'B':((slice->sliceType == 2) ? 'I':'?'):'P',
                 slice->fieldPic ? " field":"", slice->mbAffFrame ? " mbAff":"");
        if (param.sliceDebug)
          cLog::log (LOGINFO, debug.sliceString);

        return curHeader;
        }
      //}}}

      //{{{
      case cNalu::NALU_TYPE_SPS: {
        int spsId = cSps::readNalu (this, nalu);
        if (param.spsDebug)
          cLog::log (LOGINFO, sps[spsId].getString());
        break;
        }
      //}}}
      //{{{
      case cNalu::NALU_TYPE_PPS: {
        int ppsId = cPps::readNalu (this, nalu);
        if (param.ppsDebug)
          cLog::log (LOGINFO, pps[ppsId].getString());
        break;
        }
      //}}}

      case cNalu::NALU_TYPE_SEI:
        processSei (nalu->buf, nalu->len, this, slice);
        break;

      //{{{
      case cNalu::NALU_TYPE_DPA: {
        cLog::log (LOGINFO, "DPA id:%d:%d len:%d", slice->refId, slice->sliceType, nalu->len);

        if (!recoveryPointFound)
          break;

        // read dataPartition A
        slice->isIDR = false;
        slice->refId = nalu->refId;
        slice->noDataPartitionB = 1;
        slice->noDataPartitionC = 1;
        slice->dataPartitionMode = eDataPartition3;
        slice->maxDataPartitions = 3;
        cBitStream& s = slice->dataPartitions[0].bitStream;
        s.errorFlag = 0;
        s.bitStreamOffset = s.readLen = 0;
        memcpy (&s.bitStreamBuffer, &nalu->buf[1], nalu->len - 1);
        s.codeLen = s.bitStreamLen = nalu->RBSPtoSODB (s.bitStreamBuffer);
        readSliceHeader (slice);

        if (isNewPicture (picture, slice, oldSlice)) {
          if (!picSliceIndex)
            initPicture (slice);
          curHeader = eSOP;
          nalu->checkZeroByteVCL (this);
          }
        else
          curHeader = eSOS;

        slice->setQuantParams();
        slice->setSliceReadFunctions();
        if (slice->mbAffFrame)
          slice->mbIndex = slice->startMbNum << 1;
        else
          slice->mbIndex = slice->startMbNum;

        // need to read the slice ID, which depends on the value of redundantPicCountPresent
        int slice_id_a = s.readUeV ("NALU: DP_A slice_id");
        if (activePps->entropyCoding)
          cDecoder264::error ("dataPartition with eCabac not allowed");

        if (!nalu->readNalu (this))
          return curHeader;

        if (cNalu::NALU_TYPE_DPB == nalu->unitType) {
          //{{{  got nalu dataPartitionB
          s = slice->dataPartitions[1].bitStream;
           s.errorFlag = 0;
           s.bitStreamOffset = s.readLen = 0;
          memcpy (&s.bitStreamBuffer, &nalu->buf[1], nalu->len-1);
           s.codeLen = s.bitStreamLen = nalu->RBSPtoSODB (s.bitStreamBuffer);
          int slice_id_b = s.readUeV ("NALU dataPartitionB sliceId");
          slice->noDataPartitionB = 0;

          if ((slice_id_b != slice_id_a) || (nalu->lostPackets)) {
            cLog::log (LOGINFO, "NALU dataPartitionB does not match dataPartitionA");
            slice->noDataPartitionB = 1;
            slice->noDataPartitionC = 1;
            }
          else {
            if (activePps->redundantPicCountPresent)
              s.readUeV ("NALU dataPartitionB redundantPicCount");

            // we're finished with dataPartitionB, so let's continue with next dataPartition
            if (!nalu->readNalu (this))
              return curHeader;
            }
          }
          //}}}
        else
          slice->noDataPartitionB = 1;

        if (cNalu::NALU_TYPE_DPC == nalu->unitType) {
          //{{{  got nalu dataPartitionC
          s = slice->dataPartitions[2].bitStream;
          s.errorFlag = 0;
          s.bitStreamOffset = s.readLen = 0;
          memcpy (&s.bitStreamBuffer, &nalu->buf[1], nalu->len-1);
          s.codeLen = s.bitStreamLen = nalu->RBSPtoSODB (s.bitStreamBuffer);

          slice->noDataPartitionC = 0;
          int slice_id_c = s.readUeV ("NALU: DP_C slice_id");
          if ((slice_id_c != slice_id_a) || (nalu->lostPackets)) {
            cLog::log (LOGINFO, "dataPartitionC does not match dataPartitionA");
            slice->noDataPartitionC = 1;
            }

          if (activePps->redundantPicCountPresent)
            s.readUeV ("NALU:SLICE_C redudand_pic_cnt");
          }
          //}}}
        else {
          slice->noDataPartitionC = 1;
          pendingNalu = nalu;
          }

        // check if we read anything else than the expected dataPartitions
        if ((nalu->unitType != cNalu::NALU_TYPE_DPB) &&
            (nalu->unitType != cNalu::NALU_TYPE_DPC) && (!slice->noDataPartitionC))
          goto processNalu;

        return curHeader;
        }
      //}}}
      //{{{
      case cNalu::NALU_TYPE_DPB:
        cLog::log (LOGINFO, "dataPartitionB without dataPartitonA");
        break;
      //}}}
      //{{{
      case cNalu::NALU_TYPE_DPC:
        cLog::log (LOGINFO, "dataPartitionC without dataPartitonA");
        break;
      //}}}

      case cNalu::NALU_TYPE_AUD: break;
      case cNalu::NALU_TYPE_FILL: break;
      case cNalu::NALU_TYPE_EOSEQ: break;
      case cNalu::NALU_TYPE_EOSTREAM: break;

      default:
        cLog::log (LOGINFO, "NALU:%d unknown:%d\n", nalu->len, nalu->unitType);
        break;
      }
    }

  return curHeader;
  }
//}}}
//{{{
void cDecoder264::decodeSlice (cSlice* slice) {

  bool endOfSlice = false;

  slice->codCount = -1;

  if (coding.isSeperateColourPlane)
    changePlaneJV (slice->colourPlaneId, slice);
  else {
    slice->mbData = mbData;
    slice->picture = picture;
    slice->siBlock = siBlock;
    slice->predMode = predMode;
    slice->intraBlock = intraBlock;
    }

  if (slice->sliceType == eSliceB)
    slice->computeColocated (slice->listX);

  if ((slice->sliceType != eSliceI) && (slice->sliceType != eSliceSI))
    initRefPicture (slice);

  // loop over macroBlocks
  while (!endOfSlice) {
    sMacroBlock* mb;
    slice->startMacroBlockDecode (&mb);
    slice->readMacroBlock (mb);
    decodeMacroBlock (mb, slice->picture);

    if (slice->mbAffFrame && mb->mbField) {
      slice->numRefIndexActive[LIST_0] >>= 1;
      slice->numRefIndexActive[LIST_1] >>= 1;
      }

    ercWriteMbModeMv (mb);
    endOfSlice = slice->endMacroBlockDecode (!slice->mbAffFrame || (slice->mbIndex % 2));
    }
  }
//}}}
//{{{
int cDecoder264::decodeFrame() {

  int ret = 0;

  numDecodedMbs = 0;
  picSliceIndex = 0;
  numDecodedSlices = 0;

  int curHeader = 0;
  if (newFrame) {
    // get firstSlice from sliceList;
    cSlice* slice = sliceList[picSliceIndex];
    sliceList[picSliceIndex] = nextSlice;
    nextSlice = slice;

    slice = sliceList[picSliceIndex];
    useParameterSet (slice);
    initPicture (slice);

    picSliceIndex++;
    curHeader = eSOS;
    }

  while ((curHeader != eSOP) && (curHeader != eEOS)) {
    //{{{  no pending slices
    if (!sliceList[picSliceIndex])
      sliceList[picSliceIndex] = cSlice::allocSlice();

    cSlice* slice = sliceList[picSliceIndex];
    slice->decoder = this;
    slice->dpb = &dpb; //set default value;
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
      isPrimaryOk = isRedundantOk = 1;

    if (!slice->redundantPicCount && (coding.sliceType != eSliceI)) {
      for (int i = 0; i < slice->numRefIndexActive[LIST_0];++i)
        if (!slice->refFlag[i]) // reference primary slice incorrect, primary slice incorrect
          isPrimaryOk = 0;
      }
    else if (slice->redundantPicCount && (coding.sliceType != eSliceI)) // reference redundant slice incorrect
      if (!slice->refFlag[slice->redundantSliceRefIndex]) // redundant slice is incorrect
        isRedundantOk = 0;

    // If primary and redundant received, primary is correct
    //   discard redundant
    // else
    //   primary slice replaced with redundant slice.
    if ((slice->frameNum == prevFrameNum) &&
        slice->redundantPicCount && isPrimaryOk && (curHeader != eEOS))
      continue;

    if (((curHeader != eSOP) && (curHeader != eEOS)) ||
        ((curHeader == eSOP) && !picSliceIndex)) {
       slice->curSliceIndex = (int16_t)picSliceIndex;
       picture->maxSliceId = (int16_t)imax (slice->curSliceIndex, picture->maxSliceId);
       if (picSliceIndex > 0) {
         (*sliceList)->copyPoc (slice);
         sliceList[picSliceIndex-1]->endMbNumPlus1 = slice->startMbNum;
         }

       picSliceIndex++;
       if (picSliceIndex >= numAllocatedSlices)
         cDecoder264::error ("decodeFrame - sliceList numAllocationSlices too small");
      curHeader = eSOS;
      }

    else {
      if (sliceList[picSliceIndex-1]->mbAffFrame)
        sliceList[picSliceIndex-1]->endMbNumPlus1 = coding.frameSizeMbs / 2;
      else
        sliceList[picSliceIndex-1]->endMbNumPlus1 =
          coding.frameSizeMbs / (sliceList[picSliceIndex-1]->fieldPic + 1);

      newFrame = 1;

      slice->curSliceIndex = 0;
      sliceList[picSliceIndex] = nextSlice;
      nextSlice = slice;
      }
    //}}}

    copySliceInfo (slice, oldSlice);
    }
    //}}}

  // decode slices
  ret = curHeader;
  initPictureDecode();
  for (int sliceIndex = 0; sliceIndex < picSliceIndex; sliceIndex++) {
    cSlice* slice = sliceList[sliceIndex];
    curHeader = slice->curHeader;
    initSlice (slice);

    if (slice->activePps->entropyCoding) {
      //{{{  init cabac
      initCabacContexts (slice);
      cabacNewSlice (slice);
      }
      //}}}

    if (((slice->activePps->weightedBiPredIdc > 0) && (slice->sliceType == eSliceB)) ||
        (slice->activePps->hasWeightedPred && (slice->sliceType != eSliceI)))
      slice->fillWeightedPredParam();

    if (((curHeader == eSOP) || (curHeader == eSOS)) && !slice->errorFlag)
      decodeSlice (slice);

    ercMvPerMb += slice->ercMvPerMb;
    numDecodedMbs += slice->numDecodedMbs;
    numDecodedSlices++;
    }

  endDecodeFrame();
  prevFrameNum = sliceList[0]->frameNum;

  return ret;
  }
//}}}
//{{{
void cDecoder264::endDecodeFrame() {

  // return if the last picture has already been finished
  if (!picture ||
      ((numDecodedMbs != picSizeInMbs) && ((coding.yuvFormat != YUV444) || !coding.isSeperateColourPlane)))
    return;

  //{{{  error conceal
  frame recfr;
  recfr.decoder = this;
  recfr.yptr = &picture->imgY[0][0];
  if (picture->chromaFormatIdc != YUV400) {
    recfr.uptr = &picture->imgUV[0][0][0];
    recfr.vptr = &picture->imgUV[1][0][0];
    }

  // this is always true at the beginning of a picture
  int ercSegment = 0;

  // mark the start of the first segment
  if (!picture->mbAffFrame) {
    int i;
    ercStartSegment (0, ercSegment, 0 , ercErrorVar);
    // generate the segments according to the macroBlock map
    for (i = 1; i < (int)(picture->picSizeInMbs); ++i) {
      if (mbData[i].errorFlag != mbData[i-1].errorFlag) {
        ercStopSegment (i-1, ercSegment, 0, ercErrorVar); //! stop current segment

        // mark current segment as lost or OK
        if(mbData[i-1].errorFlag)
          ercMarksegmentLost (picture->sizeX, ercErrorVar);
        else
          ercMarksegmentOK (picture->sizeX, ercErrorVar);

        ++ercSegment;  //! next segment
        ercStartSegment (i, ercSegment, 0 , ercErrorVar); //! start new segment
        }
      }

    // mark end of the last segment
    ercStopSegment (picture->picSizeInMbs-1, ercSegment, 0, ercErrorVar);
    if (mbData[i-1].errorFlag)
      ercMarksegmentLost (picture->sizeX, ercErrorVar);
    else
      ercMarksegmentOK (picture->sizeX, ercErrorVar);

    // call the right error conceal function depending on the frame type.
    ercMvPerMb /= picture->picSizeInMbs;
    if (picture->sliceType == eSliceI || picture->sliceType == eSliceSI) // I-frame
      ercConcealIntraFrame (this, &recfr, picture->sizeX, picture->sizeY, ercErrorVar);
    else
      ercConcealInterFrame (&recfr, ercObjectList,
                            picture->sizeX, picture->sizeY, ercErrorVar,
                            picture->chromaFormatIdc);
    }
  //}}}
  if (!deblockMode &&
      param.deblock &&
      (deblockEnable & (1 << picture->usedForRef))) {
    if (coding.isSeperateColourPlane) {
      //{{{  deblockJV
      int colourPlaneId = sliceList[0]->colourPlaneId;
      for (int nplane = 0; nplane < MAX_PLANE; ++nplane) {
        sliceList[0]->colourPlaneId = nplane;
        changePlaneJV (nplane, NULL );
        deblockPicture (this, picture);
        }
      sliceList[0]->colourPlaneId = colourPlaneId;
      makeFramePictureJV();
      }
      //}}}
    else
      deblockPicture (this, picture);
    }
  else if (coding.isSeperateColourPlane)
    makeFramePictureJV();

  if (picture->mbAffFrame)
    mbAffPostProc();
  if (coding.picStructure != eFrame)
     idrFrameNum /= 2;
  if (picture->usedForRef)
    padPicture (picture);

  int picStructure = picture->picStructure;
  int sliceType = picture->sliceType;
  int pocNum = picture->framePoc;
  int refpic = picture->usedForRef;
  int qp = picture->qp;
  int picNum = picture->picNum;
  int isIdr = picture->isIDR;
  dpb.storePicture (picture);
  picture = NULL;

  if (lastHasMmco5)
    preFrameNum = 0;

  if (picStructure == eTopField || picStructure == eFrame) {
    //{{{
    if (sliceType == eSliceI && isIdr)
      debug.sliceTypeString = "IDR";
    else if (sliceType == eSliceI)
      debug.sliceTypeString = " I ";
    else if (sliceType == eSliceP)
      debug.sliceTypeString = " P ";
    else if (sliceType == eSliceSP)
      debug.sliceTypeString = "SP ";
    else if  (sliceType == eSliceSI)
      debug.sliceTypeString = "SI ";
    else if (refpic)
      debug.sliceTypeString = " B ";
    else
      debug.sliceTypeString = " b ";
    }
    //}}}
  else if (picStructure == eBotField) {
    //{{{
    if (sliceType == eSliceI && isIdr)
      debug.sliceTypeString += "|IDR";
    else if (sliceType == eSliceI)
      debug.sliceTypeString += "| I ";
    else if (sliceType == eSliceP)
      debug.sliceTypeString += "| P ";
    else if (sliceType == eSliceSP)
      debug.sliceTypeString += "|SP ";
    else if  (sliceType == eSliceSI)
      debug.sliceTypeString += "|SI ";
    else if (refpic)
      debug.sliceTypeString += "| B ";
    else
      debug.sliceTypeString += "| b ";
    }
    //}}}
  if ((picStructure == eFrame) || picStructure == eBotField) {
    //{{{  debug
    getTime (&debug.endTime);

    // count numOutputFrames
    int numOutputFrames = 0;
    sDecodedPic* decodedPic = outDecodedPics;
    while (decodedPic) {
      if (decodedPic->ok)
        numOutputFrames++;
      decodedPic = decodedPic->next;
      }

    debug.outSliceType = (eSliceType)sliceType;

    debug.outString = fmt::format ("{} {}:{}:{:2d} {:3d}ms ->{}-> poc:{} pic:{} -> {}",
             decodeFrameNum,
             numDecodedSlices, numDecodedMbs, qp,
             (int)timeNorm (timeDiff (&debug.startTime, &debug.endTime)),
             debug.sliceTypeString,
             pocNum, picNum, numOutputFrames);

    if (param.outDebug)
      cLog::log (LOGINFO, "-> " + debug.outString);
    //}}}

    // I or P pictures ?
    if ((sliceType == eSliceI) || (sliceType == eSliceSI) || (sliceType == eSliceP) || refpic)
      ++(idrFrameNum);
    (decodeFrameNum)++;
    }
  }
//}}}
