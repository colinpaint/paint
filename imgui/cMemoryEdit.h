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

  void drawWindow (const std::string& title, uint8_t* memData, size_t memSize, size_t baseAddress);
  void drawContents (uint8_t* memData, size_t memSize, size_t baseAddress);
  void gotoAddrAndHighlight (size_t addrMin, size_t addrMax);

private:
  enum class eDataFormat { eBin, eDec, eHex, eMax };
  //{{{
  class cContext {
  public:
    int mAddrDigitsCount = 0;

    float mLineHeight = 0;

    float mGlyphWidth = 0;
    float mHexCellWidth = 0;
    float mSpacingBetweenMidCols = 0;

    float mHexBeginPos = 0;
    float mHexEndPos = 0;

    float mAsciiBeginPos = 0;
    float mAsciiEndPos = 0;

    float mWindowWidth = 0;

    ImU32 mTextColor;
    ImU32 mGreyColor;
    ImU32 mHighlightColor;
    };
  //}}}

  // gets
  bool isCpuBigEndian() const;
  bool isReadOnly() const { return mReadOnly; };
  size_t getDataTypeSize (ImGuiDataType dataType) const;
  const char* getDataTypeDesc (ImGuiDataType dataType) const;
  const char* getDataFormatDesc (eDataFormat dataFormat) const;
  const char* getDataStr (size_t addr, const uint8_t* memData, size_t memSize,
                          ImGuiDataType dataType, eDataFormat dataFormat);

  void setReadOnly (bool readOnly) { mReadOnly = readOnly; }
  void toggleReadOnly() { mReadOnly = !mReadOnly; }

  void setContext (size_t memSize, size_t baseAddress);
  void* endianCopy (void* dst, void* src, size_t size);

  // draws
  void drawHeader (uint8_t* memData, size_t memSize, size_t baseAddress);
  void drawLine (int lineNumber);

  //{{{  vars
  // settings
  bool mOpen = true;                // set false when DrawWindow() closed
  bool mReadOnly = false;

  cContext mContext;

  uint8_t* mMemData = nullptr;
  size_t mMemSize = 0;
  size_t mBaseAddress = 0;

  // gui options
  int mAddrDigitsCount= 0;      // number of addr digits to display (default calculated based on maximum displayed addr).
  int mColumns = 16;
  bool mShowHexII = false;
  bool mHoverHexII = false;
  bool mBigEndian = false;
  bool mHoverEndian = false;

  // gui options removed
  int mMidColsCount = 8;         // set to 0 to disable extra spacing between every mid-cols.

  size_t mDataAddress = (size_t)-1;
  size_t mDataEditingAddr = (size_t)-1;
  bool mDataNext = false;
  size_t mDataEditingAddrNext = (size_t)-1;
  bool mDataEditingTakeFocus = false;

  char mDataInputBuf[32] = {0};
  char mAddrInputBuf[32] = {0};
  size_t mGotoAddr = (size_t)-1;

  size_t mHighlightMin = (size_t)-1;
  size_t mHighlightMax = (size_t)-1;

  ImGuiDataType mPreviewDataType = ImGuiDataType_U8;

  char mOutBuf [128] = { 0 };
  //}}}
  };
