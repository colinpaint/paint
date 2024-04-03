//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
#include "mcPred.h"
#include "buffer.h"
#include "fmo.h"
#include "output.h"
#include "cabac.h"
#include "sps.h"
#include "pps.h"
#include "sei.h"
#include "erc.h"
#include "quant.h"
#include "block.h"
#include "nalu.h"
#include "loopfilter.h"
#include "output.h"
#include "h264decode.h"
//}}}

sDecoder* gDecoder;

//{{{
void error (const char* text) {

  fprintf (stderr, "--- Error -> %s\n", text);
  if (gDecoder)
    flushDpb (gDecoder->dpb);
  exit (0);
  }
//}}}
//{{{
static void resetDpb (sDecoder* decoder, sDpb* dpb) {

  dpb->decoder = decoder;
  dpb->initDone = 0;
  }
//}}}

//{{{
sSlice* allocSlice (sDecoder* decoder) {

  sSlice* slice = (sSlice*)calloc (1, sizeof(sSlice));
  if (!slice)
    error ("allocSlice failed");

  // create all context models
  slice->motionInfoContexts = createMotionInfoContexts();
  slice->textureInfoContexts = createTextureInfoContexts();

  slice->maxDataPartitions = 3;  // assume dataPartition worst case
  slice->dataPartitions = allocDataPartitions (slice->maxDataPartitions);

  getMem3Dint (&(slice->weightedPredWeight), 2, MAX_REFERENCE_PICTURES, 3);
  getMem3Dint (&(slice->weightedPredOffset), 6, MAX_REFERENCE_PICTURES, 3);
  getMem4Dint (&(slice->weightedBiPredWeight), 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);
  getMem3Dpel (&(slice->mbPred), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dpel (&(slice->mbRec), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&(slice->mbRess), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&(slice->cof), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  allocPred (slice);

  // reference flag initialization
  for (int i = 0; i < 17; ++i)
    slice->refFlag[i] = 1;

  for (int i = 0; i < 6; i++) {
    slice->listX[i] = (sPicture**)calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering
    if (!slice->listX[i])
      noMemoryExit ("allocSlice - slice->listX[i]");
    }

  for (int j = 0; j < 6; j++) {
    for (int i = 0; i < MAX_LIST_SIZE; i++)
      slice->listX[j][i] = NULL;
    slice->listXsize[j]=0;
    }

  return slice;
  }
//}}}
//{{{
static void freeSlice (sSlice *slice) {

  if (slice->sliceType != eSliceI && slice->sliceType != eSliceSI)
    freeRefPicListReorderBuffer (slice);

  freePred (slice);
  freeMem3Dint (slice->cof);
  freeMem3Dint (slice->mbRess);
  freeMem3Dpel (slice->mbRec);
  freeMem3Dpel (slice->mbPred);

  freeMem3Dint (slice->weightedPredWeight);
  freeMem3Dint (slice->weightedPredOffset);
  freeMem4Dint (slice->weightedBiPredWeight);

  freeDataPartitions (slice->dataPartitions, 3);

  // delete all context models
  deleteMotionInfoContexts (slice->motionInfoContexts);
  deleteTextureInfoContexts (slice->textureInfoContexts);

  for (int i = 0; i<6; i++) {
    if (slice->listX[i]) {
      free (slice->listX[i]);
      slice->listX[i] = NULL;
      }
    }

  while (slice->decRefPicMarkBuffer) {
    sDecodedRefPicMark* tempDrpm = slice->decRefPicMarkBuffer;
    slice->decRefPicMarkBuffer = tempDrpm->next;
    free (tempDrpm);
    }

  free (slice);
  slice = NULL;
  }
//}}}

//{{{
static void freeDecoder (sDecoder* decoder) {

  freeAnnexB (&decoder->annexB);

  free (decoder->dpb);
  decoder->dpb = NULL;

  free (decoder);
  decoder = NULL;

  free (decoder->oldSlice);
  decoder->oldSlice = NULL;

  freeSlice (decoder->nextSlice);
  decoder->nextSlice = NULL;

  if (decoder->sliceList) {
    for (int i = 0; i < decoder->numAllocatedSlices; i++)
      if (decoder->sliceList[i])
        freeSlice (decoder->sliceList[i]);
    free (decoder->sliceList);
    }

  freeNALU (decoder->nalu);
  decoder->nalu = NULL;

  freeDecodedPictures (decoder->outDecodedPics);
  free (decoder);
  }
//}}}

//{{{
sDataPartition* allocDataPartitions (int n) {

  sDataPartition* dataPartitions = (sDataPartition*)calloc (n, sizeof(sDataPartition));
  if (!dataPartitions)
    error ("allocDataPartitions: Memory allocation for Data dataPartition failed");

  for (int i = 0; i < n; ++i) {
    // loop over all dataPartitions
    sDataPartition* dataPartition = &(dataPartitions[i]);
    dataPartition->stream = (sBitStream*)calloc(1, sizeof(sBitStream));
    if (dataPartition->stream == NULL)
      error ("allocDataPartitions: Memory allocation for sBitStream failed");

    dataPartition->stream->bitStreamBuffer = (uint8_t*)calloc(MAX_CODED_FRAME_SIZE, sizeof(uint8_t));
    if (dataPartition->stream->bitStreamBuffer == NULL)
      error ("allocDataPartitions: Memory allocation for bitStreamBuffer failed");
    }

  return dataPartitions;
  }
//}}}
//{{{
void freeDataPartitions (sDataPartition* dataPartitions, int n) {

  for (int i = 0; i < n; ++i) {
    free (dataPartitions[i].stream->bitStreamBuffer);
    free (dataPartitions[i].stream);
    }

  free (dataPartitions);
  }
//}}}

//{{{
void initGlobalBuffers (sDecoder* decoder) {

  // alloc coding
  if (decoder->coding.isSeperateColourPlane) {
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!decoder->mbDataJV[i])
        decoder->mbDataJV[i] = (sMacroBlock*)calloc (decoder->coding.frameSizeMbs, sizeof(sMacroBlock));
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!decoder->intraBlockJV[i])
        decoder->intraBlockJV[i] = (char*)calloc (decoder->coding.frameSizeMbs, sizeof(char));
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!decoder->predModeJV[i])
       getMem2D (&(decoder->predModeJV[i]), 4*decoder->coding.frameHeightMbs, 4*decoder->coding.picWidthMbs);
    for (uint32_t i = 0; i < MAX_PLANE; ++i)
      if (!decoder->siBlockJV[i])
        getMem2Dint (&(decoder->siBlockJV[i]), decoder->coding.frameHeightMbs, decoder->coding.picWidthMbs);
    }
  else {
    if (!decoder->mbData)
      decoder->mbData = (sMacroBlock*)calloc (decoder->coding.frameSizeMbs, sizeof(sMacroBlock));
    if (!decoder->intraBlock)
      decoder->intraBlock = (char*)calloc (decoder->coding.frameSizeMbs, sizeof(char));

    if (!decoder->predMode)
      getMem2D (&(decoder->predMode), 4*decoder->coding.frameHeightMbs, 4*decoder->coding.picWidthMbs);
    if (!decoder->siBlock)
      getMem2Dint (&(decoder->siBlock), decoder->coding.frameHeightMbs, decoder->coding.picWidthMbs);
    }

  // alloc picPos
  if (!decoder->picPos) {
    decoder->picPos = (sBlockPos*)calloc (decoder->coding.frameSizeMbs + 1, sizeof(sBlockPos));
    sBlockPos* blockPos = decoder->picPos;
    for (uint32_t i = 0; i < decoder->coding.frameSizeMbs+1; ++i) {
      blockPos[i].x = (short)(i % decoder->coding.picWidthMbs);
      blockPos[i].y = (short)(i / decoder->coding.picWidthMbs);
      }
    }

  // alloc cavlc
  if (!decoder->nzCoeff)
    getMem4D (&(decoder->nzCoeff), decoder->coding.frameSizeMbs, 3, BLOCK_SIZE, BLOCK_SIZE);

  // alloc quant
  allocQuant (decoder);
  }
