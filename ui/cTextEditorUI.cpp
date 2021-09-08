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

    //{{{  line button
    if (toggleButton ("line", mTextEditor.isShowLineNumbers()))
      mTextEditor.toggleShowLineNumbers();
    //}}}
    if (mTextEditor.hasFolds()) {
      //{{{  folded button
      ImGui::SameLine();
      if (toggleButton ("folded", mTextEditor.isShowFolds()))
        mTextEditor.toggleShowFolds();
      }
      //}}}
    //{{{  space button
    ImGui::SameLine();
    if (toggleButton ("space", mTextEditor.isShowWhiteSpace()))
      mTextEditor.toggleShowWhiteSpace();
    //}}}
    if (mTextEditor.hasClipboardText() && !mTextEditor.isReadOnly()) {
      //{{{  paste button
      ImGui::SameLine();
      if (ImGui::Button ("paste"))
        mTextEditor.paste();
      }
      //}}}
    if (mTextEditor.hasSelect()) {
      //{{{  copy, cut, delete buttons
      ImGui::SameLine();
      if (ImGui::Button ("copy"))
        mTextEditor.copy();
       if (!mTextEditor.isReadOnly()) {
         ImGui::SameLine();
        if (ImGui::Button ("cut"))
          mTextEditor.cut();
        ImGui::SameLine();
        if (ImGui::Button ("delete"))
          mTextEditor.deleteIt();
        }
      }
      //}}}
    if (!mTextEditor.isReadOnly() && mTextEditor.hasUndo()) {
      //{{{  undo button
      ImGui::SameLine();
      if (ImGui::Button ("undo"))
        mTextEditor.undo();
      }
      //}}}
    if (!mTextEditor.isReadOnly() && mTextEditor.hasRedo()) {
      //{{{  redo button
      ImGui::SameLine();
      if (ImGui::Button ("redo"))
        mTextEditor.redo();
      }
      //}}}
    //{{{  readOnly button
    ImGui::SameLine();
    if (ImGui::Button (mTextEditor.isReadOnly() ? "readOnly" : "writable"))
      mTextEditor.toggleReadOnly();
    //}}}
    //{{{  overwrite button
    ImGui::SameLine();
    if (ImGui::Button (mTextEditor.isOverwrite() ? "overwrite" : "insert"))
      mTextEditor.toggleOverwrite();
    //}}}
    //{{{  debug button
    ImGui::SameLine();
    if (toggleButton ("debug", mTextEditor.isShowLineDebug()))
      mTextEditor.toggleShowLineDebug();
    //}}}
    //{{{  info text
    ImGui::SameLine();
    ImGui::Text ("%d:%d:%d %s%s%s", mTextEditor.getCursorPosition().mColumn+1,
                                     mTextEditor.getCursorPosition().mLineNumber+1,
                                     mTextEditor.getTextNumLines(),
                                     mTextEditor.getLanguage().mName.c_str(),
                                     mTextEditor.hasTabs() ? " tabs":"",
                                     mTextEditor.hasCR() ? " CR":"");
    //}}}

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
