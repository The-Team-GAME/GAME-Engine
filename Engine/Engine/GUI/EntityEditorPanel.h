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

  // Render component using reflection from GenericValue
  void render_component_from_generic(const std::string& component_name,
    Engine::ECS::Entity& entity,
    Engine::ECS::IComponentArray* array) {

    if (ImGui::TreeNode(component_name.c_str())) {

      // Get the component data as GenericValue
      auto component_data = array->to_generic_value(entity.get_id());
      bool modified = false;

      // Render fields from GenericValue
      if (component_data.isObject()) {
        auto& fields = component_data.getObject();
        for (auto& [field_name, field_value] : fields) {
          std::string display_name = field_name;

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
            // Handle nested objects (like Vec2D, Vec4D)
            if (ImGui::TreeNode(display_name.c_str())) {
              auto& nested_fields = field_value.getObject();
              for (auto& [nested_name, nested_value] : nested_fields) {
                if (nested_value.isDouble()) {
                  float nested_val = static_cast<float>(nested_value.getDouble());
                  std::string nested_label = std::string(nested_name);
                  if (ImGui::DragFloat(nested_label.c_str(), &nested_val, 0.01f)) {
                    nested_value = static_cast<double>(nested_val);
                    modified = true;
                  }
                }
                else if (nested_value.isInt()) {
                  int nested_val = static_cast<int>(nested_value.getInt());
                  std::string nested_label = std::string(nested_name);
                  if (ImGui::DragInt(nested_label.c_str(), &nested_val)) {
                    nested_value = static_cast<ReflectionLibrary::Integer>(nested_val);
                    modified = true;
                  }
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
        }
      }

      // Write back changes if modified
      if (modified) {
        array->set_component_from_generic_value(entity.get_id(), component_data);
      }

      ImGui::Separator();

      std::string remove_label = "Remove##" + component_name;
      if (ImGui::Button(remove_label.c_str())) {
        array->remove_entity(entity.get_id());
        // Notify systems about component change
        Engine::ECS::Internal::entity_component_changed(entity.get_id());
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
        // Render the component editor with write-back support
        render_component_from_generic(type_name, entity, array);
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