//-----------------------------------------------------------------------------------
// file:    LevelPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the LevelPanel ImGui interface for managing game levels
//          including saving, loading, and component-based filtering.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "LevelPanel.h"
#include "../FileIO/LevelFileIO.h"
#include "../Path.h"
#include "../Logger.h"
#include <unordered_map>

void LevelPanel() {
  ImGui::Begin("Level Manager");

  static char level_name[128] = "NewLevel";
  static char level_path[256] = "levels/level.json";
  static std::unordered_map<std::string, bool> level_component_filter;
  static bool level_filter_initialized = false;
  static bool use_component_filter = false;

  // Initialize component filter map
  if (!level_filter_initialized) {
    auto& containers = Engine::ECS::Internal::get_containers();
    for (auto& [type_id, array] : containers.component_arrays) {
      std::string component_name = array->get_type_name();
      if (component_name != "Unknown" && array->is_reflectable()) {
        level_component_filter[component_name] = false;
      }
    }
    level_filter_initialized = true;
  }

  // Level info section
  if (ImGui::CollapsingHeader("Level Info", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::InputText("Level Name", level_name, sizeof(level_name));
    ImGui::InputText("File Path", level_path, sizeof(level_path));
    ImGui::TextWrapped("Path is relative to Assets/ folder");
  }

  ImGui::Separator();

  // Save section
    // Component filter section
  ImGui::Checkbox("Filter by Components", &use_component_filter);

  if (use_component_filter) {
    ImGui::Text("Only save entities with any of:");
    ImGui::Indent();

    // Component checkboxes
    auto& containers = Engine::ECS::Internal::get_containers();
    int checkbox_count = 0;
    for (auto& [type_id, array] : containers.component_arrays) {
      std::string component_name = array->get_type_name();
      if (component_name == "Unknown" || !array->is_reflectable()) {
        continue;
      }

      if (checkbox_count > 0 && checkbox_count % 3 == 0) {
        // New row after 3 checkboxes
      }
      else if (checkbox_count > 0) {
        ImGui::SameLine();
      }

      ImGui::Checkbox(component_name.c_str(), &level_component_filter[component_name]);
      checkbox_count++;
    }

    ImGui::Unindent();

    // Count matching entities
    size_t matching_count = 0;
    auto all_entities = Engine::ECS::get_all_entities();
    for (auto& entity : all_entities) {
      bool matches = false;
      for (const auto& [comp_name, is_selected] : level_component_filter) {
        if (is_selected) {
          entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray*) {
            if (type_name == comp_name) {
              matches = true;
            }
            });
          if (matches) {
            break;
          }
        }
      }
      if (matches) {
        matching_count++;
      }
    }

    ImGui::Text("Entities to save: %zu", matching_count);
  }
  else {
    ImGui::Text("Entities to save: %zu (all)", Engine::ECS::get_entity_count());
  }

  ImGui::Separator();

  if (ImGui::Button("Save Level", ImVec2(-1, 0))) {
    std::string full_path = Engine::Path("Assets").join(level_path).to_string();

    if (use_component_filter) {
      // Create filtered level manually
      Engine::FileIO::Level level(level_name);

      // Collect entities matching the filter
      auto all_entities = Engine::ECS::get_all_entities();
      for (auto& entity : all_entities) {
        bool matches = false;
        for (const auto& [comp_name, is_selected] : level_component_filter) {
          if (is_selected) {
            entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray*) {
              if (type_name == comp_name) {
                matches = true;
              }
              });
            if (matches) {
              break;
            }
          }
        }
        if (matches) {
          level.entities.push_back(entity);
        }
      }

      Log::info("Saving " + std::to_string(level.entities.size()) + " filtered entities to level: " + level_name);
      if (Engine::FileIO::serialize_level_to_json_file(full_path, level)) {
        Log::info("Level saved successfully!");
      }
    }
    else {
      // Save all entities
      if (Engine::FileIO::save_current_world_as_level(full_path, level_name)) {
        Log::info("Level saved successfully!");
      }
    }
  }

  ImGui::Separator();

  // Load section
  static bool clear_existing = true;
  ImGui::Checkbox("Clear existing entities before loading", &clear_existing);

  ImGui::Separator();

  if (ImGui::Button("Load Level", ImVec2(-1, 0))) {
    std::string full_path = Engine::Path("Assets").join(level_path).to_string();
    if (Engine::FileIO::load_level_into_world(full_path, clear_existing)) {
      Log::info("Level loaded successfully!");
    }
  }

  ImGui::End();
}
