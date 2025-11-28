//-----------------------------------------------------------------------------------
// file:    TileColliderEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of TileColliderEditorPanel for editing per-tile collider
//          configurations. Displays texture atlas for selection and allows editing
//          collider type (Default/Custom) and custom polygon vertices.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "TileColliderEditorPanel.h"
#include "../Precompiled.h"
#include "widgets/TextureAtlasWidget.h"
#include "widgets/ColliderEditorWidget.h"
#include "../FileIO/Settings.h"
#include "../Resources.h"
#include "../Graphics/RendererInternal.h"
#include "../Components/CollisionComponents.h"
#include "../Components/TilemapComponents.h"
#include "../ProjectSaveManager.h"
#include "../TilemapManager.h"
#include "../ECS/ECS.h"
#include "../Math/Vector2D.h"
#include <vector>
#include <algorithm>

namespace {
  bool is_open = false;
  int current_atlas_index = -1;
  int selected_tile_index = -1;
  ImGuiWidgets::TextureAtlasState atlas_widget_state;

  // Helper function to rebuild collision entities for all tiles that use this atlas/tile combination
  void update_all_tile_instances(int atlas_idx, int tile_idx, const Engine::Graphics::TileColliderConfig& config) {
    // Find all TileMap entities and rebuild layers that contain tiles matching this atlas/tile index
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
      auto* tilemap = entity.get_component<Components::TileMap>();
      if (!tilemap) continue;

      bool has_matching_tiles = false;

      // Check if this tilemap contains any tiles with the specified atlas/tile index
      for (const auto& [key, tile] : tilemap->tiles) {
        if (tile.atlas_index == atlas_idx && tile.tile_index == tile_idx) {
          has_matching_tiles = true;
          break;
        }
      }

      // If any matching tiles were found, rebuild the collision for this layer
      if (has_matching_tiles) {
        // Get all layers this entity is on and rebuild each one
        auto layers = entity.get_layers();
        for (Engine::ECS::LayerID layer_id = 0; layer_id < Engine::ECS::MAX_LAYERS; ++layer_id) {
          if (layers.test(layer_id)) {
            std::string layer_name = Engine::ECS::get_layer_name(layer_id);
            if (!layer_name.empty()) {
              TilemapManager::rebuild_tilemap_for_layer(layer_name);
            }
          }
        }
      }
    }
  }
}

namespace TileColliderEditor {
  void Open(int atlas_index) {
    is_open = true;
    current_atlas_index = atlas_index;
    selected_tile_index = -1;
    atlas_widget_state.reset();
  }
}

