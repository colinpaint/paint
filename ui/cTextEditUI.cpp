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

    //if (!mFileView)
      //mFileView = new cFileView ("C:/projects/paint/build/Release/fed.exe");

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
    ImGui::PushFont (app.getMonoFont());
    //memoryEdit.drawWindow ("Memory Editor", mFileView->getReadPtr(), mFileView->getReadBytesLeft(), 0);
    //memoryEdit.drawWindow ("Memory Editor", (uint8_t*)this, sizeof(*this), 0);
    memoryEdit.drawWindow ("Memory Editor", (uint8_t*)(this), 0x10000, 0);
    ImGui::PopFont();

    return;

    if (!mTextEdit) {
      mTextEdit = new cTextEdit();

      // set file
      ifstream fileStream (app.getName());

      string str ((istreambuf_iterator<char>(fileStream)), istreambuf_iterator<char>());
      mTextEdit->setTextString (str);

      // set language
      cTextEdit::sLanguage language = cTextEdit::sLanguage::cPlus();
      for (size_t i = 0; i < kPreProcessorNames.size(); i++) {
        cTextEdit::sIdent id;
        id.mDeclaration = kPreProcessorValues[i];
        language.mPreprocIdents.insert (make_pair (kPreProcessorNames[i], id));
        }

      for (size_t i = 0; i < kIdents.size(); i++) {
        cTextEdit::sIdent id;
        id.mDeclaration = kIdecls[i];
        language.mIdents.insert (make_pair (kIdents[i], id));
        }
      mTextEdit->setLanguage (language);

      // markers
      map <int,string> markers;
      markers.insert (make_pair<int,string>(41, "marker here"));
      mTextEdit->setMarkers (markers);

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
    if (toggleButton ("line", mTextEdit->isShowLineNumbers()))
      mTextEdit->toggleShowLineNumbers();
    if (mTextEdit->isShowLineNumbers())
      //{{{  debug button
      if (mTextEdit->isShowLineNumbers()) {
        ImGui::SameLine();
        if (toggleButton ("debug", mTextEdit->isShowDebug()))
          mTextEdit->toggleShowDebug();
        }
      //}}}
    if (mTextEdit->hasFolds()) {
      //{{{  folded button
      ImGui::SameLine();
      if (toggleButton ("folded", mTextEdit->isShowFolds()))
        mTextEdit->toggleShowFolds();
      }
      //}}}
    //{{{  space button
    ImGui::SameLine();
    if (toggleButton ("space", mTextEdit->isShowWhiteSpace()))
      mTextEdit->toggleShowWhiteSpace();
    //}}}
    if (mTextEdit->hasClipboardText() && !mTextEdit->isReadOnly()) {
      //{{{  paste button
      ImGui::SameLine();
      if (ImGui::Button ("paste"))
        mTextEdit->paste();
      }
      //}}}
    if (mTextEdit->hasSelect()) {
      //{{{  copy, cut, delete buttons
      ImGui::SameLine();
      if (ImGui::Button ("copy"))
        mTextEdit->copy();
       if (!mTextEdit->isReadOnly()) {
         ImGui::SameLine();
        if (ImGui::Button ("cut"))
          mTextEdit->cut();
        ImGui::SameLine();
        if (ImGui::Button ("delete"))
          mTextEdit->deleteIt();
        }
      }
      //}}}
    if (!mTextEdit->isReadOnly() && mTextEdit->hasUndo()) {
      //{{{  undo button
      ImGui::SameLine();
      if (ImGui::Button ("undo"))
        mTextEdit->undo();
      }
      //}}}
    if (!mTextEdit->isReadOnly() && mTextEdit->hasRedo()) {
      //{{{  redo button
      ImGui::SameLine();
      if (ImGui::Button ("redo"))
        mTextEdit->redo();
      }
      //}}}
    //{{{  readOnly button
    ImGui::SameLine();
    if (toggleButton ("readOnly", mTextEdit->isReadOnly()))
      mTextEdit->toggleReadOnly();
    //}}}
    //{{{  overwrite button
    ImGui::SameLine();
    if (toggleButton ("insert", !mTextEdit->isOverwrite()))
      mTextEdit->toggleOverwrite();
    //}}}
    //{{{  info text
    ImGui::SameLine();
    ImGui::Text ("%d:%d:%d %s%s%s%s%s", mTextEdit->getCursorPosition().mColumn+1,
                                        mTextEdit->getCursorPosition().mLineNumber+1,
                                        mTextEdit->getTextNumLines(),
                                        mTextEdit->getLanguage().mName.c_str(),
                                        mTextEdit->isTextEdited() ? " edited":"",
                                        mTextEdit->hasTabs() ? " tabs":"",
                                        mTextEdit->hasCR() ? " CR":"",
                                        mTextEdit->getDebugString().c_str()
                                        );
    //}}}

    ImGui::PushFont (app.getMonoFont());
    mTextEdit->drawContents();
    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    }

private:
  cFileView* mFileView = nullptr;
  cTextEdit* mTextEdit = nullptr;

  //{{{
  static cUI* create (const string& className) {
    return new cTextEditUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("textEdit", &create);
  };
