// cEditUI.cpp - will extend to deal with many file types invoking best editUI
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imgui
#include "../imgui/imgui.h"
#include "../app/myImgui.h"

// fed
#include "../fed/cTextEdit.h"
#include "../fed/cMemEdit.h"
#include "../fed/cFedApp.h"

// ui
#include "../ui/cUI.h"

#include "../common/cFileView.h"
#include "../common/cLog.h"
#include "../common/date.h"

using namespace std;
//}}}

class cEditUI : public cUI {
public:
  //{{{
  cEditUI (const string& name) : cUI(name) {
    }
  //}}}
  //{{{
  virtual ~cEditUI() {

    delete mFileView;
    delete mMemEdit;
    }
  //}}}

  void addToDrawList (cApp& app) final {

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

    if (false) {
      //{{{  fileView memEdit
      mFileView = new cFileView ("C:/projects/paint/build/Release/fed.exe");
      if (!mMemEdit)
        mMemEdit = new cMemEdit ((uint8_t*)(this), 0x10000);

      ImGui::PushFont (app.getMonoFont());
      mMemEdit->setMem (mFileView->getReadPtr(), mFileView->getReadBytesLeft());
      mMemEdit->drawWindow ("Memory Editor", 0);
      ImGui::PopFont();
      return;
      }
      //}}}
    if (false) {
      //{{{  memEdit
      if (!mMemEdit)
        mMemEdit = new cMemEdit ((uint8_t*)(this), 0x10000);

      ImGui::PushFont (app.getMonoFont());
      mMemEdit->setMem ((uint8_t*)this, 0x80000);
      mMemEdit->drawWindow ("Memory Editor", 0);
      ImGui::PopFont();
      return;
      }
      //}}}

    mTextEdit.draw (app);
    }

private:
  cFileView* mFileView = nullptr;
  cMemEdit* mMemEdit = nullptr;

  cTextEdit mTextEdit;

  //{{{
  static cUI* create (const string& className) {
    return new cEditUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("edit", &create);
  };
