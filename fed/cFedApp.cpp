// cFedApp.cpp - fedApp info holder from fedMain
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cFedApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/cFileUtils.h"

#include "cDocument.h"

using namespace std;
//}}}

cFedApp::cFedApp (const cPoint& windowSize, bool fullScreen, bool vsync) 
    : cApp (windowSize, fullScreen, vsync) {

  setResizeCallback ([&](int width, int height) noexcept { windowResize (width, height); });
  setDropCallback ([&](vector<string> dropItems) noexcept { drop (dropItems); });
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
