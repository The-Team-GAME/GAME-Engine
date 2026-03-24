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
#include <unordered_set>
#include "../FileIO/Settings.h"
#include "../FileIO/SettingsManager.h"
#include "../UpgradeManager.h"
#include "../UpgradeUIStateMachine.h"

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

  // ---- Rarity color helper ----
  auto get_rarity_color = [&](const std::string& rarity) -> ImVec4 {
    // Check rarity tiers in order; assign colors by position
    for (int ri = 0; ri < static_cast<int>(settings.rarities.size()); ++ri)
    {
      if (settings.rarities[ri].name == rarity)
      {
        constexpr ImVec4 tier_colors[] = {
          {0.7f, 0.7f, 0.7f, 1.0f},   // 0: Common — grey
          {0.3f, 0.5f, 1.0f, 1.0f},   // 1: Rare — blue
          {0.7f, 0.3f, 1.0f, 1.0f},   // 2: Epic — purple
          {1.0f, 0.75f, 0.0f, 1.0f},  // 3: Legendary — gold
          {1.0f, 0.3f, 0.3f, 1.0f},   // 4: Mythic — red
          {0.0f, 1.0f, 0.8f, 1.0f},   // 5+: teal
        };
        int idx = (ri < 6) ? ri : 5;
        return tier_colors[idx];
      }
    }
    return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // unknown rarity — dim grey
    };

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
  // Section 2: Rarity Tiers (inline table)
  // ==================================================================================
  if (ImGui::CollapsingHeader("Rarity Tiers"))
  {
    float total_weight = 0.0f;
    for (auto& r : settings.rarities)
      total_weight += r.drop_rate;

    bool rarities_changed = false;
    int rarity_remove_idx = -1;
    static int rarity_pending_delete = -1;

    // Spreadsheet-style: transparent widget backgrounds, subtle hover/active
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 3.0f));

    if (ImGui::BeginTable("##rarity_table", 4,
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthFixed, 90.0f);
      ImGui::TableSetupColumn("%", ImGuiTableColumnFlags_WidthFixed, 55.0f);
      ImGui::TableSetupColumn("##del", ImGuiTableColumnFlags_WidthFixed, 25.0f);
      ImGui::TableHeadersRow();

      for (int i = 0; i < static_cast<int>(settings.rarities.size()); ++i)
      {
        ImGui::PushID(i);
        auto& rarity = settings.rarities[i];
        ImGui::TableNextRow();

        // Name
        ImGui::TableNextColumn();
        char name_buf[64]{};
        strncpy_s(name_buf, rarity.name.c_str(), sizeof(name_buf) - 1);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##rn", name_buf, sizeof(name_buf)))
        {
          rarity.name = name_buf;
          rarities_changed = true;
        }

        // Weight
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::DragFloat("##rw", &rarity.drop_rate, 0.01f, 0.001f, 100.0f, "%.3f"))
          rarities_changed = true;

        // Percentage
        ImGui::TableNextColumn();
        float pct = (total_weight > 0.0f) ? (rarity.drop_rate / total_weight * 100.0f) : 0.0f;
        ImGui::TextDisabled("%.1f%%", pct);

        // Delete
        ImGui::TableNextColumn();
        if (ImGui::SmallButton("X"))
          rarity_pending_delete = i;

        ImGui::PopID();
      }

      ImGui::EndTable();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Confirmation popup for rarity deletion
    if (rarity_pending_delete >= 0)
      ImGui::OpenPopup("Delete Rarity?");

    if (ImGui::BeginPopupModal("Delete Rarity?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      if (rarity_pending_delete >= 0 && rarity_pending_delete < static_cast<int>(settings.rarities.size()))
        ImGui::Text("Delete rarity \"%s\"?", settings.rarities[rarity_pending_delete].name.c_str());
      ImGui::Separator();
      if (ImGui::Button("Delete", ImVec2(120, 0)))
      {
        rarity_remove_idx = rarity_pending_delete;
        rarity_pending_delete = -1;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0)))
      {
        rarity_pending_delete = -1;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (rarity_remove_idx >= 0)
    {
      settings.rarities.erase(settings.rarities.begin() + rarity_remove_idx);
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
  // Section 3: Upgrade Definitions (spreadsheet table)
  // ==================================================================================
  if (ImGui::CollapsingHeader("Upgrade Definitions", ImGuiTreeNodeFlags_DefaultOpen))
  {
    bool upgrades_changed = false;
    int remove_idx = -1;
    static int upgrade_pending_delete = -1;
    int duplicate_idx = -1;
    int move_up_idx = -1;
    int move_down_idx = -1;

    // ---- Build validation data ----
    std::unordered_set<std::string> seen_ids;
    auto registered_stats = mgr.get_registered_stats();
    std::unordered_set<std::string> registered_set(registered_stats.begin(), registered_stats.end());
    std::unordered_set<std::string> rarity_names;
    for (auto& r : settings.rarities)
      rarity_names.insert(r.name);

    // Pre-scan for duplicate IDs
    std::unordered_set<std::string> duplicate_ids;
    {
      std::unordered_set<std::string> id_check;
      for (auto& upg : settings.upgrades)
      {
        if (!id_check.insert(upg.id).second)
          duplicate_ids.insert(upg.id);
      }
    }

    const int col_count = 10; // ID, Name, Stat, Link, Rarity, Type, Value, MaxLvl, Actions, Warnings

    // Spreadsheet-style: transparent widget backgrounds, subtle hover/active
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 3.0f));

    if (ImGui::BeginTable("##upgrades_table", col_count,
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable
      | ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 110.0f);
      ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 120.0f);
      ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthFixed, 80.0f);
      ImGui::TableSetupColumn("Link", ImGuiTableColumnFlags_WidthFixed, 20.0f);
      ImGui::TableSetupColumn("Rarity", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 95.0f);
      ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 90.0f);
      ImGui::TableSetupColumn("Max Lvl", ImGuiTableColumnFlags_WidthFixed, 60.0f);
      ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, 75.0f);
      ImGui::TableSetupColumn("##warn", ImGuiTableColumnFlags_WidthFixed, 20.0f);
      ImGui::TableHeadersRow();

      for (int i = 0; i < static_cast<int>(settings.upgrades.size()); ++i)
      {
        ImGui::PushID(i + 2000);
        auto& upg = settings.upgrades[i];

        // ---- Validation for this row ----
        bool has_warning = false;
        std::string warning_text;
        if (upg.id.empty())
        {
          has_warning = true; warning_text += "Empty ID\n";
        }
        if (duplicate_ids.count(upg.id))
        {
          has_warning = true; warning_text += "Duplicate ID\n";
        }
        if (upg.display_name.empty())
        {
          has_warning = true; warning_text += "Empty display name\n";
        }
        if (!registered_set.count(upg.stat_name))
        {
          has_warning = true; warning_text += "No applicator for stat '" + upg.stat_name + "'\n";
        }
        if (!rarity_names.count(upg.rarity))
        {
          has_warning = true; warning_text += "Unknown rarity '" + upg.rarity + "'\n";
        }
        if (upg.type == UpgradeType::Multiplier && upg.value <= 0.0f)
        {
          has_warning = true; warning_text += "Multiplier value <= 0\n";
        }

        ImGui::TableNextRow();

        // Tint entire row if warning
        if (has_warning)
          ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(180, 80, 30, 40));

        // ID
        ImGui::TableNextColumn();
        {
          char buf[64]{};
          strncpy_s(buf, upg.id.c_str(), sizeof(buf) - 1);
          ImGui::SetNextItemWidth(-1);
          if (ImGui::InputText("##id", buf, sizeof(buf)))
          {
            upg.id = buf;
            upgrades_changed = true;
          }
        }

        // Display Name
        ImGui::TableNextColumn();
        {
          char buf[128]{};
          strncpy_s(buf, upg.display_name.c_str(), sizeof(buf) - 1);
          ImGui::SetNextItemWidth(-1);
          if (ImGui::InputText("##dn", buf, sizeof(buf)))
          {
            upg.display_name = buf;
            upgrades_changed = true;
          }
        }

        // Stat Name (dropdown of registered stats)
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##sn", upg.stat_name.c_str()))
        {
          auto registered = mgr.get_registered_stats();
          for (auto& s : registered)
          {
            bool selected = (upg.stat_name == s);
            if (ImGui::Selectable(s.c_str(), selected))
            {
              upg.stat_name = s;
              upgrades_changed = true;
            }
            if (selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }

        // Applicator indicator (green dot / red dot)
        ImGui::TableNextColumn();
        if (mgr.has_stat_applicator(upg.stat_name))
          ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "*");
        else
          ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.2f, 1.0f), "!");

        // Rarity combo — color-coded
        ImGui::TableNextColumn();
        {
          ImVec4 rc = get_rarity_color(upg.rarity);
          ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(
            static_cast<int>(rc.x * 255 * 0.25f),
            static_cast<int>(rc.y * 255 * 0.25f),
            static_cast<int>(rc.z * 255 * 0.25f), 60));
          ImGui::SetNextItemWidth(-1);
          ImGui::PushStyleColor(ImGuiCol_Text, rc);
          if (ImGui::BeginCombo("##rar", upg.rarity.c_str()))
          {
            ImGui::PopStyleColor(); // pop text color so list items are normal
            for (auto& r : settings.rarities)
            {
              ImVec4 item_col = get_rarity_color(r.name);
              ImGui::PushStyleColor(ImGuiCol_Text, item_col);
              bool selected = (upg.rarity == r.name);
              if (ImGui::Selectable(r.name.c_str(), selected))
              {
                upg.rarity = r.name;
                upgrades_changed = true;
              }
              if (selected)
                ImGui::SetItemDefaultFocus();
              ImGui::PopStyleColor();
            }
            ImGui::EndCombo();
          }
          else
          {
            ImGui::PopStyleColor(); // pop text color if combo not opened
          }
        }

        // Type combo
        ImGui::TableNextColumn();
        {
          const char* type_names[] = { "Flat", "Multiplier" };
          int current_type = static_cast<int>(upg.type);
          ImGui::SetNextItemWidth(-1);
          if (ImGui::Combo("##tp", &current_type, type_names, 2))
          {
            upg.type = static_cast<UpgradeType>(current_type);
            upgrades_changed = true;
          }
        }

        // Value
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        if (upg.type == UpgradeType::Flat)
        {
          if (ImGui::DragFloat("##val", &upg.value, 0.5f, 0.0f, 10000.0f, "+%.1f"))
            upgrades_changed = true;
        }
        else
        {
          if (ImGui::DragFloat("##val", &upg.value, 0.01f, 0.01f, 100.0f, "x%.2f"))
            upgrades_changed = true;
        }

        // Max Level
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::DragInt("##ml", &upg.max_level, 0.1f, 1, 99))
          upgrades_changed = true;

        // Action buttons: move-up, move-down, duplicate, delete
        ImGui::TableNextColumn();
        {
          bool is_first = (i == 0);
          bool is_last = (i == static_cast<int>(settings.upgrades.size()) - 1);

          if (is_first) ImGui::BeginDisabled();
          if (ImGui::SmallButton("^"))
            move_up_idx = i;
          if (is_first) ImGui::EndDisabled();

          ImGui::SameLine(0, 2);
          if (is_last) ImGui::BeginDisabled();
          if (ImGui::SmallButton("v"))
            move_down_idx = i;
          if (is_last) ImGui::EndDisabled();

          ImGui::SameLine(0, 2);
          if (ImGui::SmallButton("D"))
            duplicate_idx = i;
          if (ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip(); ImGui::Text("Duplicate"); ImGui::EndTooltip();
          }

          ImGui::SameLine(0, 2);
          if (ImGui::SmallButton("X"))
            upgrade_pending_delete = i;
        }

        // Warning indicator
        ImGui::TableNextColumn();
        if (has_warning)
        {
          ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "!");
          if (ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(warning_text.c_str());
            ImGui::EndTooltip();
          }
        }

        ImGui::PopID();
      }

      ImGui::EndTable();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Confirmation popup for upgrade deletion
    if (upgrade_pending_delete >= 0)
      ImGui::OpenPopup("Delete Upgrade?");

    if (ImGui::BeginPopupModal("Delete Upgrade?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      if (upgrade_pending_delete >= 0 && upgrade_pending_delete < static_cast<int>(settings.upgrades.size()))
      {
        auto& del_upg = settings.upgrades[upgrade_pending_delete];
        ImGui::Text("Delete upgrade \"%s\" (%s)?", del_upg.display_name.c_str(), del_upg.id.c_str());
      }
      ImGui::Separator();
      if (ImGui::Button("Delete", ImVec2(120, 0)))
      {
        remove_idx = upgrade_pending_delete;
        upgrade_pending_delete = -1;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0)))
      {
        upgrade_pending_delete = -1;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (remove_idx >= 0)
    {
      settings.upgrades.erase(settings.upgrades.begin() + remove_idx);
      upgrades_changed = true;
    }

    // Handle duplicate
    if (duplicate_idx >= 0 && duplicate_idx < static_cast<int>(settings.upgrades.size()))
    {
      UpgradeDefinition dupe = settings.upgrades[duplicate_idx];
      dupe.id += "_copy";
      dupe.display_name += " (Copy)";
      settings.upgrades.insert(settings.upgrades.begin() + duplicate_idx + 1, dupe);
      upgrades_changed = true;
    }

    // Handle move up
    if (move_up_idx > 0 && move_up_idx < static_cast<int>(settings.upgrades.size()))
    {
      std::swap(settings.upgrades[move_up_idx], settings.upgrades[move_up_idx - 1]);
      upgrades_changed = true;
    }

    // Handle move down
    if (move_down_idx >= 0 && move_down_idx < static_cast<int>(settings.upgrades.size()) - 1)
    {
      std::swap(settings.upgrades[move_down_idx], settings.upgrades[move_down_idx + 1]);
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

    // Legend for the Link column
    ImGui::SameLine();
    ImGui::TextDisabled("Link: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "*");
    ImGui::SameLine();
    ImGui::TextDisabled("= applicator registered  ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.2f, 1.0f), "!");
    ImGui::SameLine();
    ImGui::TextDisabled("= no applicator");

    if (upgrades_changed)
      SettingsManager<UpgradeSettings>::save();
  }

  // ==================================================================================
  // Section 3.5: Upgrade Levels (Debug)
  // ==================================================================================
  if (ImGui::CollapsingHeader("Upgrade Levels (Debug)", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TextDisabled("Directly set each upgrade's current level. Does not apply stat effects.");
    ImGui::Spacing();

    if (ImGui::Button("Reset All Levels"))
      mgr.reset();

    ImGui::Spacing();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 3));
    if (ImGui::BeginTable("##debug_levels_tbl", 4,
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("Upgrade", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Rarity", ImGuiTableColumnFlags_WidthFixed, 70.0f);
      ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 130.0f);
      ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 40.0f);
      ImGui::TableHeadersRow();

      for (auto& upg : settings.upgrades)
      {
        ImGui::PushID(upg.id.c_str());
        ImGui::TableNextRow();

        ImVec4 rarity_col = get_rarity_color(upg.rarity);

        // Name
        ImGui::TableNextColumn();
        ImGui::TextColored(rarity_col, "%s", upg.display_name.c_str());

        // Rarity
        ImGui::TableNextColumn();
        ImGui::TextColored(rarity_col, "%s", upg.rarity.c_str());

        // Level editor: [-] [DragInt] [+]
        ImGui::TableNextColumn();
        {
          int lvl = mgr.get_upgrade_level(upg.id);
          int new_lvl = lvl;
          bool changed = false;

          if (ImGui::SmallButton("-")) { new_lvl = std::max(0, lvl - 1);               changed = true; }
          ImGui::SameLine(0, 2);
          ImGui::SetNextItemWidth(60.0f);
          if (ImGui::DragInt("##lvl", &new_lvl, 0.1f, 0, upg.max_level))                changed = true;
          ImGui::SameLine(0, 2);
          if (ImGui::SmallButton("+")) { new_lvl = std::min(upg.max_level, lvl + 1);   changed = true; }

          if (changed)
            mgr.set_upgrade_level(upg.id, new_lvl);
        }

        // Max
        ImGui::TableNextColumn();
        ImGui::Text("%d", upg.max_level);

        ImGui::PopID();
      }

      ImGui::EndTable();
    }
    ImGui::PopStyleVar();
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

    // ---- Card Animation Effects ----
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Card Animation Effects"))
    {
      auto& anim = SettingsManager<CardAnimSettings>::get_mutable();

      ImGui::Checkbox("Flip Reveal", &anim.enable_flip_reveal);
      if (anim.enable_flip_reveal)
      {
        ImGui::Indent();
        ImGui::DragFloat("Flip Duration", &anim.flip_duration, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Flip Stagger", &anim.flip_stagger, 0.01f, 0.0f, 1.0f);
        ImGui::Unindent();
      }

      ImGui::Text("Card Intro");
      ImGui::Indent();
      ImGui::DragFloat("Intro Duration", &anim.intro_duration, 0.01f, 0.01f, 3.0f);
      ImGui::DragFloat("Intro Offset", &anim.intro_start_offset, 5.0f, 0.0f, 800.0f, "%.1f px");
      ImGui::Unindent();

      ImGui::Checkbox("Idle Float", &anim.enable_idle_float);
      if (anim.enable_idle_float)
      {
        ImGui::Indent();
        ImGui::DragFloat("Float Amplitude", &anim.float_amplitude, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Float Speed", &anim.float_speed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Float Phase Offset", &anim.float_phase_offset, 0.05f, 0.0f, 6.28f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Hover Lift", &anim.enable_hover_lift);
      if (anim.enable_hover_lift)
      {
        ImGui::Indent();
        ImGui::DragFloat("Hover Lift Y", &anim.hover_lift_y, 0.5f, -100.0f, 0.0f);
        ImGui::DragFloat("Hover Scale Boost", &anim.hover_scale_boost, 0.01f, 1.0f, 2.0f);
        ImGui::DragFloat("Hover Speed", &anim.hover_speed, 0.1f, 0.1f, 50.0f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Idle Tilt", &anim.enable_idle_tilt);
      if (anim.enable_idle_tilt)
      {
        ImGui::Indent();
        ImGui::DragFloat("Idle Tilt Amplitude", &anim.idle_tilt_amplitude, 0.1f, 0.0f, 30.0f);
        ImGui::DragFloat("Idle Tilt Speed", &anim.idle_tilt_speed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Idle Tilt Phase", &anim.idle_tilt_phase, 0.05f, 0.0f, 6.28f);
        ImGui::DragFloat("Idle Tilt Lerp", &anim.idle_tilt_lerp_speed, 0.1f, 0.1f, 50.0f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Hover Tilt", &anim.enable_hover_tilt);
      if (anim.enable_hover_tilt)
      {
        ImGui::Indent();
        ImGui::DragFloat("Max Tilt Degrees", &anim.max_tilt_deg, 0.1f, 0.0f, 45.0f);
        ImGui::DragFloat("Tilt Speed", &anim.tilt_speed, 0.1f, 0.1f, 50.0f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Selection Stamp", &anim.enable_selection_stamp);
      if (anim.enable_selection_stamp)
      {
        ImGui::Indent();
        ImGui::DragFloat("Stamp Duration", &anim.stamp_duration, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Stamp Squish", &anim.stamp_squish, 0.01f, 0.1f, 1.0f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Card Arc", &anim.enable_card_arc);
      if (anim.enable_card_arc)
      {
        ImGui::Indent();
        ImGui::DragFloat("Arc Duration", &anim.arc_duration, 0.01f, 0.05f, 3.0f);
        ImGui::DragFloat("Arc Peak", &anim.arc_peak, 1.0f, -500.0f, 500.0f);
        ImGui::DragFloat("Arc Spin Degrees", &anim.arc_spin_deg, 0.5f, 0.0f, 360.0f);
        ImGui::DragFloat("Smash Duration", &anim.smash_duration, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Linger Duration", &anim.linger_duration, 0.01f, 0.0f, 5.0f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Burst Particles", &anim.enable_burst_particles);
      if (anim.enable_burst_particles)
      {
        ImGui::Indent();
        ImGui::DragFloat("Particle Size", &anim.particle_size, 0.5f, 1.0f, 100.0f);
        ImGui::DragFloat("Particle Gravity", &anim.particle_gravity, 10.0f, 0.0f, 5000.0f);
        ImGui::DragFloat("Speed Min", &anim.particle_speed_min, 1.0f, 0.0f, 2000.0f);
        ImGui::DragFloat("Speed Max", &anim.particle_speed_max, 1.0f, 0.0f, 2000.0f);
        ImGui::DragFloat("Life Min", &anim.particle_life_min, 0.01f, 0.01f, 5.0f);
        ImGui::DragFloat("Life Max", &anim.particle_life_max, 0.01f, 0.01f, 5.0f);
        ImGui::DragInt("Count Min", &anim.particle_count_min, 1, 1, 100);
        ImGui::DragInt("Count Max", &anim.particle_count_max, 1, 1, 100);
        ImGui::DragFloat("Angle Min", &anim.particle_angle_min, 0.05f, 0.0f, 6.28f);
        ImGui::DragFloat("Angle Max", &anim.particle_angle_max, 0.05f, 0.0f, 9.42f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Card Stack", &anim.enable_card_stack);
      if (anim.enable_card_stack)
      {
        ImGui::Indent();
        ImGui::DragFloat("Stack Offset", &anim.card_stack_offset, 0.5f, 0.0f, 100.0f);
        ImGui::DragFloat("Pre-Shift Duration", &anim.pre_shift_duration, 0.01f, 0.05f, 2.0f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Mini-Card Wave", &anim.enable_mini_wave);
      if (anim.enable_mini_wave)
      {
        ImGui::Indent();
        ImGui::DragFloat("Wave Amplitude", &anim.mini_wave_amplitude, 0.1f, 0.0f, 20.0f);
        ImGui::DragFloat("Wave Speed", &anim.mini_wave_speed, 0.05f, 0.0f, 5.0f);
        ImGui::DragFloat("Wave Phase", &anim.mini_wave_phase, 0.05f, 0.0f, 6.28f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Mini-Card Tilt", &anim.enable_mini_tilt);
      if (anim.enable_mini_tilt)
      {
        ImGui::Indent();
        ImGui::DragFloat("Mini Tilt Amplitude", &anim.mini_tilt_amplitude, 0.1f, 0.0f, 20.0f);
        ImGui::DragFloat("Mini Tilt Speed", &anim.mini_tilt_speed, 0.05f, 0.0f, 5.0f);
        ImGui::DragFloat("Mini Tilt Phase", &anim.mini_tilt_phase, 0.05f, 0.0f, 6.28f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Mini-Card Hover", &anim.enable_mini_hover);
      if (anim.enable_mini_hover)
      {
        ImGui::Indent();
        ImGui::DragFloat("Mini Hover Lift", &anim.mini_hover_lift, 0.5f, -50.0f, 50.0f);
        ImGui::DragFloat("Mini Hover Scale", &anim.mini_hover_scale, 0.05f, 1.0f, 3.0f);
        ImGui::DragFloat("Mini Hover Speed", &anim.mini_hover_speed, 0.5f, 1.0f, 30.0f);
        ImGui::DragFloat("Mini Hover Tilt", &anim.mini_hover_tilt_deg, 0.5f, 0.0f, 30.0f);
        ImGui::DragFloat("Mini Tilt Speed##hover", &anim.mini_hover_tilt_speed, 0.5f, 1.0f, 30.0f);
        ImGui::Unindent();
      }

      ImGui::Checkbox("Smash Ripple", &anim.enable_smash_ripple);
      if (anim.enable_smash_ripple)
      {
        ImGui::Indent();
        ImGui::DragFloat("Ripple Amplitude", &anim.ripple_amplitude, 0.5f, 0.0f, 50.0f);
        ImGui::DragFloat("Ripple Speed", &anim.ripple_speed, 10.0f, 10.0f, 2000.0f);
        ImGui::DragFloat("Ripple Decay", &anim.ripple_decay, 0.1f, 0.1f, 20.0f);
        ImGui::Unindent();
      }

      ImGui::Spacing();
      if (ImGui::Button("Save Anim Settings"))
        SettingsManager<CardAnimSettings>::save();
      ImGui::SameLine();
      if (ImGui::Button("Reset Anim Settings"))
      {
        SettingsManager<CardAnimSettings>::reset();
        SettingsManager<CardAnimSettings>::save();
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
