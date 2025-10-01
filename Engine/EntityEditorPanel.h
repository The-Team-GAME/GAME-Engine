#pragma once
#include "imgui.h"
#include "Core.h"
#include "BasicComponents.h"
#include <string>
#include <vector>

inline void EntityEditorPanel()
{
  static s32 selectedEntityId = -1;
  static char entitySearchBuffer[256] = "";

  ImGui::Begin("Entity Editor");

  // Section 1: Entity List
  if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("Total Entities: %zu", Engine::WORLD.getEntityCount());
    
    // Search/Filter
    ImGui::InputText("Search", entitySearchBuffer, 256);
    
    ImGui::Separator();
    
    // Entity list with selection
    ImGui::BeginChild("EntityList", ImVec2(0, 200), true);
    
    auto allEntities = Engine::WORLD.getAllEntities();
    for (const auto& entity : allEntities)
    {
      bool isSelected = (selectedEntityId == static_cast<int>(entity.getId()));
      std::string label = "Entity " + std::to_string(entity.getId());
      
      if (ImGui::Selectable(label.c_str(), isSelected))
      {
        selectedEntityId = static_cast<int>(entity.getId());
      }
    }
    
    ImGui::EndChild();
    
    // Create new entity button
    if (ImGui::Button("Create New Entity"))
    {
      auto newEntity = Engine::WORLD.create_entity();
      selectedEntityId = static_cast<int>(newEntity.getId());
    }
    
    ImGui::SameLine();
    
    // Delete selected entity
    if (selectedEntityId != -1 && ImGui::Button("Delete Selected"))
    {
      Engine::WORLD.destory_entity(selectedEntityId);
      selectedEntityId = -1;
    }
  }

  ImGui::Separator();

  // Section 2: Component Editor for Selected Entity
  if (selectedEntityId != -1)
  {
    auto entity = Engine::WORLD.get_entity(selectedEntityId);
    
    if (!entity.isValid())
    {
      selectedEntityId = -1;
      ImGui::End();
      return;
    }

    ImGui::Text("Editing Entity: %d", selectedEntityId);
    ImGui::Separator();

    // Add Component Section
    if (ImGui::CollapsingHeader("Add Component"))
    {
      if (ImGui::Button("Add Position"))
      {
        if (!entity.hasComponent<Components::Position>())
          entity.addComponent<Components::Position>(0.0f, 0.0f);
      }
      
      ImGui::SameLine();
      if (ImGui::Button("Add Velocity"))
      {
        if (!entity.hasComponent<Components::Velocity>())
          entity.addComponent<Components::Velocity>(0.0f, 0.0f);
      }
      
      if (ImGui::Button("Add Size"))
      {
        if (!entity.hasComponent<Components::Size>())
          entity.addComponent<Components::Size>(100.0f, 100.0f);
      }
      
      ImGui::SameLine();
      if (ImGui::Button("Add Color"))
      {
        if (!entity.hasComponent<Components::Color>())
          entity.addComponent<Components::Color>(1.0f, 1.0f, 1.0f, 1.0f);
      }
      
      if (ImGui::Button("Add Rotation"))
      {
        if (!entity.hasComponent<Components::Rotation>())
          entity.addComponent<Components::Rotation>(0.0f);
      }
      
      ImGui::SameLine();
      if (ImGui::Button("Add Scale"))
      {
        if (!entity.hasComponent<Components::Scale>())
          entity.addComponent<Components::Scale>(1.0f, 1.0f);
      }
      
      if (ImGui::Button("Add Health"))
      {
        if (!entity.hasComponent<Components::Health>())
          entity.addComponent<Components::Health>(100, 100);
      }
      
      ImGui::SameLine();
      if (ImGui::Button("Add RotationSpeed"))
      {
        if (!entity.hasComponent<Components::RotationSpeed>())
          entity.addComponent<Components::RotationSpeed>(0.0f);
      }
    }

    ImGui::Separator();

    // Edit Components Section
    if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
    {
      // Position Component
      if (entity.hasComponent<Components::Position>())
      {
        if (ImGui::TreeNode("Position"))
        {
          auto* pos = entity.get_component<Components::Position>();
          ImGui::DragFloat("X##pos", &pos->X(), 1.0f);
          ImGui::DragFloat("Y##pos", &pos->Y(), 1.0f);
          
          if (ImGui::Button("Remove##position"))
            entity.removeComponent<Components::Position>();
          
          ImGui::TreePop();
        }
      }

      // Velocity Component
      if (entity.hasComponent<Components::Velocity>())
      {
        if (ImGui::TreeNode("Velocity"))
        {
          auto* vel = entity.get_component<Components::Velocity>();
          ImGui::DragFloat("X##vel", &vel->X(), 1.0f);
          ImGui::DragFloat("Y##vel", &vel->Y(), 1.0f);
          
          if (ImGui::Button("Remove##velocity"))
            entity.removeComponent<Components::Velocity>();
          
          ImGui::TreePop();
        }
      }

      // Size Component
      if (entity.hasComponent<Components::Size>())
      {
        if (ImGui::TreeNode("Size"))
        {
          auto* size = entity.get_component<Components::Size>();
          ImGui::DragFloat("Width", &size->width, 1.0f, 0.0f, 10000.0f);
          ImGui::DragFloat("Height", &size->height, 1.0f, 0.0f, 10000.0f);
          
          if (ImGui::Button("Remove##size"))
            entity.removeComponent<Components::Size>();
          
          ImGui::TreePop();
        }
      }

      // Color Component
      if (entity.hasComponent<Components::Color>())
      {
        if (ImGui::TreeNode("Color"))
        {
          auto* color = entity.get_component<Components::Color>();
          f32 colorArray[4] = { color->X(), color->Y(), color->Z(), color->W()};
          
          if (ImGui::ColorEdit4("RGBA", colorArray))
          {
            color->X() = colorArray[0];
            color->Y() = colorArray[1];
            color->Z() = colorArray[2];
            color->W() = colorArray[3];
          }
          
          if (ImGui::Button("Remove##color"))
            entity.removeComponent<Components::Color>();
          
          ImGui::TreePop();
        }
      }

      // Rotation Component
      if (entity.hasComponent<Components::Rotation>())
      {
        if (ImGui::TreeNode("Rotation"))
        {
          auto* rot = entity.get_component<Components::Rotation>();
          ImGui::SliderFloat("Angle (degrees)", &rot->angle, 0.0f, 360.0f);
          
          if (ImGui::Button("Remove##rotation"))
            entity.removeComponent<Components::Rotation>();
          
          ImGui::TreePop();
        }
      }

      // Scale Component
      if (entity.hasComponent<Components::Scale>())
      {
        if (ImGui::TreeNode("Scale"))
        {
          auto* scale = entity.get_component<Components::Scale>();
          ImGui::DragFloat("X##scale", &scale->X(), 0.01f, 0.01f, 10.0f);
          ImGui::DragFloat("Y##scale", &scale->Y(), 0.01f, 0.01f, 10.0f);
          
          if (ImGui::Button("Uniform Scale"))
          {
            f32 uniform = (scale->X() + scale->Y()) / 2.0f;
            scale->X() = scale->Y() = uniform;
          }
          
          if (ImGui::Button("Remove##scale"))
            entity.removeComponent<Components::Scale>();
          
          ImGui::TreePop();
        }
      }

      // Health Component
      if (entity.hasComponent<Components::Health>())
      {
        if (ImGui::TreeNode("Health"))
        {
          auto* health = entity.get_component<Components::Health>();
          ImGui::DragInt("Current", &health->value, 1, 0, health->max);
          ImGui::DragInt("Max", &health->max, 1, 1, 10000);
          
          ImGui::ProgressBar(static_cast<f32>(health->value) / health->max);
          
          if (ImGui::Button("Heal 10"))
            health->heal(10);
          
          ImGui::SameLine();
          if (ImGui::Button("Damage 10"))
            health->damage(10);
          
          if (ImGui::Button("Remove##health"))
            entity.removeComponent<Components::Health>();
          
          ImGui::TreePop();
        }
      }

      // RotationSpeed Component
      if (entity.hasComponent<Components::RotationSpeed>())
      {
        if (ImGui::TreeNode("RotationSpeed"))
        {
          auto* rotSpeed = entity.get_component<Components::RotationSpeed>();
          ImGui::DragFloat("Degrees/Second", &rotSpeed->degrees_per_second, 1.0f, -360.0f, 360.0f);
          
          if (ImGui::Button("Remove##rotspeed"))
            entity.removeComponent<Components::RotationSpeed>();
          
          ImGui::TreePop();
        }
      }

      // StaticSprite Component (Read-only for now)
      if (entity.hasComponent<Components::StaticSprite>())
      {
        if (ImGui::TreeNode("StaticSprite"))
        {
          auto* sprite = entity.get_component<Components::StaticSprite>();
          ImGui::Text("Texture ID: %d", sprite->texture_id);
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
            "Note: StaticSprite editing not yet supported");
          
          if (ImGui::Button("Remove##sprite"))
            entity.removeComponent<Components::StaticSprite>();
          
          ImGui::TreePop();
        }
      }

      // Background Component (marker only)
      if (entity.hasComponent<Components::Background>())
      {
        if (ImGui::TreeNode("Background"))
        {
          ImGui::Text("Background marker component");
          
          if (ImGui::Button("Remove##background"))
            entity.removeComponent<Components::Background>();
          
          ImGui::TreePop();
        }
      }
    }

    ImGui::Separator();

    // Clone Entity
    if (ImGui::Button("Clone This Entity"))
    {
      auto clonedEntity = Engine::WORLD.clone_entity(entity.getId());
      selectedEntityId = static_cast<int>(clonedEntity.getId());
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
  }

  ImGui::End();
}