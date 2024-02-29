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

#include "annexb.h"
#include "image.h"
#include "memalloc.h"
#include "mc_prediction.h"
#include "mbuffer.h"
#include "fmo.h"
#include "output.h"
#include "cabac.h"
#include "parset.h"
#include "sei.h"
#include "erc_api.h"
#include "quant.h"
#include "block.h"
#include "nalu.h"
#include "loopfilter.h"
//#include "rtp.h"
#include "output.h"
#include "h264decoder.h"

#define LOGFILE     "log.dec"
#define DATADECFILE "dataDec.txt"
#define TRACEFILE   "trace_dec.txt"
//}}}

DecoderParams* p_Dec;
char errortext[ET_SIZE];

static void free_slice (Slice* currSlic);
//{{{
void error (char* text, int code)
{
  fprintf(stderr, "%s\n", text);
  if (p_Dec)
  {
    flush_dpb(p_Dec->p_Vid->p_Dpb_layer[0]);
#if (MVC_EXTENSION_ENABLE)
    flush_dpb(p_Dec->p_Vid->p_Dpb_layer[1]);
#endif
  }

  exit(code);
}
//}}}
//{{{
static void reset_dpb (VideoParameters* p_Vid, DecodedPictureBuffer* p_Dpb )
{
  p_Dpb->p_Vid = p_Vid;
  p_Dpb->init_done = 0;
}
//}}}
//{{{
void report_stats_on_error() {

  //free_encoder_memory(p_Vid);
  exit (-1);
  }
//}}}

//{{{
static void alloc_video_params (VideoParameters** p_Vid) {

  int i;
  if ((*p_Vid = (VideoParameters *)calloc(1, sizeof(VideoParameters)))==NULL)
    no_mem_exit ("alloc_video_params: p_Vid");

  if (((*p_Vid)->old_slice = (OldSliceParams *) calloc(1, sizeof(OldSliceParams)))==NULL)
    no_mem_exit ("alloc_video_params: p_Vid->old_slice");

  // Allocate new dpb buffer
  for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
    if (((*p_Vid)->p_Dpb_layer[i] =  (DecodedPictureBuffer*)calloc(1, sizeof(DecodedPictureBuffer)))==NULL)
      no_mem_exit ("alloc_video_params: p_Vid->p_Dpb_layer[i]");
    (*p_Vid)->p_Dpb_layer[i]->layer_id = i;
    reset_dpb(*p_Vid, (*p_Vid)->p_Dpb_layer[i]);
    if(((*p_Vid)->p_EncodePar[i] = (CodingParameters *)calloc(1, sizeof(CodingParameters))) == NULL)
      no_mem_exit ("alloc_video_params:p_Vid->p_EncodePar[i]");
    ((*p_Vid)->p_EncodePar[i])->layer_id = i;
    if(((*p_Vid)->p_LayerPar[i] = (LayerParameters *)calloc(1, sizeof(LayerParameters))) == NULL)
      no_mem_exit ("alloc_video_params:p_Vid->p_LayerPar[i]");
    ((*p_Vid)->p_LayerPar[i])->layer_id = i;
    }
  (*p_Vid)->global_init_done[0] = (*p_Vid)->global_init_done[1] = 0;

  if (((*p_Vid)->ppSliceList = (Slice **) calloc(MAX_NUM_DECSLICES, sizeof(Slice *))) == NULL)
    no_mem_exit ("alloc_video_params: p_Vid->ppSliceList");
  (*p_Vid)->iNumOfSlicesAllocated = MAX_NUM_DECSLICES;
  //(*p_Vid)->currentSlice = NULL;
  (*p_Vid)->pNextSlice = NULL;
  (*p_Vid)->nalu = AllocNALU(MAX_CODED_FRAME_SIZE);
  (*p_Vid)->pDecOuputPic = (DecodedPicList*)calloc(1, sizeof(DecodedPicList));
  (*p_Vid)->pNextPPS = AllocPPS();
  (*p_Vid)->first_sps = TRUE;
}
//}}}
//{{{
static void alloc_params (InputParameters **p_Inp ) {
  if ((*p_Inp = (InputParameters*) calloc(1, sizeof(InputParameters)))==NULL)
    no_mem_exit ("alloc_params: p_Inp");
  }
