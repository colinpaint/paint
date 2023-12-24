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
                    uint16_t pid, uint8_t* pes, int pesSize,
                    int64_t pts, int64_t dts, int64_t streamPos,
                    std::function<cFrame*()> getFrameCallback,
                    std::function<void (cFrame* frame)> addFrameCallback)
      : mDecoder(decoder),
        mPid(pid), mPes(pes), mPesSize(pesSize),
        mPts(pts), mDts(dts), mStreamPos(streamPos),
        mGetFrameCallback(getFrameCallback),
        mAddFrameCallback(addFrameCallback) {}

  ~cDecodeQueueItem() {
    // release malloc'd pes buffer
    free (mPes);
    }

  // vars
  cDecoder* mDecoder;

  const uint16_t mPid;
  uint8_t* mPes;
  const int mPesSize;

  const int64_t mPts;
  const int64_t mDts;
  const int64_t mStreamPos;

  const std::function<cFrame*()> mGetFrameCallback;
  const std::function<void (cFrame* frame)> mAddFrameCallback;
  };
