//{{{  includes
#include <math.h>

#include "global.h"
#include "memAlloc.h"

#include "sei.h"
#include "vlc.h"
#include "sliceHeader.h"
#include "mbuffer.h"
#include "parset.h"
//}}}
static const int kDebug = 0;

//{{{
typedef enum {
  SEI_BUFFERING_PERIOD = 0,
  SEI_PIC_TIMING,
  SEI_PAN_SCAN_RECT,
  SEI_FILLER_PAYLOAD,
  SEI_USER_DATA_REGISTERED_ITU_T_T35,
  SEI_USER_DATA_UNREGISTERED,
  SEI_RECOVERY_POINT,
  SEI_DEC_REF_PIC_MARKING_REPETITION,
  SEI_SPARE_PIC,
  SEI_SCENE_INFO,
  SEI_SUB_SEQ_INFO,
  SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
  SEI_SUB_SEQ_CHARACTERISTICS,
  SEI_FULL_FRAME_FREEZE,
  SEI_FULL_FRAME_FREEZE_RELEASE,
  SEI_FULL_FRAME_SNAPSHOT,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
  SEI_FILM_GRAIN_CHARACTERISTICS,
  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,
  SEI_STEREO_VIDEO_INFO,
  SEI_POST_FILTER_HINTS,
  SEI_TONE_MAPPING,
  SEI_SCALABILITY_INFO,
  SEI_SUB_PIC_SCALABLE_LAYER,
  SEI_NON_REQUIRED_LAYER_REP,
  SEI_PRIORITY_LAYER_INFO,
  SEI_LAYERS_NOT_PRESENT,
  SEI_LAYER_DEPENDENCY_CHANGE,
  SEI_SCALABLE_NESTING,
  SEI_BASE_LAYER_TEMPORAL_HRD,
  SEI_QUALITY_LAYER_INTEGRITY_CHECK,
  SEI_REDUNDANT_PIC_PROPERTY,
  SEI_TL0_DEP_REP_INDEX,
  SEI_TL_SWITCHING_POINT,
  SEI_PARALLEL_DECODING_INFO,
  SEI_MVC_SCALABLE_NESTING,
  SEI_VIEW_SCALABILITY_INFO,
  SEI_MULTIVIEW_SCENE_INFO,
  SEI_MULTIVIEW_ACQUISITION_INFO,
  SEI_NON_REQUIRED_VIEW_COMPONENT,
  SEI_VIEW_DEPENDENCY_CHANGE,
  SEI_OPERATION_POINTS_NOT_PRESENT,
  SEI_BASE_VIEW_TEMPORAL_HRD,
  SEI_FRAME_PACKING_ARRANGEMENT,
  SEI_GREEN_METADATA=56,

  SEI_MAX_ELEMENTS  //!< number of maximum syntax elements
  } SEI_type;
//}}}
#define MAX_FN 256
#define MAX_CODED_BIT_DEPTH  12
#define MAX_SEI_BIT_DEPTH    12
#define MAX_NUM_PIVOTS     (1<<MAX_CODED_BIT_DEPTH)
//{{{  frame_packing_arrangement_information_struct
typedef struct {
  unsigned int  frame_packing_arrangement_id;
  Boolean       frame_packing_arrangement_cancel_flag;
  unsigned char frame_packing_arrangement_type;
  Boolean       quincunx_sampling_flag;
  unsigned char content_interpretation_type;
  Boolean       spatial_flipping_flag;
  Boolean       frame0_flipped_flag;
  Boolean       field_views_flag;
  Boolean       current_frame_is_frame0_flag;
  Boolean       frame0_self_contained_flag;
  Boolean       frame1_self_contained_flag;
  unsigned char frame0_grid_position_x;
  unsigned char frame0_grid_position_y;
  unsigned char frame1_grid_position_x;
  unsigned char frame1_grid_position_y;
  unsigned char frame_packing_arrangement_reserved_byte;
  unsigned int  frame_packing_arrangement_repetition_period;
  Boolean       frame_packing_arrangement_extension_flag;
  } frame_packing_arrangement_information_struct;
//}}}
//{{{  Green_metadata_information_struct
typedef struct {
  unsigned char  green_metadata_type;
  unsigned char  period_type;
  unsigned short num_seconds;
  unsigned short num_pictures;
  unsigned char percent_non_zero_macroblocks;
  unsigned char percent_intra_coded_macroblocks;
  unsigned char percent_six_tap_filtering;
  unsigned char percent_alpha_point_deblocking_instance;
  unsigned char xsd_metric_type;
  unsigned short xsd_metric_value;
  } Green_metadata_information_struct;
//}}}

