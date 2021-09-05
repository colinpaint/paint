// cTextEditor.cpp - nicked from https://github.com/BalazsJako/ImGuiColorTextEdit
// - remember this file is used to test itself
//{{{  includes
// dummy comment

/* and some old style c comments
   to test comment handling in this editor
*/

#include <cstdint>
#include <cmath>
#include <algorithm>

#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <chrono>
#include <regex>
#include <functional>

#include "cTextEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "../utils/cLog.h"

using namespace std;
//}}}
//{{{  template equals
template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals (InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, BinaryPredicate p) {
  for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
    if (!p (*first1, *first2))
      return false;
    }

  return first1 == last1 && first2 == last2;
  }
//}}}
constexpr int kLeftTextMargin = 10;

namespace {
  //{{{  const
  //{{{
  const array <ImU32, (size_t)cTextEditor::ePalette::Max> kDarkPalette = {
    0xff7f7f7f, // Default
    0xffd69c56, // Keyword
    0xff00ff00, // Number
    0xff7070e0, // String
    0xff70a0e0, // Char literal
    0xffffffff, // Punctuation
    0xff408080, // Preprocessor
    0xffaaaaaa, // Ident
    0xff9bc64d, // Known ident
    0xffc040a0, // Preproc ident
    0xff206020, // Comment (single Line
    0xff406020, // Comment (multi line)
    0xff101010, // Background
    0xffe0e0e0, // Cursor
    0x80a06020, // Selection
    0x800020ff, // Marker
    0xff707000, // Line number
    0x40000000, // Current line fill
    0x40808080, // Current line fill (inactive)
    0x40a0a0a0, // Current line edge
    0xff808080, // WhiteSpace
    0xff404040, // Tab
    0xffff0000, // FoldBeginClosed,
    0xff0000ff, // FoldBeginOpen,
    0xff00ff00, // FoldEnd,
    };
  //}}}
  //{{{
  const array <ImU32, (size_t)cTextEditor::ePalette::Max> kLightPalette = {
    0xff7f7f7f, // None
    0xffff0c06, // Keyword
    0xff008000, // Number
    0xff2020a0, // String
    0xff304070, // Char literal
    0xff000000, // Punctuation
    0xff406060, // Preprocessor
    0xff404040, // Ident
    0xff606010, // Known ident
    0xffc040a0, // Preproc ident
    0xff205020, // Comment (single line)
    0xff405020, // Comment (multi line)
    0xffffffff, // Background
    0xff000000, // Cursor
    0x80600000, // Selection
    0xa00010ff, // Marker
    0xff505000, // Line number
    0x40000000, // Current line fill
    0x40808080, // Current line fill (inactive)
    0x40000000, // Current line edge
    0xff808080, // WhiteSpace,
    0xff404040, // Tab
    0xffff0000, // FoldBeginClosed,
    0xff0000ff, // FoldBeginOpen,
    0xff00ff00, // FoldEnd,
    };
  //}}}

