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
bool cTextAnalyse::analyse (tCallback callback) {

  mCallback = callback;

  // iterate lines of text
  for (auto it = mLines.begin(); it != mLines.end(); ++it) {
    if (it->mFoldLevel == 0) // just use line
      mCallback (it->mText, 0);
    else if (it->mFoldBegin) {
      // begin of new fold, save indent and comment before we search if empty for acomment
      size_t foldBeginPos = it->mFoldBeginPos;
      string foldComment = it->mFoldComment;
      if (foldComment.empty()) {
        // no fold comment, search for first none comment line
        // !!! should check for more folds or unfold !!!
        while (++it != mLines.end())
          if (it->mText.find (kCommentMarker) == string::npos) {
            foldComment = it->mText;
            break;
            }
        }

      string foldPrefix;
      for (int i = 0; i < foldBeginPos; i++)
        foldPrefix += " ";
      foldPrefix += "...";
      mCallback (foldPrefix + foldComment, 1);
      }
    }

  return true;
  }

uint32_t cTextAnalyse::index() {
// read in lines, lookinf for foldBegin, folEnd marker, maining foldLevel

  resetRead();
  mLines.clear();

  uint8_t* ptr;
  uint32_t address;
  uint32_t numBytes;

  sLine line;
  line.mFoldLevel = 0;
  line.mFoldOpen = false;
  while (readLine (line.mText, line.mLineNumber, ptr, address, numBytes)) {
    line.mFoldBeginPos = line.mText.find (kFoldBeginMarker);
    line.mFoldBegin = (line.mFoldBeginPos != string::npos);
    if (line.mFoldBegin) {
      line.mFoldComment = line.mText.substr (line.mFoldBeginPos+5);
      line.mFoldLevel++;
      line.mFoldEnd = false;
      }
    else {
      line.mFoldComment = "";
      line.mFoldEnd = (line.mText.find (kFoldEndMarker) != string::npos);
      }

    mLines.push_back (line);

    if (line.mFoldEnd)
      line.mFoldLevel--;
    }

  return (uint32_t)mFolds.size();
  }
#endif
