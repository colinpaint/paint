// cTextEdit.cpp - folding text editor
//{{{  includes
#include "cTextEdit.h"

#include <cmath>
#include <algorithm>
#include <functional>

#include <fstream>
#include <filesystem>

#include <array>

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

  constexpr uint8_t eCursorPos     =    11;
  constexpr uint8_t eCursorLineFill =   12;
  constexpr uint8_t eCursorLineEdge =   13;
  constexpr uint8_t eCursorReadOnly = 14;
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

  // fast parsers
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

// cTextEdit
//{{{
cTextEdit::cTextEdit() {

  mOptions.mLanguage = cLanguage::c();
  mDoc.mLines.push_back (cLine());

  // push any clipboardText to pasteStack
  const char* clipText = ImGui::GetClipboardText();
  if (clipText && (strlen (clipText)) > 0)
    mEdit.pushPasteText (clipText);

  cursorFlashOn();
  }
//}}}

//{{{  gets
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
    string lineString = line.getString();
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


  setDeselect();
  sPosition position = mEdit.mCursor.mPosition;

  // line
  uint32_t lineNumber = position.mLineNumber;
  uint32_t glyphsLineNumber = getGlyphsLineNumber (lineNumber);
  cLine& glyphsLine = mDoc.mLines[glyphsLineNumber];

  // column
  uint32_t glyphIndex = getGlyphIndexFromPosition (position);
  if (glyphIndex > glyphsLine.mFirstGlyph)
    // move to prevColumn on sameLine
    setCursorPosition ({lineNumber, getColumnFromGlyphIndex (glyphsLineNumber, glyphIndex - 1)});
  else if (position.mLineNumber > 0) {
    // at begining of line, move to end of prevLine
    uint32_t moveLineIndex = getLineIndexFromNumber (lineNumber) - 1;
    uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
    uint32_t moveGlyphsLineNumber = getGlyphsLineNumber (moveLineNumber);
    //cLine& moveGlyphsLine = mDoc.mLines[moveGlyphsLineNumber];
    setCursorPosition ({moveLineNumber,
                        getColumnFromGlyphIndex (moveGlyphsLineNumber, getNumGlyphs (moveGlyphsLineNumber))});
    }
  }
//}}}
//{{{
void cTextEdit::moveRight() {

  setDeselect();
  sPosition position = mEdit.mCursor.mPosition;

  // line
  uint32_t lineNumber = position.mLineNumber;
  uint32_t glyphsLineNumber = getGlyphsLineNumber (lineNumber);
  cLine& glyphsLine = mDoc.mLines[glyphsLineNumber];

  // column
  uint32_t glyphIndex = getGlyphIndexFromPosition (position);
  if (glyphIndex < glyphsLine.getNumGlyphs())
    // move to nextColumm on sameLine
    setCursorPosition ({lineNumber, getColumnFromGlyphIndex (lineNumber, glyphIndex+1)});
  else {
    // move to start of nextLine
    uint32_t moveLineIndex = getLineIndexFromNumber (lineNumber) + 1;
    if (moveLineIndex < getNumFoldLines()) {
      uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
      uint32_t moveGlyphsLineNumber = getGlyphsLineNumber (glyphsLineNumber);
      setCursorPosition ({moveLineNumber, mDoc.mLines[moveGlyphsLineNumber].mFirstGlyph});
      }
    }

  }
//}}}
//{{{
void cTextEdit::moveRightWord() {

  setDeselect();
  sPosition position = mEdit.mCursor.mPosition;
  uint32_t lineNumber = position.mLineNumber;

  // line
  uint32_t glyphsLineNumber = getGlyphsLineNumber (lineNumber);
  cLine& glyphsLine = mDoc.mLines[glyphsLineNumber];

  // column
  uint32_t glyphIndex = getGlyphIndexFromPosition ({glyphsLineNumber, position.mColumn});
  if (glyphIndex < glyphsLine.getNumGlyphs()) {
    // not at end of line
    sPosition movePosition = findNextWord ({glyphsLineNumber, position.mColumn});
    uint32_t moveGlyphIndex = getGlyphIndexFromPosition (movePosition);
    setCursorPosition ({lineNumber, getColumnFromGlyphIndex (glyphsLineNumber, moveGlyphIndex)});
    }
  // !!!! implement inc to next line !!!!!
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

  if (!canEditAtCursor())
    return;

  if (hasSelect()) {
    // push selectedText to pasteStack
    mEdit.pushPasteText (getSelectText());
    setDeselect();
    }

  else {
    // push currentLine text to pasteStack
    sPosition position {getCursorPosition().mLineNumber, 0};
    sPosition nextLinePosition = getNextLinePosition (position);
    string text = getText (position, nextLinePosition);;
    mEdit.pushPasteText (text);

    // moveLineDown for any multiple copy
    moveLineDown();
    }
  }
//}}}
//{{{
void cTextEdit::cut() {

  if (hasSelect()) {
    // push selectedText to pasteStack
    string text = getSelectText();
    mEdit.pushPasteText (text);

    // cut selected range
    cUndo undo;
    undo.mBefore = mEdit.mCursor;
    undo.mDelete = text;
    undo.mDeleteBegin = mEdit.mCursor.mSelectBegin;
    undo.mDeleteEnd = mEdit.mCursor.mSelectEnd;

    // delete selected text
    deleteSelect();

    undo.mAfter = mEdit.mCursor;
    mEdit.addUndo (undo);
    }

  else {
    // push currentLine text to pasteStack
    sPosition position {getCursorPosition().mLineNumber,0};
    sPosition nextLinePosition = getNextLinePosition (position);
    string text = getText (position, nextLinePosition);
    mEdit.pushPasteText (text);

    cUndo undo;
    undo.mBefore = mEdit.mCursor;
    undo.mDelete = text;
    undo.mDeleteBegin = position;
    undo.mDeleteEnd = nextLinePosition;

    // delete currentLine, handling folds
    deleteLineRange (position.mLineNumber, nextLinePosition.mLineNumber);

    undo.mAfter = mEdit.mCursor;
    mEdit.addUndo (undo);
    }
  }
//}}}
//{{{
void cTextEdit::paste() {

  if (hasPaste()) {
    cUndo undo;
    undo.mBefore = mEdit.mCursor;

    if (hasSelect()) {
      // delete selectedText
      undo.mDelete = getSelectText();
      undo.mDeleteBegin = mEdit.mCursor.mSelectBegin;
      undo.mDeleteEnd = mEdit.mCursor.mSelectEnd;
      deleteSelect();
      }

    string text = mEdit.popPasteText();
    undo.mAdd = text;
    undo.mAddBegin = getCursorPosition();

    insertText (text);

    undo.mAddEnd = getCursorPosition();
    undo.mAfter = mEdit.mCursor;
    mEdit.addUndo (undo);

    // moveLineUp for any multiple paste
    moveLineUp();
    }
  }
//}}}

