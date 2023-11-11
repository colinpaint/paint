// cFedUI.cpp - will extend to deal with many file types invoking best editUI
//{{{  includes
#include <stdio.h>  // sprintf, scanf
#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <string>
#include <regex>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <fstream>
#include <filesystem>

#include "../app/cPlatform.h"

// imgui
#include "../imgui/imgui.h"
#include "../app/myImgui.h"

// fed
#include "cFedDocument.h"
#include "cFedApp.h"

// ui
#include "../ui/cUI.h"

#include "../common/cLog.h"
#include "../common/date.h"
#include "../common/cFileView.h"

#include "fmt/format.h"

using namespace std;
using namespace chrono;
//}}}

//{{{
class cFed {
public:
  cFed();
  ~cFed() = default;

  enum class eSelect { eNormal, eWord, eLine };
  //{{{  gets
  cFedDocument* getFedDocument() { return mFedDocument; }
  cLanguage& getLanguage() { return mFedDocument->getLanguage(); }

  bool isReadOnly() const { return mOptions.mReadOnly; }
  bool isShowFolds() const { return mOptions.mShowFolded; }

  // has
  bool hasSelect() const { return mEdit.mCursor.mSelectEndPosition > mEdit.mCursor.mSelectBeginPosition; }
  bool hasUndo() const { return !mOptions.mReadOnly && mEdit.hasUndo(); }
  bool hasRedo() const { return !mOptions.mReadOnly && mEdit.hasRedo(); }
  bool hasPaste() const { return !mOptions.mReadOnly && mEdit.hasPaste(); }

  // get
  std::string getTextString();
  std::vector<std::string> getTextStrings() const;

  sPosition getCursorPosition() { return sanitizePosition (mEdit.mCursor.mPosition); }
  //}}}
  //{{{  sets
  void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }

  void toggleReadOnly() { mOptions.mReadOnly = !mOptions.mReadOnly; }
  void toggleOverWrite() { mOptions.mOverWrite = !mOptions.mOverWrite; }

  void toggleShowLineNumber() { mOptions.mShowLineNumber = !mOptions.mShowLineNumber; }
  void toggleShowLineDebug() { mOptions.mShowLineDebug = !mOptions.mShowLineDebug; }
  void toggleShowWhiteSpace() { mOptions.mShowWhiteSpace = !mOptions.mShowWhiteSpace; }
  void toggleShowMonoSpaced() { mOptions.mShowMonoSpaced = !mOptions.mShowMonoSpaced; }

  void toggleShowFolded();
  //}}}
  //{{{  actions
  // move
  void moveLeft();
  void moveRight();
  void moveRightWord();

  void moveLineUp()   { moveUp (1); }
  void moveLineDown() { moveDown (1); }
  void movePageUp()   { moveUp (getNumPageLines() - 4); }
  void movePageDown() { moveDown (getNumPageLines() - 4); }
  void moveHome() { setCursorPosition ({0,0}); }
  void moveEnd() { setCursorPosition ({mFedDocument->getMaxLineNumber(), 0}); }

  // select
  void selectAll();

  // cut and paste
  void copy();
  void cut();
  void paste();

  // delete
  void deleteIt();
  void backspace();

  // enter
  void enterCharacter (ImWchar ch);
  void enterKey() { enterCharacter ('\n'); }
  void tabKey() { enterCharacter ('\t'); }

  // undo
  void undo (uint32_t steps = 1);
  void redo (uint32_t steps = 1);

  // fold
  void createFold();
  void openFold() { openFold (mEdit.mCursor.mPosition.mLineNumber); }
  void openFoldOnly() { openFoldOnly (mEdit.mCursor.mPosition.mLineNumber); }
  void closeFold() { closeFold (mEdit.mCursor.mPosition.mLineNumber); }
  //}}}

  void draw (cApp& app);
  void drawContents (cApp& app);

private:
  //{{{
  struct sCursor {
    sPosition mPosition;

    eSelect mSelect;
    sPosition mSelectBeginPosition;
    sPosition mSelectEndPosition;
    };
  //}}}
  //{{{
  class cUndo {
  public:
    //{{{
    void undo (cFed* textEdit) {

      if (!mAddText.empty())
        textEdit->getFedDocument()->deletePositionRange (mAddBeginPosition, mAddEndPosition);
      if (!mDeleteText.empty())
        textEdit->insertText (mDeleteText, mDeleteBeginPosition);

      textEdit->mEdit.mCursor = mBeforeCursor;
      }
    //}}}
    //{{{
    void redo (cFed* textEdit) {

      if (!mDeleteText.empty())
        textEdit->getFedDocument()->deletePositionRange (mDeleteBeginPosition, mDeleteEndPosition);
      if (!mAddText.empty())
        textEdit->insertText (mAddText, mAddBeginPosition);

      textEdit->mEdit.mCursor = mAfterCursor;
      }
    //}}}

    // vars
    sCursor mBeforeCursor;
    sCursor mAfterCursor;

    std::string mAddText;
    sPosition mAddBeginPosition;
    sPosition mAddEndPosition;

    std::string mDeleteText;
    sPosition mDeleteBeginPosition;
    sPosition mDeleteEndPosition;
    };
  //}}}
  //{{{
  class cEdit {
  public:
    // undo
    bool hasUndo() const { return mUndoIndex > 0; }
    bool hasRedo() const { return mUndoIndex < mUndoVector.size(); }
    //{{{
    void clearUndo() {
      mUndoVector.clear();
      mUndoIndex = 0;
      }
    //}}}
    //{{{
    void undo (cFed* textEdit, uint32_t steps) {

      while (hasUndo() && (steps > 0)) {
        mUndoVector [--mUndoIndex].undo (textEdit);
        steps--;
        }
      }
    //}}}
    //{{{
    void redo (cFed* textEdit, uint32_t steps) {

      while (hasRedo() && steps > 0) {
        mUndoVector [mUndoIndex++].redo (textEdit);
        steps--;
        }
      }
    //}}}
    //{{{
    void addUndo (cUndo& undo) {

      // trim undo list to mUndoIndex, add undo
      mUndoVector.resize (mUndoIndex+1);
      mUndoVector.back() = undo;
      mUndoIndex++;
      }
    //}}}

    // paste
    bool hasPaste() const { return !mPasteVector.empty(); }
    //{{{
    std::string popPasteText () {

      if (mPasteVector.empty())
        return "";
      else {
        std::string text = mPasteVector.back();
        mPasteVector.pop_back();
        return text;
        }
      }
    //}}}
    //{{{
    void pushPasteText (std::string text) {
      ImGui::SetClipboardText (text.c_str());
      mPasteVector.push_back (text);
      }
    //}}}

    // vars
    sCursor mCursor;

    // drag state
    sPosition mDragFirstPosition;

    // foldOnly state
    bool mFoldOnly = false;
    uint32_t mFoldOnlyBeginLineNumber = 0;

    // delayed action flags
    bool mParseDocument = true;
    bool mScrollVisible = false;

  private:
    // undo
    size_t mUndoIndex = 0;
    std::vector <cUndo> mUndoVector;

    // paste
    std::vector <std::string> mPasteVector;
    };
  //}}}
  //{{{
  class cOptions {
  public:
    int mFontSize = 16;
    int mMinFontSize = 4;
    int mMaxFontSize = 24;

    // modes
    bool mOverWrite = false;
    bool mReadOnly = false;

    // shows
    bool mShowFolded = true;
    bool mShowLineNumber = true;
    bool mShowLineDebug = false;
    bool mShowWhiteSpace = false;
    bool mShowMonoSpaced = true;
    };
  //}}}
  //{{{
  class cFedDrawContext : public cDrawContext {
  // drawContext with our palette and a couple of layout vars
  public:
    cFedDrawContext() : cDrawContext (kPalette) {}
    //{{{
    void update (float fontSize, bool monoSpaced) {

      cDrawContext::update (fontSize, monoSpaced);

      mLeftPad = getGlyphWidth() / 2.f;
      mLineNumberWidth = 0.f;
      }
    //}}}

    float mLeftPad = 0.f;
    float mLineNumberWidth = 0.f;

  private:
    // color to ImU32 lookup
    inline static const std::vector <ImU32> kPalette = {
      0xff808080, // eText
      0xffefefef, // eBackground
      0xff202020, // eIdentifier
      0xff606000, // eNumber
      0xff404040, // ePunctuation
      0xff2020a0, // eString
      0xff304070, // eLiteral
      0xff008080, // ePreProc
      0xff008000, // eComment
      0xff1010c0, // eKeyWord
      0xff800080, // eKnownWord

      0xff000000, // eCursorPos
      0x10000000, // eCursorLineFill
      0x40000000, // eCursorLineEdge
      0x800000ff, // eCursorReadOnly
      0x80600000, // eSelectCursor
      0xff505000, // eLineNumber
      0xff808080, // eWhiteSpace
      0xff404040, // eTab

      0xffff0000, // eFoldClosed,
      0xff0000ff, // eFoldOpen,

      0x80404040, // eScrollBackground
      0x80ffffff, // eScrollGrab
      0xA000ffff, // eScrollHover
      0xff00ffff, // eScrollActive
      };
    };
  //}}}

  //{{{  get
  bool isFolded() const { return mOptions.mShowFolded; }
  bool isDrawLineNumber() const { return mOptions.mShowLineNumber; }
  bool isDrawWhiteSpace() const { return mOptions.mShowWhiteSpace; }
  bool isDrawMonoSpaced() const { return mOptions.mShowMonoSpaced; }

  bool canEditAtCursor();

  // text
  std::string getSelectText() { return mFedDocument->getText (mEdit.mCursor.mSelectBeginPosition, mEdit.mCursor.mSelectEndPosition); }

  // text widths
  float getWidth (sPosition position);
  float getGlyphWidth (const cLine& line, uint32_t glyphIndex);

  // lines
  uint32_t getNumFoldLines() const { return static_cast<uint32_t>(mFoldLines.size()); }
  uint32_t getMaxFoldLineIndex() const { return getNumFoldLines() - 1; }
  uint32_t getNumPageLines() const;

  // line
  uint32_t getLineNumberFromIndex (uint32_t lineIndex) const;
  uint32_t getLineIndexFromNumber (uint32_t lineNumber) const;

  //{{{
  uint32_t getGlyphsLineNumber (uint32_t lineNumber) const {
  // return glyphs lineNumber for lineNumber - folded foldBegin closedFold seeThru into next line

    const cLine& line = mFedDocument->getLine (lineNumber);
    if (isFolded() && line.mFoldBegin && !line.mFoldOpen && (line.mFirstGlyph == line.getNumGlyphs()))
      return lineNumber + 1;
    else
      return lineNumber;
    }
  //}}}
  cLine& getGlyphsLine (uint32_t lineNumber) { return mFedDocument->getLine (getGlyphsLineNumber (lineNumber)); }

  sPosition getNextLinePosition (const sPosition& position);

  // column pos
  float getTabEndPosX (float columnX);
  uint32_t getColumnFromPosX (const cLine& line, float posX);
  //}}}
  //{{{  set
  void setCursorPosition (sPosition position);

  void deselect();
  void setSelect (eSelect select, sPosition beginPosition, sPosition endPosition);
  //}}}
  //{{{  move
  void moveUp (uint32_t amount);
  void moveDown (uint32_t amount);

  void cursorFlashOn();
  void scrollCursorVisible();

  sPosition advancePosition (sPosition position);
  sPosition sanitizePosition (sPosition position);

  sPosition findWordBeginPosition (sPosition position);
  sPosition findWordEndPosition (sPosition position);
  //}}}

  sPosition insertText (const std::string& text, sPosition position);
  void insertText (const std::string& text) { setCursorPosition (insertText (text, getCursorPosition())); }
  void deleteSelect();

  //  fold
  void openFold (uint32_t lineNumber);
  void openFoldOnly (uint32_t lineNumber);
  void closeFold (uint32_t lineNumber);
  uint32_t skipFold (uint32_t lineNumber);
  uint32_t drawFolded();

  // keyboard,mouse
  void keyboard();
  void mouseSelectLine (uint32_t lineNumber);
  void mouseDragSelectLine (uint32_t lineNumber, float posY);
  void mouseSelectText (bool selectWord, uint32_t lineNumber, ImVec2 pos);
  void mouseDragSelectText (uint32_t lineNumber, ImVec2 pos);

  // draw
  float drawGlyphs (ImVec2 pos, const cLine& line, uint8_t forceColor);
  void drawSelect (ImVec2 pos, uint32_t lineNumber, uint32_t glyphsLineNumber);
  void drawCursor (ImVec2 pos, uint32_t lineNumber);
  void drawLine (uint32_t lineNumber, uint32_t lineIndex);
  void drawLines();

  // vars
  cFedDocument* mFedDocument = nullptr;
  bool mInited = false;

  // edit state
  cEdit mEdit;
  cOptions mOptions;
  cFedDrawContext mDrawContext;

  std::vector <uint32_t> mFoldLines;

  std::chrono::system_clock::time_point mCursorFlashTimePoint;
  };
//}}}
//{{{  cFed implementation
//{{{
cFed::cFed() {

  cursorFlashOn();
  }
//}}}

// actions
//{{{
void cFed::moveLeft() {

  deselect();
  sPosition position = mEdit.mCursor.mPosition;

  // line
  cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

  // column
  uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
  if (glyphIndex > glyphsLine.mFirstGlyph)
    // move to prevColumn on sameLine
    setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex - 1)});

  else if (position.mLineNumber > 0) {
    // at begining of line, move to end of prevLine
    uint32_t moveLineIndex = getLineIndexFromNumber (position.mLineNumber) - 1;
    uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
    cLine& moveGlyphsLine = getGlyphsLine (moveLineNumber);
    setCursorPosition ({moveLineNumber, mFedDocument->getColumn (moveGlyphsLine, moveGlyphsLine.getNumGlyphs())});
    }
  }
