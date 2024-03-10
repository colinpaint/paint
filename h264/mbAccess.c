//{{{  includes
#include "global.h"

#include "buffer.h"
#include "mbAccess.h"
//}}}

//{{{
Boolean mb_is_available (int mbIndex, sMacroblock* mb) {

  sSlice* slice = mb->slice;
  if ((mbIndex < 0) || (mbIndex > ((int)slice->picture->picSizeInMbs - 1)))
    return FALSE;

  // the following line checks both: slice number and if the mb has been decoded
  if (!mb->DeblockCall)
    if (slice->mbData[mbIndex].sliceNum != mb->sliceNum)
      return FALSE;

  return TRUE;
  }
//}}}
//{{{
void CheckAvailabilityOfNeighbors (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture; //decoder->picture;
  const int mb_nr = mb->mbIndexX;
  sBlockPos *picPos = mb->decoder->picPos;

  if (picture->mbAffFrameFlag) {
    int cur_mb_pair = mb_nr >> 1;
    mb->mbIndexA = 2 * (cur_mb_pair - 1);
    mb->mbIndexB = 2 * (cur_mb_pair - picture->picWidthMbs);
    mb->mbIndexC = 2 * (cur_mb_pair - picture->picWidthMbs + 1);
    mb->mbIndexD = 2 * (cur_mb_pair - picture->picWidthMbs - 1);

    mb->mbAvailA = (Boolean) (mb_is_available(mb->mbIndexA, mb) && ((picPos[cur_mb_pair    ].x)!=0));
    mb->mbAvailB = (Boolean) (mb_is_available(mb->mbIndexB, mb));
    mb->mbAvailC = (Boolean) (mb_is_available(mb->mbIndexC, mb) && ((picPos[cur_mb_pair + 1].x)!=0));
    mb->mbAvailD = (Boolean) (mb_is_available(mb->mbIndexD, mb) && ((picPos[cur_mb_pair    ].x)!=0));
    }
  else {
    sBlockPos* p_pic_pos = &picPos[mb_nr    ];
    mb->mbIndexA = mb_nr - 1;
    mb->mbIndexD = mb->mbIndexA - picture->picWidthMbs;
    mb->mbIndexB = mb->mbIndexD + 1;
    mb->mbIndexC = mb->mbIndexB + 1;


    mb->mbAvailA = (Boolean) (mb_is_available(mb->mbIndexA, mb) && ((p_pic_pos->x)!=0));
    mb->mbAvailD = (Boolean) (mb_is_available(mb->mbIndexD, mb) && ((p_pic_pos->x)!=0));
    mb->mbAvailC = (Boolean) (mb_is_available(mb->mbIndexC, mb) && (((p_pic_pos + 1)->x)!=0));
    mb->mbAvailB = (Boolean) (mb_is_available(mb->mbIndexB, mb));
    }

  mb->mbCabacLeft = (mb->mbAvailA) ? &(slice->mbData[mb->mbIndexA]) : NULL;
  mb->mbCabacUp   = (mb->mbAvailB) ? &(slice->mbData[mb->mbIndexB]) : NULL;
  }
//}}}

