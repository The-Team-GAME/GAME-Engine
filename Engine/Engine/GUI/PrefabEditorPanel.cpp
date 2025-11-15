//-----------------------------------------------------------------------------------
// file:    PrefabEditorPanel.cpp
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file implements the PrefabEditorPanel ImGui interface for runtime
//          prefab manipulation. Uses reflection to automatically generate component
//          editor fields. Supports loading, editing, saving, and instantiating prefabs.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include <imgui.h>
#include "PrefabEditorPanel.h"
#include "widgets/ReflectionFieldWidgets.h"
#include "../FileIO/PrefabsFileIO.h"
#include "../Path.h"
#include "../Resources.h"
#include "../ProjectSaveManager.h"

void PrefabEditorPanelHelpers::render_component_tree(const std::string& component_name, Engine::ECS::Prefab& prefab, Engine::ECS::IComponentArray* array, const std::string& prefab_path)
{
  if (ImGui::TreeNode(component_name.c_str())) {
    auto component_data = array->to_generic_value(0); // Prefabs use entity ID 0
    bool modified = false;

    ImGuiWidgets::render_field(component_name, component_data, modified);

    // Write back changes if modified
    if (modified) {
      array->set_component_from_generic_value(0, component_data);

      // Mark prefab as dirty
      if (!prefab_path.empty()) {
        Engine::ProjectSaveManager::set_dirty(
          Engine::ProjectSaveManager::DirtyStates::Prefab(prefab_path)
        );
      }
    }

    ImGui::Separator();

    std::string remove_label = "Remove##" + component_name;
    if (ImGui::Button(remove_label.c_str())) {
      // Remove component directly from prefab's component storage
      auto prefab_impl = prefab.get_impl();
      if (prefab_impl) {
        auto& storage = prefab_impl->get_component_storage();
        auto& signature = prefab_impl->get_signature();
        auto it = storage.find(array->get_type_id());
        if (it != storage.end()) {
          storage.erase(it);
          signature.reset(array->get_type_id());

          // Mark prefab as dirty
          if (!prefab_path.empty()) {
            Engine::ProjectSaveManager::set_dirty(
              Engine::ProjectSaveManager::DirtyStates::Prefab(prefab_path)
            );
          }
        }
      }
    }

    ImGui::TreePop();
  }
}

Engine::ECS::Prefab PrefabEditorPanelHelpers::get_prefab_from_resources(const std::string& prefab_path)
{
  Engine::ECS::Prefab selected_prefab;

  // Construct full path from relative path
  std::string full_path = Engine::Path(Engine::Resources::resource_path).join(prefab_path);

  auto it = Engine::Resources::prefab_cache.find(full_path);

  if (it != Engine::Resources::prefab_cache.end()) {
    if (auto prefab_ptr = it->second.lock()) {
      selected_prefab = Engine::ECS::Prefab(prefab_ptr);
    }
    else {
      Log::error("PrefabEditorPanel: Prefab expired in cache: " + prefab_path);
    }
  }
  else {
    Log::error("PrefabEditorPanel: Failed to find prefab in cache: " + prefab_path);
  }

  return selected_prefab;
}

