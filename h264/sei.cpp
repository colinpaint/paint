//{{{  includes
#include <math.h>
#include "global.h"
#include "memory.h"

#include "sei.h"

#include "../common/cLog.h"

using namespace std;
//}}}

constexpr int kMaxFunction = 256;
//{{{
enum eSeqType {
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
  SEI_GREEN_METADATA = 56,
  SEI_MAX_ELEMENTS
  } SEI_type;
//}}}
//{{{  frame_packing_arrangement_information_struct
typedef struct {
  uint32_t frame_packing_arrangement_id;
  bool     frame_packing_arrangement_cancelFlag;
  uint8_t  frame_packing_arrangement_type;
  bool     quincunx_samplingFlag;
  uint8_t  content_interpretation_type;
  bool     spatial_flippingFlag;
  bool     frame0_flippedFlag;
  bool     field_viewsFlag;
  bool     current_frame_is_frame0Flag;
  bool     frame0_self_containedFlag;
  bool     frame1_self_containedFlag;
  uint8_t  frame0_grid_position_x;
  uint8_t  frame0_grid_position_y;
  uint8_t  frame1_grid_position_x;
  uint8_t  frame1_grid_position_y;
  uint8_t  frame_packing_arrangement_reserved_byte;
  uint32_t frame_packing_arrangement_repetition_period;
  bool     frame_packing_arrangement_extensionFlag;
  } frame_packing_arrangement_information_struct;
//}}}
//{{{  Green_metadata_information_struct
typedef struct {
  uint8_t  green_metadata_type;
  uint8_t  period_type;
  uint16_t num_seconds;
  uint16_t num_pictures;
  uint8_t  percent_non_zero_macroBlocks;
  uint8_t  percent_intra_coded_macroBlocks;
  uint8_t  percent_six_tap_filtering;
  uint8_t  percent_alpha_point_deblocking_instance;
  uint8_t  xsd_metric_type;
  uint16_t xsd_metric_value;
  } Green_metadata_information_struct;
//}}}

