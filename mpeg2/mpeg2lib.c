//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "global.h"
//}}}
//{{{  defines
#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */

#define RESERVED    -1
//}}}
static const int kDebug = 1;
//{{{  typedef struct VLCtab
typedef struct {
  char val, len;
  } VLCtab;
//}}}
//{{{  typedef struct DCTtab
typedef struct {
  char run, level, len;
  } DCTtab;
//}}}
//{{{  static tables
//{{{
/* Table B-3, macroblock_type in P-pictures, codes 001..1xx */
static VLCtab PMBtab0[8] = {
  {ERROR,0},
  {MACROBLOCK_MOTION_FORWARD,3},
  {MACROBLOCK_PATTERN,2}, {MACROBLOCK_PATTERN,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1}
};
//}}}
//{{{
/* Table B-3, macroblock_type in P-pictures, codes 000001..00011x */
static VLCtab PMBtab1[8] = {
  {ERROR,0},
  {MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
  {MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5}, {MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5}, {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5},
  {MACROBLOCK_INTRA,5}, {MACROBLOCK_INTRA,5}
};
//}}}

//{{{
/* Table B-4, macroblock_type in B-pictures, codes 0010..11xx */
static VLCtab BMBtab0[16] = {
  {ERROR,0},
  {ERROR,0},
  {MACROBLOCK_MOTION_FORWARD,4},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,4},
  {MACROBLOCK_MOTION_BACKWARD,3},
  {MACROBLOCK_MOTION_BACKWARD,3},
  {MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3},
  {MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2}
};
//}}}
//{{{
/* Table B-4, macroblock_type in B-pictures, codes 000001..00011x */
static VLCtab BMBtab1[8] = {
  {ERROR,0},
  {MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,6},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,6},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
  {MACROBLOCK_INTRA,5},
  {MACROBLOCK_INTRA,5}
};
//}}}

//{{{
/* Table B-10, motion_code, codes 0001 ... 01xx */
static VLCtab MVtab0[8] =
{ {ERROR,0}, {3,3}, {2,2}, {2,2}, {1,1}, {1,1}, {1,1}, {1,1}
};
//}}}
//{{{
/* Table B-10, motion_code, codes 0000011 ... 000011x */
static VLCtab MVtab1[8] =
{ {ERROR,0}, {ERROR,0}, {ERROR,0}, {7,6}, {6,6}, {5,6}, {4,5}, {4,5}
};
//}}}
//{{{
/* Table B-10, motion_code, codes 0000001100 ... 000001011x */
static VLCtab MVtab2[12] =
{ {16,9}, {15,9}, {14,9}, {13,9},
  {12,9}, {11,9}, {10,8}, {10,8},
  {9,8},  {9,8},  {8,8},  {8,8}
};
//}}}

//{{{
/* Table B-9, coded_block_pattern, codes 01000 ... 111xx */
static VLCtab CBPtab0[32] =
{ {ERROR,0}, {ERROR,0}, {ERROR,0}, {ERROR,0},
  {ERROR,0}, {ERROR,0}, {ERROR,0}, {ERROR,0},
  {62,5}, {2,5},  {61,5}, {1,5},  {56,5}, {52,5}, {44,5}, {28,5},
  {40,5}, {20,5}, {48,5}, {12,5}, {32,4}, {32,4}, {16,4}, {16,4},
  {8,4},  {8,4},  {4,4},  {4,4},  {60,3}, {60,3}, {60,3}, {60,3}
};
//}}}
//{{{
/* Table B-9, coded_block_pattern, codes 00000100 ... 001111xx */
static VLCtab CBPtab1[64] =
{ {ERROR,0}, {ERROR,0}, {ERROR,0}, {ERROR,0},
  {58,8}, {54,8}, {46,8}, {30,8},
  {57,8}, {53,8}, {45,8}, {29,8}, {38,8}, {26,8}, {37,8}, {25,8},
  {43,8}, {23,8}, {51,8}, {15,8}, {42,8}, {22,8}, {50,8}, {14,8},
  {41,8}, {21,8}, {49,8}, {13,8}, {35,8}, {19,8}, {11,8}, {7,8},
  {34,7}, {34,7}, {18,7}, {18,7}, {10,7}, {10,7}, {6,7},  {6,7},
  {33,7}, {33,7}, {17,7}, {17,7}, {9,7},  {9,7},  {5,7},  {5,7},
  {63,6}, {63,6}, {63,6}, {63,6}, {3,6},  {3,6},  {3,6},  {3,6},
  {36,6}, {36,6}, {36,6}, {36,6}, {24,6}, {24,6}, {24,6}, {24,6}
};
//}}}
//{{{
/* Table B-9, coded_block_pattern, codes 000000001 ... 000000111 */
static VLCtab CBPtab2[8] =
{ {ERROR,0}, {0,9}, {39,9}, {27,9}, {59,9}, {55,9}, {47,9}, {31,9}
};
//}}}

