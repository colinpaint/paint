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

DecoderParams* gDecoder;
char errortext[ET_SIZE];

//{{{
void error (char* text, int code) {

  fprintf (stderr, "%s\n", text);
  if (gDecoder) {
    flush_dpb (gDecoder->pVid->p_Dpb_layer[0]);
    }

  exit (code);
  }
//}}}
//{{{
static void reset_dpb (VideoParameters* pVid, DecodedPictureBuffer* p_Dpb ) {

  p_Dpb->pVid = pVid;
  p_Dpb->init_done = 0;
  }
//}}}
//{{{
void report_stats_on_error() {

  //free_encoder_memory(pVid);
  exit (-1);
  }
//}}}

//{{{
static void alloc_video_params (VideoParameters** pVid) {

  if ((*pVid = (VideoParameters*)calloc (1, sizeof(VideoParameters)))==NULL)
    no_mem_exit ("alloc_video_params: pVid");

  if (((*pVid)->old_slice = (OldSliceParams*)calloc(1, sizeof(OldSliceParams)))==NULL)
    no_mem_exit ("alloc_video_params: pVid->old_slice");

  // Allocate new dpb buffer
  for (int i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
    if (((*pVid)->p_Dpb_layer[i] = (DecodedPictureBuffer*)calloc(1, sizeof(DecodedPictureBuffer)))==NULL)
      no_mem_exit ("alloc_video_params: pVid->p_Dpb_layer[i]");

    (*pVid)->p_Dpb_layer[i]->layer_id = i;
    reset_dpb(*pVid, (*pVid)->p_Dpb_layer[i]);
    if(((*pVid)->p_EncodePar[i] = (CodingParameters*)calloc(1, sizeof(CodingParameters))) == NULL)
      no_mem_exit ("alloc_video_params:pVid->p_EncodePar[i]");

    ((*pVid)->p_EncodePar[i])->layer_id = i;
    if(((*pVid)->p_LayerPar[i] = (LayerParameters*)calloc(1, sizeof(LayerParameters))) == NULL)
      no_mem_exit ("alloc_video_params:pVid->p_LayerPar[i]");
    ((*pVid)->p_LayerPar[i])->layer_id = i;
    }

  (*pVid)->global_init_done[0] = (*pVid)->global_init_done[1] = 0;

  if (((*pVid)->ppSliceList = (Slice**)calloc(MAX_NUM_DECSLICES, sizeof(Slice *))) == NULL)
    no_mem_exit ("alloc_video_params: pVid->ppSliceList");

  (*pVid)->iNumOfSlicesAllocated = MAX_NUM_DECSLICES;
  (*pVid)->pNextSlice = NULL;
  (*pVid)->nalu = allocNALU (MAX_CODED_FRAME_SIZE);
  (*pVid)->pDecOuputPic = (DecodedPicList*)calloc(1, sizeof(DecodedPicList));
  (*pVid)->pNextPPS = AllocPPS();
  (*pVid)->first_sps = TRUE;
}
//}}}
//{{{
static void alloc_params (InputParameters **p_Inp ) {
  if ((*p_Inp = (InputParameters*) calloc(1, sizeof(InputParameters)))==NULL)
    no_mem_exit ("alloc_params: p_Inp");
  }
//}}}
//{{{
static int alloc_decoder (DecoderParams** decoder) {

  if ((*decoder = (DecoderParams*)calloc(1, sizeof(DecoderParams)))==NULL) {
    fprintf (stderr, "alloc_decoder: gDecoder\n");
    return -1;
    }

  alloc_video_params (&((*decoder)->pVid));
  alloc_params (&((*decoder)->p_Inp));
  (*decoder)->pVid->p_Inp = (*decoder)->p_Inp;
  (*decoder)->bufferSize = 0;
  (*decoder)->bitcounter = 0;

  return 0;
  }
//}}}

