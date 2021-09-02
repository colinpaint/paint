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
  enum class eSelectionMode { Normal, Word, Line };
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
    ePaletteIndex mColorIndex = ePaletteIndex::Default;
    bool mComment : 1;
    bool mMultiLineComment : 1;
    bool mPreprocessor : 1;

    sGlyph (uint8_t aChar, ePaletteIndex aColorIndex)
      : mChar(aChar), mColorIndex(aColorIndex), mComment(false), mMultiLineComment(false), mPreprocessor(false) {}
    };
  //}}}
  //{{{
  struct sLine {
  // editor line
    std::vector<sGlyph> mLine;
    uint32_t mNumber;
    bool mFoldStart;
    bool mFoldEnd;
    bool mFoldOpen;

    sLine() : mLine(), mNumber(0), mFoldStart(false), mFoldEnd(false), mFoldOpen(false) {}
    sLine (std::vector<sGlyph> line): mLine(line), mNumber(0), mFoldStart(false), mFoldEnd(false), mFoldOpen(false) {}
    };
  //}}}
  //{{{
  struct sRowColumn {
    int mRow;
    int mColumn;

    sRowColumn() : mRow(0), mColumn(0) {}
    //{{{
    sRowColumn (int row, int column) : mRow(row), mColumn(column) {
      assert (line >= 0);
      assert (column >= 0);
      }
    //}}}

    static sRowColumn Invalid() { static sRowColumn invalid(-1,-1); return invalid; }

    //{{{
    bool operator == (const sRowColumn& o) const {
      return mRow == o.mRow && mColumn == o.mColumn;
      }
    //}}}
    //{{{
    bool operator != (const sRowColumn& o) const {
      return mRow != o.mRow || mColumn != o.mColumn; }
    //}}}
    //{{{
    bool operator < (const sRowColumn& o) const {

      if (mRow != o.mRow)
        return mRow < o.mRow;

      return mColumn < o.mColumn;
      }
    //}}}
    //{{{
    bool operator > (const sRowColumn& o) const {
      if (mRow != o.mRow)
        return mRow > o.mRow;
      return mColumn > o.mColumn;
      }
    //}}}
    //{{{
    bool operator <= (const sRowColumn& o) const {
      if (mRow != o.mRow)
        return mRow < o.mRow;

      return mColumn <= o.mColumn;
      }
    //}}}
    //{{{
    bool operator >= (const sRowColumn& o) const {
      if (mRow != o.mRow)
        return mRow > o.mRow;
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
  //{{{
  struct sLanguage {
    // typedef
    typedef std::pair<std::string, ePaletteIndex> tTokenRegexString;
    typedef std::vector<tTokenRegexString> tTokenRegexStrings;
    typedef bool (*tTokenizeCallback)(const char* inBegin, const char* inEnd,
                                      const char*& outBegin, const char*& outEnd, ePaletteIndex& paletteIndex);

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
  const std::array<ImU32, (uint32_t)ePaletteIndex::Max>& getPalette() const { return mPaletteBase; }

  std::string getText() const;
  std::vector<std::string> getTextLines() const;

  std::string getSelectedText() const;
  std::string getCurrentLineText()const;
  int getTotalLines() const { return (int)mLines.size(); }

  sRowColumn getCursorPosition() const { return getActualCursorRowColumn(); }

  inline int getTabSize() const { return mTabSize; }
  //}}}
  //{{{  sets
  void setMarkers (const std::map<int,std::string>& markers) { mMarkers = markers; }
  void setLanguage (const sLanguage& language);
  void setPalette (const std::array<ImU32,(uint32_t)ePaletteIndex::Max>& value);

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

  void render (const char* title, const ImVec2& size = ImVec2(), bool border = false);
  //}}}
  //{{{  static gets
  static const std::array<ImU32, (uint32_t)ePaletteIndex::Max>& getDarkPalette();
  static const std::array<ImU32, (uint32_t)ePaletteIndex::Max>& getLightPalette();
  //}}}

private:
  typedef std::vector<std::pair<std::regex,ePaletteIndex>> tRegexList;
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
      const std::string& added,
      const cTextEditor::sRowColumn addedStart,
      const cTextEditor::sRowColumn addedEnd,

      const std::string& aRemoved,
      const cTextEditor::sRowColumn removedStart,
      const cTextEditor::sRowColumn removedEnd,

      cTextEditor::sEditorState& before,
      cTextEditor::sEditorState& after);

    void undo (cTextEditor* editor);
    void redo (cTextEditor* editor);

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
  int insertTextAt (sRowColumn& where, const char* value);

  void addUndo (sUndoRecord& value);

  void removeLine (int startCord, int endCord);
  void removeLine (int index);
  std::vector<sGlyph>& insertLine (int index);

  void enterCharacter (ImWchar aChar, bool shift);

  void backspace();
  void deleteSelection();
  //}}}

  void handleMouseInputs();
  void handleKeyboardInputs();

  void render();
  //{{{  vars
  std::vector<sLine> mLines;

  std::array<ImU32,(uint32_t)ePaletteIndex::Max> mPaletteBase;
  std::array<ImU32,(uint32_t)ePaletteIndex::Max> mPalette;
  sLanguage mLanguage;
  tRegexList mRegexList;
  std::map<int, std::string> mMarkers;

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
