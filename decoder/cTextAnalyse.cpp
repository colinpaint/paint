// cTextAnalyse.cpp
#ifdef _WIN32
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include "cTextAnalyse.h"

#include <cstdint>
#include <string>
#include <functional>

#include "../utils/formatCore.h"

using namespace std;
//}}}

// public:
bool cTextAnalyse::analyse (tCallback callback) {

  mCallback = callback;
  resetRead();

  uint32_t foldLevel = 0;
  string line;
  while (readLine (line)) {
    size_t foldStart = line.find ("//{{{");
    if (foldStart != string::npos)
      foldLevel++;

    if (foldLevel == 0)
      // jsut use line
      mCallback (0, line);

    else if (foldStart != string::npos) {
      // start of new fold, find comment in startFoldLine, or next uncommented line
      string foldComment = line.substr (foldStart+5);
      if (foldComment.empty()) {
        // no comment in startFoldLine, search for first none comment line
        // !!! should check for more folds or unfold !!!
        while (readLine (foldComment))
          if (foldComment.find ("//") == string::npos)
            break;
        }

      string foldPrefix;
      for (int i = 0; i < foldStart; i++)
        foldPrefix += " ";
      foldPrefix += "...";
      mCallback (1, foldPrefix + foldComment);
      }
    else if (line.find ("//}}}") != string::npos)
      foldLevel--;
    }

  return true;
  }
#endif
