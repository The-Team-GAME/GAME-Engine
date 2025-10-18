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
//          entity and component manipulation. Provides a GUI for creating, deleting,
//          and editing entities and their components during gameplay.
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
#include "../Math/Vector3D.h"

// Helper to render fields recursively using reflection
template<typename T>
inline void RenderReflectedField(const char* label, T& value, int depth = 0)
{
  using ValueType = std::decay_t<T>;

  if constexpr (std::is_same_v<ValueType, bool>)
  {
    ImGui::Checkbox(label, &value);
  }
  else if constexpr (std::is_integral_v<ValueType> && !std::is_same_v<ValueType, bool>)
  {
    if constexpr (sizeof(ValueType) <= 4)
      ImGui::DragInt(label, reinterpret_cast<int*>(&value));
    else
      ImGui::InputScalar(label, ImGuiDataType_S64, &value);
  }
  else if constexpr (std::is_floating_point_v<ValueType>)
  {
    ImGui::DragFloat(label, &value, 0.01f);
  }
  else if constexpr (std::is_same_v<ValueType, std::string>)
  {
    char buffer[1024];
    strncpy_s(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    if (ImGui::InputText(label, buffer, sizeof(buffer)))
    {
      value = buffer;
    }
  }
  else if constexpr (std::is_same_v<ValueType, Math::Vec2D>)
  {
    float vec[2] = { value.x, value.y };
    if (ImGui::DragFloat2(label, vec, 0.01f))
    {
      value.x = vec[0];
      value.y = vec[1];
    }
  }
  else if constexpr (std::is_same_v<ValueType, Math::Vec3D>)
  {
    float vec[3] = { value.x, value.y, value.z };
    if (ImGui::DragFloat3(label, vec, 0.01f))
    {
      value.x = vec[0];
      value.y = vec[1];
      value.z = vec[2];
    }
  }
  else if constexpr (std::is_same_v<ValueType, Math::Vec4D>)
  {
    float vec[4] = { value.x, value.y, value.z, value.w };
    if (ImGui::DragFloat4(label, vec, 0.01f))
    {
      value.x = vec[0];
      value.y = vec[1];
      value.z = vec[2];
      value.w = vec[3];
    }
  }
  else if constexpr (ReflectionLibrary::is_reflectable_v<ValueType>)
  {
    if (ImGui::TreeNode(label))
    {
      ReflectionLibrary::for_each_field(value, [&](std::string_view name, auto& field_value, int)
        {
          std::string field_label = std::string(name) + "##" + std::to_string(depth);
          RenderReflectedField(field_label.c_str(), field_value, depth + 1);
        });
      ImGui::TreePop();
    }
  }
  else if constexpr (ReflectionLibrary::is_iterable_v<ValueType>)
  {
    if (ImGui::TreeNode(label))
    {
      int idx = 0;
      for (auto& elem : value)
      {
        std::string elem_label = std::string(label) + "[" + std::to_string(idx) + "]##" + std::to_string(depth);
        RenderReflectedField(elem_label.c_str(), elem, depth + 1);
        idx++;
      }
      ImGui::TreePop();
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s: [unsupported type]", label);
  }
}

// Specialized rendering for Color component
template<>
inline void RenderReflectedField<Components::Color>(const char* label, Components::Color& color, int depth)
{
  float colorArray[4] = { color.r, color.g, color.b, color.a };
  if (ImGui::ColorEdit4(label, colorArray))
  {
    color.r = colorArray[0];
    color.g = colorArray[1];
    color.b = colorArray[2];
    color.a = colorArray[3];
  }
}

// Specialized rendering for Transform2D
template<>
inline void RenderReflectedField<Components::Transform2D>(const char* label, Components::Transform2D& transform, int depth)
{
  if (ImGui::TreeNode(label))
  {
    ImGui::Text("Position:");
    ImGui::DragFloat("X##pos", &transform.position.x, 1.0f);
    ImGui::DragFloat("Y##pos", &transform.position.y, 1.0f);

    ImGui::Separator();
    ImGui::Text("Rotation:");
    ImGui::SliderFloat("Angle (degrees)", &transform.rotation, 0.0f, 360.0f);

    ImGui::Separator();
    ImGui::Text("Scale:");
    ImGui::DragFloat("X##scale", &transform.scale.x, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("Y##scale", &transform.scale.y, 0.01f, 0.01f, 10.0f);

    if (ImGui::Button("Reset Transform"))
    {
      transform.position = Math::Vec2D(0.0f, 0.0f);
      transform.rotation = 0.0f;
      transform.scale = Math::Vec2D(1.0f, 1.0f);
    }

    ImGui::TreePop();
  }
}

// Template function to render any component using reflection
template<typename ComponentType>
inline void RenderComponent(Engine::ECS::Entity& entity, const char* component_name)
{
  if (!entity.has_component<ComponentType>())
    return;

  if (ImGui::TreeNode(component_name))
  {
    auto* component = entity.get_component<ComponentType>();

    if constexpr (ReflectionLibrary::is_reflectable_v<ComponentType>)
    {
      ReflectionLibrary::for_each_field(*component, [&](std::string_view name, auto& field_value, int)
        {
          std::string label = std::string(name);
          RenderReflectedField(label.c_str(), field_value, 0);
        });
    }
    else
    {
      // Fallback for non-reflectable components
      RenderReflectedField(component_name, *component, 0);
    }

    if (ImGui::Button(("Remove##" + std::string(component_name)).c_str()))
    {
      entity.remove_component<ComponentType>();
    }

    ImGui::TreePop();
  }
}

// Add this helper to render and edit JSON values
inline bool RenderEditableJsonValue(const char* label, rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator)
{
  bool modified = false;

  if (value.IsInt())
  {
    int temp = value.GetInt();
    if (ImGui::DragInt(label, &temp))
    {
      value.SetInt(temp);
      modified = true;
    }
  }
  else if (value.IsDouble())
  {
    double temp = value.GetDouble();
    float tempFloat = static_cast<float>(temp);
    if (ImGui::DragFloat(label, &tempFloat, 0.01f))
    {
      value.SetDouble(static_cast<double>(tempFloat));
      modified = true;
    }
  }
  else if (value.IsBool())
  {
    bool temp = value.GetBool();
    if (ImGui::Checkbox(label, &temp))
    {
      value.SetBool(temp);
      modified = true;
    }
  }
  else if (value.IsString())
  {
    std::string temp = value.GetString();
    char buffer[1024];
    strncpy_s(buffer, temp.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    if (ImGui::InputText(label, buffer, sizeof(buffer)))
    {
      value.SetString(buffer, allocator);
      modified = true;
    }
  }
  else if (value.IsObject())
  {
    if (ImGui::TreeNode(label))
    {
      for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
      {
        std::string fieldName = it->name.GetString();
        if (RenderEditableJsonValue(fieldName.c_str(), it->value, allocator))
        {
          modified = true;
        }
      }
      ImGui::TreePop();
    }
  }
  else if (value.IsArray())
  {
    if (ImGui::TreeNode(label))
    {
      for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
      {
        std::string elemLabel = std::string(label) + "[" + std::to_string(i) + "]";
        if (RenderEditableJsonValue(elemLabel.c_str(), value[i], allocator))
        {
          modified = true;
        }
      }
      ImGui::TreePop();
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s: [unsupported type]", label);
  }

  return modified;
}

inline void RenderAllComponents(Engine::ECS::Entity& entity)
{
  using namespace Engine::ECS;

  auto entityId = entity.get_id();
  const auto& signature = Internal::entity_signatures[entityId];

  for (const auto& [typeId, componentArray] : Internal::component_arrays)
  {
    if (signature.test(typeId))
    {
      std::string componentName = componentArray->get_type_name();

      if (ImGui::TreeNode(componentName.c_str()))
      {
        if (componentArray->is_reflectable())
        {
          // Serialize component to JSON
          rapidjson::Document doc;
          doc.SetObject();
          auto& allocator = doc.GetAllocator();
          rapidjson::Value componentData(rapidjson::kObjectType);

          componentArray->serialize_component(entityId, componentData, allocator);

          // Render and edit fields from JSON
          bool modified = false;
          for (auto it = componentData.MemberBegin(); it != componentData.MemberEnd(); ++it)
          {
            std::string fieldName = it->name.GetString();
            if (RenderEditableJsonValue(fieldName.c_str(), it->value, allocator))
            {
              modified = true;
            }
          }

          // If any field was modified, deserialize back to component
          if (modified)
          {
            // Remove and re-add component with new data
            componentArray->remove_component(entityId);
            componentArray->deserialize_and_add_component(entityId, componentData);

            // Notify systems about component change
            Internal::entity_component_changed(entityId);
          }
        }
        else
        {
          ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Non-reflectable component");
        }

        if (ImGui::Button(("Remove##" + componentName).c_str()))
        {
          componentArray->remove_component(entityId);
          //signature.reset(typeId);
          Internal::entity_component_changed(entityId);
        }

        ImGui::TreePop();
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
      if (ImGui::Button("Add Transform2D"))
      {
        if (!entity.has_component<Components::Transform2D>())
          entity.add_component<Components::Transform2D>();
      }

      ImGui::SameLine();

      if (ImGui::Button("Add Color"))
      {
        if (!entity.has_component<Components::Color>())
          entity.add_component<Components::Color>();
      }

      if (ImGui::Button("Add Text"))
      {
        if (!entity.has_component<Components::Text>())
          entity.add_component<Components::Text>("Sample Text");
      }
    }

    ImGui::Separator();

    // Edit Components Section - Automatically render all components
    if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
    {
      RenderAllComponents(entity);
    }

    ImGui::Separator();

    // Clone Entity
    if (ImGui::Button("Clone This Entity"))
    {
      auto clonedEntity = Engine::ECS::clone_entity(entity.get_id());
      selected_entity_id = static_cast<int>(clonedEntity.get_id());
    }

    ImGui::SameLine();

    // Save as Prefab
    if (ImGui::Button("Save as Prefab"))
    {
      // Create a prefab from the current entity
      auto prefab = Engine::ECS::create_prefab([entity_id = entity.get_id()](Engine::ECS::Entity& new_entity)
        {
          auto source = Engine::ECS::get_entity(entity_id);
          // Copy all components to the new entity
          // This will be handled by clone_entity logic
        });

      std::string json = Engine::ECS::serialize_prefab_to_json(prefab);
      ImGui::SetClipboardText(json.c_str());
      ImGui::OpenPopup("Prefab Saved");
    }

    if (ImGui::BeginPopup("Prefab Saved"))
    {
      ImGui::Text("Prefab JSON copied to clipboard!");
      ImGui::EndPopup();
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
  }

  ImGui::End();
}