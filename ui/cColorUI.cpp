// cColorUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imgui
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
      mSwatches.push_back (cColor(0.f,0.f,0.f, 0.f));
    }
  //}}}
  virtual ~cColorUI() = default;

  void addToDrawList (cCanvas& canvas, cGraphics& graphics) final {

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);

    // colorPicker
    cBrush* brush = cBrush::getCurBrush();
    ImVec4 imBrushColor = ImVec4 (brush->getColor().r,brush->getColor().g,brush->getColor().b, brush->getColor().a);
    ImGui::ColorPicker4 ("colour", (float*)&imBrushColor, kColorSelectorFlags, nullptr);
    float opacity = brush->getColor().a;
    ImGui::SliderFloat ("opacity", &opacity, 0.f, 1.f);
    brush->setColor (cColor (imBrushColor.x, imBrushColor.y, imBrushColor.z, opacity));

    // iterate swatches
    unsigned swatchIndex = 0;
    for (auto& swatch : mSwatches) {
      bool disabled = swatch.a == 0.f;
      int alphaPrev = disabled ? ImGuiColorEditFlags_AlphaPreview : 0;

      if (ImGui::ColorButton (format ("swatch##{}", swatchIndex).c_str(),
                              ImVec4 (swatch.r,swatch.g,swatch.b, swatch.a),
                              ImGuiColorEditFlags_NoTooltip | alphaPrev,
                              ImVec2 (20, 20)) && !disabled)
        brush->setColor (swatch);

      // swatch popup
      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem ("set", "S")) {
          swatch = brush->getColor();
          swatch.a = 1.f;
          }

        if (ImGui::MenuItem ("unset", "X", nullptr, swatch.a != 0.f) )
          swatch = cColor (0.f,0.f,0.f, 0.f);

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
  std::vector<cColor> mSwatches;

  //{{{
  static cUI* create (const std::string& className) {
    return new cColorUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("colour", &create);
  };