//{{{
Slice* malloc_slice (InputParameters* p_Inp, VideoParameters* pVid) {

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
    currSlice->listX[i] = calloc(MAX_LIST_SIZE, sizeof (StorablePicture*)); // +1 for reordering
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
static void free_img (VideoParameters* pVid) {

  if (pVid != NULL) {
    freeAnnexB (&pVid->annex_b);

    // Free new dpb layers
    for (int i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
      if (pVid->p_Dpb_layer[i] != NULL) {
        free (pVid->p_Dpb_layer[i]);
        pVid->p_Dpb_layer[i] = NULL;
        }

      if (pVid->p_EncodePar[i]) {
        free (pVid->p_EncodePar[i]);
        pVid->p_EncodePar[i] = NULL;
        }

      if (pVid->p_LayerPar[i]) {
        free (pVid->p_LayerPar[i]);
        pVid->p_LayerPar[i] = NULL;
        }
      }

    if (pVid->old_slice != NULL) {
      free (pVid->old_slice);
      pVid->old_slice = NULL;
      }

    if (pVid->pNextSlice) {
      free_slice (pVid->pNextSlice);
      pVid->pNextSlice=NULL;
      }

    if (pVid->ppSliceList) {
      for (int i = 0; i < pVid->iNumOfSlicesAllocated; i++)
        if (pVid->ppSliceList[i])
          free_slice (pVid->ppSliceList[i]);
      free (pVid->ppSliceList);
      }

    if (pVid->nalu) {
      freeNALU (pVid->nalu);
      pVid->nalu=NULL;
      }

    //free memory;
    FreeDecPicList (pVid->pDecOuputPic);
    if (pVid->pNextPPS) {
      FreePPS (pVid->pNextPPS);
      pVid->pNextPPS = NULL;
      }

    free (pVid);
    pVid = NULL;
    }
  }
//}}}

//{{{
static void init (VideoParameters* pVid) {

  InputParameters *p_Inp = pVid->p_Inp;
  pVid->oldFrameSizeInMbs = (unsigned int) -1;

  pVid->recovery_point = 0;
  pVid->recovery_point_found = 0;
  pVid->recovery_poc = 0x7fffffff; /* set to a max value */

  pVid->idr_psnr_number = p_Inp->ref_offset;
  pVid->psnr_number=0;

  pVid->number = 0;
  pVid->type = I_SLICE;

  pVid->g_nFrame = 0;

  // time for total decoding session
  pVid->tot_time = 0;

  pVid->dec_picture = NULL;
  pVid->MbToSliceGroupMap = NULL;
  pVid->MapUnitToSliceGroupMap = NULL;

  pVid->LastAccessUnitExists  = 0;
  pVid->NALUCount = 0;

  pVid->out_buffer = NULL;
  pVid->pending_output = NULL;
  pVid->pending_output_state = FRAME;
  pVid->recovery_flag = 0;

  pVid->newframe = 0;
  pVid->previous_frame_num = 0;

  pVid->iLumaPadX = MCBUF_LUMA_PAD_X;
  pVid->iLumaPadY = MCBUF_LUMA_PAD_Y;
  pVid->iChromaPadX = MCBUF_CHROMA_PAD_X;
  pVid->iChromaPadY = MCBUF_CHROMA_PAD_Y;

  pVid->iPostProcess = 0;
  pVid->bDeblockEnable = 0x3;
  pVid->last_dec_view_id = -1;
  pVid->last_dec_layer_id = -1;
  }
//}}}
//{{{
void init_frext (VideoParameters* pVid) {

  //pel bitdepth init
  pVid->bitdepth_luma_qp_scale   = 6 * (pVid->bitdepth_luma - 8);

  if(pVid->bitdepth_luma > pVid->bitdepth_chroma || pVid->active_sps->chroma_format_idc == YUV400)
    pVid->pic_unit_bitsize_on_disk = (pVid->bitdepth_luma > 8)? 16:8;
  else
    pVid->pic_unit_bitsize_on_disk = (pVid->bitdepth_chroma > 8)? 16:8;
  pVid->dc_pred_value_comp[0]    = 1<<(pVid->bitdepth_luma - 1);
  pVid->max_pel_value_comp[0] = (1<<pVid->bitdepth_luma) - 1;
  pVid->mb_size[0][0] = pVid->mb_size[0][1] = MB_BLOCK_SIZE;

  if (pVid->active_sps->chroma_format_idc != YUV400) {
    //for chrominance part
    pVid->bitdepth_chroma_qp_scale = 6 * (pVid->bitdepth_chroma - 8);
    pVid->dc_pred_value_comp[1]    = (1 << (pVid->bitdepth_chroma - 1));
    pVid->dc_pred_value_comp[2]    = pVid->dc_pred_value_comp[1];
    pVid->max_pel_value_comp[1]    = (1 << pVid->bitdepth_chroma) - 1;
    pVid->max_pel_value_comp[2]    = (1 << pVid->bitdepth_chroma) - 1;
    pVid->num_blk8x8_uv = (1 << pVid->active_sps->chroma_format_idc) & (~(0x1));
    pVid->num_uv_blocks = (pVid->num_blk8x8_uv >> 1);
    pVid->num_cdc_coeff = (pVid->num_blk8x8_uv << 1);
    pVid->mb_size[1][0] = pVid->mb_size[2][0] = pVid->mb_cr_size_x  = (pVid->active_sps->chroma_format_idc==YUV420 || pVid->active_sps->chroma_format_idc==YUV422)?  8 : 16;
    pVid->mb_size[1][1] = pVid->mb_size[2][1] = pVid->mb_cr_size_y  = (pVid->active_sps->chroma_format_idc==YUV444 || pVid->active_sps->chroma_format_idc==YUV422)? 16 :  8;

    pVid->subpel_x    = pVid->mb_cr_size_x == 8 ? 7 : 3;
    pVid->subpel_y    = pVid->mb_cr_size_y == 8 ? 7 : 3;
    pVid->shiftpel_x  = pVid->mb_cr_size_x == 8 ? 3 : 2;
    pVid->shiftpel_y  = pVid->mb_cr_size_y == 8 ? 3 : 2;
    pVid->total_scale = pVid->shiftpel_x + pVid->shiftpel_y;
    }
  else {
    pVid->bitdepth_chroma_qp_scale = 0;
    pVid->max_pel_value_comp[1] = 0;
    pVid->max_pel_value_comp[2] = 0;
    pVid->num_blk8x8_uv = 0;
    pVid->num_uv_blocks = 0;
    pVid->num_cdc_coeff = 0;
    pVid->mb_size[1][0] = pVid->mb_size[2][0] = pVid->mb_cr_size_x  = 0;
    pVid->mb_size[1][1] = pVid->mb_size[2][1] = pVid->mb_cr_size_y  = 0;
    pVid->subpel_x      = 0;
    pVid->subpel_y      = 0;
    pVid->shiftpel_x    = 0;
    pVid->shiftpel_y    = 0;
    pVid->total_scale   = 0;
    }

  pVid->mb_cr_size = pVid->mb_cr_size_x * pVid->mb_cr_size_y;
  pVid->mb_size_blk[0][0] = pVid->mb_size_blk[0][1] = pVid->mb_size[0][0] >> 2;
  pVid->mb_size_blk[1][0] = pVid->mb_size_blk[2][0] = pVid->mb_size[1][0] >> 2;
  pVid->mb_size_blk[1][1] = pVid->mb_size_blk[2][1] = pVid->mb_size[1][1] >> 2;

  pVid->mb_size_shift[0][0] = pVid->mb_size_shift[0][1] = CeilLog2_sf (pVid->mb_size[0][0]);
  pVid->mb_size_shift[1][0] = pVid->mb_size_shift[2][0] = CeilLog2_sf (pVid->mb_size[1][0]);
  pVid->mb_size_shift[1][1] = pVid->mb_size_shift[2][1] = CeilLog2_sf (pVid->mb_size[1][1]);
  }
//}}}

//{{{
DataPartition* AllocPartition (int n) {

  DataPartition* partArr = (DataPartition*)calloc (n, sizeof(DataPartition));
  if (partArr == NULL) {
    snprintf (errortext, ET_SIZE, "AllocPartition: Memory allocation for Data Partition failed");
    error (errortext, 100);
    }

  for (int i = 0; i < n; ++i) {// loop over all data partitions
    DataPartition* dataPart = &(partArr[i]);
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
void FreePartition (DataPartition* dp, int n) {

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
void free_layer_buffers (VideoParameters* pVid, int layer_id) {

  CodingParameters *cps = pVid->p_EncodePar[layer_id];

  if (!pVid->global_init_done[layer_id])
    return;

  // CAVLC free mem
  if (cps->nz_coeff) {
    free_mem4D(cps->nz_coeff);
    cps->nz_coeff = NULL;
    }

  // free mem, allocated for structure pVid
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

  pVid->global_init_done[layer_id] = 0;
  }
//}}}
//{{{
int init_global_buffers (VideoParameters* pVid, int layer_id) {

  int memory_size = 0;
  CodingParameters *cps = pVid->p_EncodePar[layer_id];
  BlockPos* PicPos;

  if (pVid->global_init_done[layer_id])
    free_layer_buffers (pVid, layer_id);

  // allocate memory in structure pVid
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
        no_mem_exit ("init_global_buffers: pVid->siblock_JV");
      }
    cps->siblock = NULL;
    }
  else
    memory_size += get_mem2Dint (&(cps->siblock), cps->FrameHeightInMbs, cps->PicWidthInMbs);

  init_qp_process (cps);
  cps->oldFrameSizeInMbs = cps->FrameSizeInMbs;
  pVid->global_init_done[layer_id] = 1;

  return (memory_size);
  }
//}}}
//{{{
void free_global_buffers (VideoParameters* pVid) {

  if (pVid->dec_picture) {
    free_storable_picture (pVid->dec_picture);
    pVid->dec_picture = NULL;
    }
  }
//}}}

//{{{
DecodedPicList* get_one_avail_dec_pic_from_list (DecodedPicList* pDecPicList, int b3D, int view_id) {

  DecodedPicList* pPic = pDecPicList;
  DecodedPicList* pPrior = NULL;

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
    pPic = (DecodedPicList *)calloc(1, sizeof(*pPic));
    pPrior->pNext = pPic;
    }

  return pPic;
  }
//}}}
//{{{
void ClearDecPicList (VideoParameters* pVid) {

  // find the head first;
  DecodedPicList* pPic = pVid->pDecOuputPic, *pPrior = NULL;
  while (pPic && !pPic->bValid) {
    pPrior = pPic;
    pPic = pPic->pNext;
    }

  if (pPic && (pPic != pVid->pDecOuputPic)) {
    //move all nodes before pPic to the end;
    DecodedPicList *pPicTail = pPic;
    while (pPicTail->pNext)
      pPicTail = pPicTail->pNext;

    pPicTail->pNext = pVid->pDecOuputPic;
    pVid->pDecOuputPic = pPic;
    pPrior->pNext = NULL;
    }
  }
//}}}
//{{{
void FreeDecPicList (DecodedPicList* pDecPicList) {

  while (pDecPicList) {
    DecodedPicList* pPicNext = pDecPicList->pNext;
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
void set_global_coding_par (VideoParameters* pVid, CodingParameters* cps) {

  pVid->bitdepth_chroma = 0;
  pVid->width_cr = 0;
  pVid->height_cr = 0;
  pVid->lossless_qpprime_flag = cps->lossless_qpprime_flag;
  pVid->max_vmv_r = cps->max_vmv_r;

  // Fidelity Range Extensions stuff (part 1)
  pVid->bitdepth_luma = cps->bitdepth_luma;
  pVid->bitdepth_scale[0] = cps->bitdepth_scale[0];
  pVid->bitdepth_chroma = cps->bitdepth_chroma;
  pVid->bitdepth_scale[1] = cps->bitdepth_scale[1];

  pVid->max_frame_num = cps->max_frame_num;
  pVid->PicWidthInMbs = cps->PicWidthInMbs;
  pVid->PicHeightInMapUnits = cps->PicHeightInMapUnits;
  pVid->FrameHeightInMbs = cps->FrameHeightInMbs;
  pVid->FrameSizeInMbs = cps->FrameSizeInMbs;

  pVid->yuv_format = cps->yuv_format;
  pVid->separate_colour_plane_flag = cps->separate_colour_plane_flag;
  pVid->ChromaArrayType = cps->ChromaArrayType;

  pVid->width = cps->width;
  pVid->height = cps->height;
  pVid->iLumaPadX = MCBUF_LUMA_PAD_X;
  pVid->iLumaPadY = MCBUF_LUMA_PAD_Y;
  pVid->iChromaPadX = MCBUF_CHROMA_PAD_X;
  pVid->iChromaPadY = MCBUF_CHROMA_PAD_Y;

  if (pVid->yuv_format == YUV420) {
    pVid->width_cr = (pVid->width  >> 1);
    pVid->height_cr = (pVid->height >> 1);
    }
  else if (pVid->yuv_format == YUV422) {
    pVid->width_cr = (pVid->width >> 1);
    pVid->height_cr = pVid->height;
    pVid->iChromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
  else if (pVid->yuv_format == YUV444) {
    //YUV444
    pVid->width_cr = pVid->width;
    pVid->height_cr = pVid->height;
    pVid->iChromaPadX = pVid->iLumaPadX;
    pVid->iChromaPadY = pVid->iLumaPadY;
    }

  init_frext(pVid);
  }
//}}}

//{{{
int OpenDecoder (InputParameters* p_Inp, byte* chunk, size_t chunkSize) {

  int iRet = alloc_decoder (&gDecoder);
  if (iRet)
    return (iRet|DEC_ERRMASK);

  init_time();

  memcpy (gDecoder->p_Inp, p_Inp, sizeof(InputParameters));
  gDecoder->pVid->conceal_mode = p_Inp->conceal_mode;
  gDecoder->pVid->ref_poc_gap = p_Inp->ref_poc_gap;
  gDecoder->pVid->poc_gap = p_Inp->poc_gap;
  gDecoder->pVid->annex_b = allocAnnexB (gDecoder->pVid);
  openAnnexB (gDecoder->pVid->annex_b, chunk, chunkSize);

  init_old_slice (gDecoder->pVid->old_slice);
  init (gDecoder->pVid);
  init_out_buffer (gDecoder->pVid);

  return DEC_OPEN_NOERR;
  }
//}}}
//{{{
int DecodeOneFrame (DecodedPicList** ppDecPicList) {

  DecoderParams* pDecoder = gDecoder;
  ClearDecPicList (gDecoder->pVid);

  int iRet = decode_one_frame (gDecoder);
  if (iRet == SOP)
    iRet = DEC_SUCCEED;
  else if (iRet == EOS)
    iRet = DEC_EOS;
  else
    iRet |= DEC_ERRMASK;

  *ppDecPicList = gDecoder->pVid->pDecOuputPic;
  return iRet;
  }
//}}}
//{{{
int FinitDecoder (DecodedPicList** ppDecPicList) {

  ClearDecPicList (gDecoder->pVid);
  flush_dpb (gDecoder->pVid->p_Dpb_layer[0]);

  #if (PAIR_FIELDS_IN_OUTPUT)
    flush_pending_output (gDecoder->pVid);
  #endif

  resetAnnexB (gDecoder->pVid->annex_b);

  gDecoder->pVid->newframe = 0;
  gDecoder->pVid->previous_frame_num = 0;
  *ppDecPicList = gDecoder->pVid->pDecOuputPic;

  return DEC_GEN_NOERR;
  }
//}}}
//{{{
int CloseDecoder() {

  FmoFinit (gDecoder->pVid);
  free_layer_buffers (gDecoder->pVid, 0);
  free_layer_buffers (gDecoder->pVid, 1);
  free_global_buffers (gDecoder->pVid);

  ercClose (gDecoder->pVid, gDecoder->pVid->erc_errorVar);

  CleanUpPPS (gDecoder->pVid);

  for (unsigned i = 0; i < MAX_NUM_DPB_LAYERS; i++)
    free_dpb (gDecoder->pVid->p_Dpb_layer[i]);
  uninit_out_buffer (gDecoder->pVid);
  free_img (gDecoder->pVid);
  free (gDecoder->p_Inp);
  free (gDecoder);

  gDecoder = NULL;
  return DEC_CLOSE_NOERR;
  }
//}}}
