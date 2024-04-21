#pragma once

struct sBiContext {
  uint16_t state; // index into state-table CP
  uint8_t  MPS;   // least probable symbol 0/1 CP
  uint8_t  dummy; // for alignment
  };
void biContextInit (sBiContext* context, int qp, const char* ini);

struct sCabacDecode {
  void startDecoding (uint8_t* code_buffer, int firstbyte, int* mCodeLen);

  int getBitsRead();
  uint32_t getByte();
  uint32_t getWord();

  uint32_t getSymbol (sBiContext* biContext);
  uint32_t getSymbolEqProb();
  uint32_t getFinal();

  uint32_t unaryBin (sBiContext* context, int ctx_offset);
  uint32_t unaryBinMax (sBiContext* context, int ctx_offset, uint32_t max_symbol);
  uint32_t unaryExpGolombMv (sBiContext* context, uint32_t max_bin);
  uint32_t unaryExpGolombLevel (sBiContext* context);
  uint32_t expGolombEqProb (int k);

  // vars
  uint32_t range;
  uint32_t value;
  int      bitsLeft;
  uint8_t* codeStream;
  int*     codeStreamLen;
  };