//{{{
static void interpret_spare_pic (byte* payload, int size, sVidParam* vidParam ) {

  int x,y;
  Bitstream* buf;
  int bit0, bit1, no_bit0;
  int target_frame_num = 0;
  int num_spare_pics;
  int delta_spare_frame_num, CandidateSpareFrameNum, SpareFrameNum = 0;
  int ref_area_indicator;

  int m, n, left, right, top, bottom,directx, directy;
  byte*** map;

  buf = malloc (sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  target_frame_num = read_ue_v ("SEI: target_frame_num", buf);
  num_spare_pics = 1 + read_ue_v ("SEI: num_spare_pics_minus1", buf);
  printf ("SEQ spare picture target_frame_num:%d num_spare_pics:%d\n",
          target_frame_num, num_spare_pics);

  get_mem3D (&map, num_spare_pics, vidParam->height >> 4, vidParam->width >> 4);
  for (int i = 0; i < num_spare_pics; i++) {
    if (i == 0) {
      CandidateSpareFrameNum = target_frame_num - 1;
      if (CandidateSpareFrameNum < 0)
        CandidateSpareFrameNum = MAX_FN - 1;
      }
    else
      CandidateSpareFrameNum = SpareFrameNum;

    delta_spare_frame_num = read_ue_v ("SEI: delta_spare_frame_num", buf);
    SpareFrameNum = CandidateSpareFrameNum - delta_spare_frame_num;
    if (SpareFrameNum < 0 )
      SpareFrameNum = MAX_FN + SpareFrameNum;

    ref_area_indicator = read_ue_v ("SEI: ref_area_indicator", buf);
    switch (ref_area_indicator) {
      //{{{
      case 0: // The whole frame can serve as spare picture
        for (y=0; y<vidParam->height >> 4; y++)
          for (x=0; x<vidParam->width >> 4; x++)
            map[i][y][x] = 0;
        break;
      //}}}
      //{{{
      case 1: // The map is not compressed
        for (y=0; y<vidParam->height >> 4; y++)
          for (x=0; x<vidParam->width >> 4; x++)
            map[i][y][x] = (byte) read_u_1("SEI: ref_mb_indicator", buf);
        break;
      //}}}
      //{{{
      case 2: // The map is compressed
        bit0 = 0;
        bit1 = 1;
        no_bit0 = -1;

        x = ((vidParam->width >> 4) - 1 ) / 2;
        y = ((vidParam->height >> 4) - 1 ) / 2;
        left = right = x;
        top = bottom = y;
        directx = 0;
        directy = 1;

        for (m = 0; m < vidParam->height >> 4; m++)
          for (n = 0; n < vidParam->width >> 4; n++) {
            if (no_bit0 < 0)
              no_bit0 = read_ue_v ("SEI: zero_run_length", buf);
            if (no_bit0>0)
              map[i][y][x] = (byte) bit0;
            else
              map[i][y][x] = (byte) bit1;
            no_bit0--;

            // go to the next mb:
            if ( directx == -1 && directy == 0 ) {
              //{{{
              if (x > left)
                x--;
              else if (x == 0) {
                y = bottom + 1;
                bottom++;
                directx = 1;
                directy = 0;
                }
              else if (x == left) {
                x--;
                left--;
                directx = 0;
                directy = 1;
                }
              }
              //}}}
            else if ( directx == 1 && directy == 0 ) {
              //{{{
              if (x < right)
                x++;
              else if (x == (vidParam->width >> 4) - 1) {
                y = top - 1;
                top--;
                directx = -1;
                directy = 0;
                }
              else if (x == right) {
                x++;
                right++;
                directx = 0;
                directy = -1;
                }
              }
              //}}}
            else if ( directx == 0 && directy == -1 ) {
              //{{{
              if ( y > top)
                y--;
              else if (y == 0) {
                x = left - 1;
                left--;
                directx = 0;
                directy = 1;
                }
              else if (y == top) {
                y--;
                top--;
                directx = -1;
                directy = 0;
                }
              }
              //}}}
            else if ( directx == 0 && directy == 1 ) {
              //{{{
              if (y < bottom)
                y++;
              else if (y == (vidParam->height >> 4) - 1) {
                x = right+1;
                right++;
                directx = 0;
                directy = -1;
                }
              else if (y == bottom) {
                y++;
                bottom++;
                directx = 1;
                directy = 0;
                }
              }
              //}}}
            }
        break;
      //}}}
      //{{{
      default:
        printf ("Wrong ref_area_indicator %d!\n", ref_area_indicator );
        exit(0);
        break;
      //}}}
      }
    }

  free_mem3D (map);
  free (buf);
  }
//}}}

//{{{
static void interpret_subsequence_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf = malloc (sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int sub_seq_layer_num = read_ue_v ("SEI: sub_seq_layer_num", buf);
  int sub_seq_id = read_ue_v ("SEI: sub_seq_id", buf);
  int first_ref_pic_flag = read_u_1 ("SEI: first_ref_pic_flag", buf);
  int leading_non_ref_pic_flag = read_u_1 ("SEI: leading_non_ref_pic_flag", buf);
  int last_pic_flag = read_u_1 ("SEI: last_pic_flag", buf);

  int sub_seq_frame_num = 0;
  int sub_seq_frame_num_flag = read_u_1 ("SEI: sub_seq_frame_num_flag", buf);
  if (sub_seq_frame_num_flag)
    sub_seq_frame_num = read_ue_v("SEI: sub_seq_frame_num", buf);

  printf ("Sub-sequence information SEI message\n");
  printf ("sub_seq_layer_num = %d\n", sub_seq_layer_num );
  printf ("sub_seq_id = %d\n", sub_seq_id);
  printf ("first_ref_pic_flag = %d\n", first_ref_pic_flag);
  printf ("leading_non_ref_pic_flag = %d\n", leading_non_ref_pic_flag);
  printf ("last_pic_flag = %d\n", last_pic_flag);
  printf ("sub_seq_frame_num_flag = %d\n", sub_seq_frame_num_flag);
  if (sub_seq_frame_num_flag)
    printf ("sub_seq_frame_num = %d\n", sub_seq_frame_num);

  free  (buf);
  }
//}}}
//{{{
static void interpret_subsequence_layer_characteristics_info (byte* payload, int size, sVidParam* vidParam )
{
  Bitstream* buf;
  long num_sub_layers, accurate_statistics_flag, average_bit_rate, average_frame_rate;

  buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  num_sub_layers = 1 + read_ue_v("SEI: num_sub_layers_minus1", buf);
  printf ("SEI Sub-sequence layer characteristics num_sub_layers_minus1 %ld\n", num_sub_layers - 1);
  for (int i = 0; i < num_sub_layers; i++) {
    accurate_statistics_flag = read_u_1 ("SEI: accurate_statistics_flag", buf);
    average_bit_rate = read_u_v (16,"SEI: average_bit_rate", buf);
    average_frame_rate = read_u_v (16,"SEI: average_frame_rate", buf);

    printf("layer %d: accurate_statistics_flag = %ld\n", i, accurate_statistics_flag);
    printf("layer %d: average_bit_rate = %ld\n", i, average_bit_rate);
    printf("layer %d: average_frame_rate= %ld\n", i, average_frame_rate);
  }
  free (buf);
}
//}}}
//{{{
static void interpret_subsequence_characteristics_info (byte* payload, int size, sVidParam* vidParam ) {

  int i;
  int sub_seq_layer_num, sub_seq_id, duration_flag, average_rate_flag, accurate_statistics_flag;
  unsigned long sub_seq_duration, average_bit_rate, average_frame_rate;
  int num_referenced_subseqs, ref_sub_seq_layer_num, ref_sub_seq_id, ref_sub_seq_direction;

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  sub_seq_layer_num = read_ue_v("SEI: sub_seq_layer_num", buf);
  sub_seq_id        = read_ue_v("SEI: sub_seq_id", buf);
  duration_flag     = read_u_1 ("SEI: duration_flag", buf);

  printf ("Sub-sequence characteristics SEI message\n");
  printf ("sub_seq_layer_num = %d\n", sub_seq_layer_num );
  printf ("sub_seq_id        = %d\n", sub_seq_id);
  printf ("duration_flag     = %d\n", duration_flag);

  if ( duration_flag ) {
    sub_seq_duration = read_u_v (32, "SEI: duration_flag", buf);
    printf ("sub_seq_duration = %ld\n", sub_seq_duration);
    }

  average_rate_flag = read_u_1 ("SEI: average_rate_flag", buf);

  printf ("average_rate_flag = %d\n", average_rate_flag);

  if ( average_rate_flag ) {
    accurate_statistics_flag = read_u_1 (    "SEI: accurate_statistics_flag", buf);
    average_bit_rate         = read_u_v (16, "SEI: average_bit_rate", buf);
    average_frame_rate       = read_u_v (16, "SEI: average_frame_rate", buf);

    printf ("accurate_statistics_flag = %d\n", accurate_statistics_flag);
    printf ("average_bit_rate         = %ld\n", average_bit_rate);
    printf ("average_frame_rate       = %ld\n", average_frame_rate);
    }

  num_referenced_subseqs  = read_ue_v("SEI: num_referenced_subseqs", buf);
  printf ("num_referenced_subseqs = %d\n", num_referenced_subseqs);

  for (i=0; i<num_referenced_subseqs; i++) {
    ref_sub_seq_layer_num  = read_ue_v("SEI: ref_sub_seq_layer_num", buf);
    ref_sub_seq_id         = read_ue_v("SEI: ref_sub_seq_id", buf);
    ref_sub_seq_direction  = read_u_1 ("SEI: ref_sub_seq_direction", buf);

    printf ("ref_sub_seq_layer_num = %d\n", ref_sub_seq_layer_num);
    printf ("ref_sub_seq_id        = %d\n", ref_sub_seq_id);
    printf ("ref_sub_seq_direction = %d\n", ref_sub_seq_direction);
    }

  free (buf);
  }
//}}}
//{{{
static void interpret_scene_information (byte* payload, int size, sVidParam* vidParam )
{
  int scene_id, scene_transition_type, second_scene_id;

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  scene_id              = read_ue_v ("SEI: scene_id"             , buf);
  scene_transition_type = read_ue_v ("SEI: scene_transition_type", buf);
  if ( scene_transition_type > 3 )
    second_scene_id = read_ue_v ("SEI: scene_transition_type", buf);

  printf ("Scene information SEI message\n");
  printf ("scene_transition_type = %d\n", scene_transition_type);
  printf ("scene_id              = %d\n", scene_id);
  if (scene_transition_type > 3 )
    printf ("second_scene_id       = %d\n", second_scene_id);

  free (buf);
  }
//}}}
//{{{
static void interpret_filler_payload_info (byte* payload, int size, sVidParam* vidParam ) {

  int payload_cnt = 0;
  while (payload_cnt<size)
    if (payload[payload_cnt] == 0xFF)
       payload_cnt++;

  printf ("Filler payload SEI message\n");
  if (payload_cnt == size)
    printf ("read %d bytes of filler payload\n", payload_cnt);
  else
    printf ("error reading filler payload: not all bytes are 0xFF (%d of %d)\n", payload_cnt, size);
  }
//}}}

//{{{
static void interpret_user_data_unregistered_info (byte* payload, int size, sVidParam* vidParam )
{
  int offset = 0;
  byte payload_byte;

  printf ("User data unregistered SEI message\n");
  printf ("uuid_iso_11578 = 0x");
  assert (size>=16);

  for (offset = 0; offset < 16; offset++)
    printf ("%02x",payload[offset]);

  printf ("\n");

  while (offset < size) {
    payload_byte = payload[offset];
    offset ++;
    printf ("Unreg data payload_byte = %d\n", payload_byte);
    }
  }
//}}}
//{{{
static void interpret_user_data_registered_itu_t_t35_info (byte* payload, int size, sVidParam* vidParam ) {

  int offset = 0;
  byte itu_t_t35_country_code, itu_t_t35_country_code_extension_byte, payload_byte;

  itu_t_t35_country_code = payload[offset];
  offset++;

  if (kDebug)
    printf ("SEI User data ITU-T T.35 country_code %d\n", itu_t_t35_country_code);

  if (itu_t_t35_country_code == 0xFF) {
    itu_t_t35_country_code_extension_byte = payload[offset];
    offset++;
    if (kDebug)
      printf (" ITU_T_T35_COUNTRY_CODE_EXTENSION_BYTE %d\n", itu_t_t35_country_code_extension_byte);
    }

  while (offset < size) {
    payload_byte = payload[offset];
    offset ++;
    if (kDebug)
      printf ("itu_t_t35 payload_byte %d\n", payload_byte);
    }
 }
//}}}
//{{{
static void interpret_reserved_info (byte* payload, int size, sVidParam* vidParam )
{
  int offset = 0;
  byte payload_byte;

  printf ("SEI Reserved\n");

  while (offset < size)
  {
    payload_byte = payload[offset];
    offset ++;
    printf("reserved_sei_message_payload_byte = %d\n", payload_byte);
  }
}
//}}}

//{{{
static void interpret_picture_timing_info (byte* payload, int size, sVidParam* vidParam ) {

  sSPSrbsp *active_sps = vidParam->active_sps;

  int cpb_removal_len = 24;
  int dpb_output_len  = 24;
  Boolean CpbDpbDelaysPresentFlag;

  if (NULL == active_sps) {
    fprintf (stderr, "Warning: no active SPS, timing SEI cannot be parsed\n");
    return;
    }

  Bitstream* buf = malloc (sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  if (kDebug)
    printf ("SEI Picture timing\n");

  // CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
  CpbDpbDelaysPresentFlag = (Boolean)(active_sps->vui_parameters_present_flag
                              && (   (active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag != 0)
                                   ||(active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag != 0)));

  if (CpbDpbDelaysPresentFlag ) {
    if (active_sps->vui_parameters_present_flag) {
      if (active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag) {
        cpb_removal_len = active_sps->vui_seq_parameters.nal_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
        dpb_output_len  = active_sps->vui_seq_parameters.nal_hrd_parameters.dpb_output_delay_length_minus1  + 1;
        }
      else if (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag) {
        cpb_removal_len = active_sps->vui_seq_parameters.vcl_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
        dpb_output_len = active_sps->vui_seq_parameters.vcl_hrd_parameters.dpb_output_delay_length_minus1  + 1;
        }
      }

    if ((active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag)||
        (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)) {
      int cpb_removal_delay, dpb_output_delay;
      cpb_removal_delay = read_u_v(cpb_removal_len, "SEI: cpb_removal_delay" , buf);
      dpb_output_delay = read_u_v(dpb_output_len,  "SEI: dpb_output_delay"  , buf);
      if (kDebug) {
        printf ("cpb_removal_delay = %d\n",cpb_removal_delay);
        printf ("dpb_output_delay  = %d\n",dpb_output_delay);
        }
      }
    }

  int pic_struct_present_flag, pic_struct;
  if (!active_sps->vui_parameters_present_flag)
    pic_struct_present_flag = 0;
  else
    pic_struct_present_flag = active_sps->vui_seq_parameters.pic_struct_present_flag;

  int NumClockTs = 0;
  if (pic_struct_present_flag) {
    pic_struct = read_u_v (4, "SEI: pic_struct" , buf);
    //printf ("pic_struct = %d\n",pic_struct);
    switch (pic_struct) {
      case 0:
      case 1:
      //{{{
      case 2:
        NumClockTs = 1;
        break;
      //}}}
      case 3:
      case 4:
      //{{{
      case 7:
        NumClockTs = 2;
        break;
      //}}}
      case 5:
      case 6:
      //{{{
      case 8:
        NumClockTs = 3;
        break;
      //}}}
      //{{{
      default:
        error ("reserved pic_struct used (can't determine NumClockTs)", 500);
      //}}}
      }
    if (kDebug)
      for (int i = 0; i < NumClockTs; i++) {
        //{{{  print
        int clock_timestamp_flag = read_u_1 ("SEI: clock_timestamp_flag", buf);
        printf ("clock_timestamp_flag = %d\n", clock_timestamp_flag);
        if (clock_timestamp_flag) {
          int ct_type = read_u_v (2, "SEI: ct_type", buf);
          int nuit_field_based_flag = read_u_1 ("SEI: nuit_field_based_flag", buf);
          int counting_type = read_u_v (5, "SEI: counting_type", buf);
          int full_timestamp_flag = read_u_1 ("SEI: full_timestamp_flag", buf);
          int discontinuity_flag = read_u_1 ("SEI: discontinuity_flag", buf);
          int cnt_dropped_flag = read_u_1 ("SEI: cnt_dropped_flag", buf);
          int nframes = read_u_v (8, "SEI: nframes", buf);

          printf ("ct_type = %d\n",ct_type);
          printf ("nuit_field_based_flag = %d\n",nuit_field_based_flag);
          printf ("full_timestamp_flag = %d\n",full_timestamp_flag);
          printf ("discontinuity_flag = %d\n",discontinuity_flag);
          printf ("cnt_dropped_flag = %d\n",cnt_dropped_flag);
          printf ("nframes = %d\n",nframes);

          if (full_timestamp_flag) {
            int seconds_value = read_u_v (6, "SEI: seconds_value", buf);
            int minutes_value = read_u_v (6, "SEI: minutes_value", buf);
            int hours_value = read_u_v (5, "SEI: hours_value", buf);
            printf ("seconds_value = %d\n", seconds_value);
            printf ("minutes_value= %d\n", minutes_value);
            printf ("hours_value = %d\n", hours_value);
            }
          else {
            int seconds_flag = read_u_1 ("SEI: seconds_flag", buf);
            printf ("seconds_flag = %d\n",seconds_flag);
            if (seconds_flag) {
              int seconds_value = read_u_v (6, "SEI: seconds_value", buf);
              int minutes_flag = read_u_1 ("SEI: minutes_flag", buf);
              printf ("seconds_value = %d\n",seconds_value);
              printf ("minutes_flag = %d\n",minutes_flag);
              if(minutes_flag) {
                int minutes_value = read_u_v(6, "SEI: minutes_value", buf);
                int hours_flag = read_u_1 ("SEI: hours_flag", buf);
                printf ("minutes_value = %d\n",minutes_value);
                printf ("hours_flag = %d\n",hours_flag);
                if (hours_flag) {
                  int hours_value = read_u_v (5, "SEI: hours_value", buf);
                  printf ("hours_value = %d\n",hours_value);
                  }
                }
              }
            }

          int time_offset_length;
          int time_offset;
          if (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
            time_offset_length = active_sps->vui_seq_parameters.vcl_hrd_parameters.time_offset_length;
          else if (active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
            time_offset_length = active_sps->vui_seq_parameters.nal_hrd_parameters.time_offset_length;
          else
            time_offset_length = 24;
          if (time_offset_length)
            time_offset = read_i_v (time_offset_length, "SEI: time_offset"   , buf);
          else
            time_offset = 0;
          printf ("time_offset   = %d\n",time_offset);
          }
        }
        //}}}
    }

  free (buf);
  }
//}}}
//{{{
static void interpret_pan_scan_rect_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf;
  buf = malloc (sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int pan_scan_rect_id = read_ue_v ("SEI: pan_scan_rect_id", buf);
  int pan_scan_rect_cancel_flag = read_u_1 ("SEI: pan_scan_rect_cancel_flag", buf);
  if (!pan_scan_rect_cancel_flag) {
    int pan_scan_cnt_minus1 = read_ue_v ("SEI: pan_scan_cnt_minus1", buf);
    for (int i = 0; i <= pan_scan_cnt_minus1; i++) {
      int pan_scan_rect_left_offset = read_se_v ("SEI: pan_scan_rect_left_offset", buf);
      int pan_scan_rect_right_offset = read_se_v ("SEI: pan_scan_rect_right_offset", buf);
      int pan_scan_rect_top_offset = read_se_v ("SEI: pan_scan_rect_top_offset", buf);
      int pan_scan_rect_bottom_offset = read_se_v ("SEI: pan_scan_rect_bottom_offset", buf);
      if (kDebug)
        printf ("SEI Pan scan %d/%d id %d left_offset %d right_offset %d top_offset %d bottom_offset %d\n",
                i, pan_scan_cnt_minus1, pan_scan_rect_id,
                pan_scan_rect_left_offset, pan_scan_rect_right_offset,
                pan_scan_rect_top_offset, pan_scan_rect_bottom_offset);
      }
    int pan_scan_rect_repetition_period = read_ue_v ("SEI: pan_scan_rect_repetition_period", buf);
    }

  free (buf);
  }
//}}}
//{{{
static void interpret_recovery_point_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf;
  buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int recovery_frame_cnt = read_ue_v ("SEI: recovery_frame_cnt", buf);
  int exact_match_flag = read_u_1 ("SEI: exact_match_flag", buf);
  int broken_link_flag = read_u_1 ("SEI: broken_link_flag", buf);
  int changing_slice_group_idc = read_u_v (2, "SEI: changing_slice_group_idc", buf);

  vidParam->recovery_point = 1;
  vidParam->recovery_frame_cnt = recovery_frame_cnt;
  if (kDebug)
    printf ("SEI Recovery point recovery_frame_cnt %d exact_match %d broken_link %d changing_slice_group_idc %d\n",
            recovery_frame_cnt, exact_match_flag, broken_link_flag, changing_slice_group_idc);

  free (buf);
  }
//}}}
//{{{
static void interpret_dec_ref_pic_marking_repetition_info (byte* payload, int size,
                                                           sVidParam* vidParam, sSlice *pSlice) {
  int original_idr_flag, original_frame_num;
  int original_field_pic_flag;

  DecRefPicMarking_t *tmp_drpm;
  DecRefPicMarking_t *old_drpm;
  int old_idr_flag, old_no_output_of_prior_pics_flag, old_long_term_reference_flag , old_adaptive_ref_pic_buffering_flag;

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  original_idr_flag = read_u_1 ("SEI: original_idr_flag", buf);
  original_frame_num = read_ue_v ("SEI: original_frame_num", buf);

  int original_bottom_field_flag = 0;
  if (!vidParam->active_sps->frame_mbs_only_flag) {
    original_field_pic_flag = read_u_1 ("SEI: original_field_pic_flag", buf);
    if (original_field_pic_flag)
      original_bottom_field_flag = read_u_1 ("SEI: original_bottom_field_flag", buf);
    }

  printf ("SEI Decoded Picture Buffer Management Repetition\n");
  printf ("original_idr_flag = %d\n", original_idr_flag);
  printf ("original_frame_num = %d\n", original_frame_num);

  // we need to save everything that is probably overwritten in dec_ref_pic_marking()
  old_drpm = pSlice->dec_ref_pic_marking_buffer;
  old_idr_flag = pSlice->idr_flag; 

  old_no_output_of_prior_pics_flag = pSlice->no_output_of_prior_pics_flag; //vidParam->no_output_of_prior_pics_flag;
  old_long_term_reference_flag = pSlice->long_term_reference_flag;
  old_adaptive_ref_pic_buffering_flag = pSlice->adaptive_ref_pic_buffering_flag;

  // set new initial values
  pSlice->idr_flag = original_idr_flag;
  pSlice->dec_ref_pic_marking_buffer = NULL;
  dec_ref_pic_marking (vidParam, buf, pSlice);
  //{{{  print out decoded values
  //if (vidParam->idr_flag)
  //{
    //printf("no_output_of_prior_pics_flag = %d\n", vidParam->no_output_of_prior_pics_flag);
    //printf("long_term_reference_flag     = %d\n", vidParam->long_term_reference_flag);
  //}
  //else
  //{
    //printf("adaptive_ref_pic_buffering_flag  = %d\n", vidParam->adaptive_ref_pic_buffering_flag);
    //if (vidParam->adaptive_ref_pic_buffering_flag)
    //{
      //tmp_drpm=vidParam->dec_ref_pic_marking_buffer;
      //while (tmp_drpm != NULL)
      //{
        //printf("memory_management_control_operation  = %d\n", tmp_drpm->memory_management_control_operation);

        //if ((tmp_drpm->memory_management_control_operation==1)||(tmp_drpm->memory_management_control_operation==3))
        //{
          //printf("difference_of_pic_nums_minus1        = %d\n", tmp_drpm->difference_of_pic_nums_minus1);
        //}
        //if (tmp_drpm->memory_management_control_operation==2)
        //{
          //printf("long_term_pic_num                    = %d\n", tmp_drpm->long_term_pic_num);
        //}
        //if ((tmp_drpm->memory_management_control_operation==3)||(tmp_drpm->memory_management_control_operation==6))
        //{
          //printf("long_term_frame_idx                  = %d\n", tmp_drpm->long_term_frame_idx);
        //}
        //if (tmp_drpm->memory_management_control_operation==4)
        //{
          //printf("max_long_term_pic_idx_plus1          = %d\n", tmp_drpm->max_long_term_frame_idx_plus1);
        //}
        //tmp_drpm = tmp_drpm->Next;
      //}
    //}
  //}
  //}}}

  while (pSlice->dec_ref_pic_marking_buffer) {
    tmp_drpm = pSlice->dec_ref_pic_marking_buffer;
    pSlice->dec_ref_pic_marking_buffer = tmp_drpm->Next;
    free (tmp_drpm);
    }

  // restore old values in vidParam
  pSlice->dec_ref_pic_marking_buffer = old_drpm;
  pSlice->idr_flag = old_idr_flag;
  pSlice->no_output_of_prior_pics_flag = old_no_output_of_prior_pics_flag;
  vidParam->no_output_of_prior_pics_flag = pSlice->no_output_of_prior_pics_flag;
  pSlice->long_term_reference_flag = old_long_term_reference_flag;
  pSlice->adaptive_ref_pic_buffering_flag = old_adaptive_ref_pic_buffering_flag;

  free (buf);
  }
//}}}

//{{{
static void interpret_full_frame_freeze_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int full_frame_freeze_repetition_period = read_ue_v ("SEI: full_frame_freeze_repetition_period", buf);
  printf ("full_frame_freeze_repetition_period = %d\n", full_frame_freeze_repetition_period);

  free (buf);
  }
//}}}
//{{{
static void interpret_full_frame_freeze_release_info (byte* payload, int size, sVidParam* vidParam ) {

  printf ("SEI Full-frame freeze release SEI\n");
  if (size)
    printf ("payload size of this message should be zero, but is %d bytes.\n", size);
  }
//}}}
//{{{
static void interpret_full_frame_snapshot_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int snapshot_id = read_ue_v("SEI: snapshot_id", buf);

  printf ("SEI Full-frame snapshot\n");
  printf ("snapshot_id = %d\n", snapshot_id);
  free (buf);
  }
//}}}

//{{{
static void interpret_progressive_refinement_start_info (byte* payload, int size, sVidParam* vidParam ) {


  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int progressive_refinement_id   = read_ue_v("SEI: progressive_refinement_id"  , buf);
  int num_refinement_steps_minus1 = read_ue_v("SEI: num_refinement_steps_minus1", buf);
  printf ("SEI Progressive refinement segment start\n");
  printf ("progressive_refinement_id   = %d\n", progressive_refinement_id);
  printf ("num_refinement_steps_minus1 = %d\n", num_refinement_steps_minus1);

  free (buf);
  }
//}}}
//{{{
static void interpret_progressive_refinement_end_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int progressive_refinement_id   = read_ue_v("SEI: progressive_refinement_id"  , buf);
  printf ("SEI Progressive refinement segment end\n");
  printf ("progressive_refinement_id   = %d\n", progressive_refinement_id);
  free (buf);
}
//}}}

