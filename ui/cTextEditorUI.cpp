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
    "preProc",
    };
  //}}}
  //{{{
  const vector<string> kPreProcessorValues = {
    "preProcValue",
    };
  //}}}
  //{{{
  const vector<string> kIdents = {
    "ident",
    };
  //}}}
  //{{{
  const vector<string> kIdecls = {
    "ident decl",
    };
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

  void addToDrawList (cCanvas* canvas, cGraphics& graphics, cPlatform& platform, ImFont* monoFont) final {
    //{{{  unused params
    (void)canvas;
    (void)graphics;
    (void)platform;
    //}}}

    if (!mTextLoaded) {
      //{{{  init mTextEditor
      mTextLoaded = true;

      // set file
      #ifdef _WIN32
        ifstream fileStream ("C:/projects/paint/imgui/cTextEditor.cpp");
      #else
        ifstream fileStream ("/home/pi/paint/imgui/cTextEditor.cpp");
      #endif

      string str ((istreambuf_iterator<char>(fileStream)), istreambuf_iterator<char>());
      mTextEditor.setTextString (str);

      // set language
      cTextEditor::sLanguage language = cTextEditor::sLanguage::cPlus();
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

      // markers
      map <int,string> markers;
      markers.insert (make_pair<int,string>(41, "marker here"));
      mTextEditor.setMarkers (markers);
      }
      //}}}

    ImGui::Begin ("Text Editor Demo", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
    ImGui::SetWindowSize (ImVec2(800, 600), ImGuiCond_FirstUseEver);

    if (ImGui::BeginMenuBar()) {
      //{{{  menu bar
      if (ImGui::BeginMenu ("File")) {
        if (ImGui::MenuItem ("Save")) {
          auto textToSave = mTextEditor.getTextString();
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
        //{{{  showWhiteSpace
        bool showWhiteSpace = mTextEditor.isShowWhiteSpace();
        if (ImGui::MenuItem ("White space", nullptr, &showWhiteSpace))
          mTextEditor.setShowWhiteSpace (showWhiteSpace);
        //}}}
        //{{{  folded
        bool folded = mTextEditor.isFolded();
        if (ImGui::MenuItem ("Folded", nullptr, &folded))
          mTextEditor.setFolded (folded);
        //}}}
        //{{{  showLineNumbers
        bool showLineNumbers = mTextEditor.isShowLineNumbers();
        if (ImGui::MenuItem ("Line Numbers", nullptr, &showLineNumbers))
          mTextEditor.setShowLineNumbers (showLineNumbers);
        //}}}
        //{{{  showLineDebug
        bool showLineDebug = mTextEditor.isShowLineDebug();
        if (ImGui::MenuItem ("Line debug", nullptr, &showLineDebug))
          mTextEditor.setShowLineDebug (showLineDebug);
        //}}}

        ImGui::Separator();
        if (ImGui::MenuItem ("Undo", "ALT-Backspace", nullptr, !readOnly && mTextEditor.hasUndo()))
          mTextEditor.undo();
        if (ImGui::MenuItem ("Redo", "Ctrl-Y", nullptr, !readOnly && mTextEditor.hasRedo()))
          mTextEditor.redo();

        ImGui::Separator();
        if (ImGui::MenuItem ("Copy", "Ctrl-C", nullptr, mTextEditor.hasSelect()))
          mTextEditor.copy();
        if (ImGui::MenuItem ("Cut", "Ctrl-X", nullptr, !readOnly && mTextEditor.hasSelect()))
          mTextEditor.cut();
        if (ImGui::MenuItem ("Delete", "Del", nullptr, !readOnly && mTextEditor.hasSelect()))
          mTextEditor.deleteIt();
        if (ImGui::MenuItem ("Paste", "Ctrl-V", nullptr, !readOnly && ImGui::GetClipboardText() != nullptr))
          mTextEditor.paste();

        ImGui::Separator();
        if (ImGui::MenuItem ("Select all", "Ctrl-A"))
          mTextEditor.selectAll();

        ImGui::EndMenu();
        }

      ImGui::EndMenuBar();
      }
      //}}}

    ImGui::Text ("%d:%d:%d %s%s%s%s%s%s%s%s",
                 mTextEditor.getCursorPosition().mColumn + 1,
                 mTextEditor.getCursorPosition().mLineNumber + 1,
                 mTextEditor.getTextNumLines(),
                 mTextEditor.getLanguage().mName.c_str(),
                 mTextEditor.isOverwrite() ? " overwrite" : " insert",
                 mTextEditor.isReadOnly() ? " readOnly" : "",
                 mTextEditor.hasUndo() ? " undo" : "",
                 mTextEditor.hasRedo() ? " redo" : "",
                 mTextEditor.isFolded() ? " folded" : "",
                 mTextEditor.isShowWhiteSpace() ? " whiteSpace" : "",
                 mTextEditor.isShowLineNumbers() ? " lineNumbers" : ""
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
