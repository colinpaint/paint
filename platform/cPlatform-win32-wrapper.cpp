// cPlatform-win32-wrapper.cpp - imGui backend win32Impl wrapper - abstracted from imGui win32DirectX11 example main.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cPlatform.h"

#include <cstdint>
#include <string>
#include <d3d11.h>
#include <tchar.h>

// glm
#include <vec2.hpp>

// imGui
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "../graphics/cPointRect.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

namespace {
  // vars
  cPlatform::sizeCallbackFunc gSizeCallback;

  ID3D11Device*            g_pd3dDevice = NULL;
  ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
  IDXGISwapChain*          g_pSwapChain = NULL;
  ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

  WNDCLASSEX wc;
  HWND hwnd;

  #ifndef WM_DPICHANGED
    #define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
  #endif
  //{{{
  void createRenderTarget() {

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
    }
  //}}}
  //{{{
  void cleanupRenderTarget() {

    if (g_mainRenderTargetView) {
      g_mainRenderTargetView->Release();
      g_mainRenderTargetView = NULL;
      }
    }
  //}}}
  //{{{
  bool createDeviceD3D(HWND hWnd) {

    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain (NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                                       createDeviceFlags, featureLevelArray, 2,
                                       D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                       &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
      return false;

    createRenderTarget();
    return true;
    }
  //}}}
  //{{{
  void cleanupDeviceD3D() {

    cleanupRenderTarget();

    if (g_pSwapChain) {
      g_pSwapChain->Release();
      g_pSwapChain = NULL;
      }

    if (g_pd3dDeviceContext) {
      g_pd3dDeviceContext->Release();
      g_pd3dDeviceContext = NULL;
      }

    if (g_pd3dDevice) {
      g_pd3dDevice->Release();
      g_pd3dDevice = NULL;
      }
    }
  //}}}

  //{{{
  LRESULT WINAPI WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  // Win32 message handler

    if (ImGui_ImplWin32_WndProcHandler (hWnd, msg, wParam, lParam))
      return true;

    switch (msg) {
      case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
          cleanupRenderTarget();
          g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
          createRenderTarget();
          }
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

    return ::DefWindowProc (hWnd, msg, wParam, lParam);
    }
  //}}}
  }

//{{{
bool cPlatform::init (const cPoint& windowSize, bool showViewports, const sizeCallbackFunc sizeCallback) {

  gSizeCallback = sizeCallback;

  //ImGui_ImplWin32_EnableDpiAwareness();
  wc = { sizeof(WNDCLASSEX),
         CS_CLASSDC, WndProc, 0L, 0L,
         GetModuleHandle (NULL), NULL, NULL, NULL, NULL,
         _T("ImGui Example"), NULL };
  ::RegisterClassEx(&wc);

  // Create application window
  hwnd = ::CreateWindow (wc.lpszClassName,
                         _T("Dear ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW,
                         100, 100, 1280, 800, NULL, NULL,
                         wc.hInstance, NULL);

  // Initialize Direct3D
  if (!createDeviceD3D(hwnd)) {
    cleanupDeviceD3D();
    ::UnregisterClass (wc.lpszClassName, wc.hInstance);
    return false;
    }

  // Show the window
  ::ShowWindow (hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow (hwnd);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
  //io.ConfigViewportsNoAutoMerge = true;
  //io.ConfigViewportsNoTaskBarIcon = true;
  //io.ConfigViewportsNoDefaultParent = true;
  //io.ConfigDockingAlwaysTabBar = true;
  //io.ConfigDockingTransparentPayload = true;
  //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
  //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
  ImGuiStyle& style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

  return true;
  }
//}}}
//{{{
void cPlatform::shutdown() {

  // Cleanup
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  cleanupDeviceD3D();
  ::DestroyWindow (hwnd);
  ::UnregisterClass (wc.lpszClassName, wc.hInstance);
  }
//}}}

// gets
//{{{
cPoint cPlatform::getWindowSize() {

  return cPoint (1280, 800);
  }
//}}}
//{{{
ID3D11Device* cPlatform::getDevice() {

  return g_pd3dDevice;
  }
//}}}
//{{{
ID3D11DeviceContext* cPlatform::getDeviceContext() {

  return g_pd3dDeviceContext;
  }
//}}}

// actions
//{{{
bool cPlatform::pollEvents() {
// Poll and handle messages (inputs, window resize, etc.)
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.

  MSG msg;
  while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
    if (msg.message == WM_QUIT)
      return false;
    }

  return true;
  }
//}}}
//{{{
void cPlatform::newFrame() {
  ImGui_ImplWin32_NewFrame();
  }
//}}}
//{{{
void cPlatform::present() {

  g_pSwapChain->Present(1, 0); // Present with vsync
  //g_pSwapChain->Present(0, 0); // Present without vsync
  }
//}}}
