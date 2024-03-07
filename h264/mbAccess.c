//{{{  includes
#include "global.h"

#include "buffer.h"
#include "mbAccess.h"
//}}}

//{{{
Boolean mb_is_available (int mbAddr, sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  if ((mbAddr < 0) || (mbAddr > ((int)curSlice->picture->picSizeInMbs - 1)))
    return FALSE;

  // the following line checks both: slice number and if the mb has been decoded
  if (!curMb->DeblockCall)
    if (curSlice->mbData[mbAddr].slice_nr != curMb->slice_nr)
      return FALSE;

  return TRUE;
  }
//}}}
//{{{
void CheckAvailabilityOfNeighbors (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  sPicture* picture = curSlice->picture; //vidParam->picture;
  const int mb_nr = curMb->mbAddrX;
  sBlockPos *picPos = curMb->vidParam->picPos;

  if (picture->mb_aff_frame_flag) {
    int cur_mb_pair = mb_nr >> 1;
    curMb->mbAddrA = 2 * (cur_mb_pair - 1);
    curMb->mbAddrB = 2 * (cur_mb_pair - picture->PicWidthInMbs);
    curMb->mbAddrC = 2 * (cur_mb_pair - picture->PicWidthInMbs + 1);
    curMb->mbAddrD = 2 * (cur_mb_pair - picture->PicWidthInMbs - 1);

    curMb->mbAvailA = (Boolean) (mb_is_available(curMb->mbAddrA, curMb) && ((picPos[cur_mb_pair    ].x)!=0));
    curMb->mbAvailB = (Boolean) (mb_is_available(curMb->mbAddrB, curMb));
    curMb->mbAvailC = (Boolean) (mb_is_available(curMb->mbAddrC, curMb) && ((picPos[cur_mb_pair + 1].x)!=0));
    curMb->mbAvailD = (Boolean) (mb_is_available(curMb->mbAddrD, curMb) && ((picPos[cur_mb_pair    ].x)!=0));
    }
  else {
    sBlockPos* p_pic_pos = &picPos[mb_nr    ];
    curMb->mbAddrA = mb_nr - 1;
    curMb->mbAddrD = curMb->mbAddrA - picture->PicWidthInMbs;
    curMb->mbAddrB = curMb->mbAddrD + 1;
    curMb->mbAddrC = curMb->mbAddrB + 1;


    curMb->mbAvailA = (Boolean) (mb_is_available(curMb->mbAddrA, curMb) && ((p_pic_pos->x)!=0));
    curMb->mbAvailD = (Boolean) (mb_is_available(curMb->mbAddrD, curMb) && ((p_pic_pos->x)!=0));
    curMb->mbAvailC = (Boolean) (mb_is_available(curMb->mbAddrC, curMb) && (((p_pic_pos + 1)->x)!=0));
    curMb->mbAvailB = (Boolean) (mb_is_available(curMb->mbAddrB, curMb));
    }

  curMb->mb_left = (curMb->mbAvailA) ? &(curSlice->mbData[curMb->mbAddrA]) : NULL;
  curMb->mb_up   = (curMb->mbAvailB) ? &(curSlice->mbData[curMb->mbAddrB]) : NULL;
  }
//}}}

//{{{
void CheckAvailabilityOfNeighborsNormal (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  sPicture* picture = curSlice->picture; //vidParam->picture;
  const int mb_nr = curMb->mbAddrX;
  sBlockPos* picPos = curMb->vidParam->picPos;

  sBlockPos* p_pic_pos = &picPos[mb_nr    ];
  curMb->mbAddrA = mb_nr - 1;
  curMb->mbAddrD = curMb->mbAddrA - picture->PicWidthInMbs;
  curMb->mbAddrB = curMb->mbAddrD + 1;
  curMb->mbAddrC = curMb->mbAddrB + 1;


  curMb->mbAvailA = (Boolean) (mb_is_available(curMb->mbAddrA, curMb) && ((p_pic_pos->x)!=0));
  curMb->mbAvailD = (Boolean) (mb_is_available(curMb->mbAddrD, curMb) && ((p_pic_pos->x)!=0));
  curMb->mbAvailC = (Boolean) (mb_is_available(curMb->mbAddrC, curMb) && (((p_pic_pos + 1)->x)!=0));
  curMb->mbAvailB = (Boolean) (mb_is_available(curMb->mbAddrB, curMb));


  curMb->mb_left = (curMb->mbAvailA) ? &(curSlice->mbData[curMb->mbAddrA]) : NULL;
  curMb->mb_up   = (curMb->mbAvailB) ? &(curSlice->mbData[curMb->mbAddrB]) : NULL;
  }
