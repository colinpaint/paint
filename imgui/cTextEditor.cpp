// cTextEditor.cpp - nicked from https://github.com/BalazsJako/ImGuiColorTextEdit
//{{{  includes
#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>

#include "cTextEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h" // for imGui::GetCurrentWindow()

#include "../utils/cLog.h"

using namespace std;
//}}}
//{{{  template bool equals
template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals (InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, BinaryPredicate p) {
  for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
    if (!p(*first1, *first2))
      return false;
    }

  return first1 == last1 && first2 == last2;
  }
//}}}

namespace {
  //{{{
  const array <ImU32, (unsigned)cTextEditor::ePaletteIndex::Max> kDarkPalette = {
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
    0xff206020, // Comment (single line)
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
  const array <ImU32, (unsigned)cTextEditor::ePaletteIndex::Max> kLightPalette = {
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
  bool IsUTFSequence (char c) {
    return (c & 0xC0) == 0x80;
    }
  //}}}
  //{{{
  int utf8CharLength (uint8_t c) {
  // https://en.wikipedia.org/wiki/UTF-8
  // We assume that the char is a standalone character (<128)
  // or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)

    if ((c & 0xFE) == 0xFC)
      return 6;
    if ((c & 0xFC) == 0xF8)
      return 5;
    if ((c & 0xF8) == 0xF0)
      return 4;
    else if ((c & 0xF0) == 0xE0)
      return 3;
    else if ((c & 0xE0) == 0xC0)
      return 2;

    return 1;
    }
  //}}}
  //{{{
  int ImTextCharToUtf8 (char* buf, int buf_size, unsigned int c) {
  // "Borrowed" from ImGui source

    if (c < 0x80) {
      buf[0] = (char)c;
      return 1;
      }

    if (c < 0x800) {
      if (buf_size < 2)
        return 0;
      buf[0] = (char)(0xc0 + (c >> 6));
      buf[1] = (char)(0x80 + (c & 0x3f));
      return 2;
      }

    if (c >= 0xdc00 && c < 0xe000)
      return 0;

    if (c >= 0xd800 && c < 0xdc00) {
      if (buf_size < 4) return 0;
      buf[0] = (char)(0xf0 + (c >> 18));
      buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
      buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
      buf[3] = (char)(0x80 + ((c) & 0x3f));
      return 4;
      }

    //else if (c < 0x10000)
      {
      if (buf_size < 3) return 0;
      buf[0] = (char)(0xe0 + (c >> 12));
      buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
      buf[2] = (char)(0x80 + ((c) & 0x3f));
      return 3;
      }
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
cTextEditor::sUndoRecord::sUndoRecord (
  const string& aAdded, const cTextEditor::sRowColumn aAddedStart, const cTextEditor::sRowColumn aAddedEnd,
  const string& aRemoved, const cTextEditor::sRowColumn aRemovedStart, const cTextEditor::sRowColumn aRemovedEnd,
  cTextEditor::sEditorState& aBefore, cTextEditor::sEditorState& aAfter)
    : mAdded(aAdded), mAddedStart(aAddedStart), mAddedEnd(aAddedEnd),
      mRemoved(aRemoved), mRemovedStart(aRemovedStart), mRemovedEnd(aRemovedEnd),
      mBefore(aBefore), mAfter(aAfter) {

  assert (mAddedStart <= mAddedEnd);
  assert (mRemovedStart <= mRemovedEnd);
  }
//}}}
//{{{
void cTextEditor::sUndoRecord::Undo (cTextEditor* editor) {

  if (!mAdded.empty()) {
    editor->deleteRange (mAddedStart, mAddedEnd);
    editor->colorize (mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 2);
    }

  if (!mRemoved.empty()) {
    auto start = mRemovedStart;
    editor->insertTextAt (start, mRemoved.c_str());
    editor->colorize (mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 2);
    }

  editor->mState = mBefore;
  editor->ensureCursorVisible();
  }
//}}}
//{{{
void cTextEditor::sUndoRecord::Redo (cTextEditor* editor) {

  if (!mRemoved.empty()) {
    editor->deleteRange (mRemovedStart, mRemovedEnd);
    editor->colorize (mRemovedStart.mLine - 1, mRemovedEnd.mLine - mRemovedStart.mLine + 1);
    }

  if (!mAdded.empty()) {
    auto start = mAddedStart;
    editor->insertTextAt (start, mAdded.c_str());
    editor->colorize (mAddedStart.mLine - 1, mAddedEnd.mLine - mAddedStart.mLine + 1);
    }

  editor->mState = mAfter;
  editor->ensureCursorVisible();
  }
//}}}

// cTextEditor::sLanguage
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::CPlusPlus() {

  static bool inited = false;
  static sLanguage language;

  if (!inited) {
    //{{{
    static const char* const cppKeywords[] = {
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
    for (auto& k : cppKeywords)
      language.mKeywords.insert (k);

    //{{{
    static const char* const idents[] = {
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
    for (auto& k : idents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair (string (k), id));
      }

    language.mTokenize = [](const char * inBegin, const char * inEnd,
                           const char *& outBegin, const char *& outEnd, ePaletteIndex & paletteIndex) -> bool {
      paletteIndex = ePaletteIndex::Max;

      while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
        inBegin++;

      if (inBegin == inEnd) {
        outBegin = inEnd;
        outEnd = inEnd;
        paletteIndex = ePaletteIndex::Default;
        }
      else if (findString (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::String;
      else if (findCharacterLiteral (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::CharLiteral;
      else if (findIdent (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::Ident;
      else if (findNumber (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::Number;
      else if (findPunctuation (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::Punctuation;

      return paletteIndex != ePaletteIndex::Max;
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

  static bool inited = false;
  static sLanguage language;

  if (!inited) {
    //{{{
    static const char* const keywords[] = {
      "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
      "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
      "_Noreturn", "_Static_assert", "_Thread_local"
    };
    //}}}
    for (auto& k : keywords)
      language.mKeywords.insert(k);

    //{{{
    static const char* const idents[] = {
      "abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
      "ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
    };
    //}}}
    for (auto& k : idents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair (string(k), id));
      }

    language.mTokenize = [](const char * inBegin, const char * inEnd,
                           const char *& outBegin, const char *& outEnd, ePaletteIndex & paletteIndex) -> bool {
      paletteIndex = ePaletteIndex::Max;

      while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
        inBegin++;

      if (inBegin == inEnd) {
        outBegin = inEnd;
        outEnd = inEnd;
        paletteIndex = ePaletteIndex::Default;
        }
      else if (findString (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::String;
      else if (findCharacterLiteral (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::CharLiteral;
      else if (findIdent (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::Ident;
      else if (findNumber (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::Number;
      else if (findPunctuation (inBegin, inEnd, outBegin, outEnd))
        paletteIndex = ePaletteIndex::Punctuation;

      return paletteIndex != ePaletteIndex::Max;
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
    //{{{
    static const char* const keywords[] = {
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
    for (auto& k : keywords)
      language.mKeywords.insert(k);

    //{{{
    static const char* const idents[] = {
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
    for (auto& k : idents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert(make_pair(string(k), id));
      }

    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", ePaletteIndex::Preprocessor));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", ePaletteIndex::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("\\'\\\\?[^\\']\\'", ePaletteIndex::CharLiteral));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", ePaletteIndex::Ident));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ePaletteIndex::Punctuation));

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
    //{{{
    static const char* const keywords[] = {
      "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
      "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
      "_Noreturn", "_Static_assert", "_Thread_local"
    };
    //}}}
    for (auto& k : keywords)
      language.mKeywords.insert(k);

    //{{{
    static const char* const idents[] = {
      "abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
      "ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
    };
    //}}}
    for (auto& k : idents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert(make_pair(string(k), id));
      }

    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", ePaletteIndex::Preprocessor));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", ePaletteIndex::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("\\'\\\\?[^\\']\\'",ePaletteIndex::CharLiteral));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", ePaletteIndex::Ident));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ePaletteIndex::Punctuation));

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
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::AngelScript() {

  static bool inited = false;
  static sLanguage language;

  if (!inited) {
    //{{{
    static const char* const keywords[] = {
      "and", "abstract", "auto", "bool", "break", "case", "cast", "class", "const", "continue", "default", "do", "double", "else", "enum", "false", "final", "float", "for",
      "from", "funcdef", "function", "get", "if", "import", "in", "inout", "int", "interface", "int8", "int16", "int32", "int64", "is", "mixin", "namespace", "not",
      "null", "or", "out", "override", "private", "protected", "return", "set", "shared", "super", "switch", "this ", "true", "typedef", "uint", "uint8", "uint16", "uint32",
      "uint64", "void", "while", "xor"
    };
    //}}}

    for (auto& k : keywords)
      language.mKeywords.insert(k);

    //{{{
    static const char* const idents[] = {
      "cos", "sin", "tab", "acos", "asin", "atan", "atan2", "cosh", "sinh", "tanh", "log", "log10", "pow", "sqrt", "abs", "ceil", "floor", "fraction", "closeTo", "fpFromIEEE", "fpToIEEE",
      "complex", "opEquals", "opAddAssign", "opSubAssign", "opMulAssign", "opDivAssign", "opAdd", "opSub", "opMul", "opDiv"
    };
    //}}}
    for (auto& k : idents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair (string(k), id));
      }

    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", ePaletteIndex::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("\\'\\\\?[^\\']\\'", ePaletteIndex::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", ePaletteIndex::Ident));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ePaletteIndex::Punctuation));

    language.mCommentStart = "/*";
    language.mCommentEnd = "*/";
    language.mSingleLineComment = "//";
    language.mCaseSensitive = true;
    language.mAutoIndentation = true;
    language.mName = "AngelScript";

    inited = true;
    }

  return language;
  }
//}}}
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::Lua() {

  static bool inited = false;
  static sLanguage language;

  if (!inited) {
    //{{{
    static const char* const keywords[] = {
      "and", "break", "do", "", "else", "elseif", "end", "false", "for", "function", "if", "in", "", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"
    };
    //}}}

    for (auto& k : keywords)
      language.mKeywords.insert(k);

    //{{{
    static const char* const idents[] = {
      "assert", "collectgarbage", "dofile", "error", "getmetatable", "ipairs", "loadfile", "load", "loadstring",  "next",  "pairs",  "pcall",  "print",  "rawequal",  "rawlen",  "rawget",  "rawset",
      "select",  "setmetatable",  "tonumber",  "tostring",  "type",  "xpcall",  "_G",  "_VERSION","arshift", "band", "bnot", "bor", "bxor", "btest", "extract", "lrotate", "lshift", "replace",
      "rrotate", "rshift", "create", "resume", "running", "status", "wrap", "yield", "isyieldable", "debug","getuservalue", "gethook", "getinfo", "getlocal", "getregistry", "getmetatable",
      "getupvalue", "upvaluejoin", "upvalueid", "setuservalue", "sethook", "setlocal", "setmetatable", "setupvalue", "traceback", "close", "flush", "input", "lines", "open", "output", "popen",
      "read", "tmpfile", "type", "write", "close", "flush", "lines", "read", "seek", "setvbuf", "write", "__gc", "__tostring", "abs", "acos", "asin", "atan", "ceil", "cos", "deg", "exp", "tointeger",
      "floor", "fmod", "ult", "log", "max", "min", "modf", "rad", "random", "randomseed", "sin", "sqrt", "string", "tan", "type", "atan2", "cosh", "sinh", "tanh",
      "pow", "frexp", "ldexp", "log10", "pi", "huge", "maxinteger", "mininteger", "loadlib", "searchpath", "seeall", "preload", "cpath", "path", "searchers", "loaded", "module", "require", "clock",
      "date", "difftime", "execute", "exit", "getenv", "remove", "rename", "setlocale", "time", "tmpname", "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len", "lower", "match", "rep",
      "reverse", "sub", "upper", "pack", "packsize", "unpack", "concat", "maxn", "insert", "pack", "unpack", "remove", "move", "sort", "offset", "codepoint", "char", "len", "codes", "charpattern",
      "coroutine", "table", "io", "os", "string", "utf8", "bit32", "math", "debug", "package"
    };
    //}}}

    for (auto& k : idents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair(string(k), id));
      }

    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex> ("L?\\\"(\\\\.|[^\\\"])*\\\"", ePaletteIndex::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex> ("\\\'[^\\\']*\\\'", ePaletteIndex::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex> ("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex> ("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex> ("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex> ("[a-zA-Z_][a-zA-Z0-9_]*", ePaletteIndex::Ident));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex> ("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ePaletteIndex::Punctuation));

    language.mCommentStart = "--[[";
    language.mCommentEnd = "]]";
    language.mSingleLineComment = "--";
    language.mCaseSensitive = true;
    language.mAutoIndentation = false;
    language.mName = "Lua";

    inited = true;
    }

  return language;
  }
//}}}
//{{{
const cTextEditor::sLanguage& cTextEditor::sLanguage::SQL() {

  static bool inited = false;
  static sLanguage language;

  if (!inited) {
    //{{{
    static const char* const keywords[] = {
      "ADD", "EXCEPT", "PERCENT", "ALL", "EXEC", "PLAN", "ALTER", "EXECUTE", "PRECISION", "AND", "EXISTS", "PRIMARY", "ANY", "EXIT", "PRINT", "AS", "FETCH", "PROC", "ASC", "FILE", "PROCEDURE",
      "AUTHORIZATION", "FILLFACTOR", "PUBLIC", "BACKUP", "FOR", "RAISERROR", "BEGIN", "FOREIGN", "READ", "BETWEEN", "FREETEXT", "READTEXT", "BREAK", "FREETEXTTABLE", "RECONFIGURE",
      "BROWSE", "FROM", "REFERENCES", "BULK", "FULL", "REPLICATION", "BY", "FUNCTION", "RESTORE", "CASCADE", "GOTO", "RESTRICT", "CASE", "GRANT", "RETURN", "CHECK", "GROUP", "REVOKE",
      "CHECKPOINT", "HAVING", "RIGHT", "CLOSE", "HOLDLOCK", "ROLLBACK", "CLUSTERED", "IDENTITY", "ROWCOUNT", "COALESCE", "IDENTITY_INSERT", "ROWGUIDCOL", "COLLATE", "IDENTITYCOL", "RULE",
      "COLUMN", "IF", "SAVE", "COMMIT", "IN", "SCHEMA", "COMPUTE", "INDEX", "SELECT", "CONSTRAINT", "INNER", "SESSION_USER", "CONTAINS", "INSERT", "SET", "CONTAINSTABLE", "INTERSECT", "SETUSER",
      "CONTINUE", "INTO", "SHUTDOWN", "CONVERT", "IS", "SOME", "CREATE", "JOIN", "STATISTICS", "CROSS", "KEY", "SYSTEM_USER", "CURRENT", "KILL", "TABLE", "CURRENT_DATE", "LEFT", "TEXTSIZE",
      "CURRENT_TIME", "LIKE", "THEN", "CURRENT_TIMESTAMP", "LINENO", "TO", "CURRENT_USER", "LOAD", "TOP", "CURSOR", "NATIONAL", "TRAN", "DATABASE", "NOCHECK", "TRANSACTION",
      "DBCC", "NONCLUSTERED", "TRIGGER", "DEALLOCATE", "NOT", "TRUNCATE", "DECLARE", "NULL", "TSEQUAL", "DEFAULT", "NULLIF", "UNION", "DELETE", "OF", "UNIQUE", "DENY", "OFF", "UPDATE",
      "DESC", "OFFSETS", "UPDATETEXT", "DISK", "ON", "USE", "DISTINCT", "OPEN", "USER", "DISTRIBUTED", "OPENDATASOURCE", "VALUES", "DOUBLE", "OPENQUERY", "VARYING","DROP", "OPENROWSET", "VIEW",
      "DUMMY", "OPENXML", "WAITFOR", "DUMP", "OPTION", "WHEN", "ELSE", "OR", "WHERE", "END", "ORDER", "WHILE", "ERRLVL", "OUTER", "WITH", "ESCAPE", "OVER", "WRITETEXT"
    };
    //}}}

    for (auto& k : keywords)
      language.mKeywords.insert(k);

    //{{{
    static const char* const idents[] = {
      "ABS",  "ACOS",  "ADD_MONTHS",  "ASCII",  "ASCIISTR",  "ASIN",  "ATAN",  "ATAN2",  "AVG",  "BFILENAME",  "BIN_TO_NUM",  "BITAND",  "CARDINALITY",  "CASE",  "CAST",  "CEIL",
      "CHARTOROWID",  "CHR",  "COALESCE",  "COMPOSE",  "CONCAT",  "CONVERT",  "CORR",  "COS",  "COSH",  "COUNT",  "COVAR_POP",  "COVAR_SAMP",  "CUME_DIST",  "CURRENT_DATE",
      "CURRENT_TIMESTAMP",  "DBTIMEZONE",  "DECODE",  "DECOMPOSE",  "DENSE_RANK",  "DUMP",  "EMPTY_BLOB",  "EMPTY_CLOB",  "EXP",  "EXTRACT",  "FIRST_VALUE",  "FLOOR",  "FROM_TZ",  "GREATEST",
      "GROUP_ID",  "HEXTORAW",  "INITCAP",  "INSTR",  "INSTR2",  "INSTR4",  "INSTRB",  "INSTRC",  "LAG",  "LAST_DAY",  "LAST_VALUE",  "LEAD",  "LEAST",  "LENGTH",  "LENGTH2",  "LENGTH4",
      "LENGTHB",  "LENGTHC",  "LISTAGG",  "LN",  "LNNVL",  "LOCALTIMESTAMP",  "LOG",  "LOWER",  "LPAD",  "LTRIM",  "MAX",  "MEDIAN",  "MIN",  "MOD",  "MONTHS_BETWEEN",  "NANVL",  "NCHR",
      "NEW_TIME",  "NEXT_DAY",  "NTH_VALUE",  "NULLIF",  "NUMTODSINTERVAL",  "NUMTOYMINTERVAL",  "NVL",  "NVL2",  "POWER",  "RANK",  "RAWTOHEX",  "REGEXP_COUNT",  "REGEXP_INSTR",
      "REGEXP_REPLACE",  "REGEXP_SUBSTR",  "REMAINDER",  "REPLACE",  "ROUND",  "ROWNUM",  "RPAD",  "RTRIM",  "SESSIONTIMEZONE",  "SIGN",  "SIN",  "SINH",
      "SOUNDEX",  "SQRT",  "STDDEV",  "SUBSTR",  "SUM",  "SYS_CONTEXT",  "SYSDATE",  "SYSTIMESTAMP",  "TAN",  "TANH",  "TO_CHAR",  "TO_CLOB",  "TO_DATE",  "TO_DSINTERVAL",  "TO_LOB",
      "TO_MULTI_BYTE",  "TO_NCLOB",  "TO_NUMBER",  "TO_SINGLE_BYTE",  "TO_TIMESTAMP",  "TO_TIMESTAMP_TZ",  "TO_YMINTERVAL",  "TRANSLATE",  "TRIM",  "TRUNC", "TZ_OFFSET",  "UID",  "UPPER",
      "USER",  "USERENV",  "VAR_POP",  "VAR_SAMP",  "VARIANCE",  "VSIZE "
    };
    //}}}
    for (auto& k : idents) {
      sIdent id;
      id.mDeclaration = "Built-in function";
      language.mIdents.insert (make_pair (string(k), id));
      }

    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", ePaletteIndex::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("\\\'[^\\\']*\\\'", ePaletteIndex::String));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ePaletteIndex::Number));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", ePaletteIndex::Ident));
    language.mTokenRegexStrings.push_back (
      make_pair<string, ePaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ePaletteIndex::Punctuation));

    language.mCommentStart = "/*";
    language.mCommentEnd = "*/";
    language.mSingleLineComment = "//";
    language.mCaseSensitive = false;
    language.mAutoIndentation = false;
    language.mName = "SQL";

    inited = true;
    }

  return language;
  }
//}}}

// cTextEditor
//{{{
cTextEditor::cTextEditor()
  : mLineSpacing(1.0f), mUndoIndex(0), mTabSize(4),
    mOverwrite(false) , mReadOnly(false) , mWithinRender(false),
    mScrollToCursor(false), mScrollToTop(false),
    mTextChanged(false), mColorizerEnabled(true),
    mTextStart(20.0f), mLeftMargin(10), mCursorPositionChanged(false),
    mColorRangeMin(0), mColorRangeMax(0), mSelectionMode(eSelectionMode::Normal) ,
    mHandleKeyboardInputs(true), mHandleMouseInputs(true),
    mIgnoreImGuiChild(false), mShowWhitespaces(true), mCheckComments(true),
    mStartTime(chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()),
    mLastClick(-1.0f) {

  setPalette (getLightPalette());
  setLanguage (sLanguage::HLSL());

  mLines.push_back (tLine());
  }
//}}}
//{{{  gets
//{{{
bool cTextEditor::isOnWordBoundary (const sRowColumn& aAt) const {

  if (aAt.mLine >= (int)mLines.size() || aAt.mColumn == 0)
    return true;

  auto& line = mLines[aAt.mLine];
  int cindex = getCharacterIndex (aAt);
  if (cindex >= (int)line.size())
    return true;

  if (mColorizerEnabled)
    return line[cindex].mColorIndex != line[size_t(cindex - 1)].mColorIndex;

  return isspace(line[cindex].mChar) != isspace (line[cindex - 1].mChar);
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

  for (auto & line : mLines) {
    string text;
    text.resize (line.size());
    for (size_t i = 0; i < line.size(); ++i)
      text[i] = line[i].mChar;

    result.emplace_back (move (text));
    }

  return result;
  }
//}}}

//{{{
string cTextEditor::getSelectedText() const {
  return getText (mState.mSelectionStart, mState.mSelectionEnd);
  }

//}}}
//{{{
string cTextEditor::getCurrentLineText()const {

  int lineLength = getLineMaxColumn (mState.mCursorPosition.mLine);
  return getText (
    sRowColumn (mState.mCursorPosition.mLine, 0),
    sRowColumn (mState.mCursorPosition.mLine, lineLength));
  }
//}}}

//{{{
int cTextEditor::getCharacterIndex (const sRowColumn& rowColumn) const {

  if (rowColumn.mLine >= (int)mLines.size()) {
    cLog::log (LOGERROR, "cTextEditor::getCharacterIndex");
    return -1;
    }

  auto& line = mLines[rowColumn.mLine];
  int c = 0;

  int i = 0;
  for (; i < (int)line.size() && (c < rowColumn.mColumn);) {
    if (line[i].mChar == '\t')
      c = (c / mTabSize) * mTabSize + mTabSize;
    else
      ++c;
    i += utf8CharLength (line[i].mChar);
    }

  return i;
  }
//}}}
//{{{
int cTextEditor::getCharacterColumn (int lineCoord, int index) const {

  if (lineCoord >= (int)mLines.size())
    return 0;

  auto& line = mLines[lineCoord];
  int col = 0;

  int i = 0;
  while ((i < index) && i < ((int)line.size())) {
    auto c = line[i].mChar;
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
int cTextEditor::getLineCharacterCount (int lineCoord) const {

  if (lineCoord >= (int)mLines.size())
    return 0;

  auto& line = mLines[lineCoord];
  int c = 0;
  for (size_t i = 0; i < line.size(); c++)
    i += utf8CharLength (line[i].mChar);

  return c;
  }
//}}}
//{{{
int cTextEditor::getLineMaxColumn (int lineCoord) const {

  if (lineCoord >= (int)mLines.size())
    return 0;

  auto& line = mLines[lineCoord];
  int col = 0;
  for (size_t i = 0; i < line.size(); ) {
    auto c = line[i].mChar;
    if (c == '\t')
      col = (col / mTabSize) * mTabSize + mTabSize;
    else
      col++;
    i += utf8CharLength (c);
    }

  return col;
  }
//}}}
//}}}
//{{{  sets
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
void cTextEditor::setPalette (const tPalette& value) {
  mPaletteBase = value;
  }
//}}}

//{{{
void cTextEditor::setText (const string& text) {

  mLines.clear();
  mLines.emplace_back (tLine());

  for (auto chr : text)
    if (chr == '\r') {
      // ignore the carriage return character
      }
    else if (chr == '\n')
      mLines.emplace_back (tLine());
    else
      mLines.back().emplace_back (sGlyph (chr, ePaletteIndex::Default));

  mTextChanged = true;
  mScrollToTop = true;

  mUndoBuffer.clear();
  mUndoIndex = 0;

  colorize();
  }
//}}}
//{{{
void cTextEditor::setTextLines (const vector<string>& lines) {

  mLines.clear();

  if (lines.empty())
    mLines.emplace_back (tLine());
  else {
    mLines.resize (lines.size());
    for (size_t i = 0; i < lines.size(); ++i) {
      const string& line = lines[i];
      mLines[i].reserve (line.size());
      for (size_t j = 0; j < line.size(); ++j)
        mLines[i].emplace_back (sGlyph (line[j], ePaletteIndex::Default));
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
void cTextEditor::setSelection (const sRowColumn& startCord, const sRowColumn& endCord, eSelectionMode aMode) {

  auto oldSelStart = mState.mSelectionStart;
  auto oldSelEnd = mState.mSelectionEnd;

  mState.mSelectionStart = sanitizeRowColumn (startCord);
  mState.mSelectionEnd = sanitizeRowColumn(endCord);
  if (mState.mSelectionStart > mState.mSelectionEnd)
    swap (mState.mSelectionStart, mState.mSelectionEnd);

  switch (aMode) {
    case cTextEditor::eSelectionMode::Normal:
      break;

    case cTextEditor::eSelectionMode::Word: {
      mState.mSelectionStart = findWordStart (mState.mSelectionStart);
      if (!isOnWordBoundary (mState.mSelectionEnd))
        mState.mSelectionEnd = findWordEnd (findWordStart (mState.mSelectionEnd));
      break;
      }

    case cTextEditor::eSelectionMode::Line: {
      const auto lineNo = mState.mSelectionEnd.mLine;
      //const auto lineSize = (size_t)lineNo < mLines.size() ? mLines[lineNo].size() : 0;
      mState.mSelectionStart = sRowColumn (mState.mSelectionStart.mLine, 0);
      mState.mSelectionEnd = sRowColumn (lineNo, getLineMaxColumn (lineNo));
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
  mState.mCursorPosition.mLine = max (0, mState.mCursorPosition.mLine - amount);
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
  auto oldPos = mState.mCursorPosition;
  mState.mCursorPosition.mLine = max (0, min((int)mLines.size() - 1, mState.mCursorPosition.mLine + amount));

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

  auto oldPos = mState.mCursorPosition;
  mState.mCursorPosition = getActualCursorRowColumn();
  auto line = mState.mCursorPosition.mLine;
  auto cindex = getCharacterIndex (mState.mCursorPosition);

  while (amount-- > 0) {
    if (cindex == 0) {
      if (line > 0) {
        --line;
        if ((int)mLines.size() > line)
          cindex = (int)mLines[line].size();
        else
          cindex = 0;
        }
      }
    else {
      --cindex;
      if (cindex > 0) {
        if ((int)mLines.size() > line) {
          while (cindex > 0 && IsUTFSequence (mLines[line][cindex].mChar))
            --cindex;
          }
        }
      }

    mState.mCursorPosition = sRowColumn (line, getCharacterColumn(line, cindex));
    if (wordMode) {
      mState.mCursorPosition = findWordStart (mState.mCursorPosition);
      cindex = getCharacterIndex (mState.mCursorPosition);
      }
    }

  mState.mCursorPosition = sRowColumn (line, getCharacterColumn(line, cindex));

  assert(mState.mCursorPosition.mColumn >= 0);
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

  setSelection (mInteractiveStart, mInteractiveEnd, select && wordMode ? eSelectionMode::Word : eSelectionMode::Normal);
  ensureCursorVisible();
  }
//}}}
//{{{
void cTextEditor::moveRight(int amount, bool select, bool wordMode) {

  sRowColumn oldPos = mState.mCursorPosition;

  if (mLines.empty() || oldPos.mLine >= (int)mLines.size())
    return;

  auto cindex = getCharacterIndex (mState.mCursorPosition);
  while (amount-- > 0) {
    auto lindex = mState.mCursorPosition.mLine;
    auto& line = mLines[lindex];

    if (cindex >= (int)line.size()) {
      if (mState.mCursorPosition.mLine < (int)mLines.size() - 1) {
        mState.mCursorPosition.mLine = max (0, min ((int)mLines.size() - 1, mState.mCursorPosition.mLine + 1));
        mState.mCursorPosition.mColumn = 0;
        }
      else
        return;
      }
    else {
      cindex += utf8CharLength(line[cindex].mChar);
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
                select && wordMode ? eSelectionMode::Word : eSelectionMode::Normal);
  ensureCursorVisible();
  }
//}}}

//{{{
void cTextEditor::moveTop (bool select) {

  auto oldPos = mState.mCursorPosition;
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

  auto oldPos = getCursorPosition();
  auto newPos = sRowColumn ((int)mLines.size() - 1, 0);

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

  auto oldPos = mState.mCursorPosition;
  setCursorPosition (sRowColumn(mState.mCursorPosition.mLine, 0));

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

  auto oldPos = mState.mCursorPosition;
  setCursorPosition (sRowColumn(mState.mCursorPosition.mLine, getLineMaxColumn (oldPos.mLine)));

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
  setSelection (sRowColumn (0, 0), sRowColumn((int)mLines.size(), 0));
  }
//}}}
//{{{
void cTextEditor::selectWordUnderCursor() {

  auto c = getCursorPosition();
  setSelection (findWordStart(c), findWordEnd(c));
  }
//}}}

//{{{
void cTextEditor::InsertText (const char * value) {

  if (value == nullptr)
    return;

  auto pos = getActualCursorRowColumn();
  auto start = min (pos, mState.mSelectionStart);
  int totalLines = pos.mLine - start.mLine;

  totalLines += insertTextAt (pos, value);

  setSelection (pos, pos);
  setCursorPosition (pos);
  colorize (start.mLine - 1, totalLines + 2);
  }
//}}}
//{{{
void cTextEditor::copy() {

  if (hasSelection()) {
    ImGui::SetClipboardText (getSelectedText().c_str());
    }
  else {
    if (!mLines.empty()) {
      string str;
      auto& line = mLines[getActualCursorRowColumn().mLine];
      for (auto& g : line)
        str.push_back(g.mChar);
      ImGui::SetClipboardText (str.c_str());
      }
    }
  }
//}}}
//{{{
void cTextEditor::cut() {

  if (isReadOnly())
    copy();
  else {
    if (hasSelection()) {
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
  }
//}}}
//{{{
void cTextEditor::paste() {

  if (isReadOnly())
    return;

  auto clipText = ImGui::GetClipboardText();
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

    InsertText (clipText);

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
    auto pos = getActualCursorRowColumn();
    setCursorPosition (pos);
    auto& line = mLines[pos.mLine];

    if (pos.mColumn == getLineMaxColumn (pos.mLine)) {
      if (pos.mLine == (int)mLines.size() - 1)
        return;

      u.mRemoved = '\n';
      u.mRemovedStart = u.mRemovedEnd = getActualCursorRowColumn();
      advance (u.mRemovedEnd);

      auto& nextLine = mLines[pos.mLine + 1];
      line.insert (line.end(), nextLine.begin(), nextLine.end());
      removeLine (pos.mLine + 1);
      }
    else {
      auto cindex = getCharacterIndex (pos);
      u.mRemovedStart = u.mRemovedEnd = getActualCursorRowColumn();
      u.mRemovedEnd.mColumn++;
      u.mRemoved = getText (u.mRemovedStart, u.mRemovedEnd);

      auto d = utf8CharLength (line[cindex].mChar);
      while (d-- > 0 && cindex < (int)line.size())
        line.erase (line.begin() + cindex);
      }

    mTextChanged = true;
    colorize(pos.mLine, 1);
    }

  u.mAfter = mState;
  addUndo (u);
  }
//}}}

//{{{
void cTextEditor::undo (int steps) {

  while (canUndo() && steps-- > 0)
    mUndoBuffer[--mUndoIndex].Undo (this);
  }
//}}}
//{{{
void cTextEditor::redo (int steps) {

  while (canRedo() && steps-- > 0)
    mUndoBuffer[mUndoIndex++].Redo (this);
  }
//}}}
//}}}
//{{{
void cTextEditor::render (const char* title, const ImVec2& size, bool border) {

  mWithinRender = true;
  mTextChanged = false;
  mCursorPositionChanged = false;

  ImGui::PushStyleColor (ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4 (mPalette[(int)ePaletteIndex::Background]));
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

// public static
const cTextEditor::tPalette& cTextEditor::getDarkPalette() { return kDarkPalette; }
const cTextEditor::tPalette& cTextEditor::getLightPalette() { return kLightPalette; }

// private:
//{{{  gets
//{{{
int cTextEditor::getPageSize() const {

  auto height = ImGui::GetWindowHeight() - 20.0f;
  return (int)floor (height / mCharAdvance.y);
  }
//}}}
//{{{
string cTextEditor::getText (const sRowColumn& startCord, const sRowColumn& endCord) const {

  string result;

  auto lstart = startCord.mLine;
  auto lend = endCord.mLine;

  auto istart = getCharacterIndex (startCord);
  auto iend = getCharacterIndex (endCord);

  size_t s = 0;
  for (size_t i = lstart; i < (size_t)lend; i++)
    s += mLines[i].size();

  result.reserve (s + s / 8);

  while (istart < iend || lstart < lend) {
    if (lstart >= (int)mLines.size())
      break;

    auto& line = mLines[lstart];
    if (istart < (int)line.size()) {
      result += line[istart].mChar;
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
    return mPalette[(int)ePaletteIndex::Default];

  if (glyph.mComment)
    return mPalette[(int)ePaletteIndex::Comment];

  if (glyph.mMultiLineComment)
    return mPalette[(int)ePaletteIndex::MultiLineComment];

  auto const color = mPalette[(int)glyph.mColorIndex];
  if (glyph.mPreprocessor) {
    const auto ppcolor = mPalette[(int)ePaletteIndex::Preprocessor];
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

  auto start = findWordStart (rowColumn);
  auto end = findWordEnd (rowColumn);

  auto istart = getCharacterIndex (start);
  auto iend = getCharacterIndex (end);

  string r;
  for (auto it = istart; it < iend; ++it)
    r.push_back (mLines[rowColumn.mLine][it].mChar);

  return r;
  }
//}}}
//{{{
string cTextEditor::getWordUnderCursor() const {

  auto c = getCursorPosition();
  return getWordAt (c);
  }
//}}}

//{{{
float cTextEditor::getTextDistanceToLineStart (const sRowColumn& from) const {

  auto& line = mLines[from.mLine];
  float distance = 0.0f;

  float spaceSize = ImGui::GetFont()->CalcTextSizeA (ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

  int colIndex = getCharacterIndex (from);
  for (size_t i = 0; (i < line.size()) && ((int)i < colIndex);) {
    if (line[i].mChar == '\t') {
      distance = (1.0f + floor((1.0f + distance) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
      ++i;
      }
    else {
      auto d = utf8CharLength (line[i].mChar);
      char tempCString[7];
      int j = 0;
      for (; j < 6 && d-- > 0 && i < line.size(); j++, i++)
        tempCString[j] = line[i].mChar;

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

  auto line = rowColumn.mLine;
  auto column = rowColumn.mColumn;

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

  int lineNo = max(0, (int)floor (local.y / mCharAdvance.y));
  int columnCoord = 0;

  if ((lineNo >= 0) && (lineNo < (int)mLines.size())) {
    auto& line = mLines.at (lineNo);

    int columnIndex = 0;
    float columnX = 0.0f;

    while (columnIndex < (int)line.size()) {
      float columnWidth = 0.0f;

      if (line[columnIndex].mChar == '\t') {
        float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
        float oldX = columnX;
        float newColumnX = (1.0f + floor((1.0f + columnX) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
        columnWidth = newColumnX - oldX;
        if (mTextStart + columnX + columnWidth * 0.5f > local.x)
          break;
        columnX = newColumnX;
        columnCoord = (columnCoord / mTabSize) * mTabSize + mTabSize;
        columnIndex++;
        }

      else {
        char buf[7];
        auto d = utf8CharLength (line[columnIndex].mChar);
        int i = 0;
        while (i < 6 && d-- > 0)
          buf[i++] = line[columnIndex++].mChar;
        buf[i] = '\0';

        columnWidth = ImGui::GetFont()->CalcTextSizeA (ImGui::GetFontSize(), FLT_MAX, -1.0f, buf).x;
        if (mTextStart + columnX + columnWidth * 0.5f > local.x)
          break;

        columnX += columnWidth;
        columnCoord++;
        }
      }
    }

  return sanitizeRowColumn (sRowColumn (lineNo, columnCoord));
  }
//}}}

// finds
//{{{
cTextEditor::sRowColumn cTextEditor::findWordStart (const sRowColumn& from) const {

  sRowColumn at = from;
  if (at.mLine >= (int)mLines.size())
    return at;

  auto& line = mLines[at.mLine];
  auto cindex = getCharacterIndex (at);

  if (cindex >= (int)line.size())
    return at;

  while (cindex > 0 && isspace(line[cindex].mChar))
    --cindex;

  auto cstart = (ePaletteIndex)line[cindex].mColorIndex;
  while (cindex > 0) {
    auto c = line[cindex].mChar;
    if ((c & 0xC0) != 0x80) { // not UTF code sequence 10xxxxxx
      if (c <= 32 && isspace(c)) {
        cindex++;
        break;
        }
      if (cstart != (ePaletteIndex)line[size_t(cindex - 1)].mColorIndex)
        break;
      }
    --cindex;
    }

  return sRowColumn(at.mLine, getCharacterColumn(at.mLine, cindex));
  }
//}}}
//{{{
cTextEditor::sRowColumn cTextEditor::findWordEnd (const sRowColumn& from) const {

  sRowColumn at = from;
  if (at.mLine >= (int)mLines.size())
    return at;

  auto& line = mLines[at.mLine];
  auto cindex = getCharacterIndex (at);

  if (cindex >= (int)line.size())
    return at;

  bool prevspace = (bool)isspace(line[cindex].mChar);
  auto cstart = (ePaletteIndex)line[cindex].mColorIndex;
  while (cindex < (int)line.size()) {
    auto c = line[cindex].mChar;
    auto d = utf8CharLength (c);
    if (cstart != (ePaletteIndex)line[cindex].mColorIndex)
      break;

    if (prevspace != !!isspace(c)) {
      if (isspace(c))
        while (cindex < (int)line.size() && isspace(line[cindex].mChar))
          ++cindex;
      break;
      }
    cindex += d;
    }

  return sRowColumn (from.mLine, getCharacterColumn (from.mLine, cindex));
  }
//}}}
//{{{
cTextEditor::sRowColumn cTextEditor::findNextWord (const sRowColumn& from) const {

  sRowColumn at = from;
  if (at.mLine >= (int)mLines.size())
    return at;

  // skip to the next non-word character
  bool isword = false;
  bool skip = false;

  auto cindex = getCharacterIndex (from);
  if (cindex < (int)mLines[at.mLine].size()) {
    auto& line = mLines[at.mLine];
    isword = isalnum(line[cindex].mChar);
    skip = isword;
    }

  while (!isword || skip) {
    if (at.mLine >= (int)mLines.size()) {
      auto l = max(0, (int) mLines.size() - 1);
      return sRowColumn (l, getLineMaxColumn(l));
      }

    auto& line = mLines[at.mLine];
    if (cindex < (int)line.size()) {
      isword = isalnum (line[cindex].mChar);

      if (isword && !skip)
        return sRowColumn (at.mLine, getCharacterColumn(at.mLine, cindex));

      if (!isword)
        skip = false;

      cindex++;
      }
    else {
      cindex = 0;
      ++at.mLine;
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
    auto& line = mLines[i];

    if (line.empty())
      continue;

    buffer.resize (line.size());
    for (size_t j = 0; j < line.size(); ++j) {
      auto& col = line[j];
      buffer[j] = col.mChar;
      col.mColorIndex = ePaletteIndex::Default;
      }

    const char * bufferBegin = &buffer.front();
    const char * bufferEnd = bufferBegin + buffer.size();

    auto last = bufferEnd;

    for (auto first = bufferBegin; first != last; ) {
      const char * token_begin = nullptr;
      const char * token_end = nullptr;
      ePaletteIndex token_color = ePaletteIndex::Default;

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
        if (token_color == ePaletteIndex::Ident) {
          id.assign(token_begin, token_end);

          // todo : allmost all language definitions use lower case to specify keywords, so shouldn't this use ::tolower ?
          if (!mLanguage.mCaseSensitive)
            transform (id.begin(), id.end(), id.begin(), ::toupper);

          if (!line[first - bufferBegin].mPreprocessor) {
            if (mLanguage.mKeywords.count(id) != 0)
              token_color = ePaletteIndex::Keyword;
            else if (mLanguage.mIdents.count(id) != 0)
              token_color = ePaletteIndex::KnownIdent;
            else if (mLanguage.mPreprocIdents.count(id) != 0)
              token_color = ePaletteIndex::PreprocIdent;
            }
          else {
            if (mLanguage.mPreprocIdents.count(id) != 0)
              token_color = ePaletteIndex::PreprocIdent;
            }
          }

        for (size_t j = 0; j < token_length; ++j)
          line[(token_begin - bufferBegin) + j].mColorIndex = token_color;

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
      auto& line = mLines[currentLine];
      if (currentIndex == 0 && !concatenate) {
        withinSingleLineComment = false;
        withinPreproc = false;
        firstChar = true;
        }
      concatenate = false;

      if (!line.empty()) {
        auto& g = line[currentIndex];
        auto c = g.mChar;
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
            auto& startStr = mLanguage.mCommentStart;
            auto& singleStartStr = mLanguage.mSingleLineComment;

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

            auto& endStr = mLanguage.mCommentEnd;
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

  auto top = 1 + (int)ceil(scrollY / mCharAdvance.y);
  auto bottom = (int)ceil((scrollY + height) / mCharAdvance.y);

  auto left = (int)ceil(scrollX / mCharAdvance.x);
  auto right = (int)ceil((scrollX + width) / mCharAdvance.x);

  auto pos = getActualCursorRowColumn();
  auto len = getTextDistanceToLineStart (pos);

  if (pos.mLine < top)
    ImGui::SetScrollY (max(0.0f, (pos.mLine - 1) * mCharAdvance.y));
  if (pos.mLine > bottom - 4)
    ImGui::SetScrollY (max(0.0f, (pos.mLine + 4) * mCharAdvance.y - height));

  if (len + mTextStart < left + 4)
    ImGui::SetScrollX (max(0.0f, len + mTextStart - 4));
  if (len + mTextStart > right - 4)
    ImGui::SetScrollX (max(0.0f, len + mTextStart + 4 - width));
  }
//}}}
//{{{
void cTextEditor::advance (sRowColumn& rowColumn) const {

  if (rowColumn.mLine < (int)mLines.size()) {
    auto& line = mLines[rowColumn.mLine];
    auto cindex = getCharacterIndex (rowColumn);
    if (cindex + 1 < (int)line.size()) {
      auto delta = utf8CharLength (line[cindex].mChar);
      cindex = min (cindex + delta, (int)line.size() - 1);
      }
    else {
      ++rowColumn.mLine;
      cindex = 0;
      }

    rowColumn.mColumn = getCharacterColumn (rowColumn.mLine, cindex);
    }
  }
//}}}

//{{{
void cTextEditor::addUndo (sUndoRecord& value) {

  assert(!mReadOnly);
  //printf("AddUndo: (@%d.%d) +\'%s' [%d.%d .. %d.%d], -\'%s', [%d.%d .. %d.%d] (@%d.%d)\n",
  //  value.mBefore.mCursorPosition.mLine, value.mBefore.mCursorPosition.mColumn,
  //  value.mAdded.c_str(), value.mAddedStart.mLine, value.mAddedStart.mColumn, value.mAddedEnd.mLine, value.mAddedEnd.mColumn,
  //  value.mRemoved.c_str(), value.mRemovedStart.mLine, value.mRemovedStart.mColumn, value.mRemovedEnd.mLine, value.mRemovedEnd.mColumn,
  //  value.mAfter.mCursorPosition.mLine, value.mAfter.mCursorPosition.mColumn
  //  );

  mUndoBuffer.resize ((size_t)(mUndoIndex + 1));
  mUndoBuffer.back() = value;
  ++mUndoIndex;
  }
//}}}

//{{{
void cTextEditor::removeLine (int startCord, int endCord) {

  assert (!mReadOnly);
  assert (endCord >= startCord);
  assert (mLines.size() > (size_t)(endCord - startCord));

  tMarkers etmp;
  for (auto& i : mMarkers) {
    tMarkers::value_type e(i.first >= startCord ? i.first - 1 : i.first, i.second);
    if (e.first >= startCord && e.first <= endCord)
      continue;
    etmp.insert(e);
    }
  mMarkers = move (etmp);

  mLines.erase (mLines.begin() + startCord, mLines.begin() + endCord);
  assert (!mLines.empty());

  mTextChanged = true;
  }
//}}}
//{{{
void cTextEditor::removeLine (int index) {

  assert(!mReadOnly);
  assert(mLines.size() > 1);

  tMarkers etmp;
  for (auto& i : mMarkers) {
    tMarkers::value_type e(i.first > index ? i.first - 1 : i.first, i.second);
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
    auto pos = getActualCursorRowColumn();
    setCursorPosition (pos);

    if (mState.mCursorPosition.mColumn == 0) {
      if (mState.mCursorPosition.mLine == 0)
        return;

      u.mRemoved = '\n';
      u.mRemovedStart = u.mRemovedEnd = sRowColumn (pos.mLine - 1, getLineMaxColumn (pos.mLine - 1));
      advance (u.mRemovedEnd);

      auto& line = mLines[mState.mCursorPosition.mLine];
      auto& prevLine = mLines[mState.mCursorPosition.mLine - 1];
      auto prevSize = getLineMaxColumn (mState.mCursorPosition.mLine - 1);
      prevLine.insert (prevLine.end(), line.begin(), line.end());

      tMarkers etmp;
      for (auto& i : mMarkers)
        etmp.insert (tMarkers::value_type (i.first - 1 == mState.mCursorPosition.mLine ? i.first - 1 : i.first, i.second));
      mMarkers = move (etmp);

      removeLine (mState.mCursorPosition.mLine);
      --mState.mCursorPosition.mLine;
      mState.mCursorPosition.mColumn = prevSize;
      }
    else {
      auto& line = mLines[mState.mCursorPosition.mLine];
      auto cindex = getCharacterIndex (pos) - 1;
      auto cend = cindex + 1;
      while (cindex > 0 && IsUTFSequence (line[cindex].mChar))
        --cindex;

      //if (cindex > 0 && UTF8CharLength(line[cindex].mChar) > 1)
      //  --cindex;

      u.mRemovedStart = u.mRemovedEnd = getActualCursorRowColumn();
      --u.mRemovedStart.mColumn;
      --mState.mCursorPosition.mColumn;

      while (cindex < (int)line.size() && cend-- > cindex) {
        u.mRemoved += line[cindex].mChar;
        line.erase(line.begin() + cindex);
        }
      }

    mTextChanged = true;

    ensureCursorVisible();
    colorize (mState.mCursorPosition.mLine, 1);
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
  colorize (mState.mSelectionStart.mLine, 1);
  }
//}}}
//{{{
void cTextEditor::deleteRange (const sRowColumn& startCord, const sRowColumn& endCord) {

  assert (endCord >= startCord);
  assert (!mReadOnly);

  //printf("D(%d.%d)-(%d.%d)\n", startCord.mLine, startCord.mColumn, endCord.mLine, endCord.mColumn);

  if (endCord == startCord)
    return;

  auto start = getCharacterIndex(startCord);
  auto end = getCharacterIndex(endCord);
  if (startCord.mLine == endCord.mLine) {
    auto& line = mLines[startCord.mLine];
    auto n = getLineMaxColumn(startCord.mLine);
    if (endCord.mColumn >= n)
      line.erase(line.begin() + start, line.end());
    else
      line.erase(line.begin() + start, line.begin() + end);
    }

  else {
    auto& firstLine = mLines[startCord.mLine];
    auto& lastLine = mLines[endCord.mLine];

    firstLine.erase(firstLine.begin() + start, firstLine.end());
    lastLine.erase(lastLine.begin(), lastLine.begin() + end);

    if (startCord.mLine < endCord.mLine)
      firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());

    if (startCord.mLine < endCord.mLine)
      removeLine(startCord.mLine + 1, endCord.mLine + 1);
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
    if (aChar == '\t' && mState.mSelectionStart.mLine != mState.mSelectionEnd.mLine) {
      auto start = mState.mSelectionStart;
      auto end = mState.mSelectionEnd;
      auto originalEnd = end;
      if (start > end)
        swap(start, end);
      start.mColumn = 0;
      // end.mColumn = end.mLine < mLines.size() ? mLines[end.mLine].size() : 0;
      if (end.mColumn == 0 && end.mLine > 0)
        --end.mLine;
      if (end.mLine >= (int)mLines.size())
        end.mLine = mLines.empty() ? 0 : (int)mLines.size() - 1;
      end.mColumn = getLineMaxColumn (end.mLine);

      //if (end.mColumn >= getLineMaxColumn(end.mLine))
      //  end.mColumn = getLineMaxColumn(end.mLine) - 1;
      u.mRemovedStart = start;
      u.mRemovedEnd = end;
      u.mRemoved = getText (start, end);

      bool modified = false;

      for (int i = start.mLine; i <= end.mLine; i++) {
        auto& line = mLines[i];
        if (shift) {
          if (!line.empty()) {
            if (line.front().mChar == '\t') {
              line.erase(line.begin());
              modified = true;
            }
            else {
              for (int j = 0; j < mTabSize && !line.empty() && line.front().mChar == ' '; j++) {
                line.erase(line.begin());
                modified = true;
                }
              }
            }
          }
        else {
          line.insert(line.begin(), sGlyph ('\t', cTextEditor::ePaletteIndex::Background));
          modified = true;
          }
        }

      if (modified) {
        start = sRowColumn (start.mLine, getCharacterColumn (start.mLine, 0));
        sRowColumn rangeEnd;
        if (originalEnd.mColumn != 0) {
          end = sRowColumn (end.mLine, getLineMaxColumn (end.mLine));
          rangeEnd = end;
          u.mAdded = getText (start, end);
          }
        else {
          end = sRowColumn (originalEnd.mLine, 0);
          rangeEnd = sRowColumn (end.mLine - 1, getLineMaxColumn (end.mLine - 1));
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
    insertLine (coord.mLine + 1);
    auto& line = mLines[coord.mLine];
    auto& newLine = mLines[coord.mLine + 1];

    if (mLanguage.mAutoIndentation)
      for (size_t it = 0; it < line.size() && isascii(line[it].mChar) && isblank(line[it].mChar); ++it)
        newLine.push_back(line[it]);

    const size_t whitespaceSize = newLine.size();
    auto cindex = getCharacterIndex (coord);
    newLine.insert (newLine.end(), line.begin() + cindex, line.end());
    line.erase (line.begin() + cindex, line.begin() + line.size());
    setCursorPosition (sRowColumn(coord.mLine + 1, getCharacterColumn (coord.mLine + 1, (int)whitespaceSize)));
    u.mAdded = (char)aChar;
    }

  else {
    char buf[7];
    int e = ImTextCharToUtf8(buf, 7, aChar);
    if (e > 0) {
      buf[e] = '\0';
      auto& line = mLines[coord.mLine];
      auto cindex = getCharacterIndex (coord);

      if (mOverwrite && cindex < (int)line.size()) {
        auto d = utf8CharLength (line[cindex].mChar);
        u.mRemovedStart = mState.mCursorPosition;
        u.mRemovedEnd = sRowColumn(coord.mLine, getCharacterColumn (coord.mLine, cindex + d));
        while (d-- > 0 && cindex < (int)line.size()) {
          u.mRemoved += line[cindex].mChar;
          line.erase(line.begin() + cindex);
          }
        }

      for (auto p = buf; *p != '\0'; p++, ++cindex)
        line.insert(line.begin() + cindex, sGlyph (*p, ePaletteIndex::Default));
      u.mAdded = buf;

      setCursorPosition(sRowColumn(coord.mLine, getCharacterColumn (coord.mLine, cindex)));
      }
    else
      return;
    }

  mTextChanged = true;

  u.mAddedEnd = getActualCursorRowColumn();
  u.mAfter = mState;

  addUndo(u);

  colorize (coord.mLine - 1, 3);
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
      if (cindex < (int)mLines[where.mLine].size()) {
        auto& newLine = insertLine (where.mLine + 1);
        auto& line = mLines[where.mLine];
        newLine.insert (newLine.begin(), line.begin() + cindex, line.end());
        line.erase (line.begin() + cindex, line.end());
        }
      else
        insertLine (where.mLine + 1);

      ++where.mLine;
      where.mColumn = 0;
      cindex = 0;
      ++totalLines;
      ++value;
      }

    else {
      auto& line = mLines[where.mLine];
      auto d = utf8CharLength (*value);
      while (d-- > 0 && *value != '\0')
        line.insert (line.begin() + cindex++, sGlyph (*value++, ePaletteIndex::Default));
      ++where.mColumn;
      }

    mTextChanged = true;
    }

  return totalLines;
  }
//}}}
//{{{
cTextEditor::tLine& cTextEditor::insertLine (int index) {

  assert (!mReadOnly);

  auto& result = *mLines.insert (mLines.begin() + index, tLine());

  tMarkers etmp;
  for (auto& i : mMarkers)
    etmp.insert (tMarkers::value_type (i.first >= index ? i.first + 1 : i.first, i.second));
  mMarkers = move (etmp);

  return result;
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
          mSelectionMode = eSelectionMode::Line;
          setSelection (mInteractiveStart, mInteractiveEnd, mSelectionMode);
          }
        mLastClick = -1.0f;
        }

      // left mouse button double click
      else if (doubleClick) {
        if (!ctrl) {
          mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = screenPosToRowColumn(ImGui::GetMousePos());
          if (mSelectionMode == eSelectionMode::Line)
            mSelectionMode = eSelectionMode::Normal;
          else
            mSelectionMode = eSelectionMode::Word;
          setSelection (mInteractiveStart, mInteractiveEnd, mSelectionMode);
          }
        mLastClick = (float)ImGui::GetTime();
        }

      // left mouse button click
      else if (click) {
        mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = screenPosToRowColumn(ImGui::GetMousePos());
        if (ctrl)
          mSelectionMode = eSelectionMode::Word;
        else
          mSelectionMode = eSelectionMode::Normal;
        setSelection (mInteractiveStart, mInteractiveEnd, mSelectionMode);
        mLastClick = (float)ImGui::GetTime();
        }

      // left mouse button dragging (=> update selection)
      else if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0)) {
        io.WantCaptureMouse = true;
        mState.mCursorPosition = mInteractiveEnd = screenPosToRowColumn (ImGui::GetMousePos());
        setSelection (mInteractiveStart, mInteractiveEnd, mSelectionMode);
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
  for (int i = 0; i < (int)ePaletteIndex::Max; ++i) {
    auto color = ImGui::ColorConvertU32ToFloat4 (mPaletteBase[i]);
    color.w *= ImGui::GetStyle().Alpha;
    mPalette[i] = ImGui::ColorConvertFloat4ToU32 (color);
    }
  //}}}

  assert (mLineBuffer.empty());
  auto contentSize = ImGui::GetWindowContentRegionMax();
  auto drawList = ImGui::GetWindowDrawList();
  float longest (mTextStart);

  if (mScrollToTop) {
    //{{{  scroll to top
    mScrollToTop = false;
    ImGui::SetScrollY (0.f);
    }
    //}}}

  ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
  auto scrollX = ImGui::GetScrollX();
  auto scrollY = ImGui::GetScrollY();
  auto lineNo = (int)floor (scrollY / mCharAdvance.y);
  auto globalLineMax = (int)mLines.size();
  auto lineMax = max (0,
    min ((int)mLines.size() - 1, lineNo + (int)floor((scrollY + contentSize.y) / mCharAdvance.y)));

  //{{{  calc mTextStart by evaluating mLines size (global lineMax) plus two spaces as text width
  char buf[16];
  snprintf (buf, 16, " %d ", globalLineMax);

  mTextStart = ImGui::GetFont()->CalcTextSizeA (
    ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x + mLeftMargin;
  //}}}

  if (!mLines.empty()) {
    float spaceSize = ImGui::GetFont()->CalcTextSizeA (
      ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

    while (lineNo <= lineMax) {
      ImVec2 lineStartScreenPos = ImVec2 (cursorScreenPos.x, cursorScreenPos.y + lineNo * mCharAdvance.y);
      ImVec2 textScreenPos = ImVec2 (lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);

      auto& line = mLines[lineNo];
      longest = max(mTextStart + getTextDistanceToLineStart (sRowColumn (lineNo, getLineMaxColumn (lineNo))), longest);
      auto columnNo = 0;
      sRowColumn lineStartCoord (lineNo, 0);
      sRowColumn lineEndCoord (lineNo, getLineMaxColumn (lineNo));
      //{{{  draw selection for the current line
      float sstart = -1.0f;
      float ssend = -1.0f;

      assert (mState.mSelectionStart <= mState.mSelectionEnd);
      if (mState.mSelectionStart <= lineEndCoord)
        sstart = mState.mSelectionStart > lineStartCoord ? getTextDistanceToLineStart (mState.mSelectionStart) : 0.0f;
      if (mState.mSelectionEnd > lineStartCoord)
        ssend = getTextDistanceToLineStart (mState.mSelectionEnd < lineEndCoord ? mState.mSelectionEnd : lineEndCoord);

      if (mState.mSelectionEnd.mLine > lineNo)
        ssend += mCharAdvance.x;

      if (sstart != -1 && ssend != -1 && sstart < ssend) {
        ImVec2 vstart (lineStartScreenPos.x + mTextStart + sstart, lineStartScreenPos.y);
        ImVec2 vend (lineStartScreenPos.x + mTextStart + ssend, lineStartScreenPos.y + mCharAdvance.y);
        drawList->AddRectFilled (vstart, vend, mPalette[(int)ePaletteIndex::Selection]);
        }
      //}}}
      //{{{  draw error markers
      auto start = ImVec2 (lineStartScreenPos.x + scrollX, lineStartScreenPos.y);
      auto errorIt = mMarkers.find (lineNo + 1);
      if (errorIt != mMarkers.end()) {
        auto end = ImVec2 (lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + mCharAdvance.y);
        drawList->AddRectFilled (start, end, mPalette[(int)ePaletteIndex::Marker]);

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
      snprintf (buf, 16, "%d  ", lineNo + 1);

      auto lineNoWidth = ImGui::GetFont()->CalcTextSizeA (
        ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x;

      drawList->AddText (ImVec2 (lineStartScreenPos.x + mTextStart - lineNoWidth, lineStartScreenPos.y),
                         mPalette[(int)ePaletteIndex::LineNumber], buf);

      if (mState.mCursorPosition.mLine == lineNo) {
        auto focused = ImGui::IsWindowFocused();

        // highlight the current line (where the cursor is)
        if (!hasSelection()) {
          auto end = ImVec2 (start.x + contentSize.x + scrollX, start.y + mCharAdvance.y);
          drawList->AddRectFilled (start, end,
            mPalette[(int)(focused ? ePaletteIndex::CurrentLineFill : ePaletteIndex::CurrentLineFillInactive)]);
          drawList->AddRect (start, end, mPalette[(int)ePaletteIndex::CurrentLineEdge], 1.0f);
          }

        // render cursor
        if (focused) {
          auto timeEnd = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
          auto elapsed = timeEnd - mStartTime;
          if (elapsed > 400) {
            float width = 1.0f;
            auto cindex = getCharacterIndex (mState.mCursorPosition);
            float cx = getTextDistanceToLineStart (mState.mCursorPosition);

            if (mOverwrite && cindex < (int)line.size()) {
              auto c = line[cindex].mChar;
              if (c == '\t') {
                auto x = (1.0f + floor((1.0f + cx) / (float(mTabSize) * spaceSize))) * (float(mTabSize) * spaceSize);
                width = x - cx;
                }
              else {
                char buf2[2];
                buf2[0] = line[cindex].mChar;
                buf2[1] = '\0';
                width = ImGui::GetFont()->CalcTextSizeA (ImGui::GetFontSize(), FLT_MAX, -1.0f, buf2).x;
                }
              }

            ImVec2 cstart (textScreenPos.x + cx, lineStartScreenPos.y);
            ImVec2 cend (textScreenPos.x + cx + width, lineStartScreenPos.y + mCharAdvance.y);
            drawList->AddRectFilled (cstart, cend, mPalette[(int)ePaletteIndex::Cursor]);
            if (elapsed > 800)
              mStartTime = timeEnd;
            }
          }
        }
      //}}}
      //{{{  render colored text
      auto prevColor = line.empty() ? mPalette[(int)ePaletteIndex::Default] : getGlyphColor (line[0]);
      ImVec2 bufferOffset;

      for (int i = 0; i < (int)line.size();) {
        auto& glyph = line[i];
        auto color = getGlyphColor (glyph);

        if ((color != prevColor || glyph.mChar == '\t' || glyph.mChar == ' ') && !mLineBuffer.empty()) {
          const ImVec2 newOffset (textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
          drawList->AddText (newOffset, prevColor, mLineBuffer.c_str());
          auto textSize = ImGui::GetFont()->CalcTextSizeA (
            ImGui::GetFontSize(), FLT_MAX, -1.0f, mLineBuffer.c_str(), nullptr, nullptr);
          bufferOffset.x += textSize.x;
          mLineBuffer.clear();
          }
        prevColor = color;

        if (glyph.mChar == '\t') {
          auto oldX = bufferOffset.x;
          bufferOffset.x = (1.0f + floor ((1.0f + bufferOffset.x) / (float(mTabSize) * spaceSize))) *
                           (float(mTabSize) * spaceSize);
          ++i;

          if (mShowWhitespaces) {
            const auto s = ImGui::GetFontSize();
            const auto x1 = textScreenPos.x + oldX + 1.0f;
            const auto x2 = textScreenPos.x + bufferOffset.x - 1.0f;
            const auto y = textScreenPos.y + bufferOffset.y + s * 0.5f;
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
            const auto s = ImGui::GetFontSize();
            const auto x = textScreenPos.x + bufferOffset.x + spaceSize * 0.5f;
            const auto y = textScreenPos.y + bufferOffset.y + s * 0.5f;
            drawList->AddCircleFilled (ImVec2(x, y), 1.5f, 0x80808080, 4);
            }
          bufferOffset.x += spaceSize;
          i++;
          }

        else {
          auto l = utf8CharLength (glyph.mChar);
          while (l-- > 0)
            mLineBuffer.push_back (line[i++].mChar);
          }

        ++columnNo;
        }

      if (!mLineBuffer.empty()) {
        const ImVec2 newOffset (textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
        drawList->AddText (newOffset, prevColor, mLineBuffer.c_str());
        mLineBuffer.clear();
        }
      //}}}
      ++lineNo;
      }
    //{{{  draw tooltip on idents/preprocessor symbols
    if (ImGui::IsMousePosValid()) {
      auto id = getWordAt (screenPosToRowColumn (ImGui::GetMousePos()));
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
