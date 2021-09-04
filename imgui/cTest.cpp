// cTextEditor.cpp - nicked from https://github.com/BalazsJako/ImGuiColorTextEdit
// - remember this file is used to test itself
//{{{  includes
// dummy comment

/* and some old style c comments
   to test comment handling in this editor
*/

#include <cstdint>
#include <cmath>
#include <algorithm>

#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <chrono>
#include <regex>
#include <functional>

#include "cTextEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

#include "../utils/cLog.h"

using namespace std;
//}}}

//{{{  template equals
template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals (InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, BinaryPredicate p) {
  for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
    if (!p (*first1, *first2))
      return false;
    }

  return first1 == last1 && first2 == last2;
  }
//}}}

constexpr int kLeftTextMargin = 10;

//{{{
void cTextEditor::parseFolds() {
// parse beginFold and endFold markers, set simple flags

  for (auto& line : mLines) {
    string text;
    for (auto& glyph : line.mGlyphs)
      text += glyph.mChar;

    size_t foldBeginIndent = text.find (mLanguage.mFoldBeginMarker);
    line.mFoldBegin = (foldBeginIndent != string::npos);

    if (line.mFoldBegin) {
      line.mIndent = static_cast<uint16_t>(foldBeginIndent + mLanguage.mFoldBeginMarker.size());
      // has text after the foldBeginMarker
      line.mHasComment = (text.size() != (foldBeginIndent + mLanguage.mFoldBeginMarker.size()));
      }
    else {
      size_t indent = text.find_first_not_of (' ');
      if (indent != string::npos)
        line.mIndent = static_cast<uint16_t>(indent);
      else
        line.mIndent = 0;

      // has "//" style comment as first text in line
      line.mHasComment = (text.find (mLanguage.mSingleLineComment, indent) != string::npos);
      }

    size_t foldEndIndent = text.find (mLanguage.mFoldEndMarker);
    line.mFoldEnd = (foldEndIndent != string::npos);

    // init fields set by updateFolds
    line.mFoldLevel = 0;
    line.mFoldOpen = false;
    line.mFoldLineNumber = 0;
    line.mFoldTitleLineNumber = 0xFFFFFFFF;
    }

  updateFolds();
  }
//}}}

//{{{
cTextEditor::sUndoRecord::sUndoRecord (const string& added,
                                       const cTextEditor::sPosition addedStart,
                                       const cTextEditor::sPosition addedEnd,
                                       const string& removed,
                                       const cTextEditor::sPosition removedStart,
                                       const cTextEditor::sPosition removedEnd,
                                       cTextEditor::sEditorState& before,
                                       cTextEditor::sEditorState& after)

    : mAdded(added), mAddedStart(addedStart), mAddedEnd(addedEnd),
      mRemoved(removed), mRemovedStart(removedStart), mRemovedEnd(removedEnd),
      mBefore(before), mAfter(after) {

  assert (mAddedStart <= mAddedEnd);
  assert (mRemovedStart <= mRemovedEnd);
  }
//}}}

//{{{
void cTextEditor::sUndoRecord::undo (cTextEditor* editor) {

  if (!mAdded.empty()) {
    editor->deleteRange (mAddedStart, mAddedEnd);
    editor->colorize (mAddedStart.mLineNumber - 1, mAddedEnd.mLineNumber - mAddedStart.mLineNumber + 2);
    }

  if (!mRemoved.empty()) {
    sPosition start = mRemovedStart;
    editor->insertTextAt (start, mRemoved.c_str());
    editor->colorize (mRemovedStart.mLineNumber - 1, mRemovedEnd.mLineNumber - mRemovedStart.mLineNumber + 2);
    }

  editor->mState = mBefore;
  editor->ensureCursorVisible();
  }
//}}}
//{{{
void cTextEditor::sUndoRecord::redo (cTextEditor* editor) {

  if (!mRemoved.empty()) {
    editor->deleteRange (mRemovedStart, mRemovedEnd);
    editor->colorize (mRemovedStart.mLineNumber - 1, mRemovedEnd.mLineNumber - mRemovedStart.mLineNumber + 1);
    }

  if (!mAdded.empty()) {
    sPosition start = mAddedStart;
    editor->insertTextAt (start, mAdded.c_str());
    editor->colorize (mAddedStart.mLineNumber - 1, mAddedEnd.mLineNumber - mAddedStart.mLineNumber + 1);
    }

  editor->mState = mAfter;
  editor->ensureCursorVisible();
  }
//}}}
