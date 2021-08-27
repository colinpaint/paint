// cLoader.h - video, audio - loader,player
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

class cLoadSource;
class cSong;
class iVideoPool;
//}}}

class iLoad {
public:
  virtual ~iLoad() {}

  virtual cSong* getSong() = 0;
  virtual iVideoPool* getVideoPool() = 0;
  virtual std::string getInfoString() = 0;
  virtual float getFracs (float& audioFrac, float& videoFrac) = 0;

  // actions
  virtual bool togglePlaying() = 0;
  virtual bool skipBegin() = 0;
  virtual bool skipEnd() = 0;
  virtual bool skipBack (bool shift, bool control) = 0;
  virtual bool skipForward (bool shift, bool control) = 0;
  virtual void exit() = 0;
  };

class cLoader : public iLoad {
public:
  cLoader();
  virtual ~cLoader();

  // iLoad gets
  virtual cSong* getSong() override;
  virtual iVideoPool* getVideoPool() override;
  virtual std::string getInfoString() override;
  virtual float getFracs (float& audioFrac, float& videoFrac) override;

  // iLoad actions
  virtual bool togglePlaying() override;
  virtual bool skipBegin() override;
  virtual bool skipEnd() override;
  virtual bool skipBack (bool shift, bool control) override;
  virtual bool skipForward (bool shift, bool control) override;
  virtual void exit() override;

  // load
  void launchLoad (const std::vector<std::string>& params);
  void load (const std::vector<std::string>& params);

private:
  cLoadSource* mLoadSource = nullptr;
  cLoadSource* mLoadIdle = nullptr;
  std::vector <cLoadSource*> mLoadSources;
  };
