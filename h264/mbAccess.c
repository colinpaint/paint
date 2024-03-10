//{{{  includes
#include "global.h"

#include "buffer.h"
#include "mbAccess.h"
//}}}

//{{{
Boolean mb_is_available (int mbIndex, sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  if ((mbIndex < 0) || (mbIndex > ((int)curSlice->picture->picSizeInMbs - 1)))
    return FALSE;

  // the following line checks both: slice number and if the mb has been decoded
  if (!curMb->DeblockCall)
    if (curSlice->mbData[mbIndex].sliceNum != curMb->sliceNum)
      return FALSE;

  return TRUE;
  }
//}}}
//{{{
void CheckAvailabilityOfNeighbors (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  sPicture* picture = curSlice->picture; //decoder->picture;
  const int mb_nr = curMb->mbIndexX;
  sBlockPos *picPos = curMb->decoder->picPos;

  if (picture->mbAffFrameFlag) {
    int cur_mb_pair = mb_nr >> 1;
    curMb->mbIndexA = 2 * (cur_mb_pair - 1);
    curMb->mbIndexB = 2 * (cur_mb_pair - picture->picWidthMbs);
    curMb->mbIndexC = 2 * (cur_mb_pair - picture->picWidthMbs + 1);
    curMb->mbIndexD = 2 * (cur_mb_pair - picture->picWidthMbs - 1);

    curMb->mbAvailA = (Boolean) (mb_is_available(curMb->mbIndexA, curMb) && ((picPos[cur_mb_pair    ].x)!=0));
    curMb->mbAvailB = (Boolean) (mb_is_available(curMb->mbIndexB, curMb));
    curMb->mbAvailC = (Boolean) (mb_is_available(curMb->mbIndexC, curMb) && ((picPos[cur_mb_pair + 1].x)!=0));
    curMb->mbAvailD = (Boolean) (mb_is_available(curMb->mbIndexD, curMb) && ((picPos[cur_mb_pair    ].x)!=0));
    }
  else {
    sBlockPos* p_pic_pos = &picPos[mb_nr    ];
    curMb->mbIndexA = mb_nr - 1;
    curMb->mbIndexD = curMb->mbIndexA - picture->picWidthMbs;
    curMb->mbIndexB = curMb->mbIndexD + 1;
    curMb->mbIndexC = curMb->mbIndexB + 1;


    curMb->mbAvailA = (Boolean) (mb_is_available(curMb->mbIndexA, curMb) && ((p_pic_pos->x)!=0));
    curMb->mbAvailD = (Boolean) (mb_is_available(curMb->mbIndexD, curMb) && ((p_pic_pos->x)!=0));
    curMb->mbAvailC = (Boolean) (mb_is_available(curMb->mbIndexC, curMb) && (((p_pic_pos + 1)->x)!=0));
    curMb->mbAvailB = (Boolean) (mb_is_available(curMb->mbIndexB, curMb));
    }

  curMb->mbCabacLeft = (curMb->mbAvailA) ? &(curSlice->mbData[curMb->mbIndexA]) : NULL;
  curMb->mbCabacUp   = (curMb->mbAvailB) ? &(curSlice->mbData[curMb->mbIndexB]) : NULL;
  }
//}}}

//{{{
void CheckAvailabilityOfNeighborsNormal (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  sPicture* picture = curSlice->picture; //decoder->picture;
  const int mb_nr = curMb->mbIndexX;
  sBlockPos* picPos = curMb->decoder->picPos;

  sBlockPos* p_pic_pos = &picPos[mb_nr    ];
  curMb->mbIndexA = mb_nr - 1;
  curMb->mbIndexD = curMb->mbIndexA - picture->picWidthMbs;
  curMb->mbIndexB = curMb->mbIndexD + 1;
  curMb->mbIndexC = curMb->mbIndexB + 1;


  curMb->mbAvailA = (Boolean) (mb_is_available(curMb->mbIndexA, curMb) && ((p_pic_pos->x)!=0));
  curMb->mbAvailD = (Boolean) (mb_is_available(curMb->mbIndexD, curMb) && ((p_pic_pos->x)!=0));
  curMb->mbAvailC = (Boolean) (mb_is_available(curMb->mbIndexC, curMb) && (((p_pic_pos + 1)->x)!=0));
  curMb->mbAvailB = (Boolean) (mb_is_available(curMb->mbIndexB, curMb));


  curMb->mbCabacLeft = (curMb->mbAvailA) ? &(curSlice->mbData[curMb->mbIndexA]) : NULL;
  curMb->mbCabacUp   = (curMb->mbAvailB) ? &(curSlice->mbData[curMb->mbIndexB]) : NULL;
  }
