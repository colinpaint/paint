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

class cApp;
//}}}
class cTextEdit {
public:
  enum class eSelect { eNormal, eWord, eLine };
  //{{{
  struct sPosition {
    sPosition() : mLineNumber(0), mColumn(0) {}
    sPosition (uint32_t lineNumber, uint32_t column) : mLineNumber(lineNumber), mColumn(column) {}

    //{{{
    bool operator == (const sPosition& o) const {
      return mLineNumber == o.mLineNumber && mColumn == o.mColumn;
      }
    //}}}
    //{{{
    bool operator != (const sPosition& o) const {
      return mLineNumber != o.mLineNumber || mColumn != o.mColumn; }
    //}}}
    //{{{
    bool operator < (const sPosition& o) const {

      if (mLineNumber != o.mLineNumber)
        return mLineNumber < o.mLineNumber;

      return mColumn < o.mColumn;
      }
    //}}}
    //{{{
    bool operator > (const sPosition& o) const {
      if (mLineNumber != o.mLineNumber)
        return mLineNumber > o.mLineNumber;
      return mColumn > o.mColumn;
      }
    //}}}
    //{{{
    bool operator <= (const sPosition& o) const {
      if (mLineNumber != o.mLineNumber)
        return mLineNumber < o.mLineNumber;

      return mColumn <= o.mColumn;
      }
    //}}}
    //{{{
    bool operator >= (const sPosition& o) const {
      if (mLineNumber != o.mLineNumber)
        return mLineNumber > o.mLineNumber;
      return mColumn >= o.mColumn;
      }
    //}}}

    uint32_t mLineNumber;
    uint32_t mColumn;
    };
  //}}}
  //{{{
  class cLanguage {
  public:
    using tRegex = std::vector <std::pair <std::regex,uint8_t>>;
    using tTokenSearch = bool(*)(const char* srcBegin, const char* srcEnd,
                                 const char*& dstBegin, const char*& dstEnd, uint8_t& color);
    // static const
    static const cLanguage c();
    static const cLanguage hlsl();
    static const cLanguage glsl();

    // vars
    std::string mName;
    bool mAutoIndentation = true;

    // comment tokens
    std::string mCommentSingle;
    std::string mCommentBegin;
    std::string mCommentEnd;

    // fold tokens
    std::string mFoldBeginToken;
    std::string mFoldEndToken;

    // fold indicators
    std::string mFoldBeginOpen = "{{{ ";
    std::string mFoldBeginClosed = "... ";
    std::string mFoldEnd = "}}}";

    std::unordered_set <std::string> mKeyWords;
    std::unordered_set <std::string> mKnownWords;

    tTokenSearch mTokenSearch = nullptr;
    cLanguage::tRegex mRegexList;
    };
  //}}}
  //{{{
  class cGlyph {
  public:
    cGlyph() : mChar(' '), mColor(0),
               mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {}

    cGlyph (uint8_t ch, uint8_t color) : mChar(ch), mColor(color),
                                         mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {}

    // vars
    uint8_t mChar;
    uint8_t mColor;

    // commentFlags, speeds up wholeText comment parsing
    bool mCommentSingle:1;
    bool mCommentBegin:1;
    bool mCommentEnd:1;
    };
  //}}}
  //{{{
  class cLine {
  public:
    using tGlyphs = std::vector <cGlyph>;
    //{{{
    cLine() :
      mGlyphs(),
      mCommentSingle(false), mCommentBegin(false), mCommentEnd(false),  mCommentFold(false),
      mFoldBegin(false), mFoldEnd(false),
      mFoldOpen(false), mFoldPressed(false),
      mFoldOffset(0), mFirstGlyph(0) {}
    //}}}
    //{{{
    cLine (const std::vector<cGlyph>& line) :
      mGlyphs(line),
      mCommentSingle(false), mCommentBegin(false), mCommentEnd(false),  mCommentFold(false),
      mFoldBegin(false), mFoldEnd(false),
      mFoldOpen(false), mFoldPressed(false),
      mFoldOffset(0), mFirstGlyph(0) {}
    //}}}
    //{{{
    ~cLine() {
      mGlyphs.clear();
      }
    //}}}

