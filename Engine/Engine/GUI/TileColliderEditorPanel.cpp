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
#include <imgui.h>
#include "widgets/TextureAtlasWidget.h"
#include "../FileIO/Settings.h"
#include "../Resources.h"
#include "../Graphics/RendererInternal.h"
#include "../Components/CollisionComponents.h"
#include "../ProjectSaveManager.h"
#include "../TilemapManager.h"
#include "../Math/Vector2D.h"
#include <vector>
#include <algorithm>

namespace {
  bool is_open = false;
  int current_atlas_index = -1;
  int selected_tile_index = -1;
  ImGuiWidgets::TextureAtlasState atlas_widget_state;

  // Helper to draw a handle and return true if dragged
  bool draw_handle(ImDrawList* draw_list, ImVec2 pos, float size, ImU32 color, int id, ImVec2& out_delta) {
    ImVec2 handle_min(pos.x - size, pos.y - size);
    ImVec2 handle_max(pos.x + size, pos.y + size);

    ImGui::SetCursorScreenPos(handle_min);
    ImGui::InvisibleButton(("##handle" + std::to_string(id)).c_str(), ImVec2(size * 2, size * 2));

    bool is_hovered = ImGui::IsItemHovered();
    bool is_active = ImGui::IsItemActive();

    if (is_active && ImGui::IsMouseDragging(0)) {
      out_delta = ImGui::GetIO().MouseDelta;
      draw_list->AddRectFilled(handle_min, handle_max, IM_COL32(255, 255, 0, 255)); // Yellow when dragging
      return true;
    }
    else if (is_hovered) {
      draw_list->AddRectFilled(handle_min, handle_max, IM_COL32(255, 200, 0, 255)); // Orange when hovered
    }
    else {
      draw_list->AddRectFilled(handle_min, handle_max, color);
    }

    return false;
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

  // Note: We're editing a template/configuration, not actual placed tiles
  // When tiles are placed, they'll copy this configuration

  // For simplicity, we'll store this in a static map for now
  // In a full implementation, this should be stored in AtlasEntry or project settings
  static std::unordered_map<int, std::unordered_map<int, Engine::Graphics::Tile>> tile_configs;
  Engine::Graphics::Tile& tile_config = tile_configs[current_atlas_index][selected_tile_index];

  // Initialize if first time
  if (tile_config.atlas_index == -1) {
    tile_config.atlas_index = current_atlas_index;
    tile_config.tile_index = selected_tile_index;
    tile_config.collider_type = Engine::Graphics::TileColliderType::Default;
  }

  // Collider Type Selection
  ImGui::Text("Collider Type");
  int current_type = (tile_config.collider_type == Engine::Graphics::TileColliderType::Default) ? 0 : 1;
  const char* type_names[] = { "Default (AABB - Merged)", "Custom (Polygon - Individual)" };

  if (ImGui::Combo("##ColliderType", &current_type, type_names, IM_ARRAYSIZE(type_names))) {
    tile_config.collider_type = (current_type == 0) ?
      Engine::Graphics::TileColliderType::Default :
      Engine::Graphics::TileColliderType::Custom;

    // Initialize custom vertices if switching to Custom for the first time
    if (tile_config.collider_type == Engine::Graphics::TileColliderType::Custom && tile_config.custom_vertices.empty()) {
      // Create default box shape (64x64 default tile size)
      float hw = 32.0f;
      float hh = 32.0f;
      tile_config.custom_vertices = {
        Math::Vec2D(-hw, -hh),
        Math::Vec2D(-hw,  hh),
        Math::Vec2D(hw,  hh),
        Math::Vec2D(hw, -hh)
      };
    }

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
  else {
    // Custom polygon editing
    ImGui::Text("Custom Polygon Editor");

    if (ImGui::Button("Reset to Box")) {
      float hw = 32.0f;
      float hh = 32.0f;
      tile_config.custom_vertices = {
        Math::Vec2D(-hw, -hh),
        Math::Vec2D(-hw,  hh),
        Math::Vec2D(hw,  hh),
        Math::Vec2D(hw, -hh)
      };
      Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Vertex")) {
      tile_config.custom_vertices.push_back(Math::Vec2D(0.0f, 0.0f));
      Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
    }

    // Vertex list with delete buttons
    ImGui::Text("Vertices (%zu):", tile_config.custom_vertices.size());
    int vertex_to_delete = -1;
    for (size_t i = 0; i < tile_config.custom_vertices.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));

      float v[2] = { tile_config.custom_vertices[i].x, tile_config.custom_vertices[i].y };
      if (ImGui::DragFloat2("##vertex", v, 0.5f)) {
        tile_config.custom_vertices[i] = Math::Vec2D(v[0], v[1]);
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
      }

      ImGui::SameLine();
      if (ImGui::Button("X") && tile_config.custom_vertices.size() > 3) {
        vertex_to_delete = static_cast<int>(i);
      }

      ImGui::PopID();
    }

    if (vertex_to_delete >= 0) {
      tile_config.custom_vertices.erase(tile_config.custom_vertices.begin() + vertex_to_delete);
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

      ImVec2 uv0(min_u, min_v);
      ImVec2 uv1(max_u, max_v);

      // Calculate sprite world dimensions (in virtual pixels)
      // The tile size in pixels
      float tex_width_pixels = (max_u - min_u) * atlas_entry.texture_handle->width;
      float tex_height_pixels = (max_v - min_v) * atlas_entry.texture_handle->height;

      // Sprite world dimensions - for tiles, we use the default tile grid size
      // This represents the actual world-space size of the tile
      float sprite_world_width = 64.0f;  // Default tile size in world units
      float sprite_world_height = 64.0f;

      // If we can get the actual tile size from TilemapManager, use that
      sprite_world_width = TilemapManager::get_grid_size();
      sprite_world_height = TilemapManager::get_grid_size();

      // Calculate scale to fit sprite in window while maintaining aspect ratio
      ImVec2 avail_size = ImGui::GetContentRegionAvail();
      if (avail_size.x < 100 || avail_size.y < 100) avail_size = { 400, 400 };

      if (sprite_world_width <= 0.001f) sprite_world_width = 64.0f;
      if (sprite_world_height <= 0.001f) sprite_world_height = 64.0f;

      float scale_x = avail_size.x / sprite_world_width;
      float scale_y = avail_size.y / sprite_world_height;
      float scale = std::min(scale_x, scale_y) * 0.8f; // 0.8 to leave some padding

      float draw_width = sprite_world_width * scale;
      float draw_height = sprite_world_height * scale;

      ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
      float offset_x = (avail_size.x - draw_width) * 0.5f;
      float offset_y = (avail_size.y - draw_height) * 0.5f;

      ImVec2 image_min = { cursor_pos.x + offset_x, cursor_pos.y + offset_y };
      ImVec2 image_max = { image_min.x + draw_width, image_min.y + draw_height };

      // Draw the tile sprite
      ImGui::SetCursorScreenPos(image_min);
      ImGui::Image((ImTextureID)(intptr_t)atlas_entry.texture_handle->id,
        ImVec2(draw_width, draw_height), uv0, uv1);

      // Draw collider overlay
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      ImVec2 center = { image_min.x + draw_width * 0.5f, image_min.y + draw_height * 0.5f };

      // Draw polygon edges
      if (tile_config.custom_vertices.size() >= 3) {
        for (size_t i = 0; i < tile_config.custom_vertices.size(); ++i) {
          size_t next_i = (i + 1) % tile_config.custom_vertices.size();

          Math::Vec2D v1 = tile_config.custom_vertices[i];
          Math::Vec2D v2 = tile_config.custom_vertices[next_i];

          ImVec2 p1(center.x + v1.x * scale, center.y - v1.y * scale); // Flip Y for screen space
          ImVec2 p2(center.x + v2.x * scale, center.y - v2.y * scale);

          draw_list->AddLine(p1, p2, IM_COL32(0, 255, 0, 255), 2.0f);
        }

        // Draw vertex handles
        for (size_t i = 0; i < tile_config.custom_vertices.size(); ++i) {
          Math::Vec2D& vertex = tile_config.custom_vertices[i];
          ImVec2 vertex_pos(center.x + vertex.x * scale, center.y - vertex.y * scale);

          ImVec2 delta;
          if (draw_handle(draw_list, vertex_pos, 6.0f, IM_COL32(0, 255, 0, 255), static_cast<int>(i), delta)) {
            vertex.x += delta.x / scale;
            vertex.y -= delta.y / scale; // Flip Y
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
          }
        }
      }
    }
  }

  ImGui::Columns(1);
  ImGui::End();
}
