//-----------------------------------------------------------------------------------
// file:    UpgradePanel.h
// project: Monkey Stole My GPA
//
// brief:   ImGui panel for editing roguelike upgrade definitions (rarities,
//          upgrades, thresholds) and simulating the card-pick upgrade loop.
//          All definition data is persisted to upgrade_settings.json via
//          SettingsManager. Runtime simulation state is transient per-session.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include "../FileIO/Settings.h"
#include "../Random.h"
#include "../Reflection/ReflectionLibrary/Helpers.h"

/*=================================================================================
Function Brief:
Renders the Upgrades ImGui panel. Divided into four sections:
  1) Rarity Tiers      — edit rarity names and drop weights
  2) Upgrade Definitions — edit individual upgrade cards (id, name, stat, type, value, rarity, max_level)
  3) Point Threshold    — edit base threshold and escalation scaling
  4) Simulation         — accumulate points, roll 2 random cards, pick one, track stats

Where to call it:
Registered via ImGuiManager::register_panel("Upgrades", UpgradePanel) in RegisterAllPanels.h.
==================================================================================*/
inline void UpgradePanel()
{
  using namespace Engine::FileIO;

  ImGui::Begin("Upgrades");

  auto& settings = SettingsManager<UpgradeSettings>::get_mutable();

  // ==================================================================================
  // Save / Reload
  // ==================================================================================
  if (ImGui::Button("Save"))
  {
    SettingsManager<UpgradeSettings>::save();
  }
  ImGui::SameLine();
  if (ImGui::Button("Reload"))
  {
    SettingsManager<UpgradeSettings>::load("Assets/settings/upgrade_settings.json");
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text("Save writes current values to upgrade_settings.json.");
    ImGui::Text("Reload re-reads the JSON file, discarding unsaved changes.");
    ImGui::EndTooltip();
  }

  ImGui::Separator();

  // ==================================================================================
  // Section 1: Point Threshold
  // ==================================================================================
  if (ImGui::CollapsingHeader("Point Threshold", ImGuiTreeNodeFlags_DefaultOpen))
  {
    bool changed = false;
    if (ImGui::DragInt("Base Threshold", &settings.base_point_threshold, 1.0f, 1, 100000))
      changed = true;
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Points needed for the first card pick.");
      ImGui::EndTooltip();
    }

    if (ImGui::DragFloat("Threshold Scaling", &settings.threshold_scaling, 0.01f, 1.0f, 10.0f, "%.2f"))
      changed = true;
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Multiplier applied to threshold after each card pick.");
      ImGui::Text("E.g. 1.5 => 100, 150, 225, 337...");
      ImGui::EndTooltip();
    }

    // Preview first few thresholds
    ImGui::TextDisabled("Preview: ");
    ImGui::SameLine();
    float t = static_cast<float>(settings.base_point_threshold);
    for (int i = 0; i < 6; ++i)
    {
      if (i > 0) ImGui::SameLine();
      ImGui::Text("%d", static_cast<int>(t));
      if (i < 5) { ImGui::SameLine(); ImGui::TextDisabled("->"); }
      t *= settings.threshold_scaling;
    }

    if (changed)
      SettingsManager<UpgradeSettings>::save();
  }

  // ==================================================================================
  // Section 2: Rarity Tiers
  // ==================================================================================
  if (ImGui::CollapsingHeader("Rarity Tiers", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // Compute total weight for normalization display
    float total_weight = 0.0f;
    for (auto& r : settings.rarities)
      total_weight += r.drop_rate;

    bool rarities_changed = false;
    int remove_idx = -1;

    for (int i = 0; i < static_cast<int>(settings.rarities.size()); ++i)
    {
      ImGui::PushID(i);
      auto& rarity = settings.rarities[i];

      // Name
      char name_buf[64]{};
      strncpy_s(name_buf, rarity.name.c_str(), sizeof(name_buf) - 1);
      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::InputText("##name", name_buf, sizeof(name_buf)))
      {
        rarity.name = name_buf;
        rarities_changed = true;
      }
      ImGui::SameLine();

      // Drop rate
      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::DragFloat("##weight", &rarity.drop_rate, 0.01f, 0.001f, 100.0f, "%.3f"))
        rarities_changed = true;
      ImGui::SameLine();

      // Normalized percentage
      float pct = (total_weight > 0.0f) ? (rarity.drop_rate / total_weight * 100.0f) : 0.0f;
      ImGui::TextDisabled("(%.1f%%)", pct);
      ImGui::SameLine();

      // Remove button
      if (ImGui::SmallButton("X##remove"))
        remove_idx = i;

      ImGui::PopID();
    }

    if (remove_idx >= 0)
    {
      settings.rarities.erase(settings.rarities.begin() + remove_idx);
      rarities_changed = true;
    }

    if (ImGui::Button("+ Add Rarity"))
    {
      settings.rarities.push_back({ "New Rarity", 0.1f });
      rarities_changed = true;
    }

    if (rarities_changed)
      SettingsManager<UpgradeSettings>::save();
  }

  // ==================================================================================
  // Section 3: Upgrade Definitions
  // ==================================================================================
  if (ImGui::CollapsingHeader("Upgrade Definitions", ImGuiTreeNodeFlags_DefaultOpen))
  {
    bool upgrades_changed = false;
    int remove_idx = -1;

    for (int i = 0; i < static_cast<int>(settings.upgrades.size()); ++i)
    {
      ImGui::PushID(i + 1000); // offset to avoid ID collision with rarities
      auto& upg = settings.upgrades[i];

      // Tree node label: "[display_name] (rarity)"
      std::string label = upg.display_name + " (" + upg.rarity + ")###upgrade" + std::to_string(i);
      if (ImGui::TreeNode(label.c_str()))
      {
        // ID
        char id_buf[64]{};
        strncpy_s(id_buf, upg.id.c_str(), sizeof(id_buf) - 1);
        if (ImGui::InputText("ID", id_buf, sizeof(id_buf)))
        {
          upg.id = id_buf;
          upgrades_changed = true;
        }

        // Display name
        char name_buf[128]{};
        strncpy_s(name_buf, upg.display_name.c_str(), sizeof(name_buf) - 1);
        if (ImGui::InputText("Display Name", name_buf, sizeof(name_buf)))
        {
          upg.display_name = name_buf;
          upgrades_changed = true;
        }

        // Stat name
        char stat_buf[64]{};
        strncpy_s(stat_buf, upg.stat_name.c_str(), sizeof(stat_buf) - 1);
        if (ImGui::InputText("Stat Name", stat_buf, sizeof(stat_buf)))
        {
          upg.stat_name = stat_buf;
          upgrades_changed = true;
        }

        // Rarity combo (populated from settings.rarities)
        if (ImGui::BeginCombo("Rarity", upg.rarity.c_str()))
        {
          for (auto& r : settings.rarities)
          {
            bool selected = (upg.rarity == r.name);
            if (ImGui::Selectable(r.name.c_str(), selected))
            {
              upg.rarity = r.name;
              upgrades_changed = true;
            }
            if (selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }

        // Type combo (Flat / Multiplier)
        const char* type_names[] = { "Flat", "Multiplier" };
        int current_type = static_cast<int>(upg.type);
        if (ImGui::Combo("Type", &current_type, type_names, 2))
        {
          upg.type = static_cast<UpgradeType>(current_type);
          upgrades_changed = true;
        }

        // Value
        if (upg.type == UpgradeType::Flat)
        {
          if (ImGui::DragFloat("Value (+)", &upg.value, 0.5f, 0.0f, 10000.0f, "%.1f"))
            upgrades_changed = true;
          ImGui::SameLine();
          ImGui::TextDisabled("Added to stat each pick");
        }
        else
        {
          if (ImGui::DragFloat("Value (x)", &upg.value, 0.01f, 0.01f, 100.0f, "%.2f"))
            upgrades_changed = true;
          ImGui::SameLine();
          ImGui::TextDisabled("Stat multiplied by this each pick");
        }

        // Max level
        if (ImGui::DragInt("Max Level", &upg.max_level, 0.1f, 1, 99))
          upgrades_changed = true;

        // Remove button
        ImGui::Spacing();
        if (ImGui::SmallButton("Remove Upgrade"))
          remove_idx = i;

        ImGui::TreePop();
      }

      ImGui::PopID();
    }

    if (remove_idx >= 0)
    {
      settings.upgrades.erase(settings.upgrades.begin() + remove_idx);
      upgrades_changed = true;
    }

    if (ImGui::Button("+ Add Upgrade"))
    {
      UpgradeDefinition new_upg;
      new_upg.id = "new_upgrade_" + std::to_string(settings.upgrades.size());
      new_upg.display_name = "New Upgrade";
      new_upg.rarity = settings.rarities.empty() ? "Common" : settings.rarities[0].name;
      new_upg.stat_name = "damage";
      new_upg.type = UpgradeType::Flat;
      new_upg.value = 5.0f;
      new_upg.max_level = 5;
      settings.upgrades.push_back(new_upg);
      upgrades_changed = true;
    }

    if (upgrades_changed)
      SettingsManager<UpgradeSettings>::save();
  }

  // ==================================================================================
  // Section 4: Simulation
  // ==================================================================================
  if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen))
  {
    // ---- Transient simulation state ----
    static int current_points = 0;
    static int picks_done = 0;
    static std::unordered_map<std::string, int> current_levels;        // upgrade id -> times picked
    static std::unordered_map<std::string, float> stat_values;         // stat_name -> current value
    static bool cards_rolled = false;
    static int rolled_card_indices[2] = { -1, -1 };                    // indices into settings.upgrades

    // Helper: compute next threshold
    auto compute_threshold = [&]() -> int {
      return static_cast<int>(std::floor(
        static_cast<float>(settings.base_point_threshold) *
        std::pow(settings.threshold_scaling, static_cast<float>(picks_done))
      ));
    };

    int next_threshold = compute_threshold();

    // ---- Points controls ----
    ImGui::Text("Current Points: %d", current_points);
    ImGui::SameLine();
    ImGui::Text("  |  Next Threshold: %d", next_threshold);
    ImGui::SameLine();
    ImGui::Text("  |  Picks Done: %d", picks_done);

    static int custom_points = 50;
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputInt("##custom_pts", &custom_points);
    ImGui::SameLine();
    if (ImGui::Button("Add Points"))
      current_points += custom_points;
    ImGui::SameLine();
    if (ImGui::Button("+50"))
      current_points += 50;
    ImGui::SameLine();
    if (ImGui::Button("+100"))
      current_points += 100;
    ImGui::SameLine();
    if (ImGui::Button("+500"))
      current_points += 500;

    ImGui::Separator();

    // ---- Check if all upgrades are maxed ----
    auto is_eligible = [&](int idx) -> bool {
      auto& upg = settings.upgrades[idx];
      int lvl = 0;
      auto it = current_levels.find(upg.id);
      if (it != current_levels.end()) lvl = it->second;
      return lvl < upg.max_level;
    };

    bool any_eligible = false;
    for (int i = 0; i < static_cast<int>(settings.upgrades.size()); ++i)
    {
      if (is_eligible(i)) { any_eligible = true; break; }
    }

    // ---- Roll Cards logic ----
    bool can_upgrade = (current_points >= next_threshold) && any_eligible && !cards_rolled;

    if (!any_eligible)
    {
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "All Upgrades Maxed!");
    }
    else if (current_points >= next_threshold && !cards_rolled)
    {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.3f, 1.0f), "UPGRADE AVAILABLE!");
      ImGui::SameLine();
      if (ImGui::Button("Roll Cards"))
      {
        // Roll 2 unique cards using rarity-weighted random
        rolled_card_indices[0] = -1;
        rolled_card_indices[1] = -1;

        // Compute total rarity weight
        float total_weight = 0.0f;
        for (auto& r : settings.rarities)
          total_weight += r.drop_rate;

        if (total_weight > 0.0f && !settings.upgrades.empty())
        {
          // Lambda: roll one card, excluding a specific index
          auto roll_one_card = [&](int exclude_idx) -> int {
            constexpr int max_attempts = 50;
            for (int attempt = 0; attempt < max_attempts; ++attempt)
            {
              // 1) Roll a rarity
              float roll = Engine::Random::random_float() * total_weight;
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
                if (!is_eligible(j)) continue;
                candidates.push_back(j);
              }

              // 3) Pick randomly from candidates
              if (!candidates.empty())
              {
                int pick = static_cast<int>(Engine::Random::random_float() * static_cast<float>(candidates.size()));
                if (pick >= static_cast<int>(candidates.size())) pick = static_cast<int>(candidates.size()) - 1;
                return candidates[pick];
              }
              // Rarity had no eligible candidates — re-roll
            }
            // Fallback: pick any eligible upgrade
            for (int j = 0; j < static_cast<int>(settings.upgrades.size()); ++j)
            {
              if (j == exclude_idx) continue;
              if (is_eligible(j)) return j;
            }
            return -1;
          };

          rolled_card_indices[0] = roll_one_card(-1);
          rolled_card_indices[1] = roll_one_card(rolled_card_indices[0]);
          cards_rolled = true;
        }
      }
    }
    else if (!cards_rolled)
    {
      float progress = (next_threshold > 0) ? static_cast<float>(current_points) / static_cast<float>(next_threshold) : 0.0f;
      if (progress > 1.0f) progress = 1.0f;
      ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
    }

    // ---- Display rolled cards ----
    if (cards_rolled)
    {
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "Choose an upgrade card:");
      ImGui::Spacing();

      // Helper: get rarity color
      auto get_rarity_color = [](const std::string& rarity) -> ImVec4 {
        if (rarity == "Common")  return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        if (rarity == "Rare")    return ImVec4(0.3f, 0.5f, 1.0f, 1.0f);
        if (rarity == "Epic")    return ImVec4(0.7f, 0.3f, 1.0f, 1.0f);
        return ImVec4(1.0f, 0.85f, 0.0f, 1.0f); // default gold for unknown
      };

      float panel_width = ImGui::GetContentRegionAvail().x;
      float card_width = (panel_width - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

      for (int c = 0; c < 2; ++c)
      {
        if (c == 1) ImGui::SameLine();

        int idx = rolled_card_indices[c];
        ImGui::BeginGroup();
        {
          if (idx >= 0 && idx < static_cast<int>(settings.upgrades.size()))
          {
            auto& upg = settings.upgrades[idx];
            int current_lvl = 0;
            auto it = current_levels.find(upg.id);
            if (it != current_levels.end()) current_lvl = it->second;

            ImVec4 rarity_col = get_rarity_color(upg.rarity);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(rarity_col.x * 0.15f, rarity_col.y * 0.15f, rarity_col.z * 0.15f, 1.0f));
            ImGui::BeginChild(("card" + std::to_string(c)).c_str(), ImVec2(card_width, 140), true);

            // Rarity label
            ImGui::TextColored(rarity_col, "[%s]", upg.rarity.c_str());

            // Display name
            ImGui::Text("%s", upg.display_name.c_str());

            // Type + value
            if (upg.type == UpgradeType::Flat)
              ImGui::Text("  +%.1f %s", upg.value, upg.stat_name.c_str());
            else
              ImGui::Text("  x%.2f %s", upg.value, upg.stat_name.c_str());

            // Level info
            ImGui::TextDisabled("Level: %d / %d", current_lvl, upg.max_level);

            // Pick button
            ImGui::Spacing();
            std::string btn_label = "Pick##card" + std::to_string(c);
            if (ImGui::Button(btn_label.c_str(), ImVec2(-1, 0)))
            {
              // Apply upgrade
              current_levels[upg.id]++;

              // Initialize stat if needed
              if (stat_values.find(upg.stat_name) == stat_values.end())
                stat_values[upg.stat_name] = 0.0f;

              if (upg.type == UpgradeType::Flat)
                stat_values[upg.stat_name] += upg.value;
              else
                stat_values[upg.stat_name] *= upg.value; // multiplier

              // Deduct threshold points
              current_points -= next_threshold;
              picks_done++;

              // Clear rolled cards
              cards_rolled = false;
              rolled_card_indices[0] = -1;
              rolled_card_indices[1] = -1;
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
          }
          else
          {
            ImGui::BeginChild(("card_empty" + std::to_string(c)).c_str(), ImVec2(card_width, 140), true);
            ImGui::TextDisabled("(no eligible upgrade)");
            ImGui::EndChild();
          }
        }
        ImGui::EndGroup();
      }

      // Skip button
      ImGui::Spacing();
      if (ImGui::SmallButton("Skip (discard both cards)"))
      {
        current_points -= next_threshold;
        picks_done++;
        cards_rolled = false;
        rolled_card_indices[0] = -1;
        rolled_card_indices[1] = -1;
      }
    }

    ImGui::Separator();

    // ---- Stats Overview ----
    ImGui::Text("Stat Overview:");

    if (stat_values.empty())
    {
      ImGui::TextDisabled("  No upgrades picked yet.");
    }
    else
    {
      if (ImGui::BeginTable("##stats_table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
      {
        ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableHeadersRow();

        for (auto& [name, val] : stat_values)
        {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::Text("%s", name.c_str());
          ImGui::TableNextColumn();
          ImGui::Text("%.2f", val);
        }

        ImGui::EndTable();
      }
    }

    // ---- Upgrade Levels Overview ----
    if (!current_levels.empty())
    {
      ImGui::Spacing();
      ImGui::Text("Upgrade Levels:");
      if (ImGui::BeginTable("##levels_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
      {
        ImGui::TableSetupColumn("Upgrade", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        for (auto& [id, lvl] : current_levels)
        {
          // Find display name and max level
          std::string disp = id;
          int max_lvl = 0;
          for (auto& upg : settings.upgrades)
          {
            if (upg.id == id)
            {
              disp = upg.display_name;
              max_lvl = upg.max_level;
              break;
            }
          }

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::Text("%s", disp.c_str());
          ImGui::TableNextColumn();
          ImGui::Text("%d", lvl);
          ImGui::TableNextColumn();
          ImGui::Text("%d", max_lvl);
        }

        ImGui::EndTable();
      }
    }

    // ---- Reset ----
    ImGui::Spacing();
    if (ImGui::Button("Reset Simulation"))
    {
      current_points = 0;
      picks_done = 0;
      current_levels.clear();
      stat_values.clear();
      cards_rolled = false;
      rolled_card_indices[0] = -1;
      rolled_card_indices[1] = -1;
    }
  }

  ImGui::End();
}
