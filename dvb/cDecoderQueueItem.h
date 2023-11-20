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
  //{{{
  cDecodeQueueItem (cDecoder* decoder,
                    uint16_t pid, uint8_t* pes, int pesSize, int64_t pts, int64_t dts,
                    std::function<cFrame*()> allocFrameCallback,
                    std::function<void (cFrame* frame)> addFrameCallback)
      : mDecoder(decoder),
        mPid(pid), mPes(pes), mPesSize(pesSize), mPts(pts), mDts(dts),
        mAllocFrameCallback(allocFrameCallback), mAddFrameCallback(addFrameCallback) {}
  // we gain ownership of malloc'd pes buffer
  //}}}
  ~cDecodeQueueItem() {
    // release malloc'd pes buffer
    free (mPes);
    }

  cDecoder* mDecoder;

  uint16_t mPid;
  uint8_t* mPes;
  const int mPesSize;

  const int64_t mPts;
  const int64_t mDts;

  const std::function<cFrame*()> mAllocFrameCallback;
  const std::function<void (cFrame* frame)> mAddFrameCallback;
  };
