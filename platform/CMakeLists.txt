project (platform)

if (PLATFORM STREQUAL "PLATFORM_WIN32")
  add_library (platform cPlatform-Win32.cpp cPlatform.h)
else()
  add_library (platform cPlatform-Glfw.cpp cPlatform.h)
endif()
