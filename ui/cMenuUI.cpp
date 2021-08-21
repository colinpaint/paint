// cMenuUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imGui
#include <imgui.h>

// stb
#include <stb_image_write.h>

#include "cUI.h"
#include "../brush/cBrush.h"
#include "../canvas/cLayer.h"
#include "../canvas/cCanvas.h"
#include "../utils/tinyfiledialogs.h"
#include "../utils/cLog.h"

using namespace std;
using namespace fmt;
//}}}

class cMenuUI : public cUI {
public:
  //{{{
  cMenuUI (const std::string& name) : cUI(name) {
    for (int i =0 ; i < 56; i++)
      mSwatches.push_back (cColor(0.f,0.f,0.f, 0.f));
    }
  //}}}
  virtual ~cMenuUI() = default;

  void addToDrawList (cCanvas& canvas, cGraphics& graphics) final {

    // coerce window to bottom fullWidth, kMenuHeight
    float const kMenuHeight = 184.f;
    ImGui::SetNextWindowPos ({0.f, ImGui::GetIO().DisplaySize.y - kMenuHeight});
    ImGui::SetNextWindowSize ({ImGui::GetIO().DisplaySize.x, kMenuHeight});

    ImGui::Begin (getName().c_str(), NULL,
                  ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse);

    // draw interlockedMenu and its subMenu
    const ImVec2 kButtonSize = {100.f,0.f};
    mMenuIndex = interlockedButtons ({"Paint", "Graphics", "Effects", "Pasteup", "Library"}, mMenuIndex, {100.f,0.f});
    switch (mMenuIndex) {
      //{{{
      case 0: {
        // brushSelector
        ImGui::SameLine();
        ImGui::BeginGroup();

        cBrush* brush = cBrush::getCurBrush();
        for (auto& item : cBrush::getClassRegister())
          if (ImGui::Selectable (format (item.first.c_str(), item.first).c_str(),
                                 cBrush::isCurBrushByName (item.first), 0, {150.f, 22.f}))
            cBrush::setCurBrushByName (graphics, item.first, brush->getRadius());

        //{{{  radius
        float radius = brush->getRadius();
        ImGui::SetNextItemWidth (150.f);
        if (ImGui::SliderFloat ("radius", &radius, 1.f, 100.f))
          brush->setRadius (radius);
        //}}}
        //{{{  opacity
        ImGui::SetNextItemWidth (150.f);
        cColor color = brush->getColor();
        ImGui::SliderFloat ("opacity", &color.a, 0.f, 1.f);
        brush->setColor (color);
        //}}}
        ImGui::EndGroup();

        // swatches
        ImGui::SameLine();
        ImGui::BeginGroup();
        unsigned swatchIndex = 0;
        for (auto& swatch : mSwatches) {
          //{{{  iterate swatch
          bool disabled = swatch.a == 0.f;
          int alphaPrev = disabled ? ImGuiColorEditFlags_AlphaPreview : 0;
          if (ImGui::ColorButton (format ("swatch##{}", swatchIndex).c_str(),
                                  ImVec4 (swatch.r,swatch.g,swatch.b, swatch.a),
                                  ImGuiColorEditFlags_NoTooltip | alphaPrev, {20.f, 20.f}) && !disabled)
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
          //}}}
        ImGui::EndGroup();

        // colourPicker
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::SetNextItemWidth (kMenuHeight);

        ImVec4 imBrushColor = ImVec4 (brush->getColor().r,brush->getColor().g,brush->getColor().b, brush->getColor().a);
        ImGui::ColorPicker4 (
          "colour", (float*)&imBrushColor,
          ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float |
          ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview |
          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel,
          nullptr);
        brush->setColor (cColor (imBrushColor.x, imBrushColor.y, imBrushColor.z, imBrushColor.w));

        ImGui::EndGroup();

        // no group for last button
        ImGui::SameLine();
        ImGui::ColorButton ("color", imBrushColor, ImGuiColorEditFlags_NoTooltip, {40.f, 7 * (20.f + 4.f)});

        break;
        }
      //}}}
      //{{{
      case 1: {
        ImGui::SameLine();
        ImGui::Button ("todo", kButtonSize);
        break;
        }
      //}}}
      //{{{
      case 2: {
        ImGui::SameLine();
        ImGui::BeginGroup();

        float hue = canvas.getCurLayer()->getHue();
        float sat = canvas.getCurLayer()->getSat();
        float val = canvas.getCurLayer()->getVal();

        ImGui::SetNextItemWidth (200.f);
        if (ImGui::SliderFloat ("hue", &hue, -1.0f, 1.0f))
          canvas.getCurLayer()->setHueSatVal (hue, sat, val);

        ImGui::SetNextItemWidth (200.f);
        if (ImGui::SliderFloat ("sat", &sat, -1.0f, 1.0f))
          canvas.getCurLayer()->setHueSatVal (hue, sat, val);

        ImGui::SetNextItemWidth (200.f);
        if (ImGui::SliderFloat ("val", &val, -1.0f, 1.0f))
          canvas.getCurLayer()->setHueSatVal (hue, sat, val);

        ImGui::EndGroup();

        break;
        }
      //}}}
      //{{{
      case 3: {
        ImGui::SameLine();
        ImGui::Button ("todo", kButtonSize);
        break;
        }
      //}}}
      //{{{
      case 4: {
        ImGui::SameLine();

        if (ImGui::Button ("save", kButtonSize)) {
          char const* filters[] = { "*.png" };
          char const* fileName = tinyfd_saveFileDialog ("save file", "", 1, filters, "image files");
          if (fileName) {
            cPoint size;
            uint8_t* pixels = canvas.getPixels (size);
            int result = stbi_write_png (fileName, size.x, size.y, 4, pixels, size.x* 4);
            free (pixels);
            }
          }

        break;
        }
      //}}}
      //{{{
      default:
        ImGui::SameLine();
        ImGui::Button ("default", kButtonSize);
        break;
      //}}}
      }

    ImGui::End();
    }

private:
  unsigned mMenuIndex = 0;

  std::vector<cColor> mSwatches;

  //{{{
  static cUI* create (const string& className) {
    return new cMenuUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("menu", &create);
  };
