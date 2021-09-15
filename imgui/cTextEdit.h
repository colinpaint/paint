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

struct ImFont;
//}}}

class cTextEdit {
public:
  enum class eSelection { Normal, Word, Line };
  //{{{
  enum class ePalette {
    Default,
    Keyword,
    Number,
    String,
    CharLiteral,
    Punctuation,
    Preprocessor,
    Ident,
    KnownIdent,
    PreprocIdent,
    Comment,
    MultiLineComment,
    Background,
    Cursor,
    Selection,
    Marker,
    LineNumber,
    CurrentLineFill,
    CurrentLineFillInactive,
    CurrentLineEdge,
    WhiteSpace,
    Tab,
    FoldBeginClosed,
    FoldBeginOpen,
    FoldEnd,
    Max
    };
  //}}}
  //{{{
  struct sGlyph {
    uint8_t mChar;
    ePalette mColorIndex;

    bool mComment:1;
    bool mMultiLineComment:1;
    bool mPreProc:1;

    sGlyph() :
      mChar(' '),
      mColorIndex(ePalette::Default),
      mComment(false), mMultiLineComment(false), mPreProc(false) {}

    sGlyph (uint8_t ch, ePalette colorIndex) :
      mChar(ch),
      mColorIndex(colorIndex),
      mComment(false), mMultiLineComment(false), mPreProc(false) {}
    };
  //}}}
  //{{{
  struct sLine {
    std::vector <sGlyph> mGlyphs;

    int mFoldTitleLineNumber; // closed foldTitle glyphs lineNumber
    int16_t mIndent;

    bool mFoldBegin:1;
    bool mFoldEnd:1;
    bool mFoldOpen:1;
    bool mHasComment:1;

    sLine() :
      mGlyphs(), mFoldTitleLineNumber(0), mIndent(0),
      mFoldBegin(false), mFoldEnd(false), mFoldOpen(false), mHasComment(false) {}

    sLine (const std::vector<sGlyph>& line) :
      mGlyphs(line), mFoldTitleLineNumber(0), mIndent(0),
      mFoldBegin(false), mFoldEnd(false), mFoldOpen(false), mHasComment(false) {}
    };
  //}}}
  //{{{
  struct sPosition {
    int mLineNumber;
    int mColumn;

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
    };
  //}}}
  //{{{
  struct sIdent {
    sPosition mLocation;
    std::string mDeclaration;
    };
  //}}}
  //{{{
  struct sLanguage {
    // typedef
    typedef std::pair<std::string, ePalette> tTokenRegexString;
    typedef std::vector<tTokenRegexString> tTokenRegexStrings;
    typedef bool (*tTokenizeCallback)(const char* inBegin, const char* inEnd,
                                      const char*& outBegin, const char*& outEnd, ePalette& palette);

    // vars
    std::string mName;

    std::unordered_set <std::string> mKeywords;
    std::unordered_map <std::string,sIdent> mIdents;
    std::unordered_map <std::string,sIdent> mPreprocIdents;

    std::string mCommentStart;
    std::string mCommentEnd;
    std::string mSingleLineComment;
    std::string mFoldBeginMarker;
    std::string mFoldEndMarker;
    std::string mFoldBeginOpen;
    std::string mFoldBeginClosed;
    std::string mFoldEnd;

    char mPreprocChar;
    bool mAutoIndentation;

    tTokenizeCallback mTokenize;
    tTokenRegexStrings mTokenRegexStrings;

    bool mCaseSensitive;

    // init
    sLanguage() : mPreprocChar('#'), mAutoIndentation(true), mTokenize(nullptr), mCaseSensitive(true) {}

    // static const
    static const sLanguage& cPlus();
    static const sLanguage& c();
    static const sLanguage& hlsl();
    static const sLanguage& glsl();
    };
  //}}}

  cTextEdit();
  ~cTextEdit() = default;
  //{{{  gets
  bool isReadOnly() const { return mOptions.mReadOnly; }
  bool isOverwrite() const { return mOptions.mOverwrite; }

  bool isShowFolds() const { return mOptions.mShowFolded; }
  bool isShowLineNumbers() const { return mOptions.mShowLineNumbers; }
  bool isShowDebug() const { return mOptions.mShowDebug; }
  bool isShowWhiteSpace() const { return mOptions.mShowWhiteSpace; }
  bool isShowMonoSpace() const { return mOptions.mShowMonoSpace; }

  bool isTextEdited() const { return mInfo.mTextEdited; }

