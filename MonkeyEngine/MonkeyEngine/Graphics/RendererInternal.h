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
      int atlas_index = 0; // Index into TextureAtlas for this frame
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

    // Tile collider type - determines if tiles can be merged for collision optimization
    enum class TileColliderType {
      Default,  // Can be merged with adjacent Default tiles into rectangles
      AABB,     // Custom AABB box with width/height/offset, individual per tile
      Polygon   // Custom polygon with vertices, individual per tile
    };

    // Tile data used in tilemap rendering - contains ONLY references to atlas and tile
    // Collision configuration is looked up at runtime from AtlasEntry.tile_collider_configs
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

    // Collision configuration for a tile - stored in AtlasEntry.tile_collider_configs
    struct TileColliderConfig {
      TileColliderType collider_type = TileColliderType::Default;

      // AABB collider properties (for AABB collider type)
      float aabb_width = 0.0f;
      float aabb_height = 0.0f;
      Math::Vec2D aabb_offset = { 0.0f, 0.0f };

      // Polygon collider properties (for Polygon collider type)
      std::vector<Math::Vec2D> custom_vertices;

      TileColliderConfig() = default;
    };

    // Forward declare Texture type (defined in Renderer.h)
    using Texture = std::shared_ptr<TextureData>;

    // Animation configuration for animated tile atlases
    struct AnimatedTileConfig {
      bool is_animated = false;           // Whether this atlas contains animated tiles
      int sheet_cols = 1;                 // Total columns in the sheet (exact value, not derived)
      int sheet_rows = 1;                 // Total rows in the sheet (exact value, not derived)
      int prop_cols = 1;                  // How many grid columns one prop occupies in the world
      int prop_rows = 1;                  // How many grid rows one prop occupies in the world  
      int anim_cols = 1;                  // How many columns of animation frames in the texture
      int anim_rows = 1;                  // How many rows of animation frames in the texture
      float frame_duration = 0.1f;        // Duration of each animation frame in seconds
      bool is_looping = true;             // Whether the animation loops

      // Get total frame count
      int get_frame_count() const { return anim_cols * anim_rows; }

      // Get total animation duration
      float get_total_duration() const { return frame_duration * get_frame_count(); }
    };

    // Atlas entry for tilemap rendering - flattened structure
    struct AtlasEntry {
      Texture texture_handle;                    // Shared pointer to texture data (runtime only, not serialized)
      std::vector<SubTexture> subTextures;       // UV coordinates for each tile in the atlas
      int drawIndex = -1;                        // Current subTexture index to draw (unused in tilemap)
      std::string texture_path;                  // Path to texture file (for serialization/deserialization)
      std::unordered_map<int, TileColliderConfig> tile_collider_configs; // Per-tile collider configurations (tile_index -> config)
      AnimatedTileConfig animation_config;       // Animation configuration for this atlas
      float friction = 1.0f;                     // Friction coefficient applied to colliders generated from this atlas

      AtlasEntry() : texture_handle(nullptr), subTextures(), drawIndex(-1), texture_path(""), friction(1.0f) {}
    };

  }
}

REFLECT_ENUM(Engine::Graphics::TileColliderType, Default, AABB, Polygon);
REFLECT_FIELDS(Engine::Graphics::SubTexture, tex_coords);
REFLECT_FIELDS(Engine::Graphics::Tile, atlas_index, tile_index);
REFLECT_FIELDS(Engine::Graphics::TileColliderConfig, collider_type, aabb_width, aabb_height, aabb_offset, custom_vertices);
REFLECT_FIELDS(Engine::Graphics::AnimatedTileConfig, is_animated, sheet_cols, sheet_rows, prop_cols, prop_rows, anim_cols, anim_rows, frame_duration, is_looping);
REFLECT_FIELDS(Engine::Graphics::AtlasEntry, texture_path, subTextures, drawIndex, tile_collider_configs, animation_config, friction);