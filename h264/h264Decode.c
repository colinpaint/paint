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

sDecoder* gVidParam;
char errortext[ET_SIZE];

//{{{
void error (char* text, int code) {

  fprintf (stderr, "%s\n", text);
  if (gVidParam)
    flushDpb (gVidParam->dpbLayer[0]);
  exit (code);
  }
//}}}
//{{{
static void reset_dpb (sDecoder* decoder, sDPB* dpb ) {

  dpb->decoder = decoder;
  dpb->initDone = 0;
  }
//}}}
//{{{
void report_stats_on_error() {
  exit (-1);
  }
//}}}

//{{{
sSlice* allocSlice (sDecoder* decoder) {

  sSlice* curSlice = (sSlice*)calloc (1, sizeof(sSlice));
  if (!curSlice) {
    snprintf (errortext, ET_SIZE, "Memory allocation for sSlice datastruct in NAL-mode failed");
    error (errortext, 100);
    }

  // create all context models
  curSlice->mot_ctx = create_contexts_MotionInfo();
  curSlice->tex_ctx = create_contexts_TextureInfo();

  curSlice->maxPartitionNum = 3;  //! assume data partitioning (worst case) for the following mallocs()
  curSlice->partitions = allocPartition (curSlice->maxPartitionNum);

  get_mem2Dwp (&(curSlice->WPParam), 2, MAX_REFERENCE_PICTURES);
  get_mem3Dint (&(curSlice->wpWeight), 2, MAX_REFERENCE_PICTURES, 3);
  get_mem3Dint (&(curSlice->wpOffset), 6, MAX_REFERENCE_PICTURES, 3);
  get_mem4Dint (&(curSlice->wbpWeight), 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);
  get_mem3Dpel (&(curSlice->mb_pred), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  get_mem3Dpel (&(curSlice->mb_rec), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  get_mem3Dint (&(curSlice->mb_rres), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  get_mem3Dint (&(curSlice->cof), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  allocPred (curSlice);

  // reference flag initialization
  for (int i = 0; i < 17; ++i)
    curSlice->refFlag[i] = 1;

  for (int i = 0; i < 6; i++) {
    curSlice->listX[i] = calloc (MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering
    if (!curSlice->listX[i])
      no_mem_exit ("allocSlice: curSlice->listX[i]");
    }

  for (int j = 0; j < 6; j++) {
    for (int i = 0; i < MAX_LIST_SIZE; i++)
      curSlice->listX[j][i] = NULL;
    curSlice->listXsize[j]=0;
    }

  return curSlice;
  }
//}}}
//{{{
static void freeSlice (sSlice *curSlice) {

  if (curSlice->sliceType != I_SLICE && curSlice->sliceType != SI_SLICE)
    freeRefPicListReorderingBuffer (curSlice);

  freePred (curSlice);
  free_mem3Dint (curSlice->cof);
  free_mem3Dint (curSlice->mb_rres);
  free_mem3Dpel (curSlice->mb_rec);
  free_mem3Dpel (curSlice->mb_pred);

  free_mem2Dwp (curSlice->WPParam);
  free_mem3Dint (curSlice->wpWeight);
  free_mem3Dint (curSlice->wpOffset);
  free_mem4Dint (curSlice->wbpWeight);

  freePartition (curSlice->partitions, 3);

  // delete all context models
  delete_contexts_MotionInfo (curSlice->mot_ctx);
  delete_contexts_TextureInfo (curSlice->tex_ctx);

  for (int i = 0; i<6; i++) {
    if (curSlice->listX[i]) {
      free (curSlice->listX[i]);
      curSlice->listX[i] = NULL;
      }
    }

  while (curSlice->decRefPicMarkingBuffer) {
    sDecRefPicMarking* tmp_drpm = curSlice->decRefPicMarkingBuffer;
    curSlice->decRefPicMarkingBuffer = tmp_drpm->next;
    free (tmp_drpm);
    }

  free (curSlice);
  curSlice = NULL;
  }
//}}}

//{{{
static void freeImg (sDecoder* decoder) {

  if (decoder != NULL) {
    freeAnnexB (&decoder->annexB);

    // Free new dpb layers
    for (int i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
      if (decoder->dpbLayer[i] != NULL) {
        free (decoder->dpbLayer[i]);
        decoder->dpbLayer[i] = NULL;
        }

      if (decoder->coding[i]) {
        free (decoder->coding[i]);
        decoder->coding[i] = NULL;
        }

      if (decoder->layer[i]) {
        free (decoder->layer[i]);
        decoder->layer[i] = NULL;
        }
      }

    if (decoder->oldSlice != NULL) {
      free (decoder->oldSlice);
      decoder->oldSlice = NULL;
      }

    if (decoder->nextSlice) {
      freeSlice (decoder->nextSlice);
      decoder->nextSlice=NULL;
      }

    if (decoder->sliceList) {
      for (int i = 0; i < decoder->numSlicesAllocated; i++)
        if (decoder->sliceList[i])
          freeSlice (decoder->sliceList[i]);
      free (decoder->sliceList);
      }

    if (decoder->nalu) {
      freeNALU (decoder->nalu);
      decoder->nalu=NULL;
      }

    //free memory;
    freeDecodedPictures (decoder->decOutputPic);
    if (decoder->nextPPS) {
      freePPS (decoder->nextPPS);
      decoder->nextPPS = NULL;
      }

    free (decoder);
    decoder = NULL;
    }
  }
//}}}

//{{{
static void init (sDecoder* decoder) {

  decoder->oldFrameSizeInMbs = (unsigned int) -1;

  decoder->recoveryPoint = 0;
  decoder->recoveryPointFound = 0;
  decoder->recoveryPoc = 0x7fffffff; /* set to a max value */

  decoder->idrPsnrNum = decoder->input.refOffset;
  decoder->psnrNum=0;

  decoder->number = 0;
  decoder->type = I_SLICE;

  decoder->gapNumFrame = 0;

  // time for total decoding session
  decoder->picture = NULL;
  decoder->mbToSliceGroupMap = NULL;
  decoder->mapUnitToSliceGroupMap = NULL;

  decoder->lastAccessUnitExists  = 0;
  decoder->naluCount = 0;

  decoder->outBuffer = NULL;
  decoder->pendingOut = NULL;
  decoder->pendingOutState = FRAME;
  decoder->recoveryFlag = 0;

  decoder->newFrame = 0;
  decoder->prevFrameNum = 0;

  decoder->iLumaPadX = MCBUF_LUMA_PAD_X;
  decoder->iLumaPadY = MCBUF_LUMA_PAD_Y;
  decoder->iChromaPadX = MCBUF_CHROMA_PAD_X;
  decoder->iChromaPadY = MCBUF_CHROMA_PAD_Y;

  decoder->iPostProcess = 0;
  decoder->bDeblockEnable = 0x3;
  decoder->last_dec_view_id = -1;
  decoder->last_dec_layer_id = -1;
  }
//}}}
//{{{
void init_frext (sDecoder* decoder) {

  // pel bitdepth init
  decoder->bitdepth_luma_qp_scale = 6 * (decoder->bitdepthLuma - 8);

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
sDataPartition* allocPartition (int n) {

  sDataPartition* partitions = (sDataPartition*)calloc (n, sizeof(sDataPartition));
  if (partitions == NULL) {
    snprintf (errortext, ET_SIZE, "allocPartition: Memory allocation for Data Partition failed");
    error (errortext, 100);
    }

  for (int i = 0; i < n; ++i) {// loop over all data partitions
    sDataPartition* dataPart = &(partitions[i]);
    dataPart->bitstream = (sBitstream *) calloc(1, sizeof(sBitstream));
    if (dataPart->bitstream == NULL) {
      snprintf (errortext, ET_SIZE, "allocPartition: Memory allocation for sBitstream failed");
      error (errortext, 100);
      }

    dataPart->bitstream->streamBuffer = (byte *) calloc(MAX_CODED_FRAME_SIZE, sizeof(byte));
    if (dataPart->bitstream->streamBuffer == NULL) {
      snprintf (errortext, ET_SIZE, "allocPartition: Memory allocation for streamBuffer failed");
      error (errortext, 100);
      }
    }

  return partitions;
  }
//}}}
//{{{
void freePartition (sDataPartition* dp, int n) {

  assert (dp != NULL);
  assert (dp->bitstream != NULL);
  assert (dp->bitstream->streamBuffer != NULL);

  for (int i = 0; i < n; ++i) {
    free (dp[i].bitstream->streamBuffer);
    free (dp[i].bitstream);
    }

  free (dp);
  }
//}}}

//{{{
void freeLayerBuffers (sDecoder* decoder, int layerId) {

  sCoding *coding = decoder->coding[layerId];

  if (!decoder->globalInitDone[layerId])
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

  decoder->globalInitDone[layerId] = 0;
  }
//}}}
//{{{
void initGlobalBuffers (sDecoder* decoder, int layerId) {

  sCoding *coding = decoder->coding[layerId];
  sBlockPos* picPos;

  if (decoder->globalInitDone[layerId])
    freeLayerBuffers (decoder, layerId);

  // allocate memory in structure decoder
  if (coding->sepColourPlaneFlag != 0) {
    for (int i = 0; i<MAX_PLANE; ++i )
      if (((coding->mbDataJV[i]) = (sMacroblock*)calloc(coding->FrameSizeInMbs, sizeof(sMacroblock))) == NULL)
        no_mem_exit ("initGlobalBuffers: coding->mbDataJV");
    coding->mbData = NULL;
    }
  else if (((coding->mbData) = (sMacroblock*)calloc (coding->FrameSizeInMbs, sizeof(sMacroblock))) == NULL)
    no_mem_exit ("initGlobalBuffers: coding->mbData");

  if (coding->sepColourPlaneFlag != 0) {
    for (int i = 0; i < MAX_PLANE; ++i )
      if (((coding->intraBlockJV[i]) = (char*) calloc(coding->FrameSizeInMbs, sizeof(char))) == NULL)
        no_mem_exit ("initGlobalBuffers: coding->intraBlockJV");
    coding->intraBlock = NULL;
    }
  else if (((coding->intraBlock) = (char*)calloc (coding->FrameSizeInMbs, sizeof(char))) == NULL)
    no_mem_exit ("initGlobalBuffers: coding->intraBlock");

  if (((coding->picPos) = (sBlockPos*)calloc(coding->FrameSizeInMbs + 1, sizeof(sBlockPos))) == NULL)
    no_mem_exit ("initGlobalBuffers: picPos");

  picPos = coding->picPos;
  for (int i = 0; i < (int) coding->FrameSizeInMbs + 1;++i) {
    picPos[i].x = (short)(i % coding->PicWidthInMbs);
    picPos[i].y = (short)(i / coding->PicWidthInMbs);
    }

  if( (coding->sepColourPlaneFlag != 0)) {
    for (int i = 0; i < MAX_PLANE; ++i )
      get_mem2D (&(coding->predModeJV[i]), 4*coding->FrameHeightInMbs, 4*coding->PicWidthInMbs);
    coding->predMode = NULL;
    }
  else
    get_mem2D (&(coding->predMode), 4*coding->FrameHeightInMbs, 4*coding->PicWidthInMbs);

  // CAVLC mem
  get_mem4D (&(coding->nzCoeff), coding->FrameSizeInMbs, 3, BLOCK_SIZE, BLOCK_SIZE);
  if (coding->sepColourPlaneFlag != 0) {
    for (int i = 0; i < MAX_PLANE; ++i ) {
      get_mem2Dint (&(coding->siBlockJV[i]), coding->FrameHeightInMbs, coding->PicWidthInMbs);
      if (coding->siBlockJV[i] == NULL)
        no_mem_exit ("initGlobalBuffers: decoder->siBlockJV");
      }
    coding->siBlock = NULL;
    }
  else
    get_mem2Dint (&(coding->siBlock), coding->FrameHeightInMbs, coding->PicWidthInMbs);

  allocQuant (coding);
  coding->oldFrameSizeInMbs = coding->FrameSizeInMbs;
  decoder->globalInitDone[layerId] = 1;
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
sDecodedPicture* getDecodedPicture (sDecodedPicture* decodedPicture) {

  sDecodedPicture* prevDecodedPicture = NULL;
  while (decodedPicture && (decodedPicture->valid)) {
    prevDecodedPicture = decodedPicture;
    decodedPicture = decodedPicture->next;
    }

  if (!decodedPicture) {
    decodedPicture = (sDecodedPicture*)calloc(1, sizeof(*decodedPicture));
    prevDecodedPicture->next = decodedPicture;
    }

  return decodedPicture;
  }
//}}}
//{{{
void ClearDecodedPictures (sDecoder* decoder) {

  // find the head first;
  sDecodedPicture* prevDecodedPicture = NULL;
  sDecodedPicture* decodedPicture = decoder->decOutputPic;
  while (decodedPicture && !decodedPicture->valid) {
    prevDecodedPicture = decodedPicture;
    decodedPicture = decodedPicture->next;
    }

  if (decodedPicture && (decodedPicture != decoder->decOutputPic)) {
    // move all nodes before decodedPicture to the end;
    sDecodedPicture* decodedPictureTail = decodedPicture;
    while (decodedPictureTail->next)
      decodedPictureTail = decodedPictureTail->next;

    decodedPictureTail->next = decoder->decOutputPic;
    decoder->decOutputPic = decodedPicture;
    prevDecodedPicture->next = NULL;
    }
  }
//}}}
//{{{
void freeDecodedPictures (sDecodedPicture* decodedPicture) {

  while (decodedPicture) {
    sDecodedPicture* nextDecodedPicture = decodedPicture->next;
    if (decodedPicture->yBuf) {
      free (decodedPicture->yBuf);
      decodedPicture->yBuf = NULL;
      decodedPicture->uBuf = NULL;
      decodedPicture->vBuf = NULL;
      }

    free (decodedPicture);
    decodedPicture = nextDecodedPicture;
   }
  }
//}}}

//{{{
void setGlobalCodingProgram (sDecoder* decoder, sCoding* coding) {

  decoder->bitdepthChroma = 0;
  decoder->widthCr = 0;
  decoder->heightCr = 0;
  decoder->lossless_qpprime_flag = coding->lossless_qpprime_flag;
  decoder->maxVmvR = coding->maxVmvR;

  // Fidelity Range Extensions stuff (part 1)
  decoder->bitdepthLuma = coding->bitdepthLuma;
  decoder->bitdepth_scale[0] = coding->bitdepth_scale[0];
  decoder->bitdepthChroma = coding->bitdepthChroma;
  decoder->bitdepth_scale[1] = coding->bitdepth_scale[1];

  decoder->maxFrameNum = coding->maxFrameNum;
  decoder->PicWidthInMbs = coding->PicWidthInMbs;
  decoder->PicHeightInMapUnits = coding->PicHeightInMapUnits;
  decoder->FrameHeightInMbs = coding->FrameHeightInMbs;
  decoder->FrameSizeInMbs = coding->FrameSizeInMbs;

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

  init_frext(decoder);
  }
//}}}

//{{{
void OpenDecoder (sInput* input, byte* chunk, size_t chunkSize) {

  // alloc decoder
  sDecoder* decoder = (sDecoder*)calloc (1, sizeof(sDecoder));
  gVidParam = decoder;

  init_time();

  // init input
  memcpy (&(decoder->input), input, sizeof(sInput));
  gVidParam->concealMode = input->concealMode;
  gVidParam->refPocGap = input->refPocGap;
  gVidParam->pocGap = input->pocGap;

  // init nalu, annexB
  decoder->nalu = allocNALU (MAX_CODED_FRAME_SIZE);
  decoder->nextPPS = allocPPS();
  decoder->firstSPS = TRUE;
  decoder->annexB = allocAnnexB (decoder);
  openAnnexB (decoder->annexB, chunk, chunkSize);

  // init slice
  decoder->globalInitDone[0] = decoder->globalInitDone[1] = 0;
  decoder->oldSlice = (sOldSlice*)calloc (1, sizeof(sOldSlice));
  decoder->sliceList = (sSlice**)calloc (MAX_NUM_DECSLICES, sizeof(sSlice*));
  decoder->numSlicesAllocated = MAX_NUM_DECSLICES;
  decoder->nextSlice = NULL;
  initOldSlice (decoder->oldSlice);

  init (decoder);

  // init output
  for (int i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
    decoder->dpbLayer[i] = (sDPB*)calloc (1, sizeof(sDPB));
    decoder->dpbLayer[i]->layerId = i;
    reset_dpb (decoder, decoder->dpbLayer[i]);
    decoder->coding[i] = (sCoding*)calloc (1, sizeof(sCoding));
    decoder->coding[i]->layerId = i;
    decoder->layer[i] = (sLayer*)calloc (1, sizeof(sLayer));
    decoder->layer[i]->layerId = i;
    }

  decoder->decOutputPic = (sDecodedPicture*)calloc (1, sizeof(sDecodedPicture));
  allocOutput (decoder);
  }
//}}}
//{{{
int DecodeOneFrame (sDecodedPicture** ppDecPicList) {

  ClearDecodedPictures (gVidParam);

  int iRet = decodeFrame (gVidParam);
  if (iRet == SOP)
    iRet = DEC_SUCCEED;
  else if (iRet == EOS)
    iRet = DEC_EOS;
  else
    iRet |= DEC_ERRMASK;

  *ppDecPicList = gVidParam->decOutputPic;
  return iRet;
  }
//}}}
//{{{
void FinitDecoder (sDecodedPicture** ppDecPicList) {

  ClearDecodedPictures (gVidParam);
  flushDpb (gVidParam->dpbLayer[0]);

  resetAnnexB (gVidParam->annexB);

  gVidParam->newFrame = 0;
  gVidParam->prevFrameNum = 0;
  *ppDecPicList = gVidParam->decOutputPic;
  }
//}}}
//{{{
void CloseDecoder() {

  FmoFinit (gVidParam);
  freeLayerBuffers (gVidParam, 0);
  freeLayerBuffers (gVidParam, 1);
  freeGlobalBuffers (gVidParam);

  ercClose (gVidParam, gVidParam->ercErrorVar);

  cleanUpPPS (gVidParam);

  for (unsigned i = 0; i < MAX_NUM_DPB_LAYERS; i++)
    freeDpb (gVidParam->dpbLayer[i]);
  freeOutput (gVidParam);
  freeImg (gVidParam);
  free (gVidParam);

  gVidParam = NULL;
  }
//}}}
