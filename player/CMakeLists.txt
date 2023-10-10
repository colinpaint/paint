project (player VERSION 1.0.0)
  add_executable (${PROJECT_NAME} cPlayerApp.h ../ui/cUI.h
                                  playerMain.cpp cPlayerUI.cpp cPlayerApp.cpp ../ui/cUI.cpp)

  target_include_directories (${PROJECT_NAME} PRIVATE app common imgui ../stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE song audio decoders dvb net imgui
                                                   opengl32 d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE song audio decoders dvb net imgui
                                                   asound stdc++fs pthread)
  endif()
