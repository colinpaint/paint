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
//{{{  template <...> bool equals (...)
template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals (InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, BinaryPredicate p) {
  for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
    if (!p (*first1, *first2))
      return false;
    }

  return (first1 == last1) && (first2 == last2);
  }
//}}}

namespace {
  //{{{  const
  //{{{
  const array <ImU32, cTextEdit::eMax> kPalette = {
    0xffefefef, // eBackground

    0xff404040, // eText
    0xffff0c06, // eKeyword
    0xff008000, // eNumber
    0xff2020a0, // eString
    0xff304070, // eCharLiteral
    0xff000000, // ePunctuation

    0xff406060, // ePreprocessor
    0xff404040, // eIdent
    0xff606010, // eKnownIident
    0xffc040a0, // ePreprocIdent

    0xff205020, // eComment
    0xff405020, // eMultiLineComment

    0x800010ff, // eMarker
    0x80600000, // eSelect
    0xff000000, // eCursor
    0x20000000, // eCursorLineFill
    0x40000000, // eCursorLineEdge

    0xff505000, // eLineNumber

    0xff808080, // eWhiteSpace
    0xff404040, // eTab

    0xffff0000, // eFoldBeginClosed,
    0xff0000ff, // eFoldBeginOpen,
    0xff0000ff, // eFoldEnd,

    0x80404040, // eScrollBackground
    0x80c0c0c0, // eScrollGrab
    0x80ffffff, // eScrollHover
    0xffFFFF00, // eScrollActive
    };
  //}}}

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

    const char* ptr = inBegin;

    if (*ptr == '"') {
      ptr++;

      while (ptr < inEnd) {
        // handle end of string
        if (*ptr == '"') {
          outBegin = inBegin;
          outEnd = ptr + 1;
          return true;
         }

        // handle escape character for "
        if ((*ptr == '\\') && (ptr + 1 < inEnd) && (ptr[1] == '"'))
          ptr++;

        ptr++;
        }
      }

