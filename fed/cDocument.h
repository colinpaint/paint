// cDocument.h - loaded text file, parsed for tokens, comments and folds
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <regex>

#include <vector>
#include <unordered_set>

#include "../imgui/imgui.h"
//}}}

// tricksy use of color to mark token type
//{{{  palette colors
constexpr uint8_t eText =              0;
constexpr uint8_t eBackground =        1;
constexpr uint8_t eIdentifier =        2;
constexpr uint8_t eNumber =            3;
constexpr uint8_t ePunctuation =       4;
constexpr uint8_t eString =            5;
constexpr uint8_t eLiteral =           6;
constexpr uint8_t ePreProc =           7;
constexpr uint8_t eComment =           8;
constexpr uint8_t eKeyWord =           9;
constexpr uint8_t eKnownWord =        10;

constexpr uint8_t eCursorPos     =    11;
constexpr uint8_t eCursorLineFill =   12;
constexpr uint8_t eCursorLineEdge =   13;
constexpr uint8_t eCursorReadOnly   = 14;
constexpr uint8_t eSelectCursor    =  15;
constexpr uint8_t eLineNumber =       16;
constexpr uint8_t eWhiteSpace =       17;
constexpr uint8_t eTab =              18;

constexpr uint8_t eFoldClosed =       19;
constexpr uint8_t eFoldOpen =         20;

constexpr uint8_t eScrollBackground = 21;
constexpr uint8_t eScrollGrab =       22;
constexpr uint8_t eScrollHover =      23;
constexpr uint8_t eScrollActive =     24;
constexpr uint8_t eUndefined =      0xFF;
//}}}

//{{{
class cLanguage {
public:
  using tRegex = std::vector <std::pair <std::regex,uint8_t>>;
  using tTokenSearch = bool(*)(const char* srcBegin, const char* srcEnd,
                               const char*& dstBegin, const char*& dstEnd, uint8_t& color);
  // static const
  static const cLanguage c();
  static const cLanguage hlsl();
  static const cLanguage glsl();

  // vars
  std::string mName;

  // comment tokens
  std::string mCommentSingle;
  std::string mCommentBegin;
  std::string mCommentEnd;

  // fold tokens
  std::string mFoldBeginToken;
  std::string mFoldEndToken;

  // fold indicators
  std::string mFoldBeginOpen = "{{{ ";
  std::string mFoldBeginClosed = "... ";
  std::string mFoldEnd = "}}}";

  std::unordered_set <std::string> mKeyWords;
  std::unordered_set <std::string> mKnownWords;

  tTokenSearch mTokenSearch = nullptr;
  cLanguage::tRegex mRegexList;
  };
//}}}
//{{{
class cGlyph {
public:
  //{{{
  cGlyph()
      : mNumUtf8Bytes(1), mColor(0),
        mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {

    mUtfChar[0] =  ' ';
    }
  //}}}
  //{{{
  cGlyph (ImWchar ch, uint8_t color)
      : mColor(color), mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {
  // maximum size 4

    if (ch < 0x80) {
      mUtfChar[0] = static_cast<uint8_t> (ch);
      mNumUtf8Bytes = 1;
      return;
      }

    if (ch < 0x800) {
      mUtfChar[0] = static_cast<uint8_t> (0xc0 + (ch >> 6));
      mUtfChar[1] = static_cast<uint8_t> (0x80 + (ch & 0x3f));
      mNumUtf8Bytes = 2;
      return;
      }

    if ((ch >= 0xdc00) && (ch < 0xe000)) {
      // unknown; mUtfChar[0] = '?';
      mNumUtf8Bytes = 1;
      return;
      }

    if ((ch >= 0xd800) && (ch < 0xdc00)) {
      //mUtfChar[0] = static_cast<uint8_t> (0xf0 + (ch >> 18));
      mUtfChar[0] = static_cast<uint8_t> (0xf0);
      mUtfChar[1] = static_cast<uint8_t> (0x80 + ((ch >> 12) & 0x3f));
      mUtfChar[2] = static_cast<uint8_t> (0x80 + ((ch >> 6) & 0x3f));
      mUtfChar[3] = static_cast<uint8_t> (0x80 + ((ch) & 0x3f));
      mNumUtf8Bytes = 4;
      return;
      }

    mUtfChar[0] = static_cast<uint8_t> (0xe0 + (ch >> 12));
    mUtfChar[1] = static_cast<uint8_t> (0x80 + ((ch >> 6) & 0x3f));
    mUtfChar[2] = static_cast<uint8_t> (0x80 + ((ch) & 0x3f));
    mNumUtf8Bytes = 3;
    }
  //}}}
  //{{{
  cGlyph (const uint8_t* utf8, uint8_t size, uint8_t color)
      : mNumUtf8Bytes(size), mColor(color), mCommentSingle(false), mCommentBegin(false), mCommentEnd(false) {

    // maximum size 6
    if (size <= 6)
      for (uint32_t i = 0; i < mNumUtf8Bytes; i++)
        mUtfChar[i] = *utf8++;
    }
  //}}}

