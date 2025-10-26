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
  // Helper to render a field in ImGui based on its type
  template<typename T>
  void render_field_editor(std::string_view name, T& value) {
    using ValueType = std::decay_t<T>;

    std::string label = std::string(name);

    if constexpr (std::is_same_v<ValueType, bool>) {
      ImGui::Checkbox(label.c_str(), &value);
    }
    else if constexpr (std::is_same_v<ValueType, s32> || std::is_same_v<ValueType, u32>) {
      ImGui::DragInt(label.c_str(), reinterpret_cast<int*>(&value), 1.0f);
    }
    else if constexpr (std::is_same_v<ValueType, f32>) {
      ImGui::DragFloat(label.c_str(), &value, 0.01f);
    }
    else if constexpr (std::is_same_v<ValueType, f64>) {
      float temp = static_cast<float>(value);
      if (ImGui::DragFloat(label.c_str(), &temp, 0.01f)) {
        value = static_cast<double>(temp);
      }
    }
    else if constexpr (std::is_same_v<ValueType, std::string>) {
      char buffer[1024];
      strncpy_s(buffer, value.c_str(), sizeof(buffer) - 1);
      buffer[sizeof(buffer) - 1] = '\0';
      if (ImGui::InputText(label.c_str(), buffer, sizeof(buffer))) {
        value = buffer;
      }
    }
    else if constexpr (ReflectionLibrary::is_reflectable_v<ValueType>) {
      // Nested reflectable type
      if (ImGui::TreeNode(label.c_str())) {
        ReflectionLibrary::for_each_field(value, [](std::string_view sub_name, auto& sub_value, int) {
          render_field_editor(sub_name, sub_value);
          }, 0);
        ImGui::TreePop();
      }
    }
    else if constexpr (ReflectionLibrary::is_iterable_v<ValueType>) {
      ImGui::Text("%s: [%zu elements]", label.c_str(), value.size());
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
        "Note: Collection editing not yet supported");
    }
    else {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
        "%s: [%s]", label.c_str(), typeid(ValueType).name());
    }
  }

  // Render component using reflection from GenericValue
  void render_component_from_generic(const std::string& component_name,
    const ReflectionLibrary::GenericValue& component_data,
    Engine::ECS::Entity& entity,
    Engine::ECS::ComponentTypeID type_id) {
    if (ImGui::TreeNode(component_name.c_str())) {

      // Render fields from GenericValue
      if (component_data.isObject()) {
        const auto& fields = component_data.getObject();
        for (const auto& [field_name, field_value] : fields) {
          // Simple display for now - can be enhanced to support editing
          std::string display_name = field_name;

          if (field_value.isBool()) {
            bool val = field_value.getBool();
            ImGui::Checkbox(display_name.c_str(), &val);
          }
          else if (field_value.isInt()) {
            int val = static_cast<int>(field_value.getInt());
            ImGui::DragInt(display_name.c_str(), &val);
          }
          else if (field_value.isDouble()) {
            float val = static_cast<float>(field_value.getDouble());
            ImGui::DragFloat(display_name.c_str(), &val, 0.01f);
          }
          else if (field_value.isString()) {
            ImGui::Text("%s: %s", display_name.c_str(), field_value.getString().c_str());
          }
          else if (field_value.isObject()) {
            ImGui::Text("%s: [Object]", display_name.c_str());
          }
          else if (field_value.isArray()) {
            ImGui::Text("%s: [Array with %zu elements]", display_name.c_str(),
              field_value.getArray().size());
          }
        }
      }

      ImGui::Separator();

      std::string remove_label = "Remove##" + component_name;
      if (ImGui::Button(remove_label.c_str())) {
        // Need to implement component removal by type_id
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
          "Component removal by type ID not yet implemented");
      }

      ImGui::TreePop();
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
      if (ImGui::Button("Add Transform2D") && !entity.has_component<Components::Transform2D>()) {
        entity.add_component<Components::Transform2D>();
      }
      ImGui::SameLine();
      if (ImGui::Button("Add Color") && !entity.has_component<Components::Color>()) {
        entity.add_component<Components::Color>();
      }

      if (ImGui::Button("Add Text") && !entity.has_component<Components::Text>()) {
        entity.add_component<Components::Text>();
      }
      ImGui::SameLine();
      if (ImGui::Button("Add Texture") && !entity.has_component<Components::Texture>()) {
        entity.add_component<Components::Texture>();
      }

      if (ImGui::Button("Add TextureAtlas") && !entity.has_component<Components::TextureAtlas>()) {
        entity.add_component<Components::TextureAtlas>();
      }
      ImGui::SameLine();
      if (ImGui::Button("Add Animation") && !entity.has_component<Components::Animation>()) {
        entity.add_component<Components::Animation>();
      }

      if (ImGui::Button("Add Font") && !entity.has_component<Components::Font>()) {
        entity.add_component<Components::Font>();
      }
    }

    ImGui::Separator();

    // Edit Components Section - NOW FULLY AUTOMATIC!
    if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
    {
      // Automatically iterate through ALL components the entity has
      entity.for_each_component([&entity](const std::string& type_name, Engine::ECS::IComponentArray* array) {
        // Get the component data as GenericValue
        auto component_data = array->to_generic_value(entity.get_id());

        // Get the type ID from the array
        Engine::ECS::ComponentTypeID type_id = 0; // You'll need to expose this from the array

        // Render the component editor
        render_component_from_generic(type_name, component_data, entity, type_id);
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