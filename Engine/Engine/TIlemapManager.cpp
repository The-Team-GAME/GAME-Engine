//-----------------------------------------------------------------------------------
// file:    TIlemapManager.cpp
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
#include "LevelManager.h"
#include <algorithm>
#include <limits>
#include "FileIO/Settings.h"
#include "Systems/CollisionSystem.h"

// Define this macro to force ALL tiles to use the Default (greedy-merged) collider,
// disabling custom AABB and Polygon colliders. Useful while per-tile polygon colliders
// are under development.
#define TILE_COLLIDER_FORCE_DEFAULT

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

    // Create new tilemap entity with default Parallax (scroll_factor = 1.0 = normal world movement)
    auto entity = Engine::ECS::create_entity()
      .add_component<Components::TileMap>()
      .add_component<Components::Parallax>(1.0f);

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

    // Get project settings to access AtlasEntry.tile_collider_configs
    auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get_mutable();

    // Helper lambda to get collider type for a tile by looking up in atlas entry
    auto get_tile_collider_type = [&](const Engine::Graphics::Tile& tile) -> Engine::Graphics::TileColliderType {
      if (!tile.is_valid()) return Engine::Graphics::TileColliderType::Default;

      if (tile.atlas_index >= 0 && tile.atlas_index < static_cast<int>(project_settings.loaded_tilemaps.size())) {
        auto& atlas_entry = project_settings.loaded_tilemaps[tile.atlas_index];
        auto config_it = atlas_entry.tile_collider_configs.find(tile.tile_index);
        if (config_it != atlas_entry.tile_collider_configs.end()) {
          return config_it->second.collider_type;
        }
      }

      return Engine::Graphics::TileColliderType::Default; // Default if no config found
      };

    // Helper struct for rectangle merging
    struct Rectangle {
      int x, y, width, height;
      int atlas_index;
    };

    // Track processed tiles using relative coordinates
    std::vector<std::vector<bool>> processed(height, std::vector<bool>(width, false));
    std::vector<Rectangle> rectangles;

    // Find all rectangles using COLUMN-FIRST greedy algorithm (merge only tiles with Default collider type)
    // This prioritizes vertical merging to create tall columns, which prevents seam issues when
    // the player slides along walls. For example, a layout like:
    //   1234
    //   5
    //   6
    // Will create a 1x3 column (tiles 1,5,6) and a 3x1 row (tiles 2,3,4), ensuring the player
    // doesn't get stuck at the seam between differently-sized horizontal rectangles.
    for (int rel_x = 0; rel_x < width; ++rel_x) {
      for (int rel_y = 0; rel_y < height; ++rel_y) {
        if (processed[rel_y][rel_x]) continue;

        int abs_x = min_x + rel_x;
        int abs_y = min_y + rel_y;
        Engine::Graphics::Tile tile = int_map->get_tile(abs_x, abs_y);

        if (!tile.is_valid()) {
          processed[rel_y][rel_x] = true;
          continue;
        }

        // Look up collider type from atlas entry
        Engine::Graphics::TileColliderType collider_type = get_tile_collider_type(tile);

        // Skip tiles with AABB or Polygon collider type - they will be handled separately
        if (collider_type == Engine::Graphics::TileColliderType::AABB ||
          collider_type == Engine::Graphics::TileColliderType::Polygon) {
          processed[rel_y][rel_x] = true;
          continue;
        }

        // First, expand VERTICALLY (downward) to find maximum column height
        // Only merge tiles from the same atlas so friction is consistent per collider
        int max_height = 0;
        for (int h = 0; rel_y + h < height; ++h) {
          if (processed[rel_y + h][rel_x]) break;

          Engine::Graphics::Tile check_tile = int_map->get_tile(abs_x, abs_y + h);
          if (!check_tile.is_valid()) break;
          if (get_tile_collider_type(check_tile) != Engine::Graphics::TileColliderType::Default) break;
          if (check_tile.atlas_index != tile.atlas_index) break;

          max_height++;
        }

        if (max_height == 0) {
          processed[rel_y][rel_x] = true;
          continue;
        }

        // Then, expand HORIZONTALLY (rightward) while maintaining the column height
        int rect_width = 1;
        for (int w = 1; rel_x + w < width; ++w) {
          bool can_expand = true;
          for (int h = 0; h < max_height; ++h) {
            if (processed[rel_y + h][rel_x + w]) {
              can_expand = false;
              break;
            }

            Engine::Graphics::Tile check_tile = int_map->get_tile(abs_x + w, abs_y + h);
            if (!check_tile.is_valid() || get_tile_collider_type(check_tile) != Engine::Graphics::TileColliderType::Default ||
              check_tile.atlas_index != tile.atlas_index) {
              can_expand = false;
              break;
            }
          }
          if (!can_expand) break;
          rect_width++;
        }

        // Mark all tiles in this rectangle as processed
        for (int h = 0; h < max_height; ++h) {
          for (int w = 0; w < rect_width; ++w) {
            processed[rel_y + h][rel_x + w] = true;
          }
        }

        // Store rectangle with absolute coordinates and originating atlas
        rectangles.push_back({ abs_x, abs_y, rect_width, max_height, tile.atlas_index });
      }
    }

    // Create collision-only entities for each rectangle (no rendering components)
    for (const Rectangle& rect : rectangles) {
      // Calculate the world position for the center of the rectangle
      float center_x = grid_offset.x + rect.x * grid_snap + (rect.width * grid_snap) * 0.5f;
      float center_y = grid_offset.y + rect.y * grid_snap + (rect.height * grid_snap) * 0.5f;
      Math::Vec2D center_world = { center_x, center_y };
      Math::Vec2D scale = { rect.width * grid_snap, rect.height * grid_snap };

      // Look up friction from the atlas entry
      float tile_friction = 1.0f;
      if (rect.atlas_index >= 0 && rect.atlas_index < static_cast<int>(project_settings.loaded_tilemaps.size())) {
        tile_friction = project_settings.loaded_tilemaps[rect.atlas_index].friction;
      }

      // Create collision-only entity (no rendering components)
      auto entity = Engine::ECS::create_entity()
        .add_component<Components::TileCollider>() // Marker component to identify tile collision entities
        .add_component<Components::Internal>()     // Hide from entity editor
        .add_component<Components::Transform2D>(center_world, 0.0f, scale)
        .add_component<Components::RigidBody>(true, false) // is_static = true, no gravity
        .add_component<Components::BoxCollider>(scale.x, scale.y, true); // resolve_collisions = true

      entity.get_component<Components::BoxCollider>()->kinetic_friction = tile_friction;

      entity.clear_layers();
      entity.add_to_layer(layer_name);
    }

    // Create individual collision entities for AABB and Polygon collider type tiles
    // (disabled when TILE_COLLIDER_FORCE_DEFAULT is set — non-Default tiles get no collider)
