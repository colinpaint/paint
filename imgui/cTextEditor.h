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
    Max
    };
  //}}}
  //{{{
  struct sGlyph {
    uint8_t mChar;
    ePalette mColorIndex = ePalette::Default;
    bool mComment : 1;
    bool mMultiLineComment : 1;
    bool mPreprocessor : 1;

    sGlyph (uint8_t ch, ePalette colorIndex)
      : mChar(ch), mColorIndex(colorIndex), mComment(false), mMultiLineComment(false), mPreprocessor(false) {}
    };
  //}}}
  //{{{
  struct sLine {
    std::vector<sGlyph> mGlyphs;
    uint32_t mLineNumber;
    uint8_t mFoldLevel;
    bool mFoldStart : 1;
    bool mFoldEnd : 1;
    bool mFoldOpen : 1;

    sLine() : mGlyphs(), mLineNumber(0), mFoldLevel(0), mFoldStart(false), mFoldEnd(false), mFoldOpen(false) {}
    sLine (const std::vector<sGlyph>& line) :
      mGlyphs(line), mLineNumber(0), mFoldLevel(0), mFoldStart(false), mFoldEnd(false), mFoldOpen(false) {}
    };
  //}}}
  //{{{
  struct sPosition {
  // pos in screen co-ords
    int mRow;
    int mColumn;

    sPosition() : mRow(0), mColumn(0) {}
    //{{{
    sPosition (int row, int column) : mRow(row), mColumn(column) {
      assert (line >= 0);
      assert (column >= 0);
      }
    //}}}

    //{{{
    bool operator == (const sPosition& o) const {
      return mRow == o.mRow && mColumn == o.mColumn;
      }
    //}}}
    //{{{
    bool operator != (const sPosition& o) const {
      return mRow != o.mRow || mColumn != o.mColumn; }
    //}}}
    //{{{
    bool operator < (const sPosition& o) const {

      if (mRow != o.mRow)
        return mRow < o.mRow;

      return mColumn < o.mColumn;
      }
    //}}}
    //{{{
    bool operator > (const sPosition& o) const {
      if (mRow != o.mRow)
        return mRow > o.mRow;
      return mColumn > o.mColumn;
      }
    //}}}
    //{{{
    bool operator <= (const sPosition& o) const {
      if (mRow != o.mRow)
        return mRow < o.mRow;

      return mColumn <= o.mColumn;
      }
    //}}}
    //{{{
    bool operator >= (const sPosition& o) const {
      if (mRow != o.mRow)
        return mRow > o.mRow;
      return mColumn >= o.mColumn;
      }
    //}}}
    };
  //}}}
  //{{{
  struct sLineColumn {
  // pos in file co-ords

    int mLineNumber;
    int mColumn;

    sLineColumn() : mLineNumber(0), mColumn(0) {}
    //{{{
    sLineColumn (int lineNumber, int column) : mLineNumber(lineNumber), mColumn(column) {
      assert (lineNumber >= 0);
      assert (column >= 0);
      }
    //}}}

    //{{{
    bool operator == (const sLineColumn& o) const {
      return mLineNumber == o.mLineNumber && mColumn == o.mColumn;
      }
    //}}}
    //{{{
    bool operator != (const sLineColumn& o) const {
      return mLineNumber != o.mLineNumber || mColumn != o.mColumn; }
    //}}}
    //{{{
    bool operator < (const sLineColumn& o) const {

      if (mLineNumber != o.mLineNumber)
        return mLineNumber < o.mLineNumber;

      return mColumn < o.mColumn;
      }
    //}}}
    //{{{
    bool operator > (const sLineColumn& o) const {
      if (mLineNumber != o.mLineNumber)
        return mLineNumber > o.mLineNumber;
      return mColumn > o.mColumn;
      }
    //}}}
    //{{{
    bool operator <= (const sLineColumn& o) const {
      if (mLineNumber != o.mLineNumber)
        return mLineNumber < o.mLineNumber;

      return mColumn <= o.mColumn;
      }
    //}}}
    //{{{
    bool operator >= (const sLineColumn& o) const {
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

    std::unordered_set<std::string> mKeywords;
    std::unordered_map<std::string,sIdent> mIdents;
    std::unordered_map<std::string,sIdent> mPreprocIdents;

    std::string mCommentStart;
    std::string mCommentEnd;
    std::string mSingleLineComment;

    char mPreprocChar;
    bool mAutoIndentation;

    tTokenizeCallback mTokenize;
    tTokenRegexStrings mTokenRegexStrings;

    bool mCaseSensitive;

    // init
    sLanguage() : mPreprocChar('#'), mAutoIndentation(true), mTokenize(nullptr), mCaseSensitive(true) {}

    // static const
    static const sLanguage& CPlusPlus();
    static const sLanguage& C();
    static const sLanguage& HLSL();
    static const sLanguage& GLSL();
    };
  //}}}

  cTextEditor();
  ~cTextEditor() = default;
  //{{{  gets
  bool isOverwrite() const { return mOverwrite; }
  bool isReadOnly() const { return mReadOnly; }
  bool isTextChanged() const { return mTextChanged; }
  bool isCursorPositionChanged() const { return mCursorPositionChanged; }

  bool hasSelection() const { return mState.mSelectionEnd > mState.mSelectionStart; }

  inline bool isHandleMouseInputsEnabled() const { return mHandleKeyboardInputs; }
  inline bool isHandleKeyboardInputsEnabled() const { return mHandleKeyboardInputs; }
  inline bool isImGuiChildIgnored() const { return mIgnoreImGuiChild; }
  inline bool isShowingWhitespaces() const { return mShowWhitespaces; }

  const sLanguage& getLanguage() const { return mLanguage; }
  const std::array<ImU32, (size_t)ePalette::Max>& getPalette() const { return mPaletteBase; }

  std::string getText() const;
  std::vector<std::string> getTextLines() const;
  std::string getSelectedText() const { return getText (mState.mSelectionStart, mState.mSelectionEnd); }

  std::string getCurrentLineText()const;
  int getTotalLines() const { return (int)mLines.size(); }

  sPosition getCursorPosition() const { return getActualCursorposition(); }

  inline int getTabSize() const { return mTabSize; }
  //}}}
  //{{{  sets
  void setMarkers (const std::map<int,std::string>& markers) { mMarkers = markers; }
  void setLanguage (const sLanguage& language);
  void setPalette (bool lightPalette);

  void setText (const std::string& text);
  void setTextLines (const std::vector<std::string>& lines);

  void setReadOnly (bool value) { mReadOnly = value; }

  void setCursorPosition (const sPosition& position);
  void setTabSize (int value) { mTabSize = std::max (0, std::min (32, value)); }
  void setSelectionStart (const sPosition& position);
  void setSelectionEnd (const sPosition& position);
  void setSelection (const sPosition& startPosition, const sPosition& endPosition, eSelection mode = eSelection::Normal);

  inline void setHandleMouseInputs (bool value) { mHandleMouseInputs = value;}
  inline void setHandleKeyboardInputs (bool value) { mHandleKeyboardInputs = value;}
  inline void setImGuiChildIgnored (bool value) { mIgnoreImGuiChild = value;}
  inline void setShowWhitespaces(bool value) { mShowWhitespaces = value; }
  //}}}
  //{{{  actions
  void moveUp (int amount = 1, bool select = false);
  void moveDown (int amount = 1, bool select = false);
  void moveLeft (int amount = 1, bool select = false, bool wordMode = false);
  void moveRight (int amount = 1, bool select = false, bool wordMode = false);

  void moveTop (bool select = false);
  void moveBottom (bool select = false);
  void moveHome (bool select = false);
  void moveEnd (bool select = false);

  void selectAll();
  void selectWordUnderCursor();

  void insertText(const char* value);
  void insertText (const std::string& value) { insertText (value.c_str()); }
  void copy();
  void cut();
  void paste();
  void deleteIt();

  bool canUndo() const { return !mReadOnly && mUndoIndex > 0; }
  bool canRedo() const { return !mReadOnly && mUndoIndex < (int)mUndoBuffer.size(); }
  void undo (int steps = 1);
  void redo (int steps = 1);

  void render (const std::string& title, const ImVec2& size = ImVec2(), bool border = false);
  //}}}

private:
  typedef std::vector<std::pair<std::regex,ePalette>> tRegexList;
  //{{{
  struct sEditorState {
    sPosition mSelectionStart;
    sPosition mSelectionEnd;
    sPosition mCursorPosition;
    };
  //}}}
  //{{{
  class sUndoRecord {
  public:
    sUndoRecord() {}
    ~sUndoRecord() {}

    sUndoRecord (
      const std::string& added,
      const cTextEditor::sPosition addedStart,
      const cTextEditor::sPosition addedEnd,

      const std::string& aRemoved,
      const cTextEditor::sPosition removedStart,
      const cTextEditor::sPosition removedEnd,

      cTextEditor::sEditorState& before,
      cTextEditor::sEditorState& after);

    void undo (cTextEditor* editor);
    void redo (cTextEditor* editor);

    std::string mAdded;
    sPosition mAddedStart;
    sPosition mAddedEnd;

    std::string mRemoved;
    sPosition mRemovedStart;
    sPosition mRemovedEnd;

    sEditorState mBefore;
    sEditorState mAfter;
    };
  //}}}

  typedef std::vector<sUndoRecord> tUndoBuffer;
  //{{{  gets
  bool isOnWordBoundary (const sPosition& position) const;

  std::string getText (const sPosition& startPosition, const sPosition& endPosition) const;
  sPosition getActualCursorposition() const { return sanitizePosition (mState.mCursorPosition); }

  int getCharacterIndex (const sPosition& position) const;
  int getCharacterColumn (int lineNumber, int index) const;
  int getLineCharacterCount (int row) const;
  int getLineMaxColumn (int row) const;

  std::string getWordAt (const sPosition& position) const;
  std::string getWordUnderCursor() const;
  ImU32 getGlyphColor (const sGlyph& glyph) const;

  float getTextDistanceToLineStart (const sPosition& from) const;
  int getPageNumLines() const;
  //}}}
  //{{{  sets
  sPosition screenToPosition (const ImVec2& pos) const;
  sPosition sanitizePosition (const sPosition& position) const;
  //}}}
  //{{{  find
  sPosition findWordStart (const sPosition& from) const;
  sPosition findWordEnd (const sPosition& from) const;
  sPosition findNextWord (const sPosition& from) const;
  //}}}
  //{{{  colorize
  void colorize (int fromLine = 0, int count = -1);
  void colorizeRange (int fromLine = 0, int toLine = 0);
  void colorizeInternal();
  //}}}
  //{{{  actions
  void ensureCursorVisible();

  void advance (sPosition& position) const;
  void deleteRange (const sPosition& startPosition, const sPosition& endPosition);
  int insertTextAt (sPosition& where, const char* value);

  void addUndo (sUndoRecord& value);

  void removeLine (int startPosition, int endPosition);
  void removeLine (int index);
  std::vector<sGlyph>& insertLine (int index);

  void enterCharacter (ImWchar ch, bool shift);

  void backspace();
  void deleteSelection();
  //}}}

  void handleMouseInputs();
  void handleKeyboardInputs();

  void render();
  //{{{  vars
  std::vector<sLine> mLines;

  std::array<ImU32, (size_t)ePalette::Max> mPaletteBase;
  std::array<ImU32, (size_t)ePalette::Max> mPalette;

  sLanguage mLanguage;
  tRegexList mRegexList;
  std::map<int, std::string> mMarkers;

  float mLineSpacing;

  sEditorState mState;

  tUndoBuffer mUndoBuffer;
  int mUndoIndex;

  int mTabSize;
  float mTextStart;

  bool mOverwrite;
  bool mReadOnly;
  bool mWithinRender;
  bool mScrollToCursor;
  bool mScrollToTop;

  bool mTextChanged;
  bool mCursorPositionChanged;

  int mColorRangeMin;
  int mColorRangeMax;
  eSelection mSelection;

  bool mHandleKeyboardInputs;
  bool mHandleMouseInputs;
  bool mIgnoreImGuiChild;
  bool mShowWhitespaces;

  bool mCheckComments;
  ImVec2 mCharAdvance;

  sPosition mInteractiveStart;
  sPosition mInteractiveEnd;

  uint64_t mStartTime;
  float mLastClick;
  //}}}
  };
