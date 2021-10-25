// cSubtitle.cpp
//{{{  includes
#include "cDvbSubtitle.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "../utils/cLog.h"

class cTexture;
//}}}
//{{{  defines
#define AVRB16(p) ((*(p) << 8) | *(p+1))

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
//}}}

// public:
//{{{
cDvbSubtitle::~cDvbSubtitle() {

  mColorLuts.clear();
  mRegions.clear();
  deleteObjects();

  while (mDisplayList) {
    cRegionDisplay* display = mDisplayList;
    mDisplayList = display->mNext;
    free (display);
    }

  for (auto image : mImages)
    delete image;
  mImages.clear();
  }
//}}}

//{{{
bool cDvbSubtitle::decode (const uint8_t* buf, int bufSize) {

  const uint8_t* bufEnd = buf + bufSize;
  const uint8_t* bufPtr = buf + 2;

  while (bufEnd - bufPtr >= 6) {
    // check for syncByte
    uint8_t syncByte = *bufPtr++;
    if (syncByte != 0x0f) {
      //{{{  syncByte error return
      cLog::log (LOGERROR, fmt::format ("cDvbSubtitle decode missing syncByte:{}", syncByte));
      return false;
      }
      //}}}

    uint8_t segmentType = *bufPtr++;

    uint16_t pageId = AVRB16(bufPtr);
    bufPtr += 2;

    uint16_t segmentLength = AVRB16(bufPtr);
    bufPtr += 2;

    if (segmentLength > bufEnd - bufPtr) {
      //{{{  segmentLength error return
      cLog::log (LOGERROR, "incomplete or broken packet");
      return false;
      }
      //}}}

    switch (segmentType) {
      //{{{
      case 0x10: // page composition segment
        if (!parsePage (bufPtr, segmentLength))
          return false;
        break;
      //}}}
      //{{{
      case 0x11: // region composition segment
        if (!parseRegion (bufPtr, segmentLength))
          return false;
        break;
      //}}}
      //{{{
      case 0x12: // CLUT definition segment
        if (!parseColorLut (bufPtr, segmentLength))
          return false;
        break;
      //}}}
      //{{{
      case 0x13: // object data segment
        if (!parseObject (bufPtr, segmentLength))
          return false;
        break;
      //}}}
      //{{{
      case 0x14: // display definition segment
        if (!parseDisplayDefinition (bufPtr, segmentLength))
          return false;
        break;
      //}}}
      //{{{
      case 0x80: // end of display set segment
        endDisplaySet();
        return true;
      //}}}
      //{{{
      case 0x15: // disparity signalling segment
        cLog::log (LOGERROR, "disparity signalling segment");
        break;
      //}}}
      //{{{
      case 0x16: // alternative_CLUT_segment
        cLog::log (LOGERROR, "alternative_CLUT_segment");
        break;
      //}}}
      //{{{
      default:
        cLog::log (LOGERROR, "unknown seg:%x, pageId:%d, size:%d", segmentType, pageId, segmentLength);
        break;
      //}}}
      }

    bufPtr += segmentLength;
    }

  return false;
  }
//}}}

// private:
//{{{
cDvbSubtitle::cColorLut& cDvbSubtitle::getColorLut (uint8_t id) {

  // look for id colorLut
  for (auto& colorLut : mColorLuts)
    if (colorLut.mId == id)
      return colorLut;

  // create id colorLut
  mColorLuts.emplace_back (cColorLut (id));
  return mColorLuts.back();
  }
//}}}
//{{{
cDvbSubtitle::cObject* cDvbSubtitle::getObject (uint16_t id) {

  cObject* object = mObjectList;

  while (object && (object->mId != id))
    object = object->mNext;

  return object;
  }
//}}}
//{{{
cDvbSubtitle::cRegion* cDvbSubtitle::getRegion (uint8_t id) {

  for (auto& region : mRegions)
    if (region.mId == id)
      return &region;

  mRegions.emplace_back (cRegion (id));
  return &mRegions.back();
  }
