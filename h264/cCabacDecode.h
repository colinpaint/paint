#pragma once
struct sBiContext;

class cCabacDecode {
public:
  void startDecoding (uint8_t* code_buffer, int firstbyte, int* codeLen);

  void ipcmPreamble();

  int getBitsRead() { return ((*codeStreamLen) << 3) - bitsLeft; }
  uint32_t symbol (sBiContext* biContext);
  uint32_t symbolEqProb();
  uint32_t final();

  // vars
  int*     codeStreamLen;

private:
  uint32_t getByte();
  uint32_t getWord();

  // vars
  uint32_t range;
  uint32_t value;
  int      bitsLeft;
  uint8_t* codeStream;
  };

void binaryArithmeticInitContext (int qp, sBiContext* context, const char* ini);
