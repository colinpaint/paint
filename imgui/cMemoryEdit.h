//{{{  cMemoryEdit - hacked version of https://github.com/ocornut/imgui_club
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
// Changelog:
// - v0.10: initial version
// - v0.23 (2017/08/17): added to github. fixed right-arrow triggering a byte write.
// - v0.24 (2018/06/02): changed DragInt("Rows" to use a %d data format (which is desirable since imgui 1.61).
// - v0.25 (2018/07/11): fixed wording: all occurrences of "Rows" renamed to "Columns".
// - v0.26 (2018/08/02): fixed clicking on hex region
// - v0.30 (2018/08/02): added data preview for common data types
// - v0.31 (2018/10/10): added OptUpperCaseHex option to select lower/upper casing display [@samhocevar]
// - v0.32 (2018/10/10): changed signatures to use void* instead of unsigned char*
// - v0.33 (2018/10/10): added OptShowOptions option to hide all the interactive option setting.
// - v0.34 (2019/05/07): binary preview now applies endianness setting [@nicolasnoble]
// - v0.35 (2020/01/29): using ImGuiDataType available since Dear ImGui 1.69.
// - v0.36 (2020/05/05): minor tweaks, minor refactor.
// - v0.40 (2020/10/04): fix misuse of ImGuiListClipper API, broke with Dear ImGui 1.79. made cursor position appears on left-side of edit box. option popup appears on mouse release. fix MSVC warnings where _CRT_SECURE_NO_WARNINGS wasn't working in recent versions.
// - v0.41 (2020/10/05): fix when using with keyboard/gamepad navigation enabled.
// - v0.42 (2020/10/14): fix for . character in ASCII view always being greyed out.
// - v0.43 (2021/03/12): added OptFooterExtraHeight to allow for custom drawing at the bottom of the editor [@leiradel]
// - v0.44 (2021/03/12): use ImGuiInputTextFlags_AlwaysOverwrite in 1.82 + fix hardcoded width.
//
// Todo/Bugs:
// - This is generally old code, it should work but please don't use this as reference!
// - Arrows are being sent to the InputText() about to disappear which for LeftArrow makes the text cursor appear at position 1 for one frame.
// - Using InputText() is awkward and maybe overkill here, consider implementing something custom.
//}}}
#pragma once
//{{{  includes
#include <cstdint>
#include <stdio.h>  // sprintf, scanf

#include "../imgui/imgui.h"


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

