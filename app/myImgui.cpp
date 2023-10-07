// myImgui - my imgui stuff
//{{{  includes
#include "myImgui.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

// imGui
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include "fmt/core.h" //  fmt::format core, used by a lot of logging
#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/cMiniLog.h"

using namespace std;
//}}}

//{{{
void printHex (uint8_t* ptr, uint32_t numBytes, uint32_t columnsPerRow, uint32_t address, bool full) {

  uint32_t firstRowPad = address % columnsPerRow;
  address -= firstRowPad;

  while (numBytes > 0) {
    string hexString;
    if (full)
      hexString = fmt::format ("{:04x}: ", address);
    string asciiString;

    // pad leading row values
    uint32_t byteInRow = 0;
    while (firstRowPad > 0) {
      hexString += "   ";
      asciiString += " ";
      byteInRow++;
      firstRowPad--;
      }

    // rest of row
    while (byteInRow < columnsPerRow) {
      if (numBytes > 0) {
        // append byte
        numBytes--;
        uint8_t value = *ptr++;
        hexString += fmt::format ("{:02x} ", value);
        asciiString += (value > 0x20) && (value < 0x80) ? value : '.';
        }
      else // pad trailing row values
        hexString += "   ";

      byteInRow++;
      }

    if (full)
      ImGui::Text ("%s %s", hexString.c_str(), asciiString.c_str());
    else
      ImGui::Text ("%s", hexString.c_str());

    address += columnsPerRow;
    }
  }
//}}}

//{{{
void drawMiniLog (cMiniLog& miniLog) {

  if (miniLog.getEnable()) {
    if (ImGui::Button (miniLog.getHeader().c_str()))
      miniLog.clear();

    bool filtered = false;
    for (auto& tag : miniLog.getTags()) {
      ImGui::SameLine();
      if (toggleButton (tag.mName.c_str(), tag.mEnable))
        tag.mEnable = !tag.mEnable;
      filtered |= tag.mEnable;
      }

    // draw log
    ImGui::BeginChild (fmt::format ("##log{}", miniLog.getName()).c_str(),
                       {ImGui::GetWindowWidth(), 12 * ImGui::GetTextLineHeight() }, true,
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);

    if (filtered) {
      // tag filtered draw
      for (auto& line : miniLog.getLines())
        if (miniLog.getTags()[line.mTagIndex].mEnable)
          ImGui::TextUnformatted ((date::format ("%M.%S ", line.mTimeStamp) +
                                  miniLog.getTags()[line.mTagIndex].mName + " " + line.mText).c_str());
      }
    else {
      // simple unfiltered draw, can use imgui ImGuiListClipper
      for (auto& line : miniLog.getLines())
        ImGui::TextUnformatted ((date::format ("%M.%S ", line.mTimeStamp) +
                                miniLog.getTags()[line.mTagIndex].mName + " " + line.mText).c_str());
      }

    // autoScroll child
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
      ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    if (!miniLog.getFooter().empty())
      ImGui::TextUnformatted (miniLog.getFooter().c_str());
    }
  }
//}}}

