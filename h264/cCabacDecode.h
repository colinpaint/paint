#pragma once
struct sMotionContexts;
struct sTextureContexts;
struct sSyntaxElement;
struct sMacroBlock;
class cSlice;

struct sBiContext {
  //{{{
  void init (int qp, const char* ini) {

    state = ((ini[0] * qp) >> 4) + ini[1];
    if (state >= 64) {
      state = imin (126, state);
      state = (uint16_t)(state - 64);
      MPS = 1;
      }
    else {
      state = imax (1, state);
      state = (uint16_t)(63 - state);
      MPS = 0;
      }
    }
  //}}}
  uint16_t state; // index into state-table CP
  uint8_t  MPS;   // least probable symbol 0/1 CP
  uint8_t  dummy; // for alignment
  };

class cCabacDecode {
public:
  void startDecoding (uint8_t* code_buffer, int firstbyte, int* codeLen);

  void ipcmPreamble();

  uint32_t unary_bin_max_decode (sBiContext* context, int ctx_offset, uint32_t max_symbol);
  uint32_t unary_bin_decode (sBiContext* context, int ctx_offset);
  uint32_t exp_golomb_decode_eq_prob (int k);
  uint32_t unary_exp_golomb_level_decode (sBiContext* context);
  uint32_t unary_exp_golomb_mv_decode (sBiContext* context, uint32_t max_bin);

  int getBitsRead() { return ((*codeStreamLen) << 3) - bitsLeft; }
  uint32_t getSymbol (sBiContext* biContext);
  uint32_t getSymbolEqProb();
  uint32_t getFinal();

  // vars
  int*     codeStreamLen;

private:
  uint32_t getByte() { return codeStream[(*codeStreamLen)++]; }
  uint32_t getWord();

  // vars
  uint32_t range;
  uint32_t value;
  int      bitsLeft;
  uint8_t* codeStream;
  };