//{{{
void CheckAvailabilityOfNeighborsNormal (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture; //decoder->picture;
  const int mb_nr = mb->mbIndexX;
  sBlockPos* picPos = mb->decoder->picPos;

  sBlockPos* p_pic_pos = &picPos[mb_nr    ];
  mb->mbIndexA = mb_nr - 1;
  mb->mbIndexD = mb->mbIndexA - picture->picWidthMbs;
  mb->mbIndexB = mb->mbIndexD + 1;
  mb->mbIndexC = mb->mbIndexB + 1;


  mb->mbAvailA = (Boolean) (mb_is_available(mb->mbIndexA, mb) && ((p_pic_pos->x)!=0));
  mb->mbAvailD = (Boolean) (mb_is_available(mb->mbIndexD, mb) && ((p_pic_pos->x)!=0));
  mb->mbAvailC = (Boolean) (mb_is_available(mb->mbIndexC, mb) && (((p_pic_pos + 1)->x)!=0));
  mb->mbAvailB = (Boolean) (mb_is_available(mb->mbIndexB, mb));


  mb->mbCabacLeft = (mb->mbAvailA) ? &(slice->mbData[mb->mbIndexA]) : NULL;
  mb->mbCabacUp   = (mb->mbAvailB) ? &(slice->mbData[mb->mbIndexB]) : NULL;
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
void CheckAvailabilityOfNeighborsMBAFF (sMacroblock* mb) {

  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture; //decoder->picture;
  const int mb_nr = mb->mbIndexX;
  sBlockPos* picPos = mb->decoder->picPos;

  int cur_mb_pair = mb_nr >> 1;
  mb->mbIndexA = 2 * (cur_mb_pair - 1);
  mb->mbIndexB = 2 * (cur_mb_pair - picture->picWidthMbs);
  mb->mbIndexC = 2 * (cur_mb_pair - picture->picWidthMbs + 1);
  mb->mbIndexD = 2 * (cur_mb_pair - picture->picWidthMbs - 1);

  mb->mbAvailA = (Boolean) (mb_is_available(mb->mbIndexA, mb) && ((picPos[cur_mb_pair    ].x)!=0));
  mb->mbAvailB = (Boolean) (mb_is_available(mb->mbIndexB, mb));
  mb->mbAvailC = (Boolean) (mb_is_available(mb->mbIndexC, mb) && ((picPos[cur_mb_pair + 1].x)!=0));
  mb->mbAvailD = (Boolean) (mb_is_available(mb->mbIndexD, mb) && ((picPos[cur_mb_pair    ].x)!=0));

  mb->mbCabacLeft = (mb->mbAvailA) ? &(slice->mbData[mb->mbIndexA]) : NULL;
  mb->mbCabacUp   = (mb->mbAvailB) ? &(slice->mbData[mb->mbIndexB]) : NULL;
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
void getNonAffNeighbour (sMacroblock* mb, int xN, int yN, int mbSize[2], sPixelPos* pix) {

  int maxW = mbSize[0], maxH = mbSize[1];

  if (xN < 0) {
    if (yN < 0) {
      pix->mbIndex   = mb->mbIndexD;
      pix->available = mb->mbAvailD;
      }
    else if (yN < maxH) {
      pix->mbIndex   = mb->mbIndexA;
      pix->available = mb->mbAvailA;
      }
    else
      pix->available = FALSE;
    }
  else if (xN < maxW) {
    if (yN < 0) {
      pix->mbIndex   = mb->mbIndexB;
      pix->available = mb->mbAvailB;
      }
    else if (yN < maxH) {
      pix->mbIndex   = mb->mbIndexX;
      pix->available = TRUE;
      }
    else
      pix->available = FALSE;
    }
  else if ((xN >= maxW) && (yN < 0)) {
    pix->mbIndex   = mb->mbIndexC;
    pix->available = mb->mbAvailC;
    }
  else
    pix->available = FALSE;

  if (pix->available || mb->DeblockCall) {
    sBlockPos *CurPos = &(mb->decoder->picPos[ pix->mbIndex ]);
    pix->x     = (short) (xN & (maxW - 1));
    pix->y     = (short) (yN & (maxH - 1));
    pix->posX = (short) (pix->x + CurPos->x * maxW);
    pix->posY = (short) (pix->y + CurPos->y * maxH);
    }
  }
//}}}
//{{{
void getAffNeighbour (sMacroblock* mb, int xN, int yN, int mbSize[2], sPixelPos* pix) {

  sDecoder* decoder = mb->decoder;
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
      if(!mb->mbField) {
        //{{{  frame
        if ((mb->mbIndexX & 0x01) == 0) {
          //  top
          pix->mbIndex   = mb->mbIndexD  + 1;
          pix->available = mb->mbAvailD;
          yM = yN;
          }

        else {
          //  bottom
          pix->mbIndex   = mb->mbIndexA;
          pix->available = mb->mbAvailA;
          if (mb->mbAvailA) {
            if(!decoder->mbData[mb->mbIndexA].mbField)
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
        if ((mb->mbIndexX & 0x01) == 0) {
          //  top
          pix->mbIndex   = mb->mbIndexD;
          pix->available = mb->mbAvailD;
          if (mb->mbAvailD) {
            if(!decoder->mbData[mb->mbIndexD].mbField) {
              (pix->mbIndex)++;
               yM = 2 * yN;
              }
            else
               yM = yN;
            }
          }

        else {
          //  bottom
          pix->mbIndex   = mb->mbIndexD+1;
          pix->available = mb->mbAvailD;
          yM = yN;
          }
        }
        //}}}
      }
    else {
      // xN < 0 && yN >= 0
      if (yN >= 0 && yN <maxH) {
        if (!mb->mbField) {
          //{{{  frame
          if ((mb->mbIndexX & 0x01) == 0) {
            //{{{  top
            pix->mbIndex   = mb->mbIndexA;
            pix->available = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField)
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
            pix->mbIndex   = mb->mbIndexA;
            pix->available = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField) {
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
          if ((mb->mbIndexX & 0x01) == 0) {
            //{{{  top
            pix->mbIndex  = mb->mbIndexA;
            pix->available = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField) {
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
            pix->mbIndex  = mb->mbIndexA;
            pix->available = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField) {
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
        if (!mb->mbField) {
          //{{{  frame
          if ((mb->mbIndexX & 0x01) == 0) {
            //{{{  top
            pix->mbIndex  = mb->mbIndexB;
            // for the deblocker if the current MB is a frame and the one above is a field
            // then the neighbor is the top MB of the pair
            if (mb->mbAvailB) {
              if (!(mb->DeblockCall == 1 && (decoder->mbData[mb->mbIndexB]).mbField))
                pix->mbIndex  += 1;
              }

            pix->available = mb->mbAvailB;
            yM = yN;
            }
            //}}}
          else {
            //{{{  bottom
            pix->mbIndex   = mb->mbIndexX - 1;
            pix->available = TRUE;
            yM = yN;
            }
            //}}}
          }
          //}}}
        else {
          //{{{  field
          if ((mb->mbIndexX & 0x01) == 0) {
             //{{{  top
             pix->mbIndex   = mb->mbIndexB;
             pix->available = mb->mbAvailB;
             if (mb->mbAvailB) {
               if(!decoder->mbData[mb->mbIndexB].mbField) {
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
            pix->mbIndex   = mb->mbIndexB + 1;
            pix->available = mb->mbAvailB;
            yM = yN;
            }
            //}}}
          }
          //}}}
        }
      else {
        //{{{  yN >=0
        // for the deblocker if this is the extra edge then do this special stuff
        if (yN == 0 && mb->DeblockCall == 2) {
          pix->mbIndex  = mb->mbIndexB + 1;
          pix->available = TRUE;
          yM = yN - 1;
          }

        else if ((yN >= 0) && (yN <maxH)) {
          pix->mbIndex   = mb->mbIndexX;
          pix->available = TRUE;
          yM = yN;
          }
        }
        //}}}
      }
    else {
      //{{{  xN >= maxW
      if(yN < 0) {
        if (!mb->mbField) {
          // frame
          if ((mb->mbIndexX & 0x01) == 0) {
            // top
            pix->mbIndex  = mb->mbIndexC + 1;
            pix->available = mb->mbAvailC;
            yM = yN;
            }
          else
            // bottom
            pix->available = FALSE;
          }
        else {
          // field
          if ((mb->mbIndexX & 0x01) == 0) {
            // top
            pix->mbIndex   = mb->mbIndexC;
            pix->available = mb->mbAvailC;
            if (mb->mbAvailC) {
              if(!decoder->mbData[mb->mbIndexC].mbField) {
                (pix->mbIndex)++;
                 yM = 2* yN;
                }
              else
                yM = yN;
              }
            }
          else {
            // bottom
            pix->mbIndex   = mb->mbIndexC + 1;
            pix->available = mb->mbAvailC;
            yM = yN;
            }
          }
        }
      }
      //}}}
    }

  if (pix->available || mb->DeblockCall) {
    pix->x = (short) (xN & (maxW - 1));
    pix->y = (short) (yM & (maxH - 1));
    getMbPos (decoder, pix->mbIndex, mbSize, &(pix->posX), &(pix->posY));
    pix->posX = pix->posX + pix->x;
    pix->posY = pix->posY + pix->y;
    }
  }
//}}}
//{{{
void get4x4Neighbour (sMacroblock* mb, int blockX, int blockY, int mbSize[2], sPixelPos* pix) {

  mb->decoder->getNeighbour (mb, blockX, blockY, mbSize, pix);

  if (pix->available) {
    pix->x >>= 2;
    pix->y >>= 2;
    pix->posX >>= 2;
    pix->posY >>= 2;
    }
  }
//}}}
//{{{
void get4x4NeighbourBase (sMacroblock* mb, int blockX, int blockY, int mbSize[2], sPixelPos* pix) {

  mb->decoder->getNeighbour (mb, blockX, blockY, mbSize, pix);

  if (pix->available) {
    pix->x >>= 2;
    pix->y >>= 2;
    }
  }
//}}}
