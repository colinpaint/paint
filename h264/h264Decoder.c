//{{{
/*!
 ***********************************************************************
 *  \mainpage
 *     This is the H.264/AVC decoder reference software. For detailed documentation
 *     see the comments in each file.
 *
 *     The JM software web site is located at:
 *     http://iphome.hhi.de/suehring/tml
 *
 *     For bug reporting and known issues see:
 *     https://ipbt.hhi.fraunhofer.de
 *
 *  \author
 *     The main contributors are listed in contributors.h
 *
 *  \version
 *     JM 18.4 (FRExt)
 *
 *  \note
 *     tags are used for document system "doxygen"
 *     available at http://www.doxygen.org
 */
/*!
 *  \file
 *     ldecod.c
 *  \brief
 *     H.264/AVC reference decoder project main()
 *  \author
 *     Main contributors (see contributors.h for copyright, address and affiliation details)
 *     - Inge Lille-Langøy       <inge.lille-langoy@telenor.com>
 *     - Rickard Sjoberg         <rickard.sjoberg@era.ericsson.se>
 *     - Stephan Wenger          <stewe@cs.tu-berlin.de>
 *     - Jani Lainema            <jani.lainema@nokia.com>
 *     - Sebastian Purreiter     <sebastian.purreiter@mch.siemens.de>
 *     - Byeong-Moon Jeon        <jeonbm@lge.com>
 *     - Gabi Blaettermann
 *     - Ye-Kui Wang             <wyk@ieee.org>
 *     - Valeri George
 *     - Karsten Suehring
 *
 ***********************************************************************
 */
//}}}
//{{{  includes
#include "global.h"
#include "memalloc.h"

#include "image.h"
#include "mcPrediction.h"
#include "mbuffer.h"
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
#include "h264decoder.h"
//}}}

sDecoderParams* gDecoder;
char errortext[ET_SIZE];

//{{{
void error (char* text, int code) {

  fprintf (stderr, "%s\n", text);
  if (gDecoder) {
    flush_dpb (gDecoder->vidParam->p_Dpb_layer[0]);
    }

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

  //free_encoder_memory(vidParam);
  exit (-1);
  }
//}}}

//{{{
static void alloc_video_params (sVidParam** vidParam) {

  if ((*vidParam = (sVidParam*)calloc (1, sizeof(sVidParam)))==NULL)
    no_mem_exit ("alloc_video_params: vidParam");

  if (((*vidParam)->old_slice = (OldSliceParams*)calloc(1, sizeof(OldSliceParams)))==NULL)
    no_mem_exit ("alloc_video_params: vidParam->old_slice");

  // Allocate new dpb buffer
  for (int i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
    if (((*vidParam)->p_Dpb_layer[i] = (sDPB*)calloc(1, sizeof(sDPB)))==NULL)
      no_mem_exit ("alloc_video_params: vidParam->p_Dpb_layer[i]");

    (*vidParam)->p_Dpb_layer[i]->layer_id = i;
    reset_dpb(*vidParam, (*vidParam)->p_Dpb_layer[i]);
    if(((*vidParam)->p_EncodePar[i] = (CodingParameters*)calloc(1, sizeof(CodingParameters))) == NULL)
      no_mem_exit ("alloc_video_params:vidParam->p_EncodePar[i]");

    ((*vidParam)->p_EncodePar[i])->layer_id = i;
    if(((*vidParam)->p_LayerPar[i] = (LayerParameters*)calloc(1, sizeof(LayerParameters))) == NULL)
      no_mem_exit ("alloc_video_params:vidParam->p_LayerPar[i]");
    ((*vidParam)->p_LayerPar[i])->layer_id = i;
    }

  (*vidParam)->global_init_done[0] = (*vidParam)->global_init_done[1] = 0;

  if (((*vidParam)->ppSliceList = (Slice**)calloc(MAX_NUM_DECSLICES, sizeof(Slice *))) == NULL)
    no_mem_exit ("alloc_video_params: vidParam->ppSliceList");

  (*vidParam)->iNumOfSlicesAllocated = MAX_NUM_DECSLICES;
  (*vidParam)->pNextSlice = NULL;
  (*vidParam)->nalu = allocNALU (MAX_CODED_FRAME_SIZE);
  (*vidParam)->pDecOuputPic = (sDecodedPicList*)calloc(1, sizeof(sDecodedPicList));
  (*vidParam)->pNextPPS = AllocPPS();
  (*vidParam)->first_sps = TRUE;
}
//}}}
//{{{
static void alloc_params (InputParameters **p_Inp ) {
  if ((*p_Inp = (InputParameters*) calloc(1, sizeof(InputParameters)))==NULL)
    no_mem_exit ("alloc_params: p_Inp");
  }
