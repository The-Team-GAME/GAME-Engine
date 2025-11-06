//-----------------------------------------------------------------------------------
// file:    RendererInternal.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Internal data structures and utilities for the graphics renderer.
//          Defines Frame, SubTexture, Glyph, and other internal structures used
//          by animation, texture atlas, and font rendering systems. Provides
//          FreeType library access for font processing.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Precompiled.h"
#include <memory> 
#include <unordered_map>
#include <array>
#include <string>
#include "../Logger.h"
#include "../TypeDef.h"
#include "../Math/Vector2D.h"

namespace Engine {
  namespace Graphics
  {
    FT_Library get_freetype_library();

    // Used by Animation component
    struct Frame
    {
      float duration;   // Duration of this frame in seconds
      float before_time; // Time before this frame starts (for internal calculations)
    };

    // Used by TextureAtlas and Font components
    struct SubTexture
    {
      std::array<Math::Vec2D, 4> tex_coords; // UV coordinates in the atlas
    };

    // Used by Font component
    struct Glyph
    {
      std::array<Math::Vec2D, 4> tex_coords; // UV coordinates in the atlas
      glm::ivec2 size;
      glm::ivec2 bearing;
      unsigned int advance;
    };

    // Add filtering mode enum
    enum class TextureFilter {
      Nearest,  // For pixel art
      Linear    // For smooth textures
    };

    // Holds information about a loaded texture including its OpenGL ID and dimensions.
    struct TextureData
    {
      unsigned int id = 0; // OpenGL handle (0 means invalid)
      int width = 0;
      int height = 0;
      TextureFilter filter = TextureFilter::Nearest; // Default to pixel art
      std::string path; // File path for reflection

      TextureData(const std::string& path, TextureFilter filtering = TextureFilter::Nearest);
      ~TextureData();

      // Delete copy constructor and assignment operator to prevent accidental copies
      TextureData(const TextureData&) = delete;
      TextureData& operator=(const TextureData&) = delete;
      TextureData(const TextureData&&) = delete;

      // Default constructor
      TextureData();

      // Method to change filtering mode after creation
      void set_filter(TextureFilter filtering);
    };


    // Font resource data structure
    struct FontData
    {
      TextureData texture_atlas;                                 // Glyph atlas texture
      std::unordered_map<unsigned int, Graphics::Glyph> glyphs;  // Character -> glyph mapping
      unsigned int font_size = 0;                                    // Font size in pixels
      float baseline = 0.0f;                                            // Baseline offset
      std::string path; // File path for reflection

      FontData(const std::string& path, unsigned int size);
    };

  }
}