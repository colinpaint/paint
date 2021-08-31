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
//{{{  repeating format
//int count = 10; fmt::format repeat
//fmt::format("{:\t>{}}", "", count);
//}}}

// public:
//{{{
void cTextAnalyse::readAndParse() {
// read lines, parse for folding, save a sLine for each line to mLines,
// !!! should strip all leading spaces into indent, not just folds !!!

  // mFoldBegin, mFoldOpen,mFoldonlyOpen probably single state for foldBegin line, mFoldEnd for foldEnd lines

  resetRead();
  mLines.clear();

  uint8_t* ptr;
  uint32_t address;
  uint32_t numBytes;

  sLine line;
  line.mFoldLevel = 0;
  line.mFoldOpen = false;
  while (readLine (line.mText, line.mLineNumber, ptr, address, numBytes)) {
    line.mFoldBeginIndent = line.mText.find (kFoldBeginMarker);
    line.mFoldBegin = (line.mFoldBeginIndent != string::npos);
    if (line.mFoldBegin) {
      // found a foldStartMarker, remove it and its indent from mText, must regenerate on save
      line.mText = line.mText.substr (line.mFoldBeginIndent + kFoldBeginMarker.size());
      line.mFoldLevel++;
      line.mFoldEnd = false;
      }
    else // search for foldEndMarker, leave it in mText, never displayed, still there on save
      line.mFoldEnd = (line.mText.find (kFoldEndMarker) != string::npos);

    mLines.push_back (line);

    if (line.mFoldEnd)
      line.mFoldLevel--;
    }
  }
//}}}

bool cTextAnalyse::analyse (tCallback callback) {

  mCallback = callback;

  // iterate mLines spitting out text to display
  for (auto it = mLines.begin(); it != mLines.end(); ++it) {
    if (it->mFoldLevel == 0) // just show it
      mCallback (it->mText, it->mLineNumber);
    else if (it->mFoldBegin) {
      //  show foldBegin line, prefix indent and ... folded symbol
      string foldPrefix;
      for (int i = 0; i < it->mFoldBeginIndent; i++)
        foldPrefix += " ";
      foldPrefix += "...";

      if (it->mText.empty()) {
        // no foldBegin text, use first non-comment line foldBegin text
        // !!! should check for more folds or unfold !!!
        uint32_t lineNumber = it->mLineNumber;
        while (++it != mLines.end()) {
          if (it->mText.find (kCommentMarker) == string::npos) {
            mCallback (foldPrefix + it->mText, lineNumber);
            break;
            }
          }
        }
      else
        mCallback (foldPrefix + it->mText, it->mLineNumber);
      }
    }

  return true;
  }

#endif
