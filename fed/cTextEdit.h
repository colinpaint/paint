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

// only used by cTextEdit
//{{{
class cGlyph {
public:
  //{{{
  cGlyph()
      : mNumUtf8Bytes(1), mColor(0),
        mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {

    mUtfChar[0] =  ' ';
    }
  //}}}
  //{{{
  cGlyph (ImWchar ch, uint8_t color)
      : mColor(color), mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {
  // maximum size 4

    if (ch < 0x80) {
      mUtfChar[0] = static_cast<uint8_t> (ch);
      mNumUtf8Bytes = 1;
      return;
      }

    if (ch < 0x800) {
      mUtfChar[0] = static_cast<uint8_t> (0xc0 + (ch >> 6));
      mUtfChar[1] = static_cast<uint8_t> (0x80 + (ch & 0x3f));
      mNumUtf8Bytes = 2;
      return;
      }

    if ((ch >= 0xdc00) && (ch < 0xe000)) {
      // unknown; mUtfChar[0] = '?';
      mNumUtf8Bytes = 1;
      return;
      }

    if ((ch >= 0xd800) && (ch < 0xdc00)) {
      //mUtfChar[0] = static_cast<uint8_t> (0xf0 + (ch >> 18));
      mUtfChar[0] = static_cast<uint8_t> (0xf0);
      mUtfChar[1] = static_cast<uint8_t> (0x80 + ((ch >> 12) & 0x3f));
      mUtfChar[2] = static_cast<uint8_t> (0x80 + ((ch >> 6) & 0x3f));
      mUtfChar[3] = static_cast<uint8_t> (0x80 + ((ch) & 0x3f));
      mNumUtf8Bytes = 4;
      return;
      }

    mUtfChar[0] = static_cast<uint8_t> (0xe0 + (ch >> 12));
    mUtfChar[1] = static_cast<uint8_t> (0x80 + ((ch >> 6) & 0x3f));
    mUtfChar[2] = static_cast<uint8_t> (0x80 + ((ch) & 0x3f));
    mNumUtf8Bytes = 3;
    }
  //}}}
  //{{{
  cGlyph (const uint8_t* utf8, uint8_t size, uint8_t color)
      : mNumUtf8Bytes(size), mColor(color), mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {

    // maximum size 6
    if (size <= 6)
      for (uint32_t i = 0; i < mNumUtf8Bytes; i++)
        mUtfChar[i] = *utf8++;
    }
  //}}}

  // utf
  //{{{
  static uint8_t numUtf8Bytes (uint8_t ch) {
  // https://en.wikipedia.org/wiki/UTF-8
  // We assume that the char is a standalone character ( < 128)
  // or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)

    if ((ch & 0xFE) == 0xFC)
      return 6;

    if ((ch & 0xFC) == 0xF8)
      return 5;

    if ((ch & 0xF8) == 0xF0)
      return 4;

    else if ((ch & 0xF0) == 0xE0)
      return 3;

    else if ((ch & 0xE0) == 0xC0)
      return 2;

    return 1;
    }
  //}}}

  // vars
  uint8_t mUtfChar[6];    // expensive
  uint8_t mNumUtf8Bytes;  // expensive
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
  //{{{
  ~cLine() {
    mGlyphs.clear();
    }
  //}}}

  // gets
  //{{{
  bool empty() const {
    return mGlyphs.empty();
    }
  //}}}
  //{{{
  uint32_t getNumGlyphs() const {
    return static_cast<uint32_t>(mGlyphs.size());
    }
  //}}}

  //{{{
  uint8_t getNumCharBytes (uint32_t glyphIndex) const {
    return mGlyphs[glyphIndex].mNumUtf8Bytes;
    }
  //}}}
  //{{{
  uint8_t getChar (uint32_t glyphIndex, uint32_t utf8Index) const {
    return mGlyphs[glyphIndex].mUtfChar[utf8Index];
    }
  //}}}
  //{{{
  uint8_t getChar (uint32_t glyphIndex) const {
    return getChar (glyphIndex, 0);
    }
  //}}}

