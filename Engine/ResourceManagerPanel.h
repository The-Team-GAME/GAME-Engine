#pragma once
#include "imgui.h"
#include "Core.h"

inline void ResourceManagerPanel()
{
  if (ImGui::Begin("Resource Manager"))
  {
    // Get counts from the resource manager
    size_t textureCount = Engine::RESOURCES.get_texture_count();
    size_t soundCount = Engine::RESOURCES.get_sound_count();
    size_t fontCount = Engine::RESOURCES.get_font_count();
    size_t spriteSheetCount = Engine::RESOURCES.get_sprite_sheet_count();
    size_t objectPrefabCount = Engine::RESOURCES.get_object_prefab_count();

    // Display overview
    if (ImGui::CollapsingHeader("Overview", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::Text("Textures: %zu", textureCount);
      ImGui::Text("Sounds: %zu", soundCount);
      ImGui::Text("Fonts: %zu", fontCount);
      ImGui::Text("Sprite Sheets: %zu", spriteSheetCount);
      ImGui::Text("Object Prefabs: %zu", objectPrefabCount);
      ImGui::Separator();
    }

    // Textures section
    if (ImGui::CollapsingHeader("Textures"))
    {
      if (textureCount > 0)
      {
        ImGui::Text("Total Textures: %zu", textureCount);
      }
      else
      {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No textures loaded");
      }
    }

    // Sounds section
    if (ImGui::CollapsingHeader("Sounds"))
    {
      if (soundCount > 0)
      {
        ImGui::Text("Total Sounds: %zu", soundCount);
      }
      else
      {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No sounds loaded");
      }
    }

    // Fonts section
    if (ImGui::CollapsingHeader("Fonts"))
    {
      if (fontCount > 0)
      {
        ImGui::Text("Total Fonts: %zu", fontCount);
      }
      else
      {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No fonts loaded");
      }
    }

    // Sprite Sheets section
    if (ImGui::CollapsingHeader("Sprite Sheets"))
    {
      if (spriteSheetCount > 0)
      {
        ImGui::Text("Total Sprite Sheets: %zu", spriteSheetCount);
      }
      else
      {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No sprite sheets loaded");
      }
    }

    // Object Prefabs section
    if (ImGui::CollapsingHeader("Object Prefabs"))
    {
      if (objectPrefabCount > 0)
      {
        ImGui::Text("Total Object Prefabs: %zu", objectPrefabCount);
      }
      else
      {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No object prefabs loaded");
      }
    }
  }
  ImGui::End();
}