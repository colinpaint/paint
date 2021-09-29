// cTextEdit.cpp
//{{{  includes
#include "cTextEdit.h"

#include <cmath>
#include <algorithm>
#include <functional>
#include <chrono>

#include "../platform/cPlatform.h"
#include "../imgui/myImguiWidgets.h"
#include "../ui/cApp.h"
#include "../utils/cLog.h"

using namespace std;
using namespace chrono;
//}}}
namespace {
  //{{{  palette const
  constexpr uint8_t eBackground =        0;
  constexpr uint8_t eText =              1;
  constexpr uint8_t eIdentifier =        2;
  constexpr uint8_t eNumber =            3;
  constexpr uint8_t ePunctuation =       4;
  constexpr uint8_t eString =            5;
  constexpr uint8_t eLiteral =           6;
  constexpr uint8_t ePreProc =           7;
  constexpr uint8_t eComment =           8;
  constexpr uint8_t eKeyWord =           9;
  constexpr uint8_t eKnownWord =        10;

  constexpr uint8_t eSelect =           11;
  constexpr uint8_t eCursor =           12;
  constexpr uint8_t eCursorLineFill =   13;
  constexpr uint8_t eCursorLineEdge =   14;
  constexpr uint8_t eLineNumber =       15;
  constexpr uint8_t eWhiteSpace =       16;
  constexpr uint8_t eTab =              17;

  constexpr uint8_t eFoldClosed =       18;
  constexpr uint8_t eFoldOpen =         19;

  constexpr uint8_t eScrollBackground = 20;
  constexpr uint8_t eScrollGrab =       21;
  constexpr uint8_t eScrollHover =      22;
  constexpr uint8_t eScrollActive =     23;
  constexpr uint8_t eUndefined =      0xFF;

  // color to ImU32 lookup
  const vector <ImU32> kPalette = {
    0xffefefef, // eBackground
    0xff808080, // eText
    0xff202020, // eIdentifier
    0xff606000, // eNumber
    0xff404040, // ePunctuation
    0xff2020a0, // eString
    0xff304070, // eLiteral
    0xff008080, // ePreProc
    0xff008000, // eComment
    0xff1010c0, // eKeyWord
    0xff800080, // eKnownWord

    0x80600000, // eSelect
    0xff000000, // eCursor
    0x10000000, // eCursorLineFill
    0x40000000, // eCursorLineEdge
    0xff505000, // eLineNumber
    0xff808080, // eWhiteSpace
    0xff404040, // eTab

    0xffff0000, // eFoldClosed,
    0xff0000ff, // eFoldOpen,

    0x80404040, // eScrollBackground
    0x80ffffff, // eScrollGrab
    0xA000ffff, // eScrollHover
    0xff00ffff, // eScrollActive
    };
  //}}}
  //{{{  language const
  //{{{
  const vector<string> kKeyWords = {
    "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto",
    "bitand", "bitor", "bool", "break",
    "case", "catch", "char", "char16_t", "char32_t", "class", "compl", "concept",
    "const", "constexpr", "const_cast", "continue",
    "decltype", "default", "delete", "do", "double", "dynamic_cast",
    "else", "enum", "explicit", "export", "extern",
    "false", "float", "for", "friend",
    "goto",
    "if", "import", "inline", "int",
    "long",
    "module", "mutable",
    "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
    "operator", "or", "or_eq",
    "private", "protected", "public",
    "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "synchronized",
    "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
    "union", "unsigned", "using",
    "virtual", "void", "volatile",
    "wchar_t", "while",
    "xor", "xor_eq"
    };
  //}}}
  //{{{
  const vector<string> kKnownWords = {
    "abort", "algorithm", "assert",
    "abs", "acos", "array",  "asin", "atan", "atexit", "atof", "atoi", "atol",
    "ceil", "clock", "cosh", "ctime",
    "cmath", "cstdint", "chrono",
    "div",
    "exit",
    "fabs", "floor", "fmod",
    "functional", "fstream",
    "getchar", "getenv",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "isalnum", "isalpha", "isdigit", "isgraph", "ispunct", "isspace", "isupper",
    "kbhit",
    "log10", "log2", "log",
    "map",
    "memcmp", "modf", "min", "max"
    "pow",
    "printf", "putchar", "putenv", "puts",
    "rand", "regex", "remove", "rename",
    "sinh", "sqrt", "srand",
    "std", "string", "sprintf", "snprintf", "strlen", "strcat", "strcmp", "strerror", "set",
    "streambuf",
    "time", "tolower", "toupper",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t", "unordered_map", "unordered_set",
    "vector",
    };
  //}}}

  //{{{
  const vector<string> kHlslKeyWords = {
    "AppendStructuredBuffer", "asm", "asm_fragment", "BlendState", "bool", "break", "Buffer", "ByteAddressBuffer", "case", "cbuffer", "centroid", "class", "column_major", "compile", "compile_fragment",
    "CompileShader", "const", "continue", "ComputeShader", "ConsumeStructuredBuffer", "default", "DepthStencilState", "DepthStencilView", "discard", "do", "double", "DomainShader", "dword", "else",
    "export", "extern", "false", "float", "for", "fxgroup", "GeometryShader", "groupshared", "half", "Hullshader", "if", "in", "inline", "inout", "InputPatch", "int", "interface", "line", "lineadj",
    "linear", "LineStream", "matrix", "min16float", "min10float", "min16int", "min12int", "min16uint", "namespace", "nointerpolation", "noperspective", "NULL", "out", "OutputPatch", "packoffset",
    "pass", "pixelfragment", "PixelShader", "point", "PointStream", "precise", "RasterizerState", "RenderTargetView", "return", "register", "row_major", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer",
    "RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D", "sample", "sampler", "SamplerState", "SamplerComparisonState", "shared", "snorm", "stateblock", "stateblock_state",
    "static", "string", "struct", "switch", "StructuredBuffer", "tbuffer", "technique", "technique10", "technique11", "texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS",
    "Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray", "true", "typedef", "triangle", "triangleadj", "TriangleStream", "uint", "uniform", "unorm", "unsigned", "vector", "vertexfragment",
    "VertexShader", "void", "volatile", "while",
    "bool1","bool2","bool3","bool4","double1","double2","double3","double4", "float1", "float2", "float3", "float4", "int1", "int2", "int3", "int4", "in", "out", "inout",
    "uint1", "uint2", "uint3", "uint4", "dword1", "dword2", "dword3", "dword4", "half1", "half2", "half3", "half4",
    "float1x1","float2x1","float3x1","float4x1","float1x2","float2x2","float3x2","float4x2",
    "float1x3","float2x3","float3x3","float4x3","float1x4","float2x4","float3x4","float4x4",
    "half1x1","half2x1","half3x1","half4x1","half1x2","half2x2","half3x2","half4x2",
    "half1x3","half2x3","half3x3","half4x3","half1x4","half2x4","half3x4","half4x4",
    };
  //}}}
  //{{{
  const vector<string> kHlslKnownWords = {
    "abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asint", "asuint",
    "asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross",
    "D3DCOLORtoUBYTE4", "ddx",
    "ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
    "distance", "dot", "dst", "errorf",
    "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2",
    "f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth",
    "GetRenderTargetSampleCount", "GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync",
    "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange",
    "InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan",
    "ldexp", "length", "lerp", "lit", "log", "log10", "log2",
    "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf",
    "Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
    "ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin",
    "radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt",
    "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
    "tan", "tanh", "tex1D", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2D", "tex2Dbias", "tex2Dgrad", "tex2Dlod", "tex2Dproj",
    "tex3D", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc"
    };
  //}}}

  //{{{
  const vector<string> kGlslKeyWords = {
    "auto", "break",
    "case", "char", "const", "continue",
    "default", "do", "double",
    "else", "enum", "extern",
    "float", "for",
    "goto",
    "if", "inline", "int", "long",
    "register", "restrict", "return",
    "short", "signed", "sizeof", "static", "struct", "switch",
    "typedef", "union", "unsigned",
    "void", "volatile", "while",
    "_Alignas", "_Alignof", "_Atomic",
    "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local"
    };
  //}}}
  //{{{
  const vector<string> kGlslKnownWords = {
    "abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol",
    "ceil", "clock", "cosh", "ctime", "div", "exit",
    "fabs", "floor", "fmod",
    "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
    "ispunct", "isspace", "isupper",
    "kbhit", "log10", "log2", "log",
    "memcmp", "modf", "pow", "putchar",
    "putenv", "puts", "rand", "remove", "rename",
    "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror",
    "time", "tolower", "toupper"
    };
  //}}}
  //}}}

