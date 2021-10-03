// cTextEdit.cpp - folding text editor
//{{{  includes
#include "cTextEdit.h"

#include <cmath>
#include <algorithm>
#include <functional>
#include <chrono>

#include <filesystem>
#include <fstream>

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

  constexpr uint8_t eSelectHighlight =  11;
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

    0x80600000, // eSelectHighlight
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

  mOptions.mLanguage = cLanguage::c();
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

  for (const auto& line : mDoc.mLines) {
    string lineString;
    lineString.resize (line.getNumGlyphs());
    for (uint32_t glyphIndex = 0; glyphIndex < line.getNumGlyphs(); glyphIndex++)
      lineString[glyphIndex] = line.mGlyphs[glyphIndex].mChar;

    result.emplace_back (move (lineString));
    }

  return result;
  }
//}}}
//}}}
//{{{  actions
//{{{
void cTextEdit::toggleShowFolded() {

  mOptions.mShowFolded = !mOptions.mShowFolded;
  mEdit.mScrollVisible = true;
  }
//}}}

// move
//{{{
void cTextEdit::moveLeft() {

  sPosition position = mEdit.mCursor.mPosition;

  uint32_t characterIndex = getCharacterIndex (position);
  if (characterIndex == 0) {
    // beginning of line
    if (position.mLineNumber > 0) {
      // move to end of prevous line
      uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber) - 1;
      position.mLineNumber = getLineNumberFromIndex (lineIndex);
      characterIndex = getNumGlyphs (position.mLineNumber);
      }
    }
  else {
    // move to previous column on same line
    characterIndex--;
    while ((characterIndex > 0) &&
           isUtfSequence (getGlyphs (position.mLineNumber)[characterIndex].mChar))
      characterIndex--;
    }

  setCursorPosition ({position.mLineNumber, getCharacterColumn (position.mLineNumber, characterIndex)});
  setDeselect();
  }
//}}}
//{{{
void cTextEdit::moveRight() {

  sPosition position = mEdit.mCursor.mPosition;

  // column
  uint32_t characterIndex = getCharacterIndex (position);
  if (characterIndex < getNumGlyphs (position.mLineNumber)) {
    // move to next columm on same line
    characterIndex += utf8CharLength (getGlyphs (position.mLineNumber)[characterIndex].mChar);
    uint32_t column = getCharacterColumn (position.mLineNumber, characterIndex);

    setCursorPosition ({position.mLineNumber, column});
    setDeselect();
    }
  else {
    // move to start of next line
    uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
    if (lineIndex + 1 < getNumLines()) {
      setCursorPosition ({getLineNumberFromIndex (lineIndex + 1), 0});
      setDeselect();
      }
    }
  }
//}}}
//{{{
void cTextEdit::moveRightWord() {

  sPosition position = mEdit.mCursor.mPosition;

  if (getCharacterIndex (position) < getNumGlyphs (position.mLineNumber)) {
    // move to next word, may jump to next line, what about folded???
    position = findNextWord (position);
    uint32_t characterIndex = getCharacterIndex (position);
    uint32_t column = getCharacterColumn (position.mLineNumber, characterIndex);
    setCursorPosition ({position.mLineNumber, column});
    setDeselect();
    }
  }
//}}}

// select
//{{{
void cTextEdit::selectAll() {
  setSelect (eSelect::eNormal, {0,0}, {getNumLines(),0});
  }
//}}}

// cut and paste
//{{{
void cTextEdit::copy() {

  if (hasSelect())
    // copy selected text to clipboard
    ImGui::SetClipboardText (getSelectedText().c_str());
  else {
    // copy current line to clipboard
    string text;
    for (const auto& glyph : getGlyphs (getCursorPosition().mLineNumber))
      text.push_back (glyph.mChar);
    ImGui::SetClipboardText (text.c_str());
    }
  }
//}}}
//{{{
void cTextEdit::cut() {

  if (isReadOnly())
    copy();

  else if (hasSelect()) {
    cUndo undo;
    undo.mBefore = mEdit.mCursor;
    undo.mRemove = getSelectedText();
    undo.mRemoveBegin = mEdit.mCursor.mSelectBegin;
    undo.mRemoveEnd = mEdit.mCursor.mSelectEnd;

    // copy selected text
    copy();

    // delete selected text
    deleteSelect();

    undo.mAfter = mEdit.mCursor;
    mEdit.addUndo (undo);
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
      // save and delete select
      undo.mRemove = getSelectedText();
      undo.mRemoveBegin = mEdit.mCursor.mSelectBegin;
      undo.mRemoveEnd = mEdit.mCursor.mSelectEnd;
      deleteSelect();
      }

    string clipboardText = ImGui::GetClipboardText();
    undo.mAdd = clipboardText;
    undo.mAddBegin = getCursorPosition();
    insertText (clipboardText);
    undo.mAddEnd = getCursorPosition();
    undo.mAfter = mEdit.mCursor;
    mEdit.addUndo (undo);
    }
  }
//}}}