#ifndef TILE_COLLIDER_FORCE_DEFAULT
    for (int rel_y = 0; rel_y < height; ++rel_y) {
      for (int rel_x = 0; rel_x < width; ++rel_x) {
        int abs_x = min_x + rel_x;
        int abs_y = min_y + rel_y;
        Engine::Graphics::Tile tile = int_map->get_tile(abs_x, abs_y);

        if (!tile.is_valid()) continue;

        // Look up collision configuration from atlas entry
        Engine::Graphics::TileColliderType collider_type = get_tile_collider_type(tile);

        if (collider_type == Engine::Graphics::TileColliderType::AABB ||
          collider_type == Engine::Graphics::TileColliderType::Polygon) {

          // Calculate world position for this individual tile
          float center_x = grid_offset.x + abs_x * grid_snap + grid_snap * 0.5f;
          float center_y = grid_offset.y + abs_y * grid_snap + grid_snap * 0.5f;
          Math::Vec2D center_world = { center_x, center_y };
          Math::Vec2D scale = { grid_snap, grid_snap };

          if (collider_type == Engine::Graphics::TileColliderType::AABB) {
            // Look up AABB properties from atlas entry
            float box_width = grid_snap;
            float box_height = grid_snap;
            Math::Vec2D box_offset = { 0.0f, 0.0f };

            if (tile.atlas_index >= 0 && tile.atlas_index < static_cast<int>(project_settings.loaded_tilemaps.size())) {
              auto& atlas_entry = project_settings.loaded_tilemaps[tile.atlas_index];
              auto config_it = atlas_entry.tile_collider_configs.find(tile.tile_index);
              if (config_it != atlas_entry.tile_collider_configs.end()) {
                box_width = config_it->second.aabb_width;
                box_height = config_it->second.aabb_height;
                box_offset = config_it->second.aabb_offset;
              }
            }

            // Apply offset to center position
            Math::Vec2D final_center = center_world + box_offset;

            // Look up friction from the atlas entry
            float aabb_friction = 1.0f;
            if (tile.atlas_index >= 0 && tile.atlas_index < static_cast<int>(project_settings.loaded_tilemaps.size())) {
              aabb_friction = project_settings.loaded_tilemaps[tile.atlas_index].friction;
            }

            // Create AABB collision entity
            auto entity = Engine::ECS::create_entity()
              .add_component<Components::TileCollider>()
              .add_component<Components::Internal>() // Hide from entity editor
              .add_component<Components::Transform2D>(final_center, 0.0f, scale)
              .add_component<Components::RigidBody>(true, false)
              .add_component<Components::BoxCollider>(box_width, box_height, true);

            entity.get_component<Components::BoxCollider>()->kinetic_friction = aabb_friction;

            entity.clear_layers();
            entity.add_to_layer(layer_name);
          }
          else if (collider_type == Engine::Graphics::TileColliderType::Polygon) {
            // Look up custom vertices from the atlas entry's tile_collider_configs
            std::vector<Math::Vec2D> custom_vertices;
            if (tile.atlas_index >= 0 && tile.atlas_index < static_cast<int>(project_settings.loaded_tilemaps.size())) {
              auto& atlas_entry = project_settings.loaded_tilemaps[tile.atlas_index];
              auto config_it = atlas_entry.tile_collider_configs.find(tile.tile_index);
              if (config_it != atlas_entry.tile_collider_configs.end()) {
                custom_vertices = config_it->second.custom_vertices;
              }
            }

            if (!custom_vertices.empty()) {
              // Create Polygon collision entity
              auto entity = Engine::ECS::create_entity()
                .add_component<Components::TileCollider>()
                .add_component<Components::Internal>() // Hide from entity editor
                .add_component<Components::Transform2D>(center_world, 0.0f, scale)
                .add_component<Components::RigidBody>(true, false)
                .add_component<Components::BoxCollider>(custom_vertices, true);

              entity.clear_layers();
              entity.add_to_layer(layer_name);
            }
          }
        }
      }
    }
