//{{{  includes
#include <math.h>

#include "global.h"
#include "memory.h"

#include "sei.h"
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
  bool       frame_packing_arrangement_cancelFlag;
  uint8_t frame_packing_arrangement_type;
  bool       quincunx_samplingFlag;
  uint8_t content_interpretation_type;
  bool       spatial_flippingFlag;
  bool       frame0_flippedFlag;
  bool       field_viewsFlag;
  bool       current_frame_is_frame0Flag;
  bool       frame0_self_containedFlag;
  bool       frame1_self_containedFlag;
  uint8_t frame0_grid_position_x;
  uint8_t frame0_grid_position_y;
  uint8_t frame1_grid_position_x;
  uint8_t frame1_grid_position_y;
  uint8_t frame_packing_arrangement_reserved_byte;
  uint32_t  frame_packing_arrangement_repetition_period;
  bool       frame_packing_arrangement_extensionFlag;
  } frame_packing_arrangement_information_struct;
//}}}
//{{{  Green_metadata_information_struct
typedef struct {
  uint8_t  green_metadata_type;
  uint8_t  period_type;
  uint16_t num_seconds;
  uint16_t num_pictures;
  uint8_t percent_non_zero_macroBlocks;
  uint8_t percent_intra_coded_macroBlocks;
  uint8_t percent_six_tap_filtering;
  uint8_t percent_alpha_point_deblocking_instance;
  uint8_t xsd_metric_type;
  uint16_t xsd_metric_value;
  } Green_metadata_information_struct;
//}}}

