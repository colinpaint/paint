#include "mpeg2dec.h"
//{{{  readpic.c
void Substitute_Frame_Buffer (int bitstream_framenum, int sequence_framenum);
//}}}
//{{{  getbits.c
void Initialize_Buffer();
void Fill_Buffer();
unsigned int Show_Bits (int n);
unsigned int Get_Bits1();
void Flush_Buffer (int n);
unsigned int Get_Bits (int n);
int Get_Byte();
int Get_Word();
//}}}
//{{{  systems.c
void Next_Packet();
int Get_Long();
void Flush_Buffer32();
unsigned int Get_Bits32();
//}}}
//{{{  getblk.c
void Decode_MPEG1_Intra_Block (int comp, int dc_dct_pred[]);
void Decode_MPEG1_Non_Intra_Block (int comp);
void Decode_MPEG2_Intra_Block (int comp, int dc_dct_pred[]);
void Decode_MPEG2_Non_Intra_Block (int comp);
//}}}
//{{{  gethdr.c
int Get_Hdr();
void next_start_code (void);
int slice_header();
void marker_bit (char *text);
//}}}
//{{{  getpic.c
void Decode_Picture (int bitstream_framenum, int sequence_framenum);
void Output_Last_Frame_of_Sequence (int framenum);
//}}}
//{{{  getvlc.c
int Get_macroblock_type();
int Get_motion_code();
int Get_dmvector();
int Get_coded_block_pattern();
int Get_macroblock_address_increment (void);
int Get_Luma_DC_dct_diff();
int Get_Chroma_DC_dct_diff();
//}}}
//{{{  idct.c
void Fast_IDCT (short *block);
void Initialize_Fast_IDCT();
//}}}
//{{{  motion.c
void motion_vectors (int PMV[2][2][2], int dmvector[2],
                     int motion_vertical_field_select[2][2], int s, int motion_vector_count,
                     int mv_format, int h_r_size, int v_r_size, int dmv, int mvscale);
void motion_vector (int *PMV, int *dmvector,
                    int h_r_size, int v_r_size, int dmv, int mvscale, int full_pel_vector);

void Dual_Prime_Arithmetic (int DMV[][2], int *dmvector, int mvx, int mvy);
//}}}
//{{{  mpeg2dec.c
void Error (char *text);
void Warning (char *text);
void Print_Bits (int code, int bits, int len);
//}}}
//{{{  recon.c
void form_predictions (int bx, int by, int macroblock_type,
                       int motion_type, int PMV[2][2][2], int motion_vertical_field_select[2][2],
                       int dmvector[2], int stwtype);
//}}}
//{{{  spatscal.c
void Spatial_Prediction();
//}}}
//{{{  store.c
void Write_Frame (unsigned char *src[], int frame);
//}}}

extern char Version[];
extern char Author[];
extern unsigned char scan[2][64];
extern unsigned char default_intra_quantizer_matrix[64];
extern unsigned char Non_Linear_quantizer_scale[32];

//{{{  output types (Output_Type)
#define T_YUV   0
#define T_SIF   1
#define T_TGA   2
#define T_PPM   3
#define T_X11   4
#define T_X11HIQ 5
//}}}
//{{{  decoder operation control variables
extern int Output_Type;
extern int hiQdither;
//}}}
//{{{  decoder operation control flags
extern int Quiet_Flag;
extern int Trace_Flag;
extern int Fault_Flag;
extern int Verbose_Flag;
extern int Spatial_Flag;
extern int Reference_IDCT_Flag;
extern int Frame_Store_Flag;
extern int System_Stream_Flag;
extern int Display_Progressive_Flag;
extern int Ersatz_Flag;
extern int Big_Picture_Flag;
extern int Verify_Flag;
extern int Stats_Flag;
extern int User_Data_Flag;
extern int Main_Bitstream_Flag;
//}}}
//{{{  filenames
extern char *Output_Picture_Filename;
extern char *Substitute_Picture_Filename;
extern char *Main_Bitstream_Filename;
extern char *Enhancement_Layer_Bitstream_Filename;
//}}}
//{{{  buffers for multiuse purposes
extern char Error_Text[256];
extern unsigned char *Clip;
//}}}
//{{{  pointers to generic picture buffers
extern unsigned char *backward_reference_frame[3];
extern unsigned char *forward_reference_frame[3];

extern unsigned char *auxframe[3];
extern unsigned char *current_frame[3];
extern unsigned char *substitute_frame[3];
//}}}
//{{{  pointers to scalability picture buffers
extern unsigned char *llframe0[3];
extern unsigned char *llframe1[3];