//}}}
//{{{
void freeGlobalBuffers (sDecoder* decoder) {

  freePicture (decoder->picture);
  decoder->picture = NULL;
  }
//}}}
//{{{
void freeLayerBuffers (sDecoder* decoder) {

  // free coding
  if (decoder->coding.isSeperateColourPlane) {
    for (int i = 0; i < MAX_PLANE; i++) {
      free (decoder->mbDataJV[i]);
      decoder->mbDataJV[i] = NULL;

      freeMem2Dint (decoder->siBlockJV[i]);
      decoder->siBlockJV[i] = NULL;

      freeMem2D (decoder->predModeJV[i]);
      decoder->predModeJV[i] = NULL;

      free (decoder->intraBlockJV[i]);
      decoder->intraBlockJV[i] = NULL;
      }
    }
  else {
    free (decoder->mbData);
    decoder->mbData = NULL;

    freeMem2Dint (decoder->siBlock);
    decoder->siBlock = NULL;

    freeMem2D (decoder->predMode);
    decoder->predMode = NULL;

    free (decoder->intraBlock);
    decoder->intraBlock = NULL;
    }

  // free picPos
  free (decoder->picPos);
  decoder->picPos = NULL;

  // free cavlc
  freeMem4D (decoder->nzCoeff);
  decoder->nzCoeff = NULL;

  // free quant
  freeQuant (decoder);
  }
//}}}

