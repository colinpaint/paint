#pragma once
struct sBiContext;

struct sCabacDecode {
  uint32_t range;
  uint32_t value;
  int      bitsLeft;
  uint8_t* codeStream;
  int*     codeStreamLen;
  };

void arithmeticDecodeStartDecoding (sCabacDecode* cabacDecode, uint8_t* code_buffer, int firstbyte, int* codeLen);
int arithmeticDecodeBitsRead (sCabacDecode* cabacDecode);

uint32_t binaryArithmeticDecodeSymbol (sCabacDecode* dep, sBiContext* biContext);
uint32_t binaryArithmeticDecodeSymbolEqProb (sCabacDecode* cabacDecode);
uint32_t binaryArithmeticDecodeFinal (sCabacDecode* cabacDecode);
void binaryArithmeticInitContext (int qp, sBiContext* context, const char* ini);
