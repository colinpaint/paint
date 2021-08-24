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

#include "../utils/utils.h"
#include "../utils/cLog.h"
#include "../graphics/cGraphics.h"
#include "../brush/cBrush.h"
#include "../canvas/cCanvas.h"

using namespace std;
using namespace fmt;
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
    cLog::log (LOGINFO, format ("- {}", ui.first));
  }
//}}}
//{{{
void cUI::listInstances() {

  cLog::log (LOGINFO, "ui instance register");
  for (auto& ui : getInstanceRegister())
    cLog::log (LOGINFO, format ("- {}", ui.first));
  }
//}}}

// static draw
//{{{
void cUI::draw (cCanvas& canvas, cGraphics& graphics, cPoint windowSize) {
// draw canvas and its UI's with imGui, using graphics
// - draw canvas background
//   - dummy fullscreen window, no draw,move,scroll,focus
//     - dummy invisibleButton captures mouse events
// - draw registered UI instances

  // set dummy background window to full display size
  ImGui::SetNextWindowPos (ImVec2(0,0));
  ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
  ImGui::Begin ("bgnd", NULL,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);

  // fullWindow invisibleButton grabbing mouse,leftButton events
  ImGui::InvisibleButton ("bgnd", ImGui::GetWindowSize(), ImGuiButtonFlags_MouseButtonLeft);

  #ifdef DRAW_CANVAS
    // route mouse,leftButton events to canvas, with centred pos coords
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
      canvas.mouse (ImGui::IsItemActive(),
                    ImGui::IsMouseClicked (ImGuiMouseButton_Left),
                    ImGui::IsMouseDragging (ImGuiMouseButton_Left, 0.f),
                    ImGui::IsMouseReleased (ImGuiMouseButton_Left),
                    cVec2 (ImGui::GetMousePos().x - (windowSize.x / 2.f),
                           ImGui::GetMousePos().y - (windowSize.y / 2.f)),
                    cVec2 (ImGui::GetMouseDragDelta (ImGuiMouseButton_Left).x,
                           ImGui::GetMouseDragDelta (ImGuiMouseButton_Left).y));

      if (!ImGui::IsItemActive()) {
        // draw outline circle cursor
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        cBrush* brush = cBrush::getCurBrush();
        draw_list->AddCircle (ImGui::GetMousePos(), brush->getRadius() + 1.f,
                              ImGui::GetColorU32(IM_COL32(0,0,0,255)));
        //{{{  possible filledCircle
        //draw_list->AddCircleFilled (ImGui::GetMousePos(), brush->getRadius() + 1.f,
          //ImGui::GetColorU32 (
            //IM_COL32(brush->getColor().x * 255, brush->getColor().y * 255, brush->getColor().z * 255, 255)));
        //}}}
        }
      }
    else {
      // possible swipe detection here
      }

    // draw canvas to screen window frameBuffer
    canvas.draw (windowSize);
  #endif

  ImGui::End();

  // add registered UI instances to imGui drawList
  for (auto& ui : getInstanceRegister())
    ui.second->addToDrawList (canvas, graphics);
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

  ImVec2 pos = window->DC.CursorPos;
  const ImRect rect (pos, pos + size);
  ImGui::ItemSize (size, style.FramePadding.y);

  const ImGuiID id = window->GetID (label.c_str());
  if (!ImGui::ItemAdd (rect, id))
    return false;

  ImGuiButtonFlags flags = ImGuiButtonFlags_None;
  if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
    flags |= ImGuiButtonFlags_Repeat;

  bool hovered;
  bool held;
  bool pressed = ImGui::ButtonBehavior (rect, id, &hovered, &held, flags);

  // is this tyhe simplest, compared to the string below ???
  auto datePoint = floor<date::days>(timePoint);
  auto timeOfDay = date::make_time (chrono::duration_cast<chrono::milliseconds>(timePoint - datePoint));
  //{{{  draw clock graphic
  const float kPi = 3.1415926f;
  const float radius = rect.GetWidth() / 2.f;

  const ImU32 col = (held || hovered) ?ImGui::GetColorU32 (ImGuiCol_ButtonHovered) : IM_COL32_WHITE;
  window->DrawList->AddCircle (rect.GetCenter(), radius, col, 32, 3.f);

  // draw hourHand
  float handRadius = radius * 0.6f;
  float angle = (1.f - ((timeOfDay.hours().count()) + (timeOfDay.minutes().count() / 60.f) / 6.f)) * kPi;
  window->DrawList->AddLine (rect.GetCenter(),
                             rect.GetCenter() + ImVec2(handRadius * sin (angle), handRadius * cos (angle)),
                             col, 2.f);

  // draw minuteHand
  handRadius = radius * 0.75f;
  angle = (1.f - ((timeOfDay.minutes().count()) + (timeOfDay.seconds().count() / 60.f) / 30.f)) * kPi;
  window->DrawList->AddLine (rect.GetCenter(),
                             rect.GetCenter() + ImVec2(handRadius * sin (angle), handRadius * cos (angle)),
                             col, 2.f);

  // draw secondHand
  handRadius = radius * 0.85f;
  angle = (1.f - (timeOfDay.seconds().count() / 30.f)) * kPi;
  window->DrawList->AddLine (rect.GetCenter(),
                             rect.GetCenter() + ImVec2(handRadius * sin (angle), handRadius * cos (angle)),
                             col, 2.f);
  //}}}

  const string timeString = date::format ("%H:%M:%S", date::floor<chrono::seconds>(timePoint));
  window->DrawList->AddText (rect.GetBL(), col, timeString.c_str(), NULL);

  const string dateString = date::format ("%a %d %b %y", date::floor<chrono::seconds>(timePoint));
  window->DrawList->AddText (rect.GetBL() - ImVec2 (0.f, 18.f), col, dateString.c_str(), NULL);

  IMGUI_TEST_ENGINE_ITEM_INFO(id, label.c_str(), g.LastItemData.StatusFlags);
  return pressed;
  }
