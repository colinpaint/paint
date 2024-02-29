//{{{
/*!
 ************************************************************************
 * \file  nalu.c
 *
 * \brief
 *    Decoder NALU support functions
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Stephan Wenger   <stewe@cs.tu-berlin.de>
 ************************************************************************
 */
//}}}
//{{{
#include "global.h"
#include "annexb.h"
#include "nalu.h"
#include "memalloc.h"

#if (MVC_EXTENSION_ENABLE)
  #include "vlc.h"
#endif
//}}}

//{{{
static int EBSPtoRBSP (byte *streamBuffer, int end_bytepos, int begin_bytepos)
{
  int i, j, count;
  count = 0;

  if(end_bytepos < begin_bytepos)
    return end_bytepos;

  j = begin_bytepos;

  for(i = begin_bytepos; i < end_bytepos; ++i) {
    //starting from begin_bytepos to avoid header information
    //in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any byte-aligned position
    if(count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] < 0x03)
      return -1;
    if(count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] == 0x03) {
      //check the 4th byte after 0x000003, except when cabac_zero_word is used, in which case the last three bytes of this NAL unit must be 0x000003
      if((i < end_bytepos-1) && (streamBuffer[i+1] > 0x03))
        return -1;
      //if cabac_zero_word is used, the final byte of this NAL unit(0x03) is discarded, and the last two bytes of RBSP must be 0x0000
      if(i == end_bytepos-1)
        return j;

      ++i;
      count = 0;
      }

    streamBuffer[j] = streamBuffer[i];
    if(streamBuffer[i] == 0x00)
      ++count;
    else
      count = 0;
    ++j;
    }

  return j;
  }
//}}}
//{{{
static int NALUtoRBSP (NALU_t *nalu)
{
  assert (nalu != NULL);
  nalu->len = EBSPtoRBSP (nalu->buf, nalu->len, 1) ;
  return nalu->len ;
}
//}}}

//{{{
int RBSPtoSODB (byte *streamBuffer, int last_byte_pos)
{
  int ctr_bit, bitoffset;

  bitoffset = 0;
  //find trailing 1
  ctr_bit = (streamBuffer[last_byte_pos-1] & (0x01<<bitoffset));   // set up control bit

  while (ctr_bit==0) {                 // find trailing 1 bit
    ++bitoffset;
    if(bitoffset == 8) {
      if(last_byte_pos == 0)
        printf(" Panic: All zero data sequence in RBSP \n");
      assert(last_byte_pos != 0);
      --last_byte_pos;
      bitoffset = 0;
    }
    ctr_bit= streamBuffer[last_byte_pos - 1] & (0x01<<(bitoffset));
  }

  // We keep the stop bit for now
/*  if (remove_stop)
  {
    streamBuffer[last_byte_pos-1] -= (0x01<<(bitoffset));
    if(bitoffset == 7)
      return(last_byte_pos-1);
    else
      return(last_byte_pos);
  }
*/
  return(last_byte_pos);

}
//}}}

//{{{
/*!
 *************************************************************************************
 * \brief
 *    Allocates memory for a NALU
 *
 * \param buffersize
 *     size of NALU buffer
 *
 * \return
 *    pointer to a NALU
 *************************************************************************************
 */
NALU_t* AllocNALU (int buffersize)
{
  NALU_t *n;

  if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)
    no_mem_exit ("AllocNALU: n");

  n->max_size=buffersize;
  if ((n->buf = (byte*)calloc (buffersize, sizeof (byte))) == NULL)
  {
    free (n);
    no_mem_exit ("AllocNALU: n->buf");
  }

  return n;
}
//}}}
//{{{
/*!
 *************************************************************************************
 * \brief
 *    Frees a NALU
 *
 * \param n
 *    NALU to be freed
 *
 *************************************************************************************
 */
void FreeNALU (NALU_t* n)
{
  if (n != NULL)
  {
    if (n->buf != NULL)
    {
      free(n->buf);
      n->buf=NULL;
    }
    free (n);
  }
}
//}}}
//{{{
void CheckZeroByteVCL (VideoParameters* p_Vid, NALU_t* nalu)
{
  int CheckZeroByte=0;

  // This function deals only with VCL NAL units
  if (!(nalu->nal_unit_type >= NALU_TYPE_SLICE && nalu->nal_unit_type <= NALU_TYPE_IDR))
    return;

  if (p_Vid->LastAccessUnitExists)
    p_Vid->NALUCount=0;
  p_Vid->NALUCount++;

  // the first VCL NAL unit that is the first NAL unit after last VCL NAL unit indicates
  // the start of a new access unit and hence the first NAL unit of the new access unit.           (sounds like a tongue twister :-)
  if (p_Vid->NALUCount == 1)
    CheckZeroByte = 1;
  p_Vid->LastAccessUnitExists = 1;

  if (CheckZeroByte && nalu->startcodeprefix_len==3)
    printf ("warning: zero_byte shall exist\n");
    // because it is not a very serious problem, we do not exit here
   }