//}}}
//{{{
void cFed::moveRight() {

  deselect();
  sPosition position = mEdit.mCursor.mPosition;

  // line
  cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

  // column
  uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
  if (glyphIndex < glyphsLine.getNumGlyphs())
    // move to nextColumm on sameLine
    setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex + 1)});

  else {
    // move to start of nextLine
    uint32_t moveLineIndex = getLineIndexFromNumber (position.mLineNumber) + 1;
    if (moveLineIndex < getNumFoldLines()) {
      uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
      cLine& moveGlyphsLine = getGlyphsLine (moveLineNumber);
      setCursorPosition ({moveLineNumber, moveGlyphsLine.mFirstGlyph});
      }
    }
  }
//}}}
//{{{
void cFed::moveRightWord() {

  deselect();
  sPosition position = mEdit.mCursor.mPosition;

  // line
  uint32_t glyphsLineNumber = getGlyphsLineNumber (position.mLineNumber);
  cLine& glyphsLine = mFedDocument->getLine (glyphsLineNumber);

  // find nextWord
  sPosition movePosition;
  uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
  if (glyphIndex < glyphsLine.getNumGlyphs()) {
    // middle of line, find next word
    bool skip = false;
    bool isWord = false;
    if (glyphIndex < glyphsLine.getNumGlyphs()) {
      isWord = isalnum (glyphsLine.getChar (glyphIndex));
      skip = isWord;
      }

    while ((!isWord || skip) && (glyphIndex < glyphsLine.getNumGlyphs())) {
      isWord = isalnum (glyphsLine.getChar (glyphIndex));
      if (isWord && !skip) {
        setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex)});
        return;
        }
      if (!isWord)
        skip = false;
      glyphIndex++;
      }

    setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex)});
    }
  else {
    // !!!! implement inc to next line !!!!!
    }
  }
//}}}
//{{{
void cFed::selectAll() {
  setSelect (eSelect::eNormal, {0,0}, { mFedDocument->getNumLines(),0});
  }
//}}}
//{{{
void cFed::copy() {

  if (!canEditAtCursor())
    return;

  if (hasSelect()) {
    // push selectedText to pasteStack
    mEdit.pushPasteText (getSelectText());
    deselect();
    }

  else {
    // push currentLine text to pasteStack
    sPosition position {getCursorPosition().mLineNumber, 0};
    mEdit.pushPasteText (mFedDocument->getText (position, getNextLinePosition (position)));

    // moveLineDown for any multiple copy
    moveLineDown();
    }
  }
//}}}
//{{{
void cFed::cut() {

  cUndo undo;
  undo.mBeforeCursor = mEdit.mCursor;

  if (hasSelect()) {
    // push selectedText to pasteStack
    string text = getSelectText();
    mEdit.pushPasteText (text);

    // cut selected range
    undo.mDeleteText = text;
    undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
    undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
    deleteSelect();
    }

  else {
    // push current line text to pasteStack
    sPosition position {getCursorPosition().mLineNumber,0};
    sPosition nextLinePosition = getNextLinePosition (position);

    string text = mFedDocument->getText (position, nextLinePosition);
    mEdit.pushPasteText (text);

    // cut current line
    undo.mDeleteText = text;
    undo.mDeleteBeginPosition = position;
    undo.mDeleteEndPosition = nextLinePosition;
    mFedDocument->deleteLineRange (position.mLineNumber, nextLinePosition.mLineNumber);
    }

  undo.mAfterCursor = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}
//{{{
void cFed::paste() {

  if (hasPaste()) {
    cUndo undo;
    undo.mBeforeCursor = mEdit.mCursor;

    if (hasSelect()) {
      // delete selectedText
      undo.mDeleteText = getSelectText();
      undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
      undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
      deleteSelect();
      }

    string text = mEdit.popPasteText();
    undo.mAddText = text;
    undo.mAddBeginPosition = getCursorPosition();

    insertText (text);

    undo.mAddEndPosition = getCursorPosition();
    undo.mAfterCursor = mEdit.mCursor;
    mEdit.addUndo (undo);

    // moveLineUp for any multiple paste
    moveLineUp();
    }
  }
//}}}
//{{{
void cFed::deleteIt() {

  cUndo undo;
  undo.mBeforeCursor = mEdit.mCursor;

  sPosition position = getCursorPosition();
  setCursorPosition (position);
  cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

  if (hasSelect()) {
    // delete selected range
    undo.mDeleteText = getSelectText();
    undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
    undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
    deleteSelect();
    }

  else if (position.mColumn == mFedDocument->getNumColumns (glyphsLine)) {
    if (position.mLineNumber >= mFedDocument->getMaxLineNumber())
      // no next line to join
      return;

    undo.mDeleteText = '\n';
    undo.mDeleteBeginPosition = getCursorPosition();
    undo.mDeleteEndPosition = advancePosition (undo.mDeleteBeginPosition);

    // join next line to this line
    mFedDocument->joinLine (glyphsLine, position.mLineNumber + 1);
    }

  else {
    // delete character
    undo.mDeleteBeginPosition = getCursorPosition();
    undo.mDeleteEndPosition = undo.mDeleteBeginPosition;
    undo.mDeleteEndPosition.mColumn++;
    undo.mDeleteText = mFedDocument->getText (undo.mDeleteBeginPosition, undo.mDeleteEndPosition);

    mFedDocument->deleteChar (glyphsLine, position);
    }

  undo.mAfterCursor = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}
//{{{
void cFed::backspace() {

  cUndo undo;
  undo.mBeforeCursor = mEdit.mCursor;

  sPosition position = getCursorPosition();
  cLine& line = mFedDocument->getLine (position.mLineNumber);

  if (hasSelect()) {
    // delete select
    undo.mDeleteText = getSelectText();
    undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
    undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
    deleteSelect();
    }

  else if (position.mColumn <= line.mFirstGlyph) {
    // backspace at beginning of line, append line to previous line
    if (isFolded() && line.mFoldBegin) // don't backspace to prevLine at foldBegin
      return;
    if (!position.mLineNumber) // already on firstLine, nowhere to go
      return;

    // previous Line
    uint32_t prevLineNumber = position.mLineNumber - 1;
    cLine& prevLine = mFedDocument->getLine (prevLineNumber);
    uint32_t prevLineEndColumn = mFedDocument->getNumColumns (prevLine);

    // save lineFeed undo
    undo.mDeleteText = '\n';
    undo.mDeleteBeginPosition = {prevLineNumber, prevLineEndColumn};
    undo.mDeleteEndPosition = advancePosition (undo.mDeleteBeginPosition);

    mFedDocument->appendLineToPrev (position.mLineNumber);

    // position to end of prevLine
    setCursorPosition ({prevLineNumber, prevLineEndColumn});
    }

  else {
    // at middle of line, delete previous char
    cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
    uint32_t prevGlyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn) - 1;

    // save previous char undo
    undo.mDeleteEndPosition = getCursorPosition();
    undo.mDeleteBeginPosition = getCursorPosition();
    undo.mDeleteBeginPosition.mColumn--;
    undo.mDeleteText += glyphsLine.getChar (prevGlyphIndex);

    // delete previous char
    mFedDocument->deleteChar (glyphsLine, prevGlyphIndex);

    // position to previous char
    setCursorPosition ({position.mLineNumber, position.mColumn - 1});
    }

  undo.mAfterCursor = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}
//{{{
void cFed::enterCharacter (ImWchar ch) {
// !!!! more utf8 handling !!!

  if (!canEditAtCursor())
    return;

  cUndo undo;
  undo.mBeforeCursor = mEdit.mCursor;

  if (hasSelect()) {
    //{{{  delete selected range
    undo.mDeleteText = getSelectText();
    undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
    undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;

    deleteSelect();
    }
    //}}}

  sPosition position = getCursorPosition();
  undo.mAddBeginPosition = position;

  cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
  uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);

  if (ch == '\n') {
    // newLine
    if (isFolded() && mFedDocument->getLine (position.mLineNumber).mFoldBegin)
      // no newLine in folded foldBegin - !!!! could allow with no cursor move, still odd !!!
      return;

    // break glyphsLine at glyphIndex, with new line indent
    mFedDocument->breakLine (glyphsLine, glyphIndex, position.mLineNumber + 1, glyphsLine.mIndent);

    // set cursor to newLine, start of indent
    setCursorPosition ({position.mLineNumber+1, glyphsLine.mIndent});
    }
  else {
    // char
    if (mOptions.mOverWrite && (glyphIndex < glyphsLine.getNumGlyphs())) {
      // overwrite by deleting cursor char
      undo.mDeleteBeginPosition = mEdit.mCursor.mPosition;
      undo.mDeleteEndPosition = {position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex+1)};
      undo.mDeleteText += glyphsLine.getChar (glyphIndex);
      glyphsLine.erase (glyphIndex);
      }

    // insert new char
    mFedDocument->insertChar (glyphsLine, glyphIndex, ch);
    setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex + 1)});
    }
  undo.mAddText = static_cast<char>(ch); // !!!! = utf8buf.data() utf8 handling needed !!!!

  undo.mAddEndPosition = getCursorPosition();
  undo.mAfterCursor = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}
//{{{
void cFed::undo (uint32_t steps) {

  if (hasUndo()) {
    mEdit.undo (this, steps);
    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cFed::redo (uint32_t steps) {

  if (hasRedo()) {
    mEdit.redo (this, steps);
    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cFed::toggleShowFolded() {

  mOptions.mShowFolded = !mOptions.mShowFolded;
  mEdit.mScrollVisible = true;
  }
//}}}
//{{{
void cFed::createFold() {

  // !!!! temp string for now !!!!
  string text = getLanguage().mFoldBeginToken +
                 "  new fold - loads of detail to implement\n\n" +
                getLanguage().mFoldEndToken +
                 "\n";
  cUndo undo;
  undo.mBeforeCursor = mEdit.mCursor;
  undo.mAddText = text;
  undo.mAddBeginPosition = getCursorPosition();

  insertText (text);

  undo.mAddEndPosition = getCursorPosition();
  undo.mAfterCursor = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}

// draws
//{{{
void cFed::draw (cApp& app) {
// standalone fed window

   if (!mInited) {
     // push any clipboardText to pasteStack
     const char* clipText = ImGui::GetClipboardText();
     if (clipText && (strlen (clipText)) > 0)
       mEdit.pushPasteText (clipText);
     mInited = true;
     }

   // create document editor
  cFedApp& fedApp = (cFedApp&)app;
  mFedDocument = fedApp.getFedDocument();
  if (mFedDocument) {
    bool open = true;
    ImGui::Begin ("fed", &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
    drawContents (app);
    ImGui::End();
    }
  }
//}}}
//{{{
void cFed::drawContents (cApp& app) {
// main ui draw

  // check for delayed all document parse
  mFedDocument->parseAll();
  //{{{  draw top line buttons
  //{{{  lineNumber buttons
  if (toggleButton ("line", mOptions.mShowLineNumber))
    toggleShowLineNumber();

  if (mOptions.mShowLineNumber)
    // debug button
    if (mOptions.mShowLineNumber) {
      ImGui::SameLine();
      if (toggleButton ("debug", mOptions.mShowLineDebug))
        toggleShowLineDebug();
      }
  //}}}

  if (mFedDocument->hasFolds()) {
    //{{{  folded button
    ImGui::SameLine();
    if (toggleButton ("folded", isShowFolds()))
      toggleShowFolded();
    }
    //}}}

  //{{{  monoSpaced buttom
  ImGui::SameLine();
  if (toggleButton ("mono", mOptions.mShowMonoSpaced))
    toggleShowMonoSpaced();
  //}}}
  //{{{  whiteSpace button
  ImGui::SameLine();
  if (toggleButton ("space", mOptions.mShowWhiteSpace))
    toggleShowWhiteSpace();
  //}}}

  if (hasPaste()) {
    //{{{  paste button
    ImGui::SameLine();
    if (ImGui::Button ("paste"))
      paste();
    }
    //}}}

  if (hasSelect()) {
    //{{{  copy, cut, delete buttons
    ImGui::SameLine();
    if (ImGui::Button ("copy"))
      copy();
     if (!isReadOnly()) {
       ImGui::SameLine();
      if (ImGui::Button ("cut"))
        cut();
      ImGui::SameLine();
      if (ImGui::Button ("delete"))
        deleteIt();
      }
    }
    //}}}

  if (!isReadOnly() && hasUndo()) {
    //{{{  undo button
    ImGui::SameLine();
    if (ImGui::Button ("undo"))
      undo();
    }
    //}}}

  if (!isReadOnly() && hasRedo()) {
    //{{{  redo button
    ImGui::SameLine();
    if (ImGui::Button ("redo"))
      redo();
    }
    //}}}

  //{{{  insert button
  ImGui::SameLine();
  if (toggleButton ("insert", !mOptions.mOverWrite))
    toggleOverWrite();
  //}}}
  //{{{  readOnly button
  ImGui::SameLine();
  if (toggleButton ("readOnly", isReadOnly()))
    toggleReadOnly();
  //}}}

  //{{{  fontSize button
  ImGui::SameLine();
  ImGui::SetNextItemWidth (3 * ImGui::GetFontSize());
  ImGui::DragInt ("##fs", &mOptions.mFontSize, 0.2f, mOptions.mMinFontSize, mOptions.mMaxFontSize, "%d");

  if (ImGui::IsItemHovered()) {
    int fontSize = mOptions.mFontSize + static_cast<int>(ImGui::GetIO().MouseWheel);
    mOptions.mFontSize = max (mOptions.mMinFontSize, min (mOptions.mMaxFontSize, fontSize));
    }
  //}}}
  //{{{  vsync button,fps
  if (app.getPlatform().hasVsync()) {
    // vsync button
    ImGui::SameLine();
    if (toggleButton ("vSync", app.getPlatform().getVsync()))
        app.getPlatform().toggleVsync();

    // fps text
    ImGui::SameLine();
    ImGui::Text (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
    }
  //}}}

  //{{{  fullScreen button
  if (app.getPlatform().hasFullScreen()) {
    ImGui::SameLine();
    if (toggleButton ("full", app.getPlatform().getFullScreen()))
      app.getPlatform().toggleFullScreen();
    }
  //}}}
  //{{{  save button
  if (mFedDocument->isEdited()) {
    ImGui::SameLine();
    if (ImGui::Button ("save"))
      mFedDocument->save();
    }
  //}}}

  //{{{  info
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:{} {}", getCursorPosition().mColumn+1, getCursorPosition().mLineNumber+1,
      mFedDocument->getNumLines(), getLanguage().mName).c_str());
  //}}}
  //{{{  vertice debug
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}",
               ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());
  //}}}

  ImGui::SameLine();
  ImGui::TextUnformatted (mFedDocument->getFileName().c_str());
  //}}}

  // begin childWindow, new font, new colors
  if (isDrawMonoSpaced())
    ImGui::PushFont (app.getMonoFont());

  ImGui::PushStyleColor (ImGuiCol_ScrollbarBg, mDrawContext.getColor (eScrollBackground));
  ImGui::PushStyleColor (ImGuiCol_ScrollbarGrab, mDrawContext.getColor (eScrollGrab));
  ImGui::PushStyleColor (ImGuiCol_ScrollbarGrabHovered, mDrawContext.getColor (eScrollHover));
  ImGui::PushStyleColor (ImGuiCol_ScrollbarGrabActive, mDrawContext.getColor (eScrollActive));
  ImGui::PushStyleColor (ImGuiCol_Text, mDrawContext.getColor (eText));
  ImGui::PushStyleColor (ImGuiCol_ChildBg, mDrawContext.getColor (eBackground));
  ImGui::PushStyleVar (ImGuiStyleVar_ScrollbarSize, mDrawContext.getGlyphWidth() * 2.f);
  ImGui::PushStyleVar (ImGuiStyleVar_ScrollbarRounding, 2.f);
  ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, {0.f,0.f});
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, {0.f,0.f});
  ImGui::BeginChild ("##s", {0.f,0.f}, false,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);
  mDrawContext.update (static_cast<float>(mOptions.mFontSize), isDrawMonoSpaced() && !mFedDocument->hasUtf8());

  keyboard();

  if (isFolded())
    mFoldLines.resize (drawFolded());
  else
    drawLines();

  if (mEdit.mScrollVisible)
    scrollCursorVisible();

  // end childWindow
  ImGui::EndChild();
  ImGui::PopStyleVar (4);
  ImGui::PopStyleColor (6);
  if (isDrawMonoSpaced())
    ImGui::PopFont();
  }
