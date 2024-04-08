//{{{  includes
#include "global.h"
#include "memory.h"

#include "mcPred.h"
#include "macroblock.h"
#include "erc.h"
//}}}
//{{{  defines
// If the average motion vector of the correctly received macroBlocks is less than the
// threshold, concealByCopy is used, otherwise concealByTrial is used. */
#define MVPERMB_THR 8

// used to determine the size of the allocated memory for a temporal Region (MB) */
#define DEF_REGION_SIZE 384  /* 8*8*6 */
#define ERC_BLOCK_OK                3
#define ERC_BLOCK_CONCEALED         2
#define ERC_BLOCK_CORRUPTED         1
#define ERC_BLOCK_EMPTY             0

//{{{
#define isSplit(object_list,currMBNum) \
    ((object_list+((currMBNum)<<2))->regionMode >= REGMODE_SPLITTED)
//}}}
//{{{
/* this can be used as isBlock(...,INTRA) or isBlock(...,INTER_COPY) */
#define isBlock(object_list,currMBNum,comp,regMode) \
    (isSplit(object_list,currMBNum) ? \
     ((object_list+((currMBNum)<<2)+(comp))->regionMode == REGMODE_##regMode##_8x8) : \
     ((object_list+((currMBNum)<<2))->regionMode == REGMODE_##regMode))
//}}}
//{{{
/* this can be used as getParam(...,mv) or getParam(...,xMin) or getParam(...,yMin) */
#define getParam(object_list,currMBNum,comp,param) \
    (isSplit(object_list,currMBNum) ? \
     ((object_list+((currMBNum)<<2)+(comp))->param) : \
     ((object_list+((currMBNum)<<2))->param))
//}}}

// Functions to convert MBNum representation to blockNum
#define xPosYBlock(currYBlockNum,picSizeX) ((currYBlockNum)%((picSizeX)>>3))
#define yPosYBlock(currYBlockNum,picSizeX) ((currYBlockNum)/((picSizeX)>>3))

//{{{
#define MBxy2YBlock(currXPos,currYPos,comp,picSizeX) \
  ((((currYPos)<<1)+((comp)>>1))*((picSizeX)>>3)+((currXPos)<<1)+((comp)&1))
//}}}
//{{{
#define MBNum2YBlock(currMBNum,comp,picSizeX) \
  MBxy2YBlock(xPosMB((currMBNum),(picSizeX)),yPosMB((currMBNum),(picSizeX)),(comp),(picSizeX))
//}}}
//}}}

namespace {
  const int uv_div[2][4] = {{0, 1, 1, 0}, {0, 1, 0, 0}}; //[x/y][yuvFormat]

  // intraFrame
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      collects prediction blocks only from the current column
   * \return
   *      Number of usable neighbour MacroBlocks for conceal.
   * \param predBlocks[]
   *      Array for indicating the valid neighbor blocks
   * \param currRow
   *      Current block row in the frame
   * \param currColumn
   *      Current block column in the frame
   * \param condition
   *      The block condition (ok, lost) table
   * \param maxRow
   *      Number of block rows in the frame
   * \param maxColumn
   *      Number of block columns in the frame
   * \param step
   *      Number of blocks belonging to a MB, when counting
   *      in vertical/horizontal direction. (Y:2 U,V:1)
  ** **********************************************************************
   */
  int ercCollectColumnBlocks (int predBlocks[], int currRow, int currColumn, char *condition, int maxRow, int maxColumn, int step )
  {
    int srcCounter = 0, threshold = ERC_BLOCK_CORRUPTED;

    memset( predBlocks, 0, 8*sizeof(int) );

    // in this case, row > 0 and row < 17
    if ( condition[ (currRow-1)*maxColumn + currColumn ] > threshold )
    {
      predBlocks[4] = 1;
      srcCounter++;
    }
    if ( condition[ (currRow+step)*maxColumn + currColumn ] > threshold )
    {
      predBlocks[6] = 1;
      srcCounter++;
    }

    return srcCounter;
  }
  //}}}
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      This function checks the neighbors of a sMacroBlock for usability in
   *      conceal. First the OK macroBlocks are marked, and if there is not
   *      enough of them, then the CONCEALED ones as well.
   *      A "1" in the the output array means reliable, a "0" non reliable MB.
   *      The block order in "predBlocks":
   *              1 4 0
   *              5 x 7
   *              2 6 3
   *      i.e., corners first.
   * \return
   *      Number of useable neighbor macroBlocks for conceal.
   * \param predBlocks[]
   *      Array for indicating the valid neighbor blocks
   * \param currRow
   *      Current block row in the frame
   * \param currColumn
   *      Current block column in the frame
   * \param condition
   *      The block condition (ok, lost) table
   * \param maxRow
   *      Number of block rows in the frame
   * \param maxColumn
   *      Number of block columns in the frame
   * \param step
   *      Number of blocks belonging to a MB, when counting
   *      in vertical/horizontal direction. (Y:2 U,V:1)
   * \param fNoCornerNeigh
   *      No corner neighbors are considered
  ** **********************************************************************
   */
  int ercCollect8PredBlocks (int predBlocks[], int currRow, int currColumn, char *condition,
                             int maxRow, int maxColumn, int step, uint8_t fNoCornerNeigh )
  {
    int srcCounter  = 0;
    int srcCountMin = (fNoCornerNeigh ? 2 : 4);
    int threshold   = ERC_BLOCK_OK;

    memset( predBlocks, 0, 8*sizeof(int) );

    // collect the reliable neighboring blocks
    do
    {
      srcCounter = 0;
      // top
      if (currRow > 0 && condition[ (currRow-1)*maxColumn + currColumn ] >= threshold )
      {                           //ERC_BLOCK_OK (3) or ERC_BLOCK_CONCEALED (2)
        predBlocks[4] = condition[ (currRow-1)*maxColumn + currColumn ];
        srcCounter++;
      }
      // bottom
      if ( currRow < (maxRow-step) && condition[ (currRow+step)*maxColumn + currColumn ] >= threshold )
      {
        predBlocks[6] = condition[ (currRow+step)*maxColumn + currColumn ];
        srcCounter++;
      }

      if ( currColumn > 0 )
      {
        // left
        if ( condition[ currRow*maxColumn + currColumn - 1 ] >= threshold )
        {
          predBlocks[5] = condition[ currRow*maxColumn + currColumn - 1 ];
          srcCounter++;
        }

        if ( !fNoCornerNeigh )
        {
          // top-left
          if ( currRow > 0 && condition[ (currRow-1)*maxColumn + currColumn - 1 ] >= threshold )
          {
            predBlocks[1] = condition[ (currRow-1)*maxColumn + currColumn - 1 ];
            srcCounter++;
          }
          // bottom-left
          if ( currRow < (maxRow-step) && condition[ (currRow+step)*maxColumn + currColumn - 1 ] >= threshold )
          {
            predBlocks[2] = condition[ (currRow+step)*maxColumn + currColumn - 1 ];
            srcCounter++;
          }
        }
      }

      if ( currColumn < (maxColumn-step) )
      {
        // right
        if ( condition[ currRow*maxColumn+currColumn + step ] >= threshold )
        {
          predBlocks[7] = condition[ currRow*maxColumn+currColumn + step ];
          srcCounter++;
        }

        if ( !fNoCornerNeigh )
        {
          // top-right
          if ( currRow > 0 && condition[ (currRow-1)*maxColumn + currColumn + step ] >= threshold )
          {
            predBlocks[0] = condition[ (currRow-1)*maxColumn + currColumn + step ];
            srcCounter++;
          }
          // bottom-right
          if ( currRow < (maxRow-step) && condition[ (currRow+step)*maxColumn + currColumn + step ] >= threshold )
          {
            predBlocks[3] = condition[ (currRow+step)*maxColumn + currColumn + step ];
            srcCounter++;
          }
        }
      }
      // prepare for the next round
      threshold--;
      if (threshold < ERC_BLOCK_CONCEALED)
        break;
    } while ( srcCounter < srcCountMin);

    return srcCounter;
  }
  //}}}
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      Does the actual pixel based interpolation for block[]
   *      using weighted average
   * \param decoder
   *      video encoding parameters for current picture
   * \param src[]
   *      pointers to neighboring source blocks
   * \param block
   *      destination block
   * \param blockSize
   *      16 for Y, 8 for U/V components
   * \param frameWidth
   *      Width of the frame in pixels
  ** **********************************************************************
   */
  void pixMeanInterpolateBlock (cDecoder264* decoder, sPixel *src[], sPixel *block, int blockSize, int frameWidth )
  {
    int row, column, k, tmp, srcCounter = 0, weight = 0, bmax = blockSize - 1;

    k = 0;
    for ( row = 0; row < blockSize; row++ )
    {
      for ( column = 0; column < blockSize; column++ )
      {
        tmp = 0;
        srcCounter = 0;
        // above
        if ( src[4] != NULL )
        {
          weight = blockSize-row;
          tmp += weight * (*(src[4]+bmax*frameWidth+column));
          srcCounter += weight;
        }
        // left
        if ( src[5] != NULL )
        {
          weight = blockSize-column;
          tmp += weight * (*(src[5]+row*frameWidth+bmax));
          srcCounter += weight;
        }
        // below
        if ( src[6] != NULL )
        {
          weight = row+1;
          tmp += weight * (*(src[6]+column));
          srcCounter += weight;
        }
        // right
        if ( src[7] != NULL )
        {
          weight = column+1;
          tmp += weight * (*(src[7]+row*frameWidth));
          srcCounter += weight;
        }

        if ( srcCounter > 0 )
          block[ k + column ] = (sPixel)(tmp/srcCounter);
        else
          block[ k + column ] = (sPixel) (blockSize == 8 ? decoder->coding.dcPredValueComp[1] : decoder->coding.dcPredValueComp[0]);
      }
      k += frameWidth;
    }
  }
  //}}}
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      Conceals the MB at position (row, column) using pixels from predBlocks[]
   *      using pixMeanInterpolateBlock()
   * \param decoder
   *      video encoding parameters for current picture
   * \param currFrame
   *      current frame
   * \param row
   *      y coordinate in blocks
   * \param column
   *      x coordinate in blocks
   * \param predBlocks[]
   *      list of neighboring source blocks (numbering 0 to 7, 1 means: use the neighbor)
   * \param frameWidth
   *      width of frame in pixels
   * \param mbWidthInBlocks
   *      2 for Y, 1 for U/V components
  ** **********************************************************************
   */
  void ercPixConcealIMB (cDecoder264* decoder, sPixel* currFrame, int row, int column, int predBlocks[], int frameWidth, int mbWidthInBlocks)
  {
     sPixel *src[8]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
     sPixel* currBlock = NULL;

     // collect the reliable neighboring blocks
     if (predBlocks[0])
        src[0] = currFrame + (row-mbWidthInBlocks)*frameWidth*8 + (column+mbWidthInBlocks)*8;
     if (predBlocks[1])
        src[1] = currFrame + (row-mbWidthInBlocks)*frameWidth*8 + (column-mbWidthInBlocks)*8;
     if (predBlocks[2])
        src[2] = currFrame + (row+mbWidthInBlocks)*frameWidth*8 + (column-mbWidthInBlocks)*8;
     if (predBlocks[3])
        src[3] = currFrame + (row+mbWidthInBlocks)*frameWidth*8 + (column+mbWidthInBlocks)*8;
     if (predBlocks[4])
        src[4] = currFrame + (row-mbWidthInBlocks)*frameWidth*8 + column*8;
     if (predBlocks[5])
        src[5] = currFrame + row*frameWidth*8 + (column-mbWidthInBlocks)*8;
     if (predBlocks[6])
        src[6] = currFrame + (row+mbWidthInBlocks)*frameWidth*8 + column*8;
     if (predBlocks[7])
        src[7] = currFrame + row*frameWidth*8 + (column+mbWidthInBlocks)*8;

     currBlock = currFrame + row*frameWidth*8 + column*8;
     pixMeanInterpolateBlock( decoder, src, currBlock, mbWidthInBlocks*8, frameWidth );
  }
  //}}}
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      Core for the Intra blocks conceal.
   *      It is called for each color component (Y,U,V) separately
   *      Finds the corrupted blocks and calls pixel interpolation functions
   *      to correct them, one block at a time.
   *      Scanning is done vertically and each corrupted column is corrected
   *      bi-directionally, i.e., first block, last block, first block+1, last block -1 ...
   * \param decoder
   *      video encoding parameters for current picture
   * \param lastColumn
   *      Number of block columns in the frame
   * \param lastRow
   *      Number of block rows in the frame
   * \param comp
   *      color component
   * \param recfr
   *      Reconstructed frame buffer
   * \param picSizeX
   *      Width of the frame in pixels
   * \param condition
   *      The block condition (ok, lost) table
  ** **********************************************************************
   */
  void concealBlocks (cDecoder264* decoder, int lastColumn, int lastRow, int comp, frame *recfr, int picSizeX, char *condition )
  {
    //int row, column, srcCounter = 0,  thr = ERC_BLOCK_CORRUPTED,
    int row, column, thr = ERC_BLOCK_CORRUPTED,
        lastCorruptedRow = -1, firstCorruptedRow = -1, currRow = 0,
        areaHeight = 0, i = 0, smoothColumn = 0;
    int predBlocks[8], step = 1;

    // in the Y component do the conceal MB-wise (not block-wise):
    // this is useful if only whole MBs can be damaged or lost
    if ( comp == 0 )
      step = 2;
    else
      step = 1;

    for ( column = 0; column < lastColumn; column += step )
    {
      for ( row = 0; row < lastRow; row += step )
      {
        if ( condition[row*lastColumn+column] <= thr )
        {
          firstCorruptedRow = row;
          // find the last row which has corrupted blocks (in same continuous area)
          for ( lastCorruptedRow = row+step; lastCorruptedRow < lastRow; lastCorruptedRow += step )
          {
            // check blocks in the current column
            if ( condition[ lastCorruptedRow*lastColumn + column ] > thr )
            {
              // current one is already OK, so the last was the previous one
              lastCorruptedRow -= step;
              break;
            }
          }
          if ( lastCorruptedRow >= lastRow )
          {
            // correct only from above
            lastCorruptedRow = lastRow-step;
            for ( currRow = firstCorruptedRow; currRow < lastRow; currRow += step )
            {
              //srcCounter = ercCollect8PredBlocks( predBlocks, currRow, column, condition, lastRow, lastColumn, step, 1 );
              ercCollect8PredBlocks( predBlocks, currRow, column, condition, lastRow, lastColumn, step, 1 );

              switch( comp )
              {
              case 0 :
                ercPixConcealIMB( decoder, recfr->yptr, currRow, column, predBlocks, picSizeX, 2 );
                break;
              case 1 :
                ercPixConcealIMB( decoder, recfr->uptr, currRow, column, predBlocks, (picSizeX>>1), 1 );
                break;
              case 2 :
                ercPixConcealIMB( decoder, recfr->vptr, currRow, column, predBlocks, (picSizeX>>1), 1 );
                break;
              }

              if ( comp == 0 )
              {
                condition[ currRow*lastColumn+column] = ERC_BLOCK_CONCEALED;
                condition[ currRow*lastColumn+column + 1] = ERC_BLOCK_CONCEALED;
                condition[ currRow*lastColumn+column + lastColumn] = ERC_BLOCK_CONCEALED;
                condition[ currRow*lastColumn+column + lastColumn + 1] = ERC_BLOCK_CONCEALED;
              }
              else
              {
                condition[ currRow*lastColumn+column] = ERC_BLOCK_CONCEALED;
              }

            }
            row = lastRow;
          }
          else if ( firstCorruptedRow == 0 )
          {
            // correct only from below
            for ( currRow = lastCorruptedRow; currRow >= 0; currRow -= step )
            {
              //srcCounter = ercCollect8PredBlocks( predBlocks, currRow, column, condition, lastRow, lastColumn, step, 1 );
              ercCollect8PredBlocks( predBlocks, currRow, column, condition, lastRow, lastColumn, step, 1 );

              switch( comp )
              {
              case 0 :
                ercPixConcealIMB( decoder, recfr->yptr, currRow, column, predBlocks, picSizeX, 2 );
                break;
              case 1 :
                ercPixConcealIMB( decoder, recfr->uptr, currRow, column, predBlocks, (picSizeX>>1), 1 );
                break;
              case 2 :
                ercPixConcealIMB( decoder, recfr->vptr, currRow, column, predBlocks, (picSizeX>>1), 1 );
                break;
              }

              if ( comp == 0 )
              {
                condition[ currRow*lastColumn+column] = ERC_BLOCK_CONCEALED;
                condition[ currRow*lastColumn+column + 1] = ERC_BLOCK_CONCEALED;
                condition[ currRow*lastColumn+column + lastColumn] = ERC_BLOCK_CONCEALED;
                condition[ currRow*lastColumn+column + lastColumn + 1] = ERC_BLOCK_CONCEALED;
              }
              else
              {
                condition[ currRow*lastColumn+column] = ERC_BLOCK_CONCEALED;
              }

            }

            row = lastCorruptedRow+step;
          }
          else
          {
            // correct bi-directionally

            row = lastCorruptedRow+step;
            areaHeight = lastCorruptedRow-firstCorruptedRow+step;

            // Conceal the corrupted area switching between the up and the bottom rows
            for ( i = 0; i < areaHeight; i += step )
            {
              if ( i % 2 )
              {
                currRow = lastCorruptedRow;
                lastCorruptedRow -= step;
              }
              else
              {
                currRow = firstCorruptedRow;
                firstCorruptedRow += step;
              }

              if (smoothColumn > 0)
              {
                //srcCounter = ercCollectColumnBlocks( predBlocks, currRow, column, condition, lastRow, lastColumn, step );
                ercCollectColumnBlocks( predBlocks, currRow, column, condition, lastRow, lastColumn, step );
              }
              else
              {
                //srcCounter = ercCollect8PredBlocks( predBlocks, currRow, column, condition, lastRow, lastColumn, step, 1 );
                ercCollect8PredBlocks( predBlocks, currRow, column, condition, lastRow, lastColumn, step, 1 );
              }

              switch( comp )
              {
              case 0 :
                ercPixConcealIMB( decoder, recfr->yptr, currRow, column, predBlocks, picSizeX, 2 );
                break;

              case 1 :
                ercPixConcealIMB( decoder, recfr->uptr, currRow, column, predBlocks, (picSizeX>>1), 1 );
                break;

              case 2 :
                ercPixConcealIMB( decoder, recfr->vptr, currRow, column, predBlocks, (picSizeX>>1), 1 );
                break;
              }

              if ( comp == 0 )
              {
                condition[ currRow*lastColumn+column] = ERC_BLOCK_CONCEALED;
                condition[ currRow*lastColumn+column + 1] = ERC_BLOCK_CONCEALED;
                condition[ currRow*lastColumn+column + lastColumn] = ERC_BLOCK_CONCEALED;
                condition[ currRow*lastColumn+column + lastColumn + 1] = ERC_BLOCK_CONCEALED;
              }
              else
              {
                condition[ currRow*lastColumn+column ] = ERC_BLOCK_CONCEALED;
              }
            }
          }

          lastCorruptedRow = -1;
          firstCorruptedRow = -1;

        }
      }
    }
  }
  //}}}

  // interFrame
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      Copies pixel values between a YUV frame and the temporary pixel value storage place. This is
   *      used to save some pixel values temporarily before overwriting it, or to copy back to a given
   *      location in a frame the saved pixel values.
   * \param currYBlockNum
   *      index of the block (8x8) in the Y plane
   * \param predMB
   *      memory area where the temporary pixel values are stored
   *      the Y,U,V planes are concatenated y = predMB, u = predMB+256, v = predMB+320
   * \param recfr
   *      pointer to a YUV frame
   * \param picSizeX
   *      picture width in pixels
   * \param regionSize
   *      can be 16 or 8 to tell the dimension of the region to copy
  ** **********************************************************************
   */
  void copyPredMB (int currYBlockNum, sPixel *predMB, frame *recfr,
                          int picSizeX, int regionSize)
  {
    cDecoder264* decoder = recfr->decoder;
    sPicture* picture = decoder->picture;
    int j, k, xmin, ymin, xmax, ymax;
    int locationTmp;
    int uv_x = uv_div[0][picture->chromaFormatIdc];
    int uv_y = uv_div[1][picture->chromaFormatIdc];

    xmin = (xPosYBlock(currYBlockNum,picSizeX)<<3);
    ymin = (yPosYBlock(currYBlockNum,picSizeX)<<3);
    xmax = xmin + regionSize -1;
    ymax = ymin + regionSize -1;

    for (j = ymin; j <= ymax; j++)
    {
      for (k = xmin; k <= xmax; k++)
      {
        locationTmp = (j-ymin) * 16 + (k-xmin);
        picture->imgY[j][k] = predMB[locationTmp];
      }
    }

    if (picture->chromaFormatIdc != YUV400)
    {
      for (j = (ymin>>uv_y); j <= (ymax>>uv_y); j++)
      {
        for (k = (xmin>>uv_x); k <= (xmax>>uv_x); k++)
        {
          locationTmp = (j-(ymin>>uv_y)) * decoder->mbCrSizeX + (k-(xmin>>1)) + 256;
          picture->imgUV[0][j][k] = predMB[locationTmp];

          locationTmp += 64;

          picture->imgUV[1][j][k] = predMB[locationTmp];
        }
      }
    }
  }
  //}}}
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      Calculates a weighted pixel difference between edge Y pixels of the macroBlock stored in predMB
   *      and the pixels in the given Y plane of a frame (recY) that would become neighbor pixels if
   *      predMB was placed at currYBlockNum block position into the frame. This "edge distortion" value
   *      is used to determine how well the given macroBlock in predMB would fit into the frame when
   *      considering spatial smoothness. If there are correctly received neighbor blocks (status stored
   *      in predBlocks) only they are used in calculating the edge distorion; otherwise also the already
   *      concealed neighbor blocks can also be used.
   * \return
   *      The calculated weighted pixel difference at the edges of the MB.
   * \param predBlocks
   *      status array of the neighboring blocks (if they are OK, concealed or lost)
   * \param currYBlockNum
   *      index of the block (8x8) in the Y plane
   * \param predMB
   *      memory area where the temporary pixel values are stored
   *      the Y,U,V planes are concatenated y = predMB, u = predMB+256, v = predMB+320
   * \param recY
   *      pointer to a Y plane of a YUV frame
   * \param picSizeX
   *      picture width in pixels
   * \param regionSize
   *      can be 16 or 8 to tell the dimension of the region to copy
  ** **********************************************************************
   */
  int edgeDistortion (int predBlocks[], int currYBlockNum, sPixel *predMB,
                             sPixel *recY, int picSizeX, int regionSize)
  {
    int i, j, distortion, numOfPredBlocks, threshold = ERC_BLOCK_OK;
    sPixel* currBlock = NULL, *neighbor = NULL;
    int currBlockOffset = 0;

    currBlock = recY + (yPosYBlock(currYBlockNum,picSizeX)<<3)*picSizeX + (xPosYBlock(currYBlockNum,picSizeX)<<3);

    do
    {

      distortion = 0; numOfPredBlocks = 0;

      // loop the 4 neighbors
      for (j = 4; j < 8; j++)
      {
        /* if reliable, count boundary pixel difference */
        if (predBlocks[j] >= threshold)
        {

          switch (j)
          {
          case 4:
            neighbor = currBlock - picSizeX;
            for ( i = 0; i < regionSize; i++ )
            {
              distortion += iabs((int)(predMB[i] - neighbor[i]));
            }
            break;
          case 5:
            neighbor = currBlock - 1;
            for ( i = 0; i < regionSize; i++ )
            {
              distortion += iabs((int)(predMB[i*16] - neighbor[i*picSizeX]));
            }
            break;
          case 6:
            neighbor = currBlock + regionSize*picSizeX;
            currBlockOffset = (regionSize-1)*16;
            for ( i = 0; i < regionSize; i++ )
            {
              distortion += iabs((int)(predMB[i+currBlockOffset] - neighbor[i]));
            }
            break;
          case 7:
            neighbor = currBlock + regionSize;
            currBlockOffset = regionSize-1;
            for ( i = 0; i < regionSize; i++ )
            {
              distortion += iabs((int)(predMB[i*16+currBlockOffset] - neighbor[i*picSizeX]));
            }
            break;
          }

          numOfPredBlocks++;
        }
      }

      threshold--;
      if (threshold < ERC_BLOCK_CONCEALED)
        break;
    } while (numOfPredBlocks == 0);

    if(numOfPredBlocks == 0)
    {
      return 0;
    }
    return (distortion/numOfPredBlocks);
  }
  //}}}
  //{{{
  /*!
  ************************************************************************
  * \brief
  *      Builds the motion prediction pixels from the given location (in 1/4 pixel units)
  *      of the reference frame. It not only copies the pixel values but builds the interpolation
  *      when the pixel positions to be copied from is not full pixel (any 1/4 pixel position).
  *      It copies the resulting pixel vlaues into predMB.
  * \param decoder
  *      The pointer of Decoder picStructure of current frame
  * \param mv
  *      The pointer of the predicted MV of the current (being concealed) MB
  * \param x
  *      The x-coordinate of the above-left corner pixel of the current MB
  * \param y
  *      The y-coordinate of the above-left corner pixel of the current MB
  * \param predMB
  *      memory area for storing temporary pixel values for a macroBlock
  *      the Y,U,V planes are concatenated y = predMB, u = predMB+256, v = predMB+320
  ************************************************************************
  */
  void buildPredRegionYUV (cDecoder264* decoder, int *mv, int x, int y, sPixel *predMB)
  {
    sPixel** tmp_block;
    int i=0, j=0, ii=0, jj=0,i1=0,j1=0,j4=0,i4=0;
    int uv;
    int vec1_x=0,vec1_y=0;
    int ioff,joff;
    sPixel *pMB = predMB;
    cSlice* slice;// = decoder->currentSlice;
    sPicture* picture = decoder->picture;
    int ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;
    int mv_mul;

    //FRExt
    int f1_x, f1_y, f2_x, f2_y, f3, f4;
    int b8, b4;
    int yuv = picture->chromaFormatIdc - 1;

    int ref_frame = imax (mv[2], 0); // !!KS: quick fix, we sometimes seem to get negative refPic here, so restrict to zero and above
    int mb_nr = y/16*(decoder->coding.width/16)+x/16; ///slice->mbIndex;
    int** tempRes = NULL;

    sMacroBlock* mb = &decoder->mbData[mb_nr];   // intialization code deleted, see below, StW
    slice = mb->slice;
    tempRes = slice->tempRes;

    // This should be allocated only once.
    getMem2Dpel(&tmp_block, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

    /* Update coordinates of the current concealed macroBlock */
    mb->mb.x = (int16_t) (x/MB_BLOCK_SIZE);
    mb->mb.y = (int16_t) (y/MB_BLOCK_SIZE);
    mb->blockY = mb->mb.y * BLOCK_SIZE;
    mb->piccY = mb->mb.y * decoder->mbCrSizeY;
    mb->blockX = mb->mb.x * BLOCK_SIZE;
    mb->pixcX = mb->mb.x * decoder->mbCrSizeX;

    mv_mul=4;

    // luma** *****************************************************

    for(j=0;j<MB_BLOCK_SIZE/BLOCK_SIZE;j++)
    {
      joff=j*4;
      j4=mb->blockY+j;
      for(i=0;i<MB_BLOCK_SIZE/BLOCK_SIZE;i++)
      {
        ioff=i*4;
        i4=mb->blockX+i;

        vec1_x = i4*4*mv_mul + mv[0];
        vec1_y = j4*4*mv_mul + mv[1];

        getBlockLuma (slice->listX[0][ref_frame], vec1_x, vec1_y, BLOCK_SIZE, BLOCK_SIZE,
          tmp_block,
          picture->lumaStride,picture->size_x_m1,
          (mb->mbField) ? (picture->sizeY >> 1) - 1 : picture->size_y_m1,tempRes,
          decoder->coding.maxPelValueComp[PLANE_Y],(sPixel) decoder->coding.dcPredValueComp[PLANE_Y], mb);

        for(ii=0;ii<BLOCK_SIZE;ii++)
          for(jj=0;jj<MB_BLOCK_SIZE/BLOCK_SIZE;jj++)
            slice->mbPred[eLumaComp][jj+joff][ii+ioff]=tmp_block[jj][ii];
      }
    }


    for (j = 0; j < 16; j++)
    {
      for (i = 0; i < 16; i++)
      {
        pMB[j*16+i] = slice->mbPred[eLumaComp][j][i];
      }
    }
    pMB += 256;

    if (picture->chromaFormatIdc != YUV400)
    {
      // chroma** *****************************************************
      f1_x = 64/decoder->mbCrSizeX;
      f2_x=f1_x-1;

      f1_y = 64/decoder->mbCrSizeY;
      f2_y=f1_y-1;

      f3=f1_x*f1_y;
      f4=f3>>1;

      for(uv=0;uv<2;uv++)
      {
        for (b8=0;b8<(decoder->coding.numUvBlocks);b8++)
        {
          for(b4=0;b4<4;b4++)
          {
            joff = subblk_offset_y[yuv][b8][b4];
            j4=mb->piccY+joff;
            ioff = subblk_offset_x[yuv][b8][b4];
            i4=mb->pixcX+ioff;

            for(jj=0;jj<4;jj++)
            {
              for(ii=0;ii<4;ii++)
              {
                i1=(i4+ii)*f1_x + mv[0];
                j1=(j4+jj)*f1_y + mv[1];

                ii0=iClip3 (0, picture->sizeXcr-1, i1/f1_x);
                jj0=iClip3 (0, picture->sizeYcr-1, j1/f1_y);
                ii1=iClip3 (0, picture->sizeXcr-1, ((i1+f2_x)/f1_x));
                jj1=iClip3 (0, picture->sizeYcr-1, ((j1+f2_y)/f1_y));

                if1=(i1 & f2_x);
                jf1=(j1 & f2_y);
                if0=f1_x-if1;
                jf0=f1_y-jf1;

                slice->mbPred[uv + 1][jj+joff][ii+ioff] = (sPixel)
                  ((if0*jf0*slice->listX[0][ref_frame]->imgUV[uv][jj0][ii0]+
                  if1*jf0*slice->listX[0][ref_frame]->imgUV[uv][jj0][ii1]+
                  if0*jf1*slice->listX[0][ref_frame]->imgUV[uv][jj1][ii0]+
                  if1*jf1*slice->listX[0][ref_frame]->imgUV[uv][jj1][ii1]+f4)/f3);
              }
            }
          }
        }

        for (j = 0; j < 8; j++)
        {
          for (i = 0; i < 8; i++)
          {
            pMB[j*8+i] = slice->mbPred[uv + 1][j][i];
          }
        }
        pMB += 64;

      }
    }
    // We should allocate this memory only once.
    freeMem2Dpel(tmp_block);
  }
  //}}}
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      Copies the co-located pixel values from the reference to the current frame.
   *      Used by concealByCopy
   * \param recfr
   *      Reconstructed frame buffer
   * \param currYBlockNum
   *      index of the block (8x8) in the Y plane
   * \param picSizeX
   *      Width of the frame in pixels
   * \param regionSize
   *      can be 16 or 8 to tell the dimension of the region to copy
  ** **********************************************************************
   */
  void copyBetweenFrames (frame* recfr, int currYBlockNum, int picSizeX, int regionSize)
  {
    cDecoder264* decoder = recfr->decoder;
    sPicture* picture = decoder->picture;
    int j, k, location, xmin, ymin;
    sPicture* refPic = decoder->sliceList[0]->listX[0][0];

    /* set the position of the region to be copied */
    xmin = (xPosYBlock(currYBlockNum,picSizeX)<<3);
    ymin = (yPosYBlock(currYBlockNum,picSizeX)<<3);

    for (j = ymin; j < ymin + regionSize; j++)
      for (k = xmin; k < xmin + regionSize; k++) {
        location = j * picSizeX + k;
        recfr->yptr[location] = refPic->imgY[j][k];
        }

    for (j = ymin >> uv_div[1][picture->chromaFormatIdc]; j < (ymin + regionSize) >> uv_div[1][picture->chromaFormatIdc]; j++)
      for (k = xmin >> uv_div[0][picture->chromaFormatIdc]; k < (xmin + regionSize) >> uv_div[0][picture->chromaFormatIdc]; k++) {
        location = ((j * picSizeX) >> uv_div[0][picture->chromaFormatIdc]) + k;
        recfr->uptr[location] = refPic->imgUV[0][j][k];
        recfr->vptr[location] = refPic->imgUV[1][j][k];
        }
    }
  //}}}
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      It conceals a given MB by simply copying the pixel area from the reference image
   *      that is at the same location as the macroBlock in the current image. This correcponds
   *      to COPY MBs.
   * \return
   *      Always zero (0).
   * \param recfr
   *      Reconstructed frame buffer
   * \param currMBNum
   *      current MB index
   * \param object_list
   *      Motion info for all MBs in the frame
   * \param picSizeX
   *      Width of the frame in pixels
  ** **********************************************************************
   */
  int concealByCopy (frame* recfr, int currMBNum, sObjectBuffer *object_list, int picSizeX)
  {
    sObjectBuffer* currRegion;

    currRegion = object_list+(currMBNum<<2);
    currRegion->regionMode = REGMODE_INTER_COPY;

    currRegion->xMin = (xPosMB(currMBNum,picSizeX)<<4);
    currRegion->yMin = (yPosMB(currMBNum,picSizeX)<<4);

    copyBetweenFrames (recfr, MBNum2YBlock(currMBNum,0,picSizeX), picSizeX, 16);

    return 0;
  }
  //}}}
  //{{{
  /*!
  ** **********************************************************************
   * \brief
   *      It conceals a given MB by using the motion vectors of one reliable neighbor. That MV of a
   *      neighbor is selected wich gives the lowest pixel difference at the edges of the MB
   *      (see function edgeDistortion). This corresponds to a spatial smoothness criteria.
   * \return
   *      Always zero (0).
   * \param recfr
   *      Reconstructed frame buffer
   * \param predMB
   *      memory area for storing temporary pixel values for a macroBlock
   *      the Y,U,V planes are concatenated y = predMB, u = predMB+256, v = predMB+320
   * \param currMBNum
   *      current MB index
   * \param object_list
   *      array of region structures storing region mode and mv for each region
   * \param predBlocks
   *      status array of the neighboring blocks (if they are OK, concealed or lost)
   * \param picSizeX
   *      Width of the frame in pixels
   * \param picSizeY
   *      Height of the frame in pixels
   * \param yCondition
   *      array for conditions of Y blocks from sErcVariables
  ** **********************************************************************
   */
  int concealByTrial (frame* recfr, sPixel *predMB,
                             int currMBNum, sObjectBuffer *object_list, int predBlocks[],
                             int picSizeX, int picSizeY, char *yCondition)
  {
    cDecoder264* decoder = recfr->decoder;

    int predMBNum = 0, numMBPerLine,
        compSplit1 = 0, compSplit2 = 0, compLeft = 1, comp = 0, compPred, order = 1,
        fInterNeighborExists, numIntraNeighbours, fZeroMotionChecked, predSplitted = 0,
        threshold = ERC_BLOCK_OK,
        minDist, currDist, i, k;
    int regionSize;
    sObjectBuffer* currRegion;
    int mvBest[3] = {0, 0, 0}, mvPred[3] = {0, 0, 0}, *mvptr;

    numMBPerLine = (int) (picSizeX>>4);

    comp = 0;
    regionSize = 16;

    do { /* 4 blocks loop */

      currRegion = object_list+(currMBNum<<2)+comp;

      /* set the position of the region to be concealed */

      currRegion->xMin = (xPosYBlock(MBNum2YBlock(currMBNum,comp,picSizeX),picSizeX)<<3);
      currRegion->yMin = (yPosYBlock(MBNum2YBlock(currMBNum,comp,picSizeX),picSizeX)<<3);

      do { /* reliability loop */

        minDist = 0;
        fInterNeighborExists = 0;
        numIntraNeighbours = 0;
        fZeroMotionChecked = 0;

        /* loop the 4 neighbours */
        for (i = 4; i < 8; i++) {

          /* if reliable, try it */
          if (predBlocks[i] >= threshold) {
            switch (i) {
            case 4:
              predMBNum = currMBNum-numMBPerLine;
              compSplit1 = 2;
              compSplit2 = 3;
              break;

            case 5:
              predMBNum = currMBNum-1;
              compSplit1 = 1;
              compSplit2 = 3;
              break;

            case 6:
              predMBNum = currMBNum+numMBPerLine;
              compSplit1 = 0;
              compSplit2 = 1;
              break;

            case 7:
              predMBNum = currMBNum+1;
              compSplit1 = 0;
              compSplit2 = 2;
              break;
            }

            /* try the conceal with the Motion Info of the current neighbour
            only try if the neighbour is not Intra */
            if (isBlock(object_list,predMBNum,compSplit1,INTRA) ||
              isBlock(object_list,predMBNum,compSplit2,INTRA))
            {
              numIntraNeighbours++;
            }
            else
            {
              /* if neighbour MB is splitted, try both neighbour blocks */
              for (predSplitted = isSplit(object_list, predMBNum),
                compPred = compSplit1;
                predSplitted >= 0;
                compPred = compSplit2,
                predSplitted -= ((compSplit1 == compSplit2) ? 2 : 1))
              {

                /* if Zero Motion Block, do the copying. This option is tried only once */
                if (isBlock(object_list, predMBNum, compPred, INTER_COPY))
                {

                  if (fZeroMotionChecked)
                  {
                    continue;
                  }
                  else
                  {
                    fZeroMotionChecked = 1;

                    mvPred[0] = mvPred[1] = 0;
                    mvPred[2] = 0;

                    buildPredRegionYUV (decoder, mvPred, currRegion->xMin, currRegion->yMin, predMB);
                  }
                }
                /* build motion using the neighbour's Motion Parameters */
                else if (isBlock(object_list,predMBNum,compPred,INTRA))
                {
                  continue;
                }
                else
                {
                  mvptr = getParam(object_list, predMBNum, compPred, mv);

                  mvPred[0] = mvptr[0];
                  mvPred[1] = mvptr[1];
                  mvPred[2] = mvptr[2];

                  buildPredRegionYUV (decoder, mvPred, currRegion->xMin, currRegion->yMin, predMB);
                }

                /* measure absolute boundary pixel difference */
                currDist = edgeDistortion(predBlocks,
                  MBNum2YBlock(currMBNum,comp,picSizeX),
                  predMB, recfr->yptr, picSizeX, regionSize);

                /* if so far best -> store the pixels as the best conceal */
                if (currDist < minDist || !fInterNeighborExists)
                {

                  minDist = currDist;

                  for (k=0;k<3;k++)
                    mvBest[k] = mvPred[k];

                  currRegion->regionMode =
                    (isBlock(object_list, predMBNum, compPred, INTER_COPY)) ?
                    ((regionSize == 16) ? REGMODE_INTER_COPY : REGMODE_INTER_COPY_8x8) :
                    ((regionSize == 16) ? REGMODE_INTER_PRED : REGMODE_INTER_PRED_8x8);

                  copyPredMB(MBNum2YBlock(currMBNum,comp,picSizeX), predMB, recfr,
                    picSizeX, regionSize);
                }

                fInterNeighborExists = 1;
              }
            }
          }
      }

      threshold--;

      } while ((threshold >= ERC_BLOCK_CONCEALED) && (fInterNeighborExists == 0));

      /* always try zero motion */
      if (!fZeroMotionChecked)
      {
        mvPred[0] = mvPred[1] = 0;
        mvPred[2] = 0;

        buildPredRegionYUV (decoder, mvPred, currRegion->xMin, currRegion->yMin, predMB);

        currDist = edgeDistortion(predBlocks,
          MBNum2YBlock(currMBNum,comp,picSizeX),
          predMB, recfr->yptr, picSizeX, regionSize);

        if (currDist < minDist || !fInterNeighborExists)
        {

          minDist = currDist;
          for (k=0;k<3;k++)
            mvBest[k] = mvPred[k];

          currRegion->regionMode =
            ((regionSize == 16) ? REGMODE_INTER_COPY : REGMODE_INTER_COPY_8x8);

          copyPredMB(MBNum2YBlock(currMBNum,comp,picSizeX), predMB, recfr,
            picSizeX, regionSize);
        }
      }

      for (i=0; i<3; i++)
        currRegion->mv[i] = mvBest[i];

      yCondition[MBNum2YBlock(currMBNum,comp,picSizeX)] = ERC_BLOCK_CONCEALED;
      comp = (comp+order+4)%4;
      compLeft--;

      } while (compLeft);

      return 0;
  }
  //}}}

  //{{{
  /*!
  ************************************************************************
  * \brief
  * The motion prediction pixels are calculated from the given location (in
  * 1/4 pixel units) of the referenced frame. It copies the sub block from the
  * corresponding reference to the frame to be concealed.
  *
  *************************************************************************
  */
  void buildPredblockRegionYUV (cDecoder264* decoder, int *mv,
                                       int x, int y, sPixel *predMB, int list, int mbIndex)
  {
    sPixel** tmp_block;
    int i=0,j=0,ii=0,jj=0,i1=0,j1=0,j4=0,i4=0;
    int uv;
    int vec1_x=0,vec1_y=0;
    int ioff,joff;

    sPicture* picture = decoder->picture;
    sPixel *pMB = predMB;

    int ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;
    int mv_mul;

    //FRExt
    int f1_x, f1_y, f2_x, f2_y, f3, f4;
    int yuv = picture->chromaFormatIdc - 1;

    int ref_frame = mv[2];
    int mb_nr = mbIndex;

    sMacroBlock* mb = &decoder->mbData[mb_nr];   // intialization code deleted, see below, StW
    cSlice* slice = mb->slice;

    getMem2Dpel(&tmp_block, MB_BLOCK_SIZE, MB_BLOCK_SIZE);

    /* Update coordinates of the current concealed macroBlock */

    mb->mb.x = (int16_t) (x/BLOCK_SIZE);
    mb->mb.y = (int16_t) (y/BLOCK_SIZE);
    mb->blockY = mb->mb.y * BLOCK_SIZE;
    mb->piccY = mb->mb.y * decoder->mbCrSizeY/4;
    mb->blockX = mb->mb.x * BLOCK_SIZE;
    mb->pixcX = mb->mb.x * decoder->mbCrSizeX/4;

    mv_mul=4;

    // luma** *****************************************************

    vec1_x = x*mv_mul + mv[0];
    vec1_y = y*mv_mul + mv[1];
    getBlockLuma (slice->listX[list][ref_frame],  vec1_x, vec1_y, BLOCK_SIZE, BLOCK_SIZE, tmp_block,
      picture->lumaStride,picture->size_x_m1, (mb->mbField) ? (picture->sizeY >> 1) - 1 : picture->size_y_m1,slice->tempRes,
      decoder->coding.maxPelValueComp[PLANE_Y],(sPixel) decoder->coding.dcPredValueComp[PLANE_Y], mb);

    for(jj=0;jj<MB_BLOCK_SIZE/BLOCK_SIZE;jj++)
      for(ii=0;ii<BLOCK_SIZE;ii++)
        slice->mbPred[eLumaComp][jj][ii]=tmp_block[jj][ii];


    for (j = 0; j < 4; j++)
    {
      for (i = 0; i < 4; i++)
      {
        pMB[j*4+i] = slice->mbPred[eLumaComp][j][i];
      }
    }
    pMB += 16;

    if (picture->chromaFormatIdc != YUV400)
    {
      // chroma** *****************************************************
      f1_x = 64/(decoder->mbCrSizeX);
      f2_x=f1_x-1;

      f1_y = 64/(decoder->mbCrSizeY);
      f2_y=f1_y-1;

      f3=f1_x*f1_y;
      f4=f3>>1;

      for(uv=0;uv<2;uv++)
      {
        joff = subblk_offset_y[yuv][0][0];
        j4=mb->piccY+joff;
        ioff = subblk_offset_x[yuv][0][0];
        i4=mb->pixcX+ioff;

        for(jj=0;jj<2;jj++)
        {
          for(ii=0;ii<2;ii++)
          {
            i1=(i4+ii)*f1_x + mv[0];
            j1=(j4+jj)*f1_y + mv[1];

            ii0=iClip3 (0, picture->sizeXcr-1, i1/f1_x);
            jj0=iClip3 (0, picture->sizeYcr-1, j1/f1_y);
            ii1=iClip3 (0, picture->sizeXcr-1, ((i1+f2_x)/f1_x));
            jj1=iClip3 (0, picture->sizeYcr-1, ((j1+f2_y)/f1_y));

            if1=(i1 & f2_x);
            jf1=(j1 & f2_y);
            if0=f1_x-if1;
            jf0=f1_y-jf1;

            slice->mbPred[uv + 1][jj][ii]=(sPixel) ((if0*jf0*slice->listX[list][ref_frame]->imgUV[uv][jj0][ii0]+
              if1*jf0*slice->listX[list][ref_frame]->imgUV[uv][jj0][ii1]+
              if0*jf1*slice->listX[list][ref_frame]->imgUV[uv][jj1][ii0]+
              if1*jf1*slice->listX[list][ref_frame]->imgUV[uv][jj1][ii1]+f4)/f3);
          }
        }

        for (j = 0; j < 2; j++)
        {
          for (i = 0; i < 2; i++)
          {
            pMB[j*2+i] = slice->mbPred[uv + 1][j][i];
          }
        }
        pMB += 4;

      }
    }
    freeMem2Dpel(tmp_block);
  }
  //}}}
  //{{{
  /*!
  ************************************************************************
  * \brief
  *    Copy image data from one array to another array
  ************************************************************************
  */

  void CopyImgData (sPixel** inputY, sPixel** *inputUV, sPixel** outputY, sPixel** *outputUV,
                           int img_width, int img_height, int img_width_cr, int img_height_cr)
  {
    int x, y;

    for (y=0; y<img_height; y++)
      for (x=0; x<img_width; x++)
        outputY[y][x] = inputY[y][x];

    for (y=0; y<img_height_cr; y++)
      for (x=0; x<img_width_cr; x++)
      {
        outputUV[0][y][x] = inputUV[0][y][x];
        outputUV[1][y][x] = inputUV[1][y][x];
      }
  }
  //}}}
  //{{{
  void copyToConceal (sPicture *src, sPicture *dst, cDecoder264* decoder)
  {
    int i = 0;
    int mv[3];
    int multiplier;
    sPixel *predMB, *storeYUV;
    int j, y, x, mbHeight, mbWidth, ii = 0, jj = 0;
    int uv;
    int mm, nn;
    int scale = 1;
    sPicture* picture = decoder->picture;

    int mbIndex = 0;

    dst->picSizeInMbs  = src->picSizeInMbs;

    dst->sliceType = src->sliceType = decoder->concealSliceType;

    dst->isIDR = false; //since we do not want to clears the ref list

    dst->noOutputPriorPicFlag = src->noOutputPriorPicFlag;
    dst->longTermRefFlag = src->longTermRefFlag;
    dst->adaptRefPicBufFlag = src->adaptRefPicBufFlag = 0;
    dst->chromaFormatIdc = src->chromaFormatIdc;
    dst->frameMbOnly = src->frameMbOnly;
    dst->hasCrop = src->hasCrop;
    dst->cropLeft = src->cropLeft;
    dst->cropRight = src->cropRight;
    dst->cropBot = src->cropBot;
    dst->cropTop = src->cropTop;

    dst->qp = src->qp;
    dst->sliceQpDelta = src->sliceQpDelta;

    picture = src;

    // Conceals the missing frame by frame copy conceal
    if (decoder->concealMode==1)
    {
      // We need these initializations for using deblocking filter for frame copy
      // conceal as well.
      dst->picWidthMbs = src->picWidthMbs;
      dst->picSizeInMbs = src->picSizeInMbs;

      CopyImgData( src->imgY, src->imgUV, dst->imgY, dst->imgUV, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr);
    }

    // Conceals the missing frame by motion vector copy conceal
    if (decoder->concealMode==2)
    {
      if (picture->chromaFormatIdc != YUV400)
      {
        storeYUV = (sPixel *) malloc ( (16 + (decoder->mbCrSizeX*decoder->mbCrSizeY)*2/16) * sizeof (sPixel));
      }
      else
      {
        storeYUV = (sPixel *) malloc (16  * sizeof (sPixel));
      }

      dst->picWidthMbs = src->picWidthMbs;
      dst->picSizeInMbs = src->picSizeInMbs;
      mbWidth = dst->picWidthMbs;
      mbHeight = (dst->picSizeInMbs)/(dst->picWidthMbs);
      scale = (decoder->concealSliceType == eSliceB) ? 2 : 1;

      if(decoder->concealSliceType == eSliceB)
      {
        initListsForNonRefLoss (decoder->dpb, dst->sliceType, decoder->sliceList[0]->picStructure);
      }
      else
        decoder->sliceList[0]->initLists(decoder->sliceList[0]); //decoder->currentSlice);

      multiplier = BLOCK_SIZE;

      for(i=0;i<mbHeight*4;i++)
      {
        mm = i * BLOCK_SIZE;
        for(j=0;j<mbWidth*4;j++)
        {
          nn = j * BLOCK_SIZE;

          mv[0] = src->mvInfo[i][j].mv[LIST_0].mvX / scale;
          mv[1] = src->mvInfo[i][j].mv[LIST_0].mvY / scale;
          mv[2] = src->mvInfo[i][j].refIndex[LIST_0];

          if(mv[2]<0)
            mv[2]=0;

          dst->mvInfo[i][j].mv[LIST_0].mvX = (int16_t) mv[0];
          dst->mvInfo[i][j].mv[LIST_0].mvY = (int16_t) mv[1];
          dst->mvInfo[i][j].refIndex[LIST_0] = (char) mv[2];

          x = (j) * multiplier;
          y = (i) * multiplier;

          if ((mm%16==0) && (nn%16==0))
            mbIndex++;

          buildPredblockRegionYUV (decoder, mv, x, y, storeYUV, LIST_0, mbIndex);

          predMB = storeYUV;

          for(ii=0;ii<multiplier;ii++)
          {
            for(jj=0;jj<multiplier;jj++)
            {
              dst->imgY[i*multiplier+ii][j*multiplier+jj] = predMB[ii*(multiplier)+jj];
            }
          }

          predMB = predMB + (multiplier*multiplier);

          if (picture->chromaFormatIdc != YUV400)
          {

            for(uv=0;uv<2;uv++)
            {
              for(ii=0;ii< (multiplier/2);ii++)
              {
                for(jj=0;jj< (multiplier/2);jj++)
                {
                  dst->imgUV[uv][i*multiplier/2 +ii][j*multiplier/2 +jj] = predMB[ii*(multiplier/2)+jj];
                }
              }
              predMB = predMB + (multiplier*multiplier/4);
            }
          }
        }
      }
      free(storeYUV);
    }
  }
  //}}}
  //{{{
  /*!
  ************************************************************************
  * \brief
  * Uses the previous reference pic for conceal of reference frames
  *
  ************************************************************************
  */

  void copyPrevPicToConcealPic (sPicture *picture, cDpb* dpb) {

    sPicture* refPic = dpb->getLastPicRefFromDpb();
    dpb->decoder->concealSliceType = eSliceP;
    copyToConceal (refPic, picture, dpb->decoder);
    }
  //}}}
  //{{{
  sPicture* getPicFromDpb (cDpb* dpb, int missingpoc, uint32_t *pos) {

    int usedSize = dpb->usedSize - 1;
    int concealfrom = 0;

    if (dpb->decoder->concealMode == 1)
      concealfrom = missingpoc - dpb->decoder->param.pocGap;
    else if (dpb->decoder->concealMode == 2)
      concealfrom = missingpoc + dpb->decoder->param.pocGap;

    for (int i = usedSize; i >= 0; i--) {
      if (dpb->frameStore[i]->poc == concealfrom) {
        *pos = i;
        return dpb->frameStore[i]->frame;
        }
      }

    return NULL;
    }
  //}}}
  //{{{
  int comp (const void *i, const void *j) {
    return *(int *)i - *(int *)j;
    }
  //}}}
  //{{{
  void addNode (cDecoder264* decoder, struct sConcealNode *concealment_new )
  {
    if( decoder->concealHead == NULL ) {
      decoder->concealTail = decoder->concealHead = concealment_new;
      return;
      }
    decoder->concealTail->next = concealment_new;
    decoder->concealTail = concealment_new;
  }
  //}}}
  //{{{
  void deleteNode (cDecoder264* decoder, struct sConcealNode *ptr ) {

    // We only need to delete the first node in the linked list
    if( ptr == decoder->concealHead ) {
      decoder->concealHead = decoder->concealHead->next;
      if( decoder->concealTail == ptr )
        decoder->concealTail = decoder->concealTail->next;
      free(ptr);
      }
    }
  //}}}
  //{{{
  void updateRefListForConceal (cDpb* dpb) {

    cDecoder264* decoder = dpb->decoder;

    uint32_t j = 0;
    for (uint32_t i = 0; i < dpb->usedSize; i++)
      if (dpb->frameStore[i]->concealRef)
        dpb->frameStoreRef[j++] = dpb->frameStore[i];

    dpb->refFramesInBuffer = decoder->activePps->numRefIndexL0defaultActiveMinus1;
    }
  //}}}
 }

// intraFrame
//{{{
int ercConcealIntraFrame (cDecoder264* decoder, frame *recfr,
                          int picSizeX, int picSizeY, sErcVariables *errorVar )
{
  int lastColumn = 0, lastRow = 0;

  // if conceal is on
  if ( errorVar && errorVar->conceal ) {
    // if there are segments to be concealed
    if ( errorVar->numCorruptedSegments ) {
      // Y
      lastRow = (int) (picSizeY>>3);
      lastColumn = (int) (picSizeX>>3);
      concealBlocks (decoder, lastColumn, lastRow, 0, recfr, picSizeX, errorVar->yCondition );

      // U (dimensions halved compared to Y)
      lastRow = (int) (picSizeY>>4);
      lastColumn = (int) (picSizeX>>4);
      concealBlocks (decoder, lastColumn, lastRow, 1, recfr, picSizeX, errorVar->uCondition );

      // V ( dimensions equal to U )
      concealBlocks (decoder, lastColumn, lastRow, 2, recfr, picSizeX, errorVar->vCondition );
      }
    return 1;
    }
  else
    return 0;
  }
//}}}

 // interFrame
//{{{
int ercConcealInterFrame (frame *recfr, sObjectBuffer *object_list,
                         int picSizeX, int picSizeY, sErcVariables *errorVar, int chromaFormatIdc ) {

  cDecoder264* decoder = recfr->decoder;

  int lastColumn = 0, lastRow = 0, predBlocks[8];
  int lastCorruptedRow = -1, firstCorruptedRow = -1;
  int currRow = 0, row, column, columnInd, areaHeight = 0, i = 0;

  // if conceal is on
  if (errorVar && errorVar->conceal ) {
    /* if there are segments to be concealed */
    if (errorVar->numCorruptedSegments ) {
      sPixel* predMB;
      if (chromaFormatIdc != YUV400)
        predMB = (sPixel*)malloc ( (256 + (decoder->mbCrSize)*2) * sizeof (sPixel));
      else
        predMB = (sPixel*)malloc (256 * sizeof (sPixel));

      if (predMB == NULL )
        noMemoryExit ("ercConcealInterFrame: predMB");

      lastRow = (int) (picSizeY>>4);
      lastColumn = (int) (picSizeX>>4);
      for (columnInd = 0; columnInd < lastColumn; columnInd ++) {
        column = ((columnInd%2) ? (lastColumn - columnInd/2 -1) : (columnInd/2));
        for (row = 0; row < lastRow; row++) {
          if (errorVar->yCondition[MBxy2YBlock(column, row, 0, picSizeX)] <= ERC_BLOCK_CORRUPTED ) {                           // ERC_BLOCK_CORRUPTED (1) or ERC_BLOCK_EMPTY (0)
            firstCorruptedRow = row;
            /* find the last row which has corrupted blocks (in same continuous area) */
            for (lastCorruptedRow = row+1; lastCorruptedRow < lastRow; lastCorruptedRow++) {
              /* check blocks in the current column */
              if (errorVar->yCondition[MBxy2YBlock(column, lastCorruptedRow, 0, picSizeX)] > ERC_BLOCK_CORRUPTED) {
                /* current one is already OK, so the last was the previous one */
                lastCorruptedRow --;
                break;
                }
              }
            if (lastCorruptedRow >= lastRow ) {
              //{{{  correct only from above
              lastCorruptedRow = lastRow-1;
              for ( currRow = firstCorruptedRow; currRow < lastRow; currRow++ ) {
                ercCollect8PredBlocks (predBlocks, (currRow<<1), (column<<1),
                  errorVar->yCondition, (lastRow<<1), (lastColumn<<1), 2, 0);

                if(decoder->ercMvPerMb >= MVPERMB_THR)
                  concealByTrial(recfr, predMB,
                    currRow*lastColumn+column, object_list, predBlocks,
                    picSizeX, picSizeY,
                    errorVar->yCondition);
                else
                  concealByCopy(recfr, currRow*lastColumn+column,
                    object_list, picSizeX);

                ercMarkCurrMBConcealed (currRow*lastColumn+column, -1, picSizeX, errorVar);
                }
              row = lastRow;
              }
              //}}}
            else if (firstCorruptedRow == 0 ) {
              //{{{  correct only from below
              for ( currRow = lastCorruptedRow; currRow >= 0; currRow-- )
              {

                ercCollect8PredBlocks (predBlocks, (currRow<<1), (column<<1),
                  errorVar->yCondition, (lastRow<<1), (lastColumn<<1), 2, 0);

                if(decoder->ercMvPerMb >= MVPERMB_THR)
                  concealByTrial(recfr, predMB,
                    currRow*lastColumn+column, object_list, predBlocks,
                    picSizeX, picSizeY,
                    errorVar->yCondition);
                else
                  concealByCopy(recfr, currRow*lastColumn+column,
                    object_list, picSizeX);

                ercMarkCurrMBConcealed (currRow*lastColumn+column, -1, picSizeX, errorVar);
                }

              row = lastCorruptedRow+1;
              }
              //}}}
            else {
              //{{{  correct bi-directionally
              row = lastCorruptedRow+1;
              areaHeight = lastCorruptedRow-firstCorruptedRow+1;

              /*
              *  Conceal the corrupted area switching between the up and the bottom rows
              */
              for ( i = 0; i < areaHeight; i++)
              {
                if ( i % 2 )
                {
                  currRow = lastCorruptedRow;
                  lastCorruptedRow --;
                }
                else
                {
                  currRow = firstCorruptedRow;
                  firstCorruptedRow ++;
                }

                ercCollect8PredBlocks (predBlocks, (currRow<<1), (column<<1),
                  errorVar->yCondition, (lastRow<<1), (lastColumn<<1), 2, 0);

                if(decoder->ercMvPerMb >= MVPERMB_THR)
                  concealByTrial(recfr, predMB,
                    currRow*lastColumn+column, object_list, predBlocks,
                    picSizeX, picSizeY,
                    errorVar->yCondition);
                else
                  concealByCopy(recfr, currRow*lastColumn+column,
                    object_list, picSizeX);

                ercMarkCurrMBConcealed (currRow*lastColumn+column, -1, picSizeX, errorVar);

                }
              }
              //}}}
            lastCorruptedRow = -1;
            firstCorruptedRow = -1;
            }
          }
        }

      free (predMB);
      }
    return 1;
    }
  else
    return 0;
  }
//}}}

// conceal
//{{{
struct sConcealNode* initNode (sPicture* picture, int missingpoc ) {

  struct sConcealNode* ptr = (struct sConcealNode *) calloc( 1, sizeof(struct sConcealNode ) );
  if ( ptr == NULL )
    return (struct sConcealNode *) NULL;
  else {
    ptr->picture = picture;
    ptr->missingpocs = missingpoc;
    ptr->next = NULL;
    return ptr;
    }
  }
//}}}
//{{{
/*!
************************************************************************
* \brief
*    Initialize the list based on the B frame or non reference 'p' frame
*    to be concealed. The function initialize slice->listX[0] and list 1 depending
*    on current picture type
*
************************************************************************
*/
void initListsForNonRefLoss (cDpb* dpb, int currSliceType, ePicStructure currPicStructure)
{
  cDecoder264* decoder = dpb->decoder;
  cSps *activeSps = decoder->activeSps;

  uint32_t i;
  int j;
  int maxFrameNum = 1 << (activeSps->log2maxFrameNumMinus4 + 4);

  int list0idx = 0;
  int list0index1 = 0;
  sPicture* tmp_s;

  if (currPicStructure == eFrame) {
    for (i = 0; i < dpb->refFramesInBuffer; i++) {
      if (dpb->frameStore[i]->concealRef == 1) {
        if (dpb->frameStore[i]->frameNum > decoder->concealFrame)
          dpb->frameStoreRef[i]->frameNumWrap = dpb->frameStore[i]->frameNum - maxFrameNum;
        else
          dpb->frameStoreRef[i]->frameNumWrap = dpb->frameStore[i]->frameNum;
        dpb->frameStoreRef[i]->frame->picNum = dpb->frameStoreRef[i]->frameNumWrap;
        }
      }
    }

  if (currSliceType == eSliceP) {
    // Calculate FrameNumWrap and PicNum
    if (currPicStructure == eFrame) {
      for (i = 0; i < dpb->usedSize; i++)
        if (dpb->frameStore[i]->concealRef == 1)
          decoder->sliceList[0]->listX[0][list0idx++] = dpb->frameStore[i]->frame;
      // order list 0 by PicNum
      qsort ((void *)decoder->sliceList[0]->listX[0], list0idx, sizeof(sPicture*), comparePicByPicNumDescending);
      decoder->sliceList[0]->listXsize[0] = (char) list0idx;
      }
    }

  if (currSliceType == eSliceB) {
    if (currPicStructure == eFrame) {
      for (i = 0; i < dpb->usedSize; i++)
        if (dpb->frameStore[i]->concealRef == 1)
          if (decoder->earlierMissingPoc > dpb->frameStore[i]->frame->poc)
            decoder->sliceList[0]->listX[0][list0idx++] = dpb->frameStore[i]->frame;

      qsort ((void *)decoder->sliceList[0]->listX[0], list0idx, sizeof(sPicture*), comparePicByPocdesc);
      list0index1 = list0idx;
      for (i = 0; i < dpb->usedSize; i++)
        if (dpb->frameStore[i]->concealRef == 1)
          if (decoder->earlierMissingPoc < dpb->frameStore[i]->frame->poc)
            decoder->sliceList[0]->listX[0][list0idx++] = dpb->frameStore[i]->frame;

      qsort ((void*)&decoder->sliceList[0]->listX[0][list0index1], list0idx-list0index1,
             sizeof(sPicture*), comparePicByPocAscending);
      for (j = 0; j < list0index1; j++)
        decoder->sliceList[0]->listX[1][list0idx-list0index1+j] = decoder->sliceList[0]->listX[0][j];
      for (j = list0index1; j < list0idx; j++)
        decoder->sliceList[0]->listX[1][j-list0index1]=decoder->sliceList[0]->listX[0][j];

      decoder->sliceList[0]->listXsize[0] = decoder->sliceList[0]->listXsize[1] = (char) list0idx;

      qsort ((void*)&decoder->sliceList[0]->listX[0][(int16_t) decoder->sliceList[0]->listXsize[0]], list0idx-decoder->sliceList[0]->listXsize[0],
             sizeof(sPicture*), comparePicByLtPicNumAscending);
      qsort ((void*)&decoder->sliceList[0]->listX[1][(int16_t) decoder->sliceList[0]->listXsize[0]], list0idx-decoder->sliceList[0]->listXsize[0],
             sizeof(sPicture*), comparePicByLtPicNumAscending);
      decoder->sliceList[0]->listXsize[0] = decoder->sliceList[0]->listXsize[1] = (char) list0idx;
      }
    }

  if ((decoder->sliceList[0]->listXsize[0] == decoder->sliceList[0]->listXsize[1]) &&
      (decoder->sliceList[0]->listXsize[0] > 1)) {
    // check if lists are identical, if yes swap first two elements of listX[1]
    int diff = 0;
    for (j = 0; j < decoder->sliceList[0]->listXsize[0]; j++)
      if (decoder->sliceList[0]->listX[0][j]!=decoder->sliceList[0]->listX[1][j])
        diff = 1;
    if (!diff) {
      tmp_s = decoder->sliceList[0]->listX[1][0];
      decoder->sliceList[0]->listX[1][0]=decoder->sliceList[0]->listX[1][1];
      decoder->sliceList[0]->listX[1][1]=tmp_s;
      }
    }

  // set max size
  decoder->sliceList[0]->listXsize[0] = (char) imin (decoder->sliceList[0]->listXsize[0], (int)activeSps->numRefFrames);
  decoder->sliceList[0]->listXsize[1] = (char) imin (decoder->sliceList[0]->listXsize[1], (int)activeSps->numRefFrames);
  decoder->sliceList[0]->listXsize[1] = 0;

  // set the unused list entries to NULL
  for (i = decoder->sliceList[0]->listXsize[0]; i< (MAX_LIST_SIZE) ; i++)
    decoder->sliceList[0]->listX[0][i] = NULL;
  for (i = decoder->sliceList[0]->listXsize[1]; i< (MAX_LIST_SIZE) ; i++)
    decoder->sliceList[0]->listX[1][i] = NULL;
  }
//}}}
//{{{
/*!
************************************************************************
* \brief
* This function conceals a missing reference frame. The routine is called
* based on the difference in frame number. It conceals an IDR frame loss
* based on the sudden decrease in frame number.
************************************************************************
*/
void concealLostFrames (cDpb* dpb, cSlice *slice)
{
  cDecoder264* decoder = dpb->decoder;
  int CurrFrameNum;
  int UnusedShortTermFrameNum;
  sPicture *picture = NULL;
  int tmp1 = slice->deltaPicOrderCount[0];
  int tmp2 = slice->deltaPicOrderCount[1];
  int i;

  slice->deltaPicOrderCount[0] = slice->deltaPicOrderCount[1] = 0;

  // printf("A gap in frame number is found, try to fill it.\n");

  if(decoder->idrConcealFlag == 1)
  {
    // Conceals an IDR frame loss. Uses the reference frame in the previous
    // GOP for conceal.
    UnusedShortTermFrameNum = 0;
    decoder->lastRefPicPoc = -decoder->param.pocGap;
    decoder->earlierMissingPoc = 0;
  }
  else
    UnusedShortTermFrameNum = (decoder->preFrameNum + 1) % decoder->coding.maxFrameNum;

  CurrFrameNum = slice->frameNum;

  while (CurrFrameNum != UnusedShortTermFrameNum)
  {
    picture = allocPicture (decoder, eFrame, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);

    picture->codedFrame = 1;
    picture->picNum = UnusedShortTermFrameNum;
    picture->frameNum = UnusedShortTermFrameNum;
    picture->nonExisting = 0;
    picture->isOutput = 0;
    picture->usedForReference = 1;

    picture->adaptRefPicBufFlag = 0;

    slice->frameNum = UnusedShortTermFrameNum;

    picture->topPoc = decoder->lastRefPicPoc + decoder->param.refPocGap;
    picture->botPoc = picture->topPoc;
    picture->framePoc = picture->topPoc;
    picture->poc = picture->topPoc;
    decoder->lastRefPicPoc = picture->poc;

    copyPrevPicToConcealPic(picture, dpb);

    //if (UnusedShortTermFrameNum == 0)
    if(decoder->idrConcealFlag == 1)
    {
      picture->sliceType = eSliceI;
      picture->isIDR = true;
      dpb->flushDpb();
      picture->topPoc = 0;
      picture->botPoc =picture->topPoc;
      picture->framePoc = picture->topPoc;
      picture->poc = picture->topPoc;
      decoder->lastRefPicPoc = picture->poc;
    }

    decoder->dpb->storePictureDpb (picture);

    picture = NULL;

    decoder->preFrameNum = UnusedShortTermFrameNum;
    UnusedShortTermFrameNum = (UnusedShortTermFrameNum + 1) % decoder->coding.maxFrameNum;

    // update reference flags and set current flag.
    for(i=16;i>0;i--)
    {
      slice->refFlag[i] = slice->refFlag[i-1];
    }
    slice->refFlag[0] = 0;
  }
  slice->deltaPicOrderCount[0] = tmp1;
  slice->deltaPicOrderCount[1] = tmp2;
  slice->frameNum = CurrFrameNum;
}
//}}}
//{{{
/*!
************************************************************************
* \brief
* Stores the missing non reference frames in the conceal buffer. The
* detection is based on the POC difference in the sorted POC array. A missing
* non reference frame is detected when the dpb is full. A singly linked list
* is maintained for storing the missing non reference frames.
*
************************************************************************
*/

void concealNonRefPics (cDpb* dpb, int diff)
{
  cDecoder264* decoder = dpb->decoder;
  int missingpoc = 0;
  uint32_t i, pos = 0;
  sPicture *conceal_from_picture = NULL;
  sPicture *conceal_to_picture = NULL;
  struct sConcealNode *concealment_ptr = NULL;
  int temp_used_size = dpb->usedSize;

  if(dpb->usedSize == 0 )
    return;

  qsort(decoder->dpbPoc, dpb->size, sizeof(int), comp);

  for(i=0;i<dpb->size-diff;i++)
  {
    dpb->usedSize = dpb->size;
    if((decoder->dpbPoc[i+1] - decoder->dpbPoc[i]) > decoder->param.pocGap)
    {
      conceal_to_picture = allocPicture(decoder, eFrame, decoder->coding.width, decoder->coding.height, decoder->widthCr, decoder->heightCr, 1);

      missingpoc = decoder->dpbPoc[i] + decoder->param.pocGap;
      // Diagnostics
      // printf("\n missingpoc = %d\n",missingpoc);

      if(missingpoc > decoder->earlierMissingPoc)
      {
        decoder->earlierMissingPoc  = missingpoc;
        conceal_to_picture->topPoc = missingpoc;
        conceal_to_picture->botPoc = missingpoc;
        conceal_to_picture->framePoc = missingpoc;
        conceal_to_picture->poc = missingpoc;
        conceal_from_picture = getPicFromDpb(dpb, missingpoc, &pos);

        dpb->usedSize = pos + 1;

        decoder->concealFrame = conceal_from_picture->frameNum + 1;

        updateRefListForConceal (dpb);
        decoder->concealSliceType = eSliceB;
        copyToConceal (conceal_from_picture, conceal_to_picture, decoder);
        concealment_ptr = initNode (conceal_to_picture, missingpoc );
        addNode (decoder, concealment_ptr);
      }
    }
  }

  //restore the original value
  dpb->usedSize = temp_used_size;
}
//}}}
//{{{
/*!
************************************************************************
* \brief
* Perform Sliding window decoded reference picture marking process. It
* maintains the POC s stored in the dpb at a specific instance.
*
************************************************************************
*/

void slidingWindowPocManagement (cDpb* dpb, sPicture *p)
{
  if (dpb->usedSize == dpb->size)
  {
    cDecoder264* decoder = dpb->decoder;
    uint32_t i;

    for(i=0;i<dpb->size-1; i++)
      decoder->dpbPoc[i] = decoder->dpbPoc[i+1];
  }
}

//}}}
//{{{
/*!
************************************************************************
* \brief
* Outputs the non reference frames. The POCs in the conceal buffer are
* sorted in ascending order and outputted when the lowest POC in the
* conceal buffer is lower than the lowest in the dpb-> The linked list
* entry corresponding to the outputted POC is immediately deleted.
*
************************************************************************
*/

void writeLostNonRefPic (cDpb* dpb, int poc) {

  cDecoder264* decoder = dpb->decoder;
  cFrameStore concealment_fs;
  if (poc > 0) {
    if ((poc - dpb->lastOutputPoc) > decoder->param.pocGap) {
      concealment_fs.frame = decoder->concealHead->picture;
      concealment_fs.isOutput = 0;
      concealment_fs.usedReference = 0;
      concealment_fs.isUsed = 3;

      decoder->writeStoredFrame (&concealment_fs);
      deleteNode (decoder, decoder->concealHead);
      }
    }
  }
//}}}
//{{{
/*!
************************************************************************
* \brief
* Conceals frame loss immediately after the IDR. This special case produces
* the same result for either frame copy or motion vector copy conceal.
*
************************************************************************
*/
void writeLostRefAfterIdr (cDpb* dpb, int pos) {

  cDecoder264* decoder = dpb->decoder;
  int temp = 1;
  if (decoder->lastOutFramestore->frame == NULL) {
    decoder->lastOutFramestore->frame = allocPicture (decoder, eFrame, decoder->coding.width, decoder->coding.height,
                                                 decoder->widthCr, decoder->heightCr, 1);
    decoder->lastOutFramestore->isUsed = 3;
    }

  if (decoder->concealMode == 2) {
    temp = 2;
    decoder->concealMode = 1;
    }
  copyToConceal (dpb->frameStore[pos]->frame, decoder->lastOutFramestore->frame, decoder);

  decoder->concealMode = temp;
  }
//}}}

// api
//{{{
void ercInit (cDecoder264* decoder, int pic_sizex, int pic_sizey, int flag) {

  ercClose(decoder, decoder->ercErrorVar);
  decoder->ercObjectList = (sObjectBuffer*)calloc ((pic_sizex * pic_sizey) >> 6, sizeof(sObjectBuffer));
  if (decoder->ercObjectList == NULL)
    noMemoryExit ("ercInit: ercObjectList");

  // the error conceal instance is allocated
  decoder->ercErrorVar = ercOpen();

  // set error conceal ON
  ercSetErrorConcealment (decoder->ercErrorVar, flag);
}
//}}}
//{{{
sErcVariables* ercOpen() {

  sErcVariables *errorVar = NULL;
  errorVar = (sErcVariables*)malloc (sizeof(sErcVariables));
  if ( errorVar == NULL )
    noMemoryExit("ercOpen: errorVar");

  errorVar->nOfMBs = 0;
  errorVar->segments = NULL;
  errorVar->segment = 0;
  errorVar->yCondition = NULL;
  errorVar->uCondition = NULL;
  errorVar->vCondition = NULL;
  errorVar->prevFrameYCondition = NULL;

  errorVar->conceal = 1;

  return errorVar;
}
//}}}
//{{{
void ercReset (sErcVariables *errorVar, int nOfMBs, int numOfSegments, int picSizeX ) {

  char* tmp = NULL;

  if (errorVar && errorVar->conceal ) {
    sErcSegment* segments = NULL;
    if (nOfMBs != errorVar->nOfMBs && errorVar->yCondition != NULL) {
      //{{{  free conditions
      free ( errorVar->yCondition );
      errorVar->yCondition = NULL;

      free ( errorVar->prevFrameYCondition );
      errorVar->prevFrameYCondition = NULL;

      free ( errorVar->uCondition );
      errorVar->uCondition = NULL;

      free ( errorVar->vCondition );
      errorVar->vCondition = NULL;

      free ( errorVar->segments );
      errorVar->segments = NULL;
      }
      //}}}
    if (errorVar->yCondition == NULL) {
      //{{{  allocate conditions, first frame, or frame size is changed)
      errorVar->segments = (sErcSegment*)malloc (numOfSegments*sizeof(sErcSegment) );
      if ( errorVar->segments == NULL)
        noMemoryExit ("ercReset: errorVar->segments");
      memset (errorVar->segments, 0, numOfSegments*sizeof(sErcSegment));
      errorVar->nOfSegments = numOfSegments;

      errorVar->yCondition = (char*)malloc (4*nOfMBs*sizeof(char) );
      if (errorVar->yCondition == NULL )
        noMemoryExit ("ercReset: errorVar->yCondition");

      errorVar->prevFrameYCondition = (char*)malloc (4*nOfMBs*sizeof(char) );
      if (errorVar->prevFrameYCondition == NULL )
        noMemoryExit ("ercReset: errorVar->prevFrameYCondition");

      errorVar->uCondition = (char*)malloc (nOfMBs*sizeof(char) );
      if (errorVar->uCondition == NULL )
        noMemoryExit ("ercReset: errorVar->uCondition");

      errorVar->vCondition = (char*)malloc (nOfMBs*sizeof(char) );
      if (errorVar->vCondition == NULL )
        noMemoryExit ("ercReset: errorVar->vCondition");

      errorVar->nOfMBs = nOfMBs;
      }
      //}}}
    else {
      // Store the yCondition struct of the previous frame
      tmp = errorVar->prevFrameYCondition;
      errorVar->prevFrameYCondition = errorVar->yCondition;
      errorVar->yCondition = tmp;
      }

    // Reset tables and parameters
    memset (errorVar->yCondition, 0, 4*nOfMBs*sizeof(*errorVar->yCondition));
    memset (errorVar->uCondition, 0,   nOfMBs*sizeof(*errorVar->uCondition));
    memset (errorVar->vCondition, 0,   nOfMBs*sizeof(*errorVar->vCondition));

    if (errorVar->nOfSegments != numOfSegments) {
      free (errorVar->segments);
      errorVar->segments = NULL;
      errorVar->segments = (sErcSegment*)malloc (numOfSegments*sizeof(sErcSegment) );
      if (errorVar->segments == NULL)
        noMemoryExit("ercReset: errorVar->segments");
      errorVar->nOfSegments = numOfSegments;
      }

    //memset( errorVar->segments, 0, errorVar->nOfSegments * sizeof(sErcSegment));
    segments = errorVar->segments;
    for (int i = 0; i < errorVar->nOfSegments; i++) {
      segments->startMBPos = 0;
      segments->endMBPos = (int16_t) (nOfMBs - 1);
      (segments++)->corrupted = 1; //! mark segments as corrupted
      }

    errorVar->segment = 0;
    errorVar->numCorruptedSegments = 0;
    }
  }
//}}}
//{{{
void ercClose (cDecoder264* decoder,  sErcVariables *errorVar ) {

  if (errorVar) {
    if (errorVar->yCondition) {
      free (errorVar->segments);
      free (errorVar->yCondition);
      free (errorVar->uCondition);
      free (errorVar->vCondition);
      free (errorVar->prevFrameYCondition);
      }
    free (errorVar);
    errorVar = NULL;
    }

  if (decoder->ercObjectList) {
    free (decoder->ercObjectList);
    decoder->ercObjectList=NULL;
    }
  }
//}}}
//{{{
void ercSetErrorConcealment (sErcVariables *errorVar, int value ) {

  if ( errorVar != NULL )
    errorVar->conceal = value;
  }
//}}}

//{{{
/*!
** **********************************************************************
 * \brief
 *      Creates a new segment in the segment-list, and marks the start MB and bit position.
 *      If the end of the previous segment was not explicitly marked by "ercStopSegment",
 *      also marks the end of the previous segment.
 *      If needed, it reallocates the segment-list for a larger storage place.
 * \param currMBNum
 *      The MB number where the new slice/segment starts
 * \param segment
 *      Segment/cSlice No. counted by the caller
 * \param bitPos
 *      cBitStream pointer: number of bits read from the buffer.
 * \param errorVar
 *      Variables for error detector
** **********************************************************************
 */
void ercStartSegment (int currMBNum, int segment, uint32_t bitPos, sErcVariables *errorVar ) {

  if ( errorVar && errorVar->conceal ) {
    errorVar->segmentCorrupted = 0;
    errorVar->segments[ segment ].corrupted = 0;
    errorVar->segments[ segment ].startMBPos = (int16_t) currMBNum;
    }
  }
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *      Marks the end position of a segment.
 * \param currMBNum
 *      The last MB number of the previous segment
 * \param segment
 *      Segment/cSlice No. counted by the caller
 *      If (segment<0) the internal segment counter is used.
 * \param bitPos
 *      cBitStream pointer: number of bits read from the buffer.
 * \param errorVar
 *      Variables for error detector
** **********************************************************************
 */
void ercStopSegment (int currMBNum, int segment, uint32_t bitPos, sErcVariables *errorVar ) {

  if ( errorVar && errorVar->conceal ) {
    errorVar->segments[ segment ].endMBPos = (int16_t) currMBNum;
    errorVar->segment++;
    }
  }
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *      Marks the current segment (the one which has the "currMBNum" MB in it)
 *      as lost: all the blocks of the MBs in the segment as corrupted.
 * \param picSizeX
 *      Width of the frame in pixels.
 * \param errorVar
 *      Variables for error detector
** **********************************************************************
 */
void ercMarksegmentLost (int picSizeX, sErcVariables *errorVar )
{
  int j = 0;
  int current_segment;

  current_segment = errorVar->segment-1;
  if ( errorVar && errorVar->conceal ) {
    if (errorVar->segmentCorrupted == 0) {
      errorVar->numCorruptedSegments++;
      errorVar->segmentCorrupted = 1;
      }

    for ( j = errorVar->segments[current_segment].startMBPos;
         j <= errorVar->segments[current_segment].endMBPos; j++ ) {
      errorVar->yCondition[MBNum2YBlock (j, 0, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->yCondition[MBNum2YBlock (j, 1, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->yCondition[MBNum2YBlock (j, 2, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->yCondition[MBNum2YBlock (j, 3, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->uCondition[j] = ERC_BLOCK_CORRUPTED;
      errorVar->vCondition[j] = ERC_BLOCK_CORRUPTED;
      }
    errorVar->segments[current_segment].corrupted = 1;
    }
  }
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *      Marks the current segment (the one which has the "currMBNum" MB in it)
 *      as OK: all the blocks of the MBs in the segment as OK.
 * \param picSizeX
 *      Width of the frame in pixels.
 * \param errorVar
 *      Variables for error detector
** **********************************************************************
 */
void ercMarksegmentOK (int picSizeX, sErcVariables *errorVar ) {

  int j = 0;
  int current_segment;

  current_segment = errorVar->segment-1;
  if ( errorVar && errorVar->conceal ) {
    // mark all the Blocks belonging to the segment as OK */
    for (j = errorVar->segments[current_segment].startMBPos;
         j <= errorVar->segments[current_segment].endMBPos; j++ ) {
      errorVar->yCondition[MBNum2YBlock (j, 0, picSizeX)] = ERC_BLOCK_OK;
      errorVar->yCondition[MBNum2YBlock (j, 1, picSizeX)] = ERC_BLOCK_OK;
      errorVar->yCondition[MBNum2YBlock (j, 2, picSizeX)] = ERC_BLOCK_OK;
      errorVar->yCondition[MBNum2YBlock (j, 3, picSizeX)] = ERC_BLOCK_OK;
      errorVar->uCondition[j] = ERC_BLOCK_OK;
      errorVar->vCondition[j] = ERC_BLOCK_OK;
      }
    errorVar->segments[current_segment].corrupted = 0;
    }
  }
//}}}
//{{{
/*!
** **********************************************************************
 * \brief
 *      Marks the Blocks of the given component (YUV) of the current MB as concealed.
 * \param currMBNum
 *      Selects the segment where this MB number is in.
 * \param comp
 *      Component to mark (0:Y, 1:U, 2:V, <0:All)
 * \param picSizeX
 *      Width of the frame in pixels.
 * \param errorVar
 *      Variables for error detector
** **********************************************************************
 */
void ercMarkCurrMBConcealed (int currMBNum, int comp, int picSizeX, sErcVariables *errorVar ) {

  int setAll = 0;

  if ( errorVar && errorVar->conceal ) {
    if (comp < 0) {
      setAll = 1;
      comp = 0;
      }

    switch (comp) {
      case 0:
        errorVar->yCondition[MBNum2YBlock (currMBNum, 0, picSizeX)] = ERC_BLOCK_CONCEALED;
        errorVar->yCondition[MBNum2YBlock (currMBNum, 1, picSizeX)] = ERC_BLOCK_CONCEALED;
        errorVar->yCondition[MBNum2YBlock (currMBNum, 2, picSizeX)] = ERC_BLOCK_CONCEALED;
        errorVar->yCondition[MBNum2YBlock (currMBNum, 3, picSizeX)] = ERC_BLOCK_CONCEALED;
        if (!setAll)
          break;
      case 1:
        errorVar->uCondition[currMBNum] = ERC_BLOCK_CONCEALED;
        if (!setAll)
          break;
      case 2:
        errorVar->vCondition[currMBNum] = ERC_BLOCK_CONCEALED;
      }
    }
  }
//}}}
