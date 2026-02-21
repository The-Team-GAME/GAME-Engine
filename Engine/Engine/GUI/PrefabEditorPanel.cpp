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
#include "../Precompiled.h"
#include "PrefabEditorPanel.h"
#include "widgets/ReflectionFieldWidgets.h"
#include "../UndoManager/UndoManager.h"
#include "../FileIO/PrefabsFileIO.h"
#include "../Path.h"
#include "../Resources.h"
#include "../ProjectSaveManager.h"
#include "../InputManager.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../LevelManager.h"
#include <map>
#include <filesystem>

void PrefabEditorPanelHelpers::render_component_tree(const std::string& component_name, Engine::ECS::Prefab& prefab, Engine::ECS::IComponentArray* array, const std::string& prefab_path)
{
  // Render component tree node directly (namespace grouping handled by caller)
  if (ImGui::TreeNode(component_name.c_str())) {
    // Track the value before any modifications
    static std::unordered_map<std::string, ReflectionLibrary::GenericValue> component_edit_start_values;
    static std::unordered_map<std::string, bool> component_being_edited;
    static std::unordered_map<std::string, bool> component_was_active_last_frame;

    std::string component_key = component_name + "_" + prefab_path;

    auto component_data = array->to_generic_value(0); // Prefabs use entity ID 0
    bool modified = false;

    // Get the type name from the component array for specialized rendering
    std::string type_hint = array->get_type_name();

    // Try specialized rendering first
    bool handled_by_specialization = ImGuiWidgets::render_specialized_field(
      component_name, type_hint, component_data, modified, Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY), &prefab, prefab_path
    );

    // If not specialized, render fields directly (without creating another tree node)
    if (!handled_by_specialization && component_data.isObject()) {
      auto& fields = component_data.getObject();
      for (auto& [field_name, field_value] : fields) {
        ImGuiWidgets::render_field(field_name, field_value.type_hint, field_value, modified, Engine::ECS::Entity(Engine::ECS::INVALID_ENTITY), &prefab, prefab_path);
      }
    }

    // Check if user is currently editing (any widget is active in this component)
    bool is_any_item_active = ImGui::IsAnyItemActive();
    bool was_active_last_frame = component_was_active_last_frame[component_key];

    // Start tracking when editing begins
    if (is_any_item_active && !component_being_edited[component_key]) {
      component_being_edited[component_key] = true;
      component_edit_start_values[component_key] = array->to_generic_value(0);
    }

    // Apply changes in real-time
    if (modified) {
      array->set_component_from_generic_value(0, component_data);
    }

    // Record to undo manager only when editing just finished (was active, now inactive)
    if (component_being_edited[component_key] && was_active_last_frame && !is_any_item_active) {
      auto prev_value = component_edit_start_values[component_key];
      auto new_value = component_data;

      // Only record if the value actually changed
      if (prev_value != new_value) {
        Engine::UndoManager::record(
          // Do lambda: apply new value
          [array, new_value]() {
            array->set_component_from_generic_value(0, new_value);
          },
          // Undo lambda: restore previous value
          [array, prev_value]() {
            array->set_component_from_generic_value(0, prev_value);
          }
        );

        // Save prefab immediately
        if (!prefab_path.empty()) {
          Engine::Resources::save_prefab(prefab_path, prefab, true);

          // Propagate changes to all prefab instances
          if (prefab.is_valid()) {
            Engine::ECS::update_prefab_instances(prefab.get_impl()->get_prefab_id());
          }
        }
      }

      component_being_edited[component_key] = false;
      component_edit_start_values.erase(component_key);
    }

    // Update last frame state
    component_was_active_last_frame[component_key] = is_any_item_active;

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

          // Save prefab immediately
          if (!prefab_path.empty()) {
            Engine::Resources::save_prefab(prefab_path, prefab, true);

            // Propagate changes to all prefab instances
            Engine::ECS::update_prefab_instances(prefab_impl->get_prefab_id());
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
      Log::error("PrefabEditorPanel: Prefab expired in cache: {}", prefab_path);
    }
  }
  else {
    Log::error("PrefabEditorPanel: Failed to find prefab in cache: {}", prefab_path);
  }

  return selected_prefab;
}

namespace {
  // Rename state
  std::string rename_prefab_path;
  char rename_buf[256] = "";
  bool rename_focus_set = false;
  bool open_rename_popup = false;
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

