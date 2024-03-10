#pragma once
#include "defines.h"

//{{{  values for unitType
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
  } NaluType;
//}}}
//{{{  values for nalRefIdc
typedef enum {
  NALU_PRIORITY_HIGHEST     = 3,
  NALU_PRIORITY_HIGH        = 2,
  NALU_PRIORITY_LOW         = 1,
  NALU_PRIORITY_DISPOSABLE  = 0
  } NalRefIdc;
//}}}
//{{{
// struct sNalu 
typedef struct nalu_t {
  int       startCodeLen; // 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  unsigned  len;          // Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  unsigned  maxSize;      // NAL Unit Buffer size
  int       forbiddenBit; // should be always FALSE
  NaluType  unitType;     // NALU_TYPE_xxxx
  NalRefIdc refId;        // NALU_PRIORITY_xxxx
  byte*     buf;          // contains the first byte followed by the EBSP
  uint16    lostPackets;  // true, if packet loss is detected
  } sNalu;
//}}}

//{{{
typedef struct annexBstruct {
  byte*  buffer;
  size_t bufferSize;
  byte*  bufferPtr;
  size_t bytesInBuffer;

  int    isFirstByteStreamNALU;
  int    nextStartCodeBytes;
  byte*  naluBuffer;
  } ANNEXB_t;
//}}}
extern ANNEXB_t* allocAnnexB (sDecoder* vidParam);
extern void openAnnexB (ANNEXB_t* annexB, byte* chunk, size_t chunkSize);
extern void resetAnnexB (ANNEXB_t* annexB);
extern void freeAnnexB (ANNEXB_t** p_annexB);

extern sNalu* allocNALU (int);
extern void freeNALU (sNalu* n);

extern void checkZeroByteVCL (sDecoder* vidParam, sNalu* nalu);
extern void checkZeroByteNonVCL (sDecoder* vidParam, sNalu* nalu);

extern int readNextNalu (sDecoder* vidParam, sNalu* nalu);
extern int RBSPtoSODB (byte* streamBuffer, int last_byte_pos);
