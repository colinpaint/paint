// cMemoryEdit.h - hacked version of https://github.com/ocornut/imgui_club
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

// imgui
#include "../imgui/imgui.h"
//}}}

class cMemoryEdit {
public:
  cMemoryEdit() = default;
  ~cMemoryEdit() = default;

  void drawWindow (const std::string& title, void* voidMemData, size_t memSize, size_t baseDisplayAddress);
  void drawContents (void* voidMemData, size_t memSize, size_t baseDisplayAddress);

private:
  enum eDataFormat { eDataFormatBin, eDataFormatDec, eDataFormatHex, eDataFormat_COUNT };
  //{{{
  class cSizes {
  public:
    int   mAddrDigitsCount = 0;
    float mLineHeight = 0;
    float mGlyphWidth = 0;
    float mHexCellWidth = 0;
    float mSpacingBetweenMidCols = 0;
    float mPosHexStart = 0;
    float mPosHexEnd = 0;
    float mPosAsciiStart = 0;
    float mPosAsciiEnd = 0;
    float mWindowWidth = 0;

    cSizes() = default;
    };
  //}}}

  // gets
  bool isBigEndian() const;
  size_t getDataTypeSize (ImGuiDataType dataType) const;
  const char* getDataTypeDesc (ImGuiDataType dataType) const;
  const char* getDataFormatDesc (eDataFormat dataFormat) const;

  void calcSizes (cSizes& sizes, size_t memSize, size_t baseDisplayAddress);
  void* endianCopy (void* dst, void* src, size_t size) const;
  void gotoAddrAndHighlight (size_t addrMin, size_t addrMax);

  // draws
  void drawOptionsLine (const cSizes& sizes, void* memData, size_t memSize, size_t baseDisplayAddress);
  void drawPreviewLine (const cSizes& sizes, void* voidMemData, size_t memSize, size_t baseDisplayAddress);
  void drawPreviewData (size_t addr, const ImU8* memData, size_t memSize, ImGuiDataType dataType,
                        eDataFormat dataFormat, char* out_buf, size_t out_buf_size);
  //{{{  vars
  // settings
  bool  mOpen = true;  // set to false when DrawWindow() was closed. ignore if not using DrawWindow().
  bool  mReadOnly = false;
  int   mCols = 16;
  bool  mOptShowDataPreview = false; // display a footer previewing the decimal/binary/hex/float representation of the currently selected bytes.
  bool  mOptShowHexII = false;       // display values in HexII representation instead of regular hexadecimal: hide null/zero bytes, ascii values as ".X".
  bool  mOptShowAscii = true;        // display ASCII representation on the right side.
  bool  mOptGreyOutZeroes =true;     // display null/zero bytes using the TextDisabled color.
  bool  mOptUpperCaseHex = true;     // display hexadecimal values as "FF" instead of "ff".
  int   mOptMidColsCount = 8; //     // set to 0 to disable extra spacing between every mid-cols.
  int   mOptAddrDigitsCount= 0;      // number of addr digits to display (default calculated based on maximum displayed addr).
  float mOptFooterExtraHeight = 0;   // space to reserve at the bottom of the widget to add custom widgets
  ImU32 mHighlightColor = IM_COL32(255, 255, 255, 50);  // background color of highlighted bytes.

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
  //}}}
  };