  // utf
  //{{{
  bool isUtfSequence (uint8_t ch) {
    return (ch & 0xC0) == 0x80;
    }
  //}}}
  //{{{
  uint32_t utf8CharLength (uint8_t ch) {
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
  //{{{
  uint32_t imTextCharToUtf8 (char* buf, uint8_t buf_size, uint32_t ch) {

    if (ch < 0x80) {
      buf[0] = (char)ch;
      return 1;
      }

    if (ch < 0x800) {
      if (buf_size < 2)
        return 0;
      buf[0] = (char)(0xc0 + (ch >> 6));
      buf[1] = (char)(0x80 + (ch & 0x3f));
      return 2;
      }

    if ((ch >= 0xdc00) && (ch < 0xe000))
      return 0;

    if ((ch >= 0xd800) && (ch < 0xdc00)) {
      if (buf_size < 4)
        return 0;
      buf[0] = (char)(0xf0 + (ch >> 18));
      buf[1] = (char)(0x80 + ((ch >> 12) & 0x3f));
      buf[2] = (char)(0x80 + ((ch >> 6) & 0x3f));
      buf[3] = (char)(0x80 + ((ch) & 0x3f));
      return 4;
      }

    //else c < 0x10000
    if (buf_size < 3)
      return 0;

    buf[0] = (char)(0xe0 + (ch >> 12));
    buf[1] = (char)(0x80 + ((ch >> 6) & 0x3f));
    buf[2] = (char)(0x80 + ((ch) & 0x3f));
    return 3;
    }
  //}}}

  // faster parsers
  //{{{
  bool findIdentifier (const char* srcBegin, const char* srcEnd, const char*& tokenBegin, const char*& tokenEnd) {

    const char* srcPtr = srcBegin;

    if (((*srcPtr >= 'a') && (*srcPtr <= 'z')) ||
        ((*srcPtr >= 'A') && (*srcPtr <= 'Z')) ||
        (*srcPtr == '_')) {
      srcPtr++;

      while ((srcPtr < srcEnd) &&
             (((*srcPtr >= 'a') && (*srcPtr <= 'z')) ||
              ((*srcPtr >= 'A') && (*srcPtr <= 'Z')) ||
              ((*srcPtr >= '0') && (*srcPtr <= '9')) ||
              (*srcPtr == '_')))
        srcPtr++;

      tokenBegin = srcBegin;
      tokenEnd = srcPtr;
      return true;
      }

    return false;
    }
  //}}}
  //{{{
  bool findNumber (const char* srcBegin, const char* srcEnd, const char*& tokenBegin, const char*& tokenEnd) {

    const char* srcPtr = srcBegin;

    // skip leading sign +-
    if ((*srcPtr == '+') && (*srcPtr == '-'))
      srcPtr++;

    // skip digits 0..9
    bool isNumber = false;
    while ((srcPtr < srcEnd) &&
           ((*srcPtr >= '0') && (*srcPtr <= '9'))) {
      isNumber = true;
      srcPtr++;
      }
    if (!isNumber)
      return false;

    bool isHex = false;
    bool isFloat = false;
    bool isBinary = false;

    if (srcPtr < srcEnd) {
      if (*srcPtr == '.') {
        //{{{  skip floating point frac
        isFloat = true;
        srcPtr++;
        while ((srcPtr < srcEnd) &&
               ((*srcPtr >= '0') && (*srcPtr <= '9')))
          srcPtr++;
        }
        //}}}
      else if ((*srcPtr == 'x') || (*srcPtr == 'X')) {
        //{{{  skip hex 0xhhhh
        isHex = true;

        srcPtr++;
        while ((srcPtr < srcEnd) &&
               (((*srcPtr >= '0') && (*srcPtr <= '9')) ||
                ((*srcPtr >= 'a') && (*srcPtr <= 'f')) ||
                ((*srcPtr >= 'A') && (*srcPtr <= 'F'))))
          srcPtr++;
        }
        //}}}
      else if ((*srcPtr == 'b') || (*srcPtr == 'B')) {
        //{{{  skip binary 0b0101010
        isBinary = true;

        srcPtr++;
        while ((srcPtr < srcEnd) && ((*srcPtr >= '0') && (*srcPtr <= '1')))
          srcPtr++;
        }
        //}}}
      }

    if (!isHex && !isBinary) {
      //{{{  skip floating point exponent e+dd
      if (srcPtr < srcEnd && (*srcPtr == 'e' || *srcPtr == 'E')) {
        isFloat = true;

        srcPtr++;
        if (srcPtr < srcEnd && (*srcPtr == '+' || *srcPtr == '-'))
          srcPtr++;

        bool hasDigits = false;
        while (srcPtr < srcEnd && (*srcPtr >= '0' && *srcPtr <= '9')) {
          hasDigits = true;
          srcPtr++;
          }
        if (hasDigits == false)
          return false;
        }

      // single pecision floating point type
      if (srcPtr < srcEnd && *srcPtr == 'f')
        srcPtr++;
      }
      //}}}
    if (!isFloat) {
      //{{{  skip trailing integer size ulUL letter
      while ((srcPtr < srcEnd) &&
             ((*srcPtr == 'u') || (*srcPtr == 'U') || (*srcPtr == 'l') || (*srcPtr == 'L')))
        srcPtr++;
      }
      //}}}

    tokenBegin = srcBegin;
    tokenEnd = srcPtr;
    return true;
    }
  //}}}
  //{{{
  bool findPunctuation (const char* srcBegin, const char*& tokenBegin, const char*& tokenEnd) {

    switch (*srcBegin) {
      case '[':
      case ']':
      case '{':
      case '}':
      case '!':
      case '%':
      case '^':
      case '&':
      case '*':
      case '(':
      case ')':
      case '-':
      case '+':
      case '=':
      case '~':
      case '|':
      case '<':
      case '>':
      case '?':
      case ':':
      case '/':
      case ';':
      case ',':
      case '.':
        tokenBegin = srcBegin;
        tokenEnd = srcBegin + 1;
        return true;
      }

    return false;
    }
  //}}}
  //{{{
  bool findString (const char* srcBegin, const char* srcEnd, const char*& tokenBegin, const char*& tokenEnd) {

    const char* srcPtr = srcBegin;

    if (*srcPtr == '"') {
      srcPtr++;

      while (srcPtr < srcEnd) {
        // handle end of string
        if (*srcPtr == '"') {
          tokenBegin = srcBegin;
          tokenEnd = srcPtr + 1;
          return true;
         }

        // handle escape character for "
        if ((*srcPtr == '\\') && (srcPtr + 1 < srcEnd) && (srcPtr[1] == '"'))
          srcPtr++;

        srcPtr++;
        }
      }

    return false;
    }
  //}}}
  //{{{
  bool findLiteral (const char* srcBegin, const char* srcEnd, const char*& tokenBegin, const char*& tokenEnd) {

    const char* srcPtr = srcBegin;

    if (*srcPtr == '\'') {
      srcPtr++;

      // handle escape characters
      if ((srcPtr < srcEnd) && (*srcPtr == '\\'))
        srcPtr++;

      if (srcPtr < srcEnd)
        srcPtr++;

      // handle end of literal
      if ((srcPtr < srcEnd) && (*srcPtr == '\'')) {
        tokenBegin = srcBegin;
        tokenEnd = srcPtr + 1;
        return true;
        }
      }

    return false;
    }
  //}}}
  }

// cTextEdit
//{{{
cTextEdit::cTextEdit() {

  setLanguage (cLanguage::c());

  mDoc.mLines.push_back (cLine::tGlyphs());
  mCursorFlashTimePoint = system_clock::now();
  }
//}}}
//{{{  gets
//{{{
bool cTextEdit::hasClipboardText() {

  const char* clipText = ImGui::GetClipboardText();
  return clipText && (strlen (clipText) > 0);
  }
//}}}

//{{{
string cTextEdit::getTextString() {
// get text as single string

  return getText ({0,0}, {getNumLines(),0});
  }
//}}}

//{{{
vector<string> cTextEdit::getTextStrings() const {
// get text as vector of string

  vector<string> result;
  result.reserve (getNumLines());

  for (auto& line : mDoc.mLines) {
    string lineString;
    lineString.resize (line.getNumGlyphs());
    for (size_t i = 0; i < line.getNumGlyphs(); ++i)
      lineString[i] = line.mGlyphs[i].mChar;

    result.emplace_back (move (lineString));
    }

  return result;
  }
//}}}
//}}}
//{{{  sets
//{{{
void cTextEdit::setTextString (const string& text) {
// break test into lines, store in internal mLines structure

  mDoc.mLines.clear();
  mDoc.mLines.emplace_back (cLine::tGlyphs());

  for (auto ch : text) {
    if (ch == '\r') // ignored but flag set
      mDoc.mHasCR = true;
    else if (ch == '\n') {
      parseLine (mDoc.mLines.back());
      mDoc.mLines.emplace_back (cLine::tGlyphs());
      }
    else {
      if (ch ==  '\t')
        mDoc.mHasTabs = true;
      mDoc.mLines.back().mGlyphs.emplace_back (cGlyph (ch, eText));
      }
    }

  mDoc.mEdited = false;

  mUndoList.mBuffer.clear();
  mUndoList.mIndex = 0;
  }
//}}}
//{{{
void cTextEdit::setTextStrings (const vector<string>& lines) {
// store vector of lines in internal mLines structure

  mDoc.mLines.clear();

  if (lines.empty())
    mDoc.mLines.emplace_back (cLine::tGlyphs());
  else {
    mDoc.mLines.resize (lines.size());
    for (size_t i = 0; i < lines.size(); ++i) {
      const string& line = lines[i];
      for (size_t j = 0; j < line.size(); ++j) {
        if (line[j] ==  '\t')
          mDoc.mHasTabs = true;
        mDoc.mLines[i].mGlyphs.emplace_back (cGlyph (line[j], eText));
        }
      parseLine (mDoc.mLines[i]);
      }
    }

  mDoc.mEdited = false;

  mUndoList.mBuffer.clear();
  mUndoList.mIndex = 0;
  }
//}}}

//{{{
void cTextEdit::setLanguage (const cLanguage& language) {

  mOptions.mLanguage = language;
  }
//}}}
//}}}
//{{{  actions
// move
//{{{
void cTextEdit::moveLeft() {

  // lineNumber
  sPosition position = mEdit.mCursor.mPosition;
  uint32_t lineNumber = position.mLineNumber;

  // column
  uint32_t column = getCharacterIndex (mEdit.mCursor.mPosition);
  if (column == 0) {
    // move to end of prevous line
    if (lineNumber > 0) {
      --lineNumber;
      if (lineNumber < getNumLines())
        column = getNumGlyphs (lineNumber);
      else
        column = 0;
      }
    }
  else // move to previous column on same line
    while ((--column > 0) && isUtfSequence (getGlyphs (lineNumber)[column].mChar)) {}

  mEdit.mCursor.mPosition = {lineNumber, getCharacterColumn (lineNumber, column)};
  if (mEdit.mCursor.mPosition != position) {
    // cursor changed
    mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
    mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
    setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, eSelection::eNormal);
    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::moveRight() {

  // lineNumber
  sPosition position = mEdit.mCursor.mPosition;
  uint32_t lineNumber = position.mLineNumber;

  // column
  uint32_t column = getCharacterIndex (mEdit.mCursor.mPosition);
  if (column >= getNumGlyphs (lineNumber)) {
    // move to start of next line
    if (mEdit.mCursor.mPosition.mLineNumber < getNumLines()-1) {
      mEdit.mCursor.mPosition.mLineNumber =
        max (0u, min (getNumLines()-1, mEdit.mCursor.mPosition.mLineNumber+1));
      mEdit.mCursor.mPosition.mColumn = 0;
      }
    else
      return;
    }
  else {
    // move to next columm on same line
    column += utf8CharLength (getGlyphs (lineNumber)[column].mChar);
    mEdit.mCursor.mPosition = {lineNumber, getCharacterColumn (lineNumber, column)};
    }

  if (mEdit.mCursor.mPosition != position) {
    // cursor changed
    mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
    mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
    setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, eSelection::eNormal);
    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::moveHome() {

  sPosition position = mEdit.mCursor.mPosition;
  setCursorPosition ({0,0});

  if (mEdit.mCursor.mPosition != position) {
    // cursor changed
    mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
    mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
    setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, eSelection::eNormal);
    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::moveEnd() {

  sPosition position = mEdit.mCursor.mPosition;
  setCursorPosition ({getNumLines()-1, 0});

  if (mEdit.mCursor.mPosition != position) {
    // cursor changed
    mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
    mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
    setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, eSelection::eNormal);
    scrollCursorVisible();
    }
  }
//}}}

// select
//{{{
void cTextEdit::selectAll() {
  setSelection ({0,0}, {getNumLines(),0}, eSelection::eNormal);
  }
//}}}
//{{{
void cTextEdit::selectWordUnderCursor() {

  sPosition cursorPosition = getCursorPosition();
  setSelection (findWordBegin (cursorPosition), findWordEnd (cursorPosition), eSelection::eNormal);
  }
//}}}

// cut and paste
//{{{
void cTextEdit::copy() {

  if (hasSelect())
    ImGui::SetClipboardText (getSelectedText().c_str());

  string copyString;
  for (auto& glyph : getGlyphs (getCursorPosition().mLineNumber))
    copyString.push_back (glyph.mChar);

  // copy as text to clipBoard
  ImGui::SetClipboardText (copyString.c_str());
  }
//}}}
//{{{
void cTextEdit::cut() {

  if (isReadOnly())
    copy();

  else if (hasSelect()) {
    cUndo undo;
    undo.mBefore = mEdit.mCursor;
    undo.mRemoved = getSelectedText();
    undo.mRemovedBegin = mEdit.mCursor.mSelectionBegin;
    undo.mRemovedEnd = mEdit.mCursor.mSelectionEnd;

    copy();
    deleteSelection();

    undo.mAfter = mEdit.mCursor;
    addUndo (undo);
    }
  }
//}}}
//{{{
void cTextEdit::paste() {

  if (isReadOnly())
    return;

  if (hasClipboardText()) {
    cUndo undo;
    undo.mBefore = mEdit.mCursor;

    if (hasSelect()) {
      // save and delete selection
      undo.mRemoved = getSelectedText();
      undo.mRemovedBegin = mEdit.mCursor.mSelectionBegin;
      undo.mRemovedEnd = mEdit.mCursor.mSelectionEnd;
      deleteSelection();
      }

    string clipboardString = ImGui::GetClipboardText();
    undo.mAdded = clipboardString;
    undo.mAddedBegin = getCursorPosition();

    insertText (clipboardString);

    undo.mAddedEnd = getCursorPosition();
    undo.mAfter = mEdit.mCursor;
    addUndo (undo);
    }
  }
//}}}

// delete
//{{{
void cTextEdit::deleteIt() {

  cUndo undo;
  undo.mBefore = mEdit.mCursor;

  if (hasSelect()) {
    undo.mRemoved = getSelectedText();
    undo.mRemovedBegin = mEdit.mCursor.mSelectionBegin;
    undo.mRemovedEnd = mEdit.mCursor.mSelectionEnd;
    deleteSelection();
    }

  else {
    sPosition position = getCursorPosition();
    setCursorPosition (position);
    cLine& line = getLine (position.mLineNumber);

    if (position.mColumn == getLineMaxColumn (position.mLineNumber)) {
      if (position.mLineNumber == getNumLines()-1)
        return;

      undo.mRemoved = '\n';
      undo.mRemovedBegin = undo.mRemovedEnd = getCursorPosition();
      advance (undo.mRemovedEnd);

      cLine& nextLine = getLine (position.mLineNumber+1);
      line.mGlyphs.insert (line.mGlyphs.end(), nextLine.mGlyphs.begin(), nextLine.mGlyphs.end());
      parseLine (line);

      removeLine (position.mLineNumber + 1);
      }

    else {
      undo.mRemovedBegin = undo.mRemovedEnd = getCursorPosition();
      undo.mRemovedEnd.mColumn++;
      undo.mRemoved = getText (undo.mRemovedBegin, undo.mRemovedEnd);

      uint32_t characterIndex = getCharacterIndex (position);
      uint32_t length = utf8CharLength (line.mGlyphs[characterIndex].mChar);
      while ((length > 0) && (characterIndex < line.getNumGlyphs())) {
        line.mGlyphs.erase (line.mGlyphs.begin() + characterIndex);
        length--;
        }
      parseLine (line);
      }

    mDoc.mEdited = true;
    }

  undo.mAfter = mEdit.mCursor;
  addUndo (undo);
  }
//}}}
//{{{
void cTextEdit::backspace() {

  cUndo undo;
  undo.mBefore = mEdit.mCursor;

  if (hasSelect()) {
    //{{{  backspace delete selection
    undo.mRemoved = getSelectedText();
    undo.mRemovedBegin = mEdit.mCursor.mSelectionBegin;
    undo.mRemovedEnd = mEdit.mCursor.mSelectionEnd;

    deleteSelection();
    }
    //}}}

  else {
    sPosition position = getCursorPosition();
    setCursorPosition (position);
    if (mEdit.mCursor.mPosition.mColumn == 0) {
      if (mEdit.mCursor.mPosition.mLineNumber == 0)
        return;

      undo.mRemoved = '\n';
      undo.mRemovedBegin = {position.mLineNumber - 1, getLineMaxColumn (position.mLineNumber - 1)};
      undo.mRemovedEnd = undo.mRemovedBegin;
      advance (undo.mRemovedEnd);

      cLine& line = getLine (mEdit.mCursor.mPosition.mLineNumber);
      cLine& prevLine = getLine (mEdit.mCursor.mPosition.mLineNumber-1);

      uint32_t prevSize = getLineMaxColumn (mEdit.mCursor.mPosition.mLineNumber - 1);
      prevLine.mGlyphs.insert (prevLine.mGlyphs.end(), line.mGlyphs.begin(), line.mGlyphs.end());
      removeLine (mEdit.mCursor.mPosition.mLineNumber);
      --mEdit.mCursor.mPosition.mLineNumber;
      mEdit.mCursor.mPosition.mColumn = prevSize;
      }

    else {
      uint32_t characterIndex = getCharacterIndex (position) - 1;
      uint32_t characterIndexEnd = characterIndex + 1;
      cLine& line = getLine (mEdit.mCursor.mPosition.mLineNumber);
      while ((characterIndex > 0) && isUtfSequence (line.mGlyphs[characterIndex].mChar))
        --characterIndex;

      undo.mRemovedBegin = undo.mRemovedEnd = getCursorPosition();
      --undo.mRemovedBegin.mColumn;
      --mEdit.mCursor.mPosition.mColumn;

      while ((characterIndex < line.getNumGlyphs()) && (characterIndexEnd-- > characterIndex)) {
        undo.mRemoved += line.mGlyphs[characterIndex].mChar;
        line.mGlyphs.erase (line.mGlyphs.begin() + characterIndex);
        }
      parseLine (line);
      }
    mDoc.mEdited = true;

    scrollCursorVisible();
    }

  undo.mAfter = mEdit.mCursor;
  addUndo (undo);
  }
//}}}
//{{{
void cTextEdit::deleteSelection() {

  if (mEdit.mCursor.mSelectionEnd == mEdit.mCursor.mSelectionBegin)
    return;

  deleteRange (mEdit.mCursor.mSelectionBegin, mEdit.mCursor.mSelectionEnd);

  setSelection (mEdit.mCursor.mSelectionBegin, mEdit.mCursor.mSelectionBegin, eSelection::eNormal);
  setCursorPosition (mEdit.mCursor.mSelectionBegin);
  }
//}}}

// insert
//{{{
void cTextEdit::enterCharacter (ImWchar ch, bool shift) {

  cUndo undo;
  undo.mBefore = mEdit.mCursor;

  if (hasSelect()) {
    if ((ch == '\t') && (mEdit.mCursor.mSelectionBegin.mLineNumber != mEdit.mCursor.mSelectionEnd.mLineNumber)) {
      //{{{  tab insert into selection
      sPosition selectBegin = mEdit.mCursor.mSelectionBegin;
      sPosition selectEnd = mEdit.mCursor.mSelectionEnd;
      sPosition originalEnd = selectEnd;

      if (selectBegin > selectEnd)
        swap (selectBegin, selectEnd);

      selectBegin.mColumn = 0;
      if ((selectEnd.mColumn == 0) && (selectEnd.mLineNumber > 0))
        --selectEnd.mLineNumber;
      if (selectEnd.mLineNumber >= getNumLines())
        selectEnd.mLineNumber = getNumLines()-1;
      selectEnd.mColumn = getLineMaxColumn (selectEnd.mLineNumber);

      undo.mRemovedBegin = selectBegin;
      undo.mRemovedEnd = selectEnd;
      undo.mRemoved = getText (selectBegin, selectEnd);

      bool modified = false;
      for (uint32_t i = selectBegin.mLineNumber; i <= selectEnd.mLineNumber; i++) {
        auto& glyphs = getGlyphs (i);
        if (shift) {
          if (!glyphs.empty()) {
            if (glyphs.front().mChar == '\t') {
              glyphs.erase (glyphs.begin());
              modified = true;
              }
            else {
              for (uint32_t j = 0; (j < mDoc.mTabSize) && !glyphs.empty() && (glyphs.front().mChar == ' '); j++) {
                glyphs.erase (glyphs.begin());
                modified = true;
                }
              }
            }
          }
        else {
          glyphs.insert (glyphs.begin(), cGlyph ('\t', eTab));
          modified = true;
          }
        }

      if (modified) {
        //{{{  not sure what this does yet
        selectBegin = { selectBegin.mLineNumber, getCharacterColumn (selectBegin.mLineNumber, 0)};
        sPosition rangeEnd;
        if (originalEnd.mColumn != 0) {
          selectEnd = {selectEnd.mLineNumber, getLineMaxColumn (selectEnd.mLineNumber)};
          rangeEnd = selectEnd;
          undo.mAdded = getText (selectBegin, selectEnd);
          }
        else {
          selectEnd = {originalEnd.mLineNumber, 0};
          rangeEnd = {selectEnd.mLineNumber - 1, getLineMaxColumn (selectEnd.mLineNumber - 1)};
          undo.mAdded = getText (selectBegin, rangeEnd);
          }

        undo.mAddedBegin = selectBegin;
        undo.mAddedEnd = rangeEnd;
        undo.mAfter = mEdit.mCursor;

        mEdit.mCursor.mSelectionBegin = selectBegin;
        mEdit.mCursor.mSelectionEnd = selectEnd;
        addUndo (undo);
        mDoc.mEdited = true;

        scrollCursorVisible();
        }
        //}}}

      return;
      }
      //}}}
    else {
      //{{{  deleteSelection before insert
      undo.mRemoved = getSelectedText();
      undo.mRemovedBegin = mEdit.mCursor.mSelectionBegin;
      undo.mRemovedEnd = mEdit.mCursor.mSelectionEnd;

      deleteSelection();
      }
      //}}}
    }

  sPosition position = getCursorPosition();
  undo.mAddedBegin = position;

  if (ch == '\n') {
    //{{{  enter carraigeReturn into line
    cLine& line = getLine (position.mLineNumber);

    // insert newLine
    insertLine (position.mLineNumber+1);
    cLine& newLine = getLine (position.mLineNumber+1);

    if (mOptions.mLanguage.mAutoIndentation)
      for (size_t i = 0;
          (i < line.getNumGlyphs()) && isascii (line.mGlyphs[i].mChar) && isblank (line.mGlyphs[i].mChar); ++i)
        newLine.mGlyphs.push_back (line.mGlyphs[i]);
    uint32_t indentSize = newLine.getNumGlyphs();

    // insert indent and rest of old line
    uint32_t characterIndex = getCharacterIndex (position);
    newLine.mGlyphs.insert (newLine.mGlyphs.end(), line.mGlyphs.begin() + characterIndex, line.mGlyphs.end());
    parseLine (newLine);

    // erase rest of old line
    line.mGlyphs.erase (line.mGlyphs.begin() + characterIndex, line.mGlyphs.begin() + line.getNumGlyphs());
    parseLine (line);

    // set cursor
    setCursorPosition ({position.mLineNumber+1, getCharacterColumn (position.mLineNumber+1, indentSize)});
    undo.mAdded = (char)ch;
    }
    //}}}
  else {
    // enter ImWchar
    array <char,7> utf8buf;
    uint32_t bufLength = imTextCharToUtf8 (utf8buf.data(), 7, ch);
    if (bufLength > 0) {
      //{{{  enter char into line
      utf8buf[bufLength] = '\0';
      cLine& line = getLine (position.mLineNumber);
      uint32_t characterIndex = getCharacterIndex (position);
      if (mOptions.mOverWrite && (characterIndex < line.getNumGlyphs())) {
        uint32_t length = utf8CharLength(line.mGlyphs[characterIndex].mChar);
        undo.mRemovedBegin = mEdit.mCursor.mPosition;
        undo.mRemovedEnd = {position.mLineNumber, getCharacterColumn (position.mLineNumber, characterIndex + length)};
        while ((length > 0) && (characterIndex < line.getNumGlyphs())) {
          undo.mRemoved += line.mGlyphs[characterIndex].mChar;
          line.mGlyphs.erase (line.mGlyphs.begin() + characterIndex);
          length--;
          }
        }

      for (char* bufPtr = utf8buf.data(); *bufPtr != '\0'; bufPtr++, ++characterIndex)
        line.mGlyphs.insert (line.mGlyphs.begin() + characterIndex, cGlyph (*bufPtr, eText));

      parseLine (line);

      undo.mAdded = utf8buf.data();
      setCursorPosition ({position.mLineNumber, getCharacterColumn (position.mLineNumber, characterIndex)});
      }
      //}}}
    else
      return;
    }
  mDoc.mEdited = true;

  undo.mAddedEnd = getCursorPosition();
  undo.mAfter = mEdit.mCursor;
  addUndo (undo);

  scrollCursorVisible();
  }
//}}}

// fold
//{{{
void cTextEdit::createFold() {

  if (isReadOnly())
    return;

  // !!!! temp string for now !!!!
  string insertString = mOptions.mLanguage.mFoldBeginMarker +
                        "  new fold - loads of detail to implement\n\n" +
                        mOptions.mLanguage.mFoldEndMarker +
                        "\n";
  cUndo undo;
  undo.mBefore = mEdit.mCursor;
  undo.mAdded = insertString;
  undo.mAddedBegin = getCursorPosition();

  insertText (insertString);

  undo.mAddedEnd = getCursorPosition();
  undo.mAfter = mEdit.mCursor;
  addUndo (undo);
  }
//}}}

// undo
//{{{
void cTextEdit::undo (int steps) {

  while (hasUndo() && steps-- > 0)
    mUndoList.mBuffer[--mUndoList.mIndex].undo (this);
  }
//}}}
//{{{
void cTextEdit::redo (int steps) {

  while (hasRedo() && steps-- > 0)
    mUndoList.mBuffer[mUndoList.mIndex++].redo (this);
  }
//}}}
//}}}

// draws
//{{{
void cTextEdit::drawWindow (const string& title, cApp& app) {
// standalone Memory Editor window

  mOpen = true;

  ImGui::Begin (title.c_str(), &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

  drawContents (app);
  ImGui::End();
  }
//}}}
//{{{
void cTextEdit::drawContents (cApp& app) {
// main ui handle io and draw routine

  parseComments();

  drawTop (app);

  // begin childWindow, new font, new colors
  if (isDrawMonoSpaced())
    ImGui::PushFont (app.getMonoFont());

  ImGui::PushStyleColor (ImGuiCol_ScrollbarBg, kPalette[eScrollBackground]);
  ImGui::PushStyleColor (ImGuiCol_ScrollbarGrab, kPalette[eScrollGrab]);
  ImGui::PushStyleColor (ImGuiCol_ScrollbarGrabHovered, kPalette[eScrollHover]);
  ImGui::PushStyleColor (ImGuiCol_ScrollbarGrabActive, kPalette[eScrollActive]);
  ImGui::PushStyleColor (ImGuiCol_Text, kPalette[eText]);
  ImGui::PushStyleColor (ImGuiCol_ChildBg, kPalette[eBackground]);
  ImGui::PushStyleVar (ImGuiStyleVar_ScrollbarSize, mContext.mGlyphWidth*2.f);
  ImGui::PushStyleVar (ImGuiStyleVar_ScrollbarRounding, 2.f);
  ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, {0.f,0.f});
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, {0.f,0.f});
  ImGui::BeginChild ("##s", {0.f,0.f}, false,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);

  mContext.update (mOptions);

  handleKeyboard();

  if (isFolded())
    mDoc.mFoldLines.resize (drawFolded());
  else
    drawUnfolded();

  ImGui::EndChild();
  ImGui::PopStyleVar (4);
  ImGui::PopStyleColor (6);
  if (isDrawMonoSpaced())
    ImGui::PopFont();
  }
//}}}

// private:
//{{{  gets
//{{{
uint32_t cTextEdit::getNumPageLines() const {
  float height = ImGui::GetWindowHeight() - 20.f;
  return static_cast<uint32_t>(floor (height / mContext.mLineHeight));
  }
//}}}

// text
//{{{
float cTextEdit::getTextWidth (sPosition position) {
// get width of text in pixels, of position lineNumberline,  up to position column

  const auto& glyphs = getGlyphs (position.mLineNumber);

  float distance = 0.f;
  uint32_t characterIndex = getCharacterIndex (position);
  for (uint32_t i = 0; (i < glyphs.size()) && (i < characterIndex);) {
    if (glyphs[i].mChar == '\t') {
      // tab
      distance = getTabEndPosX (distance);
      ++i;
      }
    else {
      int length = utf8CharLength (glyphs[i].mChar);
      array <char,7> str;
      int j = 0;
      for (; (j < 6) && (length-- > 0) && (i < glyphs.size()); j++, i++)
        str[j] = glyphs[i].mChar;
      distance += mContext.measureText (str.data(), str.data()+j);
      }
    }

  return distance;
  }
//}}}
//{{{
string cTextEdit::getText (sPosition beginPosition, sPosition endPosition) {
// get text as string with lineFeed line breaks

  uint32_t beginLineNumber = beginPosition.mLineNumber;
  uint32_t endLineNumber = endPosition.mLineNumber;

  // count approx num chars, reserve
  uint32_t numChars = 0;
  for (uint32_t lineNumber = beginLineNumber; lineNumber < endLineNumber; lineNumber++)
    numChars += getNumGlyphs (lineNumber);
  string textString;
  textString.reserve (numChars + (numChars / 8));

  uint32_t beginCharacterIndex = getCharacterIndex (beginPosition);
  uint32_t endCharacterIndex = getCharacterIndex (endPosition);
  while ((beginCharacterIndex < endCharacterIndex) || (beginLineNumber < endLineNumber)) {
    if (beginLineNumber >= getNumLines())
      break;

    if (beginCharacterIndex < getGlyphs (beginLineNumber).size()) {
      textString += getGlyphs (beginLineNumber)[beginCharacterIndex].mChar;
      beginCharacterIndex++;
      }
    else {
      beginCharacterIndex = 0;
      ++beginLineNumber;
      textString += '\n';
      }
    }

  return textString;
  }
//}}}

// lines
//{{{
uint32_t cTextEdit::getLineNumberFromIndex (uint32_t lineIndex) const {

  if (!isFolded()) // lineNumber is lineIndex
    return lineIndex;

  if (lineIndex < getNumFoldLines())
    return mDoc.mFoldLines[lineIndex];

  cLog::log (LOGERROR, fmt::format ("getLineNumberFromIndex {} no line for that index", lineIndex));
  return 0;
  }
//}}}
//{{{
uint32_t cTextEdit::getLineIndexFromNumber (uint32_t lineNumber) const {

  if (!isFolded()) // lineIndex is lineNumber
    return lineNumber;

  if (mDoc.mFoldLines.empty()) {
    // no foldLines
    cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber {} mFoldLines empty", lineNumber));
    return 0;
    }

  auto it = find (mDoc.mFoldLines.begin(), mDoc.mFoldLines.end(), lineNumber);
  if (it == mDoc.mFoldLines.end()) {
    // no lineNumber for that index
    cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber {} notFound", lineNumber));
    return 0;
    }
  else
    return uint32_t(it - mDoc.mFoldLines.begin());
  }
//}}}

// column
//{{{
uint32_t cTextEdit::getCharacterIndex (sPosition position) {

  if (position.mLineNumber >= getNumLines()) {
    cLog::log (LOGERROR, "cTextEdit::getCharacterIndex - lineNumber too big");
    return 0;
    }

  uint32_t column = 0;
  uint32_t i = 0;
  const auto& glyphs = getGlyphs (position.mLineNumber);
  for (; (i < glyphs.size()) && (column < position.mColumn);) {
    if (glyphs[i].mChar == '\t')
      column = getTabColumn (column);
    else
      ++column;
    i += utf8CharLength (glyphs[i].mChar);
    }

  return i;
  }
//}}}
//{{{
uint32_t cTextEdit::getCharacterColumn (uint32_t lineNumber, uint32_t characterIndex) {
// handle tabs

  if (lineNumber >= getNumLines())
    return 0;

  uint32_t column = 0;
  uint32_t i = 0;
  const auto& glyphs = getGlyphs (lineNumber);
  while ((i < characterIndex) && (i < glyphs.size())) {
    uint8_t ch = glyphs[i].mChar;
    i += utf8CharLength (ch);
    if (ch == '\t')
      column = getTabColumn (column);
    else
      column++;
    }

  return column;
  }
//}}}
//{{{
uint32_t cTextEdit::getLineNumChars (uint32_t lineNumber) {

  if (lineNumber >= getNumLines())
    return 0;

  uint32_t numChars = 0;
  const auto& glyphs = getGlyphs (lineNumber);
  for (size_t i = 0; i < glyphs.size(); numChars++)
    i += utf8CharLength (glyphs[i].mChar);

  return numChars;
  }
//}}}
//{{{
uint32_t cTextEdit::getLineMaxColumn (uint32_t lineNumber) {

  if (lineNumber >= getNumLines())
    return 0;

  uint32_t column = 0;
  const auto& glyphs = getGlyphs (lineNumber);
  for (size_t i = 0; i < glyphs.size(); ) {
    uint8_t ch = glyphs[i].mChar;
    if (ch == '\t')
      column = getTabColumn (column);
    else
      column++;
    i += utf8CharLength (ch);
    }

  return column;
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::getPositionFromPosX (uint32_t lineNumber, float posX) {

  uint32_t column = 0;
  if (lineNumber < getNumLines()) {
    float columnX = 0.f;
    size_t glyphIndex = 0;
    const auto& glyphs = getGlyphs (lineNumber);
    while (glyphIndex < glyphs.size()) {
      float columnWidth = 0.f;
      if (glyphs[glyphIndex].mChar == '\t') {
        // tab
        float oldX = columnX;
        float endTabX = getTabEndPosX (columnX);
        columnWidth = endTabX - oldX;
        if (columnX + (columnWidth/2.f) > posX)
          break;
        columnX = endTabX;
        column = getTabColumn (column);
        glyphIndex++;
        }

      else {
        // not tab
        array <char,7> str;
        int length = utf8CharLength (glyphs[glyphIndex].mChar);

        size_t i = 0;
        while ((i < str.max_size()-1) && (length-- > 0))
          str[i++] = glyphs[glyphIndex++].mChar;

        columnWidth = mContext.measureText (str.data(), str.data()+i);
        if (columnX + (columnWidth/2.f) > posX)
          break;

        columnX += columnWidth;
        column++;
        }
      }
    }

  return sanitizePosition ({lineNumber, column});
  }
//}}}

// word
//{{{
bool cTextEdit::isOnWordBoundary (sPosition position) {

  if ((position.mLineNumber >= getNumLines()) || (position.mColumn == 0))
    return true;

  const auto& glyphs = getGlyphs (position.mLineNumber);
  uint32_t characterIndex = getCharacterIndex (position);
  if (characterIndex >= glyphs.size())
    return true;

  return glyphs[characterIndex].mColor != glyphs[characterIndex - 1].mColor;
  }
//}}}
//{{{
string cTextEdit::getWordAt (sPosition position) {

  string result;
  for (uint32_t i = getCharacterIndex (findWordBegin (position)); i < getCharacterIndex (findWordEnd (position)); ++i)
    result.push_back (getGlyphs (position.mLineNumber)[i].mChar);

  return result;
  }
//}}}

// tab
//{{{
uint32_t cTextEdit::getTabColumn (uint32_t column) {
  return ((column / mDoc.mTabSize) * mDoc.mTabSize) + mDoc.mTabSize;
  }
//}}}
//{{{
float cTextEdit::getTabEndPosX (float xPos) {
// return tabEnd xPos of tab containing xPos

  float tabWidthPixels = mDoc.mTabSize * mContext.mGlyphWidth;
  return (1.f + floor ((1.f + xPos) / tabWidthPixels)) * tabWidthPixels;
  }
//}}}
//}}}
//{{{  sets
//{{{
void cTextEdit::setCursorPosition (sPosition position) {

  if (mEdit.mCursor.mPosition != position) {
    mEdit.mCursor.mPosition = position;
    scrollCursorVisible();
    }
  }
//}}}

//{{{
void cTextEdit::setSelectionBegin (sPosition position) {

  mEdit.mCursor.mSelectionBegin = sanitizePosition (position);
  if (mEdit.mCursor.mSelectionBegin > mEdit.mCursor.mSelectionEnd)
    swap (mEdit.mCursor.mSelectionBegin, mEdit.mCursor.mSelectionEnd);
  }
//}}}
//{{{
void cTextEdit::setSelectionEnd (sPosition position) {

  mEdit.mCursor.mSelectionEnd = sanitizePosition (position);
  if (mEdit.mCursor.mSelectionBegin > mEdit.mCursor.mSelectionEnd)
    swap (mEdit.mCursor.mSelectionBegin, mEdit.mCursor.mSelectionEnd);
  }
//}}}

//{{{
void cTextEdit::setSelection (sPosition beginPosition, sPosition endPosition, eSelection mode) {

  mEdit.mCursor.mSelectionBegin = sanitizePosition (beginPosition);
  mEdit.mCursor.mSelectionEnd = sanitizePosition (endPosition);
  if (mEdit.mCursor.mSelectionBegin > mEdit.mCursor.mSelectionEnd)
    swap (mEdit.mCursor.mSelectionBegin, mEdit.mCursor.mSelectionEnd);

  switch (mode) {
    case cTextEdit::eSelection::eNormal:
      break;

    case cTextEdit::eSelection::eWord: {
      // set word columns
      mEdit.mCursor.mSelectionBegin = findWordBegin (mEdit.mCursor.mSelectionBegin);
      if (!isOnWordBoundary (mEdit.mCursor.mSelectionEnd))
        mEdit.mCursor.mSelectionEnd = findWordEnd (findWordBegin (mEdit.mCursor.mSelectionEnd));
      break;
      }

    case cTextEdit::eSelection::eLine: {
      // set whole line columns
      mEdit.mCursor.mSelectionBegin.mColumn = 0;
      mEdit.mCursor.mSelectionEnd.mColumn = getLineMaxColumn (mEdit.mCursor.mSelectionEnd.mLineNumber);
      break;
      }

    default:
      break;
    }
  }
//}}}
//}}}
//{{{  utils
//{{{
void cTextEdit::advance (sPosition& position) {

  if (position.mLineNumber < getNumLines()) {
    const auto& glyphs = getGlyphs (position.mLineNumber);
    uint32_t characterIndex = getCharacterIndex (position);
    if (characterIndex + 1 < glyphs.size()) {
      uint32_t delta = utf8CharLength (glyphs[characterIndex].mChar);
      characterIndex = min (characterIndex + delta, static_cast<uint32_t>(glyphs.size())-1);
      }
    else {
      ++position.mLineNumber;
      characterIndex = 0;
      }

    position.mColumn = getCharacterColumn (position.mLineNumber, characterIndex);
    }
  }
//}}}

//{{{
void cTextEdit::scrollCursorVisible() {

  ImGui::SetWindowFocus();
  sPosition position = getCursorPosition();

  // up,down scroll
  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
  uint32_t minIndex = min (getMaxLineIndex(),
                           static_cast<uint32_t>(floor ((ImGui::GetScrollY() + ImGui::GetWindowHeight()) / mContext.mLineHeight)));
  if (lineIndex >= minIndex - 3)
    ImGui::SetScrollY (max (0.f, (lineIndex + 3) * mContext.mLineHeight) - ImGui::GetWindowHeight());
  else {
    uint32_t maxIndex = static_cast<uint32_t>(floor (ImGui::GetScrollY() / mContext.mLineHeight));
    if (lineIndex < maxIndex + 2)
      ImGui::SetScrollY (max (0.f, (lineIndex - 2) * mContext.mLineHeight));
    }

  // left,right scroll
  float textWidth = mContext.mLineNumberWidth + getTextWidth (position);
  uint32_t minWidth = static_cast<uint32_t>(floor (ImGui::GetScrollX()));
  if (textWidth - (3 * mContext.mGlyphWidth) < minWidth)
    ImGui::SetScrollX (max (0.f, textWidth - (3 * mContext.mGlyphWidth)));
  else {
    uint32_t maxWidth = static_cast<uint32_t>(ceil (ImGui::GetScrollX() + ImGui::GetWindowWidth()));
    if (textWidth + (3 * mContext.mGlyphWidth) > maxWidth)
      ImGui::SetScrollX (max (0.f, textWidth + (3 * mContext.mGlyphWidth) - ImGui::GetWindowWidth()));
    }

  mCursorFlashTimePoint = system_clock::now();
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::sanitizePosition (sPosition position) {
// return cursor position within glyphs
// - else
//    end of non empty line
// - else
//    begin of empty line
// - else
//    or begin of empty lines

  if (position.mLineNumber < getNumLines())
    return sPosition (position.mLineNumber, min (position.mColumn, getLineMaxColumn (position.mLineNumber)));
  else
    return sPosition (getNumLines()-1, getLineMaxColumn (getNumLines()-1));
  }
//}}}

// find
//{{{
cTextEdit::sPosition cTextEdit::findWordBegin (sPosition fromPosition) {

  if (fromPosition.mLineNumber >= getNumLines())
    return fromPosition;

  const auto& glyphs = getGlyphs (fromPosition.mLineNumber);
  uint32_t characterIndex = getCharacterIndex (fromPosition);
  if (characterIndex >= glyphs.size())
    return fromPosition;

  while ((characterIndex > 0) && isspace (glyphs[characterIndex].mChar))
    --characterIndex;

  uint8_t color = glyphs[characterIndex].mColor;
  while (characterIndex > 0) {
    uint8_t ch = glyphs[characterIndex].mChar;
    if ((ch & 0xC0) != 0x80) { // not UTF code sequence 10xxxxxx
      if ((ch <= 32) && isspace(ch)) {
        characterIndex++;
        break;
        }
      if (color != glyphs[size_t(characterIndex - 1)].mColor)
        break;
      }
    --characterIndex;
    }

  return sPosition (fromPosition.mLineNumber, getCharacterColumn (fromPosition.mLineNumber, characterIndex));
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::findWordEnd (sPosition fromPosition) {

  if (fromPosition.mLineNumber >= getNumLines())
    return fromPosition;

  const auto& glyphs = getGlyphs (fromPosition.mLineNumber);
  uint32_t characterIndex = getCharacterIndex (fromPosition);
  if (characterIndex >= glyphs.size())
    return fromPosition;

  bool prevSpace = (bool)isspace (glyphs[characterIndex].mChar);
  uint8_t color = glyphs[characterIndex].mColor;
  while (characterIndex < glyphs.size()) {
    uint8_t ch = glyphs[characterIndex].mChar;
    int length = utf8CharLength (ch);
    if (color != glyphs[characterIndex].mColor)
      break;

    if (prevSpace != !!isspace (ch)) {
      if (isspace (ch))
        while ((characterIndex < glyphs.size()) && isspace (glyphs[characterIndex].mChar))
          ++characterIndex;
      break;
      }
    characterIndex += length;
    }

  return sPosition (fromPosition.mLineNumber, getCharacterColumn (fromPosition.mLineNumber, characterIndex));
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::findNextWord (sPosition fromPosition) {

  if (fromPosition.mLineNumber >= getNumLines())
    return fromPosition;

  // skip to the next non-word character
  bool skip = false;
  bool isWord = false;

  uint32_t characterIndex = getCharacterIndex (fromPosition);
  if (characterIndex < getNumGlyphs (fromPosition.mLineNumber)) {
    const auto& glyphs = getGlyphs (fromPosition.mLineNumber);
    isWord = isalnum (glyphs[characterIndex].mChar);
    skip = isWord;
    }

  while (!isWord || skip) {
    if (fromPosition.mLineNumber >= getNumLines()) {
      uint32_t lineNumber = max (0u, getNumLines()-1);
      return sPosition (lineNumber, getLineMaxColumn (lineNumber));
      }

    const auto& glyphs = getGlyphs (fromPosition.mLineNumber);
    if (characterIndex < glyphs.size()) {
      isWord = isalnum (glyphs[characterIndex].mChar);
      if (isWord && !skip)
        return sPosition (fromPosition.mLineNumber, getCharacterColumn (fromPosition.mLineNumber, characterIndex));
      if (!isWord)
        skip = false;
      characterIndex++;
      }

    else {
      characterIndex = 0;
      ++fromPosition.mLineNumber;
      skip = false;
      isWord = false;
      }
    }

  return fromPosition;
  }
//}}}

// move
//{{{
void cTextEdit::moveUp (uint32_t amount) {

  // lineNumber
  sPosition position = mEdit.mCursor.mPosition;
  int lineNumber = position.mLineNumber;
  if (lineNumber == 0)
    return;

  // lineIndex
  uint32_t lineIndex = getLineIndexFromNumber (lineNumber);
  lineIndex = (amount < lineIndex) ? lineIndex - amount : 0;

  // cursorPosition
  mEdit.mCursor.mPosition.mLineNumber = getLineNumberFromIndex (lineIndex);
  if (mEdit.mCursor.mPosition != position) {
    // cursor changed
    mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
    mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
    setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, eSelection::eNormal);

    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::moveDown (uint32_t amount) {

  // lineNumber
  sPosition position = mEdit.mCursor.mPosition;
  int lineNumber = position.mLineNumber;

  // lineIndex
  uint32_t lineIndex = getLineIndexFromNumber (lineNumber);
  lineIndex = min (getMaxLineIndex(), lineIndex + amount);

  // cursorPosition
  mEdit.mCursor.mPosition.mLineNumber = getLineNumberFromIndex (lineIndex);
  if (mEdit.mCursor.mPosition != position) {
    // cursor changed
    mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
    mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
    setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, eSelection::eNormal);

    scrollCursorVisible();
    }
  }
//}}}

// insert
//{{{
cTextEdit::cLine& cTextEdit::insertLine (uint32_t index) {
  return *mDoc.mLines.insert (mDoc.mLines.begin() + index, cLine::tGlyphs());
  }
//}}}
//{{{
uint32_t cTextEdit::insertTextAt (sPosition& position, const string& insertString) {

  uint32_t totalLines = 0;

  uint32_t characterIndex = getCharacterIndex (position);

  const char* text = insertString.c_str();
  while (*text != '\0') {
    if (*text == '\r') // skip
      ++text;
    else if (*text == '\n') {
      if (characterIndex < getNumGlyphs (position.mLineNumber)) {
        cLine& line = getLine (position.mLineNumber);
        cLine& newLine = insertLine (position.mLineNumber+1);

        newLine.mGlyphs.insert (newLine.mGlyphs.begin(), line.mGlyphs.begin() + characterIndex, line.mGlyphs.end());
        parseLine (newLine);

        line.mGlyphs.erase (line.mGlyphs.begin() + characterIndex, line.mGlyphs.end());
        parseLine (line);
        }
      else
        insertLine (position.mLineNumber+1);

      position.mLineNumber++;
      position.mColumn = 0;
      characterIndex = 0;
      totalLines++;
      text++;
      }

    else {
      cLine& line = getLine (position.mLineNumber);
      auto length = utf8CharLength (*text);
      while ((length > 0) && (*text != '\0')) {
        line.mGlyphs.insert (line.mGlyphs.begin() + characterIndex++, cGlyph (*text++, eText));
        length--;
        }
      position.mColumn++;

      parseLine (line);
      }

    mDoc.mEdited = true;
    }

  return totalLines;
  }
//}}}
//{{{
void cTextEdit::insertText (const string& insertString) {

  if (insertString.empty())
    return;

  sPosition position = getCursorPosition();
  sPosition beginPosition = min (position, mEdit.mCursor.mSelectionBegin);

  int totalLines = position.mLineNumber - beginPosition.mLineNumber;
  totalLines += insertTextAt (position, insertString);

  setSelection (position, position, eSelection::eNormal);
  setCursorPosition (position);
  }
//}}}

// delete
//{{{
void cTextEdit::removeLine (uint32_t beginPosition, uint32_t endPosition) {

  mDoc.mLines.erase (mDoc.mLines.begin() + beginPosition, mDoc.mLines.begin() + endPosition);
  mEdit.mCheckComments = true;
  mDoc.mEdited = true;
  }
//}}}
//{{{
void cTextEdit::removeLine (uint32_t index) {

  mDoc.mLines.erase (mDoc.mLines.begin() + index);
  mEdit.mCheckComments = true;
  mDoc.mEdited = true;
  }
//}}}
//{{{
void cTextEdit::deleteRange (sPosition beginPosition, sPosition endPosition) {

  if (endPosition == beginPosition)
    return;

  uint32_t beginCharacterIndex = getCharacterIndex (beginPosition);
  uint32_t endCharacterIndex = getCharacterIndex (endPosition);

  if (beginPosition.mLineNumber == endPosition.mLineNumber) {
    // delete in same line
    cLine& line = getLine (beginPosition.mLineNumber);
    uint32_t maxColumn = getLineMaxColumn (beginPosition.mLineNumber);
    if (endPosition.mColumn >= maxColumn)
      line.mGlyphs.erase (line.mGlyphs.begin() + beginCharacterIndex, line.mGlyphs.end());
    else
      line.mGlyphs.erase (line.mGlyphs.begin() + beginCharacterIndex, line.mGlyphs.begin() + endCharacterIndex);

    parseLine (line);
    }

  else {
    // delete over multiple lines
    // firstLine
    cLine& firstLine = getLine (beginPosition.mLineNumber);
    firstLine.mGlyphs.erase (firstLine.mGlyphs.begin() + beginCharacterIndex, firstLine.mGlyphs.end());

    // lastLine
    cLine& lastLine = getLine (endPosition.mLineNumber);
    lastLine.mGlyphs.erase (lastLine.mGlyphs.begin(), lastLine.mGlyphs.begin() + endCharacterIndex);

    // insert partial lastLine into end of firstLine
    if (beginPosition.mLineNumber < endPosition.mLineNumber)
      firstLine.mGlyphs.insert (firstLine.mGlyphs.end(), lastLine.mGlyphs.begin(), lastLine.mGlyphs.end());

    // delete totally unused lines
    if (beginPosition.mLineNumber < endPosition.mLineNumber)
      removeLine (beginPosition.mLineNumber + 1, endPosition.mLineNumber + 1);

    parseLine (firstLine);
    parseLine (lastLine);
    }

  mDoc.mEdited = true;
  }
//}}}

// undo
//{{{
void cTextEdit::addUndo (cUndo& undo) {

  mUndoList.mBuffer.resize (mUndoList.mIndex+1);
  mUndoList.mBuffer.back() = undo;
  ++mUndoList.mIndex;
  }
//}}}
//}}}

// parse
//{{{
void cTextEdit::parseTokens (cLine& line, const string& textString) {
// parse and color tokens, recognise and color keyWords and knownWords

  const char* strBegin = &textString.front();
  const char* strEnd = strBegin + textString.size();
  const char* strPtr = strBegin;
  while (strPtr < strEnd) {
    // faster tokenize search
    const char* tokenBegin = nullptr;
    const char* tokenEnd = nullptr;
    uint8_t tokenColor = eText;
    bool tokenFound = mOptions.mLanguage.mTokenSearch &&
                      mOptions.mLanguage.mTokenSearch (strPtr, strEnd, tokenBegin, tokenEnd, tokenColor);

    if (!tokenFound) {
      // slower regex search
      for (auto& p : mOptions.mLanguage.mRegexList) {
        cmatch results;
        if (regex_search (strPtr, strEnd, results, p.first, regex_constants::match_continuous)) {
          auto& v = *results.begin();
          tokenBegin = v.first;
          tokenEnd = v.second;
          tokenColor = p.second;
          tokenFound = true;
          break;
          }
        }
      }

    if (tokenFound) {
      // token to color
      if (tokenColor == eIdentifier) {
        // extra search for keyWords, knownWords
        string tokenString (tokenBegin, tokenEnd);
        if (mOptions.mLanguage.mKeyWords.count (tokenString) != 0)
          tokenColor = eKeyWord;
        else if (mOptions.mLanguage.mKnownWords.count (tokenString) != 0)
          tokenColor = eKnownWord;
        }

      // color token glyphs
      size_t glyphIndex = tokenBegin - strBegin;
      size_t glyphIndexEnd = tokenEnd - strBegin;
      while (glyphIndex < glyphIndexEnd)
        line.mGlyphs[glyphIndex++].mColor = tokenColor;

      strPtr = tokenEnd;
      }
    else
      strPtr++;
    }
  }
//}}}
//{{{
void cTextEdit::parseLine (cLine& line) {
// parse for fold stuff, tokenize and look for keyWords, Known

  // check whole text for comments later
  mEdit.mCheckComments = true;

  line.mCommentBegin = false;
  line.mCommentEnd = false;
  line.mCommentSingle = false;
  line.mFoldBegin = false;
  line.mFoldEnd = false;
  line.mFoldOpen = false;
  line.mFoldCommentLineNumber = 0;
  line.mIndent = 0;
  line.mFirstGlyph = 0;
  line.mFirstColumn = 0;

  if (line.mGlyphs.empty())
    return;

  // glyphs to string
  string glyphString;
  for (auto& glyph : line.mGlyphs) {
    glyph.mColor = eText;
    glyphString += glyph.mChar;
    }

  //{{{  find indent
  size_t indentPos = glyphString.find_first_not_of (' ');
  line.mIndent = (indentPos == string::npos) ? 0 : static_cast<uint8_t>(indentPos);
  //}}}
  //{{{  find commentSingle token
  size_t pos = indentPos;

  do {
    pos = glyphString.find (mOptions.mLanguage.mCommentSingle, pos);
    if (pos != string::npos) {
      line.mCommentSingle = true;
      for (size_t i = 0; i < mOptions.mLanguage.mCommentSingle.size(); i++)
        line.mGlyphs[pos++].mCommentSingle = true;
      }
    } while (pos != string::npos);
  //}}}
  //{{{  find commentBegin token
  pos = indentPos;

  do {
    pos = glyphString.find (mOptions.mLanguage.mCommentBegin, pos);
    if (pos != string::npos) {
      line.mCommentBegin = true;
      for (size_t i = 0; i < mOptions.mLanguage.mCommentBegin.size(); i++)
        line.mGlyphs[pos++].mCommentBegin = true;
      }
    } while (pos != string::npos);
  //}}}
  //{{{  find commentEnd token
  pos = indentPos;

  do {
    pos = glyphString.find (mOptions.mLanguage.mCommentEnd, pos);
    if (pos != string::npos) {
      line.mCommentEnd = true;
      for (size_t i = 0; i < mOptions.mLanguage.mCommentEnd.size(); i++)
        line.mGlyphs[pos++].mCommentEnd = true;
      }
    } while (pos != string::npos);
  //}}}
  //{{{  find foldBegin token
  size_t foldBeginPos = glyphString.find (mOptions.mLanguage.mFoldBeginMarker, indentPos);
  line.mFoldBegin = (foldBeginPos != string::npos);
  //}}}
  //{{{  find foldEnd token
  size_t foldEndPos = glyphString.find (mOptions.mLanguage.mFoldEndMarker, indentPos);
  line.mFoldEnd = (foldEndPos != string::npos);
  //}}}
  if (line.mFoldBegin) {
    // does foldBegin have a comment?
    line.mCommentFold = glyphString.size() != (foldBeginPos + mOptions.mLanguage.mFoldBeginMarker.size());
    mDoc.mHasFolds = true;
    }

  parseTokens (line, glyphString);
  }
//}}}
//{{{
void cTextEdit::parseComments() {
// simple parse all lines for comments

  if (mEdit.mCheckComments) {
    mEdit.mCheckComments = false;

    bool inString = false;
    bool inSingleComment = false;
    bool srcBegsrcEndComment = false;

    uint32_t glyphIndex = 0;
    uint32_t lineNumber = 0;
    while (lineNumber < getNumLines()) {
      auto& glyphs = getGlyphs (lineNumber);
      uint32_t numGlyphs = getNumGlyphs (lineNumber);
      if (numGlyphs > 0) {
        // parse ch
        uint8_t ch = glyphs[glyphIndex].mChar;
        if (ch == '\"') {
          //{{{  start of string
          inString = true;
          if (inSingleComment || srcBegsrcEndComment)
            glyphs[glyphIndex].mColor = eComment;
          }
          //}}}
        else if (inString) {
          //{{{  in string
          if (inSingleComment || srcBegsrcEndComment)
            glyphs[glyphIndex].mColor = eComment;

          if (ch == '\"') // end of string
            inString = false;

          else if (ch == '\\') {
            // \ escapeChar in " " quotes, skip nextChar if any
            if (glyphIndex+1 < numGlyphs) {
              glyphIndex++;
              if (inSingleComment || srcBegsrcEndComment)
                glyphs[glyphIndex].mColor = eComment;
              }
            }
          }
          //}}}
        else {
          // comment begin?
          if (glyphs[glyphIndex].mCommentSingle)
            inSingleComment = true;
          else if (glyphs[glyphIndex].mCommentBegin)
            srcBegsrcEndComment = true;

          // in comment
          if (inSingleComment || srcBegsrcEndComment)
            glyphs[glyphIndex].mColor = eComment;

          // comment end ?
          if (glyphs[glyphIndex].mCommentEnd)
            srcBegsrcEndComment = false;
          }
        glyphIndex += utf8CharLength (ch);
        }

      if (glyphIndex >= numGlyphs) {
        // end of line, check for trailing concatenate '\' char
        if ((numGlyphs == 0) || (glyphs[numGlyphs-1].mChar != '\\')) {
          // no trailing concatenate, reset line flags
          inString = false;
          inSingleComment = false;
          }

        // next line
        lineNumber++;
        glyphIndex = 0;
        }
      }
    }
  }
//}}}

// fold
//{{{
void cTextEdit::openFold (uint32_t lineNumber) {

  if (isFolded()) {
    // position cursor to lineNumber, first column
    mEdit.mCursor.mPosition = {lineNumber,0};
    mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
    mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;

    cLine& line = getLine (lineNumber);
    if (line.mFoldBegin)
      line.mFoldOpen = true;
    }
  }
//}}}
//{{{
void cTextEdit::openFoldOnly (uint32_t lineNumber) {

  if (isFolded()) {
    // position cursor to lineNumber, first column,
    mEdit.mCursor.mPosition = {lineNumber,0};
    mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
    mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;

    cLine& line = getLine (lineNumber);
    if (line.mFoldBegin) {
      line.mFoldOpen = true;
      mEdit.mFoldOnly = true;
      mEdit.mFoldOnlyBeginLineNumber = lineNumber;
      }
    }
  }
//}}}
//{{{
void cTextEdit::closeFold (uint32_t lineNumber) {

  if (isFolded()) {
    // close fold containing lineNumber
    cLog::log (LOGINFO, fmt::format ("closeFold line:{}", lineNumber));

    if (getLine (lineNumber).mFoldBegin) {
      // position cursor to lineNumber, first column
      mEdit.mCursor.mPosition = {lineNumber,0};
      mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
      mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
      getLine (lineNumber).mFoldOpen = false;
      }

    else {
      // search back for this fold's foldBegin and close it
      // - skip foldEnd foldBegin pairs
      int skipFoldPairs = 0;
      while (lineNumber > 0) {
        lineNumber--;
        cLine& line = getLine (lineNumber);
        if (line.mFoldEnd) {
          skipFoldPairs++;
          cLog::log (LOGINFO, fmt::format ("- skip foldEnd:{} {}", lineNumber,skipFoldPairs));
          }
        else if (line.mFoldBegin && skipFoldPairs) {
          skipFoldPairs--;
          cLog::log (LOGINFO, fmt::format (" - skip foldBegin:{} {}", lineNumber, skipFoldPairs));
          }
        else if (line.mFoldBegin && line.mFoldOpen) {
          // position cursor to lineNumber, first column
          mEdit.mCursor.mPosition = {lineNumber,0};
          mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
          mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
          line.mFoldOpen = false;

          // possible scroll???
          break;
          }
        }
      }

    // close down foldOnly
    mEdit.mFoldOnly = false;
    mEdit.mFoldOnlyBeginLineNumber = 0;
    }
  }
//}}}
//{{{
int cTextEdit::skipFoldLines (uint32_t lineNumber) {
// recursively skip fold lines until matching foldEnd

  while (lineNumber < getNumLines())
    if (getLine (lineNumber).mFoldBegin)
      lineNumber = skipFoldLines (lineNumber+1);
    else if (getLine (lineNumber).mFoldEnd)
      return lineNumber+1;
    else
      lineNumber++;

  // error if you run off end. begin/end mismatch
  cLog::log (LOGERROR, "skipToFoldEnd - unexpected end reached with mathching fold");
  return lineNumber;
  }
//}}}

// mouse
//{{{
void cTextEdit::selectText (uint32_t lineNumber, float posX, bool selectWord) {

  mEdit.mCursor.mPosition = getPositionFromPosX (lineNumber, posX);

  mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
  mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
  mEdit.mSelection = selectWord ? eSelection::eWord : eSelection::eNormal;

  setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, mEdit.mSelection);
  }
//}}}
//{{{
void cTextEdit::dragSelectText (uint32_t lineNumber, ImVec2 pos) {

  int numDragLines = static_cast<int>(pos.y / mContext.mLineHeight);
  if (isFolded()) {
    if (numDragLines) {
      // check multiline drag across folds
      cLog::log (LOGINFO, fmt::format ("dragText - multiLine check across folds"));
      }
    }
  else {
    // allow multiline drag
    lineNumber = max (0u, min (getNumLines()-1, lineNumber + numDragLines));
    }

  mEdit.mCursor.mPosition = getPositionFromPosX (lineNumber, pos.x);
  mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
  mEdit.mSelection = eSelection::eNormal;

  setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, mEdit.mSelection);
  scrollCursorVisible();
  }
//}}}
//{{{
void cTextEdit::selectLine (uint32_t lineNumber) {

  mEdit.mCursor.mPosition = {lineNumber, 0};
  mEdit.mInteractiveBegin = mEdit.mCursor.mPosition;
  mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
  mEdit.mSelection = eSelection::eLine;

  setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, mEdit.mSelection);
  }
//}}}
//{{{
void cTextEdit::dragSelectLine (uint32_t lineNumber, float posY) {
// if folded this will use previous displays mFoldLine vector
// - we haven't drawn subsequent lines yet
//   - works because dragging does not change vector
// - otherwise we would have to delay it to after the whole draw

  int numDragLines = static_cast<int>(posY / mContext.mLineHeight);

  if (isFolded()) {
    uint32_t lineIndex = getLineIndexFromNumber (lineNumber);
    lineIndex = max (0u, min (getNumFoldLines()-1, lineIndex + numDragLines));
    lineNumber = mDoc.mFoldLines[lineIndex];
    }
  else // simple add to lineNumber
    lineNumber = max (0u, min (getNumLines()-1, lineNumber + numDragLines));

  mEdit.mCursor.mPosition.mLineNumber = lineNumber;
  mEdit.mInteractiveEnd = mEdit.mCursor.mPosition;
  mEdit.mSelection = eSelection::eLine;

  setSelection (mEdit.mInteractiveBegin, mEdit.mInteractiveEnd, mEdit.mSelection);
  scrollCursorVisible();
  }
//}}}

// draw
//{{{
void cTextEdit::drawTop (cApp& app) {

  //{{{  lineNumber buttons
  if (toggleButton ("line", mOptions.mShowLineNumber))
    toggleShowLineNumber();
  mOptions.mHoverLineNumber = ImGui::IsItemHovered();

  if (mOptions.mShowLineNumber)
    // debug button
    if (mOptions.mShowLineNumber) {
      ImGui::SameLine();
      if (toggleButton ("debug", mOptions.mShowLineDebug))
        toggleShowLineDebug();
      }
  //}}}

  if (mDoc.mHasFolds) {
    //{{{  folded button
    ImGui::SameLine();
    if (toggleButton ("folded", isShowFolds()))
      toggleShowFolded();
    //mOptions.mHoverFolded = ImGui::IsItemHovered();
    }
    //}}}

  //{{{  monoSpaced buttom
  ImGui::SameLine();
  if (toggleButton ("mono", mOptions.mShowMonoSpaced))
    toggleShowMonoSpaced();
  // mOptions.mHoverMonoSpaced = ImGui::IsItemHovered();
  //}}}
  //{{{  whiteSpace button
  ImGui::SameLine();
  if (toggleButton ("space", mOptions.mShowWhiteSpace))
    toggleShowWhiteSpace();
  //mOptions.mHoverWhiteSpace = ImGui::IsItemHovered();
  //}}}

  if (hasClipboardText() && !isReadOnly()) {
    //{{{  paste button
    ImGui::SameLine();
    if (ImGui::Button ("paste"))
      paste();
    }
    //}}}
  if (hasSelect()) {
    //{{{  copy, cut, delete buttons
    ImGui::SameLine();
    if (ImGui::Button ("copy"))
      copy();
     if (!isReadOnly()) {
       ImGui::SameLine();
      if (ImGui::Button ("cut"))
        cut();
      ImGui::SameLine();
      if (ImGui::Button ("delete"))
        deleteIt();
      }
    }
    //}}}

  if (!isReadOnly() && hasUndo()) {
    //{{{  undo button
    ImGui::SameLine();
    if (ImGui::Button ("undo"))
      undo();
    }
    //}}}
  if (!isReadOnly() && hasRedo()) {
    //{{{  redo button
    ImGui::SameLine();
    if (ImGui::Button ("redo"))
      redo();
    }
    //}}}

  //{{{  readOnly button
  ImGui::SameLine();
  if (toggleButton ("readOnly", isReadOnly()))
    toggleReadOnly();
  //}}}
  //{{{  overwrite button
  ImGui::SameLine();
  if (toggleButton ("insert", !mOptions.mOverWrite))
    toggleOverWrite();
  //}}}

  //{{{  fontSize button, vert,triangle debug text
  ImGui::SameLine();
  ImGui::SetNextItemWidth (3 * ImGui::GetFontSize());
  ImGui::DragInt ("##fs", &mOptions.mFontSize, 0.2f, mOptions.mmDocntSize, mOptions.mMaxFontSize, "%d");

  if (ImGui::IsItemHovered()) {
    int fontSize = mOptions.mFontSize + static_cast<int>(ImGui::GetIO().MouseWheel);
    mOptions.mFontSize = max (mOptions.mmDocntSize, min (mOptions.mMaxFontSize, fontSize));
    }

  // debug text
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:vert:triangle",
               ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());
  //}}}
  //{{{  vsync button, fps text
  // vsync button
  ImGui::SameLine();
  if (toggleButton ("vSync", app.getPlatform().getVsync()))
    app.getPlatform().toggleVsync();

  // fps text
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
  //}}}

  //{{{  info text
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:{} {}", getCursorPosition().mColumn+1, getCursorPosition().mLineNumber+1,
                                           getNumLines(), getLanguage().mName).c_str());
  //}}}
  }
