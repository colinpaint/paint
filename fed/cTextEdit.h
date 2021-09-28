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
  enum class eSelection { eNormal, eWord, eLine };
  //{{{
  struct sPosition {
    sPosition() : mLineNumber(0), mColumn(0) {}
    sPosition (int lineNumber, int column) : mLineNumber(lineNumber), mColumn(column) {}

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

    int mLineNumber;
    int mColumn;
    };
  //}}}
  //{{{
  struct sIdent {
    sPosition mLocation;
    std::string mDeclaration;
    };
  //}}}
  using tRegex = std::vector <std::pair <std::regex,uint8_t>>;
  //{{{
  class cLanguage {
  public:
    // static const
    static const cLanguage& c();
    static const cLanguage& hlsl();
    static const cLanguage& glsl();

    // using
    using tTokenRegexString = std::pair<std::string, uint8_t>;
    using tTokenRegexStrings = std::vector<tTokenRegexString>;
    using tTokenSearch = bool(*)(const char* srcBegin, const char* srcEnd,
                                 const char*& dstBegin, const char*& dstEnd, uint8_t& color);

    // vars
    std::string mName;

    std::string mCommentSingle;
    std::string mCommentBegin;
    std::string mCommentEnd;
    std::string mFoldBeginMarker;
    std::string mFoldEndMarker;

    // only used by drawText, no point in being a string
    const char* mFoldBeginOpen;
    const char* mFoldBeginClosed;
    const char* mFoldBeginClosedSpace;
    const char* mFoldEnd;

    bool mAutoIndentation;

    std::unordered_set <std::string> mKeyWords;
    std::unordered_set <std::string> mKnownWords;

    tTokenSearch mTokenSearch;
    tTokenRegexStrings mTokenRegex;
    tRegex mRegexList;
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
  using tGlyphs = std::vector <cGlyph>;
  //{{{
  class cLine {
  public:
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

    tGlyphs mGlyphs;

    // parsed tokens
    bool mCommentSingle:1;
    bool mCommentBegin:1;
    bool mCommentEnd:1;
    bool mCommentFold:1;
    bool mFoldBegin:1;
    bool mFoldEnd:1;

    // fold state
    bool mFoldOpen:1;
    bool mFoldPressed:1;

    // offsets
    uint32_t mFoldCommentLineNumber; // line number of closed foldBegin comment
    uint8_t mIndent;                 // leading space count
    uint8_t mFirstGlyph;             // index of first visible glyph, past fold markers
    uint8_t mFirstColumn;            // column of first visible glyph, past fold prefixes
    };
  //}}}

  cTextEdit();
  ~cTextEdit() = default;
  //{{{  gets
  bool isReadOnly() const { return mOptions.mReadOnly; }
  bool isTextEdited() const { return mInfo.mTextEdited; }
  bool isShowFolds() const { return mOptions.mShowFolded; }

  // has
  bool hasCR() const { return mInfo.mHasCR; }
  bool hasTabs() const { return mInfo.mHasTabs; }
  bool hasSelect() const { return mEdit.mState.mSelectionEnd > mEdit.mState.mSelectionBegin; }
  bool hasUndo() const { return !mOptions.mReadOnly && mUndoList.mIndex > 0; }
  bool hasRedo() const { return !mOptions.mReadOnly && mUndoList.mIndex < (int)mUndoList.mBuffer.size(); }
  bool hasClipboardText();

  // get
  std::string getTextString() const;
  std::vector<std::string> getTextStrings() const;
  int getTextNumLines() const { return (int)mInfo.mLines.size(); }

  uint32_t getTabSize() const { return mInfo.mTabSize; }
  sPosition getCursorPosition() const { return sanitizePosition (mEdit.mState.mCursorPosition); }

  const cLanguage& getLanguage() const { return mOptions.mLanguage; }
  //}}}
  //{{{  sets
  void setTextString (const std::string& text);
  void setTextStrings (const std::vector<std::string>& lines);

  void setPalette (bool lightPalette);
  void setLanguage (const cLanguage& language);

  void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }
  void setTabSize (int value) { mInfo.mTabSize = std::max (0, std::min (32, value)); }

  void toggleReadOnly() { mOptions.mReadOnly = !mOptions.mReadOnly; }
  void toggleOverWrite() { mOptions.mOverWrite = !mOptions.mOverWrite; }
  void toggleShowFolded() { mOptions.mShowFolded = !mOptions.mShowFolded; }
  void toggleShowLineNumber() { mOptions.mShowLineNumber = !mOptions.mShowLineNumber; }
  void toggleShowLineDebug() { mOptions.mShowLineDebug = !mOptions.mShowLineDebug; }
  void toggleShowWhiteSpace() { mOptions.mShowWhiteSpace = !mOptions.mShowWhiteSpace; }
  void toggleShowMonoSpaced() { mOptions.mShowMonoSpaced = !mOptions.mShowMonoSpaced; }
  //}}}
  //{{{  actions
  // move
  void moveLeft();
  void moveRight();

  void moveLineUp()   { moveUp (1); }
  void moveLineDown() { moveDown (1); }
  void movePageUp()   { moveUp (getPageNumLines() - 4); }
  void movePageDown() { moveDown (getPageNumLines() - 4); }
  void moveHome();
  void moveEnd();

  // select
  void selectAll();
  void selectWordUnderCursor();

  // cut and paste
  void copy();
  void cut();
  void paste();

  void deleteIt();
  void backspace();
  void deleteSelection();

  void undo (int steps = 1);
  void redo (int steps = 1);

  // fold
  void openFold();
  void closeFold();

  void enterCharacter (ImWchar ch, bool shift);
  //}}}

  void drawWindow (const std::string& title, cApp& app);
  void drawContents (cApp& app);

