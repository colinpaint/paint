#pragma once

struct sBiContext {
  uint16_t state; // index into state-table CP
  uint8_t  MPS;   // least probable symbol 0/1 CP
  uint8_t  dummy; // for alignment
  };
void biContextInit (sBiContext* context, int qp, const char* ini);

class cCabacDecode {
public:
  void startDecoding (uint8_t* code_buffer, int firstbyte, int* codeLen);

  // vars
  int getBitsRead();
  uint32_t symbol (sBiContext* biContext);
  uint32_t symbolEqProb();
  uint32_t final();

  uint32_t range;
  uint32_t value;
  int      bitsLeft;
  uint8_t* codeStream;
  int*     codeStreamLen;

private:
  uint32_t getByte();
  uint32_t getWord();
  };