//}}}

// parse
//{{{
bool cDvbSubtitle::parsePage (const uint8_t* buf, uint16_t bufSize) {

  //cLog::log (LOGINFO, "page");
  if (bufSize < 1)
    return false;
  const uint8_t* bufEnd = buf + bufSize;

  uint8_t pageTimeout = *buf++;
  uint8_t pageVersion = ((*buf) >> 4) & 15;
  if (mPageVersion == pageVersion)
    return true;
  mPageVersion = pageVersion;
  mPageState = ((*buf++) >> 2) & 3;
  cLog::log (LOGINFO,  fmt::format ("{:5d} {:12s} - page:{:1d} version::{:2d} timeout:{}",
                                    mSid, mName, mPageState, mPageVersion, pageTimeout));
  if ((mPageState == 1) || (mPageState == 2)) {
    //{{{  delete regions, objects, colorLuts
    mRegions.clear();
    deleteObjects();
    mColorLuts.clear();
    }
    //}}}

  cRegionDisplay* tmpDisplayList = mDisplayList;
  mDisplayList = NULL;
  while (buf + 5 < bufEnd) {
    uint8_t regionId = *buf++;
    buf += 1;

    cRegionDisplay* display = mDisplayList;
    while (display && (display->mRegionId != regionId))
      display = display->mNext;
    if (display) {
      //{{{  duplicate
      cLog::log (LOGERROR, "duplicate region");
      break;
      }
      //}}}

    display = tmpDisplayList;
    cRegionDisplay** tmpPtr = &tmpDisplayList;
    while (display && (display->mRegionId != regionId)) {
      tmpPtr = &display->mNext;
      display = display->mNext;
      }

    if (!display) {
      display = (cRegionDisplay*)malloc (sizeof(cRegionDisplay));
      display->mNext = nullptr;
      }
    display->mRegionId = regionId;

    display->xPos = AVRB16(buf);
    buf += 2;
    display->yPos = AVRB16(buf);
    buf += 2;

    *tmpPtr = display->mNext;
    display->mNext = mDisplayList;
    mDisplayList = display;
    //cLog::log (LOGINFO, fmt::format ("- regionId:{} {} {}",regionId,display->xPos,display->yPos));
    }

  while (tmpDisplayList) {
    //{{{  free tmpDisplayList
    cRegionDisplay* display = tmpDisplayList;
    tmpDisplayList = display->mNext;
    free (display);
    }
    //}}}

  return true;
  }
