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

namespace Components {
  struct Tile {};

  // Only used in TilemapPanel and ProjectSettings for saving/loading tilemaps
  struct AtlasEntry {
    Components::Texture texture;
    Components::TextureAtlas atlas;
  };

  // IntegerMap stores a 2D grid of tile indices
  // Key format: "x_y" where x and y are grid coordinates
  // Value: tile index in the atlas (-1 means empty)
  struct IntegerMap {
    std::unordered_map<std::string, int> tiles;
    float grid_size = 64.0f;
    int atlas_index = -1; // Which atlas this tilemap uses

    // Helper to get/set tile at grid position
    int get_tile(int x, int y) const {
      auto key = std::to_string(x) + "_" + std::to_string(y);
      auto it = tiles.find(key);
      return (it != tiles.end()) ? it->second : -1;
    }

    void set_tile(int x, int y, int tile_index) {
      auto key = std::to_string(x) + "_" + std::to_string(y);
      if (tile_index < 0) {
        tiles.erase(key);
      }
      else {
        tiles[key] = tile_index;
      }
    }
  };
}

REGISTER_COMPONENT(Components::Tile);
REGISTER_COMPONENT(Components::AtlasEntry, texture, atlas);
REFLECT_FIELDS(Components::IntegerMap, tiles, grid_size, atlas_index);