private:
  //{{{
  struct sCursorSelectionState {
    sPosition mCursorPosition;

    sPosition mSelectionBegin;
    sPosition mSelectionEnd;
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

    bool mHoverFolded = false;
    bool mHoverLineNumber = false;
    bool mHoverWhiteSpace = false;
    bool mHoverMonoSpaced = false;

    cLanguage mLanguage;
    };
  //}}}
  //{{{
  class cInfo {
  public:
    std::string mFilename;
    std::vector <cLine> mLines;
    std::vector <int> mFoldLines;

    bool mHasTabs = false;
    int mTabSize = 4;
    bool mHasFolds = false;
    bool mHasCR = false;

    bool mTextEdited = false;
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
    bool mCheckComments = true;

    sCursorSelectionState mState;

    eSelection mSelection = eSelection::eNormal;
    sPosition mInteractiveBegin;
    sPosition mInteractiveEnd;
    };
  //}}}
  //{{{
  class cUndo {
  public:
    cUndo() = default;
    cUndo (const std::string& added, const cTextEdit::sPosition addedBegin, const cTextEdit::sPosition addedEnd,
           const std::string& aRemoved, const cTextEdit::sPosition removedBegin, const cTextEdit::sPosition removedEnd,
           cTextEdit::sCursorSelectionState& before, cTextEdit::sCursorSelectionState& after);
    ~cUndo() = default;

    void undo (cTextEdit* textEdit);
    void redo (cTextEdit* textEdit);

    // vars
    std::string mAdded;
    sPosition mAddedBegin;
    sPosition mAddedEnd;

    std::string mRemoved;
    sPosition mRemovedBegin;
    sPosition mRemovedEnd;

    sCursorSelectionState mBefore;
    sCursorSelectionState mAfter;
    };
  //}}}
  //{{{
  class cUndoList {
  public:
    int mIndex = 0;
    std::vector <cUndo> mBuffer;
    };
  //}}}

  //{{{  gets
  bool isFolded() const { return mOptions.mShowFolded || mOptions.mHoverFolded; }
  bool isDrawLineNumber() const { return mOptions.mShowLineNumber || mOptions.mHoverLineNumber; }
  bool isDrawWhiteSpace() const { return mOptions.mShowWhiteSpace || mOptions.mHoverWhiteSpace; }
  bool isDrawMonoSpaced() const { return mOptions.mShowMonoSpaced ^ mOptions.mHoverMonoSpaced; }

  int getCharacterIndex (sPosition position) const;
  int getCharacterColumn (int lineNumber, int index) const;

  int getLineNumChars (int row) const;
  int getLineMaxColumn (int row) const;

  int getPageNumLines() const;
  int getMaxLineIndex() const;
  float getTextWidth (sPosition position) const;

  std::string getText (sPosition beginPosition, sPosition endPosition) const;
  std::string getSelectedText() const;

  bool isOnWordBoundary (sPosition position) const;
  std::string getWordAt (sPosition position) const;
  std::string getWordUnderCursor() const;

  int getTabColumn (int column) const;
  float getTabEndPosX (float columnX) const;
  sPosition getPositionFromPosX (int lineNumber, float posX) const;

  int getLineNumberFromIndex (int lineIndex) const;
  int getLineIndexFromNumber (int lineNumber) const;
  //}}}
  //{{{  sets
  void setCursorPosition (sPosition position);

  void setSelectionBegin (sPosition position);
  void setSelectionEnd (sPosition position);

  void setSelection (sPosition beginPosition, sPosition endPosition, eSelection mode);
  //}}}
  //{{{  utils
  void advance (sPosition& position) const;
  void scrollCursorVisible();
  sPosition sanitizePosition (sPosition position) const;

  // find
  sPosition findWordBegin (sPosition fromPosition) const;
  sPosition findWordEnd (sPosition fromPosition) const;
  sPosition findNextWord (sPosition fromPosition) const;

  // move
  void moveLeft (int amount, bool select , bool wordMode);
  void moveRight (int amount, bool select, bool wordMode);
  void moveUp (int amount);
  void moveDown (int amount);

  // insert
  cLine& insertLine (int index);
  void insertText (const char* value);
  void insertText (const std::string& value) { insertText (value.c_str()); }
  int insertTextAt (sPosition& position, const char* text);

  // delete
  void removeLine (int beginPosition, int endPosition);
  void removeLine (int index);
  void deleteRange (sPosition beginPosition, sPosition endPosition);

  // fold
  int skipFoldLines (int lineNumber);
  void closeFoldEnd (int lineNumber);

  // undo
  void addUndo (cUndo& undo);
  //}}}

  // parse
  void parseTokens (cLine& line, const std::string& textString);
  void parseLine (cLine& line);
  void parseComments();

  // mouse
  void clickLine (int lineNumber);
  void clickFold (int lineNumber, bool foldOpen);
  void clickText (int lineNumber, float posX, bool selectWord);
  void dragLine (int lineNumber, float posY);
  void dragText (int lineNumber, ImVec2 pos);

  // draws
  void drawTop (cApp& app);
  float drawGlyphs (ImVec2 pos, const tGlyphs& glyphs, uint8_t firstGlyph, uint8_t forceColor);
  void drawLine (int lineNumber, int lineIndex);
  void drawUnfolded();
  int drawFolded (int lineNumber, bool onlyFold);

  void handleKeyboard();

  //{{{  vars
  bool mOpen = true;  // set false when DrawWindow() closed

  cOptions mOptions;
  cInfo mInfo;
  cContext mContext;
  cEdit mEdit;
  cUndoList mUndoList;

  std::chrono::system_clock::time_point mCursorFlashTimePoint;
  //}}}
  };
