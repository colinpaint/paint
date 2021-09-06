// cCanvasUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

#include <fstream>
#include <streambuf>

// imgui
#include "../imgui/imgui.h"

// ui
#include "cUI.h"

// canvas
#include "../canvas/cCanvas.h"

// brush
#include "../brush/cBrush.h"

// utils
#include "../utils/cLog.h"

using namespace std;
//}}}
#define DRAW_CANVAS // useful to disable canvas when bringing up backends

class cCanvasUI : public cUI {
public:
  cCanvasUI (const string& name) : cUI(name) {}
  virtual ~cCanvasUI() = default;

  void addToDrawList (cApp* app) final {
    cCanvas* canvas = (cCanvas*)app;

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
        canvas->mouse (ImGui::IsItemActive(),
                      ImGui::IsMouseClicked (ImGuiMouseButton_Left),
                      ImGui::IsMouseDragging (ImGuiMouseButton_Left, 0.f),
                      ImGui::IsMouseReleased (ImGuiMouseButton_Left),
                      cVec2 (ImGui::GetMousePos().x - (ImGui::GetIO().DisplaySize.x / 2.f),
                             ImGui::GetMousePos().y - (ImGui::GetIO().DisplaySize.y / 2.f)),
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
      canvas->draw (cPoint ((int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y));
    #endif

    ImGui::End();
    }

private:
  ImFont* mMonoFont = nullptr;
  cCanvas* mCanvas;

  //{{{
  static cUI* create (const string& className) {
    return new cCanvasUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("aCanvas", &create);
  };
