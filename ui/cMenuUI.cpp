// cMenuUI.cpp
//{{{  includes
#include <cstdint>
#include <string>
#include <array>
#include <chrono>

// imGui
#include "../imgui/imgui.h"
#include "../app/myImgui.h"

// stb
#include <stb_image_write.h>

#include "../app/cPlatform.h"

#include "../ui/cUI.h"

#include "../brush/cBrush.h"
#include "../paint/cLayer.h"
#include "../paint/cPaintApp.h"

#include "../utils/tinyfiledialogs.h"
#include "../utils/cLog.h"

using namespace std;
//}}}

class cMenuUI : public cUI {
public:
  //{{{
  cMenuUI (const string& name) : cUI(name) {
    for (auto& swatch : mColorSwatches)
      swatch = {0.f,0.f,0.f, 0.f};
    }
  //}}}
  virtual ~cMenuUI() = default;

  void addToDrawList (cApp& app) final {

    cPaintApp& paintApp = dynamic_cast<cPaintApp&>(app);

    // coerce window to bottom fullWidth, kMenuHeight
    ImGui::SetNextWindowPos ({0.f, ImGui::GetIO().DisplaySize.y - kMenuHeight});
    ImGui::SetNextWindowSize ({ImGui::GetIO().DisplaySize.x, kMenuHeight});

    ImGui::Begin (getName().c_str(), NULL,
                  //ImGuiWindowFlags_NoDocking | 
                  ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
                  ImGuiWindowFlags_NoSavedSettings);

    // draw mainMenu as interlocked buttons, select mainMenuIndex
    mMainMenuIndex = interlockedButtons ({"Paint", "Graphics", "Effects", "Pasteup", "Library"},
                                         mMainMenuIndex, kMainMenuButtonSize);
    // draw mainMenu selected subMenu
    switch (mMainMenuIndex) {
      //{{{
      case 0: { // paint
        const ImVec2 kSubButtonSize = { 150.f, 18.f+4.f };
        const ImVec2 kColorButtonSize = { 40.f, kSwatchRows * kSubButtonSize.y };

        // brush group
        ImGui::SameLine();
        ImGui::BeginGroup();

        // registered brushes
        cBrush* brush = cBrush::getCurBrush();
        for (auto& item : cBrush::getClassRegister())
          if (ImGui::Selectable (fmt::format (item.first.c_str(), item.first).c_str(),
                                 cBrush::isCurBrushByName (item.first), 0, kSubButtonSize))
            cBrush::setCurBrushByName (paintApp.getGraphics(), item.first, brush->getRadius());

        // radius
        float radius = brush->getRadius();
        ImGui::SetNextItemWidth (kSubButtonSize.x);
        if (ImGui::SliderFloat ("radius", &radius, 1.f, 100.f))
          brush->setRadius (radius);

        // opacity
        ImGui::SetNextItemWidth (kSubButtonSize.x);
        cColor color = brush->getColor();
        ImGui::SliderFloat ("opacity", &color.a, 0.f, 1.f);
        brush->setColor (color);

        ImGui::EndGroup();

        // colorSwatches group
        ImGui::SameLine();
        ImGui::BeginGroup();
        unsigned swatchIndex = 0;
        for (auto& swatch : mColorSwatches) {
          //{{{  iterate swatches
          bool disabled = swatch.a == 0.f;
          int alphaPrev = disabled ? ImGuiColorEditFlags_AlphaPreview : 0;
          if (ImGui::ColorButton (fmt::format ("swatch##{}", swatchIndex).c_str(),
                                  ImVec4 (swatch.r,swatch.g,swatch.b, swatch.a),
                                  ImGuiColorEditFlags_NoTooltip | alphaPrev,
                                  {kSubButtonSize.y - 2.f, kSubButtonSize.y - 2.f}) && !disabled)
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
          if (++swatchIndex % kSwatchPerRow)
            ImGui::SameLine();
          }
          //}}}
        ImGui::EndGroup();

        // colorPicker group
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::SetNextItemWidth (kMenuHeight);

        ImVec4 imBrushColor = { brush->getColor().r,brush->getColor().g,brush->getColor().b, brush->getColor().a };
        ImGui::ColorPicker4 ("colour", (float*)&imBrushColor,
                             ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float |
                             ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview |
                             ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel,
                             nullptr);
        brush->setColor ({imBrushColor.x,imBrushColor.y,imBrushColor.z, imBrushColor.w});
        ImGui::EndGroup();

        // no group for last color button
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::ColorButton ("color", imBrushColor, ImGuiColorEditFlags_NoTooltip, kColorButtonSize);
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginGroup();
        clockButton ("clock", app.getNow(), {110.f,150.f});
        ImGui::EndGroup();
        break;
        }
      //}}}
      //{{{
      case 1: { // graphics
        ImGui::SameLine();
        ImGui::Button ("todo", kMainMenuButtonSize);
        break;
        }
      //}}}
      //{{{
      case 2: { // effects
        const ImVec2 kSubButtonSize = { 200.f,22.f };

        ImGui::SameLine();
        ImGui::BeginGroup();

        float hue = paintApp.getCurLayer()->getHue();
        float sat = paintApp.getCurLayer()->getSat();
        float val = paintApp.getCurLayer()->getVal();

        ImGui::SetNextItemWidth (kSubButtonSize.x);
        if (ImGui::SliderFloat ("hue", &hue, -1.0f, 1.0f))
          paintApp.getCurLayer()->setHueSatVal (hue, sat, val);

        ImGui::SetNextItemWidth (kSubButtonSize.x);
        if (ImGui::SliderFloat ("sat", &sat, -1.0f, 1.0f))
          paintApp.getCurLayer()->setHueSatVal (hue, sat, val);

        ImGui::SetNextItemWidth (kSubButtonSize.x);
        if (ImGui::SliderFloat ("val", &val, -1.0f, 1.0f))
          paintApp.getCurLayer()->setHueSatVal (hue, sat, val);

        ImGui::EndGroup();

        break;
        }
      //}}}
      //{{{
      case 3: { // pasteup
        ImGui::SameLine();
        ImGui::Button ("todo", kMainMenuButtonSize);
        break;
        }
      //}}}
      //{{{
      case 4: { // library
        ImGui::SameLine();
        ImGui::BeginGroup();

        if (ImGui::Button ("save", kMainMenuButtonSize)) {
          char const* filters[] = { "*.png" };
          char const* fileName = tinyfd_saveFileDialog ("save file", "", 1, filters, "image files");
          if (fileName) {
            cPoint size;
            uint8_t* pixels = paintApp.getPixels (size);
            stbi_write_png (fileName, size.x, size.y, 4, pixels, size.x* 4);
            free (pixels);
            }
          }
        ImGui::EndGroup();

        break;
        }
      //}}}
      }

    ImGui::End();
    }

private:
  // const - could be more dynamic
  static inline const float kMenuHeight = 184.f;
  static inline const ImVec2 kMainMenuButtonSize = { 100.f,0.f };

  static inline const unsigned kSwatchPerRow = 8;
  static inline const unsigned kSwatchRows = 7;

  // vars
  uint8_t mMainMenuIndex = 0;
  array<cColor, kSwatchRows * kSwatchPerRow> mColorSwatches;

  // static register
  //{{{
  static cUI* create (const string& className) {
    return new cMenuUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("menu", &create);
  };
