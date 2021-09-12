//{{{  cMemEdit.cpp - hacked version of https://github.com/ocornut/imgui_club
// Usage:
//   // Create a window and draw memory editor inside it:
//   static MemoryEditor mem_edit_1;
//   static char data[0x10000];
//   size_t data_size = 0x10000;
//   mem_edit_1.DrawWindow("Memory Editor", data, data_size);
//
// Usage:
//   // If you already have a window, use DrawContents() instead:
//   static MemoryEditor mem_edit_2;
//   ImGui::Begin("MyWindow")
//   mem_edit_2.DrawContents(this, sizeof(*this), (size_t)this);
//   ImGui::End();
//}}}
//{{{  includes
#include "cMemEdit.h"

#include <stdio.h>  // sprintf, scanf
#include <array>
#include <functional>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../imgui/myImguiWidgets.h"

#include "../utils/cLog.h"

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

using namespace std;
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

// public:
//{{{
cMemEdit::cMemEdit (uint8_t* memData, size_t memSize) : mInfo(memData,memSize) {

  mStartTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
  mLastClick = 0.f;
  }
//}}}

//{{{
void cMemEdit::drawWindow (const string& title, size_t baseAddress) {
// standalone Memory Editor window

  mOpen = true;

  ImGui::Begin (title.c_str(), &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
  drawContents (baseAddress);
  ImGui::End();
  }
//}}}
//{{{
void cMemEdit::drawContents (size_t baseAddress) {

  mInfo.setBaseAddress (baseAddress);
  mContext.init (mOptions, mInfo); // context of sizes,colours
  mEdit.begin (mOptions, mInfo);

  handleKeyboardInputs();
  handleMouseInputs();

  drawTop();

  // child scroll window begin
  ImGui::BeginChild (kChildScrollStr, ImVec2(0,0), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
  ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, ImVec2(0,0));
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

  // clipper begin
  ImGuiListClipper clipper;
  int maxNumLines = static_cast<int>((mInfo.mMemSize + (mOptions.mColumns-1)) / mOptions.mColumns);
  clipper.Begin (maxNumLines, mContext.mLineHeight);
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

//{{{
void cMemEdit::setAddressHighlight (size_t addressMin, size_t addressMax) {

  mEdit.mGotoAddress = addressMin;

  mEdit.mHighlightMin = addressMin;
  mEdit.mHighlightMax = addressMax;
  }
//}}}

//{{{
void cMemEdit::moveLeft() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress > 0) {
      mEdit.mNextEditAddress = mEdit.mEditAddress - 1;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::moveRight() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress < mInfo.mMemSize - 1) {
      mEdit.mNextEditAddress = mEdit.mEditAddress + 1;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::moveLineUp() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress >= (size_t)mOptions.mColumns) {
      mEdit.mNextEditAddress = mEdit.mEditAddress - mOptions.mColumns;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::moveLineDown() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress < mInfo.mMemSize - mOptions.mColumns) {
      mEdit.mNextEditAddress = mEdit.mEditAddress + mOptions.mColumns;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::movePageUp() {

  if (isValid (mEdit.mEditAddress)) {
    int lines = mContext.mNumPageLines;
    while (lines-- > 0)
      moveLineUp();
    }
  }
//}}}
//{{{
void cMemEdit::movePageDown() {

  if (isValid (mEdit.mEditAddress)) {
    int lines = mContext.mNumPageLines;
    while (lines-- > 0)
      moveLineDown();
    }
  }
//}}}
//{{{
void cMemEdit::moveHome() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress > 0) {
      mEdit.mNextEditAddress = 0;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}
//{{{
void cMemEdit::moveEnd() {

  if (isValid (mEdit.mEditAddress)) {
    if (mEdit.mEditAddress < (mInfo.mMemSize - 2)) {
      mEdit.mNextEditAddress = mInfo.mMemSize - 1;
      mEdit.mEditTakeFocus = true;
      }
    }
  }
//}}}

// private:
//{{{
bool cMemEdit::isCpuBigEndian() const {
// is cpu bigEndian

  uint16_t x = 1;

  char c[2];
  memcpy (c, &x, 2);

  return c[0] != 0;
  }
//}}}
//{{{
size_t cMemEdit::getDataTypeSize (ImGuiDataType dataType) const {
  return kTypeSizes[dataType];
  }
//}}}
//{{{
string cMemEdit::getDataTypeDesc (ImGuiDataType dataType) const {
  return kTypeDescs[(size_t)dataType];
  }
//}}}
//{{{
string cMemEdit::getDataFormatDesc (cMemEdit::eDataFormat dataFormat) const {
  return kFormatDescs[(size_t)dataFormat];
  }
//}}}
//{{{
string cMemEdit::getDataStr (size_t address, ImGuiDataType dataType, eDataFormat dataFormat) {
// return pointer to mOutBuf

  size_t elemSize = getDataTypeSize (dataType);
  size_t size = ((address + elemSize) > mInfo.mMemSize) ? (mInfo.mMemSize - address) : elemSize;

  uint8_t buf[8];
  memcpy (buf, mInfo.mMemData + address, size);

  mOutBuf[0] = 0;
  if (dataFormat == eDataFormat::eBin) {
    //{{{  eBin to mOutBuf
    uint8_t binbuf[8];
    copyEndian (binbuf, buf, size);

    int width = (int)size * 8;
    size_t outn = 0;
    int n = width / 8;
    for (int j = n - 1; j >= 0; --j) {
      for (int i = 0; i < 8; ++i)
        mOutBuf[outn++] = (binbuf[j] & (1 << (7 - i))) ? '1' : '0';
      mOutBuf[outn++] = ' ';
      }

    mOutBuf[outn] = 0;
    }
    //}}}
  else {
    // eHex, eDec to mOutBuf
    switch (dataType) {
      //{{{
      case ImGuiDataType_S8: {

        uint8_t int8 = 0;
        copyEndian (&int8, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof(mOutBuf), "%hhd", int8);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "0x%02x", int8 & 0xFF);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U8: {

        uint8_t uint8 = 0;
        copyEndian (&uint8, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%hhu", uint8);
        if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "0x%02x", uint8 & 0XFF);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_S16: {

        int16_t int16 = 0;
        copyEndian (&int16, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%hd", int16);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "0x%04x", int16 & 0xFFFF);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U16: {

        uint16_t uint16 = 0;
        copyEndian (&uint16, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%hu", uint16);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "0x%04x", uint16 & 0xFFFF);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_S32: {

        int32_t int32 = 0;
        copyEndian (&int32, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%d", int32);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "0x%08x", int32);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U32: {
        uint32_t uint32 = 0;
        copyEndian (&uint32, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%u", uint32);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "0x%08x", uint32);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_S64: {

        int64_t int64 = 0;
        copyEndian (&int64, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%lld", (long long)int64);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "0x%016llx", (long long)int64);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U64: {

        uint64_t uint64 = 0;
        copyEndian (&uint64, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%llu", (long long)uint64);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "0x%016llx", (long long)uint64);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_Float: {

        float float32 = 0.f;
        copyEndian (&float32, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%f", float32);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%a", float32);

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_Double: {

        double float64 = 0.0;
        copyEndian (&float64, buf, size);

        if (dataFormat == eDataFormat::eDec)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%f", float64);
        else if (dataFormat == eDataFormat::eHex)
          ImSnprintf (mOutBuf, sizeof (mOutBuf), "%a", float64);
        break;
        }
      //}}}
      }
    }

  return mOutBuf;
  }
//}}}

// copy
//{{{
void* cMemEdit::copyEndian (void* dst, void* src, size_t size) {

  if (!gEndianFunc)
    gEndianFunc = isCpuBigEndian() ? bigEndianFunc : littleEndianFunc;

  return gEndianFunc (dst, src, size, mOptions.mBigEndian ^ mOptions.mHoverEndian);
  }
//}}}

//{{{
void cMemEdit::handleMouseInputs() {

  ImGuiIO& io = ImGui::GetIO();
  bool shift = io.KeyShift;
  bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

  if (ImGui::IsWindowHovered()) {
    if (!ctrl && !shift && !alt) {
      bool leftSingleClick = ImGui::IsMouseClicked (0);
      bool righttSingleClick = ImGui::IsMouseClicked (1);
      bool leftDoubleClick = ImGui::IsMouseDoubleClicked (0);
      bool leftTripleClick = leftSingleClick &&
                             !leftDoubleClick &&
                             ((mLastClick != -1.f) && (ImGui::GetTime() - mLastClick) < io.MouseDoubleClickTime);
      if (leftTripleClick) {
        mLastClick = -1.f;
        }
      else if (leftDoubleClick) {
        mLastClick = static_cast<float>(ImGui::GetTime());
        }
      else if (leftSingleClick) {
        mLastClick = static_cast<float>(ImGui::GetTime());
        }
      else if (ImGui::IsMouseDragging (0) && ImGui::IsMouseDown (0)) {
        io.WantCaptureMouse = true;
        }

      if (righttSingleClick) {
        };
      }
    }
  }
//}}}
//{{{
void cMemEdit::handleKeyboardInputs() {

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
     {false, false, false, ImGuiKey_LeftArrow,  false, [this]{moveLeft();}},
     {false, false, false, ImGuiKey_RightArrow, false, [this]{moveRight();}},
     {false, false, false, ImGuiKey_UpArrow,    false, [this]{moveLineUp();}},
     {false, false, false, ImGuiKey_DownArrow,  false, [this]{moveLineDown();}},
     {false, false, false, ImGuiKey_PageUp,     false, [this]{movePageUp();}},
     {false, false, false, ImGuiKey_PageDown,   false, [this]{movePageDown();}},
     {false, false, false, ImGuiKey_Home,       false, [this]{moveHome();}},
     {false, false, false, ImGuiKey_End,        false, [this]{moveEnd();}},
     };

  //if (!ImGui::IsWindowFocused())
  //  return;
  //if (ImGui::IsWindowHovered())
  //  ImGui::SetMouseCursor (ImGuiMouseCursor_TextInput);

  ImGuiIO& io = ImGui::GetIO();
  bool shift = io.KeyShift;
  bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;
  //io.WantCaptureKeyboard = true;
  //io.WantTextInput = true;

  for (auto& actionKey : kActionKeys) {
    // dispatch matched actionKey
    if ((((actionKey.mGuiKey < 0x100) && ImGui::IsKeyPressed (ImGui::GetKeyIndex (actionKey.mGuiKey))) ||
         ((actionKey.mGuiKey >= 0x100) && ImGui::IsKeyPressed (actionKey.mGuiKey))) &&
        (!actionKey.mWritable || (actionKey.mWritable && !isReadOnly())) &&
        (actionKey.mCtrl == ctrl) &&
        (actionKey.mShift == shift) &&
        (actionKey.mAlt == alt)) {

      actionKey.mActionFunc();
      break;
      }
    }
  }
//}}}

// draw
//{{{
void cMemEdit::drawTop() {

  ImGuiStyle& style = ImGui::GetStyle();

  // draw columns button
  ImGui::SetNextItemWidth ((2.f*style.FramePadding.x) + (7.f*mContext.mGlyphWidth));
  ImGui::DragInt ("##column", &mOptions.mColumns, 0.2f, 2, 64, "%d wide");

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
                            getDataStr (mEdit.mDataAddress, mOptions.mDataType, dataFormat).c_str());
      }
    }
  }
//}}}
//{{{
void cMemEdit::drawLine (int lineNumber) {

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

      draw_list->AddRectFilled (pos, pos + ImVec2(highlightWidth, mContext.mLineHeight), mContext.mHighlightColor);
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
        ImVec2 posEnd = pos + ImVec2 (mContext.mGlyphWidth, mContext.mLineHeight);
        draw_list->AddRectFilled (pos, posEnd, mContext.mFrameBgndColor);
        draw_list->AddRectFilled (pos, posEnd, mContext.mTextSelectBgndColor);
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

// cMemEdit::cContext
//{{{
void cMemEdit::cContext::init (const cOptions& options, const cInfo& info) {

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

  // colors
  mTextColor = ImGui::GetColorU32 (ImGuiCol_Text);
  mGreyColor = ImGui::GetColorU32 (ImGuiCol_TextDisabled);
  mHighlightColor = IM_COL32 (255, 255, 255, 50);  // highlight background color
  mFrameBgndColor = ImGui::GetColorU32 (ImGuiCol_FrameBg);
  mTextSelectBgndColor = ImGui::GetColorU32 (ImGuiCol_TextSelectedBg);
  }
//}}}

//{{{  undefs, pop warnings
#undef _PRISizeT
#undef ImSnprintf
#ifdef _MSC_VER
  #pragma warning (pop)
#endif
//}}}
