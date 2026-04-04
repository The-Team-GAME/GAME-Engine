//-----------------------------------------------------------------------------------
// file:    SelectiveLevelSavePanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the SelectiveLevelSavePanel ImGui interface for
//          saving levels with only selected component types. Provides checkboxes
//          for component selection and custom level serialization logic.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "SelectiveLevelSavePanel.h"
#include "../ECS/ECS.h"
#include "../LevelManager.h"
#include "../FileIO/LevelFileIO.h"
#include "../Resources.h"
#include "../Logger.h"
#include "../Path.h"
#include "../Components/Components.h"
#include "../ECS/Reflection.h"
#include <imgui.h>
#include <unordered_set>
#include <filesystem>

namespace {
  // Track which components are selected for export
  std::unordered_set<std::string> selected_components;

  // File path buffer
  char file_path_buffer[256] = "levels/selective_export.json";

  // Status message
  std::string status_message;
  float status_message_time = 0.0f;

  // Helper function to create a filtered level with only selected components
  Engine::EntityList create_filtered_level(const std::unordered_set<std::string>& component_filter) {
    Engine::EntityList entities;

    Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
      // Skip tile entities and internal entities
      if (entity.has_components<Components::TileCollider, Components::Level>() ||
        entity.has_component<Components::Internal>()) {
        return;
      }

      ReflectionLibrary::GenericValue entity_value = ReflectionLibrary::Object();

      // Serialize layers (always include)
      entity_value["layers"] = Engine::ECS::Serialization::serialize_layers(entity.get_layers());

      // Serialize only selected components
      ReflectionLibrary::Array components_array;

      entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray* array) {
        // Skip non-reflectable components
        if (!array->is_reflectable()) {
          return;
        }

        // Check if this component type is selected
        if (component_filter.find(type_name) != component_filter.end()) {
          ReflectionLibrary::GenericValue component_entry;
          component_entry["type"] = type_name;
          component_entry["data"] = array->to_generic_value(entity.get_id());
          components_array.push_back(component_entry);
        }
        });

      entity_value["components"] = components_array;

      // Serialize prefab instance data if present (always include if exists)
      auto& containers = Engine::ECS::Internal::get_containers();
      Engine::ECS::EntityID entity_id = entity.get_id();
      auto prefab_it = containers.prefab_instances.find(entity_id);
      if (prefab_it != containers.prefab_instances.end()) {
        const auto& instance_data = prefab_it->second;

        ReflectionLibrary::GenericValue prefab_ref = ReflectionLibrary::Object();
        prefab_ref["id"] = instance_data.prefab_id;
        if (!instance_data.prefab_filepath.empty()) {
          prefab_ref["filepath"] = instance_data.prefab_filepath;
        }

        // Serialize field overrides
        ReflectionLibrary::GenericValue overrides_obj = ReflectionLibrary::Object();
        for (const auto& [type_id, field_set] : instance_data.field_overrides) {
          ReflectionLibrary::GenericValue fields_array = ReflectionLibrary::Array();
          for (const auto& field_path : field_set) {
            fields_array.push_back(field_path);
          }
          overrides_obj[std::to_string(type_id)] = fields_array;
        }
        prefab_ref["overrides"] = overrides_obj;

        entity_value["prefab_reference"] = prefab_ref;
      }

      entities.push_back(std::move(entity_value));
      });

    return entities;
  }
}