// delete
//{{{
void cTextEdit::deleteIt() {

  cUndo undo;
  undo.mBefore = mEdit.mCursor;

  if (hasSelect()) {
    undo.mRemove = getSelectedText();
    undo.mRemoveBegin = mEdit.mCursor.mSelectBegin;
    undo.mRemoveEnd = mEdit.mCursor.mSelectEnd;
    deleteSelect();
    }

  else {
    sPosition position = getCursorPosition();
    setCursorPosition (position);

    cLine& line = getLine (position.mLineNumber);
    if (position.mColumn == getLineMaxColumn (position.mLineNumber)) {
      if (position.mLineNumber == getNumLines()-1)
        return;

      undo.mRemove = '\n';
      undo.mRemoveBegin = undo.mRemoveEnd = getCursorPosition();
      advance (undo.mRemoveEnd);

      cLine& nextLine = getLine (position.mLineNumber+1);
      line.mGlyphs.insert (line.mGlyphs.end(), nextLine.mGlyphs.begin(), nextLine.mGlyphs.end());
      parseLine (line);

      removeLine (position.mLineNumber + 1);
      }

    else {
      undo.mRemoveBegin = undo.mRemoveEnd = getCursorPosition();
      undo.mRemoveEnd.mColumn++;
      undo.mRemove = getText (undo.mRemoveBegin, undo.mRemoveEnd);

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
  mEdit.addUndo (undo);
  }
//}}}
//{{{
void cTextEdit::backspace() {

  cUndo undo;
  undo.mBefore = mEdit.mCursor;

  if (hasSelect()) {
    // delete select
    undo.mRemove = getSelectedText();
    undo.mRemoveBegin = mEdit.mCursor.mSelectBegin;
    undo.mRemoveEnd = mEdit.mCursor.mSelectEnd;
    deleteSelect();
    }

  else {
    sPosition position = getCursorPosition();
    if (mEdit.mCursor.mPosition.mColumn == 0) {
      if (mEdit.mCursor.mPosition.mLineNumber == 0)
        return;

      undo.mRemove = '\n';
      undo.mRemoveBegin = {position.mLineNumber - 1, getLineMaxColumn (position.mLineNumber - 1)};
      undo.mRemoveEnd = undo.mRemoveBegin;
      advance (undo.mRemoveEnd);

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

      undo.mRemoveBegin = undo.mRemoveEnd = getCursorPosition();
      --undo.mRemoveBegin.mColumn;
      --mEdit.mCursor.mPosition.mColumn;

      while ((characterIndex < line.getNumGlyphs()) && (characterIndexEnd-- > characterIndex)) {
        undo.mRemove += line.mGlyphs[characterIndex].mChar;
        line.mGlyphs.erase (line.mGlyphs.begin() + characterIndex);
        }
      parseLine (line);
      }

    mDoc.mEdited = true;
    mEdit.mScrollVisible = true;
    }

  undo.mAfter = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}

// insert
//{{{
void cTextEdit::enterCharacter (ImWchar ch) {

  cUndo undo;
  undo.mBefore = mEdit.mCursor;

  if (hasSelect()) {
    if ((ch == '\t') &&
        (mEdit.mCursor.mSelectBegin.mLineNumber != mEdit.mCursor.mSelectEnd.mLineNumber)) {
      //{{{  tab all select lines
      sPosition selectBegin = mEdit.mCursor.mSelectBegin;
      sPosition selectEnd = mEdit.mCursor.mSelectEnd;
      sPosition originalEnd = selectEnd;

      if (selectBegin > selectEnd)
        swap (selectBegin, selectEnd);

      selectBegin.mColumn = 0;
      if ((selectEnd.mColumn == 0) && (selectEnd.mLineNumber > 0))
        --selectEnd.mLineNumber;
      if (selectEnd.mLineNumber >= getNumLines())
        selectEnd.mLineNumber = getNumLines()-1;
      selectEnd.mColumn = getLineMaxColumn (selectEnd.mLineNumber);

      undo.mRemoveBegin = selectBegin;
      undo.mRemoveEnd = selectEnd;
      undo.mRemove = getText (selectBegin, selectEnd);

      bool modified = false;
      for (uint32_t lineNumber = selectBegin.mLineNumber; lineNumber <= selectEnd.mLineNumber; lineNumber++) {
        auto& glyphs = getGlyphs (lineNumber);
        glyphs.insert (glyphs.begin(), cGlyph ('\t', eTab));
        modified = true;
        }

      if (modified) {
        //{{{  not sure what this does yet
        selectBegin = { selectBegin.mLineNumber, getCharacterColumn (selectBegin.mLineNumber, 0)};
        sPosition rangeEnd;
        if (originalEnd.mColumn != 0) {
          selectEnd = {selectEnd.mLineNumber, getLineMaxColumn (selectEnd.mLineNumber)};
          rangeEnd = selectEnd;
          undo.mAdd = getText (selectBegin, selectEnd);
          }
        else {
          selectEnd = {originalEnd.mLineNumber, 0};
          rangeEnd = {selectEnd.mLineNumber - 1, getLineMaxColumn (selectEnd.mLineNumber - 1)};
          undo.mAdd = getText (selectBegin, rangeEnd);
          }

        undo.mAddBegin = selectBegin;
        undo.mAddEnd = rangeEnd;
        undo.mAfter = mEdit.mCursor;

        mEdit.mCursor.mSelectBegin = selectBegin;
        mEdit.mCursor.mSelectEnd = selectEnd;
        mEdit.addUndo (undo);

        mDoc.mEdited = true;
        mEdit.mScrollVisible = true;
        }
        //}}}

      return;
      }
      //}}}
    else {
      //{{{  delete select line
      undo.mRemove = getSelectedText();
      undo.mRemoveBegin = mEdit.mCursor.mSelectBegin;
      undo.mRemoveEnd = mEdit.mCursor.mSelectEnd;
      deleteSelect();
      }
      //}}}
    }

  sPosition position = getCursorPosition();
  undo.mAddBegin = position;

  if (ch == '\n') {
    //{{{  enter lineFeed
    cLine& line = getLine (position.mLineNumber);

    // insert newLine
    insertLine (position.mLineNumber+1);
    cLine& newLine = getLine (position.mLineNumber+1);

    if (mOptions.mLanguage.mAutoIndentation)
      for (uint32_t glyphIndex = 0;
           (glyphIndex < line.getNumGlyphs()) &&
           isascii (line.mGlyphs[glyphIndex].mChar) && isblank (line.mGlyphs[glyphIndex].mChar); glyphIndex++)
        newLine.mGlyphs.push_back (line.mGlyphs[glyphIndex]);
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
    undo.mAdd = (char)ch;
    }
    //}}}
  else {
    // enter ImWchar
    array <char,7> utf8buf;
    uint32_t bufLength = imTextCharToUtf8 (utf8buf.data(), 7, ch);
    if (bufLength > 0) {
      //{{{  enter char
      utf8buf[bufLength] = '\0';
      cLine& line = getLine (position.mLineNumber);
      uint32_t characterIndex = getCharacterIndex (position);
      if (mOptions.mOverWrite && (characterIndex < line.getNumGlyphs())) {
        uint32_t length = utf8CharLength(line.mGlyphs[characterIndex].mChar);
        undo.mRemoveBegin = mEdit.mCursor.mPosition;
        undo.mRemoveEnd = {position.mLineNumber, getCharacterColumn (position.mLineNumber, characterIndex + length)};
        while ((length > 0) && (characterIndex < line.getNumGlyphs())) {
          undo.mRemove += line.mGlyphs[characterIndex].mChar;
          line.mGlyphs.erase (line.mGlyphs.begin() + characterIndex);
          length--;
          }
        }

      for (char* bufPtr = utf8buf.data(); *bufPtr != '\0'; bufPtr++, ++characterIndex)
        line.mGlyphs.insert (line.mGlyphs.begin() + characterIndex, cGlyph (*bufPtr, eText));

      parseLine (line);

      undo.mAdd = utf8buf.data();
      setCursorPosition ({position.mLineNumber, getCharacterColumn (position.mLineNumber, characterIndex)});
      }
      //}}}
    else
      return;
    }
  mDoc.mEdited = true;

  undo.mAddEnd = getCursorPosition();
  undo.mAfter = mEdit.mCursor;
  mEdit.addUndo (undo);

  mEdit.mScrollVisible = true;
  }
//}}}

// fold
//{{{
void cTextEdit::createFold() {

  if (isReadOnly())
    return;

  // !!!! temp string for now !!!!
  string text = mOptions.mLanguage.mFoldBeginMarker +
                 "  new fold - loads of detail to implement\n\n" +
                 mOptions.mLanguage.mFoldEndMarker +
                 "\n";
  cUndo undo;
  undo.mBefore = mEdit.mCursor;
  undo.mAdd = text;
  undo.mAddBegin = getCursorPosition();

  insertText (text);

  undo.mAddEnd = getCursorPosition();
  undo.mAfter = mEdit.mCursor;
  mEdit.addUndo (undo);
  }
//}}}

// undo
//{{{
void cTextEdit::undo (uint32_t steps) {

  if (hasUndo()) {
    mEdit.undo (this, steps);
    scrollCursorVisible();
    }
  }
//}}}
//{{{
void cTextEdit::redo (uint32_t steps) {

  if (hasRedo()) {
    mEdit.redo (this, steps);
    scrollCursorVisible();
    }
  }
//}}}
//}}}
//{{{
void cTextEdit::loadFile (const string& filename) {

  // parse filename path
  filesystem::path filePath (filename);
  mDoc.mFilePath = filePath.string();
  mDoc.mParentPath = filePath.parent_path().string();
  mDoc.mFileStem = filePath.stem().string();
  mDoc.mFileExtension = filePath.extension().string();

  cLog::log (LOGINFO, fmt::format ("setFile  {}", filename));
  cLog::log (LOGINFO, fmt::format ("- path   {}", mDoc.mFilePath));
  cLog::log (LOGINFO, fmt::format ("- parent {}", mDoc.mParentPath));
  cLog::log (LOGINFO, fmt::format ("- stem   {}", mDoc.mFileStem));
  cLog::log (LOGINFO, fmt::format ("- ext    {}", mDoc.mFileExtension));

  // clear doc
  mDoc.mLines.clear();

  string lineString;
  uint32_t lineNumber = 0;

  // read filename as stream
  ifstream stream (filename, ifstream::in);
  while (getline (stream, lineString)) {
    // create empty line, reserve glyphs for line
    mDoc.mLines.emplace_back (cLine::tGlyphs());
    mDoc.mLines.back().mGlyphs.reserve (lineString.size());

    // iterate char
    for (const auto ch : lineString) {
      if (ch == '\r') // CR ignored, but set flag
        mDoc.mHasCR = true;
      else {
        if (ch ==  '\t')
          mDoc.mHasTabs = true;
        mDoc.mLines.back().mGlyphs.emplace_back (cGlyph (ch, eText));
        }
      }

    // parse
    parseLine (mDoc.mLines.back());
    lineNumber++;
    }

  trimTrailingSpace();

  // add empty lastLine
  mDoc.mLines.emplace_back (cLine::tGlyphs());

  cLog::log (LOGINFO, fmt::format ("read {} lines {}", lineNumber, mDoc.mHasCR ? "hasCR" : ""));

  mDoc.mEdited = false;

  mEdit.clearUndo();
  }
//}}}
//{{{
void cTextEdit::saveFile() {

  if (!mDoc.mEdited) {
    cLog::log (LOGINFO,fmt::format ("{} unchanged, no save", mDoc.mFilePath));
    return;
    }

  // identify filePath for previous version
  filesystem::path saveFilePath (mDoc.mFilePath);
  saveFilePath.replace_extension (fmt::format ("{};{}", mDoc.mFileExtension, mDoc.mVersion++));
  while (filesystem::exists (saveFilePath)) {
    // version exits, increment version number
    cLog::log (LOGINFO,fmt::format ("skipping {}", saveFilePath.string()));
    saveFilePath.replace_extension (fmt::format ("{};{}", mDoc.mFileExtension, mDoc.mVersion++));
    }

  uint32_t highestLineNumber = trimTrailingSpace();

  // save ofstream
  ofstream stream (saveFilePath);
  for (uint32_t lineNumber = 0; lineNumber < highestLineNumber; lineNumber++) {
    string lineString;
    for (auto& glyph : mDoc.mLines[lineNumber].mGlyphs)
      lineString += glyph.mChar;
    stream.write (lineString.data(), lineString.size());
    stream.put ('\n');
    }

  // done
  cLog::log (LOGINFO,fmt::format ("{} saved", saveFilePath.string()));
  }
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
// main ui io,draw

  // top line buttons
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
  if (mDoc.mHasFolds) {
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
  //{{{  info
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:{} {}", getCursorPosition().mColumn+1, getCursorPosition().mLineNumber+1,
                                           getNumLines(), getLanguage().mName).c_str());
  //}}}
  //{{{  fontSize button
  ImGui::SameLine();
  ImGui::SetNextItemWidth (3 * ImGui::GetFontSize());
  ImGui::DragInt ("##fs", &mOptions.mFontSize, 0.2f, mOptions.mmDocntSize, mOptions.mMaxFontSize, "%d");

  if (ImGui::IsItemHovered()) {
    int fontSize = mOptions.mFontSize + static_cast<int>(ImGui::GetIO().MouseWheel);
    mOptions.mFontSize = max (mOptions.mmDocntSize, min (mOptions.mMaxFontSize, fontSize));
    }
  //}}}
  //{{{  vertice debug
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:vert:triangle",
               ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());
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
  if (isEdited()) {
    ImGui::SameLine();
    if (ImGui::Button ("save"))
      saveFile();
    }
  //}}}

  parseComments();

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

  if (mEdit.mScrollVisible)
    scrollCursorVisible();

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
string cTextEdit::getText (sPosition beginPosition, sPosition endPosition) {
// get psotion range as string with lineFeed line breaks

  uint32_t beginLineNumber = beginPosition.mLineNumber;
  uint32_t endLineNumber = endPosition.mLineNumber;

  // count approx numChars
  uint32_t numChars = 0;
  for (uint32_t lineNumber = beginLineNumber; lineNumber < endLineNumber; lineNumber++)
    numChars += getNumGlyphs (lineNumber);

  // reserve text size
  string text;
  text.reserve (numChars + (numChars / 8));

  uint32_t beginCharacterIndex = getCharacterIndex (beginPosition);
  uint32_t endCharacterIndex = getCharacterIndex (endPosition);
  while ((beginCharacterIndex < endCharacterIndex) || (beginLineNumber < endLineNumber)) {
    if (beginLineNumber >= getNumLines())
      break;

    if (beginCharacterIndex < getGlyphs (beginLineNumber).size()) {
      text += getGlyphs (beginLineNumber)[beginCharacterIndex].mChar;
      beginCharacterIndex++;
      }
    else {
      beginLineNumber++;
      beginCharacterIndex = 0;
      text += '\n';
      }
    }

  return text;
  }
