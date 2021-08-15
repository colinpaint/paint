// cBrushUI.h
#pragma once
#include "cUI.h"

class cBrushUI : public cUI {
public:
  cBrushUI (const std::string& name) : cUI(name) {}
  virtual ~cBrushUI() = default;

  void addToDrawList (cCanvas& canvas, cGraphics& graphics) final;

private:
  static cUI* createUI (const std::string& className) {
    return new cBrushUI (className);
    }

  // static to register class
  inline static const bool mRegistered = registerClass ("brush", &createUI);
  };