//}}}
//{{{
float cTextEdit::drawGlyphs (ImVec2 pos, const cLine::tGlyphs& glyphs, uint8_t firstGlyph, uint8_t forceColor) {

  if (glyphs.empty())
    return 0.f;

  // initial pos to measure textWidth on return
  float firstPosX = pos.x;

  array <char,256> str;
  size_t strIndex = 0;

  uint8_t prevColor = (forceColor == eUndefined) ? glyphs[firstGlyph].mColor : forceColor;

  for (size_t i = firstGlyph; i < glyphs.size(); i++) {
    uint8_t color = (forceColor == eUndefined) ? glyphs[i].mColor : forceColor;
    if ((strIndex > 0) && (strIndex < str.max_size()) &&
        ((color != prevColor) || (glyphs[i].mChar == '\t') || (glyphs[i].mChar == ' '))) {
      // draw colored glyphs, seperated by colorChange,tab,space
      pos.x += mContext.drawText (pos, prevColor, str.data(), str.data() + strIndex);
      strIndex = 0;
      }

    if (glyphs[i].mChar == '\t') {
      //{{{  tab
      ImVec2 arrowLeftPos = {pos.x + 1.f, pos.y + mContext.mFontSize/2.f};

      // apply tab
      pos.x = getTabEndPosX (pos.x);

      if (isDrawWhiteSpace()) {
        // draw tab arrow
        ImVec2 arrowRightPos = {pos.x - 1.f, arrowLeftPos.y};
        mContext.drawLine (arrowLeftPos, arrowRightPos, eTab);

        ImVec2 arrowTopPos = {arrowRightPos.x - (mContext.mFontSize/4.f) - 1.f,
                              arrowRightPos.y - (mContext.mFontSize/4.f)};
        mContext.drawLine (arrowRightPos, arrowTopPos, eTab);

        ImVec2 arrowBotPos = {arrowRightPos.x - (mContext.mFontSize/4.f) - 1.f,
                              arrowRightPos.y + (mContext.mFontSize/4.f)};
        mContext.drawLine (arrowRightPos, arrowBotPos, eTab);
        }
      }
      //}}}
    else if (glyphs[i].mChar == ' ') {
      //{{{  space
      if (isDrawWhiteSpace()) {
        // draw circle
        ImVec2 centre = {pos.x  + mContext.mGlyphWidth/2.f, pos.y + mContext.mFontSize/2.f};
        mContext.drawCircle (centre, 2.f, eWhiteSpace);
        }

      pos.x += mContext.mGlyphWidth;
      }
      //}}}
    else {
      int length = utf8CharLength (glyphs[i].mChar);
      while ((length-- > 0) && (strIndex < str.max_size()))
        str[strIndex++] = glyphs[i].mChar;
      }
    prevColor = color;
    }

  if (strIndex > 0) // draw any remaining glyphs
    pos.x += mContext.drawText (pos, prevColor, str.data(), str.data() + strIndex);

  return pos.x - firstPosX;
  }
