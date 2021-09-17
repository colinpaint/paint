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
  std::string getName() const { return mName; }
  cPlatform& getPlatform() const { return mPlatform; }
  cGraphics& getGraphics() const { return mGraphics; }

  ImFont* getMainFont() const { return mMainFont; }
  ImFont* getMonoFont() const { return mMonoFont; }

  // sets
  void setName (const std::string name) { mName = name; }
  void setMainFont (ImFont* font) { mMainFont = font; }
  void setMonoFont (ImFont* font) { mMonoFont = font; }

private:
  std::string mName;
  cPlatform& mPlatform;
  cGraphics& mGraphics;

  ImFont* mMainFont;
  ImFont* mMonoFont;
  };
