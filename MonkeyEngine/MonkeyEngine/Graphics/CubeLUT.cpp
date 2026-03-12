//-----------------------------------------------------------------------------------
// file:    CubeLUT.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of .cube LUT file parser and identity LUT generator.
//          Reads standard .cube format: TITLE, LUT_3D_SIZE, and float RGB triplets.
//
// Copyright  2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "CubeLUT.h"
#include "../Logger.h"

#include <fstream>
#include <sstream>

namespace Engine::Graphics {

  CubeLUTData parse_cube_lut(const std::string& filepath) {
    CubeLUTData result;

    std::ifstream file(filepath);
    if (!file.is_open()) {
      Log::error("CubeLUT: Failed to open file: {}", filepath);
      return result;
    }

    std::string line;
    int expected_entries = 0;
    int entries_read = 0;

    while (std::getline(file, line)) {
      // Skip empty lines and comments
      if (line.empty() || line[0] == '#') continue;

      // Parse TITLE (skip it)
      if (line.rfind("TITLE", 0) == 0) continue;

      // Parse DOMAIN_MIN / DOMAIN_MAX (skip for now, assume 0-1)
      if (line.rfind("DOMAIN_MIN", 0) == 0) continue;
      if (line.rfind("DOMAIN_MAX", 0) == 0) continue;

      // Parse LUT_3D_SIZE
      if (line.rfind("LUT_3D_SIZE", 0) == 0) {
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword >> result.size;
        expected_entries = result.size * result.size * result.size;
        result.data.reserve(expected_entries * 3);
        continue;
      }

      // Parse RGB data lines
      if (result.size > 0 && entries_read < expected_entries) {
        std::istringstream iss(line);
        float r, g, b;
        if (iss >> r >> g >> b) {
          result.data.push_back(r);
          result.data.push_back(g);
          result.data.push_back(b);
          entries_read++;
        }
      }
    }

    if (entries_read != expected_entries) {
      Log::error("CubeLUT: Expected {} entries but read {} in {}", expected_entries, entries_read, filepath);
      result.size = 0;
      result.data.clear();
      return result;
    }

    Log::info("CubeLUT: Loaded {}x{}x{} LUT from {}", result.size, result.size, result.size, filepath);
    return result;
  }

  CubeLUTData generate_identity_lut(int size) {
    CubeLUTData result;
    result.size = size;
    result.data.resize(size * size * size * 3);

    float scale = 1.0f / (size - 1);
    int idx = 0;
    for (int b = 0; b < size; ++b) {
      for (int g = 0; g < size; ++g) {
        for (int r = 0; r < size; ++r) {
          result.data[idx++] = r * scale;
          result.data[idx++] = g * scale;
          result.data[idx++] = b * scale;
        }
      }
    }

    return result;
  }

} // namespace Engine::Graphics