  // utf
  //{{{
  static uint8_t numUtf8Bytes (uint8_t ch) {
  // https://en.wikipedia.org/wiki/UTF-8
  // We assume that the char is a standalone character ( < 128)
  // or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)

    if ((ch & 0xFE) == 0xFC)
      return 6;

    if ((ch & 0xFC) == 0xF8)
      return 5;

    if ((ch & 0xF8) == 0xF0)
      return 4;

    else if ((ch & 0xF0) == 0xE0)
      return 3;

    else if ((ch & 0xE0) == 0xC0)
      return 2;

    return 1;
    }
  //}}}

  // vars
  uint8_t mUtfChar[6];    // expensive
  uint8_t mNumUtf8Bytes;  // expensive
  uint8_t mColor;

  // commentFlags, speeds up wholeText comment parsing
  bool mCommentSingle:1;
  bool mCommentBegin:1;
  bool mCommentEnd:1;
  };
//}}}
//{{{
class cLine {
public:
  cLine() = default;
  //{{{
  cLine (uint32_t indent) {
    for (uint32_t i = 0; i < indent; i++)
      pushBack (cGlyph (' ', eText));
    }
  //}}}
  //{{{
  ~cLine() {
    mGlyphs.clear();
    }
  //}}}

  // gets
  bool empty() const { return mGlyphs.empty(); }
  uint32_t getNumGlyphs() const { return static_cast<uint32_t>(mGlyphs.size()); }

  uint8_t getNumCharBytes (uint32_t glyphIndex) const { return mGlyphs[glyphIndex].mNumUtf8Bytes; }
  uint8_t getChar (uint32_t glyphIndex, uint32_t utf8Index) const { return mGlyphs[glyphIndex].mUtfChar[utf8Index]; }
  uint8_t getChar (uint32_t glyphIndex) const { return getChar (glyphIndex, 0); }

  std::string getString() const;
  uint8_t getColor (uint32_t glyphIndex) const { return mGlyphs[glyphIndex].mColor; }

  bool getCommentSingle (const uint32_t glyphIndex) const { return mGlyphs[glyphIndex].mCommentSingle; }
  bool getCommentBegin (const uint32_t glyphIndex) const { return mGlyphs[glyphIndex].mCommentBegin; }
  bool getCommentEnd (const uint32_t glyphIndex) const { return mGlyphs[glyphIndex].mCommentEnd; }

  cGlyph getGlyph (const uint32_t glyphIndex) const { return mGlyphs[glyphIndex]; }

  // sets
  void setColor (uint8_t color) { for (auto& glyph : mGlyphs) glyph.mColor = color; }
  void setColor (uint32_t glyphIndex, uint8_t color) { mGlyphs[glyphIndex].mColor = color; }
  //{{{
  void setCommentSingle (size_t glyphIndex) {
    mCommentSingle = true;
    mGlyphs[glyphIndex].mCommentSingle = true;
    }
  //}}}
  //{{{
  void setCommentBegin (size_t glyphIndex) {
    mCommentBegin = true;
    mGlyphs[glyphIndex].mCommentBegin = true;
    }
  //}}}
  //{{{
  void setCommentEnd (size_t glyphIndex) {
    mCommentEnd = true;
    mGlyphs[glyphIndex].mCommentEnd = true;
    }
  //}}}

  // ops
  uint32_t trimTrailingSpace();
  void parse (const cLanguage& language);
  void reserve (size_t size) { mGlyphs.reserve (size); }

  // insert
  void pushBack (cGlyph glyph) { mGlyphs.push_back (glyph); }
  void emplaceBack (cGlyph glyph) { mGlyphs.emplace_back (glyph); }
  void insert (uint32_t glyphIndex, const cGlyph& glyph) { mGlyphs.insert (mGlyphs.begin() + glyphIndex, glyph); }
  //{{{
  void appendLine (const cLine& line, uint32_t glyphIndex) {
  // append line from glyphIndex

    mGlyphs.insert (mGlyphs.end(), line.mGlyphs.begin() + glyphIndex, line.mGlyphs.end());
    }
  //}}}

