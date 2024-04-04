#pragma once

extern void checkNeighbours (sMacroBlock* mb);
extern void checkNeighboursMbAff (sMacroBlock* mb);
extern void checkNeighboursNormal (sMacroBlock* mb);

extern void getAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos *pixelPos);
extern void getNonAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos *pixelPos);
extern void get4x4Neighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos *pixelPos);
extern void get4x4NeighbourBase (sMacroBlock* mb, int blockX, int blockY, int mbSize[2], sPixelPos *pixelPos);

extern bool isMbAvailable (int mbIndex, sMacroBlock* mb);
extern void getMbPos (cDecoder264* decoder, int mbIndex, int mbSize[2], int16_t* x, int16_t* y);
extern void getMbBlockPosNormal (sBlockPos* picPos, int mbIndex, int16_t* x, int16_t* y);
extern void getMbBlockPosMbaff  (sBlockPos* picPos, int mbIndex, int16_t* x, int16_t* y);
