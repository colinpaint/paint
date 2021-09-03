// cTextEditorUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

#include <fstream>
#include <streambuf>

// imgui
#include <imgui.h>
#include <cTextEditor.h>

// ui
#include "cUI.h"

// canvas
#include "../canvas/cCanvas.h"

// utils
#include "../utils/cLog.h"

using namespace std;
//}}}

namespace {
  //{{{
  const vector<string> kPreProcessorNames = {
    "NULL",
    "PM_REMOVE",
    "ZeroMemory",
    "DXGI_SWAP_EFFECT_DISCARD",
    "D3D_FEATURE_LEVEL",
    "D3D_DRIVER_TYPE_HARDWARE",
    "WINAPI",
    "D3D11_SDK_VERSION",
    "assert"
    };
  //}}}
  //{{{
  const vector<string> kPreProcessorValues = {
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
  //{{{
  const vector<string> kIdents = {
    "HWND",
    "HRESULT",
    "LPRESULT",
    "D3D11_RENDER_TARGET_VIEW_DESC",
    "DXGI_SWAP_CHAIN_DESC","MSG",
    "LRESULT",
    "WPARAM",
    "LPARAM",
    "UINT",
    "LPVOID",
    "ID3D11Device",
    "ID3D11DeviceContext",
    "ID3D11Buffer", "ID3D11Buffer",
    "ID3D10Blob",
    "ID3D11VertexShader",
    "ID3D11InputLayout",
    "ID3D11Buffer",
    "ID3D10Blob",
    "ID3D11PixelShader",
    "ID3D11SamplerState",
    "ID3D11ShaderResourceView",
    "ID3D11RasterizerState",
    "ID3D11BlendState",
    "ID3D11DepthStencilState",
    "IDXGISwapChain",
    "ID3D11RenderTargetView",
    "ID3D11Texture2D",
    "cTextEditor" };
  //}}}
  //{{{
  const vector<string> kIdecls = {
    "typedef HWND_* HWND",
    "typedef long HRESULT",
    "typedef long* LPRESULT",
    "struct D3D11_RENDER_TARGET_VIEW_DESC",
    "struct DXGI_SWAP_CHAIN_DESC",
    "typedef tagMSG MSG\n * Message structure",
    "typedef LONG_PTR LRESULT",
    "WPARAM",
    "LPARAM",
    "UINT",
    "LPVOID",
    "ID3D11Device",
    "ID3D11DeviceContext",
    "ID3D11Buffer",
    "ID3D11Buffer",
    "ID3D10Blob",
    "ID3D11VertexShader",
    "ID3D11InputLayout",
    "ID3D11Buffer",
    "ID3D10Blob",
    "ID3D11PixelShader",
    "ID3D11SamplerState",
    "ID3D11ShaderResourceView",
    "ID3D11RasterizerState",
    "ID3D11BlendState",
    "ID3D11DepthStencilState",
    "IDXGISwapChain",
    "ID3D11RenderTargetView",
    "ID3D11Texture2D",
    "class cTextEditor" };
  //}}}
  }

class cTextEditorUI : public cUI {
public:
  //{{{
  cTextEditorUI (const string& name) : cUI(name) {
    }
  //}}}
  //{{{
  virtual ~cTextEditorUI() {
    // close the file mapping object
    }
  //}}}

