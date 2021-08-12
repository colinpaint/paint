// cUIMan.cpp- UI class man(ager)
//{{{  includes
#include "cUIMan.h"
#include "cUI.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// imGui
#include <imgui.h>

#include "../graphics/cGraphics.h"
#include "../canvas/cCanvas.h"
#include "../brushes/cBrush.h"
#include "../brushes/cBrushMan.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}
#define CANVAS

//{{{
bool cUIMan::registerClass (const std::string& name, const createFuncType createFunc) {
// register class createFunc by name to classRegister, add instance to instances

  if (getClassRegister().find (name) == getClassRegister().end()) {
    // class name not found - add to classRegister map
    getClassRegister().insert (std::make_pair (name, createFunc));

    // create instance of class and add to instances map
    getInstances().insert (std::make_pair (name, createFunc (name)));
    return true;
    }
  else {
    // className found - error
    cLog::log (LOGERROR, fmt::format ("cUI::registerClass {} already registered", name));
    return false;
    }
  }
//}}}
//{{{
cUI* cUIMan::createByName (const std::string& name) {
// create class by name from classRegister, add instance to instances

  auto uiIt = getInstances().find (name);
  if (uiIt == getInstances().end()) {
    auto ui = getClassRegister()[name](name);
    getInstances().insert (std::make_pair (name, ui));
    return ui;
    }
  else {
    cLog::log (LOGERROR, fmt::format ("cUI::createByName {} instance already created", name));
    return uiIt->second;
    }
  }
//}}}

void cUIMan::draw (cCanvas& canvas) {

  ImGui::NewFrame();

  // canvas background UI
  // - dummy fullscreen window, no draw,move,scroll,focus
  //   - dummy invisibleButton captures mouse events
  // may draw to canvas layer frameBuffer for paint,wipe,etc
  // draw canvas to screen window frameBuffer
  ImGui::SetNextWindowPos (ImVec2(0,0));
  ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
  ImGui::Begin ("bgnd", NULL,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);

  // fullWindow invisibleButton grabbing mouse,leftButton events
  ImGui::InvisibleButton ("bgnd", ImGui::GetWindowSize(), ImGuiButtonFlags_MouseButtonLeft);

  #ifdef CANVAS
    // route mouse,leftButton events to canvas, with centred pos coords
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
      canvas.mouse (ImGui::IsItemActive(),
                    ImGui::IsMouseClicked (ImGuiMouseButton_Left),
                    ImGui::IsMouseDragging (ImGuiMouseButton_Left, 0.f),
                    ImGui::IsMouseReleased (ImGuiMouseButton_Left),
                    glm::vec2 (ImGui::GetMousePos().x - (ImGui::GetIO().DisplaySize.x / 2.f),
                               ImGui::GetMousePos().y - (ImGui::GetIO().DisplaySize.y / 2.f)),
                    glm::vec2 (ImGui::GetMouseDragDelta (ImGuiMouseButton_Left).x,
                               ImGui::GetMouseDragDelta (ImGuiMouseButton_Left).y));

      if (!ImGui::IsItemActive()) {
        // draw outline circle cursor
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        cBrush* brush = cBrushMan::getCurBrush();
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
  canvas.draw (cPoint ((int)ImGui::GetWindowSize().x, (int)ImGui::GetWindowSize().y));
  #endif

  ImGui::End();

  // draw UI instances to imGui drawList
  for (auto& ui : getInstances())
    ui.second->addToDrawList (canvas);

  ImGui::Render();

  // draw imGui::drawList to screen window frameBuffer
  cGraphics::getInstance().draw();
  }