      // Right-click context menu for rename
      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Rename")) {
          rename_prefab_path = relative_path;
          // Extract just the filename without extension for editing
          std::filesystem::path p(relative_path);
          std::string stem = p.stem().string();
          strncpy_s(rename_buf, stem.c_str(), sizeof(rename_buf) - 1);
          rename_focus_set = false;
          open_rename_popup = true;
        }
        ImGui::EndPopup();
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

          // Save prefab immediately
          Engine::Resources::save_prefab(relative_path, new_prefab, true);

          // Select the newly created prefab
          selected_prefab_path = relative_path;

          Log::info("Created new prefab: {}", relative_path);
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
      auto prefab = Engine::Resources::load_prefab(selected_prefab_path);
      if (prefab.is_valid()) {
        auto newEntity = Engine::ECS::instantiate_prefab(prefab);

        // Name the entity after the prefab file (without extension)
        std::string prefab_name = std::filesystem::path(selected_prefab_path).stem().string();
        Engine::ECS::set_entity_name(newEntity, prefab_name);

        // Record undo for prefab instantiation
        Engine::ECS::EntityID entity_id = newEntity.get_id();
        Engine::ECS::Prefab captured_prefab = prefab;
        Engine::UndoManager::record(
          // Do: Recreate the entity from prefab
          [entity_id, captured_prefab, prefab_name]() {
            auto entity = Engine::ECS::instantiate_prefab_with_id(captured_prefab, entity_id);
            Engine::ECS::set_entity_name(entity, prefab_name);
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(entity_id);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          },
          // Undo: Delete the entity
          [entity_id]() {
            Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(entity_id, false);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
        );

        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      }
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
      for (Engine::ECS::ComponentTypeID type_id = 0; type_id < Engine::ECS::MAX_COMPONENTS; ++type_id) {
        auto& array = containers.component_arrays[type_id];
        if (!array) continue;
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
              // Save prefab immediately
              Engine::Resources::save_prefab(selected_prefab_path, selected_prefab, true);

              // Propagate changes to all prefab instances
              Engine::ECS::update_prefab_instances(selected_prefab.get_impl()->get_prefab_id());

              Log::info("Added component '{}' to prefab: {}", component_name, selected_prefab_path);
            }
            else {
              Log::error("Failed to add component '{}' to prefab: {}", component_name, selected_prefab_path);
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

      // First, collect all components grouped by namespace
      std::map<std::string, std::vector<std::pair<std::string, Engine::ECS::IComponentArray*>>> component_groups;

      selected_prefab.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray* array) {
        if (!array->is_reflectable()) {
          return; // Skip non-reflectable components
        }

        // Extract namespace (everything before the last "::")
        size_t last_separator = type_name.rfind("::");
        std::string namespace_key = (last_separator != std::string::npos)
          ? type_name.substr(0, last_separator)
          : "";

        // Skip "Components" namespace - treat as no namespace
        if (namespace_key == "Components") {
          namespace_key = "";
        }

        component_groups[namespace_key].push_back({ type_name, array });
        });

      // Render components grouped by namespace
      for (auto& [namespace_name, components] : component_groups) {
        if (namespace_name.empty()) {
          // Components without namespace - render directly
          for (auto& [type_name, array] : components) {
            // Strip "Components::" prefix if present, otherwise use full name
            std::string component_display_name = type_name;
            if (type_name.substr(0, 12) == "Components::") {
              component_display_name = type_name.substr(12);
            }

            PrefabEditorPanelHelpers::render_component_tree(component_display_name, selected_prefab, array, selected_prefab_path);
          }
        }
        else {
          // Components with namespace - render under namespace tree node
          if (ImGui::TreeNode(namespace_name.c_str())) {
            for (auto& [type_name, array] : components) {
              // Extract just the component name (after last "::")
              size_t last_separator = type_name.rfind("::");
              std::string component_only_name = type_name.substr(last_separator + 2);

              // Render with a temporary modified name for display
              PrefabEditorPanelHelpers::render_component_tree(component_only_name, selected_prefab, array, selected_prefab_path);
            }
            ImGui::TreePop();
          }
        }
      }
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

          // Save prefab immediately
          if (!selected_prefab_path.empty()) {
            Engine::Resources::save_prefab(selected_prefab_path, selected_prefab, true);

            // Propagate changes to all prefab instances
            Engine::ECS::update_prefab_instances(selected_prefab.get_impl()->get_prefab_id());
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

        // Save prefab immediately
        if (!selected_prefab_path.empty()) {
          Engine::Resources::save_prefab(selected_prefab_path, selected_prefab, true);

          // Propagate changes to all prefab instances
          Engine::ECS::update_prefab_instances(selected_prefab.get_impl()->get_prefab_id());
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

  // Rename popup (rendered at window level so it works from any context menu)
  if (open_rename_popup) {
    ImGui::OpenPopup("RenamePrefabPopup");
    open_rename_popup = false;
  }
  if (ImGui::BeginPopup("RenamePrefabPopup")) {
    ImGui::Text("Rename Prefab");
    ImGui::Separator();
    if (!rename_focus_set) {
      ImGui::SetKeyboardFocusHere();
      rename_focus_set = true;
    }
    bool confirmed = ImGui::InputText("##RenamePrefabInput", rename_buf, sizeof(rename_buf), ImGuiInputTextFlags_EnterReturnsTrue);
    if (confirmed && strlen(rename_buf) > 0) {
      std::string new_name = rename_buf;

      // Build the new relative path preserving the directory and extension
      std::filesystem::path old_rel(rename_prefab_path);
      std::filesystem::path new_rel = old_rel.parent_path() / (new_name + old_rel.extension().string());
      std::string new_relative_path = new_rel.generic_string();

      // Build full paths
      std::string old_full = Engine::Path(Engine::Resources::resource_path).join(rename_prefab_path);
      std::string new_full = Engine::Path(Engine::Resources::resource_path).join(new_relative_path);

      // Only rename if the name actually changed
      if (rename_prefab_path != new_relative_path) {
        std::error_code ec;
        std::filesystem::rename(old_full, new_full, ec);
        if (!ec) {
          // Update prefab cache: move entry from old key to new key
          auto it = Engine::Resources::prefab_cache.find(old_full);
          if (it != Engine::Resources::prefab_cache.end()) {
            auto prefab_weak = it->second;
            Engine::Resources::prefab_cache.erase(it);
            Engine::Resources::prefab_cache[new_full] = prefab_weak;

            // Update the prefab's internal filepath
            if (auto prefab_ptr = prefab_weak.lock()) {
              prefab_ptr->set_prefab_filepath(new_full);
            }
          }

          // Update selected path if it was the renamed prefab
          if (selected_prefab_path == rename_prefab_path) {
            selected_prefab_path = new_relative_path;
          }

          Log::info("Renamed prefab: {} -> {}", rename_prefab_path, new_relative_path);
        } else {
          Log::error("Failed to rename prefab file: {}", ec.message());
        }
      }

      rename_prefab_path.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  ImGui::End();
}
