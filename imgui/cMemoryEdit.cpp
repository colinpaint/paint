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

#include "../imgui/imgui.h"
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
  //{{{  const
  const char* kFormatDescs[] = {"Bin", "Dec", "Hex"};

  const char* kTypeDescs[] = {"Int8", "Uint8", "Int16", "Uint16", "Int32", "Uint32", "Int64", "Uint64", "Float", "Double"};
  const size_t kTypeSizes[] = {sizeof(int8_t), sizeof(uint8_t),
                               sizeof(int16_t), sizeof(uint16_t),
                               sizeof(int32_t), sizeof (uint32_t),
                               sizeof(int64_t), sizeof(uint64_t),
                               sizeof(float), sizeof(double)};

  const char* kFormatByte = kUpperCaseHex ? "%02X" : "%02x";
  const char* kFormatData = kUpperCaseHex ? "%0*" _PRISizeT "X" : "%0*" _PRISizeT "x";
  const char* kFormatAddress = kUpperCaseHex ? "%0*" _PRISizeT "X: " : "%0*" _PRISizeT "x: ";
  const char* kFormatByteSpace = kUpperCaseHex ? "%02X " : "%02x ";
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

  // save memory address
  mMemData = memData;
  mMemSize = memSize;
  mBaseAddress = baseAddress;
  setContext (mMemSize, mBaseAddress);
  drawHeader (mMemData, mMemSize, mBaseAddress);

  // begin child scroll window
  ImGui::BeginChild ("##scrolling", ImVec2(0,0), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
  ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, ImVec2(0,0));
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

  // clipper begin
  ImGuiListClipper clipper;
  clipper.Begin (static_cast<int>((memSize + mColumns - 1) / mColumns), mContext.mLineHeight);
  clipper.Step();
  const size_t visibleBeginAddr = clipper.DisplayStart * mColumns;
  const size_t visibleEndAddr = clipper.DisplayEnd * mColumns;

  mDataNext = false;
  if (mReadOnly || mDataEditingAddr >= memSize)
    mDataEditingAddr = (size_t)-1;
  if (mDataAddress >= memSize)
    mDataAddress = (size_t)-1;

  size_t prevDataEditingAddr = mDataEditingAddr;
  mDataEditingAddrNext = (size_t)-1;
  if (mDataEditingAddr != (size_t)-1) {
    //{{{  move cursor
    // move cursor but only apply on next frame so scrolling with be synchronized
    // - because currently we can't change the scrolling while the window is being rendered)
    if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_UpArrow)) && mDataEditingAddr >= (size_t)mColumns) {
      mDataEditingAddrNext = mDataEditingAddr - mColumns;
      mDataEditingTakeFocus = true;
      }

    else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_DownArrow)) && mDataEditingAddr < memSize - mColumns) {
      mDataEditingAddrNext = mDataEditingAddr + mColumns;
      mDataEditingTakeFocus = true;
      }

    else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_LeftArrow)) && mDataEditingAddr > 0) {
      mDataEditingAddrNext = mDataEditingAddr - 1;
      mDataEditingTakeFocus = true;
      }

    else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_RightArrow)) && mDataEditingAddr < memSize - 1) {
      mDataEditingAddrNext = mDataEditingAddr + 1;
      mDataEditingTakeFocus = true;
      }
    }
    //}}}

  if ((mDataEditingAddrNext != (size_t)-1) &&
      ((mDataEditingAddrNext / mColumns) != (prevDataEditingAddr / mColumns))) {
    //{{{  scroll tracks cursor
    int offset = ((int)(mDataEditingAddrNext/mColumns) - (int)(prevDataEditingAddr/mColumns));
    if (((offset < 0) && (mDataEditingAddrNext < (visibleBeginAddr + (mColumns * kPageOffset)))) ||
        ((offset > 0) && (mDataEditingAddrNext > (visibleEndAddr - (mColumns * kPageOffset)))))
      ImGui::SetScrollY (ImGui::GetScrollY() + (offset * mContext.mLineHeight));
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
  if (mDataNext && mDataEditingAddr < memSize) {
    mDataEditingAddr++;
    mDataAddress = mDataEditingAddr;
    mDataEditingTakeFocus = true;
    }
  else if (mDataEditingAddrNext != (size_t)-1) {
    mDataEditingAddr = mDataEditingAddrNext;
    mDataAddress = mDataEditingAddrNext;
    }
  }
//}}}