//}}}
//{{{
float cTextEdit::getTextWidth (sPosition position) {
// get width of text in pixels, of position lineNumber, up to position column

  const auto& glyphs = getGlyphs (position.mLineNumber);

  float distance = 0.f;
  uint32_t characterIndex = getCharacterIndex (position);
  for (uint32_t i = 0; (i < glyphs.size()) && (i < characterIndex);) {
    if (glyphs[i].mChar == '\t') {
      // tab
      distance = getTabEndPosX (distance);
      i++;
      }

    else {
      array <char,7> str;
      uint32_t j = 0;
      uint32_t length = utf8CharLength (glyphs[i].mChar);
      for (; (j < 6) && (length > 0) && (i < glyphs.size()); j++, i++) {
        str[j] = glyphs[i].mChar;
        length--;
        }

      distance += mContext.measureText (str.data(), str.data() + j);
      }
    }

  return distance;
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

  uint32_t index = 0;
  uint32_t column = 0;
  const auto& glyphs = getGlyphs (position.mLineNumber);
  for (; (index < glyphs.size()) && (column < position.mColumn);) {
    if (glyphs[index].mChar == '\t')
      column = getTabColumn (column);
    else
      column++;
    index += utf8CharLength (glyphs[index].mChar);
    }

  return index;
  }
