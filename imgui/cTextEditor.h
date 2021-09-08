// cTextEditor.h
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

#include "imgui.h"

struct ImFont;
//}}}

class cTextEditor {
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

    uint32_t mFoldLineNumber; // foldBegin lineNumber, except for foldBegin which is set to foldEnd lineNumber
    uint32_t mFoldTitleLineNumber; // lineNumber for foldTitle
    uint16_t mIndent;

    bool mFoldBegin:1;
    bool mFoldEnd:1;
    bool mFoldOpen:1;
    bool mHasComment:1;

    sLine() :
      mGlyphs(), mFoldLineNumber(0), mFoldTitleLineNumber(0), mIndent(0),
      mFoldBegin(false), mFoldEnd(false), mFoldOpen(false), mHasComment(false) {}

    sLine (const std::vector<sGlyph>& line) :
      mGlyphs(line), mFoldLineNumber(0), mFoldTitleLineNumber(0), mIndent(0),
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

  cTextEditor();
  ~cTextEditor() = default;
  //{{{  gets
  bool isReadOnly() const { return mReadOnly; }
  bool isOverwrite() const { return mOverwrite; }
  bool isTextChanged() const { return mTextChanged; }
  bool isCursorPositionChanged() const { return mCursorPositionChanged; }

  bool isShowFolds() const { return mShowFolds; }
  bool isShowLineNumbers() const { return mShowLineNumbers; }
  bool isShowLineDebug() const { return mShowLineDebug; }
  bool isShowWhiteSpace() const { return mShowWhiteSpace; }

  bool hasSelect() const { return mState.mSelectionEnd > mState.mSelectionStart; }
  bool hasClipboardText();
  bool hasUndo() const { return !mReadOnly && mUndoIndex > 0; }
  bool hasRedo() const { return !mReadOnly && mUndoIndex < (int)mUndoBuffer.size(); }
  bool hasTabs() const { return mHasTabs; }
  bool hasFolds() const { return mHasFolds; }
  bool hasCR() const { return mHasCR; }

  bool isImGuiChildIgnored() const { return mIgnoreImGuiChild; }

  std::string getTextString() const;
  std::vector<std::string> getTextStrings() const;
  int getTextNumLines() const { return (int)mLines.size(); }

  uint32_t getTabSize() const { return mTabSize; }
  sPosition getCursorPosition() const { return sanitizePosition (mState.mCursorPosition); }

  const sLanguage& getLanguage() const { return mLanguage; }
  //}}}
  //{{{  sets
  void setTextString (const std::string& text);
  void setTextStrings (const std::vector<std::string>& lines);

  void setTabSize (int value) { mTabSize = std::max (0, std::min (32, value)); }
  void setReadOnly (bool readOnly) { mReadOnly = readOnly; }

  void setShowFolds (bool showFolds) { mShowFolds = showFolds; }
  void setShowLineDebug (bool showLineDebug) { mShowLineDebug = showLineDebug; }
  void setShowLineNumbers (bool showLineNumbers) { mShowLineNumbers = showLineNumbers; }
  void setShowWhiteSpace (bool showWhiteSpace) { mShowWhiteSpace = showWhiteSpace; }

  void setPalette (bool lightPalette);
  void setMarkers (const std::map<int,std::string>& markers) { mMarkers = markers; }
  void setLanguage (const sLanguage& language);

  void setCursorPosition (const sPosition& position);
  void setSelectionStart (const sPosition& position);
  void setSelectionEnd (const sPosition& position);
  void setSelection (const sPosition& startPosition, const sPosition& endPosition, eSelection mode = eSelection::Normal);

  void toggleReadOnly() { mReadOnly = !mReadOnly; }
  void toggleOverwrite() { mOverwrite = !mOverwrite; }
  void toggleShowFolds() { mShowFolds = !mShowFolds; }
  void toggleShowLineNumbers() { mShowLineNumbers = !mShowLineNumbers; }
  void toggleShowLineDebug() { mShowLineDebug = !mShowLineDebug; }
  void toggleShowWhiteSpace() { mShowWhiteSpace = !mShowWhiteSpace; }
  //}}}
  //{{{  actions
  // move
  void moveLeft() { moveLeft (1, false, false); }
  void moveLeftSelect() { moveLeft (1, true, false); }
  void moveLeftWord() { moveLeft (1, false, true); }
  void moveLeftWordSelect() { moveLeft (1, true, true); }
  void moveRight() { moveRight (1, false, false); }
  void moveRightSelect() { moveRight (1, true, false); }
  void moveRightWord() { moveRight (1, false, true); }
  void moveRightWordSelect() { moveRight (1, true, true); }

