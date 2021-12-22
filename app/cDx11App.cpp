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

#include <stdio.h>
#include <tchar.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <tchar.h>

// imGui
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

#ifdef USE_IMPLOT
  #include <implot.h>
#endif

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

// platform
//{{{
namespace {
  cPoint gWindowSize;
  bool gVsync = true;
  bool gFullScreen = false;

  cPlatform* gPlatform = nullptr;

  WNDCLASSEX gWndClass;
  HWND gHWnd;

  ID3D11Device* gD3dDevice = NULL;
  ID3D11DeviceContext*  gD3dDeviceContext = NULL;
  IDXGISwapChain* gSwapChain = NULL;

  //{{{
  LRESULT WINAPI WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  // win32 message handler

    // does imGui want it
    if (ImGui_ImplWin32_WndProcHandler (hWnd, msg, wParam, lParam))
      return true;

    // no
    switch (msg) {
      case WM_SIZE:
        //if (wParam != SIZE_MINIMIZED)
          //if (gPlatform)
          //  gPlatform->mResizeCallback ((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
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
  }
//}}}
//{{{
class cWin32Platform : public cPlatform {
public:

  cWin32Platform (const string& name) : cPlatform (name, true, true) {}
  //{{{
  virtual ~cWin32Platform() {

    ImGui_ImplWin32_Shutdown();

    #ifdef USE_IMPLOT
      ImPlot::DestroyContext();
    #endif
    ImGui::DestroyContext();

    gSwapChain->Release();
    gD3dDeviceContext->Release();
    gD3dDevice->Release();

    ::DestroyWindow (gHWnd);
    ::UnregisterClass (gWndClass.lpszClassName, gWndClass.hInstance);
    }
  //}}}

  //{{{
  virtual bool init (const cPoint& windowSize) final {

    // register app class
    gWndClass = { sizeof(WNDCLASSEX),
                  CS_CLASSDC, WndProc, 0L, 0L,
                  GetModuleHandle (NULL), NULL, NULL, NULL, NULL,
                 _T("paintbox"), NULL };
    ::RegisterClassEx (&gWndClass);

    // create application window
    gWindowSize = windowSize;
    gHWnd = ::CreateWindow (gWndClass.lpszClassName,
                            _T("paintbox - directX11"), WS_OVERLAPPEDWINDOW,
                            100, 100, windowSize.x, windowSize.y, NULL, NULL,
                            gWndClass.hInstance, NULL);

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
    swapChainDesc.OutputWindow = gHWnd;
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
                                       D3D11_SDK_VERSION, &swapChainDesc, &gSwapChain,
                                       &gD3dDevice, &featureLevel, &gD3dDeviceContext) != S_OK) {
      //{{{  error, return
      cLog::log (LOGINFO, "DirectX device created failed");
      ::UnregisterClass (gWndClass.lpszClassName, gWndClass.hInstance);
      return false;
      }
      //}}}
    cLog::log (LOGINFO, fmt::format ("platform DirectX11 device created - featureLevel:{:x}", featureLevel));

    // Show the window
    ::ShowWindow (gHWnd, SW_SHOWDEFAULT);
    ::UpdateWindow (gHWnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    #ifdef USE_IMPLOT
      ImPlot::CeateContext();
    #endif

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //ImGui::GetIO().ConfigViewportsNoAutoMerge = true;
    //ImGui::GetIO().ConfigViewportsNoTaskBarIcon = true;
    //ImGui::GetIO().ConfigViewportsNoDefaultParent = true;
    //ImGui::GetIO().ConfigDockingAlwaysTabBar = true;
    //ImGui::GetIO().ConfigDockingTransparentPayload = true;
    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

    // if viewports enabled, tweak WindowRounding/WindowBg so platform windows look identical
    ImGuiStyle& style = ImGui::GetStyle();
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      style.WindowRounding = 0.0f;
      style.Colors[ImGuiCol_WindowBg].w = 1.0f;
      }

    ImGui_ImplWin32_Init (gHWnd);

    gPlatform = this;

    return true;
    }
  //}}}

  // gets
  virtual cPoint getWindowSize() final;

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
    gSwapChain->Present (mVsync ? 1 : 0, 0);
    }
  };
  //}}}

//}}}