extern short *lltmp;
extern char *Lower_Layer_Picture_Filename;
//}}}
//{{{  non-normative variables derived from normative elements
extern int Coded_Picture_Width;
extern int Coded_Picture_Height;
extern int Chroma_Width;
extern int Chroma_Height;
extern int block_count;
extern int Second_Field;
extern int profile, level;
//}}}
//{{{  normative derived variables (as per ISO/IEC 13818-2)
extern int horizontal_size;
extern int vertical_size;
extern int mb_width;
extern int mb_height;
extern double bit_rate;
extern double frameRate;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.2.1:  sequence_header()
extern int aspect_ratio_information;
extern int frame_rate_code;
extern int bit_rate_value;
extern int vbv_buffer_size;
extern int constrained_parameters_flag;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.2.3:  sequence_extension()
extern int profile_and_level_indication;
extern int progressive_sequence;
extern int chroma_format;
extern int low_delay;
extern int frame_rate_extension_n;
extern int frame_rate_extension_d;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.2.4:  sequence_display_extension()
extern int video_format;
extern int color_description;
extern int color_primaries;
extern int transfer_characteristics;
extern int matrix_coefficients;
extern int display_horizontal_size;
extern int display_vertical_size;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.3: picture_header()
extern int temporal_reference;
extern int picture_coding_type;
extern int vbv_delay;
extern int full_pel_forward_vector;
extern int forward_f_code;
extern int full_pel_backward_vector;
extern int backward_f_code;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.3.1: picture_coding_extension() header
extern int f_code[2][2];
extern int intra_dc_precision;
extern int picture_structure;
extern int top_field_first;
extern int frame_pred_frame_dct;
extern int concealment_motion_vectors;

extern int intra_vlc_format;

extern int repeat_first_field;

extern int chroma_420_type;
extern int progressive_frame;
extern int composite_display_flag;
extern int v_axis;
extern int field_sequence;
extern int sub_carrier;
extern int burst_amplitude;
extern int sub_carrier_phase;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.3.3: picture_display_extension() header
extern int frame_center_horizontal_offset[3];
extern int frame_center_vertical_offset[3];
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.2.5: sequence_scalable_extension() header
extern int layerId;
extern int lower_layer_prediction_horizontal_size;
extern int lower_layer_prediction_vertical_size;
extern int horizontal_subsampling_factor_m;
extern int horizontal_subsampling_factor_n;
extern int vertical_subsampling_factor_m;
extern int vertical_subsampling_factor_n;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.3.5: picture_spatial_scalable_extension() header
extern int lower_layer_temporal_reference;
extern int lower_layer_horizontal_offset;
extern int lower_layer_vertical_offset;
extern int spatial_temporal_weight_code_table_index;
extern int lower_layer_progressive_frame;
extern int lower_layer_deinterlaced_field_select;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.3.6: copyright_extension() header
extern int copyright_flag;
extern int copyright_identifier;
extern int original_or_copy;
extern int copyright_number_1;
extern int copyright_number_2;
extern int copyright_number_3;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.2.6: group_of_pictures_header()
extern int drop_flag;
extern int hour;
extern int minute;
extern int sec;
extern int frame;
extern int closed_gop;
extern int broken_link;
//}}}

//{{{
/* layer specific variables (needed for SNR and DP scalability) */
extern struct layer_data {
  /* bit input */
  int Infile;
  unsigned char Rdbfr[2048];
  unsigned char *Rdptr;
  unsigned char Inbfr[16];

  /* from mpeg2play */
  unsigned int Bfr;
  unsigned char *Rdmax;
  int Incnt;
  int Bitcnt;

  /* sequence header and quant_matrix_extension() */
  int intra_quantizer_matrix[64];
  int non_intra_quantizer_matrix[64];
  int chroma_intra_quantizer_matrix[64];
  int chroma_non_intra_quantizer_matrix[64];

  int load_intra_quantizer_matrix;
  int load_non_intra_quantizer_matrix;
  int load_chroma_intra_quantizer_matrix;
  int load_chroma_non_intra_quantizer_matrix;

  int MPEG2_Flag;

  /* sequence scalable extension */
  int scalable_mode;

  /* picture coding extension */
  int q_scale_type;
  int alternate_scan;

  /* picture spatial scalable extension */
  int pict_scal;

  /* slice/macroblock */
  int priority_breakpoint;
  int quantizer_scale;
  int intra_slice;
  short block[12][64];
  } base, enhan, *ld;
//}}}

extern int Decode_Layer;
extern int global_MBA;
extern int global_pic;
extern int True_Framenum;