//{{{
static void interpret_motion_constrained_slice_group_set_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int num_slice_groups_minus1 = read_ue_v ("SEI: num_slice_groups_minus1" , buf);
  int sliceGroupSize = ceilLog2 (num_slice_groups_minus1 + 1);
  printf ("Motion-constrained slice group set SEI message\n");
  printf ("num_slice_groups_minus1 = %d\n", num_slice_groups_minus1);

  for (int i = 0; i <= num_slice_groups_minus1; i++) {
    int slice_group_id = read_u_v (sliceGroupSize, "SEI: slice_group_id" , buf);
    printf ("slice_group_id = %d\n", slice_group_id);
    }

  int exact_match_flag = read_u_1 ("SEI: exact_match_flag"  , buf);
  int pan_scan_rect_flag = read_u_1 ("SEI: pan_scan_rect_flag"  , buf);

  printf ("exact_match_flag = %d\n", exact_match_flag);
  printf ("pan_scan_rect_flag = %d\n", pan_scan_rect_flag);

  if (pan_scan_rect_flag) {
    int pan_scan_rect_id = read_ue_v ("SEI: pan_scan_rect_id"  , buf);
    printf ("pan_scan_rect_id = %d\n", pan_scan_rect_id);
    }

  free (buf);
  }
//}}}
//{{{
static void interpret_film_grain_characteristics_info (byte* payload, int size, sVidParam* vidParam ) {

  int film_grain_characteristics_cancel_flag;
  int model_id, separate_colour_description_present_flag;
  int film_grain_bit_depth_luma_minus8, film_grain_bit_depth_chroma_minus8, film_grain_full_range_flag, film_grain_colour_primaries, film_grain_transfer_characteristics, film_grain_matrix_coefficients;
  int blending_mode_id, log2_scale_factor, comp_model_present_flag[3];
  int num_intensity_intervals_minus1, num_model_values_minus1;
  int intensity_interval_lower_bound, intensity_interval_upper_bound;
  int comp_model_value;
  int film_grain_characteristics_repetition_period;

  Bitstream* buf;

  buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  film_grain_characteristics_cancel_flag = read_u_1 ("SEI: film_grain_characteristics_cancel_flag", buf);
  printf ("film_grain_characteristics_cancel_flag = %d\n", film_grain_characteristics_cancel_flag);
  if (!film_grain_characteristics_cancel_flag) {
    model_id = read_u_v(2, "SEI: model_id", buf);
    separate_colour_description_present_flag = read_u_1("SEI: separate_colour_description_present_flag", buf);
    printf ("model_id = %d\n", model_id);
    printf ("separate_colour_description_present_flag = %d\n", separate_colour_description_present_flag);

    if (separate_colour_description_present_flag) {
      film_grain_bit_depth_luma_minus8 = read_u_v(3, "SEI: film_grain_bit_depth_luma_minus8", buf);
      film_grain_bit_depth_chroma_minus8 = read_u_v(3, "SEI: film_grain_bit_depth_chroma_minus8", buf);
      film_grain_full_range_flag = read_u_v(1, "SEI: film_grain_full_range_flag", buf);
      film_grain_colour_primaries = read_u_v(8, "SEI: film_grain_colour_primaries", buf);
      film_grain_transfer_characteristics = read_u_v(8, "SEI: film_grain_transfer_characteristics", buf);
      film_grain_matrix_coefficients = read_u_v(8, "SEI: film_grain_matrix_coefficients", buf);
      printf ("film_grain_bit_depth_luma_minus8 = %d\n", film_grain_bit_depth_luma_minus8);
      printf ("film_grain_bit_depth_chroma_minus8 = %d\n", film_grain_bit_depth_chroma_minus8);
      printf ("film_grain_full_range_flag = %d\n", film_grain_full_range_flag);
      printf ("film_grain_colour_primaries = %d\n", film_grain_colour_primaries);
      printf ("film_grain_transfer_characteristics = %d\n", film_grain_transfer_characteristics);
      printf ("film_grain_matrix_coefficients = %d\n", film_grain_matrix_coefficients);
      }

    blending_mode_id = read_u_v (2, "SEI: blending_mode_id", buf);
    log2_scale_factor = read_u_v (4, "SEI: log2_scale_factor", buf);
    printf ("blending_mode_id = %d\n", blending_mode_id);
    printf ("log2_scale_factor = %d\n", log2_scale_factor);
    for (int c = 0; c < 3; c ++) {
      comp_model_present_flag[c] = read_u_1("SEI: comp_model_present_flag", buf);
      printf ("comp_model_present_flag = %d\n", comp_model_present_flag[c]);
      }

    for (int c = 0; c < 3; c ++)
      if (comp_model_present_flag[c]) {
        num_intensity_intervals_minus1 = read_u_v(8, "SEI: num_intensity_intervals_minus1", buf);
        num_model_values_minus1 = read_u_v(3, "SEI: num_model_values_minus1", buf);
        printf("num_intensity_intervals_minus1 = %d\n", num_intensity_intervals_minus1);
        printf("num_model_values_minus1 = %d\n", num_model_values_minus1);
        for (int i = 0; i <= num_intensity_intervals_minus1; i ++) {
          intensity_interval_lower_bound = read_u_v(8, "SEI: intensity_interval_lower_bound", buf);
          intensity_interval_upper_bound = read_u_v(8, "SEI: intensity_interval_upper_bound", buf);
          printf ("intensity_interval_lower_bound = %d\n", intensity_interval_lower_bound);
          printf ("intensity_interval_upper_bound = %d\n", intensity_interval_upper_bound);
          for (int j = 0; j <= num_model_values_minus1; j++) {
            comp_model_value = read_se_v("SEI: comp_model_value", buf);
            printf ("comp_model_value = %d\n", comp_model_value);
            }
          }
        }
    film_grain_characteristics_repetition_period =
      read_ue_v("SEI: film_grain_characteristics_repetition_period", buf);
    printf ("film_grain_characteristics_repetition_period = %d\n",
            film_grain_characteristics_repetition_period);
    }

  free (buf);
  }
