// cGlfwApp.cpp - glfw + openGL3,openGLES3_x,vulkan app framework
//{{{  includes
#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
  #include <windows.h>
#endif

#include <cstdint>
#include <cmath>
#include <string>
#include <array>
#include <algorithm>
#include <functional>

// glad
#if defined(GL_2_1) || defined(GL_3)
  #include <glad/glad.h>
#endif

// glfw
#include <GLFW/glfw3.h>

// imGui
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"

#include "../imgui/backends/imgui_impl_opengl3.h"

#if defined(GL_3)
  #include "cGL3Graphics.h"
#elif defined(GLES_3_0) || defined(GLES_3_1) || defined(GLES_3_2)
  #include "cGLES3Graphics.h"
#endif

// ui
#include "../ui/cUI.h"

// utils
#include "../common/cLog.h"
#include "fmt/format.h"

// app
#include "cApp.h"
#include "cPlatform.h"
#include "cGraphics.h"

using namespace std;
//}}}

// temp
#if defined(VULKAN)
  #include <backends/imgui_impl_vulkan.h>
#endif

// cPlatform interface
//{{{
class cGlfwPlatform : public cPlatform {
public:
  cGlfwPlatform (const string& name) : cPlatform (name, true, true) {}
  //{{{
  virtual ~cGlfwPlatform() {

    #if defined(VULKAN)
      checkVkResult (vkDeviceWaitIdle (gDevice));
    #endif

    ImGui_ImplGlfw_Shutdown();

    #if defined(VULKAN)
      //{{{  cleanup vulkan
      vkDestroyDescriptorPool (gDevice, gDescriptorPool, gAllocator);

      #ifdef VALIDATION
        // Remove the debug report callback
        auto vkDestroyDebugReportCallbackEXT =
          (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr (gInstance, "vkDestroyDebugReportCallbackEXT");
        vkDestroyDebugReportCallbackEXT (gInstance, gDebugReport, gAllocator);
      #endif

      vkDestroyDevice (gDevice, gAllocator);
      vkDestroyInstance (gInstance, gAllocator);
      ImGui_ImplVulkanH_DestroyWindow (gInstance, gDevice, &gMainWindowData, gAllocator);
      //}}}
    #endif

    glfwDestroyWindow (mWindow);
    glfwTerminate();

    ImGui::DestroyContext();
    }
  //}}}

  //{{{
  virtual bool init (const cPoint& windowSize) final {

    cLog::log (LOGINFO, fmt::format ("Glfw {}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR));

    glfwSetErrorCallback (glfwErrorCallback);
    if (!glfwInit())
      return false;

    //  select openGL, openGLES version
    #if defined(GL_2_1)
      //{{{  openGL 2.1
      string title = "openGL 2.1";
      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 2);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
      //}}}
    #elif defined(GLES_2)
      //{{{  openGLES 2.0 GLSL 100
      string title = "openGLES 2";
      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 2);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
      glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      //}}}
    #elif defined(GLES_3_0)
      //{{{  openGLES 3.0
      string title = "openGLES 3.0";
      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
      glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      //}}}
    #elif defined(GLES_3_1)
      //{{{  openGLES 3.1
      string title = "openGLES 3.1";
      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 1);
      glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      //}}}
    #elif defined(GLES_3_2)
      //{{{  openGLES 3.2
      string title = "openGLES 3.2";
      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
      glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      //}}}
    #elif defined(GL_3)
      //{{{  openGL 3.3 GLSL 130
      string title = "openGL 3";
      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
      //glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
      //glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
      //}}}
    #elif defined(VULKAN)
      //{{{  vulkan
      string title = "vulkan";
      glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
      //}}}
    #else
      #error BUILD_GRAPHICS not recognised
    #endif

    setShaderVersion ("#version 130");

    mWindow = glfwCreateWindow (windowSize.x, windowSize.y, (title + " " + getName()).c_str(), NULL, NULL);
    if (!mWindow) {
      //{{{  error, log, return
      cLog::log (LOGERROR, "cGlfwPlatform - glfwCreateWindow failed");
      return false;
      }
      //}}}

    mMonitor = glfwGetPrimaryMonitor();
    glfwGetWindowSize (mWindow, &mWindowSize.x, &mWindowSize.y);
    glfwGetWindowPos (mWindow, &mWindowPos.x, &mWindowPos.y);

    #if defined(VULKAN)
      //{{{  init vulkan
      if (!glfwVulkanSupported()) {
        cLog::log (LOGERROR, fmt::format ("glfw vulkan not supported"));
        return 1;
        }

      // get glfw required vulkan extensions
      uint32_t extensionsCount = 0;
      const char** extensions = glfwGetRequiredInstanceExtensions (&extensionsCount);
      for (uint32_t i = 0; i < extensionsCount; i++)
        cLog::log (LOGINFO, fmt::format ("glfwVulkanExt:{} {}", int(i), extensions[i]));
      initVulkan (extensions, extensionsCount);

      // create windowSurface
      VkSurfaceKHR surface;
      VkResult result = glfwCreateWindowSurface (gInstance, mWindow, gAllocator, &surface);
      checkVkResult (result);

      // create framebuffers
      int width, height;
      glfwGetFramebufferSize (mWindow, &width, &height);

      vulkanWindow = &gMainWindowData;
      initVulkanWindow (vulkanWindow, surface, width, height);
      //}}}
    #else
      glfwMakeContextCurrent (mWindow);
      glfwSwapInterval (1);
    #endif

    // set imGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsClassic();
    cLog::log (LOGINFO, fmt::format ("imGui {} - {}", ImGui::GetVersion(), IMGUI_VERSION_NUM));

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    #if defined(BUILD_DOCKING)
      // enable Docking
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

      // enable multi-viewport / platform windows
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        // tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
    #endif

    #if defined(VULKAN)
      //{{{  init vulkan platform backend

      // setup Platform/Renderer backends
      ImGui_ImplGlfw_InitForVulkan (mWindow, true);

      ImGui_ImplVulkan_InitInfo vulkanInitInfo = {};
      vulkanInitInfo.Instance = gInstance;
      vulkanInitInfo.PhysicalDevice = gPhysicalDevice;
      vulkanInitInfo.Device = gDevice;
      vulkanInitInfo.QueueFamily = gQueueFamily;
      vulkanInitInfo.Queue = gQueue;
      vulkanInitInfo.PipelineCache = gPipelineCache;
      vulkanInitInfo.DescriptorPool = gDescriptorPool;
      vulkanInitInfo.Allocator = gAllocator;
      vulkanInitInfo.MinImageCount = gMinImageCount;
      vulkanInitInfo.ImageCount = vulkanWindow->ImageCount;
      vulkanInitInfo.CheckVkResultFn = checkVkResult;
      ImGui_ImplVulkan_Init (&vulkanInitInfo, vulkanWindow->RenderPass);

      uploadFonts (vulkanWindow);
      //}}}
    #else
      ImGui_ImplGlfw_InitForOpenGL (mWindow, true);
    #endif

    // set callbacks
    glfwSetKeyCallback (mWindow, keyCallback);
    glfwSetFramebufferSizeCallback (mWindow, framebufferSizeCallback);
    glfwSetDropCallback (mWindow, dropCallback);

    #if defined(GL_2_1) || defined(GL_3)
      // openGL - GLAD init before any openGL function
      if (!gladLoadGLLoader ((GLADloadproc)glfwGetProcAddress)) {
        cLog::log (LOGERROR, "cOpenGL3Platform - gladLoadGLLoader failed");
        return false;
        }
    #endif

    return true;
    }
  //}}}

