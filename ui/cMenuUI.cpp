// cMenuUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imGui
#include <imgui.h>

// stb
#include <stb_image_write.h>

#include "../tinyfiledialog/tinyfiledialogs.h"

#include "cUI.h"
#include "../brush/cBrush.h"
#include "../canvas/cLayer.h"
#include "../canvas/cCanvas.h"
#include "../log/cLog.h"

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

    // coerce next window to bottom full width, height
    ImGui::SetNextWindowPos ({0.f, ImGui::GetIO().DisplaySize.y - 200.f});
    ImGui::SetNextWindowSize ({ImGui::GetIO().DisplaySize.x, 200.f});

    ImGui::Begin (getName().c_str(), NULL,
                  ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::BeginGroup();
    ImGui::Button ("Paint", {100,0});
    ImGui::Button ("Graphics", {100,0});
    if (ImGui::Button ("Effect", {100,0})) {
      ImGui::OpenPopup ("hsv");
      if (ImGui::BeginPopupModal ("hsv")) {
        //{{{  hsv popup
        float hue = canvas.getCurLayer()->getHue();
        float sat = canvas.getCurLayer()->getSat();
        float val = canvas.getCurLayer()->getVal();
        if (ImGui::SliderFloat ("hue", &hue, -1.0f, 1.0f))
          canvas.getCurLayer()->setHueSatVal (hue, sat, val);
        if (ImGui::SliderFloat ("sat", &sat, -1.0f, 1.0f))
          canvas.getCurLayer()->setHueSatVal (hue, sat, val);
        if (ImGui::SliderFloat ("val", &val, -1.0f, 1.0f))
          canvas.getCurLayer()->setHueSatVal (hue, sat, val);

        if (ImGui::Button ("confirm##hsv")) {
          // persist changes
          canvas.renderCurLayer();
          canvas.getCurLayer()->setHueSatVal (0.f, 0.f, 0.f);
          ImGui::CloseCurrentPopup();
          }

        ImGui::SameLine();
        if (ImGui::Button ("cancel##hsv")) {
          canvas.getCurLayer()->setHueSatVal (0.f, 0.f, 0.f);
          ImGui::CloseCurrentPopup();
          }

        ImGui::EndPopup();
        }
        //}}}
      }
    ImGui::Button ("Pasteup", {100,0});

    if (ImGui::Button ("Library", {100,0})) {
      //{{{  save dialog
      char const* filters[] = { "*.png" };
      char const* fileName = tinyfd_saveFileDialog ("save file", "", 1, filters, "image files");
      if (fileName) {
        cPoint size;
        uint8_t* pixels = canvas.getPixels (size);
        int result = stbi_write_png (fileName, size.x, size.y, 4, pixels, size.x* 4);
        free (pixels);
        }
      }
      //}}}
    ImGui::EndGroup();

    //{{{  brush selection
    ImGui::SameLine();
    ImGui::BeginGroup();

    cBrush* brush = cBrush::getCurBrush();
    for (auto& item : cBrush::getClassRegister())
      if (ImGui::Selectable (format (item.first.c_str(), item.first).c_str(),
                             cBrush::isCurBrushByName (item.first), 0, {150, 20}))
        cBrush::setCurBrushByName (graphics, item.first, brush->getRadius());

    float radius = brush->getRadius();
    ImGui::SetNextItemWidth (150);
    if (ImGui::SliderFloat ("radius", &radius, 1.f, 100.f))
      brush->setRadius (radius);

    ImGui::SetNextItemWidth (150);
    cColor color = brush->getColor();
    ImGui::SliderFloat ("opacity", &color.a, 0.f, 1.f);
    brush->setColor (color);

    ImGui::EndGroup();
    //}}}
    //{{{  swatches
    ImGui::SameLine();
    ImGui::BeginGroup();
    unsigned swatchIndex = 0;
    for (auto& swatch : mSwatches) {
      bool disabled = swatch.a == 0.f;
      int alphaPrev = disabled ? ImGuiColorEditFlags_AlphaPreview : 0;
      if (ImGui::ColorButton (format ("swatch##{}", swatchIndex).c_str(),
                              ImVec4 (swatch.r,swatch.g,swatch.b, swatch.a),
                              ImGuiColorEditFlags_NoTooltip | alphaPrev, {20, 20}) && !disabled)
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
    ImGui::EndGroup();
    //}}}
    //{{{  colour
    ImGui::SameLine();
    ImGui::BeginGroup();

    // colorPicker
    ImGui::SetNextItemWidth (200);

    ImVec4 imBrushColor = ImVec4 (brush->getColor().r,brush->getColor().g,brush->getColor().b, brush->getColor().a);
    ImGui::ColorPicker4 (
      "colour", (float*)&imBrushColor,
      ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel,
      nullptr);
    brush->setColor (cColor (imBrushColor.x, imBrushColor.y, imBrushColor.z, imBrushColor.w));
    ImGui::EndGroup();
    //}}}

    ImGui::End();
    }

private:
  std::vector<cColor> mSwatches;
  //{{{
  static cUI* create (const string& className) {
    return new cMenuUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("menu", &create);
  };
