//-----------------------------------------------------------------------------------
// file:    ResourceManagerPanel.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ImGui panel implementation for the resource manager interface.
//          Displays loaded resources (textures, fonts, sounds, prefabs) with
//          statistics and management controls. Provides resource cleanup
//          functionality and integrates with the audio system for sound preview.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "../Resources.h"
#include "../Logger.h"
#include <imgui.h>
#include "../Audio/AudioPlayerInternal.h"
#include "../Audio/AudioPlayer.h"
#include <unordered_map>

// Static storage for playing channel handles
static std::unordered_map<std::string, Engine::Audio::ChannelHandle> playing_sounds;

void ResourceManagerPanel() {
  if (!ImGui::Begin("Resource Manager")) {
    ImGui::End();
    return;
  }

  // Get resource stats
  auto stats = Engine::Resources::get_resource_stats();

  // Display statistics
  ImGui::Text("Resource Statistics:");
  ImGui::Separator();
  ImGui::Text("Textures: %zu total, %zu alive", stats.texture_count, stats.texture_alive);
  ImGui::Text("Fonts: %zu total, %zu alive", stats.font_count, stats.font_alive);
  ImGui::Text("Sounds: %zu total, %zu alive", stats.sound_count, stats.sound_alive);
  ImGui::Separator();

  // Button to clear unused resources
  if (ImGui::Button("Clear Unused Resources")) {
    Engine::Resources::clear_unused_resources();
    Log::info("Cleared unused resources from cache");
  }

  ImGui::Spacing();
  ImGui::Separator();

  // Textures section
  if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent();

    // Iterate through texture cache
    for (const auto& pair : Engine::Resources::texture_cache) {
      const std::string& path = pair.first;
      auto texture_ptr = pair.second.lock();

      // Display the path
      ImGui::PushID(path.c_str());

      if (texture_ptr && texture_ptr->id != 0) {
        // Texture is alive
        ImGui::Text("%s", path.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ALIVE]");

        // Display texture info
        ImGui::Text("  Size: %dx%d", texture_ptr->width, texture_ptr->height);
        ImGui::Text("  OpenGL ID: %u", texture_ptr->id);
        ImGui::Text("  Filter: %s",
          texture_ptr->filter == Engine::Graphics::TextureFilter::Nearest ? "Nearest" : "Linear");

        // Display the texture itself
        // Calculate display size (max 256x256, maintain aspect ratio)
        float aspect = (float)texture_ptr->width / (float)texture_ptr->height;
        float display_width = 256.0f;
        float display_height = 256.0f;

        if (aspect > 1.0f) {
          display_height = display_width / aspect;
        }
        else {
          display_width = display_height * aspect;
        }

        ImGui::Image(
          (void*)(intptr_t)texture_ptr->id,
          ImVec2(display_width, display_height),
          ImVec2(0, 0), // UV0
          ImVec2(1, 1), // UV1
          ImVec4(1, 1, 1, 1), // Tint color
          ImVec4(1, 1, 1, 0.5f) // Border color
        );

        ImGui::Spacing();
      }
      else {
        // Texture is expired
        ImGui::Text("%s", path.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[EXPIRED]");
        ImGui::Spacing();
      }

      ImGui::PopID();
      ImGui::Separator();
    }

    ImGui::Unindent();
  }

  // Fonts section
  if (ImGui::CollapsingHeader("Fonts")) {
    ImGui::Indent();

    for (const auto& pair : Engine::Resources::font_cache) {
      const std::string& path = pair.first;
      auto font_ptr = pair.second.lock();

      ImGui::PushID(path.c_str());

      if (font_ptr && font_ptr->texture_atlas.id != 0) {
        ImGui::Text("%s", path.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ALIVE]");

        ImGui::Text("  Font size: %u", font_ptr->font_size);
        ImGui::Text("  Glyphs: %zu", font_ptr->glyphs.size());
        ImGui::Text("  Atlas size: %dx%d",
          font_ptr->texture_atlas.width, font_ptr->texture_atlas.height);

        // Display the font atlas
        float display_width = 256.0f;
        float aspect = (float)font_ptr->texture_atlas.width /
          (float)font_ptr->texture_atlas.height;
        float display_height = display_width / aspect;

        ImGui::Image(
          (void*)(intptr_t)font_ptr->texture_atlas.id,
          ImVec2(display_width, display_height),
          ImVec2(0, 0),
          ImVec2(1, 1),
          ImVec4(1, 1, 1, 1),
          ImVec4(1, 1, 1, 0.5f)
        );

        ImGui::Spacing();
      }
      else {
        ImGui::Text("%s", path.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[EXPIRED]");
        ImGui::Spacing();
      }

      ImGui::PopID();
      ImGui::Separator();
    }

    ImGui::Unindent();
  }

  // Sounds section
  if (ImGui::CollapsingHeader("Sounds")) {
    ImGui::Indent();

    for (const auto& pair : Engine::Resources::sound_cache) {
      const std::string& path = pair.first;
      auto sound_ptr = pair.second.lock();

      ImGui::PushID(path.c_str());

      if (sound_ptr && sound_ptr->sound != nullptr) {
        ImGui::Text("%s", path.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ALIVE]");

        ImGui::Text("  Type: %s", sound_ptr->is_stream ? "Music (Streamed)" : "Sound Effect");

        // Check if this sound is currently playing
        auto channel_it = playing_sounds.find(path);
        bool is_playing = false;
        if (channel_it != playing_sounds.end()) {
          is_playing = Engine::Audio::is_channel_playing(channel_it->second);
          // Clean up finished sounds
          if (!is_playing) {
            playing_sounds.erase(channel_it);
          }
        }

        // Playback controls
        ImGui::Text("  Controls:");
        ImGui::SameLine();

        if (!is_playing) {
          // Play button
          if (ImGui::SmallButton("Play")) {
            Engine::Audio::ChannelHandle handle = Engine::Audio::play_sound(sound_ptr, 1.0f, false);
            if (handle != Engine::Audio::INVALID_CHANNEL) {
              playing_sounds[path] = handle;
              Log::info("Playing sound: " + path);
            }
          }
          ImGui::SameLine();

          // Play looped button
          if (ImGui::SmallButton("Play Loop")) {
            Engine::Audio::ChannelHandle handle = Engine::Audio::play_sound(sound_ptr, 1.0f, true);
            if (handle != Engine::Audio::INVALID_CHANNEL) {
              playing_sounds[path] = handle;
              Log::info("Playing sound (looped): " + path);
            }
          }
        }
        else {
          // Stop button
          if (ImGui::SmallButton("Stop")) {
            Engine::Audio::stop_channel(channel_it->second);
            playing_sounds.erase(channel_it);
            Log::info("Stopped sound: " + path);
          }
          ImGui::SameLine();

          // Pause/Resume button
          ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[PLAYING]");
          ImGui::SameLine();

          if (ImGui::SmallButton("Pause")) {
            Engine::Audio::pause_channel(channel_it->second);
            Log::info("Paused sound: " + path);
          }
          ImGui::SameLine();

          if (ImGui::SmallButton("Resume")) {
            Engine::Audio::resume_channel(channel_it->second);
            Log::info("Resumed sound: " + path);
          }

          // Volume control
          static float volume = 1.0f;
          ImGui::SetNextItemWidth(150.0f);
          if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f, "%.2f")) {
            Engine::Audio::set_channel_volume(channel_it->second, volume);
          }
        }

        ImGui::Spacing();
      }
      else {
        ImGui::Text("%s", path.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[EXPIRED]");
        ImGui::Spacing();
      }

      ImGui::PopID();
      ImGui::Separator();
    }

    ImGui::Unindent();
  }

  ImGui::End();
}
