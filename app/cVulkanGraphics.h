// cVulkanGraphics.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <array>

#include "cGraphics.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#include "../common/cLog.h"
//}}}

class cVulkanGraphics : public cGraphics {
public:
  //{{{
  virtual ~cVulkanGraphics() {
    ImGui_ImplVulkan_Shutdown();
    }
  //}}}

  // imgui
  //{{{
  virtual bool init() final {
    //return ImGui_ImplVulkan_Init (); //ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass
    return true;
    }
  //}}}
  virtual void newFrame() final { ImGui_ImplVulkan_NewFrame(); }
  //{{{
  virtual void clear (const cPoint& size) final {
    (void)size;
    }
  //}}}
  //{{{
  virtual void renderDrawData() final {

    //if (!minimized)
    VkSemaphore imageAcquiredSem = cGlfwPlatform::vulkanWindow->FrameSemaphores[cGlfwPlatform::vulkanWindow->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore renderCompleteSem = cGlfwPlatform::vulkanWindow->FrameSemaphores[cGlfwPlatform::vulkanWindow->SemaphoreIndex].RenderCompleteSemaphore;

    VkResult result = vkAcquireNextImageKHR (cGlfwPlatform::gDevice, cGlfwPlatform::vulkanWindow->Swapchain, UINT64_MAX,
                                             imageAcquiredSem, VK_NULL_HANDLE, &cGlfwPlatform::vulkanWindow->FrameIndex);
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
      cGlfwPlatform::gSwapChainRebuild = true;
      return;
      }
    cGlfwPlatform::checkVkResult (result);

    ImGui_ImplVulkanH_Frame* vulkanFrame = &cGlfwPlatform::vulkanWindow->Frames[cGlfwPlatform::vulkanWindow->FrameIndex];

    // wait indefinitely instead of periodically checking
    result = vkWaitForFences (cGlfwPlatform::gDevice, 1, &vulkanFrame->Fence, VK_TRUE, UINT64_MAX);
    cGlfwPlatform::checkVkResult (result);

    result = vkResetFences (cGlfwPlatform::gDevice, 1, &vulkanFrame->Fence);
    cGlfwPlatform::checkVkResult(result);

    result = vkResetCommandPool (cGlfwPlatform::gDevice, vulkanFrame->CommandPool, 0);
    cGlfwPlatform::checkVkResult (result);

    // begin command buffer
    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer (vulkanFrame->CommandBuffer, &commandBufferBeginInfo);
    cGlfwPlatform::checkVkResult (result);

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = cGlfwPlatform::vulkanWindow->RenderPass;
    renderPassBeginInfo.framebuffer = vulkanFrame->Framebuffer;
    renderPassBeginInfo.renderArea.extent.width = cGlfwPlatform::vulkanWindow->Width;
    renderPassBeginInfo.renderArea.extent.height = cGlfwPlatform::vulkanWindow->Height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &cGlfwPlatform::vulkanWindow->ClearValue;
    vkCmdBeginRenderPass (vulkanFrame->CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // imgui primitives to command buffer
    ImGui_ImplVulkan_RenderDrawData (ImGui::GetDrawData(), vulkanFrame->CommandBuffer);

    // submit command buffer
    vkCmdEndRenderPass (vulkanFrame->CommandBuffer);

    // end command buffer
    VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAcquiredSem;
    submitInfo.pWaitDstStageMask = &pipelineStageFlags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanFrame->CommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderCompleteSem;
    result = vkEndCommandBuffer (vulkanFrame->CommandBuffer);
    cGlfwPlatform::checkVkResult (result);

    result = vkQueueSubmit (cGlfwPlatform::gQueue, 1, &submitInfo, vulkanFrame->Fence);
    cGlfwPlatform::checkVkResult (result);
    }
  //}}}

  // creates
  virtual cQuad* createQuad (const cPoint& size) final { return new cOpenVulkanQuad (size); }
  virtual cQuad* createQuad (const cPoint& size, const cRect& rect) final { return new cOpenVulkanQuad (size, rect); }

  virtual cTarget* createTarget() final { return new cOpenVulkanTarget(); }
  virtual cTarget* createTarget (const cPoint& size, cTarget::eFormat format) final {
    return new cOpenVulkanTarget (size, format); }
  virtual cTarget* createTarget (uint8_t* pixels, const cPoint& size, cTarget::eFormat format) final {
    return new cOpenVulkanTarget (pixels, size, format); }

  virtual cLayerShader* createLayerShader() final { return new cOpenVulkanLayerShader(); }
  virtual cPaintShader* createPaintShader() final { return new cOpenVulkanPaintShader(); }

  //{{{
  virtual cTexture* createTexture (cTexture::eTextureType textureType, const cPoint& size) final {
    switch (textureType) {
      case cTexture::eRgba:   return new cOpenVulkanRgbaTexture (textureType, size);
      case cTexture::eNv12:   return new cOpenVulkanNv12Texture (textureType, size);
      case cTexture::eYuv420: return new cOpenVulkanYuv420Texture (textureType, size);
      default : return nullptr;
      }
    }
  //}}}
  //{{{
  virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) final {
    switch (textureType) {
      case cTexture::eRgba:   return new cOpenVulkanRgbaShader();
      case cTexture::eNv12:   return new cOpenVulkanNv12Shader();
      case cTexture::eYuv420: return new cOpenVulkanYuv420Shader();
      default: return nullptr;
      }
    }
  //}}}

private:
  //{{{
  class cOpenVulkanQuad : public cQuad {
  public:
    cOpenVulkanQuad (const cPoint& size) : cQuad(size) {
      (void)size;
      }
    cOpenVulkanQuad (const cPoint& size, const cRect& rect) : cQuad(size) {
      (void)size;
      (void)rect;
      }

