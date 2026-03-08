//-----------------------------------------------------------------------------------
// file:    Settings.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   This file defines the settings structures used by the engine for
//          configuration management. Settings are automatically serialized to/from
//          JSON files using reflection. Default values are used only when creating
//          new JSON files, not as hard-coded runtime values.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <array>
#include <bitset>
#include "../Components/Components.h"
#include "../Reflection/ReflectionLibrary.h"
#include "SettingsManager.h"
#include "../Graphics/RendererInternal.h"
#include "../ECS/Types.h"

/*
Settings Documentation:

This file defines the settings structures used by the engine. Settings are automatically
serialized to and from JSON files using the reflection system and SettingsManager.

DEFAULT VALUES:
- The default values specified in these structs (e.g., window_width = 1280) are NOT hard-coded
  into the application at runtime.
- These defaults are ONLY used when the JSON settings file is created for the first time.
- Once the JSON file exists, the engine loads values from the file, not from these defaults.
- The values from the JSON files take precedence over these defaults.
- If a field is missing from an existing JSON file, the default value will be added automatically.

ADDING NEW SETTINGS:
1. Add the member variable to the appropriate settings struct with its default value.
2. Include it in the REFLECT_FIELDS macro at the bottom of this file.
3. The new field will automatically appear in the JSON file on the next engine run.

SETTINGS TYPES:
- EngineSettings: Values needed before engine subsystems initialize (e.g., window configuration).
- ProjectSettings: Values needed after engine initialization (e.g., assets requiring Graphics system).
- LocalSettings: User/machine-specific values (e.g., UI theme preferences).

See SettingsManager.h for more details on loading and saving settings.
*/

namespace Engine
{
  namespace FileIO
  {
    // Values that need to be loaded before initializing engine subsystems (e.g. window size, title)
    struct EngineSettings
    {
      int window_width = 1280;
      int window_height = 720;
      std::string window_title = "Monkey Stole My GPA";
      bool panel_viewport_mode = true; // True - Render to ImGui panel viewport, False - render to full window
      bool load_all_assets_on_startup = true;
    };

    // Values that need to be loaded after intializing engine subsystems, (e.g. Textures need to wait for Graphics init)
    struct ProjectSettings
    {
      std::vector<Engine::Graphics::AtlasEntry> loaded_tilemaps{};

      // Layer collision pairs: stores pairs of layer names that should collide
      // This is name-based instead of ID-based, so it survives layer additions/removals
      // Each entry is a pair of layer names: {"layerA", "layerB"}
      // The matrix is symmetric, so only one direction needs to be stored
      std::vector<std::pair<std::string, std::string>> layer_collision_pairs{};
    };

    // Values that are specific to the local user/machine (e.g. ImGui theme)
    struct LocalSettings
    {
      std::string imgui_theme = "ayu-dark";
      std::string imgui_layout = "default"; // Last active ImGui layout (saved in Assets/layouts/)
      bool auto_reload = true; // Automatically rebuild & reload when source files change
    };

    // Game settings (Window mode, Audio volumes)
    struct GameSettings
    {
      bool fullscreen = false;
      bool vsync = true;  // Enable VSync to cap FPS to monitor refresh rate
      float bgm_volume = 1.0f;
      float sfx_volume = 1.0f;
    };

    // ==================================================================================
    // Roguelike Upgrade System Settings
    // ==================================================================================

    // Upgrade type: Flat adds a fixed amount, Multiplier multiplies current stat
    enum class UpgradeType
    {
      Flat,        // Adds value to the stat each time picked
      Multiplier   // Multiplies current stat by value each time picked
    };

    // A rarity tier with its associated drop rate (weight for weighted random)
    struct RarityTier
    {
      std::string name = "Common";  // Rarity display name
      float drop_rate = 1.0f;       // Weight for weighted random selection (not required to sum to 1)
    };

    // Definition of a single upgrade card
    struct UpgradeDefinition
    {
      std::string id = "";            // Unique identifier (e.g. "flat_damage")
      std::string display_name = "";  // Display label (e.g. "Damage Boost")
      std::string image_path = "";    // Optional popup card art path under Assets/
      std::string effect_text = "";   // Optional popup buff line override (e.g. "+1 Base Attack")
      std::string description = "";   // Optional popup description/flavor text
      std::string rarity = "Common";  // Must match a RarityTier::name
      std::string stat_name = "";     // Which stat this targets (e.g. "damage", "speed")
      UpgradeType type = UpgradeType::Flat;  // Flat or Multiplier
      float value = 0.0f;            // Flat: amount added; Multiplier: factor (e.g. 1.3 = x1.3)
      int max_level = 5;             // Max times this upgrade can be picked per run
    };

    // Top-level upgrade settings loaded from JSON
    struct UpgradeSettings
    {
      int base_point_threshold = 100;    // Points needed for first card pick
      float threshold_scaling = 1.5f;    // Multiplier on threshold after each pick
      std::vector<RarityTier> rarities{};         // Rarity tiers with drop weights
      std::vector<UpgradeDefinition> upgrades{};  // All available upgrades
    };
  }
}

REFLECT_FIELDS(Engine::FileIO::EngineSettings, window_title, window_width, window_height, panel_viewport_mode, load_all_assets_on_startup)
REFLECT_FIELDS(Engine::FileIO::ProjectSettings, loaded_tilemaps, layer_collision_pairs)
REFLECT_FIELDS(Engine::FileIO::LocalSettings, imgui_theme, imgui_layout, auto_reload)
REFLECT_FIELDS(Engine::FileIO::GameSettings, fullscreen, vsync, bgm_volume, sfx_volume)

REFLECT_ENUM(Engine::FileIO::UpgradeType, Flat, Multiplier)
REFLECT_FIELDS(Engine::FileIO::RarityTier, name, drop_rate)
REFLECT_FIELDS(Engine::FileIO::UpgradeDefinition, id, display_name, image_path, effect_text, description, rarity, stat_name, type, value, max_level)
REFLECT_FIELDS(Engine::FileIO::UpgradeSettings, base_point_threshold, threshold_scaling, rarities, upgrades)
