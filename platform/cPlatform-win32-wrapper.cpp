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
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "../graphics/cPointRect.h"
#include "../log/cLog.h"

#ifndef WM_DPICHANGED
  #define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

using namespace std;
using namespace fmt;
//}}}
namespace {
  // vars
  WNDCLASSEX gWndClass;
  HWND gHWnd;

  ID3D11Device* gD3dDevice = NULL;
  ID3D11DeviceContext*  gD3dDeviceContext = NULL;

  IDXGISwapChain* gSwapChain = NULL;
  ID3D11RenderTargetView* gMainRenderTargetView = NULL;

  //{{{
  void createRenderTarget() {

    ID3D11Texture2D* pBackBuffer;
    gSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    gD3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &gMainRenderTargetView);
    pBackBuffer->Release();
    }
  //}}}
  //{{{
  void cleanupRenderTarget() {

    if (gMainRenderTargetView) {
      gMainRenderTargetView->Release();
      gMainRenderTargetView = NULL;
      }
    }
  //}}}
  //{{{
  void cleanupDeviceD3D() {

    cleanupRenderTarget();

    if (gSwapChain) {
      gSwapChain->Release();
      gSwapChain = NULL;
      }

    if (gD3dDeviceContext) {
      gD3dDeviceContext->Release();
      gD3dDeviceContext = NULL;
      }

    if (gD3dDevice) {
      gD3dDevice->Release();
      gD3dDevice = NULL;
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
        if (gD3dDevice != NULL && wParam != SIZE_MINIMIZED) {
          cleanupRenderTarget();
          gSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
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

// cPlatform
//{{{
bool cPlatform::init (const cPoint& windowSize, bool showViewports, const sizeCallbackFunc sizeCallback) {

  //ImGui_ImplWin32_EnableDpiAwareness();
  gWndClass = { sizeof(WNDCLASSEX),
                CS_CLASSDC, WndProc, 0L, 0L,
                GetModuleHandle (NULL), NULL, NULL, NULL, NULL,
               _T("ImGui Example"), NULL };
  ::RegisterClassEx (&gWndClass);

  // Create application window
  gHWnd = ::CreateWindow (gWndClass.lpszClassName,
                          _T("Dear ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW,
                          100, 100, 1280, 800, NULL, NULL,
                          gWndClass.hInstance, NULL);

  // Initialize Direct3D
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
  const D3D_FEATURE_LEVEL kFeatureLevels[2] = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_0, };
  D3D_FEATURE_LEVEL featureLevel;
  if (D3D11CreateDeviceAndSwapChain (NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                                     createDeviceFlags, kFeatureLevels, 2,
                                     D3D11_SDK_VERSION, &swapChainDesc, &gSwapChain,
                                     &gD3dDevice, &featureLevel, &gD3dDeviceContext) != S_OK) {
    cleanupDeviceD3D();
    ::UnregisterClass (gWndClass.lpszClassName, gWndClass.hInstance);
    return false;
    }

  createRenderTarget();

  // Show the window
  ::ShowWindow (gHWnd, SW_SHOWDEFAULT);
  ::UpdateWindow (gHWnd);

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


  ImGui_ImplWin32_Init (gHWnd);

  return true;
  }
//}}}
//{{{
void cPlatform::shutdown() {

  // Cleanup
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  cleanupDeviceD3D();

  ::DestroyWindow (gHWnd);
  ::UnregisterClass (gWndClass.lpszClassName, gWndClass.hInstance);
  }
//}}}

// gets
void* cPlatform::getDevice() { return (void*)gD3dDevice; }
void* cPlatform::getDeviceContext() { return (void*)gD3dDeviceContext; }
cPoint cPlatform::getWindowSize() { return cPoint (1280, 800); }

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
void cPlatform::selectMainScreen() {
  const float kClearColorWithAlpha[4] = { 0.f,0.f,0.f, 1.f };
  gD3dDeviceContext->OMSetRenderTargets (1, &gMainRenderTargetView, NULL);
  gD3dDeviceContext->ClearRenderTargetView (gMainRenderTargetView, kClearColorWithAlpha);
  }
//}}}
//{{{
void cPlatform::present() {

  // update and uender additional platform windows
  ImGuiIO& io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    }

  gSwapChain->Present(1, 0); // Present with vsync
  //gSwapChain->Present(0, 0); // Present without vsync
  }
//}}}
