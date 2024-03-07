#pragma once
#include "mbuffer.h"
#include "output.h"

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

//{{{
//! YUV pixel domain image arrays for a video frame
typedef struct frame_s {
  sVidParam* vidParam;
  sPixel *yptr;
  sPixel *uptr;
  sPixel *vptr;
  } frame;
//}}}
//{{{
//! region structure stores information about a region that is needed for conceal
typedef struct ObjectBuffer {
  byte regionMode;  //!< region mode as above
  int xMin;         //!< X coordinate of the pixel position of the top-left corner of the region
  int yMin;         //!< Y coordinate of the pixel position of the top-left corner of the region
  int mv[3];        //!< motion vectors in 1/4 pixel units: mvx = mv[0], mvy = mv[1],
                    //!< and ref_frame = mv[2]
  } sObjectBuffer;
//}}}
//{{{  sErcSegment
typedef struct ErcSegment {
  short startMBPos;
  short endMBPos;
  char  corrupted;
} sErcSegment;
//}}}
//{{{
/* Error detector & conceal instance data structure */
typedef struct ErcVariables {
  /*  Number of macroblocks (size or size/4 of the arrays) */
  int   nOfMBs;
  /* Number of segments (slices) in frame */
  int     nOfSegments;

  /*  Array for conditions of Y blocks */
  char* yCondition;
  /*  Array for conditions of U blocks */
  char* uCondition;
  /*  Array for conditions of V blocks */
  char* vCondition;

  /* Array for sSlice level information */
  sErcSegment* segments;
  int currSegment;

  /* Conditions of the MBs of the previous frame */
  char* prevFrameYCondition;

  /* Flag telling if the current segment was found to be corrupted */
  int currSegmentCorrupted;
  /* Counter for corrupted segments per picture */
  int numCorruptedSegments;

  /* State variables for error detector and concealer */
  int conceal;
  } sErcVariables;
//}}}
//{{{
struct ConcealNode {
  sPicture* picture;
  int       missingpocs;
  struct ConcealNode* next;
  };
//}}}
#define xPosMB(currMBNum,picSizeX) ((currMBNum)%((picSizeX)>>4))
#define yPosMB(currMBNum,picSizeX) ((currMBNum)/((picSizeX)>>4))

int ercConcealIntraFrame (sVidParam* vidParam, frame *recfr,
                          int picSizeX, int picSizeY, sErcVariables *errorVar );
int ercConcealInterFrame (frame *recfr, sObjectBuffer *object_list,
                          int picSizeX, int picSizeY, sErcVariables *errorVar, int chromaFormatIdc );

extern struct ConcealNode* init_node (sPicture* , int );
extern void init_lists_for_non_reference_loss (sDPB* dpb, int , sPictureStructure );
extern void conceal_lost_frames (sDPB* dpb, sSlice *pSlice);
extern void conceal_non_ref_pics (sDPB* dpb, int diff);
extern void sliding_window_poc_management (sDPB* dpb, sPicture *p);
extern void write_lost_non_ref_pic (sDPB* dpb, int poc);
extern void write_lost_ref_after_idr (sDPB* dpb, int pos);

void ercInit (sVidParam* vidParam, int pic_sizex, int pic_sizey, int flag);
sErcVariables* ercOpen();
void ercReset (sErcVariables *errorVar, int nOfMBs, int numOfSegments, int picSizeX );
void ercClose (sVidParam* vidParam, sErcVariables *errorVar );
void ercSetErrorConcealment (sErcVariables *errorVar, int value );

void ercStartSegment (int currMBNum, int segment, unsigned int bitPos, sErcVariables *errorVar );
void ercStopSegment (int currMBNum, int segment, unsigned int bitPos, sErcVariables *errorVar );
void ercMarkCurrSegmentLost (int picSizeX, sErcVariables *errorVar );
void ercMarkCurrSegmentOK (int picSizeX, sErcVariables *errorVar );
void ercMarkCurrMBConcealed (int currMBNum, int comp, int picSizeX, sErcVariables *errorVar );