//}}}
//{{{
static void interpret_deblocking_filter_display_preference_info (byte* payload, int size, sVidParam* vidParam) {

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int deblocking_display_preference_cancel_flag = read_u_1("SEI: deblocking_display_preference_cancel_flag", buf);
  printf ("deblocking_display_preference_cancel_flag = %d\n", deblocking_display_preference_cancel_flag);
  if (!deblocking_display_preference_cancel_flag) {
    int display_prior_to_deblocking_preferred_flag = read_u_1("SEI: display_prior_to_deblocking_preferred_flag", buf);
    int dec_frame_buffering_constraint_flag = read_u_1("SEI: dec_frame_buffering_constraint_flag", buf);
    int deblocking_display_preference_repetition_period = read_ue_v("SEI: deblocking_display_preference_repetition_period", buf);

    printf ("display_prior_to_deblocking_preferred_flag = %d\n", display_prior_to_deblocking_preferred_flag);
    printf ("dec_frame_buffering_constraint_flag = %d\n", dec_frame_buffering_constraint_flag);
    printf ("deblocking_display_preference_repetition_period = %d\n", deblocking_display_preference_repetition_period);
    }

  free (buf);
  }
//}}}
//{{{
static void interpret_stereo_video_info_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf = malloc (sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int field_views_flags = read_u_1 ("SEI: field_views_flags", buf);
  printf("field_views_flags = %d\n", field_views_flags);
  if (field_views_flags) {
    int top_field_is_left_view_flag = read_u_1 ("SEI: top_field_is_left_view_flag", buf);
    printf ("top_field_is_left_view_flag = %d\n", top_field_is_left_view_flag);
    }
  else {
    int current_frame_is_left_view_flag = read_u_1 ("SEI: current_frame_is_left_view_flag", buf);
    int next_frame_is_second_view_flag = read_u_1 ("SEI: next_frame_is_second_view_flag", buf);
    printf ("current_frame_is_left_view_flag = %d\n", current_frame_is_left_view_flag);
    printf ("next_frame_is_second_view_flag = %d\n", next_frame_is_second_view_flag);
    }

  int left_view_self_contained_flag = read_u_1 ("SEI: left_view_self_contained_flag", buf);
  int right_view_self_contained_flag = read_u_1 ("SEI: right_view_self_contained_flag", buf);
  printf ("left_view_self_contained_flag = %d\n", left_view_self_contained_flag);
  printf ("right_view_self_contained_flag = %d\n", right_view_self_contained_flag);

  free (buf);
  }
