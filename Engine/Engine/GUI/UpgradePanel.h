//-----------------------------------------------------------------------------------
// file:    UpgradePanel.h
// project: Monkey Stole My GPA
//
// brief:   ImGui panel for editing roguelike upgrade definitions (rarities,
//          upgrades, thresholds) and controlling the live card-pick upgrade
//          system via UpgradeManager. Upgrades are applied to the actual player
//          entity through registered stat applicators.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include "../FileIO/Settings.h"
#include "../UpgradeManager.h"

/*=================================================================================
Function Brief:
Renders the Upgrades ImGui panel. Divided into sections:
  1) Point Threshold    — edit base threshold and escalation scaling
  2) Rarity Tiers       — edit rarity names and drop weights
  3) Upgrade Definitions — edit individual upgrade cards
  4) Live Upgrade System — real-time points, card rolling, picking (affects player)

Where to call it:
Registered via ImGuiManager::register_panel("Upgrades", UpgradePanel) in RegisterAllPanels.h.
==================================================================================*/
inline void UpgradePanel()
{
  using namespace Engine::FileIO;

  ImGui::Begin("Upgrades");

  auto& settings = SettingsManager<UpgradeSettings>::get_mutable();
  auto& mgr = Engine::UpgradeManager::instance();

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
    float total_weight = 0.0f;
    for (auto& r : settings.rarities)
      total_weight += r.drop_rate;

    bool rarities_changed = false;
    int remove_idx = -1;

    for (int i = 0; i < static_cast<int>(settings.rarities.size()); ++i)
    {
      ImGui::PushID(i);
      auto& rarity = settings.rarities[i];

      char name_buf[64]{};
      strncpy_s(name_buf, rarity.name.c_str(), sizeof(name_buf) - 1);
      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::InputText("##name", name_buf, sizeof(name_buf)))
      {
        rarity.name = name_buf;
        rarities_changed = true;
      }
      ImGui::SameLine();

      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::DragFloat("##weight", &rarity.drop_rate, 0.01f, 0.001f, 100.0f, "%.3f"))
        rarities_changed = true;
      ImGui::SameLine();

      float pct = (total_weight > 0.0f) ? (rarity.drop_rate / total_weight * 100.0f) : 0.0f;
      ImGui::TextDisabled("(%.1f%%)", pct);
      ImGui::SameLine();

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
      ImGui::PushID(i + 1000);
      auto& upg = settings.upgrades[i];

      std::string label = upg.display_name + " (" + upg.rarity + ")###upgrade" + std::to_string(i);
      if (ImGui::TreeNode(label.c_str()))
      {
        char id_buf[64]{};
        strncpy_s(id_buf, upg.id.c_str(), sizeof(id_buf) - 1);
        if (ImGui::InputText("ID", id_buf, sizeof(id_buf)))
        {
          upg.id = id_buf;
          upgrades_changed = true;
        }

        char name_buf[128]{};
        strncpy_s(name_buf, upg.display_name.c_str(), sizeof(name_buf) - 1);
        if (ImGui::InputText("Display Name", name_buf, sizeof(name_buf)))
        {
          upg.display_name = name_buf;
          upgrades_changed = true;
        }

        char stat_buf[64]{};
        strncpy_s(stat_buf, upg.stat_name.c_str(), sizeof(stat_buf) - 1);
        if (ImGui::InputText("Stat Name", stat_buf, sizeof(stat_buf)))
        {
          upg.stat_name = stat_buf;
          upgrades_changed = true;
        }

        // Show whether a stat applicator is registered for this stat
        if (mgr.has_stat_applicator(upg.stat_name))
          ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.3f, 1.0f), "  Applicator: registered");
        else
          ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 1.0f), "  Applicator: NOT registered");

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

        const char* type_names[] = { "Flat", "Multiplier" };
        int current_type = static_cast<int>(upg.type);
        if (ImGui::Combo("Type", &current_type, type_names, 2))
        {
          upg.type = static_cast<UpgradeType>(current_type);
          upgrades_changed = true;
        }

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

        if (ImGui::DragInt("Max Level", &upg.max_level, 0.1f, 1, 99))
          upgrades_changed = true;

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
  // Section 4: Live Upgrade System
  // ==================================================================================
  if (ImGui::CollapsingHeader("Live Upgrade System", ImGuiTreeNodeFlags_DefaultOpen))
  {
    int next_threshold = mgr.get_next_threshold();
    int points = mgr.get_points();
    int picks = mgr.get_picks_done();

    // ---- Registered applicators ----
    auto registered = mgr.get_registered_stats();
    ImGui::TextDisabled("Registered stat applicators: %d", static_cast<int>(registered.size()));
    if (!registered.empty() && ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      for (auto& s : registered)
        ImGui::Text("  %s", s.c_str());
      ImGui::EndTooltip();
    }

    ImGui::Separator();

    // ---- Points controls ----
    ImGui::Text("Points: %d", points);
    ImGui::SameLine();
    ImGui::Text("  |  Threshold: %d", next_threshold);
    ImGui::SameLine();
    ImGui::Text("  |  Picks: %d", picks);

    static int custom_points = 50;
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputInt("##custom_pts", &custom_points);
    ImGui::SameLine();
    if (ImGui::Button("Add Points"))
      mgr.add_points(custom_points);
    ImGui::SameLine();
    if (ImGui::Button("+50"))
      mgr.add_points(50);
    ImGui::SameLine();
    if (ImGui::Button("+100"))
      mgr.add_points(100);
    ImGui::SameLine();
    if (ImGui::Button("+500"))
      mgr.add_points(500);

    ImGui::Separator();

    // ---- Upgrade status ----
    if (mgr.all_upgrades_maxed())
    {
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "All Upgrades Maxed!");
    }
    else if (mgr.can_upgrade())
    {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.3f, 1.0f), "UPGRADE AVAILABLE!");
      ImGui::SameLine();
      if (ImGui::Button("Roll Cards"))
      {
        mgr.roll_cards();
      }
    }
    else if (!mgr.has_rolled_cards())
    {
      float progress = (next_threshold > 0) ? static_cast<float>(points) / static_cast<float>(next_threshold) : 0.0f;
      if (progress > 1.0f) progress = 1.0f;
      ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
    }

    // ---- Display rolled cards ----
    if (mgr.has_rolled_cards())
    {
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "Choose an upgrade card:");
      ImGui::Spacing();

      auto get_rarity_color = [](const std::string& rarity) -> ImVec4 {
        if (rarity == "Common")  return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        if (rarity == "Rare")    return ImVec4(0.3f, 0.5f, 1.0f, 1.0f);
        if (rarity == "Epic")    return ImVec4(0.7f, 0.3f, 1.0f, 1.0f);
        return ImVec4(1.0f, 0.85f, 0.0f, 1.0f);
        };

      float panel_width = ImGui::GetContentRegionAvail().x;
      float card_width = (panel_width - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

      for (int c = 0; c < 2; ++c)
      {
        if (c == 1) ImGui::SameLine();

        int idx = mgr.get_rolled_card(c);
        ImGui::BeginGroup();
        {
          if (idx >= 0 && idx < static_cast<int>(settings.upgrades.size()))
          {
            auto& upg = settings.upgrades[idx];
            int current_lvl = mgr.get_upgrade_level(upg.id);

            ImVec4 rarity_col = get_rarity_color(upg.rarity);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(rarity_col.x * 0.15f, rarity_col.y * 0.15f, rarity_col.z * 0.15f, 1.0f));
            ImGui::BeginChild(("card" + std::to_string(c)).c_str(), ImVec2(card_width, 160), true);

            ImGui::TextColored(rarity_col, "[%s]", upg.rarity.c_str());
            ImGui::Text("%s", upg.display_name.c_str());

            if (upg.type == UpgradeType::Flat)
              ImGui::Text("  +%.1f %s", upg.value, upg.stat_name.c_str());
            else
              ImGui::Text("  x%.2f %s", upg.value, upg.stat_name.c_str());

            ImGui::TextDisabled("Level: %d / %d", current_lvl, upg.max_level);

            if (!mgr.has_stat_applicator(upg.stat_name))
              ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 0.8f), "(no applicator!)");

            ImGui::Spacing();
            std::string btn_label = "Pick##card" + std::to_string(c);
            if (ImGui::Button(btn_label.c_str(), ImVec2(-1, 0)))
            {
              mgr.pick_card(c);
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
          }
          else
          {
            ImGui::BeginChild(("card_empty" + std::to_string(c)).c_str(), ImVec2(card_width, 160), true);
            ImGui::TextDisabled("(no eligible upgrade)");
            ImGui::EndChild();
          }
        }
        ImGui::EndGroup();
      }

      ImGui::Spacing();
      if (ImGui::SmallButton("Skip (discard both cards)"))
      {
        mgr.skip_cards();
      }
    }

    ImGui::Separator();

    // ---- Upgrade Levels Overview ----
    ImGui::Text("Upgrade Levels:");
    bool has_any_level = false;
    for (auto& upg : settings.upgrades)
    {
      int lvl = mgr.get_upgrade_level(upg.id);
      if (lvl > 0) { has_any_level = true; break; }
    }

    if (!has_any_level)
    {
      ImGui::TextDisabled("  No upgrades picked yet.");
    }
    else
    {
      if (ImGui::BeginTable("##levels_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
      {
        ImGui::TableSetupColumn("Upgrade", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableHeadersRow();

        for (auto& upg : settings.upgrades)
        {
          int lvl = mgr.get_upgrade_level(upg.id);
          if (lvl <= 0) continue;

          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::Text("%s", upg.display_name.c_str());
          ImGui::TableNextColumn();
          ImGui::Text("%s", upg.stat_name.c_str());
          ImGui::TableNextColumn();
          ImGui::Text("%d", lvl);
          ImGui::TableNextColumn();
          ImGui::Text("%d", upg.max_level);
        }

        ImGui::EndTable();
      }
    }

    // ---- Reset ----
    ImGui::Spacing();
    if (ImGui::Button("Reset Upgrades"))
    {
      mgr.reset();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Resets all upgrade state (points, levels, cards).");
      ImGui::Text("Does NOT revert player stats to pre-upgrade values.");
      ImGui::Text("Reload the scene to fully reset player stats.");
      ImGui::EndTooltip();
    }
  }

  ImGui::End();
}
