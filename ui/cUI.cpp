// cUI.cpp - static manager and maybe
//{{{  includes
#include "cUI.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// imGui
#include <imgui.h>

#include "../graphics/cGraphics.h"
#include "../brushes/cBrush.h"
#include "../canvas/cCanvas.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}
#define DRAW_CANVAS // useful to disable when bringing up backends

//{{{
void cUI::listClasses() {
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, format ("ui - {}", ui.first));
  }
//}}}

void cUI::draw (cCanvas& canvas, cGraphics& graphics) {
// draw canvas + imGui using graphics

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

  #ifdef DRAW_CANVAS
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
    canvas.draw (cPoint ((int)ImGui::GetWindowSize().x, (int)ImGui::GetWindowSize().y));
  #endif

  ImGui::End();

  // draw UI instances to imGui drawList
  for (auto& ui : getInstances())
    ui.second->addToDrawList (canvas, graphics);

  ImGui::Render();

  // draw imGui::drawList to screen window frameBuffer
  graphics.draw();
  }