    uint32_t getNumGlyphs() const { return static_cast<uint32_t>(mGlyphs.size()); }

    tGlyphs mGlyphs;

    // parsed tokens
    bool mCommentSingle;
    bool mCommentBegin;
    bool mCommentEnd;
    bool mCommentFold;
    bool mFoldBegin;
    bool mFoldEnd;

    // fold state
    bool mFoldOpen;
    bool mFoldPressed;

    // offsets
    uint32_t mFoldOffset; // closed fold title line offset
    uint8_t mIndent;      // leading space count
    uint8_t mFirstGlyph;  // index of first visible glyph, past fold marker
    };
  //}}}

  cTextEdit();
  ~cTextEdit() = default;

  //{{{  gets
  bool isEdited() const { return mDoc.mEdited; }
  bool isReadOnly() const { return mOptions.mReadOnly; }
  bool isShowFolds() const { return mOptions.mShowFolded; }

  // has
  bool hasCR() const { return mDoc.mHasCR; }
  bool hasTabs() const { return mDoc.mHasTabs; }
  bool hasSelect() const { return mEdit.mCursor.mSelectEnd > mEdit.mCursor.mSelectBegin; }
  bool hasUndo() const { return !mOptions.mReadOnly && mEdit.hasUndo(); }
  bool hasRedo() const { return !mOptions.mReadOnly && mEdit.hasRedo(); }
  bool hasPaste() const { return !mOptions.mReadOnly && mEdit.hasPaste(); }

  // get
  std::string getTextString();
  std::vector<std::string> getTextStrings() const;

  uint32_t getTabSize() const { return mDoc.mTabSize; }
  sPosition getCursorPosition() { return sanitizePosition (mEdit.mCursor.mPosition); }

  const cLanguage& getLanguage() const { return mOptions.mLanguage; }
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
  void moveEnd() { setCursorPosition ({getNumLines()-1, 0}); }

  // select
  void selectAll();

  // cut and paste
  void copy();
  void cut();
  void paste();

  // delete
  void deleteIt();
  void backspace();

  // insert
  void enterCharacter (ImWchar ch);
  void enterKey()    { enterCharacter ('\n'); }
  void tabKey()      { enterCharacter ('\t'); }

  // fold
  void createFold();
  void openFold() { openFold (mEdit.mCursor.mPosition.mLineNumber); }
  void openFoldOnly() { openFoldOnly (mEdit.mCursor.mPosition.mLineNumber); }
  void closeFold() { closeFold (mEdit.mCursor.mPosition.mLineNumber); }

  // undo
  void undo (uint32_t steps = 1);
  void redo (uint32_t steps = 1);

  // file
  void loadFile (const std::string& filename);
  void saveFile();
  //}}}

  void drawWindow (const std::string& title, cApp& app);
  void drawContents (cApp& app);

private:
  //{{{
  struct sCursor {
    sPosition mPosition;

    eSelect mSelect;
    sPosition mSelectBegin;
    sPosition mSelectEnd;
    };
  //}}}
  //{{{
  class cUndo {
  public:
    //{{{
    void undo (cTextEdit* textEdit) {

      if (!mAdd.empty())
        textEdit->deletePositionRange (mAddBegin, mAddEnd);

      if (!mDelete.empty()) {
        sPosition begin = mDeleteBegin;
        textEdit->insertTextAt (begin, mDelete);
        }

      textEdit->mEdit.mCursor = mBefore;
      }
    //}}}
    //{{{
    void redo (cTextEdit* textEdit) {

      if (!mDelete.empty())
        textEdit->deletePositionRange (mDeleteBegin, mDeleteEnd);

      if (!mAdd.empty()) {
        sPosition begin = mAddBegin;
        textEdit->insertTextAt (begin, mAdd);
        }

      textEdit->mEdit.mCursor = mAfter;
      }
    //}}}

