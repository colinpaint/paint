// cSubtitleRender.h
//{{{  includes
#pragma once
#include "cRender.h"

#include <cstdint>
#include <string>

class cSubtitleImage;
//}}}

class cSubtitleRender : public cRender {
public:
  cSubtitleRender (const std::string& name, uint8_t streamType, uint16_t pid, ::cOptions* options);
  virtual ~cSubtitleRender() = default;

  size_t getNumLines() const;
  size_t getHighWatermark() const;
  cSubtitleImage& getImage (size_t line);

  // overrides
  virtual std::string getInfoString() const final;
  };