  //{{{
  const vector<string> kCppKeywords = {
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
  const vector<string> kCppIdents = {
    "abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol",
    "ceil", "clock", "cosh", "ctime",
    "div",
    "exit",
    "fabs", "floor", "fmod",
    "getchar", "getenv",
    "isalnum", "isalpha", "isdigit", "isgraph", "ispunct", "isspace", "isupper",
    "kbhit",
    "log10", "log2", "log",
    "map", "memcmp", "modf", "min", "max"
    "pow", "printf", "putchar", "putenv", "puts",
    "rand", "remove", "rename",
    "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "set", "std", "string", "sprintf", "snprintf",
    "time", "tolower", "toupper",
    "unordered_map", "unordered_set",
    "vector",
    };
  //}}}

  //{{{
  const vector<string> kCKeywords = {
    "auto",
    "break",
    "case", "char", "const", "continue",
    "default", "do", "double",
    "else", "enum", "extern",
    "float", "for",
    "goto",
    "if", "inline", "int",
    //"include",
    "long",
    "register", "restrict", "return",
    "short", "signed", "sizeof", "static", "struct", "switch",
    "typedef", "union", "unsigned",
    "void", "volatile",
    "while",
    "_Alignas", "_Alignof", "_Atomic",
    "_Bool", "_Complex", "_Generic", "_Imaginary",
    "_Noreturn", "_Static_assert", "_Thread_local"
  };
  //}}}
  //{{{
  const vector<string> kCIdents = {
    "abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
    "ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
  };
  //}}}

  //{{{
  const vector<string> kHlslKeywords = {
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
  const vector<string> kHlslIdents = {
    "abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asint", "asuint",
    "asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross", "D3DCOLORtoUBYTE4", "ddx",
    "ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
    "distance", "dot", "dst", "errorf", "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2",
    "f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount",
    "GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync", "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange",
    "InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan",
    "ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf",
    "Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
    "ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin",
    "radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
    "tan", "tanh", "tex1D", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2D", "tex2Dbias", "tex2Dgrad", "tex2Dlod", "tex2Dproj",
    "tex3D", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc"
  };
  //}}}

  //{{{
  const vector<string> kGlslKeywords = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
    "_Noreturn", "_Static_assert", "_Thread_local"
  };
  //}}}
  //{{{
  const vector<string> kGlslIdents = {
    "abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
    "ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
  };
  //}}}
  //}}}
  //{{{
  bool isUtfSequence (char ch) {
    return (ch & 0xC0) == 0x80;
    }
  //}}}
  //{{{
  int utf8CharLength (uint8_t ch) {
  // https://en.wikipedia.org/wiki/UTF-8
  // We assume that the char is a standalone character (<128)
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
  int imTextCharToUtf8 (char* buf, int buf_size, uint32_t ch) {

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

  //{{{
  bool findString (const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd) {

    const char* p = inBegin;

    if (*p == '"') {
      p++;

      while (p < inEnd) {
        // handle end of string
        if (*p == '"') {
          outBegin = inBegin;
          outEnd = p + 1;
          return true;
         }

        // handle escape character for "
        if (*p == '\\' && p + 1 < inEnd && p[1] == '"')
          p++;

        p++;
        }
      }

    return false;
    }
  //}}}
  //{{{
  bool findCharacterLiteral (const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd) {

    const char * p = inBegin;

    if (*p == '\'') {
      p++;

      // handle escape characters
      if (p < inEnd && *p == '\\')
        p++;

      if (p < inEnd)
        p++;

      // handle end of character literal
      if (p < inEnd && *p == '\'') {
        outBegin = inBegin;
        outEnd = p + 1;
        return true;
        }
      }

    return false;
    }
  //}}}
  //{{{
  bool findIdent (const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd) {

    const char * p = inBegin;

    if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') {
      p++;

      while ((p < inEnd) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
             (*p >= '0' && *p <= '9') || *p == '_'))
        p++;

      outBegin = inBegin;
      outEnd = p;
      return true;
      }

    return false;
    }
  //}}}
  //{{{
  bool findNumber (const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd) {

    const char * p = inBegin;
    const bool startsWithNumber = *p >= '0' && *p <= '9';
    if (*p != '+' && *p != '-' && !startsWithNumber)
      return false;

    p++;
    bool hasNumber = startsWithNumber;
    while (p < inEnd && (*p >= '0' && *p <= '9')) {
      hasNumber = true;
      p++;
      }
    if (hasNumber == false)
      return false;

    bool isHex = false;
    bool isFloat = false;
    bool isBinary = false;
    if (p < inEnd) {
      if (*p == '.') {
        isFloat = true;
        p++;
        while (p < inEnd && (*p >= '0' && *p <= '9'))
          p++;
        }
      else if (*p == 'x' || *p == 'X') { // hex formatted integer of the type 0xef80
        isHex = true;
        p++;
        while (p < inEnd && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')))
          p++;
        }
      else if (*p == 'b' || *p == 'B') { // binary formatted integer of the type 0b01011101
        isBinary = true;
        p++;
        while (p < inEnd && (*p >= '0' && *p <= '1'))
          p++;
        }
      }

    if (isHex == false && isBinary == false) { // floating point exponent
      if (p < inEnd && (*p == 'e' || *p == 'E')) {
        isFloat = true;
        p++;
        if (p < inEnd && (*p == '+' || *p == '-'))
          p++;
        bool hasDigits = false;
        while (p < inEnd && (*p >= '0' && *p <= '9')) {
          hasDigits = true;
          p++;
          }
        if (hasDigits == false)
          return false;
        }

      // single precision floating point type
      if (p < inEnd && *p == 'f')
        p++;
      }

    if (isFloat == false) // integer size type
      while (p < inEnd && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L'))
        p++;

    outBegin = inBegin;
    outEnd = p;
    return true;
    }
  //}}}
  //{{{
  bool findPunctuation (const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd) {

    (void)inEnd;

    switch (*inBegin) {
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
        outBegin = inBegin;
        outEnd = inBegin + 1;
        return true;
      }

    return false;
    }
  //}}}
  }

// cTextEditor
//{{{
cTextEditor::cTextEditor()
  : mTabSize(4),
    mTextChanged(false), mCursorPositionChanged(false),
    mOverwrite(false) , mReadOnly(false), mIgnoreImGuiChild(false), mCheckComments(true),
    mShowFolded(true), mShowLineNumbers(false), mShowLineDebug(false), mShowWhiteSpace(true),

    mColorRangeMin(0), mColorRangeMax(0), mSelection(eSelection::Normal),
    mUndoIndex(0),

    mHandleKeyboardInputs(true), mHandleMouseInputs(true),
    mLineSpacing(1.0f), mWithinRender(false), mScrollToTop(false), mScrollToCursor(false),

    mStartTime(chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()),
    mLastClick(-1.0f) {

  mPaletteBase = kLightPalette;
  setLanguage (sLanguage::cPlus());

  mLines.push_back (vector<sGlyph>());
  }
//}}}
//{{{  gets
//{{{
string cTextEditor::getTextString() const {
// get text as single string

  return getText (sPosition(), sPosition((int)mLines.size(), 0));
  }
//}}}
//{{{
vector<string> cTextEditor::getTextStrings() const {
// get text as vector of string

  vector<string> result;
  result.reserve (mLines.size());

  for (auto& line : mLines) {
    string text;
    text.resize (line.mGlyphs.size());
    for (size_t i = 0; i < line.mGlyphs.size(); ++i)
      text[i] = line.mGlyphs[i].mChar;

    result.emplace_back (move (text));
    }

  return result;
  }
//}}}
//}}}
//{{{  sets
//{{{
void cTextEditor::setTextString (const string& text) {
// break test into lines, store in internal mLines structure

  mLines.clear();
  mLines.emplace_back (vector<sGlyph>());

  for (auto ch : text) {
    if (ch == '\r') {
      // ignore the carriage return character
      }
    else if (ch == '\n')
      mLines.emplace_back (vector<sGlyph>());
    else
      mLines.back().mGlyphs.emplace_back (sGlyph (ch, ePalette::Default));
    }

  mTextChanged = true;
  mScrollToTop = true;

  mUndoBuffer.clear();
  mUndoIndex = 0;

  parseFolds();
  colorize();
  }
//}}}
//{{{
void cTextEditor::setTextStrings (const vector<string>& lines) {
// store vector of lines in internal mLines structure

  mLines.clear();

  if (lines.empty())
    mLines.emplace_back (vector<sGlyph>());

  else {
    mLines.resize (lines.size());
    for (size_t i = 0; i < lines.size(); ++i) {
      const string& line = lines[i];
      mLines[i].mGlyphs.reserve (line.size());
      for (size_t j = 0; j < line.size(); ++j)
        mLines[i].mGlyphs.emplace_back (sGlyph (line[j], ePalette::Default));
      }
    }

  mTextChanged = true;
  mScrollToTop = true;

  mUndoBuffer.clear();
  mUndoIndex = 0;

  parseFolds();
  colorize();
  }
//}}}

//{{{
void cTextEditor::setPalette (bool lightPalette) {
  mPaletteBase = (lightPalette ? kLightPalette : kDarkPalette);
  }
//}}}
//{{{
void cTextEditor::setLanguage (const sLanguage& language) {

  mLanguage = language;

  mRegexList.clear();
  for (auto& r : mLanguage.mTokenRegexStrings)
    mRegexList.push_back (make_pair (regex (r.first, regex_constants::optimize), r.second));

  colorize();
  }
//}}}

//{{{
void cTextEditor::setCursorPosition (const sPosition& position) {

  if (mState.mCursorPosition != position) {
    mState.mCursorPosition = position;
    mCursorPositionChanged = true;
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEditor::setSelectionStart (const sPosition& position) {

  mState.mSelectionStart = sanitizePosition (position);

  if (mState.mSelectionStart > mState.mSelectionEnd)
    swap (mState.mSelectionStart, mState.mSelectionEnd);
  }
//}}}
//{{{
void cTextEditor::setSelectionEnd (const sPosition& position) {

  mState.mSelectionEnd = sanitizePosition (position);
  if (mState.mSelectionStart > mState.mSelectionEnd)
    swap (mState.mSelectionStart, mState.mSelectionEnd);
  }
//}}}
//{{{
void cTextEditor::setSelection (const sPosition& startPosition, const sPosition& endPosition, eSelection mode) {

  sPosition oldSelStart = mState.mSelectionStart;
  sPosition oldSelEnd = mState.mSelectionEnd;

  mState.mSelectionStart = sanitizePosition (startPosition);
  mState.mSelectionEnd = sanitizePosition (endPosition);
  if (mState.mSelectionStart > mState.mSelectionEnd)
    swap (mState.mSelectionStart, mState.mSelectionEnd);

  switch (mode) {
    case cTextEditor::eSelection::Normal:
      break;

    case cTextEditor::eSelection::Word: {
      mState.mSelectionStart = findWordStart (mState.mSelectionStart);
      if (!isOnWordBoundary (mState.mSelectionEnd))
        mState.mSelectionEnd = findWordEnd (findWordStart (mState.mSelectionEnd));
      break;
      }

    case cTextEditor::eSelection::Line: {
      const int lineNumber = mState.mSelectionEnd.mLineNumber;
      //const auto lineSize = (size_t)lineNumber < mLines.size() ? mLines[lineNumber].size() : 0;
      mState.mSelectionStart = sPosition (mState.mSelectionStart.mLineNumber, 0);
      mState.mSelectionEnd = sPosition (lineNumber, getLineMaxColumn (lineNumber));
      break;
      }

    default:
      break;
    }

  if (mState.mSelectionStart != oldSelStart || mState.mSelectionEnd != oldSelEnd)
    mCursorPositionChanged = true;
  }
//}}}
//}}}
//{{{  actions
// move
//{{{
void cTextEditor::moveTop() {

  sPosition oldPosition = mState.mCursorPosition;
  setCursorPosition (sPosition(0,0));

  if (mState.mCursorPosition != oldPosition) {
    mInteractiveStart = mState.mCursorPosition;
    mInteractiveEnd = mState.mCursorPosition;
    setSelection (mInteractiveStart, mInteractiveEnd);
    }
  }
//}}}
//{{{
void cTextEditor::moveTopSelect() {

  sPosition oldPosition = mState.mCursorPosition;
  setCursorPosition (sPosition(0,0));

  if (mState.mCursorPosition != oldPosition) {
    mInteractiveEnd = oldPosition;
    mInteractiveStart = mState.mCursorPosition;
    setSelection (mInteractiveStart, mInteractiveEnd);
    }
  }
//}}}
//{{{
void cTextEditor::moveBottom() {

  sPosition newPosition = sPosition ((int)mLines.size() - 1, 0);
  setCursorPosition (newPosition);
  mInteractiveStart = mInteractiveEnd = newPosition;
  setSelection (mInteractiveStart, mInteractiveEnd);
  }
//}}}
//{{{
void cTextEditor::moveBottomSelect() {

  sPosition oldPosition = getCursorPosition();
  sPosition newPosition = sPosition ((int)mLines.size() - 1, 0);

  setCursorPosition (newPosition);

  mInteractiveStart = oldPosition;
  mInteractiveEnd = newPosition;
  setSelection (mInteractiveStart, mInteractiveEnd);
  }
//}}}

//{{{
void cTextEditor::moveHome() {

  sPosition oldPosition = mState.mCursorPosition;
  setCursorPosition (sPosition(mState.mCursorPosition.mLineNumber, 0));

  if (mState.mCursorPosition != oldPosition) {
    mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
    setSelection (mInteractiveStart, mInteractiveEnd);
    }
  }
//}}}
//{{{
void cTextEditor::moveHomeSelect() {

  sPosition oldPosition = mState.mCursorPosition;
  setCursorPosition (sPosition(mState.mCursorPosition.mLineNumber, 0));

  if (mState.mCursorPosition != oldPosition) {
    if (oldPosition == mInteractiveStart)
      mInteractiveStart = mState.mCursorPosition;
    else if (oldPosition == mInteractiveEnd)
      mInteractiveEnd = mState.mCursorPosition;
    else {
      mInteractiveStart = mState.mCursorPosition;
      mInteractiveEnd = oldPosition;
      }

    setSelection (mInteractiveStart, mInteractiveEnd);
    }
  }
//}}}
//{{{
void cTextEditor::moveEnd() {

  sPosition oldPosition = mState.mCursorPosition;
  setCursorPosition (sPosition (mState.mCursorPosition.mLineNumber, getLineMaxColumn (oldPosition.mLineNumber)));

  if (mState.mCursorPosition != oldPosition) {
    mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
    setSelection (mInteractiveStart, mInteractiveEnd);
    }
  }
//}}}
//{{{
void cTextEditor::moveEndSelect() {

  sPosition oldPosition = mState.mCursorPosition;
  setCursorPosition (sPosition (mState.mCursorPosition.mLineNumber, getLineMaxColumn (oldPosition.mLineNumber)));

  if (mState.mCursorPosition != oldPosition) {
    if (oldPosition == mInteractiveEnd)
      mInteractiveEnd = mState.mCursorPosition;
    else if (oldPosition == mInteractiveStart)
      mInteractiveStart = mState.mCursorPosition;
    else {
      mInteractiveStart = oldPosition;
      mInteractiveEnd = mState.mCursorPosition;
      }
    setSelection (mInteractiveStart, mInteractiveEnd);
    }
  }
//}}}

// select
//{{{
void cTextEditor::selectAll() {
  setSelection (sPosition (0,0), sPosition ((int)mLines.size(), 0));
  }
//}}}
//{{{
void cTextEditor::selectWordUnderCursor() {

  sPosition cursorPosition = getCursorPosition();
  setSelection (findWordStart (cursorPosition), findWordEnd (cursorPosition));
  }
//}}}

// cut and paste
//{{{
void cTextEditor::copy() {

  if (hasSelect())
    ImGui::SetClipboardText (getSelectedText().c_str());

  else if (!mLines.empty()) {
    sLine& line = mLines[getCursorPosition().mLineNumber];

    string str;
    for (auto& glyph : line.mGlyphs)
      str.push_back (glyph.mChar);

    ImGui::SetClipboardText (str.c_str());
    }
  }
//}}}
//{{{
void cTextEditor::cut() {

  if (isReadOnly())
    copy();

  else if (hasSelect()) {
    sUndoRecord u;
    u.mBefore = mState;
    u.mRemoved = getSelectedText();
    u.mRemovedStart = mState.mSelectionStart;
    u.mRemovedEnd = mState.mSelectionEnd;

    copy();
    deleteSelection();

    u.mAfter = mState;
    addUndo (u);
    }
  }
//}}}
//{{{
void cTextEditor::paste() {

  if (isReadOnly())
    return;

  const char* clipText = ImGui::GetClipboardText();
  if (clipText != nullptr && strlen (clipText) > 0) {
    sUndoRecord u;
    u.mBefore = mState;
    if (hasSelect()) {
      u.mRemoved = getSelectedText();
      u.mRemovedStart = mState.mSelectionStart;
      u.mRemovedEnd = mState.mSelectionEnd;
      deleteSelection();
      }

    u.mAdded = clipText;
    u.mAddedStart = getCursorPosition();

    insertText (clipText);

    u.mAddedEnd = getCursorPosition();
    u.mAfter = mState;
    addUndo (u);
    }
  }
//}}}

// undo
//{{{
void cTextEditor::undo (int steps) {

  while (hasUndo() && steps-- > 0)
    mUndoBuffer[--mUndoIndex].undo (this);
  }
//}}}
//{{{
void cTextEditor::redo (int steps) {

  while (hasRedo() && steps-- > 0)
    mUndoBuffer[mUndoIndex++].redo (this);
  }
//}}}

// delete
//{{{
void cTextEditor::deleteIt() {

  assert(!mReadOnly);

  if (mLines.empty())
    return;

  sUndoRecord u;
  u.mBefore = mState;

  if (hasSelect()) {
    u.mRemoved = getSelectedText();
    u.mRemovedStart = mState.mSelectionStart;
    u.mRemovedEnd = mState.mSelectionEnd;
    deleteSelection();
    }

  else {
    sPosition position = getCursorPosition();
    setCursorPosition (position);
    vector<sGlyph>& glyphs = mLines[position.mLineNumber].mGlyphs;

    if (position.mColumn == getLineMaxColumn (position.mLineNumber)) {
      if (position.mLineNumber == (int)mLines.size() - 1)
        return;

      u.mRemoved = '\n';
      u.mRemovedStart = u.mRemovedEnd = getCursorPosition();
      advance (u.mRemovedEnd);

      vector<sGlyph>& nextLineGlyphs = mLines[position.mLineNumber + 1].mGlyphs;
      glyphs.insert (glyphs.end(), nextLineGlyphs.begin(), nextLineGlyphs.end());
      removeLine (position.mLineNumber + 1);
      }
    else {
      int cindex = getCharacterIndex (position);
      u.mRemovedStart = u.mRemovedEnd = getCursorPosition();
      u.mRemovedEnd.mColumn++;
      u.mRemoved = getText (u.mRemovedStart, u.mRemovedEnd);

      int d = utf8CharLength (glyphs[cindex].mChar);
      while (d-- > 0 && cindex < (int)glyphs.size())
        glyphs.erase (glyphs.begin() + cindex);
      }

    mTextChanged = true;
    colorize (position.mLineNumber, 1);
    }

  u.mAfter = mState;
  addUndo (u);
  }
//}}}
//{{{
void cTextEditor::backspace() {

  assert(!mReadOnly);

  if (mLines.empty())
    return;

  sUndoRecord u;
  u.mBefore = mState;

  if (hasSelect()) {
    u.mRemoved = getSelectedText();
    u.mRemovedStart = mState.mSelectionStart;
    u.mRemovedEnd = mState.mSelectionEnd;
    deleteSelection();
    }

  else {
    sPosition position = getCursorPosition();
    setCursorPosition (position);

    if (mState.mCursorPosition.mColumn == 0) {
      if (mState.mCursorPosition.mLineNumber == 0)
        return;

      u.mRemoved = '\n';
      u.mRemovedStart = u.mRemovedEnd = sPosition (position.mLineNumber - 1, getLineMaxColumn (position.mLineNumber - 1));
      advance (u.mRemovedEnd);

      vector<sGlyph>& line = mLines[mState.mCursorPosition.mLineNumber].mGlyphs;
      vector<sGlyph>& prevLine = mLines[mState.mCursorPosition.mLineNumber - 1].mGlyphs;
      int prevSize = getLineMaxColumn (mState.mCursorPosition.mLineNumber - 1);
      prevLine.insert (prevLine.end(), line.begin(), line.end());

      map<int,string> etmp;
      for (auto& marker : mMarkers)
        etmp.insert (map<int,string>::value_type (
          marker.first - 1 == mState.mCursorPosition.mLineNumber ? marker.first - 1 : marker.first, marker.second));
      mMarkers = move (etmp);

      removeLine (mState.mCursorPosition.mLineNumber);
      --mState.mCursorPosition.mLineNumber;
      mState.mCursorPosition.mColumn = prevSize;
      }
    else {
      vector<sGlyph>& glyphs = mLines[mState.mCursorPosition.mLineNumber].mGlyphs;
      int cindex = getCharacterIndex (position) - 1;
      int cend = cindex + 1;
      while (cindex > 0 && isUtfSequence (glyphs[cindex].mChar))
        --cindex;

      //if (cindex > 0 && UTF8CharLength(line[cindex].mChar) > 1)
      //  --cindex;                       glyphs
      u.mRemovedStart = u.mRemovedEnd = getCursorPosition();
      --u.mRemovedStart.mColumn;
      --mState.mCursorPosition.mColumn;

      while (cindex < (int)glyphs.size() && cend-- > cindex) {
        u.mRemoved += glyphs[cindex].mChar;
        glyphs.erase (glyphs.begin() + cindex);
        }
      }

    mTextChanged = true;

    ensureCursorVisible();
    colorize (mState.mCursorPosition.mLineNumber, 1);
    }

  u.mAfter = mState;
  addUndo (u);
  }
//}}}
//{{{
void cTextEditor::deleteSelection() {

  assert(mState.mSelectionEnd >= mState.mSelectionStart);
  if (mState.mSelectionEnd == mState.mSelectionStart)
    return;

  deleteRange (mState.mSelectionStart, mState.mSelectionEnd);

  setSelection (mState.mSelectionStart, mState.mSelectionStart);
  setCursorPosition (mState.mSelectionStart);

  colorize (mState.mSelectionStart.mLineNumber, 1);
  }
//}}}

// insert
//{{{
void cTextEditor::enterCharacter (ImWchar ch, bool shift) {

  assert (!mReadOnly);

  sUndoRecord u;
  u.mBefore = mState;

  if (hasSelect()) {
    if ((ch == '\t') && (mState.mSelectionStart.mLineNumber != mState.mSelectionEnd.mLineNumber)) {
      //{{{  tab
      auto start = mState.mSelectionStart;
      auto end = mState.mSelectionEnd;
      auto originalEnd = end;

      if (start > end)
        swap (start, end);

      start.mColumn = 0;
      // end.mColumn = end.mGlyphs < mLines.size() ? mLines[end.mGlyphs].size() : 0;
      if (end.mColumn == 0 && end.mLineNumber > 0)
        --end.mLineNumber;
      if (end.mLineNumber >= (int)mLines.size())
        end.mLineNumber = mLines.empty() ? 0 : (int)mLines.size() - 1;
      end.mColumn = getLineMaxColumn (end.mLineNumber);

      //if (end.mColumn >= getLineMaxColumn(end.mGlyphs))
      //  end.mColumn = getLineMaxColumn(end.mGlyphs) - 1;
      u.mRemovedStart = start;
      u.mRemovedEnd = end;
      u.mRemoved = getText (start, end);

      bool modified = false;

      for (int i = start.mLineNumber; i <= end.mLineNumber; i++) {
        vector<sGlyph>& glyphs = mLines[i].mGlyphs;
        if (shift) {
          if (!glyphs.empty()) {
            if (glyphs.front().mChar == '\t') {
              glyphs.erase (glyphs.begin());
              modified = true;
            }
            else {
              for (int j = 0; j < mTabSize && !glyphs.empty() && glyphs.front().mChar == ' '; j++) {
                glyphs.erase (glyphs.begin());
                modified = true;
                }
              }
            }
          }
        else {
          glyphs.insert (glyphs.begin(), sGlyph ('\t', cTextEditor::ePalette::Background));
          modified = true;
          }
        }

      if (modified) {
        start = sPosition (start.mLineNumber, getCharacterColumn (start.mLineNumber, 0));
        sPosition rangeEnd;
        if (originalEnd.mColumn != 0) {
          end = sPosition (end.mLineNumber, getLineMaxColumn (end.mLineNumber));
          rangeEnd = end;
          u.mAdded = getText (start, end);
          }
        else {
          end = sPosition (originalEnd.mLineNumber, 0);
          rangeEnd = sPosition (end.mLineNumber - 1, getLineMaxColumn (end.mLineNumber - 1));
          u.mAdded = getText (start, rangeEnd);
          }

        u.mAddedStart = start;
        u.mAddedEnd = rangeEnd;
        u.mAfter = mState;

        mState.mSelectionStart = start;
        mState.mSelectionEnd = end;
        addUndo (u);
        mTextChanged = true;
        ensureCursorVisible();
        }

      return;
      } // c == '\t'
      //}}}
    else {
      u.mRemoved = getSelectedText();
      u.mRemovedStart = mState.mSelectionStart;
      u.mRemovedEnd = mState.mSelectionEnd;
      deleteSelection();
      }
    }

  auto position = getCursorPosition();
  u.mAddedStart = position;

  assert (!mLines.empty());
  if (ch == '\n') {
    //{{{  carraigeReturn
    insertLine (position.mLineNumber + 1);
    vector<sGlyph>& glyphs = mLines[position.mLineNumber].mGlyphs;
    vector<sGlyph>& newLine = mLines[position.mLineNumber + 1].mGlyphs;

    if (mLanguage.mAutoIndentation)
      for (size_t it = 0; (it < glyphs.size()) && isascii (glyphs[it].mChar) && isblank (glyphs[it].mChar); ++it)
        newLine.push_back (glyphs[it]);

    const size_t whiteSpaceSize = newLine.size();
    auto cindex = getCharacterIndex (position);
    newLine.insert (newLine.end(), glyphs.begin() + cindex, glyphs.end());
    glyphs.erase (glyphs.begin() + cindex, glyphs.begin() + glyphs.size());
    setCursorPosition (sPosition (position.mLineNumber + 1, getCharacterColumn (position.mLineNumber + 1, (int)whiteSpaceSize)));
    u.mAdded = (char)ch;
    }
    //}}}
  else {
    char buf[7];
    int e = imTextCharToUtf8 (buf, 7, ch);
    if (e > 0) {
      buf[e] = '\0';
      vector<sGlyph>& glyphs = mLines[position.mLineNumber].mGlyphs;
      int cindex = getCharacterIndex (position);
      if (mOverwrite && (cindex < (int)glyphs.size())) {
        auto d = utf8CharLength (glyphs[cindex].mChar);
        u.mRemovedStart = mState.mCursorPosition;
        u.mRemovedEnd = sPosition (position.mLineNumber, getCharacterColumn (position.mLineNumber, cindex + d));
        while ((d-- > 0) && (cindex < (int)glyphs.size())) {
          u.mRemoved += glyphs[cindex].mChar;
          glyphs.erase (glyphs.begin() + cindex);
          }
        }

      for (auto p = buf; *p != '\0'; p++, ++cindex)
        glyphs.insert (glyphs.begin() + cindex, sGlyph (*p, ePalette::Default));
      u.mAdded = buf;
      setCursorPosition (sPosition (position.mLineNumber, getCharacterColumn (position.mLineNumber, cindex)));
      }
    else
      return;
    }
  mTextChanged = true;

  u.mAddedEnd = getCursorPosition();
  u.mAfter = mState;
  addUndo (u);

  colorize (position.mLineNumber - 1, 3);

  ensureCursorVisible();
  }
//}}}
//}}}
//{{{
void cTextEditor::render (const string& title, const ImVec2& size, bool border) {

  mTextChanged = false;
  mWithinRender = true;
  mCursorPositionChanged = false;

  ImGui::PushStyleColor (ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4 (mPalette[(size_t)ePalette::Background]));
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

  if (!mIgnoreImGuiChild)
    ImGui::BeginChild (title.c_str(), size, border,
                       ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar |
                       ImGuiWindowFlags_NoMove);

  if (mHandleKeyboardInputs) {
    handleKeyboardInputs();
    ImGui::PushAllowKeyboardFocus (true);
    }

  if (mHandleMouseInputs)
    handleMouseInputs();

  colorizeInternal();

  preRender();

  if (mShowFolded) {
    // iterate lines
    while (mLineIndex < mMaxLineIndex) {
      uint32_t lineNumber = mVisibleLines[mLineIndex];
      renderLine (mLines[lineNumber].mFoldBegin ? mLines[lineNumber].mFoldTitleLineNumber : lineNumber, lineNumber);
      mLineIndex++;
      }
    }
  else {
    // iterate lines
    uint32_t lineNumber = mLineIndex;
    while (lineNumber < mMaxLineIndex) {
      renderLine (lineNumber, 0);
      lineNumber++;
      }
    }

  postRender();

  if (mHandleKeyboardInputs)
    ImGui::PopAllowKeyboardFocus();

  if (!mIgnoreImGuiChild)
    ImGui::EndChild();

  ImGui::PopStyleVar();
  ImGui::PopStyleColor();

  mWithinRender = false;
  }
//}}}

// private:
//{{{  gets
//{{{
bool cTextEditor::isOnWordBoundary (const sPosition& position) const {

  if (position.mLineNumber >= (int)mLines.size() || position.mColumn == 0)
    return true;

  const vector<sGlyph>& glyphs = mLines[position.mLineNumber].mGlyphs;
  int cindex = getCharacterIndex (position);
  if (cindex >= (int)glyphs.size())
    return true;

  return glyphs[cindex].mColorIndex != glyphs[size_t(cindex - 1)].mColorIndex;
  //return isspace (glyphs[cindex].mChar) != isspace (glyphs[cindex - 1].mChar);
  }
//}}}

//{{{
int cTextEditor::getCharacterIndex (const sPosition& position) const {

  if (position.mLineNumber >= (int)mLines.size()) {
    cLog::log (LOGERROR, "cTextEditor::getCharacterIndex");
    return -1;
    }

  const vector<sGlyph>& glyphs = mLines[position.mLineNumber].mGlyphs;

  int c = 0;
  int i = 0;
  for (; i < (int)glyphs.size() && (c < position.mColumn);) {
    if (glyphs[i].mChar == '\t')
      c = (c / mTabSize) * mTabSize + mTabSize;
    else
      ++c;
    i += utf8CharLength (glyphs[i].mChar);
    }

  return i;
  }
//}}}
//{{{
int cTextEditor::getCharacterColumn (int lineNumber, int index) const {
// handle tabs

  if (lineNumber >= (int)mLines.size())
    return 0;

  const vector<sGlyph>& glyphs = mLines[lineNumber].mGlyphs;

  int col = 0;
  int i = 0;
  while ((i < index) && i < ((int)glyphs.size())) {
    uint8_t c = glyphs[i].mChar;
    i += utf8CharLength (c);
    if (c == '\t')
      col = (col / mTabSize) * mTabSize + mTabSize;
    else
      col++;
    }

  return col;
  }
//}}}

//{{{
int cTextEditor::getLineMaxColumn (int row) const {

  if (row >= (int)mLines.size())
    return 0;

  const vector<sGlyph>& glyphs = mLines[row].mGlyphs;
  int col = 0;
  for (size_t i = 0; i < glyphs.size(); ) {
    uint8_t c = glyphs[i].mChar;
    if (c == '\t')
      col = (col / mTabSize) * mTabSize + mTabSize;
    else
      col++;
    i += utf8CharLength (c);
    }

  return col;
  }
//}}}
//{{{
int cTextEditor::getLineCharacterCount (int row) const {

  if (row >= (int)mLines.size())
    return 0;

  const vector<sGlyph>& glyphs = mLines[row].mGlyphs;
  int c = 0;
  for (size_t i = 0; i < glyphs.size(); c++)
    i += utf8CharLength (glyphs[i].mChar);

  return c;
  }
//}}}

//{{{
string cTextEditor::getText (const sPosition& startPosition, const sPosition& endPosition) const {
// get text as string with carraigeReturn line breaks

  string result;

  int lstart = startPosition.mLineNumber;
  int lend = endPosition.mLineNumber;

  int istart = getCharacterIndex (startPosition);
  int iend = getCharacterIndex (endPosition);

  size_t s = 0;
  for (size_t i = lstart; i < (size_t)lend; i++)
    s += mLines[i].mGlyphs.size();

  result.reserve (s + s / 8);

  while (istart < iend || lstart < lend) {
    if (lstart >= (int)mLines.size())
      break;

    if (istart < (int)mLines[lstart].mGlyphs.size()) {
      result += mLines[lstart].mGlyphs[istart].mChar;
      istart++;
      }
    else {
      istart = 0;
      ++lstart;
      result += '\n';
      }
    }

  return result;
  }
//}}}
//{{{
ImU32 cTextEditor::getGlyphColor (const sGlyph& glyph) const {

  if (glyph.mComment)
    return mPalette[(size_t)ePalette::Comment];

  if (glyph.mMultiLineComment)
    return mPalette[(size_t)ePalette::MultiLineComment];

  auto const color = mPalette[(size_t)glyph.mColorIndex];
  if (glyph.mPreProc) {
    const auto ppcolor = mPalette[(size_t)ePalette::Preprocessor];
    const int c0 = ((ppcolor & 0xff) + (color & 0xff)) / 2;
    const int c1 = (((ppcolor >> 8) & 0xff) + ((color >> 8) & 0xff)) / 2;
    const int c2 = (((ppcolor >> 16) & 0xff) + ((color >> 16) & 0xff)) / 2;
    const int c3 = (((ppcolor >> 24) & 0xff) + ((color >> 24) & 0xff)) / 2;
    return ImU32(c0 | (c1 << 8) | (c2 << 16) | (c3 << 24));
    }

  return color;
  }
//}}}

//{{{
string cTextEditor::getCurrentLineText() const {

  int lineLength = getLineMaxColumn (mState.mCursorPosition.mLineNumber);
  return getText (sPosition(mState.mCursorPosition.mLineNumber, 0), sPosition(mState.mCursorPosition.mLineNumber, lineLength));
  }
//}}}

//{{{
string cTextEditor::getWordAt (const sPosition& position) const {

  sPosition start = findWordStart (position);
  sPosition end = findWordEnd (position);

  int istart = getCharacterIndex (start);
  int iend = getCharacterIndex (end);

  string r;
  for (int it = istart; it < iend; ++it)
    r.push_back (mLines[position.mLineNumber].mGlyphs[it].mChar);

  return r;
  }
//}}}
//{{{
string cTextEditor::getWordUnderCursor() const {

  sPosition c = getCursorPosition();
  return getWordAt (c);
  }
//}}}

//{{{
float cTextEditor::getTextWidth (const sPosition& position) const {
// get width of text from start to position

  const vector<sGlyph>& glyphs = mLines[position.mLineNumber].mGlyphs;
  float spaceSize = ImGui::GetFont()->CalcTextSizeA (ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

  float distance = 0.0f;
  int colIndex = getCharacterIndex (position);
  for (size_t i = 0; (i < glyphs.size()) && ((int)i < colIndex);) {
    if (glyphs[i].mChar == '\t') {
      distance = (1.0f + floor((1.0f + distance) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
      ++i;
      }
    else {
      int d = utf8CharLength (glyphs[i].mChar);
      char tempCString[7];
      int j = 0;
      for (; j < 6 && d-- > 0 && i < glyphs.size(); j++, i++)
        tempCString[j] = glyphs[i].mChar;

      tempCString[j] = '\0';
      distance += ImGui::GetFont()->CalcTextSizeA (ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
      }
    }

  return distance;
  }
//}}}
//{{{
int cTextEditor::getPageNumLines() const {
  float height = ImGui::GetWindowHeight() - 20.f;
  return (int)floor (height / mCharSize.y);
  }
//}}}
//}}}
//{{{  utils
//{{{
void cTextEditor::ensureCursorVisible() {

  if (!mWithinRender) {
    mScrollToCursor = true;
    return;
    }

  float scrollX = ImGui::GetScrollX();
  float scrollY = ImGui::GetScrollY();

  auto height = ImGui::GetWindowHeight();
  auto width = ImGui::GetWindowWidth();

  int top = 1 + (int)ceil(scrollY / mCharSize.y);
  int bottom = (int)ceil((scrollY + height) / mCharSize.y);

  int left = (int)ceil(scrollX / mCharSize.x);
  int right = (int)ceil((scrollX + width) / mCharSize.x);

  sPosition position = getCursorPosition();
  float length = getTextWidth (position);

  if (position.mLineNumber < top)
    ImGui::SetScrollY (max (0.0f, (position.mLineNumber - 1) * mCharSize.y));
  if (position.mLineNumber > bottom - 4)
    ImGui::SetScrollY (max (0.0f, (position.mLineNumber + 4) * mCharSize.y - height));

  if (length + mGlyphsStart < left + 4)
    ImGui::SetScrollX (max(0.0f, length + mGlyphsStart - 4));
  if (length + mGlyphsStart > right - 4)
    ImGui::SetScrollX (max(0.0f, length + mGlyphsStart + 4 - width));
  }
//}}}

//{{{
void cTextEditor::advance (sPosition& position) const {

  if (position.mLineNumber < (int)mLines.size()) {
    const vector<sGlyph>& glyphs = mLines[position.mLineNumber].mGlyphs;
    int cindex = getCharacterIndex (position);
    if (cindex + 1 < (int)glyphs.size()) {
      auto delta = utf8CharLength (glyphs[cindex].mChar);
      cindex = min (cindex + delta, (int)glyphs.size() - 1);
      }
    else {
      ++position.mLineNumber;
      cindex = 0;
      }

    position.mColumn = getCharacterColumn (position.mLineNumber, cindex);
    }
  }
//}}}
//{{{
cTextEditor::sPosition cTextEditor::screenToPosition (const ImVec2& pos) const {

  ImVec2 origin = ImGui::GetCursorScreenPos();
  ImVec2 local = ImVec2 (pos.x - origin.x, pos.y - origin.y);

  int lineNumber = max (0, (int)floor (local.y / mCharSize.y));
  int columnCoord = 0;

  if ((lineNumber >= 0) && (lineNumber < (int)mLines.size())) {
    const vector<sGlyph>& glyphs = mLines[lineNumber].mGlyphs;

    int columnIndex = 0;
    float columnX = 0.0f;

    while (columnIndex < (int)glyphs.size()) {
      float columnWidth = 0.0f;

      if (glyphs[columnIndex].mChar == '\t') {
        float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
        float oldX = columnX;
        float newColumnX = (1.0f + floor ((1.0f + columnX) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
        columnWidth = newColumnX - oldX;
        if (mGlyphsStart + columnX + columnWidth * 0.5f > local.x)
          break;
        columnX = newColumnX;
        columnCoord = (columnCoord / mTabSize) * mTabSize + mTabSize;
        columnIndex++;
        }

      else {
        char buf[7];
        auto d = utf8CharLength (glyphs[columnIndex].mChar);
        int i = 0;
        while (i < 6 && d-- > 0)
          buf[i++] = glyphs[columnIndex++].mChar;
        buf[i] = '\0';

        columnWidth = ImGui::GetFont()->CalcTextSizeA (ImGui::GetFontSize(), FLT_MAX, -1.0f, buf).x;
        if (mGlyphsStart + columnX + columnWidth * 0.5f > local.x)
          break;

        columnX += columnWidth;
        columnCoord++;
        }
      }
    }

  return sanitizePosition (sPosition (lineNumber, columnCoord));
  }
//}}}
//{{{
cTextEditor::sPosition cTextEditor::sanitizePosition (const sPosition& position) const {

  int line = position.mLineNumber;
  int column = position.mColumn;

  if (line >= (int)mLines.size()) {
    if (mLines.empty()) {
      line = 0;
      column = 0;
      }
    else {
      line = (int)mLines.size() - 1;
      column = getLineMaxColumn (line);
      }

    return sPosition (line, column);
    }

  else {
    column = mLines.empty() ? 0 : min (column, getLineMaxColumn (line));
    return sPosition (line, column);
    }

  }
//}}}

// find
//{{{
cTextEditor::sPosition cTextEditor::findWordStart (const sPosition& from) const {

  sPosition at = from;
  if (at.mLineNumber >= (int)mLines.size())
    return at;

  const vector<sGlyph>& glyphs = mLines[at.mLineNumber].mGlyphs;
  int cindex = getCharacterIndex (at);

  if (cindex >= (int)glyphs.size())
    return at;

  while (cindex > 0 && isspace (glyphs[cindex].mChar))
    --cindex;

  ePalette cstart = glyphs[cindex].mColorIndex;
  while (cindex > 0) {
    uint8_t c = glyphs[cindex].mChar;
    if ((c & 0xC0) != 0x80) { // not UTF code sequence 10xxxxxx
      if (c <= 32 && isspace(c)) {
        cindex++;
        break;
        }
      if (cstart != glyphs[size_t(cindex - 1)].mColorIndex)
        break;
      }
    --cindex;
    }

  return sPosition (at.mLineNumber, getCharacterColumn (at.mLineNumber, cindex));
  }
//}}}
//{{{
cTextEditor::sPosition cTextEditor::findWordEnd (const sPosition& from) const {

  sPosition at = from;
  if (at.mLineNumber >= (int)mLines.size())
    return at;

  const vector<sGlyph>& glyphs = mLines[at.mLineNumber].mGlyphs;
  int cindex = getCharacterIndex (at);

  if (cindex >= (int)glyphs.size())
    return at;

  bool prevspace = (bool)isspace (glyphs[cindex].mChar);
  ePalette cstart = glyphs[cindex].mColorIndex;
  while (cindex < (int)glyphs.size()) {
    uint8_t c = glyphs[cindex].mChar;
    int d = utf8CharLength (c);
    if (cstart != glyphs[cindex].mColorIndex)
      break;

    if (prevspace != !!isspace(c)) {
      if (isspace(c))
        while (cindex < (int)glyphs.size() && isspace (glyphs[cindex].mChar))
          ++cindex;
      break;
      }
    cindex += d;
    }

  return sPosition (from.mLineNumber, getCharacterColumn (from.mLineNumber, cindex));
  }
//}}}
//{{{
cTextEditor::sPosition cTextEditor::findNextWord (const sPosition& from) const {

  sPosition at = from;
  if (at.mLineNumber >= (int)mLines.size())
    return at;

  // skip to the next non-word character
  bool isword = false;
  bool skip = false;

  int cindex = getCharacterIndex (from);
  if (cindex < (int)mLines[at.mLineNumber].mGlyphs.size()) {
    const vector<sGlyph>& glyphs = mLines[at.mLineNumber].mGlyphs;
    isword = isalnum (glyphs[cindex].mChar);
    skip = isword;
    }

  while (!isword || skip) {
    if (at.mLineNumber >= (int)mLines.size()) {
      int l = max(0, (int)mLines.size() - 1);
      return sPosition (l, getLineMaxColumn(l));
      }

    const vector<sGlyph>& glyphs = mLines[at.mLineNumber].mGlyphs;
    if (cindex < (int)glyphs.size()) {
      isword = isalnum (glyphs[cindex].mChar);

      if (isword && !skip)
        return sPosition (at.mLineNumber, getCharacterColumn (at.mLineNumber, cindex));

      if (!isword)
        skip = false;

      cindex++;
      }
    else {
      cindex = 0;
      ++at.mLineNumber;
      skip = false;
      isword = false;
      }
    }

  return at;
  }
//}}}

// move
//{{{
void cTextEditor::moveLeft (int amount, bool select, bool wordMode) {

  if (mLines.empty())
    return;

  sPosition oldPosition = mState.mCursorPosition;
  mState.mCursorPosition = getCursorPosition();

  int line = mState.mCursorPosition.mLineNumber;
  int cindex = getCharacterIndex (mState.mCursorPosition);

  while (amount-- > 0) {
    if (cindex == 0) {
      if (line > 0) {
        --line;
        if ((int)mLines.size() > line)
          cindex = (int)mLines[line].mGlyphs.size();
        else
          cindex = 0;
        }
      }
    else {
      --cindex;
      if (cindex > 0) {
        if ((int)mLines.size() > line) {
          while (cindex > 0 && isUtfSequence (mLines[line].mGlyphs[cindex].mChar))
            --cindex;
          }
        }
      }

    mState.mCursorPosition = sPosition (line, getCharacterColumn (line, cindex));
    if (wordMode) {
      mState.mCursorPosition = findWordStart (mState.mCursorPosition);
      cindex = getCharacterIndex (mState.mCursorPosition);
      }
    }

  mState.mCursorPosition = sPosition (line, getCharacterColumn (line, cindex));

  assert (mState.mCursorPosition.mColumn >= 0);
  if (select) {
    if (oldPosition == mInteractiveStart)
      mInteractiveStart = mState.mCursorPosition;
    else if (oldPosition == mInteractiveEnd)
      mInteractiveEnd = mState.mCursorPosition;
    else {
      mInteractiveStart = mState.mCursorPosition;
      mInteractiveEnd = oldPosition;
      }
    }
  else
    mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;

  setSelection (mInteractiveStart, mInteractiveEnd, select && wordMode ? eSelection::Word : eSelection::Normal);
  ensureCursorVisible();
  }
//}}}
//{{{
void cTextEditor::moveRight (int amount, bool select, bool wordMode) {

  sPosition oldPosition = mState.mCursorPosition;

  if (mLines.empty() || oldPosition.mLineNumber >= (int)mLines.size())
    return;

  int cindex = getCharacterIndex (mState.mCursorPosition);
  while (amount-- > 0) {
    int lindex = mState.mCursorPosition.mLineNumber;
    sLine& line = mLines [lindex];

    if (cindex >= (int)line.mGlyphs.size()) {
      if (mState.mCursorPosition.mLineNumber < (int)mLines.size() - 1) {
        mState.mCursorPosition.mLineNumber = max (0, min ((int)mLines.size() - 1, mState.mCursorPosition.mLineNumber + 1));
        mState.mCursorPosition.mColumn = 0;
        }
      else
        return;
      }
    else {
      cindex += utf8CharLength (line.mGlyphs[cindex].mChar);
      mState.mCursorPosition = sPosition (lindex, getCharacterColumn (lindex, cindex));
      if (wordMode)
         mState.mCursorPosition = findNextWord (mState.mCursorPosition);
     }
    }

  if (select) {
    if (oldPosition == mInteractiveEnd)
      mInteractiveEnd = sanitizePosition (mState.mCursorPosition);
    else if (oldPosition == mInteractiveStart)
      mInteractiveStart = mState.mCursorPosition;
    else {
      mInteractiveStart = oldPosition;
      mInteractiveEnd = mState.mCursorPosition;
      }
    }
  else
    mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;

  setSelection (mInteractiveStart, mInteractiveEnd,
                select && wordMode ? eSelection::Word : eSelection::Normal);

  ensureCursorVisible();
  }
//}}}

//{{{
void cTextEditor::moveUp (int amount) {

  sPosition oldPosition = mState.mCursorPosition;
  mState.mCursorPosition.mLineNumber = max (0, mState.mCursorPosition.mLineNumber - amount);

  if (oldPosition != mState.mCursorPosition) {
    mInteractiveStart = mState.mCursorPosition;
    mInteractiveEnd = mState.mCursorPosition;
    setSelection (mInteractiveStart, mInteractiveEnd);
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEditor::moveUpSelect (int amount) {

  sPosition oldPosition = mState.mCursorPosition;
  mState.mCursorPosition.mLineNumber = max (0, mState.mCursorPosition.mLineNumber - amount);

  if (oldPosition != mState.mCursorPosition) {
    if (oldPosition == mInteractiveStart)
      mInteractiveStart = mState.mCursorPosition;
    else if (oldPosition == mInteractiveEnd)
      mInteractiveEnd = mState.mCursorPosition;
    else {
      mInteractiveStart = mState.mCursorPosition;
      mInteractiveEnd = oldPosition;
      }

    setSelection (mInteractiveStart, mInteractiveEnd);
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEditor::moveDown (int amount) {

  assert (mState.mCursorPosition.mColumn >= 0);

  sPosition oldPosition = mState.mCursorPosition;
  mState.mCursorPosition.mLineNumber = max (0, min((int)mLines.size() - 1, mState.mCursorPosition.mLineNumber + amount));

  if (mState.mCursorPosition != oldPosition) {
    mInteractiveStart = mState.mCursorPosition;
    mInteractiveEnd = mState.mCursorPosition;
    setSelection (mInteractiveStart, mInteractiveEnd);
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEditor::moveDownSelect (int amount) {

  assert(mState.mCursorPosition.mColumn >= 0);

  sPosition oldPosition = mState.mCursorPosition;
  mState.mCursorPosition.mLineNumber = max (0, min((int)mLines.size() - 1, mState.mCursorPosition.mLineNumber + amount));

  if (mState.mCursorPosition != oldPosition) {
    if (oldPosition == mInteractiveEnd)
      mInteractiveEnd = mState.mCursorPosition;
    else if (oldPosition == mInteractiveStart)
      mInteractiveStart = mState.mCursorPosition;
    else {
      mInteractiveStart = oldPosition;
      mInteractiveEnd = mState.mCursorPosition;
      }

    setSelection (mInteractiveStart, mInteractiveEnd);
    ensureCursorVisible();
    }
  }
//}}}

// insert
//{{{
vector<cTextEditor::sGlyph>& cTextEditor::insertLine (int index) {

  assert (!mReadOnly);

  sLine& result = *mLines.insert (mLines.begin() + index, vector<sGlyph>());

  map<int,string> etmp;
  for (auto& marker : mMarkers)
    etmp.insert (map<int,string>::value_type (marker.first >= index ? marker.first + 1 : marker.first, marker.second));
  mMarkers = move (etmp);

  return result.mGlyphs;
  }
//}}}
//{{{
int cTextEditor::insertTextAt (sPosition& where, const char* value) {

  assert (!mReadOnly);

  int cindex = getCharacterIndex (where);
  int totalLines = 0;

  while (*value != '\0') {
    assert(!mLines.empty());

    if (*value == '\r') {
      // skip
      ++value;
      }
    else if (*value == '\n') {
      if (cindex < (int)mLines[where.mLineNumber].mGlyphs.size()) {
        vector <sGlyph>& newLine = insertLine (where.mLineNumber + 1);
        vector <sGlyph>& line = mLines[where.mLineNumber].mGlyphs;
        newLine.insert (newLine.begin(), line.begin() + cindex, line.end());
        line.erase (line.begin() + cindex, line.end());
        }
      else
        insertLine (where.mLineNumber + 1);

      ++where.mLineNumber;
      where.mColumn = 0;
      cindex = 0;
      ++totalLines;
      ++value;
      }

    else {
      vector <sGlyph>& glyphs = mLines[where.mLineNumber].mGlyphs;
      auto d = utf8CharLength (*value);
      while (d-- > 0 && *value != '\0')
        glyphs.insert (glyphs.begin() + cindex++, sGlyph (*value++, ePalette::Default));
      ++where.mColumn;
      }

    mTextChanged = true;
    }

  return totalLines;
  }
//}}}
//{{{
void cTextEditor::insertText (const char* value) {

  if (value == nullptr)
    return;

  sPosition position = getCursorPosition();
  sPosition startPosition = min (position, mState.mSelectionStart);

  int totalLines = position.mLineNumber - startPosition.mLineNumber;
  totalLines += insertTextAt (position, value);

  setSelection (position, position);
  setCursorPosition (position);

  colorize (startPosition.mLineNumber - 1, totalLines + 2);
  }
//}}}

// delete
//{{{
void cTextEditor::removeLine (int startPosition, int endPosition) {

  assert (!mReadOnly);
  assert (endPosition >= startPosition);
  assert (int(mLines.size()) > endPosition - startPosition);

  map<int,string> etmp;
  for (auto& marker : mMarkers) {
    map<int,string>::value_type e((marker.first >= startPosition) ? marker.first - 1 : marker.first, marker.second);
    if ((e.first >= startPosition) && (e.first <= endPosition))
      continue;
    etmp.insert (e);
    }
  mMarkers = move (etmp);

  mLines.erase (mLines.begin() + startPosition, mLines.begin() + endPosition);
  assert (!mLines.empty());

  mTextChanged = true;
  }
//}}}
//{{{
void cTextEditor::removeLine (int index) {

  assert(!mReadOnly);
  assert(mLines.size() > 1);

  map<int,string> etmp;
  for (auto& marker : mMarkers) {
    map<int,string>::value_type e(marker.first > index ? marker.first - 1 : marker.first, marker.second);
    if (e.first - 1 == index)
      continue;
    etmp.insert (e);
    }
  mMarkers = move (etmp);

  mLines.erase (mLines.begin() + index);
  assert (!mLines.empty());

  mTextChanged = true;
  }
//}}}
//{{{
void cTextEditor::deleteRange (const sPosition& startPosition, const sPosition& endPosition) {

  assert (endPosition >= startPosition);
  assert (!mReadOnly);

  //printf("D(%d.%d)-(%d.%d)\n", startPosition.mGlyphs, startPosition.mColumn, endPosition.mGlyphs, endPosition.mColumn);

  if (endPosition == startPosition)
    return;

  auto start = getCharacterIndex (startPosition);
  auto end = getCharacterIndex (endPosition);
  if (startPosition.mLineNumber == endPosition.mLineNumber) {
    vector<sGlyph>& glyphs = mLines[startPosition.mLineNumber].mGlyphs;
    int  n = getLineMaxColumn (startPosition.mLineNumber);
    if (endPosition.mColumn >= n)
      glyphs.erase (glyphs.begin() + start, glyphs.end());
    else
      glyphs.erase (glyphs.begin() + start, glyphs.begin() + end);
    }

  else {
    vector<sGlyph>& firstLine = mLines[startPosition.mLineNumber].mGlyphs;
    vector<sGlyph>& lastLine = mLines[endPosition.mLineNumber].mGlyphs;

    firstLine.erase (firstLine.begin() + start, firstLine.end());
    lastLine.erase (lastLine.begin(), lastLine.begin() + end);

    if (startPosition.mLineNumber < endPosition.mLineNumber)
      firstLine.insert (firstLine.end(), lastLine.begin(), lastLine.end());

    if (startPosition.mLineNumber < endPosition.mLineNumber)
      removeLine (startPosition.mLineNumber + 1, endPosition.mLineNumber + 1);
    }

  mTextChanged = true;
  }
//}}}

// undo
//{{{
void cTextEditor::addUndo (sUndoRecord& value) {

  assert(!mReadOnly);
  //printf("AddUndo: (@%d.%d) +\'%s' [%d.%d .. %d.%d], -\'%s', [%d.%d .. %d.%d] (@%d.%d)\n",
  //  value.mBefore.mCursorPosition.mGlyphs, value.mBefore.mCursorPosition.mColumn,
  //  value.mAdded.c_str(), value.mAddedStart.mGlyphs, value.mAddedStart.mColumn, value.mAddedEnd.mGlyphs, value.mAddedEnd.mColumn,
  //  value.mRemoved.c_str(), value.mRemovedStart.mGlyphs, value.mRemovedStart.mColumn, value.mRemovedEnd.mGlyphs, value.mRemovedEnd.mColumn,
  //  value.mAfter.mCursorPosition.mGlyphs, value.mAfter.mCursorPosition.mColumn
  //  );

  mUndoBuffer.resize ((size_t)(mUndoIndex + 1));
  mUndoBuffer.back() = value;
  ++mUndoIndex;
  }
//}}}

// colorize
//{{{
void cTextEditor::colorize (int fromLine, int lines) {

  int toLine = lines == -1 ? (int)mLines.size() : min((int)mLines.size(), fromLine + lines);

  mColorRangeMin = min (mColorRangeMin, fromLine);
  mColorRangeMax = max (mColorRangeMax, toLine);
  mColorRangeMin = max (0, mColorRangeMin);
  mColorRangeMax = max (mColorRangeMin, mColorRangeMax);

  mCheckComments = true;
  }
//}}}
//{{{
void cTextEditor::colorizeRange (int fromLine, int toLine) {

  if (mLines.empty() || fromLine >= toLine)
    return;

  string buffer;
  cmatch results;
  string id;

  int endLine = max(0, min((int)mLines.size(), toLine));
  for (int i = fromLine; i < endLine; ++i) {
    vector<sGlyph>& glyphs = mLines[i].mGlyphs;
    if (glyphs.empty())
      continue;

    buffer.resize (glyphs.size());
    for (size_t j = 0; j < glyphs.size(); ++j) {
      sGlyph& col = glyphs[j];
      buffer[j] = col.mChar;
      col.mColorIndex = ePalette::Default;
      }

    const char * bufferBegin = &buffer.front();
    const char * bufferEnd = bufferBegin + buffer.size();
    const char* last = bufferEnd;
    for (auto first = bufferBegin; first != last; ) {
      const char * token_begin = nullptr;
      const char * token_end = nullptr;
      ePalette token_color = ePalette::Default;

      bool hasTokenizeResult = false;
      if (mLanguage.mTokenize != nullptr) {
        if (mLanguage.mTokenize (first, last, token_begin, token_end, token_color))
          hasTokenizeResult = true;
        }

      if (hasTokenizeResult == false) {
        //printf("using regex for %.*s\n", first + 10 < last ? 10 : int(last - first), first);
        for (auto& p : mRegexList) {
          if (regex_search (first, last, results, p.first, regex_constants::match_continuous)) {
            hasTokenizeResult = true;
            auto& v = *results.begin();
            token_begin = v.first;
            token_end = v.second;
            token_color = p.second;
            break;
            }
          }
        }

      if (hasTokenizeResult == false)
        first++;

      else {
        const size_t token_length = token_end - token_begin;
        if (token_color == ePalette::Ident) {
          id.assign (token_begin, token_end);

          // almost all languages use lower case to specify keywords, so shouldn't this use ::tolower ?
          if (!mLanguage.mCaseSensitive)
            transform (id.begin(), id.end(), id.begin(),
                       [](uint8_t ch) { return static_cast<uint8_t>(std::toupper (ch)); });

          if (!glyphs[first - bufferBegin].mPreProc) {
            if (mLanguage.mKeywords.count (id) != 0)
              token_color = ePalette::Keyword;
            else if (mLanguage.mIdents.count (id) != 0)
              token_color = ePalette::KnownIdent;
            else if (mLanguage.mPreprocIdents.count (id) != 0)
              token_color = ePalette::PreprocIdent;
            }
          else if (mLanguage.mPreprocIdents.count (id) != 0)
            token_color = ePalette::PreprocIdent;
          }

        for (size_t j = 0; j < token_length; ++j)
          glyphs[(token_begin - bufferBegin) + j].mColorIndex = token_color;

        first = token_end;
        }
      }
    }
  }
//}}}
//{{{
void cTextEditor::colorizeInternal() {

  if (mLines.empty())
    return;

  if (mCheckComments) {
    int endLine = (int)mLines.size();
    int endIndex = 0;
    int commentStartLine = endLine;
    int commentStartIndex = endIndex;
    bool withinString = false;
    bool withinSingleLineComment = false;
    bool withinPreproc = false;
    bool firstChar = true;      // there is no other non-whitespace characters in the line before
    bool concatenate = false;   // '\' on the very end of the line
    int currentLine = 0;
    int currentIndex = 0;

    while (currentLine < endLine || currentIndex < endIndex) {
      vector<sGlyph>& line = mLines[currentLine].mGlyphs;
      if (currentIndex == 0 && !concatenate) {
        withinSingleLineComment = false;
        withinPreproc = false;
        firstChar = true;
        }
      concatenate = false;

      if (!line.empty()) {
        sGlyph& g = line[currentIndex];
        uint8_t c = g.mChar;
        if (c != mLanguage.mPreprocChar && !isspace(c))
          firstChar = false;
        if ((currentIndex == (int)line.size() - 1) && (line[line.size() - 1].mChar == '\\'))
          concatenate = true;

        bool inComment = (commentStartLine < currentLine ||
                          (commentStartLine == currentLine && commentStartIndex <= currentIndex));

        if (withinString) {
          line[currentIndex].mMultiLineComment = inComment;

          if (c == '\"') {
            if ((currentIndex + 1 < (int)line.size()) && (line[currentIndex + 1].mChar == '\"')) {
              currentIndex += 1;
              if (currentIndex < (int)line.size())
                line[currentIndex].mMultiLineComment = inComment;
              }
            else
              withinString = false;
            }
          else if (c == '\\') {
            currentIndex += 1;
            if (currentIndex < (int)line.size())
              line[currentIndex].mMultiLineComment = inComment;
            }
          }
        else {
          if (firstChar && c == mLanguage.mPreprocChar)
            withinPreproc = true;

          if (c == '\"') {
            withinString = true;
            line[currentIndex].mMultiLineComment = inComment;
            }
          else {
            auto pred = [](const char& a, const sGlyph& b) { return a == b.mChar; };
            auto from = line.begin() + currentIndex;
            string& startString = mLanguage.mCommentStart;
            string& singleStartString = mLanguage.mSingleLineComment;

            if ((singleStartString.size() > 0) &&
                (currentIndex + singleStartString.size() <= line.size()) &&
                equals (singleStartString.begin(), singleStartString.end(),
                        from, from + singleStartString.size(), pred)) {
              withinSingleLineComment = true;
              }
            else if ((!withinSingleLineComment && currentIndex + startString.size() <= line.size()) &&
                     equals (startString.begin(), startString.end(), from, from + startString.size(), pred)) {
              commentStartLine = currentLine;
              commentStartIndex = currentIndex;
              }

            inComment = (commentStartLine < currentLine ||
                         (commentStartLine == currentLine && commentStartIndex <= currentIndex));

            line[currentIndex].mMultiLineComment = inComment;
            line[currentIndex].mComment = withinSingleLineComment;

            string& endString = mLanguage.mCommentEnd;
            if (currentIndex + 1 >= (int)endString.size() &&
              equals (endString.begin(), endString.end(), from + 1 - endString.size(), from + 1, pred)) {
              commentStartIndex = endIndex;
              commentStartLine = endLine;
              }
            }
          }

        line[currentIndex].mPreProc = withinPreproc;
        currentIndex += utf8CharLength (c);
        if (currentIndex >= (int)line.size()) {
          currentIndex = 0;
          ++currentLine;
          }
        }
      else {
        currentIndex = 0;
        ++currentLine;
        }
      }
    mCheckComments = false;
    }

  if (mColorRangeMin < mColorRangeMax) {
    const int increment = (mLanguage.mTokenize == nullptr) ? 10 : 10000;
    const int to = min(mColorRangeMin + increment, mColorRangeMax);
    colorizeRange (mColorRangeMin, to);
    mColorRangeMin = to;

    if (mColorRangeMax == mColorRangeMin) {
      mColorRangeMin = numeric_limits<int>::max();
      mColorRangeMax = 0;
      }
    return;
    }
  }
//}}}
//}}}

// fold
//{{{
void cTextEditor::parseFolds() {
// parse beginFold and endFold markers, set simple flags

  for (auto& line : mLines) {
    string text;
    for (auto& glyph : line.mGlyphs)
      text += glyph.mChar;

    size_t foldBeginIndent = text.find (mLanguage.mFoldBeginMarker);
    line.mFoldBegin = (foldBeginIndent != string::npos);

    if (line.mFoldBegin) {
      line.mIndent = static_cast<uint16_t>(foldBeginIndent + mLanguage.mFoldBeginMarker.size());
      // has text after the foldBeginMarker
      line.mHasComment = (text.size() != (foldBeginIndent + mLanguage.mFoldBeginMarker.size()));
      }
    else {
      size_t indent = text.find_first_not_of (' ');
      if (indent != string::npos)
        line.mIndent = static_cast<uint16_t>(indent);
      else
        line.mIndent = 0;

      // has "//" style comment as first text in line
      line.mHasComment = (text.find (mLanguage.mSingleLineComment, indent) != string::npos);
      }

    size_t foldEndIndent = text.find (mLanguage.mFoldEndMarker);
    line.mFoldEnd = (foldEndIndent != string::npos);

    // init fields set by updateFolds
    line.mFoldOpen = false;
    line.mFoldLineNumber = 0;
    line.mFoldTitleLineNumber = 0xFFFFFFFF;
    }

  // create mVisibleLines
  mVisibleLines.clear();

  vector<sLine>::iterator it = mLines.begin();
  uint32_t lineNumber = 0;
  updateFold (it, lineNumber, true, true);
  }
//}}}
//{{{
void cTextEditor::updateFold (vector<sLine>::iterator& it, uint32_t& lineNumber,
                              bool parentOpen, bool foldOpen) {

  uint32_t beginLineNumber = lineNumber;

  if (parentOpen) {
    // if no comment search for first noComment line
    it->mFoldTitleLineNumber = it->mHasComment ? lineNumber : lineNumber + 1;
    mVisibleLines.push_back (lineNumber);
    }

  while (true) {
    it++;
    lineNumber++;
    if (it < mLines.end()) {
      it->mFoldLineNumber = beginLineNumber;
      if (it->mFoldBegin)
        updateFold (it, lineNumber, foldOpen, it->mFoldOpen);
      else if (it->mFoldEnd) {
        // update beginFold line with endFold lineNumber, helps reverse traversal
        mLines[beginLineNumber].mFoldLineNumber = lineNumber;
        return;
        }
      else if (foldOpen)
        mVisibleLines.push_back (lineNumber);
      }
    else
      return;
    }
  }
//}}}

//{{{
void cTextEditor::handleMouseInputs() {

  ImGuiIO& io = ImGui::GetIO();
  bool shift = io.KeyShift;
  bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

  if (ImGui::IsWindowHovered()) {
    if (!shift && !alt) {
      bool singleClick = ImGui::IsMouseClicked (0);
      bool doubleClick = ImGui::IsMouseDoubleClicked (0);
      bool tripleClick = singleClick &&
                         !doubleClick &&
                         ((mLastClick != -1.0f) && (ImGui::GetTime() - mLastClick) < io.MouseDoubleClickTime);

      // Left mouse tripleClick
      if (tripleClick) {
        if (!ctrl) {
          mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = screenToPosition (ImGui::GetMousePos());
          mSelection = eSelection::Line;
          setSelection (mInteractiveStart, mInteractiveEnd, mSelection);
          }
        mLastClick = -1.0f;
        }

      // left mouse doubleClick
      else if (doubleClick) {
        if (!ctrl) {
          mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = screenToPosition(ImGui::GetMousePos());
          if (mSelection == eSelection::Line)
            mSelection = eSelection::Normal;
          else
            mSelection = eSelection::Word;
          setSelection (mInteractiveStart, mInteractiveEnd, mSelection);
          }
        mLastClick = static_cast<float>(ImGui::GetTime());
        }

      // left mouse sinngleClick
      else if (singleClick) {
        mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = screenToPosition (ImGui::GetMousePos());
        if (ctrl)
          mSelection = eSelection::Word;
        else
          mSelection = eSelection::Normal;
        setSelection (mInteractiveStart, mInteractiveEnd, mSelection);
        mLastClick = static_cast<float>(ImGui::GetTime());
        }

      // left mouse button dragging (=> update selection)
      else if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0)) {
        io.WantCaptureMouse = true;
        mState.mCursorPosition = mInteractiveEnd = screenToPosition (ImGui::GetMousePos());
        setSelection (mInteractiveStart, mInteractiveEnd, mSelection);
        }
      }
    }
  }
//}}}
//{{{
void cTextEditor::handleKeyboardInputs() {

  //{{{
  struct sActionKey {
    bool mAlt;
    bool mCtrl;
    bool mShift;
    ImGuiKey mKey;
    bool mWritable;
    function <void()> mActionFunc;
    };
  //}}}
  const vector <sActionKey> kActionKeys = {
  //  alt    ctrl   shift  key               writable         function
     // edit
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
     {false, false, false, ImGuiKey_UpArrow,    false, [this]{moveLineUp();}},
     {false, false, true,  ImGuiKey_UpArrow,    false, [this]{moveLineUpSelect();}},
     {false, false, false, ImGuiKey_DownArrow,  false, [this]{moveLineDown();}},
     {false, false, true,  ImGuiKey_DownArrow,  false, [this]{moveLineDownSelect();}},
     {false, false, false, ImGuiKey_LeftArrow,  false, [this]{moveLeft();}},
     {false, true,  false, ImGuiKey_LeftArrow,  false, [this]{moveLeftWord();}},
     {false, false, true,  ImGuiKey_LeftArrow,  false, [this]{moveLeftSelect();}},
     {false, true,  true,  ImGuiKey_LeftArrow,  false, [this]{moveLeftWordSelect();}},
     {false, false, false, ImGuiKey_RightArrow, false, [this]{moveRight();}},
     {false, true,  false, ImGuiKey_RightArrow, false, [this]{moveRightWord();}},
     {false, false, true,  ImGuiKey_RightArrow, false, [this]{moveRightSelect();}},
     {false, true,  true,  ImGuiKey_RightArrow, false, [this]{moveRightWordSelect();}},
     {false, false, false, ImGuiKey_PageUp,     false, [this]{movePageUp();}},
     {false, false, true,  ImGuiKey_PageUp,     false, [this]{movePageUpSelect();}},
     {false, false, false, ImGuiKey_PageDown,   false, [this]{movePageDown();}},
     {false, false, true,  ImGuiKey_PageDown,   false, [this]{movePageDownSelect();}},
     {false, false, false, ImGuiKey_Home,       false, [this]{moveHome();}},
     {false, true,  false, ImGuiKey_Home,       false, [this]{moveTop();}},
     {false, true,  true,  ImGuiKey_Home,       false, [this]{moveTopSelect();}},
     {false, false, false, ImGuiKey_End,        false, [this]{moveEnd();}},
     {false, true,  false, ImGuiKey_End,        false, [this]{moveBottom();}},
     {false, true,  true,  ImGuiKey_End,        false, [this]{moveBottomSelect();}},
     // toggle mode
     {false, false, false, ImGuiKey_Insert,     false, [this]{toggleOverwrite();}},
     {true,  false, false, ImGuiKey_Space,      false, [this]{toggleFolded();}},
     {true,  false, false, ImGuiKey_PageUp,     false, [this]{toggleLineNumbers();}},
     {true,  false, false, ImGuiKey_PageDown,   false, [this]{toggleLineDebug();}},
     {true,  false, false, ImGuiKey_Home,       false, [this]{toggleLineWhiteSpace();}},
     };

  if (!ImGui::IsWindowFocused())
    return;

  if (ImGui::IsWindowHovered())
    ImGui::SetMouseCursor (ImGuiMouseCursor_TextInput);

  ImGuiIO& io = ImGui::GetIO();
  bool shift = io.KeyShift;
  bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;
  io.WantCaptureKeyboard = true;
  io.WantTextInput = true;

  for (auto& actionKey : kActionKeys)
    //{{{  dispatch matched actionKey
    if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (actionKey.mKey)) &&
        (!actionKey.mWritable || (actionKey.mWritable && !isReadOnly())) &&
        (actionKey.mCtrl == ctrl) &&
        (actionKey.mShift == shift) &&
        (actionKey.mAlt == alt)) {

      actionKey.mActionFunc();
      break;
      }
    //}}}

  if (isReadOnly())
    return;

  // character keys
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
//}}}

//{{{
void cTextEditor::preRender() {
//  setup render context

  mFont = ImGui::GetFont();
  mFontSize = ImGui::GetFontSize();
  mContentSize = ImGui::GetWindowContentRegionMax();
  mDrawList = ImGui::GetWindowDrawList();
  mCursorScreenPos = ImGui::GetCursorScreenPos();
  mFocused = ImGui::IsWindowFocused();

  // update palette alpha from style
  for (uint8_t i = 0; i < (uint8_t)ePalette::Max; ++i) {
    ImVec4 color = ImGui::ColorConvertU32ToFloat4 (mPaletteBase[i]);
    color.w *= ImGui::GetStyle().Alpha;
    mPalette[i] = ImGui::ColorConvertFloat4ToU32 (color);
    }

  // calc character mCharSize
  mCharSize = ImVec2(mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, " ", nullptr, nullptr).x,
                     ImGui::GetTextLineHeightWithSpacing() * mLineSpacing);

  if (mScrollToTop) {
    mScrollToTop = false;
    ImGui::SetScrollY (0.f);
    }

  // measure lineNumber width
  float lineNumberWidth = 0.f;
  if (mShowLineDebug) {
    //{{{  add lineDebug width to mGlyphsStart
    char str[32];
    snprintf (str, sizeof(str), "%4d:%4d:%4d ",1,1,1);
    lineNumberWidth = mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, str, nullptr, nullptr).x;
    }
    //}}}
  else if (mShowLineNumbers) {
    //{{{  add lineNumber width to mGlyphsStart
    char str[32];
    snprintf (str, sizeof(str), "%d ", (int)mLines.size());
    lineNumberWidth = mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, str, nullptr, nullptr).x;
    }
    //}}}
  mGlyphsStart = kLeftTextMargin + lineNumberWidth;
  mMaxWidth = 0;

  // calc lineIndex, maxLineIndex, lineNumber from scroll
  mScrollY = ImGui::GetScrollY();
  mLineIndex = static_cast<uint32_t>(floor (mScrollY / mCharSize.y));
  mMaxLineIndex = mLineIndex + static_cast<uint32_t>(ceil ((mScrollY + mContentSize.y) / mCharSize.y));

  if (mShowFolded) {
    mLineIndex = min (mLineIndex, static_cast<uint32_t>(mVisibleLines.size()-1));
    mMaxLineIndex = max (0u, min (static_cast<uint32_t>(mVisibleLines.size()-1), mMaxLineIndex));
    }
  else {
    mLineIndex = min (mLineIndex, static_cast<uint32_t>(mLines.size()-1));
    mMaxLineIndex = max (0u, min (static_cast<uint32_t>(mLines.size()-1), mMaxLineIndex));
    }

  mScrollX = ImGui::GetScrollX();
  mCursorPos = mCursorScreenPos + ImVec2 (mScrollX, mLineIndex * mCharSize.y);
  mLinePos = {mCursorScreenPos.x, mCursorPos.y};
  mTextPos = {mCursorScreenPos.x + mGlyphsStart, mCursorPos.y};
  }
//}}}
//{{{
void cTextEditor::renderLine (uint32_t lineNumber, uint32_t beginFoldLineNumber) {

  // c style str buffer, null terminated
  char str[256];
  char* strPtr = str;
  char* strLastPtr = str + sizeof(str) - 1;

  //{{{  draw select background
  float xStart = -1.0f;
  float xEnd = -1.0f;

  sPosition lineStartPosition (lineNumber, 0);
  sPosition lineEndPosition (lineNumber, getLineMaxColumn (lineNumber));

  if (mState.mSelectionStart <= lineEndPosition)
    xStart = mState.mSelectionStart > lineStartPosition ? getTextWidth (mState.mSelectionStart) : 0.0f;

  if (mState.mSelectionEnd > lineStartPosition)
    xEnd = getTextWidth (mState.mSelectionEnd < lineEndPosition ? mState.mSelectionEnd : lineEndPosition);

  if (mState.mSelectionEnd.mLineNumber > static_cast<int>(lineNumber))
    xEnd += mCharSize.x;

  if ((xStart != -1) && (xEnd != -1) && (xStart < xEnd))
    mDrawList->AddRectFilled (mLinePos + ImVec2 (mGlyphsStart + xStart, 0.f),
                              mLinePos + ImVec2 (mGlyphsStart + xEnd,  mCharSize.y),
                              mPalette[(size_t)ePalette::Selection]);
  //}}}
  //{{{  draw marker background
  auto markerIt = mMarkers.find (lineNumber + 1);
  if (markerIt != mMarkers.end()) {
    ImVec2 markerEndPos = mLinePos + ImVec2(mScrollX + mContentSize.x, mCharSize.y);
    mDrawList->AddRectFilled (mCursorPos, markerEndPos, mPalette[(size_t)ePalette::Marker]);

    if (ImGui::IsMouseHoveringRect (mLinePos, markerEndPos)) {
      ImGui::BeginTooltip();

      ImGui::PushStyleColor (ImGuiCol_Text, ImVec4(1.f,1.f,1.f, 1.f));
      ImGui::Text ("marker at line %d:", markerIt->first);
      ImGui::Separator();
      ImGui::Text ("%s", markerIt->second.c_str());
      ImGui::PopStyleColor();

      ImGui::EndTooltip();
      }
    }
  //}}}
  //{{{  draw cursor background
  if (!hasSelect() && (lineNumber == static_cast<uint32_t>(mState.mCursorPosition.mLineNumber))) {
    // highlight cursor line before drawText
    ImVec2 cursorEndPos = mCursorPos + ImVec2(mScrollX + mContentSize.x, mCharSize.y);

    mDrawList->AddRectFilled (mCursorPos, cursorEndPos,
      mPalette[(int)(mFocused ? ePalette::CurrentLineFill : ePalette::CurrentLineFillInactive)]);

    mDrawList->AddRect (mCursorPos, cursorEndPos, mPalette[(size_t)ePalette::CurrentLineEdge], 1.0f);
    }
  //}}}

  sLine& line = mLines[lineNumber];
  if (mShowLineDebug) {
    //{{{  draw lineDebug, rightJustified
    snprintf (str, sizeof(str), "%4d:%4d:%4d ", line.mFoldLineNumber+1, line.mFoldTitleLineNumber+1, lineNumber+1);
    float strWidth = mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, str, nullptr, nullptr).x;

    mDrawList->AddText (ImVec2 (mGlyphsStart + mLinePos.x - strWidth, mLinePos.y),
                        mPalette[(size_t)ePalette::LineNumber], str);
    }
    //}}}
  else if (mShowLineNumbers) {
    //{{{  draw lineNumber, rightJustified
    snprintf (str, sizeof(str), "%d ", lineNumber+1);
    float lineNumberWidth = mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, str, nullptr, nullptr).x;

    mDrawList->AddText (ImVec2 (mLinePos.x + mGlyphsStart - lineNumberWidth, mLinePos.y),
                        mPalette[(size_t)ePalette::LineNumber], str);
    }
    //}}}

  ImU32 prefixColor = 0;
  bool forcePrefixColor = false;
  if (mShowFolded) {
    sLine& foldLine = mLines[beginFoldLineNumber];
    //{{{  draw fold prefix
    if (foldLine.mFoldBegin) {
      if (foldLine.mFoldOpen) {
        // foldBegin - foldOpen
        prefixColor = mPalette[(size_t)ePalette::FoldBeginOpen];
        mDrawList->AddText (mTextPos, prefixColor, mLanguage.mFoldBeginOpen.c_str());
        mTextPos.x += mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, mLanguage.mFoldBeginOpen.c_str(), nullptr, nullptr).x;
        }
      else {
        // foldBegin - foldClosed
        string prefixString;
        for (uint8_t i = 0; i < line.mIndent; i++)
          prefixString += ' ';
        prefixString += mLanguage.mFoldBeginClosed;
        prefixColor = mPalette[(size_t)ePalette::FoldBeginClosed];
        mDrawList->AddText (mTextPos, prefixColor, prefixString.c_str());
        mTextPos.x += mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, prefixString.c_str(), nullptr, nullptr).x;
        forcePrefixColor = true;
        }
      }

    else if (foldLine.mFoldEnd) {
      // foldEnd
      string prefixString = mLanguage.mFoldEnd;
      prefixColor = mPalette[(size_t)ePalette::FoldEnd];
      mDrawList->AddText (mTextPos, prefixColor, mLanguage.mFoldEnd.c_str());
      mTextPos.x += mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, mLanguage.mFoldEnd.c_str(), nullptr, nullptr).x;

      // no more to draw
      return;
      }
    //}}}
    }

  // draw main text
  vector<sGlyph>& glyphs = line.mGlyphs;
  ImU32 prevColor = glyphs.empty() ? mPalette[(size_t)ePalette::Default] : getGlyphColor (glyphs[0]);
  for (auto& glyph : glyphs) {
    // write text on colour change
    ImU32 color = getGlyphColor (glyph);
    if (((color != prevColor) || (glyph.mChar == '\t') || (glyph.mChar == ' ')) && (strPtr != str)) {
      //{{{  draw colored glyphs word
      // draw and measure text
      *strPtr = 0;
      mDrawList->AddText (mTextPos, forcePrefixColor ? prefixColor : prevColor, str);
      mTextPos.x += mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, str, nullptr, nullptr).x;
      strPtr = str;

      prevColor = color;
      }
      //}}}

    if (glyph.mChar == '\t') {
      //{{{  draw tab
      const ImVec2 pos1 = mTextPos + ImVec2 (1.0f, mFontSize * 0.5f);
      mTextPos.x = (1.0f + floor ((1.0f + mTextPos.x) / (mTabSize * mCharSize.x))) * (mTabSize * mCharSize.x);

      if (mShowWhiteSpace) {
        // righthand of tab arrow
        const ImVec2 pos2 = mTextPos + ImVec2 (-1.0f, mFontSize * 0.5f);

        // draw tab whiteSpace marker
        mDrawList->AddLine (pos1, pos2, mPalette[(size_t)ePalette::Tab]);
        mDrawList->AddLine (pos2, pos2 + ImVec2 (mFontSize * -0.2f, mFontSize * -0.2f), mPalette[(size_t)ePalette::Tab]);
        mDrawList->AddLine (pos2, pos2 + ImVec2 (mFontSize * -0.2f, mFontSize * 0.2f), mPalette[(size_t)ePalette::Tab]);
        }
      }
      //}}}
    else if (glyph.mChar == ' ') {
      //{{{  draw space
      if (mShowWhiteSpace)
        mDrawList->AddCircleFilled (mTextPos + ImVec2(mCharSize.x * 0.5f, mFontSize * 0.5f), 1.5f,
                                    mPalette[(size_t)ePalette::WhiteSpace], 4);
      mTextPos.x += mCharSize.x;
      }
      //}}}
    else {
      //{{{  add char to str
      int l = utf8CharLength (glyph.mChar);
      while ((l-- > 0) && (strPtr < strLastPtr))
        *strPtr++ = glyph.mChar;
      }
      //}}}
    }
  if (strPtr != str) {
    //{{{  draw remaining glyphs
    *strPtr = 0;
    mDrawList->AddText (mTextPos, forcePrefixColor ? prefixColor : prevColor, str);
    }
    //}}}

  // cursor symbol ?
  if (mFocused && (lineNumber == static_cast<uint32_t>(mState.mCursorPosition.mLineNumber))) {
    //{{{  draw flashing cursor after drawText
    auto timeEnd = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    auto elapsed = timeEnd - mStartTime;
    if (elapsed > 400) {
      float cursorWidth = 1.f;
      int cindex = getCharacterIndex (mState.mCursorPosition);
      float cx = getTextWidth (mState.mCursorPosition);

      if (mOverwrite && (cindex < (int)glyphs.size())) {
        if (glyphs[cindex].mChar == '\t') {
          float x = (1.0f + floor((1.0f + cx) / (float(mTabSize) * mCharSize.x))) * (float(mTabSize) * mCharSize.x);
          cursorWidth = x - cx;
          }
        else {
          char cursorBuf[2];
          cursorBuf[0] = glyphs[cindex].mChar;
          cursorBuf[1] = 0;
          cursorWidth = mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.0f, cursorBuf).x;
          }
        }
      mDrawList->AddRectFilled ({mTextPos.x + cx, mLinePos.y},
                               {mTextPos.x + cx + cursorWidth, mLinePos.y + mCharSize.y},
                               mPalette[(size_t)ePalette::Cursor]);
      if (elapsed > 800)
        mStartTime = timeEnd;
      }
    }
    //}}}

  // expand mMaxWidth with our maxWidth, !!! what about prefix width !!!
  mMaxWidth = max (mMaxWidth, mGlyphsStart + getTextWidth (sPosition (lineNumber, getLineMaxColumn (lineNumber))));

  // nextLine
  mCursorPos.y += mCharSize.y;
  mLinePos.y += mCharSize.y;
  mTextPos.x = mCursorScreenPos.x + mGlyphsStart;
  mTextPos.y += mCharSize.y;
  }
//}}}
//{{{
void cTextEditor::postRender() {

  // draw tooltip for idents,preProcs
  if (ImGui::IsMousePosValid()) {
    string id = getWordAt (screenToPosition (ImGui::GetMousePos()));
    if (!id.empty()) {
      auto identIt = mLanguage.mIdents.find (id);
      if (identIt != mLanguage.mIdents.end()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted (identIt->second.mDeclaration.c_str());
        ImGui::EndTooltip();
        }

      else {
        auto preProcIdentIt = mLanguage.mPreprocIdents.find (id);
        if (preProcIdentIt != mLanguage.mPreprocIdents.end()) {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted (preProcIdentIt->second.mDeclaration.c_str());
          ImGui::EndTooltip();
          }
        }
      }
    }

  // dummy button, sized to maximum width,height, sets scroll regions without drawing them
  ImGui::Dummy ({mMaxWidth, (mShowFolded ? mVisibleLines.size() : mLines.size()) * mCharSize.y});

  if (mScrollToCursor) {
    // scroll to cursor
    ensureCursorVisible();
    ImGui::SetWindowFocus();
    mScrollToCursor = false;
    }
  }
//}}}

// cTextEditor::sUndoRecord
//{{{
cTextEditor::sUndoRecord::sUndoRecord (const string& added,
                                       const cTextEditor::sPosition addedStart,
                                       const cTextEditor::sPosition addedEnd,
                                       const string& removed,
                                       const cTextEditor::sPosition removedStart,
                                       const cTextEditor::sPosition removedEnd,
                                       cTextEditor::sEditorState& before,
                                       cTextEditor::sEditorState& after)

    : mAdded(added), mAddedStart(addedStart), mAddedEnd(addedEnd),
      mRemoved(removed), mRemovedStart(removedStart), mRemovedEnd(removedEnd),
      mBefore(before), mAfter(after) {

  assert (mAddedStart <= mAddedEnd);
  assert (mRemovedStart <= mRemovedEnd);
  }
//}}}
//{{{
void cTextEditor::sUndoRecord::undo (cTextEditor* editor) {

  if (!mAdded.empty()) {
    editor->deleteRange (mAddedStart, mAddedEnd);
    editor->colorize (mAddedStart.mLineNumber - 1, mAddedEnd.mLineNumber - mAddedStart.mLineNumber + 2);
    }

  if (!mRemoved.empty()) {
    sPosition start = mRemovedStart;
    editor->insertTextAt (start, mRemoved.c_str());
    editor->colorize (mRemovedStart.mLineNumber - 1, mRemovedEnd.mLineNumber - mRemovedStart.mLineNumber + 2);
    }

  editor->mState = mBefore;
  editor->ensureCursorVisible();
  }
//}}}
//{{{
void cTextEditor::sUndoRecord::redo (cTextEditor* editor) {

  if (!mRemoved.empty()) {
    editor->deleteRange (mRemovedStart, mRemovedEnd);
    editor->colorize (mRemovedStart.mLineNumber - 1, mRemovedEnd.mLineNumber - mRemovedStart.mLineNumber + 1);
    }

  if (!mAdded.empty()) {
    sPosition start = mAddedStart;
    editor->insertTextAt (start, mAdded.c_str());
    editor->colorize (mAddedStart.mLineNumber - 1, mAddedEnd.mLineNumber - mAddedStart.mLineNumber + 1);
    }

  editor->mState = mAfter;
  editor->ensureCursorVisible();
  }
//}}}

// cTextEditor::sLanguage
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::cPlus() {

  static sLanguage language;
  static bool inited = false;

  if (!inited) {
    for (auto& cppKeyword : kCppKeywords)
      language.mKeywords.insert (cppKeyword);

    for (auto& cppIdent : kCppIdents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair (cppIdent, id));
      }

    language.mTokenize = [](const char* inBegin, const char* inEnd,
                            const char*& outBegin, const char*& outEnd, ePalette& palette) -> bool {
      //{{{  tokenize lambda
      palette = ePalette::Max;

      while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
        inBegin++;

      if (inBegin == inEnd) {
        outBegin = inEnd;
        outEnd = inEnd;
        palette = ePalette::Default;
        }
      else if (findString (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::String;
      else if (findCharacterLiteral (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::CharLiteral;
      else if (findIdent (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::Ident;
      else if (findNumber (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::Number;
      else if (findPunctuation (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::Punctuation;

      return palette != ePalette::Max;
      };
      //}}}

    language.mCommentStart = "/*";
    language.mCommentEnd = "*/";
    language.mSingleLineComment = "//";
    language.mFoldBeginMarker = "//{{{";
    language.mFoldEndMarker = "//}}}";
    language.mFoldBeginOpen = "{{{";
    language.mFoldBeginClosed = "... ";
    language.mFoldEnd = "}}}";
    language.mCaseSensitive = true;
    language.mAutoIndentation = true;
    language.mName = "C++";

    inited = true;
    }

  return language;
  }
//}}}
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::c() {

  static sLanguage language;
  static bool inited = false;

  if (!inited) {
    for (auto& keywordString : kCKeywords)
      language.mKeywords.insert (keywordString);

    for (auto& identString : kCIdents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair (identString, id));
      }

    language.mTokenize = [](const char * inBegin, const char * inEnd,
                           const char *& outBegin, const char *& outEnd, ePalette & palette) -> bool {
      palette = ePalette::Max;

      while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
        inBegin++;

      if (inBegin == inEnd) {
        outBegin = inEnd;
        outEnd = inEnd;
        palette = ePalette::Default;
        }
      else if (findString (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::String;
      else if (findCharacterLiteral (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::CharLiteral;
      else if (findIdent (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::Ident;
      else if (findNumber (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::Number;
      else if (findPunctuation (inBegin, inEnd, outBegin, outEnd))
        palette = ePalette::Punctuation;

      return palette != ePalette::Max;
      };

    language.mCommentStart = "/*";
    language.mCommentEnd = "*/";
    language.mSingleLineComment = "//";
    language.mFoldBeginMarker = "//{{{";
    language.mFoldEndMarker = "//}}}";
    language.mFoldBeginOpen = "{{{";
    language.mFoldBeginClosed = "... ";
    language.mFoldEnd = "}}}";
    language.mCaseSensitive = true;
    language.mAutoIndentation = true;
    language.mName = "C";

    inited = true;
    }

  return language;
  }
//}}}
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::hlsl() {

  static bool inited = false;
  static sLanguage language;

  if (!inited) {
    for (auto& keywordString : kHlslKeywords)
      language.mKeywords.insert (keywordString);

    for (auto& identString : kHlslIdents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair (identString, id));
      }

    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[ \\t]*#[ \\t]*[a-zA-Z_]+", ePalette::Preprocessor));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("L?\\\"(\\\\.|[^\\\"])*\\\"", ePalette::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("\\'\\\\?[^\\']\\'", ePalette::CharLiteral));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ePalette::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ePalette::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("0[0-7]+[Uu]?[lL]?[lL]?", ePalette::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ePalette::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[a-zA-Z_][a-zA-Z0-9_]*", ePalette::Ident));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ePalette::Punctuation));

    language.mCommentStart = "/*";
    language.mCommentEnd = "*/";
    language.mSingleLineComment = "//";
    language.mFoldBeginMarker = "#{{{";
    language.mFoldEndMarker = "#}}}";
    language.mFoldBeginOpen = "{{{";
    language.mFoldBeginClosed = "... ";
    language.mFoldEnd = "}}}";
    language.mCaseSensitive = true;
    language.mAutoIndentation = true;
    language.mName = "HLSL";

    inited = true;
    }

  return language;
  }
//}}}
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::glsl() {

  static bool inited = false;
  static sLanguage language;

  if (!inited) {
    for (auto& keywordString : kGlslKeywords)
      language.mKeywords.insert (keywordString);

    for (auto& identString : kGlslIdents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair (identString, id));
      }

    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[ \\t]*#[ \\t]*[a-zA-Z_]+", ePalette::Preprocessor));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("L?\\\"(\\\\.|[^\\\"])*\\\"", ePalette::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("\\'\\\\?[^\\']\\'",ePalette::CharLiteral));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ePalette::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ePalette::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("0[0-7]+[Uu]?[lL]?[lL]?", ePalette::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ePalette::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[a-zA-Z_][a-zA-Z0-9_]*", ePalette::Ident));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePalette>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ePalette::Punctuation));

    language.mCommentStart = "/*";
    language.mCommentEnd = "*/";
    language.mSingleLineComment = "//";
    language.mFoldBeginMarker = "#{{{";
    language.mFoldEndMarker = "#}}}";
    language.mFoldBeginOpen = "{{{";
    language.mFoldBeginClosed = "... ";
    language.mFoldEnd = "}}}";
    language.mCaseSensitive = true;
    language.mAutoIndentation = true;
    language.mName = "GLSL";

    inited = true;
    }

  return language;
  }
//}}}
