// cColorUI.h
#pragma once
//{{{  includes
#include "cUI.h"
#include "cUIMan.h"

#include <vector>

// glm
#include <vec4.hpp>
//}}}

class cColorUI  : public cUI {
public:
  cColorUI (const std::string& name);
  virtual ~cColorUI() = default;

  void addToDrawList (cCanvas& canvas) override;

private:
  static cUI* createUI (const std::string& className) {
    return new cColorUI (className);
    }

  // UI owns swatches for now
  std::vector<glm::vec4> mSwatches;

  // static to register class
  inline static const bool mRegistered = cUIMan::registerClass ("colour", &createUI);
  };
