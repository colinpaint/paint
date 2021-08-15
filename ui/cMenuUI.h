// cMenuUI.h
#pragma once
#include "cUI.h"

class cMenuUI : public cUI {
public:
  cMenuUI (const std::string& name) : cUI(name) {}
  virtual ~cMenuUI() = default;

  void addToDrawList (cCanvas& canvas, cGraphics& graphics) final;

private:
  static cUI* createUI (const std::string& className) {
    return new cMenuUI (className);
    }

  // static to register class
  inline static const bool mRegistered = registerClass ("menu", &createUI);
  };
