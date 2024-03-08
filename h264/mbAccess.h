#pragma once

extern void CheckAvailabilityOfNeighbors (sMacroblock* curMb);
extern void CheckAvailabilityOfNeighborsMBAFF (sMacroblock* curMb);
extern void CheckAvailabilityOfNeighborsNormal (sMacroblock* curMb);

extern void getAffNeighbour (sMacroblock* curMb, int xN, int yN, int mbSize[2], sPixelPos *pix);
extern void getNonAffNeighbour (sMacroblock* curMb, int xN, int yN, int mbSize[2], sPixelPos *pix);
extern void get4x4Neighbour (sMacroblock* curMb, int xN, int yN, int mbSize[2], sPixelPos *pix);
extern void get4x4NeighbourBase (sMacroblock* curMb, int blockX, int blockY, int mbSize[2], sPixelPos *pix);

extern Boolean mb_is_available (int mbAddr, sMacroblock* curMb);
extern void get_mb_pos (sVidParam* vidParam, int mbAddr, int mbSize[2], short *x, short *y);
extern void get_mb_block_pos_normal (sBlockPos *picPos, int mbAddr, short *x, short *y);
extern void get_mb_block_pos_mbaff  (sBlockPos *picPos, int mbAddr, short *x, short *y);
