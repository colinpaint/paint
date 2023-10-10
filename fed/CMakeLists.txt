project (fed VERSION 1.0.0)
  add_executable (${PROJECT_NAME} cFedApp.h cDocument.h cMemEdit.h cTextEdit.h ../ui/cUI.h
                                  fedMain.cpp cFedApp.cpp cDocument.cpp
                                  cEditUI.cpp cMemEdit.cpp cTextEdit.cpp ../ui/cUI.cpp)

  target_include_directories (${PROJECT_NAME} PRIVATE app common imgui ../stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui opengl32 d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui stdc++fs)
  endif()
