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

#include "../graphics/cGraphics.h"
#include "../brush/cBrush.h"
#include "../canvas/cCanvas.h"
#include "../utils/cLog.h"

using namespace std;
using namespace fmt;
//}}}
#define DRAW_CANVAS // useful to disable canvas when bringing up backends
#define SHOW_DEMO
namespace {
  bool gShowDemoWindow;
  }

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

  #ifdef SHOW_DEMO
    ImGui::ShowDemoWindow (&gShowDemoWindow);
  #endif

  ImGui::Render();
  }
//}}}

// protected:
//{{{
bool cUI::toggleButton (string label, bool toggleOn, const ImVec2& size_arg) {
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

  if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
    flags |= ImGuiButtonFlags_Repeat;

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
int cUI::interlockedMenu (const vector<string>& menuVector, int menuIndex, const ImVec2& size_arg) {
// interlockedMenu helper, returns menuIndex

  ImGui::BeginGroup();
  for (auto it = menuVector.begin(); it != menuVector.end(); ++it)
    if (toggleButton (*it, menuIndex == int(it - menuVector.begin()), size_arg))
      menuIndex = int(it - menuVector.begin());
  ImGui::EndGroup();

  return menuIndex;
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
