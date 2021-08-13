// cPlatform-Win32.h
//{{{  includes
#include "cPlatform.h"

#include <imgui.h>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tchar.h>
#include <dwmapi.h>

#include "../graphics/cPointRect.h"
#include "../log/cLog.h"

// Configuration flags to add in your imconfig.h file:
//#define IMGUI_IMPL_WIN32_DISABLE_GAMEPAD              // Disable gamepad support. This was meaningful before <1.81 but we now load XInput dynamically so the option is now less relevant.

// Using XInput for gamepad (will load DLL dynamically)
#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
  #include <xinput.h>
  typedef DWORD (WINAPI *PFN_XInputGetCapabilities)(DWORD, DWORD, XINPUT_CAPABILITIES*);
  typedef DWORD (WINAPI *PFN_XInputGetState)(DWORD, XINPUT_STATE*);
#endif

// Allow compilation with old Windows SDK. MinGW doesn't have default _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
  #define WM_MOUSEHWHEEL 0x020E
#endif

#ifndef DBT_DEVNODES_CHANGED
  #define DBT_DEVNODES_CHANGED 0x0007
#endif

#define isWindowsVistaOrGreater()   isWindowsVersionOrGreater(HIBYTE(0x0600), LOBYTE(0x0600), 0) // _WIN32_WINNT_VISTA
#define isWindows8OrGreater()       isWindowsVersionOrGreater(HIBYTE(0x0602), LOBYTE(0x0602), 0) // _WIN32_WINNT_WIN8
#define isWindows8Point1OrGreater() isWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0) // _WIN32_WINNT_WINBLUE
#define isWindows10OrGreater()      isWindowsVersionOrGreater(HIBYTE(0x0A00), LOBYTE(0x0A00), 0) // _WIN32_WINNT_WINTHRESHOLD / _WIN32_WINNT_WIN10

#ifndef DPI_ENUMS_DECLARED
  typedef enum { PROCESS_DPI_UNAWARE = 0, PROCESS_SYSTEM_DPI_AWARE = 1, PROCESS_PER_MONITOR_DPI_AWARE = 2 } PROCESS_DPI_AWARENESS;
  typedef enum { MDT_EFFECTIVE_DPI = 0, MDT_ANGULAR_DPI = 1, MDT_RAW_DPI = 2, MDT_DEFAULT = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;
#endif

#ifndef _DPI_AWARENESS_CONTEXTS_
  DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
  #define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    (DPI_AWARENESS_CONTEXT)-3
#endif

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
  #define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT)-4
#endif

typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);                     // Shcore.lib + dll, Windows 8.1+
typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);        // Shcore.lib + dll, Windows 8.1+
typedef DPI_AWARENESS_CONTEXT(WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT); // User32.lib + dll, Windows 10 v1607+ (Creators Update)
#if defined(_MSC_VER) && !defined(NOGDI)
  #pragma comment(lib, "gdi32")   // Link with gdi32.lib for GetDeviceCaps(). MinGW will require linking with '-lgdi32'
#endif
// IME (Input Method Editor) basic support for e.g. Asian language users
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP) // UWP doesn't have Win32 functions
  #define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#endif

#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS)
  #define HAS_WIN32_IME 1
  #include <imm.h>
  #ifdef _MSC_VER
    #pragma comment(lib, "imm32")
  #endif
  //{{{
  static void setImeInputPos(ImGuiViewport* viewport, ImVec2 pos)
  {
      COMPOSITIONFORM cf = { CFS_FORCE_POSITION,{ (LONG)(pos.x - viewport->Pos.x), (LONG)(pos.y - viewport->Pos.y) },{ 0, 0, 0, 0 } };
      if (HWND hwnd = (HWND)viewport->PlatformHandle)
          if (HIMC himc = ::ImmGetContext(hwnd))
          {
              ::ImmSetCompositionWindow(himc, &cf);
              ::ImmReleaseContext(hwnd, himc);
          }
  }
  //}}}
#else
  #define HAS_WIN32_IME 0
#endif

#pragma comment (lib, "dwmapi")  // Link with dwmapi.lib. MinGW will require linking with '-ldwmapi'

using namespace std;
using namespace fmt;
//}}}

namespace {
  //{{{
  // Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend data.
  struct sViewportData {
    HWND    Hwnd;
    bool    HwndOwned;
    DWORD   DwStyle;
    DWORD   DwExStyle;

    sViewportData() { Hwnd = NULL; HwndOwned = false;  DwStyle = DwExStyle = 0; }
    ~sViewportData() { IM_ASSERT(Hwnd == NULL); }
    };
  //}}}
  //{{{
  struct sBackendData {
    HWND                        hWnd;
    HWND                        MouseHwnd;
    bool                        MouseTracked;
    INT64                       Time;
    INT64                       TicksPerSecond;
    ImGuiMouseCursor            LastMouseCursor;
    bool                        HasGamepad;
    bool                        WantUpdateHasGamepad;
    bool                        WantUpdateMonitors;

    #ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
      HMODULE                     XInputDLL;
      PFN_XInputGetCapabilities   XInputGetCapabilities;
      PFN_XInputGetState          XInputGetState;
    #endif

