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

sVidParam* gVidParam;
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
static void reset_dpb (sVidParam* vidParam, sDPB* dpb ) {

  dpb->vidParam = vidParam;
  dpb->initDone = 0;
  }
//}}}
//{{{
void report_stats_on_error() {
  exit (-1);
  }
//}}}

//{{{
sSlice* allocSlice (sVidParam* vidParam) {

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
static void freeImg (sVidParam* vidParam) {

  if (vidParam != NULL) {
    freeAnnexB (&vidParam->annexB);

    // Free new dpb layers
    for (int i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
      if (vidParam->dpbLayer[i] != NULL) {
        free (vidParam->dpbLayer[i]);
        vidParam->dpbLayer[i] = NULL;
        }

      if (vidParam->codingParam[i]) {
        free (vidParam->codingParam[i]);
        vidParam->codingParam[i] = NULL;
        }

      if (vidParam->layerParam[i]) {
        free (vidParam->layerParam[i]);
        vidParam->layerParam[i] = NULL;
        }
      }

    if (vidParam->oldSlice != NULL) {
      free (vidParam->oldSlice);
      vidParam->oldSlice = NULL;
      }

    if (vidParam->nextSlice) {
      freeSlice (vidParam->nextSlice);
      vidParam->nextSlice=NULL;
      }

    if (vidParam->sliceList) {
      for (int i = 0; i < vidParam->numSlicesAllocated; i++)
        if (vidParam->sliceList[i])
          freeSlice (vidParam->sliceList[i]);
      free (vidParam->sliceList);
      }

    if (vidParam->nalu) {
      freeNALU (vidParam->nalu);
      vidParam->nalu=NULL;
      }

    //free memory;
    freeDecodedPictures (vidParam->decOutputPic);
    if (vidParam->nextPPS) {
      freePPS (vidParam->nextPPS);
      vidParam->nextPPS = NULL;
      }

    free (vidParam);
    vidParam = NULL;
    }
  }
//}}}

//{{{
static void init (sVidParam* vidParam) {

  vidParam->oldFrameSizeInMbs = (unsigned int) -1;

  vidParam->recoveryPoint = 0;
  vidParam->recoveryPointFound = 0;
  vidParam->recoveryPoc = 0x7fffffff; /* set to a max value */

  vidParam->idrPsnrNum = vidParam->input.refOffset;
  vidParam->psnrNum=0;

  vidParam->number = 0;
  vidParam->type = I_SLICE;

  vidParam->gapNumFrame = 0;

  // time for total decoding session
  vidParam->picture = NULL;
  vidParam->MbToSliceGroupMap = NULL;
  vidParam->MapUnitToSliceGroupMap = NULL;

  vidParam->LastAccessUnitExists  = 0;
  vidParam->NALUCount = 0;

  vidParam->outBuffer = NULL;
  vidParam->pendingOut = NULL;
  vidParam->pendingOutState = FRAME;
  vidParam->recoveryFlag = 0;

  vidParam->newFrame = 0;
  vidParam->prevFrameNum = 0;

  vidParam->iLumaPadX = MCBUF_LUMA_PAD_X;
  vidParam->iLumaPadY = MCBUF_LUMA_PAD_Y;
  vidParam->iChromaPadX = MCBUF_CHROMA_PAD_X;
  vidParam->iChromaPadY = MCBUF_CHROMA_PAD_Y;

  vidParam->iPostProcess = 0;
  vidParam->bDeblockEnable = 0x3;
  vidParam->last_dec_view_id = -1;
  vidParam->last_dec_layer_id = -1;
  }
//}}}
//{{{
void init_frext (sVidParam* vidParam) {

  // pel bitdepth init
  vidParam->bitdepth_luma_qp_scale = 6 * (vidParam->bitdepthLuma - 8);

  if(vidParam->bitdepthLuma > vidParam->bitdepthChroma || vidParam->activeSPS->chromaFormatIdc == YUV400)
    vidParam->picUnitBitSizeDisk = (vidParam->bitdepthLuma > 8)? 16:8;
  else
    vidParam->picUnitBitSizeDisk = (vidParam->bitdepthChroma > 8)? 16:8;
  vidParam->dcPredValueComp[0] = 1<<(vidParam->bitdepthLuma - 1);
  vidParam->maxPelValueComp[0] = (1<<vidParam->bitdepthLuma) - 1;
  vidParam->mbSize[0][0] = vidParam->mbSize[0][1] = MB_BLOCK_SIZE;

  if (vidParam->activeSPS->chromaFormatIdc != YUV400) {
    // for chrominance part
    vidParam->bitdepthChromaQpScale = 6 * (vidParam->bitdepthChroma - 8);
    vidParam->dcPredValueComp[1] = (1 << (vidParam->bitdepthChroma - 1));
    vidParam->dcPredValueComp[2] = vidParam->dcPredValueComp[1];
    vidParam->maxPelValueComp[1] = (1 << vidParam->bitdepthChroma) - 1;
    vidParam->maxPelValueComp[2] = (1 << vidParam->bitdepthChroma) - 1;
    vidParam->numBlock8x8uv = (1 << vidParam->activeSPS->chromaFormatIdc) & (~(0x1));
    vidParam->numUvBlocks = (vidParam->numBlock8x8uv >> 1);
    vidParam->numCdcCoeff = (vidParam->numBlock8x8uv << 1);
    vidParam->mbSize[1][0] = vidParam->mbSize[2][0] = vidParam->mbCrSizeX  = (vidParam->activeSPS->chromaFormatIdc==YUV420 || vidParam->activeSPS->chromaFormatIdc==YUV422)?  8 : 16;
    vidParam->mbSize[1][1] = vidParam->mbSize[2][1] = vidParam->mbCrSizeY  = (vidParam->activeSPS->chromaFormatIdc==YUV444 || vidParam->activeSPS->chromaFormatIdc==YUV422)? 16 :  8;

    vidParam->subpelX = vidParam->mbCrSizeX == 8 ? 7 : 3;
    vidParam->subpelY = vidParam->mbCrSizeY == 8 ? 7 : 3;
    vidParam->shiftpelX = vidParam->mbCrSizeX == 8 ? 3 : 2;
    vidParam->shiftpelY = vidParam->mbCrSizeY == 8 ? 3 : 2;
    vidParam->totalScale = vidParam->shiftpelX + vidParam->shiftpelY;
    }
  else {
    vidParam->bitdepthChromaQpScale = 0;
    vidParam->maxPelValueComp[1] = 0;
    vidParam->maxPelValueComp[2] = 0;
    vidParam->numBlock8x8uv = 0;
    vidParam->numUvBlocks = 0;
    vidParam->numCdcCoeff = 0;
    vidParam->mbSize[1][0] = vidParam->mbSize[2][0] = vidParam->mbCrSizeX  = 0;
    vidParam->mbSize[1][1] = vidParam->mbSize[2][1] = vidParam->mbCrSizeY  = 0;
    vidParam->subpelX = 0;
    vidParam->subpelY = 0;
    vidParam->shiftpelX = 0;
    vidParam->shiftpelY = 0;
    vidParam->totalScale = 0;
    }

  vidParam->mbCrSize = vidParam->mbCrSizeX * vidParam->mbCrSizeY;
  vidParam->mbSizeBlock[0][0] = vidParam->mbSizeBlock[0][1] = vidParam->mbSize[0][0] >> 2;
  vidParam->mbSizeBlock[1][0] = vidParam->mbSizeBlock[2][0] = vidParam->mbSize[1][0] >> 2;
  vidParam->mbSizeBlock[1][1] = vidParam->mbSizeBlock[2][1] = vidParam->mbSize[1][1] >> 2;

  vidParam->mbSizeShift[0][0] = vidParam->mbSizeShift[0][1] = ceilLog2sf (vidParam->mbSize[0][0]);
  vidParam->mbSizeShift[1][0] = vidParam->mbSizeShift[2][0] = ceilLog2sf (vidParam->mbSize[1][0]);
  vidParam->mbSizeShift[1][1] = vidParam->mbSizeShift[2][1] = ceilLog2sf (vidParam->mbSize[1][1]);
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
void freeLayerBuffers (sVidParam* vidParam, int layerId) {

  sCoding *codingParam = vidParam->codingParam[layerId];

  if (!vidParam->globalInitDone[layerId])
    return;

  // CAVLC free mem
  if (codingParam->nzCoeff) {
    free_mem4D(codingParam->nzCoeff);
    codingParam->nzCoeff = NULL;
    }

  // free mem, allocated for structure vidParam
  if ((codingParam->sepColourPlaneFlag != 0) ) {
    for (int i = 0; i<MAX_PLANE; i++) {
      free (codingParam->mbDataJV[i]);
      codingParam->mbDataJV[i] = NULL;
      free_mem2Dint(codingParam->siBlockJV[i]);
      codingParam->siBlockJV[i] = NULL;
      free_mem2D(codingParam->predModeJV[i]);
      codingParam->predModeJV[i] = NULL;
      free (codingParam->intraBlockJV[i]);
      codingParam->intraBlockJV[i] = NULL;
      }
    }
  else {
    if (codingParam->mbData != NULL) {
      free(codingParam->mbData);
      codingParam->mbData = NULL;
      }
    if (codingParam->siBlock) {
      free_mem2Dint(codingParam->siBlock);
      codingParam->siBlock = NULL;
      }
    if (codingParam->predMode) {
      free_mem2D(codingParam->predMode);
      codingParam->predMode = NULL;
      }
    if (codingParam->intraBlock) {
      free (codingParam->intraBlock);
      codingParam->intraBlock = NULL;
      }
    }

  if (codingParam->picPos) {
    free(codingParam->picPos);
    codingParam->picPos = NULL;
    }

  freeQuant (codingParam);

  vidParam->globalInitDone[layerId] = 0;
  }
//}}}
//{{{
void initGlobalBuffers (sVidParam* vidParam, int layerId) {

  sCoding *codingParam = vidParam->codingParam[layerId];
  sBlockPos* picPos;

  if (vidParam->globalInitDone[layerId])
    freeLayerBuffers (vidParam, layerId);

  // allocate memory in structure vidParam
  if (codingParam->sepColourPlaneFlag != 0) {
    for (int i = 0; i<MAX_PLANE; ++i )
      if (((codingParam->mbDataJV[i]) = (sMacroblock*)calloc(codingParam->FrameSizeInMbs, sizeof(sMacroblock))) == NULL)
        no_mem_exit ("initGlobalBuffers: codingParam->mbDataJV");
    codingParam->mbData = NULL;
    }
  else if (((codingParam->mbData) = (sMacroblock*)calloc (codingParam->FrameSizeInMbs, sizeof(sMacroblock))) == NULL)
    no_mem_exit ("initGlobalBuffers: codingParam->mbData");

  if (codingParam->sepColourPlaneFlag != 0) {
    for (int i = 0; i < MAX_PLANE; ++i )
      if (((codingParam->intraBlockJV[i]) = (char*) calloc(codingParam->FrameSizeInMbs, sizeof(char))) == NULL)
        no_mem_exit ("initGlobalBuffers: codingParam->intraBlockJV");
    codingParam->intraBlock = NULL;
    }
  else if (((codingParam->intraBlock) = (char*)calloc (codingParam->FrameSizeInMbs, sizeof(char))) == NULL)
    no_mem_exit ("initGlobalBuffers: codingParam->intraBlock");

  if (((codingParam->picPos) = (sBlockPos*)calloc(codingParam->FrameSizeInMbs + 1, sizeof(sBlockPos))) == NULL)
    no_mem_exit ("initGlobalBuffers: picPos");

  picPos = codingParam->picPos;
  for (int i = 0; i < (int) codingParam->FrameSizeInMbs + 1;++i) {
    picPos[i].x = (short)(i % codingParam->PicWidthInMbs);
    picPos[i].y = (short)(i / codingParam->PicWidthInMbs);
    }

  if( (codingParam->sepColourPlaneFlag != 0)) {
    for (int i = 0; i < MAX_PLANE; ++i )
      get_mem2D (&(codingParam->predModeJV[i]), 4*codingParam->FrameHeightInMbs, 4*codingParam->PicWidthInMbs);
    codingParam->predMode = NULL;
    }
  else
    get_mem2D (&(codingParam->predMode), 4*codingParam->FrameHeightInMbs, 4*codingParam->PicWidthInMbs);

  // CAVLC mem
  get_mem4D (&(codingParam->nzCoeff), codingParam->FrameSizeInMbs, 3, BLOCK_SIZE, BLOCK_SIZE);
  if (codingParam->sepColourPlaneFlag != 0) {
    for (int i = 0; i < MAX_PLANE; ++i ) {
      get_mem2Dint (&(codingParam->siBlockJV[i]), codingParam->FrameHeightInMbs, codingParam->PicWidthInMbs);
      if (codingParam->siBlockJV[i] == NULL)
        no_mem_exit ("initGlobalBuffers: vidParam->siBlockJV");
      }
    codingParam->siBlock = NULL;
    }
  else
    get_mem2Dint (&(codingParam->siBlock), codingParam->FrameHeightInMbs, codingParam->PicWidthInMbs);

  allocQuant (codingParam);
  codingParam->oldFrameSizeInMbs = codingParam->FrameSizeInMbs;
  vidParam->globalInitDone[layerId] = 1;
  }
//}}}
//{{{
void freeGlobalBuffers (sVidParam* vidParam) {

  if (vidParam->picture) {
    freePicture (vidParam->picture);
    vidParam->picture = NULL;
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
void ClearDecodedPictures (sVidParam* vidParam) {

  // find the head first;
  sDecodedPicture* prevDecodedPicture = NULL;
  sDecodedPicture* decodedPicture = vidParam->decOutputPic;
  while (decodedPicture && !decodedPicture->valid) {
    prevDecodedPicture = decodedPicture;
    decodedPicture = decodedPicture->next;
    }

  if (decodedPicture && (decodedPicture != vidParam->decOutputPic)) {
    // move all nodes before decodedPicture to the end;
    sDecodedPicture* decodedPictureTail = decodedPicture;
    while (decodedPictureTail->next)
      decodedPictureTail = decodedPictureTail->next;

    decodedPictureTail->next = vidParam->decOutputPic;
    vidParam->decOutputPic = decodedPicture;
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
void setGlobalCodingProgram (sVidParam* vidParam, sCoding* codingParam) {

  vidParam->bitdepthChroma = 0;
  vidParam->widthCr = 0;
  vidParam->heightCr = 0;
  vidParam->lossless_qpprime_flag = codingParam->lossless_qpprime_flag;
  vidParam->max_vmv_r = codingParam->max_vmv_r;

  // Fidelity Range Extensions stuff (part 1)
  vidParam->bitdepthLuma = codingParam->bitdepthLuma;
  vidParam->bitdepth_scale[0] = codingParam->bitdepth_scale[0];
  vidParam->bitdepthChroma = codingParam->bitdepthChroma;
  vidParam->bitdepth_scale[1] = codingParam->bitdepth_scale[1];

  vidParam->maxFrameNum = codingParam->maxFrameNum;
  vidParam->PicWidthInMbs = codingParam->PicWidthInMbs;
  vidParam->PicHeightInMapUnits = codingParam->PicHeightInMapUnits;
  vidParam->FrameHeightInMbs = codingParam->FrameHeightInMbs;
  vidParam->FrameSizeInMbs = codingParam->FrameSizeInMbs;

  vidParam->yuvFormat = codingParam->yuvFormat;
  vidParam->sepColourPlaneFlag = codingParam->sepColourPlaneFlag;
  vidParam->ChromaArrayType = codingParam->ChromaArrayType;

  vidParam->width = codingParam->width;
  vidParam->height = codingParam->height;
  vidParam->iLumaPadX = MCBUF_LUMA_PAD_X;
  vidParam->iLumaPadY = MCBUF_LUMA_PAD_Y;
  vidParam->iChromaPadX = MCBUF_CHROMA_PAD_X;
  vidParam->iChromaPadY = MCBUF_CHROMA_PAD_Y;

  if (vidParam->yuvFormat == YUV420) {
    vidParam->widthCr = (vidParam->width  >> 1);
    vidParam->heightCr = (vidParam->height >> 1);
    }
  else if (vidParam->yuvFormat == YUV422) {
    vidParam->widthCr = (vidParam->width >> 1);
    vidParam->heightCr = vidParam->height;
    vidParam->iChromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (vidParam->yuvFormat == YUV444) {
    //YUV444
    vidParam->widthCr = vidParam->width;
    vidParam->heightCr = vidParam->height;
    vidParam->iChromaPadX = vidParam->iLumaPadX;
    vidParam->iChromaPadY = vidParam->iLumaPadY;
    }

  init_frext(vidParam);
  }
//}}}

//{{{
void OpenDecoder (sInput* input, byte* chunk, size_t chunkSize) {

  // alloc decoder
  sVidParam* vidParam = (sVidParam*)calloc (1, sizeof(sVidParam));
  gVidParam = vidParam;

  init_time();

  // init input
  memcpy (&(vidParam->input), input, sizeof(sInput));
  gVidParam->concealMode = input->concealMode;
  gVidParam->refPocGap = input->refPocGap;
  gVidParam->pocGap = input->pocGap;

  // init nalu, annexB
  vidParam->nalu = allocNALU (MAX_CODED_FRAME_SIZE);
  vidParam->nextPPS = allocPPS();
  vidParam->firstSPS = TRUE;
  vidParam->annexB = allocAnnexB (vidParam);
  openAnnexB (vidParam->annexB, chunk, chunkSize);

  // init slice
  vidParam->globalInitDone[0] = vidParam->globalInitDone[1] = 0;
  vidParam->oldSlice = (sOldSlice*)calloc (1, sizeof(sOldSlice));
  vidParam->sliceList = (sSlice**)calloc (MAX_NUM_DECSLICES, sizeof(sSlice*));
  vidParam->numSlicesAllocated = MAX_NUM_DECSLICES;
  vidParam->nextSlice = NULL;
  initOldSlice (vidParam->oldSlice);

  init (vidParam);

  // init output
  for (int i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
    vidParam->dpbLayer[i] = (sDPB*)calloc (1, sizeof(sDPB));
    vidParam->dpbLayer[i]->layerId = i;
    reset_dpb (vidParam, vidParam->dpbLayer[i]);
    vidParam->codingParam[i] = (sCoding*)calloc (1, sizeof(sCoding));
    vidParam->codingParam[i]->layerId = i;
    vidParam->layerParam[i] = (sLayer*)calloc (1, sizeof(sLayer));
    vidParam->layerParam[i]->layerId = i;
    }

  vidParam->decOutputPic = (sDecodedPicture*)calloc (1, sizeof(sDecodedPicture));
  allocOutput (vidParam);
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
