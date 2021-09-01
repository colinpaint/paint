// main.cpp - paintbox main, uses imGui, stb
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
//#include <format>

// stb - invoke header only library implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// imGui
#include <imgui.h>
#include "../implot/implot.h"
#include "../imgui/TextEditor.h"

// UI font
#include "font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

// self registered classes using static var init idiom
#include "platform/cPlatform.h"
#include "graphics/cGraphics.h"
#include "brush/cBrush.h"
#include "ui/cUI.h"

// canvas
#include "canvas/cLayer.h"
#include "canvas/cCanvas.h"

// log
#include "utils/cLog.h"

using namespace std;
//}}}

#include <fstream>
#include <streambuf>

int main (int numArgs, char* args[]) {

  // default params
  eLogLevel logLevel = LOGINFO;
  string platformName = "glfw";
  string graphicsName = "opengl";
  bool showDemoWindow = false;
  bool vsync = false;
  bool showPlotWindow = false;
  //{{{  parse command line args to params
  // args to params
  vector <string> params;
  for (int i = 1; i < numArgs; i++)
    params.push_back (args[i]);

  // parse and remove recognised params
  for (auto it = params.begin(); it < params.end();) {
    if (*it == "log1") { logLevel = LOGINFO1; params.erase (it); }
    else if (*it == "log2") { logLevel = LOGINFO2; params.erase (it); }
    else if (*it == "log3") { logLevel = LOGINFO3; params.erase (it); }
    else if (*it == "dx11") { platformName = "win32"; graphicsName = "dx11"; params.erase (it); }
    else if (*it == "demo") { showDemoWindow = true; params.erase (it); }
    else if (*it == "vsync") { vsync = true; params.erase (it); }
    else if (*it == "plot") { showPlotWindow = true; params.erase (it); }
    else ++it;
    };
  //}}}

  // start log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("paintbox {} {}", platformName, graphicsName));

  // list static registered classes
  cPlatform::listRegisteredClasses();
  cGraphics::listRegisteredClasses();
  cBrush::listRegisteredClasses();
  cUI::listRegisteredClasses();

  // create platform, graphics, UI font
  cPlatform& platform = cPlatform::createByName (platformName, cPoint(1200, 800), false, vsync);
  cGraphics& graphics = cGraphics::createByName (graphicsName, platform);
  ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f);
  ImFont* monoFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 16.f);

  // should clear main screen here before file loads

  // create canvas
  cCanvas canvas (params.empty() ? "../piccies/tv.jpg" : params[0], graphics);
  if (params.size() > 1)
    canvas.newLayer (params[1]);

  platform.setResizeCallback (
    //{{{  resize lambda
    [&](int width, int height) noexcept {
      platform.newFrame();
      graphics.windowResize (width, height);
      graphics.newFrame();
      if (showDemoWindow)
        cUI::draw (canvas, graphics, platform, monoFont);
      ImGui::ShowDemoWindow (&showDemoWindow);
      graphics.drawUI (platform.getWindowSize());
      platform.present();
      }
    );
    //}}}
  platform.setDropCallback (
    //{{{  drop lambda
    [&](vector<string> dropItems) noexcept {

      for (auto& item : dropItems) {
        cLog::log (LOGINFO, item);
        canvas.newLayer (item);
        }
      }
    );
    //}}}

  //{{{  textEditor
  TextEditor editor;
  auto lang = TextEditor::LanguageDefinition::CPlusPlus();

  // set your own known preprocessor symbols...
  //{{{
  static const char* ppnames[] = {
    "NULL", "PM_REMOVE",
    "ZeroMemory", "DXGI_SWAP_EFFECT_DISCARD", "D3D_FEATURE_LEVEL", "D3D_DRIVER_TYPE_HARDWARE",
    "WINAPI","D3D11_SDK_VERSION", "assert" };
  //}}}
  //{{{
  // ... and their corresponding values
  static const char* ppvalues[] = {
    "#define NULL ((void*)0)",
    "#define PM_REMOVE (0x0001)",
    "Microsoft's own memory zapper function\n(which is a macro actually)\nvoid ZeroMemory(\n\t[in] PVOID  Destination,\n\t[in] SIZE_T Length\n); ",
    "enum DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD = 0",
    "enum D3D_FEATURE_LEVEL",
    "enum D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE  = ( D3D_DRIVER_TYPE_UNKNOWN + 1 )",
    "#define WINAPI __stdcall",
    "#define D3D11_SDK_VERSION (7)",
    " #define assert(expression) (void)(                                                  \n"
        "    (!!(expression)) ||                                                              \n"
        "    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
        " )"
    };
  //}}}
  for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i) {
    TextEditor::Identifier id;
    id.mDeclaration = ppvalues[i];
    lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
    }

  // set your own identifiers
  //{{{
  static const char* identifiers[] = {
    "HWND", "HRESULT", "LPRESULT","D3D11_RENDER_TARGET_VIEW_DESC", "DXGI_SWAP_CHAIN_DESC","MSG",
    "LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
    "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob",
    "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
    "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView",
    "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
    "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "TextEditor" };
  //}}}
  //{{{
  static const char* idecls[] =
  {
    "typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT",
    "struct D3D11_RENDER_TARGET_VIEW_DESC", "struct DXGI_SWAP_CHAIN_DESC",
    "typedef tagMSG MSG\n * Message structure","typedef LONG_PTR LRESULT",
    "WPARAM", "LPARAM","UINT","LPVOID",
    "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer",
    "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
    "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView",
    "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
    "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "class TextEditor" };
  //}}}
  for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i) {
    TextEditor::Identifier id;
    id.mDeclaration = std::string(idecls[i]);
    lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
    }

  editor.SetLanguageDefinition (lang);
  //editor.SetPalette(TextEditor::GetLightPalette());

  // error markers
  TextEditor::ErrorMarkers markers;
  markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
  markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
  editor.SetErrorMarkers(markers);

  // "breakpoint" markers
  //TextEditor::Breakpoints bpts;
  //bpts.insert(24);
  //bpts.insert(47);
  //editor.SetBreakpoints(bpts);

  static const char* fileToEdit = "C:/Users/colin/Desktop/cPaintBrush.cpp";
  //std::ifstream t(fileToEdit);
  //if (t.good()) {
    //std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  editor.SetText("int main (int numArgs, char* args[] {\n}\n");

  //}}}

  // main UI loop
  while (platform.pollEvents()) {
    platform.newFrame();
    graphics.newFrame();
    cUI::draw (canvas, graphics, platform, monoFont);
    if (showDemoWindow)
      ImGui::ShowDemoWindow (&showDemoWindow);
    if (showPlotWindow) {
      //{{{  optional implot
      #ifdef USE_IMPLOT
        ImPlot::ShowDemoWindow();
      #endif
      }
      //}}}
    //{{{  texteditor
    auto cpos = editor.GetCursorPosition();

    ImGui::Begin("Text Editor Demo", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
    ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::BeginMenuBar()) {
      //{{{  menu bar
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save")) {
          auto textToSave = editor.GetText();
          /// save text....
        }
        if (ImGui::MenuItem("Quit", "Alt-F4"))
          break;
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        bool ro = editor.IsReadOnly();
        if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
          editor.SetReadOnly(ro);
        ImGui::Separator();

        if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
          editor.Undo();
        if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
          editor.Redo();

        ImGui::Separator();

        if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
          editor.Copy();
        if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
          editor.Cut();
        if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
          editor.Delete();
        if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
          editor.Paste();

        ImGui::Separator();

        if (ImGui::MenuItem("Select all", nullptr, nullptr))
          editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Dark palette"))
          editor.SetPalette(TextEditor::GetDarkPalette());
        if (ImGui::MenuItem("Light palette"))
          editor.SetPalette(TextEditor::GetLightPalette());
        if (ImGui::MenuItem("Retro blue palette"))
          editor.SetPalette(TextEditor::GetRetroBluePalette());
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
      }
      //}}}

    ImGui::Text ("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
                 editor.IsOverwrite() ? "Ovr" : "Ins",
                 editor.CanUndo() ? "*" : " ",
                 editor.GetLanguageDefinition().mName.c_str(), fileToEdit);

    editor.Render("TextEditor");

    ImGui::End();
    //}}}

    graphics.drawUI (platform.getWindowSize());
    platform.present();
    }

  // cleanup
  graphics.shutdown();
  platform.shutdown();

  return EXIT_SUCCESS;
  }