//}}}
//{{{
void cTextEdit::drawLine (uint32_t lineNumber, uint32_t lineIndex) {
// draw Line and return incremented lineIndex

  if (isFolded()) {
    //{{{  update mFoldLines vector
    if (lineIndex >= getNumFoldLines())
      mDoc.mFoldLines.push_back (lineNumber);
    else
      mDoc.mFoldLines[lineIndex] = lineNumber;
    }
    //}}}

  ImVec2 leftPos = ImGui::GetCursorScreenPos();
  ImVec2 curPos = leftPos;

  float leftPadWidth = mContext.mLeftPad;
  cLine& line = getLine (lineNumber);
  if (isDrawLineNumber()) {
    //{{{  draw lineNumber
    curPos.x += leftPadWidth;

    if (mOptions.mShowLineDebug)
      mContext.mLineNumberWidth = mContext.drawText (curPos, eLineNumber,
        fmt::format ("{:4d}{}{}{}{}{}{}{}{}{:4d}{:2d}{:2d}{:2d} ",
          lineNumber,
          line.mCommentSingle? 'c' : ' ',
          line.mCommentBegin ? '{' : ' ',
          line.mCommentEnd   ? '}' : ' ',
          line.mCommentFold  ? 'C' : ' ',
          line.mFoldBegin    ? 'b':' ',
          line.mFoldEnd      ? 'e':' ',
          line.mFoldOpen     ? 'o' : ' ',
          line.mFoldPressed  ? 'p':' ',
          line.mFoldCommentLineNumber,
          line.mIndent,
          line.mFirstGlyph,
          line.mFirstColumn).c_str());
    else
      mContext.mLineNumberWidth = mContext.drawSmallText (curPos, eLineNumber,
                                                          fmt::format ("{:4d} ", lineNumber+1).c_str());

    // add invisibleButton, gobble up leftPad
    ImGui::InvisibleButton (fmt::format ("##l{}", lineNumber).c_str(),
                            {leftPadWidth + mContext.mLineNumberWidth, mContext.mLineHeight});
    if (ImGui::IsItemActive()) {
      if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
        dragSelectLine (lineNumber, ImGui::GetMousePos().y - curPos.y);
      else if (ImGui::IsMouseClicked (0))
        selectLine (lineNumber);
      }

    leftPadWidth = 0.f;
    curPos.x += mContext.mLineNumberWidth;
    ImGui::SameLine();
    }
    //}}}
  else
    mContext.mLineNumberWidth = 0.f;

  // draw text
  ImVec2 textPos = curPos;
  const auto& glyphs = line.mGlyphs;
  if (isFolded() && line.mFoldBegin) {
    if (line.mFoldOpen) {
      //{{{  draw foldBegin open
      // draw foldPrefix
      curPos.x += leftPadWidth;

      // add the indent
      float indentWidth = line.mIndent * mContext.mGlyphWidth;
      curPos.x += indentWidth;

      float prefixWidth = mContext.drawText (curPos, eFoldOpen, mOptions.mLanguage.mFoldBeginOpen.c_str());
      curPos.x += prefixWidth;

      // add foldPrefix invisibleButton, action on press
      ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                              {leftPadWidth + indentWidth + prefixWidth, mContext.mLineHeight});
      if (ImGui::IsItemActive() && !line.mFoldPressed) {
        // close fold
        line.mFoldPressed = true;
        closeFold (lineNumber);
        }
      if (ImGui::IsItemDeactivated())
        line.mFoldPressed = false;

      // draw glyphs
      textPos.x = curPos.x;
      float glyphsWidth = drawGlyphs (curPos, line.mGlyphs, line.mFirstGlyph, eUndefined);
      if (glyphsWidth < mContext.mGlyphWidth) // widen to scroll something to pick
        glyphsWidth = mContext.mGlyphWidth;

      // add invisibleButton
      ImGui::SameLine();
      ImGui::InvisibleButton (fmt::format("##t{}", lineNumber).c_str(), {glyphsWidth, mContext.mLineHeight});
      if (ImGui::IsItemActive())
        selectText (lineNumber, ImGui::GetMousePos().x - textPos.x, ImGui::IsMouseDoubleClicked(0));

      curPos.x += glyphsWidth;
      }
      //}}}
    else {
      //{{{  draw foldBegin closed
      // add indent
      curPos.x += leftPadWidth;

      // add the indent
      float indentWidth = line.mIndent * mContext.mGlyphWidth;
      curPos.x += indentWidth;

      // draw foldPrefix
      float prefixWidth = mContext.drawText (curPos, eFoldClosed, mOptions.mLanguage.mFoldBeginClosed.c_str());
      curPos.x += prefixWidth;

      // add invisibleButton, indent + prefix wide, action on press
      ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                              {leftPadWidth + indentWidth + prefixWidth, mContext.mLineHeight});
      if (ImGui::IsItemActive() && !line.mFoldPressed) {
        // open fold only
        line.mFoldPressed = true;
        openFoldOnly (lineNumber);
        }
      if (ImGui::IsItemDeactivated())
        line.mFoldPressed = false;

      // draw glyphs
      textPos.x = curPos.x;
      float glyphsWidth = drawGlyphs (curPos, getGlyphs (line.mFoldCommentLineNumber), line.mFirstGlyph, eFoldClosed);
      if (glyphsWidth < mContext.mGlyphWidth) // widen to scroll something to pick
        glyphsWidth = mContext.mGlyphWidth;

      // add invisible button
      ImGui::SameLine();
      ImGui::InvisibleButton (fmt::format ("##t{}", lineNumber).c_str(), {glyphsWidth, mContext.mLineHeight});
      if (ImGui::IsItemActive())
        selectText (lineNumber, ImGui::GetMousePos().x - textPos.x, ImGui::IsMouseDoubleClicked (0));

      curPos.x += glyphsWidth;
      }
      //}}}
    }
  else if (isFolded() && line.mFoldEnd) {
    //{{{  draw foldEnd
    curPos.x += leftPadWidth;

    // add the indent
    float indentWidth = line.mIndent * mContext.mGlyphWidth;
    curPos.x += indentWidth;

    float prefixWidth = mContext.drawText (curPos, eFoldOpen, mOptions.mLanguage.mFoldEnd.c_str());

    // add invisibleButton
    ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                            {leftPadWidth + indentWidth + prefixWidth, mContext.mLineHeight});
    if (ImGui::IsItemActive()) // closeFold at foldEnd, action on press, button is removed while still pressed
      closeFold (lineNumber);

    textPos.x = curPos.x;
    }
    //}}}
  else {
    //{{{  draw glyphs
    curPos.x += leftPadWidth;

    // drawGlyphs
    textPos.x = curPos.x;
    float glyphsWidth = drawGlyphs (curPos, line.mGlyphs, line.mFirstGlyph, eUndefined);
    if (glyphsWidth < mContext.mGlyphWidth) // widen to scroll something to pick
      glyphsWidth = mContext.mGlyphWidth;

    // add invisibleButton
    ImGui::InvisibleButton (fmt::format ("##t{}", lineNumber).c_str(), {leftPadWidth + glyphsWidth, mContext.mLineHeight});
    if (ImGui::IsItemActive()) {
      if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
        dragSelectText (lineNumber, {ImGui::GetMousePos().x - textPos.x, ImGui::GetMousePos().y - curPos.y});
      else if (ImGui::IsMouseClicked (0))
        selectText (lineNumber, ImGui::GetMousePos().x - textPos.x, ImGui::IsMouseDoubleClicked (0));
      }

    curPos.x += glyphsWidth;
    }
    //}}}

  //{{{  draw select highlight
  sPosition lineBeginPosition (lineNumber, 0);
  sPosition lineEndPosition (lineNumber, getLineMaxColumn (lineNumber));

  float xBegin = -1.f;
  if (mEdit.mCursor.mSelectionBegin <= lineEndPosition)
    xBegin = (mEdit.mCursor.mSelectionBegin > lineBeginPosition) ? getTextWidth (mEdit.mCursor.mSelectionBegin) : 0.f;

  float xEnd = -1.f;
  if (mEdit.mCursor.mSelectionEnd > lineBeginPosition)
    xEnd = getTextWidth ((mEdit.mCursor.mSelectionEnd < lineEndPosition) ? mEdit.mCursor.mSelectionEnd : lineEndPosition);

  if (mEdit.mCursor.mSelectionEnd.mLineNumber > lineNumber)
    xEnd += mContext.mGlyphWidth;

  if ((xBegin != -1) && (xEnd != -1) && (xBegin < xEnd))
    mContext.drawRect ({textPos.x + xBegin, textPos.y}, {textPos.x + xEnd, textPos.y + mContext.mLineHeight}, eSelect);
  //}}}

  if (lineNumber == mEdit.mCursor.mPosition.mLineNumber) {
    // line has cursor
    if (!hasSelect()) {
      //{{{  draw cursor highlight
      ImVec2 brPos = {curPos.x, curPos.y + mContext.mLineHeight};
      mContext.drawRect (leftPos, brPos, eCursorLineFill);
      mContext.drawRectLine (leftPos, brPos, eCursorLineEdge);
      }
      //}}}
    //{{{  draw flashing cursor
    auto now = system_clock::now();
    auto elapsed = now - mCursorFlashTimePoint;
    if (elapsed < 400ms) {
      float cursorPosX = getTextWidth (mEdit.mCursor.mPosition);
      uint32_t characterIndex = getCharacterIndex (mEdit.mCursor.mPosition);

      float cursorWidth;
      if (mOptions.mOverWrite && (characterIndex < glyphs.size())) {
        // overwrite
        if (glyphs[characterIndex].mChar == '\t') // widen overwrite tab cursor
          cursorWidth = getTabEndPosX (cursorPosX) - cursorPosX;
        else {
          // widen overwrite char cursor
          array <char,2> str;
          str[0] = glyphs[characterIndex].mChar;
          cursorWidth = mContext.measureText (str.data(), str.data()+1);
          }
        }
      else // insert cursor
        cursorWidth = 2.f;

      // draw cursor
      ImVec2 tlPos = {textPos.x + cursorPosX - 1.f, textPos.y};
      ImVec2 brPos = {tlPos.x + cursorWidth, curPos.y + mContext.mLineHeight};
      mContext.drawRect (tlPos, brPos, eCursor);
      }

    if (elapsed > 800ms)
      mCursorFlashTimePoint = now;
    //}}}
    }
  }
