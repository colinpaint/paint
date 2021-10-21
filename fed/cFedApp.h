// cFedApp.h - playerApp info holder from playerMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../ui/cApp.h"

class cPlatform;
class cGraphics;
class cDocument;
//}}}

class cFedApp : public cApp {
public:
  cFedApp (cPlatform& platform, cGraphics& graphics);
  ~cFedApp() = default;

  cDocument* getDocument() const;
  bool setDocumentName (const std::string& filename);

private:
  std::vector<cDocument*> mDocuments;
  };
