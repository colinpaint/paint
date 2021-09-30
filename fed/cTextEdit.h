// cTextEdit.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <memory>
#include <regex>
#include <chrono>

#include <vector>
#include <array>
#include <map>
#include <unordered_map>
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
    static const cLanguage& c();
    static const cLanguage& hlsl();
    static const cLanguage& glsl();

    // vars
    std::string mName;

    std::string mCommentSingle;
    std::string mCommentBegin;
    std::string mCommentEnd;
    std::string mFoldBeginMarker;
    std::string mFoldEndMarker;

    std::string mFoldBeginOpen;
    std::string mFoldBeginClosed;
    std::string mFoldEnd;

    bool mAutoIndentation;

    std::unordered_set <std::string> mKeyWords;
    std::unordered_set <std::string> mKnownWords;

    tTokenSearch mTokenSearch;
    cLanguage::tRegex mRegexList;
    };
  //}}}
  //{{{
  class cGlyph {
  public:
    //{{{
    cGlyph() : mChar(' '), mColor(0),
               mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {}
    //}}}
    //{{{
    cGlyph (uint8_t ch, uint8_t color) : mChar(ch), mColor(color),
                                         mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {}
    //}}}

    uint8_t mChar;
    uint8_t mColor;

    // comment toen flags to speed up whole text comment parsing
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
      mFoldCommentLineNumber(0), mFirstGlyph(0), mFirstColumn(0) {}
    //}}}
    //{{{
    cLine (const std::vector<cGlyph>& line) :
      mGlyphs(line),
      mCommentSingle(false), mCommentBegin(false), mCommentEnd(false),  mCommentFold(false),
      mFoldBegin(false), mFoldEnd(false),
      mFoldOpen(false), mFoldPressed(false),
      mFoldCommentLineNumber(0), mFirstGlyph(0), mFirstColumn(0) {}
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
    int mFoldCommentLineNumber; // line number of closed foldBegin comment
    uint8_t mIndent;            // leading space count
    uint8_t mFirstGlyph;        // index of first visible glyph, past fold markers
    uint8_t mFirstColumn;       // column of first visible glyph, past fold prefixes
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
  bool hasUndo() const { return !mOptions.mReadOnly && (mUndoList.mIndex > 0); }
  bool hasRedo() const { return !mOptions.mReadOnly && (mUndoList.mIndex < mUndoList.mBuffer.size()); }
  bool hasClipboardText();

  // get
  std::string getTextString();
  std::vector<std::string> getTextStrings() const;

  uint32_t getTabSize() const { return mDoc.mTabSize; }
  sPosition getCursorPosition() { return sanitizePosition (mEdit.mCursor.mPosition); }

  const cLanguage& getLanguage() const { return mOptions.mLanguage; }
  //}}}
  //{{{  sets
  void setTextString (const std::string& text);
  void setTextStrings (const std::vector<std::string>& lines);

  void setPalette (bool lightPalette);
  void setLanguage (const cLanguage& language);

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

  void moveLineUp()   { moveUp (1); }
  void moveLineDown() { moveDown (1); }
  void movePageUp()   { moveUp (getNumPageLines() - 4); }
  void movePageDown() { moveDown (getNumPageLines() - 4); }
  void moveHome();
  void moveEnd();

  // select
  void selectAll();
  void selectWordUnderCursor();

  // cut and paste
  void copy();
  void cut();
  void paste();

  // delete
  void deleteIt();
  void backspace();
  void deleteSelect();

  // insert
  void enterCharacter (ImWchar ch, bool shift);

  // fold
  void createFold();
  void openFold() { openFold (mEdit.mCursor.mPosition.mLineNumber); }
  void openFoldOnly() { openFoldOnly (mEdit.mCursor.mPosition.mLineNumber); }
  void closeFold() { closeFold (mEdit.mCursor.mPosition.mLineNumber); }

  // undo
  void undo (uint32_t steps = 1);
  void redo (uint32_t steps = 1);
  //}}}

  void drawWindow (const std::string& title, cApp& app);
  void drawContents (cApp& app);

private:
  //{{{
  class cDoc {
  public:
    std::string mFilename;

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

    bool mHoverFolded = false;
    bool mHoverLineNumber = false;
    bool mHoverWhiteSpace = false;
    bool mHoverMonoSpaced = false;

    cLanguage mLanguage;
    };
  //}}}
  //{{{
  class cContext {
  public:
    void update (const cOptions& options);

    float measureText (const char* str, const char* strEnd) const;
    float drawText (ImVec2 pos, uint8_t color, const char* str, const char* strEnd = nullptr);
    float drawSmallText (ImVec2 pos, uint8_t color, const char* str, const char* strEnd = nullptr);
    void drawLine (ImVec2 pos1, ImVec2 pos2, uint8_t color);
    void drawCircle (ImVec2 centre, float radius, uint8_t color);
    void drawRect (ImVec2 pos1, ImVec2 pos2, uint8_t color);
    void drawRectLine (ImVec2 pos1, ImVec2 pos2, uint8_t color);

    float mFontSize = 0.f;
    float mGlyphWidth = 0.f;
    float mLineHeight = 0.f;

    float mLeftPad = 0.f;
    float mLineNumberWidth = 0.f;

    std::vector <ImU32> mPalette;

  private:
    ImDrawList* mDrawList = nullptr;
    ImFont* mFont = nullptr;

    float mFontAtlasSize = 0.f;
    float mFontSmallSize = 0.f;
    float mFontSmallOffset = 0.f;
    };
  //}}}
  //{{{
  class cEdit {
  public:
    struct sCursor {
      sPosition mPosition;
      sPosition mSelectBegin;
      sPosition mSelectEnd;
      };

    sCursor mCursor;

    eSelect mSelect = eSelect::eNormal;
    sPosition mInteractiveBegin;
    sPosition mInteractiveEnd;

    // foldOnly state
    bool mFoldOnly = false;
    uint32_t mFoldOnlyBeginLineNumber = 0;

    // parse comments flag
    bool mCheckComments = true;
    bool mScrollVisible = false;
    };
  //}}}
  //{{{
  class cUndo {
  public:
    cUndo() = default;
    cUndo (const std::string& add, const sPosition addBegin, const sPosition addEnd,
           const std::string& remove, const sPosition removeBegin, const sPosition removeEnd,
           cEdit::sCursor& before, cEdit::sCursor& after)
        : mAdd(add), mAddBegin(addBegin), mAddEnd(addEnd),
          mRemove(remove), mRemoveBegin(removeBegin), mRemoveEnd(removeEnd),
          mBefore(before), mAfter(after) {}
    ~cUndo() = default;

    void undo (cTextEdit* textEdit);
    void redo (cTextEdit* textEdit);

    // vars
    std::string mAdd;
    sPosition mAddBegin;
    sPosition mAddEnd;

    std::string mRemove;
    sPosition mRemoveBegin;
    sPosition mRemoveEnd;

    cEdit::sCursor mBefore;
    cEdit::sCursor mAfter;
    };
  //}}}
  //{{{
  class cUndoList {
  public:
    uint32_t mIndex = 0;
    std::vector <cUndo> mBuffer;
    };
  //}}}

  //{{{  gets
  bool isFolded() const { return mOptions.mShowFolded || mOptions.mHoverFolded; }
  bool isDrawLineNumber() const { return mOptions.mShowLineNumber || mOptions.mHoverLineNumber; }
  bool isDrawWhiteSpace() const { return mOptions.mShowWhiteSpace || mOptions.mHoverWhiteSpace; }
  bool isDrawMonoSpaced() const { return mOptions.mShowMonoSpaced ^ mOptions.mHoverMonoSpaced; }

  // nums
  uint32_t getNumLines() const { return static_cast<uint32_t>(mDoc.mLines.size()); }
  uint32_t getNumFoldLines() const { return static_cast<uint32_t>(mDoc.mFoldLines.size()); }
  uint32_t getNumPageLines() const;

  // text
  std::string getText (sPosition beginPosition, sPosition endPosition);
  std::string getSelectedText() { return getText (mEdit.mCursor.mSelectBegin, mEdit.mCursor.mSelectEnd); }
  float getTextWidth (sPosition position);

  // lines
  uint32_t getLineNumberFromIndex (uint32_t lineIndex) const;
  uint32_t getLineIndexFromNumber (uint32_t lineNumber) const;
  uint32_t getMaxLineIndex() const { return isFolded() ? getNumFoldLines()-1 : getNumLines()-1; }

  // line
  cLine& getLine (uint32_t lineNumber) { return mDoc.mLines[lineNumber]; }
  cLine::tGlyphs& getGlyphs (uint32_t lineNumber) { return getLine (lineNumber).mGlyphs; }
  uint32_t getNumGlyphs (uint32_t lineNumber) { return static_cast<uint32_t>(getLine (lineNumber).mGlyphs.size()); }

  // column
  uint32_t getCharacterIndex (sPosition position);
  uint32_t getCharacterColumn (uint32_t lineNumber, uint32_t characterIndex);
  uint32_t getLineNumChars (uint32_t lineNumber);
  uint32_t getLineMaxColumn (uint32_t lineNumber);
  sPosition getPositionFromPosX (uint32_t lineNumber, float posX);

  // tab
  uint32_t getTabColumn (uint32_t column);
  float getTabEndPosX (float columnX);

  // word
  bool isOnWordBoundary (sPosition position);
  std::string getWordAt (sPosition position);
  std::string getWordUnderCursor() { return getWordAt (getCursorPosition()); }
  //}}}
  //{{{  sets
  void setCursorPosition (sPosition position);

  void setSelectBegin (sPosition position);
  void setSelectEnd (sPosition position);

  void setSelect (sPosition beginPosition, sPosition endPosition, eSelect select);
  //}}}
  //{{{  utils
  void scrollCursorVisible();
  void advance (sPosition& position);
  sPosition sanitizePosition (sPosition position);

  // find
  sPosition findWordBegin (sPosition fromPosition);
  sPosition findWordEnd (sPosition fromPosition);
  sPosition findNextWord (sPosition fromPosition);

  // move
  void moveUp (uint32_t amount);
  void moveDown (uint32_t amount);

  // insert
  cLine& insertLine (uint32_t index);
  void insertText (const std::string& insertString);
  uint32_t insertTextAt (sPosition& position, const std::string& insertString);

  // delete
  void removeLine (uint32_t beginPosition, uint32_t endPosition);
  void removeLine (uint32_t index);
  void deleteRange (sPosition beginPosition, sPosition endPosition);

  // undo
  void addUndo (cUndo& undo);
  //}}}

  // parse
  void parseTokens (cLine& line, const std::string& textString);
  void parseLine (cLine& line);
  void parseComments();

  //  fold
  void openFold (uint32_t lineNumber);
  void openFoldOnly (uint32_t lineNumber);
  void closeFold (uint32_t lineNumber);
  int skipFoldLines (uint32_t lineNumber);

  // mouse
  void selectText (uint32_t lineNumber, float posX, bool selectWord);
  void dragSelectText (uint32_t lineNumber, ImVec2 pos);
  void selectLine (uint32_t lineNumber);
  void dragSelectLine (uint32_t lineNumber, float posY);

  // draws
  void drawTop (cApp& app);
  float drawGlyphs (ImVec2 pos, const cLine::tGlyphs& glyphs, uint8_t firstGlyph, uint8_t forceColor);
  void drawLine (uint32_t lineNumber, uint32_t lineIndex);
  uint32_t drawFolded();
  void drawUnfolded();

  void handleKeyboard();

  //{{{  vars
  bool mOpen = true;  // set false when DrawWindow() closed

  cOptions mOptions;
  cDoc mDoc;
  cContext mContext;
  cEdit mEdit;
  cUndoList mUndoList;

  std::chrono::system_clock::time_point mCursorFlashTimePoint;
  //}}}
  };
