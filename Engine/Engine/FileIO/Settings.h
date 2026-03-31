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
      std::vector<std::string> build_export_selected_assets{};  // Build Size Analyzer: remembered asset selections
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
      std::string rarity = "Common";  // Must match a RarityTier::name
      std::string stat_name = "";     // Which stat this targets (e.g. "damage", "speed")
      UpgradeType type = UpgradeType::Flat;  // Flat or Multiplier
      float value = 0.0f;            // Flat: amount added; Multiplier: factor (e.g. 1.3 = x1.3)
      int max_level = 5;             // Max times this upgrade can be picked per run
      int card_index = 0;            // Index into Cards_Spritesheet.png (0-5)
    };

    // Top-level upgrade settings loaded from JSON
    struct UpgradeSettings
    {
      int base_point_threshold = 100;    // Points needed for first card pick
      float threshold_scaling = 1.5f;    // Multiplier on threshold after each pick
      std::vector<RarityTier> rarities{};         // Rarity tiers with drop weights
      std::vector<UpgradeDefinition> upgrades{};  // All available upgrades
    };

    // ==================================================================================
    // Firebase Cloud Firestore Settings
    // ==================================================================================

    // Configuration for connecting to a Firebase Realtime Database project.
    // Game developers fill in their Firebase project details in firebase_settings.json.
    struct FirebaseSettings
    {
      std::string database_url = "";   // RTDB URL (e.g. "https://myproject-default-rtdb.firebaseio.com")
      std::string api_key = "";        // Firebase Web API key (from Firebase Console → Project Settings)
      std::string client_id = "";      // Google OAuth 2.0 client ID (Desktop type)
      std::string client_secret = "";  // Google OAuth 2.0 client secret
    };

    // ==================================================================================
    // Card Animation Settings (upgrade picker UI effects)
    // ==================================================================================
    struct CardAnimSettings
    {
      // Effect toggles
      bool enable_flip_reveal = true;
      bool enable_idle_float = true;
      bool enable_hover_lift = true;
      bool enable_selection_stamp = true;
      bool enable_hover_tilt = true;
      bool enable_idle_tilt = true;
      bool enable_card_intro = true;
      bool enable_card_arc = true;
      bool enable_burst_particles = true;

      // Flip Reveal
      float flip_duration = 0.3f;
      float flip_stagger = 0.08f;

      // Card Intro Entry
      float intro_duration = 0.90f;
      float intro_start_offset = 300.0f;

      // Idle Float
      float float_amplitude = 3.0f;
      float float_speed = 0.2f;
      float float_phase_offset = 0.5f;

      // Hover Lift
      float hover_lift_y = -15.0f;
      float hover_scale_boost = 1.05f;
      float hover_speed = 8.0f;

      // Idle Tilt
      float idle_tilt_amplitude = 1.0f;
      float idle_tilt_speed = 0.3f;
      float idle_tilt_phase = 1.2f;
      float idle_tilt_lerp_speed = 6.0f;

      // Hover Tilt
      float max_tilt_deg = 3.0f;
      float tilt_speed = 2.0f;

      // Selection Stamp
      float stamp_duration = 0.15f;
      float stamp_squish = 0.85f;

      // Card Arc
      float arc_duration = 0.4f;
      float arc_peak = -40.0f;
      float arc_spin_deg = 8.0f;
      float smash_duration = 0.12f;
      float linger_duration = 1.0f;

      // Burst Particles
      float particle_speed_min = 80.0f;
      float particle_speed_max = 180.0f;
      float particle_life_min = 0.2f;
      float particle_life_max = 0.35f;
      int   particle_count_min = 6;
      int   particle_count_max = 10;
      float particle_size = 20.0f;
      float particle_gravity = 1200.0f;
      float particle_angle_min = 2.356f;  // ~0.75π — upward-biased cone lower bound
      float particle_angle_max = 7.069f;  // ~2.25π — upward-biased cone upper bound

      // Mini-card stack on panel
      bool  enable_card_stack = true;
      float card_stack_offset = 8.0f;   // px shift per card when stacking
      float pre_shift_duration = 0.25f; // seconds for existing cards to slide before arc

      // Mini-card idle wave
      bool  enable_mini_wave = true;
      float mini_wave_amplitude = 3.0f; // px vertical bob
      float mini_wave_speed = 1.2f;     // cycles per second
      float mini_wave_phase = 0.4f;     // phase offset between adjacent cards

      // Mini-card tilt
      bool  enable_mini_tilt = true;
      float mini_tilt_amplitude = 3.0f; // degrees
      float mini_tilt_speed = 0.8f;     // cycles per second
      float mini_tilt_phase = 0.5f;     // phase offset between adjacent cards

      // Mini-card hover
      bool  enable_mini_hover = true;
      float mini_hover_lift = -8.0f;   // px upward shift (negative = up)
      float mini_hover_scale = 1.4f;    // scale multiplier on hover
      float mini_hover_speed = 10.0f;   // lerp speed
      float mini_hover_tilt_deg = 5.0f; // max tilt degrees on hover (mouse-X-based)
      float mini_hover_tilt_speed = 8.0f; // tilt lerp speed

      // Smash ripple on mini-cards
      bool  enable_smash_ripple = true;
      float ripple_amplitude = 8.0f;    // px peak displacement
      float ripple_speed = 300.0f;      // px/s propagation speed
      float ripple_decay = 4.0f;        // how fast ripple fades (1/s)
    };
  }
}

