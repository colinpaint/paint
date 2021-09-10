// cTextEditUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

#include <fstream>
#include <streambuf>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/cTextEdit.h"
#include "../imgui/cMemoryEdit.h"
#include "../imgui/myImguiWidgets.h"

// ui
#include "cUI.h"

// decoder
#include "../utils/cFileView.h"

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
  cMemoryEdit memoryEdit;
  }

class cTextEditUI : public cUI {
public:
  //{{{
  cTextEditUI (const string& name) : cUI(name) {
    }
  //}}}
  //{{{
  virtual ~cTextEditUI() {
    // close the file mapping object
    delete mFileView;
    }
  //}}}

  void addToDrawList (cApp& app) final {

    if (!mLoaded) {
      mLoaded = true;
      //mFileView = new cFileView ("C:/projects/paint/build/Release/fed.exe");
      }

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
    ImGui::PushFont (app.getMonoFont());
    //memoryEdit.drawWindow ("Memory Editor", mFileView->getReadPtr(), mFileView->getReadBytesLeft(), 0);
    memoryEdit.drawWindow ("Memory Editor", (uint8_t*)this, 0x10000, 0);
    ImGui::PopFont();

    return;

    if (!mLoaded) {
      //{{{  init mTextEditor
      mLoaded = true;

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

      ImGui::SetWindowFocus();
      }
      //}}}

    // full screen window
    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
    ImGui::PushStyleColor (ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4 (0xffefefef));
    ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
    ImGui::Begin ("fed", nullptr,
                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);

    // line button
    if (toggleButton ("line", mTextEditor.isShowLineNumbers()))
      mTextEditor.toggleShowLineNumbers();
    if (mTextEditor.isShowLineNumbers())
      //{{{  debug button
      if (mTextEditor.isShowLineNumbers()) {
        ImGui::SameLine();
        if (toggleButton ("debug", mTextEditor.isShowDebug()))
          mTextEditor.toggleShowDebug();
        }
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
    if (toggleButton ("readOnly", mTextEditor.isReadOnly()))
      mTextEditor.toggleReadOnly();
    //}}}
    //{{{  overwrite button
    ImGui::SameLine();
    if (toggleButton ("insert", !mTextEditor.isOverwrite()))
      mTextEditor.toggleOverwrite();
    //}}}
    //{{{  info text
    ImGui::SameLine();
    ImGui::Text ("%d:%d:%d %s%s%s%s%s", mTextEditor.getCursorPosition().mColumn+1,
                                        mTextEditor.getCursorPosition().mLineNumber+1,
                                        mTextEditor.getTextNumLines(),
                                        mTextEditor.getLanguage().mName.c_str(),
                                        mTextEditor.isTextEdited() ? " edited":"",
                                        mTextEditor.hasTabs() ? " tabs":"",
                                        mTextEditor.hasCR() ? " CR":"",
                                        mTextEditor.getDebugString().c_str()
                                        );
    //}}}

    ImGui::PushFont (app.getMonoFont());
    mTextEditor.drawContents();
    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    }

private:
  bool mLoaded = false;
  cFileView* mFileView = nullptr;
  cTextEditor mTextEditor;

  //{{{
  static cUI* create (const string& className) {
    return new cTextEditUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("textEdit", &create);
  };