  //{{{
  std::string getString() const {

     std::string lineString;
     lineString.reserve (mGlyphs.size());

     for (auto& glyph : mGlyphs)
       for (uint32_t utf8index = 0; utf8index < glyph.mNumUtf8Bytes; utf8index++)
         lineString += glyph.mUtfChar[utf8index];

     return lineString;
     }
  //}}}
  //{{{
  uint8_t getColor (uint32_t glyphIndex) const {
    return mGlyphs[glyphIndex].mColor;
    }
  //}}}

  //{{{
  bool getCommentSingle() const {
    return mCommentSingle;
    }
  //}}}
  //{{{
  bool getCommentSingle (const uint32_t glyphIndex) const {
    return mGlyphs[glyphIndex].mCommentSingle;
    }
  //}}}
  //{{{
  bool getCommentBegin() const {
    return mCommentBegin;
    }
  //}}}
  //{{{
  bool getCommentBegin (const uint32_t glyphIndex) const {
    return mGlyphs[glyphIndex].mCommentBegin;
    }
  //}}}
  //{{{
  bool getCommentEnd() const {
    return mCommentEnd;
    }
  //}}}
  //{{{
  bool getCommentEnd (const uint32_t glyphIndex) const {
    return mGlyphs[glyphIndex].mCommentEnd;
    }
  //}}}

  //{{{
  cGlyph getGlyph (const uint32_t glyphIndex) const {
    return mGlyphs[glyphIndex];
    }
  //}}}

  // sets
  //{{{
  void setColor (uint8_t color) {
    for (auto& glyph : mGlyphs)
      glyph.mColor = color;
    }
  //}}}
  //{{{
  void setColor (uint32_t glyphIndex, uint8_t color) {
    mGlyphs[glyphIndex].mColor = color;
    }
  //}}}
  //{{{
  void setCommentSingle (size_t glyphIndex) {
    mCommentSingle = true;
    mGlyphs[glyphIndex].mCommentSingle = true;
    }
  //}}}
  //{{{
  void setCommentBegin (size_t glyphIndex) {
    mCommentBegin = true;
    mGlyphs[glyphIndex].mCommentBegin = true;
    }
  //}}}
  //{{{
  void setCommentEnd (size_t glyphIndex) {
    mCommentEnd = true;
    mGlyphs[glyphIndex].mCommentEnd = true;
    }
  //}}}

  // ops
  //{{{
  void parseReset() {

    mIndent = 0;
    mFirstGlyph = 0;

    mCommentBegin = false;
    mCommentEnd = false;
    mCommentSingle = false;

    mFoldBegin = false;
    mFoldEnd = false;

    mFoldOpen = false;
    }
  //}}}
  //{{{
  void reserve (size_t size) {
    mGlyphs.reserve (size);
    }
  //}}}
  //{{{
  void pushBack (cGlyph glyph) {
    mGlyphs.push_back (glyph);
    }
  //}}}
  //{{{
  void emplaceBack (cGlyph glyph) {
    mGlyphs.emplace_back (glyph);
    }
  //}}}
  //{{{
  uint32_t trimTrailingSpace() {

    uint32_t trimmedSpaces = 0;
    uint32_t column = getNumGlyphs();
    while ((column > 0) && (mGlyphs[--column].mUtfChar[0] == ' ')) {
      // trailingSpace, trim it
      mGlyphs.pop_back();
      trimmedSpaces++;
      }

    return trimmedSpaces;
    }
  //}}}

  // insert
  //{{{
  void insert (uint32_t glyphIndex, const cGlyph& glyph) {
    mGlyphs.insert (mGlyphs.begin() + glyphIndex, glyph);
    }
  //}}}
  //{{{
  void insertLineAtEnd (const cLine& lineToInsert) {
  // insert lineToInsert to end of line

    mGlyphs.insert (mGlyphs.end(), lineToInsert.mGlyphs.begin(), lineToInsert.mGlyphs.end());
    }
  //}}}
  //{{{
  void insertRestOfLineAtEnd (const cLine& lineToInsert, uint32_t glyphIndex) {
  // insert from glyphIndex of lineToInsert to end of line

    mGlyphs.insert (mGlyphs.end(), lineToInsert.mGlyphs.begin() + glyphIndex, lineToInsert.mGlyphs.end());
    }
  //}}}

