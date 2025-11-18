//-----------------------------------------------------------------------------------
// file:    TilemapManager.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of the tilemap management system. Handles TileMap-based
//          tilemap storage with automatic rectangular region merging for optimized
//          rendering performance.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "TilemapManager.h"
#include "ProjectSaveManager.h"
#include "Level.h"
#include <algorithm>
#include "FileIO/Settings.h"

namespace TilemapManager {
  namespace {
    // Grid settings
    float grid_snap = 64.0f;
    Math::Vec2D grid_offset = { 0.0f, 0.0f };

    // Tilemap entity per layer (stores TileMap component)
    std::unordered_map<std::string, Engine::ECS::Entity> tilemap_entities_per_layer;
  } // anonymous namespace

  void init() {
    // Find existing TileMap entities
    tilemap_entities_per_layer.clear();
  }

  void set_grid_size(float size) {
    grid_snap = size;
  }

  float get_grid_size() {
    return grid_snap;
  }

  void set_grid_offset(const Math::Vec2D& offset) {
    grid_offset = offset;
  }

  Math::Vec2D get_grid_offset() {
    return grid_offset;
  }

  Math::Vec2D snap_to_grid(const Math::Vec2D& pos) {
    float half_snap = grid_snap * 0.5f;
    float snapped_x = std::floor((pos.x - grid_offset.x) / grid_snap) * grid_snap + grid_offset.x + half_snap;
    float snapped_y = std::floor((pos.y - grid_offset.y) / grid_snap) * grid_snap + grid_offset.y + half_snap;
    return { snapped_x, snapped_y };
  }

  void world_to_grid(const Math::Vec2D& world_pos, int& grid_x, int& grid_y) {
    grid_x = static_cast<int>(std::floor((world_pos.x - grid_offset.x) / grid_snap));
    grid_y = static_cast<int>(std::floor((world_pos.y - grid_offset.y) / grid_snap));
  }

  Math::Vec2D grid_to_world(int grid_x, int grid_y) {
    float half_snap = grid_snap * 0.5f;
    return {
      grid_x * grid_snap + grid_offset.x + half_snap,
      grid_y * grid_snap + grid_offset.y + half_snap
    };
  }

  Engine::ECS::Entity get_or_create_tilemap_entity(const std::string& layer_name, int atlas_index) {
    auto it = tilemap_entities_per_layer.find(layer_name);
    if (it != tilemap_entities_per_layer.end() && it->second.is_valid()) {
      return it->second;
    }

    // Create new tilemap entity
    auto entity = Engine::ECS::create_entity()
      .add_component<Components::TileMap>();

    entity.clear_layers();
    entity.add_to_layer(layer_name);

    auto* int_map = entity.get_component<Components::TileMap>();
    int_map->tile_size = { grid_snap, grid_snap };
    int_map->grid_offset = grid_offset;
    int_map->atlas_index = atlas_index;

    tilemap_entities_per_layer[layer_name] = entity;
    return entity;
  }

  int get_tile_at(const std::string& layer_name, int grid_x, int grid_y) {
    auto it = tilemap_entities_per_layer.find(layer_name);
    if (it == tilemap_entities_per_layer.end() || !it->second.is_valid()) {
      return -1;
    }

    auto* int_map = it->second.get_component<Components::TileMap>();
    if (!int_map) return -1;

    return int_map->get_tile(grid_x, grid_y);
  }

