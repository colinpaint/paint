// cDx11App.cpp - win32 + dx11 app framework
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <cmath>
#include <string>
#include <array>
#include <algorithm>
#include <functional>

#include <stdio.h>
#include <tchar.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <tchar.h>

// imGui
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

#include "cApp.h"
#include "cPlatform.h"
#include "cGraphics.h"

#include "../ui/cUI.h"

#include "../utils/cLog.h"

using namespace std;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#ifndef WM_DPICHANGED
  #define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif
//}}}

// cPlatform interface
//{{{
class cWin32Platform : public cPlatform {
public:
  cWin32Platform (const string& name) : cPlatform (name, true, true) {}
  //{{{
  virtual ~cWin32Platform() {

    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext();

    mSwapChain->Release();
    mDeviceContext->Release();
    mDevice->Release();

    ::DestroyWindow (mHwnd);
    ::UnregisterClass (mWndClass.lpszClassName, mWndClass.hInstance);
    }
  //}}}

  //{{{
  virtual bool init (const cPoint& windowSize) final {

    mWindowSize = windowSize;

    // register app class
    mWndClass = { sizeof(WNDCLASSEX),
                  CS_CLASSDC, WndProc, 0L, 0L,
                  GetModuleHandle (NULL), NULL, NULL, NULL, NULL,
                 _T("paintbox"), NULL };
    ::RegisterClassEx (&mWndClass);

    // create application window
    mHwnd = ::CreateWindow (mWndClass.lpszClassName, _T((string("directX11") + " " + getName()).c_str()), WS_OVERLAPPEDWINDOW,
                            100, 100, windowSize.x, windowSize.y, NULL, NULL,
                            mWndClass.hInstance, NULL);

    // init direct3D
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory (&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = mHwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const UINT createDeviceFlags = 0; // D3D11_CREATE_DEVICE_DEBUG;
    const D3D_FEATURE_LEVEL kFeatureLevels[2] = { D3D_FEATURE_LEVEL_11_0,
                                                  D3D_FEATURE_LEVEL_10_0, };
    D3D_FEATURE_LEVEL featureLevel;
    if (D3D11CreateDeviceAndSwapChain (NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                                       createDeviceFlags, kFeatureLevels, 2,
                                       D3D11_SDK_VERSION, &swapChainDesc, &mSwapChain,
                                       &mDevice, &featureLevel, &mDeviceContext) != S_OK) {
      //{{{  error, return
      cLog::log (LOGINFO, "dx11 device create failed");
      ::UnregisterClass (mWndClass.lpszClassName, mWndClass.hInstance);
      return false;
      }
      //}}}

    cLog::log (LOGINFO, fmt::format ("dx11 device created - featureLevel:{:x}", featureLevel));

    // show window
    ::ShowWindow (mHwnd, SW_SHOWDEFAULT);
    ::UpdateWindow (mHwnd);

    // set imGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsClassic();
    cLog::log (LOGINFO, fmt::format ("imGui {} - {}", ImGui::GetVersion(), IMGUI_VERSION_NUM));

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // enable Keyboard Controls

    #if defined(BUILD_DOCKING)
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable; // enable docking
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // enable multiViewport
    #endif

    //ImGui::GetIO().ConfigViewportsNoAutoMerge = true;
    //ImGui::GetIO().ConfigViewportsNoTaskBarIcon = true;
    //ImGui::GetIO().ConfigViewportsNoDefaultParent = true;
    //ImGui::GetIO().ConfigDockingAlwaysTabBar = true;
    //ImGui::GetIO().ConfigDockingTransparentPayload = true;
    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

    #if defined(BUILD_DOCKING)
      ImGuiStyle& style = ImGui::GetStyle();
      if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
    #endif

    ImGui_ImplWin32_Init (mHwnd);

    return true;
    }
  //}}}

  ID3D11Device* getDevice() { return mDevice; }
  ID3D11DeviceContext* getDeviceContext() { return mDeviceContext; }
  IDXGISwapChain* getSwapChain() { return mSwapChain; }

  // gets
  virtual cPoint getWindowSize() final { return mWindowSize; }

  // sets
  //{{{
  virtual void setVsync (bool vsync) final { mVsync = vsync;   }
  //}}}
  //{{{
  virtual void toggleVsync() final { mVsync = !mVsync; }
  //}}}

  virtual void setFullScreen (bool fullScreen) final { mFullScreen = fullScreen; }
  virtual void toggleFullScreen() final { mFullScreen = !mFullScreen; }

  // actions
  //{{{
  virtual bool pollEvents() final {
  // Poll and handle messages (inputs, window resize, etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
  // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.

    MSG msg;
    while (::PeekMessage (&msg, NULL, 0U, 0U, PM_REMOVE)) {
      ::TranslateMessage (&msg);
      ::DispatchMessage (&msg);
      if (msg.message == WM_QUIT)
        return false;
      }

    return true;
    }
  //}}}
  virtual void newFrame() final { ImGui_ImplWin32_NewFrame(); }
  //{{{
  virtual void present() final {
    // update and render additional platform windows
    //if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    //  ImGui::UpdatePlatformWindows();
    //  ImGui::RenderPlatformWindowsDefault();
    //  }
    mSwapChain->Present (mVsync ? 1 : 0, 0);
    }
  //}}}

  inline static function <void (int width, int height)> gResizeCallback ;
  inline static function <void (vector<string> dropItems)> gDropCallback;

  //{{{
  //void dropCallback (GLFWwindow* window, int count, const char** paths) {

    //(void)window;

    //vector<string> dropItems;
    //for (int i = 0;  i < count;  i++)
      //dropItems.push_back (paths[i]);

    //gDropCallback (dropItems);
    //}
  //}}}

private:
  //{{{
  static LRESULT WINAPI WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  // win32 message handler

    // does imGui want it
    if (ImGui_ImplWin32_WndProcHandler (hWnd, msg, wParam, lParam))
      return true;

    // no
    switch (msg) {
      case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
          if (cApp::isPlatformDefined())
            gResizeCallback ((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
        return 0;

      case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
          return 0;
        break;

      case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

      case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
          //const int dpi = HIWORD(wParam);
          //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
          const RECT* suggested_rect = (RECT*)lParam;
          ::SetWindowPos (hWnd, NULL,
                          suggested_rect->left, suggested_rect->top,
                          suggested_rect->right - suggested_rect->left,
                          suggested_rect->bottom - suggested_rect->top,
                          SWP_NOZORDER | SWP_NOACTIVATE);
          }
        break;
      }

    // send back to windows default
    return ::DefWindowProc (hWnd, msg, wParam, lParam);
    }
  //}}}

  WNDCLASSEX mWndClass;
  HWND mHwnd;
  cPoint mWindowSize;

  ID3D11Device* mDevice = nullptr;
  ID3D11DeviceContext*  mDeviceContext = nullptr;
  IDXGISwapChain* mSwapChain = nullptr;
  };
//}}}

// cGraphics interface
//{{{
class cDx11Graphics : public cGraphics {
public:
  cDx11Graphics (ID3D11Device* device, ID3D11DeviceContext* deviceContext, IDXGISwapChain* swapChain)
    : mDevice(device), mDeviceContext(deviceContext), mSwapChain(swapChain) {}
  //{{{
  virtual ~cDx11Graphics() {
    ImGui_ImplDX11_Shutdown();
    mMainRenderTargetView->Release();
    }
  //}}}