void SelectiveLevelSavePanel() {
  ImGui::Begin("Selective Level Save");

  ImGui::TextWrapped("Select which component types to include in the exported level file.");
  ImGui::Separator();

  // Component Selection Section
  if (ImGui::CollapsingHeader("Component Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto& containers = Engine::ECS::Internal::get_containers();

    // Collect all reflectable component types
    std::vector<std::string> component_types;
    for (Engine::ECS::ComponentTypeID type_id = 0; type_id < Engine::ECS::MAX_COMPONENTS; ++type_id) {
      auto& array = containers.component_arrays[type_id];
      if (!array) continue;
      std::string component_name = array->get_type_name();

      // Skip unknown or non-reflectable components
      if (component_name == "Unknown" || !array->is_reflectable()) {
        continue;
      }

      component_types.push_back(component_name);
    }

    // Sort for consistent display
    std::sort(component_types.begin(), component_types.end());

    if (component_types.empty()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No reflectable components available");
    }
    else {
      // Quick action buttons
      if (ImGui::Button("Select All")) {
        selected_components.clear();
        for (const auto& type : component_types) {
          selected_components.insert(type);
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Deselect All")) {
        selected_components.clear();
      }

      ImGui::Separator();

      // Display checkboxes in a scrollable region
      ImGui::BeginChild("ComponentList", ImVec2(0, 300), true);

      for (const auto& component_type : component_types) {
        bool is_selected = selected_components.find(component_type) != selected_components.end();

        // Strip "Components::" prefix for cleaner display
        std::string display_name = component_type;
        if (component_type.substr(0, 12) == "Components::") {
          display_name = component_type.substr(12);
        }

        if (ImGui::Checkbox(display_name.c_str(), &is_selected)) {
          if (is_selected) {
            selected_components.insert(component_type);
          }
          else {
            selected_components.erase(component_type);
          }
        }
      }

      ImGui::EndChild();

      ImGui::Text("Selected: %zu / %zu components", selected_components.size(), component_types.size());
    }
  }

  ImGui::Separator();

  // File Path Section
  if (ImGui::CollapsingHeader("Export Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Output File Path:");
    ImGui::InputText("##filepath", file_path_buffer, sizeof(file_path_buffer));
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Relative to resources folder");

    ImGui::Separator();

    // Entity count info
    size_t total_entities = 0;
    Engine::ECS::for_each_entity([&](Engine::ECS::Entity entity) {
      if (!entity.has_components<Components::TileCollider, Components::Level>() &&
        !entity.has_component<Components::Internal>()) {
        total_entities++;
      }
      });

    ImGui::Text("Entities to export: %zu", total_entities);
    ImGui::Text("Component types to include: %zu", selected_components.size());
  }

  ImGui::Separator();

  // Save Button
  if (ImGui::Button("Save Filtered Level", ImVec2(-1, 40))) {
    std::string filepath = std::string(file_path_buffer);

    // Validate filepath
    if (filepath.empty()) {
      status_message = "Error: File path cannot be empty";
      status_message_time = 3.0f;
    }
    else {
      // Auto-append .json extension if not present
      if (filepath.find(".json") == std::string::npos) {
        filepath += ".json";
      }

      // Create filtered level
      Engine::EntityList filtered_entities = create_filtered_level(selected_components);

      // Construct full path
      std::string full_path = Engine::Path(Engine::Resources::resource_path).join(filepath).to_string();

      // Ensure directory exists
      std::filesystem::path path_obj(full_path);
      std::filesystem::path dir = path_obj.parent_path();
      if (!dir.empty() && !std::filesystem::exists(dir)) {
        try {
          std::filesystem::create_directories(dir);
        }
        catch (const std::filesystem::filesystem_error& e) {
          status_message = "Error: Failed to create directory - " + std::string(e.what());
          status_message_time = 5.0f;
          Log::error("Failed to create directory: {}", e.what());
        }
      }

      // Save the level
      if (Engine::FileIO::serialize_level_to_json_file(full_path, filtered_entities)) {
        status_message = "Success: Level saved to " + filepath;
        status_message_time = 3.0f;
        Log::info("Selective level saved: {} ({} entities, {} component types)",
          filepath, filtered_entities.size(), selected_components.size());
      }
      else {
        status_message = "Error: Failed to save level file";
        status_message_time = 3.0f;
        Log::error("Failed to save selective level: {}", filepath);
      }
    }
  }

  // Display status message
  if (status_message_time > 0.0f) {
    status_message_time -= ImGui::GetIO().DeltaTime;

    ImVec4 color = status_message.find("Success") == 0 ?
      ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :
      ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::TextColored(color, "%s", status_message.c_str());
  }

  ImGui::End();
}

