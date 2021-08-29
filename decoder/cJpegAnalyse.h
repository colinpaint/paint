// cJpegAnalyser.h - jpeg analyser - based on tiny jpeg decoder, jhead
#pragma once
//{{{  includes
#include "cFileAnalyse.h"

#include <cstdint>
#include <string>
#include <functional>
//}}}

class cJpegAnalyse : public cFileAnalyse {
private:
  using uTagLambda = std::function <void (const std::string& info, uint8_t* ptr,
                                          unsigned offset, unsigned numBytes)>;
public:
  cJpegAnalyse (const std::string& filename, unsigned components)
    : cFileAnalyse(filename), mBytesPerPixel(components) {}
  virtual ~cJpegAnalyse() = default;

  // gets
  int getWidth() { return mWidth; }
  int getHeight() { return mHeight; }

  int getThumbOffset() { return mThumbBytes > 0 ? mThumbOffset : 0; }
  int getThumbBytes() { return mThumbBytes; }

  bool readHeader (uTagLambda jpegTagLambda, uTagLambda exifTagLambda);

private:
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
    std::string mExifTimeString;
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

  void parseExifDirectory (uint8_t* offsetBasePtr, uint8_t* ptr, bool intelEndian);
  bool parseAPP (uint8_t* ptr, unsigned length);
  bool parseDQT (uint8_t* ptr, unsigned length);
  bool parseHFT (uint8_t* ptr, unsigned length);
  bool parseDRI (uint8_t* ptr, unsigned length);
  bool parseSOF (uint8_t* ptr, unsigned length);
  bool parseSOS (uint8_t* ptr, unsigned length);

  // vars
  uTagLambda mJpegTagLambda;
  uTagLambda mExifTagLambda;

  unsigned mBytesPerPixel = 0;

  // SOF values
  unsigned mWidth = 0;  // Size of the input image
  unsigned mHeight = 0; // Size of the input image
  unsigned mSx = 0;     // MCU size in unit of block (width, height)
  unsigned mSy = 0;     // MCU size in unit of block (width, height)
  uint8_t mQtableId[3]; // Quantization table ID of each component

  // NRI value
  unsigned mNumRst = 0; // Restart inverval in MCUs

  // thumbnail
  unsigned mThumbOffset = 0;
  unsigned mThumbBytes = 0;

  // exif
  cExifInfo mExifInfo;
  cExifGpsInfo mExifGpsInfo;
  std::string mExifTimeString;
  };
