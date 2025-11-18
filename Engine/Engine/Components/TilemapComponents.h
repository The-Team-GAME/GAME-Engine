//-----------------------------------------------------------------------------------
// file:    TilemapComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for the tilemap system. Includes Tile
//          component for marking tile entities and AtlasEntry for storing
//          texture atlas information used in tilemap editing and serialization.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../ECS/Macros.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>

namespace Components {
  struct Tile {
    Math::Vec2D size = { 64.0f, 64.0f };
  };

  // Only used in TilemapPanel and ProjectSettings for saving/loading tilemaps
  struct AtlasEntry {
    Components::Texture texture;
    Components::TextureAtlas atlas;
  };

  // TileMap stores a 2D grid of tile indices using a sparse hashmap
  // Supports any integer coordinates (positive, negative, sparse)
  // Value: tile index in the atlas (-1 or missing means empty)
  struct TileMap {
    std::unordered_map<int64_t, int> tiles; // Sparse map of tile indices (key = encoded x,y)
    Math::Vec2D tile_size = { 64.0f, 64.0f }; // Size of each tile in world units
    Math::Vec2D grid_offset = { 0.0f, 0.0f }; // World position offset for tilemap origin
    int atlas_index = -1; // Which atlas this tilemap uses

    // Encode x,y coordinates into a single int64_t key
    static int64_t encode_key(int x, int y) {
      return (static_cast<int64_t>(y) << 32) | (static_cast<uint32_t>(x));
    }

    // Decode int64_t key back into x,y coordinates
    static void decode_key(int64_t key, int& x, int& y) {
      y = static_cast<int>(key >> 32);
      x = static_cast<int>(key & 0xFFFFFFFF);
    }

    // Get bounds of the tilemap (min/max x and y)
    void get_bounds(int& min_x, int& min_y, int& max_x, int& max_y) const {
      if (tiles.empty()) {
        min_x = min_y = max_x = max_y = 0;
        return;
      }

      min_x = min_y = INT_MAX;
      max_x = max_y = INT_MIN;

      for (const auto& [key, tile_index] : tiles) {
        if (tile_index == -1) continue; // Skip empty tiles

        int x, y;
        decode_key(key, x, y);
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
      }
    }

    // Get the number of rows (height of bounding box)
    int get_height() const {
      int min_x, min_y, max_x, max_y;
      get_bounds(min_x, min_y, max_x, max_y);
      return max_y - min_y + 1;
    }

    // Get the number of columns (width of bounding box)
    int get_width() const {
      int min_x, min_y, max_x, max_y;
      get_bounds(min_x, min_y, max_x, max_y);
      return max_x - min_x + 1;
    }

    // Helper to get tile at grid position (returns -1 if not found)
    int get_tile(int x, int y) const {
      int64_t key = encode_key(x, y);
      auto it = tiles.find(key);
      return (it != tiles.end()) ? it->second : -1;
    }

    // Helper to set tile at grid position
    void set_tile(int x, int y, int tile_index) {
      int64_t key = encode_key(x, y);

      if (tile_index == -1) {
        // Remove empty tiles to keep map sparse
        tiles.erase(key);
      }
      else {
        tiles[key] = tile_index;
      }
    }

    // Check if the tilemap is empty
    bool is_empty() const {
      return tiles.empty();
    }

    // Clear all tiles
    void clear() {
      tiles.clear();
    }
  };
}

REFLECT_FIELDS(Components::Tile, size);
REFLECT_FIELDS(Components::AtlasEntry, texture, atlas);
REGISTER_COMPONENT(Components::TileMap, tiles, tile_size, grid_offset, atlas_index);