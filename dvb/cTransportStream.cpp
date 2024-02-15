// cTransportStream.cpp - minimal mpeg transportStream demux for freeview streams
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
#else
  #include <unistd.h>
  #include <sys/poll.h>
#endif

#include "cTransportStream.h"

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <thread>

#include "../date/include/date/date.h"
#include "../common/utils.h"
#include "../common/cDvbUtils.h"
#include "../common/cLog.h"

#include "../decoders/cVideoRender.h"
#include "../decoders/cAudioRender.h"
#include "../decoders/cSubtitleRender.h"

using namespace std;
//}}}
//{{{  defines, const, struct
constexpr uint8_t kPacketSize = 188;
const int kInitBufSize = 512;

// pids
#define PID_PAT   0x00   /* Program Association Table */
#define PID_CAT   0x01   /* Conditional Access Table */
#define PID_NIT   0x10   /* Network Information Table */
#define PID_SDT   0x11   /* Service Description Table */
#define PID_EIT   0x12   /* Event Information Table */
#define PID_RST   0x13   /* Running Status Table */
#define PID_TDT   0x14   /* Time Date Table */
#define PID_SYN   0x15   /* Network sync */

//{{{
struct sPat {
  uint8_t table_id                 :8;

  uint8_t section_length_hi        :4;
  uint8_t dummy1                   :2;
  uint8_t dummy                    :1;
  uint8_t section_syntax_indicator :1;

  uint8_t section_length_lo        :8;

  uint8_t transport_stream_id_hi   :8;
  uint8_t transport_stream_id_lo   :8;

  uint8_t current_next_indicator   :1;
  uint8_t version_number           :5;
  uint8_t dummy2                   :2;

  uint8_t section_number           :8;
  uint8_t last_section_number      :8;
  } ;
//}}}
//{{{
struct sPatProg {
  uint8_t program_number_hi :8;
  uint8_t program_number_lo :8;

  uint8_t network_pid_hi    :5;
  uint8_t                   :3;
  uint8_t network_pid_lo    :8;
  /* or program_map_pid (if prog_num=0)*/
  } ;
//}}}

//{{{
struct sSdt {
  uint8_t table_id                 :8;
  uint8_t section_length_hi        :4;
  uint8_t                          :3;
  uint8_t section_syntax_indicator :1;
  uint8_t section_length_lo        :8;
  uint8_t transport_stream_id_hi   :8;
  uint8_t transport_stream_id_lo   :8;
  uint8_t current_next_indicator   :1;
  uint8_t version_number           :5;
  uint8_t                          :2;
  uint8_t section_number           :8;
  uint8_t last_section_number      :8;
  uint8_t original_network_id_hi   :8;
  uint8_t original_network_id_lo   :8;
  uint8_t                          :8;
  } ;
//}}}
//{{{
struct sSdtDescriptor {
  uint8_t service_id_hi              :8;
  uint8_t service_id_lo              :8;
  uint8_t eit_present_following_flag :1;
  uint8_t eit_schedule_flag          :1;
  uint8_t                            :6;
  uint8_t descrs_loop_length_hi      :4;
  uint8_t free_ca_mode               :1;
  uint8_t running_status             :3;
  uint8_t descrs_loop_length_lo      :8;
  } ;
//}}}

//{{{  tids
#define TID_PAT          0x00   /* Program Association Section */
#define TID_CAT          0x01   /* Conditional Access Section */
#define TID_PMT          0x02   /* Conditional Access Section */

#define TID_EIT          0x12   /* Event Information Section */

#define TID_NIT_ACT      0x40   /* Network Information Section - actual */
#define TID_NIT_OTH      0x41   /* Network Information Section - other */
#define TID_SDT_ACT      0x42   /* Service Description Section - actual */
#define TID_SDT_OTH      0x46   /* Service Description Section - other */
#define TID_BAT          0x4A   /* Bouquet Association Section */

#define TID_EIT_ACT      0x4E   /* Event Information Section - actual */
#define TID_EIT_OTH      0x4F   /* Event Information Section - other */
#define TID_EIT_ACT_SCH  0x50   /* Event Information Section - actual, schedule  */
#define TID_EIT_OTH_SCH  0x60   /* Event Information Section - other, schedule */

#define TID_TDT          0x70   /* Time Date Section */
#define TID_RST          0x71   /* Running Status Section */
#define TID_ST           0x72   /* Stuffing Section */
#define TID_TOT          0x73   /* Time Offset Section */
//}}}
//{{{
struct sEit {
  uint8_t table_id                    :8;

  uint8_t section_length_hi           :4;
  uint8_t                             :3;
  uint8_t section_syntax_indicator    :1;
  uint8_t section_length_lo           :8;

  uint8_t service_id_hi               :8;
  uint8_t service_id_lo               :8;
  uint8_t current_next_indicator      :1;
  uint8_t version_number              :5;
  uint8_t                             :2;
  uint8_t section_number              :8;
  uint8_t last_section_number         :8;
  uint8_t transport_stream_id_hi      :8;
  uint8_t transport_stream_id_lo      :8;
  uint8_t original_network_id_hi      :8;
  uint8_t original_network_id_lo      :8;
  uint8_t segment_last_section_number :8;
  uint8_t segment_last_table_id       :8;
  } ;
//}}}
//{{{
struct sEitEvent {
  uint8_t event_id_hi           :8;
  uint8_t event_id_lo           :8;
  uint8_t mjd_hi                :8;
  uint8_t mjd_lo                :8;
  uint8_t start_time_h          :8;
  uint8_t start_time_m          :8;
  uint8_t start_time_s          :8;
  uint8_t duration_h            :8;
  uint8_t duration_m            :8;
  uint8_t duration_s            :8;
  uint8_t descrs_loop_length_hi :4;
  uint8_t free_ca_mode          :1;
  uint8_t running_status        :3;
  uint8_t descrs_loop_length_lo :8;
  } ;
//}}}

//{{{
struct sTdt {
  uint8_t table_id                 :8;
  uint8_t section_length_hi        :4;
  uint8_t                          :3;
  uint8_t section_syntax_indicator :1;
  uint8_t section_length_lo        :8;
  uint8_t utc_mjd_hi               :8;
  uint8_t utc_mjd_lo               :8;
  uint8_t utc_time_h               :8;
  uint8_t utc_time_m               :8;
  uint8_t utc_time_s               :8;
  } ;
