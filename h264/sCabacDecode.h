#pragma once
struct sBiContext;

struct sCabacDecode {
  void startDecoding (uint8_t* code_buffer, int firstbyte, int* codeLen);
  int bitsRead();

  // vars
  uint32_t symbol (sBiContext* biContext);
  uint32_t symbolEqProb();
  uint32_t final();

  uint32_t getByte();
  uint32_t getWord();

  uint32_t range;
  uint32_t value;
  int      bitsLeft;
  uint8_t* codeStream;
  int*     codeStreamLen;
  };

void binaryArithmeticInitContext (int qp, sBiContext* context, const char* ini);
