//{{{  includes
#include "global.h"
#include "memory.h"

#include "image.h"
#include "mcPred.h"
#include "buffer.h"
#include "fmo.h"
#include "output.h"
#include "cabac.h"
#include "parset.h"
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
void error (char* text) {

  fprintf (stderr, "--- Error -> %s\n", text);
  if (gDecoder)
    flushDpb (gDecoder->dpb);
  exit (0);
  }
//}}}
//{{{
static void resetDpb (sDecoder* decoder, sDPB* dpb ) {

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
  slice->motionContext = createContextsMotionInfo();
  slice->textureContext = createContextsTextureInfo();

  slice->maxDataPartitions = 3;  // assume dataPartition worst case
  slice->dps = allocDataPartitions (slice->maxDataPartitions);

  getMem2Dwp (&(slice->WPParam), 2, MAX_REFERENCE_PICTURES);
  getMem3Dint (&(slice->wpWeight), 2, MAX_REFERENCE_PICTURES, 3);
  getMem3Dint (&(slice->wpOffset), 6, MAX_REFERENCE_PICTURES, 3);
  getMem4Dint (&(slice->wbpWeight), 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);
  getMem3Dpel (&(slice->mbPred), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dpel (&(slice->mbRec), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&(slice->mbRess), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  getMem3Dint (&(slice->cof), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  allocPred (slice);

  // reference flag initialization
  for (int i = 0; i < 17; ++i)
    slice->refFlag[i] = 1;

  for (int i = 0; i < 6; i++) {
    slice->listX[i] = calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering
    if (!slice->listX[i])
      no_mem_exit ("allocSlice - slice->listX[i]");
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

  if (slice->sliceType != I_SLICE && slice->sliceType != SI_SLICE)
    freeRefPicListReorderingBuffer (slice);

  freePred (slice);
  free_mem3Dint (slice->cof);
  free_mem3Dint (slice->mbRess);
  free_mem3Dpel (slice->mbRec);
  free_mem3Dpel (slice->mbPred);

  free_mem2Dwp (slice->WPParam);
  free_mem3Dint (slice->wpWeight);
  free_mem3Dint (slice->wpOffset);
  free_mem4Dint (slice->wbpWeight);

  freeDataPartitions (slice->dps, 3);

  // delete all context models
  delete_contexts_MotionInfo (slice->motionContext);
  delete_contexts_TextureInfo (slice->textureContext);

  for (int i = 0; i<6; i++) {
    if (slice->listX[i]) {
      free (slice->listX[i]);
      slice->listX[i] = NULL;
      }
    }

  while (slice->decRefPicMarkingBuffer) {
    sDecodedRefPicMarking* tmp_drpm = slice->decRefPicMarkingBuffer;
    slice->decRefPicMarkingBuffer = tmp_drpm->next;
    free (tmp_drpm);
    }

  free (slice);
  slice = NULL;
  }
//}}}

//{{{
static void freeDecoder (sDecoder* decoder) {

  freeAnnexB (&decoder->annexB);

  // Free new dpb layers
    if (decoder->dpb != NULL) {
      free (decoder->dpb);
      decoder->dpb = NULL;
      }

    if (decoder->coding) {
      free (decoder->coding);
      decoder->coding = NULL;
      }

  if (decoder->oldSlice != NULL) {
    free (decoder->oldSlice);
    decoder->oldSlice = NULL;
    }

  if (decoder->nextSlice) {
    freeSlice (decoder->nextSlice);
    decoder->nextSlice = NULL;
    }

  if (decoder->sliceList) {
    for (int i = 0; i < decoder->numAllocatedSlices; i++)
      if (decoder->sliceList[i])
        freeSlice (decoder->sliceList[i]);
    free (decoder->sliceList);
    }

  if (decoder->nalu) {
    freeNALU (decoder->nalu);
    decoder->nalu = NULL;
    }

  //free memory;
  freeDecodedPictures (decoder->decOutputPic);
  if (decoder->nextPPS) {
    freePPS (decoder->nextPPS);
    decoder->nextPPS = NULL;
    }

  free (decoder);
  }
//}}}

//{{{
void initFrext (sDecoder* decoder) {

  // pel bitdepth init
  decoder->bitdepthLumeQpScale = 6 * (decoder->bitdepthLuma - 8);

  if(decoder->bitdepthLuma > decoder->bitdepthChroma || decoder->activeSPS->chromaFormatIdc == YUV400)
    decoder->picUnitBitSizeDisk = (decoder->bitdepthLuma > 8)? 16:8;
  else
    decoder->picUnitBitSizeDisk = (decoder->bitdepthChroma > 8)? 16:8;
  decoder->dcPredValueComp[0] = 1<<(decoder->bitdepthLuma - 1);
  decoder->maxPelValueComp[0] = (1<<decoder->bitdepthLuma) - 1;
  decoder->mbSize[0][0] = decoder->mbSize[0][1] = MB_BLOCK_SIZE;

  if (decoder->activeSPS->chromaFormatIdc != YUV400) {
    // for chrominance part
    decoder->bitdepthChromaQpScale = 6 * (decoder->bitdepthChroma - 8);
    decoder->dcPredValueComp[1] = (1 << (decoder->bitdepthChroma - 1));
    decoder->dcPredValueComp[2] = decoder->dcPredValueComp[1];
    decoder->maxPelValueComp[1] = (1 << decoder->bitdepthChroma) - 1;
    decoder->maxPelValueComp[2] = (1 << decoder->bitdepthChroma) - 1;
    decoder->numBlock8x8uv = (1 << decoder->activeSPS->chromaFormatIdc) & (~(0x1));
    decoder->numUvBlocks = (decoder->numBlock8x8uv >> 1);
    decoder->numCdcCoeff = (decoder->numBlock8x8uv << 1);
    decoder->mbSize[1][0] = decoder->mbSize[2][0] = decoder->mbCrSizeX  = (decoder->activeSPS->chromaFormatIdc==YUV420 || decoder->activeSPS->chromaFormatIdc==YUV422)?  8 : 16;
    decoder->mbSize[1][1] = decoder->mbSize[2][1] = decoder->mbCrSizeY  = (decoder->activeSPS->chromaFormatIdc==YUV444 || decoder->activeSPS->chromaFormatIdc==YUV422)? 16 :  8;

    decoder->subpelX = decoder->mbCrSizeX == 8 ? 7 : 3;
    decoder->subpelY = decoder->mbCrSizeY == 8 ? 7 : 3;
    decoder->shiftpelX = decoder->mbCrSizeX == 8 ? 3 : 2;
    decoder->shiftpelY = decoder->mbCrSizeY == 8 ? 3 : 2;
    decoder->totalScale = decoder->shiftpelX + decoder->shiftpelY;
    }
  else {
    decoder->bitdepthChromaQpScale = 0;
    decoder->maxPelValueComp[1] = 0;
    decoder->maxPelValueComp[2] = 0;
    decoder->numBlock8x8uv = 0;
    decoder->numUvBlocks = 0;
    decoder->numCdcCoeff = 0;
    decoder->mbSize[1][0] = decoder->mbSize[2][0] = decoder->mbCrSizeX  = 0;
    decoder->mbSize[1][1] = decoder->mbSize[2][1] = decoder->mbCrSizeY  = 0;
    decoder->subpelX = 0;
    decoder->subpelY = 0;
    decoder->shiftpelX = 0;
    decoder->shiftpelY = 0;
    decoder->totalScale = 0;
    }

  decoder->mbCrSize = decoder->mbCrSizeX * decoder->mbCrSizeY;
  decoder->mbSizeBlock[0][0] = decoder->mbSizeBlock[0][1] = decoder->mbSize[0][0] >> 2;
  decoder->mbSizeBlock[1][0] = decoder->mbSizeBlock[2][0] = decoder->mbSize[1][0] >> 2;
  decoder->mbSizeBlock[1][1] = decoder->mbSizeBlock[2][1] = decoder->mbSize[1][1] >> 2;

  decoder->mbSizeShift[0][0] = decoder->mbSizeShift[0][1] = ceilLog2sf (decoder->mbSize[0][0]);
  decoder->mbSizeShift[1][0] = decoder->mbSizeShift[2][0] = ceilLog2sf (decoder->mbSize[1][0]);
  decoder->mbSizeShift[1][1] = decoder->mbSizeShift[2][1] = ceilLog2sf (decoder->mbSize[1][1]);
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
    dataPartition->s = (sBitStream*)calloc(1, sizeof(sBitStream));
    if (dataPartition->s == NULL)
      error ("allocDataPartitions: Memory allocation for sBitStream failed");

    dataPartition->s->bitStreamBuffer = (byte*)calloc(MAX_CODED_FRAME_SIZE, sizeof(byte));
    if (dataPartition->s->bitStreamBuffer == NULL)
      error ("allocDataPartitions: Memory allocation for bitStreamBuffer failed");
    }

  return dataPartitions;
  }
//}}}
//{{{
void freeDataPartitions (sDataPartition* dataPartitions, int n) {

  assert (dataPartitions != NULL);
  assert (dataPartitions->s != NULL);
  assert (dataPartitions->s->bitStreamBuffer != NULL);

  for (int i = 0; i < n; ++i) {
    free (dataPartitions[i].s->bitStreamBuffer);
    free (dataPartitions[i].s);
    }

  free (dataPartitions);
  }
//}}}

//{{{
void freeLayerBuffers (sDecoder* decoder) {

  sCoding* coding = decoder->coding;

  if (!decoder->globalInitDone)
    return;

  // CAVLC free mem
  if (coding->nzCoeff) {
    free_mem4D(coding->nzCoeff);
    coding->nzCoeff = NULL;
    }

  // free mem, allocated for structure decoder
  if ((coding->sepColourPlaneFlag != 0) ) {
    for (int i = 0; i<MAX_PLANE; i++) {
      free (coding->mbDataJV[i]);
      coding->mbDataJV[i] = NULL;
      free_mem2Dint(coding->siBlockJV[i]);
      coding->siBlockJV[i] = NULL;
      free_mem2D(coding->predModeJV[i]);
      coding->predModeJV[i] = NULL;
      free (coding->intraBlockJV[i]);
      coding->intraBlockJV[i] = NULL;
      }
    }
  else {
    if (coding->mbData != NULL) {
      free(coding->mbData);
      coding->mbData = NULL;
      }
    if (coding->siBlock) {
      free_mem2Dint(coding->siBlock);
      coding->siBlock = NULL;
      }
    if (coding->predMode) {
      free_mem2D(coding->predMode);
      coding->predMode = NULL;
      }
    if (coding->intraBlock) {
      free (coding->intraBlock);
      coding->intraBlock = NULL;
      }
    }

  if (coding->picPos) {
    free(coding->picPos);
    coding->picPos = NULL;
    }

  freeQuant (coding);

  decoder->globalInitDone = 0;
  }
//}}}
//{{{
void initGlobalBuffers (sDecoder* decoder) {

  sCoding *coding = decoder->coding;
  sBlockPos* picPos;

  if (decoder->globalInitDone)
    freeLayerBuffers (decoder);

  // allocate memory in structure decoder
  if (coding->sepColourPlaneFlag != 0) {
    for (int i = 0; i < MAX_PLANE; ++i )
      if (((coding->mbDataJV[i]) = (sMacroblock*)calloc(coding->frameSizeMbs, sizeof(sMacroblock))) == NULL)
        no_mem_exit ("initGlobalBuffers: coding->mbDataJV");
    coding->mbData = NULL;
    }
  else if (((coding->mbData) = (sMacroblock*)calloc (coding->frameSizeMbs, sizeof(sMacroblock))) == NULL)
    no_mem_exit ("initGlobalBuffers: coding->mbData");

  if (coding->sepColourPlaneFlag != 0) {
    for (int i = 0; i < MAX_PLANE; ++i )
      if (((coding->intraBlockJV[i]) = (char*) calloc(coding->frameSizeMbs, sizeof(char))) == NULL)
        no_mem_exit ("initGlobalBuffers: coding->intraBlockJV");
    coding->intraBlock = NULL;
    }
  else if (((coding->intraBlock) = (char*)calloc (coding->frameSizeMbs, sizeof(char))) == NULL)
    no_mem_exit ("initGlobalBuffers: coding->intraBlock");

  if (((coding->picPos) = (sBlockPos*)calloc(coding->frameSizeMbs + 1, sizeof(sBlockPos))) == NULL)
    no_mem_exit ("initGlobalBuffers: picPos");

  picPos = coding->picPos;
  for (int i = 0; i < (int) coding->frameSizeMbs + 1;++i) {
    picPos[i].x = (short)(i % coding->picWidthMbs);
    picPos[i].y = (short)(i / coding->picWidthMbs);
    }

  if( (coding->sepColourPlaneFlag != 0)) {
    for (int i = 0; i < MAX_PLANE; ++i )
      getMem2D (&(coding->predModeJV[i]), 4*coding->frameHeightMbs, 4*coding->picWidthMbs);
    coding->predMode = NULL;
    }
  else
    getMem2D (&(coding->predMode), 4*coding->frameHeightMbs, 4*coding->picWidthMbs);

  // CAVLC mem
  getMem4D (&(coding->nzCoeff), coding->frameSizeMbs, 3, BLOCK_SIZE, BLOCK_SIZE);
  if (coding->sepColourPlaneFlag != 0) {
    for (int i = 0; i < MAX_PLANE; ++i ) {
      getMem2Dint (&(coding->siBlockJV[i]), coding->frameHeightMbs, coding->picWidthMbs);
      if (coding->siBlockJV[i] == NULL)
        no_mem_exit ("initGlobalBuffers: decoder->siBlockJV");
      }
    coding->siBlock = NULL;
    }
  else
    getMem2Dint (&(coding->siBlock), coding->frameHeightMbs, coding->picWidthMbs);

  allocQuant (coding);
  coding->oldFrameSizeMbs = coding->frameSizeMbs;
  decoder->globalInitDone = 1;
  }
//}}}
//{{{
void freeGlobalBuffers (sDecoder* decoder) {

  if (decoder->picture) {
    freePicture (decoder->picture);
    decoder->picture = NULL;
    }
  }
//}}}

//{{{
sDecodedPic* allocDecodedPicture (sDecodedPic* decodedPic) {

  sDecodedPic* prevDecodedPicture = NULL;
  while (decodedPic && (decodedPic->valid)) {
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
  sDecodedPic* decodedPic = decoder->decOutputPic;
  while (decodedPic && !decodedPic->valid) {
    prevDecodedPicture = decodedPic;
    decodedPic = decodedPic->next;
    }

  if (decodedPic && (decodedPic != decoder->decOutputPic)) {
    // move all nodes before decodedPic to the end;
    sDecodedPic* decodedPictureTail = decodedPic;
    while (decodedPictureTail->next)
      decodedPictureTail = decodedPictureTail->next;

    decodedPictureTail->next = decoder->decOutputPic;
    decoder->decOutputPic = decodedPic;
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
void setGlobalCodingProgram (sDecoder* decoder, sCoding* coding) {

  decoder->bitdepthChroma = 0;
  decoder->widthCr = 0;
  decoder->heightCr = 0;
  decoder->losslessQpPrimeFlag = coding->losslessQpPrimeFlag;
  decoder->maxVmvR = coding->maxVmvR;

  // Fidelity Range Extensions stuff (part 1)
  decoder->bitdepthLuma = coding->bitdepthLuma;
  decoder->bitdepthScale[0] = coding->bitdepthScale[0];
  decoder->bitdepthChroma = coding->bitdepthChroma;
  decoder->bitdepthScale[1] = coding->bitdepthScale[1];

  decoder->maxFrameNum = coding->maxFrameNum;
  decoder->picWidthMbs = coding->picWidthMbs;
  decoder->picHeightMapUnits = coding->picHeightMapUnits;
  decoder->frameHeightMbs = coding->frameHeightMbs;
  decoder->frameSizeMbs = coding->frameSizeMbs;

  decoder->yuvFormat = coding->yuvFormat;
  decoder->sepColourPlaneFlag = coding->sepColourPlaneFlag;
  decoder->ChromaArrayType = coding->ChromaArrayType;

  decoder->width = coding->width;
  decoder->height = coding->height;
  decoder->iLumaPadX = MCBUF_LUMA_PAD_X;
  decoder->iLumaPadY = MCBUF_LUMA_PAD_Y;
  decoder->iChromaPadX = MCBUF_CHROMA_PAD_X;
  decoder->iChromaPadY = MCBUF_CHROMA_PAD_Y;

  if (decoder->yuvFormat == YUV420) {
    decoder->widthCr = (decoder->width  >> 1);
    decoder->heightCr = (decoder->height >> 1);
    }
  else if (decoder->yuvFormat == YUV422) {
    decoder->widthCr = (decoder->width >> 1);
    decoder->heightCr = decoder->height;
    decoder->iChromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (decoder->yuvFormat == YUV444) {
    //YUV444
    decoder->widthCr = decoder->width;
    decoder->heightCr = decoder->height;
    decoder->iChromaPadX = decoder->iLumaPadX;
    decoder->iChromaPadY = decoder->iLumaPadY;
    }

  initFrext (decoder);
  }
//}}}

//{{{
sDecoder* openDecoder (sParam* param, byte* chunk, size_t chunkSize) {

  // alloc decoder
  sDecoder* decoder = (sDecoder*)calloc (1, sizeof(sDecoder));
  gDecoder = decoder;

  init_time();

  // init param
  memcpy (&(decoder->param), param, sizeof(sParam));
  decoder->concealMode = param->concealMode;
  decoder->refPocGap = param->refPocGap;
  decoder->pocGap = param->pocGap;

  // init nalu, annexB
  decoder->nalu = allocNALU (MAX_CODED_FRAME_SIZE);
  decoder->nextPPS = allocPPS();
  decoder->gotSPS = 0;
  decoder->annexB = allocAnnexB (decoder);
  openAnnexB (decoder->annexB, chunk, chunkSize);
  decoder->gotLastNalu = 0;
  decoder->naluCount = 0;
  decoder->pendingNalu = NULL;

  // init slice
  decoder->globalInitDone = 0;
  decoder->oldSlice = (sOldSlice*)calloc (1, sizeof(sOldSlice));
  decoder->sliceList = (sSlice**)calloc (MAX_NUM_DECSLICES, sizeof(sSlice*));
  decoder->numAllocatedSlices = MAX_NUM_DECSLICES;
  decoder->nextSlice = NULL;
  initOldSlice (decoder->oldSlice);
  decoder->oldFrameSizeMbs = (unsigned int) -1;

  decoder->type = I_SLICE;
  decoder->picture = NULL;
  decoder->mbToSliceGroupMap = NULL;
  decoder->mapUnitToSliceGroupMap = NULL;

  decoder->recoveryFlag = 0;
  decoder->recoveryPoint = 0;
  decoder->recoveryPointFound = 0;
  decoder->recoveryPoc = 0x7fffffff; /* set to a max value */

  decoder->decodeFrameNum = 0;
  decoder->idrFrameNum = 0;
  decoder->newFrame = 0;
  decoder->prevFrameNum = 0;

  decoder->deblockEnable = 0x3;

  decoder->outBuffer = NULL;
  decoder->pendingOut = NULL;
  decoder->pendingOutState = FRAME;

  decoder->iLumaPadX = MCBUF_LUMA_PAD_X;
  decoder->iLumaPadY = MCBUF_LUMA_PAD_Y;
  decoder->iChromaPadX = MCBUF_CHROMA_PAD_X;
  decoder->iChromaPadY = MCBUF_CHROMA_PAD_Y;

  decoder->dpb = (sDPB*)calloc (1, sizeof(sDPB));
  resetDpb (decoder, decoder->dpb);

  decoder->coding = (sCoding*)calloc (1, sizeof(sCoding));
  decoder->layerInitDone = 0;

  decoder->decOutputPic = (sDecodedPic*)calloc (1, sizeof(sDecodedPic));
  allocOutput (decoder);

  return decoder;
  }
//}}}
//{{{
int decodeOneFrame (sDecoder* decoder, sDecodedPic** decPicList) {

  clearDecodedPics (decoder);

  int iRet = decodeFrame (decoder);
  if (iRet == SOP)
    iRet = DEC_SUCCEED;
  else if (iRet == EOS)
    iRet = DEC_EOS;
  else
    iRet |= DEC_ERRMASK;

  *decPicList = decoder->decOutputPic;
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
  *decPicList = decoder->decOutputPic;
  }
//}}}
//{{{
void closeDecoder (sDecoder* decoder) {

  closeFmo (decoder);
  freeLayerBuffers (decoder);
  freeGlobalBuffers (decoder);

  ercClose (decoder, decoder->ercErrorVar);

  for (unsigned i = 0; i < MAX_PPS; i++) {
    if (decoder->pps[i].valid && decoder->pps[i].sliceGroupId)
      free (decoder->pps[i].sliceGroupId);
    decoder->pps[i].valid = FALSE;
    }

  freeDpb (decoder->dpb);
  freeOutput (decoder);
  freeDecoder (decoder);

  gDecoder = NULL;
  }
//}}}
