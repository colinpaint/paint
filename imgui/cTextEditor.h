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
  //{{{
  enum class ePaletteIndex {
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
    ErrorMarker,
    Break,
    LineNumber,
    CurrentLineFill,
    CurrentLineFillInactive,
    CurrentLineEdge,
    Max
    };
  //}}}
  enum class eSelectionMode { Normal, Word, Line };

  //{{{
  struct sGlyph {
    uint8_t mChar;
    ePaletteIndex mColorIndex = ePaletteIndex::Default;
    bool mComment : 1;
    bool mMultiLineComment : 1;
    bool mPreprocessor : 1;

    sGlyph (uint8_t aChar, ePaletteIndex aColorIndex)
      : mChar(aChar), mColorIndex(aColorIndex), mComment(false), mMultiLineComment(false), mPreprocessor(false) {}
    };
  //}}}
  //{{{
  struct sBreak {
    int mLine;
    bool mEnabled;
    std::string mCondition;

    sBreak() : mLine(-1), mEnabled(false) {}
    };
  //}}}
  //{{{
  // Represents a character coordinate from the user's point of view,
  // i. e. consider an uniform grid (assuming fixed-width font) on the
  // screen as it is rendered, and each cell has its own coordinate, starting from 0.
  // Tabs are counted as [1..mTabSize] count empty spaces, depending on
  // how many space is necessary to reach the next tab stop.
  // For example, coordinate (1, 5) represents the character 'B' in a line "\tABC", when mTabSize = 4,
  // because it is rendered as "    ABC" on the screen.
  struct sRowColumn {
    int mLine;
    int mColumn;

    sRowColumn() : mLine(0), mColumn(0) {}
    //{{{
    sRowColumn (int line, int column) : mLine(line), mColumn(column) {
      assert (line >= 0);
      assert (column >= 0);
      }
    //}}}
    static sRowColumn Invalid() { static sRowColumn invalid(-1, -1); return invalid; }
    //{{{
    bool operator ==(const sRowColumn& o) const
    {
      return
        mLine == o.mLine &&
        mColumn == o.mColumn;
    }
    //}}}
    //{{{
    bool operator !=(const sRowColumn& o) const
    {
      return
        mLine != o.mLine ||
        mColumn != o.mColumn;
    }
    //}}}
    //{{{
    bool operator <(const sRowColumn& o) const
    {
      if (mLine != o.mLine)
        return mLine < o.mLine;
      return mColumn < o.mColumn;
    }
    //}}}
    //{{{
    bool operator >(const sRowColumn& o) const
    {
      if (mLine != o.mLine)
        return mLine > o.mLine;
      return mColumn > o.mColumn;
    }
    //}}}
    //{{{
    bool operator <=(const sRowColumn& o) const
    {
      if (mLine != o.mLine)
        return mLine < o.mLine;
      return mColumn <= o.mColumn;
    }
    //}}}
    //{{{
    bool operator >=(const sRowColumn& o) const
    {
      if (mLine != o.mLine)
        return mLine > o.mLine;
      return mColumn >= o.mColumn;
    }
    //}}}
  };
  //}}}
  //{{{
  struct sIdent {
    sRowColumn mLocation;
    std::string mDeclaration;
    };
  //}}}

  using tLine = std::vector<sGlyph>;
  using tLines = std::vector<tLine>;
  using tPalette = std::array<ImU32, (unsigned)ePaletteIndex::Max>;
  using tBreaks = std::unordered_set<int>;
  using tKeywords = std::unordered_set<std::string>;
  using tErrorMarkers = std::map<int, std::string>;
  using tIdents = std::unordered_map<std::string, sIdent>;
  //{{{
  struct sLanguage {
    // typedef
    typedef std::pair<std::string, ePaletteIndex> tTokenRegexString;
    typedef std::vector<tTokenRegexString> tTokenRegexStrings;
    typedef bool (*tTokenizeCallback)(const char* inBegin, const char* inEnd,
                                      const char*& outBegin, const char*& outEnd, ePaletteIndex& paletteIndex);

    // vars
    std::string mName;

    tKeywords mKeywords;
    tIdents mIdents;
    tIdents mPreprocIdents;

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
    static const sLanguage& AngelScript();
    static const sLanguage& Lua();
    static const sLanguage& SQL();
    };
  //}}}

  cTextEditor();
  ~cTextEditor() = default;
  //{{{  gets
  bool isOverwrite() const { return mOverwrite; }
  bool isReadOnly() const { return mReadOnly; }
  bool isTextChanged() const { return mTextChanged; }
  bool isCursorPositionChanged() const { return mCursorPositionChanged; }
  bool isColorizerEnabled() const { return mColorizerEnabled; }

  bool hasSelection() const { return mState.mSelectionEnd > mState.mSelectionStart; }

  inline bool isHandleMouseInputsEnabled() const { return mHandleKeyboardInputs; }
  inline bool isHandleKeyboardInputsEnabled() const { return mHandleKeyboardInputs; }
  inline bool isImGuiChildIgnored() const { return mIgnoreImGuiChild; }
  inline bool isShowingWhitespaces() const { return mShowWhitespaces; }

  const sLanguage& getLanguage() const { return mLanguage; }
  const tPalette& getPalette() const { return mPaletteBase; }

  std::string getText() const;
  std::vector<std::string> getTextLines() const;

  std::string getSelectedText() const;
  std::string getCurrentLineText()const;
  int getTotalLines() const { return (int)mLines.size(); }

  sRowColumn getCursorPosition() const { return getActualCursorRowColumn(); }

  inline int getTabSize() const { return mTabSize; }
  //}}}
  //{{{  sets
  void setErrorMarkers (const tErrorMarkers& markers) { mErrorMarkers = markers; }
  void setBreaks (const tBreaks& markers) { mBreaks = markers; }
  void setLanguage (const sLanguage& language);
  void setPalette (const tPalette& value);

  void setText (const std::string& text);
  void setTextLines (const std::vector<std::string>& lines);

  void setReadOnly (bool value) { mReadOnly = value; }
  void setColorizerEnable (bool value) { mColorizerEnabled = value; }

  void setCursorPosition (const sRowColumn& position);
  void setTabSize (int value) { mTabSize = std::max (0, std::min (32, value)); }
  void setSelectionStart (const sRowColumn& position);
  void setSelectionEnd (const sRowColumn& position);
  void setSelection (const sRowColumn& startCord, const sRowColumn& endCord, eSelectionMode aMode = eSelectionMode::Normal);

  inline void setHandleMouseInputs (bool value) { mHandleMouseInputs = value;}
  inline void setHandleKeyboardInputs (bool value) { mHandleKeyboardInputs = value;}
  inline void setImGuiChildIgnored (bool value) { mIgnoreImGuiChild = value;}
  inline void setShowWhitespaces(bool value) { mShowWhitespaces = value; }
  //}}}
  //{{{  actions
  void moveUp (int aAmount = 1, bool select = false);
  void moveDown (int aAmount = 1, bool select = false);
  void moveLeft (int aAmount = 1, bool select = false, bool wordMode = false);
  void moveRight (int aAmount = 1, bool select = false, bool wordMode = false);

  void moveTop (bool select = false);
  void moveBottom (bool select = false);
  void moveHome (bool select = false);
  void moveEnd (bool select = false);

  void selectAll();
  void selectWordUnderCursor();

  void InsertText(const char* value);
  void InsertText (const std::string& value) { InsertText (value.c_str()); }
  void copy();
  void cut();
  void paste();
  void deleteIt();

  bool canUndo() const { return !mReadOnly && mUndoIndex > 0; }
  bool canRedo() const { return !mReadOnly && mUndoIndex < (int)mUndoBuffer.size(); }
  void undo (int steps = 1);
  void redo (int steps = 1);

  void render (const char* title, const ImVec2& size = ImVec2(), bool border = false);
  //}}}
  //{{{  static gets
  static const tPalette& getDarkPalette();
  static const tPalette& getLightPalette();
  //}}}