// delete
//{{{
void cTextEdit::deleteIt() {

  cUndo undo;
  undo.mBefore = mEdit.mCursor;

  if (hasSelect()) {
    undo.mDelete = getSelectText();
    undo.mDeleteBegin = mEdit.mCursor.mSelectBegin;
    undo.mDeleteEnd = mEdit.mCursor.mSelectEnd;
    deleteSelect();
    }

  else {
    sPosition position = getCursorPosition();
    setCursorPosition (position);

    cLine& line = mDoc.mLines[position.mLineNumber];
    if (position.mColumn == getLineNumColumns (position.mLineNumber)) {
      if (position.mLineNumber == getNumLines()-1)
        return;

      undo.mDelete = '\n';
      undo.mDeleteBegin = getCursorPosition();
      undo.mDeleteEnd = advance (undo.mDeleteBegin);

      // insert nextLine at end of line
      line.insertLineAtEnd (mDoc.mLines[position.mLineNumber+1]);
      parseLine (line);

      // delete nextLine
      deleteLine (position.mLineNumber + 1);
      }

    else {
      undo.mDeleteBegin = undo.mDeleteEnd = getCursorPosition();
      undo.mDeleteEnd.mColumn++;
      undo.mDelete = getText (undo.mDeleteBegin, undo.mDeleteEnd);

      uint32_t glyphIndex = getGlyphIndexFromPosition (position);
      line.erase (glyphIndex);
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
    undo.mDelete = getSelectText();
    undo.mDeleteBegin = mEdit.mCursor.mSelectBegin;
    undo.mDeleteEnd = mEdit.mCursor.mSelectEnd;
    deleteSelect();
    }

  else {
    sPosition position = getCursorPosition();
    if (mEdit.mCursor.mPosition.mColumn == 0) {
      if (mEdit.mCursor.mPosition.mLineNumber == 0)
        return;

      undo.mDelete = '\n';
      undo.mDeleteBegin = {position.mLineNumber - 1, getLineNumColumns (position.mLineNumber - 1)};
      undo.mDeleteEnd = advance (undo.mDeleteBegin);

      cLine& line = mDoc.mLines[mEdit.mCursor.mPosition.mLineNumber];
      cLine& prevLine = mDoc.mLines[mEdit.mCursor.mPosition.mLineNumber-1];

      uint32_t prevSize = getLineNumColumns (mEdit.mCursor.mPosition.mLineNumber - 1);

      prevLine.insertLineAtEnd (line);
      deleteLine (mEdit.mCursor.mPosition.mLineNumber);

      --mEdit.mCursor.mPosition.mLineNumber;
      mEdit.mCursor.mPosition.mColumn = prevSize;
      }

    else {
      uint32_t glyphIndex = getGlyphIndexFromPosition (position) - 1;
      uint32_t glyphIndexEnd = glyphIndex + 1;
      cLine& line = mDoc.mLines[mEdit.mCursor.mPosition.mLineNumber];
      glyphIndex--;

      undo.mDeleteBegin = undo.mDeleteEnd = getCursorPosition();
      --undo.mDeleteBegin.mColumn;
      --mEdit.mCursor.mPosition.mColumn;

      while ((glyphIndex < line.getNumGlyphs()) && (glyphIndexEnd-- > glyphIndex)) {
        undo.mDelete += line.getChar (glyphIndex);
        line.erase (glyphIndex);
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

  if (!canEditAtCursor())
    return;

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
      selectEnd.mColumn = getLineNumColumns (selectEnd.mLineNumber);

      undo.mDeleteBegin = selectBegin;
      undo.mDeleteEnd = selectEnd;
      undo.mDelete = getText (selectBegin, selectEnd);

      bool modified = false;
      for (uint32_t lineNumber = selectBegin.mLineNumber; lineNumber <= selectEnd.mLineNumber; lineNumber++) {
        cLine& line = mDoc.mLines[lineNumber];
        line.insert (0, cGlyph ('\t', eTab));
        modified = true;
        }

      if (modified) {
        //{{{  not sure what this does yet
        selectBegin = { selectBegin.mLineNumber, getColumnFromGlyphIndex (selectBegin.mLineNumber, 0)};
        sPosition rangeEnd;
        if (originalEnd.mColumn != 0) {
          selectEnd = {selectEnd.mLineNumber, getLineNumColumns (selectEnd.mLineNumber)};
          rangeEnd = selectEnd;
          undo.mAdd = getText (selectBegin, selectEnd);
          }
        else {
          selectEnd = {originalEnd.mLineNumber, 0};
          rangeEnd = {selectEnd.mLineNumber - 1, getLineNumColumns (selectEnd.mLineNumber - 1)};
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
      undo.mDelete = getSelectText();
      undo.mDeleteBegin = mEdit.mCursor.mSelectBegin;
      undo.mDeleteEnd = mEdit.mCursor.mSelectEnd;
      deleteSelect();
      }
      //}}}
    }

  sPosition position = getCursorPosition();
  undo.mAddBegin = position;

  if (ch == '\n') {
    //{{{  enter lineFeed
    cLine& line = mDoc.mLines[position.mLineNumber];

    // insert newLine
    insertLine (position.mLineNumber+1);
    cLine& newLine = mDoc.mLines[position.mLineNumber+1];

    if (mOptions.mLanguage.mAutoIndentation)
      for (uint32_t glyphIndex = 0;
           (glyphIndex < line.getNumGlyphs()) &&
           isascii (line.getChar (glyphIndex)) && isblank (line.getChar (glyphIndex)); glyphIndex++)
        newLine.pushBack (line.getGlyph (glyphIndex));
    uint32_t indentSize = newLine.getNumGlyphs();

    // insert indent and rest of old line
    uint32_t glyphIndex = getGlyphIndexFromPosition (position);
    newLine.insertRestOfLineAtEnd (line, glyphIndex);
    parseLine (newLine);

    // erase rest of old line
    line.erase (glyphIndex, line.getNumGlyphs());
    parseLine (line);

    // set cursor
    setCursorPosition ({position.mLineNumber+1, getColumnFromGlyphIndex (position.mLineNumber+1, indentSize)});
    undo.mAdd = (char)ch;
    }
    //}}}
  else {
    // enter char
    cLine& line = mDoc.mLines[position.mLineNumber];
    uint32_t glyphIndex = getGlyphIndexFromPosition (position);
    if (mOptions.mOverWrite && (glyphIndex < line.getNumGlyphs())) {
      // overwrite
      undo.mDeleteBegin = mEdit.mCursor.mPosition;
      undo.mDeleteEnd = {position.mLineNumber, getColumnFromGlyphIndex (position.mLineNumber, glyphIndex+1)};
      undo.mDelete += line.getChar (glyphIndex);
      line.erase (glyphIndex);
      }

    cGlyph glyph (ch, eText);
    line.insert (glyphIndex, glyph);
    parseLine (line);

    // undo.mAdd = utf8buf.data(); // utf8 handling needed
    undo.mAdd = static_cast<char>(ch);
    setCursorPosition ({position.mLineNumber, getColumnFromGlyphIndex (position.mLineNumber, glyphIndex+1)});
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

  // !!!! temp string for now !!!!
  string text = mOptions.mLanguage.mFoldBeginToken +
                 "  new fold - loads of detail to implement\n\n" +
                 mOptions.mLanguage.mFoldEndToken +
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

// file
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

  uint32_t utf8chars = 0;

  string lineString;
  uint32_t lineNumber = 0;

  // read filename as stream
  ifstream stream (filename, ifstream::in);
  while (getline (stream, lineString)) {
    // create empty line, reserve enough glyphs for line
    mDoc.mLines.emplace_back (cLine());
    mDoc.mLines.back().reserve (lineString.size());

    // iterate char
    for (auto it = lineString.begin(); it < lineString.end(); ++it) {
      char ch = *it;
      if (ch == '\r') // CR ignored, but set flag
        mDoc.mHasCR = true;
      else {
        if (ch ==  '\t')
          mDoc.mHasTabs = true;

        uint8_t numUtf8Bytes = cGlyph::numUtf8Bytes(ch);
        if (numUtf8Bytes == 1)
          mDoc.mLines.back().emplaceBack (cGlyph (ch, eText));
        else {
          array <uint8_t,7> utf8Bytes = {0};
          string utf8String;
          for (uint32_t i = 0; i < numUtf8Bytes; i++) {
            utf8Bytes[i] = (i == 0) ? ch : *it++;
            utf8String += fmt::format ("{:2x} ", utf8Bytes[i]);
            }
          utf8chars++;
          //cLog::log (LOGINFO, fmt::format ("loading utf8 {} {}", size, utf8String));
          mDoc.mLines.back().emplaceBack (cGlyph (utf8Bytes.data(), numUtf8Bytes, eText));
          mDoc.mHasUtf8 = true;
          }
        }
      }
    parseLine (mDoc.mLines.back());
    lineNumber++;
    }

  trimTrailingSpace();

  // add empty lastLine
  mDoc.mLines.emplace_back (cLine());

  cLog::log (LOGINFO, fmt::format ("read {} lines {}{} utf8 {}",
                                   lineNumber,
                                   mDoc.mHasCR ? "hasCR " : "",
                                   mDoc.mHasUtf8 ? "hasUtf8 " : "",
                                   utf8chars));

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
    string lineString = mDoc.mLines[lineNumber].getString();
    stream.write (lineString.data(), lineString.size());
    stream.put ('\n');
    }

  // done
  cLog::log (LOGINFO,fmt::format ("{} saved", saveFilePath.string()));
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
// main ui io,draw

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
  ImGui::DragInt ("##fs", &mOptions.mFontSize, 0.2f, mOptions.mmDocntSize, mOptions.mMaxFontSize, "%d");

  if (ImGui::IsItemHovered()) {
    int fontSize = mOptions.mFontSize + static_cast<int>(ImGui::GetIO().MouseWheel);
    mOptions.mFontSize = max (mOptions.mmDocntSize, min (mOptions.mMaxFontSize, fontSize));
    }
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

  //{{{  info
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}:{} {}", getCursorPosition().mColumn+1, getCursorPosition().mLineNumber+1,
                                           getNumLines(), getLanguage().mName).c_str());
  //}}}
  //{{{  vertice debug
  ImGui::SameLine();
  ImGui::Text (fmt::format ("{}:{}",
               ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());
  //}}}
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
  ImGui::PushStyleVar (ImGuiStyleVar_ScrollbarSize, mDrawContext.mGlyphWidth*2.f);
  ImGui::PushStyleVar (ImGuiStyleVar_ScrollbarRounding, 2.f);
  ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, {0.f,0.f});
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, {0.f,0.f});
  ImGui::BeginChild ("##s", {0.f,0.f}, false,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);
  mDrawContext.update (mOptions, isDrawMonoSpaced() && !mDoc.mHasUtf8);

  keyboard();

  if (isFolded())
    mDoc.mFoldLines.resize (drawFolded());
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

// private:
//{{{  get
//{{{
bool cTextEdit::canEditAtCursor() {
// cannot edit readOnly, foldBegin token, foldEnd token

  sPosition position = getCursorPosition();

  const cLine& line = mDoc.mLines[position.mLineNumber];

  return !isReadOnly() &&
         !(line.mFoldBegin && (position.mColumn < line.mIndent + mOptions.mLanguage.mFoldBeginToken.size())) &&
         !line.mFoldEnd;
  }
//}}}

// text
//{{{
string cTextEdit::getText (sPosition beginPosition, sPosition endPosition) {
// get position range as string with lineFeed line breaks

  uint32_t beginLineNumber = beginPosition.mLineNumber;
  uint32_t endLineNumber = endPosition.mLineNumber;

  // count approx numChars
  size_t numChars = 0;
  for (uint32_t lineNumber = beginLineNumber; lineNumber < endLineNumber; lineNumber++)
    numChars += getNumGlyphs (lineNumber);

  // reserve text size
  string text;
  text.reserve (numChars);

  uint32_t beginGlyphIndex = getGlyphIndexFromPosition (beginPosition);
  uint32_t endGlyphIndex = getGlyphIndexFromPosition (endPosition);
  while ((beginGlyphIndex < endGlyphIndex) || (beginLineNumber < endLineNumber)) {
    if (beginLineNumber >= getNumLines())
      break;

    if (beginGlyphIndex < mDoc.mLines[beginLineNumber].getNumGlyphs()) {
      text += mDoc.mLines[beginLineNumber].getChar (beginGlyphIndex);
      beginGlyphIndex++;
      }
    else {
      beginLineNumber++;
      beginGlyphIndex = 0;
      text += '\n';
      }
    }

  return text;
  }
//}}}

// text widths
//{{{
float cTextEdit::getWidth (sPosition position) {
// get width in pixels of drawn glyphs up to and including position

  const cLine& line = mDoc.mLines[position.mLineNumber];

  float width = 0.f;
  uint32_t toGlyphIndex = getGlyphIndexFromPosition (position);
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
float cTextEdit::getGlyphWidth (const cLine& line, uint32_t glyphIndex) {

  uint32_t numCharBytes = line.getNumCharBytes(glyphIndex);
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
//{{{
uint32_t cTextEdit::getNumPageLines() const {

  float height = ImGui::GetWindowHeight() - 20.f;
  return static_cast<uint32_t>(floor (height / mDrawContext.mLineHeight));
  }
//}}}

// line
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

  if (!isFolded()) // simple case, lineIndex is lineNumber
    return lineNumber;

  if (mDoc.mFoldLines.empty()) {
    // no lineIndex vector
    cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber {} lineIndex empty", lineNumber));
    return 0;
    }

  // find lineNumber in lineIndex vector
  auto it = find (mDoc.mFoldLines.begin(), mDoc.mFoldLines.end(), lineNumber);
  if (it == mDoc.mFoldLines.end()) {
    // lineNumber notFound, error
    cLog::log (LOGERROR, fmt::format ("getLineIndexFromNumber lineNumber:{} not found", lineNumber));
    return 0xFFFFFFFF;
    }

  // lineNUmber found, return lineIndex
  return uint32_t(it - mDoc.mFoldLines.begin());
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::getNextLinePosition (sPosition position) {

  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
  uint32_t toLineNumber = getLineNumberFromIndex (lineIndex + 1);
  return {toLineNumber, 0};
  }
//}}}

// column
//{{{
uint32_t cTextEdit::getLineNumColumns (uint32_t lineNumber) {

  uint32_t column = 0;

  const cLine& line = mDoc.mLines[lineNumber];

  for (uint32_t glyphIndex = 0; glyphIndex < line.getNumGlyphs(); glyphIndex++)
    if (line.getChar (glyphIndex) == '\t')
      column = getTabColumn (column);
    else
      column++;

  return column;
  }
//}}}
//{{{
uint32_t cTextEdit::getGlyphIndexFromPosition (sPosition position) {
// return glyphIndex from position line,column, using tabs

  uint32_t glyphIndex = 0;

  const cLine& line = mDoc.mLines[position.mLineNumber];

  uint32_t column = 0;
  while ((glyphIndex < line.getNumGlyphs()) && (column < position.mColumn)) {
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
uint32_t cTextEdit::getColumnFromPosX (uint32_t lineNumber, float posX) {
// get position.mColumn for posX, using glyphWidths and tabs

  uint32_t column = 0;

  const cLine& line = mDoc.mLines[lineNumber];

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
      column = getTabColumn (column);
      }

    else {
      float width = getGlyphWidth (line, glyphIndex);
      if (columnPosX + (width/2.f) > posX)
        break;

      columnPosX += width;
      column++;
      }

  cLog::log (LOGINFO, fmt::format ("getColumnFromPosX line:{} posX:{} firstGlyph:{} column:{} -> result:{}",
                                   lineNumber, posX, line.mFirstGlyph, column, line.mFirstGlyph + column));

  return line.mFirstGlyph + column;
  }
//}}}
//{{{
uint32_t cTextEdit::getColumnFromGlyphIndex (uint32_t lineNumber, uint32_t toGlyphIndex) {
// return glyphIndex column using any tabs

  uint32_t column = 0;

  const cLine& line = mDoc.mLines[lineNumber];

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

// tab
//{{{
float cTextEdit::getTabEndPosX (float xPos) {
// return tabEndPosx of tab containing xPos

  float tabWidthPixels = mDoc.mTabSize * mDrawContext.mGlyphWidth;
  return (1.f + floor ((1.f + xPos) / tabWidthPixels)) * tabWidthPixels;
  }
//}}}
//{{{
uint32_t cTextEdit::getTabColumn (uint32_t column) {
// return column of after tab at column
  return ((column / mDoc.mTabSize) * mDoc.mTabSize) + mDoc.mTabSize;
  }
//}}}
//}}}
//{{{  set
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

  mEdit.mCursor.mSelectBegin = sanitizePosition (beginPosition);
  mEdit.mCursor.mSelectEnd = sanitizePosition (endPosition);
  if (mEdit.mCursor.mSelectBegin > mEdit.mCursor.mSelectEnd)
    swap (mEdit.mCursor.mSelectBegin, mEdit.mCursor.mSelectEnd);

  mEdit.mCursor.mSelect = select;
  switch (select) {
    case eSelect::eNormal:
      cLog::log (LOGINFO, fmt::format ("setSelect eNormal - line:{} begin:{} end:{}",
                                       mEdit.mCursor.mSelectBegin.mLineNumber,
                                       mEdit.mCursor.mSelectBegin.mColumn,
                                       mEdit.mCursor.mSelectEnd.mColumn));
      break;

    case eSelect::eWord:
      mEdit.mCursor.mSelectBegin = findWordBegin (mEdit.mCursor.mSelectBegin);
      mEdit.mCursor.mSelectEnd = findWordEnd (mEdit.mCursor.mSelectEnd);
      cLog::log (LOGINFO, fmt::format ("setSelect eWord - line:{} begin:{} end:{}",
                                       mEdit.mCursor.mSelectBegin.mLineNumber,
                                       mEdit.mCursor.mSelectBegin.mColumn,
                                       mEdit.mCursor.mSelectEnd.mColumn));
      break;

    case eSelect::eLine: {
      mEdit.mCursor.mSelectBegin.mColumn = 0;
      mEdit.mCursor.mSelectEnd = mEdit.mCursor.mSelectBegin;

      const cLine& line = mDoc.mLines[mEdit.mCursor.mSelectBegin.mLineNumber];
      if (!isFolded() || !line.mFoldBegin)
        // select line
        mEdit.mCursor.mSelectEnd = {mEdit.mCursor.mSelectEnd.mLineNumber + 1, 0};
      else if (!line.mFoldOpen)
        // select fold
        mEdit.mCursor.mSelectEnd = {skipFold (mEdit.mCursor.mSelectEnd.mLineNumber + 1), 0};
      cLog::log (LOGINFO, fmt::format ("setSelect eLine - line:{} begin:{} end:{}",
                                       mEdit.mCursor.mSelectBegin.mLineNumber,
                                       mEdit.mCursor.mSelectBegin.mColumn,
                                       mEdit.mCursor.mSelectEnd.mColumn));
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

  setDeselect();

  sPosition position = mEdit.mCursor.mPosition;
  uint32_t lineNumber = position.mLineNumber;

  if (!isFolded()) {
    //{{{  unfolded simple moveUp
    lineNumber = (amount < lineNumber) ? lineNumber - amount : 0;
    setCursorPosition ({lineNumber, position.mColumn});
    return;
    }
    //}}}

  // folded moveUp
  //cLine& line = mDoc.mLines[lineNumber];
  uint32_t lineIndex = getLineIndexFromNumber (lineNumber);

  uint32_t moveLineIndex = (amount < lineIndex) ? lineIndex - amount : 0;
  uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
  //cLine& moveLine = mDoc.mLines[moveLineNumber);
  uint32_t moveColumn = position.mColumn;

  setCursorPosition ({moveLineNumber, moveColumn});
  }
//}}}
//{{{
void cTextEdit::moveDown (uint32_t amount) {

  setDeselect();

  sPosition position = mEdit.mCursor.mPosition;
  uint32_t lineNumber = position.mLineNumber;

  if (!isFolded()) {
    //{{{  unfolded simple moveDown
    lineNumber = min (lineNumber + amount, getNumLines()-1);
    setCursorPosition ({lineNumber, position.mColumn});
    return;
    }
    //}}}

  // folded moveDown
  uint32_t lineIndex = getLineIndexFromNumber (lineNumber);

  uint32_t moveLineIndex = min (lineIndex + amount, getNumFoldLines() - 1);
  uint32_t moveLineNumber = getLineNumberFromIndex (moveLineIndex);
  uint32_t moveColumn = position.mColumn;

  setCursorPosition ({moveLineNumber, moveColumn});
  }
//}}}

//{{{
void cTextEdit::cursorFlashOn() {
// set cursor flash cycle to on, helps after any move

  mCursorFlashTimePoint = system_clock::now();
  }
//}}}
//{{{
void cTextEdit::scrollCursorVisible() {

  ImGui::SetWindowFocus();
  cursorFlashOn();

  sPosition position = getCursorPosition();

  // up,down scroll
  uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
  uint32_t minIndex =
    min (getMaxLineIndex(), static_cast<uint32_t>(floor ((ImGui::GetScrollY() + ImGui::GetWindowHeight()) /
                                                          mDrawContext.mLineHeight)));
  if (lineIndex >= minIndex - 3)
    ImGui::SetScrollY (max (0.f, (lineIndex + 3) * mDrawContext.mLineHeight) - ImGui::GetWindowHeight());
  else {
    uint32_t maxIndex = static_cast<uint32_t>(floor (ImGui::GetScrollY() / mDrawContext.mLineHeight));
    if (lineIndex < maxIndex + 2)
      ImGui::SetScrollY (max (0.f, (lineIndex - 2) * mDrawContext.mLineHeight));
    }

  // left,right scroll
  float textWidth = mDrawContext.mLineNumberWidth + getWidth (position);
  uint32_t minWidth = static_cast<uint32_t>(floor (ImGui::GetScrollX()));
  if (textWidth - (3 * mDrawContext.mGlyphWidth) < minWidth)
    ImGui::SetScrollX (max (0.f, textWidth - (3 * mDrawContext.mGlyphWidth)));
  else {
    uint32_t maxWidth = static_cast<uint32_t>(ceil (ImGui::GetScrollX() + ImGui::GetWindowWidth()));
    if (textWidth + (3 * mDrawContext.mGlyphWidth) > maxWidth)
      ImGui::SetScrollX (max (0.f, textWidth + (3 * mDrawContext.mGlyphWidth) - ImGui::GetWindowWidth()));
    }

  // done
  mEdit.mScrollVisible = false;
  }
//}}}

//{{{
cTextEdit::sPosition cTextEdit::advance (sPosition position) {

  if (position.mLineNumber < getNumLines()) {
    uint32_t glyphIndex = getGlyphIndexFromPosition (position);

    const cLine& line = mDoc.mLines[position.mLineNumber];
    if (glyphIndex + 1 < line.getNumGlyphs())
      glyphIndex++;
    else {
      position.mLineNumber++;
      glyphIndex = 0;
      }

    position.mColumn = getColumnFromGlyphIndex (position.mLineNumber, glyphIndex);
    }

  return position;
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::sanitizePosition (sPosition position) {
// return position to be within glyphs

  if (position.mLineNumber >= getNumLines()) {
    // past end, position at end of last line
    cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{} to max:{}",
                                     position.mLineNumber, getNumLines()-1));
    return sPosition (getNumLines()-1, getLineNumColumns (getNumLines()-1));
    }

  // ensure column is in range of position.mLineNumber glyphsLine
  cLine& glyphsLine = mDoc.mLines[getGlyphsLineNumber (position.mLineNumber)];

  if (position.mColumn < glyphsLine.mFirstGlyph) {
    cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{} column:{} to min:{}",
                                     position.mLineNumber, position.mColumn, glyphsLine.mFirstGlyph));
    position.mColumn = glyphsLine.mFirstGlyph;
    }
  else if (position.mColumn > glyphsLine.getNumGlyphs()) {
    cLog::log (LOGINFO, fmt::format ("sanitizePosition - limiting lineNumber:{}c olumn:{} to max:{}",
                                     position.mLineNumber, position.mColumn, glyphsLine.getNumGlyphs()));
    position.mColumn = glyphsLine.getNumGlyphs();
    }

  return position;
  }
//}}}

//{{{
cTextEdit::sPosition cTextEdit::findWordBegin (sPosition position) {

  const cLine& line = mDoc.mLines[position.mLineNumber];

  uint32_t glyphIndex = getGlyphIndexFromPosition (position);
  if (glyphIndex >= line.getNumGlyphs())
    return position;

  while (glyphIndex && isspace (line.getChar (glyphIndex)))
    glyphIndex--;

  uint8_t color = line.getColor (glyphIndex);
  while (glyphIndex > 0) {
    uint8_t ch = line.getChar (glyphIndex);
    if ((ch & 0xC0) != 0x80) {
      // not UTF code sequence 10xxxxxx
      if ((ch <= 32) && isspace (ch)) {
        glyphIndex++;
        break;
        }
      if (color != line.getColor (glyphIndex-1))
        break;
      }
    glyphIndex--;
    }

  return sPosition (position.mLineNumber, getColumnFromGlyphIndex (position.mLineNumber, glyphIndex));
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::findWordEnd (sPosition position) {

  const cLine& line = mDoc.mLines[position.mLineNumber];

  uint32_t glyphIndex = getGlyphIndexFromPosition (position);
  if (glyphIndex >= line.getNumGlyphs())
    return position;

  uint8_t prevColor = line.getColor (glyphIndex);
  bool prevSpace = isspace (line.getChar (glyphIndex));
  while (glyphIndex < line.getNumGlyphs()) {
    uint8_t ch = line.getChar (glyphIndex);
    if (line.getColor (glyphIndex) != prevColor)
      break;
    if (static_cast<bool>(isspace (ch)) != prevSpace)
      break;
    glyphIndex++;
    }

  return sPosition (position.mLineNumber, getColumnFromGlyphIndex (position.mLineNumber, glyphIndex));
  }
//}}}
//{{{
cTextEdit::sPosition cTextEdit::findNextWord (sPosition position) {
// !! unreachable !!
  bool skip = false;
  bool isWord = false;

  uint32_t glyphIndex = getGlyphIndexFromPosition (position);
  if (glyphIndex < getNumGlyphs (position.mLineNumber)) {
    const cLine& line = mDoc.mLines[position.mLineNumber];
    isWord = isalnum (line.getChar (glyphIndex));
    skip = isWord;
    }

  while (!isWord || skip) {
    const cLine& line = mDoc.mLines[position.mLineNumber];
    if (glyphIndex < line.getNumGlyphs()) {
      isWord = isalnum (line.getChar (glyphIndex));
      if (isWord && !skip)
        return sPosition (position.mLineNumber, getColumnFromGlyphIndex (position.mLineNumber, glyphIndex));
      if (!isWord)
        skip = false;
      glyphIndex++;
      }

    else {
      // start of next line
      glyphIndex = 0;
      uint32_t lineIndex = getLineIndexFromNumber (position.mLineNumber);
      if (lineIndex + 1 < getNumLines())
        return {getLineNumberFromIndex (lineIndex + 1), 0};
      else
        return {getNumLines() - 1, getLineNumColumns (getNumLines() - 1)};
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
cLine& cTextEdit::insertLine (uint32_t index) {
  return *mDoc.mLines.insert (mDoc.mLines.begin() + index, cLine());
  }
//}}}

//{{{
cTextEdit::sPosition cTextEdit::insertTextAt (sPosition position, const string& text) {
// !!! needs utf8 handling !!!!

  uint32_t glyphIndex = getGlyphIndexFromPosition (position);
  for (auto it = text.begin(); it < text.end(); ++it) {
    char ch = *it;
    if (ch == '\r') // skip
      break;

    if (ch == '\n') {
      // insert new line
      if (glyphIndex < getNumGlyphs (position.mLineNumber)) {
        cLine& line = mDoc.mLines[position.mLineNumber];
        cLine& newLine = insertLine (position.mLineNumber+1);
        newLine.insertRestOfLineAtEnd (line, glyphIndex);
        parseLine (newLine);

        line.eraseToEnd (glyphIndex);
        parseLine (line);
        }
      else
        insertLine (position.mLineNumber+1);

      position.mLineNumber++;
      position.mColumn = 0;
      glyphIndex = 0;
      }

    else {
      // within line
      cLine& line = mDoc.mLines[position.mLineNumber];
      line.insert (glyphIndex, cGlyph (ch, eText));
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
void cTextEdit::deleteLineRange (uint32_t beginLineNumber, uint32_t endLineNumber) {

  mDoc.mLines.erase (mDoc.mLines.begin() + beginLineNumber, mDoc.mLines.begin() + endLineNumber);
  mEdit.mCheckComments = true;
  mDoc.mEdited = true;
  }
//}}}
//{{{
void cTextEdit::deleteLine (uint32_t lineNumber) {

  mDoc.mLines.erase (mDoc.mLines.begin() + lineNumber);
  mEdit.mCheckComments = true;
  mDoc.mEdited = true;
  }
//}}}

//{{{
void cTextEdit::deletePositionRange (sPosition beginPosition, sPosition endPosition) {

  if (endPosition == beginPosition)
    return;

  uint32_t beginGlyphIndex = getGlyphIndexFromPosition (beginPosition);
  uint32_t endGlyphIndex = getGlyphIndexFromPosition (endPosition);

  if (beginPosition.mLineNumber == endPosition.mLineNumber) {
    // delete in same line
    cLine& line = mDoc.mLines[beginPosition.mLineNumber];
    uint32_t maxColumn = getLineNumColumns (beginPosition.mLineNumber);
    if (endPosition.mColumn >= maxColumn)
      line.eraseToEnd (beginGlyphIndex);
    else
      line.erase (beginGlyphIndex, endGlyphIndex);

    parseLine (line);
    }

  else {
    // delete over multiple lines
    // - delete backend of beginLine
    cLine& beginLine = mDoc.mLines[beginPosition.mLineNumber];
    beginLine.eraseToEnd (beginGlyphIndex);

    // delete frontend of endLine
    cLine& endLine = mDoc.mLines[endPosition.mLineNumber];
    endLine.erase (0, endGlyphIndex);

    // insert remainder of endLine after end of remainder of beginLine
    if (beginPosition.mLineNumber < endPosition.mLineNumber)
      beginLine.insertLineAtEnd (endLine);

    // delete middle whole lines
    if (beginPosition.mLineNumber < endPosition.mLineNumber)
      deleteLineRange (beginPosition.mLineNumber + 1, endPosition.mLineNumber + 1);

    parseLine (beginLine);
    parseLine (endLine);
    }

  mDoc.mEdited = true;
  }
//}}}

//{{{
void cTextEdit::deleteSelect() {

  deletePositionRange (mEdit.mCursor.mSelectBegin, mEdit.mCursor.mSelectEnd);
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
      uint32_t glyphIndex = static_cast<uint32_t>(tokenBegin - strBegin);
      uint32_t glyphIndexEnd = static_cast<uint32_t>(tokenEnd - strBegin);
      while (glyphIndex < glyphIndexEnd)
        line.setColor (glyphIndex++, tokenColor);

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

  //cLog::log (LOGINFO, fmt::format ("parseLine {}", lineNumber));

  // check whole text for comments later
  mEdit.mCheckComments = true;

  line.parseReset();
  if (line.empty())
    return;

  // create glyphs string
  string glyphString = line.getString();

  line.setColor (eText);
  //{{{  find indent
  size_t indentPos = glyphString.find_first_not_of (' ');
  line.mIndent = (indentPos == string::npos) ? 0 : static_cast<uint8_t>(indentPos);
  //}}}
  //{{{  find commentSingle token
  size_t pos = indentPos;

  do {
    pos = glyphString.find (mOptions.mLanguage.mCommentSingle, pos);
    if (pos != string::npos)
      for (size_t i = 0; i < mOptions.mLanguage.mCommentSingle.size(); i++)
        line.setCommentSingle (pos++);
    } while (pos != string::npos);
  //}}}
  //{{{  find commentBegin token
  pos = indentPos;

  do {
    pos = glyphString.find (mOptions.mLanguage.mCommentBegin, pos);
    if (pos != string::npos)
      for (size_t i = 0; i < mOptions.mLanguage.mCommentBegin.size(); i++)
        line.setCommentBegin (pos++);
    } while (pos != string::npos);
  //}}}
  //{{{  find commentEnd token
  pos = indentPos;

  do {
    pos = glyphString.find (mOptions.mLanguage.mCommentEnd, pos);
    if (pos != string::npos)
      for (size_t i = 0; i < mOptions.mLanguage.mCommentEnd.size(); i++)
        line.setCommentEnd (pos++);
    } while (pos != string::npos);
  //}}}
  //{{{  find foldBegin token
  size_t foldBeginPos = glyphString.find (mOptions.mLanguage.mFoldBeginToken, indentPos);
  line.mFoldBegin = (foldBeginPos != string::npos) && (foldBeginPos == indentPos);
  if (line.mFoldBegin)
    mDoc.mHasFolds = true;
  //}}}
  //{{{  find foldEnd token
  size_t foldEndPos = glyphString.find (mOptions.mLanguage.mFoldEndToken, indentPos);
  line.mFoldEnd = (foldEndPos != string::npos) && (foldEndPos == indentPos);
  //}}}

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
    bool inBeginEndComment = false;

    uint32_t glyphIndex = 0;
    uint32_t lineNumber = 0;
    while (lineNumber < getNumLines()) {
      cLine& line = mDoc.mLines[lineNumber];
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
  }
//}}}

//{{{
uint32_t cTextEdit::trimTrailingSpace() {
// trim trailing space
// - return highest nonEmpty lineNumber

  uint32_t nonEmptyHighWaterMark = 0;

  uint32_t lineNumber = 0;
  uint32_t trimmedSpaces = 0;
  for (auto& line : mDoc.mLines) {
    trimmedSpaces += line.trimTrailingSpace();
    if (!line.empty()) // nonEmpty line, raise waterMark
      nonEmptyHighWaterMark = lineNumber;
    lineNumber++;
    }

  if ((nonEmptyHighWaterMark != mDoc.mLines.size()-1) || (trimmedSpaces > 0))
    cLog::log (LOGINFO, fmt::format ("highest {}:{} trimmedSpaces:{}",
                                     nonEmptyHighWaterMark+1, mDoc.mLines.size(), trimmedSpaces));
  return nonEmptyHighWaterMark;
  }
//}}}
//}}}

// fold
//{{{
void cTextEdit::openFold (uint32_t lineNumber) {

  if (isFolded()) {
    cLine& line = mDoc.mLines[lineNumber];
    if (line.mFoldBegin)
      line.mFoldOpen = true;

    // position cursor to lineNumber, first glyph column
    setCursorPosition (sPosition (lineNumber, line.mFirstGlyph));
    }
  }
//}}}
//{{{
void cTextEdit::openFoldOnly (uint32_t lineNumber) {

  if (isFolded()) {
    cLine& line = mDoc.mLines[lineNumber];
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
void cTextEdit::closeFold (uint32_t lineNumber) {

  if (isFolded()) {
    // close fold containing lineNumber
    cLog::log (LOGINFO, fmt::format ("closeFold line:{}", lineNumber));

    cLine& line = mDoc.mLines[lineNumber];
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
        line = mDoc.mLines[--lineNumber];
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
uint32_t cTextEdit::skipFold (uint32_t lineNumber) {
// recursively skip fold lines until matching foldEnd

  while (lineNumber < getNumLines())
    if (mDoc.mLines[lineNumber].mFoldBegin)
      lineNumber = skipFold (lineNumber+1);
    else if (mDoc.mLines[lineNumber].mFoldEnd)
      return lineNumber+1;
    else
      lineNumber++;

  // error if you run off end. begin/end mismatch
  cLog::log (LOGERROR, "skipToFoldEnd - unexpected end reached with mathching fold");
  return lineNumber;
  }
//}}}
//{{{
uint32_t cTextEdit::drawFolded() {

  uint32_t lineIndex = 0;

  uint32_t lineNumber = mEdit.mFoldOnly ? mEdit.mFoldOnlyBeginLineNumber : 0;

  //cLog::log (LOGINFO, fmt::format ("drawFolded {}", lineNumber));

  while (lineNumber < getNumLines()) {
    cLine& line = mDoc.mLines[lineNumber];

    if (line.mFoldBegin) {
      // foldBegin
      line.mFirstGlyph = static_cast<uint8_t>(line.mIndent + mOptions.mLanguage.mFoldBeginToken.size());
      if (line.mFoldOpen)
        // draw foldBegin open fold line
        drawLine (lineNumber++, lineIndex++);
      else {
        // draw foldBegin closed fold line, skip rest
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

// keyboard,mouse
//{{{
void cTextEdit::keyboard() {
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
  //  alt    ctrl   shift  guiKey             writable function
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
//{{{
void cTextEdit::mouseSelectLine (uint32_t lineNumber) {

  cursorFlashOn();

  mEdit.mCursor.mPosition = {lineNumber, 0};
  mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
  setSelect (eSelect::eLine, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
  }
//}}}
//{{{
void cTextEdit::mouseDragSelectLine (uint32_t lineNumber, float posY) {
// if folded this will use previous displays mFoldLine vector
// - we haven't drawn subsequent lines yet
//   - works because dragging does not change vector
// - otherwise we would have to delay it to after the whole draw

  if (!canEditAtCursor())
    return;

  int numDragLines = static_cast<int>((posY - (mDrawContext.mLineHeight/2.f)) / mDrawContext.mLineHeight);

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
//{{{
void cTextEdit::mouseSelectText (bool selectWord, uint32_t lineNumber, ImVec2 pos) {

  cursorFlashOn();
  cLog::log (LOGINFO, fmt::format ("mouseSelectText line:{}", lineNumber));

  mEdit.mCursor.mPosition = {lineNumber, getColumnFromPosX (getGlyphsLineNumber (lineNumber), pos.x)};
  mEdit.mDragFirstPosition = mEdit.mCursor.mPosition;
  setSelect (selectWord ? eSelect::eWord : eSelect::eNormal, mEdit.mCursor.mPosition, mEdit.mCursor.mPosition);
  }
//}}}
//{{{
void cTextEdit::mouseDragSelectText (uint32_t lineNumber, ImVec2 pos) {

  if (!canEditAtCursor())
    return;

  int numDragLines = static_cast<int>((pos.y - (mDrawContext.mLineHeight/2.f)) / mDrawContext.mLineHeight);
  uint32_t toLineNumber = max (0u, min (getNumLines()-1, lineNumber + numDragLines));

  //cLog::log (LOGINFO, fmt::format ("mouseDragSelectText {} {}", lineNumber, numDragLines));

  if (isFolded()) {
    // cannot cross fold
    if (lineNumber < toLineNumber) {
      while (++lineNumber <= toLineNumber)
        if (mDoc.mLines[lineNumber].mFoldBegin || mDoc.mLines[lineNumber].mFoldEnd) {
          cLog::log (LOGINFO, fmt::format ("dragSelectText exit 1"));
          return;
          }
      }
    else if (lineNumber > toLineNumber) {
      while (--lineNumber >= toLineNumber)
        if (mDoc.mLines[lineNumber].mFoldBegin || mDoc.mLines[lineNumber].mFoldEnd) {
          cLog::log (LOGINFO, fmt::format ("dragSelectText exit 2"));
          return;
          }
      }
    }

  // drag select
  mEdit.mCursor.mPosition = sPosition (toLineNumber, getColumnFromPosX (toLineNumber, pos.x));
  setSelect (eSelect::eNormal, mEdit.mDragFirstPosition, mEdit.mCursor.mPosition);
  //mEdit.mScrollVisible = true;
  }
//}}}

// draw
//{{{
float cTextEdit::drawGlyphs (ImVec2 pos, uint32_t lineNumber, uint8_t forceColor) {

  cLine& line = mDoc.mLines[lineNumber];
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
      ImVec2 arrowLeftPos {pos.x + 1.f, pos.y + mDrawContext.mFontSize/2.f};

      // apply tab
      pos.x = getTabEndPosX (pos.x);

      if (isDrawWhiteSpace()) {
        // draw tab arrow
        ImVec2 arrowRightPos {pos.x - 1.f, arrowLeftPos.y};
        mDrawContext.line (arrowLeftPos, arrowRightPos, eTab);

        ImVec2 arrowTopPos {arrowRightPos.x - (mDrawContext.mFontSize/4.f) - 1.f,
                            arrowRightPos.y - (mDrawContext.mFontSize/4.f)};
        mDrawContext.line (arrowRightPos, arrowTopPos, eTab);

        ImVec2 arrowBotPos {arrowRightPos.x - (mDrawContext.mFontSize/4.f) - 1.f,
                            arrowRightPos.y + (mDrawContext.mFontSize/4.f)};
        mDrawContext.line (arrowRightPos, arrowBotPos, eTab);
        }
      }
      //}}}
    else if (line.getChar (glyphIndex) == ' ') {
      //{{{  space
      if (isDrawWhiteSpace()) {
        // draw circle
        ImVec2 centre {pos.x  + mDrawContext.mGlyphWidth/2.f, pos.y + mDrawContext.mFontSize/2.f};
        mDrawContext.circle (centre, 2.f, eWhiteSpace);
        }

      pos.x += mDrawContext.mGlyphWidth;
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
void cTextEdit::drawSelect (ImVec2 pos, uint32_t lineNumber, uint32_t glyphsLineNumber) {

  // posBegin
  ImVec2 posBegin = pos;
  if (sPosition (lineNumber, 0) >= mEdit.mCursor.mSelectBegin)
    // from left edge
    posBegin.x = 0.f;
  else
    // from selectBegin column
    posBegin.x += getWidth ({glyphsLineNumber, mEdit.mCursor.mSelectBegin.mColumn});

  // posEnd
  ImVec2 posEnd = pos;
  if (lineNumber < mEdit.mCursor.mSelectEnd.mLineNumber)
    // to end of line
    posEnd.x += getWidth ({lineNumber, getLineNumColumns (lineNumber)});
  else if (mEdit.mCursor.mSelectEnd.mColumn)
    // to selectEnd column
    posEnd.x += getWidth ({glyphsLineNumber, mEdit.mCursor.mSelectEnd.mColumn});
  else
    // to lefPad + lineNumer
    posEnd.x = mDrawContext.mLeftPad + mDrawContext.mLineNumberWidth;
  posEnd.y += mDrawContext.mLineHeight;

  // draw select rect
  mDrawContext.rect (posBegin, posEnd, eSelectCursor);
  }
//}}}
//{{{
void cTextEdit::drawCursor (ImVec2 pos, uint32_t lineNumber) {

  if (!hasSelect()) {
    // draw cursor highlight on lineNumber, could be allText or wholeLine
    ImVec2 tlPos {0.f, pos.y};
    ImVec2 brPos {pos.x, pos.y + mDrawContext.mLineHeight};
    mDrawContext.rect (tlPos, brPos, eCursorLineFill);
    mDrawContext.rectLine (tlPos, brPos, canEditAtCursor() ? eCursorLineEdge : eCursorReadOnly);
    }

  //  draw flashing cursor
  auto elapsed = system_clock::now() - mCursorFlashTimePoint;
  if (elapsed < 400ms) {
    // draw cursor
    float cursorPosX = getWidth (mEdit.mCursor.mPosition);
    float cursorWidth = 2.f;

    cLine& line = mDoc.mLines[lineNumber];
    uint32_t glyphIndex = getGlyphIndexFromPosition (mEdit.mCursor.mPosition);
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
    ImVec2 brPos {tlPos.x + cursorWidth, pos.y + mDrawContext.mLineHeight};
    mDrawContext.rect (tlPos, brPos, canEditAtCursor() ? eCursorPos : eCursorReadOnly);
    return;
    }

  if (elapsed > 800ms) // reset flashTimer
    cursorFlashOn();
  }
//}}}
//{{{
void cTextEdit::drawLine (uint32_t lineNumber, uint32_t lineIndex) {
// draw Line and return incremented lineIndex

  //cLog::log (LOGINFO, fmt::format ("drawLine {}", lineNumber));

  if (isFolded()) {
    //{{{  update mFoldLines vector
    if (lineIndex >= getNumFoldLines())
      mDoc.mFoldLines.push_back (lineNumber);
    else
      mDoc.mFoldLines[lineIndex] = lineNumber;
    }
    //}}}

  ImVec2 curPos = ImGui::GetCursorScreenPos();

  // lineNumber
  float leftPadWidth = mDrawContext.mLeftPad;
  cLine& line = mDoc.mLines[lineNumber];
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
                            {leftPadWidth + mDrawContext.mLineNumberWidth, mDrawContext.mLineHeight});
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
      float indentWidth = line.mIndent * mDrawContext.mGlyphWidth;
      curPos.x += indentWidth;

      // draw foldPrefix
      float prefixWidth = mDrawContext.text(curPos, eFoldOpen, mOptions.mLanguage.mFoldBeginOpen);

      // add foldPrefix invisibleButton, action on press
      ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                              {leftPadWidth + indentWidth + prefixWidth, mDrawContext.mLineHeight});
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
                              {ImGui::GetWindowWidth(), mDrawContext.mLineHeight});
      if (ImGui::IsItemActive()) {
        if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
          mouseDragSelectText (lineNumber,
                               {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
        else if (ImGui::IsMouseClicked (0))
          mouseSelectText (ImGui::IsMouseDoubleClicked (0), lineNumber,
                           {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
        }
      // draw glyphs
      curPos.x += drawGlyphs (glyphsPos, lineNumber, eUndefined);
      }
      //}}}
    else {
      //{{{  draw foldBegin closed + glyphs
      // add indent
      curPos.x += leftPadWidth;

      // add the indent
      float indentWidth = line.mIndent * mDrawContext.mGlyphWidth;
      curPos.x += indentWidth;

      // draw foldPrefix
      float prefixWidth = mDrawContext.text (curPos, eFoldClosed, mOptions.mLanguage.mFoldBeginClosed);

      // add foldPrefix invisibleButton, indent + prefix wide, action on press
      ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                              {leftPadWidth + indentWidth + prefixWidth, mDrawContext.mLineHeight});
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
                              {ImGui::GetWindowWidth(), mDrawContext.mLineHeight});
      if (ImGui::IsItemActive()) {
        if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
          mouseDragSelectText (lineNumber,
                               {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
        else if (ImGui::IsMouseClicked (0))
          mouseSelectText (ImGui::IsMouseDoubleClicked (0), lineNumber,
                           {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
        }

      uint32_t glyphsLineNumber = getGlyphsLineNumber (lineNumber);
      curPos.x += drawGlyphs (glyphsPos, glyphsLineNumber, eFoldClosed);
      }
      //}}}
    }
  else if (isFolded() && line.mFoldEnd) {
    //{{{  draw foldEnd
    curPos.x += leftPadWidth;

    // add the indent
    float indentWidth = line.mIndent * mDrawContext.mGlyphWidth;
    curPos.x += indentWidth;

    // draw foldPrefix
    float prefixWidth = mDrawContext.text (curPos, eFoldOpen, mOptions.mLanguage.mFoldEnd);
    glyphsPos.x = curPos.x;

    // add foldPrefix invisibleButton, only prefix wide, do not want to pick foldEnd line
    ImGui::InvisibleButton (fmt::format ("##f{}", lineNumber).c_str(),
                            {leftPadWidth + indentWidth + prefixWidth, mDrawContext.mLineHeight});
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
                            {ImGui::GetWindowWidth(), mDrawContext.mLineHeight});
    if (ImGui::IsItemActive()) {
      if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0))
        mouseDragSelectText (lineNumber,
                             {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
      else if (ImGui::IsMouseClicked (0))
        mouseSelectText (ImGui::IsMouseDoubleClicked (0), lineNumber,
                         {ImGui::GetMousePos().x - glyphsPos.x, ImGui::GetMousePos().y - glyphsPos.y});
      }

    // drawGlyphs
    curPos.x += drawGlyphs (glyphsPos, lineNumber, eUndefined);
    }
    //}}}

  uint32_t glyphsLineNumber = getGlyphsLineNumber (lineNumber);

  // select
  if ((lineNumber >= mEdit.mCursor.mSelectBegin.mLineNumber) &&
      (lineNumber <= mEdit.mCursor.mSelectEnd.mLineNumber))
    drawSelect (glyphsPos, lineNumber, glyphsLineNumber);

  // cursor
  if (lineNumber == mEdit.mCursor.mPosition.mLineNumber)
    drawCursor (glyphsPos, glyphsLineNumber);
  }
//}}}
//{{{
void cTextEdit::drawLines() {
//  draw unfolded with clipper

  // clipper begin
  ImGuiListClipper clipper;
  clipper.Begin (getNumLines(), mDrawContext.mLineHeight);
  clipper.Step();

  // clipper iterate
  for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++) {
    // not folded, simple use of line glyphs
    mDoc.mLines[lineNumber].mFirstGlyph = 0;
    drawLine (lineNumber, 0);
    }

  // clipper end
  clipper.Step();
  clipper.End();
  }
//}}}

//{{{  cTextEdit::cDrawContext
//{{{
void cTextEdit::cDrawContext::update (const cOptions& options, bool monoSpaced) {
// update draw context

  mDrawList = ImGui::GetWindowDrawList();

  mFont = ImGui::GetFont();
  mMonoSpaced = monoSpaced;

  mFontAtlasSize = ImGui::GetFontSize();
  mFontSize = static_cast<float>(options.mFontSize);
  mFontSmallSize = mFontSize > ((mFontAtlasSize * 2.f) / 3.f) ? ((mFontAtlasSize * 2.f) / 3.f) : mFontSize;
  mFontSmallOffset = ((mFontSize - mFontSmallSize) * 2.f) / 3.f;

  float scale = mFontSize / mFontAtlasSize;
  mLineHeight = ImGui::GetTextLineHeight() * scale;
  mGlyphWidth = measureSpace();

  mLeftPad = mGlyphWidth / 2.f;
  mLineNumberWidth = 0.f;
  }
//}}}

//{{{
float cTextEdit::cDrawContext::measureSpace() const {
// return width of space

  char str[2] = {' ', 0};
  return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, str+1).x;
  }
//}}}
//{{{
float cTextEdit::cDrawContext::measureChar (char ch) const {
// return width of text

  if (mMonoSpaced)
    return mGlyphWidth;
  else {
    char str[2] = {ch,0};
    return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, str+1).x;
    }
  }
//}}}
//{{{
float cTextEdit::cDrawContext::measureText (const char* str, const char* strEnd) const {
  if (mMonoSpaced)
    return mGlyphWidth * static_cast<uint32_t>(strEnd - str);
  else
    return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
  }
//}}}

//{{{
float cTextEdit::cDrawContext::text (ImVec2 pos, uint8_t color, const string& text) {
 // draw and return width of text

  mDrawList->AddText (mFont, mFontSize, pos, kPalette[color], text.c_str(), nullptr);

  if (mMonoSpaced)
    return text.size() * mGlyphWidth;
  else
    return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, text.c_str(), nullptr).x;
  }
//}}}
//{{{
float cTextEdit::cDrawContext::text (ImVec2 pos, uint8_t color, const char* str, const char* strEnd) {
 // draw and return width of text

  mDrawList->AddText (mFont, mFontSize, pos, kPalette[color], str, strEnd);

  if (mMonoSpaced)
    return measureText (str, strEnd);
  else
    return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
  }
//}}}
//{{{
float cTextEdit::cDrawContext::smallText (ImVec2 pos, uint8_t color, const string& text) {
 // draw and return width of small text

  pos.y += mFontSmallOffset;
  mDrawList->AddText (mFont, mFontSmallSize, pos, kPalette[color], text.c_str(), nullptr);

  return mFont->CalcTextSizeA (mFontSmallSize, FLT_MAX, -1.f, text.c_str(), nullptr).x;
  }
//}}}

//{{{
void cTextEdit::cDrawContext::line (ImVec2 pos1, ImVec2 pos2, uint8_t color) {
  mDrawList->AddLine (pos1, pos2, kPalette[color]);
  }
//}}}
//{{{
void cTextEdit::cDrawContext::circle (ImVec2 centre, float radius, uint8_t color) {
  mDrawList->AddCircleFilled (centre, radius, kPalette[color], 4);
  }
//}}}
//{{{
void cTextEdit::cDrawContext::rect (ImVec2 pos1, ImVec2 pos2, uint8_t color) {
  mDrawList->AddRectFilled (pos1, pos2, kPalette[color]);
  }
//}}}
//{{{
void cTextEdit::cDrawContext::rectLine (ImVec2 pos1, ImVec2 pos2, uint8_t color) {
  mDrawList->AddRect (pos1, pos2, kPalette[color], 1.f);
  }
//}}}
//}}}

// cTextEdit::cLanguage
//{{{
const cTextEdit::cLanguage cTextEdit::cLanguage::c() {

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
const cTextEdit::cLanguage cTextEdit::cLanguage::hlsl() {

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
const cTextEdit::cLanguage cTextEdit::cLanguage::glsl() {

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
