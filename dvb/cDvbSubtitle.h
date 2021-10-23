// cSubtitle.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

class cTexture;
//}}}

class cDvbSubtitle {
public:
  cDvbSubtitle();
  ~cDvbSubtitle();

  bool decode (const uint8_t* buf, int bufSize);

  // vars
  bool mChanged = false;
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

    int mClutSize = 0;
    uint32_t mClut[16];
    };
  //}}}
  std::vector <cSubtitleRect*> mRects;
  std::array <cTexture*,4> mTextures = {nullptr};

private:
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
    int mClut;
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
  struct sClut {
    sClut* mNext;

    int mVersion = 0;
    int mId = 0;

    uint32_t mClut16[16];
    uint32_t mClut16bgra[16];

    //uint32_t mClut4[4];
    //uint32_t mClut256[256];
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

  void deleteRegionDisplayList (sRegion* region);
  void deleteRegions();
  void deleteObjects();
  void deleteCluts();

  sClut* getClut (int clutId);
  sObject* getObject (int objectId);
  sRegion* getRegion (int regionId);

  int parse4bit (const uint8_t** buf, int bufSize, uint8_t* pixBuf, int pixBufSize, int pixPos,
                 int nonModifyColour, uint8_t* mapTable);
  void parseObjectBlock (sObjectDisplay* display, const uint8_t* buf, int bufSize,
                         bool bottom, int nonModifyColour);

  bool parsePage (const uint8_t* buf, int bufSize);
  bool parseRegion (const uint8_t* buf, int bufSize);
  bool parseClut (const uint8_t* buf, int bufSize);
  bool parseObject (const uint8_t* buf, int bufSize);
  bool parseDisplayDefinition (const uint8_t* buf, int bufSize);

  bool updateRects();

  //  vars
  int mVersion = 0;
  int mTimeOut = 0;

  sClut mDefaultClut;
  sDisplayDefinition* mDisplayDefinition = nullptr;

  // !!! change to vectors !!!!
  sClut* mClutList = nullptr;
  sRegion* mRegionList = nullptr;
  sObject* mObjectList = nullptr;
  sRegionDisplay* mDisplayList = nullptr;
  };
