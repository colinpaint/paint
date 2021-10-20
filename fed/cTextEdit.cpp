// cTextEdit.cpp - folding text editor, !!! undo isn't utf8 abled !!!
//{{{  includes
#include "cTextEdit.h"

#include <cmath>
#include <algorithm>
#include <functional>

#include <array>

#include <fstream>
#include <filesystem>

#include "../platform/cPlatform.h"
#include "../imgui/myImgui.h"

#include "../ui/cApp.h"
#include "../utils/cLog.h"

using namespace std;
using namespace chrono;
//}}}

// cTextEdit
//{{{
cTextEdit::cTextEdit (cDocument& document) : mDoc(document) {

  mDoc.mLines.push_back (cLine());

  // push any clipboardText to pasteStack
  const char* clipText = ImGui::GetClipboardText();
  if (clipText && (strlen (clipText)) > 0)
    mEdit.pushPasteText (clipText);

  cursorFlashOn();
  }
//}}}

//actions
//{{{
void cTextEdit::toggleShowFolded() {

  mOptions.mShowFolded = !mOptions.mShowFolded;
  mEdit.mScrollVisible = true;
  }
//}}}

//{{{  move
//{{{
void cTextEdit::moveLeft() {

  setDeselect();

  sPosition position = mEdit.mCursor.mPosition;

  // line
  cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

  // column
  uint32_t glyphIndex = mDoc.getGlyphIndex (glyphsLine, position.mColumn);
  if (glyphIndex > glyphsLine.mFirstGlyph)
    // move to prevColumn on sameLine
    setCursorPosition ({position.mLineNumber, mDoc.getColumn (glyphsLine, glyphIndex - 1)});

  else if (position.mLineNumber > 0) {
    // at begining of line, move to end of prevLine
    uint32_t moveLineIndex = getLineIndexFromNumber (position.mLineNumber) - 1;
    uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
    cLine& moveGlyphsLine = getGlyphsLine (moveLineNumber);
    setCursorPosition ({moveLineNumber, mDoc.getColumn (moveGlyphsLine, moveGlyphsLine.getNumGlyphs())});
    }
  }
//}}}
//{{{
void cTextEdit::moveRight() {

  setDeselect();

  sPosition position = mEdit.mCursor.mPosition;

  // line
  cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

  // column
  uint32_t glyphIndex = mDoc.getGlyphIndex (glyphsLine, position.mColumn);
  if (glyphIndex < glyphsLine.getNumGlyphs())
    // move to nextColumm on sameLine
    setCursorPosition ({position.mLineNumber, mDoc.getColumn (glyphsLine, glyphIndex + 1)});

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
void cTextEdit::moveRightWord() {

  setDeselect();

  sPosition position = mEdit.mCursor.mPosition;

  // line
  uint32_t glyphsLineNumber = getGlyphsLineNumber (position.mLineNumber);
  cLine& glyphsLine = mDoc.getLine (glyphsLineNumber);

  // find nextWord
  sPosition movePosition;
  uint32_t glyphIndex = mDoc.getGlyphIndex (glyphsLine, position.mColumn);
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
        setCursorPosition ({position.mLineNumber, mDoc.getColumn (glyphsLine, glyphIndex)});
        return;
        }
      if (!isWord)
        skip = false;
      glyphIndex++;
      }

    setCursorPosition ({position.mLineNumber, mDoc.getColumn (glyphsLine, glyphIndex)});
    }
  else {
    // !!!! implement inc to next line !!!!!
    }
  }
//}}}
//}}}
//{{{
void cTextEdit::selectAll() {
  setSelect (eSelect::eNormal, {0,0}, { mDoc.getNumLines(),0});
  }
//}}}
//{{{
void cTextEdit::copy() {

  if (!canEditAtCursor())
    return;

  if (hasSelect()) {
    // push selectedText to pasteStack
    mEdit.pushPasteText (getSelectText());
    setDeselect();
    }

  else {
    // push currentLine text to pasteStack
    sPosition position {getCursorPosition().mLineNumber, 0};
    sPosition nextLinePosition = getNextLinePosition (position);
    string text = mDoc.getText (position, nextLinePosition);;
    mEdit.pushPasteText (text);

    // moveLineDown for any multiple copy
    moveLineDown();
    }
  }
