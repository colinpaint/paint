// cColorUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// glm
#include <vec4.hpp>

//imgui
#include <imgui.h>

#include "cUI.h"
#include "../brush/cBrush.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}
//{{{
constexpr ImGuiColorEditFlags kColorSelectorFlags = ImGuiColorEditFlags_HDR |
                                                    ImGuiColorEditFlags_Float |
                                                    ImGuiColorEditFlags_NoInputs |
                                                    ImGuiColorEditFlags_PickerHueWheel;
//}}}

class cColorUI  : public cUI {
public:
  //{{{
  cColorUI (const std::string& name) : cUI(name) {
    for (int i =0 ; i < 16; i++)
      mSwatches.push_back (glm::vec4 (0.f,0.f,0.f,0.f));
    }
  //}}}
  virtual ~cColorUI() = default;

  void addToDrawList (cCanvas& canvas, cGraphics& graphics) final {

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);

    // colorPicker
    cBrush* brush = cBrush::getCurBrush();
    ImVec4 imBrushColor = ImVec4 (brush->getColor().x, brush->getColor().y, brush->getColor().z, brush->getColor().w);
    ImGui::ColorPicker4 ("colour", (float*)&imBrushColor, kColorSelectorFlags, nullptr);
    float opacity = brush->getColor().w;
    ImGui::SliderFloat ("opacity", &opacity, 0.f, 1.f);
    brush->setColor (glm::vec4 (imBrushColor.x, imBrushColor.y, imBrushColor.z, opacity));

    // iterate swatches
    unsigned swatchIndex = 0;
    for (auto& swatch : mSwatches) {
      bool disabled = swatch.w == 0.f;
      int alphaPrev = disabled ? ImGuiColorEditFlags_AlphaPreview : 0;

      if (ImGui::ColorButton (format ("swatch##{}", swatchIndex).c_str(),
                              ImVec4 (swatch.x, swatch.y, swatch.z, swatch.w),
                              ImGuiColorEditFlags_NoTooltip | alphaPrev,
                              ImVec2 (20, 20)) && !disabled)
        brush->setColor (swatch);

      // swatch popup
      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem ("set", "S")) {
          swatch = brush->getColor();
          swatch.w = 1.f;
          }

        if (ImGui::MenuItem ("unset", "X", nullptr, swatch.w != 0.f) )
          swatch = glm::vec4 (0.f, 0.f, 0.f, 0.f);

        ImGui::Separator();
        if (ImGui::MenuItem ("cancel", "C") )
          ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
        }

      // swatch line wrap
      if (++swatchIndex % 8)
        ImGui::SameLine();
      }

    ImGui::End();
    }

private:
  std::vector<glm::vec4> mSwatches;

  //{{{
  static cUI* createUI (const std::string& className) {
    return new cColorUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("colour", &createUI);
  };
