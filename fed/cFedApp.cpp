// cFedApp.cpp - fedApp info holder from fedMain
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include "cFedApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/fileUtils.h"

#include "cFedDocument.h"

using namespace std;
//}}}

cFedApp::cFedApp (const cPoint& windowSize, bool fullScreen, bool vsync)
    : cApp ("fed", windowSize, fullScreen, vsync) {}
cFedDocument* cFedApp::getFedDocument() const { return mFedDocuments.back(); }

bool cFedApp::setDocumentName (const std::string& filename, bool memEdit) {
  mFilename = cFileUtils::resolve (filename);
  cFedDocument* document = new cFedDocument();
  document->load (mFilename);
  mFedDocuments.push_back (document);
  mMemEdit = memEdit;
  return true;
  }

void cFedApp::drop (const vector<string>& dropItems) {
  for (auto& item : dropItems) {
    string filename = cFileUtils::resolve (item);
    setDocumentName (filename, mMemEdit);
    cLog::log (LOGINFO, filename);
    }
  }
