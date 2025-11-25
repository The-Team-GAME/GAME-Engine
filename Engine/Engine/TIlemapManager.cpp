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
#include <limits>
#include "FileIO/Settings.h"
#include "Systems/CollisionSystem.h"

namespace TilemapManager {
  namespace {
    // Grid settings
    float grid_snap = 64.0f;
    Math::Vec2D grid_offset = { 0.0f, 0.0f };
  } // anonymous namespace

  void init() {
    // No longer need to maintain a cache - we query ECS directly
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

  Engine::ECS::Entity get_or_create_tilemap_entity(const std::string& layer_name) {
    // Check if there's already a TileMap entity on this layer in the world
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
      if (entity.is_on_layer(layer_name)) {
        return entity;
      }
    }

    // Create new tilemap entity
    auto entity = Engine::ECS::create_entity()
      .add_component<Components::TileMap>();

    entity.clear_layers();
    entity.add_to_layer(layer_name);

    auto* int_map = entity.get_component<Components::TileMap>();
    int_map->tile_size = { grid_snap, grid_snap };
    int_map->grid_offset = grid_offset;

    return entity;
  }

  Engine::Graphics::Tile get_tile_at(const std::string& layer_name, int grid_x, int grid_y) {
    // Find the TileMap entity on this layer
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
      if (entity.is_on_layer(layer_name)) {
        auto* int_map = entity.get_component<Components::TileMap>();
        if (int_map) {
          return int_map->get_tile(grid_x, grid_y);
        }
      }
    }