  // gets
  //{{{
  virtual cPoint getWindowSize() final {

    int width;
    int height;
    glfwGetWindowSize (mWindow, &width, &height);

    return cPoint (width, height);
    }
  //}}}

  // sets
  //{{{
  virtual void setVsync (bool vsync) final {

    if (mVsync != vsync) {
      mVsync = vsync;
      setSwapInterval (mVsync);
      }
    }
  //}}}
  //{{{
  virtual void toggleVsync() final {

    mVsync = !mVsync;
    setSwapInterval (mVsync);
    }
  //}}}

  //{{{
  virtual void setFullScreen (bool fullScreen) final {

    if (mFullScreen != fullScreen) {
      mFullScreen = fullScreen;
      mActionFullScreen = true;
      }
    }
  //}}}
  //{{{
  virtual void toggleFullScreen() final {

    mFullScreen = !mFullScreen;
    mActionFullScreen = true;
    }
  //}}}

  // actions
  //{{{
  virtual bool pollEvents() final {

    if (mActionFullScreen) {
      // fullScreen
      if (mFullScreen) {
        // save windowPos and windowSize
        glfwGetWindowPos (mWindow, &mWindowPos.x, &mWindowPos.y);
        glfwGetWindowSize (mWindow, &mWindowSize.x, &mWindowSize.y);

        // get resolution of monitor
        const GLFWvidmode* vidMode = glfwGetVideoMode (glfwGetPrimaryMonitor());

        // switch to full screen
        glfwSetWindowMonitor  (mWindow, mMonitor, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
        }
      else
        // restore last window size and position
        glfwSetWindowMonitor (mWindow, nullptr, mWindowPos.x, mWindowPos.y, mWindowSize.x, mWindowSize.y, 0);

      mActionFullScreen = false;
      }

    if (glfwWindowShouldClose (mWindow))
      return false;

    glfwPollEvents();
    return true;
    }
  //}}}
  virtual void newFrame() final { ImGui_ImplGlfw_NewFrame(); }
  //{{{
  virtual void present() final {

    #if defined(VULKAN)
      //{{{  vulkan
      if (gSwapChainRebuild)
        return;

      VkSemaphore renderCompleteSem = vulkanWindow->FrameSemaphores[vulkanWindow->SemaphoreIndex].RenderCompleteSemaphore;

      VkPresentInfoKHR info = {};
      info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      info.waitSemaphoreCount = 1;
      info.pWaitSemaphores = &renderCompleteSem;
      info.swapchainCount = 1;
      info.pSwapchains = &vulkanWindow->Swapchain;
      info.pImageIndices = &vulkanWindow->FrameIndex;

      VkResult result = vkQueuePresentKHR (gQueue, &info);
      if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        gSwapChainRebuild = true;
        return;
        }

      checkVkResult (result);

      // now we can use next set of semaphores
      vulkanWindow->SemaphoreIndex = (vulkanWindow->SemaphoreIndex + 1) % vulkanWindow->ImageCount;
      //}}}
    #else
      glfwSwapBuffers (mWindow);
    #endif
    }
  //}}}