#endif // TILE_COLLIDER_FORCE_DEFAULT

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

  void rebuild_all_layers() {
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
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

  void move_all_tiles_to_layer(const std::string& source_layer, const std::string& dest_layer) {
    // Find the source tilemap entity
    Engine::ECS::Entity source_entity;
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
      if (entity.is_on_layer(source_layer)) {
        source_entity = entity;
        break;
      }
    }

    // If no source tilemap exists, nothing to move
    if (!source_entity.is_valid()) {
      return;
    }

    auto* source_map = source_entity.get_component<Components::TileMap>();
    if (!source_map || source_map->is_empty()) {
      return;
    }

    // Get or create the destination tilemap entity
    auto dest_entity = get_or_create_tilemap_entity(dest_layer);
    auto* dest_map = dest_entity.get_component<Components::TileMap>();
    if (!dest_map) {
      return;
    }

    // Copy all tiles from source to destination
    for (const auto& [key, tile] : source_map->tiles) {
      if (tile.is_valid()) {
        dest_map->tiles[key] = tile;
      }
    }

    // Clear the source tilemap
    source_map->clear();

    // Rebuild both layers to update collision entities
    rebuild_tilemap_for_layer(source_layer);
    rebuild_tilemap_for_layer(dest_layer);

    // Mark level as dirty
    Engine::ProjectSaveManager::set_dirty(
      Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path())
    );
  }

} // namespace TilemapManager

