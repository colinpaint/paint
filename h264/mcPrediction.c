//{{{  includes
#include "global.h"
#include "memory.h"

#include "block.h"
#include "mcPrediction.h"
#include "mbuffer.h"
#include "mbAccess.h"
#include "macroblock.h"
//}}}

//{{{
int allocate_pred_mem (sSlice* curSlice)
{
  int alloc_size = 0;
  alloc_size += get_mem2Dpel(&curSlice->tmp_block_l0, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  alloc_size += get_mem2Dpel(&curSlice->tmp_block_l1, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  alloc_size += get_mem2Dpel(&curSlice->tmp_block_l2, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  alloc_size += get_mem2Dpel(&curSlice->tmp_block_l3, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  alloc_size += get_mem2Dint(&curSlice->tmp_res, MB_BLOCK_SIZE + 5, MB_BLOCK_SIZE + 5);
  return (alloc_size);
}
//}}}
//{{{
void free_pred_mem (sSlice* curSlice)
{
  free_mem2Dint(curSlice->tmp_res);
  free_mem2Dpel(curSlice->tmp_block_l0);
  free_mem2Dpel(curSlice->tmp_block_l1);
  free_mem2Dpel(curSlice->tmp_block_l2);
  free_mem2Dpel(curSlice->tmp_block_l3);
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    block single list prediction
** **********************************************************************
 */
static void mc_prediction (sPixel** mb_pred, sPixel** block, int block_size_y, int block_size_x, int ioff)
{

  int j;

  for (j = 0; j < block_size_y; j++)
  {
    memcpy(&mb_pred[j][ioff], block[j], block_size_x * sizeof(sPixel));
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    block single list weighted prediction
** **********************************************************************
 */
static void weighted_mc_prediction (sPixel** mb_pred,
                                   sPixel** block,
                                   int block_size_y,
                                   int block_size_x,
                                   int ioff,
                                   int wp_scale,
                                   int wp_offset,
                                   int weight_denom,
                                   int color_clip)
{
  int i, j;
  int result;

  for(j = 0; j < block_size_y; j++)
  {
    for(i = 0; i < block_size_x; i++)
    {
      result = rshift_rnd((wp_scale * block[j][i]), weight_denom) + wp_offset;
      mb_pred[j][i + ioff] = (sPixel)iClip3(0, color_clip, result);
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    block bi-prediction
** **********************************************************************
 */
static void bi_prediction (sPixel** mb_pred,
                          sPixel** block_l0,
                          sPixel** block_l1,
                          int block_size_y,
                          int block_size_x,
                          int ioff)
{
  sPixel *mpr = &mb_pred[0][ioff];
  sPixel *b0 = block_l0[0];
  sPixel *b1 = block_l1[0];
  int ii, jj;
  int row_inc = MB_BLOCK_SIZE - block_size_x;
  for(jj = 0;jj < block_size_y;jj++)
  {
    // unroll the loop
    for(ii = 0; ii < block_size_x; ii += 2)
    {
      *(mpr++) = (sPixel)(((*(b0++) + *(b1++)) + 1) >> 1);
      *(mpr++) = (sPixel)(((*(b0++) + *(b1++)) + 1) >> 1);
    }
    mpr += row_inc;
    b0  += row_inc;
    b1  += row_inc;
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    block weighted biprediction
** **********************************************************************
 */
static void weighted_bi_prediction (sPixel *mb_pred,
                                   sPixel *block_l0,
                                   sPixel *block_l1,
                                   int block_size_y,
                                   int block_size_x,
                                   int wp_scale_l0,
                                   int wp_scale_l1,
                                   int wp_offset,
                                   int weight_denom,
                                   int color_clip)
{
  int i, j, result;
  int row_inc = MB_BLOCK_SIZE - block_size_x;

  for(j = 0; j < block_size_y; j++)
  {
    for(i = 0; i < block_size_x; i++)
    {
      result = rshift_rnd_sf((wp_scale_l0 * *(block_l0++) + wp_scale_l1 * *(block_l1++)),  weight_denom);

      *(mb_pred++) = (sPixel) iClip1(color_clip, result + wp_offset);
    }
    mb_pred += row_inc;
    block_l0 += row_inc;
    block_l1 += row_inc;
  }
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Integer positions
** **********************************************************************
 */
static void get_block_00 (sPixel *block, sPixel* curPixel, int span, int block_size_y)
{
  // fastest to just move an entire block, since block is a temp block is a 256 byte block (16x16)
  // writes 2 lines of 16 sPixel 1 to 8 times depending in block_size_y
  int j;

  for (j = 0; j < block_size_y; j += 2)
  {
    memcpy(block, curPixel, MB_BLOCK_SIZE * sizeof(sPixel));
    block += MB_BLOCK_SIZE;
    curPixel += span;
    memcpy(block, curPixel, MB_BLOCK_SIZE * sizeof(sPixel));
    block += MB_BLOCK_SIZE;
    curPixel += span;
  }
}

//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel (1,0) horizontal
** **********************************************************************
 */
static void get_luma_10 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos , int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line, *cur_line;
  int i, j;
  int result;

  for (j = 0; j < block_size_y; j++)
  {
    cur_line = &(curPixelY[j][x_pos]);
    p0 = &curPixelY[j][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
      *orig_line = (sPixel) ((*orig_line + *(cur_line++) + 1 ) >> 1);
      orig_line++;
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Half horizontal
** **********************************************************************
 */
static void get_luma_20 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos , int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int i, j;
  int result;
  for (j = 0; j < block_size_y; j++)
  {
    p0 = &curPixelY[j][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line++ = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel (3,0) horizontal
** **********************************************************************
 */
static void get_luma_30 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos , int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line, *cur_line;
  int i, j;
  int result;

  for (j = 0; j < block_size_y; j++)
  {
    cur_line = &(curPixelY[j][x_pos + 1]);
    p0 = &curPixelY[j][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
      *orig_line = (sPixel) ((*orig_line + *(cur_line++) + 1 ) >> 1);
      orig_line++;
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel vertical (0, 1)
** **********************************************************************
 */
static void get_luma_01 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos, int shift_x, int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line, *cur_line;
  int i, j;
  int result;
  int jj = 0;
  p0 = &(curPixelY[ - 2][x_pos]);
  for (j = 0; j < block_size_y; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];
    cur_line = &(curPixelY[jj++][x_pos]);

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
      *orig_line = (sPixel) ((*orig_line + *(cur_line++) + 1 ) >> 1);
      orig_line++;
    }
    p0 = p1 - block_size_x;
  }
}

//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Half vertical
** **********************************************************************
 */
static void get_luma_02 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos, int shift_x, int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int i, j;
  int result;
  p0 = &(curPixelY[ - 2][x_pos]);
  for (j = 0; j < block_size_y; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line++ = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
    p0 = p1 - block_size_x;
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel vertical (0, 3)
** **********************************************************************
 */
static void get_luma_03 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos, int shift_x, int max_imgpel_value)
{
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line, *cur_line;
  int i, j;
  int result;
  int jj = 1;

  p0 = &(curPixelY[ -2][x_pos]);
  for (j = 0; j < block_size_y; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];
    cur_line = &(curPixelY[jj++][x_pos]);

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
      *orig_line = (sPixel) ((*orig_line + *(cur_line++) + 1 ) >> 1);
      orig_line++;
    }
    p0 = p1 - block_size_x;
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Hpel horizontal, Qpel vertical (2, 1)
** **********************************************************************
 */
static void get_luma_21 (sPixel** block, sPixel** curPixelY, int** tmp_res, int block_size_y, int block_size_x, int x_pos, int max_imgpel_value)
{
  int i, j;
  /* Vertical & horizontal interpolation */
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  int jj = -2;

  for (j = 0; j < block_size_y + 5; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    tmp_line  = tmp_res[j];

    for (i = 0; i < block_size_x; i++)
    {
      *(tmp_line++) = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
  }

  jj = 2;
  for (j = 0; j < block_size_y; j++)
  {
    tmp_line  = tmp_res[jj++];
    x0 = tmp_res[j    ];
    x1 = tmp_res[j + 1];
    x2 = tmp_res[j + 2];
    x3 = tmp_res[j + 3];
    x4 = tmp_res[j + 4];
    x5 = tmp_res[j + 5];
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*x0++ + *x5++) - 5 * (*x1++ + *x4++) + 20 * (*x2++ + *x3++);

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((*(tmp_line++) + 16) >> 5)) + 1 )>> 1);
      orig_line++;
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Hpel horizontal, Hpel vertical (2, 2)
** **********************************************************************
 */
static void get_luma_22 (sPixel** block, sPixel** curPixelY, int** tmp_res, int block_size_y, int block_size_x, int x_pos, int max_imgpel_value)
{
  int i, j;
  /* Vertical & horizontal interpolation */
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  int jj = - 2;

  for (j = 0; j < block_size_y + 5; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    tmp_line  = tmp_res[j];

    for (i = 0; i < block_size_x; i++)
    {
      *(tmp_line++) = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
  }

  for (j = 0; j < block_size_y; j++)
  {
    x0 = tmp_res[j    ];
    x1 = tmp_res[j + 1];
    x2 = tmp_res[j + 2];
    x3 = tmp_res[j + 3];
    x4 = tmp_res[j + 4];
    x5 = tmp_res[j + 5];
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*x0++ + *x5++) - 5 * (*x1++ + *x4++) + 20 * (*x2++ + *x3++);

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Hpel horizontal, Qpel vertical (2, 3)
** **********************************************************************
 */
static void get_luma_23 (sPixel** block, sPixel** curPixelY, int** tmp_res, int block_size_y, int block_size_x, int x_pos, int max_imgpel_value)
{
  int i, j;
  /* Vertical & horizontal interpolation */
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  int jj = -2;

  for (j = 0; j < block_size_y + 5; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;
    tmp_line  = tmp_res[j];

    for (i = 0; i < block_size_x; i++)
    {
      *(tmp_line++) = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
  }

  jj = 3;
  for (j = 0; j < block_size_y; j++)
  {
    tmp_line  = tmp_res[jj++];
    x0 = tmp_res[j    ];
    x1 = tmp_res[j + 1];
    x2 = tmp_res[j + 2];
    x3 = tmp_res[j + 3];
    x4 = tmp_res[j + 4];
    x5 = tmp_res[j + 5];
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*x0++ + *x5++) - 5 * (*x1++ + *x4++) + 20 * (*x2++ + *x3++);

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((*(tmp_line++) + 16) >> 5)) + 1 )>> 1);
      orig_line++;
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel horizontal, Hpel vertical (1, 2)
** **********************************************************************
 */
static void get_luma_12 (sPixel** block, sPixel** curPixelY, int** tmp_res, int block_size_y, int block_size_x, int x_pos, int shift_x, int max_imgpel_value)
{
  int i, j;
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  p0 = &(curPixelY[ -2][x_pos - 2]);
  for (j = 0; j < block_size_y; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    tmp_line  = tmp_res[j];

    for (i = 0; i < block_size_x + 5; i++)
    {
      *(tmp_line++)  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
    p0 = p1 - (block_size_x + 5);
  }

  for (j = 0; j < block_size_y; j++)
  {
    tmp_line  = &tmp_res[j][2];
    orig_line = block[j];
    x0 = tmp_res[j];
    x1 = x0 + 1;
    x2 = x1 + 1;
    x3 = x2 + 1;
    x4 = x3 + 1;
    x5 = x4 + 1;

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(x0++) + *(x5++)) - 5 * (*(x1++) + *(x4++)) + 20 * (*(x2++) + *(x3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((*(tmp_line++) + 16)>>5))+1)>>1);
      orig_line ++;
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel horizontal, Hpel vertical (3, 2)
** **********************************************************************
 */
static void get_luma_32 (sPixel** block, sPixel** curPixelY, int** tmp_res, int block_size_y, int block_size_x, int x_pos, int shift_x, int max_imgpel_value)
{
  int i, j;
  int *tmp_line;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  int    *x0, *x1, *x2, *x3, *x4, *x5;
  sPixel *orig_line;
  int result;

  p0 = &(curPixelY[ -2][x_pos - 2]);
  for (j = 0; j < block_size_y; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    tmp_line  = tmp_res[j];

    for (i = 0; i < block_size_x + 5; i++)
    {
      *(tmp_line++)  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));
    }
    p0 = p1 - (block_size_x + 5);
  }

  for (j = 0; j < block_size_y; j++)
  {
    tmp_line  = &tmp_res[j][3];
    orig_line = block[j];
    x0 = tmp_res[j];
    x1 = x0 + 1;
    x2 = x1 + 1;
    x3 = x2 + 1;
    x4 = x3 + 1;
    x5 = x4 + 1;

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(x0++) + *(x5++)) - 5 * (*(x1++) + *(x4++)) + 20 * (*(x2++) + *(x3++));

      *orig_line = (sPixel) iClip1(max_imgpel_value, ((result + 512)>>10));
      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((*(tmp_line++) + 16)>>5))+1)>>1);
      orig_line ++;
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel horizontal, Qpel vertical (3, 3)
** **********************************************************************
 */
static void get_luma_33 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos, int shift_x, int max_imgpel_value)
{
  int i, j;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int result;

  int jj = 1;

  for (j = 0; j < block_size_y; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;

    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }

  p0 = &(curPixelY[-2][x_pos + 1]);
  for (j = 0; j < block_size_y; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((result + 16) >> 5)) + 1) >> 1);
      orig_line++;
    }
    p0 = p1 - block_size_x ;
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel horizontal, Qpel vertical (1, 1)
** **********************************************************************
 */
static void get_luma_11 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos, int shift_x, int max_imgpel_value)
{
  int i, j;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int result;

  int jj = 0;

  for (j = 0; j < block_size_y; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;

    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }

  p0 = &(curPixelY[-2][x_pos]);
  for (j = 0; j < block_size_y; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((result + 16) >> 5)) + 1) >> 1);
      orig_line++;
    }
    p0 = p1 - block_size_x ;
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel horizontal, Qpel vertical (1, 3)
** **********************************************************************
 */
static void get_luma_13 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos, int shift_x, int max_imgpel_value)
{
  /* Diagonal interpolation */
  int i, j;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int result;

  int jj = 1;

  for (j = 0; j < block_size_y; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;

    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }

  p0 = &(curPixelY[-2][x_pos]);
  for (j = 0; j < block_size_y; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((result + 16) >> 5)) + 1) >> 1);
      orig_line++;
    }
    p0 = p1 - block_size_x ;
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Qpel horizontal, Qpel vertical (3, 1)
** **********************************************************************
 */
static void get_luma_31 (sPixel** block, sPixel** curPixelY, int block_size_y, int block_size_x, int x_pos, int shift_x, int max_imgpel_value)
{
  /* Diagonal interpolation */
  int i, j;
  sPixel *p0, *p1, *p2, *p3, *p4, *p5;
  sPixel *orig_line;
  int result;

  int jj = 0;

  for (j = 0; j < block_size_y; j++)
  {
    p0 = &curPixelY[jj++][x_pos - 2];
    p1 = p0 + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;
    p4 = p3 + 1;
    p5 = p4 + 1;

    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *(orig_line++) = (sPixel) iClip1(max_imgpel_value, ((result + 16)>>5));
    }
  }

  p0 = &(curPixelY[-2][x_pos + 1]);
  for (j = 0; j < block_size_y; j++)
  {
    p1 = p0 + shift_x;
    p2 = p1 + shift_x;
    p3 = p2 + shift_x;
    p4 = p3 + shift_x;
    p5 = p4 + shift_x;
    orig_line = block[j];

    for (i = 0; i < block_size_x; i++)
    {
      result  = (*(p0++) + *(p5++)) - 5 * (*(p1++) + *(p4++)) + 20 * (*(p2++) + *(p3++));

      *orig_line = (sPixel) ((*orig_line + iClip1(max_imgpel_value, ((result + 16) >> 5)) + 1) >> 1);
      orig_line++;
    }
    p0 = p1 - block_size_x ;
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Interpolation of 1/4 subpixel
** **********************************************************************
 */
void get_block_luma (sPicture* curRef, int x_pos, int y_pos, int block_size_x, int block_size_y, sPixel** block,
                    int shift_x, int maxold_x, int maxold_y, int** tmp_res, int max_imgpel_value, sPixel no_ref_value, sMacroblock* curMb)
{
  if (curRef->no_ref) {
    //printf("list[ref_frame] is equal to 'no reference picture' before RAP\n");
    memset(block[0],no_ref_value,block_size_y * block_size_x * sizeof(sPixel));
  }
  else
  {
    sPixel** curPixelY = (curMb->vidParam->separate_colour_plane_flag && curMb->slice->colour_plane_id>PLANE_Y)? curRef->imgUV[curMb->slice->colour_plane_id-1] : curRef->curPixelY;
    int dx = (x_pos & 3);
    int dy = (y_pos & 3);
    x_pos >>= 2;
    y_pos >>= 2;
    x_pos = iClip3(-18, maxold_x+2, x_pos);
    y_pos = iClip3(-10, maxold_y+2, y_pos);

    if (dx == 0 && dy == 0)
      get_block_00(&block[0][0], &curPixelY[y_pos][x_pos], curRef->iLumaStride, block_size_y);
    else
    { /* other positions */
      if (dy == 0) /* No vertical interpolation */
      {
        if (dx == 1)
          get_luma_10(block, &curPixelY[ y_pos], block_size_y, block_size_x, x_pos, max_imgpel_value);
        else if (dx == 2)
          get_luma_20(block, &curPixelY[ y_pos], block_size_y, block_size_x, x_pos, max_imgpel_value);
        else
          get_luma_30(block, &curPixelY[ y_pos], block_size_y, block_size_x, x_pos, max_imgpel_value);
      }
      else if (dx == 0) /* No horizontal interpolation */
      {
        if (dy == 1)
          get_luma_01(block, &curPixelY[y_pos], block_size_y, block_size_x, x_pos, shift_x, max_imgpel_value);
        else if (dy == 2)
          get_luma_02(block, &curPixelY[ y_pos], block_size_y, block_size_x, x_pos, shift_x, max_imgpel_value);
        else
          get_luma_03(block, &curPixelY[ y_pos], block_size_y, block_size_x, x_pos, shift_x, max_imgpel_value);
      }
      else if (dx == 2)  /* Vertical & horizontal interpolation */
      {
        if (dy == 1)
          get_luma_21(block, &curPixelY[ y_pos], tmp_res, block_size_y, block_size_x, x_pos, max_imgpel_value);
        else if (dy == 2)
          get_luma_22(block, &curPixelY[ y_pos], tmp_res, block_size_y, block_size_x, x_pos, max_imgpel_value);
        else
          get_luma_23(block, &curPixelY[ y_pos], tmp_res, block_size_y, block_size_x, x_pos, max_imgpel_value);
      }
      else if (dy == 2)
      {
        if (dx == 1)
          get_luma_12(block, &curPixelY[ y_pos], tmp_res, block_size_y, block_size_x, x_pos, shift_x, max_imgpel_value);
        else
          get_luma_32(block, &curPixelY[ y_pos], tmp_res, block_size_y, block_size_x, x_pos, shift_x, max_imgpel_value);
      }
      else
      {
        if (dx == 1)
        {
          if (dy == 1)
            get_luma_11(block, &curPixelY[ y_pos], block_size_y, block_size_x, x_pos, shift_x, max_imgpel_value);
          else
            get_luma_13(block, &curPixelY[ y_pos], block_size_y, block_size_x, x_pos, shift_x, max_imgpel_value);
        }
        else
        {
          if (dy == 1)
            get_luma_31(block, &curPixelY[ y_pos], block_size_y, block_size_x, x_pos, shift_x, max_imgpel_value);
          else
            get_luma_33(block, &curPixelY[ y_pos], block_size_y, block_size_x, x_pos, shift_x, max_imgpel_value);
        }
      }
    }
  }
}
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *    Chroma (0,X)
** **********************************************************************
 */
static void get_chroma_0X (sPixel* block, sPixel* curPixel, int span, int block_size_y, int block_size_x, int w00, int w01, int total_scale)
{
  sPixel *cur_row = curPixel;
  sPixel *nxt_row = curPixel + span;


  sPixel *cur_line, *cur_line_p1;
  sPixel *blk_line;
  int result;
  int i, j;
  for (j = 0; j < block_size_y; j++)
  {
      cur_line    = cur_row;
      cur_line_p1 = nxt_row;
      blk_line = block;
      block += 16;
      cur_row = nxt_row;
      nxt_row += span;
    for (i = 0; i < block_size_x; i++)
    {
      result = (w00 * *cur_line++ + w01 * *cur_line_p1++);
      *(blk_line++) = (sPixel) rshift_rnd_sf(result, total_scale);
    }
  }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Chroma (X,0)
** **********************************************************************
 */
static void get_chroma_X0 (sPixel* block, sPixel* curPixel, int span, int block_size_y, int block_size_x, int w00, int w10, int total_scale)
{
  sPixel *cur_row = curPixel;


    sPixel *cur_line, *cur_line_p1;
    sPixel *blk_line;
    int result;
    int i, j;
    for (j = 0; j < block_size_y; j++)
    {
      cur_line    = cur_row;
      cur_line_p1 = cur_line + 1;
      blk_line = block;
      block += 16;
      cur_row += span;
      for (i = 0; i < block_size_x; i++)
      {
        result = (w00 * *cur_line++ + w10 * *cur_line_p1++);
        //*(blk_line++) = (sPixel) iClip1(max_imgpel_value, rshift_rnd_sf(result, total_scale));
        *(blk_line++) = (sPixel) rshift_rnd_sf(result, total_scale);
      }
    }
}
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *    Chroma (X,X)
** **********************************************************************
 */
static void get_chroma_XY (sPixel* block, sPixel* curPixel, int span, int block_size_y, int block_size_x, int w00, int w01, int w10, int w11, int total_scale)
{
  sPixel *cur_row = curPixel;
  sPixel *nxt_row = curPixel + span;


  {
    sPixel *cur_line, *cur_line_p1;
    sPixel *blk_line;
    int result;
    int i, j;
    for (j = 0; j < block_size_y; j++)
    {
      cur_line    = cur_row;
      cur_line_p1 = nxt_row;
      blk_line = block;
      block += 16;
      cur_row = nxt_row;
      nxt_row += span;
      for (i = 0; i < block_size_x; i++)
      {
        result  = (w00 * *(cur_line++) + w01 * *(cur_line_p1++));
        result += (w10 * *(cur_line  ) + w11 * *(cur_line_p1  ));
        *(blk_line++) = (sPixel) rshift_rnd_sf(result, total_scale);
      }
    }
  }
}
//}}}
//{{{
static void get_block_chroma (sPicture* curRef, int x_pos, int y_pos, int subpel_x, int subpel_y, int maxold_x, int maxold_y,
                             int block_size_x, int vert_block_size, int shiftpel_x, int shiftpel_y,
                             sPixel *block1, sPixel *block2, int total_scale, sPixel no_ref_value, sVidParam* vidParam)
{
  sPixel *img1,*img2;
  short dx,dy;
  int span = curRef->iChromaStride;
  if (curRef->no_ref) {
    //printf("list[ref_frame] is equal to 'no reference picture' before RAP\n");
    memset(block1,no_ref_value,vert_block_size * block_size_x * sizeof(sPixel));
    memset(block2,no_ref_value,vert_block_size * block_size_x * sizeof(sPixel));
  }
  else
  {
    dx = (short) (x_pos & subpel_x);
    dy = (short) (y_pos & subpel_y);
    x_pos = x_pos >> shiftpel_x;
    y_pos = y_pos >> shiftpel_y;
    //clip MV;
    assert(vert_block_size <=vidParam->iChromaPadY && block_size_x<=vidParam->iChromaPadX);
    x_pos = iClip3(-vidParam->iChromaPadX, maxold_x, x_pos); //16
    y_pos = iClip3(-vidParam->iChromaPadY, maxold_y, y_pos); //8
    img1 = &curRef->imgUV[0][y_pos][x_pos];
    img2 = &curRef->imgUV[1][y_pos][x_pos];

    if (dx == 0 && dy == 0)
    {
      get_block_00(block1, img1, span, vert_block_size);
      get_block_00(block2, img2, span, vert_block_size);
    }
    else
    {
      short dxcur = (short) (subpel_x + 1 - dx);
      short dycur = (short) (subpel_y + 1 - dy);
      short w00 = dxcur * dycur;
      if (dx == 0)
      {
        short w01 = dxcur * dy;
        get_chroma_0X(block1, img1, span, vert_block_size, block_size_x, w00, w01, total_scale);
        get_chroma_0X(block2, img2, span, vert_block_size, block_size_x, w00, w01, total_scale);
      }
      else if (dy == 0)
      {
        short w10 = dx * dycur;
        get_chroma_X0(block1, img1, span, vert_block_size, block_size_x, w00, w10, total_scale);
        get_chroma_X0(block2, img2, span, vert_block_size, block_size_x, w00, w10, total_scale);
      }
      else
      {
        short w01 = dxcur * dy;
        short w10 = dx * dycur;
        short w11 = dx * dy;
        get_chroma_XY(block1, img1, span, vert_block_size, block_size_x, w00, w01, w10, w11, total_scale);
        get_chroma_XY(block2, img2, span, vert_block_size, block_size_x, w00, w01, w10, w11, total_scale);
      }
    }
  }
}
//}}}
//{{{
void intra_cr_decoding (sMacroblock* curMb, int yuv)
{
  sVidParam* vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;
  sPicture* picture = curSlice->picture;
  sPixel** curUV;
  int uv;
  int b8,b4;
  int ioff, joff;
  int i,j;

  curSlice->intra_pred_chroma(curMb);// last argument is ignored, computes needed data for both uv channels

  for(uv = 0; uv < 2; uv++)
  {
    curMb->itrans_4x4 = (curMb->is_lossless == FALSE) ? itrans4x4 : itrans4x4_ls;

    curUV = picture->imgUV[uv];

    if(curMb->is_lossless)
    {
      if ((curMb->c_ipred_mode == VERT_PRED_8)||(curMb->c_ipred_mode == HOR_PRED_8))
        Inv_Residual_trans_Chroma(curMb, uv) ;
      else
      {
        for(j=0;j<vidParam->mb_cr_size_y;j++)
          for(i=0;i<vidParam->mb_cr_size_x;i++)
            curSlice->mb_rres [uv+1][j][i]=curSlice->cof[uv+1][j][i];
      }
    }

    if ((!(curMb->mb_type == SI4MB) && (curMb->cbp >> 4)) )
    {
      for (b8 = 0; b8 < (vidParam->num_uv_blocks); b8++)
      {
        for(b4 = 0; b4 < 4; b4++)
        {
          joff = subblk_offset_y[yuv][b8][b4];
          ioff = subblk_offset_x[yuv][b8][b4];

          curMb->itrans_4x4(curMb, (sColorPlane) (uv + 1), ioff, joff);

          copy_Image_4x4(&curUV[curMb->pix_c_y + joff], &(curSlice->mb_rec[uv + 1][joff]), curMb->pix_c_x + ioff, ioff);
        }
      }
      curSlice->is_reset_coeff_cr = FALSE;
    }
    else if (curMb->mb_type == SI4MB)
    {
      itrans_sp_cr(curMb, uv);

      for (joff  = 0; joff < 8; joff += 4)
      {
        for(ioff = 0; ioff < 8;ioff+=4)
        {
          curMb->itrans_4x4(curMb, (sColorPlane) (uv + 1), ioff, joff);

          copy_Image_4x4(&curUV[curMb->pix_c_y + joff], &(curSlice->mb_rec[uv + 1][joff]), curMb->pix_c_x + ioff, ioff);
        }
      }
      curSlice->is_reset_coeff_cr = FALSE;
    }
    else
    {
      for (b8 = 0; b8 < (vidParam->num_uv_blocks); b8++)
      {
        for(b4 = 0; b4 < 4; b4++)
        {
          joff = subblk_offset_y[yuv][b8][b4];
          ioff = subblk_offset_x[yuv][b8][b4];

          copy_Image_4x4(&curUV[curMb->pix_c_y + joff], &(curSlice->mb_pred[uv + 1][joff]), curMb->pix_c_x + ioff, ioff);
        }
      }
    }
  }
}
//}}}

//{{{
static inline void set_direct_references (const sPixelPos* mb, char* l0_rFrame, char* l1_rFrame, sPicMotionParam** mv_info)
{
  if (mb->available)
  {
    char *ref_idx = mv_info[mb->pos_y][mb->pos_x].ref_idx;
    *l0_rFrame  = ref_idx[LIST_0];
    *l1_rFrame  = ref_idx[LIST_1];
  }
  else
  {
    *l0_rFrame  = -1;
    *l1_rFrame  = -1;
  }
}
//}}}
//{{{
static void set_direct_references_mb_field (const sPixelPos* mb, char* l0_rFrame, char* l1_rFrame, sPicMotionParam** mv_info, sMacroblock *mb_data)
{
  if (mb->available)
  {
    char *ref_idx = mv_info[mb->pos_y][mb->pos_x].ref_idx;
    if (mb_data[mb->mb_addr].mb_field)
    {
      *l0_rFrame  = ref_idx[LIST_0];
      *l1_rFrame  = ref_idx[LIST_1];
    }
    else
    {
      *l0_rFrame  = (ref_idx[LIST_0] < 0) ? ref_idx[LIST_0] : ref_idx[LIST_0] * 2;
      *l1_rFrame  = (ref_idx[LIST_1] < 0) ? ref_idx[LIST_1] : ref_idx[LIST_1] * 2;
    }
  }
  else
  {
    *l0_rFrame  = -1;
    *l1_rFrame  = -1;
  }
}
//}}}
//{{{
static void set_direct_references_mb_frame (const sPixelPos* mb, char* l0_rFrame, char* l1_rFrame, sPicMotionParam** mv_info, sMacroblock *mb_data)
{
  if (mb->available)
  {
    char *ref_idx = mv_info[mb->pos_y][mb->pos_x].ref_idx;
    if (mb_data[mb->mb_addr].mb_field)
    {
      *l0_rFrame  = (ref_idx[LIST_0] >> 1);
      *l1_rFrame  = (ref_idx[LIST_1] >> 1);
    }
    else
    {
      *l0_rFrame  = ref_idx[LIST_0];
      *l1_rFrame  = ref_idx[LIST_1];
    }
  }
  else
  {
    *l0_rFrame  = -1;
    *l1_rFrame  = -1;
  }
}
//}}}
//{{{
void prepare_direct_params (sMacroblock* curMb, sPicture* picture, sMotionVector* pmvl0, sMotionVector *pmvl1, char *l0_rFrame, char *l1_rFrame)
{
  sSlice* curSlice = curMb->slice;
  char l0_refA, l0_refB, l0_refC;
  char l1_refA, l1_refB, l1_refC;
  sPicMotionParam** mv_info = picture->mv_info;

  sPixelPos mb[4];

  get_neighbors(curMb, mb, 0, 0, 16);

  if (!curSlice->mb_aff_frame_flag)
  {
    set_direct_references(&mb[0], &l0_refA, &l1_refA, mv_info);
    set_direct_references(&mb[1], &l0_refB, &l1_refB, mv_info);
    set_direct_references(&mb[2], &l0_refC, &l1_refC, mv_info);
  }
  else
  {
    sVidParam* vidParam = curMb->vidParam;
    if (curMb->mb_field)
    {
      set_direct_references_mb_field(&mb[0], &l0_refA, &l1_refA, mv_info, vidParam->mb_data);
      set_direct_references_mb_field(&mb[1], &l0_refB, &l1_refB, mv_info, vidParam->mb_data);
      set_direct_references_mb_field(&mb[2], &l0_refC, &l1_refC, mv_info, vidParam->mb_data);
    }
    else
    {
      set_direct_references_mb_frame(&mb[0], &l0_refA, &l1_refA, mv_info, vidParam->mb_data);
      set_direct_references_mb_frame(&mb[1], &l0_refB, &l1_refB, mv_info, vidParam->mb_data);
      set_direct_references_mb_frame(&mb[2], &l0_refC, &l1_refC, mv_info, vidParam->mb_data);
    }
  }

  *l0_rFrame = (char) imin(imin((unsigned char) l0_refA, (unsigned char) l0_refB), (unsigned char) l0_refC);
  *l1_rFrame = (char) imin(imin((unsigned char) l1_refA, (unsigned char) l1_refB), (unsigned char) l1_refC);

  if (*l0_rFrame >=0)
    curMb->GetMVPredictor (curMb, mb, pmvl0, *l0_rFrame, mv_info, LIST_0, 0, 0, 16, 16);

  if (*l1_rFrame >=0)
    curMb->GetMVPredictor (curMb, mb, pmvl1, *l1_rFrame, mv_info, LIST_1, 0, 0, 16, 16);
}
//}}}

//{{{
static void check_motion_vector_range (const sMotionVector *mv, sSlice *pSlice)
{
  if (mv->mv_x > 8191 || mv->mv_x < -8192)
  {
    fprintf(stderr,"WARNING! Horizontal motion vector %d is out of allowed range {-8192, 8191} in picture %d, macroblock %d\n", mv->mv_x, pSlice->vidParam->number, pSlice->current_mb_nr);
    //error("invalid stream: too big horizontal motion vector", 500);
  }

  if (mv->mv_y > (pSlice->max_mb_vmv_r - 1) || mv->mv_y < (-pSlice->max_mb_vmv_r))
  {
    fprintf(stderr,"WARNING! Vertical motion vector %d is out of allowed range {%d, %d} in picture %d, macroblock %d\n", mv->mv_y, (-pSlice->max_mb_vmv_r), (pSlice->max_mb_vmv_r - 1), pSlice->vidParam->number, pSlice->current_mb_nr);
    //error("invalid stream: too big vertical motion vector", 500);
  }
}
//}}}
//{{{
static inline int check_vert_mv (int llimit, int vec1_y,int rlimit)
{
  int y_pos = vec1_y >> 2;
  if(y_pos < llimit || y_pos > rlimit)
    return 1;
  else
    return 0;
}
//}}}
//{{{
static void perform_mc_single_wp (sMacroblock* curMb, sColorPlane pl, sPicture* picture, int pred_dir, int i, int j, int block_size_x, int block_size_y)
{
  sVidParam* vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;
  sSPS *activeSPS = curSlice->activeSPS;
  sPixel** tmp_block_l0 = curSlice->tmp_block_l0;
  sPixel** tmp_block_l1 = curSlice->tmp_block_l1;
  static const int mv_mul = 16; // 4 * 4
  int i4   = curMb->block_x + i;
  int j4   = curMb->block_y + j;
  int type = curSlice->slice_type;
  int chroma_format_idc = picture->chroma_format_idc;
  //===== Single List Prediction =====
  int ioff = (i << 2);
  int joff = (j << 2);
  sPicMotionParam *mv_info = &picture->mv_info[j4][i4];
  short       ref_idx = mv_info->ref_idx[pred_dir];
  short       ref_idx_wp = ref_idx;
  sMotionVector *mv_array = &mv_info->mv[pred_dir];
  int list_offset = curMb->list_offset;
  sPicture *list = curSlice->listX[list_offset + pred_dir][ref_idx];
  int vec1_x, vec1_y;
  // vars for get_block_luma
  int maxold_x = picture->size_x_m1;
  int maxold_y = (curMb->mb_field) ? (picture->size_y >> 1) - 1 : picture->size_y_m1;
  int shift_x  = picture->iLumaStride;
  int** tmp_res = curSlice->tmp_res;
  int max_imgpel_value = vidParam->max_pel_value_comp[pl];
  sPixel no_ref_value = (sPixel) vidParam->dc_pred_value_comp[pl];
  //

  check_motion_vector_range(mv_array, curSlice);
  vec1_x = i4 * mv_mul + mv_array->mv_x;
  vec1_y = (curMb->block_y_aff + j) * mv_mul + mv_array->mv_y;
  if(block_size_y > (vidParam->iLumaPadY-4) && CheckVertMV(curMb, vec1_y, block_size_y))
  {
    get_block_luma(list, vec1_x, vec1_y, block_size_x, BLOCK_SIZE_8x8, tmp_block_l0, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
    get_block_luma(list, vec1_x, vec1_y+BLOCK_SIZE_8x8_SP, block_size_x, block_size_y-BLOCK_SIZE_8x8, tmp_block_l0+BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
  }
  else
    get_block_luma(list, vec1_x, vec1_y, block_size_x, block_size_y, tmp_block_l0,shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);


  {
    int alpha_l0, wp_offset, wp_denom;
    if (curMb->mb_field && ((vidParam->activePPS->weighted_pred_flag&&(type==P_SLICE|| type == SP_SLICE))||(vidParam->activePPS->weighted_bipred_idc==1 && (type==B_SLICE))))
      ref_idx_wp >>=1;
    alpha_l0  = curSlice->wp_weight[pred_dir][ref_idx_wp][pl];
    wp_offset = curSlice->wp_offset[pred_dir][ref_idx_wp][pl];
    wp_denom  = pl > 0 ? curSlice->chroma_log2_weight_denom : curSlice->luma_log2_weight_denom;
    weighted_mc_prediction(&curSlice->mb_pred[pl][joff], tmp_block_l0, block_size_y, block_size_x, ioff, alpha_l0, wp_offset, wp_denom, max_imgpel_value);
  }

  if ((chroma_format_idc != YUV400) && (chroma_format_idc != YUV444) )
  {
    int ioff_cr,joff_cr,block_size_x_cr,block_size_y_cr;
    int vec1_y_cr = vec1_y + ((activeSPS->chroma_format_idc == 1)? curSlice->chroma_vector_adjustment[list_offset + pred_dir][ref_idx] : 0);
    int total_scale = vidParam->total_scale;
    int maxold_x = picture->size_x_cr_m1;
    int maxold_y = (curMb->mb_field) ? (picture->size_y_cr >> 1) - 1 : picture->size_y_cr_m1;
    int chroma_log2_weight = curSlice->chroma_log2_weight_denom;
    if (vidParam->mb_cr_size_x == MB_BLOCK_SIZE)
    {
      ioff_cr = ioff;
      block_size_x_cr = block_size_x;
    }
    else
    {
      ioff_cr = ioff >> 1;
      block_size_x_cr = block_size_x >> 1;
    }
    if (vidParam->mb_cr_size_y == MB_BLOCK_SIZE)
    {
      joff_cr = joff;
      block_size_y_cr = block_size_y;
    }
    else
    {
      joff_cr = joff >> 1;
      block_size_y_cr = block_size_y >> 1;
    }
    no_ref_value = (sPixel)vidParam->dc_pred_value_comp[1];
    {
      int *weight = curSlice->wp_weight[pred_dir][ref_idx_wp];
      int *offset = curSlice->wp_offset[pred_dir][ref_idx_wp];
      get_block_chroma(list,vec1_x,vec1_y_cr,vidParam->subpel_x,vidParam->subpel_y,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,vidParam->shiftpel_x,vidParam->shiftpel_y,&tmp_block_l0[0][0],&tmp_block_l1[0][0] ,total_scale,no_ref_value,vidParam);
      weighted_mc_prediction(&curSlice->mb_pred[1][joff_cr], tmp_block_l0, block_size_y_cr, block_size_x_cr, ioff_cr, weight[1], offset[1], chroma_log2_weight, vidParam->max_pel_value_comp[1]);
      weighted_mc_prediction(&curSlice->mb_pred[2][joff_cr], tmp_block_l1, block_size_y_cr, block_size_x_cr, ioff_cr, weight[2], offset[2], chroma_log2_weight, vidParam->max_pel_value_comp[2]);
    }
  }
}
//}}}
//{{{
static void perform_mc_single (sMacroblock* curMb, sColorPlane pl, sPicture* picture, int pred_dir, int i, int j, int block_size_x, int block_size_y)
{
  sVidParam* vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;
  sSPS *activeSPS = curSlice->activeSPS;
  sPixel** tmp_block_l0 = curSlice->tmp_block_l0;
  sPixel** tmp_block_l1 = curSlice->tmp_block_l1;
  static const int mv_mul = 16; // 4 * 4
  int i4   = curMb->block_x + i;
  int j4   = curMb->block_y + j;
  int chroma_format_idc = picture->chroma_format_idc;
  //===== Single List Prediction =====
  int ioff = (i << 2);
  int joff = (j << 2);
  sPicMotionParam *mv_info = &picture->mv_info[j4][i4];
  sMotionVector *mv_array = &mv_info->mv[pred_dir];
  short          ref_idx =  mv_info->ref_idx[pred_dir];
  int list_offset = curMb->list_offset;
  sPicture *list = curSlice->listX[list_offset + pred_dir][ref_idx];
  int vec1_x, vec1_y;
  // vars for get_block_luma
  int maxold_x = picture->size_x_m1;
  int maxold_y = (curMb->mb_field) ? (picture->size_y >> 1) - 1 : picture->size_y_m1;
  int shift_x  = picture->iLumaStride;
  int** tmp_res = curSlice->tmp_res;
  int max_imgpel_value = vidParam->max_pel_value_comp[pl];
  sPixel no_ref_value = (sPixel) vidParam->dc_pred_value_comp[pl];
  //

  //if (iabs(mv_array->mv_x) > 4 * 126 || iabs(mv_array->mv_y) > 4 * 126)
    //printf("motion vector %d %d\n", mv_array->mv_x, mv_array->mv_y);

  check_motion_vector_range(mv_array, curSlice);
  vec1_x = i4 * mv_mul + mv_array->mv_x;
  vec1_y = (curMb->block_y_aff + j) * mv_mul + mv_array->mv_y;

  if (block_size_y > (vidParam->iLumaPadY-4) && CheckVertMV(curMb, vec1_y, block_size_y))
  {
    get_block_luma(list, vec1_x, vec1_y, block_size_x, BLOCK_SIZE_8x8, tmp_block_l0, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
    get_block_luma(list, vec1_x, vec1_y+BLOCK_SIZE_8x8_SP, block_size_x, block_size_y-BLOCK_SIZE_8x8, tmp_block_l0+BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
  }
  else
    get_block_luma(list, vec1_x, vec1_y, block_size_x, block_size_y, tmp_block_l0,shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);

  mc_prediction(&curSlice->mb_pred[pl][joff], tmp_block_l0, block_size_y, block_size_x, ioff);

  if ((chroma_format_idc != YUV400) && (chroma_format_idc != YUV444) )
  {
    int ioff_cr,joff_cr,block_size_x_cr,block_size_y_cr;
    int vec1_y_cr = vec1_y + ((activeSPS->chroma_format_idc == 1)? curSlice->chroma_vector_adjustment[list_offset + pred_dir][ref_idx] : 0);
    int total_scale = vidParam->total_scale;
    int maxold_x = picture->size_x_cr_m1;
    int maxold_y = (curMb->mb_field) ? (picture->size_y_cr >> 1) - 1 : picture->size_y_cr_m1;
    if (vidParam->mb_cr_size_x == MB_BLOCK_SIZE)
    {
      ioff_cr = ioff;
      block_size_x_cr = block_size_x;
    }
    else
    {
      ioff_cr = ioff >> 1;
      block_size_x_cr = block_size_x >> 1;
    }
    if (vidParam->mb_cr_size_y == MB_BLOCK_SIZE)
    {
      joff_cr = joff;
      block_size_y_cr = block_size_y;
    }
    else
    {
      joff_cr = joff >> 1;
      block_size_y_cr = block_size_y >> 1;
    }
    no_ref_value = (sPixel)vidParam->dc_pred_value_comp[1];
    get_block_chroma(list,vec1_x,vec1_y_cr,vidParam->subpel_x,vidParam->subpel_y,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,vidParam->shiftpel_x,vidParam->shiftpel_y,&tmp_block_l0[0][0],&tmp_block_l1[0][0] ,total_scale,no_ref_value,vidParam);
    mc_prediction(&curSlice->mb_pred[1][joff_cr], tmp_block_l0, block_size_y_cr, block_size_x_cr, ioff_cr);
    mc_prediction(&curSlice->mb_pred[2][joff_cr], tmp_block_l1, block_size_y_cr, block_size_x_cr, ioff_cr);
  }
}
//}}}
//{{{
static void perform_mc_bi_wp (sMacroblock* curMb, sColorPlane pl, sPicture* picture, int i, int j, int block_size_x, int block_size_y)
{
  static const int mv_mul = 16;
  int  vec1_x, vec1_y, vec2_x, vec2_y;
  sVidParam* vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;

  int weighted_bipred_idc = vidParam->activePPS->weighted_bipred_idc;
  int block_y_aff = curMb->block_y_aff;
  int i4 = curMb->block_x + i;
  int j4 = curMb->block_y + j;
  int ioff = (i << 2);
  int joff = (j << 2);
  int chroma_format_idc = picture->chroma_format_idc;
  int list_offset = curMb->list_offset;
  sPicMotionParam *mv_info = &picture->mv_info[j4][i4];
  sMotionVector *l0_mv_array = &mv_info->mv[LIST_0];
  sMotionVector *l1_mv_array = &mv_info->mv[LIST_1];
  short l0_refframe = mv_info->ref_idx[LIST_0];
  short l1_refframe = mv_info->ref_idx[LIST_1];
  int l0_ref_idx  = (curMb->mb_field && weighted_bipred_idc == 1) ? l0_refframe >> 1: l0_refframe;
  int l1_ref_idx  = (curMb->mb_field && weighted_bipred_idc == 1) ? l1_refframe >> 1: l1_refframe;


  /// WP Parameters
  int wt_list_offset = (weighted_bipred_idc==2)? list_offset : 0;
  int *weight0 = curSlice->wbp_weight[LIST_0 + wt_list_offset][l0_ref_idx][l1_ref_idx];
  int *weight1 = curSlice->wbp_weight[LIST_1 + wt_list_offset][l0_ref_idx][l1_ref_idx];
  int *offset0 = curSlice->wp_offset[LIST_0 + wt_list_offset][l0_ref_idx];
  int *offset1 = curSlice->wp_offset[LIST_1 + wt_list_offset][l1_ref_idx];
  int maxold_y = (curMb->mb_field) ? (picture->size_y >> 1) - 1 : picture->size_y_m1;
  int pady = vidParam->iLumaPadY;
  int rlimit = maxold_y + pady - block_size_y - 2;
  int llimit = 2 - pady;
  int big_blocky = block_size_y > (pady - 4);
  sPicture *list0 = curSlice->listX[LIST_0 + list_offset][l0_refframe];
  sPicture *list1 = curSlice->listX[LIST_1 + list_offset][l1_refframe];
  sPixel** tmp_block_l0 = curSlice->tmp_block_l0;
  sPixel *block0 = tmp_block_l0[0];
  sPixel** tmp_block_l1 = curSlice->tmp_block_l1;
  sPixel *block1 = tmp_block_l1[0];
  sPixel** tmp_block_l2 = curSlice->tmp_block_l2;
  sPixel *block2 = tmp_block_l2[0];
  sPixel** tmp_block_l3 = curSlice->tmp_block_l3;
  sPixel *block3 = tmp_block_l3[0];
  int wp_offset;
  int wp_denom;

  // vars for get_block_luma
  int maxold_x = picture->size_x_m1;
  int shift_x  = picture->iLumaStride;
  int** tmp_res = curSlice->tmp_res;
  int max_imgpel_value = vidParam->max_pel_value_comp[pl];
  sPixel no_ref_value = (sPixel) vidParam->dc_pred_value_comp[pl];

  check_motion_vector_range(l0_mv_array, curSlice);
  check_motion_vector_range(l1_mv_array, curSlice);
  vec1_x = i4 * mv_mul + l0_mv_array->mv_x;
  vec2_x = i4 * mv_mul + l1_mv_array->mv_x;
  vec1_y = (block_y_aff + j) * mv_mul + l0_mv_array->mv_y;
  vec2_y = (block_y_aff + j) * mv_mul + l1_mv_array->mv_y;

  if (big_blocky && check_vert_mv(llimit, vec1_y, rlimit))
  {
    get_block_luma(list0, vec1_x, vec1_y, block_size_x, BLOCK_SIZE_8x8, tmp_block_l0, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
    get_block_luma(list0, vec1_x, vec1_y+BLOCK_SIZE_8x8_SP, block_size_x, block_size_y-BLOCK_SIZE_8x8, tmp_block_l0+BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
  }
  else
    get_block_luma(list0, vec1_x, vec1_y, block_size_x, block_size_y, tmp_block_l0,shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
  if (big_blocky && check_vert_mv(llimit, vec2_y,rlimit))
  {
    get_block_luma(list1, vec2_x, vec2_y, block_size_x, BLOCK_SIZE_8x8, tmp_block_l1, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
    get_block_luma(list1, vec2_x, vec2_y+BLOCK_SIZE_8x8_SP, block_size_x, block_size_y-BLOCK_SIZE_8x8, tmp_block_l1 + BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
  }
  else
    get_block_luma(list1, vec2_x, vec2_y, block_size_x, block_size_y, tmp_block_l1,shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);


  wp_offset = ((offset0[pl] + offset1[pl] + 1) >>1);
  wp_denom  = pl > 0 ? curSlice->chroma_log2_weight_denom : curSlice->luma_log2_weight_denom;
  weighted_bi_prediction(&curSlice->mb_pred[pl][joff][ioff], block0, block1, block_size_y, block_size_x, weight0[pl], weight1[pl], wp_offset, wp_denom + 1, max_imgpel_value);

  if ((chroma_format_idc != YUV400) && (chroma_format_idc != YUV444) )
  {
    int ioff_cr, joff_cr,block_size_y_cr,block_size_x_cr,vec2_y_cr,vec1_y_cr;
    int maxold_x = picture->size_x_cr_m1;
    int maxold_y = (curMb->mb_field) ? (picture->size_y_cr >> 1) - 1 : picture->size_y_cr_m1;
    int shiftpel_x = vidParam->shiftpel_x;
    int shiftpel_y = vidParam->shiftpel_y;
    int subpel_x = vidParam->subpel_x;
    int subpel_y =  vidParam->subpel_y;
    int total_scale = vidParam->total_scale;
    int chroma_log2 = curSlice->chroma_log2_weight_denom + 1;

    if (vidParam->mb_cr_size_x == MB_BLOCK_SIZE)
    {
      ioff_cr =  ioff;
      block_size_x_cr =  block_size_x;
    }
    else
    {
      ioff_cr = ioff >> 1;
      block_size_x_cr =  block_size_x >> 1;
    }

    if (vidParam->mb_cr_size_y == MB_BLOCK_SIZE)
    {
      joff_cr = joff;
      block_size_y_cr = block_size_y;
    }
    else
    {
      joff_cr = joff >> 1;
      block_size_y_cr = block_size_y >> 1;
    }
    if (chroma_format_idc == 1)
    {
      vec1_y_cr = vec1_y + curSlice->chroma_vector_adjustment[LIST_0 + list_offset][l0_refframe];
      vec2_y_cr = vec2_y + curSlice->chroma_vector_adjustment[LIST_1 + list_offset][l1_refframe];
    }
    else
    {
      vec1_y_cr = vec1_y;
      vec2_y_cr = vec2_y;
    }
    no_ref_value = (sPixel)vidParam->dc_pred_value_comp[1];

    wp_offset = ((offset0[1] + offset1[1] + 1) >>1);
    get_block_chroma(list0,vec1_x,vec1_y_cr,subpel_x,subpel_y,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,shiftpel_x,shiftpel_y,block0,block2 ,total_scale,no_ref_value,vidParam);
    get_block_chroma(list1,vec2_x,vec2_y_cr,subpel_x,subpel_y,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,shiftpel_x,shiftpel_y,block1,block3 ,total_scale,no_ref_value,vidParam);
    weighted_bi_prediction(&curSlice->mb_pred[1][joff_cr][ioff_cr],block0,block1,block_size_y_cr,block_size_x_cr,weight0[1],weight1[1],wp_offset,chroma_log2,vidParam->max_pel_value_comp[1]);
    wp_offset = ((offset0[2] + offset1[2] + 1) >>1);
    weighted_bi_prediction(&curSlice->mb_pred[2][joff_cr][ioff_cr],block2,block3,block_size_y_cr,block_size_x_cr,weight0[2],weight1[2],wp_offset,chroma_log2,vidParam->max_pel_value_comp[2]);
  }
}
//}}}
//{{{
static void perform_mc_bi (sMacroblock* curMb, sColorPlane pl, sPicture* picture, int i, int j, int block_size_x, int block_size_y)
{
  static const int mv_mul = 16;
  int vec1_x=0, vec1_y=0, vec2_x=0, vec2_y=0;
  sVidParam* vidParam = curMb->vidParam;
  sSlice* curSlice = curMb->slice;

  int block_y_aff = curMb->block_y_aff;
  int i4 = curMb->block_x + i;
  int j4 = curMb->block_y + j;
  int ioff = (i << 2);
  int joff = (j << 2);
  int chroma_format_idc = picture->chroma_format_idc;
  sPicMotionParam *mv_info = &picture->mv_info[j4][i4];
  sMotionVector *l0_mv_array = &mv_info->mv[LIST_0];
  sMotionVector *l1_mv_array = &mv_info->mv[LIST_1];
  short l0_refframe = mv_info->ref_idx[LIST_0];
  short l1_refframe = mv_info->ref_idx[LIST_1];
  int list_offset = curMb->list_offset;

  int maxold_y = (curMb->mb_field) ? (picture->size_y >> 1) - 1 : picture->size_y_m1;
  int pady = vidParam->iLumaPadY;
  int rlimit = maxold_y + pady - block_size_y - 2;
  int llimit = 2 - pady;
  int big_blocky = block_size_y > (pady - 4);
  sPicture *list0 = curSlice->listX[LIST_0 + list_offset][l0_refframe];
  sPicture *list1 = curSlice->listX[LIST_1 + list_offset][l1_refframe];
  sPixel** tmp_block_l0 = curSlice->tmp_block_l0;
  sPixel *block0 = tmp_block_l0[0];
  sPixel** tmp_block_l1 = curSlice->tmp_block_l1;
  sPixel *block1 = tmp_block_l1[0];
  sPixel** tmp_block_l2 = curSlice->tmp_block_l2;
  sPixel *block2 = tmp_block_l2[0];
  sPixel** tmp_block_l3 = curSlice->tmp_block_l3;
  sPixel *block3 = tmp_block_l3[0];
  // vars for get_block_luma
  int maxold_x = picture->size_x_m1;
  int shift_x  = picture->iLumaStride;
  int** tmp_res = curSlice->tmp_res;
  int max_imgpel_value = vidParam->max_pel_value_comp[pl];
  sPixel no_ref_value = (sPixel) vidParam->dc_pred_value_comp[pl];
  check_motion_vector_range(l0_mv_array, curSlice);
  check_motion_vector_range(l1_mv_array, curSlice);
  vec1_x = i4 * mv_mul + l0_mv_array->mv_x;
  vec2_x = i4 * mv_mul + l1_mv_array->mv_x;
  vec1_y = (block_y_aff + j) * mv_mul + l0_mv_array->mv_y;
  vec2_y = (block_y_aff + j) * mv_mul + l1_mv_array->mv_y;
  if (big_blocky && check_vert_mv(llimit, vec1_y, rlimit))
  {
    get_block_luma(list0, vec1_x, vec1_y, block_size_x, BLOCK_SIZE_8x8, tmp_block_l0, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
    get_block_luma(list0, vec1_x, vec1_y+BLOCK_SIZE_8x8_SP, block_size_x, block_size_y-BLOCK_SIZE_8x8, tmp_block_l0+BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
  }
  else
    get_block_luma(list0, vec1_x, vec1_y, block_size_x, block_size_y, tmp_block_l0,shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
  if (big_blocky && check_vert_mv(llimit, vec2_y,rlimit))
  {
    get_block_luma(list1, vec2_x, vec2_y, block_size_x, BLOCK_SIZE_8x8, tmp_block_l1, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
    get_block_luma(list1, vec2_x, vec2_y+BLOCK_SIZE_8x8_SP, block_size_x, block_size_y-BLOCK_SIZE_8x8, tmp_block_l1 + BLOCK_SIZE_8x8, shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
  }
  else
    get_block_luma(list1, vec2_x, vec2_y, block_size_x, block_size_y, tmp_block_l1,shift_x,maxold_x,maxold_y,tmp_res,max_imgpel_value,no_ref_value, curMb);
  bi_prediction(&curSlice->mb_pred[pl][joff],tmp_block_l0,tmp_block_l1, block_size_y, block_size_x, ioff);

  if ((chroma_format_idc != YUV400) && (chroma_format_idc != YUV444) )
  {
    int ioff_cr, joff_cr,block_size_y_cr,block_size_x_cr,vec2_y_cr,vec1_y_cr;
    int chroma_format_idc = vidParam->activeSPS->chroma_format_idc;
    int maxold_x = picture->size_x_cr_m1;
    int maxold_y = (curMb->mb_field) ? (picture->size_y_cr >> 1) - 1 : picture->size_y_cr_m1;
    int shiftpel_x = vidParam->shiftpel_x;
    int shiftpel_y = vidParam->shiftpel_y;
    int subpel_x = vidParam->subpel_x;
    int subpel_y =  vidParam->subpel_y;
    int total_scale = vidParam->total_scale;
    if (vidParam->mb_cr_size_x == MB_BLOCK_SIZE)
    {
      ioff_cr =  ioff;
      block_size_x_cr =  block_size_x;
    }
    else
    {
      ioff_cr = ioff >> 1;
      block_size_x_cr =  block_size_x >> 1;
    }
    if (vidParam->mb_cr_size_y == MB_BLOCK_SIZE)
    {
      joff_cr = joff;
      block_size_y_cr = block_size_y;
    }
    else
    {
      joff_cr = joff >> 1;
      block_size_y_cr = block_size_y >> 1;
    }
    if (chroma_format_idc == 1)
    {
      vec1_y_cr = vec1_y + curSlice->chroma_vector_adjustment[LIST_0 + list_offset][l0_refframe];
      vec2_y_cr = vec2_y + curSlice->chroma_vector_adjustment[LIST_1 + list_offset][l1_refframe];
    }
    else
    {
      vec1_y_cr = vec1_y;
      vec2_y_cr = vec2_y;
    }
    no_ref_value = (sPixel)vidParam->dc_pred_value_comp[1];
    get_block_chroma(list0,vec1_x,vec1_y_cr,subpel_x,subpel_y,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,shiftpel_x,shiftpel_y,block0,block2 ,total_scale,no_ref_value,vidParam);
    get_block_chroma(list1,vec2_x,vec2_y_cr,subpel_x,subpel_y,maxold_x,maxold_y,block_size_x_cr,block_size_y_cr,shiftpel_x,shiftpel_y,block1,block3 ,total_scale,no_ref_value,vidParam);
    bi_prediction(&curSlice->mb_pred[1][joff_cr],tmp_block_l0,tmp_block_l1, block_size_y_cr, block_size_x_cr, ioff_cr);
    bi_prediction(&curSlice->mb_pred[2][joff_cr],tmp_block_l2,tmp_block_l3, block_size_y_cr, block_size_x_cr, ioff_cr);
  }
}
//}}}
//{{{
void perform_mc (sMacroblock* curMb, sColorPlane pl, sPicture* picture, int pred_dir, int i, int j, int block_size_x, int block_size_y)
{
  sSlice* curSlice = curMb->slice;
  assert (pred_dir<=2);
  if (pred_dir != 2)
  {
    if (curSlice->weighted_pred_flag)
      perform_mc_single_wp(curMb, pl, picture, pred_dir, i, j, block_size_x, block_size_y);
    else
      perform_mc_single(curMb, pl, picture, pred_dir, i, j, block_size_x, block_size_y);
  }
  else
  {
    if (curSlice->weighted_bipred_idc)
      perform_mc_bi_wp(curMb, pl, picture, i, j, block_size_x, block_size_y);
    else
      perform_mc_bi(curMb, pl, picture, i, j, block_size_x, block_size_y);
  }
}
//}}}