    // vars
    sCursor mBefore;
    sCursor mAfter;

    std::string mAdd;
    sPosition mAddBegin;
    sPosition mAddEnd;

    std::string mDelete;
    sPosition mDeleteBegin;
    sPosition mDeleteEnd;
    };
  //}}}
  //{{{
  class cOptions {
  public:
    int mFontSize = 16;
    int mmDocntSize = 4;
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

    cLanguage mLanguage;
    };
  //}}}
  //{{{
  class cDrawContext {
  public:
    void update (const cOptions& options);

    float measureChar (char ch) const;
    float measureText (const char* str, const char* strEnd) const;
    float text (ImVec2 pos, uint8_t color, const std::string& text);
    float text (ImVec2 pos, uint8_t color, const char* str, const char* strEnd = nullptr);
    float smallText (ImVec2 pos, uint8_t color, const std::string& text);

    void line (ImVec2 pos1, ImVec2 pos2, uint8_t color);
    void circle (ImVec2 centre, float radius, uint8_t color);
    void rect (ImVec2 pos1, ImVec2 pos2, uint8_t color);
    void rectLine (ImVec2 pos1, ImVec2 pos2, uint8_t color);

    float mFontSize = 0.f;
    float mGlyphWidth = 0.f;
    float mLineHeight = 0.f;

    float mLeftPad = 0.f;
    float mLineNumberWidth = 0.f;

  private:
    ImDrawList* mDrawList = nullptr;

    ImFont* mFont = nullptr;

    float mFontAtlasSize = 0.f;
    float mFontSmallSize = 0.f;
    float mFontSmallOffset = 0.f;
    };
  //}}}
  //{{{
  class cDoc {
  public:
    std::string mFilePath;
    std::string mParentPath;
    std::string mFileStem;
    std::string mFileExtension;
    uint32_t mVersion = 1;

    std::vector <cLine> mLines;
    std::vector <uint32_t> mFoldLines;

    bool mEdited = false;
    bool mHasFolds = false;
    bool mHasCR = false;
    bool mHasTabs = false;
    uint32_t mTabSize = 4;
    };
  //}}}
  //{{{
  class cEdit {
  public:
    //{{{
    bool isSelectLine (uint32_t lineNumber) {
      return (lineNumber >= mCursor.mSelectBegin.mLineNumber) &&
             (lineNumber <= mCursor.mSelectEnd.mLineNumber);
      }
    //}}}
    //{{{
    bool isCursorLine (uint32_t lineNumber) {
      return lineNumber == mCursor.mPosition.mLineNumber;
      }
    //}}}

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
    bool mCheckComments = true;
    bool mScrollVisible = false;

  private:
    // undo
    size_t mUndoIndex = 0;
    std::vector <cUndo> mUndoVector;

    // paste
    std::vector <std::string> mPasteVector;
    };
  //}}}

  //{{{  get
  bool isFolded() const { return mOptions.mShowFolded; }
  bool isDrawLineNumber() const { return mOptions.mShowLineNumber; }
  bool isDrawWhiteSpace() const { return mOptions.mShowWhiteSpace; }
  bool isDrawMonoSpaced() const { return mOptions.mShowMonoSpaced; }

  bool canEditAtCursor();

  // text
  std::string getSelectText();
  std::string getText (sPosition beginPosition, sPosition endPosition);

  // text widths
  float getWidth (sPosition position);
  float getGlyphCharacterWidth (const cLine::tGlyphs& glyphs, uint32_t& glyphIndex);

  // lines
  uint32_t getNumLines() const { return static_cast<uint32_t>(mDoc.mLines.size()); }
  uint32_t getNumFoldLines() const { return static_cast<uint32_t>(mDoc.mFoldLines.size()); }
  uint32_t getMaxLineIndex() const { return isFolded() ? getNumFoldLines()-1 : getNumLines()-1; }
  uint32_t getNumPageLines() const;

