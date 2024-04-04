//{{{  includes
#include "global.h"

#include "buffer.h"
#include "mbAccess.h"
//}}}

//{{{
bool isMbAvailable (int mbIndex, sMacroBlock* mb) {

  sSlice* slice = mb->slice;
  if ((mbIndex < 0) || (mbIndex > ((int)slice->picture->picSizeInMbs - 1)))
    return false;

  // the following line checks both: slice number and if the mb has been decoded
  if (!mb->DeblockCall)
    if (slice->mbData[mbIndex].sliceNum != mb->sliceNum)
      return false;

  return true;
  }
//}}}
//{{{
void checkNeighbours (sMacroBlock* mb) {

  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture; //decoder->picture;
  const int mb_nr = mb->mbIndexX;
  sBlockPos* picPos = mb->decoder->picPos;

  if (picture->mbAffFrame) {
    int cur_mb_pair = mb_nr >> 1;
    mb->mbIndexA = 2 * (cur_mb_pair - 1);
    mb->mbIndexB = 2 * (cur_mb_pair - picture->picWidthMbs);
    mb->mbIndexC = 2 * (cur_mb_pair - picture->picWidthMbs + 1);
    mb->mbIndexD = 2 * (cur_mb_pair - picture->picWidthMbs - 1);

    mb->mbAvailA = (bool) (isMbAvailable(mb->mbIndexA, mb) && ((picPos[cur_mb_pair    ].x)!=0));
    mb->mbAvailB = (bool) (isMbAvailable(mb->mbIndexB, mb));
    mb->mbAvailC = (bool) (isMbAvailable(mb->mbIndexC, mb) && ((picPos[cur_mb_pair + 1].x)!=0));
    mb->mbAvailD = (bool) (isMbAvailable(mb->mbIndexD, mb) && ((picPos[cur_mb_pair    ].x)!=0));
    }

  else {
    sBlockPos* p_pic_pos = &picPos[mb_nr    ];
    mb->mbIndexA = mb_nr - 1;
    mb->mbIndexD = mb->mbIndexA - picture->picWidthMbs;
    mb->mbIndexB = mb->mbIndexD + 1;
    mb->mbIndexC = mb->mbIndexB + 1;

    mb->mbAvailA = (bool) (isMbAvailable(mb->mbIndexA, mb) && ((p_pic_pos->x)!=0));
    mb->mbAvailD = (bool) (isMbAvailable(mb->mbIndexD, mb) && ((p_pic_pos->x)!=0));
    mb->mbAvailC = (bool) (isMbAvailable(mb->mbIndexC, mb) && (((p_pic_pos + 1)->x)!=0));
    mb->mbAvailB = (bool) (isMbAvailable(mb->mbIndexB, mb));
    }

  mb->mbCabacLeft = (mb->mbAvailA) ? &(slice->mbData[mb->mbIndexA]) : NULL;
  mb->mbCabacUp   = (mb->mbAvailB) ? &(slice->mbData[mb->mbIndexB]) : NULL;
  }
//}}}

//{{{
void checkNeighboursNormal (sMacroBlock* mb) {

  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture; //decoder->picture;
  const int mb_nr = mb->mbIndexX;
  sBlockPos* picPos = mb->decoder->picPos;

  sBlockPos* p_pic_pos = &picPos[mb_nr    ];
  mb->mbIndexA = mb_nr - 1;
  mb->mbIndexD = mb->mbIndexA - picture->picWidthMbs;
  mb->mbIndexB = mb->mbIndexD + 1;
  mb->mbIndexC = mb->mbIndexB + 1;

  mb->mbAvailA = (bool) (isMbAvailable(mb->mbIndexA, mb) && ((p_pic_pos->x)!=0));
  mb->mbAvailD = (bool) (isMbAvailable(mb->mbIndexD, mb) && ((p_pic_pos->x)!=0));
  mb->mbAvailC = (bool) (isMbAvailable(mb->mbIndexC, mb) && (((p_pic_pos + 1)->x)!=0));
  mb->mbAvailB = (bool) (isMbAvailable(mb->mbIndexB, mb));

  mb->mbCabacLeft = (mb->mbAvailA) ? &(slice->mbData[mb->mbIndexA]) : NULL;
  mb->mbCabacUp   = (mb->mbAvailB) ? &(slice->mbData[mb->mbIndexB]) : NULL;
  }
//}}}
//{{{
void getMbBlockPosNormal (sBlockPos* picPos, int mbIndex, int16_t* x, int16_t* y) {

  sBlockPos* pPos = &picPos[ mbIndex ];
  *x = (int16_t) pPos->x;
  *y = (int16_t) pPos->y;
  }
//}}}

