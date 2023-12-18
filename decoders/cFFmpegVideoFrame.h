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
  virtual ~cFFmpegVideoFrame() {
    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cFFmpegVideoFrame::~cFFmpegVideoFrame"));
    }

  virtual std::vector<sMotionVector>& getMotionVectors() final { return mMotionVectors; }

  void setAVFrame (AVFrame* avFrame, bool hasMotionVectors) {
    // release old AVframe before owning new AVframe
    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cFFmpegVideoFrame::setAVFrame"));

    releasePixels();

    mAvFrame = avFrame;

    mMotionVectors.clear();
    mHasMotionVectors = hasMotionVectors;
    if (mHasMotionVectors) {
      AVFrameSideData* sideData = av_frame_get_side_data (avFrame, AV_FRAME_DATA_MOTION_VECTORS);
      if (sideData) {
        const AVMotionVector* mvs = (const AVMotionVector*)sideData->data;
        size_t num = sideData->size / sizeof(AVMotionVector);
        for (size_t i = 0; i < num; i++) {
          const AVMotionVector* mv = &mvs[i];
          mMotionVectors.push_back (sMotionVector(mv->source, mv->src_x, mv->src_y, mv->dst_x, mv->dst_y));
          // mv->w, mv->h, mv->flags, mv->motion_x, mv->motion_y, mv->motion_scale));
          }
        }
      }

    mWidth = (uint16_t)avFrame->width;
    mHeight = (uint16_t)avFrame->height;
    mStrideY = (uint16_t)avFrame->linesize[0];
    mStrideUV = (uint16_t)avFrame->linesize[1];
    mInterlaced = avFrame->flags && AV_FRAME_FLAG_INTERLACED;
    mTopFieldFirst = avFrame->flags && AV_FRAME_FLAG_TOP_FIELD_FIRST;
    }

protected:
  virtual uint8_t** getPixels() final { return mAvFrame->data; }
  virtual int* getStrides() final { return mAvFrame->linesize; }

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

private:
  AVFrame* mAvFrame = nullptr;

  bool mHasMotionVectors;
  std::vector <sMotionVector> mMotionVectors;
  };
