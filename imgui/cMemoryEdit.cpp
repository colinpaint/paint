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
constexpr bool kUpperCaseHex = false;
constexpr bool kGreyZeroes = true;

namespace {
  //{{{  const
  const char* kFormatDescs[] = {"Bin", "Dec", "Hex"};
  const char* kTypeDescs[] = {"Int8", "Uint8", "Int16", "Uint16", "Int32", "Uint32", "Int64", "Uint64", "Float", "Double"};
  const size_t kSizes[] = {1, 1, 2, 2, 4, 4, 8, 8, sizeof(float), sizeof(double)};
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
  }

// public:
//{{{
void cMemoryEdit::drawWindow (const string& title, uint8_t* memData, size_t memSize, size_t baseDisplayAddress) {
// standalone Memory Editor window

  mOpen = true;

  ImGui::Begin (title.c_str(), &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

  drawContents (memData, memSize, baseDisplayAddress);

  ImGui::End();
  }
//}}}
//{{{
void cMemoryEdit::drawContents (uint8_t* memData, size_t memSize, size_t baseDisplayAddress) {

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  const ImU32 colorText = ImGui::GetColorU32 (ImGuiCol_Text);
  const ImU32 colorDisabled = kGreyZeroes ? ImGui::GetColorU32 (ImGuiCol_TextDisabled) : colorText;

  const char* formatByte = kUpperCaseHex ? "%02X" : "%02x";
  const char* formatData = kUpperCaseHex ? "%0*" _PRISizeT "X" : "%0*" _PRISizeT "x";
  const char* formatAddress = kUpperCaseHex ? "%0*" _PRISizeT "X: " : "%0*" _PRISizeT "x: ";
  const char* formatByteSpace = kUpperCaseHex ? "%02X " : "%02x ";

  size_t previewDataSize = getDataTypeSize (mPreviewDataType);

  cSizes sizes;
  calcSizes (sizes, memSize, baseDisplayAddress);
  drawHeader (sizes, memData, memSize, baseDisplayAddress);

  // We begin into our scrolling region with the 'ImGuiWindowFlags_NoMove'
  // - to prevent click from moving the window.
  // This is used as a facility since our main click detection code
  // - doesn't assign an ActiveId so the click would normally be caught as a window-move.
  ImGui::BeginChild ("##scrolling", ImVec2(0,0), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
  ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, ImVec2(0,0));
  ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

  // We are not really using the clipper API correctly here,
  // - we rely on visible_start_addr / visible_end_addr for our scrolling function.
  const int line_total_count = (int)((memSize + mCols - 1) / mCols);
  ImGuiListClipper clipper;
  clipper.Begin (line_total_count, sizes.mLineHeight);
  clipper.Step();
  const size_t visibleBeginAddr = clipper.DisplayStart * mCols;
  const size_t visibleEndAddr = clipper.DisplayEnd * mCols;

  bool dataNext = false;
  if (mReadOnly || mDataEditingAddr >= memSize)
    mDataEditingAddr = (size_t)-1;
  if (mDataAddress >= memSize)
    mDataAddress = (size_t)-1;

  size_t prevDataEditingAddr = mDataEditingAddr;
  size_t dataEditingAddrNext = (size_t)-1;
  if (mDataEditingAddr != (size_t)-1) {
    //{{{  move cursor
    // move cursor but only apply on next frame so scrolling with be synchronized
    // - because currently we can't change the scrolling while the window is being rendered)
    if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_UpArrow)) && mDataEditingAddr >= (size_t)mCols) {
      dataEditingAddrNext = mDataEditingAddr - mCols;
      mDataEditingTakeFocus = true;
      }

    else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_DownArrow)) && mDataEditingAddr < memSize - mCols) {
      dataEditingAddrNext = mDataEditingAddr + mCols;
      mDataEditingTakeFocus = true;
      }

    else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_LeftArrow)) && mDataEditingAddr > 0) {
      dataEditingAddrNext = mDataEditingAddr - 1;
      mDataEditingTakeFocus = true;
      }

    else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_RightArrow)) && mDataEditingAddr < memSize - 1) {
      dataEditingAddrNext = mDataEditingAddr + 1;
      mDataEditingTakeFocus = true;
      }
    }
    //}}}
  if ((dataEditingAddrNext != (size_t)-1) &&
      ((dataEditingAddrNext / mCols) != (prevDataEditingAddr / mCols))) {
    //{{{  track cursor
    const int scrollOffset = ((int)(dataEditingAddrNext / mCols) - (int)(prevDataEditingAddr / mCols));
    const bool scrollDesired = ((scrollOffset < 0) && (dataEditingAddrNext < visibleBeginAddr + (mCols * 2))) ||
                               ((scrollOffset > 0) && (dataEditingAddrNext > visibleEndAddr - (mCols * 2)));
    if (scrollDesired)
      ImGui::SetScrollY (ImGui::GetScrollY() + scrollOffset * sizes.mLineHeight);
    }
    //}}}

  //{{{  draw vertical separator
  if (kShowAscii) {
    ImVec2 windowPos = ImGui::GetWindowPos();
    draw_list->AddLine (ImVec2 (windowPos.x + sizes.mPosAsciiStart - sizes.mGlyphWidth, windowPos.y),
                        ImVec2 (windowPos.x + sizes.mPosAsciiStart - sizes.mGlyphWidth, windowPos.y + 9999),
                        ImGui::GetColorU32 (ImGuiCol_Border));
    }
  //}}}

  for (int lineNumber = clipper.DisplayStart; lineNumber < clipper.DisplayEnd; lineNumber++) {
    //{{{  display only visible lines
    size_t addr = (size_t)(lineNumber * mCols);
    ImGui::Text (formatAddress, sizes.mAddrDigitsCount, baseDisplayAddress + addr);

    // draw hex
    for (int n = 0; n < mCols && addr < memSize; n++, addr++) {
      float bytePosX = sizes.mPosHexStart + sizes.mHexCellWidth * n;
      if (mMidColsCount > 0)
        bytePosX += (float)(n / mMidColsCount) * sizes.mSpacingBetweenMidCols;
      ImGui::SameLine (bytePosX);

      // draw highlight
      bool isHighlightFromUserRange = ((addr >= mHighlightMin) && (addr < mHighlightMax));
      bool isHighlightFromPreview = ((addr >= mDataAddress) && (addr < (mDataAddress + previewDataSize)));
      if (isHighlightFromUserRange || isHighlightFromPreview) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float highlightWidth = 2 * sizes.mGlyphWidth;
        bool isNextByteHighlighted =  (addr + 1 < memSize) &&
                                      (((mHighlightMax != (size_t)-1) && (addr + 1 < mHighlightMax)));
        if (isNextByteHighlighted || (n + 1 == mCols)) {
          highlightWidth = sizes.mHexCellWidth;
          if ((mMidColsCount > 0) &&
              (n > 0) && ((n + 1) < mCols) &&
              (((n + 1) % mMidColsCount) == 0))
            highlightWidth += sizes.mSpacingBetweenMidCols;
          }

        draw_list->AddRectFilled (pos, ImVec2(pos.x + highlightWidth, pos.y + sizes.mLineHeight), mHighlightColor);
        }

      if (mDataEditingAddr == addr) {
        //{{{  display text input on current byte
        bool dataWrite = false;

        ImGui::PushID ((void*)addr);
        if (mDataEditingTakeFocus) {
          ImGui::SetKeyboardFocusHere();
          ImGui::CaptureKeyboardFromApp (true);
          sprintf (mAddrInputBuf, formatData, sizes.mAddrDigitsCount, baseDisplayAddress + addr);
          sprintf (mDataInputBuf, formatByte, memData[addr]);
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

        sprintf (userData.mCurrentBufOverwrite, formatByte, memData[addr]);
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal |
                                    ImGuiInputTextFlags_EnterReturnsTrue |
                                    ImGuiInputTextFlags_AutoSelectAll |
                                    ImGuiInputTextFlags_NoHorizontalScroll |
                                    ImGuiInputTextFlags_CallbackAlways;
        flags |= ImGuiInputTextFlags_AlwaysOverwrite;
        //flags |= ImGuiInputTextFlags_AlwaysInsertMode;

        ImGui::SetNextItemWidth (sizes.mGlyphWidth * 2);

        if (ImGui::InputText ("##data", mDataInputBuf, IM_ARRAYSIZE(mDataInputBuf), flags, sUserData::callback, &userData))
          dataWrite = dataNext = true;
        else if (!mDataEditingTakeFocus && !ImGui::IsItemActive())
          mDataEditingAddr = dataEditingAddrNext = (size_t)-1;

        mDataEditingTakeFocus = false;

        if (userData.mCursorPos >= 2)
          dataWrite = dataNext = true;
        if (dataEditingAddrNext != (size_t)-1)
          dataWrite = dataNext = false;

        unsigned int dataInputValue = 0;
        if (dataWrite && sscanf(mDataInputBuf, "%X", &dataInputValue) == 1) {
          memData[addr] = (ImU8)dataInputValue;
          }

        ImGui::PopID();
        }
        //}}}
      else {
        //{{{  display text
        // NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click on.
        ImU8 b = memData[addr];

        if (mShowHexII || mHoverHexII) {
          if ((b >= 32 && b < 128))
            ImGui::Text (".%c ", b);
          else if ((b == 0xFF) && kGreyZeroes)
            ImGui::TextDisabled ("## ");
          else if (b == 0x00)
            ImGui::Text ("   ");
          else
            ImGui::Text (formatByteSpace, b);
          }

        else if ((b == 0) && kGreyZeroes)
          ImGui::TextDisabled ("00 ");

        else
          ImGui::Text (formatByteSpace, b);

        if (!mReadOnly &&
            ImGui::IsItemHovered() && ImGui::IsMouseClicked (0)) {
          mDataEditingTakeFocus = true;
          dataEditingAddrNext = addr;
          }
        }
        //}}}
      }

    if (kShowAscii) {
      //{{{  draw ASCII values
      ImGui::SameLine (sizes.mPosAsciiStart);
      ImVec2 pos = ImGui::GetCursorScreenPos();
      addr = lineNumber * mCols;

      ImGui::PushID (lineNumber);
      if (ImGui::InvisibleButton ("ascii", ImVec2 (sizes.mPosAsciiEnd - sizes.mPosAsciiStart, sizes.mLineHeight))) {
        mDataEditingAddr = mDataAddress = addr + (size_t)((ImGui::GetIO().MousePos.x - pos.x) / sizes.mGlyphWidth);
        mDataEditingTakeFocus = true;
        }

      ImGui::PopID();
      for (int column = 0; column < mCols && addr < memSize; column++, addr++) {
        if (addr == mDataEditingAddr) {
          draw_list->AddRectFilled (pos, ImVec2 (pos.x + sizes.mGlyphWidth, pos.y + sizes.mLineHeight),
                                   ImGui::GetColorU32 (ImGuiCol_FrameBg));
          draw_list->AddRectFilled (pos, ImVec2 (pos.x + sizes.mGlyphWidth, pos.y + sizes.mLineHeight),
                                   ImGui::GetColorU32 (ImGuiCol_TextSelectedBg));
          }

        uint8_t value = memData[addr];
        char displayCh = (value < 32 || value >= 128) ? '.' : value;
        draw_list->AddText (pos, (displayCh == value) ? colorText : colorDisabled, &displayCh, &displayCh + 1);
        pos.x += sizes.mGlyphWidth;
        }
      }
      //}}}
    }
    //}}}

  clipper.Step();
  clipper.End();
  ImGui::PopStyleVar (2);
  ImGui::EndChild();

  // Notify the main window of our ideal child content size
  // - (FIXME: we are missing an API to get the contents size from the child)
  ImGui::SetCursorPosX (sizes.mWindowWidth);

  if (dataNext && mDataEditingAddr < memSize) {
    mDataEditingAddr++;
    mDataAddress = mDataEditingAddr;
    mDataEditingTakeFocus = true;
    }
  else if (dataEditingAddrNext != (size_t)-1) {
    mDataEditingAddr = dataEditingAddrNext;
    mDataAddress = dataEditingAddrNext;
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
bool cMemoryEdit::isBigEndian() const {

  uint16_t x = 1;

  char c[2];
  memcpy (c, &x, 2);

  return c[0] != 0;
  }
//}}}
//{{{
size_t cMemoryEdit::getDataTypeSize (ImGuiDataType dataType) const {

  return kSizes[dataType];
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
// return pointer to class mOutBuf

  size_t elemSize = getDataTypeSize (dataType);
  size_t size = ((addr + elemSize) > memSize) ? (memSize - addr) : elemSize;

  uint8_t buf[8];
  memcpy (buf, memData + addr, size);

  mOutBuf[0] = 0;

  if (dataFormat == eDataFormat::eBin) {
    //{{{  binary to mOutBuf, return
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
  else {// other data types to mOutBuf
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
void cMemoryEdit::calcSizes (cSizes& sizes, size_t memSize, size_t baseDisplayAddress) {

  ImGuiStyle& style = ImGui::GetStyle();

  sizes.mAddrDigitsCount = mAddrDigitsCount;
  if (sizes.mAddrDigitsCount == 0)
    for (size_t n = baseDisplayAddress + memSize - 1; n > 0; n >>= 4)
      sizes.mAddrDigitsCount++;

  sizes.mLineHeight = ImGui::GetTextLineHeight();
  sizes.mGlyphWidth = ImGui::CalcTextSize ("F").x + 1;                  // We assume the font is mono-space
  sizes.mHexCellWidth = (float)(int)(sizes.mGlyphWidth * 2.5f);             // "FF " we include trailing space in the width to easily catch clicks everywhere
  sizes.mSpacingBetweenMidCols = (float)(int)(sizes.mHexCellWidth * 0.25f); // Every OptMidColsCount columns we add a bit of extra spacing
  sizes.mPosHexStart = (sizes.mAddrDigitsCount + 2) * sizes.mGlyphWidth;
  sizes.mPosHexEnd = sizes.mPosHexStart + (sizes.mHexCellWidth * mCols);
  sizes.mPosAsciiStart = sizes.mPosAsciiEnd = sizes.mPosHexEnd;

  if (kShowAscii) {
    sizes.mPosAsciiStart = sizes.mPosHexEnd + sizes.mGlyphWidth * 1;
    if (mMidColsCount > 0)
      sizes.mPosAsciiStart += (float)((mCols + mMidColsCount - 1) / mMidColsCount) * sizes.mSpacingBetweenMidCols;
    sizes.mPosAsciiEnd = sizes.mPosAsciiStart + mCols * sizes.mGlyphWidth;
    }

  sizes.mWindowWidth = sizes.mPosAsciiEnd + style.ScrollbarSize + style.WindowPadding.x * 2 + sizes.mGlyphWidth;
  }
//}}}
//{{{
void* cMemoryEdit::endianCopy (void* dst, void* src, size_t size) {

  if (!mEndianFunc)
    mEndianFunc = isBigEndian() ? bigEndianFunc : littleEndianFunc;

  return mEndianFunc (dst, src, size, mBigEndian ^ mHoverEndian);
  }
//}}}

//{{{
void cMemoryEdit::drawHeader (const cSizes& sizes, uint8_t* memData, size_t memSize, size_t baseDisplayAddress) {

  ImGuiStyle& style = ImGui::GetStyle();

  // draw col box
  ImGui::SetNextItemWidth ((2.f*style.FramePadding.x) + (7.f*sizes.mGlyphWidth));
  ImGui::DragInt ("##col", &mCols, 0.2f, 2, 64, "%d col");

  // draw hexII box
  ImGui::SameLine();
  if (toggleButton ("hexII", mShowHexII))
    mShowHexII = !mShowHexII;
  mHoverHexII = ImGui::IsItemHovered();

  // draw gotoAddress box
  ImGui::SetNextItemWidth ((2 * style.FramePadding.x) + ((sizes.mAddrDigitsCount + 1) * sizes.mGlyphWidth));
  ImGui::SameLine();
  if (ImGui::InputText ("##addr", mAddrInputBuf, IM_ARRAYSIZE(mAddrInputBuf),
                         ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
    //{{{  consume input into gotoAddress
    size_t goto_addr;
    if (sscanf (mAddrInputBuf, "%" _PRISizeT "X", &goto_addr) == 1) {
      mGotoAddr = goto_addr - baseDisplayAddress;
      mHighlightMin = mHighlightMax = (size_t)-1;
      }
    }
    //}}}
  if (mGotoAddr != (size_t)-1) {
    //{{{  valid goto address
    if (mGotoAddr < memSize) {
      // use gotoAddress and scroll to it
      ImGui::BeginChild ("##scrolling");
      ImGui::SetScrollFromPosY (ImGui::GetCursorStartPos().y + (mGotoAddr / mCols) * ImGui::GetTextLineHeight());
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
    ImGui::SetNextItemWidth ((2*style.FramePadding.x) + (8*sizes.mGlyphWidth) + style.ItemInnerSpacing.x);
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

//{{{  undefs, pop warnings
#undef _PRISizeT
#undef ImSnprintf
#ifdef _MSC_VER
  #pragma warning (pop)
#endif
//}}}
