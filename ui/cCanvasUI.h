// cCanvasUI.h
#pragma once
//{{{  includes
#include "cUI.h"

#include <cstdint>
#include <string>
//}}}

class cCanvasUI : public cUI {
public:
  cCanvasUI (const std::string& name) : cUI(name) {}
  virtual ~cCanvasUI() = default;

  void addToDrawList (cCanvas& canvas) override;

private:
  static cUI* createUI (const std::string& className) {
    return new cCanvasUI (className);
    }

  bool mShow = true;

  // static to register class
  inline static const bool mRegistered = registerClass ("layers", &createUI);
  };