//{{{
/* Table B-1, macroblock_address_increment, codes 00010 ... 011xx */
static VLCtab MBAtab1[16] =
{ {ERROR,0}, {ERROR,0}, {7,5}, {6,5}, {5,4}, {5,4}, {4,4}, {4,4},
  {3,3}, {3,3}, {3,3}, {3,3}, {2,3}, {2,3}, {2,3}, {2,3}
};
//}}}
//{{{
/* Table B-1, macroblock_address_increment, codes 00000011000 ... 0000111xxxx */
static VLCtab MBAtab2[104] =
{
  {33,11}, {32,11}, {31,11}, {30,11}, {29,11}, {28,11}, {27,11}, {26,11},
  {25,11}, {24,11}, {23,11}, {22,11}, {21,10}, {21,10}, {20,10}, {20,10},
  {19,10}, {19,10}, {18,10}, {18,10}, {17,10}, {17,10}, {16,10}, {16,10},
  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},
  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},
  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},
  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},
  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},
  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},
  {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
  {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
  {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},
  {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7}
};
//}}}

//{{{
/* Table B-12, dct_dc_size_luminance, codes 00xxx ... 11110 */
static VLCtab DClumtab0[32] =
{ {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
  {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
  {0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
  {4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}, {ERROR, 0}
};
//}}}
//{{{
/* Table B-12, dct_dc_size_luminance, codes 111110xxx ... 111111111 */
static VLCtab DClumtab1[16] =
{ {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6},
  {8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10,9}, {11,9}
};
//}}}
//{{{
/* Table B-13, dct_dc_size_chrominance, codes 00xxx ... 11110 */
static VLCtab DCchromtab0[32] =
{ {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
  {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
  {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
  {3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}, {ERROR, 0}
};
//}}}
//{{{
/* Table B-13, dct_dc_size_chrominance, codes 111110xxxx ... 1111111111 */
static VLCtab DCchromtab1[32] =
{ {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
  {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
  {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7},
  {8, 8}, {8, 8}, {8, 8}, {8, 8}, {9, 9}, {9, 9}, {10,10}, {11,10}
};
//}}}

//{{{
/* Table B-14, DCT coefficients table zero,
 * codes 0100 ... 1xxx (used for first (DC) coefficient)
 */
DCTtab DCTtabfirst[12] =
{
  {0,2,4}, {2,1,4}, {1,1,3}, {1,1,3},
  {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1},
  {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
//}}}
//{{{
/* Table B-14, DCT coefficients table zero,
 * codes 0100 ... 1xxx (used for all other coefficients)
 */
DCTtab DCTtabnext[12] =
{
  {0,2,4},  {2,1,4},  {1,1,3},  {1,1,3},
  {64,0,2}, {64,0,2}, {64,0,2}, {64,0,2}, /* EOB */
  {0,1,2},  {0,1,2},  {0,1,2},  {0,1,2}
};
//}}}
//{{{
/* Table B-14, DCT coefficients table zero,
 * codes 000001xx ... 00111xxx
 */
DCTtab DCTtab0[60] =
{
  {65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
  {2,2,7}, {2,2,7}, {9,1,7}, {9,1,7},
  {0,4,7}, {0,4,7}, {8,1,7}, {8,1,7},
  {7,1,6}, {7,1,6}, {7,1,6}, {7,1,6},
  {6,1,6}, {6,1,6}, {6,1,6}, {6,1,6},
  {1,2,6}, {1,2,6}, {1,2,6}, {1,2,6},
  {5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
  {13,1,8}, {0,6,8}, {12,1,8}, {11,1,8},
  {3,2,8}, {1,3,8}, {0,5,8}, {10,1,8},
  {0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
  {0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
  {4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
  {4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5}
};
//}}}
//{{{
/* Table B-15, DCT coefficients table one,
 * codes 000001xx ... 11111111
*/
DCTtab DCTtab0a[252] =
{
  {65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
  {7,1,7}, {7,1,7}, {8,1,7}, {8,1,7},
  {6,1,7}, {6,1,7}, {2,2,7}, {2,2,7},
  {0,7,6}, {0,7,6}, {0,7,6}, {0,7,6},
  {0,6,6}, {0,6,6}, {0,6,6}, {0,6,6},
  {4,1,6}, {4,1,6}, {4,1,6}, {4,1,6},
  {5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
  {1,5,8}, {11,1,8}, {0,11,8}, {0,10,8},
  {13,1,8}, {12,1,8}, {3,2,8}, {1,4,8},
  {2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
  {2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
  {1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
  {1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4}, /* EOB */
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
  {0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
  {0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
  {0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
  {9,1,7}, {9,1,7}, {1,3,7}, {1,3,7},
  {10,1,7}, {10,1,7}, {0,8,7}, {0,8,7},
  {0,9,7}, {0,9,7}, {0,12,8}, {0,13,8},
  {2,3,8}, {4,2,8}, {0,14,8}, {0,15,8}
};
//}}}
//{{{
/* Table B-14, DCT coefficients table zero,
 * codes 0000001000 ... 0000001111
 */
DCTtab DCTtab1[8] =
{
  {16,1,10}, {5,2,10}, {0,7,10}, {2,3,10},
  {1,4,10}, {15,1,10}, {14,1,10}, {4,2,10}
};
//}}}
//{{{
/* Table B-15, DCT coefficients table one,
 * codes 000000100x ... 000000111x
 */
DCTtab DCTtab1a[8] =
{
  {5,2,9}, {5,2,9}, {14,1,9}, {14,1,9},
  {2,4,10}, {16,1,10}, {15,1,9}, {15,1,9}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 000000010000 ... 000000011111
 */
DCTtab DCTtab2[16] =
{
  {0,11,12}, {8,2,12}, {4,3,12}, {0,10,12},
  {2,4,12}, {7,2,12}, {21,1,12}, {20,1,12},
  {0,9,12}, {19,1,12}, {18,1,12}, {1,5,12},
  {3,3,12}, {0,8,12}, {6,2,12}, {17,1,12}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 0000000010000 ... 0000000011111
 */
DCTtab DCTtab3[16] =
{
  {10,2,13}, {9,2,13}, {5,3,13}, {3,4,13},
  {2,5,13}, {1,7,13}, {1,6,13}, {0,15,13},
  {0,14,13}, {0,13,13}, {0,12,13}, {26,1,13},
  {25,1,13}, {24,1,13}, {23,1,13}, {22,1,13}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 00000000010000 ... 00000000011111
 */
DCTtab DCTtab4[16] =
{
  {0,31,14}, {0,30,14}, {0,29,14}, {0,28,14},
  {0,27,14}, {0,26,14}, {0,25,14}, {0,24,14},
  {0,23,14}, {0,22,14}, {0,21,14}, {0,20,14},
  {0,19,14}, {0,18,14}, {0,17,14}, {0,16,14}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 000000000010000 ... 000000000011111
 */
DCTtab DCTtab5[16] =
{
  {0,40,15}, {0,39,15}, {0,38,15}, {0,37,15},
  {0,36,15}, {0,35,15}, {0,34,15}, {0,33,15},
  {0,32,15}, {1,14,15}, {1,13,15}, {1,12,15},
  {1,11,15}, {1,10,15}, {1,9,15}, {1,8,15}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 0000000000010000 ... 0000000000011111
 */
DCTtab DCTtab6[16] =
{
  {1,18,16}, {1,17,16}, {1,16,16}, {1,15,16},
  {6,3,16}, {16,2,16}, {15,2,16}, {14,2,16},
  {13,2,16}, {12,2,16}, {11,2,16}, {31,1,16},
  {30,1,16}, {29,1,16}, {28,1,16}, {27,1,16}
};
//}}}

//{{{
static double frame_rate_Table[16] =
{
  0.0,
  ((23.0*1000.0)/1001.0),
  24.0,
  25.0,
  ((30.0*1000.0)/1001.0),
  30.0,
  50.0,
  ((60.0*1000.0)/1001.0),
  60.0,

  RESERVED,
  RESERVED,
  RESERVED,
  RESERVED,
  RESERVED,
  RESERVED,
  RESERVED
};
//}}}

//{{{
/* zig-zag and alternate scan patterns */
unsigned char scan[2][64] =
{
  { /* Zig-Zag scan pattern  */
    0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
    12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
    35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
  },
  { /* Alternate scan pattern */
    0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
    41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
    51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
    53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
  }
}
;
//}}}
//{{{
/* default intra quantization matrix */
unsigned char default_intra_quantizer_matrix[64] =
{
  8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83
}
;
//}}}
//{{{
/* non-linear quantization coefficient table */
unsigned char Non_Linear_quantizer_scale[32] =
{
   0, 1, 2, 3, 4, 5, 6, 7,
   8,10,12,14,16,18,20,22,
  24,28,32,36,40,44,48,52,
  56,64,72,80,88,96,104,112
}
;
//}}}
//{{{
/* color space conversion coefficients
 * for YCbCr -> RGB mapping
 *
 * entries are {crv,cbu,cgu,cgv}
 *
 * crv=(255/224)*65536*(1-cr)/0.5
 * cbu=(255/224)*65536*(1-cb)/0.5
 * cgu=(255/224)*65536*(cb/cg)*(1-cb)/0.5
 * cgv=(255/224)*65536*(cr/cg)*(1-cr)/0.5
 *
 * where Y=cr*R+cg*G+cb*B (cr+cg+cb=1)
 */

/* ISO/IEC 13818-2 section 6.3.6 sequence_display_extension() */

int Inverse_Table_6_9[8][4]
#ifdef GLOBAL
=
{
  {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
  {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
  {104597, 132201, 25675, 53279}, /* unspecified */
  {104597, 132201, 25675, 53279}, /* reserved */
  {104448, 132798, 24759, 53109}, /* FCC */
  {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
  {104597, 132201, 25675, 53279}, /* SMPTE 170M */
  {117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
}
#endif
;
//}}}
//}}}
static short iclip[1024]; /* clipping table */
static short* iclp;

//{{{
static int Get_Byte() {
/* MPEG-1 system layer demultiplexer */

  while (ld->Rdptr >= ld->Rdbfr+2048) {
    read (ld->Infile,ld->Rdbfr,2048);
    ld->Rdptr -= 2048;
    ld->Rdmax -= 2048;
    }

  return *ld->Rdptr++;
  }
//}}}
//{{{
static int Get_Word() {
/* extract a 16-bit word from the bitstream buffer */

  int Val = Get_Byte();
  return (Val<<8) | Get_Byte();
  }
//}}}
//{{{
static int Get_Long() {

  int i = Get_Word();
  return (i<<16) | Get_Word();
  }
//}}}
//{{{
static void Next_Packet() {
/* initialize buffer, call once before first getbits or showbits */
/* parse system layer, ignore everything we don't need */

  int l;
  for(;;) {
    unsigned int code = Get_Long();

    /* remove system layer byte stuffing */
    while ((code & 0xffffff00) != 0x100)
      code = (code<<8) | Get_Byte();
    switch (code) {
      //{{{
      case PACK_START_CODE: /* pack header */
        /* skip pack header (system_clock_reference and mux_rate) */
        ld->Rdptr += 8;
        break;
      //}}}
      //{{{
      case VIDEO_ELEMENTARY_STREAM:
        code = Get_Word();             /* packet_length */
        ld->Rdmax = ld->Rdptr + code;

        code = Get_Byte();

        if((code>>6)==0x02)
        {
          ld->Rdptr++;
          code=Get_Byte();  /* parse PES_header_data_length */
          ld->Rdptr+=code;    /* advance pointer by PES_header_data_length */
          printf("MPEG-2 PES packet\n");
          return;
        }
        else if(code==0xff)
        {
          /* parse MPEG-1 packet header */
          while((code=Get_Byte())== 0xFF);
        }

        /* stuffing bytes */
        if(code>=0x40)
        {
          if(code>=0x80)
          {
            fprintf(stderr,"Error in packet header\n");
            exit(1);
          }
          /* skip STD_buffer_scale */
          ld->Rdptr++;
          code = Get_Byte();
        }

        if(code>=0x30)
        {
          if(code>=0x40)
          {
            fprintf(stderr,"Error in packet header\n");
            exit(1);
          }
          /* skip presentation and decoding time stamps */
          ld->Rdptr += 9;
        }
        else if(code>=0x20)
        {
          /* skip presentation time stamps */
          ld->Rdptr += 4;
        }
        else if(code!=0x0f)
        {
          fprintf(stderr,"Error in packet header\n");
          exit(1);
        }
        return;
      //}}}
      //{{{
      case ISO_END_CODE: /* end */
        /* simulate a buffer full of sequence end codes */
        l = 0;
        while (l<2048)
        {
          ld->Rdbfr[l++] = SEQUENCE_END_CODE>>24;
          ld->Rdbfr[l++] = SEQUENCE_END_CODE>>16;
          ld->Rdbfr[l++] = SEQUENCE_END_CODE>>8;
          ld->Rdbfr[l++] = SEQUENCE_END_CODE&0xff;
        }
        ld->Rdptr = ld->Rdbfr;
        ld->Rdmax = ld->Rdbfr + 2048;
        return;
      //}}}
      //{{{
      default:
        if(code>=SYSTEM_START_CODE)
        {
          /* skip system headers and non-video packets*/
          code = Get_Word();
          ld->Rdptr += code;
        }
        else
        {
          fprintf(stderr,"Unexpected startcode %08x in system layer\n",code);
          exit(1);
        }
        break;
      //}}}
      }
    }
  }
//}}}
//{{{
static void Fill_Buffer() {

  int Buffer_Level = read (ld->Infile, ld->Rdbfr, 2048);
  ld->Rdptr = ld->Rdbfr;
  if (System_Stream_Flag)
    ld->Rdmax -= 2048;

  /* end of the bitstream file */
  if (Buffer_Level < 2048) {
    /* just to be safe */
    if (Buffer_Level < 0)
      Buffer_Level = 0;

    /* pad until the next to the next 32-bit word boundary */
    while (Buffer_Level & 3)
      ld->Rdbfr[Buffer_Level++] = 0;

    /* pad the buffer with sequence end codes */
    while (Buffer_Level < 2048) {
      ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE >> 24;
      ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE >> 16;
      ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE >> 8;
      ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE & 0xff;
      }
    }
  }
//}}}
//{{{
static void Flush_Buffer (int N) {

  int Incnt;
  ld->Bfr <<= N;
  Incnt = ld->Incnt -= N;
  if (Incnt <= 24) {
    if (System_Stream_Flag && (ld->Rdptr >= ld->Rdmax-4)) {
      do {
        if (ld->Rdptr >= ld->Rdmax)
          Next_Packet();
        ld->Bfr |= Get_Byte() << (24 - Incnt);
        Incnt += 8;
        }
      while (Incnt <= 24);
      }
    else if (ld->Rdptr < ld->Rdbfr+2044) {
      do {
        ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
        Incnt += 8;
        }
      while (Incnt <= 24);
      }
    else {
      do {
        if (ld->Rdptr >= ld->Rdbfr+2048)
          Fill_Buffer();
        ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
        Incnt += 8;
        }
      while (Incnt <= 24);
      }
    ld->Incnt = Incnt;
    }
  }
//}}}
//{{{
static void Initialize_Buffer() {

  ld->Incnt = 0;
  ld->Rdptr = ld->Rdbfr + 2048;
  ld->Rdmax = ld->Rdptr;

  ld->Bfr = 0;
  Flush_Buffer(0); /* fills valid data into bfr */
  }
//}}}
//{{{
static unsigned int Get_Bits (int N) {

  unsigned int Val = Show_Bits(N);
  Flush_Buffer(N);
  return Val;
  }
//}}}
//{{{
static unsigned int Get_Bits1() {
  return Get_Bits(1);
  }
//}}}
//{{{
static void Flush_Buffer32() {

  int Incnt;

  ld->Bfr = 0;

  Incnt = ld->Incnt;
  Incnt -= 32;

  if (System_Stream_Flag && (ld->Rdptr >= ld->Rdmax-4)) {
    while (Incnt <= 24) {
      if (ld->Rdptr >= ld->Rdmax)
        Next_Packet();
      ld->Bfr |= Get_Byte() << (24 - Incnt);
      Incnt += 8;
      }
    }
  else {
    while (Incnt <= 24) {
      if (ld->Rdptr >= ld->Rdbfr+2048)
        Fill_Buffer();
      ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
      Incnt += 8;
      }
    }
  ld->Incnt = Incnt;
  }
//}}}
//{{{
static unsigned int Get_Bits32() {

  unsigned int l = Show_Bits(32);
  Flush_Buffer32();
  return l;
  }
//}}}

//{{{
static void next_start_code() {
/* align to start of next next_start_code */

  Flush_Buffer (ld->Incnt&7);
  while (Show_Bits (24) != 0x01L)
    Flush_Buffer (8);
  }
//}}}
//{{{
static void marker_bit (char *text) {
  int marker = Get_Bits(1);
  }
//}}}

//{{{
static void sequence_extension() {
/* decode sequence extension */
/* ISO/IEC 13818-2 section 6.2.2.3 */

  int horizontal_size_extension;
  int vertical_size_extension;
  int bit_rate_extension;
  int vbv_buffer_size_extension;
  int pos;

  pos = ld->Bitcnt;

  ld->MPEG2_Flag = 1;
  ld->scalable_mode = SC_NONE; /* unless overwritten by sequence_scalable_extension() */
  layerId = 0;                /* unless overwritten by sequence_scalable_extension() */

  profile_and_level_indication = Get_Bits(8);
  progressive_sequence         = Get_Bits(1);
  chroma_format                = Get_Bits(2);
  horizontal_size_extension    = Get_Bits(2);
  vertical_size_extension      = Get_Bits(2);
  bit_rate_extension           = Get_Bits(12);
  marker_bit("sequence_extension");
  vbv_buffer_size_extension    = Get_Bits(8);
  low_delay                    = Get_Bits(1);
  frame_rate_extension_n       = Get_Bits(2);
  frame_rate_extension_d       = Get_Bits(5);

  frameRate = frame_rate_Table[frame_rate_code] *
    ((frame_rate_extension_n+1)/(frame_rate_extension_d+1));

  /* special case for 422 profile & level must be made */
  if((profile_and_level_indication>>7) & 1) {  /* escape bit of profile_and_level_indication set */

    /* 4:2:2 Profile @ Main Level */
    if((profile_and_level_indication&15)==5) {
      profile = PROFILE_422;
      level   = MAIN_LEVEL;
      }
    }
  else {
    profile = profile_and_level_indication >> 4;  /* Profile is upper nibble */
    level   = profile_and_level_indication & 0xF;  /* Level is lower nibble */
    }

  horizontal_size = (horizontal_size_extension<<12) | (horizontal_size&0x0fff);
  vertical_size = (vertical_size_extension<<12) | (vertical_size&0x0fff);

  /* ISO/IEC 13818-2 does not define bit_rate_value to be composed of
   * both the original bit_rate_value parsed in sequence_header() and
   * the optional bit_rate_extension in sequence_extension_header().
   * However, we use it for bitstream verification purposes.
   */
  bit_rate_value += (bit_rate_extension << 18);
  bit_rate = ((double) bit_rate_value) * 400.0;
  vbv_buffer_size += (vbv_buffer_size_extension << 10);

  if (kDebug) {
    printf("sequence extension (byte %d)\n",(pos>>3)-4);
    printf("  profile_and_level_indication=%d\n",profile_and_level_indication);
    if (profile_and_level_indication<128)
      printf("    profile=%d, level=%d\n",profile,level);
    printf("  progressive_sequence=%d\n",progressive_sequence);
    printf("  chroma_format=%d\n",chroma_format);
    printf("  horizontal_size_extension=%d\n",horizontal_size_extension);
    printf("  vertical_size_extension=%d\n",vertical_size_extension);
    printf("  bit_rate_extension=%d\n",bit_rate_extension);
    printf("  vbv_buffer_size_extension=%d\n",vbv_buffer_size_extension);
    printf("  low_delay=%d\n",low_delay);
    printf("  frame_rate_extension_n=%d\n",frame_rate_extension_n);
    printf("  frame_rate_extension_d=%d\n",frame_rate_extension_d);
    }
  }
//}}}
//{{{
static void sequence_display_extension() {
/* decode sequence display extension */

  int pos = ld->Bitcnt;
  video_format = Get_Bits(3);
  color_description = Get_Bits(1);

  if (color_description) {
    color_primaries = Get_Bits(8);
    transfer_characteristics = Get_Bits(8);
    matrix_coefficients = Get_Bits(8);
    }

  display_horizontal_size = Get_Bits(14);
  marker_bit ("sequence_display_extension");
  display_vertical_size = Get_Bits(14);

  if (kDebug) {
    printf ("sequence display extension (byte %d)\n",(pos>>3)-4);
    printf ("  video_format=%d\n",video_format);
    printf ("  color_description=%d\n",color_description);
    if (color_description) {
      printf ("    color_primaries=%d\n",color_primaries);
      printf ("    transfer_characteristics=%d\n",transfer_characteristics);
      printf ("    matrix_coefficients=%d\n",matrix_coefficients);
      }
    printf ("  display_horizontal_size=%d\n",display_horizontal_size);
    printf ("  display_vertical_size=%d\n",display_vertical_size);
    }
  }
//}}}
//{{{
static void quant_matrix_extension() {
/* decode quant matrix entension */
/* ISO/IEC 13818-2 section 6.2.3.2 */

  int i;
  int pos = ld->Bitcnt;
  if ((ld->load_intra_quantizer_matrix = Get_Bits(1))) {
    for (i = 0; i < 64; i++) {
      ld->chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
        = ld->intra_quantizer_matrix[scan[ZIG_ZAG][i]]
        = Get_Bits(8);
      }
    }

  if ((ld->load_non_intra_quantizer_matrix = Get_Bits(1))) {
    for (i = 0; i < 64; i++) {
      ld->chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
        = ld->non_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
        = Get_Bits (8);
      }
    }

  if ((ld->load_chroma_intra_quantizer_matrix = Get_Bits(1))) {
    for (i = 0; i < 64; i++)
      ld->chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
    }

  if ((ld->load_chroma_non_intra_quantizer_matrix = Get_Bits(1))) {
    for (i = 0; i < 64; i++)
      ld->chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
    }

  if (kDebug) {
    printf ("quant matrix extension (byte %d)\n",(pos>>3)-4);
    printf ("  load_intra_quantizer_matrix=%d\n", ld->load_intra_quantizer_matrix);
    printf ("  load_non_intra_quantizer_matrix=%d\n", ld->load_non_intra_quantizer_matrix);
    printf ("  load_chroma_intra_quantizer_matrix=%d\n", ld->load_chroma_intra_quantizer_matrix);
    printf ("  load_chroma_non_intra_quantizer_matrix=%d\n", ld->load_chroma_non_intra_quantizer_matrix);
    }
  }
//}}}
//{{{
static void picture_display_extension() {
/* decode picture display extension */
/* ISO/IEC 13818-2 section 6.2.3.3. */

  int i;
  int number_of_frame_center_offsets;
  int pos = ld->Bitcnt;
  /* based on ISO/IEC 13818-2 section 6.3.12
    (November 1994) Picture display extensions */

  /* derive number_of_frame_center_offsets */
  if (progressive_sequence) {
    if (repeat_first_field) {
      if (top_field_first)
        number_of_frame_center_offsets = 3;
      else
        number_of_frame_center_offsets = 2;
      }
    else
      number_of_frame_center_offsets = 1;
    }
  else {
    if (picture_structure != FRAME_PICTURE)
      number_of_frame_center_offsets = 1;
    else {
      if(repeat_first_field)
        number_of_frame_center_offsets = 3;
      else
        number_of_frame_center_offsets = 2;
      }
    }

  /* now parse */
  for (i = 0; i < number_of_frame_center_offsets; i++) {
    frame_center_horizontal_offset[i] = Get_Bits(16);
    marker_bit ("picture_display_extension, first marker bit");

    frame_center_vertical_offset[i] = Get_Bits(16);
    marker_bit ("picture_display_extension, second marker bit");
    }

  if (kDebug) {
    printf ("picture display extension (byte %d)\n",(pos>>3)-4);
    for (i = 0; i < number_of_frame_center_offsets; i++) {
      printf ("  frame_center_horizontal_offset[%d]=%d\n",i, frame_center_horizontal_offset[i]);
      printf ("  frame_center_vertical_offset[%d]=%d\n",i, frame_center_vertical_offset[i]);
      }
    }
  }
//}}}
//{{{
static void picture_coding_extension() {
/* decode picture coding extension */

  int pos = ld->Bitcnt;

  f_code[0][0] = Get_Bits(4);
  f_code[0][1] = Get_Bits(4);
  f_code[1][0] = Get_Bits(4);
  f_code[1][1] = Get_Bits(4);

  intra_dc_precision         = Get_Bits(2);
  picture_structure          = Get_Bits(2);
  top_field_first            = Get_Bits(1);
  frame_pred_frame_dct       = Get_Bits(1);
  concealment_motion_vectors = Get_Bits(1);
  ld->q_scale_type           = Get_Bits(1);
  intra_vlc_format           = Get_Bits(1);
  ld->alternate_scan         = Get_Bits(1);
  repeat_first_field         = Get_Bits(1);
  chroma_420_type            = Get_Bits(1);
  progressive_frame          = Get_Bits(1);
  composite_display_flag     = Get_Bits(1);

  if (composite_display_flag) {
    v_axis = Get_Bits(1);
    field_sequence = Get_Bits(3);
    sub_carrier = Get_Bits(1);
    burst_amplitude = Get_Bits(7);
    sub_carrier_phase = Get_Bits(8);
    }

  if (kDebug) {
    printf ("picture coding extension (byte %d)\n",(pos>>3)-4);
    printf ("  forward horizontal f_code=%d\n", f_code[0][0]);
    printf ("  forward vertical f_code=%d\n", f_code[0][1]);
    printf ("  backward horizontal f_code=%d\n", f_code[1][0]);
    printf ("  backward_vertical f_code=%d\n", f_code[1][1]);
    printf ("  intra_dc_precision=%d\n",intra_dc_precision);
    printf ("  picture_structure=%d\n",picture_structure);
    printf ("  top_field_first=%d\n",top_field_first);
    printf ("  frame_pred_frame_dct=%d\n",frame_pred_frame_dct);
    printf("  concealment_motion_vectors=%d\n",concealment_motion_vectors);
    printf ("  q_scale_type=%d\n",ld->q_scale_type);
    printf ("  intra_vlc_format=%d\n",intra_vlc_format);
    printf ("  alternate_scan=%d\n",ld->alternate_scan);
    printf ("  repeat_first_field=%d\n",repeat_first_field);
    printf ("  chroma_420_type=%d\n",chroma_420_type);
    printf ("  progressive_frame=%d\n",progressive_frame);
    printf ("  composite_display_flag=%d\n",composite_display_flag);

    if (composite_display_flag) {
      printf ("    v_axis=%d\n",v_axis);
      printf ("    field_sequence=%d\n",field_sequence);
      printf ("    sub_carrier=%d\n",sub_carrier);
      printf ("    burst_amplitude=%d\n",burst_amplitude);
      printf ("    sub_carrier_phase=%d\n",sub_carrier_phase);
      }
    }
  }
//}}}
//{{{
static int extra_bit_information() {
/* decode extra bit information */
/* ISO/IEC 13818-2 section 6.2.3.4. */

  int Byte_Count = 0;
  while (Get_Bits1()) {
    Flush_Buffer (8);
    Byte_Count++;
    }

  return (Byte_Count);
  }
//}}}

//{{{
static void addBlock (int comp, int bx, int by, int dct_type, int addflag) {
/* move/add 8x8-Block from block[comp] to backward_reference_frame */
/* copy reconstructed 8x8 block from block[comp] to current_frame[]
 * ISO/IEC 13818-2 section 7.6.8: Adding prediction and coefficient data
 * This stage also embodies some of the operations implied by:
 *   - ISO/IEC 13818-2 section 7.6.7: Combining predictions
 *   - ISO/IEC 13818-2 section 6.1.3: Macroblock
*/

  int cc,i, j, iincr;
  unsigned char *rfp;
  short *bp;

  /* derive color component index */
  /* equivalent to ISO/IEC 13818-2 Table 7-1 */
  cc = (comp<4) ? 0 : (comp&1)+1; /* color component index */
  if (cc==0) {
    /* luminance */
    if (picture_structure==FRAME_PICTURE)
      if (dct_type) {
        /* field DCT coding */
        rfp = current_frame[0] + Coded_Picture_Width*(by+((comp&2)>>1)) + bx + ((comp&1)<<3);
        iincr = (Coded_Picture_Width<<1) - 8;
        }
      else {
        /* frame DCT coding */
        rfp = current_frame[0] + Coded_Picture_Width*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
        iincr = Coded_Picture_Width - 8;
        }
    else {
      /* field picture */
      rfp = current_frame[0] + (Coded_Picture_Width<<1)*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
      iincr = (Coded_Picture_Width<<1) - 8;
      }
    }
  else {
    /* chrominance */
    /* scale coordinates */
    if (chroma_format!=CHROMA444)
      bx >>= 1;
    if (chroma_format==CHROMA420)
      by >>= 1;
    if (picture_structure==FRAME_PICTURE) {
      if (dct_type && (chroma_format != CHROMA420)) {
        /* field DCT coding */
        rfp = current_frame[cc] + Chroma_Width*(by+((comp&2)>>1)) + bx + (comp&8);
        iincr = (Chroma_Width<<1) - 8;
        }
      else {
        /* frame DCT coding */
        rfp = current_frame[cc] + Chroma_Width*(by+((comp&2)<<2)) + bx + (comp&8);
        iincr = Chroma_Width - 8;
        }
      }
    else {
      /* field picture */
      rfp = current_frame[cc] + (Chroma_Width<<1)*(by+((comp&2)<<2)) + bx + (comp&8);
      iincr = (Chroma_Width<<1) - 8;
      }
    }

  bp = ld->block[comp];

  if (addflag) {
    for (i = 0; i < 8; i++) {
      for (j = 0; j < 8; j++) {
        *rfp = Clip[*bp++ + *rfp];
        rfp++;
        }

      rfp+= iincr;
      }
    }
  else {
    for (i = 0; i < 8; i++) {
      for (j = 0; j < 8; j++)
        *rfp++ = Clip[*bp++ + 128];
      rfp+= iincr;
      }
    }
  }
//}}}
//{{{
static void clearBlock (int comp) {
/* IMPLEMENTATION: set scratch pad macroblock to zero */

  short* Block_Ptr = ld->block[comp];
  for (int i = 0; i < 64; i++)
    *Block_Ptr++ = 0;
  }
//}}}
//{{{
static void sumBlock (int comp) {
/* SCALABILITY: add SNR enhancement layer block data to base layer */
/* ISO/IEC 13818-2 section 7.8.3.4: Addition of coefficients from the two layes */

  short* Block_Ptr1 = base.block[comp];
  short* Block_Ptr2 = enhan.block[comp];
  for (int i = 0; i < 64; i++)
    *Block_Ptr1++ += *Block_Ptr2++;
  }
//}}}

//{{{
static void idctrow (short* blk) {
/* row (horizontal) IDCT
 *
 *           7                       pi         1
 * dst[k] = sum c[l] * src[l] * cos( -- * ( k + - ) * l )
 *          l=0                      8          2
 *
 * where: c[0]    = 128
 *        c[1..7] = 128*sqrt(2)
 */

  int x0, x1, x2, x3, x4, x5, x6, x7, x8;

  /* shortcut */
  if (!((x1 = blk[4] << 11) | (x2 = blk[6]) |
        (x3 = blk[2]) | (x4 = blk[1]) |
        (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3]))) {
    blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
    return;
    }

  x0 = (blk[0] << 11) + 128; /* for proper rounding in the fourth stage */

  /* first stage */
  x8 = W7*(x4+x5);
  x4 = x8 + (W1-W7)*x4;
  x5 = x8 - (W1+W7)*x5;
  x8 = W3*(x6+x7);
  x6 = x8 - (W3-W5)*x6;
  x7 = x8 - (W3+W5)*x7;

  /* second stage */
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6*(x3+x2);
  x2 = x1 - (W2+W6)*x2;
  x3 = x1 + (W2-W6)*x3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;

  /* third stage */
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181*(x4+x5)+128)>>8;
  x4 = (181*(x4-x5)+128)>>8;

  /* fourth stage */
  blk[0] = (x7+x1)>>8;
  blk[1] = (x3+x2)>>8;
  blk[2] = (x0+x4)>>8;
  blk[3] = (x8+x6)>>8;
  blk[4] = (x8-x6)>>8;
  blk[5] = (x0-x4)>>8;
  blk[6] = (x3-x2)>>8;
  blk[7] = (x7-x1)>>8;
  }
//}}}
//{{{
static void idctcol (short* blk) {
/* column (vertical) IDCT
 *             7                         pi         1
 * dst[8*k] = sum c[l] * src[8*l] * cos( -- * ( k + - ) * l )
 *            l=0                        8          2
 * where: c[0]    = 1/1024
 *        c[1..7] = (1/1024)*sqrt(2)
 */

  int x0, x1, x2, x3, x4, x5, x6, x7, x8;

  /* shortcut */
  if (!((x1 = (blk[8*4]<<8)) | (x2 = blk[8*6]) | (x3 = blk[8*2]) |
        (x4 = blk[8*1]) | (x5 = blk[8*7]) | (x6 = blk[8*5]) | (x7 = blk[8*3]))) {
    blk[8*0]=blk[8*1]=blk[8*2]=blk[8*3]=blk[8*4]=blk[8*5]=blk[8*6]=blk[8*7]= iclp[(blk[8*0]+32)>>6];
    return;
    }

  x0 = (blk[8*0]<<8) + 8192;

  /* first stage */
  x8 = W7*(x4+x5) + 4;
  x4 = (x8+(W1-W7)*x4)>>3;
  x5 = (x8-(W1+W7)*x5)>>3;
  x8 = W3*(x6+x7) + 4;
  x6 = (x8-(W3-W5)*x6)>>3;
  x7 = (x8-(W3+W5)*x7)>>3;

  /* second stage */
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6*(x3+x2) + 4;
  x2 = (x1-(W2+W6)*x2)>>3;
  x3 = (x1+(W2-W6)*x3)>>3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;

  /* third stage */
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181*(x4+x5)+128)>>8;
  x4 = (181*(x4-x5)+128)>>8;

  /* fourth stage */
  blk[8*0] = iclp[(x7+x1)>>14];
  blk[8*1] = iclp[(x3+x2)>>14];
  blk[8*2] = iclp[(x0+x4)>>14];
  blk[8*3] = iclp[(x8+x6)>>14];
  blk[8*4] = iclp[(x8-x6)>>14];
  blk[8*5] = iclp[(x0-x4)>>14];
  blk[8*6] = iclp[(x3-x2)>>14];
  blk[8*7] = iclp[(x7-x1)>>14];
  }
//}}}
//{{{
static void Fast_IDCT (short* block) {
/* two dimensional inverse discrete cosine transform */

  for (int i = 0; i < 8; i++)
    idctrow (block+8*i);

  for (int i = 0; i < 8; i++)
    idctcol (block+i);
  }
//}}}
//{{{
static void Initialize_Fast_IDCT() {

  iclp = iclip + 512;
  for (int i = -512; i < 512; i++)
    iclp[i] = (i<-256) ? -256 : ((i>255) ? 255 : i);
  }
//}}}

//{{{
static int Get_P_macroblock_type() {

  int code;
  if ((code = Show_Bits (6)) >= 8) {
    code >>= 3;
    Flush_Buffer (PMBtab0[code].len);
    return PMBtab0[code].val;
    }

  if (code == 0) {
    if (!Quiet_Flag)
      printf ("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
    }

  Flush_Buffer (PMBtab1[code].len);
  return PMBtab1[code].val;
  }
//}}}
//{{{
static int Get_B_macroblock_type() {

  int code;
  if ((code = Show_Bits(6))>=8) {
    code >>= 2;
    Flush_Buffer (BMBtab0[code].len);
    return BMBtab0[code].val;
    }

  if (code == 0) {
    if (!Quiet_Flag)
      printf ("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
    }

  Flush_Buffer (BMBtab1[code].len);
  return BMBtab1[code].val;
  }
//}}}
//{{{
static int Get_D_macroblock_type() {

  if (!Get_Bits1()) {
    if (!Quiet_Flag)
      printf ("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    }

  return 1;
  }
//}}}
//{{{
static int Get_I_macroblock_type() {

  if (Get_Bits1())
    return 1;

  if (!Get_Bits1()) {
    if (!Quiet_Flag)
      printf ("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    }

  return 17;
  }
//}}}
//{{{
static int Get_macroblock_type() {

  int macroblock_type = 0;

    switch (picture_coding_type) {
      case I_TYPE:
        macroblock_type = Get_I_macroblock_type();
        break;
      case P_TYPE:
        macroblock_type = Get_P_macroblock_type();
        break;
      case B_TYPE:
        macroblock_type = Get_B_macroblock_type();
        break;
      case D_TYPE:
        macroblock_type = Get_D_macroblock_type();
        break;
      default:
        printf("Get_macroblock_type(): unrecognized picture coding type\n");
        break;
      }

  return macroblock_type;
  }
//}}}
//{{{
static int Get_motion_code() {

  int code;
  if (Get_Bits1())
    return 0;

  if ((code = Show_Bits(9)) >= 64) {
    code >>= 6;
    Flush_Buffer (MVtab0[code].len);
    return Get_Bits1() ? -MVtab0[code].val : MVtab0[code].val;
    }

  if (code>=24) {
    code >>= 3;
    Flush_Buffer(MVtab1[code].len);
    return Get_Bits1() ? -MVtab1[code].val : MVtab1[code].val;
    }

  if ((code-=12) < 0) {
    Fault_Flag = 1;
    return 0;
    }

  Flush_Buffer (MVtab2[code].len);
  return Get_Bits1() ? -MVtab2[code].val : MVtab2[code].val;
  }
//}}}
//{{{
static int Get_dmvector() {
/* get differential motion vector (for dual prime prediction) */

  if (Get_Bits(1))
    return Get_Bits(1) ? -1 : 1;
  else
    return 0;
  }
//}}}
//{{{
static int Get_coded_block_pattern() {

  int code;
  if ((code = Show_Bits (9)) >= 128) {
    code >>= 4;
    Flush_Buffer (CBPtab0[code].len);
    return CBPtab0[code].val;
    }

  if (code >= 8) {
    code >>= 1;
    Flush_Buffer (CBPtab1[code].len);
    return CBPtab1[code].val;
    }

  if (code < 1) {
    Fault_Flag = 1;
    return 0;
    }

  Flush_Buffer (CBPtab2[code].len);
  return CBPtab2[code].val;
  }
//}}}
//{{{
static int Get_macroblock_address_increment() {

  int code;
  int val = 0;
  while ((code = Show_Bits (11)) < 24) {
    if (code != 15) {
      /* if not macroblock_stuffing */
      if (code == 8) /* if macroblock_escape */
        val+= 33;
      else {
        Fault_Flag = 1;
        return 1;
        }
      }

    Flush_Buffer (11);
    }

  /* macroblock_address_increment == 1 */
  /* ('1' is in the MSB position of the lookahead) */
  if (code >= 1024) {
    Flush_Buffer (1);
    return val + 1;
    }

  /* codes 00010 ... 011xx */
  if (code >= 128) {
    /* remove leading zeros */
    code >>= 6;
    Flush_Buffer (MBAtab1[code].len);
    return val + MBAtab1[code].val;
    }

  /* codes 00000011000 ... 0000111xxxx */
  code -= 24; /* remove common base */
  Flush_Buffer (MBAtab2[code].len);
  return val + MBAtab2[code].val;
  }
//}}}
//{{{
static int Get_Luma_DC_dct_diff() {
/* combined MPEG-1 and MPEG-2 stage. parse VLC and perform dct_diff arithmetic.
   MPEG-1:  ISO/IEC 11172-2 section
   MPEG-2:  ISO/IEC 13818-2 section 7.2.1
   Note: the arithmetic here is presented more elegantly than
   the spec, yet the results, dct_diff, are the same.
*/

  /* decode length */
  int code = Show_Bits(5);

  int size, dct_diff;
  if (code<31) {
    size = DClumtab0[code].val;
    Flush_Buffer (DClumtab0[code].len);
    }
  else {
    code = Show_Bits(9) - 0x1f0;
    size = DClumtab1[code].val;
    Flush_Buffer (DClumtab1[code].len);
    }

  if (size==0)
    dct_diff = 0;
  else {
    dct_diff = Get_Bits(size);
    if ((dct_diff & (1<<(size-1)))==0)
      dct_diff-= (1<<size) - 1;
    }

  return dct_diff;
  }
//}}}
//{{{
static int Get_Chroma_DC_dct_diff() {

  /* decode length */
  int code = Show_Bits (5);

  int size, dct_diff;
  if (code < 31) {
    size = DCchromtab0[code].val;
    Flush_Buffer (DCchromtab0[code].len);
    }
  else {
    code = Show_Bits (10) - 0x3e0;
    size = DCchromtab1[code].val;
    Flush_Buffer (DCchromtab1[code].len);
    }

  if (size == 0)
    dct_diff = 0;
  else {
    dct_diff = Get_Bits(size);
    if ((dct_diff & (1<<(size-1)))==0)
      dct_diff -= (1<<size) - 1;
    }

  return dct_diff;
  }
//}}}

//{{{
static void decode_motion_vector (int *pred, int r_size,
                                  int motion_code, int motion_residual, int full_pel_vector) {
/* calculate motion vector component */
/* ISO/IEC 13818-2 section 7.6.3.1: Decoding the motion vectors */
/* Note: the arithmetic here is more elegant than that which is shown
   in 7.6.3.1.  The end results (PMV[][][]) should, however, be the same.  */

  int lim = 16<<r_size;
  int vec = full_pel_vector ? (*pred >> 1) : (*pred);

  if (motion_code > 0) {
    vec+= ((motion_code-1)<<r_size) + motion_residual + 1;
    if (vec >= lim)
      vec -= lim + lim;
    }
  else if (motion_code < 0) {
    vec-= ((-motion_code-1)<<r_size) + motion_residual + 1;
    if (vec<-lim)
      vec += lim + lim;
    }

  *pred = full_pel_vector ? (vec<<1) : vec;
  }
//}}}
//{{{
static void motion_vector (int *PMV, int *dmvector, int h_r_size, int v_r_size,
                    int dmv, int mvscale, int full_pel_vector) {
/* get and decode motion vector and differential motion vector for one prediction */

  int motion_code, motion_residual;

  /* horizontal component */
  /* ISO/IEC 13818-2 Table B-10 */
  motion_code = Get_motion_code();
  motion_residual = (h_r_size!=0 && motion_code!=0) ? Get_Bits(h_r_size) : 0;
  decode_motion_vector (&PMV[0], h_r_size, motion_code, motion_residual, full_pel_vector);

  if (dmv)
    dmvector[0] = Get_dmvector();

  /* vertical component */
  motion_code = Get_motion_code();
  motion_residual = (v_r_size!=0 && motion_code!=0) ? Get_Bits(v_r_size) : 0;

  if (mvscale)
    PMV[1] >>= 1; /* DIV 2 */

  decode_motion_vector (&PMV[1], v_r_size, motion_code, motion_residual, full_pel_vector);

  if (mvscale)
    PMV[1] <<= 1;

  if (dmv)
    dmvector[1] = Get_dmvector();
  }
//}}}
//{{{
static void motion_vectors (int PMV[2][2][2], int dmvector[2], int motion_vertical_field_select[2][2],
                     int s, int motion_vector_count, int mv_format,
                     int h_r_size, int v_r_size, int dmv, int mvscale) {

  if (motion_vector_count == 1) {
    if (mv_format == MV_FIELD && !dmv) {
      motion_vertical_field_select[1][s] = motion_vertical_field_select[0][s] = Get_Bits(1);
      }
    motion_vector (PMV[0][s],dmvector,h_r_size,v_r_size,dmv,mvscale,0);

    /* update other motion vector predictors */
    PMV[1][s][0] = PMV[0][s][0];
    PMV[1][s][1] = PMV[0][s][1];
    }
  else {
    motion_vertical_field_select[0][s] = Get_Bits(1);
    motion_vector (PMV[0][s],dmvector,h_r_size,v_r_size,dmv,mvscale,0);
    motion_vertical_field_select[1][s] = Get_Bits(1);
    motion_vector (PMV[1][s],dmvector,h_r_size,v_r_size,dmv,mvscale,0);
    }
  }
//}}}
//{{{
static void Dual_Prime_Arithmetic (int DMV[][2], int *dmvector, int mvx, int mvy) {
/* ISO/IEC 13818-2 section 7.6.3.6: Dual prime additional arithmetic */

  if (picture_structure==FRAME_PICTURE) {
    if (top_field_first) {
      /* vector for prediction of top field from bottom field */
      DMV[0][0] = ((mvx  +(mvx>0))>>1) + dmvector[0];
      DMV[0][1] = ((mvy  +(mvy>0))>>1) + dmvector[1] - 1;

      /* vector for prediction of bottom field from top field */
      DMV[1][0] = ((3*mvx+(mvx>0))>>1) + dmvector[0];
      DMV[1][1] = ((3*mvy+(mvy>0))>>1) + dmvector[1] + 1;
      }
    else {
      /* vector for prediction of top field from bottom field */
      DMV[0][0] = ((3*mvx+(mvx>0))>>1) + dmvector[0];
      DMV[0][1] = ((3*mvy+(mvy>0))>>1) + dmvector[1] - 1;

      /* vector for prediction of bottom field from top field */
      DMV[1][0] = ((mvx  +(mvx>0))>>1) + dmvector[0];
      DMV[1][1] = ((mvy  +(mvy>0))>>1) + dmvector[1] + 1;
      }
    }
  else {
    /* vector for prediction from field of opposite 'parity' */
    DMV[0][0] = ((mvx+(mvx>0))>>1) + dmvector[0];
    DMV[0][1] = ((mvy+(mvy>0))>>1) + dmvector[1];

    /* correct for vertical field shift */
    if (picture_structure==TopField)
      DMV[0][1]--;
    else
      DMV[0][1]++;
    }
  }
//}}}
//{{{
static void form_component_prediction (unsigned char *src, unsigned char *dst,
                                       int lx, int lx2, int w, int h,
                                       int x, int y, int dx, int dy, int average_flag) {
/* ISO/IEC 13818-2 section 7.6.4: Forming predictions */
/* NOTE: the arithmetic below produces numerically equivalent results
 *  to 7.6.4, yet is more elegant. It differs in the following ways:
 *
 *   1. the vectors (dx, dy) are based on cartesian frame
 *      coordiantes along a half-pel grid (always positive numbers)
 *      In contrast, vector[r][s][t] are differential (with positive and
 *      negative values). As a result, deriving the integer vectors
 *      (int_vec[t]) from dx, dy is accomplished by a simple right shift.
 *
 *   2. Half pel flags (xh, yh) are equivalent to the LSB (Least
 *      Significant Bit) of the half-pel coordinates (dx,dy).
 *
 *  NOTE: the work of combining predictions (ISO/IEC 13818-2 section 7.6.7)
 *  is distributed among several other stages.  This is accomplished by
 *  folding line offsets into the source and destination (src,dst)
 *  addresses (note the call arguments to form_prediction() in Predict()),
 *  line stride variables lx and lx2, the block dimension variables (w,h),
 *  average_flag, and by the very order in which Predict() is called.
 *  This implementation design (implicitly different than the spec)
 *  was chosen for its elegance.
*/

  int xint;      /* horizontal integer sample vector: analogous to int_vec[0] */
  int yint;      /* vertical integer sample vectors: analogous to int_vec[1] */
  int xh;        /* horizontal half sample flag: analogous to half_flag[0]  */
  int yh;        /* vertical half sample flag: analogous to half_flag[1]  */
  int i, j, v;
  unsigned char *s;    /* source pointer: analogous to pel_ref[][]   */
  unsigned char *d;    /* destination pointer:  analogous to pel_pred[][]  */

  /* half pel scaling for integer vectors */
  xint = dx>>1;
  yint = dy>>1;

  /* derive half pel flags */
  xh = dx & 1;
  yh = dy & 1;

  /* compute the linear address of pel_ref[][] and pel_pred[][]
     based on cartesian/raster cordinates provided */
  s = src + lx*(y+yint) + x + xint;
  d = dst + lx*y + x;

  if (!xh && !yh) {
    //{{{  no horizontal nor vertical half-pel */
    if (average_flag) {
      for (j=0; j<h; j++) {
        for (i=0; i<w; i++) {
          v = d[i]+s[i];
          d[i] = (v+(v>=0?1:0))>>1;
          }

        s+= lx2;
        d+= lx2;
        }
      }
    else {
      for (j=0; j<h; j++) {
        for (i=0; i<w; i++)
          d[i] = s[i];
        s+= lx2;
        d+= lx2;
        }
      }
    }
    //}}}
  else if (!xh && yh) {
    //{{{  no horizontal but vertical half-pel */
    if (average_flag) {
      for (j=0; j<h; j++) {
        for (i=0; i<w; i++) {
          v = d[i] + ((unsigned int)(s[i]+s[i+lx]+1)>>1);
          d[i]=(v+(v>=0?1:0))>>1;
        }

        s+= lx2;
        d+= lx2;
        }
      }
    else {
      for (j=0; j<h; j++) {
        for (i=0; i<w; i++)
          d[i] = (unsigned int)(s[i]+s[i+lx]+1)>>1;

        s+= lx2;
        d+= lx2;
        }
      }
    }
    //}}}
  else if (xh && !yh) {
    //{{{  horizontal but no vertical half-pel */
    if (average_flag) {
      for (j=0; j<h; j++) {
        for (i=0; i<w; i++) {
          v = d[i] + ((unsigned int)(s[i]+s[i+1]+1)>>1);
          d[i] = (v+(v>=0?1:0))>>1;
          }

        s+= lx2;
        d+= lx2;
        }
      }
    else {
      for (j=0; j<h; j++) {
        for (i=0; i<w; i++) {
          d[i] = (unsigned int)(s[i]+s[i+1]+1)>>1;
          }

        s+= lx2;
        d+= lx2;
        }
      }
    }
    //}}}
  else {
    //{{{  if (xh && yh) horizontal and vertical half-pel */
    if (average_flag) {
      for (j=0; j<h; j++) {
        for (i=0; i<w; i++) {
          v = d[i] + ((unsigned int)(s[i]+s[i+1]+s[i+lx]+s[i+lx+1]+2)>>2);
          d[i] = (v+(v>=0?1:0))>>1;
          }

        s+= lx2;
        d+= lx2;
        }
      }
    else {
      for (j=0; j<h; j++) {
        for (i=0; i<w; i++) {
          d[i] = (unsigned int)(s[i]+s[i+1]+s[i+lx]+s[i+lx+1]+2)>>2;
          }

        s+= lx2;
        d+= lx2;
        }
      }
    }
    //}}}
  }
//}}}
//{{{
static void form_prediction (unsigned char *src[], int sfield,
                             unsigned char *dst[], int dfield,
                             int lx, int lx2, int w, int h, int x, int y, int dx, int dy,
                             int average_flag) {

  /* Y */
  form_component_prediction (src[0] + (sfield ? lx2>>1 : 0), dst[0] + (dfield ? lx2>>1 : 0),
                             lx, lx2, w, h, x, y, dx, dy, average_flag);

  if (chroma_format != CHROMA444) {
    lx >>= 1;
    lx2 >>= 1;
    w >>= 1;
    x >>= 1;
    dx /= 2;
    }

  if (chroma_format == CHROMA420) {
    h >>= 1;
    y >>= 1;
    dy /= 2;
    }

  /* Cb */
  form_component_prediction (src[1] + (sfield ? lx2>>1 :0), dst[1] + (dfield ? lx2>>1 : 0),
                             lx, lx2, w, h, x, y, dx, dy, average_flag);

  /* Cr */
  form_component_prediction (src[2] + (sfield ? lx2>>1 : 0), dst[2] + (dfield ? lx2>>1 : 0),
                             lx, lx2, w, h, x, y, dx, dy, average_flag);
  }
//}}}
//{{{
static void form_predictions (int bx, int by, int macroblock_type, int motion_type,
                       int PMV[2][2][2], int motion_vertical_field_select[2][2], int dmvector[2],
                       int stwtype) {

  int currentfield;
  unsigned char **predframe;
  int DMV[2][2];
  int stwtop, stwbot;

  stwtop = stwtype%3; /* 0:temporal, 1:(spat+temp)/2, 2:spatial */
  stwbot = stwtype/3;

  if ((macroblock_type & MACROBLOCK_MOTION_FORWARD) || (picture_coding_type == P_TYPE)) {
    if (picture_structure == FRAME_PICTURE) {
      if ((motion_type == MC_FRAME)
        || !(macroblock_type & MACROBLOCK_MOTION_FORWARD)) {
        //{{{  frame based prediction
        /* frame-based prediction (broken into top and bottom halves
             for spatial scalability prediction purposes) */
        if (stwtop<2)
          form_prediction (forward_reference_frame,0,current_frame,0,
                           Coded_Picture_Width,Coded_Picture_Width<<1,16,8,bx,by,
                           PMV[0][0][0],PMV[0][0][1],stwtop);

        if (stwbot<2)
          form_prediction (forward_reference_frame,1,current_frame,1,
                           Coded_Picture_Width,Coded_Picture_Width<<1,16,8,bx,by,
                           PMV[0][0][0],PMV[0][0][1],stwbot);
        }
        //}}}
      else if (motion_type == MC_FIELD) {
        //{{{  field-based prediction */
        /* top field prediction */
        if (stwtop<2)
          form_prediction (forward_reference_frame,motion_vertical_field_select[0][0],
                           current_frame,0,Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,
                           bx,by>>1,PMV[0][0][0],PMV[0][0][1]>>1,stwtop);

        /* bottom field prediction */
        if (stwbot<2)
          form_prediction (forward_reference_frame,motion_vertical_field_select[1][0],
                           current_frame,1,Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,
                           bx,by>>1,PMV[1][0][0],PMV[1][0][1]>>1,stwbot);
          }
        //}}}
      else if (motion_type == MC_DMV) {
        //{{{  dual prime prediction */
        /* calculate derived motion vectors */
        Dual_Prime_Arithmetic(DMV,dmvector,PMV[0][0][0],PMV[0][0][1]>>1);

        if (stwtop<2) {
          /* predict top field from top field */
          form_prediction (forward_reference_frame,0,current_frame,0,
                           Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,bx,by>>1,
                           PMV[0][0][0],PMV[0][0][1]>>1,0);

          /* predict and add to top field from bottom field */
          form_prediction (forward_reference_frame,1,current_frame,0,
                           Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,bx,by>>1,
                           DMV[0][0],DMV[0][1],1);
          }

        if (stwbot<2) {
          /* predict bottom field from bottom field */
          form_prediction (forward_reference_frame,1,current_frame,1,
                           Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,bx,by>>1,
                           PMV[0][0][0],PMV[0][0][1]>>1,0);

          /* predict and add to bottom field from top field */
          form_prediction (forward_reference_frame,0,current_frame,1,
                           Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,bx,by>>1,
                           DMV[1][0],DMV[1][1],1);
          }
        }
        //}}}
      else
        printf ("invalid motion_type\n");
      }
    else {
      //{{{  field picture */
      currentfield = (picture_structure==BotField);

      /* determine which frame to use for prediction */
      if ((picture_coding_type==P_TYPE) && Second_Field
         && (currentfield!=motion_vertical_field_select[0][0]))
        predframe = backward_reference_frame; /* same frame */
      else
        predframe = forward_reference_frame; /* previous frame */

      if ((motion_type==MC_FIELD)
        || !(macroblock_type & MACROBLOCK_MOTION_FORWARD)) {
        //{{{  field-based prediction */
        if (stwtop<2)
          form_prediction (predframe,motion_vertical_field_select[0][0],current_frame,0,
                           Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,16,bx,by,
                           PMV[0][0][0],PMV[0][0][1],stwtop);
        }
        //}}}
      else if (motion_type == MC_16X8) {
        if (stwtop<2) {
          form_prediction (predframe,motion_vertical_field_select[0][0],current_frame,0,
                           Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,bx,by,
                           PMV[0][0][0],PMV[0][0][1],stwtop);

          /* determine which frame to use for lower half prediction */
          if ((picture_coding_type==P_TYPE) && Second_Field
             && (currentfield!=motion_vertical_field_select[1][0]))
            predframe = backward_reference_frame; /* same frame */
          else
            predframe = forward_reference_frame; /* previous frame */

          form_prediction (predframe,motion_vertical_field_select[1][0],current_frame,0,
                           Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,bx,by+8,
                           PMV[1][0][0],PMV[1][0][1],stwtop);
          }
        }
      else if (motion_type==MC_DMV) {
        //{{{  dual prime prediction */
        if (Second_Field)
          predframe = backward_reference_frame; /* same frame */
        else
          predframe = forward_reference_frame; /* previous frame */

        /* calculate derived motion vectors */
        Dual_Prime_Arithmetic(DMV,dmvector,PMV[0][0][0],PMV[0][0][1]);

        /* predict from field of same parity */
        form_prediction (forward_reference_frame,currentfield,current_frame,0,
                         Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,16,bx,by,
                         PMV[0][0][0],PMV[0][0][1],0);

        /* predict from field of opposite parity */
        form_prediction (predframe,!currentfield,current_frame,0,
                         Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,16,bx,by,
                         DMV[0][0],DMV[0][1],1);
        }
        //}}}
      else
        /* invalid motion_type */
        printf ("invalid motion_type\n");
      }
      //}}}
    stwtop = stwbot = 1;
    }

  if (macroblock_type & MACROBLOCK_MOTION_BACKWARD) {
    if (picture_structure == FRAME_PICTURE) {
      if (motion_type == MC_FRAME) {
        //{{{  frame-based prediction */
        if (stwtop<2)
          form_prediction (backward_reference_frame,0,current_frame,0,
                           Coded_Picture_Width,Coded_Picture_Width<<1,16,8,bx,by,
                           PMV[0][1][0],PMV[0][1][1],stwtop);

        if (stwbot<2)
          form_prediction (backward_reference_frame,1,current_frame,1,
                           Coded_Picture_Width,Coded_Picture_Width<<1,16,8,bx,by,
                           PMV[0][1][0],PMV[0][1][1],stwbot);
        }
        //}}}
      else {
        //{{{  field-based prediction */
        /* top field prediction */
        if (stwtop<2)
          form_prediction (backward_reference_frame,motion_vertical_field_select[0][1],
                           current_frame,0,Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,
                           bx,by>>1,PMV[0][1][0],PMV[0][1][1]>>1,stwtop);

        /* bottom field prediction */
        if (stwbot<2)
          form_prediction (backward_reference_frame,motion_vertical_field_select[1][1],
                           current_frame,1,Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,
                           bx,by>>1,PMV[1][1][0],PMV[1][1][1]>>1,stwbot);
        }
        //}}}
      }
    else {
      //{{{  field picture */
      if (motion_type==MC_FIELD) {
        /* field-based prediction */
        form_prediction (backward_reference_frame,motion_vertical_field_select[0][1],
                         current_frame,0,Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,16,
                         bx,by,PMV[0][1][0],PMV[0][1][1],stwtop);
        }
      else if (motion_type == MC_16X8) {
        form_prediction (backward_reference_frame,motion_vertical_field_select[0][1],
                         current_frame,0,Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,
                         bx,by,PMV[0][1][0],PMV[0][1][1],stwtop);

        form_prediction (backward_reference_frame,motion_vertical_field_select[1][1],
                         current_frame,0,Coded_Picture_Width<<1,Coded_Picture_Width<<1,16,8,
                         bx,by+8,PMV[1][1][0],PMV[1][1][1],stwtop);
        }
      else
        printf ("invalid motion_type\n");
      }
      //}}}
    }
  }
//}}}
//{{{
static void frame_reorder (int Bitstream_Framenum, int Sequence_Framenum) {
/* tracking variables to insure proper output in spatial scalability */

  static int Oldref_progressive_frame, Newref_progressive_frame;

  if (Sequence_Framenum!=0) {
    if (picture_structure == FRAME_PICTURE || Second_Field) {
      if (picture_coding_type == B_TYPE)
        Write_Frame (auxframe, Bitstream_Framenum-1);
      else {
        Newref_progressive_frame = progressive_frame;
        progressive_frame = Oldref_progressive_frame;

        Write_Frame (forward_reference_frame, Bitstream_Framenum-1);

        Oldref_progressive_frame = progressive_frame = Newref_progressive_frame;
        }
      }
    }
  else
    Oldref_progressive_frame = progressive_frame;
  }
//}}}
//{{{
static void motion_compensation (int MBA, int macroblock_type, int motion_type,
                                 int PMV[2][2][2], int motion_vertical_field_select[2][2],
                                 int dmvector[2], int stwtype, int dct_type) {
/* ISO/IEC 13818-2 section 7.6 */

  /* derive current macroblock position within picture */
  /* ISO/IEC 13818-2 section 6.3.1.6 and 6.3.1.7 */
  int bx = 16*(MBA%mb_width);
  int by = 16*(MBA/mb_width);

  /* motion compensation */
  if (!(macroblock_type & MACROBLOCK_INTRA))
    form_predictions (bx,by,macroblock_type,motion_type,PMV, motion_vertical_field_select,dmvector,stwtype);

  /* copy or add block data into picture */
  for (int comp=0; comp<block_count; comp++) {
    /* SCALABILITY: SNR */
    Fast_IDCT (ld->block[comp]);

    /* ISO/IEC 13818-2 section 7.6.8: Adding prediction and coefficient data */
    addBlock (comp, bx, by, dct_type, (macroblock_type & MACROBLOCK_INTRA) == 0);
    }
  }
//}}}
//{{{
static void skipped_macroblock (int dc_dct_pred[3], int PMV[2][2][2],
                                int* motion_type, int motion_vertical_field_select[2][2],
                                int* stwtype, int* macroblock_type) {
/* ISO/IEC 13818-2 section 7.6.6 */

  for (int comp = 0; comp < block_count; comp++)
    clearBlock (comp);

  /* reset intra_dc predictors */
  /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
  dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;

  /* reset motion vector predictors */
  /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
  if (picture_coding_type == P_TYPE)
    PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0]=PMV[1][0][1]=0;

  /* derive motion_type */
  if (picture_structure == FRAME_PICTURE)
    *motion_type = MC_FRAME;
  else {
    *motion_type = MC_FIELD;

    /* predict from field of same parity */
    /* ISO/IEC 13818-2 section 7.6.6.1 and 7.6.6.3: P field picture and B field picture */
    motion_vertical_field_select[0][0] = motion_vertical_field_select[0][1] = (picture_structure == BotField);
    }

  /* skipped I are spatial-only predicted, */
  /* skipped P and B are temporal-only predicted */
  /* ISO/IEC 13818-2 section 7.7.6: Skipped macroblocks */
  *stwtype = (picture_coding_type == I_TYPE) ? 8 : 0;

 /* IMPLEMENTATION: clear MACROBLOCK_INTRA */
  *macroblock_type &= ~MACROBLOCK_INTRA;
  }
//}}}

//{{{
static void decode_MPEG2_Intra_Block (int comp, int dc_dct_pred[]) {

  int val, j, sign, nc, run;
  unsigned int code;

  /* with data partitioning, data always goes to base layer */
  struct layer_data* ld1 = &base;
  short* bp = ld1->block[comp];
  int cc = (comp<4) ? 0 : (comp&1)+1;
  int* qmat = (comp<4 || chroma_format==CHROMA420)
              ? ld1->intra_quantizer_matrix
              : ld1->chroma_intra_quantizer_matrix;

  /* ISO/IEC 13818-2 section 7.2.1: decode DC coefficients */
  if (cc==0)
    val = (dc_dct_pred[0]+= Get_Luma_DC_dct_diff());
  else if (cc==1)
    val = (dc_dct_pred[1]+= Get_Chroma_DC_dct_diff());
  else
    val = (dc_dct_pred[2]+= Get_Chroma_DC_dct_diff());

  if (Fault_Flag)
    return;

  bp[0] = val << (3-intra_dc_precision);
  nc = 0;

  /* decode AC coefficients */
  DCTtab* tab;
  for (int i = 1; ; i++) {
    code = Show_Bits(16);
    if (code>=16384 && !intra_vlc_format)
      tab = &DCTtabnext[(code>>12)-4];
    else if (code>=1024) {
      if (intra_vlc_format)
        tab = &DCTtab0a[(code>>8)-4];
      else
        tab = &DCTtab0[(code>>8)-4];
      }
    else if (code >= 512) {
      if (intra_vlc_format)
        tab = &DCTtab1a[(code>>6)-8];
      else
        tab = &DCTtab1[(code>>6)-8];
      }
    else if (code >= 256)
      tab = &DCTtab2[(code>>4)-16];
    else if (code >= 128)
      tab = &DCTtab3[(code>>3)-16];
    else if (code >= 64)
      tab = &DCTtab4[(code>>2)-16];
    else if (code >= 32)
      tab = &DCTtab5[(code>>1)-16];
    else if (code >= 16)
      tab = &DCTtab6[code-16];
    else {
      if (!Quiet_Flag)
        printf ("invalid Huffman code in Decode_MPEG2_Intra_Block()\n");
      Fault_Flag = 1;
      return;
      }

    Flush_Buffer(tab->len);

    if (tab->run == 64) /* end_of_block */
      return;

    if (tab->run == 65) {
      /* escape */
      i+= run = Get_Bits (6);
      val = Get_Bits (12);
      if ((val & 2047) == 0) {
        if (!Quiet_Flag)
          printf ("invalid escape in Decode_MPEG2_Intra_Block()\n");
        Fault_Flag = 1;
        return;
        }
      if ((sign = (val >= 2048)))
        val = 4096 - val;
      }
    else {
      i+= run = tab->run;
      val = tab->level;
      sign = Get_Bits(1);
      }

    if (i >= 64) {
      if (!Quiet_Flag)
        fprintf (stderr,"DCT coeff index (i) out of bounds (intra2)\n");
      Fault_Flag = 1;
      return;
      }

    j = scan[ld1->alternate_scan][i];
    val = (val * ld1->quantizer_scale * qmat[j]) >> 4;
    bp[j] = sign ? -val : val;
    nc++;
    }
  }
//}}}
//{{{
static void decode_MPEG2_Non_Intra_Block (int comp) {
/* decode one non-intra coded MPEG-2 block */

  int val, i, j, sign, nc, run;
  unsigned int code;
  DCTtab *tab;
  short *bp;
  int *qmat;

  /* with data partitioning, data always goes to base layer */
  struct layer_data* ld1 = &base;
  bp = ld1->block[comp];

  qmat = (comp<4 || chroma_format==CHROMA420)
         ? ld1->non_intra_quantizer_matrix
         : ld1->chroma_non_intra_quantizer_matrix;

  nc = 0;

  /* decode AC coefficients */
  for (i=0; ; i++) {
    code = Show_Bits(16);
    if (code>=16384) {
      if (i==0)
        tab = &DCTtabfirst[(code>>12)-4];
      else
        tab = &DCTtabnext[(code>>12)-4];
      }
    else if (code>=1024)
      tab = &DCTtab0[(code>>8)-4];
    else if (code>=512)
      tab = &DCTtab1[(code>>6)-8];
    else if (code>=256)
      tab = &DCTtab2[(code>>4)-16];
    else if (code>=128)
      tab = &DCTtab3[(code>>3)-16];
    else if (code>=64)
      tab = &DCTtab4[(code>>2)-16];
    else if (code>=32)
      tab = &DCTtab5[(code>>1)-16];
    else if (code>=16)
      tab = &DCTtab6[code-16];
    else {
      if (!Quiet_Flag)
        printf("invalid Huffman code in Decode_MPEG2_Non_Intra_Block()\n");
      Fault_Flag = 1;
      return;
      }

    Flush_Buffer (tab->len);

    if (tab->run == 64) /* end_of_block */
      return;

    if (tab->run == 65) {
      /* escape */
      i += run = Get_Bits (6);
      val = Get_Bits (12);
      if ((val & 2047) == 0) {
        if (!Quiet_Flag)
          printf ("invalid escape in Decode_MPEG2_Intra_Block()\n");
        Fault_Flag = 1;
        return;
        }
      if ((sign = (val >= 2048)))
        val = 4096 - val;
      }
    else {
      i+= run = tab->run;
      val = tab->level;
      sign = Get_Bits (1);

      }

    if (i >= 64) {
      if (!Quiet_Flag)
        fprintf (stderr,"DCT coeff index (i) out of bounds (inter2)\n");
      Fault_Flag = 1;
      return;
      }


    j = scan[ld1->alternate_scan][i];
    val = (((val << 1) + 1) * ld1->quantizer_scale * qmat[j]) >> 5;
    bp[j] = sign ? -val : val;
    nc++;
    }
  }
//}}}
//{{{
static void macroblock_modes (int* pmacroblock_type, int* pstwtype, int* pstwclass, int* pmotion_type,
                              int* pmotion_vector_count, int* pmv_format, int* pdmv,
                              int* pmvscale, int* pdct_type) {
/* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */

  int macroblock_type;
  int stwtype, stwcode, stwclass;
  int motion_type = 0;
  int motion_vector_count, mv_format, dmv, mvscale;
  int dct_type;
  static unsigned char stwc_table[3][4] = { {6,3,7,4}, {2,1,5,4}, {2,5,7,4} };
  static unsigned char stwclass_table[9] = {0, 1, 2, 1, 1, 2, 3, 3, 4};

  /* get macroblock_type */
  macroblock_type = Get_macroblock_type();

  if (Fault_Flag) return;

  /* get spatial_temporal_weight_code */
  if (macroblock_type & MB_WEIGHT) {
    if (spatial_temporal_weight_code_table_index==0)
      stwtype = 4;
    else {
      stwcode = Get_Bits(2);
      stwtype = stwc_table[spatial_temporal_weight_code_table_index-1][stwcode];
      }
    }
  else
    stwtype = (macroblock_type & MB_CLASS4) ? 8 : 0;

  /* SCALABILITY: derive spatial_temporal_weight_class (Table 7-18) */
  stwclass = stwclass_table[stwtype];

  /* get frame/field motion type */
  if (macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD)) {
    if (picture_structure==FRAME_PICTURE) {
      /* frame_motion_type */
      motion_type = frame_pred_frame_dct ? MC_FRAME : Get_Bits(2);
      }
    else {
      /* field_motion_type */
      motion_type = Get_Bits(2);
      }
    }
  else if ((macroblock_type & MACROBLOCK_INTRA) && concealment_motion_vectors) {
    /* conceal motion vectors */
    motion_type = (picture_structure==FRAME_PICTURE) ? MC_FRAME : MC_FIELD;
    }

  /* derive motion_vector_count, mv_format and dmv, (table 6-17, 6-18) */
  if (picture_structure==FRAME_PICTURE) {
    motion_vector_count = (motion_type==MC_FIELD && stwclass<2) ? 2 : 1;
    mv_format = (motion_type==MC_FRAME) ? MV_FRAME : MV_FIELD;
    }
  else {
    motion_vector_count = (motion_type==MC_16X8) ? 2 : 1;
    mv_format = MV_FIELD;
    }

  dmv = (motion_type==MC_DMV); /* dual prime */

  /* field mv predictions in frame pictures have to be scaled
   * ISO/IEC 13818-2 section 7.6.3.1 Decoding the motion vectors
   * IMPLEMENTATION: mvscale is derived for later use in motion_vectors()
   * it displaces the stage:
   *
   *    if((mv_format=="field")&&(t==1)&&(picture_structure=="Frame picture"))
   *      prediction = PMV[r][s][t] DIV 2;
   */
  mvscale = ((mv_format==MV_FIELD) && (picture_structure==FRAME_PICTURE));

  /* get dct_type (frame DCT / field DCT) */
  dct_type = (picture_structure==FRAME_PICTURE)
             && (!frame_pred_frame_dct)
             && (macroblock_type & (MACROBLOCK_PATTERN|MACROBLOCK_INTRA))
             ? Get_Bits(1)
             : 0;

  /* return values */
  *pmacroblock_type = macroblock_type;
  *pstwtype = stwtype;
  *pstwclass = stwclass;
  *pmotion_type = motion_type;
  *pmotion_vector_count = motion_vector_count;
  *pmv_format = mv_format;
  *pdmv = dmv;
  *pmvscale = mvscale;
  *pdct_type = dct_type;
  }
//}}}
//{{{
static int decode_macroblock (int* macroblock_type, int* stwtype, int* stwclass,
                              int* motion_type, int* dct_type,
                              int PMV[2][2][2], int dc_dct_pred[3],
                              int motion_vertical_field_select[2][2], int dmvector[2]) {

  int quantizer_scale_code;
  int comp;

  int motion_vector_count;
  int mv_format;
  int dmv;
  int mvscale;
  int coded_block_pattern;

  ld = &base;

  /* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
  macroblock_modes (macroblock_type, stwtype, stwclass,
                    motion_type, &motion_vector_count, &mv_format, &dmv, &mvscale, dct_type);

  if (Fault_Flag)
    return(0);  /* trigger: go to next slice */

  if (*macroblock_type & MACROBLOCK_QUANT) {
    quantizer_scale_code = Get_Bits(5);

    /* ISO/IEC 13818-2 section 7.4.2.2: Quantizer scale factor */
    if (ld->MPEG2_Flag)
      ld->quantizer_scale =
      ld->q_scale_type ? Non_Linear_quantizer_scale[quantizer_scale_code]
       : (quantizer_scale_code << 1);
    else
      ld->quantizer_scale = quantizer_scale_code;
    }

  /* motion vectors */
  /* ISO/IEC 13818-2 section 6.3.17.2: Motion vectors */
  /* decode forward motion vectors */
  if ((*macroblock_type & MACROBLOCK_MOTION_FORWARD)
      || ((*macroblock_type & MACROBLOCK_INTRA)
      && concealment_motion_vectors)) {
    if (ld->MPEG2_Flag)
      motion_vectors (PMV,dmvector,motion_vertical_field_select,
                      0,motion_vector_count,mv_format,f_code[0][0]-1,f_code[0][1]-1, dmv,mvscale);
    else
      motion_vector(PMV[0][0],dmvector,
      forward_f_code-1,forward_f_code-1,0,0,full_pel_forward_vector);
    }

  if (Fault_Flag)
    return(0);  /* trigger: go to next slice */

  /* decode backward motion vectors */
  if (*macroblock_type & MACROBLOCK_MOTION_BACKWARD) {
    if (ld->MPEG2_Flag)
      motion_vectors (PMV,dmvector,motion_vertical_field_select,
                      1,motion_vector_count,mv_format,f_code[1][0]-1,f_code[1][1]-1,0, mvscale);
    else
      motion_vector (PMV[0][1],dmvector,
                     backward_f_code-1,backward_f_code-1,0,0,full_pel_backward_vector);
    }

  if (Fault_Flag)
    return(0);  /* trigger: go to next slice */

  if ((*macroblock_type & MACROBLOCK_INTRA) && concealment_motion_vectors)
    Flush_Buffer(1); /* remove marker_bit */

  /* macroblock_pattern */
  /* ISO/IEC 13818-2 section 6.3.17.4: Coded block pattern */
  if (*macroblock_type & MACROBLOCK_PATTERN) {
    coded_block_pattern = Get_coded_block_pattern();
    if (chroma_format==CHROMA422) /* coded_block_pattern_1 */
      coded_block_pattern = (coded_block_pattern<<2) | Get_Bits(2);
    else if (chroma_format==CHROMA444) /* coded_block_pattern_2 */
      coded_block_pattern = (coded_block_pattern<<6) | Get_Bits(6);
    }
  else
    coded_block_pattern = (*macroblock_type & MACROBLOCK_INTRA) ? (1<<block_count)-1 : 0;

  if (Fault_Flag)
    return(0);  /* trigger: go to next slice */

  /* decode blocks */
  for (comp=0; comp<block_count; comp++) {
    clearBlock (comp);
    if (coded_block_pattern & (1<<(block_count-1-comp))) {
      if (*macroblock_type & MACROBLOCK_INTRA)
        decode_MPEG2_Intra_Block(comp,dc_dct_pred);
      else
        decode_MPEG2_Non_Intra_Block(comp);

      if (Fault_Flag)
        return(0);  /* trigger: go to next slice */
      }
    }

  if (picture_coding_type==D_TYPE) {
    /* remove end_of_macroblock (always 1, prevents startcode emulation) */
    /* ISO/IEC 11172-2 section 2.4.2.7 and 2.4.3.6 */
    marker_bit("D picture end_of_macroblock bit");
    }

  /* reset intra_dc predictors */
  /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
  if (!(*macroblock_type & MACROBLOCK_INTRA))
    dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;

  /* reset motion vector predictors */
  if ((*macroblock_type & MACROBLOCK_INTRA) && !concealment_motion_vectors) {
    /* intra mb without conceal motion vectors */
    /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
    PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
    PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;
    }

  /* special "No_MC" macroblock_type case */
  /* ISO/IEC 13818-2 section 7.6.3.5: Prediction in P pictures */
  if ((picture_coding_type==P_TYPE)
      && !(*macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_INTRA))) {
    /* non-intra mb without forward mv in a P picture */
    /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
    PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;

    /* derive motion_type */
    /* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes, frame_motion_type */
    if (picture_structure==FRAME_PICTURE)
      *motion_type = MC_FRAME;
    else {
      *motion_type = MC_FIELD;
      /* predict from field of same parity */
      motion_vertical_field_select[0][0] = (picture_structure==BotField);
      }
    }

  if (*stwclass==4) {
    /* purely spatially predicted macroblock */
    /* ISO/IEC 13818-2 section 7.7.5.1: Resetting motion vector predictions */
    PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
    PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;
    }

  /* successfully decoded macroblock */
  return(1);
  }
//}}}
//{{{
static int slice_header() {
/* decode slice header */
/* ISO/IEC 13818-2 section 6.2.4 */

  int slice_vertical_position_extension;
  int quantizer_scale_code;
  int pos;
  int slice_picture_id_enable = 0;
  int slice_picture_id = 0;
  int extra_information_slice = 0;

  pos = ld->Bitcnt;

  slice_vertical_position_extension =
    (ld->MPEG2_Flag && vertical_size>2800) ? Get_Bits(3) : 0;

  quantizer_scale_code = Get_Bits(5);
  ld->quantizer_scale =
    ld->MPEG2_Flag ? (ld->q_scale_type ? Non_Linear_quantizer_scale[quantizer_scale_code] : quantizer_scale_code<<1) : quantizer_scale_code;

  /* slice_id introduced in March 1995 as part of the video corridendum
     (after the IS was drafted in November 1994) */
  if (Get_Bits(1)) {
    ld->intra_slice = Get_Bits(1);
    slice_picture_id_enable = Get_Bits(1);
    slice_picture_id = Get_Bits(6);
    extra_information_slice = extra_bit_information();
    }
  else
    ld->intra_slice = 0;

  if (kDebug) {
    printf ("slice header (byte %d)\n",(pos>>3)-4);
    if (ld->MPEG2_Flag && vertical_size > 2800)
      printf ("  slice_vertical_position_extension=%d\n",slice_vertical_position_extension);

    printf ("  quantizer_scale_code=%d\n", quantizer_scale_code);
    printf ("  slice_picture_id_enable = %d\n", slice_picture_id_enable);
    if (slice_picture_id_enable)
      printf ("  slice_picture_id = %d\n", slice_picture_id);
    }

  return slice_vertical_position_extension;
  }
//}}}
//{{{
static int startSlice (int MBAmax, int* MBA, int* MBAinc, int dc_dct_pred[3], int PMV[2][2][2]) {
/* return==-1 means go to next picture */
/* the expression "start of slice" is used throughout the normative body of the MPEG specification */

  unsigned int code;
  int slice_vert_pos_ext;

  ld = &base;
  Fault_Flag = 0;
  next_start_code();
  code = Show_Bits(32);

  if (code<SLICE_START_CODE_MIN || code>SLICE_START_CODE_MAX) {
    /* only slice headers are allowed in picture_data */
    if (!Quiet_Flag)
      printf ("startSlice(): Premature end of picture\n");
    return(-1);  /* trigger: go to next picture */
    }

  Flush_Buffer32();

  /* decode slice header (may change quantizer_scale) */
  slice_vert_pos_ext = slice_header();

  /* decode macroblock address increment */
  *MBAinc = Get_macroblock_address_increment();

  if (Fault_Flag) {
    printf ("startSlice(): MBAinc unsuccessful\n");
    return(0);   /* trigger: go to next slice */
    }

  /* set current location */
  /* NOTE: the arithmetic used to derive macroblock_address below is
   *       equivalent to ISO/IEC 13818-2 section 6.3.17: Macroblock
   */
  *MBA = ((slice_vert_pos_ext<<7) + (code&255) - 1)*mb_width + *MBAinc - 1;
  *MBAinc = 1; /* first macroblock in slice: not skipped */

  /* reset all DC coefficient and motion vector predictors */
  /* reset all DC coefficient and motion vector predictors */
  /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
  dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;

  /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
  PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
  PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;

  /* successfull: trigger decode macroblocks in slice */
  return(1);
  }
//}}}
//{{{
/* decode all macroblocks of the current picture */
/* ISO/IEC 13818-2 section 6.3.16 */
static int slice (int framenum, int MBAmax) {

  int motion_type;
  int dc_dct_pred[3];
  int PMV[2][2][2], motion_vertical_field_select[2][2];
  int dmvector[2];
  int stwtype, stwclass;
  int ret;

  int macroblock_type = 0;
  int dct_type = 0;
  int MBA = 0; /* macroblock address */
  int MBAinc = 0;

  if ((ret = startSlice (MBAmax, &MBA, &MBAinc, dc_dct_pred, PMV))!=1)
    return (ret);

  Fault_Flag = 0;
  for (;;) {
    /* this is how we properly exit out of picture */
    if (MBA >= MBAmax)
      return (-1); /* all macroblocks decoded */

    ld = &base;
    if (MBAinc == 0) {
      if (!Show_Bits(23) || Fault_Flag) {
        /* next_start_code or fault */
resync: /* if Fault_Flag: resynchronize to next next_start_code */
        Fault_Flag = 0;
        return(0);     /* trigger: go to next slice */
        }
      else {
        /* decode macroblock address increment */
        MBAinc = Get_macroblock_address_increment();
        if (Fault_Flag)
          goto resync;
        }
      }

    if (MBA >= MBAmax) {
      /* MBAinc points beyond picture dimensions */
      if (!Quiet_Flag)
        printf ("Too many macroblocks in picture\n");
      return (-1);
      }

    if (MBAinc == 1) {
      /* not skipped */
      ret = decode_macroblock (&macroblock_type, &stwtype, &stwclass,
                               &motion_type, &dct_type, PMV, dc_dct_pred,
                               motion_vertical_field_select, dmvector);
      if  (ret == -1)
        return (-1);
      if (ret == 0)
        goto resync;
      }
    else
      /* MBAinc!=1: skipped macroblock */
      /* ISO/IEC 13818-2 section 7.6.6 */
      skipped_macroblock (dc_dct_pred, PMV, &motion_type,
                          motion_vertical_field_select, &stwtype, &macroblock_type);


    /* ISO/IEC 13818-2 section 7.6 */
    motion_compensation (MBA, macroblock_type, motion_type, PMV,
                         motion_vertical_field_select, dmvector, stwtype, dct_type);

    /* advance to next macroblock */
    MBA++;
    MBAinc--;
    if (MBA >= MBAmax)
      return (-1); /* all macroblocks decoded */
    }
  }
//}}}
//{{{
static void picture_data (int framenum) {
/* decode all macroblocks of the current picture */
/* stages described in ISO/IEC 13818-2 section 7 */

  int ret;

  /* number of macroblocks per picture */
  int MBAmax = mb_width * mb_height;
  if (picture_structure != FRAME_PICTURE)
    MBAmax >>= 1; /* field picture has half as mnay macroblocks as frame */

  for(;;)
    if ((ret = slice (framenum, MBAmax))<0)
      return;
  }
//}}}
//{{{
/* reuse old picture buffers as soon as they are no longer needed
   based on life-time axioms of MPEG */
static void update_Picture_Buffers()
{
  int cc;              /* color component index */
  unsigned char *tmp;  /* temporary swap pointer */

  for (cc=0; cc<3; cc++)
  {
    /* B pictures do not need to be save for future reference */
    if (picture_coding_type==B_TYPE)
      current_frame[cc] = auxframe[cc];
    else
    {
      /* only update at the beginning of the coded frame */
      if (!Second_Field)
      {
        tmp = forward_reference_frame[cc];

        /* the previously decoded reference frame is stored
           coincident with the location where the backward
           reference frame is stored (backwards prediction is not
           needed in P pictures) */
        forward_reference_frame[cc] = backward_reference_frame[cc];

        /* update pointer for potential future B pictures */
        backward_reference_frame[cc] = tmp;
      }

      /* can erase over old backward reference frame since it is not used
         in a P picture, and since any subsequent B pictures will use the
         previously decoded I or P frame as the backward_reference_frame */
      current_frame[cc] = backward_reference_frame[cc];
    }

    /* IMPLEMENTATION:
       one-time folding of a line offset into the pointer which stores the
       memory address of the current frame saves offsets and conditional
       branches throughout the remainder of the picture processing loop */
    if (picture_structure==BotField)
      current_frame[cc]+= (cc==0) ? Coded_Picture_Width : Chroma_Width;
  }
}
//}}}

//{{{
unsigned int Show_Bits (int N) {
  return ld->Bfr >> (32-N);
  }
//}}}

//{{{
static void user_data() {
/* ISO/IEC 13818-2  sections 6.3.4.1 and 6.2.2.2.2 */

  /* skip ahead to the next start code */
  next_start_code();
  }
//}}}
//{{{
static void extension_and_user_data() {
/* decode extension and user data */
/* ISO/IEC 13818-2 section 6.2.2.2 */

  int code,ext_ID;
  next_start_code();

  while ((code = Show_Bits(32))==EXTENSION_START_CODE || code==USER_DATA_START_CODE) {
    if (code==EXTENSION_START_CODE) {
      Flush_Buffer32();
      ext_ID = Get_Bits(4);
      switch (ext_ID) {
        case SEQUENCE_EXTENSION_ID:
          sequence_extension();
          break;
        case SEQUENCE_DISPLAY_EXTENSION_ID:
          sequence_display_extension();
          break;
        case QUANT_MATRIX_EXTENSION_ID:
          quant_matrix_extension();
          break;
          break;
        case PICTURE_DISPLAY_EXTENSION_ID:
          picture_display_extension();
          break;
        case PICTURE_CODING_EXTENSION_ID:
          picture_coding_extension();
          break;
        case COPYRIGHT_EXTENSION_ID:
        case SEQUENCE_SCALABLE_EXTENSION_ID:
        case PICTURE_SPATIAL_SCALABLE_EXTENSION_ID:
        case PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID:
          break;
        default:
          fprintf(stderr,"reserved extension start code ID %d\n",ext_ID);
          break;
        }
      next_start_code();
      }
    else {
      if (kDebug)
        printf ("user data\n");
      Flush_Buffer32();
      user_data();
      }
    }
  }
//}}}
//{{{
static void sequence_header() {
/* decode sequence header */

  int i;

  int pos = ld->Bitcnt;
  horizontal_size             = Get_Bits(12);
  vertical_size               = Get_Bits(12);
  aspect_ratio_information    = Get_Bits(4);
  frame_rate_code             = Get_Bits(4);
  bit_rate_value              = Get_Bits(18);
  marker_bit("sequence_header()");
  vbv_buffer_size             = Get_Bits(10);
  constrained_parameters_flag = Get_Bits(1);

  if((ld->load_intra_quantizer_matrix = Get_Bits(1))) {
    for (i = 0; i < 64; i++)
      ld->intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
    }
  else {
    for (i = 0; i < 64; i++)
      ld->intra_quantizer_matrix[i] = default_intra_quantizer_matrix[i];
    }

  if((ld->load_non_intra_quantizer_matrix = Get_Bits(1))) {
    for (i = 0; i < 64; i++)
      ld->non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
    }
  else {
    for (i = 0; i < 64; i++)
      ld->non_intra_quantizer_matrix[i] = 16;
    }

  /* copy luminance to chrominance matrices */
  for (i = 0; i < 64; i++) {
    ld->chroma_intra_quantizer_matrix[i] =
      ld->intra_quantizer_matrix[i];
    ld->chroma_non_intra_quantizer_matrix[i] =
      ld->non_intra_quantizer_matrix[i];
    }

  if (kDebug) {
    printf ("sequence header (byte %d)\n",(pos>>3)-4);
    printf ("  horizontal_size=%d\n",horizontal_size);
    printf ("  vertical_size=%d\n",vertical_size);
    printf ("  aspect_ratio_information=%d\n",aspect_ratio_information);
    printf ("  frame_rate_code=%d",frame_rate_code);
    printf ("  bit_rate_value=%d\n",bit_rate_value);
    printf ("  vbv_buffer_size=%d\n",vbv_buffer_size);
    printf ("  constrained_parameters_flag=%d\n",constrained_parameters_flag);
    printf ("  load_intra_quantizer_matrix=%d\n",ld->load_intra_quantizer_matrix);
    printf ("  load_non_intra_quantizer_matrix=%d\n",ld->load_non_intra_quantizer_matrix);
    }

  extension_and_user_data();
  }
//}}}
//{{{
static void gop_header() {
/* decode group of pictures header */
/* ISO/IEC 13818-2 section 6.2.2.6 */

  int pos = ld->Bitcnt;
  drop_flag   = Get_Bits(1);
  hour        = Get_Bits(5);
  minute      = Get_Bits(6);
  marker_bit("gop_header()");
  sec         = Get_Bits(6);
  frame       = Get_Bits(6);
  closed_gop  = Get_Bits(1);
  broken_link = Get_Bits(1);

  if (kDebug) {
    printf("group of pictures (byte %d)\n",(pos>>3)-4);
    printf("  drop_flag=%d\n",drop_flag);
    printf("  timecode %d:%02d:%02d:%02d\n",hour,minute,sec,frame);
    printf("  closed_gop=%d\n",closed_gop);
    printf("  broken_link=%d\n",broken_link);
    }

  extension_and_user_data();
  }
//}}}
//{{{
static void picture_header() {
/* decode picture header */
/* ISO/IEC 13818-2 section 6.2.3 */

  int pos;
  int Extra_Information_Byte_Count;

  pos = ld->Bitcnt;
  temporal_reference  = Get_Bits(10);
  picture_coding_type = Get_Bits(3);
  vbv_delay           = Get_Bits(16);

  if (picture_coding_type==P_TYPE || picture_coding_type==B_TYPE) {
    full_pel_forward_vector = Get_Bits(1);
    forward_f_code = Get_Bits(3);
    }
  if (picture_coding_type==B_TYPE) {
    full_pel_backward_vector = Get_Bits(1);
    backward_f_code = Get_Bits(3);
    }

  if (kDebug) {
    printf ("picture header (byte %d)\n",(pos>>3)-4);
    printf ("  temporal_reference=%d\n",temporal_reference);
    printf ("  picture_coding_type=%d\n",picture_coding_type);
    printf ("  vbv_delay=%d\n",vbv_delay);
    if (picture_coding_type == P_TYPE || picture_coding_type==B_TYPE) {
      printf ("  full_pel_forward_vector=%d\n",full_pel_forward_vector);
      printf ("  forward_f_code =%d\n",forward_f_code);
      }
    if (picture_coding_type == B_TYPE) {
      printf ("  full_pel_backward_vector=%d\n",full_pel_backward_vector);
      printf ("  backward_f_code =%d\n",backward_f_code);
      }
    }

  Extra_Information_Byte_Count = extra_bit_information();
  extension_and_user_data();
  }
//}}}
//{{{
int Get_Hdr() {
// decode headers from one input stream
// until an End of Sequence or picture start code
// is found

  unsigned int code;

  for (;;) {
    /* look for next_start_code */
    next_start_code();
    code = Get_Bits32();

    switch (code) {
      case SEQUENCE_HEADER_CODE:
        sequence_header();
        break;

      case GROUP_START_CODE:
        gop_header();
        break;

      case PICTURE_START_CODE:
        picture_header();
        return 1;
        break;

      case SEQUENCE_END_CODE:
        return 0;
        break;

      default:
        if (!Quiet_Flag)
          fprintf(stderr,"Unexpected next_start_code %08x (ignored)\n",code);
        break;
      }
    }
  }
//}}}

//{{{
void Decode_Picture (int bitstream_framenum, int sequence_framenum) {
/* decode one frame or field picture */

  if (picture_structure == FRAME_PICTURE && Second_Field) {
    /* recover from illegal number of field pictures */
    printf ("odd number of field pictures\n");
    Second_Field = 0;
    }

  /* IMPLEMENTATION: update picture buffer pointers */
  update_Picture_Buffers();

  /* decode picture data ISO/IEC 13818-2 section 6.2.3.7 */
  picture_data (bitstream_framenum);

  /* write or display current or previously decoded reference frame */
  /* ISO/IEC 13818-2 section 6.1.1.11: Frame reordering */
  frame_reorder (bitstream_framenum, sequence_framenum);

  if (picture_structure != FRAME_PICTURE)
    Second_Field = !Second_Field;
  }
//}}}
//{{{
void Write_Frame (unsigned char* src[], int frame) {

  if (progressive_sequence || progressive_frame || Frame_Store_Flag) {
    /* progressive */
    //sprintf (outname,Output_Picture_Filename,frame,'f');
    //store_one (outname,src,0,Coded_Picture_Width,vertical_size);
    }
  else {
    /* interlaced */
    //sprintf (outname,Output_Picture_Filename,frame,'a');
    //store_one (outname,src,0,Coded_Picture_Width<<1,vertical_size>>1);

    //sprintf (outname,Output_Picture_Filename,frame,'b');
    //store_one (outname,src, Coded_Picture_Width,Coded_Picture_Width<<1,vertical_size>>1);
    }
  }
//}}}
//{{{
void Output_Last_Frame_of_Sequence (int Framenum) {

  if (Second_Field)
    printf ("last frame incomplete, not stored\n");
  else
    Write_Frame (backward_reference_frame, Framenum-1);
}
//}}}
