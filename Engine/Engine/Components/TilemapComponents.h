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

  // IntegerMap stores a 2D grid of tile indices as a 2D array
  // Data is stored in row-major order: integerMap[row][column]
  // Value: tile index in the atlas (-1 means empty)
  struct IntegerMap {
    std::vector<std::vector<int>> tiles; // 2D array of tile indices
    Math::Vec2D tile_size = { 64.0f, 64.0f }; // Size of each tile in world units
    Math::Vec2D grid_offset = { 0.0f, 0.0f }; // World position offset for tilemap origin
    int atlas_index = -1; // Which atlas this tilemap uses

    // Get the number of rows
    int get_height() const {
      return static_cast<int>(tiles.size());
    }

    // Get the number of columns
    int get_width() const {
      return tiles.empty() ? 0 : static_cast<int>(tiles[0].size());
    }

    // Helper to get tile at grid position (returns -1 if out of bounds)
    int get_tile(int x, int y) const {
      if (y < 0 || y >= static_cast<int>(tiles.size())) return -1;
      if (x < 0 || x >= static_cast<int>(tiles[y].size())) return -1;
      return tiles[y][x];
    }

    // Helper to set tile at grid position (auto-expands array if needed)
    void set_tile(int x, int y, int tile_index) {
      if (x < 0 || y < 0) return;

      // Expand rows if necessary
      if (y >= static_cast<int>(tiles.size())) {
        tiles.resize(y + 1);
      }

      // Expand columns if necessary
      if (x >= static_cast<int>(tiles[y].size())) {
        for (auto& row : tiles) {
          if (static_cast<int>(row.size()) <= x) {
            row.resize(x + 1, -1); // Fill with -1 (empty)
          }
        }
      }

      tiles[y][x] = tile_index;

      // Trim empty rows from the end
      while (!tiles.empty() && std::all_of(tiles.back().begin(), tiles.back().end(), [](int v) { return v == -1; })) {
        tiles.pop_back();
      }

      // Trim empty columns from the end
      if (!tiles.empty()) {
        int max_col = 0;
        for (const auto& row : tiles) {
          for (int col = static_cast<int>(row.size()) - 1; col >= 0; --col) {
            if (row[col] != -1) {
              max_col = std::max(max_col, col + 1);
              break;
            }
          }
        }
        for (auto& row : tiles) {
          if (static_cast<int>(row.size()) > max_col) {
            row.resize(max_col);
          }
        }
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
REGISTER_COMPONENT(Components::IntegerMap, tiles, tile_size, grid_offset, atlas_index);