//-----------------------------------------------------------------------------------
// file:    EntityEditorPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the EntityEditorPanel ImGui interface for runtime
//          entity and component manipulation. Uses reflection to automatically
//          generate component editor fields.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "imgui.h"
#include "Core.h"
#include "../Components/Components.h"
#include "../Reflection/ReflectionLibrary.h"
#include <string>
#include <vector>
#include "../ECS/ECS.h"

namespace {
  // Recursive helper to render any GenericValue field
  bool render_generic_value_field(const std::string& display_name, ReflectionLibrary::GenericValue& field_value) {
    bool modified = false;

    if (field_value.isBool()) {
      bool val = field_value.getBool();
      if (ImGui::Checkbox(display_name.c_str(), &val)) {
        field_value = val;
        modified = true;
      }
    }
    else if (field_value.isInt()) {
      int val = static_cast<int>(field_value.getInt());
      if (ImGui::DragInt(display_name.c_str(), &val)) {
        field_value = static_cast<ReflectionLibrary::Integer>(val);
        modified = true;
      }
    }
    else if (field_value.isDouble()) {
      float val = static_cast<float>(field_value.getDouble());
      if (ImGui::DragFloat(display_name.c_str(), &val, 0.01f)) {
        field_value = static_cast<double>(val);
        modified = true;
      }
    }
    else if (field_value.isString()) {
      char buffer[1024];
      std::string str_val = field_value.getString();
      strncpy_s(buffer, str_val.c_str(), sizeof(buffer) - 1);
      buffer[sizeof(buffer) - 1] = '\0';
      if (ImGui::InputText(display_name.c_str(), buffer, sizeof(buffer))) {
        field_value = std::string(buffer);
        modified = true;
      }
    }
    else if (field_value.isObject()) {
      // Recursively handle nested objects
      if (ImGui::TreeNode(display_name.c_str())) {
        auto& nested_fields = field_value.getObject();
        for (auto& [nested_name, nested_value] : nested_fields) {
          if (render_generic_value_field(nested_name, nested_value)) {
            modified = true;
          }
        }
        ImGui::TreePop();
      }
    }
    else if (field_value.isArray()) {
      ImGui::Text("%s: [Array with %zu elements]", display_name.c_str(),
        field_value.getArray().size());
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
        "Array editing not yet supported");
    }

    return modified;
  }

  // For rendering specialized components that need custom handling
  bool render_component_from_generic_specialized(const std::string& component_name, Engine::ECS::Entity& entity)
  {
    if (component_name == "Components::Color") {
      Components::Color* colorComp = entity.get_component<Components::Color>();
      if (colorComp) {
        ImVec4 colorVec(colorComp->r, colorComp->g, colorComp->b, colorComp->a);
        if (ImGui::ColorEdit4("Color", (float*)&colorVec)) {
          colorComp->r = colorVec.x;
          colorComp->g = colorVec.y;
          colorComp->b = colorVec.z;
          colorComp->a = colorVec.w;
        }
        return true;
      }
      return true;
    }
    return false;
  }

  // Render component using reflection from GenericValue
  bool render_component_from_generic(const std::string& component_name,
    Engine::ECS::Entity& entity,
    Engine::ECS::IComponentArray* array)
  {

    if (ImGui::TreeNode(component_name.c_str())) {

      //// Cache the component data (only fetch once per TreeNode open)
      //static std::unordered_map<std::string, ReflectionLibrary::GenericValue> component_cache;
      //std::string cache_key = std::to_string(entity.get_id()) + "_" + component_name;

      //// Only fetch if not in cache or if this is a new frame
      //if (component_cache.find(cache_key) == component_cache.end()) {
      //  component_cache[cache_key] = array->to_generic_value(entity.get_id());
      //}
      //auto& component_data = component_cache[cache_key];

      auto component_data = array->to_generic_value(entity.get_id());
      bool modified = false;

      // Render fields from GenericValue
      if (component_data.isObject()) {
        auto& fields = component_data.getObject();
        for (auto& [field_name, field_value] : fields) {
          if (render_generic_value_field(field_name, field_value)) {
            modified = true;
          }
        }
      }

      // Write back changes if modified
      if (modified) {
        std::cout << "Component " << component_name << " modified on Entity "
          << entity.get_id() << std::endl;
        array->set_component_from_generic_value(entity.get_id(), component_data);
        //component_cache[cache_key] = component_data;
      }

      ImGui::Separator();

      std::string remove_label = "Remove##" + component_name;
      if (ImGui::Button(remove_label.c_str())) {
        auto& containers = Engine::ECS::Internal::get_containers();
        Engine::ECS::EntityID entityId = entity.get_id();

        // Add to removal queue instead of removing immediately
        containers.components_to_remove.emplace_back(entityId, array->get_type_id());
      }

      ImGui::TreePop();
    }
    else {
      // TreeNode is closed, clear cache for this component
      //static std::unordered_map<std::string, ReflectionLibrary::GenericValue> component_cache;
      //std::string cache_key = std::to_string(entity.get_id()) + "_" + component_name;
      //component_cache.erase(cache_key);
    }
  }
}

inline void EntityEditorPanel()
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

    // Edit Components Section - NOW FULLY AUTOMATIC!
    if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
    {
      // Automatically iterate through ALL components the entity has
      entity.for_each_component([&entity](const std::string& type_name, Engine::ECS::IComponentArray* array) {
        if (!array->is_reflectable()) {
          return; // Skip non-reflectable components
        }

        // First try specialized rendering
        // If not handled, fallback to generic reflection-based rendering
        render_component_from_generic_specialized(type_name, entity) || render_component_from_generic(type_name, entity, array);
        });
    }

    ImGui::Separator();

    // Clone Entity
    if (ImGui::Button("Clone This Entity"))
    {
      auto clonedEntity = Engine::ECS::clone_entity(entity);
      selected_entity_id = static_cast<int>(clonedEntity.get_id());
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