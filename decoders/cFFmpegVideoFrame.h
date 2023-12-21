// cFFmpegVideoFrame.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>

#include "../common/cLog.h"

#include "cVideoFrame.h"

//{{{  include libav
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/motion_vector.h>
  #include <libavutil/frame.h>
  }

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}
//}}}

class cFFmpegVideoFrame : public cVideoFrame {
public:
  cFFmpegVideoFrame() : cVideoFrame(cTexture::eYuv420) {}
  //{{{
  virtual ~cFFmpegVideoFrame() {
    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cFFmpegVideoFrame::~cFFmpegVideoFrame"));
    }
  //}}}

  //{{{
  virtual AVMotionVector* getMotionVectors (size_t& numMotionVectors) final {

    AVFrameSideData* sideData = av_frame_get_side_data (mAvFrame, AV_FRAME_DATA_MOTION_VECTORS);
    numMotionVectors = sideData->size / sizeof(AVMotionVector);
    return (AVMotionVector*)(sideData->data);
    }
  //}}}

  //{{{
  void setAVFrame (AVFrame* avFrame) {
    // release old AVframe before owning new AVframe
    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cFFmpegVideoFrame::setAVFrame"));

    releasePixels();

    mAvFrame = avFrame;

    mWidth = (uint16_t)avFrame->width;
    mHeight = (uint16_t)avFrame->height;
    mStrideY = (uint16_t)avFrame->linesize[0];
    mStrideUV = (uint16_t)avFrame->linesize[1];
    mInterlaced = avFrame->flags && AV_FRAME_FLAG_INTERLACED;
    mTopFieldFirst = avFrame->flags && AV_FRAME_FLAG_TOP_FIELD_FIRST;
    }
  //}}}

protected:
  virtual uint8_t** getPixels() final { return mAvFrame->data; }
  virtual int* getStrides() final { return mAvFrame->linesize; }
  //{{{
  virtual void releasePixels() final {
    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cFFmpegVideoFrame::~releasePixels"));

    if (mAvFrame) {
      // release old AVframe
      AVFrame* avFrame = mAvFrame;
      mAvFrame = nullptr;
      av_frame_unref (avFrame);
      av_frame_free (&avFrame);
      }
    }
  //}}}

private:
  AVFrame* mAvFrame = nullptr;
  };
