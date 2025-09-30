#pragma once
#include "imgui.h"
#include "Core.h"
#include "BasicEvents.h"
#include <filesystem>
#include <algorithm>

inline void ResourceManagerPanel()
{
  static bool showLoadConfirmation = false;
  static std::string lastLoadedFile = "";
  static std::string lastLoadError = "";

  if (ImGui::Begin("Resource Manager"))
  {
    // Get counts from the resource manager
    size_t textureCount = Engine::RESOURCES.get_texture_count();
    size_t soundCount = Engine::RESOURCES.get_sound_count();
    size_t fontCount = Engine::RESOURCES.get_font_count();
    size_t spriteSheetCount = Engine::RESOURCES.get_sprite_sheet_count();
    size_t objectPrefabCount = Engine::RESOURCES.get_object_prefab_count();

    // Drag and Drop Zone
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.3f, 0.5f));
    ImGui::BeginChild("DropZone", ImVec2(0, 80), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
    ImGui::TextWrapped("Drag and drop files here to load them");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1.0f));
    ImGui::TextWrapped("Supported: .png, .jpg, .wav, .ttf, .json");
    ImGui::PopStyleColor(2);

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Separator();

    // Display confirmation/error messages
    if (showLoadConfirmation)
    {
      if (!lastLoadError.empty())
      {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("Error: %s", lastLoadError.c_str());
        ImGui::PopStyleColor();
      }
      else if (!lastLoadedFile.empty())
      {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
        ImGui::TextWrapped("Loaded: %s", lastLoadedFile.c_str());
        ImGui::PopStyleColor();
      }

      if (ImGui::Button("Clear Message"))
      {
        showLoadConfirmation = false;
        lastLoadedFile = "";
        lastLoadError = "";
      }
      ImGui::Separator();
    }

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

  // Listen for file drop events
  static bool eventListenerRegistered = false;
  if (!eventListenerRegistered)
  {
    Engine::WORLD.addEventListener<FileDropEvent>([&](const FileDropEvent& event) {
      for (const auto& filePath : event.filePaths)
      {
        std::filesystem::path path(filePath);
        std::string filename = path.stem().string();

        // Use the generic load function
        int result = Engine::RESOURCES.load(filename, filePath);

        if (result != -1)
        {
          lastLoadedFile = filename + " (" + path.extension().string() + ")";
          lastLoadError = "";
        }
        else
        {
          lastLoadError = "Failed to load: " + filename;
        }

        showLoadConfirmation = true;
      }
      });

    eventListenerRegistered = true;
  }
}