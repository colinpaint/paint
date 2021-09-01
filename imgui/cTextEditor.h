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
    Identifier,
    KnownIdentifier,
    PreprocIdentifier,
    Comment,
    MultiLineComment,
    Background,
    Cursor,
    Selection,
    ErrorMarker,
    Breakpoint,
    LineNumber,
    CurrentLineFill,
    CurrentLineFillInactive,
    CurrentLineEdge,
    Max
    };
  //}}}
  //{{{
  enum class eSelectionMode {
    Normal,
    Word,
    Line
    };
  //}}}

  //{{{
  struct sGlyph {
    uint8_t mChar;
    ePaletteIndex mColorIndex = ePaletteIndex::Default;
    bool mComment : 1;
    bool mMultiLineComment : 1;
    bool mPreprocessor : 1;

    sGlyph(uint8_t aChar, ePaletteIndex aColorIndex) : mChar(aChar), mColorIndex(aColorIndex),
      mComment(false), mMultiLineComment(false), mPreprocessor(false) {}
    };
  //}}}
  //{{{
  struct sBreakpoint {
    int mLine;
    bool mEnabled;
    std::string mCondition;

    sBreakpoint() : mLine(-1), mEnabled(false) {}
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
  struct sCoordinates {
    int mLine;
    int mColumn;

    sCoordinates() : mLine(0), mColumn(0) {}
    //{{{
    sCoordinates(int aLine, int aColumn) : mLine(aLine), mColumn(aColumn)
    {
      assert(aLine >= 0);
      assert(aColumn >= 0);
    }
    static sCoordinates Invalid() { static sCoordinates invalid(-1, -1); return invalid; }
    //}}}
    //{{{
    bool operator ==(const sCoordinates& o) const
    {
      return
        mLine == o.mLine &&
        mColumn == o.mColumn;
    }
    //}}}
    //{{{
    bool operator !=(const sCoordinates& o) const
    {
      return
        mLine != o.mLine ||
        mColumn != o.mColumn;
    }
    //}}}
    //{{{
    bool operator <(const sCoordinates& o) const
    {
      if (mLine != o.mLine)
        return mLine < o.mLine;
      return mColumn < o.mColumn;
    }
    //}}}
    //{{{
    bool operator >(const sCoordinates& o) const
    {
      if (mLine != o.mLine)
        return mLine > o.mLine;
      return mColumn > o.mColumn;
    }
    //}}}
    //{{{
    bool operator <=(const sCoordinates& o) const
    {
      if (mLine != o.mLine)
        return mLine < o.mLine;
      return mColumn <= o.mColumn;
    }
    //}}}
    //{{{
    bool operator >=(const sCoordinates& o) const
    {
      if (mLine != o.mLine)
        return mLine > o.mLine;
      return mColumn >= o.mColumn;
    }
    //}}}
  };
  //}}}
  //{{{
  struct sIdentifier {
    sCoordinates mLocation;
    std::string mDeclaration;
    };
  //}}}

  typedef std::vector<sGlyph> tLine;
  typedef std::vector<tLine> tLines;
  typedef std::array<ImU32, (unsigned)ePaletteIndex::Max> tPalette;
  typedef std::unordered_map<std::string, sIdentifier> tIdentifiers;
  typedef std::unordered_set<std::string> tKeywords;
  typedef std::map<int, std::string> tErrorMarkers;
  typedef std::unordered_set<int> tBreakpoints;

  //{{{
  struct sLanguageDefinition {
    // typedef
    typedef std::pair<std::string, ePaletteIndex> TokenRegexString;
    typedef std::vector<TokenRegexString> TokenRegexStrings;
    typedef bool (*TokenizeCallback)(const char* in_begin, const char* in_end,
                                     const char*& out_begin, const char*& out_end, ePaletteIndex& paletteIndex);

    // vars
    std::string mName;
    tKeywords mKeywords;

    tIdentifiers mIdentifiers;
    tIdentifiers mPreprocIdentifiers;

    std::string mCommentStart;
    std::string mCommentEnd;
    std::string mSingleLineComment;

    char mPreprocChar;
    bool mAutoIndentation;

    TokenizeCallback mTokenize;
    TokenRegexStrings mTokenRegexStrings;

    bool mCaseSensitive;

    // init
    sLanguageDefinition() : mPreprocChar('#'), mAutoIndentation(true), mTokenize(nullptr), mCaseSensitive(true) {}

    // static const
    static const sLanguageDefinition& CPlusPlus();
    static const sLanguageDefinition& HLSL();
    static const sLanguageDefinition& GLSL();
    static const sLanguageDefinition& C();
    static const sLanguageDefinition& SQL();
    static const sLanguageDefinition& AngelScript();
    static const sLanguageDefinition& Lua();
    };
  //}}}

  cTextEditor();
  ~cTextEditor() = default;
  //{{{  members


  const sLanguageDefinition& GetLanguageDefinition() const { return mLanguageDefinition; }
  const tPalette& GetPalette() const { return mPaletteBase; }

  void SetErrorMarkers (const tErrorMarkers& aMarkers) { mErrorMarkers = aMarkers; }
  void SetBreakpoints (const tBreakpoints& aMarkers) { mBreakpoints = aMarkers; }
  void SetLanguageDefinition (const sLanguageDefinition& aLanguageDef);
  void SetPalette (const tPalette& aValue);
  void SetText (const std::string& aText);
  void SetTextLines (const std::vector<std::string>& aLines);

  void Render (const char* aTitle, const ImVec2& aSize = ImVec2(), bool aBorder = false);

  std::string GetText() const;
  std::vector<std::string> GetTextLines() const;

  std::string GetSelectedText() const;
  std::string GetCurrentLineText()const;

  int GetTotalLines() const { return (int)mLines.size(); }
  bool IsOverwrite() const { return mOverwrite; }

  void SetReadOnly (bool aValue) { mReadOnly = aValue; }
  bool IsReadOnly() const { return mReadOnly; }
  bool IsTextChanged() const { return mTextChanged; }
  bool IsCursorPositionChanged() const { return mCursorPositionChanged; }

  bool IsColorizerEnabled() const { return mColorizerEnabled; }
  void SetColorizerEnable (bool aValue) { mColorizerEnabled = aValue; }

  sCoordinates GetCursorPosition() const { return GetActualCursorCoordinates(); }
  void SetCursorPosition(const sCoordinates& aPosition);

  inline void SetHandleMouseInputs (bool aValue){ mHandleMouseInputs    = aValue;}
  inline bool IsHandleMouseInputsEnabled() const { return mHandleKeyboardInputs; }

  inline void SetHandleKeyboardInputs (bool aValue){ mHandleKeyboardInputs = aValue;}
  inline bool IsHandleKeyboardInputsEnabled() const { return mHandleKeyboardInputs; }

  inline void SetImGuiChildIgnored (bool aValue){ mIgnoreImGuiChild     = aValue;}
  inline bool IsImGuiChildIgnored() const { return mIgnoreImGuiChild; }

  inline void SetShowWhitespaces(bool aValue) { mShowWhitespaces = aValue; }
  inline bool IsShowingWhitespaces() const { return mShowWhitespaces; }

  void SetTabSize (int aValue) { mTabSize = std::max(0, std::min(32, aValue)); }
  inline int GetTabSize() const { return mTabSize; }

  void InsertText(const char* aValue);
  void InsertText (const std::string& aValue) { InsertText (aValue.c_str()); }

  void MoveUp(int aAmount = 1, bool aSelect = false);
  void MoveDown(int aAmount = 1, bool aSelect = false);
  void MoveLeft(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
  void MoveRight(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
  void MoveTop(bool aSelect = false);
  void MoveBottom(bool aSelect = false);
  void MoveHome(bool aSelect = false);
  void MoveEnd(bool aSelect = false);

  void SetSelectionStart(const sCoordinates& aPosition);
  void SetSelectionEnd(const sCoordinates& aPosition);
  void SetSelection(const sCoordinates& aStart, const sCoordinates& aEnd, eSelectionMode aMode = eSelectionMode::Normal);
  void SelectWordUnderCursor();
  void SelectAll();
  bool HasSelection() const;

  void Copy();
  void Cut();
  void Paste();
  void Delete();

  bool CanUndo() const;
  bool CanRedo() const;
  void Undo (int aSteps = 1);
  void Redo (int aSteps = 1);
  //}}}
  //{{{  static members
  static const tPalette& GetDarkPalette();
  static const tPalette& GetLightPalette();
  //}}}

private:
  typedef std::vector<std::pair<std::regex, ePaletteIndex>> tRegexList;
  //{{{
  struct sEditorState
  {
    sCoordinates mSelectionStart;
    sCoordinates mSelectionEnd;
    sCoordinates mCursorPosition;
  };
  //}}}
  //{{{
  class sUndoRecord
  {
  public:
    sUndoRecord() {}
    ~sUndoRecord() {}

    sUndoRecord(
      const std::string& aAdded,
      const cTextEditor::sCoordinates aAddedStart,
      const cTextEditor::sCoordinates aAddedEnd,

      const std::string& aRemoved,
      const cTextEditor::sCoordinates aRemovedStart,
      const cTextEditor::sCoordinates aRemovedEnd,

      cTextEditor::sEditorState& aBefore,
      cTextEditor::sEditorState& aAfter);

    void Undo (cTextEditor* aEditor);
    void Redo (cTextEditor* aEditor);

    std::string mAdded;
    sCoordinates mAddedStart;
    sCoordinates mAddedEnd;

    std::string mRemoved;
    sCoordinates mRemovedStart;
    sCoordinates mRemovedEnd;

    sEditorState mBefore;
    sEditorState mAfter;
  };
  //}}}

  typedef std::vector<sUndoRecord> tUndoBuffer;
  //{{{  members
  void ProcessInputs();

  void Colorize(int aFromLine = 0, int aCount = -1);
  void ColorizeRange(int aFromLine = 0, int aToLine = 0);
  void ColorizeInternal();

  float TextDistanceToLineStart(const sCoordinates& aFrom) const;
  void EnsureCursorVisible();

  int GetPageSize() const;
  std::string GetText(const sCoordinates& aStart, const sCoordinates& aEnd) const;

  sCoordinates GetActualCursorCoordinates() const;
  sCoordinates SanitizeCoordinates(const sCoordinates& aValue) const;
  void Advance(sCoordinates& aCoordinates) const;

  void DeleteRange(const sCoordinates& aStart, const sCoordinates& aEnd);
  int InsertTextAt(sCoordinates& aWhere, const char* aValue);

  void AddUndo(sUndoRecord& aValue);

  sCoordinates ScreenPosToCoordinates(const ImVec2& aPosition) const;
  sCoordinates FindWordStart(const sCoordinates& aFrom) const;
  sCoordinates FindWordEnd(const sCoordinates& aFrom) const;
  sCoordinates FindNextWord(const sCoordinates& aFrom) const;

  int GetCharacterIndex(const sCoordinates& aCoordinates) const;
  int GetCharacterColumn(int aLine, int aIndex) const;
  int GetLineCharacterCount(int aLine) const;
  int GetLineMaxColumn(int aLine) const;

  bool IsOnWordBoundary(const sCoordinates& aAt) const;

  void RemoveLine(int aStart, int aEnd);
  void RemoveLine(int aIndex);
  tLine& InsertLine(int aIndex);

  void EnterCharacter(ImWchar aChar, bool aShift);
  void Backspace();
  void DeleteSelection();

  std::string GetWordUnderCursor() const;
  std::string GetWordAt(const sCoordinates& aCoords) const;
  ImU32 GetGlyphColor(const sGlyph& aGlyph) const;

  void HandleKeyboardInputs();
  void HandleMouseInputs();

  void Render();
  //}}}
  //{{{  vars
  float mLineSpacing;
  tLines mLines;
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
  float mTextStart;                   // position (in pixels) where a code line starts relative to the left of the cTextEditor.
  int  mLeftMargin;
  bool mCursorPositionChanged;
  int mColorRangeMin, mColorRangeMax;
  eSelectionMode mSelectionMode;
  bool mHandleKeyboardInputs;
  bool mHandleMouseInputs;
  bool mIgnoreImGuiChild;
  bool mShowWhitespaces;

  tPalette mPaletteBase;
  tPalette mPalette;
  sLanguageDefinition mLanguageDefinition;
  tRegexList mRegexList;

  bool mCheckComments;
  tBreakpoints mBreakpoints;
  tErrorMarkers mErrorMarkers;
  ImVec2 mCharAdvance;
  sCoordinates mInteractiveStart, mInteractiveEnd;
  std::string mLineBuffer;
  uint64_t mStartTime;

  float mLastClick;
  //}}}
  };
