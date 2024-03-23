#pragma once

extern void arithmeticDecodeStartDecoding (sDecodeEnv* decodeEnv, unsigned char* code_buffer, int firstbyte, int* codeLen);
extern int arithmeticDecodeBitsRead (sDecodeEnv* decodeEnv);

extern unsigned int binaryArithmeticDecodeSymbol (sDecodeEnv* dep, sBiContextType* biContext);
extern unsigned int binaryArithmeticDecodeSymbolEqProb (sDecodeEnv* decodeEnv);
extern unsigned int binaryArithmeticDecodeFinal (sDecodeEnv* decodeEnv);
extern void binaryArithmeticInitContext (int qp, sBiContextType* context, const char* ini);
