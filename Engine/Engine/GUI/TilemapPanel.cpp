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

namespace TilemapPanel {
  // Internal state
  namespace {
    // Submitted atlases
    struct AtlasEntry {
      Components::Texture texture;
      Components::TextureAtlas atlas;
      std::string name;
    };

    std::vector<AtlasEntry> submitted_atlases;
    int selected_atlas_index = -1;
    int selected_tile_index = 0;

    // Current preview (before submission)
    Components::Texture preview_texture;
    Components::TextureAtlas preview_atlas;
    int atlas_rows = 1;
    int atlas_cols = 1;

    // Painting state
    bool show_grid = false;
    bool painting_enabled = false;
    bool erasing_enabled = false;
    float grid_snap = 64.0f;
    float prev_grid_snap = 64.0f;
    Math::Vec2D grid_offset = { 0.0f, 0.0f };
    Math::Vec2D prev_grid_offset = { 0.0f, 0.0f };

    // Track placed tiles to prevent duplicates
    std::unordered_set<std::string> placed_tiles;
  }

  // Helper functions

  void regenerate_atlas() {
    if (!preview_texture.texture_handle) {
      return;
    }

    int texture_width = preview_texture.texture_handle->width;
    int texture_height = preview_texture.texture_handle->height;

    int tile_width = texture_width / atlas_cols;
    int tile_height = texture_height / atlas_rows;

    preview_atlas = Components::TextureAtlas(
      tile_width,
      tile_height,
      atlas_rows,
      atlas_cols
    );
  }

  std::string get_position_key(const Math::Vec2D& pos) {
    int grid_x = static_cast<int>(std::floor(pos.x / grid_snap));
    int grid_y = static_cast<int>(std::floor(pos.y / grid_snap));
    return std::to_string(grid_x) + "_" + std::to_string(grid_y);
  }

  Math::Vec2D snap_to_grid(const Math::Vec2D& pos) {
    float half_snap = grid_snap * 0.5f;
    float snapped_x = std::floor((pos.x - grid_offset.x) / grid_snap) * grid_snap + grid_offset.x + half_snap;
    float snapped_y = std::floor((pos.y - grid_offset.y) / grid_snap) * grid_snap + grid_offset.y + half_snap;
    return { snapped_x, snapped_y };
  }

  void delete_tile_at_position(const Math::Vec2D& grid_pos) {
    std::string pos_key = get_position_key(grid_pos);

    auto all_entities = Engine::ECS::get_all_entities();
    for (auto& entity : all_entities) {
      if (entity.has_component<Components::Tile>()) {
        auto* transform = entity.get_component<Components::Transform2D>();
        if (transform) {
          Math::Vec2D entity_grid_pos = snap_to_grid(transform->position);
          std::string entity_key = get_position_key(entity_grid_pos);

          if (entity_key == pos_key) {
            entity.destroy();
            placed_tiles.erase(pos_key);
            break;
          }
        }
      }
    }
  }

  void update_all_tile_dimensions() {
    auto all_entities = Engine::ECS::get_all_entities();

    placed_tiles.clear();

    for (auto& entity : all_entities) {
      if (entity.has_component<Components::Tile>()) {
        auto* transform = entity.get_component<Components::Transform2D>();
        if (transform) {
          Math::Vec2D new_pos = snap_to_grid(transform->position);
          transform->position = new_pos;
          transform->scale = Math::Vec2D{ grid_snap, grid_snap };

          std::string pos_key = get_position_key(new_pos);
          placed_tiles.insert(pos_key);
        }
      }
    }
  }