//}}}

// private:
//{{{  get
//{{{
bool cFed::canEditAtCursor() {
// cannot edit readOnly, foldBegin token, foldEnd token

  sPosition position = getCursorPosition();

  // line for cursor
  const cLine& line = mFedDocument->getLine (position.mLineNumber);

  return !isReadOnly() &&
         !(line.mFoldBegin && (position.mColumn < getGlyphsLine (position.mLineNumber).mFirstGlyph)) &&
         !line.mFoldEnd;
  }
//}}}

// text widths
//{{{
float cFed::getWidth (sPosition position) {
// get width in pixels of drawn glyphs up to and including position

  const cLine& line = mFedDocument->getLine (position.mLineNumber);

  float width = 0.f;
  uint32_t toGlyphIndex = mFedDocument->getGlyphIndex (position);
  for (uint32_t glyphIndex = line.mFirstGlyph;
       (glyphIndex < line.getNumGlyphs()) && (glyphIndex < toGlyphIndex); glyphIndex++) {
    if (line.getChar (glyphIndex) == '\t')
      // set width to end of tab
      width = getTabEndPosX (width);
    else
      // add glyphWidth
      width += getGlyphWidth (line, glyphIndex);
    }

  return width;
  }
//}}}
//{{{
float cFed::getGlyphWidth (const cLine& line, uint32_t glyphIndex) {

  uint32_t numCharBytes = line.getNumCharBytes (glyphIndex);
  if (numCharBytes == 1) // simple common case
    return mDrawContext.measureChar (line.getChar (glyphIndex));

  array <char,6> str;
  uint32_t strIndex = 0;
  for (uint32_t i = 0; i < numCharBytes; i++)
    str[strIndex++] = line.getChar (glyphIndex, i);

  return mDrawContext.measureText (str.data(), str.data() + numCharBytes);
  }
//}}}

// lines
//{{{
uint32_t cFed::getNumPageLines() const {
  return static_cast<uint32_t>(floor ((ImGui::GetWindowHeight() - 20.f) / mDrawContext.getLineHeight()));
  }
//}}}

// line
//{{{
uint32_t cFed::getLineNumberFromIndex (uint32_t lineIndex) const {

  if (!isFolded()) // lineNumber is lineIndex
    return lineIndex;

  if (lineIndex < getNumFoldLines())
    return mFoldLines[lineIndex];

  cLog::log (LOGERROR, fmt::format ("getLineNumberFromIndex {} no line for that index", lineIndex));
  return 0;
  }
//}}}
//{{{
uint32_t cFed::getLineIndexFromNumber (uint32_t lineNumber) const {

  if (!isFolded()) // simple case, lineIndex is lineNumber
    return lineNumber;

  if (mFoldLines.empty()) {
    // no lineIndex vector
    cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber {} lineIndex empty", lineNumber));
    return 0;
    }

  // find lineNumber in lineIndex vector
  auto it = find (mFoldLines.begin(), mFoldLines.end(), lineNumber);
  if (it == mFoldLines.end()) {
    // lineNumber notFound, error
    cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber lineNumber:{} not found", lineNumber));
    return 0xFFFFFFFF;
    }

  // lineNUmber found, return lineIndex
  return uint32_t(it - mFoldLines.begin());
  }
//}}}
//{{{
sPosition cFed::getNextLinePosition (const sPosition& position) {

  uint32_t nextLineIndex = getLineIndexFromNumber (position.mLineNumber) + 1;
  uint32_t nextLineNumber = getLineNumberFromIndex (nextLineIndex);
  return {nextLineNumber, getGlyphsLine (nextLineNumber).mFirstGlyph};
  }
//}}}

// column pos
//{{{
float cFed::getTabEndPosX (float xPos) {
// return tabEndPosx of tab containing xPos

  float tabWidthPixels = mFedDocument->getTabSize() * mDrawContext.getGlyphWidth();
  return (1.f + floor ((1.f + xPos) / tabWidthPixels)) * tabWidthPixels;
  }
//}}}
//{{{
uint32_t cFed::getColumnFromPosX (const cLine& line, float posX) {
// return column for posX, using glyphWidths and tabs

  uint32_t column = 0;

  float columnPosX = 0.f;
  for (uint32_t glyphIndex = line.mFirstGlyph; glyphIndex < line.getNumGlyphs(); glyphIndex++)
    if (line.getChar (glyphIndex) == '\t') {
      // tab
      float lastColumnPosX = columnPosX;
      float tabEndPosX = getTabEndPosX (columnPosX);
      float width = tabEndPosX - lastColumnPosX;
      if (columnPosX + (width/2.f) > posX)
        break;

      columnPosX = tabEndPosX;
      column = mFedDocument->getTabColumn (column);
      }

    else {
      float width = getGlyphWidth (line, glyphIndex);
      if (columnPosX + (width/2.f) > posX)
        break;

      columnPosX += width;
      column++;
      }

  //cLog::log (LOGINFO, fmt::format ("getColumn posX:{} firstGlyph:{} column:{} -> result:{}",
  //                                 posX, line.mFirstGlyph, column, line.mFirstGlyph + column));
  return line.mFirstGlyph + column;
  }
//}}}
//}}}
//{{{  set
//{{{
void cFed::setCursorPosition (sPosition position) {
// set cursorPosition, if changed check scrollVisible

  if (position != mEdit.mCursor.mPosition) {
    mEdit.mCursor.mPosition = position;
    mEdit.mScrollVisible = true;
    }
  }
//}}}

//{{{
void cFed::deselect() {

  mEdit.mCursor.mSelect = eSelect::eNormal;
  mEdit.mCursor.mSelectBeginPosition = mEdit.mCursor.mPosition;
  mEdit.mCursor.mSelectEndPosition = mEdit.mCursor.mPosition;
  }
//}}}
//{{{
void cFed::setSelect (eSelect select, sPosition beginPosition, sPosition endPosition) {

  mEdit.mCursor.mSelectBeginPosition = sanitizePosition (beginPosition);
  mEdit.mCursor.mSelectEndPosition = sanitizePosition (endPosition);
  if (mEdit.mCursor.mSelectBeginPosition > mEdit.mCursor.mSelectEndPosition)
    swap (mEdit.mCursor.mSelectBeginPosition, mEdit.mCursor.mSelectEndPosition);

  mEdit.mCursor.mSelect = select;
  switch (select) {
    case eSelect::eNormal:
      //cLog::log (LOGINFO, fmt::format ("setSelect eNormal {}:{} to {}:{}",
      //                                 mEdit.mCursor.mSelectBeginPosition.mLineNumber,
      //                                 mEdit.mCursor.mSelectBeginPosition.mColumn,
      //                                 mEdit.mCursor.mSelectEndPosition.mLineNumber,
      //                                 mEdit.mCursor.mSelectEndPosition.mColumn));
      break;

    case eSelect::eWord:
      mEdit.mCursor.mSelectBeginPosition = findWordBeginPosition (mEdit.mCursor.mSelectBeginPosition);
      mEdit.mCursor.mSelectEndPosition = findWordEndPosition (mEdit.mCursor.mSelectEndPosition);
      //cLog::log (LOGINFO, fmt::format ("setSelect eWord line:{} col:{} to:{}",
      //                                 mEdit.mCursor.mSelectBeginPosition.mLineNumber,
      //                                 mEdit.mCursor.mSelectBeginPosition.mColumn,
      //                                 mEdit.mCursor.mSelectEndPosition.mColumn));
      break;

    case eSelect::eLine: {
      const cLine& line = mFedDocument->getLine (mEdit.mCursor.mSelectBeginPosition.mLineNumber);
      if (!isFolded() || !line.mFoldBegin)
        // select line
        mEdit.mCursor.mSelectEndPosition = {mEdit.mCursor.mSelectEndPosition.mLineNumber + 1, 0};
      else if (!line.mFoldOpen)
        // select fold
        mEdit.mCursor.mSelectEndPosition = {skipFold (mEdit.mCursor.mSelectEndPosition.mLineNumber + 1), 0};

      cLog::log (LOGINFO, fmt::format ("setSelect eLine {}:{} to {}:{}",
                                       mEdit.mCursor.mSelectBeginPosition.mLineNumber,
                                       mEdit.mCursor.mSelectBeginPosition.mColumn,
                                       mEdit.mCursor.mSelectEndPosition.mLineNumber,
                                       mEdit.mCursor.mSelectEndPosition.mColumn));
      break;
      }
    }
  }
//}}}
//}}}
//{{{  move
//{{{
void cFed::moveUp (uint32_t amount) {

  deselect();
  sPosition position = mEdit.mCursor.mPosition;

  if (!isFolded()) {
    //{{{  unfolded simple moveUp
    position.mLineNumber = (amount < position.mLineNumber) ? position.mLineNumber - amount : 0;
    setCursorPosition (position);
    return;
    }
    //}}}

  // folded moveUp
  //cLine& line = getLine (lineNumber);
  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);

  uint32_t moveLineIndex = (amount < lineIndex) ? lineIndex - amount : 0;
  uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
  //cLine& moveLine = getLine (moveLineNumber);
  uint32_t moveColumn = position.mColumn;

  setCursorPosition ({moveLineNumber, moveColumn});
  }
//}}}
//{{{
void cFed::moveDown (uint32_t amount) {

  deselect();
  sPosition position = mEdit.mCursor.mPosition;

  if (!isFolded()) {
    //{{{  unfolded simple moveDown
    position.mLineNumber = min (position.mLineNumber + amount, mFedDocument->getMaxLineNumber());
    setCursorPosition (position);
    return;
    }
    //}}}

  // folded moveDown
  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);

  uint32_t moveLineIndex = min (lineIndex + amount, getMaxFoldLineIndex());
  uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
  uint32_t moveColumn = position.mColumn;

  setCursorPosition ({moveLineNumber, moveColumn});
  }
//}}}

//{{{
void cFed::cursorFlashOn() {
// set cursor flash cycle to on, helps after any move

  mCursorFlashTimePoint = system_clock::now();
  }
//}}}
//{{{
void cFed::scrollCursorVisible() {

  ImGui::SetWindowFocus();
  cursorFlashOn();

  sPosition position = getCursorPosition();

  // up,down scroll
  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
  uint32_t maxLineIndex = isFolded() ? getMaxFoldLineIndex() : mFedDocument->getMaxLineNumber();

  uint32_t minIndex = min (maxLineIndex,
     static_cast<uint32_t>(floor ((ImGui::GetScrollY() + ImGui::GetWindowHeight()) / mDrawContext.getLineHeight())));
  if (lineIndex >= minIndex - 3)
    ImGui::SetScrollY (max (0.f, (lineIndex + 3) * mDrawContext.getLineHeight()) - ImGui::GetWindowHeight());
  else {
    uint32_t maxIndex = static_cast<uint32_t>(floor (ImGui::GetScrollY() / mDrawContext.getLineHeight()));
    if (lineIndex < maxIndex + 2)
      ImGui::SetScrollY (max (0.f, (lineIndex - 2) * mDrawContext.getLineHeight()));
    }

  // left,right scroll
  float textWidth = mDrawContext.mLineNumberWidth + getWidth (position);
  uint32_t minWidth = static_cast<uint32_t>(floor (ImGui::GetScrollX()));
  if (textWidth - (3 * mDrawContext.getGlyphWidth()) < minWidth)
    ImGui::SetScrollX (max (0.f, textWidth - (3 * mDrawContext.getGlyphWidth())));
  else {
    uint32_t maxWidth = static_cast<uint32_t>(ceil (ImGui::GetScrollX() + ImGui::GetWindowWidth()));
    if (textWidth + (3 * mDrawContext.getGlyphWidth()) > maxWidth)
      ImGui::SetScrollX (max (0.f, textWidth + (3 * mDrawContext.getGlyphWidth()) - ImGui::GetWindowWidth()));
    }

  // done
  mEdit.mScrollVisible = false;
  }
