// h264Decoder test
#include <sys/stat.h>
#include "../h264/win32.h"
#include "../h264/h264decoder.h"
#include <string>

using namespace std;

#define DECOUTPUT_TEST      0

#define PRINT_OUTPUT_POC    0
#define FCFR_DEBUG_FILENAME "fcfr_dec_rpu_stats.txt"
#define DECOUTPUT_VIEW0_FILENAME  "H264_Decoder_Output_View0.yuv"
#define DECOUTPUT_VIEW1_FILENAME  "H264_Decoder_Output_View1.yuv"

//{{{
static int WriteOneFrame (DecodedPicList *pDecPic, int hFileOutput0, int hFileOutput1, int bOutputAllFrames) {
// if bOutputAllFrames is 1, then output all valid frames to file onetime;
// else output the first valid frame and move the buffer to the end of list;

  int iOutputFrame = 0;
  DecodedPicList* pPic = pDecPic;

  if (pPic && (((pPic->iYUVStorageFormat==2) && pPic->bValid==3) ||
               ((pPic->iYUVStorageFormat!=2) && pPic->bValid==1)) ) {
    int i, iWidth, iHeight, iStride, iWidthUV, iHeightUV, iStrideUV;
    uint8_t* pbBuf;
    int hFileOutput;
    size_t res;

    iWidth = pPic->iWidth*((pPic->iBitDepth+7)>>3);
    iHeight = pPic->iHeight;
    iStride = pPic->iYBufStride;
    if(pPic->iYUVFormat != YUV444)
      iWidthUV = pPic->iWidth>>1;
    else
      iWidthUV = pPic->iWidth;
    if(pPic->iYUVFormat == YUV420)
      iHeightUV = pPic->iHeight>>1;
    else
      iHeightUV = pPic->iHeight;
    iWidthUV *= ((pPic->iBitDepth+7)>>3);
    iStrideUV = pPic->iUVBufStride;

    do {
      if(pPic->iYUVStorageFormat==2)
        hFileOutput = (pPic->iViewId&0xffff)? hFileOutput1 : hFileOutput0;
      else
        hFileOutput = hFileOutput0;
      if(hFileOutput >=0) {
        //{{{  Y;
        pbBuf = pPic->pY;
        for(i=0; i<iHeight; i++) {
          res = write(hFileOutput, pbBuf+i*iStride, iWidth);
          if (-1==res)
            error ("error writing to output file.", 600);
        }
        //}}}
        if (pPic->iYUVFormat != YUV400) {
          //{{{  U;
          pbBuf = pPic->pU;
          for(i=0; i<iHeightUV; i++) {
            res = write(hFileOutput, pbBuf+i*iStrideUV, iWidthUV);
            if (-1==res)
              error ("error writing to output file.", 600);
            }
          //}}}
          //{{{  V;
          pbBuf = pPic->pV;
          for(i=0; i<iHeightUV; i++) {
            res = write(hFileOutput, pbBuf+i*iStrideUV, iWidthUV);
            if (-1==res)
              error ("error writing to output file.", 600);
            }
          //}}}
          }

        iOutputFrame++;
        }

      if (pPic->iYUVStorageFormat == 2) {
        hFileOutput = ((pPic->iViewId>>16)&0xffff)? hFileOutput1 : hFileOutput0;
        if(hFileOutput>=0) {
          int iPicSize =iHeight*iStride;
          //{{{  Y;
          pbBuf = pPic->pY+iPicSize;
          for(i=0; i<iHeight; i++) {
            res = write(hFileOutput, pbBuf+i*iStride, iWidth);
            if (-1==res)
              error ("error writing to output file.", 600);
          }
          //}}}
          if(pPic->iYUVFormat != YUV400) {
            iPicSize = iHeightUV*iStrideUV;
            //{{{  U;
            pbBuf = pPic->pU+iPicSize;
            for(i=0; i<iHeightUV; i++) {
              res = write(hFileOutput, pbBuf+i*iStrideUV, iWidthUV);
              if (-1==res)
                error ("error writing to output file.", 600);
            }
            //}}}
            //{{{  V;
            pbBuf = pPic->pV+iPicSize;
            for(i=0; i<iHeightUV; i++) {
              res = write(hFileOutput, pbBuf+i*iStrideUV, iWidthUV);
              if (-1==res)
                error ("error writing to output file.", 600);
            }
            //}}}
            }

          iOutputFrame++;
          }
        }

      fprintf (stdout, "\nOutput frame: %d/%d\n", pPic->iPOC, pPic->iViewId);
      pPic->bValid = 0;
      pPic = pPic->pNext;
      }while(pPic != NULL && pPic->bValid && bOutputAllFrames);
    }

  return iOutputFrame;
  }