void PrefabEditorPanel()
{
  static std::string selected_prefab_path = "";
  static char prefabSearchBuffer[256] = "";

  ImGui::Begin("Prefab Editor");

  // Section 1: Prefab List and Actions
  if (ImGui::CollapsingHeader("Prefabs", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // Search/Filter
    ImGui::InputText("Search", prefabSearchBuffer, 256);

    ImGui::Separator();

    // List available prefab files
    ImGui::BeginChild("PrefabList", ImVec2(0, 200), true);

    // Get prefabs from resource cache
    std::vector<std::pair<std::string, Engine::ECS::Prefab>> prefab_list;

    for (const auto& [path, weak_prefab] : Engine::Resources::prefab_cache) {
      if (auto prefab = weak_prefab.lock()) {
        // Get relative path for display
        std::string relative_path = Engine::Resources::get_relative_path(path);

        // Apply search filter
        if (strlen(prefabSearchBuffer) > 0 && relative_path.find(prefabSearchBuffer) == std::string::npos) {
          continue;
        }

        prefab_list.push_back({ relative_path, Engine::ECS::Prefab(prefab) });
      }
    }

    // Sort by path for consistent ordering
    std::sort(prefab_list.begin(), prefab_list.end(),
      [](const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& [relative_path, prefab] : prefab_list) {
      bool isSelected = (selected_prefab_path == relative_path);

      if (ImGui::Selectable(relative_path.c_str(), isSelected)) {
        selected_prefab_path = relative_path;
      }
    }

    ImGui::EndChild();

    // Create new prefab button
    static char newPrefabNameBuffer[256] = "";
    if (ImGui::Button("Create New Prefab")) {
      ImGui::OpenPopup("Create New Prefab");
      // Clear buffer when opening popup
      newPrefabNameBuffer[0] = '\0';
    }

    // Create New Prefab popup
    if (ImGui::BeginPopup("Create New Prefab"))
    {
      ImGui::Text("Enter prefab name:");
      ImGui::InputText("##NewPrefabName", newPrefabNameBuffer, sizeof(newPrefabNameBuffer));

      ImGui::Separator();

      if (ImGui::Button("Create")) {
        std::string prefab_name = std::string(newPrefabNameBuffer);

        if (!prefab_name.empty()) {
          // Add .json extension if not present
          if (prefab_name.find(".json") == std::string::npos) {
            prefab_name += ".json";
          }

          // Construct the relative path (prefabs folder)
          std::string relative_path = "prefabs/" + prefab_name;

          // Create a new empty prefab
          Engine::ECS::Prefab new_prefab = Engine::ECS::create_prefab();

          // Add to cache without saving to disk
          Engine::Resources::cache_prefab(relative_path, new_prefab, true, true);

          // Mark as dirty in ProjectSaveManager
          Engine::ProjectSaveManager::set_dirty(
            Engine::ProjectSaveManager::DirtyStates::Prefab(relative_path)
          );

          // Select the newly created prefab
          selected_prefab_path = relative_path;

          Log::info("Created new prefab: " + relative_path);
          ImGui::CloseCurrentPopup();
        }
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Instantiate prefab button
    if (ImGui::Button("Instantiate")) {
      Engine::ECS::instantiate_prefab(PrefabEditorPanelHelpers::get_prefab_from_resources(selected_prefab_path));
    }
  }

  ImGui::Separator();

  // Section 2: Prefab Editor for Selected Prefab
  if (!selected_prefab_path.empty())
  {
    if (!selected_prefab_path.empty()) {
      ImGui::Text("Editing: %s", selected_prefab_path.c_str());
    }
    else {
      ImGui::Text("Editing: <New Prefab>");
    }
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

        // Check if prefab already has this component
        bool has_component = false;

        Engine::ECS::Prefab selected_prefab = PrefabEditorPanelHelpers::get_prefab_from_resources(selected_prefab_path);

        selected_prefab.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray*) {
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

        // Disable button if prefab already has this component
        if (has_component) {
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
          ImGui::Button(button_label.c_str());
          ImGui::PopStyleVar();
        }
        else {
          if (ImGui::Button(button_label.c_str())) {
            // Add default-constructed component to prefab by name
            if (selected_prefab.add_component_by_name(component_name)) {
              // Mark prefab as dirty
              Engine::ProjectSaveManager::set_dirty(
                Engine::ProjectSaveManager::DirtyStates::Prefab(selected_prefab_path)
              );
              Log::info("Added component '" + component_name + "' to prefab: " + selected_prefab_path);
            }
            else {
              Log::error("Failed to add component '" + component_name + "' to prefab: " + selected_prefab_path);
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
      Engine::ECS::Prefab selected_prefab = PrefabEditorPanelHelpers::get_prefab_from_resources(selected_prefab_path);

      // Automatically iterate through ALL components the prefab has
      selected_prefab.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray* array) {
        if (!array->is_reflectable()) {
          return; // Skip non-reflectable components
        }

        // Render component tree
        PrefabEditorPanelHelpers::render_component_tree(type_name, selected_prefab, array, selected_prefab_path);
        });
    }

    ImGui::Separator();

    // Layer Editor Section
    if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen))
    {
      Engine::ECS::Prefab selected_prefab = PrefabEditorPanelHelpers::get_prefab_from_resources(selected_prefab_path);

      auto& containers = Engine::ECS::Internal::get_containers();
      auto prefab_layers = selected_prefab.get_layers();

      // Get all registered layers
      ImGui::Text("Available Layers:");

      bool any_layers_registered = false;
      for (Engine::ECS::LayerID layer_id = 0; layer_id < containers.layer_id_to_name.size(); ++layer_id) {
        const std::string& layer_name = containers.layer_id_to_name[layer_id];

        // Skip empty layer names (shouldn't happen, but just in case)
        if (layer_name.empty()) continue;

        any_layers_registered = true;

        bool is_on_layer = prefab_layers.test(layer_id);

        if (ImGui::Checkbox((layer_name + " (ID: " + std::to_string(layer_id) + ")").c_str(), &is_on_layer)) {
          if (is_on_layer) {
            selected_prefab.add_to_layer(layer_name);
          }
          else {
            selected_prefab.remove_from_layer(layer_name);
          }

          // Mark prefab as dirty
          if (!selected_prefab_path.empty()) {
            Engine::ProjectSaveManager::set_dirty(
              Engine::ProjectSaveManager::DirtyStates::Prefab(selected_prefab_path)
            );
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
        selected_prefab.clear_layers();

        // Mark prefab as dirty
        if (!selected_prefab_path.empty()) {
          Engine::ProjectSaveManager::set_dirty(
            Engine::ProjectSaveManager::DirtyStates::Prefab(selected_prefab_path)
          );
        }
      }
    }

    ImGui::Separator();

    // Export to JSON
    if (ImGui::Button("Export to JSON (Clipboard)"))
    {
      Engine::ECS::Prefab selected_prefab = PrefabEditorPanelHelpers::get_prefab_from_resources(selected_prefab_path);

      auto generic_value = Engine::ECS::prefab_to_generic_value(selected_prefab);
      std::string json = ReflectionLibrary::generic_value_to_json(generic_value, true);
      ImGui::SetClipboardText(json.c_str());
      ImGui::OpenPopup("Export Success");
    }

    // Export success popup
    if (ImGui::BeginPopup("Export Success"))
    {
      ImGui::Text("Prefab JSON copied to clipboard!");
      ImGui::EndPopup();
    }

    ImGui::SameLine();

    // // Import from JSON
    // if (ImGui::Button("Import from JSON (Clipboard)"))
    // {
    //   const char* clipboard_text = ImGui::GetClipboardText();
    //   if (clipboard_text && strlen(clipboard_text) > 0) {
    //     try {
    //       auto generic_value_opt = ReflectionLibrary::generic_value_from_json(clipboard_text);
    //       if (generic_value_opt.has_value()) {
    //         auto imported_prefab = Engine::ECS::prefab_from_generic_value(generic_value_opt.value());
    //         if (imported_prefab.is_valid()) {
    //           selected_prefab = imported_prefab;
    //           selected_prefab_path = "";

    //           // Don't mark as dirty since it's a new import without a path
    //           // User needs to create/save it first

    //           ImGui::OpenPopup("Import Success");
    //         }
    //         else {
    //           ImGui::OpenPopup("Import Failed");
    //         }
    //       }
    //       else {
    //         ImGui::OpenPopup("Import Failed");
    //       }
    //     }
    //     catch (...) {
    //       ImGui::OpenPopup("Import Failed");
    //     }
    //   }
    // }

    // Import success popup
    if (ImGui::BeginPopup("Import Success"))
    {
      ImGui::Text("Prefab imported successfully!");
      ImGui::EndPopup();
    }

    // Import failed popup
    if (ImGui::BeginPopup("Import Failed"))
    {
      ImGui::Text("Failed to import prefab from clipboard!");
      ImGui::EndPopup();
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No prefab selected");
  }

  ImGui::End();
}