    return false;
    }
  //}}}
  //{{{
  bool findCharacterLiteral (const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd) {

    const char* ptr = inBegin;

    if (*ptr == '\'') {
      ptr++;

      // handle escape characters
      if ((ptr < inEnd) && (*ptr == '\\'))
        ptr++;

      if (ptr < inEnd)
        ptr++;

      // handle end of character literal
      if ((ptr < inEnd) && (*ptr == '\'')) {
        outBegin = inBegin;
        outEnd = ptr + 1;
        return true;
        }
      }

    return false;
    }
  //}}}
  //{{{
  bool findIdent (const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd) {

    const char* ptr = inBegin;

    if (((*ptr >= 'a') && (*ptr <= 'z')) ||
        ((*ptr >= 'A') && (*ptr <= 'Z')) ||
        (*ptr == '_')) {
      ptr++;

      while ((ptr < inEnd) &&
             (((*ptr >= 'a') && (*ptr <= 'z')) ||
              ((*ptr >= 'A') && (*ptr <= 'Z')) ||
              ((*ptr >= '0') && (*ptr <= '9')) ||
              (*ptr == '_')))
        ptr++;

      outBegin = inBegin;
      outEnd = ptr;
      return true;
      }

    return false;
    }
  //}}}
  //{{{
  bool findNumber (const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd) {

    const char* ptr = inBegin;
    const bool startsWithNumber = (*ptr >= '0') && (*ptr <= '9');
    if ((*ptr != '+') && (*ptr != '-') && !startsWithNumber)
      return false;

    ptr++;
    bool hasNumber = startsWithNumber;
    while ((ptr < inEnd) &&
           ((*ptr >= '0') && (*ptr <= '9'))) {
      hasNumber = true;
      ptr++;
      }
    if (hasNumber == false)
      return false;

    bool isHex = false;
    bool isFloat = false;
    bool isBinary = false;
    if (ptr < inEnd) {
      if (*ptr == '.') {
        isFloat = true;
        ptr++;
        while ((ptr < inEnd) &&
               ((*ptr >= '0') && (*ptr <= '9')))
          ptr++;
        }
      else if ((*ptr == 'x') || (*ptr == 'X')) {
        //{{{  hex formatted integer of the type 0xef80
        isHex = true;
        ptr++;
        while ((ptr < inEnd) &&
               (((*ptr >= '0') && (*ptr <= '9')) ||
                ((*ptr >= 'a') && (*ptr <= 'f')) ||
                ((*ptr >= 'A') && (*ptr <= 'F'))))
          ptr++;
        }
        //}}}
      else if ((*ptr == 'b') || (*ptr == 'B')) {
        //{{{  binary formatted integer of the type 0b01011101
        isBinary = true;
        ptr++;
        while ((ptr < inEnd) && ((*ptr >= '0') && (*ptr <= '1')))
          ptr++;
        }
        //}}}
      }

    if (isHex == false && isBinary == false) {
      //{{{  floating point exponent
      if (ptr < inEnd && (*ptr == 'e' || *ptr == 'E')) {
        isFloat = true;
        ptr++;
        if (ptr < inEnd && (*ptr == '+' || *ptr == '-'))
          ptr++;

        bool hasDigits = false;
        while (ptr < inEnd && (*ptr >= '0' && *ptr <= '9')) {
          hasDigits = true;
          ptr++;
          }
        if (hasDigits == false)
          return false;
        }

      // single pecision floating point type
      if (ptr < inEnd && *ptr == 'f')
        ptr++;
      }
      //}}}

    if (isFloat == false) // integer size type
      while ((ptr < inEnd) &&
             ((*ptr == 'u') || (*ptr == 'U') || (*ptr == 'l') || (*ptr == 'L')))
        ptr++;

    outBegin = inBegin;
    outEnd = ptr;
    return true;
    }
  //}}}
  //{{{
  bool findPunctuation (const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd) {

    //{{{  unused param
    (void)inEnd;
    //}}}
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
cTextEdit::cTextEdit() {

  mInfo.mLines.push_back (vector<sGlyph>());
  setLanguage (sLanguage::cPlus());

  mLastCursorFlashTimePoint = system_clock::now();
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
      mInfo.mLines.back().mGlyphs.emplace_back (sGlyph (ch, eText));
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
        mInfo.mLines[i].mGlyphs.emplace_back (sGlyph (line[j], eText));
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
    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::setSelectionBegin (const sPosition& position) {

  mEdit.mState.mSelectionBegin = sanitizePosition (position);
  if (mEdit.mState.mSelectionBegin > mEdit.mState.mSelectionEnd)
    swap (mEdit.mState.mSelectionBegin, mEdit.mState.mSelectionEnd);
  }
//}}}
//{{{
void cTextEdit::setSelectionEnd (const sPosition& position) {

  mEdit.mState.mSelectionEnd = sanitizePosition (position);
  if (mEdit.mState.mSelectionBegin > mEdit.mState.mSelectionEnd)
    swap (mEdit.mState.mSelectionBegin, mEdit.mState.mSelectionEnd);
  }
//}}}
//{{{
void cTextEdit::setSelection (const sPosition& startPosition, const sPosition& endPosition, eSelection mode) {

  mEdit.mState.mSelectionBegin = sanitizePosition (startPosition);
  mEdit.mState.mSelectionEnd = sanitizePosition (endPosition);
  if (mEdit.mState.mSelectionBegin > mEdit.mState.mSelectionEnd)
    swap (mEdit.mState.mSelectionBegin, mEdit.mState.mSelectionEnd);

  switch (mode) {
    case cTextEdit::eSelection::Normal:
      break;

    case cTextEdit::eSelection::Word: {
      mEdit.mState.mSelectionBegin = findWordStart (mEdit.mState.mSelectionBegin);
      if (!isOnWordBoundary (mEdit.mState.mSelectionEnd))
        mEdit.mState.mSelectionEnd = findWordEnd (findWordStart (mEdit.mState.mSelectionEnd));
      break;
      }

    case cTextEdit::eSelection::Line: {
      const int lineNumber = mEdit.mState.mSelectionEnd.mLineNumber;
      mEdit.mState.mSelectionBegin = {mEdit.mState.mSelectionBegin.mLineNumber, 0};
      mEdit.mState.mSelectionEnd = {lineNumber, getLineMaxColumn (lineNumber)};
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
    scrollCursorVisible();
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
    scrollCursorVisible();
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
    scrollCursorVisible();
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
    scrollCursorVisible();
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
    undo.mRemovedStart = mEdit.mState.mSelectionBegin;
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
      undo.mRemovedStart = mEdit.mState.mSelectionBegin;
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
    undo.mRemovedStart = mEdit.mState.mSelectionBegin;
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
    undo.mRemovedStart = mEdit.mState.mSelectionBegin;
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

    scrollCursorVisible();
    colorize (mEdit.mState.mCursorPosition.mLineNumber, 1);
    }

  undo.mAfter = mEdit.mState;
  addUndo (undo);
  }
//}}}
//{{{
void cTextEdit::deleteSelection() {

  assert(mEdit.mState.mSelectionEnd >= mEdit.mState.mSelectionBegin);
  if (mEdit.mState.mSelectionEnd == mEdit.mState.mSelectionBegin)
    return;

  deleteRange (mEdit.mState.mSelectionBegin, mEdit.mState.mSelectionEnd);

  setSelection (mEdit.mState.mSelectionBegin, mEdit.mState.mSelectionBegin);
  setCursorPosition (mEdit.mState.mSelectionBegin);

  colorize (mEdit.mState.mSelectionBegin.mLineNumber, 1);
  }
//}}}

// insert
//{{{
void cTextEdit::enterCharacter (ImWchar ch, bool shift) {

  sUndo undo;
  undo.mBefore = mEdit.mState;

  if (hasSelect()) {
    if ((ch == '\t') && (mEdit.mState.mSelectionBegin.mLineNumber != mEdit.mState.mSelectionEnd.mLineNumber)) {
      //{{{  tab insert into selection
      auto start = mEdit.mState.mSelectionBegin;
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
              for (int j = 0; (j < mInfo.mTabSize) && !glyphs.empty() && (glyphs.front().mChar == ' '); j++) {
                glyphs.erase (glyphs.begin());
                modified = true;
                }
              }
            }
          }
        else {
          glyphs.insert (glyphs.begin(), sGlyph ('\t', eTab));
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

        mEdit.mState.mSelectionBegin = start;
        mEdit.mState.mSelectionEnd = end;
        addUndo (undo);
        mInfo.mTextEdited = true;

        scrollCursorVisible();
        }
        //}}}

      return;
      }
      //}}}
    else {
      //{{{  deleteSelection before insert
      undo.mRemoved = getSelectedText();
      undo.mRemovedStart = mEdit.mState.mSelectionBegin;
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
        glyphs.insert (glyphs.begin() + cindex, sGlyph (*p, eText));

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
  scrollCursorVisible();
  }
//}}}

// fold
//{{{
void cTextEdit::openFold() {

  if (isFolded())
    if (mInfo.mLines[mEdit.mState.mCursorPosition.mLineNumber].mFoldBegin)
      mInfo.mLines[mEdit.mState.mCursorPosition.mLineNumber].mFolded = false;
  }
//}}}
//{{{
void cTextEdit::closeFold() {

  if (isFolded()) {
    int lineNumber = mEdit.mState.mCursorPosition.mLineNumber;
    sLine& line = mInfo.mLines[lineNumber];

    if (line.mFoldBegin && !line.mFolded) // if at unfolded foldBegin, fold it
      line.mFolded = true;
    else {
      // search back for this fold's foldBegin and close it
      // - !!! need to skip foldEnd foldBegin pairs !!!
      while (--lineNumber >= 0) {
        line = mInfo.mLines[lineNumber];
        if (line.mFoldBegin && !line.mFolded) {
          line.mFolded = true;
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

  drawTop (app);

  // begin childWindow, new font, new colors
  if (isDrawMonoSpaced())
    ImGui::PushFont (app.getMonoFont());

  mContext.update (mOptions);

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
  ImGui::BeginChild ("##scrolling", {0.f,0.f}, false,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);
  colorizeInternal();

  handleKeyboardInputs();

  if (isFolded()) {
    //{{{  draw folded
    int lineIndex = 0;
    drawFold (0, lineIndex, false, false);
    mInfo.mFoldLines.resize (lineIndex);
    }
    //}}}
  else {
    //{{{  draw unfolded with clipper
    // clipper begin
    ImGuiListClipper clipper;
    clipper.Begin ((int)mInfo.mLines.size(), mContext.mLineHeight);
    clipper.Step();

    // clipper iterate
    for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++)
      drawLine (lineNumber, 0, 0);

    // clipper end
    clipper.Step();
    clipper.End();
    }
    //}}}

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
      column = getTabColumn (column);
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
      column = getTabColumn (column);
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
      column = getTabColumn (column);
    else
      column++;
    i += utf8CharLength (ch);
    }

  return column;
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

  if (isFolded())
    return static_cast<int>(mInfo.mFoldLines.size()-1);
  else
    return static_cast<int>(mInfo.mLines.size()-1);
  }
//}}}
//{{{
float cTextEdit::getTextWidth (const sPosition& position) const {
// get width of text in pixels, of position lineNumberline,  up to position column

  const vector<sGlyph>& glyphs = mInfo.mLines[position.mLineNumber].mGlyphs;

  float distance = 0.f;
  int colIndex = getCharacterIndex (position);
  for (size_t i = 0; (i < glyphs.size()) && ((int)i < colIndex);) {
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
string cTextEdit::getSelectedText() const {
  return getText (mEdit.mState.mSelectionBegin, mEdit.mState.mSelectionEnd);
  }
//}}}

//{{{
uint8_t cTextEdit::getGlyphColor (const sGlyph& glyph) const {

  if (glyph.mComment)
    return eComment;

  if (glyph.mMultiLineComment)
    return eMultiLineComment;

  if (glyph.mPreProc)
    return ePreprocessor;

  return glyph.mColor;
  }
//}}}

//{{{
bool cTextEdit::isOnWordBoundary (const sPosition& position) const {

  if (position.mLineNumber >= (int)mInfo.mLines.size() || position.mColumn == 0)
    return true;

  const vector<sGlyph>& glyphs = mInfo.mLines[position.mLineNumber].mGlyphs;
  int characterIndex = getCharacterIndex (position);
  if (characterIndex >= (int)glyphs.size())
    return true;

  return glyphs[characterIndex].mColor != glyphs[size_t(characterIndex - 1)].mColor;
  //return isspace (glyphs[cindex].mChar) != isspace (glyphs[cindex - 1].mChar);
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
//{{{
string cTextEdit::getWordUnderCursor() const {
  return getWordAt (getCursorPosition());
  }
//}}}

//{{{
int cTextEdit::getTabColumn (int column) const {
  return ((column / mInfo.mTabSize) * mInfo.mTabSize) + mInfo.mTabSize;
  }
//}}}
//{{{
float cTextEdit::getTabEndPosX (float xPos) const {
// return tabEnd xPos of tab containing xPos

  float tabWidthPixels = mInfo.mTabSize * mContext.mGlyphWidth;
  return (1.f + floor ((1.f + xPos) / tabWidthPixels)) * tabWidthPixels;
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::getPositionFromPosX (int lineNumber, float posX) const {

  int column = 0;
  if ((lineNumber >= 0) && (lineNumber < (int)mInfo.mLines.size())) {
    const vector<sGlyph>& glyphs = mInfo.mLines[lineNumber].mGlyphs;

    float columnX = 0.f;

    size_t glyphIndex = 0;
    while (glyphIndex < glyphs.size()) {
      float columnWidth = 0.f;
      if (glyphs[glyphIndex].mChar == '\t') {
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

  return sanitizePosition (sPosition (lineNumber, column));
  }
//}}}

//{{{
int cTextEdit::getLineNumberFromIndex (int lineIndex) const {

  if (!isFolded()) // lineNumber is lineIndex
    return lineIndex;

  if ((lineIndex >= 0) && (lineIndex < static_cast<int>(mInfo.mFoldLines.size())))
    return mInfo.mFoldLines[lineIndex];
  else
    cLog::log (LOGERROR, fmt::format ("getLineNumberFromIndex {} no line for that index", lineIndex));

  return -1;
  }
//}}}
//{{{
int cTextEdit::getLineIndexFromNumber (int lineNumber) const {

  if (!isFolded()) // lineIndex is lineNumber
    return lineNumber;

  if (mInfo.mFoldLines.empty()) {
    // no foldLines
    cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber {} mFoldLines empty", lineNumber));
    return -1;
    }

  auto it = find (mInfo.mFoldLines.begin(), mInfo.mFoldLines.end(), lineNumber);
  if (it == mInfo.mFoldLines.end()) {
    // no lineNumber for that index
    cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber {} notFound", lineNumber));
    return -1;
    }
  else
    return int(it - mInfo.mFoldLines.begin());
  }
//}}}
//}}}
//{{{  utils
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
void cTextEdit::scrollCursorVisible() {

  ImGui::SetWindowFocus();
  sPosition position = getCursorPosition();

  // up,down scroll
  int lineIndex = getLineIndexFromNumber (position.mLineNumber);
  int minIndex = min (getMaxLineIndex(),
                   static_cast<int>(floor ((ImGui::GetScrollY() + ImGui::GetWindowHeight()) / mContext.mLineHeight)));
  if (lineIndex >= minIndex - 3)
    ImGui::SetScrollY (max (0.f, (lineIndex + 3) * mContext.mLineHeight) - ImGui::GetWindowHeight());
  else {
    int maxIndex = static_cast<int>(floor (ImGui::GetScrollY() / mContext.mLineHeight));
    if (lineIndex < maxIndex + 2)
      ImGui::SetScrollY (max (0.f, (lineIndex - 2) * mContext.mLineHeight));
    }

  // left,right scroll
  float textWidth = mContext.mLineNumberWidth + getTextWidth (position);
  int minWidth = static_cast<int>(floor (ImGui::GetScrollX()));
  if (textWidth - (3 * mContext.mGlyphWidth) < minWidth)
    ImGui::SetScrollX (max (0.f, textWidth - (3 * mContext.mGlyphWidth)));
  else {
    int maxWidth = static_cast<int>(ceil (ImGui::GetScrollX() + ImGui::GetWindowWidth()));
    if (textWidth + (3 * mContext.mGlyphWidth) > maxWidth)
      ImGui::SetScrollX (max (0.f, textWidth + (3 * mContext.mGlyphWidth) - ImGui::GetWindowWidth()));
    }

  mLastCursorFlashTimePoint = system_clock::now();
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

// find
//{{{
cTextEdit::sPosition cTextEdit::findWordStart (sPosition fromPosition) const {

  if (fromPosition.mLineNumber >= (int)mInfo.mLines.size())
    return fromPosition;

  const vector<sGlyph>& glyphs = mInfo.mLines[fromPosition.mLineNumber].mGlyphs;
  int characterIndex = getCharacterIndex (fromPosition);

  if (characterIndex >= (int)glyphs.size())
    return fromPosition;

  while (characterIndex > 0 && isspace (glyphs[characterIndex].mChar))
    --characterIndex;

  uint8_t color = glyphs[characterIndex].mColor;
  while (characterIndex > 0) {
    uint8_t ch = glyphs[characterIndex].mChar;
    if ((ch & 0xC0) != 0x80) { // not UTF code sequence 10xxxxxx
      if (ch <= 32 && isspace(ch)) {
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
cTextEdit::sPosition cTextEdit::findWordEnd (sPosition fromPosition) const {

  if (fromPosition.mLineNumber >= (int)mInfo.mLines.size())
    return fromPosition;

  const vector<sGlyph>& glyphs = mInfo.mLines[fromPosition.mLineNumber].mGlyphs;
  int characterIndex = getCharacterIndex (fromPosition);

  if (characterIndex >= (int)glyphs.size())
    return fromPosition;

  bool prevSpace = (bool)isspace (glyphs[characterIndex].mChar);
  uint8_t color = glyphs[characterIndex].mColor;
  while (characterIndex < (int)glyphs.size()) {
    uint8_t ch = glyphs[characterIndex].mChar;
    int length = utf8CharLength (ch);
    if (color != glyphs[characterIndex].mColor)
      break;

    if (prevSpace != !!isspace (ch)) {
      if (isspace (ch))
        while (characterIndex < (int)glyphs.size() && isspace (glyphs[characterIndex].mChar))
          ++characterIndex;
      break;
      }
    characterIndex += length;
    }

  return sPosition (fromPosition.mLineNumber, getCharacterColumn (fromPosition.mLineNumber, characterIndex));
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::findNextWord (sPosition fromPosition) const {

  if (fromPosition.mLineNumber >= (int)mInfo.mLines.size())
    return fromPosition;

  // skip to the next non-word character
  bool isWord = false;
  bool skip = false;

  int characterIndex = getCharacterIndex (fromPosition);
  if (characterIndex < (int)mInfo.mLines[fromPosition.mLineNumber].mGlyphs.size()) {
    const vector<sGlyph>& glyphs = mInfo.mLines[fromPosition.mLineNumber].mGlyphs;
    isWord = isalnum (glyphs[characterIndex].mChar);
    skip = isWord;
    }

  while (!isWord || skip) {
    if (fromPosition.mLineNumber >= (int)mInfo.mLines.size()) {
      int lineNumber = max (0, (int)mInfo.mLines.size()-1);
      return sPosition (lineNumber, getLineMaxColumn (lineNumber));
      }

    const vector<sGlyph>& glyphs = mInfo.mLines[fromPosition.mLineNumber].mGlyphs;
    if (characterIndex < (int)glyphs.size()) {
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
void cTextEdit::moveUp (int amount) {

  if (mInfo.mLines.empty())
    return;

  sPosition position = mEdit.mState.mCursorPosition;

  int lineNumber = mEdit.mState.mCursorPosition.mLineNumber;
  if (lineNumber == 0)
    return;

  int lineIndex = getLineIndexFromNumber (lineNumber);
  lineIndex = max (0, lineIndex - amount);

  mEdit.mState.mCursorPosition.mLineNumber = getLineNumberFromIndex(lineIndex);
  if (mEdit.mState.mCursorPosition != position) {
    mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
    mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
    setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd);

    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::moveDown (int amount) {

  if (mInfo.mLines.empty())
    return;

  sPosition position = mEdit.mState.mCursorPosition;
  int lineNumber = mEdit.mState.mCursorPosition.mLineNumber;
  int lineIndex = getLineIndexFromNumber (lineNumber);
  lineIndex = min (getMaxLineIndex(), lineIndex + amount);

  mEdit.mState.mCursorPosition.mLineNumber = getLineNumberFromIndex (lineIndex);

  if (mEdit.mState.mCursorPosition != position) {
    mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
    mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
    setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd);

    scrollCursorVisible();
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
        glyphs.insert (glyphs.begin() + cindex++, sGlyph (*value++, eText));
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
  sPosition startPosition = min (position, mEdit.mState.mSelectionBegin);

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
      col.mColor = eText;
      }

    const char* bufferBegin = &buffer.front();
    const char* bufferEnd = bufferBegin + buffer.size();
    const char* lastChar = bufferEnd;
    for (auto first = bufferBegin; first != lastChar; ) {
      const char* token_begin = nullptr;
      const char* token_end = nullptr;
      uint8_t token_color = eText;

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
        if (token_color == eIdent) {
          id.assign (token_begin, token_end);

          // almost all languages use lower case to specify keywords, so shouldn't this use ::tolower ?
          if (!mOptions.mLanguage.mCaseSensitive)
            transform (id.begin(), id.end(), id.begin(),
                       [](uint8_t ch) { return static_cast<uint8_t>(std::toupper (ch)); });

          if (!glyphs[first - bufferBegin].mPreProc) {
            if (mOptions.mLanguage.mKeywords.count (id) != 0)
              token_color = eKeyword;
            else if (mOptions.mLanguage.mIdents.count (id) != 0)
              token_color = eKnownIdent;
            else if (mOptions.mLanguage.mPreprocIdents.count (id) != 0)
              token_color = ePreprocIdent;
            }
          else if (mOptions.mLanguage.mPreprocIdents.count (id) != 0)
            token_color = ePreprocIdent;
          }

        for (size_t j = 0; j < token_length; ++j)
          glyphs[(token_begin - bufferBegin) + j].mColor = token_color;

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

// clicks
//{{{
void cTextEdit::clickLine (int lineNumber) {

  mEdit.mState.mCursorPosition = sPosition (lineNumber, 0);
  mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
  mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
  mEdit.mSelection = eSelection::Line;

  setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, mEdit.mSelection);
  }
//}}}
//{{{
void cTextEdit::clickFold (int lineNumber, bool folded) {

  mEdit.mState.mCursorPosition = sPosition (lineNumber, 0);
  mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
  mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;

  mInfo.mLines[lineNumber].mFolded = folded;
  }
//}}}
//{{{
void cTextEdit::clickText (int lineNumber, float posX, bool selectWord) {

  mEdit.mState.mCursorPosition = getPositionFromPosX (lineNumber, posX);

  mEdit.mInteractiveStart = mEdit.mState.mCursorPosition;
  mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
  mEdit.mSelection = selectWord ? eSelection::Word : eSelection::Normal;

  setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, mEdit.mSelection);
  }
//}}}
//{{{
void cTextEdit::dragLine (int lineNumber, float posY) {
// if folded this will use previous displays mFoldLine vector
// - we haven't drawn subsequent lines yet
//   - works because dragging does not change vector
// - otherwise we would have to delay it to after the whole draw

  int numDragLines = static_cast<int>(posY / mContext.mLineHeight);

  if (isFolded()) {
    int lineIndex = getLineIndexFromNumber (lineNumber);
    if (lineIndex != -1) {
      //cLog::log (LOGINFO, fmt::format ("drag lineNumber:{} numDragLines:{} lineIndex:{} max:{}",
      //           lineNumber, numDragLines, lineIndex, mInfo.mFoldLines.size()));
      lineIndex = max (0, min ((int)mInfo.mFoldLines.size()-1, lineIndex + numDragLines));
      lineNumber = mInfo.mFoldLines[lineIndex];
      }
    }
  else // simple add to lineNumber
    lineNumber = max (0, min ((int)mInfo.mLines.size()-1, lineNumber + numDragLines));

  mEdit.mState.mCursorPosition.mLineNumber = lineNumber;
  mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
  mEdit.mSelection = eSelection::Line;

  setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, mEdit.mSelection);
  scrollCursorVisible();
  }
//}}}
//{{{
void cTextEdit::dragText (int lineNumber, ImVec2 pos) {

  int numDragLines = static_cast<int>(pos.y / mContext.mLineHeight);
  if (isFolded()) {
    if (numDragLines) {
      // check multiline drag across folds
      cLog::log (LOGINFO, fmt::format ("dragText - multiLine check across folds"));
      }
    }
  else {
    // allow multiline drag
    lineNumber = max (0, min ((int)mInfo.mLines.size()-1, lineNumber + numDragLines));
    }

  mEdit.mState.mCursorPosition = getPositionFromPosX (lineNumber, pos.x);
  mEdit.mInteractiveEnd = mEdit.mState.mCursorPosition;
  mEdit.mSelection = eSelection::Normal;

  setSelection (mEdit.mInteractiveStart, mEdit.mInteractiveEnd, mEdit.mSelection);
  scrollCursorVisible();
  }
//}}}

// folds
//{{{
void cTextEdit::parseLine (sLine& line) {
// parse beginFold and endFold markers, set simple flags

  // glyphs to string
  string text;
  for (auto& glyph : line.mGlyphs)
    text += glyph.mChar;

  // look for foldBegin text
  size_t foldBeginIndent = text.find (mOptions.mLanguage.mFoldBeginMarker);
  line.mFoldBegin = (foldBeginIndent != string::npos);

  if (line.mFoldBegin) {
    // found foldBegin text, find ident
    line.mIndent = static_cast<uint8_t>(foldBeginIndent);
    // has text after the foldBeginMarker
    line.mComment = (text.size() != (foldBeginIndent + mOptions.mLanguage.mFoldBeginMarker.size()));
    mInfo.mHasFolds = true;
    }
  else {
    // normal line, find indent, find comment
    size_t indent = text.find_first_not_of (' ');
    if (indent != string::npos)
      line.mIndent = static_cast<uint8_t>(indent);
    else
      line.mIndent = 0;

    // has "//" style comment as first text in line
    line.mComment = (text.find (mOptions.mLanguage.mSingleLineComment, indent) != string::npos);
    }

  // look for foldEnd text
  size_t foldEndIndent = text.find (mOptions.mLanguage.mFoldEndMarker);
  line.mFoldEnd = (foldEndIndent != string::npos);

  // init fields set by updateFolds
  line.mFolded = true;
  line.mSeeThroughInc = 0;
  }
//}}}
//{{{
void cTextEdit::parseFolds() {
// parse beginFold and endFold markers, set simple flags

  for (auto& line : mInfo.mLines)
    parseLine (line);
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
  // {false, false, false, kNumpad0,            true,  [this]{foldCreate();}},
  // {false, false, false, kNumpad4,            false, [this]{prevFile();}},
  // {false, false, false, kNumpad6,            false, [this]{nextFile();}},
  // {false, false, false, kNumpad7,            false, [this]{foldEnter();}},
  // {false, false, false, kNumpad9,            false, [this]{foldExit();}},
  // {false, true,  false, kNumpad0,            true,  [this]{foldRemove();}},
  // {false, true,  false, kNumpad3,            false, [this]{foldCloseAll();}},
  // {true,  false, false, kNumpadMulitply,     false, [this]{findDialog();}},
  // {true,  false, false, kNumpadDivide,       false, [this]{replaceDialog();}},
  // {false, false, false, F4                   false, [this]{copy();}},
  // {false, true,  false, F                    false, [this]{findDialog();}},
  // {true,  false, false, N                    false, [this]{gotoDialog();}},
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

// draws
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

  if (mInfo.mHasFolds) {
    //{{{  folded button
    ImGui::SameLine();
    if (toggleButton ("folded", isShowFolds()))
      toggleShowFolded();
    mOptions.mHoverFolded = ImGui::IsItemHovered();
    }
    //}}}

  //{{{  monoSpaced buttom
  ImGui::SameLine();
  if (toggleButton ("mono", mOptions.mShowMonoSpaced))
    toggleShowMonoSpaced();
  else
    mOptions.mHoverMonoSpaced = ImGui::IsItemHovered();
  //}}}
  //{{{  whiteSpace button
  ImGui::SameLine();
  if (toggleButton ("space", mOptions.mShowWhiteSpace))
    toggleShowWhiteSpace();
  mOptions.mHoverWhiteSpace = ImGui::IsItemHovered();
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
  ImGui::DragInt ("##fontSize", &mOptions.mFontSize, 0.2f, mOptions.mMinFontSize, mOptions.mMaxFontSize, "%d");

  if (ImGui::IsItemHovered()) {
    int fontSize = mOptions.mFontSize + static_cast<int>(ImGui::GetIO().MouseWheel);
    mOptions.mFontSize = max (mOptions.mMinFontSize, min (mOptions.mMaxFontSize, fontSize));
    }

  // debug text
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:vert:triangle", ImGui::GetIO().MetricsRenderVertices,
                                                          ImGui::GetIO().MetricsRenderIndices/3).c_str());
  //}}}
  //{{{  vsync button, fps text
  // vsync button
  ImGui::SameLine();
  if (toggleButton ("vSync", app.getPlatform().getVsync()))
    app.getPlatform().toggleVsync();

  // fps text
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:fps", static_cast<int>(ImGui::GetIO().Framerate)).c_str());
  //}}}

  //{{{  info text
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:{} {}", getCursorPosition().mColumn+1, getCursorPosition().mLineNumber+1,
                                           getTextNumLines(), getLanguage().mName).c_str());
  //}}}
  }
//}}}
//{{{
float cTextEdit::drawGlyphs (ImVec2 pos, const vector <sGlyph>& glyphs, uint8_t forceColor) {

  if (glyphs.empty())
    return 0.f;

  // pos to measure textWidth on return
  float firstPosX = pos.x;

  array <char,256> str;
  size_t strIndex = 0;
  uint8_t strColor = (forceColor < eMax) ? forceColor : getGlyphColor (glyphs[0]);
  for (auto& glyph : glyphs) {
    uint8_t color = (forceColor < eMax) ? forceColor : getGlyphColor (glyph);
    if ((strIndex > 0) && (strIndex < str.max_size()) &&
        ((color != strColor) || (glyph.mChar == '\t') || (glyph.mChar == ' '))) {
      // draw colored glyphs, seperated by colorChange,tab,space
      pos.x += mContext.drawText (pos, strColor, str.data(), str.data()+strIndex);
      strIndex = 0;
      strColor = color;
      }

    if (glyph.mChar == '\t') {
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
    else if (glyph.mChar == ' ') {
      //{{{  space
      if (isDrawWhiteSpace()) {
        // draw circle
        ImVec2 centre = { pos.x  + mContext.mGlyphWidth/2.f, pos.y + mContext.mFontSize/2.f};
        mContext.drawCircle (centre, 2.f, eWhiteSpace);
        }

      pos.x += mContext.mGlyphWidth;
      }
      //}}}
    else {
      int length = utf8CharLength (glyph.mChar);
      while ((length-- > 0) && (strIndex < str.max_size()))
        str[strIndex++] = glyph.mChar;
      }
    }

  if (strIndex > 0) // draw remaining glyphs
    pos.x += mContext.drawText (pos, strColor, str.data(), str.data() + strIndex);

  return (pos.x - firstPosX);
  }
//}}}
//{{{
int cTextEdit::drawLine (int lineNumber, uint8_t seeThroughInc, int lineIndex) {
// draw Line and return incremented lineIndex

  if (isFolded()) {
    //{{{  update mFoldLines vector
    if (lineIndex >= mInfo.mFoldLines.size())
      mInfo.mFoldLines.push_back (lineNumber);
    else
      mInfo.mFoldLines[lineIndex] = lineNumber;
    }
    //}}}

  ImVec2 leftPos = ImGui::GetCursorScreenPos();
  ImVec2 curPos = leftPos;

  float leftPadWidth = mContext.mLeftPad;
  sLine& line = mInfo.mLines[lineNumber];
  if (isDrawLineNumber()) {
    //{{{  draw lineNumber
    curPos.x += leftPadWidth;

    if (mOptions.mShowLineDebug)
      mContext.mLineNumberWidth = mContext.drawText (curPos, eLineNumber, fmt::format ("{:4d} {}{}{}{}{}{} {:2d} {:2d} ",
        lineNumber,
        line.mFoldBegin ? 'b':' ',
        line.mFoldEnd   ? 'e':' ',
        line.mComment   ? 'c':' ',
        line.mFolded    ? 'f':' ',
        line.mSelected  ? 's':' ',
        line.mPressed   ? 'p':' ',
        line.mIndent,
        line.mSeeThroughInc).c_str());
    else
      mContext.mLineNumberWidth = mContext.drawSmallText (curPos, eLineNumber, fmt::format ("{:4d} ", lineNumber).c_str());

    // add invisibleButton, gobble up leftPad
    ImGui::InvisibleButton (fmt::format ("##line{}", lineNumber).c_str(),
                            {leftPadWidth + mContext.mLineNumberWidth, mContext.mLineHeight});
    if (ImGui::IsItemActive()) {
      if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
        dragLine (lineNumber, ImGui::GetMousePos().y - curPos.y);
      else if (ImGui::IsMouseClicked (0))
        clickLine (lineNumber);
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
  vector <sGlyph>& glyphs = line.mGlyphs;
  if (isFolded() && line.mFoldBegin) {
    if (line.mFolded) {
      //{{{  draw foldBegin folded ... + glyphs text
      // add ident
      curPos.x += leftPadWidth;

      float indentWidth = line.mIndent * mContext.mGlyphWidth;
      curPos.x += indentWidth;

      // draw foldPrefix
      float prefixWidth = mContext.drawText (curPos, eFoldBeginClosed, mOptions.mLanguage.mFoldBeginClosed);
      curPos.x += prefixWidth;

      // add invisibleButton, indent + prefix wide, want to action on press
      if (ImGui::InvisibleButton (fmt::format ("##fold{}", lineNumber).c_str(),
                                  {leftPadWidth + indentWidth + prefixWidth, mContext.mLineHeight}))
        line.mPressed = false;
      if (ImGui::IsItemActive() && !line.mPressed) {
        // unfold
        line.mPressed = true;
        clickFold (lineNumber, false);
        }

      // draw glyphs
      textPos.x = curPos.x;
      float glyphsWidth = drawGlyphs (curPos, mInfo.mLines[lineNumber + seeThroughInc].mGlyphs, eFoldBeginClosed);
      if (glyphsWidth < mContext.mGlyphWidth) // widen to scroll something to pick
        glyphsWidth = mContext.mGlyphWidth;

      // add invisible button
      ImGui::SameLine();
      ImGui::InvisibleButton (fmt::format ("##text{}", lineNumber).c_str(), {glyphsWidth, mContext.mLineHeight});
      if (ImGui::IsItemActive())
        clickText (lineNumber, ImGui::GetMousePos().x - textPos.x, ImGui::IsMouseDoubleClicked (0));

      curPos.x += glyphsWidth;
      }
      //}}}
    else {
      //{{{  draw foldBegin unfolded {{{ + glyphs text
      // draw foldPrefix
      curPos.x += leftPadWidth;

      float prefixWidth = mContext.drawText (curPos, eFoldBeginOpen, mOptions.mLanguage.mFoldBeginOpen);
      curPos.x += prefixWidth;

      // add foldPrefix invisibleButton, want to action on press
      if (ImGui::InvisibleButton (fmt::format ("##fold{}", lineNumber).c_str(),
                                  {leftPadWidth + prefixWidth, mContext.mLineHeight}))
        line.mPressed = false;
      if (ImGui::IsItemActive() && !line.mPressed) {
        // fold
        line.mPressed = true;
        clickFold (lineNumber, true);
        }

      // draw glyphs
      textPos.x = curPos.x;
      float glyphsWidth = drawGlyphs (curPos, line.mGlyphs, 0xFF);
      if (glyphsWidth < mContext.mGlyphWidth) // widen to scroll something to pick
        glyphsWidth = mContext.mGlyphWidth;

      // add invisibleButton
      ImGui::SameLine();
      ImGui::InvisibleButton (fmt::format("##text{}", lineNumber).c_str(), {glyphsWidth, mContext.mLineHeight});
      if (ImGui::IsItemActive())
        clickText (lineNumber, ImGui::GetMousePos().x - textPos.x, ImGui::IsMouseDoubleClicked(0));

      curPos.x += glyphsWidth;
      }
      //}}}
    }
  else if (isFolded() && line.mFoldEnd) {
    //{{{  draw foldEnd }}}, no glyphs text
    curPos.x += leftPadWidth;
    float prefixWidth = mContext.drawText (curPos, eFoldEnd, mOptions.mLanguage.mFoldEnd);

    // add invisibleButton
    ImGui::InvisibleButton (fmt::format ("##fold{}", lineNumber).c_str(),
                            {leftPadWidth + prefixWidth, mContext.mLineHeight});
    if (ImGui::IsItemActive())
      clickFold (lineNumber, false);

    textPos.x = curPos.x;
    }
    //}}}
  else {
    //{{{  draw glyphs text
    curPos.x += leftPadWidth;

    // drawGlyphs
    textPos.x = curPos.x;
    float glyphsWidth = drawGlyphs (curPos, line.mGlyphs, 0xFF);
    if (glyphsWidth < mContext.mGlyphWidth) // widen to scroll something to pick
      glyphsWidth = mContext.mGlyphWidth;

    // add invisibleButton
    ImGui::InvisibleButton (fmt::format ("##text{}", lineNumber).c_str(),
                            {leftPadWidth + glyphsWidth, mContext.mLineHeight});
    if (ImGui::IsItemActive()) {
      if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
        dragText (lineNumber, {ImGui::GetMousePos().x - textPos.x, ImGui::GetMousePos().y - curPos.y});
      else if (ImGui::IsMouseClicked (0))
        clickText (lineNumber, ImGui::GetMousePos().x - textPos.x, ImGui::IsMouseDoubleClicked (0));
      }

    curPos.x += glyphsWidth;
    }
    //}}}

  //{{{  draw marker line highlight
  auto markerIt = mOptions.mMarkers.find (lineNumber+1);
  if (markerIt != mOptions.mMarkers.end()) {
    ImVec2 brPos = {curPos.x, curPos.y + mContext.mLineHeight};
    mContext.drawRect (leftPos, brPos, eMarker);

    if (ImGui::IsMouseHoveringRect (ImGui::GetCursorScreenPos(), brPos)) {
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
  //{{{  draw select line highlight
  sPosition lineStartPosition (lineNumber, 0);
  sPosition lineEndPosition (lineNumber, getLineMaxColumn (lineNumber));

  float xStart = -1.f;
  if (mEdit.mState.mSelectionBegin <= lineEndPosition)
    xStart = (mEdit.mState.mSelectionBegin > lineStartPosition) ? getTextWidth (mEdit.mState.mSelectionBegin) : 0.f;

  float xEnd = -1.f;
  if (mEdit.mState.mSelectionEnd > lineStartPosition)
    xEnd = getTextWidth ((mEdit.mState.mSelectionEnd < lineEndPosition) ? mEdit.mState.mSelectionEnd : lineEndPosition);

  if (mEdit.mState.mSelectionEnd.mLineNumber > static_cast<int>(lineNumber))
    xEnd += mContext.mGlyphWidth;

  if ((xStart != -1) && (xEnd != -1) && (xStart < xEnd))
    mContext.drawRect ({textPos.x + xStart, textPos.y}, {textPos.x + xEnd, textPos.y + mContext.mLineHeight}, eSelect);
  //}}}

  if (lineNumber == mEdit.mState.mCursorPosition.mLineNumber) {
    // line has cursor
    if (!hasSelect()) {
      //{{{  draw cursor line highlight
      ImVec2 brPos = {curPos.x, curPos.y + mContext.mLineHeight};
      mContext.drawRect (leftPos, brPos, eCursorLineFill);
      mContext.drawRectLine (leftPos, brPos, eCursorLineEdge);
      }
      //}}}
    //{{{  draw flashing cursor
    auto now = system_clock::now();
    auto elapsed = now - mLastCursorFlashTimePoint;
    if (elapsed < 400ms) {
      float cursorPosX = getTextWidth (mEdit.mState.mCursorPosition);
      int characterIndex = getCharacterIndex (mEdit.mState.mCursorPosition);

      float cursorWidth;
      if (mOptions.mOverWrite && (characterIndex < static_cast<int>(glyphs.size()))) {
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
      mLastCursorFlashTimePoint = now;
    //}}}
    }

  return lineIndex + 1;
  }
//}}}
//{{{
int cTextEdit::drawFold (int lineNumber, int& lineIndex, bool parentFolded, bool folded) {
// recursive traversal of mInfo.mLines to draw visible lines
// - return lineNumber, lineIndex updated

  if (!parentFolded) {
    // show foldBegin line
    // - if no foldBegin comment, search for first noComment line, !!!! assume next line for now !!!!
    sLine& line = mInfo.mLines[lineNumber];
    line.mSeeThroughInc = line.mComment ? 0 : 1;
    lineIndex = drawLine (lineNumber, line.mSeeThroughInc, lineIndex);
    }

  while (true) {
    if (++lineNumber >= (int)mInfo.mLines.size())
      return lineNumber;

    sLine& line = mInfo.mLines[lineNumber];
    if (line.mFoldBegin)
      lineNumber = drawFold (lineNumber, lineIndex, folded, line.mFolded);
    else if (line.mFoldEnd) {
      if (!folded) // show foldEnd line of open fold
        lineIndex = drawLine (lineNumber, 0, lineIndex);
      return lineNumber;
      }
    else if (!folded) // show all lines of open fold
      lineIndex = drawLine (lineNumber, 0, lineIndex);
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
  mGlyphWidth = measureText ("#", nullptr) * scale;
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
//{{{  cTextEdit::sUndo
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
  textEdit->scrollCursorVisible();
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
  textEdit->scrollCursorVisible();
  }
//}}}
//}}}
//{{{  cTextEdit::sLanguage
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
                            const char*& outBegin, const char*& outEnd, uint8_t& palette) -> bool {
      //{{{  tokenize lambda
      palette = eMax;

      while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
        inBegin++;

      if (inBegin == inEnd) {
        outBegin = inEnd;
        outEnd = inEnd;
        palette = eText;
        }
      else if (findString (inBegin, inEnd, outBegin, outEnd))
        palette = eString;
      else if (findCharacterLiteral (inBegin, inEnd, outBegin, outEnd))
        palette = eCharLiteral;
      else if (findIdent (inBegin, inEnd, outBegin, outEnd))
        palette = eIdent;
      else if (findNumber (inBegin, inEnd, outBegin, outEnd))
        palette = eNumber;
      else if (findPunctuation (inBegin, inEnd, outBegin, outEnd))
        palette = ePunctuation;

      return palette != eMax;
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
                           const char *& outBegin, const char *& outEnd, uint8_t& palette) -> bool {
      palette = eMax;

      while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
        inBegin++;

      if (inBegin == inEnd) {
        outBegin = inEnd;
        outEnd = inEnd;
        palette = eText;
        }
      else if (findString (inBegin, inEnd, outBegin, outEnd))
        palette = eString;
      else if (findCharacterLiteral (inBegin, inEnd, outBegin, outEnd))
        palette = eCharLiteral;
      else if (findIdent (inBegin, inEnd, outBegin, outEnd))
        palette = eIdent;
      else if (findNumber (inBegin, inEnd, outBegin, outEnd))
        palette = eNumber;
      else if (findPunctuation (inBegin, inEnd, outBegin, outEnd))
        palette = ePunctuation;

      return palette != eMax;
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
      make_pair<string, uint8_t>("[ \\t]*#[ \\t]*[a-zA-Z_]+", (uint8_t)ePreprocessor));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("L?\\\"(\\\\.|[^\\\"])*\\\"", (uint8_t)eString));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("\\'\\\\?[^\\']\\'", (uint8_t)eCharLiteral));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", (uint8_t)eNumber));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", (uint8_t)eNumber));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("0[0-7]+[Uu]?[lL]?[lL]?", (uint8_t)eNumber));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", (uint8_t)eNumber));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("[a-zA-Z_][a-zA-Z0-9_]*", (uint8_t)eIdent));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", (uint8_t)ePunctuation));

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
      make_pair<string, uint8_t>("[ \\t]*#[ \\t]*[a-zA-Z_]+", (uint8_t)ePreprocessor));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("L?\\\"(\\\\.|[^\\\"])*\\\"", (uint8_t)eString));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("\\'\\\\?[^\\']\\'", (uint8_t)eCharLiteral));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", (uint8_t)eNumber));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", (uint8_t)eNumber));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("0[0-7]+[Uu]?[lL]?[lL]?", (uint8_t)eNumber));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", (uint8_t)eNumber));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("[a-zA-Z_][a-zA-Z0-9_]*", (uint8_t)eIdent));
    language.mTokenRegexStrings.push_back (
      make_pair<string, uint8_t>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", (uint8_t)ePunctuation));

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
//}}}