  // line
  cLine& getLine (uint32_t lineNumber) { return mDoc.mLines[lineNumber]; }
  cLine::tGlyphs& getGlyphs (uint32_t lineNumber) { return getLine (lineNumber).mGlyphs; }

  uint32_t getNumGlyphs (uint32_t lineNumber) { return static_cast<uint32_t>(getLine (lineNumber).mGlyphs.size()); }
  uint32_t getLineNumColumns (uint32_t lineNumber);

  uint32_t getLineNumberFromIndex (uint32_t lineIndex) const;
  uint32_t getLineIndexFromNumber (uint32_t lineNumber) const;

  uint32_t getDrawLineNumber (uint32_t lineNumber);
  sPosition getNextLinePosition (sPosition position);

  // column
  uint32_t getGlyphIndexFromPosition (sPosition position);
  sPosition getPositionFromPosX (uint32_t lineNumber, float posX);
  uint32_t getColumnFromGlyphIndex (uint32_t lineNumber, uint32_t toGlyphIndex);

  // tab
  float getTabEndPosX (float columnX);
  uint32_t getTabColumn (uint32_t column);
  //}}}
  //{{{  set
  void setCursorPosition (sPosition position);
  void setSelect (eSelect select, sPosition beginPosition, sPosition endPosition);
  void setDeselect();
  //}}}
  //{{{  move
  void moveUp (uint32_t amount);
  void moveDown (uint32_t amount);

  void cursorFlashOn();
  void scrollCursorVisible();

  sPosition advance (sPosition position);
  sPosition sanitizePosition (sPosition position);

  sPosition findWordBegin (sPosition position);
  sPosition findWordEnd (sPosition position);
  sPosition findNextWord (sPosition position);
  //}}}
  //{{{  insert
  cLine& insertLine (uint32_t index);
  sPosition insertTextAt (sPosition position, const std::string& text);
  sPosition insertText (const std::string& text);
  //}}}
  //{{{  delete
  void deleteLineRange (uint32_t beginLineNumber, uint32_t endLineNumber);
  void deleteLine (uint32_t lineNumber);

  void deletePositionRange (sPosition beginPosition, sPosition endPosition);
  void deleteSelect();
  //}}}
  //{{{  parse
  void parseTokens (cLine& line, const std::string& textString);
  void parseLine (cLine& line);
  void parseComments();

  uint32_t trimTrailingSpace();
  //}}}

  //  fold
  void openFold (uint32_t lineNumber);
  void openFoldOnly (uint32_t lineNumber);
  void closeFold (uint32_t lineNumber);
  uint32_t skipFold (uint32_t lineNumber);

  // keyboard,mouse
  void keyboard();
  void mouseSelectLine (uint32_t lineNumber);
  void mouseDragSelectLine (uint32_t lineNumber, float posY);
  void mouseSelectText (bool selectWord, uint32_t lineNumber, ImVec2 pos);
  void mouseDragSelectText (uint32_t lineNumber, ImVec2 pos);

  // draw
  float drawGlyphs (ImVec2 pos, const cLine::tGlyphs& glyphs, uint8_t firstGlyph, uint8_t forceColor);
  void drawSelect (ImVec2 pos, uint32_t lineNumber);
  void drawCursor (ImVec2 curPos, uint32_t lineNumber);
  void drawLine (uint32_t lineNumber, uint32_t lineIndex);
  void drawUnfolded();
  uint32_t drawFolded();

  //{{{  vars
  bool mOpen = true;  // set false when DrawWindow() closed

  cOptions mOptions;
  cDoc mDoc;
  cDrawContext mDrawContext;
  cEdit mEdit;

  std::chrono::system_clock::time_point mCursorFlashTimePoint;
  //}}}
  };
