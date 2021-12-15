// cLayersUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imgui
#include <imgui.h>

// stb
#include <stb_image_write.h>

#include "../utils/tinyfiledialogs.h"

#include "cUI.h"
#include "../paint/cLayer.h"
#include "../paint/cPaintApp.h"
#include "../utils/cLog.h"

using namespace std;
//}}}

class cLayersUI : public cUI {
public:
  cLayersUI (const std::string& name) : cUI(name) {}
  virtual ~cLayersUI() = default;

  void addToDrawList (cApp& app) final {

    cPaintApp& paintApp = dynamic_cast<cPaintApp&>(app);

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);
    ImGui::SetWindowPos (ImVec2(ImGui::GetIO().DisplaySize.x - ImGui::GetWindowWidth(), 0.f));

    unsigned layerIndex = 0;
    for (auto layer : paintApp.getLayers()) {
      if (ImGui::Selectable (fmt::format ("##{}", layerIndex).c_str(), paintApp.isCurLayer (layerIndex), 0,
                             ImVec2 (ImGui::GetWindowSize().x, 30)))
        paintApp.switchCurLayer (layerIndex);

      if (ImGui::BeginPopupContextItem()) {
        bool visible = layer->isVisible();
        if (ImGui::MenuItem ("visible", "", &visible))
          layer->setVisible (visible);

        if (ImGui::MenuItem ("delete", "", false, paintApp.getNumLayers() != 1))
          paintApp.deleteLayer (layerIndex);

        ImGui::Separator();
        if (ImGui::MenuItem ("cancel"))
          ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
        }

      ImGui::SameLine (0.001f);
      ImGui::Image ((void*)(intptr_t)layer->getTextureId(), ImVec2 (40, 30));

      ImGui::SameLine();
      if (layer->getName() != "")
        ImGui::Text ("%s", layer->getName().c_str());
      else if (layerIndex == 0)
        ImGui::Text ("background");
      else
        ImGui::Text ("layer %d", layerIndex);
      layerIndex++;
      }

    if (ImGui::Button ("new layer")) {
      unsigned newLayerIndex = paintApp.newLayer();
      paintApp.switchCurLayer (newLayerIndex);
      }

    ImGui::End();
    }

private:
  //{{{
  static cUI* create (const string& className) {
    return new cLayersUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("layers", &create);
  };
