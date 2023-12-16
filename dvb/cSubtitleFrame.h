// cSubtitleFrame.h
//{{{  includes
#pragma once
#include <cstdint>

#include "../decoders/cFrame.h"
//}}}

class cSubtitleFrame : public cFrame {
public:
  cSubtitleFrame() = default;

  virtual ~cSubtitleFrame() {
    releaseResources();
    }

  virtual void releaseResources() final {}
  };
