// cFedApp.cpp - fedApp info holder from fedMain
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cFedApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../utils/date.h"
#include "../utils/cLog.h"

#include "cDocument.h"

using namespace std;
//}}}

cFedApp::cFedApp (cPlatform& platform, cGraphics& graphics) : cApp (platform, graphics) {
  }

cDocument* cFedApp::getDocument() const {
  return mDocument;
  }

bool cFedApp::setDocumentName (const std::string& filename) {

  mDocument = new cDocument();
  mDocument->load (filename);
  return true;
  }
