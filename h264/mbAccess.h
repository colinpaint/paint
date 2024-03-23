#pragma once

extern void checkNeighbours (sMacroBlock* mb);
extern void checkNeighboursMbAff (sMacroBlock* mb);
extern void CheckAvailabilityOfNeighborsNormal (sMacroBlock* mb);

extern void getAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos *pixelPos);
extern void getNonAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos *pixelPos);
extern void get4x4Neighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos *pixelPos);
extern void get4x4NeighbourBase (sMacroBlock* mb, int blockX, int blockY, int mbSize[2], sPixelPos *pixelPos);

extern Boolean isMbAvailable (int mbIndex, sMacroBlock* mb);
extern void getMbPos (sDecoder* decoder, int mbIndex, int mbSize[2], short* x, short* y);
extern void getMbBlockPosNormal (sBlockPos* picPos, int mbIndex, short* x, short* y);
extern void getMbBlockPosMbaff  (sBlockPos* picPos, int mbIndex, short* x, short* y);
