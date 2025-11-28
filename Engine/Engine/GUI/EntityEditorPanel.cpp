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
#include "../Precompiled.h"
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
#include <map>
#include "../Components/SpatialComponents.h"
#include "../Components/UtilityComponents.h"
#include "../Components/HierarchyComponents.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../UndoManager/UndoManager.h"
#include "../InputManager.h"
#include "../ProjectSaveManager.h"

namespace {
  // Shared state for entity selection
  s32 selected_entity_id = -1;
  bool selection_initialized = false;

  enum class EntityViewMode {
    Flat,
    Layer,
    Hierarchy
  };

  EntityViewMode current_view_mode = EntityViewMode::Flat;

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

  // Helper to render a single entity node in the tree
  void render_entity_node(Engine::ECS::Entity& entity, bool is_leaf = true) {
    if (!entity.is_valid()) return;

    // Build entity label
    std::string entity_label = "Entity " + std::to_string(entity.get_id());

    // Try to get a name from common components
    if (entity.has_component<Components::Transform2D>()) {
      // entity_label += " (Transform)"; // Optional: Add more info
    }
    
    // Check if this entity is selected
    bool is_selected = (selected_entity_id == static_cast<s32>(entity.get_id()));
    
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (is_selected) {
      node_flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (is_leaf) {
      node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
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
        // Send delete event (handles undo/redo and marking dirty)
        Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(entity.get_id(), true);

        // Clear selection if this entity was selected
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

    // If it's not a leaf and is open, we need to let the caller handle the children rendering
    // But since this helper is generic, we might need a callback or just return the state
    // For now, this helper assumes LEAF behavior if is_leaf is true.
    // If is_leaf is false, the caller MUST call TreePop() if node_open is true.
  }
  
  // Recursive function to render hierarchy
  void render_hierarchy_node(Engine::ECS::Entity entity, const std::unordered_map<Engine::ECS::EntityID, std::vector<Engine::ECS::Entity>>& parent_to_children) {
    auto it = parent_to_children.find(entity.get_id());
    bool has_children = (it != parent_to_children.end() && !it->second.empty());

    // Build entity label
    std::string entity_label = "Entity " + std::to_string(entity.get_id());

    // Check if this entity is selected
    bool is_selected = (selected_entity_id == static_cast<s32>(entity.get_id()));
    
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (is_selected) {
      node_flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!has_children) {
      node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    else {
      node_flags |= ImGuiTreeNodeFlags_DefaultOpen; // Optional: Open by default
    }

    bool node_open = ImGui::TreeNodeEx(
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
        Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(entity.get_id(), true);
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

    if (has_children && node_open) {
      for (auto& child : it->second) {
        render_hierarchy_node(child, parent_to_children);
      }
      ImGui::TreePop();
    }
  }
}

void EntityEditorPanelHelpers::render_component_tree(const std::string& component_name, Engine::ECS::Entity& entity, Engine::ECS::IComponentArray* array)
{
  // Render component tree node directly (namespace grouping handled by caller)
  if (ImGui::TreeNode(component_name.c_str())) {
    // Track the value before any modifications
    static std::unordered_map<std::string, ReflectionLibrary::GenericValue> component_edit_start_values;
    static std::unordered_map<std::string, bool> component_being_edited;
    static std::unordered_map<std::string, bool> component_was_active_last_frame;

    std::string component_key = component_name + "_" + std::to_string(entity.get_id());

    auto component_data = array->to_generic_value(entity.get_id());
    bool modified = false;

    // Get the type name from the component array for specialized rendering
    std::string type_hint = array->get_type_name();

    // Try specialized rendering first
    bool handled_by_specialization = ImGuiWidgets::render_specialized_field(
      component_name, type_hint, component_data, modified, entity
    );

    // If not specialized, render fields directly (without creating another tree node)
    if (!handled_by_specialization && component_data.isObject()) {
      auto& fields = component_data.getObject();
      for (auto& [field_name, field_value] : fields) {
        ImGuiWidgets::render_field(field_name, field_value.type_hint, field_value, modified, entity);
      }
    }

    // Check if user is currently editing (any widget is active in this component)
    bool is_any_item_active = ImGui::IsAnyItemActive();
    bool was_active_last_frame = component_was_active_last_frame[component_key];

    // Start tracking when editing begins
    if (is_any_item_active && !component_being_edited[component_key]) {
      component_being_edited[component_key] = true;
      component_edit_start_values[component_key] = array->to_generic_value(entity.get_id());
    }

    // Apply changes in real-time
    if (modified) {
      array->set_component_from_generic_value(entity.get_id(), component_data);
    }

    // Record to undo manager only when editing just finished (was active, now inactive)
    if (component_being_edited[component_key] && was_active_last_frame && !is_any_item_active) {
      auto prev_value = component_edit_start_values[component_key];
      auto new_value = component_data;

      // Only send event if the value actually changed
      if (prev_value != new_value) {
        // Get component type ID
        Engine::ECS::ComponentTypeID type_id = array->get_type_id();

        // Send modification event (this will handle undo/redo and marking dirty)
        Engine::EventManager::send_event<Engine::Events::ModifyComponentEvent>(
          entity.get_id(), type_id, new_value, prev_value, true
        );
      }

      component_being_edited[component_key] = false;
      component_edit_start_values.erase(component_key);
    }

    // Update last frame state
    component_was_active_last_frame[component_key] = is_any_item_active;

    ImGui::Separator();

    std::string remove_label = "Remove##" + component_name;
    if (ImGui::Button(remove_label.c_str())) {
      // Get component type ID
      Engine::ECS::ComponentTypeID type_id = array->get_type_id();

      // Send remove component event (this will handle undo/redo and marking dirty)
      Engine::EventManager::send_event<Engine::Events::RemoveComponentEvent>(
        entity.get_id(), type_id, true
      );
    }

    ImGui::TreePop();
  }
}

void EntityEditorPanel()
{
  static char entitySearchBuffer[256] = "";
  
  // Initialize event listener on first call
  init_selection_listener();

  ImGui::Begin("Entity Editor");

  // Section 1: Entity List (changes based on mode)
  if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // View Mode Selector (Radio Buttons)
    if (ImGui::RadioButton("Flat", current_view_mode == EntityViewMode::Flat)) current_view_mode = EntityViewMode::Flat;
    ImGui::SameLine();
    if (ImGui::RadioButton("Layer", current_view_mode == EntityViewMode::Layer)) current_view_mode = EntityViewMode::Layer;
    ImGui::SameLine();
    if (ImGui::RadioButton("Hierarchy", current_view_mode == EntityViewMode::Hierarchy)) current_view_mode = EntityViewMode::Hierarchy;

    ImGui::Separator();

    ImGui::Text("Total Entities: %zu", Engine::ECS::get_entity_count());

    // Search/Filter
    ImGui::InputText("Search", entitySearchBuffer, 256);

    ImGui::Separator();

    // Entity list with selection - changes based on mode
    ImGui::BeginChild("EntityList", ImVec2(0, 200), true);

    auto allEntities = Engine::ECS::get_all_entities();

    if (current_view_mode == EntityViewMode::Layer) {
      // Layer view - group by layers
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
            render_entity_node(entity, true);
          }
          ImGui::TreePop();
        }
      }
    }
    else if (current_view_mode == EntityViewMode::Hierarchy) {
      // Hierarchy view - group by parent/child
      std::unordered_map<Engine::ECS::EntityID, std::vector<Engine::ECS::Entity>> parent_to_children;
      std::vector<Engine::ECS::Entity> root_entities;

      // Build hierarchy map
      for (auto& entity : allEntities) {
        if (entity.has_component<Components::Internal>()) continue;

        if (entity.has_component<Components::Parent>()) {
          auto* parent_comp = entity.get_component<Components::Parent>();
          if (parent_comp->parent_id.is_valid()) {
            parent_to_children[parent_comp->parent_id.get_id()].push_back(entity);
          }
          else {
            // Parent invalid, treat as root
            root_entities.push_back(entity);
          }
        }
        else {
          // No parent component, is root
          root_entities.push_back(entity);
        }
      }

      // Render roots (and recursively children)
      for (auto& entity : root_entities) {
        render_hierarchy_node(entity, parent_to_children);
      }
    }
    else {
      // Flat view - simple list
      for (auto& entity : allEntities)
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
      // Send delete event (handles undo/redo and marking dirty)
      Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(selected_entity_id, true);

      // Clear selection
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
            // Create default component data
            ReflectionLibrary::GenericValue default_data = ReflectionLibrary::Object();

            // Send add component event (this will handle undo/redo and marking dirty)
            Engine::EventManager::send_event<Engine::Events::AddComponentEvent>(
              entity.get_id(), type_id, default_data, true
            );

            Log::info("Added component '{}' to entity {}", component_name, entity.get_id());
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
      // First, collect all components grouped by namespace
      std::map<std::string, std::vector<std::pair<std::string, Engine::ECS::IComponentArray*>>> component_groups;

      entity.for_each_component([&](const std::string& type_name, Engine::ECS::IComponentArray* array) {
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

            EntityEditorPanelHelpers::render_component_tree(component_display_name, entity, array);
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
              EntityEditorPanelHelpers::render_component_tree(component_only_name, entity, array);
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
      auto& containers = Engine::ECS::Internal::get_containers();
      auto entity_layers = entity.get_layers();

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
            // Add prefab to resource cache so it can be tracked and appears in PrefabEditorPanel
            Engine::Resources::cache_prefab(relative_path, prefab, true, true);

            // Mark as dirty so it gets saved
            Engine::ProjectSaveManager::set_dirty(
              Engine::ProjectSaveManager::DirtyStates::Prefab(relative_path)
            );

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

    // Update Prefab from Entity
    if (ImGui::Button("Update Prefab"))
    {
      ImGui::OpenPopup("UpdatePrefabFromEntity");
    }

    // Update Prefab popup
    static char update_prefab_search[128] = "";
    static std::string selected_prefab_for_update = "";
    if (ImGui::BeginPopupModal("UpdatePrefabFromEntity", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text("Select prefab to update:");
      ImGui::Separator();

      // Search filter
      ImGui::InputText("Search##UpdatePrefab", update_prefab_search, sizeof(update_prefab_search));

      ImGui::Separator();

      // List available prefabs
      ImGui::BeginChild("UpdatePrefabList", ImVec2(400, 300), true);

      // Get prefabs from resource cache
      std::vector<std::pair<std::string, Engine::ECS::Prefab>> prefab_list;

      for (const auto& [path, weak_prefab] : Engine::Resources::prefab_cache) {
        if (auto prefab_ptr = weak_prefab.lock()) {
          // Get relative path for display
          std::string relative_path = Engine::Resources::get_relative_path(path);

          // Apply search filter
          if (strlen(update_prefab_search) > 0 && relative_path.find(update_prefab_search) == std::string::npos) {
            continue;
          }

          prefab_list.push_back({ relative_path, Engine::ECS::Prefab(prefab_ptr) });
        }
      }

      // Sort by path for consistent ordering
      std::sort(prefab_list.begin(), prefab_list.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

      for (const auto& [relative_path, prefab] : prefab_list) {
        bool is_selected = (selected_prefab_for_update == relative_path);

        if (ImGui::Selectable(relative_path.c_str(), is_selected)) {
          selected_prefab_for_update = relative_path;
        }
      }

      ImGui::EndChild();

      ImGui::Separator();

      if (ImGui::Button("Update", ImVec2(120, 0)))
      {
        if (!selected_prefab_for_update.empty())
        {
          // Construct full path from relative path
          std::string full_path = Engine::Path(Engine::Resources::resource_path).join(selected_prefab_for_update).to_string();

          // Load existing prefab to get its UUID
          Engine::ECS::Prefab existing_prefab = Engine::FileIO::deserialize_prefab_from_json_file(full_path);
          
          if (existing_prefab.is_valid()) {
            std::string preserved_uuid = existing_prefab.get_impl()->get_prefab_id();

            // Create new prefab from current entity
            auto updated_prefab = Engine::ECS::entity_to_prefab(entity);

            // Preserve the original UUID and filepath
            updated_prefab.get_impl()->set_prefab_id(preserved_uuid);
            updated_prefab.get_impl()->set_prefab_filepath(selected_prefab_for_update);

            // Save to file
            if (Engine::FileIO::serialize_prefab_to_json_file(full_path, updated_prefab))
            {
              // Update prefab in resource cache
              Engine::Resources::cache_prefab(selected_prefab_for_update, updated_prefab, true, true);

              // Mark as dirty so it gets saved
              Engine::ProjectSaveManager::set_dirty(
                Engine::ProjectSaveManager::DirtyStates::Prefab(selected_prefab_for_update)
              );

              // Update all instances of this prefab
              Engine::ECS::update_prefab_instances(preserved_uuid);

              Log::info("Updated prefab: {} (UUID: {})", selected_prefab_for_update, preserved_uuid);

              // Clear selection and close popup
              selected_prefab_for_update = "";
              update_prefab_search[0] = '\0';
              ImGui::CloseCurrentPopup();
              ImGui::OpenPopup("UpdatePrefabSuccess");
            }
          }
          else {
            Log::error("Failed to load existing prefab: {}", selected_prefab_for_update);
          }
        }
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120, 0)))
      {
        selected_prefab_for_update = "";
        update_prefab_search[0] = '\0';
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    // Update Prefab success popup
    if (ImGui::BeginPopup("UpdatePrefabSuccess"))
    {
      ImGui::Text("Prefab updated successfully!");
      ImGui::Text("UUID preserved and instances updated.");
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