//}}}

//{{{
struct sPmt {
   unsigned char table_id           :8;

   uint8_t section_length_hi        :4;
   uint8_t                          :2;
   uint8_t dummy                    :1; // has to be 0
   uint8_t section_syntax_indicator :1;
   uint8_t section_length_lo        :8;

   uint8_t program_number_hi        :8;
   uint8_t program_number_lo        :8;
   uint8_t current_next_indicator   :1;
   uint8_t version_number           :5;
   uint8_t                          :2;
   uint8_t section_number           :8;
   uint8_t last_section_number      :8;
   uint8_t PCR_PID_hi               :5;
   uint8_t                          :3;
   uint8_t PCR_PID_lo               :8;
   uint8_t program_info_length_hi   :4;
   uint8_t                          :4;
   uint8_t program_info_length_lo   :8;
   //descrs
  } ;
//}}}
//{{{
struct sPmtInfo {
   uint8_t stream_type       :8;
   uint8_t elementary_PID_hi :5;
   uint8_t                   :3;
   uint8_t elementary_PID_lo :8;
   uint8_t ES_info_length_hi :4;
   uint8_t                   :4;
   uint8_t ES_info_length_lo :8;
   // descrs
  } ;
//}}}

//{{{  descr defines
#define DESCR_VIDEO_STREAM          0x02
#define DESCR_AUDIO_STREAM          0x03
#define DESCR_HIERARCHY             0x04
#define DESCR_REGISTRATION          0x05
#define DESCR_DATA_STREAM_ALIGN     0x06
#define DESCR_TARGET_BACKGRID       0x07
#define DESCR_VIDEO_WINDOW          0x08
#define DESCR_CA                    0x09
#define DESCR_ISO_639_LANGUAGE      0x0A
#define DESCR_SYSTEM_CLOCK          0x0B
#define DESCR_MULTIPLEX_BUFFER_UTIL 0x0C
#define DESCR_COPYRIGHT             0x0D
#define DESCR_MAXIMUM_BITRATE       0x0E
#define DESCR_PRIVATE_DATA_IND      0x0F

#define DESCR_SMOOTHING_BUFFER      0x10
#define DESCR_STD                   0x11
#define DESCR_IBP                   0x12

#define DESCR_NW_NAME               0x40
#define DESCR_SERVICE_LIST          0x41
#define DESCR_STUFFING              0x42
#define DESCR_SAT_DEL_SYS           0x43
#define DESCR_CABLE_DEL_SYS         0x44
#define DESCR_VBI_DATA              0x45
#define DESCR_VBI_TELETEXT          0x46
#define DESCR_BOUQUET_NAME          0x47
#define DESCR_SERVICE               0x48
#define DESCR_COUNTRY_AVAIL         0x49
#define DESCR_LINKAGE               0x4A
#define DESCR_NVOD_REF              0x4B
#define DESCR_TIME_SHIFTED_SERVICE  0x4C
#define DESCR_SHORT_EVENT           0x4D
#define DESCR_EXT_EVENT             0x4E
#define DESCR_TIME_SHIFTED_EVENT    0x4F

#define DESCR_COMPONENT             0x50
#define DESCR_MOSAIC                0x51
#define DESCR_STREAM_ID             0x52
#define DESCR_CA_IDENT              0x53
#define DESCR_CONTENT               0x54
#define DESCR_PARENTAL_RATING       0x55
#define DESCR_TELETEXT              0x56
#define DESCR_TELEPHONE             0x57
#define DESCR_LOCAL_TIME_OFF        0x58
#define DESCR_SUBTITLING            0x59
#define DESCR_TERR_DEL_SYS          0x5A
#define DESCR_ML_NW_NAME            0x5B
#define DESCR_ML_BQ_NAME            0x5C
#define DESCR_ML_SERVICE_NAME       0x5D
#define DESCR_ML_COMPONENT          0x5E
#define DESCR_PRIV_DATA             0x5F

#define DESCR_SERVICE_MOVE          0x60
#define DESCR_SHORT_SMOOTH_BUF      0x61
#define DESCR_FREQUENCY_LIST        0x62
#define DESCR_PARTIAL_TP_STREAM     0x63
#define DESCR_DATA_BROADCAST        0x64
#define DESCR_CA_SYSTEM             0x65
#define DESCR_DATA_BROADCAST_ID     0x66
#define DESCR_TRANSPORT_STREAM      0x67
#define DESCR_DSNG                  0x68
#define DESCR_PDC                   0x69
#define DESCR_AC3                   0x6A
#define DESCR_ANCILLARY_DATA        0x6B
#define DESCR_CELL_LIST             0x6C
#define DESCR_CELL_FREQ_LINK        0x6D
#define DESCR_ANNOUNCEMENT_SUPPORT  0x6E

#define DESCR_CONTENT_ID            0x76
//}}}
//{{{
struct sDescriptorGen {
  uint8_t descr_tag    :8;
  uint8_t descr_length :8;
  };

#define getDescrTag(x) (((sDescriptorGen*)x)->descr_tag)
#define getDescrLength(x) (((sDescriptorGen*)x)->descr_length+2)
//}}}
//{{{
struct descrService {
  uint8_t descr_tag            :8;
  uint8_t descr_length         :8;
  uint8_t service_type         :8;
  uint8_t provider_name_length :8;
  } ;
//}}}
//{{{
struct descrShortEvent {
  uint8_t descr_tag         :8;
  uint8_t descr_length      :8;
  uint8_t lang_code1        :8;
  uint8_t lang_code2        :8;
  uint8_t lang_code3        :8;
  uint8_t event_name_length :8;
  } ;
//}}}
//{{{
struct descrExtendedEvent {
  uint8_t descr_tag         :8;
  uint8_t descr_length      :8;
  /* TBD */
  uint8_t last_descr_number :4;
  uint8_t descr_number      :4;
  uint8_t lang_code1        :8;
  uint8_t lang_code2        :8;
  uint8_t lang_code3        :8;
  uint8_t length_of_items   :8;
  } ;

