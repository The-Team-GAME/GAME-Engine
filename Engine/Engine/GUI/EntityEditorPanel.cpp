//-----------------------------------------------------------------------------------
// file:    EntityEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the EntityEditorPanel ImGui interface for runtime
//          entity and component manipulation. Uses reflection to automatically
//          generate component editor fields.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "EntityEditorPanel.h"
#include "widgets/ReflectionFieldWidgets.h"
#include "../FileIO/PrefabsFileIO.h"
#include "../FileIO/LevelFileIO.h"
#include "../Path.h"
#include "../Resources.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "../Components/SpatialComponents.h"
#include "../Components/UtilityComponents.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../UndoManager/UndoManager.h"

namespace {
  // Shared state for entity selection
  s32 selected_entity_id = -1;
  bool selection_initialized = false;

  // Event listener to sync selection from other sources (like Gizmo system)
  void init_selection_listener() {
    if (selection_initialized) return;
    selection_initialized = true;

    Engine::EventManager::add_event_listener<Engine::Events::SelectEntityEvent>(
      [](const Engine::Events::SelectEntityEvent& event) {
        selected_entity_id = static_cast<s32>(event.entity_id);
      }
    );
  }
}

void EntityEditorPanelHelpers::render_component_tree(const std::string& component_name, Engine::ECS::Entity& entity, Engine::ECS::IComponentArray* array)
{
  if (ImGui::TreeNode(component_name.c_str())) {
    auto component_data = array->to_generic_value(entity.get_id());
    bool modified = false;

    ImGuiWidgets::render_field(component_name, component_data, modified, entity);

    // Undo/redo for editing component
    if (modified) {
      auto prev_value = array->to_generic_value(entity.get_id());
      auto new_value = component_data;

      auto undo_action = std::make_unique<Engine::Action>(
        // Do lambda: apply new value
        [array, entity_id = entity.get_id(), new_value]() {
          array->set_component_from_generic_value(entity_id, new_value);
        },
        // Undo lambda: restore previous value
        [array, entity_id = entity.get_id(), prev_value]() {
          array->set_component_from_generic_value(entity_id, prev_value);
        }
      );
      Engine::UndoManager::add(std::move(undo_action));

      array->set_component_from_generic_value(entity.get_id(), component_data);
    }

    ImGui::Separator();

    std::string remove_label = "Remove##" + component_name;
    if (ImGui::Button(remove_label.c_str())) {
      auto prev_value = array->to_generic_value(entity.get_id());

      auto undo_action = std::make_unique<Engine::Action>(
        // Do lambda: remove component
        [entity_id = entity.get_id(), type_id = array->get_type_id()]() {
          auto& containers = Engine::ECS::Internal::get_containers();
          containers.components_to_remove.emplace_back(entity_id, type_id);
        },
        // Undo lambda: restore component
        [array, entity_id = entity.get_id(), prev_value]() {
          array->add_default_component(entity_id);
          array->set_component_from_generic_value(entity_id, prev_value);
        }
      );
      Engine::UndoManager::add(std::move(undo_action));

      auto& containers = Engine::ECS::Internal::get_containers();
      containers.components_to_remove.emplace_back(entity.get_id(), array->get_type_id());
    }

    ImGui::TreePop();
  }
}