  void addToDrawList (cCanvas& canvas, cGraphics& graphics, cPlatform& platform, ImFont* monoFont) final {
    //{{{  unused params
    (void)canvas;
    (void)graphics;
    (void)platform;
    //}}}

    if (!mTextLoaded) {
      //{{{  load text
      mTextLoaded = true;

      // set file
      #ifdef _WIN32
        ifstream fileStream ("C:/projects/paint/imgui/cTextEditor.cpp");
      #else
        ifstream fileStream ("/home/pi/paint/imgui/cTextEditor.cpp");
      #endif
      string str ((istreambuf_iterator<char>(fileStream)), istreambuf_iterator<char>());
      mTextEditor.setText (str);

      // set language
      cTextEditor::sLanguage language = cTextEditor::sLanguage::CPlusPlus();
      for (size_t i = 0; i < kPreProcessorNames.size(); i++) {
        cTextEditor::sIdent id;
        id.mDeclaration = kPreProcessorValues[i];
        language.mPreprocIdents.insert (make_pair (kPreProcessorNames[i], id));
        }
      for (size_t i = 0; i < kIdents.size(); i++) {
        cTextEditor::sIdent id;
        id.mDeclaration = kIdecls[i];
        language.mIdents.insert (make_pair (kIdents[i], id));
        }
      mTextEditor.setLanguage (language);

      //{{{  markers
      //cTextEditor::Markers markers;
      //markers.insert (make_pair<int, string>(6, "Example error here:\nInclude file not found: \"cTextEditor.h\""));
      //markers.insert (make_pair<int, string>(41, "Another example error"));
      //editor.SetMarkers (markers);
      //}}}
      }
      //}}}

    ImGui::Begin ("Text Editor Demo", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
    ImGui::SetWindowSize (ImVec2(800, 600), ImGuiCond_FirstUseEver);

    if (ImGui::BeginMenuBar()) {
      //{{{  menu bar
      if (ImGui::BeginMenu ("File")) {
        if (ImGui::MenuItem ("Save")) {
          auto textToSave = mTextEditor.getText();
          /// save text....
          }
        ImGui::EndMenu();
        }

      if (ImGui::BeginMenu ("Edit")) {
        //{{{  readOnly
        bool readOnly = mTextEditor.isReadOnly();
        if (ImGui::MenuItem ("ReadOnly", nullptr, &readOnly))
          mTextEditor.setReadOnly (readOnly);
        //}}}
        //{{{  folded
        ImGui::Separator();

        bool folded = mTextEditor.isFolded();
        if (ImGui::MenuItem ("Folded", nullptr, &folded))
          mTextEditor.setFolded (folded);
        //}}}
        //{{{  showLineNumbers
        ImGui::Separator();

        bool showLineNumbers = mTextEditor.isShowLineNumbers();
        if (ImGui::MenuItem ("Line Numbers", nullptr, &showLineNumbers))
          mTextEditor.setShowLineNumbers (showLineNumbers);
        //}}}
        //{{{  showLineDebug
        ImGui::Separator();

        bool showLineDebug = mTextEditor.isShowLineDebug();
        if (ImGui::MenuItem ("Line debug", nullptr, &showLineDebug))
          mTextEditor.setShowLineDebug (showLineDebug);
        //}}}

        ImGui::Separator();
        if (ImGui::MenuItem ("Undo", "ALT-Backspace", nullptr, !readOnly && mTextEditor.canUndo()))
          mTextEditor.undo();
        if (ImGui::MenuItem ("Redo", "Ctrl-Y", nullptr, !readOnly && mTextEditor.canRedo()))
          mTextEditor.redo();

        ImGui::Separator();

        if (ImGui::MenuItem ("Copy", "Ctrl-C", nullptr, mTextEditor.hasSelection()))
          mTextEditor.copy();
        if (ImGui::MenuItem ("Cut", "Ctrl-X", nullptr, !readOnly && mTextEditor.hasSelection()))
          mTextEditor.cut();
        if (ImGui::MenuItem ("Delete", "Del", nullptr, !readOnly && mTextEditor.hasSelection()))
          mTextEditor.deleteIt();
        if (ImGui::MenuItem ("Paste", "Ctrl-V", nullptr, !readOnly && ImGui::GetClipboardText() != nullptr))
          mTextEditor.paste();
        ImGui::Separator();

        if (ImGui::MenuItem ("Select all", nullptr, nullptr))
          mTextEditor.setSelection (cTextEditor::sPosition(),
                                    cTextEditor::sPosition(mTextEditor.getTotalLines(), 0));

        ImGui::EndMenu();
        }

      ImGui::EndMenuBar();
      }
      //}}}

    ImGui::Text ("%d:%d:%d %s %s %s",
                 mTextEditor.getCursorPosition().mColumn + 1,
                 mTextEditor.getCursorPosition().mLineNumber + 1,
                 mTextEditor.getTotalLines(),
                 mTextEditor.getLanguage().mName.c_str(),
                 mTextEditor.isOverwrite() ? "overwrite" : "insert",
                 mTextEditor.canUndo() ? "undo" : " ",
                 mTextEditor.isFolded() ? "folded" : " "
                 );
    ImGui::PushFont (monoFont);

    mTextEditor.render ("cTextEditor");
    ImGui::PopFont();

    ImGui::End();
    }

private:
  bool mTextLoaded = false;
  ImFont* mMonoFont = nullptr;

  cTextEditor mTextEditor;

  //{{{
  static cUI* create (const string& className) {
    return new cTextEditorUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("textEditor", &create);
  };