void TileColliderEditorPanel() {
  if (!is_open) return;

  ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Tile Collider Editor", &is_open)) {
    ImGui::End();
    return;
  }

  auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get_mutable();

  if (current_atlas_index < 0 || current_atlas_index >= static_cast<int>(project_settings.loaded_tilemaps.size())) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid atlas index!");
    ImGui::End();
    return;
  }

  Engine::Graphics::AtlasEntry& atlas_entry = project_settings.loaded_tilemaps[current_atlas_index];

  // Check if we have a valid texture
  if (!atlas_entry.texture_handle || atlas_entry.texture_handle->id == 0) {
    // Try to load the texture
    if (!atlas_entry.texture_path.empty()) {
      atlas_entry.texture_handle = Engine::Resources::load_texture(atlas_entry.texture_path);
    }
  }

  if (!atlas_entry.texture_handle || atlas_entry.texture_handle->id == 0) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to load texture!");
    ImGui::End();
    return;
  }

  // Create temporary component wrappers for TextureAtlasWidget
  Components::Texture temp_texture;
  temp_texture.texture_handle = atlas_entry.texture_handle;

  Components::TextureAtlas temp_atlas;
  temp_atlas.subTextures.reserve(atlas_entry.subTextures.size());
  for (const auto& sub : atlas_entry.subTextures) {
    temp_atlas.subTextures.push_back(sub);
  }

  // Split into two columns
  ImGui::Columns(2, "TileColliderColumns", true);
  ImGui::SetColumnWidth(0, 400);

  // --- LEFT COLUMN: Atlas Preview ---
  ImGui::Text("Select Tile");
  ImGui::Separator();

  ImGuiWidgets::TextureAtlasPreview(temp_texture, temp_atlas, 300.0f, atlas_widget_state);

  // Update selected tile from widget state
  if (!atlas_widget_state.current_selection.is_empty() && atlas_widget_state.current_selection.is_single()) {
    selected_tile_index = atlas_widget_state.current_selection.indices[0];
  }

  ImGui::NextColumn();

  // --- RIGHT COLUMN: Collider Editor ---
  ImGui::Text("Collider Configuration");
  ImGui::Separator();

  if (selected_tile_index < 0) {
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Select a tile from the atlas to edit its collider");
    ImGui::Columns(1);
    ImGui::End();
    return;
  }

  ImGui::Text("Editing Tile: %d", selected_tile_index);
  ImGui::Separator();

  // Get or create the tile collider configuration from the atlas entry
  Engine::Graphics::TileColliderConfig& tile_config = atlas_entry.tile_collider_configs[selected_tile_index];

  // Collider Type Selection
  ImGui::Text("Collider Type");
  int current_type = 0;
  if (tile_config.collider_type == Engine::Graphics::TileColliderType::Default) current_type = 0;
  else if (tile_config.collider_type == Engine::Graphics::TileColliderType::AABB) current_type = 1;
  else if (tile_config.collider_type == Engine::Graphics::TileColliderType::Polygon) current_type = 2;

  const char* type_names[] = { "Default (Merged)", "AABB (Custom Box)", "Polygon (Custom Shape)" };

  if (ImGui::Combo("##ColliderType", &current_type, type_names, IM_ARRAYSIZE(type_names))) {
    if (current_type == 0) {
      tile_config.collider_type = Engine::Graphics::TileColliderType::Default;
    }
    else if (current_type == 1) {
      tile_config.collider_type = Engine::Graphics::TileColliderType::AABB;
      // Initialize AABB if not set
      if (tile_config.aabb_width == 0.0f && tile_config.aabb_height == 0.0f) {
        tile_config.aabb_width = TilemapManager::get_grid_size();
        tile_config.aabb_height = TilemapManager::get_grid_size();
        tile_config.aabb_offset = { 0.0f, 0.0f };
      }
    }
    else if (current_type == 2) {
      tile_config.collider_type = Engine::Graphics::TileColliderType::Polygon;
      // Initialize custom vertices if switching to Polygon for the first time
      if (tile_config.custom_vertices.empty()) {
        float hw = TilemapManager::get_grid_size() / 2.0f;
        float hh = TilemapManager::get_grid_size() / 2.0f;
        tile_config.custom_vertices = {
          Math::Vec2D(-hw, -hh),
          Math::Vec2D(hw, -hh),
          Math::Vec2D(hw, hh),
          Math::Vec2D(-hw, hh)
        };
      }
    }

    // Update all existing tile instances in the world
    update_all_tile_instances(current_atlas_index, selected_tile_index, tile_config);

    Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
  }

  ImGui::Separator();

  // Show different UI based on collider type
  if (tile_config.collider_type == Engine::Graphics::TileColliderType::Default) {
    // Default AABB editing
    ImGui::Text("Default AABB (will be merged with adjacent tiles)");
    ImGui::TextWrapped("Default tiles are automatically merged into rectangles for optimized collision.");

    ImGui::Text("Tile size is determined by the tilemap grid settings.");
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Default tiles use automatic sizing)");
  }
  else if (tile_config.collider_type == Engine::Graphics::TileColliderType::AABB) {
    // Custom AABB editing with visual editor
    ImGui::Text("Custom AABB Editor");

    if (ImGui::Button("Reset to Tile Size")) {
      tile_config.aabb_width = TilemapManager::get_grid_size();
      tile_config.aabb_height = TilemapManager::get_grid_size();
      tile_config.aabb_offset = { 0.0f, 0.0f };
      update_all_tile_instances(current_atlas_index, selected_tile_index, tile_config);
      Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
    }

    ImGui::Separator();

    // Visual editor
    ImGui::Text("Visual Editor");

    // Get the subtexture for the selected tile
    if (selected_tile_index < static_cast<int>(atlas_entry.subTextures.size())) {
      const auto& sub_tex = atlas_entry.subTextures[selected_tile_index];

      // Calculate UV bounds
      const auto& coords = sub_tex.tex_coords;
      float min_u = coords[0].x, max_u = coords[0].x;
      float min_v = coords[0].y, max_v = coords[0].y;
      for (size_t j = 1; j < 4; ++j) {
        min_u = std::min(min_u, coords[j].x);
        max_u = std::max(max_u, coords[j].x);
        min_v = std::min(min_v, coords[j].y);
        max_v = std::max(max_v, coords[j].y);
      }

      // Setup sprite rendering configuration
      ImGuiWidgets::SpriteRenderConfig sprite_config;
      sprite_config.texture_handle = atlas_entry.texture_handle;
      sprite_config.uv0 = ImVec2(min_u, min_v);
      sprite_config.uv1 = ImVec2(max_u, max_v);
      sprite_config.sprite_world_width = TilemapManager::get_grid_size();
      sprite_config.sprite_world_height = TilemapManager::get_grid_size();

      // Calculate visual editor layout
      ImGuiWidgets::VisualEditorLayout layout = ImGuiWidgets::CalculateVisualEditorLayout(sprite_config);

      // Draw the tile sprite
      ImGuiWidgets::DrawSprite(sprite_config, layout);

      // Draw collider overlay
      ImDrawList* draw_list = ImGui::GetWindowDrawList();

      ImGuiWidgets::AABBColliderData aabb_data;
      aabb_data.width = tile_config.aabb_width;
      aabb_data.height = tile_config.aabb_height;
      aabb_data.offset = tile_config.aabb_offset;

      bool changed = ImGuiWidgets::AABBColliderEditor(
        draw_list,
        layout.center,
        layout.scale,
        aabb_data,
        false, // No flip for tiles
        1.0f,
        1.0f
      );

      if (changed) {
        tile_config.aabb_width = aabb_data.width;
        tile_config.aabb_height = aabb_data.height;
        tile_config.aabb_offset = aabb_data.offset;
        update_all_tile_instances(current_atlas_index, selected_tile_index, tile_config);
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
      }

      // Display values below the editor
      ImGui::SetCursorScreenPos({ layout.image_min.x, layout.image_max.y + 10 });
      ImGui::Text("Size: %.2f x %.2f | Offset: %.2f, %.2f",
        tile_config.aabb_width, tile_config.aabb_height,
        tile_config.aabb_offset.x, tile_config.aabb_offset.y);
    }
  }
  else if (tile_config.collider_type == Engine::Graphics::TileColliderType::Polygon) {
    // Custom polygon editing
    ImGui::Text("Custom Polygon Editor");

    if (ImGui::Button("Reset to Box")) {
      float hw = TilemapManager::get_grid_size() / 2.0f;
      float hh = TilemapManager::get_grid_size() / 2.0f;
      tile_config.custom_vertices = {
        Math::Vec2D(-hw, -hh),
        Math::Vec2D(hw, -hh),
        Math::Vec2D(hw, hh),
        Math::Vec2D(-hw, hh)
      };
      update_all_tile_instances(current_atlas_index, selected_tile_index, tile_config);
      Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Vertex")) {
      tile_config.custom_vertices.push_back(Math::Vec2D(0.0f, 0.0f));
      update_all_tile_instances(current_atlas_index, selected_tile_index, tile_config);
      Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
    }

    ImGui::Separator();

    // Visual editor
    ImGui::Text("Visual Editor");

    // Get the subtexture for the selected tile
    if (selected_tile_index < static_cast<int>(atlas_entry.subTextures.size())) {
      const auto& sub_tex = atlas_entry.subTextures[selected_tile_index];

      // Calculate UV bounds
      const auto& coords = sub_tex.tex_coords;
      float min_u = coords[0].x, max_u = coords[0].x;
      float min_v = coords[0].y, max_v = coords[0].y;
      for (size_t j = 1; j < 4; ++j) {
        min_u = std::min(min_u, coords[j].x);
        max_u = std::max(max_u, coords[j].x);
        min_v = std::min(min_v, coords[j].y);
        max_v = std::max(max_v, coords[j].y);
      }

      // Setup sprite rendering configuration
      ImGuiWidgets::SpriteRenderConfig sprite_config;
      sprite_config.texture_handle = atlas_entry.texture_handle;
      sprite_config.uv0 = ImVec2(min_u, min_v);
      sprite_config.uv1 = ImVec2(max_u, max_v);
      sprite_config.sprite_world_width = TilemapManager::get_grid_size();
      sprite_config.sprite_world_height = TilemapManager::get_grid_size();

      // Calculate visual editor layout
      ImGuiWidgets::VisualEditorLayout layout = ImGuiWidgets::CalculateVisualEditorLayout(sprite_config);

      // Draw the tile sprite
      ImGuiWidgets::DrawSprite(sprite_config, layout);

      // Draw collider overlay
      ImDrawList* draw_list = ImGui::GetWindowDrawList();

      ImGuiWidgets::PolygonColliderData poly_data;
      poly_data.vertices = tile_config.custom_vertices;

      bool changed = ImGuiWidgets::PolygonColliderEditor(
        draw_list,
        layout.center,
        layout.scale,
        poly_data,
        Math::Vec2D(0.0f, 0.0f), // No offset for tiles
        layout.image_min,
        layout.image_max
      );

      if (changed) {
        tile_config.custom_vertices = poly_data.vertices;
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
      }
    }
  }

  ImGui::Columns(1);
  ImGui::End();
}