  // has
  bool hasSelect() const { return mEdit.mState.mSelectionEnd > mEdit.mState.mSelectionStart; }
  bool hasClipboardText();
  bool hasUndo() const { return !mOptions.mReadOnly && mUndoList.mIndex > 0; }
  bool hasRedo() const { return !mOptions.mReadOnly && mUndoList.mIndex < (int)mUndoList.mBuffer.size(); }
  bool hasTabs() const { return mInfo.mHasTabs; }
  bool hasFolds() const { return mInfo.mHasFolds; }
  bool hasCR() const { return mInfo.mHasCR; }

  // get
  std::string getTextString() const;
  std::vector<std::string> getTextStrings() const;
  int getTextNumLines() const { return (int)mInfo.mLines.size(); }

  uint32_t getTabSize() const { return mInfo.mTabSize; }
  sPosition getCursorPosition() const { return sanitizePosition (mEdit.mState.mCursorPosition); }

  const sLanguage& getLanguage() const { return mOptions.mLanguage; }
  //}}}
  //{{{  sets
  void setTextString (const std::string& text);
  void setTextStrings (const std::vector<std::string>& lines);

  void setPalette (bool lightPalette);
  void setMarkers (const std::map<int,std::string>& markers) { mOptions.mMarkers = markers; }
  void setLanguage (const sLanguage& language);

  void setTabSize (int value) { mInfo.mTabSize = std::max (0, std::min (32, value)); }
  void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }

  void setShowFolded (bool showFolds) { mOptions.mShowFolded = showFolds; }
  void setShowDebug (bool showDebug) { mOptions.mShowDebug = showDebug; }
  void setShowLineNumbers (bool showLineNumbers) { mOptions.mShowLineNumbers = showLineNumbers; }
  void setShowWhiteSpace (bool showWhiteSpace) { mOptions.mShowWhiteSpace = showWhiteSpace; }
  void setShowMonoSpace (bool showMonoSpace) { mOptions.mShowMonoSpace = showMonoSpace; }

  void setCursorPosition (const sPosition& position);
  void setSelectionStart (const sPosition& position);
  void setSelectionEnd (const sPosition& position);
  void setSelection (const sPosition& startPosition, const sPosition& endPosition, eSelection mode = eSelection::Normal);

  void toggleReadOnly() { mOptions.mReadOnly = !mOptions.mReadOnly; }
  void toggleOverwrite() { mOptions.mOverwrite = !mOptions.mOverwrite; }
  void toggleShowFolded() { mOptions.mShowFolded = !mOptions.mShowFolded; }
  void toggleShowLineNumbers() { mOptions.mShowLineNumbers = !mOptions.mShowLineNumbers; }
  void toggleShowDebug() { mOptions.mShowDebug = !mOptions.mShowDebug; }
  void toggleShowWhiteSpace() { mOptions.mShowWhiteSpace = !mOptions.mShowWhiteSpace; }
  void toggleShowMonoSpace() { mOptions.mShowMonoSpace = !mOptions.mShowMonoSpace; }
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

  void drawWindow (const std::string& title, ImFont* monoFont);
  void drawContents (ImFont* monoFont);

