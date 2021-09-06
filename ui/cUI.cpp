// cUI.cpp - static manager and UI base class
//{{{  includes
#include "cUI.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// imGui
#include <imgui.h>

// imGui - internal, exposed for custom widget
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "../utils/cLog.h"
#include "../graphics/cGraphics.h"
#include "../brush/cBrush.h"
#include "../canvas/cCanvas.h"

using namespace std;
//}}}
#define DRAW_CANVAS // useful to disable canvas when bringing up backends
#define SHOW_DEMO

// static register
//{{{
cUI* cUI::createByName (const string& name) {
// create class by name from classRegister, add instance to instances

  auto uiIt = getInstanceRegister().find (name);
  if (uiIt == getInstanceRegister().end()) {
    auto ui = getClassRegister()[name](name);
    getInstanceRegister().insert (make_pair (name, ui));
    return ui;
    }
  else
    return uiIt->second;
  }
//}}}
//{{{
void cUI::listRegisteredClasses() {

  cLog::log (LOGINFO, "ui register");
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, fmt::format ("- {}", ui.first));
  }
//}}}
//{{{
void cUI::listInstances() {

  cLog::log (LOGINFO, "ui instance register");
  for (auto& ui : getInstanceRegister())
    cLog::log (LOGINFO, fmt::format ("- {}", ui.first));
  }
//}}}

// static draw
//{{{
void cUI::draw (cApp* app) {
// draw registered UI instances

  for (auto& ui : getInstanceRegister())
    ui.second->addToDrawList (app);
  }
//}}}

// protected:
//{{{
bool cUI::clockButton (const string& label, chrono::system_clock::time_point timePoint, const ImVec2& size_arg) {

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
bool cUI::toggleButton (const string& label, bool toggleOn, const ImVec2& size_arg) {
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
unsigned cUI::interlockedButtons (const vector<string>& buttonVector, unsigned index, const ImVec2& size_arg) {
// interlockedButtons helper
// draw buttonVector as toggleButtons with index toggled on
// return index of last or pressed menu button

  ImGui::BeginGroup();

  for (auto it = buttonVector.begin(); it != buttonVector.end(); ++it)
    if (toggleButton (*it, index == static_cast<unsigned>(it - buttonVector.begin()), size_arg))
      index = static_cast<unsigned>(it - buttonVector.begin());

  ImGui::EndGroup();

  return index;
  }
//}}}

//{{{
void cUI::printHex (uint8_t* ptr, uint32_t numBytes, uint32_t columnsPerRow, uint32_t address, bool full) {

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
      ImGui::Text ((hexString + " " + asciiString).c_str());
    else
      ImGui::Text (hexString.c_str());

    address += columnsPerRow;
    }
  }
//}}}

// - static manager
//{{{
bool cUI::registerClass (const string& name, const cUI::createFuncType createFunc) {
// register class createFunc by name to classRegister, add instance to instances

  if (getClassRegister().find (name) == getClassRegister().end()) {
    // class name not found - add to classRegister map
    getClassRegister().insert (make_pair (name, createFunc));

    // create instance of class and add to instances map
    getInstanceRegister().insert (make_pair (name, createFunc (name)));
    return true;
    }
  else
    return false;
  }
//}}}

// private:
//{{{
map<const string, cUI::createFuncType>& cUI::getClassRegister() {
// static map inside static method ensures map is created before use

  static map<const string, createFuncType> mClassRegister;
  return mClassRegister;
  }
//}}}
//{{{
map<const string, cUI*>& cUI::getInstanceRegister() {
// static map inside static method ensures map is created before use

  static map<const string, cUI*> mInstances;
  return mInstances;
  }
//}}}
