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
  cFedApp (const cPoint& windowSize, bool fullScreen, bool vsync);
  virtual ~cFedApp() = default;

  cDocument* getDocument() const;
  bool setDocumentName (const std::string& filename);

  void drop (const std::vector<std::string>& dropItems);

private:
  std::vector<cDocument*> mDocuments;
  };
