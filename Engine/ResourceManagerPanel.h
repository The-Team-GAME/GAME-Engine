//-----------------------------------------------------------------------------------
// file:    ResourceManagerPanel.h
// project: Monkey Stole My GPA
//
// author:  Ryan Ang
//          Primary author
//
// email:   a.ryanankiat@digipen.edu
//
// brief:   Defines the ResourceManagerPanel inline function, which provides a
//          comprehensive ImGui interface for engine resource management. This panel
//          displays loaded resources (Textures, Sounds, Fonts, etc.), allows manual
//          unloading, and implements a drag-and-drop zone for easy file loading.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "imgui.h"
#include "Core.h"
#include "BasicEvents.h"
#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>





/*=================================================================================
    Function Brief:
    Renders the "Resource Manager" ImGui window. It provides an overview of all
    loaded engine resources, allows viewing and unloading of individual resources
    (with texture previews and sound controls), and registers an event listener
    to process dropped files for dynamic resource loading.

    Where to call it:
    Must be called once per frame within the main editor/game loop's ImGui rendering
    phase.
    ==================================================================================*/
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

    // Textures section with previews
    if (ImGui::CollapsingHeader("Textures"))
    {
      if (textureCount > 0)
      {
        auto& textureMap = Engine::RESOURCES.get_all_textures();

        for (const auto& [name, texture_id] : textureMap)
        {
          ImGui::PushID(name.c_str());

          // Get texture dimensions
          s32 width = 0, height = 0;
          Engine::GRAPHICS.get_texture_dimensions(texture_id, width, height);

          // Display texture name
          ImGui::Text("%s", name.c_str());
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(ID: %d, %dx%d)", texture_id, width, height);

          // Display texture preview
          f32 previewSize = 64.0f;
          f32 aspectRatio = width > 0 ? (f32)height / (f32)width : 1.0f;
          ImVec2 imageSize(previewSize, previewSize * aspectRatio);

          // Get the OpenGL texture handle for ImGui
          u32 textureHandle = Engine::GRAPHICS.get_texture_handle(texture_id);
          ImGui::Image((ImTextureID)(intptr_t)textureHandle, imageSize);

          // Unload button
          ImGui::SameLine();
          if (ImGui::Button("Unload"))
          {
            Engine::RESOURCES.unload_texture(name);
          }

          ImGui::Separator();
          ImGui::PopID();
        }
      }
      else
      {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No textures loaded");
      }
    }

    // Sounds section with playback controls
    if (ImGui::CollapsingHeader("Sounds"))
    {
      if (soundCount > 0)
      {
        auto& soundMap = Engine::RESOURCES.get_all_sounds();

        for (const auto& [name, soundId] : soundMap)
        {
          ImGui::PushID(name.c_str());

          // Display sound name
          ImGui::Text("%s", name.c_str());
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(ID: %d)", soundId);

          // Play button
          ImGui::SameLine();
          if (ImGui::Button("Play"))
          {
            Engine::AUDIO.playSound(soundId);
          }

          // Stop button
          ImGui::SameLine();
          if (ImGui::Button("Stop"))
          {
            Engine::AUDIO.stopSound(soundId);
          }

          // Unload button
          ImGui::SameLine();
          if (ImGui::Button("Unload"))
          {
            Engine::RESOURCES.unload_sound(name);
          }

          ImGui::Separator();
          ImGui::PopID();
        }
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
        auto& fontMap = Engine::RESOURCES.get_all_fonts();

        for (const auto& [name, fontId] : fontMap)
        {
          ImGui::PushID(name.c_str());

          ImGui::Text("%s", name.c_str());
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(ID: %d)", fontId);

          ImGui::SameLine();
          if (ImGui::Button("Unload"))
          {
            Engine::RESOURCES.unload_font(name);
          }

          ImGui::Separator();
          ImGui::PopID();
        }
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
        auto& spriteSheetMap = Engine::RESOURCES.get_all_sprite_sheets();

        for (const auto& [name, spriteSheetId] : spriteSheetMap)
        {
          ImGui::PushID(name.c_str());

          ImGui::Text("%s", name.c_str());
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(ID: %d)", spriteSheetId);

          ImGui::SameLine();
          if (ImGui::Button("Unload"))
          {
            Engine::RESOURCES.unload_sprite_sheet(name);
          }

          ImGui::Separator();
          ImGui::PopID();
        }
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
        auto& objectPrefabMap = Engine::RESOURCES.get_all_object_prefabs();

        for (const auto& [name, prefabId] : objectPrefabMap)
        {
          ImGui::PushID(name.c_str());

          ImGui::Text("%s", name.c_str());
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(ID: %d)", prefabId);

          ImGui::SameLine();
          if (ImGui::Button("Unload"))
          {
            Engine::RESOURCES.unload_object_prefab(name);
          }

          ImGui::Separator();
          ImGui::PopID();
        }
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
    Engine::WORLD.add_event_listener<FileDropEvent>([&](const FileDropEvent& event) {
      for (const auto& filePath : event.filePaths)
      {
        std::filesystem::path path(filePath);
        std::string filename = path.stem().string();

        // Use the generic load function
        s32 result = Engine::RESOURCES.load(filename, filePath);

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