//}}}
//{{{
void get_mb_block_pos_normal (sBlockPos* picPos, int mb_addr, short* x, short* y) {

  sBlockPos* pPos = &picPos[ mb_addr ];
  *x = (short) pPos->x;
  *y = (short) pPos->y;
  }
//}}}
//{{{
void CheckAvailabilityOfNeighborsMBAFF (sMacroblock* curMb) {

  sSlice* curSlice = curMb->slice;
  sPicture* picture = curSlice->picture; //vidParam->picture;
  const int mb_nr = curMb->mbAddrX;
  sBlockPos* picPos = curMb->vidParam->picPos;

  int cur_mb_pair = mb_nr >> 1;
  curMb->mbAddrA = 2 * (cur_mb_pair - 1);
  curMb->mbAddrB = 2 * (cur_mb_pair - picture->PicWidthInMbs);
  curMb->mbAddrC = 2 * (cur_mb_pair - picture->PicWidthInMbs + 1);
  curMb->mbAddrD = 2 * (cur_mb_pair - picture->PicWidthInMbs - 1);

  curMb->mbAvailA = (Boolean) (mb_is_available(curMb->mbAddrA, curMb) && ((picPos[cur_mb_pair    ].x)!=0));
  curMb->mbAvailB = (Boolean) (mb_is_available(curMb->mbAddrB, curMb));
  curMb->mbAvailC = (Boolean) (mb_is_available(curMb->mbAddrC, curMb) && ((picPos[cur_mb_pair + 1].x)!=0));
  curMb->mbAvailD = (Boolean) (mb_is_available(curMb->mbAddrD, curMb) && ((picPos[cur_mb_pair    ].x)!=0));

  curMb->mb_left = (curMb->mbAvailA) ? &(curSlice->mbData[curMb->mbAddrA]) : NULL;
  curMb->mb_up   = (curMb->mbAvailB) ? &(curSlice->mbData[curMb->mbAddrB]) : NULL;
  }
//}}}
//{{{
void get_mb_block_pos_mbaff (sBlockPos* picPos, int mb_addr, short* x, short* y) {

  sBlockPos* pPos = &picPos[ mb_addr >> 1 ];
  *x = (short)  pPos->x;
  *y = (short) ((pPos->y << 1) + (mb_addr & 0x01));
  }
//}}}
//{{{
void get_mb_pos (sVidParam* vidParam, int mb_addr, int mb_size[2], short* x, short* y) {

  vidParam->get_mb_block_pos (vidParam->picPos, mb_addr, x, y);
  (*x) = (short) ((*x) * mb_size[0]);
  (*y) = (short) ((*y) * mb_size[1]);
  }
//}}}

//{{{
void getNonAffNeighbour (sMacroblock* curMb, int xN, int yN, int mb_size[2], sPixelPos* pix) {

  int maxW = mb_size[0], maxH = mb_size[1];

  if (xN < 0) {
    if (yN < 0) {
      pix->mb_addr   = curMb->mbAddrD;
      pix->available = curMb->mbAvailD;
      }
    else if (yN < maxH) {
      pix->mb_addr   = curMb->mbAddrA;
      pix->available = curMb->mbAvailA;
      }
    else
      pix->available = FALSE;
    }
  else if (xN < maxW) {
    if (yN < 0) {
      pix->mb_addr   = curMb->mbAddrB;
      pix->available = curMb->mbAvailB;
      }
    else if (yN < maxH) {
      pix->mb_addr   = curMb->mbAddrX;
      pix->available = TRUE;
      }
    else
      pix->available = FALSE;
    }
  else if ((xN >= maxW) && (yN < 0)) {
    pix->mb_addr   = curMb->mbAddrC;
    pix->available = curMb->mbAvailC;
    }
  else
    pix->available = FALSE;

  if (pix->available || curMb->DeblockCall) {
    sBlockPos *CurPos = &(curMb->vidParam->picPos[ pix->mb_addr ]);
    pix->x     = (short) (xN & (maxW - 1));
    pix->y     = (short) (yN & (maxH - 1));
    pix->pos_x = (short) (pix->x + CurPos->x * maxW);
    pix->pos_y = (short) (pix->y + CurPos->y * maxH);
    }
  }
