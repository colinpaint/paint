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

  uint32_t foldIndex = 0;
  uint32_t foldLevel = 0;

  string line;
  uint32_t lineNumber;
  uint8_t* ptr;
  uint32_t address;
  uint32_t numBytes;
  while (readLine (line, lineNumber, ptr, address, numBytes)) {
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
        while (readLine (foldComment, lineNumber, ptr, address, numBytes))
          if (foldComment.find ("//") == string::npos)
            break;
        }

      string foldPrefix;
      //int count = 10; fmt::format repeat
      //fmt::format("{:\t>{}}", "", count);
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
// make index of fold starts

  resetRead();
  mFolds.clear();

  uint32_t foldLevel = 0;
  string line;
  uint32_t lineNumber;
  uint8_t* ptr;
  uint32_t address;
  uint32_t numBytes;
  while (readLine (line, lineNumber, ptr, address, numBytes))
    if (line.find ("//{{{") != string::npos)
      mFolds.push_back (sFold (getReadLineNumber(), ++foldLevel, false));
    else if (line.find ("//}}}") != string::npos)
      foldLevel--;

  return (uint32_t)mFolds.size();
  }
//}}}
#endif
