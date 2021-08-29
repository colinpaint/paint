// cJpegAnalyser.cpp - jpeg analyser - based on tiny jpeg decoder, jhead
#ifdef _WIN32
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include "cJpegAnalyse.h"

#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <string>
#include <stdio.h>
#include <chrono>
#include <functional>

#include "../utils/formatCore.h"
#include "../utils/date.h"

using namespace std;
//}}}
//{{{  tags
// exif
//{{{  tags x
// tags
#define TAG_INTEROP_INDEX          0x0001
#define TAG_INTEROP_VERSION        0x0002
//}}}
//{{{  tags 1xx
#define TAG_IMAGE_WIDTH            0x0100
#define TAG_IMAGE_LENGTH           0x0101
#define TAG_BITS_PER_SAMPLE        0x0102
#define TAG_COMPRESSION            0x0103
#define TAG_PHOTOMETRIC_INTERP     0x0106
#define TAG_FILL_ORDER             0x010A
#define TAG_DOCUMENT_NAME          0x010D
#define TAG_IMAGE_DESCRIPTION      0x010E
#define TAG_MAKE                   0x010F
#define TAG_MODEL                  0x0110
#define TAG_SRIP_OFFSET            0x0111
#define TAG_ORIENTATION            0x0112
#define TAG_SAMPLES_PER_PIXEL      0x0115
#define TAG_ROWS_PER_STRIP         0x0116
#define TAG_STRIP_BYTE_COUNTS      0x0117
#define TAG_X_RESOLUTION           0x011A
#define TAG_Y_RESOLUTION           0x011B
#define TAG_PLANAR_CONFIGURATION   0x011C
#define TAG_RESOLUTION_UNIT        0x0128
#define TAG_TRANSFER_FUNCTION      0x012D
#define TAG_SOFTWARE               0x0131
#define TAG_DATETIME               0x0132
#define TAG_ARTIST                 0x013B
#define TAG_WHITE_POINT            0x013E
#define TAG_PRIMARY_CHROMATICITIES 0x013F
#define TAG_TRANSFER_RANGE         0x0156
//}}}
//{{{  tags 2xx
#define TAG_JPEG_PROC              0x0200
#define TAG_THUMBNAIL_OFFSET       0x0201
#define TAG_THUMBNAIL_LENGTH       0x0202
#define TAG_Y_CB_CR_COEFFICIENTS   0x0211
#define TAG_Y_CB_CR_SUB_SAMPLING   0x0212
#define TAG_Y_CB_CR_POSITIONING    0x0213
#define TAG_REFERENCE_BLACK_WHITE  0x0214
//}}}
//{{{  tags 1xxx
#define TAG_RELATED_IMAGE_WIDTH    0x1001
#define TAG_RELATED_IMAGE_LENGTH   0x1002
//}}}
//{{{  tags 8xxx
#define TAG_CFA_REPEAT_PATTERN_DIM 0x828D
#define TAG_CFA_PATTERN1           0x828E
#define TAG_BATTERY_LEVEL          0x828F
#define TAG_COPYRIGHT              0x8298
#define TAG_EXPOSURETIME           0x829A
#define TAG_FNUMBER                0x829D
#define TAG_IPTC_NAA               0x83BB
#define TAG_EXIF_OFFSET            0x8769
#define TAG_INTER_COLOR_PROFILE    0x8773
#define TAG_EXPOSURE_PROGRAM       0x8822
#define TAG_SPECTRAL_SENSITIVITY   0x8824
#define TAG_GPSINFO                0x8825
#define TAG_ISO_EQUIVALENT         0x8827
#define TAG_OECF                   0x8828
//}}}
//{{{  tags 9xxx
#define TAG_EXIF_VERSION           0x9000
#define TAG_DATETIME_ORIGINAL      0x9003
#define TAG_DATETIME_DIGITIZED     0x9004
#define TAG_COMPONENTS_CONFIG      0x9101
#define TAG_CPRS_BITS_PER_PIXEL    0x9102
#define TAG_SHUTTERSPEED           0x9201
#define TAG_APERTURE               0x9202
#define TAG_BRIGHTNESS_VALUE       0x9203
#define TAG_EXPOSURE_BIAS          0x9204
#define TAG_MAXAPERTURE            0x9205
#define TAG_SUBJECT_DISTANCE       0x9206
#define TAG_METERING_MODE          0x9207
#define TAG_LIGHT_SOURCE           0x9208
#define TAG_FLASH                  0x9209
#define TAG_FOCALLENGTH            0x920A
#define TAG_SUBJECTAREA            0x9214
#define TAG_MAKER_NOTE             0x927C
#define TAG_USERCOMMENT            0x9286
#define TAG_SUBSEC_TIME            0x9290
#define TAG_SUBSEC_TIME_ORIG       0x9291
#define TAG_SUBSEC_TIME_DIG        0x9292

