// cTextEditUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

#include <fstream>
#include <streambuf>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/myImguiWidgets.h"

// fed
#include "../fed/cTextEdit.h"
#include "../fed/cMemEdit.h"

// ui
#include "../ui/cUI.h"

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

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

    //if (!mFileView)
      //mFileView = new cFileView ("C:/projects/paint/build/Release/fed.exe");

    if (false) {
      //{{{  memEdit
      if (!mMemEdit)
        mMemEdit = new cMemEdit ((uint8_t*)(this), 0x10000);

      ImGui::PushFont (app.getMonoFont());
      //mMemEdit->setMem (mFileView->getReadPtr(), mFileView->getReadBytesLeft());
      mMemEdit->setMem ((uint8_t*)this, 0x80000);
      //mMemEdit->setMem ((uint8_t*)mMemEdit, sizeof(cMemEdit));
      mMemEdit->drawWindow ("Memory Editor", 0);
      ImGui::PopFont();
      return;
      }
      //}}}

    if (!mTextEdit) {
      //{{{  create cTextEdit for app name
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
      }
      //}}}

    // full screen window
    mTextEdit->drawWindow ("fed", app);
    }

private:
  cFileView* mFileView = nullptr;
  cTextEdit* mTextEdit = nullptr;
  cMemEdit* mMemEdit = nullptr;

  //{{{
  static cUI* create (const string& className) {
    return new cTextEditUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("textEdit", &create);
  };