namespace {
  //{{{
  void* littleEndianCopy (void* dst, void* src, size_t s, int is_little_endian) {

    if (is_little_endian)
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
  void* bigEndianCopy (void* dst, void* src, size_t s, int is_little_endian) {

    if (is_little_endian) {
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
  void* (*gEndianFuncPtr)(void*, void*, size_t, int) = nullptr;
  }

class cMemoryEdit {
public:
  cMemoryEdit() = default;
  ~cMemoryEdit() = default;

  //{{{
  void drawWindow (const char* title, void* voidMemData, size_t memSize, size_t baseDisplayAddress) {
  // Standalone Memory Editor window

    sSizes s;
    calcSizes (s, memSize, baseDisplayAddress);
    ImGui::SetNextWindowSizeConstraints (ImVec2 (0.f,0.f), ImVec2(s.mWindowWidth, FLT_MAX));

    mOpen = true;
    if (ImGui::Begin (title, &mOpen, ImGuiWindowFlags_NoScrollbar)) {

      if (ImGui::IsWindowHovered (ImGuiHoveredFlags_RootAndChildWindows) &&
          ImGui::IsMouseReleased (ImGuiMouseButton_Right))
        ImGui::OpenPopup ("context");

      drawContents (voidMemData, memSize, baseDisplayAddress);

      if (mContentsWidthChanged) {
        calcSizes (s, memSize, baseDisplayAddress);
        ImGui::SetWindowSize (ImVec2 (s.mWindowWidth, ImGui::GetWindowSize().y));
        }
      }

    ImGui::End();
    }
  //}}}
  //{{{
  void drawContents (void* voidMemData, size_t memSize, size_t baseDisplayAddress) {

    if (mCols < 1)
      mCols = 1;

    ImU8* memData = (ImU8*)voidMemData;

    sSizes s;
    calcSizes(s, memSize, baseDisplayAddress);
    ImGuiStyle& style = ImGui::GetStyle();

    // We begin into our scrolling region with the 'ImGuiWindowFlags_NoMove'
    // - to prevent click from moving the window.
    // This is used as a facility since our main click detection code
    // - doesn't assign an ActiveId so the click would normally be caught as a window-move.
    const float height_separator = style.ItemSpacing.y;
    float footer_height = mOptFooterExtraHeight;
    if (mOptShowOptions)
      footer_height += height_separator + ImGui::GetFrameHeightWithSpacing() * 1;
    if (mOptShowDataPreview)
      footer_height += height_separator + ImGui::GetFrameHeightWithSpacing() * 1 + ImGui::GetTextLineHeightWithSpacing() * 3;
    ImGui::BeginChild ("##scrolling", ImVec2(0, -footer_height), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    // We are not really using the clipper API correctly here,
    // because we rely on visible_start_addr/visible_end_addr for our scrolling function.
    const int line_total_count = (int)((memSize + mCols - 1) / mCols);
    ImGuiListClipper clipper;
    clipper.Begin(line_total_count, s.mLineHeight);
    clipper.Step();
    const size_t visible_start_addr = clipper.DisplayStart * mCols;
    const size_t visible_end_addr = clipper.DisplayEnd * mCols;

    bool data_next = false;
    if (mReadOnly || mDataEditingAddr >= memSize)
      mDataEditingAddr = (size_t)-1;
    if (mDataPreviewAddr >= memSize)
      mDataPreviewAddr = (size_t)-1;

    size_t preview_data_type_size = mOptShowDataPreview ? getDataTypeSize (mPreviewDataType) : 0;
    size_t data_editing_addr_backup = mDataEditingAddr;
    size_t data_editing_addr_next = (size_t)-1;

    if (mDataEditingAddr != (size_t)-1) {
      //{{{  move cursor but only apply on next frame so scrolling with be synchronized (because currently we can't change the scrolling while the window is being rendered)
      if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_UpArrow)) && mDataEditingAddr >= (size_t)mCols) {
        data_editing_addr_next = mDataEditingAddr - mCols;
        mDataEditingTakeFocus = true;
        }
      else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_DownArrow)) && mDataEditingAddr < memSize - mCols) {
        data_editing_addr_next = mDataEditingAddr + mCols;
        mDataEditingTakeFocus = true;
        }
      else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_LeftArrow)) && mDataEditingAddr > 0) {
        data_editing_addr_next = mDataEditingAddr - 1;
        mDataEditingTakeFocus = true;
        }
      else if (ImGui::IsKeyPressed (ImGui::GetKeyIndex (ImGuiKey_RightArrow)) && mDataEditingAddr < memSize - 1) {
        data_editing_addr_next = mDataEditingAddr + 1;
        mDataEditingTakeFocus = true;
        }
      }
      //}}}
    if (data_editing_addr_next != (size_t)-1 && (data_editing_addr_next / mCols) != (data_editing_addr_backup / mCols)) {
      //{{{  track cursor movements
      const int scroll_offset = ((int)(data_editing_addr_next / mCols) - (int)(data_editing_addr_backup / mCols));
      const bool scroll_desired = (scroll_offset < 0 && data_editing_addr_next < visible_start_addr + mCols * 2) || (scroll_offset > 0 && data_editing_addr_next > visible_end_addr - mCols * 2);
      if (scroll_desired)
        ImGui::SetScrollY (ImGui::GetScrollY() + scroll_offset * s.mLineHeight);
      }
      //}}}

    // draw vertical separator
    ImVec2 window_pos = ImGui::GetWindowPos();
    if (mOptShowAscii)
      draw_list->AddLine (ImVec2 (window_pos.x + s.mPosAsciiStart - s.mGlyphWidth, window_pos.y), ImVec2(window_pos.x + s.mPosAsciiStart - s.mGlyphWidth, window_pos.y + 9999), ImGui::GetColorU32(ImGuiCol_Border));

    const ImU32 color_text = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 color_disabled = mOptGreyOutZeroes ? ImGui::GetColorU32(ImGuiCol_TextDisabled) : color_text;
    const char* format_address = mOptUpperCaseHex ? "%0*" _PRISizeT "X: " : "%0*" _PRISizeT "x: ";
    const char* format_data = mOptUpperCaseHex ? "%0*" _PRISizeT "X" : "%0*" _PRISizeT "x";
    const char* format_byte = mOptUpperCaseHex ? "%02X" : "%02x";
    const char* format_byte_space = mOptUpperCaseHex ? "%02X " : "%02x ";

    for (int line_i = clipper.DisplayStart; line_i < clipper.DisplayEnd; line_i++) {
      //{{{  isplay only visible lines
      size_t addr = (size_t)(line_i * mCols);
      ImGui::Text (format_address, s.mAddrDigitsCount, baseDisplayAddress + addr);

      // Draw Hexadecimal
      for (int n = 0; n < mCols && addr < memSize; n++, addr++) {
        float byte_pos_x = s.mPosHexStart + s.mHexCellWidth * n;
        if (mOptMidColsCount > 0)
          byte_pos_x += (float)(n / mOptMidColsCount) * s.mSpacingBetweenMidCols;
        ImGui::SameLine(byte_pos_x);

        // Draw highlight
        bool is_highlight_from_user_range = (addr >= mHighlightMin && addr < mHighlightMax);
        bool is_highlight_from_user_func = (mHighlightFn && mHighlightFn(memData, addr));
        bool is_highlight_from_preview = (addr >= mDataPreviewAddr && addr < mDataPreviewAddr + preview_data_type_size);
        if (is_highlight_from_user_range || is_highlight_from_user_func || is_highlight_from_preview) {
          ImVec2 pos = ImGui::GetCursorScreenPos();
          float highlight_width = s.mGlyphWidth * 2;
          bool is_next_byte_highlighted =  (addr + 1 < memSize) && ((mHighlightMax != (size_t)-1 && addr + 1 < mHighlightMax) || (mHighlightFn && mHighlightFn(memData, addr + 1)));
          if (is_next_byte_highlighted || (n + 1 == mCols)) {
            highlight_width = s.mHexCellWidth;
            if (mOptMidColsCount > 0 && n > 0 && (n + 1) < mCols && ((n + 1) % mOptMidColsCount) == 0)
              highlight_width += s.mSpacingBetweenMidCols;
            }
          draw_list->AddRectFilled (pos, ImVec2(pos.x + highlight_width, pos.y + s.mLineHeight), mHighlightColor);
          }

        if (mDataEditingAddr == addr) {
          // Display text input on current byte
          bool data_write = false;
          ImGui::PushID ((void*)addr);
          if (mDataEditingTakeFocus) {
              ImGui::SetKeyboardFocusHere();
              ImGui::CaptureKeyboardFromApp (true);
              sprintf (mAddrInputBuf, format_data, s.mAddrDigitsCount, baseDisplayAddress + addr);
              sprintf (mDataInputBuf, format_byte, mReadFn ? mReadFn(memData, addr) : memData[addr]);
          }
        struct UserData {
          //{{{
          static int Callback(ImGuiInputTextCallbackData* data) {
          // FIXME: We should have a way to retrieve the text edit cursor position more easily in the API
          //  this is rather tedious. This is such a ugly mess we may be better off not using InputText() at all here.

            UserData* user_data = (UserData*)data->UserData;
            if (!data->HasSelection())
               user_data->CursorPos = data->CursorPos;

            if (data->SelectionStart == 0 && data->SelectionEnd == data->BufTextLen) {
              // When not editing a byte, always rewrite its content (this is a bit tricky, since InputText technically "owns" the master copy of the buffer we edit it in there)
              data->DeleteChars(0, data->BufTextLen);
              data->InsertChars(0, user_data->CurrentBufOverwrite);
              data->SelectionStart = 0;
              data->SelectionEnd = 2;
              data->CursorPos = 0;
              }
            return 0;
            }

          char   CurrentBufOverwrite[3];  // Input
          int    CursorPos;               // Output
          //}}}
          };
        UserData user_data;
        user_data.CursorPos = -1;

        sprintf (user_data.CurrentBufOverwrite, format_byte, mReadFn ? mReadFn(memData, addr) : memData[addr]);
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal |
                                    ImGuiInputTextFlags_EnterReturnsTrue |
                                    ImGuiInputTextFlags_AutoSelectAll |
                                    ImGuiInputTextFlags_NoHorizontalScroll |
                                    ImGuiInputTextFlags_CallbackAlways;
        #if IMGUI_VERSION_NUM >= 18104
          flags |= ImGuiInputTextFlags_AlwaysOverwrite;
        #else
          flags |= ImGuiInputTextFlags_AlwaysInsertMode;
        #endif

        ImGui::SetNextItemWidth (s.mGlyphWidth * 2);
        if (ImGui::InputText ("##data", mDataInputBuf, IM_ARRAYSIZE(mDataInputBuf), flags, UserData::Callback, &user_data))
          data_write = data_next = true;
        else if (!mDataEditingTakeFocus && !ImGui::IsItemActive())
          mDataEditingAddr = data_editing_addr_next = (size_t)-1;
        mDataEditingTakeFocus = false;

        if (user_data.CursorPos >= 2)
          data_write = data_next = true;
        if (data_editing_addr_next != (size_t)-1)
          data_write = data_next = false;

        unsigned int data_input_value = 0;
        if (data_write && sscanf(mDataInputBuf, "%X", &data_input_value) == 1) {
          if (mWriteFn)
            mWriteFn(memData, addr, (ImU8)data_input_value);
          else
            memData[addr] = (ImU8)data_input_value;
          }
        ImGui::PopID();
        }

      else {
        // NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click on.
        ImU8 b = mReadFn ? mReadFn (memData, addr) : memData[addr];

        if (mOptShowHexII) {
          if ((b >= 32 && b < 128))
            ImGui::Text(".%c ", b);
          else if (b == 0xFF && mOptGreyOutZeroes)
            ImGui::TextDisabled("## ");
          else if (b == 0x00)
            ImGui::Text("   ");
          else
            ImGui::Text(format_byte_space, b);
          }
        else {
          if (b == 0 && mOptGreyOutZeroes)
            ImGui::TextDisabled("00 ");
          else
            ImGui::Text(format_byte_space, b);
          }
        if (!mReadOnly && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
          mDataEditingTakeFocus = true;
          data_editing_addr_next = addr;
          }
        }
      }
      //}}}

    if (mOptShowAscii) {
      //{{{  Draw ASCII values
      ImGui::SameLine(s.mPosAsciiStart);
      ImVec2 pos = ImGui::GetCursorScreenPos();
      addr = line_i * mCols;
      ImGui::PushID(line_i);
      if (ImGui::InvisibleButton("ascii", ImVec2(s.mPosAsciiEnd - s.mPosAsciiStart, s.mLineHeight))) {
        mDataEditingAddr = mDataPreviewAddr = addr + (size_t)((ImGui::GetIO().MousePos.x - pos.x) / s.mGlyphWidth);
        mDataEditingTakeFocus = true;
        }
      ImGui::PopID();
      for (int n = 0; n < mCols && addr < memSize; n++, addr++) {
        if (addr == mDataEditingAddr) {
            draw_list->AddRectFilled(pos, ImVec2(pos.x + s.mGlyphWidth, pos.y + s.mLineHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));
            draw_list->AddRectFilled(pos, ImVec2(pos.x + s.mGlyphWidth, pos.y + s.mLineHeight), ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
            }
          unsigned char c = mReadFn ? mReadFn(memData, addr) : memData[addr];
          char display_c = (c < 32 || c >= 128) ? '.' : c;
          draw_list->AddText(pos, (display_c == c) ? color_text : color_disabled, &display_c, &display_c + 1);
          pos.x += s.mGlyphWidth;
          }
        }
      }
      //}}}

    IM_ASSERT (clipper.Step() == false);
    clipper.End();
    ImGui::PopStyleVar (2);
    ImGui::EndChild();

    // Notify the main window of our ideal child content size
    // - (FIXME: we are missing an API to get the contents size from the child)
    ImGui::SetCursorPosX (s.mWindowWidth);

    if (data_next && mDataEditingAddr < memSize) {
      mDataEditingAddr = mDataPreviewAddr = mDataEditingAddr + 1;
      mDataEditingTakeFocus = true;
      }
    else if (data_editing_addr_next != (size_t)-1) {
      mDataEditingAddr = mDataPreviewAddr = data_editing_addr_next;
      }

    const bool lock_show_data_preview = mOptShowDataPreview;
    if (mOptShowOptions) {
      //{{{  draw options
      ImGui::Separator();
      drawOptionsLine (s, memData, memSize, baseDisplayAddress);
      }
      //}}}

    if (lock_show_data_preview) {
      //{{{  show preview
      ImGui::Separator();
      drawPreviewLine (s, memData, memSize, baseDisplayAddress);
      }
      //}}}
    }
  //}}}