  //{{{
  virtual bool init() final {

    // create mainRenderTargetView
    ID3D11Texture2D* backBufferTexture;
    mSwapChain->GetBuffer(0, IID_PPV_ARGS (&backBufferTexture));
    mDevice->CreateRenderTargetView (backBufferTexture, NULL, &mMainRenderTargetView);
    backBufferTexture->Release();

    return ImGui_ImplDX11_Init (mDevice, mDeviceContext);
    }
  //}}}
  virtual void newFrame() final { ImGui_ImplDX11_NewFrame(); }
  //{{{
  virtual void clear (const cPoint& size) final {
    (void)size;

    // set renderTarget
    mDeviceContext->OMSetRenderTargets (1, &mMainRenderTargetView, NULL);

    // dingy green clear color
    const float clearColor[4] = { 0.0f, 0.2f, 0.00f, 1.00f };
    mDeviceContext->ClearRenderTargetView (mMainRenderTargetView, clearColor);
    }
  //}}}
  //{{{
  virtual void renderDrawData() final {

    // set renderTarget
    mDeviceContext->OMSetRenderTargets (1, &mMainRenderTargetView, NULL);

    // render imgui
    ImGui_ImplDX11_RenderDrawData (ImGui::GetDrawData());
    }
  //}}}

  // create resources
  virtual cQuad* createQuad (const cPoint& size) final { return new cDx11Quad (size); }
  virtual cQuad* createQuad (const cPoint& size, const cRect& rect) final { return new cDx11Quad (size, rect); }