//}}}
//{{{
bool cDvbSubtitle::parseRegion (const uint8_t* buf, uint16_t bufSize) {

  //cLog::log (LOGINFO, "region segment");
  if (bufSize < 10)
    return false;

  const uint8_t* bufEnd = buf + bufSize;

  uint8_t regionId = *buf++;
  cRegion* region = getRegion (regionId);
  region->mVersion = ((*buf) >> 4) & 0x0F;

  bool fill = ((*buf++) >> 3) & 1;
  region->mWidth = AVRB16(buf);
  buf += 2;
  region->mHeight = AVRB16(buf);
  buf += 2;
  if ((region->mWidth * region->mHeight) != region->mPixBufSize) {
    region->mPixBufSize = region->mWidth * region->mHeight;
    region->mPixBuf = (uint8_t*)realloc (region->mPixBuf, region->mPixBufSize);
    region->mDirty = false;
    fill = true;
    }

  region->mDepth = 1 << (((*buf++) >> 2) & 7);
  if (region->mDepth != 4) {
    //{{{  error return, allow 2 and 8 when we see them
    cLog::log (LOGERROR, fmt::format ("unknown region depth:{}", region->mDepth));
    return false;
    }
    //}}}

  region->mColorLut = *buf++;
  if (region->mDepth == 8) {
    region->mBackgroundColour = *buf++;
    buf += 1;
    }
  else {
    buf += 1;
    if (region->mDepth == 4)
      region->mBackgroundColour = ((*buf++) >> 4) & 15;
    else
      region->mBackgroundColour = ((*buf++) >> 2) & 3;
    }
  if (fill)
    memset (region->mPixBuf, region->mBackgroundColour, region->mPixBufSize);

  deleteRegionDisplayList (region);

  while (buf + 5 < bufEnd) {
    uint16_t objectId = AVRB16(buf);
    buf += 2;
    cObject* object = getObject (objectId);
    if (!object) {
      // allocate and init object
      object = (cObject*)malloc (sizeof(cObject));

      object->mId = objectId;
      object->mType = 0;
      object->mNext = mObjectList;
      object->mDisplayList = nullptr;
      mObjectList = object;
      }
    object->mType = (*buf) >> 6;

    int xpos = AVRB16(buf) & 0xFFF;
    buf += 2;
    int ypos = AVRB16(buf) & 0xFFF;
    buf += 2;
    auto display = (cObjectDisplay*)malloc (sizeof(cObjectDisplay));
    display->init (objectId, regionId, xpos, ypos);

    if (display->xPos >= region->mWidth ||
      //{{{  error return
      display->yPos >= region->mHeight) {

      cLog::log (LOGERROR, "Object outside region");
      free (display);

      return false;
      }
      //}}}

    if (((object->mType == 1) || (object->mType == 2)) && (buf+1 < bufEnd)) {
      display->mForegroundColour = *buf++;
      display->mBackgroundColour = *buf++;
      }

    display->mRegionListNext = region->mDisplayList;
    region->mDisplayList = display;

    display->mObjectListNext = object->mDisplayList;
    object->mDisplayList = display;
    }

  return true;
  }
