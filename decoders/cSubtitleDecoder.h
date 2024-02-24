// cSubtitleDecoder.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#include "cDecoder.h"
#include "cSubtitleFrame.h"
#include "cSubtitleImage.h"

#include "../common/cLog.h"
#include "../common/utils.h"
//}}}
//{{{  defines
#define AVRB16(p) ((*(p) << 8) | *(p+1))

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))

#define BGRA(r,g,b,a) static_cast<uint32_t>(((a << 24) ) | (b << 16) | (g <<  8) | r)
//}}}

class cSubtitleDecoder : public cDecoder {
public:
  cSubtitleDecoder (cRender& render) : mRender(render) {}
  //{{{
  virtual ~cSubtitleDecoder() {

    mColorLuts.clear();
    mObjects.clear();

    for (auto& region : mRegions)
      delete region;
    mRegions.clear();

    mPage.mRegionDisplays.clear();
    }
  //}}}

  virtual std::string getInfoString() const final { return "subtitle"; }

  size_t getNumLines() const { return mPage.mNumLines; }
  size_t getHighWatermark() const { return mPage.mHighwaterMark; }
  cSubtitleImage& getImage (size_t line) { return mPage.mImages[line]; }

  //{{{
  virtual void decode (uint8_t* pes, uint32_t pesSize, int64_t pts, const std::string& frameInfo,
                       std::function<cFrame* (int64_t pts)> allocFrameCallback,
                       std::function<void (cFrame* frame)> addFrameCallback) final {
    (void)frameInfo;
    mPage.mPts = pts;
    mPage.mPesSize = pesSize;
    if (pesSize < 8) {
      //{{{  strange empty pes, common on itv multiplex
      return;
      }
      //}}}

    uint8_t* pesPtr = pes;
    uint8_t data_identifier = *pesPtr++;
    uint8_t subtitle_stream_id = *pesPtr++;
    (void)data_identifier;
    (void)subtitle_stream_id;

    uint8_t* pesEnd = pes + pesSize;
    while ((pesEnd - pesPtr) >= 6) {
      // check syncByte
      uint8_t syncByte = *pesPtr++;
      if (syncByte != 0x0f) {
        //{{{  syncByte error, return, common on bbc mulitplex
        return;
        }
        //}}}

      // get segType, pageId, segLength
      uint8_t segType = *pesPtr++;
      uint16_t pageId = AVRB16 (pesPtr);
      pesPtr += 2;
      uint16_t segLength = AVRB16 (pesPtr);
      pesPtr += 2;

      if (segLength > pesEnd - pesPtr) {
        //{{{  segLength error, return
        cLog::log (LOGERROR, "cSubtitle decode incomplete or broken packet");
        return;
        }
        //}}}

      switch (segType) {
        //{{{
        case 0x10: // page composition segment
          if (!parsePage (pesPtr, segLength))
            return;
          break;
        //}}}
        //{{{
        case 0x11: // region composition segment
          if (!parseRegion (pesPtr, segLength))
            return;
          break;
        //}}}
        //{{{
        case 0x12: // CLUT definition segment
          if (!parseColorLut (pesPtr, segLength))
            return;
          break;
        //}}}
        //{{{
        case 0x13: // object data segment
          if (!parseObject (pesPtr, segLength))
            return;
          break;
        //}}}
        //{{{
        case 0x14: // display definition segment
          if (!parseDisplayDefinition (pesPtr, segLength))
            return;
          break;
        //}}}
        //{{{
        case 0x80: // end of display set segment
          {
          cSubtitleFrame* subtitleFrame = dynamic_cast<cSubtitleFrame*>(allocFrameCallback (pts));

          subtitleFrame->set (pts, 90000 / 25, pesSize);

          endDisplay();

          addFrameCallback (subtitleFrame);

          break;
          }
        //}}}
        //{{{
        case 0x15: // disparity signalling segment
          cLog::log (LOGERROR, "cSubtitle decode disparity signalling segment");
          break;
        //}}}
        //{{{
        case 0x16: // alternative_CLUT_segment
          cLog::log (LOGERROR, "cSubtitle decode alternative_CLUT_segment");
          break;
        //}}}
        //{{{
        default:
          cLog::log (LOGERROR, "cSubtitle decode unknown seg:%x, pageId:%d, size:%d",
                                segType, pageId, segLength);
          break;
        //}}}
        }

      pesPtr += segLength;
      }
    }
  //}}}

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

