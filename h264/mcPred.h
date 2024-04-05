#pragma once
#include "global.h"
#include "buffer.h"

void getBlockLuma (sPicture* curRef, int x_pos, int y_pos,
                   int blockSizeX, int blockSizeY, sPixel** block,
                   int shift_x, int maxold_x, int maxold_y,
                   int** tempRes, int max_imgpel_value,
                   sPixel no_ref_value, sMacroBlock* mb);

int getColocatedInfo8x8 (sMacroBlock* mb, sPicture* list1, int i, int j);
int getColocatedInfo4x4 (sMacroBlock* mb, sPicture* list1, int i, int j);

void intraChromaDecode (sMacroBlock* mb, int yuv);
void prepareDirectParam (sMacroBlock* mb, sPicture* picture,
                         sMotionVec* pmvl0, sMotionVec* pmvl1,
                          char* l0_rFrame, char* l1_rFrame);
void performMotionCompensation (sMacroBlock* mb, eColorPlane plane, sPicture* picture,
                                int predDir, int i, int j, int blockSizeX, int blockSizeY);
