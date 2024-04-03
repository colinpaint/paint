//{{{  includes
#include <math.h>

#include "global.h"
#include "memory.h"

#include "sei.h"
#include "vlc.h"
#include "buffer.h"
#include "sps.h"
#include "image.h"
//}}}

#define MAX_FN  256
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
//{{{  frame_packing_arrangement_information_struct
typedef struct {
  uint32_t  frame_packing_arrangement_id;
  bool       frame_packing_arrangement_cancel_flag;
  uint8_t frame_packing_arrangement_type;
  bool       quincunx_sampling_flag;
  uint8_t content_interpretation_type;
  bool       spatial_flipping_flag;
  bool       frame0_flipped_flag;
  bool       field_views_flag;
  bool       current_frame_is_frame0_flag;
  bool       frame0_self_contained_flag;
  bool       frame1_self_contained_flag;
  uint8_t frame0_grid_position_x;
  uint8_t frame0_grid_position_y;
  uint8_t frame1_grid_position_x;
  uint8_t frame1_grid_position_y;
  uint8_t frame_packing_arrangement_reserved_byte;
  uint32_t  frame_packing_arrangement_repetition_period;
  bool       frame_packing_arrangement_extension_flag;
  } frame_packing_arrangement_information_struct;
//}}}
//{{{  Green_metadata_information_struct
typedef struct {
  uint8_t  green_metadata_type;
  uint8_t  period_type;
  uint16_t num_seconds;
  uint16_t num_pictures;
  uint8_t percent_non_zero_macroblocks;
  uint8_t percent_intra_coded_macroblocks;
  uint8_t percent_six_tap_filtering;
  uint8_t percent_alpha_point_deblocking_instance;
  uint8_t xsd_metric_type;
  uint16_t xsd_metric_value;
  } Green_metadata_information_struct;
//}}}

//{{{
static void processUserDataUnregistered (byte* payload, int size, sDecoder* decoder) {

  if (decoder->param.seiDebug) {
    printf ("unregistered - ");

    for (int i = 0; i < size; i++)
      if (payload[i] >= 0x20 && payload[i] <= 0x7f)
        printf ("%c",payload[i]);
      else
        printf ("<%02x>",payload[i]);

    printf ("\n");
    }
  }
//}}}
//{{{
static void processUserDataT35 (byte* payload, int size, sDecoder* decoder) {

  if (decoder->param.seiDebug) {
    int offset = 0;
    int itu_t_t35_country_code = payload[offset++];
    printf("ITU-T:%d", itu_t_t35_country_code);

    if (itu_t_t35_country_code == 0xFF) {
      int itu_t_t35_country_code_extension_byte = payload[offset++];
      printf (" ext:%d - ", itu_t_t35_country_code_extension_byte);
      }

    for (int i = offset; i < size; i++)
      if (payload[i] >= 0x20 && payload[i] <= 0x7f)
        printf ("%c",payload[i]);
      else
        printf ("<%02x>",payload[i]);

    printf ("\n");
    }
  }
//}}}
//{{{
static void processReserved (byte* payload, int size, sDecoder* decoder) {

  if (decoder->param.seiDebug) {
    printf ("reserved - ");

    for (int i = 0; i < size; i++)
      if (payload[i] >= 0x20 && payload[i] <= 0x7f)
        printf ("%c",payload[i]);
      else
        printf ("<%02x>",payload[i]);

    printf ("\n");
    }
  }
//}}}

