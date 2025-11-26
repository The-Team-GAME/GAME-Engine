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

  // Helper function to update all existing tile instances in the world with new collider config
  void update_all_tile_instances(int atlas_idx, int tile_idx, const Engine::Graphics::Tile& config) {
    // Find all TileMap entities and update tiles that match this atlas/tile index
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
      auto* tilemap = entity.get_component<Components::TileMap>();
      if (!tilemap) continue;

      bool any_updated = false;

      // Iterate through all tiles in this tilemap
      for (auto& [key, tile] : tilemap->tiles) {
        if (tile.atlas_index == atlas_idx && tile.tile_index == tile_idx) {
          // Update the collider configuration for this tile
          tile.collider_type = config.collider_type;
          tile.custom_vertices = config.custom_vertices;
          any_updated = true;
        }
      }

      // If any tiles were updated, rebuild the collision for this layer
      if (any_updated) {
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

  // Helper to draw a handle and return true if dragged
  bool draw_handle(ImDrawList* draw_list, ImVec2 pos, float size, ImU32 color, int id, ImVec2& out_delta) {
    ImVec2 min = { pos.x - size / 2, pos.y - size / 2 };
    ImVec2 max = { pos.x + size / 2, pos.y + size / 2 };

    draw_list->AddRectFilled(min, max, color);
    draw_list->AddRect(min, max, IM_COL32(0, 0, 0, 255));

    ImGui::SetCursorScreenPos(min);
    ImGui::PushID(id);
    ImGui::InvisibleButton("##handle", { size, size });
    ImGui::PopID();

    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      ImVec2 delta = ImGui::GetIO().MouseDelta;
      out_delta = delta;
      return true;
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

  // Get or create the tile collider configuration from the atlas entry
  Engine::Graphics::Tile& tile_config = atlas_entry.tile_collider_configs[selected_tile_index];

  // Initialize if first time
  if (tile_config.atlas_index == -1) {
    tile_config.atlas_index = current_atlas_index;
    tile_config.tile_index = selected_tile_index;
    tile_config.collider_type = Engine::Graphics::TileColliderType::Default;
  }

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

      ImVec2 uv0(min_u, min_v);
      ImVec2 uv1(max_u, max_v);

      // Sprite world dimensions
      float sprite_world_width = TilemapManager::get_grid_size();
      float sprite_world_height = TilemapManager::get_grid_size();

      // Calculate scale to fit sprite in window while maintaining aspect ratio
      ImVec2 avail_size = ImGui::GetContentRegionAvail();
      if (avail_size.x < 100 || avail_size.y < 100) avail_size = { 400, 400 };

      if (sprite_world_width <= 0.001f) sprite_world_width = 64.0f;
      if (sprite_world_height <= 0.001f) sprite_world_height = 64.0f;

      float scale_x = avail_size.x / sprite_world_width;
      float scale_y = avail_size.y / sprite_world_height;
      float scale = std::min(scale_x, scale_y) * 0.8f;

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

      float handle_size = 8.0f;
      ImU32 handle_color = IM_COL32(0, 255, 0, 255);
      ImVec2 delta;
      bool changed = false;

      // Collider Center (Sprite Center + Offset)
      ImVec2 col_center = { center.x + tile_config.aabb_offset.x * scale, center.y + tile_config.aabb_offset.y * scale };

      // Collider half-extents scaled to screen
      float col_half_w = (tile_config.aabb_width / 2.0f) * scale;
      float col_half_h = (tile_config.aabb_height / 2.0f) * scale;

      // Collider screen coords
      ImVec2 col_min = { col_center.x - col_half_w, col_center.y - col_half_h };
      ImVec2 col_max = { col_center.x + col_half_w, col_center.y + col_half_h };

      // Draw Rect
      draw_list->AddRect(col_min, col_max, IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);

      // --- Body Drag (Offset) ---
      ImGui::SetCursorScreenPos(col_min);
      ImGui::PushID("##collider_body_drag");
      ImGui::InvisibleButton("##body", { col_max.x - col_min.x, col_max.y - col_min.y });
      ImGui::PopID();

      if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
        tile_config.aabb_offset.x += mouse_delta.x / scale;
        tile_config.aabb_offset.y += mouse_delta.y / scale;
        changed = true;
      }

      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Drag to move collider");
      }

      // Edge handles
      bool shift_held = ImGui::GetIO().KeyShift;

      // Right handle
      if (draw_handle(draw_list, { col_max.x, col_center.y }, handle_size, handle_color, 0, delta)) {
        float dx = delta.x / scale;
        if (shift_held) {
          // Shift: scale symmetrically from center
          tile_config.aabb_width += dx * 2.0f;
        }
        else {
          // No shift: move right edge only
          tile_config.aabb_width += dx;
          tile_config.aabb_offset.x += dx / 2.0f;
        }
        changed = true;
      }

      // Left handle
      if (draw_handle(draw_list, { col_min.x, col_center.y }, handle_size, handle_color, 1, delta)) {
        float dx = delta.x / scale;
        if (shift_held) {
          // Shift: scale symmetrically from center
          tile_config.aabb_width -= dx * 2.0f;
        }
        else {
          // No shift: move left edge only
          tile_config.aabb_width -= dx;
          tile_config.aabb_offset.x += dx / 2.0f;
        }
        changed = true;
      }

      // Top handle
      if (draw_handle(draw_list, { col_center.x, col_min.y }, handle_size, handle_color, 2, delta)) {
        float dy = delta.y / scale;
        if (shift_held) {
          // Shift: scale symmetrically from center
          tile_config.aabb_height -= dy * 2.0f;
        }
        else {
          // No shift: move top edge only
          tile_config.aabb_height -= dy;
          tile_config.aabb_offset.y += dy / 2.0f;
        }
        changed = true;
      }

      // Bottom handle
      if (draw_handle(draw_list, { col_center.x, col_max.y }, handle_size, handle_color, 3, delta)) {
        float dy = delta.y / scale;
        if (shift_held) {
          // Shift: scale symmetrically from center
          tile_config.aabb_height += dy * 2.0f;
        }
        else {
          // No shift: move bottom edge only
          tile_config.aabb_height += dy;
          tile_config.aabb_offset.y -= dy / 2.0f;
        }
        changed = true;
      }

      if (changed) {
        update_all_tile_instances(current_atlas_index, selected_tile_index, tile_config);
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
      }

      // Display values below the editor
      ImGui::SetCursorScreenPos({ image_min.x, image_max.y + 10 });
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

      float handle_size = 8.0f;
      ImU32 handle_color = IM_COL32(0, 255, 0, 255);
      ImVec2 delta;
      bool changed = false;

      // Draw polygon edges
      if (tile_config.custom_vertices.size() >= 2) {
        for (size_t i = 0; i < tile_config.custom_vertices.size(); ++i) {
          size_t next_i = (i + 1) % tile_config.custom_vertices.size();

          Math::Vec2D v1 = tile_config.custom_vertices[i];
          Math::Vec2D v2 = tile_config.custom_vertices[next_i];

          ImVec2 p1(center.x + v1.x * scale, center.y + v1.y * scale);
          ImVec2 p2(center.x + v2.x * scale, center.y + v2.y * scale);

          draw_list->AddLine(p1, p2, IM_COL32(0, 255, 0, 255), 2.0f);
        }
      }

      // Draw and interact with vertices
      static int selected_vertex = -1;
      static int hovered_vertex = -1;
      hovered_vertex = -1;

      // Reset selection when not actively dragging
      if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        selected_vertex = -1;
      }

      for (size_t i = 0; i < tile_config.custom_vertices.size(); ++i) {
        Math::Vec2D& vertex = tile_config.custom_vertices[i];
        ImVec2 vertex_pos(center.x + vertex.x * scale, center.y + vertex.y * scale);

        if (draw_handle(draw_list, vertex_pos, handle_size, handle_color, static_cast<int>(i), delta)) {
          vertex.x += delta.x / scale;
          vertex.y += delta.y / scale;
          changed = true;
          selected_vertex = static_cast<int>(i);
        }

        // Check if hovering over this vertex
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float dx = mouse_pos.x - vertex_pos.x;
        float dy = mouse_pos.y - vertex_pos.y;
        if (dx * dx + dy * dy < (handle_size * handle_size)) {
          hovered_vertex = static_cast<int>(i);
        }
      }

      // Right-click to delete vertex
      if (hovered_vertex >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        if (tile_config.custom_vertices.size() > 3) {
          tile_config.custom_vertices.erase(tile_config.custom_vertices.begin() + hovered_vertex);
          changed = true;
        }
      }

      // Click on sprite (but not on a vertex) to add new vertex
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered_vertex == -1) {
        ImVec2 mouse_pos = ImGui::GetMousePos();

        // Check if click is within the image bounds
        if (mouse_pos.x >= image_min.x && mouse_pos.x <= image_max.x &&
          mouse_pos.y >= image_min.y && mouse_pos.y <= image_max.y) {

          // Convert screen position to local vertex coordinates
          float local_x = (mouse_pos.x - center.x) / scale;
          float local_y = (mouse_pos.y - center.y) / scale;          // Insert the new vertex at the best position in the polygon
          if (tile_config.custom_vertices.empty()) {
            tile_config.custom_vertices.push_back(Math::Vec2D(local_x, local_y));
          }
          else {
            // Find the closest edge to insert between
            int best_insert_idx = 0;
            float best_dist = std::numeric_limits<float>::max();

            for (size_t i = 0; i < tile_config.custom_vertices.size(); ++i) {
              size_t next_i = (i + 1) % tile_config.custom_vertices.size();

              Math::Vec2D v1 = tile_config.custom_vertices[i];
              Math::Vec2D v2 = tile_config.custom_vertices[next_i];
              Math::Vec2D new_v(local_x, local_y);

              // Calculate distance to edge midpoint
              Math::Vec2D edge_mid = (v1 + v2) * 0.5f;
              float dist = (new_v - edge_mid).get_magnitude();

              if (dist < best_dist) {
                best_dist = dist;
                best_insert_idx = static_cast<int>(next_i);
              }
            }

            tile_config.custom_vertices.insert(
              tile_config.custom_vertices.begin() + best_insert_idx,
              Math::Vec2D(local_x, local_y)
            );
          }
          changed = true;
        }
      }

      // Show vertex count
      ImGui::SetCursorScreenPos({ image_min.x, image_min.y - 20 });
      ImGui::Text("Vertices: %zu (Click to add, Drag to move, Right-click to delete)", tile_config.custom_vertices.size());

      if (changed) {
        update_all_tile_instances(current_atlas_index, selected_tile_index, tile_config);
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
      }
    }
  }

  ImGui::Columns(1);
  ImGui::End();
}
