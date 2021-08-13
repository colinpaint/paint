// cGraphics.h - graphics singleton
#pragma once
struct ID3D11Device;
struct ID3D11DeviceContext;
  ID3D11Device* getDevice();
  ID3D11DeviceContext* getDeviceContext();

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

  bool init (ID3D11Device* device, ID3D11DeviceContext* deviceContext);
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
