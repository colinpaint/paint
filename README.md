# paint

Simple imgui app testbed
- uses ImGui, cmake

build
- git clone http://github/colinpaint/paint
- cd paint

Linux
- mkdir build
- cd build
- cmake ..
- ccmake ..
- make -j4
- ./paintbox

Windows
- mkdir windowsBuild
- cd windowsBuild
- cmake ..
- paintbox.sln in windowsBuild directory

to use clang
- CXX="clang++" CC="clang" cmake ..
