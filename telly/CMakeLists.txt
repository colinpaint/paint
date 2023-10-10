project (telly VERSION 1.0.0)
  add_executable (${PROJECT_NAME} cTellyApp.h ../ui/cUI.h
                                  cTellyApp.cpp cTellyUI.cpp tellyMain.cpp ../ui/cUI.cpp)

  target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
  target_include_directories (${PROJECT_NAME} PRIVATE app common implot imgui stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio decoders imgui implot
                                                   libmfx opengl32 dxgi d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio decoders imgui implot
                                                   asound stdc++fs pthread)
  endif()
