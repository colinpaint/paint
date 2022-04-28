// cDvbStream.cpp - file or dvbSource demux
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include "cDvbStream.h"

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <thread>

#ifdef __linux__
  #include <unistd.h>
  #include <sys/poll.h>
#endif

#include "cDvbSource.h"
#include "cDvbUtils.h"

#include "cVideoRender.h"
#include "cAudioRender.h"
#include "cSubtitleRender.h"

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/utils.h"

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

//{{{  class cDvbStream::cStream
//{{{
cDvbStream::cStream::~cStream() {
  delete mRender;
  }
//}}}

//{{{
void cDvbStream::cStream::setPidTypeId (uint16_t pid, uint8_t streamTypeId) {

  mDefined = true;
  mPid = pid;
  mTypeId = streamTypeId;
  mTypeName = cDvbUtils::getStreamTypeName (streamTypeId);
  }
//}}}

//{{{
bool cDvbStream::cStream::toggle() {
// return true if enabling

  if (mRender) {
    // remove render
    auto mTemp = mRender;
    mRender = nullptr;
    delete mTemp;
    return false;
    }

  return true;
  }
//}}}
//}}}
//{{{  class cDvbStream::cPidInfo
//{{{
string cDvbStream::cPidInfo::getTypeName() const {

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
int cDvbStream::cPidInfo::addToBuffer (uint8_t* buf, int bufSize) {

  if (getBufUsed() + bufSize > mBufSize) {
    // realloc buffer to twice size
    mBufSize = mBufSize * 2;
    cLog::log (LOGINFO1, fmt::format("demux pid:{} realloc {}", mPid,mBufSize));

    auto ptrOffset = getBufUsed();
    mBuffer = (uint8_t*)realloc (mBuffer, mBufSize);
    mBufPtr = mBuffer + ptrOffset;
    }

  memcpy (mBufPtr, buf, bufSize);
  mBufPtr += bufSize;

  return getBufUsed();
  }
//}}}
//}}}
//{{{  class cDvbStream::cService
//{{{
cDvbStream::cService::cService (uint16_t sid) : mSid(sid) {
  mStreams[eVid].setLabel ("vid:");
  mStreams[eAud].setLabel ("aud:");
  mStreams[eAds].setLabel ("ad:");
  mStreams[eSub].setLabel ("");
  }
//}}}
//{{{
cDvbStream::cService::~cService() {

  delete mNowEpgItem;

  for (auto& epgItem : mEpgItemMap)
    delete epgItem.second;

  mEpgItemMap.clear();

  closeFile();
  }
//}}}

// get
//{{{
bool cDvbStream::cService::isEpgRecord (const string& title, tTimePoint startTime) {
// return true if startTime, title selected to record in epg

  auto it = mEpgItemMap.find (startTime);
  if (it != mEpgItemMap.end())
    if (title == it->second->getTitleString())
      if (it->second->getRecord())
        return true;

  return false;
  }
//}}}

//  sets
//{{{
void cDvbStream::cService::setAudStream (uint16_t pid, uint8_t streamTypeId) {

  if (mStreams[eAud].isDefined()) {
    if (pid != mStreams[eAud].getPid()) {
      // main audStream defined, new aud pid, try audio description eAds
      if (!mStreams[eAds].isDefined())
        mStreams[eAds].setPidTypeId (pid, streamTypeId);
      }
    }
  else
    mStreams[eAud].setPidTypeId (pid, streamTypeId);
  }
//}}}

//{{{
bool cDvbStream::cService::setNow (bool record, tTimePoint time, tDurationSeconds duration,
                                            const string& titleString, const string& infoString) {

  if (mNowEpgItem && (mNowEpgItem->getTime() == time))
    return false;

  delete mNowEpgItem;

  mNowEpgItem = new cEpgItem (true, record, time, duration, titleString, infoString);

  return true;
  }
//}}}
//{{{
bool cDvbStream::cService::setEpg (bool record, tTimePoint startTime, tDurationSeconds duration,
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

//{{{
void cDvbStream::cService::toggleStream (size_t streamType, uint16_t decoderMask) {

  cStream& stream = getStream (streamType);
  if (stream.toggle()) {
    switch (streamType) {
      case eVid :
        stream.setRender (new cVideoRender (getChannelName(), stream.getTypeId(), decoderMask));
        return;

      case eAud :
      case eAds :
        stream.setRender (new cAudioRender (getChannelName(), stream.getTypeId(), decoderMask));
        return;

      case eSub :
        stream.setRender (new cSubtitleRender (getChannelName(), stream.getTypeId(), decoderMask));
        return;
      }
    }
  }
//}}}
//{{{
void cDvbStream::cService::toggleAll (uint16_t decoderMask) {

  // improve to one on all off , if all off all on
  toggleStream (eVid, decoderMask);
  toggleStream (eAud, decoderMask);
  toggleStream (eSub, decoderMask);
  }
//}}}

// record
//{{{
bool cDvbStream::cService::openFile (const string& fileName, uint16_t tsid) {

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
void cDvbStream::cService::writePacket (uint8_t* ts, uint16_t pid) {
//  pes ts packet, save only recognised pids

  if (mFile &&
      ((pid == mStreams[eVid].getPid()) || (pid == mStreams[eAud].getPid()) || (pid == mStreams[eSub].getPid())))
    fwrite (ts, 1, 188, mFile);
  }
//}}}
//{{{
void cDvbStream::cService::closeFile() {

  if (mFile)
    fclose (mFile);

  mFile = nullptr;
  }
//}}}

// private:
//{{{
uint8_t* cDvbStream::cService::tsHeader (uint8_t* ts, uint16_t pid, uint8_t continuityCount) {

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
void cDvbStream::cService::writePat (uint16_t tsid) {

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
void cDvbStream::cService::writePmt() {

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

  *tsPtr++ = 0xE0 | ((mStreams[eVid].getPid() & 0x1F00) >> 8);
  *tsPtr++ = mStreams[eVid].getPid() & 0x00FF;

  *tsPtr++ = 0xF0;
  *tsPtr++ = 0; // program_info_length;

  // video es
  *tsPtr++ = mStreams[eVid].getTypeId(); // elementary stream_type
  *tsPtr++ = 0xE0 | ((mStreams[eVid].getPid() & 0x1F00) >> 8); // elementary_PID
  *tsPtr++ = mStreams[eVid].getPid() & 0x00FF;
  *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0;       // ES_info_length
  *tsPtr++ = 0 & 0x00FF;

  // audio es
  *tsPtr++ = mStreams[eAud].getTypeId(); // elementary stream_type
  *tsPtr++ = 0xE0 | ((mStreams[eAud].getPid() & 0x1F00) >> 8); // elementary_PID
  *tsPtr++ = mStreams[eAud].getPid() & 0x00FF;
  *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0;       // ES_info_length
  *tsPtr++ = 0 & 0x00FF;

  // subtitle es
  *tsPtr++ = mStreams[eSub].getTypeId(); // elementary stream_type
  *tsPtr++ = 0xE0 | ((mStreams[eSub].getPid() & 0x1F00) >> 8); // elementary_PID
  *tsPtr++ = mStreams[eSub].getPid() & 0x00FF;
  *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0;       // ES_info_length
  *tsPtr++ = 0 & 0x00FF;

  writeSection (ts, tsSectionStart, tsPtr);
  }
//}}}
//{{{
void cDvbStream::cService::writeSection (uint8_t* ts, uint8_t* tsSectionStart, uint8_t* tsPtr) {

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

// public:
//{{{
cDvbStream::cDvbStream (const cDvbMultiplex& dvbMultiplex, const string& recordRootName,
                        bool renderFirstService, uint16_t decoderOptions)
    : mDvbMultiplex(dvbMultiplex), mRecordRootName(recordRootName),
      mRenderFirstService(renderFirstService), mDecoderOptions(decoderOptions) {

  mDvbSource = new cDvbSource (dvbMultiplex.mFrequency, 0);
  }
//}}}

// gets
//{{{
string cDvbStream::getTdtTimeString() const {
  return date::format ("%T", date::floor<chrono::seconds>(mTdtTime));
  }
//}}}
//{{{
cDvbStream::cService* cDvbStream::getService (uint16_t sid) {

  auto it = mServiceMap.find (sid);
  return (it == mServiceMap.end()) ? nullptr : &it->second;
  }
//}}}

//{{{
void cDvbStream::toggleStream (cService& service, size_t streamType, uint16_t decoderMask) {

  lock_guard<mutex> lockGuard (mMutex);
  service.toggleStream (streamType, decoderMask);
  }
//}}}

// sources
//{{{
void cDvbStream::dvbSource (bool launchThread) {

  if (launchThread)
    thread([=]() { dvbSourceInternal (true); }).detach(); //,this
  else
    dvbSourceInternal (false);
  }
//}}}
//{{{
void cDvbStream::fileSource (bool launchThread, const string& fileName) {

  if (launchThread)
    thread ([=](){ fileSourceInternal (true, fileName); } ).detach();// ,this
  else
    fileSourceInternal (false, fileName);
  }
//}}}

// private:
//{{{
void cDvbStream::clear() {

  lock_guard<mutex> lockGuard (mMutex);

  mServiceMap.clear();
  mProgramMap.clear();
  mPidInfoMap.clear();

  mNumErrors = 0;

  mFirstTimeDefined = false;
  }
//}}}
//{{{
void cDvbStream::clearPidCounts() {

  for (auto& pidInfo : mPidInfoMap)
    pidInfo.second.clearCounts();
  }
//}}}
//{{{
void cDvbStream::clearPidContinuity() {

  for (auto& pidInfo : mPidInfoMap)
    pidInfo.second.clearContinuity();
  }
//}}}

//{{{
cDvbStream::cPidInfo& cDvbStream::getPidInfo (uint16_t pid) {
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
cDvbStream::cPidInfo* cDvbStream::getPsiPidInfo (uint16_t pid) {
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
void cDvbStream::foundService (cService& service) {

  if (mRenderFirstService && !mRenderingFirstService) {
    cLog::log (LOGINFO, fmt::format ("play service {}:{}", service.getSid(), service.getProgramPid()));
    service.toggleAll (mDecoderOptions);
    mRenderingFirstService = true;
    }
  }
//}}}

//{{{
void cDvbStream::startServiceProgram (cService* service, tTimePoint tdtTime,
                                               const string& programName,
                                               tTimePoint programStartTime, bool selected) {
// start recording service program

  // close prev program on this service
  lock_guard<mutex> lockGuard (mRecordFileMutex);
  service->closeFile();

  if ((selected || service->getChannelRecord() || mDvbMultiplex.mRecordAllChannels) &&
      service->getStream (eVid).isDefined() && (service->getStream (eVid).isDefined())) {
    string filePath = mRecordRootName +
                      service->getChannelRecordName() +
                      date::format ("%d %b %y %a %H.%M.%S ", date::floor<chrono::seconds>(tdtTime)) +
                      validFileString (programName, "<>:/|?*\"\'\\") +
                      ".ts";

    // record
    service->openFile (filePath, 0x1234);

    // gui
    mRecordPrograms.push_back (filePath);

    // log program start time,date filename
    string eitStartTime = date::format ("%H.%M.%S %a %d %b %y", date::floor<chrono::seconds>(programStartTime));
    cLog::log (LOGINFO, fmt::format ("{} {}", eitStartTime, filePath));
    }
  }
//}}}
//{{{
void cDvbStream::programPesPacket (uint16_t sid, uint16_t pid, uint8_t* ts) {
// send pes ts packet to sid service

  lock_guard<mutex> lockGuard (mRecordFileMutex);

  cService* service = getService (sid);
  if (service)
    service->writePacket (ts, pid);
  }
//}}}
//{{{
void cDvbStream::stopServiceProgram (cService* service) {
// stop recording service, never called

  lock_guard<mutex> lockGuard (mRecordFileMutex);
  service->closeFile();
  }
//}}}

//{{{
bool cDvbStream::processPes (eStreamType streamType, cPidInfo* pidInfo, bool skip) {

  cService* service = getService (pidInfo->getSid());
  if (service) {
    cStream& stream = service->getStream (streamType);
    stream.setPts (pidInfo->getPts());
    if (stream.isEnabled())
      return stream.getRender().processPes (
        pidInfo->mBuffer, pidInfo->getBufUsed(), pidInfo->getPts(), pidInfo->getDts(), skip);
    }

  //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " v - " + dec(pidInfo->getBufUsed());
  return false;
  }
//}}}

// parse
//{{{
void cDvbStream::parsePat (cPidInfo* pidInfo, uint8_t* buf) {
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
void cDvbStream::parseNit (cPidInfo* pidInfo, uint8_t* buf) {

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
void cDvbStream::parseSdt (cPidInfo* pidInfo, uint8_t* buf) {
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
            string name = cDvbUtils::getString (buf + sizeof(descrService) +
                                                ((descrService*)buf)->provider_name_length);
            cService* service = getService (sid);
            if (service) {
              if (service->getChannelName().empty()) {
                bool channelRecognised = false;
                string channelRecordName;
                size_t i = 0;
                for (auto& channelName : mDvbMultiplex.mChannels) {
                  if (name == channelName) {
                    channelRecognised = true;
                    if (i < mDvbMultiplex.mChannelRecordNames.size())
                      channelRecordName = mDvbMultiplex.mChannelRecordNames[i] +  " ";
                    break;
                    }
                  i++;
                  }
                service->setChannelName (name, channelRecognised, channelRecordName);

                cLog::log (LOGINFO, fmt::format ("SDT named sid:{} {} {} {}",
                                                 sid, name, channelRecognised ? "record" : "", channelRecordName));
                }
              }
            else
              cLog::log (LOGINFO, fmt::format ("SDT - before PMT - ignored {} {}", sid, name));

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
void cDvbStream::parseEit (cPidInfo* pidInfo, uint8_t* buf) {

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
          cService* service = getService (sid);
          if (service) {
            // known service
            auto startTime = chrono::system_clock::from_time_t (
              MjdToEpochTime (eitEvent->mjd) + BcdTimeToSeconds (eitEvent->start_time));
            tDurationSeconds duration (BcdTimeToSeconds (eitEvent->duration));

            // get title
            auto bufPtr = buf + sizeof(descrShortEvent) - 1;
            auto titleString = cDvbUtils::getString (bufPtr);

            // get info
            bufPtr += ((descrShortEvent*)buf)->event_name_length;
            string infoString = cDvbUtils::getString (bufPtr);

            if (now) {
              // now event
              bool running = (eitEvent->running_status == 0x04);
              if (running &&
                  !service->getChannelName().empty() &&
                  service->getProgramPid() &&
                  service->getStream (eVid).isDefined() &&
                  service->getStream (eAud).isDefined()) {
                  //(service->getSubPid())) {
                // now event for named service with valid pgmPid, vidPid, audPid, subPid
                if (service->setNow (service->isEpgRecord (titleString, startTime),
                                     startTime, duration, titleString, infoString)) {
                  // new now event
                  auto pidInfoIt = mPidInfoMap.find (service->getProgramPid());
                  if (pidInfoIt != mPidInfoMap.end())
                    // update service pgmPid infoStr with new now event
                    pidInfoIt->second.setInfoString (service->getChannelName() + " " + service->getNowTitleString());

                  // callback to override to start new serviceItem program
                  startServiceProgram (service, mTdtTime,
                                       titleString, startTime,
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
void cDvbStream::parseTdt (cPidInfo* pidInfo, uint8_t* buf) {

  sTdt* tdt = (sTdt*)buf;
  if (tdt->table_id == TID_TDT) {
    mTdtTime = chrono::system_clock::from_time_t (
                 MjdToEpochTime (tdt->utc_mjd) + BcdTimeToSeconds (tdt->utc_time));
    if (!mFirstTimeDefined) {
      mFirstTime = mTdtTime;
      mFirstTimeDefined = true;
      }

    pidInfo->setInfoString (date::format ("%T", date::floor<chrono::seconds>(mFirstTime)) +
                            " to " + getTdtTimeString());
    }
  }
//}}}
//{{{
void cDvbStream::parsePmt (cPidInfo* pidInfo, uint8_t* buf) {
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
    if (!getService (sid)) {
      // found new service, create cService
      cLog::log (LOGINFO, fmt::format ("create service {}", sid));
      cService& service = mServiceMap.emplace (sid, cService(sid)).first->second;

      service.setProgramPid (pidInfo->getPid());
      pidInfo->setSid (sid);

      buf += sizeof(sPmt);
      sectionLength -= 4;
      uint16_t programInfoLength = HILO (pmt->program_info_length);
      uint16_t streamLength = sectionLength - programInfoLength - sizeof(sPmt);

      buf += programInfoLength;
      while (streamLength > 0) {
        sPmtInfo* pmtInfo = (sPmtInfo*)buf;

        // set service elementary stream pids
        uint16_t esPid = HILO (pmtInfo->elementary_PID);
        cPidInfo& esPidInfo = getPidInfo (esPid);
        esPidInfo.setSid (sid);
        esPidInfo.setStreamTypeId (pmtInfo->stream_type);
        switch (esPidInfo.getStreamTypeId()) {
          case   2: // ISO 13818-2 video
          case  27: // H264 video
            service.getStream (eVid).setPidTypeId (esPid, esPidInfo.getStreamTypeId()); break;

          case   3: // ISO 11172-3 audio
          case   4: // ISO 13818-3 audio
          case  15: // ADTS AAC audio
          case  17: // LATM AAC audio
          case 129: // AC3 audio
            service.setAudStream (esPid, esPidInfo.getStreamTypeId()); break;

          case   6: // subtitle
            service.getStream (eSub).setPidTypeId (esPid, esPidInfo.getStreamTypeId()); break;

          case   5: // private mpeg2 tabled data - private
          case  11: // dsm cc u_n
          case  13: // dsm cc tabled data
          case 134: break;
          //{{{
          default:
            cLog::log (LOGERROR, fmt::format ("parsePmt - unknown stream sid:{} pid:{} streamTypeId:{}",
                                              sid, esPid, esPidInfo.getStreamTypeId()));
            break;
          //}}}
          }

        uint16_t loopLength = HILO (pmtInfo->ES_info_length);
        buf += sizeof(sPmtInfo);
        streamLength -= loopLength + sizeof(sPmtInfo);
        buf += loopLength;
        }
      foundService (service);
      }
    }
  }
//}}}
//{{{
int cDvbStream::parsePsi (cPidInfo* pidInfo, uint8_t* buf) {
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
int64_t cDvbStream::demux (uint8_t* tsBuf, int64_t tsBufSize, int64_t streamPos, bool skip) {
// demux from tsBuffer to tsBuffer + tsBufferSize, streamPos is offset into full stream of first packet
// decodePid1 = -1 use all

  if (skip)
    clearPidContinuity();

  uint8_t* ts = tsBuf;
  uint8_t* tsEnd = tsBuf + tsBufSize;

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
            else if (pidInfo->getStreamTypeId() == 5) {
              //{{{  mtd - do nothing
              }
              //}}}
            else if (pidInfo->getStreamTypeId() == 11) {
              //{{{  dsm - do nothing
              }
              //}}}
            else if (pidInfo->getStreamTypeId() == 134) {
              //{{{  ??? - do nothing
              }
              //}}}
            else {
              //{{{  pes
              programPesPacket (pidInfo->getSid(), pidInfo->getPid(), ts - 1);

              ts += headerBytes;
              tsBytesLeft -= headerBytes;

              if (payloadStart) {
                // start of payload for new pes buffer
                if ((*(uint32_t*)ts == 0xBD010000) || (*(uint32_t*)ts == 0xBE010000) ||
                    (*(uint32_t*)ts == 0xC0010000) || (*(uint32_t*)ts == 0xC1010000) || (*(uint32_t*)ts == 0xC2010000) ||
                    (*(uint32_t*)ts == 0xC4010000) || (*(uint32_t*)ts == 0xC6010000) || (*(uint32_t*)ts == 0xC8010000) ||
                    (*(uint32_t*)ts == 0xCA010000) || (*(uint32_t*)ts == 0xCC010000) || (*(uint32_t*)ts == 0xCE010000) ||
                    (*(uint32_t*)ts == 0xD0010000) || (*(uint32_t*)ts == 0xD2010000) || (*(uint32_t*)ts == 0xD4010000) ||
                    (*(uint32_t*)ts == 0xD6010000) || (*(uint32_t*)ts == 0xD8010000) || (*(uint32_t*)ts == 0xDA010000) ||
                    (*(uint32_t*)ts == 0xE0010000)) {
                  if (pidInfo->mBufPtr && pidInfo->getStreamTypeId()) {
                    switch (pidInfo->getStreamTypeId()) {
                      case 2:   // ISO 13818-2 video
                      case 27:  // HD vid
                        //{{{  send buffered video pes
                        if (processPes (eVid, pidInfo, skip)) // render took ownership of pes buffer
                          pidInfo->mBuffer = (uint8_t*)malloc (pidInfo->mBufSize);
                        skip = false;
                        break;
                        //}}}
                      case 3:   // ISO 11172-3 audio
                      case 4:   // ISO 13818-3 audio
                      case 15:  // aac adts
                      case 17:  // aac latm
                      case 129: // ac3
                        //{{{  send buffered audio pes
                        if (processPes ((*(uint32_t*)ts == 0xC1010000) ? eAds : eAud, pidInfo, skip))
                          pidInfo->mBuffer = (uint8_t*)malloc (pidInfo->mBufSize);
                        break;
                        //}}}
                      case 6:   // subtitle
                        //{{{  send buffered subtitle pes
                        if (processPes (eSub, pidInfo, skip))
                          pidInfo->mBuffer = (uint8_t*)malloc (pidInfo->mBufSize);
                        break;
                        //}}}
                      default: {
                        cLog::log (LOGINFO, fmt::format("unknown pid:{} streamTypeId:{}",pid,pidInfo->getStreamTypeId()));
                        }
                      }
                    }
                  // reset pes buffer pointer
                  pidInfo->mBufPtr = pidInfo->mBuffer;

                  // save ts streamPos for start of new pes buffer
                  pidInfo->mStreamPos = streamPos;

                  if (ts[7] & 0x80) // pts
                    pidInfo->setPts ((ts[7] & 0x80) ? cDvbUtils::getPts (ts+9) : -1);
                  if (ts[7] & 0x40) // dts
                    pidInfo->setDts ((ts[7] & 0x80) ? cDvbUtils::getPts (ts+14) : -1);

                  // skip past pesHeader
                  int pesHeaderBytes = 9 + ts[8];
                  ts += pesHeaderBytes;
                  tsBytesLeft -= pesHeaderBytes;
                  }
                else {
                  uint32_t nn = *(uint32_t*)ts;
                  cLog::log (LOGINFO, fmt::format("unrecognised pes header {} {:8x}",pid,nn));
                  }
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
      return tsEnd - tsBuf;
      }
      //}}}
    }

  return ts - tsBuf;
  }
//}}}

// sources
//{{{
void cDvbStream::dvbSourceInternal (bool launchThread) {

  if (!mDvbSource->ok()) {
    //{{{  error, return
    cLog::log (LOGERROR, fmt::format ("dvbSource - no dvbSource"));
    return;
    }
    //}}}

  if (launchThread)
    cLog::setThreadName ("grab");

  FILE* mFile = mDvbMultiplex.mRecordAllChannels ?
    fopen ((mRecordRootName + mDvbMultiplex.mName + ".ts").c_str(), "wb") : nullptr;

  #ifdef _WIN32
    mDvbSource->run();
    int64_t streamPos = 0;
    auto blockSize = 0;
    while (true) {
      auto ptr = mDvbSource->getBlockBDA (blockSize);
      if (blockSize) {
        //{{{  read and demux block
        if (mFile)
          fwrite (ptr, 1, blockSize, mFile);

        streamPos += demux (ptr, blockSize, streamPos, false);
        mDvbSource->releaseBlock (blockSize);

        mErrorString.clear();
        if (getNumErrors())
          mErrorString += fmt::format ("{}err", getNumErrors());
        if (streamPos < 1000000)
          mErrorString = fmt::format ("{}k", streamPos / 1000);
        else
          mErrorString = fmt::format ("{}m", streamPos / 1000000);
        }
        //}}}
      else
        this_thread::sleep_for (1ms);
      mSignalString = mDvbSource->getStatusString();
      }
  #endif

  #ifdef __linux__
    constexpr int kDvrReadBufferSize = 50 * 188;
    auto buffer = (uint8_t*)malloc (kDvrReadBufferSize);

    uint64_t streamPos = 0;
    while (true) {
      int bytesRead = mDvbSource->getBlock (buffer, kDvrReadBufferSize);
      if (bytesRead == 0)
        cLog::log (LOGINFO, fmt::format ("cDvb grabThread no bytes read"));
      else {
        // demux
        streamPos += demux (buffer, bytesRead, 0, false);
        if (mFile)
          fwrite (buffer, 1, bytesRead, mFile);

        // get status
        mSignalString = mDvbSource->getStatusString();

        // log if more errors
        bool show = getNumErrors() != mLastErrors;
        mLastErrors = getNumErrors();
        if (show)
          cLog::log (LOGINFO, fmt::format ("err:{} {}", getNumErrors(), mSignalString));
        }
      }
    free (buffer);
  #endif

  if (mFile)
    fclose (mFile);

  if (launchThread)
    cLog::log (LOGINFO, "exit");
  }
//}}}
//{{{
void cDvbStream::fileSourceInternal (bool launchThread, const string& fileName) {

  if (launchThread)
    cLog::setThreadName ("read");

  auto file = fopen (fileName.c_str(), "rb");
  if (!file) {
    //{{{  error, return
    cLog::log (LOGERROR, "no file " + fileName);
    return;
    }
    //}}}

  uint64_t streamPos = 0;
  auto blockSize = 188 * 8;
  auto buffer = (uint8_t*)malloc (blockSize);

  //int i = 0;
  bool run = true;
  while (run) {
    //i++;
    //if (!(i % 200)) // throttle read rate
    //  this_thread::sleep_for (20ms);

    size_t bytesRead = fread (buffer, 1, blockSize, file);
    if (bytesRead > 0)
      streamPos += demux (buffer, bytesRead, streamPos, false);
    else
      break;
    //mErrorStr = fmt::format ("{}", getNumErrors());
    }

  fclose (file);
  free (buffer);

  if (launchThread)
    cLog::log (LOGERROR, "exit");
  }
//}}}