//}}}
//{{{
static void interpret_buffering_period_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  int seq_parameter_set_id = read_ue_v("SEI: seq_parameter_set_id"  , buf);
  sSPSrbsp* sps = &vidParam->SeqParSet[seq_parameter_set_id];
  activateSPS (vidParam, sps);

  printf ("Buffering period SEI message\n");
  printf ("seq_parameter_set_id   = %d\n", seq_parameter_set_id);

  // Note: NalHrdBpPresentFlag and CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
  if (sps->vui_parameters_present_flag) {
    int initial_cpb_removal_delay;
    int initial_cpb_removal_delay_offset;
    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag) {
      for (unsigned k = 0; k < sps->vui_seq_parameters.nal_hrd_parameters.cpb_cnt_minus1+1; k++) {
        initial_cpb_removal_delay = read_u_v(sps->vui_seq_parameters.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI: initial_cpb_removal_delay"        , buf);
        initial_cpb_removal_delay_offset = read_u_v(sps->vui_seq_parameters.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI: initial_cpb_removal_delay_offset" , buf);

        printf ("nal initial_cpb_removal_delay[%d] = %d\n", k, initial_cpb_removal_delay);
        printf ("nal initial_cpb_removal_delay_offset[%d] = %d\n", k, initial_cpb_removal_delay_offset);
        }
      }

    if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag) {
      for (unsigned k = 0; k < sps->vui_seq_parameters.vcl_hrd_parameters.cpb_cnt_minus1+1; k++) {
        initial_cpb_removal_delay = read_u_v(sps->vui_seq_parameters.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI: initial_cpb_removal_delay"        , buf);
        initial_cpb_removal_delay_offset = read_u_v(sps->vui_seq_parameters.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI: initial_cpb_removal_delay_offset" , buf);

        printf ("vcl initial_cpb_removal_delay[%d] = %d\n", k, initial_cpb_removal_delay);
        printf ("vcl initial_cpb_removal_delay_offset[%d] = %d\n", k, initial_cpb_removal_delay_offset);
        }
      }
    }

  free (buf);
  }
//}}}
//{{{
static void interpret_frame_packing_arrangement_info (byte* payload, int size, sVidParam* vidParam ) {

  frame_packing_arrangement_information_struct seiFramePackingArrangement;
  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  printf ("Frame packing arrangement SEI message\n");

  seiFramePackingArrangement.frame_packing_arrangement_id =
    (unsigned int)read_ue_v( "SEI: frame_packing_arrangement_id", buf);
  seiFramePackingArrangement.frame_packing_arrangement_cancel_flag =
    read_u_1( "SEI: frame_packing_arrangement_cancel_flag", buf);
  printf("frame_packing_arrangement_id = %d\n", seiFramePackingArrangement.frame_packing_arrangement_id);
  printf("frame_packing_arrangement_cancel_flag = %d\n", seiFramePackingArrangement.frame_packing_arrangement_cancel_flag);
  if ( seiFramePackingArrangement.frame_packing_arrangement_cancel_flag == FALSE ) {
    seiFramePackingArrangement.frame_packing_arrangement_type =
      (unsigned char)read_u_v( 7, "SEI: frame_packing_arrangement_type", buf);
    seiFramePackingArrangement.quincunx_sampling_flag =
      read_u_1 ("SEI: quincunx_sampling_flag", buf );
    seiFramePackingArrangement.content_interpretation_type =
      (unsigned char)read_u_v( 6, "SEI: content_interpretation_type", buf);
    seiFramePackingArrangement.spatial_flipping_flag = read_u_1 ("SEI: spatial_flipping_flag", buf);
    seiFramePackingArrangement.frame0_flipped_flag = read_u_1 ("SEI: frame0_flipped_flag", buf);
    seiFramePackingArrangement.field_views_flag = read_u_1 ("SEI: field_views_flag", buf);
    seiFramePackingArrangement.current_frame_is_frame0_flag =
      read_u_1 ("SEI: current_frame_is_frame0_flag", buf);
    seiFramePackingArrangement.frame0_self_contained_flag = read_u_1 ("SEI: frame0_self_contained_flag", buf);
    seiFramePackingArrangement.frame1_self_contained_flag = read_u_1 ("SEI: frame1_self_contained_flag", buf);

    printf ("frame_packing_arrangement_type    = %d\n", seiFramePackingArrangement.frame_packing_arrangement_type);
    printf ("quincunx_sampling_flag            = %d\n", seiFramePackingArrangement.quincunx_sampling_flag);
    printf ("content_interpretation_type       = %d\n", seiFramePackingArrangement.content_interpretation_type);
    printf ("spatial_flipping_flag             = %d\n", seiFramePackingArrangement.spatial_flipping_flag);
    printf ("frame0_flipped_flag               = %d\n", seiFramePackingArrangement.frame0_flipped_flag);
    printf ("field_views_flag                  = %d\n", seiFramePackingArrangement.field_views_flag);
    printf ("current_frame_is_frame0_flag      = %d\n", seiFramePackingArrangement.current_frame_is_frame0_flag);
    printf ("frame0_self_contained_flag        = %d\n", seiFramePackingArrangement.frame0_self_contained_flag);
    printf ("frame1_self_contained_flag        = %d\n", seiFramePackingArrangement.frame1_self_contained_flag);
    if (seiFramePackingArrangement.quincunx_sampling_flag == FALSE &&
        seiFramePackingArrangement.frame_packing_arrangement_type != 5 )  {
      seiFramePackingArrangement.frame0_grid_position_x =
        (unsigned char)read_u_v( 4, "SEI: frame0_grid_position_x", buf);
      seiFramePackingArrangement.frame0_grid_position_y =
        (unsigned char)read_u_v( 4, "SEI: frame0_grid_position_y", buf);
      seiFramePackingArrangement.frame1_grid_position_x =
        (unsigned char)read_u_v( 4, "SEI: frame1_grid_position_x", buf);
      seiFramePackingArrangement.frame1_grid_position_y =
        (unsigned char)read_u_v( 4, "SEI: frame1_grid_position_y", buf);

      printf ("frame0_grid_position_x = %d\n", seiFramePackingArrangement.frame0_grid_position_x);
      printf ("frame0_grid_position_y = %d\n", seiFramePackingArrangement.frame0_grid_position_y);
      printf ("frame1_grid_position_x = %d\n", seiFramePackingArrangement.frame1_grid_position_x);
      printf ("frame1_grid_position_y = %d\n", seiFramePackingArrangement.frame1_grid_position_y);
    }
    seiFramePackingArrangement.frame_packing_arrangement_reserved_byte = (unsigned char)read_u_v( 8, "SEI: frame_packing_arrangement_reserved_byte", buf);
    seiFramePackingArrangement.frame_packing_arrangement_repetition_period = (unsigned int)read_ue_v( "SEI: frame_packing_arrangement_repetition_period", buf);
    printf("frame_packing_arrangement_reserved_byte = %d\n", seiFramePackingArrangement.frame_packing_arrangement_reserved_byte);
    printf("frame_packing_arrangement_repetition_period  = %d\n", seiFramePackingArrangement.frame_packing_arrangement_repetition_period);
  }
  seiFramePackingArrangement.frame_packing_arrangement_extension_flag = read_u_1( "SEI: frame_packing_arrangement_extension_flag", buf);
  printf("frame_packing_arrangement_extension_flag = %d\n", seiFramePackingArrangement.frame_packing_arrangement_extension_flag);

  free (buf);
}
//}}}

//{{{
static void interpret_post_filter_hints_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  unsigned int filter_hint_size_y = read_ue_v("SEI: filter_hint_size_y", buf); // interpret post-filter hint SEI here
  unsigned int filter_hint_size_x = read_ue_v("SEI: filter_hint_size_x", buf); // interpret post-filter hint SEI here
  unsigned int filter_hint_type = read_u_v(2, "SEI: filter_hint_type", buf); // interpret post-filter hint SEI here

  int** *filter_hint;
  get_mem3Dint (&filter_hint, 3, filter_hint_size_y, filter_hint_size_x);

  for (unsigned int color_component = 0; color_component < 3; color_component ++)
    for (unsigned int cy = 0; cy < filter_hint_size_y; cy ++)
      for (unsigned int cx = 0; cx < filter_hint_size_x; cx ++)
        filter_hint[color_component][cy][cx] = read_se_v("SEI: filter_hint", buf); // interpret post-filter hint SEI here

  unsigned int additional_extension_flag = read_u_1("SEI: additional_extension_flag", buf); // interpret post-filter hint SEI here

  printf(" Post-filter hint SEI message\n");
  printf(" post_filter_hint_size_y %d \n", filter_hint_size_y);
  printf(" post_filter_hint_size_x %d \n", filter_hint_size_x);
  printf(" post_filter_hint_type %d \n",   filter_hint_type);
  for (unsigned int color_component = 0; color_component < 3; color_component ++)
    for (unsigned int cy = 0; cy < filter_hint_size_y; cy ++)
      for (unsigned int cx = 0; cx < filter_hint_size_x; cx ++)
        printf (" post_filter_hint[%d][%d][%d] %d \n", color_component, cy, cx, filter_hint[color_component][cy][cx]);

  printf (" additional_extension_flag %d \n", additional_extension_flag);

  free_mem3Dint (filter_hint);
  free( buf);
  }
