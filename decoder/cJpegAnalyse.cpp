// cJpegAnalyser.cpp
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

#include "fmt/core.h"
#include "../common/date.h"

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

// public:
//{{{
bool cJpegAnalyse::analyse (tCallback callback) {

  mCallback = callback;
  resetRead();

  // read first word, must be SOI marker
  uint8_t* markerPtr = getReadPtr();
  uint8_t* readPtr = readBytes (2);
  if (!readPtr)
    return false;
  if ((readPtr[0] != 0xFF) || (readPtr[1] != 0xD8))
    return false;
  mCallback (0, "SOI startOfInformation", markerPtr, 0, 2);
  uint32_t offset = 2;

  while (true) {
    //{{{  read marker,length
    markerPtr = getReadPtr();
    uint32_t markerOffset = offset;

    readPtr = readBytes (4);
    if (!readPtr) {
      mCallback (0, "no marker ", markerPtr, markerOffset, 0);
      return false;
      }

    uint16_t marker = (readPtr[0] << 8) | readPtr[1];
    uint16_t length = (readPtr[2] << 8) | readPtr[3];
    if (((marker >> 8) != 0xFF) || (length <= 2)) {
      mCallback (0, "bad marker ", markerPtr, markerOffset, 0);
      return false;
      }

    length -= 2;
    offset += 4;

    //  read marker body
    readPtr = readBytes (length);
    if (!readPtr) {
      mCallback (0, "markerBody short", markerPtr, markerOffset, length);
      return false;
      }

    offset += length;
    //}}}
    switch (marker) {
      //{{{
      case 0xFFC0: // SOF
        parseSOF ("SOF baseDCT", markerPtr, markerOffset, readPtr, length);
        break;
      //}}}
      //{{{
      case 0xFFC1: // SOF1
        mCallback (0, "SOF1 extSeqDCT", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFC2: // SOF2
        parseSOF ("SOF2 progDCT", markerPtr, markerOffset, readPtr, length);
        break;
      //}}}
      //{{{
      case 0xFFC3: // SOF3
        mCallback (0, "SOF3F", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFC4: // HFT
        parseHFT ("HFT huffmanTable", markerPtr, markerOffset, readPtr, length);
        break;
      //}}}

      //{{{
      case 0xFFC5: // SOF5
        mCallback (0, "SOF5", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFC6: // SOF6
        mCallback (0, "SOF6", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFC7: // SOF7
        mCallback (0, "SOF7", markerPtr, markerOffset, length);
        break;
      //}}}

      //{{{
      case 0xFFC9: // SOF9
        mCallback (0, "SOF9", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFCA: // SOF10
        mCallback (0, "SOF10", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFCB: // SOF11
        mCallback (0, "SOF11 lossless", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFCC: // DAC
        mCallback (0, "DAC arithmeticCoding", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFCD: // SOF13
        mCallback (0, "SOF13", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFCE: // SOF14
        mCallback (0, "SOF14", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFCF: // SOF15
        mCallback (0, "SOF15", markerPtr, markerOffset, length);
        break;
      //}}}

      //{{{
      case 0xFFD9: // EOI
        mCallback (0, "EOI", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFDB: // DQT
        parseDQT ("DQT quantisationTable ", markerPtr, markerOffset, readPtr, length);
        break;
      //}}}
      //{{{
      case 0xFFDC: // DNL
        mCallback (0, "DNL  dunmberLines", markerPtr, markerOffset, length);
        break;
      //}}}
      //{{{
      case 0xFFDD: // DRI
        parseDRI ("DRI resetInterval", markerPtr, markerOffset, readPtr, length);
        break;
      //}}}
      //{{{
      case 0xFFDE: // DHP
        mCallback (0, "DHP- hierachialProgressive", markerPtr, markerOffset, length);
        break;
      //}}}

      //{{{
      case 0xFFE0: // APP0
        parseAPP0 ("APP0", markerPtr, markerOffset, readPtr, length);
        break;
      //}}}
      //{{{
      case 0xFFE1: // APP1
        parseAPP1 ("APP1", markerPtr, markerOffset, readPtr, length);
        break;
      //}}}
      //{{{
      case 0xFFE2: // APP2
        parseAPP2 ("APP2", markerPtr, markerOffset, readPtr, length);
        break;
      //}}}
      //{{{
      case 0xFFED: // APP14
        parseAPP14 ("APP14", markerPtr, markerOffset, readPtr, length);
        break;
      //}}}

      //{{{
      case 0xFFDA: // SOS, marks end until full body decode
        parseSOS ("SOS startOfScan", markerPtr, markerOffset, readPtr, length);

        mCallback (0, "body", readPtr, markerOffset, getReadBytesLeft());
        return true;
      //}}}
      //{{{
      default: // unknown
        mCallback (0, fmt::format ("{:04x} marker", marker), markerPtr, markerOffset, length);
        break;
      //}}}
      }
    }

  return false;
  }
//}}}

// exif parse utils
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
    uint16_t tag = getExifWord (ptr, intelEndian);
    uint16_t format = getExifWord (ptr+2, intelEndian);
    uint32_t components = getExifLong (ptr+4, intelEndian);
    uint32_t offset = getExifLong (ptr+8, intelEndian);
    uint32_t bytes = components * kBytesPerFormat[format];
    uint8_t* valuePtr = (bytes <= 4) ? ptr+8 : offsetBasePtr + offset;
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

// exif tag parser
//{{{
void cJpegAnalyse::parseExifDirectory (uint8_t* offsetBasePtr, uint8_t* ptr, bool intelEndian) {

  uint16_t numDirectoryEntries = getExifWord (ptr, intelEndian);
  ptr += 2;

  for (uint16_t entry = 0; entry < numDirectoryEntries; entry++) {
    uint8_t* startPtr = ptr;
    uint16_t tag = getExifWord (ptr, intelEndian);
    uint16_t format = getExifWord (ptr+2, intelEndian);
    uint32_t components = getExifLong (ptr+4, intelEndian);
    uint32_t offset = getExifLong (ptr+8, intelEndian);
    uint32_t bytes = components * kBytesPerFormat[format];
    uint8_t* valuePtr = (bytes <= 4) ? ptr+8 : offsetBasePtr + offset;
    ptr += 12;

    switch (tag) {
      //{{{
      case TAG_EXIF_OFFSET:
        parseExifDirectory (offsetBasePtr, offsetBasePtr + offset, intelEndian);
        break;
      //}}}
      //{{{
      case TAG_ORIENTATION:
        mExifInfo.mExifOrientation = offset;
        mCallback (1, fmt::format ("exifOrientation {}", mExifInfo.mExifOrientation), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_APERTURE: {
        uint32_t numerator;
        uint32_t denominator;
        mExifInfo.mExifAperture = (float)exp(getExifSignedRational (valuePtr, intelEndian, numerator, denominator)*log(2)*0.5);
        mCallback (1, fmt::format ("exifAperture {}", mExifInfo.mExifAperture), startPtr, 0, bytes);
        break;
        }
      //}}}
      //{{{
      case TAG_FOCALLENGTH: {
        uint32_t numerator;
        uint32_t denominator;
        mExifInfo.mExifFocalLength = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
        mCallback (1, fmt::format ("exifFocalLength {}", mExifInfo.mExifFocalLength), startPtr, 0, bytes);
        break;
        }
      //}}}
      //{{{
      case TAG_EXPOSURETIME: {
        uint32_t numerator;
        uint32_t denominator;
        mExifInfo.mExifExposure = getExifSignedRational (valuePtr, intelEndian, numerator, denominator);
        mCallback (1, fmt::format ("exifExposure {}", mExifInfo.mExifExposure), startPtr, 0, bytes);
        break;
        }
      //}}}
      //{{{
      case TAG_COMPRESSION:
        mCallback (1, "exifCompression", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_MAKE:
        mExifInfo.mExifMake = (char*)valuePtr;
        mCallback (1, fmt::format ("exifMake {}", mExifInfo.mExifMake), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_MODEL:
        mExifInfo.mExifModel = (char*)valuePtr;
        mCallback (1, fmt::format ("exifModel {}", mExifInfo.mExifModel), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_DATETIME:
        mExifInfo.mExifTimeString = getExifTime (valuePtr, &mExifInfo.mExifTm);
        mCallback (1, fmt::format ("exifDateTime {}", mExifInfo.mExifTimeString), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_DATETIME_ORIGINAL:
        mExifInfo.mExifTimeOriginalString = getExifTime (valuePtr, &mExifInfo.mExifTmOriginal);
        mCallback (1, fmt::format ("exifDateTimeOriginal {}", mExifInfo.mExifTimeString), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_DATETIME_DIGITIZED:
        mExifInfo.mExifTimeDigitizedString = getExifTime (valuePtr, &mExifInfo.mExifTmDigitized);
        mCallback (1, fmt::format ("exifDateTimeDigitized {}", mExifInfo.mExifTimeString), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_THUMBNAIL_OFFSET:
        mExifThumbOffset = offset;
        mCallback (1, fmt::format ("exifThumbOffset {}", mExifThumbOffset), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_THUMBNAIL_LENGTH:
        mExifThumbBytes = offset;
        mCallback (1, fmt::format ("exifThumbLength {}", mExifThumbBytes), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_GPSINFO:
        getExifGpsInfo (offsetBasePtr + offset, offsetBasePtr, intelEndian);
        mCallback (1, fmt::format ("exifGps {}", mExifGpsInfo.getString()), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_MAXAPERTURE:
        mCallback (1, "exifMaxAperture", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_SHUTTERSPEED:
        mCallback (1, "exifShutterSpeed", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_X_RESOLUTION:
        mCallback (1, "exifResolutionX", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_Y_RESOLUTION:
        mCallback (1, "exifResolutionY", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_RESOLUTION_UNIT:
        mCallback (1, "exifResolutionUnit", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_SOFTWARE:
        mCallback (1, fmt::format ("exifSoftware {}", (char*)valuePtr), startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_Y_CB_CR_POSITIONING:
        mCallback (1, "exifCbCrPosition", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_FNUMBER:
        mCallback (1, "exifFnumber", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_EXPOSURE_PROGRAM:
        mCallback (1, "exifExposureProgram", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_ISO_EQUIVALENT:
        mCallback (1, "exifIsoEquivalent", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_EXIF_VERSION:
        mCallback (1, "exifVersion", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_BRIGHTNESS_VALUE:
        mCallback (1, "exifBrightness", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_EXPOSURE_BIAS:
        mCallback (1, "exifExposureBias", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_METERING_MODE:
        mCallback (1, "exifMeteringMode", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_FLASH:
        mCallback (1, "exifFlash", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_MAKER_NOTE:
        mCallback (1, "exifMakerNote", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_FLASH_PIX_VERSION:
        mCallback (1, "exifPixVersion", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_COLOR_SPACE:
        mCallback (1, "exifColorSpace", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_PIXEL_X_DIMENSION:
        mCallback (1, "exifDimensionX", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_PIXEL_Y_DIMENSION:
        mCallback (1, "exifDimeansionY", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_INTEROP_OFFSET:
        mCallback (1, "exifInteropOffset", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_SCENE_TYPE:
        mCallback (1, "exifSceneType", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_CUSTOM_RENDERED:
        mCallback (1, "exifCustomRendered", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_EXPOSURE_MODE:
        mCallback (1, "exifExposureMode", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_WHITEBALANCE:
        mCallback (1, "exifWhiteBalance", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_DIGITALZOOMRATIO:
        mCallback (1, "exifDigitalZoomRatio", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_SCENE_CAPTURE_TYPE:
        mCallback (1, "exifSceneCapture", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_CONTRAST:
        mCallback (1, "exifContrast", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_SATURATION:
        mCallback (1, "exifSaturation", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_SHARPNESS:
        mCallback (1, "exifSharpness", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      case TAG_COMPONENTS_CONFIG:
        mCallback (1, "exifComponentsConfig", startPtr, 0, bytes);
        break;
      //}}}
      //{{{
      default:
        mCallback (1, fmt::format ("exif tag {:x}", tag), startPtr, 0, bytes);
        break;
      //}}}
      }
    }

  uint32_t extraDirectoryOffset = getExifLong (ptr, intelEndian);
  if (extraDirectoryOffset > 4)
    parseExifDirectory (offsetBasePtr, offsetBasePtr + extraDirectoryOffset, intelEndian);
  }
//}}}

// jpeg tag parsers
//{{{
void cJpegAnalyse::parseAPP0 (const string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length) {
// read APP0 JFIF marker

  //{{{  unused
  (void)ptr;
  (void)length;
  //}}}

  uint8_t id[6];
  for (int i = 0; i < 5; i++)
    id[i] = ptr[i];
  id[5] = 0; // extra null termination

  uint16_t version = ptr[5] << 8 || ptr[6];
  //uint8_t units = ptr[7];
  //uint8_t xDensity = ptr[8];
  //uint8_t yDensity = ptr[9];

  mCallback (0, fmt::format ("{} version:{}", tag, version), startPtr, offset, length);
  }
//}}}
//{{{
void cJpegAnalyse::parseAPP1 (const string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length) {
// read APP1 EXIF marker, return true if thumb, valid mThumbBuffer, mThumbLength

  //{{{  unused
  (void)ptr;
  (void)length;
  //}}}
  // check exifId
  if (getExifLong (ptr, false) != 0x45786966) {
    mCallback (0, tag + " missing exif tag", startPtr, offset, length);
    return;
    }
  ptr += 4;

  // check 0 word
  if (getExifWord (ptr, false) != 0x0000) {
    mCallback (0, tag + " missing zero", startPtr, offset, length);
    return;
    }
  ptr += 2;

  uint8_t* offsetBasePtr = ptr;

  // endian
  bool intelEndian = getExifWord (ptr, false) == 0x4949;
  ptr += 2;

  // 002a word
  if (getExifWord (ptr, intelEndian) != 0x002a) {
    mCallback (0, tag + "missing 2a", startPtr, offset, length);
    return;
    }
  ptr += 2;

  // firstOffset 8
  if (getExifLong (ptr, intelEndian) != 8) {
    mCallback (0, tag + "missing first offset" , startPtr, offset, length);
    return;
    }
  ptr += 4;

  mCallback (0, tag + " exif", startPtr, offset, length);
  parseExifDirectory (offsetBasePtr, ptr, intelEndian);

  // dodgy add to offset of SOImarker(2), APP1marker(2), APP1length(2), EXIF00marker(6)
  mExifThumbOffset += 2 + 2 + 2 + 6;
  }
//}}}
//{{{
void cJpegAnalyse::parseAPP2 (const string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length) {
// read APP2 marker

  //{{{  unused
  (void)ptr;
  (void)length;
  //}}}
  // dodgy use of ptr as title
  mCallback (0, fmt::format ("{}", tag), startPtr, offset, length);
  }
//}}}
//{{{
void cJpegAnalyse::parseAPP14 (const string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length) {
// read APP14

  //{{{  unused
  (void)ptr;
  (void)length;
  //}}}
  // dodgy use of ptr as title
  mCallback (0, fmt::format ("{}", tag), startPtr, offset, length);
  }
//}}}
//{{{
void cJpegAnalyse::parseSOF (const string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length) {

  //{{{  unused param
  (void)length;
  //}}}
  uint8_t precision = ptr[0];
  mHeight = ptr[1] << 8 | ptr[2];
  mWidth =  ptr[3] << 8 | ptr[4];
  uint8_t numImageComponentsFrame = ptr[5];
  string str = fmt::format ("{} {}x{} {}bit", tag, mWidth, mHeight, precision);

  for (uint8_t i = 0; i < numImageComponentsFrame; i++) {
    uint8_t componentId = ptr[6 + 3 * i];
    uint8_t horizSample = ptr[7 + (3 * i)] >> 4;
    uint8_t vertSample = ptr[7 + (3 * i)] & 15;
    uint8_t qtableId = ptr[8 + 3 * i];
    str += fmt::format (" {}:{}x{}t{}", componentId, horizSample, vertSample, qtableId);
    }

  mCallback (0, str, startPtr, offset, length);
  }
//}}}
//{{{
void cJpegAnalyse::parseDQT (const string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length) {
// create de-quantization and prescaling tables with a DQT segment

  //{{{  unused
  (void)ptr;
  (void)length;
  //}}}
  mCallback (0, tag, startPtr, offset, length);
  }
//}}}
//{{{
void cJpegAnalyse::parseHFT (const string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length) {
// Create huffman code tables with a DHT segment

  //{{{  unused
  (void)ptr;
  (void)length;
  //}}}
  mCallback (0, tag, startPtr, offset, length);
  }
//}}}
//{{{
void cJpegAnalyse::parseDRI (const string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length) {

  //{{{  unused
  (void)ptr;
  (void)length;
  //}}}
  uint16_t numRst = ptr[0] << 8 | ptr[1];
  mCallback (0, fmt::format ("{} {}", tag, numRst), startPtr, offset, length);
  }
//}}}
//{{{
void cJpegAnalyse::parseSOS (const string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length) {

  //{{{  unused
  (void)ptr;
  (void)length;
  //}}}
  uint8_t numComponents = ptr[0];

  string str;
  for (uint8_t i = 0; i < numComponents; i++) {
    uint8_t component = ptr[1 + (i*2)];
    uint8_t dcTable = ptr[2 + (i*2)] >> 4;
    uint8_t acTable = ptr[2 + (i*2)] & 0xF;
    str += fmt::format (" {}ac{}dc{}", component, dcTable, acTable);
    }

  uint8_t ss = ptr[3 + (numComponents*2)];
  uint8_t se = ptr[4 + (numComponents*2)];
  uint8_t approx = ptr[5 + (numComponents*2)];

  mCallback (0, fmt::format ("{} {} ss:{} se:{} approx{:02x}", tag, str, ss, se, approx), startPtr, offset, length);
  }
//}}}

// cJpegAnalyse::cExifGpsInfo
//{{{
string cJpegAnalyse::cExifGpsInfo::getString() {

  string str = fmt::format ("{}", mDatum);
  //str += fmt::format("{} {} {} {}", mLatitudeDeg, mLatitudeRef, mLatitudeMin, mLatitudeSec);
  //str += fmt::format("{} {} {} {}", mLongitudeDeg, mLongitudeRef, mLongitudeMin, mLongitudeSec);

  str += fmt::format ("{}", mAltitude); //, mAltitudeRef);
  str += fmt::format ("{}", mImageDirection); //mImageDirectionRef);
  str += fmt::format ("{} {} {} {}", mDate,mHour,mMinute,mSecond);
  return str;
  }
//}}}
#endif
