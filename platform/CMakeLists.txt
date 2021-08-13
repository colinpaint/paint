project (platform)

if (PLATFORM STREQUAL "PLATFORM_WIN32")
#  add_library (platform cPlatform-win32.cpp cPlatform.h)
  add_library (platform
               cPlatform-win32-wrapper.cpp
               ../imGui/backends/imgui_impl_win32.cpp
               ../imGui/backends/imgui_impl_win32.h
               cPlatform.h)
else()
  add_library (platform
               cPlatform-glfw-wrapper.cpp
               ../imGui/backends/imgui_impl_glfw.cpp
               ../imGui/backends/imgui_impl_glfw.h
               cPlatform.h)
endif()