  virtual cTarget* createTarget() final { return new cDx11Target(); }
  //{{{
  virtual cTarget* createTarget (const cPoint& size, cTarget::eFormat format) final {
    return new cDx11Target (size, format);
    }
  //}}}
  //{{{
  virtual cTarget* createTarget (uint8_t* pixels, const cPoint& size, cTarget::eFormat format) final {
    return new cDx11Target (pixels, size, format);
    }
  //}}}

  virtual cLayerShader* createLayerShader() final { return new cDx11LayerShader(); }
  virtual cPaintShader* createPaintShader() final { return new cDx11PaintShader(); }

  //{{{
  virtual cTexture* createTexture (cTexture::eTextureType textureType, const cPoint& size) final {
  // factory create

    switch (textureType) {
      case cTexture::eRgba:   return new cDx11RgbaTexture (mDevice, mDeviceContext, textureType, size);
      case cTexture::eNv12:   return new cDx11Nv12Texture (mDevice, textureType, size);
      case cTexture::eYuv420: return new cDx11Yuv420Texture (mDevice, textureType, size);
      default : return nullptr;
      }
    }
  //}}}
  //{{{
  virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) final {
  // factory create

    switch (textureType) {
      case cTexture::eRgba:   return new cDx11RgbaShader();
      case cTexture::eYuv420: return new cDx11Yuv420Shader();
      case cTexture::eNv12:   return new cDx11Nv12Shader();
      default: return nullptr;
      }
    }
  //}}}

private:
  // !!! mostly unimplemented !!!
  //{{{
  class cDx11Quad : public cQuad {
  public:
    cDx11Quad (const cPoint& size) : cQuad(size) {
      }
    cDx11Quad (const cPoint& size, const cRect& rect) : cQuad(size) {
      (void)rect;
      }
    virtual ~cDx11Quad() {
      }

    void draw() final {
      }

  private:
    //inline static const uint32_t mNumIndices = 6;
    //inline static const uint8_t kIndices[mNumIndices] = {
    //  0, 1, 2, // 0   0-3
    //  0, 3, 1  // |\   \|
    //  };       // 2-1   1

    //uint32_t mVertexArrayObject = 0;
    //uint32_t mVertexBufferObject = 0;
    //uint32_t mElementArrayBufferObject = 0;
    };
  //}}}
  //{{{
  class cDx11Target : public cTarget {
  public:
    cDx11Target() : cTarget ({0,0}) {
      }
    cDx11Target (const cPoint& size, eFormat format) : cTarget(size) {
      (void)format;
      }
    cDx11Target (uint8_t* pixels, const cPoint& size, eFormat format) : cTarget(size) {
      (void)pixels;
      (void)format;
      }
    virtual ~cDx11Target() {
      free (mPixels);
      }

    /// gets
    uint8_t* getPixels() final {
      return mPixels;
      }

    // sets
    void setSize (const cPoint& size) final {
      (void)size;
      };
    void setTarget (const cRect& rect) final {
      (void)rect;
      }
    void setBlend() final {
      }
    void setSource() final {
      }

    // actions
    void invalidate() final {
      }
    void pixelsChanged (const cRect& rect) final {
      (void)rect;
      }

