#pragma once

extern void aridecoStartDecoding (sDecodeEnv* decodeEnv, unsigned char* code_buffer, int firstbyte, int* codeLen);
extern int aridecoBitsRead (sDecodeEnv* decodeEnv);

extern unsigned int biarDecodeSymbol (sDecodeEnv* dep, sBiContextType* biContext);
extern unsigned int biariDecodeSymbolEqProb (sDecodeEnv* decodeEnv);
extern unsigned int biariDecodeFinal (sDecodeEnv* decodeEnv);
extern void biariInitContext (int qp, sBiContextType* context, const char* ini);
