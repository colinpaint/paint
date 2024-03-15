#pragma once

extern void aridecoStartDecoding (sDecodingEnv* decodingEnv, unsigned char* code_buffer, int firstbyte, int* codeLen);
extern int aridecoBitsRead (sDecodingEnv* decodingEnv);

extern unsigned int biarDecodeSymbol (sDecodingEnv* dep, sBiContextType* biContext);
extern unsigned int biariDecodeSymbolEqProb (sDecodingEnv* decodingEnv);
extern unsigned int biariDecodeFinal (sDecodingEnv* decodingEnv);
extern void biariInitContext (int qp, sBiContextType* context, const char* ini);