    return Engine::Graphics::Tile();
  }

  void rebuild_tilemap_for_layer(const std::string& layer_name) {
    // Destroy all existing tile collision entities on this layer
    auto all_entities = Engine::ECS::get_all_entities();
    for (auto& entity : all_entities) {
      if (entity.has_component<Components::TileCollider>() && entity.is_on_layer(layer_name)) {
        entity.destroy();
      }
    }

    // Find the tilemap entity for this layer (don't create if it doesn't exist)
    Engine::ECS::Entity tilemap_entity;
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
      if (entity.is_on_layer(layer_name)) {
        tilemap_entity = entity;
        break;
      }
    }

    // If no tilemap entity exists, nothing to rebuild
    if (!tilemap_entity.is_valid()) {
      return;
    }

    auto* int_map = tilemap_entity.get_component<Components::TileMap>();
    if (!int_map || int_map->is_empty()) {
      return;
    }

    // Update the tile size and grid offset to match current settings
    int_map->tile_size = { grid_snap, grid_snap };
    int_map->grid_offset = grid_offset;

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

    // Find all rectangles using greedy algorithm (merge only tiles with Default collider type)
    for (int rel_y = 0; rel_y < height; ++rel_y) {
      for (int rel_x = 0; rel_x < width; ++rel_x) {
        if (processed[rel_y][rel_x]) continue;

        int abs_x = min_x + rel_x;
        int abs_y = min_y + rel_y;
        Engine::Graphics::Tile tile = int_map->get_tile(abs_x, abs_y);

        if (!tile.is_valid()) {
          processed[rel_y][rel_x] = true;
          continue;
        }

        // Skip tiles with Custom collider type - they will be handled separately
        if (tile.collider_type == Engine::Graphics::TileColliderType::Custom) {
          processed[rel_y][rel_x] = true;
          continue;
        }

        // Find maximum width at this row (only merge Default collider type tiles)
        int max_width = 0;
        for (int w = 0; rel_x + w < width; ++w) {
          if (processed[rel_y][rel_x + w]) break;

          Engine::Graphics::Tile check_tile = int_map->get_tile(abs_x + w, abs_y);
          if (!check_tile.is_valid()) break;
          if (check_tile.collider_type != Engine::Graphics::TileColliderType::Default) break;

          max_width++;
        }

        if (max_width == 0) {
          processed[rel_y][rel_x] = true;
          continue;
        }

        // Expand downward while maintaining the width (only Default collider type tiles)
        int rect_height = 1;
        for (int h = 1; rel_y + h < height; ++h) {
          bool can_expand = true;
          for (int w = 0; w < max_width; ++w) {
            if (processed[rel_y + h][rel_x + w]) {
              can_expand = false;
              break;
            }

            Engine::Graphics::Tile check_tile = int_map->get_tile(abs_x + w, abs_y + h);
            if (!check_tile.is_valid() || check_tile.collider_type != Engine::Graphics::TileColliderType::Default) {
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
        .add_component<Components::TileCollider>() // Marker component to identify tile collision entities
        .add_component<Components::Transform2D>(center_world, 0.0f, scale)
        .add_component<Components::RigidBody>(true, false) // is_static = true, no gravity
        .add_component<Components::BoxCollider>(scale.x, scale.y, true); // resolve_collisions = true

      entity.clear_layers();
      entity.add_to_layer(layer_name);
    }

    // Create individual collision entities for Custom collider type tiles
    auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get_mutable();

    for (int rel_y = 0; rel_y < height; ++rel_y) {
      for (int rel_x = 0; rel_x < width; ++rel_x) {
        int abs_x = min_x + rel_x;
        int abs_y = min_y + rel_y;
        Engine::Graphics::Tile tile = int_map->get_tile(abs_x, abs_y);

        if (tile.is_valid() && tile.collider_type == Engine::Graphics::TileColliderType::Custom) {
          // Calculate world position for this individual tile
          float center_x = grid_offset.x + abs_x * grid_snap + grid_snap * 0.5f;
          float center_y = grid_offset.y + abs_y * grid_snap + grid_snap * 0.5f;
          Math::Vec2D center_world = { center_x, center_y };
          Math::Vec2D scale = { grid_snap, grid_snap };

          // Get the custom vertices from the atlas entry's tile_collider_configs
          std::vector<Math::Vec2D> custom_vertices;
          if (tile.atlas_index >= 0 && tile.atlas_index < static_cast<int>(project_settings.loaded_tilemaps.size())) {
            auto& atlas_entry = project_settings.loaded_tilemaps[tile.atlas_index];
            auto config_it = atlas_entry.tile_collider_configs.find(tile.tile_index);
            if (config_it != atlas_entry.tile_collider_configs.end()) {
              // Copy vertices and flip Y-axis (screen coords have Y increasing downward)
              custom_vertices.reserve(config_it->second.custom_vertices.size());
              for (const auto& v : config_it->second.custom_vertices) {
                custom_vertices.push_back(Math::Vec2D(v.x, -v.y));
              }
            }
          }

          // Create collision-only entity for this Custom tile
          auto entity = Engine::ECS::create_entity()
            .add_component<Components::TileCollider>()
            .add_component<Components::Transform2D>(center_world, 0.0f, scale)
            .add_component<Components::RigidBody>(true, false);

          // Add appropriate collider based on whether we have custom vertices
          if (!custom_vertices.empty()) {
            entity.add_component<Components::BoxCollider>(custom_vertices, true);
          }
          else {
            // Fallback to AABB if no custom vertices defined
            entity.add_component<Components::BoxCollider>(scale.x, scale.y, true);
          }

          entity.clear_layers();
          entity.add_to_layer(layer_name);
        }
      }
    }

    broadcast_world_bounds();
  }

  void place_tile(const Math::Vec2D& world_pos, const std::string& layer_name, int atlas_index, int tile_index) {
    int grid_x, grid_y;
    world_to_grid(world_pos, grid_x, grid_y);

    // Get the tilemap entity
    auto tilemap_entity = get_or_create_tilemap_entity(layer_name);
    auto* int_map = tilemap_entity.get_component<Components::TileMap>();
    if (!int_map) return;

    // Check if the tile is already the same
    Engine::Graphics::Tile current_tile = int_map->get_tile(grid_x, grid_y);
    if (current_tile.atlas_index == atlas_index && current_tile.tile_index == tile_index) {
      return; // Same tile, nothing to do
    }

    // Set the tile
    int_map->set_tile(grid_x, grid_y, atlas_index, tile_index);

    // Update tile size and grid offset
    int_map->tile_size = { grid_snap, grid_snap };
    int_map->grid_offset = grid_offset;

    // Rebuild the tilemap to create collision entities
    rebuild_tilemap_for_layer(layer_name);

    // Mark level as dirty
    Engine::ProjectSaveManager::set_dirty(
      Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path())
    );
  }

  void delete_tile(const Math::Vec2D& world_pos, const std::string& layer_name) {
    int grid_x, grid_y;
    world_to_grid(world_pos, grid_x, grid_y);

    // Find the tilemap entity on this layer
    Engine::ECS::Entity tilemap_entity;
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
      if (entity.is_on_layer(layer_name)) {
        tilemap_entity = entity;
        break;
      }
    }

    if (!tilemap_entity.is_valid()) {
      return;
    }

    auto* int_map = tilemap_entity.get_component<Components::TileMap>();
    if (!int_map) return;

    Engine::Graphics::Tile old_tile = int_map->get_tile(grid_x, grid_y);
    if (!old_tile.is_valid()) return; // Nothing to delete

    // Delete the tile
    int_map->set_tile(grid_x, grid_y, Engine::Graphics::Tile());

    // We need to pass loaded_tilemaps to rebuild, get it from project settings
    rebuild_tilemap_for_layer(layer_name);
    broadcast_world_bounds();

    // Mark level as dirty
    Engine::ProjectSaveManager::set_dirty(
      Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path())
    );
  }

  void update_all_tile_dimensions() {
    // Update tile size and grid offset in all tilemap entities and rebuild collision entities
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
      auto* int_map = entity.get_component<Components::TileMap>();
      if (int_map) {
        int_map->tile_size = { grid_snap, grid_snap };
        int_map->grid_offset = grid_offset;
      }

      // Rebuild for each layer this entity is on
      auto layers = entity.get_layers();
      for (Engine::ECS::LayerID layer_id = 0; layer_id < Engine::ECS::MAX_LAYERS; ++layer_id) {
        if (layers.test(layer_id)) {
          std::string layer_name = Engine::ECS::get_layer_name(layer_id);
          if (!layer_name.empty()) {
            rebuild_tilemap_for_layer(layer_name);
          }
        }
      }
    }
  }

  bool get_world_bounds(Math::Vec2D& min, Math::Vec2D& max) {
    min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
    bool found_any = false;

    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
      auto* tilemap = entity.get_component<Components::TileMap>();
      if (!tilemap || tilemap->is_empty()) continue;

      int grid_min_x, grid_min_y, grid_max_x, grid_max_y;
      tilemap->get_bounds(grid_min_x, grid_min_y, grid_max_x, grid_max_y);

      // Convert grid bounds to world bounds
      // min world pos corresponds to the bottom-left corner of the min grid cell
      // max world pos corresponds to the top-right corner of the max grid cell

      float min_world_x = grid_min_x * tilemap->tile_size.x + tilemap->grid_offset.x;
      float min_world_y = grid_min_y * tilemap->tile_size.y + tilemap->grid_offset.y;

      float max_world_x = (grid_max_x + 1) * tilemap->tile_size.x + tilemap->grid_offset.x;
      float max_world_y = (grid_max_y + 1) * tilemap->tile_size.y + tilemap->grid_offset.y;

      if (min_world_x < min.x) min.x = min_world_x;
      if (min_world_y < min.y) min.y = min_world_y;
      if (max_world_x > max.x) max.x = max_world_x;
      if (max_world_y > max.y) max.y = max_world_y;

      found_any = true;
    }

    return found_any;
  }

  void broadcast_world_bounds() {
    AABB bounds;
    Math::Vec2D min_bounds, max_bounds;
    if (get_world_bounds(min_bounds, max_bounds)) {
      f32 width = max_bounds.x - min_bounds.x;
      f32 height = max_bounds.y - min_bounds.y;
      Math::Vec2D mid_point(min_bounds.x + width * 0.5f, min_bounds.y + height * 0.5f);
      bounds = AABB(mid_point, width, height);
    }
    else {
      // Fallback to screen bounds
      using settings = Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>;
      Math::Vec2D win_size = Engine::Graphics::screen_to_virtual({ settings::get().window_width, settings::get().window_height });

      f32 win_width = win_size.x;
      f32 win_height = win_size.y;
      Math::Vec2D mid_point(win_width / 2, win_height / 2);
      bounds = AABB(mid_point, win_width, win_height);
    }

    Engine::Systems::Collision2DSystem::set_world_bounds(bounds);
  }

} // namespace TilemapManager