//}}}
//{{{
void get_mb_block_pos_normal (sBlockPos* picPos, int mbIndex, short* x, short* y) {

  sBlockPos* pPos = &picPos[ mbIndex ];
  *x = (short) pPos->x;
  *y = (short) pPos->y;
  }
//}}}
//{{{
void CheckAvailabilityOfNeighborsMBAFF (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  sPicture* picture = curSlice->picture; //decoder->picture;
  const int mb_nr = curMb->mbIndexX;
  sBlockPos* picPos = curMb->decoder->picPos;

  int cur_mb_pair = mb_nr >> 1;
  curMb->mbIndexA = 2 * (cur_mb_pair - 1);
  curMb->mbIndexB = 2 * (cur_mb_pair - picture->picWidthMbs);
  curMb->mbIndexC = 2 * (cur_mb_pair - picture->picWidthMbs + 1);
  curMb->mbIndexD = 2 * (cur_mb_pair - picture->picWidthMbs - 1);

  curMb->mbAvailA = (Boolean) (mb_is_available(curMb->mbIndexA, curMb) && ((picPos[cur_mb_pair    ].x)!=0));
  curMb->mbAvailB = (Boolean) (mb_is_available(curMb->mbIndexB, curMb));
  curMb->mbAvailC = (Boolean) (mb_is_available(curMb->mbIndexC, curMb) && ((picPos[cur_mb_pair + 1].x)!=0));
  curMb->mbAvailD = (Boolean) (mb_is_available(curMb->mbIndexD, curMb) && ((picPos[cur_mb_pair    ].x)!=0));

  curMb->mbCabacLeft = (curMb->mbAvailA) ? &(curSlice->mbData[curMb->mbIndexA]) : NULL;
  curMb->mbCabacUp   = (curMb->mbAvailB) ? &(curSlice->mbData[curMb->mbIndexB]) : NULL;
  }
//}}}
//{{{
void get_mb_block_pos_mbaff (sBlockPos* picPos, int mbIndex, short* x, short* y) {

  sBlockPos* pPos = &picPos[ mbIndex >> 1 ];
  *x = (short)  pPos->x;
  *y = (short) ((pPos->y << 1) + (mbIndex & 0x01));
  }
//}}}
//{{{
void getMbPos (sDecoder* decoder, int mbIndex, int mbSize[2], short* x, short* y) {

  decoder->getMbBlockPos (decoder->picPos, mbIndex, x, y);
  (*x) = (short) ((*x) * mbSize[0]);
  (*y) = (short) ((*y) * mbSize[1]);
  }
//}}}

//{{{
void getNonAffNeighbour (sMacroblock* curMb, int xN, int yN, int mbSize[2], sPixelPos* pix) {

  int maxW = mbSize[0], maxH = mbSize[1];

  if (xN < 0) {
    if (yN < 0) {
      pix->mbIndex   = curMb->mbIndexD;
      pix->available = curMb->mbAvailD;
      }
    else if (yN < maxH) {
      pix->mbIndex   = curMb->mbIndexA;
      pix->available = curMb->mbAvailA;
      }
    else
      pix->available = FALSE;
    }
  else if (xN < maxW) {
    if (yN < 0) {
      pix->mbIndex   = curMb->mbIndexB;
      pix->available = curMb->mbAvailB;
      }
    else if (yN < maxH) {
      pix->mbIndex   = curMb->mbIndexX;
      pix->available = TRUE;
      }
    else
      pix->available = FALSE;
    }
  else if ((xN >= maxW) && (yN < 0)) {
    pix->mbIndex   = curMb->mbIndexC;
    pix->available = curMb->mbAvailC;
    }
  else
    pix->available = FALSE;

  if (pix->available || curMb->DeblockCall) {
    sBlockPos *CurPos = &(curMb->decoder->picPos[ pix->mbIndex ]);
    pix->x     = (short) (xN & (maxW - 1));
    pix->y     = (short) (yN & (maxH - 1));
    pix->posX = (short) (pix->x + CurPos->x * maxW);
    pix->posY = (short) (pix->y + CurPos->y * maxH);
    }
  }
