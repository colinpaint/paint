#include "mpeg2dec.h"

#define RB "rb"
#define WB "wb"

#ifndef O_BINARY
  #define O_BINARY 0
#endif

extern void Initialize_Fast_IDCT();
extern unsigned int Show_Bits (int N);
extern unsigned int Get_Bits32();
extern int Get_Hdr();
extern void Write_Frame (unsigned char *src[], int frame);
extern void Output_Last_Frame_of_Sequence (int Framenum);
extern void Error (char *text);

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
//{{{  ISO/IEC 13818-2 section 6.2.2.1: sequence_header()
extern int aspect_ratio_information;
extern int frame_rate_code;
extern int bit_rate_value;
extern int vbv_buffer_size;
extern int constrained_parameters_flag;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.2.3: sequence_extension()
extern int profile_and_level_indication;
extern int progressive_sequence;
extern int chroma_format;
extern int low_delay;
extern int frame_rate_extension_n;
extern int frame_rate_extension_d;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.2.4: sequence_display_extension()
extern int video_format;
extern int color_description;
extern int color_primaries;
extern int transfer_characteristics;
extern int matrix_coefficients;
extern int display_horizontal_size;
extern int display_vertical_size;
//}}}
//{{{  ISO/IEC 13818-2 section 6.2.3:   picture_header()
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
//{{{  ISO/IEC 13818-2 section 6.2.3.5: picture_spatial_scalable_extension() header
extern int lower_layer_temporal_reference;
extern int lower_layer_horizontal_offset;
extern int lower_layer_vertical_offset;
extern int spatial_temporal_weight_code_table_index;
extern int lower_layer_progressive_frame;
extern int lower_layer_deinterlaced_field_select;
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
struct layer_data {
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
