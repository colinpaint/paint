// cGraphics.h - graphics singleton
#pragma once

class cGraphics {
public:
  //{{{
  static cGraphics& getInstance() {
  // singleton pattern create
  // - thread safe
  // - allocate with `new` in case singleton is not trivially destructible.

    static cGraphics* graphics = new cGraphics();
    return *graphics;
    }
  //}}}

  bool init();
  void shutdown();

  void draw();

private:
  // singleton pattern fluff
  cGraphics() = default;

  // delete copy/move so extra instances can't be created/moved
  cGraphics (const cGraphics&) = delete;
  cGraphics& operator = (const cGraphics&) = delete;
  cGraphics (cGraphics&&) = delete;
  cGraphics& operator = (cGraphics&&) = delete;
  };
