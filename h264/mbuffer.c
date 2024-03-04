//{{{
/*!
 ***********************************************************************
 *  \file
 *      mbuffer.c
 *
 *  \brief
 *      Frame buffer functions
 *
 *  \author
 *      Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Karsten Suehring
 *      - Alexis Tourapis                 <alexismt@ieee.org>
 *      - Jill Boyce                      <jill.boyce@thomson.net>
 *      - Saurav K Bandyopadhyay          <saurav@ieee.org>
 *      - Zhenyu Wu                       <Zhenyu.Wu@thomson.net
 *      - Purvin Pandit                   <Purvin.Pandit@thomson.net>
 *      - Yuwen He                        <yhe@dolby.com>
 ***********************************************************************
 */
//}}}
//{{{
#include <limits.h>

#include "global.h"
#include "erc.h"
#include "header.h"
#include "image.h"
#include "mbuffer.h"
#include "memalloc.h"
#include "output.h"
//}}}
#define MAX_LIST_SIZE 33

#define DUMP_DPB 0
//{{{
static void dump_dpb (DecodedPictureBuffer* p_Dpb) {

#if DUMP_DPB
  for (unsigned i = 0; i < p_Dpb->used_size;i++) {
    printf ("(");
    printf ("fn=%d  ", p_Dpb->fs[i]->frame_num);
    if (p_Dpb->fs[i]->is_used & 1) {
      if (p_Dpb->fs[i]->top_field)
        printf ("T: poc=%d  ", p_Dpb->fs[i]->top_field->poc);
      else
        printf ("T: poc=%d  ", p_Dpb->fs[i]->frame->top_poc);
      }

    if (p_Dpb->fs[i]->is_used & 2) {
      if (p_Dpb->fs[i]->bottom_field)
        printf ("B: poc=%d  ", p_Dpb->fs[i]->bottom_field->poc);
      else
        printf ("B: poc=%d  ", p_Dpb->fs[i]->frame->bottom_poc);
      }

    if (p_Dpb->fs[i]->is_used == 3)
      printf ("F: poc=%d  ", p_Dpb->fs[i]->frame->poc);
    printf ("G: poc=%d)  ", p_Dpb->fs[i]->poc);

    if (p_Dpb->fs[i]->is_reference)
      printf ("ref (%d) ", p_Dpb->fs[i]->is_reference);
    if (p_Dpb->fs[i]->is_long_term)
      printf ("lt_ref (%d) ", p_Dpb->fs[i]->is_reference);
    if (p_Dpb->fs[i]->is_output)
      printf ("out  ");
    if (p_Dpb->fs[i]->is_used == 3)
      if (p_Dpb->fs[i]->frame->non_existing)
        printf ("ne  ");

#if (MVC_EXTENSION_ENABLE)
    if (p_Dpb->fs[i]->is_reference)
      printf ("view_id (%d) ", p_Dpb->fs[i]->view_id);
#endif
    printf ("\n");
    }
#endif
  }
//}}}
//{{{
static void gen_field_ref_ids (VideoParameters *p_Vid, StorablePicture *p) {
// Generate Frame parameters from field information.

  // copy the list;
  for (int j = 0; j < p_Vid->iSliceNumOfCurrPic; j++) {
    if (p->listX[j][LIST_0]) {
      p->listXsize[j][LIST_0] = p_Vid->ppSliceList[j]->listXsize[LIST_0];
      for (int i = 0; i < p->listXsize[j][LIST_0]; i++)
        p->listX[j][LIST_0][i] = p_Vid->ppSliceList[j]->listX[LIST_0][i];
      }

    if(p->listX[j][LIST_1]) {
      p->listXsize[j][LIST_1] = p_Vid->ppSliceList[j]->listXsize[LIST_1];
      for(int i = 0; i < p->listXsize[j][LIST_1]; i++)
        p->listX[j][LIST_1][i] = p_Vid->ppSliceList[j]->listX[LIST_1][i];
      }
    }
  }
//}}}
//{{{
static void insert_picture_in_dpb (VideoParameters *p_Vid, FrameStore* fs, StorablePicture* p) {

  InputParameters* p_Inp = p_Vid->p_Inp;
  //  printf ("insert (%s) pic with frame_num #%d, poc %d\n",
  //          (p->structure == FRAME)?"FRAME":(p->structure == TOP_FIELD)?"TOP_FIELD":"BOTTOM_FIELD",
  //          p->pic_num, p->poc);
  assert (p != NULL);
  assert (fs != NULL);

  switch (p->structure) {
    //{{{
    case FRAME:
      fs->frame = p;
      fs->is_used = 3;
      if (p->used_for_reference) {
        fs->is_reference = 3;
        fs->is_orig_reference = 3;
        if (p->is_long_term) {
          fs->is_long_term = 3;
          fs->long_term_frame_idx = p->long_term_frame_idx;
          }
        }

      fs->layer_id = p->layer_id;
      // generate field views
      dpb_split_field(p_Vid, fs);
      break;
    //}}}
    //{{{
    case TOP_FIELD:
      fs->top_field = p;
      fs->is_used |= 1;
      fs->layer_id = p->layer_id;

      if (p->used_for_reference) {
        fs->is_reference |= 1;
        fs->is_orig_reference |= 1;
        if (p->is_long_term) {
          fs->is_long_term |= 1;
          fs->long_term_frame_idx = p->long_term_frame_idx;
          }
        }
      if (fs->is_used == 3)
        // generate frame view
        dpb_combine_field(p_Vid, fs);
      else
        fs->poc = p->poc;

      gen_field_ref_ids(p_Vid, p);
      break;
    //}}}
    //{{{
    case BOTTOM_FIELD:
      fs->bottom_field = p;
      fs->is_used |= 2;
      fs->layer_id = p->layer_id;

      if (p->used_for_reference) {
        fs->is_reference |= 2;
        fs->is_orig_reference |= 2;
        if (p->is_long_term) {
          fs->is_long_term |= 2;
          fs->long_term_frame_idx = p->long_term_frame_idx;
          }
        }

      if (fs->is_used == 3)
        // generate frame view
        dpb_combine_field(p_Vid, fs);
      else
        fs->poc = p->poc;

      gen_field_ref_ids(p_Vid, p);
      break;
    //}}}
    }

  fs->frame_num = p->pic_num;
  fs->recovery_frame = p->recovery_frame;
  fs->is_output = p->is_output;
  if (fs->is_used == 3)
    calculate_frame_no (p_Vid, p);
}
//}}}
//{{{
static int output_one_frame_from_dpb (DecodedPictureBuffer *p_Dpb) {

  VideoParameters *p_Vid = p_Dpb->p_Vid;
  int poc, pos;

  //diagnostics
  if (p_Dpb->used_size < 1)
    error("Cannot output frame, DPB empty.",150);

  // find smallest POC
  get_smallest_poc (p_Dpb, &poc, &pos);

  if(pos==-1)
    return 0;

  // call the output function
  //  printf ("output frame with frame_num #%d, poc %d (dpb. p_Dpb->size=%d, p_Dpb->used_size=%d)\n", p_Dpb->fs[pos]->frame_num, p_Dpb->fs[pos]->frame->poc, p_Dpb->size, p_Dpb->used_size);

  // picture error concealment
  if (p_Vid->conceal_mode != 0) {
    if (p_Dpb->last_output_poc == 0)
      write_lost_ref_after_idr(p_Dpb, pos);
#if (MVC_EXTENSION_ENABLE)
    write_lost_non_ref_pic(p_Dpb, poc);
#else
    write_lost_non_ref_pic(p_Dpb, poc);
#endif
  }

// JVT-P072 ends

#if (MVC_EXTENSION_ENABLE)
  write_stored_frame(p_Vid, p_Dpb->fs[pos]);
#else
  write_stored_frame(p_Vid, p_Dpb->fs[pos]);
#endif

  // picture error concealment
  if(p_Vid->conceal_mode == 0)
    if (p_Dpb->last_output_poc >= poc)
      error ("output POC must be in ascending order", 150);

  p_Dpb->last_output_poc = poc;

  // free frame store and move empty store to end of buffer
  if (!is_used_for_reference(p_Dpb->fs[pos]))
    remove_frame_from_dpb(p_Dpb, pos);
  return 1;
  }
//}}}
//{{{
static void unmark_long_term_field_for_reference_by_frame_idx (
  DecodedPictureBuffer *p_Dpb, PictureStructure structure,
  int long_term_frame_idx, int mark_current, unsigned curr_frame_num, int curr_pic_num) {

  VideoParameters *p_Vid = p_Dpb->p_Vid;
  unsigned i;

  assert(structure!=FRAME);
  if (curr_pic_num<0)
    curr_pic_num += (2 * p_Vid->max_frame_num);

  for(i=0; i<p_Dpb->ltref_frames_in_buffer; i++) {
    if (p_Dpb->fs_ltref[i]->long_term_frame_idx == long_term_frame_idx) {
      if (structure == TOP_FIELD) {
        if (p_Dpb->fs_ltref[i]->is_long_term == 3)
          unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
        else {
          if (p_Dpb->fs_ltref[i]->is_long_term == 1)
            unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
          else {
            if (mark_current) {
              if (p_Dpb->last_picture) {
                if ( ( p_Dpb->last_picture != p_Dpb->fs_ltref[i] )|| p_Dpb->last_picture->frame_num != curr_frame_num)
                  unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                }
              else
                unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
            }
            else {
              if ((p_Dpb->fs_ltref[i]->frame_num) != (unsigned)(curr_pic_num >> 1))
                unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
              }
            }
          }
        }

      if (structure == BOTTOM_FIELD) {
        if (p_Dpb->fs_ltref[i]->is_long_term == 3)
          unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
        else {
          if (p_Dpb->fs_ltref[i]->is_long_term == 2)
            unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
          else {
            if (mark_current) {
              if (p_Dpb->last_picture) {
                if ( ( p_Dpb->last_picture != p_Dpb->fs_ltref[i] )|| p_Dpb->last_picture->frame_num != curr_frame_num)
                  unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                }
              else
                unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
              }
            else {
              if ((p_Dpb->fs_ltref[i]->frame_num) != (unsigned)(curr_pic_num >> 1))
                unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
              }
            }
          }
        }
      }
    }
  }
//}}}
//{{{
static int is_short_term_reference (FrameStore* fs) {

  if (fs->is_used==3) // frame
    if ((fs->frame->used_for_reference)&&(!fs->frame->is_long_term))
      return 1;

  if (fs->is_used & 1) // top field
    if (fs->top_field)
      if ((fs->top_field->used_for_reference)&&(!fs->top_field->is_long_term))
        return 1;

  if (fs->is_used & 2) // bottom field
    if (fs->bottom_field)
      if ((fs->bottom_field->used_for_reference)&&(!fs->bottom_field->is_long_term))
        return 1;

  return 0;
  }
//}}}
//{{{
static int is_long_term_reference (FrameStore* fs) {


  if (fs->is_used==3) // frame
    if ((fs->frame->used_for_reference)&&(fs->frame->is_long_term))
      return 1;

  if (fs->is_used & 1) // top field
    if (fs->top_field)
      if ((fs->top_field->used_for_reference)&&(fs->top_field->is_long_term))
        return 1;

  if (fs->is_used & 2) // bottom field
    if (fs->bottom_field)
      if ((fs->bottom_field->used_for_reference)&&(fs->bottom_field->is_long_term))
        return 1;

  return 0;
  }
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    Generates a alternating field list from a given FrameStore list
 *
 ************************************************************************
 */
