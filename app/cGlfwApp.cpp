// cGlfwApp.cpp - glfw + openGL2,openGL3,openGLES3_x,vulkan app framework
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

#if defined(GL_2_1) || defined(GL_3)
  #include <glad/glad.h>
#endif

// glfw
#include <GLFW/glfw3.h>

// imGui
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#if defined(GL_2_1)
  #include <backends/imgui_impl_opengl2.h>
#elif defined(VULKAN)
  #include <backends/imgui_impl_vulkan.h>
#else
  #include <backends/imgui_impl_opengl3.h>
#endif

#include "cApp.h"
#include "cPlatform.h"
#include "cGraphics.h"

#include "../ui/cUI.h"

#include "../utils/cLog.h"

using namespace std;
//}}}

#if defined(VULKAN)
  //{{{  vulkan static vars
  static VkAllocationCallbacks*   gAllocator = NULL;
  static VkInstance               gInstance = VK_NULL_HANDLE;
  static VkPhysicalDevice         gPhysicalDevice = VK_NULL_HANDLE;
  static VkDevice                 gDevice = VK_NULL_HANDLE;
  static uint32_t                 gQueueFamily = (uint32_t)-1;
  static VkQueue                  gQueue = VK_NULL_HANDLE;
  static VkDebugReportCallbackEXT gDebugReport = VK_NULL_HANDLE;
  static VkPipelineCache          gPipelineCache = VK_NULL_HANDLE;
  static VkDescriptorPool         gDescriptorPool = VK_NULL_HANDLE;

  static ImGui_ImplVulkanH_Window gMainWindowData;
  static int                      gMinImageCount = 2;
  static bool                     gSwapChainRebuild = false;

  static ImGui_ImplVulkanH_Window* vulkanWindow = nullptr;
  //}}}
  //{{{  vulkan functions
  //}}}
  //{{{
  //int main (int, char**) {

    //// setup glfw
    //glfwSetErrorCallback (glfwErrorCallback);
    //if (!glfwInit())
      //return 1;

    //// setup glfw window
    //glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
    //GLFWwindow* glfwWindow = glfwCreateWindow (1280, 720, "Dear ImGui GLFW+Vulkan example", NULL, NULL);

    //// setup vulkan
    //if (!glfwVulkanSupported()) {
      //printf ("glfw vulkan not supported\n");
      //return 1;
      //}

    //// get glfw required vulkan extensions
    //uint32_t extensionsCount = 0;
    //const char** extensions = glfwGetRequiredInstanceExtensions (&extensionsCount);
    //for (uint32_t i = 0; i < extensionsCount; i++)
      //printf ("glfwVulkanExt:%d %s\n", int(i), extensions[i]);
    //setupVulkan (extensions, extensionsCount);

    //// create windowSurface
    //VkSurfaceKHR surface;
    //VkResult result = glfwCreateWindowSurface (gInstance, glfwWindow, gAllocator, &surface);
    //checkVkResult (result);

    //// create framebuffers
    //int width, height;
    //glfwGetFramebufferSize (glfwWindow, &width, &height);
    //ImGui_ImplVulkanH_Window* vulkanWindow = &gMainWindowData;
    //setupVulkanWindow (vulkanWindow, surface, width, height);

    //// setup imGui context
    //IMGUI_CHECKVERSION();
    //ImGui::CreateContext();
    //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    //#ifdef DOCKING
      //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
      //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    //#endif
    ////io.ConfigViewportsNoAutoMerge = true;
    ////io.ConfigViewportsNoTaskBarIcon = true;

    //ImGui::StyleColorsDark();

    //#ifdef DOCKING
      //ImGuiStyle& style = ImGui::GetStyle();
      //if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        //style.WindowRounding = 0.0f;
        //style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        //}
    //#endif

    //// setup Platform/Renderer backends
    //ImGui_ImplGlfw_InitForVulkan (glfwWindow, true);

    //ImGui_ImplVulkan_InitInfo vulkanInitInfo = {};
    //vulkanInitInfo.Instance = gInstance;
    //vulkanInitInfo.PhysicalDevice = gPhysicalDevice;
    //vulkanInitInfo.Device = gDevice;
    //vulkanInitInfo.QueueFamily = gQueueFamily;
    //vulkanInitInfo.Queue = gQueue;
    //vulkanInitInfo.PipelineCache = gPipelineCache;
    //vulkanInitInfo.DescriptorPool = gDescriptorPool;
    //vulkanInitInfo.Allocator = gAllocator;
    //vulkanInitInfo.MinImageCount = gMinImageCount;
    //vulkanInitInfo.ImageCount = vulkanWindow->ImageCount;
    //vulkanInitInfo.CheckVkResultFn = checkVkResult;
    //ImGui_ImplVulkan_Init (&vulkanInitInfo, vulkanWindow->RenderPass);

    //uploadFonts (vulkanWindow);

    //// Our state
    //bool show_demo_window = true;
    //bool show_another_window = false;
    //ImVec4 clearColor = ImVec4 (0.45f, 0.55f, 0.60f, 1.00f);

    //// Main loop
    //while (!glfwWindowShouldClose (glfwWindow)) {
      //glfwPollEvents();

      //// Resize swap chain?
      //if (gSwapChainRebuild) {
        //int width;
        //int height;
        //glfwGetFramebufferSize (glfwWindow, &width, &height);
        //if ((width > 0) && (height > 0)) {
          //ImGui_ImplVulkan_SetMinImageCount (gMinImageCount);
          //ImGui_ImplVulkanH_CreateOrResizeWindow (gInstance, gPhysicalDevice, gDevice,
                                                  //&gMainWindowData, gQueueFamily,
                                                  //gAllocator, width, height, gMinImageCount);
          //gMainWindowData.FrameIndex = 0;
          //gSwapChainRebuild = false;
          //}
        //}

      //// Start the Dear ImGui frame
      //ImGui_ImplVulkan_NewFrame();
      //ImGui_ImplGlfw_NewFrame();
      //ImGui::NewFrame();

      //// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
      //if (show_demo_window)
        //ImGui::ShowDemoWindow (&show_demo_window);

      //{{{  Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
      //{
      //static float f = 0.0f;
      //static int counter = 0;

      //ImGui::Begin ("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

      //ImGui::Text ("This is some useful text.");               // Display some text (you can use a format strings too)
      //ImGui::Checkbox ("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
      //ImGui::Checkbox ("Another Window", &show_another_window);

      //ImGui::SliderFloat ("float", &f, 0.0f, 1.0f);           // Edit 1 float using a slider from 0.0f to 1.0f
      //ImGui::ColorEdit3 ("clear color", (float*)&clearColor); // Edit 3 floats representing a color

      //if (ImGui::Button ("Button"))
        //counter++;
      //ImGui::SameLine();
      //ImGui::Text ("counter = %d", counter);

      //ImGui::Text ("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      //ImGui::End();
      //}
      //}}}
      //{{{  Show another simple window.
      //if (show_another_window) {
        //ImGui::Begin ("Another Window", &show_another_window);
        //ImGui::Text ("Hello from another window!");
        //if (ImGui::Button ("Close Me"))
          //show_another_window = false;
        //ImGui::End();
        //}
      //}}}

      //// Rendering
      //ImGui::Render();
      //ImDrawData* drawData = ImGui::GetDrawData();
      //const bool minimized = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);
      //vulkanWindow->ClearValue.color.float32[0] = clearColor.x * clearColor.w;
      //vulkanWindow->ClearValue.color.float32[1] = clearColor.y * clearColor.w;
      //vulkanWindow->ClearValue.color.float32[2] = clearColor.z * clearColor.w;
      //vulkanWindow->ClearValue.color.float32[3] = clearColor.w;
      //if (!minimized)
        //renderDrawData (vulkanWindow, drawData);

      //#ifdef DOCKING
        //if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
          //ImGui::UpdatePlatformWindows();
          //ImGui::RenderPlatformWindowsDefault();
          //}
      //#endif

      //if (!minimized)
        //present (vulkanWindow);
      //}

    //// cleanup
    //result = vkDeviceWaitIdle (gDevice);
    //checkVkResult (result);

    //ImGui_ImplVulkan_Shutdown();
    //ImGui_ImplGlfw_Shutdown();
    //ImGui::DestroyContext();

    //cleanupVulkanWindow();
    //cleanupVulkan();

    //glfwDestroyWindow (glfwWindow);
    //glfwTerminate();

    //return 0;
    //}
  //}}}
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
      cleanupVulkanWindow();
      cleanupVulkan();
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
      //{{{  setup vulkan
      if (!glfwVulkanSupported()) {
        cLog::log (LOGERROR, fmt::format ("glfw vulkan not supported"));
        return 1;
        }

      // get glfw required vulkan extensions
      uint32_t extensionsCount = 0;
      const char** extensions = glfwGetRequiredInstanceExtensions (&extensionsCount);
      for (uint32_t i = 0; i < extensionsCount; i++)
        cLog::log (LOGINFO, fmt::format ("glfwVulkanExt:{} {}", int(i), extensions[i]));
      setupVulkan (extensions, extensionsCount);

      // create windowSurface
      VkSurfaceKHR surface;
      VkResult result = glfwCreateWindowSurface (gInstance, mWindow, gAllocator, &surface);
      checkVkResult (result);

      // create framebuffers
      int width, height;
      glfwGetFramebufferSize (mWindow, &width, &height);

      vulkanWindow = &gMainWindowData;
      setupVulkanWindow (vulkanWindow, surface, width, height);
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
      // Enable Docking
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

      // Enable Multi-Viewport / Platform Windows
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
      //{{{  fullScreen
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
      //}}}

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
      vulkanPresent (vulkanWindow);
    #else
      glfwSwapBuffers (mWindow);
    #endif
    }
  //}}}

  // static for glfw callback
  inline static function <void (int width, int height)> mResizeCallback ;
  inline static function <void (vector<string> dropItems)> mDropCallback;

  //{{{
  static void checkVkResult (VkResult result) {

    if (result == 0)
      return;

    cLog::log (LOGERROR, fmt::format ("vkResultError:{}", result));

    if (result < 0)
      abort();
    }
  //}}}