//{{{
static void processUserDataUnregistered (uint8_t* payload, int size, cDecoder264* decoder) {

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
static void processUserDataT35 (uint8_t* payload, int size, cDecoder264* decoder) {

  if (decoder->param.seiDebug) {
    int offset = 0;
    int itu_t_t35_country_code = payload[offset++];
    printf ("ITU-T:%d", itu_t_t35_country_code);

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
static void processReserved (uint8_t* payload, int size, cDecoder264* decoder) {

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
static void processPictureTiming (uint8_t* payload, int size, cDecoder264* decoder) {

  cSps* activeSps = decoder->activeSps;
  if (!activeSps) {
    if (decoder->param.seiDebug)
      printf ("pictureTiming - no active SPS\n");
    return;
    }

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  if (decoder->param.seiDebug) {
    printf ("pictureTiming");

    int cpb_removal_len = 24;
    int dpb_output_len = 24;
    bool cpbDpb = (bool)(activeSps->hasVui &&
                               (activeSps->vuiSeqParams.nal_hrd_parameters_presentFlag ||
                                activeSps->vuiSeqParams.vcl_hrd_parameters_presentFlag));

    if (cpbDpb) {
      if (activeSps->hasVui) {
        if (activeSps->vuiSeqParams.nal_hrd_parameters_presentFlag) {
          cpb_removal_len = activeSps->vuiSeqParams.nal_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
          dpb_output_len  = activeSps->vuiSeqParams.nal_hrd_parameters.dpb_output_delay_length_minus1  + 1;
          }
        else if (activeSps->vuiSeqParams.vcl_hrd_parameters_presentFlag) {
          cpb_removal_len = activeSps->vuiSeqParams.vcl_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
          dpb_output_len = activeSps->vuiSeqParams.vcl_hrd_parameters.dpb_output_delay_length_minus1  + 1;
          }
        }

      if ((activeSps->vuiSeqParams.nal_hrd_parameters_presentFlag) ||
          (activeSps->vuiSeqParams.vcl_hrd_parameters_presentFlag)) {
        int cpb_removal_delay, dpb_output_delay;
        cpb_removal_delay = s->readUv (cpb_removal_len, "SEI cpb_removal_delay");
        dpb_output_delay = s->readUv (dpb_output_len,  "SEI dpb_output_delay");
        if (decoder->param.seiDebug) {
          printf (" cpb:%d", cpb_removal_delay);
          printf (" dpb:%d", dpb_output_delay);
          }
        }
      }

    int pic_struct_presentFlag, pic_struct;
    if (!activeSps->hasVui)
      pic_struct_presentFlag = 0;
    else
      pic_struct_presentFlag = activeSps->vuiSeqParams.pic_struct_presentFlag;

    int numClockTs = 0;
    if (pic_struct_presentFlag) {
      pic_struct = s->readUv (4, "SEI pic_struct");
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
          cDecoder264::error ("reserved pic_struct used, can't determine numClockTs");
        //}}}
        }

      if (decoder->param.seiDebug)
        for (int i = 0; i < numClockTs; i++) {
          //{{{  print
          int clock_timestampFlag = s->readU1 ("SEI clock_timestampFlag");
          if (clock_timestampFlag) {
            printf (" clockTs");
            int ct_type = s->readUv (2, "SEI ct_type");
            int nuit_field_basedFlag = s->readU1 ("SEI nuit_field_basedFlag");
            int counting_type = s->readUv (5, "SEI counting_type");
            int full_timestampFlag = s->readU1 ("SEI full_timestampFlag");
            int discontinuityFlag = s->readU1 ("SEI discontinuityFlag");
            int cnt_droppedFlag = s->readU1 ("SEI cnt_droppedFlag");
            int nframes = s->readUv (8, "SEI nframes");

            printf ("ct:%d",ct_type);
            printf ("nuit:%d",nuit_field_basedFlag);
            printf ("full:%d",full_timestampFlag);
            printf ("dis:%d",discontinuityFlag);
            printf ("drop:%d",cnt_droppedFlag);
            printf ("nframes:%d",nframes);

            if (full_timestampFlag) {
              int seconds_value = s->readUv (6, "SEI seconds_value");
              int minutes_value = s->readUv (6, "SEI minutes_value");
              int hours_value = s->readUv (5, "SEI hours_value");
              printf ("sec:%d", seconds_value);
              printf ("min:%d", minutes_value);
              printf ("hour:%d", hours_value);
              }
            else {
              int secondsFlag = s->readU1 ("SEI secondsFlag");
              printf ("sec:%d",secondsFlag);
              if (secondsFlag) {
                int seconds_value = s->readUv (6, "SEI seconds_value");
                int minutesFlag = s->readU1 ("SEI minutesFlag");
                printf ("sec:%d",seconds_value);
                printf ("min:%d",minutesFlag);
                if(minutesFlag) {
                  int minutes_value = s->readUv(6, "SEI minutes_value");
                  int hoursFlag = s->readU1 ("SEI hoursFlag");
                  printf ("min:%d",minutes_value);
                  printf ("hour%d",hoursFlag);
                  if (hoursFlag) {
                    int hours_value = s->readUv (5, "SEI hours_value");
                    printf ("hour:%d\n",hours_value);
                    }
                  }
                }
              }

            int time_offset_length;
            int time_offset;
            if (activeSps->vuiSeqParams.vcl_hrd_parameters_presentFlag)
              time_offset_length = activeSps->vuiSeqParams.vcl_hrd_parameters.time_offset_length;
            else if (activeSps->vuiSeqParams.nal_hrd_parameters_presentFlag)
              time_offset_length = activeSps->vuiSeqParams.nal_hrd_parameters.time_offset_length;
            else
              time_offset_length = 24;
            if (time_offset_length)
              time_offset = s->readIv (time_offset_length, "SEI time_offset");
            else
              time_offset = 0;
            printf ("time:%d", time_offset);
            }
          }
          //}}}
      }
    printf ("\n");
    }

  free (s);
  }
//}}}
//{{{
static void processPanScan (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int pan_scan_rect_id = s->readUeV ("SEI pan_scan_rect_id");
  int pan_scan_rect_cancelFlag = s->readU1 ("SEI pan_scan_rect_cancelFlag");
  if (!pan_scan_rect_cancelFlag) {
    int pan_scan_cnt_minus1 = s->readUeV ("SEI pan_scan_cnt_minus1");
    for (int i = 0; i <= pan_scan_cnt_minus1; i++) {
      int pan_scan_rect_left_offset = s->readSeV ("SEI pan_scan_rect_left_offset");
      int pan_scan_rect_right_offset = s->readSeV ("SEI pan_scan_rect_right_offset");
      int pan_scan_rect_top_offset = s->readSeV ("SEI pan_scan_rect_top_offset");
      int pan_scan_rect_bottom_offset = s->readSeV ("SEI pan_scan_rect_bottom_offset");
      if (decoder->param.seiDebug)
        printf ("panScan %d/%d id:%d left:%d right:%d top:%d bot:%d\n",
                i, pan_scan_cnt_minus1, pan_scan_rect_id,
                pan_scan_rect_left_offset, pan_scan_rect_right_offset,
                pan_scan_rect_top_offset, pan_scan_rect_bottom_offset);
      }
    int pan_scan_rect_repetition_period = s->readUeV ("SEI pan_scan_rect_repetition_period");
    }

  free (s);
  }
//}}}
//{{{
static void processRecoveryPoint (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int recoveryFrameCount = s->readUeV ("SEI recoveryFrameCount");
  int exact_matchFlag = s->readU1 ("SEI exact_matchFlag");
  int broken_linkFlag = s->readU1 ("SEI broken_linkFlag");
  int changing_slice_group_idc = s->readUv (2, "SEI changing_slice_group_idc");

  decoder->recoveryPoint = 1;
  decoder->recoveryFrameCount = recoveryFrameCount;

  if (decoder->param.seiDebug)
    printf ("recovery - frame:%d exact:%d broken:%d changing:%d\n",
            recoveryFrameCount, exact_matchFlag, broken_linkFlag, changing_slice_group_idc);
  free (s);
  }
//}}}
//{{{
static void processDecRefPicMarkingRepetition (uint8_t* payload, int size, cDecoder264* decoder, cSlice *slice) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int original_idrFlag = s->readU1 ("SEI original_idrFlag");
  int original_frame_num = s->readUeV ("SEI original_frame_num");

  int original_bottom_fieldFlag = 0;
  if (!decoder->activeSps->frameMbOnly) {
    int original_field_picFlag = s->readU1 ("SEI original_field_picFlag");
    if (original_field_picFlag)
      original_bottom_fieldFlag = s->readU1 ("SEI original_bottom_fieldFlag");
    }

  if (decoder->param.seiDebug)
    printf ("decPicRepetition orig:%d:%d", original_idrFlag, original_frame_num);

  free (s);
  }
//}}}

//{{{
static void process_spare_pic (uint8_t* payload, int size, cDecoder264* decoder) {

  int x,y;
  int bit0, bit1, no_bit0;
  int target_frame_num = 0;
  int num_spare_pics;
  int delta_spare_frame_num, CandidateSpareFrameNum, SpareFrameNum = 0;
  int ref_area_indicator;

  int m, n, left, right, top, bottom,directx, directy;
  uint8_t*** map;

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  target_frame_num = s->readUeV ("SEI target_frame_num");
  num_spare_pics = 1 + s->readUeV ("SEI num_spare_pics_minus1");
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

    delta_spare_frame_num = s->readUeV ("SEI delta_spare_frame_num");
    SpareFrameNum = CandidateSpareFrameNum - delta_spare_frame_num;
    if (SpareFrameNum < 0 )
      SpareFrameNum = MAX_FN + SpareFrameNum;

    ref_area_indicator = s->readUeV ("SEI ref_area_indicator");
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
            map[i][y][x] = (uint8_t)s->readU1("SEI ref_mb_indicator");
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
              no_bit0 = s->readUeV ("SEI zero_run_length");
            if (no_bit0>0)
              map[i][y][x] = (uint8_t) bit0;
            else
              map[i][y][x] = (uint8_t) bit1;
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
  free (s);
  }
//}}}

//{{{
static void process_subsequence_info (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int sub_seq_layer_num = s->readUeV ("SEI sub_seq_layer_num");
  int sub_seq_id = s->readUeV ("SEI sub_seq_id");
  int first_ref_picFlag = s->readU1 ("SEI first_ref_picFlag");
  int leading_non_ref_picFlag = s->readU1 ("SEI leading_non_ref_picFlag");
  int last_picFlag = s->readU1 ("SEI last_picFlag");

  int sub_seq_frame_num = 0;
  int sub_seq_frame_numFlag = s->readU1 ("SEI sub_seq_frame_numFlag");
  if (sub_seq_frame_numFlag)
    sub_seq_frame_num = s->readUeV("SEI sub_seq_frame_num");

  if (decoder->param.seiDebug) {
    printf ("Sub-sequence information\n");
    printf ("sub_seq_layer_num = %d\n", sub_seq_layer_num );
    printf ("sub_seq_id = %d\n", sub_seq_id);
    printf ("first_ref_picFlag = %d\n", first_ref_picFlag);
    printf ("leading_non_ref_picFlag = %d\n", leading_non_ref_picFlag);
    printf ("last_picFlag = %d\n", last_picFlag);
    printf ("sub_seq_frame_numFlag = %d\n", sub_seq_frame_numFlag);
    if (sub_seq_frame_numFlag)
      printf ("sub_seq_frame_num = %d\n", sub_seq_frame_num);
    }

  free  (s);
  }
//}}}
//{{{
static void process_subsequence_layer_characteristics_info (uint8_t* payload, int size, cDecoder264* decoder)
{
  long num_sub_layers, accurate_statisticsFlag, average_bit_rate, average_frame_rate;

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamLen = size;
  s->bitStreamOffset = 0;

  num_sub_layers = 1 + s->readUeV("SEI num_sub_layers_minus1");

  if (decoder->param.seiDebug)
    printf ("Sub-sequence layer characteristics num_sub_layers_minus1 %ld\n", num_sub_layers - 1);
  for (int i = 0; i < num_sub_layers; i++) {
    accurate_statisticsFlag = s->readU1 ("SEI accurate_statisticsFlag");
    average_bit_rate = s->readUv (16,"SEI average_bit_rate");
    average_frame_rate = s->readUv (16,"SEI average_frame_rate");

    if (decoder->param.seiDebug) {
      printf("layer %d: accurate_statisticsFlag = %ld\n", i, accurate_statisticsFlag);
      printf("layer %d: average_bit_rate = %ld\n", i, average_bit_rate);
      printf("layer %d: average_frame_rate= %ld\n", i, average_frame_rate);
      }
    }

  free (s);
  }
//}}}
//{{{
static void process_subsequence_characteristics_info (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamLen = size;
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;

  int sub_seq_layer_num = s->readUeV ("SEI sub_seq_layer_num");
  int sub_seq_id = s->readUeV ("SEI sub_seq_id");
  int durationFlag = s->readU1 ("SEI durationFlag");

  if (decoder->param.seiDebug) {
    printf ("Sub-sequence characteristics\n");
    printf ("sub_seq_layer_num %d\n", sub_seq_layer_num );
    printf ("sub_seq_id %d\n", sub_seq_id);
    printf ("durationFlag %d\n", durationFlag);
    }

  if (durationFlag) {
    int sub_seq_duration = s->readUv (32, "SEI durationFlag");
    if (decoder->param.seiDebug)
      printf ("sub_seq_duration = %d\n", sub_seq_duration);
    }

  int average_rateFlag = s->readU1 ("SEI average_rateFlag");
  if (decoder->param.seiDebug)
    printf ("average_rateFlag %d\n", average_rateFlag);
  if (average_rateFlag) {
    int accurate_statisticsFlag = s->readU1 ("SEI accurate_statisticsFlag");
    int average_bit_rate = s->readUv (16, "SEI average_bit_rate");
    int average_frame_rate = s->readUv (16, "SEI average_frame_rate");
    if (decoder->param.seiDebug) {
      printf ("accurate_statisticsFlag %d\n", accurate_statisticsFlag);
      printf ("average_bit_rate %d\n", average_bit_rate);
      printf ("average_frame_rate %d\n", average_frame_rate);
      }
    }

  int num_referenced_subseqs  = s->readUeV("SEI num_referenced_subseqs");
  if (decoder->param.seiDebug)
    printf ("num_referenced_subseqs %d\n", num_referenced_subseqs);
  for (int i = 0; i < num_referenced_subseqs; i++) {
    int ref_sub_seq_layer_num  = s->readUeV ("SEI ref_sub_seq_layer_num");
    int ref_sub_seq_id = s->readUeV ("SEI ref_sub_seq_id");
    int ref_sub_seq_direction = s->readU1  ("SEI ref_sub_seq_direction");
    if (decoder->param.seiDebug) {
      printf ("ref_sub_seq_layer_num %d\n", ref_sub_seq_layer_num);
      printf ("ref_sub_seq_id %d\n", ref_sub_seq_id);
      printf ("ref_sub_seq_direction %d\n", ref_sub_seq_direction);
      }
    }
  free (s);
  }
//}}}
//{{{
static void process_scene_information (uint8_t* payload, int size, cDecoder264* decoder) {


  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamLen = size;
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;

  int second_scene_id;
  int scene_id = s->readUeV ("SEI scene_id");
  int scene_transition_type = s->readUeV ("SEI scene_transition_type");
  if (scene_transition_type > 3)
    second_scene_id = s->readUeV ("SEI scene_transition_type");

  if (decoder->param.seiDebug) {
    printf ("SceneInformation\n");
    printf ("scene_transition_type %d\n", scene_transition_type);
    printf ("scene_id %d\n", scene_id);
    if (scene_transition_type > 3 )
      printf ("second_scene_id %d\n", second_scene_id);
    }
  free (s);
  }
//}}}

//{{{
static void process_filler_payload_info (uint8_t* payload, int size, cDecoder264* decoder) {

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
static void process_full_frame_freeze_info (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int full_frame_freeze_repetition_period = s->readUeV ("SEI full_frame_freeze_repetition_period");
  printf ("full_frame_freeze_repetition_period = %d\n", full_frame_freeze_repetition_period);

  free (s);
  }
//}}}
//{{{
static void process_full_frame_freeze_release_info (uint8_t* payload, int size, cDecoder264* decoder) {

  printf ("full-frame freeze release SEI\n");
  if (size)
    printf ("payload size should be zero, but is %d bytes.\n", size);
  }
//}}}
//{{{
static void process_full_frame_snapshot_info (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int snapshot_id = s->readUeV ("SEI snapshot_id");

  printf ("Full-frame snapshot\n");
  printf ("snapshot_id = %d\n", snapshot_id);
  free (s);
  }
//}}}

//{{{
static void process_progressive_refinement_start_info (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int progressive_refinement_id   = s->readUeV("SEI progressive_refinement_id"  );
  int num_refinement_steps_minus1 = s->readUeV("SEI num_refinement_steps_minus1");
  printf ("Progressive refinement segment start\n");
  printf ("progressive_refinement_id   = %d\n", progressive_refinement_id);
  printf ("num_refinement_steps_minus1 = %d\n", num_refinement_steps_minus1);

  free (s);
  }
//}}}
//{{{
static void process_progressive_refinement_end_info (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int progressive_refinement_id   = s->readUeV ("SEI progressive_refinement_id"  );
  printf ("progressive refinement segment end\n");
  printf ("progressive_refinement_id:%d\n", progressive_refinement_id);
  free (s);
}
//}}}

//{{{
static void process_motion_constrained_slice_group_set_info (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int numSliceGroupsMinus1 = s->readUeV ("SEI numSliceGroupsMinus1" );
  int sliceGroupSize = ceilLog2 (numSliceGroupsMinus1 + 1);
  printf ("Motion-constrained slice group set\n");
  printf ("numSliceGroupsMinus1 = %d\n", numSliceGroupsMinus1);

  for (int i = 0; i <= numSliceGroupsMinus1; i++) {
    int sliceGroupId = s->readUv (sliceGroupSize, "SEI sliceGroupId" );
    printf ("sliceGroupId = %d\n", sliceGroupId);
    }

  int exact_matchFlag = s->readU1 ("SEI exact_matchFlag"  );
  int pan_scan_rectFlag = s->readU1 ("SEI pan_scan_rectFlag"  );

  printf ("exact_matchFlag = %d\n", exact_matchFlag);
  printf ("pan_scan_rectFlag = %d\n", pan_scan_rectFlag);

  if (pan_scan_rectFlag) {
    int pan_scan_rect_id = s->readUeV ("SEI pan_scan_rect_id"  );
    printf ("pan_scan_rect_id = %d\n", pan_scan_rect_id);
    }

  free (s);
  }
//}}}
//{{{
static void processFilmGrain (uint8_t* payload, int size, cDecoder264* decoder) {

  int film_grain_characteristics_cancelFlag;
  int model_id, separate_colour_description_presentFlag;
  int film_grain_bit_depth_luma_minus8, film_grain_bit_depth_chroma_minus8, film_grain_full_rangeFlag, film_grain_colour_primaries, film_grain_transfer_characteristics, film_grain_matrix_coefficients;
  int blending_mode_id, log2_scale_factor, comp_model_presentFlag[3];
  int num_intensity_intervals_minus1, num_model_values_minus1;
  int intensity_interval_lower_bound, intensity_interval_upper_bound;
  int comp_model_value;
  int film_grain_characteristics_repetition_period;

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  film_grain_characteristics_cancelFlag = s->readU1 ("SEI film_grain_characteristics_cancelFlag");
  printf ("film_grain_characteristics_cancelFlag = %d\n", film_grain_characteristics_cancelFlag);
  if (!film_grain_characteristics_cancelFlag) {
    model_id = s->readUv(2, "SEI model_id");
    separate_colour_description_presentFlag = s->readU1("SEI separate_colour_description_presentFlag");
    printf ("model_id = %d\n", model_id);
    printf ("separate_colour_description_presentFlag = %d\n", separate_colour_description_presentFlag);

    if (separate_colour_description_presentFlag) {
      film_grain_bit_depth_luma_minus8 = s->readUv(3, "SEI film_grain_bit_depth_luma_minus8");
      film_grain_bit_depth_chroma_minus8 = s->readUv(3, "SEI film_grain_bit_depth_chroma_minus8");
      film_grain_full_rangeFlag = s->readUv(1, "SEI film_grain_full_rangeFlag");
      film_grain_colour_primaries = s->readUv(8, "SEI film_grain_colour_primaries");
      film_grain_transfer_characteristics = s->readUv(8, "SEI film_grain_transfer_characteristics");
      film_grain_matrix_coefficients = s->readUv(8, "SEI film_grain_matrix_coefficients");
      printf ("film_grain_bit_depth_luma_minus8 = %d\n", film_grain_bit_depth_luma_minus8);
      printf ("film_grain_bit_depth_chroma_minus8 = %d\n", film_grain_bit_depth_chroma_minus8);
      printf ("film_grain_full_rangeFlag = %d\n", film_grain_full_rangeFlag);
      printf ("film_grain_colour_primaries = %d\n", film_grain_colour_primaries);
      printf ("film_grain_transfer_characteristics = %d\n", film_grain_transfer_characteristics);
      printf ("film_grain_matrix_coefficients = %d\n", film_grain_matrix_coefficients);
      }

    blending_mode_id = s->readUv (2, "SEI blending_mode_id");
    log2_scale_factor = s->readUv (4, "SEI log2_scale_factor");
    printf ("blending_mode_id = %d\n", blending_mode_id);
    printf ("log2_scale_factor = %d\n", log2_scale_factor);
    for (int c = 0; c < 3; c ++) {
      comp_model_presentFlag[c] = s->readU1("SEI comp_model_presentFlag");
      printf ("comp_model_presentFlag = %d\n", comp_model_presentFlag[c]);
      }

    for (int c = 0; c < 3; c ++)
      if (comp_model_presentFlag[c]) {
        num_intensity_intervals_minus1 = s->readUv(8, "SEI num_intensity_intervals_minus1");
        num_model_values_minus1 = s->readUv(3, "SEI num_model_values_minus1");
        printf("num_intensity_intervals_minus1 = %d\n", num_intensity_intervals_minus1);
        printf("num_model_values_minus1 = %d\n", num_model_values_minus1);
        for (int i = 0; i <= num_intensity_intervals_minus1; i ++) {
          intensity_interval_lower_bound = s->readUv(8, "SEI intensity_interval_lower_bound");
          intensity_interval_upper_bound = s->readUv(8, "SEI intensity_interval_upper_bound");
          printf ("intensity_interval_lower_bound = %d\n", intensity_interval_lower_bound);
          printf ("intensity_interval_upper_bound = %d\n", intensity_interval_upper_bound);
          for (int j = 0; j <= num_model_values_minus1; j++) {
            comp_model_value = s->readSeV("SEI comp_model_value");
            printf ("comp_model_value = %d\n", comp_model_value);
            }
          }
        }
    film_grain_characteristics_repetition_period =
      s->readUeV ("SEI film_grain_characteristics_repetition_period");
    printf ("film_grain_characteristics_repetition_period = %d\n",
            film_grain_characteristics_repetition_period);
    }

  free (s);
  }
//}}}
//{{{
static void processDeblockFilterDisplayPref (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int deblocking_display_preference_cancelFlag = s->readU1("SEI deblocking_display_preference_cancelFlag");
  printf ("deblocking_display_preference_cancelFlag = %d\n", deblocking_display_preference_cancelFlag);
  if (!deblocking_display_preference_cancelFlag) {
    int display_prior_to_deblocking_preferredFlag = s->readU1("SEI display_prior_to_deblocking_preferredFlag");
    int dec_frame_buffering_constraintFlag = s->readU1("SEI dec_frame_buffering_constraintFlag");
    int deblocking_display_preference_repetition_period = s->readUeV("SEI deblocking_display_preference_repetition_period");

    printf ("display_prior_to_deblocking_preferredFlag = %d\n", display_prior_to_deblocking_preferredFlag);
    printf ("dec_frame_buffering_constraintFlag = %d\n", dec_frame_buffering_constraintFlag);
    printf ("deblocking_display_preference_repetition_period = %d\n", deblocking_display_preference_repetition_period);
    }

  free (s);
  }
//}}}
//{{{
static void processStereoVideo (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int field_viewsFlags = s->readU1 ("SEI field_viewsFlags");
  printf ("field_viewsFlags = %d\n", field_viewsFlags);
  if (field_viewsFlags) {
    int top_field_is_left_viewFlag = s->readU1 ("SEI top_field_is_left_viewFlag");
    printf ("top_field_is_left_viewFlag = %d\n", top_field_is_left_viewFlag);
    }
  else {
    int current_frame_is_left_viewFlag = s->readU1 ("SEI current_frame_is_left_viewFlag");
    int next_frame_is_second_viewFlag = s->readU1 ("SEI next_frame_is_second_viewFlag");
    printf ("current_frame_is_left_viewFlag = %d\n", current_frame_is_left_viewFlag);
    printf ("next_frame_is_second_viewFlag = %d\n", next_frame_is_second_viewFlag);
    }

  int left_view_self_containedFlag = s->readU1 ("SEI left_view_self_containedFlag");
  int right_view_self_containedFlag = s->readU1 ("SEI right_view_self_containedFlag");
  printf ("left_view_self_containedFlag = %d\n", left_view_self_containedFlag);
  printf ("right_view_self_containedFlag = %d\n", right_view_self_containedFlag);

  free (s);
  }
//}}}
//{{{
static void processBufferingPeriod (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  int spsId = s->readUeV ("SEI spsId");
  cSps* sps = &decoder->sps[spsId];
  //useSps (decoder, sps);

  if (decoder->param.seiDebug)
    printf ("buffering\n");

  // Note: NalHrdBpPresentFlag and CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
  if (sps->hasVui) {
    int initial_cpb_removal_delay;
    int initial_cpb_removal_delay_offset;
    if (sps->vuiSeqParams.nal_hrd_parameters_presentFlag) {
      for (uint32_t k = 0; k < sps->vuiSeqParams.nal_hrd_parameters.cpb_cnt_minus1+1; k++) {
        initial_cpb_removal_delay = s->readUv(sps->vuiSeqParams.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay"        );
        initial_cpb_removal_delay_offset = s->readUv(sps->vuiSeqParams.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay_offset" );
        if (decoder->param.seiDebug) {
          printf ("nal initial_cpb_removal_delay[%d] = %d\n", k, initial_cpb_removal_delay);
          printf ("nal initial_cpb_removal_delay_offset[%d] = %d\n", k, initial_cpb_removal_delay_offset);
          }
        }
      }

    if (sps->vuiSeqParams.vcl_hrd_parameters_presentFlag) {
      for (uint32_t k = 0; k < sps->vuiSeqParams.vcl_hrd_parameters.cpb_cnt_minus1+1; k++) {
        initial_cpb_removal_delay = s->readUv(sps->vuiSeqParams.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay"        );
        initial_cpb_removal_delay_offset = s->readUv(sps->vuiSeqParams.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay_offset" );
        if (decoder->param.seiDebug) {
          printf ("vcl initial_cpb_removal_delay[%d] = %d\n", k, initial_cpb_removal_delay);
          printf ("vcl initial_cpb_removal_delay_offset[%d] = %d\n", k, initial_cpb_removal_delay_offset);
          }
        }
      }
    }

  free (s);
  }
//}}}
//{{{
static void process_frame_packing_arrangement_info (uint8_t* payload, int size, cDecoder264* decoder) {

  frame_packing_arrangement_information_struct seiFramePackingArrangement;

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  printf ("Frame packing arrangement\n");

  seiFramePackingArrangement.frame_packing_arrangement_id =
    (uint32_t)s->readUeV( "SEI frame_packing_arrangement_id");
  seiFramePackingArrangement.frame_packing_arrangement_cancelFlag =
    s->readU1( "SEI frame_packing_arrangement_cancelFlag");
  printf("frame_packing_arrangement_id = %d\n", seiFramePackingArrangement.frame_packing_arrangement_id);
  printf("frame_packing_arrangement_cancelFlag = %d\n", seiFramePackingArrangement.frame_packing_arrangement_cancelFlag);
  if ( seiFramePackingArrangement.frame_packing_arrangement_cancelFlag == false ) {
    seiFramePackingArrangement.frame_packing_arrangement_type = (uint8_t)s->readUv( 7, "SEI frame_packing_arrangement_type");
    seiFramePackingArrangement.quincunx_samplingFlag = s->readU1 ("SEI quincunx_samplingFlag");
    seiFramePackingArrangement.content_interpretation_type = (uint8_t)s->readUv( 6, "SEI content_interpretation_type");
    seiFramePackingArrangement.spatial_flippingFlag = s->readU1 ("SEI spatial_flippingFlag");
    seiFramePackingArrangement.frame0_flippedFlag = s->readU1 ("SEI frame0_flippedFlag");
    seiFramePackingArrangement.field_viewsFlag = s->readU1 ("SEI field_viewsFlag");
    seiFramePackingArrangement.current_frame_is_frame0Flag = s->readU1 ("SEI current_frame_is_frame0Flag");
    seiFramePackingArrangement.frame0_self_containedFlag = s->readU1 ("SEI frame0_self_containedFlag");
    seiFramePackingArrangement.frame1_self_containedFlag = s->readU1 ("SEI frame1_self_containedFlag");

    printf ("frame_packing_arrangement_type = %d\n", seiFramePackingArrangement.frame_packing_arrangement_type);
    printf ("quincunx_samplingFlag          = %d\n", seiFramePackingArrangement.quincunx_samplingFlag);
    printf ("content_interpretation_type    = %d\n", seiFramePackingArrangement.content_interpretation_type);
    printf ("spatial_flippingFlag           = %d\n", seiFramePackingArrangement.spatial_flippingFlag);
    printf ("frame0_flippedFlag             = %d\n", seiFramePackingArrangement.frame0_flippedFlag);
    printf ("field_viewsFlag                = %d\n", seiFramePackingArrangement.field_viewsFlag);
    printf ("current_frame_is_frame0Flag    = %d\n", seiFramePackingArrangement.current_frame_is_frame0Flag);
    printf ("frame0_self_containedFlag      = %d\n", seiFramePackingArrangement.frame0_self_containedFlag);
    printf ("frame1_self_containedFlag      = %d\n", seiFramePackingArrangement.frame1_self_containedFlag);
    if (seiFramePackingArrangement.quincunx_samplingFlag == false &&
        seiFramePackingArrangement.frame_packing_arrangement_type != 5 )  {
      seiFramePackingArrangement.frame0_grid_position_x =
        (uint8_t)s->readUv( 4, "SEI frame0_grid_position_x");
      seiFramePackingArrangement.frame0_grid_position_y =
        (uint8_t)s->readUv( 4, "SEI frame0_grid_position_y");
      seiFramePackingArrangement.frame1_grid_position_x =
        (uint8_t)s->readUv( 4, "SEI frame1_grid_position_x");
      seiFramePackingArrangement.frame1_grid_position_y =
        (uint8_t)s->readUv( 4, "SEI frame1_grid_position_y");

      printf ("frame0_grid_position_x = %d\n", seiFramePackingArrangement.frame0_grid_position_x);
      printf ("frame0_grid_position_y = %d\n", seiFramePackingArrangement.frame0_grid_position_y);
      printf ("frame1_grid_position_x = %d\n", seiFramePackingArrangement.frame1_grid_position_x);
      printf ("frame1_grid_position_y = %d\n", seiFramePackingArrangement.frame1_grid_position_y);
    }
    seiFramePackingArrangement.frame_packing_arrangement_reserved_byte = (uint8_t)s->readUv( 8, "SEI frame_packing_arrangement_reserved_byte");
    seiFramePackingArrangement.frame_packing_arrangement_repetition_period = (uint32_t)s->readUeV( "SEI frame_packing_arrangement_repetition_period");
    printf("frame_packing_arrangement_reserved_byte = %d\n", seiFramePackingArrangement.frame_packing_arrangement_reserved_byte);
    printf("frame_packing_arrangement_repetition_period  = %d\n", seiFramePackingArrangement.frame_packing_arrangement_repetition_period);
  }
  seiFramePackingArrangement.frame_packing_arrangement_extensionFlag = s->readU1( "SEI frame_packing_arrangement_extensionFlag");
  printf("frame_packing_arrangement_extensionFlag = %d\n", seiFramePackingArrangement.frame_packing_arrangement_extensionFlag);

  free (s);
}
//}}}

//{{{
static void process_post_filter_hints_info (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  uint32_t filter_hint_size_y = s->readUeV("SEI filter_hint_size_y"); // interpret post-filter hint SEI here
  uint32_t filter_hint_size_x = s->readUeV("SEI filter_hint_size_x"); // interpret post-filter hint SEI here
  uint32_t filter_hint_type = s->readUv(2, "SEI filter_hint_type"); // interpret post-filter hint SEI here

  int** *filter_hint;
  getMem3Dint (&filter_hint, 3, filter_hint_size_y, filter_hint_size_x);

  for (uint32_t color_component = 0; color_component < 3; color_component ++)
    for (uint32_t cy = 0; cy < filter_hint_size_y; cy ++)
      for (uint32_t cx = 0; cx < filter_hint_size_x; cx ++)
        filter_hint[color_component][cy][cx] = s->readSeV("SEI filter_hint"); // interpret post-filter hint SEI here

  uint32_t additional_extensionFlag = s->readU1("SEI additional_extensionFlag"); // interpret post-filter hint SEI here

  printf ("Post-filter hint\n");
  printf ("post_filter_hint_size_y %d \n", filter_hint_size_y);
  printf ("post_filter_hint_size_x %d \n", filter_hint_size_x);
  printf ("post_filter_hint_type %d \n",   filter_hint_type);
  for (uint32_t color_component = 0; color_component < 3; color_component ++)
    for (uint32_t cy = 0; cy < filter_hint_size_y; cy ++)
      for (uint32_t cx = 0; cx < filter_hint_size_x; cx ++)
        printf (" post_filter_hint[%d][%d][%d] %d \n", color_component, cy, cx, filter_hint[color_component][cy][cx]);

  printf ("additional_extensionFlag %d \n", additional_extensionFlag);

  freeMem3Dint (filter_hint);
  free (s);
  }
//}}}
//{{{
static void process_green_metadata_info (uint8_t* payload, int size, cDecoder264* decoder) {

  cBitStream* s = (cBitStream*)malloc (sizeof(cBitStream));
  s->bitStreamBuffer = payload;
  s->bitStreamOffset = 0;
  s->bitStreamLen = size;

  printf ("GreenMetadataInfo\n");

  Green_metadata_information_struct seiGreenMetadataInfo;
  seiGreenMetadataInfo.green_metadata_type = (uint8_t)s->readUv(8, "SEI green_metadata_type");
  printf ("green_metadata_type = %d\n", seiGreenMetadataInfo.green_metadata_type);

  if (seiGreenMetadataInfo.green_metadata_type == 0) {
      seiGreenMetadataInfo.period_type=(uint8_t)s->readUv(8, "SEI green_metadata_period_type");
    printf ("green_metadata_period_type = %d\n", seiGreenMetadataInfo.period_type);

    if (seiGreenMetadataInfo.period_type == 2) {
      seiGreenMetadataInfo.num_seconds = (uint16_t)s->readUv(16, "SEI green_metadata_num_seconds");
      printf ("green_metadata_num_seconds = %d\n", seiGreenMetadataInfo.num_seconds);
      }
    else if (seiGreenMetadataInfo.period_type == 3) {
      seiGreenMetadataInfo.num_pictures = (uint16_t)s->readUv(16, "SEI green_metadata_num_pictures");
      printf ("green_metadata_num_pictures = %d\n", seiGreenMetadataInfo.num_pictures);
      }

    seiGreenMetadataInfo.percent_non_zero_macroBlocks = (uint8_t)s->readUv(8, "SEI percent_non_zero_macroBlocks");
    seiGreenMetadataInfo.percent_intra_coded_macroBlocks = (uint8_t)s->readUv(8, "SEI percent_intra_coded_macroBlocks");
    seiGreenMetadataInfo.percent_six_tap_filtering = (uint8_t)s->readUv(8, "SEI percent_six_tap_filtering");
    seiGreenMetadataInfo.percent_alpha_point_deblocking_instance =
      (uint8_t)s->readUv(8, "SEI percent_alpha_point_deblocking_instance");

    printf ("percent_non_zero_macroBlocks = %f\n", (float)seiGreenMetadataInfo.percent_non_zero_macroBlocks/255);
    printf ("percent_intra_coded_macroBlocks = %f\n", (float)seiGreenMetadataInfo.percent_intra_coded_macroBlocks/255);
    printf ("percent_six_tap_filtering = %f\n", (float)seiGreenMetadataInfo.percent_six_tap_filtering/255);
    printf ("percent_alpha_point_deblocking_instance      = %f\n", (float)seiGreenMetadataInfo.percent_alpha_point_deblocking_instance/255);
    }
  else if (seiGreenMetadataInfo.green_metadata_type == 1) {
    seiGreenMetadataInfo.xsd_metric_type = (uint8_t)s->readUv(8, "SEI: xsd_metric_type");
    seiGreenMetadataInfo.xsd_metric_value = (uint16_t)s->readUv(16, "SEI xsd_metric_value");
    printf ("xsd_metric_type      = %d\n", seiGreenMetadataInfo.xsd_metric_type);
    if (seiGreenMetadataInfo.xsd_metric_type == 0)
      printf("xsd_metric_value      = %f\n", (float)seiGreenMetadataInfo.xsd_metric_value/100);
    }

  free (s);
  }
//}}}

//{{{
void processSei (uint8_t* msg, int naluLen, cDecoder264* decoder, cSlice* slice) {

  if (decoder->param.seiDebug)
    printf ("SEI:%d -> ", naluLen);

  int offset = 1;
  do {
    int payloadType = 0;
    uint8_t tempByte = msg[offset++];
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