//{{{
void cMemoryEdit::gotoAddrAndHighlight (size_t addrMin, size_t addrMax) {
  mGotoAddr = addrMin;
  mHighlightMin = addrMin;
  mHighlightMax = addrMax;
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
const char* cMemoryEdit::getDataTypeDesc (ImGuiDataType dataType) const {
  return kTypeDescs[(size_t)dataType];
  }
//}}}
//{{{
const char* cMemoryEdit::getDataFormatDesc (cMemoryEdit::eDataFormat dataFormat) const {
  return kFormatDescs[(size_t)dataFormat];
  }
//}}}
//{{{
const char* cMemoryEdit::getDataStr (size_t addr, const uint8_t* memData, size_t memSize,
                                     ImGuiDataType dataType, eDataFormat dataFormat) {
// return pointer to mOutBuf

  size_t elemSize = getDataTypeSize (dataType);
  size_t size = ((addr + elemSize) > memSize) ? (memSize - addr) : elemSize;

  uint8_t buf[8];
  memcpy (buf, memData + addr, size);

  mOutBuf[0] = 0;
  if (dataFormat == eDataFormat::eBin) {
    //{{{  eBin to mOutBuf
    uint8_t binbuf[8];
    endianCopy (binbuf, buf, size);

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
        endianCopy (&int8, buf, size);

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
        endianCopy (&uint8, buf, size);

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
        endianCopy (&int16, buf, size);

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
        endianCopy (&uint16, buf, size);

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
        endianCopy (&int32, buf, size);

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
        endianCopy (&uint32, buf, size);

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
        endianCopy (&int64, buf, size);

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
        endianCopy (&uint64, buf, size);

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
        endianCopy (&float32, buf, size);

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
        endianCopy (&float64, buf, size);

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

//{{{
void cMemoryEdit::setContext (size_t memSize, size_t baseAddress) {

  mContext.mTextColor = ImGui::GetColorU32 (ImGuiCol_Text);
  mContext.mGreyColor = ImGui::GetColorU32 (ImGuiCol_TextDisabled);
  mContext.mHighlightColor = IM_COL32 (255, 255, 255, 50);  // background color of highlighted bytes.

  ImGuiStyle& style = ImGui::GetStyle();

  mContext.mAddrDigitsCount = mAddrDigitsCount;
  if (mContext.mAddrDigitsCount == 0)
    for (size_t n = baseAddress + memSize - 1; n > 0; n >>= 4)
      mContext.mAddrDigitsCount++;

  mContext.mLineHeight = ImGui::GetTextLineHeight();
  mContext.mGlyphWidth = ImGui::CalcTextSize ("F").x + 1;                      // We assume the font is mono-space
  mContext.mHexCellWidth = (float)(int)(mContext.mGlyphWidth * 2.5f);             // "FF " we include trailing space in the width to easily catch clicks everywhere
  mContext.mSpacingBetweenMidCols = (float)(int)(mContext.mHexCellWidth * 0.25f); // Every OptMidColsCount columns we add a bit of extra spacing

  mContext.mHexBeginPos = (mContext.mAddrDigitsCount + 2) * mContext.mGlyphWidth;
  mContext.mHexEndPos = mContext.mHexBeginPos + (mContext.mHexCellWidth * mColumns);
  mContext.mAsciiBeginPos = mContext.mAsciiEndPos = mContext.mHexEndPos;

  if (kShowAscii) {
    mContext.mAsciiBeginPos = mContext.mHexEndPos + mContext.mGlyphWidth * 1;
    if (mMidColsCount > 0)
      mContext.mAsciiBeginPos += (float)((mColumns + mMidColsCount - 1) /
                               mMidColsCount) * mContext.mSpacingBetweenMidCols;
    mContext.mAsciiEndPos = mContext.mAsciiBeginPos + mColumns * mContext.mGlyphWidth;
    }

  mContext.mWindowWidth = mContext.mAsciiEndPos + style.ScrollbarSize + style.WindowPadding.x * 2 + mContext.mGlyphWidth;
  }
//}}}
//{{{
void* cMemoryEdit::endianCopy (void* dst, void* src, size_t size) {

  if (!gEndianFunc)
    gEndianFunc = isCpuBigEndian() ? bigEndianFunc : littleEndianFunc;


  return gEndianFunc (dst, src, size, mBigEndian ^ mHoverEndian);
  }
//}}}

//{{{
void cMemoryEdit::drawHeader (uint8_t* memData, size_t memSize, size_t baseAddress) {

  ImGuiStyle& style = ImGui::GetStyle();

  // draw col box
  ImGui::SetNextItemWidth ((2.f*style.FramePadding.x) + (7.f*mContext.mGlyphWidth));
  ImGui::DragInt ("##col", &mColumns, 0.2f, 2, 64, "%d wide");

  // draw hexII box
  ImGui::SameLine();
  if (toggleButton ("hexII", mShowHexII))
    mShowHexII = !mShowHexII;
  mHoverHexII = ImGui::IsItemHovered();

  // draw gotoAddress box
  ImGui::SetNextItemWidth ((2 * style.FramePadding.x) + ((mContext.mAddrDigitsCount + 1) * mContext.mGlyphWidth));
  ImGui::SameLine();
  if (ImGui::InputText ("##addr", mAddrInputBuf, IM_ARRAYSIZE(mAddrInputBuf),
                         ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
    //{{{  consume input into gotoAddress
    size_t goto_addr;
    if (sscanf (mAddrInputBuf, "%" _PRISizeT "X", &goto_addr) == 1) {
      mGotoAddr = goto_addr - baseAddress;
      mHighlightMin = mHighlightMax = (size_t)-1;
      }
    }
    //}}}
  if (mGotoAddr != (size_t)-1) {
    //{{{  valid goto address
    if (mGotoAddr < memSize) {
      // use gotoAddress and scroll to it
      ImGui::BeginChild ("##scrolling");
      ImGui::SetScrollFromPosY (ImGui::GetCursorStartPos().y + (mGotoAddr / mColumns) * ImGui::GetTextLineHeight());
      ImGui::EndChild();
      mDataEditingAddr = mGotoAddr;
      mDataAddress = mGotoAddr;
      mDataEditingTakeFocus = true;
      }

    mGotoAddr = (size_t)-1;
    }
    //}}}

  if (mDataAddress != (size_t)-1) {
    // draw dataType combo
    ImGui::SetNextItemWidth ((2*style.FramePadding.x) + (8*mContext.mGlyphWidth) + style.ItemInnerSpacing.x);
    ImGui::SameLine();
    if (ImGui::BeginCombo ("##combo_type", getDataTypeDesc (mPreviewDataType), ImGuiComboFlags_HeightLargest)) {
      for (int dataType = 0; dataType < ImGuiDataType_COUNT; dataType++)
        if (ImGui::Selectable (getDataTypeDesc((ImGuiDataType)dataType), mPreviewDataType == dataType))
          mPreviewDataType = (ImGuiDataType)dataType;
      ImGui::EndCombo();
      }

    // draw endian combo
    if ((int)mPreviewDataType > 1) {
      ImGui::SameLine();
      if (toggleButton ("big", mBigEndian))
        mBigEndian = !mBigEndian;
      mHoverEndian = ImGui::IsItemHovered();
      }

    // draw dec
    ImGui::SameLine();
    getDataStr (mDataAddress, memData, memSize, mPreviewDataType, eDataFormat::eDec);
    ImGui::Text ("dec:%s", getDataStr (mDataAddress, memData, memSize, mPreviewDataType, eDataFormat::eDec));

    // draw hex
    ImGui::SameLine();
    getDataStr (mDataAddress, memData, memSize, mPreviewDataType, eDataFormat::eHex);
    ImGui::Text ("hex:%s", getDataStr (mDataAddress, memData, memSize, mPreviewDataType, eDataFormat::eHex));

    // draw bin
    ImGui::SameLine();
    getDataStr (mDataAddress, memData, memSize, mPreviewDataType, eDataFormat::eBin);
    ImGui::Text ("bin:%s", getDataStr (mDataAddress, memData, memSize, mPreviewDataType, eDataFormat::eBin));
    }
  }
//}}}
//{{{
void cMemoryEdit::drawLine (int lineNumber) {

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  // draw address
  size_t addr = static_cast<size_t>(lineNumber * mColumns);
  ImGui::Text (kFormatAddress, mContext.mAddrDigitsCount, mBaseAddress + addr);

  // draw hex
  for (int column = 0; column < mColumns && addr < mMemSize; column++, addr++) {
    float bytePosX = mContext.mHexBeginPos + mContext.mHexCellWidth * column;
    if (mMidColsCount > 0)
      bytePosX += (float)(column / mMidColsCount) * mContext.mSpacingBetweenMidCols;

    ImGui::SameLine (bytePosX);

    // highlight
    bool isHighlightUserRange = ((addr >= mHighlightMin) && (addr < mHighlightMax));
    bool isHighlightPreview = ((addr >= mDataAddress) &&
                               (addr < (mDataAddress + getDataTypeSize (mPreviewDataType))));
    if (isHighlightUserRange || isHighlightPreview) {
      //{{{  draw hex highlight
      ImVec2 pos = ImGui::GetCursorScreenPos();

      float highlightWidth = 2 * mContext.mGlyphWidth;
      bool isNextByteHighlighted =  (addr+1 < mMemSize) &&
                                    (((mHighlightMax != (size_t)-1) && (addr + 1 < mHighlightMax)));

      if (isNextByteHighlighted || (column + 1 == mColumns)) {
        highlightWidth = mContext.mHexCellWidth;
        if ((mMidColsCount > 0) &&
            (column > 0) && ((column + 1) < mColumns) &&
            (((column + 1) % mMidColsCount) == 0))
          highlightWidth += mContext.mSpacingBetweenMidCols;
        }

      draw_list->AddRectFilled (pos, ImVec2(pos.x + highlightWidth, pos.y + mContext.mLineHeight), mContext.mHighlightColor);
      }
      //}}}
    if (mDataEditingAddr == addr) {
      //{{{  display text input on current byte
      bool dataWrite = false;

      ImGui::PushID ((void*)addr);
      if (mDataEditingTakeFocus) {
        ImGui::SetKeyboardFocusHere();
        ImGui::CaptureKeyboardFromApp (true);
        sprintf (mAddrInputBuf, kFormatData, mContext.mAddrDigitsCount, mBaseAddress + addr);
        sprintf (mDataInputBuf, kFormatByte, mMemData[addr]);
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

      sprintf (userData.mCurrentBufOverwrite, kFormatByte, mMemData[addr]);
      ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal |
                                  ImGuiInputTextFlags_EnterReturnsTrue |
                                  ImGuiInputTextFlags_AutoSelectAll |
                                  ImGuiInputTextFlags_NoHorizontalScroll |
                                  ImGuiInputTextFlags_CallbackAlways;
      flags |= ImGuiInputTextFlags_AlwaysOverwrite;

      ImGui::SetNextItemWidth (mContext.mGlyphWidth * 2);

      if (ImGui::InputText ("##data", mDataInputBuf, IM_ARRAYSIZE(mDataInputBuf), flags, sUserData::callback, &userData))
        dataWrite = mDataNext = true;
      else if (!mDataEditingTakeFocus && !ImGui::IsItemActive())
        mDataEditingAddr = mDataEditingAddrNext = (size_t)-1;

      mDataEditingTakeFocus = false;

      if (userData.mCursorPos >= 2)
        dataWrite = mDataNext = true;
      if (mDataEditingAddrNext != (size_t)-1)
        dataWrite = mDataNext = false;

      unsigned int dataInputValue = 0;
      if (dataWrite && sscanf(mDataInputBuf, "%X", &dataInputValue) == 1)
        mMemData[addr] = (ImU8)dataInputValue;

      ImGui::PopID();
      }
      //}}}
    else {
      //{{{  display text
      // NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click on.
      ImU8 value = mMemData[addr];

      if (mShowHexII || mHoverHexII) {
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

      if (!mReadOnly &&
          ImGui::IsItemHovered() && ImGui::IsMouseClicked (0)) {
        mDataEditingTakeFocus = true;
        mDataEditingAddrNext = addr;
        }
      }
      //}}}
    }

  if (kShowAscii) {
    //{{{  draw righthand ASCII
    ImGui::SameLine (mContext.mAsciiBeginPos);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    addr = lineNumber * mColumns;

    ImGui::PushID (lineNumber);
    if (ImGui::InvisibleButton ("ascii", ImVec2 (mContext.mAsciiEndPos - mContext.mAsciiBeginPos, mContext.mLineHeight))) {
      mDataAddress = addr + (size_t)((ImGui::GetIO().MousePos.x - pos.x) / mContext.mGlyphWidth);
      mDataEditingAddr = mDataAddress;
      mDataEditingTakeFocus = true;
      }

    ImGui::PopID();
    for (int column = 0; (column < mColumns) && (addr < mMemSize); column++, addr++) {
      if (addr == mDataEditingAddr) {
        draw_list->AddRectFilled (pos, ImVec2 (pos.x + mContext.mGlyphWidth, pos.y + mContext.mLineHeight),
                                 ImGui::GetColorU32 (ImGuiCol_FrameBg));
        draw_list->AddRectFilled (pos, ImVec2 (pos.x + mContext.mGlyphWidth, pos.y + mContext.mLineHeight),
                                 ImGui::GetColorU32 (ImGuiCol_TextSelectedBg));
        }

      uint8_t value = mMemData[addr];
      char displayCh = ((value < 32) || (value >= 128)) ? '.' : value;
      ImU32 color = (!kGreyZeroes || (value == displayCh)) ? mContext.mTextColor :  mContext.mGreyColor;
      draw_list->AddText (pos, color, &displayCh, &displayCh + 1);
      pos.x += mContext.mGlyphWidth;
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