private:
  //{{{
  void setSwapInterval (bool vsync) {
    #if defined(VULKAN)
      cLog::log (LOGERROR, fmt::format ("setSwapInterval for vulkan unimplemented {}", vsync));
    #else
      glfwSwapInterval (vsync ? 1 : 0)
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

  // vulkan statics
  //{{{
  static void vulkanPresent (ImGui_ImplVulkanH_Window* vulkanWindow) {

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

  //{{{
  static void setupVulkan (const char** extensions, uint32_t numExtensions) {

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
    #define VK_API_VERSION_VARIANT(version) ((uint32_t)(version) >> 29)
    #define VK_API_VERSION_MAJOR(version) (((uint32_t)(version) >> 22) & 0x7FU)
    #define VK_API_VERSION_MINOR(version) (((uint32_t)(version) >> 12) & 0x3FFU)
    #define VK_API_VERSION_PATCH(version) ((uint32_t)(version) & 0xFFFU)

    uint32_t numGpu;
    result = vkEnumeratePhysicalDevices (gInstance, &numGpu, NULL);
    checkVkResult (result);

    if (!numGpu)
      cLog::log (LOGERROR, fmt::format ("queueFamilyCount zero"));
    IM_ASSERT (numGpu > 0);

    VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc (sizeof(VkPhysicalDevice) * numGpu);
    result = vkEnumeratePhysicalDevices (gInstance, &numGpu, gpus);
    checkVkResult (result);

    for (uint32_t i = 0; i < numGpu; i++) {
      VkPhysicalDeviceProperties properties;
      vkGetPhysicalDeviceProperties (gpus[i], &properties);
      cLog::log (LOGINFO, fmt::format ("gpu:{} var:{} major:{} minor:{} patch:{} type:{} {} api:{} driver:{}",
              (int)i,
              VK_API_VERSION_VARIANT(properties.apiVersion),
              VK_API_VERSION_MAJOR(properties.apiVersion),
              VK_API_VERSION_MINOR(properties.apiVersion),
              VK_API_VERSION_PATCH(properties.apiVersion),
              properties.deviceType, properties.deviceName, properties.apiVersion, properties.driverVersion));
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
    //{{{  create logical device (with 1 queue)
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
    {
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
    }
    //}}}
    }
  //}}}
  //{{{
  static void setupVulkanWindow (ImGui_ImplVulkanH_Window* vulkanWindow, VkSurfaceKHR surface, int width, int height) {
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
  // select presentMode
    #ifdef VSYNC
      VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_FIFO_KHR };
    #else
      VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_MAILBOX_KHR,
                                          VK_PRESENT_MODE_IMMEDIATE_KHR,
                                          VK_PRESENT_MODE_FIFO_KHR };
    #endif

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
  static void uploadFonts (ImGui_ImplVulkanH_Window* vulkanWindow) {
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

  //{{{
  static void cleanupVulkan() {

    vkDestroyDescriptorPool (gDevice, gDescriptorPool, gAllocator);

    #ifdef VALIDATION
      // Remove the debug report callback
      auto vkDestroyDebugReportCallbackEXT =
        (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr (gInstance, "vkDestroyDebugReportCallbackEXT");
      vkDestroyDebugReportCallbackEXT (gInstance, gDebugReport, gAllocator);
    #endif

    vkDestroyDevice (gDevice, gAllocator);
    vkDestroyInstance (gInstance, gAllocator);
    }
  //}}}
  //{{{
  static void cleanupVulkanWindow() {
    ImGui_ImplVulkanH_DestroyWindow (gInstance, gDevice, &gMainWindowData, gAllocator);
    }
  //}}}


  GLFWmonitor* mMonitor = nullptr;
  GLFWwindow* mWindow = nullptr;
  cPoint mWindowPos = { 0,0 };
  cPoint mWindowSize = { 0,0 };

  bool mActionFullScreen = false;
  };
//}}}

// cGraphics interface
#if defined(GL_2_1)
  //{{{
  class cGL2Gaphics : public cGraphics {
  // mostly unimplemented, some just copied rom gl3
  public:
    //{{{
    virtual ~cGL2Gaphics() {
      ImGui_ImplOpenGL2_Shutdown();
      }
    //}}}

    // imgui
    //{{{
    virtual bool init() final {

      // report OpenGL versions
      cLog::log (LOGINFO, fmt::format ("OpenGL {}", glGetString (GL_VERSION)));
      cLog::log (LOGINFO, fmt::format ("- GLSL {}", glGetString (GL_SHADING_LANGUAGE_VERSION)));
      cLog::log (LOGINFO, fmt::format ("- Renderer {}", glGetString (GL_RENDERER)));
      cLog::log (LOGINFO, fmt::format ("- Vendor {}", glGetString (GL_VENDOR)));

      return ImGui_ImplOpenGL2_Init();
      }
    //}}}
    virtual void newFrame() final { ImGui_ImplOpenGL2_NewFrame(); }
    //{{{
    virtual void clear (const cPoint& size) final {

      glViewport (0, 0, size.x, size.y);

      // blend
      uint32_t modeRGB = GL_FUNC_ADD;
      uint32_t modeAlpha = GL_FUNC_ADD;
      glBlendEquationSeparate (modeRGB, modeAlpha);

      uint32_t srcRgb = GL_SRC_ALPHA;
      uint32_t dstRGB = GL_ONE_MINUS_SRC_ALPHA;
      uint32_t srcAlpha = GL_ONE;
      uint32_t dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
      glBlendFuncSeparate (srcRgb, dstRGB, srcAlpha, dstAlpha);

      glEnable (GL_BLEND);

      // disables
      glDisable (GL_SCISSOR_TEST);
      glDisable (GL_CULL_FACE);
      glDisable (GL_DEPTH_TEST);

      // clear
      glClearColor (0.f,0.f,0.f, 0.f);
      glClear (GL_COLOR_BUFFER_BIT);
      }
    //}}}
    virtual void renderDrawData() final { ImGui_ImplOpenGL2_RenderDrawData (ImGui::GetDrawData()); }

    // creates
    virtual cQuad* createQuad (const cPoint& size) final { return new cOpenGL2Quad (size); }
    virtual cQuad* createQuad (const cPoint& size, const cRect& rect) final { return new cOpenGL2Quad (size, rect); }

    virtual cTarget* createTarget() final { return new cOpenGL2Target(); }
    //{{{
    virtual cTarget* createTarget (const cPoint& size, cTarget::eFormat format) final {
      return new cOpenGL2Target (size, format);
      }
    //}}}
    //{{{
    virtual cTarget* createTarget (uint8_t* pixels, const cPoint& size, cTarget::eFormat format) final {
      return new cOpenGL2Target (pixels, size, format);
      }
    //}}}

    virtual cLayerShader* createLayerShader() final { return new cOpenGL2LayerShader(); }
    virtual cPaintShader* createPaintShader() final { return new cOpenGL2PaintShader(); }

    //{{{
    virtual cTexture* createTexture (cTexture::eTextureType textureType, const cPoint& size) final {
    // factory create

      switch (textureType) {
        case cTexture::eRgba:   return new cOpenGL2RgbaTexture (textureType, size);
        case cTexture::eNv12:   return new cOpenGL2Nv12Texture (textureType, size);
        case cTexture::eYuv420: return new cOpenGL2Yuv420Texture (textureType, size);
        default : return nullptr;
        }
      }
    //}}}
    //{{{
    virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) final {
    // factory create

      switch (textureType) {
        case cTexture::eRgba:   return new cOpenGL2RgbaShader();
        case cTexture::eYuv420: return new cOpenGL2Yuv420Shader();
        case cTexture::eNv12:   return new cOpenGL2Nv12Shader();
        default: return nullptr;
        }
      }
    //}}}

  private:
    //{{{
    inline static const string kQuadVertShader =
      "#version 330 core\n"
      "uniform mat4 uModel;"
      "uniform mat4 uProject;"

      "layout (location = 0) in vec2 inPos;"
      "layout (location = 1) in vec2 inTextureCoord;"
      "out vec2 textureCoord;"

      "void main() {"
      "  textureCoord = inTextureCoord;"
      "  gl_Position = uProject * uModel * vec4 (inPos, 0.0, 1.0);"
      "  }";
    //}}}

    //{{{
    class cOpenGL2Quad : public cQuad {
    public:
      //{{{
      cOpenGL2Quad (const cPoint& size) : cQuad(size) {

        // vertexArray
        glGenVertexArrays (1, &mVertexArrayObject);
        glBindVertexArray (mVertexArrayObject);

        // vertices
        glGenBuffers (1, &mVertexBufferObject);
        glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

        glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray (0);

        glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray (1);

        const float widthF = static_cast<float>(size.x);
        const float heightF = static_cast<float>(size.y);
        const float kVertices[] = {
          0.f,   heightF,  0.f,1.f, // tl vertex
          widthF,0.f,      1.f,0.f, // br vertex
          0.f,   0.f,      0.f,0.f, // bl vertex
          widthF,heightF,  1.f,1.f, // tr vertex
          };

        glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

        glGenBuffers (1, &mElementArrayBufferObject);
        glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);

        glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
        }
      //}}}
      //{{{
      cOpenGL2Quad (const cPoint& size, const cRect& rect) : cQuad(size) {

        // vertexArray
        glGenVertexArrays (1, &mVertexArrayObject);
        glBindVertexArray (mVertexArrayObject);

        // vertices
        glGenBuffers (1, &mVertexBufferObject);
        glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

        glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray (0);

        glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray (1);

        const float subLeftF = static_cast<float>(rect.left);
        const float subRightF = static_cast<float>(rect.right);
        const float subBottomF = static_cast<float>(rect.bottom);
        const float subTopF = static_cast<float>(rect.top);

        const float subLeftTexF = subLeftF / size.x;
        const float subRightTexF = subRightF / size.x;
        const float subBottomTexF = subBottomF / size.y;
        const float subTopTexF = subTopF / size.y;

        const float kVertices[] = {
          subLeftF, subTopF,     subLeftTexF, subTopTexF,    // tl
          subRightF,subBottomF,  subRightTexF,subBottomTexF, // br
          subLeftF, subBottomF,  subLeftTexF, subBottomTexF, // bl
          subRightF,subTopF,     subRightTexF,subTopTexF     // tr
          };

        glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

        // indices
        glGenBuffers (1, &mElementArrayBufferObject);
        glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);
        glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
        }
      //}}}
      //{{{
      virtual ~cOpenGL2Quad() {

        glDeleteBuffers (1, &mElementArrayBufferObject);
        glDeleteBuffers (1, &mVertexBufferObject);
        glDeleteVertexArrays (1, &mVertexArrayObject);
        }
      //}}}

      //{{{
      void draw() final {

        glBindVertexArray (mVertexArrayObject);
        glDrawElements (GL_TRIANGLES, mNumIndices, GL_UNSIGNED_BYTE, 0);
        }
      //}}}

    private:
      inline static const uint32_t mNumIndices = 6;
      inline static const uint8_t kIndices[mNumIndices] = {
        0, 1, 2, // 0   0-3
        0, 3, 1  // |\   \|
        };       // 2-1   1

      uint32_t mVertexArrayObject = 0;
      uint32_t mVertexBufferObject = 0;
      uint32_t mElementArrayBufferObject = 0;
      };
    //}}}
    //{{{
    class cOpenGL2Target : public cTarget {
    public:
      //{{{
      cOpenGL2Target() : cTarget ({0,0}) {
      // window Target

        mImageFormat = GL_RGBA;
        mInternalFormat = GL_RGBA;
        }
      //}}}
      //{{{
      cOpenGL2Target (const cPoint& size, eFormat format) : cTarget(size) {

        mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
        mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

        // create empty Target object
        glGenFRameBuffers (1, &mFrameBufferObject);
        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

        // create and add texture to Target object
        glGenTextures (1, &mColorTextureId);

        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, 0);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap (GL_TEXTURE_2D);

        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
        }
      //}}}
      //{{{
      cOpenGL2Target (uint8_t* pixels, const cPoint& size, eFormat format) : cTarget(size) {

        mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
        mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

        // create Target object from pixels
        glGenFramebuffers (1, &mFrameBufferObject);
        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

        // create and add texture to Target object
        glGenTextures (1, &mColorTextureId);

        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap (GL_TEXTURE_2D);

        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
        }
      //}}}
      //{{{
      virtual ~cOpenGL2Target() {

        glDeleteTextures (1, &mColorTextureId);
        glDeleteFrameBuffers (1, &mFrameBufferObject);
        free (mPixels);
        }
      //}}}

      /// gets
      //{{{
      uint8_t* getPixels() final {

        if (!mPixels) {
          // create mPixels, texture pixels shadow buffer
          mPixels = static_cast<uint8_t*>(malloc (getNumPixelBytes()));
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          #if defined(GL_3)
            glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
          #endif
          }

        else if (!mDirtyPixelsRect.isEmpty()) {
          // no openGL glGetTexSubImage, so dirtyPixelsRect not really used, is this correct ???
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          #if defined(GL_3)
            glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
          #endif
          mDirtyPixelsRect = cRect(0,0,0,0);
          }

        return mPixels;
        }
      //}}}

      // sets
      //{{{
      void setSize (const cPoint& size) final {
        if (mFrameBufferObject == 0)
          mSize = size;
        else
          cLog::log (LOGERROR, "unimplmented setSize of non screen Target");
        };
      //}}}
      //{{{
      void setTarget (const cRect& rect) final {
      // set us as target, set viewport to our size, invalidate contents (probably a clear)

        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);
        glViewport (0, 0, mSize.x, mSize.y);

        glDisable (GL_SCISSOR_TEST);
        glDisable (GL_CULL_FACE);
        glDisable (GL_DEPTH_TEST);

        // texture could be changed, add to dirtyPixelsRect
        mDirtyPixelsRect += rect;
        }
      //}}}
      //{{{
      void setBlend() final {

        uint32_t modeRGB = GL_FUNC_ADD;
        uint32_t modeAlpha = GL_FUNC_ADD;

        uint32_t srcRgb = GL_SRC_ALPHA;
        uint32_t dstRGB = GL_ONE_MINUS_SRC_ALPHA;
        uint32_t srcAlpha = GL_ONE;
        uint32_t dstAlpha = GL_ONE_MINUS_SRC_ALPHA;

        glBlendEquationSeparate (modeRGB, modeAlpha);
        glBlendFuncSeparate (srcRgb, dstRGB, srcAlpha, dstAlpha);
        glEnable (GL_BLEND);
        }
      //}}}
      //{{{
      void setSource() final {

        if (mFrameBufferObject) {
          glActiveTexture (GL_TEXTURE0);
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          }
        else
          cLog::log (LOGERROR, "windowTarget cannot be src");
        }
      //}}}

      // actions
      //{{{
      void invalidate() final {

        clear (cColor(0.f,0.f,0.f, 0.f));
        }
      //}}}
      //{{{
      void pixelsChanged (const cRect& rect) final {
      // pixels in rect changed, write back to texture

        if (mPixels) {
          // update y band, quicker x cropped line by line ???
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          glTexSubImage2D (GL_TEXTURE_2D, 0, 0, rect.top, mSize.x, rect.getHeight(),
                           mImageFormat, GL_UNSIGNED_BYTE, mPixels + (rect.top * mSize.x * 4));
          // simpler whole screen
          //glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, mSize.x, mSize.y, 0, mImageFormat, GL_UNSIGNED_BYTE, mPixels);
          }
        }
      //}}}

      //{{{
      void clear (const cColor& color) final {
        glClearColor (color.r,color.g,color.b, color.a);
        glClear (GL_COLOR_BUFFER_BIT);
        }
      //}}}
      //{{{
      void blit (cTarget& src, const cPoint& srcPoint, const cRect& dstRect) final {

        glBindFramebuffer (GL_READ_FRAMEBUFFER, src.getId());
        glBindFramebuffer (GL_DRAW_FRAMEBUFFER, mFrameBufferObject);
        glBlitFramebuffer (srcPoint.x, srcPoint.y, srcPoint.x + dstRect.getWidth(), srcPoint.y + dstRect.getHeight(),
                           dstRect.left, dstRect.top, dstRect.right, dstRect.bottom,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // texture changed, add to dirtyPixelsRect
        mDirtyPixelsRect += dstRect;
        }
      //}}}

      //{{{
      bool checkStatus() final {

        GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);

        switch (status) {
          case GL_FRAMEBUFFER_COMPLETE:
            return true;
          case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            cLog::log (LOGERROR, "Target incomplete: Attachment is NOT complete"); return false;
          case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            cLog::log (LOGERROR, "Target incomplete: No image is attached to FBO"); return false;
          case GL_FRAMEBUFFER_UNSUPPORTED:
            cLog::log (LOGERROR, "Target incomplete: Unsupported by FBO implementation"); return false;
          default:
            cLog::log (LOGERROR, "Target incomplete: Unknown error"); return false;
          }
        }
      //}}}
      //{{{
      void reportInfo() final {

        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

        GLint colorBufferCount = 0;
        glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS, &colorBufferCount);

        GLint multiSampleCount = 0;
        glGetIntegerv (GL_MAX_SAMPLES, &multiSampleCount);

        cLog::log (LOGINFO, fmt::format ("Target maxColorAttach {} masSamples {}", colorBufferCount, multiSampleCount));

        //  print info of the colorbuffer attachable image
        GLint objectType;
        GLint objectId;
        for (int i = 0; i < colorBufferCount; ++i) {
          glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
          if (objectType != GL_NONE) {
            glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                                   GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
            string formatName;
            cLog::log (LOGINFO, fmt::format ("- color{}", i));
            if (objectType == GL_TEXTURE)
              cLog::log (LOGINFO, fmt::format ("  - GL_TEXTURE {}", getTextureParameters (objectId)));
            else if(objectType == GL_RENDERBUFFER)
              cLog::log (LOGINFO, fmt::format ("  - GL_RENDERBUFFER {}", getRenderbufferParameters (objectId)));
            }
          }

        //  print info of the depthbuffer attachable image
        glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                               GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
        if (objectType != GL_NONE) {
          glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
          cLog::log (LOGINFO, fmt::format ("depth"));
          switch (objectType) {
            case GL_TEXTURE:
              cLog::log (LOGINFO, fmt::format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
              break;
            case GL_RENDERBUFFER:
              cLog::log (LOGINFO, fmt::format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
              break;
            }
          }

        // print info of the stencilbuffer attachable image
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                              &objectType);
        if (objectType != GL_NONE) {
          glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
          cLog::log (LOGINFO, fmt::format ("stencil"));
          switch (objectType) {
            case GL_TEXTURE:
              cLog::log (LOGINFO, fmt::format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
              break;
            case GL_RENDERBUFFER:
              cLog::log (LOGINFO, fmt::format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
              break;
            }
          }
        }
      //}}}

    private:
      //{{{
      static string getInternalFormat (uint32_t formatNum) {

        string formatName;

        switch (formatNum) {
          case GL_DEPTH_COMPONENT:   formatName = "GL_DEPTH_COMPONENT"; break;   // 0x1902
          case GL_ALPHA:             formatName = "GL_ALPHA"; break;             // 0x1906
          case GL_RGB:               formatName = "GL_RGB"; break;               // 0x1907
          case GL_RGB8:              formatName = "GL_RGB8"; break;              // 0x8051
          case GL_RGBA4:             formatName = "GL_RGBA4"; break;             // 0x8056
          case GL_RGB5_A1:           formatName = "GL_RGB5_A1"; break;           // 0x8057
          case GL_RGBA8:             formatName = "GL_RGBA8"; break;             // 0x8058
          case GL_RGB10_A2:          formatName = "GL_RGB10_A2"; break;          // 0x8059
          case GL_DEPTH_COMPONENT16: formatName = "GL_DEPTH_COMPONENT16"; break; // 0x81A5
          case GL_DEPTH_COMPONENT24: formatName = "GL_DEPTH_COMPONENT24"; break; // 0x81A6
          case GL_DEPTH_STENCIL:     formatName = "GL_DEPTH_STENCIL"; break;     // 0x84F9
          case GL_DEPTH24_STENCIL8:  formatName = "GL_DEPTH24_STENCIL8"; break;  // 0x88F0

        #if defined(GL_3)
          case GL_STENCIL_INDEX:     formatName = "GL_STENCIL_INDEX"; break;     // 0x1901
          case GL_RGBA:              formatName = "GL_RGBA"; break;              // 0x1908
          case GL_R3_G3_B2:          formatName = "GL_R3_G3_B2"; break;          // 0x2A10
          case GL_RGB4:              formatName = "GL_RGB4"; break;              // 0x804F
          case GL_RGB5:              formatName = "GL_RGB5"; break;              // 0x8050
          case GL_RGB10:             formatName = "GL_RGB10"; break;             // 0x8052
          case GL_RGB12:             formatName = "GL_RGB12"; break;             // 0x8053
          case GL_RGB16:             formatName = "GL_RGB16"; break;             // 0x8054
          case GL_RGBA2:             formatName = "GL_RGBA2"; break;             // 0x8055
          case GL_RGBA12:            formatName = "GL_RGBA12"; break;            // 0x805A
          case GL_RGBA16:            formatName = "GL_RGBA16"; break;            // 0x805B
          case GL_DEPTH_COMPONENT32: formatName = "GL_DEPTH_COMPONENT32"; break; // 0x81A7
        #endif
          //case GL_LUMINANCE:         formatName = "GL_LUMINANCE"; break;         // 0x1909
          //case GL_LUMINANCE_ALPHA:   formatName = "GL_LUMINANCE_ALPHA"; break;   // 0x190A
          //case GL_RGBA32F:           formatName = "GL_RGBA32F"; break;           // 0x8814
          //case GL_RGB32F:            formatName = "GL_RGB32F"; break;            // 0x8815
          //case GL_RGBA16F:           formatName = "GL_RGBA16F"; break;           // 0x881A
          //case GL_RGB16F:            formatName = "GL_RGB16F"; break;            // 0x881B
          default: formatName = fmt::format ("Unknown Format {}", formatNum); break;
          }

        return formatName;
        }
      //}}}
      //{{{
      static string getTextureParameters (uint32_t id) {

        if (glIsTexture(id) == GL_FALSE)
          return "Not texture object";

        #if defined(GL_3)
          glBindTexture (GL_TEXTURE_2D, id);
          int width;
          glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);            // get texture width
          int height;
          glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);          // get texture height
          int formatNum;
          glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &formatNum); // get texture internal format
          return fmt::format (" {} {} {}", width, height, getInternalFormat (formatNum));
        #else
          return "unknown texture";
        #endif

        }
      //}}}
      //{{{
      static string getRenderbufferParameters (uint32_t id) {

        if (glIsRenderbuffer(id) == GL_FALSE)
          return "Not Renderbuffer object";

        int width, height, formatNum, samples;
        glBindRenderbuffer (GL_RENDERBUFFER, id);
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);    // get renderbuffer width
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);  // get renderbuffer height
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &formatNum); // get renderbuffer internal format
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);   // get multisample count

        return fmt::format (" {} {} {} {}", width, height, samples, getInternalFormat (formatNum));
        }
      //}}}
      };
    //}}}

    //{{{
    class cOpenGL2RgbaTexture : public cTexture {
    public:
      //{{{
      cOpenGL2RgbaTexture (eTextureType textureType, const cPoint& size)
          : cTexture(textureType, size) {

        cLog::log (LOGINFO, fmt::format ("creating eRgba texture {}x{}", size.x, size.y));
        glGenTextures (1, &mTextureId);

        glBindTexture (GL_TEXTURE_2D, mTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateMipmap (GL_TEXTURE_2D);
        }
      //}}}
      //{{{
      virtual ~cOpenGL2RgbaTexture() {
        glDeleteTextures (1, &mTextureId);
        }
      //}}}

      virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId; }

      //{{{
      virtual void setPixels (uint8_t** pixels) final {
      // set textures using pixels in ffmpeg avFrame format

        glBindTexture (GL_TEXTURE_2D, mTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels[0]);
        }
      //}}}
      //{{{
      virtual void setSource() final {

        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mTextureId);
        }
      //}}}

    private:
      GLuint mTextureId = 0;
      };
    //}}}
    //{{{
    class cOpenGL2Nv12Texture : public cTexture {
    public:
      //{{{
      cOpenGL2Nv12Texture (eTextureType textureType, const cPoint& size)
          : cTexture(textureType, size) {

        //cLog::log (LOGINFO, fmt::format ("creating eNv12 texture {}x{}", size.x, size.y));

        glGenTextures (2, mTextureId.data());

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // uv texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        //glTexImage2D (GL_TEXTURE_2D, 0, GL_RG, size.x/2, size.y/2, 0, GL_RG, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
      //}}}
      //{{{
      virtual ~cOpenGL2Nv12Texture() {
        //glDeleteTextures (2, mTextureId.data());
        glDeleteTextures (1, &mTextureId[0]);
        glDeleteTextures (1, &mTextureId[1]);

        cLog::log (LOGINFO, fmt::format ("deleting eVv12 texture {}x{}", mSize.x, mSize.y));
        }
      //}}}

      virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId[0]; }  // luma only

      //{{{
      virtual void setPixels (uint8_t** pixels) final {
      // set textures using pixels in ffmpeg avFrame format

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[0]);

        // uv texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        //glTexImage2D (GL_TEXTURE_2D, 0, GL_RG, mSize.x/2, mSize.y/2, 0, GL_RG, GL_UNSIGNED_BYTE, pixels[1]);
        }
      //}}}
      //{{{
      virtual void setSource() final {

        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);

        glActiveTexture (GL_TEXTURE1);
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        }
      //}}}

    private:
      array <GLuint,2> mTextureId = {0};
      };
    //}}}
    //{{{
    class cOpenGL2Yuv420Texture : public cTexture {
    public:
      //{{{
      cOpenGL2Yuv420Texture (eTextureType textureType, const cPoint& size)
          : cTexture(textureType, size) {

        //cLog::log (LOGINFO, fmt::format ("creating eYuv420 texture {}x{}", size.x, size.y));

        glGenTextures (3, mTextureId.data());

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // u texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // v texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
      //}}}
      //{{{
      virtual ~cOpenGL2Yuv420Texture() {
        //glDeleteTextures (3, mTextureId.data());
        glDeleteTextures (1, &mTextureId[0]);
        glDeleteTextures (1, &mTextureId[1]);
        glDeleteTextures (1, &mTextureId[2]);

        cLog::log (LOGINFO, fmt::format ("deleting eYuv420 texture {}x{}", mSize.x, mSize.y));
        }
      //}}}

      virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId[0]; }   // luma only

      //{{{
      virtual void setPixels (uint8_t** pixels) final {
      // set textures using pixels in ffmpeg avFrame format

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[0]);

        // u texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[1]);

        // v texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[2]);
        }
      //}}}
      //{{{
      virtual void setSource() final {

        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);

        glActiveTexture (GL_TEXTURE1);
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);

        glActiveTexture (GL_TEXTURE2);
        glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
        }
      //}}}

    private:
      array <GLuint,3> mTextureId = {0};
      };
    //}}}

    //{{{
    class cOpenGL2RgbaShader : public cTextureShader {
    public:

      cOpenGL2RgbaShader() : cTextureShader() {
        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D uSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
          "  outColor = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y));"
          //"  outColor /= outColor.w;"
          "  }";
        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGL2RgbaShader() {
        glDeleteProgram (mId);
        }
      //}}}

      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void use() final {

        glUseProgram (mId);
        glUniform1i (glGetUniformLocation (mId, "uSampler"), 0);
        }
      //}}}
      };
    //}}}
    //{{{
    class cOpenGL2Nv12Shader : public cTextureShader {
    public:
      cOpenGL2Nv12Shader() : cTextureShader() {
        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D ySampler;"
          "uniform sampler2D uvSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
            "float y = texture (ySampler, vec2 (textureCoord.x, -textureCoord.y)).r;"
            "float u = texture (uvSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
            "float v = texture (uvSampler, vec2 (textureCoord.x, -textureCoord.y)).g - 0.5;"
            "y = (y - 0.0625) * 1.1643;"
            "outColor.r = y + (1.5958 * v);"
            "outColor.g = y - (0.39173 * u) - (0.81290 * v);"
            "outColor.b = y + (2.017 * u);"
            "outColor.a = 1.0;"
            "}";

        mId = compileShader (kQuadVertShader, kFragShader);
        }

      //{{{
      virtual ~cOpenGL2Nv12Shader() {
        glDeleteProgram (mId);
        }
      //}}}

      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void use() final {

        //cLog::log (LOGINFO, "video use");
        glUseProgram (mId);

        glUniform1i (glGetUniformLocation (mId, "ySampler"), 0);
        glUniform1i (glGetUniformLocation (mId, "uvSampler"), 1);
        }
      //}}}
      };
    //}}}
    //{{{
    class cOpenGL2Yuv420Shader : public cTextureShader {
    public:
      cOpenGL2Yuv420Shader() : cTextureShader() {
        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D ySampler;"
          "uniform sampler2D uSampler;"
          "uniform sampler2D vSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
            "float y = texture (ySampler, vec2 (textureCoord.x, -textureCoord.y)).r;"
            "float u = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
            "float v = texture (vSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
            "y = (y - 0.0625) * 1.1643;"
            "outColor.r = y + (1.5958 * v);"
            "outColor.g = y - (0.39173 * u) - (0.81290 * v);"
            "outColor.b = y + (2.017 * u);"
            "outColor.a = 1.0;"
            "}";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGL2Yuv420Shader() {
        glDeleteProgram (mId);
        }
      //}}}

      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void use() final {

        //cLog::log (LOGINFO, "video use");
        glUseProgram (mId);

        glUniform1i (glGetUniformLocation (mId, "ySampler"), 0);
        glUniform1i (glGetUniformLocation (mId, "uSampler"), 1);
        glUniform1i (glGetUniformLocation (mId, "vSampler"), 2);
        }
      //}}}
      };
    //}}}

    //{{{
    class cOpenGL2LayerShader : public cLayerShader {
    public:
      cOpenGL2LayerShader() : cLayerShader() {

        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D uSampler;"
          "uniform float uHue;"
          "uniform float uVal;"
          "uniform float uSat;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          //{{{
          "vec3 rgbToHsv (float r, float g, float b) {"
          "  float max_val = max(r, max(g, b));"
          "  float min_val = min(r, min(g, b));"
          "  float h;" // hue in degrees

          "  if (max_val == min_val) {" // Simple default case. Do NOT increase saturation if this is the case!
          "    h = 0.0; }"
          "  else if (max_val == r) {"
          "    h = 60.0 * (0.0 + (g - b) / (max_val - min_val)); }"
          "  else if (max_val == g) {"
          "    h = 60.0 * (2.0 + (b - r)/ (max_val - min_val)); }"
          "  else if (max_val == b) {"
          "    h = 60.0 * (4.0 + (r - g) / (max_val - min_val)); }"
          "  if (h < 0.0) {"
          "    h += 360.0; }"

          "  float s = max_val == 0.0 ? 0.0 : (max_val - min_val) / max_val;"
          "  float v = max_val;"
          "  return vec3 (h, s, v);"
          "  }"
          //}}}
          //{{{
          "vec3 hsvToRgb (float h, float s, float v) {"
          "  float r, g, b;"
          "  float c = v * s;"
          "  float h_ = mod(h / 60.0, 6);" // For convenience, change to multiples of 60
          "  float x = c * (1.0 - abs(mod(h_, 2) - 1));"
          "  float r_, g_, b_;"

          "  if (0.0 <= h_ && h_ < 1.0) {"
          "    r_ = c, g_ = x, b_ = 0.0; }"
          "  else if (1.0 <= h_ && h_ < 2.0) {"
          "    r_ = x, g_ = c, b_ = 0.0; }"
          "  else if (2.0 <= h_ && h_ < 3.0) {"
          "    r_ = 0.0, g_ = c, b_ = x; }"
          "  else if (3.0 <= h_ && h_ < 4.0) {"
          "    r_ = 0.0, g_ = x, b_ = c; }"
          "  else if (4.0 <= h_ && h_ < 5.0) {"
          "    r_ = x, g_ = 0.0, b_ = c; }"
          "  else if (5.0 <= h_ && h_ < 6.0) {"
          "    r_ = c, g_ = 0.0, b_ = x; }"
          "  else {"
          "    r_ = 0.0, g_ = 0.0, b_ = 0.0; }"

          "  float m = v - c;"
          "  r = r_ + m;"
          "  g = g_ + m;"
          "  b = b_ + m;"

          "  return vec3 (r, g, b);"
          "  }"
          //}}}

          "void main() {"
          "  outColor = texture (uSampler, textureCoord);"

          "  if (uHue != 0.0 || uVal != 0.0 || uSat != 0.0) {"
          "    vec3 hsv = rgbToHsv (outColor.x, outColor.y, outColor.z);"
          "    hsv.x += uHue;"
          "    if ((outColor.x != outColor.y) || (outColor.y != outColor.z)) {"
                 // not grayscale
          "      hsv.y = uSat <= 0.0 ? "
          "      hsv.y * (1.0 + uSat) : hsv.y + (1.0 - hsv.y) * uSat;"
          "      }"
          "    hsv.z = uVal <= 0.0 ? hsv.z * (1.0 + uVal) : hsv.z + (1.0 - hsv.z) * uVal;"
          "    vec3 rgb = hsvToRgb (hsv.x, hsv.y, hsv.z);"
          "    outColor.xyz = rgb;"
          "    }"
          //"  if (uPreMultiply)"
          //"    outColor.xyz *= outColor.w;"
          "  }";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGL2LayerShader() {
        glDeleteProgram (mId);
        }
      //}}}

      // sets
      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void setHueSatVal (float hue, float sat, float val) final {
        glUniform1f (glGetUniformLocation (mId, "uHue"), hue);
        glUniform1f (glGetUniformLocation (mId, "uSat"), sat);
        glUniform1f (glGetUniformLocation (mId, "uVal"), val);
        }
      //}}}

      //{{{
      virtual void use() final {

        glUseProgram (mId);
        }
      //}}}
      };
    //}}}
    //{{{
    class cOpenGL2PaintShader : public cPaintShader {
    public:
      cOpenGL2PaintShader() : cPaintShader() {
        const string kFragShader =
          "#version 330 core\n"

          "uniform sampler2D uSampler;"
          "uniform vec2 uPos;"
          "uniform vec2 uPrevPos;"
          "uniform float uRadius;"
          "uniform vec4 uColor;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "float distToLine (vec2 v, vec2 w, vec2 p) {"
          "  float l2 = pow (distance(w, v), 2.);"
          "  if (l2 == 0.0)"
          "    return distance (p, v);"
          "  float t = clamp (dot (p - v, w - v) / l2, 0., 1.);"
          "  vec2 j = v + t * (w - v);"
          "  return distance (p, j);"
          "  }"

          "void main() {"
          "  float dist = distToLine (uPrevPos.xy, uPos.xy, textureCoord * textureSize (uSampler, 0)) - uRadius;"
          "  outColor = mix (uColor, texture (uSampler, textureCoord), clamp (dist, 0.0, 1.0));"
          "  }";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGL2PaintShader() {
        glDeleteProgram (mId);
        }
      //}}}

      // sets
      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void setStroke (cVec2 pos, cVec2 prevPos, float radius, const cColor& color) final {

        glUniform2fv (glGetUniformLocation (mId, "uPos"), 1, (float*)&pos);
        glUniform2fv (glGetUniformLocation (mId, "uPrevPos"), 1, (float*)&prevPos);
        glUniform1f (glGetUniformLocation (mId, "uRadius"), radius);
        glUniform4fv (glGetUniformLocation (mId, "uColor"), 1, (float*)&color);
        }
      //}}}

      //{{{
      virtual void use() final {

        glUseProgram (mId);
        }
      //}}}
      };
    //}}}

    //{{{
    static uint32_t compileShader (const string& vertShaderString, const string& fragShaderString) {

      // compile vertShader
      const GLuint vertShader = glCreateShader (GL_VERTEX_SHADER);
      const GLchar* vertShaderStr = vertShaderString.c_str();
      glShaderSource (vertShader, 1, &vertShaderStr, 0);
      glCompileShader (vertShader);
      GLint success;
      glGetShaderiv (vertShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        //{{{  error, exit
        char errMessage[512];
        glGetProgramInfoLog (vertShader, 512, NULL, errMessage);
        cLog::log (LOGERROR, fmt::format ("vertShader failed {}", errMessage));
        exit (EXIT_FAILURE);
        }
        //}}}

      // compile fragShader
      const GLuint fragShader = glCreateShader (GL_FRAGMENT_SHADER);
      const GLchar* fragShaderStr = fragShaderString.c_str();
      glShaderSource (fragShader, 1, &fragShaderStr, 0);
      glCompileShader (fragShader);
      glGetShaderiv (fragShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        //{{{  error, exit
        char errMessage[512];
        glGetProgramInfoLog (fragShader, 512, NULL, errMessage);
        cLog::log (LOGERROR, fmt::format ("fragShader failed {}", errMessage));
        exit (EXIT_FAILURE);
        }
        //}}}

      // create shader program
      uint32_t id = glCreateProgram();
      glAttachShader (id, vertShader);
      glAttachShader (id, fragShader);
      glLinkProgram (id);
      glGetProgramiv (id, GL_LINK_STATUS, &success);
      if (!success) {
        //{{{  error, exit
        char errMessage[512];
        glGetProgramInfoLog (id, 512, NULL, errMessage);
        cLog::log (LOGERROR, fmt::format ("shaderProgram failed {} ",  errMessage));
        exit (EXIT_FAILURE);
        }
        //}}}

      glDeleteShader (vertShader);
      glDeleteShader (fragShader);

      return id;
      }
    //}}}
    };
  //}}}
#elif defined(GL_3)
  //{{{
  class cGL3Graphics : public cGraphics {
  public:
    //{{{
    virtual ~cGL3Graphics() {
      ImGui_ImplOpenGL3_Shutdown();
      }
    //}}}

    // imgui
    //{{{
    virtual bool init() final {

      // report OpenGL versions
      cLog::log (LOGINFO, fmt::format ("OpenGL {}", glGetString (GL_VERSION)));
      cLog::log (LOGINFO, fmt::format ("- GLSL {}", glGetString (GL_SHADING_LANGUAGE_VERSION)));
      cLog::log (LOGINFO, fmt::format ("- Renderer {}", glGetString (GL_RENDERER)));
      cLog::log (LOGINFO, fmt::format ("- Vendor {}", glGetString (GL_VENDOR)));

      return ImGui_ImplOpenGL3_Init ("#version 130");
      }
    //}}}
    virtual void newFrame() final { ImGui_ImplOpenGL3_NewFrame(); }
    //{{{
    virtual void clear (const cPoint& size) final {

      glViewport (0, 0, size.x, size.y);

      // blend
      uint32_t modeRGB = GL_FUNC_ADD;
      uint32_t modeAlpha = GL_FUNC_ADD;
      glBlendEquationSeparate (modeRGB, modeAlpha);

      uint32_t srcRgb = GL_SRC_ALPHA;
      uint32_t dstRGB = GL_ONE_MINUS_SRC_ALPHA;
      uint32_t srcAlpha = GL_ONE;
      uint32_t dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
      glBlendFuncSeparate (srcRgb, dstRGB, srcAlpha, dstAlpha);

      glEnable (GL_BLEND);

      // disables
      glDisable (GL_SCISSOR_TEST);
      glDisable (GL_CULL_FACE);
      glDisable (GL_DEPTH_TEST);

      // clear
      glClearColor (0.f,0.f,0.f, 0.f);
      glClear (GL_COLOR_BUFFER_BIT);
      }
    //}}}
    virtual void renderDrawData() final { ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData()); }

    // creates
    virtual cQuad* createQuad (const cPoint& size) final { return new cOpenGL3Quad (size); }
    virtual cQuad* createQuad (const cPoint& size, const cRect& rect) final { return new cOpenGL3Quad (size, rect); }

    virtual cTarget* createTarget() final { return new cOpenGL3Target(); }
    //{{{
    virtual cTarget* createTarget (const cPoint& size, cTarget::eFormat format) final {
      return new cOpenGL3Target (size, format);
      }
    //}}}
    //{{{
    virtual cTarget* createTarget (uint8_t* pixels, const cPoint& size, cTarget::eFormat format) final {
      return new cOpenGL3Target (pixels, size, format);
      }
    //}}}

    virtual cLayerShader* createLayerShader() final { return new cOpenGL3LayerShader(); }
    virtual cPaintShader* createPaintShader() final { return new cOpenGL3PaintShader(); }

    //{{{
    virtual cTexture* createTexture (cTexture::eTextureType textureType, const cPoint& size) final {
    // factory create

      switch (textureType) {
        case cTexture::eRgba:   return new cOpenGL3RgbaTexture (textureType, size);
        case cTexture::eNv12:   return new cOpenGL3Nv12Texture (textureType, size);
        case cTexture::eYuv420: return new cOpenGL3Yuv420Texture (textureType, size);
        default : return nullptr;
        }
      }
    //}}}
    //{{{
    virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) final {
    // factory create

      switch (textureType) {
        case cTexture::eRgba:   return new cOpenGL3RgbaShader();
        case cTexture::eYuv420: return new cOpenGL3Yuv420Shader();
        case cTexture::eNv12:   return new cOpenGL3Nv12Shader();
        default: return nullptr;
        }
      }
    //}}}

  private:
    //{{{
    inline static const string kQuadVertShader =
      "#version 330 core\n"
      "uniform mat4 uModel;"
      "uniform mat4 uProject;"

      "layout (location = 0) in vec2 inPos;"
      "layout (location = 1) in vec2 inTextureCoord;"
      "out vec2 textureCoord;"

      "void main() {"
      "  textureCoord = inTextureCoord;"
      "  gl_Position = uProject * uModel * vec4 (inPos, 0.0, 1.0);"
      "  }";
    //}}}

    //{{{
    class cOpenGL3Quad : public cQuad {
    public:
      //{{{
      cOpenGL3Quad (const cPoint& size) : cQuad(size) {

        // vertexArray
        glGenVertexArrays (1, &mVertexArrayObject);
        glBindVertexArray (mVertexArrayObject);

        // vertices
        glGenBuffers (1, &mVertexBufferObject);
        glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

        glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray (0);

        glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray (1);

        const float widthF = static_cast<float>(size.x);
        const float heightF = static_cast<float>(size.y);
        const float kVertices[] = {
          0.f,   heightF,  0.f,1.f, // tl vertex
          widthF,0.f,      1.f,0.f, // br vertex
          0.f,   0.f,      0.f,0.f, // bl vertex
          widthF,heightF,  1.f,1.f, // tr vertex
          };

        glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

        glGenBuffers (1, &mElementArrayBufferObject);
        glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);

        glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
        }
      //}}}
      //{{{
      cOpenGL3Quad (const cPoint& size, const cRect& rect) : cQuad(size) {

        // vertexArray
        glGenVertexArrays (1, &mVertexArrayObject);
        glBindVertexArray (mVertexArrayObject);

        // vertices
        glGenBuffers (1, &mVertexBufferObject);
        glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

        glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray (0);

        glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray (1);

        const float subLeftF = static_cast<float>(rect.left);
        const float subRightF = static_cast<float>(rect.right);
        const float subBottomF = static_cast<float>(rect.bottom);
        const float subTopF = static_cast<float>(rect.top);

        const float subLeftTexF = subLeftF / size.x;
        const float subRightTexF = subRightF / size.x;
        const float subBottomTexF = subBottomF / size.y;
        const float subTopTexF = subTopF / size.y;

        const float kVertices[] = {
          subLeftF, subTopF,     subLeftTexF, subTopTexF,    // tl
          subRightF,subBottomF,  subRightTexF,subBottomTexF, // br
          subLeftF, subBottomF,  subLeftTexF, subBottomTexF, // bl
          subRightF,subTopF,     subRightTexF,subTopTexF     // tr
          };

        glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

        // indices
        glGenBuffers (1, &mElementArrayBufferObject);
        glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);
        glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
        }
      //}}}
      //{{{
      virtual ~cOpenGL3Quad() {

        glDeleteBuffers (1, &mElementArrayBufferObject);
        glDeleteBuffers (1, &mVertexBufferObject);
        glDeleteVertexArrays (1, &mVertexArrayObject);
        }
      //}}}

      //{{{
      void draw() final {

        glBindVertexArray (mVertexArrayObject);
        glDrawElements (GL_TRIANGLES, mNumIndices, GL_UNSIGNED_BYTE, 0);
        }
      //}}}

    private:
      inline static const uint32_t mNumIndices = 6;
      inline static const uint8_t kIndices[mNumIndices] = {
        0, 1, 2, // 0   0-3
        0, 3, 1  // |\   \|
        };       // 2-1   1

      uint32_t mVertexArrayObject = 0;
      uint32_t mVertexBufferObject = 0;
      uint32_t mElementArrayBufferObject = 0;
      };
    //}}}
    //{{{
    class cOpenGL3Target : public cTarget {
    public:
      //{{{
      cOpenGL3Target() : cTarget ({0,0}) {
      // window Target

        mImageFormat = GL_RGBA;
        mInternalFormat = GL_RGBA;
        }
      //}}}
      //{{{
      cOpenGL3Target (const cPoint& size, eFormat format) : cTarget(size) {

        mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
        mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

        // create empty Target object
        glGenFramebuffers (1, &mFrameBufferObject);
        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

        // create and add texture to Target object
        glGenTextures (1, &mColorTextureId);

        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, 0);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap (GL_TEXTURE_2D);

        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
        }
      //}}}
      //{{{
      cOpenGL3Target (uint8_t* pixels, const cPoint& size, eFormat format) : cTarget(size) {

        mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
        mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

        // create Target object from pixels
        glGenFramebuffers (1, &mFrameBufferObject);
        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

        // create and add texture to Target object
        glGenTextures (1, &mColorTextureId);

        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap (GL_TEXTURE_2D);

        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
        }
      //}}}
      //{{{
      virtual ~cOpenGL3Target() {

        glDeleteTextures (1, &mColorTextureId);
        glDeleteFramebuffers (1, &mFrameBufferObject);
        free (mPixels);
        }
      //}}}

      /// gets
      //{{{
      uint8_t* getPixels() final {

        if (!mPixels) {
          // create mPixels, texture pixels shadow buffer
          mPixels = static_cast<uint8_t*>(malloc (getNumPixelBytes()));
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
          }

        else if (!mDirtyPixelsRect.isEmpty()) {
          // no openGL glGetTexSubImage, so dirtyPixelsRect not really used, is this correct ???
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
          mDirtyPixelsRect = cRect(0,0,0,0);
          }

        return mPixels;
        }
      //}}}

      // sets
      //{{{
      void setSize (const cPoint& size) final {
        if (mFrameBufferObject == 0)
          mSize = size;
        else
          cLog::log (LOGERROR, "unimplmented setSize of non screen Target");
        };
      //}}}
      //{{{
      void setTarget (const cRect& rect) final {
      // set us as target, set viewport to our size, invalidate contents (probably a clear)

        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);
        glViewport (0, 0, mSize.x, mSize.y);

        glDisable (GL_SCISSOR_TEST);
        glDisable (GL_CULL_FACE);
        glDisable (GL_DEPTH_TEST);

        // texture could be changed, add to dirtyPixelsRect
        mDirtyPixelsRect += rect;
        }
      //}}}
      //{{{
      void setBlend() final {

        uint32_t modeRGB = GL_FUNC_ADD;
        uint32_t modeAlpha = GL_FUNC_ADD;

        uint32_t srcRgb = GL_SRC_ALPHA;
        uint32_t dstRGB = GL_ONE_MINUS_SRC_ALPHA;
        uint32_t srcAlpha = GL_ONE;
        uint32_t dstAlpha = GL_ONE_MINUS_SRC_ALPHA;

        glBlendEquationSeparate (modeRGB, modeAlpha);
        glBlendFuncSeparate (srcRgb, dstRGB, srcAlpha, dstAlpha);
        glEnable (GL_BLEND);
        }
      //}}}
      //{{{
      void setSource() final {

        if (mFrameBufferObject) {
          glActiveTexture (GL_TEXTURE0);
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          }
        else
          cLog::log (LOGERROR, "windowTarget cannot be src");
        }
      //}}}

      // actions
      //{{{
      void invalidate() final {

        //glInvalidateFramebuffer (mFrameBufferObject, 1, GL_COLOR_ATTACHMENT0);
        clear (cColor(0.f,0.f,0.f, 0.f));
        }
      //}}}
      //{{{
      void pixelsChanged (const cRect& rect) final {
      // pixels in rect changed, write back to texture

        if (mPixels) {
          // update y band, quicker x cropped line by line ???
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          glTexSubImage2D (GL_TEXTURE_2D, 0, 0, rect.top, mSize.x, rect.getHeight(),
                           mImageFormat, GL_UNSIGNED_BYTE, mPixels + (rect.top * mSize.x * 4));
          // simpler whole screen
          //glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, mSize.x, mSize.y, 0, mImageFormat, GL_UNSIGNED_BYTE, mPixels);
          }
        }
      //}}}

      //{{{
      void clear (const cColor& color) final {
        glClearColor (color.r,color.g,color.b, color.a);
        glClear (GL_COLOR_BUFFER_BIT);
        }
      //}}}
      //{{{
      void blit (cTarget& src, const cPoint& srcPoint, const cRect& dstRect) final {

        glBindFramebuffer (GL_READ_FRAMEBUFFER, src.getId());
        glBindFramebuffer (GL_DRAW_FRAMEBUFFER, mFrameBufferObject);
        glBlitFramebuffer (srcPoint.x, srcPoint.y, srcPoint.x + dstRect.getWidth(), srcPoint.y + dstRect.getHeight(),
                           dstRect.left, dstRect.top, dstRect.right, dstRect.bottom,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // texture changed, add to dirtyPixelsRect
        mDirtyPixelsRect += dstRect;
        }
      //}}}

      //{{{
      bool checkStatus() final {

        GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);

        switch (status) {
          case GL_FRAMEBUFFER_COMPLETE:
            return true;
          case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            cLog::log (LOGERROR, "Target incomplete: Attachment is NOT complete"); return false;
          case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            cLog::log (LOGERROR, "Target incomplete: No image is attached to FBO"); return false;
          case GL_FRAMEBUFFER_UNSUPPORTED:
            cLog::log (LOGERROR, "Target incomplete: Unsupported by FBO implementation"); return false;

          case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            cLog::log (LOGERROR, "Target incomplete: Draw buffer"); return false;
          case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            cLog::log (LOGERROR, "Target incomplete: Read buffer"); return false;

          //case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
          // cLog::log (LOGERROR, "Target incomplete: Attached images have different dimensions"); return false;
          default:
            cLog::log (LOGERROR, "Target incomplete: Unknown error"); return false;
          }
        }
      //}}}
      //{{{
      void reportInfo() final {

        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

        GLint colorBufferCount = 0;
        glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS, &colorBufferCount);

        GLint multiSampleCount = 0;
        glGetIntegerv (GL_MAX_SAMPLES, &multiSampleCount);

        cLog::log (LOGINFO, fmt::format ("Target maxColorAttach {} masSamples {}", colorBufferCount, multiSampleCount));

        //  print info of the colorbuffer attachable image
        GLint objectType;
        GLint objectId;
        for (int i = 0; i < colorBufferCount; ++i) {
          glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
          if (objectType != GL_NONE) {
            glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                                   GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
            string formatName;
            cLog::log (LOGINFO, fmt::format ("- color{}", i));
            if (objectType == GL_TEXTURE)
              cLog::log (LOGINFO, fmt::format ("  - GL_TEXTURE {}", getTextureParameters (objectId)));
            else if(objectType == GL_RENDERBUFFER)
              cLog::log (LOGINFO, fmt::format ("  - GL_RENDERBUFFER {}", getRenderbufferParameters (objectId)));
            }
          }

        //  print info of the depthbuffer attachable image
        glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                               GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
        if (objectType != GL_NONE) {
          glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
          cLog::log (LOGINFO, fmt::format ("depth"));
          switch (objectType) {
            case GL_TEXTURE:
              cLog::log (LOGINFO, fmt::format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
              break;
            case GL_RENDERBUFFER:
              cLog::log (LOGINFO, fmt::format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
              break;
            }
          }

        // print info of the stencilbuffer attachable image
        glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                               GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
        if (objectType != GL_NONE) {
          glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
          cLog::log (LOGINFO, fmt::format ("stencil"));
          switch (objectType) {
            case GL_TEXTURE:
              cLog::log (LOGINFO, fmt::format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
              break;
            case GL_RENDERBUFFER:
              cLog::log (LOGINFO, fmt::format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
              break;
            }
          }
        }
      //}}}

    private:
      //{{{
      static string getInternalFormat (uint32_t formatNum) {

        string formatName;

        switch (formatNum) {
          case GL_DEPTH_COMPONENT:   formatName = "GL_DEPTH_COMPONENT"; break;   // 0x1902
          case GL_ALPHA:             formatName = "GL_ALPHA"; break;             // 0x1906
          case GL_RGB:               formatName = "GL_RGB"; break;               // 0x1907
          case GL_RGB8:              formatName = "GL_RGB8"; break;              // 0x8051
          case GL_RGBA4:             formatName = "GL_RGBA4"; break;             // 0x8056
          case GL_RGB5_A1:           formatName = "GL_RGB5_A1"; break;           // 0x8057
          case GL_RGBA8:             formatName = "GL_RGBA8"; break;             // 0x8058
          case GL_RGB10_A2:          formatName = "GL_RGB10_A2"; break;          // 0x8059
          case GL_DEPTH_COMPONENT16: formatName = "GL_DEPTH_COMPONENT16"; break; // 0x81A5
          case GL_DEPTH_COMPONENT24: formatName = "GL_DEPTH_COMPONENT24"; break; // 0x81A6
          case GL_DEPTH_STENCIL:     formatName = "GL_DEPTH_STENCIL"; break;     // 0x84F9
          case GL_DEPTH24_STENCIL8:  formatName = "GL_DEPTH24_STENCIL8"; break;  // 0x88F0

          case GL_STENCIL_INDEX:     formatName = "GL_STENCIL_INDEX"; break;     // 0x1901
          case GL_RGBA:              formatName = "GL_RGBA"; break;              // 0x1908
          case GL_R3_G3_B2:          formatName = "GL_R3_G3_B2"; break;          // 0x2A10
          case GL_RGB4:              formatName = "GL_RGB4"; break;              // 0x804F
          case GL_RGB5:              formatName = "GL_RGB5"; break;              // 0x8050
          case GL_RGB10:             formatName = "GL_RGB10"; break;             // 0x8052
          case GL_RGB12:             formatName = "GL_RGB12"; break;             // 0x8053
          case GL_RGB16:             formatName = "GL_RGB16"; break;             // 0x8054
          case GL_RGBA2:             formatName = "GL_RGBA2"; break;             // 0x8055
          case GL_RGBA12:            formatName = "GL_RGBA12"; break;            // 0x805A
          case GL_RGBA16:            formatName = "GL_RGBA16"; break;            // 0x805B
          case GL_DEPTH_COMPONENT32: formatName = "GL_DEPTH_COMPONENT32"; break; // 0x81A7

          //case GL_LUMINANCE:         formatName = "GL_LUMINANCE"; break;         // 0x1909
          //case GL_LUMINANCE_ALPHA:   formatName = "GL_LUMINANCE_ALPHA"; break;   // 0x190A
          //case GL_RGBA32F:           formatName = "GL_RGBA32F"; break;           // 0x8814
          //case GL_RGB32F:            formatName = "GL_RGB32F"; break;            // 0x8815
          //case GL_RGBA16F:           formatName = "GL_RGBA16F"; break;           // 0x881A
          //case GL_RGB16F:            formatName = "GL_RGB16F"; break;            // 0x881B
          default: formatName = fmt::format ("Unknown Format {}", formatNum); break;
          }

        return formatName;
        }
      //}}}
      //{{{
      static string getTextureParameters (uint32_t id) {

        if (glIsTexture(id) == GL_FALSE)
          return "Not texture object";

        glBindTexture (GL_TEXTURE_2D, id);
        int width;
        glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);            // get texture width
        int height;
        glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);          // get texture height
        int formatNum;
        glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &formatNum); // get texture internal format
        return fmt::format (" {} {} {}", width, height, getInternalFormat (formatNum));
        }
      //}}}
      //{{{
      static string getRenderbufferParameters (uint32_t id) {

        if (glIsRenderbuffer(id) == GL_FALSE)
          return "Not Renderbuffer object";

        int width, height, formatNum, samples;
        glBindRenderbuffer (GL_RENDERBUFFER, id);
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);    // get renderbuffer width
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);  // get renderbuffer height
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &formatNum); // get renderbuffer internal format
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);   // get multisample count

        return fmt::format (" {} {} {} {}", width, height, samples, getInternalFormat (formatNum));
        }
      //}}}
      };
    //}}}

    //{{{
    class cOpenGL3RgbaTexture : public cTexture {
    public:
      //{{{
      cOpenGL3RgbaTexture (eTextureType textureType, const cPoint& size)
          : cTexture(textureType, size) {

        cLog::log (LOGINFO, fmt::format ("creating eRgba texture {}x{}", size.x, size.y));
        glGenTextures (1, &mTextureId);

        glBindTexture (GL_TEXTURE_2D, mTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateMipmap (GL_TEXTURE_2D);
        }
      //}}}
      //{{{
      virtual ~cOpenGL3RgbaTexture() {
        glDeleteTextures (1, &mTextureId);
        }
      //}}}

      virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId; }

      //{{{
      virtual void setPixels (uint8_t** pixels) final {
      // set textures using pixels in ffmpeg avFrame format

        glBindTexture (GL_TEXTURE_2D, mTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels[0]);
        }
      //}}}
      //{{{
      virtual void setSource() final {

        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mTextureId);
        }
      //}}}

    private:
      uint32_t mTextureId = 0;
      };
    //}}}
    //{{{
    class cOpenGL3Nv12Texture : public cTexture {
    public:
      //{{{
      cOpenGL3Nv12Texture (eTextureType textureType, const cPoint& size)
          : cTexture(textureType, size) {

        //cLog::log (LOGINFO, fmt::format ("creating eNv12 texture {}x{}", size.x, size.y));

        glGenTextures (2, mTextureId.data());

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // uv texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RG, size.x/2, size.y/2, 0, GL_RG, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
      //}}}
      //{{{
      virtual ~cOpenGL3Nv12Texture() {
        //glDeleteTextures (2, mTextureId.data());
        glDeleteTextures (1, &mTextureId[0]);
        glDeleteTextures (1, &mTextureId[1]);

        cLog::log (LOGINFO, fmt::format ("deleting eVv12 texture {}x{}", mSize.x, mSize.y));
        }
      //}}}

      virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId[0]; }  // luma only

      //{{{
      virtual void setPixels (uint8_t** pixels) final {
      // set textures using pixels in ffmpeg avFrame format

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[0]);

        // uv texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RG, mSize.x/2, mSize.y/2, 0, GL_RG, GL_UNSIGNED_BYTE, pixels[1]);
        }
      //}}}
      //{{{
      virtual void setSource() final {

        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);

        glActiveTexture (GL_TEXTURE1);
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        }
      //}}}

    private:
      array <uint32_t,2> mTextureId;
      };
    //}}}
    //{{{
    class cOpenGL3Yuv420Texture : public cTexture {
    public:
      //{{{
      cOpenGL3Yuv420Texture (eTextureType textureType, const cPoint& size)
          : cTexture(textureType, size) {

        //cLog::log (LOGINFO, fmt::format ("creating eYuv420 texture {}x{}", size.x, size.y));

        glGenTextures (3, mTextureId.data());

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // u texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // v texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
      //}}}
      //{{{
      virtual ~cOpenGL3Yuv420Texture() {
        //glDeleteTextures (3, mTextureId.data());
        glDeleteTextures (1, &mTextureId[0]);
        glDeleteTextures (1, &mTextureId[1]);
        glDeleteTextures (1, &mTextureId[2]);

        cLog::log (LOGINFO, fmt::format ("deleting eYuv420 texture {}x{}", mSize.x, mSize.y));
        }
      //}}}

      virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId[0]; }   // luma only

      //{{{
      virtual void setPixels (uint8_t** pixels) final {
      // set textures using pixels in ffmpeg avFrame format

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[0]);

        // u texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[1]);

        // v texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[2]);
        }
      //}}}
      //{{{
      virtual void setSource() final {

        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);

        glActiveTexture (GL_TEXTURE1);
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);

        glActiveTexture (GL_TEXTURE2);
        glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
        }
      //}}}

    private:
      array <uint32_t,3> mTextureId;
      };
    //}}}

    //{{{
    class cOpenGL3RgbaShader : public cTextureShader {
    public:

      cOpenGL3RgbaShader() : cTextureShader() {
        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D uSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
          "  outColor = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y));"
          //"  outColor /= outColor.w;"
          "  }";
        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGL3RgbaShader() {
        glDeleteProgram (mId);
        }
      //}}}

      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void use() final {

        glUseProgram (mId);
        glUniform1i (glGetUniformLocation (mId, "uSampler"), 0);
        }
      //}}}
      };
    //}}}
    //{{{
    class cOpenGL3Nv12Shader : public cTextureShader {
    public:
      cOpenGL3Nv12Shader() : cTextureShader() {
        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D ySampler;"
          "uniform sampler2D uvSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
            "float y = texture (ySampler, vec2 (textureCoord.x, -textureCoord.y)).r;"
            "float u = texture (uvSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
            "float v = texture (uvSampler, vec2 (textureCoord.x, -textureCoord.y)).g - 0.5;"
            "y = (y - 0.0625) * 1.1643;"
            "outColor.r = y + (1.5958 * v);"
            "outColor.g = y - (0.39173 * u) - (0.81290 * v);"
            "outColor.b = y + (2.017 * u);"
            "outColor.a = 1.0;"
            "}";

        mId = compileShader (kQuadVertShader, kFragShader);
        }

      //{{{
      virtual ~cOpenGL3Nv12Shader() {
        glDeleteProgram (mId);
        }
      //}}}

      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void use() final {

        //cLog::log (LOGINFO, "video use");
        glUseProgram (mId);

        glUniform1i (glGetUniformLocation (mId, "ySampler"), 0);
        glUniform1i (glGetUniformLocation (mId, "uvSampler"), 1);
        }
      //}}}
      };
    //}}}
    //{{{
    class cOpenGL3Yuv420Shader : public cTextureShader {
    public:
      cOpenGL3Yuv420Shader() : cTextureShader() {
        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D ySampler;"
          "uniform sampler2D uSampler;"
          "uniform sampler2D vSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
            "float y = texture (ySampler, vec2 (textureCoord.x, -textureCoord.y)).r;"
            "float u = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
            "float v = texture (vSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
            "y = (y - 0.0625) * 1.1643;"
            "outColor.r = y + (1.5958 * v);"
            "outColor.g = y - (0.39173 * u) - (0.81290 * v);"
            "outColor.b = y + (2.017 * u);"
            "outColor.a = 1.0;"
            "}";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGL3Yuv420Shader() {
        glDeleteProgram (mId);
        }
      //}}}

      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void use() final {

        //cLog::log (LOGINFO, "video use");
        glUseProgram (mId);

        glUniform1i (glGetUniformLocation (mId, "ySampler"), 0);
        glUniform1i (glGetUniformLocation (mId, "uSampler"), 1);
        glUniform1i (glGetUniformLocation (mId, "vSampler"), 2);
        }
      //}}}
      };
    //}}}

    //{{{
    class cOpenGL3LayerShader : public cLayerShader {
    public:
      cOpenGL3LayerShader() : cLayerShader() {

        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D uSampler;"
          "uniform float uHue;"
          "uniform float uVal;"
          "uniform float uSat;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          //{{{
          "vec3 rgbToHsv (float r, float g, float b) {"
          "  float max_val = max(r, max(g, b));"
          "  float min_val = min(r, min(g, b));"
          "  float h;" // hue in degrees

          "  if (max_val == min_val) {" // Simple default case. Do NOT increase saturation if this is the case!
          "    h = 0.0; }"
          "  else if (max_val == r) {"
          "    h = 60.0 * (0.0 + (g - b) / (max_val - min_val)); }"
          "  else if (max_val == g) {"
          "    h = 60.0 * (2.0 + (b - r)/ (max_val - min_val)); }"
          "  else if (max_val == b) {"
          "    h = 60.0 * (4.0 + (r - g) / (max_val - min_val)); }"
          "  if (h < 0.0) {"
          "    h += 360.0; }"

          "  float s = max_val == 0.0 ? 0.0 : (max_val - min_val) / max_val;"
          "  float v = max_val;"
          "  return vec3 (h, s, v);"
          "  }"
          //}}}
          //{{{
          "vec3 hsvToRgb (float h, float s, float v) {"
          "  float r, g, b;"
          "  float c = v * s;"
          "  float h_ = mod(h / 60.0, 6);" // For convenience, change to multiples of 60
          "  float x = c * (1.0 - abs(mod(h_, 2) - 1));"
          "  float r_, g_, b_;"

          "  if (0.0 <= h_ && h_ < 1.0) {"
          "    r_ = c, g_ = x, b_ = 0.0; }"
          "  else if (1.0 <= h_ && h_ < 2.0) {"
          "    r_ = x, g_ = c, b_ = 0.0; }"
          "  else if (2.0 <= h_ && h_ < 3.0) {"
          "    r_ = 0.0, g_ = c, b_ = x; }"
          "  else if (3.0 <= h_ && h_ < 4.0) {"
          "    r_ = 0.0, g_ = x, b_ = c; }"
          "  else if (4.0 <= h_ && h_ < 5.0) {"
          "    r_ = x, g_ = 0.0, b_ = c; }"
          "  else if (5.0 <= h_ && h_ < 6.0) {"
          "    r_ = c, g_ = 0.0, b_ = x; }"
          "  else {"
          "    r_ = 0.0, g_ = 0.0, b_ = 0.0; }"

          "  float m = v - c;"
          "  r = r_ + m;"
          "  g = g_ + m;"
          "  b = b_ + m;"

          "  return vec3 (r, g, b);"
          "  }"
          //}}}

          "void main() {"
          "  outColor = texture (uSampler, textureCoord);"

          "  if (uHue != 0.0 || uVal != 0.0 || uSat != 0.0) {"
          "    vec3 hsv = rgbToHsv (outColor.x, outColor.y, outColor.z);"
          "    hsv.x += uHue;"
          "    if ((outColor.x != outColor.y) || (outColor.y != outColor.z)) {"
                 // not grayscale
          "      hsv.y = uSat <= 0.0 ? "
          "      hsv.y * (1.0 + uSat) : hsv.y + (1.0 - hsv.y) * uSat;"
          "      }"
          "    hsv.z = uVal <= 0.0 ? hsv.z * (1.0 + uVal) : hsv.z + (1.0 - hsv.z) * uVal;"
          "    vec3 rgb = hsvToRgb (hsv.x, hsv.y, hsv.z);"
          "    outColor.xyz = rgb;"
          "    }"
          //"  if (uPreMultiply)"
          //"    outColor.xyz *= outColor.w;"
          "  }";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGL3LayerShader() {
        glDeleteProgram (mId);
        }
      //}}}

      // sets
      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void setHueSatVal (float hue, float sat, float val) final {
        glUniform1f (glGetUniformLocation (mId, "uHue"), hue);
        glUniform1f (glGetUniformLocation (mId, "uSat"), sat);
        glUniform1f (glGetUniformLocation (mId, "uVal"), val);
        }
      //}}}

      //{{{
      virtual void use() final {

        glUseProgram (mId);
        }
      //}}}
      };
    //}}}
    //{{{
    class cOpenGL3PaintShader : public cPaintShader {
    public:
      cOpenGL3PaintShader() : cPaintShader() {
        const string kFragShader =
          "#version 330 core\n"

          "uniform sampler2D uSampler;"
          "uniform vec2 uPos;"
          "uniform vec2 uPrevPos;"
          "uniform float uRadius;"
          "uniform vec4 uColor;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "float distToLine (vec2 v, vec2 w, vec2 p) {"
          "  float l2 = pow (distance(w, v), 2.);"
          "  if (l2 == 0.0)"
          "    return distance (p, v);"
          "  float t = clamp (dot (p - v, w - v) / l2, 0., 1.);"
          "  vec2 j = v + t * (w - v);"
          "  return distance (p, j);"
          "  }"

          "void main() {"
          "  float dist = distToLine (uPrevPos.xy, uPos.xy, textureCoord * textureSize (uSampler, 0)) - uRadius;"
          "  outColor = mix (uColor, texture (uSampler, textureCoord), clamp (dist, 0.0, 1.0));"
          "  }";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGL3PaintShader() {
        glDeleteProgram (mId);
        }
      //}}}

      // sets
      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void setStroke (cVec2 pos, cVec2 prevPos, float radius, const cColor& color) final {

        glUniform2fv (glGetUniformLocation (mId, "uPos"), 1, (float*)&pos);
        glUniform2fv (glGetUniformLocation (mId, "uPrevPos"), 1, (float*)&prevPos);
        glUniform1f (glGetUniformLocation (mId, "uRadius"), radius);
        glUniform4fv (glGetUniformLocation (mId, "uColor"), 1, (float*)&color);
        }
      //}}}

      //{{{
      virtual void use() final {

        glUseProgram (mId);
        }
      //}}}
      };
    //}}}

    //{{{
    static uint32_t compileShader (const string& vertShaderString, const string& fragShaderString) {

      // compile vertShader
      const GLuint vertShader = glCreateShader (GL_VERTEX_SHADER);
      const GLchar* vertShaderStr = vertShaderString.c_str();
      glShaderSource (vertShader, 1, &vertShaderStr, 0);
      glCompileShader (vertShader);
      GLint success;
      glGetShaderiv (vertShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        //{{{  error, exit
        char errMessage[512];
        glGetProgramInfoLog (vertShader, 512, NULL, errMessage);
        cLog::log (LOGERROR, fmt::format ("vertShader failed {}", errMessage));
        exit (EXIT_FAILURE);
        }
        //}}}

      // compile fragShader
      const GLuint fragShader = glCreateShader (GL_FRAGMENT_SHADER);
      const GLchar* fragShaderStr = fragShaderString.c_str();
      glShaderSource (fragShader, 1, &fragShaderStr, 0);
      glCompileShader (fragShader);
      glGetShaderiv (fragShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        //{{{  error, exit
        char errMessage[512];
        glGetProgramInfoLog (fragShader, 512, NULL, errMessage);
        cLog::log (LOGERROR, fmt::format ("fragShader failed {}", errMessage));
        exit (EXIT_FAILURE);
        }
        //}}}

      // create shader program
      uint32_t id = glCreateProgram();
      glAttachShader (id, vertShader);
      glAttachShader (id, fragShader);
      glLinkProgram (id);
      glGetProgramiv (id, GL_LINK_STATUS, &success);
      if (!success) {
        //{{{  error, exit
        char errMessage[512];
        glGetProgramInfoLog (id, 512, NULL, errMessage);
        cLog::log (LOGERROR, fmt::format ("shaderProgram failed {} ",  errMessage));
        exit (EXIT_FAILURE);
        }
        //}}}

      glDeleteShader (vertShader);
      glDeleteShader (fragShader);

      return id;
      }
    //}}}
    };
  //}}}
#elif defined(GLES_3_0) || defined(GLES_3_1) || defined(GLES_3_2)
  //{{{
  class cGLES3Graphics : public cGraphics {
  public:
    //{{{
    virtual ~cGLES3Graphics() {
      ImGui_ImplOpenGL3_Shutdown();
      }
    //}}}

    // imgui
    //{{{
    virtual bool init() final {

      // report OpenGL versions
      cLog::log (LOGINFO, fmt::format ("OpenGL {}", glGetString (GL_VERSION)));
      cLog::log (LOGINFO, fmt::format ("- GLSL {}", glGetString (GL_SHADING_LANGUAGE_VERSION)));
      cLog::log (LOGINFO, fmt::format ("- Renderer {}", glGetString (GL_RENDERER)));
      cLog::log (LOGINFO, fmt::format ("- Vendor {}", glGetString (GL_VENDOR)));

      return ImGui_ImplOpenGL3_Init ("#version 300 es");
      }
    //}}}
    virtual void newFrame() final { ImGui_ImplOpenGL3_NewFrame(); }
    //{{{
    virtual void clear (const cPoint& size) final {

      glViewport (0, 0, size.x, size.y);

      // blend
      uint32_t modeRGB = GL_FUNC_ADD;
      uint32_t modeAlpha = GL_FUNC_ADD;
      glBlendEquationSeparate (modeRGB, modeAlpha);

      uint32_t srcRgb = GL_SRC_ALPHA;
      uint32_t dstRGB = GL_ONE_MINUS_SRC_ALPHA;
      uint32_t srcAlpha = GL_ONE;
      uint32_t dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
      glBlendFuncSeparate (srcRgb, dstRGB, srcAlpha, dstAlpha);

      glEnable (GL_BLEND);

      // disables
      glDisable (GL_SCISSOR_TEST);
      glDisable (GL_CULL_FACE);
      glDisable (GL_DEPTH_TEST);

      // clear
      glClearColor (0.f,0.f,0.f, 0.f);
      glClear (GL_COLOR_BUFFER_BIT);
      }
    //}}}
    virtual void renderDrawData() final { ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData()); }

    // creates
    virtual cQuad* createQuad (const cPoint& size) final { return new cOpenGLES3Quad (size); }
    virtual cQuad* createQuad (const cPoint& size, const cRect& rect) final { return new cOpenGLES3Quad (size, rect); }

    virtual cTarget* createTarget() final { return new cOpenGLES3Target(); }
    virtual cTarget* createTarget (const cPoint& size, cTarget::eFormat format) final {
      return new cOpenGLES3Target (size, format); }
    virtual cTarget* createTarget (uint8_t* pixels, const cPoint& size, cTarget::eFormat format) final {
      return new cOpenGLES3Target (pixels, size, format); }

    virtual cLayerShader* createLayerShader() final { return new cOpenGLES3LayerShader(); }
    virtual cPaintShader* createPaintShader() final { return new cOpenGLES3PaintShader(); }

    //{{{
    virtual cTexture* createTexture (cTexture::eTextureType textureType, const cPoint& size) final {
      switch (textureType) {
        case cTexture::eRgba:   return new cOpenGLES3RgbaTexture (textureType, size);
        case cTexture::eNv12:   return new cOpenGLES3Nv12Texture (textureType, size);
        case cTexture::eYuv420: return new cOpenGLES3Yuv420Texture (textureType, size);
        default : return nullptr;
        }
      }
    //}}}
    //{{{
    virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) final {
      switch (textureType) {
        case cTexture::eRgba:   return new cOpenGLES3RgbaShader();
        case cTexture::eNv12:   return new cOpenGLES3Nv12Shader();
        case cTexture::eYuv420: return new cOpenGLES3Yuv420Shader();
        default: return nullptr;
        }
      }
    //}}}

  private:
    //{{{
    inline static const string kQuadVertShader =

      "#version 300 es\n"
      "precision highp float;"

      "uniform mat4 uModel;"
      "uniform mat4 uProject;"

      "layout (location = 0) in vec2 inPos;"
      "layout (location = 1) in vec2 inTextureCoord;"

      "out vec2 textureCoord;"

      "void main() {"
      "  textureCoord = inTextureCoord;"
      "  gl_Position = uProject * uModel * vec4 (inPos, 0.0, 1.0);"
      "  }";
    //}}}

    //{{{
    class cOpenGLES3Quad : public cQuad {
    public:
      //{{{
      cOpenGLES3Quad (const cPoint& size) : cQuad(size) {

        // vertexArray
        glGenVertexArrays (1, &mVertexArrayObject);
        glBindVertexArray (mVertexArrayObject);

        // vertices
        glGenBuffers (1, &mVertexBufferObject);
        glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

        glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray (0);

        glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray (1);

        const float widthF = static_cast<float>(size.x);
        const float heightF = static_cast<float>(size.y);
        const float kVertices[] = {
          0.f,   heightF,  0.f,1.f, // tl vertex
          widthF,0.f,      1.f,0.f, // br vertex
          0.f,   0.f,      0.f,0.f, // bl vertex
          widthF,heightF,  1.f,1.f, // tr vertex
          };

        glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

        glGenBuffers (1, &mElementArrayBufferObject);
        glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);

        glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
        }
      //}}}
      //{{{
      cOpenGLES3Quad (const cPoint& size, const cRect& rect) : cQuad(size) {

        // vertexArray
        glGenVertexArrays (1, &mVertexArrayObject);
        glBindVertexArray (mVertexArrayObject);

        // vertices
        glGenBuffers (1, &mVertexBufferObject);
        glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

        glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray (0);

        glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray (1);

        const float subLeftF = static_cast<float>(rect.left);
        const float subRightF = static_cast<float>(rect.right);
        const float subBottomF = static_cast<float>(rect.bottom);
        const float subTopF = static_cast<float>(rect.top);

        const float subLeftTexF = subLeftF / size.x;
        const float subRightTexF = subRightF / size.x;
        const float subBottomTexF = subBottomF / size.y;
        const float subTopTexF = subTopF / size.y;

        const float kVertices[] = {
          subLeftF, subTopF,     subLeftTexF, subTopTexF,    // tl
          subRightF,subBottomF,  subRightTexF,subBottomTexF, // br
          subLeftF, subBottomF,  subLeftTexF, subBottomTexF, // bl
          subRightF,subTopF,     subRightTexF,subTopTexF     // tr
          };

        glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

        // indices
        glGenBuffers (1, &mElementArrayBufferObject);
        glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);
        glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
        }
      //}}}
      //{{{
      virtual ~cOpenGLES3Quad() {

        glDeleteBuffers (1, &mElementArrayBufferObject);
        glDeleteBuffers (1, &mVertexBufferObject);
        glDeleteVertexArrays (1, &mVertexArrayObject);
        }
      //}}}

      //{{{
      void draw() final {

        glBindVertexArray (mVertexArrayObject);
        glDrawElements (GL_TRIANGLES, mNumIndices, GL_UNSIGNED_BYTE, 0);
        }
      //}}}

    private:
      inline static const uint32_t mNumIndices = 6;
      inline static const uint8_t kIndices[mNumIndices] = {
        0, 1, 2, // 0   0-3
        0, 3, 1  // |\   \|
        };       // 2-1   1

      uint32_t mVertexArrayObject = 0;
      uint32_t mVertexBufferObject = 0;
      uint32_t mElementArrayBufferObject = 0;
      };
    //}}}
    //{{{
    class cOpenGLES3Target : public cTarget {
    public:
      //{{{
      cOpenGLES3Target() : cTarget ({0,0}) {
      // window Target

        mImageFormat = GL_RGBA;
        mInternalFormat = GL_RGBA;
        }
      //}}}
      //{{{
      cOpenGLES3Target (const cPoint& size, eFormat format) : cTarget(size) {

        mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
        mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

        // create empty Target object
        glGenFramebuffers (1, &mFrameBufferObject);
        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

        // create and add texture to Target object
        glGenTextures (1, &mColorTextureId);

        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, 0);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap (GL_TEXTURE_2D);

        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
        }
      //}}}
      //{{{
      cOpenGLES3Target (uint8_t* pixels, const cPoint& size, eFormat format) : cTarget(size) {

        mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
        mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

        // create Target object from pixels
        glGenFramebuffers (1, &mFrameBufferObject);
        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

        // create and add texture to Target object
        glGenTextures (1, &mColorTextureId);

        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap (GL_TEXTURE_2D);

        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
        }
      //}}}
      //{{{
      virtual ~cOpenGLES3Target() {

        glDeleteTextures (1, &mColorTextureId);
        glDeleteFramebuffers (1, &mFrameBufferObject);
        free (mPixels);
        }
      //}}}

      /// gets
      //{{{
      uint8_t* getPixels() final {

        if (!mPixels) {
          // create mPixels, texture pixels shadow buffer
          mPixels = static_cast<uint8_t*>(malloc (getNumPixelBytes()));
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          #if defined(GL_3)
            glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
          #endif
          }

        else if (!mDirtyPixelsRect.isEmpty()) {
          // no openGL glGetTexSubImage, so dirtyPixelsRect not really used, is this correct ???
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          #if defined(GL_3)
            glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
          #endif
          mDirtyPixelsRect = cRect(0,0,0,0);
          }

        return mPixels;
        }
      //}}}

      // sets
      //{{{
      void setSize (const cPoint& size) final {
        if (mFrameBufferObject == 0)
          mSize = size;
        else
          cLog::log (LOGERROR, "unimplmented setSize of non screen Target");
        };
      //}}}
      //{{{
      void setTarget (const cRect& rect) final {
      // set us as target, set viewport to our size, invalidate contents (probably a clear)

        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);
        glViewport (0, 0, mSize.x, mSize.y);

        glDisable (GL_SCISSOR_TEST);
        glDisable (GL_CULL_FACE);
        glDisable (GL_DEPTH_TEST);

        // texture could be changed, add to dirtyPixelsRect
        mDirtyPixelsRect += rect;
        }
      //}}}
      //{{{
      void setBlend() final {

        uint32_t modeRGB = GL_FUNC_ADD;
        uint32_t modeAlpha = GL_FUNC_ADD;

        uint32_t srcRgb = GL_SRC_ALPHA;
        uint32_t dstRGB = GL_ONE_MINUS_SRC_ALPHA;
        uint32_t srcAlpha = GL_ONE;
        uint32_t dstAlpha = GL_ONE_MINUS_SRC_ALPHA;

        glBlendEquationSeparate (modeRGB, modeAlpha);
        glBlendFuncSeparate (srcRgb, dstRGB, srcAlpha, dstAlpha);
        glEnable (GL_BLEND);
        }
      //}}}
      //{{{
      void setSource() final {

        if (mFrameBufferObject) {
          glActiveTexture (GL_TEXTURE0);
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          }
        else
          cLog::log (LOGERROR, "windowTarget cannot be src");
        }
      //}}}

      // actions
      //{{{
      void invalidate() final {

        //glInvalidateFramebuffer (mFrameBufferObject, 1, GL_COLOR_ATTACHMENT0);
        clear (cColor(0.f,0.f,0.f, 0.f));
        }
      //}}}
      //{{{
      void pixelsChanged (const cRect& rect) final {
      // pixels in rect changed, write back to texture

        if (mPixels) {
          // update y band, quicker x cropped line by line ???
          glBindTexture (GL_TEXTURE_2D, mColorTextureId);
          glTexSubImage2D (GL_TEXTURE_2D, 0, 0, rect.top, mSize.x, rect.getHeight(),
                           mImageFormat, GL_UNSIGNED_BYTE, mPixels + (rect.top * mSize.x * 4));
          // simpler whole screen
          //glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, mSize.x, mSize.y, 0, mImageFormat, GL_UNSIGNED_BYTE, mPixels);
          }
        }
      //}}}

      //{{{
      void clear (const cColor& color) final {
        glClearColor (color.r,color.g,color.b, color.a);
        glClear (GL_COLOR_BUFFER_BIT);
        }
      //}}}
      //{{{
      void blit (cTarget& src, const cPoint& srcPoint, const cRect& dstRect) final {

        glBindFramebuffer (GL_READ_FRAMEBUFFER, src.getId());
        glBindFramebuffer (GL_DRAW_FRAMEBUFFER, mFrameBufferObject);
        glBlitFramebuffer (srcPoint.x, srcPoint.y, srcPoint.x + dstRect.getWidth(), srcPoint.y + dstRect.getHeight(),
                           dstRect.left, dstRect.top, dstRect.right, dstRect.bottom,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // texture changed, add to dirtyPixelsRect
        mDirtyPixelsRect += dstRect;
        }
      //}}}

      //{{{
      bool checkStatus() final {

        GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);

        switch (status) {
          case GL_FRAMEBUFFER_COMPLETE:
            return true;
          case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            cLog::log (LOGERROR, "Target incomplete: Attachment is NOT complete"); return false;
          case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            cLog::log (LOGERROR, "Target incomplete: No image is attached to FBO"); return false;
          case GL_FRAMEBUFFER_UNSUPPORTED:
            cLog::log (LOGERROR, "Target incomplete: Unsupported by FBO implementation"); return false;
          default:
            cLog::log (LOGERROR, "Target incomplete: Unknown error"); return false;
          }
        }
      //}}}
      //{{{
      void reportInfo() final {

        glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

        GLint colorBufferCount = 0;
        glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS, &colorBufferCount);

        GLint multiSampleCount = 0;
        glGetIntegerv (GL_MAX_SAMPLES, &multiSampleCount);

        cLog::log (LOGINFO, fmt::format ("Target maxColorAttach {} masSamples {}", colorBufferCount, multiSampleCount));

        //  print info of the colorbuffer attachable image
        GLint objectType;
        GLint objectId;
        for (int i = 0; i < colorBufferCount; ++i) {
          glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
          if (objectType != GL_NONE) {
            glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                                   GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
            string formatName;
            cLog::log (LOGINFO, fmt::format ("- color{}", i));
            if (objectType == GL_TEXTURE)
              cLog::log (LOGINFO, fmt::format ("  - GL_TEXTURE {}", getTextureParameters (objectId)));
            else if(objectType == GL_RENDERBUFFER)
              cLog::log (LOGINFO, fmt::format ("  - GL_RENDERBUFFER {}", getRenderbufferParameters (objectId)));
            }
          }

        //  print info of the depthbuffer attachable image
        glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                               GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
        if (objectType != GL_NONE) {
          glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
          cLog::log (LOGINFO, fmt::format ("depth"));
          switch (objectType) {
            case GL_TEXTURE:
              cLog::log (LOGINFO, fmt::format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
              break;
            case GL_RENDERBUFFER:
              cLog::log (LOGINFO, fmt::format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
              break;
            }
          }

        // print info of the stencilbuffer attachable image
        glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                               GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
        if (objectType != GL_NONE) {
          glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
          cLog::log (LOGINFO, fmt::format ("stencil"));
          switch (objectType) {
            case GL_TEXTURE:
              cLog::log (LOGINFO, fmt::format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
              break;
            case GL_RENDERBUFFER:
              cLog::log (LOGINFO, fmt::format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
              break;
            }
          }
        }
      //}}}

    private:
      //{{{
      static string getInternalFormat (uint32_t formatNum) {

        string formatName;

        switch (formatNum) {
          case GL_DEPTH_COMPONENT:   formatName = "GL_DEPTH_COMPONENT"; break;   // 0x1902
          case GL_ALPHA:             formatName = "GL_ALPHA"; break;             // 0x1906
          case GL_RGB:               formatName = "GL_RGB"; break;               // 0x1907
          case GL_RGB8:              formatName = "GL_RGB8"; break;              // 0x8051
          case GL_RGBA4:             formatName = "GL_RGBA4"; break;             // 0x8056
          case GL_RGB5_A1:           formatName = "GL_RGB5_A1"; break;           // 0x8057
          case GL_RGBA8:             formatName = "GL_RGBA8"; break;             // 0x8058
          case GL_RGB10_A2:          formatName = "GL_RGB10_A2"; break;          // 0x8059
          case GL_DEPTH_COMPONENT16: formatName = "GL_DEPTH_COMPONENT16"; break; // 0x81A5
          case GL_DEPTH_COMPONENT24: formatName = "GL_DEPTH_COMPONENT24"; break; // 0x81A6
          case GL_DEPTH_STENCIL:     formatName = "GL_DEPTH_STENCIL"; break;     // 0x84F9
          case GL_DEPTH24_STENCIL8:  formatName = "GL_DEPTH24_STENCIL8"; break;  // 0x88F0

        #if defined(GL_3)
          case GL_STENCIL_INDEX:     formatName = "GL_STENCIL_INDEX"; break;     // 0x1901
          case GL_RGBA:              formatName = "GL_RGBA"; break;              // 0x1908
          case GL_R3_G3_B2:          formatName = "GL_R3_G3_B2"; break;          // 0x2A10
          case GL_RGB4:              formatName = "GL_RGB4"; break;              // 0x804F
          case GL_RGB5:              formatName = "GL_RGB5"; break;              // 0x8050
          case GL_RGB10:             formatName = "GL_RGB10"; break;             // 0x8052
          case GL_RGB12:             formatName = "GL_RGB12"; break;             // 0x8053
          case GL_RGB16:             formatName = "GL_RGB16"; break;             // 0x8054
          case GL_RGBA2:             formatName = "GL_RGBA2"; break;             // 0x8055
          case GL_RGBA12:            formatName = "GL_RGBA12"; break;            // 0x805A
          case GL_RGBA16:            formatName = "GL_RGBA16"; break;            // 0x805B
          case GL_DEPTH_COMPONENT32: formatName = "GL_DEPTH_COMPONENT32"; break; // 0x81A7
        #endif
          //case GL_LUMINANCE:         formatName = "GL_LUMINANCE"; break;         // 0x1909
          //case GL_LUMINANCE_ALPHA:   formatName = "GL_LUMINANCE_ALPHA"; break;   // 0x190A
          //case GL_RGBA32F:           formatName = "GL_RGBA32F"; break;           // 0x8814
          //case GL_RGB32F:            formatName = "GL_RGB32F"; break;            // 0x8815
          //case GL_RGBA16F:           formatName = "GL_RGBA16F"; break;           // 0x881A
          //case GL_RGB16F:            formatName = "GL_RGB16F"; break;            // 0x881B
          default: formatName = fmt::format ("Unknown Format {}", formatNum); break;
          }

        return formatName;
        }
      //}}}
      //{{{
      static string getTextureParameters (uint32_t id) {

        if (glIsTexture(id) == GL_FALSE)
          return "Not texture object";

        #if defined(GL_3)
          glBindTexture (GL_TEXTURE_2D, id);
          int width;
          glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);            // get texture width
          int height;
          glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);          // get texture height
          int formatNum;
          glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &formatNum); // get texture internal format
          return fmt::format (" {} {} {}", width, height, getInternalFormat (formatNum));
        #else
          return "unknown texture";
        #endif

        }
      //}}}
      //{{{
      static string getRenderbufferParameters (uint32_t id) {

        if (glIsRenderbuffer(id) == GL_FALSE)
          return "Not Renderbuffer object";

        int width, height, formatNum, samples;
        glBindRenderbuffer (GL_RENDERBUFFER, id);
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);    // get renderbuffer width
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);  // get renderbuffer height
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &formatNum); // get renderbuffer internal format
        glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);   // get multisample count

        return fmt::format (" {} {} {} {}", width, height, samples, getInternalFormat (formatNum));
        }
      //}}}
      };
    //}}}

    //{{{
    class cOpenGLES3RgbaTexture : public cTexture {
    public:
      cOpenGLES3RgbaTexture (eTextureType textureType, const cPoint& size) : cTexture(textureType, size) {

        cLog::log (LOGINFO, fmt::format ("creating eRgba texture {}x{}", size.x, size.y));
        glGenTextures (1, &mTextureId);

        glBindTexture (GL_TEXTURE_2D, mTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateMipmap (GL_TEXTURE_2D);
        }

      virtual ~cOpenGLES3RgbaTexture() { glDeleteTextures (1, &mTextureId); }

      virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId; }

      virtual void setPixels (uint8_t** pixels) final {
      // set textures using pixels in ffmpeg avFrame format
        glBindTexture (GL_TEXTURE_2D, mTextureId);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels[0]);
        }

      virtual void setSource() final {
        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mTextureId);
        }

    private:
      GLuint mTextureId = 0;
      };
    //}}}
    //{{{
    class cOpenGLES3Nv12Texture : public cTexture {
    public:
      cOpenGLES3Nv12Texture (eTextureType textureType, const cPoint& size) : cTexture(textureType, size) {

        //cLog::log (LOGINFO, fmt::format ("creating eNv12 texture {}x{}", size.x, size.y));
        glGenTextures (2, mTextureId.data());

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // uv texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RG, size.x/2, size.y/2, 0, GL_RG, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

      virtual ~cOpenGLES3Nv12Texture() {
        //glDeleteTextures (2, mTextureId.data());
        glDeleteTextures (1, &mTextureId[0]);
        glDeleteTextures (1, &mTextureId[1]);
        cLog::log (LOGINFO, fmt::format ("deleting eVv12 texture {}x{}", mSize.x, mSize.y));
        }

      virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId[0]; }  // luma only

      virtual void setPixels (uint8_t** pixels) final {
      // set textures using pixels in ffmpeg avFrame format

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[0]);

        // uv texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RG, mSize.x/2, mSize.y/2, 0, GL_RG, GL_UNSIGNED_BYTE, pixels[1]);
        }

      virtual void setSource() final {
        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glActiveTexture (GL_TEXTURE1);
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        }

    private:
      array <GLuint,2> mTextureId;  // Y U:V 4:2:0
      };
    //}}}
    //{{{
    class cOpenGLES3Yuv420Texture : public cTexture {
    public:
      //{{{
      cOpenGLES3Yuv420Texture (eTextureType textureType, const cPoint& size) : cTexture(textureType, size) {

        //cLog::log (LOGINFO, fmt::format ("creating eYuv420 texture {}x{}", size.x, size.y));

        glGenTextures (3, mTextureId.data());

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // u texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // v texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
      //}}}
      //{{{
      virtual ~cOpenGLES3Yuv420Texture() {
        //glDeleteTextures (3, mTextureId.data());
        glDeleteTextures (1, &mTextureId[0]);
        glDeleteTextures (1, &mTextureId[1]);
        glDeleteTextures (1, &mTextureId[2]);

        cLog::log (LOGINFO, fmt::format ("deleting eYuv420 texture {}x{}", mSize.x, mSize.y));
        }
      //}}}

      virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId[0]; }   // luma only

      //{{{
      virtual void setPixels (uint8_t** pixels) final {
      // set textures using pixels in ffmpeg avFrame format

        // y texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[0]);

        // u texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[1]);

        // v texture
        glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[2]);
        }
      //}}}
      //{{{
      virtual void setSource() final {

        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mTextureId[0]);

        glActiveTexture (GL_TEXTURE1);
        glBindTexture (GL_TEXTURE_2D, mTextureId[1]);

        glActiveTexture (GL_TEXTURE2);
        glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
        }
      //}}}

    private:
      array <GLuint,3> mTextureId;  // Y,U,V 4:2:0
      };
    //}}}

    //{{{
    class cOpenGLES3RgbaShader : public cTextureShader {
    public:
      cOpenGLES3RgbaShader() : cTextureShader() {
        const string kFragShader =
          "#version 300 es\n"
          "precision mediump float;\n"

          "uniform sampler2D uSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
          "  outColor = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y));"
          "  }";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGLES3RgbaShader() {
        glDeleteProgram (mId);
        }
      //}}}

      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void use() final {

        glUseProgram (mId);
        glUniform1i (glGetUniformLocation (mId, "uSampler"), 0);
        }
      //}}}
      };
    //}}}
    //{{{
    class cOpenGLES3Nv12Shader : public cTextureShader {
    public:
      cOpenGLES3Nv12Shader() : cTextureShader() {
        const string kFragShader =
          "#version 300 es\n"
          "precision mediump float;"

          "uniform sampler2D ySampler;"
          "uniform sampler2D uvSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
            "float y = texture (ySampler, vec2 (textureCoord.x, -textureCoord.y)).r;"
            "float u = texture (uvSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
            "float v = texture (uvSampler, vec2 (textureCoord.x, -textureCoord.y)).g - 0.5;"
            "y = (y - 0.0625) * 1.1643;"
            "outColor.r = y + (1.5958 * v);"
            "outColor.g = y - (0.39173 * u) - (0.81290 * v);"
            "outColor.b = y + (2.017 * u);"
            "outColor.a = 1.0;"
            "}";

        mId = compileShader (kQuadVertShader, kFragShader);
        }

      //{{{
      virtual ~cOpenGLES3Nv12Shader() {
        glDeleteProgram (mId);
        }
      //}}}

      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void use() final {

        //cLog::log (LOGINFO, "video use");
        glUseProgram (mId);

        glUniform1i (glGetUniformLocation (mId, "ySampler"), 0);
        glUniform1i (glGetUniformLocation (mId, "uvSampler"), 1);
        }
      //}}}
      };
    //}}}
    //{{{
    class cOpenGLES3Yuv420Shader : public cTextureShader {
    public:
      cOpenGLES3Yuv420Shader() : cTextureShader() {
        const string kFragShader =
          "#version 300 es\n"
          "precision mediump float;"

          "uniform sampler2D ySampler;"
          "uniform sampler2D uSampler;"
          "uniform sampler2D vSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
            "float y = texture (ySampler, vec2 (textureCoord.x, -textureCoord.y)).r;"
            "float u = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
            "float v = texture (vSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
            "y = (y - 0.0625) * 1.1643;"
            "outColor.r = y + (1.5958 * v);"
            "outColor.g = y - (0.39173 * u) - (0.81290 * v);"
            "outColor.b = y + (2.017 * u);"
            "outColor.a = 1.0;"
            "}";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGLES3Yuv420Shader() {
        glDeleteProgram (mId);
        }
      //}}}

      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void use() final {

        //cLog::log (LOGINFO, "video use");
        glUseProgram (mId);

        glUniform1i (glGetUniformLocation (mId, "ySampler"), 0);
        glUniform1i (glGetUniformLocation (mId, "uSampler"), 1);
        glUniform1i (glGetUniformLocation (mId, "vSampler"), 2);
        }
      //}}}
      };
    //}}}

    //{{{
    class cOpenGLES3LayerShader : public cLayerShader {
    public:
      cOpenGLES3LayerShader() : cLayerShader() {

        const string kFragShader =
          "#version 300 es\n"
          "precision mediump float;"

          "uniform sampler2D uSampler;"
          "uniform float uHue;"
          "uniform float uVal;"
          "uniform float uSat;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          //{{{
          "vec3 rgbToHsv (float r, float g, float b) {"
          "  float max_val = max(r, max(g, b));"
          "  float min_val = min(r, min(g, b));"
          "  float h;" // hue in degrees

          "  if (max_val == min_val) {" // Simple default case. Do NOT increase saturation if this is the case!
          "    h = 0.0; }"
          "  else if (max_val == r) {"
          "    h = 60.0 * (0.0 + (g - b) / (max_val - min_val)); }"
          "  else if (max_val == g) {"
          "    h = 60.0 * (2.0 + (b - r)/ (max_val - min_val)); }"
          "  else if (max_val == b) {"
          "    h = 60.0 * (4.0 + (r - g) / (max_val - min_val)); }"
          "  if (h < 0.0) {"
          "    h += 360.0; }"

          "  float s = max_val == 0.0 ? 0.0 : (max_val - min_val) / max_val;"
          "  float v = max_val;"
          "  return vec3 (h, s, v);"
          "  }"
          //}}}
          //{{{
          "vec3 hsvToRgb (float h, float s, float v) {"
          "  float r, g, b;"
          "  float c = v * s;"
          "  float h_ = mod(h / 60.0, 6);" // For convenience, change to multiples of 60
          "  float x = c * (1.0 - abs(mod(h_, 2) - 1));"
          "  float r_, g_, b_;"

          "  if (0.0 <= h_ && h_ < 1.0) {"
          "    r_ = c, g_ = x, b_ = 0.0; }"
          "  else if (1.0 <= h_ && h_ < 2.0) {"
          "    r_ = x, g_ = c, b_ = 0.0; }"
          "  else if (2.0 <= h_ && h_ < 3.0) {"
          "    r_ = 0.0, g_ = c, b_ = x; }"
          "  else if (3.0 <= h_ && h_ < 4.0) {"
          "    r_ = 0.0, g_ = x, b_ = c; }"
          "  else if (4.0 <= h_ && h_ < 5.0) {"
          "    r_ = x, g_ = 0.0, b_ = c; }"
          "  else if (5.0 <= h_ && h_ < 6.0) {"
          "    r_ = c, g_ = 0.0, b_ = x; }"
          "  else {"
          "    r_ = 0.0, g_ = 0.0, b_ = 0.0; }"

          "  float m = v - c;"
          "  r = r_ + m;"
          "  g = g_ + m;"
          "  b = b_ + m;"

          "  return vec3 (r, g, b);"
          "  }"
          //}}}

          "void main() {"
          "  outColor = texture (uSampler, textureCoord);"

          "  if (uHue != 0.0 || uVal != 0.0 || uSat != 0.0) {"
          "    vec3 hsv = rgbToHsv (outColor.x, outColor.y, outColor.z);"
          "    hsv.x += uHue;"
          "    if ((outColor.x != outColor.y) || (outColor.y != outColor.z)) {"
                 // not grayscale
          "      hsv.y = uSat <= 0.0 ? "
          "      hsv.y * (1.0 + uSat) : hsv.y + (1.0 - hsv.y) * uSat;"
          "      }"
          "    hsv.z = uVal <= 0.0 ? hsv.z * (1.0 + uVal) : hsv.z + (1.0 - hsv.z) * uVal;"
          "    vec3 rgb = hsvToRgb (hsv.x, hsv.y, hsv.z);"
          "    outColor.xyz = rgb;"
          "    }"
          //"  if (uPreMultiply)"
          //"    outColor.xyz *= outColor.w;"
          "  }";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGLES3LayerShader() {
        glDeleteProgram (mId);
        }
      //}}}

      // sets
      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void setHueSatVal (float hue, float sat, float val) final {
        glUniform1f (glGetUniformLocation (mId, "uHue"), hue);
        glUniform1f (glGetUniformLocation (mId, "uSat"), sat);
        glUniform1f (glGetUniformLocation (mId, "uVal"), val);
        }
      //}}}

      //{{{
      virtual void use() final {

        glUseProgram (mId);
        }
      //}}}
      };
    //}}}
    //{{{
    class cOpenGLES3PaintShader : public cPaintShader {
    public:
      cOpenGLES3PaintShader() : cPaintShader() {
        const string kFragShader =
          "#version 300 es\n"
          "precision mediump float;"

          "uniform sampler2D uSampler;"
          "uniform vec2 uPos;"
          "uniform vec2 uPrevPos;"
          "uniform float uRadius;"
          "uniform vec4 uColor;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "float distToLine (vec2 v, vec2 w, vec2 p) {"
          "  float l2 = pow (distance(w, v), 2.);"
          "  if (l2 == 0.0)"
          "    return distance (p, v);"
          "  float t = clamp (dot (p - v, w - v) / l2, 0., 1.);"
          "  vec2 j = v + t * (w - v);"
          "  return distance (p, j);"
          "  }"

          "void main() {"
          "  float dist = distToLine (uPrevPos.xy, uPos.xy, textureCoord * textureSize (uSampler, 0)) - uRadius;"
          "  outColor = mix (uColor, texture (uSampler, textureCoord), clamp (dist, 0.0, 1.0));"
          "  }";

        mId = compileShader (kQuadVertShader, kFragShader);
        }
      //{{{
      virtual ~cOpenGLES3PaintShader() {
        glDeleteProgram (mId);
        }
      //}}}

      // sets
      //{{{
      virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
        glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
        glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
        }
      //}}}
      //{{{
      virtual void setStroke (cVec2 pos, cVec2 prevPos, float radius, const cColor& color) final {

        glUniform2fv (glGetUniformLocation (mId, "uPos"), 1, (float*)&pos);
        glUniform2fv (glGetUniformLocation (mId, "uPrevPos"), 1, (float*)&prevPos);
        glUniform1f (glGetUniformLocation (mId, "uRadius"), radius);
        glUniform4fv (glGetUniformLocation (mId, "uColor"), 1, (float*)&color);
        }
      //}}}

      //{{{
      virtual void use() final {

        glUseProgram (mId);
        }
      //}}}
      };
    //}}}

    //{{{
    static uint32_t compileShader (const string& vertShaderString, const string& fragShaderString) {

      // compile vertShader
      const GLuint vertShader = glCreateShader (GL_VERTEX_SHADER);
      const GLchar* vertShaderStr = vertShaderString.c_str();
      glShaderSource (vertShader, 1, &vertShaderStr, 0);
      glCompileShader (vertShader);
      GLint success;
      glGetShaderiv (vertShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        //{{{  error, exit
        char errMessage[512];
        glGetProgramInfoLog (vertShader, 512, NULL, errMessage);
        cLog::log (LOGERROR, fmt::format ("vertShader failed {}", errMessage));
        exit (EXIT_FAILURE);
        }
        //}}}

      // compile fragShader
      const GLuint fragShader = glCreateShader (GL_FRAGMENT_SHADER);
      const GLchar* fragShaderStr = fragShaderString.c_str();
      glShaderSource (fragShader, 1, &fragShaderStr, 0);
      glCompileShader (fragShader);
      glGetShaderiv (fragShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        //{{{  error, exit
        char errMessage[512];
        glGetProgramInfoLog (fragShader, 512, NULL, errMessage);
        cLog::log (LOGERROR, fmt::format ("fragShader failed {}", errMessage));
        exit (EXIT_FAILURE);
        }
        //}}}

      // create shader program
      uint32_t id = glCreateProgram();
      glAttachShader (id, vertShader);
      glAttachShader (id, fragShader);
      glLinkProgram (id);
      glGetProgramiv (id, GL_LINK_STATUS, &success);
      if (!success) {
        //{{{  error, exit
        char errMessage[512];
        glGetProgramInfoLog (id, 512, NULL, errMessage);
        cLog::log (LOGERROR, fmt::format ("shaderProgram failed {} ",  errMessage));
        exit (EXIT_FAILURE);
        }
        //}}}

      glDeleteShader (vertShader);
      glDeleteShader (fragShader);

      return id;
      }
    //}}}
    };
  //}}}
#elif defined(VULKAN)
  //{{{
  class cVulkanGraphics : public cGraphics {
  public:
    virtual ~cVulkanGraphics() {
      ImGui_ImplVulkan_Shutdown();
      }

    // imgui
    virtual bool init() final {
      //return ImGui_ImplVulkan_Init (); //ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass
      return true;
      }
    virtual void newFrame() final { ImGui_ImplVulkan_NewFrame(); }
    virtual void clear (const cPoint& size) final {
      (void)size;
      }
    //{{{
    virtual void renderDrawData() final {
      //if (!minimized)
      VkSemaphore imageAcquiredSem = vulkanWindow->FrameSemaphores[vulkanWindow->SemaphoreIndex].ImageAcquiredSemaphore;
      VkSemaphore renderCompleteSem = vulkanWindow->FrameSemaphores[vulkanWindow->SemaphoreIndex].RenderCompleteSemaphore;

      VkResult result = vkAcquireNextImageKHR (gDevice, vulkanWindow->Swapchain, UINT64_MAX,
                                               imageAcquiredSem, VK_NULL_HANDLE, &vulkanWindow->FrameIndex);
      if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        gSwapChainRebuild = true;
        return;
        }
      cGlfwPlatform::checkVkResult (result);

      ImGui_ImplVulkanH_Frame* vulkanFrame = &vulkanWindow->Frames[vulkanWindow->FrameIndex];

      // wait indefinitely instead of periodically checking
      result = vkWaitForFences (gDevice, 1, &vulkanFrame->Fence, VK_TRUE, UINT64_MAX);
      cGlfwPlatform::checkVkResult (result);

      result = vkResetFences (gDevice, 1, &vulkanFrame->Fence);
      cGlfwPlatform::checkVkResult(result);

      result = vkResetCommandPool (gDevice, vulkanFrame->CommandPool, 0);
      cGlfwPlatform::checkVkResult (result);

      VkCommandBufferBeginInfo commandBufferBeginInfo = {};
      commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      result = vkBeginCommandBuffer (vulkanFrame->CommandBuffer, &commandBufferBeginInfo);
      cGlfwPlatform::checkVkResult (result);

      VkRenderPassBeginInfo renderPassBeginInfo = {};
      renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassBeginInfo.renderPass = vulkanWindow->RenderPass;
      renderPassBeginInfo.framebuffer = vulkanFrame->Framebuffer;
      renderPassBeginInfo.renderArea.extent.width = vulkanWindow->Width;
      renderPassBeginInfo.renderArea.extent.height = vulkanWindow->Height;
      renderPassBeginInfo.clearValueCount = 1;
      renderPassBeginInfo.pClearValues = &vulkanWindow->ClearValue;

      vkCmdBeginRenderPass (vulkanFrame->CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

      // record dear imgui primitives into command buffer
      ImGui_ImplVulkan_RenderDrawData (ImGui::GetDrawData(), vulkanFrame->CommandBuffer);

      // submit command buffer
      vkCmdEndRenderPass (vulkanFrame->CommandBuffer);

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

      result = vkQueueSubmit (gQueue, 1, &submitInfo, vulkanFrame->Fence);
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
  //}}}
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
  #if defined(GL_2_1)
    mGraphics = new cGL2Gaphics();
  #elif defined(GL_3)
    mGraphics = new cGL3Graphics();
  #elif defined(GLES_3_0) || defined(GLES_3_1) || defined(GLES_3_2)
    mGraphics = new cGLES3Graphics();
  #elif defined(VULKAN)
    mGraphics = new cVulkanGraphics();
  #else
    #error cGlfwApp.cpp unrecognised BUILD_GRAPHICS cmake option
  #endif

  if (!mGraphics || !mGraphics->init()) {
    cLog::log (LOGERROR, "cApp - graphics init failed");
    return;
    }

  // set callbacks
  glfwPlatform->mResizeCallback = [&](int width, int height) noexcept { windowResize (width, height); };
  glfwPlatform->mDropCallback = [&](vector<string> dropItems) noexcept { drop (dropItems); };

  // fullScreen, vsync
  mPlatform = glfwPlatform;
  mPlatform->setFullScreen (fullScreen);
  mPlatform->setVsync (vsync);
  mPlatformDefined = true;
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
