// cFedApp.h - playerApp info holder from playerMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../app/cApp.h"

class cPlatform;
class cGraphics;
class cDocument;
//}}}

class cFedApp : public cApp {
public:
  cFedApp (const cPoint& windowSize) : cApp (windowSize) {}
  virtual ~cFedApp() = default;

  cDocument* getDocument() const;
  bool setDocumentName (const std::string& filename);

private:
  std::vector<cDocument*> mDocuments;
  };
