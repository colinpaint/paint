// cUI.h - UI base class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <map>

#include "../graphics/cPointRect.h"

class cCanvas;
//}}}

// cUI
class cUI {
public:
  cUI (const std::string& name) : mName(name) {}
  virtual ~cUI() = default;

  std::string getName() const { return mName; }

  virtual void addToDrawList (cCanvas& canvas) = 0;

private:
   std::string mName;
   };