    void clear (const cColor& color) final {
      (void)color;
      }
    void blit (cTarget& src, const cPoint& srcPoint, const cRect& dstRect) final {
      (void)src;
      (void)srcPoint;
      (void)dstRect;
      }

    bool checkStatus() final { return true; }
    void reportInfo() final {
      }
    };
  //}}}

  //{{{
  class cDx11RgbaTexture : public cTexture {
  public:
    //{{{
    cDx11RgbaTexture (ID3D11Device* device, ID3D11DeviceContext* deviceContext,
                      eTextureType textureType, const cPoint& size)
        : cTexture(textureType, size), mDevice(device), mDeviceContext(deviceContext) {

      cLog::log (LOGINFO, fmt::format ("creating eRgba texture {}x{}", size.x, size.y));

      //  upload texture to graphics system
      D3D11_TEXTURE2D_DESC texture2dDesc;
      ZeroMemory (&texture2dDesc, sizeof(texture2dDesc));

      texture2dDesc.Width = size.x;
      texture2dDesc.Height = size.y;
      texture2dDesc.MipLevels = 1;
      texture2dDesc.ArraySize = 1;
      texture2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      texture2dDesc.SampleDesc.Count = 1;
      texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
      texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      texture2dDesc.CPUAccessFlags = 0;

      unsigned char* pixels = (unsigned char*)malloc (size.x*size.y*4);
      for (int i = 0; i < size.x*size.y*4; i++)
        pixels[i] = 0xc0;

      D3D11_SUBRESOURCE_DATA subResourceData;
      subResourceData.pSysMem = pixels;
      subResourceData.SysMemPitch = texture2dDesc.Width * 4;
      subResourceData.SysMemSlicePitch = 0;

      mTexture = nullptr;
      HRESULT hresult = mDevice->CreateTexture2D (&texture2dDesc, nullptr, &mTexture);
      if (hresult != S_OK)
        cLog::log (LOGERROR, fmt::format ("cDx11RgbaTexture CreateTexture2D failed {}", hresult));

      free (pixels);

      // create texture view
      D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
      ZeroMemory (&shaderResourceViewDesc, sizeof (shaderResourceViewDesc));

      shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      shaderResourceViewDesc.Texture2D.MipLevels = texture2dDesc.MipLevels;
      shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

      hresult = mDevice->CreateShaderResourceView (mTexture, &shaderResourceViewDesc, &mTextureView);
      if (hresult != S_OK)
        cLog::log (LOGERROR, fmt::format ("cDx11RgbaTexture CreateShaderResourceView failed {}", hresult));

      //  create texture sampler
      D3D11_SAMPLER_DESC samplerDesc;
      ZeroMemory (&samplerDesc, sizeof(samplerDesc));

      samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.MipLODBias = 0.f;
      samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
      samplerDesc.MinLOD = 0.f;
      samplerDesc.MaxLOD = 0.f;

      hresult = mDevice->CreateSamplerState (&samplerDesc, &mSampler);
      if (hresult != S_OK)
        cLog::log (LOGERROR, fmt::format ("cDx11RgbaTexture CreateSamplerState failed {}", hresult));
      }
    //}}}
    //{{{
    virtual ~cDx11RgbaTexture() {

      if (mSampler)
        mSampler->Release();
      if (mTextureView)
        mTextureView->Release();
      if (mTexture)
        mTexture->Release();
      }
    //}}}

    virtual void* getTextureId() final { return (void*)mTextureView; }

    virtual void setPixels (uint8_t** pixels) final {

      cLog::log (LOGINFO, fmt::format ("cDx11RgbaTexture setPixels {} {}", mSize.x, mSize.y));

      D3D11_MAPPED_SUBRESOURCE mappedSubResource;
      HRESULT hresult = mDeviceContext->Map (mTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource);
      if (hresult != S_OK) {
        cLog::log (LOGINFO, fmt::format ("cDx11RgbaTexture setPixels failed {}", hresult));
        return;
        }
      memcpy (mappedSubResource.pData, pixels[0], mSize.x * mSize.y * 4);
      mDeviceContext->Unmap (mTexture, 0);
      }

