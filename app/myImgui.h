// myImgui.h - my imgui widgets - freestanding routines like ImGui::Button but with string and other stuff
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <chrono>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

class cMiniLog;

//{{{
class cDrawContext {
// helper class for drawing text, rect, circle with palette color
public:
  cDrawContext() = default;
  ~cDrawContext() = default;
  cDrawContext (const std::vector <ImU32>& palette) : mPalette(palette) {}

  // gets
  float getFontSize() const { return mFontSize; }
  float getGlyphWidth() const { return mGlyphWidth; }
  float getLineHeight() const { return mLineHeight; }
  ImU32 getColor (uint8_t color) const { return mPalette[color]; }

  //{{{
  void update (float fontSize, bool monoSpaced) {
  // update draw context

    mDrawList = ImGui::GetWindowDrawList();

    mFont = ImGui::GetFont();
    mMonoSpaced = monoSpaced;

    mFontAtlasSize = ImGui::GetFontSize();
    mFontSize = fontSize;
    mFontSmallSize = mFontSize > ((mFontAtlasSize * 2.f) / 3.f) ? ((mFontAtlasSize * 2.f) / 3.f) : mFontSize;
    mFontSmallOffset = ((mFontSize - mFontSmallSize) * 2.f) / 3.f;

    float scale = mFontSize / mFontAtlasSize;
    mLineHeight = ImGui::GetTextLineHeight() * scale;
    mGlyphWidth = measureSpace();
    }
  //}}}

  // measure text
  //{{{
  float measureSpace() const {
  // return width of space

    char str[2] = {' ', 0};
    return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, str+1).x;
    }
  //}}}
  //{{{
  float measureChar (char ch) const {
  // return width of text

    if (mMonoSpaced)
      return mGlyphWidth;
    else {
      char str[2] = {ch,0};
      return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, str+1).x;
      }
    }
  //}}}
  //{{{
  float measureText (const std::string& text) const {
    return mMonoSpaced ? mGlyphWidth * static_cast<uint32_t>(text.size())
                       : mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, text.c_str()).x;
    }
  //}}}
  //{{{
  float measureText (const char* str, const char* strEnd) const {
    return mMonoSpaced ? mGlyphWidth * static_cast<uint32_t>(strEnd - str)
                       : mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
    }
  //}}}
  //{{{
  float measureSmallText (const std::string& text) const {
    return mMonoSpaced ? mGlyphWidth * (mFontSmallSize / mFontSize) * static_cast<uint32_t>(text.size())
                       : mFont->CalcTextSizeA (mFontSmallSize, FLT_MAX, -1.f, text.c_str()).x;
    }
  //}}}

  // text
  //{{{
  float text (const ImVec2& pos, uint8_t color, const std::string& text) {
   // draw and return width of text

    mDrawList->AddText (mFont, mFontSize, pos, mPalette[color], text.c_str());

    if (mMonoSpaced)
      return text.size() * mGlyphWidth;
    else
      return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, text.c_str()).x;
    }
  //}}}
  //{{{
  float text (const ImVec2& pos, uint8_t color, const char* str, const char* strEnd) {
   // draw and return width of text

    mDrawList->AddText (mFont, mFontSize, pos, mPalette[color], str, strEnd);

    if (mMonoSpaced)
      return measureText (str, strEnd);
    else
      return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
    }
  //}}}
  //{{{
  float smallText (const ImVec2& pos, uint8_t color, const std::string& text) {
   // draw and return width of small text

    ImVec2 offsetPos = pos;
    offsetPos.y += mFontSmallOffset;
    mDrawList->AddText (mFont, mFontSmallSize, offsetPos, mPalette[color], text.c_str(), nullptr);

    return mFont->CalcTextSizeA (mFontSmallSize, FLT_MAX, -1.f, text.c_str(), nullptr).x;
    }
  //}}}

  // graphics
  //{{{
  void line (const ImVec2& pos1, const ImVec2& pos2, uint8_t color) {
    mDrawList->AddLine (pos1, pos2, mPalette[color]);
    }
  //}}}
  //{{{
  void circle (const ImVec2& centre, float radius, uint8_t color) {
    mDrawList->AddCircleFilled (centre, radius, mPalette[color], 4);
    }
  //}}}
  //{{{
  void rect (const ImVec2& pos1, const ImVec2& pos2, uint8_t color) {
    mDrawList->AddRectFilled (pos1, pos2, mPalette[color]);
    }
  //}}}
  //{{{
  void rectLine (const ImVec2& pos1, const ImVec2& pos2, uint8_t color) {
    mDrawList->AddRect (pos1, pos2, mPalette[color], 1.f);
    }
  //}}}

private:
  ImDrawList* mDrawList = nullptr;

  ImFont* mFont = nullptr;
  bool mMonoSpaced = false;

  float mFontAtlasSize = 0.f;
  float mFontSmallSize = 0.f;
  float mFontSmallOffset = 0.f;

  float mFontSize = 0.f;
  float mGlyphWidth = 0.f;
  float mLineHeight = 0.f;

  const std::vector <ImU32> mPalette;
  };
//}}}

// miniLog
void drawMiniLog (cMiniLog& miniLog);
void printHex (uint8_t* ptr, uint32_t numBytes, uint32_t columnsPerRow, uint32_t address, bool full);

// button helpers
bool clockButton (const std::string& label, std::chrono::system_clock::time_point timePoint,
                  const ImVec2& size = ImVec2(0,0), bool drawDate = false, bool drawTime = false);

bool toggleButton (const std::string& label, bool toggleOn, const ImVec2& size = ImVec2(0,0));

bool groupButton (bool oneOnly,
                  const std::vector<std::string>& buttons, uint8_t& buttonIndex,
                  const ImVec2& size = ImVec2(0,0), bool horizontalLayout = false);

bool maxOneButton (const std::vector<std::string>& buttons, uint8_t& buttonIndex,
                   const ImVec2& size = ImVec2(0,0), bool horizontalLayout = false);

bool oneOnlyButton (const std::vector<std::string>& buttons, uint8_t& buttonIndex,
                    const ImVec2& size = ImVec2(0,0), bool horizontalLayout = false);
