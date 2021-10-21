// cTextEdit.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <chrono>
#include <regex>

#include <vector>
#include <unordered_set>

#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

#include "cDocument.h"
class cApp;
//}}}

class cTextEdit {
public:
  cTextEdit (cDocument& document);
  ~cTextEdit() = default;

  enum class eSelect { eNormal, eWord, eLine };
  //{{{  gets
  cDocument& getDocument() { return mDoc; }
  cLanguage& getLanguage() { return mDoc.getLanguage(); }

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

  uint32_t getTabSize() const { return mDoc.mTabSize; }
  sPosition getCursorPosition() { return sanitizePosition (mEdit.mCursor.mPosition); }
  //}}}
  //{{{  sets
  void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }
  void setTabSize (uint32_t tabSize) { mDoc.mTabSize = tabSize; }

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
  void moveEnd() { setCursorPosition ({mDoc.getMaxLineNumber(), 0}); }

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

  void drawWindow (const std::string& title, cApp& app);
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
    void undo (cTextEdit* textEdit) {

      if (!mAddText.empty())
        textEdit->getDocument().deletePositionRange (mAddBeginPosition, mAddEndPosition);
      if (!mDeleteText.empty())
        textEdit->insertText (mDeleteText, mDeleteBeginPosition);

      textEdit->mEdit.mCursor = mBeforeCursor;
      }
    //}}}
    //{{{
    void redo (cTextEdit* textEdit) {

      if (!mDeleteText.empty())
        textEdit->getDocument().deletePositionRange (mDeleteBeginPosition, mDeleteEndPosition);
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
    void undo (cTextEdit* textEdit, uint32_t steps) {

      while (hasUndo() && (steps > 0)) {
        mUndoVector [--mUndoIndex].undo (textEdit);
        steps--;
        }
      }
    //}}}
    //{{{
    void redo (cTextEdit* textEdit, uint32_t steps) {

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
  class cTextEditDrawContext : public cDrawContext {
  // drawContext with our palette and a couple of layout vars
  public:
    cTextEditDrawContext() : cDrawContext (kPalette) {}
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
  std::string getSelectText() { return mDoc.getText (mEdit.mCursor.mSelectBeginPosition, mEdit.mCursor.mSelectEndPosition); }

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

    const cLine& line = mDoc.mLines[lineNumber];
    if (isFolded() && line.mFoldBegin && !line.mFoldOpen && (line.mFirstGlyph == line.getNumGlyphs()))
      return lineNumber + 1;
    else
      return lineNumber;
    }
  //}}}
  cLine& getGlyphsLine (uint32_t lineNumber) { return mDoc.getLine (getGlyphsLineNumber (lineNumber)); }

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
  cDocument& mDoc;

  // edit state
  cEdit mEdit;
  cOptions mOptions;
  cTextEditDrawContext mDrawContext;

  std::vector <uint32_t> mFoldLines;

  std::chrono::system_clock::time_point mCursorFlashTimePoint;
  };
