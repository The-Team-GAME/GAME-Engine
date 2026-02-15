//-----------------------------------------------------------------------------------
// file:    ConfigurationPanel.h
// project: Monkey Stole My GPA
//
// brief:   This file defines an ImGui panel for viewing and modifying engine and
//          game configuration settings. Changes to game settings (audio volumes,
//          fullscreen) are applied in real-time and persisted to JSON files.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "Precompiled.h"
#include "FileIO/Settings.h"
#include "WindowManager.h"
#include "Audio/AudioPlayer.h"

/*=================================================================================
Function Brief:
Creates and renders an ImGui panel that displays and allows modification of
engine and game configuration settings. Game settings (fullscreen, audio volumes)
are applied immediately and saved to the settings JSON file.

Where to call it:
Call this function once per frame during the ImGui rendering phase. Register it
with ImGuiManager::register_panel() in RegisterAllPanels.h.
==================================================================================*/
inline void ConfigurationPanel()
{
  ImGui::Begin("Configuration");

  // ==================================================================================
  // Game Settings Section (Real-time editable)
  // ==================================================================================
  if (ImGui::CollapsingHeader("Game Settings", ImGuiTreeNodeFlags_DefaultOpen))
  {
    auto& game_settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get_mutable();
    bool settings_changed = false;

    // Fullscreen Toggle
    ImGui::Text("Display");
    ImGui::Separator();
    
    if (ImGui::Checkbox("Fullscreen", &game_settings.fullscreen))
    {
      Engine::WindowManager::set_fullscreen(game_settings.fullscreen);
      settings_changed = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Toggle between fullscreen and windowed mode");
      ImGui::EndTooltip();
    }

    if (ImGui::Checkbox("VSync", &game_settings.vsync))
    {
      Engine::WindowManager::set_vsync(game_settings.vsync);
      settings_changed = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Cap frame rate to monitor refresh rate");
      ImGui::EndTooltip();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // Audio Settings
    ImGui::Text("Audio");
    ImGui::Separator();

    // BGM Volume Slider
    if (ImGui::SliderFloat("BGM Volume", &game_settings.bgm_volume, 0.0f, 1.0f, "%.2f"))
    {
      Engine::Audio::set_bgm_volume(game_settings.bgm_volume);
      settings_changed = true;
    }

    // SFX Volume Slider
    if (ImGui::SliderFloat("SFX Volume", &game_settings.sfx_volume, 0.0f, 1.0f, "%.2f"))
    {
      Engine::Audio::set_sfx_volume(game_settings.sfx_volume);
      settings_changed = true;
    }

    // Master Volume (read from audio system)
    f32 master_volume = Engine::Audio::get_master_volume();
    if (ImGui::SliderFloat("Master Volume", &master_volume, 0.0f, 1.0f, "%.2f"))
    {
      Engine::Audio::set_master_volume(master_volume);
    }

    // Save settings if changed
    if (settings_changed)
    {
      Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
    }
  }

  ImGui::Spacing();

  // ==================================================================================
  // Engine Settings Section (Some require restart)
  // ==================================================================================
  if (ImGui::CollapsingHeader("Engine Settings", ImGuiTreeNodeFlags_DefaultOpen))
  {
    auto& engine_settings = Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::get_mutable();
    bool engine_settings_changed = false;

    // Window Settings (Requires Restart)
    ImGui::Text("Window (Requires Restart)");
    ImGui::Separator();
    
    // Window Title - use buffer for InputText
    static char title_buffer[256] = "";
    static bool title_initialized = false;
    if (!title_initialized)
    {
      strncpy_s(title_buffer, engine_settings.window_title.c_str(), sizeof(title_buffer) - 1);
      title_initialized = true;
    }
    if (ImGui::InputText("Window Title", title_buffer, sizeof(title_buffer)))
    {
      engine_settings.window_title = title_buffer;
      engine_settings_changed = true;
    }
    
    // Window Dimensions
    if (ImGui::InputInt("Window Width", &engine_settings.window_width))
    {
      engine_settings_changed = true;
    }
    if (ImGui::InputInt("Window Height", &engine_settings.window_height))
    {
      engine_settings_changed = true;
    }
    
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "* Changes require application restart to take effect");

    ImGui::Spacing();
    ImGui::Spacing();

    // Runtime Settings
    ImGui::Text("Runtime Settings");
    ImGui::Separator();

    // Panel Viewport Mode
    if (ImGui::Checkbox("Panel Viewport Mode", &engine_settings.panel_viewport_mode))
    {
      engine_settings_changed = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("When enabled, renders to ImGui panel viewport.");
      ImGui::Text("When disabled, renders to the full window.");
      ImGui::EndTooltip();
    }

    // Load All Assets on Startup
    if (ImGui::Checkbox("Load All Assets on Startup", &engine_settings.load_all_assets_on_startup))
    {
      engine_settings_changed = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Pre-loads all assets during engine initialization.");
      ImGui::Text("Increases startup time but prevents loading hitches.");
      ImGui::EndTooltip();
    }

    // Save engine settings if changed
    if (engine_settings_changed)
    {
      Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::save();
    }
  }

  ImGui::Spacing();

  // ==================================================================================
  // Settings File Actions
  // ==================================================================================
  if (ImGui::CollapsingHeader("Settings Actions"))
  {
    if (ImGui::Button("Save All Settings"))
    {
      Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::save();
      Engine::FileIO::SettingsManager<Engine::FileIO::EngineSettings>::save();
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Settings saved!");
    }

    ImGui::SameLine();

    if (ImGui::Button("Reload Game Settings"))
    {
      Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::load("Assets/settings/game_settings.json");
      const auto& game_settings = Engine::FileIO::SettingsManager<Engine::FileIO::GameSettings>::get();
      Engine::WindowManager::set_fullscreen(game_settings.fullscreen);
      Engine::Audio::set_bgm_volume(game_settings.bgm_volume);
      Engine::Audio::set_sfx_volume(game_settings.sfx_volume);
    }

    ImGui::Spacing();

    // Display current settings file paths
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Settings Files:");
    ImGui::BulletText("Game: Assets/settings/game_settings.json");
    ImGui::BulletText("Engine: Assets/settings/engine_settings.json");
  }

  ImGui::End();
}
