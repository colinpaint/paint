// cJpegAnalyser.h - jpeg analyser - based on tiny jpeg decoder, jhead
#pragma once
//{{{  includes
#include "cFileView.h"
#include <functional>
//}}}

class cJpegAnalyse : public cFileView {
private:
  using tCallback = std::function <void (uint8_t level, const std::string& info, uint8_t* ptr,
                                         uint32_t offset, uint32_t numBytes)>;
public:
  cJpegAnalyse (const std::string& filename, uint8_t components)
    : cFileView(filename), mBytesPerPixel(components) {}
  virtual ~cJpegAnalyse() = default;

  // gets
  int getWidth() { return mWidth; }
  int getHeight() { return mHeight; }

  int getThumbOffset() { return mExifThumbBytes > 0 ? mExifThumbOffset : 0; }
  int getThumbBytes() { return mExifThumbBytes; }

  bool analyse (tCallback callback);

private:
  // exif parse utils
  //{{{
  class cExifInfo {
  public:
    std::string mExifMake;
    std::string mExifModel;

    uint32_t mExifOrientation = 0;
    float mExifExposure = 0;
    float mExifFocalLength = 0;
    float mExifAperture = 0;

    tm mExifTm;
    tm mExifTmOriginal;
    tm mExifTmDigitized;
    std::string mExifTimeString;
    std::string mExifTimeOriginalString;
    std::string mExifTimeDigitizedString;
    };
  //}}}
  //{{{
  class cExifGpsInfo {
  public:
    cExifGpsInfo() {}

    std::string getString();

    int mVersion = 0;

    wchar_t mLatitudeRef = 0;
    float mLatitudeDeg = 0;
    float mLatitudeMin = 0;
    float mLatitudeSec = 0;

    char mLongitudeRef = 0;
    float mLongitudeDeg = 0;
    float mLongitudeMin = 0;
    float mLongitudeSec = 0;

    wchar_t mAltitudeRef = 0;
    float mAltitude = 0;

    wchar_t mImageDirectionRef;
    float mImageDirection = 0;

    float mHour = 0;
    float mMinute = 0;
    float mSecond = 0;

    std::string mDate;
    std::string mDatum;
    };
  //}}}

  uint16_t getExifWord (uint8_t* ptr, bool intelEndian);
  uint32_t getExifLong (uint8_t* ptr, bool intelEndian);
  float getExifSignedRational (uint8_t* ptr, bool intelEndian, uint32_t& numerator, uint32_t& denominator);
  void getExifGpsInfo (uint8_t* ptr, uint8_t* offsetBasePtr, bool intelEndian);
  std::string getExifTime (uint8_t* ptr, struct tm* tmPtr);

  // exif tag parser
  void parseExifDirectory (uint8_t* offsetBasePtr, uint8_t* ptr, bool intelEndian);

  // jpeg tag parsers
  void parseAPP0 (const std::string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length);
  void parseAPP1 (const std::string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length);
  void parseAPP2 (const std::string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length);
  void parseAPP14 (const std::string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length);
  void parseSOF (const std::string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length);
  void parseDQT (const std::string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length);
  void parseHFT (const std::string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length);
  void parseDRI (const std::string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length);
  void parseSOS (const std::string& tag, uint8_t* startPtr, uint32_t offset, uint8_t* ptr, uint32_t length);

  // vars
  tCallback mCallback;

  uint8_t mBytesPerPixel = 0;

  // SOF
  uint16_t mWidth = 0;  // Size of the input image
  uint16_t mHeight = 0; // Size of the input image

  // exif
  cExifInfo mExifInfo;
  cExifGpsInfo mExifGpsInfo;
  std::string mExifTimeString;
  uint32_t mExifThumbOffset = 0;
  uint32_t mExifThumbBytes = 0;
  };
