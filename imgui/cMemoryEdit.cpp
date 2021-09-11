//{{{  cMemoryEdit.cpp - hacked version of https://github.com/ocornut/imgui_club
// Get latest version at http://www.github.com/ocornut/imgui_club
//
// Right-click anywhere to access the Options menu!
// You can adjust the keyboard repeat delay/rate in ImGuiIO.
// The code assume a mono-space font for simplicity!
// If you don't use the default font, use ImGui::PushFont()/PopFont() to switch to a mono-space font before calling this.
//
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
//
// Todo/Bugs:
// - This is generally old code, it should work but please don't use this as reference!
// - Arrows are being sent to the InputText() about to disappear which for LeftArrow makes the text cursor appear at position 1 for one frame.
// - Using InputText() is awkward and maybe overkill here, consider implementing something custom.
//}}}
//{{{  includes
#include "cMemoryEdit.h"

#include <stdio.h>  // sprintf, scanf
#include <array>

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
constexpr float kPageOffset = 2.f;  // margin between top/bottom of page and cursor

namespace {
  //{{{  const string
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
void cMemoryEdit::drawWindow (const string& title, uint8_t* memData, size_t memSize, size_t baseAddress) {
// standalone Memory Editor window

  mOpen = true;

  ImGui::Begin (title.c_str(), &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
  drawContents (memData, memSize, baseAddress);
  ImGui::End();
  }
//}}}
//{{{
void cMemoryEdit::drawContents (uint8_t* memData, size_t memSize, size_t baseAddress) {

  // info we are trying to display
  cInfo info (memData, memSize, baseAddress);

  // context of sizes, colour we will display it with
  cContext context;
  setContext (info, context);

  drawTop (info, context);

  // child scroll window begin
  ImGui::BeginChild ("##scrolling", ImVec2(0,0), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
  ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, ImVec2(0,0));
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

  // clipper begin
  ImGuiListClipper clipper;
  clipper.Begin (static_cast<int>((memSize + mOptions.mColumns - 1) / mOptions.mColumns), context.mLineHeight);
  clipper.Step();
  const size_t visibleBeginAddress = clipper.DisplayStart * mOptions.mColumns;
  const size_t visibleEndAddress = clipper.DisplayEnd * mOptions.mColumns;

  mEdit.mDataNext = false;
  if (mOptions.mReadOnly || mEdit.mDataEditingAddress >= memSize)
    mEdit.mDataEditingAddress = kUndefinedAddress;
  if (mEdit.mDataAddress >= memSize)
    mEdit.mDataAddress = kUndefinedAddress;

  size_t prevDataEditingAddress = mEdit.mDataEditingAddress;
  mEdit.mDataEditingAddressNext = kUndefinedAddress;
  if (isValidAddress (mEdit.mDataEditingAddress)) {
    //{{{  move cursor
    // move cursor but only apply on next frame so scrolling with be synchronized
    // - because currently we can't change the scrolling while the window is being rendered)
    if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_UpArrow)) && mEdit.mDataEditingAddress >= (size_t)mOptions.mColumns) {
      mEdit.mDataEditingAddressNext = mEdit.mDataEditingAddress - mOptions.mColumns;
      mEdit.mDataEditingTakeFocus = true;
      }