    sBackendData()      { memset(this, 0, sizeof(*this)); }
    };
  //}}}
  //{{{
  // Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
  // It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
  // FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
  // FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
  sBackendData* getBackendData() {
    return ImGui::GetCurrentContext() ? (sBackendData*)ImGui::GetIO().BackendPlatformUserData : NULL;
    }
  //}}}

  //{{{
  bool updateMouseCursor() {

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
      return false;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
      // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
      ::SetCursor(NULL);
      }
    else {
      // Show OS mouse cursor
      LPTSTR win32_cursor = IDC_ARROW;
      switch (imgui_cursor) {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
      ::SetCursor(::LoadCursor(NULL, win32_cursor));
      }

    return true;
    }
  //}}}
  //{{{
  // This code supports multi-viewports (multiple OS Windows mapped into different Dear ImGui viewports)
  // Because of that, it is a little more complicated than your typical single-viewport binding code!
  void updateMousePos()
  {
      sBackendData* backendData = getBackendData();
      ImGuiIO& io = ImGui::GetIO();
      IM_ASSERT(backendData->hWnd != 0);

      const ImVec2 mouse_pos_prev = io.MousePos;
      io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
      io.MouseHoveredViewport = 0;

      // Obtain focused and hovered window. We forward mouse input when focused or when hovered (and no other window is capturing)
      HWND focused_window = ::GetForegroundWindow();
      HWND hovered_window = backendData->MouseHwnd;
      HWND mouse_window = NULL;
      if (hovered_window && (hovered_window == backendData->hWnd || ::IsChild(hovered_window, backendData->hWnd) || ImGui::FindViewportByPlatformHandle((void*)hovered_window)))
          mouse_window = hovered_window;
      else if (focused_window && (focused_window == backendData->hWnd || ::IsChild(focused_window, backendData->hWnd) || ImGui::FindViewportByPlatformHandle((void*)focused_window)))
          mouse_window = focused_window;
      if (mouse_window == NULL)
          return;

      // Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
      // (When multi-viewports are enabled, all Dear ImGui positions are same as OS positions)
      if (io.WantSetMousePos)
      {
          POINT pos = { (int)mouse_pos_prev.x, (int)mouse_pos_prev.y };
          if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0)
              ::ClientToScreen(mouse_window, &pos);
          ::SetCursorPos(pos.x, pos.y);
      }

      // Set Dear ImGui mouse position from OS position
      POINT mouse_screen_pos;
      if (!::GetCursorPos(&mouse_screen_pos))
          return;
      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
      {
          // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
          // This is the position you can get with ::GetCursorPos() or WM_MOUSEMOVE + ::ClientToScreen(). In theory adding viewport->Pos to a client position would also be the same.
          io.MousePos = ImVec2((float)mouse_screen_pos.x, (float)mouse_screen_pos.y);
      }
      else
      {
          // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
          // This is the position you can get with ::GetCursorPos() + ::ScreenToClient() or WM_MOUSEMOVE.
          POINT mouse_client_pos = mouse_screen_pos;
          ::ScreenToClient(backendData->hWnd, &mouse_client_pos);
          io.MousePos = ImVec2((float)mouse_client_pos.x, (float)mouse_client_pos.y);
      }