  // static for glfw callback
  inline static function <void (int width, int height)> mResizeCallback ;
  inline static function <void (vector<string> dropItems)> mDropCallback;

  #if defined(VULKAN)
    //{{{  vulkan statics
    //{{{
    static void checkVkResult (VkResult result) {

      if (result == 0)
        return;

      cLog::log (LOGERROR, fmt::format ("vulkan failed:{}", result));

      if (result < 0)
        abort();
      }
    //}}}

    inline static VkDevice gDevice = VK_NULL_HANDLE;
    inline static VkQueue gQueue = VK_NULL_HANDLE;

    inline static bool gSwapChainRebuild = false;

    inline static ImGui_ImplVulkanH_Window* vulkanWindow = nullptr;
    //}}}
  #endif

private:
  //{{{
  void setSwapInterval (bool vsync) {
    #if defined(VULKAN)
      cLog::log (LOGINFO, fmt::format ("setSwapInterval vulkan {}", vsync));
      if (vsync) {
        VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_FIFO_KHR };
        vulkanWindow->PresentMode = ImGui_ImplVulkanH_SelectPresentMode (gPhysicalDevice,
                                                                         vulkanWindow->Surface,
                                                                         &presentModes[0], IM_ARRAYSIZE(presentModes));
        }
      else {
        VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_MAILBOX_KHR,
                                            VK_PRESENT_MODE_IMMEDIATE_KHR,
                                            VK_PRESENT_MODE_FIFO_KHR };
        vulkanWindow->PresentMode = ImGui_ImplVulkanH_SelectPresentMode (gPhysicalDevice,
                                                                         vulkanWindow->Surface,
                                                                         &presentModes[0], IM_ARRAYSIZE(presentModes));
        }
    #else
      glfwSwapInterval (vsync ? 1 : 0);
    #endif
    }
  //}}}

  // static for glfw callback
  //{{{
  static void glfwErrorCallback (int error, const char* description) {
    cLog::log (LOGERROR, fmt::format ("glfw failed {} {}", error, description));
    }
  //}}}
  //{{{
  static void keyCallback (GLFWwindow* window, int key, int scancode, int action, int mode) {
  // glfw key callback exit

    (void)scancode;
    (void)mode;
    if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) // exit
      glfwSetWindowShouldClose (window, true);
    }
  //}}}
  //{{{
  static void framebufferSizeCallback (GLFWwindow* window, int width, int height) {

    (void)window;
    mResizeCallback (width, height);
    }
  //}}}
  //{{{
  static void dropCallback (GLFWwindow* window, int count, const char** paths) {

    (void)window;
    vector<string> dropItems;
    for (int i = 0;  i < count;  i++)
      dropItems.push_back (paths[i]);

    mDropCallback (dropItems);
    }
  //}}}

  // window vars
  GLFWmonitor* mMonitor = nullptr;
  GLFWwindow* mWindow = nullptr;
  cPoint mWindowPos = { 0,0 };
  cPoint mWindowSize = { 0,0 };
  bool mActionFullScreen = false;

  #if defined(VULKAN)
    //{{{  vulkan
    //{{{
    void initVulkan (const char** extensions, uint32_t numExtensions) {

      // create Vulkan Instance
      VkInstanceCreateInfo instanceCreateInfo = {};
      instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      instanceCreateInfo.enabledExtensionCount = numExtensions;
      instanceCreateInfo.ppEnabledExtensionNames = extensions;

      VkResult result;

      #ifdef VALIDATION
        //{{{  create with validation layers
        cLog::log (LOGINFO, fmt::format ("using validation"));

        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        instanceCreateInfo.enabledLayerCount = 1;
        instanceCreateInfo.ppEnabledLayerNames = layers;

        // enable debug report extension (we need additional storage
        // so we duplicate the user array to add our new extension to it)
        const char** extensionsExt = (const char**)malloc (sizeof(const char*) * (numExtensions + 1));
        memcpy (extensionsExt, extensions, numExtensions * sizeof(const char*));
        extensionsExt[numExtensions] = "VK_EXT_debug_report";
        instanceCreateInfo.enabledExtensionCount = numExtensions + 1;
        instanceCreateInfo.ppEnabledExtensionNames = extensionsExt;

        // create vulkanInstance
        result = vkCreateInstance (&instanceCreateInfo, gAllocator, &gInstance);
        checkVkResult (result);
        free (extensionsExt);

        // get function pointer (required for any extensions)
        auto vkCreateDebugReportCallbackEXT =
          (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr (gInstance, "vkCreateDebugReportCallbackEXT");
        IM_ASSERT (vkCreateDebugReportCallbackEXT != NULL);

        // setup the debugReportCallback
        VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
        debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                              VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                              VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debugReportCallbackCreateInfo.pfnCallback = debugReport;
        debugReportCallbackCreateInfo.pUserData = NULL;

        result = vkCreateDebugReportCallbackEXT (gInstance, &debugReportCallbackCreateInfo, gAllocator, &gDebugReport);
        checkVkResult (result);
        //}}}
      #else
        // create without validation layers
        result = vkCreateInstance (&instanceCreateInfo, gAllocator, &gInstance);
        checkVkResult (result);
        (void)gDebugReport;
      #endif

      //{{{  select gpu
      #define VK_API_VERSION_MAJOR(version) (((uint32_t)(version) >> 22) & 0x7FU)
      #define VK_API_VERSION_MINOR(version) (((uint32_t)(version) >> 12) & 0x3FFU)
      #define VK_API_VERSION_PATCH(version) ((uint32_t)(version) & 0xFFFU)

      uint32_t numGpu;
      result = vkEnumeratePhysicalDevices (gInstance, &numGpu, NULL);
      checkVkResult (result);

      if (!numGpu) {
        //{{{  error, log, exit
        cLog::log (LOGERROR, fmt::format ("queueFamilyCount zero"));
        exit(1);
        }
        //}}}

      VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc (sizeof(VkPhysicalDevice) * numGpu);
      result = vkEnumeratePhysicalDevices (gInstance, &numGpu, gpus);
      checkVkResult (result);

      for (uint32_t i = 0; i < numGpu; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties (gpus[i], &properties);
        cLog::log (LOGINFO, fmt::format ("gpu:{} api {}.{}.{} type:{} {} driver:{}",
                (int)i,
                VK_API_VERSION_MAJOR(properties.apiVersion),
                VK_API_VERSION_MINOR(properties.apiVersion),
                VK_API_VERSION_PATCH(properties.apiVersion),
                properties.deviceType, properties.deviceName,
                properties.driverVersion));
        }

      // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available
      // This covers most common cases (multi-gpu/integrated+dedicated graphics)
      // Handling more complicated setups (multiple dedicated GPUs) is out of scope of this sample.
      int useGpu = 0;
      for (uint32_t i = 0; i < numGpu; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties (gpus[i], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
          useGpu = i;
          break;
          }
        }

      gPhysicalDevice = gpus[useGpu];
      cLog::log (LOGINFO, fmt::format ("useGpu:{}", (int)useGpu));

      free (gpus);
      //}}}
      //{{{  select graphics queue family
      uint32_t numQueueFamily;
      vkGetPhysicalDeviceQueueFamilyProperties (gPhysicalDevice, &numQueueFamily, NULL);

      if (!numQueueFamily)
        cLog::log (LOGERROR, fmt::format ("queueFamilyCount zero"));

      VkQueueFamilyProperties* queueFamilyProperties =
        (VkQueueFamilyProperties*)malloc (sizeof(VkQueueFamilyProperties) * numQueueFamily);
      vkGetPhysicalDeviceQueueFamilyProperties (gPhysicalDevice, &numQueueFamily, queueFamilyProperties);

      for (uint32_t i = 0; i < numQueueFamily; i++)
        cLog::log (LOGINFO, fmt::format ("queue:{} count:{} queueFlags:{}",
                 i, queueFamilyProperties[i].queueCount, queueFamilyProperties[i].queueFlags));

      for (uint32_t i = 0; i < numQueueFamily; i++)
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          gQueueFamily = i;
          break;
          }

      free (queueFamilyProperties);

      IM_ASSERT(gQueueFamily != (uint32_t)-1);
      //}}}
      //{{{  create logical device with 1 queue
      int numDeviceExtension = 1;

      const char* deviceExtensions[] = { "VK_KHR_swapchain" };
      const float queuePriority[] = { 1.0f };

      VkDeviceQueueCreateInfo deviceQueueCreateInfo[1] = {};
      deviceQueueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      deviceQueueCreateInfo[0].queueFamilyIndex = gQueueFamily;
      deviceQueueCreateInfo[0].queueCount = 1;
      deviceQueueCreateInfo[0].pQueuePriorities = queuePriority;

      VkDeviceCreateInfo deviceCreateInfo = {};
      deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      deviceCreateInfo.queueCreateInfoCount = sizeof(deviceQueueCreateInfo) / sizeof(deviceQueueCreateInfo[0]);
      deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;
      deviceCreateInfo.enabledExtensionCount = numDeviceExtension;
      deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

      result = vkCreateDevice (gPhysicalDevice, &deviceCreateInfo, gAllocator, &gDevice);
      checkVkResult (result);

      vkGetDeviceQueue (gDevice, gQueueFamily, 0, &gQueue);
      //}}}
      //{{{  create descriptor pool
      VkDescriptorPoolSize descriptorPoolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

      VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
      descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
      descriptorPoolCreateInfo.maxSets = 1000 * IM_ARRAYSIZE(descriptorPoolSizes);
      descriptorPoolCreateInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(descriptorPoolSizes);
      descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

      result = vkCreateDescriptorPool (gDevice, &descriptorPoolCreateInfo, gAllocator, &gDescriptorPool);
      checkVkResult (result);
      //}}}
      }
    //}}}
    //{{{
    void initVulkanWindow (ImGui_ImplVulkanH_Window* vulkanWindow, VkSurfaceKHR surface, int width, int height) {
    // All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
    // Your real engine/app may not use them.

      vulkanWindow->Surface = surface;

      // check WSI support
      VkBool32 result;
      vkGetPhysicalDeviceSurfaceSupportKHR (gPhysicalDevice, gQueueFamily, vulkanWindow->Surface, &result);
      if (result != VK_TRUE) {
        cLog::log (LOGERROR, fmt::format (" error, no WSI support on physical device"));
        exit (-1);
        }

      // select surfaceFormat
      const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM,
                                                     VK_FORMAT_R8G8B8A8_UNORM,
                                                     VK_FORMAT_B8G8R8_UNORM,
                                                     VK_FORMAT_R8G8B8_UNORM };
      const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
      vulkanWindow->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat (gPhysicalDevice,
                                                                           vulkanWindow->Surface,
                                                                           requestSurfaceImageFormat,
                                                                           (size_t)IM_ARRAYSIZE (requestSurfaceImageFormat),
                                                                           requestSurfaceColorSpace);
      // select vsync presentMode
      VkPresentModeKHR presentModes[] = { //VK_PRESENT_MODE_MAILBOX_KHR,
                                          //VK_PRESENT_MODE_IMMEDIATE_KHR,
                                          VK_PRESENT_MODE_FIFO_KHR };
      vulkanWindow->PresentMode = ImGui_ImplVulkanH_SelectPresentMode (gPhysicalDevice,
                                                                       vulkanWindow->Surface,
                                                                       &presentModes[0], IM_ARRAYSIZE(presentModes));

      cLog::log (LOGINFO, fmt::format ("use presentMode:{}", vulkanWindow->PresentMode));

      // create swapChain, renderPass, framebuffer, etc.
      IM_ASSERT (gMinImageCount >= 2);
      ImGui_ImplVulkanH_CreateOrResizeWindow (gInstance, gPhysicalDevice, gDevice,
                                              vulkanWindow, gQueueFamily, gAllocator, width, height, gMinImageCount);
      }
    //}}}
    //{{{
    void uploadFonts (ImGui_ImplVulkanH_Window* vulkanWindow) {
    //  upload fonts texture

      VkCommandPool commandPool = vulkanWindow->Frames[vulkanWindow->FrameIndex].CommandPool;
      VkCommandBuffer commandBuffer = vulkanWindow->Frames[vulkanWindow->FrameIndex].CommandBuffer;

      VkResult result = vkResetCommandPool (gDevice, commandPool, 0);
      checkVkResult (result);

      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      result = vkBeginCommandBuffer (commandBuffer, &begin_info);
      checkVkResult (result);

      ImGui_ImplVulkan_CreateFontsTexture (commandBuffer);

      VkSubmitInfo submitInfo = {};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffer;
      result = vkEndCommandBuffer (commandBuffer);
      checkVkResult (result);

      result = vkQueueSubmit (gQueue, 1, &submitInfo, VK_NULL_HANDLE);
      checkVkResult (result);

      result = vkDeviceWaitIdle (gDevice);
      checkVkResult (result);

      ImGui_ImplVulkan_DestroyFontUploadObjects();
      }
    //}}}

    #ifdef VALIDATION
      //{{{
      static VKAPI_ATTR VkBool32 VKAPI_CALL debugReport (VkDebugReportFlagsEXT flags,
                                                         VkDebugReportObjectTypeEXT objectType,
                                                         uint64_t object,
                                                         size_t location,
                                                         int32_t messageCode,
                                                         const char* pLayerPrefix,
                                                         const char* pMessage,
                                                         void* pUserData) {

        (void)flags;
        (void)object;
        (void)location;
        (void)messageCode;
        (void)pUserData;
        (void)pLayerPrefix;

        cLog::log (LOGERROR, fmt::format ("vkDebugReport type:{}:{}", objectType, pMessage));
        return VK_FALSE;
        }
      //}}}
    #endif

    // vulkan vars
    VkInstance gInstance = VK_NULL_HANDLE;
    VkPhysicalDevice gPhysicalDevice = VK_NULL_HANDLE;
    uint32_t gQueueFamily = (uint32_t)-1;
    VkPipelineCache gPipelineCache = VK_NULL_HANDLE;
    VkDescriptorPool gDescriptorPool = VK_NULL_HANDLE;

    ImGui_ImplVulkanH_Window gMainWindowData;
    int gMinImageCount = 2;

    // vulkan static vars
    inline static VkAllocationCallbacks* gAllocator = NULL;
    inline static VkDebugReportCallbackEXT gDebugReport = VK_NULL_HANDLE;
    //}}}
  #endif
  };
