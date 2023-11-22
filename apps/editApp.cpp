// fed.cpp - fed folding editor main, uses imGui, stb
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <unordered_set>
#include <functional>
#include <regex>
#include <fstream>
#include <filesystem>

//{{{  include stb
// invoke header only library implementation here
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

  #define STB_IMAGE_IMPLEMENTATION
  #include <stb_image.h>
  #define STB_IMAGE_WRITE_IMPLEMENTATION
  #include <stb_image_write.h>

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}

// utils
#include "../common/date.h"
#include "../common/utils.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"
#include "../common/cFileView.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"
#include "../app/myImgui.h"
#include "../app/cUI.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

using namespace std;
//}}}

namespace {
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
  //{{{
  bool parseIdentifier (const char* srcBegin, const char* srcEnd, const char*& tokenBegin, const char*& tokenEnd) {

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
  bool parseNumber (const char* srcBegin, const char* srcEnd, const char*& tokenBegin, const char*& tokenEnd) {

    const char* srcPtr = srcBegin;

    // skip leading sign +-
    if ((*srcPtr == '+') || (*srcPtr == '-'))
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
  bool parsePunctuation (const char* srcBegin, const char*& tokenBegin, const char*& tokenEnd) {

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
  bool parseString (const char* srcBegin, const char* srcEnd, const char*& tokenBegin, const char*& tokenEnd) {

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
  bool parseLiteral (const char* srcBegin, const char* srcEnd, const char*& tokenBegin, const char*& tokenEnd) {

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

  //{{{
  class cLanguage {
  public:
    using tRegex = vector <pair <regex,uint8_t>>;
    using tTokenSearch = bool(*)(const char* srcBegin, const char* srcEnd,
                                 const char*& dstBegin, const char*& dstEnd, uint8_t& color);
    // static const
    //{{{
    static const cLanguage c() {

      cLanguage language;

      language.mName = "C++";

      // comment tokens
      language.mCommentBegin = "/*";
      language.mCommentEnd = "*/";
      language.mCommentSingle = "//";

      // fold tokens
      language.mFoldBeginToken = "//{{{";
      language.mFoldEndToken = "//}}}";

      for (const auto& keyWord : kKeyWords)
        language.mKeyWords.insert (keyWord);
      for (auto& knownWord : kKnownWords)
        language.mKnownWords.insert (knownWord);

      // faster token search
      language.mTokenSearch = [](const char* srcBegin, const char* srcEnd,
                                 const char*& tokenBegin, const char*& tokenEnd, uint8_t& color) -> bool {
        // tokenSearch lambda
        color = eUndefined;
        while ((srcBegin < srcEnd) && isascii (*srcBegin) && isblank (*srcBegin))
          srcBegin++;
        if (srcBegin == srcEnd) {
          tokenBegin = srcEnd;
          tokenEnd = srcEnd;
          color = eText;
          }
        if (parseIdentifier (srcBegin, srcEnd, tokenBegin, tokenEnd))
          color = eIdentifier;
        else if (parseNumber (srcBegin, srcEnd, tokenBegin, tokenEnd))
          color = eNumber;
        else if (parsePunctuation (srcBegin, tokenBegin, tokenEnd))
          color = ePunctuation;
        else if (parseString (srcBegin, srcEnd, tokenBegin, tokenEnd))
          color = eString;
        else if (parseLiteral (srcBegin, srcEnd, tokenBegin, tokenEnd))
          color = eLiteral;
        return (color != eUndefined);
        };

      // regex for preProc
      language.mRegexList.push_back (
        make_pair (regex ("[ \\t]*#[ \\t]*[a-zA-Z_]+", regex_constants::optimize), (uint8_t)ePreProc));

      return language;
      }
    //}}}
    //{{{
    static const cLanguage hlsl() {

      cLanguage language;

      language.mName = "HLSL";

      // comment tokens
      language.mCommentBegin = "/*";
      language.mCommentEnd = "*/";
      language.mCommentSingle = "//";

      // fold tokens
      language.mFoldBeginToken = "#{{{";
      language.mFoldEndToken = "#}}}";

      for (const auto& keyWord : kHlslKeyWords)
        language.mKeyWords.insert (keyWord);
      for (const auto& knownWord : kHlslKnownWords)
        language.mKnownWords.insert (knownWord);

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

      return language;
      }
    //}}}
    //{{{
    static const cLanguage glsl() {

      cLanguage language;

      language.mName = "GLSL";

      // comment tokens
      language.mCommentBegin = "/*";
      language.mCommentEnd = "*/";
      language.mCommentSingle = "//";

      // fold tokens
      language.mFoldBeginToken = "#{{{";
      language.mFoldEndToken = "#}}}";

      for (const auto& keyWord : kGlslKeyWords)
        language.mKeyWords.insert (keyWord);
      for (const auto& knownWord : kGlslKnownWords)
        language.mKnownWords.insert (knownWord);

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

      return language;
      }
    //}}}

    // vars
    string mName;

    // comment tokens
    string mCommentSingle;
    string mCommentBegin;
    string mCommentEnd;

    // fold tokens
    string mFoldBeginToken;
    string mFoldEndToken;

    // fold indicators
    string mFoldBeginOpen = "{{{ ";
    string mFoldBeginClosed = "... ";
    string mFoldEnd = "}}}";

    unordered_set <string> mKeyWords;
    unordered_set <string> mKnownWords;

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

    //{{{
    string getString() const {

       string lineString;
       lineString.reserve (mGlyphs.size());

       for (auto& glyph : mGlyphs)
         for (uint32_t utf8index = 0; utf8index < glyph.mNumUtf8Bytes; utf8index++)
           lineString += glyph.mUtfChar[utf8index];

       return lineString;
       }
    //}}}
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
    //{{{
    uint32_t trimTrailingSpace() {

      uint32_t trimmedSpaces = 0;
      uint32_t column = getNumGlyphs();
      while ((column > 0) && (mGlyphs[--column].mUtfChar[0] == ' ')) {
        // trailingSpace, trim it
        mGlyphs.pop_back();
        trimmedSpaces++;
        }

      return trimmedSpaces;
      }
    //}}}
    //{{{
    void parse (const cLanguage& language) {
    // parse for fold stuff, tokenize and look for keyWords, Known

      //cLog::log (LOGINFO, fmt::format ("parseLine {}", lineNumber));
      mIndent = 0;
      mFirstGlyph = 0;
      mCommentBegin = false;
      mCommentEnd = false;
      mCommentSingle = false;
      mFoldBegin = false;
      mFoldEnd = false;

      // ????????????? is this right ????????
      mFoldOpen = false;

      if (empty())
        return;

      // create glyphs string
      string glyphString = getString();

      setColor (eText);
      //{{{  find indent
      size_t indentPos = glyphString.find_first_not_of (' ');
      mIndent = (indentPos == string::npos) ? 0 : static_cast<uint8_t>(indentPos);
      //}}}
      //{{{  find commentSingle token
      size_t pos = indentPos;

      do {
        pos = glyphString.find (language.mCommentSingle, pos);
        if (pos != string::npos)
          for (size_t i = 0; i < language.mCommentSingle.size(); i++)
            setCommentSingle (pos++);
        } while (pos != string::npos);
      //}}}
      //{{{  find commentBegin token
      pos = indentPos;

      do {
        pos = glyphString.find (language.mCommentBegin, pos);
        if (pos != string::npos)
          for (size_t i = 0; i < language.mCommentBegin.size(); i++)
            setCommentBegin (pos++);
        } while (pos != string::npos);
      //}}}
      //{{{  find commentEnd token
      pos = indentPos;

      do {
        pos = glyphString.find (language.mCommentEnd, pos);
        if (pos != string::npos)
          for (size_t i = 0; i < language.mCommentEnd.size(); i++)
            setCommentEnd (pos++);
        } while (pos != string::npos);
      //}}}
      //{{{  find foldBegin token
      size_t foldBeginPos = glyphString.find (language.mFoldBeginToken, indentPos);
      mFoldBegin = (foldBeginPos != string::npos) && (foldBeginPos == indentPos);
      //}}}
      //{{{  find foldEnd token
      size_t foldEndPos = glyphString.find (language.mFoldEndToken, indentPos);
      mFoldEnd = (foldEndPos != string::npos) && (foldEndPos == indentPos);
      //}}}

      parseTokens (language, glyphString);
      }
    //}}}
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

    //{{{  vars
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
    //}}}

  private:
    //{{{
    void parseTokens (const cLanguage& language, const string& textString) {
    // parse and color tokens, recognise and color keyWords and knownWords

      const char* strBegin = &textString.front();
      const char* strEnd = strBegin + textString.size();
      const char* strPtr = strBegin;
      while (strPtr < strEnd) {
        // faster tokenize search
        const char* tokenBegin = nullptr;
        const char* tokenEnd = nullptr;
        uint8_t tokenColor = eText;
        bool tokenFound = language.mTokenSearch &&
            language.mTokenSearch (strPtr, strEnd, tokenBegin, tokenEnd, tokenColor);

        if (!tokenFound) {
          // slower regex search
          for (const auto& p : language.mRegexList) {
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
            if (language.mKeyWords.count (tokenString) != 0)
              tokenColor = eKeyWord;
            else if (language.mKnownWords.count (tokenString) != 0)
              tokenColor = eKnownWord;
            }

          // color token glyphs
          uint32_t glyphIndex = static_cast<uint32_t>(tokenBegin - strBegin);
          uint32_t glyphIndexEnd = static_cast<uint32_t>(tokenEnd - strBegin);
          while (glyphIndex < glyphIndexEnd)
            setColor (glyphIndex++, tokenColor);

          strPtr = tokenEnd;
          }
        else
          strPtr++;
        }
      }
    //}}}

    vector <cGlyph> mGlyphs;
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
  //{{{
  class cFedDocument {
  public:
    //{{{
    cFedDocument() {
      mLanguage = cLanguage::c();
      addEmptyLine();
      }
    //}}}
    virtual ~cFedDocument() = default;

    //{{{  gets
    bool isEdited() const { return mEdited; }
    bool hasFolds() const { return mHasFolds; }
    bool hasUtf8() const { return mHasUtf8; }

    cLanguage& getLanguage() { return mLanguage; }
    uint32_t getTabSize() const { return mTabSize; }

    string getFileName() const { return mFileStem + mFileExtension; }
    string getFilePath() const { return mFilePath; }

    uint32_t getNumLines() const { return static_cast<uint32_t>(mLines.size()); }
    uint32_t getMaxLineNumber() const { return getNumLines() - 1; }

    //{{{
    string getText (const sPosition& beginPosition, const sPosition& endPosition) {
    // get position range as string with lineFeed line breaks

      // count approx numChars
      size_t numChars = 0;
      for (uint32_t lineNumber = beginPosition.mLineNumber; lineNumber < endPosition.mLineNumber; lineNumber++)
        numChars += getLine (lineNumber).getNumGlyphs();

      // reserve text size
      string text;
      text.reserve (numChars);

      uint32_t lineNumber = beginPosition.mLineNumber;
      uint32_t glyphIndex = getGlyphIndex (beginPosition);
      uint32_t endGlyphIndex = getGlyphIndex (endPosition);
      while ((glyphIndex < endGlyphIndex) || (lineNumber < endPosition.mLineNumber)) {
        if (lineNumber >= getNumLines())
          break;

        if (glyphIndex < getLine (lineNumber).getNumGlyphs()) {
          text += getLine (lineNumber).getChar (glyphIndex);
          glyphIndex++;
          }
        else {
          lineNumber++;
          glyphIndex = 0;
          text += '\n';
          }
        }

      return text;
      }
    //}}}
    string getTextAll() { return getText ({0,0}, { getNumLines(),0}); }
    //{{{
    vector<string> getTextStrings() const {
    // simple get text as vector of string

      vector<string> result;
      result.reserve (getNumLines());

      for (const auto& line : mLines) {
        string lineString = line.getString();
        result.emplace_back (move (lineString));
        }

      return result;
      }
    //}}}

    cLine& getLine (uint32_t lineNumber) { return mLines[lineNumber]; }
    //{{{
    uint32_t getNumColumns (const cLine& line) {

      uint32_t column = 0;

      for (uint32_t glyphIndex = 0; glyphIndex < line.getNumGlyphs(); glyphIndex++)
        if (line.getChar (glyphIndex) == '\t')
          column = getTabColumn (column);
        else
          column++;

      return column;
      }
    //}}}

    //{{{
    uint32_t getGlyphIndex (const cLine& line, uint32_t toColumn) {
    // return glyphIndex from line,column, inserting tabs whose width is owned by cFedDocument

      uint32_t glyphIndex = 0;

      uint32_t column = 0;
      while ((glyphIndex < line.getNumGlyphs()) && (column < toColumn)) {
        if (line.getChar (glyphIndex) == '\t')
          column = getTabColumn (column);
        else
          column++;
        glyphIndex++;
        }

      return glyphIndex;
      }
    //}}}
    uint32_t getGlyphIndex (const sPosition& position) { return getGlyphIndex (getLine (position.mLineNumber), position.mColumn); }

    //{{{
    uint32_t getColumn (const cLine& line, uint32_t toGlyphIndex) {
    // return glyphIndex column using any tabs

      uint32_t column = 0;

      for (uint32_t glyphIndex = 0; glyphIndex < line.getNumGlyphs(); glyphIndex++) {
        if (glyphIndex >= toGlyphIndex)
          return column;
        if (line.getChar (glyphIndex) == '\t')
          column = getTabColumn (column);
        else
          column++;
        }

      return column;
      }
    //}}}
    //{{{
    uint32_t getTabColumn (uint32_t column) {
    // return column of after tab at column
      return ((column / mTabSize) * mTabSize) + mTabSize;
      }
    //}}}
    //}}}

    // insert
    //{{{
    void addEmptyLine() {
      mLines.push_back (cLine());
      }
    //}}}
    //{{{
    void insertChar (cLine& line, uint32_t glyphIndex, ImWchar ch) {

      line.insert (glyphIndex, cGlyph (ch, eText));
      parse (line);
      edited();
      }
    //}}}

    // line
    //{{{
    void appendLineToPrev (uint32_t lineNumber) {
    // append lineNumber to prev line

      cLine& line = getLine (lineNumber-1);
      line.appendLine (getLine (lineNumber), 0);
      mLines.erase (mLines.begin() + lineNumber);
      parse (line);

      edited();
      }
    //}}}
    //{{{
    void joinLine (cLine& line, uint32_t lineNumber) {
    // join lineNumber to line

      // append lineNumber to joinToLine
      line.appendLine (getLine (lineNumber), 0);
      parse (line);

      // delete lineNumber
      mLines.erase (mLines.begin() + lineNumber);
      edited();
      }
    //}}}
    //{{{
    void breakLine (cLine& line, uint32_t glyphIndex, uint32_t newLineNumber, uint32_t indent) {

      // insert newLine at newLineNumber
      cLine& newLine = *mLines.insert (mLines.begin() + newLineNumber, cLine(indent));

      // append from glyphIndex of line to newLine
      newLine.appendLine (line, glyphIndex);
      parse (newLine);

      // erase from glyphIndex of line
      line.eraseToEnd (glyphIndex);
      parse (line);

      edited();
      }
    //}}}

    // delete
    //{{{
    void deleteChar (cLine& line, uint32_t glyphIndex) {

      line.erase (glyphIndex);
      parse (line);

      edited();
      }
    //}}}
    //{{{
    void deleteChar (cLine& line, const sPosition& position) {
      deleteChar (line, getGlyphIndex (line, position.mColumn));
      }
    //}}}
    //{{{
    void deleteLine (uint32_t lineNumber) {

      mLines.erase (mLines.begin() + lineNumber);
      edited();
      }
    //}}}
    //{{{
    void deleteLineRange (uint32_t beginLineNumber, uint32_t endLineNumber) {

      mLines.erase (mLines.begin() + beginLineNumber, mLines.begin() + endLineNumber);
      edited();
      }
    //}}}
    //{{{
    void deletePositionRange (const sPosition& beginPosition, const sPosition& endPosition) {
    /// !!! need more glyphsLine !!!!

      if (endPosition == beginPosition)
        return;

      uint32_t beginGlyphIndex = getGlyphIndex (beginPosition);
      uint32_t endGlyphIndex = getGlyphIndex (endPosition);

      if (beginPosition.mLineNumber == endPosition.mLineNumber) {
        // delete in same line
        cLine& line = getLine (beginPosition.mLineNumber);
        uint32_t maxColumn = getNumColumns (line);
        if (endPosition.mColumn >= maxColumn)
          line.eraseToEnd (beginGlyphIndex);
        else
          line.erase (beginGlyphIndex, endGlyphIndex);

        parse (line);
        }

      else {
        // delete over multiple lines
        // - delete backend of beginLine
        cLine& beginLine = getLine (beginPosition.mLineNumber);
        beginLine.eraseToEnd (beginGlyphIndex);

        // delete frontend of endLine
        cLine& endLine = getLine (endPosition.mLineNumber);
        endLine.erase (0, endGlyphIndex);

        // append endLine remainder to beginLine
        if (beginPosition.mLineNumber < endPosition.mLineNumber)
          beginLine.appendLine (endLine, 0);

        // delete middle whole lines
        if (beginPosition.mLineNumber < endPosition.mLineNumber)
          deleteLineRange (beginPosition.mLineNumber + 1, endPosition.mLineNumber + 1);

        parse (beginLine);
        parse (endLine);
        }

      edited();
      }
    //}}}

    // parse
    void parse (cLine& line) { line.parse (mLanguage); };
    //{{{
    void parseAll() {
    // simple parse whole document for comments, folds
    // - assumes lines have already been parsed

      if (!mParseFlag)
        return;
      mParseFlag = false;

      mHasFolds = false;
      bool inString = false;
      bool inSingleComment = false;
      bool inBeginEndComment = false;

      uint32_t glyphIndex = 0;
      uint32_t lineNumber = 0;
      while (lineNumber < getNumLines()) {
        cLine& line = getLine (lineNumber);
        mHasFolds |= line.mFoldBegin;

        uint32_t numGlyphs = line.getNumGlyphs();
        if (numGlyphs > 0) {
          // parse ch
          uint8_t ch = line.getChar (glyphIndex);
          if (ch == '\"') {
            //{{{  start of string
            inString = true;
            if (inSingleComment || inBeginEndComment)
              line.setColor (glyphIndex, eComment);
            }
            //}}}
          else if (inString) {
            //{{{  in string
            if (inSingleComment || inBeginEndComment)
              line.setColor (glyphIndex, eComment);

            if (ch == '\"') // end of string
              inString = false;

            else if (ch == '\\') {
              // \ escapeChar in " " quotes, skip nextChar if any
              if (glyphIndex+1 < numGlyphs) {
                glyphIndex++;
                if (inSingleComment || inBeginEndComment)
                  line.setColor (glyphIndex, eComment);
                }
              }
            }
            //}}}
          else {
            // comment begin?
            if (line.getCommentSingle (glyphIndex))
              inSingleComment = true;
            else if (line.getCommentBegin (glyphIndex))
              inBeginEndComment = true;

            // in comment
            if (inSingleComment || inBeginEndComment)
              line.setColor (glyphIndex, eComment);

            // comment end ?
            if (line.getCommentEnd (glyphIndex))
              inBeginEndComment = false;
            }
          glyphIndex++;
          }

        if (glyphIndex >= numGlyphs) {
          // end of line, check for trailing concatenate '\' char
          if ((numGlyphs == 0) || (line.getChar (numGlyphs-1) != '\\')) {
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
    //}}}
    //{{{
    void edited() {

      mParseFlag = true;
      mEdited |= true;
      }
    //}}}

    // file
    //{{{
    void load (const string& filename) {

      // parse filename path
      filesystem::path filePath (filename);
      mFilePath = filePath.string();
      mParentPath = filePath.parent_path().string();
      mFileStem = filePath.stem().string();
      mFileExtension = filePath.extension().string();

      cLog::log (LOGINFO, fmt::format ("setFile  {}", filename));
      cLog::log (LOGINFO, fmt::format ("- path   {}", mFilePath));
      cLog::log (LOGINFO, fmt::format ("- parent {}", mParentPath));
      cLog::log (LOGINFO, fmt::format ("- stem   {}", mFileStem));
      cLog::log (LOGINFO, fmt::format ("- ext    {}", mFileExtension));

      // clear doc
      mLines.clear();

      uint32_t utf8chars = 0;

      string lineString;
      uint32_t lineNumber = 0;

      // read filename as stream
      ifstream stream (filename, ifstream::in);
      while (getline (stream, lineString)) {
        // create empty line, reserve enough glyphs for line
        mLines.emplace_back (cLine());
        mLines.back().reserve (lineString.size());

        // iterate char
        for (auto it = lineString.begin(); it < lineString.end(); ++it) {
          char ch = *it;
          if (ch == '\r') // CR ignored, but set flag
              mHasCR = true;
          else {
            if (ch ==  '\t')
                mHasTabs = true;

            uint8_t numUtf8Bytes = cGlyph::numUtf8Bytes(ch);
            if (numUtf8Bytes == 1)
              mLines.back().emplaceBack (cGlyph (ch, 0)); // eText
            else {
              array <uint8_t,7> utf8Bytes = {0};
              string utf8String;
              for (uint32_t i = 0; i < numUtf8Bytes; i++) {
                utf8Bytes[i] = (i == 0) ? ch : *it++;
                utf8String += fmt::format ("{:2x} ", utf8Bytes[i]);
                }
              utf8chars++;
              //cLog::log (LOGINFO, fmt::format ("loading utf8 {} {}", size, utf8String));
              mLines.back().emplaceBack (cGlyph (utf8Bytes.data(), numUtf8Bytes, 0)); //eText
              mHasUtf8 = true;
              }
            }
          }
        lineNumber++;
        }

      trimTrailingSpace();

      // add empty lastLine
      mLines.emplace_back (cLine());

      cLog::log (LOGINFO, fmt::format ("read {}:lines {}{}{}",
                                       lineNumber,
                                       mHasCR ? "hasCR " : "",
                                       mHasUtf8 ? "hasUtf8 " : "",
                                       utf8chars));
      for (auto& line : mLines)
        parse (line);

      mParseFlag = true;
      mEdited = false;
      }
    //}}}
    //{{{
    void save() {

      if (!mEdited) {
        cLog::log (LOGINFO,fmt::format ("{} unchanged, no save", mFilePath));
        return;
        }

      // identify filePath for previous version
      filesystem::path saveFilePath (mFilePath);
      saveFilePath.replace_extension (fmt::format ("{};{}", mFileExtension, mVersion++));
      while (filesystem::exists (saveFilePath)) {
        // version exits, increment version number
        cLog::log (LOGINFO,fmt::format ("skipping {}", saveFilePath.string()));
        saveFilePath.replace_extension (fmt::format ("{};{}", mFileExtension, mVersion++));
        }

      uint32_t highestLineNumber = trimTrailingSpace();

      // save ofstream
      ofstream stream (saveFilePath);
      for (uint32_t lineNumber = 0; lineNumber < highestLineNumber; lineNumber++) {
        string lineString = mLines[lineNumber].getString();
        stream.write (lineString.data(), lineString.size());
        stream.put ('\n');
        }

      // done
      cLog::log (LOGINFO,fmt::format ("{} saved", saveFilePath.string()));
      }
    //}}}

  private:
    //{{{
    uint32_t trimTrailingSpace() {
    // trim trailing space
    // - return highest nonEmpty lineNumber

      uint32_t nonEmptyHighWaterMark = 0;

      uint32_t lineNumber = 0;
      uint32_t trimmedSpaces = 0;
      for (auto& line : mLines) {
        trimmedSpaces += line.trimTrailingSpace();
        if (!line.empty()) // nonEmpty line, raise waterMark
          nonEmptyHighWaterMark = lineNumber;
        lineNumber++;
        }

      if ((nonEmptyHighWaterMark != mLines.size()-1) || (trimmedSpaces > 0))
        cLog::log (LOGINFO, fmt::format ("highest {}:{} trimmedSpaces:{}",
                                         nonEmptyHighWaterMark+1, mLines.size(), trimmedSpaces));
      return nonEmptyHighWaterMark;
      }
    //}}}

    //{{{  vars
    vector<cLine> mLines;

    string mFilePath;
    string mParentPath;
    string mFileStem;
    string mFileExtension;
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
  //}}}

  //{{{
  class cEditApp : public cApp {
  public:
    cEditApp (const cPoint& windowSize, bool fullScreen, bool vsync)
      : cApp ("fed", windowSize, fullScreen, vsync) {}
    virtual ~cEditApp() = default;

    bool getMemEdit() const { return mMemEdit; };
    cFedDocument* getFedDocument() const { return mFedDocuments.back(); }

    //{{{
    bool setDocumentName (const string& filename, bool memEdit) {

      mFilename = cFileUtils::resolve (filename);

      cFedDocument* document = new cFedDocument();
      document->load (mFilename);
      mFedDocuments.push_back (document);

      mMemEdit = memEdit;

      return true;
      }
    //}}}
    //{{{
    void drop (const vector<string>& dropItems) {

      for (auto& item : dropItems) {
        string filename = cFileUtils::resolve (item);
        setDocumentName (filename, mMemEdit);

        cLog::log (LOGINFO, filename);
        }
      }
    //}}}

  private:
    bool mMemEdit = false;
    string mFilename;
    vector<cFedDocument*> mFedDocuments;
    };
  //}}}
  //{{{
  class cFedEdit {
  public:
    //{{{
    cFedEdit() {

      cursorFlashOn();
      }
    //}}}
    ~cFedEdit() = default;

    enum class eSelect { eNormal, eWord, eLine };
    //{{{  gets
    cFedDocument* getFedDocument() { return mFedDocument; }
    cLanguage& getLanguage() { return mFedDocument->getLanguage(); }

    bool isReadOnly() const { return mOptions.mReadOnly; }
    bool isShowFolds() const { return mOptions.mShowFolded; }

    // has
    bool hasSelect() const { return mEdit.mCursor.mSelectEndPosition > mEdit.mCursor.mSelectBeginPosition; }
    bool hasUndo() const { return !mOptions.mReadOnly && mEdit.hasUndo(); }
    bool hasRedo() const { return !mOptions.mReadOnly && mEdit.hasRedo(); }
    bool hasPaste() const { return !mOptions.mReadOnly && mEdit.hasPaste(); }

    // get
    string getTextString();
    vector<string> getTextStrings() const;

    sPosition getCursorPosition() { return sanitizePosition (mEdit.mCursor.mPosition); }
    //}}}
    //{{{  sets
    void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }

    void toggleReadOnly() { mOptions.mReadOnly = !mOptions.mReadOnly; }
    void toggleOverWrite() { mOptions.mOverWrite = !mOptions.mOverWrite; }

    void toggleShowLineNumber() { mOptions.mShowLineNumber = !mOptions.mShowLineNumber; }
    void toggleShowLineDebug() { mOptions.mShowLineDebug = !mOptions.mShowLineDebug; }
    void toggleShowWhiteSpace() { mOptions.mShowWhiteSpace = !mOptions.mShowWhiteSpace; }
    void toggleShowMonoSpaced() { mOptions.mShowMonoSpaced = !mOptions.mShowMonoSpaced; }

    //{{{
    void toggleShowFolded() {

      mOptions.mShowFolded = !mOptions.mShowFolded;
      mEdit.mScrollVisible = true;
      }
    //}}}
    //}}}
    //{{{  actions
    // move
    //{{{
    void moveLeft() {

      deselect();
      sPosition position = mEdit.mCursor.mPosition;

      // line
      cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

      // column
      uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
      if (glyphIndex > glyphsLine.mFirstGlyph)
        // move to prevColumn on sameLine
        setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex - 1)});

      else if (position.mLineNumber > 0) {
        // at begining of line, move to end of prevLine
        uint32_t moveLineIndex = getLineIndexFromNumber (position.mLineNumber) - 1;
        uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
        cLine& moveGlyphsLine = getGlyphsLine (moveLineNumber);
        setCursorPosition ({moveLineNumber, mFedDocument->getColumn (moveGlyphsLine, moveGlyphsLine.getNumGlyphs())});
        }
      }
    //}}}
    //{{{
    void moveRight() {

      deselect();
      sPosition position = mEdit.mCursor.mPosition;

      // line
      cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

      // column
      uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
      if (glyphIndex < glyphsLine.getNumGlyphs())
        // move to nextColumm on sameLine
        setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex + 1)});

      else {
        // move to start of nextLine
        uint32_t moveLineIndex = getLineIndexFromNumber (position.mLineNumber) + 1;
        if (moveLineIndex < getNumFoldLines()) {
          uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
          cLine& moveGlyphsLine = getGlyphsLine (moveLineNumber);
          setCursorPosition ({moveLineNumber, moveGlyphsLine.mFirstGlyph});
          }
        }
      }
    //}}}
    //{{{
    void moveRightWord() {

      deselect();
      sPosition position = mEdit.mCursor.mPosition;

      // line
      uint32_t glyphsLineNumber = getGlyphsLineNumber (position.mLineNumber);
      cLine& glyphsLine = mFedDocument->getLine (glyphsLineNumber);

      // find nextWord
      sPosition movePosition;
      uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
      if (glyphIndex < glyphsLine.getNumGlyphs()) {
        // middle of line, find next word
        bool skip = false;
        bool isWord = false;
        if (glyphIndex < glyphsLine.getNumGlyphs()) {
          isWord = isalnum (glyphsLine.getChar (glyphIndex));
          skip = isWord;
          }

        while ((!isWord || skip) && (glyphIndex < glyphsLine.getNumGlyphs())) {
          isWord = isalnum (glyphsLine.getChar (glyphIndex));
          if (isWord && !skip) {
            setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex)});
            return;
            }
          if (!isWord)
            skip = false;
          glyphIndex++;
          }

        setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex)});
        }
      else {
        // !!!! implement inc to next line !!!!!
        }
      }
    //}}}
    void moveLineUp()   { moveUp (1); }
    void moveLineDown() { moveDown (1); }
    void movePageUp()   { moveUp (getNumPageLines() - 4); }
    void movePageDown() { moveDown (getNumPageLines() - 4); }
    void moveHome() { setCursorPosition ({0,0}); }
    void moveEnd() { setCursorPosition ({mFedDocument->getMaxLineNumber(), 0}); }

    // select
    //{{{
    void selectAll() {
      setSelect (eSelect::eNormal, {0,0}, { mFedDocument->getNumLines(),0});
      }
    //}}}

    // cut and paste
    //{{{
    void copy() {

      if (!canEditAtCursor())
        return;

      if (hasSelect()) {
        // push selectedText to pasteStack
        mEdit.pushPasteText (getSelectText());
        deselect();
        }

      else {
        // push currentLine text to pasteStack
        sPosition position {getCursorPosition().mLineNumber, 0};
        mEdit.pushPasteText (mFedDocument->getText (position, getNextLinePosition (position)));

        // moveLineDown for any multiple copy
        moveLineDown();
        }
      }
    //}}}
    //{{{
    void cut() {

      cUndo undo;
      undo.mBeforeCursor = mEdit.mCursor;

      if (hasSelect()) {
        // push selectedText to pasteStack
        string text = getSelectText();
        mEdit.pushPasteText (text);

        // cut selected range
        undo.mDeleteText = text;
        undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
        undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
        deleteSelect();
        }

      else {
        // push current line text to pasteStack
        sPosition position {getCursorPosition().mLineNumber,0};
        sPosition nextLinePosition = getNextLinePosition (position);

        string text = mFedDocument->getText (position, nextLinePosition);
        mEdit.pushPasteText (text);

        // cut current line
        undo.mDeleteText = text;
        undo.mDeleteBeginPosition = position;
        undo.mDeleteEndPosition = nextLinePosition;
        mFedDocument->deleteLineRange (position.mLineNumber, nextLinePosition.mLineNumber);
        }

      undo.mAfterCursor = mEdit.mCursor;
      mEdit.addUndo (undo);
      }
    //}}}
    //{{{
    void paste() {

      if (hasPaste()) {
        cUndo undo;
        undo.mBeforeCursor = mEdit.mCursor;

        if (hasSelect()) {
          // delete selectedText
          undo.mDeleteText = getSelectText();
          undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
          undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
          deleteSelect();
          }

        string text = mEdit.popPasteText();
        undo.mAddText = text;
        undo.mAddBeginPosition = getCursorPosition();

        insertText (text);

        undo.mAddEndPosition = getCursorPosition();
        undo.mAfterCursor = mEdit.mCursor;
        mEdit.addUndo (undo);

        // moveLineUp for any multiple paste
        moveLineUp();
        }
      }
    //}}}

    // delete
    //{{{
    void deleteIt() {

      cUndo undo;
      undo.mBeforeCursor = mEdit.mCursor;

      sPosition position = getCursorPosition();
      setCursorPosition (position);
      cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

      if (hasSelect()) {
        // delete selected range
        undo.mDeleteText = getSelectText();
        undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
        undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
        deleteSelect();
        }

      else if (position.mColumn == mFedDocument->getNumColumns (glyphsLine)) {
        if (position.mLineNumber >= mFedDocument->getMaxLineNumber())
          // no next line to join
          return;

        undo.mDeleteText = '\n';
        undo.mDeleteBeginPosition = getCursorPosition();
        undo.mDeleteEndPosition = advancePosition (undo.mDeleteBeginPosition);

        // join next line to this line
        mFedDocument->joinLine (glyphsLine, position.mLineNumber + 1);
        }

      else {
        // delete character
        undo.mDeleteBeginPosition = getCursorPosition();
        undo.mDeleteEndPosition = undo.mDeleteBeginPosition;
        undo.mDeleteEndPosition.mColumn++;
        undo.mDeleteText = mFedDocument->getText (undo.mDeleteBeginPosition, undo.mDeleteEndPosition);

        mFedDocument->deleteChar (glyphsLine, position);
        }

      undo.mAfterCursor = mEdit.mCursor;
      mEdit.addUndo (undo);
      }
    //}}}
    //{{{
    void backspace() {

      cUndo undo;
      undo.mBeforeCursor = mEdit.mCursor;

      sPosition position = getCursorPosition();
      cLine& line = mFedDocument->getLine (position.mLineNumber);

      if (hasSelect()) {
        // delete select
        undo.mDeleteText = getSelectText();
        undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
        undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;
        deleteSelect();
        }

      else if (position.mColumn <= line.mFirstGlyph) {
        // backspace at beginning of line, append line to previous line
        if (isFolded() && line.mFoldBegin) // don't backspace to prevLine at foldBegin
          return;
        if (!position.mLineNumber) // already on firstLine, nowhere to go
          return;

        // previous Line
        uint32_t prevLineNumber = position.mLineNumber - 1;
        cLine& prevLine = mFedDocument->getLine (prevLineNumber);
        uint32_t prevLineEndColumn = mFedDocument->getNumColumns (prevLine);

        // save lineFeed undo
        undo.mDeleteText = '\n';
        undo.mDeleteBeginPosition = {prevLineNumber, prevLineEndColumn};
        undo.mDeleteEndPosition = advancePosition (undo.mDeleteBeginPosition);

        mFedDocument->appendLineToPrev (position.mLineNumber);

        // position to end of prevLine
        setCursorPosition ({prevLineNumber, prevLineEndColumn});
        }

      else {
        // at middle of line, delete previous char
        cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
        uint32_t prevGlyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn) - 1;

        // save previous char undo
        undo.mDeleteEndPosition = getCursorPosition();
        undo.mDeleteBeginPosition = getCursorPosition();
        undo.mDeleteBeginPosition.mColumn--;
        undo.mDeleteText += glyphsLine.getChar (prevGlyphIndex);

        // delete previous char
        mFedDocument->deleteChar (glyphsLine, prevGlyphIndex);

        // position to previous char
        setCursorPosition ({position.mLineNumber, position.mColumn - 1});
        }

      undo.mAfterCursor = mEdit.mCursor;
      mEdit.addUndo (undo);
      }
    //}}}

    // enter
    //{{{
    void enterCharacter (ImWchar ch) {
    // !!!! more utf8 handling !!!

      if (!canEditAtCursor())
        return;

      cUndo undo;
      undo.mBeforeCursor = mEdit.mCursor;

      if (hasSelect()) {
        //{{{  delete selected range
        undo.mDeleteText = getSelectText();
        undo.mDeleteBeginPosition = mEdit.mCursor.mSelectBeginPosition;
        undo.mDeleteEndPosition = mEdit.mCursor.mSelectEndPosition;

        deleteSelect();
        }
        //}}}

      sPosition position = getCursorPosition();
      undo.mAddBeginPosition = position;

      cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
      uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);

      if (ch == '\n') {
        // newLine
        if (isFolded() && mFedDocument->getLine (position.mLineNumber).mFoldBegin)
          // no newLine in folded foldBegin - !!!! could allow with no cursor move, still odd !!!
          return;

        // break glyphsLine at glyphIndex, with new line indent
        mFedDocument->breakLine (glyphsLine, glyphIndex, position.mLineNumber + 1, glyphsLine.mIndent);

        // set cursor to newLine, start of indent
        setCursorPosition ({position.mLineNumber+1, glyphsLine.mIndent});
        }
      else {
        // char
        if (mOptions.mOverWrite && (glyphIndex < glyphsLine.getNumGlyphs())) {
          // overwrite by deleting cursor char
          undo.mDeleteBeginPosition = mEdit.mCursor.mPosition;
          undo.mDeleteEndPosition = {position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex+1)};
          undo.mDeleteText += glyphsLine.getChar (glyphIndex);
          glyphsLine.erase (glyphIndex);
          }

        // insert new char
        mFedDocument->insertChar (glyphsLine, glyphIndex, ch);
        setCursorPosition ({position.mLineNumber, mFedDocument->getColumn (glyphsLine, glyphIndex + 1)});
        }
      undo.mAddText = static_cast<char>(ch); // !!!! = utf8buf.data() utf8 handling needed !!!!

      undo.mAddEndPosition = getCursorPosition();
      undo.mAfterCursor = mEdit.mCursor;
      mEdit.addUndo (undo);
      }
    //}}}
    void enterKey() { enterCharacter ('\n'); }
    void tabKey() { enterCharacter ('\t'); }

    // undo
    //{{{
    void undo (uint32_t steps = 1) {

      if (hasUndo()) {
        mEdit.undo (this, steps);
        scrollCursorVisible();
        }
      }
    //}}}
    //{{{
    void redo (uint32_t steps = 1) {

      if (hasRedo()) {
        mEdit.redo (this, steps);
        scrollCursorVisible();
        }
      }
    //}}}

    // fold
    //{{{
    void createFold() {

      // !!!! temp string for now !!!!
      string text = getLanguage().mFoldBeginToken +
                     "  new fold - loads of detail to implement\n\n" +
                    getLanguage().mFoldEndToken +
                     "\n";
      cUndo undo;
      undo.mBeforeCursor = mEdit.mCursor;
      undo.mAddText = text;
      undo.mAddBeginPosition = getCursorPosition();

      insertText (text);

      undo.mAddEndPosition = getCursorPosition();
      undo.mAfterCursor = mEdit.mCursor;
      mEdit.addUndo (undo);
      }
    //}}}
    void openFold() { openFold (mEdit.mCursor.mPosition.mLineNumber); }
    void openFoldOnly() { openFoldOnly (mEdit.mCursor.mPosition.mLineNumber); }
    void closeFold() { closeFold (mEdit.mCursor.mPosition.mLineNumber); }
    //}}}

    //{{{
    void draw (cApp& app) {
    // standalone fed window

       if (!mInited) {
         // push any clipboardText to pasteStack
         const char* clipText = ImGui::GetClipboardText();
         if (clipText && (strlen (clipText)) > 0)
           mEdit.pushPasteText (clipText);
         mInited = true;
         }

       // create document editor
      cEditApp& fedApp = (cEditApp&)app;
      mFedDocument = fedApp.getFedDocument();
      if (mFedDocument) {
        bool open = true;
        ImGui::Begin ("fed", &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
        drawContents (app);
        ImGui::End();
        }
      }
    //}}}
    //{{{
    void drawContents (cApp& app) {
    // main ui draw

      // check for delayed all document parse
      mFedDocument->parseAll();
      //{{{  draw top line buttons
      //{{{  lineNumber buttons
      if (toggleButton ("line", mOptions.mShowLineNumber))
        toggleShowLineNumber();

      if (mOptions.mShowLineNumber)
        // debug button
        if (mOptions.mShowLineNumber) {
          ImGui::SameLine();
          if (toggleButton ("debug", mOptions.mShowLineDebug))
            toggleShowLineDebug();
          }
      //}}}

      if (mFedDocument->hasFolds()) {
        //{{{  folded button
        ImGui::SameLine();
        if (toggleButton ("folded", isShowFolds()))
          toggleShowFolded();
        }
        //}}}

      //{{{  monoSpaced buttom
      ImGui::SameLine();
      if (toggleButton ("mono", mOptions.mShowMonoSpaced))
        toggleShowMonoSpaced();
      //}}}
      //{{{  whiteSpace button
      ImGui::SameLine();
      if (toggleButton ("space", mOptions.mShowWhiteSpace))
        toggleShowWhiteSpace();
      //}}}

      if (hasPaste()) {
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

      //{{{  insert button
      ImGui::SameLine();
      if (toggleButton ("insert", !mOptions.mOverWrite))
        toggleOverWrite();
      //}}}
      //{{{  readOnly button
      ImGui::SameLine();
      if (toggleButton ("readOnly", isReadOnly()))
        toggleReadOnly();
      //}}}

      //{{{  fontSize button
      ImGui::SameLine();
      ImGui::SetNextItemWidth (3 * ImGui::GetFontSize());
      ImGui::DragInt ("##fs", &mOptions.mFontSize, 0.2f, mOptions.mMinFontSize, mOptions.mMaxFontSize, "%d");

      if (ImGui::IsItemHovered())
        mOptions.mFontSize = max (mOptions.mMinFontSize, min (mOptions.mMaxFontSize,
                               mOptions.mFontSize + static_cast<int>(ImGui::GetIO().MouseWheel)));
      //}}}
      //{{{  vsync button,fps
      if (app.getPlatform().hasVsync()) {
        // vsync button
        ImGui::SameLine();
        if (toggleButton ("vSync", app.getPlatform().getVsync()))
            app.getPlatform().toggleVsync();

        // fps text
        ImGui::SameLine();
        ImGui::Text (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
        }
      //}}}

      //{{{  fullScreen button
      if (app.getPlatform().hasFullScreen()) {
        ImGui::SameLine();
        if (toggleButton ("full", app.getPlatform().getFullScreen()))
          app.getPlatform().toggleFullScreen();
        }
      //}}}
      //{{{  save button
      if (mFedDocument->isEdited()) {
        ImGui::SameLine();
        if (ImGui::Button ("save"))
          mFedDocument->save();
        }
      //}}}

      //{{{  info
      ImGui::SameLine();
      ImGui::Text (fmt::format ("{}:{}:{} {}", getCursorPosition().mColumn+1, getCursorPosition().mLineNumber+1,
          mFedDocument->getNumLines(), getLanguage().mName).c_str());
      //}}}
      //{{{  vertice debug
      ImGui::SameLine();
      ImGui::Text (fmt::format ("{}:{}",
                   ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());
      //}}}

      ImGui::SameLine();
      ImGui::TextUnformatted (mFedDocument->getFileName().c_str());
      //}}}

      // begin childWindow, new font, new colors
      if (isDrawMonoSpaced())
        ImGui::PushFont (app.getMonoFont());

      ImGui::PushStyleColor (ImGuiCol_ScrollbarBg, mDrawContext.getColor (eScrollBackground));
      ImGui::PushStyleColor (ImGuiCol_ScrollbarGrab, mDrawContext.getColor (eScrollGrab));
      ImGui::PushStyleColor (ImGuiCol_ScrollbarGrabHovered, mDrawContext.getColor (eScrollHover));
      ImGui::PushStyleColor (ImGuiCol_ScrollbarGrabActive, mDrawContext.getColor (eScrollActive));
      ImGui::PushStyleColor (ImGuiCol_Text, mDrawContext.getColor (eText));
      ImGui::PushStyleColor (ImGuiCol_ChildBg, mDrawContext.getColor (eBackground));
      ImGui::PushStyleVar (ImGuiStyleVar_ScrollbarSize, mDrawContext.getGlyphWidth() * 2.f);
      ImGui::PushStyleVar (ImGuiStyleVar_ScrollbarRounding, 2.f);
      ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, {0.f,0.f});
      ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, {0.f,0.f});
      ImGui::BeginChild ("##s", {0.f,0.f}, false,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);
      mDrawContext.update (static_cast<float>(mOptions.mFontSize), isDrawMonoSpaced() && !mFedDocument->hasUtf8());

      keyboard();

      if (isFolded())
        mFoldLines.resize (drawFolded());
      else
        drawLines();

      if (mEdit.mScrollVisible)
        scrollCursorVisible();

      // end childWindow
      ImGui::EndChild();
      ImGui::PopStyleVar (4);
      ImGui::PopStyleColor (6);
      if (isDrawMonoSpaced())
        ImGui::PopFont();
      }
    //}}}

  private:
    //{{{
    struct sCursor {
      sPosition mPosition;

      eSelect mSelect;
      sPosition mSelectBeginPosition;
      sPosition mSelectEndPosition;
      };
    //}}}
    //{{{
    class cUndo {
    public:
      //{{{
      void undo (cFedEdit* textEdit) {

        if (!mAddText.empty())
          textEdit->getFedDocument()->deletePositionRange (mAddBeginPosition, mAddEndPosition);
        if (!mDeleteText.empty())
          textEdit->insertText (mDeleteText, mDeleteBeginPosition);

        textEdit->mEdit.mCursor = mBeforeCursor;
        }
      //}}}
      //{{{
      void redo (cFedEdit* textEdit) {

        if (!mDeleteText.empty())
          textEdit->getFedDocument()->deletePositionRange (mDeleteBeginPosition, mDeleteEndPosition);
        if (!mAddText.empty())
          textEdit->insertText (mAddText, mAddBeginPosition);

        textEdit->mEdit.mCursor = mAfterCursor;
        }
      //}}}

      // vars
      sCursor mBeforeCursor;
      sCursor mAfterCursor;

      string mAddText;
      sPosition mAddBeginPosition;
      sPosition mAddEndPosition;

      string mDeleteText;
      sPosition mDeleteBeginPosition;
      sPosition mDeleteEndPosition;
      };
    //}}}
    //{{{
    class cEdit {
    public:
      // undo
      bool hasUndo() const { return mUndoIndex > 0; }
      bool hasRedo() const { return mUndoIndex < mUndoVector.size(); }
      //{{{
      void clearUndo() {
        mUndoVector.clear();
        mUndoIndex = 0;
        }
      //}}}
      //{{{
      void undo (cFedEdit* textEdit, uint32_t steps) {

        while (hasUndo() && (steps > 0)) {
          mUndoVector [--mUndoIndex].undo (textEdit);
          steps--;
          }
        }
      //}}}
      //{{{
      void redo (cFedEdit* textEdit, uint32_t steps) {

        while (hasRedo() && steps > 0) {
          mUndoVector [mUndoIndex++].redo (textEdit);
          steps--;
          }
        }
      //}}}
      //{{{
      void addUndo (cUndo& undo) {

        // trim undo list to mUndoIndex, add undo
        mUndoVector.resize (mUndoIndex+1);
        mUndoVector.back() = undo;
        mUndoIndex++;
        }
      //}}}

      // paste
      bool hasPaste() const { return !mPasteVector.empty(); }
      //{{{
      string popPasteText () {

        if (mPasteVector.empty())
          return "";
        else {
          string text = mPasteVector.back();
          mPasteVector.pop_back();
          return text;
          }
        }
      //}}}
      //{{{
      void pushPasteText (string text) {
        ImGui::SetClipboardText (text.c_str());
        mPasteVector.push_back (text);
        }
      //}}}

      // vars
      sCursor mCursor;

      // drag state
      sPosition mDragFirstPosition;

      // foldOnly state
      bool mFoldOnly = false;
      uint32_t mFoldOnlyBeginLineNumber = 0;

      // delayed action flags
      bool mParseDocument = true;
      bool mScrollVisible = false;

    private:
      // undo
      size_t mUndoIndex = 0;
      vector <cUndo> mUndoVector;

      // paste
      vector <string> mPasteVector;
      };
    //}}}
    //{{{
    class cOptions {
    public:
      int mFontSize = 16;
      int mMinFontSize = 4;
      int mMaxFontSize = 24;

      // modes
      bool mOverWrite = false;
      bool mReadOnly = false;

      // shows
      bool mShowFolded = true;
      bool mShowLineNumber = true;
      bool mShowLineDebug = false;
      bool mShowWhiteSpace = false;
      bool mShowMonoSpaced = true;
      };
    //}}}
    //{{{
    class cFedDrawContext : public cDrawContext {
    // drawContext with our palette and a couple of layout vars
    public:
      cFedDrawContext() : cDrawContext (kPalette) {}
      //{{{
      void update (float fontSize, bool monoSpaced) {

        cDrawContext::update (fontSize, monoSpaced);

        mLeftPad = getGlyphWidth() / 2.f;
        mLineNumberWidth = 0.f;
        }
      //}}}

      float mLeftPad = 0.f;
      float mLineNumberWidth = 0.f;

    private:
      // color to ImU32 lookup
      inline static const vector <ImU32> kPalette = {
        0xff808080, // eText
        0xffefefef, // eBackground
        0xff202020, // eIdentifier
        0xff606000, // eNumber
        0xff404040, // ePunctuation
        0xff2020a0, // eString
        0xff304070, // eLiteral
        0xff008080, // ePreProc
        0xff008000, // eComment
        0xff1010c0, // eKeyWord
        0xff800080, // eKnownWord

        0xff000000, // eCursorPos
        0x10000000, // eCursorLineFill
        0x40000000, // eCursorLineEdge
        0x800000ff, // eCursorReadOnly
        0x80600000, // eSelectCursor
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
      };
    //}}}

    //{{{  get
    bool isFolded() const { return mOptions.mShowFolded; }
    bool isDrawLineNumber() const { return mOptions.mShowLineNumber; }
    bool isDrawWhiteSpace() const { return mOptions.mShowWhiteSpace; }
    bool isDrawMonoSpaced() const { return mOptions.mShowMonoSpaced; }

    //{{{
    bool canEditAtCursor() {
    // cannot edit readOnly, foldBegin token, foldEnd token

      sPosition position = getCursorPosition();

      // line for cursor
      const cLine& line = mFedDocument->getLine (position.mLineNumber);

      return !isReadOnly() &&
             !(line.mFoldBegin && (position.mColumn < getGlyphsLine (position.mLineNumber).mFirstGlyph)) &&
             !line.mFoldEnd;
      }
    //}}}

    // text
    string getSelectText() { return mFedDocument->getText (mEdit.mCursor.mSelectBeginPosition, mEdit.mCursor.mSelectEndPosition); }

    // text widths
    //{{{
    float getWidth (sPosition position) {
    // get width in pixels of drawn glyphs up to and including position

      const cLine& line = mFedDocument->getLine (position.mLineNumber);

      float width = 0.f;
      uint32_t toGlyphIndex = mFedDocument->getGlyphIndex (position);
      for (uint32_t glyphIndex = line.mFirstGlyph;
           (glyphIndex < line.getNumGlyphs()) && (glyphIndex < toGlyphIndex); glyphIndex++) {
        if (line.getChar (glyphIndex) == '\t')
          // set width to end of tab
          width = getTabEndPosX (width);
        else
          // add glyphWidth
          width += getGlyphWidth (line, glyphIndex);
        }

      return width;
      }
    //}}}
    //{{{
    float getGlyphWidth (const cLine& line, uint32_t glyphIndex) {

      uint32_t numCharBytes = line.getNumCharBytes (glyphIndex);
      if (numCharBytes == 1) // simple common case
        return mDrawContext.measureChar (line.getChar (glyphIndex));

      array <char,6> str;
      uint32_t strIndex = 0;
      for (uint32_t i = 0; i < numCharBytes; i++)
        str[strIndex++] = line.getChar (glyphIndex, i);

      return mDrawContext.measureText (str.data(), str.data() + numCharBytes);
      }
    //}}}

    // lines
    uint32_t getNumFoldLines() const { return static_cast<uint32_t>(mFoldLines.size()); }
    uint32_t getMaxFoldLineIndex() const { return getNumFoldLines() - 1; }
    //{{{
    uint32_t getNumPageLines() const {
      return static_cast<uint32_t>(floor ((ImGui::GetWindowHeight() - 20.f) / mDrawContext.getLineHeight()));
      }
    //}}}

    // line
    //{{{
    uint32_t getLineNumberFromIndex (uint32_t lineIndex) const {

      if (!isFolded()) // lineNumber is lineIndex
        return lineIndex;

      if (lineIndex < getNumFoldLines())
        return mFoldLines[lineIndex];

      cLog::log (LOGERROR, fmt::format ("getLineNumberFromIndex {} no line for that index", lineIndex));
      return 0;
      }
    //}}}
    //{{{
    uint32_t getLineIndexFromNumber (uint32_t lineNumber) const {

      if (!isFolded()) // simple case, lineIndex is lineNumber
        return lineNumber;

      if (mFoldLines.empty()) {
        // no lineIndex vector
        cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber {} lineIndex empty", lineNumber));
        return 0;
        }

      // find lineNumber in lineIndex vector
      auto it = find (mFoldLines.begin(), mFoldLines.end(), lineNumber);
      if (it == mFoldLines.end()) {
        // lineNumber notFound, error
        cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber lineNumber:{} not found", lineNumber));
        return 0xFFFFFFFF;
        }

      // lineNUmber found, return lineIndex
      return uint32_t(it - mFoldLines.begin());
      }
    //}}}

    //{{{
    uint32_t getGlyphsLineNumber (uint32_t lineNumber) const {
    // return glyphs lineNumber for lineNumber - folded foldBegin closedFold seeThru into next line

      const cLine& line = mFedDocument->getLine (lineNumber);
      if (isFolded() && line.mFoldBegin && !line.mFoldOpen && (line.mFirstGlyph == line.getNumGlyphs()))
        return lineNumber + 1;
      else
        return lineNumber;
      }
    //}}}
    cLine& getGlyphsLine (uint32_t lineNumber) { return mFedDocument->getLine (getGlyphsLineNumber (lineNumber)); }

    //{{{
    sPosition getNextLinePosition (const sPosition& position) {

      uint32_t nextLineIndex = getLineIndexFromNumber (position.mLineNumber) + 1;
      uint32_t nextLineNumber = getLineNumberFromIndex (nextLineIndex);
      return {nextLineNumber, getGlyphsLine (nextLineNumber).mFirstGlyph};
      }
    //}}}

    // column pos
    //{{{
    float getTabEndPosX (float xPos) {
    // return tabEndPosx of tab containing xPos

      float tabWidthPixels = mFedDocument->getTabSize() * mDrawContext.getGlyphWidth();
      return (1.f + floor ((1.f + xPos) / tabWidthPixels)) * tabWidthPixels;
      }
    //}}}
    //{{{
    uint32_t getColumnFromPosX (const cLine& line, float posX) {
    // return column for posX, using glyphWidths and tabs

      uint32_t column = 0;

      float columnPosX = 0.f;
      for (uint32_t glyphIndex = line.mFirstGlyph; glyphIndex < line.getNumGlyphs(); glyphIndex++)
        if (line.getChar (glyphIndex) == '\t') {
          // tab
          float lastColumnPosX = columnPosX;
          float tabEndPosX = getTabEndPosX (columnPosX);
          float width = tabEndPosX - lastColumnPosX;
          if (columnPosX + (width/2.f) > posX)
            break;

          columnPosX = tabEndPosX;
          column = mFedDocument->getTabColumn (column);
          }

        else {
          float width = getGlyphWidth (line, glyphIndex);
          if (columnPosX + (width/2.f) > posX)
            break;

          columnPosX += width;
          column++;
          }

      //cLog::log (LOGINFO, fmt::format ("getColumn posX:{} firstGlyph:{} column:{} -> result:{}",
      //                                 posX, line.mFirstGlyph, column, line.mFirstGlyph + column));
      return line.mFirstGlyph + column;
      }
    //}}}
    //}}}
    //{{{  set
    //{{{
    void setCursorPosition (sPosition position) {
    // set cursorPosition, if changed check scrollVisible

      if (position != mEdit.mCursor.mPosition) {
        mEdit.mCursor.mPosition = position;
        mEdit.mScrollVisible = true;
        }
      }
    //}}}

    //{{{
    void deselect() {

      mEdit.mCursor.mSelect = eSelect::eNormal;
      mEdit.mCursor.mSelectBeginPosition = mEdit.mCursor.mPosition;
      mEdit.mCursor.mSelectEndPosition = mEdit.mCursor.mPosition;
      }
    //}}}
    //{{{
    void setSelect (eSelect select, sPosition beginPosition, sPosition endPosition) {

      mEdit.mCursor.mSelectBeginPosition = sanitizePosition (beginPosition);
      mEdit.mCursor.mSelectEndPosition = sanitizePosition (endPosition);
      if (mEdit.mCursor.mSelectBeginPosition > mEdit.mCursor.mSelectEndPosition)
        swap (mEdit.mCursor.mSelectBeginPosition, mEdit.mCursor.mSelectEndPosition);

      mEdit.mCursor.mSelect = select;
      switch (select) {
        case eSelect::eNormal:
          //cLog::log (LOGINFO, fmt::format ("setSelect eNormal {}:{} to {}:{}",
          //                                 mEdit.mCursor.mSelectBeginPosition.mLineNumber,
          //                                 mEdit.mCursor.mSelectBeginPosition.mColumn,
          //                                 mEdit.mCursor.mSelectEndPosition.mLineNumber,
          //                                 mEdit.mCursor.mSelectEndPosition.mColumn));
          break;

        case eSelect::eWord:
          mEdit.mCursor.mSelectBeginPosition = findWordBeginPosition (mEdit.mCursor.mSelectBeginPosition);
          mEdit.mCursor.mSelectEndPosition = findWordEndPosition (mEdit.mCursor.mSelectEndPosition);
          //cLog::log (LOGINFO, fmt::format ("setSelect eWord line:{} col:{} to:{}",
          //                                 mEdit.mCursor.mSelectBeginPosition.mLineNumber,
          //                                 mEdit.mCursor.mSelectBeginPosition.mColumn,
          //                                 mEdit.mCursor.mSelectEndPosition.mColumn));
          break;

        case eSelect::eLine: {
          const cLine& line = mFedDocument->getLine (mEdit.mCursor.mSelectBeginPosition.mLineNumber);
          if (!isFolded() || !line.mFoldBegin)
            // select line
            mEdit.mCursor.mSelectEndPosition = {mEdit.mCursor.mSelectEndPosition.mLineNumber + 1, 0};
          else if (!line.mFoldOpen)
            // select fold
            mEdit.mCursor.mSelectEndPosition = {skipFold (mEdit.mCursor.mSelectEndPosition.mLineNumber + 1), 0};

          cLog::log (LOGINFO, fmt::format ("setSelect eLine {}:{} to {}:{}",
                                           mEdit.mCursor.mSelectBeginPosition.mLineNumber,
                                           mEdit.mCursor.mSelectBeginPosition.mColumn,
                                           mEdit.mCursor.mSelectEndPosition.mLineNumber,
                                           mEdit.mCursor.mSelectEndPosition.mColumn));
          break;
          }
        }
      }
    //}}}
    //}}}
    //{{{  move
    //{{{
    void moveUp (uint32_t amount) {

      deselect();
      sPosition position = mEdit.mCursor.mPosition;

      if (!isFolded()) {
        //{{{  unfolded simple moveUp
        position.mLineNumber = (amount < position.mLineNumber) ? position.mLineNumber - amount : 0;
        setCursorPosition (position);
        return;
        }
        //}}}

      // folded moveUp
      //cLine& line = getLine (lineNumber);
      uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);

      uint32_t moveLineIndex = (amount < lineIndex) ? lineIndex - amount : 0;
      uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
      //cLine& moveLine = getLine (moveLineNumber);
      uint32_t moveColumn = position.mColumn;

      setCursorPosition ({moveLineNumber, moveColumn});
      }
    //}}}
    //{{{
    void moveDown (uint32_t amount) {

      deselect();
      sPosition position = mEdit.mCursor.mPosition;

      if (!isFolded()) {
        //{{{  unfolded simple moveDown
        position.mLineNumber = min (position.mLineNumber + amount, mFedDocument->getMaxLineNumber());
        setCursorPosition (position);
        return;
        }
        //}}}

      // folded moveDown
      uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);

      uint32_t moveLineIndex = min (lineIndex + amount, getMaxFoldLineIndex());
      uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
      uint32_t moveColumn = position.mColumn;

      setCursorPosition ({moveLineNumber, moveColumn});
      }
    //}}}

    //{{{
    void cursorFlashOn() {
    // set cursor flash cycle to on, helps after any move

      mCursorFlashTimePoint = chrono::system_clock::now();
      }
    //}}}
    //{{{
    void scrollCursorVisible() {

      ImGui::SetWindowFocus();
      cursorFlashOn();

      sPosition position = getCursorPosition();

      // up,down scroll
      uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
      uint32_t maxLineIndex = isFolded() ? getMaxFoldLineIndex() : mFedDocument->getMaxLineNumber();

      uint32_t minIndex = min (maxLineIndex,
         static_cast<uint32_t>(floor ((ImGui::GetScrollY() + ImGui::GetWindowHeight()) / mDrawContext.getLineHeight())));
      if (lineIndex >= minIndex - 3)
        ImGui::SetScrollY (max (0.f, (lineIndex + 3) * mDrawContext.getLineHeight()) - ImGui::GetWindowHeight());
      else {
        uint32_t maxIndex = static_cast<uint32_t>(floor (ImGui::GetScrollY() / mDrawContext.getLineHeight()));
        if (lineIndex < maxIndex + 2)
          ImGui::SetScrollY (max (0.f, (lineIndex - 2) * mDrawContext.getLineHeight()));
        }

      // left,right scroll
      float textWidth = mDrawContext.mLineNumberWidth + getWidth (position);
      uint32_t minWidth = static_cast<uint32_t>(floor (ImGui::GetScrollX()));
      if (textWidth - (3 * mDrawContext.getGlyphWidth()) < minWidth)
        ImGui::SetScrollX (max (0.f, textWidth - (3 * mDrawContext.getGlyphWidth())));
      else {
        uint32_t maxWidth = static_cast<uint32_t>(ceil (ImGui::GetScrollX() + ImGui::GetWindowWidth()));
        if (textWidth + (3 * mDrawContext.getGlyphWidth()) > maxWidth)
          ImGui::SetScrollX (max (0.f, textWidth + (3 * mDrawContext.getGlyphWidth()) - ImGui::GetWindowWidth()));
        }

      // done
      mEdit.mScrollVisible = false;
      }
    //}}}

    //{{{
    sPosition advancePosition (sPosition position) {

      if (position.mLineNumber >= mFedDocument->getNumLines())
        return position;

      uint32_t glyphIndex = mFedDocument->getGlyphIndex (position);
      const cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
      if (glyphIndex + 1 < glyphsLine.getNumGlyphs())
        position.mColumn = mFedDocument->getColumn (glyphsLine, glyphIndex + 1);
      else {
        position.mLineNumber++;
        position.mColumn = 0;
        }

      return position;
      }
    //}}}
    //{{{
    sPosition sanitizePosition (sPosition position) {
    // return position to be within glyphs

      if (position.mLineNumber >= mFedDocument->getNumLines()) {
        // past end, position at end of last line
        //cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{} to max:{}",
        //                                 position.mLineNumber, getMaxLineNumber()));
        return sPosition (mFedDocument->getMaxLineNumber(), mFedDocument->getNumColumns (mFedDocument->getLine (mFedDocument->getMaxLineNumber())));
        }

      // ensure column is in range of position.mLineNumber glyphsLine
      cLine& glyphsLine = getGlyphsLine (position.mLineNumber);

      if (position.mColumn < glyphsLine.mFirstGlyph) {
        //cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{} column:{} to min:{}",
        //                                 position.mLineNumber, position.mColumn, glyphsLine.mFirstGlyph));
        position.mColumn = glyphsLine.mFirstGlyph;
        }
      else if (position.mColumn > glyphsLine.getNumGlyphs()) {
        //cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{} column:{} to max:{}",
        //                                 position.mLineNumber, position.mColumn, glyphsLine.getNumGlyphs()));
        position.mColumn = glyphsLine.getNumGlyphs();
        }

      return position;
      }
    //}}}

    //{{{
    sPosition findWordBeginPosition (sPosition position) {

      const cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
      uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
      if (glyphIndex >= glyphsLine.getNumGlyphs()) // already at lineEnd
        return position;

      // searck back for nonSpace
      while (glyphIndex && isspace (glyphsLine.getChar (glyphIndex)))
        glyphIndex--;

      // search back for nonSpace and of same parse color
      uint8_t color = glyphsLine.getColor (glyphIndex);
      while (glyphIndex > 0) {
        uint8_t ch = glyphsLine.getChar (glyphIndex);
        if (isspace (ch)) {
          glyphIndex++;
          break;
          }
        if (color != glyphsLine.getColor (glyphIndex - 1))
          break;
        glyphIndex--;
        }

      position.mColumn = mFedDocument->getColumn (glyphsLine, glyphIndex);
      return position;
      }
    //}}}
    //{{{
    sPosition findWordEndPosition (sPosition position) {

      const cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
      uint32_t glyphIndex = mFedDocument->getGlyphIndex (glyphsLine, position.mColumn);
      if (glyphIndex >= glyphsLine.getNumGlyphs()) // already at lineEnd
        return position;

      // serach forward for space, or not same parse color
      uint8_t prevColor = glyphsLine.getColor (glyphIndex);
      bool prevSpace = isspace (glyphsLine.getChar (glyphIndex));
      while (glyphIndex < glyphsLine.getNumGlyphs()) {
        uint8_t ch = glyphsLine.getChar (glyphIndex);
        if (glyphsLine.getColor (glyphIndex) != prevColor)
          break;
        if (static_cast<bool>(isspace (ch)) != prevSpace)
          break;
        glyphIndex++;
        }

      position.mColumn = mFedDocument->getColumn (glyphsLine, glyphIndex);
      return position;
      }
    //}}}
    //}}}

    //{{{
    sPosition insertText (const string& text, sPosition position) {
    // insert helper - !!! needs more utf8 handling !!!!

      if (text.empty())
        return position;

      bool onFoldBegin = mFedDocument->getLine (position.mLineNumber).mFoldBegin;

      uint32_t glyphIndex = mFedDocument->getGlyphIndex (getGlyphsLine (position.mLineNumber), position.mColumn);
      for (auto it = text.begin(); it < text.end(); ++it) {
        char ch = *it;
        if (ch == '\r') // skip
          break;

        cLine& glyphsLine = getGlyphsLine (position.mLineNumber);
        if (ch == '\n') {
          // insert new line
          if (onFoldBegin) // no lineFeed in allowed in foldBegin, return
            return position;

          // break glyphsLine at glyphIndex, no autoIndent ????
          mFedDocument->breakLine (glyphsLine, glyphIndex, position.mLineNumber+1, 0);

          // point to beginning of new line
          glyphIndex = 0;
          position.mColumn = 0;
          position.mLineNumber++;
          }

        else {
          // insert char
          mFedDocument->insertChar (glyphsLine, glyphIndex, ch);

          // point to next char
          glyphIndex++;
          position.mColumn++; // !!! should convert glyphIndex back to column !!!
          }
        }

      return position;
      }
    //}}}
    void insertText (const string& text) { setCursorPosition (insertText (text, getCursorPosition())); }
    //{{{
    void deleteSelect() {

      mFedDocument->deletePositionRange (mEdit.mCursor.mSelectBeginPosition, mEdit.mCursor.mSelectEndPosition);
      setCursorPosition (mEdit.mCursor.mSelectBeginPosition);
      deselect();
      }
    //}}}

    //  fold
    //{{{
    void openFold (uint32_t lineNumber) {

      if (isFolded()) {
        cLine& line = mFedDocument->getLine (lineNumber);
        if (line.mFoldBegin)
          line.mFoldOpen = true;

        // position cursor to lineNumber, first glyph column
        setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
        }
      }
    //}}}
    //{{{
    void openFoldOnly (uint32_t lineNumber) {

      if (isFolded()) {
        cLine& line = mFedDocument->getLine (lineNumber);
        if (line.mFoldBegin) {
          line.mFoldOpen = true;
          mEdit.mFoldOnly = true;
          mEdit.mFoldOnlyBeginLineNumber = lineNumber;
          }

        // position cursor to lineNumber, first glyph column
        setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
        }
      }
    //}}}
    //{{{
    void closeFold (uint32_t lineNumber) {

      if (isFolded()) {
        // close fold containing lineNumber
        cLog::log (LOGINFO, fmt::format ("closeFold line:{}", lineNumber));

        cLine& line = mFedDocument->getLine (lineNumber);
        if (line.mFoldBegin) {
          // position cursor to lineNumber, first glyph column
          line.mFoldOpen = false;
          setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
          }

        else {
          // search back for this fold's foldBegin and close it
          // - skip foldEnd foldBegin pairs
          uint32_t skipFoldPairs = 0;
          while (lineNumber > 0) {
            line = mFedDocument->getLine (--lineNumber);
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
              line.mFoldOpen = false;
              setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
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
    uint32_t skipFold (uint32_t lineNumber) {
    // recursively skip fold lines until matching foldEnd

      while (lineNumber < mFedDocument->getNumLines())
        if (mFedDocument->getLine (lineNumber).mFoldBegin)
          lineNumber = skipFold (lineNumber+1);
        else if (mFedDocument->getLine (lineNumber).mFoldEnd)
          return lineNumber+1;
        else
          lineNumber++;

      // error if you run off end. begin/end mismatch
      cLog::log (LOGERROR, "skipToFoldEnd - unexpected end reached with mathching fold");
      return lineNumber;
      }
    //}}}
    //{{{
    uint32_t drawFolded() {

      uint32_t lineIndex = 0;

      uint32_t lineNumber = mEdit.mFoldOnly ? mEdit.mFoldOnlyBeginLineNumber : 0;

      //cLog::log (LOGINFO, fmt::format ("drawFolded {}", lineNumber));

      while (lineNumber < mFedDocument->getNumLines()) {
        cLine& line = mFedDocument->getLine (lineNumber);

        if (line.mFoldBegin) {
          // foldBegin
          line.mFirstGlyph = static_cast<uint8_t>(line.mIndent + getLanguage().mFoldBeginToken.size());
          if (line.mFoldOpen)
            // draw foldBegin open fold line
            drawLine (lineNumber++, lineIndex++);
          else {
            // draw foldBegin closed fold line, skip rest
            uint32_t glyphsLineNumber = getGlyphsLineNumber (lineNumber);
            if (glyphsLineNumber != lineNumber)
                mFedDocument->getLine (glyphsLineNumber).mFirstGlyph = static_cast<uint8_t>(line.mIndent);
            drawLine (lineNumber++, lineIndex++);
            lineNumber = skipFold (lineNumber);
            }
          }

        else {
          // draw foldMiddle, foldEnd
          line.mFirstGlyph = 0;
          drawLine (lineNumber++, lineIndex++);
          if (line.mFoldEnd && mEdit.mFoldOnly)
            return lineIndex;
          }
        }

      return lineIndex;
      }
    //}}}

    // keyboard
    //{{{
    void keyboard() {
      //{{{
      struct sActionKey {
        bool mAlt;
        bool mCtrl;
        bool mShift;
        ImGuiKey mGuiKey;
        bool mWritable;
        function <void()> mActionFunc;
        };
      //}}}
      const vector<sActionKey> kActionKeys = {
      //  alt    ctrl   shift  guiKey           writable   function
         {false, false, false, ImGuiKey_Delete,     true,  [this]{deleteIt();}},
         {false, false, false, ImGuiKey_Backspace,  true,  [this]{backspace();}},
         {false, false, false, ImGuiKey_Enter,      true,  [this]{enterKey();}},
         {false, false, false, ImGuiKey_Tab,        true,  [this]{tabKey();}},
         {false, true,  false, ImGuiKey_X,          true,  [this]{cut();}},
         {false, true,  false, ImGuiKey_V,          true,  [this]{paste();}},
         {false, true,  false, ImGuiKey_Z,          true,  [this]{undo();}},
         {false, true,  false, ImGuiKey_Y,          true,  [this]{redo();}},
         // edit without change
         {false, true,  false, ImGuiKey_C,          false, [this]{copy();}},
         {false, true,  false, ImGuiKey_A,          false, [this]{selectAll();}},
         // move
         {false, false, false, ImGuiKey_LeftArrow,  false, [this]{moveLeft();}},
         {false, false, false, ImGuiKey_RightArrow, false, [this]{moveRight();}},
         {false, true,  false, ImGuiKey_RightArrow, false, [this]{moveRightWord();}},
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
         {false, false, false, ImGuiKey_Keypad1,    false, [this]{openFold();}},
         {false, false, false, ImGuiKey_Keypad3,    false, [this]{closeFold();}},
         {false, false, false, ImGuiKey_Keypad7,    false, [this]{openFoldOnly();}},
         {false, false, false, ImGuiKey_Keypad9,    false, [this]{closeFold();}},
         {false, false, false, ImGuiKey_Keypad0,    true,  [this]{createFold();}},
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

      ImGui::GetIO().WantTextInput = true;
      ImGui::GetIO().WantCaptureKeyboard = false;

      bool altKeyPressed = ImGui::GetIO().KeyAlt;
      bool ctrlKeyPressed = ImGui::GetIO().KeyCtrl;
      bool shiftKeyPressed = ImGui::GetIO().KeyShift;
      for (const auto& actionKey : kActionKeys)
        //{{{  dispatch actionKey
        if (ImGui::IsKeyPressed (actionKey.mGuiKey) &&
            (actionKey.mAlt == altKeyPressed) &&
            (actionKey.mCtrl == ctrlKeyPressed) &&
            (actionKey.mShift == shiftKeyPressed) &&
            (!actionKey.mWritable || (actionKey.mWritable && !(isReadOnly() && canEditAtCursor())))) {

          actionKey.mActionFunc();
          break;
          }
        //}}}

      if (!isReadOnly()) {
        for (int i = 0; i < ImGui::GetIO().InputQueueCharacters.Size; i++) {
          ImWchar ch = ImGui::GetIO().InputQueueCharacters[i];
          if ((ch != 0) && ((ch == '\n') || (ch >= 32)))
            enterCharacter (ch);
          }
        ImGui::GetIO().InputQueueCharacters.resize (0);
        }
      }
    //}}}

    // mouse
    //{{{
    void mouseSelectLine (uint32_t lineNumber) {

      cLog::log (LOGINFO, fmt::format ("mouseSelectLine line:{}", lineNumber));

      cursorFlashOn();
      mEdit.mCursor.mPosition = {lineNumber, 0};
      mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
      setSelect (eSelect::eLine, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
      }
    //}}}
    //{{{
    void mouseDragSelectLine (uint32_t lineNumber, float posY) {
    // if folded this will use previous displays mFoldLine vector
    // - we haven't drawn subsequent lines yet
    //   - works because dragging does not change vector
    // - otherwise we would have to delay it to after the whole draw

      if (!canEditAtCursor())
        return;

      int numDragLines = static_cast<int>((posY - (mDrawContext.getLineHeight()/2.f)) / mDrawContext.getLineHeight());

      cLog::log (LOGINFO, fmt::format ("mouseDragSelectLine line:{} dragLines:{}", lineNumber, numDragLines));

      if (isFolded()) {
        uint32_t lineIndex = max (0u, min (getMaxFoldLineIndex(), getLineIndexFromNumber (lineNumber) + numDragLines));
        lineNumber = mFoldLines[lineIndex];
        }
      else // simple add to lineNumber
        lineNumber = max (0u, min (mFedDocument->getMaxLineNumber(), lineNumber + numDragLines));

      setCursorPosition ({lineNumber, 0});

      setSelect (eSelect::eLine, mEdit.mDragFirstPosition, getCursorPosition());
      }
    //}}}
    //{{{
    void mouseSelectText (bool selectWord, uint32_t lineNumber, ImVec2 pos) {

      //cLog::log (LOGINFO, fmt::format ("mouseSelectText line:{}", lineNumber));
      cursorFlashOn();
      mEdit.mCursor.mPosition = {lineNumber, getColumnFromPosX (getGlyphsLine (lineNumber), pos.x)};
      mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
      setSelect (selectWord ? eSelect::eWord : eSelect::eNormal, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
      }
    //}}}
    //{{{
    void mouseDragSelectText (uint32_t lineNumber, ImVec2 pos) {

      if (!canEditAtCursor())
        return;

      int numDragLines = static_cast<int>((pos.y - (mDrawContext.getLineHeight()/2.f)) / mDrawContext.getLineHeight());
      uint32_t dragLineNumber = max (0u, min (mFedDocument->getMaxLineNumber(), lineNumber + numDragLines));

      //cLog::log (LOGINFO, fmt::format ("mouseDragSelectText {} {}", lineNumber, numDragLines));

      if (isFolded()) {
        // cannot cross fold
        if (lineNumber < dragLineNumber) {
          while (++lineNumber <= dragLineNumber)
            if (mFedDocument->getLine (lineNumber).mFoldBegin || mFedDocument->getLine (lineNumber).mFoldEnd) {
              cLog::log (LOGINFO, fmt::format ("dragSelectText exit 1"));
              return;
              }
          }
        else if (lineNumber > dragLineNumber) {
          while (--lineNumber >= dragLineNumber)
            if (mFedDocument->getLine (lineNumber).mFoldBegin || mFedDocument->getLine (lineNumber).mFoldEnd) {
              cLog::log (LOGINFO, fmt::format ("dragSelectText exit 2"));
              return;
              }
          }
        }

      // select drag range
      uint32_t dragColumn = getColumnFromPosX (getGlyphsLine (dragLineNumber), pos.x);
      setCursorPosition ({dragLineNumber, dragColumn});
      setSelect (eSelect::eNormal, mEdit.mDragFirstPosition, getCursorPosition());
      }
    //}}}

    // draw
    //{{{
    float drawGlyphs (ImVec2 pos, const cLine& line, uint8_t forceColor) {

      if (line.empty())
        return 0.f;

      // initial pos to measure textWidth on return
      float firstPosX = pos.x;

      array <char,256> str;
      uint32_t strIndex = 0;
      uint8_t prevColor = (forceColor == eUndefined) ? line.getColor (line.mFirstGlyph) : forceColor;

      for (uint32_t glyphIndex = line.mFirstGlyph; glyphIndex < line.getNumGlyphs(); glyphIndex++) {
        uint8_t color = (forceColor == eUndefined) ? line.getColor (glyphIndex) : forceColor;
        if ((strIndex > 0) && (strIndex < str.max_size()) &&
            ((color != prevColor) || (line.getChar (glyphIndex) == '\t') || (line.getChar (glyphIndex) == ' '))) {
          // draw colored glyphs, seperated by colorChange,tab,space
          pos.x += mDrawContext.text (pos, prevColor, str.data(), str.data() + strIndex);
          strIndex = 0;
          }

        if (line.getChar (glyphIndex) == '\t') {
          //{{{  tab
          ImVec2 arrowLeftPos {pos.x + 1.f, pos.y + mDrawContext.getFontSize()/2.f};

          // apply tab
          pos.x = getTabEndPosX (pos.x);

          if (isDrawWhiteSpace()) {
            // draw tab arrow
            ImVec2 arrowRightPos {pos.x - 1.f, arrowLeftPos.y};
            mDrawContext.line (arrowLeftPos, arrowRightPos, eTab);

            ImVec2 arrowTopPos {arrowRightPos.x - (mDrawContext.getFontSize()/4.f) - 1.f,
                                arrowRightPos.y - (mDrawContext.getFontSize()/4.f)};
            mDrawContext.line (arrowRightPos, arrowTopPos, eTab);

            ImVec2 arrowBotPos {arrowRightPos.x - (mDrawContext.getFontSize()/4.f) - 1.f,
                                arrowRightPos.y + (mDrawContext.getFontSize()/4.f)};
            mDrawContext.line (arrowRightPos, arrowBotPos, eTab);
            }
          }
          //}}}
        else if (line.getChar (glyphIndex) == ' ') {
          //{{{  space
          if (isDrawWhiteSpace()) {
            // draw circle
            ImVec2 centre {pos.x  + mDrawContext.getGlyphWidth() /2.f, pos.y + mDrawContext.getFontSize()/2.f};
            mDrawContext.circle (centre, 2.f, eWhiteSpace);
            }

          pos.x += mDrawContext.getGlyphWidth();
          }
          //}}}
        else {
          // character
          for (uint32_t i = 0; i < line.getNumCharBytes (glyphIndex); i++)
            str[strIndex++] = line.getChar (glyphIndex, i);
          }

        prevColor = color;
        }

      if (strIndex > 0) // draw any remaining glyphs
        pos.x += mDrawContext.text (pos, prevColor, str.data(), str.data() + strIndex);

      return pos.x - firstPosX;
      }
    //}}}
    //{{{
    void drawSelect (ImVec2 pos, uint32_t lineNumber, uint32_t glyphsLineNumber) {

      // posBegin
      ImVec2 posBegin = pos;
      if (sPosition (lineNumber, 0) >= mEdit.mCursor.mSelectBeginPosition)
        // from left edge
        posBegin.x = 0.f;
      else
        // from selectBegin column
        posBegin.x += getWidth ({glyphsLineNumber, mEdit.mCursor.mSelectBeginPosition.mColumn});

      // posEnd
      ImVec2 posEnd = pos;
      if (lineNumber < mEdit.mCursor.mSelectEndPosition.mLineNumber)
        // to end of line
        posEnd.x += getWidth ({lineNumber, mFedDocument->getNumColumns (mFedDocument->getLine (lineNumber))});
      else if (mEdit.mCursor.mSelectEndPosition.mColumn)
        // to selectEnd column
        posEnd.x += getWidth ({glyphsLineNumber, mEdit.mCursor.mSelectEndPosition.mColumn});
      else
        // to lefPad + lineNumer
        posEnd.x = mDrawContext.mLeftPad + mDrawContext.mLineNumberWidth;
      posEnd.y += mDrawContext.getLineHeight();

      // draw select rect
      mDrawContext.rect (posBegin, posEnd, eSelectCursor);
      }
    //}}}
    //{{{
    void drawCursor (ImVec2 pos, uint32_t lineNumber) {

      // line highlight using pos
      if (!hasSelect()) {
        // draw cursor highlight on lineNumber, could be allText or wholeLine
        ImVec2 tlPos {0.f, pos.y};
        ImVec2 brPos {pos.x, pos.y + mDrawContext.getLineHeight()};
        mDrawContext.rect (tlPos, brPos, eCursorLineFill);
        mDrawContext.rectLine (tlPos, brPos, canEditAtCursor() ? eCursorLineEdge : eCursorReadOnly);
        }

      uint32_t column = mEdit.mCursor.mPosition.mColumn;
      sPosition position = {lineNumber, column};

      //  draw flashing cursor
      auto elapsed = chrono::system_clock::now() - mCursorFlashTimePoint;
      if (elapsed < 400ms) {
        // draw cursor
        float cursorPosX = getWidth (position);
        float cursorWidth = 2.f;

        cLine& line = mFedDocument->getLine (lineNumber);
        uint32_t glyphIndex = mFedDocument->getGlyphIndex (position);
        if (mOptions.mOverWrite && (glyphIndex < line.getNumGlyphs())) {
          // overwrite
          if (line.getChar (glyphIndex) == '\t')
            // widen tab cursor
            cursorWidth = getTabEndPosX (cursorPosX) - cursorPosX;
          else
            // widen character cursor
            cursorWidth = mDrawContext.measureChar (line.getChar (glyphIndex));
          }

        ImVec2 tlPos {pos.x + cursorPosX - 1.f, pos.y};
        ImVec2 brPos {tlPos.x + cursorWidth, pos.y + mDrawContext.getLineHeight()};
        mDrawContext.rect (tlPos, brPos, canEditAtCursor() ? eCursorPos : eCursorReadOnly);
        return;
        }

      if (elapsed > 800ms) // reset flashTimer
        cursorFlashOn();
      }
    //}}}
    //{{{
    void drawLine (uint32_t lineNumber, uint32_t lineIndex) {
    // draw Line and return incremented lineIndex

      //cLog::log (LOGINFO, fmt::format ("drawLine {}", lineNumber));

      if (isFolded()) {
        //{{{  update mFoldLines vector
        if (lineIndex >= getNumFoldLines())
          mFoldLines.push_back (lineNumber);
        else
          mFoldLines[lineIndex] = lineNumber;
        }
        //}}}

      ImVec2 curPos = ImGui::GetCursorScreenPos();

      // lineNumber
      float leftPadWidth = mDrawContext.mLeftPad;
      cLine& line = mFedDocument->getLine (lineNumber);
      if (isDrawLineNumber()) {
        //{{{  draw lineNumber
        curPos.x += leftPadWidth;

        if (mOptions.mShowLineDebug)
          // debug
          mDrawContext.mLineNumberWidth = mDrawContext.text (curPos, eLineNumber,
            fmt::format ("{:4d}{}{}{}{}{}{}{}{:2d}{:2d}{:3d} ",
              lineNumber,
              line.mCommentSingle ? '/' : ' ',
              line.mCommentBegin  ? '{' : ' ',
              line.mCommentEnd    ? '}' : ' ',
              line.mFoldBegin     ? 'b' : ' ',
              line.mFoldEnd       ? 'e' : ' ',
              line.mFoldOpen      ? 'o' : ' ',
              line.mFoldPressed   ? 'p' : ' ',
              line.mIndent,
              line.mFirstGlyph,
              line.getNumGlyphs()
              ));
        else
          // normal
          mDrawContext.mLineNumberWidth = mDrawContext.smallText (
            curPos, eLineNumber, fmt::format ("{:4d} ", lineNumber+1));

        // add invisibleButton, gobble up leftPad
        ImGui::InvisibleButton (fmt::format ("##l{}", lineNumber).c_str(),
                                {leftPadWidth + mDrawContext.mLineNumberWidth, mDrawContext.getLineHeight()});
        if (ImGui::IsItemActive()) {
          if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
            mouseDragSelectLine (lineNumber, ImGui::GetMousePos().y - curPos.y);
          else if (ImGui::IsMouseClicked (0))
            mouseSelectLine (lineNumber);
          }

        leftPadWidth = 0.f;
        curPos.x += mDrawContext.mLineNumberWidth;

        ImGui::SameLine();
        }
        //}}}
      else
        mDrawContext.mLineNumberWidth = 0.f;

      // text
      ImVec2 glyphsPos = curPos;
      if (isFolded() && line.mFoldBegin) {
        if (line.mFoldOpen) {
          //{{{  draw foldBegin open + glyphs
          // draw foldPrefix
          curPos.x += leftPadWidth;

          // add the indent
          float indentWidth = line.mIndent * mDrawContext.getGlyphWidth();
          curPos.x += indentWidth;

          // draw foldPrefix
          float prefixWidth = mDrawContext.text(curPos, eFoldOpen, getLanguage().mFoldBeginOpen);

          // add foldPrefix invisibleButton, action on press
          ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                                  {leftPadWidth + indentWidth + prefixWidth, mDrawContext.getLineHeight()});
          if (ImGui::IsItemActive() && !line.mFoldPressed) {
            // close fold
            line.mFoldPressed = true;
            closeFold (lineNumber);
            }
          if (ImGui::IsItemDeactivated())
            line.mFoldPressed = false;

          curPos.x += prefixWidth;
          glyphsPos.x = curPos.x;

          // add invisibleButton, fills rest of line for easy picking
          ImGui::SameLine();
          ImGui::InvisibleButton (fmt::format("##t{}", lineNumber).c_str(),
                                  {ImGui::GetWindowWidth(), mDrawContext.getLineHeight()});
          if (ImGui::IsItemActive()) {
            if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
              mouseDragSelectText (lineNumber,
                                   {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
            else if (ImGui::IsMouseClicked (0))
              mouseSelectText (ImGui::IsMouseDoubleClicked (0), lineNumber,
                               {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
            }
          // draw glyphs
          curPos.x += drawGlyphs (glyphsPos, line, eUndefined);
          }
          //}}}
        else {
          //{{{  draw foldBegin closed + glyphs
          // add indent
          curPos.x += leftPadWidth;

          // add the indent
          float indentWidth = line.mIndent * mDrawContext.getGlyphWidth();
          curPos.x += indentWidth;

          // draw foldPrefix
          float prefixWidth = mDrawContext.text (curPos, eFoldClosed, getLanguage().mFoldBeginClosed);

          // add foldPrefix invisibleButton, indent + prefix wide, action on press
          ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                                  {leftPadWidth + indentWidth + prefixWidth, mDrawContext.getLineHeight()});
          if (ImGui::IsItemActive() && !line.mFoldPressed) {
            // open fold only
            line.mFoldPressed = true;
            openFoldOnly (lineNumber);
            }
          if (ImGui::IsItemDeactivated())
            line.mFoldPressed = false;

          curPos.x += prefixWidth;
          glyphsPos.x = curPos.x;

          // add invisibleButton, fills rest of line for easy picking
          ImGui::SameLine();
          ImGui::InvisibleButton (fmt::format ("##t{}", lineNumber).c_str(),
                                  {ImGui::GetWindowWidth(), mDrawContext.getLineHeight()});
          if (ImGui::IsItemActive()) {
            if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
              mouseDragSelectText (lineNumber,
                                   {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
            else if (ImGui::IsMouseClicked (0))
              mouseSelectText (ImGui::IsMouseDoubleClicked (0), lineNumber,
                               {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
            }

          // draw glyphs, from foldComment or seeThru line
          curPos.x += drawGlyphs (glyphsPos, getGlyphsLine (lineNumber), eFoldClosed);
          }
          //}}}
        }
      else if (isFolded() && line.mFoldEnd) {
        //{{{  draw foldEnd
        curPos.x += leftPadWidth;

        // add the indent
        float indentWidth = line.mIndent * mDrawContext.getGlyphWidth();
        curPos.x += indentWidth;

        // draw foldPrefix
        float prefixWidth = mDrawContext.text (curPos, eFoldOpen, getLanguage().mFoldEnd);
        glyphsPos.x = curPos.x;

        // add foldPrefix invisibleButton, only prefix wide, do not want to pick foldEnd line
        ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                                {leftPadWidth + indentWidth + prefixWidth, mDrawContext.getLineHeight()});
        if (ImGui::IsItemActive()) // closeFold at foldEnd, action on press
          closeFold (lineNumber);
        }
        //}}}
      else {
        //{{{  draw glyphs
        curPos.x += leftPadWidth;
        glyphsPos.x = curPos.x;

        // add invisibleButton, fills rest of line for easy picking
        ImGui::InvisibleButton (fmt::format ("##t{}", lineNumber).c_str(),
                                {ImGui::GetWindowWidth(), mDrawContext.getLineHeight()});
        if (ImGui::IsItemActive()) {
          if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
            mouseDragSelectText (lineNumber,
                                 {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
          else if (ImGui::IsMouseClicked (0))
            mouseSelectText (ImGui::IsMouseDoubleClicked (0), lineNumber,
                             {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
          }

        // drawGlyphs
        curPos.x += drawGlyphs (glyphsPos, line, eUndefined);
        }
        //}}}

      uint32_t glyphsLineNumber = getGlyphsLineNumber (lineNumber);

      // select
      if ((lineNumber >= mEdit.mCursor.mSelectBeginPosition.mLineNumber) &&
          (lineNumber <= mEdit.mCursor.mSelectEndPosition.mLineNumber))
        drawSelect (glyphsPos, lineNumber, glyphsLineNumber);

      // cursor
      if (lineNumber == mEdit.mCursor.mPosition.mLineNumber)
        drawCursor (glyphsPos, glyphsLineNumber);
      }
    //}}}
    //{{{
    void drawLines() {
    //  draw unfolded with clipper

      // clipper begin
      ImGuiListClipper clipper;
      clipper.Begin (mFedDocument->getNumLines(), mDrawContext.getLineHeight());
      clipper.Step();

      // clipper iterate
      for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++) {
        mFedDocument->getLine (lineNumber).mFirstGlyph = 0;
        drawLine (lineNumber, 0);
        }

      // clipper end
      clipper.Step();
      clipper.End();
      }
    //}}}

    // vars
    cFedDocument* mFedDocument = nullptr;
    bool mInited = false;

    // edit state
    cEdit mEdit;
    cOptions mOptions;
    cFedDrawContext mDrawContext;

    vector <uint32_t> mFoldLines;

    chrono::system_clock::time_point mCursorFlashTimePoint;
    };
  //}}}
  //{{{  class cMemEdit
  //{{{  defines, push warnings
  #ifdef _MSC_VER
    #define _PRISizeT   "I"
    #define ImSnprintf  _snprintf
  #else
    #define _PRISizeT   "z"
    #define ImSnprintf  snprintf
  #endif

  #ifdef _MSC_VER
    #pragma warning (push)
    #pragma warning (disable: 4996) // warning C4996: 'sprintf': This function or variable may be unsafe.
  #endif
  //}}}
  constexpr bool kShowAscii = true;
  constexpr bool kGreyZeroes = true;
  constexpr bool kUpperCaseHex = false;
  constexpr int kPageMarginLines = 3;

  namespace {
    //{{{  const strings
    const array<string,3> kFormatDescs = { "Dec", "Bin", "Hex" };

    const array<string,10> kTypeDescs = { "Int8", "Uint8", "Int16", "Uint16",
                                          "Int32", "Uint32", "Int64", "Uint64",
                                          "Float", "Double",
                                          };

    const array<size_t,10> kTypeSizes = { sizeof(int8_t), sizeof(uint8_t), sizeof(int16_t), sizeof(uint16_t),
                                          sizeof(int32_t), sizeof (uint32_t), sizeof(int64_t), sizeof(uint64_t),
                                          sizeof(float), sizeof(double),
                                          };

    const char* kFormatByte = kUpperCaseHex ? "%02X" : "%02x";
    const char* kFormatByteSpace = kUpperCaseHex ? "%02X " : "%02x ";

    const char* kFormatData = kUpperCaseHex ? "%0*" _PRISizeT "X" : "%0*" _PRISizeT "x";
    const char* kFormatAddress = kUpperCaseHex ? "%0*" _PRISizeT "X: " : "%0*" _PRISizeT "x: ";

    const char* kChildScrollStr = "##scrolling";
    //}}}
    //{{{
    void* bigEndianFunc (void* dst, void* src, size_t s, bool isLittleEndian) {

       if (isLittleEndian) {
        uint8_t* dstPtr = (uint8_t*)dst;
        uint8_t* srcPtr = (uint8_t*)src + s - 1;
        for (int i = 0, n = (int)s; i < n; ++i)
          memcpy(dstPtr++, srcPtr--, 1);
        return dst;
        }

      else
        return memcpy (dst, src, s);
      }
    //}}}
    //{{{
    void* littleEndianFunc (void* dst, void* src, size_t s, bool isLittleEndian) {

      if (isLittleEndian)
        return memcpy (dst, src, s);

      else {
        uint8_t* dstPtr = (uint8_t*)dst;
        uint8_t* srcPtr = (uint8_t*)src + s - 1;
        for (int i = 0, n = (int)s; i < n; ++i)
          memcpy (dstPtr++, srcPtr--, 1);
        return dst;
        }
      }
    //}}}
    void* (*gEndianFunc)(void*, void*, size_t, bool) = nullptr;
    }

  class cMemEdit {
  public:
    //{{{
    cMemEdit (uint8_t* memData, size_t memSize) : mInfo(memData,memSize) {

      mStartTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
      mLastClick = 0.f;
      }
    //}}}
    ~cMemEdit() = default;

    //{{{  draws
    //{{{
    void drawWindow (const string& title, size_t baseAddress) {
    // standalone Memory Editor window

      mOpen = true;

      ImGui::Begin (title.c_str(), &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
      drawContents (baseAddress);
      ImGui::End();
      }
    //}}}
    //{{{
    void drawContents (size_t baseAddress) {

      mInfo.setBaseAddress (baseAddress);
      mContext.update (mOptions, mInfo); // context of sizes,colours
      mEdit.begin (mOptions, mInfo);

      //ImGui::PushAllowKeyboardFocus (true);
      //ImGui::PopAllowKeyboardFocus();
      keyboard();

      drawTop();

      // child scroll window begin
      ImGui::BeginChild (kChildScrollStr, ImVec2(0,0), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
      ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, ImVec2(0,0));
      ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

      // clipper begin
      ImGuiListClipper clipper;
      clipper.Begin (mContext.mNumLines, mContext.mLineHeight);
      clipper.Step();

      if (isValid (mEdit.mNextEditAddress)) {
        //{{{  calc minimum scroll to keep mNextEditAddress on screen
        int lineNumber = static_cast<int>(mEdit.mEditAddress / mOptions.mColumns);
        int nextLineNumber = static_cast<int>(mEdit.mNextEditAddress / mOptions.mColumns);

        int lineNumberOffset = nextLineNumber - lineNumber;
        if (lineNumberOffset) {
          // nextEditAddress is on new line
          bool beforeStart = (lineNumberOffset < 0) && (nextLineNumber < (clipper.DisplayStart + kPageMarginLines));
          bool afterEnd = (lineNumberOffset > 0) && (nextLineNumber >= (clipper.DisplayEnd - kPageMarginLines));

          // is minimum scroll neeeded
          if (beforeStart || afterEnd)
            ImGui::SetScrollY (ImGui::GetScrollY() + (lineNumberOffset * mContext.mLineHeight));
          }
        }
        //}}}

      // clipper iterate
      for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++)
        drawLine (lineNumber);

      // clipper end
      clipper.Step();
      clipper.End();

      // child end
      ImGui::PopStyleVar (2);
      ImGui::EndChild();

      // Notify the main window of our ideal child content size
      ImGui::SetCursorPosX (mContext.mWindowWidth);
      if (mEdit.mDataNext && (mEdit.mEditAddress < mInfo.mMemSize)) {
        mEdit.mEditAddress++;
        mEdit.mDataAddress = mEdit.mEditAddress;
        mEdit.mEditTakeFocus = true;
        }
      else if (isValid (mEdit.mNextEditAddress)) {
        mEdit.mEditAddress = mEdit.mNextEditAddress;
        mEdit.mDataAddress = mEdit.mNextEditAddress;
        }
      }
    //}}}
    //}}}
    //{{{  sets
    //{{{
    void setMem (uint8_t* memData, size_t memSize) {
      mInfo.mMemData = memData;
      mInfo.mMemSize = memSize;
      }
    //}}}
    //{{{
    void setAddressHighlight (size_t addressMin, size_t addressMax) {

      mEdit.mGotoAddress = addressMin;

      mEdit.mHighlightMin = addressMin;
      mEdit.mHighlightMax = addressMax;
      }
    //}}}
    //}}}
    //{{{  actions

    //{{{
    void moveLeft() {

      if (isValid (mEdit.mEditAddress)) {
        if (mEdit.mEditAddress > 0) {
          mEdit.mNextEditAddress = mEdit.mEditAddress - 1;
          mEdit.mEditTakeFocus = true;
          }
        }
      }
    //}}}
    //{{{
    void moveRight() {

      if (isValid (mEdit.mEditAddress)) {
        if (mEdit.mEditAddress < mInfo.mMemSize - 2) {
          mEdit.mNextEditAddress = mEdit.mEditAddress + 1;
          mEdit.mEditTakeFocus = true;
          }
        }
      }
    //}}}
    //{{{
    void moveLineUp() {

      if (isValid (mEdit.mEditAddress)) {
        if (mEdit.mEditAddress >= (size_t)mOptions.mColumns) {
          mEdit.mNextEditAddress = mEdit.mEditAddress - mOptions.mColumns;
          mEdit.mEditTakeFocus = true;
          }
        }
      }
    //}}}
    //{{{
    void moveLineDown() {

      if (isValid (mEdit.mEditAddress)) {
        if (mEdit.mEditAddress < mInfo.mMemSize - mOptions.mColumns) {
          mEdit.mNextEditAddress = mEdit.mEditAddress + mOptions.mColumns;
          mEdit.mEditTakeFocus = true;
          }
        }
      }
    //}}}
    //{{{
    void movePageUp() {

      if (isValid (mEdit.mEditAddress)) {
        if (mEdit.mEditAddress >= (size_t)mOptions.mColumns) {
          // could be better
          mEdit.mNextEditAddress = mEdit.mEditAddress;
          int lines = mContext.mNumPageLines;
          while ((lines-- > 0) && (mEdit.mNextEditAddress >= (size_t)mOptions.mColumns))
            mEdit.mNextEditAddress -= mOptions.mColumns;
          mEdit.mEditTakeFocus = true;
          }
        }
      }
    //}}}
    //{{{
    void movePageDown() {

      if (isValid (mEdit.mEditAddress)) {
        if (mEdit.mEditAddress < mInfo.mMemSize - mOptions.mColumns) {
          // could be better
          mEdit.mNextEditAddress = mEdit.mEditAddress;
          int lines = mContext.mNumPageLines;
          while ((lines-- > 0)  && (mEdit.mNextEditAddress < (mInfo.mMemSize - mOptions.mColumns)))
            mEdit.mNextEditAddress += mOptions.mColumns;
          mEdit.mEditTakeFocus = true;
          }
        }
      }
    //}}}
    //{{{
    void moveHome() {

      if (isValid (mEdit.mEditAddress)) {
        if (mEdit.mEditAddress > 0) {
          mEdit.mNextEditAddress = 0;
          mEdit.mEditTakeFocus = true;
          }
        }
      }
    //}}}
    //{{{
    void moveEnd() {

      if (isValid (mEdit.mEditAddress)) {
        if (mEdit.mEditAddress < (mInfo.mMemSize - 2)) {
          mEdit.mNextEditAddress = mInfo.mMemSize - 1;
          mEdit.mEditTakeFocus = true;
          }
        }
      }
    //}}}
    //}}}

  private:
    enum class eDataFormat { eDec, eBin, eHex };
    static const size_t kUndefinedAddress = (size_t)-1;
    //{{{
    static bool isCpuBigEndian() {
    // is cpu bigEndian

      uint16_t x = 1;

      char c[2];
      memcpy (c, &x, 2);

      return c[0] != 0;
      }
    //}}}

    //{{{
    class cOptions {
    public:
      bool mReadOnly = false;

      int mColumns = 16;
      int mColumnExtraSpace = 8;

      bool mShowHexII = false;
      bool mHoverHexII = false;

      bool mBigEndian = false;
      bool mHoverEndian = false;

      ImGuiDataType mDataType = ImGuiDataType_U8;
      };
    //}}}
    //{{{
    class cInfo {
    public:
      //{{{
      cInfo (uint8_t* memData, size_t memSize)
        : mMemData(memData), mMemSize(memSize), mBaseAddress(kUndefinedAddress) {}
      //}}}
      //{{{
      void setBaseAddress (size_t baseAddress) {
        mBaseAddress = baseAddress;
        }
      //}}}

      // vars
      uint8_t* mMemData = nullptr;
      size_t mMemSize = kUndefinedAddress;
      size_t mBaseAddress = kUndefinedAddress;
      };
    //}}}
    //{{{
    class cContext {
    public:
      //{{{
      void update (const cOptions& options, const cInfo& info) {

        // address box size
        mAddressDigitsCount = 0;
        for (size_t n = info.mBaseAddress + info.mMemSize - 1; n > 0; n >>= 4)
          mAddressDigitsCount++;

        // char size
        mGlyphWidth = ImGui::CalcTextSize (" ").x; // assume monoSpace font
        mLineHeight = ImGui::GetTextLineHeight();
        mHexCellWidth = (float)(int)(mGlyphWidth * 2.5f);
        mExtraSpaceWidth = (float)(int)(mHexCellWidth * 0.25f);

        // hex begin,end
        mHexBeginPos = (mAddressDigitsCount + 2) * mGlyphWidth;
        mHexEndPos = mHexBeginPos + (mHexCellWidth * options.mColumns);
        mAsciiBeginPos = mAsciiEndPos = mHexEndPos;

        // ascii begin,end
        mAsciiBeginPos = mHexEndPos + mGlyphWidth * 1;
        if (options.mColumnExtraSpace > 0)
          mAsciiBeginPos += (float)((options.mColumns + options.mColumnExtraSpace - 1) /
            options.mColumnExtraSpace) * mExtraSpaceWidth;
        mAsciiEndPos = mAsciiBeginPos + options.mColumns * mGlyphWidth;

        // windowWidth
        ImGuiStyle& style = ImGui::GetStyle();
        mWindowWidth = (2.f*style.WindowPadding.x) + mAsciiEndPos + style.ScrollbarSize + mGlyphWidth;

        // page up,down inc
        mNumPageLines = static_cast<int>(ImGui::GetWindowHeight() / ImGui::GetTextLineHeight()) - kPageMarginLines;

        // numLines of memory range
        mNumLines = static_cast<int>((info.mMemSize + (options.mColumns-1)) / options.mColumns);

        // colors
        mTextColor = ImGui::GetColorU32 (ImGuiCol_Text);
        mGreyColor = ImGui::GetColorU32 (ImGuiCol_TextDisabled);
        mHighlightColor = IM_COL32 (255, 255, 255, 50);  // highlight background color
        mFrameBgndColor = ImGui::GetColorU32 (ImGuiCol_FrameBg);
        mTextSelectBgndColor = ImGui::GetColorU32 (ImGuiCol_TextSelectedBg);
        }
      //}}}

      // vars
      float mGlyphWidth = 0;
      float mLineHeight = 0;

      int mAddressDigitsCount = 0;

      float mHexCellWidth = 0;
      float mExtraSpaceWidth = 0;
      float mHexBeginPos = 0;
      float mHexEndPos = 0;

      float mAsciiBeginPos = 0;
      float mAsciiEndPos = 0;

      float mWindowWidth = 0;
      int mNumPageLines = 0;

      int mNumLines = 0;

      ImU32 mTextColor;
      ImU32 mGreyColor;
      ImU32 mHighlightColor;
      ImU32 mFrameBgndColor;
      ImU32 mTextSelectBgndColor;
      };
    //}}}
    //{{{
    class cEdit {
    public:
      //{{{
      void begin (const cOptions& options, const cInfo& info) {

        mDataNext = false;

        if (mDataAddress >= info.mMemSize)
          mDataAddress = kUndefinedAddress;

        if (options.mReadOnly || (mEditAddress >= info.mMemSize))
          mEditAddress = kUndefinedAddress;

        mNextEditAddress = kUndefinedAddress;
        }
      //}}}

      bool mDataNext = false;
      size_t mDataAddress = kUndefinedAddress;

      bool mEditTakeFocus = false;
      size_t mEditAddress = kUndefinedAddress;
      size_t mNextEditAddress = kUndefinedAddress;

      char mDataInputBuf[32] = { 0 };
      char mAddressInputBuf[32] = { 0 };
      size_t mGotoAddress = kUndefinedAddress;

      size_t mHighlightMin = kUndefinedAddress;
      size_t mHighlightMax = kUndefinedAddress;
      };
    //}}}

    //{{{  gets
    bool isReadOnly() const { return mOptions.mReadOnly; };
    bool isValid (size_t address) { return address != kUndefinedAddress; }

    //{{{
    size_t getDataTypeSize (ImGuiDataType dataType) const {
      return kTypeSizes[dataType];
      }
    //}}}
    //{{{
    string getDataTypeDesc (ImGuiDataType dataType) const {
      return kTypeDescs[(size_t)dataType];
      }
    //}}}
    //{{{
    string getDataFormatDesc (cMemEdit::eDataFormat dataFormat) const {
      return kFormatDescs[(size_t)dataFormat];
      }
    //}}}
    //{{{
    string getDataString (size_t address, ImGuiDataType dataType, eDataFormat dataFormat) {
    // return pointer to mOutBuf

      array <char,128> outBuf;

      size_t elemSize = getDataTypeSize (dataType);
      size_t size = ((address + elemSize) > mInfo.mMemSize) ? (mInfo.mMemSize - address) : elemSize;

      uint8_t buf[8];
      memcpy (buf, mInfo.mMemData + address, size);

      outBuf[0] = 0;
      if (dataFormat == eDataFormat::eBin) {
        //{{{  eBin to outBuf
        uint8_t binbuf[8];
        copyEndian (binbuf, buf, size);

        int width = (int)size * 8;
        size_t outn = 0;
        int n = width / 8;
        for (int j = n - 1; j >= 0; --j) {
          for (int i = 0; i < 8; ++i)
            outBuf[outn++] = (binbuf[j] & (1 << (7 - i))) ? '1' : '0';
          outBuf[outn++] = ' ';
          }

        outBuf[outn] = 0;
        }
        //}}}
      else {
        // eHex, eDec to outBuf
        switch (dataType) {
          //{{{
          case ImGuiDataType_S8: {

            uint8_t int8 = 0;
            copyEndian (&int8, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%hhd", int8);
            else if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%02x", int8 & 0xFF);

            break;
            }
          //}}}
          //{{{
          case ImGuiDataType_U8: {

            uint8_t uint8 = 0;
            copyEndian (&uint8, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%hhu", uint8);
            if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%02x", uint8 & 0XFF);

            break;
            }
          //}}}
          //{{{
          case ImGuiDataType_S16: {

            int16_t int16 = 0;
            copyEndian (&int16, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%hd", int16);
            else if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%04x", int16 & 0xFFFF);

            break;
            }
          //}}}
          //{{{
          case ImGuiDataType_U16: {

            uint16_t uint16 = 0;
            copyEndian (&uint16, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%hu", uint16);
            else if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%04x", uint16 & 0xFFFF);

            break;
            }
          //}}}
          //{{{
          case ImGuiDataType_S32: {

            int32_t int32 = 0;
            copyEndian (&int32, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%d", int32);
            else if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%08x", int32);

            break;
            }
          //}}}
          //{{{
          case ImGuiDataType_U32: {
            uint32_t uint32 = 0;
            copyEndian (&uint32, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%u", uint32);
            else if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%08x", uint32);

            break;
            }
          //}}}
          //{{{
          case ImGuiDataType_S64: {

            int64_t int64 = 0;
            copyEndian (&int64, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%lld", (long long)int64);
            else if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%016llx", (long long)int64);

            break;
            }
          //}}}
          //{{{
          case ImGuiDataType_U64: {

            uint64_t uint64 = 0;
            copyEndian (&uint64, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%llu", (long long)uint64);
            else if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "0x%016llx", (long long)uint64);

            break;
            }
          //}}}
          //{{{
          case ImGuiDataType_Float: {

            float float32 = 0.f;
            copyEndian (&float32, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%f", float32);
            else if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%a", float32);

            break;
            }
          //}}}
          //{{{
          case ImGuiDataType_Double: {

            double float64 = 0.0;
            copyEndian (&float64, buf, size);

            if (dataFormat == eDataFormat::eDec)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%f", float64);
            else if (dataFormat == eDataFormat::eHex)
              ImSnprintf (outBuf.data(), outBuf.max_size(), "%a", float64);
            break;
            }
          //}}}
          }
        }

      return string (outBuf.data());
      }
    //}}}
    //}}}
    //{{{  sets
    void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }
    void toggleReadOnly() { mOptions.mReadOnly = !mOptions.mReadOnly; }
    //}}}

    //{{{
    void* copyEndian (void* dst, void* src, size_t size) {

      if (!gEndianFunc)
        gEndianFunc = isCpuBigEndian() ? bigEndianFunc : littleEndianFunc;

      return gEndianFunc (dst, src, size, mOptions.mBigEndian ^ mOptions.mHoverEndian);
      }
    //}}}
    //{{{
    void drawTop() {

      ImGuiStyle& style = ImGui::GetStyle();

      // draw columns button
      ImGui::SetNextItemWidth ((2.f*style.FramePadding.x) + (7.f*mContext.mGlyphWidth));
      ImGui::DragInt ("##column", &mOptions.mColumns, 0.2f, 2, 64, "%d wide");
      if (ImGui::IsItemHovered())
        mOptions.mColumns = max (2, min (64, mOptions.mColumns + static_cast<int>(ImGui::GetIO().MouseWheel)));

      // draw hexII button
      ImGui::SameLine();
      if (toggleButton ("hexII", mOptions.mShowHexII))
        mOptions.mShowHexII = !mOptions.mShowHexII;
      mOptions.mHoverHexII = ImGui::IsItemHovered();

      // draw gotoAddress button
      ImGui::SetNextItemWidth ((2 * style.FramePadding.x) + ((mContext.mAddressDigitsCount + 1) * mContext.mGlyphWidth));
      ImGui::SameLine();
      if (ImGui::InputText ("##address", mEdit.mAddressInputBuf, sizeof(mEdit.mAddressInputBuf),
                             ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        //{{{  consume input into gotoAddress
        size_t gotoAddress;
        if (sscanf (mEdit.mAddressInputBuf, "%" _PRISizeT "X", &gotoAddress) == 1) {
          mEdit.mGotoAddress = gotoAddress - mInfo.mBaseAddress;
          mEdit.mHighlightMin = mEdit.mHighlightMax = kUndefinedAddress;
          }
        }
        //}}}

      if (isValid (mEdit.mGotoAddress)) {
        //{{{  valid goto address
        if (mEdit.mGotoAddress < mInfo.mMemSize) {
          // use gotoAddress and scroll to it
          ImGui::BeginChild (kChildScrollStr);
          ImGui::SetScrollFromPosY (ImGui::GetCursorStartPos().y +
                                    (mEdit.mGotoAddress/mOptions.mColumns) * mContext.mLineHeight);
          ImGui::EndChild();

          mEdit.mDataAddress = mEdit.mGotoAddress;
          mEdit.mEditAddress = mEdit.mGotoAddress;
          mEdit.mEditTakeFocus = true;
          }

        mEdit.mGotoAddress = kUndefinedAddress;
        }
        //}}}

      if (isValid (mEdit.mDataAddress)) {
        // draw dataType combo
        ImGui::SetNextItemWidth ((2*style.FramePadding.x) + (8*mContext.mGlyphWidth) + style.ItemInnerSpacing.x);
        ImGui::SameLine();
        if (ImGui::BeginCombo ("##comboType", getDataTypeDesc (mOptions.mDataType).c_str(), ImGuiComboFlags_HeightLargest)) {
          for (int dataType = 0; dataType < ImGuiDataType_COUNT; dataType++)
            if (ImGui::Selectable (getDataTypeDesc((ImGuiDataType)dataType).c_str(), mOptions.mDataType == dataType))
              mOptions.mDataType = (ImGuiDataType)dataType;
          ImGui::EndCombo();
          }

        // draw endian combo
        if ((int)mOptions.mDataType > 1) {
          ImGui::SameLine();
          if (toggleButton ("big", mOptions.mBigEndian))
            mOptions.mBigEndian = !mOptions.mBigEndian;
          mOptions.mHoverEndian = ImGui::IsItemHovered();
          }

        // draw formats, !! why can't you inc,iterate an enum
        for (eDataFormat dataFormat = eDataFormat::eDec; dataFormat <= eDataFormat::eHex;
             dataFormat = static_cast<eDataFormat>((static_cast<int>(dataFormat)+1))) {
          ImGui::SameLine();
          ImGui::Text ("%s:%s", getDataFormatDesc (dataFormat).c_str(),
                                getDataString (mEdit.mDataAddress, mOptions.mDataType, dataFormat).c_str());
          }
        }
      }
    //}}}
    //{{{
    void drawLine (int lineNumber) {

      ImDrawList* draw_list = ImGui::GetWindowDrawList();

      // draw address
      size_t address = static_cast<size_t>(lineNumber * mOptions.mColumns);
      ImGui::Text (kFormatAddress, mContext.mAddressDigitsCount, mInfo.mBaseAddress + address);

      // draw hex
      for (int column = 0; column < mOptions.mColumns && address < mInfo.mMemSize; column++, address++) {
        float bytePosX = mContext.mHexBeginPos + mContext.mHexCellWidth * column;
        if (mOptions.mColumnExtraSpace > 0)
          bytePosX += (float)(column / mOptions.mColumnExtraSpace) * mContext.mExtraSpaceWidth;

        // next hex value text
        ImGui::SameLine (bytePosX);

        // highlight
        bool isHighlightRange = ((address >= mEdit.mHighlightMin) && (address < mEdit.mHighlightMax));
        bool isHighlightData = (address >= mEdit.mDataAddress) &&
                               (address < (mEdit.mDataAddress + getDataTypeSize (mOptions.mDataType)));
        if (isHighlightRange || isHighlightData) {
          //{{{  draw hex highlight
          ImVec2 pos = ImGui::GetCursorScreenPos();

          float highlightWidth = 2 * mContext.mGlyphWidth;
          bool isNextByteHighlighted = (address+1 < mInfo.mMemSize) &&
                                       ((isValid (mEdit.mHighlightMax) && (address + 1 < mEdit.mHighlightMax)));

          if (isNextByteHighlighted || ((column + 1) == mOptions.mColumns)) {
            highlightWidth = mContext.mHexCellWidth;
            if ((mOptions.mColumnExtraSpace > 0) &&
                (column > 0) && ((column + 1) < mOptions.mColumns) &&
                (((column + 1) % mOptions.mColumnExtraSpace) == 0))
              highlightWidth += mContext.mExtraSpaceWidth;
            }

          ImVec2 endPos = { pos.x + highlightWidth, pos.y + mContext.mLineHeight};
          draw_list->AddRectFilled (pos, endPos, mContext.mHighlightColor);
          }
          //}}}
        if (mEdit.mEditAddress == address) {
          //{{{  display text input on current byte
          bool dataWrite = false;

          ImGui::PushID ((void*)address);
          if (mEdit.mEditTakeFocus) {
            ImGui::SetKeyboardFocusHere();
            ImGui::CaptureKeyboardFromApp (true);
            sprintf (mEdit.mAddressInputBuf, kFormatData, mContext.mAddressDigitsCount, mInfo.mBaseAddress + address);
            sprintf (mEdit.mDataInputBuf, kFormatByte, mInfo.mMemData[address]);
            }

          //{{{
          struct sUserData {
            char mCurrentBufOverwrite[3];  // Input
            int mCursorPos;                // Output

            static int callback (ImGuiInputTextCallbackData* inputTextCallbackData) {
            // FIXME: We should have a way to retrieve the text edit cursor position more easily in the API
            //  this is rather tedious. This is such a ugly mess we may be better off not using InputText() at all here.

              sUserData* userData = (sUserData*)inputTextCallbackData->UserData;
              if (!inputTextCallbackData->HasSelection())
                userData->mCursorPos = inputTextCallbackData->CursorPos;

              if ((inputTextCallbackData->SelectionStart == 0) &&
                  (inputTextCallbackData->SelectionEnd == inputTextCallbackData->BufTextLen)) {
                // When not Edit a byte, always rewrite its content
                // - this is a bit tricky, since InputText technically "owns"
                //   the master copy of the buffer we edit it in there
                inputTextCallbackData->DeleteChars (0, inputTextCallbackData->BufTextLen);
                inputTextCallbackData->InsertChars (0, userData->mCurrentBufOverwrite);
                inputTextCallbackData->SelectionStart = 0;
                inputTextCallbackData->SelectionEnd = 2;
                inputTextCallbackData->CursorPos = 0;
                }
              return 0;
              }
            };
          //}}}
          sUserData userData;
          userData.mCursorPos = -1;

          sprintf (userData.mCurrentBufOverwrite, kFormatByte, mInfo.mMemData[address]);
          ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal |
                                      ImGuiInputTextFlags_EnterReturnsTrue |
                                      ImGuiInputTextFlags_AutoSelectAll |
                                      ImGuiInputTextFlags_NoHorizontalScroll |
                                      ImGuiInputTextFlags_CallbackAlways;
          flags |= ImGuiInputTextFlags_AlwaysOverwrite;

          ImGui::SetNextItemWidth (mContext.mGlyphWidth * 2);

          if (ImGui::InputText ("##data", mEdit.mDataInputBuf, sizeof(mEdit.mDataInputBuf), flags, sUserData::callback, &userData))
            dataWrite = mEdit.mDataNext = true;
          else if (!mEdit.mEditTakeFocus && !ImGui::IsItemActive())
            mEdit.mEditAddress = mEdit.mNextEditAddress = kUndefinedAddress;

          mEdit.mEditTakeFocus = false;

          if (userData.mCursorPos >= 2)
            dataWrite = mEdit.mDataNext = true;
          if (isValid (mEdit.mNextEditAddress))
            dataWrite = mEdit.mDataNext = false;

          unsigned int dataInputValue = 0;
          if (dataWrite && sscanf(mEdit.mDataInputBuf, "%X", &dataInputValue) == 1)
            mInfo.mMemData[address] = (ImU8)dataInputValue;

          ImGui::PopID();
          }
          //}}}
        else {
          //{{{  display text
          // NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click on.
          uint8_t value = mInfo.mMemData[address];

          if (mOptions.mShowHexII || mOptions.mHoverHexII) {
            if ((value >= 32) && (value < 128))
              ImGui::Text (".%c ", value);
            else if (kGreyZeroes && (value == 0xFF))
              ImGui::TextDisabled ("## ");
            else if (value == 0x00)
              ImGui::Text ("   ");
            else
              ImGui::Text (kFormatByteSpace, value);
            }
          else if (kGreyZeroes && (value == 0))
            ImGui::TextDisabled ("00 ");
          else
            ImGui::Text (kFormatByteSpace, value);

          if (!mOptions.mReadOnly &&
              ImGui::IsItemHovered() && ImGui::IsMouseClicked (0)) {
            mEdit.mEditTakeFocus = true;
            mEdit.mNextEditAddress = address;
            }
          }
          //}}}
        }

      if (kShowAscii) {
        //{{{  draw righthand ASCII
        ImGui::SameLine (mContext.mAsciiBeginPos);

        ImVec2 pos = ImGui::GetCursorScreenPos();
        address = lineNumber * mOptions.mColumns;

        // invisibleButton over righthand ascii text for mouse hits
        ImGui::PushID (lineNumber);
        if (ImGui::InvisibleButton ("ascii", ImVec2 (mContext.mAsciiEndPos - mContext.mAsciiBeginPos, mContext.mLineHeight))) {
          mEdit.mDataAddress = address + (size_t)((ImGui::GetIO().MousePos.x - pos.x) / mContext.mGlyphWidth);
          mEdit.mEditAddress = mEdit.mDataAddress;
          mEdit.mEditTakeFocus = true;
          }
        ImGui::PopID();

        for (int column = 0; (column < mOptions.mColumns) && (address < mInfo.mMemSize); column++, address++) {
          if (address == mEdit.mEditAddress) {
            ImVec2 endPos = { pos.x + mContext.mGlyphWidth, pos.y + mContext.mLineHeight };
            draw_list->AddRectFilled (pos, endPos, mContext.mFrameBgndColor);
            draw_list->AddRectFilled (pos, endPos, mContext.mTextSelectBgndColor);
            }

          uint8_t value = mInfo.mMemData[address];
          char displayCh = ((value < 32) || (value >= 128)) ? '.' : value;
          ImU32 color = (!kGreyZeroes || (value == displayCh)) ? mContext.mTextColor : mContext.mGreyColor;
          draw_list->AddText (pos, color, &displayCh, &displayCh + 1);

          pos.x += mContext.mGlyphWidth;
          }
        }
        //}}}
      }
    //}}}

    //{{{
    void keyboard() {

      struct sActionKey {
        bool mAlt;
        bool mCtrl;
        bool mShift;
        ImGuiKey mGuiKey;
        function <void()> mActionFunc;
        };

      const vector<sActionKey> kActionKeys = {
      // alt    ctrl   shift  guiKey               function
        {false, false, false, ImGuiKey_LeftArrow,  [this]{moveLeft();}},
        {false, false, false, ImGuiKey_RightArrow, [this]{moveRight();}},
        {false, false, false, ImGuiKey_UpArrow,    [this]{moveLineUp();}},
        {false, false, false, ImGuiKey_DownArrow,  [this]{moveLineDown();}},
        {false, false, false, ImGuiKey_PageUp,     [this]{movePageUp();}},
        {false, false, false, ImGuiKey_PageDown,   [this]{movePageDown();}},
        {false, false, false, ImGuiKey_Home,       [this]{moveHome();}},
        {false, false, false, ImGuiKey_End,        [this]{moveEnd();}},
        };

      ImGui::GetIO().WantTextInput = true;
      ImGui::GetIO().WantCaptureKeyboard = false;

      if (ImGui::IsWindowHovered())
        ImGui::SetMouseCursor (ImGuiMouseCursor_TextInput);

      ImGuiIO& io = ImGui::GetIO();
      bool shift = io.KeyShift;
      bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
      bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;
      io.WantCaptureKeyboard = true;
      io.WantTextInput = true;

      // look for action keys
      for (auto& actionKey : kActionKeys) {
        if (ImGui::IsKeyPressed (actionKey.mGuiKey) &&
            (actionKey.mCtrl == ctrl) &&
            (actionKey.mShift == shift) &&
            (actionKey.mAlt == alt)) {
          actionKey.mActionFunc();
          break;
          }
        }
      }
    //}}}

    //{{{  vars
    bool mOpen = true;  // set false when DrawWindow() closed

    cInfo mInfo;
    cContext mContext;
    cEdit mEdit;
    cOptions mOptions;

    uint64_t mStartTime;
    float mLastClick;
    //}}}
    };

  //{{{  undefines, pop warnings
  #undef _PRISizeT
  #undef ImSnprintf

  #ifdef _MSC_VER
    #pragma warning (pop)
  #endif
  //}}}
  //}}}
  //{{{
  class cEditUI : public cUI {
  public:
    cEditUI (const string& name) : cUI(name) {}
    //{{{
    virtual ~cEditUI() {

      delete mFileView;
      delete mMemEdit;
      }
    //}}}

    //{{{
    void addToDrawList (cApp& app) final {

      ImGui::SetNextWindowPos (ImVec2(0,0));
      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

      if (false) {
        //{{{  fileView memEdit
        mFileView = new cFileView ("C:/projects/paint/build/Release/fed.exe");

        if (!mMemEdit)
          mMemEdit = new cMemEdit ((uint8_t*)(this), 0x10000);

        ImGui::PushFont (app.getMonoFont());
        mMemEdit->setMem (mFileView->getReadPtr(), mFileView->getReadBytesLeft());
        mMemEdit->drawWindow ("Memory Editor", 0);
        ImGui::PopFont();

        return;
        }
        //}}}

      if ((dynamic_cast<cEditApp&>(app)).getMemEdit()) {
        //{{{  memEdit
        if (!mMemEdit)
          mMemEdit = new cMemEdit ((uint8_t*)(this), 0x10000);

        ImGui::PushFont (app.getMonoFont());
        mMemEdit->setMem ((uint8_t*)this, 0x80000);
        mMemEdit->drawWindow ("MemEdit", 0);
        ImGui::PopFont();

        return;
        }
        //}}}
      else
        mFedEdit.draw (app);
      }
    //}}}

  private:
    cFedEdit mFedEdit;
    cFileView* mFileView = nullptr;
    cMemEdit* mMemEdit = nullptr;

    // self registration
    static cUI* create (const string& className) { return new cEditUI (className); }
    inline static const bool mRegistered = registerClass ("edit", &create);
    };
  //}}}
  }

int main (int numArgs, char* args[]) {

  // params
  eLogLevel logLevel = LOGINFO;
  bool fullScreen = false;
  bool memEdit = false;
  bool vsync = true;
  //{{{  parse command line args to params
  // args to params
  vector <string> params;
  for (int i = 1; i < numArgs; i++)
    params.push_back (args[i]);

  // parse and remove recognised params
  for (auto it = params.begin(); it < params.end();) {
    if (*it == "log1") { logLevel = LOGINFO1; params.erase (it); }
    else if (*it == "log2") { logLevel = LOGINFO2; params.erase (it); }
    else if (*it == "log3") { logLevel = LOGINFO3; params.erase (it); }
    else if (*it == "full") { fullScreen = true; params.erase (it); }
    else if (*it == "mem") { memEdit = true; params.erase (it); }
    else if (*it == "free") { vsync = false; params.erase (it); }
    else ++it;
    };
  //}}}

  // log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("edit"));

  // list static registered UI classes
  cUI::listRegisteredClasses();

  // app
  cEditApp editApp ({1000, 900}, fullScreen, vsync);
  editApp.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 16.f));
  editApp.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 16.f));
  editApp.setDocumentName (params.empty() ? "../../fed/cEditUI.cpp" : params[0], memEdit);
  editApp.mainUILoop();

  return EXIT_SUCCESS;
  }