//}}}
//{{{
void CheckZeroByteNonVCL(VideoParameters *p_Vid, NALU_t *nalu)
{
  int CheckZeroByte=0;

  // This function deals only with non-VCL NAL units
  if (nalu->nal_unit_type>=1&&nalu->nal_unit_type<=5)
    return;

  // for SPS and PPS, zero_byte shall exist
  if (nalu->nal_unit_type==NALU_TYPE_SPS || nalu->nal_unit_type==NALU_TYPE_PPS)
    CheckZeroByte=1;

  // check the possibility of the current NALU to be the start of a new access unit, according to 7.4.1.2.3
  if (nalu->nal_unit_type==NALU_TYPE_AUD  || nalu->nal_unit_type==NALU_TYPE_SPS ||
      nalu->nal_unit_type==NALU_TYPE_PPS || nalu->nal_unit_type==NALU_TYPE_SEI ||
      (nalu->nal_unit_type>=13 && nalu->nal_unit_type<=18)) {
    if (p_Vid->LastAccessUnitExists) {
      p_Vid->LastAccessUnitExists=0;    //deliver the last access unit to decoder
      p_Vid->NALUCount=0;
      }
    }
  p_Vid->NALUCount++;

  // for the first NAL unit in an access unit, zero_byte shall exists
  if (p_Vid->NALUCount==1)
    CheckZeroByte=1;

  if (CheckZeroByte && nalu->startcodeprefix_len==3)
    printf ("Warning: zero_byte shall exist\n");
    //because it is not a very serious problem, we do not exit here
  }
//}}}
//{{{
int read_next_nalu (VideoParameters* p_Vid, NALU_t* nalu) {

  InputParameters* p_Inp = p_Vid->p_Inp;

  int ret = get_annex_b_NALU (p_Vid, nalu, p_Vid->annex_b);
  if (ret < 0) {
    snprintf (errortext, ET_SIZE, "Error while getting the NALU in file format %s, exit\n", p_Inp->FileFormat==PAR_OF_ANNEXB?"Annex B":"RTP");
    error (errortext, 601);
    }
  if (ret == 0) {
    //FreeNALU(nalu);
    return 0;
    }

  //In some cases, zero_byte shall be present. If current NALU is a VCL NALU, we can't tell
  //whether it is the first VCL NALU at this point, so only non-VCL NAL unit is checked here.
  CheckZeroByteNonVCL (p_Vid, nalu);
  ret = NALUtoRBSP (nalu);
  if (ret < 0)
    error ("Invalid startcode emulation prevention found.", 602);

  // Got a NALU
  if (nalu->forbidden_bit)
    error ("Found NALU with forbidden_bit set, bit error?", 603);

  return nalu->len;
  }

//}}}

#if (MVC_EXTENSION_ENABLE)
  //{{{
  void nal_unit_header_mvc_extension (NALUnitHeaderMVCExt_t *NaluHeaderMVCExt, Bitstream *s)
  {
    //to be implemented;
    NaluHeaderMVCExt->non_idr_flag     = read_u_v (1, "non_idr_flag",     s, &p_Dec->UsedBits);
    NaluHeaderMVCExt->priority_id      = read_u_v (6, "priority_id",      s, &p_Dec->UsedBits);
    NaluHeaderMVCExt->view_id          = read_u_v (10, "view_id",         s, &p_Dec->UsedBits);
    NaluHeaderMVCExt->temporal_id      = read_u_v (3, "temporal_id",      s, &p_Dec->UsedBits);
    NaluHeaderMVCExt->anchor_pic_flag  = read_u_v (1, "anchor_pic_flag",  s, &p_Dec->UsedBits);
    NaluHeaderMVCExt->inter_view_flag  = read_u_v (1, "inter_view_flag",  s, &p_Dec->UsedBits);
    NaluHeaderMVCExt->reserved_one_bit = read_u_v (1, "reserved_one_bit", s, &p_Dec->UsedBits);

    if(NaluHeaderMVCExt->reserved_one_bit != 1)
      printf("Nalu Header MVC Extension: reserved_one_bit is not 1!\n");
  }
  //}}}
  //{{{
  void nal_unit_header_svc_extension()
  {
    //to be implemented for Annex G;
  }
  //}}}
  //{{{
  void prefix_nal_unit_svc()
  {
    //to be implemented for Annex G;
  }
  //}}}
#endif