#define TAG_WINXP_TITLE            0x9c9b // Windows XP - not part of exif standard.
#define TAG_WINXP_COMMENT          0x9c9c // Windows XP - not part of exif standard.
#define TAG_WINXP_AUTHOR           0x9c9d // Windows XP - not part of exif standard.
#define TAG_WINXP_KEYWORDS         0x9c9e // Windows XP - not part of exif standard.
#define TAG_WINXP_SUBJECT          0x9c9f // Windows XP - not part of exif standard.
//}}}
//{{{  tags Axxx
#define TAG_FLASH_PIX_VERSION      0xA000
#define TAG_COLOR_SPACE            0xA001
#define TAG_PIXEL_X_DIMENSION      0xA002
#define TAG_PIXEL_Y_DIMENSION      0xA003
#define TAG_RELATED_AUDIO_FILE     0xA004
#define TAG_INTEROP_OFFSET         0xA005
#define TAG_FLASH_ENERGY           0xA20B
#define TAG_SPATIAL_FREQ_RESP      0xA20C
#define TAG_FOCAL_PLANE_XRES       0xA20E
#define TAG_FOCAL_PLANE_YRES       0xA20F
#define TAG_FOCAL_PLANE_UNITS      0xA210
#define TAG_SUBJECT_LOCATION       0xA214
#define TAG_EXPOSURE_INDEX         0xA215
#define TAG_SENSING_METHOD         0xA217
#define TAG_FILE_SOURCE            0xA300
#define TAG_SCENE_TYPE             0xA301
#define TAG_CFA_PATTERN            0xA302
#define TAG_CUSTOM_RENDERED        0xA401
#define TAG_EXPOSURE_MODE          0xA402
#define TAG_WHITEBALANCE           0xA403
#define TAG_DIGITALZOOMRATIO       0xA404
#define TAG_FOCALLENGTH_35MM       0xA405
#define TAG_SCENE_CAPTURE_TYPE     0xA406
#define TAG_GAIN_CONTROL           0xA407
#define TAG_CONTRAST               0xA408
#define TAG_SATURATION             0xA409
#define TAG_SHARPNESS              0xA40A
#define TAG_DISTANCE_RANGE         0xA40C
#define TAG_IMAGE_UNIQUE_ID        0xA420
//}}}
//{{{  format
#define FMT_BYTE       1
#define FMT_STRING     2
#define FMT_USHORT     3
#define FMT_ULONG      4
#define FMT_URATIONAL  5
#define FMT_SBYTE      6
#define FMT_UNDEFINED  7
#define FMT_SSHORT     8
#define FMT_SLONG      9
#define FMT_SRATIONAL 10
#define FMT_SINGLE    11
#define FMT_DOUBLE    12
//}}}
//}}}