//{{{
//class cDx11Graphics : public cGraphics {
//public:
  //{{{
  //virtual ~cDx11Graphics() {
    //ImGui_ImplDX11_Shutdown();
    //}
  //}}}

  //{{{
  //virtual bool init() final {

    //// report Dx11 versions
    ////cLog::log (LOGINFO, fmt::format ("OpenGL {}", glGetString (GL_VERSION)));

    //return ImGui_ImplDx11_Init();
    //}
  //}}}
  //virtual void newFrame() final { ImGui_ImplDX11_NewFrame(); }
  //{{{
  //virtual void clear (const cPoint& size) final {
    //}
  //}}}
  //virtual void renderDrawData() final { ImGui_ImplDX11_RenderDrawData (ImGui::GetDrawData()); }

  //// create resources
  //virtual cQuad* createQuad (const cPoint& size) final { return new cDx11Quad (size); }
  //virtual cQuad* createQuad (const cPoint& size, const cRect& rect) final { return new cDx11Quad (size, rect); }

  //virtual cTarget* createTarget() final { return new cDx11Target(); }
  //{{{
  //virtual cTarget* createTarget (const cPoint& size, cTarget::eFormat format) final {
    //return new cDx11Target (size, format);
    //}
  //}}}
  //{{{
  //virtual cTarget* createTarget (uint8_t* pixels, const cPoint& size, cTarget::eFormat format) final {
    //return new cDx11Target (pixels, size, format);
    //}
  //}}}

  //virtual cLayerShader* createLayerShader() final { return new cDx11LayerShader(); }
  //virtual cPaintShader* createPaintShader() final { return new cDx11PaintShader(); }

  //{{{
  //virtual cTexture* createTexture (cTexture::eTextureType textureType, const cPoint& size) final {
  //// factory create

    //switch (textureType) {
      //case cTexture::eRgba:   return new cDx11RgbaTexture (textureType, size);
      //case cTexture::eNv12:   return new cDx11Nv12Texture (textureType, size);
      //case cTexture::eYuv420: return new cDx11Yuv420Texture (textureType, size);
      //default : return nullptr;
      //}
    //}
  //}}}
  //{{{
  //virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) final {
  //// factory create

    //switch (textureType) {
      //case cTexture::eRgba:   return new cDx11RgbaShader();
      //case cTexture::eYuv420: return new cDx11Yuv420Shader();
      //case cTexture::eNv12:   return new cDx11Nv12Shader();
      //default: return nullptr;
      //}
    //}
  //}}}

