// cFedApp.h - playerApp info holder from playerMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../app/cApp.h"

class cPlatform;
class cGraphics;
class cFedDocument;
//}}}

class cFedApp : public cApp {
public:
  cFedApp (const cPoint& windowSize, bool fullScreen, bool vsync);
  virtual ~cFedApp() = default;

  bool getMemEdit() const { return mMemEdit; };

  cFedDocument* getFedDocument() const;
  bool setDocumentName (const std::string& filename, bool memEdit);

  virtual void drop (const std::vector<std::string>& dropItems) final;

private:
  bool mMemEdit = false;
  std::string mFilename;
  std::vector<cFedDocument*> mFedDocuments;
  };