//{{{
void checkNeighboursMbAff (sMacroBlock* mb) {

  sSlice* slice = mb->slice;
  sPicture* picture = slice->picture; 

  const int mb_nr = mb->mbIndexX;
  sBlockPos* picPos = mb->decoder->picPos;

  int cur_mb_pair = mb_nr >> 1;
  mb->mbIndexA = 2 * (cur_mb_pair - 1);
  mb->mbIndexB = 2 * (cur_mb_pair - picture->picWidthMbs);
  mb->mbIndexC = 2 * (cur_mb_pair - picture->picWidthMbs + 1);
  mb->mbIndexD = 2 * (cur_mb_pair - picture->picWidthMbs - 1);

  mb->mbAvailA = (bool) (isMbAvailable(mb->mbIndexA, mb) && ((picPos[cur_mb_pair    ].x)!=0));
  mb->mbAvailB = (bool) (isMbAvailable(mb->mbIndexB, mb));
  mb->mbAvailC = (bool) (isMbAvailable(mb->mbIndexC, mb) && ((picPos[cur_mb_pair + 1].x)!=0));
  mb->mbAvailD = (bool) (isMbAvailable(mb->mbIndexD, mb) && ((picPos[cur_mb_pair    ].x)!=0));

  mb->mbCabacLeft = (mb->mbAvailA) ? &(slice->mbData[mb->mbIndexA]) : NULL;
  mb->mbCabacUp   = (mb->mbAvailB) ? &(slice->mbData[mb->mbIndexB]) : NULL;
  }
//}}}
//{{{
void getMbBlockPosMbaff (sBlockPos* picPos, int mbIndex, int16_t* x, int16_t* y) {

  sBlockPos* pPos = &picPos[ mbIndex >> 1 ];
  *x = (int16_t)  pPos->x;
  *y = (int16_t) ((pPos->y << 1) + (mbIndex & 0x01));
  }
//}}}

//{{{
void getMbPos (cDecoder264* decoder, int mbIndex, int mbSize[2], int16_t* x, int16_t* y) {

  decoder->getMbBlockPos (decoder->picPos, mbIndex, x, y);
  (*x) = (int16_t) ((*x) * mbSize[0]);
  (*y) = (int16_t) ((*y) * mbSize[1]);
  }
//}}}

//{{{
void getNonAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos* pixelPos) {

  int maxW = mbSize[0];
  int maxH = mbSize[1];

  if (xN < 0) {
    if (yN < 0) {
      pixelPos->mbIndex = mb->mbIndexD;
      pixelPos->available = mb->mbAvailD;
      }
    else if (yN < maxH) {
      pixelPos->mbIndex = mb->mbIndexA;
      pixelPos->available = mb->mbAvailA;
      }
    else
      pixelPos->available = false;
    }
  else if (xN < maxW) {
    if (yN < 0) {
      pixelPos->mbIndex = mb->mbIndexB;
      pixelPos->available = mb->mbAvailB;
      }
    else if (yN < maxH) {
      pixelPos->mbIndex = mb->mbIndexX;
      pixelPos->available = true;
      }
    else
      pixelPos->available = false;
    }
  else if ((xN >= maxW) && (yN < 0)) {
    pixelPos->mbIndex = mb->mbIndexC;
    pixelPos->available = mb->mbAvailC;
    }
  else
    pixelPos->available = false;

  if (pixelPos->available || mb->DeblockCall) {
    sBlockPos* blockPos = &(mb->decoder->picPos[pixelPos->mbIndex]);
    pixelPos->x = (int16_t)(xN & (maxW - 1));
    pixelPos->y = (int16_t)(yN & (maxH - 1));
    pixelPos->posX = (int16_t)(pixelPos->x + blockPos->x * maxW);
    pixelPos->posY = (int16_t)(pixelPos->y + blockPos->y * maxH);
    }
  }