//private:
  //{{{
  //class cDx11Quad : public cQuad {
  //public:
    //cDx11Quad (const cPoint& size) : cQuad(size) {
      //}
    //cDx11Quad (const cPoint& size, const cRect& rect) : cQuad(size) {
      //}
    //virtual ~cDx11Quad() {
      //}

    //void draw() final {
      //}

  //private:
    ////inline static const uint32_t mNumIndices = 6;
    ////inline static const uint8_t kIndices[mNumIndices] = {
    ////  0, 1, 2, // 0   0-3
    ////  0, 3, 1  // |\   \|
    ////  };       // 2-1   1

    ////uint32_t mVertexArrayObject = 0;
    ////uint32_t mVertexBufferObject = 0;
    ////uint32_t mElementArrayBufferObject = 0;
    //};
  //}}}
  //{{{
  //class cDx11Target : public cTarget {
  //public:
    //cDx11Target() : cTarget ({0,0}) {
      //}
    //cDx11Target (const cPoint& size, eFormat format) : cTarget(size) {
      //}
    //cDx11Target (uint8_t* pixels, const cPoint& size, eFormat format) : cTarget(size) {
      //}
    //virtual ~cDx11Target() {
      //free (mPixels);
      //}

    ///// gets
    //uint8_t* getPixels() final {
      //return mPixels;
      //}

    //// sets
    //void setSize (const cPoint& size) final {
      //};
    //void setTarget (const cRect& rect) final {
      //}
    //void setBlend() final {
      //}
    //void setSource() final {
      //}

    //// actions
    //void invalidate() final {
      //}
    //void pixelsChanged (const cRect& rect) final {
      //}

    //void clear (const cColor& color) final {
      //}
    //void blit (cTarget& src, const cPoint& srcPoint, const cRect& dstRect) final {
      //}

    //bool checkStatus() final {
      //}
    //void reportInfo() final {
      //}
    //};
  //}}}

  //{{{
  //class cDx11RgbaTexture : public cTexture {
  //public:
    //cDx11RgbaTexture (eTextureType textureType, const cPoint& size)
        //: cTexture(textureType, size) {

      //cLog::log (LOGINFO, fmt::format ("creating eRgba texture {}x{}", size.x, size.y));
      //}
    //virtual ~cDx11RgbaTexture() {
      //}

    //virtual unsigned getTextureId() const final { return mTextureId; }

    //virtual void setPixels (uint8_t** pixels) final {
      //}
    //virtual void setSource() final {
      //}

  //private:
    //uint32_t mTextureId = 0;
    //};
  //}}}
  //{{{
  //class cDx11Nv12Texture : public cTexture {
  //public:
    //cDx11Nv12Texture (eTextureType textureType, const cPoint& size)
        //: cTexture(textureType, size) {

      //cLog::log (LOGINFO, fmt::format ("creating eNv12 texture {}x{}", size.x, size.y));
    //virtual ~cDx11Nv12Texture() {
      //cLog::log (LOGINFO, fmt::format ("deleting eVv12 texture {}x{}", mSize.x, mSize.y));
      //}

    //virtual unsigned getTextureId() const final { return mTextureId[0]; }  // luma only

    //virtual void setPixels (uint8_t** pixels) final {
      //}
    //virtual void setSource() final {
      //}

  //private:
    //array <uint32_t,2> mTextureId;
    //};
  //}}}
  //{{{
  //class cDx11Yuv420Texture : public cTexture {
  //public:
    //cDx11Yuv420Texture (eTextureType textureType, const cPoint& size)
        //: cTexture(textureType, size) {
      //cLog::log (LOGINFO, fmt::format ("creating eYuv420 texture {}x{}", size.x, size.y));
      //}
    //virtual ~cDx11Yuv420Texture() {
      //cLog::log (LOGINFO, fmt::format ("deleting eYuv420 texture {}x{}", mSize.x, mSize.y));
      //}

    //virtual unsigned getTextureId() const final { return mTextureId[0]; }   // luma only

    //virtual void setPixels (uint8_t** pixels) final {
    //// set textures using pixels in ffmpeg avFrame format
      //}
    //virtual void setSource() final {
      //}
  //private:
    //array <uint32_t,3> mTextureId;
    //};
  //}}}

  //{{{
  //class cDx11RgbaShader : public cTextureShader {
  //public:
    //cDx11RgbaShader() : cTextureShader() {
      //mId = 0;
      //}
    //virtual ~cDx11RgbaShader() {
      //}

    //virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      //}
    //virtual void use() final {
      //}
    //};
  //}}}
  //{{{
  //class cDx11Nv12Shader : public cTextureShader {
  //public:
    //cDx11Nv12Shader() : cTextureShader() {
      //mId = 0;
      //}

    //virtual ~cDx11Nv12Shader() {
      //}

    //virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      //}
    //virtual void use() final {
      //}
    //};
  //}}}
  //{{{
  //class cDx11Yuv420Shader : public cTextureShader {
  //public:
    //cDx11Yuv420Shader() : cTextureShader() {
      //mId = 0;
      //}
    //virtual ~cDx11Yuv420Shader() {
      //}

    //virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      //}
    //virtual void use() final {
      //}
    //};
  //}}}

  //{{{
  //class cDx11LayerShader : public cLayerShader {
  //public:
    //cDx11LayerShader() : cLayerShader() {
      //mId = 0;
      //}
    //virtual ~cDx11LayerShader() {
      //}

    //// sets
    //virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      //}
    //virtual void setHueSatVal (float hue, float sat, float val) final {
      //}

    //virtual void use() final {
      //}
    //};
  //}}}
  //{{{
  //class cDx11PaintShader : public cPaintShader {
  //public:
    //cDx11PaintShader() : cPaintShader() {
      //mId = 0;
      //}
    //virtual ~cDx11PaintShader() {
      //}

    //// sets
    //virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      //}
    //virtual void setStroke (cVec2 pos, cVec2 prevPos, float radius, const cColor& color) final {
      //}

    //virtual void use() final {
      //}
    //};
  //}}}
  //};
//}}}

// cApp
//{{{
cApp::cApp (const string& name, const cPoint& windowSize, bool fullScreen, bool vsync) {

  // create platform
  mPlatform = new cWin32Platform (name);
  if (!mPlatform || !mPlatform->init (windowSize)) {
    cLog::log (LOGERROR, "cApp platform init failed");
    return;
    }

  // create graphics
  //mGraphics = new cOpenDx11Graphics();
  //if (!mGraphics || !mGraphics->init()) {
  //  cLog::log (LOGERROR, "cApp graphics init failed");
  //  return;
  //  }

  // fullScreen, vsync
  mPlatform->setFullScreen (fullScreen);
  mPlatform->setVsync (vsync);
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

    mPlatform->present();
    }
  }
//}}}
