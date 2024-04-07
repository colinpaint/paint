#pragma once
struct sBiContext;

struct sCabacDecode {
  void arithmeticDecodeStartDecoding (uint8_t* code_buffer, int firstbyte, int* codeLen);
  int arithmeticDecodeBitsRead();

  uint32_t binaryArithmeticDecodeSymbol (sBiContext* biContext);
  uint32_t binaryArithmeticDecodeSymbolEqProb();
  uint32_t binaryArithmeticDecodeFinal();

  uint32_t getByte();
  uint32_t getWord();

  uint32_t range;
  uint32_t value;
  int      bitsLeft;
  uint8_t* codeStream;
  int*     codeStreamLen;
  };

void binaryArithmeticInitContext (int qp, sBiContext* context, const char* ini);
