// cApp.h - use as class or base class  - holder of application context, passed around in single reference
#pragma once
//{{{  includes
#include <string>
class cPlatform;
class cGraphics;
struct ImFont;
//}}}

class cApp {
public:
  cApp (cPlatform& platform, cGraphics& graphics) : mPlatform(platform), mGraphics(graphics) {}
  virtual ~cApp() = default;

  // gets
  cPlatform& getPlatform() const { return mPlatform; }
  cGraphics& getGraphics() const { return mGraphics; }
  std::string getName() const { return mName; }
  ImFont* getMonoFont() const { return mMonoFont; }

  // sets
  void setName (const std::string name) { mName = name; }
  void setMonoFont (ImFont* mFont) { mMonoFont = mFont; }

private:
  cPlatform& mPlatform;
  cGraphics& mGraphics;
  std::string mName;
  ImFont* mMonoFont;
  };