      // (Optional) When using multiple viewports: set io.MouseHoveredViewport to the viewport the OS mouse cursor is hovering.
      // Important: this information is not easy to provide and many high-level windowing library won't be able to provide it correctly, because
      // - This is _ignoring_ viewports with the ImGuiViewportFlags_NoInputs flag (pass-through windows).
      // - This is _regardless_ of whether another viewport is focused or being dragged from.
      // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, imgui will ignore this field and infer the information by relying on the
      // rectangles and last focused time of every viewports it knows about. It will be unaware of foreign windows that may be sitting between or over your windows.
      if (HWND hovered_hwnd = ::WindowFromPoint(mouse_screen_pos))
          if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)hovered_hwnd))
              if ((viewport->Flags & ImGuiViewportFlags_NoInputs) == 0) // FIXME: We still get our NoInputs window with WM_NCHITTEST/HTTRANSPARENT code when decorated?
                  io.MouseHoveredViewport = viewport->ID;
  }
  //}}}
  //{{{
  // Gamepad navigation mapping
  void updateGamepads() {

    #ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
      ImGuiIO& io = ImGui::GetIO();
      sBackendData* backendData = getBackendData();
      memset(io.NavInputs, 0, sizeof(io.NavInputs));
      if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
          return;

      // Calling XInputGetState() every frame on disconnected gamepads is unfortunately too slow.
      // Instead we refresh gamepad availability by calling XInputGetCapabilities() _only_ after receiving WM_DEVICECHANGE.
      if (backendData->WantUpdateHasGamepad) {
        XINPUT_CAPABILITIES caps;
        backendData->HasGamepad = backendData->XInputGetCapabilities ? (backendData->XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &caps) == ERROR_SUCCESS) : false;
        backendData->WantUpdateHasGamepad = false;
        }

      io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
      XINPUT_STATE xinput_state;
      if (backendData->HasGamepad && backendData->XInputGetState && backendData->XInputGetState(0, &xinput_state) == ERROR_SUCCESS) {
        const XINPUT_GAMEPAD& gamepad = xinput_state.Gamepad;
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

        #define MAP_BUTTON(NAV_NO, BUTTON_ENUM)     { io.NavInputs[NAV_NO] = (gamepad.wButtons & BUTTON_ENUM) ? 1.0f : 0.0f; }
        #define MAP_ANALOG(NAV_NO, VALUE, V0, V1)   { float vn = (float)(VALUE - V0) / (float)(V1 - V0); if (vn > 1.0f) vn = 1.0f; if (vn > 0.0f && io.NavInputs[NAV_NO] < vn) io.NavInputs[NAV_NO] = vn; }
        MAP_BUTTON(ImGuiNavInput_Activate,      XINPUT_GAMEPAD_A);              // Cross / A
        MAP_BUTTON(ImGuiNavInput_Cancel,        XINPUT_GAMEPAD_B);              // Circle / B
        MAP_BUTTON(ImGuiNavInput_Menu,          XINPUT_GAMEPAD_X);              // Square / X
        MAP_BUTTON(ImGuiNavInput_Input,         XINPUT_GAMEPAD_Y);              // Triangle / Y
        MAP_BUTTON(ImGuiNavInput_DpadLeft,      XINPUT_GAMEPAD_DPAD_LEFT);      // D-Pad Left
        MAP_BUTTON(ImGuiNavInput_DpadRight,     XINPUT_GAMEPAD_DPAD_RIGHT);     // D-Pad Right
        MAP_BUTTON(ImGuiNavInput_DpadUp,        XINPUT_GAMEPAD_DPAD_UP);        // D-Pad Up
        MAP_BUTTON(ImGuiNavInput_DpadDown,      XINPUT_GAMEPAD_DPAD_DOWN);      // D-Pad Down
        MAP_BUTTON(ImGuiNavInput_FocusPrev,     XINPUT_GAMEPAD_LEFT_SHOULDER);  // L1 / LB
        MAP_BUTTON(ImGuiNavInput_FocusNext,     XINPUT_GAMEPAD_RIGHT_SHOULDER); // R1 / RB
        MAP_BUTTON(ImGuiNavInput_TweakSlow,     XINPUT_GAMEPAD_LEFT_SHOULDER);  // L1 / LB
        MAP_BUTTON(ImGuiNavInput_TweakFast,     XINPUT_GAMEPAD_RIGHT_SHOULDER); // R1 / RB
        MAP_ANALOG(ImGuiNavInput_LStickLeft,    gamepad.sThumbLX,  -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
        MAP_ANALOG(ImGuiNavInput_LStickRight,   gamepad.sThumbLX,  +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
        MAP_ANALOG(ImGuiNavInput_LStickUp,      gamepad.sThumbLY,  +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
        MAP_ANALOG(ImGuiNavInput_LStickDown,    gamepad.sThumbLY,  -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32767);
        #undef MAP_BUTTON
        #undef MAP_ANALOG
        }
    #endif // #ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
    }
  //}}}

  //{{{
  //--------------------------------------------------------------------------------------------------------
  // DPI-related helpers (optional)
  //--------------------------------------------------------------------------------------------------------
  // - Use to enable DPI awareness without having to create an application manifest.
  // - Your own app may already do this via a manifest or explicit calls. This is mostly useful for our examples/ apps.
  // - In theory we could call simple functions from Windows SDK such as SetProcessDPIAware(), SetProcessDpiAwareness(), etc.
  //   but most of the functions provided by Microsoft require Windows 8.1/10+ SDK at compile time and Windows 8/10+ at runtime,
  //   neither we want to require the user to have. So we dynamically select and load those functions to avoid dependencies.
  //---------------------------------------------------------------------------------------------------------
  // This is the scheme successfully used by GLFW (from which we borrowed some of the code) and other apps aiming to be highly portable.
  // EnableDpiAwareness() is just a helper called by main.cpp, we don't call it automatically.
  // If you are trying to implement your own backend for your own engine, you may ignore that noise.
  //---------------------------------------------------------------------------------------------------------

  // Perform our own check with RtlVerifyVersionInfo() instead of using functions from <VersionHelpers.h> as they
  // require a manifest to be functional for checks above 8.1. See https://github.com/ocornut/imgui/issues/4200
  BOOL isWindowsVersionOrGreater (WORD major, WORD minor, WORD) {

    typedef LONG(WINAPI* PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);
    static PFN_RtlVerifyVersionInfo RtlVerifyVersionInfoFn = NULL;

    if (RtlVerifyVersionInfoFn == NULL)
      if (HMODULE ntdllModule = ::GetModuleHandleA("ntdll.dll"))
        RtlVerifyVersionInfoFn = (PFN_RtlVerifyVersionInfo)GetProcAddress(ntdllModule, "RtlVerifyVersionInfo");
      if (RtlVerifyVersionInfoFn == NULL)
          return FALSE;

    RTL_OSVERSIONINFOEXW versionInfo = { };
    ULONGLONG conditionMask = 0;
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    versionInfo.dwMajorVersion = major;
    versionInfo.dwMinorVersion = minor;

    VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

    return (RtlVerifyVersionInfoFn(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION, conditionMask) == 0) ? TRUE : FALSE;
    }
  //}}}
  //{{{
  // Helper function to enable DPI awareness without setting up a manifest
  void enableDpiAwareness() {

    // Make sure monitors will be updated with latest correct scaling
    if (sBackendData* backendData = getBackendData())
      backendData->WantUpdateMonitors = true;

    if (isWindows10OrGreater()) {
      static HINSTANCE user32_dll = ::LoadLibraryA("user32.dll"); // Reference counted per-process
      if (PFN_SetThreadDpiAwarenessContext SetThreadDpiAwarenessContextFn = (PFN_SetThreadDpiAwarenessContext)::GetProcAddress(user32_dll, "SetThreadDpiAwarenessContext")) {
        SetThreadDpiAwarenessContextFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        return;
        }
      }

    if (isWindows8Point1OrGreater()) {
      static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
      if (PFN_SetProcessDpiAwareness SetProcessDpiAwarenessFn = (PFN_SetProcessDpiAwareness)::GetProcAddress(shcore_dll, "SetProcessDpiAwareness")) {
        SetProcessDpiAwarenessFn(PROCESS_PER_MONITOR_DPI_AWARE);
        return;
        }
      }

    #if _WIN32_WINNT >= 0x0600
      ::SetProcessDPIAware();
    #endif
    }
  //}}}
  //{{{
  float getDpiScaleForMonitor (void* monitor) {

    UINT xdpi = 96, ydpi = 96;
    if (isWindows8Point1OrGreater()) {
      static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
      static PFN_GetDpiForMonitor GetDpiForMonitorFn = NULL;
      if (GetDpiForMonitorFn == NULL && shcore_dll != NULL)
        GetDpiForMonitorFn = (PFN_GetDpiForMonitor)::GetProcAddress(shcore_dll, "GetDpiForMonitor");
      if (GetDpiForMonitorFn != NULL) {
        GetDpiForMonitorFn((HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
        IM_ASSERT(xdpi == ydpi); // Please contact me if you hit this assert!
        return xdpi / 96.0f;
        }
      }

    #ifndef NOGDI
      const HDC dc = ::GetDC(NULL);
      xdpi = ::GetDeviceCaps(dc, LOGPIXELSX);
      ydpi = ::GetDeviceCaps(dc, LOGPIXELSY);
      IM_ASSERT(xdpi == ydpi); // Please contact me if you hit this assert!
      ::ReleaseDC(NULL, dc);
    #endif

    return xdpi / 96.0f;
    }
  //}}}
  //{{{
  float getDpiScaleForHwnd (void* hwnd) {

    HMONITOR monitor = ::MonitorFromWindow((HWND)hwnd, MONITOR_DEFAULTTONEAREST);
    return getDpiScaleForMonitor(monitor);
    }
  //}}}
  //{{{
  BOOL CALLBACK updateMonitors_EnumFunc (HMONITOR monitor, HDC, LPRECT, LPARAM) {

    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    if (!::GetMonitorInfo(monitor, &info))
      return TRUE;

    ImGuiPlatformMonitor imgui_monitor;
    imgui_monitor.MainPos = ImVec2((float)info.rcMonitor.left, (float)info.rcMonitor.top);
    imgui_monitor.MainSize = ImVec2((float)(info.rcMonitor.right - info.rcMonitor.left), (float)(info.rcMonitor.bottom - info.rcMonitor.top));
    imgui_monitor.WorkPos = ImVec2((float)info.rcWork.left, (float)info.rcWork.top);
    imgui_monitor.WorkSize = ImVec2((float)(info.rcWork.right - info.rcWork.left), (float)(info.rcWork.bottom - info.rcWork.top));
    imgui_monitor.DpiScale = getDpiScaleForMonitor(monitor);
    ImGuiPlatformIO& io = ImGui::GetPlatformIO();
    if (info.dwFlags & MONITORINFOF_PRIMARY)
      io.Monitors.push_front(imgui_monitor);
    else
      io.Monitors.push_back(imgui_monitor);

    return TRUE;
    }
  //}}}
  //{{{
  void updateMonitors() {

    sBackendData* backendData = getBackendData();
    ImGui::GetPlatformIO().Monitors.resize(0);
    ::EnumDisplayMonitors(NULL, NULL, updateMonitors_EnumFunc, 0);
    backendData->WantUpdateMonitors = false;
    }
  //}}}

  //{{{
  IMGUI_IMPL_API LRESULT WndProcHandler (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    if (ImGui::GetCurrentContext() == NULL)
      return 0;

    ImGuiIO& io = ImGui::GetIO();
    sBackendData* backendData = getBackendData();

    switch (msg) {
      //{{{
      case WM_MOUSEMOVE:
          // We need to call TrackMouseEvent in order to receive WM_MOUSELEAVE events
          backendData->MouseHwnd = hwnd;
          if (!backendData->MouseTracked) {
              TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
              ::TrackMouseEvent(&tme);
              backendData->MouseTracked = true;
          }
          break;
      //}}}
      //{{{
      case WM_MOUSELEAVE:
          if (backendData->MouseHwnd == hwnd)
              backendData->MouseHwnd = NULL;
          backendData->MouseTracked = false;
          break;
      //}}}
      case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
      //{{{
      case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
      {
          int button = 0;
          if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
          if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
          if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
          if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
          if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
              ::SetCapture(hwnd);
          io.MouseDown[button] = true;
          return 0;
      }
      //}}}
      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
      case WM_MBUTTONUP:
      //{{{
      case WM_XBUTTONUP:
      {
          int button = 0;
          if (msg == WM_LBUTTONUP) { button = 0; }
          if (msg == WM_RBUTTONUP) { button = 1; }
          if (msg == WM_MBUTTONUP) { button = 2; }
          if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
          io.MouseDown[button] = false;
          if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
              ::ReleaseCapture();
          return 0;
      }
      //}}}
      //{{{
      case WM_MOUSEWHEEL:
          io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
          return 0;
      //}}}
      //{{{
      case WM_MOUSEHWHEEL:
          io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
          return 0;
      //}}}
      case WM_KEYDOWN:
      //{{{
      case WM_SYSKEYDOWN:
          if (wParam < 256)
              io.KeysDown[wParam] = 1;
          return 0;
      //}}}
      case WM_KEYUP:
      //{{{
      case WM_SYSKEYUP:
          if (wParam < 256)
              io.KeysDown[wParam] = 0;
          return 0;
      //}}}
      //{{{
      case WM_KILLFOCUS:
          memset(io.KeysDown, 0, sizeof(io.KeysDown));
          return 0;
      //}}}
      //{{{
      case WM_CHAR:
          // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
          if (wParam > 0 && wParam < 0x10000)
              io.AddInputCharacterUTF16((unsigned short)wParam);
          return 0;
      //}}}
      //{{{
      case WM_SETCURSOR:
          if (LOWORD(lParam) == HTCLIENT && updateMouseCursor())
              return 1;
          return 0;
      //}}}
      //{{{
      case WM_DEVICECHANGE:
          if ((UINT)wParam == DBT_DEVNODES_CHANGED)
              backendData->WantUpdateHasGamepad = true;
          return 0;
      //}}}
      //{{{
      case WM_DISPLAYCHANGE:
          backendData->WantUpdateMonitors = true;
          return 0;
      //}}}
      }

    return 0;
    }
  //}}}

  //{{{
  void getWin32StyleFromViewportFlags (ImGuiViewportFlags flags, DWORD* out_style, DWORD* out_ex_style) {

    if (flags & ImGuiViewportFlags_NoDecoration)
      *out_style = WS_POPUP;
    else
      *out_style = WS_OVERLAPPEDWINDOW;

    if (flags & ImGuiViewportFlags_NoTaskBarIcon)
      *out_ex_style = WS_EX_TOOLWINDOW;
    else
      *out_ex_style = WS_EX_APPWINDOW;

    if (flags & ImGuiViewportFlags_TopMost)
      *out_ex_style |= WS_EX_TOPMOST;
    }
  //}}}
  //{{{
  void createWindow (ImGuiViewport* viewport) {

    sViewportData* viewportData = IM_NEW(sViewportData)();
    viewport->PlatformUserData = viewportData;

    // Select style and parent window
    getWin32StyleFromViewportFlags (viewport->Flags, &viewportData->DwStyle, &viewportData->DwExStyle);
    HWND parent_window = NULL;
    if (viewport->ParentViewportId != 0)
      if (ImGuiViewport* parent_viewport = ImGui::FindViewportByID(viewport->ParentViewportId))
        parent_window = (HWND)parent_viewport->PlatformHandle;

    // Create window
    RECT rect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)(viewport->Pos.x + viewport->Size.x), (LONG)(viewport->Pos.y + viewport->Size.y) };
    ::AdjustWindowRectEx(&rect, viewportData->DwStyle, FALSE, viewportData->DwExStyle);
    viewportData->Hwnd = ::CreateWindowEx(
        viewportData->DwExStyle, _T("ImGui Platform"), _T("Untitled"), viewportData->DwStyle,   // Style, class name, window name
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,    // Window area
        parent_window, NULL, ::GetModuleHandle(NULL), NULL);                    // Parent window, Menu, Instance, Param
    viewportData->HwndOwned = true;

    viewport->PlatformRequestResize = false;
    viewport->PlatformHandle = viewport->PlatformHandleRaw = viewportData->Hwnd;
    }
  //}}}
  //{{{
  void destroyWindow( ImGuiViewport* viewport) {

    sBackendData* backendData = getBackendData();
    if (sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData) {
      if (::GetCapture() == viewportData->Hwnd) {
        // Transfer capture so if we started dragging from a window that later disappears, we'll still receive the MOUSEUP event.
        ::ReleaseCapture();
        ::SetCapture(backendData->hWnd);
        }
      if (viewportData->Hwnd && viewportData->HwndOwned)
        ::DestroyWindow(viewportData->Hwnd);
      viewportData->Hwnd = NULL;
      IM_DELETE(viewportData);
      }

    viewport->PlatformUserData = viewport->PlatformHandle = NULL;
    }
  //}}}
  //{{{
  void showWindow (ImGuiViewport* viewport) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);
    if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
      ::ShowWindow(viewportData->Hwnd, SW_SHOWNA);
    else
      ::ShowWindow(viewportData->Hwnd, SW_SHOW);
    }
  //}}}
  //{{{
  void updateWindow (ImGuiViewport* viewport) {

    // (Optional) Update Win32 style if it changed _after_ creation.
    // Generally they won't change unless configuration flags are changed, but advanced uses (such as manually rewriting viewport flags) make this useful.
    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);

    DWORD new_style;
    DWORD new_ex_style;
    getWin32StyleFromViewportFlags(viewport->Flags, &new_style, &new_ex_style);

    // Only reapply the flags that have been changed from our point of view (as other flags are being modified by Windows)
    if (viewportData->DwStyle != new_style || viewportData->DwExStyle != new_ex_style) {
      // (Optional) Update TopMost state if it changed _after_ creation
      bool top_most_changed = (viewportData->DwExStyle & WS_EX_TOPMOST) != (new_ex_style & WS_EX_TOPMOST);
      HWND insert_after = top_most_changed ? ((viewport->Flags & ImGuiViewportFlags_TopMost) ? HWND_TOPMOST : HWND_NOTOPMOST) : 0;
      UINT swp_flag = top_most_changed ? 0 : SWP_NOZORDER;

      // Apply flags and position (since it is affected by flags)
      viewportData->DwStyle = new_style;
      viewportData->DwExStyle = new_ex_style;
      ::SetWindowLong(viewportData->Hwnd, GWL_STYLE, viewportData->DwStyle);
      ::SetWindowLong(viewportData->Hwnd, GWL_EXSTYLE, viewportData->DwExStyle);

      RECT rect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)(viewport->Pos.x + viewport->Size.x), (LONG)(viewport->Pos.y + viewport->Size.y) };
      ::AdjustWindowRectEx(&rect, viewportData->DwStyle, FALSE, viewportData->DwExStyle); // Client to Screen
      ::SetWindowPos(viewportData->Hwnd, insert_after, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, swp_flag | SWP_NOACTIVATE | SWP_FRAMECHANGED);
      ::ShowWindow(viewportData->Hwnd, SW_SHOWNA); // This is necessary when we alter the style
      viewport->PlatformRequestMove = viewport->PlatformRequestResize = true;
      }
    }
  //}}}

  //{{{
  ImVec2 getWindowPos (ImGuiViewport* viewport) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);

    POINT pos = { 0, 0 };
    ::ClientToScreen(viewportData->Hwnd, &pos);
    return ImVec2((float)pos.x, (float)pos.y);
    }
  //}}}
  //{{{
  void setWindowPos (ImGuiViewport* viewport, ImVec2 pos) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);

    RECT rect = { (LONG)pos.x, (LONG)pos.y, (LONG)pos.x, (LONG)pos.y };
    ::AdjustWindowRectEx(&rect, viewportData->DwStyle, FALSE, viewportData->DwExStyle);
    ::SetWindowPos(viewportData->Hwnd, NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    }
  //}}}

  //{{{
  ImVec2 getWindowSize (ImGuiViewport* viewport) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);
    RECT rect;
    ::GetClientRect(viewportData->Hwnd, &rect);
    return ImVec2(float(rect.right - rect.left), float(rect.bottom - rect.top));
    }
  //}}}
  //{{{
  void setWindowSize (ImGuiViewport* viewport, ImVec2 size) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);

    RECT rect = { 0, 0, (LONG)size.x, (LONG)size.y };
    ::AdjustWindowRectEx(&rect, viewportData->DwStyle, FALSE, viewportData->DwExStyle); // Client to Screen
    ::SetWindowPos(viewportData->Hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
    }
  //}}}

  //{{{
  void setWindowFocus (ImGuiViewport* viewport) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);

    ::BringWindowToTop(viewportData->Hwnd);
    ::SetForegroundWindow(viewportData->Hwnd);
    ::SetFocus(viewportData->Hwnd);
    }
  //}}}
  //{{{
  bool getWindowFocus (ImGuiViewport* viewport) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);
    return ::GetForegroundWindow() == viewportData->Hwnd;
    }
  //}}}

  //{{{
  bool getWindowMinimized (ImGuiViewport* viewport) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);
    return ::IsIconic(viewportData->Hwnd) != 0;
    }
  //}}}
  //{{{
  void setWindowTitle (ImGuiViewport* viewport, const char* title) {

    // ::SetWindowTextA() doesn't properly handle UTF-8 so we explicitely convert our string.
    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);
    int n = ::MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    #
    ImVector<wchar_t> title_w;
    title_w.resize(n);
    ::MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w.Data, n);
    ::SetWindowTextW(viewportData->Hwnd, title_w.Data);
    }
  //}}}
  //{{{
  void setWindowAlpha (ImGuiViewport* viewport, float alpha) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);
    IM_ASSERT(alpha >= 0.0f && alpha <= 1.0f);

    if (alpha < 1.0f) {
      DWORD style = ::GetWindowLongW(viewportData->Hwnd, GWL_EXSTYLE) | WS_EX_LAYERED;
      ::SetWindowLongW(viewportData->Hwnd, GWL_EXSTYLE, style);
      ::SetLayeredWindowAttributes(viewportData->Hwnd, 0, (BYTE)(255 * alpha), LWA_ALPHA);
      }
    else {
      DWORD style = ::GetWindowLongW(viewportData->Hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED;
      ::SetWindowLongW(viewportData->Hwnd, GWL_EXSTYLE, style);
      }
    }
  //}}}
  //{{{
  float getWindowDpiScale (ImGuiViewport* viewport) {

    sViewportData* viewportData = (sViewportData*)viewport->PlatformUserData;
    IM_ASSERT(viewportData->Hwnd != 0);
    return getDpiScaleForHwnd(viewportData->Hwnd);
    }
  //}}}

  //{{{
  // FIXME-DPI: Testing DPI related ideas
  void onChangedViewport (ImGuiViewport* viewport) {

    (void)viewport;
    #if 0
      ImGuiStyle default_style;
      //default_style.WindowPadding = ImVec2(0, 0);
      //default_style.WindowBorderSize = 0.0f;
      //default_style.ItemSpacing.y = 3.0f;
      //default_style.FramePadding = ImVec2(0, 0);
      default_style.ScaleAllSizes(viewport->DpiScale);
      ImGuiStyle& style = ImGui::GetStyle();
      style = default_style;
    #endif
    }
  //}}}

  //{{{
  LRESULT CALLBACK WndProcHandler_PlatformWindow (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    if (WndProcHandler(hWnd, msg, wParam, lParam))
      return true;

    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)hWnd)) {

      switch (msg) {
        case WM_CLOSE:
          viewport->PlatformRequestClose = true;
          return 0;

        case WM_MOVE:
          viewport->PlatformRequestMove = true;
          break;

        case WM_SIZE:
          viewport->PlatformRequestResize = true;
          break;

        case WM_MOUSEACTIVATE:
          if (viewport->Flags & ImGuiViewportFlags_NoFocusOnClick)
            return MA_NOACTIVATE;
          break;

        case WM_NCHITTEST:
          // Let mouse pass-through the window. This will allow the backend to set io.MouseHoveredViewport properly (which is OPTIONAL).
          // The ImGuiViewportFlags_NoInputs flag is set while dragging a viewport, as want to detect the window behind the one we are dragging.
          // If you cannot easily access those viewport flags from your windowing/event code: you may manually synchronize its state e.g. in
          // your main loop after calling UpdatePlatformWindows(). Iterate all viewports/platform windows and pass the flag to your windowing system.
          if (viewport->Flags & ImGuiViewportFlags_NoInputs)
            return HTTRANSPARENT;
          break;
        }
      }

    return DefWindowProc(hWnd, msg, wParam, lParam);
    }
  //}}}
  //{{{
  void initPlatformInterface() {

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProcHandler_PlatformWindow;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = ::GetModuleHandle(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T("ImGui Platform");
    wcex.hIconSm = NULL;
    ::RegisterClassEx(&wcex);

    updateMonitors();

    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow = createWindow;
    platform_io.Platform_DestroyWindow = destroyWindow;
    platform_io.Platform_ShowWindow = showWindow;
    platform_io.Platform_SetWindowPos = setWindowPos;
    platform_io.Platform_GetWindowPos = getWindowPos;
    platform_io.Platform_SetWindowSize = setWindowSize;
    platform_io.Platform_GetWindowSize = getWindowSize;
    platform_io.Platform_SetWindowFocus = setWindowFocus;
    platform_io.Platform_GetWindowFocus = getWindowFocus;
    platform_io.Platform_GetWindowMinimized = getWindowMinimized;
    platform_io.Platform_SetWindowTitle = setWindowTitle;
    platform_io.Platform_SetWindowAlpha = setWindowAlpha;
    platform_io.Platform_UpdateWindow = updateWindow;
    platform_io.Platform_GetWindowDpiScale = getWindowDpiScale; // FIXME-DPI
    platform_io.Platform_OnChangedViewport = onChangedViewport; // FIXME-DPI

    #if HAS_WIN32_IME
      platform_io.Platform_SetImeInputPos = setImeInputPos;
    #endif

    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    sBackendData* backendData = getBackendData();
    sViewportData* viewportData = IM_NEW(sViewportData)();
    viewportData->Hwnd = backendData->hWnd;
    viewportData->HwndOwned = false;
    main_viewport->PlatformUserData = viewportData;
    main_viewport->PlatformHandle = (void*)backendData->hWnd;
    }
  //}}}
  //{{{
  void shutdownPlatformInterface() {
    ::UnregisterClass (_T("ImGui Platform"), ::GetModuleHandle(NULL));
    }
  //}}}

  //{{{
  // [experimental]
  // Borrowed from GLFW's function updateFramebufferTransparency() in src/win32_window.c
  // (the Dwm* functions are Vista era functions but we are borrowing logic from GLFW)
  void enableAlphaCompositing (void* hwnd)
  {
      if (!isWindowsVistaOrGreater())
          return;

      BOOL composition;
      if (FAILED(::DwmIsCompositionEnabled(&composition)) || !composition)
          return;

      BOOL opaque;
      DWORD color;
      if (isWindows8OrGreater() || (SUCCEEDED(::DwmGetColorizationColor(&color, &opaque)) && !opaque))
      {
          HRGN region = ::CreateRectRgn(0, 0, -1, -1);
          DWM_BLURBEHIND bb = {};
          bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
          bb.hRgnBlur = region;
          bb.fEnable = TRUE;
          ::DwmEnableBlurBehindWindow((HWND)hwnd, &bb);
          ::DeleteObject(region);
      }
      else
      {
          DWM_BLURBEHIND bb = {};
          bb.dwFlags = DWM_BB_ENABLE;
          ::DwmEnableBlurBehindWindow((HWND)hwnd, &bb);
      }
  }
  //}}}
  }

