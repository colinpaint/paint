#pragma once

extern void CheckAvailabilityOfNeighbors (sMacroblock* currMB);
extern void CheckAvailabilityOfNeighborsMBAFF (sMacroblock* currMB);
extern void CheckAvailabilityOfNeighborsNormal (sMacroblock* currMB);

extern void getAffNeighbour (sMacroblock* currMB, int xN, int yN, int mb_size[2], PixelPos *pix);
extern void getNonAffNeighbour (sMacroblock* currMB, int xN, int yN, int mb_size[2], PixelPos *pix);
extern void get4x4Neighbour (sMacroblock* currMB, int xN, int yN, int mb_size[2], PixelPos *pix);
extern void get4x4NeighbourBase (sMacroblock* currMB, int block_x, int block_y, int mb_size[2], PixelPos *pix);

extern Boolean mb_is_available (int mbAddr, sMacroblock* currMB);
extern void get_mb_pos (sVidParam* vidParam, int mb_addr, int mb_size[2], short *x, short *y);
extern void get_mb_block_pos_normal (BlockPos *PicPos, int mb_addr, short *x, short *y);
extern void get_mb_block_pos_mbaff  (BlockPos *PicPos, int mb_addr, short *x, short *y);
