// cSubtitle.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

class cTexture;
//}}}
#define RGBA(r,g,b,a) (uint32_t ((((a) << 24) & 0xFF000000) | (((r) << 16) & 0x00FF0000) | \
                                 (((g) <<  8) & 0x0000FF00) |  ((b)        & 0x000000FF)))

#define BGRA(r,g,b,a) (uint32_t ((((a) << 24) & 0xFF000000) | (((b) << 16) & 0x00FF0000) | \
                                 (((g) <<  8) & 0x0000FF00) |  ((r)        & 0x000000FF)))

class cDvbSubtitle {
public:
  cDvbSubtitle() = default;
  ~cDvbSubtitle();

  bool decode (const uint8_t* buf, int bufSize);

  // vars
  size_t mNumRegions = 0;
  //{{{
  class cSubtitleRect {
  public:
    cSubtitleRect() {}
    ~cSubtitleRect() { free (mPixData); }

    int mX = 0;
    int mY = 0;
    int mWidth = 0;
    int mHeight = 0;
    uint32_t* mPixData = nullptr;
    std::array <uint32_t,16> mColorLut;
    };
  //}}}
  std::vector <cSubtitleRect*> mRects;
  std::array <cTexture*,4> mTextures = {nullptr};
  bool mChanged = false;

private:
  //{{{
  class cColorLut {
  public:
    //{{{
    cColorLut() {

      m16rgba[0] = RGBA (0,0,0, 0xFF);
      m16bgra[0] = BGRA (0,0,0, 0xFF);

      for (int i = 1; i <= 0x0F; i++) {
        int r = (i < 8) ? ((i & 1) ? 0xFF : 0) : ((i & 1) ? 0x7F : 0);
        int g = (i < 8) ? ((i & 2) ? 0xFF : 0) : ((i & 2) ? 0x7F : 0);
        int b = (i < 8) ? ((i & 4) ? 0xFF : 0) : ((i & 4) ? 0x7F : 0);

        m16rgba[i] = RGBA (r,g,b, 0xFF);
        m16bgra[i] = BGRA (r,g,b, 0xFF);
        }
      }
    //}}}
    //{{{
    cColorLut(uint8_t id) : mId(id) {

      m16rgba[0] = RGBA (0,0,0, 0xFF);
      m16bgra[0] = BGRA (0,0,0, 0xFF);

      for (int i = 1; i <= 0x0F; i++) {
        int r = (i < 8) ? ((i & 1) ? 0xFF : 0) : ((i & 1) ? 0x7F : 0);
        int g = (i < 8) ? ((i & 2) ? 0xFF : 0) : ((i & 2) ? 0x7F : 0);
        int b = (i < 8) ? ((i & 4) ? 0xFF : 0) : ((i & 4) ? 0x7F : 0);

        m16rgba[i] = RGBA (r, g, b, 0xFF);
        m16bgra[i] = BGRA (r, g, b, 0xFF);
        }
      }
    //}}}

    uint8_t mId = 0xFF;
    uint8_t mVersion = 0xFF;

    std::array <uint32_t,16> m16rgba;
    std::array <uint32_t,16> m16bgra;
    };
  //}}}
  //{{{
  struct sObjectDisplay {
    //{{{
    void init (int objectId, int regionId, int xpos, int ypos) {
      mObjectListNext = nullptr;

      mObjectId = objectId;
      mRegionId = regionId;

      xPos = xpos;
      yPos = ypos;

      mForegroundColour = 0;;
      mBackgroundColour = 0;

      mRegionListNext = nullptr;
      }
    //}}}

    sObjectDisplay* mObjectListNext;

    int mObjectId;
    int mRegionId;

    int xPos;
    int yPos;

    int mForegroundColour;
    int mBackgroundColour;

    sObjectDisplay* mRegionListNext;
    };
  //}}}
  //{{{
  struct sObject {
    sObject* mNext;

    int mId;
    int mType;

    sObjectDisplay* mDisplayList;
    };
  //}}}
  //{{{
  struct sRegionDisplay {
    sRegionDisplay* mNext;

    int mRegionId;
    int xPos;
    int yPos;
    };
  //}}}
  //{{{
  struct sRegion {
    sRegion* mNext;

    int mId;
    int mVersion;

    int mWidth;
    int mHeight;
    int mDepth;
    uint8_t mColorLut;
    int mBackgroundColour;

    bool mDirty;
    int mPixBufSize;
    uint8_t* mPixBuf;

    sObjectDisplay* mDisplayList;
    };
  //}}}
  //{{{
  struct sDisplayDefinition {
    int mVersion;

    int mX;
    int mY;
    int mWidth;
    int mHeight;
    };
  //}}}
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
    int getBitsRead() {
      return mBitsRead;
      }
    //}}}
    //{{{
    int getBytesRead() {
      return (mBitsRead + 7) / 8;
      }
    //}}}

  private:
    uint8_t* mBytePtr;
    uint32_t mCache = 0;
    int32_t mCacheBits = 0;
    int mBitsRead = 0;
    };
  //}}}

  cColorLut& getColorLut (uint8_t id);
  sObject* getObject (int objectId);
  sRegion* getRegion (int regionId);

  bool parseColorLut (const uint8_t* buf, int bufSize);
  int parse4bit (const uint8_t** buf, int bufSize, uint8_t* pixBuf, int pixBufSize, int pixPos, int nonModifyColour);
  void parseObjectBlock (sObjectDisplay* display, const uint8_t* buf, int bufSize, bool bottom, int nonModifyColour);
  bool parseObject (const uint8_t* buf, int bufSize);
  bool parsePage (const uint8_t* buf, int bufSize);
  bool parseRegion (const uint8_t* buf, int bufSize);
  bool parseDisplayDefinition (const uint8_t* buf, int bufSize);

  bool updateRects();

  void deleteColorLuts();
  void deleteObjects();
  void deleteRegions();
  void deleteRegionDisplayList (sRegion* region);

  // vars
  int mVersion = 0;
  int mTimeOut = 0;

  sDisplayDefinition* mDisplayDefinition = nullptr;

  std::vector <cColorLut> mColorLuts;
  sRegion* mRegionList = nullptr;
  sObject* mObjectList = nullptr;
  sRegionDisplay* mDisplayList = nullptr;
  };