  // erase
  void erase (uint32_t glyphIndex) { mGlyphs.erase (mGlyphs.begin() + glyphIndex); }
  void eraseToEnd (uint32_t glyphIndex) { mGlyphs.erase (mGlyphs.begin() + glyphIndex, mGlyphs.end()); }
  //{{{
  void erase (uint32_t glyphIndex, uint32_t toGlyphIndex) {
    mGlyphs.erase (mGlyphs.begin() + glyphIndex, mGlyphs.begin() + toGlyphIndex);
    }
  //}}}

  // offsets
  uint8_t mIndent = 0;     // leading space count
  uint8_t mFirstGlyph = 0; // index of first visible glyph, past fold marker

  // parsed tokens
  bool mFoldBegin = false;
  bool mFoldEnd = false;

  bool mCommentSingle = false;
  bool mCommentBegin = false;
  bool mCommentEnd = false;

  // fold state
  bool mFoldOpen = false;
  bool mFoldPressed = false;

private:
  std::vector <cGlyph> mGlyphs;

  void parseTokens (const cLanguage& language, const std::string& textString);
  };
//}}}
//{{{
struct sPosition {
  sPosition() : mLineNumber(0), mColumn(0) {}
  sPosition (uint32_t lineNumber, uint32_t column) : mLineNumber(lineNumber), mColumn(column) {}

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

  uint32_t mLineNumber;
  uint32_t mColumn;
  };
//}}}

class cDocument {
public:
  cDocument();
  virtual ~cDocument() = default;

  //{{{  gets
  bool isEdited() const { return mEdited; }
  bool hasFolds() const { return mHasFolds; }
  bool hasUtf8() const { return mHasUtf8; }

  cLanguage& getLanguage() { return mLanguage; }
  uint32_t getTabSize() const { return mTabSize; }

  uint32_t getNumLines() const { return static_cast<uint32_t>(mLines.size()); }
  uint32_t getMaxLineNumber() const { return getNumLines() - 1; }

  std::string getText (const sPosition& beginPosition, const sPosition& endPosition);
  std::string getTextAll() { return getText ({0,0}, { getNumLines(),0}); }
  std::vector<std::string> getTextStrings() const;

  cLine& getLine (uint32_t lineNumber) { return mLines[lineNumber]; }
  uint32_t getNumColumns (const cLine& line);

  uint32_t getGlyphIndex (const cLine& line, uint32_t toColumn);
  uint32_t getGlyphIndex (const sPosition& position) { return getGlyphIndex (getLine (position.mLineNumber), position.mColumn); }

  uint32_t getColumn (const cLine& line, uint32_t toGlyphIndex);
  uint32_t getTabColumn (uint32_t column);
  //}}}

  void addEmptyLine();
  void appendLineToPrev (uint32_t lineNumber);
  void insertChar (cLine& line, uint32_t glyphIndex, ImWchar ch);
  void breakLine (cLine& line, uint32_t glyphIndex, uint32_t newLineNumber, uint32_t indent);
  void joinLine (cLine& joinToLine, uint32_t joinFromLineNumber);

  void deleteChar (cLine& line, uint32_t glyphIndex);
  void deleteChar (cLine& line, const sPosition& position);
  void deleteLine (uint32_t lineNumber);
  void deleteLineRange (uint32_t beginLineNumber, uint32_t endLineNumber);
  void deletePositionRange (const sPosition& beginPosition, const sPosition& endPosition);

  void parse (cLine& line) { line.parse (mLanguage); };
  void parseAll();
  void edited();

  void load (const std::string& filename);
  void save();

private:
  uint32_t trimTrailingSpace();
  //{{{  vars
  std::vector<cLine> mLines;

  std::string mFilePath;
  std::string mParentPath;
  std::string mFileStem;
  std::string mFileExtension;
  uint32_t mVersion = 1;

  bool mHasCR = false;
  bool mHasUtf8 = false;
  bool mHasTabs = false;

  uint32_t mTabSize = 4;

  // temporary state
  bool mHasFolds = false;
  bool mParseFlag = false;
  bool mEdited = false;

  cLanguage mLanguage;
  //}}}
  };
