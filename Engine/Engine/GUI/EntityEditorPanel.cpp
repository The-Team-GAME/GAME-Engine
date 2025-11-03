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
#include "../Path.h"
#include "../Resources.h"
#include <filesystem>

void EntityEditorPanelHelpers::render_component_tree(const std::string& component_name, Engine::ECS::Entity& entity, Engine::ECS::IComponentArray* array)
{
  if (ImGui::TreeNode(component_name.c_str())) {
    auto component_data = array->to_generic_value(entity.get_id());
    bool modified = false;

    ImGuiWidgets::render_field(component_name, component_data, modified, entity);

    // Write back changes if modified
    if (modified) {
      //std::cout << "Component " << component_name << " modified on Entity " << entity.get_id() << std::endl;
      array->set_component_from_generic_value(entity.get_id(), component_data);
    }

    ImGui::Separator();

    std::string remove_label = "Remove##" + component_name;
    if (ImGui::Button(remove_label.c_str())) {
      auto& containers = Engine::ECS::Internal::get_containers();
      Engine::ECS::EntityID entityId = entity.get_id();

      containers.components_to_remove.emplace_back(entityId, array->get_type_id());
    }

    ImGui::TreePop();
  }
}

void EntityEditorPanel()
{
  static s32 selected_entity_id = -1;
  static char entitySearchBuffer[256] = "";

  ImGui::Begin("Entity Editor");

  // Section 1: Entity List
  if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Total Entities: %zu", Engine::ECS::get_entity_count());

    // Search/Filter
    ImGui::InputText("Search", entitySearchBuffer, 256);

    ImGui::Separator();

    // Entity list with selection
    ImGui::BeginChild("EntityList", ImVec2(0, 200), true);

    auto allEntities = Engine::ECS::get_all_entities();
    for (const auto& entity : allEntities)
    {
      bool isSelected = (selected_entity_id == static_cast<int>(entity.get_id()));
      std::string label = "Entity " + std::to_string(entity.get_id());

      if (ImGui::Selectable(label.c_str(), isSelected))
      {
        selected_entity_id = static_cast<int>(entity.get_id());
      }
    }

    ImGui::EndChild();

    // Create new entity button
    if (ImGui::Button("Create New Entity"))
    {
      auto newEntity = Engine::ECS::create_entity();
      selected_entity_id = static_cast<int>(newEntity.get_id());
    }

    ImGui::SameLine();

    // Delete selected entity
    if (selected_entity_id != -1 && ImGui::Button("Delete Selected"))
    {
      Engine::ECS::destroy_entity(selected_entity_id);
      selected_entity_id = -1;
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
            // Add default-initialized component
            array->add_default_component(entity.get_id());
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

  ImGui::End();
}

