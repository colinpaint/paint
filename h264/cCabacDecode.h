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

  int getBitsRead();
  uint32_t symbol (sBiContext* biContext);
  uint32_t symbolEqProb();
  uint32_t final();

  uint32_t unary_bin_max_decode (sBiContext* context, int ctx_offset, uint32_t max_symbol);
  uint32_t unary_bin_decode (sBiContext* context, int ctx_offset);
  uint32_t exp_golomb_decode_eq_prob (int k);
  uint32_t unary_exp_golomb_level_decode (sBiContext* context);
  uint32_t unaryExpGolombMv (sBiContext* context, uint32_t max_bin);

  // vars
  uint32_t range;
  uint32_t value;
  int      bitsLeft;
  uint8_t* codeStream;
  int*     codeStreamLen;

private:
  uint32_t getByte();
  uint32_t getWord();
  };