//}}}
//{{{
void getAffNeighbour (sMacroblock* curMb, int xN, int yN, int mb_size[2], sPixelPos* pix) {

  sVidParam* vidParam = curMb->vidParam;
  int maxW, maxH;
  int yM = -1;

  maxW = mb_size[0];
  maxH = mb_size[1];

  // initialize to "not available"
  pix->available = FALSE;

  if (yN > (maxH - 1))
    return;
  if (xN > (maxW - 1) && yN >= 0 && yN < maxH)
    return;

  if (xN < 0) {
    if (yN < 0) {
      if(!curMb->mb_field) {
        //{{{  frame
        if ((curMb->mbAddrX & 0x01) == 0) {
          //  top
          pix->mb_addr   = curMb->mbAddrD  + 1;
          pix->available = curMb->mbAvailD;
          yM = yN;
          }

        else {
          //  bottom
          pix->mb_addr   = curMb->mbAddrA;
          pix->available = curMb->mbAvailA;
          if (curMb->mbAvailA) {
            if(!vidParam->mbData[curMb->mbAddrA].mb_field)
               yM = yN;
            else {
              (pix->mb_addr)++;
               yM = (yN + maxH) >> 1;
              }
            }
          }
        }
        //}}}
      else {
        //{{{  field
        if ((curMb->mbAddrX & 0x01) == 0) {
          //  top
          pix->mb_addr   = curMb->mbAddrD;
          pix->available = curMb->mbAvailD;
          if (curMb->mbAvailD) {
            if(!vidParam->mbData[curMb->mbAddrD].mb_field) {
              (pix->mb_addr)++;
               yM = 2 * yN;
              }
            else
               yM = yN;
            }
          }

        else {
          //  bottom
          pix->mb_addr   = curMb->mbAddrD+1;
          pix->available = curMb->mbAvailD;
          yM = yN;
          }
        }
        //}}}
      }
    else {
      // xN < 0 && yN >= 0
      if (yN >= 0 && yN <maxH) {
        if (!curMb->mb_field) {
          //{{{  frame
          if ((curMb->mbAddrX & 0x01) == 0) {
            //{{{  top
            pix->mb_addr   = curMb->mbAddrA;
            pix->available = curMb->mbAvailA;
            if (curMb->mbAvailA) {
              if(!vidParam->mbData[curMb->mbAddrA].mb_field)
                 yM = yN;
              else {
                (pix->mb_addr)+= ((yN & 0x01) != 0);
                yM = yN >> 1;
                }
              }
            }
            //}}}
          else {
            //{{{  bottom
            pix->mb_addr   = curMb->mbAddrA;
            pix->available = curMb->mbAvailA;
            if (curMb->mbAvailA) {
              if(!vidParam->mbData[curMb->mbAddrA].mb_field) {
                (pix->mb_addr)++;
                 yM = yN;
                }
              else {
                (pix->mb_addr)+= ((yN & 0x01) != 0);
                yM = (yN + maxH) >> 1;
                }
              }
            }
            //}}}
          }
          //}}}
        else {
          //{{{  field
          if ((curMb->mbAddrX & 0x01) == 0) {
            //{{{  top
            pix->mb_addr  = curMb->mbAddrA;
            pix->available = curMb->mbAvailA;
            if (curMb->mbAvailA) {
              if(!vidParam->mbData[curMb->mbAddrA].mb_field) {
                if (yN < (maxH >> 1))
                   yM = yN << 1;
                else {
                  (pix->mb_addr)++;
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
            pix->mb_addr  = curMb->mbAddrA;
            pix->available = curMb->mbAvailA;
            if (curMb->mbAvailA) {
              if(!vidParam->mbData[curMb->mbAddrA].mb_field) {
                if (yN < (maxH >> 1))
                  yM = (yN << 1) + 1;
                else {
                  (pix->mb_addr)++;
                   yM = (yN << 1 ) + 1 - maxH;
                  }
                }
              else {
                (pix->mb_addr)++;
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
        if (!curMb->mb_field) {
          //{{{  frame
          if ((curMb->mbAddrX & 0x01) == 0) {
            //{{{  top
            pix->mb_addr  = curMb->mbAddrB;
            // for the deblocker if the current MB is a frame and the one above is a field
            // then the neighbor is the top MB of the pair
            if (curMb->mbAvailB) {
              if (!(curMb->DeblockCall == 1 && (vidParam->mbData[curMb->mbAddrB]).mb_field))
                pix->mb_addr  += 1;
              }

            pix->available = curMb->mbAvailB;
            yM = yN;
            }
            //}}}
          else {
            //{{{  bottom
            pix->mb_addr   = curMb->mbAddrX - 1;
            pix->available = TRUE;
            yM = yN;
            }
            //}}}
          }
          //}}}
        else {
          //{{{  field
          if ((curMb->mbAddrX & 0x01) == 0) {
             //{{{  top
             pix->mb_addr   = curMb->mbAddrB;
             pix->available = curMb->mbAvailB;
             if (curMb->mbAvailB) {
               if(!vidParam->mbData[curMb->mbAddrB].mb_field) {
                 (pix->mb_addr)++;
                  yM = 2* yN;
                 }
               else
                  yM = yN;
               }
             }
             //}}}
           else {
            //{{{  bottom
            pix->mb_addr   = curMb->mbAddrB + 1;
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
          pix->mb_addr  = curMb->mbAddrB + 1;
          pix->available = TRUE;
          yM = yN - 1;
          }

        else if ((yN >= 0) && (yN <maxH)) {
          pix->mb_addr   = curMb->mbAddrX;
          pix->available = TRUE;
          yM = yN;
          }
        }
        //}}}
      }
    else {
      //{{{  xN >= maxW
      if(yN < 0) {
        if (!curMb->mb_field) {
          // frame
          if ((curMb->mbAddrX & 0x01) == 0) {
            // top
            pix->mb_addr  = curMb->mbAddrC + 1;
            pix->available = curMb->mbAvailC;
            yM = yN;
            }
          else
            // bottom
            pix->available = FALSE;
          }
        else {
          // field
          if ((curMb->mbAddrX & 0x01) == 0) {
            // top
            pix->mb_addr   = curMb->mbAddrC;
            pix->available = curMb->mbAvailC;
            if (curMb->mbAvailC) {
              if(!vidParam->mbData[curMb->mbAddrC].mb_field) {
                (pix->mb_addr)++;
                 yM = 2* yN;
                }
              else
                yM = yN;
              }
            }
          else {
            // bottom
            pix->mb_addr   = curMb->mbAddrC + 1;
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
    get_mb_pos (vidParam, pix->mb_addr, mb_size, &(pix->pos_x), &(pix->pos_y));
    pix->pos_x = pix->pos_x + pix->x;
    pix->pos_y = pix->pos_y + pix->y;
    }
  }
//}}}
//{{{
void get4x4Neighbour (sMacroblock* curMb, int block_x, int block_y, int mb_size[2], sPixelPos* pix) {

  curMb->vidParam->getNeighbour (curMb, block_x, block_y, mb_size, pix);

  if (pix->available) {
    pix->x >>= 2;
    pix->y >>= 2;
    pix->pos_x >>= 2;
    pix->pos_y >>= 2;
    }
  }
//}}}
//{{{
void get4x4NeighbourBase (sMacroblock* curMb, int block_x, int block_y, int mb_size[2], sPixelPos* pix) {

  curMb->vidParam->getNeighbour (curMb, block_x, block_y, mb_size, pix);

  if (pix->available) {
    pix->x >>= 2;
    pix->y >>= 2;
    }
  }
//}}}