namespace {
  //{{{
  void processUserDataUnregistered (uint8_t* payload, int size, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      string payloadString;
      for (int i = 0; i < size; i++)
        if (payload[i] >= 0x20 && payload[i] <= 0x7f)
          payloadString += fmt::format ("{}", char(payload[i]));
        else
          payloadString += fmt::format ("<{:02x}>", payload[i]);

      cLog::log (LOGINFO, fmt::format ("- unregistered {}", payloadString));
      }
    }
  //}}}
  //{{{
  void processUserDataT35 (uint8_t* payload, int size, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      string payloadString;
      int offset = 0;
      int itu_t_t35_country_code = payload[offset++];

      if (itu_t_t35_country_code == 0xFF) {
        int itu_t_t35_country_code_extension_byte = payload[offset++];
        payloadString += fmt::format (" ext:{} - ", itu_t_t35_country_code_extension_byte);
        }

      for (int i = offset; i < size; i++)
        if (payload[i] >= 0x20 && payload[i] <= 0x7f)
          payloadString += fmt::format ("{}", char(payload[i]));
        else
          payloadString += fmt::format ("<{:02x}>", payload[i]);

      cLog::log (LOGINFO, fmt::format ("- ITU-T:{} {}", itu_t_t35_country_code, payloadString));
      }
    }
  //}}}
  //{{{
  void processReserved (uint8_t* payload, int size, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      string payloadString;
      for (int i = 0; i < size; i++)
        if (payload[i] >= 0x20 && payload[i] <= 0x7f)
          payloadString += fmt::format ("{}", char(payload[i]));
        else
          payloadString += fmt::format ("<{:02x}>", payload[i]);
      cLog::log (LOGINFO, fmt::format ("- reserved {}", payloadString));
      }
    }
  //}}}

  //{{{
  void processPictureTiming (sBitStream& bitStream, cDecoder264* decoder) {

    cSps* activeSps = decoder->activeSps;
    if (!activeSps) {
      if (decoder->param.seiDebug)
        cLog::log (LOGINFO, "- pictureTiming - no active SPS");
      return;
      }

    if (decoder->param.seiDebug) {
      cLog::log (LOGINFO, "- pictureTiming");

      int cpb_removal_len = 24;
      int dpb_output_len = 24;
      bool cpbDpb = activeSps->hasVui &&
                    (activeSps->vuiSeqParams.nal_hrd_parameters_presentFlag ||
                     activeSps->vuiSeqParams.vcl_hrd_parameters_presentFlag);

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
          cpb_removal_delay = bitStream.readUv (cpb_removal_len, "SEI cpb_removal_delay");
          cLog::log (LOGINFO, " cpb:%d", cpb_removal_delay);
          dpb_output_delay = bitStream.readUv (dpb_output_len,  "SEI dpb_output_delay");
          cLog::log (LOGINFO, " dpb:%d", dpb_output_delay);
          }
        }

      int pic_struct_presentFlag, pic_struct;
      if (!activeSps->hasVui)
        pic_struct_presentFlag = 0;
      else
        pic_struct_presentFlag = activeSps->vuiSeqParams.pic_struct_presentFlag;

      int numClockTs = 0;
      if (pic_struct_presentFlag) {
        pic_struct = bitStream.readUv (4, "SEI pic_struct");
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
            int clock_timestampFlag = bitStream.readU1 ("SEI clock_timestampFlag");
            if (clock_timestampFlag) {
              cLog::log (LOGINFO, " clockTs");
              int ct_type = bitStream.readUv (2, "SEI ct_type");
              int nuit_field_basedFlag = bitStream.readU1 ("SEI nuit_field_basedFlag");
              int counting_type = bitStream.readUv (5, "SEI counting_type");
              int full_timestampFlag = bitStream.readU1 ("SEI full_timestampFlag");
              int discontinuityFlag = bitStream.readU1 ("SEI discontinuityFlag");
              int cnt_droppedFlag = bitStream.readU1 ("SEI cnt_droppedFlag");
              int nframes = bitStream.readUv (8, "SEI nframes");

              cLog::log (LOGINFO, "ct:%d",ct_type);
              cLog::log (LOGINFO, "nuit:%d",nuit_field_basedFlag);
              cLog::log (LOGINFO, "full:%d",full_timestampFlag);
              cLog::log (LOGINFO, "dis:%d",discontinuityFlag);
              cLog::log (LOGINFO, "drop:%d",cnt_droppedFlag);
              cLog::log (LOGINFO, "nframes:%d",nframes);

              if (full_timestampFlag) {
                int seconds_value = bitStream.readUv (6, "SEI seconds_value");
                int minutes_value = bitStream.readUv (6, "SEI minutes_value");
                int hours_value = bitStream.readUv (5, "SEI hours_value");
                cLog::log (LOGINFO, "sec:%d", seconds_value);
                cLog::log (LOGINFO, "min:%d", minutes_value);
                cLog::log (LOGINFO, "hour:%d", hours_value);
                }
              else {
                int secondsFlag = bitStream.readU1 ("SEI secondsFlag");
                cLog::log (LOGINFO, "sec:%d",secondsFlag);
                if (secondsFlag) {
                  int seconds_value = bitStream.readUv (6, "SEI seconds_value");
                  int minutesFlag = bitStream.readU1 ("SEI minutesFlag");
                  cLog::log (LOGINFO, "sec:%d",seconds_value);
                  cLog::log (LOGINFO, "min:%d",minutesFlag);
                  if(minutesFlag) {
                    int minutes_value = bitStream.readUv(6, "SEI minutes_value");
                    int hoursFlag = bitStream.readU1 ("SEI hoursFlag");
                    cLog::log (LOGINFO, "min:%d",minutes_value);
                    cLog::log (LOGINFO, "hour%d",hoursFlag);
                    if (hoursFlag) {
                      int hours_value = bitStream.readUv (5, "SEI hours_value");
                      cLog::log (LOGINFO, "hour:%d",hours_value);
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
                time_offset = bitStream.readIv (time_offset_length, "SEI time_offset");
              else
                time_offset = 0;
              cLog::log (LOGINFO, "time:%d", time_offset);
              }
            }
            //}}}
        }
      }
    }
  //}}}
  //{{{
  void processPanScan (sBitStream& bitStream, cDecoder264* decoder) {

    int pan_scan_rect_id =  bitStream.readUeV ("SEI pan_scan_rect_id");

    int pan_scan_rect_cancelFlag =  bitStream.readU1 ("SEI pan_scan_rect_cancelFlag");
    if (!pan_scan_rect_cancelFlag) {
      int pan_scan_cnt_minus1 =  bitStream.readUeV ("SEI pan_scan_cnt_minus1");
      for (int i = 0; i <= pan_scan_cnt_minus1; i++) {
        int pan_scan_rect_left_offset =  bitStream.readSeV ("SEI pan_scan_rect_left_offset");
        int pan_scan_rect_right_offset =  bitStream.readSeV ("SEI pan_scan_rect_right_offset");
        int pan_scan_rect_top_offset =  bitStream.readSeV ("SEI pan_scan_rect_top_offset");
        int pan_scan_rect_bottom_offset =  bitStream.readSeV ("SEI pan_scan_rect_bottom_offset");
        if (decoder->param.seiDebug)
          cLog::log (LOGINFO, "- panScan %d/%d id:%d left:%d right:%d top:%d bot:%d",
                              i, pan_scan_cnt_minus1, pan_scan_rect_id,
                              pan_scan_rect_left_offset, pan_scan_rect_right_offset,
                              pan_scan_rect_top_offset, pan_scan_rect_bottom_offset);
        }

      int pan_scan_rect_repetition_period =  bitStream.readUeV ("SEI pan_scan_rect_repetition_period");
      }
    }
  //}}}
  //{{{
  void processRecoveryPoint (sBitStream& bitStream, cDecoder264* decoder) {

    int recoveryFrameCount = bitStream.readUeV ("SEI recoveryFrameCount");
    int exact_matchFlag = bitStream.readU1 ("SEI exact_matchFlag");
    int broken_linkFlag = bitStream.readU1 ("SEI broken_linkFlag");
    int changing_slice_group_idc = bitStream.readUv (2, "SEI changing_slice_group_idc");

    decoder->recoveryPoint = 1;
    decoder->recoveryFrameCount = recoveryFrameCount;

    if (decoder->param.seiDebug)
      cLog::log (LOGINFO, "- recovery - frame:%d exact:%d broken:%d changing:%d",
                          recoveryFrameCount, exact_matchFlag, broken_linkFlag, changing_slice_group_idc);
    }
  //}}}
  //{{{
  void processDecRefPicMarkingRepetition (sBitStream& bitStream, cDecoder264* decoder, cSlice *slice) {

    int original_idrFlag = bitStream.readU1 ("SEI original_idrFlag");
    int original_frame_num = bitStream.readUeV ("SEI original_frame_num");

    int original_bottom_fieldFlag = 0;
    if (!decoder->activeSps->frameMbOnly) {
      int original_field_picFlag =  bitStream.readU1 ("SEI original_field_picFlag");
      if (original_field_picFlag)
        original_bottom_fieldFlag =  bitStream.readU1 ("SEI original_bottom_fieldFlag");
      }

    if (decoder->param.seiDebug)
      cLog::log (LOGINFO, "- decPicRepetition orig:%d:%d", original_idrFlag, original_frame_num);
    }
  //}}}

  //{{{
  void process_spare_pic (sBitStream& bitStream, cDecoder264* decoder) {

    int target_frame_num =  bitStream.readUeV ("SEI target_frame_num");
    int num_spare_pics = bitStream.readUeV ("SEI num_spare_pics_minus1") + 1;
    if (decoder->param.seiDebug)
      cLog::log (LOGINFO, "sparePicture target_frame_num:%d num_spare_pics:%d",
                          target_frame_num, num_spare_pics);

    uint8_t*** map;
    getMem3D (&map, num_spare_pics, decoder->coding.height >> 4, decoder->coding.width >> 4);

    int delta_spare_frame_num, candidateSpareFrameNum, spareFrameNum = 0;
    for (int i = 0; i < num_spare_pics; i++) {
      if (i == 0) {
        candidateSpareFrameNum = target_frame_num - 1;
        if (candidateSpareFrameNum < 0)
          candidateSpareFrameNum = kMaxFunction - 1;
        }
      else
        candidateSpareFrameNum = spareFrameNum;

      delta_spare_frame_num =  bitStream.readUeV ("SEI delta_spare_frame_num");
      spareFrameNum = candidateSpareFrameNum - delta_spare_frame_num;
      if (spareFrameNum < 0)
        spareFrameNum = kMaxFunction + spareFrameNum;

      int ref_area_indicator =  bitStream.readUeV ("SEI ref_area_indicator");
      switch (ref_area_indicator) {
        //{{{
        case 0: // The whole frame can serve as spare picture
          for (int y = 0; y < decoder->coding.height >> 4; y++)
            for (int x = 0; x < decoder->coding.width >> 4; x++)
              map[i][y][x] = 0;
          break;
        //}}}
        //{{{
        case 1: // The map is not compressed
          for (int y = 0; y < decoder->coding.height >> 4; y++)
            for (int x = 0; x < decoder->coding.width >> 4; x++)
              map[i][y][x] = (uint8_t)bitStream.readU1 ("SEI ref_mb_indicator");
          break;
        //}}}
        //{{{
        case 2: { // The map is compressed
          int bit0 = 0;
          int bit1 = 1;
          int no_bit0 = -1;

          int x = ((decoder->coding.width >> 4) - 1 ) / 2;
          int y  = ((decoder->coding.height >> 4) - 1 ) / 2;
          int left = x;
          int right = x;
          int top = y; 
          int bottom = y;
          int directx = 0;
          int directy = 1;

          for (int m = 0; m < decoder->coding.height >> 4; m++)
            for (int n = 0; n < decoder->coding.width >> 4; n++) {
              if (no_bit0 < 0)
                no_bit0 =  bitStream.readUeV ("SEI zero_run_length");
              if (no_bit0>0)
                map[i][y][x] = (uint8_t) bit0;
              else
                map[i][y][x] = (uint8_t) bit1;
              no_bit0--;

              // go to the next mb:
              if (directx == -1 && directy == 0) {
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
              else if (directx == 1 && directy == 0) {
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
              else if (directx == 0 && directy == -1) {
                //{{{
                if (y > top)
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
              else if (directx == 0 && directy == 1) {
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
            }
          break;
        //}}}
        //{{{
        default:
          cLog::log (LOGINFO, "Wrong ref_area_indicator %d!", ref_area_indicator );
          exit(0);
          break;
        //}}}
        }
      }

    freeMem3D (map);
    }
  //}}}

  //{{{
  void process_subsequence_info (sBitStream& bitStream, cDecoder264* decoder) {

    int sub_seq_layer_num =  bitStream.readUeV ("SEI sub_seq_layer_num");
    int sub_seq_id = bitStream.readUeV ("SEI sub_seq_id");
    int first_ref_picFlag = bitStream.readU1 ("SEI first_ref_picFlag");
    int leading_non_ref_picFlag = bitStream.readU1 ("SEI leading_non_ref_picFlag");
    int last_picFlag = bitStream.readU1 ("SEI last_picFlag");

    int sub_seq_frame_num = 0;
    int sub_seq_frame_numFlag = bitStream.readU1 ("SEI sub_seq_frame_numFlag");
    if (sub_seq_frame_numFlag)
      sub_seq_frame_num = bitStream.readUeV("SEI sub_seq_frame_num");

    if (decoder->param.seiDebug) {
      cLog::log (LOGINFO, "Sub-sequence information");
      cLog::log (LOGINFO, "sub_seq_layer_num = %d", sub_seq_layer_num );
      cLog::log (LOGINFO, "sub_seq_id = %d", sub_seq_id);
      cLog::log (LOGINFO, "first_ref_picFlag = %d", first_ref_picFlag);
      cLog::log (LOGINFO, "leading_non_ref_picFlag = %d", leading_non_ref_picFlag);
      cLog::log (LOGINFO, "last_picFlag = %d", last_picFlag);
      cLog::log (LOGINFO, "sub_seq_frame_numFlag = %d", sub_seq_frame_numFlag);
      if (sub_seq_frame_numFlag)
        cLog::log (LOGINFO, "sub_seq_frame_num = %d", sub_seq_frame_num);
      }
    }
  //}}}
  //{{{
  void process_subsequence_layer_characteristics_info (sBitStream& bitStream, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      long num_sub_layers, accurate_statisticsFlag, average_bit_rate, average_frame_rate;
      num_sub_layers = 1 + bitStream.readUeV ("SEI num_sub_layers_minus1");
      cLog::log (LOGINFO, "Sub-sequence layer characteristics num_sub_layers_minus1 %ld", num_sub_layers - 1);

      for (int i = 0; i < num_sub_layers; i++) {
        accurate_statisticsFlag = bitStream.readU1 ("SEI accurate_statisticsFlag");
        cLog::log (LOGINFO, "layer %d: accurate_statisticsFlag = %ld", i, accurate_statisticsFlag);
        average_bit_rate = bitStream.readUv (16,"SEI average_bit_rate");
        cLog::log (LOGINFO, "layer %d: average_bit_rate = %ld", i, average_bit_rate);
        average_frame_rate = bitStream.readUv (16,"SEI average_frame_rate");
        cLog::log (LOGINFO, "layer %d: average_frame_rate= %ld", i, average_frame_rate);
        }
      }
    }
  //}}}
  //{{{
  void process_subsequence_characteristics_info (sBitStream& bitStream, cDecoder264* decoder) {

    int sub_seq_layer_num = bitStream.readUeV ("SEI sub_seq_layer_num");
    int sub_seq_id = bitStream.readUeV ("SEI sub_seq_id");
    int durationFlag = bitStream.readU1 ("SEI durationFlag");

    if (decoder->param.seiDebug) {
      cLog::log (LOGINFO, "Sub-sequence characteristics");
      cLog::log (LOGINFO, "sub_seq_layer_num %d", sub_seq_layer_num );
      cLog::log (LOGINFO, "sub_seq_id %d", sub_seq_id);
      cLog::log (LOGINFO, "durationFlag %d", durationFlag);
      }

    if (durationFlag) {
      int sub_seq_duration = bitStream.readUv (32, "SEI durationFlag");
      if (decoder->param.seiDebug)
        cLog::log (LOGINFO, "sub_seq_duration = %d", sub_seq_duration);
      }

    int average_rateFlag = bitStream.readU1 ("SEI average_rateFlag");
    if (decoder->param.seiDebug)
      cLog::log (LOGINFO, "average_rateFlag %d", average_rateFlag);
    if (average_rateFlag) {
      int accurate_statisticsFlag = bitStream.readU1 ("SEI accurate_statisticsFlag");
      int average_bit_rate = bitStream.readUv (16, "SEI average_bit_rate");
      int average_frame_rate = bitStream.readUv (16, "SEI average_frame_rate");
      if (decoder->param.seiDebug) {
        cLog::log (LOGINFO, "accurate_statisticsFlag %d", accurate_statisticsFlag);
        cLog::log (LOGINFO, "average_bit_rate %d", average_bit_rate);
        cLog::log (LOGINFO, "average_frame_rate %d", average_frame_rate);
        }
      }

    int num_referenced_subseqs  = bitStream.readUeV("SEI num_referenced_subseqs");
    if (decoder->param.seiDebug)
      cLog::log (LOGINFO, "num_referenced_subseqs %d", num_referenced_subseqs);
    for (int i = 0; i < num_referenced_subseqs; i++) {
      int ref_sub_seq_layer_num  = bitStream.readUeV ("SEI ref_sub_seq_layer_num");
      int ref_sub_seq_id = bitStream.readUeV ("SEI ref_sub_seq_id");
      int ref_sub_seq_direction = bitStream.readU1  ("SEI ref_sub_seq_direction");
      if (decoder->param.seiDebug) {
        cLog::log (LOGINFO, "ref_sub_seq_layer_num %d", ref_sub_seq_layer_num);
        cLog::log (LOGINFO, "ref_sub_seq_id %d", ref_sub_seq_id);
        cLog::log (LOGINFO, "ref_sub_seq_direction %d", ref_sub_seq_direction);
        }
      }
    }
  //}}}
  //{{{
  void process_scene_information (sBitStream& bitStream, cDecoder264* decoder) {

    int scene_id = bitStream.readUeV ("SEI scene_id");
    int scene_transition_type = bitStream.readUeV ("SEI scene_transition_type");

    int second_scene_id = 0;
    if (scene_transition_type > 3)
      second_scene_id = bitStream.readUeV ("SEI scene_transition_type");

    if (decoder->param.seiDebug) {
      cLog::log (LOGINFO, "SceneInformation");
      cLog::log (LOGINFO, "scene_transition_type %d", scene_transition_type);
      cLog::log (LOGINFO, "scene_id %d", scene_id);
      if (scene_transition_type > 3 )
        cLog::log (LOGINFO, "second_scene_id %d", second_scene_id);
      }
    }
  //}}}

  //{{{
  void process_filler_payload_info (uint8_t* payload, int size, cDecoder264* decoder) {

    int payload_cnt = 0;
    while (payload_cnt < size)
      if (payload[payload_cnt] == 0xFF)
         payload_cnt++;

    if (decoder->param.seiDebug) {
      cLog::log (LOGINFO, "FillerPayload");
      if (payload_cnt == size)
        cLog::log (LOGINFO, "read %d bytes of filler payload", payload_cnt);
      else
        cLog::log (LOGINFO, "error reading filler payload: not all bytes are 0xFF (%d of %d)", payload_cnt, size);
      }
    }
  //}}}
  //{{{
  void process_full_frame_freeze_info (sBitStream& bitStream, cDecoder264* decoder) {

    int full_frame_freeze_repetition_period = bitStream.readUeV ("SEI full_frame_freeze_repetition_period");
    cLog::log (LOGINFO, "full_frame_freeze_repetition_period = %d", full_frame_freeze_repetition_period);

    }
  //}}}
  //{{{
  void process_full_frame_freeze_release_info (uint8_t* payload, int size, cDecoder264* decoder) {

    cLog::log (LOGINFO, "full-frame freeze release SEI");
    if (size)
      cLog::log (LOGINFO, "payload size should be zero, but is %d bytes.", size);
    }
  //}}}
  //{{{
  void process_full_frame_snapshot_info (sBitStream& bitStream, cDecoder264* decoder) {

    int snapshot_id = bitStream.readUeV ("SEI snapshot_id");

    cLog::log (LOGINFO, "Full-frame snapshot");
    cLog::log (LOGINFO, "snapshot_id = %d", snapshot_id);
    }
  //}}}

  //{{{
  void process_progressive_refinement_start_info (sBitStream& bitStream, cDecoder264* decoder) {

    int progressive_refinement_id   = bitStream.readUeV("SEI progressive_refinement_id"  );
    int num_refinement_steps_minus1 = bitStream.readUeV("SEI num_refinement_steps_minus1");
    cLog::log (LOGINFO, "Progressive refinement segment start");
    cLog::log (LOGINFO, "progressive_refinement_id   = %d", progressive_refinement_id);
    cLog::log (LOGINFO, "num_refinement_steps_minus1 = %d", num_refinement_steps_minus1);
    }
  //}}}
  //{{{
  void process_progressive_refinement_end_info (sBitStream& bitStream, cDecoder264* decoder) {

    int progressive_refinement_id   = bitStream.readUeV ("SEI progressive_refinement_id"  );
    cLog::log (LOGINFO, "progressive refinement segment end");
    cLog::log (LOGINFO, "progressive_refinement_id:%d", progressive_refinement_id);
    }
  //}}}

  //{{{
  void process_motion_constrained_slice_group_set_info (sBitStream& bitStream, cDecoder264* decoder) {

    int numSliceGroupsMinus1 = bitStream.readUeV ("SEI numSliceGroupsMinus1" );
    int sliceGroupSize = ceilLog2 (numSliceGroupsMinus1 + 1);
    cLog::log (LOGINFO, "Motion-constrained slice group set");
    cLog::log (LOGINFO, "numSliceGroupsMinus1 = %d", numSliceGroupsMinus1);

    for (int i = 0; i <= numSliceGroupsMinus1; i++) {
      int sliceGroupId = bitStream.readUv (sliceGroupSize, "SEI sliceGroupId" );
      cLog::log (LOGINFO, "sliceGroupId = %d", sliceGroupId);
      }

    int exact_matchFlag = bitStream.readU1 ("SEI exact_matchFlag"  );
    int pan_scan_rectFlag = bitStream.readU1 ("SEI pan_scan_rectFlag"  );

    cLog::log (LOGINFO, "exact_matchFlag = %d", exact_matchFlag);
    cLog::log (LOGINFO, "pan_scan_rectFlag = %d", pan_scan_rectFlag);

    if (pan_scan_rectFlag) {
      int pan_scan_rect_id = bitStream.readUeV ("SEI pan_scan_rect_id"  );
      cLog::log (LOGINFO, "pan_scan_rect_id = %d", pan_scan_rect_id);
      }
    }
  //}}}
  //{{{
  void processFilmGrain (sBitStream& bitStream, cDecoder264* decoder) {

    int film_grain_characteristics_cancelFlag;
    int model_id, separate_colour_description_presentFlag;
    int film_grain_bit_depth_luma_minus8, film_grain_bit_depth_chroma_minus8, film_grain_full_rangeFlag, film_grain_colour_primaries, film_grain_transfer_characteristics, film_grain_matrix_coefficients;
    int blending_mode_id, log2_scale_factor, comp_model_presentFlag[3];
    int num_intensity_intervals_minus1, num_model_values_minus1;
    int intensity_interval_lower_bound, intensity_interval_upper_bound;
    int comp_model_value;
    int film_grain_characteristics_repetition_period;

    film_grain_characteristics_cancelFlag = bitStream.readU1 ("SEI film_grain_characteristics_cancelFlag");
    cLog::log (LOGINFO, "film_grain_characteristics_cancelFlag = %d", film_grain_characteristics_cancelFlag);
    if (!film_grain_characteristics_cancelFlag) {
      model_id = bitStream.readUv(2, "SEI model_id");
      separate_colour_description_presentFlag = bitStream.readU1("SEI separate_colour_description_presentFlag");
      cLog::log (LOGINFO, "model_id = %d", model_id);
      cLog::log (LOGINFO, "separate_colour_description_presentFlag = %d", separate_colour_description_presentFlag);

      if (separate_colour_description_presentFlag) {
        film_grain_bit_depth_luma_minus8 = bitStream.readUv(3, "SEI film_grain_bit_depth_luma_minus8");
        film_grain_bit_depth_chroma_minus8 = bitStream.readUv(3, "SEI film_grain_bit_depth_chroma_minus8");
        film_grain_full_rangeFlag = bitStream.readUv(1, "SEI film_grain_full_rangeFlag");
        film_grain_colour_primaries = bitStream.readUv(8, "SEI film_grain_colour_primaries");
        film_grain_transfer_characteristics = bitStream.readUv(8, "SEI film_grain_transfer_characteristics");
        film_grain_matrix_coefficients = bitStream.readUv(8, "SEI film_grain_matrix_coefficients");
        cLog::log (LOGINFO, "film_grain_bit_depth_luma_minus8 = %d", film_grain_bit_depth_luma_minus8);
        cLog::log (LOGINFO, "film_grain_bit_depth_chroma_minus8 = %d", film_grain_bit_depth_chroma_minus8);
        cLog::log (LOGINFO, "film_grain_full_rangeFlag = %d", film_grain_full_rangeFlag);
        cLog::log (LOGINFO, "film_grain_colour_primaries = %d", film_grain_colour_primaries);
        cLog::log (LOGINFO, "film_grain_transfer_characteristics = %d", film_grain_transfer_characteristics);
        cLog::log (LOGINFO, "film_grain_matrix_coefficients = %d", film_grain_matrix_coefficients);
        }

      blending_mode_id = bitStream.readUv (2, "SEI blending_mode_id");
      log2_scale_factor = bitStream.readUv (4, "SEI log2_scale_factor");
      cLog::log (LOGINFO, "blending_mode_id = %d", blending_mode_id);
      cLog::log (LOGINFO, "log2_scale_factor = %d", log2_scale_factor);
      for (int c = 0; c < 3; c ++) {
        comp_model_presentFlag[c] = bitStream.readU1("SEI comp_model_presentFlag");
        cLog::log (LOGINFO, "comp_model_presentFlag = %d", comp_model_presentFlag[c]);
        }

      for (int c = 0; c < 3; c ++)
        if (comp_model_presentFlag[c]) {
          num_intensity_intervals_minus1 = bitStream.readUv(8, "SEI num_intensity_intervals_minus1");
          num_model_values_minus1 = bitStream.readUv(3, "SEI num_model_values_minus1");
          cLog::log (LOGINFO, "num_intensity_intervals_minus1 = %d", num_intensity_intervals_minus1);
          cLog::log (LOGINFO, "num_model_values_minus1 = %d", num_model_values_minus1);
          for (int i = 0; i <= num_intensity_intervals_minus1; i ++) {
            intensity_interval_lower_bound = bitStream.readUv(8, "SEI intensity_interval_lower_bound");
            intensity_interval_upper_bound = bitStream.readUv(8, "SEI intensity_interval_upper_bound");
            cLog::log (LOGINFO, "intensity_interval_lower_bound = %d", intensity_interval_lower_bound);
            cLog::log (LOGINFO, "intensity_interval_upper_bound = %d", intensity_interval_upper_bound);
            for (int j = 0; j <= num_model_values_minus1; j++) {
              comp_model_value = bitStream.readSeV("SEI comp_model_value");
              cLog::log (LOGINFO, "comp_model_value = %d", comp_model_value);
              }
            }
          }
      film_grain_characteristics_repetition_period =
        bitStream.readUeV ("SEI film_grain_characteristics_repetition_period");
      cLog::log (LOGINFO, "film_grain_characteristics_repetition_period = %d",
              film_grain_characteristics_repetition_period);
      }
    }
  //}}}
  //{{{
  void processDeblockFilterDisplayPref (sBitStream& bitStream, cDecoder264* decoder) {

    int deblocking_display_preference_cancelFlag = bitStream.readU1("SEI deblocking_display_preference_cancelFlag");
    cLog::log (LOGINFO, "deblocking_display_preference_cancelFlag = %d", deblocking_display_preference_cancelFlag);
    if (!deblocking_display_preference_cancelFlag) {
      int display_prior_to_deblocking_preferredFlag = bitStream.readU1("SEI display_prior_to_deblocking_preferredFlag");
      int dec_frame_buffering_constraintFlag = bitStream.readU1("SEI dec_frame_buffering_constraintFlag");
      int deblocking_display_preference_repetition_period = bitStream.readUeV("SEI deblocking_display_preference_repetition_period");

      cLog::log (LOGINFO, "display_prior_to_deblocking_preferredFlag = %d", display_prior_to_deblocking_preferredFlag);
      cLog::log (LOGINFO, "dec_frame_buffering_constraintFlag = %d", dec_frame_buffering_constraintFlag);
      cLog::log (LOGINFO, "deblocking_display_preference_repetition_period = %d", deblocking_display_preference_repetition_period);
      }
    }
  //}}}
  //{{{
  void processStereoVideo (sBitStream& bitStream, cDecoder264* decoder) {

    int field_viewsFlags = bitStream.readU1 ("SEI field_viewsFlags");
    cLog::log (LOGINFO, "field_viewsFlags = %d", field_viewsFlags);
    if (field_viewsFlags) {
      int top_field_is_left_viewFlag = bitStream.readU1 ("SEI top_field_is_left_viewFlag");
      cLog::log (LOGINFO, "top_field_is_left_viewFlag = %d", top_field_is_left_viewFlag);
      }
    else {
      int current_frame_is_left_viewFlag = bitStream.readU1 ("SEI current_frame_is_left_viewFlag");
      int next_frame_is_second_viewFlag = bitStream.readU1 ("SEI next_frame_is_second_viewFlag");
      cLog::log (LOGINFO, "current_frame_is_left_viewFlag = %d", current_frame_is_left_viewFlag);
      cLog::log (LOGINFO, "next_frame_is_second_viewFlag = %d", next_frame_is_second_viewFlag);
      }

    int left_view_self_containedFlag = bitStream.readU1 ("SEI left_view_self_containedFlag");
    int right_view_self_containedFlag = bitStream.readU1 ("SEI right_view_self_containedFlag");
    cLog::log (LOGINFO, "left_view_self_containedFlag = %d", left_view_self_containedFlag);
    cLog::log (LOGINFO, "right_view_self_containedFlag = %d", right_view_self_containedFlag);
    }
  //}}}
  //{{{
  void processBufferingPeriod (sBitStream& bitStream, cDecoder264* decoder) {

    int spsId = bitStream.readUeV ("SEI spsId");
    cSps* sps = &decoder->sps[spsId];
    //useSps (decoder, sps);

    if (decoder->param.seiDebug)
      cLog::log (LOGINFO, "buffering");

    // Note: NalHrdBpPresentFlag and CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
    if (sps->hasVui) {
      int initial_cpb_removal_delay;
      int initial_cpb_removal_delay_offset;
      if (sps->vuiSeqParams.nal_hrd_parameters_presentFlag) {
        for (uint32_t k = 0; k < sps->vuiSeqParams.nal_hrd_parameters.cpb_cnt_minus1+1; k++) {
          initial_cpb_removal_delay = bitStream.readUv(sps->vuiSeqParams.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay"        );
          initial_cpb_removal_delay_offset = bitStream.readUv(sps->vuiSeqParams.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay_offset" );
          if (decoder->param.seiDebug) {
            cLog::log (LOGINFO, "nal initial_cpb_removal_delay[%d] = %d", k, initial_cpb_removal_delay);
            cLog::log (LOGINFO, "nal initial_cpb_removal_delay_offset[%d] = %d", k, initial_cpb_removal_delay_offset);
            }
          }
        }

      if (sps->vuiSeqParams.vcl_hrd_parameters_presentFlag) {
        for (uint32_t k = 0; k < sps->vuiSeqParams.vcl_hrd_parameters.cpb_cnt_minus1+1; k++) {
          initial_cpb_removal_delay = bitStream.readUv(sps->vuiSeqParams.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay"        );
          initial_cpb_removal_delay_offset = bitStream.readUv(sps->vuiSeqParams.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay_offset" );
          if (decoder->param.seiDebug) {
            cLog::log (LOGINFO, "vcl initial_cpb_removal_delay[%d] = %d", k, initial_cpb_removal_delay);
            cLog::log (LOGINFO, "vcl initial_cpb_removal_delay_offset[%d] = %d", k, initial_cpb_removal_delay_offset);
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void process_frame_packing_arrangement_info (sBitStream& bitStream, cDecoder264* decoder) {

    frame_packing_arrangement_information_struct seiFramePackingArrangement;

    cLog::log (LOGINFO, "Frame packing arrangement");

    seiFramePackingArrangement.frame_packing_arrangement_id =
      (uint32_t)bitStream.readUeV( "SEI frame_packing_arrangement_id");
    seiFramePackingArrangement.frame_packing_arrangement_cancelFlag =
      bitStream.readU1( "SEI frame_packing_arrangement_cancelFlag");
    cLog::log (LOGINFO, "frame_packing_arrangement_id = %d", seiFramePackingArrangement.frame_packing_arrangement_id);
    cLog::log (LOGINFO, "frame_packing_arrangement_cancelFlag = %d", seiFramePackingArrangement.frame_packing_arrangement_cancelFlag);

    if ( seiFramePackingArrangement.frame_packing_arrangement_cancelFlag == false ) {
      seiFramePackingArrangement.frame_packing_arrangement_type = (uint8_t)bitStream.readUv( 7, "SEI frame_packing_arrangement_type");
      seiFramePackingArrangement.quincunx_samplingFlag = bitStream.readU1 ("SEI quincunx_samplingFlag");
      seiFramePackingArrangement.content_interpretation_type = (uint8_t)bitStream.readUv( 6, "SEI content_interpretation_type");
      seiFramePackingArrangement.spatial_flippingFlag = bitStream.readU1 ("SEI spatial_flippingFlag");
      seiFramePackingArrangement.frame0_flippedFlag = bitStream.readU1 ("SEI frame0_flippedFlag");
      seiFramePackingArrangement.field_viewsFlag = bitStream.readU1 ("SEI field_viewsFlag");
      seiFramePackingArrangement.current_frame_is_frame0Flag = bitStream.readU1 ("SEI current_frame_is_frame0Flag");
      seiFramePackingArrangement.frame0_self_containedFlag = bitStream.readU1 ("SEI frame0_self_containedFlag");
      seiFramePackingArrangement.frame1_self_containedFlag = bitStream.readU1 ("SEI frame1_self_containedFlag");

      cLog::log (LOGINFO, "frame_packing_arrangement_type = %d", seiFramePackingArrangement.frame_packing_arrangement_type);
      cLog::log (LOGINFO, "quincunx_samplingFlag          = %d", seiFramePackingArrangement.quincunx_samplingFlag);
      cLog::log (LOGINFO, "content_interpretation_type    = %d", seiFramePackingArrangement.content_interpretation_type);
      cLog::log (LOGINFO, "spatial_flippingFlag           = %d", seiFramePackingArrangement.spatial_flippingFlag);
      cLog::log (LOGINFO, "frame0_flippedFlag             = %d", seiFramePackingArrangement.frame0_flippedFlag);
      cLog::log (LOGINFO, "field_viewsFlag                = %d", seiFramePackingArrangement.field_viewsFlag);
      cLog::log (LOGINFO, "current_frame_is_frame0Flag    = %d", seiFramePackingArrangement.current_frame_is_frame0Flag);
      cLog::log (LOGINFO, "frame0_self_containedFlag      = %d", seiFramePackingArrangement.frame0_self_containedFlag);
      cLog::log (LOGINFO, "frame1_self_containedFlag      = %d", seiFramePackingArrangement.frame1_self_containedFlag);
      if (seiFramePackingArrangement.quincunx_samplingFlag == false &&
          seiFramePackingArrangement.frame_packing_arrangement_type != 5 )  {
        seiFramePackingArrangement.frame0_grid_position_x =
          (uint8_t)bitStream.readUv( 4, "SEI frame0_grid_position_x");
        seiFramePackingArrangement.frame0_grid_position_y =
          (uint8_t)bitStream.readUv( 4, "SEI frame0_grid_position_y");
        seiFramePackingArrangement.frame1_grid_position_x =
          (uint8_t)bitStream.readUv( 4, "SEI frame1_grid_position_x");
        seiFramePackingArrangement.frame1_grid_position_y =
          (uint8_t)bitStream.readUv( 4, "SEI frame1_grid_position_y");

        cLog::log (LOGINFO, "frame0_grid_position_x = %d", seiFramePackingArrangement.frame0_grid_position_x);
        cLog::log (LOGINFO, "frame0_grid_position_y = %d", seiFramePackingArrangement.frame0_grid_position_y);
        cLog::log (LOGINFO, "frame1_grid_position_x = %d", seiFramePackingArrangement.frame1_grid_position_x);
        cLog::log (LOGINFO, "frame1_grid_position_y = %d", seiFramePackingArrangement.frame1_grid_position_y);
        }
      seiFramePackingArrangement.frame_packing_arrangement_reserved_byte = (uint8_t)bitStream.readUv( 8, "SEI frame_packing_arrangement_reserved_byte");
      seiFramePackingArrangement.frame_packing_arrangement_repetition_period = (uint32_t)bitStream.readUeV( "SEI frame_packing_arrangement_repetition_period");
      cLog::log (LOGINFO, "frame_packing_arrangement_reserved_byte = %d", seiFramePackingArrangement.frame_packing_arrangement_reserved_byte);
      cLog::log (LOGINFO, "frame_packing_arrangement_repetition_period  = %d", seiFramePackingArrangement.frame_packing_arrangement_repetition_period);
      }
    seiFramePackingArrangement.frame_packing_arrangement_extensionFlag = bitStream.readU1( "SEI frame_packing_arrangement_extensionFlag");
    cLog::log (LOGINFO, "frame_packing_arrangement_extensionFlag = %d", seiFramePackingArrangement.frame_packing_arrangement_extensionFlag);
    }
  //}}}

  //{{{
  void process_post_filter_hints_info (sBitStream& bitStream, cDecoder264* decoder) {

    uint32_t filter_hint_size_y = bitStream.readUeV("SEI filter_hint_size_y"); // interpret post-filter hint SEI here
    uint32_t filter_hint_size_x = bitStream.readUeV("SEI filter_hint_size_x"); // interpret post-filter hint SEI here
    uint32_t filter_hint_type = bitStream.readUv(2, "SEI filter_hint_type"); // interpret post-filter hint SEI here

    int** *filter_hint;
    getMem3Dint (&filter_hint, 3, filter_hint_size_y, filter_hint_size_x);

    for (uint32_t color_component = 0; color_component < 3; color_component ++)
      for (uint32_t cy = 0; cy < filter_hint_size_y; cy ++)
        for (uint32_t cx = 0; cx < filter_hint_size_x; cx ++)
          filter_hint[color_component][cy][cx] = bitStream.readSeV("SEI filter_hint"); // interpret post-filter hint SEI here

    uint32_t additional_extensionFlag = bitStream.readU1("SEI additional_extensionFlag"); // interpret post-filter hint SEI here

    cLog::log (LOGINFO, "Post-filter hint");
    cLog::log (LOGINFO, "post_filter_hint_size_y %d ", filter_hint_size_y);
    cLog::log (LOGINFO, "post_filter_hint_size_x %d ", filter_hint_size_x);
    cLog::log (LOGINFO, "post_filter_hint_type %d ",   filter_hint_type);
    for (uint32_t color_component = 0; color_component < 3; color_component ++)
      for (uint32_t cy = 0; cy < filter_hint_size_y; cy ++)
        for (uint32_t cx = 0; cx < filter_hint_size_x; cx ++)
          cLog::log (LOGINFO, " post_filter_hint[%d][%d][%d] %d ", color_component, cy, cx, filter_hint[color_component][cy][cx]);

    cLog::log (LOGINFO, "additional_extensionFlag %d ", additional_extensionFlag);

    freeMem3Dint (filter_hint);
    }
  //}}}
  //{{{
  void process_green_metadata_info (sBitStream& bitStream, cDecoder264* decoder) {

    cLog::log (LOGINFO, "GreenMetadataInfo");

    Green_metadata_information_struct seiGreenMetadataInfo;
    seiGreenMetadataInfo.green_metadata_type = (uint8_t)bitStream.readUv(8, "SEI green_metadata_type");
    cLog::log (LOGINFO, "green_metadata_type = %d", seiGreenMetadataInfo.green_metadata_type);

    if (seiGreenMetadataInfo.green_metadata_type == 0) {
        seiGreenMetadataInfo.period_type=(uint8_t)bitStream.readUv(8, "SEI green_metadata_period_type");
      cLog::log (LOGINFO, "green_metadata_period_type = %d", seiGreenMetadataInfo.period_type);

      if (seiGreenMetadataInfo.period_type == 2) {
        seiGreenMetadataInfo.num_seconds = (uint16_t)bitStream.readUv(16, "SEI green_metadata_num_seconds");
        cLog::log (LOGINFO, "green_metadata_num_seconds = %d", seiGreenMetadataInfo.num_seconds);
        }
      else if (seiGreenMetadataInfo.period_type == 3) {
        seiGreenMetadataInfo.num_pictures = (uint16_t)bitStream.readUv(16, "SEI green_metadata_num_pictures");
        cLog::log (LOGINFO, "green_metadata_num_pictures = %d", seiGreenMetadataInfo.num_pictures);
        }

      seiGreenMetadataInfo.percent_non_zero_macroBlocks = (uint8_t)bitStream.readUv(8, "SEI percent_non_zero_macroBlocks");
      seiGreenMetadataInfo.percent_intra_coded_macroBlocks = (uint8_t)bitStream.readUv(8, "SEI percent_intra_coded_macroBlocks");
      seiGreenMetadataInfo.percent_six_tap_filtering = (uint8_t)bitStream.readUv(8, "SEI percent_six_tap_filtering");
      seiGreenMetadataInfo.percent_alpha_point_deblocking_instance =
        (uint8_t)bitStream.readUv(8, "SEI percent_alpha_point_deblocking_instance");

      cLog::log (LOGINFO, "percent_non_zero_macroBlocks = %f", (float)seiGreenMetadataInfo.percent_non_zero_macroBlocks/255);
      cLog::log (LOGINFO, "percent_intra_coded_macroBlocks = %f", (float)seiGreenMetadataInfo.percent_intra_coded_macroBlocks/255);
      cLog::log (LOGINFO, "percent_six_tap_filtering = %f", (float)seiGreenMetadataInfo.percent_six_tap_filtering/255);
      cLog::log (LOGINFO, "percent_alpha_point_deblocking_instance      = %f", (float)seiGreenMetadataInfo.percent_alpha_point_deblocking_instance/255);
      }
    else if (seiGreenMetadataInfo.green_metadata_type == 1) {
      seiGreenMetadataInfo.xsd_metric_type = (uint8_t)bitStream.readUv(8, "SEI: xsd_metric_type");
      seiGreenMetadataInfo.xsd_metric_value = (uint16_t)bitStream.readUv(16, "SEI xsd_metric_value");
      cLog::log (LOGINFO, "xsd_metric_type      = %d", seiGreenMetadataInfo.xsd_metric_type);
      if (seiGreenMetadataInfo.xsd_metric_type == 0)
        cLog::log (LOGINFO, "xsd_metric_value      = %f", (float)seiGreenMetadataInfo.xsd_metric_value/100);
      }
    }
  //}}}
  }

//{{{
void processSei (uint8_t* naluPayload, int naluPayloadLen, cDecoder264* decoder, cSlice* slice) {

  if (decoder->param.seiDebug)
    cLog::log (LOGINFO, fmt::format ("SEI:{} -> ", naluPayloadLen));

  uint32_t offset = 0;
  do {
    uint32_t payloadType = 0;
    uint8_t tempByte = naluPayload[offset++];
    while (tempByte == 0xFF) {
      payloadType += 255;
      tempByte = naluPayload[offset++];
      }
    payloadType += tempByte;

    uint32_t payloadSize = 0;
    tempByte = naluPayload[offset++];
    while (tempByte == 0xFF) {
      payloadSize += 255;
      tempByte = naluPayload[offset++];
      }
    payloadSize += tempByte;

    // simple use of bitStream.mBuffer, points into nalu, no alloc
    sBitStream bitStream = {0};
    bitStream.mBuffer = naluPayload + offset;
    bitStream.mLength = payloadSize;

    switch (payloadType) {
      case SEI_BUFFERING_PERIOD:
        processBufferingPeriod (bitStream, decoder); break;
      case SEI_PIC_TIMING:
        processPictureTiming (bitStream, decoder); break;
      case SEI_PAN_SCAN_RECT:
        processPanScan (bitStream, decoder); break;
      case SEI_FILLER_PAYLOAD:
        process_filler_payload_info (naluPayload + offset, payloadSize, decoder); break;
      case SEI_USER_DATA_REGISTERED_ITU_T_T35:
        processUserDataT35 (naluPayload + offset, payloadSize, decoder); break;
      case SEI_USER_DATA_UNREGISTERED:
        processUserDataUnregistered (naluPayload + offset, payloadSize, decoder); break;
      case SEI_RECOVERY_POINT:
        processRecoveryPoint (bitStream, decoder); break;
      case SEI_DEC_REF_PIC_MARKING_REPETITION:
        processDecRefPicMarkingRepetition (bitStream, decoder, slice); break;
      case SEI_SPARE_PIC:
        process_spare_pic (bitStream, decoder); break;
      case SEI_SCENE_INFO:
        process_scene_information (bitStream, decoder); break;
      case SEI_SUB_SEQ_INFO:
        process_subsequence_info (bitStream, decoder); break;
      case SEI_SUB_SEQ_LAYER_CHARACTERISTICS:
        process_subsequence_layer_characteristics_info (bitStream, decoder); break;
      case SEI_SUB_SEQ_CHARACTERISTICS:
        process_subsequence_characteristics_info (bitStream, decoder); break;
      case SEI_FULL_FRAME_FREEZE:
        process_full_frame_freeze_info (bitStream, decoder); break;
      case SEI_FULL_FRAME_FREEZE_RELEASE:
        process_full_frame_freeze_release_info (naluPayload + offset, payloadSize, decoder); break;
      case SEI_FULL_FRAME_SNAPSHOT:
        process_full_frame_snapshot_info (bitStream, decoder); break;
      case SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START:
        process_progressive_refinement_start_info (bitStream, decoder); break;
      case SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END:
        process_progressive_refinement_end_info (bitStream, decoder); break;
      case SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET:
        process_motion_constrained_slice_group_set_info (bitStream, decoder); break;
      case SEI_FILM_GRAIN_CHARACTERISTICS:
        processFilmGrain (bitStream, decoder); break;
      case SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE:
        processDeblockFilterDisplayPref (bitStream, decoder); break;
      case SEI_STEREO_VIDEO_INFO:
        processStereoVideo  (bitStream, decoder); break;
      case SEI_POST_FILTER_HINTS:
        process_post_filter_hints_info (bitStream, decoder); break;
      case SEI_FRAME_PACKING_ARRANGEMENT:
        process_frame_packing_arrangement_info (bitStream, decoder); break;
      case SEI_GREEN_METADATA:
        process_green_metadata_info (bitStream, decoder); break;
      default:
        processReserved (naluPayload + offset, payloadSize, decoder); break;
      }
    offset += payloadSize;
    } while (naluPayload[offset] != 0x80);

  // ignore the trailing bits rbsp_trailing_bits();
  }
//}}}