//}}}
//{{{
static void interpret_green_metadata_info (byte* payload, int size, sVidParam* vidParam ) {

  Bitstream* buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  printf ("Green Metadata Info SEI message\n");

  Green_metadata_information_struct seiGreenMetadataInfo;
  seiGreenMetadataInfo.green_metadata_type = (unsigned char)read_u_v(8, "SEI: green_metadata_type", buf);
  printf ("green_metadata_type = %d\n", seiGreenMetadataInfo.green_metadata_type);

  if (seiGreenMetadataInfo.green_metadata_type == 0) {
      seiGreenMetadataInfo.period_type=(unsigned char)read_u_v(8, "SEI: green_metadata_period_type", buf);
    printf ("green_metadata_period_type = %d\n", seiGreenMetadataInfo.period_type);

    if (seiGreenMetadataInfo.period_type == 2) {
      seiGreenMetadataInfo.num_seconds = (unsigned short)read_u_v(16, "SEI: green_metadata_num_seconds", buf);
      printf ("green_metadata_num_seconds = %d\n", seiGreenMetadataInfo.num_seconds);
      }
    else if (seiGreenMetadataInfo.period_type == 3) {
      seiGreenMetadataInfo.num_pictures = (unsigned short)read_u_v(16, "SEI: green_metadata_num_pictures", buf);
      printf ("green_metadata_num_pictures = %d\n", seiGreenMetadataInfo.num_pictures);
      }

    seiGreenMetadataInfo.percent_non_zero_macroblocks =
      (unsigned char)read_u_v(8, "SEI: percent_non_zero_macroblocks", buf);
    seiGreenMetadataInfo.percent_intra_coded_macroblocks =
      (unsigned char)read_u_v(8, "SEI: percent_intra_coded_macroblocks", buf);
    seiGreenMetadataInfo.percent_six_tap_filtering =
      (unsigned char)read_u_v(8, "SEI: percent_six_tap_filtering", buf);
    seiGreenMetadataInfo.percent_alpha_point_deblocking_instance =
      (unsigned char)read_u_v(8, "SEI: percent_alpha_point_deblocking_instance", buf);

    printf ("percent_non_zero_macroblocks = %f\n", (float)seiGreenMetadataInfo.percent_non_zero_macroblocks/255);
    printf ("percent_intra_coded_macroblocks = %f\n", (float)seiGreenMetadataInfo.percent_intra_coded_macroblocks/255);
    printf ("percent_six_tap_filtering = %f\n", (float)seiGreenMetadataInfo.percent_six_tap_filtering/255);
    printf ("percent_alpha_point_deblocking_instance      = %f\n", (float)seiGreenMetadataInfo.percent_alpha_point_deblocking_instance/255);
    }
  else if (seiGreenMetadataInfo.green_metadata_type == 1) {
    seiGreenMetadataInfo.xsd_metric_type = (unsigned char)read_u_v(8, "SEI: xsd_metric_type", buf);
    seiGreenMetadataInfo.xsd_metric_value = (unsigned short)read_u_v(16, "SEI: xsd_metric_value", buf);
    printf ("xsd_metric_type      = %d\n", seiGreenMetadataInfo.xsd_metric_type);
    if (seiGreenMetadataInfo.xsd_metric_type == 0)
      printf("xsd_metric_value      = %f\n", (float)seiGreenMetadataInfo.xsd_metric_value/100);
    }

  free (buf);
  }