//}}}
//{{{
void cTextEdit::cut() {

  if (hasSelect()) {
    // push selectedText to pasteStack
    string text = getSelectText();
    mEdit.pushPasteText (text);

    // cut selected range
    cUndo undo;
    undo.mBeforeCursor = mEdit.mCursor;
    undo.mDeleteText = text;
    undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
    undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;

    // delete selected text
    deleteSelect();

    undo.mAfterCursor = mEdit.mCursor;
    mEdit.addUndo (undo);
    }

  else {
    // push currentLine text to pasteStack
    sPosition position {getCursorPosition().mLineNumber,0};
    sPosition nextLinePosition = getNextLinePosition (position);
    string text = mDoc.getText (position, nextLinePosition);
    mEdit.pushPasteText (text);

    cUndo undo;
    undo.mBeforeCursor = mEdit.mCursor;
    undo.mDeleteText = text;
    undo.mDeleteBeginPosition = position;
    undo.mDeleteEndPosition = nextLinePosition;

    // delete currentLine, handling folds
    mDoc.deleteLineRange (position.mLineNumber, nextLinePosition.mLineNumber);

    undo.mAfterCursor = mEdit.mCursor;
    mEdit.addUndo (undo);
    }
  }
//}}}
//{{{
void cTextEdit::paste() {

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
void cTextEdit::deleteIt() {

  cUndo undo;
  undo.mBeforeCursor = mEdit.mCursor;

  if (hasSelect()) {
    undo.mDeleteText = getSelectText();
    undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
    undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
    deleteSelect();
    }

  else {
    sPosition position = getCursorPosition();
    setCursorPosition (position);

    cLine& line = mDoc.getLine (position.mLineNumber);
    if (position.mColumn == mDoc.getNumColumns (line)) {
      if (position.mLineNumber == mDoc.getMaxLineNumber())
        return;

      undo.mDeleteText = '\n';
      undo.mDeleteBeginPosition = getCursorPosition();
      undo.mDeleteEndPosition = advancePosition (undo.mDeleteBeginPosition);

      // insert nextLine at end of line
      line.insertLineAtEnd (mDoc.getLine (position.mLineNumber+1));
      mDoc.parse (line);

      // delete nextLine
      mDoc.deleteLine (position.mLineNumber + 1);
      }

    else {
      undo.mDeleteBeginPosition = undo.mDeleteEndPosition = getCursorPosition();
      undo.mDeleteEndPosition.mColumn++;
      undo.mDeleteText = mDoc.getText (undo.mDeleteBeginPosition, undo.mDeleteEndPosition);

      uint32_t glyphIndex = mDoc.getGlyphIndex (position);
      line.erase (glyphIndex);
      mDoc.parse (line);
      }
    mDoc.edited();
    }

  undo.mAfterCursor = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}
//{{{
void cTextEdit::backspace() {

  cUndo undo;
  undo.mBeforeCursor = mEdit.mCursor;

  if (hasSelect()) {
    // delete select
    undo.mDeleteText = getSelectText();
    undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
    undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
    deleteSelect();
    }

  else {
    sPosition position = getCursorPosition();
    cLine& line = mDoc.getLine (position.mLineNumber);

    if (position.mColumn <= line.mFirstGlyph) {
      // at beginning of line
      if (isFolded() && line.mFoldBegin) // don't backspace to prevLine at foldBegin
        return;
      if (!position.mLineNumber) // already on firstLine, nowhere to go
        return;

      // prevLine
      cLine& prevLine = mDoc.getLine (position.mLineNumber - 1);
      uint32_t prevSize = mDoc.getNumColumns (prevLine);

      undo.mDeleteText = '\n';
      undo.mDeleteBeginPosition = {position.mLineNumber - 1, prevSize};
      undo.mDeleteEndPosition = advancePosition (undo.mDeleteBeginPosition);

      // append this line to prevLine
      prevLine.insertLineAtEnd (line);
      mDoc.deleteLine (mEdit.mCursor.mPosition.mLineNumber);
      mDoc.parse (prevLine);

      // position to end of prevLine
      setCursorPosition ({position.mLineNumber - 1, mDoc.getNumColumns (prevLine)});
      }

    else {
      // at middle of line
      cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
      uint32_t glyphIndex = mDoc.getGlyphIndex (glyphsLine, position.mColumn);

      // set undo to prevChar
      undo.mDeleteEndPosition = getCursorPosition();
      undo.mDeleteBeginPosition = getCursorPosition();
      undo.mDeleteBeginPosition.mColumn--;
      undo.mDeleteText += glyphsLine.getChar (glyphIndex - 1);

      // delete prevGlyph
      glyphsLine.erase (glyphIndex - 1);
      mDoc.parse (glyphsLine);

      // position to prevGlyph
      setCursorPosition ({position.mLineNumber, position.mColumn - 1});
      }
    mDoc.edited();
    }

  undo.mAfterCursor = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}
//{{{
void cTextEdit::enterCharacter (ImWchar ch) {
// !!!! more utf8 handling !!!

  if (!canEditAtCursor())
    return;

  cUndo undo;
  undo.mBeforeCursor = mEdit.mCursor;

  if (hasSelect()) {
    if ((ch == '\t') &&
        (mEdit.mCursor.mSelectBeginPosition.mLineNumber != mEdit.mCursor.mSelectEndPosition.mLineNumber)) {
      //{{{  tab select lines
      sPosition selectBeginPosition = mEdit.mCursor.mSelectBeginPosition;
      sPosition selectEndPosition = mEdit.mCursor.mSelectEndPosition;
      sPosition originalEndPosition = selectEndPosition;

      if (selectBeginPosition > selectEndPosition)
        swap (selectBeginPosition, selectEndPosition);

      selectBeginPosition.mColumn = 0;
      if ((selectEndPosition.mColumn == 0) && (selectEndPosition.mLineNumber > 0))
        --selectEndPosition.mLineNumber;
      if (selectEndPosition.mLineNumber >= mDoc.getNumLines())
        selectEndPosition.mLineNumber = mDoc.getMaxLineNumber();
      selectEndPosition.mColumn = mDoc.getNumColumns (mDoc.getLine (selectEndPosition.mLineNumber));

      undo.mDeleteBeginPosition = selectBeginPosition;
      undo.mDeleteEndPosition = selectEndPosition;
      undo.mDeleteText = mDoc.getText (selectBeginPosition, selectEndPosition);

      bool modified = false;
      for (uint32_t lineNumber = selectBeginPosition.mLineNumber;
           lineNumber <= selectEndPosition.mLineNumber; lineNumber++) {
          mDoc.getLine (lineNumber).insert (0, cGlyph ('\t', eTab));
        modified = true;
        }

      if (modified) {
        //  not sure what this does yet
        cLine& selectBeginLine = mDoc.getLine (selectBeginPosition.mLineNumber);
        selectBeginPosition = { selectBeginPosition.mLineNumber, mDoc.getColumn (selectBeginLine, 0)};
        sPosition rangeEnd;
        if (originalEndPosition.mColumn != 0) {
          selectEndPosition = {selectEndPosition.mLineNumber, mDoc.getNumColumns (mDoc.getLine (selectEndPosition.mLineNumber))};
          rangeEnd = selectEndPosition;
          undo.mAddText = mDoc.getText (selectBeginPosition, selectEndPosition);
          }
        else {
          selectEndPosition = {originalEndPosition.mLineNumber, 0};
          rangeEnd = {selectEndPosition.mLineNumber - 1, mDoc.getNumColumns (mDoc.getLine (selectEndPosition.mLineNumber - 1))};
          undo.mAddText = mDoc.getText (selectBeginPosition, rangeEnd);
          }

        undo.mAddBeginPosition = selectBeginPosition;
        undo.mAddEndPosition = rangeEnd;
        undo.mAfterCursor = mEdit.mCursor;

        mEdit.mCursor.mSelectBeginPosition = selectBeginPosition;
        mEdit.mCursor.mSelectEndPosition = selectEndPosition;
        mEdit.addUndo (undo);

        mDoc.edited();
        mEdit.mScrollVisible = true;
        }

      return;
      }
      //}}}
    else {
      //{{{  delete select lines
      undo.mDeleteText = getSelectText();
      undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
      undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;

      deleteSelect();
      }
      //}}}
    }

  sPosition position = getCursorPosition();
  undo.mAddBeginPosition = position;

  cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
  uint32_t glyphIndex = mDoc.getGlyphIndex (glyphsLine, position.mColumn);

  if (ch == '\n') {
    //{{{  enter lineFeed
    if (isFolded() && mDoc.getLine (position.mLineNumber).mFoldBegin) // noe newLine in folded foldBegin
      return;

    // insert newLine
    cLine& newLine = *mDoc.mLines.insert (mDoc.mLines.begin() + position.mLineNumber + 1, cLine());

    if (getLanguage().mAutoIndentation)
      for (uint32_t indent = 0;
           (indent < glyphsLine.getNumGlyphs()) &&
           isascii (glyphsLine.getChar (indent)) && isblank (glyphsLine.getChar (indent)); indent++)
        newLine.pushBack (glyphsLine.getGlyph (indent));

    uint32_t indentSize = newLine.getNumGlyphs();

    // insert indent and rest of old line
    newLine.insertRestOfLineAtEnd (glyphsLine, glyphIndex);
    mDoc.parse (newLine);

    // erase rest of old line
    glyphsLine.erase (glyphIndex, glyphsLine.getNumGlyphs());
    mDoc.parse (glyphsLine);

    // set cursor
    setCursorPosition ({position.mLineNumber+1, mDoc.getColumn (newLine, indentSize)});
    undo.mAddText = (char)ch;
    }
    //}}}
  else {
    // enter char
    if (mOptions.mOverWrite && (glyphIndex < glyphsLine.getNumGlyphs())) {
      // overwrite, delete char
      undo.mDeleteBeginPosition = mEdit.mCursor.mPosition;
      undo.mDeleteEndPosition = {position.mLineNumber, mDoc.getColumn (glyphsLine, glyphIndex+1)};
      undo.mDeleteText += glyphsLine.getChar (glyphIndex);
      glyphsLine.erase (glyphIndex);
      }

    // insert newChar
    glyphsLine.insert (glyphIndex, cGlyph (ch, eText));
    mDoc.parse (glyphsLine);

    // undo.mAdd = utf8buf.data(); // utf8 handling needed
    undo.mAddText = static_cast<char>(ch);
    setCursorPosition ({position.mLineNumber, mDoc.getColumn (glyphsLine, glyphIndex + 1)});
    }
  mDoc.edited();

  undo.mAddEndPosition = getCursorPosition();
  undo.mAfterCursor = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}
//{{{  fold
//{{{
void cTextEdit::createFold() {

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
//}}}
//{{{  undo
//{{{
void cTextEdit::undo (uint32_t steps) {

  if (hasUndo()) {
    mEdit.undo (this, steps);
    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::redo (uint32_t steps) {

  if (hasRedo()) {
    mEdit.redo (this, steps);
    scrollCursorVisible();
    }
  }
//}}}
//}}}

// draws
//{{{
void cTextEdit::drawWindow (const string& title, cApp& app) {
// standalone textEdit window

  bool open = true;
  ImGui::Begin (title.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

  drawContents (app);
  ImGui::End();
  }
//}}}
//{{{
void cTextEdit::drawContents (cApp& app) {
// main ui io,draw

  // check for delayed all document parse
  mDoc.parseAll();

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

  if (mDoc.getHasFolds()) {
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
  if (mDoc.getEdited()) {
    ImGui::SameLine();
    if (ImGui::Button ("save"))
      mDoc.save();
    }
  //}}}

  //{{{  info
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:{} {}", getCursorPosition().mColumn+1, getCursorPosition().mLineNumber+1,
      mDoc.getNumLines(), getLanguage().mName).c_str());
  //}}}
  //{{{  vertice debug
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}",
               ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());
  //}}}
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
  mDrawContext.update (static_cast<float>(mOptions.mFontSize), isDrawMonoSpaced() && !mDoc.mHasUtf8);

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
bool cTextEdit::canEditAtCursor() {
// cannot edit readOnly, foldBegin token, foldEnd token

  sPosition position = getCursorPosition();

  // line for cursor
  const cLine& line = mDoc.getLine (position.mLineNumber);

  return !isReadOnly() &&
         !(line.mFoldBegin && (position.mColumn < getGlyphsLine (position.mLineNumber).mFirstGlyph)) &&
         !line.mFoldEnd;
  }
//}}}

// text widths
//{{{
float cTextEdit::getWidth (sPosition position) {
// get width in pixels of drawn glyphs up to and including position

  const cLine& line = mDoc.getLine (position.mLineNumber);

  float width = 0.f;
  uint32_t toGlyphIndex = mDoc.getGlyphIndex (position);
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
float cTextEdit::getGlyphWidth (const cLine& line, uint32_t glyphIndex) {

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
uint32_t cTextEdit::getNumPageLines() const {
  return static_cast<uint32_t>(floor ((ImGui::GetWindowHeight() - 20.f) / mDrawContext.getLineHeight()));
  }
//}}}

// line
//{{{
uint32_t cTextEdit::getLineNumberFromIndex (uint32_t lineIndex) const {

  if (!isFolded()) // lineNumber is lineIndex
    return lineIndex;

  if (lineIndex < getNumFoldLines())
    return mFoldLines[lineIndex];

  cLog::log (LOGERROR, fmt::format ("getLineNumberFromIndex {} no line for that index", lineIndex));
  return 0;
  }
//}}}
//{{{
uint32_t cTextEdit::getLineIndexFromNumber (uint32_t lineNumber) const {

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
sPosition cTextEdit::getNextLinePosition (const sPosition& position) {

  uint32_t nextLineIndex = getLineIndexFromNumber (position.mLineNumber) + 1;
  uint32_t nextLineNumber = getLineNumberFromIndex (nextLineIndex);
  return {nextLineNumber, getGlyphsLine (nextLineNumber).mFirstGlyph};
  }
//}}}

// column pos
//{{{
float cTextEdit::getTabEndPosX (float xPos) {
// return tabEndPosx of tab containing xPos

  float tabWidthPixels = mDoc.mTabSize * mDrawContext.getGlyphWidth();
  return (1.f + floor ((1.f + xPos) / tabWidthPixels)) * tabWidthPixels;
  }
//}}}
//{{{
uint32_t cTextEdit::getColumnFromPosX (const cLine& line, float posX) {
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
      column = mDoc.getTabColumn (column);
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
void cTextEdit::setCursorPosition (sPosition position) {
// set cursorPosition, if changed check scrollVisible

  if (position != mEdit.mCursor.mPosition) {
    mEdit.mCursor.mPosition = position;
    mEdit.mScrollVisible = true;
    }
  }
//}}}

//{{{
void cTextEdit::setDeselect() {

  mEdit.mCursor.mSelect = eSelect::eNormal;
  mEdit.mCursor.mSelectBeginPosition = mEdit.mCursor.mPosition;
  mEdit.mCursor.mSelectEndPosition = mEdit.mCursor.mPosition;
  }
//}}}
//{{{
void cTextEdit::setSelect (eSelect select, sPosition beginPosition, sPosition endPosition) {

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
      const cLine& line = mDoc.getLine (mEdit.mCursor.mSelectBeginPosition.mLineNumber);
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
void cTextEdit::moveUp (uint32_t amount) {

  setDeselect();

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
void cTextEdit::moveDown (uint32_t amount) {

  setDeselect();

  sPosition position = mEdit.mCursor.mPosition;

  if (!isFolded()) {
    //{{{  unfolded simple moveDown
    position.mLineNumber = min (position.mLineNumber + amount, mDoc.getMaxLineNumber());
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
void cTextEdit::cursorFlashOn() {
// set cursor flash cycle to on, helps after any move

  mCursorFlashTimePoint = system_clock::now();
  }
//}}}
//{{{
void cTextEdit::scrollCursorVisible() {

  ImGui::SetWindowFocus();
  cursorFlashOn();

  sPosition position = getCursorPosition();

  // up,down scroll
  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
  uint32_t maxLineIndex = isFolded() ? getMaxFoldLineIndex() : mDoc.getMaxLineNumber();

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
sPosition cTextEdit::advancePosition (sPosition position) {

  if (position.mLineNumber >= mDoc.getNumLines())
    return position;

  uint32_t glyphIndex = mDoc.getGlyphIndex (position);
  const cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
  if (glyphIndex + 1 < glyphsLine.getNumGlyphs())
    position.mColumn = mDoc.getColumn (glyphsLine, glyphIndex + 1);
  else {
    position.mLineNumber++;
    position.mColumn = 0;
    }

  return position;
  }
//}}}
//{{{
sPosition cTextEdit::sanitizePosition (sPosition position) {
// return position to be within glyphs

  if (position.mLineNumber >= mDoc.getNumLines()) {
    // past end, position at end of last line
    //cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{} to max:{}",
    //                                 position.mLineNumber, getMaxLineNumber()));
    return sPosition (mDoc.getMaxLineNumber(), mDoc.getNumColumns (mDoc.getLine (mDoc.getMaxLineNumber())));
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
sPosition cTextEdit::findWordBeginPosition (sPosition position) {

  const cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
  uint32_t glyphIndex = mDoc.getGlyphIndex (glyphsLine, position.mColumn);
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

  position.mColumn = mDoc.getColumn (glyphsLine, glyphIndex);
  return position;
  }
//}}}
//{{{
sPosition cTextEdit::findWordEndPosition (sPosition position) {

  const cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
  uint32_t glyphIndex = mDoc.getGlyphIndex (glyphsLine, position.mColumn);
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

  position.mColumn = mDoc.getColumn (glyphsLine, glyphIndex);
  return position;
  }
//}}}
//}}}
//{{{
sPosition cTextEdit::insertTextAt (sPosition position, const string& text) {
// insert helper - !!! needs utf8 handling !!!!

  if (text.empty())
    return position;

  bool onFoldBegin = mDoc.getLine (position.mLineNumber).mFoldBegin;

  uint32_t glyphIndex = mDoc.getGlyphIndex (getGlyphsLine (position.mLineNumber), position.mColumn);
  for (auto it = text.begin(); it < text.end(); ++it) {
    char ch = *it;
    if (ch == '\r') // skip
      break;

    cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
    if (ch == '\n') {
      if (onFoldBegin) // no lineFeed in allowed in foldBegin, return
        return position;

      // insert new line
      cLine& newLine = *mDoc.mLines.insert (mDoc.mLines.begin() + position.mLineNumber+1, cLine());

      if (glyphIndex < glyphsLine.getNumGlyphs()) {
        // not end of line, splitting line, copy rest of line to newLine
        newLine.insertRestOfLineAtEnd (glyphsLine, glyphIndex);

        // remove rest of line just copied to newLine
        glyphsLine.eraseToEnd (glyphIndex);
        mDoc.parse (glyphsLine);
        }
      mDoc.parse (newLine);
      glyphIndex = 0;

      // !!! should convert glyph back to column in case of tabs !!!
      position.mLineNumber++;
      position.mColumn = 0;
      }

    else {
      // insert char within line
      glyphsLine.insert (glyphIndex++, cGlyph (ch, eText));
      mDoc.parse (glyphsLine);

      // !!! should convert glyph back to column in case of tabs !!!
      position.mColumn++;
      }
    mDoc.edited();
    }

  return position;
  }
//}}}
//{{{
void cTextEdit::deleteSelect() {

  mDoc.deletePositionRange (mEdit.mCursor.mSelectBeginPosition, mEdit.mCursor.mSelectEndPosition);
  setCursorPosition (mEdit.mCursor.mSelectBeginPosition);
  setDeselect();
  }
//}}}

// fold
//{{{
void cTextEdit::openFold (uint32_t lineNumber) {

  if (isFolded()) {
    cLine& line = mDoc.getLine (lineNumber);
    if (line.mFoldBegin)
      line.mFoldOpen = true;

    // position cursor to lineNumber, first glyph column
    setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
    }
  }
//}}}
//{{{
void cTextEdit::openFoldOnly (uint32_t lineNumber) {

  if (isFolded()) {
    cLine& line = mDoc.getLine (lineNumber);
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
void cTextEdit::closeFold (uint32_t lineNumber) {

  if (isFolded()) {
    // close fold containing lineNumber
    cLog::log (LOGINFO, fmt::format ("closeFold line:{}", lineNumber));

    cLine& line = mDoc.getLine (lineNumber);
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
        line = mDoc.getLine (--lineNumber);
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
uint32_t cTextEdit::skipFold (uint32_t lineNumber) {
// recursively skip fold lines until matching foldEnd

  while (lineNumber < mDoc.getNumLines())
    if (mDoc.getLine (lineNumber).mFoldBegin)
      lineNumber = skipFold (lineNumber+1);
    else if (mDoc.getLine (lineNumber).mFoldEnd)
      return lineNumber+1;
    else
      lineNumber++;

  // error if you run off end. begin/end mismatch
  cLog::log (LOGERROR, "skipToFoldEnd - unexpected end reached with mathching fold");
  return lineNumber;
  }
//}}}
//{{{
uint32_t cTextEdit::drawFolded() {

  uint32_t lineIndex = 0;

  uint32_t lineNumber = mEdit.mFoldOnly ? mEdit.mFoldOnlyBeginLineNumber : 0;

  //cLog::log (LOGINFO, fmt::format ("drawFolded {}", lineNumber));

  while (lineNumber < mDoc.getNumLines()) {
    cLine& line = mDoc.getLine (lineNumber);

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
            mDoc.getLine (glyphsLineNumber).mFirstGlyph = static_cast<uint8_t>(line.mIndent);
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
void cTextEdit::keyboard() {
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
void cTextEdit::mouseSelectLine (uint32_t lineNumber) {

  cursorFlashOn();

  cLog::log (LOGINFO, fmt::format ("mouseSelectLine line:{}", lineNumber));

  mEdit.mCursor.mPosition = {lineNumber, 0};
  mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
  setSelect (eSelect::eLine, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
  }
//}}}
//{{{
void cTextEdit::mouseDragSelectLine (uint32_t lineNumber, float posY) {
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
    lineNumber = max (0u, min (mDoc.getMaxLineNumber(), lineNumber + numDragLines));

  setCursorPosition ({lineNumber, 0});

  setSelect (eSelect::eLine, mEdit.mDragFirstPosition, getCursorPosition());
  }
//}}}
//{{{
void cTextEdit::mouseSelectText (bool selectWord, uint32_t lineNumber, ImVec2 pos) {

  cursorFlashOn();
  //cLog::log (LOGINFO, fmt::format ("mouseSelectText line:{}", lineNumber));

  mEdit.mCursor.mPosition = {lineNumber, getColumnFromPosX (getGlyphsLine (lineNumber), pos.x)};
  mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
  setSelect (selectWord ? eSelect::eWord : eSelect::eNormal, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
  }
//}}}
//{{{
void cTextEdit::mouseDragSelectText (uint32_t lineNumber, ImVec2 pos) {

  if (!canEditAtCursor())
    return;

  int numDragLines = static_cast<int>((pos.y - (mDrawContext.getLineHeight()/2.f)) / mDrawContext.getLineHeight());
  uint32_t dragLineNumber = max (0u, min (mDoc.getMaxLineNumber(), lineNumber + numDragLines));

  //cLog::log (LOGINFO, fmt::format ("mouseDragSelectText {} {}", lineNumber, numDragLines));

  if (isFolded()) {
    // cannot cross fold
    if (lineNumber < dragLineNumber) {
      while (++lineNumber <= dragLineNumber)
        if (mDoc.getLine (lineNumber).mFoldBegin || mDoc.getLine (lineNumber).mFoldEnd) {
          cLog::log (LOGINFO, fmt::format ("dragSelectText exit 1"));
          return;
          }
      }
    else if (lineNumber > dragLineNumber) {
      while (--lineNumber >= dragLineNumber)
        if (mDoc.getLine (lineNumber).mFoldBegin || mDoc.getLine (lineNumber).mFoldEnd) {
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
float cTextEdit::drawGlyphs (ImVec2 pos, const cLine& line, uint8_t forceColor) {

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
void cTextEdit::drawSelect (ImVec2 pos, uint32_t lineNumber, uint32_t glyphsLineNumber) {

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
    posEnd.x += getWidth ({lineNumber, mDoc.getNumColumns (mDoc.getLine (lineNumber))});
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
void cTextEdit::drawCursor (ImVec2 pos, uint32_t lineNumber) {

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

    cLine& line = mDoc.getLine (lineNumber);
    uint32_t glyphIndex = mDoc.getGlyphIndex (position);
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
void cTextEdit::drawLine (uint32_t lineNumber, uint32_t lineIndex) {
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
  cLine& line = mDoc.getLine (lineNumber);
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
void cTextEdit::drawLines() {
//  draw unfolded with clipper

  // clipper begin
  ImGuiListClipper clipper;
  clipper.Begin (mDoc.getNumLines(), mDrawContext.getLineHeight());
  clipper.Step();

  // clipper iterate
  for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++) {
    mDoc.getLine (lineNumber).mFirstGlyph = 0;
    drawLine (lineNumber, 0);
    }

  // clipper end
  clipper.Step();
  clipper.End();
  }
//}}}