//}}}

// temp
#if defined(VULKAN)
  #include "cVulkanGraphics.h"
#endif

// cApp
//{{{
cApp::cApp (const string& name, const cPoint& windowSize, bool fullScreen, bool vsync) {

  // create platform
  cGlfwPlatform* glfwPlatform = new cGlfwPlatform (name);
  if (!glfwPlatform || !glfwPlatform->init (windowSize)) {
    cLog::log (LOGERROR, "cApp - glfwPlatform init failed");
    return;
    }

  // create graphics
  #if defined(GL_3)
    mGraphics = new cGL3Graphics (glfwPlatform->getShaderVersion());
  #elif defined(GLES_3_0) || defined(GLES_3_1) || defined(GLES_3_2)
    mGraphics = new cGLES3Graphics (glfwPlatform->getShaderVersion());
  #elif defined(VULKAN)
    mGraphics = new cVulkanGraphics();
  #else
    #error cGlfwApp.cpp unrecognised BUILD_GRAPHICS cmake option
  #endif

  if (mGraphics && mGraphics->init()) {
    // set callbacks
    glfwPlatform->mResizeCallback = [&](int width, int height) noexcept { windowResize (width, height); };
    glfwPlatform->mDropCallback = [&](vector<string> dropItems) noexcept { drop (dropItems); };

    // fullScreen, vsync
    mPlatform = glfwPlatform;
    mPlatform->setFullScreen (fullScreen);
    mPlatform->setVsync (vsync);
    mPlatformDefined = true;
    }
  else
    cLog::log (LOGERROR, "cApp - graphics init failed");
  }
