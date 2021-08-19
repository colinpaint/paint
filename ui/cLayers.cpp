// cLayersUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imgui
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

class cLayersUI : public cUI {
public:
  cLayersUI (const std::string& name) : cUI(name) {}
  virtual ~cLayersUI() = default;

  void addToDrawList (cCanvas& canvas, cGraphics& graphics) final {

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);
    ImGui::SetWindowPos (ImVec2(ImGui::GetIO().DisplaySize.x - ImGui::GetWindowWidth(), 0.f));

    unsigned layerIndex = 0;
    for (auto layer : canvas.getLayers()) {
      if (ImGui::Selectable (format ("##{}", layerIndex).c_str(), canvas.isCurLayer (layerIndex), 0,
                             ImVec2 (ImGui::GetWindowSize().x, 30)))
        canvas.switchCurLayer (layerIndex);

      if (ImGui::BeginPopupContextItem()) {
        bool visible = layer->isVisible();
        if (ImGui::MenuItem ("visible", "", &visible))
          layer->setVisible (visible);

        if (ImGui::MenuItem ("delete", "", false, canvas.getNumLayers() != 1))
          canvas.deleteLayer (layerIndex);

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
      unsigned newLayerIndex = canvas.newLayer();
      canvas.switchCurLayer (newLayerIndex);
      }

    ImGui::End();
    }

private:
  //{{{
  static cUI* create (const string& className) {
    return new cCanvasUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("layers", &create);
  };