//}}}
//{{{
bool cUI::toggleButton (const string& label, bool toggleOn, const ImVec2& size_arg) {
// imGui custom widget - based on ImGui::ButtonEx

  ImGuiButtonFlags flags = ImGuiButtonFlags_None;

  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return false;

  ImGuiContext& g = *GImGui;
  const ImGuiStyle& style = g.Style;
  const ImGuiID id = window->GetID (label.c_str());
  const ImVec2 label_size = ImGui::CalcTextSize (label.c_str(), NULL, true);

  ImVec2 size = ImGui::CalcItemSize (size_arg,
                                     label_size.x + style.FramePadding.x * 2.0f,
                                     label_size.y + style.FramePadding.y * 2.0f);

  ImVec2 pos = window->DC.CursorPos;
  const ImRect bb (pos, pos + size);

  ImGui::ItemSize (size, style.FramePadding.y);
  if (!ImGui::ItemAdd (bb, id))
    return false;

  bool hovered;
  bool held;
  bool pressed = ImGui::ButtonBehavior (bb, id, &hovered, &held, flags);

  // Render
  const ImU32 col = ImGui::GetColorU32 (
    toggleOn || (held && hovered) ? ImGuiCol_ButtonActive :
                                    (hovered ? ImGuiCol_ButtonHovered : ImGuiCol_FrameBg));
  ImGui::RenderNavHighlight (bb, id);
  ImGui::RenderFrame (bb.Min, bb.Max, col, true, style.FrameRounding);

  if (g.LogEnabled)
    ImGui::LogSetNextTextDecoration ("[", "]");

  ImGui::RenderTextClipped (bb.Min + style.FramePadding, bb.Max - style.FramePadding,
                            label.c_str(), NULL, &label_size,
                            style.ButtonTextAlign, &bb);

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