//}}}
//{{{
cApp::~cApp() {

  delete mGraphics;
  delete mPlatform;
  }
//}}}

// get
//{{{
chrono::system_clock::time_point cApp::getNow() {
// get time_point with daylight saving correction
// - should be a C++20 timezone thing, but not yet

  // get daylight saving flag
  time_t current_time;
  time (&current_time);
  struct tm* timeinfo = localtime (&current_time);
  //cLog::log (LOGINFO, fmt::format ("dst {}", timeinfo->tm_isdst));

  // UTC->BST only
  return std::chrono::system_clock::now() + std::chrono::hours ((timeinfo->tm_isdst == 1) ? 1 : 0);
  }
//}}}

// callback
//{{{
void cApp::windowResize (int width, int height) {

  (void)width;
  (void)height;
  mGraphics->newFrame();
  mPlatform->newFrame();
  ImGui::NewFrame();

  cUI::render (*this);
  ImGui::Render();
  mGraphics->renderDrawData();

  mPlatform->present();
  }
//}}}

// main loop
//{{{
void cApp::mainUILoop() {

  while (mPlatform->pollEvents()) {
    mGraphics->newFrame();
    mPlatform->newFrame();
    ImGui::NewFrame();

    cUI::render (*this);
    ImGui::Render();
    mGraphics->renderDrawData();

    #if defined(BUILD_DOCKING)
      if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent (backupCurrentContext);
        }
    #endif

    mPlatform->present();
    }
  }
//}}}
