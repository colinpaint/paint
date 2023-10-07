project (libmfx VERSION 1.0.0)
  file (GLOB HEADER_FILES include/*.h mfx_dispatch/windows/include/*.h)
  file (GLOB SOURCE_FILES mfx_dispatch/windows/src/*.cpp)
  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

  target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_MFX)
  target_include_directories (${PROJECT_NAME} PRIVATE include mfx_dispatch/windows/include)
