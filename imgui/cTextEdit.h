// cTextEdit.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <memory>
#include <regex>

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

  bool isTextEdited() const { return mInfo.mTextEdited; }

  bool isShowFolds() const { return mOptions.mShowFolds; }
  bool isShowLineNumbers() const { return mOptions.mShowLineNumbers; }
  bool isShowDebug() const { return mOptions.mShowDebug; }
  bool isShowWhiteSpace() const { return mOptions.mShowWhiteSpace; }

  // has
  bool hasSelect() const { return mEdit.mState.mSelectionEnd > mEdit.mState.mSelectionStart; }
  bool hasClipboardText();
  bool hasUndo() const { return !mOptions.mReadOnly && mUndoList.mUndoIndex > 0; }
  bool hasRedo() const { return !mOptions.mReadOnly && mUndoList.mUndoIndex < (int)mUndoList.mUndoBuffer.size(); }
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

  std::string getDebugString() { return mDebugString; }
  //}}}
  //{{{  sets
  void setTextString (const std::string& text);
  void setTextStrings (const std::vector<std::string>& lines);

  void setPalette (bool lightPalette);
  void setMarkers (const std::map<int,std::string>& markers) { mOptions.mMarkers = markers; }
  void setLanguage (const sLanguage& language);

  void setTabSize (int value) { mInfo.mTabSize = std::max (0, std::min (32, value)); }
  void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }

  void setShowFolds (bool showFolds) { mOptions.mShowFolds = showFolds; }
  void setShowDebug (bool showDebug) { mOptions.mShowDebug = showDebug; }
  void setShowLineNumbers (bool showLineNumbers) { mOptions.mShowLineNumbers = showLineNumbers; }
  void setShowWhiteSpace (bool showWhiteSpace) { mOptions.mShowWhiteSpace = showWhiteSpace; }

  void setCursorPosition (const sPosition& position);
  void setSelectionStart (const sPosition& position);
  void setSelectionEnd (const sPosition& position);
  void setSelection (const sPosition& startPosition, const sPosition& endPosition, eSelection mode = eSelection::Normal);

  void toggleReadOnly() { mOptions.mReadOnly = !mOptions.mReadOnly; }
  void toggleOverwrite() { mOptions.mOverwrite = !mOptions.mOverwrite; }
  void toggleShowFolds() { mOptions.mShowFolds = !mOptions.mShowFolds; }
  void toggleShowLineNumbers() { mOptions.mShowLineNumbers = !mOptions.mShowLineNumbers; }
  void toggleShowDebug() { mOptions.mShowDebug = !mOptions.mShowDebug; }
  void toggleShowWhiteSpace() { mOptions.mShowWhiteSpace = !mOptions.mShowWhiteSpace; }
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

  void drawWindow (const std::string& title);
  void drawContents();

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
  class sUndo {
  public:
    sUndo() {}
    ~sUndo() {}

    sUndo (const std::string& added, const cTextEdit::sPosition addedStart, const cTextEdit::sPosition addedEnd,
           const std::string& aRemoved, const cTextEdit::sPosition removedStart, const cTextEdit::sPosition removedEnd,
           cTextEdit::sCursorSelectionState& before, cTextEdit::sCursorSelectionState& after);

    void undo (cTextEdit* editor);
    void redo (cTextEdit* editor);

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
  class cOptions {
  public:
    // modes
    bool mOverwrite = false;
    bool mReadOnly = false;
    bool mCheckComments = true;

    // shows
    bool mShowFolds = false;
    bool mShowLineNumbers = true;
    bool mShowDebug = true;
    bool mShowWhiteSpace = false;

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
    ImFont* mFont = nullptr;
    float mFontSize = 0.f;
    float mFontHalfSize = 0.f;
    ImDrawList* mDrawList = nullptr;
    ImVec2 mCursorScreenPos;
    bool mFocused = false;

    ImVec2 mCharSize;          // size of character grid, space wide, fontHeight high
    float mGlyphsOffset = 0.f; // start offset of glyphs
    float mMaxLineWidth = 0.f; // width of widest line, used to set scroll region
    ImVec2 mLineBeginPos;      // start pos of line cursor bgnd
    ImVec2 mGlyphsPos;         // running pos of glyphs
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

  typedef std::vector<sUndo> tUndoBuffer;
  //{{{
  class cUndoList {
  public:
    int mUndoIndex = 0;
    tUndoBuffer mUndoBuffer;
    };
  //}}}

  //{{{  gets
  bool isOnWordBoundary (const sPosition& position) const;

  std::string getSelectedText() const { return getText (mEdit.mState.mSelectionStart, mEdit.mState.mSelectionEnd); }

  int getCharacterIndex (const sPosition& position) const;
  int getCharacterColumn (int lineNumber, int index) const;

  int getLineCharacterCount (int row) const;
  int getLineMaxColumn (int row) const;

  std::string getText (const sPosition& startPosition, const sPosition& endPosition) const;
  ImU32 getGlyphColor (const sGlyph& glyph) const;

  std::string getCurrentLineText() const;

  std::string getWordAt (const sPosition& position) const;
  std::string getWordUnderCursor() const;

  float getTextWidth (const sPosition& position) const;
  int getPageNumLines() const;
  int getMaxLineIndex() const;
  //}}}
  //{{{  utils
  void advance (sPosition& position) const;
  sPosition screenToPosition (const ImVec2& pos) const;
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

  void preRender();
  void renderGlyphs (const std::vector <sGlyph>& glyphs, bool forceColor, ImU32 forcedColor);
  void renderLine (int lineNumber, int beginFoldLineNumber);
  int renderFold (int lineNumber, bool parentOpen, bool foldOpen);
  void postRender();

  //{{{  vars
  bool mOpen = true;  // set false when DrawWindow() closed

  cOptions mOptions;
  cInfo mInfo;
  cContext mContext;
  cEdit mEdit;
  cUndoList mUndoList;

  uint64_t mStartTime;
  float mLastClick;
  std::string mDebugString;
  //}}}
  };