  void place_tile_at_position(const Math::Vec2D& world_pos) {
    // Check if we have a valid atlas selected
    if (selected_atlas_index < 0 || selected_atlas_index >= static_cast<int>(submitted_atlases.size())) {
      return;
    }

    const AtlasEntry& atlas_entry = submitted_atlases[selected_atlas_index];

    if (!atlas_entry.texture.texture_handle ||
      atlas_entry.atlas.subTextures.empty() ||
      selected_tile_index < 0 ||
      selected_tile_index >= static_cast<int>(atlas_entry.atlas.subTextures.size())) {
      return;
    }

    Math::Vec2D grid_pos = snap_to_grid(world_pos);
    std::string pos_key = get_position_key(grid_pos);

    // If a tile already exists at this position, delete it first (overwrite behavior)
    if (placed_tiles.find(pos_key) != placed_tiles.end()) {
      // Find and destroy the existing tile entity 
      for (auto& entity : Engine::ECS::get_entities_with<Components::Tile>()) {
        // TODO: This is very inefficient, implement hashing for Transform2D so we dont have to loop all entities 
        auto* transform = entity.get_component<Components::Transform2D>();
        if (transform) {
          Math::Vec2D entity_grid_pos = snap_to_grid(transform->position);
          std::string entity_key = get_position_key(entity_grid_pos);

          if (entity_key == pos_key) {
            entity.destroy();
            break;
          }
        }
      }

      // Remove from tracking set (will be re-added below)
      placed_tiles.erase(pos_key);
    }

    Engine::ECS::Entity tile = Engine::ECS::create_entity()
      .add_component<Components::Tile>()
      .add_component<Components::Transform2D>(
        grid_pos,
        0.0f,
        Math::Vec2D{ grid_snap, grid_snap }
      )
      .add_component<Components::Texture>(atlas_entry.texture)
      .add_component<Components::TextureAtlas>(atlas_entry.atlas)
      .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);

    tile.get_component<Components::TextureAtlas>()->drawIndex = selected_tile_index;

