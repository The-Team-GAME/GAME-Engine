//-----------------------------------------------------------------------------------
// file:    TilemapPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (50%), Chew Shee Yang (50%)
//          Primary author: Ong Zi Ming
//          Refactored tilemap logic to TilemapManager: Chew Shee Yang
//
// email:   o.ziming@digipen.edu
//          chew.s@digipen.edu
//
// brief:   Implementation of TilemapPanel UI for the tilemap editor. Handles
//          user interface, tool selection, and delegates tilemap operations to
//          TilemapManager for core logic.
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
#include "../Path.h"
#include "widgets/TextureAtlasWidget.h"
#include <vector>
#include "../FileIO/PrefabsFileIO.h"
#include "../FileIO/Helpers.h"
#include "../FileIO/SettingsManager.h"
#include "../FileIO/Settings.h"
#include "ViewportPanel.h"
#include "../ProjectSaveManager.h"
#include "../UndoManager/UndoManager.h"
#include "../TilemapManager.h"
#include "../Level.h"
#include "../ImGuiManager.h"
#include "../Logger.h"

namespace TilemapPanel {
  // Internal state
  namespace {
    std::vector<Engine::Graphics::AtlasEntry> loaded_tilemaps;
    Engine::Graphics::Tile selected_tile;

    // Current preview (before submission)
    Components::Texture preview_texture;
    Components::TextureAtlas preview_atlas;
    int atlas_rows = 1;
    int atlas_cols = 1;

    // Painting state
    bool enable_tilemap_mode = false;

    // Tool modes
    enum class ToolMode {
      Paintbrush,
      BoxFill,
      Picker,
      Eraser
    };
    ToolMode current_tool = ToolMode::Paintbrush;

    // Layer selection
    std::string selected_layer = "default";
    bool dim_non_selected_layers = true; // Toggle for dimming non-selected layers

    // BoxFill state
    bool box_fill_dragging = false;
    Math::Vec2D box_fill_start;
    Math::Vec2D box_fill_end;
  }

  // Helper functions