//{{{
static void processPictureTiming (byte* payload, int size, sDecoder* decoder) {

  sSps* activeSps = decoder->activeSps;
  if (!activeSps) {
    if (decoder->param.seiDebug)
      printf ("pictureTiming - no active SPS\n");
    return;
    }

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  if (decoder->param.seiDebug) {
    printf ("pictureTiming");

    int cpb_removal_len = 24;
    int dpb_output_len = 24;
    bool cpbDpb = (bool)(activeSps->hasVui &&
                               (activeSps->vuiSeqParams.nal_hrd_parameters_present_flag ||
                                activeSps->vuiSeqParams.vcl_hrd_parameters_present_flag));

    if (cpbDpb) {
      if (activeSps->hasVui) {
        if (activeSps->vuiSeqParams.nal_hrd_parameters_present_flag) {
          cpb_removal_len = activeSps->vuiSeqParams.nal_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
          dpb_output_len  = activeSps->vuiSeqParams.nal_hrd_parameters.dpb_output_delay_length_minus1  + 1;
          }
        else if (activeSps->vuiSeqParams.vcl_hrd_parameters_present_flag) {
          cpb_removal_len = activeSps->vuiSeqParams.vcl_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
          dpb_output_len = activeSps->vuiSeqParams.vcl_hrd_parameters.dpb_output_delay_length_minus1  + 1;
          }
        }

      if ((activeSps->vuiSeqParams.nal_hrd_parameters_present_flag) ||
          (activeSps->vuiSeqParams.vcl_hrd_parameters_present_flag)) {
        int cpb_removal_delay, dpb_output_delay;
        cpb_removal_delay = readUv (cpb_removal_len, "SEI cpb_removal_delay", buf);
        dpb_output_delay = readUv (dpb_output_len,  "SEI dpb_output_delay", buf);
        if (decoder->param.seiDebug) {
          printf (" cpb:%d", cpb_removal_delay);
          printf (" dpb:%d", dpb_output_delay);
          }
        }
      }

    int pic_struct_present_flag, pic_struct;
    if (!activeSps->hasVui)
      pic_struct_present_flag = 0;
    else
      pic_struct_present_flag = activeSps->vuiSeqParams.pic_struct_present_flag;

    int numClockTs = 0;
    if (pic_struct_present_flag) {
      pic_struct = readUv (4, "SEI pic_struct" , buf);
      switch (pic_struct) {
        case 0:
        case 1:
        //{{{
        case 2:
          numClockTs = 1;
          break;
        //}}}
        case 3:
        case 4:
        //{{{
        case 7:
          numClockTs = 2;
          break;
        //}}}
        case 5:
        case 6:
        //{{{
        case 8:
          numClockTs = 3;
          break;
        //}}}
        //{{{
        default:
          error ("reserved pic_struct used, can't determine numClockTs");
        //}}}
        }

      if (decoder->param.seiDebug)
        for (int i = 0; i < numClockTs; i++) {
          //{{{  print
          int clock_timestamp_flag = readU1 ("SEI clock_timestamp_flag", buf);
          if (clock_timestamp_flag) {
            printf (" clockTs");
            int ct_type = readUv (2, "SEI ct_type", buf);
            int nuit_field_based_flag = readU1 ("SEI nuit_field_based_flag", buf);
            int counting_type = readUv (5, "SEI counting_type", buf);
            int full_timestamp_flag = readU1 ("SEI full_timestamp_flag", buf);
            int discontinuity_flag = readU1 ("SEI discontinuity_flag", buf);
            int cnt_dropped_flag = readU1 ("SEI cnt_dropped_flag", buf);
            int nframes = readUv (8, "SEI nframes", buf);

            printf ("ct:%d",ct_type);
            printf ("nuit:%d",nuit_field_based_flag);
            printf ("full:%d",full_timestamp_flag);
            printf ("dis:%d",discontinuity_flag);
            printf ("drop:%d",cnt_dropped_flag);
            printf ("nframes:%d",nframes);

            if (full_timestamp_flag) {
              int seconds_value = readUv (6, "SEI seconds_value", buf);
              int minutes_value = readUv (6, "SEI minutes_value", buf);
              int hours_value = readUv (5, "SEI hours_value", buf);
              printf ("sec:%d", seconds_value);
              printf ("min:%d", minutes_value);
              printf ("hour:%d", hours_value);
              }
            else {
              int seconds_flag = readU1 ("SEI seconds_flag", buf);
              printf ("sec:%d",seconds_flag);
              if (seconds_flag) {
                int seconds_value = readUv (6, "SEI seconds_value", buf);
                int minutes_flag = readU1 ("SEI minutes_flag", buf);
                printf ("sec:%d",seconds_value);
                printf ("min:%d",minutes_flag);
                if(minutes_flag) {
                  int minutes_value = readUv(6, "SEI minutes_value", buf);
                  int hours_flag = readU1 ("SEI hours_flag", buf);
                  printf ("min:%d",minutes_value);
                  printf ("hour%d",hours_flag);
                  if (hours_flag) {
                    int hours_value = readUv (5, "SEI hours_value", buf);
                    printf ("hour:%d\n",hours_value);
                    }
                  }
                }
              }

            int time_offset_length;
            int time_offset;
            if (activeSps->vuiSeqParams.vcl_hrd_parameters_present_flag)
              time_offset_length = activeSps->vuiSeqParams.vcl_hrd_parameters.time_offset_length;
            else if (activeSps->vuiSeqParams.nal_hrd_parameters_present_flag)
              time_offset_length = activeSps->vuiSeqParams.nal_hrd_parameters.time_offset_length;
            else
              time_offset_length = 24;
            if (time_offset_length)
              time_offset = readIv (time_offset_length, "SEI time_offset", buf);
            else
              time_offset = 0;
            printf ("time:%d", time_offset);
            }
          }
          //}}}
      }
    printf ("\n");
    }

  free (buf);
  }
//}}}
//{{{
static void processPanScan (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int pan_scan_rect_id = readUeV ("SEI pan_scan_rect_id", buf);
  int pan_scan_rect_cancel_flag = readU1 ("SEI pan_scan_rect_cancel_flag", buf);
  if (!pan_scan_rect_cancel_flag) {
    int pan_scan_cnt_minus1 = readUeV ("SEI pan_scan_cnt_minus1", buf);
    for (int i = 0; i <= pan_scan_cnt_minus1; i++) {
      int pan_scan_rect_left_offset = readSeV ("SEI pan_scan_rect_left_offset", buf);
      int pan_scan_rect_right_offset = readSeV ("SEI pan_scan_rect_right_offset", buf);
      int pan_scan_rect_top_offset = readSeV ("SEI pan_scan_rect_top_offset", buf);
      int pan_scan_rect_bottom_offset = readSeV ("SEI pan_scan_rect_bottom_offset", buf);
      if (decoder->param.seiDebug)
        printf ("panScan %d/%d id:%d left:%d right:%d top:%d bot:%d\n",
                i, pan_scan_cnt_minus1, pan_scan_rect_id,
                pan_scan_rect_left_offset, pan_scan_rect_right_offset,
                pan_scan_rect_top_offset, pan_scan_rect_bottom_offset);
      }
    int pan_scan_rect_repetition_period = readUeV ("SEI pan_scan_rect_repetition_period", buf);
    }

  free (buf);
  }
//}}}
//{{{
static void processRecoveryPoint (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int recoveryFrameCount = readUeV ("SEI recoveryFrameCount", buf);
  int exact_match_flag = readU1 ("SEI exact_match_flag", buf);
  int broken_link_flag = readU1 ("SEI broken_link_flag", buf);
  int changing_slice_group_idc = readUv (2, "SEI changing_slice_group_idc", buf);

  decoder->recoveryPoint = 1;
  decoder->recoveryFrameCount = recoveryFrameCount;

  if (decoder->param.seiDebug)
    printf ("recovery - frame:%d exact:%d broken:%d changing:%d\n",
            recoveryFrameCount, exact_match_flag, broken_link_flag, changing_slice_group_idc);
  free (buf);
  }
//}}}
//{{{
static void processDecRefPicMarkingRepetition (byte* payload, int size, sDecoder* decoder, sSlice *slice) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int original_idr_flag = readU1 ("SEI original_idr_flag", buf);
  int original_frame_num = readUeV ("SEI original_frame_num", buf);

  int original_bottom_field_flag = 0;
  if (!decoder->activeSps->frameMbOnly) {
    int original_field_pic_flag = readU1 ("SEI original_field_pic_flag", buf);
    if (original_field_pic_flag)
      original_bottom_field_flag = readU1 ("SEI original_bottom_field_flag", buf);
    }

  if (decoder->param.seiDebug)
    printf ("decPicRepetition orig:%d:%d", original_idr_flag, original_frame_num);

  free (buf);
  }