  // erase
  //{{{
  void erase (uint32_t glyphIndex) {
    mGlyphs.erase (mGlyphs.begin() + glyphIndex);
    }
  //}}}
  //{{{
  void eraseToEnd (uint32_t glyphIndex) {
    mGlyphs.erase (mGlyphs.begin() + glyphIndex, mGlyphs.end());
    }
  //}}}
  //{{{
  void erase (uint32_t glyphIndex, uint32_t toGlyphIndex) {
    mGlyphs.erase (mGlyphs.begin() + glyphIndex, mGlyphs.begin() + toGlyphIndex);
    }
  //}}}

  // offsets
  uint8_t mIndent = 0;     // leading space count
  uint8_t mFirstGlyph = 0; // index of first visible glyph, past fold marker

  // parsed tokens
  bool mFoldBegin = false;
  bool mFoldComment = false;
  bool mFoldEnd = false;

  // fold state
  bool mFoldOpen = false;
  bool mFoldPressed = false;

private:
  std::vector <cGlyph> mGlyphs;

  bool mCommentSingle = false;
  bool mCommentBegin = false;
  bool mCommentEnd = false;
  };
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
    void update (const cOptions& options, bool monoSpaced);

    float measureSpace() const;
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
    bool mMonoSpaced = false;

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
    bool mHasUtf8 = false;
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
  std::string getText (sPosition beginPosition, sPosition endPosition);
  std::string getSelectText() { return getText (mEdit.mCursor.mSelectBegin, mEdit.mCursor.mSelectEnd); }

  // text widths
  float getWidth (sPosition position);
  float getGlyphWidth (const cLine& line, uint32_t glyphIndex);

  // lines
  uint32_t getNumLines() const { return static_cast<uint32_t>(mDoc.mLines.size()); }
  uint32_t getNumFoldLines() const { return static_cast<uint32_t>(mDoc.mFoldLines.size()); }
  uint32_t getMaxLineIndex() const { return isFolded() ? getNumFoldLines()-1 : getNumLines()-1; }
  uint32_t getNumPageLines() const;

  // line
  cLine& getLine (uint32_t lineNumber) { return mDoc.mLines[lineNumber]; }

  uint32_t getNumGlyphs (uint32_t lineNumber) { return getLine (lineNumber).getNumGlyphs(); }
  uint32_t getLineNumColumns (uint32_t lineNumber);

  uint32_t getLineNumberFromIndex (uint32_t lineIndex) const;
  uint32_t getLineIndexFromNumber (uint32_t lineNumber) const;

  sPosition getNextLinePosition (sPosition position);

  // column
  uint32_t getGlyphIndexFromPosition (sPosition position);
  uint32_t getColumnFromPosX (uint32_t lineNumber, float posX);
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
  uint32_t drawFolded();

  // keyboard,mouse
  void keyboard();
  void mouseSelectLine (uint32_t lineNumber);
  void mouseDragSelectLine (uint32_t lineNumber, float posY);
  void mouseSelectText (bool selectWord, uint32_t lineNumber, ImVec2 pos);
  void mouseDragSelectText (uint32_t lineNumber, ImVec2 pos);

  // draw
  float drawGlyphs (ImVec2 pos, const cLine& line, uint8_t forceColor);
  void drawSelect (ImVec2 pos, uint32_t lineNumber);
  void drawCursor (ImVec2 curPos, uint32_t lineNumber);
  void drawLine (uint32_t lineNumber, uint32_t lineIndex);
  void drawLines();

  //{{{  vars
  bool mOpen = true;  // set false when DrawWindow() closed

  cOptions mOptions;
  cDoc mDoc;
  cDrawContext mDrawContext;
  cEdit mEdit;

  std::chrono::system_clock::time_point mCursorFlashTimePoint;
  //}}}
  };