//}}}
//{{{
uint32_t cTextEdit::drawFolded() {

  uint32_t lineNumber = mEdit.mFoldOnly ? mEdit.mFoldOnlyBeginLineNumber : 0;

  uint32_t lineIndex = 0;
  while (lineNumber < getNumLines()) {
    cLine& line = getLine (lineNumber);
    if (line.mFoldBegin) {
      if (line.mFoldOpen) {
        // draw openFold line
        line.mFoldCommentLineNumber = lineNumber;
        line.mFirstGlyph = static_cast<uint8_t>(line.mIndent + mOptions.mLanguage.mFoldBeginMarker.size() + 2);
        line.mFirstColumn = static_cast<uint8_t>(line.mIndent + mOptions.mLanguage.mFoldBeginOpen.size());
        drawLine (lineNumber++, lineIndex++);
        }
      else if (line.mCommentFold) {
        // draw closedFold with comment line
        line.mFoldCommentLineNumber = lineNumber;
        line.mFirstGlyph = static_cast<uint8_t>(line.mIndent + mOptions.mLanguage.mFoldBeginMarker.size() + 2);
        line.mFirstColumn = static_cast<uint8_t>(line.mIndent + mOptions.mLanguage.mFoldBeginOpen.size());
        drawLine (lineNumber++, lineIndex++);

        // skip closed fold
        lineNumber = skipFoldLines (lineNumber);
        }
      else {
        // draw closedFold without comment line
        // - set mFoldCommentLineNumber to first non comment line, !!!! just use +1 for now !!!!
        line.mFoldCommentLineNumber = lineNumber+1;
        line.mFirstGlyph = getLine (line.mFoldCommentLineNumber).mIndent;
        line.mFirstColumn = static_cast<uint8_t>(line.mIndent + mOptions.mLanguage.mFoldBeginClosed.size());
        drawLine (lineNumber++, lineIndex++);

        // skip closed fold
        lineNumber = skipFoldLines (lineNumber+1);
        }
      }
    else if (!line.mFoldEnd) {
      // draw fold middle line
      line.mFoldCommentLineNumber = 0;
      line.mFirstGlyph = 0;
      line.mFirstColumn = 0;
      drawLine (lineNumber++, lineIndex++);
      }
    else {
      // draw foldEnd line
      line.mFoldCommentLineNumber = 0;
      line.mFirstGlyph = 0;
      line.mFirstColumn = 0;
      drawLine (lineNumber++, lineIndex++);
      if (mEdit.mFoldOnly)
        return lineIndex;
      }
    }

  return lineIndex;
  }