//}}}

//{{{
static void process_spare_pic (byte* payload, int size, sDecoder* decoder) {

  int x,y;
  sBitStream* buf;
  int bit0, bit1, no_bit0;
  int target_frame_num = 0;
  int num_spare_pics;
  int delta_spare_frame_num, CandidateSpareFrameNum, SpareFrameNum = 0;
  int ref_area_indicator;

  int m, n, left, right, top, bottom,directx, directy;
  byte*** map;

  buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  target_frame_num = readUeV ("SEI target_frame_num", buf);
  num_spare_pics = 1 + readUeV ("SEI num_spare_pics_minus1", buf);
  if (decoder->param.seiDebug)
    printf ("sparePicture target_frame_num:%d num_spare_pics:%d\n",
            target_frame_num, num_spare_pics);

  getMem3D (&map, num_spare_pics, decoder->coding.height >> 4, decoder->coding.width >> 4);
  for (int i = 0; i < num_spare_pics; i++) {
    if (i == 0) {
      CandidateSpareFrameNum = target_frame_num - 1;
      if (CandidateSpareFrameNum < 0)
        CandidateSpareFrameNum = MAX_FN - 1;
      }
    else
      CandidateSpareFrameNum = SpareFrameNum;

    delta_spare_frame_num = readUeV ("SEI delta_spare_frame_num", buf);
    SpareFrameNum = CandidateSpareFrameNum - delta_spare_frame_num;
    if (SpareFrameNum < 0 )
      SpareFrameNum = MAX_FN + SpareFrameNum;

    ref_area_indicator = readUeV ("SEI ref_area_indicator", buf);
    switch (ref_area_indicator) {
      //{{{
      case 0: // The whole frame can serve as spare picture
        for (y=0; y<decoder->coding.height >> 4; y++)
          for (x=0; x<decoder->coding.width >> 4; x++)
            map[i][y][x] = 0;
        break;
      //}}}
      //{{{
      case 1: // The map is not compressed
        for (y=0; y<decoder->coding.height >> 4; y++)
          for (x=0; x<decoder->coding.width >> 4; x++)
            map[i][y][x] = (byte) readU1("SEI ref_mb_indicator", buf);
        break;
      //}}}
      //{{{
      case 2: // The map is compressed
        bit0 = 0;
        bit1 = 1;
        no_bit0 = -1;

        x = ((decoder->coding.width >> 4) - 1 ) / 2;
        y = ((decoder->coding.height >> 4) - 1 ) / 2;
        left = right = x;
        top = bottom = y;
        directx = 0;
        directy = 1;

        for (m = 0; m < decoder->coding.height >> 4; m++)
          for (n = 0; n < decoder->coding.width >> 4; n++) {
            if (no_bit0 < 0)
              no_bit0 = readUeV ("SEI zero_run_length", buf);
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
              else if (x == (decoder->coding.width >> 4) - 1) {
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
              else if (y == (decoder->coding.height >> 4) - 1) {
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

  freeMem3D (map);
  free (buf);
  }
//}}}

//{{{
static void process_subsequence_info (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int sub_seq_layer_num = readUeV ("SEI sub_seq_layer_num", buf);
  int sub_seq_id = readUeV ("SEI sub_seq_id", buf);
  int first_ref_pic_flag = readU1 ("SEI first_ref_pic_flag", buf);
  int leading_non_ref_pic_flag = readU1 ("SEI leading_non_ref_pic_flag", buf);
  int last_pic_flag = readU1 ("SEI last_pic_flag", buf);

  int sub_seq_frame_num = 0;
  int sub_seq_frame_num_flag = readU1 ("SEI sub_seq_frame_num_flag", buf);
  if (sub_seq_frame_num_flag)
    sub_seq_frame_num = readUeV("SEI sub_seq_frame_num", buf);

  if (decoder->param.seiDebug) {
    printf ("Sub-sequence information\n");
    printf ("sub_seq_layer_num = %d\n", sub_seq_layer_num );
    printf ("sub_seq_id = %d\n", sub_seq_id);
    printf ("first_ref_pic_flag = %d\n", first_ref_pic_flag);
    printf ("leading_non_ref_pic_flag = %d\n", leading_non_ref_pic_flag);
    printf ("last_pic_flag = %d\n", last_pic_flag);
    printf ("sub_seq_frame_num_flag = %d\n", sub_seq_frame_num_flag);
    if (sub_seq_frame_num_flag)
      printf ("sub_seq_frame_num = %d\n", sub_seq_frame_num);
    }

  free  (buf);
  }
//}}}
//{{{
static void process_subsequence_layer_characteristics_info (byte* payload, int size, sDecoder* decoder)
{
  sBitStream* buf;
  long num_sub_layers, accurate_statistics_flag, average_bit_rate, average_frame_rate;

  buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamLen = size;
  buf->bitStreamOffset = 0;

  num_sub_layers = 1 + readUeV("SEI num_sub_layers_minus1", buf);

  if (decoder->param.seiDebug)
    printf ("Sub-sequence layer characteristics num_sub_layers_minus1 %ld\n", num_sub_layers - 1);
  for (int i = 0; i < num_sub_layers; i++) {
    accurate_statistics_flag = readU1 ("SEI accurate_statistics_flag", buf);
    average_bit_rate = readUv (16,"SEI average_bit_rate", buf);
    average_frame_rate = readUv (16,"SEI average_frame_rate", buf);

    if (decoder->param.seiDebug) {
      printf("layer %d: accurate_statistics_flag = %ld\n", i, accurate_statistics_flag);
      printf("layer %d: average_bit_rate = %ld\n", i, average_bit_rate);
      printf("layer %d: average_frame_rate= %ld\n", i, average_frame_rate);
      }
    }

  free (buf);
  }
//}}}
//{{{
static void process_subsequence_characteristics_info (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamLen = size;
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;

  int sub_seq_layer_num = readUeV ("SEI sub_seq_layer_num", buf);
  int sub_seq_id = readUeV ("SEI sub_seq_id", buf);
  int duration_flag = readU1 ("SEI duration_flag", buf);

  if (decoder->param.seiDebug) {
    printf ("Sub-sequence characteristics\n");
    printf ("sub_seq_layer_num %d\n", sub_seq_layer_num );
    printf ("sub_seq_id %d\n", sub_seq_id);
    printf ("duration_flag %d\n", duration_flag);
    }

  if (duration_flag) {
    int sub_seq_duration = readUv (32, "SEI duration_flag", buf);
    if (decoder->param.seiDebug)
      printf ("sub_seq_duration = %d\n", sub_seq_duration);
    }

  int average_rate_flag = readU1 ("SEI average_rate_flag", buf);
  if (decoder->param.seiDebug)
    printf ("average_rate_flag %d\n", average_rate_flag);
  if (average_rate_flag) {
    int accurate_statistics_flag = readU1 ("SEI accurate_statistics_flag", buf);
    int average_bit_rate = readUv (16, "SEI average_bit_rate", buf);
    int average_frame_rate = readUv (16, "SEI average_frame_rate", buf);
    if (decoder->param.seiDebug) {
      printf ("accurate_statistics_flag %d\n", accurate_statistics_flag);
      printf ("average_bit_rate %d\n", average_bit_rate);
      printf ("average_frame_rate %d\n", average_frame_rate);
      }
    }

  int num_referenced_subseqs  = readUeV("SEI num_referenced_subseqs", buf);
  if (decoder->param.seiDebug)
    printf ("num_referenced_subseqs %d\n", num_referenced_subseqs);
  for (int i = 0; i < num_referenced_subseqs; i++) {
    int ref_sub_seq_layer_num  = readUeV ("SEI ref_sub_seq_layer_num", buf);
    int ref_sub_seq_id = readUeV ("SEI ref_sub_seq_id", buf);
    int ref_sub_seq_direction = readU1  ("SEI ref_sub_seq_direction", buf);
    if (decoder->param.seiDebug) {
      printf ("ref_sub_seq_layer_num %d\n", ref_sub_seq_layer_num);
      printf ("ref_sub_seq_id %d\n", ref_sub_seq_id);
      printf ("ref_sub_seq_direction %d\n", ref_sub_seq_direction);
      }
    }
  free (buf);
  }
//}}}
//{{{
static void process_scene_information (byte* payload, int size, sDecoder* decoder) {


  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamLen = size;
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;

  int second_scene_id;
  int scene_id = readUeV ("SEI scene_id", buf);
  int scene_transition_type = readUeV ("SEI scene_transition_type", buf);
  if (scene_transition_type > 3)
    second_scene_id = readUeV ("SEI scene_transition_type", buf);

  if (decoder->param.seiDebug) {
    printf ("SceneInformation\n");
    printf ("scene_transition_type %d\n", scene_transition_type);
    printf ("scene_id %d\n", scene_id);
    if (scene_transition_type > 3 )
      printf ("second_scene_id %d\n", second_scene_id);
    }
  free (buf);
  }
//}}}

//{{{
static void process_filler_payload_info (byte* payload, int size, sDecoder* decoder) {

  int payload_cnt = 0;
  while (payload_cnt<size)
    if (payload[payload_cnt] == 0xFF)
       payload_cnt++;

  if (decoder->param.seiDebug) {
    printf ("FillerPayload\n");
    if (payload_cnt == size)
      printf ("read %d bytes of filler payload\n", payload_cnt);
    else
      printf ("error reading filler payload: not all bytes are 0xFF (%d of %d)\n", payload_cnt, size);
    }
  }
//}}}
//{{{
static void process_full_frame_freeze_info (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int full_frame_freeze_repetition_period = readUeV ("SEI full_frame_freeze_repetition_period", buf);
  printf ("full_frame_freeze_repetition_period = %d\n", full_frame_freeze_repetition_period);

  free (buf);
  }
//}}}
//{{{
static void process_full_frame_freeze_release_info (byte* payload, int size, sDecoder* decoder) {

  printf ("full-frame freeze release SEI\n");
  if (size)
    printf ("payload size should be zero, but is %d bytes.\n", size);
  }
//}}}
//{{{
static void process_full_frame_snapshot_info (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int snapshot_id = readUeV ("SEI snapshot_id", buf);

  printf ("Full-frame snapshot\n");
  printf ("snapshot_id = %d\n", snapshot_id);
  free (buf);
  }
//}}}

//{{{
static void process_progressive_refinement_start_info (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int progressive_refinement_id   = readUeV("SEI progressive_refinement_id"  , buf);
  int num_refinement_steps_minus1 = readUeV("SEI num_refinement_steps_minus1", buf);
  printf ("Progressive refinement segment start\n");
  printf ("progressive_refinement_id   = %d\n", progressive_refinement_id);
  printf ("num_refinement_steps_minus1 = %d\n", num_refinement_steps_minus1);

  free (buf);
  }
//}}}
//{{{
static void process_progressive_refinement_end_info (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int progressive_refinement_id   = readUeV ("SEI progressive_refinement_id"  , buf);
  printf ("progressive refinement segment end\n");
  printf ("progressive_refinement_id:%d\n", progressive_refinement_id);
  free (buf);
}
//}}}

//{{{
static void process_motion_constrained_slice_group_set_info (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int numSliceGroupsMinus1 = readUeV ("SEI numSliceGroupsMinus1" , buf);
  int sliceGroupSize = ceilLog2 (numSliceGroupsMinus1 + 1);
  printf ("Motion-constrained slice group set\n");
  printf ("numSliceGroupsMinus1 = %d\n", numSliceGroupsMinus1);

  for (int i = 0; i <= numSliceGroupsMinus1; i++) {
    int sliceGroupId = readUv (sliceGroupSize, "SEI sliceGroupId" , buf);
    printf ("sliceGroupId = %d\n", sliceGroupId);
    }

  int exact_match_flag = readU1 ("SEI exact_match_flag"  , buf);
  int pan_scan_rect_flag = readU1 ("SEI pan_scan_rect_flag"  , buf);

  printf ("exact_match_flag = %d\n", exact_match_flag);
  printf ("pan_scan_rect_flag = %d\n", pan_scan_rect_flag);

  if (pan_scan_rect_flag) {
    int pan_scan_rect_id = readUeV ("SEI pan_scan_rect_id"  , buf);
    printf ("pan_scan_rect_id = %d\n", pan_scan_rect_id);
    }

  free (buf);
  }
//}}}
//{{{
static void processFilmGrain (byte* payload, int size, sDecoder* decoder) {

  int film_grain_characteristics_cancel_flag;
  int model_id, separate_colour_description_present_flag;
  int film_grain_bit_depth_luma_minus8, film_grain_bit_depth_chroma_minus8, film_grain_full_range_flag, film_grain_colour_primaries, film_grain_transfer_characteristics, film_grain_matrix_coefficients;
  int blending_mode_id, log2_scale_factor, comp_model_present_flag[3];
  int num_intensity_intervals_minus1, num_model_values_minus1;
  int intensity_interval_lower_bound, intensity_interval_upper_bound;
  int comp_model_value;
  int film_grain_characteristics_repetition_period;

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  film_grain_characteristics_cancel_flag = readU1 ("SEI film_grain_characteristics_cancel_flag", buf);
  printf ("film_grain_characteristics_cancel_flag = %d\n", film_grain_characteristics_cancel_flag);
  if (!film_grain_characteristics_cancel_flag) {
    model_id = readUv(2, "SEI model_id", buf);
    separate_colour_description_present_flag = readU1("SEI separate_colour_description_present_flag", buf);
    printf ("model_id = %d\n", model_id);
    printf ("separate_colour_description_present_flag = %d\n", separate_colour_description_present_flag);

    if (separate_colour_description_present_flag) {
      film_grain_bit_depth_luma_minus8 = readUv(3, "SEI film_grain_bit_depth_luma_minus8", buf);
      film_grain_bit_depth_chroma_minus8 = readUv(3, "SEI film_grain_bit_depth_chroma_minus8", buf);
      film_grain_full_range_flag = readUv(1, "SEI film_grain_full_range_flag", buf);
      film_grain_colour_primaries = readUv(8, "SEI film_grain_colour_primaries", buf);
      film_grain_transfer_characteristics = readUv(8, "SEI film_grain_transfer_characteristics", buf);
      film_grain_matrix_coefficients = readUv(8, "SEI film_grain_matrix_coefficients", buf);
      printf ("film_grain_bit_depth_luma_minus8 = %d\n", film_grain_bit_depth_luma_minus8);
      printf ("film_grain_bit_depth_chroma_minus8 = %d\n", film_grain_bit_depth_chroma_minus8);
      printf ("film_grain_full_range_flag = %d\n", film_grain_full_range_flag);
      printf ("film_grain_colour_primaries = %d\n", film_grain_colour_primaries);
      printf ("film_grain_transfer_characteristics = %d\n", film_grain_transfer_characteristics);
      printf ("film_grain_matrix_coefficients = %d\n", film_grain_matrix_coefficients);
      }

    blending_mode_id = readUv (2, "SEI blending_mode_id", buf);
    log2_scale_factor = readUv (4, "SEI log2_scale_factor", buf);
    printf ("blending_mode_id = %d\n", blending_mode_id);
    printf ("log2_scale_factor = %d\n", log2_scale_factor);
    for (int c = 0; c < 3; c ++) {
      comp_model_present_flag[c] = readU1("SEI comp_model_present_flag", buf);
      printf ("comp_model_present_flag = %d\n", comp_model_present_flag[c]);
      }

    for (int c = 0; c < 3; c ++)
      if (comp_model_present_flag[c]) {
        num_intensity_intervals_minus1 = readUv(8, "SEI num_intensity_intervals_minus1", buf);
        num_model_values_minus1 = readUv(3, "SEI num_model_values_minus1", buf);
        printf("num_intensity_intervals_minus1 = %d\n", num_intensity_intervals_minus1);
        printf("num_model_values_minus1 = %d\n", num_model_values_minus1);
        for (int i = 0; i <= num_intensity_intervals_minus1; i ++) {
          intensity_interval_lower_bound = readUv(8, "SEI intensity_interval_lower_bound", buf);
          intensity_interval_upper_bound = readUv(8, "SEI intensity_interval_upper_bound", buf);
          printf ("intensity_interval_lower_bound = %d\n", intensity_interval_lower_bound);
          printf ("intensity_interval_upper_bound = %d\n", intensity_interval_upper_bound);
          for (int j = 0; j <= num_model_values_minus1; j++) {
            comp_model_value = readSeV("SEI comp_model_value", buf);
            printf ("comp_model_value = %d\n", comp_model_value);
            }
          }
        }
    film_grain_characteristics_repetition_period =
      readUeV("SEI film_grain_characteristics_repetition_period", buf);
    printf ("film_grain_characteristics_repetition_period = %d\n",
            film_grain_characteristics_repetition_period);
    }

  free (buf);
  }
//}}}
//{{{
static void processDeblockFilterDisplayPref (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int deblocking_display_preference_cancel_flag = readU1("SEI deblocking_display_preference_cancel_flag", buf);
  printf ("deblocking_display_preference_cancel_flag = %d\n", deblocking_display_preference_cancel_flag);
  if (!deblocking_display_preference_cancel_flag) {
    int display_prior_to_deblocking_preferred_flag = readU1("SEI display_prior_to_deblocking_preferred_flag", buf);
    int dec_frame_buffering_constraint_flag = readU1("SEI dec_frame_buffering_constraint_flag", buf);
    int deblocking_display_preference_repetition_period = readUeV("SEI deblocking_display_preference_repetition_period", buf);

    printf ("display_prior_to_deblocking_preferred_flag = %d\n", display_prior_to_deblocking_preferred_flag);
    printf ("dec_frame_buffering_constraint_flag = %d\n", dec_frame_buffering_constraint_flag);
    printf ("deblocking_display_preference_repetition_period = %d\n", deblocking_display_preference_repetition_period);
    }

  free (buf);
  }
//}}}
//{{{
static void processStereoVideo (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int field_views_flags = readU1 ("SEI field_views_flags", buf);
  printf ("field_views_flags = %d\n", field_views_flags);
  if (field_views_flags) {
    int top_field_is_left_view_flag = readU1 ("SEI top_field_is_left_view_flag", buf);
    printf ("top_field_is_left_view_flag = %d\n", top_field_is_left_view_flag);
    }
  else {
    int current_frame_is_left_view_flag = readU1 ("SEI current_frame_is_left_view_flag", buf);
    int next_frame_is_second_view_flag = readU1 ("SEI next_frame_is_second_view_flag", buf);
    printf ("current_frame_is_left_view_flag = %d\n", current_frame_is_left_view_flag);
    printf ("next_frame_is_second_view_flag = %d\n", next_frame_is_second_view_flag);
    }

  int left_view_self_contained_flag = readU1 ("SEI left_view_self_contained_flag", buf);
  int right_view_self_contained_flag = readU1 ("SEI right_view_self_contained_flag", buf);
  printf ("left_view_self_contained_flag = %d\n", left_view_self_contained_flag);
  printf ("right_view_self_contained_flag = %d\n", right_view_self_contained_flag);

  free (buf);
  }
//}}}
//{{{
static void processBufferingPeriod (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  int spsId = readUeV ("SEI spsId", buf);
  sSps* sps = &decoder->sps[spsId];
  //useSps (decoder, sps);

  if (decoder->param.seiDebug)
    printf ("buffering\n");

  // Note: NalHrdBpPresentFlag and CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
  if (sps->hasVui) {
    int initial_cpb_removal_delay;
    int initial_cpb_removal_delay_offset;
    if (sps->vuiSeqParams.nal_hrd_parameters_present_flag) {
      for (uint32_t k = 0; k < sps->vuiSeqParams.nal_hrd_parameters.cpb_cnt_minus1+1; k++) {
        initial_cpb_removal_delay = readUv(sps->vuiSeqParams.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay"        , buf);
        initial_cpb_removal_delay_offset = readUv(sps->vuiSeqParams.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay_offset" , buf);
        if (decoder->param.seiDebug) {
          printf ("nal initial_cpb_removal_delay[%d] = %d\n", k, initial_cpb_removal_delay);
          printf ("nal initial_cpb_removal_delay_offset[%d] = %d\n", k, initial_cpb_removal_delay_offset);
          }
        }
      }

    if (sps->vuiSeqParams.vcl_hrd_parameters_present_flag) {
      for (uint32_t k = 0; k < sps->vuiSeqParams.vcl_hrd_parameters.cpb_cnt_minus1+1; k++) {
        initial_cpb_removal_delay = readUv(sps->vuiSeqParams.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay"        , buf);
        initial_cpb_removal_delay_offset = readUv(sps->vuiSeqParams.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay_offset" , buf);
        if (decoder->param.seiDebug) {
          printf ("vcl initial_cpb_removal_delay[%d] = %d\n", k, initial_cpb_removal_delay);
          printf ("vcl initial_cpb_removal_delay_offset[%d] = %d\n", k, initial_cpb_removal_delay_offset);
          }
        }
      }
    }

  free (buf);
  }
//}}}
//{{{
static void process_frame_packing_arrangement_info (byte* payload, int size, sDecoder* decoder) {

  frame_packing_arrangement_information_struct seiFramePackingArrangement;

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  printf ("Frame packing arrangement\n");

  seiFramePackingArrangement.frame_packing_arrangement_id =
    (uint32_t)readUeV( "SEI frame_packing_arrangement_id", buf);
  seiFramePackingArrangement.frame_packing_arrangement_cancel_flag =
    readU1( "SEI frame_packing_arrangement_cancel_flag", buf);
  printf("frame_packing_arrangement_id = %d\n", seiFramePackingArrangement.frame_packing_arrangement_id);
  printf("frame_packing_arrangement_cancel_flag = %d\n", seiFramePackingArrangement.frame_packing_arrangement_cancel_flag);
  if ( seiFramePackingArrangement.frame_packing_arrangement_cancel_flag == false ) {
    seiFramePackingArrangement.frame_packing_arrangement_type =
      (uint8_t)readUv( 7, "SEI frame_packing_arrangement_type", buf);
    seiFramePackingArrangement.quincunx_sampling_flag =
      readU1 ("SEI quincunx_sampling_flag", buf );
    seiFramePackingArrangement.content_interpretation_type =
      (uint8_t)readUv( 6, "SEI content_interpretation_type", buf);
    seiFramePackingArrangement.spatial_flipping_flag = readU1 ("SEI spatial_flipping_flag", buf);
    seiFramePackingArrangement.frame0_flipped_flag = readU1 ("SEI frame0_flipped_flag", buf);
    seiFramePackingArrangement.field_views_flag = readU1 ("SEI field_views_flag", buf);
    seiFramePackingArrangement.current_frame_is_frame0_flag =
      readU1 ("SEI current_frame_is_frame0_flag", buf);
    seiFramePackingArrangement.frame0_self_contained_flag = readU1 ("SEI frame0_self_contained_flag", buf);
    seiFramePackingArrangement.frame1_self_contained_flag = readU1 ("SEI frame1_self_contained_flag", buf);

    printf ("frame_packing_arrangement_type    = %d\n", seiFramePackingArrangement.frame_packing_arrangement_type);
    printf ("quincunx_sampling_flag            = %d\n", seiFramePackingArrangement.quincunx_sampling_flag);
    printf ("content_interpretation_type       = %d\n", seiFramePackingArrangement.content_interpretation_type);
    printf ("spatial_flipping_flag             = %d\n", seiFramePackingArrangement.spatial_flipping_flag);
    printf ("frame0_flipped_flag               = %d\n", seiFramePackingArrangement.frame0_flipped_flag);
    printf ("field_views_flag                  = %d\n", seiFramePackingArrangement.field_views_flag);
    printf ("current_frame_is_frame0_flag      = %d\n", seiFramePackingArrangement.current_frame_is_frame0_flag);
    printf ("frame0_self_contained_flag        = %d\n", seiFramePackingArrangement.frame0_self_contained_flag);
    printf ("frame1_self_contained_flag        = %d\n", seiFramePackingArrangement.frame1_self_contained_flag);
    if (seiFramePackingArrangement.quincunx_sampling_flag == false &&
        seiFramePackingArrangement.frame_packing_arrangement_type != 5 )  {
      seiFramePackingArrangement.frame0_grid_position_x =
        (uint8_t)readUv( 4, "SEI frame0_grid_position_x", buf);
      seiFramePackingArrangement.frame0_grid_position_y =
        (uint8_t)readUv( 4, "SEI frame0_grid_position_y", buf);
      seiFramePackingArrangement.frame1_grid_position_x =
        (uint8_t)readUv( 4, "SEI frame1_grid_position_x", buf);
      seiFramePackingArrangement.frame1_grid_position_y =
        (uint8_t)readUv( 4, "SEI frame1_grid_position_y", buf);

      printf ("frame0_grid_position_x = %d\n", seiFramePackingArrangement.frame0_grid_position_x);
      printf ("frame0_grid_position_y = %d\n", seiFramePackingArrangement.frame0_grid_position_y);
      printf ("frame1_grid_position_x = %d\n", seiFramePackingArrangement.frame1_grid_position_x);
      printf ("frame1_grid_position_y = %d\n", seiFramePackingArrangement.frame1_grid_position_y);
    }
    seiFramePackingArrangement.frame_packing_arrangement_reserved_byte = (uint8_t)readUv( 8, "SEI frame_packing_arrangement_reserved_byte", buf);
    seiFramePackingArrangement.frame_packing_arrangement_repetition_period = (uint32_t)readUeV( "SEI frame_packing_arrangement_repetition_period", buf);
    printf("frame_packing_arrangement_reserved_byte = %d\n", seiFramePackingArrangement.frame_packing_arrangement_reserved_byte);
    printf("frame_packing_arrangement_repetition_period  = %d\n", seiFramePackingArrangement.frame_packing_arrangement_repetition_period);
  }
  seiFramePackingArrangement.frame_packing_arrangement_extension_flag = readU1( "SEI frame_packing_arrangement_extension_flag", buf);
  printf("frame_packing_arrangement_extension_flag = %d\n", seiFramePackingArrangement.frame_packing_arrangement_extension_flag);

  free (buf);
}
//}}}

//{{{
static void process_post_filter_hints_info (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  uint32_t filter_hint_size_y = readUeV("SEI filter_hint_size_y", buf); // interpret post-filter hint SEI here
  uint32_t filter_hint_size_x = readUeV("SEI filter_hint_size_x", buf); // interpret post-filter hint SEI here
  uint32_t filter_hint_type = readUv(2, "SEI filter_hint_type", buf); // interpret post-filter hint SEI here

  int** *filter_hint;
  getMem3Dint (&filter_hint, 3, filter_hint_size_y, filter_hint_size_x);

  for (uint32_t color_component = 0; color_component < 3; color_component ++)
    for (uint32_t cy = 0; cy < filter_hint_size_y; cy ++)
      for (uint32_t cx = 0; cx < filter_hint_size_x; cx ++)
        filter_hint[color_component][cy][cx] = readSeV("SEI filter_hint", buf); // interpret post-filter hint SEI here

  uint32_t additional_extension_flag = readU1("SEI additional_extension_flag", buf); // interpret post-filter hint SEI here

  printf ("Post-filter hint\n");
  printf ("post_filter_hint_size_y %d \n", filter_hint_size_y);
  printf ("post_filter_hint_size_x %d \n", filter_hint_size_x);
  printf ("post_filter_hint_type %d \n",   filter_hint_type);
  for (uint32_t color_component = 0; color_component < 3; color_component ++)
    for (uint32_t cy = 0; cy < filter_hint_size_y; cy ++)
      for (uint32_t cx = 0; cx < filter_hint_size_x; cx ++)
        printf (" post_filter_hint[%d][%d][%d] %d \n", color_component, cy, cx, filter_hint[color_component][cy][cx]);

  printf ("additional_extension_flag %d \n", additional_extension_flag);

  freeMem3Dint (filter_hint);
  free( buf);
  }
//}}}
//{{{
static void process_green_metadata_info (byte* payload, int size, sDecoder* decoder) {

  sBitStream* buf = (sBitStream*)malloc (sizeof(sBitStream));
  buf->bitStreamBuffer = payload;
  buf->bitStreamOffset = 0;
  buf->bitStreamLen = size;

  printf ("GreenMetadataInfo\n");

  Green_metadata_information_struct seiGreenMetadataInfo;
  seiGreenMetadataInfo.green_metadata_type = (uint8_t)readUv(8, "SEI green_metadata_type", buf);
  printf ("green_metadata_type = %d\n", seiGreenMetadataInfo.green_metadata_type);

  if (seiGreenMetadataInfo.green_metadata_type == 0) {
      seiGreenMetadataInfo.period_type=(uint8_t)readUv(8, "SEI green_metadata_period_type", buf);
    printf ("green_metadata_period_type = %d\n", seiGreenMetadataInfo.period_type);

    if (seiGreenMetadataInfo.period_type == 2) {
      seiGreenMetadataInfo.num_seconds = (uint16_t)readUv(16, "SEI green_metadata_num_seconds", buf);
      printf ("green_metadata_num_seconds = %d\n", seiGreenMetadataInfo.num_seconds);
      }
    else if (seiGreenMetadataInfo.period_type == 3) {
      seiGreenMetadataInfo.num_pictures = (uint16_t)readUv(16, "SEI green_metadata_num_pictures", buf);
      printf ("green_metadata_num_pictures = %d\n", seiGreenMetadataInfo.num_pictures);
      }

    seiGreenMetadataInfo.percent_non_zero_macroblocks =
      (uint8_t)readUv(8, "SEI percent_non_zero_macroblocks", buf);
    seiGreenMetadataInfo.percent_intra_coded_macroblocks =
      (uint8_t)readUv(8, "SEI percent_intra_coded_macroblocks", buf);
    seiGreenMetadataInfo.percent_six_tap_filtering =
      (uint8_t)readUv(8, "SEI percent_six_tap_filtering", buf);
    seiGreenMetadataInfo.percent_alpha_point_deblocking_instance =
      (uint8_t)readUv(8, "SEI percent_alpha_point_deblocking_instance", buf);

    printf ("percent_non_zero_macroblocks = %f\n", (float)seiGreenMetadataInfo.percent_non_zero_macroblocks/255);
    printf ("percent_intra_coded_macroblocks = %f\n", (float)seiGreenMetadataInfo.percent_intra_coded_macroblocks/255);
    printf ("percent_six_tap_filtering = %f\n", (float)seiGreenMetadataInfo.percent_six_tap_filtering/255);
    printf ("percent_alpha_point_deblocking_instance      = %f\n", (float)seiGreenMetadataInfo.percent_alpha_point_deblocking_instance/255);
    }
  else if (seiGreenMetadataInfo.green_metadata_type == 1) {
    seiGreenMetadataInfo.xsd_metric_type = (uint8_t)readUv(8, "SEI: xsd_metric_type", buf);
    seiGreenMetadataInfo.xsd_metric_value = (uint16_t)readUv(16, "SEI xsd_metric_value", buf);
    printf ("xsd_metric_type      = %d\n", seiGreenMetadataInfo.xsd_metric_type);
    if (seiGreenMetadataInfo.xsd_metric_type == 0)
      printf("xsd_metric_value      = %f\n", (float)seiGreenMetadataInfo.xsd_metric_value/100);
    }

  free (buf);
  }
//}}}

//{{{
void processSei (byte* msg, int naluLen, sDecoder* decoder, sSlice* slice) {

  if (decoder->param.seiDebug)
    printf ("SEI:%d -> ", naluLen);

  int offset = 1;
  do {
    int payloadType = 0;
    byte tempByte = msg[offset++];
    while (tempByte == 0xFF) {
      payloadType += 255;
      tempByte = msg[offset++];
      }
    payloadType += tempByte;

    int payloadSize = 0;
    tempByte = msg[offset++];
    while (tempByte == 0xFF) {
      payloadSize += 255;
      tempByte = msg[offset++];
      }
    payloadSize += tempByte;

    switch (payloadType) {
      case  SEI_BUFFERING_PERIOD:
        processBufferingPeriod (msg+offset, payloadSize, decoder); break;
      case  SEI_PIC_TIMING:
        processPictureTiming (msg+offset, payloadSize, decoder); break;
      case  SEI_PAN_SCAN_RECT:
        processPanScan (msg+offset, payloadSize, decoder); break;
      case  SEI_FILLER_PAYLOAD:
        process_filler_payload_info (msg+offset, payloadSize, decoder); break;
      case  SEI_USER_DATA_REGISTERED_ITU_T_T35:
        processUserDataT35 (msg+offset, payloadSize, decoder); break;
      case  SEI_USER_DATA_UNREGISTERED:
        processUserDataUnregistered (msg+offset, payloadSize, decoder); break;
      case  SEI_RECOVERY_POINT:
        processRecoveryPoint (msg+offset, payloadSize, decoder); break;
      case  SEI_DEC_REF_PIC_MARKING_REPETITION:
        processDecRefPicMarkingRepetition (msg+offset, payloadSize, decoder, slice); break;
      case  SEI_SPARE_PIC:
        process_spare_pic (msg+offset, payloadSize, decoder); break;
      case  SEI_SCENE_INFO:
        process_scene_information (msg+offset, payloadSize, decoder); break;
      case  SEI_SUB_SEQ_INFO:
        process_subsequence_info (msg+offset, payloadSize, decoder); break;
      case  SEI_SUB_SEQ_LAYER_CHARACTERISTICS:
        process_subsequence_layer_characteristics_info (msg+offset, payloadSize, decoder); break;
      case  SEI_SUB_SEQ_CHARACTERISTICS:
        process_subsequence_characteristics_info (msg+offset, payloadSize, decoder); break;
      case  SEI_FULL_FRAME_FREEZE:
        process_full_frame_freeze_info (msg+offset, payloadSize, decoder); break;
      case  SEI_FULL_FRAME_FREEZE_RELEASE:
        process_full_frame_freeze_release_info (msg+offset, payloadSize, decoder); break;
      case  SEI_FULL_FRAME_SNAPSHOT:
        process_full_frame_snapshot_info (msg+offset, payloadSize, decoder); break;
      case  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START:
        process_progressive_refinement_start_info (msg+offset, payloadSize, decoder); break;
      case  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END:
        process_progressive_refinement_end_info (msg+offset, payloadSize, decoder); break;
      case  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET:
        process_motion_constrained_slice_group_set_info (msg+offset, payloadSize, decoder); break;
      case  SEI_FILM_GRAIN_CHARACTERISTICS:
        processFilmGrain (msg+offset, payloadSize, decoder); break;
      case  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE:
        processDeblockFilterDisplayPref (msg+offset, payloadSize, decoder); break;
      case  SEI_STEREO_VIDEO_INFO:
        processStereoVideo  (msg+offset, payloadSize, decoder); break;
      case  SEI_POST_FILTER_HINTS:
        process_post_filter_hints_info (msg+offset, payloadSize, decoder); break;
      case  SEI_FRAME_PACKING_ARRANGEMENT:
        process_frame_packing_arrangement_info (msg+offset, payloadSize, decoder); break;
      case  SEI_GREEN_METADATA:
        process_green_metadata_info (msg+offset, payloadSize, decoder); break;
      default:
        processReserved (msg+offset, payloadSize, decoder); break;
      }
    offset += payloadSize;
    } while (msg[offset] != 0x80);    // moreRbspData()  msg[offset] != 0x80

  // ignore the trailing bits rbsp_trailing_bits();
  }
//}}}
