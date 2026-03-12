//-----------------------------------------------------------------------------------
// file:    CubeLUT.h
// project: Monkey Stole My GPA
//
// brief:   Parser for .cube LUT files. Reads LUT_3D_SIZE and RGB triplet data
//          into a flat float array suitable for uploading to GL_TEXTURE_3D.
//
// Copyright  2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

namespace Engine::Graphics {

  struct CubeLUTData {
    int size = 0;                   // LUT dimension (e.g. 25 for a 25x25x25 LUT)
    std::vector<float> data;        // RGB float triplets, size^3 * 3 elements
  };

  // Parse a .cube LUT file and return the data.
  // Returns a CubeLUTData with size=0 if parsing fails.
  CubeLUTData parse_cube_lut(const std::string& filepath);

  // Generate an identity (no-op) LUT of the given dimension.
  CubeLUTData generate_identity_lut(int size = 16);

} // namespace Engine::Graphics