void gen_pic_list_from_frame_list (PictureStructure currStructure, FrameStore **fs_list, int list_idx, StorablePicture **list, char *list_size, int long_term)
{
  int top_idx = 0;
  int bot_idx = 0;

  int (*is_ref)(StorablePicture *s) = (long_term) ? is_long_ref : is_short_ref;


  if (currStructure == TOP_FIELD)
  {
    while ((top_idx<list_idx)||(bot_idx<list_idx))
    {
      for ( ; top_idx<list_idx; top_idx++)
      {
        if(fs_list[top_idx]->is_used & 1)
        {
          if(is_ref(fs_list[top_idx]->top_field))
          {
            // short term ref pic
            list[(short) *list_size] = fs_list[top_idx]->top_field;
            (*list_size)++;
            top_idx++;
            break;
          }
        }
      }
      for ( ; bot_idx<list_idx; bot_idx++)
      {
        if(fs_list[bot_idx]->is_used & 2)
        {
          if(is_ref(fs_list[bot_idx]->bottom_field))
          {
            // short term ref pic
            list[(short) *list_size] = fs_list[bot_idx]->bottom_field;
            (*list_size)++;
            bot_idx++;
            break;
          }
        }
      }
    }
  }
  if (currStructure == BOTTOM_FIELD)
  {
    while ((top_idx<list_idx)||(bot_idx<list_idx))
    {
      for ( ; bot_idx<list_idx; bot_idx++)
      {
        if(fs_list[bot_idx]->is_used & 2)
        {
          if(is_ref(fs_list[bot_idx]->bottom_field))
          {
            // short term ref pic
            list[(short) *list_size] = fs_list[bot_idx]->bottom_field;
            (*list_size)++;
            bot_idx++;
            break;
          }
        }
      }
      for ( ; top_idx<list_idx; top_idx++)
      {
        if(fs_list[top_idx]->is_used & 1)
        {
          if(is_ref(fs_list[top_idx]->top_field))
          {
            // short term ref pic
            list[(short) *list_size] = fs_list[top_idx]->top_field;
            (*list_size)++;
            top_idx++;
            break;
          }
        }
      }
    }
  }
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Returns long term pic with given LongtermPicNum
 *
 ************************************************************************
 */
StorablePicture*  get_long_term_pic (Slice *currSlice, DecodedPictureBuffer *p_Dpb, int LongtermPicNum)
{
  uint32 i;

  for (i=0; i<p_Dpb->ltref_frames_in_buffer; i++)
  {
    if (currSlice->structure==FRAME)
    {
      if (p_Dpb->fs_ltref[i]->is_reference == 3)
        if ((p_Dpb->fs_ltref[i]->frame->is_long_term)&&(p_Dpb->fs_ltref[i]->frame->long_term_pic_num == LongtermPicNum))
          return p_Dpb->fs_ltref[i]->frame;
    }
    else
    {
      if (p_Dpb->fs_ltref[i]->is_reference & 1)
        if ((p_Dpb->fs_ltref[i]->top_field->is_long_term)&&(p_Dpb->fs_ltref[i]->top_field->long_term_pic_num == LongtermPicNum))
          return p_Dpb->fs_ltref[i]->top_field;
      if (p_Dpb->fs_ltref[i]->is_reference & 2)
        if ((p_Dpb->fs_ltref[i]->bottom_field->is_long_term)&&(p_Dpb->fs_ltref[i]->bottom_field->long_term_pic_num == LongtermPicNum))
          return p_Dpb->fs_ltref[i]->bottom_field;
    }
  }
  return NULL;
}
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    Update the list of frame stores that contain reference frames/fields
 *
 ************************************************************************
 */
void update_ref_list (DecodedPictureBuffer *p_Dpb)
{
  unsigned i, j;
  for (i=0, j=0; i<p_Dpb->used_size; i++)
  {
    if (is_short_term_reference(p_Dpb->fs[i]))
    {
      p_Dpb->fs_ref[j++]=p_Dpb->fs[i];
    }
  }

  p_Dpb->ref_frames_in_buffer = j;

  while (j<p_Dpb->size)
  {
    p_Dpb->fs_ref[j++]=NULL;
  }
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Update the list of frame stores that contain long-term reference
 *    frames/fields
 *
 ************************************************************************
 */
void update_ltref_list (DecodedPictureBuffer *p_Dpb)
{
  unsigned i, j;
  for (i=0, j=0; i<p_Dpb->used_size; i++)
  {
    if (is_long_term_reference(p_Dpb->fs[i]))
    {
      p_Dpb->fs_ltref[j++] = p_Dpb->fs[i];
    }
  }

  p_Dpb->ltref_frames_in_buffer = j;

  while (j<p_Dpb->size)
  {
    p_Dpb->fs_ltref[j++]=NULL;
  }
}
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    Calculate picNumX
 ************************************************************************
 */
static int get_pic_num_x (StorablePicture *p, int difference_of_pic_nums_minus1)
{
  int currPicNum;

  if (p->structure == FRAME)
    currPicNum = p->frame_num;
  else
    currPicNum = 2 * p->frame_num + 1;

  return currPicNum - (difference_of_pic_nums_minus1 + 1);
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Adaptive Memory Management: Mark short term picture unused
 ************************************************************************
 */
void mm_unmark_short_term_for_reference (DecodedPictureBuffer *p_Dpb, StorablePicture *p, int difference_of_pic_nums_minus1)
{
  int picNumX;

  uint32 i;

  picNumX = get_pic_num_x(p, difference_of_pic_nums_minus1);

  for (i=0; i<p_Dpb->ref_frames_in_buffer; i++)
  {
    if (p->structure == FRAME)
    {
      if ((p_Dpb->fs_ref[i]->is_reference==3) && (p_Dpb->fs_ref[i]->is_long_term==0))
      {
        if (p_Dpb->fs_ref[i]->frame->pic_num == picNumX)
        {
          unmark_for_reference(p_Dpb->fs_ref[i]);
          return;
        }
      }
    }
    else
    {
      if ((p_Dpb->fs_ref[i]->is_reference & 1) && (!(p_Dpb->fs_ref[i]->is_long_term & 1)))
      {
        if (p_Dpb->fs_ref[i]->top_field->pic_num == picNumX)
        {
          p_Dpb->fs_ref[i]->top_field->used_for_reference = 0;
          p_Dpb->fs_ref[i]->is_reference &= 2;
          if (p_Dpb->fs_ref[i]->is_used == 3)
          {
            p_Dpb->fs_ref[i]->frame->used_for_reference = 0;
          }
          return;
        }
      }
      if ((p_Dpb->fs_ref[i]->is_reference & 2) && (!(p_Dpb->fs_ref[i]->is_long_term & 2)))
      {
        if (p_Dpb->fs_ref[i]->bottom_field->pic_num == picNumX)
        {
          p_Dpb->fs_ref[i]->bottom_field->used_for_reference = 0;
          p_Dpb->fs_ref[i]->is_reference &= 1;
          if (p_Dpb->fs_ref[i]->is_used == 3)
          {
            p_Dpb->fs_ref[i]->frame->used_for_reference = 0;
          }
          return;
        }
      }
    }
  }
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Adaptive Memory Management: Mark long term picture unused
 ************************************************************************
 */
void mm_unmark_long_term_for_reference (DecodedPictureBuffer *p_Dpb, StorablePicture *p, int long_term_pic_num)
{
  uint32 i;
  for (i=0; i<p_Dpb->ltref_frames_in_buffer; i++)
  {
    if (p->structure == FRAME)
    {
      if ((p_Dpb->fs_ltref[i]->is_reference==3) && (p_Dpb->fs_ltref[i]->is_long_term==3))
      {
        if (p_Dpb->fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num)
        {
          unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
        }
      }
    }
    else
    {
      if ((p_Dpb->fs_ltref[i]->is_reference & 1) && ((p_Dpb->fs_ltref[i]->is_long_term & 1)))
      {
        if (p_Dpb->fs_ltref[i]->top_field->long_term_pic_num == long_term_pic_num)
        {
          p_Dpb->fs_ltref[i]->top_field->used_for_reference = 0;
          p_Dpb->fs_ltref[i]->top_field->is_long_term = 0;
          p_Dpb->fs_ltref[i]->is_reference &= 2;
          p_Dpb->fs_ltref[i]->is_long_term &= 2;
          if (p_Dpb->fs_ltref[i]->is_used == 3)
          {
            p_Dpb->fs_ltref[i]->frame->used_for_reference = 0;
            p_Dpb->fs_ltref[i]->frame->is_long_term = 0;
          }
          return;
        }
      }
      if ((p_Dpb->fs_ltref[i]->is_reference & 2) && ((p_Dpb->fs_ltref[i]->is_long_term & 2)))
      {
        if (p_Dpb->fs_ltref[i]->bottom_field->long_term_pic_num == long_term_pic_num)
        {
          p_Dpb->fs_ltref[i]->bottom_field->used_for_reference = 0;
          p_Dpb->fs_ltref[i]->bottom_field->is_long_term = 0;
          p_Dpb->fs_ltref[i]->is_reference &= 1;
          p_Dpb->fs_ltref[i]->is_long_term &= 1;
          if (p_Dpb->fs_ltref[i]->is_used == 3)
          {
            p_Dpb->fs_ltref[i]->frame->used_for_reference = 0;
            p_Dpb->fs_ltref[i]->frame->is_long_term = 0;
          }
          return;
        }
      }
    }
  }
}
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    Mark a long-term reference frame or complementary field pair unused for referemce
 ************************************************************************
 */
static void unmark_long_term_frame_for_reference_by_frame_idx (DecodedPictureBuffer *p_Dpb, int long_term_frame_idx)
{
  uint32 i;
  for(i=0; i<p_Dpb->ltref_frames_in_buffer; i++)
  {
    if (p_Dpb->fs_ltref[i]->long_term_frame_idx == long_term_frame_idx)
      unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
  }
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    mark a picture as long-term reference
 ************************************************************************
 */
static void mark_pic_long_term (DecodedPictureBuffer *p_Dpb, StorablePicture* p, int long_term_frame_idx, int picNumX)
{
  uint32 i;
  int add_top, add_bottom;

  if (p->structure == FRAME)
  {
    for (i=0; i<p_Dpb->ref_frames_in_buffer; i++)
    {
      if (p_Dpb->fs_ref[i]->is_reference == 3)
      {
        if ((!p_Dpb->fs_ref[i]->frame->is_long_term)&&(p_Dpb->fs_ref[i]->frame->pic_num == picNumX))
        {
          p_Dpb->fs_ref[i]->long_term_frame_idx = p_Dpb->fs_ref[i]->frame->long_term_frame_idx
                                             = long_term_frame_idx;
          p_Dpb->fs_ref[i]->frame->long_term_pic_num = long_term_frame_idx;
          p_Dpb->fs_ref[i]->frame->is_long_term = 1;

          if (p_Dpb->fs_ref[i]->top_field && p_Dpb->fs_ref[i]->bottom_field)
          {
            p_Dpb->fs_ref[i]->top_field->long_term_frame_idx = p_Dpb->fs_ref[i]->bottom_field->long_term_frame_idx
                                                          = long_term_frame_idx;
            p_Dpb->fs_ref[i]->top_field->long_term_pic_num = long_term_frame_idx;
            p_Dpb->fs_ref[i]->bottom_field->long_term_pic_num = long_term_frame_idx;

            p_Dpb->fs_ref[i]->top_field->is_long_term = p_Dpb->fs_ref[i]->bottom_field->is_long_term
                                                   = 1;

          }
          p_Dpb->fs_ref[i]->is_long_term = 3;
          return;
        }
      }
    }
    printf ("Warning: reference frame for long term marking not found\n");
  }
  else
  {
    if (p->structure == TOP_FIELD)
    {
      add_top    = 1;
      add_bottom = 0;
    }
    else
    {
      add_top    = 0;
      add_bottom = 1;
    }
    for (i=0; i<p_Dpb->ref_frames_in_buffer; i++)
    {
      if (p_Dpb->fs_ref[i]->is_reference & 1)
      {
        if ((!p_Dpb->fs_ref[i]->top_field->is_long_term)&&(p_Dpb->fs_ref[i]->top_field->pic_num == picNumX))
        {
          if ((p_Dpb->fs_ref[i]->is_long_term) && (p_Dpb->fs_ref[i]->long_term_frame_idx != long_term_frame_idx))
          {
              printf ("Warning: assigning long_term_frame_idx different from other field\n");
          }

          p_Dpb->fs_ref[i]->long_term_frame_idx = p_Dpb->fs_ref[i]->top_field->long_term_frame_idx
                                             = long_term_frame_idx;
          p_Dpb->fs_ref[i]->top_field->long_term_pic_num = 2 * long_term_frame_idx + add_top;
          p_Dpb->fs_ref[i]->top_field->is_long_term = 1;
          p_Dpb->fs_ref[i]->is_long_term |= 1;
          if (p_Dpb->fs_ref[i]->is_long_term == 3)
          {
            p_Dpb->fs_ref[i]->frame->is_long_term = 1;
            p_Dpb->fs_ref[i]->frame->long_term_frame_idx = p_Dpb->fs_ref[i]->frame->long_term_pic_num = long_term_frame_idx;
          }
          return;
        }
      }
      if (p_Dpb->fs_ref[i]->is_reference & 2)
      {
        if ((!p_Dpb->fs_ref[i]->bottom_field->is_long_term)&&(p_Dpb->fs_ref[i]->bottom_field->pic_num == picNumX))
        {
          if ((p_Dpb->fs_ref[i]->is_long_term) && (p_Dpb->fs_ref[i]->long_term_frame_idx != long_term_frame_idx))
          {
              printf ("Warning: assigning long_term_frame_idx different from other field\n");
          }

          p_Dpb->fs_ref[i]->long_term_frame_idx = p_Dpb->fs_ref[i]->bottom_field->long_term_frame_idx
                                             = long_term_frame_idx;
          p_Dpb->fs_ref[i]->bottom_field->long_term_pic_num = 2 * long_term_frame_idx + add_bottom;
          p_Dpb->fs_ref[i]->bottom_field->is_long_term = 1;
          p_Dpb->fs_ref[i]->is_long_term |= 2;
          if (p_Dpb->fs_ref[i]->is_long_term == 3)
          {
            p_Dpb->fs_ref[i]->frame->is_long_term = 1;
            p_Dpb->fs_ref[i]->frame->long_term_frame_idx = p_Dpb->fs_ref[i]->frame->long_term_pic_num = long_term_frame_idx;
          }
          return;
        }
      }
    }
    printf ("Warning: reference field for long term marking not found\n");
  }
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Assign a long term frame index to a short term picture
 ************************************************************************
 */
void mm_assign_long_term_frame_idx (DecodedPictureBuffer *p_Dpb, StorablePicture* p, int difference_of_pic_nums_minus1, int long_term_frame_idx)
{
  int picNumX = get_pic_num_x(p, difference_of_pic_nums_minus1);

  // remove frames/fields with same long_term_frame_idx
  if (p->structure == FRAME)
  {
    unmark_long_term_frame_for_reference_by_frame_idx(p_Dpb, long_term_frame_idx);
  }
  else
  {
    unsigned i;
    PictureStructure structure = FRAME;

    for (i=0; i<p_Dpb->ref_frames_in_buffer; i++)
    {
      if (p_Dpb->fs_ref[i]->is_reference & 1)
      {
        if (p_Dpb->fs_ref[i]->top_field->pic_num == picNumX)
        {
          structure = TOP_FIELD;
          break;
        }
      }
      if (p_Dpb->fs_ref[i]->is_reference & 2)
      {
        if (p_Dpb->fs_ref[i]->bottom_field->pic_num == picNumX)
        {
          structure = BOTTOM_FIELD;
          break;
        }
      }
    }
    if (structure==FRAME)
    {
      error ("field for long term marking not found",200);
    }

    unmark_long_term_field_for_reference_by_frame_idx(p_Dpb, structure, long_term_frame_idx, 0, 0, picNumX);
  }

  mark_pic_long_term(p_Dpb, p, long_term_frame_idx, picNumX);
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Set new max long_term_frame_idx
 ************************************************************************
 */
void mm_update_max_long_term_frame_idx (DecodedPictureBuffer *p_Dpb, int max_long_term_frame_idx_plus1)
{
  uint32 i;

  p_Dpb->max_long_term_pic_idx = max_long_term_frame_idx_plus1 - 1;

  // check for invalid frames
  for (i=0; i<p_Dpb->ltref_frames_in_buffer; i++)
  {
    if (p_Dpb->fs_ltref[i]->long_term_frame_idx > p_Dpb->max_long_term_pic_idx)
    {
      unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
    }
  }
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Mark all long term reference pictures unused for reference
 ************************************************************************
 */
void mm_unmark_all_long_term_for_reference (DecodedPictureBuffer *p_Dpb)
{
  mm_update_max_long_term_frame_idx(p_Dpb, 0);
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Mark all short term reference pictures unused for reference
 ************************************************************************
 */
void mm_unmark_all_short_term_for_reference (DecodedPictureBuffer *p_Dpb)
{
  unsigned int i;
  for (i=0; i<p_Dpb->ref_frames_in_buffer; i++)
  {
    unmark_for_reference(p_Dpb->fs_ref[i]);
  }
  update_ref_list(p_Dpb);
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Mark the current picture used for long term reference
 ************************************************************************
 */
void mm_mark_current_picture_long_term (DecodedPictureBuffer *p_Dpb, StorablePicture *p, int long_term_frame_idx)
{
  // remove long term pictures with same long_term_frame_idx
  if (p->structure == FRAME)
  {
    unmark_long_term_frame_for_reference_by_frame_idx(p_Dpb, long_term_frame_idx);
  }
  else
  {
    unmark_long_term_field_for_reference_by_frame_idx(p_Dpb, p->structure, long_term_frame_idx, 1, p->pic_num, 0);
  }

  p->is_long_term = 1;
  p->long_term_frame_idx = long_term_frame_idx;
}
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    Check if one of the frames/fields in frame store is used for reference
 ************************************************************************
 */
int is_used_for_reference (FrameStore* fs)
{
  if (fs->is_reference)
  {
    return 1;
  }

  if (fs->is_used == 3) // frame
  {
    if (fs->frame->used_for_reference)
    {
      return 1;
    }
  }

  if (fs->is_used & 1) // top field
  {
    if (fs->top_field)
    {
      if (fs->top_field->used_for_reference)
      {
        return 1;
      }
    }
  }

  if (fs->is_used & 2) // bottom field
  {
    if (fs->bottom_field)
    {
      if (fs->bottom_field->used_for_reference)
      {
        return 1;
      }
    }
  }
  return 0;
}
//}}}
//{{{
void get_smallest_poc (DecodedPictureBuffer *p_Dpb, int *poc,int * pos) {

  uint32 i;

  if (p_Dpb->used_size<1)
    error("Cannot determine smallest POC, DPB empty.",150);

  *pos=-1;
  *poc = INT_MAX;
  for (i = 0; i < p_Dpb->used_size; i++) {
    if ((*poc > p_Dpb->fs[i]->poc)&&(!p_Dpb->fs[i]->is_output)) {
      *poc = p_Dpb->fs[i]->poc;
      *pos = i;
      }
    }
  }
//}}}
//{{{
int remove_unused_frame_from_dpb (DecodedPictureBuffer *p_Dpb) {

  uint32 i;

  // check for frames that were already output and no longer used for reference
  for (i = 0; i < p_Dpb->used_size; i++) {
    if (p_Dpb->fs[i]->is_output && (!is_used_for_reference(p_Dpb->fs[i]))) {
      remove_frame_from_dpb(p_Dpb, i);
      return 1;
      }
    }

  return 0;
  }
//}}}

//{{{
int getDpbSize (VideoParameters *p_Vid, seq_parameter_set_rbsp_t *active_sps) {

  int pic_size_mb = (active_sps->pic_width_in_mbs_minus1 + 1) * (active_sps->pic_height_in_map_units_minus1 + 1) * (active_sps->frame_mbs_only_flag?1:2);
  int size = 0;

  switch (active_sps->level_idc) {
    //{{{
    case 0:
      // if there is no level defined, we expect experimental usage and return a DPB size of 16
      return 16;
    //}}}
    //{{{
    case 9:
      size = 396;
      break;
    //}}}
    //{{{
    case 10:
      size = 396;
      break;
    //}}}
    //{{{
    case 11:
      if (!is_FREXT_profile(active_sps->profile_idc) && (active_sps->constrained_set3_flag == 1))
        size = 396;
      else
        size = 900;
      break;
    //}}}
    //{{{
    case 12:
      size = 2376;
      break;
    //}}}
    //{{{
    case 13:
      size = 2376;
      break;
    //}}}
    //{{{
    case 20:
      size = 2376;
      break;
    //}}}
    //{{{
    case 21:
      size = 4752;
      break;
    //}}}
    //{{{
    case 22:
      size = 8100;
      break;
    //}}}
    //{{{
    case 30:
      size = 8100;
      break;
    //}}}
    //{{{
    case 31:
      size = 18000;
      break;
    //}}}
    //{{{
    case 32:
      size = 20480;
      break;
    //}}}
    //{{{
    case 40:
      size = 32768;
      break;
    //}}}
    //{{{
    case 41:
      size = 32768;
      break;
    //}}}
    //{{{
    case 42:
      size = 34816;
      break;
    //}}}
    //{{{
    case 50:
      size = 110400;
      break;
    //}}}
    //{{{
    case 51:
      size = 184320;
      break;
    //}}}
    //{{{
    case 52:
      size = 184320;
      break;
    //}}}
    case 60:
    case 61:
    //{{{
    case 62:
      size = 696320;
      break;
    //}}}
    //{{{
    default:
      error ("undefined level", 500);
      break;
    //}}}
    }

  size /= pic_size_mb;
#if MVC_EXTENSION_ENABLE
  if (p_Vid->profile_idc == MVC_HIGH || p_Vid->profile_idc == STEREO_HIGH) {
    int num_views = p_Vid->active_subset_sps->num_views_minus1+1;
    size = imin(2*size, imax(1, RoundLog2(num_views))*16)/num_views;
    }
  else
#endif
    size = imin( size, 16);

  if (active_sps->vui_parameters_present_flag && active_sps->vui_seq_parameters.bitstream_restriction_flag) {
    int size_vui;
    if ((int)active_sps->vui_seq_parameters.max_dec_frame_buffering > size)
      error ("max_dec_frame_buffering larger than MaxDpbSize", 500);

    size_vui = imax (1, active_sps->vui_seq_parameters.max_dec_frame_buffering);
#ifdef _DEBUG
    if(size_vui < size)
      printf("Warning: max_dec_frame_buffering(%d) is less than DPB size(%d) calculated from Profile/Level.\n", size_vui, size);
#endif
    size = size_vui;
    }

  return size;
  }
//}}}
//{{{
void check_num_ref (DecodedPictureBuffer *p_Dpb) {

  if ((int)(p_Dpb->ltref_frames_in_buffer + p_Dpb->ref_frames_in_buffer ) > imax(1, p_Dpb->num_ref_frames))
    error ("Max. number of reference frames exceeded. Invalid stream.", 500);
  }
//}}}
//{{{
void init_dpb (VideoParameters *p_Vid, DecodedPictureBuffer *p_Dpb, int type) {

  unsigned i;
  seq_parameter_set_rbsp_t *active_sps = p_Vid->active_sps;

  p_Dpb->p_Vid = p_Vid;
  if (p_Dpb->init_done)
    free_dpb(p_Dpb);

  p_Dpb->size = getDpbSize(p_Vid, active_sps) + p_Vid->p_Inp->dpb_plus[type==2? 1: 0];
  p_Dpb->num_ref_frames = active_sps->num_ref_frames;

#if (MVC_EXTENSION_ENABLE)
  if ((unsigned int)active_sps->max_dec_frame_buffering < active_sps->num_ref_frames)
#else
  if (p_Dpb->size < active_sps->num_ref_frames)
#endif
    error ("DPB size at specified level is smaller than the specified number of reference frames. This is not allowed.\n", 1000);

  p_Dpb->used_size = 0;
  p_Dpb->last_picture = NULL;

  p_Dpb->ref_frames_in_buffer = 0;
  p_Dpb->ltref_frames_in_buffer = 0;

  p_Dpb->fs = calloc(p_Dpb->size, sizeof (FrameStore*));
  if (NULL==p_Dpb->fs)
    no_mem_exit("init_dpb: p_Dpb->fs");

  p_Dpb->fs_ref = calloc(p_Dpb->size, sizeof (FrameStore*));
  if (NULL==p_Dpb->fs_ref)
    no_mem_exit("init_dpb: p_Dpb->fs_ref");

  p_Dpb->fs_ltref = calloc(p_Dpb->size, sizeof (FrameStore*));
  if (NULL==p_Dpb->fs_ltref)
    no_mem_exit("init_dpb: p_Dpb->fs_ltref");

#if (MVC_EXTENSION_ENABLE)
  p_Dpb->fs_ilref = calloc(1, sizeof (FrameStore*));
  if (NULL==p_Dpb->fs_ilref)
    no_mem_exit("init_dpb: p_Dpb->fs_ilref");
#endif

  for (i = 0; i < p_Dpb->size; i++) {
    p_Dpb->fs[i]       = alloc_frame_store();
    p_Dpb->fs_ref[i]   = NULL;
    p_Dpb->fs_ltref[i] = NULL;
    p_Dpb->fs[i]->layer_id = MVC_INIT_VIEW_ID;
    }
#if (MVC_EXTENSION_ENABLE)
  if (type == 2) {
    p_Dpb->fs_ilref[0] = alloc_frame_store();
    }
  else
    p_Dpb->fs_ilref[0] = NULL;
#endif

  /*
  for (i = 0; i < 6; i++)
  {
  currSlice->listX[i] = calloc(MAX_LIST_SIZE, sizeof (StorablePicture*)); // +1 for reordering
  if (NULL==currSlice->listX[i])
  no_mem_exit("init_dpb: currSlice->listX[i]");
  }
  */
  /* allocate a dummy storable picture */
  if (!p_Vid->no_reference_picture) {
    p_Vid->no_reference_picture = alloc_storable_picture (p_Vid, FRAME, p_Vid->width, p_Vid->height, p_Vid->width_cr, p_Vid->height_cr, 1);
    p_Vid->no_reference_picture->top_field    = p_Vid->no_reference_picture;
    p_Vid->no_reference_picture->bottom_field = p_Vid->no_reference_picture;
    p_Vid->no_reference_picture->frame        = p_Vid->no_reference_picture;
    }
  p_Dpb->last_output_poc = INT_MIN;

#if (MVC_EXTENSION_ENABLE)
  p_Dpb->last_output_view_id = -1;
#endif

  p_Vid->last_has_mmco_5 = 0;
  p_Dpb->init_done = 1;

  // picture error concealment
  if(p_Vid->conceal_mode !=0 && !p_Vid->last_out_fs)
    p_Vid->last_out_fs = alloc_frame_store();

  }

//}}}
//{{{
void re_init_dpb (VideoParameters *p_Vid, DecodedPictureBuffer *p_Dpb, int type)
{
  int i;
  seq_parameter_set_rbsp_t *active_sps = p_Vid->active_sps;
  int iDpbSize;

  iDpbSize = getDpbSize(p_Vid, active_sps)+p_Vid->p_Inp->dpb_plus[type==2? 1: 0];
  p_Dpb->num_ref_frames = active_sps->num_ref_frames;

  if( iDpbSize > (int)p_Dpb->size) {
#if (MVC_EXTENSION_ENABLE)
    if ((unsigned int)active_sps->max_dec_frame_buffering < active_sps->num_ref_frames)
#else
    if (p_Dpb->size < active_sps->num_ref_frames)
#endif
      error ("DPB size at specified level is smaller than the specified number of reference frames. This is not allowed.\n", 1000);

    p_Dpb->fs = realloc(p_Dpb->fs, iDpbSize * sizeof (FrameStore*));
    if (NULL==p_Dpb->fs)
      no_mem_exit("re_init_dpb: p_Dpb->fs");

    p_Dpb->fs_ref = realloc(p_Dpb->fs_ref, iDpbSize * sizeof (FrameStore*));
    if (NULL==p_Dpb->fs_ref)
      no_mem_exit("re_init_dpb: p_Dpb->fs_ref");

    p_Dpb->fs_ltref = realloc(p_Dpb->fs_ltref, iDpbSize * sizeof (FrameStore*));
    if (NULL==p_Dpb->fs_ltref)
      no_mem_exit("re_init_dpb: p_Dpb->fs_ltref");

#if (MVC_EXTENSION_ENABLE)
    if(!p_Dpb->fs_ilref) {
      p_Dpb->fs_ilref = calloc(1, sizeof (FrameStore*));
      if (NULL==p_Dpb->fs_ilref)
        no_mem_exit("init_dpb: p_Dpb->fs_ilref");
    }
#endif

    for (i = p_Dpb->size; i < iDpbSize; i++) {
      p_Dpb->fs[i]       = alloc_frame_store();
      p_Dpb->fs_ref[i]   = NULL;
      p_Dpb->fs_ltref[i] = NULL;
    }

#if (MVC_EXTENSION_ENABLE)
  if (type == 2 && !p_Dpb->fs_ilref[0]) {
    p_Dpb->fs_ilref[0] = alloc_frame_store();
  }
  else
    p_Dpb->fs_ilref[0] = NULL;
#endif

    p_Dpb->size = iDpbSize;
    p_Dpb->last_output_poc = INT_MIN;
#if (MVC_EXTENSION_ENABLE)
    p_Dpb->last_output_view_id = -1;
#endif
    p_Dpb->init_done = 1;
    }
  }
//}}}
//{{{
void free_dpb (DecodedPictureBuffer *p_Dpb) {

  VideoParameters *p_Vid = p_Dpb->p_Vid;
  unsigned i;
  if (p_Dpb->fs) {
    for (i = 0; i < p_Dpb->size; i++)
      free_frame_store (p_Dpb->fs[i]);
    free (p_Dpb->fs);
    p_Dpb->fs = NULL;
    }

  if (p_Dpb->fs_ref)
    free (p_Dpb->fs_ref);
  if (p_Dpb->fs_ltref)
    free (p_Dpb->fs_ltref);

#if (MVC_EXTENSION_ENABLE)
  if (p_Dpb->fs_ilref) {
    for (i = 0; i < 1; i++)
      free_frame_store (p_Dpb->fs_ilref[i]);
    free (p_Dpb->fs_ilref);
    p_Dpb->fs_ilref = NULL;
    }

  p_Dpb->last_output_view_id = -1;
#endif

  p_Dpb->last_output_poc = INT_MIN;

  p_Dpb->init_done = 0;

  // picture error concealment
  if (p_Vid->conceal_mode != 0 || p_Vid->last_out_fs)
      free_frame_store (p_Vid->last_out_fs);

  if (p_Vid->no_reference_picture) {
    free_storable_picture (p_Vid->no_reference_picture);
    p_Vid->no_reference_picture = NULL;
    }
  }
//}}}

//{{{
void alloc_pic_motion (PicMotionParamsOld *motion, int size_y, int size_x) {

  motion->mb_field = calloc (size_y * size_x, sizeof(byte));
  if (motion->mb_field == NULL)
    no_mem_exit("alloc_storable_picture: motion->mb_field");
  }
//}}}
//{{{
void free_pic_motion (PicMotionParamsOld *motion) {

  if (motion->mb_field) {
    free(motion->mb_field);
    motion->mb_field = NULL;
    }
  }
//}}}

//{{{
StorablePicture* alloc_storable_picture (VideoParameters *p_Vid, PictureStructure structure, int size_x, int size_y, int size_x_cr, int size_y_cr, int is_output)
{
  seq_parameter_set_rbsp_t *active_sps = p_Vid->active_sps;

  StorablePicture *s;
  int   nplane;

  //printf ("Allocating (%s) picture (x=%d, y=%d, x_cr=%d, y_cr=%d)\n", (type == FRAME)?"FRAME":(type == TOP_FIELD)?"TOP_FIELD":"BOTTOM_FIELD", size_x, size_y, size_x_cr, size_y_cr);

  s = calloc (1, sizeof(StorablePicture));
  if (NULL==s)
    no_mem_exit("alloc_storable_picture: s");

  if (structure!=FRAME)
  {
    size_y    /= 2;
    size_y_cr /= 2;
  }

  s->PicSizeInMbs = (size_x*size_y)/256;
  s->imgUV = NULL;

  get_mem2Dpel_pad (&(s->imgY), size_y, size_x, p_Vid->iLumaPadY, p_Vid->iLumaPadX);
  s->iLumaStride = size_x+2*p_Vid->iLumaPadX;
  s->iLumaExpandedHeight = size_y+2*p_Vid->iLumaPadY;

  if (active_sps->chroma_format_idc != YUV400)
  {
    get_mem3Dpel_pad(&(s->imgUV), 2, size_y_cr, size_x_cr, p_Vid->iChromaPadY, p_Vid->iChromaPadX);
  }

  s->iChromaStride =size_x_cr + 2*p_Vid->iChromaPadX;
  s->iChromaExpandedHeight = size_y_cr + 2*p_Vid->iChromaPadY;
  s->iLumaPadY   = p_Vid->iLumaPadY;
  s->iLumaPadX   = p_Vid->iLumaPadX;
  s->iChromaPadY = p_Vid->iChromaPadY;
  s->iChromaPadX = p_Vid->iChromaPadX;

  s->separate_colour_plane_flag = p_Vid->separate_colour_plane_flag;

  get_mem2Dmp     ( &s->mv_info, (size_y >> BLOCK_SHIFT), (size_x >> BLOCK_SHIFT));
  alloc_pic_motion( &s->motion , (size_y >> BLOCK_SHIFT), (size_x >> BLOCK_SHIFT));

  if( (p_Vid->separate_colour_plane_flag != 0) )
  {
    for( nplane=0; nplane<MAX_PLANE; nplane++ )
    {
      get_mem2Dmp      (&s->JVmv_info[nplane], (size_y >> BLOCK_SHIFT), (size_x >> BLOCK_SHIFT));
      alloc_pic_motion(&s->JVmotion[nplane] , (size_y >> BLOCK_SHIFT), (size_x >> BLOCK_SHIFT));
    }
  }

  s->pic_num   = 0;
  s->frame_num = 0;
  s->long_term_frame_idx = 0;
  s->long_term_pic_num   = 0;
  s->used_for_reference  = 0;
  s->is_long_term        = 0;
  s->non_existing        = 0;
  s->is_output           = 0;
  s->max_slice_id        = 0;

  s->structure=structure;

  s->size_x = size_x;
  s->size_y = size_y;
  s->size_x_cr = size_x_cr;
  s->size_y_cr = size_y_cr;
  s->size_x_m1 = size_x - 1;
  s->size_y_m1 = size_y - 1;
  s->size_x_cr_m1 = size_x_cr - 1;
  s->size_y_cr_m1 = size_y_cr - 1;

  s->top_field    = p_Vid->no_reference_picture;
  s->bottom_field = p_Vid->no_reference_picture;
  s->frame        = p_Vid->no_reference_picture;

  s->dec_ref_pic_marking_buffer = NULL;

  s->coded_frame  = 0;
  s->mb_aff_frame_flag  = 0;

  s->top_poc = s->bottom_poc = s->poc = 0;

  if(!p_Vid->active_sps->frame_mbs_only_flag && structure != FRAME)
  {
    int i, j;
    for(j = 0; j < MAX_NUM_SLICES; j++)
    {
      for (i = 0; i < 2; i++)
      {
        s->listX[j][i] = calloc(MAX_LIST_SIZE, sizeof (StorablePicture*)); // +1 for reordering
        if (NULL==s->listX[j][i])
        no_mem_exit("alloc_storable_picture: s->listX[i]");
      }
    }
  }

  return s;
}
//}}}
//{{{
void free_storable_picture (StorablePicture* p) {

  int nplane;
  if (p) {
    if (p->mv_info) {
      free_mem2Dmp(p->mv_info);
      p->mv_info = NULL;
    }
    free_pic_motion(&p->motion);

    if( (p->separate_colour_plane_flag != 0) ) {
      for( nplane=0; nplane<MAX_PLANE; nplane++ ) {
        if (p->JVmv_info[nplane]) {
          free_mem2Dmp(p->JVmv_info[nplane]);
          p->JVmv_info[nplane] = NULL;
        }
        free_pic_motion(&p->JVmotion[nplane]);
      }
    }

    if (p->imgY) {
      free_mem2Dpel_pad(p->imgY, p->iLumaPadY, p->iLumaPadX);
      p->imgY = NULL;
    }

    if (p->imgUV) {
      free_mem3Dpel_pad(p->imgUV, 2, p->iChromaPadY, p->iChromaPadX);
      p->imgUV=NULL;
    }

    {
      int i, j;
      for(j = 0; j < MAX_NUM_SLICES; j++)
      {
        for(i=0; i<2; i++) {
          if(p->listX[j][i])
          {
            free(p->listX[j][i]);
            p->listX[j][i] = NULL;
          }
        }
      }
    }
    free(p);
    p = NULL;
  }
}
//}}}

//{{{
FrameStore* alloc_frame_store() {

  FrameStore* f = calloc (1, sizeof(FrameStore));
  if (NULL == f)
    no_mem_exit("alloc_frame_store: f");

  f->is_used      = 0;
  f->is_reference = 0;
  f->is_long_term = 0;
  f->is_orig_reference = 0;
  f->is_output = 0;
  f->frame        = NULL;;
  f->top_field    = NULL;
  f->bottom_field = NULL;
  return f;
  }
//}}}
//{{{
void free_frame_store (FrameStore* f) {

  if (f) {
    if (f->frame) {
      free_storable_picture(f->frame);
      f->frame=NULL;
      }

    if (f->top_field) {
      free_storable_picture(f->top_field);
      f->top_field=NULL;
      }

    if (f->bottom_field) {
      free_storable_picture(f->bottom_field);
      f->bottom_field=NULL;
      }

    free(f);
    }
  }
//}}}
//{{{
void unmark_for_reference (FrameStore* fs) {

  if (fs->is_used & 1)
    if (fs->top_field)
      fs->top_field->used_for_reference = 0;

  if (fs->is_used & 2)
    if (fs->bottom_field)
      fs->bottom_field->used_for_reference = 0;

  if (fs->is_used == 3) {
    if (fs->top_field && fs->bottom_field) {
      fs->top_field->used_for_reference = 0;
      fs->bottom_field->used_for_reference = 0;
      }
    fs->frame->used_for_reference = 0;
    }

  fs->is_reference = 0;

  if(fs->frame)
    free_pic_motion(&fs->frame->motion);

  if (fs->top_field)
    free_pic_motion(&fs->top_field->motion);

  if (fs->bottom_field)
    free_pic_motion(&fs->bottom_field->motion);
  }
//}}}
//{{{
void unmark_for_long_term_reference (FrameStore* fs) {

  if (fs->is_used & 1) {
    if (fs->top_field) {
      fs->top_field->used_for_reference = 0;
      fs->top_field->is_long_term = 0;
      }
    }

  if (fs->is_used & 2) {
    if (fs->bottom_field) {
      fs->bottom_field->used_for_reference = 0;
      fs->bottom_field->is_long_term = 0;
      }
    }

  if (fs->is_used == 3) {
    if (fs->top_field && fs->bottom_field) {
      fs->top_field->used_for_reference = 0;
      fs->top_field->is_long_term = 0;
      fs->bottom_field->used_for_reference = 0;
      fs->bottom_field->is_long_term = 0;
      }
    fs->frame->used_for_reference = 0;
    fs->frame->is_long_term = 0;
    }

  fs->is_reference = 0;
  fs->is_long_term = 0;
  }
//}}}

//{{{
void update_pic_num (Slice *currSlice) {

  unsigned int i;
  VideoParameters *p_Vid = currSlice->p_Vid;
  DecodedPictureBuffer *p_Dpb = currSlice->p_Dpb;
  seq_parameter_set_rbsp_t *active_sps = p_Vid->active_sps;

  int add_top = 0, add_bottom = 0;
  int max_frame_num = 1 << (active_sps->log2_max_frame_num_minus4 + 4);

  if (currSlice->structure == FRAME) {
    for (i=0; i<p_Dpb->ref_frames_in_buffer; i++) {
      if ( p_Dpb->fs_ref[i]->is_used==3 ) {
        if ((p_Dpb->fs_ref[i]->frame->used_for_reference)&&(!p_Dpb->fs_ref[i]->frame->is_long_term)) {
          if( p_Dpb->fs_ref[i]->frame_num > currSlice->frame_num )
            p_Dpb->fs_ref[i]->frame_num_wrap = p_Dpb->fs_ref[i]->frame_num - max_frame_num;
          else
            p_Dpb->fs_ref[i]->frame_num_wrap = p_Dpb->fs_ref[i]->frame_num;
          p_Dpb->fs_ref[i]->frame->pic_num = p_Dpb->fs_ref[i]->frame_num_wrap;
          }
        }
      }

    // update long_term_pic_num
    for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
      if (p_Dpb->fs_ltref[i]->is_used==3) {
        if (p_Dpb->fs_ltref[i]->frame->is_long_term)
          p_Dpb->fs_ltref[i]->frame->long_term_pic_num = p_Dpb->fs_ltref[i]->frame->long_term_frame_idx;
        }
      }
    }
  else {
    if (currSlice->structure == TOP_FIELD) {
      add_top    = 1;
      add_bottom = 0;
      }
    else {
      add_top    = 0;
      add_bottom = 1;
      }

    for (i=0; i<p_Dpb->ref_frames_in_buffer; i++) {
      if (p_Dpb->fs_ref[i]->is_reference) {
        if( p_Dpb->fs_ref[i]->frame_num > currSlice->frame_num )
          p_Dpb->fs_ref[i]->frame_num_wrap = p_Dpb->fs_ref[i]->frame_num - max_frame_num;
        else
          p_Dpb->fs_ref[i]->frame_num_wrap = p_Dpb->fs_ref[i]->frame_num;
        if (p_Dpb->fs_ref[i]->is_reference & 1)
          p_Dpb->fs_ref[i]->top_field->pic_num = (2 * p_Dpb->fs_ref[i]->frame_num_wrap) + add_top;
        if (p_Dpb->fs_ref[i]->is_reference & 2)
          p_Dpb->fs_ref[i]->bottom_field->pic_num = (2 * p_Dpb->fs_ref[i]->frame_num_wrap) + add_bottom;
        }
      }

    // update long_term_pic_num
    for (i=0; i<p_Dpb->ltref_frames_in_buffer; i++) {
      if (p_Dpb->fs_ltref[i]->is_long_term & 1)
        p_Dpb->fs_ltref[i]->top_field->long_term_pic_num = 2 * p_Dpb->fs_ltref[i]->top_field->long_term_frame_idx + add_top;
      if (p_Dpb->fs_ltref[i]->is_long_term & 2)
        p_Dpb->fs_ltref[i]->bottom_field->long_term_pic_num = 2 * p_Dpb->fs_ltref[i]->bottom_field->long_term_frame_idx + add_bottom;
      }
    }
  }
//}}}
//{{{
void init_lists_i_slice (Slice *currSlice) {


#if (MVC_EXTENSION_ENABLE)
  currSlice->listinterviewidx0 = 0;
  currSlice->listinterviewidx1 = 0;
#endif

  currSlice->listXsize[0] = 0;
  currSlice->listXsize[1] = 0;
  }
//}}}
//{{{
void init_lists_p_slice (Slice *currSlice) {

  VideoParameters *p_Vid = currSlice->p_Vid;
  DecodedPictureBuffer *p_Dpb = currSlice->p_Dpb;

  unsigned int i;

  int list0idx = 0;
  int listltidx = 0;

  FrameStore **fs_list0;
  FrameStore **fs_listlt;

#if (MVC_EXTENSION_ENABLE)
  currSlice->listinterviewidx0 = 0;
  currSlice->listinterviewidx1 = 0;
#endif

  if (currSlice->structure == FRAME) {
    for (i=0; i<p_Dpb->ref_frames_in_buffer; i++) {
      if (p_Dpb->fs_ref[i]->is_used==3) {
        if ((p_Dpb->fs_ref[i]->frame->used_for_reference) && (!p_Dpb->fs_ref[i]->frame->is_long_term))
          currSlice->listX[0][list0idx++] = p_Dpb->fs_ref[i]->frame;
      }
    }
    // order list 0 by PicNum
    qsort((void *)currSlice->listX[0], list0idx, sizeof(StorablePicture*), compare_pic_by_pic_num_desc);
    currSlice->listXsize[0] = (char) list0idx;
    //printf("listX[0] (PicNum): "); for (i=0; i<list0idx; i++){printf ("%d  ", currSlice->listX[0][i]->pic_num);} printf("\n");

    // long term handling
    for (i=0; i<p_Dpb->ltref_frames_in_buffer; i++)
      if (p_Dpb->fs_ltref[i]->is_used==3)
        if (p_Dpb->fs_ltref[i]->frame->is_long_term)
          currSlice->listX[0][list0idx++] = p_Dpb->fs_ltref[i]->frame;
    qsort((void *)&currSlice->listX[0][(short) currSlice->listXsize[0]], list0idx - currSlice->listXsize[0], sizeof(StorablePicture*), compare_pic_by_lt_pic_num_asc);
    currSlice->listXsize[0] = (char) list0idx;
    }
  else {
    fs_list0 = calloc(p_Dpb->size, sizeof (FrameStore*));
    if (NULL==fs_list0)
      no_mem_exit("init_lists: fs_list0");
    fs_listlt = calloc(p_Dpb->size, sizeof (FrameStore*));
    if (NULL==fs_listlt)
      no_mem_exit("init_lists: fs_listlt");

    for (i=0; i<p_Dpb->ref_frames_in_buffer; i++)
      if (p_Dpb->fs_ref[i]->is_reference)
        fs_list0[list0idx++] = p_Dpb->fs_ref[i];

    qsort((void *)fs_list0, list0idx, sizeof(FrameStore*), compare_fs_by_frame_num_desc);

    //printf("fs_list0 (FrameNum): "); for (i=0; i<list0idx; i++){printf ("%d  ", fs_list0[i]->frame_num_wrap);} printf("\n");

    currSlice->listXsize[0] = 0;
    gen_pic_list_from_frame_list(currSlice->structure, fs_list0, list0idx, currSlice->listX[0], &currSlice->listXsize[0], 0);

    //printf("listX[0] (PicNum): "); for (i=0; i < currSlice->listXsize[0]; i++){printf ("%d  ", currSlice->listX[0][i]->pic_num);} printf("\n");

    // long term handling
    for (i=0; i<p_Dpb->ltref_frames_in_buffer; i++)
      fs_listlt[listltidx++]=p_Dpb->fs_ltref[i];

    qsort((void *)fs_listlt, listltidx, sizeof(FrameStore*), compare_fs_by_lt_pic_idx_asc);

    gen_pic_list_from_frame_list(currSlice->structure, fs_listlt, listltidx, currSlice->listX[0], &currSlice->listXsize[0], 1);

    free(fs_list0);
    free(fs_listlt);
    }
  currSlice->listXsize[1] = 0;

  // set max size
  currSlice->listXsize[0] = (char) imin (currSlice->listXsize[0], currSlice->num_ref_idx_active[LIST_0]);
  currSlice->listXsize[1] = (char) imin (currSlice->listXsize[1], currSlice->num_ref_idx_active[LIST_1]);

  // set the unused list entries to NULL
  for (i = currSlice->listXsize[0]; i< (MAX_LIST_SIZE) ; i++)
    currSlice->listX[0][i] = p_Vid->no_reference_picture;
  for (i = currSlice->listXsize[1]; i< (MAX_LIST_SIZE) ; i++)
    currSlice->listX[1][i] = p_Vid->no_reference_picture;
  }
//}}}
//{{{
void init_lists_b_slice (Slice *currSlice) {

  VideoParameters *p_Vid = currSlice->p_Vid;
  DecodedPictureBuffer *p_Dpb = currSlice->p_Dpb;

  unsigned int i;
  int j;

  int list0idx = 0;
  int list0idx_1 = 0;
  int listltidx = 0;

  FrameStore **fs_list0;
  FrameStore **fs_list1;
  FrameStore **fs_listlt;

#if (MVC_EXTENSION_ENABLE)
  currSlice->listinterviewidx0 = 0;
  currSlice->listinterviewidx1 = 0;
#endif

  {
    // B-Slice
    if (currSlice->structure == FRAME) {
      for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++)
        if (p_Dpb->fs_ref[i]->is_used==3)
          if ((p_Dpb->fs_ref[i]->frame->used_for_reference)&&(!p_Dpb->fs_ref[i]->frame->is_long_term))
            if (currSlice->framepoc >= p_Dpb->fs_ref[i]->frame->poc) //!KS use >= for error concealment
              currSlice->listX[0][list0idx++] = p_Dpb->fs_ref[i]->frame;
      qsort((void *)currSlice->listX[0], list0idx, sizeof(StorablePicture*), compare_pic_by_poc_desc);

      //get the backward reference picture (POC>current POC) in list0;
      list0idx_1 = list0idx;
      for (i=0; i<p_Dpb->ref_frames_in_buffer; i++)
        if (p_Dpb->fs_ref[i]->is_used==3)
          if ((p_Dpb->fs_ref[i]->frame->used_for_reference)&&(!p_Dpb->fs_ref[i]->frame->is_long_term))
            if (currSlice->framepoc < p_Dpb->fs_ref[i]->frame->poc)
              currSlice->listX[0][list0idx++] = p_Dpb->fs_ref[i]->frame;
      qsort((void *)&currSlice->listX[0][list0idx_1], list0idx-list0idx_1, sizeof(StorablePicture*), compare_pic_by_poc_asc);

      for (j=0; j<list0idx_1; j++)
        currSlice->listX[1][list0idx-list0idx_1+j]=currSlice->listX[0][j];
      for (j=list0idx_1; j<list0idx; j++)
        currSlice->listX[1][j-list0idx_1]=currSlice->listX[0][j];

      currSlice->listXsize[0] = currSlice->listXsize[1] = (char) list0idx;

      //printf("listX[0] (PicNum): "); for (i=0; i<currSlice->listXsize[0]; i++){printf ("%d  ", currSlice->listX[0][i]->pic_num);} printf("\n");
      //printf("listX[1] (PicNum): "); for (i=0; i<currSlice->listXsize[1]; i++){printf ("%d  ", currSlice->listX[1][i]->pic_num);} printf("\n");
      //printf("currSlice->listX[0] currPoc=%d (Poc): ", p_Vid->framepoc); for (i=0; i<currSlice->listXsize[0]; i++){printf ("%d  ", currSlice->listX[0][i]->poc);} printf("\n");
      //printf("currSlice->listX[1] currPoc=%d (Poc): ", p_Vid->framepoc); for (i=0; i<currSlice->listXsize[1]; i++){printf ("%d  ", currSlice->listX[1][i]->poc);} printf("\n");

      // long term handling
      for (i=0; i<p_Dpb->ltref_frames_in_buffer; i++) {
        if (p_Dpb->fs_ltref[i]->is_used==3) {
          if (p_Dpb->fs_ltref[i]->frame->is_long_term) {
            currSlice->listX[0][list0idx]   = p_Dpb->fs_ltref[i]->frame;
            currSlice->listX[1][list0idx++] = p_Dpb->fs_ltref[i]->frame;
            }
          }
        }
      qsort((void *)&currSlice->listX[0][(short) currSlice->listXsize[0]], list0idx - currSlice->listXsize[0], sizeof(StorablePicture*), compare_pic_by_lt_pic_num_asc);
      qsort((void *)&currSlice->listX[1][(short) currSlice->listXsize[0]], list0idx - currSlice->listXsize[0], sizeof(StorablePicture*), compare_pic_by_lt_pic_num_asc);
      currSlice->listXsize[0] = currSlice->listXsize[1] = (char) list0idx;
      }
    else {
      fs_list0 = calloc(p_Dpb->size, sizeof (FrameStore*));
      if (NULL==fs_list0)
        no_mem_exit("init_lists: fs_list0");
      fs_list1 = calloc(p_Dpb->size, sizeof (FrameStore*));
      if (NULL==fs_list1)
        no_mem_exit("init_lists: fs_list1");
      fs_listlt = calloc(p_Dpb->size, sizeof (FrameStore*));
      if (NULL==fs_listlt)
        no_mem_exit("init_lists: fs_listlt");

      currSlice->listXsize[0] = 0;
      currSlice->listXsize[1] = 1;

      for (i=0; i<p_Dpb->ref_frames_in_buffer; i++)
        if (p_Dpb->fs_ref[i]->is_used)
          if (currSlice->ThisPOC >= p_Dpb->fs_ref[i]->poc)
            fs_list0[list0idx++] = p_Dpb->fs_ref[i];
      qsort((void *)fs_list0, list0idx, sizeof(FrameStore*), compare_fs_by_poc_desc);

      list0idx_1 = list0idx;
      for (i=0; i<p_Dpb->ref_frames_in_buffer; i++)
        if (p_Dpb->fs_ref[i]->is_used)
          if (currSlice->ThisPOC < p_Dpb->fs_ref[i]->poc)
            fs_list0[list0idx++] = p_Dpb->fs_ref[i];
      qsort((void *)&fs_list0[list0idx_1], list0idx-list0idx_1, sizeof(FrameStore*), compare_fs_by_poc_asc);

      for (j=0; j<list0idx_1; j++)
        fs_list1[list0idx-list0idx_1+j]=fs_list0[j];
      for (j=list0idx_1; j<list0idx; j++)
        fs_list1[j-list0idx_1]=fs_list0[j];

      //printf("fs_list0 currPoc=%d (Poc): ", currSlice->ThisPOC); for (i=0; i<list0idx; i++){printf ("%d  ", fs_list0[i]->poc);} printf("\n");
      //printf("fs_list1 currPoc=%d (Poc): ", currSlice->ThisPOC); for (i=0; i<list0idx; i++){printf ("%d  ", fs_list1[i]->poc);} printf("\n");

      currSlice->listXsize[0] = 0;
      currSlice->listXsize[1] = 0;
      gen_pic_list_from_frame_list(currSlice->structure, fs_list0, list0idx, currSlice->listX[0], &currSlice->listXsize[0], 0);
      gen_pic_list_from_frame_list(currSlice->structure, fs_list1, list0idx, currSlice->listX[1], &currSlice->listXsize[1], 0);

      //printf("currSlice->listX[0] currPoc=%d (Poc): ", p_Vid->framepoc); for (i=0; i<currSlice->listXsize[0]; i++){printf ("%d  ", currSlice->listX[0][i]->poc);} printf("\n");
      //printf("currSlice->listX[1] currPoc=%d (Poc): ", p_Vid->framepoc); for (i=0; i<currSlice->listXsize[1]; i++){printf ("%d  ", currSlice->listX[1][i]->poc);} printf("\n");

      // long term handling
      for (i=0; i<p_Dpb->ltref_frames_in_buffer; i++)
        fs_listlt[listltidx++]=p_Dpb->fs_ltref[i];

      qsort((void *)fs_listlt, listltidx, sizeof(FrameStore*), compare_fs_by_lt_pic_idx_asc);

      gen_pic_list_from_frame_list(currSlice->structure, fs_listlt, listltidx, currSlice->listX[0], &currSlice->listXsize[0], 1);
      gen_pic_list_from_frame_list(currSlice->structure, fs_listlt, listltidx, currSlice->listX[1], &currSlice->listXsize[1], 1);

      free(fs_list0);
      free(fs_list1);
      free(fs_listlt);
      }
    }

  if ((currSlice->listXsize[0] == currSlice->listXsize[1]) && (currSlice->listXsize[0] > 1))
  {
    // check if lists are identical, if yes swap first two elements of currSlice->listX[1]
    int diff=0;
    for (j = 0; j< currSlice->listXsize[0]; j++) {
      if (currSlice->listX[0][j] != currSlice->listX[1][j]) {
        diff = 1;
        break;
        }
      }
    if (!diff) {
      StorablePicture *tmp_s = currSlice->listX[1][0];
      currSlice->listX[1][0]=currSlice->listX[1][1];
      currSlice->listX[1][1]=tmp_s;
      }
    }

  // set max size
  currSlice->listXsize[0] = (char) imin (currSlice->listXsize[0], currSlice->num_ref_idx_active[LIST_0]);
  currSlice->listXsize[1] = (char) imin (currSlice->listXsize[1], currSlice->num_ref_idx_active[LIST_1]);

  // set the unused list entries to NULL
  for (i=currSlice->listXsize[0]; i< (MAX_LIST_SIZE) ; i++)
    currSlice->listX[0][i] = p_Vid->no_reference_picture;
  for (i=currSlice->listXsize[1]; i< (MAX_LIST_SIZE) ; i++)
    currSlice->listX[1][i] = p_Vid->no_reference_picture;
  }
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Initialize listX[2..5] from lists 0 and 1
 *    listX[2]: list0 for current_field==top
 *    listX[3]: list1 for current_field==top
 *    listX[4]: list0 for current_field==bottom
 *    listX[5]: list1 for current_field==bottom
 *
 ************************************************************************
 */
void init_mbaff_lists (VideoParameters *p_Vid, Slice *currSlice)
{
  unsigned j;
  int i;

  for (i=2;i<6;i++) {
    for (j=0; j<MAX_LIST_SIZE; j++)
      currSlice->listX[i][j] = p_Vid->no_reference_picture;
    currSlice->listXsize[i]=0;
    }

  for (i = 0; i < currSlice->listXsize[0]; i++) {
    currSlice->listX[2][2*i  ] = currSlice->listX[0][i]->top_field;
    currSlice->listX[2][2*i+1] = currSlice->listX[0][i]->bottom_field;
    currSlice->listX[4][2*i  ] = currSlice->listX[0][i]->bottom_field;
    currSlice->listX[4][2*i+1] = currSlice->listX[0][i]->top_field;
    }
  currSlice->listXsize[2] = currSlice->listXsize[4] = currSlice->listXsize[0] * 2;

  for (i = 0; i < currSlice->listXsize[1]; i++) {
    currSlice->listX[3][2*i  ] = currSlice->listX[1][i]->top_field;
    currSlice->listX[3][2*i+1] = currSlice->listX[1][i]->bottom_field;
    currSlice->listX[5][2*i  ] = currSlice->listX[1][i]->bottom_field;
    currSlice->listX[5][2*i+1] = currSlice->listX[1][i]->top_field;
    }
  currSlice->listXsize[3] = currSlice->listXsize[5] = currSlice->listXsize[1] * 2;
  }
//}}}
//{{{
StorablePicture* get_short_term_pic (Slice *currSlice, DecodedPictureBuffer *p_Dpb, int picNum) {

  unsigned i;

  for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
    if (currSlice->structure == FRAME) {
      if (p_Dpb->fs_ref[i]->is_reference == 3)
        if ((!p_Dpb->fs_ref[i]->frame->is_long_term)&&(p_Dpb->fs_ref[i]->frame->pic_num == picNum))
          return p_Dpb->fs_ref[i]->frame;
      }
    else {
      if (p_Dpb->fs_ref[i]->is_reference & 1)
        if ((!p_Dpb->fs_ref[i]->top_field->is_long_term)&&(p_Dpb->fs_ref[i]->top_field->pic_num == picNum))
          return p_Dpb->fs_ref[i]->top_field;
      if (p_Dpb->fs_ref[i]->is_reference & 2)
        if ((!p_Dpb->fs_ref[i]->bottom_field->is_long_term)&&(p_Dpb->fs_ref[i]->bottom_field->pic_num == picNum))
          return p_Dpb->fs_ref[i]->bottom_field;
      }
    }

  return currSlice->p_Vid->no_reference_picture;
  }
//}}}

//{{{
static void reorder_short_term (Slice *currSlice, int cur_list, int num_ref_idx_lX_active_minus1, int picNumLX, int *refIdxLX) {
  StorablePicture **RefPicListX = currSlice->listX[cur_list];
  int cIdx, nIdx;

  StorablePicture *picLX;

  picLX = get_short_term_pic(currSlice, currSlice->p_Dpb, picNumLX);

  for( cIdx = num_ref_idx_lX_active_minus1+1; cIdx > *refIdxLX; cIdx-- )
    RefPicListX[ cIdx ] = RefPicListX[ cIdx - 1];

  RefPicListX[ (*refIdxLX)++ ] = picLX;

  nIdx = *refIdxLX;

  for( cIdx = *refIdxLX; cIdx <= num_ref_idx_lX_active_minus1+1; cIdx++ )
  {
    if (RefPicListX[ cIdx ])
      if( (RefPicListX[ cIdx ]->is_long_term ) ||  (RefPicListX[ cIdx ]->pic_num != picNumLX ))
        RefPicListX[ nIdx++ ] = RefPicListX[ cIdx ];
  }
}
//}}}
//{{{
static void reorder_long_term (Slice *currSlice, StorablePicture **RefPicListX, int num_ref_idx_lX_active_minus1, int LongTermPicNum, int *refIdxLX)
{
  int cIdx, nIdx;

  StorablePicture *picLX;

  picLX = get_long_term_pic(currSlice, currSlice->p_Dpb, LongTermPicNum);

  for( cIdx = num_ref_idx_lX_active_minus1+1; cIdx > *refIdxLX; cIdx-- )
    RefPicListX[ cIdx ] = RefPicListX[ cIdx - 1];

  RefPicListX[ (*refIdxLX)++ ] = picLX;

  nIdx = *refIdxLX;

  for( cIdx = *refIdxLX; cIdx <= num_ref_idx_lX_active_minus1+1; cIdx++ )
  {
    if (RefPicListX[ cIdx ])
    {
      if( (!RefPicListX[ cIdx ]->is_long_term ) ||  (RefPicListX[ cIdx ]->long_term_pic_num != LongTermPicNum ))
        RefPicListX[ nIdx++ ] = RefPicListX[ cIdx ];
    }
  }
}
//}}}

//{{{
static void sliding_window_memory_management (DecodedPictureBuffer *p_Dpb, StorablePicture* p) {

  uint32 i;

  assert (!p->idr_flag);

  // if this is a reference pic with sliding window, unmark first ref frame
  if (p_Dpb->ref_frames_in_buffer == imax(1, p_Dpb->num_ref_frames) - p_Dpb->ltref_frames_in_buffer) {
    for (i = 0; i < p_Dpb->used_size; i++) {
      if (p_Dpb->fs[i]->is_reference && (!(p_Dpb->fs[i]->is_long_term))) {
        unmark_for_reference(p_Dpb->fs[i]);
        update_ref_list(p_Dpb);
        break;
        }
      }
    }

  p->is_long_term = 0;
  }
//}}}
//{{{
static void adaptive_memory_management (DecodedPictureBuffer *p_Dpb, StorablePicture* p) {

  DecRefPicMarking_t *tmp_drpm;
  VideoParameters *p_Vid = p_Dpb->p_Vid;

  p_Vid->last_has_mmco_5 = 0;

  assert (!p->idr_flag);
  assert (p->adaptive_ref_pic_buffering_flag);

  while (p->dec_ref_pic_marking_buffer) {
    tmp_drpm = p->dec_ref_pic_marking_buffer;
    switch (tmp_drpm->memory_management_control_operation) {
      //{{{
      case 0:
        if (tmp_drpm->Next != NULL)
        {
          error ("memory_management_control_operation = 0 not last operation in buffer", 500);
        }
        break;
      //}}}
      //{{{
      case 1:
        mm_unmark_short_term_for_reference(p_Dpb, p, tmp_drpm->difference_of_pic_nums_minus1);
        update_ref_list(p_Dpb);
        break;
      //}}}
      //{{{
      case 2:
        mm_unmark_long_term_for_reference(p_Dpb, p, tmp_drpm->long_term_pic_num);
        update_ltref_list(p_Dpb);
        break;
      //}}}
      //{{{
      case 3:
        mm_assign_long_term_frame_idx(p_Dpb, p, tmp_drpm->difference_of_pic_nums_minus1, tmp_drpm->long_term_frame_idx);
        update_ref_list(p_Dpb);
        update_ltref_list(p_Dpb);
        break;
      //}}}
      //{{{
      case 4:
        mm_update_max_long_term_frame_idx (p_Dpb, tmp_drpm->max_long_term_frame_idx_plus1);
        update_ltref_list(p_Dpb);
        break;
      //}}}
      //{{{
      case 5:
        mm_unmark_all_short_term_for_reference(p_Dpb);
        mm_unmark_all_long_term_for_reference(p_Dpb);
        p_Vid->last_has_mmco_5 = 1;
        break;
      //}}}
      //{{{
      case 6:
        mm_mark_current_picture_long_term(p_Dpb, p, tmp_drpm->long_term_frame_idx);
        check_num_ref(p_Dpb);
        break;
      //}}}
      //{{{
      default:
        error ("invalid memory_management_control_operation in buffer", 500);
      //}}}
      }
    p->dec_ref_pic_marking_buffer = tmp_drpm->Next;
    free (tmp_drpm);
    }

  if ( p_Vid->last_has_mmco_5 ) {
    p->pic_num = p->frame_num = 0;
    switch (p->structure) {
      //{{{
      case TOP_FIELD:
        {
          //p->poc = p->top_poc = p_Vid->toppoc =0;
          p->poc = p->top_poc = 0;
          break;
        }
      //}}}
      //{{{
      case BOTTOM_FIELD:
        {
          //p->poc = p->bottom_poc = p_Vid->bottompoc = 0;
          p->poc = p->bottom_poc = 0;
          break;
        }
      //}}}
      //{{{
      case FRAME:
        {
          p->top_poc    -= p->poc;
          p->bottom_poc -= p->poc;

          //p_Vid->toppoc = p->top_poc;
          //p_Vid->bottompoc = p->bottom_poc;

          p->poc = imin (p->top_poc, p->bottom_poc);
          //p_Vid->framepoc = p->poc;
          break;
        }
      //}}}
      }

    flush_dpb(p_Vid->p_Dpb_layer[0]);
    }
  }
//}}}
//{{{
void idr_memory_management (DecodedPictureBuffer *p_Dpb, StorablePicture* p) {

  uint32 i;

  if (p->no_output_of_prior_pics_flag) {
    // free all stored pictures
    for (i=0; i<p_Dpb->used_size; i++) {
      // reset all reference settings
      free_frame_store (p_Dpb->fs[i]);
      p_Dpb->fs[i] = alloc_frame_store();
      }
    for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++)
      p_Dpb->fs_ref[i]=NULL;
    for (i=0; i<p_Dpb->ltref_frames_in_buffer; i++)
      p_Dpb->fs_ltref[i]=NULL;
    p_Dpb->used_size=0;
    }
  else
    flush_dpb (p_Dpb);

  p_Dpb->last_picture = NULL;

  update_ref_list (p_Dpb);
  update_ltref_list (p_Dpb);
  p_Dpb->last_output_poc = INT_MIN;

  if (p->long_term_reference_flag) {
    p_Dpb->max_long_term_pic_idx = 0;
    p->is_long_term           = 1;
    p->long_term_frame_idx    = 0;
    }
  else {
    p_Dpb->max_long_term_pic_idx = -1;
    p->is_long_term           = 0;
    }

#if (MVC_EXTENSION_ENABLE)
  p_Dpb->last_output_view_id = -1;
#endif
  }
//}}}

//{{{
void store_picture_in_dpb (DecodedPictureBuffer *p_Dpb, StorablePicture* p) {

  VideoParameters *p_Vid = p_Dpb->p_Vid;
  int poc, pos;
  // picture error concealment

  // diagnostics
  //printf ("Storing (%s) non-ref pic with frame_num #%d\n",
  //        (p->type == FRAME)?"FRAME":(p->type == TOP_FIELD)?"TOP_FIELD":"BOTTOM_FIELD", p->pic_num);
  // if frame, check for new store,
  assert (p!=NULL);

  p_Vid->last_has_mmco_5 = 0;
  p_Vid->last_pic_bottom_field = (p->structure == BOTTOM_FIELD);

  if (p->idr_flag) {
    idr_memory_management (p_Dpb, p);
    // picture error concealment
    memset (p_Vid->pocs_in_dpb, 0, sizeof(int)*100);
    }
  else {
    // adaptive memory management
    if (p->used_for_reference && (p->adaptive_ref_pic_buffering_flag))
      adaptive_memory_management (p_Dpb, p);
    }

  if ((p->structure == TOP_FIELD) || (p->structure == BOTTOM_FIELD)) {
    // check for frame store with same pic_number
    if (p_Dpb->last_picture) {
      if ((int)p_Dpb->last_picture->frame_num == p->pic_num) {
        if (((p->structure==TOP_FIELD)&&(p_Dpb->last_picture->is_used==2))||
            ((p->structure==BOTTOM_FIELD)&&(p_Dpb->last_picture->is_used==1))) {
          if ((p->used_for_reference && (p_Dpb->last_picture->is_orig_reference!=0)) ||
              (!p->used_for_reference && (p_Dpb->last_picture->is_orig_reference==0))) {
            insert_picture_in_dpb (p_Vid, p_Dpb->last_picture, p);
            update_ref_list (p_Dpb);
            update_ltref_list (p_Dpb);
            dump_dpb (p_Dpb);
            p_Dpb->last_picture = NULL;
            return;
            }
          }
        }
      }
    }

  // this is a frame or a field which has no stored complementary field sliding window, if necessary
  if ((!p->idr_flag) &&
      (p->used_for_reference &&
      (!p->adaptive_ref_pic_buffering_flag)))
    sliding_window_memory_management (p_Dpb, p);

  // picture error concealment
  if (p_Vid->conceal_mode != 0)
    for (unsigned i = 0; i < p_Dpb->size;i++)
      if (p_Dpb->fs[i]->is_reference)
        p_Dpb->fs[i]->concealment_reference = 1;

  // first try to remove unused frames
  if (p_Dpb->used_size==p_Dpb->size) {
    // picture error concealment
    if (p_Vid->conceal_mode != 0)
      conceal_non_ref_pics(p_Dpb, 2);

    remove_unused_frame_from_dpb (p_Dpb);

    if (p_Vid->conceal_mode != 0)
      sliding_window_poc_management (p_Dpb, p);
    }

  // then output frames until one can be removed
  while (p_Dpb->used_size == p_Dpb->size) {
    // non-reference frames may be output directly
    if (!p->used_for_reference) {
      get_smallest_poc(p_Dpb, &poc, &pos);
      if ((-1==pos) || (p->poc < poc)) {
#if (_DEBUG && MVC_EXTENSION_ENABLE)
        if((p_Vid->profile_idc >= MVC_HIGH))
          printf("Display order might not be correct, %d, %d\n", p->view_id, p->poc);
#endif
#if (MVC_EXTENSION_ENABLE)
        direct_output (p_Vid, p);
#else
        direct_output (p_Vid, p);
#endif
        return;
        }
      }

    // flush a frame
    output_one_frame_from_dpb (p_Dpb);
    }

  // check for duplicate frame number in short term reference buffer
  if ((p->used_for_reference)&&(!p->is_long_term))
    for (unsigned i = 0; i < p_Dpb->ref_frames_in_buffer; i++)
      if (p_Dpb->fs_ref[i]->frame_num == p->frame_num)
        error ("duplicate frame_num in short-term reference picture buffer", 500);

  // store at end of buffer
  insert_picture_in_dpb (p_Vid, p_Dpb->fs[p_Dpb->used_size],p);

  // picture error concealment
  if (p->idr_flag)
    p_Vid->earlier_missing_poc = 0;

  if (p->structure != FRAME)
    p_Dpb->last_picture = p_Dpb->fs[p_Dpb->used_size];
  else
    p_Dpb->last_picture = NULL;

  p_Dpb->used_size++;

  if(p_Vid->conceal_mode != 0)
    p_Vid->pocs_in_dpb[p_Dpb->used_size-1] = p->poc;

  update_ref_list (p_Dpb);
  update_ltref_list (p_Dpb);
  check_num_ref (p_Dpb);
  dump_dpb (p_Dpb);
  }
//}}}
//{{{
void remove_frame_from_dpb (DecodedPictureBuffer *p_Dpb, int pos) {

  //printf ("remove frame with frame_num #%d\n", fs->frame_num);
  FrameStore* fs = p_Dpb->fs[pos];
  switch (fs->is_used) {
    case 3:
      free_storable_picture(fs->frame);
      free_storable_picture(fs->top_field);
      free_storable_picture(fs->bottom_field);
      fs->frame=NULL;
      fs->top_field=NULL;
      fs->bottom_field=NULL;
      break;

    case 2:
      free_storable_picture(fs->bottom_field);
      fs->bottom_field=NULL;
      break;

    case 1:
      free_storable_picture(fs->top_field);
      fs->top_field=NULL;
      break;

    case 0:
      break;

    default:
      error("invalid frame store type",500);
    }
  fs->is_used = 0;
  fs->is_long_term = 0;
  fs->is_reference = 0;
  fs->is_orig_reference = 0;

  // move empty framestore to end of buffer
  FrameStore* tmp = p_Dpb->fs[pos];

  for (unsigned i = pos; i < p_Dpb->used_size-1; i++)
    p_Dpb->fs[i] = p_Dpb->fs[i+1];

  p_Dpb->fs[p_Dpb->used_size-1] = tmp;
  p_Dpb->used_size--;
  }
//}}}
//{{{
void flush_dpb (DecodedPictureBuffer *p_Dpb)
{
  VideoParameters *p_Vid = p_Dpb->p_Vid;
  uint32 i;

  // diagnostics
  // printf("Flush remaining frames from the dpb. p_Dpb->size=%d, p_Dpb->used_size=%d\n",p_Dpb->size,p_Dpb->used_size);
  if(!p_Dpb->init_done)
    return;

//  if(p_Vid->conceal_mode == 0)
  if (p_Vid->conceal_mode != 0)
    conceal_non_ref_pics(p_Dpb, 0);

  // mark all frames unused
  for (i=0; i<p_Dpb->used_size; i++) {
#if MVC_EXTENSION_ENABLE
    assert( p_Dpb->fs[i]->view_id == p_Dpb->layer_id);
#endif
    unmark_for_reference (p_Dpb->fs[i]);
    }

  while (remove_unused_frame_from_dpb(p_Dpb));

  // output frames in POC order
  while (p_Dpb->used_size && output_one_frame_from_dpb(p_Dpb));

  p_Dpb->last_output_poc = INT_MIN;
  }
//}}}

//{{{
void reorder_ref_pic_list (Slice *currSlice, int cur_list) {

  int *modification_of_pic_nums_idc = currSlice->modification_of_pic_nums_idc[cur_list];
  int *abs_diff_pic_num_minus1 = currSlice->abs_diff_pic_num_minus1[cur_list];
  int *long_term_pic_idx = currSlice->long_term_pic_idx[cur_list];
  int num_ref_idx_lX_active_minus1 = currSlice->num_ref_idx_active[cur_list] - 1;

  VideoParameters *p_Vid = currSlice->p_Vid;
  int i;

  int maxPicNum, currPicNum, picNumLXNoWrap, picNumLXPred, picNumLX;
  int refIdxLX = 0;

  if (currSlice->structure==FRAME) {
    maxPicNum  = p_Vid->max_frame_num;
    currPicNum = currSlice->frame_num;
    }
  else {
    maxPicNum  = 2 * p_Vid->max_frame_num;
    currPicNum = 2 * currSlice->frame_num + 1;
    }

  picNumLXPred = currPicNum;

  for (i=0; modification_of_pic_nums_idc[i]!=3; i++) {
    if (modification_of_pic_nums_idc[i]>3)
      error ("Invalid modification_of_pic_nums_idc command", 500);

    if (modification_of_pic_nums_idc[i] < 2) {
      if (modification_of_pic_nums_idc[i] == 0) {
        if( picNumLXPred - ( abs_diff_pic_num_minus1[i] + 1 ) < 0 )
          picNumLXNoWrap = picNumLXPred - ( abs_diff_pic_num_minus1[i] + 1 ) + maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred - ( abs_diff_pic_num_minus1[i] + 1 );
        }
      else {
        // (modification_of_pic_nums_idc[i] == 1)
        if( picNumLXPred + ( abs_diff_pic_num_minus1[i] + 1 )  >=  maxPicNum )
          picNumLXNoWrap = picNumLXPred + ( abs_diff_pic_num_minus1[i] + 1 ) - maxPicNum;
        else
          picNumLXNoWrap = picNumLXPred + ( abs_diff_pic_num_minus1[i] + 1 );
        }
      picNumLXPred = picNumLXNoWrap;

      if( picNumLXNoWrap > currPicNum )
        picNumLX = picNumLXNoWrap - maxPicNum;
      else
        picNumLX = picNumLXNoWrap;

      reorder_short_term(currSlice, cur_list, num_ref_idx_lX_active_minus1, picNumLX, &refIdxLX);
      }
    else //(modification_of_pic_nums_idc[i] == 2)
      reorder_long_term(currSlice, currSlice->listX[cur_list], num_ref_idx_lX_active_minus1, long_term_pic_idx[i], &refIdxLX);
    }

  // that's a definition
  currSlice->listXsize[cur_list] = (char) (num_ref_idx_lX_active_minus1 + 1);
  }
//}}}

//{{{
void dpb_split_field (VideoParameters *p_Vid, FrameStore *fs)
{
  int i, j, ii, jj, jj4;
  int idiv,jdiv;
  int currentmb;
  int twosz16 = 2 * (fs->frame->size_x >> 4);
  StorablePicture *fs_top = NULL, *fs_btm = NULL;
  StorablePicture *frame = fs->frame;

  fs->poc = frame->poc;

  if (!frame->frame_mbs_only_flag)
  {
    fs_top = fs->top_field    = alloc_storable_picture(p_Vid, TOP_FIELD,    frame->size_x, frame->size_y, frame->size_x_cr, frame->size_y_cr, 1);
    fs_btm = fs->bottom_field = alloc_storable_picture(p_Vid, BOTTOM_FIELD, frame->size_x, frame->size_y, frame->size_x_cr, frame->size_y_cr, 1);

    for (i = 0; i < (frame->size_y >> 1); i++)
      memcpy(fs_top->imgY[i], frame->imgY[i*2], frame->size_x*sizeof(imgpel));

    for (i = 0; i< (frame->size_y_cr >> 1); i++)
    {
      memcpy(fs_top->imgUV[0][i], frame->imgUV[0][i*2], frame->size_x_cr*sizeof(imgpel));
      memcpy(fs_top->imgUV[1][i], frame->imgUV[1][i*2], frame->size_x_cr*sizeof(imgpel));
    }

    for (i = 0; i < (frame->size_y>>1); i++)
      memcpy(fs_btm->imgY[i], frame->imgY[i*2 + 1], frame->size_x*sizeof(imgpel));

    for (i = 0; i < (frame->size_y_cr>>1); i++)
    {
      memcpy(fs_btm->imgUV[0][i], frame->imgUV[0][i*2 + 1], frame->size_x_cr*sizeof(imgpel));
      memcpy(fs_btm->imgUV[1][i], frame->imgUV[1][i*2 + 1], frame->size_x_cr*sizeof(imgpel));
    }

    fs_top->poc = frame->top_poc;
    fs_btm->poc = frame->bottom_poc;
    fs_top->frame_poc =  frame->frame_poc;
    fs_top->bottom_poc = fs_btm->bottom_poc =  frame->bottom_poc;
    fs_top->top_poc    = fs_btm->top_poc    =  frame->top_poc;
    fs_btm->frame_poc  = frame->frame_poc;

    fs_top->used_for_reference = fs_btm->used_for_reference
                                      = frame->used_for_reference;
    fs_top->is_long_term = fs_btm->is_long_term
                                = frame->is_long_term;
    fs->long_term_frame_idx = fs_top->long_term_frame_idx
                            = fs_btm->long_term_frame_idx
                            = frame->long_term_frame_idx;

    fs_top->coded_frame = fs_btm->coded_frame = 1;
    fs_top->mb_aff_frame_flag = fs_btm->mb_aff_frame_flag
                        = frame->mb_aff_frame_flag;

    frame->top_field    = fs_top;
    frame->bottom_field = fs_btm;
    frame->frame         = frame;
    fs_top->bottom_field = fs_btm;
    fs_top->frame        = frame;
    fs_top->top_field = fs_top;
    fs_btm->top_field = fs_top;
    fs_btm->frame     = frame;
    fs_btm->bottom_field = fs_btm;

    fs_top->chroma_format_idc = fs_btm->chroma_format_idc = frame->chroma_format_idc;
    fs_top->iCodingType = fs_btm->iCodingType = frame->iCodingType;
    if(frame->used_for_reference)
    {
      pad_dec_picture(p_Vid, fs_top);
      pad_dec_picture(p_Vid, fs_btm);
    }
  }
  else
  {
    fs->top_field       = NULL;
    fs->bottom_field    = NULL;
    frame->top_field    = NULL;
    frame->bottom_field = NULL;
    frame->frame = frame;
  }

  if (!frame->frame_mbs_only_flag)
  {
    if (frame->mb_aff_frame_flag)
    {
      PicMotionParamsOld *frm_motion = &frame->motion;
      for (j=0 ; j< (frame->size_y >> 3); j++)
      {
        jj = (j >> 2)*8 + (j & 0x03);
        jj4 = jj + 4;
        jdiv = (j >> 1);
        for (i=0 ; i < (frame->size_x>>2); i++)
        {
          idiv = (i >> 2);

          currentmb = twosz16*(jdiv >> 1)+ (idiv)*2 + (jdiv & 0x01);
          // Assign field mvs attached to MB-Frame buffer to the proper buffer
          if (frm_motion->mb_field[currentmb])
          {
            fs_btm->mv_info[j][i].mv[LIST_0] = frame->mv_info[jj4][i].mv[LIST_0];
            fs_btm->mv_info[j][i].mv[LIST_1] = frame->mv_info[jj4][i].mv[LIST_1];
            fs_btm->mv_info[j][i].ref_idx[LIST_0] = frame->mv_info[jj4][i].ref_idx[LIST_0];
            if(fs_btm->mv_info[j][i].ref_idx[LIST_0] >=0)
              fs_btm->mv_info[j][i].ref_pic[LIST_0] = p_Vid->ppSliceList[frame->mv_info[jj4][i].slice_no]->listX[4][(short) fs_btm->mv_info[j][i].ref_idx[LIST_0]];
            else
              fs_btm->mv_info[j][i].ref_pic[LIST_0] = NULL;
            fs_btm->mv_info[j][i].ref_idx[LIST_1] = frame->mv_info[jj4][i].ref_idx[LIST_1];
            if(fs_btm->mv_info[j][i].ref_idx[LIST_1] >=0)
              fs_btm->mv_info[j][i].ref_pic[LIST_1] = p_Vid->ppSliceList[frame->mv_info[jj4][i].slice_no]->listX[5][(short) fs_btm->mv_info[j][i].ref_idx[LIST_1]];
            else
              fs_btm->mv_info[j][i].ref_pic[LIST_1] = NULL;

            fs_top->mv_info[j][i].mv[LIST_0] = frame->mv_info[jj][i].mv[LIST_0];
            fs_top->mv_info[j][i].mv[LIST_1] = frame->mv_info[jj][i].mv[LIST_1];
            fs_top->mv_info[j][i].ref_idx[LIST_0] = frame->mv_info[jj][i].ref_idx[LIST_0];
            if(fs_top->mv_info[j][i].ref_idx[LIST_0] >=0)
              fs_top->mv_info[j][i].ref_pic[LIST_0] = p_Vid->ppSliceList[frame->mv_info[jj][i].slice_no]->listX[2][(short) fs_top->mv_info[j][i].ref_idx[LIST_0]];
            else
              fs_top->mv_info[j][i].ref_pic[LIST_0] = NULL;
            fs_top->mv_info[j][i].ref_idx[LIST_1] = frame->mv_info[jj][i].ref_idx[LIST_1];
            if(fs_top->mv_info[j][i].ref_idx[LIST_1] >=0)
              fs_top->mv_info[j][i].ref_pic[LIST_1] = p_Vid->ppSliceList[frame->mv_info[jj][i].slice_no]->listX[3][(short) fs_top->mv_info[j][i].ref_idx[LIST_1]];
            else
              fs_top->mv_info[j][i].ref_pic[LIST_1] = NULL;
          }
        }
      }
    }

      //! Generate field MVs from Frame MVs
    for (j=0 ; j < (frame->size_y >> 3) ; j++)
    {
      jj = 2* RSD(j);
      jdiv = (j >> 1);
      for (i=0 ; i < (frame->size_x >> 2) ; i++)
      {
        ii = RSD(i);
        idiv = (i >> 2);

        currentmb = twosz16 * (jdiv >> 1)+ (idiv)*2 + (jdiv & 0x01);

        if (!frame->mb_aff_frame_flag  || !frame->motion.mb_field[currentmb])
        {
          fs_top->mv_info[j][i].mv[LIST_0] = fs_btm->mv_info[j][i].mv[LIST_0] = frame->mv_info[jj][ii].mv[LIST_0];
          fs_top->mv_info[j][i].mv[LIST_1] = fs_btm->mv_info[j][i].mv[LIST_1] = frame->mv_info[jj][ii].mv[LIST_1];

          // Scaling of references is done here since it will not affect spatial direct (2*0 =0)
          if (frame->mv_info[jj][ii].ref_idx[LIST_0] == -1)
          {
            fs_top->mv_info[j][i].ref_idx[LIST_0] = fs_btm->mv_info[j][i].ref_idx[LIST_0] = - 1;
            fs_top->mv_info[j][i].ref_pic[LIST_0] = fs_btm->mv_info[j][i].ref_pic[LIST_0] = NULL;
          }
          else
          {
            fs_top->mv_info[j][i].ref_idx[LIST_0] = fs_btm->mv_info[j][i].ref_idx[LIST_0] = frame->mv_info[jj][ii].ref_idx[LIST_0];
            fs_top->mv_info[j][i].ref_pic[LIST_0] = fs_btm->mv_info[j][i].ref_pic[LIST_0] = p_Vid->ppSliceList[frame->mv_info[jj][ii].slice_no]->listX[LIST_0][(short) frame->mv_info[jj][ii].ref_idx[LIST_0]];
          }

          if (frame->mv_info[jj][ii].ref_idx[LIST_1] == -1)
          {
            fs_top->mv_info[j][i].ref_idx[LIST_1] = fs_btm->mv_info[j][i].ref_idx[LIST_1] = - 1;
            fs_top->mv_info[j][i].ref_pic[LIST_1] = fs_btm->mv_info[j][i].ref_pic[LIST_1] = NULL;
          }
          else
          {
            fs_top->mv_info[j][i].ref_idx[LIST_1] = fs_btm->mv_info[j][i].ref_idx[LIST_1] = frame->mv_info[jj][ii].ref_idx[LIST_1];
            fs_top->mv_info[j][i].ref_pic[LIST_1] = fs_btm->mv_info[j][i].ref_pic[LIST_1] = p_Vid->ppSliceList[frame->mv_info[jj][ii].slice_no]->listX[LIST_1][(short) frame->mv_info[jj][ii].ref_idx[LIST_1]];
          }
        }
      }
    }
  }
}
//}}}
//{{{
void dpb_combine_field_yuv (VideoParameters *p_Vid, FrameStore *fs)
{
  int i, j;

  if (!fs->frame)
    fs->frame = alloc_storable_picture(p_Vid, FRAME, fs->top_field->size_x, fs->top_field->size_y*2, fs->top_field->size_x_cr, fs->top_field->size_y_cr*2, 1);

  for (i=0; i<fs->top_field->size_y; i++)
  {
    memcpy(fs->frame->imgY[i*2],     fs->top_field->imgY[i]   , fs->top_field->size_x * sizeof(imgpel));     // top field
    memcpy(fs->frame->imgY[i*2 + 1], fs->bottom_field->imgY[i], fs->bottom_field->size_x * sizeof(imgpel)); // bottom field
  }

  for (j = 0; j < 2; j++)
  {
    for (i=0; i<fs->top_field->size_y_cr; i++)
    {
      memcpy(fs->frame->imgUV[j][i*2],     fs->top_field->imgUV[j][i],    fs->top_field->size_x_cr*sizeof(imgpel));
      memcpy(fs->frame->imgUV[j][i*2 + 1], fs->bottom_field->imgUV[j][i], fs->bottom_field->size_x_cr*sizeof(imgpel));
    }
  }
  fs->poc=fs->frame->poc =fs->frame->frame_poc = imin (fs->top_field->poc, fs->bottom_field->poc);

  fs->bottom_field->frame_poc=fs->top_field->frame_poc=fs->frame->poc;

  fs->bottom_field->top_poc=fs->frame->top_poc=fs->top_field->poc;
  fs->top_field->bottom_poc=fs->frame->bottom_poc=fs->bottom_field->poc;

  fs->frame->used_for_reference = (fs->top_field->used_for_reference && fs->bottom_field->used_for_reference );
  fs->frame->is_long_term = (fs->top_field->is_long_term && fs->bottom_field->is_long_term );

  if (fs->frame->is_long_term)
    fs->frame->long_term_frame_idx = fs->long_term_frame_idx;

  fs->frame->top_field    = fs->top_field;
  fs->frame->bottom_field = fs->bottom_field;
  fs->frame->frame = fs->frame;

  fs->frame->coded_frame = 0;

  fs->frame->chroma_format_idc = fs->top_field->chroma_format_idc;
  fs->frame->frame_cropping_flag = fs->top_field->frame_cropping_flag;
  if (fs->frame->frame_cropping_flag)
  {
    fs->frame->frame_crop_top_offset = fs->top_field->frame_crop_top_offset;
    fs->frame->frame_crop_bottom_offset = fs->top_field->frame_crop_bottom_offset;
    fs->frame->frame_crop_left_offset = fs->top_field->frame_crop_left_offset;
    fs->frame->frame_crop_right_offset = fs->top_field->frame_crop_right_offset;
  }

  fs->top_field->frame = fs->bottom_field->frame = fs->frame;
  fs->top_field->top_field = fs->top_field;
  fs->top_field->bottom_field = fs->bottom_field;
  fs->bottom_field->top_field = fs->top_field;
  fs->bottom_field->bottom_field = fs->bottom_field;
  if(fs->top_field->used_for_reference || fs->bottom_field->used_for_reference)
    pad_dec_picture(p_Vid, fs->frame);
}
//}}}
//{{{
void dpb_combine_field (VideoParameters *p_Vid, FrameStore *fs)
{
  int i,j, jj, jj4, k, l;

  dpb_combine_field_yuv(p_Vid, fs);

  fs->frame->iCodingType = fs->top_field->iCodingType; //FIELD_CODING;
   //! Use inference flag to remap mvs/references

  //! Generate Frame parameters from field information.
  for (j=0 ; j < (fs->top_field->size_y >> 2) ; j++)
  {
    jj = (j<<1);
    jj4 = jj + 1;
    for (i=0 ; i< (fs->top_field->size_x >> 2) ; i++)
    {
      fs->frame->mv_info[jj][i].mv[LIST_0] = fs->top_field->mv_info[j][i].mv[LIST_0];
      fs->frame->mv_info[jj][i].mv[LIST_1] = fs->top_field->mv_info[j][i].mv[LIST_1];

      fs->frame->mv_info[jj][i].ref_idx[LIST_0] = fs->top_field->mv_info[j][i].ref_idx[LIST_0];
      fs->frame->mv_info[jj][i].ref_idx[LIST_1] = fs->top_field->mv_info[j][i].ref_idx[LIST_1];

      /* bug: top field list doesnot exist.*/
      l = fs->top_field->mv_info[j][i].slice_no;
      k = fs->top_field->mv_info[j][i].ref_idx[LIST_0];
      fs->frame->mv_info[jj][i].ref_pic[LIST_0] = k>=0? fs->top_field->listX[l][LIST_0][k]: NULL;
      k = fs->top_field->mv_info[j][i].ref_idx[LIST_1];
      fs->frame->mv_info[jj][i].ref_pic[LIST_1] = k>=0? fs->top_field->listX[l][LIST_1][k]: NULL;

      //! association with id already known for fields.
      fs->frame->mv_info[jj4][i].mv[LIST_0] = fs->bottom_field->mv_info[j][i].mv[LIST_0];
      fs->frame->mv_info[jj4][i].mv[LIST_1] = fs->bottom_field->mv_info[j][i].mv[LIST_1];

      fs->frame->mv_info[jj4][i].ref_idx[LIST_0]  = fs->bottom_field->mv_info[j][i].ref_idx[LIST_0];
      fs->frame->mv_info[jj4][i].ref_idx[LIST_1]  = fs->bottom_field->mv_info[j][i].ref_idx[LIST_1];
      l = fs->bottom_field->mv_info[j][i].slice_no;

      k = fs->bottom_field->mv_info[j][i].ref_idx[LIST_0];
      fs->frame->mv_info[jj4][i].ref_pic[LIST_0] = k>=0? fs->bottom_field->listX[l][LIST_0][k]: NULL;
      k = fs->bottom_field->mv_info[j][i].ref_idx[LIST_1];
      fs->frame->mv_info[jj4][i].ref_pic[LIST_1] = k>=0? fs->bottom_field->listX[l][LIST_1][k]: NULL;
    }
  }
}
//}}}

//{{{
void alloc_ref_pic_list_reordering_buffer (Slice *currSlice)
{
  if (currSlice->slice_type != I_SLICE && currSlice->slice_type != SI_SLICE)
  {
    int size = currSlice->num_ref_idx_active[LIST_0] + 1;
    if ((currSlice->modification_of_pic_nums_idc[LIST_0] = calloc(size ,sizeof(int)))==NULL)
       no_mem_exit("alloc_ref_pic_list_reordering_buffer: modification_of_pic_nums_idc_l0");
    if ((currSlice->abs_diff_pic_num_minus1[LIST_0] = calloc(size,sizeof(int)))==NULL)
       no_mem_exit("alloc_ref_pic_list_reordering_buffer: abs_diff_pic_num_minus1_l0");
    if ((currSlice->long_term_pic_idx[LIST_0] = calloc(size,sizeof(int)))==NULL)
       no_mem_exit("alloc_ref_pic_list_reordering_buffer: long_term_pic_idx_l0");
#if (MVC_EXTENSION_ENABLE)
    if ((currSlice->abs_diff_view_idx_minus1[LIST_0] = calloc(size,sizeof(int)))==NULL)
       no_mem_exit("alloc_ref_pic_list_reordering_buffer: abs_diff_view_idx_minus1_l0");
#endif
  }
  else
  {
    currSlice->modification_of_pic_nums_idc[LIST_0] = NULL;
    currSlice->abs_diff_pic_num_minus1[LIST_0] = NULL;
    currSlice->long_term_pic_idx[LIST_0] = NULL;
#if (MVC_EXTENSION_ENABLE)
    currSlice->abs_diff_view_idx_minus1[LIST_0] = NULL;
#endif
  }

  if (currSlice->slice_type == B_SLICE)
  {
    int size = currSlice->num_ref_idx_active[LIST_1] + 1;
    if ((currSlice->modification_of_pic_nums_idc[LIST_1] = calloc(size,sizeof(int)))==NULL)
      no_mem_exit("alloc_ref_pic_list_reordering_buffer: modification_of_pic_nums_idc_l1");
    if ((currSlice->abs_diff_pic_num_minus1[LIST_1] = calloc(size,sizeof(int)))==NULL)
      no_mem_exit("alloc_ref_pic_list_reordering_buffer: abs_diff_pic_num_minus1_l1");
    if ((currSlice->long_term_pic_idx[LIST_1] = calloc(size,sizeof(int)))==NULL)
      no_mem_exit("alloc_ref_pic_list_reordering_buffer: long_term_pic_idx_l1");
#if (MVC_EXTENSION_ENABLE)
    if ((currSlice->abs_diff_view_idx_minus1[LIST_1] = calloc(size,sizeof(int)))==NULL)
      no_mem_exit("alloc_ref_pic_list_reordering_buffer: abs_diff_view_idx_minus1_l1");
#endif
  }
  else
  {
    currSlice->modification_of_pic_nums_idc[LIST_1] = NULL;
    currSlice->abs_diff_pic_num_minus1[LIST_1] = NULL;
    currSlice->long_term_pic_idx[LIST_1] = NULL;
#if (MVC_EXTENSION_ENABLE)
    currSlice->abs_diff_view_idx_minus1[LIST_1] = NULL;
#endif
  }
}
//}}}
//{{{
void free_ref_pic_list_reordering_buffer (Slice *currSlice)
{
  if (currSlice->modification_of_pic_nums_idc[LIST_0])
    free(currSlice->modification_of_pic_nums_idc[LIST_0]);
  if (currSlice->abs_diff_pic_num_minus1[LIST_0])
    free(currSlice->abs_diff_pic_num_minus1[LIST_0]);
  if (currSlice->long_term_pic_idx[LIST_0])
    free(currSlice->long_term_pic_idx[LIST_0]);

  currSlice->modification_of_pic_nums_idc[LIST_0] = NULL;
  currSlice->abs_diff_pic_num_minus1[LIST_0] = NULL;
  currSlice->long_term_pic_idx[LIST_0] = NULL;

  if (currSlice->modification_of_pic_nums_idc[LIST_1])
    free(currSlice->modification_of_pic_nums_idc[LIST_1]);
  if (currSlice->abs_diff_pic_num_minus1[LIST_1])
    free(currSlice->abs_diff_pic_num_minus1[LIST_1]);
  if (currSlice->long_term_pic_idx[LIST_1])
    free(currSlice->long_term_pic_idx[LIST_1]);

  currSlice->modification_of_pic_nums_idc[LIST_1] = NULL;
  currSlice->abs_diff_pic_num_minus1[LIST_1] = NULL;
  currSlice->long_term_pic_idx[LIST_1] = NULL;

#if (MVC_EXTENSION_ENABLE)
  if (currSlice->abs_diff_view_idx_minus1[LIST_0])
    free(currSlice->abs_diff_view_idx_minus1[LIST_0]);
  currSlice->abs_diff_view_idx_minus1[LIST_0] = NULL;
  if (currSlice->abs_diff_view_idx_minus1[LIST_1])
    free(currSlice->abs_diff_view_idx_minus1[LIST_1]);
  currSlice->abs_diff_view_idx_minus1[LIST_1] = NULL;
#endif
}
//}}}
//{{{
void fill_frame_num_gap (VideoParameters *p_Vid, Slice *currSlice) {

  seq_parameter_set_rbsp_t *active_sps = p_Vid->active_sps;

  int CurrFrameNum;
  int UnusedShortTermFrameNum;
  StorablePicture *picture = NULL;

  int tmp1 = currSlice->delta_pic_order_cnt[0];
  int tmp2 = currSlice->delta_pic_order_cnt[1];
  currSlice->delta_pic_order_cnt[0] = currSlice->delta_pic_order_cnt[1] = 0;

  printf("A gap in frame number is found, try to fill it.\n");

  UnusedShortTermFrameNum = (p_Vid->pre_frame_num + 1) % p_Vid->max_frame_num;
  CurrFrameNum = currSlice->frame_num; //p_Vid->frame_num;

  while (CurrFrameNum != UnusedShortTermFrameNum) {
    picture = alloc_storable_picture (p_Vid, FRAME, p_Vid->width, p_Vid->height, p_Vid->width_cr, p_Vid->height_cr, 1);
    picture->coded_frame = 1;
    picture->pic_num = UnusedShortTermFrameNum;
    picture->frame_num = UnusedShortTermFrameNum;
    picture->non_existing = 1;
    picture->is_output = 1;
    picture->used_for_reference = 1;
    picture->adaptive_ref_pic_buffering_flag = 0;

    currSlice->frame_num = UnusedShortTermFrameNum;
    if (active_sps->pic_order_cnt_type!=0)
      decodePOC (p_Vid, p_Vid->ppSliceList[0]);
    picture->top_poc    = currSlice->toppoc;
    picture->bottom_poc = currSlice->bottompoc;
    picture->frame_poc  = currSlice->framepoc;
    picture->poc        = currSlice->framepoc;

    store_picture_in_dpb (currSlice->p_Dpb, picture);

    picture=NULL;
    p_Vid->pre_frame_num = UnusedShortTermFrameNum;
    UnusedShortTermFrameNum = (UnusedShortTermFrameNum + 1) % p_Vid->max_frame_num;
    }

  currSlice->delta_pic_order_cnt[0] = tmp1;
  currSlice->delta_pic_order_cnt[1] = tmp2;
  currSlice->frame_num = CurrFrameNum;
  }
//}}}
//{{{
void compute_colocated (Slice *currSlice, StorablePicture **listX[6]) {

  int i, j;

  VideoParameters *p_Vid = currSlice->p_Vid;

  if (currSlice->direct_spatial_mv_pred_flag == 0) {
    for (j = 0; j < 2 + (currSlice->mb_aff_frame_flag * 4); j += 2) {
      for (i = 0; i < currSlice->listXsize[j];i++) {
        int prescale, iTRb, iTRp;

        if (j==0)
          iTRb = iClip3( -128, 127, p_Vid->dec_picture->poc - listX[LIST_0 + j][i]->poc );
        else if (j == 2)
          iTRb = iClip3( -128, 127, p_Vid->dec_picture->top_poc - listX[LIST_0 + j][i]->poc );
        else
          iTRb = iClip3( -128, 127, p_Vid->dec_picture->bottom_poc - listX[LIST_0 + j][i]->poc );

        iTRp = iClip3( -128, 127,  listX[LIST_1 + j][0]->poc - listX[LIST_0 + j][i]->poc);

        if (iTRp!=0) {
          prescale = ( 16384 + iabs( iTRp / 2 ) ) / iTRp;
          currSlice->mvscale[j][i] = iClip3( -1024, 1023, ( iTRb * prescale + 32 ) >> 6 ) ;
          }
        else
          currSlice->mvscale[j][i] = 9999;
        }
     }
    }
  }
//}}}

//{{{
static inline void copy_img_data (imgpel *out_img, imgpel *in_img, int ostride, int istride, unsigned int size_y, unsigned int size_x)
{
  unsigned int i;
  for (i = 0; i < size_y; i++) {
    memcpy (out_img, in_img, size_x);
    out_img += ostride;
    in_img += istride;
    }
  }
//}}}
//{{{
int init_img_data (VideoParameters *p_Vid, ImageData *p_ImgData, seq_parameter_set_rbsp_t *sps)
{
  InputParameters *p_Inp = p_Vid->p_Inp;
  int memory_size = 0;
  int nplane;

  // allocate memory for reference frame buffers: p_ImgData->frm_data
  p_ImgData->format           = p_Inp->output;
  p_ImgData->format.width[0]  = p_Vid->width;
  p_ImgData->format.width[1]  = p_Vid->width_cr;
  p_ImgData->format.width[2]  = p_Vid->width_cr;
  p_ImgData->format.height[0] = p_Vid->height;
  p_ImgData->format.height[1] = p_Vid->height_cr;
  p_ImgData->format.height[2] = p_Vid->height_cr;
  p_ImgData->format.yuv_format          = (ColorFormat) sps->chroma_format_idc;
  p_ImgData->format.auto_crop_bottom    = p_Inp->output.auto_crop_bottom;
  p_ImgData->format.auto_crop_right     = p_Inp->output.auto_crop_right;
  p_ImgData->format.auto_crop_bottom_cr = p_Inp->output.auto_crop_bottom_cr;
  p_ImgData->format.auto_crop_right_cr  = p_Inp->output.auto_crop_right_cr;
  p_ImgData->frm_stride[0]    = p_Vid->width;
  p_ImgData->frm_stride[1]    = p_ImgData->frm_stride[2] = p_Vid->width_cr;
  p_ImgData->top_stride[0] = p_ImgData->bot_stride[0] = p_ImgData->frm_stride[0] << 1;
  p_ImgData->top_stride[1] = p_ImgData->top_stride[2] = p_ImgData->bot_stride[1] = p_ImgData->bot_stride[2] = p_ImgData->frm_stride[1] << 1;

  if( sps->separate_colour_plane_flag ) {
    for (nplane=0; nplane < MAX_PLANE; nplane++ )
      memory_size += get_mem2Dpel (&(p_ImgData->frm_data[nplane]), p_Vid->height, p_Vid->width);
    }
  else {
    memory_size += get_mem2Dpel(&(p_ImgData->frm_data[0]), p_Vid->height, p_Vid->width);

    if (p_Vid->yuv_format != YUV400) {
      int i, j, k;
      memory_size += get_mem2Dpel(&(p_ImgData->frm_data[1]), p_Vid->height_cr, p_Vid->width_cr);
      memory_size += get_mem2Dpel(&(p_ImgData->frm_data[2]), p_Vid->height_cr, p_Vid->width_cr);
      if (sizeof(imgpel) == sizeof(unsigned char)) {
        for (k = 1; k < 3; k++)
          memset (p_ImgData->frm_data[k][0], 128, p_Vid->height_cr * p_Vid->width_cr * sizeof(imgpel));
        }
      else {
        imgpel mean_val;
        for (k = 1; k < 3; k++) {
          mean_val = (imgpel) ((p_Vid->max_pel_value_comp[k] + 1) >> 1);
          for (j = 0; j < p_Vid->height_cr; j++)
            for (i = 0; i < p_Vid->width_cr; i++)
              p_ImgData->frm_data[k][j][i] = mean_val;
          }
        }
      }
    }

  if (!p_Vid->active_sps->frame_mbs_only_flag) {
    // allocate memory for field reference frame buffers
    memory_size += init_top_bot_planes(p_ImgData->frm_data[0], p_Vid->height, &(p_ImgData->top_data[0]), &(p_ImgData->bot_data[0]));

    if (p_Vid->yuv_format != YUV400) {
      memory_size += 4*(sizeof(imgpel**));
      memory_size += init_top_bot_planes(p_ImgData->frm_data[1], p_Vid->height_cr, &(p_ImgData->top_data[1]), &(p_ImgData->bot_data[1]));
      memory_size += init_top_bot_planes(p_ImgData->frm_data[2], p_Vid->height_cr, &(p_ImgData->top_data[2]), &(p_ImgData->bot_data[2]));
      }
    }

  return memory_size;
  }
//}}}
//{{{
void free_img_data (VideoParameters *p_Vid, ImageData *p_ImgData) {

  if ( p_Vid->separate_colour_plane_flag ) {
    int nplane;
    for (nplane=0; nplane<MAX_PLANE; nplane++ ) {
      if (p_ImgData->frm_data[nplane]) {
        free_mem2Dpel(p_ImgData->frm_data[nplane]);      // free ref frame buffers
        p_ImgData->frm_data[nplane] = NULL;
        }
      }
    }

  else {
    if (p_ImgData->frm_data[0]) {
      free_mem2Dpel(p_ImgData->frm_data[0]);      // free ref frame buffers
      p_ImgData->frm_data[0] = NULL;
      }

    if (p_ImgData->format.yuv_format != YUV400) {
      if (p_ImgData->frm_data[1]) {
        free_mem2Dpel(p_ImgData->frm_data[1]);
        p_ImgData->frm_data[1] = NULL;
        }
      if (p_ImgData->frm_data[2]) {
        free_mem2Dpel(p_ImgData->frm_data[2]);
        p_ImgData->frm_data[2] = NULL;
        }
      }
    }

  if (!p_Vid->active_sps->frame_mbs_only_flag) {
    free_top_bot_planes(p_ImgData->top_data[0], p_ImgData->bot_data[0]);
    if (p_ImgData->format.yuv_format != YUV400) {
      free_top_bot_planes(p_ImgData->top_data[1], p_ImgData->bot_data[1]);
      free_top_bot_planes(p_ImgData->top_data[2], p_ImgData->bot_data[2]);
      }
    }
  }
//}}}
//{{{
void process_picture_in_dpb_s (VideoParameters *p_Vid, StorablePicture *p_pic)
{
  //InputParameters *p_Inp = p_Vid->p_Inp;
  ImageData *p_img_out = &p_Vid->tempData3;
  imgpel***  d_img;
  int i;

  if (p_Vid->tempData3.frm_data[0] == NULL)
    init_img_data( p_Vid, &(p_Vid->tempData3), p_Vid->active_sps);

  if (p_pic->structure == FRAME)
    d_img = p_img_out->frm_data;
  else //If reference picture is a field, then frm_data will actually contain field data and therefore top/bottom stride is set accordingly.
  {
    if (p_pic->structure == TOP_FIELD)
      d_img = p_img_out->top_data;
    else
      d_img = p_img_out->bot_data;
  }

  for(i=0; i<p_pic->size_y; i++)
    memcpy(d_img[0][i], p_pic->imgY[i], p_pic->size_x*sizeof(imgpel));
  if (p_Vid->yuv_format != YUV400)
  {
    for(i=0; i<p_pic->size_y_cr; i++)
      memcpy(d_img[1][i], p_pic->imgUV[0][i], p_pic->size_x_cr * sizeof(imgpel));
    for(i=0; i<p_pic->size_y_cr; i++)
      memcpy(d_img[2][i], p_pic->imgUV[1][i], p_pic->size_x_cr * sizeof(imgpel));
  }
}
//}}}

//{{{
int remove_unused_proc_pic_from_dpb (DecodedPictureBuffer *p_Dpb) {

  assert(!"The function is not available\n");
  return 0;
  }
//}}}