//{{{
sDecodedPic* allocDecodedPicture (sDecodedPic* decodedPic) {

  sDecodedPic* prevDecodedPicture = NULL;
  while (decodedPic && (decodedPic->ok)) {
    prevDecodedPicture = decodedPic;
    decodedPic = decodedPic->next;
    }

  if (!decodedPic) {
    decodedPic = (sDecodedPic*)calloc (1, sizeof(*decodedPic));
    prevDecodedPicture->next = decodedPic;
    }

  return decodedPic;
  }
//}}}
//{{{
void clearDecodedPics (sDecoder* decoder) {

  // find the head first;
  sDecodedPic* prevDecodedPicture = NULL;
  sDecodedPic* decodedPic = decoder->outDecodedPics;
  while (decodedPic && !decodedPic->ok) {
    prevDecodedPicture = decodedPic;
    decodedPic = decodedPic->next;
    }

  if (decodedPic && (decodedPic != decoder->outDecodedPics)) {
    // move all nodes before decodedPic to the end;
    sDecodedPic* decodedPictureTail = decodedPic;
    while (decodedPictureTail->next)
      decodedPictureTail = decodedPictureTail->next;

    decodedPictureTail->next = decoder->outDecodedPics;
    decoder->outDecodedPics = decodedPic;
    prevDecodedPicture->next = NULL;
    }
  }
//}}}
//{{{
void freeDecodedPictures (sDecodedPic* decodedPic) {

  while (decodedPic) {
    sDecodedPic* nextDecodedPicture = decodedPic->next;
    if (decodedPic->yBuf) {
      free (decodedPic->yBuf);
      decodedPic->yBuf = NULL;
      decodedPic->uBuf = NULL;
      decodedPic->vBuf = NULL;
      }

    free (decodedPic);
    decodedPic = nextDecodedPicture;
   }
  }
//}}}

//{{{
sDecoder* openDecoder (sParam* param, uint8_t* chunk, size_t chunkSize) {

  // alloc decoder
  sDecoder* decoder = (sDecoder*)calloc (1, sizeof(sDecoder));
  gDecoder = decoder;

  initTime();

  // init param
  memcpy (&(decoder->param), param, sizeof(sParam));
  decoder->concealMode = param->concealMode;

  // init nalu, annexB
  decoder->nalu = allocNALU (MAX_CODED_FRAME_SIZE);
  decoder->annexB = allocAnnexB (decoder);
  openAnnexB (decoder->annexB, chunk, chunkSize);

  // init slice
  decoder->sliceList = (sSlice**)calloc (MAX_NUM_DECSLICES, sizeof(sSlice*));
  decoder->numAllocatedSlices = MAX_NUM_DECSLICES;
  decoder->oldSlice = (sOldSlice*)calloc(1, sizeof(sOldSlice));
  initOldSlice (decoder->oldSlice);
  decoder->coding.sliceType = eSliceI;
  decoder->recoveryPoc = 0x7fffffff;
  decoder->deblockEnable = 0x3;
  decoder->pendingOutState = eFrame;

  decoder->coding.lumaPadX = MCBUF_LUMA_PAD_X;
  decoder->coding.lumaPadY = MCBUF_LUMA_PAD_Y;
  decoder->coding.chromaPadX = MCBUF_CHROMA_PAD_X;
  decoder->coding.chromaPadY = MCBUF_CHROMA_PAD_Y;

  decoder->dpb = (sDpb*)calloc (1, sizeof(sDpb));
  resetDpb (decoder, decoder->dpb);

  decoder->outDecodedPics = (sDecodedPic*)calloc (1, sizeof(sDecodedPic));
  allocOutput (decoder);

  return decoder;
  }
//}}}
//{{{
int decodeOneFrame (sDecoder* decoder, sDecodedPic** decPicList) {

  clearDecodedPics (decoder);

  int iRet = decodeFrame (decoder);
  if (iRet == eSOP)
    iRet = DEC_SUCCEED;
  else if (iRet == eEOS)
    iRet = DEC_EOS;
  else
    iRet |= DEC_ERRMASK;

  *decPicList = decoder->outDecodedPics;
  return iRet;
  }
//}}}
//{{{
void finishDecoder (sDecoder* decoder, sDecodedPic** decPicList) {

  clearDecodedPics (decoder);
  flushDpb (decoder->dpb);

  resetAnnexB (decoder->annexB);

  decoder->newFrame = 0;
  decoder->prevFrameNum = 0;
  *decPicList = decoder->outDecodedPics;
  }
//}}}
//{{{
void closeDecoder (sDecoder* decoder) {

  closeFmo (decoder);
  freeLayerBuffers (decoder);
  freeGlobalBuffers (decoder);

  ercClose (decoder, decoder->ercErrorVar);

  for (uint32_t i = 0; i < MAX_PPS; i++) {
    if (decoder->pps[i].ok && decoder->pps[i].sliceGroupId)
      free (decoder->pps[i].sliceGroupId);
    decoder->pps[i].ok = false;
    }

  freeDpb (decoder->dpb);
  freeOutput (decoder);
  freeDecoder (decoder);

  gDecoder = NULL;
  }
//}}}