// myImguiWidgets.h - my imgui widgets - freestanding routines like ImGui::Button but with string and other stuff
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include "imgui.h"

void printHex (uint8_t* ptr, uint32_t numBytes, uint32_t columnsPerRow, uint32_t address, bool full);

bool clockButton (const std::string& label, std::chrono::system_clock::time_point timePoint,
                 const ImVec2& size_arg = ImVec2(0,0));

bool toggleButton (const std::string& label, bool toggleOn,
                   const ImVec2& size_arg = ImVec2(0,0));

uint8_t interlockedButtons (const std::vector<std::string>& buttonVector, uint8_t index,
                            const ImVec2& size_arg = ImVec2(0,0));
