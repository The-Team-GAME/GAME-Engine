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
#include "ResourcePicker.h"
#include "widgets/ReflectionFieldWidgets.h"
#include "../FileIO/PrefabsFileIO.h"
#include "../FileIO/LevelFileIO.h"
#include "../FileIO/Helpers.h"
#include "../Path.h"
#include "../Resources.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <algorithm>
#include "../Components/SpatialComponents.h"
#include "../Components/UtilityComponents.h"
#include "../Components/HierarchyComponents.h"
#include "../Components/LevelComponent.h"
#include "../EventManager/EventManager.h"
#include "../Events/EngineEvents.h"
#include "../UndoManager/UndoManager.h"
#include "../InputManager.h"
#include "../ProjectSaveManager.h"
#include "../LevelManager.h"

namespace {
  // Shared state for entity selection
  s32 selected_entity_id = -1;               // primary selection (shown in inspector)
  std::unordered_set<Engine::ECS::EntityID> selected_entity_ids; // multi-selection set
  std::vector<Engine::ECS::Prefab> entity_clipboard; // copy/paste clipboard
  bool selection_initialized = false;

  // Selection export state (multi-entity)
  char selection_export_path_buf[256] = "spawn_sets/entity_selection.json";
  std::string selection_export_status;
  float selection_export_status_time = 0.0f;

  // Shift+Click range select state
  Engine::ECS::EntityID last_clicked_entity_id = Engine::ECS::INVALID_ENTITY;
  std::vector<Engine::ECS::EntityID> flat_entity_order; // rebuilt each frame

  // Rename state
  Engine::ECS::EntityID rename_entity_id = Engine::ECS::INVALID_ENTITY;
  char rename_buf[256] = "";
  bool rename_focus_set = false;
  bool open_rename_popup = false;

  enum class EntityViewMode {
    Hierarchy,
    Layer,
    Flat
  };

  EntityViewMode current_view_mode = EntityViewMode::Hierarchy;
  std::string pending_prefab_bind_path;

  // Event listener to sync selection from other sources (like Gizmo system)
  void init_selection_listener() {
    if (selection_initialized) return;
    selection_initialized = true;

    Engine::EventManager::add_event_listener<Engine::Events::SelectEntityEvent>(
      [](const Engine::Events::SelectEntityEvent& event) {
        if (event.add_to_selection) {
          // Ctrl+Click from viewport: toggle entity in multi-selection
          if (event.entity_id != Engine::ECS::INVALID_ENTITY) {
            if (selected_entity_ids.count(event.entity_id)) {
              // Deselect
              selected_entity_ids.erase(event.entity_id);
              if (selected_entity_id == static_cast<s32>(event.entity_id)) {
                if (!selected_entity_ids.empty()) {
                  selected_entity_id = static_cast<s32>(*selected_entity_ids.begin());
                }
                else {
                  selected_entity_id = -1;
                }
              }
            }
            else {
              // Add to selection
              selected_entity_ids.insert(event.entity_id);
              selected_entity_id = static_cast<s32>(event.entity_id);
            }
          }
          return;
        }
        // Normal (exclusive) selection from viewport or other sources
        selected_entity_id = static_cast<s32>(event.entity_id);
        selected_entity_ids.clear();
        if (event.entity_id != Engine::ECS::INVALID_ENTITY) {
          selected_entity_ids.insert(event.entity_id);
        }
      }
    );
  }

  // Helper: Shift+Click range select — selects all entities between two IDs in flat_entity_order
  void select_range(Engine::ECS::EntityID from_id, Engine::ECS::EntityID to_id) {
    if (to_id == Engine::ECS::INVALID_ENTITY) return;
    // If no prior anchor, treat it as a plain click on to_id
    if (from_id == Engine::ECS::INVALID_ENTITY) {
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(to_id);
      return;
    }
    int idx_from = -1, idx_to = -1;
    for (int i = 0; i < static_cast<int>(flat_entity_order.size()); ++i) {
      if (flat_entity_order[i] == from_id) idx_from = i;
      if (flat_entity_order[i] == to_id) idx_to = i;
    }
    if (idx_from == -1 || idx_to == -1) return;
    int lo = std::min(idx_from, idx_to);
    int hi = std::max(idx_from, idx_to);
    // Clear all selections first, then add the range via events so
    // both the panel state and the Gizmo components stay in sync.
    Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
    for (int i = lo; i <= hi; ++i) {
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(flat_entity_order[i], false, true);
    }
  }

