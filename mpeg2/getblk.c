//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#include "config.h"
#include "global.h"
//}}}

//{{{
/* defined in getvlc.h */
typedef struct {
  char run, level, len;
} DCTtab;
//}}}
extern DCTtab DCTtabfirst[],DCTtabnext[],DCTtab0[],DCTtab1[];
extern DCTtab DCTtab2[],DCTtab3[],DCTtab4[],DCTtab5[],DCTtab6[];
extern DCTtab DCTtab0a[],DCTtab1a[];

//{{{
void Decode_MPEG2_Intra_Block (int comp, int dc_dct_pred[]) {

  int val, i, j, sign, nc, cc, run;
  unsigned int code;
  DCTtab *tab;
  short *bp;
  int *qmat;
  struct layer_data *ld1;

  /* with data partitioning, data always goes to base layer */
  ld1 = (ld->scalable_mode==SC_DP) ? &base : ld;
  bp = ld1->block[comp];

  if (base.scalable_mode==SC_DP) {
    if (base.priority_breakpoint<64)
      ld = &enhan;
    else
      ld = &base;
    }

  cc = (comp<4) ? 0 : (comp&1)+1;

  qmat = (comp<4 || chroma_format==CHROMA420)
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
  for (i = 1; ; i++) {
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

    if (base.scalable_mode==SC_DP && nc==base.priority_breakpoint-63)
      ld = &enhan;
    }
  }
//}}}
//{{{
void Decode_MPEG2_Non_Intra_Block (int comp) {
/* decode one non-intra coded MPEG-2 block */

  int val, i, j, sign, nc, run;
  unsigned int code;
  DCTtab *tab;
  short *bp;
  int *qmat;
  struct layer_data *ld1;

  /* with data partitioning, data always goes to base layer */
  ld1 = (ld->scalable_mode==SC_DP) ? &base : ld;
  bp = ld1->block[comp];

  if (base.scalable_mode==SC_DP) {
    if (base.priority_breakpoint<64)
      ld = &enhan;
    else
      ld = &base;
    }

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

    if (base.scalable_mode == SC_DP && nc == base.priority_breakpoint-63)
      ld = &enhan;
    }
  }
//}}}
