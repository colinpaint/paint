# cmake config
cmake_minimum_required (VERSION 3.10)

# project name
project (binaryToCompressedC)

# config
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)

# main executable
add_executable (binaryToCompressedC binaryToCompressedC.cpp)

target_link_libraries (binaryToCompressedC)
