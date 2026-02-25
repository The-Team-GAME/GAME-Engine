//-----------------------------------------------------------------------------------
// file:    UpgradeManager.h
// project: Monkey Stole My GPA
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
    ENGINE_API bool is_upgrade_eligible(int upgrade_index) const;

    // ---- Reset ----

    ENGINE_API void reset();

    // ---- In-game picker UI signaling ----

    // Signal the UpgradePickerUI panel to open the card-pick modal this frame.
    ENGINE_API void request_upgrade_picker();

    // Consumed once by UpgradePickerUI; returns true exactly once per request.
    ENGINE_API bool consume_upgrade_picker_request();

  private:
    UpgradeManager() = default;

    // Stat applicator map
    std::unordered_map<std::string, StatApplicator> stat_applicators_;

    // Runtime state (transient per-run)
    int current_points_ = 0;
    int picks_done_ = 0;
    std::unordered_map<std::string, int> current_levels_; // upgrade_id -> times picked

    // Rolled cards
    bool cards_rolled_ = false;
    int rolled_indices_[2] = { -1, -1 };

    // UI signal
    bool upgrade_picker_requested_ = false;
  };

} // namespace Engine
