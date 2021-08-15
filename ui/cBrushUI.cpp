// cBrushUI.cpp
//{{{  includes
#include "cBrushUI.h"

#include <cstdint>
#include <vector>
#include <string>

// glm
#include <vec4.hpp>

// imgui
#include <imgui.h>

#include "../brushes/cBrush.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

void cBrushUI::addToDrawList (cCanvas& canvas, cGraphics& graphics) {

  ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);

  cBrush* brush = cBrush::getCurBrush();
  for (auto& item : cBrush::getClassRegister()) {
    if (ImGui::Selectable (format (item.first.c_str(), item.first).c_str(), cBrush::isCurBrushByName (item.first), 0,
                           ImVec2 (ImGui::GetWindowSize().x, 30)))
      cBrush::setCurBrushByName (item.first, brush->getRadius(), graphics);

    // when we make brush thumbnails
    //ImGui::SameLine (0.001f);
    //ImGui::Image ((void*)(intptr_t)1, ImVec2 (40, 30));
    }

  float radius = brush->getRadius();
  if (ImGui::SliderFloat ("radius", &radius, 1.f, 100.f))
    brush->setRadius (radius);

  ImGui::End();
  }