private:
  typedef std::vector<std::pair<std::regex, ePaletteIndex>> tRegexList;
  //{{{
  struct sEditorState {
    sRowColumn mSelectionStart;
    sRowColumn mSelectionEnd;
    sRowColumn mCursorPosition;
    };
  //}}}
  //{{{
  class sUndoRecord {
  public:
    sUndoRecord() {}
    ~sUndoRecord() {}

    sUndoRecord (
      const std::string& aAdded,
      const cTextEditor::sRowColumn aAddedStart,
      const cTextEditor::sRowColumn aAddedEnd,

      const std::string& aRemoved,
      const cTextEditor::sRowColumn aRemovedStart,
      const cTextEditor::sRowColumn aRemovedEnd,

      cTextEditor::sEditorState& aBefore,
      cTextEditor::sEditorState& aAfter);

    void Undo (cTextEditor* editor);
    void Redo (cTextEditor* editor);

    std::string mAdded;
    sRowColumn mAddedStart;
    sRowColumn mAddedEnd;

    std::string mRemoved;
    sRowColumn mRemovedStart;
    sRowColumn mRemovedEnd;

    sEditorState mBefore;
    sEditorState mAfter;
    };
  //}}}

  typedef std::vector<sUndoRecord> tUndoBuffer;
  //{{{  gets
  bool isOnWordBoundary (const sRowColumn& aAt) const;

  int getPageSize() const;
  std::string getText (const sRowColumn& startCord, const sRowColumn& endCord) const;
  sRowColumn getActualCursorRowColumn() const { return sanitizeRowColumn (mState.mCursorPosition); }

  int getCharacterIndex (const sRowColumn& rowColumn) const;
  int getCharacterColumn (int lineCoord, int index) const;
  int getLineCharacterCount (int lineCoord) const;
  int getLineMaxColumn (int lineCoord) const;

  std::string getWordAt (const sRowColumn& rowColumn) const;
  std::string getWordUnderCursor() const;
  ImU32 getGlyphColor (const sGlyph& glyph) const;

  float getTextDistanceToLineStart (const sRowColumn& from) const;
  //}}}
  //{{{  sets
  sRowColumn sanitizeRowColumn (const sRowColumn& rowColumn) const;
  sRowColumn screenPosToRowColumn (const ImVec2& position) const;
  //}}}
  //{{{  find
  sRowColumn findWordStart (const sRowColumn& from) const;
  sRowColumn findWordEnd (const sRowColumn& from) const;
  sRowColumn findNextWord (const sRowColumn& from) const;
  //}}}
  //{{{  colorize
  void colorize (int fromLine = 0, int count = -1);
  void colorizeRange (int fromLine = 0, int toLine = 0);
  void colorizeInternal();
  //}}}
  //{{{  actions
  void ensureCursorVisible();

  void advance (sRowColumn& rowColumn) const;
  void deleteRange (const sRowColumn& startCord, const sRowColumn& endCord);
  int insertTextAt (sRowColumn& aWhere, const char* value);

  void addUndo (sUndoRecord& value);

  void removeLine (int startCord, int endCord);
  void removeLine (int index);
  tLine& insertLine (int index);

  void enterCharacter (ImWchar aChar, bool aShift);

  void backspace();
  void deleteSelection();
  //}}}

  void handleMouseInputs();
  void handleKeyboardInputs();

  void render();
  //{{{  vars
  tLines mLines;

  tPalette mPaletteBase;
  tPalette mPalette;
  sLanguage mLanguage;
  tRegexList mRegexList;
  tBreaks mBreaks;
  tErrorMarkers mErrorMarkers;

  float mLineSpacing;

  sEditorState mState;

  tUndoBuffer mUndoBuffer;
  int mUndoIndex;

  int mTabSize;

  bool mOverwrite;
  bool mReadOnly;
  bool mWithinRender;
  bool mScrollToCursor;
  bool mScrollToTop;

  bool mTextChanged;
  bool mColorizerEnabled;

  float mTextStart; // position (in pixels) where a code line starts relative to the left of the cTextEditor.
  int mLeftMargin;
  bool mCursorPositionChanged;

  int mColorRangeMin;
  int mColorRangeMax;
  eSelectionMode mSelectionMode;

  bool mHandleKeyboardInputs;
  bool mHandleMouseInputs;
  bool mIgnoreImGuiChild;
  bool mShowWhitespaces;

  bool mCheckComments;
  ImVec2 mCharAdvance;

  sRowColumn mInteractiveStart;
  sRowColumn mInteractiveEnd;

  std::string mLineBuffer;
  uint64_t mStartTime;

  float mLastClick;
  //}}}
  };
