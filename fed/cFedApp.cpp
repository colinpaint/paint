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

#include "cDocument.h"

using namespace std;
//}}}

cFedApp::cFedApp (const cPoint& windowSize, bool fullScreen, bool vsync)
    : cApp ("fed", windowSize, fullScreen, vsync) {
  }

cDocument* cFedApp::getDocument() const {
  return mDocuments.back();
  }

bool cFedApp::setDocumentName (const std::string& filename) {
  cDocument* document = new cDocument();
  document->load (filename);
  mDocuments.push_back (document);
  return true;
  }

void cFedApp::drop (const vector<string>& dropItems) {
  for (auto& item : dropItems) {
    string filename = cFileUtils::resolve (item);
    setDocumentName (filename);
    cLog::log (LOGINFO, filename);
    }
  }
