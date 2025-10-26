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

  // Render component editor using reflection
  template<typename T>
  void render_component_editor(Engine::ECS::Entity& entity, const char* component_name) {
    if (!entity.has_component<T>())
      return;

    if (ImGui::TreeNode(component_name)) {
      auto* component = entity.get_component<T>();

      if constexpr (ReflectionLibrary::is_reflectable_v<T>) {
        // Use reflection to iterate over fields
        ReflectionLibrary::for_each_field(*component, [](std::string_view name, auto& value, int) {
          render_field_editor(name, value);
          }, 0);
      }
      else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
          "Component not reflectable");
      }

      ImGui::Separator();

      std::string remove_label = std::string("Remove##") + component_name;
      if (ImGui::Button(remove_label.c_str())) {
        entity.remove_component<T>();
      }

      ImGui::TreePop();
    }
  }

  // Helper to add components with type checking
  template<typename T>
  void add_component_button(Engine::ECS::Entity& entity, const char* button_label) {
    if (ImGui::Button(button_label)) {
      if (!entity.has_component<T>()) {
        if constexpr (std::is_default_constructible_v<T>) {
          entity.add_component<T>();
        }
        else {
          ImGui::OpenPopup("Cannot Add Component");
        }
      }
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
      add_component_button<Components::Transform2D>(entity, "Add Transform2D");
      ImGui::SameLine();
      add_component_button<Components::Color>(entity, "Add Color");

      add_component_button<Components::Text>(entity, "Add Text");
      ImGui::SameLine();
      add_component_button<Components::Texture>(entity, "Add Texture");

      add_component_button<Components::TextureAtlas>(entity, "Add TextureAtlas");
      ImGui::SameLine();
      add_component_button<Components::Animation>(entity, "Add Animation");

      add_component_button<Components::Font>(entity, "Add Font");
    }

    ImGui::Separator();

    // Edit Components Section
    if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
    {
      // Automatically generate editors for all registered components
      render_component_editor<Components::Transform2D>(entity, "Transform2D");
      render_component_editor<Components::Color>(entity, "Color");
      render_component_editor<Components::Text>(entity, "Text");
      render_component_editor<Components::Texture>(entity, "Texture");
      render_component_editor<Components::TextureAtlas>(entity, "TextureAtlas");
      render_component_editor<Components::Animation>(entity, "Animation");
      render_component_editor<Components::Font>(entity, "Font");
    }

    ImGui::Separator();

    // Clone Entity
    if (ImGui::Button("Clone This Entity"))
    {
      auto clonedEntity = Engine::ECS::clone_entity(entity.get_id());
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