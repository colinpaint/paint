// cDocument.cpp - text document for editing
//{{{  includes
#include "cDocument.h"

#include <cmath>
#include <algorithm>
#include <functional>

#include <fstream>
#include <filesystem>

#include <array>

#include "../utils/cLog.h"

using namespace std;
//}}}
namespace {
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
  }

// cTextEdit::cLanguage
//{{{
const cLanguage cLanguage::c() {

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
const cLanguage cLanguage::hlsl() {

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
const cLanguage cLanguage::glsl() {

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

// cLine
//{{{
string cLine::getString() const {

   string lineString;
   lineString.reserve (mGlyphs.size());

   for (auto& glyph : mGlyphs)
     for (uint32_t utf8index = 0; utf8index < glyph.mNumUtf8Bytes; utf8index++)
       lineString += glyph.mUtfChar[utf8index];

   return lineString;
   }
//}}}
//{{{
uint32_t cLine::trimTrailingSpace() {

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
void cLine::parse (const cLanguage& language) {
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
//{{{
uint32_t cDocument::trimTrailingSpace() {
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
//{{{
void cLine::parseTokens (const cLanguage& language, const string& textString) {
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

// cDocument
//{{{
cDocument::cDocument() {
  mLanguage = cLanguage::c();
  addEmptyLine();
  }
//}}}
//{{{
string cDocument::getText (const sPosition& beginPosition, const sPosition& endPosition) {
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
//{{{
vector<string> cDocument::getTextStrings() const {
// get text as vector of string

  std::vector<std::string> result;
  result.reserve (getNumLines());

  for (const auto& line : mLines) {
    std::string lineString = line.getString();
    result.emplace_back (move (lineString));
    }

  return result;
  }
//}}}

//{{{
uint32_t cDocument::getGlyphIndex (const cLine& line, uint32_t toColumn) {
// return glyphIndex from line,column, inserting tabs whose width is owned by cDocument

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
//{{{
uint32_t cDocument::getColumn (const cLine& line, uint32_t toGlyphIndex) {
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
uint32_t cDocument::getTabColumn (uint32_t column) {
// return column of after tab at column
  return ((column / mTabSize) * mTabSize) + mTabSize;
  }
//}}}
//{{{
uint32_t cDocument::getNumColumns (const cLine& line) {

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
void cDocument::addEmptyLine() {
  mLines.push_back (cLine());
  }
//}}}
//{{{
void cDocument::appendLineToPrev (uint32_t lineNumber) {
// append line to prev

  cLine& line = getLine (lineNumber);
  cLine& prevLine = getLine (lineNumber-1);

  prevLine.appendLine (line, 0);
  mLines.erase (mLines.begin() + lineNumber);

  parse (prevLine);
  }
//}}}
//{{{
void cDocument::insertChar (cLine& line, uint32_t glyphIndex, ImWchar ch) {

  line.insert (glyphIndex, cGlyph (ch, eText));
  parse (line);
  edited();
  }
//}}}
//{{{
void cDocument::breakLine (cLine& line, uint32_t glyphIndex, uint32_t newLineNumber, uint32_t indent) {

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
//{{{
void cDocument::joinLine (cLine& joinToLine, uint32_t joinFromLineNumber) {
// join lineNumber to line

  // append joinFromLineNumber to joinToLine
  joinToLine.appendLine (getLine (joinFromLineNumber), 0);
  parse (joinToLine);

  // delete joinFromLineNumber
  mLines.erase (mLines.begin() + joinFromLineNumber);
  edited();
  }
//}}}

//{{{
void cDocument::deleteChar (cLine& line, uint32_t glyphIndex) {

  line.erase (glyphIndex);
  parse (line);

  edited();
  }
//}}}
//{{{
void cDocument::deleteChar (cLine& line, const sPosition& position) {
  deleteChar (line, getGlyphIndex (line, position.mColumn));
  }
//}}}
//{{{
void cDocument::deleteLine (uint32_t lineNumber) {

  mLines.erase (mLines.begin() + lineNumber);
  edited();
  }
//}}}
//{{{
void cDocument::deleteLineRange (uint32_t beginLineNumber, uint32_t endLineNumber) {

  mLines.erase (mLines.begin() + beginLineNumber, mLines.begin() + endLineNumber);
  edited();
  }
//}}}
//{{{
void cDocument::deletePositionRange (const sPosition& beginPosition, const sPosition& endPosition) {
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

//{{{
void cDocument::parseAll() {
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
void cDocument::edited() {

  mParseFlag = true;
  mEdited |= true;
  }
//}}}

//{{{
void cDocument::load (const string& filename) {

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
void cDocument::save() {

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
