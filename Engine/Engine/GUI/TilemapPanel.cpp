//-----------------------------------------------------------------------------------
// file:    TilemapPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
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

namespace TilemapPanel {
  // Internal state
  namespace {


    std::vector<Components::AtlasEntry> loaded_tilemaps;
    int selected_atlas_index = -1;
    int selected_tile_index = 0;

    // Current preview (before submission)
    Components::Texture preview_texture;
    Components::TextureAtlas preview_atlas;
    int atlas_rows = 1;
    int atlas_cols = 1;

    // Painting state
    bool enable_tilemap_mode = false;
    bool painting_enabled = false;
    bool erasing_enabled = false;
    float grid_snap = 64.0f;
    float prev_grid_snap = 64.0f;
    Math::Vec2D grid_offset = { 0.0f, 0.0f };
    Math::Vec2D prev_grid_offset = { 0.0f, 0.0f };

    // Layer selection
    std::string selected_layer = "default";

    // Track placed tiles to prevent duplicates (per layer)
    std::unordered_map<std::string, std::unordered_set<std::string>> placed_tiles_per_layer;
  }

  // Helper functions

  // Doesnt actually save to file yet, just updates project settings in memory
  // TODO: rename to be more accurate
  void save_loaded_tilemaps_to_project_settings() {
    auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get_mutable();
    project_settings.loaded_tilemaps = loaded_tilemaps;
    // if (!Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::save("Assets/settings/project_settings.json")) {
    //   Log::warning("Failed to save project settings after updating tilemaps");
    // }
    Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::ProjectSettings()); // Mark dirty for saving later
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
        if (transform && entity.is_on_layer(selected_layer)) {
          Math::Vec2D entity_grid_pos = snap_to_grid(transform->position);
          std::string entity_key = get_position_key(entity_grid_pos);

          if (entity_key == pos_key) {
            entity.destroy();
            placed_tiles_per_layer[selected_layer].erase(pos_key);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
            break;
          }
        }
      }
    }
  }

  void update_all_tile_dimensions() {
    auto all_entities = Engine::ECS::get_all_entities();

    placed_tiles_per_layer.clear();

    for (auto& entity : all_entities) {
      if (entity.has_component<Components::Tile>()) {
        auto* transform = entity.get_component<Components::Transform2D>();
        if (transform) {
          Math::Vec2D new_pos = snap_to_grid(transform->position);
          transform->position = new_pos;
          transform->scale = Math::Vec2D{ grid_snap, grid_snap };

          std::string pos_key = get_position_key(new_pos);

          // Track this tile for each layer it's on
          auto layers = entity.get_layers();
          for (Engine::ECS::LayerID layer_id = 0; layer_id < Engine::ECS::MAX_LAYERS; ++layer_id) {
            if (layers.test(layer_id)) {
              std::string layer_name = Engine::ECS::get_layer_name(layer_id);
              if (!layer_name.empty()) {
                placed_tiles_per_layer[layer_name].insert(pos_key);
              }
            }
          }
        }
      }
    }
  }

  void place_tile_at_position(const Math::Vec2D& world_pos) {
    // Check if we have a valid atlas selected
    if (selected_atlas_index < 0 || selected_atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
      return;
    }

    const Components::AtlasEntry& atlas_entry = loaded_tilemaps[selected_atlas_index];

    if (!atlas_entry.texture.texture_handle ||
      atlas_entry.atlas.subTextures.empty() ||
      selected_tile_index < 0 ||
      selected_tile_index >= static_cast<int>(atlas_entry.atlas.subTextures.size())) {
      return;
    }

    Math::Vec2D grid_pos = snap_to_grid(world_pos);
    std::string pos_key = get_position_key(grid_pos);

    // Get the placed tiles set for the current layer
    auto& placed_tiles = placed_tiles_per_layer[selected_layer];

    // If a tile already exists at this position on this layer, delete it first (overwrite behavior)
    if (placed_tiles.find(pos_key) != placed_tiles.end()) {
      // Find and destroy the existing tile entity 
      for (auto& entity : Engine::ECS::get_entities_with<Components::Tile>()) {
        // TODO: This is very inefficient, implement hashing for Transform2D so we dont have to loop all entities 
        auto* transform = entity.get_component<Components::Transform2D>();
        if (transform && entity.is_on_layer(selected_layer)) {
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

    // Special do and undo for placing tile
    auto tile_id = std::make_shared<Engine::ECS::EntityID>(0); // Use 0 or your engine's invalid ID

    auto place_tile_action = std::make_unique<Engine::Action>(
      // --- DO ---
      [tile_id, grid_pos, atlas_entry,
      grid_snap_value = grid_snap,
      selected_tile_index_value = selected_tile_index,
      selected_layer_value = selected_layer]()
      {
        auto tile = Engine::ECS::create_entity()
          .add_component<Components::Tile>()
          .add_component<Components::Transform2D>(grid_pos, 0.0f,
            Math::Vec2D{ grid_snap_value, grid_snap_value })
          .add_component<Components::Texture>(atlas_entry.texture)
          .add_component<Components::TextureAtlas>(atlas_entry.atlas)
          .add_component<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);

        tile.get_component<Components::TextureAtlas>()->drawIndex =
          selected_tile_index_value;

        tile.clear_layers();
        tile.add_to_layer(selected_layer_value);

        *tile_id = tile.get_id();   // always update stored ID
      },

      // --- UNDO ---
      [tile_id]()
      {
        if (*tile_id != Engine::ECS::INVALID_ENTITY)
        {
          Engine::ECS::destroy_entity(*tile_id);
        }
      }
    );

    Engine::UndoManager::execute(std::move(place_tile_action));


    placed_tiles.insert(pos_key);

    // Mark level as dirty for saving later
    Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
  }

  // Public interface
  void init() {
    const auto& project_settings = Engine::FileIO::SettingsManager<Engine::FileIO::ProjectSettings>::get();
    loaded_tilemaps = project_settings.loaded_tilemaps;
  }

  void update() {
    // === HANDLE PAINTING IN WORLD ===
    // Only allow painting/erasing when tilemap mode is enabled
    auto world_pos_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

    if (enable_tilemap_mode) {
      if (!world_pos_opt.has_value()) {
        /// Empty by design: if mouse is outside viewport, do nothing
      }
      else if (painting_enabled && Engine::Input::is_held(Engine::KeyCode::MouseLeft)) {
        Math::Vec2D world_pos = world_pos_opt.value();
        place_tile_at_position(world_pos);
      }
      else if (erasing_enabled && Engine::Input::is_held(Engine::KeyCode::MouseLeft)) {
        Math::Vec2D world_pos = world_pos_opt.value();
        Math::Vec2D grid_pos = snap_to_grid(world_pos);
        delete_tile_at_position(grid_pos);
      }
    }

    // Update tile colors based on selected layer (only when tilemap mode is enabled)
    if (enable_tilemap_mode) {
      darken_non_selected_layer_tiles();
    }
    else {
      // Reset all tiles to full brightness when tilemap mode is disabled
      for (auto& entity : Engine::ECS::get_entities_with<Components::Tile>()) {
        auto* color = entity.get_component<Components::Color>();
        if (!color) continue;

        color->r = 1.0f;
        color->g = 1.0f;
        color->b = 1.0f;
        color->a = 1.0f;
      }
    }


  }


  void draw_panel() {
    ImGui::Begin("Tilemap Editor");

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Tilemap Painting");
    ImGui::Separator();

    // === TILEMAP MODE TOGGLE ===
    ImGui::Checkbox("Enable Tilemap Mode", &enable_tilemap_mode);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Toggle grid & editing)");

    if (!enable_tilemap_mode) {
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::BeginDisabled();
    }

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
              Components::AtlasEntry entry;
              entry.texture = preview_texture;
              entry.atlas = preview_atlas;
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
              Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
            }
          }
          placed_tiles_per_layer.clear();
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
          const Components::AtlasEntry& entry = loaded_tilemaps[i];

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

          // Calculate rows and cols from UV coordinates
          int cols = 1;
          int rows = 1;

          if (!entry.atlas.subTextures.empty()) {
            // Get the first tile's UV width to determine column count
            const auto& first_coords = entry.atlas.subTextures[0].tex_coords;
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
              selected_atlas_index = i;
              selected_tile_index = static_cast<int>(tile_idx);
            }
          );

          // Remove button
          if (ImGui::Button("Remove Atlas", ImVec2(-1, 0))) {
            loaded_tilemaps.erase(loaded_tilemaps.begin() + i);
            if (selected_atlas_index >= static_cast<int>(loaded_tilemaps.size())) {
              selected_atlas_index = static_cast<int>(loaded_tilemaps.size()) - 1;
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

      size_t total_tiles = 0;
      for (const auto& [layer, tiles] : placed_tiles_per_layer) {
        total_tiles += tiles.size();
      }
      ImGui::Text("Total placed tiles: %zu", total_tiles);
      ImGui::Text("Tiles on '%s' layer: %zu", selected_layer.c_str(), placed_tiles_per_layer[selected_layer].size());

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

    // Highlight hovered layer tiles
    if ((painting_enabled || erasing_enabled)) {
      auto world_pos_opt = Engine::Input::get_mouse_world_position_if_in_viewport();

      if (!world_pos_opt.has_value()) {
        return;
      }

      Math::Vec2D world_pos = world_pos_opt.value();
      Math::Vec2D grid_pos = snap_to_grid(world_pos);

      Math::Vec4D highlight_color = painting_enabled ?
        Math::Vec4D{ 0.0f, 1.0f, 0.0f, 0.4f } :
        Math::Vec4D{ 1.0f, 0.0f, 0.0f, 0.4f };

      Engine::Graphics::draw_rect(
        grid_pos,
        Math::Vec2D{ grid_snap, grid_snap },
        0.0f,
        highlight_color
      );
    }
  }

  void darken_non_selected_layer_tiles() {
    // Only darken when tilemap mode is enabled
    if (!enable_tilemap_mode) {
      // Reset all tiles to full brightness when tilemap mode is disabled
      for (auto& entity : Engine::ECS::get_entities_with<Components::Tile>()) {
        auto* color = entity.get_component<Components::Color>();
        if (!color) continue;

        color->r = 1.0f;
        color->g = 1.0f;
        color->b = 1.0f;
        color->a = 1.0f;
      }
      return;
    }

    // Get the selected layer ID
    Engine::ECS::LayerID selected_layer_id = Engine::ECS::get_layer_id(selected_layer);
    if (selected_layer_id == Engine::ECS::INVALID_LAYER) {
      return;
    }

    // Iterate through all tile entities
    for (auto& entity : Engine::ECS::get_entities_with<Components::Tile>()) {
      auto* color = entity.get_component<Components::Color>();
      if (!color) continue;

      // Check if entity is on the selected layer
      bool on_selected_layer = entity.is_on_layer(selected_layer_id);

      if (on_selected_layer) {
        // Reset to full brightness for tiles on selected layer
        color->r = 1.0f;
        color->g = 1.0f;
        color->b = 1.0f;
        color->a = 1.0f;
      }
      else {
        // Darken tiles on other layers
        color->r = 0.3f;
        color->g = 0.3f;
        color->b = 0.3f;
        color->a = 1.0f;
      }
    }
  }
} // namespace TilemapPanel