    virtual void setSource() final {
      }

  private:
    ID3D11Device* mDevice = nullptr;
    ID3D11DeviceContext* mDeviceContext = nullptr;

    ID3D11Texture2D* mTexture = nullptr;
    ID3D11ShaderResourceView* mTextureView = nullptr;
    ID3D11SamplerState* mSampler = nullptr;
    };
  //}}}
  //{{{
  class cDx11Nv12Texture : public cTexture {
  public:
    cDx11Nv12Texture (ID3D11Device* device, eTextureType textureType, const cPoint& size)
        : cTexture(textureType, size), mDevice(device)  {
      cLog::log (LOGINFO, fmt::format ("creating eNv12 texture {}x{}", size.x, size.y));
      }

    virtual ~cDx11Nv12Texture() {
      cLog::log (LOGINFO, fmt::format ("deleting eVv12 texture {}x{}", mSize.x, mSize.y));

      for (size_t i = 0; i < mSampler.size(); i++)
        if (mSampler[i])
          mSampler[i]->Release();
      for (size_t i = 0; i < mTextureView.size(); i++)
        if (mTextureView[i])
          mTextureView[i]->Release();
      for (size_t i = 0; i < mTexture.size(); i++)
        if (mTexture[i])
          mTexture[i]->Release();

      }

    virtual void* getTextureId() final { return (void*)mTextureView[0]; }  // luma only

    virtual void setPixels (uint8_t** pixels) final {
      (void)pixels;
      }

    virtual void setSource() final {
      }

  private:
    ID3D11Device* mDevice = nullptr;
    array <ID3D11Texture2D*,2> mTexture = {nullptr};
    array <ID3D11ShaderResourceView*,2> mTextureView = {nullptr};
    array <ID3D11SamplerState*,2> mSampler = {nullptr};
    };
  //}}}
  //{{{
  class cDx11Yuv420Texture : public cTexture {
  public:
    cDx11Yuv420Texture (ID3D11Device* device, eTextureType textureType, const cPoint& size)
        : cTexture(textureType, size), mDevice(device) {
      cLog::log (LOGINFO, fmt::format ("creating eYuv420 texture {}x{}", size.x, size.y));
      }

    virtual ~cDx11Yuv420Texture() {
      cLog::log (LOGINFO, fmt::format ("deleting eYuv420 texture {}x{}", mSize.x, mSize.y));

      for (size_t i = 0; i < mSampler.size(); i++)
        if (mSampler[i])
          mSampler[i]->Release();
      for (size_t i = 0; i < mTextureView.size(); i++)
        if (mTextureView[i])
          mTextureView[i]->Release();
      for (size_t i = 0; i < mTexture.size(); i++)
        if (mTexture[i])
          mTexture[i]->Release();
      }

    virtual void* getTextureId() final { return (void*)mTextureView[0]; }  // luma only

    virtual void setPixels (uint8_t** pixels) final {
    // set textures using pixels in ffmpeg avFrame format
      (void)pixels;
      }
    virtual void setSource() final {
      }
  private:
    ID3D11Device* mDevice = nullptr;
    array <ID3D11Texture2D*,3> mTexture = {nullptr};
    array <ID3D11ShaderResourceView*,3> mTextureView = {nullptr};
    array <ID3D11SamplerState*,3> mSampler = {nullptr};
    };
  //}}}

