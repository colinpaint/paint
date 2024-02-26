// cDvbUtils.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>
#include <time.h>

#include "cDvbUtils.h"

#include "../common/cLog.h"

using namespace std;
//}}}
namespace {
  //{{{
  // CRC32 lookup table for polynomial 0x04c11db7
  const uint32_t kCrcTable[256] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
    0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
    0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
    0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
    0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
    0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
    0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
    0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
    0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
    0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
    0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
    0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
    0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
    0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
    0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
    0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
    0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
    };
  //}}}
  }

// cDvbUtils static members
//{{{
uint32_t cDvbUtils::getCrc32 (uint8_t* buf, uint32_t len) {

  uint32_t crc = 0xffffffff;
  for (uint32_t i = 0; i < len; i++)
    crc = (crc << 8) ^ kCrcTable[((crc >> 24) ^ *buf++) & 0xff];

  return crc;
  }
//}}}
//{{{
uint32_t cDvbUtils::getEpochTime (uint8_t* buf) {
  return (((buf[0] << 8) | buf[1]) - 40587) * 86400;
  }
//}}}
//{{{
uint32_t cDvbUtils::getBcdTime (uint8_t* buf)  {
  return (3600 * ((10 * ((buf[0] & 0xF0) >> 4)) + (buf[0] & 0xF))) +
           (60 * ((10 * ((buf[1] & 0xF0) >> 4)) + (buf[1] & 0xF))) +
                 ((10 * ((buf[2] & 0xF0) >> 4)) + (buf[2] & 0xF));
  }
//}}}
//{{{
int64_t cDvbUtils::getPts (const uint8_t* buf) {
// return 33 bits pts,dts

  int64_t pts = buf[0] & 0x0E;
  pts = (pts << 7) |  buf[1];
  pts = (pts << 8) | (buf[2] & 0xFE);
  pts = (pts << 7) |  buf[3];
  pts = (pts << 7) | (buf[4] >> 1);

  if (!(buf[0] & 0x01) || !(buf[2] & 0x01) || !(buf[4] & 0x01))
    // invalid marker bits
    cLog::log (LOGINFO2, fmt::format ("getPts marker bits {:02x}:{:02x}:{:02x}:{:02x}:{:02x} - 0:{:02x} 2:{:02x} 4:{:02x}",
                                 buf[0], buf[1], buf[2], buf[3], buf[4],
                                 buf[0] & 0x01, buf[2] & 0x01, buf[4] & 0x01));

  return pts;
  }
//}}}

//{{{
std::string cDvbUtils::getStreamTypeName (uint16_t streamType) {

  switch (streamType) {
     case   0: return "pgm";
     case   2: return "m2v"; // ISO 13818-2 video
     case   3: return "m2a"; // ISO 11172-3 audio
     case   4: return "m3a"; // ISO 13818-3 audio
     case   5: return "mtd"; // private mpeg2 tabled data - private
     case   6: return "sub"; // subtitle
     case  11: return "d11"; // dsm cc u_n
     case  13: return "d13"; // dsm cc tabled data
     case  15: return "aac"; // HD aud ADTS
     case  17: return "aac"; // HD aud LATM
     case  27: return "264"; // HD vid
     case 129: return "ac3"; // aud AC3
     case 134: return "???"; // ???
     default : return fmt::format ("{:3d}", streamType);
     }
  }
//}}}

