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
// Copyright � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once
#include "imgui.h"
#include "../Resources.h"
#include <string>
#include <vector>
#include "../Audio/AudioPlayerInternal.h"

inline void ResourceManagerPanel(bool* p_open = nullptr)
{
  if (!ImGui::Begin("Resource Manager", p_open))
  {
    ImGui::End();
    return;
  }

  auto stats = Engine::Resources::get_resource_stats();

  // Summary section
  ImGui::Text("Resource Path: %s", Engine::Resources::resource_path.c_str());
  ImGui::Separator();

  ImGui::Text("Summary:");
  ImGui::Text("  Textures: %zu cached, %zu alive", stats.texture_count, stats.texture_alive);
  ImGui::Text("  Fonts: %zu cached, %zu alive", stats.font_count, stats.font_alive);
  ImGui::Text("  Sounds: %zu cached, %zu alive", stats.sound_count, stats.sound_alive);

  ImGui::Separator();

  if (ImGui::Button("Clear Unused Resources"))
  {
    Engine::Resources::clear_unused_resources();
  }

  ImGui::Separator();

  // Detailed view in collapsing headers
  if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::BeginChild("TextureList", ImVec2(0, 150), true);

    for (const auto& pair : Engine::Resources::texture_cache)
    {
      bool is_alive = !pair.second.expired();
      ImGui::PushStyleColor(ImGuiCol_Text, is_alive ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

      if (is_alive)
      {
        auto locked = pair.second.lock();
        ImGui::Text("[ALIVE] %s (%dx%d, ID: %u)",
          pair.first.c_str(),
          locked->width,
          locked->height,
          locked->id);
      }
      else
      {
        ImGui::Text("[EXPIRED] %s", pair.first.c_str());
      }

      ImGui::PopStyleColor();
    }

    if (Engine::Resources::texture_cache.empty())
    {
      ImGui::TextDisabled("No textures loaded");
    }

    ImGui::EndChild();
  }

  if (ImGui::CollapsingHeader("Fonts"))
  {
    ImGui::BeginChild("FontList", ImVec2(0, 150), true);

    for (const auto& pair : Engine::Resources::font_cache)
    {
      bool is_alive = !pair.second.expired();
      ImGui::PushStyleColor(ImGuiCol_Text, is_alive ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

      if (is_alive)
      {
        auto locked = pair.second.lock();
        ImGui::Text("[ALIVE] %s", pair.first.c_str());
      }
      else
      {
        ImGui::Text("[EXPIRED] %s", pair.first.c_str());
      }

      ImGui::PopStyleColor();
    }

    if (Engine::Resources::font_cache.empty())
    {
      ImGui::TextDisabled("No fonts loaded");
    }

    ImGui::EndChild();
  }

  if (ImGui::CollapsingHeader("Sounds/Music"))
  {
    ImGui::BeginChild("SoundList", ImVec2(0, 150), true);

    for (const auto& pair : Engine::Resources::sound_cache)
    {
      bool is_alive = !pair.second.expired();
      ImGui::PushStyleColor(ImGuiCol_Text, is_alive ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

      if (is_alive)
      {
        auto locked = pair.second.lock();
        const char* type = locked->is_stream ? "MUSIC" : "SOUND";
        ImGui::Text("[ALIVE] [%s] %s", type, pair.first.c_str());
      }
      else
      {
        ImGui::Text("[EXPIRED] %s", pair.first.c_str());
      }

      ImGui::PopStyleColor();
    }

    if (Engine::Resources::sound_cache.empty())
    {
      ImGui::TextDisabled("No sounds loaded");
    }

    ImGui::EndChild();
  }

  ImGui::End();
}