  void rebuild_tilemap_for_layer(const std::string& layer_name,
    const std::vector<Components::AtlasEntry>& loaded_tilemaps) {
    // Destroy all existing tile collision entities on this layer
    auto all_entities = Engine::ECS::get_all_entities();
    for (auto& entity : all_entities) {
      if (entity.has_component<Components::Tile>() && entity.is_on_layer(layer_name)) {
        entity.destroy();
      }
    }

    // Get the tilemap entity for this layer
    auto it = tilemap_entities_per_layer.find(layer_name);
    if (it == tilemap_entities_per_layer.end() || !it->second.is_valid()) {
      return;
    }

    auto* int_map = it->second.get_component<Components::TileMap>();
    if (!int_map || int_map->is_empty()) {
      return;
    }

    // Update the tile size and grid offset to match current settings
    int_map->tile_size = { grid_snap, grid_snap };
    int_map->grid_offset = grid_offset;

    // Get atlas info
    if (int_map->atlas_index < 0 || int_map->atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
      return;
    }

    // Get bounds of the tilemap
    int min_x, min_y, max_x, max_y;
    int_map->get_bounds(min_x, min_y, max_x, max_y);

    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;

    if (width == 0 || height == 0) {
      return;
    }

    // Helper struct for rectangle merging
    struct Rectangle {
      int x, y, width, height;
    };

    // Track processed tiles using relative coordinates
    std::vector<std::vector<bool>> processed(height, std::vector<bool>(width, false));
    std::vector<Rectangle> rectangles;

    // Find all rectangles using greedy algorithm (merge any non-empty tiles)
    for (int rel_y = 0; rel_y < height; ++rel_y) {
      for (int rel_x = 0; rel_x < width; ++rel_x) {
        if (processed[rel_y][rel_x]) continue;

        int abs_x = min_x + rel_x;
        int abs_y = min_y + rel_y;
        int tile_index = int_map->get_tile(abs_x, abs_y);

        if (tile_index < 0) {
          processed[rel_y][rel_x] = true;
          continue;
        }

        // Find maximum width at this row (any non-empty tile)
        int max_width = 0;
        for (int w = 0; rel_x + w < width; ++w) {
          if (processed[rel_y][rel_x + w] || int_map->get_tile(abs_x + w, abs_y) < 0) {
            break;
          }
          max_width++;
        }

        if (max_width == 0) {
          processed[rel_y][rel_x] = true;
          continue;
        }

        // Expand downward while maintaining the width (any non-empty tile)
        int rect_height = 1;
        for (int h = 1; rel_y + h < height; ++h) {
          bool can_expand = true;
          for (int w = 0; w < max_width; ++w) {
            if (processed[rel_y + h][rel_x + w] || int_map->get_tile(abs_x + w, abs_y + h) < 0) {
              can_expand = false;
              break;
            }
          }
          if (!can_expand) break;
          rect_height++;
        }

        // Mark all tiles in this rectangle as processed
        for (int h = 0; h < rect_height; ++h) {
          for (int w = 0; w < max_width; ++w) {
            processed[rel_y + h][rel_x + w] = true;
          }
        }

        // Store rectangle with absolute coordinates
        rectangles.push_back({ abs_x, abs_y, max_width, rect_height });
      }
    }

    // Create collision-only entities for each rectangle (no rendering components)
    for (const Rectangle& rect : rectangles) {
      // Calculate the world position for the center of the rectangle
      float center_x = grid_offset.x + rect.x * grid_snap + (rect.width * grid_snap) * 0.5f;
      float center_y = grid_offset.y + rect.y * grid_snap + (rect.height * grid_snap) * 0.5f;
      Math::Vec2D center_world = { center_x, center_y };
      Math::Vec2D scale = { rect.width * grid_snap, rect.height * grid_snap };

      // Create collision-only entity (no rendering components)
      auto entity = Engine::ECS::create_entity()
        .add_component<Components::Tile>() // Marker component to identify tile collision entities
        .add_component<Components::Transform2D>(center_world, 0.0f, scale)
        .add_component<Components::RigidBody>(true, false) // is_static = true, no gravity
        .add_component<Components::BoxCollider>(scale.x, scale.y, true); // resolve_collisions = true

      entity.clear_layers();
      entity.add_to_layer(layer_name);
    }
  }  void place_tile(const Math::Vec2D& world_pos, const std::string& layer_name,
    int atlas_index, int tile_index,
    const std::vector<Components::AtlasEntry>& loaded_tilemaps) {
    int grid_x, grid_y;
    world_to_grid(world_pos, grid_x, grid_y);

    // Get the tilemap entity
    auto tilemap_entity = get_or_create_tilemap_entity(layer_name, atlas_index);
    auto* int_map = tilemap_entity.get_component<Components::TileMap>();
    if (!int_map) return;

    // Update atlas index if it changed
    if (int_map->atlas_index != atlas_index) {
      int_map->atlas_index = atlas_index;
    }

    // Check if the tile is already the same
    int current_tile = int_map->get_tile(grid_x, grid_y);
    if (current_tile == tile_index) {
      return; // Same tile, nothing to do
    }

    // Set the tile
    int_map->set_tile(grid_x, grid_y, tile_index);

    // Update tile size and grid offset
    int_map->tile_size = { grid_snap, grid_snap };
    int_map->grid_offset = grid_offset;

    // Rebuild the tilemap to create collision entities
    rebuild_tilemap_for_layer(layer_name, loaded_tilemaps);

    // Mark level as dirty
    Engine::ProjectSaveManager::set_dirty(
      Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path())
    );
  }

  void delete_tile(const Math::Vec2D& world_pos, const std::string& layer_name) {
    int grid_x, grid_y;
    world_to_grid(world_pos, grid_x, grid_y);

    // Get the tilemap entity
    auto it = tilemap_entities_per_layer.find(layer_name);
    if (it == tilemap_entities_per_layer.end() || !it->second.is_valid()) {
      return;
    }

    auto* int_map = it->second.get_component<Components::TileMap>();
    if (!int_map) return;

    int old_tile = int_map->get_tile(grid_x, grid_y);
    if (old_tile < 0) return; // Nothing to delete

    // Delete the tile
    int_map->set_tile(grid_x, grid_y, -1);

    // We need to pass loaded_tilemaps to rebuild, get it from project settings
    const auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get();
    rebuild_tilemap_for_layer(layer_name, project_settings.loaded_tilemaps);

    // Mark level as dirty
    Engine::ProjectSaveManager::set_dirty(
      Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path())
    );
  }

  void update_all_tile_dimensions(const std::vector<Components::AtlasEntry>& loaded_tilemaps) {
    // Update tile size and grid offset in all tilemap entities and rebuild collision entities
    for (auto& [layer_name, entity] : tilemap_entities_per_layer) {
      if (entity.is_valid()) {
        auto* int_map = entity.get_component<Components::TileMap>();
        if (int_map) {
          int_map->tile_size = { grid_snap, grid_snap };
          int_map->grid_offset = grid_offset;
        }
        rebuild_tilemap_for_layer(layer_name, loaded_tilemaps);
      }
    }
  }

} // namespace TilemapManager