//{{{
bool clockButton (const string& label, chrono::system_clock::time_point timePoint, const ImVec2& size_arg) {

  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  ImGuiContext& g = *GImGui;
  const ImGuiStyle& style = g.Style;
  //const ImVec2 label_size = ImGui::CalcTextSize (label.c_str(), NULL, true);
  const ImVec2 size = ImGui::CalcItemSize (size_arg, -2.f * style.FramePadding.x, -2.0f * style.FramePadding.y);

  const ImVec2 pos = window->DC.CursorPos;
  const ImRect rect (pos, pos + size);
  ImGui::ItemSize (size, style.FramePadding.y);

  const ImGuiID id = window->GetID (label.c_str());
  if (!ImGui::ItemAdd (rect, id))
    return false;

  bool hovered;
  bool held;
  bool pressed = ImGui::ButtonBehavior (rect, id, &hovered, &held, ImGuiButtonFlags_None);

  const auto datePoint = floor<date::days>(timePoint);
  const auto timeOfDay = date::make_time (chrono::duration_cast<chrono::seconds>(timePoint - datePoint));
  const int hours = timeOfDay.hours().count() % 12;
  const int mins = timeOfDay.minutes().count();
  const int secs = static_cast<int>(timeOfDay.seconds().count());

  // draw face circle
  const float kPi = 3.1415926f;
  const float radius = rect.GetWidth() / 2.f;
  const ImU32 col = (held || hovered) ? ImGui::GetColorU32 (ImGuiCol_ButtonHovered) : IM_COL32_WHITE;
  window->DrawList->AddCircle (rect.GetCenter(), radius, col, 0, 3.f);

  // draw hourHand
  const float hourRadius = radius * 0.6f;
  const float hourAngle = (1.f - (hours + (mins / 60.f)) / 6.f) * kPi;
  window->DrawList->AddLine (
    rect.GetCenter(), rect.GetCenter() + ImVec2(hourRadius * sin (hourAngle), hourRadius * cos (hourAngle)), IM_COL32_WHITE, 2.f);

  // draw minuteHand
  const float minRadius = radius * 0.75f;
  const float minAngle = (1.f - (mins + (secs / 60.f)) / 30.f) * kPi;
  window->DrawList->AddLine (
    rect.GetCenter(), rect.GetCenter() + ImVec2(minRadius * sin (minAngle), minRadius * cos (minAngle)), IM_COL32_WHITE, 2.f);

  // draw secondHand
  const float secRadius = radius * 0.85f;
  const float secAngle = (1.f - (secs / 30.f)) * kPi;
  window->DrawList->AddLine (
    rect.GetCenter(), rect.GetCenter() + ImVec2(secRadius * sin (secAngle), secRadius * cos (secAngle)), IM_COL32_WHITE, 2.f);

  // draw date,time labels
  const string dateString = date::format ("%a %d %b %y", chrono::floor<chrono::seconds>(timePoint));
  window->DrawList->AddText (rect.GetBL() - ImVec2(0.f,16.f), col, dateString.c_str(), NULL);
  const string timeString = date::format ("%H:%M:%S", chrono::floor<chrono::seconds>(timePoint));
  window->DrawList->AddText (rect.GetBL(), col, timeString.c_str(), NULL);

  IMGUI_TEST_ENGINE_ITEM_INFO(id, label.c_str(), g.LastItemData.StatusFlags);
  return pressed;
  }
//}}}
//{{{
bool toggleButton (const string& label, bool toggleOn, const ImVec2& size_arg) {
// imGui custom widget - based on ImGui::ButtonEx

  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  ImGuiContext& g = *GImGui;
  const ImGuiStyle& style = g.Style;

  const ImVec2 label_size = ImGui::CalcTextSize (label.c_str(), NULL, true);
  const ImVec2 size = ImGui::CalcItemSize (
    size_arg, label_size.x + style.FramePadding.x * 2.f, label_size.y + style.FramePadding.y * 2.f);

  const ImVec2 pos = window->DC.CursorPos;
  const ImRect rect (pos, pos + size);
  ImGui::ItemSize (size, style.FramePadding.y);

  const ImGuiID id = window->GetID (label.c_str());
  if (!ImGui::ItemAdd (rect, id))
    return false;

  bool hovered;
  bool held;
  bool pressed = ImGui::ButtonBehavior (rect, id, &hovered, &held, ImGuiButtonFlags_None);

  // Render
  const ImU32 col = ImGui::GetColorU32 (
    toggleOn || (held && hovered) ? ImGuiCol_ButtonActive :
                                    (hovered ? ImGuiCol_ButtonHovered : ImGuiCol_FrameBg));
  ImGui::RenderNavHighlight (rect, id);
  ImGui::RenderFrame (rect.Min, rect.Max, col, true, style.FrameRounding);

  if (g.LogEnabled)
    ImGui::LogSetNextTextDecoration ("[", "]");

  ImGui::RenderTextClipped (rect.Min + style.FramePadding, rect.Max - style.FramePadding,
                            label.c_str(), NULL, &label_size,
                            style.ButtonTextAlign, &rect);

  IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);

  return pressed;
  }
//}}}
//{{{
uint8_t interlockedButtons (const vector<string>& buttonVector, uint8_t index,
                            const ImVec2& size_arg, bool tabbed) {
// interlockedButtons helper
// draw buttonVector as toggleButtons with index toggled on
// return index of last or pressed menu button

  ImGui::BeginGroup();

  for (auto it = buttonVector.begin(); it != buttonVector.end(); ++it) {
    if (toggleButton (*it, index == static_cast<uint8_t>(it - buttonVector.begin()), size_arg))
      index = static_cast<uint8_t>(it - buttonVector.begin());
    if (tabbed)
      ImGui::SameLine();
    }

  ImGui::EndGroup();

  return index;
  }
//}}}
