#pragma once
#include "global.h"
class cDecoder264;

class cAnnexB {
public:
  cAnnexB (cDecoder264* decoder, uint32_t size);
  ~cAnnexB();

  // gets
  bool getLongStartCode() const { return longStartCode; }
  uint8_t* getNaluData() { return naluBuffer; }

  // actions
  void open (uint8_t* chunk, size_t chunkSize);
  void reset();
  uint32_t readNalu();

private:
  // vars
  uint8_t*  buffer = nullptr;
  size_t    allocBufferSize = 0;
  size_t    bufferSize = 0;

  // current ptr
  uint8_t*  bufferPtr = nullptr;
  size_t    bufferLeft = 0;

  bool      startCodeFound = false;
  bool      longStartCode = false;
  uint8_t*  naluBuffer = nullptr;
  };

class cNalu {
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
  //{{{  enum eNalRefId
  typedef enum {
    NALU_PRIORITY_DISPOSABLE  = 0,
    NALU_PRIORITY_LOW         = 1,
    NALU_PRIORITY_HIGH        = 2,
    NALU_PRIORITY_HIGHEST     = 3
    } eNalRefId;
  //}}}
  cNalu (uint32_t size);
  ~cNalu();

  // gets
  uint8_t* getBuffer() { return buffer; }
  uint32_t getLength() const { return uint32_t(naluBytes); }

  uint8_t* getPayload() { return buffer+1; }
  uint32_t getPayloadLength() const { return uint32_t(naluBytes-1); }
  eNaluType getUnitType() const { return unitType; }
  eNalRefId getRefId() const { return refId; }

  std::string getNaluString() const;

  // actions
  uint32_t readNalu (cDecoder264* decoder);

  void checkZeroByteVCL (cDecoder264* decoder);
  void checkZeroByteNonVCL (cDecoder264* decoder);

  int NALUtoRBSP();
  int RBSPtoSODB (uint8_t* bitStreamBuffer);

private:
  void debug();

  // vars
  uint8_t*  buffer = nullptr;
  uint32_t  allocBufferSize = 0;
  int32_t   naluBytes = 0;

  bool      longStartCode = false;
  bool      forbiddenBit = false;
  eNaluType unitType = NALU_TYPE_NONE;
  eNalRefId refId = NALU_PRIORITY_DISPOSABLE;
  };