    uint8_t mForegroundColor = 0;
    uint8_t mBackgroundColor = 0;
    };
  //}}}
  //{{{
  class cRegion {
  public:
    cRegion (uint8_t id) : mId(id) {}
    ~cRegion() { free (mPixBuf); }

    uint8_t mId = 0;
    uint8_t mVersion = 0xFF;

    size_t mWidth = 0;  // uint32_t to ensure width * height ok
    size_t mHeight = 0;
    uint8_t* mPixBuf = nullptr;
    bool mDirty = false;

    uint8_t mColorLut = 0;
    uint8_t mColorLutDepth = 0;
    uint8_t mBackgroundColor = 0;
    };
  //}}}
  //{{{
  class cRegionDisplay {
  public:
    cRegionDisplay (uint8_t regionId, uint16_t xpos, uint16_t ypos)
      : mRegionId(regionId), mXpos(xpos), mYpos(ypos) {}

    const uint8_t mRegionId;

    const uint16_t mXpos;
    const uint16_t mYpos;
    };
  //}}}
  //{{{
  class cPage {
  public:
    int64_t mPts = 0;
    int64_t mPesSize = 0;

    uint8_t mVersion = 0xFF;
    uint8_t mState = 0;
    uint8_t mTimeout = 0xFF;

    // parser side
    std::vector <cRegionDisplay> mRegionDisplays;

    // render side, max 4 !!! should check !!!
    size_t mNumLines = 0;
    size_t mHighwaterMark = 1;
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

  //{{{
  cObject* findObject (uint16_t id) {

    for (auto& object : mObjects)
      if (id == object.mId)
        return &object;

    return nullptr;
    }
  //}}}
  //{{{
  cObject& getObject (uint16_t id) {

    cObject* object = findObject (id);
    if (object) // found
      return *object;

    // not found, allocate
    mObjects.push_back (cObject(id));
    return mObjects.back();
    }
  //}}}

  //{{{
  cColorLut& getColorLut (uint8_t id) {

    // look for id colorLut
    for (auto& colorLut : mColorLuts)
      if (id == colorLut.mId)
        return colorLut;

    // create id colorLut
    mColorLuts.push_back (cColorLut (id));
    return mColorLuts.back();
    }
  //}}}
  //{{{
  cRegion& getRegion (uint8_t id) {

    for (auto region : mRegions)
      if (id == region->mId)
        return *region;

    mRegions.push_back (new cRegion (id));
    return *mRegions.back();
    }
  //}}}

  //{{{
  bool parseDisplayDefinition (const uint8_t* buf, uint16_t bufSize) {

    //cLog::log (LOGINFO, "displayDefinition segment");
    if (bufSize < 5)
      return false;

    uint8_t infoByte = *buf++;
    uint8_t ddsVersion = infoByte >> 4;

    if (mDisplayDefinition.mVersion == ddsVersion)
      return true;
    mDisplayDefinition.mVersion = ddsVersion;

    mDisplayDefinition.mX = 0;
    mDisplayDefinition.mY = 0;

    mDisplayDefinition.mWidth = AVRB16(buf) + 1;
    buf += 2;

    mDisplayDefinition.mHeight = AVRB16(buf) + 1;
    buf += 2;

    uint8_t displayWindow = infoByte & (1 << 3);
    if (displayWindow) {
      if (bufSize < 13)
        return false;

      mDisplayDefinition.mX = AVRB16(buf);
      buf += 2;

      mDisplayDefinition.mWidth  = AVRB16(buf) - mDisplayDefinition.mX + 1;
      buf += 2;

      mDisplayDefinition.mY = AVRB16(buf);
      buf += 2;

      mDisplayDefinition.mHeight = AVRB16(buf) - mDisplayDefinition.mY + 1;
      buf += 2;
      }

    return true;
    }
  //}}}
  //{{{
  bool parsePage (const uint8_t* buf, uint16_t bufSize) {

    if (bufSize < 1)
      return false;

    const uint8_t* bufEnd = buf + bufSize;

    mPage.mTimeout = *buf++;
    uint8_t pageVersion = ((*buf) >> 4) & 15;
    if (mPage.mVersion == pageVersion)
      return true;
    mPage.mVersion = pageVersion;
    mPage.mState = ((*buf++) >> 2) & 3;

    if ((mPage.mState == 1) || (mPage.mState == 2)) {
      // delete regions, objects, colorLuts
      for (auto& region : mRegions)
        delete region;
      mRegions.clear();
      mObjects.clear();
      mColorLuts.clear();
      }

    std::string regionDebug;
    mPage.mRegionDisplays.clear();
    while (buf + 5 < bufEnd) {
      uint8_t regionId = *buf;
      buf += 2; // skip reserved

      uint16_t xPos = AVRB16(buf);
      buf += 2;
      uint16_t yPos = AVRB16(buf);
      buf += 2;

      mPage.mRegionDisplays.push_back (cRegionDisplay (regionId, xPos, yPos));

      regionDebug += fmt::format ("{}:{},{} ", regionId, xPos, yPos);
      }

    return true;
    }
  //}}}
  //{{{
  bool parseRegion (const uint8_t* buf, uint16_t bufSize) {
  // assumes all objects used by region are defined after region

    if (bufSize < 10)
      return false;
    const uint8_t* bufEnd = buf + bufSize;

    uint8_t regionId = *buf++;
    cRegion& region = getRegion (regionId);
    region.mVersion = ((*buf) >> 4) & 0x0F;

    bool fill = ((*buf++) >> 3) & 1;
    uint16_t width = AVRB16(buf);
    buf += 2;
    uint16_t height = AVRB16(buf);
    buf += 2;
    if ((width != region.mWidth) || (height != region.mHeight)) {
      //{{{  region size changed, resize mPixBuf
      region.mWidth = width;
      region.mHeight = height;
      region.mPixBuf = (uint8_t*)realloc (region.mPixBuf, width * height);
      region.mDirty = false;
      fill = true;
      }
      //}}}

    // check and choose colorLut, 4bit only expected in UK
    region.mColorLutDepth = 1 << (((*buf++) >> 2) & 7);
    if (region.mColorLutDepth != 4) {
      //{{{  error return, allow 2 and 8 when we see them
      cLog::log (LOGERROR, fmt::format ("parseRegion - unknown region depth:{}", region.mColorLutDepth));
      return false;
      }
      //}}}
    region.mColorLut = *buf++;
    buf += 1; // skip

    // background fill
    region.mBackgroundColor = ((*buf++) >> 4) & 15;
    if (fill) {
      memset (region.mPixBuf, region.mBackgroundColor, region.mWidth * region.mHeight);
      region.mDirty = true;
      }

    std::string objectDebug;
    while (buf + 5 < bufEnd) {
      uint16_t objectId = AVRB16(buf);
      buf += 2;

      cObject& object = getObject (objectId);
      object.mRegionId = regionId;
      object.mType = (*buf) >> 6; // tighly packed into word type:xpos
      object.mXpos = AVRB16(buf) & 0xFFF;
      buf += 2;
      object.mYpos = AVRB16(buf) & 0xFFF;
      buf += 2;
      if ((object.mXpos >= region.mWidth) || (object.mYpos >= region.mHeight)) {
        //{{{  error return
        cLog::log (LOGERROR, "parseRegion - object outside region");
        return false;
        }
        //}}}
      if (((object.mType == 1) || (object.mType == 2)) && (buf+1 < bufEnd)) {
        object.mForegroundColor = *buf++;
        object.mBackgroundColor = *buf++;
        }

      objectDebug += fmt::format ("{}:{},{} ", objectId, object.mXpos, object.mYpos);
      }

    return true;
    }
  //}}}
  //{{{
  bool parseColorLut (const uint8_t* buf, uint16_t bufSize) {

    //cLog::log (LOGINFO, "colorLut segment");
    const uint8_t* bufEnd = buf + bufSize;

    uint8_t id = *buf++;
    uint8_t version = ((*buf++) >> 4) & 15;

    cColorLut& colorLut = getColorLut (id);
    if (colorLut.mVersion != version) {
      colorLut.mVersion = version;
      while (buf + 4 < bufEnd) {
        uint8_t entryId = *buf++;
        uint8_t depth = (*buf) & 0xe0;
        if (depth == 0) {
          //{{{  error return
          cLog::log (LOGERROR, "Invalid colorLut depth 0x%x!n", *buf);
          return false;
          }
          //}}}

        int y, cr, cb, alpha;
        bool fullRange = (*buf++) & 1;
        if (fullRange) {
          //{{{  full range
          y = *buf++;
          cr = *buf++;
          cb = *buf++;
          alpha = *buf++;
          }
          //}}}
        else {
          //{{{  not full range
          y = buf[0] & 0xFC;

          cr = (((buf[0] & 3) << 2) | (((buf[1] >> 6) & 3)) << 4);
          cb = (buf[1] << 2) & 0xF0;

          alpha = (buf[1] << 6) & 0xC0;

          buf += 2;
          }
          //}}}

        if ((depth & 0x40) && (entryId < 16)) {
          alpha = (y == 0) ? 0x0 : (0xFF - alpha);

          int rAdd = FIX(1.40200 * 255.0 / 224.0) * (cr - 128) + ONE_HALF;
          int gAdd = -FIX(0.34414 * 255.0 / 224.0) * (cb - 128) - FIX(0.71414 * 255.0 / 224.0) * (cr - 128) + ONE_HALF;
          int bAdd = FIX(1.77200 * 255.0 / 224.0) * (cb - 128) + ONE_HALF;
          y = (y - 16) * FIX(255.0 / 219.0);

          uint8_t r = ((y + rAdd) >> SCALEBITS) & 0xFF;
          uint8_t g = ((y + gAdd) >> SCALEBITS) & 0xFF;
          uint8_t b = ((y + bAdd) >> SCALEBITS) & 0xFF;
          colorLut.m16bgra[entryId] = BGRA(r,g,b,alpha);
          }
        else
          cLog::log (LOGERROR, fmt::format("colorLut depth:{} entryId:{}", depth, entryId));
        }
      }

    return true;
    }
  //}}}

  //{{{
  uint16_t parse4bit (const uint8_t*& buf, uint16_t bufSize,
                                           uint8_t* pixBuf, uint32_t pixBufSize, uint16_t pixPos,
                                           bool nonModifyColor) {

    pixBuf += pixPos;

    cBitStream bitStream (buf);
    while ((bitStream.getBitsRead() < (uint32_t)(bufSize * 8)) && (pixPos < pixBufSize)) {
      uint8_t bits = (uint8_t)bitStream.getBits (4);
      if (bits) {
        //{{{  simple pixel value
        if (!nonModifyColor || (bits != 1))
          *pixBuf++ = (uint8_t)bits;

        pixPos++;
        }
        //}}}
      else {
        bits = (uint8_t)bitStream.getBit();
        if (bits == 0) {
          //{{{  simple runlength
          uint8_t runLength = (uint8_t)bitStream.getBits (3);
          if (runLength == 0) {
            buf += bitStream.getBytesRead();
            return pixPos;
            }

          runLength += 2;
          bits = 0;
          while (runLength && (pixPos < pixBufSize)) {
            *pixBuf++ = bits;
            pixPos++;
            runLength--;
            }
          }
          //}}}
        else {
          bits = (uint8_t)bitStream.getBit();
          if (bits == 0) {
            //{{{  bits = 0
            uint8_t runBits = (uint8_t)bitStream.getBits (2);
            uint8_t runLength = runBits + 4;

            bits = (uint8_t)bitStream.getBits (4);
            if (nonModifyColor && (bits == 1))
              pixPos += runLength;
            else
              while (runLength && (pixPos < pixBufSize)) {
                *pixBuf++ = (uint8_t)bits;
                pixPos++;
                runLength--;
                }
            }
            //}}}
          else {
            bits = (uint8_t)bitStream.getBits (2);
            if (bits == 0) {
              //{{{  0
              bits = 0;
              *pixBuf++ = (uint8_t)bits;
              pixPos ++;
              }
              //}}}
            else if (bits == 1) {
              //{{{  1
              bits = 0;

              uint8_t runLength = 2;
              while (runLength && (pixPos < pixBufSize)) {
                *pixBuf++ = bits;
                pixPos++;
                runLength--;
                }
              }
              //}}}
            else if (bits == 2) {
              //{{{  2
              uint8_t runBits = (uint8_t)bitStream.getBits (4);
              uint8_t runLength = runBits + 9;

              bits = (uint8_t)bitStream.getBits (4);
              if (nonModifyColor && (bits == 1))
                pixPos += runLength;
              else
                while (runLength && (pixPos < pixBufSize)) {
                  *pixBuf++ = bits;
                  pixPos++;
                  runLength--;
                  }
              }
              //}}}
            else if (bits == 3) {
              //{{{  3
              uint8_t runBits = (uint8_t)bitStream.getBits (8);

              // !!! ouch - runLength wider than runBits
              uint16_t runLength = runBits + 25;

              bits = (uint8_t)bitStream.getBits (4);
              if (nonModifyColor && (bits == 1))
                pixPos += (uint8_t)runLength;
              else
                while (runLength && (pixPos < pixBufSize)) {
                  *pixBuf++ = bits;
                  pixPos++;
                  runLength--;
                  }
              }
              //}}}
            }
          }
        }
      }

    // trailing zero
    int bits = bitStream.getBits (8);
    if (bits)
      cLog::log (LOGERROR, "parse4bit - line overflow");

    buf += bitStream.getBytesRead();
    return pixPos;
    }
  //}}}
  //{{{
  void parseObjectBlock (cObject* object, const uint8_t* buf, uint16_t bufSize,
                                              bool bottom, bool nonModifyColor) {

    uint16_t xpos = object->mXpos;
    uint16_t ypos = object->mYpos + (bottom ? 1 : 0);

    cRegion& region = getRegion (object->mRegionId);

    const uint8_t* bufEnd = buf + bufSize;
    while (buf < bufEnd) {
      if (((*buf != 0xF0) && (xpos >= region.mWidth)) || (ypos >= region.mHeight)) {
        //{{{  error return
        cLog::log (LOGERROR, "parseObjectBlock - invalid object location %d %d %d %d %02x",
                              xpos, region.mWidth, ypos, region.mHeight, *buf);
        return;
        }
        //}}}

      uint8_t type = *buf++;
      switch (type) {
        case 0x11: // 4 bit
          if (region.mColorLutDepth < 4) {
            cLog::log (LOGERROR, fmt::format ("parseObjectBlock - 4bit pix string in {}:bit region",
                                              region.mColorLutDepth));
            return;
            }

          xpos = parse4bit (buf, uint16_t(bufEnd - buf),
                            region.mPixBuf + (ypos * region.mWidth), (uint32_t)region.mWidth, xpos, nonModifyColor);
          region.mDirty = true;
          break;

        case 0xF0: // end of line
          xpos = object->mXpos;
          ypos += 2;
          break;

        default:
          cLog::log (LOGINFO, fmt::format ("parseObjectBlock - unimplemented objectBlock {}", type));
          break;
        }
      }
    }
  //}}}
  //{{{
  bool parseObject (const uint8_t* buf, uint16_t bufSize) {

    const uint8_t* bufEnd = buf + bufSize;

    uint16_t objectId = AVRB16(buf);
    buf += 2;

    cObject* object = findObject (objectId);
    if (!object) // not declared by region, ignore
      return false;

    uint8_t codingMethod = ((*buf) >> 2) & 3;
    bool nonModifyColor = ((*buf++) >> 1) & 1;

    if (codingMethod == 0) {
      uint16_t topFieldLen = AVRB16(buf);
      buf += 2;
      uint16_t bottomFieldLen = AVRB16(buf);
      buf += 2;

      if ((buf + topFieldLen + bottomFieldLen) > bufEnd) {
        //{{{  error return
        cLog::log (LOGERROR, "parseObjectField - data size %d+%d too large", topFieldLen, bottomFieldLen);
        return false;
        }
        //}}}

      // decode object pixel data, rendered into object.mRegion.mPixBuf
      const uint8_t* block = buf;
      parseObjectBlock (object, block, topFieldLen, false, nonModifyColor);
      uint16_t bfl = bottomFieldLen;
      if (bottomFieldLen > 0)
        block = buf + topFieldLen;
      else
        bfl = topFieldLen;
      parseObjectBlock (object, block, bfl, true, nonModifyColor);
      }
    else
      cLog::log (LOGERROR, fmt::format ("parseObject - unknown object coding {}", codingMethod));

    return true;
    }
  //}}}

  //{{{
  void endDisplay() {

    int offsetX = mDisplayDefinition.mX;
    int offsetY = mDisplayDefinition.mY;

    size_t line = 0;
    for (auto& regionDisplay : mPage.mRegionDisplays) {
      cRegion& region = getRegion (regionDisplay.mRegionId);
      if (region.mDirty) {
        // region pixels changed
        cSubtitleImage& image = mPage.mImages[line];
        image.mPageState = mPage.mState;
        image.mPageVersion = mPage.mVersion;

        image.setXpos (regionDisplay.mXpos + offsetX);
        image.setYpos (regionDisplay.mYpos + offsetY);

        bool imageSizeChanged = (image.getWidth() != region.mWidth) || (image.getHeight() != region.mHeight);
        image.setWidth (region.mWidth);
        image.setHeight (region.mHeight);

        // copy lut
        memcpy (&image.mColorLut, &getColorLut (region.mColorLut).m16bgra, sizeof (image.mColorLut));

        // allocate pixels - ??? possible race with gui ????
        if (image.hasPixels() && imageSizeChanged)
          image.releasePixels();
        if (!image.hasPixels())
          image.allocPixels();
        image.setPixels (region.mWidth, region.mHeight, region.mPixBuf);

        // reset region dirty flag
        region.mDirty = false;
        }
      line++;
      }

    mPage.mNumLines = line;
    if (line > mPage.mHighwaterMark)
      mPage.mHighwaterMark = line;
    }
  //}}}

  // vars
  cRender& mRender;

  cDisplayDefinition mDisplayDefinition;

  cPage mPage;
  std::vector <cRegion*> mRegions;
  std::vector <cObject> mObjects;
  std::vector <cColorLut> mColorLuts;
  };
