// cMenuUI.h
#pragma once
//{{{  includes
#include "cUI.h"
#include "cUIMan.h"
//}}}

class cMenuUI : public cUI {
public:
  cMenuUI (const std::string& name) : cUI(name) {}
  virtual ~cMenuUI() = default;

  void addToDrawList (cCanvas& canvas)  override;

private:
  static cUI* createUI (const std::string& className) {
    return new cMenuUI (className);
    }

  // static to register class
  inline static const bool mRegistered = cUIMan::registerClass ("menu", &createUI);
  };
