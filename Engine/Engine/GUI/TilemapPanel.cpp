//-----------------------------------------------------------------------------------
// file:    TilemapPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Implementation of TilemapPanel for painting tiles on the world using
//          TextureAtlas. Provides tile selection, grid overlay, and mouse-based
//          tile placement directly in the game world.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "TilemapPanel.h"
#include <imgui.h>
#include "../ECS/ECS.h"
#include "../Components/Components.h"
#include "../InputManager.h"
#include "../Graphics/Renderer.h"
#include "../Resources.h"
#include "widgets/TextureAtlasWidget.h"
#include <vector>
#include <string>
#include <unordered_set>

namespace {
  // Tilemap state
  struct TilemapState {
    Components::Texture selected_texture;
    Components::TextureAtlas selected_atlas;
    int selected_tile_index = 0;
    bool show_grid = true;
    bool painting_enabled = false;
    bool erasing_enabled = false;
    float tile_width = 32.0f;
    float tile_height = 32.0f;
    float grid_snap = 32.0f;
    Math::Vec2D grid_offset = { 0.0f, 0.0f };
    int z_layer = 0;

    // Track placed tiles to prevent duplicates
    std::unordered_set<std::string> placed_tiles;
  };

  TilemapState tilemap_state;

  // Helper to generate a unique position key
  std::string get_position_key(const Math::Vec2D& pos, int z_layer) {
    // Round to grid
    int grid_x = static_cast<int>(std::floor(pos.x / tilemap_state.grid_snap));
    int grid_y = static_cast<int>(std::floor(pos.y / tilemap_state.grid_snap));
    return std::to_string(grid_x) + "_" + std::to_string(grid_y) + "_" + std::to_string(z_layer);
  }

  // Helper to snap position to grid
  Math::Vec2D snap_to_grid(const Math::Vec2D& pos) {
    float snapped_x = std::floor((pos.x - tilemap_state.grid_offset.x) / tilemap_state.grid_snap) * tilemap_state.grid_snap + tilemap_state.grid_offset.x;
    float snapped_y = std::floor((pos.y - tilemap_state.grid_offset.y) / tilemap_state.grid_snap) * tilemap_state.grid_snap + tilemap_state.grid_offset.y;
    return { snapped_x, snapped_y };
  }

  // Helper to delete tile at position
  void delete_tile_at_position(const Math::Vec2D& grid_pos, int z_layer) {
    std::string pos_key = get_position_key(grid_pos, z_layer);

    // Find and destroy entities at this position
    auto all_entities = Engine::ECS::get_all_entities();
    for (auto& entity : all_entities) {
      auto* transform = entity.get_component<Components::Transform2D>();
      if (transform) {
        Math::Vec2D entity_grid_pos = snap_to_grid(transform->position);
        std::string entity_key = get_position_key(entity_grid_pos, z_layer);

        if (entity_key == pos_key) {
          entity.destroy();
          tilemap_state.placed_tiles.erase(pos_key);
          break;
        }
      }
    }
  }

  // Helper to place tile at position
  void place_tile_at_position(const Math::Vec2D& world_pos) {
    if (!tilemap_state.selected_texture.texture_handle ||
      tilemap_state.selected_atlas.subTextures.empty() ||
      tilemap_state.selected_tile_index < 0 ||
      tilemap_state.selected_tile_index >= static_cast<int>(tilemap_state.selected_atlas.subTextures.size())) {
      return;
    }

    Math::Vec2D grid_pos = snap_to_grid(world_pos);
    std::string pos_key = get_position_key(grid_pos, tilemap_state.z_layer);

    // Check if tile already exists at this position
    if (tilemap_state.placed_tiles.find(pos_key) != tilemap_state.placed_tiles.end()) {
      return; // Don't place duplicate
    }

    // Create new tile entity
    Engine::ECS::Entity tile = Engine::ECS::create_entity();

    // Add transform (scale should match the tile dimensions)
    tile.add_component<Components::Transform2D>(
      grid_pos,
      0.0f,
      Math::Vec2D{ tilemap_state.tile_width, tilemap_state.tile_height }
    );

    // Add texture
    tile.add_component<Components::Texture>(tilemap_state.selected_texture);

    // Add texture atlas with selected tile
    Components::TextureAtlas atlas = tilemap_state.selected_atlas;
    atlas.drawIndex = tilemap_state.selected_tile_index;
    tile.add_component<Components::TextureAtlas>(atlas);

    // Add color (default white)
    tile.add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);

    // Track this tile
    tilemap_state.placed_tiles.insert(pos_key);
  }
}

