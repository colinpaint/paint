// h264Decoder test
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <string>
#include <sys/stat.h>

// common
#include "../date/include/date/date.h"
#include "../common/basicTypes.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"

// h264
#include "../h264/win32.h"
#include "../h264/h264decoder.h"

using namespace std;
//}}}

//{{{
int WriteOneFrame (DecodedPicList* pDecPic, int bOutputAllFrames) {
// if bOutputAllFrames is 1, then output all valid frames to file onetime;
// else output the first valid frame and move the buffer to the end of list;

  int iOutputFrame = 0;
  DecodedPicList* pPic = pDecPic;

  cLog::log (LOGINFO, fmt::format ("WriteOneFrame {}x{}:{} format:{}:{} stride:{}x{} yuv:{:x}:{:x}:{:x}",
                                   pPic->iWidth * ((pPic->iBitDepth+7)>>3),
                                   pPic->iHeight,
                                   pPic->iBitDepth,
                                   pPic->iYUVStorageFormat, pPic->bValid,
                                   pPic->iYBufStride,
                                   pPic->iUVBufStride,
                                   (uint64_t)pPic->pY,
                                   (uint64_t)pPic->pU,
                                   (uint64_t)pPic->pV
                                   ));

  if (pPic && (((pPic->iYUVStorageFormat == 2) && pPic->bValid == 3) ||
               ((pPic->iYUVStorageFormat != 2) && pPic->bValid == 1)) ) {
    int iWidth = pPic->iWidth * ((pPic->iBitDepth+7)>>3);
    int iHeight = pPic->iHeight;
    int iStride = pPic->iYBufStride;

    int iWidthUV;
    int iHeightUV;
    if (pPic->iYUVFormat != YUV444)
      iWidthUV = pPic->iWidth>>1;
    else
      iWidthUV = pPic->iWidth;
    if (pPic->iYUVFormat == YUV420)
      iHeightUV = pPic->iHeight >> 1;
    else
      iHeightUV = pPic->iHeight;
    iWidthUV *= ((pPic->iBitDepth + 7) >> 3);
    int iStrideUV = pPic->iUVBufStride;

    do {
      uint8_t* pbBuf;
      pbBuf = pPic->pY;
      //for (i = 0; i < iHeight; i++)
      //  write (hFileOutput, pbBuf+i*iStride, iWidth);

      pbBuf = pPic->pU;
      //for(i=0; i < iHeightUV; i++) {
      //  write( hFileOutput, pbBuf+i*iStrideUV, iWidthUV);

      pbBuf = pPic->pV;
      //for (i = 0; i < iHeightUV; i++) {
      //  write(hFileOutput, pbBuf+i*iStrideUV, iWidthUV);

      pPic->bValid = 0;
      pPic = pPic->pNext;
      } while (pPic != NULL && pPic->bValid && bOutputAllFrames);
    }

  return iOutputFrame;
  }
//}}}

//{{{
int main (int numArgs, char* args[]) {

  int iRet;
  InputParameters InputParams;
  DecodedPicList *pDecPicList;
  int iFramesOutput = 0;
  int iFramesDecoded = 0;

  string fileName;
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    fileName = param;
    }

  cLog::init (LOGINFO);
  cLog::log (LOGNOTICE, fmt::format ("h264DecoderApp"));

  // input params
  memset (&InputParams, 0, sizeof(InputParameters));
  strcpy (InputParams.infile, fileName.c_str());
  strcpy (InputParams.outfile, "test.yuv"); //! set default output file name
  InputParams.FileFormat = PAR_OF_ANNEXB;
  InputParams.poc_scale = 2;
  InputParams.intra_profile_deblocking = 1;
  InputParams.poc_gap = 2;
  InputParams.ref_poc_gap = 2;
  InputParams.bDisplayDecParams = 1;
  InputParams.dpb_plus[0] = 1;
  //{{{  more input params
  //InputParams.ref_offset;
  //InputParams.write_uv;
  //InputParams.silent;
  //InputParams.source;                   //!< source related information
  //InputParams.output;                   //!< output related information
  //InputParams.ProcessInput;
  //InputParams.enable_32_pulldown;
  //InputParams.input_file1;          //!< Input video file1
  //InputParams.input_file2;          //!< Input video file2
  //InputParams.input_file3;          //!< Input video file3
  //InputParams.conceal_mode;
  //InputParams.start_frame;
  //InputParams.stdRange;                         //!< 1 - standard range, 0 - full range
  //InputParams.videoCode;                        //!< 1 - 709, 3 - 601:  See VideoCode in io_tiff.
  //InputParams.export_views;
  //InputParams.iDecFrmNum;
  //}}}

  iRet = OpenDecoder (&InputParams);
  if (iRet != DEC_OPEN_NOERR) {
    fprintf (stderr, "Open encoder failed: 0x%x!\n", iRet);
    return -1;
    }

  // decoding;
  do {
    iRet = DecodeOneFrame (&pDecPicList);
    if (iRet == DEC_EOS || iRet==DEC_SUCCEED) {
      // process the decoded picture, output or display;
      iFramesOutput += WriteOneFrame (pDecPicList, 0);
      iFramesDecoded++;
      }
    else // error handling;
      fprintf (stderr, "Error in decoding process: 0x%x\n", iRet);
    } while ((iRet == DEC_SUCCEED) &&
             ((p_Dec->p_Inp->iDecFrmNum == 0) ||
              (iFramesDecoded < p_Dec->p_Inp->iDecFrmNum)));

  iRet = FinitDecoder (&pDecPicList);
  iFramesOutput += WriteOneFrame (pDecPicList, 1);
  iRet = CloseDecoder();

  printf ("%d frames are decoded.\n", iFramesDecoded);
  return 0;
  }
//}}}
