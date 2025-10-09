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
#include <string>
#include <vector>
#include "../ECS/ECS.h"

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

      // Show ObjectId if available
      if (entity.has_component<Components::ObjectId>())
      {
        auto* objId = entity.get_component<Components::ObjectId>();
        label += " (" + std::string(*objId) + ")";
      }

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

      ImGui::SameLine();

      if (ImGui::Button("Add ObjectId"))
      {
        if (!entity.has_component<Components::ObjectId>())
          entity.add_component<Components::ObjectId>("unnamed");
      }
    }

    ImGui::Separator();

    // Edit Components Section
    if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
    {
      // ObjectId Component
      if (entity.has_component<Components::ObjectId>())
      {
        if (ImGui::TreeNode("ObjectId"))
        {
          auto* objId = entity.get_component<Components::ObjectId>();
          char buffer[256];
          strncpy_s(buffer, objId->c_str(), sizeof(buffer) - 1);
          buffer[sizeof(buffer) - 1] = '\0';

          if (ImGui::InputText("ID", buffer, sizeof(buffer)))
          {
            *objId = Components::ObjectId(buffer);
          }

          if (ImGui::Button("Remove##objectid"))
            entity.remove_component<Components::ObjectId>();

          ImGui::TreePop();
        }
      }

      // Transform2D Component
      if (entity.has_component<Components::Transform2D>())
      {
        if (ImGui::TreeNode("Transform2D"))
        {
          auto* transform = entity.get_component<Components::Transform2D>();

          ImGui::Text("Position:");
          ImGui::DragFloat("X##pos", &transform->position.x, 1.0f);
          ImGui::DragFloat("Y##pos", &transform->position.y, 1.0f);

          ImGui::Separator();
          ImGui::Text("Rotation:");
          ImGui::SliderFloat("Angle (degrees)", &transform->rotation, 0.0f, 360.0f);

          ImGui::Separator();
          ImGui::Text("Scale:");
          ImGui::DragFloat("X##scale", &transform->scale.x, 0.01f, 0.01f, 10.0f);
          ImGui::DragFloat("Y##scale", &transform->scale.y, 0.01f, 0.01f, 10.0f);

          if (ImGui::Button("Reset Transform"))
          {
            transform->position = Math::Vec2D(0.0f, 0.0f);
            transform->rotation = 0.0f;
            transform->scale = Math::Vec2D(1.0f, 1.0f);
          }

          if (ImGui::Button("Remove##transform"))
            entity.remove_component<Components::Transform2D>();

          ImGui::TreePop();
        }
      }

      // Color Component
      if (entity.has_component<Components::Color>())
      {
        if (ImGui::TreeNode("Color"))
        {
          auto* color = entity.get_component<Components::Color>();
          f32 colorArray[4] = {
            color->x,
            color->y,
            color->z,
            color->w
          };

          if (ImGui::ColorEdit4("RGBA", colorArray))
          {
            color->x = colorArray[0];
            color->y = colorArray[1];
            color->z = colorArray[2];
            color->w = colorArray[3];
          }

          if (ImGui::Button("Remove##color"))
            entity.remove_component<Components::Color>();

          ImGui::TreePop();
        }
      }

      // Text Component
      if (entity.has_component<Components::Text>())
      {
        if (ImGui::TreeNode("Text"))
        {
          auto* text = entity.get_component<Components::Text>();
          char buffer[1024];
          strncpy_s(buffer, text->content.c_str(), sizeof(buffer) - 1);
          buffer[sizeof(buffer) - 1] = '\0';

          if (ImGui::InputTextMultiline("Content", buffer, sizeof(buffer)))
          {
            text->content = buffer;
          }

          if (ImGui::Button("Remove##text"))
            entity.remove_component<Components::Text>();

          ImGui::TreePop();
        }
      }

      // Texture Component (Read-only)
      if (entity.has_component<Components::Texture>())
      {
        if (ImGui::TreeNode("Texture"))
        {
          auto* texture = entity.get_component<Components::Texture>();

          if (texture->texture_handle)
          {
            ImGui::Text("Texture ID: %u", texture->texture_handle->id);
            ImGui::Text("Size: %dx%d", texture->texture_handle->width, texture->texture_handle->height);
            ImGui::Text("Filter: %s",
              texture->texture_handle->filter == Engine::Graphics::TextureFilter::Nearest ? "Nearest" : "Linear");
          }
          else
          {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid texture");
          }

          ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
            "Note: Texture editing not yet supported");

          if (ImGui::Button("Remove##texture"))
            entity.remove_component<Components::Texture>();

          ImGui::TreePop();
        }
      }

      // TextureAtlas Component
      if (entity.has_component<Components::TextureAtlas>())
      {
        if (ImGui::TreeNode("TextureAtlas"))
        {
          auto* atlas = entity.get_component<Components::TextureAtlas>();

          ImGui::Text("Total SubTextures: %zu", atlas->subTextures.size());
          ImGui::Text("Current Draw Index: %zu", atlas->drawIndex);

          if (atlas->subTextures.size() > 0)
          {
            int currentIndex = static_cast<int>(atlas->drawIndex);
            if (ImGui::SliderInt("Frame", &currentIndex, 0, static_cast<int>(atlas->subTextures.size() - 1)))
            {
              atlas->drawIndex = static_cast<size_t>(currentIndex);
            }
          }

          ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
            "Note: Atlas editing not yet supported");

          if (ImGui::Button("Remove##atlas"))
            entity.remove_component<Components::TextureAtlas>();

          ImGui::TreePop();
        }
      }

      // Animation Component
      if (entity.has_component<Components::Animation>())
      {
        if (ImGui::TreeNode("Animation"))
        {
          auto* anim = entity.get_component<Components::Animation>();

          ImGui::Text("Total Frames: %zu", anim->frames.size());
          ImGui::Text("Total Duration: %.2f s", anim->totalDuration);
          ImGui::DragFloat("Start Time", &anim->startTime, 0.01f);

          // Calculate current frame
          if (anim->frames.size() > 0 && anim->totalDuration > 0.0f)
          {
            float elapsed = anim->startTime;
            size_t currentFrame = 0;
            for (size_t i = 0; i < anim->frames.size(); ++i)
            {
              if (elapsed >= anim->frames[i].before_time)
                currentFrame = i;
              else
                break;
            }
            ImGui::Text("Current Frame: %zu", currentFrame);
          }

          ImGui::Separator();

          if (ImGui::Button("Reset Animation"))
          {
            anim->startTime = 0.0f;
          }

          ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
            "Note: Frame editing not yet supported");

          if (ImGui::Button("Remove##animation"))
            entity.remove_component<Components::Animation>();

          ImGui::TreePop();
        }
      }

      // Font Component (Read-only)
      if (entity.has_component<Components::Font>())
      {
        if (ImGui::TreeNode("Font"))
        {
          auto* font = entity.get_component<Components::Font>();

          if (font->font_handle)
          {
            ImGui::Text("Font Size: %u", font->font_handle->font_size);
            ImGui::Text("Baseline: %.2f", font->font_handle->baseline);
            ImGui::Text("Glyphs Loaded: %zu", font->font_handle->glyphs.size());
          }
          else
          {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid font");
          }

          ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
            "Note: Font editing not yet supported");

          if (ImGui::Button("Remove##font"))
            entity.remove_component<Components::Font>();

          ImGui::TreePop();
        }
      }
    }

    ImGui::Separator();

    // Clone Entity
    if (ImGui::Button("Clone This Entity"))
    {
      auto clonedEntity = Engine::ECS::clone_entity(entity.get_id());
      selected_entity_id = static_cast<int>(clonedEntity.get_id());
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
  }

  ImGui::End();
}