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
enum eSeiType {
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
//{{{
struct sGreenMetadataInfo {
  uint8_t  type;
  uint8_t  period_type;
  uint16_t num_seconds;
  uint16_t num_pictures;
  uint8_t  percent_non_zero_macroBlocks;
  uint8_t  percent_intra_coded_macroBlocks;
  uint8_t  percent_six_tap_filtering;
  uint8_t  percent_alpha_point_deblocking_instance;
  uint8_t  xsd_metric_type;
  uint16_t xsd_metric_value;
  };
//}}}
//{{{
struct sFramePackingArrangementInfo {
  uint32_t id;
  bool     cancelFlag;
  uint8_t  type;
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
  uint8_t  reserved_byte;
  uint32_t repetition_period;
  bool     extensionFlag;
  };
//}}}

namespace {
  //{{{
  void processUserDataUnregistered (uint8_t* payload, int len, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      string payloadString;
      for (int i = 0; i < len; i++)
        if (payload[i] >= 0x20 && payload[i] <= 0x7f)
          payloadString += fmt::format ("{}", char(payload[i]));
        else
          payloadString += fmt::format ("<{:02x}>", payload[i]);

      cLog::log (LOGINFO, fmt::format ("- unregistered {}", payloadString));
      }
    }
  //}}}
  //{{{
  void processUserDataT35 (uint8_t* payload, int len, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      string payloadString;
      int offset = 0;
      int itu_t_t35_country_code = payload[offset++];

      if (itu_t_t35_country_code == 0xFF) {
        int itu_t_t35_country_code_extension_byte = payload[offset++];
        payloadString += fmt::format (" ext:{} - ", itu_t_t35_country_code_extension_byte);
        }

      for (int i = offset; i < len; i++)
        if (payload[i] >= 0x20 && payload[i] <= 0x7f)
          payloadString += fmt::format ("{}", char(payload[i]));
        else
          payloadString += fmt::format ("<{:02x}>", payload[i]);

      cLog::log (LOGINFO, fmt::format ("- ITU-T:{} {}", itu_t_t35_country_code, payloadString));
      }
    }
  //}}}
  //{{{
  void processReserved (uint8_t* payload, int len, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      string payloadString;
      for (int i = 0; i < len; i++)
        if (payload[i] >= 0x20 && payload[i] <= 0x7f)
          payloadString += fmt::format ("{}", char(payload[i]));
        else
          payloadString += fmt::format ("<{:02x}>", payload[i]);
      cLog::log (LOGINFO, fmt::format ("- reserved {}", payloadString));
      }
    }
  //}}}
  //{{{
  void processFiller (uint8_t* payload, int len, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      int payload_cnt = 0;
      while (payload_cnt < len)
        if (payload[payload_cnt] == 0xFF)
           payload_cnt++;

      cLog::log (LOGINFO, fmt::format ("FillerPayload"));
      if (payload_cnt == len)
        cLog::log (LOGINFO, fmt::format ("read {} bytes of filler payload", payload_cnt));
      else
        cLog::log (LOGINFO, fmt::format ("error reading filler payload: not all bytes are 0xFF ({} of {})", payload_cnt, len));
      }
    }
  //}}}

  //{{{
  void processPictureTiming (sBitStream& bitStream, cDecoder264* decoder) {

    cSps* activeSps = decoder->activeSps;
    if (!activeSps) {
      if (decoder->param.seiDebug)
        cLog::log (LOGINFO, fmt::format ("- pictureTiming - no active SPS"));
      return;
      }

    if (decoder->param.seiDebug) {
      cLog::log (LOGINFO, fmt::format ("- pictureTiming"));

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
          cLog::log (LOGINFO, fmt::format (" cpb:{}", cpb_removal_delay));
          dpb_output_delay = bitStream.readUv (dpb_output_len,  "SEI dpb_output_delay");
          cLog::log (LOGINFO, fmt::format (" dpb:{}", dpb_output_delay));
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
              cLog::log (LOGINFO, fmt::format (" clockTs"));
              int ct_type = bitStream.readUv (2, "SEI ct_type");
              int nuit_field_basedFlag = bitStream.readU1 ("SEI nuit_field_basedFlag");
              int counting_type = bitStream.readUv (5, "SEI counting_type");
              int full_timestampFlag = bitStream.readU1 ("SEI full_timestampFlag");
              int discontinuityFlag = bitStream.readU1 ("SEI discontinuityFlag");
              int cnt_droppedFlag = bitStream.readU1 ("SEI cnt_droppedFlag");
              int nframes = bitStream.readUv (8, "SEI nframes");

              cLog::log (LOGINFO, fmt::format ("ct:{}",ct_type));
              cLog::log (LOGINFO, fmt::format ("nuit:{}",nuit_field_basedFlag));
              cLog::log (LOGINFO, fmt::format ("full:{}",full_timestampFlag));
              cLog::log (LOGINFO, fmt::format ("dis:{}",discontinuityFlag));
              cLog::log (LOGINFO, fmt::format ("drop:{}",cnt_droppedFlag));
              cLog::log (LOGINFO, fmt::format ("nframes:{}",nframes));

              if (full_timestampFlag) {
                int seconds_value = bitStream.readUv (6, "SEI seconds_value");
                int minutes_value = bitStream.readUv (6, "SEI minutes_value");
                int hours_value = bitStream.readUv (5, "SEI hours_value");
                cLog::log (LOGINFO, fmt::format ("sec:{}", seconds_value));
                cLog::log (LOGINFO, fmt::format ("min:{}", minutes_value));
                cLog::log (LOGINFO, fmt::format ("hour:{}", hours_value));
                }
              else {
                int secondsFlag = bitStream.readU1 ("SEI secondsFlag");
                cLog::log (LOGINFO, fmt::format ("sec:{}", secondsFlag));
                if (secondsFlag) {
                  int seconds_value = bitStream.readUv (6, "SEI seconds_value");
                  int minutesFlag = bitStream.readU1 ("SEI minutesFlag");
                  cLog::log (LOGINFO, fmt::format ("sec:{}", seconds_value));
                  cLog::log (LOGINFO, fmt::format ("min:{}", minutesFlag));
                  if(minutesFlag) {
                    int minutes_value = bitStream.readUv(6, "SEI minutes_value");
                    int hoursFlag = bitStream.readU1 ("SEI hoursFlag");
                    cLog::log (LOGINFO, fmt::format ("min:{}", minutes_value));
                    cLog::log (LOGINFO, fmt::format ("hour{}", hoursFlag));
                    if (hoursFlag) {
                      int hours_value = bitStream.readUv (5, "SEI hours_value");
                      cLog::log (LOGINFO, fmt::format ("hour:{}", hours_value));
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
              cLog::log (LOGINFO, fmt::format ("time:{}", time_offset));
              }
            }
            //}}}
        }
      }
    }
  //}}}
  //{{{
  void processPanScan (sBitStream& bitStream, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      int pan_scan_rect_id =  bitStream.readUeV ("SEI pan_scan_rect_id");

      int pan_scan_rect_cancelFlag =  bitStream.readU1 ("SEI pan_scan_rect_cancelFlag");
      if (!pan_scan_rect_cancelFlag) {
        int pan_scan_cnt_minus1 =  bitStream.readUeV ("SEI pan_scan_cnt_minus1");
        for (int i = 0; i <= pan_scan_cnt_minus1; i++) {
          int pan_scan_rect_left_offset = bitStream.readSeV ("SEI pan_scan_rect_left_offset");
          int pan_scan_rect_right_offset = bitStream.readSeV ("SEI pan_scan_rect_right_offset");
          int pan_scan_rect_top_offset = bitStream.readSeV ("SEI pan_scan_rect_top_offset");
          int pan_scan_rect_bottom_offset = bitStream.readSeV ("SEI pan_scan_rect_bottom_offset");
          cLog::log (LOGINFO, fmt::format ("- panScan {}/{} id:{} left:{} right:{} top:{} bot:{}",
                                           i, pan_scan_cnt_minus1, pan_scan_rect_id,
                                           pan_scan_rect_left_offset, pan_scan_rect_right_offset,
                                           pan_scan_rect_top_offset, pan_scan_rect_bottom_offset));
          }

        int pan_scan_rect_repetition_period = bitStream.readUeV ("SEI pan_scan_rect_repetition_period");
        }
      }
    }
  //}}}
  //{{{
  void processRecoveryPoint (sBitStream& bitStream, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      int recoveryFrameCount = bitStream.readUeV ("SEI recoveryFrameCount");
      int exact_matchFlag = bitStream.readU1 ("SEI exact_matchFlag");
      int broken_linkFlag = bitStream.readU1 ("SEI broken_linkFlag");
      int changing_slice_group_idc = bitStream.readUv (2, "SEI changing_slice_group_idc");

      decoder->recoveryPoint = 1;
      decoder->recoveryFrameCount = recoveryFrameCount;

      cLog::log (LOGINFO, fmt::format ("- recovery - frame:{} exact:{} broken:{} changing:{}",
                          recoveryFrameCount, exact_matchFlag, broken_linkFlag, changing_slice_group_idc));
      }
    }
  //}}}
  //{{{
  void processDecRefPicMarkingRepetition (sBitStream& bitStream, cDecoder264* decoder, cSlice *slice) {

    if (decoder->param.seiDebug) {
      int original_idrFlag = bitStream.readU1 ("SEI original_idrFlag");
      int original_frame_num = bitStream.readUeV ("SEI original_frame_num");

      int original_bottom_fieldFlag = 0;
      if (!decoder->activeSps->frameMbOnly) {
        int original_field_picFlag =  bitStream.readU1 ("SEI original_field_picFlag");
        if (original_field_picFlag)
          original_bottom_fieldFlag = bitStream.readU1 ("SEI original_bottom_fieldFlag");
        }

      cLog::log (LOGINFO, fmt::format ("- decPicRepetition orig:{}:{}", original_idrFlag, original_frame_num));
      }
    }
  //}}}

  //{{{
  void processSpare (sBitStream& bitStream, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      int target_frame_num = bitStream.readUeV ("SEI target_frame_num");
      int num_spare_pics = bitStream.readUeV ("SEI num_spare_pics_minus1") + 1;
      cLog::log (LOGINFO, fmt::format ("sparePicture target_frame_num:{} num_spare_pics:{}",
                                       target_frame_num, num_spare_pics));

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

        delta_spare_frame_num = bitStream.readUeV ("SEI delta_spare_frame_num");
        spareFrameNum = candidateSpareFrameNum - delta_spare_frame_num;
        if (spareFrameNum < 0)
          spareFrameNum = kMaxFunction + spareFrameNum;

        int ref_area_indicator = bitStream.readUeV ("SEI ref_area_indicator");
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
            cDecoder264::error (fmt::format ("Wrong ref_area_indicator {}", ref_area_indicator));
            exit(0);
            break;
          //}}}
          }
        }

      freeMem3D (map);
      }
    }
  //}}}

  //{{{
  void processSubsequence (sBitStream& bitStream, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      int sub_seq_layer_num =  bitStream.readUeV ("SEI sub_seq_layer_num");
      int sub_seq_id = bitStream.readUeV ("SEI sub_seq_id");
      int first_ref_picFlag = bitStream.readU1 ("SEI first_ref_picFlag");
      int leading_non_ref_picFlag = bitStream.readU1 ("SEI leading_non_ref_picFlag");
      int last_picFlag = bitStream.readU1 ("SEI last_picFlag");

      int sub_seq_frame_num = 0;
      int sub_seq_frame_numFlag = bitStream.readU1 ("SEI sub_seq_frame_numFlag");
      if (sub_seq_frame_numFlag)
        sub_seq_frame_num = bitStream.readUeV ("SEI sub_seq_frame_num");

      cLog::log (LOGINFO, fmt::format ("Sub-sequence information"));
      cLog::log (LOGINFO, fmt::format ("- sub_seq_layer_num:{}", sub_seq_layer_num));
      cLog::log (LOGINFO, fmt::format ("- sub_seq_id:{}", sub_seq_id));
      cLog::log (LOGINFO, fmt::format ("- first_ref_picFlag:{}", first_ref_picFlag));
      cLog::log (LOGINFO, fmt::format ("- leading_non_ref_picFlag:{}", leading_non_ref_picFlag));
      cLog::log (LOGINFO, fmt::format ("- last_picFlag:{}", last_picFlag));
      cLog::log (LOGINFO, fmt::format ("- sub_seq_frame_numFlag:{}", sub_seq_frame_numFlag));
      if (sub_seq_frame_numFlag)
        cLog::log (LOGINFO, fmt::format ("- sub_seq_frame_num:{}", sub_seq_frame_num));
      }
    }
  //}}}
  //{{{
  void processSubsequenceLayerCharacteristics (sBitStream& bitStream, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      long num_sub_layers = bitStream.readUeV ("SEI num_sub_layers_minus1") + 1;
      cLog::log (LOGINFO, fmt::format ("Sub-sequence layer characteristics num_sub_layers_minus1 %ld",
                                       num_sub_layers - 1));

      for (int i = 0; i < num_sub_layers; i++) {
        long accurate_statisticsFlag = bitStream.readU1 ("SEI accurate_statisticsFlag");
        long average_bit_rate = bitStream.readUv (16, "SEI average_bit_rate");
        long average_frame_rate = bitStream.readUv (16, "SEI average_frame_rate");

        cLog::log (LOGINFO, fmt::format ("- layer {}: accurate_statisticsFlag = %ld", i, accurate_statisticsFlag));
        cLog::log (LOGINFO, fmt::format ("- layer {}: average_bit_rate = %ld", i, average_bit_rate));
        cLog::log (LOGINFO, fmt::format ("- layer {}: average_frame_rate= %ld", i, average_frame_rate));
        }
      }
    }
  //}}}
  //{{{
  void processSubsequenceCharacteristics (sBitStream& bitStream, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      int sub_seq_layer_num = bitStream.readUeV ("SEI sub_seq_layer_num");
      int sub_seq_id = bitStream.readUeV ("SEI sub_seq_id");
      int durationFlag = bitStream.readU1 ("SEI durationFlag");

      cLog::log (LOGINFO, fmt::format ("Sub-sequence characteristics"));
      cLog::log (LOGINFO, fmt::format ("- sub_seq_layer_num {}", sub_seq_layer_num));
      cLog::log (LOGINFO, fmt::format ("- sub_seq_id {}", sub_seq_id));
      cLog::log (LOGINFO, fmt::format ("- durationFlag {}", durationFlag));

      if (durationFlag) {
        int sub_seq_duration = bitStream.readUv (32, "SEI durationFlag");
        cLog::log (LOGINFO, fmt::format ("sub_seq_duration:{}", sub_seq_duration));
        }

      int average_rateFlag = bitStream.readU1 ("SEI average_rateFlag");
      cLog::log (LOGINFO, fmt::format ("average_rateFlag {}", average_rateFlag));
      if (average_rateFlag) {
        int accurate_statisticsFlag = bitStream.readU1 ("SEI accurate_statisticsFlag");
        int average_bit_rate = bitStream.readUv (16, "SEI average_bit_rate");
        int average_frame_rate = bitStream.readUv (16, "SEI average_frame_rate");

        cLog::log (LOGINFO, fmt::format ("accurate_statisticsFlag {}", accurate_statisticsFlag));
        cLog::log (LOGINFO, fmt::format ("average_bit_rate {}", average_bit_rate));
        cLog::log (LOGINFO, fmt::format ("average_frame_rate {}", average_frame_rate));
        }

      int num_referenced_subseqs  = bitStream.readUeV("SEI num_referenced_subseqs");
      cLog::log (LOGINFO, fmt::format ("num_referenced_subseqs {}", num_referenced_subseqs));
      for (int i = 0; i < num_referenced_subseqs; i++) {
        int ref_sub_seq_layer_num  = bitStream.readUeV ("SEI ref_sub_seq_layer_num");
        int ref_sub_seq_id = bitStream.readUeV ("SEI ref_sub_seq_id");
        int ref_sub_seq_direction = bitStream.readU1  ("SEI ref_sub_seq_direction");

        cLog::log (LOGINFO, fmt::format ("ref_sub_seq_layer_num {}", ref_sub_seq_layer_num));
        cLog::log (LOGINFO, fmt::format ("ref_sub_seq_id {}", ref_sub_seq_id));
        cLog::log (LOGINFO, fmt::format ("ref_sub_seq_direction {}", ref_sub_seq_direction));
        }
      }
    }
  //}}}
  //{{{
  void processScene (sBitStream& bitStream, cDecoder264* decoder) {

    if (decoder->param.seiDebug) {
      int scene_id = bitStream.readUeV ("SEI scene_id");
      int scene_transition_type = bitStream.readUeV ("SEI scene_transition_type");
      int second_scene_id = 0;
      if (scene_transition_type > 3)
        second_scene_id = bitStream.readUeV ("SEI scene_transition_type");

      cLog::log (LOGINFO, fmt::format ("SceneInformation"));
      cLog::log (LOGINFO, fmt::format ("scene_transition_type {}", scene_transition_type));
      cLog::log (LOGINFO, fmt::format ("scene_id {}", scene_id));
      if (scene_transition_type > 3 )
        cLog::log (LOGINFO, fmt::format ("second_scene_id {}", second_scene_id));
      }
    }
  //}}}

  //{{{
  void processFullFrameFreeze (sBitStream& bitStream, cDecoder264* decoder) {

    int full_frame_freeze_repetition_period = bitStream.readUeV ("SEI full_frame_freeze_repetition_period");
    cLog::log (LOGINFO, fmt::format ("full_frame_freeze_repetition_period:{}", full_frame_freeze_repetition_period));
    }
  //}}}
  //{{{
  void processFullFrameSnapshot (sBitStream& bitStream, cDecoder264* decoder) {

    int snapshot_id = bitStream.readUeV ("SEI snapshot_id");

    cLog::log (LOGINFO, fmt::format ("Full-frame snapshot"));
    cLog::log (LOGINFO, fmt::format ("snapshot_id:{}", snapshot_id));
    }
  //}}}
  //{{{
  void processFullFrameFreezeRelease (uint8_t* payload, int len, cDecoder264* decoder) {

    cLog::log (LOGINFO, fmt::format ("full-frame freeze release SEI"));
    if (len)
      cLog::log (LOGINFO, fmt::format ("payload len should be zero, but is {} bytes.", len));
    }
  //}}}

  //{{{
  void processProgressiveRefinementStart (sBitStream& bitStream, cDecoder264* decoder) {

    int progressive_refinement_id = bitStream.readUeV ("SEI progressive_refinement_id"  );
    int num_refinement_steps_minus1 = bitStream.readUeV ("SEI num_refinement_steps_minus1");

    cLog::log (LOGINFO, fmt::format ("Progressive refinement segment start"));
    cLog::log (LOGINFO, fmt::format ("progressive_refinement_id  :{}", progressive_refinement_id));
    cLog::log (LOGINFO, fmt::format ("num_refinement_steps_minus1:{}", num_refinement_steps_minus1));
    }
  //}}}
  //{{{
  void processProgressiveRefinementEnd (sBitStream& bitStream, cDecoder264* decoder) {

    int progressive_refinement_id = bitStream.readUeV ("SEI progressive_refinement_id"  );

    cLog::log (LOGINFO, fmt::format ("progressive refinement segment end"));
    cLog::log (LOGINFO, fmt::format ("progressive_refinement_id:{}", progressive_refinement_id));
    }
  //}}}

  //{{{
  void processMotionConstrainedSliceGroupSet (sBitStream& bitStream, cDecoder264* decoder) {

    int numSliceGroupsMinus1 = bitStream.readUeV ("SEI numSliceGroupsMinus1" );
    int sliceGroupSize = ceilLog2 (numSliceGroupsMinus1 + 1);

    cLog::log (LOGINFO, fmt::format ("Motion-constrained slice group set"));
    cLog::log (LOGINFO, fmt::format ("numSliceGroupsMinus1:{}", numSliceGroupsMinus1));

    for (int i = 0; i <= numSliceGroupsMinus1; i++) {
      int sliceGroupId = bitStream.readUv (sliceGroupSize, "SEI sliceGroupId" );
      cLog::log (LOGINFO, fmt::format ("sliceGroupId:{}", sliceGroupId));
      }

    int exact_matchFlag = bitStream.readU1 ("SEI exact_matchFlag"  );
    int pan_scan_rectFlag = bitStream.readU1 ("SEI pan_scan_rectFlag"  );

    cLog::log (LOGINFO, fmt::format ("exact_matchFlag:{}", exact_matchFlag));
    cLog::log (LOGINFO, fmt::format ("pan_scan_rectFlag:{}", pan_scan_rectFlag));

    if (pan_scan_rectFlag) {
      int pan_scan_rect_id = bitStream.readUeV ("SEI pan_scan_rect_id"  );
      cLog::log (LOGINFO, fmt::format ("pan_scan_rect_id:{}", pan_scan_rect_id));
      }
    }
  //}}}
  //{{{
  void processFilmGrain (sBitStream& bitStream, cDecoder264* decoder) {

    int model_id, separate_colour_description_presentFlag;
    int film_grain_bit_depth_luma_minus8, film_grain_bit_depth_chroma_minus8, film_grain_full_rangeFlag, film_grain_colour_primaries, film_grain_transfer_characteristics, film_grain_matrix_coefficients;
    int blending_mode_id, log2_scale_factor, comp_model_presentFlag[3];
    int num_intensity_intervals_minus1, num_model_values_minus1;
    int intensity_interval_lower_bound, intensity_interval_upper_bound;
    int comp_model_value;
    int film_grain_characteristics_repetition_period;

    int film_grain_characteristics_cancelFlag = bitStream.readU1 ("SEI film_grain_characteristics_cancelFlag");
    cLog::log (LOGINFO, fmt::format ("film_grain_characteristics_cancelFlag:{}", film_grain_characteristics_cancelFlag));
    if (!film_grain_characteristics_cancelFlag) {
      model_id = bitStream.readUv(2, "SEI model_id");
      separate_colour_description_presentFlag = bitStream.readU1 ("SEI separate_colour_description_presentFlag");
      cLog::log (LOGINFO, fmt::format ("model_id:{}", model_id));
      cLog::log (LOGINFO, fmt::format ("separate_colour_description_presentFlag:{}", separate_colour_description_presentFlag));

      if (separate_colour_description_presentFlag) {
        film_grain_bit_depth_luma_minus8 = bitStream.readUv (3, "SEI film_grain_bit_depth_luma_minus8");
        film_grain_bit_depth_chroma_minus8 = bitStream.readUv (3, "SEI film_grain_bit_depth_chroma_minus8");
        film_grain_full_rangeFlag = bitStream.readUv (1, "SEI film_grain_full_rangeFlag");
        film_grain_colour_primaries = bitStream.readUv (8, "SEI film_grain_colour_primaries");
        film_grain_transfer_characteristics = bitStream.readUv (8, "SEI film_grain_transfer_characteristics");
        film_grain_matrix_coefficients = bitStream.readUv (8, "SEI film_grain_matrix_coefficients");

        cLog::log (LOGINFO, fmt::format ("film_grain_bit_depth_luma_minus8:{}", film_grain_bit_depth_luma_minus8));
        cLog::log (LOGINFO, fmt::format ("film_grain_bit_depth_chroma_minus8:{}", film_grain_bit_depth_chroma_minus8));
        cLog::log (LOGINFO, fmt::format ("film_grain_full_rangeFlag:{}", film_grain_full_rangeFlag));
        cLog::log (LOGINFO, fmt::format ("film_grain_colour_primaries:{}", film_grain_colour_primaries));
        cLog::log (LOGINFO, fmt::format ("film_grain_transfer_characteristics:{}", film_grain_transfer_characteristics));
        cLog::log (LOGINFO, fmt::format ("film_grain_matrix_coefficients:{}", film_grain_matrix_coefficients));
        }

      blending_mode_id = bitStream.readUv (2, "SEI blending_mode_id");
      log2_scale_factor = bitStream.readUv (4, "SEI log2_scale_factor");
      cLog::log (LOGINFO, fmt::format ("blending_mode_id:{}", blending_mode_id));
      cLog::log (LOGINFO, fmt::format ("log2_scale_factor:{}", log2_scale_factor));
      for (int c = 0; c < 3; c ++) {
        comp_model_presentFlag[c] = bitStream.readU1 ("SEI comp_model_presentFlag");
        cLog::log (LOGINFO, fmt::format ("comp_model_presentFlag:{}", comp_model_presentFlag[c]));
        }

      for (int c = 0; c < 3; c ++)
        if (comp_model_presentFlag[c]) {
          num_intensity_intervals_minus1 = bitStream.readUv (8, "SEI num_intensity_intervals_minus1");
          num_model_values_minus1 = bitStream.readUv (3, "SEI num_model_values_minus1");
          cLog::log (LOGINFO, fmt::format ("num_intensity_intervals_minus1:{}", num_intensity_intervals_minus1));
          cLog::log (LOGINFO, fmt::format ("num_model_values_minus1:{}", num_model_values_minus1));
          for (int i = 0; i <= num_intensity_intervals_minus1; i++) {
            intensity_interval_lower_bound = bitStream.readUv (8, "SEI intensity_interval_lower_bound");
            intensity_interval_upper_bound = bitStream.readUv (8, "SEI intensity_interval_upper_bound");

            cLog::log (LOGINFO, fmt::format ("intensity_interval_lower_bound:{}", intensity_interval_lower_bound));
            cLog::log (LOGINFO, fmt::format ("intensity_interval_upper_bound:{}", intensity_interval_upper_bound));
            for (int j = 0; j <= num_model_values_minus1; j++) {
              comp_model_value = bitStream.readSeV ("SEI comp_model_value");
              cLog::log (LOGINFO, fmt::format ("comp_model_value:{}", comp_model_value));
              }
            }
          }
      film_grain_characteristics_repetition_period = bitStream.readUeV ("SEI film_grain_characteristics_repetition_period");
      cLog::log (LOGINFO, fmt::format ("film_grain_characteristics_repetition_period:{}",
                                       film_grain_characteristics_repetition_period));
      }
    }
  //}}}
  //{{{
  void processDeblockFilterDisplayPref (sBitStream& bitStream, cDecoder264* decoder) {

    int deblocking_display_preference_cancelFlag = bitStream.readU1("SEI deblocking_display_preference_cancelFlag");
    cLog::log (LOGINFO, fmt::format ("deblocking_display_preference_cancelFlag:{}", deblocking_display_preference_cancelFlag));

    if (!deblocking_display_preference_cancelFlag) {
      int display_prior_to_deblocking_preferredFlag = bitStream.readU1("SEI display_prior_to_deblocking_preferredFlag");
      int dec_frame_buffering_constraintFlag = bitStream.readU1("SEI dec_frame_buffering_constraintFlag");
      int deblocking_display_preference_repetition_period = bitStream.readUeV("SEI deblocking_display_preference_repetition_period");

      cLog::log (LOGINFO, fmt::format ("display_prior_to_deblocking_preferredFlag:{}", display_prior_to_deblocking_preferredFlag));
      cLog::log (LOGINFO, fmt::format ("dec_frame_buffering_constraintFlag:{}", dec_frame_buffering_constraintFlag));
      cLog::log (LOGINFO, fmt::format ("deblocking_display_preference_repetition_period:{}", deblocking_display_preference_repetition_period));
      }
    }
  //}}}
  //{{{
  void processStereoVideo (sBitStream& bitStream, cDecoder264* decoder) {

    int field_viewsFlags = bitStream.readU1 ("SEI field_viewsFlags");
    cLog::log (LOGINFO, fmt::format ("field_viewsFlags:{}", field_viewsFlags));

    if (field_viewsFlags) {
      int top_field_is_left_viewFlag = bitStream.readU1 ("SEI top_field_is_left_viewFlag");
      cLog::log (LOGINFO, fmt::format ("top_field_is_left_viewFlag:{}", top_field_is_left_viewFlag));
      }
    else {
      int current_frame_is_left_viewFlag = bitStream.readU1 ("SEI current_frame_is_left_viewFlag");
      int next_frame_is_second_viewFlag = bitStream.readU1 ("SEI next_frame_is_second_viewFlag");

      cLog::log (LOGINFO, fmt::format ("current_frame_is_left_viewFlag:{}", current_frame_is_left_viewFlag));
      cLog::log (LOGINFO, fmt::format ("next_frame_is_second_viewFlag:{}", next_frame_is_second_viewFlag));
      }

    int left_view_self_containedFlag = bitStream.readU1 ("SEI left_view_self_containedFlag");
    int right_view_self_containedFlag = bitStream.readU1 ("SEI right_view_self_containedFlag");

    cLog::log (LOGINFO, fmt::format ("left_view_self_containedFlag:{}", left_view_self_containedFlag));
    cLog::log (LOGINFO, fmt::format ("right_view_self_containedFlag:{}", right_view_self_containedFlag));
    }
  //}}}
  //{{{
  void processBufferingPeriod (sBitStream& bitStream, cDecoder264* decoder) {

    int spsId = bitStream.readUeV ("SEI spsId");
    cSps* sps = &decoder->sps[spsId];
    //useSps (decoder, sps);

    if (decoder->param.seiDebug)
      cLog::log (LOGINFO, fmt::format ("buffering"));

    // Note: NalHrdBpPresentFlag and CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
    if (sps->hasVui) {
      int initial_cpb_removal_delay;
      int initial_cpb_removal_delay_offset;
      if (sps->vuiSeqParams.nal_hrd_parameters_presentFlag) {
        for (uint32_t k = 0; k < sps->vuiSeqParams.nal_hrd_parameters.cpb_cnt_minus1+1; k++) {
          initial_cpb_removal_delay = bitStream.readUv(sps->vuiSeqParams.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay"        );
          initial_cpb_removal_delay_offset = bitStream.readUv(sps->vuiSeqParams.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay_offset" );
          if (decoder->param.seiDebug) {
            cLog::log (LOGINFO, fmt::format ("nal initial_cpb_removal_delay[{}]:{}", k, initial_cpb_removal_delay));
            cLog::log (LOGINFO, fmt::format ("nal initial_cpb_removal_delay_offset[{}]:{}", k, initial_cpb_removal_delay_offset));
            }
          }
        }

      if (sps->vuiSeqParams.vcl_hrd_parameters_presentFlag) {
        for (uint32_t k = 0; k < sps->vuiSeqParams.vcl_hrd_parameters.cpb_cnt_minus1+1; k++) {
          initial_cpb_removal_delay = bitStream.readUv(sps->vuiSeqParams.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay"        );
          initial_cpb_removal_delay_offset = bitStream.readUv(sps->vuiSeqParams.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1+1, "SEI initial_cpb_removal_delay_offset" );
          if (decoder->param.seiDebug) {
            cLog::log (LOGINFO, fmt::format ("vcl initial_cpb_removal_delay[{}]:{}", k, initial_cpb_removal_delay));
            cLog::log (LOGINFO, fmt::format ("vcl initial_cpb_removal_delay_offset[{}]:{}", k, initial_cpb_removal_delay_offset));
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void processFramePackingArrangement (sBitStream& bitStream, cDecoder264* decoder) {

    cLog::log (LOGINFO, fmt::format ("Frame packing arrangement"));

    sFramePackingArrangementInfo packing;
    packing.id = (uint32_t)bitStream.readUeV ("SEI framePacking id");
    packing.cancelFlag = bitStream.readU1 ("SEI framePacking cancelFlag");

    cLog::log (LOGINFO, fmt::format ("id:{}", packing.id));
    cLog::log (LOGINFO, fmt::format ("cancelFlag:{}", packing.cancelFlag));

    if (!packing.cancelFlag) {
      packing.type = (uint8_t)bitStream.readUv( 7, "SEI framePacking type");
      packing.quincunx_samplingFlag = bitStream.readU1 ("SEI framePacking quincunx_samplingFlag");
      packing.content_interpretation_type = (uint8_t)bitStream.readUv( 6, "SEI framePacking content_interpretation_type");
      packing.spatial_flippingFlag = bitStream.readU1 ("SEI framePacking spatial_flippingFlag");
      packing.frame0_flippedFlag = bitStream.readU1 ("SEI framePacking frame0_flippedFlag");
      packing.field_viewsFlag = bitStream.readU1 ("SEI framePacking field_viewsFlag");
      packing.current_frame_is_frame0Flag = bitStream.readU1 ("SEI framePacking current_frame_is_frame0Flag");
      packing.frame0_self_containedFlag = bitStream.readU1 ("SEI framePacking frame0_self_containedFlag");
      packing.frame1_self_containedFlag = bitStream.readU1 ("SEI framePacking frame1_self_containedFlag");

      cLog::log (LOGINFO, fmt::format ("type:{}", packing.type));
      cLog::log (LOGINFO, fmt::format ("quincunx_samplingFlag         :{}", packing.quincunx_samplingFlag));
      cLog::log (LOGINFO, fmt::format ("content_interpretation_type   :{}", packing.content_interpretation_type));
      cLog::log (LOGINFO, fmt::format ("spatial_flippingFlag          :{}", packing.spatial_flippingFlag));
      cLog::log (LOGINFO, fmt::format ("frame0_flippedFlag            :{}", packing.frame0_flippedFlag));
      cLog::log (LOGINFO, fmt::format ("field_viewsFlag               :{}", packing.field_viewsFlag));
      cLog::log (LOGINFO, fmt::format ("current_frame_is_frame0Flag   :{}", packing.current_frame_is_frame0Flag));
      cLog::log (LOGINFO, fmt::format ("frame0_self_containedFlag     :{}", packing.frame0_self_containedFlag));
      cLog::log (LOGINFO, fmt::format ("frame1_self_containedFlag     :{}", packing.frame1_self_containedFlag));

      if (!packing.quincunx_samplingFlag &&
          packing.type != 5 )  {
        packing.frame0_grid_position_x = (uint8_t)bitStream.readUv (4, "SEI framePacking frame0_grid_position_x");
        packing.frame0_grid_position_y = (uint8_t)bitStream.readUv (4, "SEI framePacking frame0_grid_position_y");
        packing.frame1_grid_position_x = (uint8_t)bitStream.readUv (4, "SEI framePacking frame1_grid_position_x");
        packing.frame1_grid_position_y = (uint8_t)bitStream.readUv (4, "SEI framePacking frame1_grid_position_y");

        cLog::log (LOGINFO, fmt::format ("frame0_grid_position_x:{}", packing.frame0_grid_position_x));
        cLog::log (LOGINFO, fmt::format ("frame0_grid_position_y:{}", packing.frame0_grid_position_y));
        cLog::log (LOGINFO, fmt::format ("frame1_grid_position_x:{}", packing.frame1_grid_position_x));
        cLog::log (LOGINFO, fmt::format ("frame1_grid_position_y:{}", packing.frame1_grid_position_y));
        }

      packing.reserved_byte = (uint8_t)bitStream.readUv (8, "SEI framePacking reserved_byte");
      packing.repetition_period = (uint32_t)bitStream.readUeV ("SEI framePacking repetition_period");
      cLog::log (LOGINFO, fmt::format ("reserved_byte:{}", packing.reserved_byte));
      cLog::log (LOGINFO, fmt::format ("repetition_period:{}", packing.repetition_period));
      }

    packing.extensionFlag = bitStream.readU1 ("SEI framePacking extensionFlag");
    cLog::log (LOGINFO, fmt::format ("extensionFlag:{}", packing.extensionFlag));
    }
  //}}}

  //{{{
  void processPostFilterHints (sBitStream& bitStream, cDecoder264* decoder) {

    uint32_t filter_hint_size_y = bitStream.readUeV ("SEI filter_hint_size_y"); // interpret post-filter hint SEI here
    uint32_t filter_hint_size_x = bitStream.readUeV ("SEI filter_hint_size_x"); // interpret post-filter hint SEI here
    uint32_t filter_hint_type = bitStream.readUv (2, "SEI filter_hint_type"); // interpret post-filter hint SEI here

    int*** filter_hint;
    getMem3Dint (&filter_hint, 3, filter_hint_size_y, filter_hint_size_x);

    for (uint32_t color_component = 0; color_component < 3; color_component++)
      for (uint32_t cy = 0; cy < filter_hint_size_y; cy++)
        for (uint32_t cx = 0; cx < filter_hint_size_x; cx++)
          filter_hint[color_component][cy][cx] = bitStream.readSeV ("SEI filter_hint"); // interpret post-filter hint SEI here

    uint32_t additional_extensionFlag = bitStream.readU1 ("SEI additional_extensionFlag"); // interpret post-filter hint SEI here

    cLog::log (LOGINFO, fmt::format ("Post-filter hint"));
    cLog::log (LOGINFO, fmt::format ("post_filter_hint_size_y {} ", filter_hint_size_y));
    cLog::log (LOGINFO, fmt::format ("post_filter_hint_size_x {} ", filter_hint_size_x));
    cLog::log (LOGINFO, fmt::format ("post_filter_hint_type {} ", filter_hint_type));
    for (uint32_t color_component = 0; color_component < 3; color_component ++)
      for (uint32_t cy = 0; cy < filter_hint_size_y; cy++)
        for (uint32_t cx = 0; cx < filter_hint_size_x; cx++)
          cLog::log (LOGINFO, fmt::format (" post_filter_hint[{}][{}][{}] {} ", color_component, cy, cx, filter_hint[color_component][cy][cx]));
    cLog::log (LOGINFO, fmt::format ("additional_extensionFlag {} ", additional_extensionFlag));

    freeMem3Dint (filter_hint);
    }
  //}}}
  //{{{
  void processGreenMetadata (sBitStream& bitStream, cDecoder264* decoder) {

    cLog::log (LOGINFO, fmt::format ("GreenMetadataInfo"));

    sGreenMetadataInfo metadata;
    metadata.type = (uint8_t)bitStream.readUv (8, "SEI greenMetadata_type");
    cLog::log (LOGINFO, fmt::format ("greenMetadata type:{}", metadata.type));

    if (metadata.type == 0) {
        metadata.period_type = (uint8_t)bitStream.readUv (8, "SEI green_metadata_period_type");
      cLog::log (LOGINFO, fmt::format ("green_metadata_period_type:{}", metadata.period_type));

      if (metadata.period_type == 2) {
        metadata.num_seconds = (uint16_t)bitStream.readUv (16, "SEI green_metadata_num_seconds");
        cLog::log (LOGINFO, fmt::format ("green_metadata_num_seconds:{}", metadata.num_seconds));
        }
      else if (metadata.period_type == 3) {
        metadata.num_pictures = (uint16_t)bitStream.readUv (16, "SEI green_metadata_num_pictures");
        cLog::log (LOGINFO, fmt::format ("green_metadata_num_pictures:{}", metadata.num_pictures));
        }

      metadata.percent_non_zero_macroBlocks = (uint8_t)bitStream.readUv (8, "SEI percent_non_zero_macroBlocks");
      metadata.percent_intra_coded_macroBlocks = (uint8_t)bitStream.readUv (8, "SEI percent_intra_coded_macroBlocks");
      metadata.percent_six_tap_filtering = (uint8_t)bitStream.readUv (8, "SEI percent_six_tap_filtering");
      metadata.percent_alpha_point_deblocking_instance = (uint8_t)bitStream.readUv (8, "SEI percent_alpha_point_deblocking_instance");

      cLog::log (LOGINFO, fmt::format ("percent_non_zero_macroBlocks {}", (float)metadata.percent_non_zero_macroBlocks/255));
      cLog::log (LOGINFO, fmt::format ("percent_intra_coded_macroBlocks {}", (float)metadata.percent_intra_coded_macroBlocks/255));
      cLog::log (LOGINFO, fmt::format ("percent_six_tap_filtering {}", (float)metadata.percent_six_tap_filtering/255));
      cLog::log (LOGINFO, fmt::format ("percent_alpha_point_deblocking_instance {}", (float)metadata.percent_alpha_point_deblocking_instance/255));
      }

    else if (metadata.type == 1) {
      metadata.xsd_metric_type = (uint8_t)bitStream.readUv (8, "SEI: xsd_metric_type");
      metadata.xsd_metric_value = (uint16_t)bitStream.readUv (16, "SEI xsd_metric_value");
      cLog::log (LOGINFO, fmt::format ("xsd_metric_type:{}", metadata.xsd_metric_type));
      if (metadata.xsd_metric_type == 0)
        cLog::log (LOGINFO, fmt::format ("xsd_metric_value:{}", (float)metadata.xsd_metric_value/100));
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

    // simple bitStream, .mBuffer points into nalu, no alloc
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
        processFiller (naluPayload + offset, payloadSize, decoder); break;
      case SEI_USER_DATA_REGISTERED_ITU_T_T35:
        processUserDataT35 (naluPayload + offset, payloadSize, decoder); break;
      case SEI_USER_DATA_UNREGISTERED:
        processUserDataUnregistered (naluPayload + offset, payloadSize, decoder); break;
      case SEI_RECOVERY_POINT:
        processRecoveryPoint (bitStream, decoder); break;
      case SEI_DEC_REF_PIC_MARKING_REPETITION:
        processDecRefPicMarkingRepetition (bitStream, decoder, slice); break;
      case SEI_SPARE_PIC:
        processSpare (bitStream, decoder); break;
      case SEI_SCENE_INFO:
        processScene (bitStream, decoder); break;
      case SEI_SUB_SEQ_INFO:
        processSubsequence (bitStream, decoder); break;
      case SEI_SUB_SEQ_LAYER_CHARACTERISTICS:
        processSubsequenceLayerCharacteristics (bitStream, decoder); break;
      case SEI_SUB_SEQ_CHARACTERISTICS:
        processSubsequenceCharacteristics (bitStream, decoder); break;
      case SEI_FULL_FRAME_FREEZE:
        processFullFrameFreeze (bitStream, decoder); break;
      case SEI_FULL_FRAME_FREEZE_RELEASE:
        processFullFrameFreezeRelease (naluPayload + offset, payloadSize, decoder); break;
      case SEI_FULL_FRAME_SNAPSHOT:
        processFullFrameSnapshot (bitStream, decoder); break;
      case SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START:
        processProgressiveRefinementStart (bitStream, decoder); break;
      case SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END:
        processProgressiveRefinementEnd (bitStream, decoder); break;
      case SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET:
        processMotionConstrainedSliceGroupSet (bitStream, decoder); break;
      case SEI_FILM_GRAIN_CHARACTERISTICS:
        processFilmGrain (bitStream, decoder); break;
      case SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE:
        processDeblockFilterDisplayPref (bitStream, decoder); break;
      case SEI_STEREO_VIDEO_INFO:
        processStereoVideo  (bitStream, decoder); break;
      case SEI_POST_FILTER_HINTS:
        processPostFilterHints (bitStream, decoder); break;
      case SEI_FRAME_PACKING_ARRANGEMENT:
        processFramePackingArrangement (bitStream, decoder); break;
      case SEI_GREEN_METADATA:
        processGreenMetadata (bitStream, decoder); break;
      default:
        processReserved (naluPayload + offset, payloadSize, decoder); break;
      }
    offset += payloadSize;
    } while (naluPayload[offset] != 0x80);

  // ignore the trailing bits rbsp_trailing_bits();
  }
//}}}