//}}}
//{{{
static int alloc_decoder (DecoderParams** p_Dec) {

  if ((*p_Dec = (DecoderParams*)calloc(1, sizeof(DecoderParams)))==NULL) {
    fprintf (stderr, "alloc_decoder: p_Dec\n");
    return -1;
    }

  alloc_video_params (&((*p_Dec)->p_Vid));
  alloc_params (&((*p_Dec)->p_Inp));
  (*p_Dec)->p_Vid->p_Inp = (*p_Dec)->p_Inp;
  (*p_Dec)->bufferSize = 0;
  (*p_Dec)->bitcounter = 0;

  return 0;
  }
//}}}

//{{{
static void free_img (VideoParameters* p_Vid) {

  int i;
  if (p_Vid != NULL) {
    if ( p_Vid->p_Inp->FileFormat == PAR_OF_ANNEXB )
      free_annex_b (&p_Vid->annex_b);

    // Free new dpb layers
    for (i = 0; i < MAX_NUM_DPB_LAYERS; i++) {
      if (p_Vid->p_Dpb_layer[i] != NULL) {
        free (p_Vid->p_Dpb_layer[i]);
        p_Vid->p_Dpb_layer[i] = NULL;
        }
      if(p_Vid->p_EncodePar[i]) {
        free(p_Vid->p_EncodePar[i]);
        p_Vid->p_EncodePar[i] = NULL;
        }
      if(p_Vid->p_LayerPar[i]) {
        free(p_Vid->p_LayerPar[i]);
        p_Vid->p_LayerPar[i] = NULL;
        }
      }
    if (p_Vid->old_slice != NULL) {
      free (p_Vid->old_slice);
      p_Vid->old_slice = NULL;
      }

    if(p_Vid->pNextSlice) {
      free_slice(p_Vid->pNextSlice);
      p_Vid->pNextSlice=NULL;
      }
    if(p_Vid->ppSliceList) {
      int i;
      for(i=0; i<p_Vid->iNumOfSlicesAllocated; i++)
        if(p_Vid->ppSliceList[i])
          free_slice(p_Vid->ppSliceList[i]);
      free(p_Vid->ppSliceList);
      }
    if(p_Vid->nalu) {
      FreeNALU(p_Vid->nalu);
      p_Vid->nalu=NULL;
      }
    //free memory;
    FreeDecPicList(p_Vid->pDecOuputPic);
    if(p_Vid->pNextPPS) {
      FreePPS(p_Vid->pNextPPS);
      p_Vid->pNextPPS = NULL;
      }

    free (p_Vid);
    p_Vid = NULL;
    }
  }
//}}}
//{{{
void FreeDecPicList (DecodedPicList* pDecPicList) {

  while (pDecPicList) {
    DecodedPicList* pPicNext = pDecPicList->pNext;
    if (pDecPicList->pY) {
      free(pDecPicList->pY);
      pDecPicList->pY = NULL;
      pDecPicList->pU = NULL;
      pDecPicList->pV = NULL;
      }

    free(pDecPicList);
    pDecPicList = pPicNext;
   }
  }
//}}}
//{{{
static void init (VideoParameters* p_Vid) {

  //int i;
  InputParameters *p_Inp = p_Vid->p_Inp;
  p_Vid->oldFrameSizeInMbs = (unsigned int) -1;

  p_Vid->recovery_point = 0;
  p_Vid->recovery_point_found = 0;
  p_Vid->recovery_poc = 0x7fffffff; /* set to a max value */

  p_Vid->idr_psnr_number = p_Inp->ref_offset;
  p_Vid->psnr_number=0;

  p_Vid->number = 0;
  p_Vid->type = I_SLICE;

  p_Vid->g_nFrame = 0;

  // time for total decoding session
  p_Vid->tot_time = 0;

  p_Vid->dec_picture = NULL;
  p_Vid->MbToSliceGroupMap = NULL;
  p_Vid->MapUnitToSliceGroupMap = NULL;

  p_Vid->LastAccessUnitExists  = 0;
  p_Vid->NALUCount = 0;

  p_Vid->out_buffer = NULL;
  p_Vid->pending_output = NULL;
  p_Vid->pending_output_state = FRAME;
  p_Vid->recovery_flag = 0;

#if (MVC_EXTENSION_ENABLE)
  p_Vid->last_pic_width_in_mbs_minus1 = 0;
  p_Vid->last_pic_height_in_map_units_minus1 = 0;
  p_Vid->last_max_dec_frame_buffering = 0;
#endif

  p_Vid->newframe = 0;
  p_Vid->previous_frame_num = 0;

  p_Vid->iLumaPadX = MCBUF_LUMA_PAD_X;
  p_Vid->iLumaPadY = MCBUF_LUMA_PAD_Y;
  p_Vid->iChromaPadX = MCBUF_CHROMA_PAD_X;
  p_Vid->iChromaPadY = MCBUF_CHROMA_PAD_Y;

  p_Vid->iPostProcess = 0;
  p_Vid->bDeblockEnable = 0x3;
  p_Vid->last_dec_view_id = -1;
  p_Vid->last_dec_layer_id = -1;
  }