    else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_DownArrow)) && mEdit.mDataEditingAddress < memSize - mOptions.mColumns) {
      mEdit.mDataEditingAddressNext = mEdit.mDataEditingAddress + mOptions.mColumns;
      mEdit.mDataEditingTakeFocus = true;
      }

    else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_LeftArrow)) && mEdit.mDataEditingAddress > 0) {
      mEdit.mDataEditingAddressNext = mEdit.mDataEditingAddress - 1;
      mEdit.mDataEditingTakeFocus = true;
      }

    else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_RightArrow)) && mEdit.mDataEditingAddress < memSize - 1) {
      mEdit.mDataEditingAddressNext = mEdit.mDataEditingAddress + 1;
      mEdit.mDataEditingTakeFocus = true;
      }
    }
    //}}}

  if (isValidAddress (mEdit.mDataEditingAddressNext) &&
      ((mEdit.mDataEditingAddressNext / mOptions.mColumns) != (prevDataEditingAddress / mOptions.mColumns))) {
    //{{{  scroll tracks cursor
    int offset = ((int)(mEdit.mDataEditingAddressNext/ mOptions.mColumns) - (int)(prevDataEditingAddress/ mOptions.mColumns));
    if (((offset < 0) && (mEdit.mDataEditingAddressNext < (visibleBeginAddress + (mOptions.mColumns * kPageOffset)))) ||
        ((offset > 0) && (mEdit.mDataEditingAddressNext > (visibleEndAddress - (mOptions.mColumns * kPageOffset)))))
      ImGui::SetScrollY (ImGui::GetScrollY() + (offset * context.mLineHeight));
    }
    //}}}

  // clipper iterate
  for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++)
    drawLine (lineNumber, info, context);

  // clipper end
  clipper.Step();
  clipper.End();

  // child end
  ImGui::PopStyleVar (2);
  ImGui::EndChild();

  // Notify the main window of our ideal child content size
  ImGui::SetCursorPosX (context.mWindowWidth);
  if (mEdit.mDataNext && mEdit.mDataEditingAddress < memSize) {
    mEdit.mDataEditingAddress++;
    mEdit.mDataAddress = mEdit.mDataEditingAddress;
    mEdit.mDataEditingTakeFocus = true;
    }
  else if (isValidAddress (mEdit.mDataEditingAddressNext)) {
    mEdit.mDataEditingAddress = mEdit.mDataEditingAddressNext;
    mEdit.mDataAddress = mEdit.mDataEditingAddressNext;
    }
  }
//}}}

//{{{
void cMemoryEdit::setAddressHighlight (size_t addressMin, size_t addressMax) {

  mEdit.mGotoAddress = addressMin;

  mEdit.mHighlightMin = addressMin;
  mEdit.mHighlightMax = addressMax;
  }
//}}}

// private:
//{{{
bool cMemoryEdit::isCpuBigEndian() const {
// is cpu bigEndian

  uint16_t x = 1;

  char c[2];
  memcpy (c, &x, 2);

  return c[0] != 0;
  }
//}}}
//{{{
size_t cMemoryEdit::getDataTypeSize (ImGuiDataType dataType) const {
  return kTypeSizes[dataType];
  }
//}}}
//{{{
string cMemoryEdit::getDataTypeDesc (ImGuiDataType dataType) const {
  return kTypeDescs[(size_t)dataType];
  }
//}}}
//{{{
string cMemoryEdit::getDataFormatDesc (cMemoryEdit::eDataFormat dataFormat) const {
  return kFormatDescs[(size_t)dataFormat];
  }