  //{{{
  class cDx11RgbaShader : public cTextureShader {
  public:
    cDx11RgbaShader() : cTextureShader() {
      mId = 0;
      }
    virtual ~cDx11RgbaShader() {
      }

    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;
      }
    virtual void use() final {
      }
    };
  //}}}
  //{{{
  class cDx11Nv12Shader : public cTextureShader {
  public:
    cDx11Nv12Shader() : cTextureShader() {
      mId = 0;
      }

    virtual ~cDx11Nv12Shader() {
      }

    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;

      }
    virtual void use() final {
      }
    };
  //}}}
  //{{{
  class cDx11Yuv420Shader : public cTextureShader {
  public:
    cDx11Yuv420Shader() : cTextureShader() {
      mId = 0;
      }
    virtual ~cDx11Yuv420Shader() {
      }

    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;

      }
    virtual void use() final {
      }
    };
  //}}}

  //{{{
  class cDx11LayerShader : public cLayerShader {
  public:
    cDx11LayerShader() : cLayerShader() {
      mId = 0;
      }
    virtual ~cDx11LayerShader() {
      }

    // sets
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;

      }
    virtual void setHueSatVal (float hue, float sat, float val) final {
      (void)hue;
      (void)sat;
      (void)val;
      }

    virtual void use() final {
      }
    };
  //}}}
  //{{{
  class cDx11PaintShader : public cPaintShader {
  public:
    cDx11PaintShader() : cPaintShader() {
      mId = 0;
      }
    virtual ~cDx11PaintShader() {
      }

    // sets
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;

      }
    virtual void setStroke (cVec2 pos, cVec2 prevPos, float radius, const cColor& color) final {
      (void)pos;
      (void)prevPos;
      (void)radius;
      (void)color;
      }

    virtual void use() final {
      }
    };
  //}}}

  ID3D11Device* mDevice = nullptr;
  ID3D11DeviceContext* mDeviceContext = nullptr;
  IDXGISwapChain* mSwapChain = nullptr;
  ID3D11RenderTargetView* mMainRenderTargetView = nullptr;
  };
//}}}

// cApp
//{{{
cApp::cApp (const string& name, const cPoint& windowSize, bool fullScreen, bool vsync) {

  // create platform
  cWin32Platform* win32Platform = new cWin32Platform (name);
  if (!win32Platform || !win32Platform->init (windowSize)) {
    cLog::log (LOGERROR, "cApp platform init failed");
    return;
    }

  // create graphics
  mGraphics = new cDx11Graphics (win32Platform->getDevice(), win32Platform->getDeviceContext(),
                                 win32Platform->getSwapChain());
  if (!mGraphics || !mGraphics->init()) {
    cLog::log (LOGERROR, "cApp graphics init failed");
    return;
    }

  // set callbacks
  win32Platform->gResizeCallback = [&](int width, int height) noexcept { windowResize (width, height); };
  win32Platform->gDropCallback = [&](vector<string> dropItems) noexcept { drop (dropItems); };

  // fullScreen, vsync
  mPlatform = win32Platform;
  mPlatform->setFullScreen (fullScreen);
  mPlatform->setVsync (vsync);
  mPlatformDefined = true;
  }
//}}}
//{{{
cApp::~cApp() {

  delete mGraphics;
  delete mPlatform;
  }
//}}}

// get
//{{{
chrono::system_clock::time_point cApp::getNow() {
// get time_point with daylight saving correction
// - should be a C++20 timezone thing, but not yet

  // get daylight saving flag
  time_t current_time;
  time (&current_time);
  struct tm* timeinfo = localtime (&current_time);
  //cLog::log (LOGINFO, fmt::format ("dst {}", timeinfo->tm_isdst));

  // UTC->BST only
  return std::chrono::system_clock::now() + std::chrono::hours ((timeinfo->tm_isdst == 1) ? 1 : 0);
  }
//}}}

// callback
//{{{
void cApp::windowResize (int width, int height) {

  (void)width;
  (void)height;

  mGraphics->newFrame();
  mPlatform->newFrame();
  ImGui::NewFrame();

  cUI::render (*this);
  ImGui::Render();
  mGraphics->renderDrawData();

  mPlatform->present();
  }
//}}}

// main loop
//{{{
void cApp::mainUILoop() {

  while (mPlatform->pollEvents()) {
    mGraphics->newFrame();
    mPlatform->newFrame();
    ImGui::NewFrame();

    cUI::render (*this);
    ImGui::Render();
    mGraphics->renderDrawData();

    #if defined(BUILD_DOCKING)
      if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        // update, render additional platform windows
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        }
    #endif

    mPlatform->present();
    }
  }
//}}}