//}}}
//{{{
void cTextEdit::drawUnfolded() {
//  draw unfolded with clipper

  // clipper begin
  ImGuiListClipper clipper;
  clipper.Begin (getNumLines(), mContext.mLineHeight);
  clipper.Step();

  // clipper iterate
  for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++) {
    // not folded, simple use of line glyphs
    getLine (lineNumber).mFoldCommentLineNumber = lineNumber;
    getLine (lineNumber).mFirstGlyph = 0;
    getLine (lineNumber).mFirstColumn = 0;
    drawLine (lineNumber, 0);
    }

  // clipper end
  clipper.Step();
  clipper.End();
  }
//}}}

//{{{
void cTextEdit::handleKeyboard() {
  //{{{  numpad codes
  // -------------------------------------------------------------------------------------
  // |    numlock       |        /           |        *             |        -            |
  // |GLFW_KEY_NUM_LOCK | GLFW_KEY_KP_DIVIDE | GLFW_KEY_KP_MULTIPLY | GLFW_KEY_KP_SUBTRACT|
  // |     0x11a        |      0x14b         |      0x14c           |      0x14d          |
  // |------------------------------------------------------------------------------------|
  // |        7         |        8           |        9             |         +           |
  // |  GLFW_KEY_KP_7   |   GLFW_KEY_KP_8    |   GLFW_KEY_KP_9      |  GLFW_KEY_KP_ADD;   |
  // |      0x147       |      0x148         |      0x149           |       0x14e         |
  // | -------------------------------------------------------------|                     |
  // |        4         |        5           |        6             |                     |
  // |  GLFW_KEY_KP_4   |   GLFW_KEY_KP_5    |   GLFW_KEY_KP_6      |                     |
  // |      0x144       |      0x145         |      0x146           |                     |
  // | -----------------------------------------------------------------------------------|
  // |        1         |        2           |        3             |       enter         |
  // |  GLFW_KEY_KP_1   |   GLFW_KEY_KP_2    |   GLFW_KEY_KP_3      |  GLFW_KEY_KP_ENTER  |
  // |      0x141       |      0x142         |      0x143           |       0x14f         |
  // | -------------------------------------------------------------|                     |
  // |        0                              |        .             |                     |
  // |  GLFW_KEY_KP_0                        | GLFW_KEY_KP_DECIMAL  |                     |
  // |      0x140                            |      0x14a           |                     |
  // --------------------------------------------------------------------------------------

  // glfw keycodes, they are platform specific
  // - ImGuiKeys small subset of normal keyboard keys
  // - have I misunderstood something here ?

  //constexpr int kNumpadNumlock = 0x11a;
  constexpr int kNumpad0 = 0x140;
  constexpr int kNumpad1 = 0x141;
  //constexpr int kNumpad2 = 0x142;
  constexpr int kNumpad3 = 0x143;
  //constexpr int kNumpad4 = 0x144;
  //constexpr int kNumpad5 = 0x145;
  //constexpr int kNumpad6 = 0x146;
  constexpr int kNumpad7 = 0x147;
  //constexpr int kNumpad8 = 0x148;
  constexpr int kNumpad9 = 0x149;
  //constexpr int kNumpadDecimal = 0x14a;
  //constexpr int kNumpadDivide = 0x14b;
  //constexpr int kNumpadMultiply = 0x14c;
  //constexpr int kNumpadSubtract = 0x14d;
  //constexpr int kNumpadAdd = 0x14e;
  //constexpr int kNumpadEnter = 0x14f;
  //}}}
  //{{{
  struct sActionKey {
    bool mAlt;
    bool mCtrl;
    bool mShift;
    int mGuiKey;
    bool mWritable;
    function <void()> mActionFunc;
    };
  //}}}
  const vector <sActionKey> kActionKeys = {
  //  alt    ctrl   shift  guiKey               writable      function
     {false, true,  false, ImGuiKey_X,          true,  [this]{cut();}},
     {false, true,  false, ImGuiKey_V,          true,  [this]{paste();}},
     {false, false, false, ImGuiKey_Delete,     true,  [this]{deleteIt();}},
     {false, false, false, ImGuiKey_Backspace,  true,  [this]{backspace();}},
     {false, true,  false, ImGuiKey_Z,          true,  [this]{undo();}},
     {false, true,  false, ImGuiKey_Y,          true,  [this]{redo();}},
     // edit without change
     {false, true,  false, ImGuiKey_C,          false, [this]{copy();}},
     {false, true,  false, ImGuiKey_A,          false, [this]{selectAll();}},
     // move
     {false, false, false, ImGuiKey_LeftArrow,  false, [this]{moveLeft();}},
     {false, false, false, ImGuiKey_RightArrow, false, [this]{moveRight();}},
     {false, false, false, ImGuiKey_UpArrow,    false, [this]{moveLineUp();}},
     {false, false, false, ImGuiKey_DownArrow,  false, [this]{moveLineDown();}},
     {false, false, false, ImGuiKey_PageUp,     false, [this]{movePageUp();}},
     {false, false, false, ImGuiKey_PageDown,   false, [this]{movePageDown();}},
     {false, false, false, ImGuiKey_Home,       false, [this]{moveHome();}},
     {false, false, false, ImGuiKey_End,        false, [this]{moveEnd();}},
     // toggle mode
     {false, false, false, ImGuiKey_Insert,     false, [this]{toggleOverWrite();}},
     {false, true,  false, ImGuiKey_Space,      false, [this]{toggleShowFolded();}},
     // numpad
     {false, false, false, kNumpad1,            false, [this]{openFold();}},
     {false, false, false, kNumpad3,            false, [this]{closeFold();}},
     {false, false, false, kNumpad7,            false, [this]{openFoldOnly();}},
     {false, false, false, kNumpad9,            false, [this]{closeFold();}},
     {false, false, false, kNumpad0,            true,  [this]{createFold();}},
  // {false, false, false, kNumpad4,            false, [this]{prevFile();}},
  // {false, false, false, kNumpad6,            false, [this]{nextFile();}},
  // {true,  false, false, kNumpadMulitply,     false, [this]{findDialog();}},
  // {true,  false, false, kNumpadDivide,       false, [this]{replaceDialog();}},

  // {false, false, false, F4                   false, [this]{copy();}},
  // {false, true,  false, F                    false, [this]{findDialog();}},
  // {false, true,  false, S                    false, [this]{saveFile();}},
  // {false, true,  false, A                    false, [this]{saveAllFiles();}},
  // {false, true,  false, Tab                  false, [this]{removeTabs();}},
  // {true,  false, false, N                    false, [this]{gotoDialog();}},
     };

  ImGuiIO& io = ImGui::GetIO();
  bool shift = io.KeyShift;
  bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;
  io.WantTextInput = true;
  io.WantCaptureKeyboard = false;

  for (auto& actionKey : kActionKeys)
    //{{{  dispatch any actionKey
    if ((((actionKey.mGuiKey < 0x100) && ImGui::IsKeyPressed (ImGui::GetKeyIndex (actionKey.mGuiKey))) ||
         ((actionKey.mGuiKey >= 0x100) && ImGui::IsKeyPressed (actionKey.mGuiKey))) &&
        (!actionKey.mWritable || (actionKey.mWritable && !isReadOnly())) &&
        (actionKey.mCtrl == ctrl) &&
        (actionKey.mShift == shift) &&
        (actionKey.mAlt == alt)) {

      actionKey.mActionFunc();
      break;
      }
    //}}}

  if (!isReadOnly()) {
    // handle character keys
    if (!ctrl && !shift && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Enter)))
     enterCharacter ('\n', false);
    else if (!ctrl && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Tab)))
      enterCharacter ('\t', shift);
    if (!io.InputQueueCharacters.empty()) {
      for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
        auto ch = io.InputQueueCharacters[i];
        if (ch != 0 && (ch == '\n' || ch >= 32))
          enterCharacter (ch, shift);
        }
      io.InputQueueCharacters.resize (0);
     }
   }
  }
