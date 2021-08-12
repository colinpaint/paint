# paint

Simple paint testbed, just for fun
- uses ImGui and cmake
- based on ImGui-paint, which demonstrated this combination worked as a testbed
- moving towards a paintbox/photoShop mashup

build linux

git clone http://github/colinpaint/paint
cd paint
mkdir build
cd build
cmake ..
make -j 4
./paintbox


build windows command prompt

git clone http://github/colinpaint/paint
cd paint
mkdir build
cd build
cmake ..

find the .sln in the build directory and build with visual studio 19 community 
