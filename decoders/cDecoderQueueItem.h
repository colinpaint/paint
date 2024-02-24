// cDecoderQueueItem.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <functional>

class cFrame;
class cDecoder;
//}}}

class cDecodeQueueItem {
public:
  // we gain ownership of malloc'd pes buffer
  cDecodeQueueItem (cDecoder* decoder,
                    uint8_t* pes, int pesSize, int64_t pts, const std::string& frameInfo,
                    std::function<cFrame* (int64_t pts)> allocFrameCallback,
                    std::function<void (cFrame* frame)> addFrameCallback)
      : mDecoder(decoder),
        mPes(pes), mPesSize(pesSize), mPts(pts), mFrameInfo(frameInfo),
        mAllocFrameCallback(allocFrameCallback),
        mAddFrameCallback(addFrameCallback) {}

  ~cDecodeQueueItem() {
    // release malloc'd pes buffer
    free (mPes);
    }

  // vars
  cDecoder* mDecoder;

  uint8_t* mPes;
  const int mPesSize;

  const int64_t mPts;
  const std::string mFrameInfo;

  const std::function<cFrame* (int64_t pts)> mAllocFrameCallback;
  const std::function<void (cFrame* frame)> mAddFrameCallback;
  };
