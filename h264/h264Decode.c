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

sDecoderParam* gDecoder;
char errortext[ET_SIZE];

//{{{
void error (char* text, int code) {

  fprintf (stderr, "%s\n", text);
  if (gDecoder)
    flush_dpb (gDecoder->vidParam->dpbLayer[0]);
  exit (code);
  }
//}}}
//{{{
static void reset_dpb (sVidParam* vidParam, sDPB* dpb ) {

  dpb->vidParam = vidParam;
  dpb->init_done = 0;
  }
//}}}
//{{{
void report_stats_on_error() {
  exit (-1);
  }
//}}}

//{{{
static void allocVidParam (sVidParam** vidParam) {

  if ((*vidParam = (sVidParam*)calloc (1, sizeof(sVidParam)))==NULL)
    no_mem_exit ("alloc_VidParamams: vidParam");

  if (((*vidParam)->oldSlice = (sOldSliceParam*)calloc(1, sizeof(sOldSliceParam)))==NULL)
    no_mem_exit ("alloc_VidParamams: vidParam->oldSlice");

  // Allocate new dpb buffer
  for (int i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
    if (((*vidParam)->dpbLayer[i] = (sDPB*)calloc(1, sizeof(sDPB)))==NULL)
      no_mem_exit ("alloc_VidParamams: vidParam->dpbLayer[i]");

    (*vidParam)->dpbLayer[i]->layerId = i;
    reset_dpb(*vidParam, (*vidParam)->dpbLayer[i]);
    if(((*vidParam)->codingParam[i] = (sCodingParam*)calloc(1, sizeof(sCodingParam))) == NULL)
      no_mem_exit ("alloc_VidParamams:vidParam->codingParam[i]");

    ((*vidParam)->codingParam[i])->layerId = i;
    if(((*vidParam)->layerParam[i] = (sLayerParam*)calloc(1, sizeof(sLayerParam))) == NULL)
      no_mem_exit ("alloc_VidParamams:vidParam->layerParam[i]");
    ((*vidParam)->layerParam[i])->layerId = i;
    }

  (*vidParam)->globalInitDone[0] = (*vidParam)->globalInitDone[1] = 0;

  if (((*vidParam)->sliceList = (sSlice**)calloc(MAX_NUM_DECSLICES, sizeof(sSlice *))) == NULL)
    no_mem_exit ("alloc_VidParamams: vidParam->sliceList");

  (*vidParam)->numSlicesAllocated = MAX_NUM_DECSLICES;
  (*vidParam)->nextSlice = NULL;
  (*vidParam)->nalu = allocNALU (MAX_CODED_FRAME_SIZE);
  (*vidParam)->decOutputPic = (sDecodedPicList*)calloc(1, sizeof(sDecodedPicList));
  (*vidParam)->nextPPS = allocPPS();
  (*vidParam)->firstSPS = TRUE;
}
//}}}
//{{{
static void allocInputParam (sInputParam** inputParam ) {
  *inputParam = (sInputParam*)calloc (1, sizeof(sInputParam));
  }
