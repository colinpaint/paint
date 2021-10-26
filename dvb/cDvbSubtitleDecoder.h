// cDvbSubtitle.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

#include "iDvbDecoder.h"
class cTexture;
//}}}

#define BGRA(r,g,b,a) static_cast<uint32_t>(((a << 24) ) | (b << 16) | (g <<  8) | r)
//{{{
class cSubtitleImage {
public:
  cSubtitleImage() {}
  ~cSubtitleImage() { free (mPixels); }

  uint8_t mPageState = 0;
  uint8_t mPageVersion = 0;

  int mX = 0;
  int mY = 0;
  int mWidth = 0;
  int mHeight = 0;

  bool mDirty = false;
  std::array <uint32_t,16> mColorLut = {0};
  uint8_t* mPixels = nullptr;

  cTexture* mTexture = nullptr;
  };
//}}}

class cDvbSubtitleDecoder : public iDvbDecoder {
public:
  cDvbSubtitleDecoder (uint16_t sid, const std::string name) : iDvbDecoder(), mSid(sid), mName(name) {}
  ~cDvbSubtitleDecoder();

  size_t getNumImages() const { return mPage.mNumImages; }
  size_t getHighWatermarkImages() const { return mPage.mHighwaterMark; }
  cSubtitleImage& getImage (size_t line) { return mPage.mImages[line]; }

  virtual bool decode (const uint8_t* buf, int bufSize) final;

private:
  //{{{
  class cBitStream {
  // dodgy faster bitstream, no size chacking, assumes multiple of 4 bytes
  public:
    cBitStream (const uint8_t* buffer) : mBytePtr((uint8_t*)buffer) {}

    //{{{
    uint32_t getBits (int numBits) {

      uint32_t data = mCache >> (31 - numBits);  // unsigned >> so zero-extend
      data >>= 1;                                // do as >> 31, >> 1 so that numBits = 0 works okay (returns 0)
      mCache <<= numBits;                        // left-justify cache
      mCacheBits -= numBits;                     // how many bits have we drawn from the cache so far

      // if we cross an int boundary, refill the cache
      if (mCacheBits < 0) {
        uint32_t lowBits = -mCacheBits;

        // assumes always 4 more bytes
        mCache  = (*mBytePtr++) << 24;
        mCache |= (*mBytePtr++) << 16;
        mCache |= (*mBytePtr++) <<  8;
        mCache |= (*mBytePtr++);
        mCacheBits = 32;

        // get the low-order bits
        data |= mCache >> (32 - lowBits);

        mCacheBits -= lowBits;  // how many bits have we drawn from the cache so far
        mCache <<= lowBits;     // left-justify cache
        }

      mBitsRead += numBits;
      return data;
      }
    //}}}
    uint32_t getBit() { return getBits (1); }

    //{{{
    uint32_t getBitsRead() {
      return mBitsRead;
      }
    //}}}
    //{{{
    uint32_t getBytesRead() {
      return (mBitsRead + 7) / 8;
      }
    //}}}

  private:
    uint8_t* mBytePtr;

    uint32_t mCache = 0;
    int32_t mCacheBits = 0;
    uint32_t mBitsRead = 0;
    };
  //}}}
  //{{{
  class cColorLut {
  public:
    //{{{
    cColorLut() {

      m16bgra[0] = BGRA (0,0,0, 0xFF);
      for (int i = 1; i <= 0x0F; i++)
        m16bgra[i] = BGRA ((i < 8) ? ((i & 1) ? 0xFF : 0) : ((i & 1) ? 0x7F : 0),
                           (i < 8) ? ((i & 2) ? 0xFF : 0) : ((i & 2) ? 0x7F : 0),
                           (i < 8) ? ((i & 4) ? 0xFF : 0) : ((i & 4) ? 0x7F : 0),
                           0xFF);
      }
    //}}}
    //{{{
    cColorLut(uint8_t id) : mId(id) {

      m16bgra[0] = BGRA (0,0,0, 0xFF);
      for (int i = 1; i <= 0x0F; i++)
        m16bgra[i] = BGRA ((i < 8) ? ((i & 1) ? 0xFF : 0) : ((i & 1) ? 0x7F : 0),
                           (i < 8) ? ((i & 2) ? 0xFF : 0) : ((i & 2) ? 0x7F : 0),
                           (i < 8) ? ((i & 4) ? 0xFF : 0) : ((i & 4) ? 0x7F : 0),
                           0xFF);
      }
    //}}}

