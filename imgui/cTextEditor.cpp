// cTextEditor.cpp - nicked from https://github.com/BalazsJako/ImGuiColorTextEdit
//{{{  includes
#include <string>
#include <cmath>
#include <algorithm>
#include <regex>
#include <chrono>

#include "cTextEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

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
    };
  //}}}

  //{{{
  const vector<string> kCppKeywords = {
    "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto",
    "bitand", "bitor", "bool", "break",
    "case", "catch", "char", "char16_t", "char32_t", "class", "compl", "concept", "const", "constexpr", "const_cast", "continue",
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
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
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
  bool IsUTFSequence (char ch) {
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
  int ImTextCharToUtf8 (char* buf, int buf_size, uint32_t ch) {

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

// cTextEditor::sUndoRecord
//{{{
cTextEditor::sUndoRecord::sUndoRecord (const string& added,
                                       const cTextEditor::sRowColumn addedStart,
                                       const cTextEditor::sRowColumn addedEnd,
                                       const string& removed,
                                       const cTextEditor::sRowColumn removedStart,
                                       const cTextEditor::sRowColumn removedEnd,
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
    editor->colorize (mAddedStart.mRow - 1, mAddedEnd.mRow - mAddedStart.mRow + 2);
    }

  if (!mRemoved.empty()) {
    sRowColumn start = mRemovedStart;
    editor->insertTextAt (start, mRemoved.c_str());
    editor->colorize (mRemovedStart.mRow - 1, mRemovedEnd.mRow - mRemovedStart.mRow + 2);
    }

  editor->mState = mBefore;
  editor->ensureCursorVisible();
  }
//}}}
//{{{
void cTextEditor::sUndoRecord::redo (cTextEditor* editor) {

  if (!mRemoved.empty()) {
    editor->deleteRange (mRemovedStart, mRemovedEnd);
    editor->colorize (mRemovedStart.mRow - 1, mRemovedEnd.mRow - mRemovedStart.mRow + 1);
    }

  if (!mAdded.empty()) {
    sRowColumn start = mAddedStart;
    editor->insertTextAt (start, mAdded.c_str());
    editor->colorize (mAddedStart.mRow - 1, mAddedEnd.mRow - mAddedStart.mRow + 1);
    }

  editor->mState = mAfter;
  editor->ensureCursorVisible();
  }
//}}}

// cTextEditor::sLanguage
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::CPlusPlus() {

  static sLanguage language;
  static bool inited = false;

  if (!inited) {
    for (auto& cppKeyword:  kCppKeywords)
      language.mKeywords.insert (cppKeyword);

    for (auto& cppIdent : kCppIdents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair (cppIdent, id));
      }

    language.mTokenize = [](const char* inBegin, const char* inEnd,
                           const char*& outBegin, const char*& outEnd, ePalette& palette) -> bool {
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
    language.mCaseSensitive = true;
    language.mAutoIndentation = true;
    language.mName = "C++";

    inited = true;
    }

  return language;
  }
//}}}
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::C() {

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
    language.mCaseSensitive = true;
    language.mAutoIndentation = true;
    language.mName = "C";

    inited = true;
    }

  return language;
  }
//}}}
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::HLSL() {

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
    language.mCaseSensitive = true;
    language.mAutoIndentation = true;
    language.mName = "HLSL";

    inited = true;
    }

  return language;
  }
//}}}
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::GLSL() {

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
    language.mCaseSensitive = true;
    language.mAutoIndentation = true;
    language.mName = "GLSL";

    inited = true;
    }

  return language;
  }
//}}}

// cTextEditor
//{{{
cTextEditor::cTextEditor()
  : mLineSpacing(1.0f), mUndoIndex(0), mTabSize(4),
    mOverwrite(false) , mReadOnly(false) , mWithinRender(false), mScrollToCursor(false), mScrollToTop(false),
    mTextChanged(false), mColorizerEnabled(true),
    mTextStart(20.0f), mLeftMargin(10), mCursorPositionChanged(false),
    mColorRangeMin(0), mColorRangeMax(0),
    mSelection(eSelection::Normal) ,
    mHandleKeyboardInputs(true), mHandleMouseInputs(true),
    mIgnoreImGuiChild(false), mShowWhitespaces(true), mCheckComments(true),
    mStartTime(chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()),
    mLastClick(-1.0f) {

  mPaletteBase = kLightPalette;
  setLanguage (sLanguage::HLSL());

  mLines.push_back (vector<sGlyph>());
  }
//}}}
//{{{  gets
//{{{
bool cTextEditor::isOnWordBoundary (const sRowColumn& at) const {

  if (at.mRow >= (int)mLines.size() || at.mColumn == 0)
    return true;

  const vector<sGlyph>& glyphs = mLines[at.mRow].mGlyphs;
  int cindex = getCharacterIndex (at);
  if (cindex >= (int)glyphs.size())
    return true;

  if (mColorizerEnabled)
    return glyphs[cindex].mColorIndex != glyphs[size_t(cindex - 1)].mColorIndex;

  return isspace (glyphs[cindex].mChar) != isspace (glyphs[cindex - 1].mChar);
  }
//}}}

//{{{
string cTextEditor::getText() const {
  return getText (sRowColumn(), sRowColumn((int)mLines.size(), 0));
  }
