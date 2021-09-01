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
  enum class eSelectionMode { Normal, Word, Line };

  //{{{
  struct sGlyph {
    uint8_t mChar;
    ePaletteIndex mColorIndex = ePaletteIndex::Default;
    bool mComment : 1;
    bool mMultiLineComment : 1;
    bool mPreprocessor : 1;

    sGlyph (uint8_t aChar, ePaletteIndex aColorIndex) : mChar(aChar), mColorIndex(aColorIndex),
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
    sCoordinates(int line, int aColumn) : mLine(line), mColumn(aColumn)
    {
      assert(line >= 0);
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
  struct sLanguage {
    // typedef
    typedef std::pair<std::string, ePaletteIndex> TokenRegexString;
    typedef std::vector<TokenRegexString> TokenRegexStrings;
    typedef bool (*TokenizeCallback)(const char* inBegin, const char* inEnd,
                                     const char*& outBegin, const char*& outEnd, ePaletteIndex& paletteIndex);

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
  bool IsOverwrite() const { return mOverwrite; }
  bool IsReadOnly() const { return mReadOnly; }
  bool IsTextChanged() const { return mTextChanged; }
  bool IsCursorPositionChanged() const { return mCursorPositionChanged; }
  bool IsColorizerEnabled() const { return mColorizerEnabled; }

  bool HasSelection() const { return mState.mSelectionEnd > mState.mSelectionStart; }

  inline bool IsHandleMouseInputsEnabled() const { return mHandleKeyboardInputs; }
  inline bool IsHandleKeyboardInputsEnabled() const { return mHandleKeyboardInputs; }
  inline bool IsImGuiChildIgnored() const { return mIgnoreImGuiChild; }
  inline bool IsShowingWhitespaces() const { return mShowWhitespaces; }

  const sLanguage& GetLanguage() const { return mLanguage; }
  const tPalette& GetPalette() const { return mPaletteBase; }

  std::string GetText() const;
  std::vector<std::string> GetTextLines() const;

  std::string GetSelectedText() const;
  std::string GetCurrentLineText()const;
  int GetTotalLines() const { return (int)mLines.size(); }

  sCoordinates GetCursorPosition() const { return GetActualCursorCoordinates(); }

  inline int GetTabSize() const { return mTabSize; }
  //}}}
  //{{{  sets
  void SetErrorMarkers (const tErrorMarkers& markers) { mErrorMarkers = markers; }
  void SetBreakpoints (const tBreakpoints& markers) { mBreakpoints = markers; }
  void SetLanguage (const sLanguage& language);
  void SetPalette (const tPalette& value);

  void SetText (const std::string& text);
  void SetTextLines (const std::vector<std::string>& lines);

  void SetReadOnly (bool value) { mReadOnly = value; }
  void SetColorizerEnable (bool value) { mColorizerEnabled = value; }

  void SetCursorPosition (const sCoordinates& position);
  void SetTabSize (int value) { mTabSize = std::max (0, std::min (32, value)); }
  void SetSelectionStart (const sCoordinates& position);
  void SetSelectionEnd (const sCoordinates& position);
  void SetSelection (const sCoordinates& startCord, const sCoordinates& endCord, eSelectionMode aMode = eSelectionMode::Normal);

  inline void SetHandleMouseInputs (bool value) { mHandleMouseInputs = value;}
  inline void SetHandleKeyboardInputs (bool value) { mHandleKeyboardInputs = value;}
  inline void SetImGuiChildIgnored (bool value) { mIgnoreImGuiChild = value;}
  inline void SetShowWhitespaces(bool value) { mShowWhitespaces = value; }
  //}}}
  //{{{  actions
  void MoveUp (int aAmount = 1, bool select = false);
  void MoveDown (int aAmount = 1, bool select = false);
  void MoveLeft (int aAmount = 1, bool select = false, bool wordMode = false);
  void MoveRight (int aAmount = 1, bool select = false, bool wordMode = false);

  void MoveTop (bool select = false);
  void MoveBottom (bool select = false);
  void MoveHome (bool select = false);
  void MoveEnd (bool select = false);

  void SelectAll();
  void SelectWordUnderCursor();

  void InsertText(const char* value);
  void InsertText (const std::string& value) { InsertText (value.c_str()); }
  void Copy();
  void Cut();
  void Paste();
  void Delete();

  bool CanUndo() const { return !mReadOnly && mUndoIndex > 0; }
  bool CanRedo() const { return !mReadOnly && mUndoIndex < (int)mUndoBuffer.size(); }
  void Undo (int steps = 1);
  void Redo (int steps = 1);

  void Render (const char* title, const ImVec2& size = ImVec2(), bool border = false);
  //}}}
  //{{{  static gets
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

    void Undo (cTextEditor* editor);
    void Redo (cTextEditor* editor);

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
  //{{{  gets
  bool IsOnWordBoundary (const sCoordinates& aAt) const;

  int GetPageSize() const;
  std::string GetText (const sCoordinates& startCord, const sCoordinates& endCord) const;
  sCoordinates GetActualCursorCoordinates() const { return SanitizeCoordinates (mState.mCursorPosition); }

  int GetCharacterIndex (const sCoordinates& aCoordinates) const;
  int GetCharacterColumn (int lineCoord, int index) const;
  int GetLineCharacterCount (int lineCoord) const;
  int GetLineMaxColumn (int lineCoord) const;

  std::string GetWordAt (const sCoordinates& coords) const;
  std::string GetWordUnderCursor() const;
  ImU32 GetGlyphColor (const sGlyph& glyph) const;

  float TextDistanceToLineStart (const sCoordinates& from) const;
  //}}}
  //{{{  coords
  sCoordinates SanitizeCoordinates (const sCoordinates& value) const;
  sCoordinates ScreenPosToCoordinates (const ImVec2& position) const;
  //}}}
  //{{{  find
  sCoordinates FindWordStart (const sCoordinates& from) const;
  sCoordinates FindWordEnd (const sCoordinates& from) const;
  sCoordinates FindNextWord (const sCoordinates& from) const;
  //}}}
  //{{{  colorize
  void Colorize (int fromLine = 0, int count = -1);
  void ColorizeRange (int fromLine = 0, int toLine = 0);
  void ColorizeInternal();
  //}}}
  //{{{  actions
  void EnsureCursorVisible();

  void Advance (sCoordinates& aCoordinates) const;
  void DeleteRange (const sCoordinates& startCord, const sCoordinates& endCord);
  int InsertTextAt (sCoordinates& aWhere, const char* value);

  void AddUndo (sUndoRecord& value);

  void RemoveLine (int startCord, int endCord);
  void RemoveLine (int index);
  tLine& InsertLine (int index);

  void EnterCharacter (ImWchar aChar, bool aShift);

  void Backspace();
  void DeleteSelection();
  //}}}

  void HandleMouseInputs();
  void HandleKeyboardInputs();

  void Render();
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
  sLanguage mLanguage;
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