//{{{
bool cPlatform::init (const cPoint& windowSize, bool showViewports, const sizeCallbackFunc sizeCallback) {

  void* hwnd = 0;

  ImGuiIO& io = ImGui::GetIO();
  IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

  INT64 perf_frequency, perf_counter;
  if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&perf_frequency))
    return false;
  if (!::QueryPerformanceCounter((LARGE_INTEGER*)&perf_counter))
    return false;

  // Setup backend capabilities flags
  sBackendData* backendData = IM_NEW(sBackendData)();
  io.BackendPlatformUserData = (void*)backendData;
  io.BackendPlatformName = "imgui_impl_win32";
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
  io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
  io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)

  backendData->hWnd = (HWND)hwnd;
  backendData->WantUpdateHasGamepad = true;
  backendData->WantUpdateMonitors = true;
  backendData->TicksPerSecond = perf_frequency;
  backendData->Time = perf_counter;
  backendData->LastMouseCursor = ImGuiMouseCursor_COUNT;

  // Our mouse update function expect PlatformHandle to be filled for the main viewport
  ImGuiViewport* main_viewport = ImGui::GetMainViewport();
  main_viewport->PlatformHandle = main_viewport->PlatformHandleRaw = (void*)backendData->hWnd;
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    initPlatformInterface();

  // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
  io.KeyMap[ImGuiKey_Tab] = VK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
  io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
  io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
  io.KeyMap[ImGuiKey_Home] = VK_HOME;
  io.KeyMap[ImGuiKey_End] = VK_END;
  io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
  io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
  io.KeyMap[ImGuiKey_Space] = VK_SPACE;
  io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
  io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
  io.KeyMap[ImGuiKey_A] = 'A';
  io.KeyMap[ImGuiKey_C] = 'C';
  io.KeyMap[ImGuiKey_V] = 'V';
  io.KeyMap[ImGuiKey_X] = 'X';
  io.KeyMap[ImGuiKey_Y] = 'Y';
  io.KeyMap[ImGuiKey_Z] = 'Z';

  return true;
  }