  // Helper to render a single entity node in the tree
  void render_entity_node(Engine::ECS::Entity entity, bool is_leaf = true) {
    if (!entity.is_valid()) return;

    // Build entity label from ECS entity name
    const std::string& name = Engine::ECS::get_entity_name(entity);
    std::string entity_label = name.empty()
      ? "Entity " + std::to_string(entity.get_id())
      : name + " (" + std::to_string(entity.get_id()) + ")";

    // Append Level info if present
    if (entity.has_component<Components::Level>()) {
      auto* level_comp = entity.get_component<Components::Level>();
      entity_label += " [Level: " + level_comp->level_path + "]";
    }

    // Check if this entity is selected (multi-selection)
    bool is_selected = selected_entity_ids.count(entity.get_id()) > 0;

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

    // Handle selection (Ctrl+Click toggles, Shift+Click range, plain click replaces)
    if (ImGui::IsItemClicked()) {
      Engine::ECS::EntityID id = entity.get_id();
      if (ImGui::GetIO().KeyCtrl) {
        // Toggle via event — listener handles both state and Gizmo component
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(id, false, true);
        last_clicked_entity_id = id;
      }
      else if (ImGui::GetIO().KeyShift) {
        select_range(last_clicked_entity_id, id);
        // don't update last_clicked_entity_id on shift-click (anchor stays)
      }
      else {
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(id);
        last_clicked_entity_id = id;
      }
    }

    // Right-click context menu
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Copy")) {
        entity_clipboard.clear();
        for (Engine::ECS::EntityID sel_id : selected_entity_ids) {
          Engine::ECS::Entity sel_e = Engine::ECS::get_entity(sel_id);
          if (sel_e.is_valid()) entity_clipboard.push_back(Engine::ECS::entity_to_prefab(sel_e));
        }
        // If nothing multi-selected, copy just this entity
        if (entity_clipboard.empty()) {
          entity_clipboard.push_back(Engine::ECS::entity_to_prefab(entity));
        }
      }
      if (ImGui::MenuItem("Delete Entity")) {
        Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(entity.get_id(), true);
      }
      if (ImGui::MenuItem("Clone Entity")) {
        Engine::ECS::Entity cloned = Engine::ECS::clone_entity(entity);
        selected_entity_ids.clear();
        selected_entity_ids.insert(cloned.get_id());
        selected_entity_id = static_cast<s32>(cloned.get_id());
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(cloned.get_id());

        // Record undo for clone
        Engine::ECS::EntityID cloned_id = cloned.get_id();
        Engine::ECS::Prefab cloned_prefab = Engine::ECS::entity_to_prefab(cloned);
        Engine::UndoManager::record(
          [cloned_id, cloned_prefab]() {
            Engine::ECS::instantiate_prefab_with_id(cloned_prefab, cloned_id);
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(cloned_id);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          },
          [cloned_id]() {
            Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(cloned_id, false);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
        );
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      }
      if (ImGui::MenuItem("Rename")) {
        rename_entity_id = entity.get_id();
        const std::string& cur_name = Engine::ECS::get_entity_name(entity);
        strncpy_s(rename_buf, cur_name.c_str(), sizeof(rename_buf) - 1);
        rename_focus_set = false;
        open_rename_popup = true;
      }
      ImGui::EndPopup();
    }

