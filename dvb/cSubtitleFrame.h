// cSubtitleFrame.h
//{{{  includes
#pragma once
#include <cstdint>

#include "cFrame.h"
//}}}

class cSubtitleFrame : public cFrame {
public:
  cSubtitleFrame() = default;
  virtual ~cSubtitleFrame() {
    releaseResources();
    }

  virtual void releaseResources() final {}
  };
