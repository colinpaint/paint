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

cFedApp::cFedApp (cPlatform& platform, cGraphics& graphics, ImFont* mainFont, ImFont* monoFont) 
   : cApp (platform, graphics, mainFont, monoFont) {}

cDocument* cFedApp::getDocument() const {
  return mDocuments.back();
  }

bool cFedApp::setDocumentName (const std::string& filename) {
  cDocument* document = new cDocument();
  document->load (filename);
  mDocuments.push_back (document);
  return true;
  }