    // If it's not a leaf and is open, we need to let the caller handle the children rendering
    // But since this helper is generic, we might need a callback or just return the state
    // For now, this helper assumes LEAF behavior if is_leaf is true.
    // If is_leaf is false, the caller MUST call TreePop() if node_open is true.
  }

  // Payload type for entity drag-drop
  const char* ENTITY_DRAG_DROP_TYPE = "ENTITY_HIERARCHY";

  // Helper function to check if an entity is an ancestor of another (to prevent circular parenting)
  bool is_ancestor_of(Engine::ECS::EntityID potential_ancestor, Engine::ECS::EntityID entity_id) {
    Engine::ECS::Entity current = Engine::ECS::get_entity(entity_id);
    while (current.is_valid() && current.has_component<Components::Parent>()) {
      auto* parent_comp = current.get_component<Components::Parent>();
      if (parent_comp->parent_id == Engine::ECS::INVALID_ENTITY) break;
      if (parent_comp->parent_id == potential_ancestor) return true;
      current = Engine::ECS::get_entity(parent_comp->parent_id);
    }
    return false;
  }

  // Helper function to set parent of an entity
  void set_entity_parent(Engine::ECS::Entity child, Engine::ECS::EntityID new_parent_id) {
    if (!child.is_valid()) return;

    // Get old parent ID if exists
    Engine::ECS::EntityID old_parent_id = Engine::ECS::INVALID_ENTITY;
    if (child.has_component<Components::Parent>()) {
      old_parent_id = child.get_component<Components::Parent>()->parent_id;
    }

    // Skip if same parent
    if (old_parent_id == new_parent_id) return;

    // Remove from old parent's Children component if exists
    if (old_parent_id != Engine::ECS::INVALID_ENTITY) {
      Engine::ECS::Entity old_parent = Engine::ECS::get_entity(old_parent_id);
      if (old_parent.is_valid() && old_parent.has_component<Components::Children>()) {
        auto* children_comp = old_parent.get_component<Components::Children>();
        auto& child_ids = children_comp->child_ids;
        child_ids.erase(std::remove(child_ids.begin(), child_ids.end(), child.get_id()), child_ids.end());
      }
    }

    // Update or add Parent component
    if (new_parent_id == Engine::ECS::INVALID_ENTITY) {
      // Unparenting - remove Parent component
      if (child.has_component<Components::Parent>()) {
        child.remove_component<Components::Parent>();
      }
    }
    else {
      // Setting new parent
      if (child.has_component<Components::Parent>()) {
        auto* parent_comp = child.get_component<Components::Parent>();
        parent_comp->parent_id = new_parent_id;
        parent_comp->offset_initialized = false; // Let HierarchySystem recalculate offset
      }
      else {
        child.add_component<Components::Parent>(new_parent_id);
      }

      // Add to new parent's Children component
      Engine::ECS::Entity new_parent = Engine::ECS::get_entity(new_parent_id);
      if (new_parent.is_valid()) {
        if (!new_parent.has_component<Components::Children>()) {
          new_parent.add_component<Components::Children>();
        }
        auto* children_comp = new_parent.get_component<Components::Children>();
        // Only add if not already in list
        if (std::find(children_comp->child_ids.begin(), children_comp->child_ids.end(), child.get_id()) == children_comp->child_ids.end()) {
          children_comp->child_ids.push_back(child.get_id());
        }
      }
    }

    // Mark level as dirty
    Engine::ProjectSaveManager::set_dirty(
      Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path())
    );
  }

  // Recursive function to render hierarchy
  void render_hierarchy_node(Engine::ECS::Entity entity, const std::unordered_map<Engine::ECS::EntityID, std::vector<Engine::ECS::Entity>>& parent_to_children) {
    auto it = parent_to_children.find(entity.get_id());
    bool has_children = (it != parent_to_children.end() && !it->second.empty());

    // Build entity label from ECS entity name
    const std::string& name = Engine::ECS::get_entity_name(entity);
    std::string entity_label = name.empty()
      ? "Entity " + std::to_string(entity.get_id())
      : name + " (" + std::to_string(entity.get_id()) + ")";

    // Append Level info if present
    if (entity.has_component<Components::Level>()) {
      auto* level_comp = entity.get_component<Components::Level>();
      entity_label += " [Level: " + level_comp->level_path + "]";
    }

    // Check if this entity is selected (multi-selection)
    bool is_selected = selected_entity_ids.count(entity.get_id()) > 0;

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

    // Handle selection (Ctrl+Click toggles, Shift+Click range, plain click replaces)
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      Engine::ECS::EntityID id = entity.get_id();
      if (ImGui::GetIO().KeyCtrl) {
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(id, false, true);
        last_clicked_entity_id = id;
      }
      else if (ImGui::GetIO().KeyShift) {
        select_range(last_clicked_entity_id, id);
      }
      else {
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(id);
        last_clicked_entity_id = id;
      }
    }

    // Drag source - allow dragging this entity
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
      Engine::ECS::EntityID entity_id = entity.get_id();
      ImGui::SetDragDropPayload(ENTITY_DRAG_DROP_TYPE, &entity_id, sizeof(Engine::ECS::EntityID));
      ImGui::Text("Move: %s", entity_label.c_str());
      ImGui::EndDragDropSource();
    }

    // Drop target - allow dropping other entities onto this one to parent them
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ENTITY_DRAG_DROP_TYPE)) {
        Engine::ECS::EntityID dropped_entity_id = *(const Engine::ECS::EntityID*)payload->Data;

        // Don't allow parenting to self
        if (dropped_entity_id != entity.get_id()) {
          // Check for circular parenting (don't allow parenting to a descendant)
          if (!is_ancestor_of(dropped_entity_id, entity.get_id())) {
            set_entity_parent(Engine::ECS::get_entity(dropped_entity_id), entity.get_id());
          }
        }
      }
      ImGui::EndDragDropTarget();
    }

    // Right-click context menu
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Copy")) {
        entity_clipboard.clear();
        for (Engine::ECS::EntityID sel_id : selected_entity_ids) {
          Engine::ECS::Entity sel_e = Engine::ECS::get_entity(sel_id);
          if (sel_e.is_valid()) entity_clipboard.push_back(Engine::ECS::entity_to_prefab(sel_e));
        }
        if (entity_clipboard.empty()) {
          entity_clipboard.push_back(Engine::ECS::entity_to_prefab(entity));
        }
      }
      if (ImGui::MenuItem("Delete Entity")) {
        Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(entity.get_id(), true);
        selected_entity_ids.erase(entity.get_id());
        if (selected_entity_id == static_cast<s32>(entity.get_id())) {
          selected_entity_id = selected_entity_ids.empty() ? -1 : static_cast<s32>(*selected_entity_ids.begin());
          Engine::ECS::EntityID new_primary = (selected_entity_id == -1)
            ? Engine::ECS::INVALID_ENTITY : static_cast<Engine::ECS::EntityID>(selected_entity_id);
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(new_primary);
        }
      }
      if (ImGui::MenuItem("Clone Entity")) {
        Engine::ECS::Entity cloned = Engine::ECS::clone_entity(entity);
        selected_entity_ids.clear();
        selected_entity_ids.insert(cloned.get_id());
        selected_entity_id = static_cast<s32>(cloned.get_id());
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(cloned.get_id());

        // Record undo for clone
        Engine::ECS::EntityID cloned_id = cloned.get_id();
        Engine::ECS::Prefab cloned_prefab = Engine::ECS::entity_to_prefab(cloned);
        Engine::UndoManager::record(
          // Do: Recreate the cloned entity
          [cloned_id, cloned_prefab]() {
            Engine::ECS::instantiate_prefab_with_id(cloned_prefab, cloned_id);
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(cloned_id);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          },
          // Undo: Delete the cloned entity
          [cloned_id]() {
            Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(cloned_id, false);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
        );
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      }
      if (ImGui::MenuItem("Rename")) {
        rename_entity_id = entity.get_id();
        const std::string& cur_name = Engine::ECS::get_entity_name(entity);
        strncpy_s(rename_buf, cur_name.c_str(), sizeof(rename_buf) - 1);
        rename_focus_set = false;
        open_rename_popup = true;
      }
      // Add unparent option if entity has a parent
      if (entity.has_component<Components::Parent>()) {
        auto* parent_comp = entity.get_component<Components::Parent>();
        if (parent_comp->parent_id != Engine::ECS::INVALID_ENTITY) {
          if (ImGui::MenuItem("Unparent")) {
            set_entity_parent(entity, Engine::ECS::INVALID_ENTITY);
          }
        }
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

void EntityEditorPanelHelpers::render_component_tree(const std::string& component_name, Engine::ECS::Entity entity, Engine::ECS::IComponentArray* array)
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
  static std::string picked_prefab_path;
  static bool picked_is_new = false;

  // Initialize event listener on first call
  init_selection_listener();

  ImGui::Begin("Entity Editor");

  // Ctrl+C / Ctrl+V copy-paste (only when this window is focused)
  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
    // Ctrl+C: copy all selected entities to clipboard
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false) && !selected_entity_ids.empty()) {
      entity_clipboard.clear();
      for (Engine::ECS::EntityID sel_id : selected_entity_ids) {
        Engine::ECS::Entity sel_e = Engine::ECS::get_entity(sel_id);
        if (sel_e.is_valid()) entity_clipboard.push_back(Engine::ECS::entity_to_prefab(sel_e));
      }
    }
    // Ctrl+V: paste clipboard entities
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false) && !entity_clipboard.empty()) {
      selected_entity_ids.clear();
      selected_entity_id = -1;

      // Collect all pasted entity IDs and their prefabs for a single undo entry
      struct PasteEntry { Engine::ECS::EntityID id; Engine::ECS::Prefab prefab; };
      std::vector<PasteEntry> paste_entries;

      for (auto& prefab : entity_clipboard) {
        Engine::ECS::Entity pasted = Engine::ECS::instantiate_prefab(prefab);
        if (pasted.is_valid()) {
          Engine::ECS::EntityID pasted_id = pasted.get_id();
          selected_entity_ids.insert(pasted_id);
          selected_entity_id = static_cast<s32>(pasted_id);
          paste_entries.push_back({ pasted_id, prefab });
        }
      }

      // Record ONE undo entry for the entire paste operation
      if (!paste_entries.empty()) {
        Engine::UndoManager::record(
          [paste_entries]() { // redo: re-create all pasted entities
            for (auto& pe : paste_entries) {
              Engine::ECS::instantiate_prefab_with_id(pe.prefab, pe.id);
            }
            if (!paste_entries.empty())
              Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(paste_entries.back().id);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          },
          [paste_entries]() { // undo: delete all pasted entities
            for (auto& pe : paste_entries) {
              Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(pe.id, false);
            }
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
        );
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      }

      if (selected_entity_id != -1) {
        Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(static_cast<Engine::ECS::EntityID>(selected_entity_id));
      }
    }
  }

  // Section 1: Entity List (changes based on mode)
  if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // View Mode Selector (Radio Buttons)
    if (ImGui::RadioButton("Hierarchy", current_view_mode == EntityViewMode::Hierarchy)) current_view_mode = EntityViewMode::Hierarchy;
    ImGui::SameLine();
    if (ImGui::RadioButton("Layer", current_view_mode == EntityViewMode::Layer)) current_view_mode = EntityViewMode::Layer;
    ImGui::SameLine();
    if (ImGui::RadioButton("Flat", current_view_mode == EntityViewMode::Flat)) current_view_mode = EntityViewMode::Flat;

    ImGui::Separator();

    ImGui::Text("Total Entities: %zu", Engine::ECS::get_entity_count());

    // Search/Filter
    ImGui::InputText("Search", entitySearchBuffer, 256);

    ImGui::Separator();

    // Entity list with selection - changes based on mode
    ImGui::BeginChild("EntityList", ImVec2(0, 200), true);

    auto allEntities = Engine::ECS::get_all_entities();

    // Build flat entity order for Shift+Click range select
    flat_entity_order.clear();
    for (auto& entity : allEntities) {
      if (!entity.has_component<Components::Internal>())
        flat_entity_order.push_back(entity.get_id());
    }

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
          if (parent_comp->parent_id != Engine::ECS::INVALID_ENTITY) {
            parent_to_children[parent_comp->parent_id].push_back(entity);
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

      // Fill remaining space with an invisible button to catch drops for unparenting
      ImVec2 avail = ImGui::GetContentRegionAvail();
      if (avail.y > 0) {
        ImGui::InvisibleButton("##unparent_drop_zone", ImVec2(-1, avail.y));
        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ENTITY_DRAG_DROP_TYPE)) {
            Engine::ECS::EntityID dropped_entity_id = *(const Engine::ECS::EntityID*)payload->Data;
            set_entity_parent(Engine::ECS::get_entity(dropped_entity_id), Engine::ECS::INVALID_ENTITY);
          }
          ImGui::EndDragDropTarget();
        }
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

        bool isSelected = selected_entity_ids.count(entity.get_id()) > 0;
        const std::string& fname = Engine::ECS::get_entity_name(entity);
        std::string label = fname.empty()
          ? "Entity " + std::to_string(entity.get_id())
          : fname + " (" + std::to_string(entity.get_id()) + ")";
        if (entity.has_component<Components::Level>()) {
          auto* level_comp = entity.get_component<Components::Level>();
          label += " [Level: " + level_comp->level_path + "]";
        }

        if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_None))
        {
          Engine::ECS::EntityID id = entity.get_id();
          if (ImGui::GetIO().KeyCtrl) {
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(id, false, true);
            last_clicked_entity_id = id;
          }
          else if (ImGui::GetIO().KeyShift) {
            select_range(last_clicked_entity_id, id);
          }
          else {
            Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(id);
            last_clicked_entity_id = id;
          }
        }
      }
    }

    ImGui::EndChild();

    // Create new entity button
    if (ImGui::Button("Create New Entity"))
    {
      auto newEntity = Engine::ECS::create_entity();
      selected_entity_ids.clear();
      selected_entity_ids.insert(newEntity.get_id());
      selected_entity_id = static_cast<int>(newEntity.get_id());
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(newEntity.get_id());

      // Record undo for entity creation
      Engine::ECS::EntityID entity_id = newEntity.get_id();
      Engine::UndoManager::record(
        // Do: Recreate the entity
        [entity_id]() {
          Engine::ECS::create_entity_with_id(entity_id);
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

    ImGui::SameLine();

    // Delete selected entities (all multi-selected)
    if (!selected_entity_ids.empty() && ImGui::Button("Delete Selected"))
    {
      std::vector<Engine::ECS::EntityID> to_delete(selected_entity_ids.begin(), selected_entity_ids.end());
      for (Engine::ECS::EntityID del_id : to_delete) {
        Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(del_id, true);
      }
      selected_entity_ids.clear();
      selected_entity_id = -1;
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(Engine::ECS::INVALID_ENTITY);
    }

    // Show multi-selection count hint
    if (selected_entity_ids.size() > 1) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%zu selected  (Ctrl+C to copy)", selected_entity_ids.size());
    }
    else if (!entity_clipboard.empty()) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[%zu in clipboard]", entity_clipboard.size());
    }
  }

  ImGui::Separator();

  // Section 2: Component Editor for Selected Entity
  if (selected_entity_id != -1)
  {
    auto entity = Engine::ECS::get_entity(selected_entity_id);

    if (!entity.is_valid())
    {
      selected_entity_ids.erase(static_cast<Engine::ECS::EntityID>(selected_entity_id));
      selected_entity_id = selected_entity_ids.empty() ? -1 : static_cast<s32>(*selected_entity_ids.begin());
      ImGui::End();
      return;
    }

    // Editable entity name
    {
      const std::string& ename = Engine::ECS::get_entity_name(entity);
      ImGui::Text("Editing: %s (ID: %d)", ename.empty() ? "Entity" : ename.c_str(), selected_entity_id);
    }

    // Prefab instance info
    if (Engine::ECS::is_prefab_instance(entity)) {
      std::string prefab_path = Engine::ECS::get_prefab_path(entity);
      ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Prefab: %s", prefab_path.empty() ? "(unknown)" : prefab_path.c_str());

      // Quick save back to source prefab
      if (!prefab_path.empty()) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Save to Source")) {
          std::string full_path = Engine::Path(Engine::Resources::resource_path).join(prefab_path).to_string();
          Engine::ECS::Prefab existing_prefab = Engine::FileIO::deserialize_prefab_from_json_file(full_path);
          if (existing_prefab.is_valid()) {
            std::string preserved_uuid = existing_prefab.get_impl()->get_prefab_id();
            auto updated_prefab = Engine::ECS::entity_to_prefab(entity);
            updated_prefab.get_impl()->set_prefab_id(preserved_uuid);
            updated_prefab.get_impl()->set_prefab_filepath(prefab_path);

            // Fix: entity_to_prefab registered the entity with a NEW uuid, re-point it to the preserved one
            auto& containers = Engine::ECS::Internal::get_containers();
            auto inst_it = containers.prefab_instances.find(entity.get_id());
            if (inst_it != containers.prefab_instances.end()) {
              inst_it->second.prefab_id = preserved_uuid;
              inst_it->second.prefab_filepath = prefab_path;
            }

            if (Engine::FileIO::serialize_prefab_to_json_file(full_path, updated_prefab)) {
              Engine::Resources::cache_prefab(prefab_path, updated_prefab, true, true);
              Engine::ECS::update_prefab_instances(preserved_uuid);
              Log::info("Saved entity to source prefab: {} (UUID: {})", prefab_path, preserved_uuid);
            }
          }
          else {
            Log::error("Failed to load source prefab: {}", prefab_path);
          }
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Overwrite the source prefab with this entity's current state");
        }
      }

      // Unbind from prefab
      {
        ImGui::SameLine();
        if (ImGui::SmallButton("Unbind")) {
          Engine::ECS::unbind_prefab_instance(entity);
          Log::info("Unbound entity {} from prefab", selected_entity_id);
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Detach this entity from its source prefab.\nThe entity keeps its data but will no longer receive prefab updates.");
        }
      }
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Bind to Prefab")) {
      ImGui::OpenPopup("BindPrefabPicker");
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Replace this entity with the selected prefab and bind it to that source.");
    }

    if (ResourcePicker::prefab_picker_popup("BindPrefabPicker", picked_prefab_path, picked_is_new)) {
      if (!picked_is_new) {
        pending_prefab_bind_path = picked_prefab_path;
      }
    }

    if (!pending_prefab_bind_path.empty()) {
      Engine::ECS::Prefab prefab_to_bind = Engine::Resources::load_prefab(pending_prefab_bind_path, true, true);
      if (prefab_to_bind.is_valid() && Engine::ECS::bind_entity_to_prefab(entity, prefab_to_bind)) {
        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
        Log::info("Bound entity {} to prefab {}", entity.get_id(), pending_prefab_bind_path);
      }
      else {
        Log::error("Failed to bind entity {} to prefab {}", entity.get_id(), pending_prefab_bind_path);
      }
      pending_prefab_bind_path.clear();
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
      selected_entity_ids.clear();
      selected_entity_ids.insert(clonedEntity.get_id());
      selected_entity_id = static_cast<int>(clonedEntity.get_id());
      Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(clonedEntity.get_id());

      // Record undo for clone
      Engine::ECS::EntityID cloned_id = clonedEntity.get_id();
      Engine::ECS::Prefab cloned_prefab = Engine::ECS::entity_to_prefab(clonedEntity);
      Engine::UndoManager::record(
        // Do: Recreate the cloned entity
        [cloned_id, cloned_prefab]() {
          Engine::ECS::instantiate_prefab_with_id(cloned_prefab, cloned_id);
          Engine::EventManager::send_event<Engine::Events::SelectEntityEvent>(cloned_id);
          Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
        },
        // Undo: Delete the cloned entity
        [cloned_id]() {
          Engine::EventManager::send_event<Engine::Events::DeleteEntityEvent>(cloned_id, false);
          Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
        }
      );
      Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
    }

    ImGui::SameLine();

    // Save as Prefab (create new or update existing)
    if (ImGui::Button("Save as Prefab"))
    {
      ImGui::OpenPopup("SaveAsPrefabPicker");
    }

    // Prefab picker popup
    {
      if (ResourcePicker::prefab_picker_popup("SaveAsPrefabPicker", picked_prefab_path, picked_is_new)) {
        if (picked_is_new) {
          // --- Create new prefab from entity ---
          auto prefab = Engine::ECS::entity_to_prefab(entity);
          prefab.get_impl()->set_prefab_filepath(picked_prefab_path);

          auto& containers = Engine::ECS::Internal::get_containers();
          auto inst_it = containers.prefab_instances.find(entity.get_id());
          if (inst_it != containers.prefab_instances.end()) {
            inst_it->second.prefab_filepath = picked_prefab_path;
          }

          std::string full_path = Engine::Path(Engine::Resources::resource_path).join(picked_prefab_path).to_string();

          // Ensure prefabs directory exists
          std::string prefabs_dir = Engine::Path(Engine::Resources::resource_path).join("prefabs").to_string();
          if (!std::filesystem::exists(prefabs_dir)) {
            std::filesystem::create_directories(prefabs_dir);
          }

          if (Engine::FileIO::serialize_prefab_to_json_file(full_path, prefab)) {
            Engine::Resources::cache_prefab(picked_prefab_path, prefab, true, true);
            Log::info("Created prefab from entity: {}", picked_prefab_path);
          }
        }
        else {
          // --- Update existing prefab from entity ---
          std::string full_path = Engine::Path(Engine::Resources::resource_path).join(picked_prefab_path).to_string();

          Engine::ECS::Prefab existing_prefab = Engine::FileIO::deserialize_prefab_from_json_file(full_path);
          if (existing_prefab.is_valid()) {
            std::string preserved_uuid = existing_prefab.get_impl()->get_prefab_id();

            auto updated_prefab = Engine::ECS::entity_to_prefab(entity);
            updated_prefab.get_impl()->set_prefab_id(preserved_uuid);
            updated_prefab.get_impl()->set_prefab_filepath(picked_prefab_path);

            // Fix: entity_to_prefab registered the entity with a NEW uuid, re-point it to the preserved one
            auto& containers = Engine::ECS::Internal::get_containers();
            auto inst_it = containers.prefab_instances.find(entity.get_id());
            if (inst_it != containers.prefab_instances.end()) {
              inst_it->second.prefab_id = preserved_uuid;
              inst_it->second.prefab_filepath = picked_prefab_path;
            }

            if (Engine::FileIO::serialize_prefab_to_json_file(full_path, updated_prefab)) {
              Engine::Resources::cache_prefab(picked_prefab_path, updated_prefab, true, true);
              Engine::ECS::update_prefab_instances(preserved_uuid);
              Log::info("Updated prefab: {} (UUID: {})", picked_prefab_path, preserved_uuid);
            }
          }
          else {
            Log::error("Failed to load existing prefab: {}", picked_prefab_path);
          }
        }
      }
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

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Selection Export", ImGuiTreeNodeFlags_DefaultOpen))
    {
      const size_t selection_count = selected_entity_ids.size();
      ImGui::Text("Selected entities: %zu", selection_count);

      ImGui::Text("Output file path (relative to resources):");
      ImGui::InputText("##SelectionExportPath", selection_export_path_buf, sizeof(selection_export_path_buf));

      if (ImGui::Button("Save Selection to JSON", ImVec2(-1, 0)))
      {
        if (selection_count == 0)
        {
          selection_export_status = "Error: No entities selected";
          selection_export_status_time = 3.0f;
        }
        else
        {
          std::string rel_path = std::string(selection_export_path_buf);
          if (rel_path.empty())
          {
            selection_export_status = "Error: Output path cannot be empty";
            selection_export_status_time = 3.0f;
          }
          else
          {
            if (rel_path.find(".json") == std::string::npos)
            {
              rel_path += ".json";
            }

            // Deterministic ordering: sort by entity name then ID
            std::vector<Engine::ECS::EntityID> ids;
            ids.reserve(selection_count);
            for (Engine::ECS::EntityID id : selected_entity_ids)
            {
              ids.push_back(id);
            }
            std::sort(ids.begin(), ids.end(), [](Engine::ECS::EntityID a, Engine::ECS::EntityID b) {
              Engine::ECS::Entity ea = Engine::ECS::get_entity(a);
              Engine::ECS::Entity eb = Engine::ECS::get_entity(b);
              const std::string& na = ea.is_valid() ? Engine::ECS::get_entity_name(ea) : std::string();
              const std::string& nb = eb.is_valid() ? Engine::ECS::get_entity_name(eb) : std::string();
              if (na != nb) return na < nb;
              return a < b;
            });

            ReflectionLibrary::GenericValue root = ReflectionLibrary::Object();
            root["dataType"] = "EntitySelection";
            root["data"]["entities"] = ReflectionLibrary::Array();

            auto& containers = Engine::ECS::Internal::get_containers();

            for (Engine::ECS::EntityID id : ids)
            {
              Engine::ECS::Entity e = Engine::ECS::get_entity(id);
              if (!e.is_valid())
              {
                continue;
              }

              ReflectionLibrary::GenericValue entry = ReflectionLibrary::Object();
              entry["id"] = static_cast<int>(id);

              const std::string& name = Engine::ECS::get_entity_name(e);
              if (!name.empty())
              {
                entry["name"] = name;
              }

              if (e.has_component<Components::Transform2D>())
              {
                const auto* t = e.get_component<Components::Transform2D>();
                entry["transform"]["position"]["x"] = t->position.x;
                entry["transform"]["position"]["y"] = t->position.y;
                entry["transform"]["rotation"] = t->rotation;
                entry["transform"]["scale"]["x"] = t->scale.x;
                entry["transform"]["scale"]["y"] = t->scale.y;
              }

              // If this is a prefab instance, store prefab filepath for spawn tooling
              auto prefab_it = containers.prefab_instances.find(id);
              if (prefab_it != containers.prefab_instances.end())
              {
                const auto& instance_data = prefab_it->second;
                if (!instance_data.prefab_filepath.empty())
                {
                  entry["prefab"] = instance_data.prefab_filepath;
                }
                entry["prefab_id"] = instance_data.prefab_id;
              }

              root["data"]["entities"].push_back(entry);
            }

            const std::string json = ReflectionLibrary::generic_value_to_json(root, true);
            const std::string full_path = Engine::Path(Engine::Resources::resource_path).join(rel_path).to_string();
            if (Engine::FileIO::write_string_to_file(full_path, json))
            {
              selection_export_status = "Saved selection to " + rel_path;
              selection_export_status_time = 3.0f;
              Log::info("Entity selection saved: {} ({} entities)", rel_path, selection_count);
            }
            else
            {
              selection_export_status = "Error: Failed to write file";
              selection_export_status_time = 3.0f;
              Log::error("Failed to save entity selection: {}", rel_path);
            }
          }
        }
      }

      if (selection_export_status_time > 0.0f)
      {
        ImGui::Separator();
        ImGui::TextWrapped("%s", selection_export_status.c_str());
        selection_export_status_time -= ImGui::GetIO().DeltaTime;
      }
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
  }

  ImGui::Separator();

  // Rename popup (rendered at window level so it works from any context menu)
  if (open_rename_popup) {
    ImGui::OpenPopup("RenameEntityPopup");
    open_rename_popup = false;
  }
  if (ImGui::BeginPopup("RenameEntityPopup")) {
    ImGui::Text("Rename Entity %zu", rename_entity_id);
    ImGui::Separator();
    if (!rename_focus_set) {
      ImGui::SetKeyboardFocusHere();
      rename_focus_set = true;
    }
    bool confirmed = ImGui::InputText("##RenameInput", rename_buf, sizeof(rename_buf), ImGuiInputTextFlags_EnterReturnsTrue);
    if (confirmed) {
      Engine::ECS::Entity rename_entity = Engine::ECS::get_entity(rename_entity_id);
      if (rename_entity.is_valid()) {
        std::string old_name = Engine::ECS::get_entity_name(rename_entity);
        std::string new_name = rename_buf;
        Engine::ECS::EntityID eid = rename_entity_id;

        Engine::ECS::set_entity_name(rename_entity, new_name);

        Engine::UndoManager::record(
          // Redo: apply new name
          [eid, new_name]() {
            Engine::ECS::Entity e = Engine::ECS::get_entity(eid);
            if (e.is_valid()) Engine::ECS::set_entity_name(e, new_name);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          },
          // Undo: restore old name
          [eid, old_name]() {
            Engine::ECS::Entity e = Engine::ECS::get_entity(eid);
            if (e.is_valid()) Engine::ECS::set_entity_name(e, old_name);
            Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
          }
        );

        Engine::ProjectSaveManager::set_dirty(Engine::ProjectSaveManager::DirtyStates::Level(Engine::LevelManager::get_current_level_path()));
      }
      rename_entity_id = Engine::ECS::INVALID_ENTITY;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  ImGui::End();
}

