#pragma once
#include "nalu.h"

typedef struct annexBstruct {
  // buffer
  byte* buffer;
  size_t bufferSize;

  // curBuffer position
  byte* bufferPtr;
  size_t bytesInBuffer;

  // NALU buffer
  int isFirstByteStreamNALU;
  int nextStartCodeBytes;
  byte* naluBuf;
  } ANNEXB_t;

extern ANNEXB_t* allocAnnexB (VideoParameters* p_Vid);
extern void openAnnexB (ANNEXB_t* annexB, byte* chunk, size_t chunkSize);
extern void resetAnnexB (ANNEXB_t* annexB);
extern void freeAnnexB (ANNEXB_t** p_annexB);

extern int getNALU (ANNEXB_t* annexB, VideoParameters* p_Vid, NALU_t* nalu);
