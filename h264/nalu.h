#pragma once
#include "global.h"

struct sDecoder;
class sAnnexB {
public:
  sAnnexB (sDecoder* decoder);
  ~sAnnexB();

  void open (uint8_t* chunk, size_t chunkSize);
  void reset();

  uint8_t getByte();

  uint8_t*  buffer = nullptr;
  size_t    bufferSize = 0;
  uint8_t*  bufferPtr = nullptr;
  size_t    bytesInBuffer = 0;

  bool      isFirstByteStreamNALU = false;
  int       nextStartCodeBytes = 0;
  uint8_t*  naluBuffer = nullptr;
  };

class sNalu {
public:
  //{{{  enum eNaluType
  typedef enum {
    NALU_TYPE_NONE     = 0,
    NALU_TYPE_SLICE    = 1,
    NALU_TYPE_DPA      = 2,
    NALU_TYPE_DPB      = 3,
    NALU_TYPE_DPC      = 4,
    NALU_TYPE_IDR      = 5,
    NALU_TYPE_SEI      = 6,
    NALU_TYPE_SPS      = 7,
    NALU_TYPE_PPS      = 8,

    NALU_TYPE_AUD      = 9,
    NALU_TYPE_EOSEQ    = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL     = 12,
    NALU_TYPE_PREFIX   = 14,
    NALU_TYPE_SUB_SPS  = 15,
    NALU_TYPE_SLC_EXT  = 20,
    NALU_TYPE_VDRD     = 24  // View and Dependency Representation Delimiter NAL Unit
    } eNaluType;
  //}}}
  //{{{  enum eNalRefIdc
  typedef enum {
    NALU_PRIORITY_DISPOSABLE  = 0,
    NALU_PRIORITY_LOW         = 1,
    NALU_PRIORITY_HIGH        = 2,
    NALU_PRIORITY_HIGHEST     = 3
    } eNalRefIdc;
  //}}}

  sNalu (int bufferSize);
  ~sNalu();

  int readNalu (sDecoder* decoder);
  void debug();

  int NALUtoRBSP();
  int RBSPtoSODB (uint8_t* bitStreamBuffer);

  void checkZeroByteVCL (sDecoder* decoder);
  void checkZeroByteNonVCL (sDecoder* decoder);

  int        startCodeLen = 0; // 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  uint32_t   len = 0;          // Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  uint32_t   maxSize = 0;      // NAL Unit Buffer size
  int        forbiddenBit = 0; // should be always false
  eNaluType  unitType = NALU_TYPE_NONE;        // NALU_TYPE_xxxx
  eNalRefIdc refId = NALU_PRIORITY_DISPOSABLE; // NALU_PRIORITY_xxxx
  uint8_t*   buf = nullptr;    // contains the first uint8_t followed by the EBSP
  uint16_t   lostPackets = 0 ; // true, if packet loss is detected

private:
  int getNALU (sAnnexB* annexB, sDecoder* decoder);
  };


int RBSPtoSODB (uint8_t* bitStreamBuffer, int lastBytePos);