//{{{
std::string cDvbUtils::getFrameInfo (uint8_t* pes, int64_t pesSize, bool h264) {

  //{{{
  class cBitstream {
  // used to parse H264 stream to find I frames
  public:
    cBitstream (const uint8_t* buffer, uint32_t bit_len) :
      mDecBuffer(buffer), mDecBufferSize(bit_len), mNumOfBitsInBuffer(0), mBookmarkOn(false) {}

    //{{{
    uint32_t getBits (uint32_t numBits) {

      //{{{
      static const uint32_t msk[33] = {
        0x00000000, 0x00000001, 0x00000003, 0x00000007,
        0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
        0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
        0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
        0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
        0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
        0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
        0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
        0xffffffff
        };
      //}}}

      if (numBits == 0)
        return 0;

      uint32_t retData;
      if (mNumOfBitsInBuffer >= numBits) {  // don't need to read from FILE
        mNumOfBitsInBuffer -= numBits;
        retData = mDecData >> mNumOfBitsInBuffer;
        // wmay - this gets done below...retData &= msk[numBits];
        }
      else {
        uint32_t nbits;
        nbits = numBits - mNumOfBitsInBuffer;
        if (nbits == 32)
          retData = 0;
        else
          retData = mDecData << nbits;

        switch ((nbits - 1) / 8) {
          case 3:
            nbits -= 8;
            if (mDecBufferSize < 8)
              return 0;
            retData |= *mDecBuffer++ << nbits;
            mDecBufferSize -= 8;
            // fall through
          case 2:
            nbits -= 8;
            if (mDecBufferSize < 8)
              return 0;
            retData |= *mDecBuffer++ << nbits;
            mDecBufferSize -= 8;
            // fall through
           case 1:
            nbits -= 8;
            if (mDecBufferSize < 8)
              return 0;
            retData |= *mDecBuffer++ << nbits;
            mDecBufferSize -= 8;
            // fall through
          case 0:
            break;
          }
        if (mDecBufferSize < nbits)
          return 0;

        mDecData = *mDecBuffer++;
        mNumOfBitsInBuffer = min(8u, mDecBufferSize) - nbits;
        mDecBufferSize -= min(8u, mDecBufferSize);
        retData |= (mDecData >> mNumOfBitsInBuffer) & msk[nbits];
        }

      return (retData & msk[numBits]);
      };
    //}}}
    //{{{
    uint32_t getUe() {

      uint32_t bits;
      uint32_t read = 0;
      int bits_left;
      bool done = false;
      bits = 0;

      // we want to read 8 bits at a time - if we don't have 8 bits,
      // read what's left, and shift.  The exp_golomb_bits calc remains the same.
      while (!done) {
        bits_left = bits_remain();
        if (bits_left < 8) {
          read = peekBits (bits_left) << (8 - bits_left);
          done = true;
          }
        else {
          read = peekBits (8);
          if (read == 0) {
            getBits (8);
            bits += 8;
            }
          else
           done = true;
          }
        }

      uint8_t coded = exp_golomb_bits[read];
      getBits (coded);
      bits += coded;

      return getBits (bits + 1) - 1;
      }
    //}}}
    //{{{
    int32_t getSe() {

      uint32_t ret;
      ret = getUe();
      if ((ret & 0x1) == 0) {
        ret >>= 1;
        int32_t temp = 0 - ret;
        return temp;
        }

      return (ret + 1) >> 1;
      }
    //}}}

  private:
    //{{{
    uint32_t peekBits (uint32_t bits) {

      bookmark (true);
      uint32_t ret = getBits (bits);
      bookmark (false);
      return ret;
      }
    //}}}
    //{{{
    const uint8_t exp_golomb_bits[256] = {
      8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0,
      };
    //}}}
    //{{{
    void bookmark (bool on) {

      if (on) {
        mNumOfBitsInBuffer_bookmark = mNumOfBitsInBuffer;
        mDecBuffer_bookmark = mDecBuffer;
        mDecBufferSize_bookmark = mDecBufferSize;
        mBookmarkOn = 1;
        mDecData_bookmark = mDecData;
        }

      else {
        mNumOfBitsInBuffer = mNumOfBitsInBuffer_bookmark;
        mDecBuffer = mDecBuffer_bookmark;
        mDecBufferSize = mDecBufferSize_bookmark;
        mDecData = mDecData_bookmark;
        mBookmarkOn = 0;
        }

      };
    //}}}

    //{{{
    int bits_remain() {
      return mDecBufferSize + mNumOfBitsInBuffer;
      };
    //}}}
    //{{{
    int byte_align() {

      int temp = 0;
      if (mNumOfBitsInBuffer != 0)
        temp = getBits (mNumOfBitsInBuffer);
      else {
        // if we are byte aligned, check for 0x7f value - this will indicate
        // we need to skip those bits
        uint8_t readval = static_cast<uint8_t>(peekBits (8));
        if (readval == 0x7f)
          readval = static_cast<uint8_t>(getBits (8));
        }

      return temp;
      };
    //}}}

    const uint8_t* mDecBuffer;
    uint32_t mDecBufferSize;
    uint32_t mNumOfBitsInBuffer;
    bool mBookmarkOn;

    uint8_t mDecData_bookmark = 0;
    uint8_t mDecData = 0;

    uint32_t mNumOfBitsInBuffer_bookmark = 0;
    const uint8_t* mDecBuffer_bookmark = 0;
    uint32_t mDecBufferSize_bookmark = 0;
    };
  //}}}

  uint8_t* pesEnd = pes + pesSize;

  if (h264) {
    string s;
    char frameType = '?';
    while (pes < pesEnd) {
      //{{{  skip past startcode, find next startcode
      uint8_t* buf = pes;
      int64_t bufSize = pesSize;

      uint32_t startOffset = 0;
      if (!buf[0] && !buf[1]) {
        if (!buf[2] && buf[3] == 1) {
          buf += 4;
          startOffset = 4;
          }
        else if (buf[2] == 1) {
          buf += 3;
          startOffset = 3;
          }
        }

      // find next startCode
      uint32_t offset = startOffset;
      uint32_t nalSize = offset;
      uint32_t val = 0xffffffff;
      while (offset++ < bufSize - 3) {
        val = (val << 8) | *buf++;
        if (val == 0x0000001) {
          nalSize = offset - 4;
          break;
          }
        if ((val & 0x00ffffff) == 0x0000001) {
          nalSize = offset - 3;
          break;
          }

        nalSize = (uint32_t)bufSize;
        }
      //}}}
      if (nalSize > 3) {
        // parse bitStream for NALs
        cBitstream bitstream (buf, (nalSize - startOffset) * 8);

        if (bitstream.getBits(1) != 0)
          s += '*';
        bitstream.getBits (2);

        int nalType = bitstream.getBits (5);
        switch (nalType) {
          //{{{
          case 1: { // nonIdr
            bitstream.getUe();
            int nalSubtype = bitstream.getUe();

            switch (nalSubtype) {
              case 0:
                frameType = 'P';
                s += fmt::format ("NonIDR:{}:{} ", nalSubtype, frameType);
                return fmt::format ("{} {}", frameType, s);
                break;

              case 1:
                frameType = 'B';
                s += fmt::format ("NonIDR:{}:{} ", nalSubtype, frameType);
                return fmt::format ("{} {}", frameType, s);
                break;

              case 2:
                frameType = 'I';
                s += fmt::format ("NonIDR:{}:{} ", nalSubtype, frameType);
                return fmt::format ("{} {}", frameType, s);
                break;

              default:
                s += fmt::format ("NonIDR:unknown:{} ", nalSubtype);
                return fmt::format ("{} {}", frameType, s);
                break;
              }

            break;
            }
          //}}}
          //{{{
          case 2:   // parta
            s += "PARTA ";
            break;
          //}}}
          //{{{
          case 3:   // partb
            s += "partB ";
            break;
          //}}}
          //{{{
          case 4:   // partc
            s += "partC ";
            break;
          //}}}
          //{{{
          case 5: { // idr
            bitstream.getUe();
            int nalSubtype = bitstream.getUe();

            switch (nalSubtype) {
              case 1:
              case 5:
                frameType = 'P';
                s += fmt::format ("IDR:{}:{} ", nalSubtype, frameType);
                return fmt::format ("{} {}", frameType, s);
                break;

              case 2:
              case 6:
                frameType =  'B';
                s += fmt::format ("IDR:{}:{} ", nalSubtype, frameType);
                return fmt::format ("{} {}", frameType, s);
                break;

              case 3:
              case 7:
                frameType =  'I';
                s += fmt::format ("IDR:{}:{} ", nalSubtype, frameType);
                return fmt::format ("{} {}", frameType, s);
                break;

              default:
                s += fmt::format ("IDR:unknown:{} ", nalSubtype);
                break;
              }

            break;
            }
          //}}}
          //{{{
          case 6:   // sei
            s += "SEI ";
            break;
          //}}}
          //{{{
          case 7:   // sps
            s += "SPS ";
            break;
          //}}}
          //{{{
          case 8:   // pps
            s += "PPS ";
            break;
          //}}}
          //{{{
          case 9:   // avd
            s += "AVD ";
            break;
          //}}}
          //{{{
          case 10:  // eoSeq
            s += "EOseq";
            break;
          //}}}
          //{{{
          case 11:  // eoStream
            s += "EOstream ";
            break;
          //}}}
          //{{{
          case 12:  // filler
            s += "Fill ";
            break;
          //}}}
          //{{{
          case 13:  // seqext
            s += "SeqExt ";
            break;
          //}}}
          //{{{
          case 14:  // prefix
            s += "PFX ";
            break;
          //}}}
          //{{{
          case 15:  // subsetSps
            s += "SubSPS ";
            break;
          //}}}
          //{{{
          case 19:  // aux
            s += "AUX ";
            break;
          //}}}
          //{{{
          case 20:  // sliceExt
            s += "SliceExt ";
            break;
          //}}}
          //{{{
          case 21:  // sliceExtDepth
            s += "SliceExtDepth ";
            break;
          //}}}
          //{{{
          default:
            s += fmt::format ("nal:{}:size:{} ", nalType, nalSize);
            break;
          //}}}
          }
        }
      pes += nalSize;
      }
    return fmt::format ("{} {}", frameType, s);
    }
  else {
    //{{{  mpeg2
    while (pes + 6 < pesEnd) {
      // look for pictureHeader 00000100
      if (!pes[0] && !pes[1] && (pes[2] == 0x01) && !pes[3])
        // extract frameType I,B,P
        switch ((pes[5] >> 3) & 0x03) {
          case 1:
            return "I";
          case 2:
            return "P";
          case 3:
            return "B";
          default:
            return "?";
        }
      pes++;
      }

    return "?";
    }
    //}}}
  }
//}}}