    uint8_t mId = 0xFF;
    uint8_t mVersion = 0xFF;

    std::array <uint32_t,16> m16bgra;
    };
  //}}}
  //{{{
  class cObject {
  public:
    cObject (uint16_t id) : mId(id) {}

    const uint16_t mId;

    uint8_t mRegionId = 0;
    uint8_t mType = 0;

    uint16_t mXpos = 0;
    uint16_t mYpos = 0;

    uint8_t mForegroundColour = 0;
    uint8_t mBackgroundColour = 0;
    };
  //}}}
  //{{{
  class cRegion {
  public:
    cRegion (uint8_t id) : mId(id) {}
    ~cRegion() {
      //free (mPixBuf);  // why can't I delete mPixBuf
      }

    uint8_t mId = 0;
    int mVersion = -1;

    int mWidth = 0;
    int mHeight = 0;

    uint8_t mColorLut = 0;
    uint8_t mColorLutDepth = 0;
    int mBackgroundColour = 0;

    bool mDirty = false;
    int mPixBufSize = 0;
    uint8_t* mPixBuf = nullptr;
    };
  //}}}
  //{{{
  class cDisplayRegion {
  public:
    cDisplayRegion (uint8_t regionId, uint16_t xpos, uint16_t ypos)
      : mRegionId(regionId), mXpos(xpos), mYpos(ypos) {}

    const uint8_t mRegionId;

    const uint16_t mXpos;
    const uint16_t mYpos;
    };
  //}}}
  //{{{
  class cPage {
  public:
    uint8_t mVersion = 0xFF;
    uint8_t mState = 0;
    uint8_t mTimeout = 0xFF;

    // parser side
    std::vector <cDisplayRegion> mDisplayRegions;

    // render side, max 4 !!! should check !!!
    size_t mNumImages = 0;
    size_t mHighwaterMark = 0;
    std::array <cSubtitleImage,4> mImages;
    };
  //}}}
  //{{{
  class cDisplayDefinition {
  public:
    uint8_t mVersion = 0xFF;

    uint16_t mX = 0;
    uint16_t mY = 0;

    uint16_t mWidth = 0;
    uint16_t mHeight = 0;
    };
  //}}}

  // get
  cObject* findObject (uint16_t id);
  cObject& getObject (uint16_t id);
  cColorLut& getColorLut (uint8_t id);
  cRegion& getRegion (uint8_t id);

  // parse
  bool parseDisplayDefinition (const uint8_t* buf, uint16_t bufSize);
  bool parsePage (const uint8_t* buf, uint16_t bufSize);
  bool parseRegion (const uint8_t* buf, uint16_t bufSize);
  bool parseColorLut (const uint8_t* buf, uint16_t bufSize);

  uint16_t parse4bit (const uint8_t*& buf, uint16_t bufSize,
                      uint8_t* pixBuf, uint32_t pixBufSize, uint16_t pixPos, bool nonModifyColour);
  void parseObjectBlock (cObject* object, const uint8_t* buf, uint16_t bufSize, bool bottom, bool nonModifyColour);
  bool parseObject (const uint8_t* buf, uint16_t bufSize);

  void endDisplay();

  // vars
  const uint16_t mSid;
  const std::string mName;

  // pools
  std::vector <cColorLut> mColorLuts;
  std::vector <cObject> mObjects;
  std::vector <cRegion> mRegions;

  cPage mPage;

  cDisplayDefinition mDisplayDefinition;
  };