//}}}
//{{{
uint32_t cTextEdit::getCharacterColumn (uint32_t lineNumber, uint32_t characterIndex) {
// handle tabs

  uint32_t column = 0;
  uint32_t glyphIndex = 0;
  const auto& glyphs = getGlyphs (lineNumber);
  while ((glyphIndex < characterIndex) && (glyphIndex < glyphs.size())) {
    uint8_t ch = glyphs[glyphIndex].mChar;
    glyphIndex += utf8CharLength (ch);
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

  uint32_t numChars = 0;
  const auto& glyphs = getGlyphs (lineNumber);
  for (uint32_t glyphIndex = 0; glyphIndex < glyphs.size(); numChars++)
    glyphIndex += utf8CharLength (glyphs[glyphIndex].mChar);

  return numChars;
  }
//}}}
//{{{
uint32_t cTextEdit::getLineMaxColumn (uint32_t lineNumber) {

  uint32_t column = 0;
  const auto& glyphs = getGlyphs (lineNumber);
  for (uint32_t glyphIndex = 0; glyphIndex < glyphs.size(); ) {
    uint8_t ch = glyphs[glyphIndex].mChar;
    if (ch == '\t')
      column = getTabColumn (column);
    else
      column++;
    glyphIndex += utf8CharLength (ch);
    }

  return column;
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::getPositionFromPosX (uint32_t lineNumber, float posX) {

  float columnX = 0.f;

  uint32_t column = 0;
  uint32_t glyphIndex = 0;
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
      uint32_t strIndex = 0;
      uint32_t length = utf8CharLength (glyphs[glyphIndex].mChar);
      while ((strIndex < str.max_size()-1) && (length > 0)) {
        str[strIndex++] = glyphs[glyphIndex++].mChar;
        length--;
        }

      columnWidth = mContext.measureText (str.data(), str.data() + strIndex);
      if (columnX + (columnWidth/2.f) > posX)
        break;

      columnX += columnWidth;
      column++;
      }
    }

  return sanitizePosition ({lineNumber, column});
  }
