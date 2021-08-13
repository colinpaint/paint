project (platform)

if (PLATFORM STREQUAL "PLATFORM_WIN32")
  add_library (platform cPlatform-win32.cpp cPlatform.h)
else()
  add_library (platform
               cPlatform-glfw-wrapper.cpp
               ../imGui/backends/imgui_impl_glfw.cpp
               ../imGui/backends/imgui_impl_glfw.h
               cPlatform.h)
#add_library (platform cPlatform-glfw-wrapper.cpp cPlatform.h)
endif()
