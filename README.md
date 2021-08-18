# paint

Simple paint testbed, just for fun
- uses ImGui and cmake
- based on ImGui-paint, which demonstrated this combination worked as a testbed
- moving towards a paintbox/photoShop mashup

build 

git clone http://github/colinpaint/paint
  cd paint
  mkdir build
  cd build
  cmake ..

Linux
  make -j 4
  ./paintbox

Windows
  paintbox.sln in build directory