//}}}
//{{{
void init_frext (VideoParameters* p_Vid) {

  //pel bitdepth init
  p_Vid->bitdepth_luma_qp_scale   = 6 * (p_Vid->bitdepth_luma - 8);

  if(p_Vid->bitdepth_luma > p_Vid->bitdepth_chroma || p_Vid->active_sps->chroma_format_idc == YUV400)
    p_Vid->pic_unit_bitsize_on_disk = (p_Vid->bitdepth_luma > 8)? 16:8;
  else
    p_Vid->pic_unit_bitsize_on_disk = (p_Vid->bitdepth_chroma > 8)? 16:8;
  p_Vid->dc_pred_value_comp[0]    = 1<<(p_Vid->bitdepth_luma - 1);
  p_Vid->max_pel_value_comp[0] = (1<<p_Vid->bitdepth_luma) - 1;
  p_Vid->mb_size[0][0] = p_Vid->mb_size[0][1] = MB_BLOCK_SIZE;

  if (p_Vid->active_sps->chroma_format_idc != YUV400) {
    //for chrominance part
    p_Vid->bitdepth_chroma_qp_scale = 6 * (p_Vid->bitdepth_chroma - 8);
    p_Vid->dc_pred_value_comp[1]    = (1 << (p_Vid->bitdepth_chroma - 1));
    p_Vid->dc_pred_value_comp[2]    = p_Vid->dc_pred_value_comp[1];
    p_Vid->max_pel_value_comp[1]    = (1 << p_Vid->bitdepth_chroma) - 1;
    p_Vid->max_pel_value_comp[2]    = (1 << p_Vid->bitdepth_chroma) - 1;
    p_Vid->num_blk8x8_uv = (1 << p_Vid->active_sps->chroma_format_idc) & (~(0x1));
    p_Vid->num_uv_blocks = (p_Vid->num_blk8x8_uv >> 1);
    p_Vid->num_cdc_coeff = (p_Vid->num_blk8x8_uv << 1);
    p_Vid->mb_size[1][0] = p_Vid->mb_size[2][0] = p_Vid->mb_cr_size_x  = (p_Vid->active_sps->chroma_format_idc==YUV420 || p_Vid->active_sps->chroma_format_idc==YUV422)?  8 : 16;
    p_Vid->mb_size[1][1] = p_Vid->mb_size[2][1] = p_Vid->mb_cr_size_y  = (p_Vid->active_sps->chroma_format_idc==YUV444 || p_Vid->active_sps->chroma_format_idc==YUV422)? 16 :  8;

    p_Vid->subpel_x    = p_Vid->mb_cr_size_x == 8 ? 7 : 3;
    p_Vid->subpel_y    = p_Vid->mb_cr_size_y == 8 ? 7 : 3;
    p_Vid->shiftpel_x  = p_Vid->mb_cr_size_x == 8 ? 3 : 2;
    p_Vid->shiftpel_y  = p_Vid->mb_cr_size_y == 8 ? 3 : 2;
    p_Vid->total_scale = p_Vid->shiftpel_x + p_Vid->shiftpel_y;
    }
  else {
    p_Vid->bitdepth_chroma_qp_scale = 0;
    p_Vid->max_pel_value_comp[1] = 0;
    p_Vid->max_pel_value_comp[2] = 0;
    p_Vid->num_blk8x8_uv = 0;
    p_Vid->num_uv_blocks = 0;
    p_Vid->num_cdc_coeff = 0;
    p_Vid->mb_size[1][0] = p_Vid->mb_size[2][0] = p_Vid->mb_cr_size_x  = 0;
    p_Vid->mb_size[1][1] = p_Vid->mb_size[2][1] = p_Vid->mb_cr_size_y  = 0;
    p_Vid->subpel_x      = 0;
    p_Vid->subpel_y      = 0;
    p_Vid->shiftpel_x    = 0;
    p_Vid->shiftpel_y    = 0;
    p_Vid->total_scale   = 0;
    }

  p_Vid->mb_cr_size = p_Vid->mb_cr_size_x * p_Vid->mb_cr_size_y;
  p_Vid->mb_size_blk[0][0] = p_Vid->mb_size_blk[0][1] = p_Vid->mb_size[0][0] >> 2;
  p_Vid->mb_size_blk[1][0] = p_Vid->mb_size_blk[2][0] = p_Vid->mb_size[1][0] >> 2;
  p_Vid->mb_size_blk[1][1] = p_Vid->mb_size_blk[2][1] = p_Vid->mb_size[1][1] >> 2;

  p_Vid->mb_size_shift[0][0] = p_Vid->mb_size_shift[0][1] = CeilLog2_sf (p_Vid->mb_size[0][0]);
  p_Vid->mb_size_shift[1][0] = p_Vid->mb_size_shift[2][0] = CeilLog2_sf (p_Vid->mb_size[1][0]);
  p_Vid->mb_size_shift[1][1] = p_Vid->mb_size_shift[2][1] = CeilLog2_sf (p_Vid->mb_size[1][1]);
  }
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    Allocates a stand-alone partition structure.  Structure should
 *    be freed by FreePartition();
 *    data structures
 *
 * \par Input:
 *    n: number of partitions in the array
 * \par return
 *    pointer to DataPartition Structure, zero-initialized
 ************************************************************************
 */