//}}}

//{{{
int main (int numArgs, char* args[]) {

  int iRet;
  DecodedPicList *pDecPicList;
  int hFileDecOutput0 = -1;
  int hFileDecOutput1 = -1;
  int iFramesOutput=0;
  int iFramesDecoded=0;
  InputParameters InputParams;

  string fileName;
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    fileName = param;
    }

  init_time();

  // get input parameters;
  memset (&InputParams, 0, sizeof(InputParameters));
  strcpy (InputParams.infile, fileName.c_str());
  strcpy (InputParams.outfile, "test_dec.yuv"); //! set default output file name

  //infile[FILE_NAME_SIZE];                       //!< H.264 inputfile
  //outfile[FILE_NAME_SIZE];                      //!< Decoded YUV 4:2:0 output
  //reffile[FILE_NAME_SIZE];                      //!< Optional YUV 4:2:0 reference file for SNR measurement
  InputParams.FileFormat = PAR_OF_ANNEXB;
  //InputParams.ref_offset;
  //InputParams.poc_scale;
  //InputParams.write_uv;
  //InputParams.silent;
  //InputParams.intra_profile_deblocking;               //!< Loop filter usage determined by flags and parameters in bitstream
  //InputParams.source;                   //!< source related information
  //InputParams.output;                   //!< output related information
  //InputParams.ProcessInput;
  //InputParams.enable_32_pulldown;
  //InputParams.input_file1;          //!< Input video file1
  //InputParams.input_file2;          //!< Input video file2
  //InputParams.input_file3;          //!< Input video file3
  //InputParams.conceal_mode;
  //InputParams.ref_poc_gap;
  //InputParams.poc_gap;
  //InputParams.start_frame;
  //InputParams.stdRange;                         //!< 1 - standard range, 0 - full range
  //InputParams.videoCode;                        //!< 1 - 709, 3 - 601:  See VideoCode in io_tiff.
  //InputParams.export_views;
  //InputParams.iDecFrmNum;
  InputParams.bDisplayDecParams = 1;
  //InputParams.dpb_plus[2];

  iRet = OpenDecoder (&InputParams);
  if (iRet != DEC_OPEN_NOERR) {
    fprintf (stderr, "Open encoder failed: 0x%x!\n", iRet);
    return -1; //failed;
    }

  // decoding;
  do {
    iRet = DecodeOneFrame (&pDecPicList);
    if (iRet == DEC_EOS || iRet==DEC_SUCCEED) {
      // process the decoded picture, output or display;
      iFramesOutput += WriteOneFrame (pDecPicList, hFileDecOutput0, hFileDecOutput1, 0);
      iFramesDecoded++;
      }
    else
      // error handling;
      fprintf (stderr, "Error in decoding process: 0x%x\n", iRet);
    } while ((iRet == DEC_SUCCEED) &&
             ((p_Dec->p_Inp->iDecFrmNum == 0) ||
              (iFramesDecoded<p_Dec->p_Inp->iDecFrmNum)));

  iRet = FinitDecoder (&pDecPicList);
  iFramesOutput += WriteOneFrame (pDecPicList, hFileDecOutput0, hFileDecOutput1 , 1);
  iRet = CloseDecoder();

  printf("%d frames are decoded.\n", iFramesDecoded);
  return 0;
  }
//}}}
