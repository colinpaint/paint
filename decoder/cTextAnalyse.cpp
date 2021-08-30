// cTextAnalyse.cpp
#ifdef _WIN32
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include "cTextAnalyse.h"

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#include "../utils/formatCore.h"

using namespace std;
//}}}

// public:
//{{{
bool cTextAnalyse::analyse (tCallback callback) {

  mCallback = callback;
  resetRead();

  uint32_t foldIndex = 0;
  uint32_t foldLevel = 0;
  string line;
  while (readLine (line)) {
    size_t foldStart = line.find ("//{{{");
    if (foldStart != string::npos) {
      foldIndex++;
      foldLevel++;
      }

    if (foldLevel == 0)
      // just use line
      mCallback (line, 0, 0);

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
      mCallback (foldPrefix + foldComment, 1, foldIndex);
      }

    else if (line.find ("//}}}") != string::npos)
      foldLevel--;
    }

  return true;
  }
//}}}
//{{{
uint32_t cTextAnalyse::indexFolds() {

  resetRead();
  mFoldsIndex.clear();

  string line;
  while (readLine (line))
    if (line.find ("//{{{") != string::npos)
      mFoldsIndex.push_back (sFold (getReadLineNumber(), false));

  return (uint32_t)mFoldsIndex.size();
  }
//}}}
#endif
