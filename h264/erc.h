#pragma once

/* "block" means an 8x8 pixel area */
/* Region modes */
#define REGMODE_INTER_COPY       0  //!< Copy region
#define REGMODE_INTER_PRED       1  //!< Inter region with motion vectors
#define REGMODE_INTRA            2  //!< Intra region
#define REGMODE_SPLITTED         3  //!< Any region mode higher than this indicates that the region
                                    //!< is splitted which means 8x8 block
#define REGMODE_INTER_COPY_8x8   4
#define REGMODE_INTER_PRED_8x8   5
#define REGMODE_INTRA_8x8        6

//{{{  frame
typedef struct frame_s {
  cDecoder264* decoder;
  sPixel *yptr;
  sPixel *uptr;
  sPixel *vptr;
  } frame;
//}}}
//{{{  sObjectBuffer
typedef struct ObjectBuffer {
  uint8_t regionMode;  //!< region mode as above
  int xMin;         //!< X coordinate of the pixel position of the top-left corner of the region
  int yMin;         //!< Y coordinate of the pixel position of the top-left corner of the region
  int mv[3];        //!< motion vectors in 1/4 pixel units: mvx = mv[0], mvy = mv[1],
                    //!< and ref_frame = mv[2]
  } sObjectBuffer;
//}}}
//{{{  sErcSegment
typedef struct ErcSegment {
  int16_t startMBPos;
  int16_t endMBPos;
  char  corrupted;
} sErcSegment;
//}}}
//{{{  sErcVariables
/* Error detector & conceal instance data picStructure */
typedef struct ErcVariables {
  /*  Number of macroBlocks (size or size/4 of the arrays) */
  int   nOfMBs;
  /* Number of segments (slices) in frame */
  int     nOfSegments;

  /*  Array for conditions of Y blocks */
  char* yCondition;
  /*  Array for conditions of U blocks */
  char* uCondition;
  /*  Array for conditions of V blocks */
  char* vCondition;

  /* Array for cSlice level information */
  sErcSegment* segments;
  int segment;

  /* Conditions of the MBs of the previous frame */
  char* prevFrameYCondition;

  /* Flag telling if the current segment was found to be corrupted */
  int segmentCorrupted;
  /* Counter for corrupted segments per picture */
  int numCorruptedSegments;

  /* State variables for error detector and concealer */
  int conceal;
  } sErcVariables;
//}}}
#define xPosMB(currMBNum,picSizeX) ((currMBNum)%((picSizeX)>>4))
#define yPosMB(currMBNum,picSizeX) ((currMBNum)/((picSizeX)>>4))

int ercConcealIntraFrame (cDecoder264* decoder, frame* recfr,
                          int picSizeX, int picSizeY, sErcVariables *errorVar );
int ercConcealInterFrame (frame *recfr, sObjectBuffer* object_list,
                          int picSizeX, int picSizeY, sErcVariables *errorVar, int chromaFormatIdc );

struct sConcealNode* initNode (sPicture* , int);
void initListsForNonRefLoss (cDpb* dpb, int , ePicStructure );
void concealLostFrames (cDpb* dpb, cSlice *slice);
void concealNonRefPics (cDpb* dpb, int diff);
void slidingWindowPocManage (cDpb* dpb, sPicture* p);
void writeLostNonRefPic (cDpb* dpb, int poc);
void writeLostRefAfterIdr (cDpb* dpb, int pos);

void ercInit (cDecoder264* decoder, int pic_sizex, int pic_sizey, int flag);
sErcVariables* ercOpen();
void ercReset (sErcVariables *errorVar, int nOfMBs, int numOfSegments, int picSizeX );
void ercClose (cDecoder264* decoder, sErcVariables *errorVar );
void ercSetErrorConcealment (sErcVariables *errorVar, int value );

void ercStartSegment (int currMBNum, int segment, uint32_t bitPos, sErcVariables *errorVar );
void ercStopSegment (int currMBNum, int segment, uint32_t bitPos, sErcVariables *errorVar );
void ercMarksegmentLost (int picSizeX, sErcVariables *errorVar );
void ercMarksegmentOK (int picSizeX, sErcVariables *errorVar );
void ercMarkCurrMBConcealed (int currMBNum, int comp, int picSizeX, sErcVariables *errorVar );