//}}}

//{{{  cTextEdit::cContext
//{{{
void cTextEdit::cContext::update (const cOptions& options) {
// update draw context

  mDrawList = ImGui::GetWindowDrawList();
  mFont = ImGui::GetFont();
  mFontAtlasSize = ImGui::GetFontSize();
  mFontSize = static_cast<float>(options.mFontSize);
  mFontSmallSize = mFontSize > ((mFontAtlasSize * 2.f) / 3.f) ? ((mFontAtlasSize * 2.f) / 3.f) : mFontSize;
  mFontSmallOffset = ((mFontSize - mFontSmallSize) * 2.f) / 3.f;

  float scale = mFontSize / mFontAtlasSize;
  mGlyphWidth = measureText (" ", nullptr) * scale;
  mLineHeight = ImGui::GetTextLineHeight() * scale;

  mLeftPad = mGlyphWidth/2.f;
  mLineNumberWidth = 0.f;
  }
//}}}

//{{{
float cTextEdit::cContext::measureText (const char* str, const char* strEnd) const {
// return width of text

  return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
  }
//}}}

//{{{
float cTextEdit::cContext::drawText (ImVec2 pos, uint8_t color, const char* str, const char* strEnd) {
 // draw and return width of text

  mDrawList->AddText (mFont, mFontSize, pos, kPalette[color], str, strEnd);
  return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
  }
