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
static int EBSPtoRBSP (byte *streamBuffer, int end_bytepos, int begin_bytepos) {

  if (end_bytepos < begin_bytepos)
    return end_bytepos;

  int count = 0;
  int j = begin_bytepos;
  for (int i = begin_bytepos; i < end_bytepos; ++i) {
    //starting from begin_bytepos to avoid header information
    //in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any byte-aligned position
    if (count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] < 0x03)
      return -1;

    if (count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] == 0x03) {
      //check the 4th byte after 0x000003, except when cabac_zero_word is used, in which case the last three bytes of this NAL unit must be 0x000003
      if ((i < end_bytepos-1) && (streamBuffer[i+1] > 0x03))
        return -1;

      //if cabac_zero_word is used, the final byte of this NAL unit(0x03) is discarded, and the last two bytes of RBSP must be 0x0000
      if (i == end_bytepos-1)
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
static int NALUtoRBSP (NALU_t* nalu) {

  assert (nalu != NULL);
  nalu->len = EBSPtoRBSP (nalu->buf, nalu->len, 1) ;
  return nalu->len ;
  }
//}}}

//{{{
NALU_t* AllocNALU (int buffersize) {

  NALU_t* n;
  if ((n = (NALU_t*)calloc(1, sizeof(NALU_t))) == NULL)
    no_mem_exit ("AllocNALU: n");

  n->max_size = buffersize;
  if ((n->buf = (byte*)calloc (buffersize, sizeof (byte))) == NULL) {
    free (n);
    no_mem_exit ("AllocNALU: n->buf");
    }

  return n;
  }
//}}}
//{{{
void FreeNALU (NALU_t* n) {

  if (n != NULL) {
    if (n->buf != NULL) {
      free (n->buf);
      n->buf = NULL;
      }
    free (n);
    }
  }
//}}}

//{{{
void CheckZeroByteVCL (VideoParameters* p_Vid, NALU_t* nalu) {

  int CheckZeroByte = 0;

  // This function deals only with VCL NAL units
  if (!(nalu->nal_unit_type >= NALU_TYPE_SLICE &&
        nalu->nal_unit_type <= NALU_TYPE_IDR))
    return;

  if (p_Vid->LastAccessUnitExists)
    p_Vid->NALUCount = 0;
  p_Vid->NALUCount++;

  // the first VCL NAL unit that is the first NAL unit after last VCL NAL unit indicates
  // the start of a new access unit and hence the first NAL unit of the new access unit.
  // (sounds like a tongue twister :-)
  if (p_Vid->NALUCount == 1)
    CheckZeroByte = 1;
  p_Vid->LastAccessUnitExists = 1;

  // because it is not a very serious problem, we do not exit here
  if (CheckZeroByte && nalu->startcodeprefix_len == 3)
    printf ("warning: zero_byte shall exist\n");
   }
//}}}
//{{{
void CheckZeroByteNonVCL (VideoParameters* p_Vid, NALU_t* nalu) {

  int CheckZeroByte = 0;

  // This function deals only with non-VCL NAL units
  if (nalu->nal_unit_type >= 1 &&
      nalu->nal_unit_type <= 5)
    return;

  // for SPS and PPS, zero_byte shall exist
  if (nalu->nal_unit_type == NALU_TYPE_SPS ||
      nalu->nal_unit_type == NALU_TYPE_PPS)
    CheckZeroByte = 1;

  // check the possibility of the current NALU to be the start of a new access unit, according to 7.4.1.2.3
  if (nalu->nal_unit_type == NALU_TYPE_AUD ||
      nalu->nal_unit_type == NALU_TYPE_SPS ||
      nalu->nal_unit_type == NALU_TYPE_PPS ||
      nalu->nal_unit_type == NALU_TYPE_SEI ||
      (nalu->nal_unit_type >= 13 && nalu->nal_unit_type <= 18)) {
    if (p_Vid->LastAccessUnitExists) {
      // deliver the last access unit to decoder
      p_Vid->LastAccessUnitExists = 0;
      p_Vid->NALUCount = 0;
      }
    }
  p_Vid->NALUCount++;

  // for the first NAL unit in an access unit, zero_byte shall exists
  if (p_Vid->NALUCount == 1)
    CheckZeroByte = 1;

  if (CheckZeroByte && nalu->startcodeprefix_len == 3)
    printf ("Warning: zero_byte shall exist\n");
    //because it is not a very serious problem, we do not exit here
  }
//}}}

//{{{
int readNextNalu (VideoParameters* p_Vid, NALU_t* nalu) {

  InputParameters* p_Inp = p_Vid->p_Inp;

  int ret = getNALU (p_Vid->annex_b, p_Vid, nalu);
  if (ret < 0) {
    snprintf (errortext, ET_SIZE, "Error while getting the NALU in file format exit\n");
    error (errortext, 601);
    }
  if (ret == 0)
    return 0;

  // In some cases, zero_byte shall be present.
  // If current NALU is a VCL NALU, we can't tell whether it is the first VCL NALU at this point,
  // so only non-VCL NAL unit is checked here.
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
//{{{
int RBSPtoSODB (byte* streamBuffer, int last_byte_pos) {

  // find trailing 1
  int bitoffset = 0;
  int ctr_bit = (streamBuffer[last_byte_pos-1] & (0x01 << bitoffset));
  while (ctr_bit == 0) {
    // find trailing 1 bit
    ++bitoffset;
    if (bitoffset == 8) {
      if (last_byte_pos == 0)
        printf (" Panic: All zero data sequence in RBSP \n");
      assert (last_byte_pos != 0);
      --last_byte_pos;
      bitoffset = 0;
      }
    ctr_bit = streamBuffer[last_byte_pos - 1] & (0x01<<(bitoffset));
    }

  return(last_byte_pos);
  }
//}}}