//}}}
//{{{
vector<string> cTextEditor::getTextLines() const {

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

//{{{
string cTextEditor::getCurrentLineText()const {

  int lineLength = getLineMaxColumn (mState.mCursorPosition.mRow);
  return getText (sRowColumn (mState.mCursorPosition.mRow, 0),
                  sRowColumn (mState.mCursorPosition.mRow, lineLength));
  }
//}}}

//{{{
int cTextEditor::getCharacterIndex (const sRowColumn& rowColumn) const {

  if (rowColumn.mRow >= (int)mLines.size()) {
    cLog::log (LOGERROR, "cTextEditor::getCharacterIndex");
    return -1;
    }

  const vector<sGlyph>& glyphs = mLines[rowColumn.mRow].mGlyphs;

  int c = 0;
  int i = 0;
  for (; i < (int)glyphs.size() && (c < rowColumn.mColumn);) {
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
//}}}
//{{{  sets
//{{{
void cTextEditor::setText (const string& text) {
// break test into lines, store in internal mLines structure

  mLines.clear();
  mLines.emplace_back (vector<sGlyph>());

  for (auto chr : text)
    if (chr == '\r') {
      // ignore the carriage return character
      }
    else if (chr == '\n')
      mLines.emplace_back (vector<sGlyph>());
    else
      mLines.back().mGlyphs.emplace_back (sGlyph (chr, ePalette::Default));

  mTextChanged = true;
  mScrollToTop = true;

  mUndoBuffer.clear();
  mUndoIndex = 0;

  colorize();
  }
//}}}
//{{{
void cTextEditor::setTextLines (const vector<string>& lines) {
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
void cTextEditor::setCursorPosition (const sRowColumn& position) {

  if (mState.mCursorPosition != position) {
    mState.mCursorPosition = position;
    mCursorPositionChanged = true;
    ensureCursorVisible();
    }
  }
//}}}

//{{{
void cTextEditor::setSelectionStart (const sRowColumn& position) {

  mState.mSelectionStart = sanitizeRowColumn (position);

  if (mState.mSelectionStart > mState.mSelectionEnd)
    swap (mState.mSelectionStart, mState.mSelectionEnd);
  }
//}}}
//{{{
void cTextEditor::setSelectionEnd (const sRowColumn& position) {

  mState.mSelectionEnd = sanitizeRowColumn (position);
  if (mState.mSelectionStart > mState.mSelectionEnd)
    swap (mState.mSelectionStart, mState.mSelectionEnd);
  }
//}}}
//{{{
void cTextEditor::setSelection (const sRowColumn& startRowColumn, const sRowColumn& endRowColumn, eSelection mode) {

  sRowColumn oldSelStart = mState.mSelectionStart;
  sRowColumn oldSelEnd = mState.mSelectionEnd;

  mState.mSelectionStart = sanitizeRowColumn (startRowColumn);
  mState.mSelectionEnd = sanitizeRowColumn(endRowColumn);
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
      const int lineNumber = mState.mSelectionEnd.mRow;
      //const auto lineSize = (size_t)lineNumber < mLines.size() ? mLines[lineNumber].size() : 0;
      mState.mSelectionStart = sRowColumn (mState.mSelectionStart.mRow, 0);
      mState.mSelectionEnd = sRowColumn (lineNumber, getLineMaxColumn (lineNumber));
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
//{{{
void cTextEditor::moveUp (int amount, bool select) {

  sRowColumn oldPos = mState.mCursorPosition;
  mState.mCursorPosition.mRow = max (0, mState.mCursorPosition.mRow - amount);
  if (oldPos != mState.mCursorPosition) {
    if (select) {
      if (oldPos == mInteractiveStart)
        mInteractiveStart = mState.mCursorPosition;
      else if (oldPos == mInteractiveEnd)
        mInteractiveEnd = mState.mCursorPosition;
      else {
        mInteractiveStart = mState.mCursorPosition;
        mInteractiveEnd = oldPos;
        }
      }
    else
      mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;

    setSelection (mInteractiveStart, mInteractiveEnd);
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEditor::moveDown (int amount, bool select) {

  assert(mState.mCursorPosition.mColumn >= 0);
  sRowColumn oldPos = mState.mCursorPosition;
  mState.mCursorPosition.mRow = max (0, min((int)mLines.size() - 1, mState.mCursorPosition.mRow + amount));

  if (mState.mCursorPosition != oldPos) {
    if (select) {
      if (oldPos == mInteractiveEnd)
        mInteractiveEnd = mState.mCursorPosition;
      else if (oldPos == mInteractiveStart)
        mInteractiveStart = mState.mCursorPosition;
      else {
        mInteractiveStart = oldPos;
        mInteractiveEnd = mState.mCursorPosition;
        }
      }
    else
      mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;

    setSelection (mInteractiveStart, mInteractiveEnd);
    ensureCursorVisible();
    }
  }
//}}}
//{{{
void cTextEditor::moveLeft (int amount, bool select, bool wordMode) {

  if (mLines.empty())
    return;

  sRowColumn oldPos = mState.mCursorPosition;
  mState.mCursorPosition = getActualCursorRowColumn();

  int line = mState.mCursorPosition.mRow;
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
          while (cindex > 0 && IsUTFSequence (mLines[line].mGlyphs[cindex].mChar))
            --cindex;
          }
        }
      }

    mState.mCursorPosition = sRowColumn (line, getCharacterColumn (line, cindex));
    if (wordMode) {
      mState.mCursorPosition = findWordStart (mState.mCursorPosition);
      cindex = getCharacterIndex (mState.mCursorPosition);
      }
    }

  mState.mCursorPosition = sRowColumn (line, getCharacterColumn (line, cindex));

  assert (mState.mCursorPosition.mColumn >= 0);
  if (select) {
    if (oldPos == mInteractiveStart)
      mInteractiveStart = mState.mCursorPosition;
    else if (oldPos == mInteractiveEnd)
      mInteractiveEnd = mState.mCursorPosition;
    else {
      mInteractiveStart = mState.mCursorPosition;
      mInteractiveEnd = oldPos;
      }
    }
  else
    mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;

  setSelection (mInteractiveStart, mInteractiveEnd, select && wordMode ? eSelection::Word : eSelection::Normal);
  ensureCursorVisible();
  }
//}}}
//{{{
void cTextEditor::moveRight(int amount, bool select, bool wordMode) {

  sRowColumn oldPos = mState.mCursorPosition;

  if (mLines.empty() || oldPos.mRow >= (int)mLines.size())
    return;

  int cindex = getCharacterIndex (mState.mCursorPosition);
  while (amount-- > 0) {
    int lindex = mState.mCursorPosition.mRow;
    sLine& line = mLines [lindex];

    if (cindex >= (int)line.mGlyphs.size()) {
      if (mState.mCursorPosition.mRow < (int)mLines.size() - 1) {
        mState.mCursorPosition.mRow = max (0, min ((int)mLines.size() - 1, mState.mCursorPosition.mRow + 1));
        mState.mCursorPosition.mColumn = 0;
        }
      else
        return;
      }
    else {
      cindex += utf8CharLength (line.mGlyphs[cindex].mChar);
      mState.mCursorPosition = sRowColumn (lindex, getCharacterColumn (lindex, cindex));
      if (wordMode)
         mState.mCursorPosition = findNextWord (mState.mCursorPosition);
     }
    }

  if (select) {
    if (oldPos == mInteractiveEnd)
      mInteractiveEnd = sanitizeRowColumn (mState.mCursorPosition);
    else if (oldPos == mInteractiveStart)
      mInteractiveStart = mState.mCursorPosition;
    else {
      mInteractiveStart = oldPos;
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
void cTextEditor::moveTop (bool select) {

  sRowColumn oldPos = mState.mCursorPosition;
  setCursorPosition (sRowColumn(0, 0));

  if (mState.mCursorPosition != oldPos) {
    if (select) {
      mInteractiveEnd = oldPos;
      mInteractiveStart = mState.mCursorPosition;
      }
    else
      mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;

    setSelection(mInteractiveStart, mInteractiveEnd);
    }
  }
//}}}
//{{{
void cTextEditor::moveBottom (bool select) {

  sRowColumn oldPos = getCursorPosition();
  sRowColumn newPos = sRowColumn ((int)mLines.size() - 1, 0);

  setCursorPosition (newPos);

  if (select) {
    mInteractiveStart = oldPos;
    mInteractiveEnd = newPos;
    }
  else
    mInteractiveStart = mInteractiveEnd = newPos;

  setSelection (mInteractiveStart, mInteractiveEnd);
  }
//}}}
//{{{
void cTextEditor::moveHome (bool select) {

  sRowColumn oldPos = mState.mCursorPosition;
  setCursorPosition (sRowColumn(mState.mCursorPosition.mRow, 0));

  if (mState.mCursorPosition != oldPos) {
    if (select) {
      if (oldPos == mInteractiveStart)
        mInteractiveStart = mState.mCursorPosition;
      else if (oldPos == mInteractiveEnd)
        mInteractiveEnd = mState.mCursorPosition;
      else {
        mInteractiveStart = mState.mCursorPosition;
        mInteractiveEnd = oldPos;
        }
      }
    else
      mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;

    setSelection (mInteractiveStart, mInteractiveEnd);
    }
  }
//}}}
//{{{
void cTextEditor::moveEnd (bool select) {

  sRowColumn oldPos = mState.mCursorPosition;
  setCursorPosition (sRowColumn (mState.mCursorPosition.mRow, getLineMaxColumn (oldPos.mRow)));

  if (mState.mCursorPosition != oldPos) {
    if (select) {
      if (oldPos == mInteractiveEnd)
        mInteractiveEnd = mState.mCursorPosition;
      else if (oldPos == mInteractiveStart)
        mInteractiveStart = mState.mCursorPosition;
      else {
        mInteractiveStart = oldPos;
        mInteractiveEnd = mState.mCursorPosition;
        }
      }
    else
      mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;

    setSelection (mInteractiveStart, mInteractiveEnd);
    }
  }
//}}}

//{{{
void cTextEditor::selectAll() {
  setSelection (sRowColumn (0,0), sRowColumn ((int)mLines.size(), 0));
  }
//}}}
//{{{
void cTextEditor::selectWordUnderCursor() {

  sRowColumn c = getCursorPosition();
  setSelection (findWordStart (c), findWordEnd (c));
  }
//}}}

//{{{
void cTextEditor::insertText (const char * value) {

  if (value == nullptr)
    return;

  sRowColumn pos = getActualCursorRowColumn();
  sRowColumn start = min (pos, mState.mSelectionStart);

  int totalLines = pos.mRow - start.mRow;
  totalLines += insertTextAt (pos, value);

  setSelection (pos, pos);
  setCursorPosition (pos);

  colorize (start.mRow - 1, totalLines + 2);
  }
//}}}
//{{{
void cTextEditor::copy() {

  if (hasSelection())
    ImGui::SetClipboardText (getSelectedText().c_str());

  else if (!mLines.empty()) {
    sLine& line = mLines[getActualCursorRowColumn().mRow];

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

  else if (hasSelection()) {
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
    if (hasSelection()) {
      u.mRemoved = getSelectedText();
      u.mRemovedStart = mState.mSelectionStart;
      u.mRemovedEnd = mState.mSelectionEnd;
      deleteSelection();
      }

    u.mAdded = clipText;
    u.mAddedStart = getActualCursorRowColumn();

    insertText (clipText);

    u.mAddedEnd = getActualCursorRowColumn();
    u.mAfter = mState;
    addUndo (u);
    }
  }
//}}}
//{{{
void cTextEditor::deleteIt() {

  assert(!mReadOnly);

  if (mLines.empty())
    return;

  sUndoRecord u;
  u.mBefore = mState;

  if (hasSelection()) {
    u.mRemoved = getSelectedText();
    u.mRemovedStart = mState.mSelectionStart;
    u.mRemovedEnd = mState.mSelectionEnd;
    deleteSelection();
    }

  else {
    sRowColumn pos = getActualCursorRowColumn();
    setCursorPosition (pos);
    vector<sGlyph>& glyphs = mLines[pos.mRow].mGlyphs;

    if (pos.mColumn == getLineMaxColumn (pos.mRow)) {
      if (pos.mRow == (int)mLines.size() - 1)
        return;

      u.mRemoved = '\n';
      u.mRemovedStart = u.mRemovedEnd = getActualCursorRowColumn();
      advance (u.mRemovedEnd);

      vector<sGlyph>& nextLineGlyphs = mLines[pos.mRow + 1].mGlyphs;
      glyphs.insert (glyphs.end(), nextLineGlyphs.begin(), nextLineGlyphs.end());
      removeLine (pos.mRow + 1);
      }
    else {
      int cindex = getCharacterIndex (pos);
      u.mRemovedStart = u.mRemovedEnd = getActualCursorRowColumn();
      u.mRemovedEnd.mColumn++;
      u.mRemoved = getText (u.mRemovedStart, u.mRemovedEnd);

      int d = utf8CharLength (glyphs[cindex].mChar);
      while (d-- > 0 && cindex < (int)glyphs.size())
        glyphs.erase (glyphs.begin() + cindex);
      }

    mTextChanged = true;
    colorize(pos.mRow, 1);
    }

  u.mAfter = mState;
  addUndo (u);
  }
//}}}

//{{{
void cTextEditor::undo (int steps) {

  while (canUndo() && steps-- > 0)
    mUndoBuffer[--mUndoIndex].undo (this);
  }
//}}}
//{{{
void cTextEditor::redo (int steps) {

  while (canRedo() && steps-- > 0)
    mUndoBuffer[mUndoIndex++].redo (this);
  }
//}}}
//}}}
//{{{
void cTextEditor::render (const char* title, const ImVec2& size, bool border) {

  mWithinRender = true;
  mTextChanged = false;
  mCursorPositionChanged = false;

  ImGui::PushStyleColor (ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4 (mPalette[(int)ePalette::Background]));
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

  if (!mIgnoreImGuiChild)
    ImGui::BeginChild (title, size, border,
                       ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar |
                       ImGuiWindowFlags_NoMove);

  if (mHandleKeyboardInputs) {
    handleKeyboardInputs();
    ImGui::PushAllowKeyboardFocus (true);
    }

  if (mHandleMouseInputs)
    handleMouseInputs();

  colorizeInternal();
  render();

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
int cTextEditor::getPageSize() const {

  float height = ImGui::GetWindowHeight() - 20.0f;
  return (int)floor (height / mCharAdvance.y);
  }
//}}}
//{{{
string cTextEditor::getText (const sRowColumn& startRowColumn, const sRowColumn& endRowColumn) const {

  string result;

  int lstart = startRowColumn.mRow;
  int lend = endRowColumn.mRow;

  int istart = getCharacterIndex (startRowColumn);
  int iend = getCharacterIndex (endRowColumn);

  size_t s = 0;
  for (size_t i = lstart; i < (size_t)lend; i++)
    s += mLines[i].mGlyphs.size();

  result.reserve (s + s / 8);

  while (istart < iend || lstart < lend) {
    if (lstart >= (int)mLines.size())
      break;

    const vector<sGlyph>& glyphs = mLines[lstart].mGlyphs;
    if (istart < (int)glyphs.size()) {
      result += glyphs[istart].mChar;
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

  if (!mColorizerEnabled)
    return mPalette[(int)ePalette::Default];

  if (glyph.mComment)
    return mPalette[(int)ePalette::Comment];

  if (glyph.mMultiLineComment)
    return mPalette[(int)ePalette::MultiLineComment];

  auto const color = mPalette[(int)glyph.mColorIndex];
  if (glyph.mPreprocessor) {
    const auto ppcolor = mPalette[(int)ePalette::Preprocessor];
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
string cTextEditor::getWordAt (const sRowColumn& rowColumn) const {

  sRowColumn start = findWordStart (rowColumn);
  sRowColumn end = findWordEnd (rowColumn);

  int istart = getCharacterIndex (start);
  int iend = getCharacterIndex (end);

  string r;
  for (int it = istart; it < iend; ++it)
    r.push_back (mLines[rowColumn.mRow].mGlyphs[it].mChar);

  return r;
  }
//}}}
//{{{
string cTextEditor::getWordUnderCursor() const {

  sRowColumn c = getCursorPosition();
  return getWordAt (c);
  }
//}}}

//{{{
float cTextEditor::getTextDistanceToLineStart (const sRowColumn& from) const {

  const vector<sGlyph>& glyphs = mLines[from.mRow].mGlyphs;
  float distance = 0.0f;

  float spaceSize = ImGui::GetFont()->CalcTextSizeA (ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

  int colIndex = getCharacterIndex (from);
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
      distance += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
      }
    }

  return distance;
  }
//}}}

// rowColumn
//{{{
cTextEditor::sRowColumn cTextEditor::sanitizeRowColumn (const sRowColumn& rowColumn) const {

  int line = rowColumn.mRow;
  int column = rowColumn.mColumn;

  if (line >= (int)mLines.size()) {
    if (mLines.empty()) {
      line = 0;
      column = 0;
      }
    else {
      line = (int)mLines.size() - 1;
      column = getLineMaxColumn (line);
      }

    return sRowColumn (line, column);
    }

  else {
    column = mLines.empty() ? 0 : min (column, getLineMaxColumn (line));
    return sRowColumn (line, column);
    }

  }
//}}}
//{{{
cTextEditor::sRowColumn cTextEditor::screenPosToRowColumn (const ImVec2& position) const {

  ImVec2 origin = ImGui::GetCursorScreenPos();
  ImVec2 local (position.x - origin.x, position.y - origin.y);

  int lineNumber = max(0, (int)floor (local.y / mCharAdvance.y));
  int columnCoord = 0;

  if ((lineNumber >= 0) && (lineNumber < (int)mLines.size())) {
    const vector<sGlyph>& glyphs = mLines.at (lineNumber).mGlyphs;

    int columnIndex = 0;
    float columnX = 0.0f;

    while (columnIndex < (int)glyphs.size()) {
      float columnWidth = 0.0f;

      if (glyphs[columnIndex].mChar == '\t') {
        float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
        float oldX = columnX;
        float newColumnX = (1.0f + floor ((1.0f + columnX) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
        columnWidth = newColumnX - oldX;
        if (mTextStart + columnX + columnWidth * 0.5f > local.x)
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
        if (mTextStart + columnX + columnWidth * 0.5f > local.x)
          break;

        columnX += columnWidth;
        columnCoord++;
        }
      }
    }

  return sanitizeRowColumn (sRowColumn (lineNumber, columnCoord));
  }
//}}}

// finds
//{{{
cTextEditor::sRowColumn cTextEditor::findWordStart (const sRowColumn& from) const {

  sRowColumn at = from;
  if (at.mRow >= (int)mLines.size())
    return at;

  const vector<sGlyph>& glyphs = mLines[at.mRow].mGlyphs;
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

  return sRowColumn (at.mRow, getCharacterColumn (at.mRow, cindex));
  }
//}}}
//{{{
cTextEditor::sRowColumn cTextEditor::findWordEnd (const sRowColumn& from) const {

  sRowColumn at = from;
  if (at.mRow >= (int)mLines.size())
    return at;

  const vector<sGlyph>& glyphs = mLines[at.mRow].mGlyphs;
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

  return sRowColumn (from.mRow, getCharacterColumn (from.mRow, cindex));
  }
//}}}
//{{{
cTextEditor::sRowColumn cTextEditor::findNextWord (const sRowColumn& from) const {

  sRowColumn at = from;
  if (at.mRow >= (int)mLines.size())
    return at;

  // skip to the next non-word character
  bool isword = false;
  bool skip = false;

  int cindex = getCharacterIndex (from);
  if (cindex < (int)mLines[at.mRow].mGlyphs.size()) {
    const vector<sGlyph>& glyphs = mLines[at.mRow].mGlyphs;
    isword = isalnum (glyphs[cindex].mChar);
    skip = isword;
    }

  while (!isword || skip) {
    if (at.mRow >= (int)mLines.size()) {
      int l = max(0, (int)mLines.size() - 1);
      return sRowColumn (l, getLineMaxColumn(l));
      }

    const vector<sGlyph>& glyphs = mLines[at.mRow].mGlyphs;
    if (cindex < (int)glyphs.size()) {
      isword = isalnum (glyphs[cindex].mChar);

      if (isword && !skip)
        return sRowColumn (at.mRow, getCharacterColumn (at.mRow, cindex));

      if (!isword)
        skip = false;

      cindex++;
      }
    else {
      cindex = 0;
      ++at.mRow;
      skip = false;
      isword = false;
      }
    }

  return at;
  }
//}}}
//}}}
//{{{  colorize
//{{{
void cTextEditor::colorize (int fromLine, int lines) {

  int toLine = lines == -1 ? (int)mLines.size() : min((int)mLines.size(), fromLine + lines);

  mColorRangeMin = min (mColorRangeMin, fromLine);
  mColorRangeMax = max (mColorRangeMax, toLine);
  mColorRangeMin = max (0, mColorRangeMin);
  mColorRangeMax = max(mColorRangeMin, mColorRangeMax);

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
        // todo : remove
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

      if (hasTokenizeResult == false) {
        first++;
        }
      else {
        const size_t token_length = token_end - token_begin;
        if (token_color == ePalette::Ident) {
          id.assign (token_begin, token_end);

          // almost all languages use lower case to specify keywords, so shouldn't this use ::tolower ?
          if (!mLanguage.mCaseSensitive)
            transform (id.begin(), id.end(), id.begin(),
                       [](uint8_t ch) { return static_cast<uint8_t>(std::toupper (ch)); });

          if (!glyphs[first - bufferBegin].mPreprocessor) {
            if (mLanguage.mKeywords.count(id) != 0)
              token_color = ePalette::Keyword;
            else if (mLanguage.mIdents.count(id) != 0)
              token_color = ePalette::KnownIdent;
            else if (mLanguage.mPreprocIdents.count(id) != 0)
              token_color = ePalette::PreprocIdent;
            }
          else {
            if (mLanguage.mPreprocIdents.count(id) != 0)
              token_color = ePalette::PreprocIdent;
            }
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

  if (mLines.empty() || !mColorizerEnabled)
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
            string& startStr = mLanguage.mCommentStart;
            string& singleStartStr = mLanguage.mSingleLineComment;

            if ((singleStartStr.size() > 0) &&
                (currentIndex + singleStartStr.size() <= line.size()) &&
                equals (singleStartStr.begin(), singleStartStr.end(), from, from + singleStartStr.size(), pred)) {
              withinSingleLineComment = true;
              }
            else if ((!withinSingleLineComment && currentIndex + startStr.size() <= line.size()) &&
                     equals (startStr.begin(), startStr.end(), from, from + startStr.size(), pred)) {
              commentStartLine = currentLine;
              commentStartIndex = currentIndex;
              }

            inComment = (commentStartLine < currentLine ||
                         (commentStartLine == currentLine && commentStartIndex <= currentIndex));

            line[currentIndex].mMultiLineComment = inComment;
            line[currentIndex].mComment = withinSingleLineComment;

            string& endStr = mLanguage.mCommentEnd;
            if (currentIndex + 1 >= (int)endStr.size() &&
              equals(endStr.begin(), endStr.end(), from + 1 - endStr.size(), from + 1, pred)) {
              commentStartIndex = endIndex;
              commentStartLine = endLine;
              }
            }
          }

        line[currentIndex].mPreprocessor = withinPreproc;
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
//{{{  actions
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

  int top = 1 + (int)ceil(scrollY / mCharAdvance.y);
  int bottom = (int)ceil((scrollY + height) / mCharAdvance.y);

  int left = (int)ceil(scrollX / mCharAdvance.x);
  int right = (int)ceil((scrollX + width) / mCharAdvance.x);

  sRowColumn pos = getActualCursorRowColumn();
  float len = getTextDistanceToLineStart (pos);

  if (pos.mRow < top)
    ImGui::SetScrollY (max (0.0f, (pos.mRow - 1) * mCharAdvance.y));
  if (pos.mRow > bottom - 4)
    ImGui::SetScrollY (max (0.0f, (pos.mRow + 4) * mCharAdvance.y - height));

  if (len + mTextStart < left + 4)
    ImGui::SetScrollX (max(0.0f, len + mTextStart - 4));
  if (len + mTextStart > right - 4)
    ImGui::SetScrollX (max(0.0f, len + mTextStart + 4 - width));
  }
//}}}
//{{{
void cTextEditor::advance (sRowColumn& rowColumn) const {

  if (rowColumn.mRow < (int)mLines.size()) {
    const vector<sGlyph>& glyphs = mLines[rowColumn.mRow].mGlyphs;
    int cindex = getCharacterIndex (rowColumn);
    if (cindex + 1 < (int)glyphs.size()) {
      auto delta = utf8CharLength (glyphs[cindex].mChar);
      cindex = min (cindex + delta, (int)glyphs.size() - 1);
      }
    else {
      ++rowColumn.mRow;
      cindex = 0;
      }

    rowColumn.mColumn = getCharacterColumn (rowColumn.mRow, cindex);
    }
  }
//}}}

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

//{{{
void cTextEditor::removeLine (int startRowColumn, int endRowColumn) {

  assert (!mReadOnly);
  assert (endRowColumn >= startRowColumn);
  assert (mLines.size() > (size_t)(endRowColumn - startRowColumn));

  map<int,string> etmp;
  for (auto& i : mMarkers) {
    map<int,string>::value_type e(i.first >= startRowColumn ? i.first - 1 : i.first, i.second);
    if (e.first >= startRowColumn && e.first <= endRowColumn)
      continue;
    etmp.insert(e);
    }
  mMarkers = move (etmp);

  mLines.erase (mLines.begin() + startRowColumn, mLines.begin() + endRowColumn);
  assert (!mLines.empty());

  mTextChanged = true;
  }
//}}}
//{{{
void cTextEditor::removeLine (int index) {

  assert(!mReadOnly);
  assert(mLines.size() > 1);

  map<int,string> etmp;
  for (auto& i : mMarkers) {
    map<int,string>::value_type e(i.first > index ? i.first - 1 : i.first, i.second);
    if (e.first - 1 == index)
      continue;
    etmp.insert(e);
    }
  mMarkers = move (etmp);

  mLines.erase (mLines.begin() + index);
  assert (!mLines.empty());

  mTextChanged = true;
  }
//}}}

//{{{
void cTextEditor::backspace() {

  assert(!mReadOnly);

  if (mLines.empty())
    return;

  sUndoRecord u;
  u.mBefore = mState;

  if (hasSelection()) {
    u.mRemoved = getSelectedText();
    u.mRemovedStart = mState.mSelectionStart;
    u.mRemovedEnd = mState.mSelectionEnd;
    deleteSelection();
    }

  else {
    sRowColumn pos = getActualCursorRowColumn();
    setCursorPosition (pos);

    if (mState.mCursorPosition.mColumn == 0) {
      if (mState.mCursorPosition.mRow == 0)
        return;

      u.mRemoved = '\n';
      u.mRemovedStart = u.mRemovedEnd = sRowColumn (pos.mRow - 1, getLineMaxColumn (pos.mRow - 1));
      advance (u.mRemovedEnd);

      vector<sGlyph>& line = mLines[mState.mCursorPosition.mRow].mGlyphs;
      vector<sGlyph>& prevLine = mLines[mState.mCursorPosition.mRow - 1].mGlyphs;
      int prevSize = getLineMaxColumn (mState.mCursorPosition.mRow - 1);
      prevLine.insert (prevLine.end(), line.begin(), line.end());

      map<int,string> etmp;
      for (auto& i : mMarkers)
        etmp.insert (map<int,string>::value_type (i.first - 1 == mState.mCursorPosition.mRow ? i.first - 1 : i.first, i.second));
      mMarkers = move (etmp);

      removeLine (mState.mCursorPosition.mRow);
      --mState.mCursorPosition.mRow;
      mState.mCursorPosition.mColumn = prevSize;
      }
    else {
      vector<sGlyph>& glyphs = mLines[mState.mCursorPosition.mRow].mGlyphs;
      int cindex = getCharacterIndex (pos) - 1;
      int cend = cindex + 1;
      while (cindex > 0 && IsUTFSequence (glyphs[cindex].mChar))
        --cindex;

      //if (cindex > 0 && UTF8CharLength(line[cindex].mChar) > 1)
      //  --cindex;                       glyphs

      u.mRemovedStart = u.mRemovedEnd = getActualCursorRowColumn();
      --u.mRemovedStart.mColumn;
      --mState.mCursorPosition.mColumn;

      while (cindex < (int)glyphs.size() && cend-- > cindex) {
        u.mRemoved += glyphs[cindex].mChar;
        glyphs.erase (glyphs.begin() + cindex);
        }
      }

    mTextChanged = true;

    ensureCursorVisible();
    colorize (mState.mCursorPosition.mRow, 1);
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
  colorize (mState.mSelectionStart.mRow, 1);
  }
//}}}
//{{{
void cTextEditor::deleteRange (const sRowColumn& startRowColumn, const sRowColumn& endRowColumn) {

  assert (endRowColumn >= startRowColumn);
  assert (!mReadOnly);

  //printf("D(%d.%d)-(%d.%d)\n", startRowColumn.mGlyphs, startRowColumn.mColumn, endRowColumn.mGlyphs, endRowColumn.mColumn);

  if (endRowColumn == startRowColumn)
    return;

  auto start = getCharacterIndex (startRowColumn);
  auto end = getCharacterIndex (endRowColumn);
  if (startRowColumn.mRow == endRowColumn.mRow) {
    vector<sGlyph>& glyphs = mLines[startRowColumn.mRow].mGlyphs;
    int  n = getLineMaxColumn (startRowColumn.mRow);
    if (endRowColumn.mColumn >= n)
      glyphs.erase (glyphs.begin() + start, glyphs.end());
    else
      glyphs.erase (glyphs.begin() + start, glyphs.begin() + end);
    }

  else {
    vector<sGlyph>& firstLine = mLines[startRowColumn.mRow].mGlyphs;
    vector<sGlyph>& lastLine = mLines[endRowColumn.mRow].mGlyphs;

    firstLine.erase(firstLine.begin() + start, firstLine.end());
    lastLine.erase (lastLine.begin(), lastLine.begin() + end);

    if (startRowColumn.mRow < endRowColumn.mRow)
      firstLine.insert (firstLine.end(), lastLine.begin(), lastLine.end());

    if (startRowColumn.mRow < endRowColumn.mRow)
      removeLine (startRowColumn.mRow + 1, endRowColumn.mRow + 1);
    }

  mTextChanged = true;
  }
//}}}

//{{{
void cTextEditor::enterCharacter (ImWchar aChar, bool shift) {

  assert (!mReadOnly);

  sUndoRecord u;
  u.mBefore = mState;

  if (hasSelection()) {
    if (aChar == '\t' && mState.mSelectionStart.mRow != mState.mSelectionEnd.mRow) {
      auto start = mState.mSelectionStart;
      auto end = mState.mSelectionEnd;
      auto originalEnd = end;
      if (start > end)
        swap(start, end);
      start.mColumn = 0;
      // end.mColumn = end.mGlyphs < mLines.size() ? mLines[end.mGlyphs].size() : 0;
      if (end.mColumn == 0 && end.mRow > 0)
        --end.mRow;
      if (end.mRow >= (int)mLines.size())
        end.mRow = mLines.empty() ? 0 : (int)mLines.size() - 1;
      end.mColumn = getLineMaxColumn (end.mRow);

      //if (end.mColumn >= getLineMaxColumn(end.mGlyphs))
      //  end.mColumn = getLineMaxColumn(end.mGlyphs) - 1;
      u.mRemovedStart = start;
      u.mRemovedEnd = end;
      u.mRemoved = getText (start, end);

      bool modified = false;

      for (int i = start.mRow; i <= end.mRow; i++) {
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
        start = sRowColumn (start.mRow, getCharacterColumn (start.mRow, 0));
        sRowColumn rangeEnd;
        if (originalEnd.mColumn != 0) {
          end = sRowColumn (end.mRow, getLineMaxColumn (end.mRow));
          rangeEnd = end;
          u.mAdded = getText (start, end);
          }
        else {
          end = sRowColumn (originalEnd.mRow, 0);
          rangeEnd = sRowColumn (end.mRow - 1, getLineMaxColumn (end.mRow - 1));
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

    else {
      u.mRemoved = getSelectedText();
      u.mRemovedStart = mState.mSelectionStart;
      u.mRemovedEnd = mState.mSelectionEnd;
      deleteSelection();
      }
    } // HasSelection

  auto coord = getActualCursorRowColumn();
  u.mAddedStart = coord;

  assert (!mLines.empty());
  if (aChar == '\n') {
    insertLine (coord.mRow + 1);
    vector<sGlyph>& glyphs = mLines[coord.mRow].mGlyphs;
    vector<sGlyph>& newLine = mLines[coord.mRow + 1].mGlyphs;

    if (mLanguage.mAutoIndentation)
      for (size_t it = 0; it < glyphs.size() && isascii (glyphs[it].mChar) && isblank (glyphs[it].mChar); ++it)
        newLine.push_back (glyphs[it]);

    const size_t whitespaceSize = newLine.size();
    auto cindex = getCharacterIndex (coord);
    newLine.insert (newLine.end(), glyphs.begin() + cindex, glyphs.end());
    glyphs.erase (glyphs.begin() + cindex, glyphs.begin() + glyphs.size());
    setCursorPosition (sRowColumn (coord.mRow + 1, getCharacterColumn (coord.mRow + 1, (int)whitespaceSize)));
    u.mAdded = (char)aChar;
    }

  else {
    char buf[7];
    int e = ImTextCharToUtf8(buf, 7, aChar);
    if (e > 0) {
      buf[e] = '\0';
      vector<sGlyph>& glyphs = mLines[coord.mRow].mGlyphs;
      int cindex = getCharacterIndex (coord);

      if (mOverwrite && cindex < (int)glyphs.size()) {
        auto d = utf8CharLength (glyphs[cindex].mChar);
        u.mRemovedStart = mState.mCursorPosition;
        u.mRemovedEnd = sRowColumn(coord.mRow, getCharacterColumn (coord.mRow, cindex + d));
        while (d-- > 0 && cindex < (int)glyphs.size()) {
          u.mRemoved += glyphs[cindex].mChar;
          glyphs.erase (glyphs.begin() + cindex);
          }
        }

      for (auto p = buf; *p != '\0'; p++, ++cindex)
        glyphs.insert (glyphs.begin() + cindex, sGlyph (*p, ePalette::Default));
      u.mAdded = buf;

      setCursorPosition (sRowColumn(coord.mRow, getCharacterColumn (coord.mRow, cindex)));
      }
    else
      return;
    }

  mTextChanged = true;

  u.mAddedEnd = getActualCursorRowColumn();
  u.mAfter = mState;

  addUndo(u);

  colorize (coord.mRow - 1, 3);
  ensureCursorVisible();
  }
//}}}
//{{{
int cTextEditor::insertTextAt (sRowColumn& /* inout */ where, const char* value) {

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
      if (cindex < (int)mLines[where.mRow].mGlyphs.size()) {
        vector<sGlyph>& newLine = insertLine (where.mRow + 1);
        vector<sGlyph>& line = mLines[where.mRow].mGlyphs;
        newLine.insert (newLine.begin(), line.begin() + cindex, line.end());
        line.erase (line.begin() + cindex, line.end());
        }
      else
        insertLine (where.mRow + 1);

      ++where.mRow;
      where.mColumn = 0;
      cindex = 0;
      ++totalLines;
      ++value;
      }

    else {
      vector<sGlyph>& glyphs = mLines[where.mRow].mGlyphs;
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
vector<cTextEditor::sGlyph>& cTextEditor::insertLine (int index) {

  assert (!mReadOnly);

  sLine& result = *mLines.insert (mLines.begin() + index, vector<sGlyph>());

  map<int,string> etmp;
  for (auto& i : mMarkers)
    etmp.insert (map<int,string>::value_type (i.first >= index ? i.first + 1 : i.first, i.second));
  mMarkers = move (etmp);

  return result.mGlyphs;
  }
//}}}
//}}}

//{{{
void cTextEditor::handleMouseInputs() {

  ImGuiIO& io = ImGui::GetIO();

  auto shift = io.KeyShift;
  auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

  if (ImGui::IsWindowHovered()) {
    if (!shift && !alt) {
      auto click = ImGui::IsMouseClicked(0);
      auto doubleClick = ImGui::IsMouseDoubleClicked (0);
      auto t = ImGui::GetTime();
      auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

      // Left mouse button triple click
      if (tripleClick) {
        if (!ctrl) {
          mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = screenPosToRowColumn (ImGui::GetMousePos());
          mSelection = eSelection::Line;
          setSelection (mInteractiveStart, mInteractiveEnd, mSelection);
          }
        mLastClick = -1.0f;
        }

      // left mouse button double click
      else if (doubleClick) {
        if (!ctrl) {
          mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = screenPosToRowColumn(ImGui::GetMousePos());
          if (mSelection == eSelection::Line)
            mSelection = eSelection::Normal;
          else
            mSelection = eSelection::Word;
          setSelection (mInteractiveStart, mInteractiveEnd, mSelection);
          }
        mLastClick = (float)ImGui::GetTime();
        }

      // left mouse button click
      else if (click) {
        mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = screenPosToRowColumn(ImGui::GetMousePos());
        if (ctrl)
          mSelection = eSelection::Word;
        else
          mSelection = eSelection::Normal;
        setSelection (mInteractiveStart, mInteractiveEnd, mSelection);
        mLastClick = (float)ImGui::GetTime();
        }

      // left mouse button dragging (=> update selection)
      else if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0)) {
        io.WantCaptureMouse = true;
        mState.mCursorPosition = mInteractiveEnd = screenPosToRowColumn (ImGui::GetMousePos());
        setSelection (mInteractiveStart, mInteractiveEnd, mSelection);
        }
      }
    }
  }
//}}}
//{{{
void cTextEditor::handleKeyboardInputs() {

  ImGuiIO& io = ImGui::GetIO();
  auto shift = io.KeyShift;
  auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

  if (ImGui::IsWindowFocused()) {
    if (ImGui::IsWindowHovered())
      ImGui::SetMouseCursor (ImGuiMouseCursor_TextInput);
    //ImGui::CaptureKeyboardFromApp(true);

    io.WantCaptureKeyboard = true;
    io.WantTextInput = true;

    if (!isReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
      undo();
    else if (!isReadOnly() && !ctrl && !shift && alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Backspace)))
      undo();
    else if (!isReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Y)))
      redo();
    else if (!ctrl && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_UpArrow)))
      moveUp (1, shift);
    else if (!ctrl && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_DownArrow)))
      moveDown (1, shift);
    else if (!alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_LeftArrow)))
      moveLeft (1, shift, ctrl);
    else if (!alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_RightArrow)))
      moveRight (1, shift, ctrl);
    else if (!alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_PageUp)))
      moveUp (getPageSize() - 4, shift);
    else if (!alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_PageDown)))
      moveDown (getPageSize() - 4, shift);
    else if (!alt && ctrl && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Home)))
      moveTop (shift);
    else if (ctrl && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_End)))
      moveBottom (shift);
    else if (!ctrl && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Home)))
      moveHome (shift);
    else if (!ctrl && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_End)))
      moveEnd (shift);
    else if (!isReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Delete)))
      deleteIt();
    else if (!isReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Backspace)))
      backspace();
    else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex (ImGuiKey_Insert)))
      mOverwrite ^= true;
    else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex (ImGuiKey_Insert)))
      copy();
    else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex (ImGuiKey_C)))
      copy();
    else if (!isReadOnly() && !ctrl && shift && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Insert)))
      paste();
    else if (!isReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_V)))
      paste();
    else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X)))
      cut();
    else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex (ImGuiKey_Delete)))
      cut();
    else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex (ImGuiKey_A)))
      selectAll();
    else if (!isReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Enter)))
      enterCharacter ('\n', false);
    else if (!isReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_Tab)))
      enterCharacter ('\t', shift);

    if (!isReadOnly() && !io.InputQueueCharacters.empty()) {
      for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
        auto c = io.InputQueueCharacters[i];
        if (c != 0 && (c == '\n' || c >= 32))
          enterCharacter (c, shift);
        }
      io.InputQueueCharacters.resize (0);
      }
    }
  }