#define CastExtendedEventDescr(x) ((descrExtendedEvent*)(x))
//}}}
//{{{
struct itemExtendedEvent {
  uint8_t item_description_length:8;
  } ;

#define CastExtendedEventItem(x) ((itemExtendedEvent*)(x))
//}}}

#define HILO(x) (x##_hi << 8 | x##_lo)

#define mjdToEpochTime(x) (unsigned int)((((x##_hi << 8) | x##_lo) - 40587) * 86400)

#define bcdTimeToSeconds(x) ((3600 * ((10*((x##_h & 0xF0)>>4)) + (x##_h & 0xF))) + \
                               (60 * ((10*((x##_m & 0xF0)>>4)) + (x##_m & 0xF))) + \
                                     ((10*((x##_s & 0xF0)>>4)) + (x##_s & 0xF)))
//}}}

//{{{  class cTransportStream::cPidInfo
//{{{
string cTransportStream::cPidInfo::getPidName() const {

  // known pids
  switch (mPid) {
    case PID_PAT: return "PAT";
    case PID_CAT: return "CAT";
    case PID_NIT: return "NIT";
    case PID_SDT: return "SDT";
    case PID_EIT: return "EIT";
    case PID_RST: return "RST";
    case PID_TDT: return "TDT";
    case PID_SYN: return "SYN";
    }

  if (mSid)
    return cDvbUtils::getStreamTypeName (mStreamTypeId);

  // unknown pid
  return "---";
  }
//}}}

//{{{
int cTransportStream::cPidInfo::addToBuffer (uint8_t* buf, int bufSize) {

  if (getBufSize() + bufSize > mBufSize) {
    // realloc buffer to twice size
    mBufSize = mBufSize * 2;
    cLog::log (LOGINFO1, fmt::format ("demux pid:{} realloc {}", mPid,mBufSize));

    int ptrOffset = getBufSize();
    mBuffer = (uint8_t*)realloc (mBuffer, mBufSize);
    mBufPtr = mBuffer + ptrOffset;
    }

  memcpy (mBufPtr, buf, bufSize);
  mBufPtr += bufSize;

  return getBufSize();
  }
//}}}
//}}}
//{{{  class cTransportStream::cService
//{{{
cTransportStream::cService::cService (uint16_t sid, iOptions* options) : mSid(sid), mOptions(options) {

  mStreams[cRenderStream::eVideo].setLabel ("vid:");
  mStreams[cRenderStream::eAudio].setLabel ("aud:");
  mStreams[cRenderStream::eDescription].setLabel ("des:");
  mStreams[cRenderStream::eSubtitle].setLabel ("sub:");
  }
//}}}
//{{{
cTransportStream::cService::~cService() {

  delete mNowEpgItem;

  for (auto& epgItem : mTodayEpg)
    delete epgItem.second;
  mTodayEpg.clear();

  for (auto& epgItem : mOtherEpg)
    delete epgItem.second;
  mOtherEpg.clear();

  closeFile();
  }
//}}}

//{{{
int64_t cTransportStream::cService::getPtsFromStart() {
// service pts is audio playPts

  cRenderStream& audioStream = getStream (cRenderStream::eAudio);
  if (audioStream.isEnabled())
    return audioStream.getRender().getPtsFromStart();

  return 0;
  }
//}}}

// streams
//{{{
cRenderStream* cTransportStream::cService::getStreamByPid (uint16_t pid) {

  for (uint8_t streamType = cRenderStream::eVideo; streamType <= cRenderStream::eSubtitle; streamType++) {
    cRenderStream& stream = getStream (cRenderStream::eType(streamType));
    if (stream.isDefined() && (stream.getPid() == pid))
      return &mStreams[streamType];
    }

  return nullptr;
  }
//}}}
//{{{
void cTransportStream::cService::enableStream (cRenderStream::eType streamType) {

  switch (streamType) {
    case cRenderStream::eVideo: {
      cRenderStream& stream = getStream (cRenderStream::eVideo);
      stream.setRender (new cVideoRender (true, 50, mName, stream.getTypeId(), stream.getPid()));
      break;
      }

    case cRenderStream::eAudio: {
      cRenderStream& stream = getStream (cRenderStream::eAudio);
      stream.setRender (new cAudioRender (true, 48, true, true,
                                          mName, stream.getTypeId(), stream.getPid()));
      break;
      }

    case cRenderStream::eSubtitle: {
      cRenderStream& stream = getStream (cRenderStream::eSubtitle);
      stream.setRender (new cSubtitleRender (true, 1,
                                             mName, stream.getTypeId(), stream.getPid()));
      break;
      }

    default: break;
    }
  }
//}}}
//{{{
bool cTransportStream::cService::throttle() {
// return true if audioStream needs throttle

  cRenderStream& audioStream = getStream (cRenderStream::eAudio);
  return audioStream.isEnabled() && audioStream.getRender().throttle();
  }
//}}}

// record program
//{{{
void cTransportStream::cService::start (chrono::system_clock::time_point tdt,
                                        const string& programName,
                                        chrono::system_clock::time_point programStartTime,
                                        bool selected) {
// start recording program

  // close prev program
  closeFile();

  if (getStream(cRenderStream::eAudio).isDefined() &&
      getStream (cRenderStream::eVideo).isDefined() &&
      (selected || getRecord() || ((dynamic_cast<cOptions*>(mOptions))->mRecordAllServices))) {
    string filePath = (dynamic_cast<cOptions*>(mOptions))->mRecordRoot +
                      getRecordName() + date::format ("%d %b %y %a %H.%M.%S ",
                        date::floor<chrono::seconds>(tdt)) + utils::getValidFileString (programName) + ".ts";

    // open new record program
    openFile (filePath, 0x1234);

    cLog::log (LOGINFO, fmt::format ("{} {}",
      date::format ("%H.%M.%S %a %d %b %y", date::floor<chrono::seconds>(programStartTime)),
      filePath));
    }
  }
//}}}
//{{{
void cTransportStream::cService::pesPacket (uint16_t pid, uint8_t* ts) {
//  ts pes packet, write only video,audio,subtitle pids to file

  if (mRecording && mFile &&
      ((pid == mStreams[cRenderStream::eVideo].getPid()) ||
       (pid == mStreams[cRenderStream::eAudio].getPid()) ||
       (pid == mStreams[cRenderStream::eSubtitle].getPid())))
    fwrite (ts, 1, kPacketSize, mFile);
  }
//}}}

// epg
//{{{
bool cTransportStream::cService::isEpgRecord (const string& title, chrono::system_clock::time_point startTime) {
// return true if startTime, title selected to record in epg

  auto it = mTodayEpg.find (startTime);
  if (it != mTodayEpg.end())
    if (title == it->second->getTitle())
      if (it->second->getRecord())
        return true;

  it = mOtherEpg.find (startTime);
  if (it != mOtherEpg.end())
    if (title == it->second->getTitle())
      if (it->second->getRecord())
        return true;

  return false;
  }
//}}}
//{{{
bool cTransportStream::cService::setNow (bool record,
                                         chrono::system_clock::time_point startTime,
                                         chrono::seconds duration,
                                         const string& title,
                                         const string& info) {
// return true if new now epgItem

  if (mNowEpgItem && (mNowEpgItem->getStartTime() == startTime))
    return false;

  delete mNowEpgItem;

  mNowEpgItem = new cEpgItem (true, record, startTime, duration, title, info);
  return true;
  }
//}}}
//{{{
void cTransportStream::cService::setEpg (bool record,
                                         chrono::system_clock::time_point nowTime,
                                         chrono::system_clock::time_point startTime,
                                         chrono::seconds duration,
                                         const string& title,
                                         const string& info) {

  auto nowDay = date::floor<date::days>(nowTime);
  auto startDay = date::floor<date::days>(startTime);
  if (startDay == nowDay) {
    // today
    auto it = mTodayEpg.find (startTime);
    if (it == mTodayEpg.end())
      mTodayEpg.emplace (startTime, new cEpgItem (false, record, startTime, duration, title, info));
    else
      it->second->set (startTime, duration, title, info);
    }
  else {
    // other
    auto it = mOtherEpg.find (startTime);
    if (it == mOtherEpg.end())
      mOtherEpg.emplace (startTime, new cEpgItem (false, record, startTime, duration, title, info));
    else
      it->second->set (startTime, duration, title, info);
    }
  }
//}}}

// private statics
//{{{
uint8_t* cTransportStream::cService::tsHeader (uint8_t* ts, uint16_t pid, uint8_t continuityCount) {

  memset (ts, 0xFF, kPacketSize);

  *ts++ = 0x47;                         // sync byte
  *ts++ = 0x40 | ((pid & 0x1f00) >> 8); // payload_unit_start_indicator + pid upper
  *ts++ = pid & 0xff;                   // pid lower
  *ts++ = 0x10 | continuityCount;       // no adaptControls + cont count
  *ts++ = 0;                            // pointerField

  return ts;
  }
//}}}
//{{{
void cTransportStream::cService::writeSection (FILE* file, uint8_t* ts, uint8_t* tsSectionStart, uint8_t* tsPtr) {

  // tsSection crc, calc from tsSection start to here
  uint32_t crc = cDvbUtils::getCrc32 (tsSectionStart, int(tsPtr - tsSectionStart));
  *tsPtr++ = (crc & 0xff000000) >> 24;
  *tsPtr++ = static_cast<uint8_t>((crc & 0x00ff0000) >> 16);
  *tsPtr++ = (crc & 0x0000ff00) >>  8;
  *tsPtr++ =  crc & 0x000000ff;

  fwrite (ts, 1, kPacketSize, file);
  }
//}}}

// private
//{{{
bool cTransportStream::cService::openFile (const string& fileName, uint16_t tsid) {

  mFile = fopen (fileName.c_str(), "wb");
  if (mFile) {
    writePat (tsid);
    writePmt();
    mRecording = true;
    return true;
    }

  cLog::log (LOGERROR, "cService::openFile " + fileName);
  return false;
  }
//}}}
//{{{
void cTransportStream::cService::writePat (uint16_t tsid) {

  uint8_t ts[kPacketSize];
  uint8_t* tsSectionStart = tsHeader (ts, PID_PAT, 0);
  uint8_t* tsPtr = tsSectionStart;

  uint32_t sectionLength = 5+4 + 4;
  *tsPtr++ = TID_PAT;                                // PAT tid
  *tsPtr++ = 0xb0 | ((sectionLength & 0x0F00) >> 8); // PAT sectionLength upper
  *tsPtr++ = sectionLength & 0x0FF;                  // PAT sectionLength lower

  // sectionLength from here to end of crc
  *tsPtr++ = (tsid & 0xFF00) >> 8;                   // transportStreamId
  *tsPtr++ =  tsid & 0x00FF;
  *tsPtr++ = 0xc1;                                   // for simplicity, we'll have a version_id of 0
  *tsPtr++ = 0x00;                                   // first section number = 0
  *tsPtr++ = 0x00;                                   // last section number = 0

  *tsPtr++ = (mSid & 0xFF00) >> 8;                   // first section serviceId
  *tsPtr++ =  mSid & 0x00FF;
  *tsPtr++ = 0xE0 | ((mProgramPid & 0x1F00) >> 8);   // first section pgmPid
  *tsPtr++ = mProgramPid & 0x00FF;

  writeSection (mFile, ts, tsSectionStart, tsPtr);
  }
//}}}
//{{{
void cTransportStream::cService::writePmt() {

  uint8_t ts[kPacketSize];
  uint8_t* tsSectionStart = tsHeader (ts, mProgramPid, 0);
  uint8_t* tsPtr = tsSectionStart;

  int sectionLength = 28; // 5+4 + program_info_length + esStreams * (5 + ES_info_length) + 4
  *tsPtr++ = TID_PMT;
  *tsPtr++ = 0xb0 | ((sectionLength & 0x0F00) >> 8);
  *tsPtr++ = sectionLength & 0x0FF;

  // sectionLength from here to end of crc
  *tsPtr++ = (mSid & 0xFF00) >> 8;
  *tsPtr++ = mSid & 0x00FF;
  *tsPtr++ = 0xc1; // version_id of 0
  *tsPtr++ = 0x00; // first section number = 0
  *tsPtr++ = 0x00; // last section number = 0

  *tsPtr++ = 0xE0 | ((mStreams[cRenderStream::eVideo].getPid() & 0x1F00) >> 8);
  *tsPtr++ = mStreams[cRenderStream::eVideo].getPid() & 0x00FF;

  *tsPtr++ = 0xF0;
  *tsPtr++ = 0; // program_info_length;

  // video es
  *tsPtr++ = mStreams[cRenderStream::eVideo].getTypeId(); // elementary stream_type
  *tsPtr++ = 0xE0 | ((mStreams[cRenderStream::eVideo].getPid() & 0x1F00) >> 8); // elementary_PID
  *tsPtr++ = mStreams[cRenderStream::eVideo].getPid() & 0x00FF;
  *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0; // ES_info_length
  *tsPtr++ = 0 & 0x00FF;

  // audio es
  *tsPtr++ = mStreams[cRenderStream::eAudio].getTypeId(); // elementary stream_type
  *tsPtr++ = 0xE0 | ((mStreams[cRenderStream::eAudio].getPid() & 0x1F00) >> 8); // elementary_PID
  *tsPtr++ = mStreams[cRenderStream::eAudio].getPid() & 0x00FF;
  *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0; // ES_info_length
  *tsPtr++ = 0 & 0x00FF;

  // subtitle es
  *tsPtr++ = mStreams[cRenderStream::eSubtitle].getTypeId(); // elementary stream_type
  *tsPtr++ = 0xE0 | ((mStreams[cRenderStream::eSubtitle].getPid() & 0x1F00) >> 8); // elementary_PID
  *tsPtr++ = mStreams[cRenderStream::eSubtitle].getPid() & 0x00FF;
  *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0; // ES_info_length
  *tsPtr++ = 0 & 0x00FF;

  writeSection (mFile, ts, tsSectionStart, tsPtr);
  }
//}}}
//{{{
void cTransportStream::cService::closeFile() {

  if (mFile)
    fclose (mFile);

  mFile = nullptr;
  mRecording = false;
  }

//}}}
//}}}

// cTransportStream
//{{{
cTransportStream::cTransportStream (const cDvbMultiplex& dvbMultiplex, iOptions* options,
                                    const function<void (cService& service)> addServiceCallback,
                                    const function<void (cService& service, cPidInfo& pidInfo)> pesCallback) :
    mDvbMultiplex(dvbMultiplex),
    mOptions(options),
    mAddServiceCallback(addServiceCallback),
    mPesCallback(pesCallback) {}
//}}}

// gets
//{{{
string cTransportStream::getTdtString() const {
  return date::format ("%T", date::floor<chrono::seconds>(mNowTdt));
  }
//}}}

// actions
//{{{
bool cTransportStream::throttle() {
// return true if any service needs throttle

  for (auto& service : mServiceMap)
    if (service.second.throttle())
      return true;

  return false;
  }
//}}}
// demux
//{{{
int64_t cTransportStream::demux (uint8_t* chunk, int64_t chunkSize) {
// demux from chunk to chunk + chunkSize

  uint8_t* ts = chunk;
  uint8_t* tsEnd = chunk + chunkSize;

  uint8_t* nextPacket = ts + kPacketSize;
  while (nextPacket <= tsEnd) {
    if (*ts == 0x47) {
      if ((ts+kPacketSize >= tsEnd) || (*(ts+kPacketSize) == 0x47)) {
        ts++;
        int tsBytesLeft = kPacketSize-1;

        uint16_t pid = ((ts[0] & 0x1F) << 8) | ts[1];
        if (pid != 0x1FFF) {
          bool payloadStart =   ts[0] & 0x40;
          int continuityCount = ts[2] & 0x0F;
          int headerBytes =    (ts[2] & 0x20) ? (4 + ts[3]) : 3; // adaption field

          lock_guard<mutex> lockGuard (mPidInfoMutex);
          cPidInfo* pidInfo = getPsiPidInfo (pid);
          if (pidInfo) {
            if ((pidInfo->mContinuity >= 0) &&
                (continuityCount != ((pidInfo->mContinuity+1) & 0x0F))) {
              //{{{  continuity error, abandon multi packet buffering
              if (pidInfo->mContinuity == continuityCount) // strange case of bbc subtitles
                pidInfo->mRepeatContinuity++;
              else {
                mNumErrors++;
                // abandon any buffered pes or section
                pidInfo->mBufPtr = nullptr;
                }
              }
              //}}}
            pidInfo->mContinuity = continuityCount;
            pidInfo->mPackets++;

            if (pidInfo->isPsi()) {
              //{{{  psi
              ts += headerBytes;
              tsBytesLeft -= headerBytes;

              if (payloadStart) {
                // handle the very odd pointerField
                uint8_t pointerField = *ts++;
                tsBytesLeft--;
                if ((pointerField > 0) && pidInfo->mBufPtr)
                  pidInfo->addToBuffer (ts, pointerField);
                ts += pointerField;
                tsBytesLeft -= pointerField;

                // parse any outstanding buffer
                if (pidInfo->mBufPtr) {
                  uint8_t* bufPtr = pidInfo->mBuffer;
                  while ((bufPtr+3) <= pidInfo->mBufPtr) // enough for tid,sectionLength
                    if (bufPtr[0] == 0xFF) // invalid tid, end of psi sections
                      break;
                    else // valid tid, parse psi section
                      bufPtr += parsePSI (*pidInfo, bufPtr);
                  }

                while ((tsBytesLeft >= 3) &&
                       (ts[0] != 0xFF) && (tsBytesLeft >= (((ts[1] & 0x0F) << 8) + ts[2] + 3))) {
                  // parse section without buffering
                  uint32_t sectionLength = parsePSI (*pidInfo, ts);
                  ts += sectionLength;
                  tsBytesLeft -= sectionLength;
                  }

                if (tsBytesLeft > 0 && (ts[0] != 0xFF)) {
                  // buffer rest of psi packet in new buffer
                  pidInfo->mBufPtr = pidInfo->mBuffer;
                  pidInfo->addToBuffer (ts, tsBytesLeft);
                  }
                }
              else if (pidInfo->mBufPtr)
                pidInfo->addToBuffer (ts, tsBytesLeft);
              }
              //}}}
            else if ((pidInfo->getStreamTypeId() == 5) ||  // mtd
                     (pidInfo->getStreamTypeId() == 11) || // dsm
                     (pidInfo->getStreamTypeId() == 134)) {
              }
            else {
              //{{{  pes
              cService* service;
              {
              lock_guard<mutex> fileLockGuard (mServiceMapMutex);
              service = getServiceBySid (pidInfo->getSid());
              }
              if (service)
                service->pesPacket (pidInfo->getPid(), ts - 1);

              ts += headerBytes;
              tsBytesLeft -= headerBytes;

              if (payloadStart) {
                if ((*(uint32_t*)ts & 0x00FFFFFF) == 0x00010000) {
                  if (service && pidInfo->mBufPtr) {
                    uint8_t streamId = (*((uint32_t*)(ts+3))) & 0xFF;
                    if ((streamId == 0xBD) || (streamId == 0xBE) || ((streamId >= 0xC0) && (streamId <= 0xEF)))
                      mPesCallback (*service, *pidInfo);
                    else if ((streamId == 0xB0) || (streamId == 0xB1) || (streamId == 0xBF))
                      cLog::log (LOGINFO, fmt::format ("demux unused pesPayload pid:{:4d} streamId:{:8x}", pid, streamId));
                    else
                      cLog::log (LOGINFO, fmt::format ("demux unknown pesPayload pid:{:4d} streamId:{:8x}", pid, streamId));
                    }

                  // reset pesBuffer pointer
                  pidInfo->mBufPtr = pidInfo->mBuffer;

                  // hasPts ?
                  if (ts[7] & 0x80)
                    pidInfo->setPts ((ts[7] & 0x80) ? cDvbUtils::getPts (ts+9) : -1);

                  // hasDts ?
                  if (ts[7] & 0x40)
                    pidInfo->setDts ((ts[7] & 0x80) ? cDvbUtils::getPts (ts+14) : -1);

                  // skip past pesHeader
                  int pesHeaderBytes = 9 + ts[8];
                  ts += pesHeaderBytes;
                  tsBytesLeft -= pesHeaderBytes;
                  }
                else
                  cLog::log (LOGINFO, fmt::format ("unrecognised pesHeader {} {:8x}", pid, *(uint32_t*)ts));
                }

              // add payload to buffer
              if (pidInfo->mBufPtr && (tsBytesLeft > 0))
                pidInfo->addToBuffer (ts, tsBytesLeft);
              }
              //}}}
            }
          }

        ts = nextPacket;
        nextPacket += kPacketSize;
        mNumPackets++;
        }
      }
    else {
      //{{{  sync error, return
      cLog::log (LOGERROR, "cTransportStream::demux lostSync");
      return tsEnd - chunk;
      }
      //}}}
    }

  return ts - chunk;
  }
//}}}

// private
//{{{
void cTransportStream::clear() {

  {
  lock_guard<mutex> lockGuard (mServiceMapMutex);
  mServiceMap.clear();
  }

  {
  lock_guard<mutex> lockGuard (mPidInfoMutex);
  mPidInfoMap.clear();
  mProgramMap.clear();
  }

  mNumErrors = 0;
  mNumPackets = 0;
  mHasFirstTdt = false;
  }
//}}}
//{{{
void cTransportStream::clearPidCounts() {

  for (auto& pidInfo : mPidInfoMap)
    pidInfo.second.clearCounts();
  }
//}}}
//{{{
void cTransportStream::clearPidContinuity() {

  for (auto& pidInfo : mPidInfoMap)
    pidInfo.second.clearContinuity();
  }
//}}}

//{{{
cTransportStream::cPidInfo& cTransportStream::getPidInfo (uint16_t pid) {
// find/create pidInfo by pid

  auto pidInfoIt = mPidInfoMap.find (pid);
  if (pidInfoIt == mPidInfoMap.end()) {
    // create new psi cPidInfo
    pidInfoIt = mPidInfoMap.emplace (pid, cPidInfo (pid, false)).first;

    // allocate buffer
    pidInfoIt->second.mBufSize = kInitBufSize;
    pidInfoIt->second.mBuffer = (uint8_t*)malloc (kInitBufSize);
    }

  return pidInfoIt->second;
  }
//}}}
//{{{
cTransportStream::cPidInfo* cTransportStream::getPsiPidInfo (uint16_t pid) {
// find/create recognised psi pidInfo by pid

  auto pidInfoIt = mPidInfoMap.find (pid);
  if (pidInfoIt == mPidInfoMap.end()) {
    if ((pid == PID_PAT) || (pid == PID_CAT) || (pid == PID_NIT) || (pid == PID_SDT) ||
        (pid == PID_EIT) || (pid == PID_RST) || (pid == PID_TDT) || (pid == PID_SYN) ||
        (mProgramMap.find (pid) != mProgramMap.end())) {
      // create new psi cPidInfo
      pidInfoIt = mPidInfoMap.emplace (pid, cPidInfo (pid, true)).first;

      // allocate buffer
      pidInfoIt->second.mBufSize = kInitBufSize;
      pidInfoIt->second.mBuffer = (uint8_t*)malloc (kInitBufSize);
      }
    else
      return nullptr;
    }

  return &pidInfoIt->second;
  }
//}}}
//{{{
cTransportStream::cService* cTransportStream::getServiceBySid (uint16_t sid) {

  auto it = mServiceMap.find (sid);
  return (it == mServiceMap.end()) ? nullptr : &it->second;
  }
//}}}

// parse
//{{{
uint16_t cTransportStream::parsePSI (cPidInfo& pidInfo, uint8_t* buf) {
// return sectionLength

  switch (pidInfo.getPid()) {
    case PID_PAT: parsePAT (buf); break;
    case PID_SDT: parseSDT (buf); break;
    case PID_EIT: parseEIT (buf); break;
    case PID_TDT: parseTDT (pidInfo, buf); break;

    case PID_NIT:
    case PID_CAT:
    case PID_RST:
    case PID_SYN: break;

    default: parsePMT (pidInfo, buf); break;
    }

  return ((buf[1] & 0x0F) << 8) + buf[2] + 3;
  }
//}}}
//{{{
void cTransportStream::parseTDT (cPidInfo& pidInfo, uint8_t* buf) {

  sTdt* tdt = (sTdt*)buf;
  if (tdt->table_id == TID_TDT) {
    mNowTdt = chrono::system_clock::from_time_t (mjdToEpochTime (tdt->utc_mjd) +
                                                 bcdTimeToSeconds (tdt->utc_time));
    if (!mHasFirstTdt) {
      mFirstTdt = mNowTdt;
      mHasFirstTdt = true;
      }

    pidInfo.setInfo (date::format ("%T", date::floor<chrono::seconds>(mFirstTdt)) + " " + getTdtString());
    }
  }
//}}}
//{{{
void cTransportStream::parsePAT (uint8_t* buf) {
// PAT declares programPid,sid to mProgramMap to recognise programPid PMT to declare service streams

  sPat* pat = (sPat*)buf;
  uint16_t sectionLength = HILO(pat->section_length) + 3;
  if (cDvbUtils::getCrc32(buf, sectionLength) != 0) {
    //{{{  bad crc error, return
    cLog::log (LOGERROR, fmt::format("parsePAT bad crc sectionLength:{}",sectionLength));
    return;
    }
    //}}}

  if (pat->table_id == TID_PAT) {
    buf += sizeof(sPat);
    sectionLength -= sizeof(sPat) + 4;
    while (sectionLength > 0) {
      auto patProgram = (sPatProg*)buf;
      uint16_t sid = HILO (patProgram->program_number);
      uint16_t pid = HILO (patProgram->network_pid);
      if (mProgramMap.find (pid) == mProgramMap.end())
        mProgramMap.emplace (pid, sid);

      sectionLength -= sizeof(sPatProg);
      buf += sizeof(sPatProg);
      }
    }
  }
//}}}
//{{{
void cTransportStream::parseSDT (uint8_t* buf) {
// SDT name services in mServiceMap

  sSdt* sdt = (sSdt*)buf;
  uint16_t sectionLength = HILO(sdt->section_length) + 3;
  if (cDvbUtils::getCrc32 (buf, sectionLength) != 0) {
    //{{{  wrong crc, error, return
    cLog::log (LOGERROR, fmt::format("parseSDT bad crc {}",sectionLength));
    return;
    }
    //}}}

  if (sdt->table_id == TID_SDT_ACT) {
    // only want this multiplex services
    buf += sizeof(sSdt);
    sectionLength -= sizeof(sSdt) + 4;
    while (sectionLength > 0) {
      sSdtDescriptor* sdtDescr = (sSdtDescriptor*)buf;
      buf += sizeof(sSdtDescriptor);

      uint16_t sid = HILO (sdtDescr->service_id);
      uint16_t loopLength = HILO (sdtDescr->descrs_loop_length);
      uint16_t descrLength = 0;
      while ((descrLength < loopLength) &&
             (getDescrLength (buf) > 0) && (getDescrLength (buf) <= loopLength - descrLength)) {
        switch (getDescrTag (buf)) {
          case DESCR_SERVICE: {
            //{{{  service
            string serviceName = cDvbUtils::getDvbString (buf + sizeof(descrService) +
                                                          ((descrService*)buf)->provider_name_length);
            cService* service = getServiceBySid (sid);
            if (service) {
              if (service->getName().empty()) {
                bool found = false;
                string recordName;
                size_t i = 0;
                for (auto& name : mDvbMultiplex.mNames) {
                  if (serviceName == name) {
                    found = true;
                    if (i < mDvbMultiplex.mRecordNames.size())
                      recordName = mDvbMultiplex.mRecordNames[i] +  " ";
                    break;
                    }
                  i++;
                  }
                service->setName (serviceName, found, recordName);
                cLog::log (LOGINFO, fmt::format ("SDT declares sid:{} as {} {} {}",
                                                 sid, serviceName, found ? "record" : "", recordName));
                }
              }
            else
              cLog::log (LOGINFO, fmt::format ("SDT before PMT ignored {} {}", sid, serviceName));

            break;
            }
            //}}}
          case DESCR_PRIV_DATA:
          case DESCR_CA_IDENT:
          case DESCR_COUNTRY_AVAIL:
          case DESCR_DATA_BROADCAST:
          case 0x73: // default_authority
          case 0x7e: // FTA_content_management
          case 0x7f: // extension
            break;
          default: cLog::log (LOGINFO, fmt::format ("SDT unexpected tag {}", (int)getDescrTag(buf)));
          }

        descrLength += getDescrLength (buf);
        buf += getDescrLength (buf);
        }

      sectionLength -= loopLength + sizeof(sSdtDescriptor);
      }
    }
  }
//}}}
//{{{
void cTransportStream::parseEIT (uint8_t* buf) {

  sEit* eit = (sEit*)buf;
  uint16_t sectionLength = HILO(eit->section_length) + 3;
  if (cDvbUtils::getCrc32 (buf, sectionLength) != 0) {
    //{{{  bad crc, error, return
    cLog::log (LOGERROR, fmt::format("parseEit bad CRC {}",sectionLength));
    return;
    }
    //}}}

  uint16_t tid = eit->table_id;

  bool now = (tid == TID_EIT_ACT);
  bool next = (tid == TID_EIT_OTH);
  bool epg = (tid == TID_EIT_ACT_SCH) || (tid == TID_EIT_OTH_SCH) ||
             (tid == TID_EIT_ACT_SCH+1) || (tid == TID_EIT_OTH_SCH+1);

  if (now || epg) {
    uint16_t sid = HILO (eit->service_id);
    buf += sizeof(sEit);
    sectionLength -= sizeof(sEit) + 4;
    while (sectionLength > 0) {
      sEitEvent* eitEvent = (sEitEvent*)buf;
      uint16_t loopLength = HILO (eitEvent->descrs_loop_length);
      buf += sizeof(sEitEvent);
      sectionLength -= sizeof(sEitEvent);

      // parse descriptors
      while (loopLength > 0) {
        if (getDescrTag (buf) == DESCR_SHORT_EVENT)  {
          //{{{  shortEvent
          cService* service = getServiceBySid (sid);
          if (service) {
            // known service
            auto startTime = chrono::system_clock::from_time_t (mjdToEpochTime (eitEvent->mjd) +
                                                                bcdTimeToSeconds (eitEvent->start_time));
            chrono::seconds duration (bcdTimeToSeconds (eitEvent->duration));

            // get title
            uint8_t* bufPtr = buf + sizeof(descrShortEvent) - 1;
            string title = cDvbUtils::getDvbString (bufPtr);

            // get info
            bufPtr += ((descrShortEvent*)buf)->event_name_length;
            string info = cDvbUtils::getDvbString (bufPtr);

            if (now) {
              // now event
              bool running = (eitEvent->running_status == 0x04);
              if (running &&
                  !service->getName().empty() &&
                  service->getProgramPid() &&
                  service->getStream (cRenderStream::eVideo).isDefined() &&
                  service->getStream (cRenderStream::eVideo).isDefined() &&
                  service->getStream (cRenderStream::eAudio).isDefined()) {
                // now event for named service with valid pgmPid,vidPid,audPid
                if (service->setNow (service->isEpgRecord (title, startTime),
                                     startTime, duration, title, info)) {
                  // new nowEvent
                  auto pidInfoIt = mPidInfoMap.find (service->getProgramPid());
                  if (pidInfoIt != mPidInfoMap.end())
                    // update service pgmPid infoStr with new nowEvent
                    pidInfoIt->second.setInfo (service->getName() + " " + service->getNowTitleString());

                  // start new program on service
                  lock_guard<mutex> lockGuard (mServiceMapMutex);
                  service->start (mNowTdt, title, startTime, service->isEpgRecord (title, startTime));
                  }
                }
              }
            else // epg event, add it
              service->setEpg (false, getNowTdt(), startTime, duration, title, info);
            }
          }
          //}}}

        loopLength -= getDescrLength (buf);
        sectionLength -= getDescrLength (buf);
        buf += getDescrLength (buf);
        }
      }
    }
  else if (!next)
    cLog::log (LOGERROR, fmt::format("parseEIT unexpected tid {}",tid));
  }
//}}}
//{{{
void cTransportStream::parsePMT (cPidInfo& pidInfo, uint8_t* buf) {
// PMT declares service pgmPid,streams

  sPmt* pmt = (sPmt*)buf;
  uint16_t sectionLength = HILO(pmt->section_length) + 3;
  if (cDvbUtils::getCrc32 (buf, sectionLength) != 0) {
    //{{{  badCrc error, return
    cLog::log (LOGERROR, "parsePMT pid:%d bad crc %d", pidInfo.getPid(), sectionLength);
    return;
    }
    //}}}

  if (pmt->table_id == TID_PMT) {
    uint16_t sid = HILO (pmt->program_number);
    if (!getServiceBySid (sid)) {
      // create cService
      cLog::log (LOGINFO, fmt::format ("PMT program pid:{} declares service sid:{}", pidInfo.getPid(), sid));
      cService& service = mServiceMap.emplace (sid, cService(sid, mOptions)).first->second;
      service.setProgramPid (pidInfo.getPid());
      pidInfo.setSid (sid);

      buf += sizeof(sPmt);
      sectionLength -= 4;
      uint16_t programInfoLength = HILO (pmt->program_info_length);
      uint16_t streamLength = sectionLength - programInfoLength - sizeof(sPmt);

      buf += programInfoLength;
      while (streamLength > 0) {
        //{{{  iterate service streams, set service elementary stream pids
        sPmtInfo* pmtInfo = (sPmtInfo*)buf;
        uint16_t esPid = HILO (pmtInfo->elementary_PID);
        cPidInfo& esPidInfo = getPidInfo (esPid);
        esPidInfo.setSid (sid);

        esPidInfo.setStreamTypeId (pmtInfo->stream_type);
        switch (esPidInfo.getStreamTypeId()) {
          case 2: // ISO 13818-2 video
          case kH264StreamType: // 27 - H264 video
            service.setVideoStream (esPid, esPidInfo.getStreamTypeId());
            service.getStream (cRenderStream::eVideo).setPidTypeId (esPid, esPidInfo.getStreamTypeId());
            break;

          case 3: // ISO 11172-3 audio
          case 4: // ISO 13818-3 audio
          case 15: // ADTS AAC audio
          case kAacLatmStreamType: // 17 - LATM AAC audio
          case 129: // AC3 audio
            if (!service.getStream (cRenderStream::eAudio).isDefined()) {
              service.setAudioStream (esPid, esPidInfo.getStreamTypeId());
              service.getStream (cRenderStream::eAudio).setPidTypeId (esPid, esPidInfo.getStreamTypeId());
              }
            else if (esPid != service.getStream (cRenderStream::eAudio).getPid()) {
              // got main eAudio, use new audPid as eDescription
              if (!service.getStream (cRenderStream::eDescription).isDefined())
                service.getStream (cRenderStream::eDescription).setPidTypeId (esPid,  esPidInfo.getStreamTypeId());
              }
            break;

          case 6: // subtitle
            service.setSubtitleStream (esPid, esPidInfo.getStreamTypeId());
            service.getStream (cRenderStream::eSubtitle).setPidTypeId (esPid, esPidInfo.getStreamTypeId());
            break;

          case 5: // private mpeg2 tabled data - private
          case 11: // dsm cc u_n
          case 13: // dsm cc tabled data
          case 134:
            break;

          default:
            cLog::log (LOGERROR, fmt::format ("parsePmt unknown stream sid:{} pid:{} streamTypeId:{}",
                                              sid, esPid, esPidInfo.getStreamTypeId()));
            break;
          }
        //}}}
        uint16_t loopLength = HILO (pmtInfo->ES_info_length);
        buf += sizeof(sPmtInfo);
        streamLength -= loopLength + sizeof(sPmtInfo);
        buf += loopLength;
        }

      // callback to add new service
      mAddServiceCallback (service);
      }
    }
  }
//}}}