  // Doesnt actually save to file yet, just updates project settings in memory
  // TODO: rename to be more accurate
  void save_loaded_tilemaps_to_project_settings() {
    auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get_mutable();
    project_settings.loaded_tilemaps = loaded_tilemaps;
    Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings());
  }

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

  void delete_tile_at_position(const Math::Vec2D& world_pos) {
    int grid_x, grid_y;
    TilemapManager::world_to_grid(world_pos, grid_x, grid_y);

    // Check if there's a tile to delete
    Engine::Graphics::Tile old_tile = TilemapManager::get_tile_at(selected_layer, grid_x, grid_y);
    if (!old_tile.is_valid()) return; // Nothing to delete

    // Capture state for undo
    int captured_grid_x = grid_x;
    int captured_grid_y = grid_y;
    Engine::Graphics::Tile captured_tile = old_tile;
    std::string captured_layer = selected_layer;

    Engine::UndoManager::execute(
      [captured_grid_x, captured_grid_y, captured_layer, world_pos]() {
        TilemapManager::delete_tile(world_pos, captured_layer);
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      },
      [captured_grid_x, captured_grid_y, captured_tile, captured_layer]() {
        Math::Vec2D restore_pos = TilemapManager::grid_to_world(captured_grid_x, captured_grid_y);
        TilemapManager::place_tile(restore_pos, captured_layer, captured_tile.atlas_index, captured_tile.tile_index);
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      }
    );
  }

  void update_all_tile_dimensions() {
    TilemapManager::update_all_tile_dimensions();
  }

  void pick_tile_at_position(const Math::Vec2D& world_pos) {
    int grid_x, grid_y;
    TilemapManager::world_to_grid(world_pos, grid_x, grid_y);

    // Get the tile at this position on the selected layer
    Engine::Graphics::Tile tile = TilemapManager::get_tile_at(selected_layer, grid_x, grid_y);
    if (!tile.is_valid()) {
      return; // No tile at this position
    }

    // Verify the atlas index is valid
    if (tile.atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
      return;
    }

    // Set the selected tile
    selected_tile = tile;

    // Switch back to Paintbrush tool after picking
    current_tool = ToolMode::Paintbrush;
  }

  void fill_box_with_tiles(const Math::Vec2D& start_pos, const Math::Vec2D& end_pos) {
    // Check if we have a valid atlas selected
    if (!selected_tile.is_valid() || selected_tile.atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
      return;
    }

    const Engine::Graphics::AtlasEntry& atlas_entry = loaded_tilemaps[selected_tile.atlas_index];

    if (!atlas_entry.texture_handle ||
      atlas_entry.subTextures.empty() ||
      selected_tile.tile_index >= static_cast<int>(atlas_entry.subTextures.size())) {
      return;
    }

    // Convert to grid coordinates
    int start_grid_x, start_grid_y, end_grid_x, end_grid_y;
    TilemapManager::world_to_grid(start_pos, start_grid_x, start_grid_y);
    TilemapManager::world_to_grid(end_pos, end_grid_x, end_grid_y);

    // Ensure start is less than end
    if (start_grid_x > end_grid_x) std::swap(start_grid_x, end_grid_x);
    if (start_grid_y > end_grid_y) std::swap(start_grid_y, end_grid_y);

    // Capture state for undo
    struct TileState {
      int x, y;
      Engine::Graphics::Tile old_tile;
    };
    std::vector<TileState> old_tiles;

    // Collect old tile states
    for (int y = start_grid_y; y <= end_grid_y; ++y) {
      for (int x = start_grid_x; x <= end_grid_x; ++x) {
        Engine::Graphics::Tile old_tile = TilemapManager::get_tile_at(selected_layer, x, y);
        old_tiles.push_back({ x, y, old_tile });
      }
    }

    // Capture variables for lambda
    int captured_start_x = start_grid_x;
    int captured_start_y = start_grid_y;
    int captured_end_x = end_grid_x;
    int captured_end_y = end_grid_y;
    Engine::Graphics::Tile captured_tile = selected_tile;
    std::string captured_layer = selected_layer;
    std::vector<TileState> captured_old_tiles = old_tiles;

    Engine::UndoManager::execute(
      [captured_start_x, captured_start_y, captured_end_x, captured_end_y, captured_tile, captured_layer]() {
        // Fill the box with new tiles
        for (int y = captured_start_y; y <= captured_end_y; ++y) {
          for (int x = captured_start_x; x <= captured_end_x; ++x) {
            Math::Vec2D world_pos = TilemapManager::grid_to_world(x, y);
            TilemapManager::place_tile(world_pos, captured_layer, captured_tile.atlas_index, captured_tile.tile_index);
          }
        }
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      },
      [captured_old_tiles, captured_layer]() {
        // Restore old tiles
        for (const auto& tile_state : captured_old_tiles) {
          Math::Vec2D restore_pos = TilemapManager::grid_to_world(tile_state.x, tile_state.y);
          if (!tile_state.old_tile.is_valid()) {
            // Tile didn't exist, delete it
            TilemapManager::delete_tile(restore_pos, captured_layer);
          }
          else {
            // Restore the old tile
            TilemapManager::place_tile(restore_pos, captured_layer, tile_state.old_tile.atlas_index, tile_state.old_tile.tile_index);
          }
        }
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      }
    );
  }

  void place_tile_at_position(const Math::Vec2D& world_pos) {
    // Check if we have a valid atlas selected
    if (!selected_tile.is_valid() || selected_tile.atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
      return;
    }

    const Engine::Graphics::AtlasEntry& atlas_entry = loaded_tilemaps[selected_tile.atlas_index];

    if (!atlas_entry.texture_handle ||
      atlas_entry.subTextures.empty() ||
      selected_tile.tile_index >= static_cast<int>(atlas_entry.subTextures.size())) {
      return;
    }

    int grid_x, grid_y;
    TilemapManager::world_to_grid(world_pos, grid_x, grid_y);

    // Check if the tile is already the same
    Engine::Graphics::Tile current_tile = TilemapManager::get_tile_at(selected_layer, grid_x, grid_y);
    if (current_tile.atlas_index == selected_tile.atlas_index && current_tile.tile_index == selected_tile.tile_index) {
      return; // Same tile, nothing to do
    }

    // Capture state for undo
    int captured_grid_x = grid_x;
    int captured_grid_y = grid_y;
    Engine::Graphics::Tile captured_old_tile = current_tile;
    Engine::Graphics::Tile captured_new_tile = selected_tile;
    std::string captured_layer = selected_layer;

    Engine::UndoManager::execute(
      [world_pos, captured_new_tile, captured_layer]() {
        TilemapManager::place_tile(world_pos, captured_layer, captured_new_tile.atlas_index, captured_new_tile.tile_index);
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      },
      [captured_grid_x, captured_grid_y, captured_old_tile, captured_layer]() {
        Math::Vec2D restore_pos = TilemapManager::grid_to_world(captured_grid_x, captured_grid_y);
        if (!captured_old_tile.is_valid()) {
          // If there was no tile before, delete it
          TilemapManager::delete_tile(restore_pos, captured_layer);
        }
        else {
          // Restore the old tile
          TilemapManager::place_tile(restore_pos, captured_layer, captured_old_tile.atlas_index, captured_old_tile.tile_index);
        }
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      }
    );
  }

  // Public interface
  void init() {
    const auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get();
    loaded_tilemaps = project_settings.loaded_tilemaps;

    // Reload texture handles from paths (since texture_handle is not serialized)
    for (auto& entry : loaded_tilemaps) {
      if (!entry.texture_path.empty() && !entry.texture_handle) {
        // Load the texture using Resources (handles cache lookup automatically)
        entry.texture_handle = Engine::Resources::load_texture(entry.texture_path, true, false);
        if (!entry.texture_handle || entry.texture_handle->id == 0) {
          Log::warning("Failed to load tilemap atlas texture: " + entry.texture_path);
        }
      }
    }

    // Initialize TilemapManager
    TilemapManager::init();

    // Rebuild all tilemaps from loaded TileMap entities
    for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
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

  void update() {
    // === KEYBOARD SHORTCUTS FOR TOOLS ===
    if (enable_tilemap_mode) {
      // Tool selection shortcuts
      if (Engine::Input::is_pressed(Engine::KeyCode::B)) {
        current_tool = ToolMode::Paintbrush;
      }
      else if (Engine::Input::is_pressed(Engine::KeyCode::U)) {
        current_tool = ToolMode::BoxFill;
      }
      else if (Engine::Input::is_pressed(Engine::KeyCode::I)) {
        current_tool = ToolMode::Picker;
      }
      else if (Engine::Input::is_pressed(Engine::KeyCode::D)) {
        current_tool = ToolMode::Eraser;
      }

      // Toggle dim non-selected layers with L key
      if (Engine::Input::is_pressed(Engine::KeyCode::L)) {
        dim_non_selected_layers = !dim_non_selected_layers;
      }
    }

    // === HANDLE PAINTING IN WORLD ===
    // Only allow painting/erasing when tilemap mode is enabled
    auto world_pos_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

    if (enable_tilemap_mode) {
      if (!world_pos_opt.has_value()) {
        /// Empty by design: if mouse is outside viewport, do nothing
      }
      else if (Engine::Input::is_held(Engine::KeyCode::MouseLeft)) {
        Math::Vec2D world_pos = world_pos_opt.value();

        // Check for temporary tool modifiers
        ToolMode active_tool = current_tool;

        // Ctrl temporarily switches to Picker (unless already in Eraser mode)
        if (Engine::Input::is_held(Engine::KeyCode::CtrlLeft) || Engine::Input::is_held(Engine::KeyCode::CtrlRight)) {
          if (current_tool == ToolMode::Paintbrush || current_tool == ToolMode::BoxFill) {
            active_tool = ToolMode::Picker;
          }
        }

        // Shift temporarily switches to Eraser
        if (Engine::Input::is_held(Engine::KeyCode::ShiftLeft) || Engine::Input::is_held(Engine::KeyCode::ShiftRight)) {
          if (current_tool == ToolMode::Paintbrush || current_tool == ToolMode::BoxFill) {
            active_tool = ToolMode::Eraser;
          }
        }

        // Execute tool action
        if (active_tool == ToolMode::Paintbrush) {
          place_tile_at_position(world_pos);
        }
        else if (active_tool == ToolMode::BoxFill) {
          // BoxFill: Start dragging on mouse press
          if (!box_fill_dragging) {
            box_fill_dragging = true;
            box_fill_start = world_pos;
            box_fill_end = world_pos;
          }
          else {
            // Update end position while dragging
            box_fill_end = world_pos;
          }
        }
        else if (active_tool == ToolMode::Picker) {
          pick_tile_at_position(world_pos);
        }
        else if (active_tool == ToolMode::Eraser) {
          delete_tile_at_position(world_pos);
        }
      }
      else if (box_fill_dragging) {
        // Mouse released - execute the box fill
        fill_box_with_tiles(box_fill_start, box_fill_end);
        box_fill_dragging = false;
      }
      else {
        // Mouse not held and not dragging - reset box fill state
        if (box_fill_dragging) {
          box_fill_dragging = false;
        }
      }
    }
  }


  void draw_panel() {
    ImGui::Begin("Tilemap Editor");

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Tilemap Painting");
    ImGui::Separator();

    // === TILEMAP MODE TOGGLE ===
    // bool prev_tilemap_mode = enable_tilemap_mode;
    ImGui::Checkbox("Enable Tilemap Mode", &enable_tilemap_mode);

    // Dim non-selected layers toggle
    ImGui::Checkbox("Dim Non-Selected Layers", &dim_non_selected_layers);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(L)");

    // Check if trying to enable tilemap mode without a level loaded
    // if (!prev_tilemap_mode && enable_tilemap_mode) {
    //   std::string current_level = Engine::LevelManager::get_current_level_path();
    //   if (current_level.empty()) {
    //     enable_tilemap_mode = false;
    //     ImGuiManager::show_toast("A level must be loaded before enabling tilemap mode", ImGuiManager::ToastType::Warning);
    //   }
    // }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Toggle grid & editing)");

    if (!enable_tilemap_mode) {
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::BeginDisabled();
    }

    ImGui::Separator();
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

                  // Automatically calculate columns and rows by dividing by 32
                  atlas_cols = preview_texture.texture_handle->width / 32;
                  atlas_rows = preview_texture.texture_handle->height / 32;

                  // Ensure at least 1 row and 1 column
                  if (atlas_cols < 1) atlas_cols = 1;
                  if (atlas_rows < 1) atlas_rows = 1;

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
            if (ImGui::DragInt("Columns", &atlas_cols, 1.0f, 1, 100)) {
              regenerate_atlas();
            }

            if (ImGui::DragInt("Rows", &atlas_rows, 1.0f, 1, 100)) {
              regenerate_atlas();
            }

            ImGui::Text("Tiles: %zu", preview_atlas.subTextures.size());

            // Atlas preview
            if (!preview_atlas.subTextures.empty()) {
              ImGui::Separator();
              ImGui::Text("Preview:");
              ImGuiWidgets::TextureAtlasPreview(
                preview_texture,
                preview_atlas,
                600.0f,
                [](size_t) {} // No selection in preview
              );
            }

            // Submit button
            ImGui::Separator();

            if (ImGui::Button("Submit Atlas", ImVec2(-1, 0))) {
              Engine::Graphics::AtlasEntry entry;
              entry.texture_handle = preview_texture.texture_handle;
              entry.texture_path = preview_texture.texture_handle->path;
              entry.subTextures = preview_atlas.subTextures;
              entry.drawIndex = preview_atlas.drawIndex;
              loaded_tilemaps.push_back(entry);

              // Save to project settings
              save_loaded_tilemaps_to_project_settings();
            }
          }
        }

        ImGui::Unindent();
      }

      // if (ImGui::Button("Done?", ImVec2(120, 0))) {
      //   ImGui::CloseCurrentPopup();
      // }

      ImGui::EndPopup();
    }

    ImGui::Separator();
    // === PAINTING CONTROLS ===
    if (ImGui::CollapsingHeader("Painting Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent();

      // Tool selection
      ImGui::Text("Tool:");

      int tool_index = static_cast<int>(current_tool);
      if (ImGui::RadioButton("Paintbrush (B)", &tool_index, static_cast<int>(ToolMode::Paintbrush))) {
        current_tool = ToolMode::Paintbrush;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Box Fill (U)", &tool_index, static_cast<int>(ToolMode::BoxFill))) {
        current_tool = ToolMode::BoxFill;
      }

      if (ImGui::RadioButton("Picker (I)", &tool_index, static_cast<int>(ToolMode::Picker))) {
        current_tool = ToolMode::Picker;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Eraser (D)", &tool_index, static_cast<int>(ToolMode::Eraser))) {
        current_tool = ToolMode::Eraser;
      }

      ImGui::Spacing();
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tip: Hold Ctrl for Picker, Shift for Eraser");

      ImGui::Separator();

      // Get current grid settings from TilemapManager
      float grid_snap = TilemapManager::get_grid_size();
      Math::Vec2D grid_offset = TilemapManager::get_grid_offset();

      // Tile size with change detection
      float new_grid_snap = grid_snap;
      if (ImGui::DragFloat("Tile Size", &new_grid_snap, 1.0f, 1.0f, 256.0f, "%.1f")) {
        if (new_grid_snap != grid_snap) {
          TilemapManager::set_grid_size(new_grid_snap);
          update_all_tile_dimensions();
        }
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Updates all tiles)");

      // Grid offset with change detection
      Math::Vec2D new_grid_offset = grid_offset;
      if (ImGui::DragFloat2("Grid Offset", &new_grid_offset.x, 1.0f, -1000.0f, 1000.0f, "%.1f")) {
        if (new_grid_offset.x != grid_offset.x || new_grid_offset.y != grid_offset.y) {
          TilemapManager::set_grid_offset(new_grid_offset);
          update_all_tile_dimensions();
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
            if (entity.has_components<Components::TileCollider>()) {
              entity.destroy();
              Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
            }
          }
          // Also clear all TileMap entities
          for (auto& entity : Engine::ECS::get_entities_with<Components::TileMap>()) {
            entity.destroy();
          }
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

    // === LAYER SELECTION ===
    if (ImGui::CollapsingHeader("Layer Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent();

      // Get all available layers
      auto& containers = Engine::ECS::Internal::get_containers();

      ImGui::Text("Active Painting Layer:");

      bool any_layers_registered = false;
      for (Engine::ECS::LayerID layer_id = 0; layer_id < containers.layer_id_to_name.size(); ++layer_id) {
        const std::string& layer_name = containers.layer_id_to_name[layer_id];

        // Skip empty layer names
        if (layer_name.empty()) continue;

        any_layers_registered = true;

        bool is_selected = (selected_layer == layer_name);

        if (ImGui::RadioButton((layer_name + " (ID: " + std::to_string(layer_id) + ")").c_str(), is_selected)) {
          selected_layer = layer_name;
        }
      }

      if (!any_layers_registered) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  (No layers registered)");
      }

      ImGui::Separator();

      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "New tiles will be placed on this layer");

      ImGui::Unindent();
    }

    ImGui::Separator();

    // // === SAVE/LOAD ===
    // if (ImGui::CollapsingHeader("Save/Load"), ImGuiTreeNodeFlags_DefaultOpen) {
    //   ImGui::Indent();

    //   // Display current filename
    //   ImGui::Text("Current file: %s", tilemap_filename);
    //   ImGui::Spacing();


    //   if (ImGui::Button("Save...")) {
    //     show_save_as_popup = true;
    //     ImGui::OpenPopup("Save Tilemap As");
    //   }

    //   // Save As popup
    //   if (ImGui::BeginPopupModal("Save Tilemap As", &show_save_as_popup, ImGuiWindowFlags_AlwaysAutoResize)) {
    //     ImGui::Text("Enter filename:");
    //     ImGui::InputText("##filename", tilemap_filename, sizeof(tilemap_filename));

    //     ImGui::Spacing();

    //     if (ImGui::Button("Save", ImVec2(120, 0))) {
    //       if (strlen(tilemap_filename) > 0) {
    //         // Perform save
    //         auto tilemap_entities = Engine::ECS::get_entities_with<Components::Tile>();
    //         ReflectionLibrary::GenericValue prefab_array = ReflectionLibrary::Array{};

    //         for (auto& entity : tilemap_entities) {
    //           Engine::ECS::Prefab prefab = Engine::ECS::entity_to_prefab(entity);
    //           auto prefab_gen = Engine::ECS::prefab_to_generic_value(prefab);
    //           prefab_array.getArray().push_back(prefab_gen);
    //         }

    //         auto prefab_array_json = ReflectionLibrary::generic_value_to_json(prefab_array, true);

    //         // Ensure the Assets/tilemap directory exists and prepend path if needed
    //         std::string filepath = tilemap_filename;
    //         if (filepath.find("Assets/tilemap/") != 0) {
    //           filepath = Engine::Path("Assets/tilemap").join(filepath);
    //         }
    //         Engine::FileIO::write_string_to_file(filepath, prefab_array_json);

    //         show_save_as_popup = false;
    //         ImGui::CloseCurrentPopup();
    //       }
    //     }

    //     ImGui::SameLine();

    //     if (ImGui::Button("Cancel", ImVec2(120, 0))) {
    //       show_save_as_popup = false;
    //       ImGui::CloseCurrentPopup();
    //     }

    //     ImGui::EndPopup();
    //   }

    //   ImGui::SameLine();

    //   if (ImGui::Button("Load...")) {
    //     show_load_popup = true;
    //     ImGui::OpenPopup("Load Tilemap");
    //   }

    //   // Load popup
    //   if (ImGui::BeginPopupModal("Load Tilemap", &show_load_popup, ImGuiWindowFlags_AlwaysAutoResize)) {
    //     ImGui::Text("Enter filename to load:");
    //     ImGui::InputText("##loadfilename", tilemap_filename, sizeof(tilemap_filename));

    //     ImGui::Spacing();

    //     if (ImGui::Button("Load", ImVec2(120, 0))) {
    //       if (strlen(tilemap_filename) > 0) {
    //         // Read from file - ensure Assets/tilemap/ prefix
    //         std::string filepath = tilemap_filename;
    //         if (filepath.find("Assets/tilemap/") != 0) {
    //           filepath = Engine::Path("Assets/tilemap").join(filepath);
    //         }
    //         std::optional<std::string> prefab_array_json_opt = Engine::FileIO::read_string_from_file(filepath);

    //         if (prefab_array_json_opt.has_value()) {
    //           std::string prefab_array_json = prefab_array_json_opt.value();

    //           // Parse json to GenericValue
    //           std::optional<ReflectionLibrary::GenericValue> prefab_array_gen_opt = ReflectionLibrary::generic_value_from_json(prefab_array_json);

    //           if (!prefab_array_gen_opt.has_value()) {
    //             // TODO: Show error message in popup
    //             show_load_popup = false;
    //             ImGui::CloseCurrentPopup();
    //             ImGui::OpenPopup("Load Error");
    //           }
    //           else {
    //             ReflectionLibrary::GenericValue prefab_array_gen = prefab_array_gen_opt.value();

    //             // Clear existing tiles
    //             auto all_entities = Engine::ECS::get_all_entities();
    //             for (auto& entity : all_entities) {
    //               if (entity.has_components<Components::Tile>()) {
    //                 entity.destroy();
    //               }
    //             }
    //             placed_tiles_per_layer.clear();

    //             // Deserialize each prefab and instantiate entity
    //             const auto& prefab_array = prefab_array_gen.getArray();
    //             for (const auto& prefab_gen : prefab_array) {
    //               Engine::ECS::Prefab prefab = Engine::ECS::prefab_from_generic_value(prefab_gen);
    //               Engine::ECS::instantiate_prefab(prefab);
    //             }

    //             show_load_popup = false;
    //             ImGui::CloseCurrentPopup();
    //           }
    //         }
    //         else {
    //           // File doesn't exist or couldn't be read
    //           show_load_popup = false;
    //           ImGui::CloseCurrentPopup();
    //         }
    //       }
    //     }

    //     ImGui::SameLine();

    //     if (ImGui::Button("Cancel", ImVec2(120, 0))) {
    //       show_load_popup = false;
    //       ImGui::CloseCurrentPopup();
    //     }

    //     ImGui::EndPopup();
    //   }

    //   // Load Error popup
    //   if (ImGui::BeginPopupModal("Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    //     ImGui::Text("Failed to load tilemap!");
    //     ImGui::Text("The file may be corrupted or invalid.");
    //     ImGui::Spacing();

    //     if (ImGui::Button("OK", ImVec2(120, 0))) {
    //       ImGui::CloseCurrentPopup();
    //     }

    //     ImGui::EndPopup();
    //   }

    //   ImGui::Unindent();
    // }

    // === TILE SELECTION ===
    if (!loaded_tilemaps.empty()) {
      if (ImGui::CollapsingHeader("Tile Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        ImGui::Text("Tilemaps:");
        ImGui::Separator();

        // Display all submitted atlases
        for (int i = 0; i < static_cast<int>(loaded_tilemaps.size()); ++i) {
          const Engine::Graphics::AtlasEntry& entry = loaded_tilemaps[i];

          ImGui::PushID(i);

          // Atlas name and select button
          bool is_selected = (i == selected_tile.atlas_index);
          if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
          }

          // Skip this atlas if texture handle is null (not properly loaded)
          if (!entry.texture_handle) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ERROR: Atlas %d has null texture!", i);
            if (ImGui::Button("Remove Broken Atlas", ImVec2(-1, 0))) {
              loaded_tilemaps.erase(loaded_tilemaps.begin() + i);
              if (selected_tile.atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
                selected_tile.atlas_index = static_cast<int>(loaded_tilemaps.size()) - 1;
              }
              save_loaded_tilemaps_to_project_settings();
            }
            ImGui::PopID();
            ImGui::Separator();
            continue;
          }

          // Create Components wrapper types for TextureAtlasPreview widget
          Components::Texture temp_texture;
          temp_texture.texture_handle = entry.texture_handle;

          Components::TextureAtlas temp_atlas;
          temp_atlas.subTextures = entry.subTextures;

          // Set the drawIndex to the currently selected tile if this is the selected atlas
          if (is_selected) {
            temp_atlas.drawIndex = selected_tile.tile_index;
          }
          else {
            temp_atlas.drawIndex = -1; // No tile selected in this atlas
          }

          // Calculate rows and cols from UV coordinates
          int cols = 1;
          int rows = 1;

          if (!entry.subTextures.empty()) {
            // Get the first tile's UV width to determine column count
            const auto& first_coords = entry.subTextures[0].tex_coords;
            float tile_uv_width = std::abs(first_coords[1].x - first_coords[0].x);
            float tile_uv_height = std::abs(first_coords[0].y - first_coords[3].y);

            if (tile_uv_width > 0.0f) {
              cols = static_cast<int>(std::round(1.0f / tile_uv_width));
            }
            if (tile_uv_height > 0.0f) {
              rows = static_cast<int>(std::round(1.0f / tile_uv_height));
            }
          }

          // Calculate preview height to make tiles uniform size (64x64)
          const float desired_tile_size = 64.0f;
          float preview_height = desired_tile_size * rows;

          ImGuiWidgets::TextureAtlasPreview(
            temp_texture,
            temp_atlas,
            preview_height,
            [i](size_t tile_idx) {
              selected_tile.atlas_index = i;
              selected_tile.tile_index = static_cast<int>(tile_idx);
            }
          );

          // Remove button
          if (ImGui::Button("Remove Atlas", ImVec2(-1, 0))) {
            loaded_tilemaps.erase(loaded_tilemaps.begin() + i);
            if (selected_tile.atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
              selected_tile.atlas_index = static_cast<int>(loaded_tilemaps.size()) - 1;
            }

            // Save to project settings
            save_loaded_tilemaps_to_project_settings();
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

    // === STATISTICS ===
    if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Indent();

      // Count total tiles across all layers
      size_t total_tiles = 0;
      size_t selected_layer_tiles = 0;

      for (auto& entity : Engine::ECS::get_entities_with<Components::TileCollider>()) {
        total_tiles++;
        if (entity.is_on_layer(selected_layer)) {
          selected_layer_tiles++;
        }
      }

      ImGui::Text("Total placed tiles: %zu", total_tiles);
      ImGui::Text("Tiles on '%s' layer: %zu", selected_layer.c_str(), selected_layer_tiles);

      Math::Vec2D mouse_world_pos = Engine::Input::get_mouse_world_position();
      Math::Vec2D snapped_pos = TilemapManager::snap_to_grid(mouse_world_pos);

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

      ImGui::Spacing();
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Keyboard Shortcuts:");
      ImGui::BulletText("B - Paintbrush tool");
      ImGui::BulletText("U - Box Fill tool");
      ImGui::BulletText("I - Picker tool");
      ImGui::BulletText("D - Eraser tool");
      ImGui::BulletText("L - Toggle dim non-selected layers");
      ImGui::BulletText("Ctrl - Temporary Picker (while painting)");
      ImGui::BulletText("Shift - Temporary Eraser (while painting)");

      ImGui::Unindent();
    }

    if (!enable_tilemap_mode) {
      ImGui::EndDisabled();
      ImGui::PopStyleVar();
    }

    ImGui::End();
  }

  void draw_overlay() {
    // === DRAW GRID OVERLAY ===
    // Only draw grid when tilemap mode is enabled
    if (!enable_tilemap_mode) {
      return;
    }

    // Get grid settings from TilemapManager
    float grid_snap = TilemapManager::get_grid_size();
    Math::Vec2D grid_offset = TilemapManager::get_grid_offset();

    // Get screen bounds in world space using the helper functions
    auto virtual_res = Engine::Graphics::get_virtual_resolution();

    // Convert the four corners of the screen from virtual to world space
    Math::Vec2D top_left_world = Engine::Graphics::virtual_to_world(Math::Vec2D{ 0.0f, 0.0f });
    Math::Vec2D bottom_right_world = Engine::Graphics::virtual_to_world(Math::Vec2D{ virtual_res.x, virtual_res.y });

    float left = top_left_world.x;
    float right = bottom_right_world.x;
    float top = top_left_world.y;
    float bottom = bottom_right_world.y;

    // Calculate grid line positions aligned to grid_offset
    float start_x = std::floor((left - grid_offset.x) / grid_snap) * grid_snap + grid_offset.x;
    float start_y = std::floor((top - grid_offset.y) / grid_snap) * grid_snap + grid_offset.y;

    Math::Vec4D grid_color = Math::Vec4D{ 1.0f, 1.0f, 1.0f, 0.3f }; // White with transparency

    // Draw vertical lines (loop from left until right)
    for (float x = start_x; x <= right; x += grid_snap) {
      Engine::Graphics::draw_line(
        Math::Vec2D{ x, top },
        Math::Vec2D{ x, bottom },
        1.0f,
        grid_color
      );
    }

    // Draw horizontal lines (loop from top until bottom)
    for (float y = start_y; y <= bottom; y += grid_snap) {
      Engine::Graphics::draw_line(
        Math::Vec2D{ left, y },
        Math::Vec2D{ right, y },
        1.0f,
        grid_color
      );
    }

    // Draw box fill preview if dragging
    if (box_fill_dragging) {
      int start_grid_x, start_grid_y, end_grid_x, end_grid_y;
      TilemapManager::world_to_grid(box_fill_start, start_grid_x, start_grid_y);
      TilemapManager::world_to_grid(box_fill_end, end_grid_x, end_grid_y);

      // Ensure start is less than end
      if (start_grid_x > end_grid_x) std::swap(start_grid_x, end_grid_x);
      if (start_grid_y > end_grid_y) std::swap(start_grid_y, end_grid_y);

      // Calculate box dimensions in world space
      Math::Vec2D box_start_world = TilemapManager::grid_to_world(start_grid_x, start_grid_y);
      Math::Vec2D box_end_world = TilemapManager::grid_to_world(end_grid_x, end_grid_y);

      Math::Vec2D box_center = {
        (box_start_world.x + box_end_world.x) * 0.5f,
        (box_start_world.y + box_end_world.y) * 0.5f
      };
      Math::Vec2D box_size = {
        (end_grid_x - start_grid_x + 1) * grid_snap,
        (end_grid_y - start_grid_y + 1) * grid_snap
      };

      // Draw semi-transparent box preview
      Engine::Graphics::draw_rect(
        box_center,
        box_size,
        0.0f,
        Math::Vec4D{ 0.0f, 1.0f, 0.0f, 0.3f }
      );

      // Draw border for better visibility
      float half_width = box_size.x * 0.5f;
      float half_height = box_size.y * 0.5f;
      Math::Vec2D top_left = { box_center.x - half_width, box_center.y - half_height };
      Math::Vec2D top_right = { box_center.x + half_width, box_center.y - half_height };
      Math::Vec2D bottom_left = { box_center.x - half_width, box_center.y + half_height };
      Math::Vec2D bottom_right = { box_center.x + half_width, box_center.y + half_height };

      Math::Vec4D border_color = Math::Vec4D{ 0.0f, 1.0f, 0.0f, 0.8f };
      Engine::Graphics::draw_line(top_left, top_right, 2.0f, border_color);
      Engine::Graphics::draw_line(top_right, bottom_right, 2.0f, border_color);
      Engine::Graphics::draw_line(bottom_right, bottom_left, 2.0f, border_color);
      Engine::Graphics::draw_line(bottom_left, top_left, 2.0f, border_color);
    }
    // Highlight hovered cell based on current tool (only when not box filling)
    else {
      auto world_pos_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

      if (world_pos_opt.has_value()) {
        Math::Vec2D world_pos = world_pos_opt.value();
        Math::Vec2D grid_pos = TilemapManager::snap_to_grid(world_pos);

        // Different highlight colors for different tools
        Math::Vec4D highlight_color;
        if (current_tool == ToolMode::Paintbrush || current_tool == ToolMode::BoxFill) {
          highlight_color = Math::Vec4D{ 0.0f, 1.0f, 0.0f, 0.4f }; // Green for painting
        }
        else if (current_tool == ToolMode::Eraser) {
          highlight_color = Math::Vec4D{ 1.0f, 0.0f, 0.0f, 0.4f }; // Red for erasing
        }
        else if (current_tool == ToolMode::Picker) {
          highlight_color = Math::Vec4D{ 0.0f, 0.5f, 1.0f, 0.4f }; // Blue for picker
        }

        Engine::Graphics::draw_rect(
          grid_pos,
          Math::Vec2D{ grid_snap, grid_snap },
          0.0f,
          highlight_color
        );
      }
    }
  }

  bool is_tilemap_mode_enabled()
  {
    return enable_tilemap_mode;
  }

  std::string get_selected_layer()
  {
    return selected_layer;
  }

  bool is_dim_non_selected_layers_enabled()
  {
    return dim_non_selected_layers;
  }
} // namespace TilemapPanel