//}}}

// word
//{{{
bool cTextEdit::isOnWordBoundary (sPosition position) {

  if (position.mColumn == 0)
    return true;

  const auto& glyphs = getGlyphs (position.mLineNumber);
  uint32_t characterIndex = getCharacterIndex (position);
  if (characterIndex >= glyphs.size())
    return true;

  // rely on toekn color to mark word boundaries
  return glyphs[characterIndex].mColor != glyphs[characterIndex-1].mColor;
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
// set cursorPosition, if changed check scrollVisible

  if (position != mEdit.mCursor.mPosition) {
    mEdit.mCursor.mPosition = position;
    mEdit.mScrollVisible = true;
    }
  }
//}}}
//{{{
void cTextEdit::setSelect (eSelect select, sPosition beginPosition, sPosition endPosition) {

  mEdit.mCursor.mSelect = select;
  mEdit.mCursor.mSelectBegin = sanitizePosition (beginPosition);
  mEdit.mCursor.mSelectEnd = sanitizePosition (endPosition);

  if (mEdit.mCursor.mSelectBegin > mEdit.mCursor.mSelectEnd)
    swap (mEdit.mCursor.mSelectBegin, mEdit.mCursor.mSelectEnd);

  switch (select) {
    case eSelect::eNormal:
      break;

    case eSelect::eWord: {
      // set word columns
      mEdit.mCursor.mSelectBegin = findWordBegin (mEdit.mCursor.mSelectBegin);
      if (!isOnWordBoundary (mEdit.mCursor.mSelectEnd))
        mEdit.mCursor.mSelectEnd = findWordEnd (findWordBegin (mEdit.mCursor.mSelectEnd));
      break;
      }

    case eSelect::eLine: {
      // set whole line columns
      mEdit.mCursor.mSelectBegin.mColumn = 0;
      mEdit.mCursor.mSelectEnd.mColumn = getLineMaxColumn (mEdit.mCursor.mSelectEnd.mLineNumber);
      break;
      }
    }
  }
