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

    // Tile data used in tilemap rendering - contains atlas reference and tile index
    struct Tile {
      int atlas_index = -1; // Which atlas this tile uses
      int tile_index = -1;  // Which tile within the atlas

      // Default constructor for empty tiles
      Tile() = default;

      // Constructor for setting atlas and tile index
      Tile(int atlas_idx, int tile_idx)
        : atlas_index(atlas_idx), tile_index(tile_idx) {
      }

      // Check if this is a valid tile
      bool is_valid() const {
        return atlas_index >= 0 && tile_index >= 0;
      }
    };

    // Forward declare Texture type (defined in Renderer.h)
    using Texture = std::shared_ptr<TextureData>;

    // Atlas entry for tilemap rendering - flattened structure
    struct AtlasEntry {
      Texture texture_handle;                    // Shared pointer to texture data (runtime only, not serialized)
      std::vector<SubTexture> subTextures;       // UV coordinates for each tile in the atlas
      int drawIndex = -1;                        // Current subTexture index to draw (unused in tilemap)
      std::string texture_path;                  // Path to texture file (for serialization/deserialization)

      AtlasEntry() : texture_handle(nullptr), subTextures(), drawIndex(-1), texture_path("") {}

      // Custom reflector for serialization/deserialization
      struct Reflector {
        std::string texture_path;
        std::vector<std::array<Math::Vec2D, 4>> tex_coords_list;
        int drawIndex;

        static Reflector reflect(const AtlasEntry& entry);
        static AtlasEntry create(const Reflector& r);
      };
    };

  }
}

REFLECT_FIELDS(Engine::Graphics::SubTexture, tex_coords);
REFLECT_FIELDS(Engine::Graphics::Tile, atlas_index, tile_index);
REFLECT_CUSTOM(Engine::Graphics::AtlasEntry, texture_path, tex_coords_list, drawIndex);