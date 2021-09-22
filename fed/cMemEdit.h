// cMemEdit.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

// imgui
#include "../imgui/imgui.h"
//}}}

class cMemEdit {
public:
  cMemEdit (uint8_t* memData, size_t memSize);
  ~cMemEdit() = default;

  void drawWindow (const std::string& title, size_t baseAddress);
  void drawContents (size_t baseAddress);

  // set
  void setMem (uint8_t* memData, size_t memSize);
  void setAddressHighlight (size_t addressMin, size_t addressMax);

  // actions
  void moveLeft();
  void moveRight();
  void moveLineUp();
  void moveLineDown();
  void movePageUp();
  void movePageDown();
  void moveHome();
  void moveEnd();

private:
  enum class eDataFormat { eDec, eBin, eHex };
  static const size_t kUndefinedAddress = (size_t)-1;
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

    ImGuiDataType mDataType = ImGuiDataType_U8;
    };
  //}}}
  //{{{
  class cInfo {
  public:
    //{{{
    cInfo (uint8_t* memData, size_t memSize)
      : mMemData(memData), mMemSize(memSize), mBaseAddress(kUndefinedAddress) {}
    //}}}
    //{{{
    void setBaseAddress (size_t baseAddress) {
      mBaseAddress = baseAddress;
      }
    //}}}

    // vars
    uint8_t* mMemData = nullptr;
    size_t mMemSize = kUndefinedAddress;
    size_t mBaseAddress = kUndefinedAddress;
    };
  //}}}
  //{{{
  class cContext {
  public:
    void update (const cOptions& options, const cInfo& info);

    // vars
    float mGlyphWidth = 0;
    float mLineHeight = 0;

    int mAddressDigitsCount = 0;

    float mHexCellWidth = 0;
    float mExtraSpaceWidth = 0;
    float mHexBeginPos = 0;
    float mHexEndPos = 0;

    float mAsciiBeginPos = 0;
    float mAsciiEndPos = 0;

    float mWindowWidth = 0;
    int mNumPageLines = 0;

    int mNumLines = 0;

    ImU32 mTextColor;
    ImU32 mGreyColor;
    ImU32 mHighlightColor;
    ImU32 mFrameBgndColor;
    ImU32 mTextSelectBgndColor;
    };
  //}}}
  //{{{
  class cEdit {
  public:
    //{{{
    void begin (const cOptions& options, const cInfo& info) {

      mDataNext = false;

      if (mDataAddress >= info.mMemSize)
        mDataAddress = kUndefinedAddress;

      if (options.mReadOnly || (mEditAddress >= info.mMemSize))
        mEditAddress = kUndefinedAddress;

      mNextEditAddress = kUndefinedAddress;
      }
    //}}}

    bool mDataNext = false;
    size_t mDataAddress = kUndefinedAddress;

    bool mEditTakeFocus = false;
    size_t mEditAddress = kUndefinedAddress;
    size_t mNextEditAddress = kUndefinedAddress;

    char mDataInputBuf[32] = { 0 };
    char mAddressInputBuf[32] = { 0 };
    size_t mGotoAddress = kUndefinedAddress;

    size_t mHighlightMin = kUndefinedAddress;
    size_t mHighlightMax = kUndefinedAddress;
    };
  //}}}

  // gets
  bool isCpuBigEndian() const;
  bool isReadOnly() const { return mOptions.mReadOnly; };
  bool isValid (size_t address) { return address != kUndefinedAddress; }

  size_t getDataTypeSize (ImGuiDataType dataType) const;
  std::string getDataTypeDesc (ImGuiDataType dataType) const;
  std::string getDataFormatDesc (eDataFormat dataFormat) const;
  std::string getDataString (size_t address, ImGuiDataType dataType, eDataFormat dataFormat);

  // sets
  void setReadOnly (bool readOnly) { mOptions.mReadOnly = readOnly; }
  void toggleReadOnly() { mOptions.mReadOnly = !mOptions.mReadOnly; }

  void initContext();
  void initEdit();

  void* copyEndian (void* dst, void* src, size_t size);

  void handleKeyboard();

  // draws
  void drawTop();
  void drawLine (int lineNumber);

  // vars
  bool mOpen = true;  // set false when DrawWindow() closed

  cInfo mInfo;
  cContext mContext;
  cEdit mEdit;
  cOptions mOptions;

  uint64_t mStartTime;
  float mLastClick;
  };
