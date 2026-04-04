//-----------------------------------------------------------------------------------
// file:    UpgradeManager.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Runtime manager for the roguelike upgrade system. Holds transient
//          per-run state (points, picked levels, stat applicators) and provides
//          the API consumed by the UpgradePanel and gameplay code.
//          Stat applicators are callbacks registered by GameLogic so that
//          upgrades can modify player components (damage, speed, health, etc.)
//          without Engine needing to know about GameLogic types.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>
#include "EngineExport.h"
#include "FileIO/Settings.h"
#include "Random.h"
#include "Logger.h"

namespace Engine
{
  // Callback signature for applying a stat change to the player.
  // Parameters: (UpgradeType type, float value)
  //   Flat:       applicator should ADD `value` to the stat
  //   Multiplier: applicator should MULTIPLY the stat by `value`
  using StatApplicator = std::function<void(Engine::FileIO::UpgradeType, float)>;

  class UpgradeManager
  {
  public:
    // ---- Singleton (lives in Engine EXE, shared with DLLs) ----
    ENGINE_API static UpgradeManager& instance();
    ENGINE_API static void destroy_instance();

    // ---- Stat applicator registration (called from GameLogic) ----

    // Register a callback that knows how to modify a specific stat on the player.
    // `stat_name` must match the `stat_name` field in UpgradeDefinition JSON.
    ENGINE_API void register_stat_applicator(const std::string& stat_name, StatApplicator applicator);

    // Unregister a stat applicator (call before DLL unload).
    ENGINE_API void unregister_stat_applicator(const std::string& stat_name);

    // Unregister all stat applicators (call in UnregisterGameLogic).
    ENGINE_API void unregister_all_stat_applicators();

    // Check if an applicator is registered for a stat.
    ENGINE_API bool has_stat_applicator(const std::string& stat_name) const;

    // Get all registered stat names (for debug display).
    ENGINE_API std::vector<std::string> get_registered_stats() const;

    // ---- Points ----

    ENGINE_API void add_points(int amount);
    ENGINE_API int  get_points() const;
    ENGINE_API void set_points(int amount);

    // ---- Threshold ----

    ENGINE_API int  get_next_threshold() const;
    ENGINE_API int  get_picks_done() const;
    ENGINE_API bool can_upgrade() const;       // points >= threshold AND has eligible upgrades
    ENGINE_API bool all_upgrades_maxed() const;

    // ---- Card rolling ----

    // Rolls 2 unique upgrade card indices. Returns true if at least 1 card was rolled.
    // Results stored internally; retrieve via get_rolled_card().
    ENGINE_API bool roll_cards();
    ENGINE_API bool has_rolled_cards() const;
    ENGINE_API int  get_rolled_card(int slot) const; // slot 0 or 1, returns index into upgrades vector (-1 if none)

    // ---- Picking a card ----

    // Pick the card in `slot` (0 or 1). Applies the upgrade to the player,
    // deducts threshold points, increments picks, clears rolled cards.
    ENGINE_API void pick_card(int slot);

    // Skip both cards (deducts points, increments picks, clears cards).
    ENGINE_API void skip_cards();

    // ---- Upgrade level queries ----

    ENGINE_API int  get_upgrade_level(const std::string& upgrade_id) const;
    ENGINE_API void set_upgrade_level(const std::string& upgrade_id, int level);
    ENGINE_API bool is_upgrade_eligible(int upgrade_index) const;

    // Reapplies the current picked upgrades to the active player entity.
    // Use this after loading a fresh player prefab while preserving run state.
    ENGINE_API void apply_current_upgrades();

    // ---- LevelTwo entry snapshot ----

    // Captures the current run state so LevelTwo can restart from its entry state.
    ENGINE_API void capture_level_two_entry_state();
    ENGINE_API bool has_level_two_entry_state() const;
    ENGINE_API void restore_level_two_entry_state();
    ENGINE_API void clear_level_two_entry_state();

    // ---- Checkpoint snapshot (Level 1) ----

    // Captures upgrade state at the moment a checkpoint is activated.
    // On death the state is restored so the player keeps those upgrades.
    ENGINE_API void capture_checkpoint_state();
    ENGINE_API bool has_checkpoint_state() const;
    ENGINE_API void restore_checkpoint_state();
    ENGINE_API void clear_checkpoint_state();

    // ---- Reset ----

    ENGINE_API void reset();

  private:
    struct SavedState {
      int current_points = 0;
      int picks_done = 0;
      std::unordered_map<std::string, int> current_levels;
      bool cards_rolled = false;
      int rolled_indices[2] = { -1, -1 };
    };

    UpgradeManager() = default;

    SavedState capture_state() const;
    void restore_state(const SavedState& state);

    // Stat applicator map
    std::unordered_map<std::string, StatApplicator> stat_applicators_;

    // Runtime state (transient per-run)
    int current_points_ = 0;
    int picks_done_ = 0;
    std::unordered_map<std::string, int> current_levels_; // upgrade_id -> times picked

    // Rolled cards
    bool cards_rolled_ = false;
    int rolled_indices_[2] = { -1, -1 };

    SavedState level_two_entry_state_{};
    bool has_level_two_entry_state_ = false;

    SavedState checkpoint_state_{};
    bool has_checkpoint_state_ = false;
  };

} // namespace Engine


