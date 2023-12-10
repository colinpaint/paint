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
  cSubtitleRender (const std::string& name, uint8_t streamType, uint16_t pid, bool live);
  ~cSubtitleRender();

  size_t getNumLines() const;
  size_t getHighWatermark() const;
  cSubtitleImage& getImage (size_t line);

  // callbacks
  cFrame* getFrame();
  void addFrame (cFrame* frame);

  // overrides
  virtual std::string getInfoString() const final;
  };