    void draw() final {
      }

  private:
    inline static const uint32_t mNumIndices = 6;
    inline static const uint8_t kIndices[mNumIndices] = {
      0, 1, 2, // 0   0-3
      0, 3, 1  // |\   \|
      };       // 2-1   1
    };
  //}}}
  //{{{
  class cOpenVulkanTarget : public cTarget {
  public:
    cOpenVulkanTarget() : cTarget ({0,0}) {
      }
    cOpenVulkanTarget (const cPoint& size, eFormat format) : cTarget(size) {
      (void)size;
      (void)format;
      }
    cOpenVulkanTarget (uint8_t* pixels, const cPoint& size, eFormat format) : cTarget(size) {
      (void)pixels;
      (void)size;
      (void)format;
      }
    virtual ~cOpenVulkanTarget() {
      }

    /// gets
    uint8_t* getPixels() final {
      return mPixels;
      }

    // sets
    void setSize (const cPoint& size) final {
      if (mFrameBufferObject == 0)
        mSize = size;
      else
        cLog::log (LOGERROR, "unimplmented setSize of non screen Target");
      };
    void setTarget (const cRect& rect) final {
      (void)rect;
      }
    void setBlend() final {
      }
    void setSource() final {
      }

    // actions
    void invalidate() final {
      }
    void pixelsChanged (const cRect& rect) final {
      (void)rect;
      }

    void clear (const cColor& color) final {
      (void)color;
      }
    void blit (cTarget& src, const cPoint& srcPoint, const cRect& dstRect) final {
      (void)src;
      (void)srcPoint;
      (void)dstRect;
      }

    bool checkStatus() final {
      return true;
      }
    void reportInfo() final {
      }
    };
  //}}}

  //{{{
  class cOpenVulkanRgbaTexture : public cTexture {
  public:
    cOpenVulkanRgbaTexture (eTextureType textureType, const cPoint& size) : cTexture(textureType, size) {
      }

    virtual ~cOpenVulkanRgbaTexture() {
      }

    virtual void* getTextureId() final {
      return nullptr;
      }

    virtual void setPixels (uint8_t** pixels) final {
      (void)pixels;
      }

    virtual void setSource() final {
      }
    };
  //}}}
  //{{{
  class cOpenVulkanNv12Texture : public cTexture {
  public:
    cOpenVulkanNv12Texture (eTextureType textureType, const cPoint& size) : cTexture(textureType, size) {
      }

    virtual ~cOpenVulkanNv12Texture() {
      }

    virtual void* getTextureId() final {
      return nullptr;
      }

    virtual void setPixels (uint8_t** pixels) final {
      (void)pixels;
      }

    virtual void setSource() final {
      }
    };
  //}}}
  //{{{
  class cOpenVulkanYuv420Texture : public cTexture {
  public:
    cOpenVulkanYuv420Texture (eTextureType textureType, const cPoint& size) : cTexture(textureType, size) {
      }
    virtual ~cOpenVulkanYuv420Texture() {
      }

    virtual void* getTextureId() final {
      return nullptr;
      }

    virtual void setPixels (uint8_t** pixels) final {
      (void)pixels;
      }
    virtual void setSource() final {
      }
    };
  //}}}

  //{{{
  class cOpenVulkanRgbaShader : public cTextureShader {
  public:
    cOpenVulkanRgbaShader() : cTextureShader() {
      }
    virtual ~cOpenVulkanRgbaShader() {
      }

    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;
      }
    virtual void use() final {
      }
    };
  //}}}
  //{{{
  class cOpenVulkanNv12Shader : public cTextureShader {
  public:
    cOpenVulkanNv12Shader() : cTextureShader() {
      }
    virtual ~cOpenVulkanNv12Shader() {
      }

    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;
      }
    virtual void use() final {
      }
    };
  //}}}
  //{{{
  class cOpenVulkanYuv420Shader : public cTextureShader {
  public:
    cOpenVulkanYuv420Shader() : cTextureShader() {
      }
    virtual ~cOpenVulkanYuv420Shader() {
      }

    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;
      }
    virtual void use() final {
      }
    };
  //}}}

  //{{{
  class cOpenVulkanLayerShader : public cLayerShader {
  public:
    cOpenVulkanLayerShader() : cLayerShader() {
      }
    virtual ~cOpenVulkanLayerShader() {
      }

    // sets
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;
      }
    virtual void setHueSatVal (float hue, float sat, float val) final {
      (void)hue;
      (void)sat;
      (void)val;
      }

    virtual void use() final {
      }
    };
  //}}}
  //{{{
  class cOpenVulkanPaintShader : public cPaintShader {
  public:
    cOpenVulkanPaintShader() : cPaintShader() {
      }
    virtual ~cOpenVulkanPaintShader() {
      }

    // sets
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;
      }
    virtual void setStroke (cVec2 pos, cVec2 prevPos, float radius, const cColor& color) final {
      (void)pos;
      (void)prevPos;
      (void)radius;
      (void)color;
      }

    virtual void use() final {
      }
    };
  //}}}
  };
