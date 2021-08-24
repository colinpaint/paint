// utils.h
#pragma once
#include <cstdint>
#include <string>
#include "../utils/core.h"
#include "../utils/date.h"


inline std::string getTimeString (uint32_t secs) {
  return fmt::format ("{}:{}:{}", secs / (60 * 60), (secs / 60) % 60, secs % 60);
  }