//}}}
//{{{
void getAffNeighbour (sMacroBlock* mb, int xN, int yN, int mbSize[2], sPixelPos* pixelPos) {

  cDecoder264* decoder = mb->decoder;
  int maxW, maxH;
  int yM = -1;

  maxW = mbSize[0];
  maxH = mbSize[1];

  // initialize to "not available"
  pixelPos->available = false;

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
          pixelPos->mbIndex   = mb->mbIndexD  + 1;
          pixelPos->available = mb->mbAvailD;
          yM = yN;
          }

        else {
          //  bottom
          pixelPos->mbIndex   = mb->mbIndexA;
          pixelPos->available = mb->mbAvailA;
          if (mb->mbAvailA) {
            if(!decoder->mbData[mb->mbIndexA].mbField)
               yM = yN;
            else {
              (pixelPos->mbIndex)++;
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
          pixelPos->mbIndex   = mb->mbIndexD;
          pixelPos->available = mb->mbAvailD;
          if (mb->mbAvailD) {
            if(!decoder->mbData[mb->mbIndexD].mbField) {
              (pixelPos->mbIndex)++;
               yM = 2 * yN;
              }
            else
               yM = yN;
            }
          }

        else {
          //  bottom
          pixelPos->mbIndex   = mb->mbIndexD+1;
          pixelPos->available = mb->mbAvailD;
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
            pixelPos->mbIndex   = mb->mbIndexA;
            pixelPos->available = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField)
                 yM = yN;
              else {
                (pixelPos->mbIndex)+= ((yN & 0x01) != 0);
                yM = yN >> 1;
                }
              }
            }
            //}}}
          else {
            //{{{  bottom
            pixelPos->mbIndex   = mb->mbIndexA;
            pixelPos->available = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField) {
                (pixelPos->mbIndex)++;
                 yM = yN;
                }
              else {
                (pixelPos->mbIndex)+= ((yN & 0x01) != 0);
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
            pixelPos->mbIndex  = mb->mbIndexA;
            pixelPos->available = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField) {
                if (yN < (maxH >> 1))
                   yM = yN << 1;
                else {
                  (pixelPos->mbIndex)++;
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
            pixelPos->mbIndex  = mb->mbIndexA;
            pixelPos->available = mb->mbAvailA;
            if (mb->mbAvailA) {
              if(!decoder->mbData[mb->mbIndexA].mbField) {
                if (yN < (maxH >> 1))
                  yM = (yN << 1) + 1;
                else {
                  (pixelPos->mbIndex)++;
                   yM = (yN << 1 ) + 1 - maxH;
                  }
                }
              else {
                (pixelPos->mbIndex)++;
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
            pixelPos->mbIndex  = mb->mbIndexB;
            // for the deblocker if the current MB is a frame and the one above is a field
            // then the neighbor is the top MB of the pair
            if (mb->mbAvailB) {
              if (!(mb->DeblockCall == 1 && (decoder->mbData[mb->mbIndexB]).mbField))
                pixelPos->mbIndex  += 1;
              }

            pixelPos->available = mb->mbAvailB;
            yM = yN;
            }
            //}}}
          else {
            //{{{  bottom
            pixelPos->mbIndex   = mb->mbIndexX - 1;
            pixelPos->available = true;
            yM = yN;
            }
            //}}}
          }
          //}}}
        else {
          //{{{  field
          if ((mb->mbIndexX & 0x01) == 0) {
             //{{{  top
             pixelPos->mbIndex   = mb->mbIndexB;
             pixelPos->available = mb->mbAvailB;
             if (mb->mbAvailB) {
               if(!decoder->mbData[mb->mbIndexB].mbField) {
                 (pixelPos->mbIndex)++;
                  yM = 2* yN;
                 }
               else
                  yM = yN;
               }
             }
             //}}}
           else {
            //{{{  bottom
            pixelPos->mbIndex   = mb->mbIndexB + 1;
            pixelPos->available = mb->mbAvailB;
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
          pixelPos->mbIndex  = mb->mbIndexB + 1;
          pixelPos->available = true;
          yM = yN - 1;
          }

        else if ((yN >= 0) && (yN <maxH)) {
          pixelPos->mbIndex   = mb->mbIndexX;
          pixelPos->available = true;
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
            pixelPos->mbIndex  = mb->mbIndexC + 1;
            pixelPos->available = mb->mbAvailC;
            yM = yN;
            }
          else
            // bottom
            pixelPos->available = false;
          }
        else {
          // field
          if ((mb->mbIndexX & 0x01) == 0) {
            // top
            pixelPos->mbIndex   = mb->mbIndexC;
            pixelPos->available = mb->mbAvailC;
            if (mb->mbAvailC) {
              if(!decoder->mbData[mb->mbIndexC].mbField) {
                (pixelPos->mbIndex)++;
                 yM = 2* yN;
                }
              else
                yM = yN;
              }
            }
          else {
            // bottom
            pixelPos->mbIndex   = mb->mbIndexC + 1;
            pixelPos->available = mb->mbAvailC;
            yM = yN;
            }
          }
        }
      }
      //}}}
    }

  if (pixelPos->available || mb->DeblockCall) {
    pixelPos->x = (int16_t) (xN & (maxW - 1));
    pixelPos->y = (int16_t) (yM & (maxH - 1));
    getMbPos (decoder, pixelPos->mbIndex, mbSize, &(pixelPos->posX), &(pixelPos->posY));
    pixelPos->posX = pixelPos->posX + pixelPos->x;
    pixelPos->posY = pixelPos->posY + pixelPos->y;
    }
  }
//}}}
//{{{
void get4x4Neighbour (sMacroBlock* mb, int blockX, int blockY, int mbSize[2], sPixelPos* pixelPos) {

  mb->decoder->getNeighbour (mb, blockX, blockY, mbSize, pixelPos);
  if (pixelPos->available) {
    pixelPos->x >>= 2;
    pixelPos->y >>= 2;
    pixelPos->posX >>= 2;
    pixelPos->posY >>= 2;
    }
  }
//}}}
//{{{
void get4x4NeighbourBase (sMacroBlock* mb, int blockX, int blockY, int mbSize[2], sPixelPos* pixelPos) {

  mb->decoder->getNeighbour (mb, blockX, blockY, mbSize, pixelPos);
  if (pixelPos->available) {
    pixelPos->x >>= 2;
    pixelPos->y >>= 2;
    }
  }
//}}}
