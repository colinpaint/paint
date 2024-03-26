#pragma once

extern void arithmeticDecodeStartDecoding (sCabacDecodeEnv* cabacDecodeEnv, unsigned char* code_buffer, int firstbyte, int* codeLen);
extern int arithmeticDecodeBitsRead (sCabacDecodeEnv* cabacDecodeEnv);

extern unsigned int binaryArithmeticDecodeSymbol (sCabacDecodeEnv* dep, sBiContext* biContext);
extern unsigned int binaryArithmeticDecodeSymbolEqProb (sCabacDecodeEnv* cabacDecodeEnv);
extern unsigned int binaryArithmeticDecodeFinal (sCabacDecodeEnv* cabacDecodeEnv);
extern void binaryArithmeticInitContext (int qp, sBiContext* context, const char* ini);
