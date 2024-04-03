#pragma once
#include "global.h"

struct sAnnexB {
  uint8_t*  buffer;
  size_t bufferSize;
  uint8_t*  bufferPtr;
  size_t bytesInBuffer;

  int    isFirstByteStreamNALU;
  int    nextStartCodeBytes;
  uint8_t*  naluBuffer;
  };

//{{{  enum eNaluType
typedef enum {
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
struct sNalu {
  int        startCodeLen; // 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  uint32_t   len;          // Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  uint32_t   maxSize;      // NAL Unit Buffer size
  int        forbiddenBit; // should be always false
  eNaluType  unitType;     // NALU_TYPE_xxxx
  eNalRefIdc refId;        // NALU_PRIORITY_xxxx
  uint8_t*   buf;          // contains the first uint8_t followed by the EBSP
  uint16_t   lostPackets;  // true, if packet loss is detected
  };

struct sDecoder;
sAnnexB* allocAnnexB (sDecoder* decoder);
void openAnnexB (sAnnexB* annexB, uint8_t* chunk, size_t chunkSize);
void resetAnnexB (sAnnexB* annexB);
void freeAnnexB (sAnnexB** annexB);

sNalu* allocNALU (int);
void freeNALU (sNalu* n);

void checkZeroByteVCL (sDecoder* decoder, sNalu* nalu);
void checkZeroByteNonVCL (sDecoder* decoder, sNalu* nalu);

int readNalu (sDecoder* decoder, sNalu* nalu);
int RBSPtoSODB (uint8_t* bitStreamBuffer, int lastBytePos);
