// cTransportStream.cpp - mpeg transport stream demux
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

#include "../common/date.h"
#include "../common/utils.h"
#include "../common/cDvbUtils.h"
#include "../common/cLog.h"

#include "cVideoRender.h"
#include "cAudioRender.h"
#include "cSubtitleRender.h"

using namespace std;
//}}}
//{{{  defines, const, struct
#define HILO(x) (x##_hi << 8 | x##_lo)

#define MjdToEpochTime(x) (unsigned int)((((x##_hi << 8) | x##_lo) - 40587) * 86400)

#define BcdTimeToSeconds(x) ((3600 * ((10*((x##_h & 0xF0)>>4)) + (x##_h & 0xF))) + \
                               (60 * ((10*((x##_m & 0xF0)>>4)) + (x##_m & 0xF))) + \
                                     ((10*((x##_s & 0xF0)>>4)) + (x##_s & 0xF)))

const int kInitBufSize = 512;

//{{{  pids
#define PID_PAT   0x00   /* Program Association Table */
#define PID_CAT   0x01   /* Conditional Access Table */
#define PID_NIT   0x10   /* Network Information Table */
#define PID_SDT   0x11   /* Service Description Table */
#define PID_EIT   0x12   /* Event Information Table */
#define PID_RST   0x13   /* Running Status Table */
#define PID_TDT   0x14   /* Time Date Table */
#define PID_SYN   0x15   /* Network sync */
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

//{{{
struct sNit {
  uint8_t table_id                 :8;

  uint8_t section_length_hi        :4;
  uint8_t                          :3;
  uint8_t section_syntax_indicator :1;
  uint8_t section_length_lo        :8;

  uint8_t network_id_hi            :8;
  uint8_t network_id_lo            :8;
  uint8_t current_next_indicator   :1;
  uint8_t version_number           :5;
  uint8_t                          :2;
  uint8_t section_number           :8;
  uint8_t last_section_number      :8;
  uint8_t network_descr_length_hi  :4;
  uint8_t                          :4;
  uint8_t network_descr_length_lo  :8;
  /* descrs */
  } ;
//}}}
//{{{
struct sNitMid {
  uint8_t transport_stream_loop_length_hi  :4;
  uint8_t                                  :4;
  uint8_t transport_stream_loop_length_lo  :8;
  } ;
//}}}
//{{{
struct sNitTs {
  uint8_t transport_stream_id_hi      :8;
  uint8_t transport_stream_id_lo      :8;
  uint8_t original_network_id_hi      :8;
  uint8_t original_network_id_lo      :8;
  uint8_t transport_descrs_length_hi  :4;
  uint8_t                             :4;
  uint8_t transport_descrs_length_lo  :8;
  /* descrs  */
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
    return cDvbUtils::getStreamTypeName (mStreamType);

  // unknown pid
  return "---";
  }
//}}}

//{{{
int cTransportStream::cPidInfo::addToBuffer (uint8_t* buf, int bufSize) {

  if (getBufUsed() + bufSize > mBufSize) {
    // realloc buffer to twice size
    mBufSize = mBufSize * 2;
    cLog::log (LOGINFO1, fmt::format ("demux pid:{} realloc {}", mPid,mBufSize));

    int ptrOffset = getBufUsed();
    mBuffer = (uint8_t*)realloc (mBuffer, mBufSize);
    mBufPtr = mBuffer + ptrOffset;
    }

  memcpy (mBufPtr, buf, bufSize);
  mBufPtr += bufSize;

  return getBufUsed();
  }
//}}}
//}}}
//{{{  class cTransportStream::cStream
cTransportStream::cStream::~cStream() { delete mRender; }

//{{{
void cTransportStream::cStream::setPidStreamType (uint16_t pid, uint8_t streamType) {

  mDefined = true;

  mPid = pid;
  mTypeId = streamType;
  mTypeName = cDvbUtils::getStreamTypeName (streamType);
  }
//}}}
//{{{
bool cTransportStream::cStream::disable() {
// return true if not enabled

  if (mRender) {
    // remove render
    cRender* mTemp = mRender;
    mRender = nullptr;
    delete mTemp;
    return false;
    }

  return true;
  }
//}}}
//}}}
//{{{  class cTransportStream::cService
//{{{
cTransportStream::cService::cService (uint16_t sid, iOptions* options) : mSid(sid), mOptions(options) {

  mStreams[eVideo].setLabel ("vid:");
  mStreams[eAudio].setLabel ("aud:");
  mStreams[eDescription].setLabel ("des:");
  mStreams[eSubtitle].setLabel ("sub:");
  }
//}}}
//{{{
cTransportStream::cService::~cService() {

  delete mNowEpgItem;

  for (auto& epgItem : mEpgItemMap)
    delete epgItem.second;

  mEpgItemMap.clear();

  closeFile();
  }
//}}}

// streams
//{{{
cTransportStream::cStream* cTransportStream::cService::getStreamByPid (uint16_t pid) {

  for (uint8_t streamType = eVideo; streamType <= eSubtitle; streamType++) {
    cTransportStream::cStream& stream = getStream (eStreamType(streamType));
    if (stream.isDefined() && (stream.getPid() == pid))
      return &mStreams[streamType];
    }

  return nullptr;
  }
//}}}
//{{{
void cTransportStream::cService::enableStreams() {

  cStream& videoStream = getStream (eVideo);
  videoStream.setRender (
    new cVideoRender (getName(), videoStream.getTypeId(), videoStream.getPid(), mOptions));

  cStream& audioStream = getStream (eAudio);
  audioStream.setRender (
    new cAudioRender (getName(), audioStream.getTypeId(), audioStream.getPid(), mOptions));

  cStream& subtitleStream = getStream (eSubtitle);
  subtitleStream.setRender (
    new cSubtitleRender (getName(), subtitleStream.getTypeId(), subtitleStream.getPid(), mOptions));
  }
//}}}
//{{{
void cTransportStream::cService::skipStreams (int64_t skipPts) {
  cLog::log (LOGINFO, fmt::format ("cTransportStream::cService::skip {}", skipPts));
  }
//}}}

// record
//{{{
bool cTransportStream::cService::openFile (const string& fileName, uint16_t tsid) {

  mFile = fopen (fileName.c_str(), "wb");
  if (mFile) {
    writePat (tsid);
    writePmt();
    return true;
    }

  cLog::log (LOGERROR, "cService::openFile " + fileName);
  return false;
  }
//}}}
//{{{
void cTransportStream::cService::writePacket (uint8_t* ts, uint16_t pid) {
//  pes ts packet, save only recognised pids

  if (mFile &&
      ((pid == mStreams[eVideo].getPid()) ||
       (pid == mStreams[eAudio].getPid()) ||
       (pid == mStreams[eSubtitle].getPid())))
    fwrite (ts, 1, 188, mFile);
  }
//}}}
//{{{
void cTransportStream::cService::closeFile() {

  if (mFile)
    fclose (mFile);

  mFile = nullptr;
  }
//}}}

// epg
//{{{
bool cTransportStream::cService::isEpgRecord (const string& title, chrono::system_clock::time_point startTime) {
// return true if startTime, title selected to record in epg

  auto it = mEpgItemMap.find (startTime);
  if (it != mEpgItemMap.end())
    if (title == it->second->getTitleString())
      if (it->second->getRecord())
        return true;

  return false;
  }
//}}}
//{{{
bool cTransportStream::cService::setNow (bool record,
                                   chrono::system_clock::time_point time, chrono::seconds duration,
                                   const string& titleString, const string& infoString) {

  if (mNowEpgItem && (mNowEpgItem->getTime() == time))
    return false;

  delete mNowEpgItem;

  mNowEpgItem = new cEpgItem (true, record, time, duration, titleString, infoString);

  return true;
  }
//}}}
//{{{
bool cTransportStream::cService::setEpg (bool record,
                                   chrono::system_clock::time_point startTime, chrono::seconds duration,
                                   const string& titleString, const string& infoString) {
// could return true only if changed

  auto it = mEpgItemMap.find (startTime);
  if (it == mEpgItemMap.end())
    mEpgItemMap.emplace (startTime, new cEpgItem (false, record, startTime, duration, titleString, infoString));
  else
    it->second->set (startTime, duration, titleString, infoString);

  return true;
  }
//}}}

// private:
//{{{
uint8_t* cTransportStream::cService::tsHeader (uint8_t* ts, uint16_t pid, uint8_t continuityCount) {

  memset (ts, 0xFF, 188);

  *ts++ = 0x47;                         // sync byte
  *ts++ = 0x40 | ((pid & 0x1f00) >> 8); // payload_unit_start_indicator + pid upper
  *ts++ = pid & 0xff;                   // pid lower
  *ts++ = 0x10 | continuityCount;       // no adaptControls + cont count
  *ts++ = 0;                            // pointerField

  return ts;
  }
//}}}

//{{{
void cTransportStream::cService::writePat (uint16_t tsid) {

  uint8_t ts[188];
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

  writeSection (ts, tsSectionStart, tsPtr);
  }
//}}}
//{{{
void cTransportStream::cService::writePmt() {

  uint8_t ts[188];
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

  *tsPtr++ = 0xE0 | ((mStreams[eVideo].getPid() & 0x1F00) >> 8);
  *tsPtr++ = mStreams[eVideo].getPid() & 0x00FF;

  *tsPtr++ = 0xF0;
  *tsPtr++ = 0; // program_info_length;

  // video es
  *tsPtr++ = mStreams[eVideo].getTypeId(); // elementary stream_type
  *tsPtr++ = 0xE0 | ((mStreams[eVideo].getPid() & 0x1F00) >> 8); // elementary_PID
  *tsPtr++ = mStreams[eVideo].getPid() & 0x00FF;
  *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0;       // ES_info_length
  *tsPtr++ = 0 & 0x00FF;

  // audio es
  *tsPtr++ = mStreams[eAudio].getTypeId(); // elementary stream_type
  *tsPtr++ = 0xE0 | ((mStreams[eAudio].getPid() & 0x1F00) >> 8); // elementary_PID
  *tsPtr++ = mStreams[eAudio].getPid() & 0x00FF;
  *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0;       // ES_info_length
  *tsPtr++ = 0 & 0x00FF;

  // subtitle es
  *tsPtr++ = mStreams[eSubtitle].getTypeId(); // elementary stream_type
  *tsPtr++ = 0xE0 | ((mStreams[eSubtitle].getPid() & 0x1F00) >> 8); // elementary_PID
  *tsPtr++ = mStreams[eSubtitle].getPid() & 0x00FF;
  *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0;       // ES_info_length
  *tsPtr++ = 0 & 0x00FF;

  writeSection (ts, tsSectionStart, tsPtr);
  }
//}}}
//{{{
void cTransportStream::cService::writeSection (uint8_t* ts, uint8_t* tsSectionStart, uint8_t* tsPtr) {

  // tsSection crc, calc from tsSection start to here
  uint32_t crc = cDvbUtils::getCrc32 (tsSectionStart, int(tsPtr - tsSectionStart));
  *tsPtr++ = (crc & 0xff000000) >> 24;
  *tsPtr++ = static_cast<uint8_t>((crc & 0x00ff0000) >> 16);
  *tsPtr++ = (crc & 0x0000ff00) >>  8;
  *tsPtr++ =  crc & 0x000000ff;

  fwrite (ts, 1, 188, mFile);
  }
//}}}
//}}}

// cTransportStream
//{{{
cTransportStream::cTransportStream (const cDvbMultiplex& dvbMultiplex, iOptions* options) :
    mDvbMultiplex(dvbMultiplex), mOptions(options) {}
//}}}

// gets
//{{{
string cTransportStream::getTdtString() const {
  return date::format ("%T", date::floor<chrono::seconds>(mTdt));
  }
//}}}

// demux
//{{{
void cTransportStream::skip (int64_t skipPts) {

  for (auto& service : mServiceMap)
    service.second.skipStreams (skipPts);
  }
//}}}
//{{{
int64_t cTransportStream::demux (uint8_t* chunk, int64_t chunkSize, int64_t streamPos, int64_t skipPts) {
// demux from chunk to chunk + chunkSize, streamPos offset from first packet

  if (skipPts != 0)
    clearPidContinuity();

  uint8_t* ts = chunk;
  uint8_t* tsEnd = chunk + chunkSize;

  uint8_t* nextPacket = ts + 188;
  while (nextPacket <= tsEnd) {
    if (*ts == 0x47) {
      if ((ts+188 >= tsEnd) || (*(ts+188) == 0x47)) {
        ts++;
        int tsBytesLeft = 188-1;

        uint16_t pid = ((ts[0] & 0x1F) << 8) | ts[1];
        if (pid != 0x1FFF) {
          bool payloadStart =   ts[0] & 0x40;
          int continuityCount = ts[2] & 0x0F;
          int headerBytes =    (ts[2] & 0x20) ? (4 + ts[3]) : 3; // adaption field

          lock_guard<mutex> lockGuard (mMutex);
          cPidInfo* pidInfo = getPsiPidInfo (pid);
          if (pidInfo) {
            if ((pidInfo->mContinuity >= 0) &&
                (continuityCount != ((pidInfo->mContinuity+1) & 0x0F))) {
              //{{{  continuity error
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
                      bufPtr += parsePsi (pidInfo, bufPtr);
                  }

                while ((tsBytesLeft >= 3) &&
                       (ts[0] != 0xFF) && (tsBytesLeft >= (((ts[1] & 0x0F) << 8) + ts[2] + 3))) {
                  // parse section without buffering
                  uint32_t sectionLength = parsePsi (pidInfo, ts);
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
            else if (pidInfo->getStreamType() == 5) {
              //{{{  mtd - do nothing
              }
              //}}}
            else if (pidInfo->getStreamType() == 11) {
              //{{{  dsm - do nothing
              }
              //}}}
            else if (pidInfo->getStreamType() == 134) {
              //{{{  ??? - do nothing
              }
              //}}}
            else {
              //{{{  pes
              programPesPacket (pidInfo->getSid(), pidInfo->getPid(), ts - 1);

              ts += headerBytes;
              tsBytesLeft -= headerBytes;

              if (payloadStart) {
                if ((*(uint32_t*)ts & 0x00FFFFFF) == 0x00010000) {
                  //{{{  process previous pesBuffer, use new pesBuffer
                  uint8_t streamId = (*((uint32_t*)(ts+3))) & 0xFF;
                  if ((streamId == 0xB0) || // program stream map
                      (streamId == 0xB1) || // private stream1
                      (streamId == 0xBF))   // private stream2
                    cLog::log (LOGINFO, fmt::format ("recognised pesHeader - pid:{} streamId:{:8x}", pid, streamId));

                  else if ((streamId == 0xBD) ||
                           (streamId == 0xBE) || // ???
                           ((streamId >= 0xC0) && (streamId <= 0xEF))) {
                    // subtitle, audio, video streamId
                    if (pidInfo->mBufPtr)
                      if (renderPes (*pidInfo, skipPts)) // transferred ownership of mBuffer to render
                        pidInfo->mBuffer = (uint8_t*)malloc (pidInfo->mBufSize);
                    }
                  else
                    cLog::log (LOGINFO, fmt::format ("cTransportStream::demux - unknown streamId:{:2x}", streamId));

                  // reset pesBuffer pointer
                  pidInfo->mBufPtr = pidInfo->mBuffer;

                  // save ts streamPos for start of new pesBuffer
                  pidInfo->mStreamPos = streamPos;

                  // pts
                  if (ts[7] & 0x80)
                    pidInfo->setPts ((ts[7] & 0x80) ? cDvbUtils::getPts (ts+9) : -1);

                  // dts
                  if (ts[7] & 0x40)
                    pidInfo->setDts ((ts[7] & 0x80) ? cDvbUtils::getPts (ts+14) : -1);

                  // skip past pesHeader
                  int pesHeaderBytes = 9 + ts[8];
                  ts += pesHeaderBytes;
                  tsBytesLeft -= pesHeaderBytes;
                  }
                  //}}}
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
        nextPacket += 188;
        streamPos += 188;
        mNumPackets++;
        }
      }
    else {
      //{{{  sync error, return
      cLog::log (LOGERROR, "demux - lostSync");
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

  lock_guard<mutex> lockGuard (mMutex);

  mServiceMap.clear();
  mProgramMap.clear();
  mPidInfoMap.clear();

  mNumErrors = 0;

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
// find or create pidInfo by pid

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
// find or create pidInfo by pid

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

//{{{
bool cTransportStream::renderPes (cPidInfo& pidInfo, int64_t skipPts) {
// send pes to render stream, return true if pes ownership transferred

  cService* service = getServiceBySid (pidInfo.getSid());
  if (service) {
    cStream* stream = service->getStreamByPid (pidInfo.getPid());
    if (stream) {
      stream->setPts (pidInfo.getPts());
      if (stream->isEnabled())
        return stream->getRender().processPes (pidInfo.getPid(),
                                               pidInfo.mBuffer, pidInfo.getBufUsed(),
                                               pidInfo.getPts(), pidInfo.getDts(), skipPts);
      }
    }

  return false;
  }
//}}}

//{{{
void cTransportStream::startServiceProgram (cService& service,
                                            chrono::system_clock::time_point tdt,
                                            const string& programName,
                                            chrono::system_clock::time_point programStartTime,
                                            bool selected) {
// start recording service program

  // close prev program on this service
  lock_guard<mutex> lockGuard (mRecordFileMutex);
  service.closeFile();

  if ((selected || service.getChannelRecord() || (dynamic_cast<cOptions*>(mOptions))->mRecordAll) &&
      service.getStream (eVideo).isDefined() &&
      (service.getStream(eAudio).isDefined())) {
    string filePath = (dynamic_cast<cOptions*>(mOptions))->mRecordRoot +
                      service.getRecordName() +
                      date::format ("%d %b %y %a %H.%M.%S ", date::floor<chrono::seconds>(tdt)) +
                      utils::getValidFileString (programName) +
                      ".ts";

    // record
    service.openFile (filePath, 0x1234);

    // gui
    mRecorded.push_back (filePath);

    // log program start time,date filename
    string eitStartTime = date::format ("%H.%M.%S %a %d %b %y", date::floor<chrono::seconds>(programStartTime));
    cLog::log (LOGINFO, fmt::format ("{} {}", eitStartTime, filePath));
    }
  }
//}}}
//{{{
void cTransportStream::programPesPacket (uint16_t sid, uint16_t pid, uint8_t* ts) {
// send pes ts packet to sid service

  lock_guard<mutex> lockGuard (mRecordFileMutex);

  cService* service = getServiceBySid (sid);
  if (service)
    service->writePacket (ts, pid);
  }
//}}}
//{{{
void cTransportStream::stopServiceProgram (cService& service) {
// stop recording service, never called

  lock_guard<mutex> lockGuard (mRecordFileMutex);
  service.closeFile();
  }
//}}}

// parse
//{{{
int cTransportStream::parsePsi (cPidInfo* pidInfo, uint8_t* buf) {
// return sectionLength

  switch (pidInfo->getPid()) {
    case PID_PAT: parsePat (pidInfo, buf); break;
    case PID_NIT: parseNit (pidInfo, buf); break;
    case PID_SDT: parseSdt (pidInfo, buf); break;
    case PID_EIT: parseEit (pidInfo, buf); break;
    case PID_TDT: parseTdt (pidInfo, buf); break;

    case PID_CAT:
    case PID_RST:
    case PID_SYN: break;

    default: parsePmt (pidInfo, buf); break;
    }

  return ((buf[1] & 0x0F) << 8) + buf[2] + 3;
  }
//}}}
//{{{
void cTransportStream::parsePat (cPidInfo* pidInfo, uint8_t* buf) {
// PAT declares programPid,sid to mProgramMap to recognise programPid PMT to declare service streams

  (void)pidInfo;

  sPat* pat = (sPat*)buf;
  uint16_t sectionLength = HILO(pat->section_length) + 3;
  if (cDvbUtils::getCrc32(buf, sectionLength) != 0) {
    //{{{  bad crc error, return
    cLog::log (LOGERROR, fmt::format("parsePAT - bad crc - sectionLength:{}",sectionLength));
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
void cTransportStream::parseNit (cPidInfo* pidInfo, uint8_t* buf) {

  (void)pidInfo;

  sNit* nit = (sNit*)buf;
  uint16_t sectionLength = HILO(nit->section_length) + 3;
  if (cDvbUtils::getCrc32 (buf, sectionLength) != 0) {
    //{{{  bad crc, error, return
    cLog::log (LOGERROR, fmt::format("parseNIT - bad crc {}",sectionLength));
    return;
    }
    //}}}
  if ((nit->table_id != TID_NIT_ACT) &&
      (nit->table_id != TID_NIT_OTH) &&
      (nit->table_id != TID_BAT)) {
    //{{{  wrong tid, error, return
    cLog::log (LOGERROR, fmt::format ("parseNIT - wrong TID {}", (int)(nit->table_id)));
    return;
    }
    //}}}

  //auto networkId = HILO (nit->network_id);
  buf += sizeof(sNit);
  uint16_t loopLength = HILO (nit->network_descr_length);

  sectionLength -= sizeof(sNit) + 4;
  if (loopLength <= sectionLength) {
    sectionLength -= loopLength;

    buf += loopLength;
    auto nitMid = (sNitMid*)buf;
    loopLength = HILO (nitMid->transport_stream_loop_length);
    if ((sectionLength > 0) && (loopLength <= sectionLength)) {
      // iterate nitMids
      sectionLength -= sizeof(sNitMid);
      buf += sizeof(sNitMid);

      while (loopLength > 0) {
        auto TSDesc = (sNitTs*)buf;
        //auto tsid = HILO (TSDesc->transport_stream_id);
        uint16_t loopLength2 = HILO (TSDesc->transport_descrs_length);
        buf += sizeof(sNitTs);
        loopLength -= loopLength2 + sizeof(sNitTs);
        sectionLength -= loopLength2 + sizeof(sNitTs);
        buf += loopLength2;
        }
      }
    }
  }
//}}}
//{{{
void cTransportStream::parseSdt (cPidInfo* pidInfo, uint8_t* buf) {
// SDT name services in mServiceMap

  (void)pidInfo;

  sSdt* sdt = (sSdt*)buf;
  uint16_t sectionLength = HILO(sdt->section_length) + 3;
  if (cDvbUtils::getCrc32 (buf, sectionLength) != 0) {
    //{{{  wrong crc, error, return
    cLog::log (LOGERROR, fmt::format("parseSDT - bad crc {}",sectionLength));
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
                cLog::log (LOGINFO, fmt::format ("SDT named sid:{} as {} {} {}",
                                                 sid, serviceName, found ? "record" : "", recordName));
                }
              }
            else
              cLog::log (LOGINFO, fmt::format ("SDT - before PMT - ignored {} {}", sid, serviceName));

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
          default: cLog::log (LOGINFO, fmt::format ("SDT - unexpected tag {}", (int)getDescrTag(buf)));
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
void cTransportStream::parseEit (cPidInfo* pidInfo, uint8_t* buf) {

  (void)pidInfo;

  sEit* eit = (sEit*)buf;
  uint16_t sectionLength = HILO(eit->section_length) + 3;
  if (cDvbUtils::getCrc32 (buf, sectionLength) != 0) {
    //{{{  bad crc, error, return
    cLog::log (LOGERROR, fmt::format("parseEit - bad CRC {}",sectionLength));
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
            auto startTime = chrono::system_clock::from_time_t (
              MjdToEpochTime (eitEvent->mjd) + BcdTimeToSeconds (eitEvent->start_time));
            chrono::seconds duration (BcdTimeToSeconds (eitEvent->duration));

            // get title
            auto bufPtr = buf + sizeof(descrShortEvent) - 1;
            auto titleString = cDvbUtils::getDvbString (bufPtr);

            // get info
            bufPtr += ((descrShortEvent*)buf)->event_name_length;
            string infoString = cDvbUtils::getDvbString (bufPtr);

            if (now) {
              // now event
              bool running = (eitEvent->running_status == 0x04);
              if (running &&
                  !service->getName().empty() &&
                  service->getProgramPid() &&
                  service->getStream (eVideo).isDefined() &&
                  service->getStream (eAudio).isDefined()) {
                  //(service->getSubPid())) {
                // now event for named service with valid pgmPid, vidPid, audPid, subPid
                if (service->setNow (service->isEpgRecord (titleString, startTime),
                                     startTime, duration, titleString, infoString)) {
                  // new now event
                  auto pidInfoIt = mPidInfoMap.find (service->getProgramPid());
                  if (pidInfoIt != mPidInfoMap.end())
                    // update service pgmPid infoStr with new now event
                    pidInfoIt->second.setInfoString (service->getName() + " " + service->getNowTitleString());

                  // callback to override to start new serviceItem program
                  startServiceProgram (*service, mTdt, titleString, startTime,
                                       service->isEpgRecord (titleString, startTime));
                  }
                }
              }
            else // epg event, add it
              service->setEpg (false, startTime, duration, titleString, infoString);
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
    cLog::log (LOGERROR, fmt::format("parseEIT - unexpected tid {}",tid));
  }
//}}}
//{{{
void cTransportStream::parseTdt (cPidInfo* pidInfo, uint8_t* buf) {

  sTdt* tdt = (sTdt*)buf;
  if (tdt->table_id == TID_TDT) {
    mTdt =
      chrono::system_clock::from_time_t (MjdToEpochTime (tdt->utc_mjd) + BcdTimeToSeconds (tdt->utc_time));

    if (!mHasFirstTdt) {
      mFirstTdt = mTdt;
      mHasFirstTdt = true;
      }

    pidInfo->setInfoString (date::format ("%T", date::floor<chrono::seconds>(mFirstTdt)) +
                            " to " + getTdtString());
    }
  }
//}}}
//{{{
void cTransportStream::parsePmt (cPidInfo* pidInfo, uint8_t* buf) {
// PMT declares pgmPid and streams for a service

  sPmt* pmt = (sPmt*)buf;
  uint16_t sectionLength = HILO(pmt->section_length) + 3;
  if (cDvbUtils::getCrc32 (buf, sectionLength) != 0) {
    //{{{  badCrc error, return
    cLog::log (LOGERROR, "parsePMT - pid:%d bad crc %d", pidInfo->getPid(), sectionLength);
    return;
    }
    //}}}

  if (pmt->table_id == TID_PMT) {
    uint16_t sid = HILO (pmt->program_number);
    if (!getServiceBySid (sid)) {
      // found new service, create cService
      cLog::log (LOGINFO, fmt::format ("create service {}", sid));
      cService& service = mServiceMap.emplace (sid, cService(sid, mOptions)).first->second;

      service.setProgramPid (pidInfo->getPid());
      pidInfo->setSid (sid);

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

        esPidInfo.setStreamType (pmtInfo->stream_type);
        switch (esPidInfo.getStreamType()) {
          case 2: // ISO 13818-2 video
          case kH264StreamType: // 27 - H264 video
            service.getStream (eVideo).setPidStreamType (esPid, esPidInfo.getStreamType()); break;

          case 3: // ISO 11172-3 audio
          case 4: // ISO 13818-3 audio
          case 15: // ADTS AAC audio
          case kAacLatmStreamType: // 17 - LATM AAC audio
          case 129: // AC3 audio
            if (!service.getStream (eAudio).isDefined())
              service.getStream (eAudio).setPidStreamType (esPid, esPidInfo.getStreamType());
            else if (esPid != service.getStream (eAudio).getPid()) {
              // got main eAudio, use new audPid as eDescription
              if (!service.getStream (eDescription).isDefined())
                service.getStream (eDescription).setPidStreamType (esPid,  esPidInfo.getStreamType());
              }
            break;

          case 6: // subtitle
            service.getStream (eSubtitle).setPidStreamType (esPid, esPidInfo.getStreamType());
            break;

          case 5: // private mpeg2 tabled data - private
          case 11: // dsm cc u_n
          case 13: // dsm cc tabled data
          case 134: break;

          default:
            cLog::log (LOGERROR, fmt::format ("parsePmt - unknown stream sid:{} pid:{} streamType:{}",
                                              sid, esPid, esPidInfo.getStreamType()));
            break;
          }
        //}}}

        uint16_t loopLength = HILO (pmtInfo->ES_info_length);
        buf += sizeof(sPmtInfo);
        streamLength -= loopLength + sizeof(sPmtInfo);
        buf += loopLength;
        }

      // enable at least one service streams
      if (!mShowingFirstService || (dynamic_cast<cOptions*>(mOptions))->mShowAllServices)
        if (service.getStream (eStreamType(eVideo)).isDefined()) {
          mShowingFirstService = true;
          service.enableStreams();
          }
      }
    }
  }
//}}}