//}}}
//{{{
static int allocDecoder (sDecoderParam** decoder) {

  *decoder = (sDecoderParam*)calloc(1, sizeof(sDecoderParam));
  allocVidParam(&((*decoder)->vidParam));
  allocInputParam(&((*decoder)->inputParam));
  (*decoder)->vidParam->inputParam = (*decoder)->inputParam;
  return 0;
}
//}}}
//{{{
sSlice* allocSlice (sInputParam* inputParam, sVidParam* vidParam) {

  sSlice* curSlice = (sSlice*)calloc (1, sizeof(sSlice));
  if (!curSlice) {
    snprintf (errortext, ET_SIZE, "Memory allocation for sSlice datastruct in NAL-mode failed");
    error (errortext, 100);
    }

  // create all context models
  curSlice->mot_ctx = create_contexts_MotionInfo();
  curSlice->tex_ctx = create_contexts_TextureInfo();

  curSlice->max_part_nr = 3;  //! assume data partitioning (worst case) for the following mallocs()
  curSlice->partArr = allocPartition (curSlice->max_part_nr);

  get_mem2Dwp (&(curSlice->WPParam), 2, MAX_REFERENCE_PICTURES);
  get_mem3Dint (&(curSlice->wp_weight), 2, MAX_REFERENCE_PICTURES, 3);
  get_mem3Dint (&(curSlice->wp_offset), 6, MAX_REFERENCE_PICTURES, 3);
  get_mem4Dint (&(curSlice->wbp_weight), 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);
  get_mem3Dpel (&(curSlice->mb_pred), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  get_mem3Dpel (&(curSlice->mb_rec), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  get_mem3Dint (&(curSlice->mb_rres), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  get_mem3Dint (&(curSlice->cof), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  allocPred (curSlice);

  // reference flag initialization
  for (int i = 0; i < 17; ++i)
    curSlice->ref_flag[i] = 1;

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

  if (curSlice->slice_type != I_SLICE && curSlice->slice_type != SI_SLICE)
    free_ref_pic_list_reordering_buffer (curSlice);

  freePred (curSlice);
  free_mem3Dint (curSlice->cof);
  free_mem3Dint (curSlice->mb_rres);
  free_mem3Dpel (curSlice->mb_rec);
  free_mem3Dpel (curSlice->mb_pred);

  free_mem2Dwp (curSlice->WPParam);
  free_mem3Dint (curSlice->wp_weight);
  free_mem3Dint (curSlice->wp_offset);
  free_mem4Dint (curSlice->wbp_weight);

  freePartition (curSlice->partArr, 3);

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
    freeDecPicList (vidParam->decOutputPic);
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

  sInputParam *inputParam = vidParam->inputParam;
  vidParam->oldFrameSizeInMbs = (unsigned int) -1;

  vidParam->recovery_point = 0;
  vidParam->recovery_point_found = 0;
  vidParam->recovery_poc = 0x7fffffff; /* set to a max value */

  vidParam->idrPsnrNum = inputParam->ref_offset;
  vidParam->psnrNum=0;

  vidParam->number = 0;
  vidParam->type = I_SLICE;

  vidParam->gapNumFrame = 0;

  // time for total decoding session
  vidParam->totTime = 0;

  vidParam->picture = NULL;
  vidParam->MbToSliceGroupMap = NULL;
  vidParam->MapUnitToSliceGroupMap = NULL;

  vidParam->LastAccessUnitExists  = 0;
  vidParam->NALUCount = 0;

  vidParam->outBuffer = NULL;
  vidParam->pendingOutput = NULL;
  vidParam->pendingOutputState = FRAME;
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
  vidParam->bitdepth_luma_qp_scale = 6 * (vidParam->bitdepth_luma - 8);

  if(vidParam->bitdepth_luma > vidParam->bitdepth_chroma || vidParam->activeSPS->chromaFormatIdc == YUV400)
    vidParam->pic_unit_bitsize_on_disk = (vidParam->bitdepth_luma > 8)? 16:8;
  else
    vidParam->pic_unit_bitsize_on_disk = (vidParam->bitdepth_chroma > 8)? 16:8;
  vidParam->dc_pred_value_comp[0] = 1<<(vidParam->bitdepth_luma - 1);
  vidParam->max_pel_value_comp[0] = (1<<vidParam->bitdepth_luma) - 1;
  vidParam->mb_size[0][0] = vidParam->mb_size[0][1] = MB_BLOCK_SIZE;

  if (vidParam->activeSPS->chromaFormatIdc != YUV400) {
    // for chrominance part
    vidParam->bitdepth_chroma_qp_scale = 6 * (vidParam->bitdepth_chroma - 8);
    vidParam->dc_pred_value_comp[1] = (1 << (vidParam->bitdepth_chroma - 1));
    vidParam->dc_pred_value_comp[2] = vidParam->dc_pred_value_comp[1];
    vidParam->max_pel_value_comp[1] = (1 << vidParam->bitdepth_chroma) - 1;
    vidParam->max_pel_value_comp[2] = (1 << vidParam->bitdepth_chroma) - 1;
    vidParam->num_blk8x8_uv = (1 << vidParam->activeSPS->chromaFormatIdc) & (~(0x1));
    vidParam->num_uv_blocks = (vidParam->num_blk8x8_uv >> 1);
    vidParam->num_cdc_coeff = (vidParam->num_blk8x8_uv << 1);
    vidParam->mb_size[1][0] = vidParam->mb_size[2][0] = vidParam->mb_cr_size_x  = (vidParam->activeSPS->chromaFormatIdc==YUV420 || vidParam->activeSPS->chromaFormatIdc==YUV422)?  8 : 16;
    vidParam->mb_size[1][1] = vidParam->mb_size[2][1] = vidParam->mb_cr_size_y  = (vidParam->activeSPS->chromaFormatIdc==YUV444 || vidParam->activeSPS->chromaFormatIdc==YUV422)? 16 :  8;

    vidParam->subpel_x = vidParam->mb_cr_size_x == 8 ? 7 : 3;
    vidParam->subpel_y = vidParam->mb_cr_size_y == 8 ? 7 : 3;
    vidParam->shiftpel_x = vidParam->mb_cr_size_x == 8 ? 3 : 2;
    vidParam->shiftpel_y = vidParam->mb_cr_size_y == 8 ? 3 : 2;
    vidParam->total_scale = vidParam->shiftpel_x + vidParam->shiftpel_y;
    }
  else {
    vidParam->bitdepth_chroma_qp_scale = 0;
    vidParam->max_pel_value_comp[1] = 0;
    vidParam->max_pel_value_comp[2] = 0;
    vidParam->num_blk8x8_uv = 0;
    vidParam->num_uv_blocks = 0;
    vidParam->num_cdc_coeff = 0;
    vidParam->mb_size[1][0] = vidParam->mb_size[2][0] = vidParam->mb_cr_size_x  = 0;
    vidParam->mb_size[1][1] = vidParam->mb_size[2][1] = vidParam->mb_cr_size_y  = 0;
    vidParam->subpel_x = 0;
    vidParam->subpel_y = 0;
    vidParam->shiftpel_x = 0;
    vidParam->shiftpel_y = 0;
    vidParam->total_scale = 0;
    }

  vidParam->mb_cr_size = vidParam->mb_cr_size_x * vidParam->mb_cr_size_y;
  vidParam->mb_size_blk[0][0] = vidParam->mb_size_blk[0][1] = vidParam->mb_size[0][0] >> 2;
  vidParam->mb_size_blk[1][0] = vidParam->mb_size_blk[2][0] = vidParam->mb_size[1][0] >> 2;
  vidParam->mb_size_blk[1][1] = vidParam->mb_size_blk[2][1] = vidParam->mb_size[1][1] >> 2;

  vidParam->mb_size_shift[0][0] = vidParam->mb_size_shift[0][1] = ceilLog2sf (vidParam->mb_size[0][0]);
  vidParam->mb_size_shift[1][0] = vidParam->mb_size_shift[2][0] = ceilLog2sf (vidParam->mb_size[1][0]);
  vidParam->mb_size_shift[1][1] = vidParam->mb_size_shift[2][1] = ceilLog2sf (vidParam->mb_size[1][1]);
  }
//}}}

//{{{
sDataPartition* allocPartition (int n) {

  sDataPartition* partArr = (sDataPartition*)calloc (n, sizeof(sDataPartition));
  if (partArr == NULL) {
    snprintf (errortext, ET_SIZE, "allocPartition: Memory allocation for Data Partition failed");
    error (errortext, 100);
    }

  for (int i = 0; i < n; ++i) {// loop over all data partitions
    sDataPartition* dataPart = &(partArr[i]);
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

  return partArr;
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

  sCodingParam *codingParam = vidParam->codingParam[layerId];

  if (!vidParam->globalInitDone[layerId])
    return;

  // CAVLC free mem
  if (codingParam->nzCoeff) {
    free_mem4D(codingParam->nzCoeff);
    codingParam->nzCoeff = NULL;
    }

  // free mem, allocated for structure vidParam
  if ((codingParam->separate_colour_plane_flag != 0) ) {
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

  sCodingParam *codingParam = vidParam->codingParam[layerId];
  sBlockPos* picPos;

  if (vidParam->globalInitDone[layerId])
    freeLayerBuffers (vidParam, layerId);

  // allocate memory in structure vidParam
  if (codingParam->separate_colour_plane_flag != 0) {
    for (int i = 0; i<MAX_PLANE; ++i )
      if (((codingParam->mbDataJV[i]) = (sMacroblock*)calloc(codingParam->FrameSizeInMbs, sizeof(sMacroblock))) == NULL)
        no_mem_exit ("initGlobalBuffers: codingParam->mbDataJV");
    codingParam->mbData = NULL;
    }
  else if (((codingParam->mbData) = (sMacroblock*)calloc (codingParam->FrameSizeInMbs, sizeof(sMacroblock))) == NULL)
    no_mem_exit ("initGlobalBuffers: codingParam->mbData");

  if (codingParam->separate_colour_plane_flag != 0) {
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

  if( (codingParam->separate_colour_plane_flag != 0)) {
    for (int i = 0; i < MAX_PLANE; ++i )
      get_mem2D (&(codingParam->predModeJV[i]), 4*codingParam->FrameHeightInMbs, 4*codingParam->PicWidthInMbs);
    codingParam->predMode = NULL;
    }
  else
    get_mem2D (&(codingParam->predMode), 4*codingParam->FrameHeightInMbs, 4*codingParam->PicWidthInMbs);

  // CAVLC mem
  get_mem4D (&(codingParam->nzCoeff), codingParam->FrameSizeInMbs, 3, BLOCK_SIZE, BLOCK_SIZE);
  if (codingParam->separate_colour_plane_flag != 0) {
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
sDecodedPicList* getAvailableDecPic (sDecodedPicList* pDecPicList, int b3D, int view_id) {

  sDecodedPicList* pPic = pDecPicList;
  sDecodedPicList* pPrior = NULL;

  if (b3D) {
    while (pPic && (pPic->bValid &(1<<view_id))) {
      pPrior = pPic;
      pPic = pPic->next;
      }
    }
  else {
    while (pPic && (pPic->bValid)) {
      pPrior = pPic;
      pPic = pPic->next;
     }
    }

  if (!pPic) {
    pPic = (sDecodedPicList *)calloc(1, sizeof(*pPic));
    pPrior->next = pPic;
    }

  return pPic;
  }
//}}}
//{{{
void ClearDecPicList (sVidParam* vidParam) {

  // find the head first;
  sDecodedPicList* pPic = vidParam->decOutputPic, *pPrior = NULL;
  while (pPic && !pPic->bValid) {
    pPrior = pPic;
    pPic = pPic->next;
    }

  if (pPic && (pPic != vidParam->decOutputPic)) {
    //move all nodes before pPic to the end;
    sDecodedPicList *pPicTail = pPic;
    while (pPicTail->next)
      pPicTail = pPicTail->next;

    pPicTail->next = vidParam->decOutputPic;
    vidParam->decOutputPic = pPic;
    pPrior->next = NULL;
    }
  }
//}}}
//{{{
void freeDecPicList (sDecodedPicList* pDecPicList) {

  while (pDecPicList) {
    sDecodedPicList* pPicNext = pDecPicList->next;
    if (pDecPicList->pY) {
      free (pDecPicList->pY);
      pDecPicList->pY = NULL;
      pDecPicList->pU = NULL;
      pDecPicList->pV = NULL;
      }

    free (pDecPicList);
    pDecPicList = pPicNext;
   }
  }
//}}}

//{{{
void setGlobalCodingProgram (sVidParam* vidParam, sCodingParam* codingParam) {

  vidParam->bitdepth_chroma = 0;
  vidParam->widthCr = 0;
  vidParam->heightCr = 0;
  vidParam->lossless_qpprime_flag = codingParam->lossless_qpprime_flag;
  vidParam->max_vmv_r = codingParam->max_vmv_r;

  // Fidelity Range Extensions stuff (part 1)
  vidParam->bitdepth_luma = codingParam->bitdepth_luma;
  vidParam->bitdepth_scale[0] = codingParam->bitdepth_scale[0];
  vidParam->bitdepth_chroma = codingParam->bitdepth_chroma;
  vidParam->bitdepth_scale[1] = codingParam->bitdepth_scale[1];

  vidParam->max_frame_num = codingParam->max_frame_num;
  vidParam->PicWidthInMbs = codingParam->PicWidthInMbs;
  vidParam->PicHeightInMapUnits = codingParam->PicHeightInMapUnits;
  vidParam->FrameHeightInMbs = codingParam->FrameHeightInMbs;
  vidParam->FrameSizeInMbs = codingParam->FrameSizeInMbs;

  vidParam->yuvFormat = codingParam->yuvFormat;
  vidParam->separate_colour_plane_flag = codingParam->separate_colour_plane_flag;
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
int OpenDecoder (sInputParam* inputParam, byte* chunk, size_t chunkSize) {

  int iRet = allocDecoder (&gDecoder);
  if (iRet)
    return (iRet|DEC_ERRMASK);

  init_time();

  memcpy (gDecoder->inputParam, inputParam, sizeof(sInputParam));
  gDecoder->vidParam->concealMode = inputParam->concealMode;
  gDecoder->vidParam->ref_poc_gap = inputParam->ref_poc_gap;
  gDecoder->vidParam->pocGap = inputParam->pocGap;
  gDecoder->vidParam->annexB = allocAnnexB (gDecoder->vidParam);
  openAnnexB (gDecoder->vidParam->annexB, chunk, chunkSize);

  initOldSlice (gDecoder->vidParam->oldSlice);
  init (gDecoder->vidParam);
  allocOutput (gDecoder->vidParam);

  return DEC_OPEN_NOERR;
  }
//}}}
//{{{
int DecodeOneFrame (sDecodedPicList** ppDecPicList) {

  sDecoderParam* pDecoder = gDecoder;
  ClearDecPicList (gDecoder->vidParam);

  int iRet = decode_one_frame (gDecoder);
  if (iRet == SOP)
    iRet = DEC_SUCCEED;
  else if (iRet == EOS)
    iRet = DEC_EOS;
  else
    iRet |= DEC_ERRMASK;

  *ppDecPicList = gDecoder->vidParam->decOutputPic;
  return iRet;
  }
//}}}
//{{{
int FinitDecoder (sDecodedPicList** ppDecPicList) {

  ClearDecPicList (gDecoder->vidParam);
  flush_dpb (gDecoder->vidParam->dpbLayer[0]);

  #if (PAIR_FIELDS_IN_OUTPUT)
    flush_pending_output (gDecoder->vidParam);
  #endif

  resetAnnexB (gDecoder->vidParam->annexB);

  gDecoder->vidParam->newFrame = 0;
  gDecoder->vidParam->prevFrameNum = 0;
  *ppDecPicList = gDecoder->vidParam->decOutputPic;

  return DEC_GEN_NOERR;
  }
//}}}
//{{{
int CloseDecoder() {

  FmoFinit (gDecoder->vidParam);
  freeLayerBuffers (gDecoder->vidParam, 0);
  freeLayerBuffers (gDecoder->vidParam, 1);
  freeGlobalBuffers (gDecoder->vidParam);

  ercClose (gDecoder->vidParam, gDecoder->vidParam->ercErrorVar);

  cleanUpPPS (gDecoder->vidParam);

  for (unsigned i = 0; i < MAX_NUM_DPB_LAYERS; i++)
    freeDpb (gDecoder->vidParam->dpbLayer[i]);
  freeOutput (gDecoder->vidParam);
  freeImg (gDecoder->vidParam);
  free (gDecoder->inputParam);
  free (gDecoder);

  gDecoder = NULL;
  return DEC_CLOSE_NOERR;
  }
//}}}