//}}}
//{{{
void getAffNeighbour (sMacroblock* curMb, int xN, int yN, int mbSize[2], sPixelPos* pix) {

  sDecoder* decoder = curMb->decoder;
  int maxW, maxH;
  int yM = -1;

  maxW = mbSize[0];
  maxH = mbSize[1];

  // initialize to "not available"
  pix->available = FALSE;

  if (yN > (maxH - 1))
    return;
  if (xN > (maxW - 1) && yN >= 0 && yN < maxH)
    return;

  if (xN < 0) {
    if (yN < 0) {
      if(!curMb->mbField) {
        //{{{  frame
        if ((curMb->mbIndexX & 0x01) == 0) {
          //  top
          pix->mbIndex   = curMb->mbIndexD  + 1;
          pix->available = curMb->mbAvailD;
          yM = yN;
          }

        else {
          //  bottom
          pix->mbIndex   = curMb->mbIndexA;
          pix->available = curMb->mbAvailA;
          if (curMb->mbAvailA) {
            if(!decoder->mbData[curMb->mbIndexA].mbField)
               yM = yN;
            else {
              (pix->mbIndex)++;
               yM = (yN + maxH) >> 1;
              }
            }
          }
        }
        //}}}
      else {
        //{{{  field
        if ((curMb->mbIndexX & 0x01) == 0) {
          //  top
          pix->mbIndex   = curMb->mbIndexD;
          pix->available = curMb->mbAvailD;
          if (curMb->mbAvailD) {
            if(!decoder->mbData[curMb->mbIndexD].mbField) {
              (pix->mbIndex)++;
               yM = 2 * yN;
              }
            else
               yM = yN;
            }
          }

        else {
          //  bottom
          pix->mbIndex   = curMb->mbIndexD+1;
          pix->available = curMb->mbAvailD;
          yM = yN;
          }
        }
        //}}}
      }
    else {
      // xN < 0 && yN >= 0
      if (yN >= 0 && yN <maxH) {
        if (!curMb->mbField) {
          //{{{  frame
          if ((curMb->mbIndexX & 0x01) == 0) {
            //{{{  top
            pix->mbIndex   = curMb->mbIndexA;
            pix->available = curMb->mbAvailA;
            if (curMb->mbAvailA) {
              if(!decoder->mbData[curMb->mbIndexA].mbField)
                 yM = yN;
              else {
                (pix->mbIndex)+= ((yN & 0x01) != 0);
                yM = yN >> 1;
                }
              }
            }
            //}}}
          else {
            //{{{  bottom
            pix->mbIndex   = curMb->mbIndexA;
            pix->available = curMb->mbAvailA;
            if (curMb->mbAvailA) {
              if(!decoder->mbData[curMb->mbIndexA].mbField) {
                (pix->mbIndex)++;
                 yM = yN;
                }
              else {
                (pix->mbIndex)+= ((yN & 0x01) != 0);
                yM = (yN + maxH) >> 1;
                }
              }
            }
            //}}}
          }
          //}}}
        else {
          //{{{  field
          if ((curMb->mbIndexX & 0x01) == 0) {
            //{{{  top
            pix->mbIndex  = curMb->mbIndexA;
            pix->available = curMb->mbAvailA;
            if (curMb->mbAvailA) {
              if(!decoder->mbData[curMb->mbIndexA].mbField) {
                if (yN < (maxH >> 1))
                   yM = yN << 1;
                else {
                  (pix->mbIndex)++;
                   yM = (yN << 1 ) - maxH;
                  }
                }
              else
                 yM = yN;
              }
            }
            //}}}
          else {
            //{{{  bottom
            pix->mbIndex  = curMb->mbIndexA;
            pix->available = curMb->mbAvailA;
            if (curMb->mbAvailA) {
              if(!decoder->mbData[curMb->mbIndexA].mbField) {
                if (yN < (maxH >> 1))
                  yM = (yN << 1) + 1;
                else {
                  (pix->mbIndex)++;
                   yM = (yN << 1 ) + 1 - maxH;
                  }
                }
              else {
                (pix->mbIndex)++;
                 yM = yN;
                }
              }
            }
            //}}}
          }
          //}}}
        }
      }
    }
  else {
     // xN >= 0
    if (xN >= 0 && xN < maxW) {
      if (yN<0) {
        if (!curMb->mbField) {
          //{{{  frame
          if ((curMb->mbIndexX & 0x01) == 0) {
            //{{{  top
            pix->mbIndex  = curMb->mbIndexB;
            // for the deblocker if the current MB is a frame and the one above is a field
            // then the neighbor is the top MB of the pair
            if (curMb->mbAvailB) {
              if (!(curMb->DeblockCall == 1 && (decoder->mbData[curMb->mbIndexB]).mbField))
                pix->mbIndex  += 1;
              }

            pix->available = curMb->mbAvailB;
            yM = yN;
            }
            //}}}
          else {
            //{{{  bottom
            pix->mbIndex   = curMb->mbIndexX - 1;
            pix->available = TRUE;
            yM = yN;
            }
            //}}}
          }
          //}}}
        else {
          //{{{  field
          if ((curMb->mbIndexX & 0x01) == 0) {
             //{{{  top
             pix->mbIndex   = curMb->mbIndexB;
             pix->available = curMb->mbAvailB;
             if (curMb->mbAvailB) {
               if(!decoder->mbData[curMb->mbIndexB].mbField) {
                 (pix->mbIndex)++;
                  yM = 2* yN;
                 }
               else
                  yM = yN;
               }
             }
             //}}}
           else {
            //{{{  bottom
            pix->mbIndex   = curMb->mbIndexB + 1;
            pix->available = curMb->mbAvailB;
            yM = yN;
            }
            //}}}
          }
          //}}}
        }
      else {
        //{{{  yN >=0
        // for the deblocker if this is the extra edge then do this special stuff
        if (yN == 0 && curMb->DeblockCall == 2) {
          pix->mbIndex  = curMb->mbIndexB + 1;
          pix->available = TRUE;
          yM = yN - 1;
          }

        else if ((yN >= 0) && (yN <maxH)) {
          pix->mbIndex   = curMb->mbIndexX;
          pix->available = TRUE;
          yM = yN;
          }
        }
        //}}}
      }
    else {
      //{{{  xN >= maxW
      if(yN < 0) {
        if (!curMb->mbField) {
          // frame
          if ((curMb->mbIndexX & 0x01) == 0) {
            // top
            pix->mbIndex  = curMb->mbIndexC + 1;
            pix->available = curMb->mbAvailC;
            yM = yN;
            }
          else
            // bottom
            pix->available = FALSE;
          }
        else {
          // field
          if ((curMb->mbIndexX & 0x01) == 0) {
            // top
            pix->mbIndex   = curMb->mbIndexC;
            pix->available = curMb->mbAvailC;
            if (curMb->mbAvailC) {
              if(!decoder->mbData[curMb->mbIndexC].mbField) {
                (pix->mbIndex)++;
                 yM = 2* yN;
                }
              else
                yM = yN;
              }
            }
          else {
            // bottom
            pix->mbIndex   = curMb->mbIndexC + 1;
            pix->available = curMb->mbAvailC;
            yM = yN;
            }
          }
        }
      }
      //}}}
    }

  if (pix->available || curMb->DeblockCall) {
    pix->x = (short) (xN & (maxW - 1));
    pix->y = (short) (yM & (maxH - 1));
    getMbPos (decoder, pix->mbIndex, mbSize, &(pix->posX), &(pix->posY));
    pix->posX = pix->posX + pix->x;
    pix->posY = pix->posY + pix->y;
    }
  }
//}}}
//{{{
void get4x4Neighbour (sMacroblock* curMb, int blockX, int blockY, int mbSize[2], sPixelPos* pix) {

  curMb->decoder->getNeighbour (curMb, blockX, blockY, mbSize, pix);

  if (pix->available) {
    pix->x >>= 2;
    pix->y >>= 2;
    pix->posX >>= 2;
    pix->posY >>= 2;
    }
  }
//}}}
//{{{
void get4x4NeighbourBase (sMacroblock* curMb, int blockX, int blockY, int mbSize[2], sPixelPos* pix) {

  curMb->decoder->getNeighbour (curMb, blockX, blockY, mbSize, pix);

  if (pix->available) {
    pix->x >>= 2;
    pix->y >>= 2;
    }
  }
//}}}
