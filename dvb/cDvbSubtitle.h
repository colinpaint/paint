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

  uint8_t mPageState = 0;
  uint8_t mPageVersion = 0;

  int mX = 0;
  int mY = 0;
  int mWidth = 0;
  int mHeight = 0;

  bool mDirty = false;
  std::array <uint32_t,16> mColorLut;
  uint8_t* mPixels = nullptr;
  cTexture* mTexture = nullptr;
  };
//}}}

class cDvbSubtitle {
public:
  cDvbSubtitle (uint16_t sid, const std::string name)  : mSid(sid), mName(name) {}
  ~cDvbSubtitle();

  size_t getNumImages() const { return mNumImages; }
  bool decode (const uint8_t* buf, int bufSize);

  // vars !!! can't get non pointer vector to work !!!
  size_t mNumImages = 0;
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
    uint8_t mVersion = 0xFF;

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
  class cObjectDisplay {
  public:
    //{{{
    void init (uint16_t objectId, uint8_t regionId, int xpos, int ypos) {

      mObjectId = objectId;
      mRegionId = regionId;

      xPos = xpos;
      yPos = ypos;

      mForegroundColour = 0;;
      mBackgroundColour = 0;

      mObjectListNext = nullptr;
      mRegionListNext = nullptr;
      }
    //}}}

    uint16_t mObjectId;
    uint8_t mRegionId;

    int xPos;
    int yPos;

    int mForegroundColour;
    int mBackgroundColour;

    cObjectDisplay* mObjectListNext;
    cObjectDisplay* mRegionListNext;
    };
  //}}}
  //{{{
  class cObject {
  public:
    uint16_t mId;
    int mType;
    cObjectDisplay* mDisplayList;

    cObject* mNext;
    };
  //}}}
  //{{{
  class cRegion {
  public:
    cRegion (uint8_t id) : mId(id) {}
    //~cRegion() { free (mPixBuf); } // why can't I delete mPixBuf
    ~cRegion() { }

    uint8_t mId = 0;
    int mVersion = -1;

    int mWidth = 0;
    int mHeight = 0;
    int mDepth = 0;
    uint8_t mColorLut = 0;
    int mBackgroundColour = 0;

    bool mDirty = false;
    int mPixBufSize = 0;
    uint8_t* mPixBuf = nullptr;

    cObjectDisplay* mDisplayList = nullptr;
    };
  //}}}
  //{{{
  class cRegionDisplay {
  public:
    uint8_t mRegionId;
    int xPos;
    int yPos;

    cRegionDisplay* mNext;
    };
  //}}}

  // get
  cColorLut& getColorLut (uint8_t id);
  cObject* getObject (uint16_t id);
  cRegion* getRegion (uint8_t id);

  // parse
  bool parseColorLut (const uint8_t* buf, uint16_t bufSize);
  int parse4bit (const uint8_t** buf, uint16_t bufSize,
                 uint8_t* pixBuf, uint32_t pixBufSize, uint32_t pixPos, bool nonModifyColour);
  void parseObjectBlock (cObjectDisplay* display, const uint8_t* buf, uint16_t bufSize,
                         bool bottom, bool nonModifyColour);
  bool parseObject (const uint8_t* buf, uint16_t bufSize);
  bool parsePage (const uint8_t* buf, uint16_t bufSize);
  bool parseRegion (const uint8_t* buf, uint16_t bufSize);
  bool parseDisplayDefinition (const uint8_t* buf, uint16_t bufSize);

  // delete
  void deleteColorLuts();
  void deleteObjects();
  void deleteRegionDisplayList (cRegion* region);
  void deleteRegions();

  // update
  bool endDisplaySet();

  // vars
  const uint16_t mSid;
  const std::string mName;

  // page info
  uint8_t mPageVersion = 0xFF;
  uint8_t mPageState = 0;

  // pools
  std::vector <cColorLut> mColorLuts;
  std::vector <cRegion> mRegions;
  cObject* mObjectList = nullptr;

  cRegionDisplay* mDisplayList = nullptr;

  cDisplayDefinition mDisplayDefinition;
  };