//}}}
//{{{
bool cDvbSubtitle::parseColorLut (const uint8_t* buf, uint16_t bufSize) {

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
int cDvbSubtitle::parse4bit (const uint8_t** buf, uint16_t bufSize,
                             uint8_t* pixBuf, uint32_t pixBufSize, uint32_t pixPos, bool nonModifyColour) {

  pixBuf += pixPos;

  cBitStream bitStream (*buf);
  while ((bitStream.getBitsRead() < (uint32_t)(bufSize * 8)) && (pixPos < pixBufSize)) {
    uint8_t bits = (uint8_t)bitStream.getBits (4);
    if (bits) {
      //{{{  simple pixel value
      if (!nonModifyColour || (bits != 1))
        *pixBuf++ = (uint8_t)bits;

      pixPos++;
      }
      //}}}
    else {
      bits = (uint8_t)bitStream.getBit();
      if (bits == 0) {
        //{{{  simple runlength
        int runLength = (uint8_t)bitStream.getBits (3);
        if (runLength == 0) {
          *buf += bitStream.getBytesRead();
          return pixPos;
          }

        runLength += 2;
        bits = 0;
        while ((runLength-- > 0) && (pixPos < pixBufSize)) {
          *pixBuf++ = (uint8_t)bits;
          pixPos++;
          }
        }
        //}}}
      else {
        bits = (uint8_t)bitStream.getBit();
        if (bits == 0) {
          //{{{  bits = 0
          uint8_t runBits = (uint8_t)bitStream.getBits (2);
          int runLength = runBits + 4;

          bits = (uint8_t)bitStream.getBits (4);
          if (nonModifyColour && (bits == 1))
            pixPos += runLength;
          else
            while ((runLength-- > 0) && (pixPos < pixBufSize)) {
              *pixBuf++ = (uint8_t)bits;
              pixPos++;
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

            int runLength = 2;
            while ((runLength-- > 0) && (pixPos < pixBufSize)) {
              *pixBuf++ = (uint8_t)bits;
              pixPos++;
              }
            }
            //}}}
          else if (bits == 2) {
            //{{{  2
            uint8_t runBits = (uint8_t)bitStream.getBits (4);
            int runLength = runBits + 9;

            bits = (uint8_t)bitStream.getBits (4);
            if (nonModifyColour && (bits == 1))
              pixPos += runLength;
            else
              while ((runLength-- > 0) && (pixPos < pixBufSize)) {
                *pixBuf++ = (uint8_t)bits;
                pixPos++;
                }
            }
            //}}}
          else if (bits == 3) {
            //{{{  3
            uint8_t runBits = (uint8_t)bitStream.getBits (8);
            int runLength = runBits + 25;

            bits = (uint8_t)bitStream.getBits (4);
            if (nonModifyColour && (bits == 1))
              pixPos += runLength;
            else
              while ((runLength-- > 0) && (pixPos < pixBufSize)) {
                *pixBuf++ = (uint8_t)bits;
                pixPos++;
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
    cLog::log (LOGERROR, "line overflow");

  *buf += bitStream.getBytesRead();
  return pixPos;
  }
//}}}
//{{{
void cDvbSubtitle::parseObjectBlock (cObjectDisplay* display, const uint8_t* buf, uint16_t bufSize,
                                     bool bottom, bool nonModifyColour) {

  cRegion* region = getRegion (display->mRegionId);
  if (!region)
    return;

  int xPos = display->xPos;
  int yPos = display->yPos + (bottom ? 1 : 0);

  region->mDirty = true;
  uint8_t* pixBuf = region->mPixBuf;
  const uint8_t* bufEnd = buf + bufSize;
  while (buf < bufEnd) {
    if (((*buf != 0xF0) && (xPos >= region->mWidth)) || (yPos >= region->mHeight)) {
      //{{{  error return
      cLog::log (LOGERROR, "invalid object location %d %d %d %d %02x",
                            xPos, region->mWidth, yPos, region->mHeight, *buf);
      return;
      }
      //}}}

    uint8_t type = *buf++;
    uint16_t bufLeft = uint16_t(bufEnd - buf);
    uint8_t* pixPtr = pixBuf + (yPos * region->mWidth);

    switch (type) {
      case 0x11: // 4 bit
        if (region->mDepth < 4) {
          cLog::log (LOGERROR, "4-bit pix string in %d-bit region!", region->mDepth);
          return;
          }
        xPos = parse4bit (&buf, bufLeft, pixPtr, region->mWidth, xPos, nonModifyColour);
        break;

      case 0xF0: // end of line
        xPos = display->xPos;
        yPos += 2;
        break;

      default:
        cLog::log (LOGINFO, "unimplemented objectBlock %x", type);
        break;
      }
    }
  }
//}}}
//{{{
bool cDvbSubtitle::parseObject (const uint8_t* buf, uint16_t bufSize) {

  //cLog::log (LOGINFO, "object segment");
  const uint8_t* bufEnd = buf + bufSize;

  uint16_t objectId = AVRB16(buf);
  buf += 2;
  cObject* object = getObject (objectId);
  if (!object)
    return false;

  uint8_t codingMethod = ((*buf) >> 2) & 3;
  bool nonModifyColour = ((*buf++) >> 1) & 1;

  if (codingMethod == 0) {
    uint16_t topFieldLen = AVRB16(buf);
    buf += 2;
    uint16_t bottomFieldLen = AVRB16(buf);
    buf += 2;

    if ((buf + topFieldLen + bottomFieldLen) > bufEnd) {
      //{{{  error return
      cLog::log (LOGERROR, "Field data size %d+%d too large", topFieldLen, bottomFieldLen);
      return false;
      }
      //}}}

    for (auto display = object->mDisplayList; display; display = display->mObjectListNext) {
      const uint8_t* block = buf;
      parseObjectBlock (display, block, topFieldLen, false, nonModifyColour);

      uint16_t bfl = bottomFieldLen;
      if (bottomFieldLen > 0)
        block = buf + topFieldLen;
      else
        bfl = topFieldLen;
      parseObjectBlock (display, block, bfl, true, nonModifyColour);
      }
    }
  else
    cLog::log (LOGERROR, "unknown object coding %d", codingMethod);

  return true;
  }
//}}}

//{{{
bool cDvbSubtitle::parseDisplayDefinition (const uint8_t* buf, uint16_t bufSize) {

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

  cLog::log (LOGINFO, fmt::format ("{:5d} {:12s} - display{} x:{} y:{} w:{} h:{}",
                                   mSid, mName,
                                   displayWindow != 0 ? " window" : "",
                                   mDisplayDefinition.mX, mDisplayDefinition.mY,
                                   mDisplayDefinition.mWidth, mDisplayDefinition.mHeight));
  return true;
  }
//}}}
//{{{
bool cDvbSubtitle::endDisplaySet() {

  int offsetX = mDisplayDefinition.mX;
  int offsetY = mDisplayDefinition.mY;

  mNumImages = 0;
  for (cRegionDisplay* regionDisplay = mDisplayList; regionDisplay; regionDisplay = regionDisplay->mNext) {
    cRegion* region = getRegion (regionDisplay->mRegionId);
    if (!region || !region->mDirty)
      continue;

    if (mNumImages == mImages.size())
      mImages.emplace_back (new cSubtitleImage());

    cSubtitleImage& image = *mImages[mNumImages];
    image.mPageState = mPageState;
    image.mPageVersion = mPageVersion;

    image.mX = regionDisplay->xPos + offsetX;
    image.mY = regionDisplay->yPos + offsetY;
    image.mWidth = region->mWidth;
    image.mHeight = region->mHeight;

    // copy lut
    memcpy (&image.mColorLut, &getColorLut (region->mColorLut).m16bgra, sizeof (image.mColorLut));

    // allocate pixels
    image.mPixels = (uint8_t*)realloc (image.mPixels, image.mWidth * image.mHeight * sizeof(uint32_t));

    // region->mPixBuf -> lut -> mPixels
    uint32_t* ptr = (uint32_t*)image.mPixels;
    for (int i = 0; i < image.mWidth * image.mHeight; i++)
      *ptr++ = image.mColorLut[region->mPixBuf[i]];

    // set changed flag, to update texture in gui
    image.mDirty = true;

    // update num regions as they become valid
    mNumImages++;
    }

  return true;
  }
//}}}

// delete
//{{{
void cDvbSubtitle::deleteObjects() {

  uint32_t num = 0;
  while (mObjectList) {
    cObject* object = mObjectList;
    mObjectList = object->mNext;
    free (object);
    num++;
    }

  cLog::log (LOGINFO1, fmt::format ("deleteObjects {}", num));
  }
//}}}
//{{{
void cDvbSubtitle::deleteRegionDisplayList (cRegion* region) {

  uint32_t num = 0;
  uint32_t num1 = 0;

  while (region->mDisplayList) {
    cObjectDisplay* display = region->mDisplayList;
    cObject* object = getObject (display->mObjectId);
    if (object) {
      cObjectDisplay** objDispPtr = &object->mDisplayList;
      cObjectDisplay* objDisp = *objDispPtr;

      while (objDisp && (objDisp != display)) {
        objDispPtr = &objDisp->mObjectListNext;
        objDisp = *objDispPtr;
        }

      if (objDisp) {
        *objDispPtr = objDisp->mObjectListNext;

        if (!object->mDisplayList) {
          cObject** object2Ptr = &mObjectList;
          cObject* object2 = *object2Ptr;

          while (object2 != object) {
            object2Ptr = &object2->mNext;
            object2 = *object2Ptr;
            }

          *object2Ptr = object2->mNext;
          free (object2);
          num1++;
          }
        }
      }

    region->mDisplayList = display->mRegionListNext;
    free (display);
    num++;
    }

  cLog::log (LOGINFO1, fmt::format ("deleteRegionDisplayList {} {}", num, num1));
  }
//}}}
