// cApp.h = app base class
#pragma once
#include <string>
class cPlatform;
class cGraphics;
struct ImFont;

class cApp {
public:
  cApp (cPlatform& platform, cGraphics& graphics) : mPlatform(platform), mGraphics(graphics) {}
  virtual ~cApp() = default;

  cPlatform& getPlatform() { return mPlatform; }
  cGraphics& getGraphics() { return mGraphics; }
  std::string getName() { return mName; }
  ImFont* getMonoFont() { return mMonoFont; }

  void setName (const std::string name) { mName = name; }
  void setMonoFont (ImFont* mFont) { mMonoFont = mFont; }
                           
protected:
  cPlatform& mPlatform;
  cGraphics& mGraphics;
  std::string mName;
  ImFont* mMonoFont;
  };