void EntityEditorPanel()
{
  static char entitySearchBuffer[256] = "";
  static bool hierarchy_mode = false; // Toggle between flat list and hierarchy view

  // Initialize event listener on first call
  init_selection_listener();

  ImGui::Begin("Entity Editor");

  // Mode toggle button at the top
  if (hierarchy_mode) {
    if (ImGui::Button("Switch to Flat View")) {
      hierarchy_mode = false;
    }
  }
  else {
    if (ImGui::Button("Switch to Hierarchy View")) {
      hierarchy_mode = true;
    }
  }

  ImGui::Separator();

  // Section 1: Entity List (changes based on mode)
  if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Total Entities: %zu", Engine::ECS::get_entity_count());

    // Search/Filter
    ImGui::InputText("Search", entitySearchBuffer, 256);

    ImGui::Separator();

    // Entity list with selection - changes based on mode
    ImGui::BeginChild("EntityList", ImVec2(0, 200), true);

    auto allEntities = Engine::ECS::get_all_entities();

    if (hierarchy_mode) {
      // Hierarchy view - group by layers
      std::unordered_map<std::string, std::vector<Engine::ECS::Entity>> entities_by_layer;

      for (auto& entity : allEntities) {
        // Skip entities with Internal component
        if (entity.has_component<Components::Internal>()) {
          continue;
        }

        Engine::ECS::LayerSignature layers = entity.get_layers();

        // Get layer names
        std::vector<std::string> layer_names;
        for (Engine::ECS::LayerID layer_id = 0; layer_id < Engine::ECS::MAX_LAYERS; ++layer_id) {
          if (layers.test(layer_id)) {
            std::string layer_name = Engine::ECS::get_layer_name(layer_id);
            layer_names.push_back(layer_name);
          }
        }

        // If entity has no layers, put it in "default"
        if (layer_names.empty()) {
          layer_names.push_back("default");
        }

        // Add entity to each of its layers
        for (const auto& layer_name : layer_names) {
          entities_by_layer[layer_name].push_back(entity);
        }
      }

      // Display entities grouped by layer
      for (auto& [layer_name, entities] : entities_by_layer) {
        ImGuiTreeNodeFlags layer_flags = ImGuiTreeNodeFlags_DefaultOpen;
        bool layer_open = ImGui::TreeNodeEx(
          (layer_name + " (" + std::to_string(entities.size()) + ")").c_str(),
          layer_flags
        );

        if (layer_open) {
          for (auto& entity : entities) {
            if (!entity.is_valid()) continue;

            // Build entity label
            std::string entity_label = "Entity " + std::to_string(entity.get_id());

            // Try to get a name from common components
            if (entity.has_component<Components::Transform2D>()) {
              entity_label += " (Transform)";
            }

            // Check if this entity is selected
            bool is_selected = (selected_entity_id == static_cast<s32>(entity.get_id()));
            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (is_selected) {
              node_flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(
              (void*)(intptr_t)entity.get_id(),
              node_flags,
              "%s",
              entity_label.c_str()
            );

            // Handle selection
            if (ImGui::IsItemClicked()) {
              selected_entity_id = static_cast<s32>(entity.get_id());
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(entity.get_id());
            }

            // Right-click context menu
            if (ImGui::BeginPopupContextItem()) {
              if (ImGui::MenuItem("Delete Entity")) {
                Engine::ECS::destroy_entity(entity);
                if (selected_entity_id == static_cast<s32>(entity.get_id())) {
                  selected_entity_id = -1;
                  Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
                }
              }
              if (ImGui::MenuItem("Clone Entity")) {
                Engine::ECS::Entity cloned = Engine::ECS::clone_entity(entity);
                selected_entity_id = static_cast<s32>(cloned.get_id());
                Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(cloned.get_id());
              }
              ImGui::EndPopup();
            }
          }
          ImGui::TreePop();
        }
      }
    }
    else {
      // Flat view - simple list
      for (const auto& entity : allEntities)
      {
        // Skip entities with Internal component
        if (entity.has_component<Components::Internal>()) {
          continue;
        }

        bool isSelected = (selected_entity_id == static_cast<int>(entity.get_id()));
        std::string label = "Entity " + std::to_string(entity.get_id());

        if (ImGui::Selectable(label.c_str(), isSelected))
        {
          selected_entity_id = static_cast<int>(entity.get_id());
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(entity.get_id());
        }
      }
    }

    ImGui::EndChild();

    // Create new entity button
    if (ImGui::Button("Create New Entity"))
    {
      auto newEntity = Engine::ECS::create_entity();
      selected_entity_id = static_cast<int>(newEntity.get_id());
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(newEntity.get_id());
    }

    ImGui::SameLine();

    // Delete selected entity
    if (selected_entity_id != -1 && ImGui::Button("Delete Selected"))
    {
      Engine::ECS::destroy_entity(selected_entity_id);
      selected_entity_id = -1;
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
    }
  }

  ImGui::Separator();

  // Section 2: Component Editor for Selected Entity
  if (selected_entity_id != -1)
  {
    auto entity = Engine::ECS::get_entity(selected_entity_id);

    if (!entity.is_valid())
    {
      selected_entity_id = -1;
      ImGui::End();
      return;
    }

    ImGui::Text("Editing Entity: %d", selected_entity_id);
    ImGui::Separator();

    // Add Component Section
    if (ImGui::CollapsingHeader("Add Component"))
    {
      // Get all registered component types from the ECS
      auto& containers = Engine::ECS::Internal::get_containers();

      int button_count = 0;
      for (auto& [type_id, array] : containers.component_arrays) {
        std::string component_name = array->get_type_name();

        // Skip if component type name is unknown or not reflectable
        if (component_name == "Unknown" || !array->is_reflectable()) {
          continue;
        }

        // Check if entity already has this component
        bool has_component = false;
        entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray*) {
          if (type_name == component_name) {
            has_component = true;
          }
          });

        // Create add button
        std::string button_label = "Add " + component_name;

        // Add spacing between buttons (2 per row)
        if (button_count > 0 && button_count % 2 == 0) {
          // New row
        }
        else if (button_count > 0) {
          ImGui::SameLine();
        }

        // Disable button if entity already has this component
        if (has_component) {
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
          ImGui::Button(button_label.c_str());
          ImGui::PopStyleVar();
        }
        else {
          if (ImGui::Button(button_label.c_str())) {
            // Add default-constructed component to entity by name
            if (entity.add_component_by_name(component_name)) {
              Log::info("Added component '" + component_name + "' to entity " + std::to_string(entity.get_id()));
            }
            else {
              Log::error("Failed to add component '" + component_name + "' to entity " + std::to_string(entity.get_id()));
            }
          }
        }

        button_count++;
      }

      if (button_count == 0) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
          "No reflectable components available");
      }
    }

    ImGui::Separator();

    // Edit Components Section
    if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
    {
      // Automatically iterate through ALL components the entity has
      entity.for_each_component([&entity](const std::string& type_name, Engine::ECS::IComponentArray* array) {
        if (!array->is_reflectable()) {
          return; // Skip non-reflectable components
        }

        // First try specialized rendering
        // If not handled, fallback to generic reflection-based rendering
        EntityEditorPanelHelpers::render_component_tree(type_name, entity, array);
        });
    }

    ImGui::Separator();

    // Layer Editor Section
    if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen))
    {
      auto& containers = Engine::ECS::Internal::get_containers();
      auto entity_layers = entity.get_layers();

      // // Display current layers
      // ImGui::Text("Entity is on layers:");

      // bool has_layers = false;
      // for (Engine::ECS::LayerID layer = 0; layer < containers.layer_id_to_name.size(); ++layer) {
      //   if (entity_layers.test(layer)) {
      //     has_layers = true;
      //     const std::string& layer_name = containers.layer_id_to_name[layer];
      //     if (!layer_name.empty()) {
      //       ImGui::BulletText("%s (ID: %d)", layer_name.c_str(), layer);
      //     }
      //   }
      // }

      // if (!has_layers) {
      //   ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  (No layers)");
      // }

      // ImGui::Separator();

      // Get all registered layers
      ImGui::Text("Available Layers:");

      bool any_layers_registered = false;
      for (Engine::ECS::LayerID layer_id = 0; layer_id < containers.layer_id_to_name.size(); ++layer_id) {
        const std::string& layer_name = containers.layer_id_to_name[layer_id];

        // Skip empty layer names (shouldn't happen, but just in case)
        if (layer_name.empty()) continue;

        any_layers_registered = true;

        bool is_on_layer = entity_layers.test(layer_id);

        if (ImGui::Checkbox((layer_name + " (ID: " + std::to_string(layer_id) + ")").c_str(), &is_on_layer)) {
          if (is_on_layer) {
            entity.add_to_layer(layer_name);
          }
          else {
            entity.remove_from_layer(layer_name);
          }
        }
      }

      if (!any_layers_registered) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  (No layers registered)");
      }

      ImGui::Separator();

      // Add new layer section
      static char new_layer_name[128] = "";
      ImGui::Text("Register New Layer:");
      ImGui::InputText("##NewLayerName", new_layer_name, sizeof(new_layer_name));
      ImGui::SameLine();

      if (ImGui::Button("Register Layer")) {
        if (strlen(new_layer_name) > 0) {
          Engine::ECS::LayerID new_id = Engine::ECS::register_layer(new_layer_name);
          if (new_id != Engine::ECS::INVALID_LAYER) {
            // Clear the input
            new_layer_name[0] = '\0';
          }
        }
      }

      ImGui::Separator();

      // Quick actions
      if (ImGui::Button("Clear All Layers")) {
        entity.clear_layers();
      }
    }

    ImGui::Separator();

    // Clone Entity
    if (ImGui::Button("Clone This Entity"))
    {
      auto clonedEntity = Engine::ECS::clone_entity(entity);
      selected_entity_id = static_cast<int>(clonedEntity.get_id());
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(clonedEntity.get_id());
    }

    ImGui::SameLine();

    // Create Prefab from Entity
    if (ImGui::Button("Create Prefab from Entity"))
    {
      ImGui::OpenPopup("CreatePrefabFromEntity");
    }

    // Create Prefab popup
    static char prefab_filename[128] = "";
    if (ImGui::BeginPopupModal("CreatePrefabFromEntity", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text("Enter prefab filename:");
      ImGui::InputText("##prefabfilename", prefab_filename, sizeof(prefab_filename));
      ImGui::Text(".json");

      ImGui::Separator();

      if (ImGui::Button("Create", ImVec2(120, 0)))
      {
        if (strlen(prefab_filename) > 0)
        {
          std::string filename = std::string(prefab_filename);
          if (filename.find(".json") == std::string::npos) {
            filename += ".json";
          }

          // Create prefab from entity
          auto prefab = Engine::ECS::entity_to_prefab(entity);

          // Save to file
          std::string relative_path = "prefabs/" + filename;
          std::string full_path = Engine::Path(Engine::Resources::resource_path).join(relative_path).to_string();

          // Ensure prefabs directory exists
          std::string prefabs_dir = Engine::Path(Engine::Resources::resource_path).join("prefabs").to_string();
          if (!std::filesystem::exists(prefabs_dir)) {
            std::filesystem::create_directories(prefabs_dir);
          }

          if (Engine::FileIO::serialize_prefab_to_json_file(full_path, prefab))
          {
            // Clear the input and close popup
            prefab_filename[0] = '\0';
            ImGui::CloseCurrentPopup();
            ImGui::OpenPopup("CreatePrefabSuccess");
          }
        }
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0)))
      {
        prefab_filename[0] = '\0';
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    // Create Prefab success popup
    if (ImGui::BeginPopup("CreatePrefabSuccess"))
    {
      ImGui::Text("Prefab created successfully!");
      ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Serialize/Export Entity
    if (ImGui::Button("Export to JSON"))
    {
      auto generic_value = Engine::ECS::entity_to_generic_value(entity);
      std::string json = ReflectionLibrary::generic_value_to_json(generic_value, true);
      ImGui::SetClipboardText(json.c_str());
      ImGui::OpenPopup("Export Success");
    }

    // Export success popup
    if (ImGui::BeginPopup("Export Success"))
    {
      ImGui::Text("Entity JSON copied to clipboard!");
      ImGui::EndPopup();
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
  }

  ImGui::Separator();

  ImGui::End();
}