//}}}
//{{{
float cTextEdit::cContext::drawSmallText (ImVec2 pos, uint8_t color, const char* str, const char* strEnd) {
 // draw and return width of small text

  pos.y += mFontSmallOffset;
  mDrawList->AddText (mFont, mFontSmallSize, pos, kPalette[color], str, strEnd);
  return mFont->CalcTextSizeA (mFontSmallSize, FLT_MAX, -1.f, str, strEnd).x;
  }
//}}}

//{{{
void cTextEdit::cContext::drawLine (ImVec2 pos1, ImVec2 pos2, uint8_t color) {
  mDrawList->AddLine (pos1, pos2, kPalette[color]);
  }
//}}}
//{{{
void cTextEdit::cContext::drawCircle (ImVec2 centre, float radius, uint8_t color) {
  mDrawList->AddCircleFilled (centre, radius, kPalette[color], 4);
  }
//}}}
//{{{
void cTextEdit::cContext::drawRect (ImVec2 pos1, ImVec2 pos2, uint8_t color) {
  mDrawList->AddRectFilled (pos1, pos2, kPalette[color]);
  }
//}}}
//{{{
void cTextEdit::cContext::drawRectLine (ImVec2 pos1, ImVec2 pos2, uint8_t color) {
  mDrawList->AddRect (pos1, pos2, kPalette[color], 1.f);
  }
//}}}
//}}}
//{{{  cTextEdit::cUndo
//{{{
void cTextEdit::cUndo::undo (cTextEdit* textEdit) {

  if (!mAdded.empty())
    textEdit->deleteRange (mAddedBegin, mAddedEnd);

  if (!mRemoved.empty()) {
    sPosition begin = mRemovedBegin;
    textEdit->insertTextAt (begin, mRemoved);
    }

  textEdit->mEdit.mCursor = mBefore;
  textEdit->scrollCursorVisible();
  }
//}}}
//{{{
void cTextEdit::cUndo::redo (cTextEdit* textEdit) {

  if (!mRemoved.empty())
    textEdit->deleteRange (mRemovedBegin, mRemovedEnd);

  if (!mAdded.empty()) {
    sPosition begin = mAddedBegin;
    textEdit->insertTextAt (begin, mAdded);
    }

  textEdit->mEdit.mCursor = mAfter;
  textEdit->scrollCursorVisible();
  }
//}}}
//}}}

// cTextEdit::cLanguage
//{{{
const cTextEdit::cLanguage& cTextEdit::cLanguage::c() {

  static cLanguage language;

  static bool inited = false;
  if (!inited) {
    inited = true;
    language.mName = "C++";

    language.mCommentBegin = "/*";
    language.mCommentEnd = "*/";
    language.mCommentSingle = "//";

    language.mFoldBeginMarker = "//{{{";
    language.mFoldEndMarker = "//}}}";

    language.mFoldBeginOpen = "{{{ ";
    language.mFoldBeginClosed = "... ";
    language.mFoldEnd = "}}}";

    language.mAutoIndentation = true;

    for (auto& keyWord : kKeyWords)
      language.mKeyWords.insert (keyWord);
    for (auto& knownWord : kKnownWords)
      language.mKnownWords.insert (knownWord);

    language.mTokenSearch = [](const char* srcBegin, const char* srcEnd,
                               const char*& tokenBegin, const char*& tokenEnd, uint8_t& color) -> bool {
      // tokenSearch  lambda
      color = eUndefined;
      while ((srcBegin < srcEnd) && isascii (*srcBegin) && isblank (*srcBegin))
        srcBegin++;
      if (srcBegin == srcEnd) {
        tokenBegin = srcEnd;
        tokenEnd = srcEnd;
        color = eText;
        }
      if (findIdentifier (srcBegin, srcEnd, tokenBegin, tokenEnd))
        color = eIdentifier;
      else if (findNumber (srcBegin, srcEnd, tokenBegin, tokenEnd))
        color = eNumber;
      else if (findPunctuation (srcBegin, tokenBegin, tokenEnd))
        color = ePunctuation;
      else if (findString (srcBegin, srcEnd, tokenBegin, tokenEnd))
        color = eString;
      else if (findLiteral (srcBegin, srcEnd, tokenBegin, tokenEnd))
        color = eLiteral;
      return (color != eUndefined);
      };

    language.mRegexList.push_back (
      make_pair (regex ("[ \\t]*#[ \\t]*[a-zA-Z_]+", regex_constants::optimize), (uint8_t)ePreProc));
    }

  return language;
  }
//}}}
//{{{
const cTextEdit::cLanguage& cTextEdit::cLanguage::hlsl() {

  static bool inited = false;
  static cLanguage language;
  if (!inited) {
    inited = true;
    language.mName = "HLSL";

    language.mCommentBegin = "/*";
    language.mCommentEnd = "*/";
    language.mCommentSingle = "//";

    language.mFoldBeginMarker = "#{{{";
    language.mFoldEndMarker = "#}}}";

    language.mFoldBeginOpen = "{{{ ";
    language.mFoldBeginClosed = "... ";
    language.mFoldEnd = "}}}";

    language.mAutoIndentation = true;

    for (auto& keyWord : kHlslKeyWords)
      language.mKeyWords.insert (keyWord);
    for (auto& knownWord : kHlslKnownWords)
      language.mKnownWords.insert (knownWord);

    language.mTokenSearch = nullptr;

    language.mRegexList.push_back (
      make_pair (regex ("[ \\t]*#[ \\t]*[a-zA-Z_]+", regex_constants::optimize), (uint8_t)ePreProc));
    language.mRegexList.push_back (
      make_pair (regex ("L?\\\"(\\\\.|[^\\\"])*\\\"", regex_constants::optimize), (uint8_t)eString));
    language.mRegexList.push_back (
      make_pair (regex ("\\'\\\\?[^\\']\\'", regex_constants::optimize), (uint8_t)eLiteral));
    language.mRegexList.push_back (
      make_pair (regex ("[a-zA-Z_][a-zA-Z0-9_]*", regex_constants::optimize), (uint8_t)eIdentifier));
    language.mRegexList.push_back (
      make_pair (regex ("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", regex_constants::optimize), (uint8_t)eNumber));
    language.mRegexList.push_back (
      make_pair (regex ("[+-]?[0-9]+[Uu]?[lL]?[lL]?", regex_constants::optimize), (uint8_t)eNumber));
    language.mRegexList.push_back (
      make_pair (regex ("0[0-7]+[Uu]?[lL]?[lL]?", regex_constants::optimize), (uint8_t)eNumber));
    language.mRegexList.push_back (
      make_pair (regex ("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", regex_constants::optimize), (uint8_t)eNumber));
    language.mRegexList.push_back (
      make_pair (regex ("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", regex_constants::optimize), (uint8_t)ePunctuation));
    }

  return language;
  }
//}}}
//{{{
const cTextEdit::cLanguage& cTextEdit::cLanguage::glsl() {

  static cLanguage language;
  static bool inited = false;
  if (!inited) {
    inited = true;
    language.mName = "GLSL";

    language.mCommentBegin = "/*";
    language.mCommentEnd = "*/";
    language.mCommentSingle = "//";

    language.mFoldBeginMarker = "#{{{";
    language.mFoldEndMarker = "#}}}";

    language.mFoldBeginOpen = "{{{ ";
    language.mFoldBeginClosed = "... ";
    language.mFoldEnd = "}}}";

    language.mAutoIndentation = true;

    for (auto& keyWord : kGlslKeyWords)
      language.mKeyWords.insert (keyWord);
    for (auto& knownWord : kGlslKnownWords)
      language.mKnownWords.insert (knownWord);

    language.mTokenSearch = nullptr;

    language.mRegexList.push_back (
      make_pair (regex ("[ \\t]*#[ \\t]*[a-zA-Z_]+", regex_constants::optimize), (uint8_t)ePreProc));
    language.mRegexList.push_back (
      make_pair (regex ("L?\\\"(\\\\.|[^\\\"])*\\\"", regex_constants::optimize), (uint8_t)eString));
    language.mRegexList.push_back (
      make_pair (regex ("\\'\\\\?[^\\']\\'", regex_constants::optimize), (uint8_t)eLiteral));
    language.mRegexList.push_back (
      make_pair (regex ("[a-zA-Z_][a-zA-Z0-9_]*", regex_constants::optimize), (uint8_t)eIdentifier));
    language.mRegexList.push_back (
      make_pair (regex ("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", regex_constants::optimize), (uint8_t)eNumber));
    language.mRegexList.push_back (
      make_pair (regex ("[+-]?[0-9]+[Uu]?[lL]?[lL]?", regex_constants::optimize), (uint8_t)eNumber));
    language.mRegexList.push_back (
      make_pair (regex ("0[0-7]+[Uu]?[lL]?[lL]?", regex_constants::optimize), (uint8_t)eNumber));
    language.mRegexList.push_back (
      make_pair (regex ("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", regex_constants::optimize), (uint8_t)eNumber));
    language.mRegexList.push_back (
      make_pair (regex ("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", regex_constants::optimize), (uint8_t)ePunctuation));
    }

  return language;
  }
//}}}