//}}}

//{{{
void cTextEditor::render() {

  //{{{  calc mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)
  const float fontSize = ImGui::GetFont()->CalcTextSizeA (
    ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;

  mCharAdvance = ImVec2 (fontSize, ImGui::GetTextLineHeightWithSpacing() * mLineSpacing);
  //}}}
  //{{{  update palette with the current alpha from style
  for (int i = 0; i < (int)ePalette::Max; ++i) {
    ImVec4 color = ImGui::ColorConvertU32ToFloat4 (mPaletteBase[i]);
    color.w *= ImGui::GetStyle().Alpha;
    mPalette[i] = ImGui::ColorConvertFloat4ToU32 (color);
    }
  //}}}

  assert (mLineBuffer.empty());
  ImVec2 contentSize = ImGui::GetWindowContentRegionMax();
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  float longest (mTextStart);

  if (mScrollToTop) {
    //{{{  scroll to top
    mScrollToTop = false;
    ImGui::SetScrollY (0.f);
    }
    //}}}

  ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
  float scrollX = ImGui::GetScrollX();
  float scrollY = ImGui::GetScrollY();
  int lineNumber = (int)floor (scrollY / mCharAdvance.y);
  int globalLineMax = (int)mLines.size();
  int lineMax = max (0,
    min ((int)mLines.size() - 1, lineNumber + (int)floor((scrollY + contentSize.y) / mCharAdvance.y)));

  //{{{  calc mTextStart by evaluating mLines size (global lineMax) plus two spaces as text width
  char buf[16];
  snprintf (buf, 16, " %d ", globalLineMax);

  mTextStart = ImGui::GetFont()->CalcTextSizeA (
    ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x + mLeftMargin;
  //}}}

  if (!mLines.empty()) {
    float spaceSize = ImGui::GetFont()->CalcTextSizeA (
      ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

    while (lineNumber <= lineMax) {
      ImVec2 lineStartScreenPos = ImVec2 (cursorScreenPos.x, cursorScreenPos.y + lineNumber * mCharAdvance.y);
      ImVec2 textScreenPos = ImVec2 (lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);

      vector<sGlyph>& glyphs = mLines[lineNumber].mGlyphs;
      longest = max(mTextStart + getTextDistanceToLineStart (sRowColumn (lineNumber, getLineMaxColumn (lineNumber))), longest);
      int columnNo = 0;
      sRowColumn lineStartCoord (lineNumber, 0);
      sRowColumn lineEndCoord (lineNumber, getLineMaxColumn (lineNumber));
      //{{{  draw selection for the current line
      float sstart = -1.0f;
      float ssend = -1.0f;

      assert (mState.mSelectionStart <= mState.mSelectionEnd);
      if (mState.mSelectionStart <= lineEndCoord)
        sstart = mState.mSelectionStart > lineStartCoord ? getTextDistanceToLineStart (mState.mSelectionStart) : 0.0f;
      if (mState.mSelectionEnd > lineStartCoord)
        ssend = getTextDistanceToLineStart (mState.mSelectionEnd < lineEndCoord ? mState.mSelectionEnd : lineEndCoord);

      if (mState.mSelectionEnd.mRow > lineNumber)
        ssend += mCharAdvance.x;

      if (sstart != -1 && ssend != -1 && sstart < ssend) {
        ImVec2 vstart (lineStartScreenPos.x + mTextStart + sstart, lineStartScreenPos.y);
        ImVec2 vend (lineStartScreenPos.x + mTextStart + ssend, lineStartScreenPos.y + mCharAdvance.y);
        drawList->AddRectFilled (vstart, vend, mPalette[(int)ePalette::Selection]);
        }
      //}}}
      //{{{  draw error markers
      ImVec2 start = ImVec2 (lineStartScreenPos.x + scrollX, lineStartScreenPos.y);
      auto errorIt = mMarkers.find (lineNumber + 1);
      if (errorIt != mMarkers.end()) {
        ImVec2 end = ImVec2 (lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + mCharAdvance.y);
        drawList->AddRectFilled (start, end, mPalette[(int)ePalette::Marker]);

        if (ImGui::IsMouseHoveringRect (lineStartScreenPos, end)) {
          ImGui::BeginTooltip();

          ImGui::PushStyleColor( ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
          ImGui::Text ("Error at line %d:", errorIt->first);
          ImGui::PopStyleColor();

          ImGui::Separator();
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
          ImGui::Text ("%s", errorIt->second.c_str());
          ImGui::PopStyleColor();

          ImGui::EndTooltip();
          }
        }
      //}}}
      //{{{  draw lineNumber rightAligned
      snprintf (buf, 16, "%d  ", lineNumber + 1);

      float lineNumberWidth = ImGui::GetFont()->CalcTextSizeA (
        ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x;

      drawList->AddText (ImVec2 (lineStartScreenPos.x + mTextStart - lineNumberWidth, lineStartScreenPos.y),
                         mPalette[(int)ePalette::LineNumber], buf);

      if (mState.mCursorPosition.mRow == lineNumber) {
        auto focused = ImGui::IsWindowFocused();

        // highlight the current line (where the cursor is)
        if (!hasSelection()) {
          ImVec2 end = ImVec2 (start.x + contentSize.x + scrollX, start.y + mCharAdvance.y);
          drawList->AddRectFilled (start, end,
            mPalette[(int)(focused ? ePalette::CurrentLineFill : ePalette::CurrentLineFillInactive)]);
          drawList->AddRect (start, end, mPalette[(int)ePalette::CurrentLineEdge], 1.0f);
          }

        // render cursor
        if (focused) {
          auto timeEnd = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
          auto elapsed = timeEnd - mStartTime;
          if (elapsed > 400) {
            float width = 1.0f;
            int cindex = getCharacterIndex (mState.mCursorPosition);
            float cx = getTextDistanceToLineStart (mState.mCursorPosition);

            if (mOverwrite && cindex < (int)glyphs.size()) {
              uint8_t c = glyphs[cindex].mChar;
              if (c == '\t') {
                float x = (1.0f + floor((1.0f + cx) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
                width = x - cx;
                }
              else {
                char buf2[2];
                buf2[0] = glyphs[cindex].mChar;
                buf2[1] = '\0';
                width = ImGui::GetFont()->CalcTextSizeA (ImGui::GetFontSize(), FLT_MAX, -1.0f, buf2).x;
                }
              }

            ImVec2 cstart (textScreenPos.x + cx, lineStartScreenPos.y);
            ImVec2 cend (textScreenPos.x + cx + width, lineStartScreenPos.y + mCharAdvance.y);
            drawList->AddRectFilled (cstart, cend, mPalette[(int)ePalette::Cursor]);
            if (elapsed > 800)
              mStartTime = timeEnd;
            }
          }
        }
      //}}}
      //{{{  render colored text
      ImU32 prevColor = glyphs.empty() ? mPalette[(int)ePalette::Default] : getGlyphColor (glyphs[0]);
      ImVec2 bufferOffset;

      for (int i = 0; i < (int)glyphs.size();) {
        sGlyph& glyph = glyphs[i];
        ImU32 color = getGlyphColor (glyph);

        if ((color != prevColor || glyph.mChar == '\t' || glyph.mChar == ' ') && !mLineBuffer.empty()) {
          const ImVec2 newOffset (textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
          drawList->AddText (newOffset, prevColor, mLineBuffer.c_str());
          ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA (
            ImGui::GetFontSize(), FLT_MAX, -1.0f, mLineBuffer.c_str(), nullptr, nullptr);
          bufferOffset.x += textSize.x;
          mLineBuffer.clear();
          }
        prevColor = color;

        if (glyph.mChar == '\t') {
          float oldX = bufferOffset.x;
          bufferOffset.x = (1.0f + floor ((1.0f + bufferOffset.x) / (float(mTabSize) * spaceSize))) *
                           (float(mTabSize) * spaceSize);
          ++i;

          if (mShowWhitespaces) {
            const float s = ImGui::GetFontSize();
            const float x1 = textScreenPos.x + oldX + 1.0f;
            const float x2 = textScreenPos.x + bufferOffset.x - 1.0f;
            const float y = textScreenPos.y + bufferOffset.y + s * 0.5f;
            const ImVec2 p1(x1, y);
            const ImVec2 p2(x2, y);
            const ImVec2 p3(x2 - s * 0.2f, y - s * 0.2f);
            const ImVec2 p4(x2 - s * 0.2f, y + s * 0.2f);
            drawList->AddLine (p1, p2, 0x90909090);
            drawList->AddLine (p2, p3, 0x90909090);
            drawList->AddLine (p2, p4, 0x90909090);
            }
          }
        else if (glyph.mChar == ' ') {
          if (mShowWhitespaces) {
            const float s = ImGui::GetFontSize();
            const float x = textScreenPos.x + bufferOffset.x + spaceSize * 0.5f;
            const float y = textScreenPos.y + bufferOffset.y + s * 0.5f;
            drawList->AddCircleFilled (ImVec2(x, y), 1.5f, 0x80808080, 4);
            }
          bufferOffset.x += spaceSize;
          i++;
          }

        else {
          int l = utf8CharLength (glyph.mChar);
          while (l-- > 0)
            mLineBuffer.push_back (glyphs[i++].mChar);
          }

        ++columnNo;
        }

      if (!mLineBuffer.empty()) {
        const ImVec2 newOffset (textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
        drawList->AddText (newOffset, prevColor, mLineBuffer.c_str());
        mLineBuffer.clear();
        }
      //}}}
      ++lineNumber;
      }
    //{{{  draw tooltip on idents/preprocessor symbols
    if (ImGui::IsMousePosValid()) {
      string id = getWordAt (screenPosToRowColumn (ImGui::GetMousePos()));
      if (!id.empty()) {
        auto it = mLanguage.mIdents.find (id);
        if (it != mLanguage.mIdents.end()) {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted (it->second.mDeclaration.c_str());
          ImGui::EndTooltip();
          }

        else {
          auto pi = mLanguage.mPreprocIdents.find (id);
          if (pi != mLanguage.mPreprocIdents.end()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted (pi->second.mDeclaration.c_str());
            ImGui::EndTooltip();
            }
          }
        }
      }
    //}}}
    }

  ImGui::Dummy (ImVec2 ((longest + 2), mLines.size() * mCharAdvance.y));

  if (mScrollToCursor) {
    //{{{  scroll to cursor
    ensureCursorVisible();
    ImGui::SetWindowFocus();
    mScrollToCursor = false;
    }
    //}}}
  }
//}}}