//}}}

//{{{
sPosition cFed::advancePosition (sPosition position) {

  if (position.mLineNumber >= mFedDocument->getNumLines())
    return position;

  uint32_t glyphIndex = mFedDocument->getGlyphIndex (position);
  const cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
  if (glyphIndex + 1 < glyphsLine.getNumGlyphs())
    position.mColumn = mFedDocument->getColumn (glyphsLine, glyphIndex + 1);
  else {
    position.mLineNumber++;
    position.mColumn = 0;
    }

  return position;
  }
//}}}
//{{{
sPosition cFed::sanitizePosition (sPosition position) {
// return position to be within glyphs

  if (position.mLineNumber >= mFedDocument->getNumLines()) {
    // past end, position at end of last line
    //cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{} to max:{}",
    //                                 position.mLineNumber, getMaxLineNumber()));
    return sPosition (mFedDocument->getMaxLineNumber(), mFedDocument->getNumColumns (mFedDocument->getLine (mFedDocument->getMaxLineNumber())));
    }

  // ensure column is in range of position.mLineNumber glyphsLine
  cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

  if (position.mColumn < glyphsLine.mFirstGlyph) {
    //cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{} column:{} to min:{}",
    //                                 position.mLineNumber, position.mColumn, glyphsLine.mFirstGlyph));
    position.mColumn = glyphsLine.mFirstGlyph;
    }
  else if (position.mColumn > glyphsLine.getNumGlyphs()) {
    //cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{} column:{} to max:{}",
    //                                 position.mLineNumber, position.mColumn, glyphsLine.getNumGlyphs()));
    position.mColumn = glyphsLine.getNumGlyphs();
    }

  return position;
  }
//}}}

//{{{
sPosition cFed::findWordBeginPosition (sPosition position) {

  const cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
  uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
  if (glyphIndex >= glyphsLine.getNumGlyphs()) // already at lineEnd
    return position;

  // searck back for nonSpace
  while (glyphIndex && isspace (glyphsLine.getChar (glyphIndex)))
    glyphIndex--;

  // search back for nonSpace and of same parse color
  uint8_t color = glyphsLine.getColor (glyphIndex);
  while (glyphIndex > 0) {
    uint8_t ch = glyphsLine.getChar (glyphIndex);
    if (isspace (ch)) {
      glyphIndex++;
      break;
      }
    if (color != glyphsLine.getColor (glyphIndex - 1))
      break;
    glyphIndex--;
    }

  position.mColumn = mFedDocument->getColumn (glyphsLine, glyphIndex);
  return position;
  }
//}}}
//{{{
sPosition cFed::findWordEndPosition (sPosition position) {

  const cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
  uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
  if (glyphIndex >= glyphsLine.getNumGlyphs()) // already at lineEnd
    return position;

  // serach forward for space, or not same parse color
  uint8_t prevColor = glyphsLine.getColor (glyphIndex);
  bool prevSpace = isspace (glyphsLine.getChar (glyphIndex));
  while (glyphIndex < glyphsLine.getNumGlyphs()) {
    uint8_t ch = glyphsLine.getChar (glyphIndex);
    if (glyphsLine.getColor (glyphIndex) != prevColor)
      break;
    if (static_cast<bool>(isspace (ch)) != prevSpace)
      break;
    glyphIndex++;
    }

  position.mColumn = mFedDocument->getColumn (glyphsLine, glyphIndex);
  return position;
  }
//}}}
//}}}
//{{{
sPosition cFed::insertText (const string& text, sPosition position) {
// insert helper - !!! needs more utf8 handling !!!!

  if (text.empty())
    return position;

  bool onFoldBegin = mFedDocument->getLine (position.mLineNumber).mFoldBegin;

  uint32_t glyphIndex = mFedDocument->getGlyphIndex (getGlyphsLine (position.mLineNumber), position.mColumn);
  for (auto it = text.begin(); it < text.end(); ++it) {
    char ch = *it;
    if (ch == '\r') // skip
      break;

    cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
    if (ch == '\n') {
      // insert new line
      if (onFoldBegin) // no lineFeed in allowed in foldBegin, return
        return position;

      // break glyphsLine at glyphIndex, no autoIndent ????
      mFedDocument->breakLine (glyphsLine, glyphIndex, position.mLineNumber+1, 0);

      // point to beginning of new line
      glyphIndex = 0;
      position.mColumn = 0;
      position.mLineNumber++;
      }

    else {
      // insert char
      mFedDocument->insertChar (glyphsLine, glyphIndex, ch);

      // point to next char
      glyphIndex++;
      position.mColumn++; // !!! should convert glyphIndex back to column !!!
      }
    }

  return position;
  }
//}}}
//{{{
void cFed::deleteSelect() {

  mFedDocument->deletePositionRange (mEdit.mCursor.mSelectBeginPosition, mEdit.mCursor.mSelectEndPosition);
  setCursorPosition (mEdit.mCursor.mSelectBeginPosition);
  deselect();
  }
//}}}

// fold
//{{{
void cFed::openFold (uint32_t lineNumber) {

  if (isFolded()) {
    cLine& line = mFedDocument->getLine (lineNumber);
    if (line.mFoldBegin)
      line.mFoldOpen = true;

    // position cursor to lineNumber, first glyph column
    setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
    }
  }
//}}}
//{{{
void cFed::openFoldOnly (uint32_t lineNumber) {

  if (isFolded()) {
    cLine& line = mFedDocument->getLine (lineNumber);
    if (line.mFoldBegin) {
      line.mFoldOpen = true;
      mEdit.mFoldOnly = true;
      mEdit.mFoldOnlyBeginLineNumber = lineNumber;
      }

    // position cursor to lineNumber, first glyph column
    setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
    }
  }
//}}}
//{{{
void cFed::closeFold (uint32_t lineNumber) {

  if (isFolded()) {
    // close fold containing lineNumber
    cLog::log (LOGINFO, fmt::format ("closeFold line:{}", lineNumber));

    cLine& line = mFedDocument->getLine (lineNumber);
    if (line.mFoldBegin) {
      // position cursor to lineNumber, first glyph column
      line.mFoldOpen = false;
      setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
      }

    else {
      // search back for this fold's foldBegin and close it
      // - skip foldEnd foldBegin pairs
      uint32_t skipFoldPairs = 0;
      while (lineNumber > 0) {
        line = mFedDocument->getLine (--lineNumber);
        if (line.mFoldEnd) {
          skipFoldPairs++;
          cLog::log (LOGINFO, fmt::format ("- skip foldEnd:{} {}", lineNumber,skipFoldPairs));
          }
        else if (line.mFoldBegin && skipFoldPairs) {
          skipFoldPairs--;
          cLog::log (LOGINFO, fmt::format (" - skip foldBegin:{} {}", lineNumber, skipFoldPairs));
          }
        else if (line.mFoldBegin && line.mFoldOpen) {
          // position cursor to lineNumber, first column
          line.mFoldOpen = false;
          setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
          break;
          }
        }
      }

    // close down foldOnly
    mEdit.mFoldOnly = false;
    mEdit.mFoldOnlyBeginLineNumber = 0;
    }
  }
//}}}
//{{{
uint32_t cFed::skipFold (uint32_t lineNumber) {
// recursively skip fold lines until matching foldEnd

  while (lineNumber < mFedDocument->getNumLines())
    if (mFedDocument->getLine (lineNumber).mFoldBegin)
      lineNumber = skipFold (lineNumber+1);
    else if (mFedDocument->getLine (lineNumber).mFoldEnd)
      return lineNumber+1;
    else
      lineNumber++;

  // error if you run off end. begin/end mismatch
  cLog::log (LOGERROR, "skipToFoldEnd - unexpected end reached with mathching fold");
  return lineNumber;
  }
//}}}
//{{{
uint32_t cFed::drawFolded() {

  uint32_t lineIndex = 0;

  uint32_t lineNumber = mEdit.mFoldOnly ? mEdit.mFoldOnlyBeginLineNumber : 0;

  //cLog::log (LOGINFO, fmt::format ("drawFolded {}", lineNumber));

  while (lineNumber < mFedDocument->getNumLines()) {
    cLine& line = mFedDocument->getLine (lineNumber);

    if (line.mFoldBegin) {
      // foldBegin
      line.mFirstGlyph = static_cast<uint8_t>(line.mIndent + getLanguage().mFoldBeginToken.size());
      if (line.mFoldOpen)
        // draw foldBegin open fold line
        drawLine (lineNumber++, lineIndex++);
      else {
        // draw foldBegin closed fold line, skip rest
        uint32_t glyphsLineNumber = getGlyphsLineNumber (lineNumber);
        if (glyphsLineNumber != lineNumber)
            mFedDocument->getLine (glyphsLineNumber).mFirstGlyph = static_cast<uint8_t>(line.mIndent);
        drawLine (lineNumber++, lineIndex++);
        lineNumber = skipFold (lineNumber);
        }
      }

    else {
      // draw foldMiddle, foldEnd
      line.mFirstGlyph = 0;
      drawLine (lineNumber++, lineIndex++);
      if (line.mFoldEnd && mEdit.mFoldOnly)
        return lineIndex;
      }
    }

  return lineIndex;
  }
//}}}

// keyboard,mouse
//{{{
void cFed::keyboard() {
  //{{{  numpad codes
  // -------------------------------------------------------------------------------------
  // |    numlock       |        /           |        *             |        -            |
  // |GLFW_KEY_NUM_LOCK | GLFW_KEY_KP_DIVIDE | GLFW_KEY_KP_MULTIPLY | GLFW_KEY_KP_SUBTRACT|
  // |     0x11a        |      0x14b         |      0x14c           |      0x14d          |
  // |------------------------------------------------------------------------------------|
  // |        7         |        8           |        9             |         +           |
  // |  GLFW_KEY_KP_7   |   GLFW_KEY_KP_8    |   GLFW_KEY_KP_9      |  GLFW_KEY_KP_ADD;   |
  // |      0x147       |      0x148         |      0x149           |       0x14e         |
  // | -------------------------------------------------------------|                     |
  // |        4         |        5           |        6             |                     |
  // |  GLFW_KEY_KP_4   |   GLFW_KEY_KP_5    |   GLFW_KEY_KP_6      |                     |
  // |      0x144       |      0x145         |      0x146           |                     |
  // | -----------------------------------------------------------------------------------|
  // |        1         |        2           |        3             |       enter         |
  // |  GLFW_KEY_KP_1   |   GLFW_KEY_KP_2    |   GLFW_KEY_KP_3      |  GLFW_KEY_KP_ENTER  |
  // |      0x141       |      0x142         |      0x143           |       0x14f         |
  // | -------------------------------------------------------------|                     |
  // |        0                              |        .             |                     |
  // |  GLFW_KEY_KP_0                        | GLFW_KEY_KP_DECIMAL  |                     |
  // |      0x140                            |      0x14a           |                     |
  // --------------------------------------------------------------------------------------

  // glfw keycodes, they are platform specific
  // - ImGuiKeys small subset of normal keyboard keys
  // - have I misunderstood something here ?

  //constexpr int kNumpadNumlock = 0x11a;
  constexpr int kNumpad0 = 0x140;
  constexpr int kNumpad1 = 0x141;
  //constexpr int kNumpad2 = 0x142;
  constexpr int kNumpad3 = 0x143;
  //constexpr int kNumpad4 = 0x144;
  //constexpr int kNumpad5 = 0x145;
  //constexpr int kNumpad6 = 0x146;
  constexpr int kNumpad7 = 0x147;
  //constexpr int kNumpad8 = 0x148;
  constexpr int kNumpad9 = 0x149;
  //constexpr int kNumpadDecimal = 0x14a;
  //constexpr int kNumpadDivide = 0x14b;
  //constexpr int kNumpadMultiply = 0x14c;
  //constexpr int kNumpadSubtract = 0x14d;
  //constexpr int kNumpadAdd = 0x14e;
  //constexpr int kNumpadEnter = 0x14f;
  //}}}
  //{{{
  struct sActionKey {
    bool mAlt;
    bool mCtrl;
    bool mShift;
    int mGuiKey;
    bool mWritable;
    function <void()> mActionFunc;
    };
  //}}}
  const vector <sActionKey> kActionKeys = {
  //  alt    ctrl   shift  guiKey             writable function
     {false, false, false, ImGuiKey_Delete,     true,  [this]{deleteIt();}},
     {false, false, false, ImGuiKey_Backspace,  true,  [this]{backspace();}},
     {false, false, false, ImGuiKey_Enter,      true,  [this]{enterKey();}},
     {false, false, false, ImGuiKey_Tab,        true,  [this]{tabKey();}},
     {false, true,  false, ImGuiKey_X,          true,  [this]{cut();}},
     {false, true,  false, ImGuiKey_V,          true,  [this]{paste();}},
     {false, true,  false, ImGuiKey_Z,          true,  [this]{undo();}},
     {false, true,  false, ImGuiKey_Y,          true,  [this]{redo();}},
     // edit without change
     {false, true,  false, ImGuiKey_C,          false, [this]{copy();}},
     {false, true,  false, ImGuiKey_A,          false, [this]{selectAll();}},
     // move
     {false, false, false, ImGuiKey_LeftArrow,  false, [this]{moveLeft();}},
     {false, false, false, ImGuiKey_RightArrow, false, [this]{moveRight();}},
     {false, true,  false, ImGuiKey_RightArrow, false, [this]{moveRightWord();}},
     {false, false, false, ImGuiKey_UpArrow,    false, [this]{moveLineUp();}},
     {false, false, false, ImGuiKey_DownArrow,  false, [this]{moveLineDown();}},
     {false, false, false, ImGuiKey_PageUp,     false, [this]{movePageUp();}},
     {false, false, false, ImGuiKey_PageDown,   false, [this]{movePageDown();}},
     {false, false, false, ImGuiKey_Home,       false, [this]{moveHome();}},
     {false, false, false, ImGuiKey_End,        false, [this]{moveEnd();}},
     // toggle mode
     {false, false, false, ImGuiKey_Insert,     false, [this]{toggleOverWrite();}},
     {false, true,  false, ImGuiKey_Space,      false, [this]{toggleShowFolded();}},
     // numpad
     {false, false, false, kNumpad1,            false, [this]{openFold();}},
     {false, false, false, kNumpad3,            false, [this]{closeFold();}},
     {false, false, false, kNumpad7,            false, [this]{openFoldOnly();}},
     {false, false, false, kNumpad9,            false, [this]{closeFold();}},
     {false, false, false, kNumpad0,            true,  [this]{createFold();}},
  // {false, false, false, kNumpad4,            false, [this]{prevFile();}},
  // {false, false, false, kNumpad6,            false, [this]{nextFile();}},
  // {true,  false, false, kNumpadMulitply,     false, [this]{findDialog();}},
  // {true,  false, false, kNumpadDivide,       false, [this]{replaceDialog();}},
  // {false, false, false, F4                   false, [this]{copy();}},
  // {false, true,  false, F                    false, [this]{findDialog();}},
  // {false, true,  false, S                    false, [this]{saveFile();}},
  // {false, true,  false, A                    false, [this]{saveAllFiles();}},
  // {false, true,  false, Tab                  false, [this]{removeTabs();}},
  // {true,  false, false, N                    false, [this]{gotoDialog();}},
     };

  ImGui::GetIO().WantTextInput = true;
  ImGui::GetIO().WantCaptureKeyboard = false;

  bool altKeyPressed = ImGui::GetIO().KeyAlt;
  bool ctrlKeyPressed = ImGui::GetIO().KeyCtrl;
  bool shiftKeyPressed = ImGui::GetIO().KeyShift;
  for (const auto& actionKey : kActionKeys)
    //{{{  dispatch actionKey
    if ((((actionKey.mGuiKey < 0x100) && ImGui::IsKeyPressed (ImGui::GetKeyIndex (actionKey.mGuiKey))) ||
         ((actionKey.mGuiKey >= 0x100) && ImGui::IsKeyPressed (actionKey.mGuiKey))) &&
        (actionKey.mAlt == altKeyPressed) &&
        (actionKey.mCtrl == ctrlKeyPressed) &&
        (actionKey.mShift == shiftKeyPressed) &&
        (!actionKey.mWritable || (actionKey.mWritable && !(isReadOnly() && canEditAtCursor())))) {

      actionKey.mActionFunc();
      break;
      }
    //}}}

  if (!isReadOnly()) {
    for (int i = 0; i < ImGui::GetIO().InputQueueCharacters.Size; i++) {
      ImWchar ch = ImGui::GetIO().InputQueueCharacters[i];
      if ((ch != 0) && ((ch == '\n') || (ch >= 32)))
        enterCharacter (ch);
      }
    ImGui::GetIO().InputQueueCharacters.resize (0);
    }
  }