//}}}
//{{{
void cTextEdit::setDeselect() {

  mEdit.mCursor.mSelect = eSelect::eNormal;
  mEdit.mCursor.mSelectBegin = mEdit.mCursor.mPosition;
  mEdit.mCursor.mSelectEnd = mEdit.mCursor.mPosition;
  }
//}}}
//}}}
//{{{  move
//{{{
void cTextEdit::moveUp (uint32_t amount) {

  sPosition position = mEdit.mCursor.mPosition;

  if (position.mLineNumber == 0)
    return;

  // unsigned arithmetic to decrement lineIndex
  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
  lineIndex = (amount < lineIndex) ? lineIndex - amount : 0;

  setCursorPosition ({getLineNumberFromIndex (lineIndex), position.mColumn});
  setDeselect();
  }
//}}}
//{{{
void cTextEdit::moveDown (uint32_t amount) {

  sPosition position = mEdit.mCursor.mPosition;

  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
  lineIndex = min (getMaxLineIndex(), lineIndex + amount);

  setCursorPosition ({getLineNumberFromIndex (lineIndex), position.mColumn});
  setDeselect();
  }
//}}}

//{{{
void cTextEdit::scrollCursorVisible() {

  ImGui::SetWindowFocus();

  sPosition position = getCursorPosition();

  // up,down scroll
  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
  uint32_t minIndex =
    min (getMaxLineIndex(), static_cast<uint32_t>(floor ((ImGui::GetScrollY() + ImGui::GetWindowHeight()) / mContext.mLineHeight)));
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

  mEdit.mScrollVisible = false;
  }
