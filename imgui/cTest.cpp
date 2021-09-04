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
