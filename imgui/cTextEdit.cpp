// cTextEdit.cpp - nicked from https://github.com/BalazsJako/ImGuiColorTextEdit
//{{{  includes
#include "cTextEdit.h"

#include <cmath>
#include <algorithm>
#include <functional>
#include <chrono>

#include "../ui/cApp.h"
#include "../imgui/myImguiWidgets.h"
#include "../platform/cPlatform.h"

#include "../utils/cLog.h"

using namespace std;
using namespace chrono;
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

namespace {
  //{{{  const
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

  const char* kChildScrollStr = "##scrolling";
  //{{{
  const array <ImU32, (size_t)cTextEdit::ePalette::Max> kLightPalette = {
    0xff606060, // None
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
    0xff0000ff, // FoldEnd,
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

// cTextEdit
//{{{
cTextEdit::cTextEdit()
   : mLastFlashTime(system_clock::now()) {

  mInfo.mLines.push_back (vector<sGlyph>());
  mOptions.mPalette = kLightPalette;
  setLanguage (sLanguage::cPlus());
  }
//}}}
//{{{  gets
//{{{
bool cTextEdit::hasClipboardText() {

  const char* clipText = ImGui::GetClipboardText();
  return (clipText != nullptr) && (strlen (clipText) > 0);
  }
//}}}

//{{{
string cTextEdit::getTextString() const {
// get text as single string

  return getText (sPosition(), sPosition((int)mInfo.mLines.size(), 0));
  }
//}}}
//{{{
vector<string> cTextEdit::getTextStrings() const {
// get text as vector of string

  vector<string> result;
  result.reserve (mInfo.mLines.size());

  for (auto& line : mInfo.mLines) {
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
void cTextEdit::setTextString (const string& text) {
// break test into lines, store in internal mLines structure

  mInfo.mLines.clear();
  mInfo.mLines.emplace_back (vector<sGlyph>());

  for (auto ch : text) {
    if (ch == '\r') // ignored but flag set
      mInfo.mHasCR = true;
    else if (ch == '\n')
      mInfo.mLines.emplace_back (vector<sGlyph>());
    else {
      if (ch ==  '\t')
        mInfo.mHasTabs = true;
      mInfo.mLines.back().mGlyphs.emplace_back (sGlyph (ch, ePalette::Default));
      }
    }

  mInfo.mTextEdited = false;

  mUndoList.mBuffer.clear();
  mUndoList.mIndex = 0;

  parseFolds();
  colorize();
  }
//}}}
//{{{
void cTextEdit::setTextStrings (const vector<string>& lines) {
// store vector of lines in internal mLines structure

  mInfo.mLines.clear();

  if (lines.empty())
    mInfo.mLines.emplace_back (vector<sGlyph>());

  else {
    mInfo.mLines.resize (lines.size());
    for (size_t i = 0; i < lines.size(); ++i) {
      const string& line = lines[i];
      mInfo.mLines[i].mGlyphs.reserve (line.size());
      for (size_t j = 0; j < line.size(); ++j) {
        if (line[j] ==  '\t')
          mInfo.mHasTabs = true;
        mInfo.mLines[i].mGlyphs.emplace_back (sGlyph (line[j], ePalette::Default));
        }
      }
    }

  mInfo.mTextEdited = false;

  mUndoList.mBuffer.clear();
  mUndoList.mIndex = 0;

  parseFolds();
  colorize();
  }
//}}}

//{{{
void cTextEdit::setLanguage (const sLanguage& language) {

  mOptions.mLanguage = language;

  mOptions.mRegexList.clear();
  for (auto& r : mOptions.mLanguage.mTokenRegexStrings)
    mOptions.mRegexList.push_back (make_pair (regex (r.first, regex_constants::optimize), r.second));

  colorize();
  }
//}}}

//{{{
void cTextEdit::setCursorPosition (const sPosition& position) {

  if (mEdit.mState.mCursorPosition != position) {
    mEdit.mState.mCursorPosition = position;
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::setSelectionStart (const sPosition& position) {

  mEdit.mState.mSelectionStart = sanitizePosition (position);
  if (mEdit.mState.mSelectionStart > mEdit.mState.mSelectionEnd)
    swap (mEdit.mState.mSelectionStart, mEdit.mState.mSelectionEnd);
  }
//}}}
//{{{
void cTextEdit::setSelectionEnd (const sPosition& position) {

  mEdit.mState.mSelectionEnd = sanitizePosition (position);
  if (mEdit.mState.mSelectionStart > mEdit.mState.mSelectionEnd)
    swap (mEdit.mState.mSelectionStart, mEdit.mState.mSelectionEnd);
  }
//}}}
//{{{
void cTextEdit::setSelection (const sPosition& startPosition, const sPosition& endPosition, eSelection mode) {

  mEdit.mState.mSelectionStart = sanitizePosition (startPosition);
  mEdit.mState.mSelectionEnd = sanitizePosition (endPosition);
  if (mEdit.mState.mSelectionStart > mEdit.mState.mSelectionEnd)
    swap (mEdit.mState.mSelectionStart, mEdit.mState.mSelectionEnd);

  switch (mode) {
    case cTextEdit::eSelection::Normal:
      break;

    case cTextEdit::eSelection::Word: {
      mEdit.mState.mSelectionStart = findWordStart (mEdit.mState.mSelectionStart);
      if (!isOnWordBoundary (mEdit.mState.mSelectionEnd))
        mEdit.mState.mSelectionEnd = findWordEnd (findWordStart (mEdit.mState.mSelectionEnd));
      break;
      }

    case cTextEdit::eSelection::Line: {
      const int lineNumber = mEdit.mState.mSelectionEnd.mLineNumber;
      //const auto lineSize = (size_t)lineNumber < mLines.size() ? mLines[lineNumber].size() : 0;
      mEdit.mState.mSelectionStart = sPosition (mEdit.mState.mSelectionStart.mLineNumber, 0);
      mEdit.mState.mSelectionEnd = sPosition (lineNumber, getLineMaxColumn (lineNumber));
      break;
      }

    default:
      break;
    }
  }
//}}}
//}}}
//{{{  actions
// move
//{{{
void cTextEdit::moveLeft() {

  if (mInfo.mLines.empty())
    return;

  sPosition position = mEdit.mState.mCursorPosition;

  //mEdit.mState.mCursorPosition = getCursorPosition();
  int column = getCharacterIndex (mEdit.mState.mCursorPosition);
  int lineNumber = mEdit.mState.mCursorPosition.mLineNumber;
  if (column == 0) {
    // move to end of prevous line
    if (lineNumber > 0) {
      --lineNumber;
      if ((int)mInfo.mLines.size() > lineNumber)
        column = (int)mInfo.mLines[lineNumber].mGlyphs.size();
      else
        column = 0;
      }
    }
  else {
    // move to previous column on same line
    --column;
    if (column > 0)
      while (column > 0 && isUtfSequence (mInfo.mLines[lineNumber].mGlyphs[column].mChar))
        --column;
    }

  mEdit.mState.mCursorPosition = sPosition (lineNumber, getCharacterColumn (lineNumber, column));

  if (mEdit.mState.mCursorPosition != position) {
    mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
    mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
    setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, eSelection::Normal);
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::moveRight() {

  if (mInfo.mLines.empty())
    return;

  sPosition position = mEdit.mState.mCursorPosition;

  int column = getCharacterIndex (mEdit.mState.mCursorPosition);
  int lineNumber = mEdit.mState.mCursorPosition.mLineNumber;
  if (column >= (int)mInfo.mLines [lineNumber].mGlyphs.size()) {
    // move to start of next line
    if (mEdit.mState.mCursorPosition.mLineNumber < (int)mInfo.mLines.size()-1) {
      mEdit.mState.mCursorPosition.mLineNumber =
        max (0, min ((int)mInfo.mLines.size() - 1, mEdit.mState.mCursorPosition.mLineNumber + 1));
      mEdit.mState.mCursorPosition.mColumn = 0;
      }
    else
      return;
    }
  else {
    // move to next columm on same line
    column += utf8CharLength (mInfo.mLines [lineNumber].mGlyphs[column].mChar);
    mEdit.mState.mCursorPosition = sPosition (lineNumber, getCharacterColumn (lineNumber, column));
    }

  if (mEdit.mState.mCursorPosition != position) {
    mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
    mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
    setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, eSelection::Normal);
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::moveHome() {

  sPosition position = mEdit.mState.mCursorPosition;

  setCursorPosition (sPosition (0,0));

  if (mEdit.mState.mCursorPosition != position) {
    mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
    mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
    setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd);
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::moveEnd() {

  sPosition position = mEdit.mState.mCursorPosition;

  setCursorPosition ({(int)mInfo.mLines.size()-1, 0});

  if (mEdit.mState.mCursorPosition != position) {
    mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
    mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
    setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd);
    ensureCursorVisible();
    }
  }
//}}}

// select
//{{{
void cTextEdit::selectAll() {
  setSelection (sPosition (0,0), sPosition ((int)mInfo.mLines.size(), 0));
  }
//}}}
//{{{
void cTextEdit::selectWordUnderCursor() {

  sPosition cursorPosition = getCursorPosition();
  setSelection (findWordStart (cursorPosition), findWordEnd (cursorPosition));
  }
//}}}

// cut and paste
//{{{
void cTextEdit::copy() {

  if (hasSelect())
    ImGui::SetClipboardText (getSelectedText().c_str());

  else if (!mInfo.mLines.empty()) {
    sLine& line = mInfo.mLines[getCursorPosition().mLineNumber];

    string str;
    for (auto& glyph : line.mGlyphs)
      str.push_back (glyph.mChar);

    ImGui::SetClipboardText (str.c_str());
    }
  }
//}}}
//{{{
void cTextEdit::cut() {

  if (isReadOnly())
    copy();

  else if (hasSelect()) {
    sUndo undo;
    undo.mBefore = mEdit.mState;
    undo.mRemoved = getSelectedText();
    undo.mRemovedStart = mEdit.mState.mSelectionStart;
    undo.mRemovedEnd = mEdit.mState.mSelectionEnd;

    copy();
    deleteSelection();

    undo.mAfter = mEdit.mState;
    addUndo (undo);
    }
  }
//}}}
//{{{
void cTextEdit::paste() {

  if (isReadOnly())
    return;

  if (hasClipboardText()) {
    sUndo undo;
    undo.mBefore = mEdit.mState;
    if (hasSelect()) {
      undo.mRemoved = getSelectedText();
      undo.mRemovedStart = mEdit.mState.mSelectionStart;
      undo.mRemovedEnd = mEdit.mState.mSelectionEnd;
      deleteSelection();
      }

    const char* clipText = ImGui::GetClipboardText();
    undo.mAdded = clipText;
    undo.mAddedStart = getCursorPosition();

    insertText (clipText);

    undo.mAddedEnd = getCursorPosition();
    undo.mAfter = mEdit.mState;
    addUndo (undo);
    }
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

// delete
//{{{
void cTextEdit::deleteIt() {

  if (mInfo.mLines.empty())
    return;

  sUndo undo;
  undo.mBefore = mEdit.mState;

  if (hasSelect()) {
    undo.mRemoved = getSelectedText();
    undo.mRemovedStart = mEdit.mState.mSelectionStart;
    undo.mRemovedEnd = mEdit.mState.mSelectionEnd;
    deleteSelection();
    }

  else {
    sPosition position = getCursorPosition();
    setCursorPosition (position);
    vector<sGlyph>& glyphs = mInfo.mLines[position.mLineNumber].mGlyphs;

    if (position.mColumn == getLineMaxColumn (position.mLineNumber)) {
      if (position.mLineNumber == (int)mInfo.mLines.size() - 1)
        return;

      undo.mRemoved = '\n';
      undo.mRemovedStart = undo.mRemovedEnd = getCursorPosition();
      advance (undo.mRemovedEnd);

      vector<sGlyph>& nextLineGlyphs = mInfo.mLines[position.mLineNumber + 1].mGlyphs;
      glyphs.insert (glyphs.end(), nextLineGlyphs.begin(), nextLineGlyphs.end());
      removeLine (position.mLineNumber + 1);
      }
    else {
      int cindex = getCharacterIndex (position);
      undo.mRemovedStart = undo.mRemovedEnd = getCursorPosition();
      undo.mRemovedEnd.mColumn++;
      undo.mRemoved = getText (undo.mRemovedStart, undo.mRemovedEnd);

      int d = utf8CharLength (glyphs[cindex].mChar);
      while (d-- > 0 && cindex < (int)glyphs.size())
        glyphs.erase (glyphs.begin() + cindex);
      }

    mInfo.mTextEdited = true;
    colorize (position.mLineNumber, 1);
    }

  undo.mAfter = mEdit.mState;
  addUndo (undo);
  }
//}}}
//{{{
void cTextEdit::backspace() {

  if (mInfo.mLines.empty())
    return;

  sUndo undo;
  undo.mBefore = mEdit.mState;

  if (hasSelect()) {
    //{{{  backspace delete selection
    undo.mRemoved = getSelectedText();
    undo.mRemovedStart = mEdit.mState.mSelectionStart;
    undo.mRemovedEnd = mEdit.mState.mSelectionEnd;

    deleteSelection();
    }
    //}}}

  else {
    sPosition position = getCursorPosition();
    setCursorPosition (position);
    if (mEdit.mState.mCursorPosition.mColumn == 0) {
      if (mEdit.mState.mCursorPosition.mLineNumber == 0)
        return;

      undo.mRemoved = '\n';
      undo.mRemovedStart = undo.mRemovedEnd = sPosition (position.mLineNumber - 1, getLineMaxColumn (position.mLineNumber - 1));
      advance (undo.mRemovedEnd);

      vector<sGlyph>& line = mInfo.mLines[mEdit.mState.mCursorPosition.mLineNumber].mGlyphs;
      vector<sGlyph>& prevLine = mInfo.mLines[mEdit.mState.mCursorPosition.mLineNumber - 1].mGlyphs;
      int prevSize = getLineMaxColumn (mEdit.mState.mCursorPosition.mLineNumber - 1);
      prevLine.insert (prevLine.end(), line.begin(), line.end());

      map<int,string> etmp;
      for (auto& marker : mOptions.mMarkers)
        etmp.insert (map<int,string>::value_type (
          marker.first - 1 == mEdit.mState.mCursorPosition.mLineNumber ? marker.first - 1 : marker.first, marker.second));
      mOptions.mMarkers = move (etmp);

      removeLine (mEdit.mState.mCursorPosition.mLineNumber);
      --mEdit.mState.mCursorPosition.mLineNumber;
      mEdit.mState.mCursorPosition.mColumn = prevSize;
      }

    else {
      vector<sGlyph>& glyphs = mInfo.mLines[mEdit.mState.mCursorPosition.mLineNumber].mGlyphs;
      int cindex = getCharacterIndex (position) - 1;
      int cend = cindex + 1;
      while ((cindex > 0) && isUtfSequence (glyphs[cindex].mChar))
        --cindex;

      //if (cindex > 0 && UTF8CharLength(line[cindex].mChar) > 1)
      //  --cindex;
      undo.mRemovedStart = undo.mRemovedEnd = getCursorPosition();
      --undo.mRemovedStart.mColumn;
      --mEdit.mState.mCursorPosition.mColumn;

      while ((cindex < (int)glyphs.size()) && (cend-- > cindex)) {
        undo.mRemoved += glyphs[cindex].mChar;
        glyphs.erase (glyphs.begin() + cindex);
        }
      }
    mInfo.mTextEdited = true;

    ensureCursorVisible();
    colorize (mEdit.mState.mCursorPosition.mLineNumber, 1);
    }

  undo.mAfter = mEdit.mState;
  addUndo (undo);
  }
//}}}
//{{{
void cTextEdit::deleteSelection() {

  assert(mEdit.mState.mSelectionEnd >= mEdit.mState.mSelectionStart);
  if (mEdit.mState.mSelectionEnd == mEdit.mState.mSelectionStart)
    return;

  deleteRange (mEdit.mState.mSelectionStart, mEdit.mState.mSelectionEnd);

  setSelection (mEdit.mState.mSelectionStart, mEdit.mState.mSelectionStart);
  setCursorPosition (mEdit.mState.mSelectionStart);

  colorize (mEdit.mState.mSelectionStart.mLineNumber, 1);
  }
//}}}

// insert
//{{{
void cTextEdit::enterCharacter (ImWchar ch, bool shift) {

  sUndo undo;
  undo.mBefore = mEdit.mState;

  if (hasSelect()) {
    if ((ch == '\t') && (mEdit.mState.mSelectionStart.mLineNumber != mEdit.mState.mSelectionEnd.mLineNumber)) {
      //{{{  tab insert into selection
      auto start = mEdit.mState.mSelectionStart;
      auto end = mEdit.mState.mSelectionEnd;
      auto originalEnd = end;

      if (start > end)
        swap (start, end);

      start.mColumn = 0;
      // end.mColumn = end.mGlyphs < mInfo.mLines.size() ? mInfo.mLines[end.mGlyphs].size() : 0;
      if (end.mColumn == 0 && end.mLineNumber > 0)
        --end.mLineNumber;
      if (end.mLineNumber >= (int)mInfo.mLines.size())
        end.mLineNumber = mInfo.mLines.empty() ? 0 : (int)mInfo.mLines.size() - 1;
      end.mColumn = getLineMaxColumn (end.mLineNumber);

      //if (end.mColumn >= getLineMaxColumn(end.mGlyphs))
      //  end.mColumn = getLineMaxColumn(end.mGlyphs) - 1;
      undo.mRemovedStart = start;
      undo.mRemovedEnd = end;
      undo.mRemoved = getText (start, end);

      bool modified = false;
      for (int i = start.mLineNumber; i <= end.mLineNumber; i++) {
        vector<sGlyph>& glyphs = mInfo.mLines[i].mGlyphs;
        if (shift) {
          if (!glyphs.empty()) {
            if (glyphs.front().mChar == '\t') {
              glyphs.erase (glyphs.begin());
              modified = true;
              }
            else {
              for (int j = 0; j < mInfo.mTabSize && !glyphs.empty() && glyphs.front().mChar == ' '; j++) {
                glyphs.erase (glyphs.begin());
                modified = true;
                }
              }
            }
          }
        else {
          glyphs.insert (glyphs.begin(), sGlyph ('\t', cTextEdit::ePalette::Background));
          modified = true;
          }
        }

      if (modified) {
        //{{{  not sure what this does yet
        start = sPosition (start.mLineNumber, getCharacterColumn (start.mLineNumber, 0));
        sPosition rangeEnd;
        if (originalEnd.mColumn != 0) {
          end = sPosition (end.mLineNumber, getLineMaxColumn (end.mLineNumber));
          rangeEnd = end;
          undo.mAdded = getText (start, end);
          }
        else {
          end = sPosition (originalEnd.mLineNumber, 0);
          rangeEnd = sPosition (end.mLineNumber - 1, getLineMaxColumn (end.mLineNumber - 1));
          undo.mAdded = getText (start, rangeEnd);
          }

        undo.mAddedStart = start;
        undo.mAddedEnd = rangeEnd;
        undo.mAfter = mEdit.mState;

        mEdit.mState.mSelectionStart = start;
        mEdit.mState.mSelectionEnd = end;
        addUndo (undo);
        mInfo.mTextEdited = true;

        ensureCursorVisible();
        }
        //}}}

      return;
      }
      //}}}
    else {
      //{{{  deleteSelection before insert
      undo.mRemoved = getSelectedText();
      undo.mRemovedStart = mEdit.mState.mSelectionStart;
      undo.mRemovedEnd = mEdit.mState.mSelectionEnd;
      deleteSelection();
      }
      //}}}
    }

  auto position = getCursorPosition();
  undo.mAddedStart = position;

  assert (!mInfo.mLines.empty());
  if (ch == '\n') {
    //{{{  enter carraigeReturn
    insertLine (position.mLineNumber + 1);
    vector<sGlyph>& glyphs = mInfo.mLines[position.mLineNumber].mGlyphs;
    vector<sGlyph>& newLine = mInfo.mLines[position.mLineNumber+1].mGlyphs;

    if (mOptions.mLanguage.mAutoIndentation)
      for (size_t it = 0; (it < glyphs.size()) && isascii (glyphs[it].mChar) && isblank (glyphs[it].mChar); ++it)
        newLine.push_back (glyphs[it]);

    const size_t whiteSpaceSize = newLine.size();

    auto cindex = getCharacterIndex (position);
    newLine.insert (newLine.end(), glyphs.begin() + cindex, glyphs.end());
    glyphs.erase (glyphs.begin() + cindex, glyphs.begin() + glyphs.size());

    setCursorPosition (sPosition (position.mLineNumber + 1,
                                  getCharacterColumn (position.mLineNumber + 1, (int)whiteSpaceSize)));
    undo.mAdded = (char)ch;
    }
    //}}}
  else {
    // enter char
    char buf[7];
    int e = imTextCharToUtf8 (buf, 7, ch);
    if (e > 0) {
      buf[e] = '\0';
      vector<sGlyph>& glyphs = mInfo.mLines[position.mLineNumber].mGlyphs;
      int cindex = getCharacterIndex (position);
      if (mOptions.mOverWrite && (cindex < (int)glyphs.size())) {
        auto d = utf8CharLength (glyphs[cindex].mChar);
        undo.mRemovedStart = mEdit.mState.mCursorPosition;
        undo.mRemovedEnd = sPosition (position.mLineNumber, getCharacterColumn (position.mLineNumber, cindex + d));
        while ((d-- > 0) && (cindex < (int)glyphs.size())) {
          undo.mRemoved += glyphs[cindex].mChar;
          glyphs.erase (glyphs.begin() + cindex);
          }
        }

      for (auto p = buf; *p != '\0'; p++, ++cindex)
        glyphs.insert (glyphs.begin() + cindex, sGlyph (*p, ePalette::Default));

      undo.mAdded = buf;
      setCursorPosition (sPosition (position.mLineNumber, getCharacterColumn (position.mLineNumber, cindex)));
      }
    else
      return;
    }

  mInfo.mTextEdited = true;

  undo.mAddedEnd = getCursorPosition();
  undo.mAfter = mEdit.mState;
  addUndo (undo);

  colorize (position.mLineNumber - 1, 3);
  ensureCursorVisible();
  }
//}}}

// fold
//{{{
void cTextEdit::openFold() {

  if (mOptions.mShowFolded)
    if (mInfo.mLines[mEdit.mState.mCursorPosition.mLineNumber].mFoldBegin)
      mInfo.mLines[mEdit.mState.mCursorPosition.mLineNumber].mFoldOpen = true;
  }
//}}}
//{{{
void cTextEdit::closeFold() {

  if (mOptions.mShowFolded) {
    int lineNumber = mEdit.mState.mCursorPosition.mLineNumber;
    sLine& line = mInfo.mLines[lineNumber];

    if (line.mFoldBegin && line.mFoldOpen) // if at open foldBegin, close it
      line.mFoldOpen = false;
    else {
      // search back for this fold's foldBegin and close it
      // - !!! need to skip foldEnd foldBegin pairs !!!
      while (--lineNumber >= 0) {
        line = mInfo.mLines[lineNumber];
        if (line.mFoldBegin && line.mFoldOpen) {
          line.mFoldOpen = false;
          // !!!! set cursor position to lineNumber !!!
          break;
          }
        }
      }
    }
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

  //io.FontGlobalScale = 1.f;
  drawTop (app);

  if (mOptions.mShowMonoSpace)
    ImGui::PushFont (app.getMonoFont());

  // new colours
  ImGui::PushStyleColor (ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4 (0xffefefef));
  ImGui::PushStyleColor (ImGuiCol_Text, mOptions.mPalette[(size_t)ePalette::Default]);

  ImGui::BeginChild (kChildScrollStr, ImVec2(0,0), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);

  // grab padding before it is zeroed in childWindow
  mContext.update (mOptions);
  ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, ImVec2(0,0));
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

  //ImGui::PushAllowKeyboardFocus (true);
  handleKeyboardInputs();
  //handleMouseInputs();

  colorizeInternal();

  if (mOptions.mShowFolded) {
    //{{{  draw folded
    mInfo.mFoldLines.clear();
    drawFold (0, true, true);
    }
    //}}}
  else {
    // clipper begin
    ImGuiListClipper clipper;
    clipper.Begin ((int)mInfo.mLines.size(), mContext.mLineHeight);
    clipper.Step();

    // clipper iterate
    for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++)
      drawLine (lineNumber, 0);

    // clipper end
    clipper.Step();
    clipper.End();
    }

  //{{{  highlight idents, preproc, rewrite as curso coords not mouse coords
  //if (ImGui::IsMousePosValid()) {
    //string word = getWordAt (screenToPosition (ImGui::GetMousePos()));
    //if (!word.empty()) {
      //{{{  draw tooltip for idents,preProcs
      //auto identIt = mOptions.mLanguage.mIdents.find (word);
      //if (identIt != mOptions.mLanguage.mIdents.end()) {
        //ImGui::BeginTooltip();
        //ImGui::TextUnformatted (identIt->second.mDeclaration.c_str());
        //ImGui::EndTooltip();
        //}

      //else {
        //auto preProcIdentIt = mOptions.mLanguage.mPreprocIdents.find (word);
        //if (preProcIdentIt != mOptions.mLanguage.mPreprocIdents.end()) {
          //ImGui::BeginTooltip();
          //ImGui::TextUnformatted (preProcIdentIt->second.mDeclaration.c_str());
          //ImGui::EndTooltip();
          //}
        //}
      //}
      //}}}
    //}
  //}}}

  ImGui::PopStyleVar (2);
  //ImGui::PopAllowKeyboardFocus();

  ImGui::EndChild();

  ImGui::PopStyleColor(2);

  if (mOptions.mShowMonoSpace)
    ImGui::PopFont();
  }
//}}}

// private:
//{{{  gets
//{{{
bool cTextEdit::isOnWordBoundary (const sPosition& position) const {

  if (position.mLineNumber >= (int)mInfo.mLines.size() || position.mColumn == 0)
    return true;

  const vector<sGlyph>& glyphs = mInfo.mLines[position.mLineNumber].mGlyphs;
  int characterIndex = getCharacterIndex (position);
  if (characterIndex >= (int)glyphs.size())
    return true;

  return glyphs[characterIndex].mColorIndex != glyphs[size_t(characterIndex - 1)].mColorIndex;
  //return isspace (glyphs[cindex].mChar) != isspace (glyphs[cindex - 1].mChar);
  }
//}}}

//{{{
int cTextEdit::getCharacterIndex (const sPosition& position) const {

  if (position.mLineNumber >= static_cast<int>(mInfo.mLines.size())) {
    cLog::log (LOGERROR, "cTextEdit::getCharacterIndex - lineNumber too big");
    return 0;
    }

  const vector <sGlyph>& glyphs = mInfo.mLines[position.mLineNumber].mGlyphs;

  int column = 0;
  int i = 0;
  for (; (i < static_cast<int>(glyphs.size())) && (column < position.mColumn);) {
    if (glyphs[i].mChar == '\t')
      column = (column / mInfo.mTabSize) * mInfo.mTabSize + mInfo.mTabSize;
    else
      ++column;
    i += utf8CharLength (glyphs[i].mChar);
    }

  return i;
  }
//}}}
//{{{
int cTextEdit::getCharacterColumn (int lineNumber, int index) const {
// handle tabs

  if (lineNumber >= static_cast<int>(mInfo.mLines.size()))
    return 0;

  const vector<sGlyph>& glyphs = mInfo.mLines[lineNumber].mGlyphs;

  int column = 0;
  int i = 0;
  while ((i < index) && i < (static_cast<int>(glyphs.size()))) {
    uint8_t ch = glyphs[i].mChar;
    i += utf8CharLength (ch);
    if (ch == '\t')
      column = ((column / mInfo.mTabSize) * mInfo.mTabSize) + mInfo.mTabSize;
    else
      column++;
    }

  return column;
  }
//}}}

//{{{
int cTextEdit::getLineNumChars (int row) const {

  if (row >= static_cast<int>(mInfo.mLines.size()))
    return 0;

  const vector <sGlyph>& glyphs = mInfo.mLines[row].mGlyphs;
  int numChars = 0;
  for (size_t i = 0; i < glyphs.size(); numChars++)
    i += utf8CharLength (glyphs[i].mChar);

  return numChars;
  }
//}}}
//{{{
int cTextEdit::getLineMaxColumn (int row) const {

  if (row >= static_cast<int>(mInfo.mLines.size()))
    return 0;

  const vector <sGlyph>& glyphs = mInfo.mLines[row].mGlyphs;
  int column = 0;
  for (size_t i = 0; i < glyphs.size(); ) {
    uint8_t ch = glyphs[i].mChar;
    if (ch == '\t')
      column = ((column / mInfo.mTabSize) * mInfo.mTabSize) + mInfo.mTabSize;
    else
      column++;
    i += utf8CharLength (ch);
    }

  return column;
  }
//}}}

//{{{
string cTextEdit::getText (const sPosition& startPosition, const sPosition& endPosition) const {
// get text as string with lineFeed line breaks

  int lstart = startPosition.mLineNumber;
  int lend = endPosition.mLineNumber;

  int istart = getCharacterIndex (startPosition);
  int iend = getCharacterIndex (endPosition);

  size_t s = 0;
  for (size_t i = lstart; i < (size_t)lend; i++)
    s += mInfo.mLines[i].mGlyphs.size();

  string result;
  result.reserve (s + s / 8);

  while (istart < iend || lstart < lend) {
    if (lstart >= (int)mInfo.mLines.size())
      break;

    if (istart < (int)mInfo.mLines[lstart].mGlyphs.size()) {
      result += mInfo.mLines[lstart].mGlyphs[istart].mChar;
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
string cTextEdit::getCurrentLineText() const {

  int lineLength = getLineMaxColumn (mEdit.mState.mCursorPosition.mLineNumber);
  return getText (sPosition (mEdit.mState.mCursorPosition.mLineNumber, 0),
                  sPosition (mEdit.mState.mCursorPosition.mLineNumber, lineLength));
  }
//}}}
//{{{
ImU32 cTextEdit::getGlyphColor (const sGlyph& glyph) const {

  if (glyph.mComment)
    return mOptions.mPalette[(size_t)ePalette::Comment];

  if (glyph.mMultiLineComment)
    return mOptions.mPalette[(size_t)ePalette::MultiLineComment];

  auto const color = mOptions.mPalette[(size_t)glyph.mColorIndex];

  if (glyph.mPreProc) {
    const auto ppcolor = mOptions.mPalette[(size_t)ePalette::Preprocessor];
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
string cTextEdit::getWordAt (const sPosition& position) const {

  string r;
  for (int i = getCharacterIndex (findWordStart (position)); i < getCharacterIndex (findWordEnd (position)); ++i)
    r.push_back (mInfo.mLines[position.mLineNumber].mGlyphs[i].mChar);

  return r;
  }
//}}}
//{{{  *
string cTextEdit::getWordUnderCursor() const {
  return getWordAt (getCursorPosition());
  }
//}}}

//{{{
float cTextEdit::getTextWidth (const sPosition& position) {
// get textWidth to position

  const vector<sGlyph>& glyphs = mInfo.mLines[position.mLineNumber].mGlyphs;

  float distance = 0.f;
  int colIndex = getCharacterIndex (position);
  for (size_t i = 0; (i < glyphs.size()) && ((int)i < colIndex);) {
    if (glyphs[i].mChar == '\t') {
      // tab
      distance = tabEndPos (distance);
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
int cTextEdit::getPageNumLines() const {
  float height = ImGui::GetWindowHeight() - 20.f;
  return (int)floor (height / mContext.mLineHeight);
  }
//}}}

//{{{
int cTextEdit::getMaxLineIndex() const {

  if (mOptions.mShowFolded)
    return static_cast<int>(mInfo.mFoldLines.size()-1);
  else
    return static_cast<int>(mInfo.mLines.size()-1);
  }
//}}}
//}}}
//{{{  utils
//{{{
void cTextEdit::clickCursor (int lineNumber, float xPos, bool selectWord) {

  mEdit.mState.mCursorPosition = xPosToPosition (lineNumber, xPos);

  mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
  mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;

  if (selectWord)
    mEdit.mSelection = eSelection::Word;
  else
    mEdit.mSelection = eSelection::Normal;
  setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, mEdit.mSelection);

  mLastClickTime = static_cast<float>(ImGui::GetTime());
  }
//}}}

//{{{
float cTextEdit::tabEndPos (float xPos) {
// return tabEnd xPos of tab containing xPos

  float tabWidthPixels = mInfo.mTabSize * mContext.mGlyphWidth;
  return (1.f + floor ((1.f + xPos) / tabWidthPixels)) * tabWidthPixels;
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::xPosToPosition (int lineNumber, float xPos) {

  int column = 0;
  if ((lineNumber >= 0) && (lineNumber < (int)mInfo.mLines.size())) {
    const vector<sGlyph>& glyphs = mInfo.mLines[lineNumber].mGlyphs;

    float columnX = 0.f;

    size_t glyphIndex = 0;
    while (glyphIndex < glyphs.size()) {
      float columnWidth = 0.f;
      if (glyphs[glyphIndex].mChar == '\t') {
        float oldX = columnX;
        float endTabX = tabEndPos (columnX);
        columnWidth = endTabX - oldX;
        if (mContext.mTextBegin + columnX + (columnWidth/2.f) > xPos)
          break;
        columnX = endTabX;
        column = ((column / mInfo.mTabSize) * mInfo.mTabSize) + mInfo.mTabSize;
        glyphIndex++;
        }

      else {
        array <char,7> str;
        int length = utf8CharLength (glyphs[glyphIndex].mChar);
        size_t i = 0;
        while ((i < str.max_size()-1) && (length-- > 0))
          str[i++] = glyphs[glyphIndex++].mChar;
        columnWidth = mContext.measureText (str.data(), str.data()+i);
        if ((mContext.mTextBegin + columnX + (columnWidth/2.f)) > xPos)
          break;

        columnX += columnWidth;
        column++;
        }
      }
    }

  return sanitizePosition (sPosition (lineNumber, column));
  }
//}}}

//{{{
void cTextEdit::advance (sPosition& position) const {

  if (position.mLineNumber < (int)mInfo.mLines.size()) {
    const vector<sGlyph>& glyphs = mInfo.mLines[position.mLineNumber].mGlyphs;
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
cTextEdit::sPosition cTextEdit::sanitizePosition (const sPosition& position) const {

  if (position.mLineNumber >= static_cast<int>(mInfo.mLines.size())) {
    if (mInfo.mLines.empty())
      return sPosition (0,0);
    else
      return sPosition (static_cast<int>(mInfo.mLines.size())-1,
                        getLineMaxColumn (static_cast<int>(mInfo.mLines.size()-1)));
    }
  else
    return sPosition (position.mLineNumber,
                      mInfo.mLines.empty() ? 0 : min (position.mColumn, getLineMaxColumn (position.mLineNumber)));
  }
//}}}

// lineIndex
//{{{
int cTextEdit::lineIndexToNumber (int lineIndex) const {

  if (!mOptions.mShowFolded)
    return lineIndex;

  if ((lineIndex >= 0) && (lineIndex < static_cast<int>(mInfo.mFoldLines.size())))
    return mInfo.mFoldLines[lineIndex];

  return -1;
  }
//}}}
//{{{
int cTextEdit::lineNumberToIndex (int lineNumber) const {

  if (!mOptions.mShowFolded) // lineIndex is  lineNumber
    return lineNumber;

  if (mInfo.mFoldLines.empty()) { // no entry for that index
    cLog::log (LOGERROR, fmt::format ("lineNumberToIndex {} mFoldLines empty", lineNumber));
    return -1;
    }

  auto it = find (mInfo.mFoldLines.begin(), mInfo.mFoldLines.end(), lineNumber);
  if (it == mInfo.mFoldLines.end()) {
    cLog::log (LOGERROR, fmt::format ("lineNumberToIndex {} not found", lineNumber));
    return -1;
    }
  else
    return int(it - mInfo.mFoldLines.begin());
  }
//}}}

// find
//{{{
cTextEdit::sPosition cTextEdit::findWordStart (const sPosition& from) const {

  sPosition at = from;
  if (at.mLineNumber >= (int)mInfo.mLines.size())
    return at;

  const vector<sGlyph>& glyphs = mInfo.mLines[at.mLineNumber].mGlyphs;
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
cTextEdit::sPosition cTextEdit::findWordEnd (const sPosition& from) const {

  sPosition at = from;
  if (at.mLineNumber >= (int)mInfo.mLines.size())
    return at;

  const vector<sGlyph>& glyphs = mInfo.mLines[at.mLineNumber].mGlyphs;
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
cTextEdit::sPosition cTextEdit::findNextWord (const sPosition& from) const {

  sPosition at = from;
  if (at.mLineNumber >= (int)mInfo.mLines.size())
    return at;

  // skip to the next non-word character
  bool isword = false;
  bool skip = false;

  int cindex = getCharacterIndex (from);
  if (cindex < (int)mInfo.mLines[at.mLineNumber].mGlyphs.size()) {
    const vector<sGlyph>& glyphs = mInfo.mLines[at.mLineNumber].mGlyphs;
    isword = isalnum (glyphs[cindex].mChar);
    skip = isword;
    }

  while (!isword || skip) {
    if (at.mLineNumber >= (int)mInfo.mLines.size()) {
      int l = max(0, (int)mInfo.mLines.size() - 1);
      return sPosition (l, getLineMaxColumn(l));
      }

    const vector<sGlyph>& glyphs = mInfo.mLines[at.mLineNumber].mGlyphs;
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
void cTextEdit::moveUp (int amount) {

  if (mInfo.mLines.empty())
    return;

  sPosition position = mEdit.mState.mCursorPosition;

  int lineNumber = mEdit.mState.mCursorPosition.mLineNumber;
  if (lineNumber == 0)
    return;

  int lineIndex = lineNumberToIndex (lineNumber);
  lineIndex = max (0, lineIndex - amount);
  lineNumber = lineIndexToNumber (lineIndex);

  mEdit.mState.mCursorPosition.mLineNumber = lineNumber;

  if (mEdit.mState.mCursorPosition != position) {
    mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
    mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
    setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd);

    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::moveDown (int amount) {

  if (mInfo.mLines.empty())
    return;

  sPosition position = mEdit.mState.mCursorPosition;

  int lineNumber = mEdit.mState.mCursorPosition.mLineNumber;

  int lineIndex = lineNumberToIndex (lineNumber);
  lineIndex = min (getMaxLineIndex(), lineIndex + amount);
  lineNumber = lineIndexToNumber (lineIndex);

  mEdit.mState.mCursorPosition.mLineNumber = lineNumber;

  if (mEdit.mState.mCursorPosition != position) {
    mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
    mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
    setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd);

    ensureCursorVisible();
    }
  }
//}}}

// insert
//{{{
vector<cTextEdit::sGlyph>& cTextEdit::insertLine (int index) {

  sLine& result = *mInfo.mLines.insert (mInfo.mLines.begin() + index, vector<sGlyph>());

  map<int,string> etmp;
  for (auto& marker : mOptions.mMarkers)
    etmp.insert (map<int,string>::value_type (marker.first >= index ? marker.first + 1 : marker.first, marker.second));
  mOptions.mMarkers = move (etmp);

  return result.mGlyphs;
  }
//}}}
//{{{
int cTextEdit::insertTextAt (sPosition& where, const char* value) {

  int cindex = getCharacterIndex (where);
  int totalLines = 0;

  while (*value != '\0') {
    assert(!mInfo.mLines.empty());

    if (*value == '\r') {
      // skip
      ++value;
      }
    else if (*value == '\n') {
      if (cindex < (int)mInfo.mLines[where.mLineNumber].mGlyphs.size()) {
        vector <sGlyph>& newLine = insertLine (where.mLineNumber + 1);
        vector <sGlyph>& line = mInfo.mLines[where.mLineNumber].mGlyphs;
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
      vector <sGlyph>& glyphs = mInfo.mLines[where.mLineNumber].mGlyphs;
      auto d = utf8CharLength (*value);
      while (d-- > 0 && *value != '\0')
        glyphs.insert (glyphs.begin() + cindex++, sGlyph (*value++, ePalette::Default));
      ++where.mColumn;
      }

    mInfo.mTextEdited = true;
    }

  return totalLines;
  }
//}}}
//{{{
void cTextEdit::insertText (const char* value) {

  if (value == nullptr)
    return;

  sPosition position = getCursorPosition();
  sPosition startPosition = min (position, mEdit.mState.mSelectionStart);

  int totalLines = position.mLineNumber - startPosition.mLineNumber;
  totalLines += insertTextAt (position, value);

  setSelection (position, position);
  setCursorPosition (position);

  colorize (startPosition.mLineNumber - 1, totalLines + 2);
  }
//}}}

// delete
//{{{
void cTextEdit::removeLine (int startPosition, int endPosition) {

  assert (endPosition >= startPosition);
  assert (int(mInfo.mLines.size()) > endPosition - startPosition);

  map<int,string> etmp;
  for (auto& marker : mOptions.mMarkers) {
    map<int,string>::value_type e((marker.first >= startPosition) ? marker.first - 1 : marker.first, marker.second);
    if ((e.first >= startPosition) && (e.first <= endPosition))
      continue;
    etmp.insert (e);
    }
  mOptions.mMarkers = move (etmp);

  mInfo.mLines.erase (mInfo.mLines.begin() + startPosition, mInfo.mLines.begin() + endPosition);
  assert (!mInfo.mLines.empty());

  mInfo.mTextEdited = true;
  }
//}}}
//{{{
void cTextEdit::removeLine (int index) {

  assert (mInfo.mLines.size() > 1);

  map<int,string> etmp;
  for (auto& marker : mOptions.mMarkers) {
    map<int,string>::value_type e(marker.first > index ? marker.first - 1 : marker.first, marker.second);
    if (e.first - 1 == index)
      continue;
    etmp.insert (e);
    }
  mOptions.mMarkers = move (etmp);

  mInfo.mLines.erase (mInfo.mLines.begin() + index);
  assert (!mInfo.mLines.empty());

  mInfo.mTextEdited = true;
  }
//}}}
//{{{
void cTextEdit::deleteRange (const sPosition& startPosition, const sPosition& endPosition) {

  assert (endPosition >= startPosition);

  //printf("D(%d.%d)-(%d.%d)\n", startPosition.mGlyphs, startPosition.mColumn, endPosition.mGlyphs, endPosition.mColumn);

  if (endPosition == startPosition)
    return;

  auto start = getCharacterIndex (startPosition);
  auto end = getCharacterIndex (endPosition);
  if (startPosition.mLineNumber == endPosition.mLineNumber) {
    vector<sGlyph>& glyphs = mInfo.mLines[startPosition.mLineNumber].mGlyphs;
    int  n = getLineMaxColumn (startPosition.mLineNumber);
    if (endPosition.mColumn >= n)
      glyphs.erase (glyphs.begin() + start, glyphs.end());
    else
      glyphs.erase (glyphs.begin() + start, glyphs.begin() + end);
    }

  else {
    vector<sGlyph>& firstLine = mInfo.mLines[startPosition.mLineNumber].mGlyphs;
    vector<sGlyph>& lastLine = mInfo.mLines[endPosition.mLineNumber].mGlyphs;

    firstLine.erase (firstLine.begin() + start, firstLine.end());
    lastLine.erase (lastLine.begin(), lastLine.begin() + end);

    if (startPosition.mLineNumber < endPosition.mLineNumber)
      firstLine.insert (firstLine.end(), lastLine.begin(), lastLine.end());

    if (startPosition.mLineNumber < endPosition.mLineNumber)
      removeLine (startPosition.mLineNumber + 1, endPosition.mLineNumber + 1);
    }

  mInfo.mTextEdited = true;
  }
//}}}

// undo
//{{{
void cTextEdit::addUndo (sUndo& value) {

  //printf("AddUndo: (@%d.%d) +\'%s' [%d.%d .. %d.%d], -\'%s', [%d.%d .. %d.%d] (@%d.%d)\n",
  //  value.mBefore.mCursorPosition.mGlyphs, value.mBefore.mCursorPosition.mColumn,
  //  value.mAdded.c_str(), value.mAddedStart.mGlyphs, value.mAddedStart.mColumn, value.mAddedEnd.mGlyphs, value.mAddedEnd.mColumn,
  //  value.mRemoved.c_str(), value.mRemovedStart.mGlyphs, value.mRemovedStart.mColumn, value.mRemovedEnd.mGlyphs, value.mRemovedEnd.mColumn,
  //  value.mAfter.mCursorPosition.mGlyphs, value.mAfter.mCursorPosition.mColumn
  //  );

  mUndoList.mBuffer.resize ((size_t)(mUndoList.mIndex + 1));
  mUndoList.mBuffer.back() = value;
  ++mUndoList.mIndex;
  }
//}}}

// colorize
//{{{
void cTextEdit::colorize (int fromLine, int lines) {

  int toLine = lines == -1 ? (int)mInfo.mLines.size() : min((int)mInfo.mLines.size(), fromLine + lines);

  mEdit.mColorRangeMin = min (mEdit.mColorRangeMin, fromLine);
  mEdit.mColorRangeMax = max (mEdit.mColorRangeMax, toLine);
  mEdit.mColorRangeMin = max (0, mEdit.mColorRangeMin);
  mEdit.mColorRangeMax = max (mEdit.mColorRangeMin, mEdit.mColorRangeMax);

  mOptions.mCheckComments = true;
  }
//}}}
//{{{
void cTextEdit::colorizeRange (int fromLine, int toLine) {

  if (mInfo.mLines.empty() || fromLine >= toLine)
    return;

  string buffer;
  cmatch results;
  string id;

  int endLine = max(0, min((int)mInfo.mLines.size(), toLine));
  for (int i = fromLine; i < endLine; ++i) {
    vector<sGlyph>& glyphs = mInfo.mLines[i].mGlyphs;
    if (glyphs.empty())
      continue;

    buffer.resize (glyphs.size());
    for (size_t j = 0; j < glyphs.size(); ++j) {
      sGlyph& col = glyphs[j];
      buffer[j] = col.mChar;
      col.mColorIndex = ePalette::Default;
      }

    const char* bufferBegin = &buffer.front();
    const char* bufferEnd = bufferBegin + buffer.size();
    const char* lastChar = bufferEnd;
    for (auto first = bufferBegin; first != lastChar; ) {
      const char* token_begin = nullptr;
      const char* token_end = nullptr;
      ePalette token_color = ePalette::Default;

      bool hasTokenizeResult = false;
      if (mOptions.mLanguage.mTokenize != nullptr) {
        if (mOptions.mLanguage.mTokenize (first, lastChar, token_begin, token_end, token_color))
          hasTokenizeResult = true;
        }

      if (hasTokenizeResult == false) {
        //printf("using regex for %.*s\n", first + 10 < last ? 10 : int(last - first), first);
        for (auto& p : mOptions.mRegexList) {
          if (regex_search (first, lastChar, results, p.first, regex_constants::match_continuous)) {
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
          if (!mOptions.mLanguage.mCaseSensitive)
            transform (id.begin(), id.end(), id.begin(),
                       [](uint8_t ch) { return static_cast<uint8_t>(std::toupper (ch)); });

          if (!glyphs[first - bufferBegin].mPreProc) {
            if (mOptions.mLanguage.mKeywords.count (id) != 0)
              token_color = ePalette::Keyword;
            else if (mOptions.mLanguage.mIdents.count (id) != 0)
              token_color = ePalette::KnownIdent;
            else if (mOptions.mLanguage.mPreprocIdents.count (id) != 0)
              token_color = ePalette::PreprocIdent;
            }
          else if (mOptions.mLanguage.mPreprocIdents.count (id) != 0)
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
void cTextEdit::colorizeInternal() {

  if (mInfo.mLines.empty())
    return;

  if (mOptions.mCheckComments) {
    int endLine = (int)mInfo.mLines.size();
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
      vector<sGlyph>& line = mInfo.mLines[currentLine].mGlyphs;
      if (currentIndex == 0 && !concatenate) {
        withinSingleLineComment = false;
        withinPreproc = false;
        firstChar = true;
        }
      concatenate = false;

      if (!line.empty()) {
        sGlyph& g = line[currentIndex];
        uint8_t c = g.mChar;
        if (c != mOptions.mLanguage.mPreprocChar && !isspace(c))
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
          if (firstChar && c == mOptions.mLanguage.mPreprocChar)
            withinPreproc = true;

          if (c == '\"') {
            withinString = true;
            line[currentIndex].mMultiLineComment = inComment;
            }
          else {
            auto pred = [](const char& a, const sGlyph& b) { return a == b.mChar; };
            auto from = line.begin() + currentIndex;
            string& startString = mOptions.mLanguage.mCommentStart;
            string& singleStartString = mOptions.mLanguage.mSingleLineComment;

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

            string& endString = mOptions.mLanguage.mCommentEnd;
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
    mOptions.mCheckComments = false;
    }

  if (mEdit.mColorRangeMin < mEdit.mColorRangeMax) {
    const int increment = (mOptions.mLanguage.mTokenize == nullptr) ? 10 : 10000;
    const int to = min(mEdit.mColorRangeMin + increment, mEdit.mColorRangeMax);
    colorizeRange (mEdit.mColorRangeMin, to);
    mEdit.mColorRangeMin = to;

    if (mEdit.mColorRangeMax == mEdit.mColorRangeMin) {
      mEdit.mColorRangeMin = numeric_limits<int>::max();
      mEdit.mColorRangeMax = 0;
      }
    return;
    }
  }
//}}}
//}}}
//{{{
void cTextEdit::ensureCursorVisible() {

  ImGui::SetWindowFocus();

  sPosition position = getCursorPosition();
  int lineIndex = lineNumberToIndex (position.mLineNumber);

  int topIndex = static_cast<int>(floor (ImGui::GetScrollY() / mContext.mLineHeight));
  int botIndex = min (getMaxLineIndex(),
                     static_cast<int>(floor ((ImGui::GetScrollY() + ImGui::GetWindowHeight()) / mContext.mLineHeight)));
  if (lineIndex <= topIndex+1) {
    cLog::log (LOGINFO, fmt::format ("top:{} line:{}", topIndex, lineIndex));
    //ImGui::SetScrollY (max (0.f, (lineIndex - 1) * mContext.mLineHeight));
    }
  else if (lineIndex >= botIndex-7) {
    cLog::log (LOGINFO, fmt::format ("bot:{} line:{}", botIndex, lineIndex));
    //ImGui::SetScrollY (max (0.f, (lineIndex+7) * mContext.mLineHeight) - ImGui::GetWindowHeight());
    }
  else
    cLog::log (LOGINFO, fmt::format ("top:{} bot:{} line:{}", topIndex, botIndex, lineIndex));

  //{{{  left right
  //float textWidth = getTextWidth (position);
  //int leftColumn = (int)floor(ImGui::GetScrollX() / mContext.mGlyphWidth);
  //int rightColumn = (int)ceil((ImGui::GetScrollX() + ImGui::GetWindowWidth()) / mContext.mGlyphWidth);
  //if (mGlyphsBegin + textWidth < leftColumn + 4) {
    //cLog::log (LOGINFO, "left");
    //ImGui::SetScrollX (max (0.f, mGlyphsBegin + textWidth - 4));
    //}
  //else if (mGlyphsBegin + textWidth > rightColumn - 4) {
    //cLog::log (LOGINFO, "right");
    //ImGui::SetScrollX (max (0.f, mGlyphsBegin + textWidth + 4 - ImGui::GetWindowWidth()));
    //}
  //}}}
  }
//}}}

// fold
//{{{
void cTextEdit::parseFolds() {
// parse beginFold and endFold markers, set simple flags

  for (auto& line : mInfo.mLines) {
    string text;
    for (auto& glyph : line.mGlyphs)
      text += glyph.mChar;

    // look for foldBegin text
    size_t foldBeginIndent = text.find (mOptions.mLanguage.mFoldBeginMarker);
    line.mFoldBegin = (foldBeginIndent != string::npos);

    if (line.mFoldBegin) {
      // found foldBegin text, find ident
      line.mIndent = static_cast<uint16_t>(foldBeginIndent);
      // has text after the foldBeginMarker
      line.mHasComment = (text.size() != (foldBeginIndent + mOptions.mLanguage.mFoldBeginMarker.size()));
      mInfo.mHasFolds = true;
      }
    else {
      // normal line, find indent, find comment
      size_t indent = text.find_first_not_of (' ');
      if (indent != string::npos)
        line.mIndent = static_cast<uint16_t>(indent);
      else
        line.mIndent = 0;

      // has "//" style comment as first text in line
      line.mHasComment = (text.find (mOptions.mLanguage.mSingleLineComment, indent) != string::npos);
      }

    // look for foldEnd text
    size_t foldEndIndent = text.find (mOptions.mLanguage.mFoldEndMarker);
    line.mFoldEnd = (foldEndIndent != string::npos);

    // init fields set by updateFolds
    line.mFoldOpen = false;
    line.mFoldTitleLineNumber = -1;
    }

  //mOptions.mShowFolded = mInfo.mHasFolds;
  }
//}}}

//{{{
void cTextEdit::handleMouseInputs() {

  ImGuiIO& io = ImGui::GetIO();
  bool shift = io.KeyShift;
  //bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

  //if (ImGui::IsWindowHovered()) {
    if (!shift && !alt) {
      //bool leftSingleClick = ImGui::IsMouseClicked (0);
      //bool righttSingleClick = ImGui::IsMouseClicked (1);
      //bool leftDoubleClick = ImGui::IsMouseDoubleClicked (0);
      //bool leftTripleClick = leftSingleClick &&
      //                       !leftDoubleClick &&
       //                      ((mLastClickTime != -1.f) && (ImGui::GetTime() - mLastClickTime) < io.MouseDoubleClickTime);
      //if (righttSingleClick) {
        //{{{  right mouse right singleClick
        //if (mOptions.mShowFolded) {
          //// test cursor position
          //sPosition position = screenToPosition (ImGui::GetMousePos());
          //if (mInfo.mLines[position.mLineNumber].mFoldBegin) {
            //// set cursor position
            //mEdit.mState.mCursorPosition = position;
            //mEdit.mInteractiveStart = position;
            //mEdit.mInteractiveEnd = position;

            //// open fold
            //mInfo.mLines[position.mLineNumber].mFoldOpen ^= true;
            //}
          //}

        //mLastClickTime = static_cast<float>(ImGui::GetTime());
        //}
        //}}}
      //else if (leftTripleClick) {
        //{{{  left mouse tripleClick
        //if (!ctrl) {
          //mEdit.mState.mCursorPosition = screenToPosition (ImGui::GetMousePos());
          //mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
          //mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;

          //mEdit.mSelection = eSelection::Line;
          //setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, mEdit.mSelection);
          //}

        //mLastClickTime = -1.f;
        //}
        //}}}
      //else if (leftDoubleClick) {
        //{{{  left mouse doubleClick
        //if (!ctrl) {
          //mEdit.mState.mCursorPosition = screenToPosition (ImGui::GetMousePos());
          //mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
          //mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;

          //if (mEdit.mSelection == eSelection::Line)
            //mEdit.mSelection = eSelection::Normal;
          //else
            //mEdit.mSelection = eSelection::Word;
          //setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, mEdit.mSelection);
          //}

        //mLastClickTime = static_cast<float>(ImGui::GetTime());
        //}
        //}}}
      //else if (leftSingleClick) {
        //clickCursor (ctrl);
      //  }
      //else if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0)) {
        //{{{  left mouse button dragging (=> update selection)
        //io.WantCaptureMouse = true;

        //mEdit.mState.mCursorPosition = screenToPosition (ImGui::GetMousePos());
        //mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
        //setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, mEdit.mSelection);
        //}
        //}}}
      }
  // }
  }
//}}}
//{{{
void cTextEdit::handleKeyboardInputs() {

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
  //constexpr int kNumpad0 = 0x140;
  constexpr int kNumpad1 = 0x141;
  //constexpr int kNumpad2 = 0x142;
  constexpr int kNumpad3 = 0x143;
  //constexpr int kNumpad4 = 0x144;
  //constexpr int kNumpad5 = 0x145;
  //constexpr int kNumpad6 = 0x146;
  //constexpr int kNumpad7 = 0x147;
  //constexpr int kNumpad8 = 0x148;
  //constexpr int kNumpad9 = 0x149;
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
     };

  //if (!ImGui::IsWindowFocused())
  // return;
  if (ImGui::IsWindowHovered())
    ImGui::SetMouseCursor (ImGuiMouseCursor_TextInput);

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

//{{{
void cTextEdit::drawTop (cApp& app) {

  // lineNumber button
  if (toggleButton ("line", mOptions.mShowLineNumbers))
    toggleShowLineNumbers();
  if (mOptions.mShowLineNumbers)
    //{{{  debug button
    if (mOptions.mShowLineNumbers) {
      ImGui::SameLine();
      if (toggleButton ("debug", mOptions.mShowDebug))
        toggleShowDebug();
      }
    //}}}

  if (mInfo.mHasFolds) {
    //{{{  folded button
    ImGui::SameLine();
    if (toggleButton ("folded", isShowFolds()))
      toggleShowFolded();
    }
    //}}}

  ImGui::SameLine();
  if (toggleButton ("mono", mOptions.mShowMonoSpace))
    toggleShowMonoSpace();
  ImGui::SameLine();
  if (toggleButton ("space", mOptions.mShowWhiteSpace))
    toggleShowWhiteSpace();

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

  // readOnly button
  ImGui::SameLine();
  if (toggleButton ("readOnly", isReadOnly()))
    toggleReadOnly();

  // overwrite button
  ImGui::SameLine();
  if (toggleButton ("insert", !mOptions.mOverWrite))
    toggleOverWrite();

  // fontSize button
  ImGui::SameLine();
  ImGui::SetNextItemWidth (3 * mContext.mFontAtlasSize);
  ImGui::DragInt ("##fontSize", &mOptions.mFontSize, 0.2f, mOptions.mMinFontSize, mOptions.mMaxFontSize, "%d");

  // info text
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:{} {}", getCursorPosition().mColumn+1, getCursorPosition().mLineNumber+1,
                                           getTextNumLines(), getLanguage().mName).c_str());

  // vsync button
  ImGui::SameLine();
  if (toggleButton ("vSync", app.getPlatform().getVsync()))
    app.getPlatform().toggleVsync();

  // debug text
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:vert:triangle {}:fps", ImGui::GetIO().MetricsRenderVertices,
                                                          ImGui::GetIO().MetricsRenderIndices/3,
                                                          static_cast<int>(ImGui::GetIO().Framerate)).c_str());
  }
//}}}
//{{{
float cTextEdit::drawGlyphs (ImVec2 pos, const vector <sGlyph>& glyphs, bool forceColor, ImU32 forcedColor) {

  if (glyphs.empty())
    return 0.f;

  // beginPos to measure textWidth on return
  float beginPosX = pos.x;

  // init
  array <char,256> str;

  size_t i = 0;
  ImU32 strColor = forceColor ? forcedColor : getGlyphColor (glyphs[0]);
  for (auto& glyph : glyphs) {
    ImU32 color = forceColor ? forcedColor : getGlyphColor (glyph);
    if ((i > 0) && (i < str.max_size()) &&
        ((color != strColor) || (glyph.mChar == '\t') || (glyph.mChar == ' '))) {
      // draw colored glyphs, seperated by colorChange,tab,space
      pos.x += mContext.drawText (pos, strColor, str.data(), str.data()+i);
      i = 0;
      strColor = color;
      }

    if (glyph.mChar == '\t') {
      //{{{  tab
      ImVec2 arrowLeftPos = { pos.x + 1.f, pos.y + mContext.mFontSize / 2.f };

      // apply tab
      pos.x = tabEndPos (pos.x);

      if (mOptions.mShowWhiteSpace) {
        // draw tab arrow
        ImVec2 arrowRightPos = { pos.x - 1.f, arrowLeftPos.y };
        mContext.mDrawList->AddLine (arrowLeftPos, arrowRightPos,mOptions.mPalette[(size_t)ePalette::Tab]);

        ImVec2 arrowTopPos = { arrowRightPos.x - (mContext.mFontSize * 0.2f) - 1.f,
                               arrowRightPos.y - (mContext.mFontSize * 0.2f) };
        mContext.mDrawList->AddLine (arrowRightPos, arrowTopPos, mOptions.mPalette[(size_t)ePalette::Tab]);
        ImVec2 arrowBotPos = { arrowRightPos.x - (mContext.mFontSize * 0.2f) - 1.f,
                               arrowRightPos.y + (mContext.mFontSize * 0.2f) };
        mContext.mDrawList->AddLine (arrowRightPos, arrowBotPos, mOptions.mPalette[(size_t)ePalette::Tab]);
        }
      }
      //}}}
    else if (glyph.mChar == ' ') {
      //{{{  space
      if (mOptions.mShowWhiteSpace) {
        // draw circle
        ImVec2 centre = { pos.x  + mContext.mGlyphWidth/2.f, pos.y + mContext.mFontSize/2.f};
        mContext.mDrawList->AddCircleFilled (centre, 2.f, mOptions.mPalette[(size_t)ePalette::WhiteSpace], 4);
        }

      pos.x += mContext.mGlyphWidth;
      }
      //}}}
    else {
      int length = utf8CharLength (glyph.mChar);
      while ((length-- > 0) && (i < str.max_size()))
        str[i++] = glyph.mChar;
      }
    }

  if (i > 0) // draw remaining glyphs
    pos.x += mContext.drawText (pos, strColor, str.data(), str.data()+i);

  return (pos.x - beginPosX);
  }
//}}}
//{{{
void cTextEdit::drawLine (int lineNumber, int glyphsLineNumber) {

  if (mOptions.mShowFolded)
    mInfo.mFoldLines.push_back (lineNumber);

  ImVec2 beginPos = ImGui::GetCursorScreenPos();
  beginPos.x += mContext.mPadding;
  mContext.mTextBegin = beginPos.x;

  sLine& line = mInfo.mLines[lineNumber];
  if (mOptions.mShowLineNumbers) {
    array <char,16> str;
    if (mOptions.mShowDebug)
      snprintf (str.data(), str.max_size(), "%4d %4d ", lineNumber, line.mFoldTitleLineNumber);
    else
      snprintf (str.data(), str.max_size(), "%4d ", lineNumber);

    // draw text overlaid by invisible button
    float lineNumberWidth = mContext.drawText (beginPos, mOptions.mPalette[(size_t)ePalette::LineNumber],
                                               str.data(), nullptr);

    // lineNumber invisible button
    snprintf (str.data(), str.max_size(), "##line%d", lineNumber);
    if (ImGui::InvisibleButton (str.data(), ImVec2 (lineNumberWidth, mContext.mLineHeight)))
      cLog::log (LOGINFO, "hit lineNumber");
    ImGui::SameLine();
    mContext.mTextBegin = beginPos.x + lineNumberWidth;
    }

  float textWidth = getTextWidth (sPosition (lineNumber, getLineMaxColumn (lineNumber)));
  //{{{  draw select background highlight
  sPosition lineStartPosition (lineNumber, 0);
  sPosition lineEndPosition (lineNumber, getLineMaxColumn (lineNumber));

  float xStart = -1.f;
  if (mEdit.mState.mSelectionStart <= lineEndPosition)
    xStart = (mEdit.mState.mSelectionStart > lineStartPosition) ? getTextWidth (mEdit.mState.mSelectionStart) : 0.f;

  float xEnd = -1.f;
  if (mEdit.mState.mSelectionEnd > lineStartPosition)
    xEnd = getTextWidth ((mEdit.mState.mSelectionEnd < lineEndPosition) ? mEdit.mState.mSelectionEnd : lineEndPosition);

  if (mEdit.mState.mSelectionEnd.mLineNumber > static_cast<int>(lineNumber))
    xEnd += mContext.mGlyphWidth;

  if ((xStart != -1) && (xEnd != -1) && (xStart < xEnd)) {
    ImVec2 pos = { beginPos.x + mContext.mTextBegin + xStart, beginPos.y };
    ImVec2 endPos = { beginPos.x + mContext.mTextBegin + xEnd, beginPos.y + mContext.mLineHeight };
    ImColor color = mOptions.mPalette[(size_t)ePalette::Selection];
    mContext.mDrawList->AddRectFilled (pos, endPos, color);
    }
  //}}}
  //{{{  draw marker background highlight
  auto markerIt = mOptions.mMarkers.find (lineNumber + 1);
  if (markerIt != mOptions.mMarkers.end()) {
    ImVec2 endPos = { beginPos.x + mContext.mTextBegin + textWidth, beginPos.y };
    mContext.mDrawList->AddRectFilled (beginPos, endPos, mOptions.mPalette[(size_t)ePalette::Marker]);

    if (ImGui::IsMouseHoveringRect (ImGui::GetCursorScreenPos(), endPos)) {
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
  //{{{  draw cursor background highlight
  if (!hasSelect() && (lineNumber == mEdit.mState.mCursorPosition.mLineNumber)) {
    ImVec2 endPos = { beginPos.x + mContext.mTextBegin + textWidth, beginPos.y + mContext.mLineHeight };
    ImColor fillColor = mOptions.mPalette[size_t(mContext.mFocused ? ePalette::CurrentLineFill
                                                                   : ePalette::CurrentLineFillInactive)];
    mContext.mDrawList->AddRectFilled (beginPos, endPos, fillColor);
    mContext.mDrawList->AddRect (beginPos, endPos, mOptions.mPalette[(size_t)ePalette::CurrentLineEdge], 1.f);
    }
  //}}}

  // draw text
  ImVec2 textPos = { beginPos.x + mContext.mTextBegin, beginPos.y };
  vector <sGlyph>& glyphs = line.mGlyphs;
  if (mOptions.mShowFolded && line.mFoldBegin) {
    if (line.mFoldOpen) {
      //{{{  draw foldOpen prefix + glyphs text
      // draw prefix
      float prefixWidth = mContext.drawText (textPos, mOptions.mPalette[(size_t)ePalette::FoldBeginOpen],
                                             mOptions.mLanguage.mFoldBeginOpen.c_str(), nullptr);
      // fold invisible button
      array <char,16> str;
      snprintf (str.data(), str.max_size(), "##fold%d", lineNumber);
      if (ImGui::InvisibleButton (str.data(), ImVec2 (prefixWidth, mContext.mLineHeight))) {
        cLog::log (LOGINFO, "hit foldBegin open prefix");
        }
      textPos.x += prefixWidth;

      float glyphsWidth = drawGlyphs (textPos, line.mGlyphs, false, 0);
      if (glyphsWidth < 1.f)
        glyphsWidth = 1.f;

      // text invisible button
      snprintf (str.data(), str.max_size(), "##text%d", lineNumber);
      ImGui::SameLine();
      if (ImGui::InvisibleButton (str.data(), ImVec2 (glyphsWidth, mContext.mLineHeight)))
        cLog::log (LOGINFO, "hit foldBegin open text");
      if (ImGui::IsItemActive())
        clickCursor (lineNumber, ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x, false);

      textPos.x += glyphsWidth;
      }
      //}}}
    else {
      //{{{  draw foldClosed prefix + glyphs text
      string prefixString;
      for (uint8_t i = 0; i < line.mIndent; i++)
        prefixString += ' ';
      prefixString += mOptions.mLanguage.mFoldBeginClosed;

      float prefixWidth = mContext.drawText (textPos, mOptions.mPalette[(size_t)ePalette::FoldBeginClosed],
                                             prefixString.c_str(), nullptr);
      // fold invisible button
      array <char,16> str;
      snprintf (str.data(), str.max_size(), "##fold%d", lineNumber);
      if (ImGui::InvisibleButton (str.data(), ImVec2 (prefixWidth, mContext.mLineHeight))) {
        cLog::log (LOGINFO, "hit foldBegin closed prefix");
        }

      textPos.x += prefixWidth;

      float glyphsWidth = drawGlyphs (textPos, mInfo.mLines[glyphsLineNumber].mGlyphs, true,
                                      mOptions.mPalette[(size_t)ePalette::FoldBeginClosed]);
      if (glyphsWidth < 1.f)
        glyphsWidth = 1.f;

      // text invisible button
      snprintf (str.data(), str.max_size(), "##text%d", lineNumber);
      ImGui::SameLine();
      if (ImGui::InvisibleButton (str.data(), ImVec2 (glyphsWidth, mContext.mLineHeight)))
        cLog::log (LOGINFO, "hit foldBegin closed text");
      if (ImGui::IsItemActive())
        clickCursor (lineNumber, ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x, false);

      textPos.x += glyphsWidth;
      }
      //}}}
    }
  else if (mOptions.mShowFolded && line.mFoldEnd) {
    //{{{  draw foldEnd prefix, no glyphs text
    float prefixWidth = mContext.drawText (textPos, mOptions.mPalette[(size_t)ePalette::FoldEnd],
                                           mOptions.mLanguage.mFoldEnd.c_str(), nullptr);
    // fold invisible button
    array <char,16> str;
    snprintf (str.data(), str.max_size(), "##fold%d", lineNumber);
    if (ImGui::InvisibleButton (str.data(), ImVec2 (prefixWidth, mContext.mLineHeight)))
      cLog::log (LOGINFO, "hit foldEnd");
    if (ImGui::IsItemActive())
      clickCursor (lineNumber, ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x, false);

    textPos.x += prefixWidth;
    }
    //}}}
  else {
    //{{{  draw glyphs text
    float glyphsWidth = drawGlyphs (textPos, line.mGlyphs, false, 0);
    if (glyphsWidth < 1.f)
      glyphsWidth = 1.f;

    // text invisible button
    array <char,16> str;
    snprintf (str.data(), str.max_size(), "##text%d", lineNumber);
    if (ImGui::InvisibleButton (str.data(), ImVec2 (glyphsWidth, mContext.mLineHeight)))
      cLog::log (LOGINFO, fmt::format ("hit text line:{} x:{} ",
                          lineNumber, ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x));

    if (ImGui::IsItemActive()) {
      if (ImGui::IsMouseClicked (0)) {
        //cLog::log (LOGINFO, fmt::format ("singleClick {}", lineNumber));
        clickCursor (lineNumber, ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x, false);
        }
      if (ImGui::IsMouseDoubleClicked (0)) {
        //cLog::log (LOGINFO, fmt::format ("doubleClick {}", lineNumber));
        clickCursor (lineNumber, ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x, true);
        }
      if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
        cLog::log (LOGINFO, fmt::format ("draggingtext line:{} hov:{} act:{} {}:{}",
                   lineNumber, ImGui::IsItemHovered(), ImGui::IsItemActive(),
                   ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x,
                   ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y));
      }

    //if (ImGui::IsItemHovered())
    //  cLog::log (LOGINFO, fmt::format ("hover text {} {} {}", lineNumber, ImGui::IsItemHovered(), ImGui::IsItemActive()));


    textPos.x += glyphsWidth;
    }
    //}}}

  // cursor
  if (mContext.mFocused && (lineNumber == mEdit.mState.mCursorPosition.mLineNumber)) {
    //{{{  draw flashing cursor
    chrono::system_clock::time_point now = system_clock::now();

    auto elapsed = now - mLastFlashTime;
    if (elapsed > 400ms) {
      if (elapsed > 800ms)
        mLastFlashTime = now;

      float widthToCursor = getTextWidth (mEdit.mState.mCursorPosition);
      int characterIndex = getCharacterIndex (mEdit.mState.mCursorPosition);

      float cursorWidth;
      if (mOptions.mOverWrite && (characterIndex < static_cast<int>(glyphs.size()))) {
        // overwrite
        if (glyphs[characterIndex].mChar == '\t') // widen overwrite tab cursor
          cursorWidth = tabEndPos (widthToCursor) - widthToCursor;
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
      ImVec2 pos = { beginPos.x + mContext.mTextBegin + widthToCursor - 1.f, beginPos.y };
      ImVec2 endPos = { pos.x + cursorWidth, pos.y + mContext.mLineHeight };
      mContext.mDrawList->AddRectFilled (pos, endPos, mOptions.mPalette[(size_t)ePalette::Cursor]);
      }
    }
    //}}}
  }
//}}}
//{{{
int cTextEdit::drawFold (int lineNumber, bool parentOpen, bool foldOpen) {
// recursive traversal of mInfo.mLines to produce mVisbleLines of folds

  if (parentOpen) {
    // show foldBegin line
    // - if no foldBegin comment, search for first noComment line, !!!! assume next line for now !!!!
    sLine& line = mInfo.mLines[lineNumber];
    line.mFoldTitleLineNumber = line.mHasComment ? lineNumber : lineNumber + 1;
    drawLine (lineNumber, line.mFoldTitleLineNumber);
    }

  while (true) {
    if (++lineNumber >= (int)mInfo.mLines.size())
      return lineNumber;

    sLine& line = mInfo.mLines[lineNumber];
    if (line.mFoldBegin)
      lineNumber = drawFold (lineNumber, foldOpen, line.mFoldOpen);
    else if (line.mFoldEnd) {
      if (foldOpen) // show foldEnd line of open fold
        drawLine (lineNumber, lineNumber);
      return lineNumber;
      }
    else if (foldOpen) // show all lines of open fold
      drawLine (lineNumber, lineNumber);
    }
  }
//}}}

// cTextEdit::cContext
//{{{
void cTextEdit::cContext::update (const cOptions& options) {
// update draw context

  mDrawList = ImGui::GetWindowDrawList();
  mFocused = ImGui::IsWindowFocused();

  mFont = ImGui::GetFont();
  mFontAtlasSize = ImGui::GetFontSize();
  mFontSize = static_cast<float>(options.mFontSize);

  float scale = mFontSize / mFontAtlasSize;
  mLineHeight = ImGui::GetTextLineHeight() * scale;
  mGlyphWidth = measureText ("#", nullptr) * scale;

  mPadding = mGlyphWidth / 2.f;
  mTextBegin = 0.f;
  }
//}}}
//{{{
float cTextEdit::cContext::measureText (const char* str, const char* strEnd) {
// return width of text

  return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
  }
//}}}
//{{{
float cTextEdit::cContext::drawText (ImVec2 pos, ImU32 color, const char* str, const char* strEnd) {
 // draw and return width of text

  mDrawList->AddText (mFont, mFontSize, pos, color, str, strEnd);
  return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
  }
//}}}

// cTextEdit::sUndo
//{{{
cTextEdit::sUndo::sUndo (const string& added,
                         const cTextEdit::sPosition addedStart,
                         const cTextEdit::sPosition addedEnd,
                         const string& removed,
                         const cTextEdit::sPosition removedStart,
                         const cTextEdit::sPosition removedEnd,
                         cTextEdit::sCursorSelectionState& before,
                         cTextEdit::sCursorSelectionState& after)
    : mAdded(added), mAddedStart(addedStart), mAddedEnd(addedEnd),
      mRemoved(removed), mRemovedStart(removedStart), mRemovedEnd(removedEnd),
      mBefore(before), mAfter(after) {

  assert (mAddedStart <= mAddedEnd);
  assert (mRemovedStart <= mRemovedEnd);
  }
//}}}
//{{{
void cTextEdit::sUndo::undo (cTextEdit* textEdit) {

  if (!mAdded.empty()) {
    textEdit->deleteRange (mAddedStart, mAddedEnd);
    textEdit->colorize (mAddedStart.mLineNumber - 1, mAddedEnd.mLineNumber - mAddedStart.mLineNumber + 2);
    }

  if (!mRemoved.empty()) {
    sPosition start = mRemovedStart;
    textEdit->insertTextAt (start, mRemoved.c_str());
    textEdit->colorize (mRemovedStart.mLineNumber - 1, mRemovedEnd.mLineNumber - mRemovedStart.mLineNumber + 2);
    }

  textEdit->mEdit.mState = mBefore;
  textEdit->ensureCursorVisible();
  }
//}}}
//{{{
void cTextEdit::sUndo::redo (cTextEdit* textEdit) {

  if (!mRemoved.empty()) {
    textEdit->deleteRange (mRemovedStart, mRemovedEnd);
    textEdit->colorize (mRemovedStart.mLineNumber - 1, mRemovedEnd.mLineNumber - mRemovedStart.mLineNumber + 1);
    }

  if (!mAdded.empty()) {
    sPosition start = mAddedStart;
    textEdit->insertTextAt (start, mAdded.c_str());
    textEdit->colorize (mAddedStart.mLineNumber - 1, mAddedEnd.mLineNumber - mAddedStart.mLineNumber + 1);
    }

  textEdit->mEdit.mState = mAfter;
  textEdit->ensureCursorVisible();
  }
//}}}

// cTextEdit::sLanguage
//{{{
const cTextEdit::sLanguage& cTextEdit::sLanguage::cPlus() {

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
const cTextEdit::sLanguage& cTextEdit::sLanguage::c() {

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
const cTextEdit::sLanguage& cTextEdit::sLanguage::hlsl() {

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
const cTextEdit::sLanguage& cTextEdit::sLanguage::glsl() {

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