//}}}
//{{{
void cPlatform::shutdown() {

  ImGuiIO& io = ImGui::GetIO();
  sBackendData* bd = getBackendData();
  shutdownPlatformInterface();

  io.BackendPlatformName = NULL;
  io.BackendPlatformUserData = NULL;
  IM_DELETE(bd);
  }
//}}}

// gets
//{{{
cPoint cPlatform::getWindowSize() {
  return cPoint();
  }
//}}}

// actions
//{{{
bool cPlatform::pollEvents() {
  return true;
  }
//}}}
//{{{
void cPlatform::newFrame() {

  ImGuiIO& io = ImGui::GetIO();
  sBackendData* backendData = getBackendData();
  IM_ASSERT(bd != NULL && "Did you call Init()?");

  // Setup display size (every frame to accommodate for window resizing)
  RECT rect = { 0, 0, 0, 0 };
  ::GetClientRect(backendData->hWnd, &rect);
  io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
  if (backendData->WantUpdateMonitors)
      updateMonitors();

  // Setup time step
  INT64 current_time = 0;
  ::QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
  io.DeltaTime = (float)(current_time - backendData->Time) / backendData->TicksPerSecond;
  backendData->Time = current_time;

  // Read keyboard modifiers inputs
  io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
  io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
  io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
  io.KeySuper = false;
  // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

  // Update OS mouse position
  updateMousePos();

  // Update OS mouse cursor with the cursor requested by imgui
  ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
  if (backendData->LastMouseCursor != mouse_cursor) {
    backendData->LastMouseCursor = mouse_cursor;
    updateMouseCursor();
    }

  // Update game controllers (if enabled and available)
  updateGamepads();
  }
//}}}
//{{{
void cPlatform::present() {
  }
//}}}