  void moveLineUp() { moveUp (1); }
  void moveLineUpSelect() { moveUpSelect (1); }
  void moveLineDown() { moveDown (1); }
  void moveLineDownSelect() { moveDownSelect (1); }
  void movePageUp() { moveUp (getPageNumLines() - 4); }
  void movePageUpSelect() { moveUpSelect (getPageNumLines() - 4); }
  void movePageDown() { moveDown (getPageNumLines() - 4); }
  void movePageDownSelect() { moveDownSelect (getPageNumLines() - 4); }

  void moveTop();
  void moveTopSelect();
  void moveBottom();
  void moveBottomSelect();

  void moveHome();
  void moveHomeSelect();
  void moveEnd();
  void moveEndSelect();

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
  void render (const std::string& title, const ImVec2& size = ImVec2(), bool border = false);

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

    sUndo (const std::string& added, const cTextEditor::sPosition addedStart, const cTextEditor::sPosition addedEnd,
           const std::string& aRemoved, const cTextEditor::sPosition removedStart, const cTextEditor::sPosition removedEnd,
           cTextEditor::sCursorSelectionState& before, cTextEditor::sCursorSelectionState& after);

    void undo (cTextEditor* editor);
    void redo (cTextEditor* editor);

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

  typedef std::vector<sUndo> tUndoBuffer;
  //{{{  gets
  bool isOnWordBoundary (const sPosition& position) const;

  std::string getSelectedText() const { return getText (mState.mSelectionStart, mState.mSelectionEnd); }

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
  //}}}
  //{{{  utils
  void ensureCursorVisible();

  void advance (sPosition& position) const;
  sPosition screenToPosition (const ImVec2& pos) const;
  sPosition sanitizePosition (const sPosition& position) const;

  // find
  sPosition findWordStart (const sPosition& from) const;
  sPosition findWordEnd (const sPosition& from) const;
  sPosition findNextWord (const sPosition& from) const;

  // move
  void moveLeft (int amount, bool select , bool wordMode);
  void moveRight (int amount, bool select, bool wordMode);
  void moveUp (int amount);
  void moveUpSelect (int amount);
  void moveDown (int amount);
  void moveDownSelect( int amount);

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

  void preRender (uint32_t& minLineIndex, uint32_t& maxLineIndex);
  void renderText (const std::vector <sGlyph>& glyphs, bool forceColor, ImU32 forcedColor);
  void renderLine (uint32_t lineNumber, uint32_t beginFoldLineNumber);
  void renderFold (std::vector<sLine>::iterator& it, uint32_t& lineNumber, uint32_t& lineIndex,
                   uint32_t minLineIndex, uint32_t maxLineIndex, bool parentOpen, bool foldOpen);
  void postRender();

  //{{{  vars
  std::vector <sLine> mLines;
  std::vector <uint32_t> mVisibleLines;

  // config
  sLanguage mLanguage;
  tRegexList mRegexList;
  std::map <int,std::string> mMarkers;

  std::array <ImU32,(size_t)ePalette::Max> mPalette;
  std::array <ImU32,(size_t)ePalette::Max> mPaletteBase;

  bool mHasTabs = false;
  int mTabSize;
  bool mHasFolds = false;
  bool mHasCR = false;

  // changed flags
  bool mTextChanged;
  bool mCursorPositionChanged;

  // modes
  bool mOverwrite;
  bool mReadOnly;
  bool mIgnoreImGuiChild;
  bool mCheckComments;

  // shows
  bool mShowFolds;
  bool mShowLineNumbers;
  bool mShowLineDebug;
  bool mShowWhiteSpace;

  // range
  int mColorRangeMin;
  int mColorRangeMax;
  sPosition mInteractiveStart;
  sPosition mInteractiveEnd;
  eSelection mSelection;

  // undo
  int mUndoIndex;
  tUndoBuffer mUndoBuffer;

  // internal
  sCursorSelectionState mState;
  float mLineSpacing;
  bool mWithinRender;
  bool mScrollToTop;
  bool mScrollToCursor;

  uint64_t mStartTime;
  float mLastClick;
  //}}}
  //{{{  render context vars
  ImFont* mFont = nullptr;
  float mFontSize = 0.f;
  float mFontHalfSize = 0.f;
  ImDrawList* mDrawList = nullptr;
  ImVec2 mCursorScreenPos;
  bool mFocused = false;

  float mGlyphsStart = 0.f;  // start x of main text
  float mMaxLineWidth = 0.f; // width of widest line, used to set scroll region
  ImVec2 mCharSize;          // size of character grid, space wide, fontHeight high
  ImVec2 mCursorPos;         // start pos of line cursor bgnd
  ImVec2 mLinePos;           // start pos of lineNumber
  ImVec2 mTextPos;           // running pos of glyphs
  //}}}
  };
