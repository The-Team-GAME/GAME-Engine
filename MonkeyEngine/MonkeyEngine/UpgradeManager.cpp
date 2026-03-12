//-----------------------------------------------------------------------------------
// file:    UpgradeManager.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the UpgradeManager singleton. Handles runtime
//          upgrade state, card rolling with rarity-weighted random, and
//          dispatching stat changes to registered applicator callbacks.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "UpgradeManager.h"

namespace Engine
{
  // ---- Singleton ----

  namespace { UpgradeManager* g_upgrade_manager = nullptr; }

  UpgradeManager& UpgradeManager::instance()
  {
    if (!g_upgrade_manager) g_upgrade_manager = new UpgradeManager();
    return *g_upgrade_manager;
  }

  void UpgradeManager::destroy_instance()
  {
    delete g_upgrade_manager;
    g_upgrade_manager = nullptr;
  }

  // ---- Stat applicator registration ----

  void UpgradeManager::register_stat_applicator(const std::string& stat_name, StatApplicator applicator)
  {
    stat_applicators_[stat_name] = std::move(applicator);
    Log::info("[UpgradeManager] Registered stat applicator: {}", stat_name);
  }

  void UpgradeManager::unregister_stat_applicator(const std::string& stat_name)
  {
    stat_applicators_.erase(stat_name);
  }

  void UpgradeManager::unregister_all_stat_applicators()
  {
    stat_applicators_.clear();
  }

  bool UpgradeManager::has_stat_applicator(const std::string& stat_name) const
  {
    return stat_applicators_.count(stat_name) > 0;
  }

  std::vector<std::string> UpgradeManager::get_registered_stats() const
  {
    std::vector<std::string> stats;
    stats.reserve(stat_applicators_.size());
    for (auto& [name, _] : stat_applicators_)
      stats.push_back(name);
    return stats;
  }

  // ---- Points ----

  void UpgradeManager::add_points(int amount)
  {
    current_points_ += amount;
  }

  int UpgradeManager::get_points() const
  {
    return current_points_;
  }

  void UpgradeManager::set_points(int amount)
  {
    current_points_ = amount;
  }

  // ---- Threshold ----

  int UpgradeManager::get_next_threshold() const
  {
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();
    return static_cast<int>(std::floor(
      static_cast<float>(settings.base_point_threshold) *
      std::pow(settings.threshold_scaling, static_cast<float>(picks_done_))
    ));
  }

  int UpgradeManager::get_picks_done() const
  {
    return picks_done_;
  }

  bool UpgradeManager::can_upgrade() const
  {
    return (current_points_ >= get_next_threshold()) && !all_upgrades_maxed() && !cards_rolled_;
  }

  bool UpgradeManager::all_upgrades_maxed() const
  {
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();
    for (int i = 0; i < static_cast<int>(settings.upgrades.size()); ++i)
    {
      if (is_upgrade_eligible(i))
        return false;
    }
    return true;
  }

  // ---- Card rolling ----

  bool UpgradeManager::roll_cards()
  {
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();

    rolled_indices_[0] = -1;
    rolled_indices_[1] = -1;
    cards_rolled_ = false;

    // Compute total rarity weight
    float total_weight = 0.0f;
    for (auto& r : settings.rarities)
      total_weight += r.drop_rate;

    if (total_weight <= 0.0f || settings.upgrades.empty())
      return false;

    // Lambda: roll one card, excluding a specific index
    auto roll_one_card = [&](int exclude_idx) -> int {
      constexpr int max_attempts = 50;
      for (int attempt = 0; attempt < max_attempts; ++attempt)
      {
        // 1) Roll a rarity
        float roll = Random::random_float() * total_weight;
        std::string chosen_rarity;
        float cumulative = 0.0f;
        for (auto& r : settings.rarities)
        {
          cumulative += r.drop_rate;
          if (roll < cumulative)
          {
            chosen_rarity = r.name;
            break;
          }
        }
        if (chosen_rarity.empty())
          chosen_rarity = settings.rarities.back().name;

        // 2) Collect eligible upgrades of this rarity
        std::vector<int> candidates;
        for (int j = 0; j < static_cast<int>(settings.upgrades.size()); ++j)
        {
          if (j == exclude_idx) continue;
          if (settings.upgrades[j].rarity != chosen_rarity) continue;
          if (!is_upgrade_eligible(j)) continue;
          candidates.push_back(j);
        }

        // 3) Pick randomly from candidates
        if (!candidates.empty())
        {
          int pick = static_cast<int>(Random::random_float() * static_cast<float>(candidates.size()));
          if (pick >= static_cast<int>(candidates.size()))
            pick = static_cast<int>(candidates.size()) - 1;
          return candidates[pick];
        }
        // Rarity had no eligible candidates — re-roll
      }
      // Fallback: pick any eligible upgrade
      for (int j = 0; j < static_cast<int>(settings.upgrades.size()); ++j)
      {
        if (j == exclude_idx) continue;
        if (is_upgrade_eligible(j)) return j;
      }
      return -1;
      };

    rolled_indices_[0] = roll_one_card(-1);
    rolled_indices_[1] = roll_one_card(rolled_indices_[0]);
    cards_rolled_ = (rolled_indices_[0] >= 0 || rolled_indices_[1] >= 0);
    return cards_rolled_;
  }

