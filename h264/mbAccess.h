#pragma once

extern void CheckAvailabilityOfNeighbors (sMacroblock* curMb);
extern void CheckAvailabilityOfNeighborsMBAFF (sMacroblock* curMb);
extern void CheckAvailabilityOfNeighborsNormal (sMacroblock* curMb);

extern void getAffNeighbour (sMacroblock* curMb, int xN, int yN, int mb_size[2], sPixelPos *pix);
extern void getNonAffNeighbour (sMacroblock* curMb, int xN, int yN, int mb_size[2], sPixelPos *pix);
extern void get4x4Neighbour (sMacroblock* curMb, int xN, int yN, int mb_size[2], sPixelPos *pix);
extern void get4x4NeighbourBase (sMacroblock* curMb, int block_x, int block_y, int mb_size[2], sPixelPos *pix);

extern Boolean mb_is_available (int mbAddr, sMacroblock* curMb);
extern void get_mb_pos (sVidParam* vidParam, int mb_addr, int mb_size[2], short *x, short *y);
extern void get_mb_block_pos_normal (sBlockPos *picPos, int mb_addr, short *x, short *y);
extern void get_mb_block_pos_mbaff  (sBlockPos *picPos, int mb_addr, short *x, short *y);
