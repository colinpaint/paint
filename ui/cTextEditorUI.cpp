// cTextEditorUI.cpp
#ifdef _WIN32
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

      ifstream fileStream ("C:\\projects\\paint\\imgui\\cTextEditor.cpp");
      string str ((istreambuf_iterator<char>(fileStream)), istreambuf_iterator<char>());

      auto language = cTextEditor::sLanguage::CPlusPlus();
      //{{{  set your own preprocessor
      static const char* ppnames[] = {
        "NULL",
        "PM_REMOVE",
        "ZeroMemory",
        "DXGI_SWAP_EFFECT_DISCARD",
        "D3D_FEATURE_LEVEL",
        "D3D_DRIVER_TYPE_HARDWARE",
        "WINAPI",
        "D3D11_SDK_VERSION",
        "assert" };

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

      for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i) {
        cTextEditor::sIdent id;
        id.mDeclaration = ppvalues[i];
        language.mPreprocIdents.insert (make_pair (string (ppnames[i]), id));
        }
      //}}}
      //{{{  set your own idents
      static const char* idents[] = {
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

      static const char* idecls[] = {
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

      for (int i = 0; i < sizeof(idents) / sizeof(idents[0]); ++i) {
        cTextEditor::sIdent id;
        id.mDeclaration = string (idecls[i]);
        language.mIdents.insert (make_pair (string (idents[i]), id));
        }
      //}}}

      mTextEditor.setLanguage (language);
      mTextEditor.setText (str);
      //{{{  error markers
      //cTextEditor::ErrorMarkers markers;
      //markers.insert (make_pair<int, string>(6, "Example error here:\nInclude file not found: \"cTextEditor.h\""));
      //markers.insert (make_pair<int, string>(41, "Another example error"));
      //editor.SetErrorMarkers (markers);
      //}}}
      //{{{  "break" markers
      //cTextEditor::Breaks bpts;
      //bpts.insert(24);
      //bpts.insert(47);
      //editor.SetBreaks (bpts);
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
        bool ro = mTextEditor.isReadOnly();
        if (ImGui::MenuItem ("Read-only mode", nullptr, &ro))
          mTextEditor.setReadOnly (ro);
        ImGui::Separator();

        if (ImGui::MenuItem ("Undo", "ALT-Backspace", nullptr, !ro && mTextEditor.canUndo()))
          mTextEditor.undo();
        if (ImGui::MenuItem ("Redo", "Ctrl-Y", nullptr, !ro && mTextEditor.canRedo()))
          mTextEditor.redo();

        ImGui::Separator();

        if (ImGui::MenuItem ("Copy", "Ctrl-C", nullptr, mTextEditor.hasSelection()))
          mTextEditor.copy();
        if (ImGui::MenuItem ("Cut", "Ctrl-X", nullptr, !ro && mTextEditor.hasSelection()))
          mTextEditor.cut();
        if (ImGui::MenuItem ("Delete", "Del", nullptr, !ro && mTextEditor.hasSelection()))
          mTextEditor.deleteIt();
        if (ImGui::MenuItem ("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
          mTextEditor.paste();
        ImGui::Separator();

        if (ImGui::MenuItem ("Select all", nullptr, nullptr))
          mTextEditor.setSelection (cTextEditor::sRowColumn(), 
                                    cTextEditor::sRowColumn (mTextEditor.getTotalLines(), 0));

        ImGui::EndMenu();
        }

      ImGui::EndMenuBar();
      }
      //}}}

    ImGui::Text ("%6d/%-6d %6d lines  | %s | %s | %s",
                 mTextEditor.getCursorPosition().mLine + 1, mTextEditor.getCursorPosition().mColumn + 1,
                 mTextEditor.getTotalLines(),
                 mTextEditor.isOverwrite() ? "overwrite" : "insert",
                 mTextEditor.canUndo() ? "undo" : " ",
                 mTextEditor.getLanguage().mName.c_str());
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
#endif
