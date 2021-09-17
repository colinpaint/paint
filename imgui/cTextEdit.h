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
  enum class eSelection { Normal, Word, Line };
  //{{{  palette const
  // tried enum but array of foxed it
  static const uint8_t eDefault =           0;
  static const uint8_t eKeyword =           1;
  static const uint8_t eNumber =            2;
  static const uint8_t eString =            3;
  static const uint8_t eCharLiteral =       4;
  static const uint8_t ePunctuation =       5;

  static const uint8_t ePreprocessor =      6;
  static const uint8_t eIdent =             7;
  static const uint8_t eKnownIdent =        8;
  static const uint8_t ePreprocIdent =      9;

  static const uint8_t eComment =          10;
  static const uint8_t eMultiLineComment = 11;

  static const uint8_t eCursor =           12;
  static const uint8_t eSelect =           13;
  static const uint8_t eMarker =           14;
  static const uint8_t eCurrentLineFill =  15;
  static const uint8_t eCurrentLineFillInactive = 16;
  static const uint8_t eCurrentLineEdge =  17;

  static const uint8_t eLineNumber =       18;

  static const uint8_t eWhiteSpace =       19;
  static const uint8_t eTab =              20;

  static const uint8_t eFoldBeginClosed = 21;
  static const uint8_t eFoldBeginOpen =   22;
  static const uint8_t eFoldEnd =         23;

  static const uint8_t eMax =             24;
  //}}}
  //{{{
  struct sGlyph {
    uint8_t mChar;
    uint8_t mColorIndex;

    bool mComment:1;
    bool mMultiLineComment:1;
    bool mPreProc:1;

    sGlyph() :
      mChar(' '),
      mColorIndex(eDefault),
      mComment(false), mMultiLineComment(false), mPreProc(false) {}

    sGlyph (uint8_t ch, uint8_t colorIndex) :
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
    typedef std::pair<std::string, uint8_t> tTokenRegexString;
    typedef std::vector<tTokenRegexString> tTokenRegexStrings;
    typedef bool (*tTokenizeCallback)(const char* inBegin, const char* inEnd,
                                      const char*& outBegin, const char*& outEnd, uint8_t& palette);

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

    // only used by drawText, no point in being a string
    const char* mFoldBeginOpen;
    const char* mFoldBeginClosed;
    const char* mFoldEnd;

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
  bool isTextEdited() const { return mInfo.mTextEdited; }
  bool isShowFolds() const { return mOptions.mShowFolded; }

  // has
  bool hasCR() const { return mInfo.mHasCR; }
  bool hasTabs() const { return mInfo.mHasTabs; }
  bool hasSelect() const { return mEdit.mState.mSelectionEnd > mEdit.mState.mSelectionStart; }
  bool hasUndo() const { return !mOptions.mReadOnly && mUndoList.mIndex > 0; }
  bool hasRedo() const { return !mOptions.mReadOnly && mUndoList.mIndex < (int)mUndoList.mBuffer.size(); }
  bool hasClipboardText();

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

  void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }
  void setTabSize (int value) { mInfo.mTabSize = std::max (0, std::min (32, value)); }

  void setCursorPosition (const sPosition& position);
  void setSelectionStart (const sPosition& position);
  void setSelectionEnd (const sPosition& position);
  void setSelection (const sPosition& startPosition, const sPosition& endPosition, eSelection mode = eSelection::Normal);

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
  typedef std::vector <std::pair <std::regex,uint8_t>> tRegexList;
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
    bool mOverWrite = false;
    bool mReadOnly = false;
    bool mCheckComments = true;

    // shows
    bool mShowFolded = false;
    bool mShowLineNumber = true;
    bool mShowLineDebug = false;
    bool mShowWhiteSpace = false;
    bool mShowMonoSpaced = true;

    bool mHoverFolded = false;
    bool mHoverLineNumber = false;
    bool mHoverWhiteSpace = false;
    bool mHoverMonoSpaced = false;

    sLanguage mLanguage;
    tRegexList mRegexList;
    std::map <int,std::string> mMarkers;

    std::array <ImU32,eMax> mPalette;
    std::array <ImU32,eMax> mPaletteBase;
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

    float measureText (const char* str, const char* strEnd) const;
    float drawText (ImVec2 pos, ImU32 color, const char* str, const char* strEnd = nullptr);
    float drawSmallText (ImVec2 pos, ImU32 color, const char* str, const char* strEnd = nullptr);

    ImDrawList* mDrawList = nullptr;
    bool mFocused = false;

    float mFontAtlasSize = 0.f;
    float mFontSize = 0.f;

    float mLineHeight = 0.f;
    float mGlyphWidth = 0.f;

    float mLeftPad = 0.f;

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
  bool isFolded() const { return mOptions.mShowFolded || mOptions.mHoverFolded; }
  bool isDrawLineNumber() const { return mOptions.mShowLineNumber || mOptions.mHoverLineNumber; }
  bool isDrawWhiteSpace() const { return mOptions.mShowWhiteSpace || mOptions.mHoverWhiteSpace; }
  bool isDrawMonoSpaced() const { return mOptions.mShowMonoSpaced ^ mOptions.mHoverMonoSpaced; }

  int getCharacterIndex (const sPosition& position) const;
  int getCharacterColumn (int lineNumber, int index) const;

  int getLineNumChars (int row) const;
  int getLineMaxColumn (int row) const;

  int getPageNumLines() const;
  int getMaxLineIndex() const;
  float getTextWidth (const sPosition& position) const;

  std::string getText (const sPosition& startPosition, const sPosition& endPosition) const;
  std::string getCurrentLineText() const;
  std::string getSelectedText() const;

  ImU32 getGlyphColor (const sGlyph& glyph) const;

  bool isOnWordBoundary (const sPosition& position) const;
  std::string getWordAt (const sPosition& position) const;
  std::string getWordUnderCursor() const;

  int getTabColumn (int column) const;
  float getTabEndPosX (float columnX) const;
  sPosition getPositionFromPosX (int lineNumber, float posX) const;

  int getLineNumberFromIndex (int lineIndex) const;
  int getLineIndexFromNumber (int lineNumber) const;
  //}}}
  //{{{  utils
  void advance (sPosition& position) const;
  sPosition sanitizePosition (const sPosition& position) const;
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

  // clicks
  void clickLine (int lineNumber);
  void clickFold (int lineNumber, bool foldOpen);
  void clickText (int lineNumber, float posX, bool selectWord);
  void dragLine (int lineNumber, float posY);
  void dragText (int lineNumber, ImVec2 pos);

  // folds
  void parseFolds();

  void handleKeyboardInputs();

  void drawTop (cApp& app);
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

  std::chrono::system_clock::time_point mLastFlashTime;
  //}}}
  };
