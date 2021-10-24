// cDvbSubtitle.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

class cTexture;
//}}}

#define BGRA(r,g,b,a) static_cast<uint32_t>(((a << 24) ) | (b << 16) | (g <<  8) | r)
//{{{
class cSubtitleImage {
public:
  cSubtitleImage() {}
  ~cSubtitleImage() { free (mPixels); }

  int mX = 0;
  int mY = 0;
  int mWidth = 0;
  int mHeight = 0;

  bool mPixelsChanged = false;
  std::array <uint32_t,16> mColorLut;
  uint8_t* mPixels = nullptr;
  cTexture* mTexture = nullptr;
  };
//}}}

class cDvbSubtitle {
public:
  cDvbSubtitle() = default;
  ~cDvbSubtitle();

  size_t getNumRegions() const { return mNumRegions; }
  bool decode (const uint8_t* buf, int bufSize);

  // vars !!! can't get non pointer vector to work !!!
  std::vector <cSubtitleImage*> mImages;

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
  class cDisplayDefinition {
  public:
    int mVersion = -1;

    int mX = 0;
    int mY = 0;

    int mWidth = 0;
    int mHeight = 0;
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
  struct sRegionDisplay {
    sRegionDisplay* mNext;

    int mRegionId;
    int xPos;
    int yPos;
    };
  //}}}

  // get
  cColorLut& getColorLut (uint8_t id);
  sObject* getObject (int objectId);
  sRegion* getRegion (int regionId);

  // parse
  bool parseColorLut (const uint8_t* buf, uint32_t bufSize);
  int parse4bit (const uint8_t** buf, uint32_t bufSize,
                 uint8_t* pixBuf, uint32_t pixBufSize, uint32_t pixPos, bool nonModifyColour);
  void parseObjectBlock (sObjectDisplay* display, const uint8_t* buf, uint32_t bufSize,
                         bool bottom, bool nonModifyColour);
  bool parseObject (const uint8_t* buf, uint32_t bufSize);
  bool parsePage (const uint8_t* buf, uint32_t bufSize);
  bool parseRegion (const uint8_t* buf, uint32_t bufSize);
  bool parseDisplayDefinition (const uint8_t* buf, uint32_t bufSize);

  // delete
  void deleteColorLuts();
  void deleteObjects();
  void deleteRegions();
  void deleteRegionDisplayList (sRegion* region);

  // update
  bool updateRects();

  // vars
  cDisplayDefinition mDisplayDefinition;

  int mPageVersion = -1;
  int mPageTimeOut = 0;

  size_t mNumRegions = 0;
  std::vector <cColorLut> mColorLuts;
  sRegion* mRegionList = nullptr;
  sObject* mObjectList = nullptr;
  sRegionDisplay* mDisplayList = nullptr;
  };
