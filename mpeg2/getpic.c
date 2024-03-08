//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#include "config.h"
#include "global.h"
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
static void saturate (short* Block_Ptr) {
/* limit coefficients to -2048..2047 */
/* ISO/IEC 13818-2 section 7.4.3 and 7.4.4: Saturation and Mismatch control */

  /* ISO/IEC 13818-2 section 7.4.3: Saturation */
  int sum = 0;
  for (int i = 0; i < 64; i++) {
    int val = Block_Ptr[i];
    if (val > 2047)
      val = 2047;
    else if (val < -2048)
      val = -2048;

    Block_Ptr[i] = val;
    sum+= val;
    }

  /* ISO/IEC 13818-2 section 7.4.4: Mismatch control */
  if ((sum & 1) == 0)
    Block_Ptr[63]^= 1;
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

  /* SCALABILITY: Data Partitioning */
  if (base.scalable_mode==SC_DP) {
    ld = &enhan;
    next_start_code();
    code = Show_Bits(32);

    if (code<SLICE_START_CODE_MIN || code>SLICE_START_CODE_MAX) {
      /* only slice headers are allowed in picture_data */
      if (!Quiet_Flag)
        printf("DP: Premature end of picture\n");
      return(-1);    /* trigger: go to next picture */
      }

    Flush_Buffer32();

    /* decode slice header (may change quantizer_scale) */
    slice_vert_pos_ext = slice_header();

    if (base.priority_breakpoint!=1)
      ld = &base;
    }

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

  /* SCALABILITY: Data Patitioning */
  if (base.scalable_mode == SC_DP) {
    if (base.priority_breakpoint<=2)
      ld = &enhan;
    else
      ld = &base;
  }

  /* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
  macroblock_modes (macroblock_type, stwtype, stwclass,
                    motion_type, &motion_vector_count, &mv_format, &dmv, &mvscale, dct_type);

  if (Fault_Flag) return(0);  /* trigger: go to next slice */

  if (*macroblock_type & MACROBLOCK_QUANT) {
    quantizer_scale_code = Get_Bits(5);

    /* ISO/IEC 13818-2 section 7.4.2.2: Quantizer scale factor */
    if (ld->MPEG2_Flag)
      ld->quantizer_scale =
      ld->q_scale_type ? Non_Linear_quantizer_scale[quantizer_scale_code]
       : (quantizer_scale_code << 1);
    else
      ld->quantizer_scale = quantizer_scale_code;

    /* SCALABILITY: Data Partitioning */
    if (base.scalable_mode==SC_DP)
      /* make sure base.quantizer_scale is valid */
      base.quantizer_scale = ld->quantizer_scale;
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

  if (base.scalable_mode==SC_DP && base.priority_breakpoint==3)
    ld = &enhan;

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
    /* SCALABILITY: Data Partitioning */
    if (base.scalable_mode==SC_DP)
      ld = &base;

    clearBlock (comp);
    if (coded_block_pattern & (1<<(block_count-1-comp))) {
      if (*macroblock_type & MACROBLOCK_INTRA) {
        if (ld->MPEG2_Flag)
          Decode_MPEG2_Intra_Block(comp,dc_dct_pred);
        else
          Decode_MPEG1_Intra_Block(comp,dc_dct_pred);
        }
      else {
        if (ld->MPEG2_Flag)
          Decode_MPEG2_Non_Intra_Block(comp);
        else
          Decode_MPEG1_Non_Intra_Block(comp);
        }

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
      motion_vertical_field_select[0][0] = (picture_structure==BOTTOM_FIELD);
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
static void frame_reorder (int Bitstream_Framenum, int Sequence_Framenum) {

  /* tracking variables to insure proper output in spatial scalability */
  static int Oldref_progressive_frame, Newref_progressive_frame;

  if (Sequence_Framenum!=0) {
    if (picture_structure==FRAME_PICTURE || Second_Field) {
      if (picture_coding_type==B_TYPE)
        Write_Frame(auxframe,Bitstream_Framenum-1);
      else {
        Newref_progressive_frame = progressive_frame;
        progressive_frame = Oldref_progressive_frame;

        Write_Frame(forward_reference_frame,Bitstream_Framenum-1);

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

  int bx, by;
  int comp;

  /* derive current macroblock position within picture */
  /* ISO/IEC 13818-2 section 6.3.1.6 and 6.3.1.7 */
  bx = 16*(MBA%mb_width);
  by = 16*(MBA/mb_width);

  /* motion compensation */
  if (!(macroblock_type & MACROBLOCK_INTRA))
    form_predictions(bx,by,macroblock_type,motion_type,PMV,
      motion_vertical_field_select,dmvector,stwtype);

  /* SCALABILITY: Data Partitioning */
  if (base.scalable_mode==SC_DP)
    ld = &base;

  /* copy or add block data into picture */
  for (comp=0; comp<block_count; comp++) {
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

  /* SCALABILITY: Data Paritioning */
  if (base.scalable_mode==SC_DP)
    ld = &base;

  for (int comp=0; comp<block_count; comp++)
    clearBlock (comp);

  /* reset intra_dc predictors */
  /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
  dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;

  /* reset motion vector predictors */
  /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
  if (picture_coding_type == P_TYPE)
    PMV[0][0][0]=PMV[0][0][1] = PMV[1][0][0]=PMV[1][0][1]=0;

  /* derive motion_type */
  if (picture_structure == FRAME_PICTURE)
    *motion_type = MC_FRAME;
  else {
    *motion_type = MC_FIELD;

    /* predict from field of same parity */
    /* ISO/IEC 13818-2 section 7.6.6.1 and 7.6.6.3: P field picture and B field
       picture */
    motion_vertical_field_select[0][0] = motion_vertical_field_select[0][1] =
                                         (picture_structure == BOTTOM_FIELD);
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
static void decode_SNR_Macroblock (int* SNRMBA, int* SNRMBAinc, int MBA, int MBAmax, int *dct_type) {
/* ISO/IEC 13818-2 section 7.8 */

  int SNRmacroblock_type, SNRcoded_block_pattern, SNRdct_type, dummy;
  int slice_vert_pos_ext, quantizer_scale_code, comp, code;

  ld = &enhan;
  if (*SNRMBAinc==0) {
    if (!Show_Bits(23)) {
      /* next_start_code */
      next_start_code();
      code = Show_Bits(32);

      if (code<SLICE_START_CODE_MIN || code>SLICE_START_CODE_MAX) {
        /* only slice headers are allowed in picture_data */
        if (!Quiet_Flag)
          printf("SNR: Premature end of picture\n");
        return;
        }

      Flush_Buffer32();

      /* decode slice header (may change quantizer_scale) */
      slice_vert_pos_ext = slice_header();
      /* decode macroblock address increment */
      *SNRMBAinc = Get_macroblock_address_increment();
      /* set current location */
      *SNRMBA = ((slice_vert_pos_ext<<7) + (code&255) - 1)*mb_width + *SNRMBAinc - 1;
      *SNRMBAinc = 1; /* first macroblock in slice: not skipped */
      }
    else {
      /* not next_start_code */
      if (*SNRMBA>=MBAmax) {
        if (!Quiet_Flag)
          printf("Too many macroblocks in picture\n");
        return;
        }
      /* decode macroblock address increment */
      *SNRMBAinc = Get_macroblock_address_increment();
      }
    }

  if (*SNRMBA!=MBA) {
    /* streams out of sync */
    if (!Quiet_Flag)
      printf("Cant't synchronize streams\n");
    return;
    }

  if (*SNRMBAinc==1) {
    /* not skipped */
    macroblock_modes (&SNRmacroblock_type, &dummy, &dummy,
                      &dummy, &dummy, &dummy, &dummy, &dummy, &SNRdct_type);
    if (SNRmacroblock_type & MACROBLOCK_PATTERN)
      *dct_type = SNRdct_type;
    if (SNRmacroblock_type & MACROBLOCK_QUANT) {
      quantizer_scale_code = Get_Bits(5);
      ld->quantizer_scale = ld->q_scale_type ? Non_Linear_quantizer_scale[quantizer_scale_code] :
                                               quantizer_scale_code<<1;
    }

    /* macroblock_pattern */
    if (SNRmacroblock_type & MACROBLOCK_PATTERN) {
      SNRcoded_block_pattern = Get_coded_block_pattern();

      if (chroma_format==CHROMA422)
        SNRcoded_block_pattern = (SNRcoded_block_pattern<<2) | Get_Bits(2); /* coded_block_pattern_1 */
      else if (chroma_format==CHROMA444)
        SNRcoded_block_pattern = (SNRcoded_block_pattern<<6) | Get_Bits(6); /* coded_block_pattern_2 */
    }
    else
      SNRcoded_block_pattern = 0;

    /* decode blocks */
    for (comp = 0; comp < block_count; comp++) {
      clearBlock (comp);
      if (SNRcoded_block_pattern & (1<<(block_count-1-comp)))
        Decode_MPEG2_Non_Intra_Block(comp);
    }
  }
  else {
    /* SNRMBAinc!=1: skipped macroblock */
    for (comp = 0; comp < block_count; comp++)
      clearBlock (comp);
    }

  ld = &base;
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
    if (MBA>=MBAmax)
      return(-1); /* all macroblocks decoded */

    ld = &base;
    if (MBAinc==0) {
      if (base.scalable_mode==SC_DP && base.priority_breakpoint==1)
          ld = &enhan;

      if (!Show_Bits(23) || Fault_Flag) {
        /* next_start_code or fault */
resync: /* if Fault_Flag: resynchronize to next next_start_code */
        Fault_Flag = 0;
        return(0);     /* trigger: go to next slice */
        }
      else {
        /* neither next_start_code nor Fault_Flag */
        if (base.scalable_mode==SC_DP && base.priority_breakpoint==1)
          ld = &enhan;

        /* decode macroblock address increment */
        MBAinc = Get_macroblock_address_increment();

        if (Fault_Flag) goto resync;
        }
      }

    if (MBA>=MBAmax) {
      /* MBAinc points beyond picture dimensions */
      if (!Quiet_Flag)
        printf("Too many macroblocks in picture\n");
      return(-1);
      }

    if (MBAinc==1) {
      /* not skipped */
      ret = decode_macroblock (&macroblock_type, &stwtype, &stwclass,
                               &motion_type, &dct_type, PMV, dc_dct_pred,
                               motion_vertical_field_select, dmvector);
      if  (ret == -1)
        return (-1);
      if (ret == 0)
        goto resync;
      }
    else {
      /* MBAinc!=1: skipped macroblock */
      /* ISO/IEC 13818-2 section 7.6.6 */
      skipped_macroblock (dc_dct_pred, PMV, &motion_type,
                          motion_vertical_field_select, &stwtype, &macroblock_type);
      }


    /* ISO/IEC 13818-2 section 7.6 */
    motion_compensation (MBA, macroblock_type, motion_type, PMV,
                         motion_vertical_field_select, dmvector, stwtype, dct_type);


    /* advance to next macroblock */
    MBA++;
    MBAinc--;

    if (MBA>=MBAmax)
      return(-1); /* all macroblocks decoded */
    }
  }
//}}}
//{{{
static void picture_data (int framenum) {
/* decode all macroblocks of the current picture */
/* stages described in ISO/IEC 13818-2 section 7 */

  int ret;

  /* number of macroblocks per picture */
  int MBAmax = mb_width*mb_height;
  if (picture_structure!=FRAME_PICTURE)
    MBAmax>>=1; /* field picture has half as mnay macroblocks as frame */

  for(;;)
    if ((ret=slice(framenum, MBAmax))<0)
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
    {
      current_frame[cc] = auxframe[cc];
    }
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
    if (picture_structure==BOTTOM_FIELD)
      current_frame[cc]+= (cc==0) ? Coded_Picture_Width : Chroma_Width;
  }
}
//}}}

//{{{
void Output_Last_Frame_of_Sequence (int Framenum) {

  if (Second_Field)
    printf("last frame incomplete, not stored\n");
  else
    Write_Frame(backward_reference_frame,Framenum-1);
}
//}}}
//{{{
void Decode_Picture (int bitstream_framenum, int sequence_framenum) {
/* decode one frame or field picture */

  if (picture_structure == FRAME_PICTURE && Second_Field) {
    /* recover from illegal number of field pictures */
    printf("odd number of field pictures\n");
    Second_Field = 0;
    }

  /* IMPLEMENTATION: update picture buffer pointers */
  update_Picture_Buffers();

  /* ISO/IEC 13818-4 section 2.4.5.4 "frame buffer intercept method" */
  /* (section number based on November 1995 (Dallas) draft of the
      conformance document) */
  if (Ersatz_Flag)
    Substitute_Frame_Buffer (bitstream_framenum, sequence_framenum);

  /* form spatial scalable picture */
  /* ISO/IEC 13818-2 section 7.7: Spatial scalability */
  if (base.pict_scal && !Second_Field)
    Spatial_Prediction();

  /* decode picture data ISO/IEC 13818-2 section 6.2.3.7 */
  picture_data (bitstream_framenum);

  /* write or display current or previously decoded reference frame */
  /* ISO/IEC 13818-2 section 6.1.1.11: Frame reordering */
  frame_reorder (bitstream_framenum, sequence_framenum);

  if (picture_structure != FRAME_PICTURE)
    Second_Field = !Second_Field;
  }
//}}}
