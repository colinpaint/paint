// cEditUI.cpp - will extend to deal with many file types invoking best editUI
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>

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
#include "../utils/date.h"

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
    // close the file mapping object
    delete mFileView;
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
    if (!mTextEdit) {
      //{{{  create cTextEdit for app name
      mTextEdit = new cTextEdit();

      filesystem::path filePath (app.getName());
      //filesystem::path filePath = filesystem::current_path();
      cLog::log (LOGINFO, fmt::format ("rootNme   - {}", filePath.root_name().string()));
      cLog::log (LOGINFO, fmt::format ("rootDir   - {}", filePath.root_directory().string()));
      cLog::log (LOGINFO, fmt::format ("relative  - {}", filePath.relative_path().string()));
      cLog::log (LOGINFO, fmt::format ("parent    - {}", filePath.parent_path().string()));
      cLog::log (LOGINFO, fmt::format ("fileName  - {}", filePath.filename().string()));
      cLog::log (LOGINFO, fmt::format ("stem      - {}", filePath.stem().string()));
      cLog::log (LOGINFO, fmt::format ("extension - {}", filePath.extension().string()));

      int i = 0;
      for (const auto& part : filePath)
        cLog::log (LOGINFO, fmt::format ("part {}  - {}", i++, part.string()));

       if (filesystem::exists (filePath) && filesystem::is_regular_file (filePath)) {
         std::error_code err = std::error_code{};
         uint64_t fileSize = filesystem::file_size (filePath, err);
         if (fileSize != static_cast<uintmax_t>(-1))
           cLog::log (LOGINFO, fmt::format ("filesize:{} {}", fileSize, err.value()));
         }

      // set file
      ifstream fileStream (app.getName());

      string str ((istreambuf_iterator<char>(fileStream)), istreambuf_iterator<char>());
      mTextEdit->setTextString (str);
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
    return new cEditUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("edit", &create);
  };