REFLECT_FIELDS(Engine::FileIO::EngineSettings, window_title, window_width, window_height, panel_viewport_mode, load_all_assets_on_startup)
REFLECT_FIELDS(Engine::FileIO::ProjectSettings, loaded_tilemaps, layer_collision_pairs)
REFLECT_FIELDS(Engine::FileIO::LocalSettings, imgui_theme, imgui_layout, auto_reload, build_export_selected_assets)
REFLECT_FIELDS(Engine::FileIO::GameSettings, fullscreen, vsync, bgm_volume, sfx_volume)
REFLECT_FIELDS(Engine::FileIO::FirebaseSettings, database_url, api_key, client_id, client_secret)

REFLECT_ENUM(Engine::FileIO::UpgradeType, Flat, Multiplier)
REFLECT_FIELDS(Engine::FileIO::RarityTier, name, drop_rate)
REFLECT_FIELDS(Engine::FileIO::UpgradeDefinition, id, display_name, rarity, stat_name, type, value, max_level, card_index)
REFLECT_FIELDS(Engine::FileIO::UpgradeSettings, base_point_threshold, threshold_scaling, rarities, upgrades)
REFLECT_FIELDS(Engine::FileIO::CardAnimSettings,
  enable_flip_reveal, enable_idle_float, enable_hover_lift, enable_selection_stamp,
  enable_hover_tilt, enable_idle_tilt, enable_card_intro, enable_card_arc, enable_burst_particles,
  flip_duration, flip_stagger,
  intro_duration, intro_start_offset,
  float_amplitude, float_speed, float_phase_offset,
  hover_lift_y, hover_scale_boost, hover_speed,
  idle_tilt_amplitude, idle_tilt_speed, idle_tilt_phase, idle_tilt_lerp_speed,
  max_tilt_deg, tilt_speed,
  stamp_duration, stamp_squish,
  arc_duration, arc_peak, arc_spin_deg, smash_duration, linger_duration,
  particle_speed_min, particle_speed_max, particle_life_min, particle_life_max,
  particle_count_min, particle_count_max, particle_size, particle_gravity,
  particle_angle_min, particle_angle_max,
  enable_card_stack, card_stack_offset, pre_shift_duration,
  enable_mini_wave, mini_wave_amplitude, mini_wave_speed, mini_wave_phase,
  enable_mini_tilt, mini_tilt_amplitude, mini_tilt_speed, mini_tilt_phase,
  enable_mini_hover, mini_hover_lift, mini_hover_scale, mini_hover_speed, mini_hover_tilt_deg, mini_hover_tilt_speed,
  enable_smash_ripple, ripple_amplitude, ripple_speed, ripple_decay)