private:
  enum eDataFormat { DataFormat_Bin, DataFormat_Dec, DataFormat_Hex, DataFormat_COUNT };
  //{{{
  struct sSizes {
    int   mAddrDigitsCount;
    float mLineHeight;
    float mGlyphWidth;
    float mHexCellWidth;
    float mSpacingBetweenMidCols;
    float mPosHexStart;
    float mPosHexEnd;
    float mPosAsciiStart;
    float mPosAsciiEnd;
    float mWindowWidth;

    sSizes() { memset(this, 0, sizeof(*this)); }
    };
  //}}}

  //{{{
  bool isBigEndian() const {

    uint16_t x = 1;

    char c[2];
    memcpy (c, &x, 2);

    return c[0] != 0;
    }
  //}}}
  //{{{
  size_t getDataTypeSize (ImGuiDataType data_type) const {

    const size_t sizes[] = { 1, 1, 2, 2, 4, 4, 8, 8, sizeof(float), sizeof(double) };

    IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    return sizes[data_type];
    }
  //}}}
  //{{{
  const char* getDataTypeDesc (ImGuiDataType data_type) const {

    const char* descs[] = { "Int8", "Uint8", "Int16", "Uint16", "Int32",
                            "Uint32", "Int64", "Uint64", "Float", "Double" };

    IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    return descs[data_type];
    }
  //}}}
  //{{{
  const char* getDataFormatDesc (eDataFormat data_format) const {

    const char* descs[] = { "Bin", "Dec", "Hex" };

    IM_ASSERT(data_format >= 0 && data_format < DataFormat_COUNT);
    return descs[data_format];
    }
  //}}}

  //{{{
  void calcSizes (sSizes& s, size_t memSize, size_t baseDisplayAddress) {

    ImGuiStyle& style = ImGui::GetStyle();

    s.mAddrDigitsCount = mOptAddrDigitsCount;
    if (s.mAddrDigitsCount == 0)
      for (size_t n = baseDisplayAddress + memSize - 1; n > 0; n >>= 4)
        s.mAddrDigitsCount++;

    s.mLineHeight = ImGui::GetTextLineHeight();
    s.mGlyphWidth = ImGui::CalcTextSize ("F").x + 1;                  // We assume the font is mono-space
    s.mHexCellWidth = (float)(int)(s.mGlyphWidth * 2.5f);             // "FF " we include trailing space in the width to easily catch clicks everywhere
    s.mSpacingBetweenMidCols = (float)(int)(s.mHexCellWidth * 0.25f); // Every OptMidColsCount columns we add a bit of extra spacing
    s.mPosHexStart = (s.mAddrDigitsCount + 2) * s.mGlyphWidth;
    s.mPosHexEnd = s.mPosHexStart + (s.mHexCellWidth * mCols);
    s.mPosAsciiStart = s.mPosAsciiEnd = s.mPosHexEnd;

    if (mOptShowAscii) {
      s.mPosAsciiStart = s.mPosHexEnd + s.mGlyphWidth * 1;
      if (mOptMidColsCount > 0)
        s.mPosAsciiStart += (float)((mCols + mOptMidColsCount - 1) / mOptMidColsCount) * s.mSpacingBetweenMidCols;
      s.mPosAsciiEnd = s.mPosAsciiStart + mCols * s.mGlyphWidth;
      }

    s.mWindowWidth = s.mPosAsciiEnd + style.ScrollbarSize + style.WindowPadding.x * 2 + s.mGlyphWidth;
    }
  //}}}
  //{{{
  void* endianCopy (void* dst, void* src, size_t size) const {

    if (!gEndianFuncPtr)
      gEndianFuncPtr = isBigEndian() ? bigEndianCopy : littleEndianCopy;

    return gEndianFuncPtr (dst, src, size, mPreviewEndianess);
    }
  //}}}
  //{{{
  const char* formatBinary (const uint8_t* buf, int width) const {

    IM_ASSERT(width <= 64);

    static char out_buf[64 + 8 + 1];

    size_t out_n = 0;
    int n = width / 8;
    for (int j = n - 1; j >= 0; --j) {
      for (int i = 0; i < 8; ++i)
        out_buf[out_n++] = (buf[j] & (1 << (7 - i))) ? '1' : '0';
      out_buf[out_n++] = ' ';
      }

    IM_ASSERT(out_n < IM_ARRAYSIZE(out_buf));
    out_buf[out_n] = 0;
    return out_buf;
    }
  //}}}

  //{{{
  void drawOptionsLine (const sSizes& s, void* memData, size_t memSize, size_t baseDisplayAddress) {

    IM_UNUSED(memData);
    ImGuiStyle& style = ImGui::GetStyle();
    const char* format_range = mOptUpperCaseHex ? "Range %0*" _PRISizeT "X..%0*" _PRISizeT "X" : "Range %0*" _PRISizeT "x..%0*" _PRISizeT "x";

    // Options menu
    if (ImGui::Button ("Options"))
      ImGui::OpenPopup ("context");

    if (ImGui::BeginPopup ("context")) {
      ImGui::SetNextItemWidth (s.mGlyphWidth * 7 + style.FramePadding.x * 2.f);
      if (ImGui::DragInt ("##cols", &mCols, 0.2f, 4, 32, "%d cols")) {
        mContentsWidthChanged = true;
        if (mCols < 1)
          mCols = 1;
        }
      ImGui::Checkbox ("Show Data Preview", &mOptShowDataPreview);
      ImGui::Checkbox ("Show HexII", &mOptShowHexII);
      if (ImGui::Checkbox ("Show Ascii", &mOptShowAscii)) {
        mContentsWidthChanged = true;
        }
      ImGui::Checkbox ("Grey out zeroes", &mOptGreyOutZeroes);
      ImGui::Checkbox ("Uppercase Hex", &mOptUpperCaseHex);

      ImGui::EndPopup();
      }

    ImGui::SameLine();
    ImGui::Text (format_range, s.mAddrDigitsCount, baseDisplayAddress, s.mAddrDigitsCount, baseDisplayAddress + memSize - 1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth ((s.mAddrDigitsCount + 1) * s.mGlyphWidth + style.FramePadding.x * 2.f);
    if (ImGui::InputText ("##addr", mAddrInputBuf, IM_ARRAYSIZE(mAddrInputBuf),
                           ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
      size_t goto_addr;
      if (sscanf (mAddrInputBuf, "%" _PRISizeT "X", &goto_addr) == 1) {
        mGotoAddr = goto_addr - baseDisplayAddress;
        mHighlightMin = mHighlightMax = (size_t)-1;
        }
      }

    if (mGotoAddr != (size_t) - 1) {
      if (mGotoAddr < memSize) {
        ImGui::BeginChild ("##scrolling");
        ImGui::SetScrollFromPosY (ImGui::GetCursorStartPos().y + (mGotoAddr / mCols) * ImGui::GetTextLineHeight());
        ImGui::EndChild();
        mDataEditingAddr = mDataPreviewAddr = mGotoAddr;
        mDataEditingTakeFocus = true;
        }
      mGotoAddr = (size_t)-1;
      }
    }
  //}}}
  //{{{
  void drawPreviewLine (const sSizes& s, void* voidMemData, size_t memSize, size_t baseDisplayAddress) {

    IM_UNUSED(baseDisplayAddress);
    ImU8* memData = (ImU8*)voidMemData;

    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::AlignTextToFramePadding();
    ImGui::Text ("Preview as:");

    ImGui::SameLine();
    ImGui::SetNextItemWidth ((s.mGlyphWidth * 10.f) + style.FramePadding.x * 2.f + style.ItemInnerSpacing.x);
    if (ImGui::BeginCombo("##combo_type", getDataTypeDesc (mPreviewDataType), ImGuiComboFlags_HeightLargest)) {
      for (int n = 0; n < ImGuiDataType_COUNT; n++)
        if (ImGui::Selectable (getDataTypeDesc((ImGuiDataType)n), mPreviewDataType == n))
          mPreviewDataType = (ImGuiDataType)n;
      ImGui::EndCombo();
      }

    ImGui::SameLine();
    ImGui::SetNextItemWidth ((s.mGlyphWidth * 6.f) + style.FramePadding.x * 2.f + style.ItemInnerSpacing.x);
    ImGui::Combo ("##combo_endianess", &mPreviewEndianess, "LE\0BE\0\0");

    char buf[128] = "";
    float x = s.mGlyphWidth * 6.f;
    bool has_value = mDataPreviewAddr != (size_t)-1;
    if (has_value)
      drawPreviewData (mDataPreviewAddr, memData, memSize, mPreviewDataType, DataFormat_Dec, buf, (size_t)IM_ARRAYSIZE(buf));

    ImGui::Text("Dec");
    ImGui::SameLine(x);
    ImGui::TextUnformatted (has_value ? buf : "N/A");

    if (has_value)
      drawPreviewData (mDataPreviewAddr, memData, memSize, mPreviewDataType, DataFormat_Hex, buf, (size_t)IM_ARRAYSIZE(buf));

    ImGui::Text("Hex");
    ImGui::SameLine(x);
    ImGui::TextUnformatted (has_value ? buf : "N/A");

    if (has_value)
      drawPreviewData (mDataPreviewAddr, memData, memSize, mPreviewDataType, DataFormat_Bin, buf, (size_t)IM_ARRAYSIZE(buf));

    buf[IM_ARRAYSIZE (buf) - 1] = 0;
    ImGui::Text ("Bin");
    ImGui::SameLine (x);
    ImGui::TextUnformatted (has_value ? buf : "N/A");
    }
  //}}}
  //{{{
  void drawPreviewData (size_t addr, const ImU8* memData, size_t memSize, ImGuiDataType data_type,
                        eDataFormat data_format, char* out_buf, size_t out_buf_size) const {

    uint8_t buf[8];

    size_t elem_size = getDataTypeSize(data_type);
    size_t size = addr + elem_size > memSize ? memSize - addr : elem_size;
    if (mReadFn)
      for (int i = 0, n = (int)size; i < n; ++i)
        buf[i] = mReadFn(memData, addr + i);
    else
      memcpy(buf, memData + addr, size);

    if (data_format == DataFormat_Bin) {
      uint8_t binbuf[8];
      endianCopy (binbuf, buf, size);
      ImSnprintf (out_buf, out_buf_size, "%s", formatBinary(binbuf, (int)size * 8));
        return;
      }

    out_buf[0] = 0;
    switch (data_type) {
      //{{{
      case ImGuiDataType_S8: {
        int8_t int8 = 0;
        endianCopy (&int8, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%hhd", int8);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "0x%02x", int8 & 0xFF);
          return;
          }

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U8: {
        uint8_t uint8 = 0;
        endianCopy (&uint8, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%hhu", uint8);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "0x%02x", uint8 & 0XFF);
          return;
          }

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_S16: {
        int16_t int16 = 0;
        endianCopy (&int16, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%hd", int16);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "0x%04x", int16 & 0xFFFF);
          return;
          }

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U16: {
        uint16_t uint16 = 0;
        endianCopy (&uint16, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%hu", uint16);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "0x%04x", uint16 & 0xFFFF);
          return;
          }

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_S32: {
        int32_t int32 = 0;
        endianCopy (&int32, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%d", int32);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "0x%08x", int32);
          return;
          }

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U32: {
        uint32_t uint32 = 0;
        endianCopy (&uint32, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%u", uint32);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "0x%08x", uint32);
          return;
          }

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_S64: {
        int64_t int64 = 0;
        endianCopy (&int64, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%lld", (long long)int64);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)int64);
          return;
          }

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_U64: {
        uint64_t uint64 = 0;
        endianCopy (&uint64, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%llu", (long long)uint64);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)uint64);
          return;
          }

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_Float: {
        float float32 = 0.f;
        endianCopy (&float32, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%f", float32);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "%a", float32);
          return;
          }

        break;
        }
      //}}}
      //{{{
      case ImGuiDataType_Double: {
        double float64 = 0.0;
        endianCopy (&float64, buf, size);

        if (data_format == DataFormat_Dec) {
          ImSnprintf(out_buf, out_buf_size, "%f", float64);
          return;
          }

        if (data_format == DataFormat_Hex) {
          ImSnprintf(out_buf, out_buf_size, "%a", float64);
          return;
          }

        break;
        }
      //}}}
      case ImGuiDataType_COUNT:
        break;
      }

    IM_ASSERT(0); // Shouldn't reach
    }
  //}}}

  //{{{
  void gotoAddrAndHighlight (size_t addr_min, size_t addr_max) {

    mGotoAddr = addr_min;
    mHighlightMin = addr_min;
    mHighlightMax = addr_max;
    }
  //}}}

  // settings
  bool  mOpen = true;  // set to false when DrawWindow() was closed. ignore if not using DrawWindow().
  bool  mReadOnly = false;
  int   mCols = 16;
  bool  mOptShowOptions = true;      // display options button/context menu. when disabled, options will be locked unless you provide your own UI for them.
  bool  mOptShowDataPreview = false; // display a footer previewing the decimal/binary/hex/float representation of the currently selected bytes.
  bool  mOptShowHexII = false;       // display values in HexII representation instead of regular hexadecimal: hide null/zero bytes, ascii values as ".X".
  bool  mOptShowAscii = true;        // display ASCII representation on the right side.
  bool  mOptGreyOutZeroes =true;     // display null/zero bytes using the TextDisabled color.
  bool  mOptUpperCaseHex = true;     // display hexadecimal values as "FF" instead of "ff".
  int   mOptMidColsCount = 8; //     // set to 0 to disable extra spacing between every mid-cols.
  int   mOptAddrDigitsCount= 0;      // number of addr digits to display (default calculated based on maximum displayed addr).
  float mOptFooterExtraHeight = 0;   // space to reserve at the bottom of the widget to add custom widgets
  ImU32 mHighlightColor = IM_COL32(255, 255, 255, 50);  // background color of highlighted bytes.
  ImU8  (*mReadFn)(const ImU8* data, size_t off) = nullptr;      // optional handler to read bytes.
  void  (*mWriteFn)(ImU8* data, size_t off, ImU8 d) = nullptr;   // optional handler to write bytes.
  bool  (*mHighlightFn)(const ImU8* data, size_t off) = nullptr; // optional handler to return Highlight property (to support non-contiguous highlighting).

  // state/internals
  bool    mContentsWidthChanged = false;
  size_t  mDataPreviewAddr = (size_t)-1;
  size_t  mDataEditingAddr = (size_t)-1;
  bool    mDataEditingTakeFocus = false;
  char    mDataInputBuf[32] = {0};
  char    mAddrInputBuf[32] = {0};
  size_t  mGotoAddr = (size_t)-1;
  size_t  mHighlightMin = (size_t)-1;
  size_t  mHighlightMax = (size_t)-1;
  int     mPreviewEndianess = 0;
  ImGuiDataType mPreviewDataType = ImGuiDataType_S32;
  };

#undef _PRISizeT
#undef ImSnprintf
#ifdef _MSC_VER
  #pragma warning (pop)
#endif
