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
#include "../canvas/cLayer.h"
#include "../canvas/cCanvas.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

class cMenuUI : public cUI {
public:
  cMenuUI (const std::string& name) : cUI(name) {}
  virtual ~cMenuUI() = default;

  void addToDrawList (cCanvas& canvas, cGraphics& graphics) final {

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);

    if (ImGui::Button ("hsv"))
      ImGui::OpenPopup ("hsv");

    if (ImGui::BeginPopupModal ("hsv")) {
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

    if (ImGui::Button ("save")) {
      char const* filters[] = { "*.png" };
      char const* fileName = tinyfd_saveFileDialog ("save file", "", 1, filters, "image files");
      if (fileName) {
        cPoint size;
        uint8_t* pixels = canvas.getPixels (size);
        int result = stbi_write_png (fileName, size.x, size.y, 4, pixels, size.x* 4);
        free (pixels);
        }
      }

    ImGui::End();
    }

private:
  //{{{
  static cUI* create (const std::string& className) {
    return new cMenuUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("menu", &create);
  };