//}}}
//{{{
string cMemoryEdit::getDataStr (size_t address, const cInfo& info, ImGuiDataType dataType, eDataFormat dataFormat) {
// return pointer to mOutBuf

  size_t elemSize = getDataTypeSize (dataType);
  size_t size = ((address + elemSize) > info.mMemSize) ? (info.mMemSize - address) : elemSize;

  uint8_t buf[8];
  memcpy (buf, info.mMemData + address, size);

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

// sets
//{{{
void cMemoryEdit::setContext (const cInfo& info, cContext& context) {

  // size of address box
  context.mAddressDigitsCount = 0;
  if (context.mAddressDigitsCount == 0)
    for (size_t n = info.mBaseAddress + info.mMemSize - 1; n > 0; n >>= 4)
      context.mAddressDigitsCount++;

  // char size
  context.mGlyphWidth = ImGui::CalcTextSize (" ").x; // assume monoSpace font
  context.mLineHeight = ImGui::GetTextLineHeight();
  context.mHexCellWidth = (float)(int)(context.mGlyphWidth * 2.5f);
  context.mExtraSpaceWidth = (float)(int)(context.mHexCellWidth * 0.25f);

  // hex
  context.mHexBeginPos = (context.mAddressDigitsCount + 2) * context.mGlyphWidth;
  context.mHexEndPos = context.mHexBeginPos + (context.mHexCellWidth * mOptions.mColumns);
  context.mAsciiBeginPos = context.mAsciiEndPos = context.mHexEndPos;

  // ascii
  context.mAsciiBeginPos = context.mHexEndPos + context.mGlyphWidth * 1;
  if (mOptions.mColumnExtraSpace > 0)
    context.mAsciiBeginPos += (float)((mOptions.mColumns + mOptions.mColumnExtraSpace - 1) /
      mOptions.mColumnExtraSpace) * context.mExtraSpaceWidth;
  context.mAsciiEndPos = context.mAsciiBeginPos + mOptions.mColumns * context.mGlyphWidth;

  // windowWidth
  ImGuiStyle& style = ImGui::GetStyle();
  context.mWindowWidth = context.mAsciiEndPos + style.ScrollbarSize + style.WindowPadding.x * 2 + context.mGlyphWidth;

  // color
  context.mTextColor = ImGui::GetColorU32 (ImGuiCol_Text);
  context.mGreyColor = ImGui::GetColorU32 (ImGuiCol_TextDisabled);
  context.mHighlightColor = IM_COL32 (255, 255, 255, 50);  // highlight background color
  }
//}}}

// copy
//{{{
void* cMemoryEdit::copyEndian (void* dst, void* src, size_t size) {

  if (!gEndianFunc)
    gEndianFunc = isCpuBigEndian() ? bigEndianFunc : littleEndianFunc;

  return gEndianFunc (dst, src, size, mOptions.mBigEndian ^ mOptions.mHoverEndian);
  }
//}}}

// draw
//{{{
void cMemoryEdit::drawTop (const cInfo& info, const cContext& context) {

  ImGuiStyle& style = ImGui::GetStyle();

  // draw columns button
  ImGui::SetNextItemWidth ((2.f*style.FramePadding.x) + (7.f*context.mGlyphWidth));
  ImGui::DragInt ("##col", &mOptions.mColumns, 0.2f, 2, 64, "%d wide");

  // draw hexII button
  ImGui::SameLine();
  if (toggleButton ("hexII", mOptions.mShowHexII))
    mOptions.mShowHexII = !mOptions.mShowHexII;
  mOptions.mHoverHexII = ImGui::IsItemHovered();

  // draw gotoAddress button
  ImGui::SetNextItemWidth ((2 * style.FramePadding.x) + ((context.mAddressDigitsCount + 1) * context.mGlyphWidth));
  ImGui::SameLine();
  if (ImGui::InputText ("##address", mEdit.mAddressInputBuf, IM_ARRAYSIZE(mEdit.mAddressInputBuf),
                         ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
    //{{{  consume input into gotoAddress
    size_t gotoAddress;
    if (sscanf (mEdit.mAddressInputBuf, "%" _PRISizeT "X", &gotoAddress) == 1) {
      mEdit.mGotoAddress = gotoAddress - info.mBaseAddress;
      mEdit.mHighlightMin = mEdit.mHighlightMax = kUndefinedAddress;
      }
    }
    //}}}

  if (isValidAddress (mEdit.mGotoAddress)) {
    //{{{  valid goto address
    if (mEdit.mGotoAddress < info.mMemSize) {
      // use gotoAddress and scroll to it
      ImGui::BeginChild ("##scrolling");
      ImGui::SetScrollFromPosY (ImGui::GetCursorStartPos().y + (mEdit.mGotoAddress / mOptions.mColumns) * ImGui::GetTextLineHeight());
      ImGui::EndChild();
      mEdit.mDataEditingAddress = mEdit.mGotoAddress;
      mEdit.mDataAddress = mEdit.mGotoAddress;
      mEdit.mDataEditingTakeFocus = true;
      }

    mEdit.mGotoAddress = kUndefinedAddress;
    }
    //}}}

  if (isValidAddress (mEdit.mDataAddress)) {
    // draw dataType combo
    ImGui::SetNextItemWidth ((2*style.FramePadding.x) + (8*context.mGlyphWidth) + style.ItemInnerSpacing.x);
    ImGui::SameLine();
    if (ImGui::BeginCombo ("##combo_type", getDataTypeDesc (mOptions.mPreviewDataType).c_str(), ImGuiComboFlags_HeightLargest)) {
      for (int dataType = 0; dataType < ImGuiDataType_COUNT; dataType++)
        if (ImGui::Selectable (getDataTypeDesc((ImGuiDataType)dataType).c_str(), mOptions.mPreviewDataType == dataType))
          mOptions.mPreviewDataType = (ImGuiDataType)dataType;
      ImGui::EndCombo();
      }

    // draw endian combo
    if ((int)mOptions.mPreviewDataType > 1) {
      ImGui::SameLine();
      if (toggleButton ("big", mOptions.mBigEndian))
        mOptions.mBigEndian = !mOptions.mBigEndian;
      mOptions.mHoverEndian = ImGui::IsItemHovered();
      }

    // draw formats,  !! why can't you inc,iterate an enum
    for (eDataFormat dataFormat = eDataFormat::eDec; dataFormat <= eDataFormat::eHex;
         dataFormat = static_cast<eDataFormat>((static_cast<int>(dataFormat)+1))) {
      ImGui::SameLine();
      ImGui::Text ("%s:%s", getDataFormatDesc (dataFormat).c_str(),
                            getDataStr (mEdit.mDataAddress, info, mOptions.mPreviewDataType, dataFormat).c_str());
      }
    }
  }
//}}}
//{{{
void cMemoryEdit::drawLine (int lineNumber, const cInfo& info, const cContext& context) {

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  // draw address
  size_t address = static_cast<size_t>(lineNumber * mOptions.mColumns);
  ImGui::Text (kFormatAddress, context.mAddressDigitsCount, info.mBaseAddress + address);

  // draw hex
  for (int column = 0; column < mOptions.mColumns && address < info.mMemSize; column++, address++) {
    float bytePosX = context.mHexBeginPos + context.mHexCellWidth * column;
    if (mOptions.mColumnExtraSpace > 0)
      bytePosX += (float)(column / mOptions.mColumnExtraSpace) * context.mExtraSpaceWidth;

    ImGui::SameLine (bytePosX);

    // highlight
    bool isHighlightRange = ((address >= mEdit.mHighlightMin) && (address < mEdit.mHighlightMax));
    bool isHighlightPreview = ((address >= mEdit.mDataAddress) &&
                               (address < (mEdit.mDataAddress + getDataTypeSize (mOptions.mPreviewDataType))));
    if (isHighlightRange || isHighlightPreview) {
      //{{{  draw hex highlight
      ImVec2 pos = ImGui::GetCursorScreenPos();

      float highlightWidth = 2 * context.mGlyphWidth;
      bool isNextByteHighlighted = (address+1 < info.mMemSize) &&
                                   ((isValidAddress (mEdit.mHighlightMax) && (address + 1 < mEdit.mHighlightMax)));

      if (isNextByteHighlighted || ((column + 1) == mOptions.mColumns)) {
        highlightWidth = context.mHexCellWidth;
        if ((mOptions.mColumnExtraSpace > 0) &&
            (column > 0) && ((column + 1) < mOptions.mColumns) &&
            (((column + 1) % mOptions.mColumnExtraSpace) == 0))
          highlightWidth += context.mExtraSpaceWidth;
        }

      draw_list->AddRectFilled (pos, pos + ImVec2(highlightWidth, context.mLineHeight), context.mHighlightColor);
      }
      //}}}
    if (mEdit.mDataEditingAddress == address) {
      //{{{  display text input on current byte
      bool dataWrite = false;

      ImGui::PushID ((void*)address);
      if (mEdit.mDataEditingTakeFocus) {
        ImGui::SetKeyboardFocusHere();
        ImGui::CaptureKeyboardFromApp (true);
        sprintf (mEdit.mAddressInputBuf, kFormatData, context.mAddressDigitsCount, info.mBaseAddress + address);
        sprintf (mEdit.mDataInputBuf, kFormatByte, info.mMemData[address]);
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
            // When not editing a byte, always rewrite its content
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

      sprintf (userData.mCurrentBufOverwrite, kFormatByte, info.mMemData[address]);
      ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal |
                                  ImGuiInputTextFlags_EnterReturnsTrue |
                                  ImGuiInputTextFlags_AutoSelectAll |
                                  ImGuiInputTextFlags_NoHorizontalScroll |
                                  ImGuiInputTextFlags_CallbackAlways;
      flags |= ImGuiInputTextFlags_AlwaysOverwrite;

      ImGui::SetNextItemWidth (context.mGlyphWidth * 2);

      if (ImGui::InputText ("##data", mEdit.mDataInputBuf, IM_ARRAYSIZE(mEdit.mDataInputBuf), flags, sUserData::callback, &userData))
        dataWrite = mEdit.mDataNext = true;
      else if (!mEdit.mDataEditingTakeFocus && !ImGui::IsItemActive())
        mEdit.mDataEditingAddress = mEdit.mDataEditingAddressNext = kUndefinedAddress;

      mEdit.mDataEditingTakeFocus = false;

      if (userData.mCursorPos >= 2)
        dataWrite = mEdit.mDataNext = true;
      if (isValidAddress (mEdit.mDataEditingAddressNext))
        dataWrite = mEdit.mDataNext = false;

      unsigned int dataInputValue = 0;
      if (dataWrite && sscanf(mEdit.mDataInputBuf, "%X", &dataInputValue) == 1)
        info.mMemData[address] = (ImU8)dataInputValue;

      ImGui::PopID();
      }
      //}}}
    else {
      //{{{  display text
      // NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click on.
      uint8_t value = info.mMemData[address];

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
        mEdit.mDataEditingTakeFocus = true;
        mEdit.mDataEditingAddressNext = address;
        }
      }
      //}}}
    }

  if (kShowAscii) {
    //{{{  draw righthand ASCII
    ImGui::SameLine (context.mAsciiBeginPos);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    address = lineNumber * mOptions.mColumns;

    ImGui::PushID (lineNumber);
    if (ImGui::InvisibleButton ("ascii", ImVec2 (context.mAsciiEndPos - context.mAsciiBeginPos, context.mLineHeight))) {
      mEdit.mDataAddress = address + (size_t)((ImGui::GetIO().MousePos.x - pos.x) / context.mGlyphWidth);
      mEdit.mDataEditingAddress = mEdit.mDataAddress;
      mEdit.mDataEditingTakeFocus = true;
      }

    ImGui::PopID();
    for (int column = 0; (column < mOptions.mColumns) && (address < info.mMemSize); column++, address++) {
      if (address == mEdit.mDataEditingAddress) {
        draw_list->AddRectFilled (pos, ImVec2 (pos.x + context.mGlyphWidth, pos.y + context.mLineHeight),
                                 ImGui::GetColorU32 (ImGuiCol_FrameBg));
        draw_list->AddRectFilled (pos, ImVec2 (pos.x + context.mGlyphWidth, pos.y + context.mLineHeight),
                                 ImGui::GetColorU32 (ImGuiCol_TextSelectedBg));
        }

      uint8_t value = info.mMemData[address];
      char displayCh = ((value < 32) || (value >= 128)) ? '.' : value;
      ImU32 color = (!kGreyZeroes || (value == displayCh)) ? context.mTextColor :  context.mGreyColor;
      draw_list->AddText (pos, color, &displayCh, &displayCh + 1);
      pos.x += context.mGlyphWidth;
      }
    }
    //}}}
  }
//}}}

//{{{  undefs, pop warnings
#undef _PRISizeT
#undef ImSnprintf
#ifdef _MSC_VER
  #pragma warning (pop)
#endif
//}}}