//}}}
//{{{
static int alloc_decoder (sDecoderParams** decoder) {

  if ((*decoder = (sDecoderParams*)calloc(1, sizeof(sDecoderParams)))==NULL) {
    fprintf (stderr, "alloc_decoder: gDecoder\n");
    return -1;
    }

  alloc_video_params (&((*decoder)->vidParam));
  alloc_params (&((*decoder)->p_Inp));
  (*decoder)->vidParam->p_Inp = (*decoder)->p_Inp;
  (*decoder)->bufferSize = 0;
  (*decoder)->bitcounter = 0;

  return 0;
  }
//}}}

//{{{
Slice* malloc_slice (InputParameters* p_Inp, sVidParam* vidParam) {

  int i, j, memory_size = 0;
  Slice *currSlice;

  currSlice = (Slice *) calloc(1, sizeof(Slice));
  if ( currSlice  == NULL) {
    snprintf(errortext, ET_SIZE, "Memory allocation for Slice datastruct in NAL-mode failed");
    error(errortext,100);
    }

  // create all context models
  currSlice->mot_ctx = create_contexts_MotionInfo();
  currSlice->tex_ctx = create_contexts_TextureInfo();

  currSlice->max_part_nr = 3;  //! assume data partitioning (worst case) for the following mallocs()
  currSlice->partArr = AllocPartition (currSlice->max_part_nr);

  memory_size += get_mem2Dwp (&(currSlice->wp_params), 2, MAX_REFERENCE_PICTURES);
  memory_size += get_mem3Dint(&(currSlice->wp_weight), 2, MAX_REFERENCE_PICTURES, 3);
  memory_size += get_mem3Dint(&(currSlice->wp_offset), 6, MAX_REFERENCE_PICTURES, 3);
  memory_size += get_mem4Dint(&(currSlice->wbp_weight), 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);
  memory_size += get_mem3Dpel(&(currSlice->mb_pred), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  memory_size += get_mem3Dpel(&(currSlice->mb_rec ), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  memory_size += get_mem3Dint(&(currSlice->mb_rres), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  memory_size += get_mem3Dint(&(currSlice->cof    ), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  //  memory_size += get_mem3Dint(&(currSlice->fcf    ), MAX_PLANE, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
  allocate_pred_mem(currSlice);

  // reference flag initialization
  for (i = 0; i < 17;++i)
    currSlice->ref_flag[i] = 1;

  for (i = 0; i < 6; i++) {
    currSlice->listX[i] = calloc(MAX_LIST_SIZE, sizeof (sPicture*)); // +1 for reordering
    if (NULL == currSlice->listX[i])
      no_mem_exit("malloc_slice: currSlice->listX[i]");
    }

  for (j = 0; j < 6; j++) {
    for (i = 0; i < MAX_LIST_SIZE; i++)
      currSlice->listX[j][i] = NULL;
    currSlice->listXsize[j]=0;
    }

  return currSlice;
  }
//}}}
//{{{
static void free_slice (Slice *currSlice) {

  if (currSlice->slice_type != I_SLICE && currSlice->slice_type != SI_SLICE)
    free_ref_pic_list_reordering_buffer (currSlice);

  free_pred_mem (currSlice);
  free_mem3Dint (currSlice->cof);
  free_mem3Dint (currSlice->mb_rres);
  free_mem3Dpel (currSlice->mb_rec);
  free_mem3Dpel (currSlice->mb_pred);

  free_mem2Dwp (currSlice->wp_params);
  free_mem3Dint (currSlice->wp_weight);
  free_mem3Dint (currSlice->wp_offset);
  free_mem4Dint (currSlice->wbp_weight);

  FreePartition (currSlice->partArr, 3);

  // delete all context models
  delete_contexts_MotionInfo (currSlice->mot_ctx);
  delete_contexts_TextureInfo (currSlice->tex_ctx);

  for (int i = 0; i<6; i++) {
    if (currSlice->listX[i]) {
      free (currSlice->listX[i]);
      currSlice->listX[i] = NULL;
      }
    }

  while (currSlice->dec_ref_pic_marking_buffer) {
    DecRefPicMarking_t* tmp_drpm = currSlice->dec_ref_pic_marking_buffer;
    currSlice->dec_ref_pic_marking_buffer = tmp_drpm->Next;
    free (tmp_drpm);
    }

  free (currSlice);
  currSlice = NULL;
  }
//}}}

//{{{
static void free_img (sVidParam* vidParam) {

  if (vidParam != NULL) {
    freeAnnexB (&vidParam->annex_b);

    // Free new dpb layers
    for (int i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
      if (vidParam->p_Dpb_layer[i] != NULL) {
        free (vidParam->p_Dpb_layer[i]);
        vidParam->p_Dpb_layer[i] = NULL;
        }

      if (vidParam->p_EncodePar[i]) {
        free (vidParam->p_EncodePar[i]);
        vidParam->p_EncodePar[i] = NULL;
        }

      if (vidParam->p_LayerPar[i]) {
        free (vidParam->p_LayerPar[i]);
        vidParam->p_LayerPar[i] = NULL;
        }
      }

    if (vidParam->old_slice != NULL) {
      free (vidParam->old_slice);
      vidParam->old_slice = NULL;
      }

    if (vidParam->pNextSlice) {
      free_slice (vidParam->pNextSlice);
      vidParam->pNextSlice=NULL;
      }

    if (vidParam->ppSliceList) {
      for (int i = 0; i < vidParam->iNumOfSlicesAllocated; i++)
        if (vidParam->ppSliceList[i])
          free_slice (vidParam->ppSliceList[i]);
      free (vidParam->ppSliceList);
      }

    if (vidParam->nalu) {
      freeNALU (vidParam->nalu);
      vidParam->nalu=NULL;
      }

    //free memory;
    FreeDecPicList (vidParam->pDecOuputPic);
    if (vidParam->pNextPPS) {
      FreePPS (vidParam->pNextPPS);
      vidParam->pNextPPS = NULL;
      }

    free (vidParam);
    vidParam = NULL;
    }
  }
//}}}

//{{{
static void init (sVidParam* vidParam) {

  InputParameters *p_Inp = vidParam->p_Inp;
  vidParam->oldFrameSizeInMbs = (unsigned int) -1;

  vidParam->recovery_point = 0;
  vidParam->recovery_point_found = 0;
  vidParam->recovery_poc = 0x7fffffff; /* set to a max value */

  vidParam->idr_psnr_number = p_Inp->ref_offset;
  vidParam->psnr_number=0;

  vidParam->number = 0;
  vidParam->type = I_SLICE;

  vidParam->g_nFrame = 0;

  // time for total decoding session
  vidParam->tot_time = 0;

  vidParam->dec_picture = NULL;
  vidParam->MbToSliceGroupMap = NULL;
  vidParam->MapUnitToSliceGroupMap = NULL;

  vidParam->LastAccessUnitExists  = 0;
  vidParam->NALUCount = 0;

  vidParam->out_buffer = NULL;
  vidParam->pending_output = NULL;
  vidParam->pending_output_state = FRAME;
  vidParam->recovery_flag = 0;

  vidParam->newframe = 0;
  vidParam->previous_frame_num = 0;

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

  //pel bitdepth init
  vidParam->bitdepth_luma_qp_scale   = 6 * (vidParam->bitdepth_luma - 8);

  if(vidParam->bitdepth_luma > vidParam->bitdepth_chroma || vidParam->active_sps->chroma_format_idc == YUV400)
    vidParam->pic_unit_bitsize_on_disk = (vidParam->bitdepth_luma > 8)? 16:8;
  else
    vidParam->pic_unit_bitsize_on_disk = (vidParam->bitdepth_chroma > 8)? 16:8;
  vidParam->dc_pred_value_comp[0]    = 1<<(vidParam->bitdepth_luma - 1);
  vidParam->max_pel_value_comp[0] = (1<<vidParam->bitdepth_luma) - 1;
  vidParam->mb_size[0][0] = vidParam->mb_size[0][1] = MB_BLOCK_SIZE;

  if (vidParam->active_sps->chroma_format_idc != YUV400) {
    //for chrominance part
    vidParam->bitdepth_chroma_qp_scale = 6 * (vidParam->bitdepth_chroma - 8);
    vidParam->dc_pred_value_comp[1]    = (1 << (vidParam->bitdepth_chroma - 1));
    vidParam->dc_pred_value_comp[2]    = vidParam->dc_pred_value_comp[1];
    vidParam->max_pel_value_comp[1]    = (1 << vidParam->bitdepth_chroma) - 1;
    vidParam->max_pel_value_comp[2]    = (1 << vidParam->bitdepth_chroma) - 1;
    vidParam->num_blk8x8_uv = (1 << vidParam->active_sps->chroma_format_idc) & (~(0x1));
    vidParam->num_uv_blocks = (vidParam->num_blk8x8_uv >> 1);
    vidParam->num_cdc_coeff = (vidParam->num_blk8x8_uv << 1);
    vidParam->mb_size[1][0] = vidParam->mb_size[2][0] = vidParam->mb_cr_size_x  = (vidParam->active_sps->chroma_format_idc==YUV420 || vidParam->active_sps->chroma_format_idc==YUV422)?  8 : 16;
    vidParam->mb_size[1][1] = vidParam->mb_size[2][1] = vidParam->mb_cr_size_y  = (vidParam->active_sps->chroma_format_idc==YUV444 || vidParam->active_sps->chroma_format_idc==YUV422)? 16 :  8;

    vidParam->subpel_x    = vidParam->mb_cr_size_x == 8 ? 7 : 3;
    vidParam->subpel_y    = vidParam->mb_cr_size_y == 8 ? 7 : 3;
    vidParam->shiftpel_x  = vidParam->mb_cr_size_x == 8 ? 3 : 2;
    vidParam->shiftpel_y  = vidParam->mb_cr_size_y == 8 ? 3 : 2;
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
    vidParam->subpel_x      = 0;
    vidParam->subpel_y      = 0;
    vidParam->shiftpel_x    = 0;
    vidParam->shiftpel_y    = 0;
    vidParam->total_scale   = 0;
    }

  vidParam->mb_cr_size = vidParam->mb_cr_size_x * vidParam->mb_cr_size_y;
  vidParam->mb_size_blk[0][0] = vidParam->mb_size_blk[0][1] = vidParam->mb_size[0][0] >> 2;
  vidParam->mb_size_blk[1][0] = vidParam->mb_size_blk[2][0] = vidParam->mb_size[1][0] >> 2;
  vidParam->mb_size_blk[1][1] = vidParam->mb_size_blk[2][1] = vidParam->mb_size[1][1] >> 2;

  vidParam->mb_size_shift[0][0] = vidParam->mb_size_shift[0][1] = CeilLog2_sf (vidParam->mb_size[0][0]);
  vidParam->mb_size_shift[1][0] = vidParam->mb_size_shift[2][0] = CeilLog2_sf (vidParam->mb_size[1][0]);
  vidParam->mb_size_shift[1][1] = vidParam->mb_size_shift[2][1] = CeilLog2_sf (vidParam->mb_size[1][1]);
  }
//}}}

//{{{
sDataPartition* AllocPartition (int n) {

  sDataPartition* partArr = (sDataPartition*)calloc (n, sizeof(sDataPartition));
  if (partArr == NULL) {
    snprintf (errortext, ET_SIZE, "AllocPartition: Memory allocation for Data Partition failed");
    error (errortext, 100);
    }

  for (int i = 0; i < n; ++i) {// loop over all data partitions
    sDataPartition* dataPart = &(partArr[i]);
    dataPart->bitstream = (Bitstream *) calloc(1, sizeof(Bitstream));
    if (dataPart->bitstream == NULL) {
      snprintf (errortext, ET_SIZE, "AllocPartition: Memory allocation for Bitstream failed");
      error (errortext, 100);
      }

    dataPart->bitstream->streamBuffer = (byte *) calloc(MAX_CODED_FRAME_SIZE, sizeof(byte));
    if (dataPart->bitstream->streamBuffer == NULL) {
      snprintf (errortext, ET_SIZE, "AllocPartition: Memory allocation for streamBuffer failed");
      error (errortext, 100);
      }
    }

  return partArr;
  }
//}}}
//{{{
void FreePartition (sDataPartition* dp, int n) {

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
void free_layer_buffers (sVidParam* vidParam, int layer_id) {

  CodingParameters *cps = vidParam->p_EncodePar[layer_id];

  if (!vidParam->global_init_done[layer_id])
    return;

  // CAVLC free mem
  if (cps->nz_coeff) {
    free_mem4D(cps->nz_coeff);
    cps->nz_coeff = NULL;
    }

  // free mem, allocated for structure vidParam
  if ((cps->separate_colour_plane_flag != 0) ) {
    for (int i = 0; i<MAX_PLANE; i++) {
      free (cps->mb_data_JV[i]);
      cps->mb_data_JV[i] = NULL;
      free_mem2Dint(cps->siblock_JV[i]);
      cps->siblock_JV[i] = NULL;
      free_mem2D(cps->ipredmode_JV[i]);
      cps->ipredmode_JV[i] = NULL;
      free (cps->intra_block_JV[i]);
      cps->intra_block_JV[i] = NULL;
      }
    }
  else {
    if (cps->mb_data != NULL) {
      free(cps->mb_data);
      cps->mb_data = NULL;
      }
    if (cps->siblock) {
      free_mem2Dint(cps->siblock);
      cps->siblock = NULL;
      }
    if (cps->ipredmode) {
      free_mem2D(cps->ipredmode);
      cps->ipredmode = NULL;
      }
    if (cps->intra_block) {
      free (cps->intra_block);
      cps->intra_block = NULL;
      }
    }

  if (cps->PicPos) {
    free(cps->PicPos);
    cps->PicPos = NULL;
    }

  free_qp_matrices(cps);

  vidParam->global_init_done[layer_id] = 0;
  }
//}}}
//{{{
int init_global_buffers (sVidParam* vidParam, int layer_id) {

  int memory_size = 0;
  CodingParameters *cps = vidParam->p_EncodePar[layer_id];
  BlockPos* PicPos;

  if (vidParam->global_init_done[layer_id])
    free_layer_buffers (vidParam, layer_id);

  // allocate memory in structure vidParam
  if ((cps->separate_colour_plane_flag != 0) ) {
    for (int i = 0; i<MAX_PLANE; ++i )
      if (((cps->mb_data_JV[i]) = (Macroblock*)calloc(cps->FrameSizeInMbs, sizeof(Macroblock))) == NULL)
        no_mem_exit ("init_global_buffers: cps->mb_data_JV");
    cps->mb_data = NULL;
    }
  else if (((cps->mb_data) = (Macroblock*)calloc (cps->FrameSizeInMbs, sizeof(Macroblock))) == NULL)
    no_mem_exit ("init_global_buffers: cps->mb_data");

  if ((cps->separate_colour_plane_flag != 0) ) {
    for (int i = 0; i < MAX_PLANE; ++i )
      if (((cps->intra_block_JV[i]) = (char*) calloc(cps->FrameSizeInMbs, sizeof(char))) == NULL)
        no_mem_exit ("init_global_buffers: cps->intra_block_JV");
    cps->intra_block = NULL;
    }
  else if (((cps->intra_block) = (char*)calloc (cps->FrameSizeInMbs, sizeof(char))) == NULL)
    no_mem_exit ("init_global_buffers: cps->intra_block");

  if (((cps->PicPos) = (BlockPos*)calloc(cps->FrameSizeInMbs + 1, sizeof(BlockPos))) == NULL)
    no_mem_exit ("init_global_buffers: PicPos");

  PicPos = cps->PicPos;
  for (int i = 0; i < (int) cps->FrameSizeInMbs + 1;++i) {
    PicPos[i].x = (short)(i % cps->PicWidthInMbs);
    PicPos[i].y = (short)(i / cps->PicWidthInMbs);
    }

  if( (cps->separate_colour_plane_flag != 0)) {
    for (int i = 0; i < MAX_PLANE; ++i )
      get_mem2D (&(cps->ipredmode_JV[i]), 4*cps->FrameHeightInMbs, 4*cps->PicWidthInMbs);
    cps->ipredmode = NULL;
    }
  else
    memory_size += get_mem2D (&(cps->ipredmode), 4*cps->FrameHeightInMbs, 4*cps->PicWidthInMbs);

  // CAVLC mem
  memory_size += get_mem4D (&(cps->nz_coeff), cps->FrameSizeInMbs, 3, BLOCK_SIZE, BLOCK_SIZE);
  if ((cps->separate_colour_plane_flag != 0)) {
    for (int i = 0; i < MAX_PLANE; ++i ) {
      get_mem2Dint (&(cps->siblock_JV[i]), cps->FrameHeightInMbs, cps->PicWidthInMbs);
      if (cps->siblock_JV[i] == NULL)
        no_mem_exit ("init_global_buffers: vidParam->siblock_JV");
      }
    cps->siblock = NULL;
    }
  else
    memory_size += get_mem2Dint (&(cps->siblock), cps->FrameHeightInMbs, cps->PicWidthInMbs);

  init_qp_process (cps);
  cps->oldFrameSizeInMbs = cps->FrameSizeInMbs;
  vidParam->global_init_done[layer_id] = 1;

  return (memory_size);
  }
//}}}
//{{{
void free_global_buffers (sVidParam* vidParam) {

  if (vidParam->dec_picture) {
    free_storable_picture (vidParam->dec_picture);
    vidParam->dec_picture = NULL;
    }
  }
//}}}

//{{{
sDecodedPicList* get_one_avail_dec_pic_from_list (sDecodedPicList* pDecPicList, int b3D, int view_id) {

  sDecodedPicList* pPic = pDecPicList;
  sDecodedPicList* pPrior = NULL;

  if (b3D) {
    while (pPic && (pPic->bValid &(1<<view_id))) {
      pPrior = pPic;
      pPic = pPic->pNext;
      }
    }
  else {
    while (pPic && (pPic->bValid)) {
      pPrior = pPic;
      pPic = pPic->pNext;
     }
    }

  if (!pPic) {
    pPic = (sDecodedPicList *)calloc(1, sizeof(*pPic));
    pPrior->pNext = pPic;
    }

  return pPic;
  }
//}}}
//{{{
void ClearDecPicList (sVidParam* vidParam) {

  // find the head first;
  sDecodedPicList* pPic = vidParam->pDecOuputPic, *pPrior = NULL;
  while (pPic && !pPic->bValid) {
    pPrior = pPic;
    pPic = pPic->pNext;
    }

  if (pPic && (pPic != vidParam->pDecOuputPic)) {
    //move all nodes before pPic to the end;
    sDecodedPicList *pPicTail = pPic;
    while (pPicTail->pNext)
      pPicTail = pPicTail->pNext;

    pPicTail->pNext = vidParam->pDecOuputPic;
    vidParam->pDecOuputPic = pPic;
    pPrior->pNext = NULL;
    }
  }
//}}}
//{{{
void FreeDecPicList (sDecodedPicList* pDecPicList) {

  while (pDecPicList) {
    sDecodedPicList* pPicNext = pDecPicList->pNext;
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
void set_global_coding_par (sVidParam* vidParam, CodingParameters* cps) {

  vidParam->bitdepth_chroma = 0;
  vidParam->width_cr = 0;
  vidParam->height_cr = 0;
  vidParam->lossless_qpprime_flag = cps->lossless_qpprime_flag;
  vidParam->max_vmv_r = cps->max_vmv_r;

  // Fidelity Range Extensions stuff (part 1)
  vidParam->bitdepth_luma = cps->bitdepth_luma;
  vidParam->bitdepth_scale[0] = cps->bitdepth_scale[0];
  vidParam->bitdepth_chroma = cps->bitdepth_chroma;
  vidParam->bitdepth_scale[1] = cps->bitdepth_scale[1];

  vidParam->max_frame_num = cps->max_frame_num;
  vidParam->PicWidthInMbs = cps->PicWidthInMbs;
  vidParam->PicHeightInMapUnits = cps->PicHeightInMapUnits;
  vidParam->FrameHeightInMbs = cps->FrameHeightInMbs;
  vidParam->FrameSizeInMbs = cps->FrameSizeInMbs;

  vidParam->yuv_format = cps->yuv_format;
  vidParam->separate_colour_plane_flag = cps->separate_colour_plane_flag;
  vidParam->ChromaArrayType = cps->ChromaArrayType;

  vidParam->width = cps->width;
  vidParam->height = cps->height;
  vidParam->iLumaPadX = MCBUF_LUMA_PAD_X;
  vidParam->iLumaPadY = MCBUF_LUMA_PAD_Y;
  vidParam->iChromaPadX = MCBUF_CHROMA_PAD_X;
  vidParam->iChromaPadY = MCBUF_CHROMA_PAD_Y;

  if (vidParam->yuv_format == YUV420) {
    vidParam->width_cr = (vidParam->width  >> 1);
    vidParam->height_cr = (vidParam->height >> 1);
    }
  else if (vidParam->yuv_format == YUV422) {
    vidParam->width_cr = (vidParam->width >> 1);
    vidParam->height_cr = vidParam->height;
    vidParam->iChromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (vidParam->yuv_format == YUV444) {
    //YUV444
    vidParam->width_cr = vidParam->width;
    vidParam->height_cr = vidParam->height;
    vidParam->iChromaPadX = vidParam->iLumaPadX;
    vidParam->iChromaPadY = vidParam->iLumaPadY;
    }

  init_frext(vidParam);
  }
//}}}

//{{{
int OpenDecoder (InputParameters* p_Inp, byte* chunk, size_t chunkSize) {

  int iRet = alloc_decoder (&gDecoder);
  if (iRet)
    return (iRet|DEC_ERRMASK);

  init_time();

  memcpy (gDecoder->p_Inp, p_Inp, sizeof(InputParameters));
  gDecoder->vidParam->conceal_mode = p_Inp->conceal_mode;
  gDecoder->vidParam->ref_poc_gap = p_Inp->ref_poc_gap;
  gDecoder->vidParam->poc_gap = p_Inp->poc_gap;
  gDecoder->vidParam->annex_b = allocAnnexB (gDecoder->vidParam);
  openAnnexB (gDecoder->vidParam->annex_b, chunk, chunkSize);

  init_old_slice (gDecoder->vidParam->old_slice);
  init (gDecoder->vidParam);
  init_out_buffer (gDecoder->vidParam);

  return DEC_OPEN_NOERR;
  }
//}}}
//{{{
int DecodeOneFrame (sDecodedPicList** ppDecPicList) {

  sDecoderParams* pDecoder = gDecoder;
  ClearDecPicList (gDecoder->vidParam);

  int iRet = decode_one_frame (gDecoder);
  if (iRet == SOP)
    iRet = DEC_SUCCEED;
  else if (iRet == EOS)
    iRet = DEC_EOS;
  else
    iRet |= DEC_ERRMASK;

  *ppDecPicList = gDecoder->vidParam->pDecOuputPic;
  return iRet;
  }
//}}}
//{{{
int FinitDecoder (sDecodedPicList** ppDecPicList) {

  ClearDecPicList (gDecoder->vidParam);
  flush_dpb (gDecoder->vidParam->p_Dpb_layer[0]);

  #if (PAIR_FIELDS_IN_OUTPUT)
    flush_pending_output (gDecoder->vidParam);
  #endif

  resetAnnexB (gDecoder->vidParam->annex_b);

  gDecoder->vidParam->newframe = 0;
  gDecoder->vidParam->previous_frame_num = 0;
  *ppDecPicList = gDecoder->vidParam->pDecOuputPic;

  return DEC_GEN_NOERR;
  }
//}}}
//{{{
int CloseDecoder() {

  FmoFinit (gDecoder->vidParam);
  free_layer_buffers (gDecoder->vidParam, 0);
  free_layer_buffers (gDecoder->vidParam, 1);
  free_global_buffers (gDecoder->vidParam);

  ercClose (gDecoder->vidParam, gDecoder->vidParam->erc_errorVar);

  CleanUpPPS (gDecoder->vidParam);

  for (unsigned i = 0; i < MAX_NUM_DPB_LAYERS; i++)
    free_dpb (gDecoder->vidParam->p_Dpb_layer[i]);
  uninit_out_buffer (gDecoder->vidParam);
  free_img (gDecoder->vidParam);
  free (gDecoder->p_Inp);
  free (gDecoder);

  gDecoder = NULL;
  return DEC_CLOSE_NOERR;
  }
//}}}
