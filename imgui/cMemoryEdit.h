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

  void drawWindow (const std::string& title, uint8_t* memData, size_t memSize, size_t baseDisplayAddress);
  void drawContents (uint8_t* memData, size_t memSize, size_t baseDisplayAddress);

private:
  enum class eDataFormat { eBin, eDec, eHex, eMax };
  //{{{
  class cSizes {
  public:
    int mAddrDigitsCount = 0;

    float mLineHeight = 0;

    float mGlyphWidth = 0;
    float mHexCellWidth = 0;
    float mSpacingBetweenMidCols = 0;

    float mPosHexStart = 0;
    float mPosHexEnd = 0;

    float mPosAsciiStart = 0;
    float mPosAsciiEnd = 0;

    float mWindowWidth = 0;
    };
  //}}}

  // gets
  bool isBigEndian() const;
  bool isReadOnly() const { return mReadOnly; };
  size_t getDataTypeSize (ImGuiDataType dataType) const;
  const char* getDataTypeDesc (ImGuiDataType dataType) const;
  const char* getDataFormatDesc (eDataFormat dataFormat) const;
  const char* getDataStr (size_t addr, const uint8_t* memData, size_t memSize,
                          ImGuiDataType dataType, eDataFormat dataFormat);

  void setReadOnly (bool readOnly) { mReadOnly = readOnly; }
  void toggleReadOnly() { mReadOnly = !mReadOnly; }

  void calcSizes (cSizes& sizes, size_t memSize, size_t baseDisplayAddress);
  void* endianCopy (void* dst, void* src, size_t size);
  void gotoAddrAndHighlight (size_t addrMin, size_t addrMax);

  // draws
  void drawHeader (const cSizes& sizes, uint8_t* memData, size_t memSize, size_t baseDisplayAddress);

  //{{{  vars
  // settings
  bool mOpen = true;                // set false when DrawWindow() closed
  bool mReadOnly = false;

  // gui options
  int  mOptAddrDigitsCount= 0;      // number of addr digits to display (default calculated based on maximum displayed addr).
  int mCols = 16;
  bool mOptShowHexII = false;       // display values in HexII representation instead of regular hexadecimal: hide null/zero bytes, ascii values as ".X".
  bool mOptShowDataPreview = false; // display preview the decimal/binary/hex/float representation of the currently selected bytes.

  // gui options removed
  int mOptMidColsCount = 8;         // set to 0 to disable extra spacing between every mid-cols.
  bool mOptGreyOutZeroes = true;    // display null/zero bytes using the TextDisabled color.
  bool mOptUpperCaseHex = false;    // display hexadecimal values as "FF" instead of "ff".
  bool mOptShowAscii = true;        // display ASCII representation on the right side.

  ImU32 mHighlightColor = IM_COL32(255, 255, 255, 50);  // background color of highlighted bytes.

  size_t mDataAddress = (size_t)-1;
  size_t mDataEditingAddr = (size_t)-1;

  bool mDataEditingTakeFocus = false;

  char mDataInputBuf[32] = {0};
  char mAddrInputBuf[32] = {0};
  size_t mGotoAddr = (size_t)-1;

  size_t mHighlightMin = (size_t)-1;
  size_t mHighlightMax = (size_t)-1;

  ImGuiDataType mPreviewDataType = ImGuiDataType_S32;
  int mPreviewEndianess = 0;

  char mOutBuf [128] = { 0 };

  void* (*mEndianFunc)(void*, void*, size_t, int) = nullptr;
  //}}}
  };