DataPartition* AllocPartition (int n) {

  DataPartition *partArr, *dataPart;
  int i;

  partArr = (DataPartition *) calloc(n, sizeof(DataPartition));
  if (partArr == NULL) {
    snprintf(errortext, ET_SIZE, "AllocPartition: Memory allocation for Data Partition failed");
    error(errortext, 100);
  }

  for (i = 0; i < n; ++i) {// loop over all data partitions
    dataPart = &(partArr[i]);
    dataPart->bitstream = (Bitstream *) calloc(1, sizeof(Bitstream));
    if (dataPart->bitstream == NULL) {
      snprintf(errortext, ET_SIZE, "AllocPartition: Memory allocation for Bitstream failed");
      error(errortext, 100);
    }
    dataPart->bitstream->streamBuffer = (byte *) calloc(MAX_CODED_FRAME_SIZE, sizeof(byte));
    if (dataPart->bitstream->streamBuffer == NULL) {
      snprintf(errortext, ET_SIZE, "AllocPartition: Memory allocation for streamBuffer failed");
      error(errortext, 100);
    }
  }
  return partArr;
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Frees a partition structure (array).
 *
 * \par Input:
 *    Partition to be freed, size of partition Array (Number of Partitions)
 *
 * \par return
 *    None
 *
 * \note
 *    n must be the same as for the corresponding call of AllocPartition
 ************************************************************************
 */
void FreePartition (DataPartition* dp, int n)
{
  int i;

  assert (dp != NULL);
  assert (dp->bitstream != NULL);
  assert (dp->bitstream->streamBuffer != NULL);
  for (i=0; i<n; ++i) {
    free (dp[i].bitstream->streamBuffer);
    free (dp[i].bitstream);
    }

  free (dp);
  }
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    Allocates the slice structure along with its dependent
 *    data structures
 *
 * \par Input:
 *    Input Parameters InputParameters *p_Inp,  VideoParameters *p_Vid
 ************************************************************************
 */
Slice* malloc_slice (InputParameters* p_Inp, VideoParameters* p_Vid) {

  int i, j, memory_size = 0;
  Slice *currSlice;

  currSlice = (Slice *) calloc(1, sizeof(Slice));
  if ( currSlice  == NULL) {
    snprintf(errortext, ET_SIZE, "Memory allocation for Slice datastruct in NAL-mode %d failed", p_Inp->FileFormat);
    error(errortext,100);
    }

  // create all context models
  currSlice->mot_ctx = create_contexts_MotionInfo();
  currSlice->tex_ctx = create_contexts_TextureInfo();

  currSlice->max_part_nr = 3;  //! assume data partitioning (worst case) for the following mallocs()
  currSlice->partArr = AllocPartition(currSlice->max_part_nr);

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

#if (MVC_EXTENSION_ENABLE)
  currSlice->view_id = MVC_INIT_VIEW_ID;
  currSlice->inter_view_flag = 0;
  currSlice->anchor_pic_flag = 0;
#endif
  // reference flag initialization
  for(i=0;i<17;++i)
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
/*!
 ************************************************************************
 * \brief
 *    Memory frees of the Slice structure and of its dependent
 *    data structures
 *
 * \par Input:
 *    Input Parameters Slice *currSlice
 ************************************************************************
 */
static void free_slice (Slice *currSlice) {

  int i;

  if (currSlice->slice_type != I_SLICE && currSlice->slice_type != SI_SLICE)
    free_ref_pic_list_reordering_buffer(currSlice);

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

  //if (1)
  {
    // delete all context models
    delete_contexts_MotionInfo (currSlice->mot_ctx);
    delete_contexts_TextureInfo (currSlice->tex_ctx);
  }

  for (i=0; i<6; i++) {
    if (currSlice->listX[i]) {
      free (currSlice->listX[i]);
      currSlice->listX[i] = NULL;
      }
    }

  while (currSlice->dec_ref_pic_marking_buffer) {
    DecRefPicMarking_t *tmp_drpm=currSlice->dec_ref_pic_marking_buffer;
    currSlice->dec_ref_pic_marking_buffer=tmp_drpm->Next;
    free (tmp_drpm);
    }

  free(currSlice);
  currSlice = NULL;
  }
//}}}

//{{{
/*!
 ************************************************************************
 * \brief
 *    Dynamic memory allocation of frame size related global buffers
 *    buffers are defined in global.h, allocated memory must be freed in
 *    void free_global_buffers()
 *
 *  \par Input:
 *    Input Parameters VideoParameters *p_Vid
 *
 *  \par Output:
 *     Number of allocated bytes
 ***********************************************************************
 */
int init_global_buffers (VideoParameters* p_Vid, int layer_id)
{
  int memory_size=0;
  int i;
  CodingParameters *cps = p_Vid->p_EncodePar[layer_id];
  BlockPos* PicPos;

  if (p_Vid->global_init_done[layer_id])
  {
    free_layer_buffers(p_Vid, layer_id);
  }

  // allocate memory in structure p_Vid
  if( (cps->separate_colour_plane_flag != 0) )
  {
    for( i=0; i<MAX_PLANE; ++i )
    {
      if(((cps->mb_data_JV[i]) = (Macroblock *) calloc(cps->FrameSizeInMbs, sizeof(Macroblock))) == NULL)
        no_mem_exit("init_global_buffers: cps->mb_data_JV");
    }
    cps->mb_data = NULL;
  }
  else
  {
    if(((cps->mb_data) = (Macroblock *) calloc(cps->FrameSizeInMbs, sizeof(Macroblock))) == NULL)
      no_mem_exit("init_global_buffers: cps->mb_data");
  }
  if( (cps->separate_colour_plane_flag != 0) )
  {
    for( i=0; i<MAX_PLANE; ++i )
    {
      if(((cps->intra_block_JV[i]) = (char*) calloc(cps->FrameSizeInMbs, sizeof(char))) == NULL)
        no_mem_exit("init_global_buffers: cps->intra_block_JV");
    }
    cps->intra_block = NULL;
  }
  else
  {
    if(((cps->intra_block) = (char*) calloc(cps->FrameSizeInMbs, sizeof(char))) == NULL)
      no_mem_exit("init_global_buffers: cps->intra_block");
  }


  //memory_size += get_mem2Dint(&PicPos,p_Vid->FrameSizeInMbs + 1,2);  //! Helper array to access macroblock positions. We add 1 to also consider last MB.
  if(((cps->PicPos) = (BlockPos*) calloc(cps->FrameSizeInMbs + 1, sizeof(BlockPos))) == NULL)
    no_mem_exit("init_global_buffers: PicPos");

  PicPos = cps->PicPos;
  for (i = 0; i < (int) cps->FrameSizeInMbs + 1;++i)
  {
    PicPos[i].x = (short) (i % cps->PicWidthInMbs);
    PicPos[i].y = (short) (i / cps->PicWidthInMbs);
  }

  if( (cps->separate_colour_plane_flag != 0) )
  {
    for( i=0; i<MAX_PLANE; ++i )
    {
      get_mem2D(&(cps->ipredmode_JV[i]), 4*cps->FrameHeightInMbs, 4*cps->PicWidthInMbs);
    }
    cps->ipredmode = NULL;
  }
  else
   memory_size += get_mem2D(&(cps->ipredmode), 4*cps->FrameHeightInMbs, 4*cps->PicWidthInMbs);

  // CAVLC mem
  memory_size += get_mem4D(&(cps->nz_coeff), cps->FrameSizeInMbs, 3, BLOCK_SIZE, BLOCK_SIZE);
  if( (cps->separate_colour_plane_flag != 0) )
  {
    for( i=0; i<MAX_PLANE; ++i )
    {
      get_mem2Dint(&(cps->siblock_JV[i]), cps->FrameHeightInMbs, cps->PicWidthInMbs);
      if(cps->siblock_JV[i]== NULL)
        no_mem_exit("init_global_buffers: p_Vid->siblock_JV");
    }
    cps->siblock = NULL;
  }
  else
  {
    memory_size += get_mem2Dint(&(cps->siblock), cps->FrameHeightInMbs, cps->PicWidthInMbs);
  }
  init_qp_process(cps);
  cps->oldFrameSizeInMbs = cps->FrameSizeInMbs;

  p_Vid->global_init_done[layer_id] = 1;

  return (memory_size);
}
//}}}
//{{{
/*!
 ************************************************************************
 * \brief
 *    Free allocated memory of frame size related global buffers
 *    buffers are defined in global.h, allocated memory is allocated in
 *    int init_global_buffers()
 *
 * \par Input:
 *    Input Parameters VideoParameters *p_Vid
 *
 * \par Output:
 *    none
 *
 ************************************************************************
 */
void free_layer_buffers (VideoParameters* p_Vid, int layer_id)
{
  CodingParameters *cps = p_Vid->p_EncodePar[layer_id];

  if(!p_Vid->global_init_done[layer_id])
    return;

  // CAVLC free mem
  if (cps->nz_coeff)
  {
    free_mem4D(cps->nz_coeff);
    cps->nz_coeff = NULL;
  }

  // free mem, allocated for structure p_Vid
  if( (cps->separate_colour_plane_flag != 0) )
  {
    int i;
    for(i=0; i<MAX_PLANE; i++)
    {
      free(cps->mb_data_JV[i]);
      cps->mb_data_JV[i] = NULL;
      free_mem2Dint(cps->siblock_JV[i]);
      cps->siblock_JV[i] = NULL;
      free_mem2D(cps->ipredmode_JV[i]);
      cps->ipredmode_JV[i] = NULL;
      free (cps->intra_block_JV[i]);
      cps->intra_block_JV[i] = NULL;
    }
  }
  else
  {
    if (cps->mb_data != NULL)
    {
      free(cps->mb_data);
      cps->mb_data = NULL;
    }
    if(cps->siblock)
    {
      free_mem2Dint(cps->siblock);
      cps->siblock = NULL;
    }
    if(cps->ipredmode)
    {
      free_mem2D(cps->ipredmode);
      cps->ipredmode = NULL;
    }
    if(cps->intra_block)
    {
      free (cps->intra_block);
      cps->intra_block = NULL;
    }
  }
  if(cps->PicPos)
  {
    free(cps->PicPos);
    cps->PicPos = NULL;
  }

  free_qp_matrices(cps);


  p_Vid->global_init_done[layer_id] = 0;
}
//}}}
//{{{
void free_global_buffers (VideoParameters* p_Vid)
{
  if(p_Vid->dec_picture)
  {
    free_storable_picture(p_Vid->dec_picture);
    p_Vid->dec_picture = NULL;
  }
#if MVC_EXTENSION_ENABLE
  if(p_Vid->active_subset_sps && p_Vid->active_subset_sps->sps.Valid && (p_Vid->active_subset_sps->sps.profile_idc==MVC_HIGH||p_Vid->active_subset_sps->sps.profile_idc == STEREO_HIGH))
    free_img_data( p_Vid, &(p_Vid->tempData3) );
#endif
}
//}}}

//{{{
void ClearDecPicList (VideoParameters* p_Vid) {

  DecodedPicList* pPic = p_Vid->pDecOuputPic, *pPrior = NULL;
  //find the head first;
  while (pPic && !pPic->bValid) {
    pPrior = pPic;
    pPic = pPic->pNext;
    }

  if (pPic && (pPic != p_Vid->pDecOuputPic)) {
    //move all nodes before pPic to the end;
    DecodedPicList *pPicTail = pPic;
    while (pPicTail->pNext)
      pPicTail = pPicTail->pNext;

    pPicTail->pNext = p_Vid->pDecOuputPic;
    p_Vid->pDecOuputPic = pPic;
    pPrior->pNext = NULL;
    }
  }
//}}}
//{{{
DecodedPicList* get_one_avail_dec_pic_from_list (DecodedPicList* pDecPicList, int b3D, int view_id)
{
  DecodedPicList *pPic = pDecPicList, *pPrior = NULL;
  if(b3D)
  {
    while(pPic && (pPic->bValid &(1<<view_id)))
    {
      pPrior = pPic;
      pPic = pPic->pNext;
    }
  }
  else
  {
    while(pPic && (pPic->bValid))
    {
      pPrior = pPic;
      pPic = pPic->pNext;
    }
  }

  if(!pPic)
  {
    pPic = (DecodedPicList *)calloc(1, sizeof(*pPic));
    pPrior->pNext = pPic;
  }

  return pPic;
}
//}}}

//{{{
int OpenDecoder (InputParameters* p_Inp) {

  DecoderParams* pDecoder;
  int iRet = alloc_decoder (&p_Dec);
  if (iRet)
    return (iRet|DEC_ERRMASK);

  init_time();

  pDecoder = p_Dec;
  memcpy (pDecoder->p_Inp, p_Inp, sizeof(InputParameters));
  pDecoder->p_Vid->conceal_mode = p_Inp->conceal_mode;
  pDecoder->p_Vid->ref_poc_gap = p_Inp->ref_poc_gap;
  pDecoder->p_Vid->poc_gap = p_Inp->poc_gap;

  malloc_annex_b (pDecoder->p_Vid, &pDecoder->p_Vid->annex_b);
  open_annex_b (pDecoder->p_Inp->infile, pDecoder->p_Vid->annex_b);

  // Allocate Slice data struct
  //pDecoder->p_Vid->currentSlice = NULL; //malloc_slice(pDecoder->p_Inp, pDecoder->p_Vid);
  init_old_slice (pDecoder->p_Vid->old_slice);
  init (pDecoder->p_Vid);
  init_out_buffer (pDecoder->p_Vid);

  return DEC_OPEN_NOERR;
  }
//}}}
//{{{
int DecodeOneFrame (DecodedPicList** ppDecPicList) {

  DecoderParams* pDecoder = p_Dec;
  ClearDecPicList (pDecoder->p_Vid);

  int iRet = decode_one_frame (pDecoder);
  if (iRet == SOP)
    iRet = DEC_SUCCEED;
  else if (iRet == EOS)
    iRet = DEC_EOS;
  else
    iRet |= DEC_ERRMASK;

  *ppDecPicList = pDecoder->p_Vid->pDecOuputPic;
  return iRet;
  }
//}}}
//{{{
int FinitDecoder (DecodedPicList** ppDecPicList) {

  DecoderParams* pDecoder = p_Dec;
  if (!pDecoder)
    return DEC_GEN_NOERR;

  ClearDecPicList (pDecoder->p_Vid);
  flush_dpb (pDecoder->p_Vid->p_Dpb_layer[0]);

  #if (PAIR_FIELDS_IN_OUTPUT)
    flush_pending_output (pDecoder->p_Vid);
  #endif

  if (pDecoder->p_Inp->FileFormat == PAR_OF_ANNEXB)
    reset_annex_b (pDecoder->p_Vid->annex_b);

  pDecoder->p_Vid->newframe = 0;
  pDecoder->p_Vid->previous_frame_num = 0;
  *ppDecPicList = pDecoder->p_Vid->pDecOuputPic;

  return DEC_GEN_NOERR;
  }
//}}}
//{{{
int CloseDecoder() {

  int i;
  DecoderParams *pDecoder = p_Dec;
  if( !pDecoder)
    return DEC_CLOSE_NOERR;

  FmoFinit(pDecoder->p_Vid);
  free_layer_buffers (pDecoder->p_Vid, 0);
  free_layer_buffers (pDecoder->p_Vid, 1);
  free_global_buffers (pDecoder->p_Vid);
  close_annex_b (pDecoder->p_Vid->annex_b);

  ercClose (pDecoder->p_Vid, pDecoder->p_Vid->erc_errorVar);

  CleanUpPPS (pDecoder->p_Vid);

  for (i = 0; i < MAX_NUM_DPB_LAYERS; i++)
    free_dpb (pDecoder->p_Vid->p_Dpb_layer[i]);


  uninit_out_buffer (pDecoder->p_Vid);

  free_img (pDecoder->p_Vid);
  free (pDecoder->p_Inp);
  free (pDecoder);

  p_Dec = NULL;
  return DEC_CLOSE_NOERR;
  }
//}}}

//{{{
void set_global_coding_par (VideoParameters* p_Vid, CodingParameters* cps)
{
    p_Vid->bitdepth_chroma = 0;
    p_Vid->width_cr        = 0;
    p_Vid->height_cr       = 0;
    p_Vid->lossless_qpprime_flag   = cps->lossless_qpprime_flag;
    p_Vid->max_vmv_r = cps->max_vmv_r;

    // Fidelity Range Extensions stuff (part 1)
    p_Vid->bitdepth_luma       = cps->bitdepth_luma;
    p_Vid->bitdepth_scale[0]   = cps->bitdepth_scale[0];
    p_Vid->bitdepth_chroma = cps->bitdepth_chroma;
    p_Vid->bitdepth_scale[1] = cps->bitdepth_scale[1];

    p_Vid->max_frame_num = cps->max_frame_num;
    p_Vid->PicWidthInMbs = cps->PicWidthInMbs;
    p_Vid->PicHeightInMapUnits = cps->PicHeightInMapUnits;
    p_Vid->FrameHeightInMbs = cps->FrameHeightInMbs;
    p_Vid->FrameSizeInMbs = cps->FrameSizeInMbs;

    p_Vid->yuv_format = cps->yuv_format;
    p_Vid->separate_colour_plane_flag = cps->separate_colour_plane_flag;
    p_Vid->ChromaArrayType = cps->ChromaArrayType;

    p_Vid->width = cps->width;
    p_Vid->height = cps->height;
    p_Vid->iLumaPadX = MCBUF_LUMA_PAD_X;
    p_Vid->iLumaPadY = MCBUF_LUMA_PAD_Y;
    p_Vid->iChromaPadX = MCBUF_CHROMA_PAD_X;
    p_Vid->iChromaPadY = MCBUF_CHROMA_PAD_Y;
    if (p_Vid->yuv_format == YUV420)
    {
      p_Vid->width_cr  = (p_Vid->width  >> 1);
      p_Vid->height_cr = (p_Vid->height >> 1);
    }
    else if (p_Vid->yuv_format == YUV422)
    {
      p_Vid->width_cr  = (p_Vid->width >> 1);
      p_Vid->height_cr = p_Vid->height;
      p_Vid->iChromaPadY = MCBUF_CHROMA_PAD_Y*2;
    }
    else if (p_Vid->yuv_format == YUV444)
    {
      //YUV444
      p_Vid->width_cr = p_Vid->width;
      p_Vid->height_cr = p_Vid->height;
      p_Vid->iChromaPadX = p_Vid->iLumaPadX;
      p_Vid->iChromaPadY = p_Vid->iLumaPadY;
    }

    init_frext(p_Vid);
}
//}}}
