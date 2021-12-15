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
  cApp (cPlatform& platform, cGraphics& graphics, ImFont* mainFont, ImFont* monoFont)
    : mPlatform(platform), mGraphics(graphics), mMainFont(mainFont), mMonoFont(monoFont) {}
  virtual ~cApp() = default;

  // gets
  cPlatform& getPlatform() const { return mPlatform; }
  cGraphics& getGraphics() const { return mGraphics; }

  ImFont* getMainFont() const { return mMainFont; }
  ImFont* getMonoFont() const { return mMonoFont; }

private:
  cPlatform& mPlatform;
  cGraphics& mGraphics;

  ImFont* mMainFont;
  ImFont* mMonoFont;
  };
