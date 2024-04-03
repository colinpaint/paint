#pragma once

extern void arithmeticDecodeStartDecoding (sCabacDecodeEnv* cabacDecodeEnv, uint8_t* code_buffer, int firstbyte, int* codeLen);
extern int arithmeticDecodeBitsRead (sCabacDecodeEnv* cabacDecodeEnv);

extern uint32_t binaryArithmeticDecodeSymbol (sCabacDecodeEnv* dep, sBiContext* biContext);
extern uint32_t binaryArithmeticDecodeSymbolEqProb (sCabacDecodeEnv* cabacDecodeEnv);
extern uint32_t binaryArithmeticDecodeFinal (sCabacDecodeEnv* cabacDecodeEnv);
extern void binaryArithmeticInitContext (int qp, sBiContext* context, const char* ini);
