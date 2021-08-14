project (platform)

if (PLATFORM STREQUAL "PLATFORM_WIN32")
  add_library (platform
               cPlatform.h
               cPlatform-win32-wrapper.cpp
               ../imgui/backends/imgui_impl_win32.cpp
               ../imgui/backends/imgui_impl_win32.h
               )
elseif (PLATFORM STREQUAL "PLATFORM_GLFW")
  add_library (platform
               cPlatform.h
               cPlatform-glfw-wrapper.cpp
               ../imgui/backends/imgui_impl_glfw.cpp
               ../imgui/backends/imgui_impl_glfw.h
               )
endif()
