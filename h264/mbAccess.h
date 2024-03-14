#pragma once

extern void CheckAvailabilityOfNeighbors (sMacroblock* mb);
extern void CheckAvailabilityOfNeighborsMBAFF (sMacroblock* mb);
extern void CheckAvailabilityOfNeighborsNormal (sMacroblock* mb);

extern void getAffNeighbour (sMacroblock* mb, int xN, int yN, int mbSize[2], sPixelPos *pixelPos);
extern void getNonAffNeighbour (sMacroblock* mb, int xN, int yN, int mbSize[2], sPixelPos *pixelPos);
extern void get4x4Neighbour (sMacroblock* mb, int xN, int yN, int mbSize[2], sPixelPos *pixelPos);
extern void get4x4NeighbourBase (sMacroblock* mb, int blockX, int blockY, int mbSize[2], sPixelPos *pixelPos);

extern Boolean isMbAvailable (int mbIndex, sMacroblock* mb);
extern void getMbPos (sDecoder* decoder, int mbIndex, int mbSize[2], short* x, short* y);
extern void getMbBlockPosNormal (sBlockPos* picPos, int mbIndex, short* x, short* y);
extern void getMbBlockPosMbaff  (sBlockPos* picPos, int mbIndex, short* x, short* y);