void TilemapPanel() {
  ImGui::Begin("Tilemap Editor");

  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Tilemap Painting");
  ImGui::Separator();

  // === TEXTURE ATLAS SELECTION ===
  if (ImGui::CollapsingHeader("Atlas Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent();

    // Check if we have a valid texture and atlas
    bool has_valid_atlas = tilemap_state.selected_texture.texture_handle &&
      !tilemap_state.selected_atlas.subTextures.empty();

    if (!has_valid_atlas) {
      ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No atlas selected");
      ImGui::Text("Select an entity with Texture and TextureAtlas");
      ImGui::Text("components from the Entity Editor to use as tileset.");
    }
    else {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Atlas loaded");
      ImGui::Text("Texture: %dx%d",
        tilemap_state.selected_texture.texture_handle->width,
        tilemap_state.selected_texture.texture_handle->height);
      ImGui::Text("Tiles: %zu", tilemap_state.selected_atlas.subTextures.size());
      ImGui::Text("Selected tile: %d", tilemap_state.selected_tile_index);
    }

    // Button to load atlas from selected entity
    if (ImGui::Button("Load from Selected Entity")) {
      auto all_entities = Engine::ECS::get_all_entities();
      bool found = false;

      for (auto& entity : all_entities) {
        auto* texture = entity.get_component<Components::Texture>();
        auto* atlas = entity.get_component<Components::TextureAtlas>();

        if (texture && atlas && texture->texture_handle && !atlas->subTextures.empty()) {
          tilemap_state.selected_texture = *texture;
          tilemap_state.selected_atlas = *atlas;
          tilemap_state.selected_tile_index = 0;
          found = true;
          break;
        }
      }

      if (!found) {
        ImGui::OpenPopup("No Atlas Found");
      }
    }

    // Error popup
    if (ImGui::BeginPopupModal("No Atlas Found", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("No entity found with both Texture and TextureAtlas components!");
      if (ImGui::Button("OK", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::Unindent();
  }

  ImGui::Separator();

  // === TILE PREVIEW ===
  if (tilemap_state.selected_texture.texture_handle &&
    !tilemap_state.selected_atlas.subTextures.empty()) {

    if (ImGui::CollapsingHeader("Tile Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent();

      // Use the TextureAtlasWidget for tile selection
      ImGuiWidgets::TextureAtlasPreview(
        "TilesetPreview",
        tilemap_state.selected_texture,
        tilemap_state.selected_atlas,
        300.0f,
        [](size_t index) {
          tilemap_state.selected_tile_index = static_cast<int>(index);
        }
      );

      ImGui::Unindent();
    }
  }

  ImGui::Separator();

  // === PAINTING CONTROLS ===
  if (ImGui::CollapsingHeader("Painting Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent();

    // Painting mode toggle
    if (ImGui::Checkbox("Enable Painting", &tilemap_state.painting_enabled)) {
      if (tilemap_state.painting_enabled) {
        tilemap_state.erasing_enabled = false;
      }
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Click in world to place tiles)");

    // Erasing mode toggle
    if (ImGui::Checkbox("Enable Erasing", &tilemap_state.erasing_enabled)) {
      if (tilemap_state.erasing_enabled) {
        tilemap_state.painting_enabled = false;
      }
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Click in world to delete tiles)");

    ImGui::Separator();

    // Grid controls
    ImGui::Checkbox("Show Grid", &tilemap_state.show_grid);

    ImGui::DragFloat("Grid Snap Size", &tilemap_state.grid_snap, 1.0f, 1.0f, 256.0f, "%.1f");
    ImGui::DragFloat("Tile Width", &tilemap_state.tile_width, 1.0f, 1.0f, 256.0f, "%.1f");
    ImGui::DragFloat("Tile Height", &tilemap_state.tile_height, 1.0f, 1.0f, 256.0f, "%.1f");

    ImGui::DragFloat2("Grid Offset", &tilemap_state.grid_offset.x, 1.0f, -1000.0f, 1000.0f, "%.1f");

    ImGui::Separator();

    // Z-layer control
    ImGui::DragInt("Z-Layer", &tilemap_state.z_layer, 1.0f, -100, 100);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Not yet implemented in rendering)");

    ImGui::Separator();

    // Clear all tiles button
    if (ImGui::Button("Clear All Tiles", ImVec2(-1, 0))) {
      ImGui::OpenPopup("Clear Tiles Confirm");
    }

    if (ImGui::BeginPopupModal("Clear Tiles Confirm", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Are you sure you want to delete all placed tiles?");
      ImGui::Text("This action cannot be undone!");
      ImGui::Separator();

      if (ImGui::Button("Yes, Clear All", ImVec2(120, 0))) {
        auto all_entities = Engine::ECS::get_all_entities();
        for (auto& entity : all_entities) {
          if (entity.has_components<Components::Transform2D, Components::Texture, Components::TextureAtlas>()) {
            entity.destroy();
          }
        }
        tilemap_state.placed_tiles.clear();
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    ImGui::Unindent();
  }

  ImGui::Separator();

  // === STATISTICS ===
  if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent();

    ImGui::Text("Placed tiles: %zu", tilemap_state.placed_tiles.size());

    Math::Vec2D mouse_world_pos = Engine::Input::get_mouse_world_position();
    Math::Vec2D snapped_pos = snap_to_grid(mouse_world_pos);

    ImGui::Text("Mouse world pos: (%.1f, %.1f)", mouse_world_pos.x, mouse_world_pos.y);
    ImGui::Text("Snapped pos: (%.1f, %.1f)", snapped_pos.x, snapped_pos.y);

    ImGui::Unindent();
  }

  ImGui::Separator();

  // === INSTRUCTIONS ===
  if (ImGui::CollapsingHeader("Instructions")) {
    ImGui::Indent();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "How to use:");
    ImGui::BulletText("Load an atlas from an entity with Texture + TextureAtlas");
    ImGui::BulletText("Select a tile from the atlas preview");
    ImGui::BulletText("Enable painting mode");
    ImGui::BulletText("Click in the game world to place tiles");
    ImGui::BulletText("Enable erasing mode to delete tiles");
    ImGui::BulletText("Toggle grid to see tile boundaries");
    ImGui::BulletText("Adjust grid snap size to match your tile size");

    ImGui::Unindent();
  }

  ImGui::End();
}

// Separate function called during rendering phase
void TilemapPanel_Draw() {
  // === HANDLE PAINTING IN WORLD ===
  // Only process clicks when not hovering over ImGui windows
  if (!ImGui::GetIO().WantCaptureMouse) {
    if (tilemap_state.painting_enabled && Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
      Math::Vec2D world_pos = Engine::Input::get_mouse_world_position();
      place_tile_at_position(world_pos);
    }

    if (tilemap_state.erasing_enabled && Engine::Input::is_pressed(Engine::KeyCode::MouseLeft)) {
      Math::Vec2D world_pos = Engine::Input::get_mouse_world_position();
      Math::Vec2D grid_pos = snap_to_grid(world_pos);
      delete_tile_at_position(grid_pos, tilemap_state.z_layer);
    }
  }

  // === DRAW GRID OVERLAY ===
  if (tilemap_state.show_grid) {
    // Get camera and virtual resolution
    Math::Vec2D virtual_res = Engine::Graphics::get_virtual_resolution();
    Engine::Graphics::Camera camera = Engine::Graphics::get_camera();

    // Calculate visible area in world space
    float half_width = (virtual_res.x / 2.0f) / camera.zoom;
    float half_height = (virtual_res.y / 2.0f) / camera.zoom;

    Math::Vec2D camera_center = camera.position;
    float visible_left = camera_center.x - half_width;
    float visible_right = camera_center.x + half_width;
    float visible_top = camera_center.y - half_height;
    float visible_bottom = camera_center.y + half_height;

    // Calculate grid line positions
    float grid_size = tilemap_state.grid_snap;

    // Snap to grid
    float start_x = std::floor((visible_left - tilemap_state.grid_offset.x) / grid_size) * grid_size + tilemap_state.grid_offset.x;
    float start_y = std::floor((visible_top - tilemap_state.grid_offset.y) / grid_size) * grid_size + tilemap_state.grid_offset.y;

    // Draw vertical lines
    for (float x = start_x; x <= visible_right; x += grid_size) {
      Engine::Graphics::draw_line(
        Math::Vec2D{ x, visible_top },
        Math::Vec2D{ x, visible_bottom },
        1.0f,
        Math::Vec4D{ 0.5f, 0.5f, 0.5f, 0.3f }
      );
    }

    // Draw horizontal lines
    for (float y = start_y; y <= visible_bottom; y += grid_size) {
      Engine::Graphics::draw_line(
        Math::Vec2D{ visible_left, y },
        Math::Vec2D{ visible_right, y },
        1.0f,
        Math::Vec4D{ 0.5f, 0.5f, 0.5f, 0.3f }
      );
    }

    // Highlight current grid cell when painting
    if ((tilemap_state.painting_enabled || tilemap_state.erasing_enabled) && !ImGui::GetIO().WantCaptureMouse) {
      Math::Vec2D world_pos = Engine::Input::get_mouse_world_position();
      Math::Vec2D grid_pos = snap_to_grid(world_pos);

      Math::Vec4D highlight_color = tilemap_state.painting_enabled ?
        Math::Vec4D{ 0.0f, 1.0f, 0.0f, 0.4f } :
        Math::Vec4D{ 1.0f, 0.0f, 0.0f, 0.4f };

      Engine::Graphics::draw_rect(
        grid_pos,
        Math::Vec2D{ grid_size, grid_size },
        0.0f,
        highlight_color
      );
    }
  }
}