    placed_tiles.insert(pos_key);
  }

  // Public interface
  void update() {
    // === HANDLE PAINTING IN WORLD ===
    if (!ImGui::GetIO().WantCaptureMouse) {
      if (painting_enabled && Engine::Input::is_held(Engine::KeyCode::MouseLeft)) {
        Math::Vec2D world_pos = Engine::Input::get_mouse_world_position();
        place_tile_at_position(world_pos);
      }

      if (erasing_enabled && Engine::Input::is_held(Engine::KeyCode::MouseLeft)) {
        Math::Vec2D world_pos = Engine::Input::get_mouse_world_position();
        Math::Vec2D grid_pos = snap_to_grid(world_pos);
        delete_tile_at_position(grid_pos);
      }
    }
  }

  void draw_panel() {
    ImGui::Begin("Tilemap Editor");

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Tilemap Painting");
    ImGui::Separator();

    // === TEXTURE ATLAS SELECTION ===
    if (ImGui::Button("Create Atlas")) {
      ImGui::OpenPopup("Atlas Selection");
    }
    ImGui::SetNextWindowSize(ImVec2(560, 540), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(480, 420), ImVec2(FLT_MAX, FLT_MAX));

    bool modal_open = true;

    if (ImGui::BeginPopupModal("Atlas Selection", &modal_open, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking)) {

      if (!modal_open) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        modal_open = true;
        return;
      }

      if (ImGui::CollapsingHeader("Atlas Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Get all loaded textures from Resources
        std::vector<std::string> texture_paths;
        for (const auto& pair : Engine::Resources::texture_cache) {
          if (auto texture = pair.second.lock()) {
            texture_paths.push_back(pair.first);
          }
        }

        if (texture_paths.empty()) {
          ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No textures loaded in Resources!");
          ImGui::Text("Load textures first using Resources::load_texture()");
        }
        else {
          ImGui::Text("Available Textures:");

          // Texture selection combo box
          const char* preview_name = preview_texture.texture_handle ?
            preview_texture.texture_handle->path.c_str() : "Select texture...";

          if (ImGui::BeginCombo("##TextureSelect", preview_name)) {
            for (int i = 0; i < static_cast<int>(texture_paths.size()); ++i) {
              bool is_selected = preview_texture.texture_handle &&
                (preview_texture.texture_handle->path == texture_paths[i]);

              if (ImGui::Selectable(texture_paths[i].c_str(), is_selected)) {
                // Load the texture
                if (auto tex = Engine::Resources::texture_cache[texture_paths[i]].lock()) {
                  preview_texture = Components::Texture(tex);
                  regenerate_atlas();
                }
              }
              if (is_selected) {
                ImGui::SetItemDefaultFocus();
              }
            }
            ImGui::EndCombo();
          }

          // Atlas configuration and preview
          if (preview_texture.texture_handle) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Atlas Configuration");

            ImGui::Text("Texture: %dx%d",
              preview_texture.texture_handle->width,
              preview_texture.texture_handle->height);

            // Rows and columns input
            if (ImGui::DragInt("Rows", &atlas_rows, 1.0f, 1, 100)) {
              regenerate_atlas();
            }

            if (ImGui::DragInt("Columns", &atlas_cols, 1.0f, 1, 100)) {
              regenerate_atlas();
            }

            ImGui::Text("Tiles: %zu", preview_atlas.subTextures.size());

            // Atlas preview
            if (!preview_atlas.subTextures.empty()) {
              ImGui::Separator();
              ImGui::Text("Preview:");
              ImGuiWidgets::TextureAtlasPreview(
                "AtlasPreview",
                preview_texture,
                preview_atlas,
                60.0f,
                [](size_t) {} // No selection in preview
              );
            }

            // Submit button
            ImGui::Separator();
            static char atlas_name[128] = "";
            ImGui::InputText("Atlas Name", atlas_name, IM_ARRAYSIZE(atlas_name));

            if (ImGui::Button("Submit Atlas", ImVec2(-1, 0))) {
              if (strlen(atlas_name) > 0) {
                AtlasEntry entry;
                entry.texture = preview_texture;
                entry.atlas = preview_atlas;
                entry.name = std::string(atlas_name);
                submitted_atlases.push_back(entry);

                // Clear the name field
                atlas_name[0] = '\0';
              }
            }
          }
        }

        ImGui::Unindent();
      }

      if (ImGui::Button("Done?", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    ImGui::Separator();

    // === TILE SELECTION ===
    if (!submitted_atlases.empty()) {
      if (ImGui::CollapsingHeader("Tile Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        ImGui::Text("Tilemaps:");
        ImGui::Separator();

        // Display all submitted atlases
        for (int i = 0; i < static_cast<int>(submitted_atlases.size()); ++i) {
          const AtlasEntry& entry = submitted_atlases[i];

          ImGui::PushID(i);

          // Atlas name and select button
          bool is_selected = (i == selected_atlas_index);
          if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
          }

          // Create non-const copies for TextureAtlasPreview
          Components::Texture temp_texture = entry.texture;
          Components::TextureAtlas temp_atlas = entry.atlas;

          // Set the drawIndex to the currently selected tile if this is the selected atlas
          if (is_selected) {
            temp_atlas.drawIndex = selected_tile_index;
          }
          else {
            temp_atlas.drawIndex = -1; // No tile selected in this atlas
          }

          ImGuiWidgets::TextureAtlasPreview(
            "TileSelection",
            temp_texture,
            temp_atlas,
            80.0f,
            [i](size_t tile_idx) {
              selected_atlas_index = i;
              selected_tile_index = static_cast<int>(tile_idx);
            }
          );

          // Remove button
          if (ImGui::Button("Remove Atlas", ImVec2(-1, 0))) {
            submitted_atlases.erase(submitted_atlases.begin() + i);
            if (selected_atlas_index >= static_cast<int>(submitted_atlases.size())) {
              selected_atlas_index = static_cast<int>(submitted_atlases.size()) - 1;
            }
          }

          if (is_selected) {
            ImGui::PopStyleColor();
          }

          ImGui::PopID();
          ImGui::Separator();
        }

        ImGui::Unindent();
      }
    }

    ImGui::Separator();

    // === PAINTING CONTROLS ===
    if (ImGui::CollapsingHeader("Painting Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent();

      // Painting mode toggle
      if (ImGui::Checkbox("Enable Painting", &painting_enabled)) {
        if (painting_enabled) {
          erasing_enabled = false;
        }
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Click in world to place tiles)");

      // Erasing mode toggle
      if (ImGui::Checkbox("Enable Erasing", &erasing_enabled)) {
        if (erasing_enabled) {
          painting_enabled = false;
        }
      }


      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Click in world to delete tiles)");

      ImGui::Separator();

      // Grid controls
      ImGui::Checkbox("Show Grid", &show_grid);

      // Tile size with change detection
      if (ImGui::DragFloat("Tile Size", &grid_snap, 1.0f, 1.0f, 256.0f, "%.1f")) {
        if (grid_snap != prev_grid_snap) {
          update_all_tile_dimensions();
          prev_grid_snap = grid_snap;
        }
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Updates all tiles)");

      // Grid offset with change detection
      if (ImGui::DragFloat2("Grid Offset", &grid_offset.x, 1.0f, -1000.0f, 1000.0f, "%.1f")) {
        if (grid_offset.x != prev_grid_offset.x ||
          grid_offset.y != prev_grid_offset.y) {
          update_all_tile_dimensions();
          prev_grid_offset = grid_offset;
        }
      }

      ImGui::Separator();

      // Clear all tiles button
      if (ImGui::Button("Clear All Tiles", ImVec2(-1, 0))) {
        ImGui::OpenPopup("Clear Tiles Confirm");
      }

      bool modal_openb = true;

      if (ImGui::BeginPopupModal("Clear Tiles Confirm", &modal_openb, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete all placed tiles?");
        ImGui::Text("This action cannot be undone!");
        ImGui::Separator();

        if (!modal_openb) {
          ImGui::CloseCurrentPopup();
          ImGui::EndPopup();
          modal_openb = true;
          return;
        }

        if (ImGui::Button("Yes, Clear All", ImVec2(120, 0))) {
          auto all_entities = Engine::ECS::get_all_entities();
          for (auto& entity : all_entities) {
            if (entity.has_components<Components::Tile>()) {
              entity.destroy();
            }
          }
          placed_tiles.clear();
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

    // === EXPORT/IMPORT ===
    if (ImGui::CollapsingHeader("Export/Import")) {
      ImGui::Indent();

      if (ImGui::Button("Export Tilemap")) {
        // TODO: Implement export functionality in Resources first
      }

      ImGui::Unindent();
    }

    // === STATISTICS ===
    if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent();

      ImGui::Text("Placed tiles: %zu", placed_tiles.size());

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

  void draw_overlay() {
    // === DRAW GRID OVERLAY ===
    if (show_grid) {
      Math::Vec2D virtual_res = Engine::Graphics::get_virtual_resolution();
      Engine::Graphics::Camera camera = Engine::Graphics::get_camera();

      float view_width = virtual_res.x / camera.zoom;
      float view_height = virtual_res.y / camera.zoom;

      float visible_left = camera.position.x;
      float visible_right = camera.position.x + view_width;
      float visible_top = camera.position.y;
      float visible_bottom = camera.position.y + view_height;

      float grid_size = grid_snap;

      float start_x = std::floor((visible_left - grid_offset.x) / grid_size) * grid_size + grid_offset.x;
      float start_y = std::floor((visible_top - grid_offset.y) / grid_size) * grid_size + grid_offset.y;

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
      if ((painting_enabled || erasing_enabled) && !ImGui::GetIO().WantCaptureMouse) {
        Math::Vec2D world_pos = Engine::Input::get_mouse_world_position();
        Math::Vec2D grid_pos = snap_to_grid(world_pos);

        Math::Vec4D highlight_color = painting_enabled ?
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
} // namespace TilemapPanel
