// cTextEditorUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

#include <fstream>
#include <streambuf>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/cTextEditor.h"

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

  void addToDrawList (cApp& app) final {

    if (!mTextLoaded) {
      //{{{  init mTextEditor
      mTextLoaded = true;

      // set file
      ifstream fileStream (app.getName());

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

    // full screen window
    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

    ImGui::Begin ("Text Editor Demo", nullptr, 
                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);

    if (toggleButton ("folded", mTextEditor.isFolded()))
      mTextEditor.toggleShowFolded();
    ImGui::SameLine();
    if (toggleButton ("lineNumbers", mTextEditor.isShowLineNumbers()))
      mTextEditor.toggleShowLineNumbers();
    ImGui::SameLine();
    if (toggleButton ("lineDebug", mTextEditor.isShowLineDebug()))
      mTextEditor.toggleShowLineDebug();
    ImGui::SameLine();
    if (toggleButton ("whiteSpace", mTextEditor.isShowWhiteSpace()))
      mTextEditor.toggleShowWhiteSpace();
    ImGui::SameLine();
    if (ImGui::Button (mTextEditor.isReadOnly() ? "readOnly" : "writable"))
      mTextEditor.toggleReadOnly();
    ImGui::SameLine();
    if (ImGui::Button (mTextEditor.isOverwrite() ? "overwrite" : "insert"))
      mTextEditor.toggleOverwrite();
    if (mTextEditor.hasUndo()) {
      ImGui::SameLine();
      if (ImGui::Button ("undo"))
        mTextEditor.undo();
      }
    if (mTextEditor.hasRedo()) {
      ImGui::SameLine();
      if (ImGui::Button ("redo"))
        mTextEditor.redo();
      }
    if (mTextEditor.hasSelect()) {
      ImGui::SameLine();
      if (ImGui::Button ("paste"))
        mTextEditor.paste();
      ImGui::SameLine();
      if (ImGui::Button ("copy"))
        mTextEditor.copy();
      ImGui::SameLine();
      if (ImGui::Button ("cut"))
        mTextEditor.cut();
      ImGui::SameLine();
      if (ImGui::Button ("delete"))
        mTextEditor.deleteIt();
      }

    ImGui::SameLine();
    ImGui::Text ("%d:%d:%d %s",
                 mTextEditor.getCursorPosition().mColumn+1,
                 mTextEditor.getCursorPosition().mLineNumber+1,
                 mTextEditor.getTextNumLines(),
                 mTextEditor.getLanguage().mName.c_str()
                 );
    ImGui::PushFont (app.getMonoFont());

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