namespace {
  const int kBytesPerFormat[] = { 0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8 };
  const char* kWeekDay[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  }

// cJpegAnalyse
//{{{
bool cJpegAnalyse::readHeader (uTagLambda jpegTagLambda, uTagLambda exifTagLambda) {

  mJpegTagLambda = jpegTagLambda;
  mExifTagLambda = exifTagLambda;

  mThumbOffset = 0;
  mThumbBytes = 0;

  // read first word, must be SOI marker
  resetReadBytes();
  uint8_t* startPtr = getReadPtr();
  uint8_t* readPtr = readBytes (2);
  if (!readPtr)
    return false;
  if ((readPtr[0] != 0xFF) || (readPtr[1] != 0xD8))
    return false;
  mJpegTagLambda ("SOI", startPtr, 0, 2);

  uint32_t offset = 2;
  while (true) {
    startPtr = getReadPtr();
    uint32_t startOffset = offset;
    //{{{  read marker,length
    readPtr = readBytes (4);
    if (!readPtr)
      return false;

    uint16_t marker = (readPtr[0] << 8) | readPtr[1];
    uint16_t length = (readPtr[2] << 8) | readPtr[3];
    if (((marker >> 8) != 0xFF) || (length <= 2))
      return false;

    length -= 2;
    offset += 4;
    //}}}
    //{{{  read marker body
    readPtr = readBytes (length);
    if (!readPtr)
      return false;

    offset += length;
    //}}}
    switch (marker & 0xFF) {
      //{{{
      case 0xC0: // SOF
        if (!parseSOF (readPtr, length))
          return false;

        mJpegTagLambda (fmt::format ("SOF - image:{}x{} mcu:{}x{} QtableId:{}:{}:{}:{}",
                                     mWidth, mHeight, mSx, mSy, mQtableId[0], mQtableId[1], mQtableId[2], mQtableId[3]),
                        startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xC1: // SOF1
        mJpegTagLambda ("SOF1", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xC2: // SOF2
        mJpegTagLambda ("SOF2", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xC3: // SOF3
        mJpegTagLambda ("SOF3F", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xC4: // HFT
        mJpegTagLambda ("HFT", startPtr, startOffset, length);
        if (!parseHFT (readPtr, length))
          return false;
        break;
      //}}}
      //{{{
      case 0xC5: // SOF5
        mJpegTagLambda ("SOF5", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xC6: // SOF6
        mJpegTagLambda ("SOF6", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xC7: // SOF7
        mJpegTagLambda ("SOF7", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xC9: // SOF9
        mJpegTagLambda ("SOF9", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xCA: // SOF10
        mJpegTagLambda ("SOF10", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xCB: // SOF11
        mJpegTagLambda ("SOF11", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xCD: // SOF13
        mJpegTagLambda ("SOF13", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xCE: // SOF14
        mJpegTagLambda ("SOF14", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xCF: // SOF15
        mJpegTagLambda ("SOF15", startPtr, startOffset, length);
        break;
      //}}}

      //{{{
      case 0xD9: // EOI
        mJpegTagLambda ("EOI", startPtr, startOffset, length);
        return true;
      //}}}
      //{{{
      case 0xDA: // SOS
        //  parseSOS
        mJpegTagLambda ("SOS", startPtr, startOffset, length);

        if (!parseSOS (readPtr, length))
          return false;
        break;
      //}}}
      //{{{
      case 0xDB: // DQT
        mJpegTagLambda ("DQT", startPtr, startOffset, length);
        if (!parseDQT (readPtr, length))
          return false;
        break;
      //}}}
      //{{{
      case 0xDD: // DRI
        parseDRI (readPtr, length);
        mJpegTagLambda (fmt::format ("DRI {}", mNumRst), startPtr, startOffset, length);
        break;
      //}}}

      //{{{
      case 0xE0: // APP0
        mJpegTagLambda ("APP0", startPtr, startOffset, length);
        break;
      //}}}
      //{{{
      case 0xE1: // APP1
        mJpegTagLambda ("APP1", startPtr, startOffset, length);
        parseAPP (readPtr, length);
        break;
      //}}}

      //{{{
      default: // unknown
        mJpegTagLambda (fmt::format ("{:02x} unknown", marker & 0xFF), startPtr, startOffset, length);
        break;
      //}}}
      }
    }

  return false;
  }
//}}}

// private:
//{{{
uint16_t cJpegAnalyse::getExifWord (uint8_t* ptr, bool intelEndian) {

  return *(ptr + (intelEndian ? 1 : 0)) << 8 |  *(ptr + (intelEndian ? 0 : 1));
  }
//}}}
//{{{
uint32_t cJpegAnalyse::getExifLong (uint8_t* ptr, bool intelEndian) {

  return getExifWord (ptr + (intelEndian ? 2 : 0), intelEndian) << 16 |
         getExifWord (ptr + (intelEndian ? 0 : 2), intelEndian);
  }
//}}}
//{{{
float cJpegAnalyse::getExifSignedRational (uint8_t* ptr, bool intelEndian, uint32_t& numerator, uint32_t& denominator) {

  numerator = getExifLong (ptr, intelEndian);
  denominator = getExifLong (ptr+4, intelEndian);
  return (denominator == 0) ? 0 : (float)numerator / (float)denominator;
  }
//}}}

//{{{
void cJpegAnalyse::getExifGpsInfo (uint8_t* ptr, uint8_t* offsetBasePtr, bool intelEndian) {

  auto numDirectoryEntries = getExifWord (ptr, intelEndian);
  ptr += 2;

  for (auto entry = 0; entry < numDirectoryEntries; entry++) {
    auto tag = getExifWord (ptr, intelEndian);
    auto format = getExifWord (ptr+2, intelEndian);
    auto components = getExifLong (ptr+4, intelEndian);
    auto offset = getExifLong (ptr+8, intelEndian);
    auto bytes = components * kBytesPerFormat[format];
    auto valuePtr = (bytes <= 4) ? ptr+8 : offsetBasePtr + offset;
    ptr += 12;

    uint32_t numerator;
    uint32_t denominator;
    switch (tag) {
      //{{{
      case 0x00:  // version
        mExifGpsInfo.mVersion = getExifLong (valuePtr, intelEndian);
        break;
      //}}}
      //{{{
      case 0x01:  // latitude ref
        mExifGpsInfo.mLatitudeRef = valuePtr[0];
        break;
      //}}}
      //{{{
      case 0x02:  // latitude
        mExifGpsInfo.mLatitudeDeg = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
        mExifGpsInfo.mLatitudeMin = getExifSignedRational (valuePtr + 8, intelEndian, numerator, denominator);
        mExifGpsInfo.mLatitudeSec = getExifSignedRational (valuePtr + 16, intelEndian, numerator, denominator);
        break;
      //}}}
      //{{{
      case 0x03:  // longitude ref
        mExifGpsInfo.mLongitudeRef = valuePtr[0];
        break;
      //}}}
      //{{{
      case 0x04:  // longitude
        mExifGpsInfo.mLongitudeDeg = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
        mExifGpsInfo.mLongitudeMin = getExifSignedRational (valuePtr + 8, intelEndian, numerator, denominator);
        mExifGpsInfo.mLongitudeSec = getExifSignedRational (valuePtr + 16, intelEndian, numerator, denominator);
        break;
      //}}}
      //{{{
      case 0x05:  // altitude ref
        mExifGpsInfo.mAltitudeRef = valuePtr[0];
        break;
      //}}}
      //{{{
      case 0x06:  // altitude
        mExifGpsInfo.mAltitude = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
        break;
      //}}}
      //{{{
      case 0x07:  // timeStamp
        mExifGpsInfo.mHour = getExifSignedRational (valuePtr, intelEndian, numerator, denominator),
        mExifGpsInfo.mMinute = getExifSignedRational (valuePtr + 8, intelEndian, numerator, denominator),
        mExifGpsInfo.mSecond = getExifSignedRational (valuePtr + 16, intelEndian, numerator, denominator);
        break;
      //}}}
      //{{{
      case 0x08:  // satellites
        printf ("TAG_gps_Satellites %s \n", valuePtr);
        break;
      //}}}
      //{{{
      case 0x0B:  // dop
        printf ("TAG_gps_DOP %f\n", getExifSignedRational (valuePtr, intelEndian, numerator, denominator));
        break;
      //}}}
      //{{{
      case 0x10:  // direction ref
        mExifGpsInfo.mImageDirectionRef = valuePtr[0];
        break;
      //}}}
      //{{{
      case 0x11:  // direction
        mExifGpsInfo.mImageDirection = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
        break;
      //}}}
      //{{{
      case 0x12:  // datum
        mExifGpsInfo.mDatum = (char*)valuePtr;
        break;
      //}}}
      //{{{
      case 0x1D:  // date
        mExifGpsInfo.mDate = (char*)valuePtr;
        break;
      //}}}
      //{{{
      case 0x1B:
        //printf ("TAG_gps_ProcessingMethod\n");
        break;
      //}}}
      //{{{
      case 0x1C:
        //printf ("TAG_gps_AreaInformation\n");
        break;
      //}}}
      case 0x09: printf ("TAG_gps_status\n"); break;
      case 0x0A: printf ("TAG_gps_MeasureMode\n"); break;
      case 0x0C: printf ("TAG_gps_SpeedRef\n"); break;
      case 0x0D: printf ("TAG_gps_Speed\n"); break;
      case 0x0E: printf ("TAG_gps_TrackRef\n"); break;
      case 0x0F: printf ("TAG_gps_Track\n"); break;
      case 0x13: printf ("TAG_gps_DestLatitudeRef\n"); break;
      case 0x14: printf ("TAG_gps_DestLatitude\n"); break;
      case 0x15: printf ("TAG_gps_DestLongitudeRef\n"); break;
      case 0x16: printf ("TAG_gps_DestLongitude\n"); break;
      case 0x17: printf ("TAG_gps_DestBearingRef\n"); break;
      case 0x18: printf ("TAG_gps_DestBearing\n"); break;
      case 0x19: printf ("TAG_gps_DestDistanceRef\n"); break;
      case 0x1A: printf ("TAG_gps_DestDistance\n"); break;
      case 0x1E: printf ("TAG_gps_Differential\n"); break;
      default: printf ("unknown gps tag %x\n", tag); break;
      }
    }
  }
//}}}
//{{{
string cJpegAnalyse::getExifTime (uint8_t* ptr, struct tm* tmPtr) {
// Convert exif time to Unix time structure

  // Check for format: YYYY:MM:DD HH:MM:SS format.
  // Date and time normally separated by a space, but also seen a ':' there, so
  // skip the middle space with '%*c' so it can be any character.
  tmPtr->tm_wday = -1;
  tmPtr->tm_sec = 0;
  sscanf ((char*)ptr, "%d%*c%d%*c%d%*c%d:%d:%d",
          &tmPtr->tm_year, &tmPtr->tm_mon, &tmPtr->tm_mday,
          &tmPtr->tm_hour, &tmPtr->tm_min, &tmPtr->tm_sec);

  tmPtr->tm_isdst = -1;
  tmPtr->tm_mon -= 1;     // Adjust for unix zero-based months
  tmPtr->tm_year -= 1900; // Adjust for year starting at 1900

  // find day of week
  mktime (tmPtr);

  string str = (char*)ptr;
  if ((tmPtr->tm_wday >= 0) && (tmPtr->tm_wday <= 6)) {
    str += " ";
    str += kWeekDay[tmPtr->tm_wday];
    }

  return str;
  }
//}}}

//{{{
void cJpegAnalyse::parseExifDirectory (uint8_t* offsetBasePtr, uint8_t* ptr, bool intelEndian) {

  auto numDirectoryEntries = getExifWord (ptr, intelEndian);
  ptr += 2;

  for (auto entry = 0; entry < numDirectoryEntries; entry++) {
    uint8_t* startPtr = ptr;
    auto tag = getExifWord (ptr, intelEndian);
    auto format = getExifWord (ptr+2, intelEndian);
    auto components = getExifLong (ptr+4, intelEndian);
    auto offset = getExifLong (ptr+8, intelEndian);
    auto bytes = components * kBytesPerFormat[format];
    auto valuePtr = (bytes <= 4) ? ptr+8 : offsetBasePtr + offset;
    ptr += 12;

    uint32_t numerator;
    uint32_t denominator;
    switch (tag) {
      case TAG_EXIF_OFFSET:
        parseExifDirectory (offsetBasePtr, offsetBasePtr + offset, intelEndian);
        break;
      //{{{
      case TAG_ORIENTATION:
        mExifInfo.mExifOrientation = offset;
        mExifTagLambda (fmt::format ("orientation {}", mExifInfo.mExifOrientation), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_APERTURE:
        mExifInfo.mExifAperture = (float)exp(getExifSignedRational (valuePtr, intelEndian, numerator, denominator)*log(2)*0.5);
        mExifTagLambda (fmt::format ("aperture {}", mExifInfo.mExifAperture), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_FOCALLENGTH:
        mExifInfo.mExifFocalLength = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
        mExifTagLambda (fmt::format ("focalLength {}", mExifInfo.mExifFocalLength), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_EXPOSURETIME:
        mExifInfo.mExifExposure = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
        mExifTagLambda (fmt::format ("exposure {}", mExifInfo.mExifExposure), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_MAKE:
        if (mExifInfo.mExifMake.empty()) {
          mExifInfo.mExifMake = (char*)valuePtr;
          mExifTagLambda (fmt::format ("make {}", mExifInfo.mExifMake), startPtr, 0, bytes);
          }
        break;
      //}}}
      //{{{
      case TAG_MODEL:
        if (mExifInfo.mExifModel.empty()) {
          mExifInfo.mExifModel = (char*)valuePtr;
          mExifTagLambda (fmt::format ("model {}", mExifInfo.mExifModel), startPtr, 0, bytes);
          }
        break;
      //}}}
      case TAG_DATETIME:
      case TAG_DATETIME_ORIGINAL:
      //{{{
      case TAG_DATETIME_DIGITIZED:
        if (mExifInfo.mExifTimeString.empty()) {
          mExifInfo.mExifTimeString = getExifTime (valuePtr, &mExifInfo.mExifTm);
          mExifTagLambda (fmt::format ("dateTime {}", mExifInfo.mExifTimeString), startPtr, 0, bytes);
          }
        break;
      //}}}
      //{{{
      case TAG_THUMBNAIL_OFFSET:
        mThumbOffset = offset;
        mExifTagLambda (fmt::format ("thumbOffset {}", mThumbOffset), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_THUMBNAIL_LENGTH:
        mThumbBytes = offset;
        mExifTagLambda (fmt::format ("thumbLength {}", mThumbBytes), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_GPSINFO:
        getExifGpsInfo (offsetBasePtr + offset, offsetBasePtr, intelEndian);
        mExifTagLambda (fmt::format ("gps"), startPtr, 0, bytes);
        break;
      //}}}
      //case TAG_MAXAPERTURE:
      //  printf ("TAG_MAXAPERTURE\n"); break;
      //case TAG_SHUTTERSPEED:
      //  printf ("TAG_SHUTTERSPEED\n"); break;
      default:;
      //  printf ("TAG %x\n", tag);
      }
    }

  auto extraDirectoryOffset = getExifLong (ptr, intelEndian);
  if (extraDirectoryOffset > 4)
    parseExifDirectory (offsetBasePtr, offsetBasePtr + extraDirectoryOffset, intelEndian);
  }
//}}}
//{{{
bool cJpegAnalyse::parseAPP (uint8_t* ptr, unsigned length) {
// find and read APP1 EXIF marker, return true if thumb, valid mThumbBuffer, mThumbLength

  (void)length;

  // check exifId
  if (getExifLong (ptr, false) != 0x45786966)
    return false;
  ptr += 4;

  // check 0 word
  if (getExifWord (ptr, false) != 0x0000)
    return false;
  ptr += 2;

  auto offsetBasePtr = ptr;

  // endian
  bool intelEndian = getExifWord (ptr, false) == 0x4949;
  ptr += 2;

  // 002a word
  if (getExifWord (ptr, intelEndian) != 0x002a)
    return false;
  ptr += 2;

  // firstOffset 8
  if (getExifLong (ptr, intelEndian) != 8)
    return false;
  ptr += 4;

  parseExifDirectory (offsetBasePtr, ptr, intelEndian);

  mThumbOffset += 6 + 6; // SOImarker(2), APP1marker(2), APP1length(2), EXIF00marker(6)
  return mThumbBytes > 0;
  }
//}}}
//{{{
bool cJpegAnalyse::parseDQT (uint8_t* ptr, unsigned length) {
// create de-quantization and prescaling tables with a DQT segment
  (void)ptr;
  (void)length;
  return true;
  }
//}}}
//{{{
bool cJpegAnalyse::parseHFT (uint8_t* ptr, unsigned length) {
// Create huffman code tables with a DHT segment
  (void)ptr;
  (void)length;
  return true;
  }
//}}}
//{{{
bool cJpegAnalyse::parseDRI (uint8_t* ptr, unsigned length) {
  mNumRst = ptr[0] << 8 | ptr[1];
  return length >= 2;
  }
//}}}
//{{{
bool cJpegAnalyse::parseSOF (uint8_t* ptr, unsigned length) {

  (void)length;

  mHeight = ptr[1] << 8 | ptr[2];
  mWidth =  ptr[3] << 8 | ptr[4];

  // only Y/Cb/Cr format
  if (ptr[5] != 3)
    return false;

  // Check three image components
  for (auto i = 0; i < 3; i++) {
    // Get sampling factor
    uint8_t b = ptr[7 + 3 * i];
    if (!i) {
      // Y component
      if (b != 0x11 && b != 0x22 && b != 0x21) //  Check sampling factor, only 4:4:4, 4:2:0 or 4:2:2
        return false;

      // Size of MCU [blocks]
      mSx = b >> 4;
      mSy = b & 15;
      }
    else if (b != 0x11) // Cb/Cr component
      return false;

    // Get dequantizer table ID for this component
    b = ptr[8 + 3 * i];
    if (b > 3)
      return false;

    mQtableId[i] = b;
    }

  return true;
  }
//}}}
//{{{
bool cJpegAnalyse::parseSOS (uint8_t* ptr, unsigned length) {

  (void)length;

  if (!mWidth || !mHeight)
    return false;

  if (ptr[0] != 3)
    return false;

  return true;
  }
//}}}

// cJpegAnalyse::cExifGpsInfo
//{{{
string cJpegAnalyse::cExifGpsInfo::getString() {

  string str = fmt::format ("{} {} {} {} {} {} {} {} {}", mDatum,
                                mLatitudeDeg,mLatitudeRef,mLatitudeMin,mLatitudeSec,
                                mLongitudeDeg,mLongitudeRef,mLongitudeMin,mLongitudeSec);
  str += fmt::format ("{} {}", mAltitude, mAltitudeRef);
  str += fmt::format ("{} {}", mImageDirection, mImageDirectionRef);
  str += fmt::format ("{} {} {} {}", mDate,mHour,mMinute,mSecond);
  return str;
  }
//}}}
#endif