//}}}
//{{{
void cFed::mouseSelectLine (uint32_t lineNumber) {

  cursorFlashOn();

  cLog::log (LOGINFO, fmt::format ("mouseSelectLine line:{}", lineNumber));

  mEdit.mCursor.mPosition = {lineNumber, 0};
  mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
  setSelect (eSelect::eLine, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
  }
//}}}
//{{{
void cFed::mouseDragSelectLine (uint32_t lineNumber, float posY) {
// if folded this will use previous displays mFoldLine vector
// - we haven't drawn subsequent lines yet
//   - works because dragging does not change vector
// - otherwise we would have to delay it to after the whole draw

  if (!canEditAtCursor())
    return;

  int numDragLines = static_cast<int>((posY - (mDrawContext.getLineHeight()/2.f)) / mDrawContext.getLineHeight());

  cLog::log (LOGINFO, fmt::format ("mouseDragSelectLine line:{} dragLines:{}", lineNumber, numDragLines));

  if (isFolded()) {
    uint32_t lineIndex = max (0u, min (getMaxFoldLineIndex(), getLineIndexFromNumber (lineNumber) + numDragLines));
    lineNumber = mFoldLines[lineIndex];
    }
  else // simple add to lineNumber
    lineNumber = max (0u, min (mFedDocument->getMaxLineNumber(), lineNumber + numDragLines));

  setCursorPosition ({lineNumber, 0});

  setSelect (eSelect::eLine, mEdit.mDragFirstPosition, getCursorPosition());
  }
//}}}
//{{{
void cFed::mouseSelectText (bool selectWord, uint32_t lineNumber, ImVec2 pos) {

  cursorFlashOn();
  //cLog::log (LOGINFO, fmt::format ("mouseSelectText line:{}", lineNumber));

  mEdit.mCursor.mPosition = {lineNumber, getColumnFromPosX (getGlyphsLine (lineNumber), pos.x)};
  mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
  setSelect (selectWord ? eSelect::eWord : eSelect::eNormal, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
  }
//}}}
//{{{
void cFed::mouseDragSelectText (uint32_t lineNumber, ImVec2 pos) {

  if (!canEditAtCursor())
    return;

  int numDragLines = static_cast<int>((pos.y - (mDrawContext.getLineHeight()/2.f)) / mDrawContext.getLineHeight());
  uint32_t dragLineNumber = max (0u, min (mFedDocument->getMaxLineNumber(), lineNumber + numDragLines));

  //cLog::log (LOGINFO, fmt::format ("mouseDragSelectText {} {}", lineNumber, numDragLines));

  if (isFolded()) {
    // cannot cross fold
    if (lineNumber < dragLineNumber) {
      while (++lineNumber <= dragLineNumber)
        if (mFedDocument->getLine (lineNumber).mFoldBegin || mFedDocument->getLine (lineNumber).mFoldEnd) {
          cLog::log (LOGINFO, fmt::format ("dragSelectText exit 1"));
          return;
          }
      }
    else if (lineNumber > dragLineNumber) {
      while (--lineNumber >= dragLineNumber)
        if (mFedDocument->getLine (lineNumber).mFoldBegin || mFedDocument->getLine (lineNumber).mFoldEnd) {
          cLog::log (LOGINFO, fmt::format ("dragSelectText exit 2"));
          return;
          }
      }
    }

  // select drag range
  uint32_t dragColumn = getColumnFromPosX (getGlyphsLine (dragLineNumber), pos.x);
  setCursorPosition ({dragLineNumber, dragColumn});
  setSelect (eSelect::eNormal, mEdit.mDragFirstPosition, getCursorPosition());
  }
//}}}

// draw
//{{{
float cFed::drawGlyphs (ImVec2 pos, const cLine& line, uint8_t forceColor) {

  if (line.empty())
    return 0.f;

  // initial pos to measure textWidth on return
  float firstPosX = pos.x;

  array <char,256> str;
  uint32_t strIndex = 0;
  uint8_t prevColor = (forceColor == eUndefined) ? line.getColor (line.mFirstGlyph) : forceColor;

  for (uint32_t glyphIndex = line.mFirstGlyph; glyphIndex < line.getNumGlyphs(); glyphIndex++) {
    uint8_t color = (forceColor == eUndefined) ? line.getColor (glyphIndex) : forceColor;
    if ((strIndex > 0) && (strIndex < str.max_size()) &&
        ((color != prevColor) || (line.getChar (glyphIndex) == '\t') || (line.getChar (glyphIndex) == ' '))) {
      // draw colored glyphs, seperated by colorChange,tab,space
      pos.x += mDrawContext.text (pos, prevColor, str.data(), str.data() + strIndex);
      strIndex = 0;
      }

    if (line.getChar (glyphIndex) == '\t') {
      //{{{  tab
      ImVec2 arrowLeftPos {pos.x + 1.f, pos.y + mDrawContext.getFontSize()/2.f};

      // apply tab
      pos.x = getTabEndPosX (pos.x);

      if (isDrawWhiteSpace()) {
        // draw tab arrow
        ImVec2 arrowRightPos {pos.x - 1.f, arrowLeftPos.y};
        mDrawContext.line (arrowLeftPos, arrowRightPos, eTab);

        ImVec2 arrowTopPos {arrowRightPos.x - (mDrawContext.getFontSize()/4.f) - 1.f,
                            arrowRightPos.y - (mDrawContext.getFontSize()/4.f)};
        mDrawContext.line (arrowRightPos, arrowTopPos, eTab);

        ImVec2 arrowBotPos {arrowRightPos.x - (mDrawContext.getFontSize()/4.f) - 1.f,
                            arrowRightPos.y + (mDrawContext.getFontSize()/4.f)};
        mDrawContext.line (arrowRightPos, arrowBotPos, eTab);
        }
      }
      //}}}
    else if (line.getChar (glyphIndex) == ' ') {
      //{{{  space
      if (isDrawWhiteSpace()) {
        // draw circle
        ImVec2 centre {pos.x  + mDrawContext.getGlyphWidth() /2.f, pos.y + mDrawContext.getFontSize()/2.f};
        mDrawContext.circle (centre, 2.f, eWhiteSpace);
        }

      pos.x += mDrawContext.getGlyphWidth();
      }
      //}}}
    else {
      // character
      for (uint32_t i = 0; i < line.getNumCharBytes (glyphIndex); i++)
        str[strIndex++] = line.getChar (glyphIndex, i);
      }

    prevColor = color;
    }

  if (strIndex > 0) // draw any remaining glyphs
    pos.x += mDrawContext.text (pos, prevColor, str.data(), str.data() + strIndex);

  return pos.x - firstPosX;
  }
//}}}
//{{{
void cFed::drawSelect (ImVec2 pos, uint32_t lineNumber, uint32_t glyphsLineNumber) {

  // posBegin
  ImVec2 posBegin = pos;
  if (sPosition (lineNumber, 0) >= mEdit.mCursor.mSelectBeginPosition)
    // from left edge
    posBegin.x = 0.f;
  else
    // from selectBegin column
    posBegin.x += getWidth ({glyphsLineNumber, mEdit.mCursor.mSelectBeginPosition.mColumn});

  // posEnd
  ImVec2 posEnd = pos;
  if (lineNumber < mEdit.mCursor.mSelectEndPosition.mLineNumber)
    // to end of line
    posEnd.x += getWidth ({lineNumber, mFedDocument->getNumColumns (mFedDocument->getLine (lineNumber))});
  else if (mEdit.mCursor.mSelectEndPosition.mColumn)
    // to selectEnd column
    posEnd.x += getWidth ({glyphsLineNumber, mEdit.mCursor.mSelectEndPosition.mColumn});
  else
    // to lefPad + lineNumer
    posEnd.x = mDrawContext.mLeftPad + mDrawContext.mLineNumberWidth;
  posEnd.y += mDrawContext.getLineHeight();

  // draw select rect
  mDrawContext.rect (posBegin, posEnd, eSelectCursor);
  }
//}}}
//{{{
void cFed::drawCursor (ImVec2 pos, uint32_t lineNumber) {

  // line highlight using pos
  if (!hasSelect()) {
    // draw cursor highlight on lineNumber, could be allText or wholeLine
    ImVec2 tlPos {0.f, pos.y};
    ImVec2 brPos {pos.x, pos.y + mDrawContext.getLineHeight()};
    mDrawContext.rect (tlPos, brPos, eCursorLineFill);
    mDrawContext.rectLine (tlPos, brPos, canEditAtCursor() ? eCursorLineEdge : eCursorReadOnly);
    }

  uint32_t column = mEdit.mCursor.mPosition.mColumn;
  sPosition position = {lineNumber, column};

  //  draw flashing cursor
  auto elapsed = system_clock::now() - mCursorFlashTimePoint;
  if (elapsed < 400ms) {
    // draw cursor
    float cursorPosX = getWidth (position);
    float cursorWidth = 2.f;

    cLine& line = mFedDocument->getLine (lineNumber);
    uint32_t glyphIndex = mFedDocument->getGlyphIndex (position);
    if (mOptions.mOverWrite && (glyphIndex < line.getNumGlyphs())) {
      // overwrite
      if (line.getChar (glyphIndex) == '\t')
        // widen tab cursor
        cursorWidth = getTabEndPosX (cursorPosX) - cursorPosX;
      else
        // widen character cursor
        cursorWidth = mDrawContext.measureChar (line.getChar (glyphIndex));
      }

    ImVec2 tlPos {pos.x + cursorPosX - 1.f, pos.y};
    ImVec2 brPos {tlPos.x + cursorWidth, pos.y + mDrawContext.getLineHeight()};
    mDrawContext.rect (tlPos, brPos, canEditAtCursor() ? eCursorPos : eCursorReadOnly);
    return;
    }

  if (elapsed > 800ms) // reset flashTimer
    cursorFlashOn();
  }
//}}}
//{{{
void cFed::drawLine (uint32_t lineNumber, uint32_t lineIndex) {
// draw Line and return incremented lineIndex

  //cLog::log (LOGINFO, fmt::format ("drawLine {}", lineNumber));

  if (isFolded()) {
    //{{{  update mFoldLines vector
    if (lineIndex >= getNumFoldLines())
      mFoldLines.push_back (lineNumber);
    else
      mFoldLines[lineIndex] = lineNumber;
    }
    //}}}

  ImVec2 curPos = ImGui::GetCursorScreenPos();

  // lineNumber
  float leftPadWidth = mDrawContext.mLeftPad;
  cLine& line = mFedDocument->getLine (lineNumber);
  if (isDrawLineNumber()) {
    //{{{  draw lineNumber
    curPos.x += leftPadWidth;

    if (mOptions.mShowLineDebug)
      // debug
      mDrawContext.mLineNumberWidth = mDrawContext.text (curPos, eLineNumber,
        fmt::format ("{:4d}{}{}{}{}{}{}{}{:2d}{:2d}{:3d} ",
          lineNumber,
          line.mCommentSingle ? '/' : ' ',
          line.mCommentBegin  ? '{' : ' ',
          line.mCommentEnd    ? '}' : ' ',
          line.mFoldBegin     ? 'b' : ' ',
          line.mFoldEnd       ? 'e' : ' ',
          line.mFoldOpen      ? 'o' : ' ',
          line.mFoldPressed   ? 'p' : ' ',
          line.mIndent,
          line.mFirstGlyph,
          line.getNumGlyphs()
          ));
    else
      // normal
      mDrawContext.mLineNumberWidth = mDrawContext.smallText (
        curPos, eLineNumber, fmt::format ("{:4d} ", lineNumber+1));

    // add invisibleButton, gobble up leftPad
    ImGui::InvisibleButton (fmt::format ("##l{}", lineNumber).c_str(),
                            {leftPadWidth + mDrawContext.mLineNumberWidth, mDrawContext.getLineHeight()});
    if (ImGui::IsItemActive()) {
      if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
        mouseDragSelectLine (lineNumber, ImGui::GetMousePos().y - curPos.y);
      else if (ImGui::IsMouseClicked (0))
        mouseSelectLine (lineNumber);
      }

    leftPadWidth = 0.f;
    curPos.x += mDrawContext.mLineNumberWidth;

    ImGui::SameLine();
    }
    //}}}
  else
    mDrawContext.mLineNumberWidth = 0.f;

  // text
  ImVec2 glyphsPos = curPos;
  if (isFolded() && line.mFoldBegin) {
    if (line.mFoldOpen) {
      //{{{  draw foldBegin open + glyphs
      // draw foldPrefix
      curPos.x += leftPadWidth;

      // add the indent
      float indentWidth = line.mIndent * mDrawContext.getGlyphWidth();
      curPos.x += indentWidth;

      // draw foldPrefix
      float prefixWidth = mDrawContext.text(curPos, eFoldOpen, getLanguage().mFoldBeginOpen);

      // add foldPrefix invisibleButton, action on press
      ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                              {leftPadWidth + indentWidth + prefixWidth, mDrawContext.getLineHeight()});
      if (ImGui::IsItemActive() && !line.mFoldPressed) {
        // close fold
        line.mFoldPressed = true;
        closeFold (lineNumber);
        }
      if (ImGui::IsItemDeactivated())
        line.mFoldPressed = false;

      curPos.x += prefixWidth;
      glyphsPos.x = curPos.x;

      // add invisibleButton, fills rest of line for easy picking
      ImGui::SameLine();
      ImGui::InvisibleButton (fmt::format("##t{}", lineNumber).c_str(),
                              {ImGui::GetWindowWidth(), mDrawContext.getLineHeight()});
      if (ImGui::IsItemActive()) {
        if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
          mouseDragSelectText (lineNumber,
                               {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
        else if (ImGui::IsMouseClicked (0))
          mouseSelectText (ImGui::IsMouseDoubleClicked (0), lineNumber,
                           {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
        }
      // draw glyphs
      curPos.x += drawGlyphs (glyphsPos, line, eUndefined);
      }
      //}}}
    else {
      //{{{  draw foldBegin closed + glyphs
      // add indent
      curPos.x += leftPadWidth;

      // add the indent
      float indentWidth = line.mIndent * mDrawContext.getGlyphWidth();
      curPos.x += indentWidth;

      // draw foldPrefix
      float prefixWidth = mDrawContext.text (curPos, eFoldClosed, getLanguage().mFoldBeginClosed);

      // add foldPrefix invisibleButton, indent + prefix wide, action on press
      ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                              {leftPadWidth + indentWidth + prefixWidth, mDrawContext.getLineHeight()});
      if (ImGui::IsItemActive() && !line.mFoldPressed) {
        // open fold only
        line.mFoldPressed = true;
        openFoldOnly (lineNumber);
        }
      if (ImGui::IsItemDeactivated())
        line.mFoldPressed = false;

      curPos.x += prefixWidth;
      glyphsPos.x = curPos.x;

      // add invisibleButton, fills rest of line for easy picking
      ImGui::SameLine();
      ImGui::InvisibleButton (fmt::format ("##t{}", lineNumber).c_str(),
                              {ImGui::GetWindowWidth(), mDrawContext.getLineHeight()});
      if (ImGui::IsItemActive()) {
        if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
          mouseDragSelectText (lineNumber,
                               {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
        else if (ImGui::IsMouseClicked (0))
          mouseSelectText (ImGui::IsMouseDoubleClicked (0), lineNumber,
                           {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
        }

      // draw glyphs, from foldComment or seeThru line
      curPos.x += drawGlyphs (glyphsPos, getGlyphsLine (lineNumber), eFoldClosed);
      }
      //}}}
    }
  else if (isFolded() && line.mFoldEnd) {
    //{{{  draw foldEnd
    curPos.x += leftPadWidth;

    // add the indent
    float indentWidth = line.mIndent * mDrawContext.getGlyphWidth();
    curPos.x += indentWidth;

    // draw foldPrefix
    float prefixWidth = mDrawContext.text (curPos, eFoldOpen, getLanguage().mFoldEnd);
    glyphsPos.x = curPos.x;

    // add foldPrefix invisibleButton, only prefix wide, do not want to pick foldEnd line
    ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                            {leftPadWidth + indentWidth + prefixWidth, mDrawContext.getLineHeight()});
    if (ImGui::IsItemActive()) // closeFold at foldEnd, action on press
      closeFold (lineNumber);
    }
    //}}}
  else {
    //{{{  draw glyphs
    curPos.x += leftPadWidth;
    glyphsPos.x = curPos.x;

    // add invisibleButton, fills rest of line for easy picking
    ImGui::InvisibleButton (fmt::format ("##t{}", lineNumber).c_str(),
                            {ImGui::GetWindowWidth(), mDrawContext.getLineHeight()});
    if (ImGui::IsItemActive()) {
      if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
        mouseDragSelectText (lineNumber,
                             {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
      else if (ImGui::IsMouseClicked (0))
        mouseSelectText (ImGui::IsMouseDoubleClicked (0), lineNumber,
                         {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
      }

    // drawGlyphs
    curPos.x += drawGlyphs (glyphsPos, line, eUndefined);
    }
    //}}}

  uint32_t glyphsLineNumber = getGlyphsLineNumber (lineNumber);

  // select
  if ((lineNumber >= mEdit.mCursor.mSelectBeginPosition.mLineNumber) &&
      (lineNumber <= mEdit.mCursor.mSelectEndPosition.mLineNumber))
    drawSelect (glyphsPos, lineNumber, glyphsLineNumber);

  // cursor
  if (lineNumber == mEdit.mCursor.mPosition.mLineNumber)
    drawCursor (glyphsPos, glyphsLineNumber);
  }
//}}}
//{{{
void cFed::drawLines() {
//  draw unfolded with clipper

  // clipper begin
  ImGuiListClipper clipper;
  clipper.Begin (mFedDocument->getNumLines(), mDrawContext.getLineHeight());
  clipper.Step();

  // clipper iterate
  for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++) {
    mFedDocument->getLine (lineNumber).mFirstGlyph = 0;
    drawLine (lineNumber, 0);
    }

  // clipper end
  clipper.Step();
  clipper.End();
  }
//}}}
//}}}

//{{{
class cMemEdit {
public:
  cMemEdit (uint8_t* memData, size_t memSize);
  ~cMemEdit() = default;

  void drawWindow (const std::string& title, size_t baseAddress);
  void drawContents (size_t baseAddress);

  // set
  void setMem (uint8_t* memData, size_t memSize);
  void setAddressHighlight (size_t addressMin, size_t addressMax);

  //{{{  actions
  void moveLeft();
  void moveRight();
  void moveLineUp();
  void moveLineDown();
  void movePageUp();
  void movePageDown();
  void moveHome();
  void moveEnd();
  //}}}

private:
  enum class eDataFormat { eDec, eBin, eHex };
  static const size_t kUndefinedAddress = (size_t)-1;
  //{{{
  class cOptions {
  public:
    bool mReadOnly = false;

    int mColumns = 16;
    int mColumnExtraSpace = 8;

    bool mShowHexII = false;
    bool mHoverHexII = false;

    bool mBigEndian = false;
    bool mHoverEndian = false;

    ImGuiDataType mDataType = ImGuiDataType_U8;
    };
  //}}}
  //{{{
  class cInfo {
  public:
    //{{{
    cInfo (uint8_t* memData, size_t memSize)
      : mMemData(memData), mMemSize(memSize), mBaseAddress(kUndefinedAddress) {}
    //}}}
    //{{{
    void setBaseAddress (size_t baseAddress) {
      mBaseAddress = baseAddress;
      }
    //}}}

    // vars
    uint8_t* mMemData = nullptr;
    size_t mMemSize = kUndefinedAddress;
    size_t mBaseAddress = kUndefinedAddress;
    };
  //}}}
  //{{{
  class cContext {
  public:
    void update (const cOptions& options, const cInfo& info);

    // vars
    float mGlyphWidth = 0;
    float mLineHeight = 0;

    int mAddressDigitsCount = 0;

    float mHexCellWidth = 0;
    float mExtraSpaceWidth = 0;
    float mHexBeginPos = 0;
    float mHexEndPos = 0;

    float mAsciiBeginPos = 0;
    float mAsciiEndPos = 0;

    float mWindowWidth = 0;
    int mNumPageLines = 0;

    int mNumLines = 0;

    ImU32 mTextColor;
    ImU32 mGreyColor;
    ImU32 mHighlightColor;
    ImU32 mFrameBgndColor;
    ImU32 mTextSelectBgndColor;
    };
  //}}}
  //{{{
  class cEdit {
  public:
    //{{{
    void begin (const cOptions& options, const cInfo& info) {

      mDataNext = false;

      if (mDataAddress >= info.mMemSize)
        mDataAddress = kUndefinedAddress;

      if (options.mReadOnly || (mEditAddress >= info.mMemSize))
        mEditAddress = kUndefinedAddress;

      mNextEditAddress = kUndefinedAddress;
      }
    //}}}

    bool mDataNext = false;
    size_t mDataAddress = kUndefinedAddress;

    bool mEditTakeFocus = false;
    size_t mEditAddress = kUndefinedAddress;
    size_t mNextEditAddress = kUndefinedAddress;

    char mDataInputBuf[32] = { 0 };
    char mAddressInputBuf[32] = { 0 };
    size_t mGotoAddress = kUndefinedAddress;

    size_t mHighlightMin = kUndefinedAddress;
    size_t mHighlightMax = kUndefinedAddress;
    };
  //}}}

  //{{{  gets
  bool isCpuBigEndian() const;
  bool isReadOnly() const { return mOptions.mReadOnly; };
  bool isValid (size_t address) { return address != kUndefinedAddress; }

  size_t getDataTypeSize (ImGuiDataType dataType) const;
  std::string getDataTypeDesc (ImGuiDataType dataType) const;
  std::string getDataFormatDesc (eDataFormat dataFormat) const;
  std::string getDataString (size_t address, ImGuiDataType dataType, eDataFormat dataFormat);
  //}}}
  //{{{  sets
  void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }
  void toggleReadOnly() { mOptions.mReadOnly = !mOptions.mReadOnly; }
  //}}}
  //{{{  inits
  void initContext();
  void initEdit();
  //}}}

  void* copyEndian (void* dst, void* src, size_t size);
  void keyboard();

  //{{{  draws
  void drawTop();
  void drawLine (int lineNumber);
  //}}}

  // vars
  bool mOpen = true;  // set false when DrawWindow() closed

  cInfo mInfo;
  cContext mContext;
  cEdit mEdit;
  cOptions mOptions;

  uint64_t mStartTime;
  float mLastClick;
  };
//}}}
//{{{  cMemEdit implementation
//{{{  defines, push warnings
#ifdef _MSC_VER
  #define _PRISizeT   "I"
  #define ImSnprintf  _snprintf
#else
  #define _PRISizeT   "z"
  #define ImSnprintf  snprintf
#endif

#ifdef _MSC_VER
  #pragma warning (push)
  #pragma warning (disable: 4996) // warning C4996: 'sprintf': This function or variable may be unsafe.
#endif
//}}}

constexpr bool kShowAscii = true;
constexpr bool kGreyZeroes = true;
constexpr bool kUpperCaseHex = false;
constexpr int kPageMarginLines = 3;

namespace {
  //{{{  const strings
  const array<string,3> kFormatDescs = { "Dec", "Bin", "Hex" };

  const array<string,10> kTypeDescs = { "Int8", "Uint8", "Int16", "Uint16",
                                        "Int32", "Uint32", "Int64", "Uint64",
                                        "Float", "Double",
                                        };

  const array<size_t,10> kTypeSizes = { sizeof(int8_t), sizeof(uint8_t), sizeof(int16_t), sizeof(uint16_t),
                                        sizeof(int32_t), sizeof (uint32_t), sizeof(int64_t), sizeof(uint64_t),
                                        sizeof(float), sizeof(double),
                                        };

  const char* kFormatByte = kUpperCaseHex ? "%02X" : "%02x";
  const char* kFormatByteSpace = kUpperCaseHex ? "%02X " : "%02x ";

  const char* kFormatData = kUpperCaseHex ? "%0*" _PRISizeT "X" : "%0*" _PRISizeT "x";
  const char* kFormatAddress = kUpperCaseHex ? "%0*" _PRISizeT "X: " : "%0*" _PRISizeT "x: ";

  const char* kChildScrollStr = "##scrolling";
  //}}}
  //{{{
  void* bigEndianFunc (void* dst, void* src, size_t s, bool isLittleEndian) {

     if (isLittleEndian) {
      uint8_t* dstPtr = (uint8_t*)dst;
      uint8_t* srcPtr = (uint8_t*)src + s - 1;
      for (int i = 0, n = (int)s; i < n; ++i)
        memcpy(dstPtr++, srcPtr--, 1);
      return dst;
      }

    else
      return memcpy (dst, src, s);
    }
  //}}}
  //{{{
  void* littleEndianFunc (void* dst, void* src, size_t s, bool isLittleEndian) {

    if (isLittleEndian)
      return memcpy (dst, src, s);

    else {
      uint8_t* dstPtr = (uint8_t*)dst;
      uint8_t* srcPtr = (uint8_t*)src + s - 1;
      for (int i = 0, n = (int)s; i < n; ++i)
        memcpy (dstPtr++, srcPtr--, 1);
      return dst;
      }
    }
  //}}}
  void* (*gEndianFunc)(void*, void*, size_t, bool) = nullptr;
  }

// public:
//{{{
cMemEdit::cMemEdit (uint8_t* memData, size_t memSize) : mInfo(memData,memSize) {

  mStartTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
  mLastClick = 0.f;
  }
//}}}

//{{{
void cMemEdit::drawWindow (const string& title, size_t baseAddress) {
// standalone Memory Editor window

  mOpen = true;

  ImGui::Begin (title.c_str(), &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
  drawContents (baseAddress);
  ImGui::End();
  }
//}}}
//{{{
void cMemEdit::drawContents (size_t baseAddress) {

  mInfo.setBaseAddress (baseAddress);
  mContext.update (mOptions, mInfo); // context of sizes,colours
  mEdit.begin (mOptions, mInfo);

  //ImGui::PushAllowKeyboardFocus (true);
  //ImGui::PopAllowKeyboardFocus();
  keyboard();

  drawTop();

  // child scroll window begin
  ImGui::BeginChild (kChildScrollStr, ImVec2(0,0), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
  ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, ImVec2(0,0));
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

  // clipper begin
  ImGuiListClipper clipper;
  clipper.Begin (mContext.mNumLines, mContext.mLineHeight);
  clipper.Step();

  if (isValid (mEdit.mNextEditAddress)) {
    //{{{  calc minimum scroll to keep mNextEditAddress on screen
    int lineNumber = static_cast<int>(mEdit.mEditAddress / mOptions.mColumns);
    int nextLineNumber = static_cast<int>(mEdit.mNextEditAddress / mOptions.mColumns);

    int lineNumberOffset = nextLineNumber - lineNumber;
    if (lineNumberOffset) {
      // nextEditAddress is on new line
      bool beforeStart = (lineNumberOffset < 0) && (nextLineNumber < (clipper.DisplayStart + kPageMarginLines));
      bool afterEnd = (lineNumberOffset > 0) && (nextLineNumber >= (clipper.DisplayEnd - kPageMarginLines));

      // is minimum scroll neeeded
      if (beforeStart || afterEnd)
        ImGui::SetScrollY (ImGui::GetScrollY() + (lineNumberOffset * mContext.mLineHeight));
      }
    }
    //}}}

  // clipper iterate
  for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++)
    drawLine (lineNumber);

  // clipper end
  clipper.Step();
  clipper.End();

  // child end
  ImGui::PopStyleVar (2);
  ImGui::EndChild();

  // Notify the main window of our ideal child content size
  ImGui::SetCursorPosX (mContext.mWindowWidth);
  if (mEdit.mDataNext && (mEdit.mEditAddress < mInfo.mMemSize)) {
    mEdit.mEditAddress++;
    mEdit.mDataAddress = mEdit.mEditAddress;
    mEdit.mEditTakeFocus = true;
    }
  else if (isValid (mEdit.mNextEditAddress)) {
    mEdit.mEditAddress = mEdit.mNextEditAddress;
    mEdit.mDataAddress = mEdit.mNextEditAddress;
    }
  }
//}}}

//{{{
void cMemEdit::setMem (uint8_t* memData, size_t memSize) {
  mInfo.mMemData = memData;
  mInfo.mMemSize = memSize;
  }
//}}}
//{{{
void cMemEdit::setAddressHighlight (size_t addressMin, size_t addressMax) {

  mEdit.mGotoAddress = addressMin;

  mEdit.mHighlightMin = addressMin;
  mEdit.mHighlightMax = addressMax;
  }
//}}}

//{{{
void cMemEdit::moveLeft() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress > 0) {
      mEdit.mNextEditAddress = mEdit.mEditAddress - 1;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::moveRight() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress < mInfo.mMemSize - 2) {
      mEdit.mNextEditAddress = mEdit.mEditAddress + 1;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::moveLineUp() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress >= (size_t)mOptions.mColumns) {
      mEdit.mNextEditAddress = mEdit.mEditAddress - mOptions.mColumns;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::moveLineDown() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress < mInfo.mMemSize - mOptions.mColumns) {
      mEdit.mNextEditAddress = mEdit.mEditAddress + mOptions.mColumns;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::movePageUp() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress >= (size_t)mOptions.mColumns) {
      // could be better
      mEdit.mNextEditAddress = mEdit.mEditAddress;
      int lines = mContext.mNumPageLines;
      while ((lines-- > 0) && (mEdit.mNextEditAddress >= (size_t)mOptions.mColumns))
        mEdit.mNextEditAddress -= mOptions.mColumns;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::movePageDown() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress < mInfo.mMemSize - mOptions.mColumns) {
      // could be better
      mEdit.mNextEditAddress = mEdit.mEditAddress;
      int lines = mContext.mNumPageLines;
      while ((lines-- > 0)  && (mEdit.mNextEditAddress < (mInfo.mMemSize - mOptions.mColumns)))
        mEdit.mNextEditAddress += mOptions.mColumns;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::moveHome() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress > 0) {
      mEdit.mNextEditAddress = 0;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::moveEnd() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress < (mInfo.mMemSize - 2)) {
      mEdit.mNextEditAddress = mInfo.mMemSize - 1;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}

// private:
//{{{
bool cMemEdit::isCpuBigEndian() const {
// is cpu bigEndian

  uint16_t x = 1;

  char c[2];
  memcpy (c, &x, 2);

  return c[0] != 0;
  }
//}}}
//{{{
size_t cMemEdit::getDataTypeSize (ImGuiDataType dataType) const {
  return kTypeSizes[dataType];
  }
//}}}
//{{{
string cMemEdit::getDataTypeDesc (ImGuiDataType dataType) const {
  return kTypeDescs[(size_t)dataType];
  }
//}}}
//{{{
string cMemEdit::getDataFormatDesc (cMemEdit::eDataFormat dataFormat) const {
  return kFormatDescs[(size_t)dataFormat];
  }
//}}}
//{{{
string cMemEdit::getDataString (size_t address, ImGuiDataType dataType, eDataFormat dataFormat) {
// return pointer to mOutBuf

  array <char,128> outBuf;

  size_t elemSize = getDataTypeSize (dataType);
  size_t size = ((address + elemSize) > mInfo.mMemSize) ? (mInfo.mMemSize - address) : elemSize;

  uint8_t buf[8];
  memcpy (buf, mInfo.mMemData + address, size);

  outBuf[0] = 0;
  if (dataFormat == eDataFormat::eBin) {
    //{{{  eBin to outBuf
    uint8_t binbuf[8];
    copyEndian (binbuf, buf, size);

    int width = (int)size * 8;
    size_t outn = 0;
    int n = width / 8;
    for (int j = n - 1; j >= 0; --j) {
      for (int i = 0; i < 8; ++i)
        outBuf[outn++] = (binbuf[j] & (1 << (7 - i))) ? '1' : '0';
      outBuf[outn++] = ' ';
      }

    outBuf[outn] = 0;
    }
    //}}}
  else {
    // eHex, eDec to outBuf
    switch (dataType) {
      //{{{
      case ImGuiDataType_S8: {

        uint8_t int8 = 0;
        copyEndian (&int8, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%hhd", int8);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%02x", int8 & 0xFF);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U8: {

        uint8_t uint8 = 0;
        copyEndian (&uint8, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%hhu", uint8);
        if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%02x", uint8 & 0XFF);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_S16: {

        int16_t int16 = 0;
        copyEndian (&int16, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%hd", int16);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%04x", int16 & 0xFFFF);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U16: {

        uint16_t uint16 = 0;
        copyEndian (&uint16, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%hu", uint16);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%04x", uint16 & 0xFFFF);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_S32: {

        int32_t int32 = 0;
        copyEndian (&int32, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%d", int32);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%08x", int32);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U32: {
        uint32_t uint32 = 0;
        copyEndian (&uint32, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%u", uint32);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%08x", uint32);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_S64: {

        int64_t int64 = 0;
        copyEndian (&int64, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%lld", (long long)int64);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%016llx", (long long)int64);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U64: {

        uint64_t uint64 = 0;
        copyEndian (&uint64, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%llu", (long long)uint64);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%016llx", (long long)uint64);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_Float: {

        float float32 = 0.f;
        copyEndian (&float32, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%f", float32);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%a", float32);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_Double: {

        double float64 = 0.0;
        copyEndian (&float64, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%f", float64);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (outBuf.data(), outBuf.max_size(), "%a", float64);
        break;
        }
      //}}}
      }
    }

  return string (outBuf.data());
  }
//}}}

// copy
//{{{
void* cMemEdit::copyEndian (void* dst, void* src, size_t size) {

  if (!gEndianFunc)
    gEndianFunc = isCpuBigEndian() ? bigEndianFunc : littleEndianFunc;

  return gEndianFunc (dst, src, size, mOptions.mBigEndian ^ mOptions.mHoverEndian);
  }
//}}}

//{{{
void cMemEdit::keyboard() {

  //{{{
  struct sActionKey {
    bool mAlt;
    bool mCtrl;
    bool mShift;
    int mGuiKey;
    bool mWritable;
    function <void()> mActionFunc;
    };
  //}}}
  const vector <sActionKey> kActionKeys = {
  //  alt    ctrl   shift  guiKey               writable      function
     {false, false, false, ImGuiKey_LeftArrow,  false, [this]{moveLeft();}},
     {false, false, false, ImGuiKey_RightArrow, false, [this]{moveRight();}},
     {false, false, false, ImGuiKey_UpArrow,    false, [this]{moveLineUp();}},
     {false, false, false, ImGuiKey_DownArrow,  false, [this]{moveLineDown();}},
     {false, false, false, ImGuiKey_PageUp,     false, [this]{movePageUp();}},
     {false, false, false, ImGuiKey_PageDown,   false, [this]{movePageDown();}},
     {false, false, false, ImGuiKey_Home,       false, [this]{moveHome();}},
     {false, false, false, ImGuiKey_End,        false, [this]{moveEnd();}},
     };

  //if (ImGui::IsWindowHovered())
  //  ImGui::SetMouseCursor (ImGuiMouseCursor_TextInput);

  ImGuiIO& io = ImGui::GetIO();
  bool shift = io.KeyShift;
  bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;
  //io.WantCaptureKeyboard = true;
  //io.WantTextInput = true;

  for (auto& actionKey : kActionKeys) {
    //{{{  dispatch matched actionKey
    if ((((actionKey.mGuiKey < 0x100) && ImGui::IsKeyPressed (ImGui::GetKeyIndex (actionKey.mGuiKey))) ||
         ((actionKey.mGuiKey >= 0x100) && ImGui::IsKeyPressed (actionKey.mGuiKey))) &&
        (!actionKey.mWritable || (actionKey.mWritable && !isReadOnly())) &&
        (actionKey.mCtrl == ctrl) &&
        (actionKey.mShift == shift) &&
        (actionKey.mAlt == alt)) {

      actionKey.mActionFunc();
      break;
      }
    }
    //}}}
  }
//}}}

// draw
//{{{
void cMemEdit::drawTop() {

  ImGuiStyle& style = ImGui::GetStyle();

  // draw columns button
  ImGui::SetNextItemWidth ((2.f*style.FramePadding.x) + (7.f*mContext.mGlyphWidth));
  ImGui::DragInt ("##column", &mOptions.mColumns, 0.2f, 2, 64, "%d wide");
  if (ImGui::IsItemHovered())
    mOptions.mColumns = max (2, min (64, mOptions.mColumns + static_cast<int>(ImGui::GetIO().MouseWheel)));

  // draw hexII button
  ImGui::SameLine();
  if (toggleButton ("hexII", mOptions.mShowHexII))
    mOptions.mShowHexII = !mOptions.mShowHexII;
  mOptions.mHoverHexII = ImGui::IsItemHovered();

  // draw gotoAddress button
  ImGui::SetNextItemWidth ((2 * style.FramePadding.x) + ((mContext.mAddressDigitsCount + 1) * mContext.mGlyphWidth));
  ImGui::SameLine();
  if (ImGui::InputText ("##address", mEdit.mAddressInputBuf, sizeof(mEdit.mAddressInputBuf),
                         ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
    //{{{  consume input into gotoAddress
    size_t gotoAddress;
    if (sscanf (mEdit.mAddressInputBuf, "%" _PRISizeT "X", &gotoAddress) == 1) {
      mEdit.mGotoAddress = gotoAddress - mInfo.mBaseAddress;
      mEdit.mHighlightMin = mEdit.mHighlightMax = kUndefinedAddress;
      }
    }
    //}}}

  if (isValid (mEdit.mGotoAddress)) {
    //{{{  valid goto address
    if (mEdit.mGotoAddress < mInfo.mMemSize) {
      // use gotoAddress and scroll to it
      ImGui::BeginChild (kChildScrollStr);
      ImGui::SetScrollFromPosY (ImGui::GetCursorStartPos().y +
                                (mEdit.mGotoAddress/mOptions.mColumns) * mContext.mLineHeight);
      ImGui::EndChild();

      mEdit.mDataAddress = mEdit.mGotoAddress;
      mEdit.mEditAddress = mEdit.mGotoAddress;
      mEdit.mEditTakeFocus = true;
      }

    mEdit.mGotoAddress = kUndefinedAddress;
    }
    //}}}

  if (isValid (mEdit.mDataAddress)) {
    // draw dataType combo
    ImGui::SetNextItemWidth ((2*style.FramePadding.x) + (8*mContext.mGlyphWidth) + style.ItemInnerSpacing.x);
    ImGui::SameLine();
    if (ImGui::BeginCombo ("##comboType", getDataTypeDesc (mOptions.mDataType).c_str(), ImGuiComboFlags_HeightLargest)) {
      for (int dataType = 0; dataType < ImGuiDataType_COUNT; dataType++)
        if (ImGui::Selectable (getDataTypeDesc((ImGuiDataType)dataType).c_str(), mOptions.mDataType == dataType))
          mOptions.mDataType = (ImGuiDataType)dataType;
      ImGui::EndCombo();
      }

    // draw endian combo
    if ((int)mOptions.mDataType > 1) {
      ImGui::SameLine();
      if (toggleButton ("big", mOptions.mBigEndian))
        mOptions.mBigEndian = !mOptions.mBigEndian;
      mOptions.mHoverEndian = ImGui::IsItemHovered();
      }

    // draw formats, !! why can't you inc,iterate an enum
    for (eDataFormat dataFormat = eDataFormat::eDec; dataFormat <= eDataFormat::eHex;
         dataFormat = static_cast<eDataFormat>((static_cast<int>(dataFormat)+1))) {
      ImGui::SameLine();
      ImGui::Text ("%s:%s", getDataFormatDesc (dataFormat).c_str(),
                            getDataString (mEdit.mDataAddress, mOptions.mDataType, dataFormat).c_str());
      }
    }
  }
//}}}
//{{{
void cMemEdit::drawLine (int lineNumber) {

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  // draw address
  size_t address = static_cast<size_t>(lineNumber * mOptions.mColumns);
  ImGui::Text (kFormatAddress, mContext.mAddressDigitsCount, mInfo.mBaseAddress + address);

  // draw hex
  for (int column = 0; column < mOptions.mColumns && address < mInfo.mMemSize; column++, address++) {
    float bytePosX = mContext.mHexBeginPos + mContext.mHexCellWidth * column;
    if (mOptions.mColumnExtraSpace > 0)
      bytePosX += (float)(column / mOptions.mColumnExtraSpace) * mContext.mExtraSpaceWidth;

    // next hex value text
    ImGui::SameLine (bytePosX);

    // highlight
    bool isHighlightRange = ((address >= mEdit.mHighlightMin) && (address < mEdit.mHighlightMax));
    bool isHighlightData = (address >= mEdit.mDataAddress) &&
                           (address < (mEdit.mDataAddress + getDataTypeSize (mOptions.mDataType)));
    if (isHighlightRange || isHighlightData) {
      //{{{  draw hex highlight
      ImVec2 pos = ImGui::GetCursorScreenPos();

      float highlightWidth = 2 * mContext.mGlyphWidth;
      bool isNextByteHighlighted = (address+1 < mInfo.mMemSize) &&
                                   ((isValid (mEdit.mHighlightMax) && (address + 1 < mEdit.mHighlightMax)));

      if (isNextByteHighlighted || ((column + 1) == mOptions.mColumns)) {
        highlightWidth = mContext.mHexCellWidth;
        if ((mOptions.mColumnExtraSpace > 0) &&
            (column > 0) && ((column + 1) < mOptions.mColumns) &&
            (((column + 1) % mOptions.mColumnExtraSpace) == 0))
          highlightWidth += mContext.mExtraSpaceWidth;
        }

      ImVec2 endPos = { pos.x + highlightWidth, pos.y + mContext.mLineHeight};
      draw_list->AddRectFilled (pos, endPos, mContext.mHighlightColor);
      }
      //}}}
    if (mEdit.mEditAddress == address) {
      //{{{  display text input on current byte
      bool dataWrite = false;

      ImGui::PushID ((void*)address);
      if (mEdit.mEditTakeFocus) {
        ImGui::SetKeyboardFocusHere();
        ImGui::CaptureKeyboardFromApp (true);
        sprintf (mEdit.mAddressInputBuf, kFormatData, mContext.mAddressDigitsCount, mInfo.mBaseAddress + address);
        sprintf (mEdit.mDataInputBuf, kFormatByte, mInfo.mMemData[address]);
        }

      //{{{
      struct sUserData {
        char mCurrentBufOverwrite[3];  // Input
        int mCursorPos;                // Output

        static int callback (ImGuiInputTextCallbackData* inputTextCallbackData) {
        // FIXME: We should have a way to retrieve the text edit cursor position more easily in the API
        //  this is rather tedious. This is such a ugly mess we may be better off not using InputText() at all here.

          sUserData* userData = (sUserData*)inputTextCallbackData->UserData;
          if (!inputTextCallbackData->HasSelection())
            userData->mCursorPos = inputTextCallbackData->CursorPos;

          if ((inputTextCallbackData->SelectionStart == 0) &&
              (inputTextCallbackData->SelectionEnd == inputTextCallbackData->BufTextLen)) {
            // When not Edit a byte, always rewrite its content
            // - this is a bit tricky, since InputText technically "owns"
            //   the master copy of the buffer we edit it in there
            inputTextCallbackData->DeleteChars (0, inputTextCallbackData->BufTextLen);
            inputTextCallbackData->InsertChars (0, userData->mCurrentBufOverwrite);
            inputTextCallbackData->SelectionStart = 0;
            inputTextCallbackData->SelectionEnd = 2;
            inputTextCallbackData->CursorPos = 0;
            }
          return 0;
          }
        };
      //}}}
      sUserData userData;
      userData.mCursorPos = -1;

      sprintf (userData.mCurrentBufOverwrite, kFormatByte, mInfo.mMemData[address]);
      ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal |
                                  ImGuiInputTextFlags_EnterReturnsTrue |
                                  ImGuiInputTextFlags_AutoSelectAll |
                                  ImGuiInputTextFlags_NoHorizontalScroll |
                                  ImGuiInputTextFlags_CallbackAlways;
      flags |= ImGuiInputTextFlags_AlwaysOverwrite;

      ImGui::SetNextItemWidth (mContext.mGlyphWidth * 2);

      if (ImGui::InputText ("##data", mEdit.mDataInputBuf, sizeof(mEdit.mDataInputBuf), flags, sUserData::callback, &userData))
        dataWrite = mEdit.mDataNext = true;
      else if (!mEdit.mEditTakeFocus && !ImGui::IsItemActive())
        mEdit.mEditAddress = mEdit.mNextEditAddress = kUndefinedAddress;

      mEdit.mEditTakeFocus = false;

      if (userData.mCursorPos >= 2)
        dataWrite = mEdit.mDataNext = true;
      if (isValid (mEdit.mNextEditAddress))
        dataWrite = mEdit.mDataNext = false;

      unsigned int dataInputValue = 0;
      if (dataWrite && sscanf(mEdit.mDataInputBuf, "%X", &dataInputValue) == 1)
        mInfo.mMemData[address] = (ImU8)dataInputValue;

      ImGui::PopID();
      }
      //}}}
    else {
      //{{{  display text
      // NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click on.
      uint8_t value = mInfo.mMemData[address];

      if (mOptions.mShowHexII || mOptions.mHoverHexII) {
        if ((value >= 32) && (value < 128))
          ImGui::Text (".%c ", value);
        else if (kGreyZeroes && (value == 0xFF))
          ImGui::TextDisabled ("## ");
        else if (value == 0x00)
          ImGui::Text ("   ");
        else
          ImGui::Text (kFormatByteSpace, value);
        }
      else if (kGreyZeroes && (value == 0))
        ImGui::TextDisabled ("00 ");
      else
        ImGui::Text (kFormatByteSpace, value);

      if (!mOptions.mReadOnly &&
          ImGui::IsItemHovered() && ImGui::IsMouseClicked (0)) {
        mEdit.mEditTakeFocus = true;
        mEdit.mNextEditAddress = address;
        }
      }
      //}}}
    }

  if (kShowAscii) {
    //{{{  draw righthand ASCII
    ImGui::SameLine (mContext.mAsciiBeginPos);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    address = lineNumber * mOptions.mColumns;

    // invisibleButton over righthand ascii text for mouse hits
    ImGui::PushID (lineNumber);
    if (ImGui::InvisibleButton ("ascii", ImVec2 (mContext.mAsciiEndPos - mContext.mAsciiBeginPos, mContext.mLineHeight))) {
      mEdit.mDataAddress = address + (size_t)((ImGui::GetIO().MousePos.x - pos.x) / mContext.mGlyphWidth);
      mEdit.mEditAddress = mEdit.mDataAddress;
      mEdit.mEditTakeFocus = true;
      }
    ImGui::PopID();

    for (int column = 0; (column < mOptions.mColumns) && (address < mInfo.mMemSize); column++, address++) {
      if (address == mEdit.mEditAddress) {
        ImVec2 endPos = { pos.x + mContext.mGlyphWidth, pos.y + mContext.mLineHeight };
        draw_list->AddRectFilled (pos, endPos, mContext.mFrameBgndColor);
        draw_list->AddRectFilled (pos, endPos, mContext.mTextSelectBgndColor);
        }

      uint8_t value = mInfo.mMemData[address];
      char displayCh = ((value < 32) || (value >= 128)) ? '.' : value;
      ImU32 color = (!kGreyZeroes || (value == displayCh)) ? mContext.mTextColor : mContext.mGreyColor;
      draw_list->AddText (pos, color, &displayCh, &displayCh + 1);

      pos.x += mContext.mGlyphWidth;
      }
    }
    //}}}
  }
//}}}

// cMemEdit::cContext
//{{{
void cMemEdit::cContext::update (const cOptions& options, const cInfo& info) {

  // address box size
  mAddressDigitsCount = 0;
  for (size_t n = info.mBaseAddress + info.mMemSize - 1; n > 0; n >>= 4)
    mAddressDigitsCount++;

  // char size
  mGlyphWidth = ImGui::CalcTextSize (" ").x; // assume monoSpace font
  mLineHeight = ImGui::GetTextLineHeight();
  mHexCellWidth = (float)(int)(mGlyphWidth * 2.5f);
  mExtraSpaceWidth = (float)(int)(mHexCellWidth * 0.25f);

  // hex begin,end
  mHexBeginPos = (mAddressDigitsCount + 2) * mGlyphWidth;
  mHexEndPos = mHexBeginPos + (mHexCellWidth * options.mColumns);
  mAsciiBeginPos = mAsciiEndPos = mHexEndPos;

  // ascii begin,end
  mAsciiBeginPos = mHexEndPos + mGlyphWidth * 1;
  if (options.mColumnExtraSpace > 0)
    mAsciiBeginPos += (float)((options.mColumns + options.mColumnExtraSpace - 1) /
      options.mColumnExtraSpace) * mExtraSpaceWidth;
  mAsciiEndPos = mAsciiBeginPos + options.mColumns * mGlyphWidth;

  // windowWidth
  ImGuiStyle& style = ImGui::GetStyle();
  mWindowWidth = (2.f*style.WindowPadding.x) + mAsciiEndPos + style.ScrollbarSize + mGlyphWidth;

  // page up,down inc
  mNumPageLines = static_cast<int>(ImGui::GetWindowHeight() / ImGui::GetTextLineHeight()) - kPageMarginLines;

  // numLines of memory range
  mNumLines = static_cast<int>((info.mMemSize + (options.mColumns-1)) / options.mColumns);

  // colors
  mTextColor = ImGui::GetColorU32 (ImGuiCol_Text);
  mGreyColor = ImGui::GetColorU32 (ImGuiCol_TextDisabled);
  mHighlightColor = IM_COL32 (255, 255, 255, 50);  // highlight background color
  mFrameBgndColor = ImGui::GetColorU32 (ImGuiCol_FrameBg);
  mTextSelectBgndColor = ImGui::GetColorU32 (ImGuiCol_TextSelectedBg);
  }
//}}}

//{{{  undefines, pop warnings
#undef _PRISizeT
#undef ImSnprintf

#ifdef _MSC_VER
  #pragma warning (pop)
#endif
//}}}
//}}}

class cFedUI : public cUI {
public:
  cFedUI (const string& name) : cUI(name) {}
  //{{{
  virtual ~cFedUI() {

    delete mFileView;
    delete mMemEdit;
    }
  //}}}

  void addToDrawList (cApp& app) final {

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

    if (false) {
      //{{{  fileView memEdit
      mFileView = new cFileView ("C:/projects/paint/build/Release/fed.exe");

      if (!mMemEdit)
        mMemEdit = new cMemEdit ((uint8_t*)(this), 0x10000);

      ImGui::PushFont (app.getMonoFont());
      mMemEdit->setMem (mFileView->getReadPtr(), mFileView->getReadBytesLeft());
      mMemEdit->drawWindow ("Memory Editor", 0);
      ImGui::PopFont();

      return;
      }
      //}}}

    if ((dynamic_cast<cFedApp&>(app)).getMemEdit()) {
      //{{{  memEdit
      if (!mMemEdit)
        mMemEdit = new cMemEdit ((uint8_t*)(this), 0x10000);

      ImGui::PushFont (app.getMonoFont());
      mMemEdit->setMem ((uint8_t*)this, 0x80000);
      mMemEdit->drawWindow ("MemEdit", 0);
      ImGui::PopFont();

      return;
      }
      //}}}
    else
      mFed.draw (app);
    }

private:
  cFed mFed;
  cFileView* mFileView = nullptr;
  cMemEdit* mMemEdit = nullptr;

  //{{{
  static cUI* create (const string& className) {
    return new cFedUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("fed", &create);
  };
