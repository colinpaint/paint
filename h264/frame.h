#pragma once

typedef enum {
  CM_UNKNOWN = -1,
  CM_YUV     =  0,
  CM_RGB     =  1,
  CM_XYZ     =  2
  } eColorModel;

typedef enum {
  CF_UNKNOWN = -1, // Unknown color format
  YUV400     =  0, // Monochrome
  YUV420     =  1, // 4:2:0
  YUV422     =  2, // 4:2:2
  YUV444     =  3  // 4:4:4
  } eYuvFormat;

typedef enum {
  PF_UNKNOWN = -1, // Unknown color ordering
  UYVY       =  0, // UYVY
  YUY2       =  1, // YUY2
  YUYV       =  1, // YUYV
  YVYU       =  2, // YVYU
  BGR        =  3, // BGR
  V210       =  4  // Video Clarity 422 format (10 bits)
  } ePixelFormat;

struct sFrameFormat {
  eYuvFormat   yuvFormat;       // YUV format (0=4:0:0, 1=4:2:0, 2=4:2:2, 3=4:4:4)
  eColorModel  colourModel;     // 4:4:4 format (0: YUV, 1: RGB, 2: XYZ)
  ePixelFormat pixelFormat;     // pixel format support for certain interleaved yuv sources

  int          width[3];        // component frame width
  int          height[3];       // component frame height

  int          mbWidth;         // luma component frame width
  int          mbHeight;        // luma component frame height

  int          sizeCmp[3];      // component sizes (width * height)
  int          bitDepth[3];     // component bit depth
  };
