// cBrushUI.h
#pragma once
//{{{  includes
#include "cUI.h"
#include "cUIMan.h"
//}}}

class cBrushUI : public cUI {
public:
  cBrushUI (const std::string& name) : cUI(name) {}
  virtual ~cBrushUI() = default;

  void addToDrawList (cCanvas& canvas) override;

private:
  static cUI* createUI (const std::string& className) {
    return new cBrushUI (className);
    }

  // static to register class
  inline static const bool mRegistered = cUIMan::registerClass ("brush", &createUI);
  };