//}}}

//{{{
void InterpretSEIMessage (byte* msg, int size, sVidParam* vidParam, sSlice *pSlice) {

  int offset = 1;
  do {
    int payload_type = 0;
    byte tmp_byte = msg[offset++];
    while (tmp_byte == 0xFF) {
      payload_type += 255;
      tmp_byte = msg[offset++];
      }
    payload_type += tmp_byte;   // this is the last byte

    int payload_size = 0;
    tmp_byte = msg[offset++];
    while (tmp_byte == 0xFF) {
      payload_size += 255;
      tmp_byte = msg[offset++];
      }
    payload_size += tmp_byte;   // this is the last byte

    switch (payload_type) {
      case  SEI_BUFFERING_PERIOD:
        interpret_buffering_period_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_PIC_TIMING:
        interpret_picture_timing_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_PAN_SCAN_RECT:
        interpret_pan_scan_rect_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_FILLER_PAYLOAD:
        interpret_filler_payload_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_USER_DATA_REGISTERED_ITU_T_T35:
        interpret_user_data_registered_itu_t_t35_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_USER_DATA_UNREGISTERED:
        interpret_user_data_unregistered_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_RECOVERY_POINT:
        interpret_recovery_point_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_DEC_REF_PIC_MARKING_REPETITION:
        interpret_dec_ref_pic_marking_repetition_info (msg+offset, payload_size, vidParam, pSlice ); break;
      case  SEI_SPARE_PIC:
        interpret_spare_pic (msg+offset, payload_size, vidParam ); break;
      case  SEI_SCENE_INFO:
        interpret_scene_information (msg+offset, payload_size, vidParam ); break;
      case  SEI_SUB_SEQ_INFO:
        interpret_subsequence_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_SUB_SEQ_LAYER_CHARACTERISTICS:
        interpret_subsequence_layer_characteristics_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_SUB_SEQ_CHARACTERISTICS:
        interpret_subsequence_characteristics_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_FULL_FRAME_FREEZE:
        interpret_full_frame_freeze_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_FULL_FRAME_FREEZE_RELEASE:
        interpret_full_frame_freeze_release_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_FULL_FRAME_SNAPSHOT:
        interpret_full_frame_snapshot_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START:
        interpret_progressive_refinement_start_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END:
        interpret_progressive_refinement_end_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET:
        interpret_motion_constrained_slice_group_set_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_FILM_GRAIN_CHARACTERISTICS:
        interpret_film_grain_characteristics_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE:
        interpret_deblocking_filter_display_preference_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_STEREO_VIDEO_INFO:
        interpret_stereo_video_info_info  (msg+offset, payload_size, vidParam ); break;
      case  SEI_POST_FILTER_HINTS:
        interpret_post_filter_hints_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_FRAME_PACKING_ARRANGEMENT:
        interpret_frame_packing_arrangement_info (msg+offset, payload_size, vidParam ); break;
      case  SEI_GREEN_METADATA:
        interpret_green_metadata_info (msg+offset, payload_size, vidParam ); break;
      default:
        interpret_reserved_info (msg+offset, payload_size, vidParam ); break;
      }
    offset += payload_size;
    } while (msg[offset] != 0x80);    // more_rbsp_data()  msg[offset] != 0x80

  // ignore the trailing bits rbsp_trailing_bits();
  assert(msg[offset] == 0x80);      // this is the trailing bits
  assert (offset+1 == size );
  }
//}}}