private:
  typedef std::vector <std::pair <std::regex,ePalette>> tRegexList;
  //{{{
  struct sCursorSelectionState {
    sPosition mCursorPosition;

    sPosition mSelectionStart;
    sPosition mSelectionEnd;
    };
  //}}}
  //{{{
  class cOptions {
  public:
    int mFontSize = 16;
    int mMinFontSize = 4;
    int mMaxFontSize = 32;

    // modes
    bool mOverwrite = false;
    bool mReadOnly = false;
    bool mCheckComments = true;

    // shows
    bool mShowFolded = false;
    bool mShowLineNumbers = true;
    bool mShowDebug = true;
    bool mShowWhiteSpace = false;
    bool mShowMonoSpace = true;

    sLanguage mLanguage;
    tRegexList mRegexList;
    std::map <int,std::string> mMarkers;

    std::array <ImU32,(size_t)ePalette::Max> mPalette;
    std::array <ImU32,(size_t)ePalette::Max> mPaletteBase;
    };
  //}}}
  //{{{
  class cInfo {
  public:
    std::string mFilename;
    std::vector <sLine> mLines;
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

    float measureText (const char* str, const char* strEnd);
    float drawText (ImVec2 pos, ImU32 color, const char* str, const char* strEnd);

    ImDrawList* mDrawList = nullptr;
    bool mFocused = false;

    float mFontAtlasSize = 0.f;
    float mFontSize = 0.f;

    float mLineHeight = 0.f;
    float mGlyphWidth = 0.f;

    float mPadding = 0.f;
    float mTextBegin = 0.f;

  private:
    ImFont* mFont = nullptr;
    };
  //}}}
  //{{{
  class cEdit {
  public:
    int mColorRangeMin = 0;
    int mColorRangeMax = 0;

    sPosition mInteractiveStart;
    sPosition mInteractiveEnd;
    eSelection mSelection = eSelection::Normal;

    sCursorSelectionState mState;
    };
  //}}}
  //{{{
  class sUndo {
  public:
    sUndo() {}
    ~sUndo() {}

    sUndo (const std::string& added, const cTextEdit::sPosition addedStart, const cTextEdit::sPosition addedEnd,
           const std::string& aRemoved, const cTextEdit::sPosition removedStart, const cTextEdit::sPosition removedEnd,
           cTextEdit::sCursorSelectionState& before, cTextEdit::sCursorSelectionState& after);

    void undo (cTextEdit* textEdit);
    void redo (cTextEdit* textEdit);

    // vars
    std::string mAdded;
    sPosition mAddedStart;
    sPosition mAddedEnd;

    std::string mRemoved;
    sPosition mRemovedStart;
    sPosition mRemovedEnd;

    sCursorSelectionState mBefore;
    sCursorSelectionState mAfter;
    };
  //}}}
  //{{{
  class cUndoList {
  public:
    int mIndex = 0;
    std::vector <sUndo> mBuffer;
    };
  //}}}

  //{{{  gets
  bool isOnWordBoundary (const sPosition& position) const;

  std::string getSelectedText() const { return getText (mEdit.mState.mSelectionStart, mEdit.mState.mSelectionEnd); }

  int getCharacterIndex (const sPosition& position) const;
  int getCharacterColumn (int lineNumber, int index) const;

  int getLineNumChars (int row) const;
  int getLineMaxColumn (int row) const;

  std::string getText (const sPosition& startPosition, const sPosition& endPosition) const;
  std::string getCurrentLineText() const;
  ImU32 getGlyphColor (const sGlyph& glyph) const;

  std::string getWordAt (const sPosition& position) const;
  std::string getWordUnderCursor() const;

  float getTextWidth (const sPosition& position);
  int getPageNumLines() const;
  int getMaxLineIndex() const;
  //}}}
  //{{{  utils
  float tabEndPos (float columnX);
  void clickCursor (int lineNumber, float xPos, bool selectWord);
  sPosition xPosToPosition (int lineNumber, float xPos);

  void advance (sPosition& position) const;
  sPosition sanitizePosition (const sPosition& position) const;
  int lineIndexToNumber (int lineIndex) const;
  int lineNumberToIndex (int lineNumber) const;
  void ensureCursorVisible();

  // find
  sPosition findWordStart (const sPosition& from) const;
  sPosition findWordEnd (const sPosition& from) const;
  sPosition findNextWord (const sPosition& from) const;

  // move
  void moveLeft (int amount, bool select , bool wordMode);
  void moveRight (int amount, bool select, bool wordMode);
  void moveUp (int amount);
  void moveDown (int amount);

  // insert
  std::vector<sGlyph>& insertLine (int index);
  void insertText (const char* value);
  void insertText (const std::string& value) { insertText (value.c_str()); }
  int insertTextAt (sPosition& where, const char* value);

  // delete
  void removeLine (int startPosition, int endPosition);
  void removeLine (int index);
  void deleteRange (const sPosition& startPosition, const sPosition& endPosition);

  // undo
  void addUndo (sUndo& value);

  // colorize
  void colorize (int fromLine = 0, int count = -1);
  void colorizeRange (int fromLine = 0, int toLine = 0);
  void colorizeInternal();
  //}}}

  // fold
  void parseFolds();

  void handleMouseInputs();
  void handleKeyboardInputs();

  void drawTop();
  float drawGlyphs (ImVec2 pos, const std::vector <sGlyph>& glyphs, bool forceColor, ImU32 forcedColor);
  void drawLine (int lineNumber, int beginFoldLineNumber);
  int drawFold (int lineNumber, bool parentOpen, bool foldOpen);

  //{{{  vars
  bool mOpen = true;  // set false when DrawWindow() closed

  cOptions mOptions;
  cInfo mInfo;
  cContext mContext;
  cEdit mEdit;
  cUndoList mUndoList;

  float mLastClickTime = -1;
  std::chrono::system_clock::time_point mLastFlashTime;
  //}}}
  };