//}}}
//{{{
void cTextEdit::advance (sPosition& position) {

  if (position.mLineNumber < getNumLines()) {
    uint32_t characterIndex = getCharacterIndex (position);
    const auto& glyphs = getGlyphs (position.mLineNumber);
    if (characterIndex + 1 < glyphs.size()) {
      uint32_t delta = utf8CharLength (glyphs[characterIndex].mChar);
      characterIndex = min (characterIndex + delta, static_cast<uint32_t>(glyphs.size())-1);
      }
    else {
      position.mLineNumber++;
      characterIndex = 0;
      }

    position.mColumn = getCharacterColumn (position.mLineNumber, characterIndex);
    }
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

//{{{
cTextEdit::sPosition cTextEdit::findWordBegin (sPosition position) {

  const auto& glyphs = getGlyphs (position.mLineNumber);
  uint32_t characterIndex = getCharacterIndex (position);
  if (characterIndex >= glyphs.size())
    return position;

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
      if (color != glyphs[characterIndex-1].mColor)
        break;
      }
    --characterIndex;
    }

  return sPosition (position.mLineNumber, getCharacterColumn (position.mLineNumber, characterIndex));
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::findWordEnd (sPosition position) {

  const auto& glyphs = getGlyphs (position.mLineNumber);
  uint32_t characterIndex = getCharacterIndex (position);
  if (characterIndex >= glyphs.size())
    return position;

  bool prevSpace = (bool)isspace (glyphs[characterIndex].mChar);
  uint8_t color = glyphs[characterIndex].mColor;
  while (characterIndex < glyphs.size()) {
    uint8_t ch = glyphs[characterIndex].mChar;
    uint32_t length = utf8CharLength (ch);
    if (color != glyphs[characterIndex].mColor)
      break;

    if (prevSpace != !!isspace (ch)) {
      if (isspace (ch))
        while ((characterIndex < glyphs.size()) && isspace (glyphs[characterIndex].mChar))
          characterIndex++;
      break;
      }
    characterIndex += length;
    }

  return sPosition (position.mLineNumber, getCharacterColumn (position.mLineNumber, characterIndex));
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::findNextWord (sPosition position) {

  // skip position to the next non-word character
  bool skip = false;
  bool isWord = false;

  uint32_t characterIndex = getCharacterIndex (position);
  if (characterIndex < getNumGlyphs (position.mLineNumber)) {
    const auto& glyphs = getGlyphs (position.mLineNumber);
    isWord = isalnum (glyphs[characterIndex].mChar);
    skip = isWord;
    }

  while (!isWord || skip) {
    const auto& glyphs = getGlyphs (position.mLineNumber);
    if (characterIndex < glyphs.size()) {
      isWord = isalnum (glyphs[characterIndex].mChar);
      if (isWord && !skip)
        return sPosition (position.mLineNumber, getCharacterColumn (position.mLineNumber, characterIndex));
      if (!isWord)
        skip = false;
      characterIndex++;
      }

    else {
      // start of next line
      characterIndex = 0;
      uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
      if (lineIndex + 1 < getNumLines())
        return {getLineNumberFromIndex (lineIndex + 1), 0};
      else
        return {getNumLines() - 1, getLineMaxColumn (getNumLines() - 1)};
      skip = false;
      isWord = false;
      }
    }

  return position;
  }
//}}}
//}}}
//{{{  insert
//{{{
cTextEdit::cLine& cTextEdit::insertLine (uint32_t index) {
  return *mDoc.mLines.insert (mDoc.mLines.begin() + index, cLine::tGlyphs());
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::insertTextAt (sPosition position, const string& text) {

  uint32_t characterIndex = getCharacterIndex (position);
  const char* textPtr = text.c_str();
  while (*textPtr != '\0') {
    if (*textPtr == '\r') // skip
      textPtr++;
    else if (*textPtr == '\n') {
      // insert new line
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
      textPtr++;
      }

    else {
      // within line
      cLine& line = getLine (position.mLineNumber);
      auto length = utf8CharLength (*textPtr);
      while ((length > 0) && (*textPtr != '\0')) {
        line.mGlyphs.insert (line.mGlyphs.begin() + characterIndex++, cGlyph (*textPtr++, eText));
        length--;
        }
      position.mColumn++;
      parseLine (line);
      }

    mDoc.mEdited = true;
    }

  return position;
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::insertText (const string& text) {

  sPosition position = getCursorPosition();
  if (!text.empty()) {
    position = insertTextAt (position, text);
    setCursorPosition (position);
    }

  return position;
  }
//}}}
//}}}
//{{{  delete
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
//{{{
void cTextEdit::deleteSelect() {

  deleteRange (mEdit.mCursor.mSelectBegin, mEdit.mCursor.mSelectEnd);
  setCursorPosition (mEdit.mCursor.mSelectBegin);
  setDeselect();
  }
//}}}
//}}}
//{{{  parse
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
      for (const auto& p : mOptions.mLanguage.mRegexList) {
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

  // create glyphs string
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

//{{{
uint32_t cTextEdit::trimTrailingSpace() {
// trim trailing space
// - return highest nonEmpty lineNumber

  uint32_t nonEmptyWaterMark = 0;

  uint32_t lineNumber = 0;
  uint32_t trimmedSpaces = 0;
  for (auto& line : mDoc.mLines) {
    size_t column = line.mGlyphs.size();
    while ((column > 0) && (line.mGlyphs[--column].mChar == ' ')) {
      // trailingSpace, trim it
      line.mGlyphs.pop_back();
      trimmedSpaces++;
      //cLog::log (LOGINFO, fmt::format ("trim space {}:{}", lineNumber, column));
      }

    if (!line.mGlyphs.empty()) // nonEmpty line, raise waterMark
      nonEmptyWaterMark = lineNumber;

    lineNumber++;
    }

  cLog::log (LOGINFO, fmt::format ("highest {}:{} trimmedSpaces:{}",
                                   nonEmptyWaterMark+1, mDoc.mLines.size(), trimmedSpaces));
  return nonEmptyWaterMark;
  }
//}}}
//}}}

// fold
//{{{
void cTextEdit::openFold (uint32_t lineNumber) {

  if (isFolded()) {
    // position cursor to lineNumber, first column
    mEdit.mCursor.mPosition = {lineNumber,0};
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
      getLine (lineNumber).mFoldOpen = false;
      }

    else {
      // search back for this fold's foldBegin and close it
      // - skip foldEnd foldBegin pairs
      uint32_t skipFoldPairs = 0;
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
uint32_t cTextEdit::skipFold (uint32_t lineNumber) {
// recursively skip fold lines until matching foldEnd

  while (lineNumber < getNumLines())
    if (getLine (lineNumber).mFoldBegin)
      lineNumber = skipFold (lineNumber+1);
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
  mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
  setSelect (selectWord ? eSelect::eWord : eSelect::eNormal, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
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
  setSelect (eSelect::eNormal, mEdit.mDragFirstPosition, mEdit.mCursor.mPosition);
  mEdit.mScrollVisible = true;
  }
//}}}
//{{{
void cTextEdit::selectLine (uint32_t lineNumber) {

  mEdit.mCursor.mPosition = {lineNumber, 0};
  mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
  setSelect (eSelect::eLine, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
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
    uint32_t lineIndex = max (0u, min (getNumFoldLines()-1, getLineIndexFromNumber (lineNumber) + numDragLines));
    lineNumber = mDoc.mFoldLines[lineIndex];
    }
  else // simple add to lineNumber
    lineNumber = max (0u, min (getNumLines()-1, lineNumber + numDragLines));

  mEdit.mCursor.mPosition.mLineNumber = lineNumber;
  setSelect (eSelect::eLine, mEdit.mDragFirstPosition, mEdit.mCursor.mPosition);
  mEdit.mScrollVisible = true;
  }
//}}}

// draw
//{{{
float cTextEdit::drawGlyphs (ImVec2 pos, const cLine::tGlyphs& glyphs, uint8_t firstGlyph, uint8_t forceColor) {

  if (glyphs.empty())
    return 0.f;

  // initial pos to measure textWidth on return
  float firstPosX = pos.x;

  array <char,256> str;
  uint32_t strIndex = 0;
  uint8_t prevColor = (forceColor == eUndefined) ? glyphs[firstGlyph].mColor : forceColor;

  for (uint32_t glyphIndex = firstGlyph; glyphIndex < glyphs.size(); glyphIndex++) {
    uint8_t color = (forceColor == eUndefined) ? glyphs[glyphIndex].mColor : forceColor;
    if ((strIndex > 0) && (strIndex < str.max_size()) &&
        ((color != prevColor) || (glyphs[glyphIndex].mChar == '\t') || (glyphs[glyphIndex].mChar == ' '))) {
      // draw colored glyphs, seperated by colorChange,tab,space
      pos.x += mContext.drawText (pos, prevColor, str.data(), str.data() + strIndex);
      strIndex = 0;
      }

    if (glyphs[glyphIndex].mChar == '\t') {
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
    else if (glyphs[glyphIndex].mChar == ' ') {
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
      uint32_t length = utf8CharLength (glyphs[glyphIndex].mChar);
      while ((length > 0) && (strIndex < str.max_size())) {
        str[strIndex++] = glyphs[glyphIndex].mChar;
        length--;
        }
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
    if (ImGui::IsItemActive()) // closeFold at foldEnd, action on press, button is remove while still pressed
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
  if (mEdit.mCursor.mSelectBegin <= lineEndPosition)
    xBegin = (mEdit.mCursor.mSelectBegin > lineBeginPosition) ? getTextWidth (mEdit.mCursor.mSelectBegin) : 0.f;

  float xEnd = -1.f;
  if (mEdit.mCursor.mSelectEnd > lineBeginPosition)
    xEnd = getTextWidth ((mEdit.mCursor.mSelectEnd < lineEndPosition) ? mEdit.mCursor.mSelectEnd : lineEndPosition);

  if (mEdit.mCursor.mSelectEnd.mLineNumber > lineNumber)
    xEnd += mContext.mGlyphWidth;

  if ((xBegin != -1) && (xEnd != -1) && (xBegin < xEnd))
    mContext.drawRect ({textPos.x + xBegin, textPos.y}, {textPos.x + xEnd, textPos.y + mContext.mLineHeight}, eSelectHighlight);
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
        lineNumber = skipFold (lineNumber);
        }
      else {
        // draw closedFold without comment line
        // - set mFoldCommentLineNumber to first non comment line, !!!! just use +1 for now !!!!
        line.mFoldCommentLineNumber = lineNumber+1;
        line.mFirstGlyph = getLine (line.mFoldCommentLineNumber).mIndent;
        line.mFirstColumn = static_cast<uint8_t>(line.mIndent + mOptions.mLanguage.mFoldBeginClosed.size());
        drawLine (lineNumber++, lineIndex++);

        // skip closed fold
        lineNumber = skipFold (lineNumber+1);
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
     {false, false, false, ImGuiKey_Enter,      true,  [this]{enterKey();}},
     {false, false, false, ImGuiKey_Tab,        true,  [this]{tabKey();}},
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

  ImGui::GetIO().WantTextInput = true;
  ImGui::GetIO().WantCaptureKeyboard = false;

  bool altKeyPressed = ImGui::GetIO().KeyAlt;
  bool ctrlKeyPressed = ImGui::GetIO().KeyCtrl;
  bool shiftKeyPressed = ImGui::GetIO().KeyShift;
  for (const auto& actionKey : kActionKeys)
    //{{{  dispatch actionKey
    if ((((actionKey.mGuiKey < 0x100) && ImGui::IsKeyPressed (ImGui::GetKeyIndex (actionKey.mGuiKey))) ||
         ((actionKey.mGuiKey >= 0x100) && ImGui::IsKeyPressed (actionKey.mGuiKey))) &&
        (actionKey.mAlt == altKeyPressed) &&
        (actionKey.mCtrl == ctrlKeyPressed) &&
        (actionKey.mShift == shiftKeyPressed) &&
        (!actionKey.mWritable || (actionKey.mWritable && !isReadOnly()))) {

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
  mLineHeight = ImGui::GetTextLineHeight() * scale;
  mGlyphWidth = mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, " ").x;

  mLeftPad = mGlyphWidth / 2.f;
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

    for (const auto& keyWord : kKeyWords)
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

    for (const auto& keyWord : kHlslKeyWords)
      language.mKeyWords.insert (keyWord);
    for (const auto& knownWord : kHlslKnownWords)
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

    for (const auto& keyWord : kGlslKeyWords)
      language.mKeyWords.insert (keyWord);
    for (const auto& knownWord : kGlslKnownWords)
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
