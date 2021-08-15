project (platform)

file (GLOB HEADER_FILES *.h)

file (GLOB SRCFILES
  cPlatform.cpp
  ../imgui/backends/imgui_impl_glfw.cpp
  )

if (WIN32)
  add_library (platform ${SRCFILES} ../imgui/backends/imgui_impl_win32.cpp ${HEADER_FILES})
else()
  add_library (platform ${SRCFILES} ${HEADER_FILES})

endif()
