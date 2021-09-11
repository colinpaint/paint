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
  void drawWindow (const std::string& title, uint8_t* memData, size_t memSize, size_t baseAddress);
  void drawContents (uint8_t* memData, size_t memSize, size_t baseAddress);

  void setAddrHighlight (size_t addrMin, size_t addrMax);

private:
  enum class eDataFormat { eDec, eBin, eHex };
  //{{{
  class cInfo {
  public:
    //{{{
    cInfo (uint8_t* memData, size_t memSize, size_t baseAddress)
      : mMemData(memData), mMemSize(memSize), mBaseAddress(baseAddress) {}
    //}}}

    uint8_t* mMemData = nullptr;
    const size_t mMemSize = 0;
    const size_t mBaseAddress = 0;
    };
  //}}}
  //{{{
  class cContext {
  public:
    float mGlyphWidth = 0;
    float mLineHeight = 0;

    int mAddrDigitsCount = 0;

    float mHexCellWidth = 0;
    float mExtraSpaceWidth = 0;
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
  //{{{
  class cEdit {
  public:
    size_t mDataAddress = (size_t)-1;
    bool mDataNext = false;

    size_t mDataEditingAddr = (size_t)-1;
    size_t mDataEditingAddrNext = (size_t)-1;
    bool mDataEditingTakeFocus = false;

    char mDataInputBuf[32] = {0};
    char mAddrInputBuf[32] = {0};
    size_t mGotoAddr = (size_t)-1;

    size_t mHighlightMin = (size_t)-1;
    size_t mHighlightMax = (size_t)-1;
    };
  //}}}
  //{{{
  class cOptions {
  public:
    bool mReadOnly = false;

    int mColumns = 16;
    int mColumnExtraSpace = 8;

    bool mShowHexII = false;
    bool mHoverHexII = false;

    bool mBigEndian = false;
    bool mHoverEndian = false;

    ImGuiDataType mPreviewDataType = ImGuiDataType_U8;
    };
  //}}}

  // gets
  bool isCpuBigEndian() const;
  bool isReadOnly() const { return mOptions.mReadOnly; };

  size_t getDataTypeSize (ImGuiDataType dataType) const;
  std::string getDataTypeDesc (ImGuiDataType dataType) const;
  std::string getDataFormatDesc (eDataFormat dataFormat) const;
  std::string getDataStr (size_t addr, const cInfo& info, ImGuiDataType dataType, eDataFormat dataFormat);

  // sets
  void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }
  void toggleReadOnly() { mOptions.mReadOnly = !mOptions.mReadOnly; }

  void setContext (const cInfo& info, cContext& context);
  void* copyEndian (void* dst, void* src, size_t size);

  // draws
  void drawTop (const cInfo& info, const cContext& context);
  void drawLine (int lineNumber, const cInfo& info, const cContext& context);

  // vars
  bool mOpen = true;  // set false when DrawWindow() closed

  cEdit mEdit;
  cOptions mOptions;

  char mOutBuf [128] = { 0 };
  };