  bool UpgradeManager::has_rolled_cards() const
  {
    return cards_rolled_;
  }

  int UpgradeManager::get_rolled_card(int slot) const
  {
    if (slot < 0 || slot > 1) return -1;
    return rolled_indices_[slot];
  }

  // ---- Picking a card ----

  void UpgradeManager::pick_card(int slot)
  {
    if (slot < 0 || slot > 1 || !cards_rolled_) return;

    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();
    int idx = rolled_indices_[slot];
    if (idx < 0 || idx >= static_cast<int>(settings.upgrades.size())) return;

    auto& upg = settings.upgrades[idx];

    // Increment level
    current_levels_[upg.id]++;

    // Apply to player via stat applicator
    auto it = stat_applicators_.find(upg.stat_name);
    if (it != stat_applicators_.end())
    {
      it->second(upg.type, upg.value);
      Log::info("[UpgradeManager] Applied {} upgrade '{}': {} {} = {}",
        upg.type == FileIO::UpgradeType::Flat ? "Flat" : "Multiplier",
        upg.display_name, upg.stat_name,
        upg.type == FileIO::UpgradeType::Flat ? "+" : "x",
        upg.value);
    }
    else
    {
      Log::warning("[UpgradeManager] No stat applicator registered for '{}'", upg.stat_name);
    }

    // Deduct threshold and advance
    current_points_ -= get_next_threshold();
    if (current_points_ < 0) current_points_ = 0;
    picks_done_++;

    // Clear rolled cards
    cards_rolled_ = false;
    rolled_indices_[0] = -1;
    rolled_indices_[1] = -1;
  }

  void UpgradeManager::skip_cards()
  {
    if (!cards_rolled_) return;

    current_points_ -= get_next_threshold();
    if (current_points_ < 0) current_points_ = 0;
    picks_done_++;

    cards_rolled_ = false;
    rolled_indices_[0] = -1;
    rolled_indices_[1] = -1;
  }

  // ---- Upgrade level queries ----

  int UpgradeManager::get_upgrade_level(const std::string& upgrade_id) const
  {
    auto it = current_levels_.find(upgrade_id);
    return (it != current_levels_.end()) ? it->second : 0;
  }

  void UpgradeManager::set_upgrade_level(const std::string& upgrade_id, int level)
  {
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();
    int max_level = 0;
    for (auto& upg : settings.upgrades)
    {
      if (upg.id == upgrade_id) { max_level = upg.max_level; break; }
    }
    int clamped = std::max(0, std::min(level, max_level));
    if (clamped == 0)
      current_levels_.erase(upgrade_id);
    else
      current_levels_[upgrade_id] = clamped;
  }

  bool UpgradeManager::is_upgrade_eligible(int upgrade_index) const
  {
    auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();
    if (upgrade_index < 0 || upgrade_index >= static_cast<int>(settings.upgrades.size()))
      return false;
    auto& upg = settings.upgrades[upgrade_index];
    return get_upgrade_level(upg.id) < upg.max_level;
  }

  // ---- Reset ----

  void UpgradeManager::reset()
  {
    current_points_ = 0;
    picks_done_ = 0;
    current_levels_.clear();
    cards_rolled_ = false;
    rolled_indices_[0] = -1;
    rolled_indices_[1] = -1;
  }

} // namespace Engine
